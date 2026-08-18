# umbra-rti-jpype

Optional adapter that makes an installed IEEE 1516.1-2025 Java RTI available
through the same pure-Python `hla.rti1516_2025` contract as Umbra's pybind11
provider.

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

For a supported Java RTI, use a separate small adapter package rather than
putting its JAR in this transport package. The repository's
`../umbra-rti-java-mock` package is the working reference for named provider
registration, explicit JAR resolution, and fixed Java factory selection.

The adapter currently covers the same connected/unjoined foundation as the
native provider. `getEncoderFactory()` deliberately remains unavailable from
the public Python contract until the shared Python encoding-value layer exists.
Advanced migration code can explicitly use `unwrap_java_object()` or
`unwrap_java_encoder_factory()`; those methods are provider-specific and are
not portable to the native provider.
