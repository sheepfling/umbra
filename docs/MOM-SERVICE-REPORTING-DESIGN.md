# MOM Service Reporting Design

## Status

This is an embedded-profile design with a bounded filesystem and private
joined-federate-MOM foundation. Umbra now creates a real per-joined-federate
report file and its Table 5 initial record, then retains an unpublished
RTI-owned MIM-object snapshot with the exact file location. It does not yet
emit `HLAreportServiceInvocation`, register/discover/reflect a public MOM
object or attribute, provide a Requirements Lab validation record, or claim
conformance.

## Authorities

- IEEE 1516.1-2025 §11.5.1 supplies the service-reporting behavior and the
  mutual exclusion with a federate's subscription to the exact report-service
  interaction.
- IEEE 1516.1-2025 §11.5.2--§11.5.2.1 supplies the report-file lifetime,
  initial-record, and brace-delimited log-record rules.
- The vendored 2025 standard MIM defines
  `HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation`,
  its `HLAfederate` and `HLAserviceGroup` dimensions, seven parameters, and
  receive-order / reliable delivery.
- The official 2025 C++ headers remain the public ABI authority.  Umbra must
  not publish a parallel public MOM or encoding API.

## Non-negotiable runtime model

Every report interaction requires one RTI-owned report endpoint update set.
Its private point region has:

- `HLAfederate` set from `normalizeFederateHandle` for the indicated joined
  federate;
- `HLAserviceGroup` set from `normalizeServiceGroup` for the reported
  service; and
- a private identity, so public region services cannot modify, delete, or use
  the endpoint as a caller-supplied update region.

The endpoint is not an ordinary federate-originated interaction.  In
particular, it must not pass through public `sendInteraction`: that path
requires caller publication and caller-owned regions, and would recursively
report the report itself.  A registry-owned reporting planner must instead
perform MIM class/parameter lookup, regional subscription selection, and
callback-time revalidation directly.

## Delivery and state invariants

1. A successful or failed HLA service invocation to or from a joined federate
   is reportable only when that federate's Service Reporting switch is enabled.
2. The subject federate's counter starts at zero and is incremented once for
   every report actually accepted for emission.  It is independent for each
   joined membership and is never advanced by an unsent report.
3. Report interactions use reliable, receive-order delivery and never trigger
   another report.  They bypass the subject's Service Reporting switch during
   their own delivery.
4. Regional subscriptions are selected against the private point region.  The
   normal DDM predicate and callback-time subscription recheck remain the
   common source of truth; the report endpoint is an additional internal
   producer form, not a relaxed special case.
5. A subject federate cannot become a report recipient for itself through the
   excluded exact subscription.  The existing two-way interlock remains the
   admission boundary and must be preserved atomically.
6. `HLAsendServiceReportsToFile` selects the required sink: when enabled for
   the subject federate, the generated interaction is recorded in that
   federate's unique service-report file **instead of** being sent to
   subscribers.  File naming/location remains implementation-defined, but
   bypassing either sink is not permitted.

## Filesystem report-file foundation

The embedded profile's production service-report store is filesystem-backed.
Its public convenience boundary is
`umbra::embedded::ServiceReportConfiguration { std::filesystem::path directory; }`
plus `makeEmbeddedRtiConfiguration`; callers configure a directory and are
never offered a production memory/store-backend choice. The helper serializes
that implementation-defined choice through the official opaque
`RtiConfiguration::additionalSettings` field as
`serviceReportDirectory=<directory>`, preserving the public IEEE ABI. If no
directory is supplied through the official configuration, the embedded profile
uses its own temporary-directory child. The RTI validates/creates the selected
directory at connect time and fails with `RTIinternalError` if it cannot create
or use it. It never silently substitutes an in-memory store.

