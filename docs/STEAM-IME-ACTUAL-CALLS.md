# Steam actual IMM call observer

Status: Windows native ABI/original-call tests and full build PASS; game IME validation PENDING.
The latest supplied 5ba86729 sessions confirm polling flag True, bootstrap False,
and actual ime-pass-through for Sogou. User reports unchanged failure. Host has
23 focused samples, changing Chinese/English HKLs, a context in all187 samples,
but open0/conversion-query failure and no observed composition/result. These
observations do not prove Steam's actual context or complete absence of events:
its hooks may consume messages. Bootstrap and polling have not fixed the issue.

This observer changes no arguments, call thread, return values or last error.
It observes named imports only in the already loaded Steam renderer, preserving
its stored original chain exactly once: Create/Destroy/Associate/GetContext,
GetConversionStatus/GetOpenStatus/SetOpenStatus/GetKeyboardLayout/GetProperty.
Context handles remain inside the same process and are only logged, never sent
to the other process or dereferenced. No composition contents are read or logged.
No forced open/conversion or cross-thread context replacement is attempted.

Events identify caller thread, target window owner PID/thread, Presenter thread,
context, argument/return and layout. GetKeyboardLayout argument is requested
thread ID; GetProperty argument is the IGP selector, layout_argument its input.
SetOpenStatus value packs requested bool in bit1 and returned bool in bit0.
Other values are unmodified result bits. AssociateContext NULL return can mean
no prior context; a stale LastError does not prove failure. Creation/association
thread comparison and actual window PID are evidence, not inferred ownership.

Identical observations are limited to once per five seconds per API/thread/callsite; changed
states are retained. All output is bounded at4096 lines. A limit record signals
incomplete coverage. Contention can drop observations and a128-site limit record signals incomplete
callsite coverage. Context handle reuse must not be inferred as permanent ownership.
Dynamic lookup/direct code and imports absent from the
renderer are not covered; attachment mask/import records report actual coverage.

The fixture verifies x86/x64 API signatures, exactly one original call, arguments,
outputs, true/false/noncanonical BOOL and LastError, disabled mode, bounded logs
and exact IAT restoration. It does not use the user's DLL or a real IME engine.

Use the new matched three-binary update, preserving the working Steam/Presenter
configuration. Merge config/STEAM-IME-ACTUAL-CALLS.conf and restart. Both ineffective
behavior switches are False; only call/context diagnostics are enabled. Open
Overlay, click the text input field, switch Chinese/English, attempt brief input,
close Overlay and collect both bridge logs. No need to repeat bootstrap A/B.
Main/Release and Exit Game behavior remain unchanged.

## Build and delivery evidence

Source: `746c003818a8185099170468dab57daa18603903`.
Build ID: `l4d2-1.3.1+af9077d98ae12293`.
[Windows build and native tests](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38471423460)
completed successfully. The x86 and x64 fixture each passed disabled and enabled
modes, preserving actual original arguments, results, output buffers and LastError.
The full existing native regression suite and three Bridge binaries also built
successfully. These tests do not establish real Chinese composition support.

The initially stalled duplicate run38470376378 was cancelled; replacement
run38471423460 subsequently executed and passed. Release publication was skipped.

[Matched three-binary diagnostic ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/7aab9ed4fa3adcbae658edc545333d1347b94bec/steam-ime-actual-calls-r38471423460/l4d2-bridge-steam-ime-actual-calls-af9077d98ae12293-r38471423460.zip)

SHA-256: `2f8a5329828ca84001028737696163871ab52c783dd4b465d06af804e8cff2d9`.

Archive digest, manifest, matched build IDs and PE architectures were verified,
and the immutable uploaded bytes were downloaded again and compared. Only the
experimental download branch and this development branch were updated. Main,
formal release, protocol semantics and Exit Game behavior remain unchanged.
