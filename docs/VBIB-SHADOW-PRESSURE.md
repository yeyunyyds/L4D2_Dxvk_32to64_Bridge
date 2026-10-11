# VB/IB shadow 与极端重型负载下的 AV 收益调查

结论：**应继续研究 shadow 的分配与销毁如何归还地址空间，但不能直接删除 FULL_SHADOW。** Windows 原生实验已经证明，大量小缓冲可以在对象销毁后留下超过 1 GiB 的堆 reserved 地址空间，显著压低 x86 Client 的 AV。这不是每次动态 Lock 都积累一份备份，也不是仍有同样数量的有效 shadow 或物理 RAM 被占用。极端用户“原生约 700 MB、x64 Host 桥约 800 MB”的具体归因仍需该用户的分类日志；本实验不能冒充其游戏复现。

本调查只提交隔离测试分支 `codex/vbib-shadow-pressure`。正式生产补丁、main、Release、FULL_SHADOW、Lock/Unlock、IPC、PageBlock/GC 与 DXVK 均未修改。

## 实测来源与复现

- 基线：正式 v1.2.2 / main `de74cd7d5dd546bfd4f03ab451d0565f1fff0107`。
- 最终测试代码：`fe07a42`。生产累计补丁 SHA-256：`e8a61ece359546f8b1ca815f3e708b8bb8caef12b3be8f46de0d3b46201a3ee9`。
- [最终 Windows 原生实验与既有契约回归：成功](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38103413596)，构件 `vbib-shadow-pressure-evidence`。
- [先行原生实验：成功](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38103080486)，[纯数组对照：成功](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38103257626)。先行实验与最终实验的大量小缓冲首轮 AV 残留均为 1311.625 MiB。
- 第一轮编译因测试变量名与 Windows `small` 宏冲突失败；只修正测试变量名后重跑。没有删除生产检查或降低编译警告要求。
- 环境：GitHub Actions Windows 2022，MSVC 14.29，原生 x86 `/O2 /W4 /WX /LARGEADDRESSAWARE`；静态 CRT 与生产 Meson 的 `b_vscrt=static_from_buildtype`、debugoptimized 构建类型相符。地址扫描上限实测约 4 GiB。

```powershell
python scripts/prepare_bridge.py
./scripts/test_buffer_contract.ps1
./scripts/test_buffer_pressure.ps1
```

`generate_buffer_pressure_test.py` 复用既有生成器，机械提取生产 `LockableBuffer` 模板，模板正文没有改写。COM 与 IPC 使用既有确定性适配器；压力模式关闭既有测试的自定义 new[] 包装，采用真实 CRT new[]/delete[]，防止额外分配头改变尺寸与堆行为。纯数组对照直接调用生产 `l4d2_buffer::allocate` 后以原生 delete[] 释放，不创建代理对象、不执行 IPC。源码哈希、五组原始 JSONL、汇总和校验清单见 [evidence](evidence/vbib-shadow-pressure/summary.json)。

这不是完整游戏/Host/GPU 仿真。它测量 x86 Client shadow 路径，不实例化真实 x64 Host；用户已确认其低收益对照使用 x64 Host、相同地图阶段。原生 D3D9 对照进程、引擎私有分配与驱动内存不由该适配器模拟。

## 实际存储链与假设

生产 `client/lockable_buffer.h`：构造器在非共享堆路径调用 `initShadowMem()` → `l4d2_buffer::allocate(desc.Size, kind, true)` → `new uint8_t[size]()`，按完整资源尺寸分配并清零。静态、动态均如此。普通 Lock 返回这份 shadow 内的范围指针，Unlock 按原范围提交；销毁后由 unique_ptr 的 delete[] 释放。DISCARD/NOOVERWRITE 不在此路径创建新的永久 shadow。现有 optimized dynamic Lock 即使向 IPC 预留区返回指针，非共享堆构造器仍分配完整 shadow，不能将启用它视为已消除副本。

默认 `useSharedHeap=False`。纹理/Surface 的 PagefileShadow、view cache 与 PageBlock 回收不负责这些 new[] VB/IB。VB/IB shadow 在游戏的 x86 Client 中，因此换成 x64 Host 不会自动移走这部分地址占用。

