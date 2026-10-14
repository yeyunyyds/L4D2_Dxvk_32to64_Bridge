# 1.3.2 安装与回滚

[正式版下载](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/releases/tag/v1.3.2) · [更新说明](RELEASE-V1.3.2-NOTES.md) · [正确性修补证据](LDB-CORRECTNESS-AUDIT.md)

## 安装

完全退出游戏与全部 Bridge Host，备份 `bin/dxvk_d3d9.dll`、`bin/.l4d2bridge/`、已有 L4N 插件及配置。

- 首次安装：使用 `l4d2-bridge-v1.3.2.zip`，将 `bin` 合入游戏根目录。包含 x86 Client、x86/x64 Host、官方 DXVK 2.6.1 双架构后端及默认配置。
- 已有安装：优先使用 `l4d2-bridge-patch-v1.3.2.zip`。它只替换三个 Bridge 二进制，保留运行配置、DXVK、ReShade 和 retention DB。
- 可选插件：`l4d2-bridge-l4n-v1.3.2.zip`，安装到 `bin/neko/plugins/`；不包含 L4N 本体。
- 使用 Release 附件 `SHA256SUMS.txt` 校验下载；包内保留二进制哈希及 `BUILD-INFO.json`。

三个 Bridge 二进制必须同批更新，构建标识为 `l4d2-1.3.2+3d545d8aa5ca7164`。本次 SharedHeap 内部报文增加长度字段，旧 Client/Host 不可混用；启动时会拒绝不同构建标识。

从 Steam 启动游戏，启动选项沿用 `-vulkan -insecure -windowed`；不要直接启动 Host EXE。

## Steam 与中文输入配置

运行配置为 `bin/.l4d2bridge/bridge.conf`。升级包保留旧配置；如缺少相关设置，合入 `config/STEAM-OVERLAY-FIX.conf`，每个键保留一个有效定义，然后完整重启游戏及 Host。

```ini
server.presenterWindow = True
server.presenterInput = True
server.steamNativeInput = True
server.steamImeTsfActivate = True
server.steamImeNativeWarmup = True
client.hookMessagePump = False
client.overrideCustomWinHooks = False
```

Steam 输入依赖 Presenter 子窗口。L4N 的 ReShade / Steam Overlay 开关控制此共享依赖，关闭会使 Steam 输入失效；无需安装 ReShade 即可使用 Steam 修复。

新安装默认 x64 Host、C8；C1/C2/C4/C8/C16/C32 均保留。SharedHeap 和 mapped Shadow 默认关闭，详细诊断默认关闭。已有配置显式值继续生效。

## 验证范围

中文输入法初始化此前已有用户实机通过记录，main 已有的新版 Steam Renderer 修复一并保留。Query、Shader、UpdateTexture、SharedHeap、Host 修补逐项通过 Windows 原生正反向及故障测试；发布版本还须通过全量回归与三架构构建、打包校验。

这些原生测试不替代 L4D2/GPU 测试。本次正确性组合没有新增实机性能或连续切图结果，不宣称 FPS 改善。用户已明确授权正式发布，历史实验测试记录保持原样。

建议更新后检查启动、进图及连续切图、资源上传、Shift+Tab、邀请界面、中文输入、Workshop 和正常退出；使用 ReShade 时同时检查 Home 与 Steam Overlay 交替开关。

## 回滚

完全退出游戏与全部 Host，恢复之前备份的 Client 和两个 Host；若使用完整包覆盖了配置或 DXVK，同时恢复对应备份。不要混用不同版本三件套。重新启动后检查日志中的 build ID。
