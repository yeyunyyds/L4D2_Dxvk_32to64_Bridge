# Steam GameUI invitation availability experiment

Status: AWAITING INVITATION INPUT VALIDATION. Latest user report: the invitation
panel opens, but cannot be controlled. Fresh paired interaction logs are pending.
Supplied earlier failure logs establish
experimental Client + formal 1.3.0 Host mixing and an explicit handshake rejection.
Native tests, full build and package verification remain PASS; actual L4D2 startup
with the full matched set and invitation delivery are still unverified.
Branch: `codex/steam-invite-availability`. No main merge, VERSION or release change.

## Invitation panel opens, input remains unverified

Latest report: "可以呼出邀请界面。但是控制不了邀请界面". The user additionally
confirms ordinary Shift+Tab does not open the Overlay and neither mouse nor
keyboard can interact with or close the invitation panel in this session.
Investigate the common Steam input path, not just the invitation command. This is partial
functional evidence that the availability/request path can open the panel; it
does not establish mouse/keyboard operation or invitation receipt/joining.
The supplied config enables all retained Steam options but has
`client.hookMessagePump=True`, unlike the previously accepted False setup, and
no explicit external-ReShade input option. It disables input/window/web
diagnostics. Do not infer a missing Steam callback, capture failure or wrong
request PID without a fresh paired run.

Next controlled comparison: set only `client.hookMessagePump=False` as the
functional change; enable `client.steamInputDiagnostics`,
`server.steamWindowDiagnostics` and `client.steamWebDiagnostics` for the run.
Keep the current Presenter/ReShade/Steam options otherwise unchanged. Compare
ordinary Shift+Tab with the lobby invitation panel in the same session, then
retain both logs. Existing diagnostics report callback331, aggregate capture,
focus/capture HWNDs, message counts, suppression callers and Host hook results.
Do not force UI capture solely on an SDK activation method returning void.

## Reported startup regression

After delivery of `b0a628e02609a4b4`, the user reported: "游戏都打不开了".
Subsequently supplied Client/Host logs have the same session identifier. Client
reports `ldb-ipc-c32-dev+b0a628e02609a4b4`; Host reports
`l4d2-1.3.0+7c1982f257ea333b` and explicitly exits because their versions differ.
The Host exits with code 1 before backend initialization, ACK, or the invitation
observer. Thus these logs do not demonstrate a startup regression in the invitation
hook. The Client then faults with `0xc0000005`, read address `0x8`, at bridge RVA
`0xcc295` while handling startup failure; this secondary failure is retained as a
separate unresolved failure-path issue, not used as evidence of Steam hook failure.

The effective config also lacks an explicit `server.reshadeExternalInput=True`
and has `client.hookMessagePump=True`, unlike the accepted external-input run.
Restore the required mode flags when testing the full matched set. Static package
comparison found identical loader metadata and no new dependency. This does not
substitute for a successful matched-install gameplay run.

Retain the failed session logs before rollback. Restore the complete user-accepted
`8fffd697a6a95120` Bridge/ReShade set together; keep its known working config.
The original-window ReShade pass is retained independently. Before retesting
invitation, replace all three Bridge binaries and both custom ReShade DLLs from
the same archive and verify both logs report the same experimental build. This
record does not blame a particular installer or updater; none has been evidenced.

## Verified experimental download

- Build: `ldb-ipc-c32-dev+b0a628e02609a4b4`; matching ReShade:
  `ldb-reshade-input-dev+b0a628e02609a4b4`.
- Compiled and packaging source: `6705e3086f4550ece1b43f5cd30ee8d508be66c3`.
- [Download the matched invitation/ReShade ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/e2d6b576bf216675d57f59564f1b10db4159e9df/l4d2-bridge-steam-invite-reshade-dev-b0a628e02609a4b4-r38312406560.zip), 5,061,797 bytes.
- SHA-256: `5aef2b624e02b957781cb98ba858c6d17730cc57c18dd8b5e304e77385889c29`.
- [Full Windows native gate and build](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38312406560): PASS.
- [Permanent native/raw transport/binary/upload evidence](steam-input-evidence/38312406560/).

The invitation ABI test, 13 existing native suites, four status/input architecture
pairs, 252 production transport cases, 30 failure cases, 39 Python contracts, and
both compiled ReShade DLL ABI probes passed. Offline verification independently
checked all five binaries against the original native outputs, actual named build
getter words, PE bitness, Release `/O2`+`NDEBUG` receipts, local-layer manifests,
license files and every packaged file hash. The append-only ZIP upload was
downloaded again and its SHA-256 checked. Supplied GameUI/Steam DLLs were not
executed or bundled. Native synthetic LobbyInvite events in the retained test log
are authored ABI fixtures, not real L4D2 invitation evidence.

