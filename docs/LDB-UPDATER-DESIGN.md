# LDB Workshop 更新器实施方案

本分支只建立设计和配置模板，没有修改生产代码、编译 updater、启用自动更新或发布版本。配置中的 Workshop ID 和签名公钥均未确定。

源码基线：main `cafc0442446c502a0de0ebb52d5e73ccaae67516`，VERSION 为 `1.3.2`，包含 Steam Renderer/IME 和正确性修复。后续编写代码前重新核对远端，保留其他会话的更新。

## 1. 确定的行为

更新源为游戏 `left4dead2/addons/workshop/` 下指定数字 ID 的 `.vpk`；内容是正式 LDB ZIP，仅改变后缀。每个游戏进程在真实启动加载完成时由 Client 通知 Host 一次，切图不重新触发。

Host 后台读取包、计算 SHA256。与上次成功安装的包哈希不同，才启动固定路径的 LDBupdater.exe。updater 验证和暂存包，比较正式数字版本；相同或更旧版本结束，不提示。发现新版本，由 Host 显示当前版本、目标版本、README 简要介绍和“是/否”。

选择否：本次运行不再提醒；清理可删除的暂存内容，updater 退出；不持久记录拒绝，也不更新已安装包哈希。下一次游戏启动可重新提示。

选择是：先完成准备检查，再请求正常退出。updater 等待真实游戏及 Host 退出后，更新三件套和简要 README，核验、记录成功状态、清理并退出。默认不自动重启游戏。

只更新 Client、Host32、Host64 和指定 README；保留 conf、DXVK、ReShade、插件和 retention DB。失败整套回退，不能留下混合版本。

## 2. Workshop 数字 ID：单一来源

唯一人工维护位置为 [config/updater-source.json](../config/updater-source.json) 中的 `workshop_id`。

- 目前为 null：未配置，不猜测数字；运行时不访问 Workshop、不启动更新任务、不提醒。CI 可以构建普通 Bridge，但不生成可发布 Workshop 自动更新资产。
- 确定后填写非零、无前导零的十进制字符串，解析后须能放入 uint64。字符串避免脚本/JSON 大整数精度问题。
- Host 查找文件名、updater 源标识、发布 ZIP 改后缀的名称及发布目标均由该字段派生，不在代码、conf、脚本和 CI 中复制第二个实际数字。
- `scripts/generate_updater_config.py`（待实现）读取配置，生成共享 `updater_source.h` 和构建记录；Host/updater 使用相同生成常量，打包器读取相同 JSON。生成文件不得手工修改。
- VERSION 继续是唯一发布版本来源，不能新增第二份手填版本。

“一改全改”指改一个字段后重新构建配套 Host/updater 与发布资产；并非修改源码 JSON 就能让已安装旧 EXE 实时改变 ID。Client 不需要知道 ID。

固定目录规则：

```text
HostDirectory = 当前 Host 可执行文件的绝对父目录
BinDirectory  = HostDirectory.parent
GameRoot      = BinDirectory.parent
Package       = GameRoot / workshop_directory / (workshop_id + package_extension)
Updater       = HostDirectory / LDBupdater.exe
Staging       = HostDirectory / LDBupdate / 事务标识
```

使用 GetModuleFileNameW，不使用进程工作目录。验证安装布局为 `bin/.l4d2bridge`；异常或自定义布局提示手动更新，不能猜位置。第一版不递归扫描 Steam 库或整个 Workshop。

## 3. 已确认的源码入口