At each successful join, Umbra allocates an RTI-owned collision-resistant file
path below that directory with exclusive no-replacement filesystem creation,
then writes one
`ServiceReportInitialRecord` before switch state is consulted. The joined
federate retains that writer and immutable path until resignation; switching
file reporting off and on only gates future record appends. A later rejoin gets
a new path. Records are directly concatenated brace-delimited JSON-like
records, with no newline or other separator, matching §11.5.2.1. The writer
opens only the already reserved path for an append: if an external actor
removes it, the append fails deterministically rather than silently recreating
or replacing the joined-federate's logical report file. A writer serializes
local concurrent appends, preserving each complete brace-delimited record;
cross-process append and failure behavior remains a deployment-profile test
item.

If the runtime store cannot create the writer, Join fails with a deterministic
`RTIinternalError` and Umbra rolls the just-created registry membership back
before exposing a successful join. It does not fall back to memory. A
test-injected failing store verifies this rollback; deployment-specific
permission/full-disk failure matrices remain future work.

`MemoryServiceReportStore` exists only as an internal unit-test seam. It is
not selected by a public/runtime profile option and does not advertise a
`memory://` location. The non-installed internal `UmbraRtiAmbassador` test
constructor is the only path that injects it; factory-created ambassadors
always construct `FilesystemServiceReportStore`.

The initial record currently captures the actual `RtiConfiguration` fields,
federation/FOM state, and joined federate identity available to the embedded
profile. Its host field is the documented implementation value
`umbra-embedded`; remote host discovery is not implemented.

For a production filesystem writer, the same Join transaction also establishes
one private `HLAobjectRoot.HLAmanager.HLAfederate` snapshot. It reserves an
`ObjectInstanceHandle` from the common federation namespace, holds the complete
effective MIM attribute set as metadata, captures an immutable private
`HLAfederate` point region, and encodes the seven direct initial values using
the official MIM types. The report-file value is the exact absolute pathname
returned by the writer; changing either report switch does not replace the
snapshot or its value, and resignation removes it. Test-only memory stores do
not establish this snapshot and never invent an advertised pathname.

`HLAFOMmoduleDesignatorList` is populated from FOM modules supplied by that
specific Join, not from the federation's accumulated FDD. The registry uses
the validation layer's canonical source identity to retain the first supplied
designator when the same module appears more than once.

This is deliberately not public object registration. The snapshot is held
outside the federate-created object map, so current object-management,
ownership, discovery, reflection, and removal callbacks cannot accidentally
treat it as a federate-produced object. A non-installed test inspection seam
verifies this state without manufacturing a callback producer handle.

The composed FOM catalog now retains each object's standard attribute
`updateType`, `updateCondition`, `valueRequired`, and `ownership` alongside
its existing type, sharing, transportation, and order fields. A Catch2 regression reads the
unmodified MIM's `HLAmanager.HLAfederate.HLAreportServiceFile` declaration
through that catalog. The private joined-federate snapshot consumes this
metadata to reject an inconsistent MIM: it accepts only Table 8's `Static`
entry or the vendored MIM's exact documented `Conditional` rule, rather than
treating an arbitrary update policy as equivalent. It neither registers an
object nor chooses a public publication event.

The catalog now also exposes a validated effective-attribute projection that
walks the object-class hierarchy without permitting a derived declaration to
hide an inherited policy. The corresponding MIM regression demonstrates why a
complete joined-federate MOM object cannot be assembled from
`HLAfederate`'s direct attributes alone: it inherits
`HLAprivilegeToDeleteObject` from `HLAobjectRoot`, and that inherited
attribute declares `DivestAcquire`, unlike the direct joined-federate
attributes' `NoTransfer` policy. This is a construction prerequisite, not an
ownership implementation or a claim that a federate may delete a MOM object.

The private snapshot now holds the real `HLAreportServiceFile` value, but it
does not expose that value through a public MOM attribute update. The embedded
profile's runtime decision is fixed: the eventual public initial update follows
the IEEE 1516.1-2025 Table 8 `Static` entry and uses the already allocated
immutable pathname. The vendored 1516.2 MIM calls the attribute `Conditional`
at the first time both switches become true; Umbra retains that cross-artifact
discrepancy in RL-041 and in the catalog, but it does not let the conflicting
field postpone pathname selection or create a second file on later enablement.

