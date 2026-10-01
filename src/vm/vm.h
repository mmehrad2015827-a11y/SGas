#ifndef SGAS_VM_H
#define SGAS_VM_H

#include "../common/value.h"
#include "../compiler/bytecode.h"

#define SGAS_STACK_MAX    256
#define SGAS_FRAMES_MAX   64
#define SGAS_GLOBAL_CAP   512   /* باید توان ۲ باشد */

typedef struct {
    ObjFunction* fn;
    uint8_t*     ip;
    Value*       slots;
} CallFrame;

typedef struct {
    ObjString* keys[SGAS_GLOBAL_CAP];
    Value      values[SGAS_GLOBAL_CAP];
    int        count;
} Globals;

typedef struct VM {
    Value      stack[SGAS_STACK_MAX];
    Value*     sp;
    CallFrame  frames[SGAS_FRAMES_MAX];
    int        frame_count;
    Globals    globals;
    bool       had_error;
} VM;

void vm_init(VM* vm);
void vm_free(VM* vm);

int  vm_interpret(VM* vm, ObjFunction* fn);

bool  vm_global_get(VM* vm, ObjString* name, Value* out);
void  vm_global_set(VM* vm, ObjString* name, Value v);
void  vm_define_native(VM* vm, const char* name, int arity,
                       Value (*fn)(VM*, int, Value*));

#endif