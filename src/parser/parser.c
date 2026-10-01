#include "parser.h"

void parser_init(Parser* p, const char* source) {
    lexer_init(&p->lx, source);
    p->had_error = false;
    p->error_msg[0] = '\0';
    p->current  = lexer_next(&p->lx);
    p->previous = p->current;
}

static void advance(Parser* p) {
    p->previous = p->current;
    p->current  = lexer_next(&p->lx);
    if (p->current.type == TOK_ERROR && !p->had_error) {
        p->had_error = true;
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "Lexer error at line %d", p->current.line);
    }
}

static bool check(Parser* p, TokenType t) { return p->current.type == t; }

static bool match(Parser* p, TokenType t) {
    if (check(p, t)) { advance(p); return true; }
    return false;
}

static bool consume(Parser* p, TokenType t, const char* msg) {
    if (check(p, t)) { advance(p); return true; }
    if (!p->had_error) {
        p->had_error = true;
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "Line %d: expected %s (%s), got %s",
                 p->current.line, token_type_name(t), msg,
                 token_type_name(p->current.type));
    }
    return false;
}

/* --- forward decls --- */
static Node* declaration(Parser* p);
static Node* statement(Parser* p);
static Node* expression(Parser* p);

/* --- statements --- */
static Node* block(Parser* p) {
    int line = p->current.line;
    Node** stmts = NULL; int count = 0, cap = 0;
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        Node* s = declaration(p);
        if (!s) break;
        if (count == cap) { cap = cap ? cap * 2 : 8;
                            stmts = (Node**)sgas_realloc(stmts, sizeof(Node*)*cap); }
        stmts[count++] = s;
    }
    consume(p, TOK_RBRACE, "to close block");
    return ast_block(stmts, count, line);
}

static Node* let_decl(Parser* p) {
    int line = p->previous.line;
    if (!consume(p, TOK_IDENT, "identifier after 'let'")) return NULL;
    const char* name = sgas_strdup(
        (char[]){0}); /* placeholder to avoid warning; replaced below */
    free((void*)name);
    /* Build name from token */
    char buf[128];
    size_t n = p->previous.length < sizeof(buf)-1 ? p->previous.length : sizeof(buf)-1;
    memcpy(buf, p->previous.start, n); buf[n] = '\0';
    name = sgas_strdup(buf);

    Node* init = NULL;
    if (match(p, TOK_EQ)) init = expression(p);
    match(p, TOK_SEMI);
    return ast_let(name, init, line);
}

static Node* func_decl(Parser* p) {
    int line = p->previous.line;
    if (!consume(p, TOK_IDENT, "function name")) return NULL;
    char nbuf[128];
    size_t nlen = p->previous.length < sizeof(nbuf)-1 ? p->previous.length : sizeof(nbuf)-1;
    memcpy(nbuf, p->previous.start, nlen); nbuf[nlen] = '\0';
    const char* name = sgas_strdup(nbuf);

    consume(p, TOK_LPAREN, "(");

    const char** params = NULL; int pcount = 0, pcap = 0;
    if (!check(p, TOK_RPAREN)) {
        do {
            if (!consume(p, TOK_IDENT, "parameter name")) return NULL;
            char pb[128];
            size_t pl = p->previous.length < sizeof(pb)-1 ? p->previous.length : sizeof(pb)-1;
            memcpy(pb, p->previous.start, pl); pb[pl] = '\0';
            if (pcount == pcap) { pcap = pcap ? pcap*2 : 4;
                                  params = (const char**)sgas_realloc(params, sizeof(char*)*pcap); }
            params[pcount++] = sgas_strdup(pb);
        } while (match(p, TOK_COMMA));
    }
    consume(p, TOK_RPAREN, ")");
    consume(p, TOK_LBRACE, "{");
    Node* body = block(p);
    return ast_func(name, params, pcount, body, line);
}

static Node* if_stmt(Parser* p) {
    int line = p->previous.line;
    Node* cond = expression(p);
    consume(p, TOK_LBRACE, "{");
    Node* then_b = block(p);
    Node* else_b = NULL;
    if (match(p, TOK_ELSE)) {
        if (match(p, TOK_IF)) {
            /* else if → wrap as nested if statement inside a block */
            Node* nested = if_stmt(p);
            Node** one = (Node**)sgas_alloc(sizeof(Node*));
            one[0] = nested;
            else_b = ast_block(one, 1, line);
        } else {
            consume(p, TOK_LBRACE, "{");
            else_b = block(p);
        }
    }
    return ast_if(cond, then_b, else_b, line);
}

static Node* while_stmt(Parser* p) {
    int line = p->previous.line;
    Node* cond = expression(p);
    consume(p, TOK_LBRACE, "{");
    Node* body = block(p);
    return ast_while(cond, body, line);
}

static Node* return_stmt(Parser* p) {
    int line = p->previous.line;
    Node* v = NULL;
    if (!check(p, TOK_SEMI) && !check(p, TOK_RBRACE)) v = expression(p);
    match(p, TOK_SEMI);
    return ast_return(v, line);
}

static Node* expr_stmt(Parser* p) {
    int line = p->current.line;
    Node* e = expression(p);
    match(p, TOK_SEMI);
    return ast_expr_stmt(e, line);
}

static Node* statement(Parser* p) {
    if (match(p, TOK_IF))    return if_stmt(p);
    if (match(p, TOK_WHILE)) return while_stmt(p);
    if (match(p, TOK_RETURN))return return_stmt(p);
    if (match(p, TOK_LBRACE))return block(p);
    return expr_stmt(p);
}

