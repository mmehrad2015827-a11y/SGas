#ifndef SGAS_COMMON_H
#define SGAS_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/* Allocation wrappers (abort on OOM). */
void* sgas_alloc(size_t size);
void* sgas_realloc(void* ptr, size_t size);
void  sgas_free(void* ptr);
char* sgas_strdup(const char* s);

/* Fatal error reporting. */
void sgas_error(const char* fmt, ...);

#endif