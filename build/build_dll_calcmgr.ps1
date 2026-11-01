$src = "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calculator-main\src"
$out = "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build"
$eng = "$src\CalcManager"

$rsp = @"
/c /O2 /EHsc /std:c++20 /permissive- /Zc:twoPhase- /utf-8 /DUNICODE /D_UNICODE
/I"$src"
/I"$eng"
"$out\calc_api.cpp"
"$out\calc_engine.cpp"
"$out\calc_unit.cpp"
"$out\calc_cmp.cpp"
"@
$rsp = $rsp -replace "`n","`r`n"
$rspPath = "$out\calc_api.rsp"
[IO.File]::WriteAllText($rspPath, $rsp, (New-Object Text.UTF8Encoding $false))

$bat = @"
@echo off
call "C:\vs\vs\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "$out"
cl @"$rspPath"
if errorlevel 1 exit /b 1
link /DLL /OUT:"$out\CalcManager.dll" "$out\calc_api.obj" "$out\calc_engine.obj" "$out\calc_unit.obj" "$out\calc_cmp.obj" "$out\CalcManager.lib" /IMPLIB:"$out\CalcManager_dll.lib" /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1
echo BUILD_DLL_DONE
"@
$bat = $bat -replace "`n","`r`n"
$batPath = "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\build_calcmgr_dll.bat"
[IO.File]::WriteAllText($batPath, $bat, (New-Object Text.UTF8Encoding $false))
Write-Output ("dll rsp: "+$rspPath)
& cmd.exe /c "`"$batPath`""
Write-Output ("DLL_BAT_EXIT: "+$LASTEXITCODE)
