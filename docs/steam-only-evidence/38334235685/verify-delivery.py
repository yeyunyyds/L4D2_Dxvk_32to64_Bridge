from pathlib import Path, PurePosixPath
import hashlib
import json
import re
import struct
import sys
import zipfile

sys.path.insert(0, str(Path('scripts').resolve()))
from prepare_ipc_c32 import identity

artifact = Path(sys.argv[1])
commit = sys.argv[2]
run = sys.argv[3]
out = Path('work/steam-only-delivery')
out.mkdir(exist_ok=True)
with zipfile.ZipFile(artifact) as z:
    assert all(not PurePosixPath(n).is_absolute() and '..' not in PurePosixPath(n).parts for n in z.namelist())
    packages = [n for n in z.namelist() if n.startswith('dist/') and n.endswith('.zip')]
    assert len(packages) == 1, packages
    raw = z.read(packages[0])
    digest = hashlib.sha256(raw).hexdigest()
    assert z.read('dist/SHA256SUMS.txt').decode().split()[0] == digest
    source = json.loads(z.read('work/steam-only-source-verification.json'))
    assert source['status'] == 'PASS' and source['experimental_reshade_removed']
    (out/'source-verification.json').write_text(json.dumps(source, indent=2)+'\n')
    for filename, target, count in (
        ('work/ipc-c32-native/c32-native-results.json', 'ipc-raw.json', 252),
        ('work/ipc-c32-native/c32-native-fault-results.json', 'ipc-faults.json', 30),
    ):
        rows = json.loads(z.read(filename))
        assert len(rows) == count
        for batch in ('1', '2', '4', '8', '16', '32'):
            assert sum(r['batch'] == batch for r in rows) == count//6
        if count == 30: assert all(r['rc'] == 0 for r in rows)
        else: assert all(r['client_rc'] == r['host_rc'] == 0 for r in rows)
        (out/target).write_text(json.dumps(rows, indent=2)+'\n')
    package = out / PurePosixPath(packages[0]).name
    package.write_bytes(raw)

with zipfile.ZipFile(package) as z:
    info = json.loads(z.read('BUILD-INFO.json'))
    assert info['source_commit'] == commit
    assert info['native_run_id'] == run
    assert info['protocol_source_sha256'] == identity()
    assert info['build_id'] == 'ldb-ipc-c32-dev+'+identity()[:16]
    assert info['experiment_profile'] == 'steam-only-presenter'
    assert info['reshade_external_input_removed'] and info['bundled_reshade_binaries'] == 0
    assert info['distribution'] == 'experimental' and info['status'] == 'AWAITING GAME VALIDATION'
    assert not info['gameplay_validated']
    assert info['default_batch'] == 1 and info['supported_batches'] == [1, 2, 4, 8, 16, 32]
    assert info['reference_config_host'] == 'x64' and info['reference_presenter_enabled']
    assert not info['default_diagnostics']
    assert all('/O2' in c and 'NDEBUG' in c and 'DEBUGOPT' not in c for c in info['compile_examples'].values())
    expected = {
        'bin/dxvk_d3d9.dll': 0x14c,
        'bin/.l4d2bridge/L4D2Bridge32.exe': 0x14c,
        'bin/.l4d2bridge/L4D2Bridge64.exe': 0x8664,
    }
    assert set(info['files']) == set(expected)
    assert set(n for n in z.namelist() if n.lower().endswith(('.dll', '.exe'))) == set(expected)
    assert not any('reshade' in n.lower() for n in z.namelist())
    forbidden = ('VK_LAYER_LDB_reshade', 'ReShadeLDBConfigureInputV1', 'ReShadeLDBInputBuildV1',
                 'LDBReShadeInputV1', 'server.reshadeExternalInput', 'LDB_EXTERNAL_INPUT')
    binaries = []
    for name, arch in expected.items():
        b = z.read(name)
        sha = hashlib.sha256(b).hexdigest()
        assert sha == info['files'][name]
        pe = struct.unpack_from('<I', b, 0x3c)[0]
        assert b[pe:pe+4] == b'PE\0\0'
        assert struct.unpack_from('<H', b, pe+4)[0] == arch
        assert info['build_id'].encode() in b
        if name.endswith('32.exe'): assert struct.unpack_from('<H', b, pe+22)[0] & 0x20
        for marker in forbidden:
            assert marker.encode() not in b and marker.encode('utf-16le') not in b, (name, marker)
        required = ['L4D2_STEAM_WINDOW event=window-query', 'L4D2_STEAM_WINDOW event=hook-install']
        if name.endswith('.dll'):
            required += ['L4D2_STEAM_INVITE event=request', 'L4D2_STEAM_INVITE event=return',
                         'L4D2_STEAM_INPUT event=steam-capture', 'client.steamWorkshopAvailabilityProxy',
                         'client.steamWindowDiagnostics', 'L4D2_STEAM_WORKSHOP event=availability']
        else:
            required += ['server.steamWindowAdapter', 'server.steamNativeInput',
                         'L4D2_STEAM_INPUT event=native-focus acquired=1',
                         'L4D2_STEAM_WORKSHOP event=host-channel']
        for marker in required: assert marker.encode() in b, (name, marker)
        binaries.append(dict(path=name, machine=hex(arch), sha256=sha, bytes=len(b)))
    assert json.loads(z.read('SOURCE-VERIFICATION.json')) == source
    for name in ('config/bridge.conf', 'config/STEAM-ONLY-EXPERIMENT.conf'):
        config = z.read(name).decode()
        values = dict(re.findall(r'^([^#\s]+)\s*=\s*(\S+)\s*$', config, re.M))
        for key in ('server.presenterWindow', 'server.presenterInput', 'server.steamNativeInput',
                    'server.steamWindowAdapter', 'client.steamOverlayInput', 'server.steamOverlayInput',
                    'client.steamWorkshopAvailabilityProxy', 'client.steamHookChainPassThrough',
                    'client.steamMouseCaptureHandoff', 'client.steamPollChainPassThrough'):
            assert values[key] == 'True', (name, key)
        for key in ('client.testX86Server', 'client.hookMessagePump', 'client.overrideCustomWinHooks',
                    'client.steamInputDiagnostics', 'server.steamInputDiagnostics',
                    'client.steamWindowDiagnostics', 'server.steamWindowDiagnostics', 'client.steamWebDiagnostics'):
            assert values[key] == 'False', (name, key)
        assert 'server.reshadeExternalInput' not in values
    for name in ('LICENSE', 'THIRD_PARTY.md', 'licenses/LICENSE-MIT', 'licenses/ThirdPartyLicenses.txt',
                 'STEAM-ONLY-INSTALL-AND-TEST.md', 'INSTALL-AND-TEST.md'):
        assert z.read(name)

evidence = dict(status='PASS', build=info['build_id'], source_commit=commit, native_run=run,
                zip=package.name, sha256=digest, bytes=len(raw), binaries=binaries,
                ipc_cases=252, fault_cases=30, experimental_reshade_removed=True,
                no_reshade_or_proprietary_binaries_bundled=True, gameplay_validated=False)
(out/'BUILD-INFO.json').write_text(json.dumps(info, indent=2)+'\n')
(out/'artifact-inspection.json').write_text(json.dumps(evidence, indent=2)+'\n')
print(json.dumps(evidence, indent=2))
