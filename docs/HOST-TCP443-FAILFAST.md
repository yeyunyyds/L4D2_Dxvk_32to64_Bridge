# Host TCP 443 快速失败测试

`server.blockHostTcp443=True` 在 Bridge Host 初始化期间安装进程级 Winsock 拦截。它覆盖 IPv4/IPv6 的 `connect`、`WSAConnect`，并包装通过 `WSAIoctl` 获取的 `ConnectEx`。远程端口为 TCP 443 时立即返回 `WSAENETUNREACH`；其他地址、端口、命名对象和 Bridge IPC 不变。

该选项用于 Steam Renderer 等注入 Host 的模块在受限网络中同步连接 HTTPS、导致 CreateDevice 被 SYN 重传长时间占用的情况。Windows 将连接归属显示为 `L4D2Bridge64.exe`，但 LDB 本体没有主动创建该 Steam Web 请求。

默认配置：

```ini
server.blockHostTcp443 = False
client.createDeviceTimeoutMs = 30000
```

测试时关闭游戏和两个 Host，将第一项改为 `True` 后完整重启。Host 日志应出现：

```text
L4D2_HOST_NETWORK policy=tcp443-fail-fast enabled=1 result=installed
```

第二项是 CreateDevice 匹配回复的独立硬截止，单位毫秒，有效范围 1000–600000；超出范围会夹到边界。它不改变普通 IPC 超时、批次或运行期 FPS 路径。

启用快速失败会阻止 Host 进程内所有模块的 TCP 443，包括 Steam 网页、Workshop 页面和可能的崩溃上传。它不按 URL 或目标 IP 筛选，也不阻止 UDP 443。若 Steam 改用其他传输，本选项不会声称已经覆盖。关闭选项并完整重启即可回退；Hook 不会在关闭状态安装。

实机验证应记录启用前后的 CreateDevice 等待时间、Host 日志、TCP SYN 行为、能否进入游戏，以及 Shift+Tab、本地 Overlay 和 Workshop 的实际结果。快速失败只处理已确认的 TCP 443 黑洞，不替代 CreateDevice 失败清理或其他 Host 阻塞调查。
