/* src/main.c - SGas C entry point. */
#include "../include/sgas.h"
#include "../src/common/common.h"
#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include "../src/ast/ast.h"
#include "../src/compiler/compiler.h"
#include "../src/vm/vm.h"
#include "../src/runtime/runtime.h"
#include "../src/server/server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* sgas_version(void) { return SGAS_VERSION; }

/* ------------------------------------------------------------------ */
/*  اجرای یک رشتهٔ کد SGas                                             */
/* ------------------------------------------------------------------ */
static int run_source(const char* source, const char* name) {
    Parser p;
    parser_init(&p, source);
    Node* program = parser_parse(&p);
    if (!program) {
        fprintf(stderr, "%s: %s\n", name,
                p.error_msg[0] ? p.error_msg : "parse error");
        return 65;
    }

    ObjFunction* fn = compiler_compile(program);
    if (!fn) {
        ast_free(program);
        return 65;
    }

    VM vm;
    vm_init(&vm);
    int rc = vm_interpret(&vm, fn);

    vm_free(&vm);
    compiler_free_function(fn);
    ast_free(program);
    return rc;
}

/* ------------------------------------------------------------------ */
/*  API عمومی (include/sgas.h)                                         */
/* ------------------------------------------------------------------ */
int sgas_run_string(const char* source) {
    return run_source(source, "<string>");
}

int sgas_run_file(const char* path) {
    size_t size = 0;
    char* src = rt_read_file(path, &size);
    if (!src) {
        fprintf(stderr, "SGas: cannot open '%s'\n", path);
        return 66;
    }
    int rc = run_source(src, path);
    free(src);
    return rc;
}

/* ------------------------------------------------------------------ */
/*  راهنما                                                             */
/* ------------------------------------------------------------------ */
static void print_usage(const char* prog) {
    fprintf(stderr,
        "SGas %s - a small Go/Lua-inspired language\n"
        "Usage:\n"
        "  %s <file.sgas>            اجرای فایل\n"
        "  %s -e \"<source>\"          اجرای کد مستقیم\n"
        "  %s --serve [port]         اجرای رابط وب (پیش‌فرض: 8080)\n"
        "  %s -v | --version         نمایش نسخه\n",
        SGAS_VERSION, prog, prog, prog, prog);
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */
int main(int argc, char** argv) {
    if (argc < 2) { print_usage(argv[0]); return 64; }

    /* ---------- نسخه ---------- */
    if (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0) {
        printf("SGas %s\n", SGAS_VERSION);
        return 0;
    }

    /* ---------- سرور وب ---------- */
    if (strcmp(argv[1], "--serve") == 0) {
        int port = 8080;
        if (argc >= 3) {
            int p = atoi(argv[2]);
            if (p > 0 && p < 65536) port = p;
        }

        /* پوشهٔ فایل‌های استاتیک: پیش‌فرض Aew/ کنار اجرایی */
        char static_dir[512];
        if (argc >= 4) {
            snprintf(static_dir, sizeof(static_dir), "%s", argv[3]);
        } else {
            snprintf(static_dir, sizeof(static_dir), "Aew");
        }

        return sgas_server_run(port, static_dir);
    }

    /* ---------- اجرای رشته ---------- */
    if (strcmp(argv[1], "-e") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 64; }
        return sgas_run_string(argv[2]);
    }

    /* ---------- پیش‌فرض: اجرای فایل ---------- */
    return sgas_run_file(argv[1]);
}