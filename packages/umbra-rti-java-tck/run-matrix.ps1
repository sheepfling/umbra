[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string]$ConfigurationPath,
    [string]$ClassesDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck\classes'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck\matrix')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot '..\hla-rti-java-tck\run-matrix.ps1') @PSBoundParameters
exit $LASTEXITCODE
