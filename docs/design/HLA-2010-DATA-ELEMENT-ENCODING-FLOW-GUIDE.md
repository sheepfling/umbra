# IEEE 1516.1-2010 C++ DataElement Encoding: Flow Guide

> **Edition and profile boundary:** this page covers only the pinned IEEE
> 1516.1-2010 C++ encoding API and Umbra's separate 2010 codec implementation.
> It is not evidence for the 2025 stream, a complete conformance inventory, or
> the behavior of a full RTI.

This companion makes selected value-encoding behavior easier to follow. The
2010 API remains separate from the [2025 DataElement guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md):
similar byte-order names and one shared unsigned-octet helper do not establish
that the editions' wrappers, composites, or edge cases are equivalent.

## Scope and evidence

- The pinned [2010 `DataElement` declaration](../../third_party/ieee1516.1-2010/include/RTI/encoding/DataElement.h)
  and [basic element declarations](../../third_party/ieee1516.1-2010/include/RTI/encoding/BasicDataElements.h)
  define the public C++ types and operations.
- The separate [2010 basic-element implementation](../../cpp/src/ieee1516_2010_basic_data_elements.cpp#L89)
  chooses explicit big- or little-endian helpers for the corresponding named
  types. Those wrappers call a shared primitive that appends or reads unsigned
  octets; the primitive does not select an order for a type.
- The diagrams describe current source behavior and selected assertions in
  [`ieee1516_2010_encoding_smoke`](../../cpp/tests/ieee1516_2010_encoding_smoke.cpp#L23)
  and [`ieee1516_2010_composite_encoding_smoke`](../../cpp/tests/ieee1516_2010_composite_encoding_smoke.cpp#L53).
  CMake registers these as separate 2010 smoke tests
  ([encoding target](../../CMakeLists.txt#L443),
  [composite target](../../CMakeLists.txt#L453)). This guide did not run them.
- `DataElement::encode()` returns encoded value bytes. This guide does not
  describe process transport framing, federation messages, or network behavior.

## Scalar order is selected by the concrete 2010 type

For the tested 32-bit integer, the BE wrapper emits the most-significant octet
first and the LE wrapper emits the least-significant octet first. The selected
type also controls decoding: interpreting the same octets with the other type
produces a different value, not an automatic byte-order negotiation. The
implementation passes an explicit `ByteOrder` to the shared octet helper; host
byte order is not the selection mechanism.

```mermaid
flowchart TD
  V[Integer value 0x01020304] --> T{Concrete 2010 element type}
  T -->|HLAinteger32BE| B[Emit 01 02 03 04]
  T -->|HLAinteger32LE| L[Emit 04 03 02 01]
  B --> D{Decode with which type?}
  L --> D
  D -->|Matching type| M[Recover the encoded value]
  D -->|Other type| R[Interpret the octets as a different value]
  D -->|Truncated octets| X[EncoderException]
```

The smoke test asserts both exact byte vectors and the changed value after
decoding with the other order. It also checks a big-endian `HLAfloat64BE`
vector, four-octet boolean representation, length-prefixed ASCII and Unicode
strings, opaque data, and rejection of truncated integer input. These are
selected cases, not a claim that every basic element or malformed buffer has
been exhaustively tested.

## Composite shape determines counts, padding, and alternatives

Each composite delegates an element's own value bytes to that element while
the 2010 composite implementation controls collection shape and placement.
The following paths are observed in the separate composite smoke target:

```mermaid
flowchart TD
  C[2010 composite value] --> K{Composite kind}
  K -->|Fixed array| FA[Encode configured elements in order]
  FA --> FAP[No element-count prefix]
  K -->|Variable array| VA[Encode 32-bit BE element count]
  VA --> VAE[Encode each element and required placement]
  K -->|Fixed record| FR[Encode fields in declared order]
  FR --> FRP[Zero-pad before the next field boundary]
  K -->|Variant record| VR[Encode current discriminant]
  VR --> VM{Discriminant mapped?}
  VM -->|Yes| VP[Pad then encode selected alternative]
  VM -->|No| VN[Emit discriminant only]
  FAP --> OUT[Return value octets]
  VAE --> OUT
  FRP --> OUT
  VP --> OUT
  VN --> OUT
```

The exact tested examples make those shape differences concrete: a two-element
fixed array of 32-bit BE integers is eight octets without a count; a two-element
variable array begins with `00 00 00 02`; an octet followed by a 32-bit field
in a fixed record includes three zero padding octets; and the mapped versus
unmapped variant cases produce eight versus one octet. The smoke test also
rejects nonzero fixed-record padding. These examples do not imply that every
prototype, boundary combination, or decode failure has a focused assertion.

The implementation details stay in separate 2010 translation units:
[fixed arrays](../../cpp/src/ieee1516_2010_fixed_array.cpp#L177),
[variable arrays](../../cpp/src/ieee1516_2010_variable_array.cpp#L210),
[fixed records](../../cpp/src/ieee1516_2010_fixed_record.cpp#L142), and
[variant records](../../cpp/src/ieee1516_2010_variant_record.cpp#L269).
The relevant observed cases are grouped in the [2010 composite smoke test](../../cpp/tests/ieee1516_2010_composite_encoding_smoke.cpp#L53).

### Decoding follows the composite shape and validates its placement

The public full-buffer `decode()` calls a shape-specific cursor decoder, then
rejects any bytes left over. The 2010 source has distinct validation branches
for the count-prefixed variable array, field/element padding, and a variant
whose discriminant is not mapped. The diagram is source-derived profile
behavior; the smoke assertions below are narrower than every branch shown.

```mermaid
flowchart TB
  subgraph SHAPES["Choose the configured composite shape"]
    direction TB
    subgraph FIXED_ARRAY["Fixed array"]
      direction LR
      FA_COUNT["Use configured element count"] --> FA_DECODE["Decode slots in order"] --> FA_PADDING["Check zero padding between slots"]
    end
    subgraph VARIABLE_ARRAY["Variable array"]
      direction LR
      VA_COUNT["Read signed 32-bit BE count"] --> VA_NEGATIVE{"Count negative?"}
      VA_NEGATIVE -->|Yes| VA_ERROR["EncoderException"]
      VA_NEGATIVE -->|No| VA_ZERO{"Count zero?"}
      VA_ZERO -->|Yes| VA_DONE["Finish after count prefix"]
      VA_ZERO -->|No| VA_ITEMS["Validate leading alignment;<br/>decode elements and check zero padding"]
    end
    subgraph FIXED_RECORD["Fixed record"]
      direction LR
      FR_FIELDS["Decode fields in declared order"] --> FR_PADDING["Check zero padding before each next field"]
    end
    subgraph VARIANT_RECORD["Variant record"]
      direction LR
      VR_DISC["Decode discriminant"] --> VR_MAPPED{"Alternative mapped?"}
      VR_MAPPED -->|No| VR_UNKNOWN["Consume discriminant only"]
      VR_MAPPED -->|Yes| VR_ALT["Validate placement padding;<br/>decode selected alternative"]
    end
  end
```

The public full-buffer decode methods require the cursor to consume the entire
input; leftover bytes throw `EncoderException`. Truncated elements/fields and
invalid padding also throw. For a variable array, the zero-count path stops
after the count prefix without element-placement padding. For an unmapped
variant discriminant, the cursor stops after the discriminant, so extra bytes
are rejected by the full-buffer check.

The composite smoke source round-trips a fixed array and checks the decoded
element's encoded length; it round-trips a variable array and checks its
decoded count; and it rejects a fixed record with nonzero inter-field padding.
Those assertions do not cover negative variable-array counts, truncated
composite components, trailing bytes, or decoding an unmapped variant
discriminant. Those latter branches are visible in the separate 2010 sources
([fixed array decode](../../cpp/src/ieee1516_2010_fixed_array.cpp#L193),
[variable array decode](../../cpp/src/ieee1516_2010_variable_array.cpp#L234),
[fixed record decode](../../cpp/src/ieee1516_2010_fixed_record.cpp#L160),
[variant record decode](../../cpp/src/ieee1516_2010_variant_record.cpp#L294)),
not independently asserted by this smoke test.

## Keep the edition boundary explicit

The 2010 basic-element wrappers, composite translation units, and smoke tests
are distinct from the 2025 ones. Both streams call the same small
[unsigned-octet byte-order primitive](../../cpp/src/internal/encoding/byte_order.hpp#L15),
but a common low-level helper is not evidence of API or composite parity.
There is also a narrowly shared internal composite layer: the separate
[`composite_2010.hpp`](../../cpp/src/internal/encoding/composite_2010.hpp)
and [`composite_2025.hpp`](../../cpp/src/internal/encoding/composite_2025.hpp)
adapters import checked-size, range, and padding helpers from
[`composite_primitives.hpp`](../../cpp/src/internal/encoding/composite_primitives.hpp).
The 2010 codecs call those operations through the 2010 namespace; their codec
translation units and smoke assertions remain edition-specific. This shared
type-free utility does not establish composite-format parity. Keep changes and
explanations edition-scoped, and consult the [separate 2025 guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md)
for its own sources and focused tests. In particular, do not project its
`HLAextendableVariantRecord`, `HLAlogicalTime`, or
`HLAlogicalTimeInterval` DataElement wrappers backward into the 2010 encoding
API. The 2010 [`LogicalTime`](../../third_party/ieee1516.1-2010/include/RTI/LogicalTime.h),
[`LogicalTimeInterval`](../../third_party/ieee1516.1-2010/include/RTI/LogicalTimeInterval.h),
and [`LogicalTimeFactory`](../../third_party/ieee1516.1-2010/include/RTI/LogicalTimeFactory.h)
interfaces are separate API surfaces, not those 2025 DataElement wrappers.

The pinned 2010 [`DataElement` declaration](../../third_party/ieee1516.1-2010/include/RTI/encoding/DataElement.h#L36)
also makes its C++ ownership/signature boundary explicit: `clone()` returns
`std::auto_ptr<DataElement>`, and full-buffer `decode()` returns `void`. The
2025 companion records the separate 2025 declaration. These method signatures
do not select or prove an encoded value format; the byte-layout observations
above are specific to the 2010 implementation and smoke cases.