需要分别检查：

1. 当前仍存活的静态/动态完整 shadow。
2. 资源是否实际销毁，而非仍被绑定状态、引用或 StateBlock 保留。
3. shadow 已 delete[]，但 CRT/Windows 堆仍保留的地址区域。
4. 总 AV 与最大连续空闲块的差异。
5. Surface 映射、IPC 和引擎私有存储等其他 Client 占用。

## 控制负载与指标

每组独立 x86 进程，背景仅用 MEM_RESERVE 模拟已占用地址空间，不伪造游戏 RAM 负载。2300/800 是目标余量（MiB），受 64 MiB 预留粒度影响，实际初始 AV 不完全等于目标。碎片组先填充再分散释放 64 MiB 区域。

- 少量大静态缓冲：每对 VB 8 MiB、IB 2 MiB。
- 大量小静态缓冲：每对 VB 64 KiB、IB 16 KiB；1280 MiB 静态载荷对应 32768 个对象，再加 2 个动态对象。
- 每轮动态基线：VB 12 MiB、IB 256 KiB，合计 12.25 MiB。
- 静态载荷依次增长到约 128/256/512/768/1024/1280 MiB，之后每轮执行 20000 次动态 Lock/Unlock。随后释放静态、单独增长动态到约 512 MiB、释放所有对象。重复三轮。
- 保留约 64 MiB AV 作为上传适配器余量，因此低 AV 组结果不是生产 OOM 阈值。碎片组另验证 96 MiB 单块申请。
- 故意保留旧“地图”对象再创建新对象只验证同时存活的叠加占用，不作为游戏泄漏证据。

指标在批次边界采样：GlobalMemoryStatusEx 的 ullAvailVirtual、生产 VirtualQuery 扫描的 free/largest/private committed/private reserved、GetProcessMemoryInfo 的 Private Bytes/Working Set、真实 CRT 堆的 HeapWalk 活跃字节。HeapWalk 在解锁后才输出；所有最终采样均完成扫描。没有在热循环内计时，不声称 CPU/FPS 收益。

上传适配器每次调用后清除载荷捕获，防止测量累计测试副本。逻辑 shadow 字节是当前拥有对象的完整描述尺寸；纯数组对照的同名字段表示对应数组载荷。它不包含代理基类在真实游戏中的全部存储，不能视为完整 Bridge 内存总数。

## 最终结果

第一轮结果，单位 MiB。AV 减少相对于同组受压背景；“释放后仍减少”在 static 与 dynamic 对象全部销毁后测量。

| 负载 | 初始实际 AV | 峰值逻辑载荷 | 峰值 AV 减少 | 释放后 AV 仍减少 |
|---|---:|---:|---:|---:|
| 2300 / 少量大 VB/IB | 2289.86 | 1292.25 | 1301.95 | 0.00 |
| 2300 / 大量小 VB/IB | 2290.50 | 1292.25 | 1323.68 | 1311.63 |
| 2300 / 同尺寸纯数组对照 | 2290.50 | 1292.25 | 1307.82 | 1295.81 |
| 800 / 少量大 VB/IB | 754.50 | 692.25 | 697.00 | 0.00 |
| 800 / 64 MiB 碎片孔洞 | 818.50 | 752.25 | 757.43 | 0.00 |

大量小 VB/IB 首轮释放后：shadow=0、对象=0、CRT 活跃块约 **0.251 MiB**、Private Bytes 约 **11.723 MiB**，但 Private Reserved 相对背景增加 **1303.027 MiB**。后者与 AV 未恢复约 1311.625 MiB 的数量级相符，其余包含少量 committed 与元数据变化。不是仍有 1.3 GiB 的有效数据副本或 resident RAM。

三轮后，大量小 VB/IB 的 AV 从 **2290.50 → 978.88 → 931.44 → 931.44 MiB**；纯数组对照从 **2290.50 → 994.69 → 947.25 → 931.44 MiB**。两者最终相对初始背景均残留 **1359.06 MiB**，活跃堆块约 0.25 MiB。它们说明释放后堆预留与分配形状有关，不能据三轮数据宣称无限增长或真实 COM 泄漏。

