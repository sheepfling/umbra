# Requirements and testing

Umbra uses the adjacent HLA Requirements Lab as the source of requirements,
state machines, native API declarations, and compliance verification. It does
not vendor that project or import its Python internals at runtime. The portable
JSON CorpusBundle is the boundary.

Observed corpus/export ambiguities and proposed Lab refinements are recorded
in [Requirements Lab observations](REQUIREMENTS-LAB-OBSERVATIONS.md). The log
keeps local mitigations separate from requests to change the Lab itself.

Useful scenarios from the adjacent legacy Python RTI are curated separately in
the [legacy Python RTI scenario backlog](LEGACY-PYTHON-RTI-TEST-BACKLOG.md) and
[test-resource register](LEGACY-PYTHON-RTI-TEST-RESOURCES.md). The
[FOM stress-corpus backlog](FOM-STRESS-CORPUS-BACKLOG.md) tracks richer
model-family candidates independently. They are advisory stress inputs, never
substitute requirements or portable conformance evidence.

## Current baseline

The unmodified IEEE 1516.1-2025 C++ headers live under
third_party/ieee1516.1-2025/include. Their provenance and required IEEE
attribution are recorded beside them. The canonical API source is the August
2025 archive used by the Requirements Lab; its C++ header tree is
byte-identical to the imported February distribution.
The binding defines every exception type declared by the official 2025
`RTI/Exception.h`; `umbra.ieee1516_2025.exception_binding` compares the
official declaration list with those definitions so the standards baseline
cannot regress into an unresolved official exception symbol.

The unmodified IEEE 1516.2-2025 OMT schemas, standard MIM, and supplied
restaurant examples live under third_party/ieee1516.2-2025/resources. Their
separate provenance and resource digest manifest are checked by
`umbra.ieee1516_2_2025.resources`. They are authoritative inputs for the
opt-in private FOM validator, not evidence that FOM services have been
implemented. To build its separate libxml2-backed test target:

