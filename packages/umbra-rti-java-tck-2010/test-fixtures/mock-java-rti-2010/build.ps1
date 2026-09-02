[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string]$ApiJar,
    [string]$OutputDirectory = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$api = (Resolve-Path -LiteralPath $ApiJar -ErrorAction Stop).Path
$OutputDirectory = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $PSScriptRoot 'build' }
$sourceRoot = Join-Path $PSScriptRoot 'src\main\java'
$resourceRoot = Join-Path $PSScriptRoot 'src\main\resources'
$output = [System.IO.Path]::GetFullPath($OutputDirectory)
$classes = Join-Path $output 'classes'
New-Item -ItemType Directory -Force -Path $classes | Out-Null
$sources = @(Get-ChildItem -LiteralPath $sourceRoot -Filter '*.java' -Recurse |
    Select-Object -ExpandProperty FullName)
& javac -encoding UTF-8 -source 11 -target 11 -cp $api -d $classes $sources
if ($LASTEXITCODE -ne 0) { throw "2010 mock Java compile failed with exit code $LASTEXITCODE" }
$serviceDirectory = Join-Path $classes 'META-INF\services'
New-Item -ItemType Directory -Force -Path $serviceDirectory | Out-Null
Copy-Item -LiteralPath (Join-Path $resourceRoot 'META-INF\services\hla.rti1516e.RtiFactory') `
    -Destination $serviceDirectory -Force
$jar = Join-Path $output 'umbra-mock-java-rti-1516e.jar'
if (Test-Path -LiteralPath $jar) { Remove-Item -LiteralPath $jar -Force }
& jar cf $jar -C $classes .
if ($LASTEXITCODE -ne 0) { throw "2010 mock Java jar creation failed with exit code $LASTEXITCODE" }
Write-Output $jar
