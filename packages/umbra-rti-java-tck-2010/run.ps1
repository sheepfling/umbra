[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string]$ApiJar,
    [Parameter(Mandatory = $true)] [string[]]$ProviderJar,
    [string[]]$DependencyJar = @(),
    [string]$FactoryName,
    [string]$FomPath,
    [string]$MimPath,
    [string]$ClassesDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck-2010\classes'),
    [string]$CapabilityProfile,
    [string]$ResultsPath = (Join-Path $PSScriptRoot '..\..\out\java-tck-2010\results.json')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$api = (Resolve-Path -LiteralPath $ApiJar -ErrorAction Stop).Path
$providers = @(
    foreach ($jar in $ProviderJar) {
        (Resolve-Path -LiteralPath $jar -ErrorAction Stop).Path
    }
)
$dependencies = @(
    foreach ($jar in $DependencyJar) {
        (Resolve-Path -LiteralPath $jar -ErrorAction Stop).Path
    }
)
$classes = (Resolve-Path -LiteralPath $ClassesDirectory -ErrorAction Stop).Path
$results = [IO.Path]::GetFullPath($ResultsPath)
$resultsDirectory = [IO.Path]::GetDirectoryName($results)
if ($resultsDirectory) { New-Item -ItemType Directory -Force -Path $resultsDirectory | Out-Null }
$classpathEntries = @($classes, $api) + $providers + $dependencies
$classpath = $classpathEntries -join [IO.Path]::PathSeparator
$javaArgs = @('-cp', $classpath, ('-Dumbra.rti.tck2010.results=' + $results))
if ($FactoryName) { $javaArgs += '-Dumbra.rti.tck2010.factory=' + $FactoryName }
if ($FomPath) { $javaArgs += '-Dumbra.rti.tck2010.fom=' + (Resolve-Path -LiteralPath $FomPath).Path }
if ($MimPath) { $javaArgs += '-Dumbra.rti.tck2010.mim=' + (Resolve-Path -LiteralPath $MimPath).Path }
if ($CapabilityProfile) { $javaArgs += '-Dumbra.rti.tck2010.capabilityProfile=' + (Resolve-Path -LiteralPath $CapabilityProfile -ErrorAction Stop).Path }
$javaArgs += 'umbra.rti.tck1516e.RtiTck2010Main'
& java @javaArgs
if ($LASTEXITCODE -ne 0) { throw "2010 Java TCK failed with exit code $LASTEXITCODE" }
