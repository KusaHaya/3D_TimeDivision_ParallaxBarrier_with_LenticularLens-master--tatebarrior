# Run explicitly from an elevated PowerShell after building. This script does
# not change the OpenXR ActiveRuntime, start a service, enable output, or open USB.
# It registers only the projectpb development DLL; no runtime files are replaced.
param([string]$Binary = (Join-Path $PSScriptRoot '../build/Release/DisplayXR-ProjectPB.dll'))
$ErrorActionPreference = 'Stop'
$dllPath = (Resolve-Path -LiteralPath $Binary).Path
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Use an elevated 64-bit PowerShell to register this development DLL.'
}
if (-not [Environment]::Is64BitProcess) { throw 'Use 64-bit PowerShell.' }
$pluginKey = 'HKLM:\Software\DisplayXR\DisplayProcessors\projectpb'
New-Item -Path $pluginKey -Force | Out-Null
New-ItemProperty -Path $pluginKey -Name Binary -Value $dllPath -PropertyType String -Force | Out-Null
New-ItemProperty -Path $pluginKey -Name DisplayName -Value 'ProjectPB fixed-phase diagnostic' -PropertyType String -Force | Out-Null
New-ItemProperty -Path $pluginKey -Name Vendor -Value 'ProjectPB research' -PropertyType String -Force | Out-Null
New-ItemProperty -Path $pluginKey -Name ProbeOrder -Value 75 -PropertyType DWord -Force | Out-Null
New-ItemProperty -Path $pluginKey -Name Version -Value '0.1.0' -PropertyType String -Force | Out-Null
Write-Output "Registered development DLL: $dllPath"
Write-Output 'Output remains opt-in (ENABLE defaults to 0). Configure it with displayxr-cli dp set.'
