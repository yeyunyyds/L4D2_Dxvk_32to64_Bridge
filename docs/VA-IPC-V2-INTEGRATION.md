# VA Allocator 与 PR7 IPC v2 生产接入实验

## 结论与范围

独立分支：`codex/vbib-va-ipc-v2`。生产代码提交 `f09b44380f3e47d3651b8857216f59d0aacc6d2c`，实验版本 `1.2.2-dev.2`，构建身份 `l4d2-1.2.2-dev.2+c45ef456eb8dde36`。继承 VA 分支 `b90d056006f892765b0cc2ebfc895b406bda4725`，PR7 原型来源 `dbb7b2d91561ba220585c9af659d5d9a2ae5248b`。固定对照为 main `de74cd7d5dd546bfd4f03ab451d0565f1fff0107`（v1.2.2、PR6 修复路径）。**本次没有把这个对照称为 v1.2.1 原始 IPC。**

用户已授权生产接入与扩大验证。当前交付是 **四条生产命令通道使用 v2 核心、payload 保留 PR6 的混合实现**。游戏会实际执行新的命令路径，不是独立原型自测；但不等于 PR7 完整统一流、upload pool、Reply Slot 或 Host 主导资源架构。没有合并 main、创建正式 Release/Tag 或修改原 VA 分支。

**结论：生产接入和原生回归通过，可以提供实验包；性能门槛未达到，不建议作为默认替代或继续强行迁移 payload。** VA 的回收收益来自既有 VA 分配器，不能归功于 v2 IPC。活跃资源仍保持 FULL_SHADOW，不能解决活跃 Shadow 自身占用。完整统一 payload 的接入仍未完成，需要独立设计和验证，不能直接移除旧 ring/pins。

## 实际调用链与生产变更

`Client Command → PR6 完整数据 reservation/写入/提交 → v2 命令记录即时发布 → Host wait/peek/pull → 原 handler 解码 → PR6 end_read_data/pin/消费确认`。Device 与 Module 各有请求、回复，共四通道。反向回复沿相同适配器传输 Header。

每命令一个 64-byte payload block；记录由 24-byte Record 与 4-byte dataOffset 构成。保留 Header 的 command、flags、handle、dataOffset，原 UID 编解码及资源回包 handle 不变。每个 API 立即发布，不依赖下一 API 触发 flush，也不按大小选择不同协议。数据与 Header 仍分开发布；命令 completed **不代表** payload consumed 或 GPU 完成。

生产变更仅涉及 `bridge/src/util/ipc_v2/platform.h`、`ipc_v2/transport.h`、`util_ipcv2commandqueue.h`、`util_ipcchannel.h`、`util_bridgecommand.cpp`。`buffer_shadow.h`、`buffer_shadow_arena.h`、`lockable_buffer.h` 与继承的 VA 分支逐字节相同。没有改动态 VB/IB Lock/Unlock、上传语义、PageBlock、GC、DXVK 或 ReShade。

来源依据与定位：

- `util_ipcchannel.h`：默认生产命令队列选择适配器，原 payload Ring 控制和布局不变；IpcChannel 拥有对象改为 `unique_ptr`，保证构造失败回滚。
- `util_ipcv2commandqueue.h`：Header 编码/校验、原 queueSize−1 quota、共享发送/接收历史、Client 回复串行保护、四通道故障注册与有限状态缓存。
- `ipc_v2/transport.h`、`platform.h`：发布/消费游标、armed/recheck 事件唤醒、映射就绪、构建身份和冷路径等待/peer 检查。
- `util_bridgecommand.cpp`：session 故障传播、Header 取出后的健康检查；原 PR6 packet、失败归因和 payload pin 仍保留。
- `experiments/ipc-v2` 保留原 PR7 原型，不应把它的历史测试与本次适配代码的验证混为一谈。

## 正确性条件与成本

