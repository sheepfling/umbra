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

Umbra currently implements C++ `VariableLengthData` value semantics and its
reference logical-time encoders. It does **not** implement the C++ basic
`DataElement` definitions declared in `RTI/encoding/BasicDataElements.h`.
Consequently, publishing a Python `EncoderFactory` now would be a façade over
unimplemented native behavior. `RtiFactory.getEncoderFactory()` must continue
to report that the shared encoding slice is unavailable.

The Java adapter may expose an encoding factory only once it wraps its Java
objects behind the same Python interfaces. It must not return the raw JPype
object as the portable API result.

## First bindable encoding slice

The first slice is one standard primitive, selected only after Umbra supplies
the matching C++ implementation (for example `HLAinteger32BE`):

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
and `cpp/RTI/{VariableLengthData,encoding/DataElement,BasicDataElements}.h`).
The bundle is available from the IEEE downloads page; it is used as a local
development reference and is not copied into this repository.