## MOM-object publication prerequisite

The standard MIM says that the RTI publishes `HLAmanager.HLAfederate` and
registers one object instance for every joined federate. It also requires the
RTI to satisfy Request Attribute Value Update through the normal attribute
update mechanism regardless of whether an attribute is static, periodic, or
conditional. Consequently, `HLAreportServiceFile` cannot be added as a
special callback, a synthetic `memory://` value, or a call through the public
federate-owned `registerObjectInstance` path.

The public registration path currently requires a joined federate's
publication and models its transfer-capable attribute ownership. A standards
facing MOM object layer needs a distinct, RTI-owned producer model instead.
It must not manufacture an invalid federate-handle sentinel to satisfy a
callback, and it must use the ordinary discovery, reflection, subscription,
and callback-time revalidation machinery once the correct producer-designator
rule is sourced.

The private report-routing foundation now carries an explicit RTI interaction
source rather than passing numeric `0` through the ordinary federate-sender
selector. That source can decide DDM/subscription eligibility without claiming
that it is a callback-visible `FederateHandle`; the existing public receive-
interaction queue still accepts only a joined-federate producer. This is an
intentional source gate, not a partial public delivery implementation.

The intended implementation order is:

1. Retain the MIM object-attribute policy in the composed catalog (complete).
2. Add a private RTI-owned object-instance foundation with a common-namespace
   identity, full effective-attribute metadata, seven encoded initial values,
   and an immutable `HLAfederate` dimension point (complete; unpublished).
3. Register the complete required joined-federate MOM object at join, retain
   its object-instance identity through resignation, and serve requested
   attribute values through the normal reflection route.
4. Implement the static, conditional, and periodic attribute update scheduler
   from catalog metadata and the relevant 2025 source rules.
5. Add `HLAreportServiceFile` to that complete object model with the already
   allocated filesystem pathname using the selected Table 8 static
   publication event.

This sequence deliberately avoids a partial one-attribute MOM implementation.
It gives all MOM attributes one RTI-owned object foundation and keeps public
binding APIs aligned with the official headers.

## Payload boundary

All seven MIM parameters must be present and use their declared 1516.2 data
types:

| Parameter | MIM type |
| --- | --- |
| `HLAservice` | `HLAunicodeString` |
| `HLAserviceType` | `HLAserviceTypeEnum` |
| `HLAsuccessIndicator` | `HLAboolean` |
| `HLAsuppliedArguments` | `HLAargumentList` |
| `HLAreturnedArgument` | `HLAargument` |
| `HLAexception` | `HLAunicodeString` |
| `HLAserialNumber` | `HLAcount` |

The standard argument-list is a variable array of the MIM's fixed-record
`HLAargument` type (not a variant record).  Umbra now has the first private
MIM encoder and byte-vector tests for that record plus all seven individual
report parameters. The 1516.2 Requirements Lab's `HLAvariableArray` source
requires a signed `HLAinteger32BE` **element count**, followed by properly
padded elements; it is not a byte-length prefix. Umbra applies that rule to
both `HLAargumentList` and the `HLAunicodeString` data element used by MOM
parameters. The public binding also now implements the required
integer, boolean, and Unicode primitives.  Each argument's Unicode value is
the Table 5 JSON-like textual depiction for its `HLAargumentType`; that table
must be modeled before a service wrapper is admitted. This remains an
encoding foundation: the registry now plans a private normalized endpoint and
uses the ordinary DDM subscription predicate for its recipients, but no report
is emitted until callback transport and a concrete service wrapper are
present. Serial reservation is already atomic with the selected interaction or
file sink and starts at zero for each joined federate.

The same rule is now enforced for the public C++ handle values that a future
joined-federate MOM object must place in `HLAfederateHandle` and related MIM
fields. IEEE 1516.1-2025 §12.12.5 requires each C++ handle helper to emit the
corresponding `HLA<type>Handle` data type defined by `HLAstandardMIM`; those
MIM types are dynamic `HLAvariableArray` values of `HLAbyte`. Umbra's internal
identity payload is deliberately fixed to eight octets, so `Handle::encode()`
emits a twelve-octet HLAvariableArray: the signed big-endian element count
`8`, then the eight opaque identity octets. This is an Umbra binding
foundation, not a claim that every RTI uses an eight-octet identity payload or
that interoperability has been demonstrated.

