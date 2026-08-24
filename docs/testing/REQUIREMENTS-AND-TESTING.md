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
[FOM stress-corpus backlog](../fom/FOM-STRESS-CORPUS-BACKLOG.md) tracks richer
model-family candidates independently. They are advisory stress inputs, never
substitute requirements or portable conformance evidence.

## Test authority and promotion order

The native C++ Catch2 suite is the authoritative implementation lane for the
RTI. Each behavior is first tied to the exact official IEEE 1516.1-2025 C++
signature and a focused native test; a passing Python, JPype, or Java test
cannot close that C++ requirement by itself. Those adjacent tests are valuable
harvest and bridge evidence: when a scenario is useful, it is translated into
the C++ API surface, added to the focused Catch2 lane, and only then mirrored
through the Java adapter to check the corresponding Java surface. This keeps
the C++ implementation independently testable and prevents adapter behavior
or a Python-side model from becoming an accidental standards substitute.

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
cmake -S . -B out/cmake/fom -G "Visual Studio 17 2022" -A x64 `
  -DUMBRA_FETCH_CATCH2=ON `
  -DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON `
  -DUMBRA_FETCH_LIBXML2=ON
cmake --build out/cmake/fom --config Debug
ctest --test-dir out/cmake/fom -C Debug --output-on-failure
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
`compliance/requirements-lab/fom-data-representation-requirements-contract.json` and its Catch2
scenario now also reject unresolved simple/enumerated representations and
ordinary reference-data representations that resolve to a basic or reference
type. The two standard instance identifier names are handled by a dedicated
exception rule requiring `HLAunicodeString` or `HLAobjectInstanceHandle` as
appropriate; its exact §4.14.9 candidate is pinned by
`compliance/requirements-lab/fom-reference-data-special-instance-identifiers-requirements-contract.json`.
The official MIM/Restaurant `HLAboolean` representation remains
accepted under the reviewed RL-009 interpretation rather than being rejected
by a generic basic-only predicate. The paired
`compliance/requirements-lab/attribute-parameter-data-type-requirements-contract.json` scenario
also rejects raw basic-data representations in object-attribute and
interaction-parameter data-type columns while retaining the standard
array-data `HLAtoken` case. The generic resolver retains basic-data names from
the schema's complete key so table-specific predicates report the applicable
category condition instead of an undeclared-name error. The separate
`compliance/requirements-lab/fom-attribute-na-companion-requirements-contract.json` scenario
adds the bounded direct companion rule: for an attribute whose data type is
`NA`, supplied transportation/order values must be non-`NA`, and supplied
update type/update condition values must be `NA`. It accepts a partial DIF row
that a later compatible module completes before FDD materialization, but does
not manufacture omitted values. The 2025 DIF schema has no per-attribute
available-dimensions representation, so the source phrase is not inferred as a
class-wide restriction; RL-051 records that mapping limit. This is private
traceability only.
The paired
`compliance/requirements-lab/fom-attribute-value-required-sharing-requirements-contract.json`
scenario adds the bounded `sharing=Neither` companion rule: a supplied
`valueRequired` field must be `false`; omitted fields remain representable for
a partial DIF attribute. It does not infer defaults or model declaration/runtime
state. RL-052 records the candidate's broad exported clause metadata. This is
private traceability only.
The paired
`compliance/requirements-lab/fom-attribute-dynamic-update-condition-requirements-contract.json`
scenario adds the bounded Dynamic Update Condition rule: when an attribute
supplies update type Conditional or Periodic and also supplies Update Condition,
the latter must be nonempty, non-NA text. An omitted condition remains
representable in a partial DIF attribute row. This does not prove the periodic
rate grammar or initial-condition prose. The distinct Static/NA source/example
tension remains deferred under RL-046; it does not affect this independent
predicate. This is private traceability only.
The paired `compliance/requirements-lab/fom-enumerated-representation-requirements-contract.json`
scenario additionally requires a supplied enumerated-data representation to
resolve to a basic-data declaration; it leaves omitted DIF fields and the
separate simple-data RL-009 interpretation unchanged. This is private
traceability only.
The paired `compliance/requirements-lab/fom-array-element-data-type-requirements-contract.json`
scenario requires a supplied array Element Type to resolve to a simple,
enumerated, reference, fixed-record, array, or variant-record declaration;
raw basic-data names fail after complete-model resolution. Omitted Element Type
remains representable for an incomplete DIF row. This is private traceability
only.
The paired `compliance/requirements-lab/fom-record-member-data-type-requirements-contract.json`
scenario applies that same bounded category rule to supplied fixed-record Field
Type and variant-record Alternative Type values. Raw basic-data names fail,
while omitted member types remain representable for incomplete DIF rows. The
Requirements Lab candidates retain broad `clause-4` metadata; RL-047 records
the verified source-provenance gap. This is private traceability only.
The paired `compliance/requirements-lab/fom-variant-discriminant-data-type-requirements-contract.json`
scenario requires a supplied variant-record Discriminant Type to resolve
specifically to an enumerated-data declaration. It permits an omitted field for
an incomplete DIF row, but rejects `NA` because the applicable 2025
variant-record rule does not provide that marker path. This is private
traceability only.
The paired `compliance/requirements-lab/fom-variant-discriminant-enumerator-requirements-contract.json`
scenario validates a supplied Discriminant Enumerator field's comma-separated
list and bracketed two-endpoint range grammar. `HLAother` is a standalone,
once-per-record marker; an omitted DIF field remains representable. It does not
by itself expand ranges or resolve discriminant membership; those semantics are
covered by the paired private scenarios below. This is private traceability
only.
The paired `compliance/requirements-lab/fom-variant-discriminant-enumerator-membership-requirements-contract.json`
scenario then requires each supplied individual discriminant enumerator and
each range endpoint to be declared by the selected enumerated type after
composition. It retains an incomplete DIF enumeration that supplies no members.
The paired range-semantics scenario below uses that resolved table membership.
RL-049 records the precise candidate's broad exported clause provenance. This
is private traceability only.
The paired `compliance/requirements-lab/fom-variant-discriminant-enumerator-range-semantics-requirements-contract.json`
scenario retains the enumerated table's declaration order, expands each
bracketed range over the inclusive span between its endpoints, and rejects a
member assigned through direct/range entries to more than one named alternative
after composition. `HLAother` is represented internally as the complement of
the explicit members. It retains an enumeration with no declared members as
incomplete DIF and does not invent a directional endpoint rule. RL-049 and
RL-050 record the Requirements Lab provenance limits. This is private
traceability only.
The paired `compliance/requirements-lab/fom-dimension-input-data-type-requirements-contract.json`
scenario requires each supplied Dimension-table Input data type to resolve to
the same table-defined declaration families; raw basic-data names fail, while
the `NA` marker remains valid. The paired
`compliance/requirements-lab/fom-dimension-input-data-description-requirements-contract.json`
scenario adds the bounded companion direction: an `inputDataTypes` sequence
with no named type requires non-`NA` description text. It accepts the official
empty-list representation and retains named-type rows with either `NA` or
amplifying descriptions. Neither predicate infers suitable-type selection or
the unambiguity of a supplied description. RL-048 records the broad exported
clause metadata. The additional
`compliance/requirements-lab/fom-dimension-input-data-type-na-exclusivity-requirements-contract.json`
scenario rejects an explicit `NA` marker mixed with a named input type, without
adding a suitability, cardinality, deduplication, or description-content
judgment. This is private traceability only.
The paired `compliance/requirements-lab/fom-root-hierarchy-requirements-contract.json` scenario
also rejects a nonstandard top-level object or interaction class after the
completed model merges, while leaving an omitted table representable for an
incomplete DIF module. It is private traceability only. The distinct
Static/NA direction remains deferred under RL-046 because the supplied 2025
Restaurant FOM uses Static with On change; Umbra does not reject the official
example through a homegrown interpretation. The separate bounded
Conditional/Periodic non-NA predicate remains active. Directed
interaction names are likewise resolved after the complete interaction
hierarchy is merged; this does not remove the separate official FDD-schema
cardinality limitation for the supplied extension example. The composed
catalog now also retains object-class, interaction-class, and class-directed-
interaction P/S declarations (and class semantics) as exact OMT metadata.
Those fields express model capability and Annex C merge identity; Umbra does
not use them to invent a per-federate declaration fence, whose current state
comes from the 1516.1 declaration-management services. Object and
interaction available-dimension references are resolved against the merged
dimension table, including a later provider module. Attribute and interaction
transportation names likewise resolve against the merged transportation table.
The paired `compliance/requirements-lab/fom-table-constraints-requirements-contract.json` and
its Catch2 scenario reject a supplied zero/non-positive update rate while
preserving incomplete DIF rows; the 2025 FOM XSD enforces positive dimension
upper bounds before composition. The separate
`compliance/requirements-lab/fom-dimension-default-value-requirements-contract.json` scenario
also checks integer and half-open dimension default ranges against
`[0, upperBound)` while permitting `Excluded`. Non-negative lookahead and
remaining table-specific checks are still open. RL-144 records that the
lookahead rule has no machine-readable DIF/XSD sign/domain predicate and that
the official MIM's signed `HLAinteger64BE` carrier makes a naïve signedness
check unsafe. The paired
`compliance/requirements-lab/fom-array-cardinality-requirements-contract.json` scenario now
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
composed FOM catalog retains each synchronization-table row's label, tag type,
capability, semantics, and remapped note references; the federation registry
continues to own live registration and achievement state. This is a private
projection and not a public synchronization-service or conformance claim. The
Annex C.5 composition path compares same-label synchronization points with the
first definition: an identical duplicate is ignored, while a duplicate with
any differing label, tag data type, capability, semantics, or note reference
fails before FDD materialization. This is exercised by the paired
`compliance/requirements-lab/fom-synchronization-merge-requirements-contract.json` Catch2 and
traceability lane and remains private preflight evidence rather than a public
synchronization-service or conformance claim. The Annex C.6 composition path
applies the same first-definition rule to transportation types: an identical
same-name duplicate is ignored, while a duplicate with any differing name,
semantics, or note reference fails before FDD materialization. This is
exercised by `compliance/requirements-lab/fom-transportation-merge-requirements-contract.json`
and its Catch2/traceability lane; it does not by itself implement transport
delivery. The embedded delivery foundation now resolves a declared custom
transportation to one execution-scoped handle and carries it through focused
ordinary, nonregional timestamped, and timestamped regional interaction
callbacks. Regional attribute, ordinary regional, directed, remote, and
conformance transport behavior remain separately scoped.
The
Annex C.7 composition path applies the same first-definition rule to update
rates: an identical same-name duplicate is ignored, while a duplicate with any
differing name, rate, semantics, or note sub-element fails before FDD
materialization. Omitted DIF rate/semantics fields remain representable for
later completion. This is exercised by
`compliance/requirements-lab/fom-update-rate-merge-requirements-contract.json` and its
Catch2/traceability lane; it does not implement update-rate scheduling. The
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
modules. The schema test includes missing and existing non-regular source
diagnostics, malformed XML, wrong namespace, and DTD rejection; the embedded
Create Federation Execution case proves that an existing unusable FOM path
maps to `ErrorReadingFOM` without committing a federation. The private
`compliance/requirements-lab/fom-source-diagnostics-requirements-contract.json` records the
broader Lab candidate and its source/test boundary, while
`compliance/requirements-lab/fom-source-diagnostics-api-contract.json` pins the exact C++
Create Federation Execution surface. This distinguishes a complete verified
2025 schema-resource set from the still-limited Annex C and runtime service
scope; details and the optional external-corpus policy are in
[FOM validation design](../fom/FOM-VALIDATION-DESIGN.md).

A dedicated complete-model lane now exercises the strict side of that policy:
`cpp/tests/data/strict-omt-complete-fom.xml` is independently DIF-validated,
materialized, and fed back through the official `IEEE1516-OMT-2025.xsd` with a
passing Catch2 result. The materializer carries the validated source
model-identification fields and inserts `Composed_From` references in the
schema-defined order. The packaged MIM+Restaurant composition remains a
separate negative because its Restaurant custom basic-data row is intentionally
partial and RL-009 records the official representation-reference tension; that
is not silently “fixed” by inventing encoding metadata.

The FOM composition contract also preserves the Annex C.8 switch rule: the
existing value wins for a repeated switch name, while an incompatible duplicate
produces private preflight warning data rather than invalidating the otherwise
valid composition. That warning is not a public service response or
Requirements-Lab evidence promotion.

The switch regression also covers the official XML-schema default: an omitted
`switchType/@isEnabled` is equivalent to an explicit `false`, regardless of
module order, so equivalent duplicates do not emit a misleading Annex C.8
warning. Automatic Resign Action remains intentionally separate because RL-024
records a Lab/XSD default conflict that still requires an edition decision.

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
`compliance/requirements-lab/support-switches-table-requirements-contract.json`,
`compliance/requirements-lab/support-switches-service-requirements-contract.json`, and
`compliance/requirements-lab/support-switches-api-contract.json` files pin the Lab records and
official declarations. A dedicated
`compliance/requirements-lab/convey-region-designator-callback-requirements-contract.json`
now traces the recipient-side callback policy: regional reflection and
interaction callbacks omit optional sent-region metadata while the switch is
disabled and carry the sent update-region realization while enabled. The new
`compliance/requirements-lab/delay-subscription-evaluation-requirements-contract.json` records
the bounded ordinary interaction and known-object attribute-update behavior of
the federation-wide Delay Subscription Evaluation switch: six Catch2 cases
distinguish enabled late subscription at receive-order and TSO-grant
eligibility from the disabled generation-time filter, and confirm callback-time
unsubscribe suppression in both switch settings. The four ordinary cases run
under both `HLA_EVOKED` and `HLA_IMMEDIATE`: the receive-order immediate
sections use callback disable/enable to create a real delivery boundary, while
the timestamped sections receive their eligible work with the direct Time
Advance Grant. The attribute cases first establish object discovery so they
test declaration timing rather than discovery timing. Two focused explicit-
source regional cases extend the timestamped path: an enabled federation
retains the regional recipient while unsubscribed and delivers after the
regional declaration is restored for both attribute and interaction passels,
while both modes suppress a later callback when that declaration is removed
before its grant. Asynchronous-delivery timing, directed interactions, and the
remaining lifecycle/retraction matrix remain outside this slice.
The separate Allow Relaxed DDM regression pair proves Umbra's intentional
implementation-specific policy for a regional interaction: exact
boundary-touching committed ranges are added only when the static FDD switch is
enabled, while a positive gap remains filtered and strict overlap is retained;
the object-attribute companion additionally covers discovery and reflection.
The existing support-switch contract continues to pin the exact official getter
requirement; the underlying §9.1.4/§9.1.8 delivery prose has no immutable Lab
candidate, so `../design/RELAXED-DDM-POLICY.md` and RL-030 retain direct-source
traceability without overstating that getter mapping.
It intentionally excludes regional interaction and explicit update-region
attribute forms, directed messages, HLA_IMMEDIATE/asynchronous, and
lifecycle-race matrices. This remains development-profile traceability only;
full connection-loss TSO-cutover semantics, generic MOM service-report/report-file
behavior, default/conveyed-region use, directed regional callbacks, broader
relaxed-DDM matrices, broader delayed-subscription lifecycle matrices, JUnit
promotion, protected review, and conformance remain open. RL-018 records the Delay Subscription
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
changes, and allocates a new one on rejoin. It appends the explicit Table 5
successful-void `[null]` file record for seven support services when the file
sink is selected: six Boolean setters (the four relevance/scope advisory
setters, `Set Convey Region Designator Sets Switch`, and `Set Exception
Reporting Switch`) plus `Set Automatic Resign Directive`. The service-report
foundation now has a separate `service-report-file-lifecycle` slice. Its production
filesystem regression performs a report-producing object-class lookup while
the reporting/file switches are enabled, verifies that disabling the file
switch leaves the single absolute path and file contents unchanged, and then
verifies that re-enabling resumes appends to that same file. The paired
`service-report-file-lifecycle-requirements-contract.json` and
`service-report-file-lifecycle-api-contract.json` pin the join/routing source
symbols and the official `setServiceReportingSwitch`,
`setSendServiceReportsToFileSwitch`, and `getObjectClassHandle` declarations.
This is native development-profile traceability only; it does not promote
Requirements-Lab validation, public MOM reflection, remote transport,
package/JUnit evidence, protected review, or conformance.
The focused DDM lane
also records paired failed `Create Region`/`Get Range Bounds` invocations with
their type-11 or type-42/type-10 supplied values, Null returns, false
indicators, exception text, and contiguous serials, and a native
`HLA_IMMEDIATE` case decodes the same service-type-5 interaction payload. The
focused DDM lane now adds source-backed non-void file records
for `Create Region` (type-11 `DimensionHandleSet` supplied and type-42
`RegionHandle` returned) and `Get Range Bounds` (type-42/type-10 supplied and
type-41 `RangeBounds` returned as a lower/upper record). Their one-element
Table 5 returned-argument arrays are covered by C++ unit and filesystem
integration tests; the Lab still lacks positive Table 5 row-level candidates
and conditional failure mappings, so this is development traceability rather
than validation or conformance. The adjacent DDM mutation lane now covers
failed `Commit Region Modifications`, `Delete Region`, and `Set Range Bounds`
with type-43/type-42/type-42/type-10/type-35 forms and serials zero through
three; the same HLA_IMMEDIATE case now also decodes successful
SetRangeBounds/CommitRegionModifications/DeleteRegion records with true
indicators, Null returns, and serials four through six. A
paired §9.10/§9.11 regional interaction-subscription failure lane now covers
invalid interaction-class and region-set inputs with type-27/type-43 (and
Subscribe's type-6 passive-subscription) forms in both the filesystem sink and
the HLA_IMMEDIATE MOM interaction. Disabled switches suppress records, while
successful regional Subscribe/Unsubscribe calls remain in the same serial
stream. RL-152 still prevents Lab validation or conformance claims. A
paired §9.8/§9.9 regional object-class subscription failure lane now covers
invalid object-class and attribute/region-pair inputs with type-36/type-4
forms, plus Subscribe's type-6 passive and type-53/type-34 update-rate slots,
in both the filesystem sink and HLA_IMMEDIATE MOM interaction. Disabled
switches suppress records, while successful regional Subscribe/Unsubscribe
calls retain serials four and five. RL-152 still prevents Lab validation or
conformance claims. A
paired §9.6/§9.7 regional object-attribute association failure lane now
covers invalid object-instance and region inputs with type-37/type-4 forms in
both the filesystem sink and HLA_IMMEDIATE MOM interaction. Disabled switches
suppress records, while successful Associate/Unassociate calls retain serials
four and five. RL-152 still prevents Lab validation or conformance claims. A
separate native case now proves one accepted ordinary
`SendInteraction` with Service Reporting enabled is emitted as the
RTI-originated seven-parameter `HLAreportServiceInvocation` to an eligible
observer whose own reporting switch is disabled. It checks reliable delivery,
service type 2, serial zero, encoded supplied/returned arguments, and
report-before-ordinary callback ordering. The producer remains the embedded
adapter's default-invalid `FederateHandle` under RL-043. A companion directed
case covers accepted untimed and timestamped `SendDirectedInteraction`
invocations through the same RTI-originated route; it checks serials zero and
one, reliable delivery, target callback fan-out, and the timestamped logical-
time callback argument. Generic file `ReturnArgument` formatting beyond this
bounded DDM slice (RL-042), broader regional/failure matrices, and broader
public MOM interaction families,
`HLAreportServiceFile` publication, the generic public MOM lifecycle, Lab
validation, and conformance remain open.

The bounded support-report lane now also covers the remaining §10.6--§10.12
non-void lookups. The filesystem records use the official Table 5/MIM forms:
type-37/type-36 object-instance/object-class handles, type-53 object-instance
and class-attribute names, type-0 attribute handles, and type-35 maximum
update-rate Numbers. A focused C++ unit test checks the exact encoded records;
the filesystem integration case performs all seven successful lookups after
switch-gated setup and verifies one immutable file with serials 0--6. The
Requirements Lab still exports only generic §11.5.1 service-report candidates,
not row-level positive `ReturnArgument` mappings for these forms (RL-151), so
this is development-profile source traceability rather than Lab validation or
conformance. A seven-case C++ failure matrix now covers the §10.6--§10.12
known-object, object-instance, attribute, and update-rate lookups, preserving
each supplied argument form, Null returned argument, false success indicator,
exception text, and contiguous serial sequence. The Lab does not export this
conditional failure relation as a row-level candidate (RL-152). An
HLA_IMMEDIATE public MOM interaction matrix now carries all seven failed
§10.6--§10.12 lookups with service type 6, their typed supplied arguments,
Null returns, false indicators, exception text, and serials zero through six
before a successful serial-seven lookup. Failures in other service families
and protected review remain open.

The adjacent §10.13--§10.16 interaction-class and parameter lookup failures
now have the same paired evidence. The filesystem matrix records the official
type-53/type-27/type-39 supplied forms, Null returns, false indicators,
exception text, and serials zero through three before a successful serial-four
lookup; the HLA_IMMEDIATE public MOM matrix decodes the same four forms through
the interaction sink. RL-152 still limits this to development-profile source
traceability, with other failure families, protected review, and conformance
open.

The neighboring §10.17--§10.20 order and transportation lookup failures now
have paired filesystem and HLA_IMMEDIATE evidence as well. Both matrices
preserve the official type-53/type-38/type-59 supplied forms, Null returns,
false indicators, public exception text, and serials zero through three before
a successful serial-four GetOrderType. Invalid OrderType uses the deterministic
type-38 UNSUPPORTED diagnostic because the closed Table 5 encoder has no
canonical invalid-enum spelling. RL-152 still limits this to development-profile
source traceability; other failure families, protected review, and conformance
remain open.

The same paired evidence now covers §10.21--§10.26 dimension and region
lookups. The filesystem and HLA_IMMEDIATE matrices preserve the official
type-36/type-27/type-53/type-10/type-42 supplied forms, Null returns, false
indicators, public exception text, and serials zero through five before a
successful serial-six GetDimensionHandle. RL-152 remains the conditional
failure-mapping gap; other service families, protected review, and conformance
remain open.

The paired failure lane now also covers §10.29--§10.33 handle normalization.
The filesystem and HLA_IMMEDIATE matrices preserve the official
type-50/type-15/type-36/type-27/type-37 supplied forms, Null returns, false
indicators, public exception text, deterministic `UNSUPPORTED` text for an
invalid ServiceGroup enum, and serials zero through four before a successful
serial-five NormalizeServiceGroup. RL-152 remains the conditional
failure-mapping gap; other service families, protected review, and conformance
remain open.

The RTI-initiated §6.9 `Discover Object Instance` callback now uses the same
recipient-local filesystem route. After its callback-time discovery recheck,
the selected file receives type-37 ObjectInstanceHandle, type-36
ObjectClassHandle, type-53 object name, and type-15 producing FederateHandle
text immediately before `discoverObjectInstance()` enters user code. The
focused HLA_EVOKED regression proves no record exists while discovery remains
queued, that the exact record is present at callback entry, and that a later
unsubscribe suppresses both callback and file append. This remains private
Table 5 file text, not public MOM interaction delivery. The separate public
MOM object-management regressions now prove the bounded RTI-owned discovery
path for active ordinary subscriptions and matching regional subscriptions,
including the reliable initial
`HLAreportServiceFile` reflection; it uses the local default-invalid producer
policy documented in RL-043 and is not conformance evidence.

The receive-order §6.17 `Remove Object Instance` callback now follows the
same recipient-local route. Its report is withheld while HLA_EVOKED delivery
is queued, then written immediately before `removeObjectInstance()` reaches
user code. The exact no-time record preserves type-37 ObjectInstanceHandle,
type-63 base-64 user tag, type-38 `RECEIVE` sent order, type-15 producer, and
three type-34 Null optional slots. The bounded timestamped callback forms now
retain the same route: a non-time-constrained recipient records
`TIMESTAMP`/`RECEIVE` at its immediate callback boundary, while a constrained
recipient records `TIMESTAMP`/`TIMESTAMP` before its TSO callback and matching
grant. Both include type-31 LogicalTime and, when supplied, type-33
MessageRetractionHandle. The focused filesystem regression proves the exact
record is durable before either user callback. It does not claim a timestamped
`Delete Object Instance` invocation record, public MOM interaction delivery,
or regional deletion reporting.

The same selected-filesystem path also records five successful no-argument
Time Management invocations: Disable Time Regulation,
Enable/Disable Asynchronous Delivery, and Enable/Disable Time Constrained.
Their supplied-argument lists are empty as §11.5.1 requires, while the
explicit successful-void [null] return form remains unchanged. Enable Time
Regulation records its one Lookahead as Table 5 LogicalTimeInterval type 32
with the quoted interval.toString() form. Modify Lookahead records its one
Requested lookahead in the same form when the request is accepted, even when a
lower actual value remains deferred. Time Advance Request, Time Advance
Request Available, Next Message Request, Next Message Request Available, and
Flush Queue Request each record one Logical time as Table 5 LogicalTime type 31
with the quoted time.toString() form when accepted. The first four reports
precede their later Time Advance Grant callbacks; the Flush Queue record
precedes its distinct actual/optimistic Flush Queue Grant. The two Next Message
Request reports and Flush Queue Request retain their supplied boundaries even
when queued TSO input produces an earlier grant. Both enable records are
written when their requests are accepted; their respective temporal roles
remain callback-gated. Retract records its one MessageRetractionDesignator as
Table 5 MessageRetractionHandle type 33 in the exact
`MessageRetractionHandle<decimal-identity>` form before its separate,
callback-gated Request Retraction consequence.
Synchronization Point Achieved records its accepted §4.17 Synchronization
point label as Table 5 String type 53 and its Optional
synchronization-success indicator as Boolean type 6. The defaulted C++ form
records its effective true value, while the explicit failed form records false.
The direct selected filesystem record is durable before the separately queued
Federation Synchronized callbacks. On completion, each recipient's own §4.18
file also receives its type-53 label and type-18 failed-federate set record
before that callback can be evoked; a rejected post-completion label appends no
successful-void record.
Register Federation Synchronization Point records its accepted §4.14
invocation, then the unified §4.15 confirmation report at the registering
federate before it queues either C++ registration-result callback. Its Table 5
request record carries type-53 Synchronization point label, the bounded
type-63 base-64 User-supplied tag, and the required third optional-set slot.
The two-argument C++ overload preserves that slot as type-34 Null; the
three-argument overload uses type-18 FederateHandleSet with quoted exact
`FederateHandle::toString()` values, including an explicitly supplied empty
`[]`. The confirmation record carries the label, type-6 Registration-success
indicator, and type-34 Null or type-56 SynchronizationPointFailureReason
Optional failure reason. This preserves argument presence even where omitted
and empty sets share global-registration semantics. §4.16 recipient
announcements now reserve and append through a weak joined-federate report
endpoint before their C++ callback is queued. The endpoint is attached at
Join, and either report-selection switch only gates later appends, so a
disable/re-enable cycle preserves the same report file and serial sequence.
The user-tag form remains private filesystem text under RL-077.
Federation Synchronized is likewise an RTI-created recipient service: the
completion route appends the type-53 Synchronization point label plus type-18
Set of joined federate designators before each selected recipient's callback.
The focused lane covers both ordinary all-achieved completion and completion
after an unachieved synchronization-set member resigns.
Request Federation Save records its accepted §4.19 invocation before any
separately queued Initiate Federate Save work. The two official C++ overloads
preserve the same Table 5 type-53 Federation save label and keep the optional
timestamp slot explicit: the no-time overload uses type-34 Null, while the
timestamped overload uses type-31 LogicalTime with the RTI-owned clone's
quoted `toString()` value. The focused test keeps the first request pending at
a time-constrained member, then replaces it with the valid time-regulating
timestamped request without evoking save callbacks. The Lab's §4.19 candidate
ownership/provenance remains covered by RL-078 and RL-002.
The ordinary queued §4.20 recipient path is separately covered: the requester
and a peer each append an independent selected-file `InitiateFederateSave`
record before their HLA_EVOKED callback, retaining the requested type-53 label
and type-34 Null optional timestamp. The unit companion fixes the type-31
timestamp form as well. Direct time-constrained pre-grant reporting remains a
separate lane; RL-082 records that the Lab's page-67 §4.20 candidates are
exported under §4.21.1.
Request Federation Restore records its normally returned §4.27 invocation
with the one type-53 Federation save label. The distinct RTI-initiated §4.28
Confirm Federation Restoration Request record then appends that label plus the
type-6 Boolean `Request-success indicator` to the requesting federate's
selected file before either result callback can be evoked. The focused lane
covers a real-snapshot true result and a missing-snapshot false result, which
does not start a restore operation. The Lab's direct §4.27 candidates have the
same coalesced-page owner/provenance limitation (RL-078/RL-002); the §4.28
candidates themselves have correct structural ownership.
Federation Restore Begun then appends the §4.29 Table 5 successful-void
no-argument form to every joined recipient's selected file, including the
requester, before either HLA_EVOKED callback can be evoked. The focused
two-recipient lane fixes the requester and peer serial sequences independently
around one real snapshot. Its source candidates are actual §4.29 text but have
the Lab's checker-required coalesced-page §4.29.3 owner (RL-078/RL-002).
Initiate Federate Restore then appends §4.30's successful-void report before
each recipient's HLA_EVOKED callback, using the recipient's existing file and
next serial. Its supplied arguments preserve the type-53 Federation save
label, type-15 joined federate designator, and type-53 federate name. The
focused two-recipient filesystem lane checks both independent report streams
and callback ordering. Its actual service text has the checker-required
coalesced-page §4.30.6 owner and stale display provenance (RL-078/RL-002).
Federate Restore Complete records §4.31's required type-6 Federate
restore-success indicator. The two official C++ selector calls map to that one
record: `federateRestoreComplete()` supplies true, while
`federateRestoreNotComplete()` supplies false. Each accepted record is durable
before its respective Federation Restored or Federation Not Restored callback;
a pre-restore completion appends no successful-void record. The focused
filesystem test also proves that the same joined-federate file advances serial
numbers through application-state restore rather than replaying the snapshot's
older counter. The Lab's direct §4.31 candidate is structurally owned by §4.32
and has stale title provenance, both recorded under RL-078/RL-002.
Abort Federation Restore records its accepted §4.33 no-argument invocation
with the Table 5 successful-void form before the ordinary `RESTORE_ABORTED`
Federation Not Restored callback. A rejected pre-restore abort appends no
record. This focused lane deliberately excludes the standard's special case
where all members have completed successfully and its final restore callback
may still indicate success. Its direct §4.33 candidates are structurally owned
by following §4.34 and retain stale title provenance, both recorded under
RL-078/RL-002.
Query Federation Restore Status records its accepted §4.34 no-argument
invocation with the Table 5 successful-void form before the distinct Federation
Restore Status Response callback supplies the descriptor vector. The focused
lane proves the recipient-local §4.35 record follows at the next serial before
that callback, using type-20 `FederateRestoreStatusSet` with public pre/post
handle text and the `NO_RESTORE_IN_PROGRESS` spelling. The null post handle is
the official C++ binding's `FederateHandle(invalid)` representation. A
`SaveInProgress` rejection appends no query record. Its direct candidates retain
the generic stale title provenance recorded under RL-002, but their structural
owner is correctly §4.34; the Table 5 collection-name/example conflict is
recorded under RL-083.
Resign Federation Execution now records its accepted §4.12 action as the final
service-report-file record for that joined federate. The source-backed
transaction reserves the serial only after its ordinary rejection paths have
cleared and before membership removal, allowing the adapter to append one
type-44 `HLAresignAction` record and then release its writer. Section 11.5.1
makes the argument's descriptive text implementation-dependent; Umbra uses the
matching standard-MIM spelling rather than inventing a separate vocabulary. The
focused filesystem lane proves an invalid action appends nothing, a successful
`NO_ACTION` resignation follows an earlier selected-file record at serial one,
and a second call cannot append to the completed lifetime file. A failing test writer also proves that an append
failure leaves the ambassador resigned before it surfaces `RTIinternalError`.
The distinct RTI-initiated §4.13 Federate Resigned path reserves its final
selected-file serial during successful clean member removal, appends its
type-53 `Reason for resigning` record, and only then delivers the official C++
callback. Its focused production-file lane proves that ordering and duplicate
suppression while retaining the connection for a later Join. The separately
implemented §4.4 Connection Lost lane instead reserves its final file serial
during forced loss cleanup, appends a type-53 `Fault description` record, and
then queues the best-effort callback before the old writer is released. Its
production-filesystem test proves ordering, duplicate suppression, and a
fresh-Connect cleanup boundary. It remains a distinct disconnected lifecycle,
not a Federate Resigned variant or a remote-transport claim.
Federate Save Begun records its accepted §4.21 transition with an empty
supplied-argument list and the explicit Table 5 successful-void `[null]`
returned-argument form. A rejected pre-initiation invocation appends nothing;
later save completion and Federation Saved remain separate service/callback
boundaries.
Federate Save Complete records §4.22's required type-6 Federate
save-success indicator. The two official C++ selector calls map to that one
record: `federateSaveComplete()` supplies true, while
`federateSaveNotComplete()` supplies false. Each accepted record is durable
before its respective Federation Saved or Federation Not Saved callback; a
pre-begun completion appends no successful-void record. The §4.22 Requirements
Lab source candidates have the coalesced-page ownership/provenance problem
documented under RL-078 and RL-002, so the direct service semantics remain
explicit in the implementation and test plan.
Federation Saved is the separate RTI-initiated §4.23 result record at every
selected recipient: normal completion writes a type-6 Federation save-success
indicator of true plus a type-34 Null optional failure reason, while failure
writes false plus the type-48 quoted `SaveFailureReason`. The record is durable
before the matching Federation Saved or Federation Not Saved callback. Its
focused production-filesystem lane covers a two-federate normal completion and
the ordinary `SAVE_ABORTED` result; the source candidates' checker-required
`clause-4.23.2` owner is the RL-078 coalesced-page defect, not a claim that the
service prose belongs to returned arguments.
Abort Federation Save records an accepted §4.24 invocation using the Table 5
successful-void no-argument form. A rejected no-save invocation appends no
record, and the selected report is durable before the ordinary `SAVE_ABORTED`
Federation Not Saved callback. This slice does not claim the standard's special
case where an abort follows all successful member completions and the final
Federation Saved result can still be successful.
Query Federation Save Status records an accepted §4.25 invocation using the
same Table 5 no-argument successful-void form before the separate Federation
Save Status Response callback supplies the joined-federate status vector. The
separate RTI-initiated §4.26 response now also records its type-17
`FederateHandleSaveStatusPairSet` before the HLA_EVOKED callback: each array
element uses Table 5's `handle`/`status` record with the public quoted values.
The focused lane starts one save only to demonstrate the
`FEDERATE_INSTRUCTED_TO_SAVE` form; it does not claim the full status matrix or
restore-in-progress behavior. The direct §4.26 Lab candidates are checker-owned
by §4.27.2 under the RL-078 coalesced-page defect, so the contracts retain their
immutable IDs while documenting the actual service association.
Change Attribute Order Type records three accepted supplied arguments: Object
instance designator as Table 5 type 37 with its quoted exact
`ObjectInstanceHandle::toString()` value, Set of attribute designators as type 1
with its bracketed array of quoted `AttributeHandle::toString()` values, and
Order type as type 38 with quoted `RECEIVE` or `TIMESTAMP`. The unsuccessful
unknown-object path does not add a successful-void report record.
Change Default Attribute Order Type records the same latter two forms alongside
Object class designator as Table 5 type 36 with its quoted exact
`ObjectClassHandle::toString()` value. It records the accepted prospective
class-default request; the unsuccessful invalid-object-class path does not add
a successful-void report record.
Change Default Attribute Transportation Type records the same type-36 Object
class designator and type-1 attribute-set forms together with Transportation
type as Table 5 type 59 using its quoted exact
`TransportationTypeHandle::toString()` value. It records the accepted
prospective class-default request; the unsuccessful invalid-object-class path
does not add a successful-void report record.
Change Interaction Order Type records two accepted supplied arguments:
Interaction class designator as Table 5 type 27 with its quoted exact
`InteractionClassHandle::toString()` value, and Order type as type 38 with
quoted `RECEIVE` or `TIMESTAMP`. The unsuccessful unpublished-class path does
not add a successful-void report record.
Request Interaction Transportation Type Change likewise records its accepted
Interaction class designator as type 27 and Transportation type as type 59,
each with the Table 5 quoted exact handle `toString()` form. The report is
written on accepted request; the separately queued confirmation remains the
transport-preference-change boundary, and an unpublished-class rejection adds
no successful-void record.
Request Attribute Transportation Type Change records its accepted Object
instance designator as type 37, Set of attribute designators as type 1, and
Transportation type as type 59. The two handle values use the Table 5 quoted
exact `toString()` form, and the attribute set is a bracketed array of quoted
`AttributeHandle::toString()` values. Its report is written on accepted request;
the separately queued confirmation remains the transport-preference-change
boundary, and an unknown-object rejection adds no successful-void record.
Query Attribute Ownership records its accepted Object instance designator as
type 37 and Set of attribute designators as type 1. The handle is quoted exact
`ObjectInstanceHandle::toString()` text and the attribute set is a bracketed
array of quoted `AttributeHandle::toString()` values. Its record precedes the
separately queued federate-owned and unowned ownership-result callbacks, while
an unknown-object rejection adds no successful-void record.
Unconditional Attribute Ownership Divestiture likewise records an accepted
Object instance designator (type 37), Set of attribute designators (type 1),
and its user-supplied tag as double-quoted base-64 Binary Data before the later
Request Attribute Ownership Assumption callback. Table 5 depicts that tag as
type 63, while the bundled official MIM assigns `UserSuppliedTag` type 60;
Umbra keeps the Table 5 literal isolated to the filesystem record and records
the unresolved source conflict in RL-077 rather than presenting it as a live
MOM interaction encoding.
Local Delete Object Instance records its accepted §6.18 Object instance
designator as type 37 using the quoted exact `ObjectInstanceHandle::toString()`
form after the invoking federate has forgotten the instance. An unknown-object
rejection appends no successful-void record, and this local state transition has
no later callback boundary.
The paired §6.16 receive-order `Delete Object Instance` failure matrices extend
that lane to the object-removal service's three supplied slots: type-37 object
instance handle, Table 5 type-63 base-64 user tag, and type-34 Null optional
timestamp. Failed unknown-object calls retain a Null return, false indicator,
exact `ObjectInstanceNotKnown` text, and serials zero and two around the
accepted serial-one deletion in both the filesystem and HLA_IMMEDIATE sinks.
The accepted record is emitted outside native locks before the queued removal
callback; timestamped deletion remains separate. RL-152 keeps this at C++
development traceability rather than Lab validation or conformance.
The timestamped §6.16 failure lane now covers the pre-admission failures that
are distinct from the accepted TSO sender interaction: an invalid object
designator and a timestamp below current logical time plus lookahead. Both
filesystem and HLA_IMMEDIATE records preserve type-37/type-63/type-31 supplied
forms, Null returns, false indicators, exact `ObjectInstanceNotKnown` and
`InvalidLogicalTime` text, and serials zero and one. The accepted sender file
record, retraction result, recipient-local §6.17 callback, and broader TSO
delivery remain separate work; RL-152 keeps this at C++ development
traceability rather than Lab validation or conformance.
The paired timestamped §6.10 `Update Attribute Values` failure matrices cover
the same pre-admission distinction for TSO object updates. Invalid object and
attribute designators and a timestamp below current logical time plus lookahead
preserve type-37/type-2/type-63/type-31 supplied forms, Null returns, false
indicators, exact `ObjectInstanceNotKnown`/`AttributeNotDefined`/
`InvalidLogicalTime` text, and serials zero through two in both filesystem and
HLA_IMMEDIATE sinks. The focused accepted-sender filesystem case now closes the
backend-selection gap: it proves the same type-37/type-2/type-63/type-31 forms,
the type-34 Null return for a non-time-regulating sender, serial zero, and
sender-report-before-`Reflect Attribute Values` ordering. Time-regulated
type-33 returns, regional/default-region updates, recipient-local callback
reports, and broader TSO behavior remain separate; RL-152 keeps this at C++
development traceability rather than Lab validation or conformance.
The paired timestamped §6.12 `Send Interaction` failure matrices cover the
same TSO pre-admission distinction for interactions. Invalid interaction-class
and parameter designators and a timestamp below current logical time plus
lookahead preserve type-27/type-40/type-63/type-31 supplied forms, Null
returns, false indicators, exact `InteractionClassNotDefined`/
`InteractionParameterNotDefined`/`InvalidLogicalTime` text, and serials zero
through two in both filesystem and HLA_IMMEDIATE sinks. Accepted sender output,
retraction, recipient delivery, and broader TSO behavior remain separate;
RL-152 keeps this at C++ development traceability rather than Lab validation
or conformance.
The paired timestamped §6.14 `Send Directed Interaction` failure matrices cover
the same pre-admission distinction for directed TSO sends. Invalid
interaction-class, target-object, and parameter designators and a timestamp
below current logical time plus lookahead preserve type-27/type-37/type-40/
type-63/type-31 supplied forms, Null returns, false indicators, exact
`InteractionClassNotDefined`/`ObjectInstanceNotKnown`/
`InteractionParameterNotDefined`/`InvalidLogicalTime` text, and serials zero
through three in both filesystem and HLA_IMMEDIATE sinks. Accepted sender
output, retraction, directed delivery, and broader TSO behavior remain
separate; RL-152 keeps this at C++ development traceability rather than Lab
validation or conformance.
Publish Object Class Attributes records its accepted §5.2 Object class
designator as type 36 using quoted exact `ObjectClassHandle::toString()` text
and its Set of attribute designators as type 1 using a bracketed array of
quoted `AttributeHandle::toString()` values. An invalid class appends no
successful-void record; an accepted record precedes separately queued
declaration advisories and ownership-assumption work. The Requirements Lab
currently owns the coalesced candidate as §5.2.4 rather than §5.2 (RL-078).
Unpublish Object Class Attributes records its accepted §5.3 Object class
designator as type 36 using quoted exact `ObjectClassHandle::toString()` text
and its Optional set of attribute designators as type 1 using a bracketed array
of quoted `AttributeHandle::toString()` values. An invalid class appends no
successful-void record; an accepted post-publication unpublish follows
synchronous ownership cleanup and precedes separately queued declaration
advisories. The Requirements Lab currently owns the coalesced candidates as
§5.3.3 rather than §5.3 (RL-078).
Subscribe Object Class Attributes records its accepted §5.8 Object class
designator as type 36 using quoted exact `ObjectClassHandle::toString()` text,
Set of attribute designators as type 1 using a bracketed array of quoted
`AttributeHandle::toString()` values, its standards-facing Optional passive
subscription indicator as type 6, and its explicit update rate as type 53
String. The C++ `active` selector is inverted for the passive indicator; an
empty/default update-rate selector occupies the optional position as type-34
Null. An invalid-class rejection appends no successful-void record, and an
accepted record precedes separately queued declaration, scope, relevance, and
discovery callbacks. The Lab assigns the continuation's §5.8 update-rate
candidates to §5.9; RL-080 records that mismatch.
Unsubscribe Object Class Attributes records its accepted §5.9 Object class
designator as type 36 using quoted exact `ObjectClassHandle::toString()` text.
The official C++ attribute-set overload supplies its Optional set of attribute
designators as a type-1 bracketed array of quoted `AttributeHandle::toString()`
values; the official whole-class overload retains that same required optional
argument position as type-34 Null. An empty supplied set remains distinct from
the whole-class Null form. An invalid-class rejection appends no
successful-void record, and either accepted record precedes separately queued
declaration, scope, and relevance callbacks. The Lab owns the coalesced source
candidates as §5.9.4 rather than §5.9 (RL-078).
The new `object-attribute-declaration-failure` lane covers the rejected and
accepted §5.2/§5.3 object-attribute declaration outcomes. Filesystem and
HLA_IMMEDIATE cases preserve type-36 ObjectClassHandle and type-1
AttributeHandleSet forms, Null returns, false indicators, exact exception
descriptions, and failure serials zero through three before accepted
Publish/Unpublish records at four and five. Accepted public MOM emission is
outside native locks; RL-152 keeps this C++ evidence at development
traceability rather than Lab validation or conformance.
The companion `object-attribute-subscription-failure` lane covers §5.8/§5.9
Subscribe/Unsubscribe Object Class Attributes, including subset and
whole-class unsubscription. Filesystem and HLA_IMMEDIATE cases preserve
type-36/type-1 forms, Subscribe's type-6 passive and type-53 update-rate
arguments, the whole-class type-34 Null slot, Null returns, false indicators,
exact exception descriptions, and failure serials zero through four before
accepted records at five through seven. Accepted public MOM emission remains
outside native locks; RL-152 keeps this at development traceability rather
than Lab validation or conformance.
The new `object-name-reservation-failure` lane carries the same paired
filesystem/HLA_IMMEDIATE evidence through §6.2 `Reserve Object Instance Name`
and §6.4 `Release Object Instance Name`. It preserves the type-53 `Name`
supplied value, Null returned argument, false indicator, exact
`IllegalName`/`ObjectInstanceNameNotReserved` exception descriptions, and
serials zero and two around accepted records at one and three. Accepted public
MOM emission remains outside native locks; RL-152 keeps this focused
object-management slice at development traceability rather than Lab
validation or conformance.
Publish Object Class Directed Interactions records its accepted §5.6 Object
class designator as type 36 using quoted exact `ObjectClassHandle::toString()`
text and its Set of interaction class designators as type 28 using Table 5's
bracketed `Array<InteractionClassHandle>` form of quoted exact
`InteractionClassHandle::toString()` values. Rejected object or interaction
class inputs append no successful-void record. A supplied empty set is a
successful no-op declaration and remains present as `[]`, not as an omitted
argument. The Lab assigns the coalesced source candidates to §5.6.3 rather
than §5.6 (RL-078).
Unpublish Object Class Directed Interactions records its accepted §5.7 Object
class designator as type 36 using quoted exact `ObjectClassHandle::toString()`
text. Its official C++ interaction-set overload supplies the Optional set of
interaction class designators as type 28 using the bracketed
`Array<InteractionClassHandle>` form; a supplied empty set remains `[]`. The
whole-class overload keeps that optional position as type-34 Null. Rejected
object or interaction class inputs append no successful-void record. The Lab
assigns the coalesced source candidates to §5.7.4 rather than §5.7 (RL-078).
The paired `directed-declaration-failure` lane now covers those rejected
inputs as failed service reports: filesystem and HLA_IMMEDIATE cases preserve
type-36/type-28 forms, the whole-class type-34 Null slot, Null returns, false
indicators, exact exception text, and serials zero through three before
accepted explicit-set records at four/five and a whole-class record at six.
The public MOM route is emitted outside native locks. RL-152 keeps this
overload matrix at development traceability rather than Lab validation.
Subscribe Object Class Directed Interactions records its accepted §5.12 Object
class designator as type 36, its Set of directed interaction designators as
type 28 using Table 5's bracketed `Array<InteractionClassHandle>` form, and its
Optional universal subscription indicator as type 6. The official C++ binding
represents that optional indicator as a defaulted `universally` Boolean, so the
report records the effective value: default by-ownership is lowercase `false`,
and an explicit universal selection is lowercase `true`. A supplied empty set
remains type-28 `[]`; rejected object or interaction class inputs append no
successful-void record. The Lab misassigns the §5.12 page-102 continuation to
§5.13 (RL-080), and §11.5.1's lowercase Boolean definition governs over the
uppercase Table 5 example (RL-079).
Unsubscribe Object Class Directed Interactions records its accepted §5.13
Object class designator as type 36. Its official C++ interaction-set overload
supplies the Optional set of directed interaction designators as type 28 using
the bracketed `Array<InteractionClassHandle>` form; a supplied empty set remains
`[]`. The whole-class overload keeps that optional position as type-34 Null.
Rejected object or interaction class inputs append no successful-void record.
The Lab misassigns the §5.13 postcondition continuation on page 103 to §5.14.3
(RL-080).
Publish Interaction Class records its accepted §5.4 Interaction class
designator as type 27 using quoted exact `InteractionClassHandle::toString()`
text after the declaring federate's publication is established. An invalid-class
rejection appends no successful-void record; the record precedes separately
queued declaration advisories.
Subscribe Interaction Class records its accepted §5.10 Interaction class
designator as type 27 using quoted exact `InteractionClassHandle::toString()`
text and its Optional passive subscription indicator as type 6. The C++
`active` selector is inverted for that standards-facing indicator; the output
uses §11.5.1's lowercase unquoted `true`/`false` definition rather than Table
5's uppercase Boolean example (RL-079). An invalid-class rejection appends no
successful-void record; the record precedes separately queued declaration
advisories.
Unpublish Interaction Class records its accepted §5.5 Interaction class
designator as type 27 using quoted exact `InteractionClassHandle::toString()`
text after the declaring federate's publication is removed. An invalid-class
rejection appends no successful-void record; the record precedes separately
queued declaration advisories.
Unsubscribe Interaction Class records its accepted §5.11 Interaction class
designator as type 27 using quoted exact `InteractionClassHandle::toString()`
text after an ordinary-unsubscription invocation. An invalid-class rejection
appends no successful-void record; the record precedes separately queued
declaration advisories.
The paired `declaration-interaction-failure` lane now covers the declared
failure outcome for §5.4/§5.5 as well: when reporting is enabled, invalid
handles append type-27 supplied arguments, Null returned arguments, a false
success indicator, and the public exception description in the same joined
federate serial stream. Its HLA_IMMEDIATE companion decodes declaration
service type 1 through `HLAreportServiceInvocation`; accepted Publish and
Unpublish calls follow at serials two and three. The lane is development
profile traceability only because the Lab still exports no row-level
conditional failure relation (RL-152).
The adjacent `declaration-subscription-failure` lane applies the same matrix
to §5.10/§5.11 Subscribe/Unsubscribe Interaction Class. Subscribe retains its
type-6 Optional passive subscription indicator (the inverse of the public
`active` selector), and the filesystem and HLA_IMMEDIATE cases both preserve
Null returns, false indicators, exact exception text, and serials zero/one
before accepted records at serials two/three. Accepted public MOM emission is
outside native locks; RL-152 likewise keeps this development-profile evidence
out of Lab validation and conformance claims.
The new `directed-subscription-failure` lane carries that evidence through
§5.12/§5.13 Subscribe/Unsubscribe Object Class Directed Interactions. It
preserves type-36 object-class and type-28 directed interaction-set arguments,
Subscribe's type-6 universal selector, and the whole-class Unsubscribe type-34
Null optional slot. Filesystem and HLA_IMMEDIATE cases retain Null returns,
false indicators, exact exception descriptions, and failure serials zero
through three before accepted records at four through six. Accepted public MOM
emission remains outside native locks; RL-152 keeps this C++ slice at
development traceability rather than Lab validation or conformance.

Separately, the bounded embedded transport-fault path plans
`HLAreportFederateLost` before automatic-resign cleanup and delivers it to
current ordinary or DDM-matching regional subscribers through the normal
receive-order `Receive Interaction` callback. It uses the MIM's `HLAfederate`,
`HLAfederateName`, `HLAtimeStamp`, and `HLAfaultDescription` parameters and
captures the lost time-regulating federate's current granted logical time.
The focused composite case enables asynchronous delivery for a time-constrained
survivor and verifies that this `HLAtimeStamp` is the same time-6 cutoff used
to deliver its queued timestamped application interaction before the matching
grant. It deliberately does not manufacture a special MOM exception to normal
receive-order temporal gating or assert an order between those two callbacks.
The ordinary and DDM-regional routes have direct `HLA_IMMEDIATE` regressions:
the matching subscribed survivor receives the report and the faulted federate
receives Connection Lost before the deterministic fault source returns, without
an Evoke call. The private `HLAfederate` routing point is not exposed as sent
regions. Its
default-invalid `FederateHandle{}` producer argument is Umbra's narrow local
adapter representation for an RTI-originated report, not a standard-defined
producer mapping; RL-065 records that boundary. This Catch2-backed slice now
establishes the page-50 inclusive cutoff with a strict-less-than and two
ordinary-interaction equal-to paths: a queued timestamp-5 interaction remains
deliverable through a publisher loss at time 6, while timestamp-6 interactions
reach a remaining constrained subscriber before its matching grant whether its
Time Advance Request was pending at the fault or arrives afterward. A directed
timestamp-6 counterpart proves that source resignation does not suppress an
already accepted target-specific payload when its target and recipient
selector remain valid. A reliable timestamp-6 attribute-update counterpart
retains its reflection value, source, timing, and retraction metadata through
the same loss. A paired two-survivor case retains the same passel and cutoff
marker after the first reflection/grant until the later requester crosses time
6. `DELETE_OBJECTS` collisions cover both a survivor already awaiting time 6
and one that requests time 6 after the loss: cutoff attribute reflections and
directed interactions retain the source/target object until their timestamped
callback and grant, after which their distinct receive-order cleanup releases
at the next eligible gate. A mixed two-survivor attribute case proves that
automatic cleanup remains independently gated per survivor. A callback-boundary
attribute or directed unsubscribe correctly suppresses its callback without
leaving a fake pending delivery that would strand the separately reserved
automatic removal. Timestamp-5 and
timestamp-6 object-deletion counterparts retain
their accepted removals through the source's unconditional-divest cleanup and
call the survivor before its matching time-6 grant. A `DELETE_OBJECTS`
counterpart confirms the automatic cleanup does not replace the accepted
timestamped removal with a receive-order callback or discard its cutoff state.
A staggered two-survivor companion proves the accepted object state and cutoff
marker remain after the first removal/grant until the later requester crosses
the same time-6 boundary. The registry captures the last-known position before forced
resignation and records a delivery-only marker; it does not treat ordinary
in-memory timing as a general no-regulated-grant override.
With asynchronous delivery enabled, bounded late-TAR attribute counterparts in
both callback models assert `Reflect Attribute Values < Time Advance Grant <
receive-order Remove Object Instance` within the same advance.
A timestamp after the cutoff is intentionally not asserted because the source
permits either result. Multi-recipient ordinary/direct interaction,
alternate-advance, later-timestamp, multi-passel/regional update, the remaining
timestamped-deletion/automatic-resign-action matrix,
remaining selector/lifecycle matrices under loss, remote transport, generic MOM callbacks,
JUnit/protected review, and
conformance remain open.

The focused `connection-lost-directed-selector-mutation` lane now covers the
directed half of that boundary: a queued timestamped directed interaction is
captured for a universal recipient, the source faults under `DELETE_OBJECTS`,
the recipient switches to by-ownership before its callback boundary, and the
directed callback is suppressed without stranding the independent automatic
object removal. RL-097 records that the Lab exports the selector and
Connection Lost/cleanup records separately. The paired
`connection-lost-regional-selector-mutation` lane now performs the same bounded
callback-boundary check for a timestamped regional attribute update: the
receiver commits a disjoint region after the source fault, suppressing only the
regional reflection while the independent automatic removal remains available.
RL-098 records that the Lab exports regional overlap, region mutation, and
Connection Lost/cleanup records separately; the remaining transport,
review, and conformance boundaries are still open.

The same successful production-profile Join now establishes an RTI-owned
joined-federate MOM snapshot behind the registry seam. It reserves a
common-namespace object identity, preserves all effective MIM attribute
metadata (including the inherited delete-privilege policy), captures a private
normalized `HLAfederate` point, and encodes the seven direct initial values
using official MIM types and the exact filesystem path. The first public
object-management slice now uses that ledger for active ordinary and
immutable-point-matching regional discovery, reliable reflection of all seven
required Table 8 initial values,
direct known-object requested-value reflection for the same complete initial
projection, and removal on represented-federate resignation. The regional
case filters against the immutable `HLAfederate` point and exercises matching
versus disjoint regions in both callback models. Switch changes preserve the
state and resignation removes it. The execution-scoped
`HLAmanager.HLAfederation` membership case now discovers the federation object
from a membership-only subscription, directly requests
`HLAfederatesInFederation` as the official nested `HLAfederateReferenceList`,
and proves Join/Resign conditional reflections at 1→2→1; the Java/JPype
companion covers the same bridge shape. A companion public regression covers
event-driven reflections of all nine predefined conditional switch attributes
after their successful setters and accepted `HLAsetSwitches` subset, plus four
bounded temporal-state attributes after successful role transitions and all
five time-advance request/grant forms, in both callback models; it also covers
direct known-object requests for their current encodings. It now also requests
the MIM-periodic `HLAlogicalTime` and `HLAlookahead` values directly, using the
selected official time-provider encodings even before a report period is set.
The companion MOM timing case also projects official `HLAGALT` and `HLALITS`
values from the same federation-owned GALT/LITS calculator used by Query GALT
and Query LITS, including the empty-array undefined form. A queued-TSO
companion projects `HLATSOlength` from the coordinator's recipient-scoped
queue, proving both direct and HLAsetTiming values before and after delivery.
A companion ownership projection supplies
`HLAobjectInstancesThatCanBeDeleted` from the live
`HLAprivilegeToDeleteObject` ledger, proving 0/1 around a real registration,
periodic reflection, and deletion.
A companion update-counter projection supplies `HLAupdatesSent` from an
 RTI-owned joined-membership counter advanced only after a successful
 `Update Attribute Values` service boundary, proving direct values 0/1/2 and
 periodic reflection of 2 after two accepted invocations. The companion
 `HLAobjectInstancesUpdated` projection retains distinct accepted object
 handles, proving repeated updates to one object remain at 1 while a second
 updated object raises the direct and periodic value to 2.
The common successful registration path also supplies
`HLAobjectInstancesRegistered`, proving direct 0/1/2 values and periodic 2
after two successful object registrations.
The deletion statistic now counts accepted receive-order deletion and
timestamped queue-admission boundaries, with direct 0/1/2 and periodic 0
before deletion and 2 after two deletions. The receiving federate's
`HLAobjectInstancesRemoved` counter now advances at committed no-time and
timestamped Remove Object Instance callback boundaries for ordinary
application objects; the focused C++ vector proves direct 0/1/2 and periodic
2 for two receive-order callbacks. The same MOM vector proves
`HLAobjectInstancesDiscovered` at 0/1/2 for two application-object callbacks,
then at 3 after Local Delete Object Instance and a repeated eligible
subscription rediscover the first object.
The interaction-send MOM companion now counts accepted sender-side
`sendInteraction` service invocations once at the C++ service boundary:
native Catch2 proves `HLAinteractionsSent` at 0/1/2/3/4/5/6 and
`HLAdirectedInteractionsSent` at 0/0/1/1/2/2 across ordinary, directed,
timestamped, regional, and timestamped-regional forms, with periodic
reflection at 6/2. DDM recipient fan-out is intentionally not a second
sender count.
The same native MOM case now verifies the receiver side through the peer's
RTI-owned MOM object: `HLAinteractionsReceived` advances at direct
0/1/2/3/4/5/6 and periodic 6, while `HLAdirectedInteractionsReceived` advances
at 0/0/1/1/2/2 and periodic 2. Each value is committed once immediately before
an accepted application Receive Interaction callback, independent of parameter
count or sender fan-out; RTI-originated MOM callbacks and suppressed callbacks
remain excluded. The Java/JPype companion remains the smaller direct 0/1/2 and
0/0/1 bridge vector.
A focused reflection-statistics companion now verifies the application-object
receive side: `HLAreflectionsReceived` counts accepted Reflect Attribute Values
callback invocations, while `HLAobjectInstancesReflected` retains distinct
object handles across repeated updates. Its direct vector is total/distinct
0/0, 1/1, 2/1, 3/2, and 4/2 after a queued timestamped reflection; periodic
`HLAsetTiming` returns 4/2. MOM-owned reflections are deliberately excluded,
and the queued TSO path uses the same callback-admission boundary.
A focused receive-order queue companion now verifies `HLAROlength` from the
recipient-scoped callback/deferred-receive ledger: direct values are 0 before
the send, 1 while one application callback remains queued, 1 through the
`HLAsetTiming` periodic reflection, and 0 after the target crosses its
callback boundary. The native case covers `HLA_EVOKED`; the Java/JPype
companion covers both callback models. The generic Lab candidate does not
encode the queue source or callback gate, so RL-108 records that refinement
gap.
A focused native MOM request/report case now consumes the Subscribe-only
`HLArequestObjectInstancesUpdated` interaction and emits one reliable
RTI-originated `HLAreportObjectInstancesUpdated` interaction. Its report
parameter is the official nested `HLAobjectClassBasedCounts` value, grouped
from the target joined-federate's accepted update ledger by registered object
class. The test proves repeated updates to one object remain one count for
that class, a second class is reported separately, the user tag is empty, the
producer is the default-invalid RTI value, and the callback remains gated by
the observer's `HLA_EVOKED` boundary. The report route rechecks target
membership and report subscription at callback time; RL-109 records that the
generic Lab candidate does not express this request/report correlation,
nested encoding, or producer route. Other HLArequest/HLAreport families,
transport-specific statistics, remote transport, package/protected review,
and conformance remain open.
A companion native MOM request/report case now consumes the Subscribe-only
`HLArequestObjectInstancesThatCanBeDeleted` interaction and emits one reliable
RTI-originated `HLAreportObjectInstancesThatCanBeDeleted` interaction. Its
class-grouped nested `HLAobjectClassBasedCounts` value is derived from the
target's live `HLAprivilegeToDeleteObject` ownership ledger rather than a
registration counter: two classes report initially, then deleting one object
removes only that class from the next response. The case proves the empty tag,
default-invalid RTI producer, target-point routing, and `HLA_EVOKED` callback
gate. RL-110 records that the generic Lab candidate does not express this live
ownership/request-report relation; the remaining public MOM request/report
families remain open.
A third native MOM request/report case now consumes the Subscribe-only
`HLArequestObjectInstancesReflected` interaction and emits one reliable
RTI-originated `HLAreportObjectInstancesReflected` interaction. Its nested
`HLAobjectClassBasedCounts` value is derived from the target's accepted
application reflection ledger, grouped by registered class, and retains one
count for repeated reflections of the same object. The case proves two classes,
empty tag, default-invalid RTI producer, target-point routing, and the
`HLA_EVOKED` callback gate. RL-111 records that the generic Lab candidate does
not express this distinct-object callback relation; the remaining public MOM
request/report families remain open.
A fourth native MOM request/report case now consumes the Subscribe-only
`HLArequestUpdatesSent` interaction and emits one reliable RTI-originated
`HLAreportUpdatesSent` interaction for each supported transportation type,
including an empty `HLAupdateCounts` NULL response. Its
`HLAtransportation` parameter identifies `HLAreliable` or `HLAbestEffort`,
while the nested `HLAupdateCounts` value groups accepted updates by registered
object class. The populated case proves two best-effort Server updates and one
reliable Soda update; the companion empty-ledger case proves both NULL buckets.
Reports have empty tags, default-invalid RTI producers, target-point routing,
and the `HLA_EVOKED` callback gate. RL-112 records that the generic Lab
candidate does not express the per-transport cardinality, accepted-update
boundary, nested encoding, or producer route; custom or remote transport and
remaining public MOM request/report families remain open.
A fifth native MOM request/report case now consumes the Subscribe-only
`HLArequestInteractionsSent` interaction and emits one reliable RTI-originated
`HLAreportInteractionsSent` interaction for each supported transportation type,
including an empty `HLAinteractionCounts` NULL response. Its
`HLAtransportation` parameter identifies the bucket, while nested
`HLAinteractionCounts` groups accepted sends by sent interaction class and
includes a dimensioned regional send. The populated case proves one reliable
TakeOrder, one best-effort TakeOrder, and one reliable regional MainCourseServed
count; the companion empty-ledger case proves both NULL buckets. Reports have
empty tags, default-invalid RTI producers, target-point routing, and the
`HLA_EVOKED` callback gate. RL-113 records that the generic Lab candidate does
not express regional inclusion, per-transport cardinality, accepted sender
boundary, nested encoding, or producer route; custom or remote transport and
remaining public MOM request/report families remain open.
A sixth native MOM request/report case now consumes the Subscribe-only
`HLArequestDirectedInteractionsSent` interaction and emits one reliable
RTI-originated `HLAreportDirectedInteractionsSent` interaction for each
supported transportation type, including an empty `HLAinteractionCounts` NULL
response. Its nested `HLAinteractionCounts` groups accepted directed sends by
sent interaction class, separate from the all-interactions sender ledger. The
populated case proves two reliable directed TakeOrder sends, while the
companion empty-ledger case proves the empty best-effort bucket. Reports have
empty tags, default-invalid RTI producers, target-point routing, and the
`HLA_EVOKED` callback gate. RL-114 records that the generic Lab candidate does
not express directed-subset accounting, per-transport cardinality, accepted
directed-send boundary, nested encoding, or producer route; custom or remote
transport and the remaining public MOM request/report families remain open.
A seventh native MOM request/report case now consumes the Subscribe-only
`HLArequestInteractionsReceived` interaction and emits one reliable
RTI-originated `HLAreportInteractionsReceived` interaction for each supported
transportation type, including an empty `HLAinteractionCounts` NULL response.
Its nested `HLAinteractionCounts` groups
accepted application receive callbacks by original sent interaction class and
transportation. The case proves one reliable and one best-effort TakeOrder
receive, empty tag, default-invalid RTI producer, target-point routing, and
the `HLA_EVOKED` callback gate. RL-115 records that the generic Lab candidate
does not express callback-boundary receipt semantics, per-transport
cardinality, nested encoding, or producer route; custom or remote transport,
directed-receipt reports, and the remaining public MOM request/report families
remain open.
A native directed-receive MOM request/report case now consumes the
Subscribe-only `HLArequestDirectedInteractionsReceived` interaction and emits
one reliable RTI-originated `HLAreportDirectedInteractionsReceived` interaction
for each supported transportation type, including an empty
`HLAinteractionCounts` NULL response. Its nested counts use the directed
receive ledger, separate from ordinary interaction receives. The case proves
that an ordinary `TakeOrder` receive is excluded, one reliable directed
`TakeOrder` callback is counted, the best-effort bucket is empty, and the
reports have empty tags, default-invalid RTI producers, target-point routing,
and the `HLA_EVOKED` callback gate. RL-116 records that the generic Lab
candidate does not express directed callback-boundary semantics, per-transport
cardinality, nested encoding, or producer route; custom or remote transport
and the remaining public MOM request/report families remain open.
A shared native sender-count regression now requests the three sender-side
MOM report classes against an empty ledger and proves two reliable
RTI-originated reports per family (`HLAreliable` and `HLAbestEffort`), each with
an empty official count array. It checks the report classes and parameters,
empty tags, default-invalid RTI producers, target-point routing, and the
`HLA_EVOKED` callback gate. RL-117 records that the generic Lab candidate does
not express correlated per-transport NULL cardinality; custom or remote
transport and the remaining public MOM request/report families remain open.
A ninth native MOM request/report case now consumes the Subscribe-only
`HLArequestReflectionsReceived` interaction and emits one reliable
RTI-originated `HLAreportReflectionsReceived` interaction for each supported
transportation type, including an empty `HLAreflectCounts` NULL response. Its
official `HLAobjectClassBasedCounts` value groups accepted application
reflection callbacks by registered object class and effective transportation.
The case proves one reliable and one best-effort reflection, then both empty
transport buckets for a joined federate with no reflections; reports have
empty tags, default-invalid RTI producers, target-point routing, and the
      `HLA_EVOKED` callback gate. RL-118 records that the generic Lab candidate does
      not express callback-boundary reflection semantics, per-transport cardinality,
      nested encoding, or producer route; custom or remote transport and the
      remaining public MOM request/report families remain open.
      A tenth native MOM request/report case now consumes the Subscribe-only
      `HLArequestObjectInstanceInformation` interaction and emits one reliable
      RTI-originated `HLAreportObjectInstanceInformation` response through the
      requesting federate's private `HLAfederate` point. The official nested
      `HLAattributeHandleList` is decoded to prove the known-versus-NULL
      parameter shape, registered/known class values, and the registering
      federate's `Efficiency` plus implicit `HLAprivilegeToDeleteObject`
      ownership. RL-119 records that the generic Lab candidate does not express
      these object-state, ownership, nested-encoding, endpoint, or producer
      semantics; the remaining public MOM request/report families remain open.
A native MOM request/report case now consumes the Subscribe-only
`HLArequestPublications` interaction and emits the three required reliable
RTI-originated publication reports: one interaction publication report, one
object-class publication report per published class, and one directed-
interaction publication report per directed-publication class. It decodes the
official nested attribute/interaction handle lists, proves implicit
`HLAprivilegeToDeleteObject` publication, checks the distinct MIM NULL shapes
after unpublication, and verifies empty tags, default-invalid RTI producers,
private target-point routing, and the `HLA_EVOKED` callback gate. RL-120
records that the generic Lab candidate does not express three-report
cardinality, publication snapshots, directed grouping, NULL parameter shape,
nested encoding, endpoint, or producer semantics; subscription/FOM-module and
other statistical MOM request/report families remain open.
A twelfth native MOM request/report case now consumes the Subscribe-only
`HLArequestSubscriptions` interaction and emits reliable object-class,
interaction, and directed-interaction subscription reports. It decodes
active/passive object-class groups with their maximum `HLAupdateRateName`, the
official nested `HLAattributeHandleList` and `HLAinteractionSubList`, and the
local official-MIM directed report shape. The case also verifies ordinary and
directed subscription lists, all three NULL responses after unsubscription,
private target-point routing, empty tags, default-invalid RTI producers, and
the `HLA_EVOKED` callback gate. RL-122 records that the generic Lab candidate
does not express the correlated report family, regional DDM relation, nested
interaction records, NULL forms, endpoint, producer route, or the local-MIM /
semantic-reconstruction `HLAuniversal` discrepancy. A thirteenth native MOM
request/report case now consumes the Subscribe-only
`HLArequestFOMmoduleData` interaction and emits a reliable
`HLAreportFOMmoduleData` response through the reported federate's private
`HLAfederate` point. It verifies Join-time retention of validated canonical
module content in first-designator order, official `HLAinteger32BE` and
`HLAunicodeString` encodings, reliable transport, empty tag, default-invalid
producer, callback-time endpoint revalidation, and deterministic invalid-index
rejection. RL-124 records that the broad Lab content-access candidate does not
express this federate-scoped request/report contract; federation-level
FOM/MIM/current-FDD access remains open. A fourteenth native MOM
request/report case now consumes the dimensionless federation-scoped
`HLArequestFOMmoduleData` and `HLArequestMIMdata` interactions and emits
reliable `HLAreportFOMmoduleData` and `HLAreportMIMdata` broadcasts to current
subscribers. It verifies retained validated FOM/MIM content, official
`HLAindex`/`HLAunicodeString` encodings, empty tags, default-invalid RTI
producers, callback gating and callback-time subscription revalidation, strict
MIM no-parameter handling, unexpected-parameter rejection, and synchronous
invalid-index rejection. RL-124 now covers both bounded content-report
relations; federation-scoped current-FDD reporting remains open. A fifteenth
native MOM request/report case now consumes the dimensionless
`HLArequestSynchronizationPoints` and
`HLArequestSynchronizationPointStatus` interactions and emits reliable
`HLAreportSynchronizationPoints` and
`HLAreportSynchronizationPointStatus` broadcasts. It decodes the official
`HLAsyncPointList` and `HLAsyncPointFederateList` variable/fixed-record values,
derives `MovingToSyncPoint` and `WaitingForRestOfFederation` from the retained
achievement ledger, verifies empty-array NULL responses for unknown and
completed points, checks empty tags/default-invalid RTI producers and the
`HLA_EVOKED` gate, revalidates subscriptions at callback time, and rejects
missing or unexpected request parameters. RL-125 records that the Lab's
independent MOM table candidates do not encode this request/report relation;
remote transport, JUnit/protected review, and conformance remain open.
A sixteenth native MOM request/report case now covers the subscription-selected
`HLAreportMOMexception` interaction for malformed or precondition-rejected MOM
`Send Interaction` requests. The bounded empty-parameter `HLAsetSwitches`
request preserves the caller's `InteractionParameterNotDefined` and emits an
RTI-originated reliable report with the fully qualified MOM interaction name,
exception text, and `HLAparameterError=true`; a well-formed
service-reporting adjustment rejected by its report-subscription precondition
emits `HLAparameterError=false`. Both target the rejected sender's private
`HLAfederate` point. The native case verifies default-invalid producer, empty
tag, `HLA_EVOKED` gating, and callback-time unsubscribe suppression. Ordinary
application interaction failures remain on `HLAreportException`; generic
`HLAservice` spoofing, other malformed MOM families, remote transport,
JUnit/protected review, and conformance remain open.
A seventeenth focused native MOM object case now exposes the federation-scoped
`HLAcurrentFDD` attribute as an official `HLAunicodeString` carrying the
materialized, schema-validated current FDD. It discovers the RTI-owned
`HLAfederation` object, requests the composed base FDD, observes a reliable
conditional refresh after a compatible additional FOM Join contributes
`UmbraReferenceFixtureClass`, and verifies that a direct request returns the
same refreshed value with default-invalid RTI producer, no regions, and an
empty tag. RL-127 records that the Lab's cross-cutting current-FDD candidate
does not model the MOM attribute shape or Join-triggered lifecycle; module/FDD
access matrices, remote transport, JUnit/protected review, and conformance
remain open.
A focused federation-scoped MOM save-conditional case now verifies the empty
initial `HLAnextSaveName`/`HLAnextSaveTime` and `HLAlastSaveName`/
`HLAlastSaveTime` values, a pending timestamped request using the official
`HLAunicodeString` and `HLAinteger64Time` encodings, the reliable two-attribute
reflection that clears the next values when Initiate Federate Save is admitted
at the constrained time-advance boundary, and the completion reflection that
updates the last values only after all joined federates complete the snapshot.
The native case checks default-invalid RTI producer metadata, no regions, an
empty tag, and all three joined participants. RL-128 records that the Lab's
generic MOM candidate does not model this save-ledger attribute lifecycle.
The official 2025 federation MOM has no separate restore-name/time
conditionals; the remaining open work is restore-operation semantics, remote
transport, JUnit/protected review, and conformance.
A focused `HLAsetTiming` companion now validates the official
`HLAfederateReference`/`HLAseconds` pair, rejects a negative period without
mutation, and drives one catalog-declared periodic reflection after a
wall-clock deadline at an `HLA_EVOKED` callback boundary; zero disables later
reflections. A companion HLA_IMMEDIATE case uses a thread-safe observer and
proves the same reflection arrives without an Evoke call, then stops after
HLAreportPeriod=0. The private Catch2 assertion remains the stronger
identity/metadata check. The duration pair `HLAtimeGrantedTime` and
`HLAtimeAdvancingTime` now has a native Catch2 companion that verifies direct
known-object AVU and consume-once HLAsetTiming reflection using official
`HLAinteger32BE` HLAmsec encodings in both callback models; the JNI/JPype
companion supplies the bridge evidence. Remaining periodic/other conditional
scheduling
 (including traffic/statistical values beyond the
 ownership-backed deletable-object, successful-update-count, updated-object-count,
 registered-object-count, deleted-object-count, removed-object-count,
 discovered-object-count, reflection-count, interaction-send, and interaction-receive
 projections),
  optional/inherited non-initial attributes, broader public MOM interactions
  beyond the bounded service-report invocation and the
  `HLArequestPublications`/`HLAreportObjectClassPublication` plus
  `HLAreportInteractionPublication` and
  `HLAreportDirectedInteractionPublication` and
  `HLArequestSubscriptions`/`HLAreportObjectClassSubscription` plus
  `HLAreportInteractionSubscription` and
  `HLAreportDirectedInteractionSubscription` and
  `HLArequestFOMmoduleData`/`HLAreportFOMmoduleData` and
  federation-scoped `HLArequestMIMdata`/`HLAreportMIMdata` and
  `HLArequestSynchronizationPoints`/`HLAreportSynchronizationPoints` and
  `HLArequestSynchronizationPointStatus`/
  `HLAreportSynchronizationPointStatus` and
  `HLArequestObjectInstanceInformation`/`HLAreportObjectInstanceInformation`,
  `HLArequestObjectInstancesUpdated`/`HLAreportObjectInstancesUpdated` plus
`HLArequestObjectInstancesThatCanBeDeleted`/
`HLAreportObjectInstancesThatCanBeDeleted` and
`HLArequestObjectInstancesReflected`/
`HLAreportObjectInstancesReflected` and
`HLArequestUpdatesSent`/`HLAreportUpdatesSent` cases,
plus `HLArequestInteractionsSent`/`HLAreportInteractionsSent`,
`HLArequestDirectedInteractionsSent`/`HLAreportDirectedInteractionsSent`,
`HLArequestInteractionsReceived`/`HLAreportInteractionsReceived`,
  `HLArequestDirectedInteractionsReceived`/
  `HLAreportDirectedInteractionsReceived`,
and the
source-backed callback producer-designator mapping remain open, so neither
lane is conformance evidence. A save/restore companion now drives the official
`HLAfederateState` values (1/3/5) from the real operation ledgers at the
corresponding callback boundaries and verifies both callback models. It also
checks that save initiation suppresses the event reflection at the saving
federate itself without weakening direct known-object AVU. This is still a
bounded development-profile projection rather than full MOM conformance.

The same support-switch contract now traces the bounded joined-federate
`HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches` control path. A non-empty
parameter subset updates only the sending member's nine predefined switch
values with strict standard encodings; the regression proves peer isolation,
invalid resign-action rejection, and the report-service interlock's atomic
state guard. It also proves that a FOM-added parameter and a compatible
extension subclass are received, while only inherited predefined values are
processed. The current local error is deliberately not represented as a normal
MOM failure-report interaction, and this does not add MOM objects, reports,
or report-file behavior. The public joined-federate MOM companion now
re-reflects each changed predefined switch value from both individual setters
and the accepted `HLAsetSwitches` subset, while leaving periodic/other
conditional scheduling separate. RL-032 records the Requirements Lab's missing
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
`compliance/requirements-lab/handle-normalization-api-contract.json` and
`compliance/requirements-lab/handle-normalization-requirements-contract.json` files record
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

`compliance/fom/external-2025-fom-corpus.json` pins four external modules by
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
mandatory and composed-FDD transportation-type name/handle lookup, limited receive-order
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
`compliance/requirements-lab/order-type-api-contract.json` and
`compliance/requirements-lab/order-type-requirements-contract.json` files keep this source/API
traceability distinct from validation or conformance evidence. The adjacent
`compliance/requirements-lab/order-type-control-api-contract.json` and
`compliance/requirements-lab/order-type-control-requirements-contract.json` contracts now pin
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
(`compliance/requirements-lab/tso-message-queue-requirements-contract.json` and
`compliance/requirements-lab/federation-time-coordination-tso-requirements-contract.json`)
check those source/test anchors against the pinned Requirements Lab bundle.
The first bounded public consumer is traced separately by
`compliance/requirements-lab/timestamped-interaction-requirements-contract.json`: the official
non-regional timestamped Send Interaction overload, Retract, and
MessageRetractionHandle decoding. Its companion
`compliance/requirements-lab/request-retraction-requirements-contract.json` traces the bounded
post-delivery normal interaction, region-context interaction, normal and
bounded regional attribute-update, and directed-interaction paths: strict
Retract eligibility, Request Retraction for an already-delivered recipient,
and suppression of still-queued fanout. Its current lifecycle slice also
keeps a lightweight terminal designator after the strict boundary has passed,
so queued typed delivery can drain while later `Retract` calls report
`MessageCanNoLongerBeRetracted`; timestamped deletion state and an object name
are released only after that terminal state has no pending recipient.
The separate
`compliance/requirements-lab/timestamped-attribute-update-requirements-contract.json` traces
the non-regional timestamped Update Attribute Values / Reflect Attribute
Values path, including recipient-specific transportation passels. Their
Catch2 scenarios cover lower-bound validation, retraction before grant,
exact-bound callbacks before Time Advance Grant, TARA/NMRA alternate-advance
boundaries, FQR delivery with actual and optimistic grant values, and an
HLA_EVOKED future-input interaction case whose messages arrive after FQR
submission but before callback dispatch. The private FQR grant calculator also
includes explicit in-transit payloads in its undelivered frontier. These are
in-process boundaries; remote future transport is not implied.
timestamp/order/retraction fields, sender exclusion, pending fanout
suppression, and a two-passel immediate attribute recipient followed by
Request Retraction. The exact 2025 source, rather than a fragment-only Lab candidate,
supplies the strict time-plus-actual-lookahead interpretation; RL-019 records
the export limitation.
The paired `compliance/requirements-lab/timestamped-regional-attribute-update-requirements-contract.json`
and `compliance/requirements-lab/timestamped-regional-attribute-update-api-contract.json` now
trace the bounded regional extension. Its Catch2 scenario commits update
regions, queues a timestamped reflection for a time-constrained recipient,
proves retraction before the grant, and rechecks the recipient's Convey Region
Designator Sets switch so the optional sent-region callback metadata is absent
when disabled and present when enabled. The same Catch2 case also proves
mixed immediate/TSO fanout: the non-time-constrained recipient receives its
timestamped callback immediately while the constrained recipient remains
queued until its grant. The same scenario proves `Request Retraction` for the
delivered immediate recipient and suppresses the pending constrained fanout.
A focused nonconstrained companion begins with strict regional overlap, commits
the recipient range to `[2, 3]` before callback dispatch, and proves that the
current projection suppresses the reflection, terminally closes its recipient
ledger state, and emits no `Request Retraction` after a still-legal `Retract`.
A time-constrained companion queues the same initially eligible passel, makes
the region disjoint before time 6, and receives only the time-6 grant; the
producer's advancing boundary then correctly makes its designator no longer
retractable without inventing a retraction callback.
A focused class-level regional Request/Provide companion now has the provider
invoke timestamped `Update Attribute Values` from the official callback. It
queues the overlap-qualified response for a time-constrained requester,
preserves the request tag, timestamp, order, producer, and source RegionHandle,
and proves one reflection before the matching grant with a valid retraction
handle. This is a timestamped provider-response composition; automatic
provision, alternate advances, and broader regional request/retraction behavior
remain separate slices.
A focused ordinary TAR/NMR companion drives an explicit-source
overlap-qualified attribute passel through both grant frontiers at the
inclusive timestamp-7 boundary and preserves the source-region metadata before
each grant.
A mixed default-source/default-region companion now drives one ordinary object
update through FQR, TARA, and NMRA recipients. Each reflection precedes its
grant; FQR reports actual/optimistic time 7, the producer TAR completes at 2,
and Convey Region Designator Sets exposes the supplied-empty marker because no
public source RegionHandle exists. Remaining timestamped default-region/
re-enable behavior, additional regional request edges, and alternate advance
modes beyond these bounded FQR/TARA/NMRA cases, transport,
package/JUnit/protected-review evidence, and conformance remain outside this
slice.
A focused explicit-source association-replacement companion now queues a
timestamp-6 passel under `sourceRegionA`, unassociates that region and associates
`sourceRegionB` before the constrained grant, and proves the queued passel is
suppressed rather than retargeted. A later timestamp-7 update is delivered with
`sourceRegionB` in the conveyed callback metadata. This is a bounded callback-
time replacement case; replacement semantics for the remaining advance,
transport, lifecycle, and persistence modes remain open.
A matching mixed default-source interaction companion now drives one ordinary
timestamped Send Interaction through FQR, TARA, and NMRA. Each Receive
Interaction callback precedes its grant; FQR reports actual/optimistic time 7,
the producer TAR completes at 2, and the callback carries the supplied-empty
region marker. A focused companion now disables and callback-gated re-enables
Time Constrained while that default-source interaction is queued, then proves
one callback before the matching grant with no duplicate delivery. Retraction,
explicit-source interaction replacement while queued, and the remaining alternate/
 re-enable matrix remain open.
The corresponding ordinary non-regional companion uses the official
`subscribeInteractionClass` surface and drives one timestamp-5 interaction
through independent FQR, TARA, and NMRA recipients. Each `Receive Interaction`
callback is observed before its matching grant; FQR preserves actual and
optimistic time 5, the producer TAR completes at 4 after lookahead admission,
and timestamp/order/tag/producer metadata is checked. This is one focused
in-process frontier, not a claim that every available-advance, transport,
save/restore, or conformance case is complete. A separately contracted
ordinary lifecycle companion now queues one non-regional interaction across a
Time Constrained disable and callback-gated re-enable, then proves exactly one
callback before the matching grant with the original metadata and terminal
retraction classification. This isolates joined-federate queue lifetime from
DDM/source-region behavior; broader re-enable and alternate-advance matrices
remain open.
The matching explicit-source regional object-update companion queues an
overlap-qualified Update Attribute Values passel, disables and callback-gated
re-enables Time Constrained before its grant, and proves one reflection before
the grant with the original source RegionHandle, timestamp, order, tag, and
Convey Region Designator Sets metadata intact. This closes only that focused
composition; alternate advances, replacement, relaxed-DDM, and transport remain
separate work.
A focused untimed save/restore companion now preserves one queued explicit-
source regional attribute passel, its object/update association, source
RegionHandle set, and live retraction designator through restore. Flush Queue
Request proves the original reflection before the grant, and a later legal
Retract issues Request Retraction from the restored designator. RL-137 records
the Requirements Lab's missing regional attribute save/restore relation;
timed/durable restore, region mutation, passive/relaxed-DDM variants, and
broader recovery remain open.
A matching default-source interaction companion now saves one queued
timestamped Send Interaction whose private default realization overlaps a
committed receiver region, terminalizes the pre-save designator, restores the
payload and recipient ledger, and proves supplied-empty callback region
metadata before Flush Queue delivery and Request Retraction. RL-138 records
the corresponding Requirements Lab default-source save/restore relation gap;
timed/durable restore and broader recovery remain open.
A matching non-regional object-update companion now saves one queued
timestamped Update Attribute Values passel and its ordinary recipient ledger,
terminalizes the pre-save designator, restores the typed payload, and proves
Flush Queue reflection followed by Request Retraction from the original
designator. RL-139 records the corresponding non-regional attribute recovery
relation gap; timed/durable restore and broader recovery remain open.
A matching default-source object-update companion now saves one queued
timestamped passel whose private default source overlaps a committed receiver
region, restores the payload and recipient ledger, and proves supplied-empty
sent-region metadata before Flush Queue reflection and Request Retraction.
RL-140 records the corresponding default-source attribute recovery relation
gap; timed/durable restore and broader recovery remain open.
The timed counterpart now schedules the save at logical time 6 while the
timestamp-8 default-source passel remains queued. The constrained receiver and
regulating publisher cross that save boundary, complete the save, and then
restore the post-save-terminalized designator before Flush Queue reflection at
the original metadata and a later Request Retraction. RL-141 records the
Requirements Lab's missing save-boundary relation; durable restore, other
advance modes, other payload families, and broader recovery remain open.
`compliance/requirements-lab/timestamped-object-deletion-requirements-contract.json` and its API
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
lookahead. A positive mixed-advance companion now drives one timestamped
deletion through FQR, TARA, and NMRA, proving callback-before-grant ordering
and preserving FQR actual/optimistic time plus producer TAR completion.
The separately contracted ordinary lifecycle companion now queues one
timestamped deletion across a Time Constrained disable and callback-gated
re-enable, then proves exactly one Remove Object Instance callback before the
matching grant with the original object, tag, timestamp, order, producer, and
terminal retraction classification. Alternate-advance coverage beyond the
positive companion, broader re-enable, active in-flight ownership, other
resignation, and save/restore recovery evidence, transport, and conformance
remain outside this bounded slice. A separately contracted save/restore case
also saves that live queued deletion, makes the designator terminal after the
save, restores the snapshot, delivers the original removal through Flush Queue
Request, and then proves post-delivery Request Retraction reconstitutes the
object and name. Timed/durable restore and changed-membership or ownership
recovery remain open.
`compliance/requirements-lab/timestamped-directed-interaction-requirements-contract.json` and
its API companion add the bounded non-regional timestamped directed-interaction
send/receive path, including known-target routing, pending retraction, exact-
bound callback delivery before the grant, and timestamped callback fields.
The focused `timestamped-directed-interaction` CTest lane additionally covers
the ownership/universal subscription selector at later TSO grant boundaries:
the target owner/default and known universal non-owner receive, the known
default non-owner does not, and a selector change or unsubscription before a
later grant suppresses the stale callback. RL-028 records the generic selector
candidate limitation; RL-080 records the cross-page ownership mismatch for the
unsubscription continuation. The remaining same-object, multiple-directed-class
matrix is blocked by the supplied DIF/FDD cardinality conflict recorded in
RL-081; its valid-DIF, invalid-composed-FDD fixture is retained as a regression
guard rather than presented as runtime evidence. The same lane also exercises
the target-departure boundary across two TSO times: a time-6 Remove Object
Instance callback commits departure, then a queued time-7 directed payload is
suppressed before its later Time Advance Grant.
The focused timestamped directed service-report file case now closes the
backend-selection gap for accepted §6.14 sends. A non-time-regulating sender
records type-27/type-37/type-40/type-63/type-31 supplied forms, the official
type-34 Null returned argument, serial zero, and durable report-before-callback
ordering in the configured filesystem; the existing MOM interaction case
continues to cover the public HLAreportServiceInvocation route. Time-regulated
type-33 return records, directed DDM/time/transport matrices, the generic file
ReturnArgument relation, Lab validation, and conformance remain open.
The focused timestamped ordinary-interaction service-report file case now closes
the analogous backend-selection gap for accepted §6.12 sends. It proves the
type-27/type-40/type-63/type-31 supplied forms, the type-34 Null returned
argument for a non-time-regulating sender, serial zero, and durable
report-before-callback ordering in the configured filesystem; time-regulated
type-33 returns and regional/timestamped object-update/delete backend matrices
remain open.
The focused timestamped §6.16 `Delete Object Instance` service-report file case
now closes the corresponding object-management backend gap. It proves the
type-37/type-63/type-31 supplied forms, the type-34 Null returned argument for
a non-time-regulating sender, serial zero, and durable sender-report-before-
`Remove Object Instance` ordering in the configured filesystem; time-regulated
type-33 returns, regional deletion, and recipient-local callback file matrices
remain open.
A focused alternate-advance companion now sends one target-qualified timestamped
directed interaction to three universal recipients through FQR, TARA, and NMRA;
each directed callback precedes its own grant, FQR preserves actual and
optimistic time 7, and the producer completes TAR at 2. The callback metadata
and terminal post-delivery Retract classification are traced to the same
directed/time-management contracts. Directed DDM, alternate advance modes
beyond this FQR/TARA/NMRA case, region-context evidence, transport, and
conformance remain outside the slice. A separately contracted lifecycle
companion queues one non-regional directed payload, disables and callback-gated
re-enables the constrained recipient, and proves exactly one callback before
the matching grant with the original target, tag, producer, timestamp, order,
and retraction metadata. RL-134 records that the Requirements Lab lacks a
cross-service relation for this directed Time-Constrained lifecycle, so the
case remains development-profile evidence only. A separate save/restore
companion saves one queued directed payload and its live retraction ledger,
terminalizes it after the save, restores the snapshot, delivers the original
target-qualified callback through Flush Queue Request, and proves the same
designator issues Request Retraction after delivery. RL-135 records the
corresponding directed save/restore relation gap; timed/durable restore,
selector mutation, alternate advances, directed DDM, and broader recovery
remain open. A timed directed companion now schedules the save at logical time
6 with a timestamp-8 target-qualified payload, crosses the boundary for both
members, restores the post-save-terminalized designator, and proves Flush Queue
delivery plus Request Retraction; RL-141 records the timed save-boundary gap.
A matching regional save/restore companion now saves one overlap-qualified
timestamped `Send Interaction With Regions` payload with its live source
RegionHandle set and retraction ledger, terminalizes the designator after the
save, restores the image, and delivers the original callback through Flush
Queue Request before proving post-delivery Request Retraction. RL-136 records
that the Requirements Lab lacks the cross-service relation for this regional
save/restore lifecycle; timed/durable restore, region mutation,
passive/relaxed-DDM variants, and broader recovery remain open.
`compliance/requirements-lab/timestamped-regional-interaction-requirements-contract.json` and
its API companion add the bounded region-context timestamped Send Interaction
With Regions / Receive Interaction path, including strict committed overlap,
pending retraction, exact-bound callback ordering, and sent-region callback
metadata. A focused explicit-source companion exercises ordinary TAR/NMR plus
the inclusive TARA and NMRA boundaries, preserving the source RegionHandle
before each grant. A companion now carries that explicit source-region
timestamped passel across a Time Constrained disable/re-enable transition and
proves one callback before the post-re-enable grant with the same source
RegionHandle, tag, timestamp, order, and terminal retraction classification.
The corresponding default-source object-update companion now queues an
ordinary-registration timestamped passel across the same transition and proves
one reflection before the post-re-enable grant with the private default
realization represented by a supplied-empty RegionHandleSet.
The regional TAR/NMR companion additionally sends one overlap-qualified TSO
payload to ordinary recipients and asserts each callback before its
corresponding grant. The mixed-member regional companion sends one
overlap-qualified TSO payload to FQR, TARA, and NMRA recipients and asserts
each callback before its corresponding grant. A matching mixed-member regional
object-update companion sends one overlap-qualified attribute passel through
the same FQR, TARA, and NMRA frontiers, preserving source-region metadata and
callback-before-grant order. A paired negative regional object-update
companion retracts a timestamp-8 passel before any of those callbacks and
proves all three alternate grants plus the producer TAR still complete without
reflection or Request Retraction. Its strict TAR-plus-lookahead setup is
recorded explicitly so the scenario does not mistake the equality boundary
for a legal Retract.
The focused production-filesystem case now closes the accepted regional
service-report backend slice. It proves the timestamped `Send Interaction With Regions`
report is durable before the constrained `Receive Interaction` callback and retains
the type-27 interaction class, type-40 parameter map, type-43 source-region set,
type-63 tag, type-31 timestamp, and type-33 retraction return at serial zero.
Its dedicated `timestamped-regional-interaction-service-report` CTest lane
runs the Catch2 case with the regional Requirements-Lab/API/MOM traceability
checks. The lane now also runs a paired HLA_IMMEDIATE public-MOM case that
decodes service type 2, type-27/type-40/type-43/type-63/type-31 supplied
forms, the quoted type-33 `MessageRetractionHandle` return, success/empty
  exception fields, and serial zero before the constrained callback; the callback
  verifies its source `RegionHandle`, timestamp, TIMESTAMP order metadata, and
  valid retraction. Regional failure matrices, regional object-update/delete
  forms, remote transport, and conformance remain open.
  The matching no-time regional `Send Interaction With Regions` companion now
  closes the ordinary accepted service-report boundary. Its filesystem case
  preserves service type 2, type-27/type-40/type-43/type-63/type-34 supplied
  forms, the type-34 Null return, serial zero, and report-before-constrained-
  `Receive Interaction` ordering; the callback verifies the source
  `RegionHandleSet`. A paired HLA_IMMEDIATE public-MOM case decodes the same
  successful report before the callback. The dedicated
  `ordinary-regional-interaction-service-report` lane keeps this C++ evidence
  separate from timestamped and failure matrices.
  The new `timestamped-regional-interaction-failure` lane closes the paired
exception path for the same public overload. Filesystem and HLA_IMMEDIATE
MOM cases cover invalid interaction-class, parameter, region, and logical-time
inputs, preserving serials zero through three, type-27/type-40/type-43/type-63/
type-31 supplied forms, Null returns, false indicators, exact exceptions, and
no application callback. RL-105/RL-152 still leave the conditional failure
relation outside Lab validation; broader regional failure families remain open.
The matching ordinary regional object-update filesystem case now closes the
corresponding no-time backend gap. It proves the shared selector is still used
when an explicit committed object/attribute region association drives
`Update Attribute Values`, preserving serial-zero type-37/type-2/type-63/type-34
  sender forms and durable report-before-constrained-regional-`Reflect Attribute
  Values` ordering. The callback separately conveys the source `RegionHandleSet`.
  Its dedicated `ordinary-regional-attribute-update-service-report` CTest lane runs the
  Catch2 case with dedicated API/requirements and MOM traceability contracts;
  the paired HLA_IMMEDIATE public-MOM case now decodes service type 2,
  type-37/type-2/type-63/type-34 supplied forms, the type-34 Null return,
  success/empty-exception fields, and serial zero before the constrained
  reflection callback. Timestamped and other regional-update failure matrices,
  re-enable/save/restore, transport, and conformance remain open. The paired
  ordinary regional failure lane now also decodes invalid-object and
  invalid-attribute records through HLA_IMMEDIATE public MOM, preserving the
  type-37/type-2/type-63/type-34 forms, Null return, false indicator, exact
  exception text, and serials zero and one without reflection delivery.
The matching production-filesystem case now closes the accepted timestamped
regional object-update service-report backend slice. It proves that an explicit
committed object/attribute region association still uses the shared
file-or-MOM selector for `Update Attribute Values(..., LogicalTime)`, retaining
the type-37 object, type-2 attribute/value map, type-63 tag, type-31 timestamp,
and type-33 `MessageRetractionHandle` return at serial zero before the
constrained regional `Reflect Attribute Values` callback. The callback's
source `RegionHandleSet` remains a separate delivery projection. Its dedicated
`timestamped-regional-attribute-update-service-report` CTest lane runs the
Catch2 case with the regional Requirements-Lab/API/MOM traceability checks;
the paired HLA_IMMEDIATE public-MOM case now decodes the accepted service type
2 interaction, all four standard supplied forms, the quoted type-33 return,
success/empty-exception fields, and serial zero before the constrained callback.
Other regional-update failure matrices, regional delete forms,
re-enable/save/restore, remote transport, and conformance remain open.
The paired `timestamped-regional-attribute-update-failure` lane now covers the
remaining timestamped regional exception slice on the same filesystem route:
invalid object, invalid attribute, and invalid logical time preserve serials
zero through two, type-37/type-2/type-63/type-31 supplied forms, Null returns,
false indicators, and exact exception text without a callback. Public MOM
interaction delivery for those failures is now covered by the companion
HLA_IMMEDIATE matrix, which decodes the same fields after explicit regional
association. Other regional failure families, lifecycle/transport, Lab
validation, and conformance remain open.
Other regional/object-region forms, further alternate advance modes, legal
post-delivery request-retraction callbacks outside the currently bounded
interaction, attribute-update, and non-regional deletion paths, transport, and
conformance remain outside these slices. The focused explicit-source regional
save/restore companion is limited to one untimed live record; timed/durable
restore, region mutation, passive/relaxed-DDM variants, and broader recovery
remain open.

The embedded profile now also has bounded untimed save and restore control
slices. The paired `compliance/requirements-lab/save-control-api-contract.json` and
`compliance/requirements-lab/save-control-requirements-contract.json` files pin the official
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
`compliance/requirements-lab/restore-control-api-contract.json` and
`compliance/requirements-lab/restore-control-requirements-contract.json` files pin Request
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
`compliance/requirements-lab/save-restore-interlock-api-contract.json` and
`compliance/requirements-lab/save-restore-interlock-requirements-contract.json` files now pin
the shared SaveInProgress/RestoreInProgress gate for object-class and
interaction declaration forms, directed declaration, order/transport,
ownership, time-role/query, scope-advisory, and representative publication,
subscription, object, DDM, update, interaction, and time-advance services;
Catch2 exercises both operation states before any of those services mutate
state. Timed restore, durable external persistence, post-restore handle remapping,
interlocks on every remaining service, transport, package/JUnit/protected
evidence, and conformance remain open. A separate
`compliance/requirements-lab/timed-save-api-contract.json` and
`compliance/requirements-lab/timed-save-requirements-contract.json` pair now covers the exact
2025 `Request Federation Save(label, LogicalTime)` overload and the matching
timestamped `FederateAmbassador::initiateFederateSave(label, LogicalTime)`
callback: official logical-time validation, replacement of one pending request,
the requirement that all time-constrained members cross the timestamp, and
release only after queued or in-transit TSO payloads at or below that timestamp
have been delivered. The ordinary-grant dispatcher keeps a constrained
recipient Time Advancing while it drains those TSO payloads, invokes the exact
timestamped Initiate Federate Save callback directly, and only then applies the
matching grant. One Catch2 case proves the TSO →
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
cmake -S . -B out/cmake/fom-services -G "Visual Studio 17 2022" -A x64 `
  -DUMBRA_FETCH_CATCH2=ON `
  -DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON `
  -DUMBRA_FETCH_LIBXML2=ON `
  -DUMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=ON
cmake --build out/cmake/fom-services --config Debug -- /m:1
ctest --test-dir out/cmake/fom-services -C Debug --output-on-failure
~~~

`compliance/requirements-lab/fom-composition-requirements-contract.json` is a separate
Requirements-Lab reference contract for that private preflight. It pins the
Annex C requirement IDs to the implementation symbols and Catch2 selectors,
and CTest runs it as `umbra.ieee1516_2_2025.composition_traceability`. It is
source/test traceability only: it deliberately produces no public-service
catalog entry, JUnit sidecar result, or verified evidence status.

`compliance/requirements-lab/reference-time-requirements-contract.json` similarly pins the
private IEEE 1516.1 reference-time values, encodings, factory names, and
epsilon arithmetic, plus the creation default and common-implementation
selection boundary, to the Lab's source-derived requirement IDs. CTest runs it
as `umbra.ieee1516_2025.reference_time_traceability`. It is not public
`getTimeFactory`, Create, or time-management evidence, and it deliberately does
not add an entry to the JUnit sidecar catalog.

`compliance/requirements-lab/logical-time-encoding-requirements-contract.json` separately pins
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

`compliance/requirements-lab/authorization-requirements-contract.json` separately pins the
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

`compliance/requirements-lab/federation-management-preparation-requirements-contract.json`
separately pins the private MIM-first creation and additional-FOM preparation
pipeline. `compliance/requirements-lab/federation-management-embedded-requirements-contract.json`
separately pins the opt-in adapter's connection, Create, Destroy, Join, and
Resign symbols to the same source-derived Lab records and real Catch2
selectors. CTest registers that second contract only in the development
profile. Both contracts are traceability only: they are not catalog, JUnit,
protected-review, package, or conformance evidence.

The explicit-MIM federation-management slice is also kept native-first. The
focused Catch2 case `Embedded Create Federation Execution accepts a validated
explicit MIM path` passes the official 2025 MIM by a real filesystem designator,
then joins, resigns, and destroys the resulting federation. The reserved
`HLAstandardMIM` spelling remains covered by the neighboring rejection case;
the successful path is traceability for the embedded development profile, not
custom-MIM interoperability or conformance evidence.

`compliance/requirements-lab/get-time-factory-api-contract.json` is a third, development-profile
only contract. It validates the Lab's exact C++ `getTimeFactory` signature and
declared exceptions against the adapter and its Catch2 selector. The Lab has no
higher-level implementation mapping for that service yet, so this is API
traceability—not catalog or evidence promotion.

`compliance/requirements-lab/federate-lookup-api-contract.json` similarly pins the exact 2025
C++ `getFederateHandle` and `getFederateName` signatures and declared exception
sets to the opt-in adapter and a real Catch2 selector. `getFederateHandle`
resolves a currently joined name, while `getFederateName` retains the immutable
name of a returned designator after normal or Connection Lost resignation;
neither behavior expands the caller's joined-federation boundary. The paired
`compliance/requirements-lab/federate-lookup-requirements-contract.json` selects the Lab's Join
designator-lifetime and Get Federate Name requirements. The Lab has no
higher-level implementation mapping for either service, so this remains
source/API traceability—not catalog or evidence promotion.

`compliance/requirements-lab/object-class-lookup-api-contract.json` pins the exact 2025 C++
`getObjectClassHandle` and `getObjectClassName` signatures and declared
exception sets to the same profile. Its real Catch2 scenario resolves classes
from the composed FOM catalog and preserves a prior handle when a compatible
additional-FOM join extends that catalog. The Lab has no higher-level
implementation mapping for either service, so this remains API traceability—
not catalog or evidence promotion.

`compliance/requirements-lab/interaction-class-lookup-api-contract.json` pins the exact 2025
C++ `getInteractionClassHandle` and `getInteractionClassName` signatures and
declared exception sets to the same profile. Its real Catch2 scenario resolves
interactions from the composed FOM catalog and preserves a prior handle when a
compatible additional-FOM join extends that catalog. The Lab has no higher-level
implementation mapping for either service, so this remains API traceability—
not catalog or evidence promotion.

`compliance/requirements-lab/attribute-lookup-api-contract.json` pins the exact 2025 C++
`getAttributeHandle` and `getAttributeName` signatures and declared exception
sets to the same profile. Its real Catch2 scenario resolves an inherited
attribute through its defining class, distinguishes invalid class/attribute
handles from a valid attribute not defined on the supplied class, and preserves
a prior value when a compatible additional-FOM join extends the catalog. The
Lab has no higher-level implementation mapping for either service, so this
remains API traceability—not catalog or evidence promotion.

`compliance/requirements-lab/parameter-lookup-api-contract.json` pins the exact 2025 C++
`getParameterHandle` and `getParameterName` signatures and declared exception
sets to the same profile. Its real Catch2 scenario resolves an inherited
parameter through its defining interaction class, distinguishes invalid
interaction-class/parameter handles from a valid parameter not defined on the
supplied interaction class, and preserves a prior value when a compatible
additional-FOM join extends the catalog. The Lab has no higher-level
implementation mapping for either service, so this remains API traceability—
not catalog or evidence promotion.

`compliance/requirements-lab/interaction-declaration-api-contract.json` pins the exact 2025 C++
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
On/Off relevance advisory slice. Its focused filesystem companion proves all
four ordinary relevance callbacks (§§5.14--5.17) append their recipient-local
type-36/type-27 records before HLA_EVOKED delivery. Regional advisories,
broader MOM interaction delivery beyond the bounded service-report invocation
and object-instance request/report cases (including the reflected-object pair)
plus the transportation-aware `HLArequestUpdatesSent`/
`HLAreportUpdatesSent` and `HLArequestInteractionsSent`/
`HLAreportInteractionsSent` plus the directed-send
`HLArequestDirectedInteractionsSent`/`HLAreportDirectedInteractionsSent`
and receive-side `HLArequestInteractionsReceived`/
`HLAreportInteractionsReceived` plus
`HLArequestDirectedInteractionsReceived`/
`HLAreportDirectedInteractionsReceived` pairs,
or conformance remain outside this
traceability-only claim.

`compliance/requirements-lab/publish-interaction-class-requirements-contract.json` and
`compliance/requirements-lab/publish-interaction-class-api-contract.json` add the focused §5.4
service-report trace. The production-filesystem regression proves an invalid
class adds no record, while an accepted publication adds one type-27
interaction-class record before any declaration advisories are queued. The
Requirements Lab currently owns the coalesced source candidate as §5.5.2;
RL-078 records that mismatch. This remains source/API traceability only; full
advisory matrices, MOM interaction delivery, timestamped interaction,
save/restore, DDM, remote transport, and conformance remain outside the slice.

`compliance/requirements-lab/subscribe-interaction-class-requirements-contract.json` and
`compliance/requirements-lab/subscribe-interaction-class-api-contract.json` add the focused
§5.10 service-report trace. The production-filesystem regression proves an
invalid class adds no record, while an accepted passive subscription adds a
type-27 interaction-class record and type-6 Optional passive subscription
indicator before any declaration advisories are queued. The public C++ `active`
selector is inverted, and §11.5.1 lowercase Boolean text takes precedence over
Table 5's uppercase example (RL-079). The Lab owns the coalesced source
candidate as §5.10.2; RL-078 records that mismatch. This remains source/API
traceability only; full delivery/advisory matrices, MOM interaction delivery,
timestamped interaction, save/restore, DDM, remote transport, and conformance
remain outside the slice.

`compliance/requirements-lab/publish-object-class-attributes-requirements-contract.json` and
`compliance/requirements-lab/publish-object-class-attributes-api-contract.json` add the focused
§5.2 service-report trace. The production-filesystem regression proves an
invalid object class adds no record, while an accepted publication adds type-36
Object class designator and type-1 Set of attribute designators records before
any declaration advisories or ownership-assumption work are queued. The Lab
owns the coalesced source candidate as §5.2.4; RL-078 records that mismatch.
This remains source/API traceability only; complete declaration, advisory,
ownership, MOM interaction delivery, save/restore, DDM, remote transport, and
conformance behavior remain outside the slice.

`compliance/requirements-lab/publish-object-class-directed-interactions-requirements-contract.json`
and `compliance/requirements-lab/publish-object-class-directed-interactions-api-contract.json`
add the focused §5.6 service-report trace. The production-filesystem regression
proves rejected undefined object or interaction designators append no record,
an accepted declaration adds type-36 Object class and type-28 Interaction class
set arguments, and an accepted supplied-empty set remains type-28 `[]`. The Lab
owns the coalesced source candidates as §5.6.3 rather than §5.6; RL-078 records
that mismatch. This remains source/API traceability only; complete directed
delivery/subscription behavior, MOM interaction delivery, timestamped behavior,
save/restore, DDM, remote transport, and conformance remain outside the slice.

`compliance/requirements-lab/unpublish-object-class-directed-interactions-requirements-contract.json`
and `compliance/requirements-lab/unpublish-object-class-directed-interactions-api-contract.json`
add the focused §5.7 service-report trace. The production-filesystem regression
proves rejected undefined object or interaction designators append no record,
the interaction-set overload adds type-36/type-28 arguments (including `[]`
for a supplied-empty set), and the whole-class overload uses type-34 Null. The
Lab owns the coalesced source candidates as §5.7.4 rather than §5.7; RL-078
records that mismatch. This remains source/API traceability only; complete
directed delivery/subscription behavior, MOM interaction delivery, timestamped
behavior, save/restore, DDM, remote transport, and conformance remain outside
the slice.

`compliance/requirements-lab/subscribe-object-class-directed-interactions-requirements-contract.json`
and `compliance/requirements-lab/subscribe-object-class-directed-interactions-api-contract.json`
add the focused §5.12 service-report trace. The production-filesystem regression
proves rejected undefined object or interaction designators append no record;
an accepted defaulted C++ invocation emits type-36/type-28/type-6 arguments
with the effective by-ownership selector `false`; and an accepted explicit
universal invocation with a supplied empty set preserves type-28 `[]` plus
Boolean `true`. The public C++ virtual call cannot distinguish omitted from
explicitly false once the binding supplies its default, so this is source/API
traceability for the effective selector rather than a claim about source-level
argument-presence recovery. RL-080 records the Lab's cross-page §5.12 ownership
mismatch and RL-079 records the Boolean-case inconsistency. Complete directed
delivery/subscription, MOM interaction delivery, timestamped behavior,
save/restore, DDM, remote transport, and conformance remain outside the slice.

`compliance/requirements-lab/unsubscribe-object-class-directed-interactions-requirements-contract.json`
and `compliance/requirements-lab/unsubscribe-object-class-directed-interactions-api-contract.json`
add the focused §5.13 service-report trace. The production-filesystem regression
proves rejected undefined object or interaction designators append no record;
the interaction-set overload adds type-36/type-28 arguments (including `[]`
for a supplied-empty set), and the whole-class overload uses type-34 Null. The
Lab's page-103 postcondition continuation is owned as §5.14.3 rather than
§5.13; RL-080 records that mismatch. This remains source/API traceability only;
complete directed unsubscription/delivery, MOM interaction delivery,
timestamped behavior, save/restore, DDM, remote transport, and conformance
remain outside the slice.

`compliance/requirements-lab/unpublish-object-class-attributes-requirements-contract.json` and
`compliance/requirements-lab/unpublish-object-class-attributes-api-contract.json` add the
focused §5.3 service-report trace. The production-filesystem regression proves
an invalid object class adds no record, while an accepted post-publication
unpublish adds type-36 Object class designator and type-1 Optional set of
attribute designators records after the registry's synchronous ownership
cleanup and before any declaration advisories are queued. The Lab owns the
coalesced source candidates as §5.3.3; RL-078 records that mismatch. This
remains source/API traceability only; complete declaration, ownership,
acquisition, advisory, MOM interaction delivery, save/restore, DDM, remote
transport, and conformance behavior remain outside the slice.

`compliance/requirements-lab/subscribe-object-class-attributes-requirements-contract.json` and
`compliance/requirements-lab/subscribe-object-class-attributes-api-contract.json` add the
focused §5.8 service-report trace. The production-filesystem regression proves
an invalid object class adds no record; an accepted passive `High`-rate
subscription adds type-36 Object class, type-1 attribute-set, type-6 Optional
passive subscription indicator, and type-53 String update-rate records; a
later default-rate subscription uses the type-34 Null optional-update-rate
slot. Records precede separately queued declaration, scope, relevance, and
discovery callbacks. The Lab owns the §5.8 continuation candidates as §5.9;
RL-080 records that cross-page mismatch. This remains source/API traceability
only; full declaration, delivery, advisory, MOM interaction, save/restore,
DDM, remote transport, and conformance behavior remain outside the slice.

`compliance/requirements-lab/unpublish-interaction-class-requirements-contract.json` and
`compliance/requirements-lab/unpublish-interaction-class-api-contract.json` add the focused
§5.5 service-report trace. The production-filesystem regression proves an
invalid class adds no record, while an accepted post-publication unpublish adds
one type-27 interaction-class record before any declaration advisories are
queued. This remains source/API traceability only; full advisory matrices, MOM
interaction delivery, timestamped interaction, save/restore, DDM, remote
transport, and conformance remain outside the slice.

`compliance/requirements-lab/unsubscribe-interaction-class-requirements-contract.json` and
`compliance/requirements-lab/unsubscribe-interaction-class-api-contract.json` add the focused
§5.11 service-report trace. The production-filesystem regression proves an
invalid class adds no record, while an accepted ordinary-unsubscription
invocation adds one type-27 interaction-class record before any declaration
advisories are queued. The Lab owns the coalesced source candidate as §5.11.3;
RL-078 records that mismatch. This remains source/API traceability only; full
delivery/advisory matrices, MOM interaction delivery, timestamped interaction,
save/restore, DDM, remote transport, and conformance remain outside the slice.

`compliance/requirements-lab/object-class-attribute-declaration-requirements-contract.json` and
`compliance/requirements-lab/object-class-attribute-declaration-api-contract.json` pin the exact
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
contract; regional advisory behavior, full ownership, complete update-rate
producer/timing evidence, regions, catalog evidence, and conformance remain
outside this slice.

The separate `compliance/requirements-lab/whole-object-class-declaration-requirements-contract.json`
and `compliance/requirements-lab/whole-object-class-declaration-api-contract.json` cover the
whole-class teardown boundary. The embedded profile removes the complete
ordinary publication set (including the implicit delete privilege), clears the
corresponding ownership on registered instances, rejects later updates, and
removes ordinary subscriptions without deleting independent regional
declarations. Catch2 covers the official methods and their lifecycle/error
boundaries. Regional whole-class teardown and regional declaration advisory
callbacks, save/restore, transport, packaging, protected evidence, and
conformance remain outside this traceability-only slice.

`compliance/requirements-lab/object-instance-registration-requirements-contract.json` and
`compliance/requirements-lab/object-instance-registration-api-contract.json` pin the exact 2025
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

`compliance/requirements-lab/object-instance-name-reservation-requirements-contract.json` and
`compliance/requirements-lab/object-instance-name-reservation-api-contract.json` pin the exact
2025 single/multiple reservation and release declarations plus all four result
callbacks. The test covers official empty/`HLA.` guards, asynchronous success
and contention, mixed multiple outcomes, atomic release, generated-name
collision avoidance, and resignation cleanup. These are development-profile
source/API traceability contracts only; JUnit/protected review evidence,
packaging, and conformance remain deferred. The adjacent focused filesystem
lane additionally proves that accepted single-name Reserve/Release calls use
the Table 5 type-53 `Name` successful-void form in the stable joined-federate
file, while switch-gated and rejected calls append nothing. A companion focused
case now proves accepted multiple-name Reserve/Release calls use the Table 5
type-54 `StringSet`/`Array<String>` form, preserve the exact `Name Set`/`Name
set` labels, and suppress records for switch-gated or atomically rejected
calls. The multiple-name result callback report form remains outside the lane
because its composite name/success-indicator value has no reviewed Table 5
file-log mapping.

`compliance/requirements-lab/object-instance-named-registration-requirements-contract.json` and
`compliance/requirements-lab/object-instance-named-registration-api-contract.json` pin the exact
2025 non-region and regional named registration overloads. The Catch2 case
requires reservation ownership, preserves a reservation across a publication
failure, consumes it only after object/name commit, checks name-in-use and
not-reserved outcomes, and verifies uniform named lookup/discovery plus
release-before-registration reuse. These contracts are source/API traceability
only; timestamped/retraction, broader DDM, ownership transfer,
save/restore, JUnit/protected review evidence, packaging, and conformance remain
deferred.

`compliance/requirements-lab/object-instance-deletion-requirements-contract.json` and
`compliance/requirements-lab/object-instance-deletion-api-contract.json` pin the exact 2025
no-time `deleteObjectInstance` and `removeObjectInstance` records to the
opt-in adapter and a real Catch2 selector. The integration case covers
connection/membership/known-instance boundaries, requires the current
`HLAprivilegeToDeleteObject` owner, suppresses the deleting federate's induced
callback, preserves an evoked recipient's known state until removal starts, and
checks tag/producer propagation for both callback models. These contracts are
source/API traceability only. Timestamped/retraction deletion, ownership
transfer, DDM, FOM sharing policy, save/restore, resign-action disposition,
catalog evidence, and conformance remain outside the slice.

`compliance/requirements-lab/local-delete-object-instance-requirements-contract.json` and
`compliance/requirements-lab/local-delete-object-instance-api-contract.json` trace the exact
2025 `localDeleteObjectInstance` service to the bounded registry transition and
real Catch2 case. The case proves that only the invoking federate forgets the
known instance, that the federation-wide object remains available for
rediscovery, and that `FederateOwnsAttributes` and
`OwnershipAcquisitionPending` are enforced. Its focused filesystem-report
companion proves that a rejected unknown-object call appends nothing and an
accepted call records its type-37 object-instance argument only after the
local-forget transition. These contracts are source/API traceability only;
timestamped/local-delete interaction behavior, DDM, save/restore, remote
transport, catalog evidence, and conformance remain outside the slice.
The paired `local-delete-failure` lane now exercises the same §6.18 service in
both sinks. Filesystem and HLA_IMMEDIATE cases preserve type-37 supplied
handles, Null returns, false indicators, exact `ObjectInstanceNotKnown`
descriptions, and serials zero and two around the accepted serial-one
local-forget report. Accepted interaction emission is outside native locks;
RL-152 keeps this focused failure evidence at development traceability rather
than Lab validation or conformance.

`compliance/requirements-lab/dimension-lookup-requirements-contract.json` and
`compliance/requirements-lab/dimension-lookup-api-contract.json` trace the bounded 2025
dimension metadata foundation. The composed FOM catalog retains class and
interaction dimension associations plus each dimension's upper bound; the
registry allocates stable handles; and the official lookup services expose
inherited available-dimension sets, names, and upper bounds. These contracts
are source/API traceability only.

`compliance/requirements-lab/region-lifecycle-requirements-contract.json` and
`compliance/requirements-lab/region-lifecycle-api-contract.json` trace the bounded 2025
region-template/specification slice. The non-installable development profile creates private region
templates from composed dimensions, keeps `Set Range Bounds` values pending,
requires a complete set for an atomic commit, validates lower/upper values
against the FOM dimension bound, exposes owner-scoped dimension/range support
lookups, deletes unused regions, and decodes official region handles. These
contracts remain source/API traceability only. Its focused service-report
companion proves that an accepted §9.3 Commit Region Modifications call writes
the Table 5 type-43 RegionHandleSet argument to the joined federate's selected
filesystem, while invalid handles append nothing. The adjacent non-void DDM
lane now covers Create Region's type-11 DimensionHandleSet supplied value and
type-42 RegionHandle return using the rendered Table 5 returned-argument array.
Its paired failure case covers invalid DimensionHandleSet and invalid
dimension-in-region inputs with Null returns, false indicators, exception text,
and contiguous serials; the public HLA_IMMEDIATE companion uses service type 5.
The neighboring mutation-failure lane covers invalid region sets, invalid
region handles, invalid dimensions, and invalid bounds for Commit Region
Modifications, Delete Region, and Set Range Bounds, while retaining the
current file-only/suppressed behavior for their successful void interaction
route.
The adjacent focused Delete Region lane covers the
source-named type-42 RegionHandle argument after an accepted §9.4 deletion and
also proves invalid handles append nothing. Region-realization/callback
delivery, generic non-void returns beyond Create/Get Range Bounds, broader DDM
routing, and conformance remain outside these lanes.
The adjacent focused Set Range Bounds lane covers the source-named §10.28
Region/Dimension/Number arguments and suppresses the report on an invalid
bound. The same non-void lane covers §10.27 Get Range Bounds with type-42/type-10
supplied values and type-41 RangeBounds lower/upper return. Region-realization/
callback delivery, generic non-void returns beyond this bounded DDM slice,
broader DDM routing, and conformance remain outside these lanes. The separate object-attribute
regional contract below covers a bounded no-name registration, association,
subscription, and no-time overlap-routing slice; timestamped regional forms
are traced separately below,
broader DDM routing, save/restore, packaging, catalog evidence, and conformance
remain outside this region-template slice.

The support-service lookup lane now adds source-backed non-void file records
for §10.21--§10.26. It covers available dimensions for object and interaction
classes (type-36/type-27 supplied handles and type-11 returned sets),
dimension-name/handle conversion (type-53/type-10), dimension upper bounds
(type-35 Number), and region dimension-set lookup (type-42 supplied region and
type-11 returned set). Setup calls are switch-gated, each accepted lookup
retains the joined-federate file and serial order, and the unit/integration
selectors are independently runnable. The Lab has no row-level Table 5
ReturnArgument records for these forms, so the accepted evidence remains
development-profile traceability rather than validation or conformance
(RL-042/RL-067/RL-076). Paired filesystem and HLA_IMMEDIATE failure matrices
now preserve the six official supplied forms, Null returns, false indicators,
public exception text, and serials zero through five before a successful
serial-six lookup. Broader non-void coverage, package/JUnit/protected review,
and conformance remain open; RL-152 records the missing conditional failure
relation.

The same support-service lane now covers the mandatory §10.17--§10.20 order
and transportation lookup pairs. `GetOrderType`/`GetOrderName` use type-53
`Order name` and type-38 quoted `Order type` values; the transportation pair
uses type-53 `Transportation type name` and type-59 quoted
`TransportationTypeHandle::toString()` values. The C++ unit selector validates
the Table 5 forms directly, while the filesystem integration selector proves
switch-gated setup, one stable joined-federate file, and serial ordering for
all four accepted lookups. The Lab still has no row-level ReturnArgument
mapping for these Table 5 forms, so the accepted evidence remains
development-profile traceability rather than validation or conformance
(RL-042/RL-067/RL-076/RL-147). Paired filesystem and HLA_IMMEDIATE failure
matrices now preserve the four official supplied forms, Null returns, false
indicators, public exception text, and serials zero through three before a
successful serial-four lookup; invalid OrderType uses the deterministic
type-38 UNSUPPORTED diagnostic. Broader non-void coverage, package/JUnit/
protected review, and conformance remain open.

The same support-service lane now covers the mandatory §10.2--§10.5 federate
and object-class lookup pairs. `GetFederateHandle`/`GetFederateName` use
type-53 `Federate name` and type-15 `Federate handle`; the object-class pair
uses type-53 `Object class name` and type-36 `Object class handle`. The C++
unit selector validates the Table 5 forms directly, while the filesystem
integration selector proves one immutable joined-federate file and serial
ordering for all four accepted lookups. The Lab still has no row-level
ReturnArgument mapping for these forms, so the evidence remains
development-profile traceability rather than validation or conformance
(RL-042/RL-067/RL-076/RL-149); invalid/failure matrices, public MOM delivery,
broader non-void coverage, package/JUnit/protected review, and conformance
remain open.

The same support-service lane now covers the mandatory §10.13--§10.16
interaction-class and parameter lookup pairs. `GetInteractionClassHandle` /
`GetInteractionClassName` use type-53 `Interaction class name` and type-27
`Interaction class handle`; `GetParameterHandle` uses type-27 interaction
class handle plus type-53 `Parameter name` and returns type-39 `Parameter
handle`, while `GetParameterName` reverses the type-27/type-39 supplied
forms. The C++ unit selector validates the Table 5 forms directly, while the
filesystem integration selector proves one immutable joined-federate file and
serial ordering for all four accepted lookups. The Lab still has no row-level
ReturnArgument mapping for these forms, so the evidence remains
development-profile traceability rather than validation or conformance
(RL-042/RL-067/RL-076/RL-150); invalid/failure matrices, public MOM delivery,
broader non-void coverage, package/JUnit/protected review, and conformance
remain open.

The following support-service slice covers §10.29--§10.33 handle
normalization. `NormalizeServiceGroup` records type-50 `Service group
indicator` and returns a type-35 `Normalized value` Number; the federate,
object-class, interaction-class, and object-instance forms use type 15, 36,
27, and 37 supplied handle arguments and the same type-35 Number return. The
focused C++ unit selector checks the Table 5 forms directly, while the
filesystem integration selector proves switch-gated setup, one stable
joined-federate file, and serial ordering for all five accepted services. The
returned normalized values are execution-scoped coordinates; MIM
`HLAnormalized*` datatype names are not substituted for the Table 5 Number
argument type. Paired filesystem and HLA_IMMEDIATE failure matrices now cover
the five invalid normalization calls with Null returns, false indicators,
exception text, deterministic `UNSUPPORTED` enum text, and serials zero
through four before a successful serial-five call. The Lab still has no
row-level ReturnArgument mapping for these forms, and RL-152 records the
conditional failure relation gap, so the evidence remains development-profile
traceability rather than validation or conformance (RL-042/RL-067/RL-076/
RL-148); broader non-void coverage, package/JUnit/protected review, and
conformance remain open.

`compliance/requirements-lab/interaction-region-requirements-contract.json` and
`compliance/requirements-lab/interaction-region-api-contract.json` pin the 2025 regional
interaction declaration and no-time send overloads to the opt-in adapter and
the `Embedded regional interaction subscriptions filter 2025 receive-order
sends` Catch2 case plus the passive-subscription regression. Together they
cover independent ordinary/regional state, active/passive regional pairs,
committed-range overlap, empty regional sets, callback-entry rechecks, and
foreign/uncommitted/context-invalid region failures. The object-attribute
regional forms are traced separately below; focused service-report assertions
  now also prove the recipient-local §9.10/
§9.11 successful-void records: type-27 interaction handle, type-43 region set,
and the type-6 passive indicator for subscription, followed by the type-27 /
type-43 unsubscription record. Switch gating keeps setup mutations out of the
serial sequence, and the dedicated CTest lane is
`regional-interaction-subscription-service-report`.
The paired native HLA_IMMEDIATE case now decodes the accepted public MOM
records as well, including passive-indicator inversion for a passive
subscription followed by active replacement and serials zero through two.
RL-152 still leaves the conditional backend/lifecycle relation outside Lab
validation, so these are development-profile C++ traces rather than
conformance evidence.

The accepted ordinary regional `Send Interaction With Regions` service-report
pair now covers the sender boundary as well: the configured filesystem and
HLA_IMMEDIATE MOM routes preserve service type 2, type-27/type-40/type-43/
type-63/type-34 supplied forms, the Null return, success true, and serial zero
before the constrained application callback, which verifies the source
`RegionHandleSet`. Its focused CTest lane is
`ordinary-regional-interaction-service-report`; RL-105/RL-152 still provide
no row-level backend/lifecycle relation, so this is development-profile
traceability rather than Lab validation or conformance.
The new `ordinary-regional-interaction-failure` lane closes the no-time
`Send Interaction With Regions` exception path. Filesystem and HLA_IMMEDIATE
MOM cases cover invalid interaction-class, parameter, and region inputs,
preserving serials zero through two, type-27/type-40/type-43/type-63/type-34
forms, Null returns, false indicators, exact exception text, and no application
callback. RL-105/RL-152 leave the conditional failure relation outside Lab
validation; timestamped and other regional failure families remain separate.
Regional value-update requests beyond the
class-level form,
timestamped/retraction behavior, region realization beyond explicit
associations, broader DDM routing, save/restore, packaging, catalog evidence,
and conformance remain separate.

`compliance/requirements-lab/object-attribute-region-requirements-contract.json` and
`compliance/requirements-lab/object-attribute-region-api-contract.json` trace the bounded 2025
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

The sibling two-dimensional multi-attribute RegionalThing stress case is now
translated into the native Catch2 test
`Embedded two-dimensional regional object updates filter independent attribute
sources`. It uses a checked-in DIF fixture, publishes Flavor and Organic with
independent two-dimensional source regions, and subscribes each recipient to
its own overlapping region. The focused assertions cover initial delivery,
X-only source disjointness, Y-only source disjointness, restored delivery, and
the conveyed source `RegionHandleSet`; the `ddm-regional-multi-attribute`
CTest lane runs that Catch2 case with
`compliance/requirements-lab/regional-multi-attribute-ddm-requirements-contract.json` and
`compliance/requirements-lab/regional-multi-attribute-ddm-api-contract.json`. This is a
sibling-derived development stress vector, not a claim of full DDM coverage or
conformance.

The focused filesystem lane additionally proves accepted §9.6/§9.7
Associate/Unassociate Regions For Updates calls append type-37 object-instance
and type-4 attribute/region-pair-list arguments before any scope/relevance
work, while an invalid region appends nothing. It does not claim registration
or region-realization reports, callback delivery, timestamped/retraction
forms, public MOM interaction delivery, packaging, or conformance.

`compliance/requirements-lab/default-region-requirements-contract.json` records the bounded
IEEE 1516.1-2025 default-region realization. The registry never exposes a
synthetic default `RegionHandle`: absence of an explicit source association,
or absence of an explicit regional declaration on the receiving side, selects
derived default-region state. The paired object
and interaction Catch2 cases prove ordinary/regional effectiveness, replacement
and restoration around an explicit object update association, discovery/scope
continuity, and supplied-empty `RegionHandleSet` callback metadata when Convey
Region Designator Sets is enabled. Two companion time-constrained TSO cases
prove the same supplied-empty convention for one timestamped object reflection
and one timestamped interaction callback after federation-owned queueing. A
third default-source interaction companion uses the inclusive TARA and NMRA
boundaries, proving callback-before-grant ordering and the same supplied-empty
marker at timestamps 7 and 9. The default-source object and interaction
companions now apply the same three frontiers to ordinary timestamped updates,
and a fourth interaction companion keeps one queued passel across a Time
Constrained disable/re-enable transition before the grant,
with each callback preceding its grant and the producer completing its TAR
independently. A pair of default-source object and interaction cases split
mixed fanout: each
delivers immediately to a non-time-constrained regional subscriber, then a
legal `Retract` requests retraction only there while withdrawing the constrained
subscriber's queued passel before its grant. This remains bounded
development-profile traceability only; the new default-source object companion
also covers the FQR/TARA/NMRA frontiers, while the remaining timestamped/
re-enable matrix, relaxed DDM, broad dimension combinations, packaging, catalog
evidence, and conformance remain outside the claim.

`compliance/requirements-lab/ownership-transfer-update-region-requirements-contract.json` and
`compliance/requirements-lab/ownership-transfer-update-region-api-contract.json` add the
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

`compliance/requirements-lab/object-attribute-scope-requirements-contract.json` and
`compliance/requirements-lab/object-attribute-scope-api-contract.json` trace the official 2025
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

`compliance/requirements-lab/attribute-relevance-advisory-requirements-contract.json` and
`compliance/requirements-lab/attribute-relevance-advisory-api-contract.json` trace the official
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
complete update-rate producer/timing evidence, package/JUnit/protected-review
evidence, and conformance remain explicitly unimplemented. These contracts are
source/API traceability only.

`compliance/requirements-lab/advisories-use-known-class-requirements-contract.json` and
`compliance/requirements-lab/advisories-use-known-class-api-contract.json` pin the official
`getAdvisoriesUseKnownClassSwitch` getter and its two clause-10.55.1 return
statements. The FDD composer parses the switch, omitted input remains
Disabled, and federation creation captures one static value that all members
observe without rewriting it during an additional-FOM join. The Catch2 case
covers both the enabled value and the exact getter surface. The switch is not yet used
to implement complete known-class advisory generation, so this remains
development-profile traceability rather than conformance evidence.

`compliance/requirements-lab/update-rate-value-requirements-contract.json` and
`compliance/requirements-lab/update-rate-value-api-contract.json` pin the exact 2025
`getUpdateRateValue` and `getUpdateRateValueForAttribute` declarations. The
Catch2 case proves the Restaurant FOM's composed `High`/`Medium`/`Low` values,
the `HLAdefault` no-reduction boundary, invalid-designator handling, retained
ordinary subscription rates, unsubscribe removal, and known-object/defined-
attribute exception paths. The paired
`compliance/requirements-lab/update-rate-subscription-api-contract.json` pins the two official
designator-bearing subscription declarations. The private `UpdateRateGate` now
enforces retained FDD rates per projected attribute for bounded receive-order
and timestamped callbacks, with reliable bypass and federation-lifetime reset.
`compliance/requirements-lab/update-rate-reduction-requirements-contract.json` and its focused
Catch2/CTest lanes bind the five fragmented candidates without presenting them
as conformance evidence; the timestamped lane proves a real two-federate
queued-delivery boundary. RL-099 records the Lab's missing cross-cutting
delivery relation, and RL-100 records the implementation boundary. The Lab's exported second requirement
retains clause `10.13.2` even though its source-page heading has a `10.12`
numbering drift; that discrepancy is documented separately. These contracts
are development traceability only, not validation or conformance evidence.

`compliance/requirements-lab/receive-order-attribute-update-requirements-contract.json` and
`compliance/requirements-lab/receive-order-attribute-update-api-contract.json` pin the exact
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

`compliance/requirements-lab/attribute-value-update-request-requirements-contract.json` and
`compliance/requirements-lab/attribute-value-update-request-api-contract.json` pin the exact
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

`compliance/requirements-lab/attribute-value-update-response-requirements-contract.json` and
`compliance/requirements-lab/attribute-value-update-response-api-contract.json` cover the
separate bounded 2025 response lifecycle. The provider test invokes the
official non-timestamped `Update Attribute Values` service from inside
`Provide Attribute Value Update`; the requester then verifies
`Reflect Attribute Values`, the response tag, producing federate, and mandatory
transportation type. This is explicit provider behavior, not RTI-automatic
provision, and remains development-profile source/API traceability only.

`compliance/requirements-lab/auto-provide-requirements-contract.json` and
`compliance/requirements-lab/auto-provide-api-contract.json` cover the bounded federation-wide
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

`compliance/requirements-lab/object-class-attribute-value-update-request-requirements-contract.json`
and `compliance/requirements-lab/object-class-attribute-value-update-request-api-contract.json`
pin the exact 2025 object-class `requestAttributeValueUpdate` declaration and
matching `provideAttributeValueUpdate` callback to the opt-in adapter, private
class expansion, and a real Catch2 lifecycle case. The case covers the
connection/membership and class/attribute boundaries, expansion from a base
class to two registered subclass instances without requester discovery, owner
grouping per object instance, requester-owner suppression, tag propagation, and
a resigned provider before evoked delivery. These contracts are source/API
traceability only. A separate filesystem regression proves that the two class-
expanded provider callbacks append their instance-specific §6.22 records, in
serial order, before callback user code. It deliberately excludes additional
regional request forms, automatic provision, and broader timestamped/retraction
behavior. A focused class-expansion response companion now has the provider
invoke timestamped `Update Attribute Values` from the official callback and
proves one reflection before the constrained requester's matching grant, with
the request tag, response value/tag, producer, order, time, and retraction
metadata intact. The response remains explicit provider code; alternate
advances, DDM, update-rate reduction, ownership transfer, FOM sharing policy,
save/restore, catalog evidence, and conformance remain outside this
traceability-only slice.

`compliance/requirements-lab/attribute-value-update-with-regions-requirements-contract.json`
and `compliance/requirements-lab/attribute-value-update-with-regions-api-contract.json` pin the
exact 2025 class-level `requestAttributeValueUpdateWithRegions` declaration and
matching `provideAttributeValueUpdate` callback to the opt-in adapter, private
region-aware class expansion, and a real Catch2 lifecycle case. The case covers
committed region ownership/context, empty-pair no-op behavior, explicit
update-region overlap, default-region eligibility, tag propagation, and
callback-entry rechecks. One focused companion response case has an
overlap-qualified provider invoke non-timestamped `Update Attribute Values`
from the official callback and verifies the requester-side response value/tag,
producer, transportation, and explicit sent update region when conveyance is
enabled. A second changes the requester's committed subscription to a valid
disjoint range before its queued reflection boundary and verifies suppression.
Provider responses remain explicit user behavior; the new timestamped regional
response is traced separately from the no-time response. Automatic provision,
broader DDM, catalog evidence, and conformance remain outside this
traceability-only slice.

The regional request contracts now also have focused service-report cases. The
regional request case uses one committed overlap-qualified owner/receiver
region pair and proves both sides of the accepted service boundary: the
requester's type-36/type-4/type-63 §9.13 successful-void record is appended to
its selected file before provider delivery, and the provider's separate
type-37/type-1/type-63 §6.22 record is present before the regional class-request
callback. The adjacent object-class
regional subscription lane records the §9.8/§9.9 type-36/type-4 arguments plus
the passive and update-rate slots, while the association lane records the
§9.6/§9.7 object-instance form. Region filtering remains the eligibility gate;
region handles are not added to the §6.22 callback report's three supplied
arguments. The requester-side assertion is runnable through the focused
`regional-attribute-value-update-request-service-report` CTest lane, while the
provider assertion remains in `provide-attribute-value-update-regional-service-report`.

`compliance/requirements-lab/attribute-ownership-query-requirements-contract.json` and
`compliance/requirements-lab/attribute-ownership-query-api-contract.json` pin the exact 2025
`queryAttributeOwnership`, `informAttributeOwnership`, and
`attributeIsNotOwned` declarations to the opt-in adapter, private ownership
snapshot, and a real Catch2 lifecycle case. The case covers connection and
membership boundaries, known-instance and known-class attribute validation,
grouped federate-owner/unowned results, the returned federate handle, and
nullification of pending reports when receive-order `Remove Object Instance`
starts. These contracts are source/API traceability only. They deliberately do
not model RTI-owned state or ownership acquisition/divestiture, and make no
catalog or conformance claim.

`compliance/requirements-lab/attribute-ownership-check-requirements-contract.json` and
`compliance/requirements-lab/attribute-ownership-check-api-contract.json` pin the exact 2025
`isAttributeOwnedByFederate` declaration to the opt-in adapter, private
ownership snapshot, and a real Catch2 case. The case validates the official
connection, membership, known-instance, known-class attribute, and removal
boundaries, then distinguishes the invoking current owner from a remote owner
and an unowned attribute. It is source/API traceability only: there is no
callback, ownership transfer, RTI-owned state, catalog evidence, or conformance
claim.

`compliance/requirements-lab/attribute-ownership-acquisition-if-available-requirements-contract.json`
and `compliance/requirements-lab/attribute-ownership-acquisition-if-available-api-contract.json`
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

`compliance/requirements-lab/attribute-ownership-acquisition-requirements-contract.json` and
`compliance/requirements-lab/attribute-ownership-acquisition-api-contract.json` pin the exact
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

`compliance/requirements-lab/attribute-ownership-divestiture-if-wanted-requirements-contract.json`
and `compliance/requirements-lab/attribute-ownership-divestiture-if-wanted-api-contract.json`
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

`compliance/requirements-lab/unconditional-attribute-ownership-divestiture-requirements-contract.json`
and `compliance/requirements-lab/unconditional-attribute-ownership-divestiture-api-contract.json`
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

`compliance/requirements-lab/negotiated-attribute-ownership-divestiture-requirements-contract.json`
and `compliance/requirements-lab/negotiated-attribute-ownership-divestiture-api-contract.json`
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
search, negotiated acquisition, RTI-owned state, full resign-action disposition,
catalog evidence, validation, or conformance. The separate
`compliance/requirements-lab/negotiated-willing-to-acquire-requirements-contract.json` and
`compliance/requirements-lab/negotiated-willing-to-acquire-api-contract.json` trace the focused
extension that selects one already pending Willing-to-Acquire candidate,
preserves its acquisition tag through Request Divestiture Confirmation, and
suppresses the stale If Available callback after confirmation. That extension
uses a deterministic private selection policy; it is not complete owner search
or arbitration and carries no conformance claim.

`compliance/requirements-lab/transportation-type-api-contract.json` pins the exact 2025 C++
`getTransportationTypeHandle` and `getTransportationTypeName` signatures and
declared exception sets to the opt-in adapter. Its Catch2 coverage checks the
connection/membership boundary, both mandatory `HLAreliable` and
`HLAbestEffort` names, stable handles, invalid names/handles, and a declared
custom transportation resolved from a composed provider/consumer FOM and
shared by two joined federates. Focused ordinary, ordinary-regional,
nonregional-timestamped, timestamped-regional, and ordinary/timestamped directed
interaction cases now carry that declared custom type through the corresponding
receive callbacks; ordinary and timestamped nonregional plus ordinary/timestamped
regional attribute cases carry it through reflection callbacks.
The Lab has no higher-level implementation mapping for these services, so this
is API/source traceability—not catalog or evidence promotion. The separate
`transportation-type-change-requirements-contract.json`,
`interaction-transportation-type-change-requirements-contract.json`, and
`transportation-type-change-api-contract.json` trace the official
change/default/query services and four callback surfaces. The development
profile captures per-federate attribute defaults, commits accepted changes at
confirmation callbacks, and applies interaction overrides to future ordinary
and regional sends. These contracts still do not claim custom transportation
policy for remote delivery, package transport, timestamped/local
object-lifecycle delivery, message transport, or conformance.

`compliance/requirements-lab/receive-order-interaction-requirements-contract.json` and
`compliance/requirements-lab/receive-order-interaction-api-contract.json` trace the exact
non-timestamped C++ `Send Interaction` overload and no-time `Receive
Interaction` callback. The integration scenario covers official exception
boundaries, publication, exact active and passive-suppressed superclass
subscriptions, one callback for a dual subscription, sender exclusion, tag/producer/mandatory
transportation propagation, unsubscribe-before-delivery, and both callback
models. The private registry scenario uses the official Restaurant FOM to test
descendant-parameter projection. A separate composed-FDD case covers one
declared custom transportation in ordinary interaction delivery; custom
transportation policy outside that case remains open. These contracts explicitly
exclude timestamped/retraction behavior, regional forms beyond the separate
interaction case, FOM sharing-policy enforcement, catalog evidence, and
conformance.

`compliance/requirements-lab/directed-interaction-requirements-contract.json` and
`compliance/requirements-lab/directed-interaction-api-contract.json` trace the bounded
non-timestamped, non-DDM C++ `Send Directed Interaction` overload, no-time
`Receive Directed Interaction` callback, and six object-class declaration
overloads. The integration scenario covers known-target discovery, sender
exclusion, immediate and evoked callbacks, selective unsubscribe, source
unpublication, stale callback suppression, republishing, tag/producer/mandatory
transportation propagation, and the ownership/universal selector. It now also
proves that deleting a target after a receive-order send but before evocation
suppresses the stale directed callback while its Remove Object Instance callback
still arrives. The selector
regression proves default by-ownership suppression for a known non-owner,
universal delivery to that non-owner, empty-set preservation, and supplied-class
mode changes. Timestamped/retraction behavior beyond the separate bounded
slice, directed DDM, ordering, runtime use of FOM sharing capability metadata,
validation, and conformance remain deferred. The composed FOM catalog now
retains that P/S metadata without treating it as a homegrown per-federate
permission rule; the time-6 removal/time-7 timestamped target-departure case
is covered by the paired TSO scenario.

`compliance/requirements-lab/federation-listing-requirements-contract.json` and
`compliance/requirements-lab/federation-listing-api-contract.json` pin the same development
profile's two List methods plus the three official report callback declarations
to source-derived requirement records and exact C++ Lab surfaces. The members
contract includes the separate missing-federation report path. CTest registers
both only for this profile. The Lab has no higher-level implementation mapping
for those records, so they remain source/API traceability rather than catalog
or evidence promotion.

`compliance/requirements-lab/time-advance-requirements-contract.json` and
`compliance/requirements-lab/time-advance-api-contract.json` pin joined-federate initial time,
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

The focused `float-time` lane extends this public slice to the official
`HLAfloat64Time` representation. Its Catch2 case creates a schema-valid FOM
that documents the float representation, verifies initial time and factory
identity, keeps Enable Time Regulation and Query Lookahead callback-gated, and
advances to a fractional grant while preserving the selected representation.
`compliance/requirements-lab/float-time-requirements-contract.json` and the matching CTest
traceability check bind that case to the source-derived time-advance record.
This is development-profile source/API traceability only; it does not claim
complete time-management coordination, JUnit/protected evidence, or
conformance.

`compliance/requirements-lab/time-role-requirements-contract.json` and
`compliance/requirements-lab/time-role-api-contract.json` similarly pin the initial
Enable/Disable Time Regulation, Enable/Disable Time Constrained, Query
Lookahead, and callback paths. They make the callback-gated boundary explicit:
a pending enable blocks time advance until its callback. GALT/LITS are traced
separately, and role state changes re-evaluate the limited TAR scheduler when
they change eligibility. The separate coordinator contract covers private
timestamped queue state; the five timestamped-service contracts record the
bounded public producers. No contract asserts full time-management
coordination, a JUnit sidecar, catalog entry, protected review, package
support, or conformance.

`compliance/requirements-lab/modify-lookahead-requirements-contract.json` and
`compliance/requirements-lab/modify-lookahead-api-contract.json` add the bounded 2025 Modify
Lookahead path. Catch2 verifies immediate increases, gradual decreases at
grant boundaries, and the `InTimeAdvancingState`/not-enabled fences; the
accepted transition also wakes the limited TAR scheduler when its bound
changes. Remaining advance modes and full time-management coordination are
not implied.

`compliance/requirements-lab/next-message-request-requirements-contract.json` and
`compliance/requirements-lab/next-message-request-api-contract.json` add the bounded 2025
Next Message Request path. Catch2 queues a real timestamped interaction,
selects its timestamp when it is below the requested boundary, and verifies
that the equal-timestamp delivery occurs before `Time Advance Grant`. The
effective grant target and caller request boundary remain separate in private
state; the focused filesystem regression proves the report preserves that
supplied boundary, and `umbra_test_next_message_request` selects only this
service's direct state, TSO, save/restore, encoding, report, and traceability
cases. Future transport input, Flush Queue Request/Grant, and full
time-management coordination remain outside this traceability slice.

`compliance/requirements-lab/time-advance-request-available-requirements-contract.json` and
`compliance/requirements-lab/time-advance-request-available-api-contract.json` trace the
bounded `Time Advance Request Available` path. The matching Catch2 scenario
proves that a queued TSO message can be delivered before the grant at an
inclusive defined-GALT boundary. Its focused filesystem regression separately
proves an accepted request records Table 5 `LogicalTime` before its later
grant, and the `umbra_test_time_advance_request_available` CTest lane selects
only this service's direct state, TSO, restore, encoding, report, and
traceability cases. The paired
`compliance/requirements-lab/next-message-request-available-requirements-contract.json` and
`compliance/requirements-lab/next-message-request-available-api-contract.json` trace the
currently queued `Next Message Request Available` target/cohort path and its
same inclusive boundary. Its focused filesystem regression proves the report
retains the supplied boundary even when the queued message yields an earlier
grant, and `umbra_test_next_message_request_available` selects only this
service's direct state, GALT, TSO, bounded save/restore, encoding, report,
and traceability cases. Future transport input, Flush Queue Grant, general
save/restore, package support, and conformance remain outside both slices.

`compliance/requirements-lab/flush-queue-request-requirements-contract.json` and
`compliance/requirements-lab/flush-queue-request-api-contract.json` trace the bounded Flush
Queue Request/Grant path. The Catch2 scenario flushes all currently queued
in-process TSO payloads, checks the request/GALT/delivered-timestamp minimum,
verifies the optimistic logical-time argument, and proves that the optimistic
floor constrains the next advance. The timestamped-save cases additionally
prove strict actual-FQG admission: an equal grant does not start the save, a
later grant drains TSO before direct initiation and FQG, and mixed FQR/TAR
members are all prequalified before the operation starts. Regional explicit-
source Send Interaction With Regions and default-source Send Interaction
companions use committed overlap and verify the conveyed source-region set
before FQG, including the supplied-empty marker for the private default region;
an HLA_EVOKED future-input companion
also proves messages submitted after FQR acceptance but before callback
dispatch precede FQG. These are in-process callback-frontier cases, not remote
transport evidence. Regional TAR/NMR companions also prove the same
source-region payload crosses ordinary callback frontiers before their grants
for both interaction and object-attribute payloads.
A mixed-member regional interaction companion proves the same source-region
payload crosses FQR, TARA, and NMRA callback frontiers before their grants. A
matching mixed-member regional object-update companion proves the equivalent
Reflect Attribute Values frontiers and metadata; its negative retraction
companion proves the queued passel can be withdrawn before those frontiers
without manufacturing callback traffic. A six-member timestamped-save case combines that FQR
branch with TAR, NMR, TARA, and NMRA
before the non-time-constrained notification. Future transport and multi-member/
multi-mode in-transit coordination, Request Retraction behavior outside normal timestamped Send
Interaction, and general save/restore behavior remain outside this slice. The
paired live-record restore regression uses Flush Queue Request only to cross a
restored recipient's callback boundary without moving the producer's strict
retraction lower bound; it is not a general save/restore claim. Package support
and conformance remain outside this slice.

`compliance/requirements-lab/asynchronous-delivery-requirements-contract.json` and
`compliance/requirements-lab/asynchronous-delivery-api-contract.json` trace the official
Enable/Disable Asynchronous Delivery pair. The unit and integration scenarios
prove the default-disabled state for time-constrained federates, defer
receive-order callbacks while idle, release them after enabling or entering
Time Advancing, and enforce the public already-enabled/already-disabled and
membership/connection boundaries under both `HLA_EVOKED` and `HLA_IMMEDIATE`.
The immediate regression confirms that each release occurs during the enabling
or Time Advance Request service call rather than during an Evoke. The implementation applies this gate to the
bounded embedded receive-order interaction, reflection, directed-interaction,
and object-removal routes. A regional receive-order companion applies the same
gate to an overlap-qualified `Send Interaction With Regions` callback and
verifies source-region metadata across disable/re-enable. Timestamped messages, remote transport, MOM
reporting, save/restore persistence of deferred callbacks, protected review,
package evidence, and conformance remain open.

`compliance/requirements-lab/federation-time-coordination-foundation-requirements-contract.json`,
`compliance/requirements-lab/time-bounds-requirements-contract.json`, and
`compliance/requirements-lab/time-bounds-api-contract.json` record the federation-owned
snapshot, FDD Non-Regulated-Grant metadata, and exact Query GALT/Query LITS
surfaces. The private coordinator now supplies queued, in-transit, and
delivered-since-last-advance message inputs. GALT includes those timestamps and
the other-regulator candidate; LITS uses future queued/in-transit timestamps
and can remain defined when GALT is unregulated. The other-regulator candidate
uses current or pending time plus lookahead, with a factory epsilon for a
forward zero-lookahead TAR boundary. The contracts are source/API traceability
only, not a catalog or evidence promotion.

`compliance/requirements-lab/time-grant-policy-requirements-contract.json` separately traces
the private no-TSO TAR eligibility decision: strict comparison with a defined
GALT and FDD Non-Regulated-Grant behavior when the bound is undefined. It is a
pure kernel test, not a scheduled grant, public-service, catalog, or evidence
claim.

`compliance/requirements-lab/time-grant-scheduler-requirements-contract.json` traces the
limited private scheduler that uses that policy to queue eligible TAR grants
and rechecks the policy at delivery. Its embedded Catch2 cases cover a defined
GALT, disabled/default and enabled NRG, regulator enable/disable/resignation,
time-constrained disable, and a successful additional-FOM definition
replacement that changes NRG before waking an existing TAR. Like the other
temporal contracts, it is source/test traceability only, not a catalog or
evidence promotion.

compliance/requirements-lab/api-baseline.json maps a deliberately small set of official
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
-DUMBRA_FETCH_CATCH2=ON to fetch pinned Catch2 v3.15.3 into out/cmake/default. It tests
the official factory, the embedded Connect/Disconnect lifecycle, official
callback controls, the official integer/float reference-time and factory
behavior, and the private federation/handle/callback kernels. Tests use the
official NullFederateAmbassador helper, preserving the exact standard callback
interface without introducing a second public API. The development-profile
target additionally exercises two independent two-federate lifecycles and a
federation-listing report path plus callback-gated time advancement through
only official C++ methods and standard exceptions.

## Development test lanes

CMake discovers each Catch2 case as an individual CTest test and exposes its
existing Catch2 tags as CTest labels. Discovery is generated after the Catch2
test executable is built, so focused CTest lanes only read the already-created
manifest and can run independently without competing to write discovery state.
The build also verifies the tag taxonomy: every Catch2 scenario must have
exactly one execution-scope tag (`unit` or `integration`) and at least one
named domain tag. This makes an unclassified test a fast, actionable failure
rather than an accidental omission from all focused lanes.
Each named one-service lane is also audited from CTest's generated catalog: it
must retain at least one Catch2 behavior case, one Requirements-Lab
traceability check, and one API-contract traceability check. That prevents a
tag or contract-name edit from quietly turning a fast lane into a partial
signal. Filesystem-backed Catch2 fixtures atomically reserve their temporary
report directories, so independently launched focused executables do not race
for a process-local `…-1` path.
Requirements-Lab source/API contracts are also labelled by domain, so a domain
lane exercises implementation behavior and its nearby traceability checks
together. Labels are routing metadata only: a passing lane is development
feedback, not JUnit evidence, protected review, or a conformance promotion.

Use the named CMake targets from an already configured build tree:

~~~powershell
# Tight edit/build/test loop: foundational checks plus Catch2 [unit] cases.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_rapid

# Core binding/runtime utility slice, including the tag-taxonomy guardrail.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_foundation

# A single-case FOM catalog lane: preserves directed-interaction P/S capability
# declarations without re-running the whole FOM validator or runtime suite.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_fom_directed_interaction_sharing

# The complete Annex C composition slice: C.1-C.10 Catch2 cases plus the
# exact composition/table/merge Requirements-Lab contracts, without running
# the broader FOM or federation-management lanes.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_fom_annex_c

# Equivalent direct CTest routing when the Catch2 target is already built.
ctest --test-dir out/cmake/fom-services -C Debug -L '^annex-c$' --output-on-failure

# A single equal-to-cutoff Connection Lost/TSO regression: it keeps the
# page-50 forced-delivery behavior fast without running every transport case.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_connection_lost_tso_cutoff

# A feature/domain lane: tagged Catch2 cases and matching Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federation_management

# The official floating reference-time slice: HLAfloat64Time selection,
# callback-gated regulation/lookahead, and grant-time logical-time mutation,
# with its focused Requirements-Lab contract.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_float_time

# A focused timestamped regional object-update lane: ordinary regional TSO
# cases, explicit-source association replacement, and both traceability checks.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_timestamped_regional_attribute_update

# A focused class-level regional Request/Provide lane: timestamped provider
# response, constrained-recipient ordering, and its composite traceability checks.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_timestamped_regional_request_provider_response

# A focused nonregional class-designator Request/Provide lane: subclass
# expansion, timestamped provider response, and constrained-grant ordering.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_object_class_request_provider_response

# A focused delayed-subscription lane: ordinary interaction/attribute cases
# plus the explicit-source regional timestamped attribute extension.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_delay_subscription_evaluation

# A narrow cross-cutting feature lane: filesystem lifecycle, MIM encoding,
# report routing, and the reporting/subscription interlock.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_service_reporting

# A one-service lane: bounded FQR scheduling, report-file encoding, and its
# matching Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_flush_queue_request

# A one-service lane: Retract type-33 report-file encoding and the bounded
# request-retraction behavior/contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_retract

# A one-service lane: Register Federation Synchronization Point's type-53/
# type-63/type-18-or-Null report-file encoding and matching contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_register_federation_synchronization_point

# A one-service lane: Confirm Synchronization Point Registration's successful
# Null and duplicate-label type-56 report forms before C++ result callbacks.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_confirm_synchronization_point_registration_service_report

# A one-service lane: recipient-local Announce Synchronization Point reports,
# late-join routing, and service-reporting-switch append gating.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_announce_synchronization_point_service_report

# A one-service lane: recipient-local Federation Synchronized type-53/type-18
# reports before HLA_EVOKED completion callbacks.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federation_synchronized_service_report

# A one-service lane: ordinary queued Initiate Federate Save recipient reports
# before HLA_EVOKED callbacks, with independent report-file serials.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_initiate_federate_save_service_report

# A one-service lane: Federation Saved recipient result forms (true/Null and
# false/SAVE_ABORTED) durable before HLA_EVOKED result callbacks.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federation_saved_service_report

# A one-service lane: Request Federation Save's type-53 label plus the
# type-34-Null/type-31-LogicalTime optional timestamp overload forms.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_request_federation_save

# A one-service lane: Request Federation Restore's type-53 Federation save
# label report and its confirmation-callback ordering.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_request_federation_restore

# A one-service lane: Confirm Federation Restoration Request's requester-local
# type-53 label/type-6 true-or-false result record before its callback.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_confirm_federation_restoration_request_service_report

# A one-service lane: Federation Restore Begun's no-argument recipient-local
# record at every joined federate before the HLA_EVOKED callback.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federation_restore_begun_service_report

# A one-service lane: Initiate Federate Restore's recipient-local label,
# joined-federate designator, and federate-name report before its callback.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_initiate_federate_restore_service_report

# A one-service lane: Federate Restore Complete's true/false Boolean report
# forms, callback ordering, and serial continuity across restored state.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federate_restore_complete

# A one-service lane: Abort Federation Restore's accepted no-argument report
# and ordinary RESTORE_ABORTED callback ordering.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_abort_federation_restore

# A one-service lane: Query Federation Restore Status's accepted no-argument
# report, idle response boundary, and rejected-save preservation.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_query_federation_restore_status

# A one-service lane: Federation Restore Status Response's type-20 descriptor
# record at the querying recipient before the HLA_EVOKED callback.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federation_restore_status_response_service_report

# A one-service lane: Resign Federation Execution's final type-44 action
# report, rejection preservation, teardown, and matching contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_resign_federation_execution

# A one-service lane: RTI-initiated Federate Resigned's final type-53 reason
# report, callback ordering, and matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federate_resigned_service_report

# A one-service lane: RTI-initiated Connection Lost's final type-53 fault
# report, best-effort callback ordering, and matching traceability contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_connection_lost_service_report

# A one-service lane: Federate Save Begun's successful no-argument report,
# accepted-save state boundary, and matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federate_save_begun

# A one-service lane: Federate Save Complete's true/false Boolean report forms,
# result-callback ordering, and matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federate_save_complete

# A one-service lane: Abort Federation Save's accepted no-argument report,
# ordinary SAVE_ABORTED callback ordering, and matching contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_abort_federation_save

# A one-service lane: Query Federation Save Status's accepted no-argument
# report, status-response callback ordering, and matching contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_query_federation_save_status

# A one-service lane: Federation Save Status Response's type-17 nested
# status-pair report durable before its HLA_EVOKED callback.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_federation_save_status_response_service_report

# A one-service lane: Change Interaction Order Type's type-27/type-38
# report-file encoding and its matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_change_interaction_order_type

# A one-service lane: Change Attribute Order Type's type-37/type-1/type-38
# report-file encoding and its matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_change_attribute_order_type

# A one-service lane: Change Default Attribute Order Type's type-36/type-1/
# type-38 report-file encoding and its matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_change_default_attribute_order_type

# A one-service lane: Change Default Attribute Transportation Type's
# type-36/type-1/type-59 report-file encoding and matching API/Requirements-Lab
# contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_change_default_attribute_transportation_type

# A one-service lane: Query Attribute Transportation Type's type-37/type-0
# report-file encoding, later response callback, and matching API/Requirements-Lab
# contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_query_attribute_transportation_type

# A one-service lane: Query Attribute Ownership's type-37/type-1 report-file
# encoding, later ownership-result callbacks, and matching API/Requirements-Lab
# contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_query_attribute_ownership

# A one-service lane: Unconditional Attribute Ownership Divestiture's
# type-37/type-1/base-64 tag report-file encoding, later ownership-assumption
# callback, and matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_unconditional_attribute_ownership_divestiture

# A one-service lane: Local Delete Object Instance's accepted type-37
# report-file encoding, local-forget transition, and matching
# API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_local_delete_object_instance

# A one-service lane: receive-order Delete Object Instance failure reporting in
# both the production filesystem and HLA_IMMEDIATE MOM interaction sinks,
# including the conditional Null/false/exception form and serial ordering.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_delete_failure

# A one-service lane: Publish Object Class Attributes' accepted type-36/type-1
# report-file encoding, declaration/ownership-work ordering, and matching
# API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_publish_object_class_attributes

# A one-service lane: Publish Object Class Directed Interactions' accepted
# type-36/type-28 report-file encoding, including the supplied-empty set form,
# and matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_publish_object_class_directed_interactions

# A one-service lane: Unpublish Object Class Directed Interactions' accepted
# type-36/type-28-or-Null report-file encoding for both official C++ overloads,
# including the supplied-empty set form and matching API/Requirements-Lab
# contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_unpublish_object_class_directed_interactions

# A one-service lane: Subscribe Object Class Directed Interactions' accepted
# type-36/type-28/type-6 report-file encoding, including default ownership,
# explicit universal, supplied-empty-set forms, and matching contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_subscribe_object_class_directed_interactions

# A one-service lane: Unsubscribe Object Class Directed Interactions' accepted
# type-36/type-28-or-Null report-file encoding for both official C++ overloads,
# including the supplied-empty set form and matching contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_unsubscribe_object_class_directed_interactions

# A one-service lane: Unpublish Object Class Attributes' accepted type-36/type-1
# report-file encoding, teardown/advisory ordering, and matching
# API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_unpublish_object_class_attributes

# A one-service lane: Subscribe Object Class Attributes' accepted
# type-36/type-1/type-6/type-53-or-Null report-file encoding, callback ordering,
# and matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_subscribe_object_class_attributes

# A one-service lane: Unsubscribe Object Class Attributes' accepted
# type-36/type-1-or-Null report-file encoding for both official C++ overloads,
# callback ordering, and matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_unsubscribe_object_class_attributes

# A one-service lane: Publish Interaction Class's accepted type-27
# report-file encoding, declaration-advisory ordering, and matching
# API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_publish_interaction_class

# A one-service lane: Subscribe Interaction Class's accepted type-27/type-6
# report-file encoding, active-to-passive selector mapping, declaration-advisory
# ordering, and matching API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_subscribe_interaction_class

# A one-service lane: Unpublish Interaction Class's accepted type-27
# report-file encoding, declaration-advisory ordering, and matching
# API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_unpublish_interaction_class

# A one-service lane: Unsubscribe Interaction Class's accepted type-27
# report-file encoding, declaration-advisory ordering, and matching
# API/Requirements-Lab contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_unsubscribe_interaction_class

# A callback-family lane: the four ordinary declaration relevance advisories
# write their recipient-local type-36/type-27 report before HLA_EVOKED delivery.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_declaration_relevance_advisory_service_report

# A callback-service lane: Discover Object Instance's recipient-local
# type-37/type-36/type-53/type-15 record is present at HLA_EVOKED callback entry.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_discover_object_instance_service_report

# A callback-service lane: receive-order Remove Object Instance's seven-slot
# recipient-local record is present at HLA_EVOKED callback entry.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_remove_object_instance_service_report

# A timestamped callback-service lane: separate immediate and TSO §6.17
# report records are present before the corresponding user callbacks.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_timestamped_remove_object_instance_service_report

# A callback-service lane: the provider's §6.22 record is present at explicit
# object-instance Provide Attribute Value Update callback entry.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_provide_attribute_value_update_service_report

# An RTI-invoked callback-service lane: Auto Provide's §6.22 record carries an
# empty tag and is present at provider callback entry.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_auto_provide_service_report

# A one-service lane: Query Interaction Transportation Type's type-15/type-27
# report-file encoding, later response callback, and matching API/Requirements-Lab
# contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_query_interaction_transportation_type

# A one-service lane: Request Interaction Transportation Type Change's
# type-27/type-59 report-file encoding and its matching API/Requirements-Lab
# contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_request_interaction_transportation_type_change

# A one-service lane: Request Attribute Transportation Type Change's
# type-37/type-1/type-59 report-file encoding and matching API/Requirements-Lab
# contracts.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_request_attribute_transportation_type_change

# Direct support-switch state, the MOM switch-update/interlock paths, the seven
# sourced successful-void report-file wrappers, and their seven nearby Lab/API
# contracts. Downstream regional callback behavior remains in the DDM lane by
# design.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_support_switches

# A narrow cross-cutting callback-model lane: direct HLA_IMMEDIATE regressions.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_callback_immediate

# Exact service or callback focus remains available through its Catch2 tag.
ctest --test-dir out/cmake/fom-services -C Debug --output-on-failure `
  -L '^rti[.]service[.]connection-lost$'

