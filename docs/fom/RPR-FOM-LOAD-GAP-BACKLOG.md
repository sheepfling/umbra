# RPR FOM load-gap backlog

Status: working implementation note, reviewed 2026-08-29.

This note answers a narrow question: when the RPR FOM does not load, is the
cause an Umbra RTI/schema-pipeline limitation, an RPR source-model constraint,
or a combination of the two? It covers the pinned RPR 2.0 family and its Link
16/Link 11 extensions, with a small note about the 2025-labelled RPR 3.0
input. It is not a conformance or interoperability claim.

## Classification

Use these labels when adding a new failure:

- **Umbra/RTI** — a missing loader, composer, catalog, FDD, runtime, or wire
  capability in Umbra.
- **RPR/source** — a property of the supplied RPR XML, its edition metadata,
  modular packaging, or its custom encoding vocabulary.
- **Both** — the RPR input exposes a real source-model requirement and Umbra
  has not implemented the generic capability needed to consume it.
- **Policy/intentional** — a deliberate safety, provenance, or edition guard;
  it is not a defect merely because the input is rejected.

## Current answer in one page

The RPR 2.0 material is an ordered 16-file family, not one standalone FOM
root. The selected load set is:

- RPR Foundation and Enumerations;
- RPR Base, Physical, Logistics, Minefield, Switches, DER, Warfare,
  Communication, SIMAN, Underwater Acoustics, Aggregate, and Synthetic
  Environment; and
- the Link 16 and Link 11/11B extension modules.

The current evidence is:

- The default Umbra profile is 2025. Every pinned SISO input declares the
  IEEE 1516-2010 namespace/schema, so the strict 2025 guard rejects all 22
  pinned inputs as `invalid_model`.
- On the explicit 2010 path, the official IEEE 1516.1/1516.2-2010 resource
  set and standard MIM are now pinned by
  `compliance/fom/external-2010-fom-resources.json` and compose into a lookup
  catalog.
- Fifteen of the 16 RPR 2.0/Link inputs pass strict validation. The explicit
  RPR 2010 compatibility path normalizes the one known
  `reference/identification` value in `RPR-Enumerations_v2.0.xml` from prose
  to a URI in memory before official DIF validation; the source file remains
  unchanged and the module retains a normalization warning.
- The composed catalog now retains the exact RPR `<encoding>` or
  representation label and maps the five exercised RPR structural labels plus
  the four `RPRunsignedInteger*BE` scalar representations. It also retains
  array element/cardinality and variant discriminant metadata, and records an
  available status only for independently tested RPR adapters.
- If that module is simply omitted, composition gets past schema validation
  but fails because the remaining family refers to the missing
  `BreachedStatusEnum8` declaration. Omitting it is therefore not a fix.
- Umbra now composes the complete ordered family with the official 2010 DIF
  schema, creates/registers an object through the 2025 API, and delivers a
  bounded RPR payload exchange between two embedded federates. The exchange
  covers a null-terminated object identifier, an RPR boolean octet, and a
  64-bit unsigned RPR representation. This still does not establish full
  composite payload serialization, external-RTI interoperability, or a full
  scenario simulation.

The short version is: the first hard failure is RPR-source/schema metadata,
while the later missing pieces are Umbra's generic RPR-dialect, 2010 FDD, and
wire/runtime support. The family must not be made to pass by silently changing
the source XML or relabelling it as 2025.

## Failure inventory

