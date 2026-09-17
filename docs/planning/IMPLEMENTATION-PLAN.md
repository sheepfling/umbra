# Native C++ RTI implementation plan

Use the short [query card](QUERY-CARD.md) and bounded work index before
opening this long-form plan. The detailed [query guide](QUERY-GUIDE.md) is a
reference, not a first-read. Start with
`python tools/query_rti_work.py queue --summary --compact`, which lists the
bounded open-family queue. Then `python tools/query_rti_work.py ready --summary
--compact` resolves exactly one planned/source or deliberate active-new-case
handoff, followed by
`python tools/query_rti_work.py work --compact`,
which prints the active work item,
current
lane, focused Catch2 selector, mapped 2025 subsections, and plan IDs. When the
selector is
marked `baseline_test`, it is an already-green starting point rather than a
new test to rerun; the actionable slice is the adjacent `next_work_query`.
`python tools/query_rti_work.py plan` prints this plan's section outline. The canonical
commands and source-of-truth links are recorded in
[ROADMAP-INDEX.json](ROADMAP-INDEX.json).
If a chosen family has exhausted implementation/source rows but retains an
unclassified source-located case, `ready --family <id> --summary --compact`
returns that mapping row directly with its source, plan id, and trace/focus/
check handles; no broad plan or Requirements-Lab search is needed.
When `ready` reports `handoff_kind=new-case`, the proposed lane is not
runnable yet; add its C++ `TEST_CASE` and mapped plan row first, then use the
emitted post-mapping focus/check handles.
Test and lane queries also report the derived `cpp/tests/...cpp:line` location
for each Catch2 case; use `query_rti_work.py check --lane <exact-tag>` as the
iteration-local mapping gate and unscoped `check` for strict whole-plan
reconciliation. The lane-scoped form keeps unrelated historical source drift
out of the active work loop while listing any unlocated rows. Use
`query_rti_work.py trace <exact-handle>` when the requirement-to-subsection
relationship itself is the question: it prints direct
`lab_requirement_id -> document_id:clause_id` rows beside the C++ source
location and API surfaces without expanding the full plan.
For a reverse lookup by one requirement or standard subsection, the
`--summary --compact` form uses the compact matrix renderer and narrows the
displayed direct pairs to that requested handle. The index records the process
TSO in-transit ACK seam and its public observer case explicitly; use that
bounded lane before adding broader transport forms.

The process-boundary slice is now green and is no longer an open
implementation queue: its 109 source-located Catch2 cases are mapped to the
current canonical 2025 requirement/section index and retain 5,239 indexed
Catch2 assertions. The newest bounded process TSO fan-out case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:24629` adds 133 assertions,
maps 20 Lab requirements to 13 canonical sections and 15 official C++ API
surfaces, and proves that two same-timestamp interactions plus one later
timestamped interaction are delivered FIFO to two subscribed receivers across
timestamp-five and timestamp-seven grants, retaining distinct tags and
retraction designators with the expected GALT/LITS boundary. Query it with
`focus process-tso-interaction-fanout`, `trace`, `matrix`, `check --lane`, or
its exact CTest title. The preceding same-timestamp multiple-message FIFO case
at `cpp/tests/ieee1516_2025_connection_catch2.cpp:24182` adds 51 assertions and
remains independently queryable. The preceding configured-process
changed-lookahead interaction case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:23702` adds 67 assertions and
remains independently queryable. The preceding configured-process
timestamped-attribute case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:23195` adds 70 assertions and
remains separately queryable. The preceding configured-process retraction-lifetime case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:22808` adds 45 assertions,
maps five Lab requirements to four canonical sections and 15 official C++ API
surfaces, and remains independently queryable. The preceding
regional two-receiver FIFO case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:21443` remains independently
queryable with 122 assertions, 25 Lab requirements, 15 canonical sections,
and 21 official C++ API surfaces. The predecessor single-receiver ordering
and suppression cases remain independently queryable; the earlier process TSO in-transit
observer at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:11510` adds 62 assertions,
maps 20 Lab requirements to 13 canonical sections and 12 official C++ API
surfaces, and proves public Query GALT/LITS at the timestamp-5 in-transit
boundary before the receive callback acknowledges delivery and the matching
grant is issued. Query it with `focus process-tso-in-transit`, `trace`,
`matrix`, `check --lane`, or the exact CTest title. Multiple-message/order,
fanout, directed/regional, re-enable/resignation,
package/JUnit, protected-review, interoperability, validation, and
conformance remain separate lanes. The preceding bounded zero-lookahead
GALT/LITS companion at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:19624` adds 28 assertions,
maps six Lab requirements to four canonical sections and six official C++ API
surfaces, advances a zero-lookahead regulator through an evoked Time Advance
Grant, and proves the exclusive integer GALT/LITS boundary for a second
federate. Query it with `focus process-query-time-bounds-zero-lookahead`,
`trace`, `matrix`, `check --lane`, or the exact CTest title. The preceding
queued-TSO LITS companion at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:19332` adds 31 assertions,
queues timestamped input for a constrained observer, preserves undefined GALT
after regulator disable, and reports the queued timestamp as defined LITS.
The preceding multi-federate available-bound companion at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:19125` adds 24 assertions,
excludes the requesting regulator from its own bounds, and proves that an
observer receives the other regulator's lookahead as defined GALT/LITS. The
preceding immediate undefined GALT/LITS companion at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:19015` adds 14 assertions,
proves immediate callback delivery, and preserves caller outputs; in-transit
TSO, zero-lookahead epsilon, grant scheduling, retraction, save/restore,
package/JUnit/protected-review evidence, interoperability, validation, and
conformance remain separate. The preceding bounded queued
NMR/NMRA case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12600` adds 101 aggregate
assertions across `HLA_EVOKED` and `HLA_IMMEDIATE`, maps 19 Lab requirements to
12 canonical sections, and proves timestamp-5 then timestamp-8 selection with
interaction-before-grant ordering for both request forms. Query it with
`focus process-time-advance-next-message-queued-focused`, `trace`, `matrix`,
`check --lane`, or the exact CTest title. The preceding bounded multiple-record
Flush Queue case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:18501` adds 110 assertions over
the configured process endpoint, maps 22 Lab requirements to 12 canonical
sections and 22 official C++ API surfaces, proves timestamp-5, timestamp-8,
and exact requested-frontier timestamp-10 interactions arrive FIFO before one
HLA_EVOKED or HLA_IMMEDIATE grant, suppresses a retracted timestamp-14 record before that
frontier, and delivers retained timestamp-12 on the later frontier while
actual and optimistic times remain separately reported. The preceding
GALT-frontier case
at `cpp/tests/ieee1516_2025_connection_catch2.cpp:18614` adds 57 assertions,
and the preceding
single-endpoint Flush Queue Request case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:18207` adds 16 assertions,
proving official logical-time request encoding, actual/optimistic grant
propagation, and HLA_EVOKED callback gating. The preceding handle-normalization case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:17455` adds 23 assertions over
the configured process endpoint, proving ServiceGroup and execution-issued
handle normalization without a process-local registry. The earlier order-name
lookup case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:17953` adds 14
assertions, and the federate-identity lookup case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:17756` adds 11 assertions for
active-name handle resolution and execution-scoped name retention after
resignation. The current merged-JUnit artifact contains 4,132 testcases with
zero failures/errors; the older m108 snapshot contained 2,733. The local-
delete process foundation adds a 9-assertion codec contract, extends the
registry-bound service integration to 44 assertions, and adds an 18-assertion
public two-federate endpoint integration. Receive-order Delete Object Instance
adds a 25-assertion codec contract and a 25-assertion public endpoint/removal-
callback integration; the timestamped public endpoint slice adds 64 assertions
under both callback models, and the timestamped regional Update Attribute Values
endpoint adds 86 assertions under both callback models, and the regional
subscription-removal endpoint adds 112 assertions under both callback models;
the disjoint regional-update endpoint adds 50, the remote regional
subscription/update endpoint adds 66, the regional-registration endpoint adds 34,
and the object-registration endpoint adds 20 under both callback models; the
public object-class subscription endpoint adds 24 under both callback models, and
the public object-discovery endpoint adds 44 under both callback models through
the official `discoverObjectInstance` callback. The private registry-binding
case adds a 10-assertion official encoded `Enable Time Regulation` and
retained-state boundary; the public lifecycle case adds a 4-assertion
callback-gating boundary. These are bounded process role-enable proofs, not
cross-federate grant scheduling or conformance evidence.
The three local-delete
rows map to IEEE 1516.1-2025
§6.18.1; the two new deletion rows map to §§6.16, 6.16.4, and 6.17.1. It includes the
public process region lifecycle, regional object registration, remote regional
subscription/update, timestamped regional update/reflect, regional
unsubscription and disjoint suppression, Attribute Relevance Advisory
transitions, directed delivery, and timestamped retraction before and after
receive. The eight installed-package lanes and their catalog verifier are also
green. Use the lane-scoped query card as the gate; do not reopen this slice to
look for the next task. The indexed plan count is derived by
`python tools/query_rti_work.py status --summary --compact`, including
explicit-disposition foundation rows and a small set of failing harness rows;
the latest bounded additions are the separately indexed process Modify
Lookahead lanes at `cpp/tests/ieee1516_2025_connection_catch2.cpp:12058` and
`:12133`, with exact Requirements-Lab and 2025 clause mappings recorded
alongside their green focused cases; GALT/LITS/TSO, role-disable, resignation,
save/restore, and conformance remain separate follow-on slices. A source-unlocated historical row is
reconciliation work, not an executable candidate.
The newest ownership-management slice is the public configured-process
Negotiated Attribute Ownership Divestiture plus cancellation path at
`cpp/tests/attribute_ownership_acquisition_catch2.cpp:1432`. It is deliberately
isolated as the exact `process-negotiated-divestiture-cancellation` lane: 34
assertions, three direct Requirements-Lab requirements, three canonical 2025
sections, and 16 official C++ API surfaces. The implementation uses typed
owner/object/attribute negotiated and cancellation requests/results, registry
pending-offer selection/removal, stale Request Divestiture Confirmation
suppression, restored owner-release routing through the existing ownership
receive fence, and callback-time delivery under `HLA_EVOKED`. Use `case`,
`trace`, `matrix`, `focus`, and `check --lane process-negotiated-divestiture-cancellation`
for the bounded handoff; direct confirmation delivery/Confirm Divestiture, push
behavior, assumption-search continuation, mixed/RTI-owned cases,
DDM/timestamped behavior, interoperability, package/JUnit/protected review,
validation, and conformance remain explicit follow-on gates. The completed
acquisition-cancellation, owner-release-denied, owner-release, regular, and If
Available process lanes remain directly queryable by their exact plan ids and
lanes.
The completed public object-discovery case is indexed with its source line, three
requirement ids, three canonical sections, six official API surfaces, and 44
assertions under both callback models. The connection support-types baseline is
indexed separately as an 11-assertion SDK-consumability case with an explicit
no-standalone-Lab-surface disposition. The external IEEE 1516-2010 ordered-
catalog compatibility slice is indexed with 13 passing assertions and clauses
`4.5.5`/`4.11.4`, explicitly separated from 2025 service conformance. The
strict 2025-mode rejection and mixed-edition composer guard are also indexed
as two- and seven-assertion compatibility slices against clause 4. The
ambiguous 202x edition-setting guard is indexed as a one-assertion explicit
no-standalone-Lab-surface policy row. The regional-unpublish update-region
lifetime slice is indexed with 17 assertions and 18 requirement anchors. The
receive-order deletion update-region lifetime slice is indexed with 17
assertions and 20 requirement anchors. The final object-removal callback
update-region lifetime slice is indexed with 27 assertions and 21 requirement
anchors. The Join advisory-switch seed slice is indexed with 16 assertions and
13 requirement anchors. The Join-time explicit NoAction automatic-resign slice
is indexed with 7 assertions and 9 requirement anchors. The active pointer now
records the order and transportation MOM service-classification slice as 66
assertions with 19 requirement anchors. The active pointer now points to the
joined-federate MOM/FOM-module snapshot slice is indexed with 12 assertions and
6 requirement anchors, and the completed report-file identity save/restore
slice is indexed with 46 assertions and 18 requirement anchors. The joined-
federate MOM regional discovery slice is indexed with 124 assertions and 17
requirement anchors. The bounded `service-report-file-lifecycle` lane is now
queryable alongside the service-report cases: three
source-located C++ cases, all mapped, with 232 assertions across 25
Requirements-Lab references and 19 canonical 2025 sections. Its public
discovery/reflection/removal case is at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:35806` (152
assertions under HLA_EVOKED and HLA_IMMEDIATE); the switch-cycle and
save/restore companions are at lines 35458 and 35683. Use the exact lane
`focus`/`matrix`/`trace` handles from `QUERY-CARD.md`; this lane is an
implementation traceability slice, not a conformance claim.
The Auto Provide service-report callback-boundary case is
also source-backed and green at
`cpp/tests/auto_provide_service_report_file_catch2.cpp:166` with 142
HLA_EVOKED assertions, five Requirements-Lab anchors, and four canonical 2025
sections (`1`, `6.1.10`, `11.5`, and `11.5.2`), plus six official C++ API
surfaces. Its exact title and stable
`auto-provide-service-report` lane are the handoff handles; the production
filesystem record is development-profile evidence, not a conformance claim.
The alternate-advance timestamped directed-interaction case is now likewise
standalone and source-backed at
`cpp/tests/timestamped_directed_interaction_alternate_advance_catch2.cpp:164`:
108 HLA_EVOKED assertions, 15 Requirements-Lab anchors, 10 canonical 2025
sections, and 11 official C++ API surfaces. Its stable
`timestamped-directed-interaction-alternate-advance` lane and exact CTest
filter are the bounded handoff; it covers target-qualified TSO delivery and
callback-before-grant behavior through FQR, TARA, and NMRA, while DDM,
save/restore, ownership, remote transport, package/review, interoperability,
and conformance remain separate.
The explicit object-instance Request Attribute Value Update baseline is also
standalone at
`cpp/tests/attribute_value_update_request_baseline_catch2.cpp:93`: 36
HLA_EVOKED assertions, eight Requirements-Lab anchors, three canonical 2025
sections (`6.21`, `6.21.5`, and `6.22`), and 15 official C++ API surfaces. Its
stable `attribute-value-update-request-baseline-state` lane proves known-
instance targeting, grouped current-owner solicitation, unowned/requester-
owned suppression, request-tag propagation, and evoked callback delivery.
Keep the sibling class overload, regional requests, automatic provision,
timestamped/retraction behavior, response delivery, and broader DDM,
ownership, save/restore, package/review, validation, and conformance work in
separate bounded lanes.
The nonregional object-class Request Attribute Value Update baseline is now
standalone at
`cpp/tests/object_class_attribute_value_update_request_baseline_catch2.cpp:94`:
43 HLA_EVOKED assertions, seven Requirements-Lab anchors, three canonical 2025
sections (`6.21`, `6.21.5`, and `6.22`), and 15 official C++ API surfaces. Its
stable `object-class-attribute-value-update-request-baseline-state` lane proves
class-designator expansion over two concrete subclass instances, per-instance
owner callbacks, unowned/requester-owned suppression, request-tag propagation,
and evoked delivery. Keep regional class requests, automatic provision,
timestamped/retraction behavior, response delivery, and broader DDM, ownership,
save/restore, package/review, validation, and conformance work separate.
The class-designator service-report companion is now standalone at
`cpp/tests/object_class_attribute_value_update_service_report_catch2.cpp:196`:
403 HLA_EVOKED assertions, six Requirements-Lab anchors, five canonical 2025
sections (`6.21`, `6.21.5`, `6.22`, `11.5`, and `11.5.2`), and 19 official C++
API surfaces. Its production filesystem store creates one immutable file per
joined federate, records serial-ordered Table 5 type-37/type-1/type-63 records
before each provider callback, carries the base64 request tag, and leaves the
requester file unchanged. Use the stable
`object-class-provide-attribute-value-update-service-report` lane and exact
CTest filter; object-instance reporting, request-argument preservation,
regional/automatic-provision variants, timestamped/retraction behavior, public
MOM, package/review, validation, and conformance remain separate.
The Request Attribute Value Update argument-preservation companion is now
standalone at
`cpp/tests/request_attribute_value_update_service_report_catch2.cpp:204`:
242 HLA_EVOKED assertions, four Requirements-Lab anchors, four canonical 2025
sections (`6.21`, `6.21.5`, `11.5`, and `11.5.2.1`), and three official C++ API
surfaces. Its production filesystem report remains unchanged for an unknown-
object rejection, then records the accepted object-instance/class designator,
complete attribute set, and base64 tag before each separately queued provider
callback. Use the stable `request-attribute-value-update-service-report-file`
lane and exact CTest filter; regional requests, automatic provision, response,
generic failure/return records, public MOM, package/review, validation, and
conformance remain separate.

The process NMR/NMRA transport slice is also independently selectable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12492`. Its 22
`HLA_EVOKED` assertions map four exact 2025 Lab anchors across clauses 8.10.2,
8.11, and 8.11.3, proving distinct request operations, official logical-time
encoding, callback-gated grants, and Query Logical Time readback. Query it
with `focus process-time-advance-next-message-focused`, `trace`, `matrix`,
`check --lane`, or the exact CTest title. Queued TSO selection, GALT/LITS,
Flush Queue, multi-federate ordering, save/restore, package/JUnit, and
conformance remain separate process slices. The queued-TSO companion at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12600` is a 101-assertion,
two-federate `HLA_EVOKED`/`HLA_IMMEDIATE` case mapped to 19 Lab anchors and 12 canonical 2025
sections. It sends timestamped interactions at 5 and 8, proves NMR(10) selects
5 and NMRA(10) selects 8, and checks interaction-before-grant ordering. Query
it with `focus process-time-advance-next-message-queued-focused`, `trace`,
`matrix`, `check --lane`, or the exact CTest title; GALT/LITS, Flush Queue,
retraction, multi-recipient ordering, save/restore, and conformance remain
separate slices.
The next process object-instance lookup slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13040`: 18 assertions under
`HLA_EVOKED` and `HLA_IMMEDIATE`, five Lab requirements, two canonical 2025
subsections (`6.8.4`, `6.9.3`), and six official C++ API surfaces. It proves
known name↔handle resolution through the process service ledger and maps
unknown lookups to `ObjectInstanceNotKnown`. Use `focus object-instance-lookup`,
`trace`, `matrix`, `check --lane`, or the exact CTest title; discovery,
reservation/collision, deletion/reuse, DDM, save/restore, MOM, package/JUnit,
review, validation, interoperability, and conformance remain separate.
The next bounded process declaration slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13224`: 20 assertions under
`HLA_EVOKED` and `HLA_IMMEDIATE`, four Lab requirements, four canonical 2025
subsections (`10.4.6`, `10.6.2`, `10.13.2`, and `10.14.5`), and four official
C++ API surfaces. It resolves object-class and interaction-class names through
the federation-owned FOM catalog and maps unknown handles to the official
invalid-handle exceptions. Use `focus reverse-fom-lookup`, `trace`, `matrix`,
`check --lane`, or the exact CTest title; attribute, parameter, dimension,
transportation, multi-federate declaration management, package/JUnit, review,
validation, interoperability, and conformance remain separate.
The newest process reverse-FOM slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13411`: 24 assertions under
`HLA_EVOKED` and `HLA_IMMEDIATE`, four Lab requirements, three canonical 2025
subsections (`10.9.1`, `10.10.3`, and `10.16.1`), and four official C++ API
surfaces. It proves public attribute/parameter handle↔name resolution through
the federation-owned FOM catalog and maps unknown handles to
`AttributeNotDefined` and `InteractionParameterNotDefined`. Use the exact
`focus reverse-fom-lookup`, `trace`, `matrix`, `check --lane`, and CTest
handles; the dimension/transportation cases below remain separate from
multi-federate declaration management, package/JUnit, review, validation,
interoperability, and conformance.
The newest process reverse-FOM slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13628`: 16 assertions under
`HLA_EVOKED` and `HLA_IMMEDIATE`, six Lab requirements, three canonical 2025
subsections (`9.1.2`, `10.19`, and `10.20.4`), and four official C++ API
surfaces. It proves public dimension/transportation handle↔name resolution
through the federation-owned FOM catalog. Use the exact `focus
reverse-fom-lookup`, `trace`, `matrix`, `check --lane`, and CTest handles;
the m104 round-trip case remains distinct from the m105 error matrix, while
multi-federate declaration management, package/JUnit, review, validation,
interoperability, and conformance remain separate.
The preceding process reverse-FOM slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13785`: 16 assertions under
`HLA_EVOKED` and `HLA_IMMEDIATE`, six Lab requirements, three canonical 2025
subsections (`9.1.2`, `10.19`, and `10.20.4`), and four official C++ API
surfaces. It keeps unknown-name and invalid-handle behavior on a separate
process rejection fence with the public NameNotFound,
InvalidDimensionHandle, InvalidTransportationName, and
InvalidTransportationTypeHandle exceptions. Use the exact `focus
reverse-fom-lookup`, `trace`, `matrix`, `check --lane`, and CTest handles;
multi-federate declaration management, package/JUnit, review, validation,
interoperability, and conformance remain separate.
The latest process callback-ordering slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:14106`: 79 assertions under
`HLA_EVOKED` and `HLA_IMMEDIATE`, ten Lab requirements, six canonical 2025 subsections (`4`,
`5.1.4`, `6.12`, `6.12.4`, `6.13`, and `10.60.6`), and five official C++ API
surfaces. It joins one sender and two subscribed receivers, preserves each
recipient's interaction FIFO plus tags/parameters/producer identity, delivers
one receive callback per Evoke Callback, and excludes the sender from receive
callbacks. Use `focus process-multi-recipient-callback-ordering`, `trace`,
`matrix`, `check --lane`, or the exact CTest title; immediate delivery,
callback-disable, timestamped/region/directed fanout, package/JUnit, review,
validation, interoperability, and conformance remain separate.
The newest process TSO/DDM slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:15195`: 71 assertions under
`HLA_EVOKED`, 28 Lab requirements, 16 canonical 2025 subsections, and 20
official C++ API surfaces. Use the exact `focus
timestamped-process-regional-interaction`, `trace
m109.embedded-process-tso-regional-interaction`, `matrix`, `test`, `check
--lane`, and CTest handles. It proves overlap-qualified timestamped regional
delivery through the process endpoint, time-advance gating, conveyed
source-region metadata, and timestamp/order/retraction reconstruction. Keep
callback-disable, directed, relaxed-DDM, package/JUnit, review, validation,
interoperability, and conformance separate.
The preceding process DDM slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:14552`: 126 assertions under
`HLA_EVOKED` and `HLA_IMMEDIATE`, thirteen Lab requirements, seven canonical
2025 subsections, and thirteen official C++ API surfaces. Use the exact
`focus process-multi-recipient-regional-interaction`, `trace m108.embedded-process-multi-recipient-regional-interaction`, `matrix`, `test`, `check --lane`,
and CTest handles. It proves overlap-filtered delivery to two disjoint
regional recipients, send-time source-region metadata through Convey Region
Designator Sets, and sender exclusion; timestamped/retraction, directed,
relaxed-DDM, callback-disable, package/JUnit, review, validation,
interoperability, and conformance remain separate lanes.
The preceding process DDM metadata slice is independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13933`: 28 assertions under
`HLA_EVOKED` and `HLA_IMMEDIATE`, four Lab requirements, one canonical 2025
subsection (`9.1.2`), and two official C++ API surfaces. It routes inherited
available-dimension queries for object and interaction classes through the
federation-owned process catalog, preserves an empty result, and maps unknown
class handles to the official invalid-handle exceptions. Use `focus
process-available-dimensions-hierarchy`, `trace`, `matrix`, `check --lane`, or
the exact CTest title; process region lifecycle/routing, multi-federate
declaration management, package/JUnit, review, validation, interoperability,
and conformance remain separate slices.
The provider-side companion is now standalone at
`cpp/tests/provide_attribute_value_update_service_report_catch2.cpp:201`:
145 HLA_EVOKED assertions, six Requirements-Lab anchors, five canonical 2025
sections (`6.21`, `6.21.5`, `6.22`, `11.5`, and `11.5.2`), and five official
request/provider API surfaces. Its production filesystem file stays unchanged
while the provider callback is pending, then receives one serial-0 successful-
void Table 5 type-37/type-1/type-63 record at callback entry; the requester
file stays unchanged. Use the stable `provide-attribute-value-update-service-report-file`
lane and exact CTest filter; class, regional, automatic-provision, response,
public MOM interaction, timestamped/retraction, package/review, validation,
and conformance remain separate.
The receive-order `Update Attribute Values` service-report companion is now
standalone at
`cpp/tests/update_attribute_values_service_report_catch2.cpp:224`: 157
HLA_EVOKED assertions, four Requirements-Lab anchors, three canonical 2025
sections (`6.10`, `11.5`, and `11.5.2.1`), and two official C++ API surfaces.
Its production filesystem report stays unchanged for a rejected unknown-object
update, then records the accepted non-timestamped type-37 object designator,
type-2 AttributeHandleValueMap, type-63 base64 tag, and type-34 Null optional
timestamp before the separately queued reflection callback. Use the stable
`update-attribute-values-service-report-file` lane and exact CTest filter;
timestamped/regional/DDM forms, return/failure records, interaction-selected
delivery, public MOM, package/review, validation, and conformance remain
separate.
The receive-order `Delete Object Instance` service-report companion is now
standalone at
`cpp/tests/delete_object_instance_service_report_catch2.cpp:194`: 106
HLA_EVOKED assertions, five Requirements-Lab anchors, four canonical 2025
sections (`6.16`, `6.16.4`, `11.5`, and `11.5.2.1`), and two official C++ API
surfaces. Its production filesystem report stays unchanged for a rejected
unknown-object deletion, then records the accepted non-timestamped type-37
object designator, type-63 base64 tag, and type-34 Null optional timestamp
before the separately queued removal callback. Use the stable
`delete-object-instance-service-report-file` lane and exact CTest filter;
timestamped/regional/DDM forms, return/failure records, interaction-selected
delivery, public MOM, package/review, validation, and conformance remain
separate.
The receive-order `Delete Object Instance` failure-file matrix is now
standalone at
`cpp/tests/delete_object_instance_failure_service_report_catch2.cpp:135`:
137 HLA_EVOKED assertions, four Requirements-Lab anchors, three canonical 2025
sections (`6.16`, `6.16.4`, and `11.5`), and three official C++ API surfaces.
It verifies failed serial-0 and serial-2 records around accepted serial-1
deletion, preserving type-37/type-63/type-34 supplied forms, a Null returned
argument, false indicators, and exact exception text. Use the stable
`delete-object-instance-failure-file-matrix` lane and exact CTest filter;
timestamped failure, regional/DDM, MOM-interaction, package/review, validation,
and conformance remain separate.
The receive-order `Delete Object Instance` MOM failure matrix is now
standalone at
`cpp/tests/delete_object_instance_failure_service_report_interaction_catch2.cpp:84`:
124 HLA_IMMEDIATE assertions, four Requirements-Lab anchors, three canonical
2025 sections (`6.16`, `6.16.4`, and `11.5`), and three official C++ API
surfaces. It decodes object-management service type 2 through
`HLAreportServiceInvocation`, verifying failed serial-0 and serial-2 reports
around accepted serial-1 deletion with type-37/type-63/type-34 supplied forms,
Null return, false indicators, and exact exception text. Use the stable
`delete-object-instance-failure-mom-interaction` lane and exact CTest filter;
filesystem, timestamped, regional/DDM, package/review, validation, and
conformance remain separate.
The accepted ordinary regional `Send Interaction With Regions` filesystem
matrix is now standalone at
`cpp/tests/regional_interaction_service_report_file_catch2.cpp:185`: 173
HLA_EVOKED assertions, one Requirements-Lab anchor, canonical §11.5, and six
official C++ API surfaces. It verifies an overlap-qualified send is recorded on
the real configured joined-federate file before constrained Receive Interaction
delivery, including type-27/type-40/type-43/type-63/type-34 forms, the type-34
Null return, and source-region/payload/tag metadata. Use the stable
`ordinary-regional-interaction-service-report-file` lane; its MOM companion,
failure rows, timestamped/re-enable/save/restore, transport, package/JUnit/
protected-review, validation, and conformance remain separate.
The accepted ordinary regional MOM companion is now standalone at
`cpp/tests/regional_interaction_service_report_interaction_catch2.cpp:128`:
107 assertions across HLA_EVOKED publisher/receiver and an HLA_IMMEDIATE
observer, two Requirements-Lab anchors, canonical §11.5, and five official C++
API surfaces. With file reporting disabled, it decodes the accepted serial-zero
service type 2 `SendInteractionWithRegions` report through
`HLAreportServiceInvocation` before the constrained callback, retaining the
type-27/type-40/type-43/type-63/type-34 forms, type-34 Null return, success,
source-region, payload, producer, and tag metadata. Use the stable
`ordinary-regional-interaction-service-report-mom-interaction` lane and keep
filesystem, failure, timestamped/re-enable/save/restore, transport,
package/JUnit/protected-review, validation, and conformance separate.
The ordinary regional `Send Interaction With Regions` failure-file matrix is
now standalone at
`cpp/tests/regional_interaction_failure_service_report_file_catch2.cpp:151`:
302 HLA_EVOKED assertions, one Requirements-Lab anchor, one canonical 2025
section (`11.5`), and three official C++ API surfaces. It drives invalid
interaction-class, parameter, and region calls through the configured real
joined-federate filesystem and verifies serials zero through two, exact
type-27/type-40/type-43/type-63/type-34 supplied forms, Null returns, false
indicators, and exception text. Use the stable
`ordinary-regional-interaction-failure-service-report-file` lane and exact
CTest filter; the paired public MOM interaction, accepted regional delivery,
timestamped/re-enable/save/restore, transport, package/JUnit/protected-review,
validation, and conformance rows remain separate, with RL-105/RL-152 recorded
as the bounded Lab caveats.
The paired HLA_IMMEDIATE MOM-interaction matrix is standalone at
`cpp/tests/regional_interaction_failure_service_report_interaction_catch2.cpp:88`:
149 assertions, two Requirements-Lab anchors, canonical §11.5, and five
official C++ API surfaces. With file reporting disabled it decodes the same
three failure reports through `HLAreportServiceInvocation`, preserving the
type-27/type-40/type-43/type-63/type-34 forms, Null return, false indicator,
and exact exception text without an application callback. Use the stable
`ordinary-regional-interaction-failure-mom-interaction` lane; filesystem,
accepted regional, timestamped/re-enable/save/restore, transport, package/
JUnit/protected-review, validation, and conformance remain separate.
The ordinary provider-response spine is now standalone at
`cpp/tests/attribute_value_update_response_catch2.cpp:129`: 37 HLA_EVOKED
assertions, six Requirements-Lab anchors, three canonical 2025 sections
(`6.10`, `6.21`, and `6.21.5`), and four official C++ API surfaces. It proves
the known-object Request Attribute Value Update → Provide Attribute Value
Update → no-time Update Attribute Values → Reflect Attribute Values chain,
including request/response tags, reliable transport, producer identity, and
the absent sent-region designator. Use the stable
`attribute-value-update-response` lane and exact CTest filter; regional/DDM,
automatic provision, timestamped/retraction, update-rate, save/restore,
package/review, validation, and conformance remain separate.
The regional provider-response DDM recheck is now standalone at
`cpp/tests/regional_attribute_value_update_provider_response_recheck_catch2.cpp:136`:
39 HLA_EVOKED assertions, four Requirements-Lab anchors, two canonical 2025
sections (`6.10` and `9.13.1`), and eight official C++ API surfaces. It queues
a no-time provider response from an overlap-qualified regional request, moves
the requester to a valid disjoint range before reflection delivery, and proves
callback-time DDM suppression. Use the stable
`regional-provider-response-ddm-recheck` lane and exact CTest filter; automatic
provision, successful post-mutation reflection, timestamped/retraction,
relaxed-DDM, save/restore, package/review, validation, and conformance remain
separate.
The joined-federate MOM `HLAfederateState` save/restore case is now also
standalone at
`cpp/tests/joined_federate_mom_federate_state_save_restore_catch2.cpp:138`.
It passes 152 assertions under HLA_EVOKED and HLA_IMMEDIATE, maps the single
§11.4.1 Lab requirement, and records 11 official C++ save/restore surfaces.
Use the stable `joined-federate-mom-federate-state` lane and its exact CTest
filter; broader MOM attributes and conformance remain open. The adjacent
joined-federate MOM deletable-object projection is now source-backed at
`cpp/tests/joined_federate_mom_deletable_object_count_catch2.cpp:108` with 55
HLA_EVOKED assertions, one §11.4.1 requirement, and 12 official C++ API
surfaces. It verifies the live `HLAprivilegeToDeleteObject` ledger directly and
through `HLAsetTiming` periodic reflection (0 → 1 → 1 → 0); use the exact
`joined-federate-mom-deletable-object-count` focus/trace/matrix/check handles.
Remaining MOM statistics and transport/package/conformance evidence stay
separate.
The joined-federate MOM receive-order queue projection is now source-backed at
`cpp/tests/joined_federate_mom_ro_length_periodic_catch2.cpp:134` with 63
HLA_EVOKED assertions, one §11.4.1 requirement, and 12 official C++ API
surfaces. It samples `HLAROlength` at the accepted direct MOM request boundary
so the recipient-scoped receive-order ledger reports 0 → 1 before callback
delivery, preserves 1 through `HLAsetTiming` periodic reflection, and returns
to 0 after the application callback. Use the exact
`joined-federate-mom-ro-length-periodic` focus/trace/matrix/check handles.
Deferred asynchronous/TSO variants, remaining MOM statistics, regional or
transport variants, and package/conformance evidence stay separate.
The joined-federate MOM `HLAupdatesSent` projection is now source-backed at
`cpp/tests/joined_federate_mom_updates_sent_periodic_catch2.cpp:108` with 56
HLA_EVOKED assertions, one §11.4.1 requirement, and 12 official C++ API
surfaces. It samples the accepted `Update Attribute Values` invocation ledger
as `HLAinteger32BE` (0 → 1 → 2) through direct MOM requests and preserves 2 in
the `HLAsetTiming` periodic reflection. Use the exact
`joined-federate-mom-updates-sent-periodic` focus/trace/matrix/check handles;
remaining MOM statistics, timestamped/update-rate and regional/transport
variants, and package/conformance evidence stay separate.
The joined-federate MOM `HLAobjectInstancesUpdated` projection is now
source-backed at
`cpp/tests/joined_federate_mom_updated_object_count_periodic_catch2.cpp:109`
with 77 HLA_EVOKED assertions, one §11.4.1 requirement, and 12 official C++
API surfaces. It retains distinct accepted object handles, proving direct
`HLAinteger32BE` values 0 → 1 → 1 → 2 for two updates to one object followed
by an update to a second object, and preserves 2 alongside `HLAupdatesSent=3`
in the `HLAsetTiming` periodic reflection. Use the exact
`joined-federate-mom-updated-object-count-periodic` focus/trace/matrix/check
handles; remaining MOM statistics, timestamped/update-rate and
regional/transport variants, and package/conformance evidence stay separate.
The joined-federate MOM `HLAobjectInstancesRegistered` projection is now
source-backed at
`cpp/tests/joined_federate_mom_registered_object_count_periodic_catch2.cpp:115`
with 112 HLA_EVOKED assertions, one §11.4.1 requirement, and 12 official C++
API surfaces. It samples the successful registration ledger as
`HLAinteger32BE` (0 → 1 → 2) through direct MOM requests; three accepted
updates establish `HLAupdatesSent=3` and `HLAobjectInstancesUpdated=2`, and
the `HLAsetTiming` periodic reflection preserves the complete snapshot. Use
the exact `joined-federate-mom-registered-object-count-periodic`
focus/trace/matrix/check handles; invalid registration paths, remaining MOM
statistics, timestamped/update-rate and regional/transport variants, and
package/conformance evidence stay separate.
The timestamped Delete Object Instance failure-file case is now standalone at
`cpp/tests/timestamped_delete_object_instance_failure_service_report_catch2.cpp:127`.
It passes 134 HLA_EVOKED assertions, maps five Lab anchors to four canonical
2025 sections, and records the failed unknown-object and earlier-than-lookahead
calls in the production filesystem report with deterministic argument forms,
exception text, and serials. Use the stable
`timestamped-delete-object-instance-failure-service-report` lane
and its exact CTest filter; accepted deletion, recipient callback/interactions,
and conformance remain separate.
The accepted timestamped Delete Object Instance sender-file companion is now
standalone at
`cpp/tests/timestamped_delete_object_instance_service_report_catch2.cpp:193`.
It passes 91 HLA_EVOKED assertions, maps four Lab anchors to four canonical
2025 sections, and proves the type-34 Null-return record is durable before the
recipient's timestamped removal callback. Use the stable
`timestamped-delete-object-instance-sender-file` lane and exact CTest
filter; regulated, regional, retraction, package/review, and conformance
evidence remain separate.
The time-regulated timestamped Delete Object Instance sender-file companion is
now standalone at
`cpp/tests/time_regulated_timestamped_delete_object_instance_service_report_catch2.cpp:206`.
It passes 172 HLA_EVOKED assertions, maps five Lab anchors to five canonical
2025 sections, and proves the type-33 MessageRetractionHandle sender record,
immutable file identity through a switch cycle, and callback-before-grant
ordering. Use the stable
`timestamped-delete-object-instance-time-regulated-sender-file` lane and exact
CTest filter; regional, alternate-advance, save/restore, package/review, and
conformance evidence remain separate.
The accepted timestamped Delete Object Instance MOM-interaction companion is
now standalone at
`cpp/tests/timestamped_delete_object_instance_service_report_interaction_catch2.cpp:145`.
It passes 104 assertions across HLA_EVOKED publisher/receiver and an
HLA_IMMEDIATE observer, maps five Lab anchors to five canonical 2025 sections,
and proves the type-33 MessageRetractionHandle report through
`HLAreportServiceInvocation` while file reporting is disabled, followed by
timestamped callback-before-grant ordering. Use the stable
`timestamped-delete-object-instance-service-report-interaction` lane and exact
CTest filter; regional, alternate-advance, save/restore, package/review, and
conformance evidence remain separate.
The timestamped Delete Object Instance MOM-failure matrix is now standalone at
`cpp/tests/timestamped_delete_object_instance_failure_service_report_interaction_catch2.cpp:85`.
It passes 92 HLA_IMMEDIATE assertions, maps five Lab anchors to four canonical
2025 sections, and proves deterministic unknown-object and earlier-than-
lookahead failure records through HLAreportServiceInvocation with type-37/
type-63/type-31 forms, the type-34 Null return, exact exception text, and
serials zero and one. Use the stable
`timestamped-delete-object-instance-failure-mom-interaction` lane and exact
CTest filter; accepted/retraction delivery, recipient callbacks, regional,
package/review, and conformance evidence remain separate.
The active pointer now points to the optional
federation-management regional Request Attribute Value Update provider-response
case at `cpp/tests/ieee1516_2025_federation_management_catch2.cpp:3786`, now
indexed with 44 assertions and 20 requirement anchors. The timestamped Delete
Object Instance retraction case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4305` is now green
with 49 HLA_EVOKED assertions. Its callback-drain boundary consumes the
joined-federate MOM work item after Enable Time Regulation before asserting the
timestamped deletion, retraction, reconstitution, removal, and grant-order
path. The known-class-disabled
attribute-relevance case at
`cpp/tests/attribute_relevance_known_class_disabled_subscription_catch2.cpp:97` is now green
with 35 HLA_EVOKED assertions, 18 Requirements-Lab anchors, 13 canonical 2025
sections, and 18 official C++ API surfaces. The paired static known-class-
enabled case at
`cpp/tests/attribute_relevance_known_class_enabled_subscription_catch2.cpp:97` is also green
with 30 HLA_EVOKED assertions, 15 Requirements-Lab anchors, 13 canonical 2025
sections, and 17 official C++ API surfaces. The update-rate passive-regional-
subscription case at
`cpp/tests/update_rate_passive_regional_subscription_catch2.cpp:73` is also green
with 43 HLA_EVOKED assertions, 15 Requirements-Lab anchors, 12 canonical 2025
sections, and 21 official C++ API surfaces. The custom-transportation
handle-stability case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:44136` and the
restored-baseline timestamped MOM interaction case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4538` are mapped and
green. The restored-baseline regional Provide Attribute Value Update case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4697` is green with
251 assertions and 11 Requirements-Lab anchors. The three-dimensional
regional object-attribute overlap case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:47660` is green with
57 assertions and 10 Requirements-Lab anchors. The restored-baseline regional
Request Attribute Value Update solicitation case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4999` is green with
58 assertions, 9 Requirements-Lab anchors, 7 canonical 2025 sections, and 20
official C++ API surfaces. The restored-baseline timestamped regional Update
Attribute Values MOM failure case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:5461` is green with
138 assertions, 11 Requirements-Lab anchors, 9 canonical 2025 sections, and 30
official C++ API surfaces. The restored-baseline timestamped Update Attribute
Values file-failure case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:5863` is now green
with 218 HLA_EVOKED assertions, 11 Requirements-Lab anchors, 9 canonical 2025
sections, and 15 official C++ API surfaces. The m46 regional reflection-order,
m47 Unpublish Object Class Attributes, m48 partial ownership cancellation, and
m49 pre-delivery ownership cancellation slices are now mapped and green with
206, 102, 48, and 33 HLA_EVOKED assertions respectively. The m50 disabled Auto
Provide discovery baseline is also mapped and green with 21 assertions. The
m51 Federation Synchronized-after-resignation service-report case is mapped
and green with 23 HLA_EVOKED assertions. The m52 terminal TSO-designator case
is mapped and green with 40 HLA_EVOKED assertions. The m53 timestamped Update
Attribute Values queue/retraction case is mapped and green with 68 HLA_EVOKED
assertions at `cpp/tests/ieee1516_2025_federation_management_catch2.cpp:59032`.
The m54 mixed update-rate subscription case is mapped and green with 37
HLA_EVOKED assertions, eight Requirements-Lab anchors, two canonical 2025
sections, and 15 official C++ API surfaces at
`cpp/tests/mixed_update_rate_subscriptions_catch2.cpp:199`. The next
source pointer is the federation-teardown update-rate-history case at
`cpp/tests/federation_teardown_update_rate_history_catch2.cpp:158`; retrieve
that queue head with `python tools/query_rti_work.py next --pointer --compact`.
The m55 slice is green with 46 HLA_EVOKED assertions, nine Requirements-Lab
anchors, five canonical 2025 sections, and 15 official C++ API surfaces. The
next source head is the unplanned regional best-effort attribute-rate case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:59566`. The m56
slice is green with 47 HLA_EVOKED assertions, 19 Requirements-Lab anchors, 11
canonical 2025 sections, and 22 official C++ API surfaces. The m57 slice is
green with 57 HLA_EVOKED assertions, 30 Requirements-Lab anchors, 19 canonical
2025 sections, and 28 official C++ API surfaces. The m58 slice is green with
120 assertions, three Requirements-Lab anchors, one canonical Section 8.1.10
mapping, and 22 official C++ API surfaces under HLA_EVOKED and HLA_IMMEDIATE.
The m59 timestamped directed-interaction slice is green with 166 assertions
under HLA_EVOKED and HLA_IMMEDIATE in the focused target at
`cpp/tests/delay_subscription_evaluation_timestamped_directed_interaction_catch2.cpp:129`;
its callback drain consumes the queued Time Regulation Enabled notification
before the TSO assertions. Query it with
`python tools/query_rti_work.py trace m59.embedded-delay-subscription-evaluation-timestamped-directed-interaction --summary --compact`.
The m60 timestamped directed-interaction TSO/retraction slice is green with 56
HLA_EVOKED assertions in the focused target at
`cpp/tests/timestamped_directed_interaction_retraction_catch2.cpp:150`; it
applies the same callback-drain boundary before verifying retraction and
delivery. Query it with
`python tools/query_rti_work.py trace m60.embedded-timestamped-directed-interaction-tso-retraction --summary --compact`.
The next
source head is the immediate timestamped directed-interaction source-resignation
case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:62516`; m61 is green
with 45 HLA_EVOKED assertions. The m62 declaration at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:63554` is a disabled
 `#if 0` malformed source artifact with no executable evidence. The m63
 regional source-region snapshot case is green with 42 HLA_EVOKED assertions
 at `cpp/tests/ieee1516_2025_federation_management_catch2.cpp:69776`. The m64
 timestamped regional interaction TSO/retraction case is green with 68
 HLA_EVOKED assertions at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:75428`. The m65
 public HLAfloat64Time representation case is green with 31 assertions at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:76784`. The m66,
 m67, m68, and m69 no-TSO GALT/NRG cases are green with 21, 15, 27, and 24
 assertions at lines 77307, 77349, 77382, and 77428 respectively. The source
 queue for this translation unit is now exhausted; select the next bounded
 service family from the indexed roadmap with `queue`, `work`, or `focus`.
