#include "compiler.h"
#include "../lexer/lexer.h"   /* برای TOK_* */

void chunk_init(Chunk* c) {
    c->count = c->capacity = 0;
    c->code = NULL; c->lines = NULL;
    c->constants = NULL;
    c->constants_count = c->constants_cap = 0;
}

void chunk_free(Chunk* c) {
    free(c->code);
    free(c->lines);
    free(c->constants);
    c->code = NULL; c->lines = NULL; c->constants = NULL;
    c->count = c->capacity = 0;
    c->constants_count = c->constants_cap = 0;
}

int chunk_add_constant(Chunk* c, Value v) {
    if (c->constants_count == c->constants_cap) {
        c->constants_cap = c->constants_cap ? c->constants_cap * 2 : 8;
        c->constants = (Value*)sgas_realloc(c->constants,
                                            sizeof(Value) * c->constants_cap);
    }
    c->constants[c->constants_count] = v;
    return c->constants_count++;
}

void chunk_write(Chunk* c, uint8_t byte, int line) {
    if (c->count == c->capacity) {
        c->capacity = c->capacity ? c->capacity * 2 : 16;
        c->code  = (uint8_t*)sgas_realloc(c->code,  c->capacity);
        c->lines = (int*)    sgas_realloc(c->lines, sizeof(int) * c->capacity);
    }
    c->code[c->count]  = byte;
    c->lines[c->count] = line;
    c->count++;
}

int chunk_emit_constant(Chunk* c, Value v, int line) {
    int idx = chunk_add_constant(c, v);
    chunk_write(c, OP_CONSTANT, line);
    chunk_write(c, (uint8_t)idx, line);
    return idx;
}

/* ---------- state ---------- */

typedef struct {
    const char** names;
    int          count;
    int          cap;
} LocalList;

static void locals_init(LocalList* l) { l->names = NULL; l->count = 0; l->cap = 0; }

static int locals_add(LocalList* l, const char* n) {
    if (l->count == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 8;
        l->names = (const char**)sgas_realloc(l->names, sizeof(char*) * l->cap);
    }
    l->names[l->count] = n;
    return l->count++;
}

static int locals_find(LocalList* l, const char* n) {
    for (int i = l->count - 1; i >= 0; i--)
        if (strcmp(l->names[i], n) == 0) return i;
    return -1;
}

static void locals_free(LocalList* l) { free(l->names); }

typedef struct {
    Chunk*       chunk;
    ObjFunction* fn;
    int          scope_depth;
    bool         had_error;
    char         err[256];
} Compiler;

static void emit(Compiler* c, uint8_t b, int line) { chunk_write(c->chunk, b, line); }

static void emit_u16(Compiler* c, uint16_t v, int line) {
    chunk_write(c->chunk, (uint8_t)(v & 0xff), line);
    chunk_write(c->chunk, (uint8_t)((v >> 8) & 0xff), line);
}

static void error(Compiler* c, int line, const char* msg) {
    if (c->had_error) return;
    c->had_error = true;
    snprintf(c->err, sizeof(c->err), "Compile error at line %d: %s", line, msg);
}

/* Forward declarations */
static void compile_node(Compiler* c, Node* n, LocalList* locals);
static void compile_expr(Compiler* c, Node* n, LocalList* locals);
static void compile_function(Compiler* c, Node* n, LocalList* parent_locals);

/* ---------- expression compile ---------- */

