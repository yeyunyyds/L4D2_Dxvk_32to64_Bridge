# Steam 中文与 ReShade 6.8 输入实验

从 LDB main `cafc0442446c502a0de0ebb52d5e73ccaae67516` / v1.3.2 建立 `codex/steam-ime-reshade68`。只参考并适配 [NPCodex Nightlybuild `b1fa0a1`](https://github.com/NPCodex/L4D2_Dxvk_32to64_Bridge-Nightlybuild/commit/b1fa0a1d6ba9f2e2c23c92e4da13bf324e883cc8) 的 Steam 中文/组合键与 ReShade 6.8 输入修复，未合并 main 或发布 Release/Tag。

## 生产改动

仅改变累计补丁生成的 `bridge/src/server/overlay_presenter.h`；原有 `steam_ime_native.h` 与参考源相同，继续复用。Client、Steam Renderer 身份校验、IPC v2、默认 C8、Shadow/VA、上传、GC、后端、配置和 updater 不变。

- Steam 原生捕获期间保留系统组合键及完整抬键，不再把 Win+Space 的 Space 单独转发；焦点过渡期间也保留原生队列输入。
- 保留同线程同进程子窗口和临时 IME 窗口的焦点。复用原 250 ms 窗口定时器，在捕获期间有条件恢复丢失焦点；没有新增扫描、每帧任务或计时器。
- 在 Steam 原生捕获的 `WM_IME_COMPOSITION/GCS_RESULTSTR` 边界阻止默认过程再次提交结果。保留其他合成消息，不按文字或时间去重。该机制参考已审阅 Steam Renderer 11.12.30.7；其他版本仍需实机核对。
- 已绑定 ReShade 的捕获取得 Presenter 原生焦点，避免 ReShade 6.8 包装低级键盘钩子后跳过 Bridge 回调，使关闭菜单的 Home 丢失。原生路径不再重复转发按键；未绑定 ReShade 的 Home 兜底保持原转发方式。
- Steam/ReShade 捕获源分别记录；来源变化即使未改变聚合捕获，也刷新焦点模式。最后一个原生捕获结束后恢复游戏焦点。

只增加少量 Presenter 元数据（组合键状态 256 字节、ReShade 捕获原子标记、受限诊断计数）。没有新同步 RPC、内存扫描或渲染命令开销；输入事件和既有窗口定时器的额外判断仍有成本，未测量 FPS，不能声称零开销。结果诊断默认关闭、最多 32 条且不记录文本。

## 验证

执行 `scripts/test_overlay_input_fixes.ps1`，固定 MSVC 14.29，分别运行 x86/x64：

- 提取实际 Presenter 键盘路由，覆盖 Win+Space、提前释放修饰键、注入事件放行、前后台、子窗口、捕获与关闭边界。
- 提取实际窗口过程，覆盖结果只走一次默认处理边界、相同文字连续提交、非 Steam 路径与诊断关闭/异常。该模型不执行 Steam 实际文字提交。
- 从固定 ReShade 6.8.0 `18deaa52de0c425a78b329e9cb3c497281cd00ec` 下载并 SHA256 核验三份源码，提取实际输入/钩子包装/菜单快捷键函数；验证 Home、活动控件、快捷键忽略、长按和未绑定兜底。
- 三类测试均使用 frozen main 的未修改 Presenter 作对照。旧实现须在指定断言失败，不能以编译失败代替。
- 更新既有跨进程真实 Presenter HWND 测试，覆盖两种混合位数、原生焦点、Steam/ReShade 交接、输入子窗口、临时 IME、定时恢复和退出。保留原 IPC、资源生命周期、Steam/IME 及诊断回归和完整三端 Release 编译。

[Windows CI 38958100923](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38958100923) 全部通过。编译源码 `138e7ca982173cf627d5dd9ce30e949cfe5e44d4`，共同构建 `l4d2-1.3.2+25947f0275b38ed8`，MSVC 14.29 Release `/O2 /DNDEBUG`，IPC ABI `0x00020004`、C8、mapped Shadow 默认关闭。

| 验证 | x86 / x64 结果 |
|---|---|
| 键盘路由 | 各 88 断言通过；精确旧源在 Win+Space 断言失败 |
| IME 提交边界 | 各 280 断言通过；精确旧源在默认重复处理边界失败 |
| ReShade 6.8 固定源码模型 | 各 7 个 Home/兜底场景、54 断言通过，额外两个组合键场景也通过；旧源每侧 5 个包装器场景在第二次 Home 精确失败，直接/兜底场景仍通过 |
| 真实 HWND 与混合位数 | x86 父进程→x64 Presenter、x64 父进程→x86 Presenter 均通过；覆盖焦点交接、子窗口、临时 IME、自然定时恢复与清理 |
| 既有回归与构建 | 360 组 IPC 配对、24 项 fail-closed、上传/回复/资源/Reset/GC/Steam/诊断检查及三端 Release 编译通过 |
| 独立构件核验 | CI ZIP digest、每文件哈希、PE 架构、共同身份、Host32 LAA 均核对；bin 仅三个桥文件 |

本地 48 项 Python 检查通过。首轮 Windows `38957828947` 被 Git 检出时转换许可证换行导致的固定字节校验阻断；新增该文件的 `-text` 属性后通过，未放松校验或改变生产修复。过时运行被取消；原始失败日志保留在 Actions。

机器可读摘要、原生结果/来源清单、编译收据与压缩 IPC 原始结果见 [results/input-fixes](../results/input-fixes)。完整 native artifact ID `12072162973`、SHA256 `735a7c0b5f66de855cb9161e432fe2d085055b9696f7752059ae871b93d8a35f`。原生模型和 NB 自己的游戏验收不等于 LDB 实机验收；本轮没有新增 FPS、真实中文提交或真实 ReShade 渲染器验收。

[下载已独立核验的实验三件套](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/26884d575f5932db5c6ae6d5ee613fadd8ff3cf2/ldb-input-fixes-v1.3.2-25947f02-r38958100923.zip)。ZIP SHA256：`75a191e5d8468aa40804bc23160e8654e4c1e87c0f2bad59ed29b265cc23cc69`。该下载与 CI 原构件字节一致；后续报告提交仅保存证据，不改变已编译源码。

## 实机安装与回退

退出游戏与全部 Host，备份并同时替换 `bin/dxvk_d3d9.dll`、`bin/.l4d2bridge/L4D2Bridge32.exe`、`bin/.l4d2bridge/L4D2Bridge64.exe`。沿用现有 conf、DXVK、ReShade、L4N 与 retention DB。不要混用三端。

ReShade 安装程序应选择 `bin/.l4d2bridge/L4D2Bridge64.exe`，API 选择 Vulkan。先确认现有 Presenter/Steam 输入选项保持原先启用值，本次不增加配置键。

检查 Steam 聊天和网页输入框的中文候选、重复确认同一词、Win+Space、中文状态 Shift+Tab、Alt+Tab；ReShade Home 多次开关、效果复选框和文本控件；Steam/ReShade 交接后游戏键鼠恢复。另检查地图切换、正常退出与帧时间。回退时退出并成套恢复三个备份。

## 来源

生产 Presenter 与测试来自 NB 的 MIT 源，保留其原 SPDX/版权及生成来源说明；适配位置在测试文件中标注。参考 [Steam 修复](https://github.com/NPCodex/L4D2_Dxvk_32to64_Bridge-Nightlybuild/blob/b1fa0a1d6ba9f2e2c23c92e4da13bf324e883cc8/docs/STEAM-IME-20261015.md) 与 [ReShade 修复](https://github.com/NPCodex/L4D2_Dxvk_32to64_Bridge-Nightlybuild/blob/b1fa0a1d6ba9f2e2c23c92e4da13bf324e883cc8/docs/RESHADE-INPUT-20261016.md)。ReShade 原始实现只在测试时按固定来源下载/提取，不编入运行时或玩家包，不把其版权归属改成本项目。
