# StudioRender 模型构建崩溃诊断

本功能只做归因，不修复或跳过模型崩溃。独立分支：`codex/studio-model-diagnostics`，基于 main `cafc0442446c502a0de0ebb52d5e73ccaae67516`（1.3.2）。不修改 IPC、VB/IB、Shadow、PageBlock、DXVK 或模型计算逻辑。

## 使用

退出游戏，备份原来的配套 Bridge 文件和 `bridge.conf`，安装本分支编译的 Client 与配套 Host。保留原有 DXVK、ReShade、retention DB 和其他配置。**不要用本功能附带或替换游戏的 StudioRender DLL**；构建包不包含 Valve 引擎文件。

在原有 `bridge.conf` 中增加或修改：

```ini
client.studioModelDiagnostics = True
```

重启游戏。此开关会启用现有 Client 基础异常处理器，不要求打开 `crashDiagnostics`、内存监视、API Wait 或逐 API 日志。先在 `bridge32.log` 中确认：

```text
studio_model status=installed entry=0x69cd0 slot=0x7f064 sha256=03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6 lod=unknown
```

- `waiting_for_module`：当时 DLL 尚未加载；只在后续 D3D/Device 创建时重试，没有后台扫描。如果之后没有 `installed`，这次运行不具备模型追踪能力。
- `disabled reason=identity_or_live_code_or_slot_verification_failed`：文件或加载代码不匹配，或者槽已被其他 Hook 修改；没有强行安装。不要自行替换偏移以绕过验证。
- `event=begin/end`：同一线程同一 `sequence` 的模型构建开始/正常结束。访问违规不会被此功能捕获后继续运行，也不会伪造结束事件。

复现后保留 `bridge32.log` 与现有日志目录 `crash_reports` 下的 `bridge-exception-*.txt`。优先看报告中的 `studio_model kind=active ... scope=faulting_thread`。`recent_completed` 只是该线程最近一次正常完成且名称有效的模型，**不能当作正在崩溃的模型**。

关闭时设为 `False` 并重启。默认值就是 `False`。

## 已确认的二进制证据

实际文件来自用户本次上传，未通过文件名推断版本，未执行其引擎函数。SHA-256：

```text
03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6
```

| 属性 | 实际值 |
| --- | --- |
| 文件大小 / Machine | 634,672 字节 / `0x014c`，x86 PE32 |
| 首选 ImageBase / SizeOfImage | `0x10000000` / `0x653000` |
| PE 时间戳 | `0x6a440dd6` |
| 实际接口字符串 | `VStudioRender026` |
| CodeView | `studiorender.pdb`；GUID `CF61F71B-77BF-4B70-94CD-DE40F297F519`，Age 12 |
| 对照 | 与 NB 固定提交 `b1fa0a1d6ba9f2e2c23c92e4da13bf324e883cc8` 的 ThinFlex 文件逐字节一致 |

没有取得匹配 PDB。下面的内部函数名称是根据机器码、参数流和分析字符串做出的结构识别，不能称作恢复了正式符号。

```text
CStudioRenderContext 虚表 RVA 0x7f03c
  槽 10，RVA 0x7f064 → 0x69cd0，bool LoadModel(this, hdr, vtx, hardware)
    0x69e36 call 0x69b00  [逐 LOD 的 StaticMeshes]
      0x69c62 call 0x69950 [SingleMesh]
        0x69a32 call 0x69560 [BuildMeshGroup]
          0x696f2 call 0x68890 [顶点数据处理辅助函数]
            0x6896f: f3 0f 10 30    movss xmm6, dword ptr [eax]
```

`+0x6896F` 是**读取**计算出的顶点相关地址。它不是 ThinFlex 修补说明中另一个地址的写入故障；仅凭地址不能确定坏模型、非法索引或上游损坏中的哪一个是根因。

入口 ABI 的依据：`+0x69CD9` 保存 ECX 为对象，`[EBP+8/+0xC/+0x10]` 分别作为三个输入指针；`+0x69CF3..+0x69CF9` 对比 VTX/MDL checksum；失败分支 `+0x69D0F` 返回 AL=0，成功分支 `+0x69E7B` 返回 AL=1；两个返回点均为 `ret 0x0c`。名称错误输出分支 `+0x69CFB` 对第一个输入加 `0x0c`，随后用作 `Model %s` 的参数。RTTI 指向 `.?AVCStudioRenderContext@@`。

参考资料：

