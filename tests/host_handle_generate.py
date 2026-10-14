# SPDX-License-Identifier: MIT
"""Extract the real GET_RES macro, handle lookup, and Query lifecycle dispatches."""
import argparse,re,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from generate_buffer_contract_test import method
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
s=(a.source/'bridge/src/server/main.cpp').read_text(encoding='utf-8');out=[]
if 'static typename Map::mapped_type lookupHostHandle(' in s:
 b,e=method(s,'static typename Map::mapped_type lookupHostHandle(')
 out.append('template<typename Map>\n'+s[b:e])
for name in ['GET_HND','GET_RES']:
 b=s.index('#define '+name+'(');e=b
 while True:
  e=s.index('\n',e)+1
  if not s[b:e].rstrip('\n').endswith('\\'):break
 out.append(s[b:e])
# Null after the macro is observed before a backend call, making the frozen
# missing-handle control deterministic without dereferencing it in the adapter.
out.append('static void macroDispatch() { GET_RES(resource,gpD3DDevices); if (!resource) { throw std::runtime_error("missing host handle continued dispatch"); } ++resource->calls; }')
for name in ['IDirect3DQuery9_Issue','IDirect3DQuery9_Destroy']:
 b=s.index('      case '+name+':');e=s.index('      case ',b+12)
 body=s[b:e].replace('case '+name+':','case 1:',1).replace('IDirect3DQuery9*','Query*')
 out.append('static void dispatchQuery'+name.rsplit('_',1)[1]+'() { switch(1) {\n'+body+'} }')
a.output.mkdir(parents=True,exist_ok=True);(a.output/'actual_host_handles.h').write_text('\n'.join(out),encoding='utf-8')
