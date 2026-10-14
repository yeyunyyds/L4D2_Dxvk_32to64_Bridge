# Steam IME full-chain experiment

Status: Windows x86/x64 native tests and full build PASS; Chinese gameplay input PENDING.
Main, VERSION, release assets, resource/IPC semantics and Exit Game unchanged.
All new production options default False. Preserve the currently working Steam
Presenter and native input configuration; do not install an older config file.

## Why multiple configurations

The actual-call diagnostic confirms Steam creates/associates its context on the
Presenter owning thread, but its first conversion query fails before association
and focus handoff. This downranks a foreign-thread HIMC explanation for that
sequence. It does not prove a TSF failure. Chinese/English HKL changes alone do
not prove an active text input processor. The user reports no candidate or
composition UI, and English-only typing with every IME attempted.

Previous bootstrap initialized COM, not explicit TSF activation. Previous Client
polling exemption alone failed; dependencies between Host activation and Client
IME polling were never tested together. Keep those experiments unaccepted.

One matched three-binary package contains four snippets. Merge exactly one into
existing bridge.conf, replacing previous occurrences; stop game AND Host between
runs. Each run takes only one Overlay text field test, not map-loading benchmarks.

| Run | Explicit TSF activation | Client IME polling exemption | TSF GetMessage |
| --- | --- | --- | --- |
| A-OBSERVE | False | False | False |
| B-TSF | True | False | False |
| C-TSF-POLL | True | True | False |
| D-TSF-PUMP | True | False | True |

Suggested order: B, A, C, D. Try switching to Chinese, type a short sequence,
switch to English, then Chinese again. Note candidate window, committed Chinese,
and whether Shift+Tab/mouse/close remain normal. Save paired bridge logs per run
before restart. Avoid logging/chatting passwords. No need to change Presenter,
ReShade, DXVK, IPC or InputFix. Diagnostic content contains no typed text.
If an experiment disrupts input, revert to A or set all three new flags False.
The installed formal matching package remains the complete binary rollback.

## New observation points

`L4D2_STEAM_IME_CHAIN` records at most once per second and at focus boundaries:
- current existing TSF manager, activation flags and document focus presence;
- existing COM apartment, HKL and ImmIsIME result, default IME window PID/thread;
- GUI active/focus/capture window, parent and child enabled/style state;
- bounded module-name inventory (.ime and known Windows text service modules),
  at bind and when focused layout changes. Missing names do not prove no engine:
  TSF TIPs can be DLLs, out of process, or loaded later;
- classified counters for ordinary keys, ASCII/non-ASCII characters, dead chars,
  composition start/update/result/end, IME context/control, language and focus.

Counters distinguish WH_GETMESSAGE removal, the message returned to the loop,
WH_CALLWNDPROC entry, Bridge's own WndProc, after DispatchMessage, and
WH_CALLWNDPROCRET. They neither store nor log virtual keys, character values or
composition strings. Sent messages may occur without a queue counter. Other
windows on the Presenter thread are counted explicitly. Hook order still matters:
an earlier hook may change a message before ours, or intercept WndProc entry.
Missing counts never prove absent original OS events. TranslateMessage can
produce later messages, so dispatch-return is not a post-translation text count.
The after-dispatch stage counts dispatch attempts, not acceptance by Steam.
No hook handles a negative hook code, changes a message or skips the next hook.

Read-only TSF observation uses a previously loaded MSCTF and TF_GetThreadMgr,
not creation/activation. Existing IMM diagnostics may themselves cause Windows
TSF initialization; snapshots are current state, not pristine pre-IMM state.
Presence of a TSF manager/document and nonzero flags are evidence, not proof that
Steam's text field has a working IME. ImmIsIME False is possible for modern TSF
profiles and must not be misclassified as English-only by itself.

State/count output is capped at1800 lines per Presenter; module inventory at256
selected module lines plus bounded summaries. Limits explicitly mark incomplete
coverage. The existing Steam IAT observer has its separate4096-line/site bounds.
It now also observes named ImmGetCompositionStringW/A imports and records only
index and signed return length/status (encoded in pointer-width value), never
buffer contents. Successful conversion queries include conversion/sentence
flags. Original calls, arguments, buffers, returns and LastError are preserved.
Dynamic resolutions/direct calls remain outside named-IAT coverage.

## Optional behavior, isolated from observation

B explicitly initializes COM STA on the Presenter thread before its window is
created, obtains/creates a TSF thread manager and calls Activate once. Only its
own successful Activate is balanced by Deactivate on the same thread, after
window teardown. Its COM reference is independently balanced. Existing managers
are reused and their own activations are not removed. Apartment/init failure
skips activation; no manager or foreign HIMC is forced onto Steam's window.
No text store/document manager, profile or composition is created/replaced.

