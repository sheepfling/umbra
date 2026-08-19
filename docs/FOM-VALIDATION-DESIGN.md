# FOM validation boundary

## Status

Umbra now has an opt-in, private per-document validator, an Annex C-guided
FOM/MIM composition preflight, a schema-validated composed FDD artifact, and a
private selector for the two IEEE reference logical-time implementations. The
default packaged profile keeps federation-management methods on the generated
`RTIinternalError` fallback. The explicit
`UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=ON` profile wires
`createFederationExecution`, `createFederationExecutionWithMIM`,
`destroyFederationExecution`, `joinFederationExecution`, and
`resignFederationExecution` through an in-process registry only after that
pipeline succeeds. It also returns the selected logical-time factory from
`getTimeFactory` only to a joined federate. That profile is intentionally
non-installable: its libxml2 linkage and source-tree resource path are not yet
part of the SDK contract.

## Authoritative inputs

Umbra vendors seven byte-for-byte official IEEE 1516.2-2025 resources under
`third_party/ieee1516.2-2025/resources`:

- DIF, FDD, and OMT XML schemas;
- `HLAstandardMIM-2025.xml`; and
- the three restaurant module examples supplied with the standard.

`tools/ieee_1516_2_resources.py` imports them only after verifying the outer
and nested archive hashes, then records individual resource digests. CTest
checks those digests and SDK installation packages the resources with their
IEEE attribution. The source archive itself is intentionally not committed.

## Required runtime boundary

~~~text
development-profile Create/Join service
          |
federation-management coordinator
          |
FOM source resolver -> secure XML/XSD validation + composition preflight
          |                                      |
          +--------------------------------------+
                         |
              materialized FDD -> reference-time selector
                         |
                         v
          EmbeddedFederationRegistry
~~~

The registry receives `PrevalidatedFomModule` descriptors only. Each descriptor
keeps the exact supplied FOM/MIM designator separate from the canonical local
source path used to open it. The registry does not open files, parse XML,
select a schema, or declare a module valid. That keeps a future parser backend
from leaking parser-specific objects or ad-hoc XML rules into federation state.

`FederationManagementCoordinator` is the private pre-commit layer between the
resolver and registry. It loads the supplied MIM (or RTI-owned standard MIM)
first, validates every FOM, composes a schema-valid FDD, selects a compatible
reference time implementation, and only then emits an immutable
`FederationDefinition`. It performs no registry mutation itself. The
development-profile adapter maps only these determined outcomes to official
exceptions, creates only after preparation succeeds, and uses the registry's
atomic definition-and-membership join transition for additional modules.

The validator must:

1. preserve the supplied FOM/MIM designator exactly while canonicalizing its
   permitted local source separately for safe file access;
2. reject network resolution, external entities, and DTD processing;
3. validate the IEEE 1516-2025 namespace and the selected vendored schema;
4. return structured diagnostics without treating a well-formed document as a
   valid FOM merely because it has an `objectModel` root; and
5. revalidate and inspect every requested FOM module with the selected MIM
   before the registry mutation is committed; and
6. materialize and validate an FDD only after the implemented Annex C
   transformations succeed, while refusing any input set that the official FDD
   schema cannot represent.

The official inputs use `IEEE1516-DIF-2025.xsd` in their schema locations. The
runtime schema policy must be explicit and reviewed: it must resolve only the
vendored 2025 schemas, not trust an arbitrary schema path embedded in a user
document. The stricter OMT schema is retained for validation and test policy;
the FDD schema validates Umbra's generated RTI-facing artifact.

### Executable 2025 schema policy

The three official 2025 schemas have deliberately distinct roles. The
resource-integrity test verifies all three byte-for-byte before the C++ tests
run; no schema imports or includes another XSD, so that sealed three-file set
has no untracked transitive schema dependency.

