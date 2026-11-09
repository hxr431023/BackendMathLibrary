# build_tests_calcmgr.ps1 — build & run the assertion tests against build/out/CalcManager.dll.
# Requires CalcManager.dll + CalcManager_dll.lib first (run build_lib + build_dll).
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\build\build_tests_calcmgr.ps1
#   powershell ... -File .\build\build_tests_calcmgr.ps1 -VcVars "...\vcvars64.bat"
param([string]$VcVars = "")

$root = Split-Path -Parent $PSScriptRoot                 # repo root
$eng  = Join-Path $root "src\CalcManager\CalcManager"
$api  = Join-Path $root "api"
$tests = Join-Path $root "tests"
$out  = Join-Path $root "build\out"
if (-not (Test-Path -LiteralPath $out)) { New-Item -ItemType Directory -Path $out | Out-Null }

if (-not (Test-Path -LiteralPath "$out\CalcManager_dll.lib")) {
    Write-Error "CalcManager_dll.lib not found in $out. Run build_lib_calcmgr.ps1 and build_dll_calcmgr.ps1 first."
    exit 1
}

# --- locate vcvars64.bat ---
if ([string]::IsNullOrEmpty($VcVars)) {
    $cands = @(
        "C:\vs\vs\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )
    $VcVars = $cands | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (-not $VcVars -or -not (Test-Path -LiteralPath $VcVars)) {
    Write-Error "vcvars64.bat not found. Pass -VcVars <path-to-vcvars64.bat>."
    exit 1
}

$testNames = @("test_calc", "test_engine", "test_unit", "test_cmp", "test_concurrency", "test_settings", "test_native")

$cmds = @(
    "@echo off"
    "call `"$VcVars`" >nul"
    "if errorlevel 1 exit /b 1"
    "cd /d `"$out\tests`""
    "setlocal"
)
foreach ($t in $testNames) {
    $cmds += "cl /nologo /O2 /EHsc /std:c++20 /permissive- /utf-8 /DUNICODE /D_UNICODE /I`"$eng`" /I`"$eng\Header Files`" /I`"$eng\Ratpack`" /I`"$api`" /I`"$tests`" /Fe:`"$out\$t.exe`" `"$tests\$t.cpp`" /link /LIBPATH:`"$out`" CalcManager_dll.lib"
    $cmds += "if errorlevel 1 exit /b 1"
    $cmds += "`"$out\$t.exe`" > `"$out\$t`_run.log`" 2>&1"
    # Compare the raw exit code as text: a crash code such as 0xC0000005 is
    # negative, and "if errorlevel 1" treats a negative value as success, which
    # hid a crashing test behind a PASSED line.
    $cmds += "set RC=%ERRORLEVEL%"
    $cmds += "if not `"%RC%`"==`"0`" (echo $t FAILED rc=%RC%) else (echo $t PASSED)"
}
$cmds += "echo BUILD_TESTS_DONE"
$bat = ($cmds -join "`r`n")
$batPath = Join-Path $out "build_tests_calcmgr.bat"
[IO.File]::WriteAllText($batPath, $bat, (New-Object Text.UTF8Encoding $false))
Write-Output ("tests bat: " + $batPath)
& cmd.exe /c "`"$batPath`""
Write-Output ("BAT_EXIT: " + $LASTEXITCODE)
