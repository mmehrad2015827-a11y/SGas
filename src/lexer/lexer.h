#ifndef SGAS_LEXER_H
#define SGAS_LEXER_H

#include "../common/common.h"

typedef enum {
    TOK_EOF,
    TOK_ERROR,
    /* literals */
    TOK_NUMBER, TOK_STRING, TOK_IDENT,
    /* keywords */
    TOK_LET, TOK_FUNC, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_RETURN,
    TOK_TRUE, TOK_FALSE, TOK_NIL, TOK_AND, TOK_OR, TOK_NOT,
    /* punctuation */
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_COMMA, TOK_SEMI, TOK_COLON,
    /* operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_EQ, TOK_EQEQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LE, TOK_GE
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t length;
    int line;
    /* parsed payload for NUMBER / STRING */
    double number_value;
    char*  string_value;   /* owned; may be NULL */
} Token;

typedef struct {
    const char* source;
    const char* start;
    const char* current;
    int line;
} Lexer;

void  lexer_init(Lexer* lx, const char* source);
Token lexer_next(Lexer* lx);
const char* token_type_name(TokenType t);

#endif