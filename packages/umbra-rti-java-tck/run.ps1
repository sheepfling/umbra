[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ApiJar,
    [Parameter(Mandatory = $true)]
    [string[]]$ProviderJar,
    [Parameter(Mandatory = $true)]
    [string]$FomPath,
    [string]$MimPath = '',
    [string]$FactoryName = '',
    [string]$TimeImplementation = 'HLAinteger64Time',
    [string]$NativeLibrary = '',
    [string]$ClassesDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck\classes'),
    [string]$CapabilityProfile = '',
    [string]$ResultsPath = '',
    [string]$JUnitPath = '',
    [string]$ProviderId = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$api = (Resolve-Path -LiteralPath $ApiJar -ErrorAction Stop).Path
$fom = (Resolve-Path -LiteralPath $FomPath -ErrorAction Stop).Path
$classes = (Resolve-Path -LiteralPath $ClassesDirectory -ErrorAction Stop).Path
$providers = @(
    foreach ($jar in $ProviderJar) {
        (Resolve-Path -LiteralPath $jar -ErrorAction Stop).Path
    }
)

$javaArguments = @(
    '-cp', (($classes, $api) + $providers -join [IO.Path]::PathSeparator),
    "-Dumbra.rti.tck.fom=$fom",
    "-Dumbra.rti.tck.time=$TimeImplementation",
    "-Dumbra.rti.tck.apiJar=$api",
    "-Dumbra.rti.tck.providerJars=$($providers -join [IO.Path]::PathSeparator)"
)
if (-not [string]::IsNullOrWhiteSpace($FactoryName)) {
    $javaArguments += "-Dumbra.rti.tck.factory=$FactoryName"
}
if (-not [string]::IsNullOrWhiteSpace($MimPath)) {
    $mim = (Resolve-Path -LiteralPath $MimPath -ErrorAction Stop).Path
    $javaArguments += "-Dumbra.rti.tck.mim=$mim"
}
if (-not [string]::IsNullOrWhiteSpace($NativeLibrary)) {
    $native = (Resolve-Path -LiteralPath $NativeLibrary -ErrorAction Stop).Path
    $javaArguments += "-Dumbra.rti.jni.library=$native"
}
if (-not [string]::IsNullOrWhiteSpace($CapabilityProfile)) {
    $profile = (Resolve-Path -LiteralPath $CapabilityProfile -ErrorAction Stop).Path
    $javaArguments += "-Dumbra.rti.tck.capabilityProfile=$profile"
}
if (-not [string]::IsNullOrWhiteSpace($ResultsPath)) {
    $javaArguments += "-Dumbra.rti.tck.results=$([IO.Path]::GetFullPath($ResultsPath))"
}
if (-not [string]::IsNullOrWhiteSpace($JUnitPath)) {
    $javaArguments += "-Dumbra.rti.tck.junit=$([IO.Path]::GetFullPath($JUnitPath))"
}
if (-not [string]::IsNullOrWhiteSpace($ProviderId)) {
    $javaArguments += "-Dumbra.rti.tck.provider=$ProviderId"
}
$javaArguments += 'umbra.rti.tck.RtiTckMain'

& java @javaArguments
exit $LASTEXITCODE
