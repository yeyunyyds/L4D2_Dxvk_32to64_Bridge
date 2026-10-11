"""Prepare immutable v1.2.2 allocation baseline from its verified production patch."""
from pathlib import Path
import hashlib
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = 'de74cd7d5dd546bfd4f03ab451d0565f1fff0107'
EXPECTED = 'e8a61ece359546f8b1ca815f3e708b8bb8caef12b3be8f46de0d3b46201a3ee9'
patch = subprocess.check_output(['git', 'show', BASE + ':patches/l4d2-bridge.patch'], cwd=ROOT)
assert hashlib.sha256(patch).hexdigest() == EXPECTED, 'baseline patch changed'
patch_file = ROOT / '.deps/shadow-crt-baseline.patch'
patch_file.write_bytes(patch)
target = ROOT / '.deps/shadow-crt-baseline'
if not target.exists():
    subprocess.run(['git', 'clone', '--shared', str(ROOT / '.deps/dxvk-remix'), str(target)], check=True)
    subprocess.run(['git', 'apply', str(patch_file)], cwd=target, check=True)
else:
    subprocess.run(['git', 'apply', '--reverse', '--check', str(patch_file)], cwd=target, check=True)
print('Frozen CRT baseline:', BASE, EXPECTED)
