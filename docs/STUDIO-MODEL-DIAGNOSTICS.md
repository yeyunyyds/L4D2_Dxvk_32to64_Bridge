# v1.3.3 StudioRender 诊断与持久化菜单测试版

本分支在正式 v1.3.3 与可选 L4N Native HUD 的基础上加入模型构建崩溃归因。它不修改模型数据、不跳过异常，也不修改正式 `VERSION`、Tag 或 Release。

## 使用

三个 Bridge 文件必须来自同一构建：

- `bin/dxvk_d3d9.dll`
- `bin/.l4d2bridge/L4D2Bridge32.exe`
- `bin/.l4d2bridge/L4D2Bridge64.exe`

L4N 菜单插件安装到 `bin/neko/plugins/L4D2BridgePlugin.dll`。它把用户调整后的菜单坐标写入同目录的 `L4D2BridgePlugin.ini`。没有保存坐标时，菜单默认位于屏幕左侧、垂直中心略偏上。插件仍不读取或捕获鼠标。

模型诊断默认关闭。复现崩溃前在现有 `bin/.l4d2bridge/bridge.conf` 中加入：

```ini
client.studioModelDiagnostics = True
```

重启游戏后，先在 `bridge32.log` 确认出现 `studio_model status=installed`。异常报告应包含 `studio_model kind=active`；名称来自进入 Hook 时复制到 TLS 的 `studiohdr.name[64]`，报告阶段不会读取模型指针。

## 支持的 StudioRender 文件

只接受下列两个经过实际文件反汇编验证的 x86 文件。加载后的完整 `.text`、PE 身份、虚表槽与入口必须同时匹配；其他文件会拒绝安装。

| 变体 | SHA-256 | SizeOfImage |
| --- | --- | ---: |
| ThinFlex | `03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6` | `0x653000` |
| Valve | `3f5f5b0f539e8ad22bcfc4381be41571257c0c29e8061057682f9b8525ca7b85` | `0x453000` |

两者具有相同的模型入口 ABI、`CStudioRenderContext` 虚表槽和已确认调用链：

```text
LoadModel 0x69cd0
  0x69e36 -> 0x69b00
  0x69c62 -> 0x69950
  0x69a32 -> 0x69560
  0x696f2 -> 0x68890
  fault 0x6896f: movss xmm6, dword ptr [eax]
```

ThinFlex 文件额外包含 `.cflex` 虚拟节并修改五处 flex 常量；这些差异不在 Hook 入口和模型追踪 ABI 内。实现仍按完整文件哈希选择各自的映像尺寸和代码验证，不进行特征扫描。

## 边界

- Hook 只覆盖安装后经已验证虚表槽进入的 `LoadModel`。
- 当前 LOD 在该入口无法可靠取得，报告为 `unknown`。
- 模型头不可读或名称为空时只能记录状态和已复制的指针数值。
- 开启时每次模型构建会增加有界复制、TLS 更新和 begin/end 日志；正常逐帧绘制不经过该 Hook。
- 关闭时不安装 Hook、不分配模型 TLS，也没有周期扫描。

Windows CI 必须同时完成双文件静态审计、双映像加载校验、x86 Hook ABI/TLS/异常测试、三端 Release 构建、L4N x86 插件构建以及既有回归测试。游戏实机结果仍需单独记录。
