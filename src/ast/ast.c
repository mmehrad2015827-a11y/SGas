#include "ast.h"

static Node* node_new(NodeType t, int line) {
    Node* n = (Node*)sgas_alloc(sizeof(Node));
    memset(n, 0, sizeof(Node));
    n->type = t;
    n->line = line;
    return n;
}

Node* ast_program(Node** stmts, int count) {
    Node* n = node_new(NODE_PROGRAM, 0);
    n->as.program.stmts = stmts;
    n->as.program.count = count;
    return n;
}

Node* ast_let(const char* name, Node* init, int line) {
    Node* n = node_new(NODE_LET, line);
    n->as.let.name = name;
    n->as.let.init = init;
    return n;
}

Node* ast_func(const char* name, const char** params, int param_count,
               Node* body, int line) {
    Node* n = node_new(NODE_FUNC, line);
    n->as.func.name = name;
    n->as.func.params = params;
    n->as.func.param_count = param_count;
    n->as.func.body = body;
    return n;
}

Node* ast_if(Node* cond, Node* then_b, Node* else_b, int line) {
    Node* n = node_new(NODE_IF, line);
    n->as.ifs.cond = cond;
    n->as.ifs.then_branch = then_b;
    n->as.ifs.else_branch = else_b;
    return n;
}

Node* ast_while(Node* cond, Node* body, int line) {
    Node* n = node_new(NODE_WHILE, line);
    n->as.whiles.cond = cond;
    n->as.whiles.body = body;
    return n;
}

Node* ast_return(Node* value, int line) {
    Node* n = node_new(NODE_RETURN, line);
    n->as.ret.value = value;
    return n;
}

Node* ast_block(Node** stmts, int count, int line) {
    Node* n = node_new(NODE_BLOCK, line);
    n->as.block.stmts = stmts;
    n->as.block.count = count;
    return n;
}

Node* ast_expr_stmt(Node* expr, int line) {
    Node* n = node_new(NODE_EXPR_STMT, line);
    n->as.expr_stmt.expr = expr;
    return n;
}

Node* ast_binary(int op, Node* l, Node* r, int line) {
    Node* n = node_new(NODE_BINARY, line);
    n->as.binary.op = op; n->as.binary.left = l; n->as.binary.right = r;
    return n;
}

Node* ast_unary(int op, Node* operand, int line) {
    Node* n = node_new(NODE_UNARY, line);
    n->as.unary.op = op; n->as.unary.operand = operand;
    return n;
}

Node* ast_call(Node* callee, Node** args, int arg_count, int line) {
    Node* n = node_new(NODE_CALL, line);
    n->as.call.callee = callee; n->as.call.args = args; n->as.call.arg_count = arg_count;
    return n;
}

Node* ast_ident(const char* name, int line) {
    Node* n = node_new(NODE_IDENT, line);
    n->as.ident.name = name;
    return n;
}

Node* ast_number(double v, int line) {
    Node* n = node_new(NODE_LITERAL, line);
    n->as.literal.type_tag = 0; n->as.literal.num = v;
    return n;
}

Node* ast_string(const char* s, int line) {
    Node* n = node_new(NODE_LITERAL, line);
    n->as.literal.type_tag = 1; n->as.literal.str = sgas_strdup(s);
    return n;
}

Node* ast_bool(int v, int line) {
    Node* n = node_new(NODE_LITERAL, line);
    n->as.literal.type_tag = 2; n->as.literal.boolean = v;
    return n;
}

Node* ast_nil(int line) {
    Node* n = node_new(NODE_LITERAL, line);
    n->as.literal.type_tag = 3;
    return n;
}

Node* ast_assign(const char* name, Node* value, int line) {
    Node* n = node_new(NODE_ASSIGN, line);
    n->as.assign.name = name; n->as.assign.value = value;
    return n;
}

/* Simple recursive free (best-effort; program exit reclaims anyway). */
void ast_free(Node* n) {
    if (!n) return;
    switch (n->type) {
        case NODE_PROGRAM: for (int i=0;i<n->as.program.count;i++) ast_free(n->as.program.stmts[i]);
                           free(n->as.program.stmts); break;
        case NODE_BLOCK:   for (int i=0;i<n->as.block.count;i++)   ast_free(n->as.block.stmts[i]);
                           free(n->as.block.stmts); break;
        case NODE_LET:     ast_free(n->as.let.init); break;
        case NODE_FUNC:    ast_free(n->as.func.body); break;
        case NODE_IF:      ast_free(n->as.ifs.cond); ast_free(n->as.ifs.then_branch);
                           ast_free(n->as.ifs.else_branch); break;
        case NODE_WHILE:   ast_free(n->as.whiles.cond); ast_free(n->as.whiles.body); break;
        case NODE_RETURN:  ast_free(n->as.ret.value); break;
        case NODE_EXPR_STMT: ast_free(n->as.expr_stmt.expr); break;
        case NODE_BINARY:  ast_free(n->as.binary.left); ast_free(n->as.binary.right); break;
        case NODE_UNARY:   ast_free(n->as.unary.operand); break;
        case NODE_CALL:    ast_free(n->as.call.callee);
                           for (int i=0;i<n->as.call.arg_count;i++) ast_free(n->as.call.args[i]);
                           free(n->as.call.args); break;
        case NODE_ASSIGN:  ast_free(n->as.assign.value); break;
        case NODE_IDENT: case NODE_LITERAL: break;
    }
    free(n);
}