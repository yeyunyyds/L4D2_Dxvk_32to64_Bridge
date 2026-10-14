# SPDX-License-Identifier: MIT
param([ValidateSet('client32','host32','host64')][string]$Role)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root '.deps/dxvk-remix'
$outRoot=Join-Path $root 'work/correctness-sharedheap-native'
if(!$Role){
 foreach($variant in @('none','buffer-discard','surface-discard')) {
  python "$root/tests/sharedheap_prepare.py" --source $source --output "$outRoot/$variant" --control $variant
  if($LASTEXITCODE -ne 0){throw 'SharedHeap production extraction failed'}
 }
 python "$root/tests/sharedheap_prepare.py" --source "$root/.deps/correctness-baseline" --output "$outRoot/frozen"
 if($LASTEXITCODE -ne 0){throw 'Frozen SharedHeap production extraction failed'}
 foreach($compileRole in @('client32','host32','host64')) {
  & powershell.exe -NoProfile -File $PSCommandPath -Role $compileRole
  if($LASTEXITCODE -ne 0){throw "SharedHeap build/test failed: $compileRole"}
 }
 python "$root/tests/sharedheap_runner.py" --output "$outRoot/none"
 if($LASTEXITCODE -ne 0){throw 'SharedHeap cross-process lifecycle failed'}
 exit 0
}
$arch=if($Role -eq 'host64'){'x64'}else{'x86'}
. (Join-Path $source 'bridge/build_common.ps1')
. (Join-Path $PSScriptRoot 'assert_native_architecture.ps1')
SetupVS -Platform $arch -VcVarsVer '14.29'
[string[]]$flags=if($Role -eq 'host32'){@('/DREMIX_BRIDGE_X86_SERVER','/DREMIX_BRIDGE_SERVER')}else{@()}
$out=Join-Path $outRoot 'none'
Push-Location $out
try {
 $exe=Join-Path $out "heap-$Role.exe"
 & cl.exe /nologo /std:c++17 /EHsc /O2 /DNDEBUG /W3 /DNOMINMAX /DWIN32 @flags "/I$out" "$root/tests/sharedheap_contract.cpp" util_sharedheap.cpp util_sharedmemory.cpp "/Fe:$exe" /link /LARGEADDRESSAWARE ole32.lib
 if($LASTEXITCODE -ne 0){throw "SharedHeap compile failed: $Role"}
 Assert-NativeArchitecture -Path $exe -Architecture $arch
}finally{Pop-Location}
if($Role -ne 'client32'){
 $gen=Join-Path $outRoot 'frozen'
 Push-Location $gen
 try {
  $exe=Join-Path $gen "frozen-$Role.exe"
  & cl.exe /nologo /std:c++17 /EHsc /O2 /DNDEBUG /W3 /DNOMINMAX /DWIN32 @flags "/I$gen" "$root/tests/sharedheap_frozen_contract.cpp" util_sharedheap.cpp util_sharedmemory.cpp "/Fe:$exe" /link /LARGEADDRESSAWARE ole32.lib
  if($LASTEXITCODE -ne 0){throw 'Frozen heap compilation failed'}
  python "$PSScriptRoot/run_correctness_case.py" --expect 'unknown heap lookup resolved to allocated data' $exe
  if($LASTEXITCODE -ne 0){throw 'Frozen SharedHeap defect not reproduced'}
 }finally{Pop-Location}
}
if($Role -ne 'host32'){
 foreach($variant in @('none','buffer-discard','surface-discard')) {
  $gen=Join-Path $outRoot $variant
  Push-Location $gen
  try {
   $exe=Join-Path $gen "clientheap-$arch.exe"
   & cl.exe /nologo /std:c++17 /EHsc /O2 /DNDEBUG /W3 /DNOMINMAX /DWIN32 "/I$gen" "/I$source/bridge/src/client" "/I$source/bridge/src/util" "$root/tests/sharedheap_client_contract.cpp" "$source/bridge/src/util/data_diagnostics.cpp" "/Fe:$exe"
   if($LASTEXITCODE -ne 0){throw "Client SharedHeap compilation failed: $variant"}
   Assert-NativeArchitecture -Path $exe -Architecture $arch
   [string[]]$expected=@()
   if($variant -eq 'buffer-discard'){$expected=@('--expect','failed discard lost old buffer')}
   if($variant -eq 'surface-discard'){$expected=@('--expect','surface discard lost old buffer')}
   python "$PSScriptRoot/run_correctness_case.py" @expected $exe
   if($LASTEXITCODE -ne 0){throw "Heap failure contract: $variant"}
  }finally{Pop-Location}
 }
}
