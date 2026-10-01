#ifndef SGAS_STDLIB_H
#define SGAS_STDLIB_H

#include "../vm/vm.h"

/* Register all built-in functions into the VM's globals. */
void stdlib_register(VM* vm);

#endif