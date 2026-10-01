#Requires -Version 5.1
<#
.SYNOPSIS
    Run the SGas unit tests on Windows.

.DESCRIPTION
    Ensures the interpreter objects are built (calls build.ps1 if needed),
    then compiles and runs tests\lexer_test.c, tests\parser_test.c and
    tests\vm_test.c against the project's object files.
#>

$ErrorActionPreference = "Stop"

# --- locate project root ----------------------------------------------
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $Root

$BuildDir = "build"
if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# --- pick the same compiler build.ps1 would ---------------------------
function Get-SgasCompiler {
    if ($env:CC -and (Get-Command $env:CC -ErrorAction SilentlyContinue)) {
        return $env:CC
    }
    foreach ($candidate in @("gcc", "clang", "cl")) {
        if (Get-Command $candidate -ErrorAction SilentlyContinue) {
            return $candidate
        }
    }
    throw "No C compiler found."
}

$CC      = Get-SgasCompiler
$ccName  = (Split-Path -Leaf $CC) -replace '\.exe$', ''
$IsMsvc  = ($ccName -eq 'cl')

$ObjExt  = if ($IsMsvc) { ".obj" } else { ".o" }

# --- the object files every test links against ------------------------
$ObjectStems = @(
    "src_common_common",
    "src_common_value",
    "src_lexer_lexer",
    "src_ast_ast",
    "src_parser_parser",
    "src_compiler_compiler",
    "src_vm_vm",
    "src_runtime_runtime",
    "src_stdlib_stdlib"
)
$Objs = $ObjectStems | ForEach-Object { Join-Path $BuildDir ($_ + $ObjExt) }

# --- ensure the objects exist; build once if not ----------------------
if (!(Test-Path $Objs[0])) {
    Write-Host "[SGas] Building objects first ..."
    & (Join-Path $PSScriptRoot "build.ps1")
    if ($LASTEXITCODE -ne 0) { throw "build.ps1 failed" }
}

# --- include / flag setup ---------------------------------------------
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
$IncFlags = if ($IsMsvc) { $IncDirs | ForEach-Object { "/I$_" } }
            else         { $IncDirs | ForEach-Object { "-I$_" } }

# --- test definitions --------------------------------------------------
$Tests = @(
    @{ Name = "lexer_test";  Src = "tests/lexer_test.c"  },
    @{ Name = "parser_test"; Src = "tests/parser_test.c" },
    @{ Name = "vm_test";     Src = "tests/vm_test.c"     }
)

# --- compile + run each test ------------------------------------------
foreach ($t in $Tests) {
    Write-Host "[TEST] $($t.Name)"
    $out = Join-Path $BuildDir ($t.Name + ".exe")

    if ($IsMsvc) {
        & $CC "/nologo" "/std:c11" "/W3" $IncFlags $t.Src $Objs "/Fe:$out"
    } else {
        & $CC "-std=c11" "-Wall" "-Wextra" "-O2" $IncFlags $t.Src $Objs "-o" $out "-lm"
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Test build failed: $($t.Name)"
    }

    & $out
    if ($LASTEXITCODE -ne 0) {
        throw "Test failed: $($t.Name)"
    }
}

Write-Host "[SGas] All tests passed."