| Schema | Umbra policy | Executable boundary |
| --- | --- | --- |
| `IEEE1516-DIF-2025.xsd` | Validate each supplied MIM, FOM, or SOM module before composition. | The standard MIM and all three supplied Restaurant modules are accepted; malformed XML, wrong namespace, and DTD-bearing sources are rejected. |
| `IEEE1516-FDD-2025.xsd` | Validate only the RTI-facing FDD materialized from a compatible module set. | The MIM plus Restaurant base produces a schema-valid FDD; fixed-order materialization is also checked for repeatability. |
| `IEEE1516-OMT-2025.xsd` | Retain as the strict complete-object-model schema; never substitute it for individual module validation. | The standard MIM and Restaurant modules are intentionally rejected when individually checked against OMT because their cross-module key/keyref references are unresolved. That guards the DIF policy against an accidental schema swap. |

The final OMT row is a policy test rather than a negative verdict about the
official modules. A future complete-model serializer may add a positive OMT
vector once it can represent all required references without relying on a
private FDD projection.

Richer external model families are deliberately kept in the
[FOM stress-corpus backlog](FOM-STRESS-CORPUS-BACKLOG.md), outside this
vendored authority boundary.  They require their own provenance, license,
edition, module-order, and expected-verdict review before they become inputs.

## Development-profile error boundary

The public adapter will translate only determined validator outcomes:

| Validator outcome | Official service error |
| --- | --- |
| source cannot be opened | `CouldNotOpenFOM` / `CouldNotOpenMIM` |
| source cannot be read or parsed safely | `ErrorReadingFOM` / `ErrorReadingMIM` |
| schema or model validation fails | `InvalidFOM` / `InvalidMIM` |
| individually valid modules cannot form one model | `InconsistentFOM` |
| FDD-documented standard time type conflicts with selected factory | `InconsistentFOM` |
| requested reference-time factory is unavailable | `CouldNotCreateLogicalTimeFactory` |

`FederationExecutionAlreadyExists` is checked only after all validation and
composition work succeeds. This preserves atomicity: an invalid request must
not create a partially registered federation.

## Backend and tests

`LibXml2FomValidator` and `LibXml2FomModuleComposer` are the first backend
components. They are compiled only with
`-DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON`; setting
`-DUMBRA_FETCH_LIBXML2=ON` fetches the pinned official libxml2 2.15.3 source
with its SHA-256 checked. The backend uses libxml2's XSD document-validation
API, explicitly supplies a vendored schema rather than trusting a document's
schema location, reads the primary source from an explicitly opened local
buffer, denies secondary XML resources with a parser-context loader, rejects
DTD-bearing input, and parses with network and XXE access disabled. It is a
private test/runtime component until a public service depends on it and its
dependency packaging is finalized.

## Current composition scope

The preflight follows the Annex C root-to-leaf direction for objects and
interactions. It compares the supported OMT sections (`objects`, `interactions`,
`dimensions`, `time`, `tags`, `synchronizations`, `transportations`,
`switches`, `updateRates`, and `dataTypes`) as a structural tree. A repeated
named component can add missing compatible sub-elements; conflicting scalar
sub-elements fail the entire preflight. Annex C.8 switches are the recorded
exception: the first named setting remains authoritative, an equivalent
duplicate is ignored, and a non-equivalent duplicate is ignored with a private
preflight warning rather than rejecting the whole module set.

Targeted checks currently cover incompatible data-type kinds, duplicate
enumerated values assigned to different enumerator names, extension of a
non-extendable variant record, direct or range-expanded discriminants that
would select more than one named alternative, and the prohibited `HLAother`
alternative in an extendable variant record, plus the Annex C.8 first-setting
and warning behavior for repeated switches. It also rejects an object-class
attribute or interaction-class parameter that overloads a name declared by an
ancestor. The backend then emits only FDD-shaped information: `modelIdentification` deliberately
contains only Annex C.1 `Composed_From` references, referenced notes receive
new deterministic `UmbraNoteN` labels before their `noteReferences` values are
merged, and matching service-usage entries combine their `isUsed` value with a
logical OR. The generated XML is validated against the official relaxed FDD
schema before it becomes an immutable `MaterializedFdd` artifact.