The accepted and invalid-directory companions, AlreadyConnected ordering,
unsupported-callback, absent-Disconnect, AttributeHandle, and callback-route
baselines are already explicitly indexed; their private/internal exceptions are
kept separate from conformance promotion.
The official `RtiConfiguration::rtiAddress` now selects a real `tcp://host:port`
endpoint for the public Connect surface, and the bounded public process slice
covers Create, Join, NoAction Resign, receive-order Send Interaction,
server-owned interaction/parameter handle lookup, ordinary interaction
declaration state, and object-class publication plus unnamed object
registration through the private `ProcessFederationClient`. Public ordinary
and timestamped Receive/Evoke delivery now have dedicated mapped
process-boundary baselines, and the ordinary case also proves Evoke Multiple
 plus EVOKED callback enable/disable gating. Ordinary Subscribe Object Class
 Attributes now crosses a dedicated private process service operation; its
 package projection invokes the official public subscription call and the
 process service projects automatic discovery through the official callback.
 Ordinary Update Attribute Values now has
 mapped private-service, public-endpoint, and official Reflect callback
 baselines.
timestamped process Update Attribute Values now has a paired private/public
baseline: the private service case covers the transport/registry contract and
the public endpoint case covers the official retraction handle, timestamped
reflection, and reflection-before-grant ordering. Query
`process-tso-attribute-before-grant` for the two-case focus card; it totals 114
`HLA_EVOKED` assertions and maps 17 Requirements-Lab anchors across 12
canonical 2025 sections. The public case also observes Query GALT/LITS at the
in-transit timestamp before the reflection callback acknowledges delivery.
Keep retraction stress, multiple-message ordering,
fanout, regional/DDM, save/restore, package/JUnit, validation,
interoperability, and conformance as separate lanes.
The installed profile has separate
ordinary, timestamped, parameterized, connection-loss, object-registration,
named-registration, and ordinary attribute-update/Reflect two-client package
  lanes. The focused named-registration process case and installed package lane
	are green, including duplicate-name and illegal-name error mapping. The public
 process region lifecycle/regional-registration, remote regional
subscription/update, timestamped regional update/reflect, regional
 unsubscription suppression, and disjoint suppression cases are green; the
private m24 advisory projection is green, and the public m25 advisory bridge is
green under both callback models. The callback bridge now has a focused private callback-entry
suppression case; regional advisory transitions and directed retraction are
complete. Select the next bounded service family through
`query_rti_work.py work`, `next`, and an exact lane query; if no source-located
candidate exists, add one new indexed C++ case with its requirement and 2025
subsection mapping instead of rescanning the unchanged Requirements Lab. No
Requirements Lab rescan is needed.
The installable-package smoke also verifies the eight indexed
package test/label pairs through `tools/verify_process_package_lanes.py` before running
the downstream consumer. Start with the exact lane/test handles from
`python tools/query_rti_work.py next --summary`; keep this evidence gate
separate from the completed terminal tombstone, Delete Object Instance TAR/NMR,
Flush Queue future-input, available-advance, mixed-fanout, TAR/NMR,
suppression, retraction, mixed-advance,
service-report, Send Interaction, directed, timestamped-directed,
target-departure, subscription-kind, save/restore, and other regional lanes.

```powershell
python tools/query_rti_work.py lane process-boundary --summary --compact
python tools/query_rti_work.py test "Private registry-bound service exchanges federation traffic across independently launched processes" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador resolves interaction and parameter handles through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador publishes object-class attributes and registers an object through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes ordinary interaction declarations through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador receives a process interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py test "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes a remote regional subscription and scoped update through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador preserves a timestamped regional update through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador removes a regional subscription through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador suppresses a disjoint regional update through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "Private process service projects owner-directed Attribute Relevance Advisory events" --summary --compact
python tools/query_rti_work.py test "Private process service projects owner-directed Attribute Relevance Advisory events" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers Attribute Relevance Advisory callbacks through a configured process endpoint under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers Attribute Relevance Advisory callbacks through a configured process endpoint under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py trace "RTIambassador removes a regional subscription through a configured process endpoint" --summary
python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
python tools/query_rti_work.py focus process-tso-attribute-before-grant --summary --compact
python tools/query_rti_work.py trace "Private process service releases timestamped Update Attribute Values before a constrained grant" --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^(umbra\\.process_boundary_private\\.catch2\\.Private process service releases timestamped Update Attribute Values before a constrained grant|umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassadors deliver a deferred timestamped process attribute update before the grant)$" --output-on-failure
python tools/query_rti_work.py test "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py trace "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py lane rti.service.subscribe-object-class-attributes --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Update Attribute Values through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers a process Update Attribute Values event through the official Reflect callback" --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-object-registration --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-named-registration --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-attribute-update --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_directed_retraction_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-directed-retraction --output-on-failure
```

The delivered regional provider-response no-replay companion is now green
under both callback models (187 assertions), and its exact lane, test title,
source location, requirements, and 2025 subsection mappings are recorded in
`ROADMAP-INDEX.json`.
The recent completion ledger also records the class-designator Request
Attribute Value Update TSO (97 assertions under both callback models), the
automatic-delete cutoff deletion (53), the two-survivor cutoff deletion (77),
the local-delete attribute-delivery fanout (86), the local-delete object
removal fanout (60), regional Auto Provide baseline (45), HLA_IMMEDIATE
ordering (42), receive-order response (96), timestamped response/retraction
(134), timestamped switch-mutation preservation (206), multi-provider fan-out
(196), switch-mutation fencing (224), switch-admission fencing (114),
timestamped switch-admission fencing (177), relaxed-DDM boundary (240), and
independent source-region fan-out (144), TSO retraction-designator uniqueness
(71), directed TSO multi-recipient restore (116), directed TSO single-recipient
restore (74), default-region multi-recipient TSO attribute restore (123), timed
explicit-source regional multi-recipient TSO attribute restore (147),
timestamped directed-interaction subscription-kind (117), and timestamped
directed target-departure (121), and time-regulated timestamped
directed-interaction MOM reporting (124), and the paired production-filesystem
report (236), and the non-time-regulating timestamped directed-interaction
filesystem report (163), receive-order Send Interaction filesystem report
(145), receive-order Send Interaction MOM report (85), timestamped Send
Interaction filesystem report (153), timestamped Send Interaction MOM report
(91), time-regulated timestamped Send Interaction MOM report (115),
time-regulated timestamped Update Attribute Values MOM report (119),
timestamped Update Attribute Values immediate-only retraction (69), mixed-fanout
retraction (75), suppressed timestamped interaction (31), suppressed
timestamped attribute callback (33), mixed regional timestamped attribute
delivery (131), mixed regional pre-callback retraction (88), and receive-order
Send Directed Interaction filesystem report (158).
Retrieve those exact
source lines and
mappings with
`python tools/query_rti_work.py recent --summary --compact` rather than
reopening the full plan.
The mixed interaction-declaration,
transportation-override, mixed-override, and directed-declaration companions
are green under HLA_EVOKED and HLA_IMMEDIATE (146, 186, 176, and 172
assertions respectively), as are the directed target-routing, ownership
handoff, directed TSO ownership-callback, and regional multi-source Auto
Provide companions (216, 264, 282, and 144 assertions), and the regular
ownership-release companion is green at 178 assertions. The public timestamped
object-deletion save/restore and delivered-retraction companions are green at
321 and 242 assertions, and the eligible directed-TSO delivery/retraction
companion is green under both callback models (220 assertions). The public
FQR/TARA/NMRA alternate-advance case is green under both HLA_EVOKED and
HLA_IMMEDIATE (462 assertions), the parameterized directed-TSO projection
case is green under both models (202 assertions), the directed-TSO
fan-out/retraction case is green under both models (302 assertions), and the
post-delivery-resignation case is green under both models (300 assertions).
The regional timestamped-interaction DDM companion is green under both
callback models (315 assertions), preserving the original source-region
snapshot, queued timestamped payload, recipient-specific delivery, and
immutable filesystem report-file identity. Retrieve the exact source line,
Requirements-Lab IDs, and canonical 2025 clause/subsection list with
`python tools/query_rti_work.py next --summary` or the indexed negotiated
If Available owner-confirmation lane. The regional timestamped-attribute-update DDM companion
is also green under both callback models (375 assertions), preserving its
source-region snapshot, recipient-specific delivery, valid retraction
metadata, and immutable filesystem report-file identity. Keep this
callback-model slice separate from regional/directed DDM, relaxed DDM,
transport, Java, and conformance work.

