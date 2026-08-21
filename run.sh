#!/usr/bin/env bash
set -euo pipefail

mkdir -p build

if [ "${1:-}" = "test" ]; then
    mkdir -p test_data
    gcc -Wall -Wextra -pedantic -g -fmacro-prefix-map=src/= -DDEFAULT_DATA_DIRECTORY=\"test_data\" test/*.c test/**/*.c src/**/*.c -o build/test_runner -lm
    build/test_runner
else
    mkdir -p data
    gcc -Wall -Wextra -pedantic -g -fmacro-prefix-map=src/= src/*.c src/**/*.c -o build/gymtrac -lm
    build/gymtrac
fi
