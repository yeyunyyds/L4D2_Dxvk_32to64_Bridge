# LDB correctness-only audit and sequential validation

Base: main `64798c77e96b05d70720bcd5bedab32aa9a0b7e9` (includes the Steam Renderer allowlist and confirmed IME integration). Development branch: `codex/ldb-correctness-only`.

The candidates and several test adapters were informed by [NPCodex Nightlybuild](https://github.com/NPCodex/L4D2_Dxvk_32to64_Bridge-Nightlybuild/tree/f722abffb02dd6419c34ac54a50e85432d03a39a). Each change was checked against the actual patched LDB source, not accepted on the basis of a changelog. Production changes remain in `patches/l4d2-bridge.patch` against pinned NVIDIA commit `9aa74f8dfad2188efbd0f717c64d9f8fa909787e`.

## Findings and minimal repairs

| Order | Confirmed in frozen main | Repair | Native evidence |
|---|---|---|---|
| Query | Client and Host treat `S_FALSE` as result-bearing; PIXELTIMINGS uses the wrong SDK structure; insufficient request validation | Copy/serialize data only for `S_OK`; correct stage-timing size; validate flags, exact nonzero length and destination; bound Host allocation to legal SDK result sizes; non-inserting lookup | [Query gate](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38544104454): x86/x64 pass, frozen main fails, pending/size negative controls fail |
| Shader | Unsigned `startRegister + count` can wrap before validation; signed clamp uses the same unchecked sum | Subtraction-based range check, bounded hardware count, compile-time wire byte limit | [Shader gate](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38544334751): 390 range cases per architecture, six public constant pairs, recording/live state, frozen and guard-reverted failures |
| UpdateTexture | Null input is dereferenced; Release omits the type assertion and can dispatch mismatched wrappers | Runtime null/type checks before existing 2D/Cube/Volume dispatch | [Texture gate](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38544650695): both architectures pass; frozen Release accepts mismatched types and its isolated null-input process exits with `0xC0000005` |
| SharedHeap | Unknown ID lookup inserts an entry; no logical size on Host; duplicate/overlapping/out-of-segment allocations accepted; DISCARD failures lose old backing; null views undergo pointer arithmetic | Track/transmit logical byte length; reject invalid ID/ranges; preserve old VB/IB/Surface backing on failed allocation/view; validate source spans before backend locks; initialize buffer policy from the incoming descriptor; reject offset before arithmetic and bound optimized-lock reservation | [SharedHeap gate](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38545259380): real Windows mappings, Client32→Host32/64 bytes, 2048 release/recreate cycles, failures/recovery and shared upload edges; frozen unknown-ID and DISCARD negative controls fail |
| Host | `GET_RES` and upload/Query lookups insert absent IDs; upload casts do not verify resource kind; ATI can examine malformed RECT before its normal-path checks | Non-inserting lookup; terminal channel fault for absent required object; VB/IB/Surface/VolumeTexture kind checks, Volume lookup, Query Issue error and safe repeated destroy; early Surface RECT validation; keep Device Destroy idempotent for failed creation/duplicate cleanup | [Host gate](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38546203975): x86/x64 missing/stale/null/recreate, Query lifetime, five upload routes, reserved-pin cleanup and all preceding suites pass |

The final lifecycle review added a Device Destroy exception to the required-object guard: cleaning up a missing Device must remain a no-op, as in frozen main. Reusing `GET_RES` here would convert failed-creation cleanup/repeated Destroy into a terminal channel fault. The added real-dispatch fixture checks this behavior and a `strict-destroy` negative control must fail with `device cleanup faulted channel`.

The first Host fixture run failed to compile because its wrapper name collided with the Windows SDK `IDirect3DQuery9_Issue` macro. The wrapper was renamed; this was a test adapter repair, with no production change. The failed run remains available as [38545895692](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38545895692).

## Boundaries and compatibility

Only seven production files change: Client device, Query, lockable buffer and Surface; SharedHeap header/implementation; Host main dispatcher. Steam, IME, Presenter, ReShade, IPC publication/wait algorithms, shadow allocator, retention database and runtime configuration are preserved byte-for-byte relative to the base.

The shared allocation packet gains one `uint32_t` logical-byte field after its first chunk. Client, Host64 and comparison Host32 must be replaced together. Existing startup matching compares the full patch-derived build identity before command dispatch; an older Host is rejected rather than interpreting this field as another packet. The standalone audit used VERSION 1.3.1 and created no Tag or Release at that stage. Subsequent v1.3.2 integration is recorded below.

SharedHeap remains disabled by default. Its size/range maps are used only by the opt-in path. Other added runtime work is bounded parameter checks and map `find`/resource-kind validation. No new threads, kernel waits or per-command diagnostics are added. No performance or FPS improvement is claimed without measurement.

The Surface shared source bound uses `(rows - 1) * pitch + rowBytes`, not `rows * pitch`, so a partial right/bottom rectangle does not falsely demand padding beyond the allocation. Existing row-wise copies and zero-byte VB/IB Lock/Unlock behavior remain unchanged.

Query keeps per-call allocation after validation; the Nightly persistent scratch-buffer optimization is not included. Pool scan hints, deadline clock changes, bulk-copy fast paths, database tuning and unrelated creation-lifecycle rewrites are also excluded.

## Reproduction and limits

On Windows with Python 3.11 and MSVC 14.29:

```powershell
python scripts/prepare_bridge.py
powershell -NoProfile -File scripts/test_correctness.ps1 -Stage all
```

The checkout must include frozen main `64798c7` (CI uses fetch-depth 0). The runner reconstructs the original source from the same pinned upstream, compiles real extracted Client methods/Host dispatches and unchanged SharedHeap/SharedMemory, and retains stdout and source manifests under `work/correctness-*`. Each subprocess has a timeout; negative controls must produce their specific failure, not merely exit unsuccessfully.

Stage inputs are `query`, `shader`, `texture`, `sharedheap`, `host` and `all`. Each stage includes previous stages; implementation advanced only after the preceding native stage passed. The full build workflow runs this gate plus the established IPC, upload safety, buffer/shadow, Steam/IME, lifecycle and diagnostic regressions, then verifies actual Release compile receipts for all three binaries.

These tests use backend/transport adapters where a game or GPU is unnecessary. They do not replace L4D2 map-switch, Steam UI, GPU resource upload or gameplay testing. At the standalone audit stage, disposition was **AWAITING GAME VALIDATION** after native/build gates passed; main had not yet been updated.

## Final integrated result

[Final Windows run 38548826740](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38548826740) passed every required gate and compiled all three matched Bridge roles plus L4N, from source `7bfc3e021d739d09b94472edbf84f6cccc42a660`. Build ID: `l4d2-1.3.1+3d545d8aa5ca7164`.

The final IPC matrix contains 360 successful Client32→Host32/64 pairs: three repetitions, six batch settings, two Host architectures and ten modes. It also passes 24 intentional transport-fault cases, the seven SharedHeap native/cross-process cases, and the established upload, buffers, reset, Steam/IME, window capture, diagnostics, wakeup and Readback/residency recovery regressions. This is correctness evidence, not an FPS result.

[Final Host cleanup gate 38548680765](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38548680765) confirms Device Destroy idempotence on x86/x64 and rejects the strict-destroy negative control. An earlier full run passed before this compatibility review; its artifacts were not distributed.

Raw matrix results, fault cases, SharedHeap results/source manifests, compile receipts and a compact production-source diff against frozen main are retained in [results/correctness-20261014](../results/correctness-20261014). Source commit in the compile receipt identifies the binaries; later commits update evidence only.

Standalone experimental disposition: **AWAITING GAME VALIDATION**. Recommended game checks cover map entry/repeated switches, resource uploads, Steam invite/Chinese input and normal exit with the matched binaries. Keep the existing configuration; SharedHeap stays disabled by default. Roll back by restoring the previous matched Client and both Hosts together.

Experimental downloads (original CI ZIP bytes, SHA-256 rechecked after upload):

- [Matched update (preserves runtime config/backends)](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/e42dd64da727b8eb64739014857be01e019718fe/correctness-7bfc3e0-r38548826740/l4d2-bridge-correctness-7bfc3e0-r38548826740-patch.zip)
- [Full Bridge with default config and official DXVK](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/e42dd64da727b8eb64739014857be01e019718fe/correctness-7bfc3e0-r38548826740/l4d2-bridge-correctness-7bfc3e0-r38548826740-full.zip)
- [Optional L4N plugin](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/e42dd64da727b8eb64739014857be01e019718fe/correctness-7bfc3e0-r38548826740/l4d2-bridge-l4n-correctness-7bfc3e0-r38548826740.zip)
- [SHA256SUMS](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/e42dd64da727b8eb64739014857be01e019718fe/correctness-7bfc3e0-r38548826740/SHA256SUMS.txt)

## v1.3.2 integration and release

After the standalone audit and native gates, the user explicitly authorized integration into main and stable Release v1.3.2. The seven-file production patch is unchanged from the tested correctness candidate (SHA-256 `3d545d8aa5ca71641f3305f66c1859936dc5628a0519f0e68c348b6e03c8e6f4`). Main's existing Steam Renderer and confirmed IME implementation are retained.

VERSION changes to 1.3.2 and all three Bridge roles are rebuilt, with build ID `l4d2-1.3.2+3d545d8aa5ca7164`. The release requires a fresh full Windows build/regression gate and matching artifact verification. Earlier 1.3.1 experimental ZIPs above are historical evidence and are not relabeled as 1.3.2. Release authorization does not add gameplay or performance evidence; the combined correctness patch has no new L4D2/GPU result. See [release installation and validation scope](RELEASE-V1.3.2.md).
