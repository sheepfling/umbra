[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$JavaApiJar,
    [string]$OutputDirectory = (Join-Path $PSScriptRoot "build"),
    [switch]$RunSmokeTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$apiPath = (Resolve-Path -LiteralPath $JavaApiJar -ErrorAction Stop).Path
$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
$nativeBuildDirectory = Join-Path $outputRoot "native"
$classesDirectory = Join-Path $outputRoot "classes"
$jarPath = Join-Path $outputRoot "umbra-rti-jni-2010.jar"
$nativeLibraryPath = Join-Path $outputRoot "umbra_rti_jni_2010.dll"

if (Test-Path -LiteralPath $classesDirectory) {
    Remove-Item -LiteralPath $classesDirectory -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $outputRoot, $classesDirectory | Out-Null

$cmakeArguments = @(
    "-S", $PSScriptRoot,
    "-B", $nativeBuildDirectory
)
& cmake @cmakeArguments
if ($LASTEXITCODE -ne 0) {
    throw "2010 JNI CMake configuration failed with exit code $LASTEXITCODE"
}
& cmake --build $nativeBuildDirectory --config Release --target umbra_rti_jni_2010
if ($LASTEXITCODE -ne 0) {
    throw "2010 JNI native build failed with exit code $LASTEXITCODE"
}

$nativeCandidates = @(
    "umbra_rti_jni_2010.dll",
    "libumbra_rti_jni_2010.so",
    "libumbra_rti_jni_2010.dylib"
)
$builtLibrary = $null
foreach ($candidate in $nativeCandidates) {
    $builtLibrary = Get-ChildItem -Path $nativeBuildDirectory -Recurse -File -Filter $candidate |
        Select-Object -First 1
    if ($null -ne $builtLibrary) {
        break
    }
}
if ($null -eq $builtLibrary) {
    throw "CMake did not produce the 2010 JNI native library"
}
Copy-Item -LiteralPath $builtLibrary.FullName -Destination $nativeLibraryPath -Force

$sources = @(Get-ChildItem -Path (Join-Path $PSScriptRoot "src\main\java") -Filter "*.java" -File -Recurse |
    Select-Object -ExpandProperty FullName)
if ($sources.Count -eq 0) {
    throw "No 2010 JNI Java sources found"
}
& javac -encoding UTF-8 -source 11 -target 11 -cp $apiPath -d $classesDirectory @sources
if ($LASTEXITCODE -ne 0) {
    throw "2010 JNI Java compile failed with exit code $LASTEXITCODE"
}
Copy-Item -Path (Join-Path $PSScriptRoot "src\main\resources\*") `
    -Destination $classesDirectory -Recurse -Force
& jar --create --file $jarPath -C $classesDirectory .
if ($LASTEXITCODE -ne 0) {
    throw "2010 JNI bridge JAR creation failed with exit code $LASTEXITCODE"
}

$apiEntries = @(& jar tf $apiPath)
if ($LASTEXITCODE -ne 0) {
    throw "jar could not inspect the supplied 2010 API JAR"
}
foreach ($entry in @(
        "hla/rti1516e/RtiFactory.class",
        "hla/rti1516e/RtiFactoryFactory.class",
        "hla/rti1516e/RTIambassador.class",
        "hla/rti1516e/FederateAmbassador.class",
        "hla/rti1516e/LogicalTimeFactory.class",
        "hla/rti1516e/LogicalTimeFactoryFactory.class",
        "hla/rti1516e/time/HLAinteger64TimeFactory.class",
        "hla/rti1516e/time/HLAfloat64TimeFactory.class",
        "hla/rti1516e/encoding/EncoderFactory.class",
        "hla/rti1516e/exceptions/RTIinternalError.class")) {
    if ($apiEntries -notcontains $entry) {
        throw "The supplied API JAR is missing standard entry: $entry"
    }
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$bridgeArchive = [System.IO.Compression.ZipFile]::OpenRead($jarPath)
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
} finally {
    $bridgeArchive.Dispose()
}

if ($RunSmokeTest) {
    $classpath = $apiPath + [System.IO.Path]::PathSeparator + $jarPath
    & java "-Dumbra.rti.jni.2010.library=$nativeLibraryPath" `
        -cp $classpath `
        org.umbra.jni.rti1516e.SurfaceSmokeTest
    if ($LASTEXITCODE -ne 0) {
        throw "2010 JNI surface smoke test failed with exit code $LASTEXITCODE"
    }
    & java "-Dumbra.rti.jni.2010.library=$nativeLibraryPath" `
        -cp $classpath `
        org.umbra.jni.rti1516e.NativeSmokeTest
    if ($LASTEXITCODE -ne 0) {
        throw "2010 JNI Java smoke test failed with exit code $LASTEXITCODE"
    }
    & java "-Dumbra.rti.jni.2010.library=$nativeLibraryPath" `
        -cp $classpath `
        org.umbra.jni.rti1516e.NativeTypeRoundTripTest
    if ($LASTEXITCODE -ne 0) {
        throw "2010 JNI type round-trip test failed with exit code $LASTEXITCODE"
    }
}

Write-Output $jarPath
Write-Output $nativeLibraryPath
Write-Output $apiPath
