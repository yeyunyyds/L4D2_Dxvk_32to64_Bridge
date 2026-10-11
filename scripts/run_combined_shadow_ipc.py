"""Three actual production combinations, fresh processes, full-byte validation."""
import json
from pathlib import Path
import statistics
import subprocess
import uuid
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'.deps/ipc-combined-results'

def main():
    OUT.mkdir(exist_ok=True)
    rows=[]
    variants=('pr6','va-pr6','combined')
    for arch in ('32','64'):
        for repeat in range(5):
            order=variants[repeat%3:]+variants[:repeat%3]
            if repeat%2: order=tuple(reversed(order))
            for variant in order:
                directory=ROOT/'.deps'/f'ipc-combined-{variant}'
                session=str(uuid.uuid4())
                row=dict(host_arch=arch,variant=variant,repeat=repeat,valid=False)
                children=[]
                try:
                    children=[subprocess.Popen([str(directory/f'lifecycle-{role}.exe'),session],
                        stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True) for role in (f'host{arch}','client32')]
                    logs=[p.communicate(timeout=120)[0] for p in children]
                    row['logs']=logs
                    metrics=[json.loads(line) for log in logs for line in log.splitlines() if line.startswith('{')]
                    if any(p.returncode for p in children) or any('COMBINED_SHADOW_IPC_PASS' not in log for log in logs):
                        raise RuntimeError('production lifecycle failed')
                    if sum(m['kind']=='timing' for m in metrics)!=3 or sum(m['kind']=='va' for m in metrics)!=8:
                        raise RuntimeError('missing complete Client samples')
                    row.update(valid=True,metrics=metrics)
                except (RuntimeError,subprocess.TimeoutExpired) as error:
                    row['error']=str(error)
                finally:
                    for child in children:
                        if child.poll() is None: child.kill();child.wait()
                rows.append(row)
                (OUT/'shadow-ipc-raw.json').write_text(json.dumps(rows,indent=2)+'\n')
                if not row['valid']: raise RuntimeError(f'Preserved failed combined lifecycle: {row}')
    summary=[]
    for arch in ('32','64'):
        for variant in variants:
            sample=[r for r in rows if r['host_arch']==arch and r['variant']==variant]
            result=dict(host_arch=arch,variant=variant,rounds=[])
            for round_ in range(3):
                values={k:[] for k in ('creation_first_write_ns','submission_ns','upload_complete_ns','release_ns','released_av_loss','released_largest','mapped_channel_delta')}
                for row in sample:
                    by_stage={m['stage']:m for m in row['metrics'] if m['kind']=='va' and (m['round']==round_ or m['stage'] in ('before-channels','channels-live'))}
                    timing=next(m for m in row['metrics'] if m['kind']=='timing' and m['round']==round_)
                    for metric in ('creation_first_write_ns','submission_ns','upload_complete_ns','release_ns'): values[metric].append(timing[metric])
                    values['released_av_loss'].append(by_stage['channels-live']['free']-by_stage['shadows-released-channels-live']['free'])
                    values['released_largest'].append(by_stage['shadows-released-channels-live']['largest'])
                    values['mapped_channel_delta'].append(by_stage['channels-live']['mapped']-by_stage['before-channels']['mapped'])
                result['rounds'].append(dict(round=round_,stats={k:dict(median=statistics.median(v),min=min(v),max=max(v)) for k,v in values.items()}))
            summary.append(result)
    (OUT/'shadow-ipc-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print('COMBINED_SHADOW_IPC_ALL_PASS',len(rows),flush=True)

if __name__=='__main__': main()
