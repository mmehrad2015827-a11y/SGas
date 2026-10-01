#include "value.h"
#include "vm.h"
#include <math.h>

static Obj* obj_list = NULL;   /* head of allocated objects (GC hook) */

static void obj_register(Obj* o) {
    o->next = obj_list;
    obj_list = o;
}

ObjString* obj_string_new(const char* chars, size_t length) {
    char* buf = (char*)sgas_alloc(length + 1);
    memcpy(buf, chars, length);
    buf[length] = '\0';
    return obj_string_take(buf, length);
}

ObjString* obj_string_take(char* chars, size_t length) {
    ObjString* s = (ObjString*)sgas_alloc(sizeof(ObjString));
    s->obj.type = OBJ_STRING;
    s->chars    = chars;
    s->length   = length;
    /* FNV-1a */
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        h ^= (uint8_t)chars[i];
        h *= 16777619u;
    }
    s->hash = h;
    obj_register((Obj*)s);
    return s;
}

ObjNative* obj_native_new(const char* name, int arity,
                          Value (*fn)(VM*, int, Value*)) {
    ObjNative* n = (ObjNative*)sgas_alloc(sizeof(ObjNative));
    n->obj.type = OBJ_NATIVE;
    n->name = name;
    n->arity = arity;
    n->fn = fn;
    obj_register((Obj*)n);
    return n;
}

void value_print(Value v) {
    switch (v.type) {
        case VAL_NIL:    printf("nil"); break;
        case VAL_BOOL:   printf(AS_BOOL(v) ? "true" : "false"); break;
        case VAL_NUMBER: {
            double d = AS_NUMBER(v);
            if (d == (long long)d) printf("%lld", (long long)d);
            else printf("%g", d);
            break;
        }
        case VAL_OBJ:
            switch (AS_OBJ(v)->type) {
                case OBJ_STRING: {
                    ObjString* s = AS_STRING(v);
                    fwrite(s->chars, 1, s->length, stdout);
                    break;
                }
                case OBJ_FUNCTION:
                    printf("<func %s>", AS_FUNC(v)->name ? AS_FUNC(v)->name : "?");
                    break;
                case OBJ_NATIVE:
                    printf("<native %s>", AS_NATIVE(v)->name);
                    break;
            }
            break;
    }
}

bool value_truthy(Value v) {
    if (IS_NIL(v))    return false;
    if (IS_BOOL(v))   return AS_BOOL(v);
    return true;
}

bool value_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_NIL:    return true;
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: {
            if (AS_OBJ(a)->type != AS_OBJ(b)->type) return false;
            if (AS_OBJ(a)->type == OBJ_STRING) {
                ObjString* x = AS_STRING(a);
                ObjString* y = AS_STRING(b);
                return x->length == y->length &&
                       memcmp(x->chars, y->chars, x->length) == 0;
            }
            return AS_OBJ(a) == AS_OBJ(b);
        }
    }
    return false;
}