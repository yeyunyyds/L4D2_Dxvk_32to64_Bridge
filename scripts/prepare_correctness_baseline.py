# SPDX-License-Identifier: MIT
"""Reconstruct frozen main 64798c7 from the pinned source, without fetching another SDK."""
from pathlib import Path
import shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
BASE='64798c77e96b05d70720bcd5bedab32aa9a0b7e9'
def main():
 source=ROOT/'.deps/dxvk-remix';out=ROOT/'.deps/correctness-baseline'
 if out.exists(): shutil.rmtree(out)
 out.mkdir(parents=True)
 shutil.copytree(source/'bridge',out/'bridge')
 subprocess.run(['git','init','-q',str(out)],check=True)
 subprocess.run(['git','apply','--reverse',str(ROOT/'patches/l4d2-bridge.patch')],cwd=out,check=True)
 # Checkout fetch-depth=0 is required by this reproducible negative control.
 patch=subprocess.check_output(['git','show',BASE+':patches/l4d2-bridge.patch'],cwd=ROOT)
 subprocess.run(['git','apply','-'],input=patch,cwd=out,check=True)
 print('Frozen main baseline:',BASE)
if __name__=='__main__':main()