# Full CTest regression remains the merge, release, and scheduled gate.
cmake --build out/cmake/fom-services --config Debug --target umbra_test_all
~~~

The initially named domain targets are `foundation`, `federation_management`,
`time_management`, `ddm`, `mom`, `service_reporting`, `support_switches`, `fom`,
`object_management`, `ownership_management`, `declaration_management`,
`interaction_management`, and `save_restore`. The equivalent direct CTest form is `ctest -L
'^<domain-label>$'`; `ctest -N -L '<regex>'` shows the exact selected work
without executing it. `umbra_test_callback_immediate` is the additional
cross-cutting lane for direct `HLA_IMMEDIATE` behavior; it deliberately spans
domains rather than becoming a competing domain classification. The service
lanes `umbra_test_time_advance_request`,
`umbra_test_time_advance_request_available`, `umbra_test_next_message_request`,
`umbra_test_next_message_request_available`, and
`umbra_test_flush_queue_request`, `umbra_test_retract`, and
`umbra_test_change_interaction_order_type`,
`umbra_test_change_attribute_order_type`, and
`umbra_test_change_default_attribute_order_type`,
    `umbra_test_change_default_attribute_transportation_type`,
  `umbra_test_query_attribute_transportation_type`,
  `umbra_test_query_attribute_ownership`,
    `umbra_test_unconditional_attribute_ownership_divestiture`,
    `umbra_test_local_delete_object_instance`,
    `umbra_test_publish_object_class_attributes`,
    `umbra_test_unpublish_object_class_attributes`,
    `umbra_test_subscribe_object_class_attributes`,
    `umbra_test_publish_interaction_class`,
    `umbra_test_subscribe_interaction_class`,
    `umbra_test_unpublish_interaction_class`,
    `umbra_test_unsubscribe_interaction_class`,
    `umbra_test_declaration_relevance_advisory_service_report`,
    `umbra_test_discover_object_instance_service_report`,
    `umbra_test_regional_object_attribute_association_service_report`,
    `umbra_test_regional_object_attribute_subscription_service_report`,
    `umbra_test_regional_interaction_subscription_service_report`,
  `umbra_test_query_interaction_transportation_type`,
  `umbra_test_request_interaction_transportation_type_change`, and
