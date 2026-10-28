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

$sourceList = Join-Path $output 'javac-sources.args'
$sourceArguments = @(
    foreach ($source in $sources) {
        if ($source.Contains('"')) {
            throw "Java source path contains an unsupported quote character: $source"
        }
        if ($source -match '\s') {
            '"{0}"' -f $source
        } else {
            $source
        }
    }
)
[System.IO.File]::WriteAllLines(
    $sourceList,
    $sourceArguments,
    [System.Text.UTF8Encoding]::new($false)
)

try {
    & javac -encoding UTF-8 -source 11 -target 11 -cp $api -d $output "@$sourceList"
    $javacExitCode = $LASTEXITCODE
} finally {
    Remove-Item -LiteralPath $sourceList -Force -ErrorAction SilentlyContinue
}
if ($javacExitCode -ne 0) {
    throw "Java TCK compilation failed with exit code $javacExitCode"
}

Write-Output $output
