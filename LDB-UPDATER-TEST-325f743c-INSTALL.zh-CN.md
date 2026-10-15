# LDB updater 本地实机测试包

[下载 ZIP](./ldb-updater-test-v1.3.2-325f743c-r38663341438.zip)

- 基于 v1.3.2；构建 `l4d2-1.3.2+325f743c784c0e30`。
- 编译源码 `c7d9607db3e124a89787fb8f956b843b01dd7101`。
- [全部通过的 Windows CI](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38663341438)：95 项更新核心检查、18 个 x86/x64 Host 跨进程场景、实际 Client 控制提取、360 组 IPC、24 项故障和既有完整资源/Steam/IME/诊断回归。
- 三端为 MSVC 14.29 /O2 NDEBUG Release；updater 为 x86 .NET Framework 程序，服务两个 Host 位宽，需要 Windows .NET Framework 4.8。
- ZIP SHA256：`598a55c4be22bb3e8c3cc7984786cb69a277a42110f08f0fe10457b9997066f5`。

## 首次安装

完全退出游戏和所有 Host，备份三个旧桥文件。将 ZIP 内 `bootstrap/bin` 合并到 `left4dead2.exe` 所在游戏根目录的 bin，安装匹配三件套、LDBupdater.exe 与专用 LDBupdater-test.ini。沿用原 Host 选择和所有 bridge.conf、DXVK、ReShade、插件、retention DB。包内许可文件须随测试包保留。

在 `bin/.l4d2bridge/LDBupdater-test.ini` 只填一处 `WorkshopId=你选择的未占用数字`，保留 Enabled=1；不含 .vpk 后缀。此 INI 是本地测试入口，不是修改桥 conf。正式 Workshop ID/签名包入口未启用。

## 修改版本并触发

1. 用 ZIP 工具打开 `LOCAL-TEST.vpk`（必要时先改后缀为 .zip），编辑根目录 VERSION：把 1.3.2 改为 1.3.3，UTF-8 保存。不要改 payload 和清单；重新打包时不多套目录。
2. 把文件改名为 INI 中相同数字加 `.vpk`。第一次隔离测试建议先启动游戏，进入主菜单并确认加载完成，再将这个文件放进 `left4dead2/addons/workshop/`。
3. 按 Ctrl+Shift+F10 手动检查；每个游戏进程只检查一次。这一包不自动判断“完全加载”。
4. 选否：继续游戏，清理暂存，下次完整启动可重新触发；同一次运行不再提示。选是：准备成功后再确认正常退出；若游戏未退出，请用游戏菜单正常退出，勿强制结束。
5. updater 确认实际游戏和 Host 都退出后，替换三件套与 `.l4d2bridge/README.md`，保留配置及后端，清理并退出，不自动重启。以 `LDBupdater.log` 的 UPDATE_OK 和更新后的 README 为完成依据，再重新启动游戏。

此 VPK 实质是 ZIP，本轮验证更新器读取和替换；没有验证引擎挂载或 Steam 实际分发。为隔离这两件事，首次测试结束先移走测试 VPK，再启动游戏。再次验证拒绝重提示时，可在下次游戏加载完成后放回并触发。未验证的 Source VPK 挂载行为不能当作更新器已通过的能力。

数字版本是可编辑的模拟标签，随包样例三件套来自本次同一构建；不会把内部身份伪造成新正式版本。程序也验证了不同新构建配套三件套的实际替换。成功后记录包哈希，同包不反复提醒；再次测试可修改 VERSION 让包哈希变化，仍需高于实际旧版。

## 恢复与回退

如有 LDBupdate-transaction.json，不要删除暂存/备份。完全退出游戏和全部 Host，再从 Host 目录运行 `LDBupdater.exe --recover`。无法完成回退时保留全部恢复材料。回退到原版本需退出后同时恢复原三件套；确认无中断事务再移除测试 updater、专用 INI 和测试状态文件。

当前保持 [PR #13](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/pull/13) 为 Draft，未合并 main，未发布 Release/Tag，尚待 L4D2 实机验证。
