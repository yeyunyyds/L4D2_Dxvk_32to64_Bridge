# Runtime / diagnostics separation audit

Audit baseline: `main`, v1.2.0, commit `47c4da5403e3b8f1b1beca0ffa35474be3027394`.
This report was completed before changing runtime implementation. Paths below
refer to the pinned upstream checkout after applying `patches/l4d2-bridge.patch`.

Categories: **A** correctness, **B** unified PageBlock residency/reclaim,
**C** explicitly requested runtime features, **D** observation only. Only D is
made inactive by diagnostic switches. C initialization follows its own feature
settings. Automatic reclaim and LGC/AGC/FGC continue to use the same implementation.

| Subsystem / file and entry | Current enable condition | Work with diagnostics off / classification | Planned separation and disabled state |
| --- | --- | --- | --- |
| Client memory, `client/memory_diagnostics.h::sample`, `allocate`, `released`; `d3d9_swapchain.cpp::Present` | No monitoring switch; 5 s sampling from allocations/Present; forced samples bypass timer | Whole VA `VirtualQuery`, process/CPU queries, mutex, log file, timers and allocation atomics: D. Actual allocation and exceptions: A. | `client.memoryMonitoring=False`: return before clock/lock/file/query; no memory counters. Allocation and failure propagation unchanged. |
| Host memory, `server/main.cpp::sampleHostMemory`, `host_memory_diagnostics.h::Recorder` | Unconditional command-loop and lifecycle calls | File opened on first Recorder access, VA/process/CPU/commit/handle/GPU queries every 5 s: D. | `server.memoryMonitoring=False`: no Recorder construction/access, sampling, GPU adapter setup or counter updates. |
| Host resource inventory, `host_memory_details.h::ResourceInventory`, `main.cpp::gResourceInventory` | Unconditional resource registration/erase | Extra identity map allocation, detail counters: D. `gpD3D*` maps and ownership tables: A. | Disable the diagnostic inventory independently; preserve forwarding/COM ownership maps. |
| GC snapshots, `client/pageblock_control.cpp::L4D2BridgePageBlockControl` | Forced memory sample before/after all sweeps | Whole-process scans: D. GC gate, shared reclaim, skips, bytes and ACK/drain results: B. | Monitoring off makes forced samples no-ops; all GC modes/counters still execute. |
| Exception history, `util/exception_diagnostics.cpp::CallScope`, `CommandScope`, `recordCommand`; `util/util_seh.cpp` | Always tracked; detailed switch only affects report verbosity | API/command TLS context and rings per call: D. Fatal exit codes, bounded fatal reporting/peer handling: A. | New per-side `crashDiagnostics=False` stops history/context accesses. Preserve fatal handler/report and error propagation. Legacy detailed=True requests history when new key is absent. |
| API wait, `util/api_wait_diagnostics.h::Span`, `initialize`, `queued` | Per-side `apiWaitDiagnostics` | Disabled checks precede QPC, TLS stats, atomics, thread/file creation: D already gated. Core waits/timeouts: A. | Preserve implementation; prove disabled entry points perform no instrumentation or initialization. |
| Color, `util/color_diagnostics.*`, `server/color_backend_diagnostics.h::backendSnapshot`, upload call sites | Per-side `colorDiagnostics` | Getters/COM references behind `due`; hashes behind producer checks; diagnostic payload locals remain. D. Wire descriptors/layouts also used for forwarding: A. | Preserve output and Volume path; make Host diagnostic maps lazy/no-op while empty; test no disabled hashing/Getters/files/worker. Guard any remaining producer preparation only when diagnostic-only. |
| Data, `util/data_diagnostics.*::Resource`, `Dispatch`, `Temporary`, `Wire`; Client stateSet call sites | Per-side `dataDiagnostics` | Resource detail allocation, timings, counters, histograms, comparisons already behind enabled checks: D. Empty stack/member tokens remain. Required buffer/Volume storage: A. | Preserve gated implementation and add disabled-work tests, including fake invalid pointers/state inputs. |
| VB/IB memory totals, `client/memory_diagnostics.h`, `lockable_buffer.h::g_totalBufferShadow` | Memory totals unconditional; data histograms separately gated | Memory totals and trace-only shadow total: D. FULL_SHADOW bytes and LockInfo queue: A. | Monitoring/trace gates for totals, no storage or Lock/Unlock contract changes. |
| PageBlock detailed, `client/pageblock_diagnostics.h`, `pagefile_shadow.h::configureDiagnostics` | `client.pageBlockDiagnostics`; detailed modifies enabled output | Record allocation and simulation gated; summary checks used before clock. D. LRU view budget, pinning and actual state/coverage queried by control: B. | Keep pointer/enable early-outs and test no record/file/history updates when disabled. |
| Learned policy logs, `client/retention_runtime.h`, `retention_policy.h::Context::write`, summary | Policy enables log even without diagnostics; unavailable log currently forces KEEP fallback | Resource identification/recovery/promotions write continuously; diagnostic formatting before write early-out. D. Identity hashes, creation-site fingerprint, persistent learned DB and verification: B. | Make detailed file/formatting/summary opt-in via PageBlock diagnostics; remove diagnostic-file dependency from policy eligibility. Keep real warnings/errors and core identities/DB/safety decisions. |
| Residency logs, `client/pageblock_runtime.h`, `pageblock_residency.h` | Detailed events usually diagnostics-gated; GC/config and some skips unconditional | Detailed file/log formatting: D. User sweep result, policy changes/errors: B / required low-frequency logging. | Route mandatory user results/errors to normal log; detailed file only when requested. Avoid disabled coverage formatting/sweeps solely for logging. |
| Queue observation, `util/queue_wait_diagnostics.h`, `util_atomiccircularqueue.h::push`, `waitForWrite` | Unconditional | Diagnostic atomic increments and full-wait timing: D. Wake event, queue indices, waiting flags, timeout clocks/retries: A. | Memory-monitoring enables counters; off skips counter updates/extra full-wait clock. Always execute `SetEvent` and preserve queue order/wake/timeout behavior. |
| Presenter, `server/main.cpp::configureOverlayPresenter`, `overlayPresentWindow`, `overlay_presenter.h` | Window creation already requires presenterWindow + vanilla DXVK + windowed + no Remix API | No window/thread when false. Input=False still runs module scans every second; Steam module polling/logging independent of experiment: D. Presenter resizing/routing/input enabled behavior: C. | Early-outs before input/module polling when input off; Steam module observation only with its diagnostic request. Preserve enabled window/input routing. |
| Message pump / window, `client/window.cpp::init/set/remixMsg`, `di_hook.cpp::AttachConventionalInput` | WndProc and conventional hooks always; Get/PeekMessage conditional | Cursor/key/raw-input detours installed with all optional input switches off: C without demand. Window management, server activity notifications/timeout changes: A. | Keep game WndProc/window/session notifications; skip optional UI processing and input detours with no requested input. Explicit hook requests and Remix mode remain supported. |
| DirectInput, `client/di_hook.cpp::DInputHookAttach`, `DInputHookDetach`, forwarder | Always attach/create dummy DI devices even policies=Never | Detours, DirectInput setup, forwarding state: C without demand. Game's unhooked DirectInput belongs to game. | Demand predicate includes Presenter input, existing hook/forwarding/exclusive requests and Remix mode; fully off returns before detour transaction/library/device/forwarder setup. No forwarding redesign. |
| ReShade binding, `overlay_presenter.h::findReShade`, `bindReShade` | Binding checks m_input; findReShade scans modules first | Module snapshot when Presenter Input=False: D/unnecessary C setup. Enabled addon callbacks: C. | Check input/window before snapshot; no addon binding or capture when input off. Enabled input discovery remains. |
| Steam experiments, `client/window.cpp::sampleSteamInput`, `util/steam_activity_diagnostics.h`, Presenter native capture | Client experiment/diagnostic gates; Host Steam module observation currently unconditional with Presenter | Client diagnostic helper constructed before early-out; Host module polling with experiment off. D. Explicit Steam capture/native focus: C. | Early check before helper creation; no disabled polling/events/atomics. Separate observation log from explicitly requested capture behavior. |
| API logging, `client/d3d9_util.h::LogFunctionCall`, `d3d9_lss.cpp::FunctionEntryExitLogger` | logApiCalls/logAllCalls checked inside constructor | std::string parameter constructed before check: D; exception scope independently active. | const char* input / lazy logging gate; no string allocation, formatting, logging counters/locks when off. Crash scope controlled separately. |
| Server command logging, `server/main.cpp`, `module_processing.cpp`, `util_bridgecommand.cpp` | logServerCommands/logAllCommands at producer | Command strings already built inside flags: D gated. Actual opcode dispatch/UIDs: A. | Preserve existing producer checks and prove strings are not evaluated off. |
| General debug/trace logs, `Logger::debug/trace` call sites | Logger filters level after caller formatted arguments | Formatting/allocations before logger filter; trace-only buffer totals: D. Warnings/errors/lifecycle Info: required low-frequency logs. | Lazy producer checks for trace/debug, including stringstream/command-name setup; compile-time command profiling also requires Trace. Keep enabled text and error/lifecycle logs. |
| Queue dump, `PrintRecentCommandHistory`, Bridge::Command::print_* | Shutdown/failure paths, not continuous | Copies queues/formats histories on ordinary exit: D. No history ring writes here; queues are A. | Crash-history gate for optional dumps, no queue changes. |
| Tracy, `bridge/meson.build`, util_common.h | enable_tracy=False by default | Zone macros compile to no-op; no profiler worker in standard build. D already compile-time gated. | Retain default compile gate, test/inspect build settings. No new build product. |
| L4N control, `client/bridge_control.cpp`, `bridge_settings.h`, `pageblock_control.cpp` | Export calls; startup target snapshot only | No periodic plugin/config polling. Configuration editing/status/GC only when called: C/B. | Preserve passive ABI v1/v2/general ABI and optional plugin; no menu/source changes. |

