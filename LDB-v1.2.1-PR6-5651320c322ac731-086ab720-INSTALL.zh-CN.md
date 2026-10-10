# PR #6：L4D2 IPC 实机 A/B 测试包

仅用于现有 LDB v1.2.1 安装的测试更新，不是正式发布。

- 构建版本：`l4d2-1.2.1+5651320c322ac731`。
- 编译源提交：`086ab720b3c43ebe4ba8ac7545e34c0c9bc41aad`。
- 核验时 PR 最新提交：`57d8df33346eb63ac03948308478e2f6dcf130dd`，与编译源相比仅增加 `docs/IPC-DATA-QUEUE-CORRECTNESS.md`，生产代码及构建、测试输入没有变化。
- 来源：[Windows Actions run 38037527938](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38037527938)。`windows-build`、`baseline` 均成功，完整构建 x86 Client、x86 Host、x64 Host。
- 本 ZIP 从 Actions 已上传的配套构件中提取；二进制逐字节不变，仅精简分发内容并增加本说明和核验信息。没有重新编译或更改 PR 代码。

## 备份和安装

1. 完全退出 L4D2，确认 `L4D2Bridge32.exe`、`L4D2Bridge64.exe` 均已退出。
2. 在游戏目录之外建立备份目录，按原路径备份以下三个文件。另存一份当前配置和 retention DB，供记录测试起点；安装过程中不替换它们。
3. 将包内 `bin` 文件夹的内容复制到游戏的 `bin`，只替换下表三个文件。三个文件必须成套替换，不能混用新旧 Client/Host。

| 包内路径＝游戏目录下的替换路径 | 用途 |
|---|---|
| `bin/dxvk_d3d9.dll` | x86 Client |
| `bin/.l4d2bridge/L4D2Bridge32.exe` | x86 Host |
| `bin/.l4d2bridge/L4D2Bridge64.exe` | x64 Host |

包内没有后端 DLL、游戏配置、ReShade 或 retention DB；不要替换游戏根目录的 `d3d9.dll`、`d3d9vk_x86.dll`、`d3d9vk_x64.dll`、`bridge.conf`、ReShade 文件和 `resource-retention.db`。说明、许可证、JSON 和 `VERSION` 可以留在解压目录，不需要复制进游戏。

启动后查看 `bridge32.log` 以及实际选用的 `bridge64.log` 或 `bridge-host32.log`，两边 `Project build` 都应为 `l4d2-1.2.1+5651320c322ac731`。Host 位数沿用现有配置；不需要为了安装更改 `client.testX86Server`。

可在 PowerShell 中用 `Get-FileHash -Algorithm SHA256 <文件路径>` 与包内 `SHA256.json` 对照。

## 回退

完全退出游戏和两个 Host，用备份恢复以上三个文件，必须一起恢复。由于本包没有替换其他内容，DXVK、配置、ReShade 和 retention DB 无需因本次安装回退。重启后核对 Client/Host 日志恢复原构建版本。

## 建议的 A/B 记录

- A：恢复原版三个文件；B：使用本包三个文件。固定 Host 位数、DXVK、画质、分辨率、帧率上限、ReShade、插件和地图版本。
- 性能测试与故障诊断分开进行：测 FPS 时两组采用相同诊断配置，优先关闭可选 API Wait/Data/PageBlock 诊断；复现白屏时两组再开启相同诊断。使用各自独立备份的配置，切换时检查实际生效值。
- 使用相同 demo 或固定路线、相同采集长度；每组先预热，并交替 A/B、B/A，至少重复三轮。保存原始逐帧时间记录，汇总平均 FPS、1% Low、0.1% Low 与帧时间分布。
- 对 `Whitaker's Gun Range (Mod Test & Survival)` 另记录能否进图、加载时间、白屏/超时/弹窗和地图切换情况。失败运行单独报告，不能作为正常游戏 FPS 样本。
- 分轮保存 Client/Host 日志，记录 build ID、Host 位数及 retention DB 起始状态，避免把不同进程或不同运行的日志混在一起。DB 会在正常运行中更新；不要把初次学习与热运行直接比较。

原生回归通过，覆盖双架构传输、并发/UID、队列边界/故障、失败传播、Reset、VB/IB、Surface/Volume、ATI、Readback/Retention 与既有诊断等。但 PR 仍为 Draft，实机稳定性和游戏性能未验收。此前原生小包 IPC A/B 测得新增成本；该结果不能换算为实际 FPS。本包用于测清这项代价及进图稳定性，不承诺修复所有地图或提高 FPS。

原始构件：[Actions 配套测试构件](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38037527938/artifacts/11664263515)。
测试证据：[Actions 原生回归及 A/B 证据](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38037527938/artifacts/11664630837)。

源码补丁 SHA-256：`5651320c322ac731295e83cae5537a4ce3c4207362da88cbd730dfcf3c07520b`。完整构建及分发来源见 `BUILD-INFO.json`、原始 `SOURCE-BUILD-INFO.json`；原始 metadata 中 `distribution=release` 是既有打包脚本的字段，本包未创建 GitHub Release 或 Tag。