The original §11.5.1 prose confirms the MIM wire representation: both
`HLAsuppliedArguments` and `HLAreturnedArgument` use the `HLAargument` fixed
record, a void or failed service uses the `Null` argument type, and multiple
returns use an appropriate composite `HLAargumentType`.  The interaction
encoder follows that shared source rule.

There is deliberately no generic `ServiceReportRecord` file formatter. Table
5's log-record row calls the return field `ReturnArgument` without defining
that type and depicts an array-shaped `[null]` no-return value. Section
11.5.2.1 directs log records to Table 5, but does not state how that log-only
notation maps to the interaction's one `HLAargument` fixed record. The
§11.5.1 implementation-dependent wording applies to the textual depiction in
the `HLAargumentName` field; it does not declare the `ReturnArgument` alias or
authorize treating the interaction record as the file record. RL-042 preserves
that boundary. The private code therefore provides Table 5 primitive and
initial-record formatters plus the exact interaction encoder, but no guessed
file-record renderer. The filesystem foundation implements unique file
lifetime and initial records, but no service invocation currently reaches that
writer, and public `HLAreportServiceFile` discovery/reflection remains
unimplemented.

## Implementation order and evidence

1. Resolve the RTI-created callback producer-designator rule (RL-043), then
   connect the established joined-federate snapshot to ordinary public
   discovery/reflection and requested-value handling without changing its
   already allocated file identity. Preserve the Table 8/MIM discrepancy as
   RL-041 source traceability. Establish a source-backed, per-service Table 5
   `ReturnArgument` mapping before generated file records are enabled; do not
   reuse the interaction `HLAargument` formatter as a file-record substitute.
2. Complete the remaining public 1516.2 helpers required beyond the current
   report-payload foundation, using only the official C++ binding types.
3. Add direct tests for the private endpoint planner: normalized point
   coordinates, regional selection, callback-time withdrawal, and no recursive
   reporting.
4. Connect the private planner to callback transport and serial accounting,
   then wrap one fully specified existing public service family, including success
   and declared-exception outcomes, and prove the seven encoded parameters and
   serial behavior through Catch2.
5. Add the source/API and Requirements Lab contracts only for that concrete
   behavior.  Do not promote a catalog/JUnit/validation record until the Lab
   workflow has accepted real test evidence.
6. Extend report coverage service family by service family, connect generated
   file-sink records to the established writer, and implement
   `HLAreportException` as distinct work.

## Standards-gated future test matrix

The adjacent legacy Python RTI contributed useful test topology, not a runtime
model. Its report sink is a JSONL audit facility and must not be transplanted:
the 2025 behavior chooses exactly one generated-report sink. The extracted
matrix is tracked in the [legacy Python RTI scenario backlog](LEGACY-PYTHON-RTI-TEST-BACKLOG.md).
The first Umbra cases should establish the following independently reviewed
2025 expectations:

1. A three-state sink matrix: reporting disabled yields no generated report;
   reporting enabled with file reporting disabled yields exactly one eligible
   MOM interaction; both enabled yields exactly one report-file record and no
   MOM interaction.
2. A subject/witness observer topology across evoked and immediate callback
   models, including unsubscribe-after-plan and callback-time recipient
   revalidation.
3. Success and declared-exception coverage for one completed service family;
   an invalid incoming MOM adjustment must not create a positive service
   report.
4. Service-family expansion through declaration, ownership, time, and DDM
   paths only after each wrapper's arguments, return values, and exceptions
   have a reviewed 2025 mapping.
5. Multi-federate/rejoin lifecycle pressure: independent serials and files,
   no self-report recursion, toggle stability, and a new file only for a new
   joined-federate lifetime.
