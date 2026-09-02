[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ApiJar,
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck\classes')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$genericBuild = Join-Path $PSScriptRoot '..\hla-rti-java-tck\build.ps1'
& $genericBuild -ApiJar $ApiJar -OutputDirectory $OutputDirectory
exit $LASTEXITCODE
