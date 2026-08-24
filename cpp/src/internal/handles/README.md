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
- handle_variable_array_encoding.hpp contains the shared private encoding
  helper used by handle values.

The directory implementations consume the [FOM catalog](../fom/README.md).
Federation-owned code uses them through narrow lookup operations rather than
reconstructing names or raw values itself.

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