少量大缓冲三轮销毁后 AV 均恢复到初始背景。所有生产模板负载每轮 20000 次动态 Lock/Unlock，逻辑 shadow 与 AV 均未增加；纯数组对照没有运行这些 API，不算入该结论。

碎片组初始总 AV **818.50 MiB**，最大连续空闲块仅 **64 MiB**，96 MiB 单块申请被拒绝。这验证了“AV 尚有数百 MiB”并不保证大块 shadow 能分配成功；不是 Bridge 本轮新增的错误返回语义。

## 现有实机记录能说明什么

重新分析用户先前的 `l4d2-data-client.log`（PID 4620，build `l4d2-1.2.1+e8a61ece359546f8`，与 v1.2.2 同生产补丁），按同一 snapshot 对齐 VB 和 IB：

- 同时存活的 shadow 峰值：**268.72 MiB**，snapshot 95，共 6193 个对象。
- 该时刻动态 **6.25 MiB**、静态 **262.47 MiB**。
- 动态的独立峰值：**12.25 MiB**，snapshot 11；不能将不同 snapshot 的分类峰值直接相加。
- 最终 snapshot 111 的两类 live shadow 与对象数均为 0。这只证明逻辑资源释放，不能由 data 日志推断 CRT 已归还全部 VA。
- 这不是极端用户的日志，不能代表该用户的对象数、峰值、分配尺寸或堆保留量；累计创建/上传字节也不是实时 shadow 或保留 VA。

这些记录不支持把正常用户的数百 MiB 占用主要归给动态缓冲。重型用户可能有更大的静态几何缓冲或更多小资源，也可能主要受 Surface/其他内存限制；必须分类核对。

## 对 700 → 800 AV 的判断及优化建议

“桥移出渲染端后，只增加约 100 MB AV”是净结果，不能直接说明 Host 移出的内存只有 100 MB。净变化由迁出的 native D3D/驱动占用、Client shadow/堆预留、IPC 与 Surface 映射，以及其他路径变化共同决定。native 本来也可能持有相关 CPU 存储，不能把 Bridge shadow 全部当成新增净损失。

本实验已经有力回答机制问题：**重型分配形状确实可能让 Client shadow 及释放后的堆预留吞掉上 GB 的 AV 收益，值得做分配/释放策略的优化可行性评估。** 但尚不能回答该极端用户实际损失中各项的比例，也不证明某个候选分配器已经更好。

下一阶段优先研究 allocator 的 VA 生命周期，保留完整 shadow、历史字节与多锁语义。分别比较资源存活期间的 AV、Private Bytes、销毁后的实际归还、碎片及分配/销毁 CPU 成本。不能只看到 free 后好看就采用对所有小资源单独 VirtualAlloc 的方案：64 KiB 预留对齐、页粒度、系统调用数量和连续分配成功率均可能改变净收益。也不能通过清空仍存活 shadow、强制释放仍被引用资源或替换空暂存来绕过 D3D9 契约。

极端用户后续沿用既有诊断，合并下列键而非覆盖配置；重启后采集相同地图的进图、章节结束和切图。用于内存归因，不与关闭诊断的 FPS 对照混用。

```ini
client.dataDiagnostics = True
server.dataDiagnostics = True
client.dataSnapshotMs = 5000
server.dataSnapshotMs = 5000
client.memoryMonitoring = True
server.memoryMonitoring = True
```

对齐 data 的 full/dynamic/static shadow 与 memory 的 va_private_reserved、va_private_committed、largest_free，并核对现有 Surface mapped/backing、IPC 容量和版本/Host。若 shadow live 归零但 private reserved 仍高，可进一步检查堆保留；若 shadow 和资源数持续高，则先审计真实引用生命周期。仅凭全进程 private reserved 不能区分 Bridge 与引擎堆，仍需结合资源趋势或独立分配追踪。

目前建议是 **继续查内存分配与 VA 回收，不盲目优化或删除动态 shadow**。没有修改正式生产行为，也没有根据该实验声称游戏 FPS、加载时间或极端用户 AV 已改善。
