# Session logs and peer error dialog

Bridge-owned common logs use one directory per Client/Host run:

```text
<game root>/L4D2BridgeLogs/<session GUID>/
```

The directory contains `session-info.txt`, the Client and selected Host main log, optional API wait,
data and memory diagnostics, and deterministic `exception-client.txt` / `exception-host.txt` reports.
The Client creates the directory before its logger starts and is the sole owner of startup retention.
`logging.sessionRetention` defaults to 10 total sessions and is clamped to 1–100. Cleanup failure never
blocks Bridge startup. Older Color, PageBlock, readback, retention and Sentry diagnostic locations are
unchanged.

When one peer terminates with an exception exit code, the surviving peer reads only the matching report
from its current GUID directory. It displays the observed prior Bridge fault, exception location, access,
model context and leading stack frames. The dialog labels this as observed context rather than a proven
root cause. Closing the dialog allows the surviving peer to terminate.
