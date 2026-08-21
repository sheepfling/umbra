# Python encoding binding design

## Decision

The Python API will be a binding façade over a provider's actual HLA encoder;
it will not independently implement HLA wire encodings in Python.

The authoritative 2025 Java API presents encoding through
`hla.rti1516_2025.encoding.EncoderFactory`. Its elements implement
`DataElement`, encode to Java `byte[]`, and decode from Java `byte[]` or a
`ByteWrapper`. The corresponding C++ API declares concrete `DataElement`
classes, whose encoding results are `VariableLengthData` values. Python's
portable binary representation is therefore immutable `bytes`, not a new
public Python ownership model for the C++ `VariableLengthData` pointer.

~~~text
Python public API                    Provider boundary
-----------------                    -----------------
encoding.EncoderFactory  ----------> Java EncoderFactory
     DataElement.encode() -> bytes         DataElement.toByteArray()
     DataElement.decode(bytes)             DataElement.decode(byte[])

encoding.EncoderFactory  ----------> Umbra C++ concrete DataElement
     DataElement.encode() -> bytes         encode() -> VariableLengthData
     DataElement.decode(bytes)             VariableLengthData -> decode(...)
~~~

The C++ conversion is private to `umbra._native.rti1516_2025`: it copies
between `bytes` and a native `VariableLengthData` at a call boundary. Python
will never receive a borrowed C++ data pointer, and no portable Python class
will claim `takeDataPointer`/`setDataPointer` ownership semantics that Java
does not have.

## Current implementation gate

Umbra currently implements C++ VariableLengthData value semantics, reference
logical-time encoders, all official BasicDataElements helpers, and the
separate official HLAopaqueData helper. The native C++ helper catalog covers
HLAinteger32BE, HLAunsignedInteger32BE, HLAinteger32LE,
HLAunsignedInteger32LE, HLAinteger64BE/LE, HLAunsignedInteger64BE/LE,
HLAboolean, HLAunicodeString, HLAoctet, HLAbyte, HLAASCIIchar,
HLAASCIIstring, HLAunicodeChar, HLAfloat32BE/LE, HLAfloat64BE/LE, the 16-bit
signed/unsigned big- and little-endian helpers, and the big-/little-endian
octet-pair helpers. HLAopaqueData has a separately tested HLAbyte
HLAvariableArray wire form and caller-owned external-buffer behavior; its
portable binding deliberately exposes copied `bytes`, `size`, and indexed
access rather than borrowed external memory. This completes the native
official C++ helper catalog plus that specific predefined data helper. The
native HLAfixedRecord, HLAfixedArray, HLAvariableArray, HLAvariantRecord, and
HLAextendableVariantRecord foundations are source-traced and tested. The
shared Python provider now exposes fixed-record, variant-record, fixed-array,
and variable-array contracts; nested fixed-record/fixed-array/variable-array
ownership, nested fixed-record alternatives, and composite discriminants are
verified across providers. The native provider also exposes a deliberately
provider-specific `HLAextendableVariantRecord` extension with length-prefixed
alternatives and unknown-alternative skipping. It is not added to the shared
Java-shaped factory because its Java 2025 counterpart does not expose the
 mapping-registration operations needed to preserve those C++ semantics in the
 shared Python contract. The JNI bridge now binds the standard Java handle and
logical-time `EncoderFactory` carriers through provider-scoped Python façade
methods. Those methods call the exact Java overloads and return typed shells
whose bytes and validation still come from the selected C++ factories. The
Java extendable-variant façade registers its C++ mapping on the first
`setVariant` and remains composable inside a standard Java fixed record through
a private native-composition marker. All of these provider-scoped extensions
are intentionally not promoted to the provider-neutral shared Python contract
because the native provider has no corresponding standard `EncoderFactory`
interface. The
native `HLAlogicalTime` and `HLAlogicalTimeInterval` wrappers likewise remain
native-only: their factory lifetime and bounded reference-profile
`decodeFrom` behavior need a provider-neutral time-factory contract before
they can be represented as portable Python encoding elements. The
shared Python provider surface currently exposes the independently verified
twenty-six-form slice (`HLAinteger16BE`, `HLAinteger16LE`, `HLAinteger32BE`,
`HLAinteger32LE`, `HLAinteger64BE`, `HLAinteger64LE`, `HLAfloat32BE`, `HLAfloat32LE`,
`HLAfloat64BE`, `HLAfloat64LE`,
`HLAunsignedInteger16BE`, `HLAunsignedInteger16LE`, `HLAunsignedInteger32BE`, `HLAunsignedInteger32LE`,
`HLAunsignedInteger64BE`, `HLAunsignedInteger64LE`, `HLAbyte`, `HLAoctet`,
`HLAASCIIchar`, `HLAASCIIstring`, `HLAunicodeChar`, `HLAoctetPairBE`,
`HLAoctetPairLE`, `HLAopaqueData`, `HLAboolean`, and
`HLAunicodeString`); native-only
helpers must not be advertised merely because their C++ definitions exist.

