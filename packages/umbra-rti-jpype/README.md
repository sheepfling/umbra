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
The opt-in JNI encoder lane also starts at the Python `EncoderFactory` and
checks composite factory/varargs arguments, indexed fixed-array values,
variable-array children, fixed-record additions, and variant values after the
round-trip through the standard Java carrier and C++ implementation.

## IEEE 1516.1-2010 / 1516e route

The same package can consume a standards-shaped 2010 Java RTI JAR through the
exact `hla.rti1516e.RtiFactoryFactory`/`RtiFactory` surface:

```python
from umbra._java.rti1516e import Java2010RtiFactory

factory = Java2010RtiFactory.from_jar(
    r"C:\vendor\ieee-1516.1-2010-java-api.jar",
    dependencies=(r"C:\vendor\vendor-rti.jar",),
    factory_name="Vendor RTI",
)
probe = factory.probe()
print(probe.rti_name, probe.rti_version)
ambassador = factory.getRtiAmbassador()
```

This route starts the JVM lazily and uses the 2010 Java package—not the 2025
package or a bespoke Java facade. The generated parameter inventory selects
the standard overload by both arity and Python argument shape (including the
same-arity `URL`/`URL[]`/`String` alternatives), and drives enum, URL, set/map,
and known callback carrier conversion at the adapter boundary; JPype performs
the final Java dispatch. Provider-owned 2010 encoder objects and logical-time factories now
cross through typed Python façades, including provider-owned time arithmetic.
Attribute/region pair-list arguments and supplemental reflect/receive/remove
callback records also have typed boundary conversions; the remaining work is
an actual vendor-JAR run and provider-specific callback/complex-encoder
behavior. The transplantable fixture run is cataloged separately under
`compliance/catalogs/python-2010-tck-scenario-catalog.json` and exercises a
real JVM/proxy callback, provider-owned primitive encoding, integer-time
arithmetic, and the bounded standard-MIM `HLAsetTiming` time/lookahead
projection in both callback models. The timing slice is deliberately bounded;
the remaining periodic MOM matrix and a real vendor-JAR run are still external
gates.

The 2010 JNI carrier test consumes the same 79-case basic data-element value
matrix as the direct native test (`umbra_rti_test_support.data_element_matrix`).
It constructs every value through the provider's standard Java
`EncoderFactory`, validates the returned bytes through the C++ JNI probe, and
checks Python `ByteWrapper` cursor advancement. The matrix is deliberately
value-only so a transplanted vendor-JAR test does not inherit a Python-side
codec. A companion composite-creator check forwards fixed-array factory and
varargs forms, variable-array factories and varargs, fixed-record additions,
and variant discriminant/value pairs through the standard Java carrier so
JPype argument selection is tested without inventing Python wire semantics.
The adapter's focused boundary suite also invokes all 172 generated RTI
overloads and all 60 generated callback overloads against a dynamic Java-shaped
surface, so a missing forwarder is detected independently of provider behavior.
The shared `save_restore_matrix.py` adds 720 lifecycle cases to both edition
provider suites, covering every federate save/restore completion, failure, and
abort pair across callback, time, overload, all five advance-service forms, and member
dimensions. These cases assert forwarding names and argument shapes only; they
do not turn a binding test into a federation persistence claim.
The opt-in 2025 JVM matrix additionally exercises those lifecycle callbacks
through a real Java proxy across both callback models, both reference time
families, scalar/timestamped save requests, one-to-three members, and every
completion/failure/abort outcome. Enable it with
`UMBRA_ENABLE_JPYPE_STATE_SPACE_MATRIX=1`; it remains outside the default gate
because it requires a local JVM.
`unwrap_java_object()` and `unwrap_java_encoder_factory()` remain explicit
escape hatches for migration code.
No 2010 provider is bundled by this package.

The opt-in `test_jpype_2010_mock_integration.py` lane builds the repository's
small `mock-java-rti-2010` provider, discovers its named factory through the
standard `ServiceLoader` path, and runs a provider-neutral object/interaction
matrix for both callback models, both standard integer64/float64 time
factories, and two- or three-member federations. It uses the real 2010 Java
callback carriers (including fractional float64 timestamps) and the Python
adapter's `Path` → `java.net.URL` conversion; the fixture intentionally
supplies only enough stateful behavior to expose boundary and ordering
mistakes. Enable it with `UMBRA_ENABLE_JPYPE_2010_MOCK_INTEGRATION=1`. It is
evidence for the adapter surface, not a claim that the fixture or Umbra
implements every 2010 RTI semantic or vendor-specific state space.

