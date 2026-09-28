@echo off
setlocal
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set "DETOURS=E:\3D\Gaea\_detours"
if not exist "%VCVARS%" set "VCVARS=E:\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
call "%VCVARS%" >nul 2>&1
if errorlevel 1 ( echo [FAIL] vcvars64 & exit /b 1 )
cd /d "%~dp0"

echo [1/6] winhttp_orig.dll
if not exist winhttp_orig.dll copy /y "C:\Windows\System32\winhttp.dll" winhttp_orig.dll >nul
if not exist winhttp_orig.dll ( echo   FAIL copy winhttp & exit /b 1 )

echo [2/6] winhttp_orig.lib  ^(import lib for the real dll^)
del /q winhttp_orig.lib winhttp.dll winhttp.exp winhttp.lib proxy.obj thunks.obj >nul 2>&1
lib /nologo /def:winhttp_orig.def /machine:x64 /out:winhttp_orig.lib
if errorlevel 1 ( echo   FAIL lib & exit /b 1 )

echo [3/6] thunks.asm
ml64 /nologo /c /Fo thunks.obj thunks.asm
if errorlevel 1 ( echo   FAIL ml64 & exit /b 1 )

echo [4/6] proxy.cpp
cl /nologo /c /O2 /MT /EHsc /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE /utf-8 ^
   /I"%DETOURS%\include" /Foproxy.obj proxy.cpp
if errorlevel 1 ( echo   FAIL cl & exit /b 1 )

echo [5/6] link winhttp.dll
link /nologo /DLL /OUT:winhttp.dll /MACHINE:X64 /DEF:winhttp.def ^
   proxy.obj thunks.obj winhttp_orig.lib "%DETOURS%\lib.X64\detours.lib" ^
   user32.lib kernel32.lib advapi32.lib shell32.lib
if errorlevel 1 ( echo   FAIL link & exit /b 1 )

echo [6/6] result
if exist winhttp.dll ( echo BUILD_OK & dir /b winhttp.dll ) else ( echo BUILD_FAILED & exit /b 1 )
