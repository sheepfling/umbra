[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$DirectResults,
    [Parameter(Mandatory = $true)]
    [string]$ApiJar,
    [Parameter(Mandatory = $true)]
    [string[]]$ProviderJar,
    [Parameter(Mandatory = $true)]
    [string]$FactoryName,
    [string]$NativeLibrary = '',
    [string]$OutputPath = (Join-Path $PSScriptRoot '..\..\out\java-tck\jpype-evidence.json'),
    [string]$Python = 'python'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$arguments = @(
    (Join-Path $PSScriptRoot 'jpype_smoke.py'),
    '--direct-results', (Resolve-Path -LiteralPath $DirectResults -ErrorAction Stop).Path,
    '--api-jar', (Resolve-Path -LiteralPath $ApiJar -ErrorAction Stop).Path,
    '--factory-name', $FactoryName,
    '--output', [IO.Path]::GetFullPath($OutputPath)
)
foreach ($jar in $ProviderJar) {
    $arguments += @('--provider-jar', (Resolve-Path -LiteralPath $jar -ErrorAction Stop).Path)
}
if (-not [string]::IsNullOrWhiteSpace($NativeLibrary)) {
    $arguments += @('--native-library', (Resolve-Path -LiteralPath $NativeLibrary -ErrorAction Stop).Path)
}

& $Python @arguments
exit $LASTEXITCODE
