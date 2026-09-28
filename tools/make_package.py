# -*- coding: utf-8 -*-
"""Assemble the distributable package under _hanhua/out/.

.bat files are written as GBK + CRLF: cmd.exe reads batch files in the OEM code
page (936 here), so UTF-8 Chinese would come out as mojibake, and LF line
endings break multi-line if/for blocks. The readme is UTF-8 with BOM so Notepad
renders it correctly.
"""
import os
import shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..'))          # _hanhua
OUT = os.path.join(ROOT, 'out', 'Materialize-星空汉化')
SITE = 'https://space.bilibili.com/177308205'

DETECT = r'''
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
'''

INSTALL = r'''@echo off
setlocal
title Materialize 星空汉化 - 安装
''' + DETECT + r'''
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
echo    B 站主页：__SITE__
echo.
echo    Materialize 本体文件一个字节都没有改动。
echo    卸载：运行 卸载.bat
echo   ────────────────────────────────────────────────
echo.
pause
'''

UNINSTALL = r'''@echo off
setlocal
title Materialize 星空汉化 - 卸载
''' + DETECT + r'''
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
'''

INI = '''# ─────────────────────────────────────────────────────────────
#  Materialize 星空汉化  运行时汉化配置
#  B 站：%s
# ─────────────────────────────────────────────────────────────
#  词典 dict.tsv 改完保存即生效（程序每 2 秒检查一次）。
#  本文件里 mode / lang / title / tray / hotkey 的改动需要重启程序。

mode = translate        ; translate=汉化界面   probe=只写日志、不改界面（排错用）
lang = zh               ; 启动时的语言：zh=中文  en=英文

tray   = 1              ; 托盘图标：左键切换中/英，右键出菜单
hotkey = Ctrl+Alt+Z     ; 中英切换热键（支持 Ctrl/Alt/Shift/Win + 字母或 F1~F24），留空=不用

title    = Materialize - 星空汉化版   ; 中文界面时的窗口标题
title_en = Materialize                 ; 英文界面时的窗口标题

# ── 排错用（平时不用动）─────────────────────────────────────
log_miss = 1   ; 记录「界面上出现过、但词典里没有」的串 → _hanhua\\logs\\miss.log
log_all  = 0   ; 记录所有被绘制的串 → _hanhua\\logs\\drawn.log（用于查漏，文件会很大）
''' % SITE

README = '''# Materialize 1.78 星空汉化

运行时汉化，**Materialize 本体一个字节都没有改动**，卸载即完全还原。

- B 站：%s

## 用法

| 操作 | 说明 |
|---|---|
| 启动 | 直接双击 `Materialize.exe`，界面自动变中文，不需要额外步骤 |
| 中英切换 | 托盘图标**左键**点击；或**右键**菜单选「中文 / English」；或热键 **Ctrl+Alt+Z** |
| 重新载入词典 | 托盘右键 →「重新载入词典」（改完 `_hanhua\\dict.tsv` 其实会自动生效） |
| 卸载 | 运行 `卸载.bat`，界面恢复英文 |

## 装了什么

```
Materialize_1.78\\
  Materialize.exe          原版，未改动
  UnityPlayer.dll          原版，未改动
  Materialize_Data\\        原版，未改动
  winhttp.dll              ← 新增：汉化组件（代理 + Hook）
  winhttp_orig.dll         ← 新增：系统 winhttp.dll 的备份
  _hanhua\\                  ← 新增：词典、配置、日志
```

`UnityPlayer.dll` 会从系统加载 `winhttp.dll`。Windows 对非 KnownDLL 的库
按「程序目录优先」查找，所以程序目录里这个同名 DLL 会被加载，我们在它的
`DllMain` 里起线程接入汉化。**它把原版 winhttp 的导出全部转发给了
`winhttp_orig.dll`，所以程序自身的网络功能（如授权验证）行为不变。**

## 汉化是怎么做的

界面是 Unity 的 **IMGUI**（`OnGUI`），所有文字最终都会走到 Unity 引擎里几个
内部调用（icall）上。我们在这几个出口处把 `GUIContent` 里的文本换掉：

| 出口 | 覆盖 |
|---|---|
| `GUI::INTERNAL_CALL_DoLabel` | `GUI.Label` / `GUILayout.Label` |
| `GUI::INTERNAL_CALL_DoButton` | `GUI.Button` / `GUILayout.Button` |
| `GUI::INTERNAL_CALL_DoToggle` | `GUI.Toggle` / `GUILayout.Toggle` |
| `GUI::INTERNAL_CALL_Internal_DoWindow` | 窗口标题栏 |
| `GUIStyle::Internal_Draw` / `INTERNAL_CALL_Internal_Draw2` | `GUI.Box`、`style.Draw`、自绘 |

这是**渲染出口**，此时查找、比较、状态判定都已经做完了 —— 所以换文本
只可能影响「显示什么」，不可能影响程序逻辑，也不会写坏工程文件。

其余细节：

- 文本指针**只在这一次调用期间替换，调用完立刻还原**
  （`GUIContent.Temp` 复用同一个静态实例，不还原会串到别的控件上）
- 词典**精确匹配、区分大小写**（`Normal`=法线，`normal` 是另一回事）
- 托盘图标、热键、窗口标题都由这个 DLL 提供，不动程序自己的界面布局

## 词典

`_hanhua\\dict.tsv`，每行 `英文<TAB>中文`，`#` 开头是注释。
支持 `\\n` `\\r` `\\t` `\\\\` 转义（有些按钮标题本身就是两行的）。

改完**保存即生效**，不用重启。

## 已知取舍

- `BMP / JPG / PNG / TGA / TIFF`、通道标记 `R: G: B:` 按惯例保留英文
- 文件浏览器里列出的**真实文件夹名**（如 `Desktop`、`Materialize_Data`）不翻译 —— 那是磁盘上的实际名字
- 贴图面板上的 `P/C/O/S` 四个小按钮（20×20 像素）译作 `粘/复/开/存`，
  即 Paste / Copy / Open / Save；两字词在该尺寸下会截断
''' % SITE

