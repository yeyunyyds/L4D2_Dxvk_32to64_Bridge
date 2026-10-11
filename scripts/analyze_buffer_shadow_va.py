"""Paired native x86 VA evidence; compare time only for equal completed work."""
from pathlib import Path
import hashlib
import json
import platform
import statistics
import sys

root = Path(sys.argv[1])
results = {}
for variant in ('crt', 'arena'):
    samples = {}
    for path in sorted((root / variant).glob('*.jsonl')):
        rows = [json.loads(line) for line in path.read_text(encoding='utf-8-sig').splitlines() if line.startswith('{')]
        assert rows and all(r['scan_complete'] and r['crt_heap_walk_complete'] for r in rows), path
        background = next(r for r in rows if r['phase'] == 'background')
        assert background['address_limit'] > 3 * 1024**3, 'requires actual x86 LAA'
        if variant == 'arena':
            assert all(r['arena_allocations'] == r['objects'] and not r['arena_release_failures'] for r in rows)
        rounds = []
        for n in (1, 2, 3):
            batch = [r for r in rows if r['round'] == n]
            if not batch or not any(r['phase'] == 'map-released' for r in batch):
                assert rows[-1]['phase'] == 'process-oom-released', 'incomplete sample without recorded OOM'
                break
            before = next(r for r in batch if r['phase'] == 'before-map')
            peak = max(batch, key=lambda r: r['static_shadow_bytes'] + r['dynamic_shadow_bytes'])
            release = next(r for r in batch if r['phase'] == 'map-released')
            assert not release['objects'] and not release['static_shadow_bytes'] and not release['dynamic_shadow_bytes']
            if variant == 'arena':
                assert not release['arena_regions'] and not release['arena_reserved_bytes'] and not release['arena_committed_bytes']
            lock = next(r for r in batch if r['phase'] == 'after-lock-phase')
            growth = [r for r in batch[:batch.index(lock)] if r['phase'] in ('dynamic-12.25mib', 'static-growth', 'allocation-failed')]
            assert lock['static_shadow_bytes'] == growth[-1]['static_shadow_bytes']
            survivors = [r for r in batch if r['phase'] == 'partial-survivors']
            dynamic = next(r for r in batch if r['phase'] == 'dynamic-growth')
            release_phases = [r for r in batch if r['phase'] in ('partial-survivors', 'static-released', 'map-released')]
            rounds.append({
                'round': n, 'logical_peak': peak['static_shadow_bytes'] + peak['dynamic_shadow_bytes'],
                'av_loss_peak': before['available_virtual_bytes'] - peak['available_virtual_bytes'],
                'av_residual': background['available_virtual_bytes'] - release['available_virtual_bytes'],
                'largest_free_released': release['largest_free_bytes'],
                'create_ns': sum(r['phase_nanoseconds'] for r in growth),
                'create_cpu_ns': sum(r['phase_cpu_nanoseconds'] for r in growth),
                'create_work_bytes': lock['static_shadow_bytes'] + lock['dynamic_shadow_bytes'],
                'lock_ns': lock['phase_nanoseconds'],
                'lock_cpu_ns': lock['phase_cpu_nanoseconds'],
                'lock_work_calls': lock['lock_cycles'],
                'release_ns': sum(r['phase_nanoseconds'] for r in release_phases),
                'release_cpu_ns': sum(r['phase_cpu_nanoseconds'] for r in release_phases),
                'release_work_bytes': lock['static_shadow_bytes'] + dynamic['dynamic_shadow_bytes'],
                'survivors': survivors, 'released': release,
            })
        assert rows[-1]['phase'] in ('all-owned-objects-released', 'process-oom-released') and not rows[-1]['objects']
        if variant == 'arena': assert not rows[-1]['arena_reserved_bytes']
        samples[path.stem] = {'background': background, 'rounds': rounds,
                             'exhausted_va': rows[-1]['phase'] == 'process-oom-released',
                             'incomplete_round_records': [r for r in rows if r['round'] > len(rounds)],
                             'probes': [r for r in rows if 'probe-' in r['phase']]}
    assert len(samples) == 25, (variant, len(samples))
    results[variant] = samples

def distribution(values):
    return {'median': statistics.median(values), 'min': min(values), 'max': max(values)}

paired = {}
for scenario in ('2300-few-large', '2300-many-small', '800-many-small', '800-fragmented', '2300-partial-survivors'):
    pairs = [(results['crt'][f'{scenario}-{i}']['rounds'][0], results['arena'][f'{scenario}-{i}']['rounds'][0]) for i in range(1, 6)
             if results['crt'][f'{scenario}-{i}']['rounds'] and results['arena'][f'{scenario}-{i}']['rounds']]
    assert pairs, 'no completed first-round comparison'
    row = {}
    for variant, index in (('crt', 0), ('arena', 1)):
        row[variant] = {metric: distribution([pair[index][metric] for pair in pairs]) for metric in
            ('logical_peak', 'av_loss_peak', 'av_residual', 'largest_free_released', 'create_ns', 'lock_ns', 'release_ns',
             'create_cpu_ns', 'lock_cpu_ns', 'release_cpu_ns')}
    equal = [pair for pair in pairs if pair[0]['logical_peak'] == pair[1]['logical_peak']]
    row['equal_work_pairs'] = len(equal)
    row['exhausted_va_samples'] = {v: sum(results[v][f'{scenario}-{i}']['exhausted_va'] for i in range(1, 6)) for v in ('crt', 'arena')}
    # Later rounds deliberately expose retained VA changing achievable work;
    # never present their different workloads as an allocator speedup.
    for metric in ('create_ns', 'lock_ns', 'release_ns'):
        work = metric.split('_')[0] + ('_work_calls' if metric == 'lock_ns' else '_work_bytes')
        completed = [(a, b) for a, b in pairs if a[work] == b[work] and a[metric]]
        row[metric + '_equal_work_pairs'] = len(completed)
        row[metric + '_paired_change'] = distribution([b[metric] / a[metric] - 1 for a, b in completed]) if completed else None
    paired[scenario] = row
    print(scenario, 'CRT/arena residual MiB', row['crt']['av_residual']['median'] / 2**20,
          row['arena']['av_residual']['median'] / 2**20, 'equal-work pairs', len(equal))
(root / 'summary.json').write_text(json.dumps({'environment': {'platform': platform.platform(), 'processor': platform.processor()},
    'paired_first_round': paired, 'samples': results}, indent=2))
hashes = {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(root.rglob('*')) if p.is_file()}
(root / 'SHA256.json').write_text(json.dumps(hashes, indent=2))
