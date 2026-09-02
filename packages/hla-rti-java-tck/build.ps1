[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ApiJar,
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\..\out\java-tck\classes')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$api = (Resolve-Path -LiteralPath $ApiJar -ErrorAction Stop).Path
$sourceRoot = Join-Path $PSScriptRoot 'src\main\java'
$output = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null

$sources = @(Get-ChildItem -LiteralPath $sourceRoot -Filter '*.java' -Recurse |
    Select-Object -ExpandProperty FullName
)
if ($sources.Count -eq 0) {
    throw "No Java TCK sources found below $sourceRoot"
}

& javac -encoding UTF-8 -source 11 -target 11 -cp $api -d $output $sources
if ($LASTEXITCODE -ne 0) {
    throw "Java TCK compilation failed with exit code $LASTEXITCODE"
}

Write-Output $output
