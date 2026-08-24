# Umbra Java RTI Contract TCK

## Start here

Use this Java artifact to test an IEEE 1516.1-2025 Java provider through the
standard Java interfaces. It is provider-neutral: it does not need Umbra JNI,
and it is not a Python package. The TCK needs a Java 11+ toolchain, an
authoritative API JAR, a provider JAR, and a suitable FOM module.

Use the [Java TCK guide](../../docs/testing/JAVA-RTI-CONFORMANCE-TCK.md) for
the evidence boundary and provider-matrix workflow.

## Scope

This package contains provider-neutral Java tests for the IEEE 1516.1-2025
Java RTI surface.  It intentionally imports only `hla.rti1516_2025.*`; it does
not import Umbra JNI classes and does not assume that the provider is backed by
C++.

The same test executable can run against Umbra's JNI provider or a completely
different Java RTI.  The provider is selected through the standard Java
`RtiFactoryFactory` and `ServiceLoader` path.

## Compile

PowerShell:

```powershell
.\build.ps1 `
  -ApiJar C:\path\to\hla-4-api-2.1.0.jar `
  -OutputDirectory ..\..\out\java-tck\classes
```

## Run

The provider JARs must be on the runtime class path.  A FOM module is required
for the lifecycle and time tests.

```powershell
$api = 'C:\path\to\hla-4-api-2.1.0.jar'
$provider = 'C:\path\to\vendor-rti.jar'
$fom = 'C:\path\to\minimal-fom.xml'

.\run.ps1 `
  -ApiJar $api `
  -ProviderJar $provider `
  -FactoryName "Vendor RTI" `
  -FomPath $fom `
  -MimPath C:\path\to\HLAstandardMIM-2025.xml `
  -ClassesDirectory ..\..\out\java-tck\classes
```

Pass `-TimeImplementation HLAfloat64Time` when the provider should create the
federation with its floating logical-time implementation instead of the
default `HLAinteger64Time`.

For Umbra's JNI provider, add `umbra-rti-jni.jar` to the class path and pass
the native library through its documented system property:

```powershell
.\run.ps1 `
  -ApiJar $api `
  -ProviderJar 'C:\path\to\umbra-rti-jni.jar' `
  -NativeLibrary 'C:\path\to\umbra_rti_jni.dll' `
  -FactoryName 'Umbra JNI C++ RTI' `
  -FomPath $fom `
  -ClassesDirectory ..\..\out\java-tck\classes
```

The current executable slice reports factory discovery, standard encoder round
trips and malformed octets, federation membership/callback delivery,
logical-time factory behavior, declaration management, object
registration/discovery/attribute reflection, API-surface inventory, overloads
and standard exceptions, malformed FOM input, DDM region lifecycle,
save/restore, ownership, synchronization, MOM, and a two-federate
time-regulation/time-constrained advance with grant callbacks. The checked-in
Umbra JNI profile now passes all fifteen scenarios; other providers may report
`unsupported` or `not applicable` until a profile enables them. The
declaration/object scenarios use
the standard example class `HLAobjectRoot.Employee.Server` and its `Efficiency`
attribute, so pass the IEEE Restaurant FOM module (or a provider-compatible FOM
with those standard names) as `-FomPath`.

The object-management scenario deliberately checks opaque attribute octets and
callback ordering rather than decoding a provider-specific data representation.
That keeps the Java TCK transplantable to another RTI while still exercising
the complete publish/subscribe and register/discover/update/reflect boundary.
`run-matrix.ps1` runs the unchanged compiled classes against two or more
provider configurations. `compliance/catalogs/java-tck-scenario-catalog.json` and
`tools/java_tck.py` join scenario/API/Requirements Lab records and export
provider evidence. `run-jpype.ps1` gates a JPype handoff on a passing direct
Java result artifact before discovering the same provider through JPype.
