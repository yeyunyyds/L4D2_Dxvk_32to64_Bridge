# 完整 IPC v2 + VA allocator 实机测试包

实验构建：`ldb-ipc-full-dev+9f30cbf7ddabdb95`。编译提交：`2064d38c1f9c576846da3b2d31f1cd769462e0d1`。不是正式 Release。

退出游戏及所有 Host，备份并成套替换：

- `bin/dxvk_d3d9.dll`：x86 Client。
- `bin/.l4d2bridge/L4D2Bridge32.exe`：x86 Host。
- `bin/.l4d2bridge/L4D2Bridge64.exe`：x64 Host。

仅合并包中 bin；保留现有 DXVK、配置、ReShade 与 retention DB。自定义 Client DLL 名称按实际 Bridge 文件更新，避免覆盖 ReShade。回退时退出全部进程并成套恢复三个备份，不要混用版本。

在原 `bin/.l4d2bridge/bridge.conf` 设置 `client.ipcBatchSize = 1`，重启。支持 1/2/4/8/16/32；先测 C1 再逐档比较。C1 也是完整 v2，不是旧 IPC。日志应为同一构建，Client IPC_FULL 应有 `legacy_data_ring=0 reply_v2=1 upload_leases=1` 与选择的 batch。

Windows 原生回归和三端构建通过，但小参数 Client CPU 开销仍比最新 PR7 生产基线高，尚无游戏 FPS 改善证据。请比较固定场景 Avg FPS、1% Low、0.1% Low、帧时间、AV、最大连续空闲区、加载时间及枪店↔训练图稳定性。

完整构建与原始证据：https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38129726737
报告：https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/blob/codex/ipc-v2-full-integration/docs/IPC-FULL-VALIDATION.md
