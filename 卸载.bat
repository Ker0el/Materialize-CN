@echo off
setlocal
title Materialize 星空汉化 - 卸载

set "SRC=%~dp0"
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"

set "GAME=%~1"
if "%GAME%"=="" if exist "%CD%\Materialize.exe"         set "GAME=%CD%"
if "%GAME%"=="" if exist "%CD%\..\Materialize.exe"      set "GAME=%CD%\.."
if "%GAME%"=="" if exist "%SRC%\..\Materialize.exe"     set "GAME=%SRC%\.."
if "%GAME%"=="" if exist "%SRC%\game\Materialize.exe"   set "GAME=%SRC%\game"
if "%GAME%"=="game" set "GAME=%SRC%\game"

if not exist "%GAME%\Materialize.exe" (
  echo.
  echo   [错误] 没找到 Materialize.exe
  echo.
  echo   如果是从本仓库运行的：直接把 game 目录作为参数传进来
  echo        安装.bat game
  echo   如果 Materialize 装在别处：
  echo        安装.bat "D:\路径\Materialize_1.78"
  echo.
  pause
  exit /b 1
)

echo.
echo   卸载目标：%GAME%
echo.

del /q "%GAME%\winhttp.dll" 2>nul
if exist "%GAME%\winhttp.dll" (
  echo   [x] winhttp.dll 删除失败 —— 请先完全关闭 Materialize 再试
) else (
  echo   [OK] winhttp.dll 已移除
)

del /q "%GAME%\winhttp_orig.dll" 2>nul
if exist "%GAME%\winhttp_orig.dll" (
  echo   [x] winhttp_orig.dll 删除失败
) else (
  echo   [OK] winhttp_orig.dll 已移除
)

echo.
echo   汉化已卸载，界面恢复英文。
echo   词典、配置和日志保留在 %GAME%\_hanhua\ ，需要的话手动删除。
echo.
pause
