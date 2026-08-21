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
| `RtiFactory.getEncoderFactory` | `hla.rti1516_2025.RtiFactory` | Native C++ basic data elements, fixed/variable arrays, fixed records, variant records, and the provider-specific extendable variant are bound through pybind11 | Implemented for the shared twenty-six-form primitive slice plus `HLAfixedArray`, `HLAvariableArray`, `HLAfixedRecord`, and `HLAvariantRecord`; nested fixed-record/fixed-array/variable-array ownership, nested fixed-record alternatives, and composite discriminants are verified. The native factory additionally exposes `createHLAextendableVariantRecord`; the external Java adapter now binds standard handle, logical-time, and extendable-variant carriers through C++-validated factories, while the shared Python factory remains provider-neutral |

## Provider transport adapters

| Provider | Implementation namespace | Current public capability | Deliberate boundary |
| --- | --- | --- | --- |
| Umbra C++ | `umbra._native.rti1516_2025` | Connected/unjoined foundation through pybind11 | The public objects are Python values; pybind objects stay private to the provider. |
| Java 2025 RTI | `umbra._java.rti1516_2025` | Same foundation through optional JPype, with a fake-runtime contract test | A Java vendor JAR is selected at JVM start. Raw Java objects are available only through explicit provider-specific `unwrap_java_*` methods while shared Python value families are incomplete. |
| Mock vendor reference | `umbra._java.mock_rti1516_2025` | Registered `umbra-mock-java` alias, JAR validation, and fixed Java factory selection | Demonstrates the separate-adapter pattern for a supported vendor without bundling a JAR. |
| Umbra C++ through Java/JNI | `umbra._java.jni_rti1516_2025` | Registered `umbra-jni` alias; assembles the exact IEEE API JAR, bridge JAR, and native library before using Java `ServiceLoader`; the companion package has a standard-entry-point discovery gate | Configuration-only adapter; it adds no RTI state or service behavior and requires those artifacts to be supplied externally. |

The Java adapter registers the transport alias `java`; it is not a claim that
`java` is the vendor's standard `RtiFactory.rtiName()`. After Java factory
selection, `rtiName()` returns the actual Java provider name. The dedicated
JNI adapter registers `umbra-jni` and likewise leaves the standard Java
`RtiFactoryFactory`/`ServiceLoader` as the provider-selection boundary.

## Value and service factories

| Family | Java API factories | C++ basis | Required before Python binding |
| --- | --- | --- | --- |
| Encoding | `EncoderFactory`, `DataElementFactory` | Native C++ basic data elements, fixed/variable arrays, fixed records, variant records, and a C++-only extendable variant, plus Java `EncoderFactory` | Implemented for twenty-six basic elements and the provider-owned fixed/variable-array/fixed-record/variant-record contracts with typed errors; nested fixed/variable arrays, nested fixed-record alternatives, and composite discriminants cross both providers. The external Java route additionally round-trips standard handle, logical-time, and extendable-variant data elements through C++ factories. The extendable variant remains outside the provider-neutral shared Python contract |
| Handles and collections | Attribute, dimension, federate, interaction, message-retraction, object, parameter, region, and transportation handle factories; interaction-class set, attribute/region pair-list, set/map factories | Native C++ uses its provider-owned handle decoders and standard containers; Java exposes the standard handle/set/map factories | Implemented for all Java-declared handle decoders, including `MessageRetractionHandleFactory`, `InteractionClassHandleSetFactory`, `AttributeSetRegionSetPairListFactory`, federate/dimension/region/attribute sets, and attribute/parameter maps. Python returns immutable snapshots plus mutable Java-shaped builders and `AttributeRegionAssociation(ahset, rhset)` values; C++ transport handles use Umbra's private codec because the C++ RTIambassador has no transport decode service. |
| Logical time | `RTIambassador.getTimeFactory`, `HLAfloat64TimeFactory`, `HLAinteger64TimeFactory` in the Java-shaped surface currently verified here | Both reference C++ time factories and values are implemented; C++ `HLAlogicalTimeFactoryFactory`/`LogicalTimeFactoryFactory` are edition-specific static helpers | Provider-neutral time/interval values and factory operations are implemented through the ambassador boundary. Keep the C++ static factory helpers out of the shared API unless a matching Java service surface is verified |
| Authorization | `AuthorizerFactory`, `AuthorizerFactoryFactory` | `HLAplainTextPassword`, the native reference `HLAauthorizer`/factory, and a static library-forwarding boundary exist | The raw Java JNI route now publishes `HLAauthorizer` through the standard `AuthorizerFactoryFactory` ServiceLoader and delegates all authorization decisions to C++; keep authorization values out of the provider-neutral shared Python contract until a canonical value/configuration model exists |

The Java `RTIambassador` declares handle/set/map factory accessors while C++
uses value types and standard containers instead. The Python binding exposes
the same Java-shaped accessors, but returns provider-neutral Python builders;
native handle decoders still validate through Umbra's real C++ handle codecs.

## Dependency order

1. Extend native encoding values and `EncoderFactory` beyond the twenty-six-form
   slice. The C++-only extendable-variant mapping is available as a
   provider-specific extension; the JNI adapter also consumes the standard Java
   creator, registering its mapping at the first `setVariant` call without
   inventing a Java method absent from the verified surface.
2. Bind encoded bytes, exceptions, and the shared Python data-element lifetime
   model.
3. Bind handles and add the handle/set/map factory family. **Complete** for
   the Java-declared factories; message-retraction remains an internal
   provider boundary because Java has no corresponding public decoder.
4. Bind reference logical time/interval values and factories.
5. Bind `RTIambassador` factory accessors as the native service profile makes
   each one valid.
6. Extend the raw Java authorization route with RID-backed configuration and
   service checks when that native runtime exists; do not widen the shared
   Python contract with provider-specific credential objects.
7. Repeat the inventory for the independent `hla.rti1516e` C++ lane; no 2010
   provider is implied by the 2025 implementation.

## Rules

- Provider discovery is edition-specific. A 2010 provider registers only in
  the 2010 entry-point group, and a 2025 provider only in the 2025 group.
- Do not return a placeholder factory. An unavailable standard factory raises
  the documented RTI exception at the provider boundary.
- Keep provider implementation modules under `umbra._native`; only
  `hla.rti1516e` and `hla.rti1516_2025` are public API namespaces.
