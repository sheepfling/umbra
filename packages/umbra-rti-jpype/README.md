# umbra-rti-jpype

## Start here

Use this provider when Python should consume an IEEE 1516.1-2025 Java RTI.
Install the API package first, then the optional JPype bridge:

    python -m pip install -e packages/umbra-rti-api
    python -m pip install -e packages/umbra-rti-jpype[jpype]

Use the [package map](../README.md) to choose between this Java transport and
the direct native provider. The test-fixture and JNI sections below are for
adapter development, not normal application setup.

## Scope and Java configuration

Optional adapter that makes an installed IEEE 1516.1-2025 Java RTI available
through the same pure-Python `hla.rti1516_2025` contract as Umbra's pybind11
provider.

This is a thin adapter over the Java RTI surface: service calls and callback
proxies target the standard `hla.rti1516_2025.RtiFactory` and
`RTIambassador` interfaces directly. The optional external-API integration
lane starts the same adapter against an independently obtained IEEE
1516.1-2025 Java API JAR, checks that the underlying ambassador is assignable
to the exact standard interface, and runs the shared provider-parity tests.
It does not introduce a Python-specific Java facade.

It does not bundle a Java RTI or start a JVM merely by being imported. Install
the optional bridge and configure the Java classpath before creating an
ambassador:

```powershell
python -m pip install 'umbra-rti-jpype[jpype]'
$env:UMBRA_JAVA_RTI_CLASSPATH = 'C:\vendor\rti.jar;C:\vendor\dependencies\*'
```

For a Java RTI with more than one service provider, create the adapter with an
explicit configuration:

```python
from hla.rti1516_2025 import CallbackModel, FederateAmbassador
from umbra._java.rti1516_2025 import JavaProviderConfiguration, JavaRtiFactory

factory = JavaRtiFactory(
    JavaProviderConfiguration(
        classpath=(r"C:\vendor\rti.jar",),
        rti_factory_name="Vendor RTI",
    )
)
ambassador = factory.getRtiAmbassador()
ambassador.connect(FederateAmbassador(), CallbackModel.HLA_EVOKED)
```

For the usual one-JAR case, the same standard `RtiFactoryFactory` path is
available as a single call:

```python
from umbra._java.rti1516_2025 import JavaRtiFactory

factory = JavaRtiFactory.from_jar(
    r"C:\vendor\pitch-rti.jar",
    factory_name="Pitch RTI",
    dependencies=(r"C:\vendor\pitch-support.jar",),
    native_library_path=r"C:\vendor\bin",
)
ambassador = factory.getRtiAmbassador()
```

This helper only assembles JVM configuration; discovery remains the exact
standard Java `hla.rti1516_2025.RtiFactoryFactory`/`ServiceLoader` flow. The
IEEE API JAR must still be present on the classpath when the vendor JAR does
not bundle it.

To validate a JAR before connecting, use the metadata-only probe:

```python
probe = JavaRtiFactory.probe_jar("vendor-rti.jar", factory_name="Pitch RTI")
print(probe.rti_name, probe.rti_version)
ambassador = probe.factory.getRtiAmbassador()
```

The probe invokes only the standard `RtiFactoryFactory` and the required
`RtiFactory.rtiName()`/`rtiVersion()` methods. It does not connect or create
federation state.

Package discovery can select the Java *transport* without starting every
installed provider. Keep the Java vendor's factory name separate from the
Python transport alias:

```powershell
$env:HLA_RTI_FACTORY_NAME = 'java'
$env:UMBRA_JAVA_RTI_FACTORY_NAME = 'Vendor RTI'
```

`RtiFactoryFactory.getRtiFactory()` then returns a `JavaRtiFactory`; its
`rtiName()` returns the Java vendor's actual standard name, not the `java`
transport alias.

## Test fixture

The repository includes a compileable Java mock surface at
`test-fixtures/mock-java-rti`. It uses Java `ServiceLoader`, supports the
current connection and federation-execution discovery slice including all four
`connect` overloads, and can deliver queued `connectionLost` and
`reportFederationExecutions` callbacks. It is intentionally not an HLA implementation. Build its Java-only
smoke test with:

```powershell
.\test-fixtures\mock-java-rti\build.ps1 -RunSmokeTest
```

The regular Python test suite compiles this fixture in a temporary directory.
When the optional JPype dependency is installed, it additionally starts a JVM
with the fixture JAR and verifies the complete Java-to-Python adapter path.

For a supported vendor Java RTI, use a separate small adapter package rather
than putting its JAR in this transport package. The repository's
`../umbra-rti-java-mock` package is the working reference for named provider
registration, explicit JAR resolution, and fixed Java factory selection.

## Umbra JNI route

`UmbraJniRtiFactory` is included here because it is only a configuration preset
for Umbra's own bridge; it adds no RTI services or Java shadow state. It
requires an independently supplied IEEE API JAR, Umbra bridge JAR, and native
library, then selects the exact `Umbra JNI C++ RTI` factory through standard
Java `ServiceLoader` discovery.

```python
from umbra._java.rti1516_2025 import UmbraJniRtiFactory

factory = UmbraJniRtiFactory(
    api_jar=r"C:\\vendor\\ieee-1516.1-2025-java-api.jar",
    artifact_directory=r"C:\\vendor\\umbra-rti-jni-build",
)
```

The registered `umbra-jni` entry point constructs this same factory. Its
`UMBRA_JNI_*` environment variables and release-directory discovery avoid
mixing JNI setup into ordinary vendor-JAR configuration.

The adapter exposes the completed shared contract, including provider-owned
encoder, logical-time, handle, callback, federation-management, and DDM value
conversions covered by the conformance suite. `unwrap_java_object()` and
`unwrap_java_encoder_factory()` remain explicit provider-specific escape
hatches for migration code; normal application code and parity tests consume
the shared Python values and do not depend on Java proxies.
