# Independent IME window probe and early-hook diagnostic

Status: x86/x64 native tests and full Bridge build PASS; actual IME composition PENDING.
A/B/C/D all failed Chinese typing. B/C show the Chinese/English taskbar mode
indicator; A/D do not. B/C/D load Sogou and activate TSF; D also obtains the pump.
Activation alone and that pump do not restore Steam composition. There are no
observed composition messages or default IMM window in the captured Presenter
samples, and few ordinary key events reach our queue observers. This motivates
window/IME interoperability and early-hook comparisons, not another forced mode.

## Independent test: game and Steam Overlay closed

Extract the probe folder into a writable directory and launch Start-Probe.cmd
(or ImeWindowProbe32.exe). It creates its own x86 parent and launches its own
x64 child; it neither attaches to the game nor loads/patches Steam. Six buttons:

1. Host-owned x64 top-level window.
2. Same-process x64 child window.
3. x86 parent with x64 child, enabled at creation.
4. Same cross-process child, initially disabled then enabled (Presenter style).
5. Case4 with a hidden Host-owned top-level window created beforehand.
6. Case4 without explicit Bridge TSF activation.

Click each case, select Chinese and type pinyin into Custom Unicode first.
Record candidate UI and whether committed Chinese is shown. Optionally click
Use fresh HIMC to reproduce Steam-style context replacement, try typing, then
Restore original HIMC. This changes only the probe-owned field, never the game,
and does not force open/conversion. It pairs owned creation/destruction and
restores the original association before close. THEN click Add
Native EDIT, try that field and the Custom field again. A native EDIT can itself
initialize text services: the log samples before/after its creation, and it is
not pre-created to contaminate the first Custom comparison. The custom field
uses Unicode WM_CHAR and DefWindowProc for IME handling, with a normal caret;
it does not implement Steam's private text field. Neither field stores typed
text in the logs. Do not paste as a substitute for input-method composition.

Try cases1 and4 first. If behavior differs, cases2/3 isolate parent ownership and
initial disabled style; case5 tests an initialization-anchor hypothesis, and
case6 isolates explicit activation. No hypothesis is claimed as a repair.
Select the next case to close the prior owned child, or close the launcher.
Only these created probe processes can be terminated after a bounded cleanup
wait; no game/Steam/other process is targeted. The parent HWND/PID/class is
validated before cross-process child creation. No taskbar state spoofing.

Logs are written next to the probe executables in logs/ as
probe-caseN-32-or-64-pPID.log. Send that folder with a short results table:
case, Custom before EDIT, fresh HIMC/after restore if tested, Native EDIT,
Custom after EDIT, taskbar mode indicator.
Logs contain window/process/thread IDs, layout/context/service states and message
counts, not keys, characters or composition contents. Text visible in the fields
is private process memory, bounded and discarded when the case is closed.

An anchor changing results is evidence of per-thread initialization, not proof
that the production renderer should gain a window. Native EDIT success with
Custom failure implicates custom text input interoperability; it does not prove
Steam would work with an EDIT child. Cross-process failure with owned-window
success narrows the window relationship. CI only tests synthetic Unicode
handling and lifecycle; real Sogou/Microsoft IME composition requires this run.

## Optional matched Bridge early-hook pass

The separate bridge update preserves working Steam/Presenter settings and all
IPC/backend/resource behavior. Replace all three matching binaries. Merge
config/STEAM-IME-EARLY-HOOK.conf into the working config; restart both processes.
This uses B's explicit activation and ordinary message pump, not D's failed
pump. No new input behavior is introduced by the counters.

The existing Bridge WH_KEYBOARD_LL hook now aggregates metadata: callbacks seen,
native-path CallNext count/nonzero result, posted events, Bridge-owned blocking,
other next-hook count/nonzero and OS-shortcut/injected bypass counts. It retains
original branches, exactly one CallNext in the same conditions, original result
and LastError. It records no virtual keys or text, installs no new low-level
hook, and observes only the foreground game already eligible for that hook.

`native_next_nonzero` means a downstream hook consumed an event, not that Steam
is necessarily the consumer or defective. Other input-method/security hooks can
consume events legitimately. Zero seen events can reflect earlier hooks; it is
not proof of absent physical input. Hook-installation target/module records are
also enabled, preserving Steam's procedure/module/thread arguments unchanged.
Thread-level message observers retain their ordering/coverage limitations.

One brief Overlay typing session plus paired bridge logs should suffice. Record
candidate appearance and English/Chinese results. These observations should
choose the next repair boundary; no default enablement or successful fix is
claimed. Main, VERSION, Tags and formal release are unchanged.

## Native gates