`umbra_test_request_attribute_transportation_type_change` provide the tighter
edit/build/test loop.

Every new Catch2 scenario must retain one execution-scope tag (`unit` or
`integration`), one domain tag, and—where applicable—the exact RTI service or
Federate Ambassador callback tag. New Requirements-Lab contracts should use a
domain-bearing name so the CMake label mapping includes them; extend that
mapping when a new domain cannot be expressed by the existing vocabulary. New
named one-service targets must use `umbra_add_ctest_service_lane`, rather than
the generic lane helper, so the catalog audit automatically requires behavior,
Requirements-Lab, and API-contract membership. The aggregate `ctest` suite
remains mandatory before integration: lanes reduce the inner-loop cost, but
they are not a substitute for regression coverage.

One focused-lane maintenance edge is worth keeping explicit: an unanchored
CMake name regex can silently pull a neighboring requirement family into the
wrong service lane (for example, an object-class contract matching an
object-instance prefix). Use an exact, end-anchored contract-name expression
for each lane and run `focused_service_lane_catalog` after changing the
mapping. This is an Umbra slicing hazard, not a Requirements Lab defect, and
the catalog check is the guard against accidentally widening a lane.

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
appends the selected-file §4.4 fault report before queuing the official
Connection Lost callback, and releases the completed writer after that
invocation. A second multi-federate case
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
callback despite the Lab crosswalk issue logged as RL-025. During a transport
fault, the bounded MOM route now queues `HLAreportFederateLost` for current
ordinary or DDM-matching regional subscribers before automatic-resign
consequence callbacks. It records the lost federate's MIM identity, name,
current granted time, and fault description with `HLAreliable` receive order.
The default-invalid callback
producer handle is a local adapter choice for this RTI-originated interaction,
not a source-backed handle mapping; RL-065 retains that distinction. The
paired `compliance/requirements-lab/federate-lost-report-requirements-contract.json` and
`compliance/requirements-lab/federate-lost-report-api-contract.json` pin the two direct
page-50 candidates and the exact non-timestamped Receive Interaction
declaration. The separate connection-loss contracts and focused Catch2 lane
now pin page-50 candidates `l24-7` and `l27-8`: one time-regulating publisher
is granted time 6, faults before its constrained subscriber evokes, and a
queued time-5 interaction is delivered before the survivor's time-6 grant;
queued time-6 interactions are likewise delivered before that grant both when
the subscriber's TAR was already pending and when it is requested after the
loss. A timestamp-6 directed interaction also survives its regulating
publisher's forced resignation and reaches a universal constrained subscriber
before the matching grant when its target remains alive; the callback keeps the
target and selector rechecks rather than treating the source snapshot as a
blanket delivery override. A reliable timestamp-6 attribute update likewise
retains its reflection value and metadata through source resignation before the
survivor's matching grant. Its `DELETE_OBJECTS` collision companion keeps that
TSO reflection distinct from the automatic receive-order removal: the
reflection reaches time 6 before the grant, then the default disabled
asynchronous-delivery gate holds the removal until the survivor's next TAR.
The implementation captures the cutoff before resignation and marks the
existing message ledger rather than inventing a new delivery queue. It
deliberately does not assert a later timestamp, for which the source permits
either result. Multi-recipient ordinary/direct interaction, alternate-advance,
multi-passel/regional update, remaining automatic-resign collision directives,
remaining selector/lifecycle matrices under loss, remote transport, JUnit/protected review, and
the conformance matrix remain open. The bounded `NO_ACTION` forced-resign
case now proves that forced cleanup retains the known object, divests the lost
member's owned attributes, offers them to an eligible survivor, and emits no
automatic removal callback. A separate final-member Connection Lost case sets
`NO_ACTION`, exercises the 4.12.4 directive-two override, and verifies the
deleted object's name can be reserved and registered after a fresh Join. Other
federation-event callbacks, the remaining forced-resign disposition policy,
and complete object/ownership behavior remain open. The focused
`connection-lost-negotiated-cancellation` lane additionally starts a regular
acquisition, replaces the queued owner release with negotiated-divestiture
confirmation work, and verifies that forced directive three suppresses both
owner callbacks before the surviving owner is resigned. The paired
`compliance/requirements-lab/connection-lost-requirements-contract.json` and
`compliance/requirements-lab/connection-lost-api-contract.json` pin the Connection Lost source
requirements, the page-50 inclusive cutoff candidates, and the exact
`FederateAmbassador::connectionLost` declaration. Their focused error-path
lane also proves the embedded fault hook is one-shot: a duplicate fault is
rejected, the callback is delivered once, and a fresh Connect is required
before the ambassador can re-enter the normal lifecycle. The Catch2 result
remains development-profile traceability until JUnit and protected review
exist. Its
focused `connection-lost-report-ordering` lane uses two subscribed survivors
and one time-regulating lost publisher with `DELETE_OBJECTS`. It drains each
survivor independently and proves that `HLAreportFederateLost` arrives before
that survivor's automatic `Remove Object Instance` callback, while leaving any
global ordering between independent survivor queues unspecified. RL-096 records
the Lab's missing fan-out/per-recipient ordering relation; this is still source
and test traceability, not a MOM or conformance claim. The adjacent directed
and regional selector-mutation lanes now cover callback-boundary rechecks after
the same source fault; RL-097 and RL-098 record their separate Lab
cross-cutting relation gaps.
The same profile's joined federates can obtain a caller-owned factory for
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
`Reflect Attribute Values`; it does not imply RTI-automatic provision. A
class-level regional companion additionally invokes timestamped `Update
Attribute Values` from that callback and verifies the constrained recipient's
reflection before its grant, including source-region and retraction metadata.
This is one explicit provider-response composition, not RTI-automatic
provision. Regional forms beyond the bounded request cases, alternate advances,
broader DDM, and ensuing timestamped/retraction behavior remain outside this
slice. The sibling
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

