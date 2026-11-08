# LDB v1.3.3 Studio Hook + 持久化菜单测试包

关闭游戏以及 L4D2Bridge32/L4D2Bridge64，备份并替换以下四个文件：

- `bin/dxvk_d3d9.dll`
- `bin/.l4d2bridge/L4D2Bridge32.exe`
- `bin/.l4d2bridge/L4D2Bridge64.exe`
- `bin/neko/plugins/L4D2BridgePlugin.dll`

不要复制或覆盖 `bridge.conf`、DXVK 后端、ReShade 和 retention DB。需要模型诊断时，将 `STUDIO-DIAGNOSTICS.conf.txt` 中的配置行合并到现有 `bridge.conf` 并重启。功能默认关闭。启动后可在 `bridge32.log` 核对已匹配的 StudioRender 变体与哈希。

菜单位置在移动时立即写入插件同目录的 `L4D2BridgePlugin.ini`。首次使用的默认位置位于屏幕左侧、垂直居中稍偏上；菜单内重置位置会删除已保存坐标。

回退时关闭游戏和 Host，恢复上述四个备份文件。可删除 `L4D2BridgePlugin.ini` 复位菜单位置。