For the Java-backed provider, each element shell also accepts the exact
standard Java `ByteWrapper` overloads through `encode(wrapper)` and
`decode(wrapper)`. The portable Python `encode()`/`decode(bytes)` forms remain
copying byte-oriented operations, so cursor access is an explicit provider
extension rather than a second wire-format implementation.
The Java façade likewise accepts the exact fixed-array `DataElement...`
overload and exposes `HLAvariableArray.resize(int)` as provider-scoped
extensions; the shared factory/size and append-oriented contracts remain
unchanged for C++ parity.

RtiFactory.getEncoderFactory() is therefore available for that shared
twenty-six-form slice plus the proven fixed-record/fixed-array/variable-array/
variant-record composites. The Java provider additionally offers all exact
standard handle, logical-time, and extendable-variant creator methods as
provider-scoped extensions; the native factory additionally offers the
provider-specific `createHLAextendableVariantRecord` method described above. It
remains inappropriate to publish a full Python EncoderFactory, expose a C++
ownership model, or claim complete
BasicDataElements behavior until every additional provider path has its own
source-derived vectors and API contract.

The Java adapter may expose an encoding factory only once it wraps its Java
objects behind the same Python interfaces. It must not return the raw JPype
object as the portable API result.

## First bindable encoding slice

The first shared slice is complete for twenty-six standard primitives plus
provider-owned `HLAfixedRecord`, `HLAfixedArray`, `HLAvariableArray`, and
`HLAvariantRecord` composites:
HLAinteger16BE, HLAinteger16LE, HLAinteger32BE, HLAinteger32LE, HLAinteger64BE,
HLAinteger64LE, HLAfloat32BE, HLAfloat32LE, HLAfloat64BE, HLAfloat64LE,
HLAunsignedInteger16BE, HLAunsignedInteger16LE, HLAunsignedInteger32BE, HLAunsignedInteger32LE,
HLAunsignedInteger64BE, HLAunsignedInteger64LE, HLAbyte, HLAoctet,
HLAASCIIchar, HLAASCIIstring, HLAunicodeChar, HLAoctetPairBE,
HLAoctetPairLE, HLAopaqueData, HLAboolean, and
HLAunicodeString.
The array types follow the Java `DataElementFactory` contract: the
factory supplies the prototype and fresh decoded children, while each
provider owns the actual array and element lifetimes. The native binding
delegates alignment, cardinality, padding, and type checks to C++
`HLAfixedArray`/`HLAvariableArray`; the Java adapter delegates to the selected
Java factory. Fixed records delegate heterogeneous component ownership and
record-offset alignment to the provider; the shared fixed-record `set` method
is an explicit copied-value
convenience because the C++ surface can replace slots while the Java surface
normally exposes fixed-record replacement through the component returned by
`get` and initializes fixed arrays through its factory.
Variant records delegate discriminant identity, alternative ownership, and
discriminant-to-alternative padding to the provider. The shared API mirrors
the Java `setVariant`/`setDiscriminant`/`getDiscriminant`/`getValue` methods;
unmapped discriminants preserve their provider-defined discriminant-only wire
form and return `None` from `getValue`.
Each later primitive is selected only after Umbra supplies the matching C++
implementation:

1. Add/verify the C++ element and its Catch2 byte-vector, round-trip, malformed
   input, and boundary tests.
2. Add only the matching public Python `encoding.DataElement` and concrete
   element contract, using the official Java method names.
3. Bind the C++ object rather than reproducing its endian or range logic in
   Python; expose Python `bytes` at the boundary.
4. Wrap the same Java element created by `EncoderFactory` in the JPype
   provider.
5. Run one shared conformance suite against both providers with identical
   expected byte vectors and decode failures.

Each further primitive, array, record, and handle encoder follows that same
vertical slice. The Python API remains intentionally smaller than the full
Java `EncoderFactory` until the matching C++ and Java paths are both proven.

## Sources checked

The mapping above was checked against the official IEEE 1516.1-2025 API bundle
(`java/hla/rti1516_2025/encoding/{EncoderFactory,DataElement,ByteWrapper}.java`
and `cpp/RTI/{VariableLengthData,encoding/DataElement,BasicDataElements,HLAopaqueData,HLAfixedRecord,HLAfixedArray,HLAvariableArray}.h`).
The bundle is available from the IEEE downloads page; it is used as a local
development reference and is not copied into this repository.
