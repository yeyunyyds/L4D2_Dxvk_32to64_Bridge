# SPDX-License-Identifier: MIT
# Run production-source input regressions and exact frozen-main controls.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
& python (Join-Path $PSScriptRoot 'prepare_input_baseline.py')
if ($LASTEXITCODE -ne 0) { throw 'Frozen input source extraction failed' }
& python (Join-Path $PSScriptRoot 'fetch_reshade_keyboard_source.py')
if ($LASTEXITCODE -ne 0) { throw 'Pinned ReShade source verification failed' }
$baseline = Join-Path $root '.deps/input-baseline'
& powershell.exe -NoProfile -File (Join-Path $PSScriptRoot 'test_overlay_keyboard_route.ps1') -ControlSource $baseline
if ($LASTEXITCODE -ne 0) { throw 'Keyboard routing or old-source control failed' }
& powershell.exe -NoProfile -File (Join-Path $PSScriptRoot 'test_overlay_ime_result.ps1') -PreFixSource $baseline
if ($LASTEXITCODE -ne 0) { throw 'IME result boundary or old-source control failed' }
& powershell.exe -NoProfile -File (Join-Path $PSScriptRoot 'test_overlay_reshade_keyboard.ps1') -ControlSource $baseline
if ($LASTEXITCODE -ne 0) { throw 'ReShade 6.8 keyboard or old-source control failed' }
