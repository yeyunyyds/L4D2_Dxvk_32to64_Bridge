# Steam-only integration into main (unreleased)

Integrate the Steam-only experiment and shared L4N Presenter description on top
of main `c8d55b0`, including PR11's close-tail, reply-budget and upload-failure
correctness fixes. Keep main's full IPC v2, six batch sizes, default C8, VA arena,
optional mapped Shadow, resource lifecycle and existing graphics backend behavior.
Do not replace them with the earlier C32 experimental transport.

## Production changes

Main already contains the renderer window adapter, native Presenter input and
Workshop availability proxy. Port the remaining Steam changes from tested source
`e3e66652a9b031f5508e11f926a9adad4e8bc8dd`:

- Extend the full-hash-verified GameUI availability allowlist with the generic
  overlay helper return site0x11A03, covering LobbyInvite. Preserve the real
  original SDK request, borrowed argument and thiscall ABI exactly once.
- Real callback331 owns its own capture source. Presenter/legacy notifications
  own another, with serialized aggregate transitions and balanced DirectInput
  effects. Either source may close first without prematurely restoring input.
- The activation helper performs owner-thread mouse handoff or posts one local
  registered notification. Stale/duplicate callbacks cannot recapture the game.
- Add default-off Client renderer named-IAT observation, explicit Client/Host
  roles and pinned observer code. Window substitution remains Host-only and uses
  a genuine Host-owned child. No PID rewriting or Steam binary patch is added.

Only six production patch sections change: window.cpp, remix_state.h/.cpp,
steam_web_diagnostics.h, steam_workshop_status.h and steam_window_diagnostics.h.
All126 other existing main sections remain byte-for-byte intact. No ReShade
original-window experiment, custom ReShade DLL, external input mapping, layer
environment activation or vendor dependency is integrated.

## Configuration and L4N

The user-validated Steam-only setup needs `server.presenterWindow=True` and
Presenter/native input plus window adaptation. Main's fresh-install reference
now enables that window; optional message-pump/custom-hook flags stay False.
The Steam fix snippet uses the same settings and keeps diagnostics False.
Existing installation configs are retained; merge the snippet and restart game
and Host to apply. The raw SDK capture source alone cannot supply a native Steam
input window when Presenter is disabled.

L4N labels the setting **ReShade / Steam Overlay** in both languages. Its page
describes the shared input dependency; disabling displays the Steam input loss
notice. Keep the existing configuration operations and numerical action IDs,
Host/IPC/Shadow menus, language behavior and old-interface fallbacks. The native
plugin fixture checks both localized descriptions and disabled-state feedback.

## Evidence and publication boundary

The 0901 Presenter control had paired runtime logs showing mapped Host windows,
successful input hooks, four open/close cycles and restored focus/capture, plus
the user's "全部正常" report. The stripped2DA41 experiment then received the
single-setting report: Presenter True works, changing only Presenter False makes
Steam input fail. Those tests establish the experimental setup's acceptance;
they do not automatically validate this new main/full-IPC combination in game.

Required gate: full Windows main build, actual full-IPC/PR11 regressions,
two-architecture Steam capture/window/status/request ABI tests, compiled L4N
callbacks/localization/fallbacks and existing resource/native checks. Local
40 Python contracts and all six extracted input-hook modes pass.

This is a source integration, not a release. VERSION remains1.3.0; no Tag,
Release, prerelease or release asset is created or updated. Native build outputs
may be retained as ordinary CI artifacts. The previous immutable experimental
ZIP remains unchanged. Real-game regression of the new integrated main build
is still pending and no FPS or memory improvement is claimed.

## Completed native validation

[Windows run38341029534](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38341029534)
passed all build and regression steps for source
`e6db372f8a03fb8e13142d94c5e51bea3394b8cb`; the publication job was skipped.
This includes x86 Client, x86/x64 Host, the x86 L4N DLL, full IPC and PR11 safety,
Steam capture/window/request ABI, bilingual menu callbacks, resources and
cross-process input tests. Artifact manifests and all listed binary SHA-256
values were independently verified, including matching embedded Bridge IDs
and the new compiled L4N labels. No custom ReShade DLL is packaged.

Build ID: `l4d2-1.3.0+3d13bf31676468b8`. CI archive SHA-256:

- Bridge: `7c34ed8b0d30d19a7fd47fc3618d2433ee253aa8f904b53b014195923271b7df`
- L4N: `501553806d65b0b711081b5e337c42cf95f74529245eaef2d3a73ceb1a589681`

The archives remain CI artifacts; this validation does not publish a Release.
Real-game validation of the integrated main composition remains pending.
