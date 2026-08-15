#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
COMPILE_FLAGS=(-Wall -Wextra -pedantic -g)

mkdir -p "$BUILD_DIR"

if [ "${1:-}" = "test" ]; then
    mapfile -t SOURCE_FILES < <(find src -name '*.c' ! -name 'main.c' | sort)
    gcc "${COMPILE_FLAGS[@]}" test_main.c "${SOURCE_FILES[@]}" -o "$BUILD_DIR/test_runner"
    "$BUILD_DIR/test_runner"
else
    mapfile -t SOURCE_FILES < <(find src -name '*.c' | sort)
    gcc "${COMPILE_FLAGS[@]}" "${SOURCE_FILES[@]}" -o "$BUILD_DIR/gymtrac"
    "$BUILD_DIR/gymtrac"
fi
