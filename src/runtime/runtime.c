#include "runtime.h"

ObjString* rt_to_string(Value v) {
    char buf[64];
    switch (v.type) {
        case VAL_NIL:  return obj_string_new("nil", 3);
        case VAL_BOOL: return obj_string_new(AS_BOOL(v) ? "true" : "false",
                                             AS_BOOL(v) ? 4 : 5);
        case VAL_NUMBER: {
            double d = AS_NUMBER(v);
            int n;
            if (d == (long long)d) n = snprintf(buf, sizeof(buf), "%lld", (long long)d);
            else n = snprintf(buf, sizeof(buf), "%g", d);
            return obj_string_new(buf, (size_t)n);
        }
        case VAL_OBJ:
            if (AS_OBJ(v)->type == OBJ_STRING) {
                ObjString* s = AS_STRING(v);
                return obj_string_new(s->chars, s->length);
            }
            return obj_string_new("<obj>", 5);
    }
    return obj_string_new("?", 1);
}

char* rt_read_file(const char* path, size_t* out_size) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (len < 0) { fclose(fp); return NULL; }
    char* buf = (char*)sgas_alloc((size_t)len + 1);
    size_t read = fread(buf, 1, (size_t)len, fp);
    buf[read] = '\0';
    fclose(fp);
    if (out_size) *out_size = read;
    return buf;
}