The focused `ownership-assumption-research` lane closes one narrow terminal-
callback hole without widening that claim. When an unconditional-divestiture
assumption callback is queued and its candidate unpublishes before callback
delivery, the registry suppresses the stale callback and releases that
candidate's outstanding assumption reservation. A later publication therefore
continues the still-unowned search and queues exactly one fresh grouped
`Request Attribute Ownership Assumption` callback carrying the originating
divestiture tag. The paired
`compliance/requirements-lab/ownership-assumption-research-requirements-contract.json` and
`compliance/requirements-lab/ownership-assumption-research-api-contract.json` record the
source/API traceability, and the Catch2 case remains development-profile
evidence only. Complete arbitration, RTI-owned state, remaining divestiture and
resignation forms, remote transport, JUnit/protected review, packaging, and
conformance remain open.

`compliance/requirements-lab/resign-action-requirements-contract.json` and
`compliance/requirements-lab/resign-action-api-contract.json` now record the bounded official
`ResignAction` transition. Ten Catch2 cases cover directive 1
(`UNCONDITIONALLY_DIVEST_ATTRIBUTES`) leaving owned attributes unowned and
offering current eligible federates through `Request Attribute Ownership
Assumption`, directive 2 (`DELETE_OBJECTS`) removing objects for which the
resigning federate owns `HLAprivilegeToDeleteObject`, the direct voluntary
directive 4 (`DELETE_OBJECTS_THEN_DIVEST`) deleting a delete-privileged object
before divesting a retained object's remaining attribute, standalone directive 3
(`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`) suppressing a queued release request
when the requester resigns, plus a direct directive-3 negotiated-divestiture
case that cancels the selected confirmation and suppresses both stale owner
callback forms, and directive 5
(`CANCEL_THEN_DELETE_THEN_DIVEST`) resolving the resigning federate's pending
acquisition work before cleanup. A third direct directive-3 case removes an
If Available reservation and suppresses its already-queued requester callback.
The cases also assert the official
`FederateOwnsAttributes` and `OwnershipAcquisitionPending` rejection paths,
plus the clause-4.12.4 rule that the final federate's `NO_ACTION` still
processes directive 2 and releases a delete-privileged object name.
Two additional cases cover the bounded 4.12.4 continuation triggers: a later
eligible publication and a later join followed by discovery. This remains
source/API traceability only: complete owner arbitration, remaining automatic-
resign directive combinations, RTI-owned state, all remaining ownership forms,
package/JUnit/protected-review evidence, and conformance are not claimed. The
separate `ownership-assumption-research` lane also covers terminal callback
suppression, reservation release, and re-offer with the originating
divestiture tag.
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
re-search is covered by the focused native regression; full owner arbitration
remains outside the slice.
The bounded `Negotiated Attribute Ownership Divestiture` path keeps the
current owner in private Waiting state, selects the earliest existing regular
or Willing-to-Acquire request, and invokes `Request Divestiture Confirmation`
with the selected acquisition tag. `Confirm Divestiture` transfers a confirmed attribute
before a standard acquisition notification carrying the confirm tag; an
explicit cancel clears the waiting record and restores ordinary regular-release
planning. A cancelled selected regular request yields `NoAcquisitionPending`
and resets the owner to waiting. Normal release reservations remain while their
regular acquisition is pending, but a queued release is consumed when it
reaches negotiated Waiting state; that prevents a negotiated/cancelled race
from duplicating the owner callback. The profile does not yet search for further candidates or
select candidates through a standards-level arbitration policy. The dedicated
Willing-to-Acquire case proves that the selected If Available reservation is
consumed before its stale callback can report unavailable and that the normal
acquisition notification carries the confirmation tag.
The adjacent `Is Attribute Owned By Federate` lookup is read-only and answers
only whether the invoking joined federate owns one valid known-instance
attribute. It also resolves only the mandatory 2025 `HLAreliable`
and `HLAbestEffort` transportation-type names and opaque handles for joined
federates, plus deterministic execution-scoped lookup for transportation rows
declared by the composed FDD. The embedded profile now implements bounded attribute default,
change, and query services plus interaction change and query services:
instance defaults are captured per federate, accepted changes commit at their
confirmation callback, and future ordinary/regional interaction sends and
attribute updates use the effective type. Custom transportation delivery and
general message transport remain unimplemented. Joined federates start at the selected initial
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
timestamped service families, alternate-advance coverage outside the
non-regional attribute-update TARA/NMRA/FQR and explicit-regional interaction
FQR cases, and general time-management support remain unimplemented.

