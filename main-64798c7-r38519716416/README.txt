LDB main build 64798c7 / l4d2-1.3.1+d08b980cc177f23d

完整包与可选 L4N 插件均来自通过 CI 的同一次 main 构建，ZIP 字节未改动。
非新的正式 Release，VERSION 保持 1.3.1。
关闭游戏及 Host 后，备份当前二进制和 bin/.l4d2bridge/bridge.conf，再按包内 bin 路径覆盖。
完整包含官方 DXVK 后端和默认配置；L4N 设置插件为单独可选包。
保留自定义 bridge.conf 时，确认 server.steamImeNativeWarmup=True、server.steamImeTsfActivate=True；其余正常 Steam/Presenter 配置保持原样。
包含 PR12 Renderer 支持与中文 IME 修复，不含刚讨论但尚未实施的正确性移植。
