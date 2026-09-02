[CmdletBinding()]
param(
    [ValidateSet("surface", "native")]
    [string]$Mode = "surface",
    [string]$JavaApiJar,
    [string]$FomPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$productRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$providerJar = Join-Path $productRoot "umbra-rti-jni.jar"
if (-not (Test-Path -LiteralPath $providerJar -PathType Leaf)) {
    throw "Umbra 2025 provider JAR is missing: $providerJar"
}

if ([string]::IsNullOrWhiteSpace($JavaApiJar)) {
    $apiCandidates = @(Get-ChildItem -LiteralPath $productRoot -Filter "*.jar" -File |
        Where-Object { $_.Name -ne "umbra-rti-jni.jar" })
    if ($apiCandidates.Count -ne 1) {
        throw "Pass -JavaApiJar when the product directory does not contain exactly one API JAR"
    }
    $JavaApiJar = $apiCandidates[0].FullName
}
$apiPath = (Resolve-Path -LiteralPath $JavaApiJar -ErrorAction Stop).Path

$nativeCandidates = @(
    "umbra_rti_jni.dll",
    "libumbra_rti_jni.so",
    "libumbra_rti_jni.dylib"
)
$nativePath = $null
foreach ($candidate in $nativeCandidates) {
    $path = Join-Path $productRoot $candidate
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $nativePath = (Resolve-Path -LiteralPath $path).Path
        break
    }
}
if ($null -eq $nativePath) { throw "Umbra 2025 native library is missing from $productRoot" }

$mainClass = if ($Mode -eq "surface") {
    "org.umbra.jni.rti1516_2025.StandardSurfaceSmokeTest"
} else {
    "org.umbra.jni.rti1516_2025.NativeSmokeTest"
}
$arguments = @(
    "-Dumbra.rti.jni.library=$nativePath",
    "-cp", ($apiPath + [System.IO.Path]::PathSeparator + $providerJar),
    $mainClass
)
if ($Mode -eq "native" -and -not [string]::IsNullOrWhiteSpace($FomPath)) {
    $arguments += (Resolve-Path -LiteralPath $FomPath -ErrorAction Stop).Path
}
& java @arguments
if ($LASTEXITCODE -ne 0) { throw "Umbra 2025 $Mode run failed with exit code $LASTEXITCODE" }
