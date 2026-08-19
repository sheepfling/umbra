# Python factory inventory

This inventory is the implementation gate for the Java-shaped Python API. It
is derived from the official IEEE 1516.1-2025 Java API archive and compared to
the current Umbra C++ implementation. A Python factory is public only after it
can create, decode, and validate the standard Python value it returns.

## Discovery and top-level factories

| Java API factory | Python location | Native status | Python status |
| --- | --- | --- | --- |
| `RtiFactoryFactory` | `hla.rti1516_2025.RtiFactoryFactory` | C++ creates one linked RTI through `RTIambassadorFactory` | Implemented with Python entry points, the `HLA_RTI_FACTORY_NAME` selection convention, and `UmbraRtiFactory` |
| `RtiFactory.getRtiAmbassador` | `hla.rti1516_2025.RtiFactory` | Implemented | Implemented |
| `RtiFactory.getEncoderFactory` | `hla.rti1516_2025.RtiFactory` | No C++ encoding runtime | Deliberately raises `RTIinternalError`; it is not a usable encoder factory |

## Provider transport adapters

| Provider | Implementation namespace | Current public capability | Deliberate boundary |
| --- | --- | --- | --- |
| Umbra C++ | `umbra._native.rti1516_2025` | Connected/unjoined foundation through pybind11 | The public objects are Python values; pybind objects stay private to the provider. |
| Java 2025 RTI | `umbra._java.rti1516_2025` | Same foundation through optional JPype, with a fake-runtime contract test | A Java vendor JAR is selected at JVM start. Raw Java objects are available only through explicit provider-specific `unwrap_java_*` methods while shared Python value families are incomplete. |
| Mock vendor reference | `umbra._java.mock_rti1516_2025` | Registered `umbra-mock-java` alias, JAR validation, and fixed Java factory selection | Demonstrates the separate-adapter pattern for a supported vendor without bundling a JAR. |

The Java adapter registers the transport alias `java`; it is not a claim that
`java` is the vendor's standard `RtiFactory.rtiName()`. After Java factory
selection, `rtiName()` returns the actual Java provider name.

## Value and service factories

| Family | Java API factories | C++ basis | Required before Python binding |
| --- | --- | --- | --- |
| Encoding | `EncoderFactory`, `DataElementFactory` | No concrete encoder/data-element runtime yet | Implement all standard data elements, byte-wrapper rules, encode/decode behavior, and exceptions; then bind the complete factory family |
| Handles and collections | Attribute, dimension, federate, interaction, object, parameter, region, and transportation handle factories; set/map/pair-list factories (Java exposes no public message-retraction factory) | Native C++ uses its provider-owned handle decoders and standard containers; Java exposes the standard handle/set/map factories | Implemented for all Java-declared handle decoders, federate/dimension/region/attribute sets, and attribute/parameter maps. Python returns immutable snapshots plus mutable factory builders; C++ transport handles use Umbra's private codec because the C++ RTIambassador has no transport decode service. Message-retraction decoding remains provider-owned. |
| Logical time | `LogicalTimeFactoryFactory`, `HLAfloat64TimeFactory`, `HLAinteger64TimeFactory` | Both reference C++ time factories and values are implemented | Bind time/interval value semantics and encoding first; expose the factories only when the Python types can preserve their standard behavior. `RTIambassador.getTimeFactory` also depends on the current federation-management profile becoming package-safe |
| Authorization | `AuthorizerFactory`, `AuthorizerFactoryFactory` | `HLAplainTextPassword`, the native reference `HLAauthorizer`/factory, and a static library-forwarding boundary exist; no RID-backed runtime selection or service lifecycle exists | Keep authorization factories and values unbound until native `HLAauthorizer` configuration, lifecycle, and service checks exist |

The Java `RTIambassador` declares handle/set/map factory accessors while C++
uses value types and standard containers instead. The Python binding exposes
the same Java-shaped accessors, but returns provider-neutral Python builders;
native handle decoders still validate through Umbra's real C++ handle codecs.

## Dependency order

1. Complete native encoding values and `EncoderFactory`.
2. Bind encoded bytes, exceptions, and the shared Python data-element lifetime
   model.
3. Bind handles and add the handle/set/map factory family. **Complete** for
   the Java-declared factories; message-retraction remains an internal
   provider boundary because Java has no corresponding public decoder.
4. Bind reference logical time/interval values and factories.
5. Bind `RTIambassador` factory accessors as the native service profile makes
   each one valid.
6. Add authorization factories after RID-backed native authorization runtime
   selection and service lifecycle exist.
7. Repeat the inventory for the independent `hla.rti1516e` C++ lane; no 2010
   provider is implied by the 2025 implementation.

## Rules

- Provider discovery is edition-specific. A 2010 provider registers only in
  the 2010 entry-point group, and a 2025 provider only in the 2025 group.
- Do not return a placeholder factory. An unavailable standard factory raises
  the documented RTI exception at the provider boundary.
- Keep provider implementation modules under `umbra._native`; only
  `hla.rti1516e` and `hla.rti1516_2025` are public API namespaces.
