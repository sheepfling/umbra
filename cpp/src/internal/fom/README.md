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
- fom_wire_encoding.hpp is the neutral wire-shape seam. It preserves the
  source encoding label, maps standard and explicitly enabled RPR labels to
  generic structural properties, and reports codec availability separately
  from catalog recognition. It intentionally has no dependency on the RPR
  byte-codec implementation.
- fom_rpr_wire_encoding.hpp is the validation-only RPR adapter. It is the
  sole layer allowed to promote the five RPR structural labels and the four
  RPR unsigned-integer representations from metadata-only to available.
- fom_wire_codec.hpp contains the isolated, byte-level RPR structural and
  scalar adapters. It does not depend on the public RTI DataElement hierarchy:
  enclosing callers supply child decoders and bounds where the legacy format
  omits them.
- fdd_document.hpp represents the materialized private FDD input.
- The libxml2_fom_* files implement XML/XSD parsing and Annex C-oriented
  composition behavior when the optional validator profile is enabled.

The selected `FomStandardEdition` is the IEEE 1516.2 model/schema edition;
it is independent of the public IEEE 1516.1 C++ binding.  The default is
2025.  The embedded development profile accepts the exact
`fomEdition=2010` setting as an opt-in compatibility mode and rejects other
edition spellings, including ambiguous `202x` values.  The 2010 slice builds
an edition-neutral catalog for lookup, publication, and registration; it does
not synthesize a 2025 FDD or MOM projection from the 2010 MIM.

RPR-specific labels such as `RPRlengthlessArray` and
`RPRextendedVariantRecord` are accepted only through the explicit
`FomSourceCompatibility::rpr_2010` profile. The catalog retains their exact
source spelling and neutralizes their structural meaning. Their
`FomWireCodecStatus::available` claim is backed only by the validation-only
RPR adapter and its isolated structural/scalar byte adapters: null termination,
externally bounded concatenation, zero alignment padding, the length-delimited
extended-variant envelope, and exact-width big-endian unsigned integers.
Lengthless arrays still require an enclosing byte bound and child decoder;
callers may provide an external element count, or consume fixed-size elements
until the bound when the element boundary cannot add padding. A count is
required where padding could make the final boundary ambiguous. Composite RPR
datatypes are not silently promoted to a public
runtime serialization API. Standard HLA array/record shapes retain their
existing codec-available status.
RPR basic aliases such as `RPRunsignedInteger16BE` retain their source width
and byte order and are promoted only when the exact RPR declaration matches
those fields. RPR simple/enumerated representations use the same exact-width
adapter. `RPRboolean` remains an enumerated type represented by `HLAoctet`, not
an invented RPR primitive class.

Unvendored 2010 schemas/MIM and reviewed external modules are enabled only by
explicit CMake paths.  `tools/verify_external_2010_fom_resources.py` checks
the configured resource snapshot against
`compliance/fom/external-2010-fom-resources.json` before the optional tests
run.  The sibling Target Radar module is an opt-in registration-only fixture;
it is not part of the packaged resource set.

`tools/verify_rpr_standard_boundary.py` is a source-level guard: the neutral
descriptor may be used by standard RTI code, while the full RPR codec and its
promotion adapter may enter only through the FOM composer dependency cone.

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
external_2010_fom_catch2.cpp, libxml2_fom_composer_catch2.cpp, and
libxml2_fom_validator_catch2.cpp under
[cpp/tests/](../../../tests/). Use
[FOM validation design](../../../../docs/fom/FOM-VALIDATION-DESIGN.md) for the
implemented boundary and deliberate limits. The optional SISO test covers the
full pinned 2025 cross-edition rejection set, 2010 Space/RPR Foundation lookup
catalogs, the strict per-module RPR 2.0 boundary, and a bounded embedded
two-federate RPR payload exchange; it does not establish full scenario
simulation or external runtime interoperability. See the [RPR FOM load-gap
backlog](../../../../docs/fom/RPR-FOM-LOAD-GAP-BACKLOG.md) for the current
ownership classification and follow-on work.
