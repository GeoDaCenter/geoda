; *** Inno Setup version 4.2.2+ Chinese messages ***
;
; To download user-contributed translations of this file, go to:
;   http://www.jrsoftware.org/is3rdparty.php
;
; Note: When translating this text, do not add periods (.) to the end of
; messages that didn't have them already, because on those messages Inno
; Setup adds the periods automatically (appending a period would result in
; two periods being displayed).
;
; $jrsoftware: issrc/Files/Default.isl,v 1.58 2004/04/07 20:17:13 jr Exp $

[LangOptions]
LanguageName=<7B80><4F53><4E2D><6587>
LanguageID=$0804
LanguageCodePage=936
; If the language you are translating to requires special font faces or
; sizes, uncomment any of the following entries and change them accordingly.
DialogFontName=宋体
DialogFontSize=9
WelcomeFontName=宋体
WelcomeFontSize=12
TitleFontName=宋体
;TitleFontSize=29
CopyrightFontName=宋体
;CopyrightFontSize=8

[Messages]

; *** Application titles
SetupAppTitle=安装
SetupWindowTitle=安装 - %1
UninstallAppTitle=卸载
UninstallAppFullTitle=卸载 - %1

; *** Misc. common
InformationTitle=提示
ConfirmTitle=确认
ErrorTitle=错误

; *** SetupLdr messages
SetupLdrStartupMessage=即将安装 %1。你确定要继续吗？
LdrCannotCreateTemp=不能创建临时文件，安装终止
LdrCannotExecTemp=不能在临时目录执行文件，安装终止

; *** Startup error messages
LastErrorMessage=%1.%n%nError %2: %3
SetupFileMissing=缺少文件 %1 ，请确定程序正确或获得最新版本。
SetupFileCorrupt=安装文件损坏，请获得最新版本。
SetupFileCorruptOrWrongVer=安装文件损坏, 或不兼容当前版本。请确定程序正确或获得最新版本。
NotOnThisPlatform=本程序不能运行在，“ %1”上。
OnlyOnThisPlatform=本程序必须运行在“%1”上。
WinVersionTooLowError=本程序需要“ %1 ”的版本在 %2 以上。
WinVersionTooHighError=本程序不能安装在“ %1 ” %2 以上版本。
AdminPrivilegesRequired=您必须以“系统管理员”登录才能安装本程序。
PowerUserPrivilegesRequired=您必须以“系统管理员”或“Power Users”组成员登录才能安装本程序。
SetupAppRunningError=安装程序检测到“ %1 ”正在运行，%n%n请关闭该程序, 然后点击【确定】继续，或点击【取消】退出安装。
UninstallAppRunningError=安装程序检测到“ %1 ”正在运行，%n%n请关闭该程序, 然后点击【确定】继续，或点击【取消】退出安装。

; *** Misc. errors
ErrorCreatingDir=安装程序不能创建目录"%1"
ErrorTooManyFilesInDir=目录"%1"中包含太多文件，不能再创建文件

; *** Setup common messages
ExitSetupTitle=退出安装
ExitSetupMessage=安装尚未完成，如果你现在退出，系统将不被安装。%n%n你可以在其他时间运行程序以完成安装.%n%n确定退出安装？
AboutSetupMenuItem=关于(&A)安装...
AboutSetupTitle=关于安装
AboutSetupMessage=%1 版本 %2%n%3%n%n%1 主页:%n%4
AboutSetupNote=

; *** Buttons
ButtonBack=< 上一步(&B)
ButtonNext=下一步(&N) >
ButtonInstall=安装(&I)
ButtonOK=确定
ButtonCancel=取消
ButtonYes=是(&Y)
ButtonYesToAll=全部是(&A)
ButtonNo=否(&N)
ButtonNoToAll=全部否(&o)
ButtonFinish=完成(&F)
ButtonBrowse=浏览(&B)
ButtonWizardBrowse=浏览(&r)...
ButtonNewFolder=创建新目录(&M)

; *** "Select Language" dialog messages
SelectLanguageTitle=选择安装语言
SelectLanguageLabel=选择安装时提示语言：

