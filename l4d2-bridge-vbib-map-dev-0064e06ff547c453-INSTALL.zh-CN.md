# 静态 VB/IB 按需映射实机测试包

构建：`ldb-vbib-map-dev+0064e06ff547c453`；编译提交：`eaeee728f2b09d71a213d0e113d66a8cd9f1d29b`。实验版，保留完整 IPC v2 和六档；不是正式 Release。

退出游戏及全部 Host，备份并成套替换：

- `bin/dxvk_d3d9.dll`：x86 Client。
- `bin/.l4d2bridge/L4D2Bridge32.exe`：x86 Host。
- `bin/.l4d2bridge/L4D2Bridge64.exe`：x64 Host。

保留现有 DXVK、配置、ReShade 和 retention DB。自定义 Client 文件名按实际已加载 Bridge 更新，避免覆盖 ReShade。回退时退出全部进程，成套恢复三个备份。三端日志必须显示同一构建。

默认已开启 ≥256 KiB 的非 DYNAMIC VB/IB 按需映射，空闲缓存 32 MiB。动态、小缓冲和共享堆保留原存储。完整历史内容保存在 section；解除映射归还 VA，不向 Host 取回内容。

同版本 A/B：在原 `bin/.l4d2bridge/bridge.conf` 设置 `client.vbibMappedShadow = False` 并重启，恢复原 VB/IB allocator。再设置 True 重启对照，保持 IPC batch、Host、地图、路线、Mod 和其他设置一致。默认 `client.vbibShadowCacheMB = 32`；0 可测最大回收，但冷访问更贵，不建议直接作为日常默认。

AV 测试可开启现有 `client.memoryMonitoring = True`，记录 l4d2-memory.log 的 va_free、largest_free、vbib_section_backing_bytes、vbib_mapped_va_bytes、vbib_idle_view_budget_bytes。Backing 不等于 Client VA；若选中 backing 很小，不能期待大量改善。FPS 对照应保持双方诊断一致，记录 Avg FPS、1% Low、0.1% Low、帧时间和加载时间，覆盖枪店↔训练图及重度 4K Mod。

全部 Windows 回归和三端构建通过，但冷/超缓存 Lock 有数 µs 开销，大缓冲完整创建上传也可能增加加载时间。尚无游戏 FPS 或这位 4K 用户 AV 改善结论。

CI：https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38183934718
报告：https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/blob/codex/vbib-mapped-shadow/docs/VBIB-MAPPED-VALIDATION.md
