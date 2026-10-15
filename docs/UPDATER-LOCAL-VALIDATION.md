# Updater local-test validation

Compiled source: `c7d9607db3e124a89787fb8f956b843b01dd7101`.
Build: `l4d2-1.3.2+325f743c784c0e30` (MSVC 14.29, Release /O2 NDEBUG).

[Windows CI, all checks passed](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38663341438).

- 95 updater-core assertions; failure injection at all four replacement positions and hard interruption/recovery at all four positions.
- 18 native cross-process sessions: x86/x64 Host, each accept/decline/cancel/same/older/crash/linger/alias-client/new-build. Different-build payloads prove all three binaries change; linger proves no replacement before game exit.
- Actual production Client hotkey/close/recovery code extraction passes for x86/x64 test harnesses, including exception containment.
- Existing IPC: 360 successful pairs, 24 fail-closed cases, 13 reply-control records; existing correctness/resource/Reset/GC/Steam/IME/diagnostic checks and full matched three-binary builds pass.
- Package independently checked: PE architectures, Host32 LAA, matching embedded build IDs, all payload hashes, Framework x86 updater, editable root VERSION, licensing, no replacement configuration/backend/DB/plugin files.
- Source comparison with main baseline: only four Bridge files changed, 282 byte-identical. See source-scope.json; no IPC transport or resource upload path changes.

This is an explicitly enabled local test. Ctrl+Shift+F10 triggers once per game process; automatic engine load-completion detection and signed production distribution are not enabled. Numeric VERSION edits are simulated test labels; embedded binary build identity remains truthful. Updater waits for actual pinned game/Host processes to exit and refuses abnormal game exit, concurrent same-install processes, or incomplete validation. No forced termination or automatic restart.

The renamed .vpk is a ZIP. Native tests validate updater parsing and replacement, not Source mounting, Steam delivery, real game shutdown behavior, or FPS. For the first game test, load the game before placing the file; remove it before the next launch to isolate mounting behavior.

[Direct verified ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/5c853a2f27cd245efb6ed6c8e5f78376524b6a5b/ldb-updater-test-v1.3.2-325f743c-r38663341438.zip).
SHA256: `598a55c4be22bb3e8c3cc7984786cb69a277a42110f08f0fe10457b9997066f5`.

[Chinese installation and recovery guide](UPDATER-LOCAL-TEST.zh-CN.md). Download branch receives only the ZIP, SHA256 and guide. No main merge, Release or Tag. Recovery materials must be retained until any pending transaction is resolved with `LDBupdater.exe --recover` after all game/Host processes exit.
