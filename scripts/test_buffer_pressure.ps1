# Test environment only; no production forwarding or allocation edits.
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$source = Join-Path $repoRoot '.deps/dxvk-remix'
. (Join-Path $source 'bridge/build_common.ps1')
SetupVS -Platform x86 -VcVarsVer '14.29'
$output = Join-Path $repoRoot '.deps/buffer-pressure-test'
New-Item -ItemType Directory -Force $output | Out-Null
python "$PSScriptRoot/generate_buffer_pressure_test.py" --output "$output/actual_buffer_pressure.cpp"
if ($LASTEXITCODE -ne 0) { throw 'Pressure extraction failed' }
Push-Location $output
try {
  & cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /WX /DNOMINMAX "/I$source/bridge/src/util" "/I$source/bridge/src/client" "/I$repoRoot/tests" actual_buffer_pressure.cpp "$source/bridge/src/util/data_diagnostics.cpp" /Fe:buffer-pressure.exe /link /LARGEADDRESSAWARE psapi.lib
  if ($LASTEXITCODE -ne 0) { throw 'Pressure compilation failed' }
  foreach ($scenario in @(@('2300','few-large'), @('2300','many-small'), @('800','few-large'), @('800','fragmented'))) {
    $name = "$($scenario[0])-$($scenario[1])"
    & .\buffer-pressure.exe $scenario[0] $scenario[1] 3 | Out-File "$name.jsonl" -Encoding utf8
    if ($LASTEXITCODE -ne 0) { throw "Pressure scenario failed: $name" }
  }
  python "$PSScriptRoot/analyze_buffer_pressure.py" $output
  if ($LASTEXITCODE -ne 0) { throw 'Pressure lifecycle analysis failed' }
} finally { Pop-Location }
