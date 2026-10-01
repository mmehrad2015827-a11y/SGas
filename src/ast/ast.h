#ifndef SGAS_AST_H
#define SGAS_AST_H

#include "../common/common.h"

typedef enum {
    NODE_PROGRAM,
    NODE_LET,
    NODE_FUNC,
    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,
    NODE_BLOCK,
    NODE_EXPR_STMT,
    NODE_BINARY,
    NODE_UNARY,
    NODE_CALL,
    NODE_IDENT,
    NODE_LITERAL,
    NODE_ASSIGN
} NodeType;

typedef struct Node Node;

struct Node {
    NodeType type;
    int line;
    union {
        struct { Node** stmts; int count; } program;
        struct { const char* name; Node* init; } let;
        struct { const char* name; const char** params; int param_count;
                 Node* body; } func;
        struct { Node* cond; Node* then_branch; Node* else_branch; } ifs;
        struct { Node* cond; Node* body; } whiles;
        struct { Node* value; } ret;
        struct { Node** stmts; int count; } block;
        struct { Node* expr; } expr_stmt;
        struct { int op; Node* left; Node* right; } binary;   /* op = TokenType */
        struct { int op; Node* operand; } unary;
        struct { Node* callee; Node** args; int arg_count; } call;
        struct { const char* name; } ident;
        struct { int type_tag; double num; char* str; int boolean; } literal;
        struct { const char* name; Node* value; } assign;
    } as;
};

/* Constructors */
Node* ast_program(Node** stmts, int count);
Node* ast_let(const char* name, Node* init, int line);
Node* ast_func(const char* name, const char** params, int param_count,
               Node* body, int line);
Node* ast_if(Node* cond, Node* then_b, Node* else_b, int line);
Node* ast_while(Node* cond, Node* body, int line);
Node* ast_return(Node* value, int line);
Node* ast_block(Node** stmts, int count, int line);
Node* ast_expr_stmt(Node* expr, int line);
Node* ast_binary(int op, Node* l, Node* r, int line);
Node* ast_unary(int op, Node* operand, int line);
Node* ast_call(Node* callee, Node** args, int arg_count, int line);
Node* ast_ident(const char* name, int line);
Node* ast_number(double v, int line);
Node* ast_string(const char* s, int line);
Node* ast_bool(int v, int line);
Node* ast_nil(int line);
Node* ast_assign(const char* name, Node* value, int line);

void ast_free(Node* node);

#endif