When the completed model supplies object or interaction class tables, their
top-level class must be `HLAobjectRoot` or `HLAinteractionRoot`, respectively.
Every other class is structurally nested below that standard root. The check
runs only after compatible DIF modules have merged, so a module that omits an
entire table remains representable; a nonstandard top-level root does not. Its
source/test traceability is
`compliance/fom-root-hierarchy-requirements-contract.json`, not public FOM
conformance evidence.

The preflight also resolves every direct `dataType` value in the merged
supported model against the composed data-type key. As in the vendored 2025
OMT XSD, that key includes basic-data representations as well as simple,
enumerated, reference, array, fixed-record, and variant-record declarations.
It runs only after all supplied modules have merged, so an extension can refer
to a type supplied by a later module. `NA` remains a permitted no-type marker.
That general resolver only establishes the complete-model name: narrower table
predicates decide whether the resolved declaration family is valid for each
column. The object-attribute and interaction-parameter columns, and now the
array-data Element Type column, each require a simple, enumerated, reference,
array, fixed-record, or variant-record type. A raw basic-data representation is
not valid in those columns. `HLAtoken` requires no Umbra name exception because
the official MIM declares it as an array type. For `arrayData`, an omitted
Element Type (and the established `NA` marker) remains representable for an
incomplete DIF row; a supplied name is checked after composition. The bounded
attribute-`NA` companion predicate checks supplied direct fields after merging:
transportation and order must be non-`NA`, while update type and update
condition must be `NA`. A partial DIF attribute can be completed by a later
compatible module; a complete FDD still requires transportation and order.
The official DIF schema represents dimensions at object-class scope, not
per-attribute scope, so this predicate does not invent a class-wide mapping for
the source's available-dimensions phrase (RL-051). Its source/test trace is
`compliance/fom-attribute-na-companion-requirements-contract.json`; it is
private preflight evidence, not a FOM conformance claim. A separate attribute
sharing predicate rejects a supplied `sharing` value of `Neither` paired with a
supplied `valueRequired=true`; an omitted Value Required field remains a partial
DIF row rather than an invented default. Its source/test trace is
`compliance/fom-attribute-value-required-sharing-requirements-contract.json`;
RL-052 records its broad exported clause metadata. A separate dynamic
attribute update-condition predicate rejects a supplied empty or NA condition
when the supplied update type is Conditional or Periodic. It preserves an
omitted condition in a partial DIF row and does not parse periodic-rate grammar
or initial-condition prose. Its source/test trace is
`compliance/fom-attribute-dynamic-update-condition-requirements-contract.json`;
the conflicting Static/NA direction remains outside the predicate under
RL-046. The same completed-model
category predicate applies to a supplied fixed-record Field Type and
variant-record Alternative Type; omitted member types remain representable for
incomplete DIF rows. That bounded record-member slice is traced by
`compliance/fom-record-member-data-type-requirements-contract.json`. Its two
Requirements Lab candidates retain the export's broad `clause-4` metadata;
RL-047 records the precise source-heading provenance issue. The Dimension
table's supplied Input data type members apply the same category boundary,
while `NA` remains a supported no-type marker. The paired Dimension input-
description predicate requires non-`NA` text when the DIF `inputDataTypes`
sequence supplies no named type; this covers the official empty-list form and
the established explicit `NA` marker without deciding whether a suitable named
type exists. Named types may still use `NA` or amplifying text. Its source/test
trace is `compliance/fom-dimension-input-data-description-requirements-contract.json`.
The separate NA-exclusivity predicate keeps that no-type marker out of an
otherwise named `inputDataTypes` sequence: it rejects an explicit `NA` beside a
supplied named type, but does not infer suitability, cardinality, duplicate
names, or description unambiguity. Its trace is
`compliance/fom-dimension-input-data-type-na-exclusivity-requirements-contract.json`.
A distinct 2025
variant-record predicate requires a supplied Discriminant Type to resolve
specifically to an enumerated-data declaration. It preserves an omitted field
for incomplete DIF, but does not accept `NA`: the source rule supplies no
no-type marker for that column. The discriminant predicate is traced by
`compliance/fom-variant-discriminant-data-type-requirements-contract.json`;
the paired discriminant-enumerator predicate validates comma-separated
enumerators, bracketed two-endpoint ranges, and standalone one-per-record
`HLAother` through
`compliance/fom-variant-discriminant-enumerator-requirements-contract.json`.
It preserves an omitted field for incomplete DIF. The paired membership
predicate checks every supplied individual enumerator and range endpoint
against the selected enumeration after all modules merge; an enumeration with
no supplied members remains incomplete DIF rather than an invented closed set.
That bounded source/test trace is
`compliance/fom-variant-discriminant-enumerator-membership-requirements-contract.json`;
RL-049 records its broad exported clause provenance. The paired range-semantics
predicate retains enumerator rows in their source-table declaration order,
expands a bracketed range over the inclusive span between its declared
endpoints, and rejects a member reached by more than one named alternative.
It represents `HLAother` as the complement of explicitly assigned members, not
as a literal enumerator. The text does not establish a separate
lower-versus-upper endpoint convention, so this bounded predicate uses the
inclusive table span rather than inventing one. Its source/test trace is
`compliance/fom-variant-discriminant-enumerator-range-semantics-requirements-contract.json`;
RL-049 and RL-050 record the exported provenance limits. The Dimension input
predicates do not infer whether a named type is suitable, whether a textual
description is unambiguous, or whether named types need `NA` descriptions. The
category, NA-exclusivity, and no-named-type description traces are
`compliance/fom-dimension-input-data-type-requirements-contract.json` and
`compliance/fom-dimension-input-data-type-na-exclusivity-requirements-contract.json`,
and `compliance/fom-dimension-input-data-description-requirements-contract.json`;
RL-048 records the candidate's broad exported clause metadata.
The separate Static/NA direction remains deferred: the source table's NA
wording conflicts with the supplied 2025 Restaurant FOM's Static/On change
row. RL-046 records that cross-artifact tension rather than allowing a generic
preflight rule to reject the official example. This does not defer the
independent bounded Conditional/Periodic non-NA predicate.
The time table applies a narrower completed rule: a logical-time or interval
representation must name a simple, enumerated, array, fixed-record, or
variant-record data type, or `NA`. It deliberately does not yet prove that a
lookahead representation is non-negative.
User-supplied and synchronization tags apply a related completed rule: their
data type may name a simple, enumerated, reference, array, fixed-record, or
variant-record data type, or `NA`; basic-data names are not permitted there.
Representation fields now receive a bounded composition check as well. Simple
representation names must resolve in the composed model; the official
MIM/Restaurant `HLAboolean` example is retained under the reviewed RL-009
compatibility interpretation even though the source table wording calls for a
basic-data row. An enumerated data type has a separate completed predicate:
its supplied representation must resolve to a basic-data declaration. Ordinary
reference-data representations must resolve to a simple, enumerated, array,
fixed-record, or variant-record type and cannot point to a basic or another
reference type. The two standard instance-identifier
references are handled as an explicit exception with their standardized
`HLAunicodeString` and `HLAobjectInstanceHandle` representations. This is still
not the full OMT reference checker: lookahead non-negative inference and the
remaining table-specific referential constraints remain separately tracked
work. The enumerated predicate is traced by
`compliance/fom-enumerated-representation-requirements-contract.json`.
The FOM-specific synchronization-capability predicate is also intentionally
deferred: its 2025 source record requires `NA`, while the supplied Restaurant
FOM uses other schema-permitted capability values. Umbra does not add a rule
that rejects that supplied official example without a reviewed interpretation.

