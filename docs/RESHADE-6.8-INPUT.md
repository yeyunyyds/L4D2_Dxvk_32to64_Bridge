# ReShade 6.8 输入兼容

参考 [NPCodex Nightlybuild 的 `b1fa0a1` 修复](https://github.com/NPCodex/L4D2_Dxvk_32to64_Bridge-Nightlybuild/commit/b1fa0a1)，Presenter 现在在已绑定 ReShade 且菜单捕获输入时取得 Host 子窗口的原生键盘焦点。ReShade 6.8 屏蔽应用低级键盘回调后，Home 的第二次按键仍通过真实窗口消息队列进入 ReShade，因此可以关闭菜单；鼠标复选框也使用同一原生捕获窗口。

Steam 与 ReShade 的捕获来源分别记录，再聚合决定游戏输入状态。即使聚合状态保持开启，来源发生变化时也会重新应用原生焦点，避免 Steam 菜单和 ReShade 菜单交接时沿用错误模式。未绑定 ReShade 时，原有 Home 兜底行为不变；Steam 中文输入初始化和输入窗口适配继续保留。

修复不修改 ReShade、不注入合成点击、不增加轮询线程，也不改变未创建 Presenter 时的路径。ReShade 应安装到 `bin/.l4d2bridge/L4D2Bridge64.exe`，选择 Vulkan。当前原生回归验证焦点获取、来源交接和恢复；真实 ReShade 6.8 的 Home、复选框、Shift+Tab 与切图仍需游戏内验收。
