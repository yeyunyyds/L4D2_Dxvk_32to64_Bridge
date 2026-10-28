# Steam Renderer 11.15.70.27 兼容更新

本次核对用户提供的更新后 `GameOverlayRenderer64.dll` 与 `GameOverlayRenderer.dll`，只做静态读取，没有加载、执行或重新分发这些专有 DLL。

| 文件 | 位数 | 字节数 | SHA-256 | `IsOverlayEnabled` |
|---|---:|---:|---|---|
| `GameOverlayRenderer64.dll` | x64 | 1,679,512 | `05a62859e55a9f2a8cb987cbdc79a3303f23ad7c83b4be56f5494beaa993ed43` | RVA `0xb8a90`，`movzx eax, byte ptr [rip+0xdd852]; ret`，字段 RVA `0x1962e9` |
| `GameOverlayRenderer.dll` | x86 | 1,443,480 | `1afe9b93e0f36cbee628346e03ccae0c3f75567fdd2dd99c7b326b7f01eec841` | RVA `0x9ea70`，`mov al, byte ptr [0x101581b9]; ret`，字段 RVA `0x1581b9`（首选 ImageBase `0x10000000`） |

资源版本均为 11.15.70.27。x64 的 getter 布局、导出 RVA、映像大小和字段 RVA 与已核对的 11.12.30.7 样本相同，但完整文件身份已经改变，因此旧白名单会报告 `renderer-file-hash-mismatch`，好友邀请／Workshop 入口继续保持原不可用结果。

修复把两个新文件身份分别加入对应位数的白名单，并新增 x86 getter 的绝对地址解码。Host 仍在首次识别时只哈希一次完整文件，再核对导出、指令字节、PE 位数、映像边界和字段地址；未知版本继续拒绝。代码只读取 Renderer 自己公开 getter 所返回的同一状态字节，不写 Renderer、不调用私有 Steam ABI，也不强制把不可用状态改为可用。

原先 x86 Host 明确返回 unsupported；有了本次用户提供的 x86 样本后，x86/x64 Host 都走相同的“完整身份 + 指令 + 边界”验证。实际好友邀请界面、Workshop、焦点和输入仍需在 Steam 游戏内验收，静态核对和原生 fixture 不能替代运行验证。