## Non-negotiable boundary

Umbra implements the official IEEE 1516.1-2025 C++ binding in
rti1516_2025. It does not introduce a competing public umbra::* RTI API.
All runtime types, state machines, transports, and test helpers remain private
implementation details until an officially declared type requires them.

The checked-in headers are an immutable baseline. Updating them requires a new
upstream archive hash, a digest-manifest refresh, licensing review, a
Requirements-Lab mapping review, and an explicit API-version decision.

The initial binding is a static library. Its public CMake `umbra::rti` target
carries the official STATIC_RTI definition to both Umbra and consumers, as
required by the IEEE header configuration. The separate static
`umbra::fedtime` target carries STATIC_FEDTIME and forwards the standard
libfedtime factory entry point to the reference time library. A shared-library
ABI is deferred until the required official exported classes can be implemented
and versioned together.

## Implementation sequence

### 1. Binding-complete fallback

Generate a reviewed inventory of every pure virtual member of RTIambassador and
FederateAmbassador. Add the official factory, rtiName, and rtiVersion
implementation, then provide a private concrete ambassador whose public methods
have the exact official signatures and exceptions. Do not advertise services
that return placeholder success values.

The inventory, static factory, and generated fallback ambassador are complete.
The fallback overrides all 181 RTIambassador members and throws RTIinternalError
unless a real private derived ambassador overrides the exact standard
declaration. This keeps the linkage checkpoint while allowing implementation to
advance one service slice at a time.

Gate: the built library links a consumer that calls the official factory, and
the generated inventory shows every required override is intentionally routed
to either an implemented service or the prescribed exception path.

### 2. Runtime kernel

Create private modules with one-way dependencies:

~~~text
standard binding adapter
        |
service coordinators
  /     |       |       \
state  callbacks FOM    transport
        |                 |
   executors/queues    local or remote backend
~~~

- State owns connections, federations, federates, handles, and lifecycle
  invariants.
- Callbacks own ordering, immediate/evoked models, re-entrancy rules, and
  failure containment.
- FOM owns module parsing, catalog identity, and schema validation; it is not
  mixed into transport messages.
- Transport owns framing, discovery, authentication hooks, and failure
  reporting. The first embedded backend is a test backend, not the final
  distributed architecture.

Gate: private state and callback behavior have deterministic unit tests with no
dependency on the public C++ binding.

The first components are the private top-level federate lifecycle, the
thread-safe embedded federation registry, one official FederateHandle
implementation, and the callback dispatcher. The registry receives only
prevalidated FOM descriptors; it does not parse XML or validate OMT itself.
An opt-in libxml2 component owns individual XML/XSD validation and an Annex
C-guided module compatibility preflight, also outside the registry.
The dispatcher implements the immediate/evoked queue semantics and is bound to
the four callback-control services. CallbackSession gives each caller-owned
FederateAmbassador an explicit shutdown fence before a dispatcher task invokes
it. Connect and Disconnect are bound to the public API under a mutex. Join and
Resign are also bound in the opt-in, embedded federation-management
development profile. That profile now has a private embedded transport
endpoint: a one-shot endpoint fault applies the member's Automatic Resign
Directive through forced registry cleanup, transitions the ambassador to Not
Connected, and queues the official Connection Lost callback. The endpoint is
an in-process seam for a later socket/IPC transport, not a distributed
transport implementation. A separate private in-session control seam models
the distinct Federate Resigned transition: its bounded NO_ACTION path removes
a clean member while retaining the connection, appends its final selected-file
reason record when selected, and queues the official callback. Connection Lost
uses its own final selected-file fault record during forced disconnect before
its best-effort callback; the two lifecycle paths remain distinct.

### 3. Federation-management vertical slice

Before broader object or time services, implement one end-to-end standard path:
connection, federation creation/destruction, join/resign, listing reports,
disconnect, and the connection-lost callback. The initial embedded path now covers all four
standard C++ Connect overloads, Disconnect, and the four callback-control
services. The embedded endpoint also drives the official Connection Lost
callback from a one-shot transport fault, applies Automatic Resign cleanup,
and removes the lost member before returning the ambassador to the
disconnected lifecycle. The embedded control seam separately drives Federate
Resigned without disconnecting, then permits a fresh Join on the same
connection. The aggregate Connect crosswalk is still ambiguous in
the Requirements Lab, so its tests cannot yet enter the compliance catalog;
Disconnect has a selected C++ surface and raw JUnit evidence, while the new
Connection Lost traceability remains development-profile-only. An opt-in
libxml2 backend now validates individual official 1516.2 documents against the
vendored schema and performs an Annex C-guided private composition preflight.
The official integer and finite-float reference time values, intervals,
factories, byte encodings, and static fedtime forwarding boundary are now
implemented and Catch2-tested. The private backend now materializes a
schema-validated FDD with Annex C `Composed_From` metadata, referenced-note
remapping, and service-usage OR semantics. Its reference selector now uses the
official HLAfloat64 default and rejects a documented mismatch with its two
reference factories; custom fedtime implementations remain intentionally
unavailable pending a provider contract. The same private preflight now
resolves direct `dataType` references only after the complete composed module
set is available, allowing later modules to provide referenced types and
recognizing the full OMT data-type key (including basic data representations).
Representation references are now resolved as a separate bounded rule:
simple/enumerated names must resolve, ordinary reference-data representations
must name a higher-level data type, and the two standard instance-identifier
names use their explicit `HLAunicodeString`/`HLAobjectInstanceHandle`
exception. The MIM/Restaurant `HLAboolean` interpretation remains recorded as
RL-009 rather than being rejected by a generic basic-only predicate. The
same preflight now rejects supplied non-positive update rates and validates
dimension default ranges against `[0, upperBound)`, while preserving
incomplete DIF rows for later composition; the 2025 FOM XSD already enforces
positive dimension upper bounds. Non-negative lookahead inference and other
table-specific rules remain open.
The same private preflight now applies the bounded 2025 Clause 3.3.1 name
convention. Libxml2 validates XML NCName syntax, while the composer rejects
periods in user-defined simple names, unknown case-insensitive `hla` prefixes,
and the case-insensitive `na` marker across named OMT declarations, note
labels, and qualified directed-interaction paths. Standard roots and
MIM-derived HLA names remain available after the merged pass. The supplied
Restaurant enumerator `NA` is accepted only as a recorded source-compatibility
exception because the official example conflicts with the prose reservation;
this private trace is not a conformance claim. The paired native Catch2 case
and Requirements-Lab contract are `fom-name-conventions` traceability only.
preflight rejects a reference-data type whose named object class is absent from
the complete hierarchy and, for ordinary attributes, resolves the named
attribute through ancestors and compares its type. Directed-interaction names
are resolved against the completed
interaction hierarchy as well, but the official FDD schema's one-entry
cardinality prevents materializing the supplied extension's two-entry merge.
Object and interaction available-dimension references resolve against the
completed dimension table, allowing a later module to provide the dimension;
this preflight does not implement runtime object/attribute DDM behavior.
The same catalog now retains each declared dimension's upper bound and each
object/interaction class's dimension associations. The development profile
allocates stable per-federation `DimensionHandle` values and implements the
official 2025 available-dimension, name, and upper-bound lookup services,
including inherited class associations. The development profile now also has a
metadata-only 2025 region-template/specification slice: official
`RegionHandle` and `RangeBounds` implementations, owner-scoped create,
pending range updates, complete commit, delete, dimension-set/range lookups,
and handle decoding. Commit is atomic across the selected regions and requires
one pending range for every declared dimension; pending values remain visible
until a successful commit, and bounds are checked against each FOM dimension's
  upper bound. Delete Region now rejects a still-used region—even when its
  regional subscription is passive—and only removes an unused owner-created
  template. The interaction regional declaration/send slice consumes those
  committed specs for independent subscriptions and 2025 overlap filtering.
  An accepted `Commit Region Modifications` invocation also has a focused
  production-filesystem MOM report path: its successful-void record uses the
  official Table 5 type-43 `RegionHandleSet` argument, while invalid handles
  append nothing. The companion lane is traceability evidence for §9.3 only;
  the adjacent Delete Region lane now covers the type-42 RegionHandle form;
  the Set Range Bounds lane covers its type-42/type-10/type-35 forms. A paired
  native HLA_IMMEDIATE public-MOM case now decodes the accepted Create Region
  type-11/type-42 and Get Range Bounds type-42/type-10/type-41 forms with file
  reporting disabled; generic non-void returns, broader DDM routing, and
  conformance remain open. A neighboring focused lane now
  records the accepted §9.6/§9.7 Associate/Unassociate Regions For Updates
  pair with type-37 ObjectInstanceHandle and MIM type-4
  AttributeSetRegionSetPairList arguments; invalid region input is suppressed
  and the callback/report forms remain separate. Neighboring focused coverage
  now also records the accepted §9.8/§9.9 regional Subscribe/Unsubscribe
  Object Class Attributes transitions: type-36 ObjectClassHandle and MIM
  type-4 AttributeSetRegionSetPairList arguments, plus the type-6 passive
  indicator and type-53/type-34 update-rate slot for regional subscription.
  Invalid region input is suppressed and callback/report forms remain
  separate.
  The bounded object-attribute regional slice now consumes the same committed
  specs for no-name regional registration, additive association/unassociation,
  active/passive regional attribute subscriptions, active-overlap-filtered
  discovery and no-time reflection, optional sent-region callback metadata, and
  reservation-consuming
  named regional registration. The per-federate Attribute Scope Advisory Switch
  now gates grouped in/out callbacks for known-object committed-overlap,
  update-region-association, and subscription transitions, with immediate/evoked
  delivery and stale-work suppression. Direct time-constrained timestamped
  default-region callback coverage is now present; broader DDM routing, save/restore,
  and package support
  remain separate work.
Attribute and interaction transportation names resolve against the completed
transportation table, allowing a later module to provide the transportation;
this does not implement data transport behavior.
Logical-time and interval representations are limited to the permitted 2025
data-type families (or `NA`); proving a non-negative lookahead remains later
semantic work. RL-144 records that the 2025 DIF/XSD and Requirements-Lab
export do not define an executable sign/domain predicate, so this boundary
does not infer one from a type name, underlying signed representation, or
free-form semantics text.
User-supplied and synchronization tag types are likewise restricted to their
permitted 2025 data-type families (or `NA`), including reference data but not
basic data. The composed catalog now also retains each synchronization-table
row's label, tag type, capability, semantics, and remapped note references for
the registry's future standards-facing synchronization coordination; live
registration state remains separate from this immutable FOM projection.
The Annex C.5 composition path treats synchronization points as a
same-label/first-definition table: an identical duplicate is ignored, while a
duplicate with any differing label, tag type, capability, semantics, or note
sub-element fails composition. This prevents the generic note-reference union
from silently manufacturing a new synchronization definition. The paired
`compliance/requirements-lab/fom-synchronization-merge-requirements-contract.json` and Catch2
case are private preflight traceability, not public synchronization-service or
conformance evidence.
The Annex C.6 composition path applies the same first-definition rule to
transportation types: an identical same-name duplicate is ignored, while a
duplicate with any differing name, semantics, or note sub-element fails before
FDD materialization. The paired
`compliance/requirements-lab/fom-transportation-merge-requirements-contract.json` and Catch2
case are private preflight traceability only; they do not implement transport
delivery.
The Annex C.7 composition path applies the same first-definition rule to update
rates: an identical same-name duplicate is ignored, while a duplicate with any
differing name, rate, semantics, or note sub-element fails. Omitted DIF rate or
semantics fields remain eligible for later completion. The paired
`compliance/requirements-lab/fom-update-rate-merge-requirements-contract.json` and Catch2 case
are private preflight traceability only; the catalog value does not implement
update-rate scheduling.
The completed hierarchies also reject object attributes and interaction
parameters that overload a name declared by an ancestor. This is private
composition preflight only, not object or interaction runtime support.
The opt-in
`UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT` profile now uses that pipeline
for the official Create/Destroy/Join/Resign methods: MIM-first preparation
precedes creation, an additional-FOM join commits a replacement definition and
membership atomically, and two independent two-federate Catch2 scenarios
exercise the public methods, exception, and rollback boundaries. It deliberately
remains a source-tree,
embedded profile with two event-producing service paths: List
Federation Executions and List Federation Execution Members snapshot shared
state and report through the standard FederateAmbassador methods in immediate
and evoked modes. CallbackSession closes queued/in-flight delivery safely at
Disconnect. It also gives each joined federate an initial selected LogicalTime,
holds Time Advance Request in a per-federate time-advancing state until Time
Advance Grant is dispatched, and supports Query Logical Time. It also
callback-gates regulation/constrained enable requests, stores the enabled
lookahead through official interval objects, supports Query Lookahead, and
clears each role through the matching Disable service. Its federation-owned
time snapshot also provides read-only Query GALT/Query LITS from other
regulators' current or pending time plus lookahead, with factory epsilon for a
forward zero-lookahead TAR boundary. The embedded scheduler has no broader
public TSO traffic or full federation-wide coordination; the separate public
surface contains five bounded timestamped interaction, attribute-update,
object-deletion/removal, directed-interaction, and region-context interaction
slices.
A private recipient-scoped queue and temporal coordinator now feed queued, in-transit, and
delivered-since-last-advance timestamps into the GALT/LITS snapshot and
limited scheduler. The scheduler now registers and
re-evaluates limited cross-federate TAR grants under strict defined-GALT and
undefined-GALT/NRG policy after relevant role and membership changes, while
retaining static NRG across additional-FOM definition changes, then rechecks
the bound at callback delivery. The profile still has no federation-event
callbacks beyond Connection Lost and the bounded Federate Resigned seam,
complete object/ownership state, broader time-management services, or remote
transport. The embedded endpoint's deterministic fault hook is test-only; it
does not claim socket/IPC delivery. It also implements the
official per-federate asynchronous-delivery switch: receive-order interaction,
reflection, directed-interaction, and object-removal callbacks are deferred for
idle time-constrained federates while disabled, released on enable or Time
Advancing, and gated again on disable. The bounded integration case exercises
both `HLA_EVOKED` and direct `HLA_IMMEDIATE` release behavior. A regional
receive-order companion applies the same gate to an overlap-qualified
`Send Interaction With Regions` callback and preserves its source-region set
across disable/re-enable. This callback queue is live-session state; timestamped
messages, MOM reporting, durable save/restore, and remote transport remain
outside the slice. `getTimeFactory` is exposed
there only to return a factory for the immutable selected federation time
representation. The same profile exposes `getFederateHandle` and
`getFederateName` for the caller's joined federation: the former resolves only
active joined names, while the latter retains the immutable name of a valid
returned handle after resignation. It keeps invalid handles distinct from valid
handles not known to that federation. It also exposes `getObjectClassHandle`, `getObjectClassName`,
`getInteractionClassHandle`, and `getInteractionClassName` through the current
composed FOM catalog. It also exposes `getAttributeHandle` and
`getAttributeName`, resolving an attribute through the class that declares it
so the same handle is returned from a subclass. It likewise resolves parameters
through the interaction class that declares them, so a subclass returns the
same parameter handle. Its immutable per-federation handle directories preserve
issued object-, interaction-class, defining-attribute, and defining-parameter
handles when a compatible additional-FOM join extends that catalog. The default
packaged profile keeps
those methods on the fallback. The development profile also retains independent
per-federate publication and subscription declarations for one interaction
class through the four official declaration services. It now also implements
the non-timestamped, non-region Send Interaction overload and matching no-time
Receive Interaction callback: recipient selection walks the composed FOM
hierarchy to the closest subscription, projects available parameters, excludes
the sender, rechecks queued delivery, and preserves tag/producer/mandatory
FOM-selected transportation data through the recipient's callback model. It
also retains per-federate class-directed publication and subscription state
for the official object-class directed-interaction declaration services and
implements the bounded non-timestamped, non-DDM `Send Directed Interaction`
overload with the no-time `Receive Directed Interaction` callback. The planner
requires the target object to be known, chooses at most one applicable received
class, excludes the sender, and projects callback delivery through the same
immediate/evoked dispatcher. Callback entry rechecks target lifecycle and both
declaration sides. A missing or false `universally` selector is by ownership
and requires the known target to have at least one attribute owned by the
recipient; true is universal and permits every known target. Each supplied
class takes the invocation selector, while an empty class set preserves any
existing selectors. Additional timestamped/retraction behavior beyond the
separate bounded slice, directed DDM, ordering, sharing-policy enforcement,
and remote transport are not implemented. The native directed receive-order
planner now also retains a route-only recipient when the federation's Delay
Subscription Evaluation switch is enabled, allowing a selector added before
the callback boundary to receive the accepted send while preserving the live
unsubscribe, target, and publication rechecks. It also covers that
delayed-selector boundary for timestamped directed interactions: the route-only
recipient remains in the federation-owned TSO queue until the recipient grant,
where the current selector is re-evaluated. The profile also implements the
bounded regional interaction declaration and no-time send
overloads: regional subscriptions are independent, only active overlapping
pairs gate delivery, passive pairs remain declared and retain their region-use
fence, empty sent sets suppress it, and queued callbacks recheck the active
overlap. Its focused report lane now appends the accepted §9.10/§9.11
recipient-local successful-void records with the official type-27 interaction
handle, type-43 region set, and type-6 passive-indicator argument before the
next interaction operation. The separate object-attribute regional boundary is
implemented only for no-name registration, association/unassociation, active/
passive regional subscriptions, active-overlap-filtered no-time reflection,
reservation-consuming
named regional registration, the bounded timestamped regional
Update/Reflect Attribute Values path, and the bounded Attribute Scope Advisory
path for known-object overlap, association, and subscription transitions;
   the regional class-request path now also resolves RTI-owned `HLAfederate`
   MOM objects against their immutable private points and rechecks that
   predicate at an evoked callback boundary. Additional regional request edge
   cases, timestamped/retraction behavior beyond that bounded path, broader
   DDM routing, directed DDM, FOM sharing-policy enforcement, custom
   transportation, and full object delivery remain open. The official
Restaurant extension remains correctly rejected because its directed-interaction
merge cannot be encoded by the vendored FDD schema.

The same development profile now retains explicit publication and active/passive
subscription state for available attributes of object classes through the
four non-region 2025 object-class attribute declaration services. The state is
per-federate and per-class, validates inherited handles against the composed
FOM, and clears at resign. Only active declarations feed the bounded discovery,
scope, and reflection paths; passive declarations remain retained without
arranging delivery. The current object-registration slice consumes it
to determine whether a class is published and to snapshot the currently
published attributes, including the limited implicit privilege-to-delete rule.

The same profile implements the unnamed `Register Object Instance`
overload, generated private object-instance names, non-region Discover Object
Instance callbacks, and `getKnownObjectClassHandle`,
`getObjectInstanceHandle`, and `getObjectInstanceName`. The private registry
captures a registered class, producer, owned published-attribute snapshot,
recipient-local known class, and pending discovery reservation. Discovery uses
the closest active subscribed class, rechecks a queued recipient's eligibility before
callback delivery, and honors immediate versus evoked callbacks. It also
retains that recipient's selected service-report route, appending the §6.9
type-37/type-36/type-53/type-15 private record at callback entry only when
the discovery survives the recheck. Receive-order §6.17 removal now likewise
appends its recipient-local seven-slot private record at callback entry. The
timestamped/retraction deletion slice and its separate callback path are
described below. The registry records the current owner of
each registration-established attribute, requires the deleting federate to own
`HLAprivilegeToDeleteObject`, makes that federate unknown immediately, and
keeps another recipient known only until its removal callback starts. Named
registration, regional DDM scope, ownership transfer, FOM sharing policy,
save/restore, and full resign-action object disposition remain separate work.

The adjacent 2025 object-instance name reservation slice now owns single and
multiple reservation/release state. It applies the official empty and `HLA.`
name guards, commits successful reservations before queuing the four standard
result callbacks, reports a successful/failed subset for mixed multiple
requests, validates multiple release atomically, avoids collisions with
generated unnamed-registration names, and clears reservations during resign.
The named registration slice consumes only a successful reservation owned by
the registering federate, preserves it across publication failure, commits the
object/name indexes before erasing it, and covers both non-region and regional
overloads. These slices are traceability and development-profile test evidence
only. The single-name Reserve/Release service-report lane now appends the
source-backed type-53 `Name` successful-void record after accepted transitions,
with switch gating and rejected-call suppression; multiple-name report forms
now append the source-backed type-54 `StringSet`/`Array<String>` record for
§6.5/§6.7, with the official `Name Set`/`Name set` labels and the same gating
and atomic-rejection behavior. The multiple-name result callback report forms
remain separate because their composite success-indicator shape is not yet
mapped for the Table 5 file log.

The four 2025 `Register Object Instance` overloads now use the embedded
filesystem service-report seam. Accepted ordinary and regional calls record
their Table 5 object-class, optional name, attribute/region pair-list, and
object-instance return forms after registry commit and before queued discovery;
an invalid object class records the Null/false failure form. The focused
Catch2 case is mapped to the official C++ API surfaces and MOM/object-
management/DDM requirement candidates. This remains development-profile
traceability only; timestamped/retraction, broader DDM/ownership, public MOM
interaction delivery, transport, package/JUnit, protected review, validation,
and conformance remain open.

The registry now also implements the bounded 2025 `Local Delete Object
Instance` service. It removes only the invoking federate's known-instance
state, rejects a federate that still owns instance attributes or has a pending
ownership acquisition, leaves the federation-wide object and other federates'
knowledge untouched, and permits a later eligible subscription to rediscover
the object. Timestamped/local-delete interaction behavior, DDM, save/restore,
remaining resign-action object disposition, and remote transport remain separate work.

The same registry now implements the no-time `Update Attribute Values` overload
and matching `Reflect Attribute Values` callback. It accepts
only source-owned FOM-defined attributes of a known live instance, retains one
passel per FOM transportation type, projects each passel at the receiver's
known class and current active subscription, excludes the source, and rechecks a
queued callback before delivery. The test fixture proves multi-transport
passels, active-superclass projection, passive-subscription suppression,
unsubscribe suppression, tag/producer
propagation, and both callback models. Its separate regional object-attribute
  scenario proves active committed-region overlap discovery/reflection,
  passive-region suppression, and optional sent-region callback metadata. The
  separate Attribute Scope Advisory slice at
  `cpp/tests/attribute_scope_advisory_catch2.cpp:97` covers switch-gated in/out
  callbacks for known-object overlap, update-region-association, and
  subscription transitions. It is green with 187 assertions, nine
  Requirements-Lab anchors, five canonical 2025 sections, and eleven official
  C++ API surfaces under both HLA_EVOKED and HLA_IMMEDIATE. Timestamped/retraction
updates now have a separate bounded non-regional path: time-regulating sends
use the federation-owned TSO queue, preserve recipient-specific transportation
passels, and deliver timestamped `Reflect Attribute Values` callbacks before
the matching grant. A focused ordinary non-regional lifecycle companion now
keeps one queued attribute passel across callback-gated Time Constrained
disable/re-enable and proves exactly one reflection before the matching grant,
including payload, tag, producer, timestamp/order, and retraction metadata.
Its Catch2/Requirements Lab and API records are
`compliance/requirements-lab/timestamped-attribute-update-requirements-contract.json` and
`compliance/requirements-lab/timestamped-attribute-update-api-contract.json`.
Timestamped deletion now has a separate bounded non-regional path: the
registry retains a pending object-removal payload, queues constrained
recipients, supports retraction-before-delivery reconstitution, and delivers
the timestamped `Remove Object Instance` callback before the grant. Its
recipient-local filesystem route now appends the corresponding private §6.17
record before each callback: immediate recipients use `TIMESTAMP`/`RECEIVE`,
while TSO recipients use `TIMESTAMP`/`TIMESTAMP`, with the timestamp and
supplied retraction designator preserved. The sender-side §6.16 invocation
also has a selected filesystem report: non-time-regulated sends retain the
type-34 Null return, while a time-regulated send records the type-33
`MessageRetractionHandle` before the queued removal callback. Its
Catch2/Requirements Lab records are
`compliance/requirements-lab/timestamped-object-deletion-requirements-contract.json` and
`compliance/requirements-lab/timestamped-object-deletion-api-contract.json`. A second Catch2
scenario proves a legal post-delivery Retract reconstitutes the object/name/
known state and committed split ownership before Request Retraction reaches a
delivered nonconstrained recipient, while a constrained recipient's pending
removal is suppressed. A third scenario proves a departed delivered owner is
not reconstituted or notified, and its former attribute remains unowned. A
terminal no-recipient case retains `MessageCanNoLongerBeRetracted` while
releasing the deletion snapshot, marker, and object name for a fresh named
registration. A focused normal-interaction regression now covers one Disable
Time Regulation/re-enable lifetime path at unchanged lookahead; its companion
keeps one accepted queued timestamped interaction through that producer-role
transition and delivers it exactly once before the recipient grant. A focused
ordinary timestamped-attribute companion covers the corresponding
Time-Constrained disable/re-enable queue lifetime. Complete alternate advances,
broader re-enable, in-flight ownership, other resignation
cases, recovery, transport, and conformance remain
separate work. Remaining regional updates, timestamped default-region coverage,
update-rate reduction, ownership transfer, FOM sharing-policy enforcement,
custom transportation implementation, save/restore, and remote transport
remain separate work.

The registry also implements the object-instance `Request Attribute Value
Update` overload and matching `Provide Attribute Value Update` callback. It
validates the request at the requester's currently known class, groups currently
owned requested attributes by provider, suppresses callbacks for requester-owned
and unowned attributes, preserves the tag, and rechecks a queued provider before
invocation. The provider's queued work also retains its selected service-report
route; after that callback-time recheck it appends the private §6.22
type-37/type-1/type-63 record before provider code enters the callback.
Four focused filesystem regressions now prove that ordering for the explicit
object-instance request path, Auto Provide (including its empty tag), class
expansion, and one overlap-qualified regional request. The accepted regional
request now also appends its private §9.13 type-36/type-4/type-63 record in the
requester's file before the queued provider callback; a dedicated CTest lane
keeps that requester boundary independently runnable from the provider lane.
The queue primitive is shared, but those tests intentionally do not claim
timestamped, multi-owner or regional-matrix, generic return/failure, or public MOM delivery coverage. A separate
bounded response scenario now exercises provider code
invoking the official non-timestamped `Update Attribute Values` service from
inside that callback and the requester receiving `Reflect Attribute Values` with
the response tag, producer, and mandatory transportation type. This is not RTI-
automatic provision: the provider supplies the response explicitly. A second
bounded object-instance response companion invokes the official timestamped
`Update Attribute Values` service from that callback and verifies one requester
reflection before the matching grant, including time/order and retraction
metadata. Regional requests and timestamped/retraction behavior beyond these
two bounded response companions, DDM, update-rate reduction, ownership transfer,
FOM sharing policy, save/restore, and remote transport remain outside this slice.