1. Client 在创建 Host 前初始化通道：first opener 初始化映射，另一端校验 ready、协议和 Config。新协议版本 `0x00020003`；四个构建身份散列来源于双方启动前设置的 BRIDGE_VERSION。不同 v2 构建拒绝连接。PR6/v2 混装不支持，旧端可能表现为有界握手失败，并非保证立即报版本不匹配。
2. 保持原 usable quota，不增加预算掩盖性能或满载问题。原 256-byte 命令元数据区域复用为跨端发送/接收计数和历史，没有增加独立历史映射。
3. Client Reply 的 peek/copy/pull 使用局部 SRW 锁，防止多个 UID waiter 中一个释放槽位、另一个继续读该槽。Host 单 dispatch 不增加此锁。空/满等待用冷路径 timed mutex 保证单一事件 waiter，正常就绪分支不使用该 mutex。
4. 发布/消费保持原子同步，满载 armed 后再次检查，避免覆盖和丢失唤醒。try_push 遇满可重试；等待失败、终止或协议损坏导致 sticky session fault、停止后续提交，并唤醒四方向通道。
5. 首次解析校验 sequence、窗口、记录数量、bytes/bodyBytes、generation/uid。之后缓存 **已复制的 Header 值**，只在 completed 不变时复用；pull 仍进行一次消费确认。生产 dispatch 始终额外检查通道健康，故障后允许队列诊断读取不意味着允许游戏继续处理。
6. 冷路径缓存首次取得的 peer process HANDLE，之后不会用重用的 PID 代替旧进程；首次取得句柄仍依赖 PID 和既有进程退出处理，不能宣称已解决所有启动期 PID 竞态。
7. IpcChannel 构造异常释放所有已拥有的映射/句柄。Reset、资源销毁、长 Lock 和 payload 生命周期仍遵循 PR6。没有引入新的资源 generation 跟踪或独立 Host 状态镜像。

额外运行时状态：每通道约一个 Header/sequence/ready 缓存（约 20 bytes，四通道约 80 bytes）、局部同步对象与必要通道元数据；session 固定 32 个注册槽，仅初始化/退出/故障使用锁。新命令映射 `384 + nextPowerOfTwo(queueSize)×128` bytes，原映射保留以保持 payload capacity/offset；实测四通道额外 mapped VA 为 **0.515625 MiB**。这不是物理 RAM 指标，也不是 IPC VA 节省。

## 可重复验证方法

Windows 2022，MSVC 14.29 `/O2`、caller/Command 独立 TU、无 LTO。x86 Client 配 x86 与 x64 Host，计时在批次外层。失败样本保留，任一完整性失败终止运行。没有为候选增加队列预算、更改对照配置或削弱错误处理。

- `test_ipc_transport.ps1`：生产源码提取，原 packet/known/generic、跨进程、并发 UID 回复、长 Lock pin、回绕、失败与取消测试；新增 v2 quota、uint32 回绕、构建拒绝、100 次构造回滚句柄计数、四通道 sticky fault、损坏 Header/record、跨端诊断历史。
- `test_combined_ipc.ps1`：三套真实生产提取物（CRT＋PR6、VA＋PR6、VA＋v2），18 个性能/生命周期二进制。纯传输比较 PR6/v2，11 场景×两种 Host×两实现×（一次预热＋五组测量），共 264 个进程对；包括 0/64/4096/65536-byte batch、64/512KiB/2MiB stream、满载、回绕、slow Host 和 idle。
- `combined_shadow_ipc.cpp`、`run_combined_shadow_ipc.py`：三实现×两 Host×五个新进程，30 个进程对；每对连续三轮，16,384 对 64KiB VB/16KiB IB，合计 1280 MiB。使用实际生产 Shadow 分配 API、Command 和通道；Host 逐字节验证全部数据，完成回复后才能释放 Shadow；通道持续映射时扫描 Client AV 和最大连续空闲区。
- 现有 allocator pressure/首次写入、VB/IB 历史内容、多 Lock、Reset、PageBlock/GC、资源追踪、读回恢复及完整三端构建回归继续运行。Python 单元测试 39 项通过。

联合生命周期负载直接覆盖 allocator→Command→真实跨进程传输，**没有实例化 D3D9 GPU/DXVK**；Lock/Unlock 语义由独立现有回归覆盖。Host wall 包含等待 Client 分配，不能视为纯 Host 服务耗时；CPU 指标单独记录。此处没有实际游戏 FPS、1% Low 或 0.1% Low 数据。

## 优化前归因与 VA 结果