~~~powershell
cmake -S . -B .build-fom -G "Visual Studio 17 2022" -A x64 `
  -DUMBRA_FETCH_CATCH2=ON `
  -DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON `
  -DUMBRA_FETCH_LIBXML2=ON
cmake --build .build-fom --config Debug
ctest --test-dir .build-fom -C Debug --output-on-failure
~~~

The backend validates individual documents, performs a private Annex C-guided
FOM/MIM compatibility preflight, and materializes a schema-validated FDD for
module sets the official FDD XSD can represent. Its source/test contract now
pins `Composed_From` metadata, referenced-note remapping, and service-usage OR
rules, alongside direct composed-model data-type-reference resolution, as
private traceability. That resolver follows the 2025 OMT schema's complete
data-type key, including declared basic-data names, and is not a claim of full
table-specific reference resolution. It also checks that a reference data
type's named object class exists in the completed hierarchy and, for ordinary
attributes, that the attribute and representation type match. The paired
`compliance/fom-data-representation-requirements-contract.json` and its Catch2
scenario now also reject unresolved simple/enumerated representations and
ordinary reference-data representations that resolve to a basic or reference
type. The two standard instance identifier names are handled by a dedicated
exception rule requiring `HLAunicodeString` or `HLAobjectInstanceHandle` as
appropriate. The official MIM/Restaurant `HLAboolean` representation remains
accepted under the reviewed RL-009 interpretation rather than being rejected
by a generic basic-only predicate. The paired
`compliance/attribute-parameter-data-type-requirements-contract.json` scenario
also rejects raw basic-data representations in object-attribute and
interaction-parameter data-type columns while retaining the standard
array-data `HLAtoken` case. The generic resolver retains basic-data names from
the schema's complete key so table-specific predicates report the applicable
category condition instead of an undeclared-name error. The separate
`compliance/fom-attribute-na-companion-requirements-contract.json` scenario
adds the bounded direct companion rule: for an attribute whose data type is
`NA`, supplied transportation/order values must be non-`NA`, and supplied
update type/update condition values must be `NA`. It accepts a partial DIF row
that a later compatible module completes before FDD materialization, but does
not manufacture omitted values. The 2025 DIF schema has no per-attribute
available-dimensions representation, so the source phrase is not inferred as a
class-wide restriction; RL-051 records that mapping limit. This is private
traceability only.
The paired
`compliance/fom-attribute-value-required-sharing-requirements-contract.json`
scenario adds the bounded `sharing=Neither` companion rule: a supplied
`valueRequired` field must be `false`; omitted fields remain representable for
a partial DIF attribute. It does not infer defaults or model declaration/runtime
state. RL-052 records the candidate's broad exported clause metadata. This is
private traceability only.
The paired
`compliance/fom-attribute-dynamic-update-condition-requirements-contract.json`
scenario adds the bounded Dynamic Update Condition rule: when an attribute
supplies update type Conditional or Periodic and also supplies Update Condition,
the latter must be nonempty, non-NA text. An omitted condition remains
representable in a partial DIF attribute row. This does not prove the periodic
rate grammar or initial-condition prose. The distinct Static/NA source/example
tension remains deferred under RL-046; it does not affect this independent
predicate. This is private traceability only.
The paired `compliance/fom-enumerated-representation-requirements-contract.json`
scenario additionally requires a supplied enumerated-data representation to
resolve to a basic-data declaration; it leaves omitted DIF fields and the
separate simple-data RL-009 interpretation unchanged. This is private
traceability only.
The paired `compliance/fom-array-element-data-type-requirements-contract.json`
scenario requires a supplied array Element Type to resolve to a simple,
enumerated, reference, fixed-record, array, or variant-record declaration;
raw basic-data names fail after complete-model resolution. Omitted Element Type
remains representable for an incomplete DIF row. This is private traceability
only.
The paired `compliance/fom-record-member-data-type-requirements-contract.json`
scenario applies that same bounded category rule to supplied fixed-record Field
Type and variant-record Alternative Type values. Raw basic-data names fail,
while omitted member types remain representable for incomplete DIF rows. The
Requirements Lab candidates retain broad `clause-4` metadata; RL-047 records
the verified source-provenance gap. This is private traceability only.
The paired `compliance/fom-variant-discriminant-data-type-requirements-contract.json`
scenario requires a supplied variant-record Discriminant Type to resolve
specifically to an enumerated-data declaration. It permits an omitted field for
an incomplete DIF row, but rejects `NA` because the applicable 2025
variant-record rule does not provide that marker path. This is private
traceability only.
The paired `compliance/fom-variant-discriminant-enumerator-requirements-contract.json`
scenario validates a supplied Discriminant Enumerator field's comma-separated
list and bracketed two-endpoint range grammar. `HLAother` is a standalone,
once-per-record marker; an omitted DIF field remains representable. It does not
by itself expand ranges or resolve discriminant membership; those semantics are
covered by the paired private scenarios below. This is private traceability
only.
The paired `compliance/fom-variant-discriminant-enumerator-membership-requirements-contract.json`
scenario then requires each supplied individual discriminant enumerator and
each range endpoint to be declared by the selected enumerated type after
composition. It retains an incomplete DIF enumeration that supplies no members.
The paired range-semantics scenario below uses that resolved table membership.
RL-049 records the precise candidate's broad exported clause provenance. This
is private traceability only.
The paired `compliance/fom-variant-discriminant-enumerator-range-semantics-requirements-contract.json`
scenario retains the enumerated table's declaration order, expands each
bracketed range over the inclusive span between its endpoints, and rejects a
member assigned through direct/range entries to more than one named alternative
after composition. `HLAother` is represented internally as the complement of
the explicit members. It retains an enumeration with no declared members as
incomplete DIF and does not invent a directional endpoint rule. RL-049 and
RL-050 record the Requirements Lab provenance limits. This is private
traceability only.
The paired `compliance/fom-dimension-input-data-type-requirements-contract.json`
scenario requires each supplied Dimension-table Input data type to resolve to
the same table-defined declaration families; raw basic-data names fail, while
the `NA` marker remains valid. The paired
`compliance/fom-dimension-input-data-description-requirements-contract.json`
scenario adds the bounded companion direction: an `inputDataTypes` sequence
with no named type requires non-`NA` description text. It accepts the official
empty-list representation and retains named-type rows with either `NA` or
amplifying descriptions. Neither predicate infers suitable-type selection or
the unambiguity of a supplied description. RL-048 records the broad exported
clause metadata. The additional
`compliance/fom-dimension-input-data-type-na-exclusivity-requirements-contract.json`
scenario rejects an explicit `NA` marker mixed with a named input type, without
adding a suitability, cardinality, deduplication, or description-content
judgment. This is private traceability only.
The paired `compliance/fom-root-hierarchy-requirements-contract.json` scenario
also rejects a nonstandard top-level object or interaction class after the
completed model merges, while leaving an omitted table representable for an
incomplete DIF module. It is private traceability only. The distinct
Static/NA direction remains deferred under RL-046 because the supplied 2025
Restaurant FOM uses Static with On change; Umbra does not reject the official
example through a homegrown interpretation. The separate bounded
Conditional/Periodic non-NA predicate remains active. Directed
interaction names are likewise resolved after the complete interaction
hierarchy is merged; this does not remove the separate official FDD-schema
cardinality limitation for the supplied extension example. Object and
interaction available-dimension references are resolved against the merged
dimension table, including a later provider module. Attribute and interaction
transportation names likewise resolve against the merged transportation table.
The paired `compliance/fom-table-constraints-requirements-contract.json` and
its Catch2 scenario reject a supplied zero/non-positive update rate while
preserving incomplete DIF rows; the 2025 FOM XSD enforces positive dimension
upper bounds before composition. The separate
`compliance/fom-dimension-default-value-requirements-contract.json` scenario
also checks integer and half-open dimension default ranges against
`[0, upperBound)` while permitting `Excluded`. Non-negative lookahead and
remaining table-specific checks are still open. The paired
`compliance/fom-array-cardinality-requirements-contract.json` scenario now
checks the 2025 scalar, comma-separated component, bounded-range, mixed, and
`Dynamic` array cardinality forms, while retaining omitted cardinality for
incomplete DIF modules. It also rejects a schema-valid one-dimensional
`HLAfixedArray`/varying-cardinality or `HLAvariableArray`/fixed-cardinality
mismatch; multidimensional and provider-defined encoding interpretation remains
open.
Logical-time and interval representations are limited to their permitted 2025
data-type families (or `NA`); non-negative lookahead semantics remain a later
rule. User-supplied and synchronization tag types have their own permitted
2025 families (including reference data but excluding basic data). The same
private preflight rejects an object attribute or interaction parameter that
  overloads a name declared by an ancestor; it does not implement object/
  attribute DDM behavior, data transport behavior, or an object/interaction
  runtime service.
The private reference-time contract also pins the IEEE default-selection and
FDD-documentation boundary. The default profile does not enable public
Create/Join service methods or create a conformance claim.

The XML/XSD policy is also executable: individual 2025 MIM/FOM/SOM modules
validate against DIF; synthesized federation artifacts validate against FDD;
and strict OMT is explicitly prevented from replacing DIF for standalone
modules. The schema test includes malformed XML, wrong namespace, and DTD
rejection. This distinguishes a complete verified 2025 schema-resource set
from the still-limited Annex C and runtime service scope; details and the
optional external-corpus policy are in [FOM validation design](FOM-VALIDATION-DESIGN.md).

The FOM composition contract also preserves the Annex C.8 switch rule: the
existing value wins for a repeated switch name, while an incompatible duplicate
produces private preflight warning data rather than invalidating the otherwise
valid composition. That warning is not a public service response or
Requirements-Lab evidence promotion.

The composed catalog also retains the 1516.2 advisory switch-table settings.
Omitted advisory entries use the standard Disabled default, while a joining
federate is seeded from the current composed FDD and may subsequently change
its own switch values. Existing members are not reset by an additional-FOM
join; the bounded behavior is traced by the paired advisory-switch contracts.

The complete remaining 2025 support-switch table is now parsed and retained
as well. Per-federate Convey Region Designator Sets, Automatic Resign Action,
Service Reporting, Exception Reporting, and Send Service Reports To File
values are seeded on join and mutated independently through their official C++
accessors. Federation-wide Delay Subscription Evaluation and Allow Relaxed DDM
values are captured at creation and shared by all members. The focused Catch2
case verifies explicit FDD values, per-federate isolation, static sharing, and
invalid-resign-action rejection. The paired
`compliance/support-switches-table-requirements-contract.json`,
`compliance/support-switches-service-requirements-contract.json`, and
`compliance/support-switches-api-contract.json` files pin the Lab records and
official declarations. A dedicated
`compliance/convey-region-designator-callback-requirements-contract.json`
now traces the recipient-side callback policy: regional reflection and
interaction callbacks omit optional sent-region metadata while the switch is
disabled and carry the sent update-region realization while enabled. The new
`compliance/delay-subscription-evaluation-requirements-contract.json` records
the bounded ordinary interaction and known-object attribute-update behavior of
the federation-wide Delay Subscription Evaluation switch: four Catch2 cases
distinguish enabled late subscription at receive-order and TSO-grant
eligibility from the disabled generation-time filter, and confirm callback-time
unsubscribe suppression in both modes. The attribute cases first establish
object discovery so they test declaration timing rather than discovery timing.
The separate Allow Relaxed DDM regression pair proves Umbra's intentional
implementation-specific policy for a regional interaction: exact
boundary-touching committed ranges are added only when the static FDD switch is
enabled, while a positive gap remains filtered and strict overlap is retained;
the object-attribute companion additionally covers discovery and reflection.
The existing support-switch contract continues to pin the exact official getter
requirement; the underlying §9.1.4/§9.1.8 delivery prose has no immutable Lab
candidate, so `docs/RELAXED-DDM-POLICY.md` and RL-030 retain direct-source
traceability without overstating that getter mapping.
It intentionally excludes regional interaction and explicit update-region
attribute forms, directed messages, HLA_IMMEDIATE/asynchronous, and
lifecycle-race matrices. This remains development-profile traceability only;
real connection-loss automatic resign, MOM service-report/report-file behavior,
default/conveyed-region use, directed regional callbacks, broader relaxed-DDM
matrices, broader delayed-subscription lifecycle matrices, JUnit promotion, protected
review, and conformance remain open. RL-018 records the Delay Subscription
source-title/clause metadata mismatch, while RL-024 records the Lab/XSD
automatic-resign default mismatch.

The embedded profile also enforces the exact MOM service-reporting exclusion:
a federate with Service Reporting enabled cannot subscribe—ordinarily or with
regions—to `HLAreportServiceInvocation`, and a federate with either exact
subscription cannot enable the switch. The focused Catch2 case proves the two
official exception types, passive-mode coverage, state preservation after a
failed enable, and removal-before-enable recovery. The bounded filesystem
foundation now creates one RTI-owned initial-record file per joined federate,
including when reporting starts disabled, preserves that file across switch
changes, and allocates a new one on rejoin. It does not emit service-report
interactions, append generated report records, publish
`HLAreportServiceFile`, or implement generic MOM handling; those remain
separate work.

The same successful production-profile Join now establishes an unpublished
RTI-owned joined-federate MOM snapshot behind the registry seam. It reserves a
common-namespace object identity, preserves all effective MIM attribute
metadata (including the inherited delete-privilege policy), captures a private
normalized `HLAfederate` point, and encodes the seven direct initial values
using official MIM types and the exact filesystem path. Switch changes preserve
that state and resignation removes it. The Catch2 assertion is deliberately an
internal inspection only: it does not register, discover, reflect, remove, or
serve requested values for a public MOM object, because RL-043 still lacks a
source-backed callback producer-designator rule.

The same support-switch contract now traces the bounded joined-federate
`HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches` control path. A non-empty
parameter subset updates only the sending member's nine predefined switch
values with strict standard encodings; the regression proves peer isolation,
invalid resign-action rejection, and the report-service interlock's atomic
state guard. It also proves that a FOM-added parameter and a compatible
extension subclass are received, while only inherited predefined values are
processed. The current local error is deliberately not represented as a normal
MOM failure-report interaction, and this does not add MOM objects, reports,
or report-file behavior. RL-032 records the Requirements Lab's missing
per-parameter Table 20 candidates and truncated at-least-one candidate.

The basic DDM/MOM prerequisite is now real rather than a generated binding
stub: all five official handle-normalization services (10.29--10.33) validate
the standard lifecycle/input boundaries and return stable in-execution point
coordinates for the standard service-group, federate, object-class,
interaction-class, and live object-instance domains. The focused Catch2 case
proves equality preservation across joined ambassadors, invalid-designator
exceptions, and the preserved value of a departed federate designator. The
opaque handle-coordinate mapping is deliberately not exposed as a sequential
scheme, while `HLAserviceGroup` remains inside its fixed MIM range. The paired
`compliance/handle-normalization-api-contract.json` and
`compliance/handle-normalization-requirements-contract.json` files record
source/API traceability only. This enables a later RTI-originated MOM point
region and report routing path; it does not itself send MOM reports or prove
general DDM/MOM conformance. RL-033 records the source-candidate clause
attribution drift discovered while mapping the services.

For a developer-supplied, reviewed SISO corpus, add the following option to
the non-installable libxml2 build configuration:

~~~powershell
-DUMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY="C:\path\to\siso-corpus"
~~~

It checks seven pinned external XML files before testing that their
IEEE 1516-2010 namespace is rejected by the 2025 DIF policy. It does not copy
those files into Umbra or establish 2010 compatibility.

For a developer-supplied, reviewed **2025-native** corpus, use the separate
non-installable libxml2 development-profile option:

~~~powershell
-DUMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY="C:\path\to\2025-fom-corpus"
~~~

`compliance/external-2025-fom-corpus.json` pins four external modules by
digest, 2025 namespace, and DIF schema location. With that option, the
development tests verify every module under DIF, then prove that the exact
MIM-first set is deterministically rejected by the source-backed
object-attribute/interaction-parameter data-type rule and maps to
`InconsistentFOM` at embedded federation creation. The pinned prototype uses
raw basic-data representations in those application-table columns, so DIF
acceptance alone is intentionally not treated as 2025 semantic acceptance.
This is not raw Requirements-Lab evidence, a release dependency, a runtime
scenario result, or a conformance claim. Umbra remains 2025-only; the SISO
lane above remains an expected-rejection guard rather than a compatibility
feature.

For a fixed ordered official MIM plus Restaurant-base module set, Catch2 also
repeats composition and compares the resulting private FDD bytes, composed
module names, and representative catalog lookups. This is a reproducibility
invariant for Umbra's private materializer, not a canonicalization claim about
arbitrary FOM XML or a public FOM-service result.

The following separate source-tree build enables Umbra's non-installable
embedded federation-management development profile. It adds real official
Create/Destroy/Join/Resign, joined-name/departed-designator lookup,
FOM-backed object-/interaction-class plus inherited-attribute/parameter name/handle lookup,
mandatory transportation-type name/handle lookup, limited receive-order
interaction and attribute-update/reflection delivery, object-instance and
object-class request/provide attribute-value delivery, unnamed object-instance
registration/discovery with known-instance lookup, single and multiple
object-instance name reservation/release with result callbacks,
federation-list/report
callback behavior, metadata-only region-template/specification lifecycle with
pending/committed range state, and initial time-advance/query/grant/bound
paths, but no public catalog
evidence, JUnit sidecar evidence, protected review, package support, or
conformance claim.

The same development profile now has a bounded order-type lookup slice. The
Catch2 case exercises the official `getOrderType` and `getOrderName` methods,
the legal `Receive`/`TimeStamp` pair, invalid name/type handling, and the
connected-but-unjoined boundary. The paired
`compliance/order-type-api-contract.json` and
`compliance/order-type-requirements-contract.json` files keep this source/API
traceability distinct from validation or conformance evidence. The adjacent
`compliance/order-type-control-api-contract.json` and
`compliance/order-type-control-requirements-contract.json` contracts now pin
`changeAttributeOrderType`, `changeDefaultAttributeOrderType`, and
`changeInteractionOrderType`. Their Catch2 lifecycle cases prove prospective
class defaults, per-instance overrides, publisher-scoped interaction order,
mixed immediate/TSO callback fields, and a negotiated ownership transfer that
resets the old instance override to the acquiring federate's default. The If
Available and remaining ownership-disposition variants still need dedicated
order-aware public cases.

The time-management kernel now contains a private `TsoMessageQueue` and
federation-owned `FederationTimeCoordinator`. Catch2 uses the official 2025
`LogicalTime` classes to exercise recipient-scoped ordering, equal-timestamp
stability, inclusive/exclusive eligibility, pending fanout retraction,
queued/in-transit/completed callback state, resign cleanup, and LITS when GALT
is unregulated. The queue and coordinator contracts
(`compliance/tso-message-queue-requirements-contract.json` and
`compliance/federation-time-coordination-tso-requirements-contract.json`)
check those source/test anchors against the pinned Requirements Lab bundle.
The first bounded public consumer is traced separately by
`compliance/timestamped-interaction-requirements-contract.json`: the official
non-regional timestamped Send Interaction overload, Retract, and
MessageRetractionHandle decoding. Its companion
`compliance/request-retraction-requirements-contract.json` traces the bounded
post-delivery normal interaction, region-context interaction, normal and
bounded regional attribute-update, and directed-interaction paths: strict
Retract eligibility, Request Retraction for an already-delivered recipient,
and suppression of still-queued fanout. Its current lifecycle slice also
keeps a lightweight terminal designator after the strict boundary has passed,
so queued typed delivery can drain while later `Retract` calls report
`MessageCanNoLongerBeRetracted`; timestamped deletion state and an object name
are released only after that terminal state has no pending recipient.
The separate
`compliance/timestamped-attribute-update-requirements-contract.json` traces
the non-regional timestamped Update Attribute Values / Reflect Attribute
Values path, including recipient-specific transportation passels. Their
Catch2 scenarios cover lower-bound validation, retraction before grant,
exact-bound callbacks before Time Advance Grant, timestamp/order/retraction
fields, sender exclusion, pending fanout suppression, and a two-passel
immediate attribute recipient followed by Request Retraction. The exact 2025
source, rather than a fragment-only Lab candidate,
supplies the strict time-plus-actual-lookahead interpretation; RL-019 records
the export limitation.
The paired `compliance/timestamped-regional-attribute-update-requirements-contract.json`
and `compliance/timestamped-regional-attribute-update-api-contract.json` now
trace the bounded regional extension. Its Catch2 scenario commits update
regions, queues a timestamped reflection for a time-constrained recipient,
proves retraction before the grant, and rechecks the recipient's Convey Region
Designator Sets switch so the optional sent-region callback metadata is absent
when disabled and present when enabled. The same Catch2 case also proves
mixed immediate/TSO fanout: the non-time-constrained recipient receives its
timestamped callback immediately while the constrained recipient remains
queued until its grant. The same scenario proves `Request Retraction` for the
delivered immediate recipient and suppresses the pending constrained fanout.
Timestamped default-region behavior, regional request forms, alternate advance modes,
transport, package/JUnit/protected-review evidence, and conformance remain
outside this slice.
`compliance/timestamped-object-deletion-requirements-contract.json` and its API
companion add the bounded non-regional timestamped Delete Object Instance /
Remove Object Instance path, including pending-delete reconstitution, exact-
bound removal before the grant, and the timestamped callback fields. A paired
mixed-fanout Catch2 scenario now also proves legal post-delivery Request
Retraction: an execution-owned invocation snapshot restores the object/name/
known state and committed split ownership before the delivered recipient sees
the official callback, while a constrained recipient's pending removal is
suppressed. A separate scenario also proves an owner who resigns after delivered
removal is neither reconstituted nor notified. A terminal no-recipient case
preserves `MessageCanNoLongerBeRetracted` while releasing the deletion snapshot
and name for fresh registration. A focused normal-interaction regression now
covers one Disable Time Regulation/re-enable lifetime path at unchanged
lookahead. Complete alternate-advance, broader re-enable, active in-flight
ownership, other resignation, and save/restore recovery evidence, transport,
and conformance remain outside this bounded slice.
`compliance/timestamped-directed-interaction-requirements-contract.json` and
its API companion add the bounded non-regional timestamped directed-interaction
send/receive path, including known-target routing, pending retraction, exact-
bound callback delivery before the grant, and timestamped callback fields.
`compliance/timestamped-regional-interaction-requirements-contract.json` and
its API companion add the bounded region-context timestamped Send Interaction
With Regions / Receive Interaction path, including strict committed overlap,
pending retraction, exact-bound callback ordering, and sent-region callback
metadata.
Other regional/object-region forms, alternate advance modes, legal
post-delivery request-retraction callbacks outside the currently bounded
interaction, attribute-update, and non-regional deletion paths, transport, and
conformance remain outside these slices.

The embedded profile now also has bounded untimed save and restore control
slices. The paired `compliance/save-control-api-contract.json` and
`compliance/save-control-requirements-contract.json` files pin the official
Request Federation Save, Federate Save Begun/Complete/Not Complete, Abort, and
Query Federation Save Status declarations plus their callbacks. Completed saves
now retain a process-local copyable federation image. A separate source-linked
three-member Catch2 case exercises the time-constrained exception to ordinary
untimed initiation: it keeps the request pending while constrained members are
idle, invokes `Initiate Federate Save` directly before each constrained
member's ordinary `Time Advance Grant`, and queues the non-time-constrained
member only after both constrained members reach their callback boundary. That
untimed path is deliberately limited to stable membership/time roles and
non-Flush-Queue grant dispatch; its timestamped counterpart is recorded below,
and neither path is evidence for pending-grant restore or membership/role
churn. The paired
`compliance/restore-control-api-contract.json` and
`compliance/restore-control-requirements-contract.json` files pin Request
Federation Restore, all-member restore initiation, completion/failure/abort,
and status callbacks. Catch2 proves object-state rollback, status transitions,
missing-label failure, participant failure, and abort. This remains bounded
development-profile evidence. A companion directed-TSO regression proves that
the private queue restores saved temporal payload/state without reusing a
message retraction designator allocated after the save; the stale handle is
invalid after restore and cannot affect fresh traffic. The paired
normal-interaction regression also saves a live queued payload and retraction
record, terminalizes that record after the save, restores it, delivers the
original callback through Flush Queue Request, and performs a legal restored
Retract with its Request Retraction callback. The paired
one-federate regression saves a terminal retraction tombstone, creates a
distinct post-save message, restores the snapshot, and distinguishes the saved
`MessageCanNoLongerBeRetracted` result from the discarded handle's invalid
result. A further one-federate regression saves a time-regulating member at
logical time 3 with actual lookahead 2, makes post-save changes to both values,
and verifies that restore returns the saved time window. It remains a narrow
process-local rollback proof rather than pending-advance, time-constrained,
timed, durable, or transport restore evidence. A companion regression also
saves an actual lookahead with a deferred decrease, consumes it after the
save, then restores and advances again to prove the deferred target returns.
The paired
`compliance/save-restore-interlock-api-contract.json` and
`compliance/save-restore-interlock-requirements-contract.json` files now pin
the shared SaveInProgress/RestoreInProgress gate for object-class and
interaction declaration forms, directed declaration, order/transport,
ownership, time-role/query, scope-advisory, and representative publication,
subscription, object, DDM, update, interaction, and time-advance services;
Catch2 exercises both operation states before any of those services mutate
state. Timed restore, durable external persistence, post-restore handle remapping,
interlocks on every remaining service, transport, package/JUnit/protected
evidence, and conformance remain open. A separate
`compliance/timed-save-api-contract.json` and
`compliance/timed-save-requirements-contract.json` pair now covers the exact
2025 `Request Federation Save(label, LogicalTime)` overload: official logical-
time validation, replacement of one pending request, the requirement that all
time-constrained members cross the timestamp, and release only after queued or
in-transit TSO payloads at or below that timestamp have been delivered. The
ordinary-grant dispatcher keeps a constrained recipient Time Advancing while
it drains those TSO payloads, invokes `Initiate Federate Save` directly, and
only then applies the matching grant. One Catch2 case proves the TSO →
initiate → grant ordering at the exact TAR save boundary; a second
three-member case proves both constrained members are admitted before the
non-time-constrained regulator is queued. A third case requests the save while
the constrained recipient's timestamped callback is in progress and proves its
direct initiation waits for that in-transit delivery to return. A fourth case
proves TARA's exclusive
boundary: an equal next grant does not initiate save, while a strictly later
grant does. A fourth case covers the corresponding next-message forms: NMR at
the exact timestamp initiates before its grant, whereas an equal NMRA grant
does not initiate the replacement request until a later grant. Flush Queue
Request, pending-grant restore, membership/role churn, durable save/restore,
transport, protected evidence, and conformance remain open.

~~~powershell
cmake -S . -B .build-fom-services -G "Visual Studio 17 2022" -A x64 `
  -DUMBRA_FETCH_CATCH2=ON `
  -DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON `
  -DUMBRA_FETCH_LIBXML2=ON `
  -DUMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=ON
cmake --build .build-fom-services --config Debug -- /m:1
ctest --test-dir .build-fom-services -C Debug --output-on-failure
~~~

`compliance/fom-composition-requirements-contract.json` is a separate
Requirements-Lab reference contract for that private preflight. It pins the
Annex C requirement IDs to the implementation symbols and Catch2 selectors,
and CTest runs it as `umbra.ieee1516_2_2025.composition_traceability`. It is
source/test traceability only: it deliberately produces no public-service
catalog entry, JUnit sidecar result, or verified evidence status.

`compliance/reference-time-requirements-contract.json` similarly pins the
private IEEE 1516.1 reference-time values, encodings, factory names, and
epsilon arithmetic, plus the creation default and common-implementation
selection boundary, to the Lab's source-derived requirement IDs. CTest runs it
as `umbra.ieee1516_2025.reference_time_traceability`. It is not public
`getTimeFactory`, Create, or time-management evidence, and it deliberately does
not add an entry to the JUnit sidecar catalog.

`compliance/logical-time-encoding-requirements-contract.json` separately pins
the official `HLAlogicalTime` and `HLAlogicalTimeInterval` C++ helpers to the
source-derived §12.3 behaviors for opaque factory encoding/decoding and
factory-created initial or zero values. CTest runs it as
`umbra.ieee1516_2025.logical_time_encoding_requirements_traceability`. The
implementation delegates to the selected factory and has no universal Umbra
time wire form; its nested `decodeFrom` path is deliberately limited to the
fixed-width reference profile because `LogicalTimeFactory` does not report a
consumed length. This is source/test traceability only, not public
`getTimeFactory`, catalog, JUnit, package, interoperability, or conformance
evidence. RL-055 records that the selected Lab candidates currently export
§12.4/Annex provenance despite the reconstructed §12.3 source.

`compliance/authorization-requirements-contract.json` separately pins the
official `HLAplainTextPassword` C++ credential value to the source-derived
`HLAunicodeString` constructor behavior, the `HLAauthorizer` and factory
foundation, the standard factory-forwarding boundary, and the embedded
profile's disabled-authorization Connect gate. CTest runs it as
`umbra.ieee1516_2025.authorization_requirements_traceability`. The current
profile accepts the explicit empty `HLAnoCredentials` form but rejects another
supplied credential with `Unauthorized`; it does not select or execute the
reference authorizer. Its global plaintext-password behavior is exercised
only through an internal test seam pending secure RID configuration and
runtime lifecycle work. This is source/test traceability only, not
authorization conformance, catalog, JUnit, package, interoperability, or
security-review evidence. RL-056 records the exported §12.8/Annex provenance
drift from the reconstructed §§12.5-12.6 source; RL-057 records the
authorization-library source/header method-name discrepancy.

`compliance/federation-management-preparation-requirements-contract.json`
separately pins the private MIM-first creation and additional-FOM preparation
pipeline. `compliance/federation-management-embedded-requirements-contract.json`
separately pins the opt-in adapter's connection, Create, Destroy, Join, and
Resign symbols to the same source-derived Lab records and real Catch2
selectors. CTest registers that second contract only in the development
profile. Both contracts are traceability only: they are not catalog, JUnit,
protected-review, package, or conformance evidence.

`compliance/get-time-factory-api-contract.json` is a third, development-profile
only contract. It validates the Lab's exact C++ `getTimeFactory` signature and
declared exceptions against the adapter and its Catch2 selector. The Lab has no
higher-level implementation mapping for that service yet, so this is API
traceability—not catalog or evidence promotion.

`compliance/federate-lookup-api-contract.json` similarly pins the exact 2025
C++ `getFederateHandle` and `getFederateName` signatures and declared exception
sets to the opt-in adapter and a real Catch2 selector. `getFederateHandle`
resolves a currently joined name, while `getFederateName` retains the immutable
name of a returned designator after normal or Connection Lost resignation;
neither behavior expands the caller's joined-federation boundary. The paired
`compliance/federate-lookup-requirements-contract.json` selects the Lab's Join
designator-lifetime and Get Federate Name requirements. The Lab has no
higher-level implementation mapping for either service, so this remains
source/API traceability—not catalog or evidence promotion.

`compliance/object-class-lookup-api-contract.json` pins the exact 2025 C++
`getObjectClassHandle` and `getObjectClassName` signatures and declared
exception sets to the same profile. Its real Catch2 scenario resolves classes
from the composed FOM catalog and preserves a prior handle when a compatible
additional-FOM join extends that catalog. The Lab has no higher-level
implementation mapping for either service, so this remains API traceability—
not catalog or evidence promotion.

`compliance/interaction-class-lookup-api-contract.json` pins the exact 2025
C++ `getInteractionClassHandle` and `getInteractionClassName` signatures and
declared exception sets to the same profile. Its real Catch2 scenario resolves
interactions from the composed FOM catalog and preserves a prior handle when a
compatible additional-FOM join extends that catalog. The Lab has no higher-level
implementation mapping for either service, so this remains API traceability—
not catalog or evidence promotion.

`compliance/attribute-lookup-api-contract.json` pins the exact 2025 C++
`getAttributeHandle` and `getAttributeName` signatures and declared exception
sets to the same profile. Its real Catch2 scenario resolves an inherited
attribute through its defining class, distinguishes invalid class/attribute
handles from a valid attribute not defined on the supplied class, and preserves
a prior value when a compatible additional-FOM join extends the catalog. The
Lab has no higher-level implementation mapping for either service, so this
remains API traceability—not catalog or evidence promotion.

`compliance/parameter-lookup-api-contract.json` pins the exact 2025 C++
`getParameterHandle` and `getParameterName` signatures and declared exception
sets to the same profile. Its real Catch2 scenario resolves an inherited
parameter through its defining interaction class, distinguishes invalid
interaction-class/parameter handles from a valid parameter not defined on the
supplied interaction class, and preserves a prior value when a compatible
additional-FOM join extends the catalog. The Lab has no higher-level
implementation mapping for either service, so this remains API traceability—
not catalog or evidence promotion.

`compliance/interaction-declaration-api-contract.json` pins the exact 2025 C++
`publishInteractionClass`, `unpublishInteractionClass`,
`subscribeInteractionClass`, and `unsubscribeInteractionClass` signatures and
declared exception sets to the opt-in adapter. Its Catch2 coverage checks the
public connection/membership/FOM-handle boundary; a registry-level case checks
independent per-federate declaration state, active/passive subscription state,
active-only receive eligibility, idempotent removal, and resign cleanup. The
Lab has no higher-level
implementation mapping for these services, so this is API traceability—not
catalog or evidence promotion. The declaration state now feeds a separate
limited receive-order interaction path and the ordinary Turn Interactions
On/Off relevance advisory slice. Regional advisories, MOM behavior, or
conformance remain outside this traceability-only claim.

`compliance/object-class-attribute-declaration-requirements-contract.json` and
`compliance/object-class-attribute-declaration-api-contract.json` pin the exact
2025 C++ `publishObjectClassAttributes`, `unpublishObjectClassAttributes`,
`subscribeObjectClassAttributes`, and `unsubscribeObjectClassAttributes`
signatures and declared exception sets to the opt-in adapter. The private
registry scenario covers explicit state, inherited-attribute validation,
active/passive subscription state, active-only discovery/reflection routing,
passive delivery suppression, class-local removal, and resign cleanup; the
public scenario covers connection, membership, and FOM-handle boundaries. The
later registration/discovery profile consumes this declaration state, including
the limited implicit privilege effect at registration time. The declaration
slice also raises the official `OwnershipAcquisitionPending` exception rather
than unpublish a class attribute required by the bounded acquisition-if-
available path. RL-014 records that the pinned Lab export has no granular
candidate for that 5.3.3(f) rule. Ordinary Start/Stop Registration relevance
advisories are now traced by the separate declaration-relevance-advisory
contract; regional advisory behavior, full ownership, update-rate enforcement,
regions, catalog evidence, and conformance remain outside this slice.

The separate `compliance/whole-object-class-declaration-requirements-contract.json`
and `compliance/whole-object-class-declaration-api-contract.json` cover the
whole-class teardown boundary. The embedded profile removes the complete
ordinary publication set (including the implicit delete privilege), clears the
corresponding ownership on registered instances, rejects later updates, and
removes ordinary subscriptions without deleting independent regional
declarations. Catch2 covers the official methods and their lifecycle/error
boundaries. Regional whole-class teardown and regional declaration advisory
callbacks, save/restore, transport, packaging, protected evidence, and
conformance remain outside this traceability-only slice.

`compliance/object-instance-registration-requirements-contract.json` and
`compliance/object-instance-registration-api-contract.json` pin the exact 2025
unnamed `registerObjectInstance`, `discoverObjectInstance`,
`getKnownObjectClassHandle`, `getObjectInstanceHandle`, and
`getObjectInstanceName` records to the opt-in adapter and real Catch2 selectors.
The public integration case covers publication and lifecycle preconditions,
generated names, source-known lookup, exact and active-superclass discovery, late
subscription, cancellation before evocation, producer identity, and both
callback models; the private registry case covers unique generated identities.
These contracts are source/API traceability only. Timestamped/retraction, DDM
scope, ownership transfer, FOM
sharing policy, save/restore, resign-action disposition, catalog evidence, and
conformance are explicitly outside the slice. The separate receive-order
update/reflection slice consumes its registration ownership and known-instance
state.

`compliance/object-instance-name-reservation-requirements-contract.json` and
`compliance/object-instance-name-reservation-api-contract.json` pin the exact
2025 single/multiple reservation and release declarations plus all four result
callbacks. The test covers official empty/`HLA.` guards, asynchronous success
and contention, mixed multiple outcomes, atomic release, generated-name
collision avoidance, and resignation cleanup. These are development-profile
source/API traceability contracts only; JUnit/protected review evidence,
packaging, and conformance remain deferred.

`compliance/object-instance-named-registration-requirements-contract.json` and
`compliance/object-instance-named-registration-api-contract.json` pin the exact
2025 non-region and regional named registration overloads. The Catch2 case
requires reservation ownership, preserves a reservation across a publication
failure, consumes it only after object/name commit, checks name-in-use and
not-reserved outcomes, and verifies uniform named lookup/discovery plus
release-before-registration reuse. These contracts are source/API traceability
only; timestamped/retraction, broader DDM, ownership transfer,
save/restore, JUnit/protected review evidence, packaging, and conformance remain
deferred.

`compliance/object-instance-deletion-requirements-contract.json` and
`compliance/object-instance-deletion-api-contract.json` pin the exact 2025
no-time `deleteObjectInstance` and `removeObjectInstance` records to the
opt-in adapter and a real Catch2 selector. The integration case covers
connection/membership/known-instance boundaries, requires the current
`HLAprivilegeToDeleteObject` owner, suppresses the deleting federate's induced
callback, preserves an evoked recipient's known state until removal starts, and
checks tag/producer propagation for both callback models. These contracts are
source/API traceability only. Timestamped/retraction deletion, ownership
transfer, DDM, FOM sharing policy, save/restore, resign-action disposition,
catalog evidence, and conformance remain outside the slice.

`compliance/local-delete-object-instance-requirements-contract.json` and
`compliance/local-delete-object-instance-api-contract.json` trace the exact
2025 `localDeleteObjectInstance` service to the bounded registry transition and
real Catch2 case. The case proves that only the invoking federate forgets the
known instance, that the federation-wide object remains available for
rediscovery, and that `FederateOwnsAttributes` and
`OwnershipAcquisitionPending` are enforced. These contracts are source/API
traceability only; timestamped/local-delete interaction behavior, DDM,
save/restore, remote transport, catalog evidence, and conformance remain
outside the slice.

`compliance/dimension-lookup-requirements-contract.json` and
`compliance/dimension-lookup-api-contract.json` trace the bounded 2025
dimension metadata foundation. The composed FOM catalog retains class and
interaction dimension associations plus each dimension's upper bound; the
registry allocates stable handles; and the official lookup services expose
inherited available-dimension sets, names, and upper bounds. These contracts
are source/API traceability only.

`compliance/region-lifecycle-requirements-contract.json` and
`compliance/region-lifecycle-api-contract.json` trace the bounded 2025
region-template/specification slice. The non-installable development profile creates private region
templates from composed dimensions, keeps `Set Range Bounds` values pending,
requires a complete set for an atomic commit, validates lower/upper values
against the FOM dimension bound, exposes owner-scoped dimension/range support
lookups, deletes unused regions, and decodes official region handles. These
contracts remain source/API traceability only. The separate object-attribute
regional contract below covers a bounded no-name registration, association,
subscription, and no-time overlap-routing slice; timestamped regional forms
are traced separately below,
broader DDM routing, save/restore, packaging, catalog evidence, and conformance
remain outside this region-template slice.

`compliance/interaction-region-requirements-contract.json` and
`compliance/interaction-region-api-contract.json` pin the 2025 regional
interaction declaration and no-time send overloads to the opt-in adapter and
the `Embedded regional interaction subscriptions filter 2025 receive-order
sends` Catch2 case plus the passive-subscription regression. Together they
cover independent ordinary/regional state, active/passive regional pairs,
committed-range overlap, empty regional sets, callback-entry rechecks, and
foreign/uncommitted/context-invalid region failures. The object-attribute
regional forms are traced separately below; regional value-update requests beyond the
class-level form,
timestamped/retraction behavior, region realization beyond explicit
associations, broader DDM routing, save/restore, packaging, catalog evidence,
and conformance remain separate.

`compliance/object-attribute-region-requirements-contract.json` and
`compliance/object-attribute-region-api-contract.json` trace the bounded 2025
clauses 9.5 through 9.9 object-attribute regional forms to the official C++
adapter and the `Embedded regional object attributes filter 2025 no-time
updates by overlap` Catch2 case plus the passive object-attribute regression.
Together they cover no-name registration with explicit associations,
additive/idempotent association, empty-region no-ops, active/passive regional
subscriptions and unsubscriptions, committed region ownership and dimension
validation, active-overlap-filtered discovery and no-time reflection, passive
suppression, the optional sent-region callback marker, and disjoint-range
suppression. Named
regional registration, the separately traced default-region realization, remaining timestamped/
retraction behavior beyond the bounded regional attribute-update contract,
broader DDM routing, save/restore, packaging,
catalog evidence, and conformance remain outside the slice. These contracts are
source/API traceability only.

`compliance/default-region-requirements-contract.json` records the bounded
IEEE 1516.1-2025 default-region realization. The registry never exposes a
synthetic default `RegionHandle`: absence of an explicit source association,
or absence of an explicit regional declaration on the receiving side, selects
derived default-region state. The paired object
and interaction Catch2 cases prove ordinary/regional effectiveness, replacement
and restoration around an explicit object update association, discovery/scope
continuity, and supplied-empty `RegionHandleSet` callback metadata when Convey
Region Designator Sets is enabled. Two companion time-constrained TSO cases
now prove the same supplied-empty convention for one timestamped object
reflection and one timestamped interaction callback after federation-owned
queueing. This remains bounded development-profile traceability only; a
complete timestamped matrix, relaxed DDM, broad dimension combinations,
packaging, catalog evidence, and conformance remain outside the claim.

`compliance/ownership-transfer-update-region-requirements-contract.json` and
`compliance/ownership-transfer-update-region-api-contract.json` add the
ownership boundary that the regional slice previously left open. The
`Embedded ownership transfer clears the former owner's 2025 update-region
association` Catch2 case registers and subscribes a regional object, transfers
an attribute through If Available and Divestiture If Wanted, proves that the
former owner's explicit association is removed and that the default source
realization resumes, then proves replacement with the new owner's explicit
region. The private
registry applies the same owner-scoped cleanup to Confirm Divestiture,
unconditional divestiture, unpublish, and resignation. These contracts remain
development-profile traceability, not conformance evidence; complete transfer
arbitration, a complete timestamped default-region matrix, advisory callback completeness, and
remaining regional forms are still open.

`compliance/object-attribute-scope-requirements-contract.json` and
`compliance/object-attribute-scope-api-contract.json` trace the official 2025
`attributesInScope` / `attributesOutOfScope` callbacks and the per-federate
Attribute Scope Advisory Switch accessors. The Catch2 case enables the switch,
discovers one known regional object, and moves active committed subscriber-region
overlap and the producer's update-region association through both HLA_IMMEDIATE
and HLA_EVOKED delivery. Passive state remains declared but cannot make an
attribute in scope. It verifies one grouped callback per transition and
suppresses stale queued transitions when the current overlap, association, or
subscription changes before evocation. The Lab's at-most-one candidates are
retained as bounded traceability anchors; this case now exercises all three
known-object transition sources. These contracts are source/API traceability
only; full timestamped default-region coverage, timestamped/retraction behavior, broader DDM
routing, packaging, catalog evidence, and conformance remain separate.

`compliance/attribute-relevance-advisory-requirements-contract.json` and
`compliance/attribute-relevance-advisory-api-contract.json` trace the official
2025 Attribute Relevance Advisory switch accessors plus both the no-rate and
rate-bearing `Turn Updates On For Object Instance` / `Turn Updates Off For
Object Instance` callbacks. The integration case establishes knowledge through a subscribed
attribute, then moves another owned attribute into and out of effective scope
through active ordinary subscription transitions; the registry uses the same
planning boundary for active regional subscription and update-region transitions;
passive state cannot arrange a relevance advisory.
The adapter rechecks the switch, ownership, known instance, current scope, and
retained designator before callback entry. The rate-bearing path is covered for
an explicit `High` designator in both ordinary and regional transitions;
initial registration/discovery-time advisory generation, complete regional DDM,
update-rate enforcement, package/JUnit/protected-review evidence, and
conformance remain explicitly unimplemented. These contracts are source/API
traceability only.

`compliance/advisories-use-known-class-requirements-contract.json` and
`compliance/advisories-use-known-class-api-contract.json` pin the official
`getAdvisoriesUseKnownClassSwitch` getter and its two clause-10.55.1 return
statements. The FDD composer parses the switch, omitted input remains
Disabled, and federation creation captures one static value that all members
observe without rewriting it during an additional-FOM join. The Catch2 case
covers both the enabled value and the exact getter surface. The switch is not yet used
to implement complete known-class advisory generation, so this remains
development-profile traceability rather than conformance evidence.

`compliance/update-rate-value-requirements-contract.json` and
`compliance/update-rate-value-api-contract.json` pin the exact 2025
`getUpdateRateValue` and `getUpdateRateValueForAttribute` declarations. The
Catch2 case proves the Restaurant FOM's composed `High`/`Medium`/`Low` values,
the `HLAdefault` no-reduction boundary, invalid-designator handling, retained
ordinary subscription rates, unsubscribe removal, and known-object/defined-
attribute exception paths. The paired
`compliance/update-rate-subscription-api-contract.json` pins the two official
designator-bearing subscription declarations. Throttling and rate reduction
are kept as the next runtime slice. The Lab's exported second requirement
retains clause `10.13.2` even though its source-page heading has a `10.12`
numbering drift; that discrepancy is documented separately. These contracts
are development traceability only, not validation or conformance evidence.

`compliance/receive-order-attribute-update-requirements-contract.json` and
`compliance/receive-order-attribute-update-api-contract.json` pin the exact
2025 no-time `updateAttributeValues` and `reflectAttributeValues` declarations
to the opt-in adapter, private passel routing, and a real Catch2 lifecycle
case. The case covers source ownership and defined-attribute failures,
multi-transport FOM passels, known-class projection, passive-subscription
suppression, source exclusion, unsubscribe-before-delivery, tag/producer/type propagation,
and both callback models. These contracts are source/API traceability only.
They deliberately exclude timestamped/retraction behavior, regional request
forms, broader DDM routing, update-rate reduction, ownership transfer,
custom transportation, FOM sharing
policy, save/restore, catalog evidence, and conformance. RL-013 records the
Requirements Lab's current passelization clause-ownership mismatch.

`compliance/attribute-value-update-request-requirements-contract.json` and
`compliance/attribute-value-update-request-api-contract.json` pin the exact
2025 object-instance `requestAttributeValueUpdate` declaration and matching
`provideAttributeValueUpdate` callback to the opt-in adapter, private owner
routing, and a real Catch2 lifecycle case. The case covers the
connection/membership and known-instance/attribute boundaries, owner grouping,
defined-but-unowned suppression, requester-owner suppression, tag propagation,
one callback for a provider group, and a resigned provider before evoked
delivery. These contracts are source/API traceability only. They deliberately
exclude additional regional request forms, automatic provision and an ensuing value
update, timestamped/retraction behavior, DDM, update-rate reduction, ownership
transfer, FOM sharing policy, save/restore, catalog evidence, and conformance.

`compliance/attribute-value-update-response-requirements-contract.json` and
`compliance/attribute-value-update-response-api-contract.json` cover the
separate bounded 2025 response lifecycle. The provider test invokes the
official non-timestamped `Update Attribute Values` service from inside
`Provide Attribute Value Update`; the requester then verifies
`Reflect Attribute Values`, the response tag, producing federate, and mandatory
transportation type. This is explicit provider behavior, not RTI-automatic
provision, and remains development-profile source/API traceability only.

`compliance/auto-provide-requirements-contract.json` and
`compliance/auto-provide-api-contract.json` cover the bounded federation-wide
Auto Provide switch. The composed FDD preserves its Disabled omission default,
the development profile exposes the exact `getAutoProvideSwitch` getter, and
an enabled execution solicits each current owner of a newly discovered,
in-scope attribute through `Provide Attribute Value Update` with an empty
RTI-invoked tag. The disabled case proves discovery still succeeds without
that provider callback. The standard federation-wide `HLAsetSwitches` MOM
interaction is also exercised with the official four-byte `HLAswitch` values;
the change is visible to every current member and controls later discovery.
Other MOM control/reporting families, complete multi-owner/regional and
update-rate semantics, and catalog/JUnit/protected-review/conformance evidence remain
outside this traceability-only slice.

`compliance/object-class-attribute-value-update-request-requirements-contract.json`
and `compliance/object-class-attribute-value-update-request-api-contract.json`
pin the exact 2025 object-class `requestAttributeValueUpdate` declaration and
matching `provideAttributeValueUpdate` callback to the opt-in adapter, private
class expansion, and a real Catch2 lifecycle case. The case covers the
connection/membership and class/attribute boundaries, expansion from a base
class to two registered subclass instances without requester discovery, owner
grouping per object instance, requester-owner suppression, tag propagation, and
a resigned provider before evoked delivery. These contracts are source/API
traceability only. They deliberately exclude additional regional request forms,
automatic provision and an ensuing value update, timestamped/retraction behavior, DDM,
update-rate reduction, ownership transfer, FOM sharing policy, save/restore,
catalog evidence, and conformance.

`compliance/attribute-value-update-with-regions-requirements-contract.json`
and `compliance/attribute-value-update-with-regions-api-contract.json` pin the
exact 2025 class-level `requestAttributeValueUpdateWithRegions` declaration and
matching `provideAttributeValueUpdate` callback to the opt-in adapter, private
region-aware class expansion, and a real Catch2 lifecycle case. The case covers
committed region ownership/context, empty-pair no-op behavior, explicit
update-region overlap, default-region eligibility, tag propagation, and
callback-entry rechecks. Provider responses remain explicit user behavior;
automatic provision, resulting reflection, timestamped/retraction behavior,
broader DDM, catalog evidence, and conformance
remain outside this traceability-only slice.

`compliance/attribute-ownership-query-requirements-contract.json` and
`compliance/attribute-ownership-query-api-contract.json` pin the exact 2025
`queryAttributeOwnership`, `informAttributeOwnership`, and
`attributeIsNotOwned` declarations to the opt-in adapter, private ownership
snapshot, and a real Catch2 lifecycle case. The case covers connection and
membership boundaries, known-instance and known-class attribute validation,
grouped federate-owner/unowned results, the returned federate handle, and
nullification of pending reports when receive-order `Remove Object Instance`
starts. These contracts are source/API traceability only. They deliberately do
not model RTI-owned state or ownership acquisition/divestiture, and make no
catalog or conformance claim.

`compliance/attribute-ownership-check-requirements-contract.json` and
`compliance/attribute-ownership-check-api-contract.json` pin the exact 2025
`isAttributeOwnedByFederate` declaration to the opt-in adapter, private
ownership snapshot, and a real Catch2 case. The case validates the official
connection, membership, known-instance, known-class attribute, and removal
boundaries, then distinguishes the invoking current owner from a remote owner
and an unowned attribute. It is source/API traceability only: there is no
callback, ownership transfer, RTI-owned state, catalog evidence, or conformance
claim.

`compliance/attribute-ownership-acquisition-if-available-requirements-contract.json`
and `compliance/attribute-ownership-acquisition-if-available-api-contract.json`
pin the exact 2025 C++ `attributeOwnershipAcquisitionIfAvailable`,
`attributeOwnershipAcquisitionNotification`, and `attributeOwnershipUnavailable`
declarations to the opt-in adapter, private pending state, and a real Catch2
case. The case covers connection/membership, known-instance and known-class
attribute validation, class/attribute publication, repeat-WTA no-op and
additional-attribute behavior, the paired pending-unpublication guard,
callback-time transfer of an unowned attribute, a remote-owned unavailable
result, tag propagation, and removal cancellation. These contracts are
source/API traceability only: they do not claim complete regular or negotiated
acquisition, remaining divestiture flows, RTI-owned state, full resign-action disposition,
catalog evidence, or conformance.

`compliance/attribute-ownership-acquisition-requirements-contract.json` and
`compliance/attribute-ownership-acquisition-api-contract.json` pin the exact
2025 C++ regular `attributeOwnershipAcquisition`,
`requestAttributeOwnershipRelease`, `attributeOwnershipReleaseDenied`,
`cancelAttributeOwnershipAcquisition`, and terminal callback declarations to
the opt-in adapter, separate private pending state, and real Catch2 cases. The
cases cover unowned callback-time
transfer, WTA override, repeat-request release suppression, acquisition-tag
propagation, multi-acquirer release denial, denial-tagged unavailable results,
accepted regular cancellation, a grouped confirmation callback, the
pending-unpublication guard, and removal cancellation. They are source/API
traceability only: competing cancellation races, negotiated acquisition,
remaining divestiture flows, RTI-owned state, full resign-action disposition, catalog evidence,
and conformance remain outside this slice.

`compliance/attribute-ownership-divestiture-if-wanted-requirements-contract.json`
and `compliance/attribute-ownership-divestiture-if-wanted-api-contract.json`
pin the exact 2025 C++ `attributeOwnershipDivestitureIfWanted` declaration and
`attributeOwnershipAcquisitionNotification` callback to the opt-in adapter,
its synchronous private transfer state, and two real Catch2 cases. The cases
cover connection/membership, known-instance, defined-attribute, and current-owner
exceptions; an empty returned result with no pending acquirer; synchronous
transfer to regular and If Available acquirers; divestiture-tag propagation;
the selected acquirer's publication guard through notification entry; stale
former-owner work; and deferred regular follow-up at the new owner. The
mixed-form earliest-accepted selection is explicitly bounded profile behavior,
not a standards-level arbitration claim. These contracts do not claim Request
Attribute Ownership Assumption outside the separate unconditional slice,
negotiated divestiture, RTI-owned state, full resign-action disposition,
catalog evidence, or conformance.

`compliance/unconditional-attribute-ownership-divestiture-requirements-contract.json`
and `compliance/unconditional-attribute-ownership-divestiture-api-contract.json`
pin the exact 2025 C++ `unconditionalAttributeOwnershipDivestiture`
declaration and `requestAttributeOwnershipAssumption` callback to the opt-in
adapter, its immediate private unowned transition, and a real Catch2 case. The
case covers connection/membership, known-instance, defined-attribute, and
current-owner boundaries; full-set immediate divestiture; preservation of
pending regular and If Available acquisitions; one grouped tagged offer to a currently eligible
non-pending federate; a known but unpublished exclusion; callback-time stale
publication suppression; and later standard If Available acquisition. These
contracts are source/API traceability only. They do not claim a continuing
owner search after later eligibility changes, negotiated divestiture and
confirmation, RTI-owned state, full resign-action disposition, catalog
evidence, or conformance.

`compliance/negotiated-attribute-ownership-divestiture-requirements-contract.json`
and `compliance/negotiated-attribute-ownership-divestiture-api-contract.json`
pin the exact 2025 C++ `negotiatedAttributeOwnershipDivestiture`,
`confirmDivestiture`, and `cancelNegotiatedAttributeOwnershipDivestiture`
declarations, plus the `requestDivestitureConfirmation` and resulting
`attributeOwnershipAcquisitionNotification` callbacks, to private runtime
state and a real Catch2 case. The case covers official connection/membership,
known-instance, attribute, ownership, and duplicate-divestiture boundaries;
owner retention while waiting; an existing or later regular acquirer; both
official user-tag paths; one-shot stale callback suppression; synchronous
confirmed transfer; the notification publication guard; explicit cancellation;
and `NoAcquisitionPending` after the selected regular acquirer cancels. These
contracts are source/API traceability only. They do not claim an ongoing owner
search, Willing-to-Acquire selection, negotiated acquisition, RTI-owned state,
full resign-action disposition, catalog evidence, validation, or conformance.

`compliance/transportation-type-api-contract.json` pins the exact 2025 C++
`getTransportationTypeHandle` and `getTransportationTypeName` signatures and
declared exception sets to the opt-in adapter. Its Catch2 coverage checks the
connection/membership boundary, both mandatory `HLAreliable` and
`HLAbestEffort` names, stable handles, and invalid names/handles. The Lab has
no higher-level implementation mapping for these services, so this is API
traceability—not catalog or evidence promotion. The separate
`transportation-type-change-requirements-contract.json`,
`interaction-transportation-type-change-requirements-contract.json`, and
`transportation-type-change-api-contract.json` trace the official
change/default/query services and four callback surfaces. The development
profile captures per-federate attribute defaults, commits accepted changes at
confirmation callbacks, and applies interaction overrides to future ordinary
and regional sends. These contracts still do not claim custom transportation,
timestamped/local
object-lifecycle delivery, message transport, or conformance.

`compliance/receive-order-interaction-requirements-contract.json` and
`compliance/receive-order-interaction-api-contract.json` trace the exact
non-timestamped C++ `Send Interaction` overload and no-time `Receive
Interaction` callback. The integration scenario covers official exception
boundaries, publication, exact active and passive-suppressed superclass
subscriptions, one callback for a dual subscription, sender exclusion, tag/producer/mandatory
transportation propagation, unsubscribe-before-delivery, and both callback
models. The private registry scenario uses the official Restaurant FOM to test
descendant-parameter projection. These contracts explicitly exclude
timestamped/retraction behavior, regional forms beyond the separate interaction
case, FOM sharing-policy enforcement, custom transportation, catalog evidence,
and conformance.

`compliance/directed-interaction-requirements-contract.json` and
`compliance/directed-interaction-api-contract.json` trace the bounded
non-timestamped, non-DDM C++ `Send Directed Interaction` overload, no-time
`Receive Directed Interaction` callback, and six object-class declaration
overloads. The integration scenario covers known-target discovery, sender
exclusion, immediate and evoked callbacks, selective unsubscribe, source
unpublication, stale callback suppression, republishing, tag/producer/mandatory
transportation propagation, and the ownership/universal selector. The selector
regression proves default by-ownership suppression for a known non-owner,
universal delivery to that non-owner, empty-set preservation, and supplied-class
mode changes. Timestamped/retraction behavior beyond the separate bounded
slice, directed DDM, ordering, FOM sharing-policy enforcement, catalog evidence,
validation, and conformance remain deferred.

`compliance/federation-listing-requirements-contract.json` and
`compliance/federation-listing-api-contract.json` pin the same development
profile's two List methods plus the three official report callback declarations
to source-derived requirement records and exact C++ Lab surfaces. The members
contract includes the separate missing-federation report path. CTest registers
both only for this profile. The Lab has no higher-level implementation mapping
for those records, so they remain source/API traceability rather than catalog
or evidence promotion.

`compliance/time-advance-requirements-contract.json` and
`compliance/time-advance-api-contract.json` pin joined-federate initial time,
Time Advance Request / Time Advance Grant, and Query Logical Time to
source-derived Lab requirements and exact C++ API surfaces. They run only in
the development profile. A separate limited scheduler now applies the shared
GALT/NRG policy to TAR callbacks across federates, and the private temporal
coordinator contributes queued/in-transit/delivered TSO state to the snapshot.
The bounded timestamped interaction, attribute-update, object-deletion,
directed-interaction, and region-context interaction contracts are the current
public TSO traffic consumers. The time-advance contracts now cover TAR, the
Available forms, and the bounded currently-queued NMR/NMRA paths, with an
explicit strict-versus-inclusive GALT policy. The Lab exports no higher-level
implementation mapping, so these contracts remain source/API traceability
rather than catalog or evidence promotion.

`compliance/time-role-requirements-contract.json` and
`compliance/time-role-api-contract.json` similarly pin the initial
Enable/Disable Time Regulation, Enable/Disable Time Constrained, Query
Lookahead, and callback paths. They make the callback-gated boundary explicit:
a pending enable blocks time advance until its callback. GALT/LITS are traced
separately, and role state changes re-evaluate the limited TAR scheduler when
they change eligibility. The separate coordinator contract covers private
timestamped queue state; the five timestamped-service contracts record the
bounded public producers. No contract asserts full time-management
coordination, a JUnit sidecar, catalog entry, protected review, package
support, or conformance.

`compliance/modify-lookahead-requirements-contract.json` and
`compliance/modify-lookahead-api-contract.json` add the bounded 2025 Modify
Lookahead path. Catch2 verifies immediate increases, gradual decreases at
grant boundaries, and the `InTimeAdvancingState`/not-enabled fences; the
accepted transition also wakes the limited TAR scheduler when its bound
changes. Remaining advance modes and full time-management coordination are
not implied.

`compliance/next-message-request-requirements-contract.json` and
`compliance/next-message-request-api-contract.json` add the bounded 2025
Next Message Request path. Catch2 queues a real timestamped interaction,
selects its timestamp when it is below the requested boundary, and verifies
that the equal-timestamp delivery occurs before `Time Advance Grant`. The
effective grant target and caller request boundary remain separate in private
state. Future transport input, Flush Queue Request/Grant, and full
time-management coordination remain outside this traceability slice.

`compliance/time-advance-request-available-requirements-contract.json` and
`compliance/time-advance-request-available-api-contract.json` trace the
bounded `Time Advance Request Available` path. The matching Catch2 scenario
proves that a queued TSO message can be delivered before the grant at an
inclusive defined-GALT boundary. The paired
`compliance/next-message-request-available-requirements-contract.json` and
`compliance/next-message-request-available-api-contract.json` trace the
currently queued `Next Message Request Available` target/cohort path and its
same inclusive boundary. Future transport input, Flush Queue Grant,
save/restore, package support, and conformance remain outside both slices.

`compliance/flush-queue-request-requirements-contract.json` and
`compliance/flush-queue-request-api-contract.json` trace the bounded Flush
Queue Request/Grant path. The Catch2 scenario flushes all currently queued
in-process TSO payloads, checks the request/GALT/delivered-timestamp minimum,
verifies the optimistic logical-time argument, and proves that the optimistic
floor constrains the next advance. The timestamped-save cases additionally
prove strict actual-FQG admission: an equal grant does not start the save, a
later grant drains TSO before direct initiation and FQG, and mixed FQR/TAR
members are all prequalified before the operation starts. A six-member
timestamped-save case combines that FQR branch with TAR, NMR, TARA, and NMRA
before the non-time-constrained notification. Future transport and multi-member/
multi-mode in-transit coordination, Request Retraction behavior outside normal timestamped Send
Interaction, and general save/restore behavior remain outside this slice. The
paired live-record restore regression uses Flush Queue Request only to cross a
restored recipient's callback boundary without moving the producer's strict
retraction lower bound; it is not a general save/restore claim. Package support
and conformance remain outside this slice.

`compliance/asynchronous-delivery-requirements-contract.json` and
`compliance/asynchronous-delivery-api-contract.json` trace the official
Enable/Disable Asynchronous Delivery pair. The unit and integration scenarios
prove the default-disabled state for time-constrained federates, defer
receive-order callbacks while idle, release them after enabling or entering
Time Advancing, and enforce the public already-enabled/already-disabled and
membership/connection boundaries. The implementation applies this gate to the
bounded embedded receive-order interaction, reflection, directed-interaction,
and object-removal routes; timestamped messages, remote transport, MOM
reporting, save/restore persistence of deferred callbacks, protected review,
package evidence, and conformance remain open.

`compliance/federation-time-coordination-foundation-requirements-contract.json`,
`compliance/time-bounds-requirements-contract.json`, and
`compliance/time-bounds-api-contract.json` record the federation-owned
snapshot, FDD Non-Regulated-Grant metadata, and exact Query GALT/Query LITS
surfaces. The private coordinator now supplies queued, in-transit, and
delivered-since-last-advance message inputs. GALT includes those timestamps and
the other-regulator candidate; LITS uses future queued/in-transit timestamps
and can remain defined when GALT is unregulated. The other-regulator candidate
uses current or pending time plus lookahead, with a factory epsilon for a
forward zero-lookahead TAR boundary. The contracts are source/API traceability
only, not a catalog or evidence promotion.

`compliance/time-grant-policy-requirements-contract.json` separately traces
the private no-TSO TAR eligibility decision: strict comparison with a defined
GALT and FDD Non-Regulated-Grant behavior when the bound is undefined. It is a
pure kernel test, not a scheduled grant, public-service, catalog, or evidence
claim.

`compliance/time-grant-scheduler-requirements-contract.json` traces the
limited private scheduler that uses that policy to queue eligible TAR grants
and rechecks the policy at delivery. Its embedded Catch2 cases cover a defined
GALT, disabled/default and enabled NRG, regulator enable/disable/resignation,
time-constrained disable, and a successful additional-FOM definition
replacement that changes NRG before waking an existing TAR. Like the other
temporal contracts, it is source/test traceability only, not a catalog or
evidence promotion.

compliance/api-baseline.json maps a deliberately small set of official
declarations to Requirements-Lab mapping, requirement, and transition IDs. It
is source traceability only: it names no Umbra implementation symbol or test
evidence.

Run the two local baseline checks with:

~~~powershell
python tools/verify_ieee_headers.py
python tools/verify_ieee_exception_binding.py
python tools/requirements_lab.py check
~~~

The CMake CTest suite runs the digest and exception-binding checks when a
Python interpreter is available, as well as a C++20 compile-and-run smoke test
against the official headers. Passing those checks establishes only that the
imported API baseline is intact and consumable.

The default-package smoke test configures a fresh client project against the
installed SDK, not the source tree.  It currently verifies the official
`VariableLengthData` caller-copy, borrowed-storage/copy, invalid-handle,
configuration-builder, ambassador-factory, and reference-time factory paths.
The matching Catch2 tests cover `VariableLengthData` ownership replacement and
custom deletion.  This is support-type/SDK-consumability evidence only; it is
not a public service or conformance claim. The bounded
RTI/encoding/BasicDataElements.h implementation has IEEE-source-derived
byte-vector coverage for HLAinteger32BE, HLAunsignedInteger32BE,
HLAinteger32LE, HLAunsignedInteger32LE, HLAinteger64BE, HLAinteger64LE,
HLAunsignedInteger64BE, HLAunsignedInteger64LE, HLAboolean, HLAunicodeString,
the 16-bit signed/unsigned big- and little-endian helpers, the big-/little-
endian octet-pair helpers, HLAoctet, HLAbyte, HLAASCIIchar, HLAASCIIstring,
HLAunicodeChar, and HLAfloat32BE/LE and HLAfloat64BE/LE. The raw one-octet
helpers preserve byte values; the ASCII helpers reject non-ASCII data and the
string uses the separately traced signed HLAinteger32BE HLAvariableArray count;
HLAunicodeChar carries exactly one UTF-16BE code unit; HLAunicodeString rejects
unmatched source and decoded surrogates; and the floating helpers require
32-bit/64-bit IEC 559 native float/double representations before applying the
standard byte order. All official `BasicDataElements.h` helper classes now have
definitions and source-derived direct Catch2 coverage, while the C++ helper and
1516.2 BasicDataElements contracts are CTest Requirements-Lab checks. This is
not generic constructed-data encoding, adapter exposure, interoperability, or
conformance evidence.
The distinct official `RTI/encoding/HLAopaqueData.h` C++ helper is now a
separate bounded slice. Its 1516.2 Table 35 representation is a dynamic
`HLAvariableArray` of `HLAbyte`: Catch2 fixes its signed `HLAinteger32BE`
element count, raw-byte payload, four-octet boundary, nested decode, malformed
input, and trailing-data behavior. The C++ header's external-memory form is
implemented as caller-owned borrowed storage: encoding observes caller changes,
while `set` and `decode` write only within the caller's declared buffer and
raise `EncoderException` when it is too small. Umbra neither allocates into nor
takes ownership of that storage. The separate 1516.1 helper contract and the
existing 1516.2 variable-array contract are CTest Requirements-Lab checks;
RL-053 records that the source-verified Table 27/Table 35 rows do not have
individual immutable helper candidates. This remains a specific SDK foundation
slice, not generic constructed encoding, adapter exposure, interoperability, or
conformance evidence.
The official `RTI/encoding/HLAfixedRecord.h` helper is now a separate bounded
constructed-data slice. The direct 1516.2 candidates require declaration-order
fields beginning at offset zero, zero padding between fields needed to align
the following element, and no trailing padding. Catch2 fixes the source
standard octet/boolean/float64 vector and covers reverse order, nesting,
malformed padding, structural type checks, cloned elements, and raw pointer
operations. Umbra treats the raw `DataElement*` operations as borrowed,
non-owning references because the standard header does not define ownership
transfer; that is an implementation safety policy rather than a standards
claim. A dedicated CTest Requirements-Lab contract verifies the three direct
candidate IDs. Their exported metadata remains broad `clause-4` even though
the reconstructed source heading is §4.14.10.1; RL-054 records that provenance
drift. Generic arrays or variants, model-specific encoder factories, adapter
exposure, interoperability, and conformance remain open.
The official `RTI/encoding/HLAfixedArray.h` helper is now a separate bounded
constructed-data slice. §4.14.10.4 defines a fixed-cardinality element
sequence beginning at offset zero, with zero inter-element padding determined
by the prototype element's octet boundary and no padding after the final
element. Catch2 fixes a source-derived array of fixed-record elements and
covers nested use, malformed padding, full-decode trailing data, templated
wrappers, prototype cloning, type checks, and externally supplied element
pointers. The public header expressly leaves raw-pointer lifetime with the
caller, so Umbra treats them as borrowed non-owning storage while copy/clone
uses internal memory; that policy is an implementation safety decision, not a
separate standard claim. A dedicated CTest Requirements-Lab contract validates
the fixed-array candidates. The direct sequence/no-final candidates retain
broad `clause-4` metadata and Equation (5)'s zero-padding source text lacks an
individual candidate; RL-054 records both limitations. Model-specific factories, adapter exposure,
interoperability, and conformance remain open.
The official `RTI/encoding/HLAvariableArray.h` helper is now a separate
bounded constructed-data slice. §4.14.10.5 requires a signed
`HLAinteger32BE` number-of-elements component at offset zero, then sequence
elements; its leading padding aligns the first element using the maximum of
the element boundary and four, while later padding uses the fixed-array rule
and no final padding is emitted. Catch2 fixes the source float64 leading-pad
vector and covers nested decoding, negative/truncated/malformed input,
full-decode trailing data, variable cardinality, fixed-record elements,
templated wrappers, type checks, and externally supplied element pointers.
The public header expressly leaves raw-pointer lifetime with the caller, so
Umbra treats those slots as borrowed non-owning storage while copies use
internal memory; that is an implementation safety decision, not a separate
standard claim. A dedicated CTest Requirements-Lab contract validates the
available direct candidates. Their metadata is broad `clause-4`, while the
Equation (6) boundary definition and zero-byte statement lack individual
candidates; RL-044 records those limits. Model-specific factories, adapter exposure, interoperability, and conformance
remain open.
The official `RTI/encoding/HLAvariantRecord.h` helper is now a separate
bounded constructed-data slice. §4.14.10.2 places the discriminant at record
offset zero and, when a mapped alternative exists, aligns that alternative by
adding the smallest needed zero padding after the discriminant. The source
defines the alignment boundary as the maximum across all alternatives. An
unmapped discriminant receives neither padding nor a value; no padding follows
a mapped alternative. Catch2 fixes a vector where a four-octet selected
alternative is padded to an eight-octet boundary because of another
alternative, and covers nested decoding, malformed padding, full-decode
trailing data, unmapped values, mapping/type checks, templated wrappers,
cloning, and externally supplied variant pointers. The public header expressly
leaves raw-pointer lifetime with the caller, so Umbra retains those variants as
borrowed non-owning storage while copies use internal memory. `DataElement::hash`
is used as its declared mapping hook but confirmed with both type and encoded
bytes before selecting a variant; collision handling is an implementation
safety choice, not a separate standard claim. A dedicated CTest
Requirements-Lab contract validates the direct candidates. Their metadata is
broad `clause-4`, while Equation (2)'s `Size`/`V` definitions have no
individual candidate; RL-054 records those limits. FOM-level enumerator ranges
and `HLAother` expansion, model-specific factories, adapter exposure,
interoperability, and conformance remain open.
The official `RTI/encoding/HLAextendableVariantRecord.h` helper is now a
separate bounded constructed-data slice. §4.14.10.3 places the discriminant at
record offset zero, then pads it to the four-octet boundary of the
`HLAinteger32BE` `encoded_length` element. The length describes only the
alternative bytes, is zero when no alternative applies, and excludes the
padding that follows it. A second zero-padding calculation aligns the
alternative to its predefined eight-octet boundary; alternatives with a
greater boundary are rejected. Umbra keeps that second layout padding in the
zero-length form as well, producing one deterministic length-prefixed layout
for an unknown discriminant to skip. No padding follows an alternative.
Catch2 fixes mapped and unmapped vectors, leading and post-length alignment,
nested decoding, strict zero-padding validation, declared-length mismatch,
truncation, trailing data, unknown-alternative skips, mapping/type controls,
templated wrappers, clones, the eight-octet boundary prohibition, and the
official header's caller-owned raw-pointer lifetime. `DataElement::hash` is
used as a mapping accelerator but checked with type and encoded bytes before a
mapping is selected; that collision handling is an implementation safety
choice, not a separate standard claim. A dedicated CTest Requirements-Lab
contract validates the five direct candidates. Their metadata is broad
`clause-4`, while Equations (3)/(4) and their `Size`/`V` definitions lack
individual candidates; RL-054 records those limits. FOM-level enumerator
ranges and `HLAother` expansion, model-specific factories, adapter exposure,
interoperability, and conformance remain open.
The official convenience header does not itself include the encoding helper
headers, so consumer tests include the needed `RTI/encoding/*` header
explicitly rather than relying on an Umbra-specific transitive include.

Catch2 is an explicit build-only dependency. A system Catch2::Catch2WithMain
target is used when available; otherwise configure CMake with
-DUMBRA_FETCH_CATCH2=ON to fetch pinned Catch2 v3.15.3 into .build. It tests
the official factory, the embedded Connect/Disconnect lifecycle, official
callback controls, the official integer/float reference-time and factory
behavior, and the private federation/handle/callback kernels. Tests use the
official NullFederateAmbassador helper, preserving the exact standard callback
interface without introducing a second public API. The development-profile
target additionally exercises two independent two-federate lifecycles and a
federation-listing report path plus callback-gated time advancement through
only official C++ methods and standard exceptions.

## Refresh the local requirements input

The helper defaults to ../Document-Recreation; set HLA_REQUIREMENTS_LAB_ROOT
when the lab is elsewhere. It uses the reviewed revision in
requirements-lab.lock.json by default. Override it only while deliberately
updating the dependency.

~~~powershell
$env:HLA_REQUIREMENTS_LAB_ROOT = (Resolve-Path ..\Document-Recreation)
python tools/requirements_lab.py export
python tools/requirements_lab.py check
~~~

To assess a newer Lab revision before changing the lock:

~~~powershell
python tools/requirements_lab.py export --revision <requirements-lab-commit>
python tools/requirements_lab.py check
~~~

export delegates to the Lab's official
hla_lab.publication.export_compliance_bundle command and writes the result to
.compliance/corpus-bundle.json. That directory is ignored: bundles, JUnit XML,
and compliance manifests are build or CI artifacts, not Umbra source.

## Native implementation evidence: first raw slice

The embedded connection slice implements all four C++ connect overloads and the
selected disconnect declaration through the official factory. It also binds the
four callback-control declarations to a private immediate/evoked dispatcher.
The configuration result explicitly reports that endpoint/additional settings
are not yet used except for the documented embedded service-report directory.
The backend has no configured authorizer: the ordinary no-credentials forms
remain usable, while an explicit non-`HLAnoCredentials` envelope is rejected
with `Unauthorized` rather than silently accepted. `HLAplainTextPassword` and
the reference authorizer/factory/library-forwarding foundation are present,
but reference authorizer configuration and runtime authorization remain later
work. In the default
packaged configuration, Create/Destroy/Join/Resign and all other unimplemented
RTI services continue to throw RTIinternalError from the generated fallback. The opt-in development
profile wires Create/Destroy/Join/Resign through MIM-first prevalidation and a
shared in-process registry. It additionally snapshots the registry for the
official List Federation Executions and List Federation Execution Members
services, then sends the corresponding report (or missing-federation report)
through the configured callback model. A private callback session prevents
queued/in-flight list reports from outliving Disconnect. The private embedded
transport endpoint now has a deterministic fault path that applies Automatic
Resign cleanup, forces the ambassador to Not Connected, removes membership,
and queues the official Connection Lost callback. A second multi-federate case
sets `DELETE_OBJECTS` through the official Automatic Resign Directive service,
then verifies delete-privileged object removal at the survivor. Its fault hook
is test-only and does not claim socket/IPC or package support. A third
multi-federate case selects `UNCONDITIONALLY_DIVEST_ATTRIBUTES`, then proves
that the survivor keeps the object known and receives the current eligible
ownership-assumption callback after the loss. The closest Requirements-Lab
candidate for the required automatic-resign step ends before its continuation
explicitly names the directive; RL-060 records that source-extraction gap. A
fourth case selects `DELETE_OBJECTS_THEN_DIVEST`, transfers a non-delete-
privileged attribute to the future lost federate, and then proves the loss
deletes its separately delete-privileged object before re-offering the retained
attribute. A fifth case selects `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, leaves
a regular acquisition request pending at the owner, then proves loss suppresses
the stale queued release request and lets a real surviving candidate receive a
later assumption offer. A sixth case selects
`CANCEL_THEN_DELETE_THEN_DIVEST`, leaves a regular acquisition pending, and
then proves the forced loss suppresses its stale release work, removes a known
delete-privileged object, and re-offers a retained non-delete attribute. The
separate private in-session control hook exercises the distinct `Federate Resigned`
callback: it removes a clean joined member through the standard `NO_ACTION`
registry path, keeps the connection open, and permits a new Join. It also
proves that self-resignation and Connection Lost do not deliver that callback.
The new paired Federate Resigned contracts select only the exact official C++
callback despite the Lab crosswalk issue logged as RL-025. Other
federation-event callbacks, bounded `NO_ACTION` forced-resign disposition
policy, remaining forced-resign disposition policy, and complete object/ownership behavior
remain open. The paired
`compliance/connection-lost-requirements-contract.json` and
`compliance/connection-lost-api-contract.json` pin the two explicit Lab
requirements and the exact `FederateAmbassador::connectionLost` declaration;
the Catch2 result remains development-profile traceability until JUnit and
protected review exist. Its joined federates can obtain a caller-owned factory for
their stored selected reference-time representation through `getTimeFactory`.
They can also resolve joined names to active handles and returned departed
handles to stable names within their joined federation; an invalid handle and a
valid handle outside that federation take their distinct declared exception
paths. They can resolve object- and
interaction-class names and handles plus inherited attribute and parameter
names and handles from the current composed FOM catalog; immutable
per-federation directories preserve existing class, defining-attribute, and
defining-parameter values across a compatible additional-FOM join. The same
profile retains independent per-federate interaction publication and
subscription declarations and implements a limited non-timestamped, non-region
Send Interaction / Receive Interaction path. That path selects the closest
active subscribed class for each recipient, projects available parameters, suppresses
the sender, and rechecks a queued recipient's subscription before callback
delivery. It also implements the bounded non-timestamped, non-DDM directed
interaction declaration and delivery path: the target must be known, the
sender is excluded, and declaration/lifecycle state is rechecked before an
immediate or evoked callback. The same profile also supports the three bounded regional interaction
services. A regional declaration is independent from the ordinary subscription
set; committed active region ranges and the 2025 overlap rule gate no-time
regional sends, passive pairs do not arrange delivery, empty sent sets suppress
delivery, and queued callbacks recheck the active overlap. It also supports unnamed non-region object registration, generated
private names, discovery to eligible subscribers, and the three known-instance
lookup services. The registry rechecks a queued discovery before callback and
fixes the recipient's discovered class when it succeeds. It supports no-time
Delete Object Instance / Remove Object Instance and a bounded non-regional
timestamped Delete Object Instance / Remove Object Instance path: the current
delete-privilege owner removes itself without an induced callback, each other
known recipient transitions at its callback boundary, and pending timestamped
deletion can be retracted before delivery to reconstitute the object.
Regional forms beyond the
bounded object-attribute case, broader DDM scope, ownership transfer, FOM
sharing policy, save/restore, and remaining resign-action object disposition remain
unimplemented. It also supports a no-time Update Attribute Values / Reflect
Attribute Values path: it requires source ownership, groups values
by FOM transportation into passels, projects each passel at the receiver's
known class and subscription, excludes the source, and rechecks delivery at
the callback boundary. It also supports object-instance Request Attribute Value
Update / Provide Attribute Value Update: it validates the requester's known
class, groups current owners, suppresses unowned and requester-owned callback
targets, preserves the tag, and rechecks a queued provider. The separate
response scenario covers explicit provider invocation of non-timestamped
`Update Attribute Values` from inside the callback and requester-side
`Reflect Attribute Values`; it does not imply RTI-automatic provision.
Regional attribute-value requests, timestamped/retraction behavior, and ensuing
timestamped or regional value updates remain outside this slice. The sibling
object-class path validates selected-class attributes and expands over current
registered instances at that class and its subclasses without requester
discovery. It also supports bounded `Query Attribute Ownership`: known-instance
attributes are grouped into joined-federate-owner and available-for-acquisition
reports, and queued reports are nullified at the removal boundary. It does not
invent an RTI-owned owner handle; that awaits the remaining ownership-transfer
lifecycle. The bounded `Attribute Ownership Acquisition If Available` path
retains a private willing-to-acquire request until its callback begins, then
assigns only attributes still unowned and sends the acquisition notification;
joined remote-owned attributes instead receive the unavailable callback. It
preserves an existing WTA state on repeat, admits eligible additional
attributes separately, and blocks unpublication of a required class attribute
until its terminal callback. It does not yet cover complete regular/negotiated acquisition, remaining divestiture flows,
RTI-owned state, or full resign-action ownership disposition.
The bounded regular acquisition path has separate pending state: it overrides
the same requester's WTA request for an overlapping attribute, transfers an
unowned attribute only at acquisition notification, and sends a release request
to a joined remote owner with the original tag. A release denial keeps that
owner and sends denial-tagged unavailable callbacks to matching regular
acquirers. A bounded regular cancellation moves a still-pending request into a
separate reservation, suppresses stale acquisition/release work, and retains
publication until grouped confirmation begins. Competing cancellation races,
negotiated acquisition, remaining divestiture flows, RTI-owned state, and full resign-action
ownership disposition remain outside the slice.

`compliance/resign-action-requirements-contract.json` and
`compliance/resign-action-api-contract.json` now record the bounded official
`ResignAction` transition. Six Catch2 cases cover directive 1
(`UNCONDITIONALLY_DIVEST_ATTRIBUTES`) leaving owned attributes unowned and
offering current eligible federates through `Request Attribute Ownership
Assumption`, directive 2 (`DELETE_OBJECTS`) removing objects for which the
resigning federate owns `HLAprivilegeToDeleteObject`, and directive 5
(`CANCEL_THEN_DELETE_THEN_DIVEST`) resolving the resigning federate's pending
acquisition work before cleanup. The cases also assert the official
`FederateOwnsAttributes` and `OwnershipAcquisitionPending` rejection paths,
plus the clause-4.12.4 rule that the final federate's `NO_ACTION` still
processes directive 2 and releases a delete-privileged object name.
Two additional cases cover the bounded 4.12.4 continuation triggers: a later
eligible publication and a later join followed by discovery. This remains
source/API traceability only: terminal-callback owner re-search and
arbitration, remaining automatic-resign directive combinations, RTI-owned
state, all remaining ownership forms, package/JUnit/protected-review evidence,
and conformance are not claimed.
The bounded `Attribute Ownership Divestiture If Wanted` path validates the
divesting owner across the entire supplied set, returns only attributes with a
selected already-pending regular or If Available acquirer, moves those
attributes synchronously, and forwards the divestiture tag to acquisition
notification. It keeps the selected acquirer's publication reservation until
the notification begins, invalidates former-owner work, and then replans later
regular acquisition work at the new owner. Its shared earliest-accepted
sequence is a deterministic development-profile policy only. Request Attribute
Ownership Assumption and unconditional divestiture are covered separately by a
bounded current-recipient path; negotiated divestiture, RTI-owned state, and
full resign-action disposition remain separate work.
The bounded `Unconditional Attribute Ownership Divestiture` path validates the
whole owner set, immediately removes those ownership records, and keeps
existing regular/If Available requests on their own standard callback paths.
It sends one grouped `Request Attribute Ownership Assumption` callback with
the divestiture tag to each currently eligible non-pending candidate. Callback
entry rechecks the candidate's known-instance, publication, pending-state, and
unowned conditions; the offer itself does not establish ownership. The registry
retains unowned search state and rechecks it after later join, discovery, or
publication changes, with duplicate-offer suppression. Terminal callback
re-search and full owner arbitration remain outside the slice.
The bounded `Negotiated Attribute Ownership Divestiture` path keeps the
current owner in private Waiting state, selects only an existing regular
acquisition request, and invokes `Request Divestiture Confirmation` with the
regular acquisition tag. `Confirm Divestiture` transfers a confirmed attribute
before a standard acquisition notification carrying the confirm tag; an
explicit cancel clears the waiting record and restores ordinary regular-release
planning. A cancelled selected regular request yields `NoAcquisitionPending`
and resets the owner to waiting. Normal release reservations remain while their
regular acquisition is pending, but a queued release is consumed when it
reaches negotiated Waiting state; that prevents a negotiated/cancelled race
from duplicating the owner callback. The profile does not yet search for further candidates or
select a willing-to-acquire candidate.
The adjacent `Is Attribute Owned By Federate` lookup is read-only and answers
only whether the invoking joined federate owns one valid known-instance
attribute. It also resolves only the mandatory 2025 `HLAreliable`
and `HLAbestEffort` transportation-type names and opaque handles for joined
federates. The embedded profile now implements bounded attribute default,
change, and query services plus interaction change and query services:
instance defaults are captured per federate, accepted changes commit at their
confirmation callback, and future ordinary/regional interaction sends and
attribute updates use the effective type. Custom transportation and general
message transport remain unimplemented. Joined federates start at the selected initial
value and can issue Time Advance Request; its state changes only as Time
Advance Grant is dispatched, and Query Logical Time observes that state. The same callback
boundary completes initial regulation
and constrained-mode requests, stores the enabled official lookahead, and makes
it observable through Query Lookahead. It also computes read-only GALT/LITS
from other regulators' temporal state and private queued/in-transit/delivered
TSO inputs, including the
exclusive zero-lookahead TAR boundary. A limited private scheduler retains TAR
requests and queues only those currently eligible under the GALT/NRG policy;
it rechecks the policy immediately before Time Advance Grant. The separate
timestamped-interaction contracts add the five bounded timestamped Send
Interaction, Update Attribute Values, Delete Object Instance, directed Send
Directed Interaction, and region-context Send Interaction paths; other
timestamped service families, the remaining alternate advance modes, and
general time-management support remain unimplemented.

compliance/test-catalog.json has one real embedded Disconnect integration test.
Its raw sidecar manifest reports one implemented binding and one unreviewed,
real passed evidence record. It is deliberately not verified. Check its
source/test traceability and regenerate raw evidence with:

~~~powershell
$python = 'C:\Users\peanu\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $python tools/requirements_lab.py check --contract compliance/connection-implementation-contract.json
cmake --build .build --target umbra_catch2_junit --config Debug
& $python tools/requirements_lab_sidecar.py raw --python $python --catalog compliance/test-catalog.json --results .build/compliance/catch2-results.xml
~~~

The aggregate C++ Connect mapping still has no selected surface in the Lab
sidecar request, so its real tests remain outside the catalog. The callback
control methods and development-profile federate/object-class/interaction-class/attribute/parameter/object-instance lookup, listing,
object discovery, and time reports have API-surface records but no Requirements-Lab implementation
mapping, catalog record, or public package support, so they remain outside the
catalog. The same is true of
the remaining development-profile federation-management scenarios: they remain
in `compliance/catch2-test-plan.json` until the service and packaging scope are
stable. The Lab JUnit adapter consumes already-produced test output; it does
not create or promote conformance evidence itself.
