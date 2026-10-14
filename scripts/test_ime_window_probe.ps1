# SPDX-License-Identifier: MIT
param([ValidateSet('x86','x64')][string]$CompileArchitecture)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root '.deps/dxvk-remix'
$out=Join-Path $root 'work/ime-window-probe-native'
New-Item -ItemType Directory -Force $out | Out-Null
. (Join-Path $PSScriptRoot 'assert_native_architecture.ps1')
if($CompileArchitecture){
 . (Join-Path $source 'bridge/build_common.ps1')
 SetupVS -Platform $CompileArchitecture -VcVarsVer '14.29'
 $probeBits=if($CompileArchitecture -eq 'x86'){'32'}else{'64'}
 $probeSource=(git -C $root rev-parse HEAD).Trim()
 $probeId='ime-window-probe+'+$probeSource.Substring(0,12)
 ('#define LDB_PROBE_BUILD_ID "'+$probeId+'"') | Set-Content -Encoding ASCII (Join-Path $out 'probe_build_identity.h')
 & cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /WX "/FI$out/probe_build_identity.h" "/I$source/bridge/src/server" "$root/tools/ime_window_probe.cpp" user32.lib ole32.lib "/Fe:$out/ImeWindowProbe$probeBits.exe"
 if($LASTEXITCODE -ne 0){throw 'IME fixture compilation failed'}
 Assert-NativeArchitecture -Path "$out/ImeWindowProbe$probeBits.exe" -Architecture $CompileArchitecture
 # Build-only in architecture-specific subprocess; parent drives native pairs.

}else{
 foreach($arch in @('x86','x64')){
  & powershell.exe -NoProfile -File $PSCommandPath -CompileArchitecture $arch
  if($LASTEXITCODE -ne 0){throw "IME fixture failed: $arch"}
 }
}

if(-not $CompileArchitecture){
 function Run-Probe([string]$Program,[string[]]$Arguments){
  $process=Start-Process -FilePath $Program -ArgumentList $Arguments -NoNewWindow -PassThru
  if(-not $process.WaitForExit(30000)){$process.Kill();throw 'Own probe timed out'}
  $process.Refresh()
  if($process.ExitCode -ne 0){throw "Probe failed: $Program $Arguments code=$($process.ExitCode)"}
 }
 foreach($arch in @('32','64')){
  foreach($case in @(0,1)) {Run-Probe "$out/ImeWindowProbe$arch.exe" @('--selftest','--case',"$case")}
 }
 foreach($child in @('32','64')){
  foreach($case in @(2,3,4,5)){
   Run-Probe "$out/ImeWindowProbe32.exe" @('--selftest-parent',"`"$out/ImeWindowProbe$child.exe`"","$case")
  }
 }
 Write-Output 'PROBE_NATIVE_PASS x86/x64 Unicode synthetic controls, six window cases, x86->x64 and x86->x86 ownership and bounded cleanup; no real composition tested'
}
