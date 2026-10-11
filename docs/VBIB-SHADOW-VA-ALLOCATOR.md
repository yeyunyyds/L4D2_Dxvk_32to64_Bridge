# VB/IB Shadow VA 分配优化：实现与原生测试报告

本分支已实现分配器优化并通过 Windows 原生验证。它保留完整 Client Shadow；收益主要来自资源销毁后归还 VA，以及小资源分配的空间利用率。它不能消除仍存活资源的 Shadow，也没有证实实际 FPS 收益。正式 main、Release 和 Tag 均未修改。

## 版本与范围

- 基线：main `de74cd7d5dd546bfd4f03ab451d0565f1fff0107`，v1.2.2。
- 最终生产实现提交：`859a123333071c7ba7ea97c2ab1776281207a831`。
- 扩展测试提交：`ba05339ffc1a29faea1b407414b0caa3a51cd607`；与上述提交的生产补丁完全相同。
- 测试包标识：`l4d2-1.2.2-dev.1+df5515b3dc8bcda7`。
- 生产补丁 SHA-256：`df5515b3dc8bcda72b2b8affc98fda8db84803b68f340e283a366c03a8b5dd90`。

与基线逐文件对比，生产增量只涉及 `bridge/src/client/buffer_shadow.h`、`lockable_buffer.h` 和新增的 `buffer_shadow_arena.h`。Lock/Unlock 方法体逐字一致；FULL_SHADOW、历史内容、部分写入、重叠 Lock、上传、IPC、Host、PageBlock/LG 和 DXVK 路径未改。

## 优化原理与实现

原有小块 Shadow 经 CRT 分配。资源释放后，逻辑字节减少并不保证堆的保留地址空间同步归还。因此新实现将部分 Shadow 的地址空间所有权交给可释放的 Windows VA region：最后一个资源释放时使用 `VirtualFree(..., MEM_RELEASE)` 归还整个 reservation。

最终采用混合策略：

| Shadow 大小 | 分配方式 |
| --- | --- |
| ≤256 KiB | 28 档固定大小槽位，16 B 到 256 KiB，约 1.5 倍间距 |
| >256 KiB 且 <1 MiB | 独立 VA reservation |
| ≥1 MiB | 保留原 CRT `new[]/delete[]` |

小块 region 按需建立，通常最多 1 MiB、最少 64 KiB；只 commit 已使用前缀需要的页面。reservation 失败会尝试更小 region。空 region 不缓存，存活资源指针不迁移。元数据通过 Windows process heap 分配，空闲槽位链表放在已释放 payload 中，没有每个 payload 的额外堆节点。

Owner 保存 payload 指针与 region 指针，区分 arena 与 CRT 释放。新增 SRW 锁只参与分配、释放及统计，不进入 Lock/Unlock 热路径。全新 Windows 页面利用系统清零保证；复用槽位仍按原要求清零。分配失败继续抛出 `std::bad_alloc`。OS release 失败不会虚报 VA 已归还：小块 region 保留供复用，独立 reservation 保存描述符并在后续分配尝试回收。

这不是无成本优化。region 内只要有一个资源存活，其他空槽占用的 reservation 就不能释放；独立 VA 分配也有系统调用和页面首次触碰成本。

## 测试方法与正确性

[完整编译及现有原生检查](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38110932082)成功；[最终 VA A/B、首次写入及扩展回归](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38111113484)成功。

覆盖 x86/x64 allocator 边界、清零与复用、元数据/reserve/commit 失败回滚、缩小 reservation、释放失败恢复、部分资源存活和多线程分配释放；运行现有 VB/IB 内容、Lock、错误传播、生命周期、运行时观察及构建流程中的 IPC 回归。并发分配测试不等同于授权任意并发 D3D9 调用。

压力测试使用实际生产 allocator 与机械提取的 LockableBuffer，双方采用相同 COM/IPC fixture。Windows x86 LAA，每个场景五组新进程配对、交替运行顺序，每进程三轮生命周期；六场景共 60 次进程运行。另外三种首次完整写入场景各五组配对，共 30 次进程运行。计时在批次外层，用 wall clock 与 GetProcessTimes；VA 扫描、heap walk 不进入计时区。失败记录保留，只比较完成工作量一致的计时批次。

背景压力是 reserve-only，不是游戏或 Host 模拟。GetProcessTimes 的短批次可能受约 15.625 ms 量化影响；零 CPU 读数不表示零成本。原生测试不能给出 Avg FPS、1% Low 或 0.1% Low。

## VA 结果

下表为五组第一轮中位数，单位 MiB。剩余占用为所有被测资源销毁后、相对背景阶段的可用 VA 损失；最大空闲区也是释放后值。

| 场景 | 逻辑峰值：CRT / 新实现 | 峰值 AV 损失：CRT / 新实现 | 释放后剩余：CRT / 新实现 | 最大连续空闲区：CRT / 新实现 |
| --- | --- | --- | --- | --- |
| 约 2300 MiB 背景 AV，少量大资源 | 1292.25 / 1292.25 | 1301.63 / 1302.60 | 0 / 0 | 2036.12 / 2039.74 |
| 约 2300 MiB 背景 AV，大量小资源 | 1292.25 / 1292.25 | 1323.67 / 1296.02 | **1311.63 / 3.00** | **928.00 / 1626.88** |
| 约 800 MiB 背景 AV，大量小资源 | 667.02 / 686.08 | 691.16 / 691.04 | 679.13 / 3.00 | 22.69 / 355.00 |
| 约 800 MiB 背景 AV，碎片化背景 | 752.25 / 752.25 | 757.36 / 758.66 | 0 / 0 | 64.03 / 64.03 |
| 约 2300 MiB 背景 AV，分批保留少量资源，最后全部释放 | 1292.25 / 1292.25 | 1323.65 / 1296.06 | 1311.63 / 3.00 | 930.25 / 1622.88 |
| 约 2300 MiB 背景 AV，跨阈值混合尺寸 | 1293.50 / 1293.50 | 1341.67 / 1323.15 | **299.63 / 0** | 915.31 / 2043.49 |