Both probe PE architectures and embedded source IDs must match. Test owned top-
level/same-process controls on x86/x64, then x86-parent/x86-child and x86-parent/
x64-child for four cross-process cases. Assert control ownership, enablement,
Unicode synthetic dispatch, owned fresh HIMC replacement/restoration/destruction
and bounded close/exit. No real IME, Steam DLL or
uploaded third-party binary is executed or redistributed by these tests.
Full Bridge regressions and matched three-binary build remain mandatory before
shipping the optional early-hook update. Diagnostic limits remain explicit.

## Verified experimental delivery

Source: `ad3893e6661a3ad05ae0bad113fa9674ba4318a6`.
Bridge build: `l4d2-1.3.1+b839021167aac481`.
Probe build: `ime-window-probe+ad3893e6661a`.
[Successful Windows native build](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38505071449).

Twelve independent probe process pairs/runs passed: owned top-level and same-
process child on both architectures; four cross-process cases with x86 parent
and each of x86/x64 child. Synthetic Unicode, fresh context association/restore/
destruction, ownership and bounded exit were checked. The extended chain modes,
original-call fixtures, existing Bridge regressions and all three Bridge binaries
also passed. These tests do not exercise real Sogou composition or Steam text.

Earlier run38504396718 caught the probe Unicode cursor parameter and command-line
macro quoting; run38504628041 compiled both architectures but exposed a null
exit-code read in the PowerShell test driver. The final run uses an explicit
Unicode resource, forced-include identity header and retained native Process
handle. Failed/partial builds were never delivered as passing artifacts.

[Combined independent-probe and matched Bridge update ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/aa9f9f03a4de3c01f19e093d27299614d69ff5cc/steam-ime-window-probe-r38505071449/l4d2-bridge-ime-window-probe-b839021167aac481-r38505071449.zip)

SHA-256: `5d5604b50314bed27c6628d62a994bc7ccd801a9ba3f97aed183d2dc7b679ded`.

Both CI archive digests, checksum manifests, PE architectures and source IDs
were verified before combination. Every combined file was compared byte-for-byte
with its validated original archive, and the immutable public ZIP was downloaded
again and checked. The ZIP contains IME-Probe/, bridge-update/ and START-HERE.md.
The archive design documents were written before the build; this section records
the completed validation. Main, VERSION, Tags and formal Release remain unchanged.


## Direct Presenter follow-up (cases 7 and 8)

User logs for case1 PID2276 and case4 PID14792 (probe ad3893e6661a)
show real composition/result/Unicode delivery to Custom before Native EDIT was
created, both with original and fresh HIMC. Case4 therefore demonstrates that
cross-process parenting and initial WS_DISABLED alone do not prevent IME.
The Custom control and caret remain different from production Presenter.

Matched Bridge b839021167aac481, session
8e9a80f7-4d9f-4987-a7ab-e195fef4a053 had diagnostics disabled. Subsequent
21:01:55--21:02:45 logs (Client15080, Host13728) enable diagnostics. The existing
Bridge LL hook observed98 callbacks, native-next86, nonzero-next0, block0.
Thread stage counters show no composition/result/Unicode, despite native focus,
TSF activation and Chinese layout. Default IMM window and focused TSF document
remain absent. Fresh Steam context conversion query fails. Downstream zero
returns do not exclude earlier hooks, thread-hook consumption, or TSF filtering.
Renderer hash rejection is a separate verified-web-adapter gate, not proven IME
causality. No game repair claim follows from these records.

Cases7/8 bind both observers to the Presenter-like root itself: no Custom,
STATIC, BUTTON, EDIT, caret (case7), hidden top-level or Host shell exists at
initial typing. The registered Unicode class has no class cursor/background;
window uses WS_CHILD|WS_VISIBLE|WS_DISABLED with WS_EX_NOACTIVATE and
WS_EX_NOPARENTNOTIFY, then is enabled/focused on its owning thread. Case8 adds
only a caret at initial focus. Both explicitly activate TSF with the ordinary
message pump. The launcher is still an owned x86 probe window, not the real game;
Steam, DXVK, game-parent IME policy and private Steam controls are not reproduced.
Painting and Unicode echo remain for visibility; this is not exact production
behavior and cannot eliminate those integration differences by itself.

Close the game; extract and run Start-Probe.cmd. Select **7**, click the panel
and type Chinese pinyin before pressing any function key. Repeat in a new
process with **8**. Record candidate appearance and committed text for each.
Within each process: **F2** toggles fresh/original HIMC; **F3** toggles the caret;
**F4** creates Native EDIT and focuses it. Test each step, then click the panel
to retry direct input. Do initial tests first: later EDIT initialization may
change the thread's IME environment. F2/F3 auto-repeat is ignored. Logs contain
phase markers and counters, never entered text. Return case7/case8 logs and
results. No Bridge installation or configuration change is needed.

