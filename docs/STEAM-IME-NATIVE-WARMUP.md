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
