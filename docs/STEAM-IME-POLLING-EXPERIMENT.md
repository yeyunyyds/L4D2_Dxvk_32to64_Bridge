# Steam IME polling exemption experiment

Status: Windows build/native tests PASS; real game IME validation pending.
Main, formal VERSION/Release and the cursor/exit behavior are unchanged.

## Paired feedback on bootstrap experiment

Both attached sessions use build l4d2-1.3.1+74bec765ec9bfa94. The first has bootstrap
True, the second False; the user reports failure for both. True successfully
initializes COM and yields an IMM context. Across 118 owner-thread samples,
open remains0 and conversion queries fail; Host focus is present in22 samples,
and Chinese/English layouts demonstrably change while focused. Queue counters
show no observed composition/result, but Steam hooks may consume messages.
This does not establish context validity or prove all IME events absent.

The first Client log directly identifies SogouPy.ime, SogouTSF.ime and Windows
textinputframework.dll calling keyboard getters while the Bridge neutralizes
those returns. Client suppression is broader than game input alone. This is a
concrete boundary to test, not proof of the only root cause or a successful fix.

## Scoped candidate

With client.steamImePollingPassThrough=True, only a known-active Steam overlay,
aggregate UI-active state and an identified input-method caller allow actual
GetKeyState/GetAsyncKeyState/GetKeyboardState/GetCursorPos results. .ime modules
are recognized without a Sogou-specific name. Windows msctf/imm32/textinputframework
DLLs must be loaded directly from System32 or SysWOW64; similarly named game DLLs
remain excluded. Classification preserves LastError and fails closed on lookup,
long-path or unknown-module results. It is performed only while the opt-in
exception is active, with no stale address cache or module pinning.

Original functions execute exactly once on exemption and their real failures,
output buffers and LastError remain intact. Game cursor caches are untouched.
Game/engine/MOD callers remain neutralized; no setter, raw-input or DirectInput
exception is added. This is not an unrestricted key-state pass-through switch.
Bootstrap remains False in the test snippet to stop repeating the failed control.
The proposed Steam IMM IAT observer is archived locally, not included in this
candidate. Existing owner-thread diagnostics remain available, without logging
composition contents. All new behavior switches default False.

## Validation and test

Extracted actual production gating and all four getter bodies pass seven local
modes, including IME callers, inactive/unknown Steam, game callers, failure and
invalid-buffer cases, exact original-call counts and game cache/setter refusal.
Portable module-path checks pass. Windows x86/x64 real-module classification,
production compilation and the complete prior native suite are required before
shipping. These checks do not demonstrate actual Sogou/Microsoft composition.

Use the matched experimental three-binary update after closing game/Hosts and
backing up existing binaries. Preserve runtime configuration and merge only
config/STEAM-IME-POLLING-EXPERIMENT.conf, then restart. Compare the new Client flag
True versus False on the same binaries, with bootstrap False for both. Test
Chinese switching, composition/commit/candidate display, repeated open/close and
ordinary gameplay after closing. Look for action=ime-pass-through and verify
engine/MOD polling still logs neutralization. Collect both logs on failure.

## Verified native build and download

[Run38461907481](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38461907481)
passed all production builds and native checks for source
`d9a7860121e33ec1dac3e92ed3664b9c0b5b0b30`. Both x86/x64 real-module classification
fixtures pass; extracted production getter bodies pass all seven modes on each
architecture. Existing full-IPC, Steam capture/window/request, resource and peer
failure checks pass. Publication was skipped; no game composition was tested.

[Matched experimental update](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/d50d99b4bd72850e05a8304067522b928e58e6b5/steam-ime-polling-experiment-r38461907481/l4d2-bridge-steam-ime-polling-experiment-5ba86729c6fbce0d-r38461907481.zip)

Build `l4d2-1.3.1+5ba86729c6fbce0d`; ZIP SHA256
`a3b69c02b3d75b37ba585b21849242ece59f49a1685ac8a513bebab520606fd9`.
Artifact source, architecture, binary checksums and embedded matching identities
were verified before ZIP-branch upload; public download hash matches.
Main and formal Release remain unchanged. Exit Game remains an open separate
issue; its dialog is not suppressed by this IME experiment.
