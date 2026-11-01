$src = "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calculator-main\src"
$out = "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build"
$eng = "$src\CalcManager"

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
 "/I`"$src`""
 "/I`"$eng`""
 ($files | ForEach-Object { "`"$eng\$_`"" })
) -join "`r`n"

if (-not (Test-Path -LiteralPath $out)) { New-Item -ItemType Directory -Path $out | Out-Null }
$rspPath = "$out\calc_lib.rsp"
[IO.File]::WriteAllText($rspPath, $body, (New-Object Text.UTF8Encoding $false))

$bat = @"
@echo off
call "C:\vs\vs\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "$out"
cl @"$rspPath"
if errorlevel 1 exit /b 1
lib /OUT:"$out\CalcManager.lib" "$out\*.obj"
if errorlevel 1 exit /b 1
echo BUILD_LIB_DONE
"@
$bat = $bat -replace "`n","`r`n"
$batPath = "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\build_calcmgr_lib.bat"
[IO.File]::WriteAllText($batPath, $bat, (New-Object Text.UTF8Encoding $false))
Write-Output ("rsp written: "+$rspPath)
& cmd.exe /c "`"$batPath`""
Write-Output ("BAT_EXIT: "+$LASTEXITCODE)
