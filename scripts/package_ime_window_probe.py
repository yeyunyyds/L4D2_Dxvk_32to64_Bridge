# SPDX-License-Identifier: MIT
"""Package only our independently compiled probes, never third-party IMEs."""
from pathlib import Path
import hashlib,json,shutil,subprocess
from package_release import machine
root=Path(__file__).resolve().parents[1]
source=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
build='ime-window-probe+'+source[:12]
out=root/'dist/l4d2-ime-window-probe';out.mkdir(parents=True,exist_ok=False)
hashes={}
for bits,pe in ((32,0x14c),(64,0x8664)):
 name=f'ImeWindowProbe{bits}.exe';p=root/'work/ime-window-probe-native'/name
 assert machine(p)==pe and build.encode() in p.read_bytes(),name
 shutil.copy2(p,out/name);hashes[name]=hashlib.sha256(p.read_bytes()).hexdigest()
shutil.copy2(root/'docs/STEAM-IME-WINDOW-PROBE.md',out/'README.md')
(out/'Start-Probe.cmd').write_text('@echo off\r\ncd /d "%~dp0"\r\nImeWindowProbe32.exe\r\n',newline='')
(out/'BUILD-INFO.json').write_text(json.dumps({'source_commit':source,'build_id':build,'independent_tool':True,'gameplay_validated':False,'native_synthetic_validation':'PASS','files':hashes},indent=2)+'\n')
(out/'SHA256.json').write_text(json.dumps(hashes,indent=2)+'\n')
print('Packaged native-verified owned Unicode probes; real IME composition pending.')
