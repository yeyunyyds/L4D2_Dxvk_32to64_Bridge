# StudioRender 模型崩溃诊断测试包（Draft PR #15）

构建：`l4d2-1.3.2+7b8842a668f4c50a`
源码：`a848177e937e9b3d767af3c971062ea9b803295d`
Windows 完整构建及原生回归：https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/39220989490
报告：https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/blob/codex/studio-model-diagnostics/docs/STUDIO-MODEL-DIAGNOSTICS.md

本包是可选诊断测试版，没有合并 main 或发布正式 Release。只定位模型，不修复、跳过或吞掉崩溃。

## 安装与备份

1. 完全退出 L4D2 及 Bridge Host，备份下列三个旧文件和原有 bridge.conf。
2. 将本包 bin 目录合并到游戏根目录，只替换：
   - `bin/dxvk_d3d9.dll`（x86 Client）
   - `bin/.l4d2bridge/L4D2Bridge32.exe`（x86 Host）
   - `bin/.l4d2bridge/L4D2Bridge64.exe`（x64 Host）
3. 现有 DXVK、ReShade、配置和 retention DB 继续保留。本包没有 studiorender.dll、后端或配置文件。
4. 在正在使用的 bridge.conf 中设置 `client.studioModelDiagnostics = True`，然后重启游戏。默认值为 False；无需开启逐 API、内存扫描或命令历史诊断。

## 确认启用与收集

在 bridge32.log 中确认 `studio_model status=installed`，然后复现。若只有 waiting_for_module，表示还没安装；若为 disabled，表示文件身份/代码/槽校验没有通过。

本次仅支持用户上传的 x86 StudioRender：
`03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6`。
不需要替换这份游戏 DLL。其他版本会拒绝 Hook。

收集 bridge32.log 和现有日志目录 crash_reports 下的 bridge-exception-*.txt。报告 kind=active 是故障线程的活动模型；recent_completed 只表示最近正常返回的模型。名称最多来自模型头 64 字节，LOD 显示 unknown。

开启时每模型写两条事件；这是复现诊断用途，尚未进行游戏帧时间验证。

## 关闭与回退

退出游戏，将 `client.studioModelDiagnostics` 改为 False，重启即可关闭追踪。
需要完整回退时，退出游戏后恢复备份的三个文件及需要恢复的配置项。
