# SPDX-License-Identifier: MIT
param([ValidateSet('x86','x64')][string]$CompileArchitecture)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root '.deps/dxvk-remix'
$out=Join-Path $root 'work/steam-ime-native-thread'
New-Item -ItemType Directory -Force $out | Out-Null
. (Join-Path $PSScriptRoot 'assert_native_architecture.ps1')
if($CompileArchitecture){
 . (Join-Path $source 'bridge/build_common.ps1')
 SetupVS -Platform $CompileArchitecture -VcVarsVer '14.29'
 & cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /WX "/I$source/bridge/src/server" "$root/tests/steam_ime_native.cpp" user32.lib ole32.lib "/Fe:$out/ime-native-$CompileArchitecture.exe"
 if($LASTEXITCODE -ne 0){throw 'IME fixture compilation failed'}
 Assert-NativeArchitecture -Path "$out/ime-native-$CompileArchitecture.exe" -Architecture $CompileArchitecture
 foreach($mode in @('disabled','sta','mta')){
  & "$out/ime-native-$CompileArchitecture.exe" $mode
  if($LASTEXITCODE -ne 0){throw "IME native lifetime failed: $mode"}
 }
}else{
 foreach($arch in @('x86','x64')){
  & powershell.exe -NoProfile -File $PSCommandPath -CompileArchitecture $arch
  if($LASTEXITCODE -ne 0){throw "IME fixture failed: $arch"}
 }
}