- `scripts/prepare_bridge.py` 固定 NVIDIA 上游 `9aa74f8dfad2188efbd0f717c64d9f8fa909787e`，应用累计生产补丁。
- `scripts/build_bridge.ps1` 实际使用 `package_development.py`；三端身份为 `l4d2-VERSION+patchHash`。现有 PE、配套身份、BUILD-INFO.json、SHA256.json 和升级包检查可以复用。
- 固定上游 `util_messagechannel.{h,cpp}` 提供 GetMessage/PostThreadMessage 低频消息通道，参数为两个 uint32，适合少量状态通知，不适合路径、指针或 README。
- Client `d3d9_lss.cpp` 握手后初始化 `gpServerMessageChannel`；Host `main.cpp::RegisterMessageChannel()` 的 `init(nullptr, nullptr)` 使用方式是单向 Client→Host，不能假设已有对称 Host→Client 发送。
- Host `RegisterExitCallback()` 使用注册等待监视实际游戏进程；Client Process 封装保留实际子 Host 句柄。可以复用进程身份，不依靠退出后重新寻找 PID。
- Present 和 Bridge 握手不能证明游戏加载完成。

上述依据为本分支基线累计补丁、实际构建脚本和未改动的固定上游通道。下文新增类和消息均为拟议设计，不是已有能力。

## 4. 架构分工

| 组件 | 职责 |
|---|---|
| Client UpdateReadyNotifier | 验证过的加载完成事件、进程级一次通知；把正常退出请求交到游戏线程 |
| Host UpdateCoordinator | 单次后台哈希、当前版本/目录识别、启动 helper、候选弹窗与退出协调 |
| x86 LDBupdater.exe | 同时服务 Host32/Host64；包认证、流式解压、独立控制连接、进程等待、事务替换与回退 |
| 发布打包器 | 使用通过检查的三端资产、生成清单及简要 README、签名，按唯一 ID 改 ZIP 后缀 |

Client→Host 就绪通知复用低频消息通道。Host→Client 正常关闭请求通过新登记的窗口消息异步投递到游戏窗口；不借用 Remix UI Active/Steam capture 位。

Host↔updater 使用独立本地命名管道，绑定现有 Bridge session GUID。仅允许当前用户、拒绝远程连接，验证连接端 PID/固定可执行文件路径。跨位宽进程句柄通过 DuplicateHandle 建立，不把 x64 句柄装入 uint32 消息。

预计修改位置：Client d3d9_lss/message_channels/window 和经验证的就绪适配器；Host main、独立 coordinator；共享 updater 协议头；新增 updater 工程；打包器/CI。不改 IPC v2 的数据记录、批次、上传池、资源生命周期、Shadow、GC、DXVK。

## 5. 加载完成信号的验证门

第一版的“加载完成”定义为本次启动第一次进入可操作主菜单；直接连接服务器时需等价的首次可操作就绪事件。不是每次地图完成加载。

当前 Bridge 尚无已证实的该事件。实现前必须核对 L4D2 引擎/GameUI 的真实回调或有明确 SDK 契约的接口，在主线程验证冷启动、快速启动、直连和异常启动。不能猜 vtable 槽位、硬编码指令地址、进行无边界内存扫描，也不能使桥本体依赖可选 L4N 插件。

若不能取得可信信号，保持自动检查关闭并报告障碍；不得暗中用首帧或固定延时代替。ReadyNotifier 使用进程级状态，Reset/Device 重建不重新触发。就绪早于控制通道建立时只保留一个待发标记，在握手完成时发送；发送失败则本次跳过。Host 对重复消息去重，不让 Client 同步等待更新结果。

## 6. 一次哈希及源文件稳定性

Host 在后台打开源文件，请求只读、仅共享读取。持有这个对象计算 SHA256，并复制相同源句柄给 updater；哈希和解压使用同一对象，防止 Steam 下载中途改写导致两次读到不同内容。共享冲突则本次跳过，下次启动再检查；不修改 Workshop 文件。

一次完整顺序读取，初始块缓冲 64 KiB。本次启动无论缺文件、拒绝、无新版本或错误，检查都进入终态，不重复哈希/启动 helper。正常渲染线程不读文件。

已安装状态固定存储在 `.l4d2bridge/LDBupdate-installed.json`，记录成功版本、源包哈希、三端身份和已安装文件哈希；不放在待清理的 LDBupdate 目录。不记录拒绝。只有完整事务成功提交才更新，已有安装无记录时按实际运行版本判断。

## 7. 清单、版本与四个安装目标