The same opt-in 2010 lane also drives complete, not-complete, and abort
save/restore callbacks for both standard time factories and one- and two-member
federations. Its fixture delivers the timed callback immediately rather than
implementing a scheduled persistence boundary; the shared adapter matrix
remains the broader evidence for timestamped overload forwarding and all
save/restore state combinations. Failed and aborted saves additionally verify
the standard restore-request failure callback and label propagation.

Run that lane explicitly after building or supplying the 2010 API archive:

```powershell
$env:UMBRA_ENABLE_JPYPE_2010_MOCK_INTEGRATION = '1'
$env:UMBRA_JAVA_1516E_API_JAR = 'C:\vendor\ieee-1516.1-2010-java-api.jar'
python -m pytest packages/umbra-rti-jpype/tests/test_jpype_2010_mock_integration.py
```

The 2010 JNI carrier lane is a separate opt-in check. After building
`packages/umbra-rti-jni-2010` against the caller-supplied API JAR, point
`UMBRA_JNI_2010_BRIDGE_ARTIFACT_DIRECTORY` at its output and run
`test_jpype_2010_jni_type_roundtrip.py`. It uses the normal `JPype2010Runtime`
byte, handle, `ByteWrapper`, and standard-interface conversions while the
Java probe validates the encoded carriers in the official C++ implementation.
The carrier test includes all 24 standard data-element kinds through both
Java `DataElement.decode` overloads, with the `ByteWrapper` path checking
offset/cursor and trailing-byte semantics. It also runs a 28-case
malformed-wire matrix (truncation plus declared-length overruns), asserting
the standard Java `DecoderException` identity at the Python boundary. RTI
service behavior remains intentionally outside this transport test. The
Python lane also reflects `hla.rti1516e.encoding.EncoderFactory` and checks
that all 24 Java creator names and their overload arities are present on the
Python façade; the Java-side carrier test performs the same signature audit
before exercising the no-argument creators.

The 2010 and 2025 JNI lanes include a provider-owned unknown `DataElement`
probe that round-trips through JNI as a generic Python carrier and consumes
exact, empty, truncated, and overlong payload vectors without assigning
standard wire semantics. Both JNI lanes also encode that carrier into exact,
offset, and undersized `ByteWrapper` windows, checking typed `EncoderException`
mapping and preservation of the caller's bytes/cursor on failure. The fixed
four-octet payload is test infrastructure, not a new IEEE encoding rule. The
same 2010 lane also invokes every standard
time/interval arithmetic operation for the null provider and checks Java
return-category casts only; numeric and vendor arithmetic remains explicitly
deferred.

The 2025 fake-provider lane also exercises a provider-defined
`LogicalTimeFactory` through the normal Python façade. Its opaque time and
interval snapshots preserve the vendor factory name, bytes, callback class
identity, and all seven Java arithmetic operation shapes—including deliberate
nonstandard results—without coercing the carrier to `HLAinteger64Time` or
`HLAfloat64Time`. This is delegation/transport evidence only; arithmetic
semantics for an external vendor remain provider-owned and are not claimed as
portable IEEE behavior.
The same fixture routes the custom carrier through the standard
`createHLAlogicalTime` and `createHLAlogicalTimeInterval` data elements,
checking selected-factory identity on set/get and preserving the provider's
encoded bytes.

Before starting a JVM, a supplied provider can be checked with the repository's
metadata-only preflight:

```powershell
python tools/verify_1516e_provider_jar.py `
  --provider-jar C:\vendor\vendor-rti-2010.jar `
  --api-jar C:\vendor\ieee-1516.1-2010-java-api.jar
```

The check requires the standard `META-INF/services/hla.rti1516e.RtiFactory`
descriptor, verifies that each listed provider class exists in the supplied
provider/dependency class path, rejects a mixed `hla.rti1516_2025` namespace,
and validates the canonical API entries when an API JAR is supplied. It does
not load classes, start a JVM, or replace the TCK/provider run.