大量小资源三轮释放后，CRT 剩余中位数为 1311.63、1359.06、1359.06 MiB，新实现均为 3 MiB；新 allocator 的 owned region reserve 全部归零。剩余 3 MiB 属于进程整体测量，不能记为 arena 保留。

低 AV 小资源场景 CRT 五次中四次耗尽，新实现没有耗尽。因此该行峰值工作量不同，不计算其分配/释放速度提升。所有失败保留在原始数据中。

部分存活是主要限制：只剩 52.25 MiB 逻辑资源时，CRT 的 AV 损失约 1323.65 MiB，新实现仍约 784.06 MiB，其中 arena reservation 769 MiB。改善并不意味着接近逻辑大小；交错生命周期会让大量 region 无法整体释放。

## 成本与选择依据

初版让全部大块也使用独立 VA。1.25 GiB 大资源“分配＋首次完整写入”中位数约 500.22 ms，而 CRT 约 274.45 ms。该方案已放弃，最终 ≥1 MiB 使用原 CRT。

最终版本首次分配并完整写入的结果如下。每组累计约 1.25 GiB；配对变化取五组比例的中位数，因此不等于两个时间中位数之比。

| 负载 | CRT / 新实现 wall 中位数 ms | 配对变化中位数及范围 | CRT / 新实现 CPU 中位数 ms |
| --- | --- | --- | --- |
| VB 8 MiB、IB 2 MiB | 275.005 / 269.334 | -2.51%，-9.28% 到 +24.84% | 281.25 / 265.625 |
| VB 64 KiB、IB 16 KiB | 517.294 / 483.139 | **-12.52%**，-17.88% 到 -6.32% | 515.625 / 484.375 |
| VB 1 MiB、IB 1 MiB−1 B | 312.432 / 378.378 | **+22.71%**，+20.71% 到 +26.67% | 312.5 / 375 |

上述三组释放 wall 中位数分别为 CRT/新实现 24.444/24.296、99.544/53.538、28.980/40.713 ms；配对完整生命周期时间差中位数分别为 -7.58、-116.86、+79.64 ms。

大量小资源压力场景的仅分配时间为 456.538→40.427 ms，释放为 90.198→55.570 ms；不能将仅分配改善解释为首次写入、上传或帧时间的同等改善。20,000 次动态 Lock 批次中位数为 2.370→2.401 ms，配对变化中位数 +1.27%，范围 -1.92% 到 +18.55%；方法体未变，当前样本不支持 FPS 结论。

接近 1 MiB 的首次写入退化可重复，是当前未解决问题。因此本版本只建议实验性实机 A/B，不直接推广为无损优化。若真实资源分布使这个代价主导加载或帧时间，应调整适用范围或放弃对应策略，不能为 VA 数字削弱 D3D9 语义。

## 复现与证据

1. `python scripts/prepare_bridge.py` 准备生产源码。
2. `scripts/test_buffer_shadow_arena.ps1` 执行 x86/x64 core 回归。
3. 运行现有 buffer contract、runtime observation 及 IPC 回归脚本。
4. `scripts/test_buffer_shadow_va.ps1` 自动准备固定 CRT 基线、生成两套生产测试、执行配对压力与首次写入，再汇总。

基线补丁 SHA-256 为 `e8a61ece359546f8b1ca815f3e708b8bb8caef12b3be8f46de0d3b46201a3ee9`。证据目录 [evidence/vbib-shadow-va](evidence/vbib-shadow-va) 提供 summary、源文件 manifest、构建核验及压缩原始文本，SHA256.json 校验文件内容。原始 diagnostics header 的文件哈希差异来自 CRLF/LF 换行；逐文件源码核验未发现其生产内容变化。

## 实机包与下一步

[下载配套测试构件](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38110932082/artifacts/11691920852)，包含匹配的 x86 Client、x86 Host 和 x64 Host。已核验 PE 架构、三者版本字符串、x86 Host LAA 和文件 SHA-256，记录在 evidence 中。GitHub Actions 构件有保留期限，下载可能要求登录。

完全退出游戏与 Host，备份当前游戏 `bin/d3d9.dll`、`bin/.l4d2bridge/L4D2Bridge32.exe`、`bin/.l4d2bridge/L4D2Bridge64.exe`。将包内 `bin/dxvk_d3d9.dll` 作为游戏 `bin/d3d9.dll` 安装，并替换两个同名 Host。三者同时替换或同时回退；保留原 DXVK 后端、配置、ReShade 和 retention DB。

实机应与 v1.2.2 基线在相同 Mod、地图、Host 位数和诊断配置下比较，分别记录首次进图、枪店/训练图反复切换和章节结束时的 Client AV、最大连续空闲区、加载时长、Avg FPS、1% Low、0.1% Low。重点检查接近 1 MiB 的资源首次使用、部分存活和重型 Mod 情况。当前确认的是合成生产路径中的释放后 VA 改善；极端用户首进图 AV 是否改善、游戏加载是否退化及帧时间净收益仍需实机证据。
