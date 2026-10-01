#include "lexer.h"

void lexer_init(Lexer* lx, const char* source) {
    lx->source  = source;
    lx->start   = source;
    lx->current = source;
    lx->line    = 1;
}

static bool is_digit(char c)  { return c >= '0' && c <= '9'; }
static bool is_alpha(char c)  { return (c >= 'a' && c <= 'z') ||
                                       (c >= 'A' && c <= 'Z') || c == '_'; }
static bool is_alnum(char c)  { return is_alpha(c) || is_digit(c); }

static char advance(Lexer* lx) { return *lx->current++; }
static char peek(Lexer* lx)    { return *lx->current; }
static char peek2(Lexer* lx)   { return lx->current[0] ? lx->current[1] : '\0'; }
static bool match(Lexer* lx, char expect) {
    if (peek(lx) != expect) return false;
    lx->current++;
    return true;
}

static Token make(Lexer* lx, TokenType t) {
    Token tok;
    tok.type    = t;
    tok.start   = lx->start;
    tok.length  = (size_t)(lx->current - lx->start);
    tok.line    = lx->line;
    tok.number_value = 0;
    tok.string_value = NULL;
    return tok;
}

static Token error_token(Lexer* lx, const char* msg) {
    Token t = make(lx, TOK_ERROR);
    t.string_value = sgas_strdup(msg);
    return t;
}

static void skip_ws_and_comments(Lexer* lx) {
    for (;;) {
        char c = peek(lx);
        switch (c) {
            case ' ': case '\r': case '\t': advance(lx); break;
            case '\n': lx->line++; advance(lx); break;
            case '-':
                if (peek2(lx) == '-') {
                    while (peek(lx) != '\n' && peek(lx) != '\0') advance(lx);
                } else return;
                break;
            default: return;
        }
    }
}

static TokenType check_keyword(Lexer* lx, size_t start, size_t len,
                               const char* rest, TokenType type) {
    if ((size_t)(lx->current - lx->start) == start + len &&
        memcmp(lx->start + start, rest, len) == 0)
        return type;
    return TOK_IDENT;
}

static TokenType ident_type(Lexer* lx) {
    switch (lx->start[0]) {
        case 'a': return check_keyword(lx, 1, 2, "nd", TOK_AND);
        case 'e': return check_keyword(lx, 1, 3, "lse", TOK_ELSE);
        case 'f':
            if (lx->current - lx->start > 1) {
                if (lx->start[1] == 'a') return check_keyword(lx, 2, 3, "lse", TOK_FALSE);
                if (lx->start[1] == 'u') return check_keyword(lx, 2, 2, "nc", TOK_FUNC);
            }
            break;
        case 'i': return check_keyword(lx, 1, 1, "f", TOK_IF);
        case 'l': return check_keyword(lx, 1, 2, "et", TOK_LET);
        case 'n':
            if (lx->current - lx->start > 1) {
                if (lx->start[1] == 'i') return check_keyword(lx, 2, 1, "l", TOK_NIL);
                if (lx->start[1] == 'o') return check_keyword(lx, 2, 1, "t", TOK_NOT);
            }
            break;
        case 'o': return check_keyword(lx, 1, 1, "r", TOK_OR);
        case 'r': return check_keyword(lx, 1, 5, "eturn", TOK_RETURN);
        case 't': return check_keyword(lx, 1, 3, "rue", TOK_TRUE);
        case 'w': return check_keyword(lx, 1, 4, "hile", TOK_WHILE);
    }
    return TOK_IDENT;
}

static Token number(Lexer* lx) {
    while (is_digit(peek(lx))) advance(lx);
    if (peek(lx) == '.' && is_digit(peek2(lx))) {
        advance(lx);
        while (is_digit(peek(lx))) advance(lx);
    }
    Token t = make(lx, TOK_NUMBER);
    char buf[64];
    size_t n = t.length < sizeof(buf) - 1 ? t.length : sizeof(buf) - 1;
    memcpy(buf, t.start, n);
    buf[n] = '\0';
    t.number_value = strtod(buf, NULL);
    return t;
}

static Token string(Lexer* lx) {
    while (peek(lx) != '"' && peek(lx) != '\0') {
        if (peek(lx) == '\n') lx->line++;
        advance(lx);
    }
    if (peek(lx) == '\0') return error_token(lx, "unterminated string");
    advance(lx); /* closing quote */

    size_t len = (size_t)(lx->current - lx->start - 2);
    char* buf = (char*)sgas_alloc(len + 1);
    memcpy(buf, lx->start + 1, len);
    buf[len] = '\0';

    Token t = make(lx, TOK_STRING);
    t.string_value = buf;
    return t;
}

Token lexer_next(Lexer* lx) {
    skip_ws_and_comments(lx);
    lx->start = lx->current;

    if (peek(lx) == '\0') return make(lx, TOK_EOF);

    char c = advance(lx);

    if (is_digit(c)) return number(lx);
    if (is_alpha(c)) {
        while (is_alnum(peek(lx))) advance(lx);
        TokenType t = ident_type(lx);
        return make(lx, t);
    }

    switch (c) {
        case '(': return make(lx, TOK_LPAREN);
        case ')': return make(lx, TOK_RPAREN);
        case '{': return make(lx, TOK_LBRACE);
        case '}': return make(lx, TOK_RBRACE);
        case ',': return make(lx, TOK_COMMA);
        case ';': return make(lx, TOK_SEMI);
        case ':': return make(lx, TOK_COLON);
        case '+': return make(lx, TOK_PLUS);
        case '-': return make(lx, TOK_MINUS);
        case '*': return make(lx, TOK_STAR);
        case '/': return make(lx, TOK_SLASH);
        case '%': return make(lx, TOK_PERCENT);
        case '"': return string(lx);
        case '=': return make(lx, match(lx, '=') ? TOK_EQEQ : TOK_EQ);
        case '!': return make(lx, match(lx, '=') ? TOK_NEQ  : TOK_ERROR);
        case '<': return make(lx, match(lx, '=') ? TOK_LE   : TOK_LT);
        case '>': return make(lx, match(lx, '=') ? TOK_GE   : TOK_GT);
    }
    return error_token(lx, "unexpected character");
}

const char* token_type_name(TokenType t) {
    switch (t) {
        case TOK_EOF: return "EOF"; case TOK_ERROR: return "ERROR";
        case TOK_NUMBER: return "NUMBER"; case TOK_STRING: return "STRING";
        case TOK_IDENT: return "IDENT";
        case TOK_LET: return "let"; case TOK_FUNC: return "func";
        case TOK_IF: return "if"; case TOK_ELSE: return "else";
        case TOK_WHILE: return "while"; case TOK_RETURN: return "return";
        case TOK_TRUE: return "true"; case TOK_FALSE: return "false";
        case TOK_NIL: return "nil"; case TOK_AND: return "and";
        case TOK_OR: return "or"; case TOK_NOT: return "not";
        case TOK_LPAREN: return "("; case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{"; case TOK_RBRACE: return "}";
        case TOK_COMMA: return ","; case TOK_SEMI: return ";";
        case TOK_COLON: return ":";
        case TOK_PLUS: return "+"; case TOK_MINUS: return "-";
        case TOK_STAR: return "*"; case TOK_SLASH: return "/";
        case TOK_PERCENT: return "%";
        case TOK_EQ: return "="; case TOK_EQEQ: return "==";
        case TOK_NEQ: return "!="; case TOK_LT: return "<";
        case TOK_GT: return ">"; case TOK_LE: return "<=";
        case TOK_GE: return ">=";
    }
    return "?";
}