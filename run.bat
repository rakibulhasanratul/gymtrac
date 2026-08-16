@echo off
setlocal EnableDelayedExpansion

set "BUILD_DIR=build"
set "COMPILE_FLAGS=-Wall -Wextra -pedantic -g"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

if "%~1"=="test" (
    set "SOURCE_FILES="
    for /r src %%f in (*.c) do (
        if not "%%~nxf"=="main.c" set "SOURCE_FILES=!SOURCE_FILES! "%%f""
    )
    gcc %COMPILE_FLAGS% test\*.c %SOURCE_FILES% -o "%BUILD_DIR%\test_runner.exe"
    "%BUILD_DIR%\test_runner.exe"
) else (
    set "SOURCE_FILES="
    for /r src %%f in (*.c) do set "SOURCE_FILES=!SOURCE_FILES! "%%f""
    gcc %COMPILE_FLAGS% %SOURCE_FILES% -o "%BUILD_DIR%\gymtrac.exe"
    "%BUILD_DIR%\gymtrac.exe"
)
