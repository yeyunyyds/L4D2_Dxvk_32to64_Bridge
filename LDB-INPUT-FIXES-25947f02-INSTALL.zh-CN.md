# LDB Steam 中文 / ReShade 6.8 实机测试

实验分支 `codex/steam-ime-reshade68`，基于 v1.3.2；构建 `l4d2-1.3.2+25947f0275b38ed8`。main、Release 与 Tag 不变。本包没有 updater、Unicode 路径或 NVIDIA 初始化更新。

完全退出游戏及所有 Host，备份下面三个文件；解压后只把包内 bin 合并到游戏根目录：

- `bin/dxvk_d3d9.dll`
- `bin/.l4d2bridge/L4D2Bridge32.exe`
- `bin/.l4d2bridge/L4D2Bridge64.exe`

三件套必须配套。保留 bridge.conf、DXVK、ReShade、L4N 和 retention DB；包内顶层 config 是参考文档目录，不要拿它覆盖现有配置。

沿用当前已启用的 Presenter/Steam 输入设置；无新配置键。ReShade 6.8 应安装到 `bin/.l4d2bridge/L4D2Bridge64.exe`，API 选 Vulkan。

重点验收：Steam 聊天与网页输入框的中文候选、连续确认相同词语、Win+Space、中文状态 Shift+Tab；ReShade Home 连续开关、效果复选框、文本控件；两个 Overlay 交替捕获、Alt+Tab 后游戏键鼠恢复。再检查正常进图、切图和退出。

原生测试不替代真实 Steam/ReShade/L4D2 验收。本轮不宣称 FPS 改善。若异常，完全退出后成套恢复原三件套。

[全部通过的 Windows CI](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38958100923) · [Draft PR #14](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/pull/14)

编译源码：`138e7ca982173cf627d5dd9ce30e949cfe5e44d4`。三端 MSVC 14.29 Release /O2 NDEBUG。x86/x64 每侧键盘 88、IME 结果 280、Home 7 场景/54 断言及额外两个组合键场景通过；原始代码负对照有效失败。两种混合位数真实窗口、360 组 IPC 与 24 项故障及其他既有回归通过。

[下载 ZIP](./ldb-input-fixes-v1.3.2-25947f02-r38958100923.zip)

ZIP SHA256：`75a191e5d8468aa40804bc23160e8654e4c1e87c0f2bad59ed29b265cc23cc69`。