static void compile_expr(Compiler* c, Node* n, LocalList* locals) {
    if (!n) { emit(c, OP_NIL, 0); return; }
    switch (n->type) {
        case NODE_LITERAL:
            switch (n->as.literal.type_tag) {
                case 0:
                    chunk_emit_constant(c->chunk, NUM_VAL(n->as.literal.num), n->line);
                    break;
                case 1: {
                    ObjString* s = obj_string_new(n->as.literal.str,
                                                  strlen(n->as.literal.str));
                    chunk_emit_constant(c->chunk, OBJ_VAL(s), n->line);
                    break;
                }
                case 2:
                    emit(c, n->as.literal.boolean ? OP_TRUE : OP_FALSE, n->line);
                    break;
                case 3:
                    emit(c, OP_NIL, n->line);
                    break;
            }
            break;

        case NODE_IDENT: {
            int slot = locals_find(locals, n->as.ident.name);
            if (slot >= 0) {
                emit(c, OP_GET_LOCAL, n->line);
                emit(c, (uint8_t)slot, n->line);
            } else {
                ObjString* s = obj_string_new(n->as.ident.name,
                                              strlen(n->as.ident.name));
                int idx = chunk_add_constant(c->chunk, OBJ_VAL(s));
                emit(c, OP_GET_GLOBAL, n->line);
                emit(c, (uint8_t)idx, n->line);
            }
            break;
        }

        case NODE_ASSIGN: {
            compile_expr(c, n->as.assign.value, locals);
            int slot = locals_find(locals, n->as.assign.name);
            if (slot >= 0) {
                emit(c, OP_SET_LOCAL, n->line);
                emit(c, (uint8_t)slot, n->line);
            } else {
                ObjString* s = obj_string_new(n->as.assign.name,
                                              strlen(n->as.assign.name));
                int idx = chunk_add_constant(c->chunk, OBJ_VAL(s));
                emit(c, OP_SET_GLOBAL, n->line);
                emit(c, (uint8_t)idx, n->line);
            }
            break;
        }

        case NODE_BINARY: {
            int op = n->as.binary.op;

            if (op == TOK_AND || op == TOK_OR) {
                compile_expr(c, n->as.binary.left, locals);
                if (op == TOK_AND) {
                    int j = c->chunk->count;
                    emit(c, OP_JUMP_IF_FALSE, n->line);
                    emit_u16(c, 0xFFFF, n->line);
                    emit(c, OP_POP, n->line);
                    compile_expr(c, n->as.binary.right, locals);
                    int end = c->chunk->count;
                    int off = end - j - 3;
                    c->chunk->code[j + 1] = (uint8_t)(off & 0xff);
                    c->chunk->code[j + 2] = (uint8_t)((off >> 8) & 0xff);
                } else {
                    int j = c->chunk->count;
                    emit(c, OP_JUMP_IF_FALSE, n->line);
                    emit_u16(c, 0xFFFF, n->line);
                    int j2 = c->chunk->count;
                    emit(c, OP_JUMP, n->line);
                    emit_u16(c, 0xFFFF, n->line);
                    int mid = c->chunk->count;
                    int off1 = mid - j - 3;
                    c->chunk->code[j + 1] = (uint8_t)(off1 & 0xff);
                    c->chunk->code[j + 2] = (uint8_t)((off1 >> 8) & 0xff);
                    emit(c, OP_POP, n->line);
                    compile_expr(c, n->as.binary.right, locals);
                    int end = c->chunk->count;
                    int off2 = end - j2 - 3;
                    c->chunk->code[j2 + 1] = (uint8_t)(off2 & 0xff);
                    c->chunk->code[j2 + 2] = (uint8_t)((off2 >> 8) & 0xff);
                }
                break;
            }

            compile_expr(c, n->as.binary.left, locals);
            compile_expr(c, n->as.binary.right, locals);
            switch (op) {
                case TOK_PLUS:    emit(c, OP_ADD, n->line); break;
                case TOK_MINUS:   emit(c, OP_SUB, n->line); break;
                case TOK_STAR:    emit(c, OP_MUL, n->line); break;
                case TOK_SLASH:   emit(c, OP_DIV, n->line); break;
                case TOK_PERCENT: emit(c, OP_MOD, n->line); break;
                case TOK_EQEQ:    emit(c, OP_EQ,  n->line); break;
                case TOK_NEQ:     emit(c, OP_NEQ, n->line); break;
                case TOK_LT:      emit(c, OP_LT,  n->line); break;
                case TOK_GT:      emit(c, OP_GT,  n->line); break;
                case TOK_LE:      emit(c, OP_LE,  n->line); break;
                case TOK_GE:      emit(c, OP_GE,  n->line); break;
                default:          error(c, n->line, "unsupported binary operator");
            }
            break;
        }

        case NODE_UNARY:
            compile_expr(c, n->as.unary.operand, locals);
            if (n->as.unary.op == TOK_MINUS) emit(c, OP_NEG, n->line);
            else if (n->as.unary.op == TOK_NOT) emit(c, OP_NOT, n->line);
            break;

        case NODE_CALL: {
            compile_expr(c, n->as.call.callee, locals);
            for (int i = 0; i < n->as.call.arg_count; i++)
                compile_expr(c, n->as.call.args[i], locals);
            emit(c, OP_CALL, n->line);
            emit(c, (uint8_t)n->as.call.arg_count, n->line);
            break;
        }

        default:
            error(c, n->line, "invalid expression node");
    }
}

