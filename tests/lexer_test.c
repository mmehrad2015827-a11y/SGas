#include "../src/lexer/lexer.h"
#include <assert.h>

static void check(Lexer* lx, TokenType expected, const char* label) {
    Token t = lexer_next(lx);
    if (t.type != expected) {
        fprintf(stderr, "lexer_test: %s -> got %s, want %s\n",
                label, token_type_name(t.type), token_type_name(expected));
        exit(1);
    }
}

int main(void) {
    Lexer lx;

    lexer_init(&lx, "let x = 42;");
    check(&lx, TOK_LET,    "let");
    check(&lx, TOK_IDENT,  "x");
    check(&lx, TOK_EQ,     "=");
    check(&lx, TOK_NUMBER, "42");
    check(&lx, TOK_SEMI,   ";");
    check(&lx, TOK_EOF,    "eof");

    lexer_init(&lx, "func add(a, b) { return a + b; }");
    check(&lx, TOK_FUNC,   "func");
    check(&lx, TOK_IDENT,  "add");
    check(&lx, TOK_LPAREN, "(");
    check(&lx, TOK_IDENT,  "a");
    check(&lx, TOK_COMMA,  ",");
    check(&lx, TOK_IDENT,  "b");
    check(&lx, TOK_RPAREN, ")");
    check(&lx, TOK_LBRACE, "{");
    check(&lx, TOK_RETURN, "return");
    check(&lx, TOK_IDENT,  "a");
    check(&lx, TOK_PLUS,   "+");
    check(&lx, TOK_IDENT,  "b");
    check(&lx, TOK_SEMI,   ";");
    check(&lx, TOK_RBRACE, "}");
    check(&lx, TOK_EOF,    "eof");

    lexer_init(&lx, "-- comment\n\"hello\" == true");
    check(&lx, TOK_STRING, "\"hello\"");
    check(&lx, TOK_EQEQ,   "==");
    check(&lx, TOK_TRUE,   "true");

    printf("lexer_test: OK\n");
    return 0;
}