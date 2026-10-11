$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$source = Join-Path $repoRoot '.deps/dxvk-remix'
. (Join-Path $source 'bridge/build_common.ps1')
SetupVS -Platform x86 -VcVarsVer '14.29'
python "$PSScriptRoot/prepare_buffer_shadow_baseline.py"
if ($LASTEXITCODE -ne 0) { throw 'Baseline preparation failed' }
$output = Join-Path $repoRoot '.deps/buffer-shadow-va-evidence'
New-Item -ItemType Directory -Force $output | Out-Null
foreach ($variant in @('crt', 'arena')) {
  $production = if ($variant -eq 'crt') { Join-Path $repoRoot '.deps/shadow-crt-baseline' } else { $source }
  $dir = Join-Path $output $variant
  New-Item -ItemType Directory -Force $dir | Out-Null
  python "$PSScriptRoot/generate_buffer_pressure_test.py" --source $production --output "$dir/actual_buffer_pressure.cpp"
  if ($LASTEXITCODE -ne 0) { throw 'Pressure extraction failed' }
  $flags = @()
  if ($variant -eq 'arena') { $flags += '/DLDB_SHADOW_ARENA' }
  Push-Location $dir
  try {
    & cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /WX /DNOMINMAX @flags "/I$production/bridge/src/util" "/I$production/bridge/src/client" "/I$repoRoot/tests" actual_buffer_pressure.cpp "$production/bridge/src/util/data_diagnostics.cpp" /Fe:buffer-pressure.exe /link /LARGEADDRESSAWARE psapi.lib
    if ($LASTEXITCODE -ne 0) { throw 'Pressure compilation failed' }
  } finally { Pop-Location }
}
# Fresh process per sample; alternate order. Both use the same workloads and
# capture adapter. No per-call timer, no simulated FPS, preserve low-AV failures.
for ($repeat = 1; $repeat -le 5; ++$repeat) {
  $order = if ($repeat % 2) { @('crt', 'arena') } else { @('arena', 'crt') }
  foreach ($scenario in @(@('2300','few-large'), @('2300','many-small'), @('800','many-small'), @('800','fragmented'), @('2300','partial-survivors'))) {
    foreach ($variant in $order) {
      $name = "$($scenario[0])-$($scenario[1])-$repeat"
      & "$output/$variant/buffer-pressure.exe" $scenario[0] $scenario[1] 3 | Out-File "$output/$variant/$name.jsonl" -Encoding utf8
      if ($LASTEXITCODE -ne 0) { throw "Pressure scenario failed: $variant $name" }
    }
  }
}
python "$PSScriptRoot/analyze_buffer_shadow_va.py" $output
if ($LASTEXITCODE -ne 0) { throw 'A/B analysis failed' }
