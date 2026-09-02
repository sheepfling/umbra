[CmdletBinding()]
param(
    [string]$NativePackageDirectory = (Join-Path $PSScriptRoot '..\..\out\native-2010-install'),
    [string]$CapabilityProfile = (Join-Path $PSScriptRoot '..\umbra-rti-java-tck-2010\profiles\native-2010-provider.properties'),
    [string]$PythonPath = 'python',
    [string]$ResultsPath = (Join-Path $PSScriptRoot '..\..\out\java-tck-2010\python-native-2010.json'),
    [string]$ProviderLabel = 'UmbraNative2010'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$native = (Resolve-Path -LiteralPath $NativePackageDirectory -ErrorAction Stop).Path
$profile = (Resolve-Path -LiteralPath $CapabilityProfile -ErrorAction Stop).Path
$results = [IO.Path]::GetFullPath($ResultsPath)
$resultsDirectory = [IO.Path]::GetDirectoryName($results)
if ($resultsDirectory) { New-Item -ItemType Directory -Force -Path $resultsDirectory | Out-Null }

$sourceRoots = @(
    (Join-Path $root 'packages\umbra-rti-api\src'),
    (Join-Path $root 'packages\umbra-rti-jpype\src'),
    (Join-Path $root 'packages\umbra-rti-java-tck-2010'),
    $native
)
if ($env:PYTHONPATH) { $sourceRoots += $env:PYTHONPATH }
$oldPythonPath = $env:PYTHONPATH
$env:PYTHONPATH = ($sourceRoots -join [IO.Path]::PathSeparator)

try {
    & $PythonPath (Join-Path $root 'tools\verify_1516e_python_surface.py') '--check-native'
    if ($LASTEXITCODE -ne 0) { throw "2010 native Python surface verification failed with exit code $LASTEXITCODE" }

    & $PythonPath (Join-Path $root 'tools\verify_1516e_capability_profile.py') '--profile' $profile
    if ($LASTEXITCODE -ne 0) { throw "2010 native capability-profile verification failed with exit code $LASTEXITCODE" }

    & $PythonPath (Join-Path $root 'packages\umbra-rti-java-tck-2010\jpype_smoke.py') `
        '--native-provider' '--capability-profile' $profile '--results' $results
    if ($LASTEXITCODE -ne 0) { throw "2010 native Python TCK failed with exit code $LASTEXITCODE" }

    & $PythonPath (Join-Path $root 'tools\verify_1516e_python_tck_results.py') $results
    if ($LASTEXITCODE -ne 0) { throw "2010 native Python result verification failed with exit code $LASTEXITCODE" }

    $complianceResults = Join-Path $resultsDirectory (([IO.Path]::GetFileNameWithoutExtension($results)) + '-compliance.json')
    & $PythonPath (Join-Path $root 'tools\python_tck_2010.py') $results '--provider' $ProviderLabel '--output' $complianceResults
    if ($LASTEXITCODE -ne 0) { throw "2010 native Python result export failed with exit code $LASTEXITCODE" }
}
finally {
    $env:PYTHONPATH = $oldPythonPath
}
