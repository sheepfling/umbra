[CmdletBinding()]
param(
    [ValidateSet("surface", "native", "types")]
    [string]$Mode = "surface",
    [string]$JavaApiJar
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$productRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$providerJar = Join-Path $productRoot "umbra-rti-jni-2010.jar"
if (-not (Test-Path -LiteralPath $providerJar -PathType Leaf)) {
    throw "Umbra 2010 provider JAR is missing: $providerJar"
}

if ([string]::IsNullOrWhiteSpace($JavaApiJar)) {
    $apiCandidates = @(Get-ChildItem -LiteralPath $productRoot -Filter "*.jar" -File |
        Where-Object { $_.Name -ne "umbra-rti-jni-2010.jar" })
    if ($apiCandidates.Count -ne 1) {
        throw "Pass -JavaApiJar when the product directory does not contain exactly one API JAR"
    }
    $JavaApiJar = $apiCandidates[0].FullName
}
$apiPath = (Resolve-Path -LiteralPath $JavaApiJar -ErrorAction Stop).Path

$nativeCandidates = @(
    "umbra_rti_jni_2010.dll",
    "libumbra_rti_jni_2010.so",
    "libumbra_rti_jni_2010.dylib"
)
$nativePath = $null
foreach ($candidate in $nativeCandidates) {
    $path = Join-Path $productRoot $candidate
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $nativePath = (Resolve-Path -LiteralPath $path).Path
        break
    }
}
if ($null -eq $nativePath) { throw "Umbra 2010 native library is missing from $productRoot" }

$mainClass = switch ($Mode) {
    "surface" { "org.umbra.jni.rti1516e.SurfaceSmokeTest"; break }
    "native" { "org.umbra.jni.rti1516e.NativeSmokeTest"; break }
    "types" { "org.umbra.jni.rti1516e.NativeTypeRoundTripTest"; break }
}
$arguments = @(
    "-Dumbra.rti.jni.2010.library=$nativePath",
    "-cp", ($apiPath + [System.IO.Path]::PathSeparator + $providerJar),
    $mainClass
)
& java @arguments
if ($LASTEXITCODE -ne 0) { throw "Umbra 2010 $Mode run failed with exit code $LASTEXITCODE" }
