# SPDX-License-Identifier: MIT
# Copyright (c) 2026 yeyunyyds.
param([ValidateSet('x86', 'x64')][string]$CompileArchitecture)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'assert_native_architecture.ps1')
$source = Join-Path $repoRoot '.deps/dxvk-remix'

if ($CompileArchitecture) {
  . (Join-Path $source 'bridge/build_common.ps1')
  SetupVS -Platform $CompileArchitecture -VcVarsVer '14.29'
  $testDir = Join-Path $repoRoot ".deps/host-plugin-test/$CompileArchitecture"
  $pluginDir = Join-Path $testDir 'plugins'
  New-Item -ItemType Directory -Force $pluginDir | Out-Null
  Push-Location $testDir
  try {
    & cl.exe /nologo /std:c++17 /EHsc /W4 /WX /LD "/I$source/bridge/include" "$repoRoot/tests/host_plugin_fake.cpp" "/Fe:$pluginDir/ldb-host-plugin-test.dll" /link "/DEF:$repoRoot/tests/host_plugin_fake.def"
    if ($LASTEXITCODE -ne 0) { throw "Host plugin fixture compilation failed for $CompileArchitecture" }
    & cl.exe /nologo /std:c++17 /EHsc /W4 /WX "/I$source/bridge/include" "/I$source/bridge/src/server" "$repoRoot/tests/host_plugins.cpp" "/Fe:host-plugins-$CompileArchitecture.exe"
    if ($LASTEXITCODE -ne 0) { throw "Host plugin manager compilation failed for $CompileArchitecture" }
    Assert-NativeArchitecture -Path (Join-Path $pluginDir 'ldb-host-plugin-test.dll') -Architecture $CompileArchitecture
    Assert-NativeArchitecture -Path (Join-Path $testDir "host-plugins-$CompileArchitecture.exe") -Architecture $CompileArchitecture
    & (Join-Path $testDir "host-plugins-$CompileArchitecture.exe")
    if ($LASTEXITCODE -ne 0) { throw "Host plugin manager test failed for $CompileArchitecture" }
  } finally { Pop-Location }
} else {
  foreach ($arch in @('x86', 'x64')) {
    & powershell.exe -NoProfile -File $PSCommandPath -CompileArchitecture $arch
    if ($LASTEXITCODE -ne 0) { throw "Host plugin tests failed for $arch" }
  }
}
