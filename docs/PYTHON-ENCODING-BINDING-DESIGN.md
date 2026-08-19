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
HLAvariableArray wire form and caller-owned external-buffer behavior. This
completes the native official C++ helper catalog plus that specific predefined
data helper. The native HLAfixedRecord, HLAfixedArray, HLAvariableArray,
HLAvariantRecord, and HLAextendableVariantRecord foundations are also
source-traced and tested, but they are deliberately not exposed through the
shared Python provider:
constructed-element lifetime, discriminant/alternative mapping, and a
cross-provider creation API need their own portable contract first. The
native `HLAlogicalTime` and `HLAlogicalTimeInterval` wrappers likewise remain
native-only: their factory lifetime and bounded reference-profile
`decodeFrom` behavior need a provider-neutral time-factory contract before
they can be represented as portable Python encoding elements. The
shared Python provider surface deliberately exposes only the independently
verified common four scalar forms; native-only helpers must not be advertised
merely because their C++ definitions exist.

RtiFactory.getEncoderFactory() is therefore available for that shared
four-form slice. It remains inappropriate to publish a full Python
EncoderFactory, expose a C++ ownership model, or claim complete
BasicDataElements behavior until every additional provider path has its own
source-derived vectors and API contract.

The Java adapter may expose an encoding factory only once it wraps its Java
objects behind the same Python interfaces. It must not return the raw JPype
object as the portable API result.

## First bindable encoding slice

The first shared slice is complete for four standard primitives:
HLAinteger32BE, HLAunsignedInteger32BE, HLAboolean, and HLAunicodeString.
Each later primitive is selected only after Umbra supplies the matching C++
implementation:

1. Add/verify the C++ element and its Catch2 byte-vector, round-trip, malformed
   input, and boundary tests.
2. Add only the matching public Python `encoding.DataElement` and
   `encoding.HLAinteger32BE` contracts, using the official Java method names.
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
