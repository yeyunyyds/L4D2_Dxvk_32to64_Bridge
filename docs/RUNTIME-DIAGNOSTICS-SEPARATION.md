# Runtime / diagnostics separation

本次基于 main/v1.2.0 `47c4da5403e3b8f1b1beca0ffa35474be3027394` 清理。实现前已完成 [逐路径分类审计](RUNTIME-DIAGNOSTICS-AUDIT.md)。本次不引入优化算法、修改渲染输出、资源生命周期、Host 选择、Reset 或 IPC 顺序。L4N 源码、菜单和控制 ABI 均未修改；不重编译/打包未修改插件。已发布 v1.2.0 构件保持原样，本文记录新的源码行为。

## 实现结果

| 子系统 | 关闭后的工作 | 开启时 |
|---|---|---|
| Client memoryMonitoring | 在时钟/锁/扫描/文件前返回；无 VA/进程采样、memory atomics/峰值、文件或异步任务。GC 的 forced sample 也不能绕过开关。 | 原 Present/分配驱动的约 5 秒采样与事件快照。 |
| Host memoryMonitoring | 不访问/构造 Recorder，不配置 GPU 观察，不建立 diagnostic inventory，不更新队列 observation atomics 或 processed-command 总数；无周期扫描/内存文件。 | 原 VA、CPU/commit/GPU budget、resource/queue 日志。 |
| Crash history | CallScope/CommandScope/recordCommand 在 TLS/history 前返回；无命令 ring 或 API context 更新、可选退出队列 dump。 | 原上下文与 ring，basic/detailed 报告保留。 |
| API wait | 既有 gate 保留；无 diagnostic QPC/TLS stats/atomic/histogram/线程/文件。 | 原阶段计时与周期输出，格式和控制流不变。 |
| Color | 既有 producer gate 保留；无 payload 哈希、diagnostic Getter/COM refs/状态记录、线程或日志。 | 原纹理/状态摘要和 backend 快照。 |
| Data / VB / IB | 既有 detail/histogram/状态比较 gate 保留；无 detail 分配、载荷比较、timing/counters、线程/文件。另将 memory total/Trace buffer total 归到明确开关。 | 原数据追踪/直方图；FULL_SHADOW 和锁契约不变。 |
| PageBlock / learned details | 无资源诊断记录、历史/模拟 LRU、learned summary 计数和日志 formatting/file；纯 recovery profiling 计时关闭。 | 详细输出继续存在；选定策略的 DB/identity/恢复正确性独立于日志开关。 |
| Presenter / Input | 全部相关功能关闭时，不创建 Presenter window/thread/hook，不进入可选 input detour/DirectInput setup，不做 ReShade 模块扫描/binding/capture；输入消息保留核心窗口/session 通知。 | 原按需窗口/输入路由及 ReShade addon 协作。Input=False 的呈现窗口不初始化其输入。 |
| Steam input | 实验与诊断均关时无 callback/module polling、输入状态观察或日志。单独开启实验时不因 diagnostic=False 停止真实状态转发。 | 观察日志与实验状态转发分别启用；没有新增 Steam Overlay 修复。 |
| API / command / Debug / Trace logging | API 名使用 const char*，日志计数 map 延后至真正启用；server command 字符串原有 flag gate 保留；Debug/Trace 用 lazy producer，关闭后不求值、不格式化或分配字符串。 | 保留原输出。初始化前的一次性日志缓存不改。 |

新开关 `client/server.memoryMonitoring=False`、`client/server.crashDiagnostics=False` 均启动时读取，随包显式关闭。新 crashDiagnostics 缺失时，旧 Detailed=True 仍请求 history；显式 False 优先。PageBlock diagnostics 不自动开启 memory monitoring；`PB_POLICY_SUMMARY memory_monitoring=0` 时 global current/peak backing 观察字段为未采集的零值，不能解释为没有 backing。核心 control Stats 从真实 entries 查询，不受该零值影响。

## 保留的核心与检查

