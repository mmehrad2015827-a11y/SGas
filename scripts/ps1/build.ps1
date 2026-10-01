#Requires -Version 5.1
<#
.SYNOPSIS
    Build the SGas interpreter on Windows.

.DESCRIPTION
    Detects an available C compiler (gcc, clang, or MSVC cl.exe),
    compiles every translation unit under src/, and links the final
    executable into build\sgas.exe.

    Override the compiler by setting the CC environment variable, e.g.
        $env:CC = "C:\msys64\mingw64\bin\gcc.exe"
        .\scripts\ps1\build.ps1
#>

$ErrorActionPreference = "Stop"

# --- locate project root (two levels up from scripts\ps1\) -------------
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $Root

$BuildDir = "build"
if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# --- pick a compiler ---------------------------------------------------
function Get-SgasCompiler {
    if ($env:CC -and (Get-Command $env:CC -ErrorAction SilentlyContinue)) {
        return $env:CC
    }
    foreach ($candidate in @("gcc", "clang", "cl")) {
        if (Get-Command $candidate -ErrorAction SilentlyContinue) {
            return $candidate
        }
    }
    throw "No C compiler found. Install gcc/clang, or open a VS Developer Prompt for cl.exe, or set `$env:CC."
}

$CC = Get-SgasCompiler
$ccName  = (Split-Path -Leaf $CC) -replace '\.exe$', ''
$IsMsvc  = ($ccName -eq 'cl')

Write-Host "[SGas] Using compiler: $CC"
if ($IsMsvc) { Write-Host "[SGas] Mode: MSVC" }
else         { Write-Host "[SGas] Mode: gcc/clang-compatible" }

# --- sources / include dirs -------------------------------------------
$SrcFiles = @(
    "src/common/common.c",
    "src/common/value.c",
    "src/lexer/lexer.c",
    "src/ast/ast.c",
    "src/parser/parser.c",
    "src/compiler/compiler.c",
    "src/vm/vm.c",
    "src/runtime/runtime.c",
    "src/stdlib/stdlib.c",
    "src/main.c"
)

$IncDirs = @(
    "include",
    "src/common",
    "src/lexer",
    "src/ast",
    "src/parser",
    "src/compiler",
    "src/vm",
    "src/runtime",
    "src/stdlib"
)

# --- flag sets ---------------------------------------------------------
if ($IsMsvc) {
    $CommonFlags = @("/nologo", "/std:c11", "/W3", "/O2")
    $IncFlags    = $IncDirs | ForEach-Object { "/I$_" }
    $ObjExt      = ".obj"
} else {
    $CommonFlags = @("-std=c11", "-Wall", "-Wextra", "-O2")
    $IncFlags    = $IncDirs | ForEach-Object { "-I$_" }
    $ObjExt      = ".o"
}

$ExePath  = Join-Path $BuildDir "sgas.exe"
$ObjFiles = @()

# --- compile -----------------------------------------------------------
foreach ($src in $SrcFiles) {
    $objName = ($src -replace '[\\/]', '_') -replace '\.c$', $ObjExt
    $objPath = Join-Path $BuildDir $objName

    Write-Host "  CC  $src"
    if ($IsMsvc) {
        & $CC $CommonFlags $IncFlags "/c" $src "/Fo$objPath"
    } else {
        & $CC $CommonFlags $IncFlags "-c" $src "-o" $objPath
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: $src"
    }
    $ObjFiles += $objPath
}

# --- link --------------------------------------------------------------
Write-Host "[SGas] Linking ..."
if ($IsMsvc) {
    & $CC "/nologo" "/Fe:$ExePath" $ObjFiles
} else {
    & $CC $CommonFlags $ObjFiles "-o" $ExePath "-lm"
}
if ($LASTEXITCODE -ne 0) { throw "Linking failed" }

Write-Host "[SGas] Done: $ExePath"