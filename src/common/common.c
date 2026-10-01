#include "common.h"

void* sgas_alloc(size_t size) {
    void* p = malloc(size);
    if (!p) { fprintf(stderr, "SGas: out of memory\n"); exit(70); }
    return p;
}

void* sgas_realloc(void* ptr, size_t size) {
    void* p = realloc(ptr, size);
    if (!p) { fprintf(stderr, "SGas: out of memory\n"); exit(70); }
    return p;
}

void sgas_free(void* ptr) { free(ptr); }

char* sgas_strdup(const char* s) {
    size_t n = strlen(s);
    char* out = (char*)sgas_alloc(n + 1);
    memcpy(out, s, n + 1);
    return out;
}

void sgas_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "SGas error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(70);
}