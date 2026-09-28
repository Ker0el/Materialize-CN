; Materialize 1.78 星空汉化版 —— Inno Setup 安装包脚本
;
; 中文向导：Inno 6 自带的 29 种语言里没有中文，必须自带 ChineseSimplified.isl
; （与本机 Default.isl 逐键对齐，296/296），所以 [Languages] 是必需的。
;
; 编译：ISCC.exe Materialize-CN.iss
; 产物：output\Materialize-1.78-CN-Setup.exe
;
; 产物名保持 ASCII：用 gh release upload 上传时，中文文件名会被丢掉
; （实测 "…-星空汉化版-安装包.exe" 上传后变成 "Materialize-1.78-.-.exe"）。
; 中文名字体现在 Release 标题和向导里，不影响用户观感。

#define AppName        "Materialize 星空汉化版"
#define AppVersion     "1.78"
#define AppPublisher   "星空汉化"
#define AppURL         "https://space.bilibili.com/177308205"
#define RepoURL        "https://github.com/Ker0el/Materialize-CN"
#define SrcRoot        ".."

[Setup]
AppId={{8F3A2C41-6D5B-4E77-9A0C-2B7E4D1F8A63}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#RepoURL}
AppUpdatesURL={#RepoURL}
VersionInfoVersion=1.78.0.0
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} 安装程序
VersionInfoProductName={#AppName}

; 不需要管理员权限：装到用户目录，不弹 UAC
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
DefaultDirName={autopf}\Materialize
DisableDirPage=no
DisableProgramGroupPage=yes
AllowNoIcons=yes

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

OutputDir=output
OutputBaseFilename=Materialize-1.78-CN-Setup
SetupIconFile=app.ico
UninstallDisplayIcon={app}\Materialize.exe
UninstallDisplayName={#AppName}

Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ShowLanguageDialog=no

InfoBeforeFile=安装前必读.txt

[Languages]
Name: "cn"; MessagesFile: "ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务："

[Files]
; 原版程序（GPL-3.0，来自上游，未作修改）
Source: "{#SrcRoot}\game\Materialize.exe";   DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRoot}\game\UnityPlayer.dll";   DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRoot}\game\Materialize_Data\*"; DestDir: "{app}\Materialize_Data"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

; 汉化组件
Source: "{#SrcRoot}\winhttp.dll";            DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRoot}\_hanhua\dict.tsv";       DestDir: "{app}\_hanhua"; Flags: ignoreversion
Source: "{#SrcRoot}\_hanhua\hook.ini";       DestDir: "{app}\_hanhua"; Flags: ignoreversion
Source: "{#SrcRoot}\_hanhua\说明.md";         DestDir: "{app}\_hanhua"; Flags: ignoreversion

; 许可与声明（GPL-3.0 要求随二进制一起分发）
Source: "{#SrcRoot}\LICENSE";                DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRoot}\NOTICE.md";              DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcRoot}\安装说明.txt";             DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\Materialize.exe"
Name: "{autodesktop}\{#AppName}";  Filename: "{app}\Materialize.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Materialize.exe"; Description: "立即运行 {#AppName}"; \
    WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; 运行时会生成日志和用户设置，卸载时一并清掉
Type: filesandordirs; Name: "{app}\_hanhua\logs"
Type: files;          Name: "{app}\settings.txt"
Type: files;          Name: "{app}\favorites.txt"
; winhttp_orig.dll 是 [Code] 里拷进来的，不在 [Files] 清单中，
; Inno 默认不会删，必须显式列出，否则卸载后会留一个 1.2MB 的残留。
Type: files;          Name: "{app}\winhttp_orig.dll"

[Code]
// winhttp_orig.dll 是系统 winhttp.dll 的副本。它是微软的系统文件，不在 Materialize
// 的 GPL 覆盖范围内，不能随安装包分发，所以安装时从用户自己的系统复制一份。
// 必须取 64 位的 System32；ArchitecturesInstallIn64BitMode 保证 {sys} 指向它。
procedure CreateOrigWinhttp();
var
  Src, Dst: String;
begin
  Src := ExpandConstant('{sys}\winhttp.dll');
  Dst := ExpandConstant('{app}\winhttp_orig.dll');
  if FileExists(Dst) then
    Exit;
  if not CopyFile(Src, Dst, False) then
    MsgBox('无法从系统复制 winhttp.dll：' + #13#10 + Src + #13#10 + #13#10 +
           '请确认该文件存在，否则 Materialize 将无法启动。', mbError, MB_OK);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    CreateOrigWinhttp();
end;
