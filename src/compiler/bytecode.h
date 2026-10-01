#ifndef SGAS_BYTECODE_H
#define SGAS_BYTECODE_H

#include "../common/value.h"

typedef enum {
    OP_CONSTANT,       /* u8 idx */
    OP_NIL, OP_TRUE, OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,      /* u8 slot */
    OP_SET_LOCAL,      /* u8 slot */
    OP_GET_GLOBAL,     /* u8 idx of name */
    OP_SET_GLOBAL,     /* u8 idx of name */
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_NEG, OP_NOT,
    OP_EQ, OP_NEQ,
    OP_LT, OP_GT, OP_LE, OP_GE,
    OP_PRINT,
    OP_JUMP,           /* u16 offset */
    OP_JUMP_IF_FALSE,  /* u16 offset */
    OP_LOOP,           /* u16 offset */
    OP_CALL,           /* u8 argc */
    OP_RETURN,
    OP_HALT
} OpCode;

/* نکته: در value.h از قبل typedef struct Chunk Chunk; داریم.
 * پس اینجا فقط struct را با همان tag تعریف می‌کنیم. */
struct Chunk {
    int      count;
    int      capacity;
    uint8_t* code;
    int*     lines;
    Value*   constants;
    int      constants_count;
    int      constants_cap;
};

void chunk_init(Chunk* c);
void chunk_free(Chunk* c);
int  chunk_add_constant(Chunk* c, Value v);
void chunk_write(Chunk* c, uint8_t byte, int line);
int  chunk_emit_constant(Chunk* c, Value v, int line);

#endif