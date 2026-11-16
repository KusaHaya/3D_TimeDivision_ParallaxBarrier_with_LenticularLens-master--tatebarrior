param(
    [Parameter(Mandatory=$true)][string]$RuntimeSource,
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$pluginRoot = Split-Path $PSScriptRoot -Parent
$runtimeRoot = (Resolve-Path -LiteralPath $RuntimeSource).Path
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) { $cmakePath = $cmakeCommand.Source }
else {
    $cmakePath = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (-not (Test-Path -LiteralPath $cmakePath)) { throw 'Install CMake or add the Visual Studio CMake directory to PATH.' }
}
$buildRoot = Join-Path $pluginRoot 'build'
& $cmakePath -S $pluginRoot -B $buildRoot -G 'Visual Studio 17 2022' -A x64 "-DDISPLAYXR_RUNTIME_SOURCE_DIR=$runtimeRoot"
if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
# Single MSBuild node also works in restricted Windows process environments.
& $cmakePath --build $buildRoot --config $Configuration -- /m:1 /nr:false
if ($LASTEXITCODE) { throw 'Build failed.' }
$ctestPath = Join-Path (Split-Path $cmakePath -Parent) 'ctest.exe'
& $ctestPath --test-dir $buildRoot -C $Configuration --output-on-failure
if ($LASTEXITCODE) { throw 'Validation failed (the OSC test requires local UDP networking).' }
