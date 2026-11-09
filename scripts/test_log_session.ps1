param([ValidateSet('x86', 'x64')][string]$CompileArchitecture)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root '.deps/dxvk-remix'
. (Join-Path $PSScriptRoot 'assert_native_architecture.ps1')

if (!$CompileArchitecture) {
  foreach ($arch in @('x86', 'x64')) {
    & powershell.exe -NoProfile -File $PSCommandPath -CompileArchitecture $arch
    if ($LASTEXITCODE -ne 0) { throw "Log session test failed for $arch" }
  }
  return
}

. (Join-Path $source 'bridge/build_common.ps1')
SetupVS -Platform $CompileArchitecture -VcVarsVer '14.29'
$directory = Join-Path $root ".deps/log-session-$CompileArchitecture"
New-Item -ItemType Directory -Force $directory | Out-Null
Push-Location $directory
try {
  & cl.exe /nologo /std:c++17 /EHsc /O2 /W2 /WX /utf-8 `
    "/I$source/bridge/src/util" `
    "$root/tests/log_session.cpp" "$source/bridge/src/util/log_session.cpp" `
    /Fe:log-session-test.exe
  if ($LASTEXITCODE -ne 0) { throw "Log session compilation failed for $CompileArchitecture" }
  Assert-NativeArchitecture -Path (Join-Path $directory 'log-session-test.exe') -Architecture $CompileArchitecture
  & .\log-session-test.exe
  if ($LASTEXITCODE -ne 0) { throw "Log session test failed for $CompileArchitecture" }
} finally {
  Pop-Location
}