下一版正式 ZIP 增加 `UPDATE-MANIFEST.json`、detached signature 和 `UPDATE-README.md`。Full ZIP 可保留配置/后端，但 updater 只解压并安装允许的四项。缺少可信清单的历史 ZIP 不自动安装。

清单包含 schema、产品标识、稳定版本、通道、最低 updater 协议、三端共同构建身份及四项 payload 的大小/SHA256。固定 slot 不允许清单指定任意目标位置。

| slot | ZIP source | 固定安装目标（相对游戏根） |
|---|---|---|
| client | bin/dxvk_d3d9.dll | bin/dxvk_d3d9.dll |
| host32 | bin/.l4d2bridge/L4D2Bridge32.exe | 同路径 |
| host64 | bin/.l4d2bridge/L4D2Bridge64.exe | 同路径 |
| readme | UPDATE-README.md | bin/.l4d2bridge/README.md |

README 为独立 UTF-8 简要介绍，显示文字，不执行脚本/HTML；不覆盖游戏根目录或插件文档。原 README 不存在时允许新建并记录此状态以便回退。

正式版本按 major/minor/patch 三段 uint32 比较，不按字符串或哈希排序；相同或更旧版本不提示。同版本替换包也不作为新版本安装。内部现有构建哈希保留用于三端匹配，只从准确已验证的构建身份提取正式版本展示。

第一版仅支持现有 Client DLL 加载布局；如果实际 Client 以其他名称/路径加载，则提示手动更新，不写另一个未加载的 DLL。检测发现实际已安装三端不匹配时也停止自动更新，不猜测基线。

初始限额建议：源 ZIP 64 MiB，展开总量 128 MiB，单二进制 32 MiB，README 256 KiB；元数据另设小限额。发布打包器同样检查这些限制。

## 8. 包认证和解压

SHA256 判断内容变化，不能证明发布者身份。清单采用 Windows CNG 可验证的 RSA-PSS/SHA256 签名，覆盖清单原始 bytes；公钥由统一配置生成，私钥不入仓库、不随包发布，专用发布步骤签名。公钥未配置时自动安装保持关闭。

验证顺序：源大小限额 → 读取有界清单/签名 → 认证/产品/schema/版本 → 四 slot 和路径校验 → 流式展开允许的四个文件 → 文件 SHA/大小、PE 位数及配套身份 → CANDIDATE。

选择固定版本、许可证保留的 ZIP 库；不调用 shell/包内程序。拒绝路径越界、绝对路径、ADS、设备名、重复条目、大小写等价重名、链接/重解析点、损坏和超限数据。目标/暂存父目录也检查重解析点。只解压允许 payload，不将整包覆盖到游戏目录。

## 9. 交互与退出顺序

控制消息都有版本、session/事务标识和有界长度：HELLO、NO_UPDATE、ERROR、CANDIDATE、DECLINE、PREPARE、PREPARED、COMMIT_ON_EXIT、CANCEL。EXIT_INTENT 只作提示，不是进程退出证明。

Host 在独立更新 UI/任务显示候选，不阻塞 D3D dispatch 或 Presenter 输入循环，关联游戏窗口。弹窗包含“当前版本 XX，可更新至 XXX；更新需要退出游戏，现有配置保留”和 README 摘要。

否：DECLINE，清理后退出；本次不重提示。

是：PREPARE，检查暂存、权限、空间、备份资格及 updater 能力。PREPARED 后显示退出/取消提示；取消清理。选择退出，先发 COMMIT_ON_EXIT 并收到确认，再向 Client 主线程发送 REQUEST_NORMAL_CLOSE。

关闭动作优先使用游戏已有正常退出处理，验证 WM_CLOSE 行为；若不能可靠退出则提示用户在游戏菜单退出。不能强杀游戏或修改 Host 原有退出兜底。Host 不能在自己已经退出后发送第二次通知，退出意图在关闭前完成交接。

updater 持有真实游戏和 Host 进程句柄，等待其 signaled 后才替换。管道断开、Host 日志、心跳停止、DLL 卸载都不能代替。未经批准的退出/游戏异常退出不自动安装。正常游戏退出码及 Host 退出兼容性由原生/实机验证确定。