| ID | Load gate | What prevents loading or limits the result | Primary classification | Status |
| --- | --- | --- | --- | --- |
| `RPR-LOAD-001` | Acquisition | The external SISO XML is intentionally not vendored. A normal checkout without the explicitly supplied corpus root cannot open the RPR files. | Policy/provenance; Umbra tooling | Expected boundary |
| `RPR-LOAD-002` | XML safety | Missing/unreadable files, malformed XML, DTDs, external entities, or network references are rejected by the general safe-input gate. No such issue is recorded for the pinned RPR files. | Umbra/RTI policy | Generic guard, not an RPR finding |
| `RPR-LOAD-003` | Edition selection | RPR 2.0 is 2010-shaped but the default profile selects 2025. A 2010 namespace/schema cannot be accepted by the 2025 DIF policy. | Umbra policy plus RPR edition metadata | Observed and intentional |
| `RPR-LOAD-004` | 2010 resource selection | The 2010 schema and standard MIM are external inputs. Without them, Umbra cannot validate or compose the 2010 RPR family. | Umbra/RTI packaging and provenance | Resolved for the reviewed development lane; still externally supplied |
| `RPR-LOAD-005` | Edition mixing | A 2010 MIM/FOM and a 2025 MIM/FOM cannot be composed in one model set. This protects the catalog from cross-edition declarations. | Umbra/RTI policy | Observed and intentional |
| `RPR-LOAD-006` | DIF schema validation | `RPR-Enumerations_v2.0.xml` contains a reference-identification value that libxml2 rejects as an `xs:anyURI`. | RPR/source | Resolved in explicit RPR 2010 compatibility mode; strict mode still rejects it |
| `RPR-LOAD-007` | Family dependency closure | Removing the failing Enumerations module leaves references such as `BreachedStatusEnum8` unresolved. A partial family is not a valid substitute for the ordered family. | Both: RPR packaging/dependency plus Umbra reference checking | Resolved by loading the complete ordered family |
| `RPR-LOAD-008` | Standalone-module use | Link 16, Link 11/11B, and most RPR extension modules are not complete federation roots on their own. They need the standard MIM and the declarations supplied by their family. | RPR/source load shape | Expected family constraint |
| `RPR-LOAD-009` | Cross-module composition | After individual XSD validation, the composer still has to resolve duplicate declarations, inherited members, data-type references, variant discriminants, dimensions, and transportation/update metadata across the ordered set. | Umbra/RTI generic composition | Resolved for the reviewed RPR 2.0 family catalog gate |
| `RPR-LOAD-010` | RPR datatype vocabulary | RPR uses custom structural encodings including `RPRnullTerminatedArray`, `RPRlengthlessArray`, `RPRpaddingTo32Array`, `RPRpaddingTo64Array`, and `RPRextendedVariantRecord`, plus the four `RPRunsignedInteger*BE` scalar representations. Their meaning is not represented by the ordinary HLA encoding names alone. | Both | Resolved for the isolated structural/scalar codec boundary; broad composite projection remains open |
| `RPR-LOAD-011` | Catalog fidelity | The catalog retains the source label, neutral shape, codec status, array element/cardinality, variant discriminant, record members, and basic width/byte-order metadata. Available status is tied to independently tested RPR adapters; child datatype binding and general FDD serialization remain separate. | Umbra/RTI | Structural/scalar catalog slice complete; broad composite projection open |
| `RPR-LOAD-012` | 2010 FDD projection | The 2010 compatibility lane intentionally stops at an edition-neutral lookup catalog. It does not materialize a 2010 FDD or an RTI-owned 2025 MOM projection from the legacy model. | Umbra/RTI | Intentional first-slice boundary; open later milestone |
| `RPR-LOAD-013` | Runtime object/data use | RPR-specific object registration and a two-federate attribute payload exchange are proven for one representative class. Interactions, region use, time use, ownership, and scenario replay remain unproven. | Umbra/RTI | Bounded payload-delivery slice complete; broader runtime gap remains |
| `RPR-LOAD-014` | Scale and resource behavior | The complete family composes and registers an object, but memory, load time, handle allocation, and lookup behavior at the full reported scale are not yet measured in Umbra. | Umbra/RTI | Unmeasured stress gap |
| `RPR-LOAD-015` | Publication/version metadata | Some RPR publications are labelled with a later publication year while their XML still selects the 2010 namespace/schema. Treating the filename or publication date as the edition would select the wrong loader. | RPR/source plus Umbra policy | Observed; explicit edition setting required |

## Detailed backlog items

### `RPR-LOAD-001` — Make the external-input boundary easy to diagnose

The RPR XML remains outside this repository. The manifest
`compliance/fom/external-siso-fom-corpus.json` pins paths, SHA-256 values,
namespace, schema location, and expected status, while
`UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY` supplies the actual files.

This is not an RTI parser failure. It is an intentional acquisition and
licensing boundary. The next tooling improvement is a single preflight report
that says which root is missing, which fixture is missing, and which digest is
stale before Catch2 attempts composition.

### `RPR-LOAD-003` / `RPR-LOAD-004` — Keep the edition decision explicit

RPR 2.0 must be loaded with `fomEdition=2010` and a matching 2010 schema/MIM
root. `2025` remains the default for the public binding, and ambiguous values
such as `202x` are rejected. That is an Umbra policy decision intended to
prevent a source labelled with a future or publication year from being sent
through the wrong DIF schema.

