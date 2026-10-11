"""Validate measured shadow lifetime; report VA/commit separately from logical bytes."""
from pathlib import Path
import json
import sys

root = Path(sys.argv[1])
out = {}
for path in sorted(root.glob('*.jsonl')):
    rows = [json.loads(line) for line in path.read_text(encoding='utf-8-sig').splitlines() if line.startswith('{')]
    assert rows and all(r['scan_complete'] for r in rows), path
    background = next(r for r in rows if r['phase'] == 'background')
    assert background['address_limit'] > 3 * 1024**3, 'Need a real x86 LAA 4 GiB process'
    summary = {'background': background, 'rounds': [], 'contiguous_probe': [r for r in rows if 'probe-' in r['phase']]}
    for n in (1, 2, 3):
        batch = [r for r in rows if r['round'] == n]
        before = next(r for r in batch if r['phase'] == 'before-map')
        peak = max(batch, key=lambda r: r['static_shadow_bytes'] + r['dynamic_shadow_bytes'])
        locks = next(r for r in batch if r['phase'] == 'after-lock-phase')
        prelocks = next(r for r in reversed(batch[:batch.index(locks)]) if r['phase'] in ('static-growth', 'allocation-failed'))
        end = next(r for r in batch if r['phase'] == 'map-released')
        assert (locks['static_shadow_bytes'], locks['dynamic_shadow_bytes']) == (prelocks['static_shadow_bytes'], prelocks['dynamic_shadow_bytes'])
        assert not end['static_shadow_bytes'] and not end['dynamic_shadow_bytes'] and not end['objects']
        # CRT/queue heap retention is observed, not forced to equal zero.
        summary['rounds'].append({'round': n, 'before': before, 'peak': peak, 'locks': locks, 'released': end,
                                 'av_loss_at_peak': before['available_virtual_bytes'] - peak['available_virtual_bytes'],
                                 'av_remaining_loss_after_release': before['available_virtual_bytes'] - end['available_virtual_bytes'],
                                 'av_change_during_locks': prelocks['available_virtual_bytes'] - locks['available_virtual_bytes']})
    final = rows[-1]
    assert final['phase'] == 'all-owned-objects-released' and not final['objects']
    summary['retained_old_map'] = [r for r in rows if r['phase'] in ('retained-old-map', 'new-map-old-still-owned', 'all-owned-objects-released')]
    out[path.stem] = summary
(root / 'summary.json').write_text(json.dumps(out, indent=2))
for name, s in out.items():
    r = s['rounds'][0]
    print(name, 'shadow_MiB=', (r['peak']['static_shadow_bytes']+r['peak']['dynamic_shadow_bytes'])/2**20,
          'AV_loss_MiB=', r['av_loss_at_peak']/2**20, 'release_residual_MiB=', r['av_remaining_loss_after_release']/2**20,
          'lock_change_MiB=', r['av_change_during_locks']/2**20)
