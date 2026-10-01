#!/usr/bin/env bash
# scripts/test.sh - Run SGas unit tests.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

CC="${CC:-cc}"
CFLAGS="${CFLAGS:--std=c11 -Wall -Wextra -O2 -Iinclude -Isrc/common -Isrc/lexer -Isrc/ast -Isrc/parser -Isrc/compiler -Isrc/vm -Isrc/runtime -Isrc/stdlib}"

OBJS=(
    "$BUILD_DIR/src_common_common.o"
    "$BUILD_DIR/src_common_value.o"
    "$BUILD_DIR/src_lexer_lexer.o"
    "$BUILD_DIR/src_ast_ast.o"
    "$BUILD_DIR/src_parser_parser.o"
    "$BUILD_DIR/src_compiler_compiler.o"
    "$BUILD_DIR/src_vm_vm.o"
    "$BUILD_DIR/src_runtime_runtime.o"
    "$BUILD_DIR/src_stdlib_stdlib.o"
)

if [[ ! -f "${OBJS[0]}" ]]; then
    echo "[SGas] Building objects first..."
    ./scripts/build.sh
fi

run_test() {
    local name="$1" src="$2" out="$BUILD_DIR/$1"
    echo "[TEST] $name"
    $CC $CFLAGS "$src" "${OBJS[@]}" -o "$out" -lm
    "$out"
}

run_test lexer_test  tests/lexer_test.c
run_test parser_test tests/parser_test.c
run_test vm_test     tests/vm_test.c

echo "[SGas] All tests passed."