; *** Common wizard text
ClickNext=点击【下一步】继续，或【取消】退出安装。
BeveledLabel=
BrowseDialogTitle=浏览目录
BrowseDialogLabel=选择下面目录列表，然后点击确定。
NewFolderName=新建目录

; *** "Welcome" wizard page
WelcomeLabel1=欢迎使用 [name] 安装向导
WelcomeLabel2=即将安装 [name/ver] 到您的计算机上，%n%n建议您在继续安装前关闭其他全部程序。

; *** "Password" wizard page
WizardPassword=密码
PasswordLabel1=本安装程序被密码保护
PasswordLabel3=请输入密码，然后点击【下一步】继续，（密码区分大小写）。
PasswordEditLabel=密码(&P):
IncorrectPassword=密码不正确，请重新输入

; *** "License Agreement" wizard page
WizardLicense=许可协议
LicenseLabel=在安装前请读以下重要信息
LicenseLabel3=请读以下许可协议，您必须接受以下条款才能继续安装。
LicenseAccepted=接受此协议(&A)
LicenseNotAccepted=不接受此协议(&D)

; *** "Information" wizard pages
WizardInfoBefore=提示
InfoBeforeLabel=在安装前请读以下重要信息
InfoBeforeClickLabel=当你准备好继续安装，点击【下一步】
WizardInfoAfter=提示
InfoAfterLabel=在继续前读以下重要信息
InfoAfterClickLabel=当你准备好继续，点击【下一步】

; *** "User Information" wizard page
WizardUserInfo=用户信息
UserInfoDesc=请输入您的信息
UserInfoName=用户名称(&U):
UserInfoOrg=公司名(&O):
UserInfoSerial=序列号(&S):
UserInfoNameRequired=您必须输入名称

; *** "Select Destination Location" wizard page
WizardSelectDir=选择目标路径
SelectDirDesc=确定将 [name] 安装到这里？
SelectDirLabel3=程序将安装 [name] 到下面目录
SelectDirBrowseLabel=点击【下一步】继续，如果您想选择不同的目录，点击【浏览】
DiskSpaceMBLabel=需要不少于 [mb] MB 磁盘空间
ToUNCPathname=程序不能安装到一 UNC 路径，如果您想安装到网络上，则需要映射网络路径
InvalidPath=您必须输入一个保护驱动盘符的完整路径; 例如:%n%nC:\APP%n%或者:%n%n\\server\share
InvalidDrive=选择的驱动器或UNC共享不存在或不能访问，请选择其他
DiskSpaceWarningTitle=没有足够的磁盘空间
DiskSpaceWarning=安装程序需要不少于%1KB磁盘空间, 但目标盘仅有%2KB.%n%n您还想继续安装吗？
DirNameTooLong=目录名或路径太长
InvalidDirName=目录名无效
BadDirName32=目录名不能包含以下字符:%n%n%1
DirExistsTitle=目录已经存在
DirExists=目录:%n%n%1%n%n已经存在，您还是想安装到该目录下吗？
DirDoesntExistTitle=目录不存在
DirDoesntExist=目录:%n%n%1%n%n不存在，您想创建该目录吗？

; *** "Select Components" wizard page
WizardSelectComponents=选择组件
SelectComponentsDesc=哪些组件将被安装？
SelectComponentsLabel2=选择您想安装的组件，清楚您不想安装的组件，完成后点击【下一步】继续
FullInstallation=完全安装
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=最小安装
CustomInstallation=自定义安装
NoUninstallWarningTitle=组件已经存在
NoUninstallWarning=安装程序发现以下组件已经安装在您的计算机上:%n%n%1%n%n取消选择将卸载它们.%n%n您还想继续吗？
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceMBLabel=当前选择需要不少于 [mb] MB 磁盘空间

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=选择附加任务
SelectTasksDesc=哪一个附加任务将被执行？
SelectTasksLabel2=选择当安装 [name] 完成时，您希望被执行的附加任务，然后点【下一步】

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=选择启动菜单目录
SelectStartMenuFolderDesc=添加快捷方式到程序菜单的哪里？
SelectStartMenuFolderLabel3=安装程序将在下面的启动菜单创建快捷方式
SelectStartMenuFolderBrowseLabel=点击【下一步】继续，如果您想选择不同的目录，点击【浏览】
NoIconsCheck=不创建任何图标(&D)
MustEnterGroupName=您必须输入一个目录名
GroupNameTooLong=目录名或路径太长
InvalidGroupName=目录名无效
BadGroupName=目录名不能包含以下字符:%n%n%1
NoProgramGroupCheck2=不能创建启动目录(&D)

