"""Exercise production SharedHeap bounds plus x86->x86/x64 physical mappings."""
import json
import argparse
from pathlib import Path
import subprocess
import uuid
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--output', required=True, type=Path)
OUT = parser.parse_args().output
rows = []
for arch in ('x86','x64'):
    result = subprocess.run([OUT / f'clientheap-{arch}.exe'], capture_output=True, text=True, timeout=20)
    rows.append(dict(role='client-'+arch, rc=result.returncode, stdout=result.stdout, stderr=result.stderr))
    (OUT / 'results.json').write_text(json.dumps(rows, indent=2) + '\n')
    result.check_returncode()
    print(result.stdout, end='', flush=True)
for role in ('client32', 'host32', 'host64'):
    result = subprocess.run([OUT / f'heap-{role}.exe'], capture_output=True, text=True, timeout=20)
    rows.append(dict(role=role, rc=result.returncode, stdout=result.stdout, stderr=result.stderr))
    (OUT / 'results.json').write_text(json.dumps(rows, indent=2) + '\n')
    result.check_returncode()
    print(result.stdout, end='', flush=True)
for role in ('host32', 'host64'):
    guid = str(uuid.uuid4())
    log_path = OUT / ('client-' + role + '-' + guid + '.txt')
    log = log_path.open('w', encoding='utf-8')
    client = subprocess.Popen([OUT / 'heap-client32.exe', guid], stdin=subprocess.PIPE,
                              stdout=log, stderr=subprocess.STDOUT, text=True)
    try:
        deadline = time.monotonic() + 20
        while 'HEAP_READY' not in log_path.read_text(encoding='utf-8'):
            if client.poll() is not None or time.monotonic() >= deadline:
                raise RuntimeError('Client readiness failed: ' + log_path.read_text(encoding='utf-8'))
            time.sleep(0.05)
        fields = []
        for line in log_path.read_text(encoding='utf-8').splitlines():
            if line.startswith('HEAP_ALLOCATION '): fields.extend(line.split()[1:])
        result = subprocess.run([OUT / f'heap-{role}.exe', guid, *fields], capture_output=True, text=True, timeout=20)
        client.communicate('q', timeout=20)
        log.flush(); ct = log_path.read_text(encoding='utf-8'); ce = ''
        rows.append(dict(role=role, cross_process=True, fields=fields, client_rc=client.returncode,
                         host_rc=result.returncode, host=result.stdout, stderr=result.stderr, client=ct, client_stderr=ce))
        (OUT / 'results.json').write_text(json.dumps(rows, indent=2) + '\n')
        result.check_returncode()
        if client.returncode:
            raise RuntimeError(f'Client failure: {ct} {ce}')
        print(result.stdout, end='', flush=True)
    finally:
        if client.poll() is None:
            client.kill()
            client.wait(timeout=5)
        log.close()
print('SHAREDHEAP_NATIVE_PASS', flush=True)
