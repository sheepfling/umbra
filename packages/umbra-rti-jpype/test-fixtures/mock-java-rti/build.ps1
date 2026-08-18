[CmdletBinding()]
param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot "build"),
    [switch]$RunSmokeTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$classesDirectory = Join-Path $OutputDirectory "classes"
$jarPath = Join-Path $OutputDirectory "umbra-mock-java-rti.jar"
$sourceDirectory = Join-Path $PSScriptRoot "src\main\java"
$resourceDirectory = Join-Path $PSScriptRoot "src\main\resources"

New-Item -ItemType Directory -Force -Path $classesDirectory | Out-Null
$sources = Get-ChildItem -Path $sourceDirectory -Filter "*.java" -File -Recurse |
    Select-Object -ExpandProperty FullName
if ($sources.Count -eq 0) {
    throw "No Java sources found below $sourceDirectory"
}

& javac -d $classesDirectory @sources
if ($LASTEXITCODE -ne 0) {
    throw "javac failed with exit code $LASTEXITCODE"
}

Copy-Item -Path (Join-Path $resourceDirectory "*") -Destination $classesDirectory -Recurse -Force
& jar --create --file $jarPath -C $classesDirectory .
if ($LASTEXITCODE -ne 0) {
    throw "jar failed with exit code $LASTEXITCODE"
}

if ($RunSmokeTest) {
    & java -cp $jarPath org.umbra.testfixture.rti1516_2025.MockSmokeTest
    if ($LASTEXITCODE -ne 0) {
        throw "Mock Java RTI smoke test failed with exit code $LASTEXITCODE"
    }
}

Write-Output $jarPath
