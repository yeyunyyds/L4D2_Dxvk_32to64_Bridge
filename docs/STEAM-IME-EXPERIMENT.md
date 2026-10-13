# Steam IME thread bootstrap experiment

Status: Windows native verification PASS; real Sogou/Microsoft IME untested.
Independent branch; VERSION remains 1.3.1 and published assets/main are untouched.
Patch SHA changes the matched binary build ID, distinguishing the experiment.

A: server.steamImeBootstrap=False, server.steamImeDiagnostics=True.
B: server.steamImeBootstrap=True, server.steamImeDiagnostics=True.
Merge config/STEAM-IME-EXPERIMENT.conf into the known-working Steam configuration.
Restart both processes between A/B. Preserve Presenter, Steam native input, IPC,
backends, ReShade and resource settings. Defaults for both new flags are False.

The Presenter owner thread optionally initializes COM STA before window creation,
restores the system default IMM context only when initially absent, and refreshes
its layout from the game thread when opening Steam. Existing Steam IMM contexts
are never replaced on activation. Windows/Steam retain composition, candidate and
Unicode delivery; no synthesized text, forced Chinese mode or foreign HIMC is used.
COM success/S_FALSE is balanced on the same worker, after window destruction.
Imm/COM APIs are resolved from System32 DLLs with lifetime bounded to that worker.
Diagnostics run on the owner thread once per second and on transitions, recording
layout, context existence, open/conversion status and message counts, never text.
WH_GETMESSAGE/Steam may consume messages before our queue observation, so zero
counters do not prove no Steam-side IME activity.

Native fixtures verify owner-thread bounds, disabled behavior, COM lifecycle,
LastError, no character contents, counter semantics and callback failure safety.
Existing x86/x64 cross-process Presenter/Steam regressions and production builds
are also required. These tests do not establish real IME support, candidate window
visibility or correctness with Sogou. No performance improvement is claimed.

Native fixture caveat: querying an actual IMM context can activate Windows TSF,
which may retain its own COM apartment reference. The fixture verifies the Bridge
COM pair separately before context activation; it does not require system-owned
IME/TSF references to disappear while their thread/window remain alive.

## Native evidence and experimental package

[Windows run38453713243](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38453713243)
passed for source `edca3a3e07524e1cb0e2541fc825b1b50bc3f3ef`. This includes
x86/x64 IME owner-thread/COM/counter tests, all three actual Bridge builds and
existing full-IPC/Steam/window/resource regressions. Publication was skipped.
The earlier run38452690592 failed the overly broad COM-state assertion, not a
passed release gate; the final control separates the Bridge pair from system
initialization following context access. Real IME engines were not tested.

[Matched experimental update](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/18ff2531b9c5e977f56ec41dab46436b7c66b196/steam-ime-experiment-r38453713243/l4d2-bridge-steam-ime-experiment-74bec765ec9bfa94-r38453713243.zip)

- Build: `l4d2-1.3.1+74bec765ec9bfa94`
- ZIP SHA256: `7920a706abb8b6b02ecdc5fd04cdef225c169d3ca0670f387115f4e9f09d2712`

Exit game/Hosts, back up and replace all three Bridge binaries together. Preserve
DXVK, existing runtime configuration, ReShade, plugin and DB. Merge only the
experimental snippet into the real runtime conf. Test B (bootstrap True), then
A (False) on the same binaries with full restarts; diagnostics remain True.
Observe Chinese composition/commit, candidate visibility, reopening, English,
backspace, and ordinary Steam closing/game input. Send paired bridge logs after
a short failing sequence; neither the counters nor logs contain composed text.
Main and formal v1.3.1 assets remain unchanged. This package is not a Release.