compliance/requirements-lab/test-catalog.json has one real embedded Disconnect integration test.
Its raw sidecar manifest reports one implemented binding and one unreviewed,
real passed evidence record. It is deliberately not verified. Check its
source/test traceability and regenerate raw evidence with:

~~~powershell
$python = 'C:\Users\peanu\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $python tools/requirements_lab.py check --contract compliance/requirements-lab/connection-implementation-contract.json
cmake --build out/cmake/default --target umbra_catch2_junit --config Debug
& $python tools/requirements_lab_sidecar.py raw --python $python --catalog compliance/requirements-lab/test-catalog.json --results out/cmake/default/compliance/catch2-results.xml
~~~

The aggregate C++ Connect mapping still has no selected surface in the Lab
sidecar request, so its real tests remain outside the catalog. The callback
control methods and development-profile federate/object-class/interaction-class/attribute/parameter/object-instance lookup, listing,
object discovery, and time reports have API-surface records but no Requirements-Lab implementation
mapping, catalog record, or public package support, so they remain outside the
catalog. The same is true of
the remaining development-profile federation-management scenarios: they remain
in `compliance/requirements-lab/catch2-test-plan.json` until the service and packaging scope are
stable. The Lab JUnit adapter consumes already-produced test output; it does
not create or promote conformance evidence itself.