The eventual product decision is one of these, and must be recorded before
changing the loader:

1. retain an explicit 2010 catalog-only compatibility lane;
2. implement a complete 2010 FDD/runtime lane; or
3. provide a separately named legacy RTI surface rather than pretending that a
   2010 model is a native 2025 FDD.

### `RPR-LOAD-006` — Resolve the Enumerations reference value

The strict 2010 schema failure is source-specific. The problematic source
field is in `RPR-Enumerations_v2.0.xml` and the diagnostic identifies
`reference/identification` as an invalid `xs:anyURI` value. The source is
preserved unchanged and the strict test records the rejection; it is not
repaired in place.

The strict path still reports the source defect. The explicit compatibility
path implements the narrow normalization option:

Umbra implements this as `FomSourceCompatibility::rpr_2010`, selected by the
explicit `fomEdition=2010` runtime lane. It changes only the in-memory
`SISO-REF-010-00v20-0` identification value, records a module warning, and
does not rewrite or rehash the source file. The normalization must not turn a
source-level repair into an unlabelled standards or interoperability claim.

### `RPR-LOAD-007` — Validate dependency closure before composition

The family is modular. The 2010 standard MIM supplies the HLA declarations;
RPR Foundation supplies shared RPR declarations; later modules supply
declarations used by still later modules. A load set that omits the
Enumerations module demonstrates the failure clearly: references to
`BreachedStatusEnum8` remain unresolved.

The current implementation loads the complete ordered family through the
external SISO manifest and exercises the missing-symbol dependency in the
full catalog gate. A later machine-readable family manifest can improve
diagnostics with:

- required module order;
- required companion modules;
- declared dependency names;
- a pre-composition missing-symbol report; and
- a final catalog count/name fingerprint.

The missing-symbol report should distinguish a missing RPR declaration from a
missing standard MIM declaration. That distinction tells us whether to fix the
source load set or the RTI's standard-model catalog.

### `RPR-LOAD-010` / `RPR-LOAD-011` — Add generic support for RPR wire shapes

The adjacent workbench records 76 datatype normalizations for the RPR 2.0
family. The important source encodings are:

- null-terminated arrays;
- lengthless arrays whose extent is derived from surrounding message context;
- 32-bit and 64-bit alignment padding; and
- an RPR extended-variant-record convention; and
- unsigned 8-, 16-, 32-, and 64-bit big-endian scalar representations.

