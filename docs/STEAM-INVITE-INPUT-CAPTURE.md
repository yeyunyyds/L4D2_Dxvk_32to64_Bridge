# Steam callback capture and invitation input experiment

Status: IMPLEMENTED; native build and gameplay validation pending.
Branch: `codex/steam-invite-availability`. No main, VERSION, tag or release change.

## Paired hardware evidence

The latest supplied Client/Host logs share one session and both report
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

Observers pin their own code and retain renderer/target lifetime, then restore
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
