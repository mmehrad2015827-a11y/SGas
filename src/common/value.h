#ifndef SGAS_VALUE_H
#define SGAS_VALUE_H

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjFunction ObjFunction;
typedef struct ObjNative ObjNative;
typedef struct Chunk Chunk;
typedef struct VM VM;

typedef enum { VAL_NIL, VAL_BOOL, VAL_NUMBER, VAL_OBJ } ValueType;

typedef struct Value {
    ValueType type;
    union {
        bool   boolean;
        double number;
        Obj*   obj;
    } as;
} Value;

#define NIL_VAL       ((Value){ VAL_NIL,    { .number = 0 } })
#define BOOL_VAL(b)   ((Value){ VAL_BOOL,   { .boolean = (b) } })
#define NUM_VAL(n)    ((Value){ VAL_NUMBER, { .number = (n) } })
#define OBJ_VAL(o)    ((Value){ VAL_OBJ,    { .obj = (Obj*)(o) } })

#define IS_NIL(v)     ((v).type == VAL_NIL)
#define IS_BOOL(v)    ((v).type == VAL_BOOL)
#define IS_NUMBER(v)  ((v).type == VAL_NUMBER)
#define IS_OBJ(v)     ((v).type == VAL_OBJ)

#define AS_BOOL(v)    ((v).as.boolean)
#define AS_NUMBER(v)  ((v).as.number)
#define AS_OBJ(v)     ((v).as.obj)

typedef enum { OBJ_STRING, OBJ_FUNCTION, OBJ_NATIVE } ObjType;

struct Obj {
    ObjType type;
    Obj*    next;   /* GC list hook */
};

struct ObjString {
    Obj      obj;
    size_t   length;
    uint32_t hash;
    char*    chars;
};

struct ObjNative {
    Obj         obj;
    const char* name;
    int         arity;
    Value     (*fn)(VM*, int, Value*);
};

/* ObjFunction lives here (not in bytecode.h) to keep includes flat.
 * Chunk is forward-declared. */
struct ObjFunction {
    Obj         obj;
    const char* name;
    int         arity;
    int         upvalue_count;
    Chunk*      chunk;
};

/* Constructors / utilities. */
ObjString* obj_string_new(const char* chars, size_t length);
ObjString* obj_string_take(char* chars, size_t length);
ObjNative* obj_native_new(const char* name, int arity,
                          Value (*fn)(VM*, int, Value*));

void value_print(Value v);
bool value_truthy(Value v);
bool value_equal(Value a, Value b);

/* Convenience accessors. */
#define IS_STRING(v)  (IS_OBJ(v) && AS_OBJ(v)->type == OBJ_STRING)
#define IS_FUNC(v)    (IS_OBJ(v) && AS_OBJ(v)->type == OBJ_FUNCTION)
#define IS_NATIVE(v)  (IS_OBJ(v) && AS_OBJ(v)->type == OBJ_NATIVE)
#define AS_STRING(v)  ((ObjString*)AS_OBJ(v))
#define AS_FUNC(v)    ((ObjFunction*)AS_OBJ(v))
#define AS_NATIVE(v)  ((ObjNative*)AS_OBJ(v))

#endif