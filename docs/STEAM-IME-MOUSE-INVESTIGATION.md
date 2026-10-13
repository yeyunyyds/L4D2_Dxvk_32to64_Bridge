# Steam mouse and IME investigation after v1.3.1

Status: OPEN / no production fix or new gameplay claim.
Baseline release source: 9e3ee1292f901b0bb9352e36bba93a4cd5cfbda4;
production patch SHA256: 3d13bf31676468b81984b9d68360a19f74693d2d521b73dc44a717de4ea7cc26.

## User reports

Direct Shift+Tab from active gameplay leaves the cursor at screen center, while
background view does not rotate. Earlier menu/chat scenarios did not establish
this transition's correctness. Whether Esc/chat before opening resolves it and
whether ReShade is involved are pending clarification.
All input methods reportedly produce English in Overlay; Sogou is in use and
native DXVK works. No paired logs of these failures have yet been supplied.

## Source evidence and limits

- Client callback331 sets the independent Steam UI-active source. Aggregate
  transitions call DI begin/end neutralization. Game RawInput deltas and DI state
  are neutralized; lack of camera movement alone does not identify mouse owner.
- di_hook.cpp HookedSetCursorPos avoids OS warps while UI-active. HookedGetCursorPos
  returns a virtual game position. SetCapture is gated and release runs on the
  game window thread when Steam opens. These are distinct from cursor clipping.
- No ClipCursor hook or clipping release exists in the production Bridge source.
  A persistent narrow clip or reacquisition is a candidate, not a confirmed cause.
  DirectInput exclusive acquisition/cooperative state is another candidate.
  Imported APIs from other modules and downstream hooks also require evidence.
- Host Presenter takes focus for native Steam input and passes real OS events in
  the focused case. It copies game keyboard layout only once at creation. Its
  default WndProc and TranslateMessage remain intact; absence of explicit WM_IME
  handling does not prove missing Windows IME support.
- Provided Steam Renderer64 imports IMM composition/context APIs. InputFix
  b23b71d27aace0a2e193e79779fb254f3cb76c0c instead accepts character-only LWJGL
  events (key==0) in Minecraft's GUI; it is not a native IME-context fix.

## First evidence using existing v1.3.1 binaries

Merge only config/STEAM-IME-MOUSE-DIAGNOSTICS.conf into the running configuration,
then restart the game and Host. Do not use the historical
STEAM-INPUT-DIAGNOSTICS.conf, which explicitly disables Steam capture/native input.
Keep the known-working Presenter/Steam configuration unchanged.

Record a short same-session sequence: active gameplay; direct Shift+Tab;
move/click/type in Overlay; close; verify game mouse; Esc/chat and repeat.
Collect bridge32.log, bridge64.log and effective bridge.conf. Existing bounded
state logs include global clip dimensions, foreground/local focus, GUI capture,
capture transitions and immediate SetCursorPos/SetCapture suppression callers.
Client cursor/hit observations may use the virtualized GetCursorPos path; do not
interpret them as physical cursor movement. Host state provides a separate view.
Existing counters do not record text and do not yet distinguish IME messages.

## Next instrumentation and candidate criteria

If existing logs cannot distinguish the branches, add default-off observations
for clipping requests and original API results, focus transitions and owned
thread IME layout/context/open state, plus WM_IME/WM_CHAR counts only. Never log
text/composition contents or pass HIMC pointers across processes. Keep query
and effect work on the appropriate owning thread and preserve LastError.

A mouse candidate must be limited to known active Steam and the game foreground
root, release only game-owned constraints, preserve Host/Steam capture and handle
repeated clipping/warps. Closing, Alt+Tab, focus loss, failures and repeated
open/close must not restore stale constraints or steal another application's
cursor. Distinguish viewport clipping from a one-pixel center clip before acting.
Do not force DI Unacquire or globally ClipCursor(NULL) without ownership evidence.
IME work separately distinguishes no composition generated from generated text
being dropped, and preserves native Steam processing and Unicode commit exactly
once. These are investigation criteria, not a tested implementation.

No IPC, backend, resource, default config, released ZIP or v1.3.1 asset changes.