; *** "Ready to Install" wizard page
WizardReady=准备好安装
ReadyLabel1=程序已经准备好安装 [name] 到你的计算机上
ReadyLabel2a=点击【安装】以继续安装,如果您想查看或修改设置，则点击【上一步】
ReadyLabel2b=点击【安装】以继续安装
ReadyMemoUserInfo=用户信息：
ReadyMemoDir=安装目录：
ReadyMemoType=安装类型：
ReadyMemoComponents=选择组件：
ReadyMemoGroup=开始菜单目录：
ReadyMemoTasks=附加任务：

; *** "Preparing to Install" wizard page
WizardPreparing=准备安装
PreparingDesc=安装程序正在为将 [name] 安装到您的计算机上做准备。
PreviousInstallNotCompleted=安装/卸载 原来的程序未完成。为完成安装，您需要重新启动计算机。%n%n在重启计算机后，运行安装程序完成安装[name]
CannotContinue=安装不能继续，点击【取消】退出

; *** "Installing" wizard page
WizardInstalling=安装
InstallingLabel=请等待，程序正在安装 [name] 到你的计算机上

; *** "Setup Completed" wizard page
FinishedHeadingLabel= [name] 安装向导完成
FinishedLabelNoIcons=安装程序已经将 [name] 安装到您的计算机上
FinishedLabel=安装程序已经将 [name] 安装到您的计算机上。您可以通过点击相应的图标以启动安装好的程序
ClickFinish=点击【完成】退出安装
FinishedRestartLabel= [name] 安装完成, 安装程序必须重启您的计算机，您想现在重启吗？
FinishedRestartMessage=为完成 [name] 的安装，安装程序必须重启您的计算机。%n%n您想现在重启吗？
ShowReadmeCheck=是的，我想查看 README 文件
YesRadio=是(&Y)，现在重新启动计算机
NoRadio=否(&N)，我将在以后重启计算机
; used for example as 'Run MyProg.exe'
RunEntryExec=运行 %1
; used for example as 'View Readme.txt'
RunEntryShellExec=查看 %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=安装程序需要下一张盘
SelectDiskLabel2=请插入盘 %1 然后点击【确定】。%n%n如果文件在其他目录，请输入正确的路径，或点击【浏览】
PathLabel=路径(&)：
FileNotInDir2=文件 "%1" 没有在目录 "%2" 中被找到。请插入正确的盘或选择其他目录
SelectDirectoryLabel=请指定下一张盘的位置

; *** Installation phase messages
SetupAborted=安装未完成。%n%n请解决问题后再安装
EntryAbortRetryIgnore=点击【重试】重试，【忽略】继续，或【终止】以取消安装

; *** Installation status messages
StatusCreateDirs=创建目录...
StatusExtractFiles=解压文件...
StatusCreateIcons=创建快捷方式...
StatusCreateIniEntries=创建 INI 条目...
StatusCreateRegistryEntries=创建注册表条目...
StatusRegisterFiles=注册文件...
StatusSavingUninstall=保存卸载信息...
StatusRunProgram=完成安装...
StatusRollback=回滚修改...

; *** Misc. errors
ErrorInternal2=内部错误：%1
ErrorFunctionFailedNoCode=%1 失败
ErrorFunctionFailed=%1 失败；代码 %2
ErrorFunctionFailedWithMessage=%1 失败；代码 %2.%n%3
ErrorExecutingProgram=不能执行文件：%n%1

