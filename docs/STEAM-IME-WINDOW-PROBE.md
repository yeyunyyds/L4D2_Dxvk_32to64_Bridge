# Independent IME window probe and early-hook diagnostic

Status: source prepared; native compilation/tests and actual IME validation pending.
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
