#ifndef SGAS_PARSER_H
#define SGAS_PARSER_H

#include "../lexer/lexer.h"
#include "../ast/ast.h"

typedef struct {
    Lexer  lx;
    Token  current;
    Token  previous;
    bool   had_error;
    char   error_msg[256];
} Parser;

void  parser_init(Parser* p, const char* source);
Node* parser_parse(Parser* p);   /* returns NODE_PROGRAM or NULL on error */

#endif