以规范化安装根派生同用户互斥对象，一套安装只有一个事务。提交前用 Restart Manager 等一次占用检查确认无其他同目录游戏/Host；不自动关闭其他实例、不自动提权。初始退出等待建议有界 5 分钟，超时取消，不留下无限常驻 helper。

## 10. 文件事务和恢复

LDBupdater.exe 固定放 Host 目录，不能位于其将清理的 LDBupdate 内。第一版不更新自身；包要求更高 updater 协议时提示手动升级。旧版桥不会自动获得 updater 能力，首次需人工安装配套桥和 updater。

事务目录保存 payload 和完整旧文件 backup。`.l4d2bridge/LDBupdate-transaction.json` 保存小型事务状态，独立于可删除暂存。

提交顺序：

1. 等待真实退出，再检查目标占用、父路径、权限、空间和实际版本未改变。
2. 持久写事务记录并 FlushFileBuffers；备份全部旧三件套及原 README，核验备份；README 原本不存在要记录。
3. 同卷临时文件逐项写入、核验、替换，记录进度。不能宣称四文件一起原子替换。
4. 校验最终四项和三端身份；写已安装记录并持久标记 COMMITTED。
5. 清理本事务暂存/备份、移除事务记录，退出。清理失败仅延后清理，不把成功安装撤回；不递归删除未知目录。

任一步失败整套回退。回退失败保留备份和事务，不删除恢复材料；断电/强杀残留不能当普通缓存清理。

支持 updater 的 Client 在 Host 握手前检查一次小型未完成事务标记，发现中断时不启动混合 Host，提示退出后运行固定 updater 恢复。已加载的 Client 也必须退出才能恢复其 DLL。既有三端构建身份拒绝保持第二层保护。

游戏在提交期间重新启动是必须测试的竞态；更新互斥不能阻止任意外部游戏启动。占用检查、启动事务门及版本拒绝共同处理，不保证未知旧版本具备新的启动门。

## 11. 成本预算

未配置 ID/公钥时只有启动时生成常量判断，文件读取/哈希/更新线程/helper/弹窗为零。已配置时 Client 有少量进程级状态和一次消息，Host 有一次短期后台任务。解压缓冲、包处理和文件事务主要在 updater 地址空间，不能将完整包常驻 x86 Client。

Present、Lock/Unlock、D3D dispatch 不引入 hash/ZIP/版本解析/等待/弹窗或逐帧扫描。异步读取仍会消耗 CPU/磁盘，必须对加载时间和帧时间测量，不能预先宣称零 FPS 成本。

## 12. 分阶段实施和验收

1. 配置生成、清单/签名/版本和独立离线 updater 夹具；ID 保持未配置，禁止真实安装。验证一处 ID 改动覆盖所有消费者、未知版本拒绝和路径/大小边界。
2. 验证真实加载完成与正常退出入口；接入单次通知及 Host 后台候选检测，先不替换。覆盖两种 Host、主菜单/直连/切图/Reset、源文件缺失/下载冲突、重复消息和关闭路径零工作。
3. 独立测试安装目录完整事务：拒绝下次重提示、相同/旧版本、低空间、权限、占用、多进程、重启竞态、各替换步骤失败、强杀/断电模型和回退恢复。
4. Windows 三端+x86 updater 配套 Release 编译；现有 IPC/资源/Steam/IME/诊断测试完整保留，核验 PE、身份、签名和四项哈希。比较禁用、无更新、有变化三个负载的启动时间、CPU/IO、Avg FPS、1% Low、0.1% Low。
5. 单独测试分支实机验收，再配置真实源 ID。未经发布授权不合并 main、不创建 Release/Tag。

实施前尚需确认：用户的 Workshop ID；真实加载完成与正常关闭入口；签名公钥/发布步骤；ZIP 库版本。已确定 README 目标及单一配置结构。这些未验证事项不得写成已完成生产能力。
