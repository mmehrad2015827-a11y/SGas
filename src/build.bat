@echo off
setlocal
cd /d C:\SGas

if not exist .tmpbuild mkdir .tmpbuild

echo [SGas] Compiling + linking ...
gcc -std=c11 -Wall -Wextra -O2 ^
    -Iinclude ^
    -Isrc\common -Isrc\lexer -Isrc\ast -Isrc\parser ^
    -Isrc\compiler -Isrc\vm -Isrc\runtime ^
    src\common\common.c ^
    src\common\value.c ^
    src\lexer\lexer.c ^
    src\ast\ast.c ^
    src\parser\parser.c ^
    src\compiler\compiler.c ^
    src\vm\vm.c ^
    src\runtime\runtime.c ^
    src\stdlib\stdlib.c ^
    src\main.c ^
    -o sgas.exe -lm

if errorlevel 1 (
    echo [SGas] BUILD FAILED
    exit /b 1
)

echo [SGas] Build OK
echo.
echo [SGas] Running examples\hello.sgas ...
sgas.exe examples\hello.sgas

endlocal