优化前生产提交 `af543260ccacc2595a12368f34209937dffb8bb8`；扩展联合测试的 `a8961200fab234e7104e1d21f9e56dfe013a7a71` 只改变测试，生产 patch 相同。两轮 Windows runner 都发现小包/回绕可重复退化。

第二轮五组配对中位数：

| 负载 | x86/x86 Client | x86/x64 Client |
|---|---:|---:|
| 64-byte batch | +10.05% | +11.52% |
| 64-byte wrap | +8.08% | +8.92% |
| 64-byte stream | −5.32% | −7.77% |

正号表示 v2 比 PR6 耗时更高。Host 在 paused-backlog 64-byte batch 的处理 wall 从 36.65→80.89 ns/命令（x86）和 29.51→72.95 ns/命令（x64）。这暴露 `waitForCommand.peek` 与 `pull` 对同一不可变记录重复校验，是最后 Header 值缓存优化的直接依据。不同 runner 的绝对 ns 不作直接前后配对比较。

联合负载优化前结果：通道保持映射时，CRT＋PR6 的 Shadow 释放后残留第一轮 **1295.8125 MiB**，后两轮 **1359.0625 MiB**；VA＋PR6 和 VA＋v2 三轮均为 **0 MiB**。三者最大连续空闲区约 2036–2045 MiB，本负载未证明最大连续区改善。v2 多出 0.515625 MiB 映射。

VA allocator 的部分存活放大和接近 1MiB 首次写入代价仍存在：既有部分存活负载曾出现逻辑 52.25MiB 对应 arena 769MiB，既有近 1MiB 首写配对退化约 22.7%。本次没有修改 allocator；不能把释放后归还 VA 的效果延伸为活跃资源更省内存或所有首次进图更快。

## 最后候选结果与交付

最后生产提交 `f09b443` 的全部 Windows 回归通过，264/264 传输进程对和 30/30 联合生命周期进程对有效；独立 VA 压力和首次写入回归亦通过。完整构建的所有必要检查通过。下载后核验了三个 PE 的架构、构建身份与 SHA-256，x86 Host LAA 标志正确。`gameplay_validated=false`：尚未完成用户实机验证。

最后候选与**同一 runner 的 PR6**五组配对：

| 负载 | x86/x86 Client 耗时变化 | x86/x64 Client 耗时变化 |
|---|---:|---:|
| 0-byte batch | +19.35% | +20.91% |
| 64-byte batch | **+23.72%**（+15.75%～+37.60%） | **+16.34%**（+15.23%～+22.55%） |
| 4096-byte batch | +0.25% | −0.29% |
| 65536-byte batch | +1.77% | −0.68% |
| 64-byte stream | +2.19%（−3.23%～+26.88%） | +9.65%（−18.53%～+22.21%） |
| 64-byte near-full | **+41.71%** | **+44.33%** |
| 64-byte wrap | +13.81% | +23.43% |

表中为逐对百分比的中位数，不是中位数相除。stream 范围跨零，不宣称稳定提升或退化。小命令 batch、near-full 和 wrap 五组均退化，已经足以否定当前候选达到性能门槛的假设。

64-byte batch 的绝对中位数和批次 CPU cycles：

| 指标 | x86/x86 PR6 → v2 | x86/x64 PR6 → v2 |
|---|---:|---:|
| Client ns/命令 | 181.24 → 216.96 | 183.83 → 213.21 |
| Host backlog drain ns/命令 | 80.36 → 124.14 | 63.20 → 104.96 |
| Client cycles/命令 | 443.11 → 527.54 | 449.74 → 519.28 |
| Host cycles/命令 | 196.64 → 303.86 | 154.77 → 256.50 |
| 完整批次折算端到端 ns/命令 | 265.73 → 356.03 | 248.75 → 323.12 |

Client/Host 增量不能简单相加当作游戏帧时间：生产线程可并行，负载也不同。短批次 GetProcessTimes 分辨率不足，多个样本为 0，不能解释为不消耗 CPU；因此保留原始 CPU 数据，并以批次 wall/cycles 辅助判断。

