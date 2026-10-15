# LDB updater 手动版本 VPK 实机测试

这是独立的本地测试构建，不是正式自动更新。保留现有 IPC v2、C8、Shadow/Steam/IME 及配置；没有合并 main 或发布 Release。需要 Windows 自带的 .NET Framework 4.8。

## 首次安装

完全退出游戏及所有 Host，备份原来的匹配三件套。把 `bootstrap/bin` 合并到游戏根目录的 `bin`，同时安装三个桥二进制、`LDBupdater.exe` 和 `LDBupdater-test.ini`。不要覆盖现有 bridge.conf、DXVK、插件、ReShade 或 retention DB。

本测试只支持 Client 实际使用 `bin/dxvk_d3d9.dll`、Host 位于 `bin/.l4d2bridge` 的标准布局；升级时仍须一起替换三个桥文件。x86/x64 Host 都有配套构建。

## 准备 VPK：只改版本号即可测试候选判断

1. 将 `LOCAL-TEST.vpk` 作为 ZIP 打开（必要时改名为 `.zip`）。编辑根目录 `VERSION`，从当前 `1.3.2` 改为较新的稳定版本，例如下一个 patch 版本。三段数字，不加哈希或测试后缀。
2. 保存并重新打包；VERSION、UPDATE-MANIFEST.json 和 bin 必须直接位于包根目录，不要多套一层文件夹。不修改三件套或其哈希清单。修改 payload 会被拒绝。
3. 选择测试数字文件名，将文件改名为该数字加 `.vpk`。首次测试先让游戏加载完成，再放到 `left4dead2/addons/workshop/`。该包实际是 ZIP，本轮没有验证 Source 引擎挂载或 Steam 分发；测试结束先移走它，再启动游戏，以隔离挂载影响。
4. 在 `bin/.l4d2bridge/LDBupdater-test.ini` 的 **WorkshopId** 填同一个数字。测试数字只在这一项配置，不要改 EXE、桥 conf 或其他脚本。Enabled=1 代表明确启用本地测试。

真实 Workshop ID 尚未决定；测试文件可使用自己选择的未占用数字，勿覆盖已订阅内容。测试不会联网下载、订阅或写入 Workshop 原文件。正式 ID 配置模板仍为 null，正式安装入口仍关闭。

## 操作流程

本轮只验证更新链路，不声称已检测游戏完全加载。游戏进入主菜单/测试场景、你确认加载结束后，按 **Ctrl+Shift+F10** 主动检查。每个游戏进程只检查一次；重复按键不会再启动 updater，重新测试需完整重启游戏和 Host。

- 新版包：Host 弹出当前/目标版本、简要说明和是/否。
- 否：继续游戏，暂存内容清理，不替换文件、不保存拒绝记录。下次完整启动可再次提示。
- 是：准备成功后再提示退出；确认后向游戏投递正常 WM_CLOSE。若游戏未退出，请使用游戏菜单正常退出，不要强制结束。
- updater 必须确认实际游戏和会话 Host 都退出，且没有其他同目录游戏/Host，才替换三件套与 `bin/.l4d2bridge/README.md`；超时/异常退出不安装。
- 更新完成后 helper 自动退出，不自动重启游戏。查看 `bin/.l4d2bridge/LDBupdater.log` 的 **UPDATE_OK**，及 README 的新介绍。看到成功后再启动游戏。

测试包的数字版本是可编辑的模拟标签；三件套仍来自同一已验证测试构建，实际内部身份不会伪装成所填写的新正式版本。测试状态记录明确标注 test_only/simulated_version。成功记录源包哈希；同一个包再次启动不会重复提醒。再次测试请改变 VERSION，使包哈希变化，并使用比实际当前版本更高的数字。

相同/更旧版本不提示；缺文件、ID 未填写、源文件被 Steam 写入、损坏 ZIP、哈希不匹配等不会替换。失败信息见桥日志与 LDBupdater.log。当前真实加载就绪事件和正式签名包入口仍未启用。

## 恢复和回退

正常失败会恢复全部旧三件套和旧 README。若存在 `LDBupdate-transaction.json`，不要删除 LDBupdate/backup；完全退出游戏和所有 Host 后，在 `.l4d2bridge` 运行：

```text
LDBupdater.exe --recover
```

未完成事务会阻止新的桥 Host 启动。恢复也会拒绝仍被同目录游戏/Host 占用的安装。

回退到原版时退出游戏及 Host，恢复原备份三件套；仅在确认无中断事务后移除本测试的 updater、INI、测试状态文件和 VPK。不要删除原配置、DXVK 或 DB。测试 updater 第一版不更新自身；正式发布方案的签名清单仍是后续工作，不能把这个明确启用的本地测试程序当作正式更新器分发。
