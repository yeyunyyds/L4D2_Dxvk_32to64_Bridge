# Steam callback capture and invitation input experiment

Status: BLOCKED — user gameplay test failed invitation interaction and closed-state Shift+Tab; independent native tests, full build and package checks PASS.
Branch: `codex/steam-invite-availability`. No main, VERSION, tag or release change.

## Verified matched experiment

- [Download Steam capture/invitation/ReShade experiment](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/eeef417be4993e78a4409f120cdcfbb195e946e1/l4d2-bridge-steam-capture-invite-reshade-dev-0901d263de181431-r38327953766.zip).
- Build `ldb-ipc-c32-dev+0901d263de181431`; ReShade `ldb-reshade-input-dev+0901d263de181431`.
- Compiled and packaging source `817e7d1b9029acf7f537f3395394b57e713dc3df`.
- ZIP: 5,071,792 bytes, SHA-256 `bdf6a8b22373ad781e6dd4e5d3cfaec870d0af74a8326d6f190742fcd8b56dc6`.
- [Full Windows native gate/build](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38327953766): PASS.
- [Permanent raw/native/package/upload evidence](steam-input-evidence/38327953766/).

Both architecture runs passed actual three-owner capture/activation helpers,
owner-thread/worker/stale/no-window notification cases and 30,000 concurrent
open/close pairs. Existing 13 native suites, four input/status architecture pairs,
252 production transport cases, 30 failure cases, 39 Python checks and compiled
ReShade32/64 export/mapping/build probes passed. Offline verification confirms
all five delivered binaries equal the native originals, actual named getter words,
PE bitness, Release receipts, manifests, notices and all packaged hashes. The ZIP
was appended to the existing download branch and fetched again for SHA validation.
Synthetic tests do not establish real Steam consumption or closed-state Shift+Tab.

## Paired hardware evidence

### Matched 0901 capture experiment: gameplay failed

The next supplied logs both report `ldb-ipc-c32-dev+0901d263de181431`,
complete the handshake and shut down cleanly. At 18:02:33.078 the verified
GameUI helper issues `LobbyInvite`; at 18:02:33.107 callback331 reports active1.
At 18:02:33.108 the actual new capture record reports source_capture1 and
aggregate_capture1, and subsequent Client samples retain capture1. The user
explicitly reports that invitation mouse/keyboard interaction still fails and
Shift+Tab still cannot open the Overlay. Capture propagation is verified;
real Steam input acceptance is FAILED. No close callback appears in these logs.
See [sanitized hardware result](steam-input-evidence/38327953766/capture-paired-input-hardware-result.json).

The Host window observer remains False and the Client observer was not enabled.
Consequently no actual renderer window-query or hook-install evidence is available,
and zero Host counters must not be interpreted as zero Steam calls. Existing input
message counts do not prove those messages reached Steam's input transport.

Source inspection confirms that `selectWindow()` requires a valid Host-owned
Presenter child to substitute a game-owned HWND. With Presenter False the adapter
returns the original HWND; enabling `server.steamWindowAdapter` alone does not
create an input window. `server.steamNativeInput` controls the existing Presenter
implementation and supplies no separate input relay when that object is absent.
This limitation is a hypothesis for the remaining failure, not a runtime-proven
sole cause. Enable both window observers on the same matched package before
choosing a transport change. Previous user-reported Presenter-False success for
other Overlay actions remains recorded and is not disproved by this later test.

### Follow-up 0901 run: renderer selection mismatch observed

Both window observers are now True. The matched 0901 Client and Host handshake
and exit cleanly. Client PID18540 owns game HWND0x2A0B08/thread22748; Host PID24240
owns no Presenter. On Host rendering thread24056 the actual renderer return sites
0xA6AD4 and 0xA6AE2 repeatedly report NULL active followed by the game foreground
HWND, owner PID18540, same_process0 and result unchanged. This persists after the
18:19:23.579 invitation request and 18:19:23.607 callback/capture transition.
Before that request the Client observes Shift+Tab keydown counts7 and5, without
an intervening activation callback. Client capture after invitation is1.

Host renderer timestamp0x6A98CE0D, image size0x1B7000 and the return sites match
the uploaded statically analyzed DLL. That sample rejects a foreign-process
foreground candidate before input window binding. Full loaded-file identity and
the actual conditional branch are not traced here; the observed selection inputs
are nevertheless direct runtime evidence supporting that specific failure path.
Observer summaries are Client active0/foreground130/hooks0 and Host
active26/foreground194/hooks0, dropped0. Hook counts cover only this named import
after attachment; they do not exclude pre-existing or dynamically resolved hooks.
See [sanitized window result](steam-input-evidence/38327953766/window-paired-input-hardware-result.json).

The next comparison requires no new binary: restart with the same matched set,
keep the window adapter and both observers enabled, and enable
server.presenterWindow, server.presenterInput and server.steamNativeInput.
For this comparison server.reshadeExternalInput must be False: the existing
external original-window mode explicitly refuses Presenter True. Preserve all
other settings, test closed-state Shift+Tab and invitation interaction, then
compare mapped Host-owned selection and actual hook-install results. This is a
temporary architecture control, not a claim that ReShade needs to revert or that
Steam input will necessarily work. If it restores real input, investigate
separating Steam's process-owned input binding from the ReShade presentation path;
an independent input-only anchor is not implemented or validated by this audit.

### Previous b0 capture omission

