# IEEE 1516e-2010 JNI null provider

This package is the first bounded Java/JNI bridge for the official IEEE
1516.1-2010 (`rti1516e` / `hla.rti1516e`) API. It is intentionally a null
provider:

- the C++ library exposes the official 2010 headers and all 151 pure virtual
  `RTIambassador` services;
- `RTIambassadorFactory` creates a concrete C++ ambassador;
- `connect`/`disconnect` cross the JNI boundary with a bounded connection
  state and standard duplicate-connect/internal-error behavior; and
- `NativeTypeRoundTrip` exercises the standard JVM carrier boundary for
  primitives, UTF-16 strings, `ByteWrapper`, every 2010 basic/composite data
  element, all nine C++ encoded handle families plus the Java-only
  `TransportationTypeHandle`, standard handle/time/collection factories,
  handle collections, sets, maps, pair lists, records, callback records,
  and the four standard logical-time families. Its malformed-wire matrix
  sends truncated encodings for all 24 data-element kinds, plus declared
  length overruns for the four length-prefixed carriers, and requires the
  standard Java `hla.rti1516e.encoding.DecoderException` identity. The same
  24 carriers are decoded through both Java `DataElement.decode` overloads;
  the `ByteWrapper` path verifies non-zero offsets, exact cursor advancement,
  and preservation of a trailing octet.
  Its proxy carriers implement the supplied `hla.rti1516e` interfaces, while
  each seeded value is decoded and re-encoded by the official C++ 1516.1 types
  before it returns to Java; and
- `NativeTypeRoundTrip.encoderFactoryCarrier()` exposes the standard
  `EncoderFactory` shape (alongside the standard handle, logical-time, and
  collection factory shapes) for the same probe, and the registered
  `NativeRtiFactory.getEncoderFactory()` returns that carrier for bounded
  scalar/composite encoding and malformed-input checks; and
- every other RTI ambassador service throws
  `hla.rti1516e.exceptions.RTIinternalError`, with no federation, callback,
  logical-time, or transport service state maintained.

It is an ABI and binding bring-up artifact, not a conformance implementation.
The connection slice is evidence that one service family can cross C++ → JNI
→ standard Java → JPype. The type probe is deliberately independent of RTI
service semantics: it proves carrier and encoding transport, not federation
behavior or callback delivery.
The standard Java API JAR remains a caller-supplied dependency and is never
bundled into the bridge.

Build it with the locally obtained IEEE 1516.1-2010 Java API JAR:

```powershell
.\build.ps1 `
  -JavaApiJar C:\path\to\ieee-1516.1-2010-java-api.jar `
  -OutputDirectory C:\path\to\umbra-rti-jni-2010-build `
  -RunSmokeTest
```

The output directory contains `umbra-rti-jni-2010.jar` and
`umbra_rti_jni_2010.dll` (or the platform equivalent). Put both beside the
standard API JAR and use the standard `RtiFactoryFactory`/`ServiceLoader`
route; the named provider is `Umbra JNI IEEE 1516e Null RTI`.

For a directly consumable product directory, run the Python builder from
`packages`:

```text
python build_umbra_jni.py --edition 2010 \
  --java-api-jar C:\path\to\ieee-1516.1-2010-java-api.jar \
  --run-smoke-test
```

The output includes the provider JAR, native library, copied standard API JAR,
verification/dependency manifests, and `run.py`. The bridge registers both
the standard `RtiFactory` and top-level `LogicalTimeFactory` providers. Run
`python run.py` for the surface check, or pass `--mode native`/`--mode types`
for the bounded lifecycle and carrier checks.

The build also includes `SurfaceSmokeTest`, which resolves every declared
`RTIambassador` method through the dynamic proxy without invoking service
behavior. `NativeTypeRoundTripTest` is the corresponding carrier check; the
connection smoke remains a separate bounded capability slice.
That carrier check also reflects the supplied 2010 `EncoderFactory`, requiring
the complete 24-name creator set and its Java overload arities before invoking
the no-argument creators. This keeps a transplanted API JAR from silently
changing the Python/JNI carrier surface. It also retains composite creator
arguments in the standard carrier probe and checks fixed-array factory/varargs,
variable-array varargs, fixed-record additions, and variant discriminant/value
dispatch at the Java boundary.

The time portion of that check is value-sensitive: it exercises initial,
final, zero, epsilon, explicit constructor values, exact eight-octet encodings,
decode offsets with trailing bytes, and malformed-length rejection. It does
not claim that the null provider implements RTI service behavior.

The opt-in Python leg additionally feeds all 16 shared integer64/float64
logical-time wire vectors through the standard Java factory proxy. It checks
offset-window decoding, C++-owned canonical output (including signed zero),
and `CouldNotDecode` mapping for malformed values. Arithmetic is deliberately
not inferred from this null-provider carrier probe; it remains covered by the
provider-neutral Java fixture and direct native profile.
The same Python test also round-trips all ten standard handle families,
including opaque provider-issued `RegionHandle` and
`MessageRetractionHandle` values, all four typed handle sets, the
federation-information set,
attribute/parameter value maps, and the attribute-region pair list through the
provider's standard carrier factories, so the return path is tested as Python
values rather than only as raw Java objects.
The companion callback/record probe converts the three supplemental callback
records and all six standard return records (`RangeBounds`, federation
information, save/restore status pairs, message-retraction, and time-query)
into their provider-neutral Python values.
The Python provider-return matrix then routes all 50 generated non-void
`RTIambassador` service overloads through JNI-produced Java return carriers for
both integer64 and float64 logical-time implementations; JPype primitive
wrappers are normalized to Python `bool`, `float`, and `int` values.
The companion factory-surface check resolves this provider through the standard
`hla.rti1516e.RtiFactoryFactory`/`ServiceLoader` path, consumes its standard
`EncoderFactory`, and verifies all 150 ambassador method names and 172 declared
overloads on both the raw Java proxy and Python façade before exercising the
bounded connect/disconnect lifecycle.
Its service-argument matrix then feeds the remaining 169 non-lifecycle
overload declarations for both integer64 and float64 time configurations
through the Python conversion path into the real JNI ambassador; each null
service terminates with the standard `RTIinternalError`. This is dispatch and
transport evidence only, not stateful RTI behavior.

The detailed carrier catalog records the Requirements Lab contracts for each
logical-time, handle/collection, basic-value, malformed-wire, ByteWrapper, and
encoder façade matrix. `tools/verify_1516e_type_roundtrip.py` checks those
references and their files as part of the structural gate, so a transplanted
carrier test retains its normative linkage.

For the Python leg, build the artifacts first and run the opt-in JPype test
with the standard API JAR on the class path:

```powershell
$env:UMBRA_ENABLE_JNI_2010_TYPE_ROUNDTRIP_TESTS = "1"
$env:UMBRA_JNI_2010_BRIDGE_ARTIFACT_DIRECTORY = (Resolve-Path .\out\jni-2010)
python -m unittest packages/umbra-rti-jpype/tests/test_jpype_2010_jni_type_roundtrip.py -v
```