/* ---------- statement compile ---------- */

static void compile_block(Compiler* c, Node** stmts, int count, LocalList* locals,
                          bool new_scope) {
    int saved = locals->count;
    for (int i = 0; i < count; i++) {
        compile_node(c, stmts[i], locals);
        if (c->had_error) return;
    }
    if (new_scope) {
        int to_pop = locals->count - saved;
        for (int i = 0; i < to_pop; i++) emit(c, OP_POP, 0);
        locals->count = saved;
    }
}

static void compile_node(Compiler* c, Node* n, LocalList* locals) {
    if (!n) return;
    switch (n->type) {
        case NODE_LET: {
            if (n->as.let.init) {
                compile_expr(c, n->as.let.init, locals);
            } else {
                emit(c, OP_NIL, n->line);
            }
            if (c->scope_depth == 0) {
                ObjString* s = obj_string_new(n->as.let.name, strlen(n->as.let.name));
                int idx = chunk_add_constant(c->chunk, OBJ_VAL(s));
                emit(c, OP_SET_GLOBAL, n->line);
                emit(c, (uint8_t)idx, n->line);
                emit(c, OP_POP, n->line);        /* ← اصلاح شد */
            } else {
                locals_add(locals, n->as.let.name);
            }
            break;
        }

        case NODE_FUNC:
            compile_function(c, n, locals);
            break;

        case NODE_EXPR_STMT:
            compile_expr(c, n->as.expr_stmt.expr, locals);
            emit(c, OP_POP, n->line);
            break;

        case NODE_RETURN:
            if (n->as.ret.value) compile_expr(c, n->as.ret.value, locals);
            else emit(c, OP_NIL, n->line);
            emit(c, OP_RETURN, n->line);
            break;

        case NODE_IF: {
            compile_expr(c, n->as.ifs.cond, locals);
            int j1 = c->chunk->count;
            emit(c, OP_JUMP_IF_FALSE, n->line);
            emit_u16(c, 0xFFFF, n->line);
            compile_node(c, n->as.ifs.then_branch, locals);
            int j2 = c->chunk->count;
            emit(c, OP_JUMP, n->line);
            emit_u16(c, 0xFFFF, n->line);
            int else_start = c->chunk->count;
            int off1 = else_start - j1 - 3;
            c->chunk->code[j1 + 1] = (uint8_t)(off1 & 0xff);
            c->chunk->code[j1 + 2] = (uint8_t)((off1 >> 8) & 0xff);
            if (n->as.ifs.else_branch) compile_node(c, n->as.ifs.else_branch, locals);
            int end = c->chunk->count;
            int off2 = end - j2 - 3;
            c->chunk->code[j2 + 1] = (uint8_t)(off2 & 0xff);
            c->chunk->code[j2 + 2] = (uint8_t)((off2 >> 8) & 0xff);
            break;
        }

        case NODE_WHILE: {
            int loop_start = c->chunk->count;
            compile_expr(c, n->as.whiles.cond, locals);
            int j = c->chunk->count;
            emit(c, OP_JUMP_IF_FALSE, n->line);
            emit_u16(c, 0xFFFF, n->line);
            compile_node(c, n->as.whiles.body, locals);
            int back = c->chunk->count - loop_start + 3;
            emit(c, OP_LOOP, n->line);
            emit_u16(c, (uint16_t)back, n->line);
            int end = c->chunk->count;
            int off = end - j - 3;
            c->chunk->code[j + 1] = (uint8_t)(off & 0xff);
            c->chunk->code[j + 2] = (uint8_t)((off >> 8) & 0xff);
            break;
        }

        case NODE_BLOCK:
            compile_block(c, n->as.block.stmts, n->as.block.count, locals, true);
            break;

        default:
            error(c, n->line, "invalid statement node");
    }
}

