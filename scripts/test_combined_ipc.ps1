param([ValidateSet('client32','host32','host64')][string]$CompileRole,
      [ValidateSet('pr6','va-pr6','combined')][string]$Variant)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$source = Join-Path $repoRoot '.deps/dxvk-remix'
if ($CompileRole) {
  $arch = if ($CompileRole -eq 'host64') { 'x64' } else { 'x86' }
  . (Join-Path $source 'bridge/build_common.ps1')
  SetupVS -Platform $arch -VcVarsVer '14.29'
  $testDir = Join-Path $repoRoot ".deps/ipc-combined-$Variant"
  Push-Location $testDir
  try {
    [string[]]$roleFlags = if ($CompileRole -eq 'host32') { @('/DREMIX_BRIDGE_X86_SERVER','/DREMIX_BRIDGE_SERVER') } else { @() }
    & cl.exe /nologo /std:c++17 /EHsc /O2 /W3 /DNOMINMAX /DWIN32 /DIPC_SEPARATE_CALLER @roleFlags "/I$testDir" "/I$repoRoot/tests" ipc_command.cpp ipc_caller.cpp util_sharedmemory.cpp util_semaphore.cpp data_diagnostics.cpp exception_diagnostics.cpp "/Fe:perf-$CompileRole.exe" /link /LARGEADDRESSAWARE ole32.lib psapi.lib
    if ($LASTEXITCODE -ne 0) { throw "Combined performance compilation failed: $Variant $CompileRole" }
    & cl.exe /nologo /std:c++17 /EHsc /O2 /W3 /DNOMINMAX /DWIN32 @roleFlags "/I$testDir" "/I$repoRoot/tests" ipc_command.cpp "$repoRoot/tests/combined_shadow_ipc.cpp" util_sharedmemory.cpp util_semaphore.cpp data_diagnostics.cpp exception_diagnostics.cpp "/Fe:lifecycle-$CompileRole.exe" /link /LARGEADDRESSAWARE ole32.lib psapi.lib
    if ($LASTEXITCODE -ne 0) { throw "Combined lifecycle compilation failed: $Variant $CompileRole" }
  } finally { Pop-Location }
} else {
  python "$PSScriptRoot/prepare_combined_ipc_test.py"
  if ($LASTEXITCODE -ne 0) { throw 'Combined fixture extraction failed' }
  foreach ($variant in @('pr6','va-pr6','combined')) {
    foreach ($role in @('client32','host32','host64')) {
      & powershell.exe -NoProfile -File $PSCommandPath -Variant $variant -CompileRole $role
      if ($LASTEXITCODE -ne 0) { throw 'Combined performance compilation failed' }
    }
  }
  python "$PSScriptRoot/run_combined_ipc.py"
  if ($LASTEXITCODE -ne 0) { throw 'Combined performance/correctness sample failed' }
  python "$PSScriptRoot/run_combined_shadow_ipc.py"
  if ($LASTEXITCODE -ne 0) { throw 'Actual Shadow-to-IPC lifecycle failed' }
}
