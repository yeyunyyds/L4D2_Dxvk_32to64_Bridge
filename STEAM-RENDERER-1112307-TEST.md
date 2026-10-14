# Steam Renderer 11.12.30.7 实机测试更新包

实验构建：`l4d2-1.3.1+d7f3336571a9fc8b`。
源码提交：`e4485f704e7a70621d8a91671b95f45a4440a27e`，基于正式 1.3.1。
[Draft PR #12](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/pull/12) · [通过的 Windows CI](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38510357861)。

新增已审查的 x64 GameOverlayRenderer64.dll 11.12.30.7 支持，保留旧版支持及未知版本拒绝。只改两个生产头文件；文件哈希在初次识别时只读一遍，不增加周期扫描。完整原生回归、360 组 IPC 组合、24 项失败封闭检查及三端 MSVC Release 编译通过。尚需实机确认 Workshop 与邀请菜单。

[下载测试 ZIP](l4d2-bridge-steam-renderer-1112307-test-d7f3336571a9fc8b-r38510357861.zip)。
ZIP SHA256：`59d77ba30e7ac888bef78fe4e7ab15b164476184572bf224edf795d9d966a74d`。

完全退出游戏与两个 Host，备份并同时替换 ZIP 内的三个文件：

- `bin/dxvk_d3d9.dll`
- `bin/.l4d2bridge/L4D2Bridge32.exe`
- `bin/.l4d2bridge/L4D2Bridge64.exe`

保留现有配置、DXVK、ReShade、retention DB 和插件。使用当前 x64 Host 配置，不需要替换 Steam 自身 DLL，也不需要开启全量诊断。测试 Shift+Tab、Workshop、邀请面板、交互与关闭。失败时提供 `bridge32.log` 与 `bridge64.log`。回退时同时恢复三份备份。

这个 ZIP 保存于下载分支，不是新的正式 Release；生产 main、Tag 与已发布附件均未修改。
