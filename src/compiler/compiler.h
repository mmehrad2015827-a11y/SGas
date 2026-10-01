#ifndef SGAS_COMPILER_H
#define SGAS_COMPILER_H

#include "../ast/ast.h"
#include "bytecode.h"

/* Compile an AST program into a top-level function.
 * Returns a new ObjFunction (with chunk). Caller owns. */
ObjFunction* compiler_compile(Node* program);

/* Free a function + its chunk. */
void compiler_free_function(ObjFunction* fn);

#endif