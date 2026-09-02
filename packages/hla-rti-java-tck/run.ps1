[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ApiJar,
    [Parameter(Mandatory = $true)]
    [string[]]$ProviderJar,
    [string[]]$DependencyJar = @(),
    [Parameter(Mandatory = $true)]
    [string]$FomPath,
    [string]$MimPath = '',
    [string]$FactoryName = '',
    [string]$TimeImplementation = 'HLAinteger64Time',
    [string[]]$JvmArgument = @(),
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
$dependencies = @(
    foreach ($jar in $DependencyJar) {
        (Resolve-Path -LiteralPath $jar -ErrorAction Stop).Path
    }
)
$runtimeJars = $providers + $dependencies

$javaArguments = @(
    '-cp', (($classes, $api) + $runtimeJars -join [IO.Path]::PathSeparator),
    "-Dhla.rti.tck.fom=$fom",
    "-Dhla.rti.tck.time=$TimeImplementation",
    "-Dhla.rti.tck.apiJar=$api",
    "-Dhla.rti.tck.providerJars=$($providers -join [IO.Path]::PathSeparator)"
)
if (-not [string]::IsNullOrWhiteSpace($FactoryName)) {
    $javaArguments += "-Dhla.rti.tck.factory=$FactoryName"
}
if (-not [string]::IsNullOrWhiteSpace($MimPath)) {
    $mim = (Resolve-Path -LiteralPath $MimPath -ErrorAction Stop).Path
    $javaArguments += "-Dhla.rti.tck.mim=$mim"
}
if (-not [string]::IsNullOrWhiteSpace($CapabilityProfile)) {
    $profile = (Resolve-Path -LiteralPath $CapabilityProfile -ErrorAction Stop).Path
    $javaArguments += "-Dhla.rti.tck.capabilityProfile=$profile"
}
if (-not [string]::IsNullOrWhiteSpace($ResultsPath)) {
    $javaArguments += "-Dhla.rti.tck.results=$([IO.Path]::GetFullPath($ResultsPath))"
}
if (-not [string]::IsNullOrWhiteSpace($JUnitPath)) {
    $javaArguments += "-Dhla.rti.tck.junit=$([IO.Path]::GetFullPath($JUnitPath))"
}
if (-not [string]::IsNullOrWhiteSpace($ProviderId)) {
    $javaArguments += "-Dhla.rti.tck.provider=$ProviderId"
}
foreach ($argument in $JvmArgument) {
    if (-not [string]::IsNullOrWhiteSpace($argument)) {
        $javaArguments += $argument
    }
}
$javaArguments += 'org.hla.rti.tck.RtiTckMain'

& java @javaArguments
exit $LASTEXITCODE
