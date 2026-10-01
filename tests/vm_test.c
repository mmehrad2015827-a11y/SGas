#include "../src/parser/parser.h"
#include "../src/compiler/compiler.h"
#include "../src/vm/vm.h"
#include "../src/ast/ast.h"

static int run(const char* src) {
    Parser p; parser_init(&p, src);
    Node* program = parser_parse(&p);
    if (!program) {
        fprintf(stderr, "vm_test parse fail: %s\n", p.error_msg);
        return -1;
    }
    ObjFunction* fn = compiler_compile(program);
    if (!fn) { ast_free(program); return -1; }

    VM vm; vm_init(&vm);
    int rc = vm_interpret(&vm, fn);
    vm_free(&vm);
    compiler_free_function(fn);
    ast_free(program);
    return rc;
}

int main(void) {
    if (run("let x = 10; print(x);") != 0) return 1;
    if (run("print(1 + 2 * 3);") != 0) return 1;
    if (run("func f(a, b) { return a + b; } print(f(2, 3));") != 0) return 1;
    if (run("let i = 0; while i < 3 { print(i); i = i + 1; }") != 0) return 1;
    if (run("if 1 < 2 { print(\"yes\"); } else { print(\"no\"); }") != 0) return 1;
    if (run("func fib(n){ if n < 2 { return n; } return fib(n-1)+fib(n-2); } print(fib(8));") != 0) return 1;

    printf("vm_test: OK\n");
    return 0;
}