# Handle internals

This domain owns private representations of IEEE handle values, their
encoding helpers, and the directories that resolve FOM names into stable
runtime handles. It supports the public standard binding; it does not decide
federation membership, delivery, ownership, or time behavior.

## Key files

- The type-specific handle headers hold private handle representations and
  validation helpers.
- The <kind>_handle_directory files keep FOM-derived name and handle mappings
  stable for an active federation.
- fom_handle_allocator retains collision-checked, CSPRNG-generated assignments
  by typed FDD name for the RTI registry lifetime, so recreating a federation
  execution with the same FDD reuses those handles.
- handle_variable_array_encoding.hpp is the 2025 typed handle wire adapter:
  its HLAvariableArray<HLAbyte> payload contains a stable one-octet handle
  kind followed by the eight-octet identity. Keep the explicit type-code table
  stable across future language bindings so cross-API reconstitution can be
  implemented consistently.
- handle_variable_array_encoding_2010.hpp retains the legacy eight-octet
  identity format through the shared private wire primitive. Do not change or
  share that format with the 2025 typed encoding.
- 2025_handle_definition.hpp contains the 2025-only common class definition
  macro. It must not be used by the 2010 implementation.

The directory implementations consume the [FOM catalog](../fom/README.md).
Federation-owned code uses them through narrow lookup operations rather than
reconstructing names or raw values itself. The 2025 handle assignment cache is
owned by one RTI registry and is shared across its federation executions; it is
not a cross-process persistence store.

## Where new code belongs

- Add a private handle value, byte representation, or strict validity rule
  here.
- Add a directory entry only when a FOM-derived handle must remain stable for
  a federation.
- Put public binding definitions in cpp/src/, not here.
- Put lifecycle, message-routing, or callback decisions in their owning
  [federation](../federation/README.md),
  [time](../time/README.md), or
  [callbacks](../callbacks/README.md) domain.

## Tests and references

The focused native tests are under
[cpp/tests/](../../../tests/): attribute, dimension, federate, interaction,
message-retraction, object-class, object-instance, parameter, region, and
transportation handle test files. FOM-backed directory behavior is also
covered by the FOM and federation-management tests.

Read the [internal map](../README.md) before adding a dependency outside this
domain.
