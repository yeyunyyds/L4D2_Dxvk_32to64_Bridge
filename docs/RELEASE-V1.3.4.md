# 1.3.4 安装与回滚

[正式版下载](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/releases/tag/v1.3.4) · [更新说明](RELEASE-V1.3.4-NOTES.md) · [配置参考](CONFIGURATION.md)

## 安装

完全退出游戏与全部 Bridge Host，备份 `bin/dxvk_d3d9.dll`、`bin/.l4d2bridge/`、已有 L4N 插件及配置。

- 首次安装：使用 `l4d2-bridge-v1.3.4.zip`，将 `bin` 合入游戏根目录。包含 x86 Client、x86/x64 Host、官方 DXVK 2.6.1 双架构后端及默认配置。
- 已有安装：优先使用 `l4d2-bridge-patch-v1.3.4.zip`。它只替换三个 Bridge 二进制，保留运行配置、DXVK、ReShade 和 retention DB。
- 可选插件：`l4d2-bridge-l4n-v1.3.4.zip`，安装到 `bin/neko/plugins/`；不包含 L4N 本体。
- 使用 Release 附件 `SHA256SUMS.txt` 校验下载；包内 `BUILD-INFO.json` 记录三个配套二进制的准确构建标识与哈希。

三个 Bridge 二进制必须同批更新。从 Steam 使用原有 `-vulkan -insecure -windowed` 启动，不要直接启动 Host EXE。

## 配置与日志

升级包保留现有 `bin/.l4d2bridge/bridge.conf`。新增选项省略时采用安全默认值：`server.blockHostTcp443=False`、`client.studioModelDiagnostics=False`、`logging.sessionRetention=10`。修改后完整退出游戏及 Host 再启动。

常用日志位于游戏根目录 `L4D2BridgeLogs/<session-guid>/`。异常时保留完整 Session 目录；错误窗口显示的模块调用链是定位线索，不等于已证明的根因。

L4N 插件将菜单位置和语言写入插件同目录的 `L4D2BridgePlugin.ini`。删除该文件可恢复默认位置与 English。

## 回滚

完全退出游戏与全部 Host，恢复之前备份的 Client 和两个 Host；若使用完整包覆盖了配置或 DXVK，同时恢复对应备份。L4N 插件可独立恢复或删除，不要混用不同版本三件套。