The enabled federation-wide Auto Provide baseline is independently queryable at
`cpp/tests/auto_provide_baseline_catch2.cpp:84`. It is green with 28
HLA_EVOKED assertions, three Requirements-Lab anchors, two canonical 2025
sections, and 15 official C++ API surfaces including Get Auto Provide Switch
and Provide Attribute Value Update. The enabled regional FDD value is observed by both members;
one discovery produces one grouped in-scope owner solicitation covering both
attributes with the required empty RTI-invoked tag. Disabled discovery and MOM
switch mutation remain separate lanes, as do regional/relaxed-DDM,
multi-provider, timestamped, save/restore, package/JUnit/protected-review,
validation, and conformance evidence.

The bounded Auto Provide path now retains the federation-wide dynamic switch
from the creation FDD and exposes `getAutoProvideSwitch`. After a newly
completed discovery, the registry groups the discovered recipient's in-scope
owned attributes by current provider and schedules the standard
`Provide Attribute Value Update` callback with an empty RTI-invoked tag. The
provider's selected service-report route now follows that queued work and
appends its private §6.22 type-37/type-1/type-63 successful-void record before
the callback; the focused filesystem regression verifies the zero-length tag
and ordering at callback entry. This does not add a report for discovery or
for changing the switch itself. The standard federation-wide `HLAsetSwitches` MOM interaction can now change that
value during execution using the vendored `HLAswitch` four-byte encoding, and
the change is visible to every current member. The existing callback-time
provider recheck remains the lifecycle fence for stale work. The separate
joined-federate `HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches` path now
accepts a non-empty subset of its nine predefined parameters, updates only the
sending member, and accepts compatible FOM extension parameters and subclasses
without processing their extension values. It strictly decodes `HLAswitch` and
`HLAresignAction`; a report-service subscription conflict preserves the whole
pending update but is presently surfaced through the local error path rather
than a normal MOM failure interaction. Other MOM control/reporting families
and complete regional, multi-owner, update-rate, and automatic-value semantics
remain separate work.

The public regional Auto Provide matrix now has separate multi-source-region
and Allow Relaxed DDM cases. One provider associates two attributes with
independent source regions, two recipient federates subscribe to their
respective regions, and each overlap-qualified source produces exactly one
grouped provider callback and one receive-order reflection under both
`HLA_EVOKED` and `HLA_IMMEDIATE`. The Relaxed DDM companion adds the exact
touching boundary: disabled strict filtering blocks discovery, the enabled
policy admits one scoped provider response, a positive gap suppresses delivery,
and restoring strict overlap permits one ordinary update without duplicate
discovery or solicitation. Both tests keep source RegionHandleSet, value, tag,
producer, and transport visible and use a fixed bounded callback drain because
a final provider callback can enqueue a recipient callback after its own
dispatcher becomes empty. These policy cases remain separate from switch
mutation, timestamped/retraction, save/restore, and transport.

The five official 10.29--10.33 normalization services now form the first DDM
coordinate bridge required by MOM. The adapter validates the standard
connection/member/input boundaries. The registry owns a stable opaque mapping
for valid federate, object-class, interaction-class, and live object-instance
handles, restores its per-execution seed with a saved federation, and preserves
equality for equal designators without promising a sequential or unique result.
`ServiceGroup` is deliberately returned as the standard in-range
`HLAserviceGroup` coordinate rather than arbitrary per-execution data. The
filesystem lane and native HLA_IMMEDIATE companion now encode and route the
five successful service reports with their official Table 5 forms while file
reporting is disabled. RTI-owned MOM point-region realization, distributed
execution, package/JUnit/protected-review evidence, and conformance remain
separate work rather than being hidden behind ordinary federate-originated
interaction delivery.

The remaining 2025 support-switch metadata and accessors are now scaffolded in
the same standards-first path. The FDD composer retains per-federate Convey
Region Designator Sets, Automatic Resign Action, Service Reporting, Exception
Reporting, and Send Service Reports To File defaults, while the registry
captures federation-wide Delay Subscription Evaluation and Allow Relaxed DDM
values at creation. Official getters/setters preserve per-federate isolation
and enum validation. The recipient projection now also applies the Convey
Region Designator Sets switch at callback entry for the bounded regional
reflection and interaction paths, omitting or supplying the optional sent
regions accordingly. The bounded §8.1.8 Delay Subscription Evaluation path now
also retains joined non-source recipients for ordinary and explicit-regional
`Send Interaction` plus ordinary and explicit-regional `Update Attribute Values`
passels when the creation-time switch is enabled and planning finds no current
active subscription. It reprojects the recipient at its ordinary callback
boundary or time-constrained TSO grant. Both boundaries are now exercised under
`HLA_EVOKED` and `HLA_IMMEDIATE` for the ordinary families: the receive-order
cases control dispatch explicitly, while the timestamped cases receive their
eligible delivery with the direct Time Advance Grant. The focused regional
attribute and interaction companions exercise the explicit-source TSO grant
boundary and preserve the source-region metadata when delivery remains eligible.
The attribute regressions establish a known object before changing its
declaration. The Disabled default retains generation-time ineligibility, while
both modes suppress a recipient that later unsubscribes. Focused explicit-
source regional attribute and interaction cases now prove the same
enabled/disabled split for update-region and interaction passels and preserve
sent-region metadata at the eligible grant. The dedicated contract records the
Lab's stale source-title/clause metadata (RL-018). The bounded
embedded transport-fault path now executes Automatic Resign cleanup and emits
`HLAreportFederateLost` to ordinary or DDM-matching regional subscribers; its
ordinary and DDM-regional subscriber routes are also exercised directly under
`HLA_IMMEDIATE`.
Generic §11.5 MOM interaction emission and generic file output, default/conveyed-region reuse, directed
regional callbacks, or the remaining delayed-subscription matrix beyond the
bounded regional attribute and interaction cases. The exact MOM report-service subscription/switch
interlock is implemented separately for ordinary and regional declarations and
the joined-federate `HLAsetSwitches` update path. The dedicated
`cpp/tests/mom_service_reporting_interlock_catch2.cpp:50` lane records 41
`HLA_EVOKED` assertions across §§5.10.2, 9.10.3, and 11.5, including active
and passive admission rejection plus removal-before-enable recovery. Seven source-backed
successful-void support-service file appends are now exercised against the
selected filesystem sink: six Boolean setters (the four relevance/scope
advisory setters, `Set Convey Region Designator Sets Switch`, and `Set
Exception Reporting Switch`) plus `Set Automatic Resign Directive`; the full
generic report-generation path remains open. Five no-argument Time Management
services—`Disable Time Regulation`, `Enable/Disable Asynchronous Delivery`,
and `Enable/Disable Time Constrained`—also append the explicit
successful-void form with empty supplied-argument lists. `Enable Time
Regulation` appends one `Lookahead` using Table 5's `LogicalTimeInterval`
type 32 and quoted `interval.toString()` form. `Modify Lookahead` appends one
`Requested lookahead` using the same form when the request is accepted, even
when a lower actual value remains deferred; `Modify Lookahead`, `Disable Time
Regulation`, and `Disable Time Constrained` now also emit public MOM reports
after acceptance and outside native service locks. `Enable Time Regulation`
and `Enable Time Constrained` emit public MOM reports after acceptance and
before their typed callbacks, while both enable records preserve their
separate callback-gated completion. `Time
Advance Request`, `Time Advance
Request Available`, `Next Message Request`, and `Next Message Request
Available` each append one `Logical time` using Table 5's `LogicalTime` type
31 and quoted `time.toString()` form on acceptance before their distinct Time
Advance Grant callbacks complete the advance. The two Next Message Request
reports preserve their supplied boundaries when a queued TSO message produces
an earlier grant target. The
Register Federation Synchronization Point service appends its accepted §4.14
successful-void record, then the unified §4.15 confirmation record at the
registering federate before either C++ registration-result callback. The
type-53 `Synchronization point label` and Table 5 type-63 base-64
`User-supplied tag` are followed by the required third optional-set slot: the
two-argument C++ overload retains it as type-34 Null, while the explicit-set
overload uses type-18 `FederateHandleSet` with quoted
`FederateHandle::toString()` values, including an explicitly supplied empty
`[]`. The confirmation carries type-53 label, type-6 `Registration-success
indicator`, and type-34 Null or type-56
`SynchronizationPointFailureReason` optional failure reason. §4.16 recipient
announcements use a weak private joined-federate report endpoint to append the
type-53/type-63 record before their callback is queued. Attaching that endpoint
at Join preserves the same recipient file and serial sequence across reporting
switch disable/re-enable cycles. The type-63 form remains bounded private file
text under RL-077. On synchronization completion, the same endpoint appends
each §4.18 recipient's type-53 label and type-18 failed-federate set before its
Federation Synchronized callback is queued; the focused initial lane covers
both normal all-achieved completion and completion after an unachieved member
resigns.
Synchronization Point Achieved service appends its accepted §4.17 Table 5
successful-void record before it queues the corresponding Federation
Synchronized callbacks. Its supplied arguments are the type-53
`Synchronization point label` and the type-6 effective optional
`synchronization-success indicator`; the defaulted C++ form records `true` and
the explicit failed form records `false`. The
Request Federation Save service now appends its accepted §4.19 Table 5
successful-void record before any separately queued Initiate Federate Save
work. Both official C++ overloads write the type-53 `Federation save label`;
the no-timestamp form retains type-34 Null for `Optional timestamp`, and the
timestamped form writes type-31 `LogicalTime` from the RTI-owned clone's
quoted `toString()` value. The focused filesystem lane establishes a joined
federate that is both time-constrained and time-regulating, keeps the untimed
request pending, then proves a valid timestamped request replaces it without
evoking either save callback. Invalid-time handling and full time-bound save
admission remain covered separately. The
ordinary queued §4.20 recipient path now carries the same private
per-joined-federate report route: every selected recipient appends an
`InitiateFederateSave` type-53 label plus type-34 Null or type-31 LogicalTime
timestamp record before its HLA_EVOKED callback can run. The focused filesystem
case covers two non-time-constrained recipients with independent report files;
the direct time-constrained pre-grant route remains explicitly separate. The
Requirements Lab's page-67 §4.20 candidates are exported as §4.21.1, recorded
as RL-082 rather than silently accepted as the implementation association. The
Request Federation Restore service appends its normally returned §4.27 Table 5
successful-void record before the distinct RTI-initiated §4.28 Confirm
Federation Restoration Request record. That requester-local record preserves
the type-53 `Federation save label` and adds a type-6 `Request-success
indicator` Boolean, true for a matching snapshot and false when it is missing,
before either HLA_EVOKED callback can run. The focused filesystem lanes prove
both result forms; the negative form does not start a restore operation and is
not a generic failed-service file record. The RTI-initiated §4.29 Federation
Restore Begun callback now appends its Table 5 successful-void no-argument
record to every joined recipient's selected file, including the requester,
before that recipient's HLA_EVOKED callback. The focused two-recipient
filesystem lane proves the independent requester and peer serial sequences
around a real shared snapshot. Its source candidates are actual §4.29 text but
retain the Lab checker-required coalesced-page §4.29.3 owner; RL-078/RL-002
document that provenance defect. The following §4.30 Initiate Federate Restore
callback now appends one recipient-local successful-void Table 5 record before
each HLA_EVOKED callback, retaining the type-53 Federation save label, type-15
joined federate designator, and type-53 federate name in that recipient's
existing report file and serial sequence. The focused two-recipient filesystem
lane proves both independent streams. Its actual source candidates retain the
Lab's checker-required coalesced-page §4.30.6 owner and stale display
provenance, documented under RL-078/RL-002. The
Federate Restore Complete service now appends the one §4.31
`FederateRestoreComplete` Table 5 successful-void record after its accepted
state transition. `federateRestoreComplete()` selects the required type-6
`Federate restore-success indicator` true, while
`federateRestoreNotComplete()` selects false; both are durable before their
respective Federation Restored or Federation Not Restored callback. The report
serial is live per-joined-federate audit state rather than restored application
state, so it continues monotonically across restore. The focused filesystem
lane proves both forms and a rejected pre-restore call that appends nothing.
The source candidate is retained with the Lab's checker-required §4.32 owner,
while RL-078/RL-002 document its actual §4.31 source placement and stale title.
The Abort Federation Restore service now appends its accepted §4.33 Table 5
successful-void record with empty supplied arguments before the ordinary
`RESTORE_ABORTED` Federation Not Restored callback. A rejected pre-restore
abort appends nothing. The all-members-already-complete special case, where the
later restore result may still be successful, remains outside this bounded
lane. Its source candidates retain their checker-required §4.34 owner while
RL-078/RL-002 document their actual §4.33 source placement and stale title.
The Query Federation Restore Status service now appends its accepted §4.34
Table 5 successful-void record with empty supplied arguments before its
separately queued Federation Restore Status Response callback. The §4.35
recipient route now appends the descriptor vector to that same selected file
before callback delivery, using official-MIM type 20
`FederateRestoreStatusSet`, Table 5's valid singular record fields, public
pre/post handle text, and quoted `RestoreStatus` spelling. The focused lane
proves the idle `NO_RESTORE_IN_PROGRESS` response and confirms a
`SaveInProgress` rejection appends no query record. Its direct candidates have
the correct §4.34 structural owner; only their generic stale title provenance
remains tracked by RL-002. The Table 5 collection-name/example discrepancy is
recorded under RL-083.
The Resign Federation Execution service now appends its accepted §4.12 action
as the final selected-file report before the joined-federate writer is released.
Its lifecycle differs from ordinary post-service reports: the registry reserves
the serial inside the successful resignation transaction, after all standard
rejection paths and immediately before the member is erased. The adapter writes
the resulting type-44 `HLAresignAction` record with the standard MIM's spelling
for its implementation-defined descriptive name. The focused production-file
lane proves an invalid action is silent, a successful `NO_ACTION` invocation
writes the next serial after an earlier selected-file record, and a repeated
resignation cannot append. The failure seam
also proves an append error cannot leave the ambassador half-joined or cause a
memory fallback.
The RTI-initiated Federate Resigned path now applies the corresponding final
record discipline to §4.13: clean embedded member removal reserves the next
file serial before erasure, the adapter appends the Table 5 type-53 `Reason for
resigning` record, and only then queues the official callback. The focused
production-file regression proves that durable-before-callback ordering,
duplicate suppression, and retained-connection rejoin behavior. It explicitly
does not conflate this path with Connection Lost. The separate §4.4 fault path
reserves its final file serial during forced cleanup, appends the type-53
`Fault description` record, and then queues its best-effort callback before
releasing the writer. Its production-file regression proves durable-before-
callback ordering, duplicate-fault suppression, and the fresh-Connect cleanup
boundary without claiming remote fault delivery.
The companion native C++ HLA_IMMEDIATE observer regression now covers the same
§4.13 route when file reporting is disabled: it reserves the public
federation-management type-0 interaction before member removal, delivers the
type-53 reason and Null return to an eligible observer, and crosses that
delivery before the official evoked callback. This is an additive
development-profile test-plan slice; public administration/session-timeout
inputs, remote transport, package/JUnit/protected-review evidence, Requirements
Lab validation, and conformance remain open.
The
Federate Save Begun service appends its accepted §4.21 Table 5 successful-void
record after its save-control state transition. Because §4.21 has no supplied
or returned arguments, its record has an empty supplied-argument list and the
explicit `[null]` returned-argument form. A rejected pre-initiation call
appends nothing; later save completion and Federation Saved remain separate
service/callback boundaries. Federate Save Complete's §4.22 report has one
type-6 `Federate save-success indicator`: the C++ `federateSaveComplete()`
selector records `true`, while `federateSaveNotComplete()` records `false`.
Both forms use the one `FederateSaveComplete` service name after their accepted
state transitions and before their respective Federation Saved or Federation
Not Saved callbacks. A rejected pre-begun completion appends nothing. The
separate RTI-initiated §4.23 `FederationSaved` report now reaches each selected
recipient before its result callback: normal completion uses a type-6 true
`Federation save-success indicator` plus type-34 Null `Optional failure
reason`, and failure uses false plus type-48 quoted `SaveFailureReason`. The
focused production-filesystem lane proves normal two-federate completion and
the ordinary `SAVE_ABORTED` failure form. §4.23's source candidates retain the
coalesced-page checker owner `clause-4.23.2` tracked under RL-078; that does
not change the implementation's direct §4.23 service association. The
Abort Federation Save service appends its accepted §4.24 Table 5
successful-void record with empty supplied arguments before the ordinary
`SAVE_ABORTED` Federation Not Saved callback. The test deliberately excludes
the exceptional case in which all members have already completed successfully
and the abort may still yield Federation Saved with a successful selector. A
rejected no-save abort appends nothing. The
Query Federation Save Status service appends its accepted §4.25 Table 5
successful-void record with empty supplied arguments before the separately
queued Federation Save Status Response callback. The distinct RTI-initiated
§4.26 response now appends its selected-file type-17
`FederateHandleSaveStatusPairSet` first: the Table 5 array carries an explicit
quoted `handle` and quoted `SaveStatus` per member. The focused regression
starts one ordinary save to verify the `FEDERATE_INSTRUCTED_TO_SAVE` response
record and callback ordering; it does not claim the full
no-save/multi-member/restore status matrix. The direct §4.26 candidates retain
the RL-078 coalesced-page checker owner `clause-4.27.2` while the implementation
and tests identify the actual §4.26 callback.
The
nonregional receive-order `Update Attribute Values` overload now appends its
accepted §6.10 successful-void record before the separately queued `Reflect
Attribute Values` callback. Its source-backed Table 5 arguments are type-37
`Object instance designator`, type-2 `AttributeHandleValueMap` rendered as a
`PairList<AttributeHandle:BinaryData>`, the bounded type-63 base-64
`User-supplied tag`, and a type-34 Null placeholder for the absent optional
timestamp. The ordinary timestamped form is covered by the native filesystem
and public-MOM companions below; regional and `HLAsetSwitches`-specific forms
remain excluded until their return, region, or MOM-control records have
equivalent source-backed file representations. The ordinary timestamped Update
Attribute Values sender now has a native HLA_IMMEDIATE companion: after TSO
admission it decodes type-37/type-2/type-63/type-31 supplied forms and the
quoted type-33 MessageRetractionHandle before the constrained reflection
callback, which preserves valid retraction and callback metadata. The
nonregional receive-order `Send Interaction`
overload now appends its accepted §6.12 successful-void record before the
separately queued `Receive Interaction` callback. Its source-backed Table 5
arguments are type-27 `Interaction class designator`, type-40
`ParameterHandleValueMap` rendered as a
`PairList<ParameterHandle:BinaryData>`, the bounded type-63 base-64
`User-supplied tag`, and a type-34 Null placeholder for the absent optional
timestamp. The native HLA_IMMEDIATE public-MOM companion now decodes the same
accepted ordinary report before the evoked callback, with the type-34 Null
successful-void return, empty exception, unresolved RTI producer, no regions,
and serial zero. The ordinary timestamped form is covered by the native
public-MOM companion below; regional and `HLAsetSwitches`-specific forms remain
excluded until their return, region, or MOM-control records have equivalent
source-backed file representations. The receive-order `Send Directed
Interaction` overload now appends its accepted §6.14 successful-void record
before the separately queued `Receive Directed Interaction` callback. Its
source-backed Table 5 arguments are type-27 `Interaction class designator`,
type-37 `Object instance designator`, type-40 `ParameterHandleValueMap`
rendered as a `PairList<ParameterHandle:BinaryData>`, the bounded type-63
base-64 `User-supplied tag`, and a type-34 Null placeholder for the absent
optional timestamp. The native HLA_IMMEDIATE companion now decodes both the
untimestamped type-34 and timestamped type-31 optional forms, along with the
type-34 Null successful-void return, before the directed callbacks. The
time-regulated timestamped sender now also has a native HLA_IMMEDIATE
public-MOM companion: TSO admission supplies the quoted
type-33 MessageRetractionHandle before the constrained callback, preserving
the official type-27/type-37/type-40/type-63/type-31 forms and callback
metadata. The paired production-filesystem case also verifies that quoted
type-33 return and immutable file content before the callback, including after
both reporting switches are disabled. Directed DDM, transport, and conformance
remain separate. The nonregional receive-order
`Delete Object Instance` overload now appends its accepted §6.16
successful-void record after its committed deletion transition and before the
separately queued `Remove Object Instance` callback. Its source-backed Table 5
arguments are type-37 `Object instance designator`, the bounded type-63
base-64 `User-supplied tag`, and a type-34 Null placeholder for the absent
optional timestamp. The ordinary timestamped sender invocation is now covered
by the native C++ `HLA_IMMEDIATE` public-MOM companion with production
filesystem reporting disabled; its logical-time supplied record is decoded
before the separately queued callback.
The paired native cases decode official type-27/type-40/type-63/type-31
supplied forms and preserve callback metadata before delivery. The
non-time-regulating sender has the type-34 Null return; the time-regulating
sender receives the quoted type-33 MessageRetractionHandle after TSO admission
and before the constrained callback, which preserves a valid retraction. The
external Java/JPype lane remains complementary, while regional forms remain
separate work. The bounded timestamped recipient callback record is traced
separately above. The
two self-selecting setters, `Set Service Reporting Switch` and `Set Send
Service Reports To File Switch`, remain deferred pending the report-order
source relation recorded in RL-066. The
central explicit-region predicate now implements the documented
Umbra Allow Relaxed DDM policy: enabled federations add only exactly
boundary-touching committed ranges to the strict-overlap set, with no numerical
gap threshold and no loss of existing strict overlap. `../design/RELAXED-DDM-POLICY.md`
records that implementation-defined decision; broader relaxed-DDM behavior and
matrices remain open. RL-024 tracks the Lab/XSD default disagreement.

The sibling object-class `Request Attribute Value Update` overload expands the
same owner solicitation over every current instance registered at the selected
class and its subclasses. It validates the selected class and attributes,
does not require the requester to know each expanded instance, groups work per
provider and object instance, preserves the tag, and rechecks providers at the
callback boundary. Its provider-specific service-report route now survives the
class expansion and appends one type-37/type-1/type-63 §6.22 record per queued
instance callback before provider user code; a focused two-instance filesystem
case proves serial ordering and per-callback accumulation. Additional regional
request forms, automatic provision and broader value-update behavior, DDM,
ownership transfer, FOM sharing policy, save/restore, and remote transport
remain separate work. The accepted requester invocation now also crosses the
public `HLAreportServiceInvocation` route before provider callbacks, preserving
the class designator, attribute set, tag, and Null return argument through the
external Java/JPype surface. A focused class-expansion response case now has the
provider invoke timestamped `Update Attribute Values` from the official
callback; the requester remains time-constrained until its grant and receives
one reflection with the response value/tag, producer, order, time, and
retraction metadata. This is explicit provider code, not automatic provision
or complete timestamped/retraction coverage.

The accepted `Query Attribute Ownership` request now has the same public
MOM boundary: the ownership-management `HLAreportServiceInvocation` is
emitted before the grouped owner/not-owned callbacks, while the C++ registry
remains the sole source of ownership results. The external Java/JPype vector
decodes its object/attribute supplied arguments and Null return record.

The configured public process endpoint now carries the same C++ Query Attribute
Ownership call through a typed request/result operation. Grouped federate-owned
and unowned callbacks cross the receive-order fence and revalidate the pending
object/ownership projection at service dequeue. This is a separate
`process-ownership-query` foundation lane; RTI-owned results, ownership
transitions, interoperability, package/JUnit/protected review, validation, and
conformance remain explicit follow-up gates.

The attribute/interaction order changes and default attribute transportation
change now emit complete public MOM interactions after their C++ registry
transitions, outside the registry lock. The external Java/JPype vector covers
all four service forms and decodes their Table 5 argument records and Null
returns before the call returns to Python.

The single and multiple object-instance-name reservation services now use the
same public MOM boundary after their C++ registry reservations and before the
typed asynchronous reservation callbacks. The external Java/JPype vector
decodes their type-53 String and type-54 StringSet records plus Null returns.
The accepted §6.7 multiple-name release now uses that public boundary after
the atomic registry mutation and outside native locks; the focused C++
HLA_IMMEDIATE observer decodes its type-54 StringSet payload, Null return,
reliable transport, unresolved RTI producer, empty exception, and serial zero.
File-selected/rejected multiple-name paths, external Java/remote evidence,
and conformance remain open.

The attribute and interaction transportation-type request services now use
the same public MOM boundary after their C++ registry plans accept the pending
change. The native C++ embedded profile also consumes the official MIM
`HLArequestAttributeTransportationTypeChange` and
`HLArequestInteractionTransportationTypeChange` interactions through the
public `Send Interaction` surface, decodes their handle payloads, and reuses
the callback-gated registry plans; the focused two-federate Catch2 case proves
that subsequent attribute and interaction delivery uses the confirmed
`HLAbestEffort` type under both `HLA_EVOKED` and `HLA_IMMEDIATE`. A second
publisher in the same case remains on its independent reliable default after
the subject's change, proving the interaction override is invoker-scoped. The
same case checks missing-required-parameter and malformed/unknown-handle
failures without confirmation side effects and observes the accepted request as
a reliable `HLAreportServiceInvocation` with the standard invalid RTI
producer policy. Their standard Java/JPype vectors decode the handle arguments
and Null returns before the C++ confirmation callbacks are delivered.
The MIM classes are under `HLAfederate.HLAservice` in the unmodified 2025 MIM,
not `HLAfederate.HLArequest`; this source-path distinction is kept in the
canonical name constants and is not treated as a Requirements-Lab change.

The corresponding attribute and interaction transportation-type query services
now use that public boundary after their accepted query plans and before the
typed transportation-report callbacks. Their standard Java/JPype vectors
decode the type-37/type-0 and type-15/type-27 supplied records and Null returns.

The class-level `Request Attribute Value Update With Regions` overload now uses
the same private class expansion and owner grouping, with a request-region map
validated for committed ownership and object-class dimension context. Explicit
update associations are solicited only when they overlap the corresponding
request region; attributes using the default region remain eligible, and an
empty region pair is a no-op. The request tag and official Provide callback are
preserved, and callback-entry rechecks prevent stale region state from leaking
into provider code. Its provider-side route now appends the private §6.22
type-37/type-1/type-63 record before each eligible callback; a focused
overlap-qualified filesystem case proves that ordering. A companion focused
case has an overlap-qualified provider
explicitly invoke no-time `Update Attribute Values` from that callback and
checks the requester's response value/tag, producer, transportation, and
conveyed source region. A second focused companion commits a disjoint
requester subscription before the queued reflection boundary and proves
 suppression. Provider responses remain explicit user actions;
 automatic provision, broader DDM, save/restore, and remote transport remain
 separate work. A third focused companion has the provider invoke timestamped
 `Update Attribute Values` from the official callback and proves that the
 overlap-qualified response remains queued for a constrained requester until
 its grant. The reflection preserves timestamp/order, request tag, producer,
 valid retraction handle, and the explicit source RegionHandle; this is one
 composite provider-response case, not automatic provision or complete
 timestamped/retraction coverage.

