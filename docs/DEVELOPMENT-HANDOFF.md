# 下一轮开发交接

本 PR 从当前 main 建立，交接已经合并和发布的工作。此提交只增加交接文档，不改变运行时代码、配置、ABI 或构件；VERSION 保持 `1.2.1`，下一开发版本号及开发范围由后续任务确定。

## 基线与源码位置

- main 基线：`cf49175e8a3c6c2fe45c99aec0680b0d1c864d24`，包含未修改插件不重新发布的构建/发布支持。
- 正式版 `v1.2.1`：`0d450b7707f6954d639a333e5258a1275f00fc63`，原 PR #4 已 squash 合并。
- [v1.2.1 Release](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/releases/tag/v1.2.1)。发布说明为诊断分离、默认关闭可选诊断及内存监测，修复内存扫描引起的掉帧，以及其他小 bug 修复。
- 当前运行时补丁：`patches/l4d2-bridge.patch`，SHA256 `8ce8005da14ea40a4a10d6d7be848be05ce8ac9da25676667fa67820178b87e3`；正式 build ID `l4d2-1.2.1+8ce8005da14ea40a`。
- `scripts/prepare_bridge.py` 准备固定上游源码并应用补丁。`.deps/dxvk-remix` 是被忽略的准备目录，不能以只修改该目录代替提交运行时补丁。
- README 按上一轮发布要求保持原样，仍有 v1.2.0 历史说明。最新发布事实以本交接、tag、Release 和构建记录为准；本 PR 不更新 README。

## 已完成的运行时工作

### v1.2.0 核心与常用设置

- x86 D3D9 Client 经 IPC 转发到 x86/x64 Host，再由 Host 加载 DXVK。完整包默认 x86 Host；x64 保留可选。
- PageBlock 统一 capability/safety/policy/residency/reclaim；支持 keep、推荐的 learned-aggressive 和实验 drop。LGC/AGC/FGC 是同一回收实现上的触发/扫描模式。
- Host readback recovery、资源/transfer pinning、顺序 ACK 与持久 KEEP 学习；扩展符合条件的 mip/Cube/Surface 和 Q8/ATI 恢复/传输兼容。
- 窗口/全屏黑屏相关 Reset、隐式资源引用平衡、Device 最终释放后成员访问修复；资源创建失败清理。
- Volume LockBox/UnlockBox 字节行距、层距及上传偏移修复，解决已确认的联机蓝绿偏色；该路径保持回归保护。
- 可选 ReShade Presenter/Input；文档验证组合仍是 x64 Host + Vulkan ReShade 6.0.1，不强制更改 Host。
- 可选 x86 L4N 常用设置插件及 Client 控制 ABI、安全配置持久化。
- 增加资源数据追踪、异常/API wait/color/data 诊断和分析器、契约回归测试。追踪不等于已经实现回收或优化。

### v1.2.1 诊断分离及修正

- Client/Host memoryMonitoring 默认 False：禁用观察扫描、周期采样任务及诊断日志；GC 不强制 whole-process 扫描，保留自身回收结果。
- crashDiagnostics 默认 False；API wait 的完整 threads 表、TLS API/command history、Data 每资源 metadata 改为按需创建；PageBlock nullable sidecar 保留。
- Core VB/IB allocation/free 自行完成，memory diagnostics 仅可选观察；诊断初始化、分配、日志和回调失败不改变 LG/GC/recovery 或资源生命周期。
- 关闭 API/command/Debug/Trace 日志后，相关 lazy producer 不构建诊断字符串；关闭诊断后不执行其采样、计时、线程/文件和统计更新入口。
- Presenter/Input 按配置启用，保留已启用功能；Bridge 不轮询 L4N，控制 ABI 由调用者触发。
- Client 不再安装无效的线程级 `WH_KEYBOARD_LL`（idHook=13）；保留合法线程 hook 与 Host Presenter 的前台过滤全局 hook。真实安装失败立即保留 Win32 错误，空句柄不卸载。
- DirectInput A/W 原参数及 HRESULT 不变；不再将成功的旧版本请求报为 Unsupported，真正失败仍记录请求版本与 HRESULT。
- Query AddRef/Release 删除遗留 missing-call 错误标记，引用计数、销毁和 IPC 行为不变。

逐路径审计与实现证据见 [审计](RUNTIME-DIAGNOSTICS-AUDIT.md) 和 [诊断分离报告](RUNTIME-DIAGNOSTICS-SEPARATION.md)。不能把已覆盖入口的 OFF 零工作结论扩大为所有 Host 路径都已清理。

## L4N、配置和 ABI 的现有边界

根菜单顺序保持 Status → GC → Memory Policy → ReShade Presenter → Host，不加入诊断项。

- Status 仅查询 PageBlock 信息，不显示 Host/ReShade。
- Memory Policy 的 keep/lg/drop 只改变 runtime；底部 `save to configure` 才确认实时 runtime 并保存。相同值显示 configure，不同值显示 runtime；drop 保持 experimental。
- Host 显示 configured 值，选择后点击 Save 才保存，完整重启后生效。x86/x64 都保持 `forceX64Server=True`，分别写 `client.testX86Server=True/False`。不运行时更换 Host。
- ReShade Enable/Disable 保存 Presenter/Input 配置并提示重启，不安装 ReShade，不重建 Device/Presenter 或杀 Host。
- 插件不读写 bridge.conf；Client 复用实际配置路径，逐键保留注释、未知键、Unicode、行序及换行，处理重复键，通过同目录临时文件、flush/close、原文件变化检测和替换保存，分别报告 runtime 与持久化结果。
- `L4D2BridgePageBlockControl` v1/v2 不变；独立 `L4D2BridgeControl` 提供通用配置控制。插件通过已加载模块查找导出，旧 Bridge 安全回退；Bridge 本体不依赖插件。

