# IEEE 1516.1-2025 FOM Module Admission and Composition: Flow Guide

An HLA Federation Object Model supplies shared vocabulary for model-dependent
services. With modular object models, validity of each input, validity of the
composed set, and successful materialization for a particular RTI are distinct
questions; passing one does not establish the next. This guide explains those
model-admission distinctions first, then uses ordered-module Create/Join paths
in Umbra's selected IEEE 1516.1-2025 embedded profile as bounded examples—not
as the definition of HLA behavior or a conformance claim.

## Scope and authority

- **Edition boundary:** RTI services are from IEEE 1516.1-2025; model-module
  syntax and the DIF/FDD/OMT schemas are from the [official IEEE
  1516.2-2025 OMT specification](https://standards.ieee.org/ieee/1516.2/6689/).
  The [IEEE 1516.1-2025 Federate Interface
  Specification](https://standards.ieee.org/ieee/1516.1/6688/) is the authority
  for the public Create/Join services.
- **Implementation boundary:** this is the 2025 strict-source, embedded
  development profile. Process-endpoint evidence is not treated as proof of
  embedded-path equivalence.
- **Documentation boundary:** this guide explains current source and selected
  tests; it is not a new requirement mapping or conformance claim.

IEEE describes [1516.2-2025](https://standards.ieee.org/ieee/1516.2/6689/)
as specifying the format and syntax, but not the content, of HLA object models.
Schema validation therefore answers a structural question; it does not decide
whether a model's chosen semantics fit its federation's design.

The pinned schema files make the three structural roles precise:

| 1516.2-2025 schema | Role stated in the schema | How this guide uses it |
| --- | --- | --- |
| [DIF](../../third_party/ieee1516.2-2025/resources/schemas/IEEE1516-DIF-2025.xsd) | Data Interchange Format for exchanging possibly incomplete OMT files between tools. | Validate each supplied MIM/FOM/SOM module in this 2025 path; do not require cross-module references to resolve yet. |
| [FDD](../../third_party/ieee1516.2-2025/resources/schemas/IEEE1516-FDD-2025.xsd) | A relaxed OMT schema defining the minimum OMT information required by any RTI. | Validate the composed, RTI-facing artifact after merge and materialization. |
| [OMT](../../third_party/ieee1516.2-2025/resources/schemas/IEEE1516-OMT-2025.xsd) | Stricter than DIF and used to validate conformance with the OMT specification; the schema itself says that passing it does not guarantee full conformance. | Keep it separate from per-module DIF validation. It is not a shortcut for composition or evidence that all declared model behavior is implemented. |

These are different validation artifacts, not three interchangeable ways to
ask whether one XML file is “valid.” The repository's pinned upstream-file
inventory and attribution are recorded in the
[1516.2-2025 manifest](../../third_party/ieee1516.2-2025/manifest.json).

At the 1516.1-2025 API boundary, Create accepts one module designator or a
vector of module designators, while Join accepts an optional list of additional
module designators. Those signatures establish how model inputs enter the
services; they do not collapse input validation, complete-set composition,
and RTI-specific representation into one notion of validity. See the pinned
[Create overloads](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L128)
and [Join overload](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L212).

The existing [FOM validation design](../fom/FOM-VALIDATION-DESIGN.md#required-runtime-boundary)
documents the larger validator policy and its evidence. This guide supplies
the learner-facing sequence and calls out the create/join state boundary.

## Three different meanings of “valid”

Do not collapse these stages:

| Stage | Question answered | What it does not establish |
| --- | --- | --- |
| Per-module DIF validation | Is this supplied 2025 MIM/FOM/SOM document well-formed for its module role and valid against the selected 2025 DIF schema? | That its declarations are compatible with other modules or can form a valid FDD. |
| Complete-set composition/preflight | Can the ordered, same-edition module set be merged under the implemented Annex C rules, and do completed-model references resolve? | That the resulting FDD serialization is acceptable to the official FDD schema. |
| FDD materialization and validation | Can Umbra represent the merged 2025 model in its RTI-facing FDD artifact, and does that artifact validate against the selected 2025 FDD schema? | That every behavior described by every FOM is implemented by the RTI. |

Individual modules are intentionally allowed to leave some cross-module
key/keyref references unresolved. Complete-set resolution happens only after
composition; validating one extension in isolation must not manufacture rows
that belong to another module.

Composition supplies shared model vocabulary; it does not itself create a
federate's publication or subscription declarations, register an object
instance, or make that instance known to another federate. Those are later
runtime service transitions with their own eligibility and observation
rules. For that next boundary, continue to the cross-cutting
[model-vocabulary/runtime-state map](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#a-federation-model-defines-vocabulary-services-create-live-state),
the 2025 [object registration and discovery flow](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#object-path-declarations-registration-and-discovery),
and [interaction publication/subscription and delivery](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#interaction-path-class-subscription-parameter-projection-callback).

Model admission also does not execute a federate's value encoder. Keep the
application-side C++ octet boundary separate: the 2025
[DataElement Encoding guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md#pick-the-representation-boundary-before-reasoning-about-bytes)
distinguishes encoded `DataElement` values from FOM/OMT model input and RTI
transport framing.

## Create: prepare the entire definition before reserving its name

In the embedded profile, creation is a prepare-then-commit transition. A
failed validation, composition, FDD, or time-selection step occurs before the
registry's create operation, so it must not reserve the federation name.

```mermaid
flowchart TB
  A[Create Federation Execution request] --> B{At least one FOM module supplied?}
  B -- No --> X[Reject; registry is unchanged]
  B -- Yes --> C[Select 2025 MIM and schema resources]
  C --> D[Validate and compose modules; materialize the FDD]
  D --> E{Requested logical-time implementation matches?}
  E -- No --> X
  E -- Yes --> F[Prepare immutable FederationDefinition]
  F --> G[Registry creates execution and reserves its name]
```

The model-preparation sub-flow is expanded in the composition diagram below.
This chart emphasizes the public Create transaction boundary: every rejection
occurs before the registry reserves the federation name.

If the standard MIM is omitted, the coordinator loads Umbra's selected 2025
standard MIM first. A supplied MIM designator is validated as the MIM input;
it is not an extra FOM. At least one FOM designator is required by this
development profile's Create path. The coordinator returns a prepared
definition without mutating the registry; the ambassador calls registry
`create` only after preparation succeeds.

The public designator and the canonical local file path have different jobs:
the designator is retained as the supplied module identity, while the
canonical path is used to open the source safely. The composer reloads and
revalidates its module descriptors at the composition boundary rather than
treating an earlier validation result as a permanent parsed document.

## Composition: merge first, resolve the completed model second

Every module must first pass its selected DIF validation. That is necessary,
but not sufficient: composition applies table-specific duplicate rules and
then resolves references against the **whole** merged model.

The diagram's MIM-first input describes the current Umbra preparation
sequence. It is not a normative claim about required module ordering; use the
applicable 1516.1/1516.2 composition clauses for that rule.

```mermaid
flowchart TB
  A[Ordered MIM-first 2025 module set] --> B{Every module passes its role-specific 2025 DIF validation?}
  B -- No --> E[Reject preparation; registry is unchanged]
  B -- Yes --> D{Umbra gate: common edition and source profile?}
  D -- No --> E
  D -- Yes --> M[Ordered Annex C merge]
  M --> F[Resolve names and references after merge]
  F --> G{Completed-model predicates pass?}
  G -- No --> E
  G -- Yes --> H[Build composed catalog]
  H --> I[Materialize RTI-facing 2025 FDD]
  I --> J{Official 2025 FDD schema accepts artifact?}
  J -- No --> E
  J -- Yes --> L[Return catalog, module set, FDD, and warnings]
```

This is an Umbra compatibility checkpoint: the composer compares each module
with the first module's edition and source-compatibility profile before it
revalidates and merges that module. It is not an HLA rule that makes the 2010
and 2025 behavior streams interchangeable.

Consequences that are easy to miss:

- A reference in an earlier module may resolve to a declaration in a later
  module. The composer intentionally delays completed-model resolution until
  after the full MIM-first set has merged.
- “Merge” is not “concatenate” and is not universally “last definition wins.”
  Annex C defines different duplicate behavior by table. Equivalent class
  definitions may merge; incompatible definitions fail. The implemented
  Annex C.8 switch path retains the first module's setting and reports a
  warning for a conflicting later setting, so preserve module order.
- A DIF-valid extension can still make the merged set unrepresentable in the
  official FDD schema. FDD generation/validation is its own failure boundary,
  not a guarantee implied by per-module validation.
- Fixed module order is repeatable in the tested materializer. That is not a
  promise that arbitrary reorderings produce identical FDD bytes or equivalent
  first-setting outcomes.

For the 2025 schemas, use DIF on individual modules, FDD on Umbra's composed
artifact, and OMT only for the separate complete-model validation policy.
Never make a single module appear complete by inventing cross-module rows.

The composed model's object- and interaction-class parent chains feed
different runtime questions. The object-information guide's [cross-path class
ancestry map](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#class-ancestry-has-different-jobs-in-different-paths)
compares those selected 2025 uses; class hierarchy does not imply one generic
nearest-ancestor callback rule.

## Join with additional FOM modules: replace definition and add member together

An ordinary Join without extra FOM modules uses the existing federation
definition. When additional modules are supplied, Umbra prepares a replacement
definition before committing the new membership. It revalidates the existing
modules as well as the added ones, composes the entire replacement set, and
keeps the federation's established logical-time implementation.

```mermaid
flowchart TD
  A[Join request with additional 2025 FOM modules] --> B[Read current federation definition]
  B --> C[Revalidate existing modules and validate additions]
  C --> D[Compose complete replacement model and FDD]
  D --> E{Valid and same established time implementation?}
  E -- No --> F[Reject Join; keep current definition and memberships]
  E -- Yes --> G[Prepare replacement handle directories and member time state]
  G --> H{Registry commit checks succeed?}
  H -- No --> F
  H -- Yes --> I[Atomically commit replacement definition plus new membership]
  I --> J[Initialize joined-federate report and lifecycle state]
  J --> K{Post-commit setup succeeds?}
  K -- Yes --> L[Return the new FederateHandle]
  K -- No --> M[Resign the new member and report Join failure]
  M --> N[Observed rollback does not restore the replacement definition]
```

The registry commit is atomic with respect to its shared state: after preparing
the replacement directories, it commits the replacement definition and new
membership together. Before that commit, preparation and registry rejection
leave the prior shared definition in place. This is not an end-to-end
transaction for every later step in the public Join call, however. In the
observed embedded path, the ambassador next creates joined-federate
service-report state and advances its lifecycle. If that setup fails, its
rollback resigns the new member; it does not restore the prior federation
definition or handle directories. Thus an additional-module Join can report an
error after the complete replacement catalog has already been installed.
The focused writer-creation failure test covers an ordinary Join and proves
that the failed applicant's member name can be reused; it does not exercise this
post-commit failure with additional modules or assert catalog state. Treat this
as a source-observed gap, not a portable HLA rule or a tested guarantee.

### Caller-visible preparation failures are phase-specific

The 2025 embedded adapter translates preparation results into public exception
types after module validation and composition. This mapping helps interpret
the generic rejection edges above; it is an observed implementation boundary,
not a claim that every RTI uses the same failure translation.

| Preparation outcome | Observed embedded 2025 exception |
| --- | --- |
| FOM source cannot be opened | `CouldNotOpenFOM` |
| FOM source is unreadable or cannot be parsed | `ErrorReadingFOM` |
| A supplied FOM fails the selected DIF validation | `InvalidFOM` |
| Modules conflict, the composed FDD is rejected, or the selected time representation is inconsistent | `InconsistentFOM` |
| Requested logical-time factory is unavailable | `CouldNotCreateLogicalTimeFactory` |
| Preparation fails for an unclassified backend reason | `RTIinternalError` |
| Selected/supplied MIM cannot be opened, read, or validated during Create | `CouldNotOpenMIM`, `ErrorReadingMIM`, or `InvalidMIM`; explicitly supplying the standard-MIM designator maps to `DesignatorIsHLAstandardMIM` |

These exception declarations are overload-specific; do not assume the public
Create/Join overloads expose identical failures. The following differences are
source/API observations for the embedded 2025 adapter, not normative HLA
outcomes:

| Pinned public overload | Inspected embedded preparation path | Difference to track |
| --- | --- | --- |
| Create with one FOM module | Delegates to vector Create, which loads the selected standard MIM. | The API lists MIM failures and `DesignatorIsHLAstandardMIM`; this embedded call has no supplied MIM designator, so the latter preparation status is not produced on this path. |
| Create with a vector of FOM modules | Also loads and validates the selected standard MIM, then uses the shared preparation translator. | The API list omits `CouldNotOpenMIM`, `ErrorReadingMIM`, and `InvalidMIM`, although the translator can emit them if loading that MIM fails. |
| Create with an explicit MIM | Validates the supplied MIM and uses the same logical-time selection path. | The API list includes MIM errors but omits `CouldNotCreateLogicalTimeFactory`, which the preparation translator can emit when the requested factory is unavailable. |
| Either Join overload with additional FOM modules | Revalidates all retained modules, including the existing MIM, then validates additions through the shared translator. | The API list includes FOM/time exceptions but no MIM-specific exceptions. An unreadable or invalid retained MIM can be classified and translated as an MIM failure. An ordinary Join with no additions does not enter this revalidation path. |

The pinned API declarations are [single-module Create](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L128),
[vector Create](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L145),
[Create with MIM](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L165),
and [both Join overloads](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L212).
The headers' provenance and byte identity with the canonical IEEE API archive
are recorded in the [pinned API manifest](../../third_party/ieee1516.1-2025/manifest.json);
the full prose standard is not vendored here.
The selected adapter paths are [Create delegation and preparation](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L113),
[Create with MIM](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L196),
[additional-module Join](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L474),
[retained-module revalidation](../../cpp/src/internal/federation/federation_management_coordinator.cpp#L231),
and [status translation](../../cpp/src/internal/runtime/umbra_rti_ambassador_service_failure_translation.cpp#L1079).
The [focused coordinator case](../../cpp/tests/libxml2_fom_composer_catch2.cpp#L2265)
checks default-MIM retention, unavailable time selection, and rejection of the
standard-MIM designator, but no focused public-adapter case asserts these
overload differences or changes the retained MIM between Create and Join.
Keep these as implementation/API gaps for follow-up, not as HLA-wide behavior.

A successful registry commit reconciles existing class, attribute,
interaction, parameter, dimension, and transportation handle directories
before the replacement definition and membership become visible together.
The lookup consequence is separately tested: in the embedded 2025 profile, an
already-joined member resolves object and interaction classes contributed by
the successful Join, and each focused test checks that its earlier typed class
handle still compares equal in that same execution. The configured-process
additional-FOM test instead checks lookup by the joining client; it does not
test whether an already-connected process member sees the added class. These
are bounded Umbra observations, not cross-execution numeric-handle stability
or embedded/process parity. See the [name-and-handle guide's additional-module
Join example](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md#tested-example-additional-module-join-and-later-model-lookups),
including its API and evidence limits. The focused cases are the [object-class
lookup test](../../cpp/tests/object_class_lookup_catch2.cpp#L99),
[interaction-class lookup test](../../cpp/tests/interaction_class_lookup_catch2.cpp#L103),
and [configured-process additional-FOM test](../../cpp/tests/process_federation_additional_fom_catch2.cpp#L219).

### Model replacement is not itself an observer-discovery event

The additional-module Join commit reconciles the replacement model's handle
directories and swaps them with the federation definition while adding the
new member ([registry commit](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L216)).
That commit path does not itself call the ordinary object-instance discovery
planner. In the inspected 2025 source, receiver-wide discovery replanning is
explicitly requested after object-class subscription changes in both the
embedded ambassador and configured process service
([embedded subscription](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_subscription.cpp#L133),
[process subscription](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L234));
the planner separately skips instances that receiver already knows
([discovery planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L83)).
So the source does not support drawing an immediate discovery callback from
the additional-FOM Join commit itself.

Keep that claim narrow: it does not mean a module-adding Join has no
observer-visible callback of any kind. The 2025 MIM marks the federation MOM
attribute `HLAcurrentFDD` conditional on successful Create/Join changes, and
one focused embedded test observes its updated value through
`reflectAttributeValues` after an additional-module Join ([MIM condition](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L743),
[test assertion](../../cpp/tests/federation_mom_current_fdd_catch2.cpp#L303)). This is a
reflection for the RTI-owned, execution-scoped Federation MOM object; it is not
discovery or a value callback for an arbitrary application object. The test is
embedded-profile evidence only. See the [Federation MOM guide's comparison of
the effective model and MOM projections](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#the-effective-model-mom-projections-and-module-reports-are-different-views).

This is a bounded Umbra call-path observation, not a portable HLA rule. The
focused embedded lookup cases verify that an already-joined member can resolve
the extended object and interaction catalogs while its earlier handles still
compare equal. The configured-process case verifies lookup by the joining
client. None observes callbacks to a pre-existing subscriber for an
already-registered object after the model replacement. The inspected evidence
therefore leaves open how any newly applicable class/member relationship is
observed by existing subscribers; it does not establish that such a callback
is forbidden or required. Keep that question separate from the tested catalog
and handle behavior, and see the [object/interaction information guide's
receiver-specific discovery model](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md).

```mermaid
flowchart TB
  accTitle: Additional-module Join changes model vocabulary, not automatically live application-object state
  accDescr: The selected 2025 sources show a model-and-membership commit, new catalog lookup, retained object knowledge and declarations, a separate Federation MOM reflection, and an unresolved normative callback question.
  Join["Additional-module Join succeeds"] --> Commit["Commit replacement catalog and handle directories<br/>with the new member"]
  Commit --> Catalog["Shared model vocabulary changes"]
  Catalog --> Lookup["Added classes or parameters can resolve;<br/>selected embedded tests preserve old handles"]
  Commit --> Runtime["Existing object knowledge, per-class subscriptions,<br/>and instance ownership remain separate state"]
  Runtime --> NoCallback["Commit path does not directly call ordinary<br/>object-discovery planning or synthesize Receive Interaction"]
  NoCallback --> Normative["Source observation only:<br/>normative Join-to-callback duty is unresolved"]
  Commit --> Mom["Separate embedded Federation MOM projection"]
  Mom --> Reflection["HLAcurrentFDD reflection observed after Join;<br/>not ordinary application-object discovery"]
  Runtime --> Later["Later declarations and sends are separate transitions<br/>with their own catalog and receiver-state gates"]
  Later --> Gap["No focused post-Join delivery test for added<br/>attribute or interaction cases"]
```

The lookup and MOM observations in this diagram are embedded-profile tests;
the configured-process test only verifies that its joining client can look up
an added object class. It does not establish process parity or callback
behavior after the catalog change.

#### Extension shapes in the inspected evidence

The successful additional-module lookup fixture contributes a new child class,
`HLAobjectRoot.UmbraReferenceFixtureClass`, under the existing root. The
embedded lookup case proves that an already-joined member can resolve that
class while its earlier Employee-class handle remains equal; it does not add
an attribute to an existing class or observe object callbacks
([fixture](../../cpp/tests/data/reference-data-class-provider-fom.xml#L9),
[test](../../cpp/tests/object_class_lookup_catch2.cpp#L99)).

The official Restaurant extension illustrates a broader shape: it adds the
`Organic` attribute under the existing `Food.Drink.Soda` class and adds
interaction parameters. But this exact module set is rejected during FDD
materialization by the focused atomicity case, so it is not evidence that those
member additions successfully commit in an additional-module Join
([extension](../../third_party/ieee1516.2-2025/resources/examples/RestaurantExtensionFOMmodule-2025.xml#L71),
[rejection case](../../cpp/tests/ieee1516_2025_embedded_fom_preparation_atomicity_catch2.cpp#L28)).
At the composer level, matching semantic children are recursively merged by
identity and unmatched children are appended, but the composed model still
has to pass later materialization and validation
([child identity](../../cpp/src/internal/fom/libxml2_fom_composer.cpp#L160),
[recursive merge](../../cpp/src/internal/fom/libxml2_fom_composer.cpp#L843)).
Structural merge capability alone therefore does not establish a successful
Join shape.

For a successful commit, the inspected registry path reconciles old and new
class/attribute handles, then installs the replacement definition; it does
not rewrite existing per-class subscription records or per-instance attribute
ownership records ([class handles](../../cpp/src/internal/handles/object_class_handle_directory.cpp#L10),
[attribute handles](../../cpp/src/internal/handles/attribute_handle_directory.cpp#L35),
[subscription state](../../cpp/src/internal/federation/federation_registry.hpp#L2323),
[instance ownership](../../cpp/src/internal/federation/federation_registry.hpp#L2419),
[Join commit](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L596)).
The discovery resolver consults the current catalog hierarchy together with
those explicit subscriptions and concrete instance-owned attributes
([resolver](../../cpp/src/internal/federation/federation_registry.cpp#L830)).
Thus a later declaration-triggered reevaluation uses the current model and
stored runtime state; adding a FOM definition alone does not create a
subscription, an instance-owned value, or an immediate discovery plan. The
observed call sites replan after subscription changes
([embedded](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_subscription.cpp#L133),
[process](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L234)).
This describes the inspected 2025 implementation only. There is no successful
focused Join test here for adding an attribute to an already-defined class,
and no test of how pre-existing observers see such a change.

For an already-registered object, keep three consequences separate:

| Additional-module shape or later event | Existing-object consequence in the inspected 2025 path | Evidence limit |
| --- | --- | --- |
| Join adds a child class below an existing class. | Existing members can resolve the new class, but an object registered under the old class keeps its registered-class handle; the current resolver starts from that registered class and walks toward its ancestors, not down into newly added children. | The successful embedded fixture tests catalog lookup and stability of an older class handle, not a live object's discovery or retyping ([test](../../cpp/tests/object_class_lookup_catch2.cpp#L99), [instance class field](../../cpp/src/internal/federation/federation_registry.hpp#L2414), [resolver](../../cpp/src/internal/federation/federation_registry.cpp#L830)). |
| A module adds an attribute to an existing class. | Replacing the catalog does not itself add the new attribute to an instance's ownership record or to a member's subscription declarations. | The cited Restaurant extension is rejected during FDD materialization, before any Join commit; there is no successful focused add-attribute Join case ([rejection case](../../cpp/tests/ieee1516_2025_embedded_fom_preparation_atomicity_catch2.cpp#L28), [Join replacement](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L596), [instance owner record](../../cpp/src/internal/federation/federation_registry.hpp#L2419), [subscription declaration state](../../cpp/src/internal/federation/federation_registry.hpp#L2323)). |
| A member subscribes after the model change, but already knows the object. | This is a separate declaration transition: discovery replanning skips the receiver's known instance, and the ordinary current-value reflection question remains open. | The subscription guide documents the source path and the missing isolated value-only test; do not infer an immediate Join callback or a normative no-reflection rule ([known-object subscription boundary](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md)). |

These are bounded source and test observations, not a substitute for the
licensed standard text. They separate catalog visibility, instance class and
ownership, receiver object knowledge, and value reflection; none alone implies
the next transition. The pinned 2025 API headers identify Join as §4.11 and
Discover/Reflect as §§6.9/6.11 ([Join API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L212),
[object callbacks](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L229));
their signatures and exception annotations do not include the full normative
transition text needed to decide whether a model-changing Join requires a
discovery or value-reflection callback. The complete 2025 standard text is not
in this checkout; IEEE's [standard entry](https://standards.ieee.org/ieee/1516.1/6688/)
lists subscription access.

#### Interaction catalog growth is not a Receive Interaction callback

The additional-module interaction lookup case adds
`HLAinteractionRoot.UmbraDirectedFixtureInteraction`; an already-joined member
can look up its class/name while its earlier `ServerAction.TakeOrder` handle
still compares equal. A separate parameter lookup case adds
`UmbraParameterFixtureBase` with an `Identifier` parameter and a child class;
the existing member resolves that inherited parameter through the child
after Join. Both cases exercise catalog/handle lookup only: neither declares
an interaction subscription nor sends an interaction
([class lookup](../../cpp/tests/interaction_class_lookup_catch2.cpp#L103),
[parameter lookup](../../cpp/tests/parameter_lookup_catch2.cpp#L119),
[parameter extension fixture](../../cpp/tests/data/parameter-handle-provider-fom.xml#L7)).
The class lookup fixture's name includes “Directed,” but that test does not
invoke Send Directed Interaction.

For contrast, the separate ordinary inherited-receipt test has the complete
base/middle/child hierarchy at Create. A subscriber first subscribes to the
base, the publisher sends the child, and the observed callback carries the
closest subscribed class; adding middle and then child subscriptions moves
that callback class closer and still yields one receipt
([receipt case](../../cpp/tests/inherited_interaction_receipt_catch2.cpp#L71)).
This verifies the selected 2025 hierarchy-routing scenario, not what happens
when the descendant was introduced by a later Join.

In the inspected registry, ordinary interaction subscriptions are stored per
federate and interaction-class handle. A later ordinary send resolves the sent
class in the current catalog, walks its parent chain, and selects the closest
eligible stored subscription
([declaration state](../../cpp/src/internal/federation/federation_registry.hpp#L2252),
[send-time recipient selection](../../cpp/src/internal/federation/federation_registry.cpp#L1421),
[send planner](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L16)).
Therefore the source path can consider a pre-existing ancestor subscription
for a later send of a newly added descendant; this is an implementation
inference, not an exercised post-Join outcome. Updating the FOM catalog does
not itself synthesize an ordinary Receive Interaction for the new class. No
focused test combines an existing ancestor subscription, additional-module
Join, and a subsequent send of the added descendant. The configured-process
additional-FOM case checks object-class lookup only, not interaction send or
receive ([process case](../../cpp/tests/process_federation_additional_fom_catch2.cpp#L219)).
Keep this separate from [object-instance discovery](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#object-path-declarations-registration-and-discovery)
and from [directed-interaction routing](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md).

The process-profile source narrows that gap but does not close it. A
module-bearing process Join is accepted only when the service has an
`additionalFomPreparation` callback; otherwise `handleJoin` rejects it. The
configured-process test and process service probe install a
`FederationManagementCoordinator`-backed callback. The service serializes
preparation and registry commit, then passes the prepared replacement into
the registry Join. The registry copies and reconciles the existing interaction
class directory against the replacement catalog, preserving prior named
handles, and swaps the definition and directories with the new membership.
The per-federate interaction-declaration map is separate and is not rewritten
by that commit. A later process send calls the shared receive-order planner;
its recipient resolver uses the current catalog and walks the sent class's
parent chain against stored subscriptions. This is a 2025 process source
inference that a pre-existing ancestor subscription can be considered for a
subsequent send of an added descendant—not a tested delivery result.

The configured-process fixture's added module is object-only
([module](../../cpp/tests/data/reference-data-class-provider-fom.xml#L5)); its
public assertions resolve that new object class by handle and name. No focused
process case adds an interaction class or parameter, preserves a pre-existing
ancestor subscription through such an addition, and sends to a descendant.
The process source path is therefore not process/embedded parity evidence.

Implementation evidence: [optional process preparation hook](../../cpp/src/internal/federation/process_federation_service.hpp#L31),
[serialized process Join preparation](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L257),
[atomic definition/directory replacement with membership](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L298),
[existing interaction-handle reconciliation](../../cpp/src/internal/handles/interaction_class_handle_directory.cpp#L10),
[per-federate declarations](../../cpp/src/internal/federation/federation_registry.hpp#L2250),
[later process send planner](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389),
and [current-catalog parent resolution](../../cpp/src/internal/federation/federation_registry.cpp#L1421).

For how FOM dimension declarations become class-available dimensions and then
application-chosen region scope, see the DDM guide's
[FOM-to-region bridge](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#from-fom-dimensions-to-runtime-region-scope).
For one bounded example where dimensions along an object-class parent chain
are used by a child-class regional registration and subscription, see the
DDM guide's [inherited-dimensions case](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#inherited-dimensions-in-a-child-class-regional-path).
That is source/test evidence, not a general normative inheritance ruling.
The corresponding interaction-class path is a separate source-only case:
available dimensions, exact-class regional declaration storage, and the
send-time parent walk are distinct steps, with no reviewed cross-class
regional test. See the DDM guide's [interaction ancestry map](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#interaction-class-ancestry-available-context-and-declaration-lookup).

The effective model described here is separate from the RTI-owned Federation
MOM projection. For the distinction between the federation-wide conditional
module list and `HLAcurrentFDD`, the same-named per-member Join-input list, and
the separate module-content request/report interactions, see the [Federation
MOM guide](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#the-effective-model-mom-projections-and-module-reports-are-different-views).

## Keep the 2010 model lane separate

This flow is **not** the IEEE 1516.2-2010 compatibility path and is not a
2010 RTI implementation guide. The current code rejects a mixed 2010/2025
module set. The opt-in 2010 model-compatibility milestone selects separate
2010 resources and is catalog-only; it does not manufacture a 2025 FDD or
claim a 2010 RTI API/conformance profile. See the edition-specific boundary
and limitations in the [FOM validation design](../fom/FOM-VALIDATION-DESIGN.md#L595).

Keep the model-edition choice separate from the RTI interface edition. In this
compatibility slice, `fomEdition=2010` selects a 2010 model catalog under the
2025 API; it does not switch the ambassador to IEEE 1516.1-2010 behavior.

```mermaid
flowchart TB
  A["RTI interface: IEEE 1516.1-2025"] --> B{"Selected FOM model edition"}
  B -->|"Default 2025"| C["2025 MIM/FOM and 2025 schemas"]
  C --> D["2025 composed catalog and validated FDD"]
  B -->|"Explicit fomEdition=2010"| E["2010 MIM/FOM and separate 2010 resources"]
  E --> F["2010 catalog only"]
  F --> G["Bounded 2025 API path: lookup, publication, and registration"]
  F -.-> H["Not produced in this slice: 2025 FDD and 2025 MOM projection"]
```

The 2010 branch is deliberately narrow: the optional focused lane composes a
2010 catalog, while the selected embedded case uses the 2025 API for class and
attribute lookup, publication, and object registration without claiming
simulation. Mixed-edition composition is rejected. These are Umbra profile
boundaries, not equivalence between the 2010 and 2025 RTI streams; consult the
separate [2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md) for
that implementation's own evidence and limits. See the [2010 catalog
composition case](../../cpp/tests/external_2010_fom_catch2.cpp#L109), [mixed
edition rejection case](../../cpp/tests/external_2010_fom_catch2.cpp#L155),
and [2025-API registration-only case](../../cpp/tests/external_2010_fom_catch2.cpp#L208).

## Implementation and focused evidence

| Boundary | Implementation | Focused evidence |
| --- | --- | --- |
| Per-module 2025 DIF validation | [LibXml2 module validator](../../cpp/src/internal/fom/libxml2_fom_validator.cpp#L7), [Create preparation](../../cpp/src/internal/federation/federation_management_coordinator.cpp#L151) | [valid official modules, missing/malformed/wrong-namespace/DTD cases](../../cpp/tests/libxml2_fom_validator_catch2.cpp#L41), [stricter OMT schema is not the per-module policy](../../cpp/tests/libxml2_fom_validator_catch2.cpp#L147) |
| Ordered merge, edition/profile gate, and complete-model reference resolution | [Composer compatibility check before revalidation/merge](../../cpp/src/internal/fom/libxml2_fom_composer.cpp#L2137) | [cross-module data-type and class references](../../cpp/tests/fom_composer_reference_resolution_catch2.cpp#L11), [mixed-edition rejection](../../cpp/tests/external_2010_fom_catch2.cpp#L155) |
| FDD serialization/schema boundary | [FDD materializer](../../cpp/src/internal/fom/libxml2_fom_composer.cpp#L1430) | [MIM plus supplied base FOM produces catalog and validated FDD](../../cpp/tests/libxml2_fom_composer_catch2.cpp#L345), [DIF-valid extension rejected at FDD boundary](../../cpp/tests/ieee1516_2025_embedded_fom_preparation_atomicity_catch2.cpp#L29) |
| Duplicate behavior and fixed-order repeatability | [Annex C merge implementation](../../cpp/src/internal/fom/libxml2_fom_composer.cpp#L843) | [first switch retained with warning](../../cpp/tests/libxml2_fom_composer_catch2.cpp#L885), [fixed-input repeatability](../../cpp/tests/libxml2_fom_composer_catch2.cpp#L837), [equivalent duplicate vs. conflict](../../cpp/tests/libxml2_fom_composer_catch2.cpp#L2131) |
| Create failure before federation-name reservation | [Prepare, then registry create](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L201) | [failed Create leaves name available; valid Create then succeeds](../../cpp/tests/ieee1516_2025_embedded_fom_preparation_atomicity_catch2.cpp#L4) |
| Additional-module Join commit boundary | [Prepare replacement and commit definition with membership](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L508), [atomic registry transition](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L216) | [failed addition leaves base federation joinable](../../cpp/tests/ieee1516_2025_embedded_fom_preparation_atomicity_catch2.cpp#L29), [registry replacement-definition contract](../../cpp/tests/federation_registry_foundation_catch2.cpp#L123) |
| Configured-process additional-module interaction boundary | [optional preparation and Join handler](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L257), [atomic catalog/directory replacement](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L298), [existing interaction-handle reconciliation](../../cpp/src/internal/handles/interaction_class_handle_directory.cpp#L10), [later shared send planner and current-catalog resolver](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389) | [configured-process additional-FOM test](../../cpp/tests/process_federation_additional_fom_catch2.cpp#L219) uses an object-only module and checks object-class lookup; no focused interaction subscription/send/receive case after Join was identified |
| Post-commit Join setup rollback | [Report/lifecycle setup and membership-only rollback](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L570), [registry swaps the replacement definition before returning](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L596) | [ordinary-Join writer-creation failure and member-name reuse](../../cpp/tests/service_report_writer_failure_catch2.cpp#L92); additional-module catalog state after this failure is not tested |

The linked tests prove only their named cases. They do not establish exhaustive
coverage of every Annex C table, every schema-valid FOM, process/embedded
equivalence, or general IEEE conformance.

## What this guide does not claim

- It does not describe the 2010 catalog compatibility path as a 2025 pipeline
  or infer a 2010 RTI implementation.
- It does not treat validation of every individual module as proof that the
  composite model is valid or representable as an FDD.
- It does not claim the current composer implements every 2025 OMT rule or
  that the resulting runtime implements every declared model feature.
- It does not expand or map the HLA Requirements Lab.
