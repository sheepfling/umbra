# Private wire-encoding utilities

This directory owns small byte-level primitives. `byte_order.hpp` is the
single implementation for fixed-width unsigned serialization and decoding;
call sites must pass `ByteOrder::big` or `ByteOrder::little` explicitly so the
wire contract is visible where a value is encoded. `handle_variable_array.hpp`
builds on that utility for the shared eight-octet `HLAvariableArray` wire
shape.

`transport_wire.hpp` is the private process-boundary adapter for the
2010/2025-neutral transport protocols. Its function names explicitly say
`BigEndian`, so framing call sites keep the byte order visible while sharing
one tested implementation.

`variable_length_data_2025.hpp` owns the 2025-only copy from
`VariableLengthData` to an owned byte vector. It is used by runtime and
process-service paths that retain encoded values beyond the public call, and
by public encoders that supply their type-specific invalid-buffer message.
`validateEncodedBytes` centralizes the 2025 invalid-pointer check, while
`appendEncodedBytes` appends directly to an existing buffer for encoders such
as logical time without creating an intermediate owned vector. It must not be
included by the 2010 binding.

`variable_length_data_2010.hpp` is the separate 2010 composite-decoder
adapter. It owns the same pointer validation and copy shape for the 2010
`rti1516e::VariableLengthData` type, while callers retain their existing
type-specific diagnostics. Neither version-specific adapter may be used by
the other binding stream.

`composite_primitives.hpp` contains the type-free checked-add, alignment,
range, and zero-padding operations. The legacy RPR FOM wire codec reuses only
these type-free primitives directly; its FOM namespace and error messages
remain separate. `composite_2010.hpp` and `composite_2025.hpp` are separate
namespace adapters over the same primitives, so the composite source files
retain their 2010/2025 include and namespace boundaries while sharing one
tested implementation.

`fnv1a.hpp` contains the version-neutral byte hash used by the 2010 and 2025
`DataElement::hash` implementations. It accepts only a pointer and length, so
the public binding types and their version-specific return conversions remain
outside the shared helper.

The primitives do not define either public binding stream. The 2010 and 2025
wrappers remain separate (`handle_variable_array_encoding_2010.hpp` versus
`handle_variable_array_encoding.hpp`), and the 2025-only composite helpers
live in `composite_2025.hpp`. Version-specific exception text, public types,
and handle definitions stay in their respective source paths.

These headers are private implementation details. Public IEEE binding types
and their existing exception behavior remain the ownership boundary, while
the binding-specific wrappers supply the appropriate public type and error
message.