关闭后的入口仍有一次 bool/atomic enable 检查；部分调用点保留空 stack token 和必要 wire/layout 局部量。静态固定表/ring 的存储仍编译在二进制内，但不更新。没有建立新诊断构建产品；Tracy 继续默认 compile-time 关闭。某些入口第一次调用会读取一次 Config 并缓存启动结果，没有周期 Config/UI 轮询。

保留真实 PageBlock registry、capability/safety、view LRU/预算、locks/transfers/pins、resource map、command UID、ordered Host ACK 和恢复同步。drop/remap/reconstruction failure 的少量普通整数是已公开控制状态，用户触发 GC 的 scan/skip/byte/ACK/drain 结果计数和计时仍存在；没有 diagnostic memory/queue atomics。FGC 等待语义不变，不解除游戏持有的锁。

保留 learned 内容指纹、创建来源 fingerprint、KEEP DB 读取/学习写入和 recovery hash 验证；它们决定分类、安全或恢复结果，不能作为 profiling 删除。恢复命名用 QPC nonce 保留；纯 readback/rebuild timing 只在 PageBlock diagnostics/reference test 请求时运行。GC 的既有 ACK/drain 计时按 ABI 保留。真正 queue timeout 和 readback event-query deadline 的时钟/轮询继续执行。

保留窗口 WndProc/Set/GetWindowLong 协作、游戏窗口/session 活动通知，以维持窗口/Reset 与超时正确性；它们不是 disabled Presenter-specific input hooks。单独启用 forwarding/message-pump/exclusive-input/Remix 时仍执行明确请求的功能。省略旧 DirectInput policy 保持上游缺省 2，完整全关需显式 policy=0；随包已是该组合。

Fatal handler 的一次性准备、有界故障时寄存器/堆栈/模块报告、peer exit、flush 和终止语义保留；关闭 history 后不再有命令/API 归因信息，故障时仍可能做地址/模块查询及创建 crash report。普通 startup/build/handshake/Device/Reset/shutdown、用户 GC/配置动作和真实 warning/error 仍写普通日志，正常帧间不产生可选周期观察日志。

## 验证

`scripts/test_runtime_observation.ps1` 使用生产 helper 和从生产文件提取的 startup/input/logger 函数，测试对象中拦截 Win32，并统计分配；release 没有测试 hooks。独立 OFF 进程 1000 次调用要求诊断 scan/clock/file/thread/hook/HWND/module/SRW lock/allocation 全为 0，内存/queue/history 无更新；不可解引用 Device 哨兵用于证明没有 Getter。随后真实 PagefileShadow + AGC 仍 evict 1 / release 4096 bytes；其 1 次 VirtualQuery 是实际 mapped view，结果计时属于用户请求结果。monitor-on/input-on/crash-on/logging-on 为正对照；OFF 后不能有新诊断日志。

本地 MSVC 14.29 / SDK 10.0.22621 在 Wine 下已完成 x86 Client 与 x86/x64 Host 编译/链接，关闭测试首轮 x64 执行通过。最终源码的 Windows CI 与其余 core/enabled regression 结果在提交验证后补充。没有以 FPS 作为验收，也不声称 FPS 提升。

受保护源码哈希检查覆盖 Volume Client/Host UnlockBox、volume_layout、wire helpers、原 Volume 测试/脚本、Reset helper、Host bootstrap、控制 ABI 与插件；同时执行原 Volume 回归。native fixtures 和 fake backend 不代表实际 L4D2/DXVK 驱动已覆盖。

## 实机验证与后续边界

仍需真实 L4D2 + DXVK 在 x86/x64 Host 上复测联机过图、Reset/切窗、所选策略和三种 GC/恢复；开 Presenter/Input 后复测已验证 ReShade 组合（x64 + Vulkan ReShade 6.0.1）及实际键鼠。验证 OFF 运行没有新 diagnostics 文件，再逐项启用观察确认需要的输出。

本次未清理上游整个 Remix 产品，也不改 DXVK 自己的 monitoring/HUD。若以后裁剪 compile-time diagnostics 或进一步消除空 stack token/固定表，需另行验证，不以这些存储或核心观察为理由删除正确性行为。Steam Overlay 的既有不完整支持仍为已知限制。
