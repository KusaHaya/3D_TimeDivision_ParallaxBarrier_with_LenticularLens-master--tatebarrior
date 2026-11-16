# For the separate experimental PC. Runs only headless GPU/OSC diagnostics.
# Does not register the DLL, open a camera/serial port, or change the display mode.
param([string]$BinaryDirectory = (Join-Path $PSScriptRoot '../build/Release'))
$ErrorActionPreference = 'Stop'
$binaryRoot = (Resolve-Path -LiteralPath $BinaryDirectory).Path
$coreTest = Join-Path $binaryRoot 'projectpb_core_tests.exe'
$gpuTest = Join-Path $binaryRoot 'projectpb_abi_gpu_tests.exe'
$pluginDll = Join-Path $binaryRoot 'DisplayXR-ProjectPB.dll'
foreach ($filePath in @($coreTest,$gpuTest,$pluginDll)) {
    if (-not (Test-Path -LiteralPath $filePath)) { throw "Missing file: $filePath" }
}
& $coreTest
if ($LASTEXITCODE) { throw 'Optical/coordinate core validation failed.' }
& $gpuTest $pluginDll
if ($LASTEXITCODE) { throw 'DLL/GPU/loopback OSC validation failed. Allow local UDP on test ports 39147 and 39148.' }
Write-Output 'PASS: software connection diagnostics. LCD/LED optical timing is still a separate hardware test.'