One reference-data rule is now checked independently: a
`referenceDataType`'s `referenceClass` must name an object class in the final
composed hierarchy. This too runs after all modules merge, so a later module
can supply the class. For an ordinary `referencedAttribute`, the preflight then
resolves it on that class or an ancestor and requires its data type to match the
reference's `representation`. The two standard instance identifier names,
`HLAobjectInstanceName` and `HLAobjectInstanceHandle`, are intentionally held
out of this attribute/type predicate: IEEE 1516.2 gives them special instance
semantics, and their implicit type treatment needs a separate reviewed rule.

The preflight also resolves every `directedInteraction` name against the final
interaction-class hierarchy, after all modules have merged. It therefore
accepts a directed interaction whose class is supplied by a later module and
rejects a genuinely missing interaction class before FDD materialization.

Available dimensions on object and interaction classes are likewise resolved
against the final top-level dimension table, following the 2025 OMT schema's
`dimensionRef` keyref. A later module may therefore supply the named
dimension. This validates FOM references only; it does not implement DDM
regions or any object/interaction runtime service.

Transportation names on attributes and interaction classes are similarly
resolved against the final top-level transportation table, following the 2025
OMT schema's `transportationRef` keyref. A provider can occur in a later
module. This is not an implementation of data transport behavior.

