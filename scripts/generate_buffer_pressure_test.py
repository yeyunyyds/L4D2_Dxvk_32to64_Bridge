"""Reuse the existing exact production template extraction and native CRT allocator."""
from pathlib import Path
import argparse
import hashlib
import json
from generate_buffer_contract_test import generate

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
source = ROOT / '.deps/dxvk-remix'
generate(source, args.output)
text = args.output.read_text()
assert text.count('#include "buffer_contract_cases.cpp"') == 1
text = '#define LDB_PRESSURE_NATIVE_ALLOCATOR\n' + text.replace(
    '#include "buffer_contract_cases.cpp"', '#include "buffer_pressure_cases.cpp"')
args.output.write_text(text)
names = ['bridge/src/client/lockable_buffer.h', 'bridge/src/client/buffer_shadow.h',
         'bridge/src/client/memory_diagnostics.h']
manifest = {name: hashlib.sha256((source / name).read_bytes()).hexdigest() for name in names}
manifest['production_patch_sha256'] = hashlib.sha256((ROOT / 'patches/l4d2-bridge.patch').read_bytes()).hexdigest()
(args.output.parent / 'source-manifest.json').write_text(json.dumps(manifest, indent=2))
