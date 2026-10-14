# SPDX-License-Identifier: MIT
param(
 [ValidateSet('query','shader','texture','sharedheap','host','all')][string]$Stage='all',
 [ValidateSet('x86','x64')][string]$Platform
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root '.deps/dxvk-remix'
$baseline=Join-Path $root '.deps/correctness-baseline'
if (!$Platform) {
 python "$PSScriptRoot/prepare_correctness_baseline.py"
 if ($LASTEXITCODE -ne 0) {throw 'Frozen main preparation failed'}
 foreach($arch in @('x86','x64')) {
  & powershell.exe -NoProfile -File $PSCommandPath -Stage $Stage -Platform $arch
  if($LASTEXITCODE -ne 0){throw "Correctness stage failed: $Stage $arch"}
 }
 exit 0
}
. (Join-Path $source 'bridge/build_common.ps1')
. (Join-Path $PSScriptRoot 'assert_native_architecture.ps1')
SetupVS -Platform $Platform -VcVarsVer '14.29'
$out=Join-Path $root "work/correctness-$Stage-$Platform"
New-Item -ItemType Directory -Force $out | Out-Null
function Run-Case($exe,$caseArgs,$expected='') {
 [string[]]$options=@()
 if($expected){$options+=@('--expect',$expected)}
 python "$PSScriptRoot/run_correctness_case.py" @options $exe @caseArgs
 if($LASTEXITCODE -ne 0){throw "Native case failed: $exe $caseArgs"}
}
function Client-Case($inputSource,$control,$caseStage,$expected='') {
 $gen=Join-Path $out "client-$caseStage-$control-$(Split-Path $inputSource -Leaf)"
 python "$root/tests/client_parameter_contract_generate.py" --source $inputSource --output $gen --control $control
 if($LASTEXITCODE -ne 0){throw 'Client method extraction failed'}
 $exe=Join-Path $gen 'client.exe'
 Push-Location $gen
 try {
  & cl.exe /nologo /std:c++17 /EHsc /O2 /W3 /DNDEBUG /DNOMINMAX "/I$gen" "$root/tests/client_parameter_contract.cpp" "/Fe:$exe"
  if($LASTEXITCODE -ne 0){throw 'Client contract compilation failed'}
  Assert-NativeArchitecture -Path $exe -Architecture $Platform
  Run-Case $exe @($caseStage) $expected
 }finally{Pop-Location}
}
function Host-Query($inputSource,$control,$expected='') {
 $gen=Join-Path $out "host-query-$control-$(Split-Path $inputSource -Leaf)"
 python "$root/tests/query_host_generate.py" --source $inputSource --output $gen --control $control
 if($LASTEXITCODE -ne 0){throw 'Host Query extraction failed'}
 $exe=Join-Path $gen 'host.exe'
 Push-Location $gen
 try {
  & cl.exe /nologo /std:c++17 /EHsc /O2 /W3 /DNDEBUG /DNOMINMAX "/I$gen" "$root/tests/query_host_contract.cpp" "/Fe:$exe"
  if($LASTEXITCODE -ne 0){throw 'Host Query compilation failed'}
  Assert-NativeArchitecture -Path $exe -Architecture $Platform
  Run-Case $exe @() $expected
 }finally{Pop-Location}
}
Client-Case $source none query
Client-Case $baseline none query 'query SDK result size mismatch'
Client-Case $source query query 'pending query overwrote caller data'
Client-Case $source pixel-size query 'query SDK result size mismatch'
Host-Query $source none
Host-Query $baseline none 'pending query leaked an uninitialized or stale payload'
Host-Query $source pending 'pending query leaked an uninitialized or stale payload'

if($Stage -ne 'query') {
 Client-Case $source none shader
 Client-Case $baseline none shader 'wrapped constant range was accepted'
 Client-Case $source range shader 'wrapped constant range was accepted'
}
