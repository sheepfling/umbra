[CmdletBinding()]
param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot "build"),
    [string]$JavaApiJar,
    [switch]$RunSmokeTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$outputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
$mockBuildDirectory = Join-Path $outputRoot "mock-java-api"
$nativeBuildDirectory = Join-Path $outputRoot "native"
$classesDirectory = Join-Path $outputRoot "classes"
$jarPath = Join-Path $outputRoot "umbra-rti-jni.jar"
$nativeLibraryPath = Join-Path $outputRoot "umbra_rti_jni.dll"
$mockJarPath = Join-Path $mockBuildDirectory "umbra-mock-java-rti.jar"
$sourceDirectory = Join-Path $PSScriptRoot "src\main\java"
$resourceDirectory = Join-Path $PSScriptRoot "src\main\resources"

# javac does not remove classes for deleted or renamed sources.  Always start
# the generated Java output cleanly so a reused output directory cannot carry
# a stale façade class (or an accidentally bundled API class) into a release
# artifact.  This target is a fixed child of the caller-selected output root.
if (Test-Path -LiteralPath $classesDirectory) {
    Remove-Item -LiteralPath $classesDirectory -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $outputRoot, $classesDirectory | Out-Null

# The checked-in fixture supplies only the standard-shaped Java declarations
# used by the repository integration lane. Release/consumer builds supply the
# independently obtained IEEE API JAR explicitly; the bridge never bundles or
# shadows that API surface.
if ([string]::IsNullOrWhiteSpace($JavaApiJar)) {
    & (Join-Path $repositoryRoot "packages\umbra-rti-jpype\test-fixtures\mock-java-rti\build.ps1") `
        -OutputDirectory $mockBuildDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "mock Java API build failed with exit code $LASTEXITCODE"
    }
    $apiJarPath = $mockJarPath
} else {
    $apiJarPath = [System.IO.Path]::GetFullPath($JavaApiJar)
    if (-not (Test-Path -LiteralPath $apiJarPath -PathType Leaf)) {
        throw "Java API JAR does not exist: $apiJarPath"
    }
}

$cmakeArguments = @(
    "-S", $PSScriptRoot,
    "-B", $nativeBuildDirectory,
    "-DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON",
    "-DUMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=ON",
    "-DUMBRA_FETCH_LIBXML2=ON"
)
$cachedLibxmlSource = Join-Path $repositoryRoot "packages\umbra-rti-native\build\cp312-cp312-win_amd64\_deps\libxml2-src"
if (Test-Path -LiteralPath $cachedLibxmlSource -PathType Container) {
    $cmakeArguments += "-DFETCHCONTENT_SOURCE_DIR_LIBXML2=$cachedLibxmlSource"
}
& cmake @cmakeArguments
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE"
}
& cmake --build $nativeBuildDirectory --config Release --target umbra_rti_jni
if ($LASTEXITCODE -ne 0) {
    throw "JNI native build failed with exit code $LASTEXITCODE"
}

$builtLibrary = Get-ChildItem -Path $nativeBuildDirectory -Recurse -File -Filter "umbra_rti_jni.dll" |
    Select-Object -First 1
if ($null -eq $builtLibrary) {
    throw "CMake did not produce umbra_rti_jni.dll below $nativeBuildDirectory"
}
Copy-Item -LiteralPath $builtLibrary.FullName -Destination $nativeLibraryPath -Force

$sources = Get-ChildItem -Path $sourceDirectory -Filter "*.java" -File -Recurse |
    Select-Object -ExpandProperty FullName
if ($sources.Count -eq 0) {
    throw "No JNI Java sources found below $sourceDirectory"
}
& javac -cp $apiJarPath -d $classesDirectory @sources
if ($LASTEXITCODE -ne 0) {
    throw "javac failed with exit code $LASTEXITCODE"
}
Copy-Item -Path (Join-Path $resourceDirectory "*") -Destination $classesDirectory -Recurse -Force
& jar --create --file $jarPath -C $classesDirectory .
if ($LASTEXITCODE -ne 0) {
    throw "jar failed with exit code $LASTEXITCODE"
}

# Keep the independently supplied IEEE API authoritative for every build,
# not only for the Python integration test.  A stale generated class or an
# accidentally copied ServiceLoader descriptor would otherwise turn this
# bridge into a second Java-side API/provider surface.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$bridgeArchive = [System.IO.Compression.ZipFile]::OpenRead($jarPath)
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

    $expectedServiceDescriptors = @{
        "META-INF/services/hla.rti1516_2025.RtiFactory" =
            "org.umbra.jni.rti1516_2025.NativeRtiFactory"
        "META-INF/services/hla.rti1516_2025.auth.AuthorizerFactory" =
            "org.umbra.jni.rti1516_2025.NativeAuthorizerFactory"
    }
    foreach ($descriptor in $expectedServiceDescriptors.Keys) {
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
        if ($providers.Count -ne 1 -or $providers[0] -ne $expectedServiceDescriptors[$descriptor]) {
            throw (
                "JNI bridge ServiceLoader descriptor $descriptor must contain only " +
                "$($expectedServiceDescriptors[$descriptor])"
            )
        }
    }
} finally {
    $bridgeArchive.Dispose()
}

if ($RunSmokeTest) {
    $classpath = "$apiJarPath$([System.IO.Path]::PathSeparator)$jarPath"
    & java "-Dumbra.rti.jni.library=$nativeLibraryPath" -cp $classpath `
        org.umbra.jni.rti1516_2025.NativeSmokeTest
    if ($LASTEXITCODE -ne 0) {
        throw "JNI Java RTI smoke test failed with exit code $LASTEXITCODE"
    }
}

Write-Output $jarPath
Write-Output $nativeLibraryPath
Write-Output $apiJarPath
