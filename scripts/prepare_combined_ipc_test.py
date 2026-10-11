"""Extract actual main/combined production paths, with identical separate callers."""
import hashlib
import json
from pathlib import Path
from generate_ipc_transport_test import generate, ROOT
import subprocess

def main():
    subprocess.run(['python', str(ROOT/'scripts/prepare_buffer_shadow_baseline.py')], check=True)
    baseline = ROOT/'.deps/shadow-crt-baseline' 
    for name, source in [('pr6', baseline), ('va-pr6', baseline), ('combined', ROOT/'.deps/dxvk-remix')]:
        out = ROOT/'.deps'/f'ipc-combined-{name}'
        generate(source, out)
        code = (out/'ipc_transport.cpp').read_text()
        code = '#define IPC_KNOWN_PACKET\n' + code
        seam = code.index('\nnamespace {')
        header = code[:seam]
        header = header.replace('std::atomic<bool> gbBridgeRunning {true};', 'extern std::atomic<bool> gbBridgeRunning;')
        header = header.replace('std::atomic<bool> ipcInjectWaitFailure {false};', 'extern std::atomic<bool> ipcInjectWaitFailure;')
        header = header.replace('bridge_util::Guid gUniqueIdentifier;', 'extern bridge_util::Guid gUniqueIdentifier;')
        body = code[seam:code.index('#include "ipc_transport_cases.cpp"')]
        (out/'ipc_fixture.h').write_text('#pragma once\n'+header)
        globals_ = 'std::atomic<bool> gbBridgeRunning {true};\nstd::atomic<bool> ipcInjectWaitFailure {false};\nbridge_util::Guid gUniqueIdentifier;\n'
        (out/'ipc_command.cpp').write_text('#include "ipc_fixture.h"\n'+globals_+body)
        caller = (ROOT/'tests/ipc_performance_cases.cpp').read_text()
        caller = caller.replace('int wmain(int argc,wchar_t** argv){', 'int wmain(int argc,wchar_t** argv){\n  bridge_data::buildIdentity = "native-production-fixture";')
        (out/'ipc_caller.cpp').write_text('#include "ipc_fixture.h"\n'+caller)
        shadow_source = baseline if name == 'pr6' else ROOT/'.deps/dxvk-remix'
        (out/'client').mkdir(exist_ok=True)
        for filename in ('buffer_shadow.h','buffer_shadow_arena.h','memory_diagnostics.h'):
            original = shadow_source/'bridge/src/client'/filename
            if original.exists():
                contents = original.read_text()
                if filename == 'memory_diagnostics.h':
                    contents = contents.replace('../util/queue_wait_diagnostics.h','../queue_wait_diagnostics.h')
                    contents = contents.replace('../util/runtime_observation.h','../runtime_observation.h')
                (out/'client'/filename).write_text(contents)
        manifest = json.loads((out/'source-manifest.json').read_text())
        manifest.update(lifecycle_caller_sha256=hashlib.sha256((ROOT/'tests/combined_shadow_ipc.cpp').read_bytes()).hexdigest(),shadow_source_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (shadow_source/'bridge/src/client').glob('buffer_shadow*.h')},variant=name, caller='tests/ipc_performance_cases.cpp',
            compilation='MSVC /O2, separate Command/caller translation units, no LTO',
            scope='actual D3D9 transport with integrity dispatch fixture, no backend/GPU',
            caller_sha256=hashlib.sha256((ROOT/'tests/ipc_performance_cases.cpp').read_bytes()).hexdigest())
        (out/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

if __name__ == '__main__':
    main()
