# Steam GameUI invitation availability experiment

Status: IMPLEMENTED; native build and gameplay validation pending.
Branch: `codex/steam-invite-availability`. No main merge, VERSION or release change.

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