详细字段、返回值、菜单行为见 [API](API.md)、[L4N controls](L4N-BRIDGE-CONTROLS.md) 和 [配置](CONFIGURATION.md)。

## 未完成与已知限制

1. Steam Overlay/Shift+Tab 支持仍不完整，未在 v1.2.1 修复。
2. Host `bridge/src/server/swapchain_references.h` 的 `SurfaceQueries::finish()` 仍更新诊断 `count/mismatches`。这是已披露的清理剩余项。实际 Getter、IUnknown 身份校验及临时引用 Release 属于正确性工作，不能随诊断一起关闭。
3. 旧 `Failed to restore display mode` 提示可能仅因空 monitor 触发，不能单凭此日志判定发生了系统恢复失败或黑屏。尚未修改该提示路径。
4. VB/IB 继续 FULL_SHADOW；没有静态 reclaim、范围暂存、零拷贝、合并上传或状态去重。仅 static + WRITEONLY 不能证明每次 Lock 完整写入，不能据此丢弃旧字节。契约不确定则保留 FULL_SHADOW。
5. 未扩大 learned identity 范围、修改 KEEP DB 规则或处理 keep → LG 时既有资源 metadata 补建。
6. 其他未确认归属于 Bridge 的游戏崩溃不作为已修复或已知 Bridge bug；本项目不包含游戏内部越界修改。

## 验证证据与实机范围

[正式源码 Windows 构建 38008260919](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38008260919) 通过，源码为 `0d450b7`：x86 Client、x86/x64 Host 构建及相关回归成功，未重编译插件。

- 13 个独立诊断观察测试模式覆盖 OFF、启用正对照、初始化/分配/回调失败、TLS 退出边界及 LG 闭环；OFF 夹具中的 scan/clock/file/thread/hook/HWND/module/SRW lock/allocation 为零，无 API wait 表/history/Data metadata。
- Core VB/IB 分配、锁内容契约及 FULL_SHADOW 回归通过。
- 真正的 Runtime/Parent/Entry/DB 配合独立模拟 Host 字节源验证 DB miss → Evictable → reclaim → preserve miss → Host recovery → KEEP promotion → persist → 新 Runtime DB hit；retention log 创建/写入/回调失败不触发 fallback KEEP。
- GC 在 monitoring OFF 时仍回收，保留必要 mapped-view 查询，不执行 whole-process 诊断扫描。
- 原 Volume 测试保持不变，x86/x64 各 59 项通过；Reset、隐式引用平衡、PageBlock、readback、IPC/队列、Host、适配器/ATI、Presenter 等回归通过。插件 DLL/callback 构建夹具因插件未修改而跳过，控制 ABI/设置测试仍执行。
- 本地 Python 测试共 38 项通过；[发布校验 38009011657](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38009011657) 通过，正式附件公开下载的长度与 SHA256 已核验。

最近 x86 实机日志使用同一补丁的 `l4d2-1.2.0+8ce8005da14ea40a` 实验构件，约 123 秒：Client/Host 无 warn/err，AGC 卸载 1195 项、VA 约 43.51 MiB、backing 约 39.59 MiB，正常退出。此轮没有 Reset 记录，不能据此宣称验证了切屏；也未覆盖 x64/FGC。之前用户测试涵盖 x86 进图、GC/插件、全屏窗口、c2m3→c2m4 和 ReShade 开关。日志正常不能代替用户对画面与输入的确认；自动模拟后端测试也不能代替真实 GPU 验证。

## 后续开发、构建和分发

- 先明确下一任务范围和版本号，再修改实现。本交接不安排新优化算法。
- Windows 使用仓库固定工具链和 `build.yml`。本新分支不在现有 push 自动构建名单内，需要手动 workflow_dispatch。
- 本地 Python：`python -m unittest discover -s tests -p 'test_*.py'`；原生套件入口和条件见 [TESTING](TESTING.md)。修改相关路径后运行对应回归，Volume 修改必须保留原布局套件。
- 未修改的组件不重新编译或打 ZIP；Bridge-only 构建传 `build_l4n=false`。插件改变时才构建，正式 Release 插件独立 ZIP，不嵌入本体/patch。
- 只发布成功构建且核验来源的构件，tag 指向实际构建源码。发布 workflow `publish-release.yml` 使用成功的 `build_run_id` 和版本；插件未修改时传 `include_l4n=false`，只发布本体、匹配 patch 和 SHA256SUMS。
- ZIP 专用分支是 `experimental-downloads-1.1.2-dev.2`，保留历史包；不要用源码分支或新 PR 替代该下载分支，也不要覆盖已发布历史构件。
- 本次交接不构建、不上传新 ZIP、不创建新 Release。

## 必须保留的开发约束

Core owns correctness/state → Diagnostics optionally observe Core。诊断失败不得改变核心行为；真正 Core DB、分配、同步和恢复失败仍执行安全处理。

PageBlock、learned-aggressive/keep/drop、GC、recovery、IPC 顺序、Reset、资源生命周期、Host 选择和 D3D9 契约不得因清理诊断而改变。保留必要 pin/ACK/timeout、核心身份指纹与持久 DB、GC 本身结果统计。Volume 字节布局不进行泛化重构。Bridge 职责限制在自身渲染/转发与相关正确性，不修改 Source Engine/studiorender 等游戏内部行为。

架构入口：[ARCHITECTURE](ARCHITECTURE.md)；接口入口：[API](API.md)；回收语义：[PAGEBLOCK-DROP-GC](PAGEBLOCK-DROP-GC.md)。交接文档记录已完成状态和明确边界，后续任务不得将未完成项自动解释为已授权开发范围。