## Evidence and minimal repair

The user accepted the original-window ReShade repair but reports that the lobby
Invite Friends button displays the Steam Community In-Game requirement.
Earlier Steam acceptance covers Shift+Tab, mouse, Join Server and View Workshop;
it did not establish invitation support.

Static inspection of the previously supplied, unexecuted x86 GameUI sample
SHA-256 `321ec9b5865582330af61e81eb347da31cc213cc6c95b80d6d98170b8a5fbb40`
locates the screenshot's localization tokens in helper RVA `0x119e0`.
Lobby handlers at `0x4f9ff`, `0x5f0f6`, and `0x68b8a` pass `LobbyInvite` to it.
It invokes SteamUtils010 slot17, returning to `0x11a03`. False branches to
`0x11a20`, displays that warning and skips SteamFriends017 slot28
(`ActivateGameOverlay`), whose request would return to `0x11a1a`.
The supplied Steam API flat thunk at `0x2010` independently confirms its x86
thiscall ABI: one borrowed string argument, virtual offset `0x70`.

The previous proxy accepted only `0x199ea`, `0x19aca`, and `0x6ea9a`. The new
patch adds only `0x11a03`, gated by the same full GameUI file hash. This is its
shared generic Overlay helper, including LobbyInvite; unrelated caller sites
retain the SDK result. This establishes a coverage omission matching the warning,
not the actual runtime value during the failed click or subsequent routing.

Reuse `client.steamWorkshopAvailabilityProxy=True`, the existing 16-byte
session-specific Host status mapping and Present sampling. False becomes True
only at a verified guard with a live Host and available state within the existing
three-second lease. Unknown/stale/future/dead Host stays False. The original SDK
predicate is always called exactly once. No new thread, window, periodic observer,
render IPC command or global forced True is added.

Observe the existing Friends017 slot28 method, preserve its this pointer, stack
ABI, borrowed argument, LastError and one original invocation. Log only a bounded
dialog class (`lobby-invite` or `other`), never invitee IDs or private text.
The Bridge does not duplicate or forward an invitation request to the Host.
Conditional restoration and pinned callback/target lifetime remain intact.

This adds one allowlist comparison and a rare activation observer, without a new
per-frame cross-process operation. No measured performance/FPS benefit is claimed.
All existing Steam input, Workshop and ReShade implementations are retained.

## Installation and real-machine test

Exit game/Host, back up the working config and binaries, then replace **all three
Bridge binaries and both custom ReShade DLLs together**, preserving architecture
directories. Their common source fingerprint changes with the Bridge patch;
older experimental ReShade DLLs cannot be mixed with this Bridge.
Keep the working backend, presets, textures and effects; they are not bundled.

Keep current config, with:

```ini
client.steamWorkshopAvailabilityProxy = True
server.reshadeExternalInput = True
server.presenterWindow = False
server.presenterInput = False
client.hookMessagePump = False
```

Keep the previously working Steam input switches enabled.
`client.steamWebDiagnostics=True` is optional; the proxy already enables bounded
availability and activation records.

1. Enter a lobby, wait for Overlay availability, click Invite Friends and verify
   the friend-selection panel visibly opens.
2. Send one invitation to a consenting friend; verify receipt and joining.
   SDK return or an open panel alone does not prove invitation delivery.
3. Repeat after panel reopen and a map/lobby transition.
4. Recheck Shift+Tab/mouse, Join Server, View Workshop, ReShade Home/mouse,
   closing-panel control restoration and switching between the overlays.
5. Exit normally and retain both Bridge logs; add ReShade.log for input regressions.

Expected: attachment `overlay017=1`; availability at `caller_rva=0x11a03`
(`original=0 returned=1 host_ready=1` if substitution is needed); then
`L4D2_STEAM_INVITE event=request ... dialog_class=lobby-invite ... verified_helper=1`.
`event=return` proves SDK return, not visible UI or invitation delivery.
A warning without a request needs availability/context evidence; a request
without UI needs routing evidence. Do not infer that Host forwarding is necessary.

Disable the existing proxy and restart to revert both invitation and Workshop
substitutions. To retain the accepted Workshop repair while reverting this change,
restore the complete backed-up `8fffd697a6a95120` Bridge/ReShade set and config.

## Required native checks

All four scoped guard sites; x86 thiscall this/stack/borrowed arguments; one SDK
invocation; unknown/stale/future/disabled refusal; unrelated callers; bounded
diagnostics; throwing logger/LastError; retained callbacks; conditional restoration.
Retain all cross-bitness Host status, transport and ReShade suites. Supplied
proprietary DLLs are statically inspected only, never executed or redistributed.
