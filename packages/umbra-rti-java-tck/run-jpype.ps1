[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string]$DirectResults,
    [Parameter(Mandatory = $true)] [string]$ApiJar,
    [Parameter(Mandatory = $true)] [string[]]$ProviderJar,
    [string[]]$DependencyJar = @(),
    [Parameter(Mandatory = $true)] [string]$FactoryName,
    [string]$NativeLibrary = '',
    [string[]]$JvmArgument = @(),
    [string]$OutputPath = (Join-Path $PSScriptRoot '..\..\out\java-tck\jpype-evidence.json'),
    [string]$Python = 'python'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$forwardedJvmArguments = @($JvmArgument)
if (-not [string]::IsNullOrWhiteSpace($NativeLibrary)) {
    $native = (Resolve-Path -LiteralPath $NativeLibrary -ErrorAction Stop).Path
    $forwardedJvmArguments += "-Dumbra.rti.jni.library=$native"
}
$generic = Join-Path $PSScriptRoot '..\hla-rti-java-tck\run-jpype.ps1'
$arguments = @{
    DirectResults = $DirectResults
    ApiJar = $ApiJar
    ProviderJar = $ProviderJar
    DependencyJar = $DependencyJar
    FactoryName = $FactoryName
    JvmArgument = $forwardedJvmArguments
    OutputPath = $OutputPath
    Python = $Python
}
& $generic @arguments
exit $LASTEXITCODE
