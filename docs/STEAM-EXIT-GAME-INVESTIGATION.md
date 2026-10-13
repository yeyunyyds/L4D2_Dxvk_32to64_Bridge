# Steam Overlay Exit Game investigation

Status: OPEN; no exit behavior change in the IME experiment.

The user reports that the Overlay Exit Game action produces a Bridge crash dialog.
Both supplied sessions run matched 74bec765ec9bfa94 experimental binaries; bootstrap
True/False is the only declared difference. Both Client logs record Host exitcode1
followed by the CPU crash dialog. The second Host log stops while Client remains
active and lacks a normal Bridge_Terminate/cleanup or exception record. Client
still attempts a VB Lock after Host exit. Therefore this is not evidence of a
normal Client shutdown being misclassified: the rendering Host exits first.

OnServerExited correctly treats an unplanned Host exit while Client is not
DoneProcessing as failure. Only a paired explicit termination handshake suppresses
the normal report. Host exitcode1 alone is not a trustworthy quit-intent marker;
blanket suppression would hide backend failures with that code. The provided
Steam Renderer imports ExitProcess/TerminateProcess, but static imports and absent
logs do not prove the exact killer or call path. No unique root cause is claimed.

Candidate next evidence: correlate the explicit Steam quit request with Host
ExitProcess/TerminateProcess or window-close delivery, distinguishing in-process
calls from Steam's external process termination. Fix the request target/order so
Client starts ordinary shutdown and releases the Host, or use a verified explicit
quit marker/handshake. Preserve genuine GPU/CPU exception reporting and abrupt peer
failure tests; do not reinterpret every exitcode1 or active Overlay as normal quit.

IME remains the primary task. No quit shortcut, automatic exit relay or report
suppression is included without verified intent evidence. Raw user logs are not
committed and the public 1.3.1 release remains untouched.