- [实际 NB 文件及固定版本来源](https://github.com/NPCodex/L4D2_Dxvk_32to64_Bridge-Nightlybuild/tree/b1fa0a1d6ba9f2e2c23c92e4da13bf324e883cc8/runtime/engine)。用户上传文件是运行时支持的唯一身份依据；没有把“NB 文件存在”当作用户一定用了它。
- [官方 Source SDK `istudiorender.h`](https://github.com/ValveSoftware/source-sdk-2013/blob/be57e3d01ac7d7aa6a2cacbfef187c0b46616db0/src/public/istudiorender.h#L272)：公开 `LoadModel(hdr,vtx,hardware)` 语义。SDK 是 `VStudioRender025`，实际文件是 026，**没有复用 SDK 虚表序号**。
- [官方 Source SDK `studio.h`](https://github.com/ValveSoftware/source-sdk-2013/blob/be57e3d01ac7d7aa6a2cacbfef187c0b46616db0/src/public/studio.h#L2134)：`studiohdr_t` 前缀和 `name[64]`。结合实际 `hdr+12` 的使用确认名字来源。
- 官方 SDK 仓库没有提供这几个 StudioRender 内部函数的实现。公开引擎参考中的 `R_StudioCreateStaticMeshes`、`R_StudioCreateSingleMesh`、`R_StudioBuildMeshGroup` 只用于理解结构，没有复制引擎实现，也没有借用其地址和签名作为 Hook 依据。

可重复静态审计：

```text
python -m pip install pefile==2024.8.26 capstone==5.0.6
python scripts/inspect_studiorender.py <实际studiorender.dll> --require-known --output disassembly.json
```

脚本从已确认的各函数入口解码，核对直接调用边、故障指令、返回点、RTTI 和虚表槽；不加载/执行 DLL。未知哈希不会输出这些偏移注释。

## 实现与安全边界

生产源码由项目既有 `patches/l4d2-bridge.patch` 提供，通过 `scripts/prepare_bridge.py` 应用到固定上游。新增文件是 `client/studio_model_diagnostics.{h,cpp}`、`util/studio_model_context.{h,cpp}`。

1. `RemixAttach` 只读取开关。哈希、模块探测、页保护和 Hook 安装在随后 `Direct3DCreate9/Ex`、`CreateDevice/Ex` 入口执行，不在新加的 DllMain 路径执行。没有 Present/Draw/Lock 轮询。
2. 开启后，先验证完整磁盘文件 SHA-256，再对加载后的整个 `.text` 与该文件比较，正确应用 PE `HIGHLOW` 基址重定位，并核对实际槽仍指向已审计入口。比较用临时 `VirtualAlloc`，结束 `MEM_RELEASE`；不保留整段引擎代码副本。
3. 对齐的 32 位槽通过 `InterlockedCompareExchangePointer` 替换，保留已经取到旧函数指针的在途调用；不改写机器指令、不暂停线程、不覆盖第三方已占用槽。记录“安装之后经此虚表槽进入”的模型。安装之前已进入或自行缓存原函数指针的外部调用不在覆盖保证内。
4. 回调使用经验证的 x86 fastcall 适配 thiscall：ECX 为对象，忽略 EDX，原样转发三个栈参数、bool 返回和异常。保留 Win32 LastError。隔离的 `__try/__finally` 只在异常展开时清除本层 TLS，不吞异常、不改数据、不改变失败返回。异常搜索阶段尚未展开，未处理 AV 的报告仍能读取活动模型；若引擎原有外层 SEH 自行处理异常，展开后也不会留下悬空 TLS。该清理分支不写日志、不分配、不调用 API。
5. 每个参与线程首次健康进入时才分配一个小 TLS 状态，包含独立的活动快照和最近正常返回快照。每层 Scope 在自身栈上保留用于嵌套恢复的数据；**异常 getter 只读取 TLS 自有活动副本，不解引用 Scope 栈指针**。进入保存前一层，返回或异常展开时恢复。TLS 析构先清空指针，再释放存储。
6. 入口用一次 `ReadProcessMemory` 复制 80 字节头前缀，校验 IDST 和最小长度，最多复制 64 字节原名，转义控制字节/引号，明确标记空名、不可读、非法头或截断。不在日志中解引用传入的 context/vtx/hardware 指针。
7. 异常报告只调用读取已发布 TLS 的 getter；不会调用分配 TLS 的函数，不读引擎资源、不做哈希、不加载模块、不获取新的锁。报告中的名称来自有界副本，输出另设 256 字节上限，即使副本终止符被破坏也不作无界字符串读取。未处理 AV 在异常搜索阶段进入现有报告器，不依靠吞掉 AV 或继续执行来保存上下文。
8. 开启并成功 Hook 后保留 StudioRender 模块引用，并固定回调所属模块至进程退出，防止共享虚表留下悬空回调。不提供运行中卸载开关；关闭需要重启。

### 明确的覆盖限制

- 只支持本次 SHA-256 对应 DLL；原版、其他 ThinFlex 版本或运行中已被其他工具修改 `.text` 的版本均拒绝安装。
- 当前 LOD 在所选入口不可可靠取得，始终显示 `unknown`。不为取得 LOD 再挂每顶点/每网格 Hook。
- 名称来源是模型头的 `name[64]`，可以包含 `.mdl` 路径，但无法补出原数据没有的完整路径。64 字节未终止时标为截断；不会猜测目录或附加后缀。
- 模型头本身不可读/被破坏时只能报告指针值及不可用原因。记录某模型正被处理不等同于证明该模型文件本身有错。
- 硬终止、FailFast、堆损坏使现有报告器无法运行，或其他组件再次替换异常处理器时，不能保证产生报告。没有为诊断新增拦截所有异常的全局 VEH。
- Host32/Host64 均不加载这份游戏 x86 模块；Hook 位于共同的 x86 Client，适用于这两种 Host 组合。

## 验证与性能

Windows 原生测试入口：`scripts/test_studio_model_diagnostics.ps1`。测试从固定公开来源下载与用户文件哈希完全一致的 DLL；只以 `DONT_RESOLVE_DLL_REFERENCES` 映射，不调用它的 DllMain、导出或引擎函数，不将该 DLL 纳入构建包/测试结果构件。

测试使用真实生产跟踪、安装、校验和异常报告代码，覆盖嵌套、8 线程隔离、读取失败/截断/转义、C++ 展开、x86 thiscall 参数与栈平衡、bool 返回、LastError、原函数抛异常、已释放模型头、故障线程与正常主线程分离、关闭状态、代码/哈希/槽不匹配拒绝以及一次安装。特别测试原函数 AV 由外层原有 SEH 接住后的 TLS 恢复，以及未处理 AV 穿过实际 Hook 包装仍能归因。故障前禁止该线程 C++ `new`，懒分配会导致用例失败；配合源码检查确认新增报告分支只读快照。异常测试产生的是合成 AV，不是本图实机复现。

性能用批次外层 QPC：每批 4000 次模型级调用，重复 7 轮，比较未 Hook、关闭、开启无输出句柄、开启实际文件写入。没有逐调用计时。它衡量“每次模型构建新增诊断工作”，不能换算成实际 Avg FPS、1% Low 或 0.1% Low。

关闭时模型虚表保持原样，不分配模型 TLS、不写模型日志、不做哈希和内存扫描；只有既有少量 D3D/Device 创建入口的开关判断，以及异常发生后读取空 TLS。TLS 增量是每线程少量标量，未开启线程不分配快照。开启时每个模型增加有界头复制、快照维护及两条事件格式化/WriteFile，不做逐顶点日志，也不逐事件 FlushFileBuffers；现有崩溃路径仍强制刷盘。加载期磁盘较慢或模型数量很多时诊断可能增加加载时间，不能宣称开启后零 FPS 成本。

本分支的完整 Windows 构建、原生测试结果及实测微基准数值在 CI 完成后补入。实际游戏的模型定位效果和帧时间仍需用户实机验证。

## 示例

下面仅展示**合成测试格式**，不是用户游戏崩溃的定位结果：

```text
studio_model event=begin sequence=2 depth=2 model_tid=1234 model="models/fixture/inner.mdl" name_status=available name_source=studiohdr.name[64] lod=unknown context=0x0 studiohdr=0x12340000 vtx=0x0 hardware=0x0
studio_model kind=active available=1 scope=faulting_thread model_tid=1234 sequence=2 depth=2 model="models/fixture/inner.mdl" name_status=available name_source=studiohdr.name[64] lod=unknown context=0x0 studiohdr=0x12340000 vtx=0x0 hardware=0x0
studio_model kind=recent_completed available=0 scope=faulting_thread
```

嵌套 AV 的 inner 没有正常返回，因此不会出现该 `sequence` 的 `event=end`；主线程另一个正常模型不会被拿来替代故障线程上下文。