## Boundaries retained

- `PagefileShadow::s_mappedBytes`, mapped-view cost, locks/transfers, shared residency
  gate, capability/recovery/policy decisions and Host ACK/drain synchronization remain.
  The single-region `VirtualQuery` measuring an actual mapped view is intrinsic GC
  mapped-VA accounting, not a whole-process diagnostic scan.
- Learned content hashes, creation-site fingerprints and retention DB are policy
  identity/safety work, not color/data diagnostic hashes. They remain when the
  learned policy requires them. Readback verification hashes remain unchanged.
- QPC used for unique recovery names and requested GC ACK/drain result timing is
  retained. Timeout clocks, waits, retries, queue events and required command UIDs
  remain. Removing these would change correctness or the existing control ABI.
- Fatal exception/peer termination still emits a bounded fault-time report. Turning
  off crash history removes continuous healthy-path tracking, not fatal handling.
- The Volume implementation, byte pitches, layout/wire code and existing production
  regression tests will remain byte-for-byte unchanged. Diagnostic callees can
  reject disabled work without touching those paths.

## Verification plan

Add production-path disabled tests with intercepted Win32 operations for diagnostic
scans/clocks/files/threads/hooks/Getters and allocation/counter/history assertions.
Run enabled diagnostic suites as positive controls and the unchanged Volume suite.
Run unified policy/GC/recovery, buffer contract, Reset and queue regressions on both
Host architectures. Check full native Windows builds. Report actual operation
counts, never an inferred FPS benefit.
