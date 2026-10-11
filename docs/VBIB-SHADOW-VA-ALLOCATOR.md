# VB/IB Shadow VA allocator experiment

This branch preserves full CPU shadow contents and the production Lock/Unlock
methods. Only payload allocation/ownership changes. It is not a release or a
Host-authority implementation; real L4D2 frame-time validation is pending.

## Structure

`buffer_shadow_arena.h` implements an allocator core. Windows reserve/commit/
release and metadata operations are supplied by `buffer_shadow.h`. Small
allocations use fixed-size classes from 16 bytes through 256 KiB, normally
up to 1 MiB per region (64 KiB minimum). Regions grow on demand, committing
only the page-rounded used prefix. Reserve failure retries smaller regions.
Larger allocations have dedicated VA. Metadata uses the Windows process heap,
not per-payload CRT arrays. No empty region cache is retained.

The owner stores the payload pointer and region descriptor. Live pointers do
not move. Reused slots honor zero initialization. Allocation failure still
throws `std::bad_alloc`. Region bookkeeping uses one SRW lock at creation and
destruction only. Lock/Unlock and IPC have no new allocator synchronization.

When the last allocation leaves a region, `MEM_RELEASE` returns its entire
reservation. A nonempty region can strand free slots; the experiment therefore
includes partial survivors. OS release failure is accounted as retained VA,
with descriptors preserved for reuse/retry, not falsely counted as a successful
release. Per-resource payloads cannot be freed through `delete[]`.

## Reproduce

- Prepare candidate with `python scripts/prepare_bridge.py`.
- Run `scripts/test_buffer_shadow_arena.ps1` for x86/x64 core regressions.
- Run the existing buffer contract and runtime observation regressions.
- Run `scripts/test_buffer_shadow_va.ps1` for Windows x86 LAA A/B evidence.

The CRT baseline is main `de74cd7d5dd546bfd4f03ab451d0565f1fff0107`, verified
against production patch SHA-256
`e8a61ece359546f8b1ca815f3e708b8bb8caef12b3be8f46de0d3b46201a3ee9`.
Both variants mechanically extract the actual production LockableBuffer and use
the same COM/IPC fixture, native allocation and batch-boundary timing. Five
fresh-process paired samples alternate order; each has three lifecycle rounds.
The workloads cover large/small resources, low AV, fragmented VA and surviving
resources. Workload failures are preserved. Timing comparisons use equal-work
first rounds only; later rounds can achieve different amounts of allocation
because retained VA differs.

Real diagnostics and allocator counters separately report logical bytes,
region reserve/commit, largest free range and CRT heap state. The fixed-size
classes and region geometry are candidates, not a proven optimum. Synthetic
memory-pressure background is reserve-only, not a simulated native game/Host.
CPU/total batch time includes fixture work and cannot be interpreted as FPS.

## Adoption gate

Require existing content/failure/lifetime regressions and matched Client/Host
builds to pass. Check live VA, released VA, largest free region and allocation/
destruction time together. If region fragmentation or system-call cost defeats
the benefit, reject or revise the geometry without weakening D3D9 semantics.
Actual Avg FPS, 1% Low, 0.1% Low and map-switch loading still require game tests.