The same private ownership snapshot now supports bounded `Query Attribute
Ownership`. The request is valid only for an instance known to the requester
and attributes available at that known class. It groups joined-federate-owned
attributes for `Inform Attribute Ownership` and available attributes for
`Attribute Is Not Owned`, then rechecks each queued report so Remove Object
Instance nullifies stale delivery. RTI-owned joined-federate MOM objects use a
separate ledger branch and emit the official grouped `Attribute Is Owned By
RTI` callback without synthesizing a federate handle. The companion read-only
`Is Attribute Owned By Federate` path returns false for that MOM attribute.
This is query-only RTI-owned coverage; ownership mutation, resign-action
disposition, DDM, save/restore, and remote transport remain separate work.

The adjacent `Is Attribute Owned By Federate` service uses the same ownership
snapshot as a read-only boolean check. It validates the invoking federate's
known instance and available attribute, returns true only for that federate's
current ownership, and makes no callback or ownership-state transition.

The bounded 2025 MOM `HLAmodifyAttributeState` path now provides the first
direct RTI-owned ownership-control transition in this slice. The adapter
recognizes the official
`HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState` leaf, decodes the
inherited target-federate coordinate plus object, attribute, and
`HLAownership` values, and delegates to
`EmbeddedFederationRegistry::setFederateMomAttributeState`. The registry
validates federation membership, target knowledge, the target's publication at
its known class, and the predefined MOM-object boundary before resolving the
target's default transportation/order state and mutating the ownership ledger
atomically. `Owned` and `Unowned` are synchronous and callback-free, so no
acquisition/release notification is fabricated. The focused Catch2 lane proves
owner-to-target assignment, unowned state, target-publication rejection,
immediate reassignment, and rejection of an RTI-owned `HLAfederateName`
attribute. The public configuration/API surface remains standards-facing; the
private registry seam is not an additional user-level ownership service.
Complete MOM exception-report delivery and parameter/membership failure
matrices, negotiated/pending arbitration, full RTI-owned query and transition
semantics, resign disposition, remote transport, package/JUnit/protected review,
and conformance remain open. RL-142 records that the Requirements Lab exports
only generic MOM/ownership candidates rather than a row-level
`HLAmodifyAttributeState` candidate.

The first ownership-state transition is the bounded 2025 `Attribute Ownership
Acquisition If Available` path. It validates the requester's known instance,
known-class attribute publication, current ownership, and existing pending
request; a successful call records the private Willing to Acquire state. At
the matching callback boundary it atomically assigns attributes still unowned
to the requester and invokes `Attribute Ownership Acquisition Notification`;
attributes owned by another joined federate invoke `Attribute Ownership
Unavailable` without a release callback. A mixed request can yield both
official callback forms, with the original tag. Repeating the service for an
attribute already in that federate's WTA state changes neither state nor
callback work; an additional eligible attribute gets its own reservation. The
paired `Unpublish Object Class Attributes` path raises
`OwnershipAcquisitionPending` rather than remove a publication needed by either
request. Resignation and receive-order removal clear an undelivered request.
Its accepted request now appends the private service-report file's source-backed
§7.9 record with ObjectInstanceHandle type 37, AttributeHandleSet type 1, and
the Table 5 type-63 base-64 user tag before notification/unavailable work is
queued; rejected calls append nothing. This private file-text record does not
generalize the MIM discrepancy to public MOM interactions (RL-077).
This does not claim complete regular or negotiated
acquisition, divestiture beyond the bounded If Wanted and unconditional
current-recipient transitions, RTI-owned
state, or full resign-action ownership disposition.

The profile also has a deliberately narrow regular 2025 `Attribute Ownership
Acquisition` / `Attribute Ownership Release Denied` transition. A regular
request replaces the same requester's WTA reservation for an overlapping
attribute, keeps a separate pending record, transfers an unowned attribute at
the acquisition-notification callback, or asks a joined remote owner for
release with the original request tag. A repeated regular request does not add
a duplicate release callback. Release denied retains ownership and terminates
all matching regular requests with unavailable callbacks that carry the denial
tag. Its accepted regular request now appends the private service-report file's
source-backed §7.8 record with ObjectInstanceHandle type 37,
AttributeHandleSet type 1, and the Table 5 type-63 base-64 user tag before
any owner-side release or acquisition work is queued; rejected calls append
nothing. This private file-text record does not generalize the MIM discrepancy
to public MOM interactions (RL-077). Its accepted owner-side Release Denied
now appends the private service-report file's §7.12 record with the source's
long unwilling-to-divest AttributeHandleSet name and type-63 base-64 user tag
before Attribute Ownership Unavailable callbacks are queued; rejected calls
append nothing. A bounded `Cancel Attribute Ownership
Acquisition` transition moves only
a still-pending regular request into private cancellation state, suppresses its
stale notification/release work, and retains the matching publication guard
until `Confirm Attribute Ownership Acquisition Cancellation` begins. That
       confirmation groups qualifying attributes for one cancellation call and then
       allows later regular work. A competing `Attribute Ownership Divestiture
       If Wanted` transfer consumes the queued cancellation reservation, making
       the second-form `Attribute Ownership Acquisition Notification` the sole
       terminal reply. The bounded owner-denial race retains the denied
       acquisition until its unavailable callback boundary, so an owner answer
       that arrives after cancellation has begun produces
       `Attribute Ownership Unavailable` with the denial tag and no cancellation
       confirmation. Negotiated acquisition, remaining divestiture flows,
      RTI-owned state, and complete resign-action ownership disposition remain
      open.
Its accepted cancellation now also appends the private service-report file's
source-backed §7.15 record with ObjectInstanceHandle type 37 and
AttributeHandleSet type 1 before the confirmation callback is queued; rejected
calls append nothing. The report slice deliberately does not generalize to
the supplied-empty, in-flight-race, public MOM, or generic return/failure
forms.
The accepted cancellation now also has a public HLA_IMMEDIATE MOM interaction
route. Its report is emitted after the registry cancellation plan and outside
native locks, before the separately queued confirmation callback; the focused
observer decodes service type 3, type-37/type-1 supplied arguments, Null return,
success, reliable transport, empty exception, and serial zero. This remains
development-profile evidence and does not claim file-selected/rejected paths,
remote transport, packaging, Lab validation, or conformance.

The adjacent bounded 2025 `Unconditional Attribute Ownership Divestiture`
service validates the full supplied owner set and immediately removes every
validated ownership record. Existing regular acquisition work is replanned
from the unowned state, while existing If Available requests retain their
already-queued terminal callbacks. The profile groups one `Request Attribute
Ownership Assumption` offer, carrying the divestiture tag, for each currently
eligible joined federate that knows the instance, publishes the attribute at
its known class, and is not already pending an acquisition or cancellation for
it. The callback rechecks that boundary before user code and does not itself
transfer ownership; a recipient must issue a standard acquisition request.
The registry retains unowned search state and rechecks it after later join,
discovery, or publication changes, suppressing duplicate offers. Terminal
assumption-callback re-search now advances on callback return as well: if a
different federate becomes eligible while the first assumption callback is in
user code, the original unowned search queues that federate exactly once.
Completed ownership transfer clears the reservation epoch so a later
divestiture can search afresh. Full owner arbitration remains separate work.

The federation-management registry now consumes the official 2025
`ResignAction` argument for a bounded disposition slice. Directive 1 leaves
owned attributes unowned and queues current eligible assumption offers;
directive 2 removes objects for which the resigning federate owns
`HLAprivilegeToDeleteObject`; standalone directive 3 cancels a voluntary
resigning federate's pending acquisition work, including a selected negotiated
divestiture confirmation and both stale owner callback forms plus an If
Available reservation and its stale requester callback; direct directive 4 now
proves one voluntary delete-then-divest transaction removes a privileged
object before re-offering the retained object's remaining attribute; and
directive 5 cancels that federate's pending acquisition work before applying
delete/divest cleanup. If the resigning
federate is the final joined member, directive 2 is applied even when the
supplied action is `NO_ACTION`. The adapter preserves
the official `FederateOwnsAttributes` and `OwnershipAcquisitionPending`
exceptions and queues assumption/removal callbacks after releasing the registry
lock. Bounded continuation after later publication, discovery, and join is
covered; terminal callback re-search, automatic directives, RTI-owned state,
remaining action combinations, remote transport, and conformance remain
separate work.
The resignation kernel also preflights every eligible assumption callback route
before any delete/divest mutation. A focused private Catch2 case proves that a
missing later route returns an internal request failure without partially
changing ownership or membership; this protects the embedded delivery seam
without claiming the remaining resign-action matrix.

The adjacent bounded 2025 `Attribute Ownership Divestiture If Wanted` service
validates that the caller owns every supplied attribute, returns only the
subset for which an already-pending regular or If Available requester can be
selected, and changes ownership synchronously before returning that subset.
Its private post-transfer notification reservation preserves the selected
acquirer's publication until `Attribute Ownership Acquisition Notification`
begins; that callback receives the divestiture tag, stale work addressed to
the former owner is suppressed, and later regular acquisition work is replanned
against the new owner. When multiple eligible requests exist, the embedded
serial profile chooses the earliest accepted private sequence across both
request forms. That deterministic behavior is an implementation policy, not an
IEEE arbitration priority. The complete negotiated-divestiture lifecycle,
terminal-callback continuation for the unconditional owner search, RTI-owned
state, and complete resign-action disposition remain separate work.

The adjacent bounded 2025 `Negotiated Attribute Ownership Divestiture` slice
now includes `Request Divestiture Confirmation`, `Confirm Divestiture`, and
`Cancel Negotiated Attribute Ownership Divestiture`. The registry retains
the current owner while the supplied attributes wait, chooses only an already
pending regular or Willing-to-Acquire request, and delivers one confirmation
callback with that acquisition tag. Successful confirmation transfers the selected
attribute before the standard acquisition-notification callback, which carries
the Confirm Divestiture tag. A selected regular acquirer that cancels produces
the official `NoAcquisitionPending` result and resets the private state to
waiting; explicit cancellation removes the waiting state and re-plans ordinary
release work. Ordinary release reservations persist while their regular
acquisition is pending, but a queued release that reaches negotiated Waiting
state is consumed and suppressed so the negotiated/cancelled race cannot yield
a duplicate owner callback. Its accepted §7.3 request appends the private
service-report file's type-37 object, type-1 attribute-set, and type-63
base-64 tag record before Request Divestiture Confirmation work is queued; the
Table 5 tag literal remains private file text (RL-077). Its accepted §7.14 cancellation also appends the
private service-report file's type-37 object and type-1 attribute-set record
before restored ordinary release work is queued; RL-017's generated state-chart
inconsistency does not define that source-backed runtime transition. This is not
the complete negotiated-divestiture lifecycle: ongoing owner search, standards-
level arbitration, and negotiated acquisition remain separate work. The
Willing-to-Acquire selection is deliberately a deterministic private policy;
its focused contract and lane do not promote it to conformance evidence.

