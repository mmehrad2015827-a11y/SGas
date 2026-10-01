#!/usr/bin/env bash
# scripts/build.sh - Build the SGas interpreter.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

CC="${CC:-cc}"
CXX="${CXX:-c++}"
CFLAGS="${CFLAGS:--std=c11 -Wall -Wextra -O2 -Iinclude -Isrc/common -Isrc/lexer -Isrc/ast -Isrc/parser -Isrc/compiler -Isrc/vm -Isrc/runtime -Isrc/stdlib}"

echo "[SGas] Building with $CC ..."

SRC=(
    src/common/common.c
    src/common/value.c
    src/lexer/lexer.c
    src/ast/ast.c
    src/parser/parser.c
    src/compiler/compiler.c
    src/vm/vm.c
    src/runtime/runtime.c
    src/stdlib/stdlib.c
    src/main.c
)

for f in "${SRC[@]}"; do
    obj="$BUILD_DIR/$(echo "$f" | tr '/' '_' | sed 's/\.c$/.o/')"
    echo "  CC  $f"
    $CC $CFLAGS -c "$f" -o "$obj"
done

$CC $CFLAGS "$BUILD_DIR"/*.o -o "$BUILD_DIR/sgas" -lm

echo "[SGas] Done: $BUILD_DIR/sgas"