@echo off
setlocal
title Materialize 星空汉化 - 安装

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
echo   安装目标：%GAME%
echo.

echo   [1/3] 备份系统 winhttp.dll 为 winhttp_orig.dll
if exist "%GAME%\winhttp_orig.dll" (
  echo         已存在，跳过
) else (
  copy /y "%SystemRoot%\System32\winhttp.dll" "%GAME%\winhttp_orig.dll" >nul
  if errorlevel 1 ( echo         [x] 备份失败 & pause & exit /b 1 )
)

echo   [2/3] 复制汉化组件
copy /y "%SRC%\winhttp.dll" "%GAME%\winhttp.dll" >nul
if errorlevel 1 ( echo         [x] 复制 winhttp.dll 失败 & pause & exit /b 1 )
if not exist "%GAME%\_hanhua"        mkdir "%GAME%\_hanhua"
if not exist "%GAME%\_hanhua\logs"   mkdir "%GAME%\_hanhua\logs"
copy /y "%SRC%\_hanhua\dict.tsv"  "%GAME%\_hanhua\dict.tsv"  >nul
copy /y "%SRC%\_hanhua\hook.ini"  "%GAME%\_hanhua\hook.ini"  >nul
copy /y "%SRC%\_hanhua\说明.md"    "%GAME%\_hanhua\说明.md"    >nul

echo   [3/3] 完成
echo.
echo   ────────────────────────────────────────────────
echo    直接双击 Materialize.exe 即可，界面自动变中文。
echo.
echo    中英切换：托盘图标左键点击 / 右键菜单 / 热键 Ctrl+Alt+Z
echo    B 站主页：https://space.bilibili.com/177308205
echo.
echo    Materialize 本体文件一个字节都没有改动。
echo    卸载：运行 卸载.bat
echo   ────────────────────────────────────────────────
echo.
pause