The negotiated ownership slice now also has a two-candidate continuation lane.
Two If Available requests remain pending while the owner enters negotiated
Waiting; the earliest private request is selected, and that candidate resigns
with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`. Reissuing the negotiated request
selects the retained second candidate and carries its acquisition tag to Request
Divestiture Confirmation. Confirmation is completed only after the common Flush
Queue boundary, so both regional recipients receive the publisher-owned saved
reflection before the retained candidate receives exactly one acquisition
notification. This is a focused development-profile test of continuation and
callback ordering, not a standards-mandated arbitration priority or complete
negotiated owner-search implementation.

The tag-free cancellation now also has a public MOM route. The accepted
service report is emitted after the registry lock is released, so an
`HLA_IMMEDIATE` observer can receive one reliable
`HLAreportServiceInvocation` before the restored ordinary release callback is
queued. The interaction case decodes the official service type, object and
attribute-set supplied arguments, Null return, success indicator, empty
exception, and serial zero. The Table 5 user-tag forms for the neighboring
negotiated/confirm/divestiture services remain filesystem-only until the
type-63/Table-5 versus type-60/MIM conflict in RL-077 is resolved.

The accepted timestamped `Retract` service now uses the same public selector
after the federation-owned retraction ledger accepts its designator. A focused
three-federate case keeps the timestamped recipient on `HLA_EVOKED` while an
`HLA_IMMEDIATE` observer decodes service type 4, the type-33
`MessageRetractionDesignator`, the quoted `MessageRetractionHandle<...>`
value, the type-34 Null return, and the success/empty-exception fields. The
report is delivered before the separate `Request Retraction` callback is
evoked, and the configured filesystem remains unchanged while file reporting
is disabled. This is development-profile evidence; failure, transport,
alternate callback model, and conformance cases remain open.

The same profile now implements the mandatory 2025 transportation-type lookup
pair, `getTransportationTypeHandle` and `getTransportationTypeName`, for the
two predefined names plus declarations in the composed FOM. Custom names use
an execution-scoped directory: an additional-FOM join cannot renumber a handle
already issued to a joined federate, and all members resolve the same name/
handle pair. The no-region receive-order interaction and attribute-update
paths now use the effective per-federate type. The bounded transport-control
services provide prospective per-class attribute defaults, callback-gated
instance changes and queries, plus callback-gated published interaction
changes and queries for future ordinary/regional sends. This remains an
execution-local transportation policy and message metadata implementation,
not a distributed transport implementation, and its no-time paths do not
imply the separate bounded timestamped object-lifecycle slice.

The profile also implements the mandatory 2025 order-type lookup pair,
`getOrderType` and `getOrderName`, for only `Receive` and `TimeStamp`, with
official connection, membership, invalid-name, and invalid-type exception
mapping. The three official order-control services are now also wired through
the embedded registry: class defaults are prospective and per-federate,
registered instances capture their preferred attribute order, explicit instance
changes affect future owned updates, ownership transitions reset the captured
value from the acquiring federate's default, and interaction changes are scoped
to the invoking publisher. Timestamped interaction and attribute planners carry
the selected order into callback metadata and partition mixed Receive/TimeStamp
traffic. Save/restore, alternate advance modes, remote transport, and complete
time/TSO coordination remain separate work.

The accepted 2025 federate and object-class lookup pairs are also covered by a
paired public-MOM slice. With file reporting disabled, an HLA_IMMEDIATE
observer receives four reliable type-6 service reports for
`getFederateHandle`/`getFederateName` and
`getObjectClassHandle`/`getObjectClassName`, preserving the official typed
supplied/returned arguments, success and empty-exception fields, serial order,
and the development profile's invalid-producer/no-region policy. This is
direct C++ evidence for the public interaction route; the Lab's missing
row-level ReturnArgument export (RL-149), failure families, protected review,
package/JUnit evidence, and conformance remain open.

The same public-MOM route now covers the accepted §10.6--§10.12 support
lookups. With file reporting disabled, an HLA_IMMEDIATE observer receives
seven reliable type-6 reports in serial order for known-object,
object-instance, class-attribute, and update-rate queries. The focused case
decodes the official type-37/type-36, type-53, type-0, and type-35 supplied /
returned forms plus success, empty exception, invalid producer, and no-region
metadata. RL-151 remains the Lab's missing positive ReturnArgument export;
failure families, protected review, package/JUnit evidence, validation, and
conformance remain open.

The public-MOM route also covers the accepted §10.13--§10.16 interaction-class
and parameter lookup pairs. With file reporting disabled, an HLA_IMMEDIATE
observer receives four reliable type-6 reports in serial order and decodes
the official type-53/type-27 and type-27/type-53 or type-39 supplied/returned
forms, success, empty exception, invalid producer, and no-region metadata.
RL-150 remains the Lab's missing positive ReturnArgument export; failure
families, protected review, package/JUnit evidence, validation, and
conformance remain open.

The public-MOM route also covers the accepted §10.17--§10.20 order and
transportation lookup pairs. With file reporting disabled, an HLA_IMMEDIATE
observer receives four reliable type-6 reports in serial order and decodes
the official type-53/type-38 order and type-53/type-59 transportation
supplied/returned forms, success, empty exception, invalid producer, and
no-region metadata. RL-147 remains the Lab's missing positive ReturnArgument
export; failure families, protected review, package/JUnit evidence,
validation, and conformance remain open.

The public-MOM route now also covers the accepted §10.21--§10.26 dimension and
region lookup services. With file reporting disabled, an HLA_IMMEDIATE
observer receives six reliable type-6 reports in serial order and decodes the
type-36/type-27 class-handle and type-11 dimension-set forms, type-53/type-10
name/handle forms, type-35 upper-bound Number, and type-42/type-11 region
dimension-set form, together with success, empty exception, invalid producer,
and no-region metadata. RL-042/RL-067/RL-076 remain the Lab's missing positive
ReturnArgument export; failure families, protected review, package/JUnit
evidence, validation, and conformance remain open.

The dimension and region foundation is intentionally bounded: it exposes the
FOM-defined dimension identity and upper bound, implements the private
region-template/specification lifecycle, and supplies committed specs to the
interaction and bounded object-attribute regional slices. It does not implement
general region realization, additional regional request edge cases, the remaining
timestamped default-region/re-enable matrix beyond the bounded FQR/TARA/NMRA
companion, remaining mixed-fanout timestamped regional forms, or broader DDM
routing. The current timestamped regional attribute-update slice covers
committed update-region association, one immediate and one constrained TSO
recipient, pending retraction, and callback-time Convey Region Designator Sets
gating. A focused explicit-source replacement case additionally proves that a
queued passel captured under one update-region association is not retargeted
when the association is replaced before its callback boundary; a later passel
uses the replacement region. This does not close the remaining alternate,
transport, lifecycle, persistence, or broader DDM matrix.

The region lifecycle now also treats a committed receiver-range mutation as a
discovery boundary for ordinary regional object instances. When an already
registered source object was disjoint at registration time, the registry
compares the prior and newly committed region snapshots and queues one
`Discover Object Instance` callback when the receiver enters overlap; the
callback is reserved in the same transaction and delivered through the normal
HLA_EVOKED/HLA_IMMEDIATE queue. The focused `region-commit-discovery` Catch2
case proves the disjoint-to-overlap transition without a new subscription or
registration event. This is bounded regional discovery coverage, not a claim
for complete DDM region realization or the remaining response/advisory matrix.
The same boundary is now covered for `Associate Regions For Updates`: adding
an overlapping source realization to an already-registered object compares the
old and revised association maps, reserves one discovery in the accepted
transaction, and routes it through the standard callback-time recheck. The
focused `association-discovery` Catch2 case proves this without a new object
registration or subscription event and also covers `Unassociate Regions For
Updates`, where removing a disjoint association restores the default source
realization and discovers a second existing object. The broader regional
response, ownership, and transport matrices remain separate work.

The bounded receive-order default-region slice now treats the RTI-provided
default as derived private state, never as a caller-visible `RegionHandle`.
For dimensional object attributes and interactions, it is selected when no
explicit source association or regional declaration supplies a non-default
realization; committed non-empty explicit regions overlap it, while an empty
region overlaps none. The same predicate feeds discovery, scope/advisory
planning, reflection/interaction delivery, update-rate lookup, and Convey
Region Designator Sets metadata (supplied and empty). The paired
`default-region-requirements-contract.json` and Catch2 cases are bounded
development-profile traceability only. They cover receive-order, one
time-constrained timestamped object reflection and interaction callback with
the supplied-empty convention, and default-source object and interaction mixed
fanout where each delivered nonconstrained regional recipient receives Request
Retraction while its constrained pending passel is withdrawn before the grant;
the bounded interaction companion also retains one queued default-source passel
across a Time Constrained disable/re-enable transition before its grant. The
remaining timestamped/re-enable matrix and DDM surface are still separate work.
An explicit-source regional interaction companion now preserves the queued
source RegionHandle, tag, timestamp, and order across that same transition and
proves one callback before the post-re-enable grant. A second explicit-source
subscription-replacement companion removes receiverRegionA and adds disjoint
receiverRegionB before the queued callback, proving the old sourceRegionA
passel is suppressed rather than retargeted and that a later sourceRegionB
passel is delivered once. The corresponding default-source object-update
companion preserves an ordinary-registration timestamped passel across the
transition and proves one reflection before the post-re-enable grant with a
supplied-empty default-region marker. A default-source and explicit-source
interaction companions also exercise ordinary TAR/NMR plus the inclusive TARA
and NMRA boundaries, preserving callback-before-grant ordering and the
supplied-empty/source-region markers; further alternate advances remain open. A
mixed-member regional companion now drives one overlap-qualified TSO
payload through FQR, TARA, and NMRA recipients and asserts each callback before
its own grant. A regional TAR/NMR companion separately drives one
overlap-qualified TSO payload through ordinary grants and asserts each callback
before its corresponding grant. A regional timestamped attribute-update
companion drives one overlap-qualified object passel through ordinary TAR/NMR
grants and preserves its source-region metadata before each grant. A mixed-
member regional object-update companion now drives that passel through FQR,
TARA, and NMRA callback frontiers and preserves the same source-region metadata
before each grant.
The default-source/default-region object-update companion now drives one
ordinary timestamped passel through the same FQR/TARA/NMRA frontiers: each
reflection precedes its grant, FQR preserves actual/optimistic time 7, the
producer TAR completes at 2, and the callback carries a supplied-empty
`RegionHandleSet`.
The adjacent resignation probe keeps one such default-source passel queued while
the producer resigns with `UNCONDITIONALLY_DIVEST_ATTRIBUTES`; an independent
regulator releases the recipient before its TAR(7) grant, and the callback
retains producer, payload, tag, timestamp/order, valid retraction metadata, and
the supplied-empty region marker. This is clean focused lifecycle coverage,
not a recurrence or conformance evidence; the r12 2025 resync added no Lab ID
changes and consumes no RL-176 observation.
The matching default-source/default-region interaction companion drives one
ordinary timestamped Send Interaction through those same three frontiers,
preserving callback-before-grant ordering, FQR actual/optimistic time 7, the
producer TAR at 2, and the supplied-empty callback region marker.
A matching native C++ object-update companion now keeps one
default-source/default-region timestamped passel queued while the producer
disables and callback-gated re-enables Time Regulation. The focused
`timestamped-default-region-attribute-regulation-reenable` lane now runs both
the unchanged-lookahead and changed-lookahead cases; in the latter, the
producer re-enables at lookahead three and advances to the new GALT boundary.
Both cases prove one reflection before the matching grant with the original
payload and retraction metadata. The synchronized Requirements-Lab contract is
additive development-profile traceability only.
A bounded four-member mixed-family save/restore companion now composes one
saved timestamped object update with one saved timestamped interaction. After
both post-save designators are terminalized, restore reconstitutes the two
recipient-local ledgers; independent Flush Queue requests deliver the original
payloads before their matching grants, and post-delivery Retract reaches both
recipients. This is focused development-profile evidence only; durable
persistence, timed/changing-membership recovery, arbitrary handle remapping,
remote transport, package/JUnit/protected review, and conformance remain open.
The adjacent two-recipient resignation companion keeps one timestamped
default-source Send Interaction queued for overlapping regional subscribers,
admits direct TAR(7) and NMR(10), then resigns the producer with `NO_ACTION`.
An independent regulator releases each recipient-local queue in turn; both
Receive Interaction callbacks precede their grants and retain the original
producer, payload, tag, timestamp/order, supplied-empty region marker, and
retraction metadata. This is clean lifecycle coverage rather than a
Requirements Lab recurrence; the unchanged r12 resync consumes no RL-176
observation.

The changed-lookahead re-enable companion keeps one non-regional timestamped
interaction queued while the producer disables and callback-gated re-enables
Time Regulation from lookahead one to lookahead three. Query Lookahead confirms
the new value; producer TAR(2) then establishes the timestamp-five GALT boundary
for the constrained recipient. The focused native C++ lane verifies the
original callback metadata, callback-before-grant ordering, and terminal
retraction classification. This remains development-profile traceability only;
alternate advances, other timestamped families, transport, save/restore,
package/protected-review evidence, and conformance remain open.

The matching default-source/default-region timestamped interaction case now
also exercises the changed-lookahead re-enable boundary: the producer disables
Time Regulation, re-enables at lookahead three, and advances to the new
timestamp boundary. The focused native C++ lane preserves the original receive
callback metadata and callback-before-grant ordering across both unchanged- and
changed-lookahead scenarios; this remains development-profile evidence only.

The ownership/DDM boundary now also clears explicit object-attribute
update-region associations when the current owner loses ownership. The helper is
called by If Available acquisition transfer, Divestiture If Wanted, Confirm
Divestiture, unconditional divestiture, unpublish, and resignation paths. The
focused Catch2 scenario at
`cpp/tests/ownership_transfer_update_region_catch2.cpp:142` passes 88
HLA_EVOKED assertions and the paired `ownership-transfer-update-region` Lab
contracts now point to that standalone source. It proves the If Available/
If Wanted path: clearing a former owner's explicit association restores the
default source realization, and a new owner can replace it with an owned
explicit region. This is
still a bounded development-profile slice; complete ownership arbitration,
the complete timestamped default-region matrix, complete regional advisory
coverage, package evidence, and conformance remain future work.

Every newly implemented public method must validate preconditions, mutate state
atomically, queue callbacks according to the selected model, and map failure to
the official exception hierarchy.

Current evidence: Catch2 calls the official symbols through the factory, covers
valid/repeated/invalid connection transitions, empty-queue callback-control
behavior, a callback-session shutdown fence, active federate name/handle
lookup with post-resignation designator retention, stable FOM-backed object-/interaction-class and inherited-attribute/
parameter name/handle lookup, mandatory transportation-type name/handle lookup,
per-federate interaction and object-class attribute declaration state and resign cleanup,
ordinary hierarchy-aware declaration relevance advisories with their per-federate
switches and recipient-local pre-callback service-report records, including
active/passive regional declaration-relevance transitions,
unnamed object registration/discovery with known-instance lookup in both callback models and
the recipient-local §6.9 report record at callback entry,
no-time object deletion/removal with tag/producer propagation in both callback models,
limited receive-order interaction and attribute-update delivery in both
callback models, active/passive ordinary and regional interaction eligibility,
the regional interaction subscription/send case with overlap and
callback-recheck assertions, the regional object-attribute registration /
association / subscription case with active-overlap-filtered no-time reflection
and passive suppression, and the
object-instance Request/Provide Attribute Value Update owner-solicitation path
with tag propagation and a resigned-provider delivery fence, the object-class
Request/Provide path across registered subclasses without requester discovery,
the class-level regional Request/Provide path with overlap/default-region
filtering,
the Query Attribute Ownership path with federate-owned/unowned grouping and a
Remove Object Instance cancellation fence, the read-only Is Attribute Owned By
Federate check, the bounded Attribute Ownership Acquisition If Available
pending/callback transition, the bounded regular Attribute Ownership
Acquisition / Release Denied / cancellation callback transition, the bounded
Divestiture If Wanted returned-set / notification / follow-up-release
transition, the bounded Unconditional Divestiture / current-recipient
Assumption-offer transition, the bounded Negotiated Divestiture /
Request-Confirmation / Confirm / Cancel transition, and the
development-profile two- and three-federate federation lifecycle plus listing,
time-advance, temporal-role, no-TSO GALT/LITS-query, and limited GALT/NRG
scheduler reports, and the bounded untimed federation-save and process-local
federation-restore control planes with per-member status/failure callbacks,
object-state rollback, one logical-time/actual-lookahead rollback boundary,
one deferred-lookahead rollback boundary, paired HLA_EVOKED/HLA_IMMEDIATE
saved-pending NRG Time Advance Request rollback boundaries, a six-scenario
HLA_EVOKED/HLA_IMMEDIATE TARA/NMR/NMRA rollback matrix, paired HLA_EVOKED/
HLA_IMMEDIATE saved-pending Flush Queue Request rollback boundaries, and resignation
cleanup. The HLA_EVOKED restore-resignation boundary now fails the in-flight
restore for surviving members and resets the departing ambassador's callback
dispatcher so queued restore callbacks cannot leak past resignation. The untimed
save coordinator now also defers a request when constrained members exist,
waits until every constrained member has a pending ordinary grant, invokes
each constrained member's Initiate Federate Save callback directly before its
grant, then queues the non-time-constrained recipients. The exact timestamped
`Request Federation Save(label, LogicalTime)` overload remains a separate
bounded slice: it retains one replaceable pending request, validates the
official logical-time implementation, waits for constrained federate positions
and TSO delivery through the requested boundary, then invokes each qualifying
constrained member directly before its Time Advance Grant. Only after all
constrained admissions does it queue non-time-constrained members. Catch2
currently proves the inclusive TAR boundary, including a TSO message at the
scheduled save time and a three-member admission sequence, plus TARA's
exclusive boundary and the NMR/NMRA inclusive/exclusive next-message
boundaries. It also proves strict actual-FQG admission: an equal grant remains
pending, a later FQG follows queued TSO and direct initiation, and mixed
FQR/TAR membership is prequalified before the operation starts. A separate
three-member TARA/NMRA case proves both strict ordinary modes are
prequalified before non-time-constrained notification, and a six-member case
combines all five advance modes. A cross-member TSO case proves a member's
queued payload precedes its direct initiation even after another member starts
the operation, and an in-transit callback case proves a newly requested save
waits for the recipient's callback to return. The paired HLA_EVOKED/
HLA_IMMEDIATE saved-pending NRG TAR cases, the six-scenario TARA/NMR/NMRA
matrix under both callback models, and paired HLA_EVOKED/HLA_IMMEDIATE
saved-pending Flush Queue Request cases recreate their fresh stale-work-fenced
grants only after Federation Restored; the filesystem save-commit seam now
records a canonical route-free `umbra-federation-state/v1` payload, including
per-federate ordinary/regional/directed interaction declarations and
interaction transport/order overrides, plus ordinary/directed/attribute-update/
timestamped-object-deletion payload bytes, recipient projections,
source-region snapshots, and queued/in-transit/delivered recipient phase
records; it validates its identity and official encoded temporal values during
restore admission, and gates Federation Saved on successful persistence;
queue-phase plus all four bounded delivery-payload rehydration families,
the timestamped-deletion invocation snapshot, and the shared retraction-
recipient ledger are now covered by the bounded restore seam. The typed
ownership slice also serializes and rehydrates If Available and regular
acquisition reservations (request identity, ordering sequence,
desired/queued/unavailable attributes, owner-release callback partitions,
user tag, cancellation identity/attribute sets, and Divestiture-If-Wanted
notification identity/attribute sets, Confirm Divestiture notification
identity/attribute sets, pending attribute transportation-type changes, and
negotiated-divestiture candidate/confirmation state, plus retained
ownership-assumption recipient/tag ledgers, and the typed known-class/pending
discovery/removal visibility projection, plus connection-loss automatic/
deferred removal classifications and timestamped-deletion message/recipient
linkage), and accepted Update Attribute Values telemetry (lifetime count,
class/transportation buckets, and distinct updated-object projections), plus
the accepted application Reflect Attribute Values callback count, its
class/transportation buckets and distinct reflected-object/class projections,
the joined-federate object-lifecycle MOM counters (Registered, Deleted,
Removed, and Discovered), and accepted Send Interaction telemetry
(total/directed counters and class/transportation buckets), plus accepted
Receive Interaction telemetry (total/directed receipt counters and
class/transportation buckets), while preserving unsupported live-only pending
work, and the exact federate-owned object-instance-name reservation map, are
rehydrated when the process-local object snapshot is present. The focused
filesystem process-restart ownership-assumption regression also admits the
route-free recipient/tag ledger into a fresh registry, persists the separate
still-queued callback tuple, rebinds that callback to the live recipient route,
and proves that a later discovery/publication continues the search without
replaying the recorded candidate. A callback that already crossed its begin
boundary is not replayed. The same image
now carries the per-federate object-class attribute publication/subscription,
update-rate, regional, default-transportation, default-order, and
delete-privilege declaration ledgers, with canonical ordering and restore-time
catalog/region validation. Pending synchronization-point labels, tags,
participant/announcement sets, and achievement results are also carried in a
typed canonical section and restored with live-member validation; the remaining
federation-owned region specifications now carry their owner, dimension set,
pending/committed range maps, commit state, and in-use flag through the same
typed image and are restored before regional declaration validation; the remaining
ownership/value-ledger rehydration, timed restore,
role/membership churn,
remaining service interlocks, transport, and conformance remain future work.
The new two-member HLA_EVOKED save/restore case closes the bounded
multi-member pending-TAR gap: separate callback routes hold both requests in
the process-local image, and restore delivers independent fresh grants only
after each member's Federation Restored callback. Durable, timed, and remote
restore plus the broader queued/in-transit TSO matrix remain separate work;
the bounded ordinary-interaction fan-out recovery slice is described below.
Its restore phase now drains and checks the two callback routes independently:
the first member's restored grant is visible while the second member remains
at its pre-restore grant ledger, then the second member receives its own fresh
grant. This is a per-federate isolation assertion within the existing mapping,
not a claim about cross-process ordering or durable restore.
A focused synchronization-point companion now covers the adjacent registry
state boundary: an announced but unachieved point is saved, the live point is
completed, and the canonical typed image is restored before achieving it again.
The second Federation Synchronized callback proves that the saved
synchronization ledger, rather than the post-save live state, was restored.
This is clean native C++ development-profile evidence only; distributed,
complete save/restore, validation, and conformance remain open.
One bounded three-member non-regional attribute-update restore case now saves
one queued timestamped passel for two constrained recipients, restores both
recipient ledgers, and proves independent Flush Queue delivery followed by
Request Retraction; it does not claim general multi-member pending/in-transit
or durable restore coverage. A companion one-member terminal-tombstone case
saves an already-terminal timestamped attribute retraction record, restores
the saved ledger, and proves the saved handle remains
`MessageCanNoLongerBeRetracted` while a post-save handle is invalid; this is
classification evidence only, not live-payload or multi-member recovery. A
matching one-member terminal timestamped-object-deletion case saves the
already-reclaimed deletion tombstone and released name state, restores the
saved classification, rejects a distinct post-save deletion handle, and
keeps the released name absent; other terminal-lifetime families remain open.
A bounded two-member terminal timestamped-directed-interaction case now saves
the terminal designator alongside a separate live passel used for constrained
save admission, restores the saved classification, and rejects a distinct
post-save directed handle; it deliberately does not claim directed callback
delivery or live-payload recovery. A matching two-member explicit-source
regional-interaction case preserves the terminal designator across restore
with a separate overlap-qualified admission passel and rejects a distinct
post-save regional handle; region mutation, callback delivery, and live
payload recovery remain open.
A matching two-member explicit-source regional-attribute case now preserves a
terminal timestamped Update Attribute Values classification across restore
with a separate overlap-qualified admission passel and rejects a distinct
post-save regional-attribute handle; regional callback delivery, live payload
recovery, and region mutation remain open.
A matching two-member default-source regional-attribute case now preserves a
terminal timestamped Update Attribute Values classification across restore
with a separate live private-default admission passel and rejects a distinct
post-save default-region handle; callback delivery, live payload recovery, and
region mutation remain open.
A matching two-member default-source regional-interaction case now preserves a
terminal timestamped Send Interaction classification across restore with a
separate live private-default admission passel and rejects a distinct post-save
default-region interaction handle; callback delivery, live payload recovery,
and region mutation remain open.
The timestamped regional attribute-update slice now has a real Catch2 scenario
and paired Requirements-Lab contracts: committed update-region association is
carried into the TSO reflection payload, immediate and constrained recipients
are split correctly, pending retraction is tested before the grant, and
callback-time Convey Region Designator Sets gating is verified. A focused
explicit-source replacement scenario now also proves that a queued passel is
not retargeted after its update-region association is replaced, while a later
passel carries the replacement RegionHandle. A focused non-regional companion
also exercises TARA at defined GALT and NMRA at the
next queued-message boundary, asserting reflection-before-grant ordering and
source/time/order/retraction/tag metadata. A second companion covers FQR/Flush
Queue for the same non-regional family, delivering all queued passels before
the grant and asserting actual-grant and optimistic-floor values. A separate
HLA_EVOKED interaction companion accepts FQR before in-process future input
arrives, while the pure grant calculator includes explicit in-transit payloads
in its undelivered frontier. Regional explicit-source and default-source
interaction companions preserve the source-region set before FQG, including
      the supplied-empty marker for the private default region. A regional
      TAR/NMR companion also exercises ordinary callback frontiers before each
       grant. The matching explicit-source regional object-update re-enable
       companion also preserves its source RegionHandle, callback order,
       timestamp, tag, and conveyed-region metadata across the same transition.
       A regional timestamped attribute-update companion also exercises
      ordinary TAR/NMR callback frontiers for an overlap-qualified object
      passel. Mixed-member regional interaction and object-update companions
      exercise FQR, TARA, and NMRA callback frontiers in one shared source-region
      overlap. A paired negative regional object-update companion retracts an
      overlap-qualified timestamp-8 passel before any FQR/TARA/NMRA callback,
      verifies all three grants plus the producer TAR, and records the strict
      timestamp-versus-TAR-plus-lookahead setup. Remote future-input/
The new bounded explicit-source regional object-update restore companion also
preserves one queued attribute passel, its object/update association, source
RegionHandle set, and live retraction ledger across an untimed snapshot. It
terminalizes the post-save designator, restores the image, delivers through
Flush Queue Request before the grant, and verifies Request Retraction from the
original designator; RL-137 records the corresponding Lab cross-service gap. A
matching default-source interaction restore companion preserves one queued
timestamped payload, its private source realization, supplied-empty callback
projection, and live retraction ledger across the same untimed snapshot before
Flush Queue delivery and Request Retraction; RL-138 records the corresponding
default-source Lab cross-service gap. A matching non-regional attribute
restore companion preserves one queued typed Update Attribute Values passel,
its ordinary recipient ledger, and live retraction designator through an
untimed snapshot before Flush Queue reflection and Request Retraction; RL-139
records the corresponding non-regional attribute Lab cross-service gap. A
matching default-source attribute restore companion preserves one ordinary
registration passel, its private default realization, supplied-empty callback
projection, and live retraction ledger through the same untimed snapshot before
Flush Queue reflection and Request Retraction; RL-140 records the corresponding
default-source attribute Lab cross-service gap.
Timed save/restore companions now schedule save at logical time 6 while
retaining timestamp-8 default-source attribute and interaction passels plus a
target-qualified directed passel, cross the boundary for both members,
terminalize the designators after save, and restore them for FQR
reflection/interaction/directed delivery and Request Retraction. RL-141 records
the corresponding timed save-boundary gap; durable restore and other
payload/advance variants remain open. The ordinary non-regional interaction
family now has a matching three-member fan-out restore companion: one queued
timestamped `Send Interaction` is restored to two constrained recipient-local
queues, each copy is delivered independently through `Flush Queue Request`,
and the original designator reaches both recipients through `Request Retraction`.
The focused CTest tag is
`timestamped-interaction-restore-multi-recipient`; this closes only that
in-process untimed recovery slice, not the broader timed/durable/in-transit,
changed-membership, ownership, transport, package, or conformance matrix.
The matching non-regional object-deletion fan-out restore companion preserves
one queued timestamped `Delete Object Instance`, its object-reconstitution
snapshot, and two recipient-local removal ledgers through the same untimed
snapshot. Each `Remove Object Instance` copy is released independently through
`Flush Queue Request`, and the original designator reconstitutes the object
for both recipients through `Request Retraction`. Its focused CTest tag is
`timestamped-object-deletion-restore-multi-recipient`; timed/durable/in-transit
restore and the broader changed-membership, ownership, transport, package,
and conformance matrix remain open.
The target-qualified directed-interaction family now has the corresponding
three-member fan-out restore companion: one queued timestamped `Send Directed
Interaction` is restored to two constrained recipients, each directed copy
is delivered independently through `Flush Queue Request`, and the original
designator reaches both recipients through `Request Retraction`. Its focused
CTest tag is
`timestamped-directed-interaction-restore-multi-recipient`; selector mutation,
timed/durable/in-transit restore, changed membership/ownership, directed DDM,
transport, package, and conformance remain open.
A matching explicit-source regional-interaction fan-out restore companion now
restores one overlap-qualified timestamped `Send Interaction With Regions` to
two constrained recipient-local queues. Each callback preserves the source
`RegionHandleSet` and is delivered independently through `Flush Queue Request`
before the original designator reaches both recipients through `Request
Retraction`. Its focused CTest tag is
`timestamped-regional-interaction-restore-multi-recipient`; region mutation,
timed/durable/in-transit restore, changed membership/ownership, alternate
advances beyond this FQR boundary, transport, package, and conformance remain
open.
A native save/restore boundary now keeps the RTI-owned joined-federate MOM
`HLAreportServiceFile` value explicit and immutable. Restore validates each
saved joined-federate path against the live writer identity before mutating
federation state; missing or duplicate identities and path changes fail
deterministically. The focused Catch2 case `Embedded joined-federate MOM
report-file identity survives save and restore` passes 46 assertions and
proves public MOM reflection, stable object/path identity, and continued file
appends after restore. The Requirements Lab remains frozen input for this
native implementation change.
A matching default-source/default-region attribute-update fan-out restore
companion now restores one queued timestamped `Update Attribute Values` passel
to two constrained regional subscribers. Each reflection preserves the
supplied-empty sent-region marker and is delivered independently through
`Flush Queue Request` before the original designator reaches both recipients
through `Request Retraction`. Its focused CTest tag is
`timestamped-default-region-attribute-restore-multi-recipient`; explicit-source
replacement, timed/durable/in-transit restore, region mutation, alternate
advances beyond this FQR boundary, transport, package, and conformance remain
open.
The explicit-source regional-interaction recovery family now also has a timed
save-boundary companion. It schedules save at logical time 6 while a timestamp-8
`Send Interaction With Regions` remains queued, crosses the boundary for both
federates, terminalizes the post-save designator, restores the committed source
`RegionHandle`, and delivers through `Flush Queue Request` at actual time 7 with
optimistic time 8 before `Request Retraction`. The focused CTest tag is
`timestamped-regional-interaction-timed-restore`; durable persistence, alternate
advance modes, region mutation, and broader recovery remain open.
The explicit-source regional object-update family now has the matching timed
save-boundary companion. It schedules save at logical time 6 while a timestamp-8
`Update Attribute Values` passel remains queued, restores the object/update
association, source `RegionHandle`, and live retraction ledger, and proves
`Flush Queue Request` reflection at actual time 7 with optimistic time 8 before
`Request Retraction`. Its focused CTest tag is
`timestamped-regional-attribute-timed-restore`; durable persistence, alternate
advances, region mutation after saving, and broader recovery remain open. The
source-backed case is
`cpp/tests/timed_restore_live_tso_regional_attribute_update_catch2.cpp:190`
with 84 HLA_EVOKED assertions, 23 Requirements-Lab anchors, 15 canonical 2025
sections, and 23 official C++ API surfaces; use the indexed `focus`, `trace`,
`matrix`, and `check --lane` handles rather than reopening the full plan.
The companion source-resignation case is now also source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp:180`
with 106 HLA_EVOKED assertions, 27 Requirements-Lab anchors, 18 canonical 2025
sections, and 23 official C++ API surfaces. Its independent lane is
`tso-regional-attribute-update-timed-live-resignation-state`; it keeps the
timestamp-8 passel through source-region mutation, restore, disjoint mutation,
and producer resignation before survivor Flush Queue delivery. Use its exact
indexed handles instead of reopening the full plan.
The same family now has a three-member fan-out companion: one timestamp-8
explicit-source passel is restored into two constrained recipient ledgers while
the non-constrained regulator crosses the same logical-time-6 boundary. Each
recipient is released independently through `Flush Queue Request` at actual
time 7 with optimistic time 8, preserving the committed source `RegionHandle`
and the original retraction identity before `Request Retraction` reaches both.
Its focused CTest tag is
`timestamped-regional-attribute-timed-restore-multi-recipient`; durable
persistence, alternate advances, region mutation, and broader recovery remain
open. The source-backed Catch2 case is
`cpp/tests/timed_restore_live_tso_regional_attribute_update_multi_recipient_catch2.cpp:190`
with 157 HLA_EVOKED assertions, 23 Requirements-Lab anchors, 15 canonical
2025 sections, and 23 selected official C++ API surfaces. Query it through the
indexed focus/trace/matrix/check handles so this three-member fan-out remains
an independently runnable slice. The companion tag
       `tso-regional-attribute-update-timed-multi-resignation-state` now proves source
       mutation, restore, disjoint mutation, and producer resignation for both
       constrained recipients. Its source-backed Catch2 case is
       `cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_source_resignation_after_restore_catch2.cpp:190`
       with 167 HLA_EVOKED assertions, 27 Requirements-Lab anchors, 18
       canonical 2025 sections, and 22 selected official C++ API surfaces.