These are RPR-specific structural pressures, but the implementation response
is generic. The parser preserves the source label for audit and maps it to a
neutral internal structural kind. `fom_wire_codec.hpp` now defines and tests
the byte boundary. The rules below follow the published RPR guidance in
[SISO-STD-001-2015, section 6.8.5](https://cdn.ymaws.com/www.sisostandards.org/resource/resmgr/standards_products/siso-std-001-2015_grim_rpr_f.pdf);
the IEEE 1516-2010 modules remain the source-of-truth FOM inputs.

- null-terminated arrays append and require a unique `0x00` sentinel;
- lengthless arrays concatenate child bytes and decode only with an enclosing
  bound and a child decoder; an external element count is required when the
  element boundary can add padding, so final padding cannot be mistaken for
  another element;
- padding arrays emit/validate zero octets to the next 32- or 64-bit boundary;
  and
- extended variants carry the discriminant followed immediately by an
  unsigned 32-bit big-endian alternative length and the length-delimited
  alternative, with strict truncation checks.
- `RPRunsignedInteger8BE`, `RPRunsignedInteger16BE`,
  `RPRunsignedInteger32BE`, and `RPRunsignedInteger64BE` encode exact-width
  unsigned values in big-endian order, with range and truncation checks.

The catalog's `available` status is now coupled to those five structural labels
and four scalar representation labels through the validation-only RPR adapter.
The neutral descriptor used by the standard RTI does not include that adapter.
It is deliberately a source-dialect capability, not a claim that every RPR
composite child type or public runtime payload path is complete.

### `RPR-LOAD-012` — Decide whether a 2010 FDD is a product requirement

The current composer returns a catalog for 2010 and intentionally does not
serialize the legacy model to an FDD. This avoids inventing a 2025-shaped FDD
from a 2010 source that may use different legal table shapes. It is an Umbra
boundary, not an RPR defect.

If a future requirement is to create a federation execution from the complete
RPR 2.0 family, the work needs a 2010-aware FDD materializer, MOM projection,
logical-time selection policy, and public service error mapping. That work
should follow a successful full-family catalog composition and must be tested
separately from source validation.

### `RPR-LOAD-013` / `RPR-LOAD-014` — Add runtime and scale gates after loading

The completed runtime slice is deliberately bounded:

1. load the complete, reviewed RPR family into the selected edition lane;
2. query a representative deep object class and its RPR-defined attributes;
3. create/register an object instance; and
4. publish, discover, update, and reflect a null-terminated RPR identifier,
   RPR boolean octet, and RPR unsigned scalar between two embedded federates.

The remaining runtime work is to bind more composite RPR child datatypes,
exercise interactions/time/DDM/ownership, and record load time, catalog counts,
and failure diagnostics at full corpus scale.

Only after those pass should the backlog expand to attribute updates,
interactions, time, DDM, ownership, or scenario replay. Those are runtime
capability questions, not evidence that the FOM XML itself was loadable.

## Generic gates to keep visible

These are valid ways for any FOM, including RPR, to fail. They should remain
diagnostically distinct from RPR-specific issues:

- source cannot be opened;
- source cannot be read or parsed safely;
- XML namespace and selected schema do not match;
- official DIF validation fails;
- required MIM/FOM declarations are absent;
- duplicate declarations conflict during Annex C composition;
- a data type, class, dimension, transportation type, or update rate is
  referenced but not declared;
- inherited object/interaction members conflict;
- the completed model cannot be materialized into the selected FDD shape; or
- the selected public runtime has no compatible logical-time, catalog, or wire
  codec support.

The service-level mapping already distinguishes source-open, read/parse,
schema-validation, composition, and logical-time outcomes in
`docs/fom/FOM-VALIDATION-DESIGN.md`. New RPR diagnostics should reuse those
categories rather than introducing an RPR-specific catch-all error.

## Recommended order

1. Keep the 22-fixture manifest and strict 2025 rejection guard intact.
2. Keep the official 2010 resource manifest and ordered RPR compatibility gate
   covered by tests.
3. Keep the neutral datatype normalization and explicit codec-status contract
   covered by the RPR catalog test.
4. Keep the isolated RPR wire target and its golden/malformed vectors green.
5. Keep the completed scalar/structural adapter and two-federate payload test
   green without broadening the public 2025 API implicitly.
6. Bind additional RPR composite types to child codecs and add independent
   interaction/time/DDM/ownership contracts.
7. Re-run full 2010 validation and catalog composition, recording counts and
   names.
8. Decide whether 2010 FDD/MOM/runtime projection is required.

## Evidence locations

- [External SISO test](../../cpp/tests/external_siso_fom_catch2.cpp) — current
  2025 guard, Space/RPR catalog checks, full 2010 RPR compatibility gate,
  registration test, and bounded two-federate payload exchange.
- [External SISO manifest](../../compliance/fom/external-siso-fom-corpus.json)
  — unvendored paths and immutable fixture digests.
- [FOM validation design](FOM-VALIDATION-DESIGN.md) — edition selection,
  catalog-only 2010 boundary, and service-error categories.
- [FOM stress backlog](FOM-STRESS-CORPUS-BACKLOG.md) — family-level promotion
  boundaries and scenario limits.
- `cpp/src/internal/fom/libxml2_fom_composer.cpp` — current generic
  composition and validation rules.
- `cpp/src/internal/fom/fom_catalog.hpp` — intentionally narrow catalog
  projection that preserves RPR wire metadata without becoming a public wire
  API.
- `cpp/src/internal/fom/fom_wire_codec.hpp` and
  `cpp/tests/fom_wire_codec_catch2.cpp` — isolated RPR structural/scalar codec
  seam and golden/malformed-input evidence.
- `cpp/src/internal/fom/fom_rpr_wire_encoding.hpp` and
  `cpp/tests/rpr_standard_isolation_catch2.cpp` — the opt-in promotion seam
  and the standard-RTI isolation regression.
- `tools/verify_rpr_standard_boundary.py` — source-level guard against
  importing the RPR codec into standard RTI translation units.
- Adjacent inspection sources —
  `sturdy-broccoli/artifacts/fom_workbench_bundle/frontend/validation_packets/siso-rpr-2.0/fom_validation_report.json`
  and `sturdy-broccoli/docs/rpr_type_normalization_notes.md`.
