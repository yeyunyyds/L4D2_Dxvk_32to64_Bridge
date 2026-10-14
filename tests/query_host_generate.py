# SPDX-License-Identifier: MIT
"""Extract the complete production Host Query GetData dispatch case."""
import argparse
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--control',choices=['none','pending'],default='none');a=p.parse_args()
s=(a.source/'bridge/src/server/main.cpp').read_text(encoding='utf-8')
start=s.index('      case IDirect3DQuery9_GetData:\n');end=s.index('      /*\n       * Other commands',start)
s=s[start:end].replace('case IDirect3DQuery9_GetData:', 'case 1:',1)
if a.control=='pending':
 assert s.count('if (hresult == S_OK && dwSize > 0)')==1
 s=s.replace('if (hresult == S_OK && dwSize > 0)','if (SUCCEEDED(hresult) && dwSize > 0)')
a.output.mkdir(parents=True,exist_ok=True)
(a.output/'host_query_extracted.h').write_text('static void extractedHostQuery() { switch (1) {\n'+s+'} }\n',encoding='utf-8')
