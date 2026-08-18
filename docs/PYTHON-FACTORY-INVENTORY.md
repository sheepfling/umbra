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
| Handles and collections | Attribute, dimension, federate, interaction, object, parameter, region, transportation, and message-retraction handle factories; set/map/pair-list factories | Native C++ has private handle construction/decoding plus standard containers, but no Java-style factory interface | Bind immutable handle values and their encodings first; then implement the Java-shaped Python factories without exposing C++ private identities |
| Logical time | `LogicalTimeFactoryFactory`, `HLAfloat64TimeFactory`, `HLAinteger64TimeFactory` | Both reference C++ time factories and values are implemented | Bind time/interval value semantics and encoding first; expose the factories only when the Python types can preserve their standard behavior. `RTIambassador.getTimeFactory` also depends on the current federation-management profile becoming package-safe |
| Authorization | `AuthorizerFactory`, `AuthorizerFactoryFactory` | API headers exist; Umbra has no authorizer implementation | Implement the native authorization slice and its credentials/result values before binding |

The Java `RTIambassador` also declares handle/map factory accessors. Those are
not C++ 2025 API methods: C++ uses value types and standard containers instead.
The Python binding will provide Java-compatible accessors only after the
corresponding Python values and factories exist.

## Dependency order

1. Complete native encoding values and `EncoderFactory`.
2. Bind encoded bytes, exceptions, and the shared Python data-element lifetime
   model.
3. Bind handles and add the handle/set/map factory family.
4. Bind reference logical time/interval values and factories.
5. Bind `RTIambassador` factory accessors as the native service profile makes
   each one valid.
6. Add authorization factories after native authorization exists.
7. Repeat the inventory for the independent `hla.rti1516e` C++ lane; no 2010
   provider is implied by the 2025 implementation.

## Rules

- Provider discovery is edition-specific. A 2010 provider registers only in
  the 2010 entry-point group, and a 2025 provider only in the 2025 group.
- Do not return a placeholder factory. An unavailable standard factory raises
  the documented RTI exception at the provider boundary.
- Keep provider implementation modules under `umbra._native`; only
  `hla.rti1516e` and `hla.rti1516_2025` are public API namespaces.
