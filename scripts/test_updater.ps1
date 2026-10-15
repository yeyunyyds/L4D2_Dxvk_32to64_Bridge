# SPDX-License-Identifier: MIT
param([ValidateSet('x86','x64')][string]$CompileArchitecture)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=Join-Path $root 'work/updater-native'
New-Item -ItemType Directory -Force $out | Out-Null
if($CompileArchitecture){
  . (Join-Path $root '.deps/dxvk-remix/bridge/build_common.ps1')
  SetupVS -Platform $CompileArchitecture -VcVarsVer '14.29'
  & cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /WX /utf-8 "/I$root/.deps/dxvk-remix/bridge/src/server" "$root/tests/updater_session.cpp" user32.lib advapi32.lib "/Fe:$out/fixture-$CompileArchitecture.exe"
  if($LASTEXITCODE -ne 0){throw 'Native updater fixture failed'}
  exit 0
}
$csc=Join-Path $env:WINDIR 'Microsoft.NET/Framework/v4.0.30319/csc.exe'
$refs=@('/r:System.IO.Compression.dll','/r:System.IO.Compression.FileSystem.dll','/r:System.Web.Extensions.dll','/r:System.Windows.Forms.dll')
& $csc /nologo /target:exe /platform:x86 /optimize+ /warnaserror+ $refs "/out:$out/LDBupdater.exe" "$root/updater/UpdateCore.cs" "$root/updater/Program.cs"
if($LASTEXITCODE -ne 0){throw 'Updater compilation failed'}
& $csc /nologo /target:exe /platform:x86 /optimize+ /warnaserror+ $refs "/out:$out/updater-core-tests.exe" "$root/updater/UpdateCore.cs" "$root/tests/updater_core.cs"
if($LASTEXITCODE -ne 0){throw 'Updater core fixture compilation failed'}
& "$out/updater-core-tests.exe"
if($LASTEXITCODE -ne 0){throw 'Updater core regression failed'}
foreach($arch in @('x86','x64')){
  & powershell.exe -NoProfile -File $PSCommandPath -CompileArchitecture $arch
  if($LASTEXITCODE -ne 0){throw 'Updater cross-bitness fixture compilation failed'}
}
python "$root/scripts/test_updater_sessions.py"
if($LASTEXITCODE -ne 0){throw 'Updater process/session regression failed'}