Header 值缓存从结构上减少了同一记录的重复解析，但**没有消除正常小命令路径退化**。优化前后 runner 不同，不用其绝对 ns 估算缓存收益。剩余成本的源码依据包括 Record 编解码、独立 block/header 访问、发布/消费同步、共享历史及健康检查；128-byte 物理槽位较紧凑 Header ring 扩大工作集。各项所占纳秒没有独立测定，不能声称已经定量定位到某一个原子操作。不能直接删除防覆盖/唤醒/故障检查换取达标。

最终联合 VA 结果与此前一致：CRT＋PR6 释放残留 1295.8125/1359.0625/1359.0625 MiB，VA＋PR6 与 VA＋v2 均为 0；额外映射 0.515625 MiB，最大连续空闲区约 2035.5–2042.0 MiB，未证明联合负载的连续区改善。第一轮完整上传完成 wall：x86 Host 的 VA＋PR6 531.19ms、VA＋v2 531.82ms；x64 Host 515.31ms、514.71ms。这些近似结果不能抵消小命令路径退化，也不证明游戏收益。

独立分配器回归另显示：2300-many-small 的释放后 CRT/arena 残留为 1311.625/3 MiB，最大连续区 918.56/1614.81 MiB；这与联合负载不同，不能互换。混合尺寸（包含接近 1MiB）首次创建＋完整写入，arena 相对 CRT 配对中位数 **+25.07%**（五组均退化）；many-small 总计则为 −15.63%。分配和首写必须合计，不仅看 allocation；此代价与 IPC 改动无关。

交付链接：

- [完整三端构建与所有构建回归](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38116766750)
- [配套测试构件：l4d2-bridge-patch-v1.2.2-dev.2](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38116766750/artifacts/11694460506)
- [最终原生对照与 VA 回归](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38116766924)
- [最终完整原始数据、日志与测试二进制](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38116766924/artifacts/11693916241)
- [优化前生产路径与扩展联合负载原始证据](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38115604987)

Actions 构件保留 30 天，下载需要 GitHub 登录。汇总、源码清单、逐文件构建核验和来源信息已归档到 `docs/evidence/va-ipc-v2/`；最终原始 ZIP 已实际下载并核验，原始样本与测试二进制保留在上述 Actions artifact。优化前大构件的数字汇总由仅输出固定有限数值且核对源码身份的只读 reader 获取，来源记录独立保留。

生产 patch SHA-256：`c45ef456eb8dde364e7b2b50826560ea0bba9216d3923f852544a0e5ec726f39`。

| 文件 | SHA-256 |
|---|---|
| x86 Client `bin/dxvk_d3d9.dll` | `14bcca4f925566e92c690100e760d41328c2df0fa74d0d071b3a7802f74a3592` |
| x86 Host `bin/.l4d2bridge/L4D2Bridge32.exe` | `b6ea3634465d653be8fac2541663f8887cef9c9abc155a4961c3109d77adaaea` |
| x64 Host `bin/.l4d2bridge/L4D2Bridge64.exe` | `462dee1f26375dc36fcf10b43b5f7fa93546e819c29ff56ed836b816cb553621` |

## 实机安装、对照与回退

退出游戏及 Host，备份游戏 `bin/d3d9.dll`、`bin/.l4d2bridge/L4D2Bridge32.exe`、`L4D2Bridge64.exe`。测试包仅取三个配套文件：`bin/dxvk_d3d9.dll` 重命名为游戏 `bin/d3d9.dll`，两份 Host 放入原 `.l4d2bridge`。**不要覆盖现有 DXVK、配置、ReShade 或 retention DB，不整包覆盖目录。** 三个文件必须来自同一构建；运行后核对双方 build identity。回退时退出进程并一起恢复这三个备份。

建议三组实机对照：当前稳定 main/PR6、VA＋PR6、VA＋v2；同一 Host 架构、地图、Mod、画质与诊断配置，分别冷启动、反复跨图、粒子压力和至少三次固定时长章节运行。每次重置 OSD，记录进图时间、Client AV、最大连续空闲区、Avg FPS、1% Low、0.1% Low。不得从 ns/命令或微基准推导未经测量的 FPS 收益。若 VA＋v2 不能稳定胜过 VA＋PR6，保持 VA＋PR6 即可；不为完整迁移而牺牲实测成本或正确性。
