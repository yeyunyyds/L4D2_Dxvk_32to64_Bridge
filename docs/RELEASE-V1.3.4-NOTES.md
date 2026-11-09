# v1.3.4

- 常用日志统一按 Bridge Session 保存，默认保留最近 10 个 Session
- Client/Host 异常退出显示已观察到的故障链与完整报告路径
- 新增默认关闭的 StudioRender 模型诊断和 Host TCP 443 快速失败选项
- L4N 菜单语言与位置均可跨启动保留
- 保留 v1.3.3 的 Steam Renderer 11.15.70.27 与 ReShade 6.8 输入修复

[下载正式版](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/releases/tag/v1.3.4) · [安装与回滚](RELEASE-V1.3.4.md)

提供完整包、配套三件套升级包、独立 L4N 插件与 SHA-256 清单。异常窗口和调用链只报告已观察到的故障上下文，不单独证明根因。StudioRender 与其他详细诊断默认关闭；`server.blockHostTcp443` 默认关闭。
