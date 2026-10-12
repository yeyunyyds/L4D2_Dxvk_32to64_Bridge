# ReShade original-window input experiment

Status: AWAITING GAME VALIDATION. Experimental branch: `codex/reshade-external-input`.
Formal VERSION/main/tags/releases are unchanged. This is an update to an existing
working LDB install; DXVK backend, presets, textures and effects are not bundled.

## Verified experimental download

Recommended build: `ldb-ipc-c32-dev+8fffd697a6a95120` with
`ldb-reshade-input-dev+8fffd697a6a95120`.

- Native source: `4ce6441027b4f26245f63d80b37a170bde71a654`.
- [Windows full build and native gate](https://github.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/actions/runs/38290865181): PASS.
- [Download the matched experimental ZIP](https://raw.githubusercontent.com/yeyunyyds/L4D2_Dxvk_32to64_Bridge/e2557536e4e07befb950895cb9fabf6b3a75496c/l4d2-bridge-reshade-original-window-dev-8fffd697a6a95120-r38290865181.zip).
- ZIP SHA-256: `45c28732d09f1c2a8fbce5633ac70f9a4cacb5e41f288f42dda6dad76fc93497`.
- [Permanent native and binary verification evidence](reshade-input-evidence/38290865181/).
- The earlier `67b3c8d0ad086d8d` candidate is superseded; this build additionally
  corrects no-legacy Raw Input Shift text translation and contains its native test.

All three Bridge binaries and both ReShade DLLs are identical to native CI outputs.
13 existing native suites, 252 production transport cases, 30 failure cases,
39 Python contracts, x86/x64 source-method/Client window/capture tests, four input
process-architecture pairs, and both compiled-DLL ABI probes passed. These gates
exercise source and OS transport; they do not claim L4D2/GPU validation.

## Architecture and gates

The game keeps its original x86 HWND. The x86 Client observes that window's actual
WndProc input once, before game-facing input neutralization; it does not forward
process-private RAWINPUT handles. A bounded 512-record, 16,704-byte shared mapping
carries ordered key/button/wheel/UTF-16/focus records. Pointer position is coalesced.
Only the window-owning Client thread enqueues; other threads can refresh position
and liveness. Events publish with release; one acquire batch read and release
acknowledgment retire a batch. Unread data is never overwritten.

The real ReShade `runtime::on_present` consumes before effects and GUI access.
The DLL retains native key/frame/text semantics, including ordinary native handling
of press and release inside the same frame. Client modifier/CapsLock state replaces
Host-local queue state. Panel-closed effects/hotkeys continue to receive input.
After GUI processing, the actual `_show_overlay` state publishes capture, protected
by generation, seqlock and a one-second freshness lease. Client also checks peer
process exit. A requested open/vetoable callback is not treated as completed open.

Steam/legacy UI and external ReShade own separate atomic capture bits. Releasing
one source does not release another. OS cursor capture/clip changes execute on the
window owner and restore only owned state. Steam activity suspends external input.
No new Presenter HWND, input forwarding thread, low-level global keyboard hook,
or timer is added. Existing Bridge/DXVK/ReShade threads still exist.

The experimental input ABI and all Bridge transport identifiers derive from the
same source fingerprint, including vendor patch and shared layout. The Host pins
and configures only a loaded DLL with the exact named interface and fingerprint;
wrong DLLs are refused. Generation isolates stale runtime replies and window
rebinds. Overflow permanently faults that input session and releases game capture;
restart both processes to recover. This does not replace D3D/IPC error handling.

Independent source-method tests, real cross-process tests, and actual Client window
collector tests precede the full build. Native full-DLL loading probes its exported
ABI and mapping; those probes do not claim a GPU runtime/gameplay pass.

## Installation

1. Exit L4D2 and both Bridge processes. Back up your current three Bridge files and
   `.l4d2bridge/bridge.conf`. Keep your working DXVK backend and ReShade configuration.
2. Copy this package's `bin/dxvk_d3d9.dll` over your existing Client bridge DLL.
   Copy its `bin/.l4d2bridge/L4D2Bridge32.exe`, `L4D2Bridge64.exe` and complete
   `reshade-external` directory into your existing `bin/.l4d2bridge/` directory.
   All Bridge binaries must be replaced together. DLLs must retain the x86/x64
   subdirectories shown in the archive.
3. Merge `config/RESHADE-EXTERNAL-INPUT-EXPERIMENT.conf` into the actual active
   `.l4d2bridge/bridge.conf`, replacing duplicate keys. Do not replace your entire
   config: Host/backend, arena, shadow, retention and other controls remain yours.
   The supplied general `config/bridge.conf` keeps this experiment off by default.
4. Use your existing Vulkan ReShade installation/config layout. This experiment
   loads its local custom Vulkan layer only in the Host, disables standard ReShade's
   implicit layer only in that process, and does not modify registry/global files.
   Remove any *manually configured additional ReShade proxy inside the Host path*
   from this test's load chain if present; two ReShade instances are unsupported.
   No need to replace Steam's overlay DLLs or install ReShade32 in the game.
5. Start normally. Host64 uses custom ReShade64 despite the x86-owned game HWND;
   Host32 uses custom ReShade32. Existing ReShade.ini, effects and presets remain
   in their original Host configuration location (normally `.l4d2bridge/ReShade.ini`).
   Do not turn the old Presenter on through L4N while this mode is enabled.
   Prefer `InputProcessing=2` in its `[INPUT]` section for this first experiment.

Required new mode:

```ini
server.reshadeExternalInput = True
server.presenterWindow = False
server.presenterInput = False
client.hookMessagePump = False
client.overrideCustomWinHooks = False
```

The config snippet retains the accepted Steam/Workshop keys and C8. It turns off
per-frame diagnostic logging. You can retain another already validated IPC gear.

## What to verify on hardware

Normal Host logs: `LDB_EXTERNAL_INPUT event=host-channel ready=1`, Client:
`event=client-channel ready=1`, then Host: `event=host-configured ... no_presenter=1`.
Actual ReShade log additionally needs `event=runtime-bound epoch=...`; configuration
alone does not prove a rendering runtime attached. Standard `L4D2_OVERLAY
 event=presenter` should not appear in this mode.

Test Home/your configured overlay chord, mouse movement/click/drag/wheel, Unicode
text, modifiers, preset/effect/screenshot hotkeys with panel closed, opening and
closing without stuck game keys/cursor, Alt+Tab, resolution/window-mode changes,
map reload and repeated map transitions. Alt+Tab here means focus-loss/recovery,
not an overlay hotkey. Test Shift+Tab, Steam browser/Join Server/View Workshop,
and Steam/ReShade transitions while either panel was already open. Compare CPU
and frame times with your accepted Steam build using the same map/effects/IPC gear.

If input fails, send bridge32.log, bridge64.log, ReShade.log and active bridge.conf.
No additional default diagnostic thread is required.

## Current experimental limits / rollback

One primary game window/runtime is bound at a time. This first integration uses
whole-panel gameplay suppression while ReShade is visibly open, consistent with
the prior Presenter approach; ReShade's fine-grained InputProcessing=0/1 policy is
not mirrored per keyboard/mouse. Other custom swapchain/VR/add-on input paths are
not hardware validated. Capture propagation is asynchronous, with a frame-scale
transition window; no zero-latency suppression or performance gain is claimed.

For a first rollback, close the processes, set `server.reshadeExternalInput=False`
and restore your previously working Presenter/input config. The local layer is
then unused, and standard Vulkan ReShade follows the normal inherited environment.
For a full rollback also restore the backed-up three Bridge binaries together and
old config. Do not pair these experimental Client/Host binaries with the prior set.

The independent native gate passed. Full native compilation/regressions and package
checks are recorded separately with their run IDs and SHAs. L4D2/GPU stability,
Steam coexistence in this new mode, and actual performance await user testing.
