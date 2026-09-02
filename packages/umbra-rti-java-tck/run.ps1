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
    [string]$NativeLibrary = '',
    [string[]]$JvmArgument = @(),
    [string]$ClassesDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck\classes'),
    [string]$CapabilityProfile = '',
    [string]$ResultsPath = '',
    [string]$JUnitPath = '',
    [string]$ProviderId = 'umbra-jni'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$genericRun = Join-Path $PSScriptRoot '..\hla-rti-java-tck\run.ps1'
$forwardedJvmArguments = @($JvmArgument)
if (-not [string]::IsNullOrWhiteSpace($NativeLibrary)) {
    $native = (Resolve-Path -LiteralPath $NativeLibrary -ErrorAction Stop).Path
    $forwardedJvmArguments += "-Dumbra.rti.jni.library=$native"
}
$arguments = @{
    ApiJar = $ApiJar
    ProviderJar = $ProviderJar
    DependencyJar = $DependencyJar
    FomPath = $FomPath
    TimeImplementation = $TimeImplementation
    ClassesDirectory = $ClassesDirectory
    JvmArgument = $forwardedJvmArguments
    ProviderId = $ProviderId
}
if (-not [string]::IsNullOrWhiteSpace($MimPath)) { $arguments.MimPath = $MimPath }
if (-not [string]::IsNullOrWhiteSpace($FactoryName)) { $arguments.FactoryName = $FactoryName }
if (-not [string]::IsNullOrWhiteSpace($CapabilityProfile)) { $arguments.CapabilityProfile = $CapabilityProfile }
if (-not [string]::IsNullOrWhiteSpace($ResultsPath)) { $arguments.ResultsPath = $ResultsPath }
if (-not [string]::IsNullOrWhiteSpace($JUnitPath)) { $arguments.JUnitPath = $JUnitPath }
& $genericRun @arguments
exit $LASTEXITCODE