The companion tag
`tso-regional-attribute-update-timed-delete-state` now adds
`DELETE_OBJECTS_THEN_DIVEST`: after the same restore and source-region
mutations, each constrained recipient receives one delivery-boundary Remove
Object Instance and no stale timestamped reflection. Its source-backed Catch2
case is
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_delete_then_divest_after_restore_catch2.cpp:218`
with 124 HLA_EVOKED assertions, 36 Requirements-Lab anchors, 19 canonical
2025 sections, and 25 selected official C++ API surfaces. Resolve it with the
indexed focus/trace/matrix/check commands; the exact CTest selector is printed
by `focus`. The matching `CANCEL_THEN_DELETE_THEN_DIVEST` companion is now
source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_cancel_then_delete_then_divest_after_restore_catch2.cpp:218`.
It is indexed as `tso-regional-attribute-update-timed-cancel-state` with 124
HLA_EVOKED assertions, 36 Requirements-Lab anchors, 19 canonical 2025 sections,
and 25 selected official C++ API surfaces. It covers the same restored,
disjoint-source timestamp-8 passel as the delete-then-divest case, suppresses
the stale reflection, and delivers one removal per constrained recipient at
independent Flush Queue boundaries. Query the exact slice with
`python tools/query_rti_work.py focus tso-regional-attribute-update-timed-cancel-state --summary --compact`;
the narrower pending-ownership slice is source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_cancel_pending_ownership_after_restore_catch2.cpp:251`.
It is indexed as `tso-regional-attribute-update-timed-cancel-pending-ownership-state`
with 130 HLA_EVOKED assertions, 39 Requirements-Lab anchors, 21 canonical 2025
sections, and 28 selected official C++ API surfaces. Query it with
`python tools/query_rti_work.py focus tso-regional-attribute-update-timed-cancel-pending-ownership-state --summary --compact`;
the combined cancel-then-delete case intentionally has no pending ownership
request and therefore makes no cancellation-callback claim. The next bounded
action-specific handoff is now source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_if_available_cancel_pending_ownership_after_restore_catch2.cpp:251`.
It is indexed as `tso-regional-attribute-update-timed-if-available-cancel-pending-ownership-state`
with 130 HLA_EVOKED assertions, 41 Requirements-Lab anchors, 22 canonical 2025
sections, and 29 selected official C++ API surfaces. The first constrained
recipient queues an If Available acquisition after restore, retaining only
private Willing-to-Acquire state with no owner-release callback; its
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` resignation suppresses stale requester
callback/reflection work while the surviving recipient receives one saved
reflection before its grant. Query it with
`python tools/query_rti_work.py focus tso-regional-attribute-update-timed-if-available-cancel-pending-ownership-state --summary --compact`;
the negotiated regular-candidate continuation is now source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_catch2.cpp:307`.
It is indexed as `tso-regional-attribute-update-timed-negotiated-regular-candidate-state`
with 163 HLA_EVOKED assertions, 46 Requirements-Lab anchors, 24 canonical 2025
sections, and 30 selected official C++ API surfaces. The first constrained
recipient and independent clock queue regular candidates, the owner enters
negotiated divestiture, the first requester resigns with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, and reissuing negotiation selects the
retained regular candidate; its confirmation and acquisition notification occur
only after the surviving regional reflection crosses its grant boundary. Query
it with
`python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-regular-candidate-state --summary --compact`;
the mixed If Available-to-regular continuation is now source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_if_available_regular_candidate_continuation_after_restore_catch2.cpp:307`.
It is independently indexed as
`tso-regional-attribute-update-timed-negotiated-if-available-regular-candidate-continuation-state`
with 163 HLA_EVOKED assertions, 48 Requirements-Lab anchors, 25 canonical 2025
sections, and 31 selected official C++ API surfaces. It keeps the first
constrained recipient on the If Available path, retains the independent clock's
regular candidate through requester cancellation, and completes the reissued
negotiation only after the surviving saved reflection. Query it with
`python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-if-available-regular-candidate-continuation-state --summary --compact`;
the next bounded action-specific handoff remains the subsequent indexed queue
entry.
in-transit FQR, regional directed/default-region forms outside these bounded
cases, remaining timestamped default-region/re-enable matrix coverage beyond
the new class-level regional timestamped provider-response composition,
the new bounded Time Constrained transition,
save/restore,
transport, and the complete package/JUnit/protected-review evidence path
remain open.
The positive non-regional timestamped Delete Object Instance companion now
drives one removal through FQR, TARA, and NMRA as well: each timestamped
Remove Object Instance callback precedes its own grant, FQR preserves actual
and optimistic time, and the producer TAR completes independently. This is
still a focused development-profile lane, not deletion-family completion. A
separate two-recipient TAR/NMR companion now drives the same timestamp-7
removal through direct TAR(7) and NMR(10), proving both callbacks precede
their grant at 7 with the producer completing TAR(2). The remaining alternate
advance, fanout, DDM, recovery, and transport matrix remains open. A
separately contracted ordinary lifecycle companion now carries one timestamped
removal across a Time Constrained disable and callback-gated re-enable, proving
one removal callback before the matching grant with the original metadata and
terminal retraction classification. Broader re-enable, save/restore,
ownership/resignation, transport, package/JUnit/protected-review, and
conformance remain open. A separately contracted save/restore companion also
saves a live queued deletion, terminalizes the designator after saving,
restores the snapshot, delivers Remove Object Instance through Flush Queue
Request, and proves post-delivery Request Retraction reconstitutes the
object/name. Its timed-save-boundary companion now schedules a timestamp-six
deletion against a logical-time-four save, restores the live deletion ledger,
and repeats the FQR removal plus Request Retraction checks. General timed or
durable restore and changed-membership/ownership recovery remain open.
A matching positive non-regional timestamped directed-interaction companion
now drives one target-qualified payload through FQR, TARA, and NMRA. Each
Receive Directed Interaction callback precedes its own grant, FQR preserves
actual and optimistic time, the producer TAR completes at 2, and the
post-delivery Retract classification remains terminal. A direct TAR/NMR
companion now drives the same queued directed payload to two constrained
recipients, proving the inclusive direct frontier and callback-before-grant
ordering while the producer completes TAR at 2. A separately contracted
directed lifecycle companion now carries one queued payload across a Time
Constrained disable and callback-gated re-enable, proving exactly one callback
before the matching grant with the original metadata; RL-134 records the
missing Lab lifecycle relation. Directed DDM and broader alternate-advance
coverage beyond these bounded TAR/NMR/FQR/TARA/NMRA cases remain open. A separate save/restore
companion preserves a queued directed payload and live retraction record across
an untimed snapshot, delivers it through Flush Queue Request, and proves
Request Retraction from the original designator after delivery; RL-135 records
the missing cross-service relation. Timed/durable restore and broader recovery
remain open.
A matching regional save/restore companion now preserves one queued
overlap-qualified timestamped interaction, its explicit source RegionHandle
set, and its live retraction ledger across an untimed snapshot. It
terminalizes the post-save designator, restores the image, delivers through
Flush Queue Request before the grant, and verifies Request Retraction from the
original designator; RL-136 records the Requirements Lab's missing regional
cross-service relation. Timed/durable restore, region mutation,
passive/relaxed-DDM variants, and broader recovery remain open.
The ordinary non-regional timestamped interaction family now has a matching
mixed-advance companion as well: one queued payload reaches independent FQR,
TARA, and NMRA recipients, each callback precedes its grant, FQR preserves
actual/optimistic time 5, and the producer TAR completes at 4 after the
lookahead setup. A separately contracted ordinary lifecycle companion also
queues one non-regional payload across a Time Constrained disable and
callback-gated re-enable, then proves exactly one callback before the matching
grant with the original metadata and terminal retraction classification. These
close only bounded in-process frontiers; the broader available-advance,
re-enable, timed/durable save/restore, remote, package/JUnit/protected-review,
and conformance matrix remains open.
The matching voluntary-source-resignation fanout companion accepts one
timestamped interaction for two constrained recipients, then resigns the
producer with `NO_ACTION` before independent TAR(6) and NMR(10) release the
recipient-local queues. Each callback precedes its own grant and retains the
original producer, payload, tag, timestamp/order, and retraction metadata;
the departed producer's later `Retract` is rejected by the official membership
precondition. This is clean local coverage, not a complete resignation,
alternate-advance, recovery, transport, package, or conformance claim.
The adjacent timestamped attribute-update fanout companion accepts one
timestamped passel for two constrained recipients, then applies
`UNCONDITIONALLY_DIVEST_ATTRIBUTES` before an independent regulator releases
direct TAR(7) and NMR(10) one recipient at a time. Both reflections precede
their grants and retain the departed producer, payload, tag, timestamp/order,
and retraction metadata. This remains development-profile evidence for the
recipient ledger, not a complete ownership/resignation, alternate-advance,
recovery, DDM, transport, package, or conformance claim.
A focused three-member timestamped object-attribute ordering companion now
submits timestamp 7 before a timestamp-5 update and a second timestamp-5
cohort member, then advances two independent constrained recipients at 5 and
7. Different timestamps are delivered in timestamp order, both equal-timestamp
records precede the timestamp-7 record, and the equal-timestamp tie-break is
left unspecified. Transport-arrival, cross-process, alternate-advance,
ownership, save/restore, package, and conformance variants remain open.
Its adjacent alternate-advance companion drives independent recipients through
exact TARA and NMRA boundaries for the same cohort, preserving the
callback-before-grant boundary while leaving equal-timestamp tie-breaking
unspecified. Other advance combinations, transport-arrival, cross-process,
ownership, save/restore, package, and conformance variants remain open.
A four-member cross-producer timestamped-interaction companion now occupies the
same focused ordering lane. Two distinct publishers submit an equal-timestamp
cohort and a later message; each constrained recipient observes the complete
cohort before the later timestamp, with callback-before-grant ordering and no
invented equal-timestamp tie-break. The existing §8 requirements mapping is
reused; transport-arrival, cross-process, alternate-advance, save/restore,
package, and conformance variants remain open.
The four-member mixed regional attribute-update companion now occupies that
lane as well. One explicit-source regional update is admitted before FQR, TARA,
and NMRA; each recipient's untouched queue is asserted before its own callback
is evoked, preserving recipient-local callback-before-grant ordering. Its
existing §6/§8/§9 requirements mapping is retained; broader regional,
transport-arrival, cross-process, and conformance variants remain open.
The default-source mixed-fanout companion now extends the same recipient-local
isolation check to a retraction boundary. An ordinary timestamped update is
delivered immediately to one regional subscriber while the constrained
subscriber keeps its passel pending; Retract produces only the immediate
recipient's Request Retraction, and the constrained recipient later observes
only its grant. This is an implementation regression assertion, not a
cross-service ordering claim. Its existing §6/§8/§9 mapping is retained;
passive declarations, alternate advances, transport-arrival, save/restore,
package, and conformance variants remain open.
The paired default-source mixed-fanout interaction companion now carries the
same bounded assertion through the interaction dispatch path. The immediate
regional subscriber receives the timestamped interaction and its Request
Retraction while the constrained subscriber's pending queue remains quiet;
after the producer advances, that recipient observes only its grant. This is
also implementation regression coverage rather than a cross-service ordering
claim, with its existing §6.13/§8/§9 mapping retained. Directed, passive,
alternate-advance, transport-arrival, save/restore, package, and conformance
variants remain open.
The four-member mixed-advance interaction companion now closes the adjacent
recipient-isolation edge across FQR, TARA, and NMRA. It drains those recipients
one at a time and asserts that an untouched recipient's callback ledger remains
empty until its own evoke, while retaining the existing callback-before-grant
and supplied-empty default-region metadata checks. This remains implementation
regression coverage rather than a cross-service ordering claim; its existing
§6.13/§8/§9 mapping is retained. Passive, relaxed-DDM, alternate-advance,
transport-arrival, save/restore, package, and conformance variants remain open.
The matching non-regional mixed-advance interaction companion now applies the
same queue-isolation check without DDM state: FQR, TARA, and NMRA are drained
independently, and each untouched callback ledger stays empty until its own
evoke while the existing callback-before-grant metadata remains asserted. This
is implementation regression coverage, not a cross-service ordering claim; its
existing §5.1.5/§8 mapping is retained. Re-enable, cross-producer, ownership,
membership, transport-arrival, save/restore, package, and conformance variants
remain open.
The explicit-source regional attribute mixed-fanout companion now carries the
same first-update isolation check across immediate and time-constrained
recipients. Retracting the immediate reflection leaves the constrained queue
callback-quiet until its grant, while the later regional updates retain their
existing source-region and retraction assertions. This is implementation
regression coverage, not a cross-service ordering claim; its existing
§6/§8/§9 mapping is retained. Default-region completion, alternate advances,
transport-arrival, save/restore, package, and conformance variants remain open.
The multi-recipient save/restore attribute-update companion now extends the
same isolation invariant across a restored image. After restore, draining the
first recipient produces its reflection and Flush Queue Grant while the second
recipient's callback ledger remains empty; the second restored copy is then
drained independently before the shared retraction is delivered. This is
recipient-local implementation regression coverage, not a cross-service
ordering claim; its existing §6/§8 mapping is retained. Timed/durable,
changed-membership, alternate-advance, transport, package, and conformance
variants remain open.
The four-member timed-save companion now extends recipient isolation to mixed
save-admission frontiers: two Flush Queue recipients are evoked independently
before the ordinary TAR recipient, and each untouched recipient/regulator
callback ledger remains empty until its own save-initiation and grant
boundary. This is implementation regression coverage, not a cross-service
ordering claim; its existing §4.19/§4.19.6/§4.20 mapping is retained.
Six-member all-advance, queued/in-transit, restore, transport, package, and
conformance variants remain open.
The resigned-FQR timed-save companion now covers membership churn while a save
is pending: after the constrained FQR member resigns with `NO_ACTION`, the
remaining ordinary TAR member is admitted at the save boundary, the resigned
member receives no save callbacks, and the surviving members complete the
save. This is implementation regression coverage across §4.12 and
§4.19/§4.19.6/§4.20, not a broader resignation or save-ordering claim.
The active-callback resignation companion now exercises the official guard:
Resign Federation Execution attempted from Initiate Federate Save is rejected
with `CallNotAllowedFromWithinCallback`; after the callback returns, the same
resignation cancels the in-flight save for surviving members, suppresses a
stale failure callback to the departing member, and leaves a clean follow-up
save path. This is development-profile regression evidence across §4.12,
§4.19, §4.20, and §4.23, not a timed active-callback, durable, remote, or
conformance claim. Pending-grant restore, timed active-callback variants,
transport, package/JUnit/protected-review evidence, and conformance remain
open.
The timed active-callback counterpart reaches the scheduled save timestamp
before making the same guarded resignation attempt from the constrained
Initiate Federate Save callback. The callback is rejected without changing the
timed-save state; both members then complete the save and the constrained
member resigns normally afterward. This keeps the timed boundary and callback
guard evidence in one independently runnable case, while FQR/alternate-advance,
durable, remote, package, protected-review, validation, and conformance work
remain open.
The FQR active-callback counterpart crosses the strict Flush Queue Request
boundary before making the same guarded resignation attempt. It verifies
Initiate Federate Save before the Flush Queue Grant, preserves the timed save
after the rejected call, and lets the FQR member resign normally after save
completion. Alternate-advance, durable, remote, package, protected-review,
validation, and conformance variants remain open.
The TARA active-callback counterpart crosses the strict Time Advance Request
Available boundary before making the same guarded resignation attempt. It
verifies Initiate Federate Save before the Time Advance Grant, preserves the
timed save after the rejected call, and lets the TARA member resign normally
after save completion. Alternate NMRA/FQR active-callback, durable, remote,
package, protected-review, validation, and conformance variants remain open.
The NMRA active-callback counterpart crosses the strict Next Message Request
Available boundary before making the same guarded resignation attempt. It
verifies Initiate Federate Save before the Time Advance Grant, preserves the
timed save after the rejected call, and lets the NMRA member resign normally
after save completion. Alternate FQR active-callback, durable, remote,
package, protected-review, validation, and conformance variants remain open.
The six-member all-advance companion now extends that isolation check across
TAR, NMR, TARA, NMRA, and FQR: each constrained recipient is evoked in
sequence while every later recipient and the regulator retain an empty
callback ledger until their own boundary. This remains implementation
regression coverage, not a cross-service ordering claim; its existing
§4.19/§4.19.6/§4.20 mapping is retained. Queued/in-transit TSO combinations,
role/resignation churn, restore, transport, package, and conformance variants
remain open.
The two-member in-transit TSO companion now records the reentrant save
boundary: the regulator retains only its earlier grant while the recipient
callback is active, and receives the deferred save initiation only after that
interaction returns. This is implementation regression coverage, not a
cross-service ordering claim; its existing §4.19/§4.19.6/§4.20 mapping is
retained. Multi-member/multi-mode in-transit, role/resignation, restore,
transport, package, and conformance variants remain open.
The two-member exclusive-FQR companion now records the equal-time boundary
explicitly: the timestamped interaction precedes the equal FQG, the save
initiation remains pending, and a later strict FQG carries the
save-initiation/later-grant sequence while the regulator's earlier grant
remains its only callback until it is evoked. This is implementation
regression coverage, not a cross-service ordering claim; its existing
§4.19/§4.19.6/§4.20 mapping is retained. Role/resignation, restore, transport,
package, and conformance variants remain open.
The three-member ordinary-TAR-before-FQR companion now records the
complementary admission edge: TAR save initiation leaves the pending FQR
recipient's callback ledger empty, and the later FQR does not alter the
completed TAR recipient's callback sequence. This is implementation
regression coverage, not a cross-service ordering claim; its existing
§4.19/§4.19.6/§4.20 mapping is retained. Role/resignation, restore, transport,
package, and conformance variants remain open.
The single-constrained-member pending-request replacement companion now
records the TSO/save boundary: replacing the first timed-save request produces
no premature callback, the queued interaction is delivered before direct save
initiation, and the regulator's ledger contains only its earlier grant until
its own save initiation is evoked. This is implementation regression coverage,
not a cross-service
ordering claim; its existing §4.19/§4.19.6/§4.20 mapping is retained. Invalid
requests, durable serialization, restore/interlocks, transport, package, and
conformance variants remain open.
The three-member constrained-admission companion now records recipient
isolation at the TAR boundary: each constrained federate receives direct save
initiation before its own grant, while the non-constrained regulator's ledger
remains at its earlier grant until the second constrained member is admitted
and the regulator is evoked. These are implementation regression assertions,
not a cross-service ordering claim; the existing §4.19/§4.19.6/§4.20 mapping is
retained. Mixed-mode, role/resignation churn, pending-grant save/restore,
transport, package, and conformance variants remain open.
The two-member TARA boundary companion now records the available-mode ledger
edge: an equal-time next grant leaves both the save request and the regulator
callback queue pending, while only a strictly greater next grant admits direct
save initiation before the recipient's grant. This is implementation
regression coverage, not a cross-service ordering claim; its existing
§4.19/§4.19.6/§4.20 mapping is retained. Role, resignation, restore,
transport, package, and conformance variants remain open.
The two-member NMR/NMRA companion now records both next-message boundaries:
the inclusive NMR timestamp starts direct save initiation before its grant,
while equal-time NMRA remains pending until a strictly later request is
available. Recipient and regulator callback ledgers are checked at each
boundary as implementation regression coverage, not as a cross-service
ordering claim; the existing §4.19/§4.19.6/§4.20 mapping is retained.
Queued/in-transit TSO combinations, role/resignation, restore, transport,
package, and conformance variants remain open.
The three-member Available/NMRA companion now records dual strict-later
admission: each constrained recipient receives its own save initiation and
grant without leaking callbacks into the other recipient or the regulator,
which remains un-instructed until both are admitted. These recipient-ledger
checks are implementation regression coverage, not a cross-service ordering
claim; the existing §4.19/§4.19.6/§4.20 mapping is retained.
Queued/in-transit TSO combinations, role/resignation, restore, transport,
package, and conformance variants remain open.
The three-federate GALT/LITS companion now occupies the lane's federation-wide
time-management edge. Independent TARs on two regulators are held pending,
one regulator is granted without draining the other's callback queue, and the
minimum regulator's resignation updates the observer's GALT/LITS snapshot.
Its existing §4.12/§8.1.5/§8.2/§8.3.1/§8.5.5/§8.6.3/§8.8.3/§8.18.1/§8.19.3
mapping is retained; TSO inputs, transport ordering, and alternate-advance
variants remain open.
The mixed ownership-disposition companion now occupies the same lane. An
earlier If Available request is transferred first, while a later regular
request remains isolated in its own queue until the selected owner denies the
release; only then does the later requester receive its terminal Unavailable
callback. Its existing §7.1.2.1/§7.1.4/§7.7.3/§7.8/§7.11/§7.12/§7.13
mapping is retained; negotiated, resign-action, save/restore, transport, and
conformance variants remain open.
The negotiated Willing-to-Acquire continuation now extends that lane to three
members: cancelling the first candidate's superseding regular request leaves
the second candidate's confirmation work intact, and draining the first
candidate cannot consume the second candidate's terminal notification. Its
existing §7.3.4/§7.5.3/§7.6.3/§7.7.3/§7.15 mapping is retained; broader owner
search and negotiated-acquisition behavior remain open.
The mixed directive-4 resignation companion now extends the same lane to a
terminal delete/divest transition. One owner resigns with
`DELETE_OBJECTS_THEN_DIVEST`; the peer receives exactly one assumption for the
retained object and one removal for the delete-privileged object. The embedded
ambassador currently drains the recipient-local assumption callback before the
removal callback; that sequence is recorded as an implementation regression,
not a cross-service ordering claim. Its three selected Requirements Lab IDs
resolve to §4.12; RL-086 still records that the Lab exposes `ResignAction` as
one aggregate candidate without directive-specific facets.
The larger unconditional-divestiture companion now joins the lane's stale-work
edge. It unpublishes a queued candidate before callback delivery, proves that
the stale assumption is suppressed, and then proves one fresh grouped offer
after republishing with the original tag. Its selected mapping covers
§7.1.1/§7.2/§7.4; the remaining arbitration and resignation forms stay open.
The later-publication resignation companion now joins that lane as well. It
keeps a known-but-unpublished candidate out of the initial offer set, then
proves that a later publication receives exactly one continuation offer after
the owner has resigned. Its selected mapping resolves to §4.12 and §4.12.4;
the later join/discovery variant and broader declaration transitions remain
open.
The later-join/discovery companion now closes the adjacent eligibility edge. A
new member discovers the retained unowned object before publishing, receives no
premature offer, and then receives exactly one assumption after publication.
Its selected mapping also resolves to §4.12 and §4.12.4; terminal callback
re-search and other declaration transitions remain open.
The automatic `DELETE_OBJECTS_THEN_DIVEST` Connection Lost companion now joins
the lane's forced-resignation edge. It retains one transferred attribute,
removes a separate delete-privileged object, and asserts the survivor's current
recipient-local assumption-before-removal callback sequence. Its selected
mapping covers §4.1.1, §4.4, §4.10.44, and §4.10.45.3; the sequence remains
implementation regression coverage rather than a cross-service ordering claim.
The combined `CANCEL_THEN_DELETE_THEN_DIVEST` Connection Lost companion now
closes the adjacent forced-cleanup edge. It suppresses a stale owner-release
request, removes a delete-privileged object, re-offers the retained attribute,
and records the same recipient-local assumption-before-removal sequence. Its
selected mapping adds §7.2/§7.4 ownership-assumption clauses to the Connection
Lost and support-switch clauses; the sequence remains implementation-only.
The ownership-disposition lane now also proves that one `Release Denied`
invocation queues one terminal `Unavailable` callback per regular acquirer.
Draining the first requester leaves the second requester's callback untouched
until its own queue is evoked. This extends the existing §7.8/§7.12 mapping without
claiming complete acquisition arbitration, resign disposition, or remote
transport behavior.
The explicit-source regional-interaction companion then exposed a distinct
voluntary-resignation defect: the live source region was released before the
queued callback's overlap check, suppressing an otherwise accepted TSO
passel. The runtime now stores the committed invocation-time region snapshot
with the queued interaction and uses it at the callback boundary; the focused
case passes 60 assertions and receives once before TAR(6). RL-174 records the
consumer regression and mitigation. This does not close the remaining
regional resignation, alternate-advance, recovery, transport, or conformance
       matrix.
The matching explicit-source regional object-attribute case now covers the
adjacent payload recurrence: an accepted timestamp-7 passel remains queued while
the producer resigns with `UNCONDITIONALLY_DIVEST_ATTRIBUTES`, and an independent
regulator releases the recipient. The runtime captures the committed
invocation-time `RegionSpecificationSnapshot` and explicit update-region
association at admission, so the reflection still precedes TAR(7) with its
source RegionHandle, producer, payload, tag, timestamp/order, and retraction
metadata. RL-175 records the recurrence and cites RL-174; this is bounded
development-profile evidence, not a complete regional resignation matrix.
The ordinary regional object-update path now retains the committed source
region realization in each accepted receive-order passel. Its HLA_EVOKED
regression mutates that source range before callback dispatch and verifies the
original reflection remains deliverable; the shared evaluator applies the same
snapshot to TSO callbacks, including the direct HLA_EVOKED callback used for a
non-time-constrained recipient, while continuing to recheck live associations.
This closes one source-range-mutation boundary, not the broader save/restore,
passive, alternate-advance, remote-transport, or conformance matrices.
The timestamped regional attribute-update admission now carries those
invocation-time snapshots from each accepted passel into the federation-owned
TSO message. Queue admission validates snapshot coverage and committed bounds
without rereading the mutable live region map, so a source-range change or
region release cannot give immediate and queued recipients different DDM
realizations for one service call. A registry-focused Catch2 regression also
round-trips a captured snapshot for an opaque source handle through the TSO
delivery boundary; the Requirements Lab remains frozen input for this native
implementation change.
It emits raw JUnit only for the real embedded Disconnect catalog entry. The
next object-management gates are remaining timestamped default-region/re-enable
matrix coverage beyond the bounded FQR/TARA/NMRA and changed-lookahead object
cases and the now bounded interaction, default-source object, and
explicit-source regional object Time Constrained transitions, the
remaining regional response forms, remaining timestamped/retraction behavior,
remaining regular/negotiated acquisition and remaining divestiture flows, RTI-owned state, and full resign ownership
disposition, followed by the remaining
local, timestamped, and ownership-disposition lifecycle design, DDM-region and
FOM sharing-policy rules, then package/transport design before any broader
service claim.

### Current indexed slice

> **Live-pointer rule (2026-09-11):** the prose in this section records the
> history of completed slices and is not a work queue. Do not select a “next”
> case from an older paragraph below. The authoritative current pointer is
> always the bounded query output from `ready`, `work`, `queue`, and `focus`;
> use `plan <heading-query>` only to locate this section without loading its
> prose. The latest completed implementation slice is the two-receiver
> process TSO fan-out lane at
> `cpp/tests/ieee1516_2025_connection_catch2.cpp:24629`; query
> `umbra-cpp-process-tso-interaction-fanout-integration` for its exact source,
> tests, and requirement pairs (133 assertions; 20 Lab requirements; 13
> clauses; 15 official C++ API surfaces). The preceding same-timestamp
> multiple-message FIFO lane remains independently queryable at
> `cpp/tests/ieee1516_2025_connection_catch2.cpp:24182`. The preceding
> time-regulation re-enable and attribute-retraction lanes remain queryable by
> their exact plan ids. The preceding
> multiple-record Flush Queue, GALT-frontier, and
> single-endpoint
> FQR, handle-normalization, order lookup, and federate-identity process slices
> remain queryable through their exact plan ids.
> `umbra-cpp-process-federate-identity-lookup-integration`.
> When
> a family queue is exhausted, choose a new bounded family through the printed
> `queue --summary --compact` options. The deferred-association codec and
> filesystem-restore slices remain queryable by their exact lanes.

Use `docs/planning/ROADMAP-INDEX.json` and `tools/query_rti_work.py` as the
work queue. Start with `status --summary --compact`, then `queue --summary
--compact` and `next --pointer`; when the indexed source pointer is exhausted,
the latter prints the deterministic planned-row head with its plan id and
requirement/section/API counts instead of requiring a full-plan search. The
process-boundary source slice remains a completed package/JUnit regression
under the `transport-and-conformance` work item; it is not the active pointer.
Run `python tools/query_rti_work.py ready --summary --compact` for bounded
family choices; no active handoff is pending, so the completed re-enable row
is not presented as new work.
Use `python tools/query_rti_work.py focus process-boundary --summary --compact`
for its 109-case mapped C++/JUnit card (5,239 indexed assertions). The completed
`application-value-state` slice is intentionally
small: it adds the typed latest object-attribute value ledger, maps it to the
existing object-update/save/restore requirements, and includes a route-free
fresh-registry filesystem process-restart companion. Query it with
`python tools/query_rti_work.py lane application-value-state --compact`; do
not rescan the Requirements Lab unless its pinned export changes. The public
filesystem companion is independently queryable with `python
tools/query_rti_work.py lane public-process-restart-application-value
--compact`; it checks fresh-registry value rehydration and report-file identity
without coupling the lane to per-record service-report timing. The source-
located focused companion is independently buildable at
`cpp/tests/public_process_restart_application_value_catch2.cpp:189` and is
indexed as `public-process-restart-application-value-focused` with 82
assertions, nine Requirements-Lab anchors, five canonical 2025 sections, and
18 official C++ API surfaces. Use its exact `focus`, `trace`, `matrix`,
`check`, and CTest handles before selecting another save/restore lane.
The public process time lane also has a separate grant-boundary Modify
Lookahead companion at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12164`, with 34 assertions,
six Requirements-Lab anchors, four canonical 2025 sections, and 14 official
C++ API surfaces. Query its exact `focus`, `trace`, `matrix`, `check`, and
CTest handles through `process-modify-lookahead-grant-focused`; it is bounded
to deferred lower-lookahead application after a shared two-federate grant.
GALT/LITS/TSO, role-disable, resignation, save/restore, and conformance remain
separate lanes.
The next completed process temporal slice is the Available-form request at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12396`, with 15 `HLA_EVOKED`
assertions, four Requirements-Lab anchors, three canonical 2025 sections, and
nine official C++ API surfaces. Query it through
`process-time-advance-available-focused`; it proves the dedicated TARA
operation, official logical-time encoding, callback-gated grant, and Query
Logical Time readback. Inclusive GALT, queued TSO, FQR,
multi-federate coordination, save/restore, and conformance remain separate.
Remaining
pending application request ledgers are being sliced by exact lane; the
temporal generation ledger is now covered by
`python tools/query_rti_work.py lane pending-application-request-state
--compact`. The temporal role-enable slice now also rebinds pending Enable
Time Regulation/Enable Time Constrained callbacks through live ambassador
factories after restore and fences stale pre-restore callback epochs. Its
public HLA_EVOKED/HLA_IMMEDIATE regulation and constrained-role companions
are green. A focused filesystem process-restart case now materializes the
route-free control/temporal image in a fresh embedded registry and rebinds the
pending role callback through the new live factory. The same fresh-registry
       path now rehydrates one object-name reservation, one synchronization-point
       ledger, one committed federation-owned region's dimension set and range
       bounds, and one object-class declaration ledger covering publication,
       active subscription, update-rate, and class defaults. A separate private
       fresh-registry companion now rehydrates a pending ordinary TAR and
       deferred decreasing Modify Lookahead, applying the deferred target at
       grant time. The public HLA_EVOKED/HLA_IMMEDIATE fresh-registry
       filesystem companion for that image is green too: it inspects the
       durable state image, rebinds the pending TAR through public callback
       models, and applies the deferred lookahead target after restore. The
       first fresh-registry TSO payload companion is now green: one ordinary
       timestamped interaction payload survives the filesystem boundary for
       two live recipients with independent in-transit and queued phases,
       including durable bytes, tag, transport, and timestamp. A matching
       directed-interaction companion now restores target projections and fresh
       callback routes across the same queued/in-transit split. The timestamped
       attribute-update companion is green as well: one passel-bearing payload
       survives the filesystem boundary with independent queued/in-transit
       recipients, preserving bytes, passel handles, order, transport, tag,
       and timestamp. The timestamped object-deletion companion is green too:
       one filesystem-backed payload survives a fresh-registry boundary with
       one recipient in transit and another queued, preserving its
       invocation-time object/value/known-class snapshot, retraction ledger,
       tag, timestamp, and live callback routes. The regional attribute-update
       companion is green too: its filesystem image preserves the
       invocation-time source-region dimensions/ranges in both the message and
       recipient passel across a fresh-registry restore. A follow-on variant
       now mutates the committed source region, resigns its producer, restores
       the region in a fresh registry, mutates it again, and proves that the
       invocation snapshot remains authoritative. Query the next bounded slice
       with `python tools/query_rti_work.py next --compact`; the public live
       explicit-source regional restore variant is now green as well, including
       producer resignation before Flush Queue delivery. The next slice is the
       timed live explicit-source regional restore variant, which is now green
       with the same independent-regulator resignation boundary. The timed
       multi-recipient explicit-source regional restore variant is now green
       through source mutation, restore, disjoint mutation, and producer
       resignation for both recipients. Its action matrix now covers
       `UNCONDITIONALLY_DIVEST_ATTRIBUTES`, `DELETE_OBJECTS_THEN_DIVEST`,
       `CANCEL_THEN_DELETE_THEN_DIVEST`, live regular, If Available, and
       negotiated acquisition cancellation, and `NO_ACTION`; the matrix is
       queryable with
        `python tools/query_rti_work.py lane
        tso-regional-attribute-update-timed-resignation-matrix --summary`.
        The two-candidate continuation is independently queryable with
        `python tools/query_rti_work.py lane
        tso-regional-attribute-update-timed-negotiated-continuation-state
        --compact`. The next slice is a negotiated owner-confirmation
        cancellation boundary after retained-candidate selection at that same
        saved timed boundary, now covered by the focused cancellation lane;
        query it with
        `python tools/query_rti_work.py lane
        tso-regional-attribute-update-timed-negotiated-confirmation-cancel-state
        --compact`. The mixed regular/If Available two-candidate continuation
        is now green at the same saved timed boundary; query it with
        `python tools/query_rti_work.py lane
        tso-regional-attribute-update-timed-negotiated-mixed-candidate-state
        --compact`. The corresponding mixed retained owner-confirmation
        cancellation case is now green too; its pre-delivery companion is
        green as well and is queryable with
        `python tools/query_rti_work.py lane
        tso-regional-attribute-update-timed-negotiated-mixed-pre-delivery-cancel-state
        --compact`. The retained regular-candidate pre-delivery and
        after-delivery cancellation companions are now green and indexed as
        their own focused lanes. The next slice is a regular-to-regular
        two-candidate continuation, now green alongside the opposite
        If Available-to-regular ordering. The regular-to-regular retained
        owner-confirmation cancellation pair is now green at both callback
        timing boundaries and remains independently queryable. Keep the
        17-case matrix as a regression gate. Fresh-registry process-restart
        evidence now covers one pending regular owner-release reservation, one
        pending If Available requester callback, negotiated regular and
        negotiated If Available owner-confirmation callbacks, and a mixed
        regular/If Available negotiated ledger, each rebuilt against live
        routes after the Federation Restored boundary. A delivered negotiated
          confirmation is also preserved without replay and can complete through
          `Confirm Divestiture` after restore. The mixed regular/If Available
          delivered-confirmation image is now green under both callback models
          (273 assertions): both delivered markers are preserved in a fresh
          registry, neither callback is replayed, and one grouped `Confirm
          Divestiture` transfers both attributes. Query the bounded slice with
          `python tools/query_rti_work.py lane
          public-process-restart-mixed-confirmation-delivered --compact` or its
          exact test selector; the asymmetric mixed confirmation boundary is
          now green under both callback models (274 assertions), and the next
          bounded ownership ledger is the reverse asymmetric mixed confirmation
          boundary (one delivered candidate and one pending candidate), followed
          by the remaining lifecycle coverage. The first
        asymmetric form is now green under both callback models (274
        assertions): a delivered regular confirmation is preserved while
        exactly one If Available confirmation is rebound after restore. The
        reverse asymmetric form is green under both callback models (274
        assertions) as well: a delivered If Available confirmation is preserved
        while exactly one regular confirmation is rebound after restore. The
         the pending attribute-transportation-type-change and pending interaction-
          transportation-type-change companions are green under both callback
          models (220 and 196 assertions), and all three pending Query Attribute
           Ownership companions are green under both callback models (136, 150,
           and 142 assertions). The active next pointer is the source-missing
           single-recipient directed TSO restore case; use `python
           tools/query_rti_work.py next --summary` and its exact test query.
           Malformed
        mixed-confirmation
        images are now rejected deterministically for missing candidates,
        mismatched candidate identity, and stale delivered/queued flags. A
        pending Divestiture If Wanted notification is now restored into a fresh
        registry and consumed exactly once against the live requester route. A
        pending Confirm Divestiture notification is likewise restored and
        consumed exactly once. The ownership-acquisition cancellation
        reservation case is now also green: a fresh registry rebinds the
        requester route, confirms the cancellation once, and releases the
        requester publication without replay. A pending attribute
        transportation-type change is now also restored into a fresh registry,
        rebound to the requester route, and committed exactly once at the
        confirmation boundary. A pending interaction transportation-type change
        is now likewise restored with its publication declaration, rebound to
        the live publisher route, and committed exactly once at confirmation.
        A standalone published interaction declaration is also restored into a
        fresh registry and remains visible without inventing callback or
        transportation state. A standalone interaction subscription is also
        restored, as is a same-class publication plus subscription image and a
        committed regional interaction subscription with its owned region
        rehydrated before the declaration. Two independent publication entries
        across joined federates are restored without cross-federate leakage,
        and a committed interaction transportation-type override survives
        restore without replaying a pending callback. The mixed multi-federate
        publication/subscription image is now green, including the
        publisher-scoped committed override form. A directed object-class
        publication/subscription pair now also restores across two federates
        into a fresh registry and survives a second durable round trip. A known
        target-object companion now restores its visibility, application value,
        directed declarations, and receive-order route before a second durable
        round trip. The by-ownership companion now restores one target with two
        ownership selectors, transfers both target attributes through regular
        acquisition and Divestiture If Wanted, and follows the route to the new
        owner before a second durable save. The directed TSO callback-boundary
         companion now also survives filesystem save and fresh-registry restore,
         transfers both target-owned attributes before the old owner's callback,
         suppresses the stale by-ownership callback, and completes a legal
         Retract without Request Retraction. The positive by-ownership
         timestamped delivery/retraction companion now also survives filesystem
         save and fresh-registry restore, delivers to the eligible owner, and
         emits one Request Retraction on a legal producer Retract. The
         multi-recipient fan-out companion now saves two recipient entries,
         restores them in a fresh registry, delivers the still-eligible
         recipient, suppresses the unsubscribed recipient, and emits exactly
          one Request Retraction for the delivered recipient. The parameterized
          directed TSO companion now preserves a non-empty parameter projection
          together with HLAreliable transport and timestamped order metadata
           through filesystem save and fresh-registry restore. The latest bounded
           target is now green as the ordinary timestamped Send Interaction With
           Regions DDM projection, using the same recipient-specific restore
           boundary. The public ambassador companion is green as well: a
           timestamped passel beyond a timed save survives a disjoint source
           mutation and restore while the configured filesystem service-report
           pathname remains stable. The public-facade fresh-registry companion
           is green too: a three-member/two-recipient filesystem image restores
           through a new registry and callback routes, including the publisher
           TIMESTAMP order override. The public-facade regional attribute-update
           companion is green as a separate bounded slice: one timestamped
           source-region passel restores to two live recipients through a fresh
           registry, retains the invocation-time region snapshot after a
           disjoint mutation, and keeps joined-federate report-file identities
           distinct. The public-facade fresh-registry timestamped
           object-deletion route-rebinding case is green as a separate public
           slice: its saved object-value and deletion reconstitution snapshot
           reaches both fresh callback routes with stable report-file
           identities. The companion public retraction boundary is green too:
           the saved MessageRetractionHandle survives a fresh registry, Flush
           Queue delivers only one deletion recipient, and Retract issues one
           recipient-local Request Retraction while suppressing the queued
           copy. The directed public-facade fan-out/retraction companion is
           green too: a fresh registry restores two explicit directed
           subscribers plus one delayed-subscription route-only member, retains
           the publisher's TIMESTAMP order, delivers only the eligible
           subscriber, and emits one recipient-local Request Retraction while
           suppressing the other routes. The public directed-parameter
           companion is green too: a dedicated one-parameter 2025 fixture
           preserves non-empty payload bytes, HLAreliable transport,
           timestamped order metadata, and the configured filesystem
           report-file identity through a fresh-registry restore. The public
           regular ownership-acquisition companion is green as well: a fresh
           registry rebinds the pending owner-side release callback with its
           object/attribute set and acquisition tag while preserving distinct
           filesystem report identities. The public If Available ownership-
           acquisition companion is green too: a fresh registry rebinds the
           pending requester-side unavailable callback with the same object/
           attribute set and acquisition tag while preserving distinct
           filesystem report identities. The public negotiated owner-
           confirmation companion is green too: a fresh registry rebinds the
           owner-side Request Divestiture Confirmation callback with its
           object/attribute set and acquisition tag while preserving the
           negotiated ledger and distinct filesystem report identities. The
           public negotiated If Available owner-confirmation companion is green
           too: a fresh registry rebinds the owner-side confirmation callback,
           suppresses the duplicate requester WTA callback, and preserves the
           negotiated ledger and distinct filesystem report identities. The
           public mixed negotiated-ownership companion is green as well: a fresh
           registry restores regular and If Available pending acquisitions
           together, rebinds both owner-side confirmation callbacks, completes
           one grouped Confirm Divestiture transfer, and preserves both
           immutable filesystem report identities. The public delivered
           negotiated owner-confirmation companion is green too: it delivers
           the owner-side confirmation before save, preserves the delivered
           marker through a fresh registry without replaying the callback,
           completes the retained transfer through Confirm Divestiture, and
           preserves distinct report-file identities. The public delivered
           negotiated If Available owner-confirmation companion is green too:
           it delivers the owner-side confirmation while leaving the requester
           WTA route pending, preserves the delivered marker through a fresh
           registry without replaying either callback, completes the retained
           transfer through Confirm Divestiture, and preserves distinct
           report-file identities. The mixed delivered negotiated-confirmation
           companion is green too: it delivers both
           owner-side confirmations before save, preserves both delivered
           markers through a fresh registry without replaying either callback,
           completes one grouped Confirm Divestiture transfer, and preserves
           distinct report-file identities. The public asymmetric mixed
           negotiated-confirmation companion is green too: it preserves the
           delivered regular confirmation without replay and rebinds exactly
           one pending If Available confirmation after a fresh registry, then
           completes one grouped Confirm Divestiture transfer with distinct
           report-file identities. The public reverse asymmetric mixed
           negotiated-confirmation companion is green too: it preserves the
           delivered If Available confirmation without replay and rebinds
           exactly one pending regular confirmation after a fresh registry,
           then completes one grouped Confirm Divestiture transfer with
           distinct report-file identities. The public pending
           attribute-transportation-type-change companion is green too (220
           assertions under both callback models): it restores one pending
           confirmation and durable application value into a fresh registry,
           commits HLAbestEffort at the callback boundary, and preserves
           distinct report-file identities. Keep the six public
           fresh-registry TSO gates and all ten ownership companions together.
            Malformed mixed-confirmation rejection, Divestiture-If-Wanted
            notification, Confirm Divestiture notification, ownership-cancellation,
           and attribute-transportation-type-change restore are green. The
           public pending interaction-transportation-type-change companion is
           green too (196 assertions under both callback models): it restores a
           two-member publication/subscription image, rebinds one publisher
           confirmation, applies HLAbestEffort after the callback, and preserves
           distinct report-file identities. The public
           committed interaction-transportation-type override companion is
           green as well: it restores the publisher-scoped override without
           replaying a confirmation callback, proves the effective query and
           delivery, and preserves distinct report-file identities. The public
           mixed interaction-override companion is green too: it restores the
           independent subscriber declaration, proves the publisher override
           through public query and subscriber delivery, and preserves distinct
           report-file identities. The private and public directed interaction
           declaration restore lanes are green, including a fresh-registry
           round-trip of the directed publication/subscription ledgers. The
           private and public directed target-routing restore lanes are green,
           including receive-order delivery through the restored target. The
           private and public directed ownership-handoff restore lanes are
           green too, including complete ownership transfer and post-handoff
           by-ownership receive-order routing. The private and public directed
           timestamped ownership-callback lanes are green: the public
           switch-support case persists both candidate routes, suppresses the
           stale old-owner callback after handoff, and reaches the strict
           Retract boundary without emitting Request Retraction. The private
           and public eligible directed timestamped ownership-delivery/retraction
           lanes are green: the public fresh-registry case restores one eligible
           by-ownership recipient, delivers it with timestamp/order/retraction
           metadata intact, and emits exactly one Request Retraction. The public
           post-delivery-resignation HLA_EVOKED baseline is green too: its durable image
           retains the departed peer's immutable payload projection and
           delivered retraction state, fresh restore skips the dead route,
           rebinds the surviving ownership-qualified route, and emits one
           recipient-local Request Retraction. The native
           mixed default-region FQR/TARA/NMRA lane now drains each ambassador's
           callback queue explicitly, and its companion proves pre-grant
           retraction for TARA and NMRA. The directed fan-out and alternate-
           advance anchors are green. The indexed disk-backed fresh-registry
           object-instance Request Attribute Value Update slice is green at
           both the native registry and public HLA_EVOKED/HLA_IMMEDIATE
           callback boundaries; query it with
           `python tools/query_rti_work.py lane
           public-process-restart-pending-attribute-value-update --compact` or
           the exact public test query. The class-designator ledger is green at
           both the native registry and public HLA_EVOKED/HLA_IMMEDIATE callback
           boundaries; query it with
           `python tools/query_rti_work.py lane
           process-restart-class-pending-attribute-value-update --compact`.
           The regional class-designator ledger is green at both boundaries as
           well, preserving per-attribute request regions through a filesystem
           fresh-registry restore; query it with
           `python tools/query_rti_work.py lane
           process-restart-regional-pending-attribute-value-update --compact`
           or its public companion. The native and public callback-entry
           negative matrices are now explicit for stale/disjoint region,
           mismatched identity, duplicate begin, requester post-resignation,
           and provider post-resignation suppression under
           HLA_EVOKED/HLA_IMMEDIATE. The provider response/retraction boundary
           for an accepted regional request is green for the timestamped form:
           the response is saved
           before delivery, restored into a fresh registry, and retains its
           value, tag, source region, order, time, and terminal retraction
           classification. The receive-order companion is also green: after
           delivery, the current value is retained without a TSO payload or
           retraction entry, restore does not replay the old response, and a
           new regional request still produces one fresh response. Query the
           timestamped lane with
           `python tools/query_rti_work.py lane
           durable-save-regional-pending-attribute-value-update-response-retraction
           --compact`. The receive-order boundary is queryable with
           `python tools/query_rti_work.py lane
           durable-save-regional-pending-attribute-value-update-regular-response-no-replay
           --compact`. The regional automatic-provision anchors are now green
           for HLA_EVOKED and HLA_IMMEDIATE; query the combined lane with
           `python tools/query_rti_work.py lane regional-automatic-provision
           --compact` or the immediate companion with
           `python tools/query_rti_work.py lane
           regional-automatic-provision-immediate --compact`. The public
           provider-response companion is green under both callback models;
           query it with `python tools/query_rti_work.py lane
            regional-automatic-provision-response --compact`. The timestamped
            Auto Provide response companion is green under both callback
            models; query it with `python tools/query_rti_work.py lane
            regional-automatic-provision-timestamped-response --compact`.
            The two-owner/two-attribute Auto Provide fan-out companion is now
            green under both callback models: one regional discovery produces
            one grouped provider callback per owner, the transferred attribute
            is acquired through the official ownership services, and each
            response reflects once without duplicate work. Query it with
             `python tools/query_rti_work.py lane
             regional-automatic-provision-multi-provider --compact`. The
             callback-boundary switch-mutation companion is now green under both
             callback models (224 assertions): the official HLAsetSwitches MOM interaction changes
             the federation-wide switch while two provider callbacks are queued,
             stale work is suppressed at callback entry, and re-enable permits
             one fresh in-scope discovery. Query it with `python
             tools/query_rti_work.py lane regional-automatic-provision-switch-mutation
             --compact`. The timestamped switch-mutation companion is also
             green under both callback models (206 assertions): an admitted timestamped response
             survives switch disable and arrives once at its matching grant,
             while re-enable permits one fresh timestamped discovery/response.
             Query it with `python tools/query_rti_work.py lane
             regional-automatic-provision-timestamped-switch-mutation --compact`.
             The non-timestamped switch-before-admission companion is now green
             under both callback models: changing the federation-wide switch
             from the requester discovery callback consumes queued solicitation
             without a provider callback, and re-enable permits one fresh
             in-scope discovery. Query it with `python tools/query_rti_work.py
             lane regional-automatic-provision-switch-admission-mutation
             --compact`. Query the next work handle with `python
             tools/query_rti_work.py next --summary`; the timestamped
             switch-before-admission companion is now green under both callback
             models too. Query it with `python tools/query_rti_work.py lane
             regional-automatic-provision-timestamped-switch-admission-mutation
             --compact`. The independent multi-source-region and Allow Relaxed
             DDM companions are now green under both callback models. Query
             them with `python tools/query_rti_work.py lane
             regional-automatic-provision-multi-source-region --compact` and
             `python tools/query_rti_work.py lane
              regional-automatic-provision-relaxed-ddm --compact`. The latest
               application-value/report-file companion and all three federate-
               owner, unowned, and RTI-owned pending Query Attribute Ownership
               fresh-registry baselines are green under both callback models
               (136, 150, and 142 assertions). Query the three ownership
               baselines with `python tools/query_rti_work.py lane
               public-process-restart-pending-attribute-ownership-query
               --compact`, its `-unowned` companion, and the
               `public-process-restart-pending-rti-owned-attribute-ownership-query`
               companion, or use their exact test queries. The next bounded
              implementation slice is the source-missing default-region
              multi-recipient TSO attribute-restore case; start with `python
              tools/query_rti_work.py next --summary` and its exact test query,
               then keep save/restore state boundaries separate from the
               completed regional lanes. Keep callback-model expansion separate
               from transport, Java, and conformance work. The focused
              HLA_IMMEDIATE Query Attribute Ownership matrix is now green as
              well: it dispatches one synchronous callback for each 2025
              federate-owned, unowned, and RTI-owned report kind and proves no
              queued replay. Query it with `python tools/query_rti_work.py
              test "Embedded HLA_IMMEDIATE Query Attribute Ownership
              dispatches all 2025 report kinds" --compact`. Keep future work
              on the indexed next-step path; `python tools/query_rti_work.py
               unlocated --summary` retains source-missing plan placeholders, and
               `check --compact` keeps the known source-drift
               queue visible. Keep the bounded source-state check in the normal
               workflow so future catalog drift is visible without reopening the
               Requirements Lab.
           The
           official 2025 C++ binding
           has no sendDirectedInteractionWithRegions overload, so directed DDM
           remains a separate future design boundary rather than a
           non-standard public API.

