"""Same-machine production PR6/hybrid comparison; failures are never discarded."""
import itertools
import json
import platform
from pathlib import Path
import statistics
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'.deps/ipc-combined-results'
SCENARIOS = [('batch',0,24000,8388608),('batch',64,24000,8388608),
             ('batch',4096,3000,8388608),('batch',65536,190,8388608),
             ('stream',64,24000,8388608),('stream',524288,48,8388608),
             ('stream',2097152,48,8388608),('near-full',64,204,4096),
             ('wrap',64,12000,4096),('slow',64,12000,4096),('idle',0,1,4096)]

def main():
    OUT.mkdir(exist_ok=True)
    rows=[]
    for arch,scenario in itertools.product(('32','64'),SCENARIOS):
        mode,size,count,capacity=scenario
        for repeat in range(-1,5):
            order=('pr6','combined') if repeat%2 else ('combined','pr6')
            for variant in order:
                args=[str(uuid.uuid4()),str(count),str(size),mode,str(capacity),'known']
                directory=ROOT/'.deps'/f'ipc-combined-{variant}'
                row=dict(host_arch=arch,mode=mode,bytes=size,count=count,capacity=capacity,
                         variant=variant,repeat=repeat,valid=False)
                children=[]
                try:
                    children=[subprocess.Popen([str(directory/f'perf-{role}.exe'),*args],
                              stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
                              for role in (f'host{arch}','client32')]
                    logs=[p.communicate(timeout=25)[0] for p in children]
                    row['logs']=logs
                    metrics=[json.loads(line) for log in logs for line in log.splitlines() if line.startswith('{')]
                    if any(p.returncode for p in children) or len(metrics)!=2 or {m['role'] for m in metrics}!={'client','host'}:
                        raise RuntimeError('missing metrics or failed process')
                    row.update(valid=True,metrics=metrics)
                except (RuntimeError,subprocess.TimeoutExpired) as error:
                    row['error']=str(error)
                finally:
                    for child in children:
                        if child.poll() is None: child.kill();child.wait()
                rows.append(row)
                (OUT/'raw.json').write_text(json.dumps(dict(platform=platform.platform(),samples=rows),indent=2)+'\n')
                if not row['valid']: raise RuntimeError(f'Preserved failed production sample: {row}')
        print('COMBINED_IPC_SCENARIO',arch,mode,size,flush=True)
    summary=[]
    for arch,scenario in itertools.product(('32','64'),SCENARIOS):
        mode,size,_,_=scenario
        group=[r for r in rows if r['host_arch']==arch and r['mode']==mode and r['bytes']==size and r['repeat']>=0]
        result=dict(host_arch=arch,mode=mode,bytes=size,variants={},paired={})
        for variant in ('pr6','combined'):
            stats={}
            for role,metric in itertools.product(('client','host'),('wall_ns_call','cpu_ns_call','cycles_call','end_to_end_ns_call')):
                values=[next(m for m in r['metrics'] if m['role']==role)[metric] for r in group if r['variant']==variant]
                stats[f'{role}_{metric}']=dict(median=statistics.median(values),min=min(values),max=max(values))
            result['variants'][variant]=stats
        for role,metric in itertools.product(('client','host'),('wall_ns_call','cycles_call','end_to_end_ns_call')):
            deltas=[]
            for repeat in range(5):
                pair={r['variant']:next(m for m in r['metrics'] if m['role']==role)[metric] for r in group if r['repeat']==repeat}
                if pair['pr6']: deltas.append((pair['combined']/pair['pr6']-1)*100)
            if deltas: result['paired'][f'{role}_{metric}']=dict(median_percent=statistics.median(deltas),min=min(deltas),max=max(deltas),pairs=len(deltas))
        summary.append(result)
    (OUT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print('COMBINED_IPC_ALL_PASS',len(rows),flush=True)

if __name__=='__main__': main()