; *** Registry errors
ErrorRegOpenKey=打开下面注册表键出错：%n%1\%2
ErrorRegCreateKey=创建下面注册表键出错：%n%1\%2
ErrorRegWriteKey=写下面注册表键出错：%n%1\%2

; *** INI errors
ErrorIniEntry=在文件"%1"中创建INI条目出错

; *** File copying errors
FileAbortRetryIgnore=点击【重试】重试，【忽略】忽略该文件(不建议)，或【终止】以取消安装
FileAbortRetryIgnore2=点击【重试】重试，【忽略】继续(不建议)，或【终止】以取消安装
SourceIsCorrupted=源文件损坏
SourceDoesntExist=源文件 "%1" 不存在
ExistingFileReadOnly=文件已存在且被标识为只读属性.%n%n点击【重试】移除只读属性后重试，或【终止】以取消安装
ErrorReadingExistingDest=尝试读已存在文件时发生一错误：
FileExists=文件已经存在。%n%n你想覆盖它吗？
ExistingFileNewer=已经存在的文件比要安装文件新，建议您保留现有文件。%n%n您想保留现有的文件吗？
ErrorChangingAttr=修改存在文件的属性时发生错误：
ErrorCreatingTemp=在目标路径建立新文件时发生错误：
ErrorReadingSource=读源文件时发生错误：
ErrorCopying=拷贝文件时发生错误：
ErrorReplacingExistingFile=替换存在文件时发生错误：
ErrorRestartReplace=重启替换失败：
ErrorRenamingTemp=在目标目录下修改文件名时发生错误：
ErrorRegisterServer=不能注册 DLL/OCX: %1
ErrorRegisterServerMissingExport=DllRegisterServer 导出没有发现
ErrorRegisterTypeLib=不能注册类库：%1

; *** Post-installation errors
ErrorOpeningReadme=打开 README 文件时发生错误
ErrorRestartingComputer=安装程序不能重启计算机，请手工重启

; *** Uninstaller messages
UninstallNotFound=文件 "%1" 不存在，不能卸载
UninstallOpenError=文件 "%1" 不能被打开，不能卸载
UninstallUnsupportedVer=卸载记录文件 "%1"格式与当前卸载程序不符，不能卸载
UninstallUnknownEntry=在卸载记录文件中遇到未知条目 (%1)
ConfirmUninstall=您确实想完全移除 %1 和它的组件吗？
OnlyAdminCanUninstall=仅当用户有系统管理员的权限时，才能卸载该程序
UninstallStatusLabel=正在从您的计算机中移除 %1 ，请等待
UninstalledAll=%1 被安全的从您的计算机中移除
UninstalledMost=%1 卸载完成，%n%n部分资料不能被删除，请手工删除
UninstalledAndNeedsRestart=为完成 %1 的安装，必须重启您的计算机。%n%n您想现在重启吗？
UninstallDataCorrupted="%1" 文件损坏，不能卸载

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=移除共享文件？
ConfirmDeleteSharedFile2=以下共享文件被系统标示为不再被其他程序使用，%n%n如果任何程序依然需要使用这些文件而它们被移除，这些程序将有可能不能正常工作。如果您不确定，请选择【否】，保留这些文件在您的系统里不会引起任何伤害
SharedFileNameLabel=文件名:
SharedFileLocationLabel=位置:
WizardUninstalling=卸载状态
StatusUninstalling=卸载 %1...

; The custom messages below aren't used by Setup itself, but if you make
; use of them in your scripts, you'll want to translate them.

[CustomMessages]

NameAndVersion=%1 版本 %2
AdditionalIcons=附加图标：
CreateDesktopIcon=创建桌面图标
CreateQuickLaunchIcon=创建快速启动图标(&Q)
ProgramOnTheWeb=%1 在 Web
UninstallProgram=卸载 %1
LaunchProgram=启动 %1
AssocFileExtension=关联 %1 到 %2 文件扩展名(&A)
AssocingFileExtension=关联 %1 到 %2 文件扩展名...