The earlier supplied Client/Host logs share one session and both report
`ldb-ipc-c32-dev+b0a628e02609a4b4`; they start and shut down cleanly.
The effective config has Presenter/input/message-pump/custom-hooks all False,
Steam input/chain/poll/mouse/proxy/adapter True, and no explicit external-ReShade
input option. Client diagnostics are enabled; `server.steamWindowDiagnostics`
is still False despite `server.steamInputDiagnostics=True`.

The Client records multiple Shift+Tab keydowns, but no activation callback after
those attempts. At 17:15:05.528 it records the exact GameUI `0x11a03` guard:
original False, returned True, fresh Host ready. The same thread calls
`ActivateGameOverlay("LobbyInvite")` once and the SDK returns. At 17:15:05.570 a
real callback331 publishes active1. Subsequent Client samples still have capture0,
with actual mouse/key messages reaching the game window. The user reports a
visible invitation panel that cannot be operated or closed.

This confirms the invitation availability/request path and actual activation
notification. The message-pump False comparison has not restored input, so do
not identify that flag as the sole cause. The source only updated the cached
Steam flag and refreshed ReShade; without the old Presenter relay it never added
Steam to `RemixState` capture. A real notification and a missing capture state
are a concrete Bridge state-propagation omission. This is not yet proof that
correcting capture alone supplies Steam's proprietary input transport or fixes
closed-state Shift+Tab.

The Host's zero window-query/hook counters were recorded with that observer off;
they do not prove no renderer calls occurred. The Host's zero Bridge WndProc
message counters without a Presenter also do not enumerate Steam's input hooks.
The Client's downstream WndProc identifies L4D2Evolved.dll; a single hook head
does not prove the presence or absence of the whole Steam hook chain.

## Minimal behavior change

1. Give SDK Steam an independent capture bit4, alongside legacy UI bit1 and
   external ReShade bit2. Real callback331 calls `setSteam(active)` only when
   `client.steamOverlayInput=True`. Duplicate events do not repeat aggregate
   activation/deactivation. Closing one source cannot clear another.
2. The existing aggregate transition performs conventional gameplay input
   suppression/DirectInput cooperative handling. Retained Steam query/hook-chain
   passthrough remains active, and the existing game-owned mouse capture handoff
   can now satisfy its UI-active predicate without a Presenter.
3. Request the capture handoff on the game window owner. Callback on the owner
   performs it directly; a different callback thread posts one local registered
   window notification on a real inactive-to-active edge. Its consumer checks the
   latest Steam state, so an old notification after deactivation does not release
   newly restored game capture. No new thread, polling timer, synthetic shortcut,
   public activation API call or Host-owned window is added.
4. Add default-off `client.steamWindowDiagnostics` to observe the already loaded
   32-bit renderer's same three named USER32 imports. Client mode never adapts
   windows; original queries/hooks run once with original arguments/LastError.
   The existing Host observer/adapter remains independent. Both include an
   explicit role field; the retained `host_pid` field names the observer process,
   even on Client records. Named-IAT coverage excludes dynamic resolution.

Observers pin their own code, retain renderer references and save prior API targets, then restore
only their own IAT replacements on stop. There is no per-command IPC change or
new shared mapping. Normal key/mouse messages bypass notification lookup using
the registered-message range; notification registration occurs only if an actual
cross-thread handoff needs it. Observer polling is absent when its option is off.
No measured CPU/FPS improvement is claimed.

## Required independent checks

Extract and compile the actual RemixState and actual activation/notification
helpers, with authored transition/cursor effects. Test three-owner overlap,
duplicate completion, balanced transitions, disabled external mode/no Presenter,
real owner-thread versus worker notification, stale delivery, no window and a
retained ReShade suspension model, plus 30,000 concurrent open/close pairs.
Authored cursor effects do not claim actual Steam/DirectInput acceptance.
Keep the existing actual input-hook, same-architecture USER32/IAT-chain and
cross-architecture window/status tests, invitation thiscall ABI, full IPC and
ReShade DLL gates. Supplied proprietary DLLs are not executed or distributed.

## Installation and test

Exit game/Host and replace all three Bridge binaries and both custom ReShade DLLs
from one matched archive. Keep backend, presets and the working config. Merge
`config/STEAM-INVITE-INPUT-EXPERIMENT.conf` by replacing existing values, not
appending conflicting duplicates. In particular, enable **both**
`client.steamWindowDiagnostics=True` and `server.steamWindowDiagnostics=True` for
the diagnostic run; `server.steamInputDiagnostics` is a different option.
The snippet preserves the user's current ReShade mode rather than changing it.

Test Shift+Tab from closed Overlay, then the lobby Invite Friends action and
mouse/keyboard/close. A successful invitation capture should show callback active1
followed by `steam-capture ... source_capture=1 aggregate_capture=1` and input
state capture1. Actual close callback must retire only Steam capture.
An input-capture transition is not proof the panel consumes input. Client and Host
renderer records are needed to locate a remaining native window/hook path failure.
Recheck View Workshop, Join Server, normal gameplay control after close, and
accepted ReShade Home/mouse/capture coexistence. Verify an invited friend receives
the invitation and can join before calling the full feature passed.

Retain both Bridge logs and actual config. Turn diagnostic flags off after the
run. If closed-state Shift+Tab or panel interaction remains broken, report it;
do not declare this state-only fix a complete input implementation.
Rollback restores the full backed-up matched Bridge/ReShade set and its config.
