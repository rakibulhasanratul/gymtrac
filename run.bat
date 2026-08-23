@echo off

if not exist build mkdir build
if not exist data mkdir data

if "%~1"=="test" (
    gcc -Wall -Wextra -pedantic -g -fmacro-prefix-map=src/= test\*.c test\**\*.c src\**\*.c -o build\test_runner.exe -lm
    build\test_runner.exe
) else (
    gcc -Wall -Wextra -pedantic -g -fmacro-prefix-map=src/= src\*.c src\modules\*.c src\utils\*.c -o build\gymtrac.exe -lm
    build\gymtrac.exe
)
