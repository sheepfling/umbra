[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string]$ApiJar,
    [Parameter(Mandatory = $true)] [string[]]$ProviderJar,
    [string[]]$DependencyJar = @(),
    [string]$FactoryName,
    [string]$FomPath,
    [string]$MimPath,
    [string]$JvmPath,
    [string[]]$JvmOption = @(),
    [string]$CapabilityProfile,
    [string]$PythonPath = "python",
    [string]$ResultsPath = (Join-Path $PSScriptRoot '..\..\out\java-tck-2010\python-vendor-smoke.json'),
    [string]$ProviderLabel = "vendor-java-2010"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$api = (Resolve-Path -LiteralPath $ApiJar -ErrorAction Stop).Path
$providers = @(
    foreach ($path in $ProviderJar) {
        (Resolve-Path -LiteralPath $path -ErrorAction Stop).Path
    }
)
$capabilityProfilePath = $null
if ($CapabilityProfile) { $capabilityProfilePath = (Resolve-Path -LiteralPath $CapabilityProfile -ErrorAction Stop).Path }
$fom = $null
if ($FomPath) { $fom = (Resolve-Path -LiteralPath $FomPath -ErrorAction Stop).Path }
$mim = $null
if ($MimPath) { $mim = (Resolve-Path -LiteralPath $MimPath -ErrorAction Stop).Path }
$dependencies = @(
    foreach ($path in $DependencyJar) {
        (Resolve-Path -LiteralPath $path -ErrorAction Stop).Path
    }
)
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$smoke = Join-Path $PSScriptRoot 'jpype_smoke.py'
$results = [IO.Path]::GetFullPath($ResultsPath)
$resultsDirectory = [IO.Path]::GetDirectoryName($results)
if (-not $resultsDirectory) { $resultsDirectory = (Get-Location).Path }
if ($resultsDirectory) { New-Item -ItemType Directory -Force -Path $resultsDirectory | Out-Null }

# Keep the runner source-checkout friendly; a caller-supplied PYTHONPATH is
# appended so additional optional dependencies (such as JPype) remain visible.
$sourceRoots = @(
    (Join-Path $root 'packages\umbra-rti-api\src'),
    (Join-Path $root 'packages\umbra-rti-jpype\src')
)
if ($env:PYTHONPATH) { $sourceRoots += $env:PYTHONPATH }
$oldPythonPath = $env:PYTHONPATH
$env:PYTHONPATH = ($sourceRoots -join [IO.Path]::PathSeparator)

try {
    if ($capabilityProfilePath) {
        $profileVerifier = Join-Path $root 'tools\verify_1516e_capability_profile.py'
        & $PythonPath $profileVerifier '--profile' $capabilityProfilePath
        if ($LASTEXITCODE -ne 0) { throw "Python 2010 capability-profile verification failed with exit code $LASTEXITCODE" }
    }

    $preflight = Join-Path $root 'tools\verify_1516e_provider_jar.py'
    $preflightArgs = @(
        $preflight,
        '--api-jar', $api
    )
    foreach ($provider in $providers) {
        $preflightArgs += @('--provider-jar', $provider)
    }
    foreach ($dependency in $dependencies) {
        $preflightArgs += @('--dependency-jar', $dependency)
    }
    & $PythonPath @preflightArgs
    if ($LASTEXITCODE -ne 0) { throw "Python 2010 provider-JAR preflight failed with exit code $LASTEXITCODE" }

    $pythonArgs = @(
        $smoke,
        '--api-jar', $api,
        '--results', $results
    )
    foreach ($provider in $providers) {
        $pythonArgs += @('--provider-jar', $provider)
    }
    if ($FactoryName) { $pythonArgs += @('--factory-name', $FactoryName) }
    if ($fom) { $pythonArgs += @('--fom-path', $fom) }
    if ($mim) { $pythonArgs += @('--mim-path', $mim) }
    if ($capabilityProfilePath) { $pythonArgs += @('--capability-profile', $capabilityProfilePath) }
    if ($JvmPath) { $pythonArgs += @('--jvm-path', $JvmPath) }
    foreach ($dependency in $dependencies) {
        $pythonArgs += @('--dependency-jar', $dependency)
    }
    foreach ($option in $JvmOption) {
        # Use the equals form because JVM options themselves begin with '-'
        # and argparse would otherwise interpret them as new switches.
        $pythonArgs += ('--jvm-option=' + $option)
    }
    & $PythonPath @pythonArgs
    if ($LASTEXITCODE -ne 0) { throw "Python 2010 JPype smoke failed with exit code $LASTEXITCODE" }

    $verifier = Join-Path $root 'tools\verify_1516e_python_tck_results.py'
    & $PythonPath $verifier $results
    if ($LASTEXITCODE -ne 0) { throw "Python 2010 result verification failed with exit code $LASTEXITCODE" }

    $exporter = Join-Path $root 'tools\python_tck_2010.py'
    $complianceResults = Join-Path $resultsDirectory (([IO.Path]::GetFileNameWithoutExtension($results)) + '-compliance.json')
    & $PythonPath $exporter $results '--provider' $ProviderLabel '--output' $complianceResults
    if ($LASTEXITCODE -ne 0) { throw "Python 2010 result export failed with exit code $LASTEXITCODE" }
}
finally {
    $env:PYTHONPATH = $oldPythonPath
}
