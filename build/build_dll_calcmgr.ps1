# build_dll_calcmgr.ps1 — build CalcManager.dll (pure C API DLL) from repo-relative sources.
# Requires CalcManager.lib first (run build_lib_calcmgr.ps1), which this script links against.
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\build\build_dll_calcmgr.ps1
#   powershell ... -File .\build\build_dll_calcmgr.ps1 -VcVars "C:\vs\vs\VC\Auxiliary\Build\vcvars64.bat"
param([string]$VcVars = "")

$root = Split-Path -Parent $PSScriptRoot                 # repo root
$eng  = Join-Path $root "src\CalcManager\CalcManager"
$api  = Join-Path $root "api"
$out  = Join-Path $root "build\out"
if (-not (Test-Path -LiteralPath $out)) { New-Item -ItemType Directory -Path $out | Out-Null }

if (-not (Test-Path -LiteralPath "$out\CalcManager.lib")) {
    Write-Error "CalcManager.lib not found in $out. Run build_lib_calcmgr.ps1 first."
    exit 1
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

$rsp = @"
/c /O2 /EHsc /std:c++20 /permissive- /Zc:twoPhase- /utf-8 /DUNICODE /D_UNICODE
/I"$eng"
/I"$eng\CEngine"
/I"$eng\Header Files"
/I"$eng\Ratpack"
/I"$api"
"$api\calc_api.cpp"
"$api\calc_engine.cpp"
"$api\calc_unit.cpp"
"$api\calc_cmp.cpp"
"@
$rsp = $rsp -replace "`n","`r`n"
$rspPath = "$out\calc_api.rsp"
[IO.File]::WriteAllText($rspPath, $rsp, (New-Object Text.UTF8Encoding $false))

$bat = @"
@echo off
call "$VcVars" >nul
if errorlevel 1 exit /b 1
cd /d "$out\dll"
cl @"$rspPath"
if errorlevel 1 exit /b 1
link /DLL /OUT:"$out\CalcManager.dll" "$out\dll\*.obj" "$out\CalcManager.lib" /IMPLIB:"$out\CalcManager_dll.lib" /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1
echo BUILD_DLL_DONE
"@
$bat = $bat -replace "`n","`r`n"
$batPath = Join-Path $out "build_dll_calcmgr.bat"
[IO.File]::WriteAllText($batPath, $bat, (New-Object Text.UTF8Encoding $false))
Write-Output ("dll rsp: " + $rspPath)
Write-Output ("vcvars: " + $VcVars)
& cmd.exe /c "`"$batPath`""
Write-Output ("BAT_EXIT: " + $LASTEXITCODE)
