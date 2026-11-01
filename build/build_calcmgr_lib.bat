@echo off
call "C:\vs\vs\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build"
cl @"C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\calc_lib.rsp"
if errorlevel 1 exit /b 1
lib /OUT:"C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\CalcManager.lib" "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\*.obj"
if errorlevel 1 exit /b 1
echo BUILD_LIB_DONE