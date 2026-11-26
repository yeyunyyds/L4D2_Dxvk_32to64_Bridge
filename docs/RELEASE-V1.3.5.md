# 1.3.5 安装与回滚

[正式版下载](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/releases/tag/v1.3.5) · [更新说明](RELEASE-V1.3.5-NOTES.md) · [配置参考](CONFIGURATION.md)

## 安装

完全退出游戏与全部 Bridge Host，备份 `bin/dxvk_d3d9.dll`、`bin/.l4d2bridge/`、已有 L4N 插件及配置。

- 首次安装：使用 `l4d2-bridge-v1.3.5.zip`，将 `bin` 合入游戏根目录，包含 x86 Client、x86/x64 Host、官方 DXVK 2.6.1 双架构后端及默认配置。
- 已有安装：优先使用 `l4d2-bridge-patch-v1.3.5.zip`，只更新三个 Bridge 二进制，保留运行配置、DXVK、ReShade 和 retention DB。
- 可选插件：`l4d2-bridge-l4n-v1.3.5.zip`，安装到 `bin/neko/plugins/`；不包含 L4N 本体，插件功能不变。
- 使用 `SHA256SUMS.txt` 核对附件，包内 `BUILD-INFO.json` 记录配套构建标识与哈希。

三个 Bridge 二进制必须同批更新。通过 Steam 使用原有启动参数，不直接启动 Host EXE。

## LG 与诊断

默认策略仍为 `learned-aggressive`，现有 V1 retention DB 可继续使用，无需删除或迁移。已知 KEEP 不重复写入，新学习仍同步持久化并验证追加记录。首次需要恢复纹理时仍可能短暂等待。诊断开关保持原有默认值（详细诊断关闭）；游戏根目录 `L4D2BridgeLogs/<session-guid>/` 保存同一会话的常用日志。

## 回滚

完全退出游戏与全部 Host，恢复同批备份的 Client 和两个 Host；若覆盖配置、后端或插件，同时恢复对应备份。数据库格式未改变；必要时可恢复备份。不要混用不同版本三件套。
