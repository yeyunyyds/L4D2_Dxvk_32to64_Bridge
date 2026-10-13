# Steam actual IMM call observer

Status: implementation prepared; native verification and game evidence pending.
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
