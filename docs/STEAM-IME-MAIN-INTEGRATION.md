# Steam Overlay Chinese IME main integration

Base main20ef9df includes PR12, reviewed Steam Renderer11.12.30.7 support.
This change preserves its entire availability implementation, reviewed hashes,
getter decoding, web/invite forwarding, window adapters and regression fixture
byte-for-byte. IPC/resource/AV/ReShade implementation blocks are also unchanged.
Only main.cpp Presenter options, overlay_presenter.h and a new minimal
steam_ime_native.h runtime helper change in the cumulative production patch.

The user validated experimental build l4d2-1.3.1+31cae0cdcb96c4ec. Six nonempty
Steam GCS_RESULTSTR reads and the user's visible Chinese text confirm that
session worked. Independent modes9/12 (no hidden EDIT focus) fail;10/11 succeed,
and11 destroys the helper before typing. Mere default IMM window creation is
insufficient; short native control focus establishes the missing TSF state.

Main retains the tested activation and cleanup order, extracted into ImeThread:
on the existing Presenter thread initialize own STA COM reference, acquire or
create TSF manager, activate one owned TSF client; on worker exit deactivate
only that client, release manager, uninitialize only our successful COM reference,
release library handles. A foreign MTA apartment is left intact. There are no
new diagnostic hooks, observers, TSF message pump, polling overrides, bootstrap
HIMC replacement or input mode forcing. Those experiment branches are not merged.

First foreground Steam capture, after normal Presenter focus, creates a hidden
1x1 EDIT on the same owning thread, briefly focuses it, restores Presenter if
still foreground/capturing, and destroys EDIT. Attempt once per Presenter
window; a failed attempt logs metadata and does not loop or abort rendering.
The successful experimental warmup operation and guards are preserved, with
only experiment observation callbacks omitted. No extra renderer/window remains.

server.steamImeTsfActivate and server.steamImeNativeWarmup default True, and the
shipped normal/Steam preset configs state them explicitly. Existing Steam-native
input guards still apply; ReShade-only input does not trigger the helper. Disable
both options and restart for the previous IME baseline; existing Steam options
continue independently. Regular diagnostic flags stay False. VERSION and formal
release/tag contents are not changed. No uploaded Steam DLL is redistributed.

Integration gates: PR12 legacy/current Renderer hashes and reject/LastError
fixtures on both architectures; web/workshop/window/capture regressions;
production Presenter foreground/helper/focus/once/teardown tests with x86/x64
Hosts; native COM/TSF disabled, existing STA and foreign MTA ownership tests;
disabled observation invariants; all three Release compilations with matching
identities. Experimental game validation covers one x64 session; this merged
Renderer+IME combination still deserves the normal subsequent gameplay checks.


## Completed integration validation

Source4d61407e3df0e218d40a096c5359c90c18f53eba, based on main20ef9df.
[Windows integration run38519119353](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38519119353)
passed: disabled/STA/MTA ImeThread on x86/x64; production Presenter capture,
focus/helper destruction/one attempt on both Host architectures; Steam capture,
window adapter, web, PR12 old/new availability, cross-bitness Workshop status,
and disabled runtime observation. Seven local runtime separation checks passed.
All three Release binaries compiled and matched the core build receipt:
`l4d2-1.3.1+d08b980cc177f23d`. Protocol identities, PE architectures, x86 LAA,
source/build stamps and packaged checksums were verified after downloading the
CI artifact and checking its archive SHA256. Publication job was skipped.

PR12 availability fixture remains SHA256
25c3688466d27d2899d319b3c00a673d843c35466649821911fe13da5bf1d55f.
Every preexisting production patch block other than main.cpp and
Overlay Presenter is byte-for-byte preserved from main20ef9df; only the new
ImeThread header is added. The former experiment's diagnostics, probe tooling,
polling/COM bootstrap alternatives and protocol-unrelated work were not merged.
The final validation-record commit changes docs only. VERSION remains1.3.1;
no Tag or Release creation is performed by this integration.
