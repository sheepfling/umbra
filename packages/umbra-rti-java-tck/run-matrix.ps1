[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ConfigurationPath,
    [string]$ClassesDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck\classes'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck\matrix')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$configurationFile = (Resolve-Path -LiteralPath $ConfigurationPath -ErrorAction Stop).Path
$configurations = Get-Content -LiteralPath $configurationFile -Raw | ConvertFrom-Json
if ($configurations -isnot [System.Array]) {
    $configurations = @($configurations)
}
if (@($configurations).Count -lt 2) {
    throw 'The Java TCK matrix requires at least two provider configurations'
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$runScript = Join-Path $PSScriptRoot 'run.ps1'
foreach ($configuration in @($configurations)) {
    foreach ($required in @('id', 'apiJar', 'providerJar', 'factoryName', 'fomPath', 'capabilityProfile')) {
        if ([string]::IsNullOrWhiteSpace([string]$configuration.$required)) {
            throw "Provider configuration is missing '$required'"
        }
    }
    $providerJars = @($configuration.providerJar)
    $resultPath = Join-Path $OutputDirectory ("{0}.results.json" -f $configuration.id)
    $junitPath = Join-Path $OutputDirectory ("{0}.junit.xml" -f $configuration.id)
    $arguments = @{
        ApiJar = [string]$configuration.apiJar
        ProviderJar = $providerJars
        FactoryName = [string]$configuration.factoryName
        FomPath = [string]$configuration.fomPath
        CapabilityProfile = [string]$configuration.capabilityProfile
        ClassesDirectory = $ClassesDirectory
        ResultsPath = $resultPath
        JUnitPath = $junitPath
        ProviderId = [string]$configuration.id
    }
    $mimProperty = $configuration.PSObject.Properties['mimPath']
    if ($null -ne $mimProperty -and
        -not [string]::IsNullOrWhiteSpace([string]$mimProperty.Value)) {
        $arguments.MimPath = [string]$mimProperty.Value
    }
    $nativeLibraryProperty = $configuration.PSObject.Properties['nativeLibrary']
    if ($null -ne $nativeLibraryProperty -and
        -not [string]::IsNullOrWhiteSpace([string]$nativeLibraryProperty.Value)) {
        $arguments.NativeLibrary = [string]$nativeLibraryProperty.Value
    }
    & $runScript @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Java TCK failed for provider $($configuration.id) with exit code $LASTEXITCODE"
    }
}

Write-Output ([System.IO.Path]::GetFullPath($OutputDirectory))
