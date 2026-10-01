#include "../src/parser/parser.h"
#include <assert.h>

static int g_failed = 0;

static void expect_program(const char* src, int expected_stmts) {
    Parser p; parser_init(&p, src);
    Node* program = parser_parse(&p);
    if (!program) {
        fprintf(stderr, "parser_test: FAIL on '%s': %s\n", src, p.error_msg);
        g_failed = 1;
        return;
    }
    if (program->type != NODE_PROGRAM ||
        program->as.program.count != expected_stmts) {
        fprintf(stderr, "parser_test: FAIL on '%s': got %d stmts, want %d\n",
                src, program->as.program.count, expected_stmts);
        g_failed = 1;
    }
    ast_free(program);
}

static void expect_error(const char* src) {
    Parser p; parser_init(&p, src);
    Node* program = parser_parse(&p);
    if (program) {
        fprintf(stderr, "parser_test: expected error on '%s'\n", src);
        g_failed = 1;
        ast_free(program);
    }
}

int main(void) {
    expect_program("let x = 1;", 1);
    expect_program("let x = 1; let y = 2;", 2);
    expect_program("func f(a) { return a; }", 1);
    expect_program("if true { print(1); } else { print(2); }", 1);
    expect_program("while false { let x = 1; }", 1);

    expect_error("let = 5;");
    expect_error("func (");   /* malformed */

    if (g_failed) { fprintf(stderr, "parser_test: FAILED\n"); return 1; }
    printf("parser_test: OK\n");
    return 0;
}