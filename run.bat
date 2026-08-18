@echo off

if not exist build mkdir build
if not exist data mkdir data

if "%~1"=="test" (
    gcc -Wall -Wextra -pedantic -g test\test_main.c test\modules\*.c test\utils\*.c src\modules\*.c src\utils\*.c -o build\test_runner.exe -lm
    build\test_runner.exe
) else (
    gcc -Wall -Wextra -pedantic -g src\main.c src\modules\*.c src\utils\*.c -o build\gymtrac.exe -lm
    build\gymtrac.exe
)
