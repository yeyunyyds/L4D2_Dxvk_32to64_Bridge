# SPDX-License-Identifier: MIT
param([ValidateSet('x86','x64')][string]$CompileArchitecture)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root '.deps/dxvk-remix'
$out=Join-Path $root 'work/steam-ime-calls-native'
New-Item -ItemType Directory -Force $out | Out-Null
. (Join-Path $PSScriptRoot 'assert_native_architecture.ps1')
if($CompileArchitecture){
 . (Join-Path $source 'bridge/build_common.ps1')
 SetupVS -Platform $CompileArchitecture -VcVarsVer '14.29'
 Push-Location $out
 try {
  & cl.exe /nologo /std:c++17 /EHsc /W4 /WX /LD "$root/tests/steam_ime_call_fixture.cpp" "/Fe:ime-calls-$CompileArchitecture.dll" /link user32.lib imm32.lib "/DEF:$root/tests/steam_ime_call_fixture.def"
  if($LASTEXITCODE -ne 0){throw 'IME import fixture compile failed'}
  & cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /WX "/I$source/bridge/src/server" "$root/tests/steam_ime_calls.cpp" user32.lib "/Fe:ime-calls-$CompileArchitecture.exe"
  if($LASTEXITCODE -ne 0){throw 'IME observer compile failed'}
  foreach($suffix in @('exe','dll')){Assert-NativeArchitecture -Path "$out/ime-calls-$CompileArchitecture.$suffix" -Architecture $CompileArchitecture}
  foreach($mode in @('disabled','enabled')){
   & "$out/ime-calls-$CompileArchitecture.exe" "$out/ime-calls-$CompileArchitecture.dll" $mode
   if($LASTEXITCODE -ne 0){throw "IME observer fixture failed: $mode"}
  }
 } finally {Pop-Location}
}else{
 foreach($arch in @('x86','x64')){
  & powershell.exe -NoProfile -File $PSCommandPath -CompileArchitecture $arch
  if($LASTEXITCODE -ne 0){throw "IME observer failed: $arch"}
 }
}