D additionally queries the TSF message pump and uses its Unicode GetMessage.
If initialization/QI fails it retains the ordinary pump and logs that outcome.
A runtime TSF pump failure is logged and stops the Presenter loop; no retry can
consume a second message or hide the failure. Native shutdown/quit semantics
must be tested before delivery. This path remains experimental, not a default.

C combines B with the previously tested narrow Client polling policy: known IME
modules may poll while Steam UI capture is active; game/MOD callers remain
suppressed. This does not broaden raw input, DirectInput or setters. Its prior
standalone failure is preserved rather than presented as an accepted fix.

## Native gate and interpretation

Tests must run separately in native x86 and x64 processes: disabled/observe,
activation, TSF pump, and conflicting MTA apartment. Verify actual Activate and
own Deactivate, original queue arguments and WM_QUIT, hook dispatch arguments/
return, LastError isolation and foreign-thread refusal. Synthetic counter inputs
only test observation classification; they do not establish IME composition.
Expanded IAT fixture verifies composition-buffer effects, negative return status,
conversion output flags and exact original-call count with tracing on/off.
Full existing native regression suite and all three production binaries required.
No user's Steam DLL or IME binary is executed or redistributed in the tests.

If B repairs input, isolate explicit activation; if only C does, investigate
interaction with Client IME polling. If only D does, isolate TSF pump behavior.
If none does, compare stage counts and TSF focus with actual Steam IMM results;
do not fake successful queries or force open status to conceal the fault.
A gameplay result, not CI activation success, decides whether any path is useful.

Primary API sources:
- https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-tf_getthreadmgr
- https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-tf_createthreadmgr
- https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfthreadmgr-activate
- https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfmessagepump-getmessagew

## Verified delivery

Source: `7d6bdecf5df3c22fa72fc87dac5881ea70729eee`.
Build: `l4d2-1.3.1+8682d963820507a3`.
[Native tests and full Windows build](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38500284525)
completed successfully. Five chain modes passed independently on each of x86
and x64 (ten successful native invocations). Expanded original-call fixtures
passed enabled/disabled on each architecture (four invocations). Three actual
Bridge binaries and the existing production regressions passed. The frozen
v1.3.0 negative-control failures are intentional comparison evidence, not current
candidate failures. No real IME composition was exercised by CI.

[Matched experimental ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/18983c5d23fb7d799ba02e9a89e7cd4586a2697d/steam-ime-chain-r38500284525/l4d2-bridge-steam-ime-chain-8682d963820507a3-r38500284525.zip)

SHA-256: `6e23f88b254ce5e6349bda17473adae3e5908d48961a0d4c4d8738df1e594cd4`.

CI archive digest, manifest, binary architecture/build IDs and immutable public
download bytes were verified. The archive is unchanged CI output; its design
document was written before the build, while this section records the later
verification. Main, Tags and formal Release were not changed. Native PASS is not
a gameplay repair claim. Keep per-case paired logs and user-visible outcomes.

## Supplied B session: ineffective despite successful activation

Both logs identify `l4d2-1.3.1+8682d963820507a3` and the intended B flags:
TSF Activate True, TSF pump False, Client IME polling pass-through False,
old bootstrap False; all chain/actual-call observers enabled. The user reports
unchanged failure. Do not describe B as an accepted repair.

Presenter-thread COM, manager acquisition and TSF Activate all return S_OK;
active_flags remain0x80000000. The Host loads SogouTSF.ime, SogouPy.ime,
IMM32, MSCTF and textinputframework. Actual window focus/capture is the Host
Presenter. Thus missing TSF activation, missing loaded Sogou modules and failure
to obtain Win32 focus do not explain this B result by themselves.

Steam still creates/associates its HIMC on the Presenter thread, but the fresh
context conversion query returns FALSE before association. All observed TSF
samples have no document focus and a NULL default IMM window. These are useful
signals for window/legacy-IMM interoperation tests, not proof of a single cause:
legacy applications may legitimately have no TSF document focus.

During the capture interval, each GETMESSAGE/queue-return/dispatch-return stage
records only3 key messages and1 ASCII character; Bridge's WndProc records1 of
each. No stage records composition/start/result/end or Unicode characters.
Sent-message hook stages record6 language events and focus acquisition; an IME
SETCONTEXT event appears when capture is released. There is no observer limit
record. Counts do not establish the total actual input delivered to Steam:
Steam may consume keyboard events before these queue hooks or use another path.
No composition-string retrieval was recorded through the named Steam imports.
Do not interpret absent records as proof that OS composition never occurred.

Client records SogouPy.ime GetKeyState as poll-chain-neutralize on opening.
The immediate next supplied-package comparison is C: change only
client.steamImePollingPassThrough=True relative to B and fully restart. Prior
polling-only failure does not resolve this combined condition. D then isolates
TSF message-pump behavior; A remains the non-activation baseline. No new binary
or default enablement is warranted while these comparisons remain outstanding.

Own TSF Deactivate returns S_OK and both peers cleanly terminate in this session;
it does not reproduce the independently tracked Steam Exit Game error.
