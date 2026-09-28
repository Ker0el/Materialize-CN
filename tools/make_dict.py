# -*- coding: utf-8 -*-
"""Materialize 1.78 dictionary: English -> Chinese, emitted as the runtime TSV.

Hand-written rather than machine translated. The glossary below is the one
locked in by the earlier Materialize pass; DeepL gets 3D/graphics vocabulary
systematically wrong (Normal->正常, Plane->飞机, Pillow->枕头, Sat->周六,
Mids->中场, Property Map->房产地图 ...), and at a few hundred entries reviewing
machine output costs more than writing it.

Keys are matched EXACTLY, including leading/trailing spaces and case -- several
labels in this app are " Diffuse" / "File Masks: " with deliberate padding, and
GUI.Toggle's "Low"/"High" are different things from GUI.Button's "Low"/"High".
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'dict', 'dict.tsv')

# ---------------------------------------------------------------- glossary --
# 高度图 / 漫反射(贴图) / 法线(贴图) / 金属度(贴图) / 光滑度(贴图) / 边缘(贴图)
# AO 贴图 / 属性贴图 / 偏置 / 对比度 / 饱和度 / 色相 / 亮度 / 泛光 / 暗角 /
# 镜头光晕 / 景深 / 置换 / 平铺 / 偏移 / 遮挡 / 遮罩 / 中间调 / 高通 / 采样

D = {}


def add(en, zh):
    D[en] = zh


# ---- 主界面 / MainGui -------------------------------------------------------
add('Height Map', '高度图')
add('Diffuse Map', '漫反射贴图')
add('Normal Map', '法线贴图')
add('Metallic Map', '金属度贴图')
add('Smoothness Map', '光滑度贴图')
add('Edge Map', '边缘贴图')
add('AO Map', 'AO 贴图')
add('Property Map', '属性贴图')
add('Quick Save', '快速保存')
add('Quick Save\r\nProperty Map', '快速保存\r\n属性贴图')
add('Save\r\nProperty Map', '保存\r\n属性贴图')
add('Save Project', '保存工程')
add('Load Project', '载入工程')
add('Backup Project', '备份工程')
add('Flip Normal Y', '翻转法线 Y')
add('Post Process', '后处理')
add('Show Full\r\nMaterial', '显示完整\r\n材质')
add('Next\r\nCube Map', '下一张\r\n立方体贴图')
add('Tile\r\nMaps', '平铺\r\n贴图')
add('Clear All\r\nTexture Maps', '清除全部\r\n贴图')
add('Adjust\r\nAlignment', '调整\r\n对齐')
add('Create', '创建')
add('Edit', '编辑')
add('Clear', '清除')
add('Preview', '预览')
add('Saving Options', '保存选项')
add('File Format', '文件格式')
add('Map for Channel', '通道对应贴图')
add('Red None', '红 无')
add('Green None', '绿 无')
add('Blue None', '蓝 无')
add('Make Suggestion', '提交建议')
add('Hide Gui', '隐藏界面')
add('Show Gui', '显示界面')
add('Full Screen', '全屏')
add('Windowed', '窗口模式')
add('Quit', '退出')
add('Are You Sure?', '确定吗？')
add('Yes', '是')
add('No', '否')
add('None', '无')
add('Metallic', '金属度')
add('Smoothness', '光滑度')
add('Edge', '边缘')
add('Ambient Occlusion', '环境光遮蔽')
add('AO + Edge', 'AO + 边缘')
add('Settings', '设置')
add('Controls', '操作说明')

# ---- 各功能窗口标题 ---------------------------------------------------------
add('Texture Alignment Adjuster', '贴图对齐调整器')
add('Normal + Depth to AO', '法线 + 深度转 AO')
add('Edge from Normal', '由法线生成边缘')
add('Edit Diffuse', '编辑漫反射')
add('Full Material', '完整材质')
add('Height From Diffuse', '由漫反射生成高度')
add('Metallic From Diffuse', '由漫反射生成金属度')
add('Normal From Height', '由高度生成法线')
add('Smoothness From Diffuse', '由漫反射生成光滑度')
add('Tiling Texture Maker', '平铺贴图生成器')
add('Setting and Preferences', '设置与首选项')
add('Make A Suggestion!', '提出建议！')
add('Error', '错误')
add('File Browser', '文件浏览器')

# ---- Alignment -------------------------------------------------------------
add('Alignment Reveal Slider', '对齐显示滑块')
add('Preview Map', '预览贴图')
add('Reset Points', '重置控制点')
add('Set All Maps', '设置全部贴图')

# ---- HeightFromDiffuse -----------------------------------------------------
add('Height Reveal Slider', '高度显示滑块')
add('Frequency Contrast Equalizer', '频率对比度均衡器')
add('Height', '高度')
add('Set as Height Map', '设为高度图')
add('Details', '细节')
add('Original Diffuse Map', '原始漫反射贴图')
add(' Shape from Diffuse (Uncheked from Height)', ' 由漫反射生成形状（取消勾选则改用高度图）')
add('Cracks', '裂纹')
add('Funky', '夸张')
add('Adjust', '调整')

# ---- NormalFromHeight ------------------------------------------------------
add('Normal Reveal Slider', '法线显示滑块')
add('Frequency Weight Equalizer', '频率权重均衡器')
add('Angular Intensity', '角度强度')
add('Angularity Amount', '棱角程度')
add(' Shape Recognition, Rotation, Spread, Bias', ' 形状识别、旋转、扩散、偏置')
add('Set as Normal Map', '设为法线图')
add('Natural', '自然')
add('Smooth', '平滑')
add('Crisp', '锐利')
add('Mids', '中间调')

# ---- EdgeFromNormal --------------------------------------------------------
add('Frequency Equalizer', '频率均衡器')
add('Presets', '预设')
add('Set as Edge Map', '设为边缘贴图')
add('Default', '默认')
add('Displace', '置换')
add('Soft', '柔和')
add('Tight', '紧凑')

# ---- Metallic / Smoothness -------------------------------------------------
add('Metalic Reveal Slider', '金属度显示滑块')
add('Smoothness Reveal Slider', '光滑度显示滑块')
add('Diffuse Reveal Slider', '漫反射显示滑块')
add('Set as Metallic', '设为金属度')
add('Set as Smoothness', '设为光滑度')
add('Set as Diffuse', '设为漫反射')
add(' Use Edited Diffuse', ' 使用编辑后的漫反射')
add(' Use Original Diffuse', ' 使用原始漫反射')
add('Original Diffuse', '原始漫反射')
add('Isolate Mask', '隔离遮罩')
add('Use Color Sample 1', '使用颜色采样 1')
add('Use Color Sample 2', '使用颜色采样 2')

# ---- Material (球体预览) ---------------------------------------------------
add('Sphere', '球体')
add('Cube', '立方体')
add('Cylinder', '圆柱体')
add('Plane', '平面')
add('Light Color', '光照颜色')
add('Intensity', '强度')
add('Pick Color', '拾取颜色')

# ---- Normal + Depth to AO --------------------------------------------------
add('Set as AO Map', '设为 AO 贴图')
add(' Normal', ' 法线')
add(' Max Style', ' Max 风格')

# ---- Edit Diffuse ----------------------------------------------------------
add(' Diffuse', ' 漫反射')

# ---- TilingTextureMaker ----------------------------------------------------
add('New Texture Size X', '新贴图宽度 X')
add('New Texture Size Y', '新贴图高度 Y')
add('Tiling Test Variables', '平铺测试参数')
add('Technique Overlap', '方式：重叠')
add('Technique Splat', '方式：Splat')
add('Set Maps', '设置贴图')

# ---- Settings --------------------------------------------------------------
add('Setting and Preferences', '设置与首选项')
add('Enable Post Process', '启用后处理')
add(' Enable Post Process By Default', ' 默认启用后处理')
add('Use Auto Focus', '使用自动对焦')
add('Normal Map Style', '法线贴图风格')
add(' Maya Style', ' Maya 风格')
add('Set Default File Format', '设置默认文件格式')
add('Set Default Property Map Channels', '设置默认属性贴图通道')
add('Save and Close', '保存并关闭')

# ---- Controls --------------------------------------------------------------
add('Move Model', '移动模型')
add('Rotate Model', '旋转模型')
add('Rotate Light', '旋转光照')
add('Rotate Background', '旋转背景')
add('Zoom In/Out', '缩放')
add('Middle Mouse Button', '鼠标中键')
add('Middle Mouse Button + L', '鼠标中键 + L')
add('Middle Mouse Button + B', '鼠标中键 + B')
add('Right Mouse Button', '鼠标右键')
add('Mouse Scroll Wheel', '鼠标滚轮')
add('Close', '关闭')

# ---- Suggestion ------------------------------------------------------------
add('Got a suggestion?  Send it to us!', '有建议？发给我们！')
add('Sending....', '发送中……')
add('Something went wrong!', '出错了！')
add("Thanks!  I'll get right on that!", '谢谢！我们会尽快处理！')
add('Send', '发送')
add('Try Again', '重试')

# ---- Authentication --------------------------------------------------------
add('Authentication', '授权验证')
add('Authenticate', '验证')
add('Authenticating...', '正在验证……')
add('Enter your email and key below.', '请在下方输入邮箱和密钥。')
add('Email:', '邮箱：')
add('Authentication Key:', '授权密钥：')
add('Paste Key', '粘贴密钥')
add('Continue', '继续')
add('Next', '下一步')
add('Update', '更新')
add('No authentication found.  Click next to enter your email and key.',
    '未找到授权信息。点击「下一步」输入邮箱和密钥。')
add('Authenticaion data corrupt.  Click next to re-enter your email and key.',
    '授权数据已损坏。点击「下一步」重新输入邮箱和密钥。')
add('Authenticaion key is incorrect.  Click next to re-enter your email and key.',
    '授权密钥不正确。点击「下一步」重新输入邮箱和密钥。')
add('Authenticaion failed.  That shouldn\'t happen...  You must have done something wrong, try it again.',
    '验证失败。这本不该发生……可能是操作有误，请重试。')
add('There is no account for the email you entered.  Go to www.boundingboxsoftware.com/materialize/ to set up your account. Then click next to enter your email and key.',
    '该邮箱没有对应账号。请到 www.boundingboxsoftware.com/materialize/ 注册账号，然后点击「下一步」输入邮箱和密钥。')
add('Your version of Materialize is out of date. Click below to update to the latest version.',
    '当前 Materialize 版本已过时。点击下方按钮更新到最新版本。')
add('The Materialize beta is closed.  Check www.boundingboxsoftware.com to get your copy today!',
    'Materialize 测试版已结束。请访问 www.boundingboxsoftware.com 获取正式版！')
add('You are trying to run Materialize from a different machine too soon after your last session.',
    '您尝试在上一台机器使用后过短时间内，于另一台机器上运行 Materialize。')
add('Your trial period has ended.\r\nGo to www.boundingboxsoftware.com/materialize/ to purchase a license!',
    '试用期已结束。\r\n请访问 www.boundingboxsoftware.com/materialize/ 购买授权！')

# ---- File browser ----------------------------------------------------------
add('Favorites', '收藏夹')
add('Add To Favorites', '添加到收藏夹')
add('Create New Folder', '新建文件夹')
add('Paste Directory', '粘贴目录')
add('No entries', '（空）')
add('Filename: ', '文件名： ')
add('File Masks: ', '文件掩码： ')
add('Ok', '确定')
add('Cancel', '取消')
add('Confirm', '确认')
add('Open', '打开')
add('Open Folder', '打开文件夹')
add('Load', '载入')
add('Save', '保存')
add('Explore Dirs', '浏览目录')
add('Accept Dirs', '接受目录')
add('Allow Deletes', '允许删除')
add('Hide Extensions', '隐藏扩展名')
add('Show Places', '显示位置栏')
add('Show Text Input', '显示文本输入框')
add('Unity Default', 'Unity 默认')
add('Choose Music', '选择音乐')
add('Textures', '贴图')
add('File Browser', '文件浏览器')
add('Info', '信息')
add('Offset:', '偏移：')
add('Scales:', '缩放：')
add('Skin:', '皮肤：')
add('Low', '低')
add('High', '高')

# ---- 参数面板：GuiHelper.Slider 的标题 -------------------------------------
# 这些不是 GUI.Label 的参数，是直接传进 GuiHelper.Slider(rect, "标题", ...) 的，
# 静态提取必须专门覆盖 GuiHelper.*，否则整片参数面板会漏掉。
add('Metallic Multiplier', '金属度倍数')
add('Smoothness Multiplier', '光滑度倍数')
add('Paralax Displacement', '视差置换')          # 原文拼写就是 Paralax
add('Edge Amount', '边缘强度')
add('Ambient Occlusion Power', '环境光遮蔽强度')
add('Texture Tiling', '贴图平铺')
add('Texture Tiling X', '贴图平铺 X')
add('Texture Tiling Y', '贴图平铺 Y')
add('Texture Offset X', '贴图偏移 X')
add('Texture Offset Y', '贴图偏移 Y')

add('Bloom Threshold', '泛光阈值')
add('Bloom Amount', '泛光强度')
add('Lens Flare Amount', '镜头光晕强度')
add('Lens Dirt Amount', '镜头污渍强度')
add('Vignette Amount', '暗角强度')
add('DOF Max Blur', '景深最大模糊')
add('DOF Focal Depth', '景深对焦深度')
add('DOF Max Distance', '景深最大距离')

add('AO Bias', 'AO 偏置')
add('AO Power', 'AO 强度')
add('AO pixel Spread', 'AO 像素扩散')
add('Pixel Depth', '像素深度')
add('Blend Normal AO and Depth AO', '混合法线 AO 与深度 AO')
add('Set as AO Map', '设为 AO 贴图')

add('Base Smoothness', '基础光滑度')
add('Metal Smoothness', '金属光滑度')
add('High Pass Blur Size', '高通模糊半径')
add('Sample Blur Size', '采样模糊半径')
add('Blur Size', '模糊半径')
add('High Pass Overlay', '高通叠加')

add('Average Color Blur Size', '平均色模糊半径')
add('Dark Spot Removal', '暗斑去除')
add('Hot Spot Removal', '亮斑去除')
add('Keep Original Color', '保留原始颜色')
add('Light Mask Power', '光照遮罩强度')
add('Shadow Mask Power', '阴影遮罩强度')
add('Overlay Blur Contrast', '叠加模糊对比度')
add('Overlay Blur Size', '叠加模糊半径')
add('Remove Light', '去除光照')
add('Remove Shadow', '去除阴影')
add('Saturation', '饱和度')
add('Sample Blend', '采样混合')
add('Sample Spread', '采样扩散')
add('Sample Spread Boost', '采样扩散增强')
add('Final Gain', '最终增益')
add('Final Bias', '最终偏置')
add('Final Contrast', '最终对比度')
add('Pre Contrast', '预对比度')
add('Crevice Amount', '缝隙强度')
add('Edge Falloff', '边缘衰减')
add('Pillow', '膨胀')
add('Pinch', '收缩')

add('Overlap X', '重叠 X')
add('Overlap Y', '重叠 Y')
add('Splat Scale', 'Splat 缩放')
add('Splat Rotation', 'Splat 旋转')
add('Splat Random Rotation', 'Splat 随机旋转')
add('Splat Randomize', 'Splat 随机化')
add('Splat Wooble Amount', 'Splat 抖动强度')      # 原文拼写就是 Wooble

add('Lens Distort Correction', '镜头畸变校正')
add('Perspective Correction X', '透视校正 X')
add('Perspective Correction Y', '透视校正 Y')

# ---- 文件对话框标题（FileBrowser.ShowBrowser 的第一个参数）------------------
add('Load Project', '载入工程')
add('Open Height Map', '打开高度图')
add('Open Diffuse Map', '打开漫反射贴图')
add('Open Normal Map', '打开法线贴图')
add('Open Metallic Map', '打开金属度贴图')
add('Open Smoothness Map', '打开光滑度贴图')
add('Open Edge Map', '打开边缘贴图')
add('Open AO Map', '打开 AO 贴图')
add('Save Height Map', '保存高度图')
add('Save Diffuse Map', '保存漫反射贴图')
add('Save Normal Map', '保存法线贴图')
add('Save Metallic Map', '保存金属度贴图')
add('Save Smoothness Map', '保存光滑度贴图')
add('Save Edge Map', '保存边缘贴图')
add('Save AO Map', '保存 AO 贴图')
add('Save Property Map', '保存属性贴图')
add('Pick a texture for the Floor', '为地面选择贴图')
add('Pick a texture for the Objects', '为物体选择贴图')
add('Pick a texture for the Walls', '为墙壁选择贴图')
add("Pick an audio file for the scene's music", '为场景音乐选择音频文件')

# ---- 文件浏览器自身的标签 ---------------------------------------------------
# 这几个是 places 侧栏的显示名。FileBrowser 里 placesList[].name 与
# currentDirectoryParts[] 都只用于绘制，导航走的是 .path / Path.GetDirectoryName，
# 所以翻译它们不会影响实际路径解析。
add('Drives', '驱动器')
add('Desktop', '桌面')
add('My Docs', '我的文档')
add('My Pics', '我的图片')
add('Nothing to preview', '无可预览内容')
add('Select', '选择')

# ---- 色相/饱和度/亮度滑块（颜色采样权重）-------------------------------------
add('Hue', '色相')
add('Sat', '饱和度')
add('Lum', '亮度')

# ---- 贴图面板上的 P/C/O/S 小按钮（20x20 px）---------------------------------
# 含义是 Paste / Copy / Open / Save，只作为 GUI.Button 的标签出现，不是数据键。
# 按钮只有 20x20 像素，双字词放不下 —— 所以 native 侧给这四个按钮单独做了处理：
# 用整行富余空间把它们加宽到 24px，并换成一份 clipping=Overflow 的 button 样式副本。
add('P', '粘贴')
add('C', '复制')
add('O', '打开')
add('S', '保存')

# ---- 保持原样（格式名 / 通道标记）---------------------------------------------
KEEP = ['BMP', 'JPG', 'PNG', 'TGA', 'TIFF', 'R', 'G', 'B', 'R:', 'G:', 'B:', '-']


def esc(s):
    return (s.replace('\\', '\\\\')
             .replace('\r', '\\r')
             .replace('\n', '\\n')
             .replace('\t', '\\t'))


def main():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w', encoding='utf-8') as f:
        f.write('# Materialize 1.78 runtime dictionary  (english<TAB>chinese)\n')
        f.write('# \\n \\r \\t \\\\ are unescaped by the loader; matching is exact and case-sensitive.\n')
        for k in sorted(D, key=lambda x: x.lower()):
            # escape BOTH halves: values for the CRLF-joined labels contain real
            # newlines, and an unescaped one silently splits the TSV line.
            f.write('%s\t%s\n' % (esc(k), esc(D[k])))
    print('[ok] %d entries -> %s' % (len(D), os.path.normpath(OUT)))
    print('[i] deliberately left untranslated: %s' % ', '.join(KEEP))


if __name__ == '__main__':
    main()
