#include "vm.h"
#include "../runtime/runtime.h"
#include "../stdlib/stdlib.h"
#include <math.h>

static void reset_stack(VM* vm) { vm->sp = vm->stack; }

void vm_init(VM* vm) {
    reset_stack(vm);
    vm->frame_count = 0;
    vm->had_error = false;
    memset(&vm->globals, 0, sizeof(vm->globals));
    stdlib_register(vm);
}

void vm_free(VM* vm) {
    /* جدول هش آرایه‌ی ثابت است؛ چیزی برای free کردن نیست */
    (void)vm;
}

/* ---------- hash-table globals ---------- */

static inline uint32_t g_slot(ObjString* name) {
    return name->hash & (SGAS_GLOBAL_CAP - 1);
}

bool vm_global_get(VM* vm, ObjString* name, Value* out) {
    Globals* g = &vm->globals;
    uint32_t i = g_slot(name);
    for (int probes = 0; probes < SGAS_GLOBAL_CAP; probes++) {
        ObjString* k = g->keys[i];
        if (!k) return false;                   /* خانه خالی → پیدا نشد */
        if (k->hash == name->hash &&
            k->length == name->length &&
            memcmp(k->chars, name->chars, name->length) == 0) {
            *out = g->values[i];
            return true;
        }
        i = (i + 1) & (SGAS_GLOBAL_CAP - 1);
    }
    return false;
}

void vm_global_set(VM* vm, ObjString* name, Value v) {
    Globals* g = &vm->globals;
    uint32_t i = g_slot(name);
    for (int probes = 0; probes < SGAS_GLOBAL_CAP; probes++) {
        ObjString* k = g->keys[i];
        if (!k) {
            g->keys[i]   = name;
            g->values[i] = v;
            g->count++;
            return;
        }
        if (k->hash == name->hash &&
            k->length == name->length &&
            memcmp(k->chars, name->chars, name->length) == 0) {
            g->values[i] = v;
            return;
        }
        i = (i + 1) & (SGAS_GLOBAL_CAP - 1);
    }
    fprintf(stderr, "SGas: global table full\n");
    exit(70);
}

void vm_define_native(VM* vm, const char* name, int arity,
                      Value (*fn)(VM*, int, Value*)) {
    ObjString* s = obj_string_new(name, strlen(name));
    ObjNative* n = obj_native_new(name, arity, fn);
    vm_global_set(vm, s, OBJ_VAL(n));
}

/* ---------- helpers ---------- */

static void runtime_error(VM* vm, const char* msg) {
    fprintf(stderr, "Runtime error: %s\n", msg);
    for (int i = vm->frame_count - 1; i >= 0; i--) {
        ObjFunction* fn = vm->frames[i].fn;
        size_t off = (size_t)(vm->frames[i].ip - fn->chunk->code - 1);
        int line = fn->chunk->lines[off];
        fprintf(stderr, "  at %s() line %d\n", fn->name ? fn->name : "?", line);
    }
    vm->had_error = true;
}

static inline void push(VM* vm, Value v) { *vm->sp++ = v; }
static inline Value pop(VM* vm)          { return *--vm->sp; }
static inline Value peek(VM* vm, int d)  { return vm->sp[-1 - d]; }

static bool call_value(VM* vm, Value callee, int argc) {
    if (IS_NATIVE(callee)) {
        ObjNative* n = AS_NATIVE(callee);
        if (n->arity >= 0 && n->arity != argc) {
            runtime_error(vm, "wrong number of arguments to native");
            return false;
        }
        Value* args = vm->sp - argc;
        Value r = n->fn(vm, argc, args);
        vm->sp -= argc + 1;
        push(vm, r);
        return true;
    }
    if (IS_FUNC(callee)) {
        ObjFunction* fn = AS_FUNC(callee);
        if (fn->arity != argc) {
            runtime_error(vm, "wrong number of arguments to function");
            return false;
        }
        if (vm->frame_count >= SGAS_FRAMES_MAX) {
            runtime_error(vm, "stack overflow (frames)");
            return false;
        }
        CallFrame* f = &vm->frames[vm->frame_count++];
        f->fn = fn;
        f->ip = fn->chunk->code;
        f->slots = vm->sp - argc;
        return true;
    }
    runtime_error(vm, "can only call functions");
    return false;
}

