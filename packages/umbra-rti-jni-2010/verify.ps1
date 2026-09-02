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
$bridgePath = Join-Path $artifactRoot "umbra-rti-jni-2010.jar"
if (-not (Test-Path -LiteralPath $bridgePath -PathType Leaf)) {
    throw "2010 JNI bridge JAR does not exist: $bridgePath"
}

$nativeCandidates = @(
    "umbra_rti_jni_2010.dll",
    "libumbra_rti_jni_2010.so",
    "libumbra_rti_jni_2010.dylib"
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
    throw "2010 JNI native library is missing below: $artifactRoot"
}

function Assert-Hash([string]$Path, [string]$Expected, [string]$Description) {
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
    if (-not [string]::IsNullOrWhiteSpace($Expected) -and
        $actual -ne $Expected.Trim().ToUpperInvariant()) {
        throw "$Description SHA-256 mismatch: expected $Expected, actual $actual"
    }
    return $actual
}

$apiHash = Assert-Hash $apiPath $ExpectedApiSha256 "IEEE 1516e Java API"
$bridgeHash = Assert-Hash $bridgePath $ExpectedBridgeSha256 "2010 JNI bridge"
$nativeHash = Assert-Hash $nativePath $ExpectedNativeSha256 "2010 JNI native library"

function Get-JarEntries([string]$Path) {
    $entries = @(& jar tf $Path)
    if ($LASTEXITCODE -ne 0) { throw "jar could not inspect: $Path" }
    return $entries
}

$requiredApiEntries = @(
    "hla/rti1516e/RtiFactory.class",
    "hla/rti1516e/RtiFactoryFactory.class",
    "hla/rti1516e/RTIambassador.class",
    "hla/rti1516e/FederateAmbassador.class",
    "hla/rti1516e/LogicalTimeFactory.class",
    "hla/rti1516e/LogicalTimeFactoryFactory.class",
    "hla/rti1516e/time/HLAinteger64TimeFactory.class",
    "hla/rti1516e/time/HLAfloat64TimeFactory.class",
    "hla/rti1516e/encoding/EncoderFactory.class",
    "hla/rti1516e/exceptions/RTIinternalError.class"
)
$apiEntries = Get-JarEntries $apiPath
foreach ($entry in $requiredApiEntries) {
    if ($apiEntries -notcontains $entry) {
        throw "The supplied 2010 Java API JAR is missing standard entry: $entry"
    }
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$bridgeArchive = [System.IO.Compression.ZipFile]::OpenRead($bridgePath)
try {
    $bridgeEntries = @($bridgeArchive.Entries | Select-Object -ExpandProperty FullName)
    $shadowedApiEntries = @(
        $bridgeEntries | Where-Object { $_ -like "hla/rti1516e/*" }
    )
    if ($shadowedApiEntries.Count -ne 0) {
        throw "2010 JNI bridge must not bundle IEEE API classes: $($shadowedApiEntries -join ', ')"
    }

    $expectedDescriptors = @{
        "META-INF/services/hla.rti1516e.RtiFactory" = @(
            "org.umbra.jni.rti1516e.NativeRtiFactory"
        )
        "META-INF/services/hla.rti1516e.LogicalTimeFactory" = @(
            "org.umbra.jni.rti1516e.NativeInteger64TimeFactory",
            "org.umbra.jni.rti1516e.NativeFloat64TimeFactory"
        )
    }
    foreach ($descriptorName in $expectedDescriptors.Keys) {
        $descriptor = $bridgeArchive.GetEntry($descriptorName)
        if ($null -eq $descriptor) {
            throw "2010 JNI bridge is missing ServiceLoader descriptor: $descriptorName"
        }
        $reader = [System.IO.StreamReader]::new($descriptor.Open())
        try {
            $providers = @(
                $reader.ReadToEnd().Trim() -split "`r?`n" |
                    Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
            )
        } finally {
            $reader.Dispose()
        }
        $expectedProviders = @($expectedDescriptors[$descriptorName])
        if ($providers.Count -ne $expectedProviders.Count -or
            -not [System.Linq.Enumerable]::SequenceEqual(
                [string[]]$providers, [string[]]$expectedProviders)) {
            throw "unexpected 2010 JNI ServiceLoader providers for ${descriptorName}: $($providers -join ', ')"
        }
    }
    foreach ($providerClass in @(
        "org/umbra/jni/rti1516e/NativeRtiFactory.class",
        "org/umbra/jni/rti1516e/NativeInteger64TimeFactory.class",
        "org/umbra/jni/rti1516e/NativeFloat64TimeFactory.class")) {
        if ($bridgeEntries -notcontains $providerClass) {
            throw "2010 JNI bridge is missing registered provider class: $providerClass"
        }
    }
} finally {
    $bridgeArchive.Dispose()
}

if (-not $SkipSmokeTest) {
    $classpath = $apiPath + [System.IO.Path]::PathSeparator + $bridgePath
    $property = "-Dumbra.rti.jni.2010.library=$nativePath"
    foreach ($smokeClass in @(
        "org.umbra.jni.rti1516e.SurfaceSmokeTest",
        "org.umbra.jni.rti1516e.NativeSmokeTest",
        "org.umbra.jni.rti1516e.NativeTypeRoundTripTest")) {
        & java $property -cp $classpath $smokeClass
        if ($LASTEXITCODE -ne 0) {
            throw "2010 JNI smoke test failed for $smokeClass with exit code $LASTEXITCODE"
        }
    }
}

$manifest = [ordered]@{
    schemaVersion = 1
    edition = "IEEE 1516e-2010"
    capabilityProfile = "bounded-null-provider"
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
