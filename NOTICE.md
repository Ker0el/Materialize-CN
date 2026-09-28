# 来源与改动声明 · NOTICE

本文件说明本仓库里**哪些是别人的**、**哪些是我们做的**，以及各自的许可。

---

## 一、上游作品

**Materialize** — 把图片转换成 PBR 材质贴图的工具

- 作者：Bounding Box Software
- 源码：<https://github.com/BoundingBoxSoftware/Materialize>
- 许可：**GNU General Public License v3.0**
- 版权：Copyright (C) Bounding Box Software

本仓库 `game/` 目录下的文件（`Materialize.exe`、`UnityPlayer.dll`、`Materialize_Data/`）
来自 Materialize 1.78 官方发行版，**原样附带，未作任何修改**。
可以自行用哈希比对验证：

| 文件 | MD5 |
|---|---|
| `Materialize.exe` | `7b65b7c856770f0c3c14b4f7103dc183` |
| `UnityPlayer.dll` | `745d09bc77ef107b1b4d4c4d99b44b05` |
| `Materialize_Data/Managed/Assembly-CSharp.dll` | `8dac04295c967b43bd029ab833d85cae` |

`game/` 目录按上游的 GPL-3.0 分发，版权归 Bounding Box Software 所有，
完整协议见仓库根目录的 [LICENSE](LICENSE)。

---

## 二、本仓库新增的部分

以下内容由 **星空汉化** 编写，同样以 **GPL-3.0** 发布：

| 路径 | 内容 |
|---|---|
| `src/proxy.cpp` | winhttp 代理 + Mono 运行时引导 + IMGUI icall 钩子 |
| `src/thunks.asm`、`src/winhttp.def`、`src/winhttp_orig.def`、`src/names.txt` | 导出转发桩（由 `tools/gen_proxy.py` 生成） |
| `src/build.bat` | 构建脚本 |
| `winhttp.dll` | 上述源码编译出的成品 |
| `_hanhua/dict.tsv` | 288 条英文→简中词典（手工校对） |
| `_hanhua/hook.ini`、`_hanhua/说明.md` | 配置与说明 |
| `安装.bat`、`卸载.bat`、`安装说明.txt` | 安装 / 卸载 |
| `tools/*.py` | 词典提取、生成、校验、代理脚手架生成、打包 |
| `docs/*.png` | 截图 |

**改动说明（GPL-3.0 要求标注）：**
本仓库**没有修改上游程序**。汉化在运行时完成 —— 通过代理 `winhttp.dll`
把汉化组件注入进程，在 IMGUI 的渲染出口替换显示文本。
`game/` 里的文件与官方原版逐字节一致。

---

## 三、没有包含的东西

- **`winhttp_orig.dll`**：系统 `C:\Windows\System32\winhttp.dll` 的副本，
  属于 Microsoft，不在 GPL 覆盖范围内，因此**不随仓库分发**。
  `安装.bat` 会在安装时从**用户自己的系统**复制一份。
- 任何反编译产物、日志、构建中间文件。

---

## 四、对应源码

GPL-3.0 要求分发二进制时提供对应源码：

- 上游程序（`game/`）的对应源码：<https://github.com/BoundingBoxSoftware/Materialize>
- 汉化组件（`winhttp.dll`）的对应源码：本仓库 `src/`，用 `src/build.bat` 即可重建

---

汉化制作：**星空汉化** · <https://space.bilibili.com/177308205>