static inline bool is_falsey(Value v) { return !value_truthy(v); }

static bool do_add(VM* vm, Value a, Value b, Value* out) {
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        *out = NUM_VAL(AS_NUMBER(a) + AS_NUMBER(b));
        return true;
    }
    if (IS_STRING(a) && IS_STRING(b)) {
        ObjString* x = AS_STRING(a);
        ObjString* y = AS_STRING(b);
        size_t len = x->length + y->length;
        char* buf = (char*)sgas_alloc(len + 1);
        memcpy(buf, x->chars, x->length);
        memcpy(buf + x->length, y->chars, y->length);
        buf[len] = '\0';
        *out = OBJ_VAL(obj_string_take(buf, len));
        return true;
    }
    if (IS_STRING(a) && (IS_NUMBER(b) || IS_BOOL(b) || IS_NIL(b))) {
        char tmp[64];
        if (IS_NUMBER(b)) {
            double d = AS_NUMBER(b);
            if (d == (long long)d) snprintf(tmp, sizeof(tmp), "%lld", (long long)d);
            else snprintf(tmp, sizeof(tmp), "%g", d);
        } else if (IS_BOOL(b)) snprintf(tmp, sizeof(tmp), "%s", AS_BOOL(b) ? "true" : "false");
        else snprintf(tmp, sizeof(tmp), "nil");
        ObjString* x = AS_STRING(a);
        size_t l2 = strlen(tmp);
        size_t len = x->length + l2;
        char* buf = (char*)sgas_alloc(len + 1);
        memcpy(buf, x->chars, x->length);
        memcpy(buf + x->length, tmp, l2);
        buf[len] = '\0';
        *out = OBJ_VAL(obj_string_take(buf, len));
        return true;
    }
    runtime_error(vm, "operands must be two numbers or two strings");
    return false;
}

/* ---------- main loop ---------- */

