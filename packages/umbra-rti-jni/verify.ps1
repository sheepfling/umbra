[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ArtifactDirectory,
    [Parameter(Mandatory = $true)]
    [string]$JavaApiJar,
    [string]$ExpectedApiSha256,
    [string]$ExpectedBridgeSha256,
    [string]$ExpectedNativeSha256,
    [string]$ManifestPath,
    [switch]$SkipSmokeTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$artifactRoot = (Resolve-Path -LiteralPath $ArtifactDirectory -ErrorAction Stop).Path
$apiPath = (Resolve-Path -LiteralPath $JavaApiJar -ErrorAction Stop).Path
$bridgePath = Join-Path $artifactRoot "umbra-rti-jni.jar"
if (-not (Test-Path -LiteralPath $bridgePath -PathType Leaf)) {
    throw "JNI bridge JAR does not exist: $bridgePath"
}

$nativeCandidates = @(
    "umbra_rti_jni.dll",
    "libumbra_rti_jni.so",
    "libumbra_rti_jni.dylib"
)
$nativePath = $null
foreach ($candidate in $nativeCandidates) {
    $path = Join-Path $artifactRoot $candidate
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $nativePath = (Resolve-Path -LiteralPath $path).Path
        break
    }
}
if ($null -eq $nativePath) {
    throw "JNI native library is missing below: $artifactRoot"
}

$apiHash = (Get-FileHash -LiteralPath $apiPath -Algorithm SHA256).Hash.ToUpperInvariant()
if (-not [string]::IsNullOrWhiteSpace($ExpectedApiSha256) -and
    $apiHash -ne $ExpectedApiSha256.Trim().ToUpperInvariant()) {
    throw "IEEE Java API SHA-256 mismatch: expected $ExpectedApiSha256, actual $apiHash"
}
$bridgeHash = (Get-FileHash -LiteralPath $bridgePath -Algorithm SHA256).Hash.ToUpperInvariant()
if (-not [string]::IsNullOrWhiteSpace($ExpectedBridgeSha256) -and
    $bridgeHash -ne $ExpectedBridgeSha256.Trim().ToUpperInvariant()) {
    throw "JNI bridge SHA-256 mismatch: expected $ExpectedBridgeSha256, actual $bridgeHash"
}
$nativeHash = (Get-FileHash -LiteralPath $nativePath -Algorithm SHA256).Hash.ToUpperInvariant()
if (-not [string]::IsNullOrWhiteSpace($ExpectedNativeSha256) -and
    $nativeHash -ne $ExpectedNativeSha256.Trim().ToUpperInvariant()) {
    throw "JNI native library SHA-256 mismatch: expected $ExpectedNativeSha256, actual $nativeHash"
}

function Get-JarEntries([string]$path) {
    $entries = @(& jar tf $path)
    if ($LASTEXITCODE -ne 0) {
        throw "jar could not inspect: $path"
    }
    return $entries
}

$requiredApiEntries = @(
    "hla/rti1516_2025/RtiFactoryFactory.class",
    "hla/rti1516_2025/RtiFactory.class",
    "hla/rti1516_2025/RTIambassador.class",
    "hla/rti1516_2025/FederateAmbassador.class"
)
$apiEntries = Get-JarEntries $apiPath
foreach ($entry in $requiredApiEntries) {
    if ($apiEntries -notcontains $entry) {
        throw "The supplied Java API JAR is missing standard entry: $entry"
    }
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$bridgeArchive = [System.IO.Compression.ZipFile]::OpenRead($bridgePath)
try {
    $bridgeEntries = @($bridgeArchive.Entries | Select-Object -ExpandProperty FullName)
    $shadowedApiEntries = @(
        $bridgeEntries | Where-Object { $_ -like "hla/rti1516_2025/*" }
    )
    if ($shadowedApiEntries.Count -ne 0) {
        throw (
            "JNI bridge must not bundle IEEE Java API classes: " +
            ($shadowedApiEntries -join ", ")
        )
    }

    $expectedDescriptors = @{
        "META-INF/services/hla.rti1516_2025.RtiFactory" =
            "org.umbra.jni.rti1516_2025.NativeRtiFactory"
        "META-INF/services/hla.rti1516_2025.auth.AuthorizerFactory" =
            "org.umbra.jni.rti1516_2025.NativeAuthorizerFactory"
    }
    foreach ($descriptor in $expectedDescriptors.Keys) {
        $entry = $bridgeArchive.GetEntry($descriptor)
        if ($null -eq $entry) {
            throw "JNI bridge is missing ServiceLoader descriptor: $descriptor"
        }
        $reader = [System.IO.StreamReader]::new($entry.Open())
        try {
            $providers = @(
                $reader.ReadToEnd().Trim() -split "`r?`n" |
                    Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
            )
        } finally {
            $reader.Dispose()
        }
        if ($providers.Count -ne 1 -or $providers[0] -ne $expectedDescriptors[$descriptor]) {
            throw (
                "JNI bridge ServiceLoader descriptor $descriptor must contain only " +
                "$($expectedDescriptors[$descriptor])"
            )
        }
    }
} finally {
    $bridgeArchive.Dispose()
}

if (-not $SkipSmokeTest) {
    $classpath = $apiPath + [System.IO.Path]::PathSeparator + $bridgePath
    $smokeArguments = @(
        "-Dumbra.rti.jni.library=$nativePath",
        "-cp", $classpath,
        "org.umbra.jni.rti1516_2025.NativeSmokeTest"
    )
    $repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
    $smokeFom = Join-Path $repositoryRoot "third_party\ieee1516.2-2025\resources\examples\RestaurantFOMmodule-2025.xml"
    if (Test-Path -LiteralPath $smokeFom -PathType Leaf) {
        $smokeArguments += $smokeFom
    }
    & java @smokeArguments
    if ($LASTEXITCODE -ne 0) {
        throw "JNI Java RTI smoke test failed with exit code $LASTEXITCODE"
    }
}

$manifest = [ordered]@{
    javaApiJar = $apiPath
    javaApiSha256 = $apiHash
    bridgeJar = (Resolve-Path -LiteralPath $bridgePath).Path
    bridgeJarSha256 = $bridgeHash
    nativeLibrary = $nativePath
    nativeLibrarySha256 = $nativeHash
}
if (-not [string]::IsNullOrWhiteSpace($ManifestPath)) {
    $manifest | ConvertTo-Json | Set-Content -LiteralPath $ManifestPath -Encoding UTF8
}
$manifest | ConvertTo-Json -Compress
