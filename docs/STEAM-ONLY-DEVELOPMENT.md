# Steam-only Presenter development package

Status: EXPERIMENTAL / AWAITING GAME VALIDATION; this stripped binary needs
its own gameplay test. Native build and package results are retained with the
workflow and BUILD-INFO receipt. Branch `codex/steam-only-experiment`. main, VERSION, tags,
releases and the retained combined experiment branch are not changed.

## Scope

Based on compiled production source817e7d1 and the user-accepted matched0901
Presenter control, remove the new original-window ReShade input experiment from
production. Removed: Client input encoder, cross-process input mapping/wire,
Host custom ReShade ABI binding and local Vulkan-layer environment activation,
external ReShade capture source, its cursor/raw-input helpers and vendor build
identity dependencies. The package contains only three matched Bridge binaries,
with no ReShade DLL, experimental layer JSON, vendor source or shader/preset.

Keep the IPC batch options, D3D9/resource/Surface correctness paths, memory policy,
ordinary backend calls, Steam window adapter, native Presenter input, independent
callback331/Presenter capture ownership, scoped Workshop and LobbyInvite checks,
and opt-in two-process diagnostics. Steam capture and Presenter notifications
may close in either order; the retained serialized two-source state prevents one
notification from releasing the other's capture prematurely.

The old optional Presenter/ReShade compatibility code remains as part of the
baseline. Removing the new experimental path does not disable independently
installed standard Vulkan layers. Their coexistence is not newly validated here.
No FPS or memory improvement is inferred from removing an optional path.

## Install and test

1. Exit L4D2 and both Host variants. Back up the existing three Bridge binaries,
   working `bin/.l4d2bridge/bridge.conf` and any prior experimental input directory.
2. Replace `bin/dxvk_d3d9.dll`, `bin/.l4d2bridge/L4D2Bridge32.exe` and
   `bin/.l4d2bridge/L4D2Bridge64.exe` together from this archive. Never mix builds.
3. Move the previous `bin/.l4d2bridge/reshade-external` directory to your backup
   outside the game install. This package neither needs nor supplies those custom
   DLLs. Keep the working DXVK backend, retention DB and unrelated settings.
4. Merge `config/STEAM-ONLY-EXPERIMENT.conf` into the actual working config,
   replacing old values. The supplied complete `config/bridge.conf` is a reference,
   not an instruction to overwrite your working settings. Steam options, Presenter
   window/input, native input and adapter are True; default Host is x64. Preserve
   your chosen IPC batch; the reference default remains C1. Remove the old
   `server.reshadeExternalInput` key: there is no handler for that feature now.
5. Restart and check both logs show the same new build ID. Test Shift+Tab opening
   and closing, invitation mouse/keyboard/send/close, normal game controls after
   close, Workshop and Join Server. Repeat and verify the friend receives the
   invitation and joins if full invitation delivery is part of acceptance.

Normal diagnostic flags are False. If a regression occurs, temporarily enable
client/server.steamInputDiagnostics, client/server.steamWindowDiagnostics and
client.steamWebDiagnostics, then restart and collect paired logs. Expect a genuine
Host-owned mapped window, successful input-hook handles and capture/focus release.

Rollback restores the complete backed-up matched binary set and config; restore
the experimental directory as well if returning to that prior ReShade mode.

## Verification gates

The build checks prepared production for removed experiment markers and required
Steam paths. Native tests compile actual capture and callback/notification helpers
on x86/x64, with both close orders, duplicate/stale/no-window cases and30,000
concurrent open/close pairs. Existing cross-process IPC/fault, resource/reset,
input hook, real window adapter, Steam availability/request thiscall ABI and
status-lifecycle regressions remain required. All three Bridge binaries compile
in Release /O2 with NDEBUG and share one build fingerprint; x86 Host retains LAA.

Synthetic/native checks do not establish real Steam or GPU/game acceptance for
this newly stripped build. The prior0901 user acceptance applies to the control
that still contained the disabled ReShade code, not automatically to this package.
