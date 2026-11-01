# build_lib_calcmgr.ps1 — build CalcManager.lib (static lib) from repo-relative engine source.
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\build\build_lib_calcmgr.ps1
#   powershell ... -File .\build\build_lib_calcmgr.ps1 -VcVars "C:\vs\vs\VC\Auxiliary\Build\vcvars64.bat"
# Locates engine source under <repo>/src/CalcManager/CalcManager, writes outputs to <repo>/build/out.
param([string]$VcVars = "")

$root = Split-Path -Parent $PSScriptRoot                 # repo root = <repo>/build/..
$eng  = Join-Path $root "src\CalcManager\CalcManager"
$out  = Join-Path $root "build\out"
if (-not (Test-Path -LiteralPath $out)) { New-Item -ItemType Directory -Path $out | Out-Null }
foreach ($sub in @("lib","dll","tests")) {
    if (-not (Test-Path -LiteralPath "$out\$sub")) { New-Item -ItemType Directory -Path "$out\$sub" | Out-Null }
}

if (-not (Test-Path -LiteralPath "$eng\pch.h")) {
    Write-Error ("engine root not found: " + $eng); exit 1
}

# --- locate vcvars64.bat: explicit -VcVars, then common auto-detect paths ---
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

$files = @(
 "CalculatorHistory.cpp","CalculatorManager.cpp","CEngine\calc.cpp","CEngine\CalcUtils.cpp",
 "CEngine\History.cpp","CEngine\CalcInput.cpp","CEngine\Number.cpp","CEngine\Rational.cpp",
 "CEngine\scicomm.cpp","CEngine\scidisp.cpp","CEngine\scifunc.cpp","CEngine\RationalMath.cpp",
 "CEngine\scioper.cpp","CEngine\sciset.cpp","ExpressionCommand.cpp","Ratpack\basex.cpp",
 "Ratpack\conv.cpp","Ratpack\exp.cpp","Ratpack\fact.cpp","Ratpack\itrans.cpp","Ratpack\itransh.cpp",
 "Ratpack\logic.cpp","Ratpack\num.cpp","Ratpack\rat.cpp","Ratpack\support.cpp","Ratpack\trans.cpp",
 "Ratpack\transh.cpp","NumberFormattingUtils.cpp","UnitConverter.cpp"
)
$body = @(
 "/c /O2 /EHsc /std:c++20 /permissive- /Zc:twoPhase- /utf-8 /DUNICODE /D_UNICODE"
 "/FI`"$eng\pch.h`""
 "/I`"$eng`""
 "/I`"$eng\CEngine`""
 "/I`"$eng\Header Files`""
 "/I`"$eng\Ratpack`""
 ($files | ForEach-Object { "`"$eng\$_`"" })
) -join "`r`n"
$rspPath = "$out\calc_lib.rsp"
[IO.File]::WriteAllText($rspPath, $body, (New-Object Text.UTF8Encoding $false))

$bat = @"
@echo off
call "$VcVars" >nul
if errorlevel 1 exit /b 1
cd /d "$out\lib"
cl @"$rspPath"
if errorlevel 1 exit /b 1
lib /OUT:"$out\CalcManager.lib" "$out\lib\*.obj"
if errorlevel 1 exit /b 1
echo BUILD_LIB_DONE
"@
$bat = $bat -replace "`n","`r`n"
$batPath = Join-Path $out "build_lib_calcmgr.bat"
[IO.File]::WriteAllText($batPath, $bat, (New-Object Text.UTF8Encoding $false))
Write-Output ("lib rsp: " + $rspPath)
Write-Output ("vcvars: " + $VcVars)
& cmd.exe /c "`"$batPath`""
Write-Output ("BAT_EXIT: " + $LASTEXITCODE)