The joined-federate MOM `HLAobjectInstancesDeleted` projection is now
source-backed at
`cpp/tests/joined_federate_mom_deleted_object_count_periodic_catch2.cpp:108`
with 66 HLA_EVOKED assertions, one §11.4.1 requirement, and 12 official C++
API surfaces. It proves direct 0 → 1 → 2 accepted deletion history and
periodic 0-before/2-after snapshots; use the exact
`joined-federate-mom-deleted-object-count-periodic` focus/trace/matrix/check
handles and CTest filter.
The receiving-federate MOM `HLAobjectInstancesRemoved` projection is now
source-backed at
`cpp/tests/joined_federate_mom_removed_object_count_periodic_catch2.cpp:145`
with 78 HLA_EVOKED assertions, one §11.4.1 requirement, and 14 official C++
API surfaces. It proves the observer's committed no-time Remove Object
Instance callback ledger 0 → 1 → 2 and periodic snapshots on its own MOM
object; use the exact `joined-federate-mom-removed-object-count-periodic`
focus/trace/matrix/check handles and CTest filter. Timestamped/retraction,
remaining MOM statistics, producer mapping, and promotion evidence stay
separate.

The receiving-federate MOM `HLAobjectInstancesDiscovered` projection is now
source-backed at
`cpp/tests/joined_federate_mom_discovered_object_count_periodic_catch2.cpp:127`
with 66 HLA_EVOKED assertions, one §11.4.1 requirement, and 14 official C++
API surfaces. It proves eligible discovery callbacks 0 → 2 → 3, including
Local Delete Object Instance followed by subscription rediscovery; use the
exact `joined-federate-mom-discovered-object-count-periodic`
focus/trace/matrix/check handles and CTest filter. RTI-owned discovery and
remaining MOM/promotion variants stay separate.

The official handle-normalization support-services slice is source-backed at
`cpp/tests/handle_normalization_catch2.cpp:60` with 52 HLA_EVOKED assertions,
six Requirements-Lab anchors, six canonical 2025 sections (§§10.1.3 and
10.29–10.33), and all five normalizer API surfaces. It proves
NotConnected/FederateNotExecutionMember fences, typed invalid-designator
exceptions, equal execution-scoped coordinates through two joined ambassadors,
the seven-value `HLAserviceGroup` domain, and federate-coordinate stability
after resignation. Keep the exact `handle-normalization` focus/trace/matrix/
check handles and CTest filter; distributed DDM, save/restore, transport,
review, validation, and conformance remain separate.

The next standalone MOM request/report slice is now source-backed at
`cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp:137`
with 42 HLA_EVOKED assertions, one §11.4.1 Requirements-Lab anchor, and 12
official C++ API surfaces. It consumes the Subscribe-only
`HLArequestObjectInstancesUpdated` interaction, snapshots accepted updates by
registered object class, keeps repeated updates to one object at one distinct
count, and decodes the nested `HLAobjectClassBasedCounts` value in the
RTI-originated `HLAreportObjectInstancesUpdated` callback. Use the exact
`joined-federate-mom-object-instances-updated-report` focus/trace/matrix/check
handles and CTest filter; HLA_IMMEDIATE, transport/timestamp variants, other
MOM request/report families, review, validation, and conformance remain
separate.

The adjacent live-ownership MOM request/report slice is source-backed at
`cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp:137`
with 53 HLA_EVOKED assertions, one §11.4.1 Requirements-Lab anchor, and 12
official C++ API surfaces. It consumes the Subscribe-only
`HLArequestObjectInstancesThatCanBeDeleted` interaction, derives live
`HLAprivilegeToDeleteObject` ownership counts by registered class, proves two
classes before deletion and one remaining class after deletion, and decodes
the nested `HLAobjectClassBasedCounts` RTI report. Use the exact
`joined-federate-mom-object-instances-that-can-be-deleted-report`
focus/trace/matrix/check handles and CTest filter; HLA_IMMEDIATE,
transport/timestamp variants, other MOM request/report families, review,
validation, and conformance remain separate.

The latest standalone MOM request/report slice is source-backed at
`cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp:160`
with 51 HLA_EVOKED assertions, one §11.4.1 Requirements-Lab anchor, and 14
official C++ API surfaces. It consumes the Subscribe-only
`HLArequestObjectInstancesReflected` interaction, scopes the report to the
requesting/receiving federate's accepted application reflection ledger,
preserves one distinct count across repeated reflections of one object, and
decodes the nested `HLAobjectClassBasedCounts` value for two registered
classes. Use the exact
`joined-federate-mom-object-instances-reflected-report` focus/trace/matrix/check
handles and CTest filter. HLA_IMMEDIATE, transport/timestamp variants, other
MOM request/report families, review, validation, and conformance remain
separate.

The latest known/NULL object-information MOM request/report slice is
source-backed at
`cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp:146`
with 71 HLA_EVOKED assertions, one §11.4.1 Requirements-Lab anchor, and 13
official C++ API surfaces. It consumes the Subscribe-only
`HLArequestObjectInstanceInformation` interaction, decodes the nested
`HLAattributeHandleList`, proves the registering federate's `Efficiency` and
implicit `HLAprivilegeToDeleteObject` ownership, preserves registered/known
class values, and transitions a requester-local object to the MIM NULL shape
after Local Delete Object Instance. Use the exact
`joined-federate-mom-object-instance-information-report` focus/trace/matrix/
check handles and CTest filter. HLA_IMMEDIATE, transport variants, other MOM
request/report families, review, validation, and conformance remain separate.

### 4. Service families

Build independently testable vertical slices in this order:

1. FOM module management and declaration management.
2. Object and interaction management, including encoding/decoding boundaries.
3. Data distribution management.
4. Time management.
5. Ownership management.
6. Save/restore and then MOM services.

For each service, add requirements mapping, API-surface selection, state and
error-path tests, callback ordering tests, and integration tests before moving
to the next family.

### 5. Interoperability and release confidence

Add a remote transport only after embedded behavior is stable. Run the same
scenario suite in-process, cross-process, and, where licensing and access
allow, against independent federates. Test Windows and Linux with the supported
compiler/ABI matrix. Package headers, library, licenses, and CMake targets as a
consumable SDK.

Gate: a reviewed Requirements-Lab catalog links real Catch2 JUnit results to
selected API surfaces and requirements. This evidence supports a scoped claim;
it never substitutes for an independent conformance assessment.

## Engineering controls

- Keep public-header digests and source provenance green in CI.
- Keep every type declared by the pinned 2025 `Exception.h` linkable; the
  exception-binding audit compares the official declaration list to the binding
  definitions so a newly used standard exception cannot surface as a linker
  failure.
- Treat a change to a public declaration, ABI macro, exception mapping, or
  callback ordering as an architecture decision with compatibility review.
- Give every concurrency boundary an ownership and shutdown model before it
  carries federate traffic.
- Make local transport deterministic enough for repeatable tests, then use
  fault injection for transport loss, callback failure, and process shutdown.
- Keep requirements bundles, JUnit files, and test catalogs as reviewable
  artifacts with a clear difference between planned, raw, and verified
  evidence.

The active-maximum Attribute Relevance Advisory slice is now independently
buildable at `cpp/tests/attribute_relevance_rate_reissue_catch2.cpp:108`.
It records 108 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps seven
Requirements-Lab anchors directly to §§6.23 and 6.24, and exercises the
official `subscribeObjectClassAttributes` plus the rate-bearing Turn Updates
On/Off callback forms. The three-federate scenario proves High/Medium/Low
maximum selection, silence for lower-rate peers, Medium/High reissue, Low
refresh after removing the higher-rate peer, and the final Turn Updates Off
boundary. Use the exact `attribute-relevance-rate-reissue` focus, trace,
matrix, check, and CTest handles; regional/DDM, process transport, review,
validation, interoperability, and conformance remain separate.

The regional explicit-rate Attribute Relevance Advisory slice is now independently
buildable at `cpp/tests/regional_attribute_relevance_rate_designator_catch2.cpp:105`.
It records 100 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps eight
Requirements-Lab anchors directly to §§6.23, 6.24, and 10.37.1, and exercises
the official regional Subscribe/Register/Associate/Unassociate surfaces plus
Turn Updates On/Off callbacks. The two-federate scenario proves overlapping
source-region registration, disjoint suppression, one Off on loss of overlap,
and one High-rate re-entry after the original overlap is restored. Use the exact
`regional-attribute-relevance-rate-designator` focus, trace, matrix, check, and
CTest handles; ordinary active-maximum, process transport, review, validation,
interoperability, and conformance remain separate.

The known-class-disabled Attribute Relevance Advisory slice is now independently
buildable at `cpp/tests/attribute_relevance_known_class_disabled_subscription_catch2.cpp:97`.
It records 35 assertions under `HLA_EVOKED`, maps 18 Requirements-Lab anchors
to 13 canonical 2025 sections, and exercises 18 official C++ API surfaces. The
two-federate scenario discovers a `Server` instance through an `Employee`
subscription, then proves that a Server-only subscription still produces the
owner-directed Turn Updates On advisory when Advisories Use Known Class is
disabled, and that removing the Server declaration produces Turn Updates Off.
Use the exact `known-class-disabled` focus, trace, matrix, check, and CTest
handles; the paired enabled policy, rate, regional/DDM, process transport,
review, validation, interoperability, and conformance remain separate.

The paired known-class-enabled Attribute Relevance Advisory slice is now
independently buildable at
`cpp/tests/attribute_relevance_known_class_enabled_subscription_catch2.cpp:97`.
It records 30 assertions under `HLA_EVOKED`, maps 15 Requirements-Lab anchors
to 13 canonical 2025 sections, and exercises 17 official C++ API surfaces. The
two-federate scenario confirms the static known-class policy is enabled,
discovers a `Server` instance through an `Employee` subscription, and proves a
later Server-only subscription does not create an owner-directed advisory while
`Server` is unknown to the subscriber; removing the Employee subscription still
produces the expected Off advisory. Use the exact `known-class-enabled` focus,
trace, matrix, check, and CTest handles; disabled-policy, rate, regional/DDM,
process transport, review, validation, interoperability, and conformance remain
separate.
