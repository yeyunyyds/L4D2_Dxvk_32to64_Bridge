# VA Allocator 与 PR7 IPC v2 生产接入实验

实验分支：`codex/vbib-va-ipc-v2`。继承 VA 分支 `b90d056006f892765b0cc2ebfc895b406bda4725`，PR7 独立原型参考 `dbb7b2d91561ba220585c9af659d5d9a2ae5248b`。

本次用户授权生产接入与完整验证。当前第一阶段是 **v2 命令记录＋PR6 payload ring 的混合生产实现**，不是 PR7 完整统一消息流或 upload pool。是否继续迁移 payload，必须由此阶段的正确性、资源回收与成本证据决定。没有合并或正式发布。

## 已接入边界

`Client Command → PR6 数据 reservation/提交 → v2 命令记录即时发布 → Host wait/pop → 原 Host handler 解码 → PR6 end_read_data/pin/消费确认`。

Device/Module 请求与回复四条通道均使用新的命令适配器。每条命令对应一个 64-byte payload block，保留原 Header 的 command、flags、handle、dataOffset；Record 为 24 bytes，body 为 4-byte dataOffset。每 API 发布一次，禁止依赖下一 API 才提交的隐式 batch。数据包大小不改变策略。

尚未迁移：scalar/blob 本体、Header 与 payload 的双重发布、原 UID 回复顺序、长 Lock 的 read pins、SharedHeap ownership，以及 GPU/DXVK 同步。它们仍由已验证的 PR6 路径负责。不能用 Header 的 v2 completed 替代 payload consumed，更不能当 GPU fence。

## 为生产新增的条件

- Client 建通道先于 Host 启动：允许 first opener 初始化 mapping，peer 校验 ready、协议、Config 和构建身份；独立原型的 writer-only 创建不能直接沿用。
- Reply 多线程：就绪数据 peek 返回 Header 值；消费确认发布后，后续 waiter acquire 可观察原 payload 游标完成。Client 回复队列的 peek/copy/pull 用局部 SRW 锁防止一个 waiter 读 Header 时另一 waiter 归还并复用该槽位；这属于新增同步成本，不能省略。Host 单 dispatch 线程不加此锁。空队列等待另通过冷路径 timed mutex 保证一个 registered waiter。
- 原命令队列 usable capacity 为 queueSize−1；新物理槽为 power-of-two，但逻辑 quota 保留原值，不能扩大测试预算获得优势。
- 容量 deadline、故障、等待失败与取消导致 sticky session fault，并唤醒已注册的四方向通道；不能失败后继续提交。
- 冷路径保留原 peer process handle，避免 PID 重用把旧进程误认成活着。
- 新协议版本与生产构建身份拒绝不同 v2 构建混用。旧 PR6 与新包也必须三端成套安装，不支持混搭。

生产 `ipc_v2` 是以 PR7 核心为来源的适配副本；`experiments/ipc-v2` 保留原型原文件与历史证据，二者不应混称同一已验证版本。

## 内存与性能假设

新命令映射为 `384 + nextPowerOfTwo(queueSize)×128` bytes，四方向单独映射。旧 command reservation 留在原 mapping 中，确保 payload capacity/偏移不变。本阶段增加 VA，而不是节省 IPC VA；应纳入 Client AV 净值。默认 Device forward queueSize=3072，新增约 512 KiB；逆向及 Module 也按实际配置计入。64 KiB 映射粒度可能进一步影响实际 AV。

Header 编码、v2 SC 发布/完成、健康检查和冷路径同步均可能增加成本。减少原 Header queue 的阻塞不自动等于 FPS 改善，原 PR7 即发负载也没有稳定胜出。VA allocator 的既有近 1 MiB 首写退化仍存在。

## 验证入口

- 现有 `test_ipc_transport.ps1`：真实生产序列化、线程回复、失败、pin、回绕、known/generic 完整布局。
- 现有 allocator、VB/IB contract、diagnostics 和完整三端 build 回归。
- `test_combined_ipc.ps1`：固定 main v1.2.2 与组合实现，同 caller、独立 TU、MSVC /O2，无 LTO；大小包、批次、连续、满载、回绕、slow Host、idle，x86/x86 与 x86/x64，五组交替。
- `test_buffer_shadow_va.ps1`：重跑 VA 生命周期、部分存活与首次完整写入，保留耗尽样本。

当前报告在实施阶段；Windows 原生结果与构建链接待补，未声称测试通过、性能收益或游戏可运行性。统一 payload 与完整 PR7 替换仍未完成。只有生产回归通过才能提供对应实机包；若性能/正确性门槛失败，应保留明确失败证据，停止推广当前候选。
