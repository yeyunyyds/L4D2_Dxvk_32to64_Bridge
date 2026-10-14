# SPDX-License-Identifier: MIT
"""Bound every native fixture, retain stdout, and verify the exact negative-control reason."""
import argparse,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--expect',default='');p.add_argument('exe');p.add_argument('args',nargs='*');a=p.parse_args()
r=subprocess.run([a.exe,*a.args],capture_output=True,text=True,timeout=40)
out=r.stdout+r.stderr
print(out,end='')
Path(a.exe+'.log').write_text(out,encoding='utf-8')
if a.expect:
 if r.returncode!=1 or a.expect not in out:raise SystemExit(f'Negative control failed: expected {a.expect!r}, exit={r.returncode}')
 print('EXPECTED FAILURE:',a.expect)
elif r.returncode:raise SystemExit(f'Native fixture failed: exit={r.returncode}')