The same completed hierarchies reject an object-class attribute or
interaction-class parameter that duplicates a name declared by an ancestor.
This is a structural preflight rule only; Umbra still has no object or
interaction runtime service implementation.

The table-specific preflight rejects a supplied non-positive/non-finite update
rate and checks each supplied dimension `value` against its dimension
`upperBound`. Integer and half-open range forms must describe a nonnegative
subrange of `[0, upperBound)`; `Excluded` remains valid. Missing values remain
allowed for incomplete DIF modules. A missing `upperBound` is also valid for
dimensions such as the standard MIM `HLAfederate`; the composed catalog maps
that form to the full unsigned-long coordinate domain rather than a zero-sized
domain. A supplied upper bound must be positive. These checks are traced by
`compliance/fom-table-constraints-requirements-contract.json` and
`compliance/fom-dimension-default-value-requirements-contract.json`; the
`arrayData/cardinality` field is also checked when supplied: nonnegative scalar,
`Dynamic`, and nonnegative `[lower..upper]` components may be combined in a
comma-separated list, while malformed or reversed ranges are rejected. The
cardinality check is traced by
`compliance/fom-array-cardinality-requirements-contract.json`. Missing
cardinality remains representable for incomplete DIF modules. For a supplied
one-dimensional predefined encoding, `HLAfixedArray` is paired with fixed
cardinality and `HLAvariableArray` with varying or `Dynamic` cardinality;
multidimensional interpretation and provider-defined encodings remain outside
this bounded rule. The array Element Type category check is separately traced
by `compliance/fom-array-element-data-type-requirements-contract.json`; the
remaining table work includes non-negative lookahead inference and other Annex
C rules.

The standard MIM plus Restaurant base FOM produces such an FDD; an identical
duplicate base is accepted as well. The supplied Restaurant extension is
structurally compatible with that base, but its additional named directed
interaction makes the result contain two `directedInteraction` elements for
one object class. The official 2025 FDD XSD permits only one. Umbra therefore
rejects that exact set at the FDD-schema boundary rather than silently omitting
the added interaction or accepting an unvalidated private representation. This
is an upstream resource/standard-interpretation question that must be resolved
before that example can enter a federation execution. Annex C's set-merge rule
and the preflight's name-resolution check still apply; neither authorizes an
Umbra-specific FDD shape that contradicts the vendored schema.

This remains a deliberately limited composition and materialization boundary.
The development profile maps its determined preparation outcomes to official
Create/Join exceptions, but it does not resolve every type and object reference
or derive every switch default. Annex C.8 warnings are retained in the private
`FomCompositionResult`; they are not a public warning API, JUnit result, or
conformance claim. The default packaged profile continues to route all
federation-management methods through the fallback.

On successful preflight, the backend does retain an immutable `FomCatalog`
projection of the supported model shape: qualified object and interaction
classes, their declared attributes and parameters, dimensions, data-type
categories, and the declared logical-time data types. It gives subsequent
runtime layers a deterministic query boundary without pretending that the
projection is the required complete FDD.

