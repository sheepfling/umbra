# FOM and FDD internals

This domain owns Umbra's private model-input pipeline: XML document access,
schema validation, FOM/MIM composition, FDD materialization, and the retained
catalog queried by other runtime domains. It validates and describes a model;
it does not itself create a public federation, deliver callbacks, or claim
conformance.

## Key files

- fom_validation.hpp and fom_composition.hpp define the private validation and
  composition boundaries.
- fom_catalog.hpp retains validated type, class, parameter, attribute,
  dimension, and transportation metadata.
- fdd_document.hpp represents the materialized private FDD input.
- The libxml2_fom_* files implement XML/XSD parsing and Annex C-oriented
  composition behavior when the optional validator profile is enabled.

## Working here

- Keep vendor XML, schemas, MIM, and supplied examples immutable under
  [third_party/](../../../../third_party/README.md).
- Add a small fixture under cpp/tests/data/ for a distinct model input; do not
  add unreviewed external corpora there.
- Keep FOM semantics separate from runtime policy. The
  [federation](../federation/README.md) domain decides when validated input is
  used to create or extend runtime state.
- Preserve source diagnostics. A caller needs the original model location to
  report useful errors.

## Tests and references

The direct tests are
external_2025_fom_catch2.cpp, external_siso_fom_catch2.cpp,
libxml2_fom_composer_catch2.cpp, and libxml2_fom_validator_catch2.cpp under
[cpp/tests/](../../../tests/). Use
[FOM validation design](../../../../docs/fom/FOM-VALIDATION-DESIGN.md) for the
implemented boundary and deliberate limits.
