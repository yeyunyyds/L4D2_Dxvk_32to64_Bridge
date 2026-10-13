# Steam IME thread bootstrap experiment

Status: native verification pending; real Sogou/Microsoft IME untested.
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
