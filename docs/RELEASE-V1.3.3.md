# 1.3.3 安装与回滚

[正式版下载](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/releases/tag/v1.3.3) · [更新说明](RELEASE-V1.3.3-NOTES.md) · [Steam 11.15 适配](STEAM-RENDERER-11.15.70.27.md) · [ReShade 6.8 输入](RESHADE-6.8-INPUT.md)

## 安装

完全退出游戏与全部 Bridge Host，备份 `bin/dxvk_d3d9.dll`、`bin/.l4d2bridge/`、已有 L4N 插件及配置。

- 首次安装：使用 `l4d2-bridge-v1.3.3.zip`，将 `bin` 合入游戏根目录。包含 x86 Client、x86/x64 Host、官方 DXVK 2.6.1 双架构后端及默认配置。
- 已有安装：优先使用 `l4d2-bridge-patch-v1.3.3.zip`。它只替换三个 Bridge 二进制，保留运行配置、DXVK、ReShade 和 retention DB。
- 可选插件：`l4d2-bridge-l4n-v1.3.3.zip`，安装到 `bin/neko/plugins/`；不包含 L4N 本体。
- 使用 Release 附件 `SHA256SUMS.txt` 校验下载；包内保留二进制哈希及 `BUILD-INFO.json`。

三个 Bridge 二进制必须同批更新，构建标识为 `l4d2-1.3.3+05a6b510fec2a797`。从 Steam 使用原有 `-vulkan -insecure -windowed` 启动，不要直接启动 Host EXE。

## 配置与验证

升级包保留现有 `bin/.l4d2bridge/bridge.conf`。Steam 与 Presenter 配置继续沿用 [Steam 修复片段](../config/STEAM-OVERLAY-FIX.conf) 和 [Overlay 输入片段](../config/OVERLAY-INPUT.conf)，修改后完整退出游戏及 Host 再启动。

本次静态核对用户提供的 Steam Renderer 11.15.70.27 x86/x64 文件身份、getter 指令与映像边界；代码不会加载或重新分发这些专有 DLL。Windows 原生测试覆盖两种位数的 getter 解码、未知版本拒绝、ReShade Presenter 焦点获取、捕获来源交接和恢复。

原生测试不能替代所有 Steam、ReShade、GPU、地图与 MOD 组合。更新后建议检查启动、进图、Shift+Tab、好友邀请、Workshop、中文输入、ReShade Home/复选框、Steam/ReShade 交替开关、切图及正常退出。

## 回滚

完全退出游戏与全部 Host，恢复之前备份的 Client 和两个 Host；若使用完整包覆盖了配置或 DXVK，同时恢复对应备份。不要混用不同版本三件套。
