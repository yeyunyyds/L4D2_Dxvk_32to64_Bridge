# v1.3.5

- 修复 D3D9 PrivateData 资源所有权、COM 引用计数及缓冲区边界问题，简化普通数据存储。
- 优化 LG（learned-aggressive）持久 KEEP 数据库，消除已知指纹命中的重复写入；新学习仅增量读取和校验追加记录，减少地图加载和纹理读取中的冗余开销。
- 保持默认 LG 策略、历史数据库格式、纹理回收与恢复安全检查；不新增后台线程。
- 保留 v1.3.4 的既有功能和配置。EM2 实机验证通过，首次纹理学习或恢复仍可能短暂卡顿，不保证所有 MOD 兼容。

[安装与回滚](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/blob/main/docs/RELEASE-V1.3.5.md)

已有安装优先使用三件套升级包，保留配置、DXVK、ReShade 和 retention DB。提供完整包、配套升级包、独立 L4N 插件和 SHA-256 清单。
