[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,
    [Parameter(Mandatory = $true)]
    [string]$JavaApiJar,
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,
    [string]$JavaApiCoordinate = "se.pitch.oss.fedpro:hla-4-api",
    [string]$JavaApiVersion = "2.1.0",
    [string]$JavaApiSource,
    [string]$ExpectedApiSha256,
    [string]$ExpectedBridgeSha256,
    [string]$ExpectedNativeSha256,
    [switch]$IncludeJavaApiJar,
    [switch]$SkipSmokeTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RequiredFile([string]$Path, [string]$Description) {
    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    if (-not (Test-Path -LiteralPath $resolved.Path -PathType Leaf)) {
        throw "$Description is not a file: $($resolved.Path)"
    }
    return $resolved.Path
}

$buildRoot = (Resolve-Path -LiteralPath $BuildDirectory -ErrorAction Stop).Path
$apiPath = Resolve-RequiredFile $JavaApiJar "Java API JAR"
$bridgeSource = Resolve-RequiredFile (Join-Path $buildRoot "umbra-rti-jni.jar") "JNI bridge JAR"

$nativeCandidates = @(
    "umbra_rti_jni.dll",
    "libumbra_rti_jni.so",
    "libumbra_rti_jni.dylib"
)
$nativeSource = $null
foreach ($candidate in $nativeCandidates) {
    $candidatePath = Join-Path $buildRoot $candidate
    if (Test-Path -LiteralPath $candidatePath -PathType Leaf) {
        $nativeSource = Resolve-RequiredFile $candidatePath "JNI native library"
        break
    }
}
if ($null -eq $nativeSource) {
    throw "JNI native library is missing below build directory: $buildRoot"
}

# The output directory is an explicit staging target.  Only the known release
# files are overwritten; unrelated files supplied by a caller are preserved.
$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
$bridgeTarget = Join-Path $outputRoot "umbra-rti-jni.jar"
$nativeTarget = Join-Path $outputRoot ([System.IO.Path]::GetFileName($nativeSource))
Copy-Item -LiteralPath $bridgeSource -Destination $bridgeTarget -Force
Copy-Item -LiteralPath $nativeSource -Destination $nativeTarget -Force
$runSource = Join-Path $PSScriptRoot "run.ps1"
if (Test-Path -LiteralPath $runSource -PathType Leaf) {
    Copy-Item -LiteralPath $runSource -Destination (Join-Path $outputRoot "run.ps1") -Force
}

$apiTarget = $apiPath
if ($IncludeJavaApiJar) {
    $apiTarget = Join-Path $outputRoot ([System.IO.Path]::GetFileName($apiPath))
    Copy-Item -LiteralPath $apiPath -Destination $apiTarget -Force
}

$manifestPath = Join-Path $outputRoot "umbra-rti-jni-manifest.json"
$verifyParams = @{
    ArtifactDirectory = $outputRoot
    JavaApiJar = $apiTarget
    ManifestPath = $manifestPath
    SkipSmokeTest = $SkipSmokeTest
}
if (-not [string]::IsNullOrWhiteSpace($ExpectedApiSha256)) {
    $verifyParams.ExpectedApiSha256 = $ExpectedApiSha256
}
if (-not [string]::IsNullOrWhiteSpace($ExpectedBridgeSha256)) {
    $verifyParams.ExpectedBridgeSha256 = $ExpectedBridgeSha256
}
if (-not [string]::IsNullOrWhiteSpace($ExpectedNativeSha256)) {
    $verifyParams.ExpectedNativeSha256 = $ExpectedNativeSha256
}
& (Join-Path $PSScriptRoot "verify.ps1") @verifyParams | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "JNI artifact verification failed with exit code $LASTEXITCODE"
}

$apiHash = (Get-FileHash -LiteralPath $apiTarget -Algorithm SHA256).Hash.ToUpperInvariant()
$bridgeHash = (Get-FileHash -LiteralPath $bridgeTarget -Algorithm SHA256).Hash.ToUpperInvariant()
$nativeHash = (Get-FileHash -LiteralPath $nativeTarget -Algorithm SHA256).Hash.ToUpperInvariant()
$dependencyManifest = [ordered]@{
    schemaVersion = 1
    edition = "IEEE 1516.1-2025"
    capabilityProfile = "bounded-jni-bridge"
    launcher = "run.ps1"
    bridge = [ordered]@{
        file = [System.IO.Path]::GetFileName($bridgeTarget)
        sha256 = $bridgeHash
    }
    native = [ordered]@{
        file = [System.IO.Path]::GetFileName($nativeTarget)
        sha256 = $nativeHash
    }
    javaApi = [ordered]@{
        coordinate = $JavaApiCoordinate
        version = $JavaApiVersion
        file = [System.IO.Path]::GetFileName($apiTarget)
        sha256 = $apiHash
        bundled = [bool]$IncludeJavaApiJar
    }
    notes = @(
        "The IEEE Java API remains the authoritative standard surface.",
        "The product directory includes the declared Java API dependency and a standard ServiceLoader launcher.",
        "The bridge exposes bounded RTI behavior; this bundle is not a full RTI conformance claim.",
        "Redistribution rights and the API source URL must be recorded by the release owner."
    )
}
if (-not [string]::IsNullOrWhiteSpace($JavaApiSource)) {
    $dependencyManifest.javaApi.source = $JavaApiSource
}
$dependencyPath = Join-Path $outputRoot "umbra-rti-jni-dependencies.json"
$dependencyManifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $dependencyPath -Encoding UTF8

Write-Output $outputRoot
Write-Output $manifestPath
Write-Output $dependencyPath
