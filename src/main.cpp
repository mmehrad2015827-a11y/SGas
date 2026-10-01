// src/main.cpp - SGas C++ entry point.
// Thin wrapper around the C implementation. Provided so the project
// can be linked either as C or C++ without changing anything else.

#include "../include/sgas.h"

#include <cstdio>
#include <cstring>

static void print_usage(const char* prog) {
    std::fprintf(stderr,
        "SGas %s - a small Go/Lua-inspired language (C++ entry)\n"
        "Usage:\n"
        "  %s <file.sgas>\n"
        "  %s -e \"<source>\"\n"
        "  %s -v\n",
        sgas_version(), prog, prog, prog);
}

int main(int argc, char** argv) {
    if (argc < 2) { print_usage(argv[0]); return 64; }

    if (std::strcmp(argv[1], "-v") == 0 ||
        std::strcmp(argv[1], "--version") == 0) {
        std::printf("SGas %s\n", sgas_version());
        return 0;
    }
    if (std::strcmp(argv[1], "-e") == 0) {
        if (argc < 3) { print_usage(argv[0]); return 64; }
        return sgas_run_string(argv[2]);
    }
    return sgas_run_file(argv[1]);
}