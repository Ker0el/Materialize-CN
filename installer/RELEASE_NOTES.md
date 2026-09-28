# Materialize 1.78 星空汉化版

把 Materialize 界面完整汉化成简体中文。**安装包内含原版程序**，装完直接启动就是中文。

## 安装

下载下方的 `Materialize-1.78-CN-Setup.exe`，双击运行即可。

不需要管理员权限，默认装到用户目录，不会弹 UAC。

## 中英切换

三种方式，任选：

- **程序内** —— 点右上角「隐藏界面」**下方**的那个按钮
- **托盘图标** —— 左键点一下切换，右键出菜单
- **热键** —— <kbd>Ctrl</kbd> + <kbd>Alt</kbd> + <kbd>Z</kbd>

## 汉化范围

- **288 条翻译**，覆盖主界面、全部功能窗口、文件浏览器、设置、授权界面
- 3D / 图形学术语按统一术语表处理：法线 / 漫反射 / 金属度 / 光滑度 / 边缘 /
  AO 贴图 / 属性贴图 / 置换 / 泛光 / 暗角 / 镜头光晕 / 景深 / 中间调 …
- 贴图面板上原来那四个 20×20 的小按钮（`P`/`C`/`O`/`S`）已重排成 2×2 并加宽，
  译作 **粘贴 / 复制 / 打开 / 保存**

## 不修改原程序

`Materialize.exe`、`UnityPlayer.dll`、`Assembly-CSharp.dll` 与官方原版**逐字节一致**。

汉化在运行时完成 —— 代理 `winhttp.dll` 接入 Mono 运行时，钩住 Unity IMGUI 的
7 个渲染出口替换显示文本。卸载后界面立即恢复英文，不留痕迹。

## 卸载

开始菜单 → 星空汉化 → 卸载，或在「设置 → 应用」里卸载。卸载会一并清理词典、
配置、日志和运行时生成的 `winhttp_orig.dll`。

## 说明

- 原程序 **Materialize** © Bounding Box Software，以 **GPL-3.0** 发布，
  源码：<https://github.com/BoundingBoxSoftware/Materialize>
- 安装目录内附 `LICENSE` 与 `NOTICE.md`（来源与改动声明）
- 汉化补丁的源码、词典、构建脚本：<https://github.com/Ker0el/Materialize-CN>

---

**星空汉化** · <https://space.bilibili.com/177308205> · 如果帮到了你，欢迎点个 Star ⭐
