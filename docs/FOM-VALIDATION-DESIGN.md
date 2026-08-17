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
non-extendable variant record, duplicate variant discriminants, and the
prohibited `HLAother` alternative in an extendable variant record, plus the
Annex C.8 first-setting and warning behavior for repeated switches. It also
rejects an object-class attribute or interaction-class parameter that overloads
a name declared by an ancestor. The backend then emits only FDD-shaped
information: `modelIdentification` deliberately
contains only Annex C.1 `Composed_From` references, referenced notes receive
new deterministic `UmbraNoteN` labels before their `noteReferences` values are
merged, and matching service-usage entries combine their `isUsed` value with a
logical OR. The generated XML is validated against the official relaxed FDD
schema before it becomes an immutable `MaterializedFdd` artifact.

The preflight also resolves every direct `dataType` value in the merged
supported model against the composed data-type key. As in the vendored 2025
OMT XSD, that key includes basic-data representations as well as simple,
enumerated, reference, array, fixed-record, and variant-record declarations.
It runs only after all supplied modules have merged, so an extension can refer
to a type supplied by a later module. `NA` remains a permitted no-type marker.
The time table applies a narrower completed rule: a logical-time or interval
representation must name a simple, enumerated, array, fixed-record, or
variant-record data type, or `NA`. It deliberately does not yet prove that a
lookahead representation is non-negative.
User-supplied and synchronization tags apply a related completed rule: their
data type may name a simple, enumerated, reference, array, fixed-record, or
variant-record data type, or `NA`; basic-data names are not permitted there.
This is not yet the full OMT reference checker: basic-data `representation`
values, special reference-data instance-identifier type handling, and the
other table-specific referential constraints remain separately tracked work.
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

The optional 2025 lane adds realistic positive model pressure without turning
external XML into an Umbra dependency. A developer explicitly supplies its
local root through `UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY`; the
`compliance/external-2025-fom-corpus.json` manifest then pins four 2025-native
modules by relative path, SHA-256, namespace, and schema location. The CTest
integrity check runs before Catch2 reads the files.

With that option enabled, Catch2 proves three bounded properties for the
ordered standard MIM plus the four external modules:

1. each individual module is valid under the official 2025 DIF schema;
2. the MIM-first set materializes a valid private FDD/catalog, and repeated
   composition produces the same private FDD bytes; and
3. in the non-installable embedded development profile, the exact module list
   reaches official `createFederationExecution` and two official
   `joinFederationExecution` calls.

This is development-profile evidence for FOM preparation and the existing
federation lifecycle slice only. It does not establish object, interaction,
declaration, callback, time-management, transport, interoperability, or
conformance behavior for the external model. The official 2025 MIM and
Restaurant examples remain Umbra's packaged baseline; the external XML is not
copied into this repository or released with the SDK.

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
