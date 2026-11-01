@echo off
call "C:\vs\vs\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build"
cl @"C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\calc_api.rsp"
if errorlevel 1 exit /b 1
link /DLL /OUT:"C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\CalcManager.dll" "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\calc_api.obj" "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\calc_engine.obj" "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\calc_unit.obj" "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\calc_cmp.obj" "C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\CalcManager.lib" /IMPLIB:"C:\Users\aussiehcl BO\Doubao\chats\2026-10-31\new-chat\calc_build\CalcManager_dll.lib" /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1
echo BUILD_DLL_DONE