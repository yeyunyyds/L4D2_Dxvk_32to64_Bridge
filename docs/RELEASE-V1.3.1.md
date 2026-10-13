# 1.3.1 候选包 / Release candidate

本包用于发布前实机确认，尚未创建 Tag 或 Release。正式版仍为 1.3.0。

## 安装与回滚

退出游戏并确认两个 Host 进程均已退出。备份现有 bin/dxvk_d3d9.dll、
bin/.l4d2bridge、bin/neko/plugins/L4D2BridgePlugin.dll 及运行配置。

新安装使用完整包，将 bin 合入游戏目录；包含 x86 Client、x86/x64 Host、
DXVK 2.6.1 双架构后端和默认配置。已有安装优先使用 matched-update 三件套包，
只替换配套 Client 与两个 Host，保留后端、配置、ReShade 和 retention DB。
L4N 为单独可选插件包，将其中 bin/neko/plugins 合入游戏目录；不会安装 L4N 本体。
不要混用不同 build ID 的桥二进制。需要回滚时完整恢复备份并重新启动游戏。

已有配置不会自动更新。Steam 修复需合入 config/STEAM-OVERLAY-FIX.conf 并完整重启；
server.presenterWindow=True 是 Steam 输入依赖，server.presenterInput=True，
server.presenterNativeSteamInput=True；其它窗口适配键见该 snippet。
保持 client.hookMessagePump=False、client.overrideCustomWinHooks=False。
新安装默认 x64 Host、C8；六档 C1/C2/C4/C8/C16/C32 保留。
Mapped Shadow 默认关闭，原有 arena、资源管理和 IPC 正确性修复保留。

## 验收范围

请确认启动、进图及连续切图、Shift+Tab 开关、鼠标键盘与关闭、Join Server、
View Workshop，以及房间邀请好友界面能操作并关闭。使用 ReShade 时同时验证 Home
与 Steam Overlay 交替开关。L4N 的 ReShade / Steam Overlay 开关控制共享 Presenter
依赖，关闭会使 Steam 输入失效；不安装 ReShade 本体。

前一 Steam-only 实验已有用户实机通过记录；该记录不自动代表此完整 main 组合或
新版本候选已通过。Windows 原生回归和候选实机结果分别记录，不宣称 FPS 改善。
在用户确认之前不发布 1.3.1 Release。
