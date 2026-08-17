@echo off

if not exist build mkdir build

if "%~1"=="test" (
    gcc -Wall -Wextra -pedantic -g test\test_main.c test\modules\*.c test\utils\*.c src\modules\*.c src\utils\*.c -o build\test_runner.exe
    build\test_runner.exe
) else (
    gcc -Wall -Wextra -pedantic -g src\main.c src\modules\*.c src\utils\*.c -o build\gymtrac.exe
    build\gymtrac.exe
)