static void compile_function(Compiler* c, Node* n, LocalList* parent_locals) {
    Chunk* fn_chunk = (Chunk*)sgas_alloc(sizeof(Chunk));
    chunk_init(fn_chunk);

    ObjFunction* fn = (ObjFunction*)sgas_alloc(sizeof(ObjFunction));
    fn->obj.type      = OBJ_FUNCTION;
    fn->name          = n->as.func.name;
    fn->arity         = n->as.func.param_count;
    fn->upvalue_count = 0;
    fn->chunk         = fn_chunk;

    Chunk*    saved_chunk = c->chunk;
    int       saved_scope = c->scope_depth;
    LocalList fn_locals;   locals_init(&fn_locals);
    for (int i = 0; i < n->as.func.param_count; i++)
        locals_add(&fn_locals, n->as.func.params[i]);

    c->chunk       = fn_chunk;
    c->scope_depth = 1;

    if (n->as.func.body && n->as.func.body->type == NODE_BLOCK) {
        compile_block(c, n->as.func.body->as.block.stmts,
                      n->as.func.body->as.block.count,
                      &fn_locals, false);
    } else {
        compile_node(c, n->as.func.body, &fn_locals);
    }
    /* implicit return nil */
    emit(c, OP_NIL, 0);
    emit(c, OP_RETURN, 0);

    locals_free(&fn_locals);
    c->chunk       = saved_chunk;
    c->scope_depth = saved_scope;

    /* push function object as a constant */
    chunk_emit_constant(c->chunk, OBJ_VAL(fn), n->line);

    if (c->scope_depth == 0) {
        ObjString* name = obj_string_new(n->as.func.name, strlen(n->as.func.name));
        int idx = chunk_add_constant(c->chunk, OBJ_VAL(name));
        emit(c, OP_SET_GLOBAL, n->line);
        emit(c, (uint8_t)idx, n->line);
        emit(c, OP_POP, n->line);            /* ← اصلاح شد */
    } else {
        locals_add(parent_locals, n->as.func.name);
    }
}

/* ---------- entry ---------- */

ObjFunction* compiler_compile(Node* program) {
    Chunk* chunk = (Chunk*)sgas_alloc(sizeof(Chunk));
    chunk_init(chunk);

    ObjFunction* fn = (ObjFunction*)sgas_alloc(sizeof(ObjFunction));
    fn->obj.type      = OBJ_FUNCTION;
    fn->name          = "<main>";
    fn->arity         = 0;
    fn->upvalue_count = 0;
    fn->chunk         = chunk;

    Compiler c;
    c.chunk       = chunk;
    c.fn          = fn;
    c.scope_depth = 0;
    c.had_error   = false;
    c.err[0]      = '\0';

    LocalList top; locals_init(&top);

    if (program && program->type == NODE_PROGRAM) {
        for (int i = 0; i < program->as.program.count; i++) {
            compile_node(&c, program->as.program.stmts[i], &top);
            if (c.had_error) break;
        }
    }

    locals_free(&top);
    emit(&c, OP_HALT, 0);

    if (c.had_error) {
        fprintf(stderr, "%s\n", c.err);
        chunk_free(chunk);
        free(chunk);
        free(fn);
        return NULL;
    }
    return fn;
}

void compiler_free_function(ObjFunction* fn) {
    if (!fn) return;
    if (fn->chunk) {
        chunk_free(fn->chunk);
        free(fn->chunk);
    }
    free(fn);
}