Native gates additionally assert direct input HWND equals root, no warm-up
controls exist before testing, caret state changes/restores, fresh association
restores and both Unicode targets work. These synthetic gates cannot establish
real composition correctness; real user testing remains necessary.


## Verified direct-window delivery

Probe source: `c2f393240c7dc28402108f96d5534da0bf438505`. Build: `ime-window-probe+c2f393240c7d`.
[Windows native run38510273048](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38510273048) passed.
Sixteen runs/pairs cover all eight cases with x86/x64 child processes; synthetic
Unicode, direct target identity, no initial warm-up controls, context restore,
caret toggles and bounded close passed. Full Bridge build and release publication
jobs are skipped. The original full run38509827802 passed the integrated native
regressions and probe packaging, then was cancelled during unrelated frozen
upload regressions because the production patch is unchanged. It is not
represented as a completed passing full build.

[Standalone direct Presenter ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/c72430652b51368fcae5c986e8f54a4c6fcb55ce/steam-ime-direct-probe-r38510273048/l4d2-ime-direct-presenter-probe-r38510273048.zip)

SHA256: `b1abe6a21bff6ff0ab7d97ef55829ddc9387fda28a0f90adc1fe7cb1248ded79`. Both PE architectures, embedded source IDs,
manifest SHA256s, original CI archive digest and combined member bytes were
verified; public immutable ZIP was downloaded again and its SHA256 checked.
Real IME composition for cases7/8 is pending. This ZIP contains only independent
tools, not replacement Bridge binaries. Main, VERSION and releases are untouched.


## Hidden EDIT initialization follow-up (cases 9--12)

Real case7 PID22116 logs (probe c2f393240c7d) show5 ASCII chars with no
composition before F4. After native EDIT creation and focus, default IMM window
appears and conversion queries succeed; after returning to the original direct
window, its own WndProc receives3 composition results and5 Unicode chars.
The user confirms both inputs now accept Chinese. This establishes a working
intervention, not which part of creation/focus or visibility is necessary.

Each new mode starts a fresh owned child process, explicitly activates TSF,
uses the same direct Presenter-style window with no caret, and performs one
initialization sequence before user typing:

* **9**: create a1x1 EDIT child without WS_VISIBLE; retain it; never focus it.
* **10**: create the hidden EDIT, synchronously attempt focus, then immediately
  restore the direct window's focus; retain the hidden control.
* **11**: same as10, then destroy the EDIT before user typing.
* **12**: create the hidden EDIT, destroy it immediately, never focus it.

No forced Chinese/open mode, user text forwarding, caret, extra process, thread,
backend or keyboard hook is introduced. Log actual focus acquisition/restoration
rather than assuming hidden focus succeeds. No helper message pump or waits are
added; normal observation starts after initialization. A missing/disabled IME
engine or deferred initialization can still cause failure. This is an owned
probe, not game production integration.

Close game and run Start-Probe.cmd in a new extracted directory. First repeat7
as negative control and4 as positive control; then test9,10,11,12 separately.
Click the direct white panel, switch Chinese and type pinyin before F2/F3/F4.
Record candidate/committed text and send the logs. F4 later shows the retained
EDIT in9/10 or creates a visible one in11/12, providing the previous positive
intervention. Only do this after the initial typing test. All entered text stays
in memory. Native tests cover24 runs/pairs, hidden state, ownership, destruction,
context restoration, synthetic Unicode and bounded close; real IME remains a
manual gate before production adoption.


## Verified hidden EDIT matrix delivery

Source `0a6dc9a2a93fe679b39327407ec65d498bc49626`, probe `ime-window-probe+0a6dc9a2a93f`.
[Windows run38514052045](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38514052045)
passed native compilation and24 synthetic runs/pairs; production build and
release publication jobs were skipped. Native archive contains24 process logs;
cases10/11 both architectures record hidden-focus acquired=1 and restored=1,
cases11/12 record helper destruction, and9/10 retain a hidden helper until
the subsequent synthetic EDIT test. No real IME composition is claimed.
Earlier run38513913726 passed the three-combination version; the delivered
source completes the fourth create/destroy-without-focus control.

[Hidden EDIT matrix ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/7c47ef819df00b6af454c00d63be4a9550cc7b88/steam-ime-hidden-edit-probe-r38514052045/l4d2-ime-hidden-edit-probe-r38514052045.zip)

SHA256 `8e4125561c021f9c57dc50b358022185585cfad85fd5f78200fd9c106705591e`. Probe and native CI archive digests, PE architecture,
embedded source IDs, manifest checksums and packaged byte identity were checked.
The immutable public ZIP was downloaded again and verified. Test9--12 initial
input before F4. Real user validation is pending before production integration.
Main, VERSION, production patch and formal releases remain unchanged.
