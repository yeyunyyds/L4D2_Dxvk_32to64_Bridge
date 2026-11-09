param([ValidateSet('x86', 'x64')][string]$CompileArchitecture)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root '.deps/dxvk-remix'
. (Join-Path $PSScriptRoot 'assert_native_architecture.ps1')

if (!$CompileArchitecture) {
  foreach ($arch in @('x86', 'x64')) {
    & powershell.exe -NoProfile -File $PSCommandPath -CompileArchitecture $arch
    if ($LASTEXITCODE -ne 0) { throw "Host network policy test failed for $arch" }
  }
  return
}

. (Join-Path $source 'bridge/build_common.ps1')
SetupVS -Platform $CompileArchitecture -VcVarsVer '14.29'
$directory = Join-Path $root ".deps/host-network-$CompileArchitecture"
New-Item -ItemType Directory -Force $directory | Out-Null
Push-Location $directory
try {
  & cl.exe /nologo /std:c++17 /EHsc /O2 /W2 /WX `
    "/I$source/bridge/src/server" "/I$source/submodules/Detours/src" `
    "$root/tests/host_network_policy_test.cpp" "$source/bridge/src/server/host_network_policy.cpp" `
    /Fe:host-network-test.exe /link ws2_32.lib
  if ($LASTEXITCODE -ne 0) { throw "Host network policy compilation failed for $CompileArchitecture" }
  Assert-NativeArchitecture -Path (Join-Path $directory 'host-network-test.exe') -Architecture $CompileArchitecture
  & .\host-network-test.exe
  if ($LASTEXITCODE -ne 0) { throw "Host network policy test failed for $CompileArchitecture" }
} finally {
  Pop-Location
}
