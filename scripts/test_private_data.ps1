# SPDX-License-Identifier: MIT
param([ValidateSet('x86', 'x64')][string]$CompileArchitecture)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$source = Join-Path $repoRoot '.deps/dxvk-remix'
$testDir = Join-Path $repoRoot '.deps/private-data-test'
New-Item -ItemType Directory -Force $testDir | Out-Null
if ($CompileArchitecture) {
  . (Join-Path $source 'bridge/build_common.ps1')
  SetupVS -Platform $CompileArchitecture -VcVarsVer '14.29'
  Push-Location $testDir
  try {
    & cl.exe /nologo /std:c++17 /EHsc /W4 /WX "/I$source/bridge/src/client" "$repoRoot/tests/private_data.cpp" "/Fe:private-data-$CompileArchitecture.exe"
    if ($LASTEXITCODE -ne 0) { throw "PrivateData compile failed for $CompileArchitecture" }
    & (Join-Path $testDir "private-data-$CompileArchitecture.exe")
    if ($LASTEXITCODE -ne 0) { throw "PrivateData test failed for $CompileArchitecture" }
  } finally { Pop-Location }
} else {
  foreach ($arch in @('x86', 'x64')) {
    & powershell.exe -NoProfile -File $PSCommandPath -CompileArchitecture $arch
    if ($LASTEXITCODE -ne 0) { throw "PrivateData test failed for $arch" }
  }
}