NOTES = '''Materialize 1.78 星空汉化 —— 安装说明
════════════════════════════════════════════════════════

  B 站：%s

【怎么装】
  1. 把本文件夹里的东西复制到 Materialize 的安装目录
     （就是有 Materialize.exe 的那个文件夹）
  2. 双击 安装.bat

  也可以直接把安装目录当参数传进去：
     安装.bat "D:\\路径\\Materialize_1.78"

【怎么用】
  装完直接双击 Materialize.exe 就行，界面自动是中文，不需要额外启动器。

  中英切换有三种方式，任选：
    · 托盘图标左键点一下
    · 托盘图标右键 → 菜单里选「中文 / English」
    · 按热键 Ctrl+Alt+Z

  热键和托盘图标都可以在 _hanhua\\hook.ini 里改或关掉。

【怎么卸】
  双击 卸载.bat，界面立刻恢复英文。

【装了什么】
  只新增文件，Materialize 自身文件一个字节都没动：

    winhttp.dll        汉化组件（转发 + Hook）
    winhttp_orig.dll   系统 winhttp.dll 的备份
    _hanhua\\            词典、配置、日志

【想改翻译】
  打开 _hanhua\\dict.tsv，每行「英文<TAB>中文」，改完保存即生效，不用重启。

【出问题了】
  · 界面没变中文 → 看 _hanhua\\logs\\hook.log，末尾会写明哪一步失败
  · 想找没翻译到的地方 → 把 hook.ini 里的 log_miss 设为 1，
    正常用一遍软件，_hanhua\\logs\\miss.log 里就是所有漏掉的原文
  · 想让界面先回到英文排查 → 按 Ctrl+Alt+Z，或者删掉 winhttp.dll

【注意】
  Materialize 是 Bounding Box Software 的软件，本汉化只改运行时显示，
  不修改也不分发原程序。

════════════════════════════════════════════════════════
''' % SITE


def w(path, text, enc, crlf=True):
    data = text.replace('\r\n', '\n')
    if crlf:
        data = data.replace('\n', '\r\n')
    with open(path, 'wb') as f:
        f.write(data.encode(enc))
    print('  %-28s %6d bytes  [%s]' % (os.path.basename(path), os.path.getsize(path), enc))


def main():
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(os.path.join(OUT, '_hanhua'))
    print('[package] %s' % OUT)

    nat = os.path.join(ROOT, 'native')
    shutil.copy2(os.path.join(nat, 'winhttp.dll'), OUT)
    shutil.copy2(os.path.join(nat, 'winhttp_orig.dll'), OUT)
    print('  winhttp.dll / winhttp_orig.dll copied')

    w(os.path.join(OUT, '安装.bat'), INSTALL.replace('__SITE__', SITE), 'gbk')
    w(os.path.join(OUT, '卸载.bat'), UNINSTALL, 'gbk')
    w(os.path.join(OUT, '安装说明.txt'), NOTES, 'utf-8-sig')

    w(os.path.join(OUT, '_hanhua', 'hook.ini'), INI, 'utf-8')
    w(os.path.join(OUT, '_hanhua', '说明.md'), README, 'utf-8')
    shutil.copy2(os.path.join(ROOT, 'dict', 'dict.tsv'),
                 os.path.join(OUT, '_hanhua', 'dict.tsv'))
    print('  _hanhua/dict.tsv copied (%d lines)' %
          sum(1 for _ in open(os.path.join(ROOT, 'dict', 'dict.tsv'), encoding='utf-8')))

    print('[ok] package ready')


if __name__ == '__main__':
    main()
