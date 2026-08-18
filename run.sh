#!/usr/bin/env bash
set -euo pipefail

mkdir -p build

if [ "${1:-}" = "test" ]; then
    gcc -Wall -Wextra -pedantic -g test/test_main.c test/modules/*.c test/utils/*.c src/modules/*.c src/utils/*.c -o build/test_runner -lm
    build/test_runner
else
    gcc -Wall -Wextra -pedantic -g src/main.c src/modules/*.c src/utils/*.c -o build/gymtrac -lm
    build/gymtrac
fi
