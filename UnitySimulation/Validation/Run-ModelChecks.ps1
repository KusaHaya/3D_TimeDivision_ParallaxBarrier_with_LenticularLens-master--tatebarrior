$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    dotnet run --project ModelTests.csproj
    if ($LASTEXITCODE -ne 0) { throw 'Model checks failed.' }
} finally {
    Pop-Location
}