`ReferenceLogicalTimeSelector` applies IEEE 1516.1's empty-name default using
the official `HLAlogicalTimeFactoryFactory`: `HLAfloat64Time` when no name is
supplied, or the requested `HLAinteger64Time`/`HLAfloat64Time` reference
factory. When the composed FDD documents a non-`NA` logical-time or interval
data type, it must match the selected standard type. An FDD that documents no
time use does not constrain the selected factory. No guessed mapping exists for
custom fedtime libraries; they stop at the `CouldNotCreateLogicalTimeFactory`
boundary until a provider contract can establish their encoding and FDD
relationship.

The current test matrix validates every supplied restaurant module and the
standard MIM under DIF; rejects missing source, malformed XML, a wrong
namespace, and a DTD-bearing fixture; asserts that strict OMT is not silently
substituted for standalone modules; runs the positive/negative composition
cases above; and exercises two independent two-federate
Create/Join/Resign/Destroy scenarios solely through official C++ methods and
standard exceptions in the development profile. The Requirements-Lab reference
contract `compliance/fom-composition-requirements-contract.json` pins the
relevant Annex C IDs to those Catch2 selectors without producing conformance
evidence.

## Optional external SISO corpus boundary

Umbra does not vendor third-party SISO XML merely to make a test convenient.
Instead, a developer may configure a reviewed local corpus root with
`UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY`. The
`compliance/external-siso-fom-corpus.json` manifest pins seven representative
files—the complete Space ordered family, an RPR 3.0 foundation module, and a
Link 16 extension—by path, digest, XML namespace, and schema location. The
optional CTest integrity check detects corpus drift before Catch2 reads it.

Those fixtures all declare the IEEE 1516-2010 namespace and
`IEEE1516-DIF-2010.xsd`. This includes the RPR publication whose title is dated
2025. The optional Catch2 lane therefore proves a narrow but important
property: Umbra's 2025 DIF policy rejects each as `invalid_model`. It is not a
2010 validation result, a claim that their ordered composition succeeds, or a
claim of interoperability. Umbra's current scope is IEEE 1516.1/1516.2-2025
only: this repository has no 2010 schema, adapter, conversion, or positive
compatibility profile. The rejection lane is retained solely to prevent an
accidental cross-edition acceptance.

## Optional external 2025 corpus boundary

The optional 2025 lane keeps realistic external model pressure without turning
external XML into an Umbra dependency. A developer explicitly supplies its
local root through `UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY`; the
`compliance/external-2025-fom-corpus.json` manifest pins four 2025-native
prototype modules by relative path, SHA-256, namespace, and schema location.
The CTest integrity check runs before Catch2 reads the files.

The currently pinned snapshot is deliberately a schema-positive,
semantic-negative guard. Each module validates under the official 2025 DIF
schema, but the MIM-first set uses raw basic-data representations in
object-attribute and interaction-parameter data-type columns. Under the
explicit 1516.2 table rules enforced above, the set must fail private
composition deterministically and the embedded `createFederationExecution`
path must map that outcome to `InconsistentFOM`. This keeps the corpus useful
as regression pressure while refusing to call XSD acceptance a complete 2025
model-validation result.

The official 2025 MIM and Restaurant examples remain Umbra's packaged positive
baseline; the external XML is not copied into this repository or released with
the SDK. This expected-rejection lane establishes no object, interaction,
declaration, callback, time-management, transport, interoperability, or
conformance behavior for the external model.

Remaining required tests are:

1. unreadable-source diagnostics and a strict-OMT positive complete-model
   vector once the materializer can represent one without private shortcuts;
2. remaining Annex C merge, data-representation, special reference-data
   instance-identifier handling, other table-specific resolution, and
   switch-default rules;
3. callback/event behavior, object and ownership effects, and federation-wide
   time-management coordination beyond the initial per-federate advance state; and
4. packaged dependency handling, cross-process transport, and reproducible
   JUnit evidence for a reviewable service slice.

The development-profile contract is source/test traceability only. It does not
create Requirements-Lab catalog entries; raw JUnit evidence still requires
protected review before any binding is called verified.