int vm_interpret(VM* vm, ObjFunction* top) {
    CallFrame* f = &vm->frames[vm->frame_count++];
    f->fn = top;
    f->ip = top->chunk->code;
    f->slots = vm->stack;

#define READ_BYTE()   (*frame->ip++)
#define READ_U16()    (frame->ip += 2, (uint16_t)((frame->ip[-2]) | (frame->ip[-1] << 8)))
#define READ_CONST(i) (frame->fn->chunk->constants[i])

    CallFrame* frame = &vm->frames[vm->frame_count - 1];

    for (;;) {
        uint8_t op = READ_BYTE();
        switch (op) {
            case OP_CONSTANT:
                push(vm, READ_CONST(READ_BYTE()));
                break;

            case OP_NIL:   push(vm, NIL_VAL);    break;
            case OP_TRUE:  push(vm, BOOL_VAL(1)); break;
            case OP_FALSE: push(vm, BOOL_VAL(0)); break;

            case OP_POP: (void)pop(vm); break;

            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm, frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(vm, 0);
                break;
            }

            case OP_GET_GLOBAL: {
                ObjString* name = AS_STRING(READ_CONST(READ_BYTE()));
                Value v;
                if (!vm_global_get(vm, name, &v)) {
                    runtime_error(vm, "undefined variable");
                    return 70;
                }
                push(vm, v);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = AS_STRING(READ_CONST(READ_BYTE()));
                vm_global_set(vm, name, peek(vm, 0));
                break;
            }

            case OP_ADD: {
                Value b = pop(vm), a = pop(vm), out;
                if (!do_add(vm, a, b, &out)) return 70;
                push(vm, out);
                break;
            }
            case OP_SUB: {
                Value b = pop(vm), a = pop(vm);
                if (!IS_NUMBER(a) || !IS_NUMBER(b)) { runtime_error(vm, "'-' expects numbers"); return 70; }
                push(vm, NUM_VAL(AS_NUMBER(a) - AS_NUMBER(b)));
                break;
            }
            case OP_MUL: {
                Value b = pop(vm), a = pop(vm);
                if (!IS_NUMBER(a) || !IS_NUMBER(b)) { runtime_error(vm, "'*' expects numbers"); return 70; }
                push(vm, NUM_VAL(AS_NUMBER(a) * AS_NUMBER(b)));
                break;
            }
            case OP_DIV: {
                Value b = pop(vm), a = pop(vm);
                if (!IS_NUMBER(a) || !IS_NUMBER(b)) { runtime_error(vm, "'/' expects numbers"); return 70; }
                if (AS_NUMBER(b) == 0) { runtime_error(vm, "division by zero"); return 70; }
                push(vm, NUM_VAL(AS_NUMBER(a) / AS_NUMBER(b)));
                break;
            }
            case OP_MOD: {
                Value b = pop(vm), a = pop(vm);
                if (!IS_NUMBER(a) || !IS_NUMBER(b)) { runtime_error(vm, "'%' expects numbers"); return 70; }
                push(vm, NUM_VAL(fmod(AS_NUMBER(a), AS_NUMBER(b))));
                break;
            }
            case OP_NEG: {
                Value a = pop(vm);
                if (!IS_NUMBER(a)) { runtime_error(vm, "unary '-' expects number"); return 70; }
                push(vm, NUM_VAL(-AS_NUMBER(a)));
                break;
            }
            case OP_NOT:
                push(vm, BOOL_VAL(is_falsey(pop(vm))));
                break;

            case OP_EQ:  { Value b=pop(vm), a=pop(vm); push(vm, BOOL_VAL(value_equal(a,b)));  break; }
            case OP_NEQ: { Value b=pop(vm), a=pop(vm); push(vm, BOOL_VAL(!value_equal(a,b))); break; }
            case OP_LT:  {
                Value b=pop(vm), a=pop(vm);
                if (!IS_NUMBER(a)||!IS_NUMBER(b)) { runtime_error(vm,"'<' expects numbers"); return 70; }
                push(vm, BOOL_VAL(AS_NUMBER(a) < AS_NUMBER(b)));
                break;
            }
            case OP_GT:  {
                Value b=pop(vm), a=pop(vm);
                if (!IS_NUMBER(a)||!IS_NUMBER(b)) { runtime_error(vm,"'>' expects numbers"); return 70; }
                push(vm, BOOL_VAL(AS_NUMBER(a) > AS_NUMBER(b)));
                break;
            }
            case OP_LE:  {
                Value b=pop(vm), a=pop(vm);
                if (!IS_NUMBER(a)||!IS_NUMBER(b)) { runtime_error(vm,"'<=' expects numbers"); return 70; }
                push(vm, BOOL_VAL(AS_NUMBER(a) <= AS_NUMBER(b)));
                break;
            }
            case OP_GE:  {
                Value b=pop(vm), a=pop(vm);
                if (!IS_NUMBER(a)||!IS_NUMBER(b)) { runtime_error(vm,"'>=' expects numbers"); return 70; }
                push(vm, BOOL_VAL(AS_NUMBER(a) >= AS_NUMBER(b)));
                break;
            }

            case OP_PRINT: {
                Value v = pop(vm);
                value_print(v);
                printf("\n");
                break;
            }

            case OP_JUMP: {
                uint16_t off = READ_U16();
                frame->ip += off;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t off = READ_U16();
                if (is_falsey(peek(vm, 0))) frame->ip += off;
                break;
            }
            case OP_LOOP: {
                uint16_t off = READ_U16();
                frame->ip -= off;
                break;
            }

            case OP_CALL: {
                int argc = READ_BYTE();
                Value callee = peek(vm, argc);
                if (!call_value(vm, callee, argc)) return 70;
                if (vm->frame_count > 0 && IS_FUNC(callee)) {
                    for (int i = argc; i > 0; i--)
                        vm->sp[-i - 1] = vm->sp[-i];
                    vm->sp--;
                    frame = &vm->frames[vm->frame_count - 1];
                    frame->slots = vm->sp - argc;
                }
                break;
            }

            case OP_RETURN: {
                Value result = pop(vm);
                vm->frame_count--;
                if (vm->frame_count == 0) {
                    vm->sp = vm->stack;
                    return vm->had_error ? 70 : 0;
                }
                vm->sp = frame->slots;
                push(vm, result);
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }

            case OP_HALT:
                return vm->had_error ? 70 : 0;

            default:
                runtime_error(vm, "unknown opcode");
                return 70;
        }
    }

#undef READ_BYTE
#undef READ_U16
#undef READ_CONST
}