static Node* declaration(Parser* p) {
    if (match(p, TOK_FUNC)) return func_decl(p);
    if (match(p, TOK_LET))  return let_decl(p);
    return statement(p);
}

/* --- expressions --- */
static Node* primary(Parser* p) {
    if (match(p, TOK_NUMBER)) {
        return ast_number(p->previous.number_value, p->previous.line);
    }
    if (match(p, TOK_STRING)) {
        const char* s = p->previous.string_value ? p->previous.string_value : "";
        Node* n = ast_string(s, p->previous.line);
        if (p->previous.string_value) free(p->previous.string_value);
        return n;
    }
    if (match(p, TOK_TRUE))  return ast_bool(1, p->previous.line);
    if (match(p, TOK_FALSE)) return ast_bool(0, p->previous.line);
    if (match(p, TOK_NIL))   return ast_nil(p->previous.line);
    if (match(p, TOK_IDENT)) {
        char buf[128];
        size_t n = p->previous.length < sizeof(buf)-1 ? p->previous.length : sizeof(buf)-1;
        memcpy(buf, p->previous.start, n); buf[n] = '\0';
        return ast_ident(sgas_strdup(buf), p->previous.line);
    }
    if (match(p, TOK_LPAREN)) {
        Node* e = expression(p);
        consume(p, TOK_RPAREN, ")");
        return e;
    }
    if (!p->had_error) {
        p->had_error = true;
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "Line %d: unexpected token %s",
                 p->current.line, token_type_name(p->current.type));
    }
    return NULL;
}

static Node* call_expr(Parser* p) {
    Node* e = primary(p);
    if (!e) return NULL;
    while (match(p, TOK_LPAREN)) {
        int line = p->previous.line;
        Node** args = NULL; int ac = 0, cap = 0;
        if (!check(p, TOK_RPAREN)) {
            do {
                if (ac == cap) { cap = cap ? cap*2 : 4;
                                 args = (Node**)sgas_realloc(args, sizeof(Node*)*cap); }
                args[ac++] = expression(p);
            } while (match(p, TOK_COMMA));
        }
        consume(p, TOK_RPAREN, ")");
        e = ast_call(e, args, ac, line);
    }
    return e;
}

static Node* unary_expr(Parser* p) {
    if (match(p, TOK_NOT) || match(p, TOK_MINUS)) {
        int op = p->previous.type;
        int line = p->previous.line;
        return ast_unary(op, unary_expr(p), line);
    }
    return call_expr(p);
}

static Node* factor(Parser* p) {
    Node* e = unary_expr(p);
    while (match(p, TOK_STAR) || match(p, TOK_SLASH) || match(p, TOK_PERCENT)) {
        int op = p->previous.type; int line = p->previous.line;
        Node* r = unary_expr(p);
        e = ast_binary(op, e, r, line);
    }
    return e;
}

static Node* term(Parser* p) {
    Node* e = factor(p);
    while (match(p, TOK_PLUS) || match(p, TOK_MINUS)) {
        int op = p->previous.type; int line = p->previous.line;
        Node* r = factor(p);
        e = ast_binary(op, e, r, line);
    }
    return e;
}

static Node* comparison(Parser* p) {
    Node* e = term(p);
    while (match(p, TOK_LT) || match(p, TOK_GT) ||
           match(p, TOK_LE) || match(p, TOK_GE)) {
        int op = p->previous.type; int line = p->previous.line;
        Node* r = term(p);
        e = ast_binary(op, e, r, line);
    }
    return e;
}

static Node* equality(Parser* p) {
    Node* e = comparison(p);
    while (match(p, TOK_EQEQ) || match(p, TOK_NEQ)) {
        int op = p->previous.type; int line = p->previous.line;
        Node* r = comparison(p);
        e = ast_binary(op, e, r, line);
    }
    return e;
}

static Node* logic_and(Parser* p) {
    Node* e = equality(p);
    while (match(p, TOK_AND)) {
        int line = p->previous.line;
        Node* r = equality(p);
        e = ast_binary(TOK_AND, e, r, line);
    }
    return e;
}

static Node* logic_or(Parser* p) {
    Node* e = logic_and(p);
    while (match(p, TOK_OR)) {
        int line = p->previous.line;
        Node* r = logic_and(p);
        e = ast_binary(TOK_OR, e, r, line);
    }
    return e;
}

static Node* assignment(Parser* p) {
    Node* e = logic_or(p);
    if (match(p, TOK_EQ)) {
        int line = p->previous.line;
        Node* value = assignment(p);
        if (e->type == NODE_IDENT) {
            return ast_assign(e->as.ident.name, value, line);
        }
        if (!p->had_error) {
            p->had_error = true;
            snprintf(p->error_msg, sizeof(p->error_msg),
                     "Line %d: invalid assignment target", line);
        }
        return NULL;
    }
    return e;
}

static Node* expression(Parser* p) { return assignment(p); }

Node* parser_parse(Parser* p) {
    Node** stmts = NULL; int count = 0, cap = 0;
    while (!check(p, TOK_EOF) && !p->had_error) {
        Node* s = declaration(p);
        if (!s) break;
        if (count == cap) { cap = cap ? cap * 2 : 16;
                            stmts = (Node**)sgas_realloc(stmts, sizeof(Node*)*cap); }
        stmts[count++] = s;
    }
    if (p->had_error) return NULL;
    return ast_program(stmts, count);
}