#include "stdlib.h"
#include "../runtime/runtime.h"

/* ---- built-in functions ---- */

static Value native_print(VM* vm, int argc, Value* args) {
    (void)vm;
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        value_print(args[i]);
    }
    printf("\n");
    return NIL_VAL;
}

static Value native_str(VM* vm, int argc, Value* args) {
    (void)vm;
    if (argc < 1) return OBJ_VAL(obj_string_new("", 0));
    return OBJ_VAL(rt_to_string(args[0]));
}

static Value native_num(VM* vm, int argc, Value* args) {
    (void)vm;
    if (argc < 1) return NUM_VAL(0);
    Value v = args[0];
    if (IS_NUMBER(v)) return v;
    if (IS_BOOL(v))   return NUM_VAL(AS_BOOL(v) ? 1 : 0);
    if (IS_STRING(v)) return NUM_VAL(atof(AS_STRING(v)->chars));
    return NUM_VAL(0);
}

static Value native_len(VM* vm, int argc, Value* args) {
    (void)vm;
    if (argc < 1) return NUM_VAL(0);
    Value v = args[0];
    if (IS_STRING(v)) return NUM_VAL((double)AS_STRING(v)->length);
    return NUM_VAL(0);
}

static Value native_type(VM* vm, int argc, Value* args) {
    (void)vm;
    if (argc < 1) return OBJ_VAL(obj_string_new("nil", 3));
    const char* t = "unknown";
    switch (args[0].type) {
        case VAL_NIL:    t = "nil";    break;
        case VAL_BOOL:   t = "bool";   break;
        case VAL_NUMBER: t = "number"; break;
        case VAL_OBJ:
            switch (AS_OBJ(args[0])->type) {
                case OBJ_STRING:   t = "string"; break;
                case OBJ_FUNCTION: t = "function"; break;
                case OBJ_NATIVE:   t = "native"; break;
            }
            break;
    }
    return OBJ_VAL(obj_string_new(t, strlen(t)));
}

static Value native_clock(VM* vm, int argc, Value* args) {
    (void)vm; (void)argc; (void)args;
    extern long time(long*);
    return NUM_VAL((double)(unsigned long)(time(NULL)));
}

static Value native_assert(VM* vm, int argc, Value* args) {
    (void)vm;
    if (argc < 1 || !value_truthy(args[0])) {
        fprintf(stderr, "Assertion failed\n");
        return NIL_VAL;
    }
    return NIL_VAL;
}

void stdlib_register(VM* vm) {
    vm_define_native(vm, "print",  -1, native_print);
    vm_define_native(vm, "str",     1, native_str);
    vm_define_native(vm, "num",     1, native_num);
    vm_define_native(vm, "len",     1, native_len);
    vm_define_native(vm, "type",    1, native_type);
    vm_define_native(vm, "clock",   0, native_clock);
    vm_define_native(vm, "assert",  1, native_assert);
}