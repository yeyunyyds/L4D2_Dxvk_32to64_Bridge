LDB correctness-only development build 7bfc3e0 / l4d2-1.3.1+3d545d8aa5ca7164

实验包，未合入 main，未发布新 Release；VERSION 保持 1.3.1。
包含 Query、Shader 范围、UpdateTexture、SharedHeap 和 Host 句柄正确性修补；保留 main 的 Steam/IME 修复及默认配置。
优先使用 patch ZIP：退出游戏及 Host，备份并覆盖包内三份配套 Bridge 二进制；它保留现有后端与运行配置。
full ZIP 另含官方 x86/x64 DXVK 后端和默认配置；覆盖前备份 bin/.l4d2bridge/bridge.conf。L4N 是单独可选包，插件源码未改动。
回滚时整组恢复此前 main 构建的三份 Bridge 二进制；不得新旧 Client/Host 混用。
SharedHeap 默认仍关闭。无需为本轮测试主动开启。请验证进入地图、反复切图、动态资源、Steam 邀请/中文输入和正常退出。
原生专项、集成回归及三架构编译均通过；L4D2 实机验证待完成。
完整审计见包内 docs/LDB-CORRECTNESS-AUDIT.md。
