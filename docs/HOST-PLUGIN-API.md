# LDB Host 插件 ABI v1

LDB 提供一个默认关闭、在 Host 初始化阶段一次性决定的原生插件入口。此入口用于把屏幕歌词等可选功能保留在独立 DLL 中；本仓库当前只提供接口，不包含歌词插件，也不复制 PeakDangoMusic 的实现。

## 启用边界

运行配置：

```ini
server.plugins = False
```

False 是源码缺省值和随包值。关闭时 Manager 在获取 Host 路径之前返回，因此不会构造 `plugins` 路径、调用目录枚举、读取插件文件、查询导出或加载 DLL。运行期间不轮询目录，也不支持热开启。D3D9 呈现路径只保留一个可预测的“是否有 Present 回调”判断，不执行插件回调、状态保存、文件访问或线程工作。

启用后，Host 在 DXVK 后端初始化完成、创建 D3D9 Device 之前扫描：

```text
bin/.l4d2bridge/
├─ L4D2Bridge64.exe
├─ L4D2Bridge32.exe
└─ plugins/
   └─ ExamplePlugin.dll
```

插件必须与 Host 位数一致。加载失败、缺少导出、ABI 不匹配或插件拒绝初始化时，Host 记录结果并继续启动。插件按文件名排序加载，随 Host 退出反序卸载；不支持运行中替换或卸载。

## SDK 与导出

公共头文件为 [`plugins/host-sdk/ldb_host_plugin.h`](../plugins/host-sdk/ldb_host_plugin.h)。插件导出不修饰的 `LDBHostPluginQuery`，调用约定为 `WINAPI`：

```cpp
extern "C" __declspec(dllexport)
BOOL WINAPI LDBHostPluginQuery(uint32_t requested_abi, LDBHostPluginV1* out_plugin);
```

Host 先写入 `out_plugin->struct_size`。插件只接受 `LDB_HOST_PLUGIN_ABI_VERSION`，填满 `LDBHostPluginV1` 并返回 TRUE。`on_load` 随后收到只读 Host 信息：ABI、Host 位数、Host 目录、构建版本和日志函数。

ABI v1 提供这些可选回调：

| 回调 | 时机 |
|---|---|
| `on_load` | DLL 查询通过后，尚未创建 Device |
| `on_device_created` | Host 的真实 DXVK D3D9 Device 创建成功后 |
| `on_swapchain_created` | 主 SwapChain 链接或附加 SwapChain 创建成功后，传入真实 `IDirect3DSwapChain9*` |
| `on_before_present` | `IDirect3DDevice9::Present` 或 SwapChain `Present` 之前 |
| `on_before_reset` | `Reset` / `ResetEx` 调用之前，适合释放默认池资源 |
| `on_after_reset` | Reset 返回后，同时提供真实 HRESULT |
| `on_swapchain_destroyed` | Host 释放对应 SwapChain 之前 |
| `on_device_destroyed` | Device 最终 Release 之前 |
| `on_shutdown` | Host 退出且 DLL 卸载之前；插件必须在这里停止线程 |

`on_before_present` 在 Host 渲染线程调用。`LDBHostPresentInfoV1::swap_chain` 提供当前真实 SwapChain：SwapChain `Present` 路径保证非空；Device `Present` 路径按需取得主 SwapChain，查询失败时可能为空。插件不能在此等待网络、文件或后台锁；音乐状态应由插件自己的后台线程读取并发布无阻塞快照。插件若执行 D3D9 绘制，必须恢复自己改变的 render target、shader、texture、vertex/index buffer、viewport 和 render state。Host 不替插件保存整套状态，因为这会让未使用插件的路径承担成本。

## 故障与兼容边界

插件 DLL 与 Host 同进程，错误指针、死锁或设备误用仍可能使 Host 崩溃；C ABI 和异常边界不能提供进程隔离。插件不应保存回调参数中的临时结构指针，不能自行 Release Host 传入的 Device 或 SwapChain，也不能取消或替换 Present/Reset 返回值。若插件需要在回调外保留 COM 对象，必须自行 `AddRef`，并在对应销毁回调或退出前配对 `Release`。

未来的屏幕歌词插件可以复用自己的音乐 API、歌词解析和动画，在 `on_before_present` 使用真实 Host D3D9 Device 与当前 SwapChain 绘制；也可以按 SwapChain 生命周期维护独立资源。LDB 本体不负责网络、歌词、配置界面或字体资源。
