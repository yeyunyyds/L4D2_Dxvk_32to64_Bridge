# Experimental Presenter IME native initialization

User probe build0a6dc9a2a93f reproduced the English-only failure without Steam.
Case9 PID11100 and12 PID20464 create a default IMM window but never establish
focused TSF document/composition; their direct windows receive31/20 ASCII and
zero Unicode/results. Case10 PID17292 briefly focuses hidden EDIT, restores
Presenter and retains EDIT:7 results and14 Unicode. Case11 PID12592 does the
same but destroys EDIT before typing:2 results and5 Unicode on the original
Presenter. The user confirms10/11 work and9/12 fail. This supports one-time
focus initialization plus helper destruction, not default IMM HWND creation
alone. It does not prove equivalent behavior under actual Steam hooks.

`server.steamImeNativeWarmup` defaults false. Merge the supplied experimental
config with existing working Steam settings and install all three matched
binaries, then restart both processes. First Steam capture in foreground,
after the existing native path focuses Presenter, attempts initialization on
its existing owning thread: create a hidden1x1 EDIT, SetFocus(helper), verify
actual focus, restore Presenter if still foreground and Steam capturing, destroy
helper. One attempt per Presenter window, reset upon worker recreation; no
per-frame work, retries, new thread, message pump, text relay or forced input
mode. No retained EDIT or changed Client window/IPC layout/resource path.

A single `event=native-warmup` receipt records owning thread, creation/focus/
restoration/destruction and creation error. Detailed IME diagnostics remain
optional and bounded. Failure does not abort rendering or loop; turn the new
option off to compare existing input behavior, or restore all old matched
binaries. Normal Presenter destruction also cleans up any failed helper destroy.
The existing explicit TSF activation is enabled in this experiment as in the
successful independent matrix. Its steady-state impact is not yet measured.

Native cross-process Presenter tests exercise background/ReShade non-activation,
foreground Steam trigger, focus restoration, destroyed helper, repeated open
without repeated initialization, closure and existing presentation/capture
behavior for both Host architectures. Other IME/Steam capture native gates and
all three Release compilations are required. They do not exercise real Sogou,
Steam private hooks, gameplay, TSF restart or arbitrary input methods.

Real gate: foreground in-game Shift+Tab -> chat Chinese candidate/commit, close
and reopen, Chinese/English switching, invite UI, Workshop URL, Alt+Tab return,
and ReShade coexistence. Keep the known renderer hash rejection separate from
IME validation; this change does not expand the verified web-adapter allowlist.
After testing send paired logs and symptoms. Do not declare Steam IME fixed or
merge before this game validation. Main, VERSION, Tags and Releases unchanged.

## Verified experimental delivery

Source `39ee5168e69d3931e141cc2d82e478e6d4f0f853`; matched Bridge `l4d2-1.3.1+31cae0cdcb96c4ec`.
[Windows run38514887546](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38514887546)
passed IME chain/call/session, capture and production Presenter regressions, then
compiled all three Release binaries with matched protocol/build identities.
Production Presenter cross-process fixtures passed both Host architectures,
including helper focus/restore/destruction and one attempt across repeated
Steam capture. Formal publication was skipped. Full unchanged IPC/gameplay
regressions are not claimed as rerun in this targeted build.

[Matched experimental update ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/6f30918cbf375767dc3fd4dba9734b38467986b2/steam-ime-warmup-r38514887546/l4d2-bridge-steam-ime-warmup-31cae0cdcb96c4ec-r38514887546.zip)

SHA256 `825873fb2dc2bd563bb5c34b2535e8259b14330b8b7f9f5efac423a16d5693b1`. Native and Bridge CI artifact digests, core build
receipt/source/PE architecture/LAA checks, binary identities and checksum
manifests were verified before repacking; every member was checked against
the CI archive and the public immutable ZIP was downloaded and reverified.
Merge config/STEAM-IME-NATIVE-WARMUP.conf rather than overwrite existing config.
Game Steam IME validation was pending at delivery; see the subsequent real-game
result below. Main and formal releases remain unchanged.


## Real-game result: PASS for tested x64 Steam IME session

On2026-10-14 the user tested matched build
`l4d2-1.3.1+31cae0cdcb96c4ec` and confirmed Chinese input works with no issue.
Client/Host logs match that identity, warmup=True, session
`ec8f5d9b-4064-4f8f-8846-fa365503bca5`, Host PID9860, Presenter thread17292. The test session lasts66 seconds and contains one observed
Steam capture interval.

Exactly one native-warmup receipt records created=1, focused=1, restored=1,
destroyed=1, create_error=0. Afterward, focused TSF document and default IMM
window are present. Sent-message observers see6 composition-result events;
Steam's named-IAT ImmGetCompositionStringW(GCS_RESULTSTR) observer independently
records6 nonempty results (byte lengths6,4,2,4,6,2). Both sent hook stages see
the same events; these are6 results, not12. No entered text is retained.
The original Presenter WndProc has no composition records because these
observations occur in the hook chain; successful Steam API reads and the user's
visible result are the positive evidence. Both peers perform normal Terminate
handshake and clean exit.

Mark Steam Overlay Chinese IME **PASS for this tested x64 session**, not universal
IME/Host/gameplay coverage. Repeated reopen, layout changes, x86 Host, other
input methods, invite and web actions are not independently established by
this single capture interval. The renderer-file-hash-mismatch gate is still
logged and is a separate web/invite-adapter issue; it does not invalidate the
observed successful IME result. Main/Release remain unchanged.

For ordinary use keep server.steamImeNativeWarmup=True and
server.steamImeTsfActivate=True, with legacy bootstrap and special TSF pump
False. Diagnostic flags may be disabled after validation; do not measure
steady-state performance with the verbose experimental observers enabled.
