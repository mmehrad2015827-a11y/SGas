#ifndef SGAS_RUNTIME_H
#define SGAS_RUNTIME_H

#include "../common/value.h"

/* Runtime helpers shared between VM and stdlib. */

/* Format a value as a string (allocates ObjString). */
ObjString* rt_to_string(Value v);

/* Read a whole file into a heap buffer (NULL on failure). */
char* rt_read_file(const char* path, size_t* out_size);

#endif