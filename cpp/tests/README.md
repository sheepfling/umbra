# Native test guide

This directory contains Umbra's native C++ tests. A test name identifies the
runtime domain or standard-binding surface it exercises; the file stays beside
similar focused tests instead of being grouped by framework.

## Test layers

| Test kind | Purpose | Typical file name |
| --- | --- | --- |
| Smoke test | Compiles or exercises a narrow installed/baseline surface without Catch2. | ieee1516_2025_headers_smoke.cpp |
| Focused Catch2 test | Tests one private domain or bounded service slice. | federation_registry_catch2.cpp |
| Integration Catch2 test | Tests a standard-facing workflow across internal domains. | ieee1516_2025_federation_management_catch2.cpp |
| Contract/traceability test | CMake invokes a tool to confirm source and requirement mappings. | Registered in the root CMake file. |

The default build profile runs smoke, standards-integrity, traceability, and
package tests. Catch2 tests are available through the native-catch2 or
native-fom profiles described in [CONTRIBUTING.md](../../CONTRIBUTING.md).

## Add or change a test

1. Start with the README for the owning internal domain.
2. Add a focused area_catch2.cpp test when a behavior needs Catch2.
3. Add the source to the root CMake test target near similar tests.
4. Use a descriptive fixture from [data/](data/README.md) only when the model
   input is part of the behavior.
5. Add or update a Requirements Lab contract only for an official,
   source-traceable behavior.

Run the narrowest test while iterating, then run the baseline:

    ctest --test-dir out/cmake/catch2 -C Debug -R federation_registry --output-on-failure
    python tools/ci.py native

## Roadmap lanes

Cross-cutting roadmap slices use Catch2 tags as CTest labels. The callback
kernel and multi-federate callback-ordering slices can be run without selecting
individual cases from the full integration executable:

    ctest --test-dir .build-fom-services -C Debug -L "^callbacks$" --output-on-failure
    ctest --test-dir .build-fom-services -C Debug -L "^multi-federate-callback-ordering$" --output-on-failure

The corresponding CMake targets are `umbra_test_callbacks` and
`umbra_test_multi_federate_callback_ordering`.
Keep new cases tagged with one execution scope, one owning domain, and the
cross-cutting roadmap tag when they belong to this slice.

The ownership-management process slices are independently queryable; use the
owner-release-denied card as the current handoff and keep the owner-release,
regular, and If Available companions separate:

    python tools/query_rti_work.py case umbra-cpp-attribute-ownership-acquisition-release-denied-process-integration --summary --compact
    python tools/query_rti_work.py focus process-ownership-acquisition-release-denied --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors deliver Attribute Ownership Unavailable after a denied process acquisition" --summary --compact
    python tools/query_rti_work.py matrix process-ownership-acquisition-release-denied --summary --compact
    python tools/query_rti_work.py check --lane process-ownership-acquisition-release-denied --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors deliver Attribute Ownership Unavailable after a denied process acquisition$" --output-on-failure

The owner-release-denied process case at
`attribute_ownership_acquisition_catch2.cpp:815` records 36 assertions and
maps three exact 2025 requirements to clauses 7.8 and 7.12. It proves the
typed denial request/result, two-federate terminal handshake, callback-time
reservation consumption, and exact denial-tag propagation under `HLA_EVOKED`;
multi-acquirer/cancellation races, push behavior, divestiture, RTI-owned
state, and conformance remain separate follow-on slices. The owner-release,
regular, and If Available process cases are addressable through their exact
plan ids and lanes.

The callback-gated time-role slice is indexed as `time-role` and maps one
aggregate Catch2 case to eight exact 2025 Requirements Lab candidates. Use the
query handles below for the requirement/API/source crosswalk before opening
the large federation-management source:

    python tools/query_rti_work.py focus time-role --summary --compact --limit 8
    python tools/query_rti_work.py trace "Embedded time-role services keep enable requests callback-gated before TSO support" --summary --compact
    python tools/query_rti_work.py matrix "Embedded time-role services keep enable requests callback-gated before TSO support" --summary --compact
    ctest --test-dir <build-dir> -C Debug -L "^time-role$" --output-on-failure

The mapped case has 51 assertions under both `HLA_EVOKED` and `HLA_IMMEDIATE`
and covers Enable/Disable Time Regulation, Enable/Disable Time Constrained,
Query Lookahead, and the two completion callbacks. Timestamped delivery,
Modify Lookahead, save/restore, and conformance are separate lanes. The
aggregate source currently has known integrity errors; the query card and
Requirements Lab observations note the direct-case fallback and execution
gate until that source is repaired.

The adjacent Modify Lookahead case is also mapped and queryable as a separate
lane. It has 28 `HLA_EVOKED` assertions and covers immediate increases,
grant-boundary decreases, Query Lookahead, and lifecycle fences in clause
8.20.4:

    python tools/query_rti_work.py focus modify-lookahead --summary --compact --limit 8
    python tools/query_rti_work.py trace "Embedded Modify Lookahead applies increases immediately and decreases gradually" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Modify Lookahead applies increases immediately and decreases gradually" --summary --compact
    ctest --test-dir <build-dir> -C Debug -L "^modify-lookahead$" --output-on-failure

Its aggregate native executable shares the documented source-integrity gate;
the exact already-built case passed directly while the requirements/API
contract checks remain runnable through the label.

The public process-boundary Modify Lookahead slice is independently selectable
from the connection test. It has 17 `HLA_EVOKED` assertions, maps three exact
2025 Lab anchors to clause 8.20.4, and keeps the not-enabled fence, immediate
increase, and deferred lower-request retention separate from grant-time and
multi-federate scheduling:

    python tools/query_rti_work.py focus process-modify-lookahead-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassador modifies lookahead through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador modifies lookahead through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane process-modify-lookahead-focused --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador modifies lookahead through a configured process endpoint$" --output-on-failure

The grant-boundary companion is an independently selectable two-federate
slice. It has 34 `HLA_EVOKED` assertions and maps six exact 2025 Lab anchors
across clauses 8.5.5, 8.6.3, 8.8.3, and 8.20.4. It verifies lower-lookahead
retention before a pending grant, shared scheduler release, and application of
the lower interval at the regulating grant boundary:

    python tools/query_rti_work.py focus process-modify-lookahead-grant-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors apply a deferred lower lookahead at a configured process grant" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors apply a deferred lower lookahead at a configured process grant" --summary --compact
    python tools/query_rti_work.py check --lane process-modify-lookahead-grant-focused --summary --compact

The Available-form process temporal slice is independently selectable:

    python tools/query_rti_work.py focus process-time-advance-available-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassador requests available time advance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador requests available time advance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane process-time-advance-available-focused --summary --compact

The process NMR/NMRA temporal lane is independently selectable:

    python tools/query_rti_work.py focus process-time-advance-next-message-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassador requests next message advances through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador requests next message advances through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane process-time-advance-next-message-focused --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests next message advances through a configured process endpoint$" --output-on-failure
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests available time advance through a configured process endpoint$" --output-on-failure
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors apply a deferred lower lookahead at a configured process grant$" --output-on-failure

The queued-TSO companion is a separate two-federate process slice. It sends
timestamped interactions at 5 and 8, then proves NMR(10) selects 5 and
NMRA(10) selects 8, with each interaction callback before its grant:

    python tools/query_rti_work.py focus process-time-advance-next-message-queued-focused --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
    python tools/query_rti_work.py check --lane process-time-advance-next-message-queued-focused --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors select queued timestamped process messages for next-message advances$" --output-on-failure

The object-instance lookup companion is a separate public process slice. It
uses the service ledger to round-trip an unnamed instance's generated name and
handle, and checks unknown-name/unknown-handle exception mapping:

    python tools/query_rti_work.py focus object-instance-lookup --summary --compact
    python tools/query_rti_work.py trace "RTIambassador resolves known object-instance names and handles through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix object-instance-lookup --summary --compact
    python tools/query_rti_work.py check --lane object-instance-lookup --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador resolves known object-instance names and handles through a configured process endpoint$" --output-on-failure

The case at `ieee1516_2025_connection_catch2.cpp:12902` records 18
assertions under both callback models, maps five Lab requirements to clauses
`6.8.4`/`6.9.3`, and exercises six official API surfaces. Discovery fan-out,
reservation/collision, deletion/reuse, DDM, save/restore, MOM, package/JUnit,
review, validation, interoperability, and conformance remain separate lanes.

The reverse-FOM lookup companion is independently selectable:

    python tools/query_rti_work.py focus reverse-fom-lookup --summary --compact
    python tools/query_rti_work.py trace "RTIambassador reports reverse FOM lookup errors through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix reverse-fom-lookup --summary --compact
    python tools/query_rti_work.py check --lane reverse-fom-lookup --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador reports reverse FOM lookup errors through a configured process endpoint$" --output-on-failure

The lane now has four source-located cases and 76 aggregate assertions (m102
contributes 20; m103 contributes 24; m104 contributes 16; m105 contributes 16)
under both callback models. The m105 case at
`ieee1516_2025_connection_catch2.cpp:13647` records 16 assertions, maps six Lab
requirements to clauses `9.1.2`/`10.19`/`10.20.4`, and exercises four official API
surfaces for the unknown-name/invalid-handle error fence. The m104
dimension/transportation round-trip, m103, m102, and m101 cases remain
queryable by their exact titles; multi-federate declaration management,
package/JUnit, review, validation, interoperability, and conformance remain
separate.

The newest process callback-ordering slice is independently selectable:

    python tools/query_rti_work.py focus process-multi-recipient-callback-ordering --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix process-multi-recipient-callback-ordering --summary --compact
    python tools/query_rti_work.py check --lane process-multi-recipient-callback-ordering --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint$" --output-on-failure

The m107 case at `ieee1516_2025_connection_catch2.cpp:13968` records 79
assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps ten Lab anchors to six canonical 2025 subsections
and five official C++ API surfaces, and proves independent per-recipient FIFO
interaction delivery with preserved tags/parameters/producer identity, one
receive callback per Evoke Callback, and sender exclusion. Immediate,
callback-disable, timestamped/region/directed fanout, package/JUnit, review,
validation, interoperability, and conformance remain separate lanes.

The newest process TSO/DDM slice is independently selectable:

    python tools/query_rti_work.py focus timestamped-process-regional-interaction --summary --compact
    python tools/query_rti_work.py trace m109.embedded-process-tso-regional-interaction --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-process-regional-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a timestamped regional interaction through a configured process endpoint$" --output-on-failure

The m109 case at `ieee1516_2025_connection_catch2.cpp:15057` records 71
`HLA_EVOKED` assertions, maps 28 Lab anchors to 16 canonical 2025 subsections,
and exercises 20 official C++ API surfaces. It verifies overlap-qualified
timestamped regional process delivery, time-advance gating, conveyed source
region metadata, and timestamp/order/retraction reconstruction. Callback-disable,
directed, relaxed-DDM, package/JUnit, review, validation, interoperability, and
conformance remain separate lanes.

The preceding process DDM slice is independently selectable:

    python tools/query_rti_work.py focus process-multi-recipient-regional-interaction --summary --compact
    python tools/query_rti_work.py trace m108.embedded-process-multi-recipient-regional-interaction --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py check --lane process-multi-recipient-regional-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint$" --output-on-failure

The m108 case at `ieee1516_2025_connection_catch2.cpp:14414` records 126
assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps thirteen Lab anchors to
seven canonical 2025 subsections, and exercises thirteen official C++ API
surfaces. It proves overlap-filtered delivery to two disjoint regional
recipients, send-time source-region metadata, and sender exclusion. Timestamped
/retraction, directed, relaxed-DDM, callback-disable, package/JUnit, review,
validation, interoperability, and conformance remain separate lanes.

The preceding process declaration-management slice is the available-dimensions
hierarchy case:

    python tools/query_rti_work.py focus process-available-dimensions-hierarchy --summary --compact
    python tools/query_rti_work.py trace process-available-dimensions-hierarchy --summary --compact
    python tools/query_rti_work.py matrix process-available-dimensions-hierarchy --summary --compact
    python tools/query_rti_work.py check --lane process-available-dimensions-hierarchy --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador resolves available FOM dimensions through a configured process endpoint$" --output-on-failure

The m106 declaration at `ieee1516_2025_connection_catch2.cpp:13795` records
28 assertions under both callback models and maps four Lab anchors to clause
`9.1.2` through two official `RTIambassador` API surfaces. It verifies inherited
object-class dimensions, the empty interaction-class set, unknown-class
responses, and invalid public handles across the process boundary. Process
regions and multi-federate hierarchy behavior remain intentionally separate
focus lanes.

Flush Queue Request is the next adjacent time-management lane. It has 42
`HLA_EVOKED` assertions and maps the official request/grant path to clauses
8.12 and 8.12.3:

    python tools/query_rti_work.py focus flush-queue-request --summary --compact --limit 8
    python tools/query_rti_work.py trace "Embedded Flush Queue Request flushes queued TSO and reports optimistic time" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Flush Queue Request flushes queued TSO and reports optimistic time" --summary --compact
    ctest --test-dir <build-dir> -C Debug -L "^flush-queue-request$" --output-on-failure

The direct case passed from the existing binary; aggregate rebuild/discovery is
still behind the documented source-integrity gate. Regional/future-input,
save/restore, transport, and conformance remain separate lanes.

The next no-TSO scheduler slice has an exact title/source/requirement handle:

    python tools/query_rti_work.py focus no-tso-galt-scheduler --summary --compact --limit 8
    python tools/query_rti_work.py trace "Embedded constrained TAR waits for GALT and is released by a regulator advance" --summary --compact
    python tools/query_rti_work.py matrix "Embedded constrained TAR waits for GALT and is released by a regulator advance" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded constrained TAR waits for GALT and is released by a regulator advance$" --output-on-failure

It maps Time Advance Request, Enable Time Regulation, Enable Time Constrained,
and Time Advance Grant to two clause-8 Lab candidates and has 30
`HLA_EVOKED` assertions. The case holds a constrained TAR at GALT until the
regulator advances; NRG/undefined-GALT, timestamped ordering, alternate
advance modes, save/restore, transport, and conformance remain separate. The
aggregate source gate is reported by `focus`; use the exact title for direct
execution while the known federation-management source integrity issue remains.

The read-only Query GALT/Query LITS bounds slice is a separate exact handle:

    python tools/query_rti_work.py focus query-galt-lits --summary --compact --limit 8
    python tools/query_rti_work.py trace "Embedded Query GALT and Query LITS observe other regulator time and pending advances" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Query GALT and Query LITS observe other regulator time and pending advances" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded Query GALT and Query LITS observe other regulator time and pending advances$" --output-on-failure

It maps Query GALT and Query LITS to five exact clause-8/8.1.5/8.18.1/8.19.3
Lab candidates and has 35 `HLA_EVOKED` assertions. The case proves undefined
bounds before and after regulation, current lookahead, pending-advance bounds,
matching GALT/LITS values, and the logical-time type fence. Scheduler release,
timestamped delivery, alternate advance modes, save/restore, transport, and
conformance remain separate; use `focus query-galt-lits` for the aggregate source gate.

The configured process endpoint has a separate temporal-bounds baseline:

    python tools/query_rti_work.py focus process-query-time-bounds --summary --compact --limit 8
    python tools/query_rti_work.py trace "RTIambassador queries GALT and LITS through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador queries GALT and LITS through a configured process endpoint" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries GALT and LITS through a configured process endpoint$" --output-on-failure

This 12-assertion `HLA_EVOKED` case maps the same five 2025 Lab anchors to
the public Query GALT and Query LITS methods through the process seam. It
proves the undefined no-TSO bound. The available multi-federate, queued-TSO,
and zero-lookahead companions are separate, independently queryable slices;
grants, in-transit TSO, save/restore, package/JUnit, validation, and
conformance remain separate.

The available multi-federate process companion has its own exact handles:

    python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-multi-federate-integration --summary --compact
    python tools/query_rti_work.py focus process-query-time-bounds-multi-federate --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors expose defined GALT and LITS across configured process federates" --summary --compact
    python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-multi-federate-integration --summary --compact
    python tools/query_rti_work.py check --lane process-query-time-bounds-multi-federate --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassadors expose defined GALT and LITS across configured process federates$" --output-on-failure

It has 24 `HLA_IMMEDIATE` assertions over the same five Lab anchors and four
canonical sections. The regulator excludes its own time state from GALT/LITS,
while an observer receives the other regulator's lookahead as defined bounds;
queued/in-transit TSO, grants, save/restore, and conformance remain separate.

The queued-TSO LITS companion has its own exact handles:

    python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-queued-tso-integration --summary --compact
    python tools/query_rti_work.py focus process-query-time-bounds-queued-tso --summary --compact
    python tools/query_rti_work.py trace "RTIambassador queries LITS from queued timestamped process input after regulator disable" --summary --compact
    python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-queued-tso-integration --summary --compact
    python tools/query_rti_work.py check --lane process-query-time-bounds-queued-tso --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries LITS from queued timestamped process input after regulator disable$" --output-on-failure

It has 31 `HLA_IMMEDIATE` assertions over the same five Lab anchors and four
canonical sections plus twelve official API surfaces. It queues timestamped
input for a constrained observer, preserves undefined GALT after regulator
disable, and reports the queued timestamp as defined LITS; in-transit TSO,
zero-lookahead epsilon, grants, retraction, save/restore, package/JUnit,
validation, and conformance remain separate.

The zero-lookahead process companion has its own exact handles:

    python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-zero-lookahead-integration --summary --compact
    python tools/query_rti_work.py focus process-query-time-bounds-zero-lookahead --summary --compact
    python tools/query_rti_work.py trace "RTIambassador exposes a zero-lookahead exclusive GALT and LITS boundary through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-zero-lookahead-integration --summary --compact
    python tools/query_rti_work.py check --lane process-query-time-bounds-zero-lookahead --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador exposes a zero-lookahead exclusive GALT and LITS boundary through a configured process endpoint$" --output-on-failure

It has 28 `HLA_EVOKED` assertions over six Lab anchors, four canonical
sections, and six official C++ API surfaces. It proves the exclusive integer
boundary after an evoked zero-lookahead Time Advance Grant; in-transit TSO,
queued-TSO delivery, grant scheduling, retraction, save/restore, and
conformance remain separate.

For any mapped reverse lookup, `section <document:clause-or-number> --summary --compact`
and `requirement <lab-id> --summary --compact` use the same bounded
matrix row as `matrix`; each row includes source/assertion counts and direct
requirement-to-subsection pairs without expanding contract provenance. The
process in-transit TSO case remains a documented protocol seam until delivery
acknowledgement or a deterministic test barrier exists; do not introduce a
sleep-based timing test.

The process time-role callback baseline is separately queryable:

    python tools/query_rti_work.py focus process-time-role --summary --compact --limit 8
    python tools/query_rti_work.py trace "RTIambassador enables time constrained through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador enables time constrained through a configured process endpoint" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador enables time constrained through a configured process endpoint$" --output-on-failure

This 11-assertion `HLA_EVOKED` case maps Enable Time Constrained and the Time
Constrained Enabled callback through the process seam to two existing clause-8
time-role anchors. It proves callback gating for the endpoint-owned
role transition; process grants, multi-federate bounds, timestamped delivery,
save/restore, package/JUnit, validation, and conformance remain separate.

The configured process endpoint also has an independently queryable
Time Advance Request/grant baseline:

    python tools/query_rti_work.py focus process-time-advance --summary --compact
    python tools/query_rti_work.py trace "RTIambassador requests time advance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador requests time advance through a configured process endpoint" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests time advance through a configured process endpoint$" --output-on-failure

This 14-assertion `HLA_EVOKED` case maps the official Time Advance Request,
Time Advance Grant, and Query Logical Time surfaces to four exact 2025 Lab
anchors. It proves the endpoint-owned request/grant transition and callback
queueing; distributed GALT/LITS/TSO scheduling, save/restore, package/JUnit,
validation, and conformance remain separate.

The matching process rejection fence is independently runnable:

    python tools/query_rti_work.py focus process-time-advance-rejection --summary --compact
    python tools/query_rti_work.py trace "RTIambassador rejects a backward time advance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador rejects a backward time advance through a configured process endpoint" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a backward time advance through a configured process endpoint$" --output-on-failure

This 12-assertion `HLA_EVOKED` case proves the official
`LogicalTimeAlreadyPassed` exception after an endpoint-owned grant and keeps
the grant callback count stable. Pending-role cases and distributed
GALT/LITS/TSO scheduling remain separate lanes.

The incompatible-time decode fence is independently runnable:

    python tools/query_rti_work.py focus process-time-advance-invalid-time --summary --compact
    python tools/query_rti_work.py trace "RTIambassador rejects an incompatible logical time through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador rejects an incompatible logical time through a configured process endpoint" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects an incompatible logical time through a configured process endpoint$" --output-on-failure

This 9-assertion `HLA_EVOKED` case proves the official `InvalidLogicalTime`
exception for an HLAfloat64Time supplied to an HLAinteger64Time federation and
confirms that no grant callback is created.

The callback-gated pending-role rejection is independently runnable:

    python tools/query_rti_work.py focus process-time-advance-time-regulation-pending --summary --compact
    python tools/query_rti_work.py trace "RTIambassador rejects a process time advance while time regulation enable is pending" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador rejects a process time advance while time regulation enable is pending" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a process time advance while time regulation enable is pending$" --output-on-failure

This 14-assertion `HLA_EVOKED` case keeps the official
`RequestForTimeRegulationPending` exception active until the queued
`timeRegulationEnabled` callback crosses the shared dispatcher. The analogous
Time-Constrained pending case is independently mapped below; malformed
encoding, distributed scheduling, and conformance remain separate slices.

The matching constrained-role pending case is independently runnable:

    python tools/query_rti_work.py focus process-time-advance-time-constrained-pending --summary --compact
    python tools/query_rti_work.py trace "RTIambassador rejects a process time advance while time constrained enable is pending" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador rejects a process time advance while time constrained enable is pending" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a process time advance while time constrained enable is pending$" --output-on-failure

This 14-assertion `HLA_EVOKED` case keeps the official
`RequestForTimeConstrainedPending` exception active until the queued
`timeConstrainedEnabled` callback crosses the shared dispatcher. It maps the
Enable Time Constrained request/callback and Time Advance Request to clauses
8.5.5, 8.6.3, and 8.8.3; malformed encoding, distributed scheduling, and
conformance remain separate slices.

The malformed process logical-time decode case is independently runnable:

    python tools/query_rti_work.py focus process-time-advance-malformed-encoding --summary --compact
    python tools/query_rti_work.py trace "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects malformed logical-time encoding through a configured process endpoint$" --output-on-failure

This 9-assertion `HLA_EVOKED` case injects a one-byte malformed logical-time
encoding at the process boundary, proves the official `InvalidLogicalTime`
exception, and confirms that no grant callback is emitted. It maps directly to
clause 8.8.3; valid grants, pending-role fences, distributed scheduling, and
conformance remain separate slices.

The two-federate process scheduler case has its own exact crosswalk handle:

    python tools/query_rti_work.py focus process-time-advance-federation-scheduler --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors coordinate deferred process time advances through the federation scheduler$" --output-on-failure

This 30-assertion `HLA_EVOKED` case maps Enable Time Regulation, Enable Time
Constrained, Time Advance Request, and Time Advance Grant to clauses 8.2,
8.5.5, 8.6.3, and 8.8.3. It proves deferred constrained TAR admission,
regulator-driven release, unsolicited grant transport, and Evoke callback
delivery. Timestamped TSO delivery, save/restore, package/JUnit, validation,
interoperability, and conformance remain separate slices.

The focused timestamped process-interaction-before-grant case has its own
crosswalk handle:

    python tools/query_rti_work.py focus process-tso-interaction-before-grant --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a deferred timestamped process interaction before the grant$" --output-on-failure

This 60-assertion `HLA_EVOKED` case maps 15 Requirements-Lab anchors to nine
canonical 2025 sections. It proves a regulating sender's timestamped
`MainCourseServed` interaction is retained for a constrained receiver at
logical time 5, carries a valid execution-owned retraction handle, crosses the
process boundary before the matching Time Advance Grant, and preserves the
official parameter/tag/transportation/producer/time/order callback surface.
Pre-grant retraction, multiple-message ordering, directed or regional TSO,
save/restore, package/JUnit, validation, interoperability, and conformance
remain separate slices.

The support-switch state case has its own exact crosswalk handle:

    python tools/query_rti_work.py focus support-switch-state --summary --compact --limit 8
    python tools/query_rti_work.py trace "Embedded support switches are seeded per federate and retain static FDD policy" --summary --compact
    python tools/query_rti_work.py matrix "Embedded support switches are seeded per federate and retain static FDD policy" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded support switches are seeded per federate and retain static FDD policy$" --output-on-failure

It maps the twelve official support-switch APIs to eight exact Lab candidates
across clauses 8.1.10, 9.1.8, 10.44, 10.45.3, 10.46.6, 10.48.1, 10.50.6,
and 10.55.1 and has 40 `HLA_EVOKED` assertions. It proves FDD-seeded
per-federate state, mutation isolation, static getters, and invalid resign
action handling. Keep HLAsetSwitches, MOM interlocks, connection-loss cleanup,
delayed timestamped delivery, relaxed-DDM routing, filesystem reporting, and
conformance separate; `focus` reports the aggregate source gate.

The whole-object-class declaration teardown has a dedicated crosswalk handle:

    python tools/query_rti_work.py focus whole-object-class-declaration --summary --compact --limit 8
    python tools/query_rti_work.py trace "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries" --summary --compact
    python tools/query_rti_work.py matrix "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.fom_declaration_management\.catch2\.Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries$" --output-on-failure

It maps whole-class unpublication/unsubscription and declaration teardown to
eight exact Lab candidates in clauses 5.3, 5.3.3, and 5.9 and has 34
`HLA_EVOKED` assertions. It covers invalid-handle/member fences, inherited
publication lifetime, ordinary-subscription removal with independent regional
state, idempotent teardown, and later-update rejection. Keep publication
setup, ownership arbitration, regional teardown, save/restore, transport,
packaging, validation, and conformance separate.

The public handle-decoding API slice is an explicit, queryable disposition:

    python tools/query_rti_work.py focus public-handle-decoding --summary --compact --limit 8
    python tools/query_rti_work.py trace "Embedded public handle decoders enforce lifecycle and preserve encoded identities" --summary --compact
    python tools/query_rti_work.py matrix "Embedded public handle decoders enforce lifecycle and preserve encoded identities" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded public handle decoders enforce lifecycle and preserve encoded identities$" --output-on-failure

The Lab exports the eight official decoding APIs but no standalone 2025
requirement candidate for this codec behavior, so no requirement IDs are
invented. The case has 50 assertions for lifecycle fences, scoped round trips,
and malformed-value rejection; cross-RTI interoperability, transport,
packaging, protected review, validation, and conformance remain separate.

The 2025 region-template lifecycle is independently buildable in
`region_lifecycle_catch2.cpp:51`. It passes 67 assertions and maps the
official create/commit/delete/range-bound/handle surfaces to eight Lab
requirements across §§9.1.2, 9.3, and 9.4. Use the exact lane handles instead
of the federation-management aggregate:

    python tools/query_rti_work.py focus region-lifecycle --summary --compact
    python tools/query_rti_work.py trace "Standalone 2025 region templates preserve pending and committed range state" --summary --compact
    python tools/query_rti_work.py matrix region-lifecycle --summary --compact
    python tools/query_rti_work.py check --lane region-lifecycle --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.region_lifecycle\.catch2\.Standalone 2025 region templates preserve pending and committed range state$" --output-on-failure

The standalone Query Attribute Ownership case is independently buildable in
`attribute_ownership_query_catch2.cpp:125`. It passes 48 assertions and maps
the official query and owner/unowned callback surfaces to six Lab requirements
in §§7.17.5 and 7.18.4. It deliberately keeps the RTI-owned callback variant
in its separate mapped rows. Use the ownership lane handles for this focused
slice:

    python tools/query_rti_work.py focus query-attribute-ownership --summary --compact
    python tools/query_rti_work.py trace "Standalone Query Attribute Ownership groups 2025 owner and unowned results" --summary --compact
    python tools/query_rti_work.py matrix query-attribute-ownership --summary --compact
    python tools/query_rti_work.py check --lane query-attribute-ownership --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.attribute_ownership_query\\.catch2\\.Standalone Query Attribute Ownership groups 2025 owner and unowned results$" --output-on-failure

The Query Attribute Ownership service-report interaction case is independently
buildable in `attribute_ownership_query_catch2.cpp:296`. It passes 67
assertions and maps the accepted service-report invocation, standard MOM
interaction, and grouped owner/unowned callbacks to 11 Lab requirements across
§§7.17.5, 7.18.4, 11.5, 11.5.1, 11.5.2, and 11.5.2.1. The HLA_IMMEDIATE
observer receives the decoded type-3 report before the HLA_EVOKED requester
drains its ownership-result callbacks:

    python tools/query_rti_work.py focus query-attribute-ownership-service-report-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers Query Attribute Ownership through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix query-attribute-ownership-service-report-interaction --summary --compact
    python tools/query_rti_work.py check --lane query-attribute-ownership-service-report-interaction --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\\.attribute_ownership_query\\.catch2\\.Embedded service reporting delivers Query Attribute Ownership through MOM interaction$" --output-on-failure

The Cancel Attribute Ownership Acquisition service-report interaction case is
independently buildable in
`attribute_ownership_acquisition_cancellation_catch2.cpp:387`. It passes 77
assertions and maps the accepted cancellation, confirmation callback, and MOM
service-report invocation to six Lab requirements across §§7.15, 7.16, 11.5,
11.5.2, and 11.5.2.1. The HLA_IMMEDIATE observer receives the decoded
type-3 report before the HLA_EVOKED requester drains its queued cancellation
confirmation:

    python tools/query_rti_work.py focus cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers Cancel Attribute Ownership Acquisition through MOM" --summary --compact
    python tools/query_rti_work.py matrix cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
    python tools/query_rti_work.py check --lane cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\\.attribute_ownership_acquisition_cancellation\\.catch2\\.Embedded service reporting delivers Cancel Attribute Ownership Acquisition through MOM$" --output-on-failure

The Cancel Negotiated Attribute Ownership Divestiture service-report
interaction case is independently buildable in
`negotiated_attribute_ownership_divestiture_pending_catch2.cpp:547`. It passes
87 assertions and maps the acquisition prerequisite, negotiated cancellation,
and MOM service-report invocation to five Lab requirements across §§7.8,
7.14.6, 11.5, 11.5.2, and 11.5.2.1. The HLA_IMMEDIATE observer receives the
decoded type-3 report before the HLA_EVOKED owner drains its restored ordinary
release callback:

    python tools/query_rti_work.py focus cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers Cancel Negotiated Attribute Ownership Divestiture through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
    python tools/query_rti_work.py check --lane cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\\.negotiated_attribute_ownership_divestiture_pending\\.catch2\\.Embedded service reporting delivers Cancel Negotiated Attribute Ownership Divestiture through MOM interaction$" --output-on-failure

The MOM Service Reporting subscription interlock is independently buildable in
`mom_service_reporting_interlock_catch2.cpp:50`. It passes 41 `HLA_EVOKED`
assertions and maps the ordinary declaration, regional DDM declaration, and
Service Reporting switch boundaries to three Lab requirements in §§5.10.2,
9.10.3, and 11.5. The case rejects active and passive report-service
subscriptions while reporting is enabled, preserves the switch on failed
enable attempts, and verifies removal-before-enable recovery for ordinary and
regional subscriptions. Its committed `HLAserviceGroup` region setup also
keeps the regional path on the standard MIM context:

    python tools/query_rti_work.py focus service-reporting-interlock --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM service-reporting state excludes report-service subscriptions" --summary --compact
    python tools/query_rti_work.py matrix service-reporting-interlock --summary --compact
    python tools/query_rti_work.py check --lane service-reporting-interlock --summary --compact
    cmake --build <build-dir> --config Release --target umbra_mom_service_reporting_interlock_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\\.mom_service_reporting_interlock\\.catch2\\.Embedded MOM service-reporting state excludes report-service subscriptions$" --output-on-failure

This is bounded development-profile evidence; generic MOM interaction
generation/routing, filesystem report-file lifecycle, remote transport,
package/JUnit/protected review, Requirements-Lab validation, interoperability,
and conformance remain separate lanes.

The joined-federate MOM deletable-object-count case is independently buildable
in `joined_federate_mom_deletable_object_count_catch2.cpp:108`. It passes 55
`HLA_EVOKED` assertions and maps the live
`HLAprivilegeToDeleteObject`/`HLAobjectInstancesThatCanBeDeleted` projection to
the §11.4.1 Lab requirement. The observer requests the owner's MOM object
directly, verifies counts 0 → 1 after implicit-privilege registration, observes
the same count through an `HLAsetTiming` periodic reflection, then verifies 0
after owner deletion. RTI-owned MOM objects are excluded:

    python tools/query_rti_work.py focus joined-federate-mom-deletable-object-count --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes deletable object count" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-deletable-object-count --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-deletable-object-count --summary --compact
    cmake --build <build-dir> --config Release --target umbra_joined_federate_mom_deletable_object_count_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.joined_federate_mom_deletable_object_count\.catch2\.Embedded joined-federate MOM exposes deletable object count$" --output-on-failure

This is bounded development-profile MOM evidence; remaining traffic/statistical
attributes, regional/transport variants, public producer mapping, remote
transport, package/JUnit/protected review, Requirements-Lab validation,
interoperability, and conformance remain separate lanes.

The joined-federate MOM receive-order-length case is independently buildable in
`joined_federate_mom_ro_length_periodic_catch2.cpp:134`. It passes 63
`HLA_EVOKED` assertions and maps `HLAROlength` to the §11.4.1 Lab requirement.
The direct request samples the recipient-scoped receive-order queue before
delivery (0 → 1), the same value is exposed by `HLAsetTiming` periodic
reflection, and the count returns to 0 after the application callback crosses
its delivery boundary. RTI-owned MOM traffic is not counted. Use the exact
handles below:

    python tools/query_rti_work.py focus joined-federate-mom-ro-length-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes receive-order queue length" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-ro-length-periodic --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-ro-length-periodic --summary --compact
    cmake --build <build-dir> --config Release --target umbra_joined_federate_mom_ro_length_periodic_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.joined_federate_mom_ro_length_periodic\.catch2\.Embedded joined-federate MOM exposes receive-order queue length$" --output-on-failure

This remains bounded development-profile MOM evidence; deferred asynchronous/
TSO variants, remaining traffic/statistical attributes, regional/transport
variants, public producer mapping, remote transport, package/JUnit/protected
review, Requirements-Lab validation, interoperability, and conformance remain
separate lanes.

The joined-federate MOM `HLAupdatesSent` projection is independently buildable
in `joined_federate_mom_updates_sent_periodic_catch2.cpp:108`. It passes 56
`HLA_EVOKED` assertions and maps the accepted `Update Attribute Values`
invocation ledger to the §11.4.1 Lab requirement. Direct MOM requests expose
the official `HLAinteger32BE` count 0 → 1 → 2, and `HLAsetTiming` periodic
reflection preserves 2; the counter is per accepted service invocation, not per
value or downstream callback:

    python tools/query_rti_work.py focus joined-federate-mom-updates-sent-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAupdatesSent count" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-updates-sent-periodic --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-updates-sent-periodic --summary --compact
    cmake --build <build-dir> --config Release --target umbra_joined_federate_mom_updates_sent_periodic_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.joined_federate_mom_updates_sent_periodic\.catch2\.Embedded joined-federate MOM exposes HLAupdatesSent count$" --output-on-failure

This remains bounded development-profile MOM evidence; remaining statistics,
timestamped/update-rate and regional/transport variants, public producer
mapping, package/JUnit/protected review, Requirements-Lab validation,
interoperability, and conformance remain separate lanes.

The joined-federate MOM `HLAobjectInstancesUpdated` projection is independently
buildable in
`joined_federate_mom_updated_object_count_periodic_catch2.cpp:109`. It passes
77 `HLA_EVOKED` assertions and maps the distinct-object Update Attribute Values
ledger to the §11.4.1 Lab requirement. Direct MOM requests expose the official
`HLAinteger32BE` count as 0 → 1 → 1 → 2: two accepted updates to one object
remain one distinct object, then an update to a second object raises the value
to two. The periodic `HLAsetTiming` reflection preserves 2 alongside
`HLAupdatesSent=3`:

    python tools/query_rti_work.py focus joined-federate-mom-updated-object-count-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAobjectInstancesUpdated count" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-updated-object-count-periodic --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-updated-object-count-periodic --summary --compact
    cmake --build <build-dir> --config Release --target umbra_joined_federate_mom_updated_object_count_periodic_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.joined_federate_mom_updated_object_count_periodic\.catch2\.Embedded joined-federate MOM exposes HLAobjectInstancesUpdated count$" --output-on-failure

This remains bounded development-profile MOM evidence; remaining statistics,
timestamped/update-rate and regional/transport variants, public producer
mapping, package/JUnit, protected review, Requirements-Lab validation,
interoperability, and conformance remain separate lanes.

The joined-federate MOM `HLAobjectInstancesRegistered` projection is
independently buildable in
`joined_federate_mom_registered_object_count_periodic_catch2.cpp:115`. It
passes 112 `HLA_EVOKED` assertions and maps the successful registration ledger
to the §11.4.1 Lab requirement. Direct MOM requests expose the official
`HLAinteger32BE` count as 0 → 1 → 2 for two successful object registrations;
three accepted updates establish `HLAupdatesSent=3` and
`HLAobjectInstancesUpdated=2`, and the periodic `HLAsetTiming` reflection
preserves that complete snapshot:

    python tools/query_rti_work.py focus joined-federate-mom-registered-object-count-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAobjectInstancesRegistered count" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-registered-object-count-periodic --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-registered-object-count-periodic --summary --compact
    cmake --build <build-dir> --config Release --target umbra_joined_federate_mom_registered_object_count_periodic_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.joined_federate_mom_registered_object_count_periodic\.catch2\.Embedded joined-federate MOM exposes HLAobjectInstancesRegistered count$" --output-on-failure

This remains bounded development-profile MOM evidence; invalid registration
paths, remaining statistics, timestamped/update-rate and regional/transport
variants, public producer mapping, package/JUnit, protected review,
Requirements-Lab validation, interoperability, and conformance remain
separate lanes.

The standalone If Available ownership-acquisition case is independently
buildable in `attribute_ownership_acquisition_if_available_catch2.cpp:125`.
It passes 59 assertions and maps the request, pending-state, atomic transfer,
and unavailable-callback surfaces to four Lab requirements in §§7.7.3, 7.9.1,
and 7.10.6:

    python tools/query_rti_work.py focus attribute-ownership-acquisition-if-available --summary --compact
    python tools/query_rti_work.py trace "Standalone Attribute Ownership Acquisition If Available resolves 2025 callbacks" --summary --compact
    python tools/query_rti_work.py matrix attribute-ownership-acquisition-if-available --summary --compact
    python tools/query_rti_work.py check --lane attribute-ownership-acquisition-if-available --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.attribute_ownership_acquisition_if_available\\.catch2\\.Standalone Attribute Ownership Acquisition If Available resolves 2025 callbacks$" --output-on-failure

The standalone regular ownership-acquisition case is independently buildable
in `attribute_ownership_acquisition_catch2.cpp:141`. It passes 74 assertions
and maps the regular acquisition, owner-release, notification, and denial
surfaces to six Lab requirements in §§7.7.3, 7.8, 7.11, and 7.12:

    python tools/query_rti_work.py focus attribute-ownership-acquisition --summary --compact
    python tools/query_rti_work.py trace "Standalone Attribute Ownership Acquisition honors 2025 release and denial callbacks" --summary --compact
    python tools/query_rti_work.py matrix attribute-ownership-acquisition --summary --compact
    python tools/query_rti_work.py check --lane attribute-ownership-acquisition --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.attribute_ownership_acquisition\\.catch2\\.Standalone Attribute Ownership Acquisition honors 2025 release and denial callbacks$" --output-on-failure

The standalone Unconditional Attribute Ownership Divestiture case is
independently buildable in
`unconditional_attribute_ownership_divestiture_catch2.cpp:122`. It passes 116
assertions and maps validated-set unownership, candidate filtering, assumption
offers, tag propagation, and later acquisition to six Lab requirements in
§§7.1.2.1, 7.2, and 7.4:

    python tools/query_rti_work.py focus unconditional-attribute-ownership-divestiture --summary --compact
    python tools/query_rti_work.py trace "Standalone Unconditional Attribute Ownership Divestiture offers eligible 2025 federates" --summary --compact
    python tools/query_rti_work.py matrix unconditional-attribute-ownership-divestiture --summary --compact
    python tools/query_rti_work.py check --lane unconditional-attribute-ownership-divestiture --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.unconditional_attribute_ownership_divestiture\\.catch2\\.Standalone Unconditional Attribute Ownership Divestiture offers eligible 2025 federates$" --output-on-failure

The standalone resign-action directive-1 case is independently buildable in
`resign_action_unconditional_divestiture_catch2.cpp:104`. It passes 32
assertions under HLA_EVOKED and maps the official `resignFederationExecution`,
`requestAttributeOwnershipAssumption`, and `attributeOwnershipAcquisition`
surfaces to the three §4.12 Requirements-Lab candidates for resolving owned
attributes, leaving them unowned, and offering current eligible survivors:

    python tools/query_rti_work.py focus resign-action-unconditional-divestiture --summary --compact
    python tools/query_rti_work.py trace "Standalone resign action unconditionally divests attributes for the 2025 ownership model" --summary --compact
    python tools/query_rti_work.py matrix resign-action-unconditional-divestiture --summary --compact
    python tools/query_rti_work.py check --lane resign-action-unconditional-divestiture --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.resign_action_unconditional_divestiture\\.catch2\\.Standalone resign action unconditionally divests attributes for the 2025 ownership model$" --output-on-failure

The standalone resign-action pending-acquisition rejection case is
independently buildable in
`resign_action_pending_acquisition_rejection_catch2.cpp:113`. It passes 23
assertions under HLA_EVOKED and maps the pending regular acquisition,
resignation precondition, and stale owner-release suppression to the existing
§4.12 and §7.8 Requirements-Lab candidates:

    python tools/query_rti_work.py focus resign-action-pending-acquisition-rejection --summary --compact
    python tools/query_rti_work.py trace "Standalone resign action rejects pending ownership acquisition work" --summary --compact
    python tools/query_rti_work.py matrix resign-action-pending-acquisition-rejection --summary --compact
    python tools/query_rti_work.py check --lane resign-action-pending-acquisition-rejection --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.resign_action_pending_acquisition_rejection\\.catch2\\.Standalone resign action rejects pending ownership acquisition work$" --output-on-failure

The standalone directive-2 delete case is independently buildable in
`resign_action_delete_objects_catch2.cpp:96`. It passes 25 assertions under
HLA_EVOKED and maps the official `FederateOwnsAttributes` precondition,
delete-privileged object removal, `Remove Object Instance` callback metadata,
and post-removal unknown-instance state:

    python tools/query_rti_work.py focus resign-action-delete-objects --summary --compact
    python tools/query_rti_work.py trace "Standalone resign action deletes delete-privileged objects and reports removal" --summary --compact
    python tools/query_rti_work.py matrix resign-action-delete-objects --summary --compact
    python tools/query_rti_work.py check --lane resign-action-delete-objects --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.resign_action_delete_objects\\.catch2\\.Standalone resign action deletes delete-privileged objects and reports removal$" --output-on-failure

The final-federate directive-2 override is independently buildable in
`resign_action_final_federate_catch2.cpp:69`. It passes 22 assertions and
maps §4.12.4 to the official object-name reservation callback: a final
federate resigning with `NO_ACTION` still deletes its object, and a fresh
joined lifetime can reserve and register the deleted object's name:

    python tools/query_rti_work.py focus resign-action-final-federate --summary --compact
    python tools/query_rti_work.py trace "Standalone final-federate resignation applies directive two regardless of the supplied action" --summary --compact
    python tools/query_rti_work.py matrix resign-action-final-federate --summary --compact
    python tools/query_rti_work.py check --lane resign-action-final-federate --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.resign_action_final_federate\\.catch2\\.Standalone final-federate resignation applies directive two regardless of the supplied action$" --output-on-failure

The FDD update-rate metadata lookup is independently buildable in
`update_rate_value_catch2.cpp:48`. It passes 40 assertions and maps the
official `Get Update Rate Value`, `Get Update Rate Value For Attribute`,
active/passive subscription, and unsubscribe surfaces to §§5.2.4, 5.8,
10.11.6, and 10.12:

    python tools/query_rti_work.py focus update-rate-value --summary --compact
    python tools/query_rti_work.py trace "Standalone update-rate lookup exposes FDD values and the default attribute boundary" --summary --compact
    python tools/query_rti_work.py matrix update-rate-value --summary --compact
    python tools/query_rti_work.py check --lane update-rate-value --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.update_rate_value\\.catch2\\.Standalone update-rate lookup exposes FDD values and the default attribute boundary$" --output-on-failure

The three directive-3 resign-action cancellation variants are independently
buildable and intentionally kept as separate queryable lanes. The regular
acquisition case is `resign_action_cancel_pending_acquisition_catch2.cpp:96`
with 24 assertions; negotiated cancellation is
`resign_action_cancel_negotiated_pending_catch2.cpp:114` with 25 assertions;
If Available cancellation is
`resign_action_cancel_if_available_pending_catch2.cpp:115` with 22 assertions.
All run under HLA_EVOKED and map their exact §4.12/§7.2/§7.3/§7.8/§7.9.1/
§7.15 candidates, including stale callback suppression:

    python tools/query_rti_work.py focus resign-action-cancel-pending-acquisition --summary --compact
    python tools/query_rti_work.py focus resign-action-cancel-negotiated-pending --summary --compact
    python tools/query_rti_work.py focus resign-action-cancel-if-available-pending --summary --compact
    python tools/query_rti_work.py trace "Standalone resign action cancels pending ownership acquisition work" --summary --compact
    python tools/query_rti_work.py matrix resign-action-cancel-negotiated-pending --summary --compact
    python tools/query_rti_work.py check --lane resign-action-cancel-if-available-pending --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "resign_action_cancel_(pending_acquisition|negotiated_pending|if_available_pending)" --output-on-failure

The standalone Connection Lost automatic-divestiture case is independently
buildable in
`connection_loss_automatic_unconditional_divestiture_catch2.cpp:95`. It passes
38 assertions under HLA_EVOKED and maps the configured support-service
directive, official Connection Lost callback, retained object, released
ownership, and survivor assumption offer to the existing Connection Lost and
support-switch requirements:

    python tools/query_rti_work.py focus connection-lost-automatic-unconditional-divestiture --summary --compact
    python tools/query_rti_work.py trace "Standalone transport loss applies the configured automatic unconditional-divest directive" --summary --compact
    python tools/query_rti_work.py matrix connection-lost-automatic-unconditional-divestiture --summary --compact
    python tools/query_rti_work.py check --lane connection-lost-automatic-unconditional-divestiture --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.connection_loss_automatic_unconditional_divestiture\\.catch2\\.Standalone transport loss applies the configured automatic unconditional-divest directive$" --output-on-failure

The standalone Connection Lost pending-acquisition cancellation case is
independently buildable in
`connection_loss_automatic_cancel_pending_acquisition_catch2.cpp:113`. It
passes 62 assertions under HLA_EVOKED and maps directive-3 cleanup, stale
owner-release suppression, the official Connection Lost callback, and later
survivor assumption delivery to the existing Connection Lost/support-switch
requirements:

    python tools/query_rti_work.py focus connection-lost-automatic-cancel-pending-acquisition --summary --compact
    python tools/query_rti_work.py trace "Standalone transport loss cancels the lost federate's pending ownership acquisition" --summary --compact
    python tools/query_rti_work.py matrix connection-lost-automatic-cancel-pending-acquisition --summary --compact
    python tools/query_rti_work.py check --lane connection-lost-automatic-cancel-pending-acquisition --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.connection_loss_automatic_cancel_pending_acquisition\\.catch2\\.Standalone transport loss cancels the lost federate's pending ownership acquisition$" --output-on-failure

Save/restore interaction slices have exact indexed handles so their focused
checks do not require a full integration-label scan. The regional
multi-recipient case is source-backed at
`restore_live_tso_regional_interaction_multi_recipient_catch2.cpp:169` with
128 assertions and 11 mapped 2025 sections:

The public fresh-registry application-value/report-file companion is an
independent 82-assertion HLA_EVOKED case at
`public_process_restart_application_value_catch2.cpp:189`. It maps nine
Requirements-Lab anchors to five canonical 2025 sections and 18 official C++
API surfaces. Query only this lane when changing filesystem state-image
rehydration or joined-federate report-file identity:

    python tools/query_rti_work.py focus public-process-restart-application-value-focused --summary --compact
    python tools/query_rti_work.py trace "Embedded public fresh-registry restore rehydrates application value and retains report files" --summary --compact
    python tools/query_rti_work.py matrix public-process-restart-application-value-focused --summary --compact
    python tools/query_rti_work.py check --lane public-process-restart-application-value-focused --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_public_process_restart_application_value_catch2
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.public_process_restart_application_value\.catch2\.Embedded public fresh-registry restore rehydrates application value and retains report files$" --output-on-failure

It remains development-profile evidence; pending application-request ledgers,
timestamped payloads, ownership transfer, remote/package/JUnit/protected-review
evidence, interoperability, and conformance remain separate lanes.

    python tools/query_rti_work.py focus timestamped-regional-interaction-restore-multi-recipient --summary --compact
    python tools/query_rti_work.py trace "Embedded federation restore restores one queued timestamped regional interaction to multiple recipients" --summary --compact
    python tools/query_rti_work.py matrix "Embedded federation restore restores one queued timestamped regional interaction to multiple recipients" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-regional-interaction-restore-multi-recipient --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded federation restore restores one queued timestamped regional interaction to multiple recipients$" --output-on-failure

Use the non-regional companion's exact lane in the same way with
`timestamped-interaction-restore-multi-recipient`; the two lanes are separate
so regional overlap and source-region preservation can be changed without
re-running the ordinary interaction case.

The timed explicit-source regional attribute-update restore slice is source-
backed at `timed_restore_live_tso_regional_attribute_update_catch2.cpp:190`
with 84 HLA_EVOKED assertions, 23 Requirements-Lab anchors, 15 canonical
2025 sections, and 23 selected official C++ API surfaces. It saves at logical
time 6 while a timestamp-8 overlap-qualified `Update Attribute Values` passel
is queued, removes the live association after save, restores the object,
source `RegionHandle`, recipient ledger, and retraction identity, then proves
Flush Queue reflection at actual time 7 with optimistic time 8. Resolve it
without a broad scan:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-live-restore-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary" --summary --compact
    python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-live-restore-state --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-live-restore-state --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_timed_restore_regional_attr_catch2
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timed_restore_live_tso_regional_attribute_update\\.catch2\\.Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary$" --output-on-failure

The timed explicit-source regional attribute-update fan-out companion is
source-backed at
`timed_restore_live_tso_regional_attribute_update_multi_recipient_catch2.cpp:190`
with 157 HLA_EVOKED assertions, 23 Requirements-Lab anchors, 15 canonical
2025 sections, and 23 selected official C++ API surfaces. It keeps one
timestamp-8 overlap-qualified update queued for two constrained regional
recipients through the logical-time-6 save, restores both recipient ledgers and
the committed source `RegionHandle`, then releases each recipient independently
at actual time 7 (optimistic time 8) before one Request Retraction reaches both:

    python tools/query_rti_work.py focus timestamped-regional-attribute-timed-restore-multi-recipient --summary --compact
    python tools/query_rti_work.py trace "Embedded timed federation restore restores one queued explicit-source regional timestamped attribute update to multiple recipients" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed federation restore restores one queued explicit-source regional timestamped attribute update to multiple recipients" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-regional-attribute-timed-restore-multi-recipient --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed federation restore restores one queued explicit-source regional timestamped attribute update to multiple recipients$" --output-on-failure

The timed source-resignation companion is source-backed at
`timed_live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp:180`
with 106 HLA_EVOKED assertions, 27 Requirements-Lab anchors, 18 canonical
2025 sections, and 23 selected official C++ API surfaces. It saves an
overlap-qualified timestamp-8 explicit-source update at logical time 6,
mutates the source region before and after restore, resigns the producer with
`UNCONDITIONALLY_DIVEST_ATTRIBUTES`, and proves the independent regulator can
release the surviving receiver's saved passel while retaining the invocation-
time source `RegionHandle`, payload, timestamp, tag, and callback ordering:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-live-resignation-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-live-resignation-state --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_timed_regional_attr_source_resign_restore_catch2
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timed_live_tso_regional_attribute_update_source_resignation_after_restore\\.catch2\\.Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore$" --output-on-failure

The four-member timed multi-recipient source-resignation companion is
source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_source_resignation_after_restore_catch2.cpp:190`
with 167 HLA_EVOKED assertions, 27 Requirements-Lab anchors, 18 canonical
2025 sections, and 22 selected official C++ API surfaces. It keeps the same
timestamp-8 explicit-source passel queued through the logical-time-6 save,
mutates the source region before and after restore, resigns the producer with
`UNCONDITIONALLY_DIVEST_ATTRIBUTES`, and uses an independent regulating clock
to release both surviving recipient copies at the Flush Queue boundary:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-multi-resignation-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-multi-resignation-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives source mutation and resignation after restore$" --output-on-failure

The four-member timed multi-recipient delete-then-divest companion is
source-backed at
\`timed_live_tso_regional_attribute_update_multi_recipient_delete_then_divest_after_restore_catch2.cpp:218\`
with 124 HLA_EVOKED assertions, 36 Requirements-Lab anchors, 19 canonical
2025 sections, and 25 selected official C++ API surfaces. It keeps one
timestamp-8 overlap-qualified explicit-source update queued through the
logical-time-6 save, restores the committed source region after a mutation,
mutates it to a disjoint range, and resigns with
\`DELETE_OBJECTS_THEN_DIVEST\`. Each constrained recipient independently flushes
one receive-order Remove Object Instance while the stale timestamped
reflection is suppressed and post-delivery name lookup reports
\`ObjectInstanceNotKnown\`:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-delete-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update is suppressed after delete-then-divest resignation following restore" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update is suppressed after delete-then-divest resignation following restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-delete-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update is suppressed after delete-then-divest resignation following restore$" --output-on-failure

The matching four-member timed multi-recipient cancel-then-delete-then-divest
companion is source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_cancel_then_delete_then_divest_after_restore_catch2.cpp:218`
with 124 HLA_EVOKED assertions, 36 Requirements-Lab anchors, 19 canonical
2025 sections, and 25 selected official C++ API surfaces. It restores the
saved timestamp-8 explicit-source passel after source-region mutation, resigns
the producer with `CANCEL_THEN_DELETE_THEN_DIVEST`, and proves each constrained
recipient independently receives one delivery-boundary Remove Object Instance
with no stale timestamped reflection and an unknown post-delivery object name:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-cancel-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update is suppressed after cancel-then-delete-then-divest resignation following restore" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update is suppressed after cancel-then-delete-then-divest resignation following restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-cancel-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded timed multi-recipient regional timestamped attribute update is suppressed after cancel-then-delete-then-divest resignation following restore" --output-on-failure

The pending-ownership cancellation companion is source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_cancel_pending_ownership_after_restore_catch2.cpp:251`
with 130 HLA_EVOKED assertions, 39 Requirements-Lab anchors, 21 canonical
2025 sections, and 28 selected official C++ API surfaces. After restore, the
first constrained recipient publishes and queues a regular ownership
acquisition, then resigns with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`; its
queued owner-release callback and stale timestamped reflection are suppressed,
while the surviving recipient receives one saved reflection before its grant:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-cancel-pending-ownership-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending ownership acquisition after restore" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending ownership acquisition after restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-cancel-pending-ownership-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending ownership acquisition after restore$" --output-on-failure

The If Available pending-ownership cancellation companion is source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_if_available_cancel_pending_ownership_after_restore_catch2.cpp:251`
with 130 HLA_EVOKED assertions, 41 Requirements-Lab anchors, 22 canonical
2025 sections, and 29 selected official C++ API surfaces. After restore, the
first constrained recipient queues an If Available acquisition, creating only
private Willing-to-Acquire state; no owner-release callback is emitted. Its
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` resignation suppresses the stale
requester terminal callback and timestamped reflection, while the surviving
recipient receives one saved reflection before its grant:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-if-available-cancel-pending-ownership-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending If Available ownership acquisition after restore" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending If Available ownership acquisition after restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-if-available-cancel-pending-ownership-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending If Available ownership acquisition after restore$" --output-on-failure

The negotiated regular-candidate continuation companion is source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_lane_catch2.cpp:11`
with 164 HLA_EVOKED assertions, 46 Requirements-Lab anchors, 24 canonical
2025 sections, and 30 selected official C++ API surfaces. It queues a regular
acquisition on the first constrained recipient and a second regular candidate
on the independent clock, enters negotiated divestiture, consumes the first
candidate's stale path after `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, then
reissues negotiation for the retained candidate and completes it only after the
surviving recipient receives the saved reflection:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore" --summary --compact
    python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
    cmake --build <build-dir> --config Release --target umbra_tso_regional_regular_continuation_restore_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore$" --output-on-failure

The mixed If Available-to-regular continuation companion is source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_if_available_regular_candidate_continuation_after_restore_catch2.cpp:307`
with 163 HLA_EVOKED assertions, 48 Requirements-Lab anchors, 25 canonical
2025 sections, and 31 selected official C++ API surfaces. It queues an If
Available acquisition on the first constrained recipient and a regular
candidate on the independent clock, then verifies that requester cancellation
leaves the retained regular candidate available for reissued negotiation and
Confirm Divestiture after the surviving recipient receives its saved reflection:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-if-available-regular-candidate-continuation-state --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from an If Available request to a regular candidate after restore" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update continues from an If Available request to a regular candidate after restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-if-available-regular-candidate-continuation-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from an If Available request to a regular candidate after restore$" --output-on-failure

The mixed regular-to-If-Available continuation companion is source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore_catch2.cpp:15`
with 161 HLA_EVOKED assertions, 46 Requirements-Lab anchors, 24 canonical
2025 sections, and 31 selected official C++ API surfaces. It restores the
logical-time-8 explicit-source regional update, queues a regular candidate for
the first constrained recipient and an If Available candidate on the
independent clock, then reissues negotiated divestiture after the first
requester resigns. The registry retains the If Available reservation until
Confirm Divestiture, suppresses the stale ordinary callback, and delivers the
surviving regional reflections before the common Flush Queue grant:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to an If Available candidate after restore" --summary --compact
    python tools/query_rti_work.py matrix "tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore --summary --compact
    cmake --build <build-dir> --config Release --target umbra_tso_regional_mixed_continuation_restore_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to an If Available candidate after restore$" --output-on-failure

This is bounded development-profile evidence; alternate callback models,
passive/relaxed DDM, remote transport, package/JUnit/protected review,
Requirements-Lab validation, interoperability, and conformance remain separate.

The mixed If-Available-to-retained-regular pre-delivery cancellation companion
is source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore_catch2.cpp:15`
with 161 HLA_EVOKED assertions, 50 Requirements-Lab anchors, 26 canonical
2025 sections, and 32 selected official C++ API surfaces. It restores the
logical-time-8 explicit-source regional update, queues an If Available
candidate for the first constrained recipient and a regular candidate on the
independent clock, reissues negotiation after requester resignation, and
cancels before Request Divestiture Confirmation enters user code. Stale
confirmation work is consumed without an acquisition notification, ownership
remains with the publisher, and both surviving regional recipients reflect
before the common Flush Queue grant:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation before delivery after restore" --summary --compact
    python tools/query_rti_work.py matrix "tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore" --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore --summary --compact
    cmake --build <build-dir> --config Release --target umbra_tso_mixed_pre_delivery_cancel_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation before delivery after restore$" --output-on-failure

This is bounded development-profile evidence; alternate callback models,
passive/relaxed DDM, remote transport, package/JUnit/protected review,
Requirements-Lab validation, interoperability, and conformance remain separate.

The mixed If-Available-to-retained-regular confirmation-cancellation companion
is source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore_catch2.cpp:15`
with 163 HLA_EVOKED assertions, 50 Requirements-Lab anchors, 26 canonical
2025 sections, and 32 selected official C++ API surfaces. It delivers the
owner's confirmation, preserves the surviving regional reflection through the
common Flush Queue boundary, then cancels; publisher ownership remains, stale
Confirm Divestiture is rejected, and the regular reservation emits no
acquisition callback:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation after restore" --summary --compact
    python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
    cmake --build <build-dir> --config Release --target umbra_tso_mixed_confirmation_cancel_catch2
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation after restore$" --output-on-failure

This is bounded development-profile evidence; alternate callback models,
passive/relaxed DDM, remote transport, package/JUnit/protected review,
Requirements-Lab validation, interoperability, and conformance remain separate.

The Attribute Scope Advisory slice is source-backed at
`attribute_scope_advisory_catch2.cpp:97`. It passes 187 assertions across
HLA_EVOKED and HLA_IMMEDIATE, with nine Requirements-Lab anchors, five
canonical 2025 sections, and eleven selected official C++ API surfaces. The
case groups two attributes per transition and covers region, association,
ordinary/regional subscription, stale evoked, switch-gate, and default-region
boundaries:

    python tools/query_rti_work.py focus object-attribute-scope-advisory-state --summary --compact
    python tools/query_rti_work.py trace "Embedded regional object scope callbacks follow 2025 region, association, and subscription changes" --summary --compact
    python tools/query_rti_work.py matrix "Embedded regional object scope callbacks follow 2025 region, association, and subscription changes" --summary --compact
    python tools/query_rti_work.py check --lane object-attribute-scope-advisory-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded regional object scope callbacks follow 2025 region, association, and subscription changes$" --output-on-failure

FOM-composition lanes have an independent target so they do not wait on the
large federation-management translation unit. The current Annex C
directed-interaction guards are selected by the exact indexed query:

    python tools/query_rti_work.py focus schema-conflict --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_fom_composer_catch2
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.fom_composer\\.catch2\\.(The FDD materializer surfaces the multiple-directed-class schema conflict|The FDD materializer refuses the supplied extension when the official FDD schema cannot represent its directed-interaction merge)$" --output-on-failure

The exact trace joins each guard to its source line, RL-081 requirement, and
canonical IEEE 1516.2-2025 subsection; it does not imply runtime selector or
conformance coverage.

The active Annex C/reference-resolution pointer is the eight-case FOM-composer
lane. `focus reference-resolution --summary --compact` prints the exact CTest
regex; its current baseline is the transportation-name reference guard and
its trace maps to 2025 clauses 4.11.2, 6.2.5, 6.2.6, and Annex C. Run the
bounded lane only when all eight mapped guards are needed; the single baseline
CTest title from `work annex-c-and-reference-resolution --summary --compact`
keeps iteration to one test; copy the exact lane regex from `focus` when all
eight are required.

The adjacent support-switch composition guard is queryable independently with:

    python tools/query_rti_work.py trace "The FDD materializer retains the complete 2025 support-switch table" --summary --compact

and runs as the exact CTest title
`umbra.fom_composer.catch2.The FDD materializer retains the complete 2025 support-switch table`.

The declaration-management reproducibility baseline is directly traceable
without opening the full FOM-composer file:

    python tools/query_rti_work.py trace "The FDD materializer is repeatable for a fixed official module set" --summary --compact
    .build\Debug\umbra_fom_composer_catch2.exe "The FDD materializer is repeatable for a fixed official module set" --reporter compact

Its mapping is intentionally private reproducibility evidence anchored to the
existing FDD materialization candidates (IEEE 1516.2 clauses 4.14.2 and C.1).

The declaration-management lane's Annex C/reference-resolution cases are now
all indexed and source-backed; its FOM-composer reconciliation queue is
exhausted. Keep those exact `trace` examples as historical evidence and use
`python tools/query_rti_work.py next --pointer` for the single global
unplanned-source pointer instead of replaying this sequence or rescanning the
file.

For requirement-driven work, start with the bounded roadmap selector before
opening this directory:

    python tools/query_rti_work.py next --summary

The focused Connection Lost/TSO directed-interaction case is independently
queryable and runnable; its 55 assertions map to the two page-50 Clause 4
requirements and the canonical `hla-1516.1-2025:clause-4` section:

    python tools/query_rti_work.py trace "Embedded transport loss delivers timestamped directed interactions through the lost federate's last-known time" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss delivers timestamped directed interactions through the lost federate's last-known time" --output-on-failure
    python tools/query_rti_work.py lane <exact-catch2-tag> --compact
    python tools/query_rti_work.py test "<exact TEST_CASE title>" --compact
    python tools/query_rti_work.py requirement <lab-id-or-clause> --summary
    python tools/query_rti_work.py section <document-id:clause-id> --summary
    python tools/query_rti_work.py source cpp/tests/external_2010_fom_catch2.cpp --summary --limit 20
    python tools/query_rti_work.py check --lane <exact-catch2-tag> --compact
    python tools/query_rti_work.py unplanned --path <source-file> --summary

The paired timestamped attribute-update cutoff case is independently queryable
and runnable; its 53 assertions map to the page-50 loss requirements, the
Clause 6/8 Update/Reflect and TSO requirements, and the indexed order-control
surface:

    python tools/query_rti_work.py trace "Embedded transport loss delivers timestamped attribute updates through the lost federate's last-known time" --summary --compact
    python tools/query_rti_work.py matrix "Embedded transport loss delivers timestamped attribute updates through the lost federate's last-known time" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss delivers timestamped attribute updates through the lost federate's last-known time" --output-on-failure
    ctest --test-dir <build-dir> -C Debug -L connection-lost-tso-cutoff --output-on-failure

The two-survivor companion uses the same lane and proves independent queued
copies when the second TAR is submitted after the source fault:

    python tools/query_rti_work.py trace "Embedded transport loss drains a cutoff timestamped attribute update to each pending survivor" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss drains a cutoff timestamped attribute update to each pending survivor" --output-on-failure

The directed-selector mutation companion uses the same lane and proves that a
timestamped directed interaction admitted under a universal subscription is
suppressed when the survivor changes to by-ownership before its callback
boundary, while the independent receive-order `DELETE_OBJECTS` cleanup still
arrives at the next gate:

    python tools/query_rti_work.py trace "Embedded transport loss rechecks a directed ownership selector before automatic cleanup" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss rechecks a directed ownership selector before automatic cleanup" --output-on-failure

The regional-selector mutation companion uses the same lane and proves that a
timestamped regional attribute update admitted while ranges overlap is
suppressed after the survivor commits a disjoint range before its callback
boundary, while the independent receive-order `DELETE_OBJECTS` cleanup still
arrives at the next gate:

    python tools/query_rti_work.py trace "Embedded transport loss rechecks a regional selector before automatic cleanup" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss rechecks a regional selector before automatic cleanup" --output-on-failure

The late-TAR automatic-delete companion faults before the survivor requests
time 6, then checks reflection-before-grant and deferred receive-order removal:

    python tools/query_rti_work.py trace "Embedded transport loss protects a late cutoff attribute reflection from automatic object cleanup" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss protects a late cutoff attribute reflection from automatic object cleanup" --output-on-failure

The directed-interaction late-TAR automatic-delete companion faults before the
survivor requests time 6, then checks directed-interaction-before-grant and
deferred receive-order removal:

    python tools/query_rti_work.py trace "Embedded transport loss protects a late cutoff directed interaction from automatic object cleanup" --summary --compact
    python tools/query_rti_work.py matrix "Embedded transport loss protects a late cutoff directed interaction from automatic object cleanup" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss protects a late cutoff directed interaction from automatic object cleanup" --output-on-failure

The per-recipient automatic-cleanup companion uses two survivors and proves
that each later TAR releases only its own DELETE_OBJECTS removal:

    python tools/query_rti_work.py trace "Embedded transport loss drains automatic cleanup independently after cutoff attribute updates" --summary --compact
    python tools/query_rti_work.py matrix "Embedded transport loss drains automatic cleanup independently after cutoff attribute updates" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss drains automatic cleanup independently after cutoff attribute updates" --output-on-failure

The HLA_IMMEDIATE asynchronous-delivery counterpart runs the same cutoff
collision on the direct callback surface and checks reflection, grant, and
receive-order removal in that TAR:

    python tools/query_rti_work.py trace "Embedded immediate transport loss releases automatic cleanup after cutoff reflection under asynchronous delivery" --summary --compact
    python tools/query_rti_work.py matrix "Embedded immediate transport loss releases automatic cleanup after cutoff reflection under asynchronous delivery" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded immediate transport loss releases automatic cleanup after cutoff reflection under asynchronous delivery" --output-on-failure

The timestamped object-deletion cutoff case queues a Delete Object Instance at
time 6, faults the regulating publisher at that cutoff, and checks the
timestamped removal (with retraction metadata) before the survivor's grant:

    python tools/query_rti_work.py trace "Embedded transport loss delivers timestamped object deletion through the lost federate's last-known time" --summary --compact
    python tools/query_rti_work.py matrix "Embedded transport loss delivers timestamped object deletion through the lost federate's last-known time" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss delivers timestamped object deletion through the lost federate's last-known time" --output-on-failure

The strict-less-than companion shares the same source file and queues its
timestamped removal at time 5 before the lost regulator reaches time 6:

    python tools/query_rti_work.py trace "Embedded transport loss delivers timestamped object deletion before the lost federate's last-known time" --summary --compact
    python tools/query_rti_work.py matrix "Embedded transport loss delivers timestamped object deletion before the lost federate's last-known time" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss delivers timestamped object deletion before the lost federate's last-known time" --output-on-failure

The automatic-delete collision companion shares that source file and verifies
that DELETE_OBJECTS cleanup does not replace or duplicate an accepted cutoff
timestamped removal:

    python tools/query_rti_work.py trace "Embedded transport loss preserves a cutoff timestamped deletion across automatic delete cleanup" --summary --compact
    python tools/query_rti_work.py matrix "Embedded transport loss preserves a cutoff timestamped deletion across automatic delete cleanup" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss preserves a cutoff timestamped deletion across automatic delete cleanup" --output-on-failure

The multi-recipient object-deletion companion uses two constrained survivors;
the second requests the cutoff after loss so each pending recipient gets its
own accepted removal:

    python tools/query_rti_work.py trace "Embedded transport loss drains a cutoff timestamped deletion to each pending survivor" --summary --compact
    python tools/query_rti_work.py matrix "Embedded transport loss drains a cutoff timestamped deletion to each pending survivor" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded transport loss drains a cutoff timestamped deletion to each pending survivor" --output-on-failure

The exact test query resolves the C++ source line, selected Requirements-Lab
IDs, and canonical IEEE 1516.1-2025 clause/subsection keys. Use
`check --lane <exact-catch2-tag> --compact` as the iteration-local integrity
gate; it keeps unrelated historical source drift out of the active slice while
still listing unlocated rows. Use unscoped `check --compact` for a bounded
whole-plan drift sample and `check --json` when a tool or review needs the
complete mapping diagnostic.

`unplanned` is the inverse source check: it lists C++ `TEST_CASE` declarations
that are not exact plan rows, optionally narrowed to one source path. It is a
reconciliation queue only; it does not assign requirements or implementation
status.

The current process-boundary transport starting point is directly queryable:

    python tools/query_rti_work.py test "Private process transport exchanges framed data after endpoint handshake" --summary --compact
    .build-fom-services\Debug\umbra_ieee1516_2025_catch2.exe "Private process transport exchanges framed data after endpoint handshake" --reporter compact

The registry-bound process service slice is a bounded regression baseline:

    python tools/query_rti_work.py test "Private process service binds create join and receive-order interaction to the federation registry" --summary --compact
    .build-fom-services\Debug\umbra_ieee1516_2025_catch2.exe "Private process service binds create join and receive-order interaction to the federation registry" --reporter compact

The process additional-FOM Join slice is independently traceable and runnable:

    python tools/query_rti_work.py trace "Private process service composes additional FOM modules during Join Federation Execution" --summary --compact
    python tools/query_rti_work.py matrix "Private process service composes additional FOM modules during Join Federation Execution" --summary --compact
    .build-fom-services\Debug\umbra_process_boundary_private_catch2.exe "Private process service composes additional FOM modules during Join Federation Execution" --reporter compact

The matching process error boundary is independently queryable as well:

    python tools/query_rti_work.py trace "Private process service rejects an invalid additional FOM without mutating the execution" --summary --compact
    python tools/query_rti_work.py matrix "Private process service rejects an invalid additional FOM without mutating the execution" --summary --compact
    .build-fom-services\Debug\umbra_process_boundary_private_catch2.exe "Private process service rejects an invalid additional FOM without mutating the execution" --reporter compact

The independently launched process baseline is also directly queryable:

    python tools/query_rti_work.py test "Private registry-bound service exchanges federation traffic across independently launched processes" --summary --compact
    python tools/query_rti_work.py lane process-boundary --summary --compact
    ctest --test-dir .build-fom-services -C Debug -L process-boundary --output-on-failure
    ctest --test-dir .build-fom-services\package-smoke-consumer -C Debug -L package-process-connection-loss --output-on-failure
    .build-fom-services\Debug\umbra_ieee1516_2025_catch2.exe "Private registry-bound service exchanges federation traffic across independently launched processes" --reporter compact

It launches separate server, sender, and receiver helpers. The sender and
receiver use the private `ProcessFederationClient` seam; the receiver consumes
a pushed event frame and exercises the official C++
`FederateAmbassador::receiveInteraction` callback through the private bridge.
The public process-address/message slice is also directly queryable:

    python tools/query_rti_work.py test "RTIambassador routes public Create, Join, and Resign through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador resolves interaction and parameter handles through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes ordinary interaction declarations through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py lane callback-controls --summary --compact
    python tools/query_rti_work.py test "RTIambassador receives a process interaction through the official Evoke callback surface" --summary --compact
    python tools/query_rti_work.py test "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface" --summary --compact
    python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
    python tools/query_rti_work.py test "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
    python tools/query_rti_work.py trace "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
    python tools/query_rti_work.py test "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
    python tools/query_rti_work.py trace "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py trace "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py trace "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador preserves a timestamped regional update through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py trace "RTIambassador preserves a timestamped regional update through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py trace "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public Update Attribute Values through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador delivers a process Update Attribute Values event through the official Reflect callback" --summary --compact
    python tools/query_rti_work.py test "Embedded transport loss applies the bounded automatic NoAction forced-resign policy" --summary --compact
    python tools/query_rti_work.py test "RTIambassador selects a configured tcp process endpoint through the official address field" --summary --compact
    python tools/query_rti_work.py test "RTIambassador rejects malformed tcp process addresses before connecting" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public object-class attribute subscription through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes public object-class attribute subscription through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers object discovery through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers object discovery through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories use subscriptions when known-class policy is disabled" --summary --compact
python tools/query_rti_work.py search known-class-disabled --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories honor known class when the static policy is enabled" --summary --compact
python tools/query_rti_work.py search known-class-enabled --summary --compact
python tools/query_rti_work.py trace "Embedded update-rate lookup ignores passive regional subscriptions" --summary --compact
python tools/query_rti_work.py focus update-rate-passive-regional-subscription --summary --compact
python tools/query_rti_work.py trace "Embedded mixed update-rate subscriptions gate each attribute independently" --summary --compact
python tools/query_rti_work.py focus update-rate-mixed-attribute-gating --summary --compact
python tools/query_rti_work.py trace "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
python tools/query_rti_work.py search transportation-handle-stability --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped Update Attribute Values invocations through MOM interaction (restored baseline copy)" --summary --compact
python tools/query_rti_work.py search restored-baseline-copy --summary --compact
python tools/query_rti_work.py trace "Embedded regional Provide Attribute Value Update reports before callback delivery (restored baseline copy)" --summary --compact
python tools/query_rti_work.py trace "Embedded three-dimensional regional object attributes require complete overlap" --summary --compact
python tools/query_rti_work.py search complete-overlap --summary --compact
python tools/query_rti_work.py trace "Embedded regional Request Attribute Value Update filters 2025 owner solicitations (restored baseline copy)" --summary --compact
python tools/query_rti_work.py search regional-request-filtering --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped regional Update Attribute Values invocations through MOM interaction (restored baseline copy)" --summary --compact
python tools/query_rti_work.py search timestamped-regional-attribute-update-failure --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records failed timestamped Update Attribute Values invocations (restored baseline copy)" --summary --compact
python tools/query_rti_work.py search timestamped-attribute-update-failure --summary --compact

The public object-discovery process-endpoint case is now implemented and indexed
with 44 assertions under both callback models, three exact requirement/section
mappings, and six official API surfaces. The external IEEE 1516-2010 ordered-
catalog compatibility case is indexed with 13 assertions and clauses `4.5.5`/
`4.11.4`, explicitly separate from 2025 service conformance. The connection
support-types baseline
is also indexed as an 11-assertion SDK-consumability case; its empty Lab/API
mapping is intentional because these support declarations are not standalone
Requirements-Lab surfaces. The callback-route receive-order, immediate-callback
reentrancy, callback-disable, evoked one-at-a-time, concurrent-serialization,
disabled-backlog, Evoke Multiple FIFO, evoked-disabled-pending, callback-session-close,
callback-session concurrent-invocation, and callback-session active-close cases are indexed
as private foundation/API traceability rows. The strict 2025-mode rejection of
an IEEE 1516-2010 module is now indexed as two-assertion compatibility-only
evidence. The mixed-edition composer rejection is also indexed as a
seven-assertion compatibility-only row. The ambiguous 202x edition-setting
guard is indexed as a one-assertion explicit no-standalone-Lab-surface policy
row. The RPR full-family object-registration and custom-payload compatibility
slices are now indexed with explicit 2010 compatibility dispositions. The
regional-unpublish update-region lifetime slice is indexed with 17 assertions
and 18 requirement anchors. The receive-order deletion update-region lifetime
slice is indexed with 17 assertions and 20 requirement anchors. The final
object-removal callback update-region lifetime slice is indexed with 27
assertions and 21 requirement anchors. The Join advisory-switch seed slice is
indexed with 16 assertions and 13 requirement anchors. The Join-time explicit
NoAction automatic-resign slice is indexed with 7 assertions and 9 requirement
anchors. The order and transportation MOM service-classification slice is
indexed with 66 assertions and 19 requirement anchors. The joined-federate MOM
regional discovery slice is now indexed with 124 assertions and 17 requirement
anchors. The custom-transportation handle-stability slice and the restored-
baseline timestamped MOM failure slice are now mapped and green; the next
source pointer is the unplanned federation-management regional Provide
Attribute Value Update provider-response case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4697` in
`[federation-management]`. Query it with `next --pointer` or
`unplanned --path cpp/tests/ieee1516_2025_federation_management_catch2.cpp --summary --limit 1`,
then map the provider-response and regional-overlap contract deliberately
before adding a plan row.
The joined-federate MOM/FOM-module snapshot slice is indexed with 12 assertions
and 6 requirement anchors; the report-file identity save/restore slice is
indexed with 46 assertions and 18 requirement anchors. The restored-baseline
regional provider-response slice is indexed with 44 assertions and 20
requirement anchors. The dedicated explicit-source regional TSO restore slice
is now source-backed and green with 78 assertions, 14 Requirements-Lab anchors,
and 13 canonical 2025 sections:

    python tools/query_rti_work.py trace "Embedded federation restore restores a saved live explicit-source regional timestamped attribute update" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded federation restore restores a saved live explicit-source regional timestamped attribute update" --output-on-failure

The parallel explicit-source regional timestamped-interaction restore slice is
source-backed and green with 74 HLA_EVOKED assertions, 11 Requirements-Lab
anchors, 11 canonical 2025 sections, and 17 official C++ API surfaces. It
saves a queued overlap-qualified interaction, restores its retraction ledger,
and proves the original source RegionHandle set arrives before Flush Queue
Grant followed by one legal Request Retraction:

    python tools/query_rti_work.py focus timestamped-regional-interaction-live-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded federation restore restores a saved live timestamped regional interaction" --summary --compact
    ctest --test-dir <build-dir> -C Debug -L timestamped-regional-interaction-live-restore --output-on-failure

The non-regional multi-recipient timestamped-interaction restore companion is
source-backed and green with 116 HLA_EVOKED assertions, 11 Requirements-Lab
anchors, 10 canonical 2025 sections, and 20 official C++ API surfaces. It
saves one queued interaction for two constrained recipients, restores both
independent recipient/retraction entries, delivers each copy through its own
Flush Queue Grant, and verifies one legal Request Retraction per recipient:

    python tools/query_rti_work.py focus timestamped-interaction-restore-multi-recipient --summary --compact
    python tools/query_rti_work.py trace "Embedded federation restore restores one queued timestamped interaction to multiple recipients" --summary --compact
    ctest --test-dir <build-dir> -C Debug -L timestamped-interaction-restore-multi-recipient --output-on-failure

Its three-federate source-resignation companion is source-backed and green with
91 HLA_EVOKED assertions, 20 Requirements-Lab anchors, 18 canonical 2025
sections, and 23 official C++ API surfaces. It keeps the saved explicit-source
passel through source-region mutation, restore, unconditional-divestiture
resignation, and survivor Flush Queue delivery:

    python tools/query_rti_work.py trace "Embedded live regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded live regional timestamped attribute update survives source mutation and resignation after restore" --output-on-failure

The timed default-region interaction companion is source-backed and green with
72 HLA_EVOKED assertions, 13 Requirements-Lab anchors, 10 canonical 2025
sections, and 24 official C++ API surfaces. It saves at logical time 6 while a
timestamp-8 default-source interaction remains queued, restores the live
passel, and proves supplied-empty region metadata plus post-restore Request
Retraction:

    python tools/query_rti_work.py trace "Embedded timed federation restore restores a live timestamped default-region interaction at the save boundary" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded timed federation restore restores a live timestamped default-region interaction at the save boundary" --output-on-failure
    ctest --test-dir <build-dir> -C Debug -L timestamped-default-region-interaction-timed-restore --output-on-failure

The timed directed-interaction companion is source-backed and green with 72
HLA_EVOKED assertions, 10 Requirements-Lab anchors, 8 canonical 2025 sections,
and 21 official C++ API surfaces. It saves at logical time 6 while a
target-qualified timestamp-8 directed interaction remains queued, restores the
target/retraction ledger, and proves Flush Queue delivery before the grant plus
post-restore Request Retraction:

    python tools/query_rti_work.py trace "Embedded timed federation restore restores a live timestamped directed interaction at the save boundary" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded timed federation restore restores a live timestamped directed interaction at the save boundary" --output-on-failure
    ctest --test-dir <build-dir> -C Debug -L timestamped-directed-interaction-timed-restore --output-on-failure

The timed default-region attribute-update companion is source-backed and green
with 76 HLA_EVOKED assertions, 16 Requirements-Lab anchors, 10 canonical 2025
sections, and 23 official C++ API surfaces. It saves at logical time 6 while a
default-source timestamp-8 Update Attribute Values passel remains queued,
restores the retraction ledger, and proves supplied-empty sent-region metadata
plus Flush Queue reflection-before-grant delivery:

    python tools/query_rti_work.py trace "Embedded timed federation restore restores a live timestamped default-region attribute update at the save boundary" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded timed federation restore restores a live timestamped default-region attribute update at the save boundary" --output-on-failure
    ctest --test-dir <build-dir> -C Debug -L timestamped-default-region-attribute-timed-restore --output-on-failure

The untimed default-region attribute-update companion is source-backed and green
with 74 HLA_EVOKED assertions, 18 Requirements-Lab anchors, 14 canonical 2025
sections, and 23 official C++ API surfaces. It saves a queued timestamp-7
default-source Update Attribute Values passel, restores the retraction ledger,
and proves supplied-empty sent-region metadata plus Flush Queue reflection-
before-grant delivery:

    python tools/query_rti_work.py trace "Embedded federation restore restores a saved live timestamped default-region attribute update" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded federation restore restores a saved live timestamped default-region attribute update" --output-on-failure
    ctest --test-dir <build-dir> -C Debug -L timestamped-default-region-attribute-restore --output-on-failure

The non-regional timestamped attribute-update restore companion is source-backed
and green with 69 HLA_EVOKED assertions, 15 Requirements-Lab anchors, 11
canonical 2025 sections, and 20 official C++ API surfaces. It uses ordinary
subscription plus `Change Default Attribute Order Type` and proves the untimed
save/restore recipient ledger, reflection-before-grant delivery, and later
Request Retraction:

    python tools/query_rti_work.py trace "Embedded federation restore restores a saved live timestamped attribute update" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "Embedded federation restore restores a saved live timestamped attribute update" --output-on-failure
    ctest --test-dir <build-dir> -C Debug -L timestamped-attribute-update-restore --output-on-failure

The Auto Provide service-report boundary is a separate fast lane, not another
large-file scan. Its source-backed case is at
`cpp/tests/auto_provide_service_report_file_catch2.cpp:166` with 142
HLA_EVOKED assertions, five Requirements-Lab anchors, and four canonical 2025
sections (`1`, `6.1.10`, `11.5`, `11.5.2`), plus six official C++ API surfaces.
It uses the production filesystem store and proves the serial-0 type-37/type-1/type-63 successful-void record
with an empty tag immediately before callback delivery. Resolve it by exact
title, then run the lane gate. The stable lane contains two mapped cases and
143 aggregate assertions because the one-assertion formatter unit shares its
tag:

    python tools/query_rti_work.py trace "Embedded Auto Provide service reporting records empty tag before its callback" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Auto Provide service reporting records empty tag before its callback" --summary --compact
    python tools/query_rti_work.py section hla-1516.1-2025:clause-11.5.2 --summary --limit 20
    python tools/query_rti_work.py focus auto-provide-service-report --summary --compact
    python tools/query_rti_work.py check --lane auto-provide-service-report --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.(Embedded Auto Provide service reporting records empty tag before its callback|MOM service-report files preserve the empty Auto Provide tag descriptor form)$" --output-on-failure

The federation-wide MOM `HLAsetSwitches` Auto Provide mutation is its own
standalone C++ lane at
`cpp/tests/auto_provide_mom_catch2.cpp:120`. It passes 41 HLA_EVOKED
assertions, exercises 18 official C++ API surfaces, and is explicitly anchored
to canonical 2025 clauses `4`, `6.1.10`, and `11.4.1`. The Lab has no
standalone row-level candidate for this Table 20 parameter (RL-032), so the
case is intentionally an explicit development-profile disposition rather than
an invented requirement mapping:

    python tools/query_rti_work.py trace m90.embedded-mom-hlasetswitches-auto-provide --summary --compact
    python tools/query_rti_work.py matrix mom-auto-provide-switch-mutation --summary --compact
    python tools/query_rti_work.py focus mom-auto-provide-switch-mutation --summary --compact
    python tools/query_rti_work.py check --lane mom-auto-provide-switch-mutation --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_auto_provide_mom_catch2
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.auto_provide_mom\\.catch2\\.Embedded MOM HLAsetSwitches adjusts federation-wide Auto Provide$" --output-on-failure

The service-report writer-creation failure boundary is a separate focused
lane at `cpp/tests/service_report_writer_failure_catch2.cpp:59`. It passes 10
HLA_EVOKED assertions, maps one Requirements-Lab candidate to canonical 2025
clause `11.5.2`, and exercises six official federation-management C++ API
surfaces. The test injects the internal test-only store seam, proves
`RTIinternalError` with no `memory://` fallback, and demonstrates join
membership rollback by reusing the rejected federate name. This is bounded
development-profile evidence; production permission/full-disk, cross-process,
package/JUnit/protected-review, validation, interoperability, and conformance
remain separate:

    python tools/query_rti_work.py trace m91.embedded-service-report-writer-creation-failure --summary --compact
    python tools/query_rti_work.py matrix service-report-writer-failure --summary --compact
    python tools/query_rti_work.py focus service-report-writer-failure --summary --compact
    python tools/query_rti_work.py check --lane service-report-writer-failure --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_service_report_writer_failure_catch2
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.service_report_writer_failure\\.catch2\\.Service-report writer creation failure rejects the join without an in-memory fallback$" --output-on-failure

The subscription-generation restore case is a separate focused lane at
`cpp/tests/libxml2_fom_composer_catch2.cpp:1742`. It passes 31 native C++
unit assertions, maps the §4.32 save/restore candidate to canonical 2025
clause `4.32`, and records the two official restore API surfaces. The case
restores declaration state and proves the next subscription mutation consumes
the saved generation identity; it remains development-profile evidence:

    python tools/query_rti_work.py trace m92.federation-registry-subscription-generation-restore --summary --compact
    python tools/query_rti_work.py matrix subscription-generation --summary --compact
    python tools/query_rti_work.py focus subscription-generation --summary --compact
    python tools/query_rti_work.py check --lane subscription-generation --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_fom_composer_catch2
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.fom_composer\\.catch2\\.The federation registry restores subscription-generation allocation with declaration state$" --output-on-failure

The enabled Auto Provide baseline is a separate focused lane at
`cpp/tests/auto_provide_baseline_catch2.cpp:84`. It passes 28 HLA_EVOKED
assertions and maps three Requirements-Lab anchors to two canonical 2025
sections and 15 official C++ API surfaces. The case verifies the enabled FDD switch, discovery, one grouped
in-scope owner solicitation for both attributes, and the required empty
RTI-invoked tag. Keep Disabled and MOM-mutation behavior in their separate
lanes:

    python tools/query_rti_work.py focus auto-provide-baseline-state --summary --compact
    python tools/query_rti_work.py trace "Embedded Auto Provide solicits in-scope owners after discovery" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Auto Provide solicits in-scope owners after discovery" --summary --compact
    python tools/query_rti_work.py check --lane auto-provide-baseline-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded Auto Provide solicits in-scope owners after discovery$" --output-on-failure

The explicit object-instance Request Attribute Value Update baseline is an
independent C++ lane at
`cpp/tests/attribute_value_update_request_baseline_catch2.cpp:93`. It passes
36 HLA_EVOKED assertions, maps eight Requirements-Lab anchors to three
canonical 2025 sections (`6.21`, `6.21.5`, `6.22`), and exercises 15 official
C++ API surfaces. It verifies known-instance targeting, grouped current-owner
solicitation, unowned/requester-owned suppression, request-tag propagation,
and evoked callback delivery:

    python tools/query_rti_work.py focus attribute-value-update-request-baseline-state --summary --compact
    python tools/query_rti_work.py trace "Embedded object-instance Request Attribute Value Update solicits 2025 owners" --summary --compact
    python tools/query_rti_work.py matrix "Embedded object-instance Request Attribute Value Update solicits 2025 owners" --summary --compact
    python tools/query_rti_work.py check --lane attribute-value-update-request-baseline-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded object-instance Request Attribute Value Update solicits 2025 owners$" --output-on-failure

Keep the object-class overload, regional requests, automatic provision,
timestamped/retraction behavior, response delivery, and broader DDM,
ownership, save/restore, and packaging work in their own lanes.

The nonregional object-class Request Attribute Value Update baseline is an
independent C++ lane at
`cpp/tests/object_class_attribute_value_update_request_baseline_catch2.cpp:94`.
It passes 43 HLA_EVOKED assertions, maps seven Requirements-Lab anchors to
three canonical 2025 sections (`6.21`, `6.21.5`, `6.22`), and exercises 15
official C++ API surfaces. It verifies class-designator expansion over two
concrete subclass instances, per-instance grouped owner callbacks,
unowned/requester-owned suppression, request-tag propagation, and evoked
delivery:

    python tools/query_rti_work.py focus object-class-attribute-value-update-request-baseline-state --summary --compact
    python tools/query_rti_work.py trace "Embedded object-class Request Attribute Value Update solicits 2025 subclass owners" --summary --compact
    python tools/query_rti_work.py matrix "Embedded object-class Request Attribute Value Update solicits 2025 subclass owners" --summary --compact
    python tools/query_rti_work.py check --lane object-class-attribute-value-update-request-baseline-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded object-class Request Attribute Value Update solicits 2025 subclass owners$" --output-on-failure

Keep regional class requests, automatic provision, timestamped/retraction
behavior, response delivery, and broader DDM, ownership, save/restore, and
packaging work in their own lanes.

The class-designator service-report companion is an independent filesystem
lane at
`cpp/tests/object_class_attribute_value_update_service_report_catch2.cpp:196`.
It passes 403 HLA_EVOKED assertions, maps six Requirements-Lab anchors to five
canonical 2025 sections (`6.21`, `6.21.5`, `6.22`, `11.5`, and `11.5.2`), and
exercises 19 official C++ API surfaces. It verifies one immutable report file
per joined federate, provider-file durability before each callback, serial-
ordered Table 5 type-37/type-1/type-63 records, propagated base64 tag bytes,
and an unchanged requester file. This is development-profile evidence only:

    python tools/query_rti_work.py focus object-class-provide-attribute-value-update-service-report --summary --compact
    python tools/query_rti_work.py trace "Embedded class Request Attribute Value Update reports each provider callback before delivery" --summary --compact
    python tools/query_rti_work.py matrix "Embedded class Request Attribute Value Update reports each provider callback before delivery" --summary --compact
    python tools/query_rti_work.py check --lane object-class-provide-attribute-value-update-service-report --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded class Request Attribute Value Update reports each provider callback before delivery$" --output-on-failure

Keep object-instance reporting, request-argument preservation, regional and
automatic-provision variants, timestamped/retraction behavior, public MOM,
packaging, validation, and conformance in separate lanes.

The Request Attribute Value Update argument-preservation case is an independent
filesystem lane at
`cpp/tests/request_attribute_value_update_service_report_catch2.cpp:204`.
It passes 242 HLA_EVOKED assertions, maps four Requirements-Lab anchors to four
canonical 2025 sections (`6.21`, `6.21.5`, `11.5`, and `11.5.2.1`), and covers
the object-instance/class request overloads plus the provider callback. It
proves unknown-object rejection is file-silent, then preserves type-37 or
type-36 designators, the complete type-1 attribute set, and type-63 base64 tags
before each queued callback:

    python tools/query_rti_work.py focus request-attribute-value-update-service-report-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting preserves Request Attribute Value Update arguments" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting preserves Request Attribute Value Update arguments" --summary --compact
    python tools/query_rti_work.py check --lane request-attribute-value-update-service-report-file --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting preserves Request Attribute Value Update arguments$" --output-on-failure

Keep regional requests, automatic provision, response delivery, generic
failure/return records, public MOM interaction, timestamped/retraction,
packaging, validation, and conformance in separate lanes.

The provider-side companion is a standalone filesystem-backed C++ lane at
`cpp/tests/provide_attribute_value_update_service_report_catch2.cpp:201`.
It passes 145 HLA_EVOKED assertions, maps six Requirements-Lab anchors to five
canonical 2025 sections (`6.21`, `6.21.5`, `6.22`, `11.5`, and `11.5.2`), and
records five official request/provider API surfaces. It proves the owner file
is unchanged while HLA_EVOKED work is pending, then appends one serial-0
successful-void Table 5 type-37/type-1/type-63 record at callback entry; the
requester file remains unchanged:

    python tools/query_rti_work.py focus provide-attribute-value-update-service-report-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records Provide Attribute Value Update before its callback" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records Provide Attribute Value Update before its callback" --summary --compact
    python tools/query_rti_work.py check --lane provide-attribute-value-update-service-report-file --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting records Provide Attribute Value Update before its callback$" --output-on-failure

Keep class, regional, automatic-provision, response, public MOM interaction,
timestamped/retraction, packaging, validation, and conformance in separate
lanes.

The receive-order `Update Attribute Values` service-report companion is an
independent filesystem lane at
`cpp/tests/update_attribute_values_service_report_catch2.cpp:224`. It passes
157 HLA_EVOKED assertions, maps four Requirements-Lab anchors to three
canonical 2025 sections (`6.10`, `11.5`, and `11.5.2.1`), and covers the two
official Update/Reflect Attribute Values API surfaces. It proves unknown-object
rejection is file-silent, then preserves the accepted type-37 object designator,
type-2 AttributeHandleValueMap, type-63 base64 tag, and type-34 Null optional
timestamp before reflection; the receiver file remains unchanged:

    python tools/query_rti_work.py focus update-attribute-values-service-report-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting preserves receive-order Update Attribute Values arguments" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting preserves receive-order Update Attribute Values arguments" --summary --compact
    python tools/query_rti_work.py check --lane update-attribute-values-service-report-file --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting preserves receive-order Update Attribute Values arguments$" --output-on-failure

Keep timestamped/regional/DDM forms, return/failure records, interaction-
selected delivery, public MOM, packaging, validation, and conformance in
separate lanes.

The receive-order `Delete Object Instance` service-report companion is an
independent filesystem lane at
`cpp/tests/delete_object_instance_service_report_catch2.cpp:194`. It passes
106 HLA_EVOKED assertions, maps five Requirements-Lab anchors to four
canonical 2025 sections (`6.16`, `6.16.4`, `11.5`, and `11.5.2.1`), and covers
the two official Delete/Remove Object Instance API surfaces. It proves
unknown-object deletion is file-silent, then preserves the accepted type-37
object designator, type-63 base64 tag, and type-34 Null optional timestamp
before removal; the receiver file remains unchanged:

    python tools/query_rti_work.py focus delete-object-instance-service-report-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting preserves receive-order Delete Object Instance arguments" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting preserves receive-order Delete Object Instance arguments" --summary --compact
    python tools/query_rti_work.py check --lane delete-object-instance-service-report-file --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting preserves receive-order Delete Object Instance arguments$" --output-on-failure

Keep timestamped/regional/DDM forms, return/failure records, interaction-
selected delivery, public MOM, packaging, validation, and conformance in
separate lanes.

The receive-order `Delete Object Instance` failure-file matrix is an
independent source-backed lane at
`cpp/tests/delete_object_instance_failure_service_report_catch2.cpp:135`. It
passes 137 HLA_EVOKED assertions, maps four Requirements-Lab anchors to three
canonical 2025 sections (`6.16`, `6.16.4`, and `11.5`), and covers the Delete
Object Instance service plus both service-report switch accessors. It verifies
failed serial-0 and serial-2 records around accepted serial-1 deletion, with
type-37/type-63/type-34 supplied forms, Null return, false indicators, and
exact exception text:

    python tools/query_rti_work.py focus delete-object-instance-failure-file-matrix --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records failed receive-order Delete Object Instance invocations" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records failed receive-order Delete Object Instance invocations" --summary --compact
    python tools/query_rti_work.py check --lane delete-object-instance-failure-file-matrix --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting records failed receive-order Delete Object Instance invocations$" --output-on-failure

Keep timestamped failure, regional/DDM, MOM-interaction, packaging, validation,
and conformance evidence separate.

The receive-order `Delete Object Instance` MOM failure matrix is an
independent source-backed lane at
`cpp/tests/delete_object_instance_failure_service_report_interaction_catch2.cpp:84`.
It passes 124 HLA_IMMEDIATE assertions, maps four Requirements-Lab anchors to
three canonical 2025 sections (`6.16`, `6.16.4`, and `11.5`), and decodes
object-management service type 2 through `HLAreportServiceInvocation`. It
verifies failed serial-0 and serial-2 reports around accepted serial-1 deletion,
with type-37/type-63/type-34 supplied forms, Null return, false indicators, and
exact exception text:

    python tools/query_rti_work.py focus delete-object-instance-failure-mom-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers failed receive-order Delete Object Instance invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting delivers failed receive-order Delete Object Instance invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py check --lane delete-object-instance-failure-mom-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting delivers failed receive-order Delete Object Instance invocations through MOM interaction$" --output-on-failure

Keep filesystem, timestamped, regional/DDM, packaging, validation, and
conformance evidence separate.

The ordinary provider-response spine is an independent source-backed lane at
`cpp/tests/attribute_value_update_response_catch2.cpp:129`. It passes 37
HLA_EVOKED assertions, maps six Requirements-Lab anchors to three canonical
2025 sections (`6.10`, `6.21`, and `6.21.5`), and exercises four official C++
API surfaces. It proves the known-object request/provider callback/response
reflection chain with request and response tags, reliable transport, producer
identity, and no sent-region designator:

    python tools/query_rti_work.py focus attribute-value-update-response --summary --compact
    python tools/query_rti_work.py trace "Embedded Request Attribute Value Update supports a 2025 provider response" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Request Attribute Value Update supports a 2025 provider response" --summary --compact
    python tools/query_rti_work.py check --lane attribute-value-update-response --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded Request Attribute Value Update supports a 2025 provider response$" --output-on-failure

Keep regional/DDM, automatic provision, timestamped/retraction, update-rate,
save/restore, packaging, validation, and conformance in separate lanes.

The regional provider-response DDM recheck is an independent source-backed
lane at
`cpp/tests/regional_attribute_value_update_provider_response_recheck_catch2.cpp:136`.
It passes 39 HLA_EVOKED assertions, maps four Requirements-Lab anchors to two
canonical 2025 sections (`6.10` and `9.13.1`), and exercises eight official C++
API surfaces. It queues the provider's no-time response from an
overlap-qualified regional request, commits a valid disjoint requester range
before reflection delivery, and proves the stale reflection is suppressed at
the callback boundary:

    python tools/query_rti_work.py focus regional-provider-response-ddm-recheck --summary --compact
    python tools/query_rti_work.py trace "Embedded regional provider response rechecks DDM eligibility at reflection delivery" --summary --compact
    python tools/query_rti_work.py matrix "Embedded regional provider response rechecks DDM eligibility at reflection delivery" --summary --compact
    python tools/query_rti_work.py check --lane regional-provider-response-ddm-recheck --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded regional provider response rechecks DDM eligibility at reflection delivery$" --output-on-failure

Keep automatic provision, successful post-mutation reflection, timestamped/
retraction, relaxed-DDM, save/restore, packaging, validation, and conformance
in separate lanes.

The ownership/update-region boundary is an independent source-backed lane at
`cpp/tests/ownership_transfer_update_region_catch2.cpp:142`. It passes 88
HLA_EVOKED assertions, maps five Requirements-Lab anchors to four canonical
2025 sections (`7.1.2.1`, `9.1.3.3`, `9.5.4`, and `9.6`), and exercises eight
official C++ API surfaces. It transfers one regional object attribute with If
Available followed by Divestiture If Wanted, proves the former owner cannot
update, checks default-region reflection, and then requires an explicit
replacement association from the new owner:

    python tools/query_rti_work.py focus ownership-transfer-update-region --summary --compact
    python tools/query_rti_work.py trace "Embedded ownership transfer clears the former owner's 2025 update-region association" --summary --compact
    python tools/query_rti_work.py matrix "Embedded ownership transfer clears the former owner's 2025 update-region association" --summary --compact
    python tools/query_rti_work.py check --lane ownership-transfer-update-region --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded ownership transfer clears the former owner's 2025 update-region association$" --output-on-failure

Keep Confirm Divestiture, unconditional/negotiated transfer, timestamped/
default-region behavior, scope advisories, save/restore, packaging, validation,
and conformance in separate lanes.

The alternate-advance timestamped directed-interaction case is a separate
focused source lane with 108 HLA_EVOKED assertions, 15 Lab requirement
anchors, 10 canonical 2025 sections, and 11 official C++ API surfaces. It is
queryable without scanning `ieee1516_2025_federation_management_catch2.cpp`:

    python tools/query_rti_work.py focus timestamped-directed-interaction-alternate-advance --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped directed interaction delivers before FQR TARA and NMRA grants" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timestamped directed interaction delivers before FQR TARA and NMRA grants" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-directed-interaction-alternate-advance --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timestamped directed interaction delivers before FQR TARA and NMRA grants$" --output-on-failure

The joined-federate MOM `HLAfederateState` save/restore case is a separate
focused source lane with 152 assertions under HLA_EVOKED and HLA_IMMEDIATE,
one Lab requirement, one canonical §11.4.1 section, and 11 official C++
save/restore surfaces. It proves the observer-visible state sequence 1→3→1→5→1
and saving-federate self-state suppression:

    python tools/query_rti_work.py focus joined-federate-mom-federate-state --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAfederateState follows save and restore callbacks" --summary --compact
    python tools/query_rti_work.py matrix "Embedded joined-federate MOM HLAfederateState follows save and restore callbacks" --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-federate-state --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded joined-federate MOM HLAfederateState follows save and restore callbacks$" --output-on-failure

The timestamped Delete Object Instance failure-file case is another isolated
service-report lane. It is source-backed at
`cpp/tests/timestamped_delete_object_instance_failure_service_report_catch2.cpp:127`
with 134 HLA_EVOKED assertions, five Lab anchors, four canonical 2025 sections,
and three official C++ API surfaces. It checks deterministic serial-0/serial-1
filesystem records for unknown-object and earlier-than-lookahead failures:

    python tools/query_rti_work.py focus timestamped-delete-object-instance-failure-service-report --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records failed timestamped Delete Object Instance invocations" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records failed timestamped Delete Object Instance invocations" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-delete-object-instance-failure-service-report --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting records failed timestamped Delete Object Instance invocations$" --output-on-failure

The accepted timestamped Delete Object Instance sender-file companion is
source-backed at
`cpp/tests/timestamped_delete_object_instance_service_report_catch2.cpp:193`
with 91 HLA_EVOKED assertions, four Lab anchors, four canonical 2025 sections,
and four official C++ API surfaces. It checks the type-34 Null-return record
before the timestamped Remove Object Instance callback:

    python tools/query_rti_work.py focus timestamped-delete-object-instance-sender-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records timestamped Delete Object Instance before removal callback" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records timestamped Delete Object Instance before removal callback" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-delete-object-instance-sender-file --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting records timestamped Delete Object Instance before removal callback$" --output-on-failure

The time-regulated timestamped Delete Object Instance sender-file companion is
source-backed at
`cpp/tests/time_regulated_timestamped_delete_object_instance_service_report_catch2.cpp:206`
with 172 HLA_EVOKED assertions, five Lab anchors, five canonical 2025 sections,
and eight official C++ API surfaces. It checks the type-33 MessageRetractionHandle
return, immutable file identity through a report-switch cycle, and the
timestamped removal callback before its matching grant:

    python tools/query_rti_work.py focus timestamped-delete-object-instance-time-regulated-sender-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records time-regulated timestamped Delete Object Instance with retraction handle" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records time-regulated timestamped Delete Object Instance with retraction handle" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-delete-object-instance-time-regulated-sender-file --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records time-regulated timestamped Delete Object Instance with retraction handle$" --output-on-failure

The accepted timestamped Delete Object Instance MOM-interaction companion is
source-backed at
`cpp/tests/timestamped_delete_object_instance_service_report_interaction_catch2.cpp:145`
with 104 assertions across HLA_EVOKED publisher/receiver and an HLA_IMMEDIATE
observer, five Lab anchors, four canonical 2025 sections, and ten official C++
API surfaces. It decodes the type-33 MessageRetractionHandle through
HLAreportServiceInvocation while file reporting is disabled, then verifies the
timestamped removal callback before its matching grant:

    python tools/query_rti_work.py focus timestamped-delete-object-instance-service-report-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers accepted timestamped Delete Object Instance through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting delivers accepted timestamped Delete Object Instance through MOM interaction" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-delete-object-instance-service-report-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted timestamped Delete Object Instance through MOM interaction$" --output-on-failure

The timestamped Delete Object Instance MOM-failure matrix is source-backed at
`cpp/tests/timestamped_delete_object_instance_failure_service_report_interaction_catch2.cpp:85`
with 92 HLA_IMMEDIATE assertions, five Lab anchors, four canonical 2025
sections, and five official C++ API surfaces. It records unknown-object and
earlier-than-lookahead failures through HLAreportServiceInvocation with the
type-37/type-63/type-31 forms, type-34 Null return, exact exception text, and
serials zero and one:

    python tools/query_rti_work.py focus timestamped-delete-object-instance-failure-mom-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-delete-object-instance-failure-mom-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction$" --output-on-failure

The accepted timestamped regional Send Interaction With Regions service-report
file slice is source-backed at
`cpp/tests/timestamped_regional_interaction_service_report_file_catch2.cpp:220`
with 242 HLA_EVOKED assertions, one Lab anchor, one canonical 2025 section,
and eight selected official C++ API surfaces. It verifies the production
filesystem record's type-27/type-40/type-43/type-63/type-31 supplied forms and
type-33 MessageRetractionHandle at serial zero before the constrained regional
Receive Interaction callback. Temporal-driving reports are disabled after the
accepted record so the callback-entry ordering remains directly observable:

    python tools/query_rti_work.py focus timestamped-regional-interaction-service-report --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records timestamped Send Interaction With Regions before interaction callback" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records timestamped Send Interaction With Regions before interaction callback" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-regional-interaction-service-report --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.(Embedded service reporting records timestamped Send Interaction With Regions before interaction callback|Embedded service reporting delivers accepted timestamped regional Send Interaction With Regions through MOM interaction)$" --output-on-failure

The same indexed lane now includes the public MOM companion at
`cpp/tests/timestamped_regional_interaction_service_report_interaction_catch2.cpp:179`
with 248 assertions across HLA_EVOKED publisher/receiver and an HLA_IMMEDIATE
observer. With Send Service Reports to File disabled, it decodes one public
`HLAreportServiceInvocation` for the accepted timestamped regional send,
including the type-27/type-40/type-43/type-63/type-31 supplied forms and the
type-33 retraction return, then verifies the constrained callback's source
region, timestamp, order, and retraction metadata. Use the exact `-R` selector
printed by `focus`; the broad label also contains historical traceability
checks whose generated selectors are tracked separately as baseline drift.

The timestamped regional service-report failure lane is also source-backed in
two paths: the production filesystem matrix at
`cpp/tests/timestamped_regional_interaction_failure_service_report_file_catch2.cpp:157`
has 307 HLA_EVOKED assertions, and the HLA_IMMEDIATE MOM matrix at
`cpp/tests/timestamped_regional_interaction_failure_service_report_interaction_catch2.cpp:90`
has 189 assertions. Together they exercise invalid interaction-class,
parameter, region, and logical-time calls, preserving serials zero through
three, the type-27/type-40/type-43/type-63/type-31 supplied forms, the type-34
Null return, false indicators, exact exception text, and no application
callback. Query and run both with the separate failure lane:

    python tools/query_rti_work.py focus timestamped-regional-interaction-failure --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records failed timestamped regional Send Interaction With Regions invocations" --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped regional Send Interaction With Regions invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-regional-interaction-failure --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.(Embedded service reporting records failed timestamped regional Send Interaction With Regions invocations|Embedded service reporting delivers failed timestamped regional Send Interaction With Regions invocations through MOM interaction)$" --output-on-failure

The accepted ordinary regional Send Interaction With Regions filesystem case
is source-backed at
`cpp/tests/regional_interaction_service_report_file_catch2.cpp:185` with 173
HLA_EVOKED assertions, one Lab anchor, canonical section `11.5`, and six
official C++ API surfaces. It records one overlap-qualified serial-zero
service type 2 report to the configured joined-federate file before constrained
Receive Interaction delivery, preserving the type-27/type-40/type-43/type-63/
type-34 forms, type-34 Null return, source RegionHandleSet, payload, producer,
and tag. Query it independently:

    python tools/query_rti_work.py focus ordinary-regional-interaction-service-report-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records accepted regional Send Interaction With Regions before interaction callback" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records accepted regional Send Interaction With Regions before interaction callback" --summary --compact
    python tools/query_rti_work.py check --lane ordinary-regional-interaction-service-report-file --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records accepted regional Send Interaction With Regions before interaction callback$" --output-on-failure

The accepted ordinary regional MOM companion is source-backed at
`cpp/tests/regional_interaction_service_report_interaction_catch2.cpp:128`
with 107 assertions across HLA_EVOKED publisher/receiver and an HLA_IMMEDIATE
observer, two Lab anchors, canonical section `11.5`, and five official C++ API
surfaces. With file reporting disabled, it decodes the accepted serial-zero
report through `HLAreportServiceInvocation` before the application callback:

    python tools/query_rti_work.py focus ordinary-regional-interaction-service-report-mom-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction" --summary --compact
    python tools/query_rti_work.py check --lane ordinary-regional-interaction-service-report-mom-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction$" --output-on-failure

The joined-federate MOM/file identity lane is independently queryable from the
large federation-management executable. It contains three source-located C++
cases, all mapped, with 232 assertions across 25 Requirements-Lab references
and 19 canonical 2025 sections. The public discovery/reflection/removal case
is at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:35806` (152
assertions across HLA_EVOKED and HLA_IMMEDIATE); the switch-cycle case is at
line 35458 (34 assertions), and the save/restore companion at line 35683 (46
assertions). Use the exact handles below rather than scanning the full plan:

    python tools/query_rti_work.py focus service-report-file-lifecycle --summary --compact
    python tools/query_rti_work.py matrix service-report-file-lifecycle --summary --compact --limit 10
    python tools/query_rti_work.py trace m82.mapping.embedded-joined-federate-mom-public-object-management --summary --compact
    python tools/query_rti_work.py test "Embedded joined-federate MOM objects use the public discovery reflection and removal route" --summary --compact
    python tools/query_rti_work.py check --lane service-report-file-lifecycle --summary --compact
    ctest --test-dir <build-dir> -C Debug -L service-report-file-lifecycle --output-on-failure

Keep these exact accepted lanes separate from the ordinary failure matrices,
timestamped/re-enable/save/restore, transport, package/JUnit/protected-review,
validation, and conformance evidence; RL-105/RL-152 remain the bounded Lab
caveats.

The ordinary regional Send Interaction With Regions failure-file case is
source-backed at
`cpp/tests/regional_interaction_failure_service_report_file_catch2.cpp:151`
with 302 HLA_EVOKED assertions, one Lab anchor, one canonical 2025 section,
and three official C++ API surfaces. It uses the real configured
joined-federate filesystem route and records invalid interaction-class,
parameter, and region calls with serials zero through two, type-27/type-40/
type-43/type-63/type-34 supplied forms, Null returns, false indicators, exact
exception text, and no application callback. Query it independently of the
aggregate executable:

    python tools/query_rti_work.py focus ordinary-regional-interaction-failure-service-report-file --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting records failed regional Send Interaction With Regions invocations" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting records failed regional Send Interaction With Regions invocations" --summary --compact
    python tools/query_rti_work.py check --lane ordinary-regional-interaction-failure-service-report-file --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed regional Send Interaction With Regions invocations$" --output-on-failure

Keep the paired HLA_IMMEDIATE MOM-interaction row, accepted regional delivery,
timestamped/re-enable/save/restore, transport, package/JUnit/protected-review,
validation, and conformance separate; RL-105/RL-152 are the bounded Lab
caveats and do not require a corpus resync.

The paired ordinary regional MOM-interaction failure case is source-backed at
`cpp/tests/regional_interaction_failure_service_report_interaction_catch2.cpp:88`
with 179 HLA_IMMEDIATE assertions, three Lab anchors, canonical sections
`9.1.3.3` and `11.5`, and five official C++ API surfaces. With file reporting
disabled, it decodes serials zero through three from
`HLAreportServiceInvocation`, including the committed `SodaFlavor` region
rejected by `MainCourseServed` as `InvalidRegionContext`, and expects no
application callback:

    python tools/query_rti_work.py focus ordinary-regional-interaction-failure-mom-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded service reporting delivers failed regional Send Interaction With Regions invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py matrix "Embedded service reporting delivers failed regional Send Interaction With Regions invocations through MOM interaction" --summary --compact
    python tools/query_rti_work.py check --lane ordinary-regional-interaction-failure-mom-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting delivers failed regional Send Interaction With Regions invocations through MOM interaction$" --output-on-failure

Keep the filesystem row, accepted regional delivery, timestamped/re-enable/
save/restore, transport, package/JUnit/protected-review, validation, and
conformance independent; RL-105/RL-152 remain the bounded Lab caveats.

The timestamped Delete Object Instance retraction slice is now green at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4305` with 49
HLA_EVOKED assertions. Its callback-drain boundary consumes the joined-
federate MOM work item after Enable Time Regulation before asserting the
timestamped deletion, retraction, reconstitution, removal, and grant-order
path. The known-class-disabled attribute-relevance case at
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
sections, and 21 official C++ API surfaces. The mapped custom-transportation
handle-stability case at
`cpp/tests/custom_transportation_handle_stability_catch2.cpp:44` is green with
21 assertions, 12 Requirements-Lab anchors, 11 canonical sections, and 8
official C++ API surfaces; the restored-baseline timestamped MOM interaction
case at `cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4538` is
green with 125 assertions under both callback models. The restored-baseline
regional Provide Attribute Value Update case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4697` is green with
251 assertions and 11 Requirements-Lab anchors. The three-dimensional regional
object-attribute overlap case at
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
assertions at
`cpp/tests/timestamped_attribute_update_queued_passel_retraction_catch2.cpp:154`.
The m54 mixed update-rate subscription case is mapped and green with 37
HLA_EVOKED assertions, eight Requirements-Lab anchors, two canonical 2025
sections, and 15 official C++ API surfaces at
`cpp/tests/mixed_update_rate_subscriptions_catch2.cpp:199`. The next
source pointer is the federation-teardown update-rate-history case at
`cpp/tests/federation_teardown_update_rate_history_catch2.cpp:158` in
`[update-rate-federation-teardown-isolation]`. The m55 slice is green with 46 HLA_EVOKED
assertions, nine Requirements-Lab anchors, five canonical 2025 sections, and
15 official C++ API surfaces. The next source pointer is the unplanned
regional best-effort attribute-rate case at
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
source pointer is the unplanned immediate timestamped directed-interaction
 source-resignation case at
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
 queue for this translation unit is now exhausted; choose the next bounded
 lane from the indexed roadmap rather than rescanning the Requirements Lab.

The indexed `[process-boundary]` loop is 78 mapped plan rows / 4,104 indexed
Catch2 assertions, with every row mapped and source-located. Its stable label
selects 108 registered CTest executions, and the merged JUnit report remains
the last generated m108 snapshot with 2,733 testcases and zero failures; these
are distinct traceability, assertion, execution, and report metrics. Regenerate
the aggregate JUnit artifact only when that evidence gate is intentionally run.
The
local-delete codec contract contributes 9 direct assertions, the registry
service integration 44, and the public two-federate endpoint integration 18;
receive-order Delete Object Instance adds a 25-assertion codec contract and a
25-assertion public endpoint/removal-callback integration; the timestamped
public endpoint/removal-callback slice adds 64 assertions under both callback
models, and the timestamped regional Update Attribute Values endpoint adds 86
assertions under both models; the regional subscription-removal endpoint adds
112 assertions under both callback models; the disjoint regional-update endpoint
adds 50, the remote regional subscription/update endpoint adds 66, the
regional-registration endpoint adds 34, the object-registration endpoint adds 20
under both models, and the public object-class subscription endpoint adds 24 under
both callback models; the public object-discovery endpoint adds 44 under both
callback models. The private registry-binding case adds a ten-assertion official
encoded `Enable Time Regulation`/retained-state check, and the public lifecycle
case adds a four-assertion callback-gating check. The process TAR rejection
slices add 28 callback-gated pending-role assertions across regulation and
constrained role enablement. The malformed process logical-time decode fence
adds 9 assertions and maps directly to clause 8.8.3. The two-federate process
federation scheduler adds 30 assertions across deferred constrained TAR,
regulator-driven release, unsolicited grant transport, and Evoke callback
delivery. The focused timestamped process interaction-before-grant case adds
60 `HLA_EVOKED` assertions mapped to 15 Requirements-Lab anchors and nine
canonical 2025 sections; query `process-tso-interaction-before-grant` for its
exact trace. Pre-grant retraction, multiple-message ordering,
directed/regional TSO, save/restore, and conformance remain separate slices.
The paired timestamped process-attribute-update cases are independently
queryable without reopening the aggregate test file:

    python tools/query_rti_work.py focus process-tso-attribute-before-grant --summary --compact
    python tools/query_rti_work.py trace "Private process service releases timestamped Update Attribute Values before a constrained grant" --summary --compact
    python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
    python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^(umbra\\.process_boundary_private\\.catch2\\.Private process service releases timestamped Update Attribute Values before a constrained grant|umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassadors deliver a deferred timestamped process attribute update before the grant)$" --output-on-failure

Together they total 109 `HLA_EVOKED` assertions and share 12 mapped
Requirements-Lab anchors across eight canonical 2025 sections. Keep the
private transport/registry contract and public callback behavior separate in
failure triage; retraction stress, multiple-message ordering, fanout,
regional/DDM, save/restore, package/JUnit, validation, interoperability, and
conformance are separate lanes.
The 78-case query count is the unique mapped plan/test-declaration count; the
108-test CTest label count is the executable cross-target count. The
`process-boundary` CTest label uses the independently buildable private process
target plus the public connection target. The JUnit target runs both and
merges their reports, so this lane does not depend on the damaged aggregate
federation-management translation unit.
Each completed slice records its exact
assertion count in `ROADMAP-INDEX.json`. The bounded `coverage --lane transport` query
reports 82 plan entries (76 mapped, 82 source-located, and no source-drift
rows; 6 have no Lab requirement mapping); use that query rather than a full-suite
scan when selecting transport work. The installed-profile ordinary,
timestamped, parameterized-envelope, connection-loss, object-registration,
and ordinary attribute-update/Reflect package checks are
separate CTest labels;
their exact names and the five-clause connection-loss baseline mapping are
available through `python tools/query_rti_work.py work --summary --compact`.
These remain process-boundary foundation evidence until protected review and
the broader interoperability gate are complete.

The focused accepted-TSO ownership-transfer case is independently buildable at
`timestamped_attribute_update_ownership_transfer_catch2.cpp:191`. It records
60 HLA_EVOKED assertions and maps nine Requirements-Lab anchors to clauses
6.10, 6.11.1, 7.2, and 8.1.4. Its unique lane keeps one timestamped Update
Attribute Values passel queued while the owner divests and a new federate
acquires the attribute; the recipient then verifies the original producer,
payload, timestamp/order, and retraction before its constrained grant:

    python tools/query_rti_work.py focus timestamped-attribute-update-ownership-transfer --summary --compact
    python tools/query_rti_work.py trace "Embedded accepted timestamped attribute update survives ownership transfer before constrained grant" --summary --compact
    python tools/query_rti_work.py matrix "Embedded accepted timestamped attribute update survives ownership transfer before constrained grant" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-attribute-update-ownership-transfer --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_attribute_update_ownership_transfer\.catch2\.Embedded accepted timestamped attribute update survives ownership transfer before constrained grant$" --output-on-failure

The ordinary declared custom-transportation delivery slice is independently
buildable at `custom_transportation_delivery_catch2.cpp:142`. It records 42
HLA_EVOKED assertions and intentionally carries an explicit no-standalone-Lab
requirement disposition while proving that both receive-order interaction and
attribute callbacks preserve the composed FOM transportation, producer, tag,
and handle metadata:

    python tools/query_rti_work.py focus custom-transportation-delivery --summary --compact
    python tools/query_rti_work.py trace "Embedded ordinary delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py matrix "Embedded ordinary delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py check --lane custom-transportation-delivery --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.custom_transportation_delivery\.catch2\.Embedded ordinary delivery accepts a declared custom FOM transportation$" --output-on-failure

The ordinary regional declared custom-transportation slice is independently
buildable at `custom_transportation_ordinary_regional_interaction_catch2.cpp:107`.
It records 40 HLA_EVOKED assertions and keeps the same explicit no-standalone-
Lab requirement disposition while proving overlapping two-dimensional regions,
conveyed designators, and preservation of the custom transportation through
the receive-order interaction callback:

    python tools/query_rti_work.py focus custom-transportation-ordinary-regional-interaction --summary --compact
    python tools/query_rti_work.py trace "Embedded ordinary regional delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py matrix "Embedded ordinary regional delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py check --lane custom-transportation-ordinary-regional-interaction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.custom_transportation_ordinary_regional_interaction\.catch2\.Embedded ordinary regional delivery accepts a declared custom FOM transportation$" --output-on-failure

The ordinary regional-attribute declared custom-transportation slice is
independently buildable at
`custom_transportation_ordinary_regional_attribute_catch2.cpp:121`. It records
51 HLA_EVOKED assertions and keeps the explicit no-standalone-Lab disposition
while proving regional object registration, conveyed designators, and custom
transportation preservation through the ordinary Reflect Attribute Values
callback:

    python tools/query_rti_work.py focus custom-transportation-ordinary-regional-attribute --summary --compact
    python tools/query_rti_work.py trace "Embedded ordinary regional attribute delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py matrix "Embedded ordinary regional attribute delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py check --lane custom-transportation-ordinary-regional-attribute --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.custom_transportation_ordinary_regional_attribute\.catch2\.Embedded ordinary regional attribute delivery accepts a declared custom FOM transportation$" --output-on-failure

The federation-scoped Current FDD MOM slice is independently buildable at
`federation_mom_current_fdd_catch2.cpp:150`. It records 60 HLA_EVOKED
assertions and maps the pinned clause-4 content-access candidate to the
official HLAfederation object, HLAunicodeString FDD value, Join-triggered
conditional refresh, and direct requested-value agreement:

    python tools/query_rti_work.py focus federation-mom-current-fdd --summary --compact
    python tools/query_rti_work.py trace "Embedded federation MOM exposes and refreshes HLAcurrentFDD" --summary --compact
    python tools/query_rti_work.py matrix "Embedded federation MOM exposes and refreshes HLAcurrentFDD" --summary --compact
    python tools/query_rti_work.py check --lane federation-mom-current-fdd --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.federation_mom_current_fdd\.catch2\.Embedded federation MOM exposes and refreshes HLAcurrentFDD$" --output-on-failure

The latest FOM-composition transportation-handle stability slice is
independently buildable at
`custom_transportation_handle_stability_catch2.cpp:44`. It records 21
HLA_EVOKED assertions and maps 12 Requirements-Lab anchors to 11 canonical
2025 sections and eight official C++ API surfaces. It proves that a later Join
adding an earlier-sorting transportation preserves the existing handle for
both joined federates while exposing the new shared handle/name:

    python tools/query_rti_work.py focus transportation-handle-stability --summary --compact
    python tools/query_rti_work.py trace "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
    python tools/query_rti_work.py matrix "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
    python tools/query_rti_work.py check --lane transportation-handle-stability --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.custom_transportation_handle_stability\.catch2\.Embedded custom transportation handles remain stable across an additional FOM join$" --output-on-failure

The latest unnamed object-instance registration/discovery slice is independently
buildable at `object_instance_registration_discovery_catch2.cpp:78`. It records
81 assertions under both `HLA_EVOKED` and `HLA_IMMEDIATE`, maps eight
Requirements-Lab anchors to four canonical 2025 sections, and covers generated
instance identity, publication preconditions, superclass discovery promotion,
unsubscribe-before-callback re-evaluation, and known-instance lookup:

    python tools/query_rti_work.py focus object-instance-registration-discovery --summary --compact
    python tools/query_rti_work.py trace "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle" --summary --compact
    python tools/query_rti_work.py matrix "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle" --summary --compact
    python tools/query_rti_work.py check --lane object-instance-registration-discovery --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.object_instance_registration_discovery\.catch2\.Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle$" --output-on-failure

The declared custom-transportation timestamped-delivery case is independently
buildable at `custom_transportation_timestamped_delivery_catch2.cpp:185`. It
records 48 HLA_EVOKED assertions and intentionally carries an explicit
no-standalone-Lab-requirement disposition while proving that both timestamped
interaction and attribute callbacks preserve the user-declared transportation:

    python tools/query_rti_work.py focus custom-transportation-timestamped-delivery --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timestamped delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py check --lane custom-transportation-timestamped-delivery --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.custom_transportation_timestamped_delivery\.catch2\.Embedded timestamped delivery accepts a declared custom FOM transportation$" --output-on-failure

The directed custom-transportation companion is independently buildable at
`custom_transportation_timestamped_directed_delivery_catch2.cpp:143`. It
records 42 HLA_EVOKED assertions and keeps the same explicit no-standalone-Lab
requirement disposition while proving target routing and custom transportation
preservation through the timestamped Receive Directed Interaction callback:

    python tools/query_rti_work.py focus custom-transportation-timestamped-directed-delivery --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped directed delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timestamped directed delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py check --lane custom-transportation-timestamped-directed-delivery --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.custom_transportation_timestamped_directed_delivery\.catch2\.Embedded timestamped directed delivery accepts a declared custom FOM transportation$" --output-on-failure

The regional custom-transportation companion is independently buildable at
`custom_transportation_timestamped_regional_attribute_delivery_catch2.cpp:163`.
It records 60 HLA_EVOKED assertions and keeps the same explicit no-standalone-Lab
requirement disposition while proving regional designator propagation,
timestamp/order, retraction, and custom transportation preservation through
the timestamped Reflect Attribute Values callback:

    python tools/query_rti_work.py focus custom-transportation-timestamped-regional-attribute-delivery --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped regional attribute delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timestamped regional attribute delivery accepts a declared custom FOM transportation" --summary --compact
    python tools/query_rti_work.py check --lane custom-transportation-timestamped-regional-attribute-delivery --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.custom_transportation_timestamped_regional_attribute_delivery\.catch2\.Embedded timestamped regional attribute delivery accepts a declared custom FOM transportation$" --output-on-failure

The MOM transportation-type request slice is independently buildable at
`mom_transportation_type_change_request_catch2.cpp:189`. It records 168
assertions across HLA_EVOKED and HLA_IMMEDIATE, maps to canonical 2025 sections
6.25.1, 6.26, 6.27, 6.30.3, 6.31.3, and 11.5, and exercises official MIM
request payload decoding, callback confirmations, malformed/unknown-handle
failures, publisher-local scope, and service-report delivery:

    python tools/query_rti_work.py focus mom-transportation-type-change-request --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM transportation-type request interactions invoke public changes" --summary --compact
    python tools/query_rti_work.py matrix "Embedded MOM transportation-type request interactions invoke public changes" --summary --compact
    python tools/query_rti_work.py check --lane mom-transportation-type-change-request --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.mom_transportation_type_change_request\.catch2\.Embedded MOM transportation-type request interactions invoke public changes$" --output-on-failure

The multi-recipient timestamped directed-interaction source-resignation slice
is independently buildable at
`timestamped_directed_interaction_multi_recipient_source_resignation_catch2.cpp:172`.
It records 113 HLA_EVOKED assertions, maps seven Requirements-Lab candidates to
canonical 2025 clauses 4.12, 5.1.5, 8.1.5, 8.1.6, and 8.8.3, and drains
FQR/TARA/NMRA independently after NO_ACTION source resignation while preserving
target/producer/tag/time/order/transport/retraction metadata:

    python tools/query_rti_work.py focus timestamped-directed-interaction-multi-recipient-source-resignation --summary --compact
    python tools/query_rti_work.py trace "Embedded queued timestamped directed interaction survives source resignation for each recipient" --summary --compact
    python tools/query_rti_work.py matrix "Embedded queued timestamped directed interaction survives source resignation for each recipient" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-directed-interaction-multi-recipient-source-resignation --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_directed_interaction_multi_recipient_source_resignation\.catch2\.Embedded queued timestamped directed interaction survives source resignation for each recipient$" --output-on-failure

The direct TAR/NMR timestamped directed-interaction frontier is independently
buildable at `timestamped_directed_interaction_tar_nmr_catch2.cpp:150`. It
records 79 HLA_EVOKED assertions, maps seven Requirements-Lab candidates to
canonical 2025 clauses 5.1.5, 8.1.5, 8.1.6, 8.8.3, and 8.22.3, and proves that
two constrained recipients receive the target-qualified payload before their
TAR(7) and NMR(10) grants while preserving target/producer/tag/time/order/
transport/retraction metadata:

    python tools/query_rti_work.py focus timestamped-directed-interaction-tar-nmr --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped directed interaction delivers before TAR and NMR grants" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timestamped directed interaction delivers before TAR and NMR grants" --summary --compact
    python tools/query_rti_work.py check --lane timestamped-directed-interaction-tar-nmr --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_directed_interaction_tar_nmr\.catch2\.Embedded timestamped directed interaction delivers before TAR and NMR grants$" --output-on-failure

The federation-scoped MOM save-conditionals slice is independently buildable
at `federation_mom_save_conditionals_catch2.cpp:183`. It records 95
HLA_EVOKED assertions, maps six Requirements-Lab candidates to canonical 2025
clauses 4.19, 4.19.6, 4.20, and 11.4.1, and checks empty initial
HLAnextSaveName/Time and HLAlastSaveName/Time, pending timestamped save
encoding, clear-on-admission, and last-on-completion reflection:

    python tools/query_rti_work.py focus federation-mom-save-conditionals --summary --compact
    python tools/query_rti_work.py trace "Embedded federation MOM save conditionals follow pending admission and completion" --summary --compact
    python tools/query_rti_work.py matrix "Embedded federation MOM save conditionals follow pending admission and completion" --summary --compact
    python tools/query_rti_work.py check --lane federation-mom-save-conditionals --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.federation_mom_save_conditionals\.catch2\.Embedded federation MOM save conditionals follow pending admission and completion$" --output-on-failure

The joined-federate MOM GALT/LITS projection is independently buildable at
`joined_federate_mom_galt_lits_periodic_catch2.cpp:138`. It records 58
HLA_EVOKED assertions, maps the MOM projection to canonical 2025 clause 11.4.1,
and checks direct Query GALT/Query LITS agreement, one HLAsetTiming periodic
reflection, and the official empty-array undefined form after disabling the
sole regulator:

    python tools/query_rti_work.py focus joined-federate-mom-galt-lits-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes federation GALT and LITS" --summary --compact
    python tools/query_rti_work.py matrix "Embedded joined-federate MOM exposes federation GALT and LITS" --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-galt-lits-periodic --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_galt_lits_periodic\.catch2\.Embedded joined-federate MOM exposes federation GALT and LITS$" --output-on-failure

The joined-federate MOM queued-TSO-length projection is independently buildable
at `joined_federate_mom_tso_length_periodic_catch2.cpp:184`. It records 65
HLA_EVOKED assertions, maps the MOM projection to canonical 2025 clause 11.4.1,
and checks HLATSOlength=1 for a pending timestamped interaction, one
HLAsetTiming periodic reflection, and HLATSOlength=0 after the matching grant:

    python tools/query_rti_work.py focus joined-federate-mom-tso-length-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes queued TSO length" --summary --compact
    python tools/query_rti_work.py matrix "Embedded joined-federate MOM exposes queued TSO length" --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-tso-length-periodic --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_tso_length_periodic\.catch2\.Embedded joined-federate MOM exposes queued TSO length$" --output-on-failure

The joined-federate MOM time-state duration projection is independently
buildable at `joined_federate_mom_time_state_duration_catch2.cpp:120`. It records
192 assertions across HLA_EVOKED and HLA_IMMEDIATE, maps the MOM projection to
canonical 2025 clause 11.4.1, and checks direct HLAinteger32BE
HLAtimeGrantedTime/HLAtimeAdvancingTime reads, one HLAsetTiming periodic
reflection, and the non-consuming direct versus consume-once periodic boundary:

    python tools/query_rti_work.py focus joined-federate-mom-time-state-duration --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes time-state durations directly and periodically" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-time-state-duration --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-time-state-duration --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_time_state_duration\.catch2\.Embedded joined-federate MOM exposes time-state durations directly and periodically$" --output-on-failure

The joined-federate MOM reflection-count projection is independently buildable
at `joined_federate_mom_reflection_count_catch2.cpp:140`. It records 103
HLA_EVOKED assertions, maps the MOM projection to canonical 2025 clause 11.4.1,
and checks distinct-object values 0/1/1/2 against callback-invocation totals
0/1/2/3, then 2/4 after one queued timestamped reflection and one periodic
HLAsetTiming reflection:

    python tools/query_rti_work.py focus joined-federate-mom-reflection-counts --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM separates reflection totals from distinct objects" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-reflection-counts --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-reflection-counts --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_reflection_count\.catch2\.Embedded joined-federate MOM separates reflection totals from distinct objects$" --output-on-failure

The joined-federate MOM updates-sent projection is independently buildable at
`joined_federate_mom_updates_sent_catch2.cpp:145`. It records 98 HLA_EVOKED
assertions, maps the MOM projection to canonical 2025 clause 11.4.1, and
checks two best-effort `Server` updates plus one reliable `Soda` update. It
decodes the official `HLAtransportation` and nested
`HLAobjectClassBasedCounts` parameters, verifies callback-gated
RTI-originated metadata, and checks empty NULL arrays for both transportation
buckets on an idle federate:

    python tools/query_rti_work.py focus joined-federate-mom-updates-sent --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestUpdatesSent reports class and transportation counts" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-updates-sent --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-updates-sent --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_updates_sent\.catch2\.Embedded MOM requestUpdatesSent reports class and transportation counts$" --output-on-failure

The joined-federate MOM interactions-sent projection is independently
buildable at `joined_federate_mom_interactions_sent_catch2.cpp:153`. It records
107 HLA_EVOKED assertions, maps the MOM projection to canonical 2025 clause
11.4.1, and checks one reliable and one best-effort `TakeOrder` plus one
reliable regional `MainCourseServed` send. It decodes the official
`HLAtransportation` and nested `HLAinteractionCounts` parameters, verifies
callback-gated RTI-originated metadata, and checks empty NULL arrays for both
transportation buckets on an idle federate:

    python tools/query_rti_work.py focus joined-federate-mom-interactions-sent --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestInteractionsSent reports class and transportation counts" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-interactions-sent --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-interactions-sent --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_interactions_sent\.catch2\.Embedded MOM requestInteractionsSent reports class and transportation counts$" --output-on-failure

The cross-family MOM sender-count NULL-bucket slice is independently buildable
at `mom_sender_count_reports_null_buckets_catch2.cpp:116`. It records 111
`HLA_EVOKED` assertions, maps one Requirements-Lab candidate to canonical
2025 clause 11.4.1, and requests updates-sent, interactions-sent, and
directed-interactions-sent reports for an idle joined federate. It verifies two
reliable RTI-originated transportation buckets per family, official empty
nested arrays, and callback gating:

    python tools/query_rti_work.py focus mom-sender-count-reports-null-buckets --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM sender count reports emit NULL buckets for empty ledgers" --summary --compact
    python tools/query_rti_work.py matrix mom-sender-count-reports-null-buckets --summary --compact
    python tools/query_rti_work.py check --lane mom-sender-count-reports-null-buckets --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.mom_sender_count_reports_null_buckets\.catch2\.Embedded MOM sender count reports emit NULL buckets for empty ledgers$" --output-on-failure

The receiver-ledger MOM reflections-received slice is independently buildable
at `joined_federate_mom_reflections_received_catch2.cpp:194`. It records 129
`HLA_EVOKED` assertions, maps one Requirements-Lab candidate to canonical
2025 clause 11.4.1, and records two best-effort Server reflections plus one
reliable Soda reflection at the receiving federate. It decodes the official
`HLAtransportation` and nested `HLAobjectClassBasedCounts` values, verifies
two empty `HLAreflectCounts` NULL buckets for an idle federate, and keeps
RTI-originated report delivery callback-gated:

    python tools/query_rti_work.py focus joined-federate-mom-reflections-received --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestReflectionsReceived reports class and transportation counts" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-reflections-received --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-reflections-received --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_reflections_received\.catch2\.Embedded MOM requestReflectionsReceived reports class and transportation counts$" --output-on-failure

The receiver-ledger MOM interactions-received slice is independently buildable
at `joined_federate_mom_interactions_received_catch2.cpp:149`. It records 113
`HLA_EVOKED` assertions, maps one Requirements-Lab candidate to canonical
2025 clause 11.4.1, and delivers one reliable plus one best-effort `TakeOrder`
receive callback at the represented federate. It decodes the official
`HLAtransportation` and nested `HLAinteractionCounts` values, verifies one
populated report bucket per supported transportation, two empty NULL buckets
for an idle federate, and RTI-originated callback metadata:

    python tools/query_rti_work.py focus joined-federate-mom-interactions-received --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestInteractionsReceived reports class and transportation counts" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-interactions-received --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-interactions-received --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_interactions_received\.catch2\.Embedded MOM requestInteractionsReceived reports class and transportation counts$" --output-on-failure

The directed receiver-ledger MOM slice is independently buildable at
`joined_federate_mom_directed_interactions_received_catch2.cpp:180`. It records
119 `HLA_EVOKED` assertions, maps one Requirements-Lab candidate to canonical
2025 clause 11.4.1, proves an ordinary `TakeOrder` receive stays out of the
directed ledger, and counts one directed reliable receive. It decodes the
official `HLAtransportation` and nested `HLAinteractionCounts` values, verifies
the empty best-effort bucket and two idle NULL buckets, and preserves
RTI-originated callback metadata:

    python tools/query_rti_work.py focus joined-federate-mom-directed-interactions-received --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestDirectedInteractionsReceived reports directed counts and NULL buckets" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-directed-interactions-received --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-directed-interactions-received --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_directed_interactions_received\.catch2\.Embedded MOM requestDirectedInteractionsReceived reports directed counts and NULL buckets$" --output-on-failure

The timestamped regional attribute source-resignation slice is independently
buildable at
`timestamped_regional_attribute_update_resignation_catch2.cpp:146`. It records
58 `HLA_EVOKED` assertions, maps eight Requirements-Lab candidates to six
canonical 2025 clauses, and proves one queued timestamped update from an
ordinary registration's private default source survives source resignation.
The independent regulator releases the callback before the receiver's TAR(7)
grant; the callback preserves the original producer, payload, tag,
timestamp/order, valid retraction metadata, and supplied-empty
`RegionHandleSet`, while post-resignation `Retract` raises
`FederateNotExecutionMember`:

    python tools/query_rti_work.py focus timestamped-regional-attribute-update-resignation --summary --compact
    python tools/query_rti_work.py trace "Embedded queued timestamped regional attribute update survives source resignation" --summary --compact
    python tools/query_rti_work.py matrix timestamped-regional-attribute-update-resignation --summary --compact
    python tools/query_rti_work.py check --lane timestamped-regional-attribute-update-resignation --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_regional_attribute_update_resignation\.catch2\.Embedded queued timestamped regional attribute update survives source resignation$" --output-on-failure

The requirements-facing `timestamped-default-region-attribute-update-resignation`
lane aliases this same executable source and CTest handle; use the alias for
the default-region mapping without maintaining a duplicate runtime case.

The timed negotiated-cancellation-after-restore slice is independently
buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore_catch2.cpp:263`.
It records 135 `HLA_EVOKED` assertions, maps 44 Requirements-Lab candidates to
23 canonical 2025 clauses, and runs through the short target
`umbra_tso_regional_negotiated_cancel_restore_catch2`. It restores a saved
timestamped regional update, queues an If Available willing-to-acquire
reservation, enters negotiated divestiture, and cancels the requester before
the confirmation callback; the source retains ownership and the surviving
recipient receives one saved reflection at its Flush Queue boundary:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending negotiated ownership transfer after restore" --summary --compact
    python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending negotiated ownership transfer after restore$" --output-on-failure

Keep negotiated transfer completion, multi-candidate arbitration, alternate
callback models, passive/relaxed DDM, remote transport, package/JUnit/protected
review, Lab validation, and conformance as separate evidence lanes.

The timed negotiated-continuation-after-restore slice is independently
buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore_catch2.cpp:283`.
It records 158 `HLA_EVOKED` assertions, maps 44 Requirements-Lab candidates to
23 canonical 2025 sections, and runs through
`umbra_tso_regional_negotiated_continuation_restore_catch2`. It restores one
saved timestamped regional update, queues two If Available requests before
negotiated divestiture, resigns the first requester with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, and reissues negotiation against the
still-pending second callback. The surviving regional recipient receives one
reflection before its Flush Queue grant; Confirm Divestiture transfers ownership
and the second candidate receives one acquisition notification. The original
If Available callback is intentionally drained only after confirmation so stale
callback work is suppressed:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues to a retained negotiated ownership candidate after restore" --summary --compact
    python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues to a retained negotiated ownership candidate after restore$" --output-on-failure

This is bounded callback-order and negotiated-transfer evidence; persistent
Willing-to-Acquire behavior after an If Available callback, broader arbitration,
alternate callback models, passive/relaxed DDM, remote transport,
package/JUnit/protected review, Lab validation, and conformance remain separate.

The timed negotiated-confirmation-cancel-after-restore slice is independently
buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore_catch2.cpp:283`.
It records 161 `HLA_EVOKED` assertions, maps 48 Requirements-Lab candidates to
26 canonical 2025 sections, and runs through
`umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2`. It restores
one saved timestamped regional update, queues two If Available candidates,
resigns the first requester with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, and
reissues negotiation against the retained second callback. After Request
Divestiture Confirmation is delivered, the owner cancels the negotiated
divestiture, keeps ownership, rejects stale Confirm Divestiture, and emits no
owner-release callback; the queued clock callback reports unavailable:

    python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
    python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels retained negotiated owner confirmation after restore" --summary --compact
    python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
    python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update cancels retained negotiated owner confirmation after restore$" --output-on-failure

This is bounded cancellation evidence; persistent post-callback
Willing-to-Acquire behavior, negotiated transfer completion, alternate callback
models, passive/relaxed DDM, remote transport, package/JUnit/protected review,
Lab validation, and conformance remain separate.

The timestamped-attribute-order-cohort slice is independently buildable at
`timestamped_attribute_order_cohort_catch2.cpp:152`. It records 87
`HLA_EVOKED` assertions, maps five Requirements-Lab candidates to five
canonical 2025 sections, and runs through
`umbra_timestamped_attribute_order_cohort_catch2`. It submits timestamp 7
before the timestamp-5 cohort, advances two constrained recipients to 5 and
then 7, and verifies callback-before-grant delivery, different-timestamp
ordering, and complete equal-timestamp cohorts. The equal-timestamp tie-break
is intentionally unspecified:

    python tools/query_rti_work.py focus timestamped-attribute-order-cohort --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped attribute updates preserve different-timestamp order for each constrained recipient" --summary --compact
    python tools/query_rti_work.py matrix timestamped-attribute-order-cohort --summary --compact
    python tools/query_rti_work.py check --lane timestamped-attribute-order-cohort --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timestamped_attribute_order_cohort\.catch2\.Embedded timestamped attribute updates preserve different-timestamp order for each constrained recipient$" --output-on-failure

This is bounded in-process ordering evidence; alternate advances, simultaneous
transport-arrival ordering, save/restore composition, ownership/resignation,
remote transport, package/JUnit/protected review, Lab validation, and
conformance remain separate.

The timestamped-directed-interaction-regulation-reenable-changed-lookahead
slice is independently buildable at
`timestamped_directed_interaction_regulation_reenable_changed_lookahead_catch2.cpp:151`.
It records 57 `HLA_EVOKED` assertions, maps 11 Requirements-Lab candidates to
eight canonical 2025 sections, and runs through
`umbra_tso_directed_reenable_changed_lookahead_catch2`. It queues one
target-qualified reliable timestamped directed interaction at time five under
lookahead one, disables and callback-gated re-enables Time Regulation at
lookahead three, verifies Query Lookahead, and advances the producer to time
two so the directed callback arrives at time five before the constrained
recipient's grant. Target, tag, producer, timestamp/order, transportation,
callback, and terminal retraction metadata remain intact:

    python tools/query_rti_work.py focus timestamped-directed-interaction-regulation-reenable-changed-lookahead --summary --compact
    python tools/query_rti_work.py trace "Embedded queued timestamped directed interaction survives time-regulation disable and re-enable with changed lookahead" --summary --compact
    python tools/query_rti_work.py matrix timestamped-directed-interaction-regulation-reenable-changed-lookahead --summary --compact
    python tools/query_rti_work.py check --lane timestamped-directed-interaction-regulation-reenable-changed-lookahead --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\.timestamped_directed_interaction_regulation_reenable_changed_lookahead\.catch2\.Embedded queued timestamped directed interaction survives time-regulation disable and re-enable with changed lookahead$" --output-on-failure

This is bounded in-process directed-TSO evidence; directed DDM breadth,
alternate advances, ownership/resignation, save/restore composition, remote
transport, package/JUnit/protected review, Lab validation, interoperability,
and conformance remain separate.

The joined-federate MOM removed-object history/retraction slice is independently
buildable at
`joined_federate_mom_removed_object_count_tso_retraction_catch2.cpp:197`.
It records 89 HLA_EVOKED assertions, maps nine Requirements-Lab candidates to
canonical 2025 clauses 6.16, 6.17.1, 8.22.3, 8.23.3, and 11.4.1, and checks
that the receiving federate's HLAobjectInstancesRemoved count advances at a
delivered timestamped removal, remains historical after legal Retract, and
does not fan out to the still-pending constrained recipient:

    python tools/query_rti_work.py focus joined-federate-mom-removed-object-count-tso-retraction --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAobjectInstancesRemoved preserves history after timestamped retraction" --summary --compact
    python tools/query_rti_work.py matrix "Embedded joined-federate MOM HLAobjectInstancesRemoved preserves history after timestamped retraction" --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-removed-object-count-tso-retraction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_removed_object_count_tso_retraction\.catch2\.Embedded joined-federate MOM HLAobjectInstancesRemoved preserves history after timestamped retraction$" --output-on-failure

The TSO retraction-lifetime slice is independently buildable at
`timestamped_interaction_regulation_reenable_catch2.cpp:256`. It records 30
HLA_EVOKED assertions, maps five Requirements-Lab candidates to four canonical
2025 §8 clauses, and keeps one `MessageRetractionHandle` across a disabled
time-regulation interval: retract is rejected while regulation is disabled,
the same handle is accepted after the callback-gated re-enable, and a second
retract reports `MessageCanNoLongerBeRetracted`:

    python tools/query_rti_work.py focus tso-retraction-disable-reenable-lifetime --summary --compact
    python tools/query_rti_work.py trace "Embedded Disable Time Regulation preserves a live TSO retraction designator across re-enable" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Disable Time Regulation preserves a live TSO retraction designator across re-enable" --summary --compact
    python tools/query_rti_work.py check --lane tso-retraction-disable-reenable-lifetime --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_interaction_regulation_reenable\.catch2\.Embedded Disable Time Regulation preserves a live TSO retraction designator across re-enable$" --output-on-failure

The no-recipient timestamped attribute-update designator slice is independently
buildable at `timestamped_attribute_update_no_fanout_catch2.cpp:54`. It records
20 HLA_EVOKED assertions, maps four Requirements-Lab candidates to canonical
2025 clauses 6.10, 8.1.5, and 8.22.3, and proves that a qualifying TSO
`Update Attribute Values` call returns a valid designator even when no
subscriber is eligible. The first retract succeeds; a later designator is
terminalized by the producer's advance boundary:

    python tools/query_rti_work.py focus tso-attribute-update-no-fanout-designator --summary --compact
    python tools/query_rti_work.py trace "Embedded timestamped Update Attribute Values returns a retraction designator without recipient fanout" --summary --compact
    python tools/query_rti_work.py matrix "Embedded timestamped Update Attribute Values returns a retraction designator without recipient fanout" --summary --compact
    python tools/query_rti_work.py check --lane tso-attribute-update-no-fanout-designator --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_attribute_update_no_fanout\.catch2\.Embedded timestamped Update Attribute Values returns a retraction designator without recipient fanout$" --output-on-failure

The delivered-recipient timestamped object-deletion retraction slice is
independently buildable at `timestamped_object_deletion_retraction_catch2.cpp:147`.
It records 57 HLA_EVOKED assertions, maps eight Requirements-Lab candidates to
canonical 2025 clauses 6.16, 6.17.1, 8.22.3, and 8.23.3, and exercises the
three-federate boundary: an immediate recipient receives Remove Object
Instance, Retract restores the invocation-time object/name/ownership state and
queues Request Retraction only for that delivered recipient, and the
constrained recipient's pending removal is suppressed:

    python tools/query_rti_work.py focus tso-object-deletion-retraction-reconstitution --summary --compact
    python tools/query_rti_work.py trace "Embedded Request Retraction reconstitutes a delivered timestamped object deletion" --summary --compact
    python tools/query_rti_work.py matrix "Embedded Request Retraction reconstitutes a delivered timestamped object deletion" --summary --compact
    python tools/query_rti_work.py check --lane tso-object-deletion-retraction-reconstitution --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_object_deletion_retraction\.catch2\.Embedded Request Retraction reconstitutes a delivered timestamped object deletion$" --output-on-failure

The joined-owner timestamped object-deletion retraction slice is independently
buildable at `timestamped_object_deletion_joined_owner_retraction_catch2.cpp:170`.
It passes 65 `HLA_EVOKED` assertions, maps four Requirements-Lab candidates to
canonical 2025 clauses 6.16 and 8.22.3, and proves that a delivered owner who
resigns before `Retract` is excluded from reconstitution and
`Request Retraction`; still-joined members recover the object/name, and the
departed owner's former attribute is reported through `Attribute Is Not Owned`:

    python tools/query_rti_work.py focus tso-object-deletion-joined-owner-retraction --summary --compact
    python tools/query_rti_work.py trace "Embedded Request Retraction reconstitutes timestamped deletion only for joined owners" --summary --compact
    python tools/query_rti_work.py matrix tso-object-deletion-joined-owner-retraction --summary --compact
    python tools/query_rti_work.py check --lane tso-object-deletion-joined-owner-retraction --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_object_deletion_joined_owner_retraction\.catch2\.Embedded Request Retraction reconstitutes timestamped deletion only for joined owners$" --output-on-failure

The optimistic-time Flush Queue Request slice is independently buildable at
`flush_queue_request_optimistic_time_catch2.cpp:140`. It passes 58
`HLA_EVOKED` assertions, maps five Requirements-Lab candidates to canonical
2025 clauses 8.12 and 8.12.3, and proves queued timestamped interactions arrive
FIFO before the grant, with actual grant 5 and optimistic time 7:

    python tools/query_rti_work.py focus flush-queue-request-optimistic-time --summary --compact
    python tools/query_rti_work.py trace "Embedded Flush Queue Request flushes queued TSO and reports optimistic time" --summary --compact
    python tools/query_rti_work.py matrix flush-queue-request-optimistic-time --summary --compact
    python tools/query_rti_work.py check --lane flush-queue-request-optimistic-time --summary --compact
    ctest --test-dir <build-dir> -C Debug -R "^umbra\.flush_queue_request_optimistic_time\.catch2\.Embedded Flush Queue Request flushes queued TSO and reports optimistic time$" --output-on-failure

## Fixture and evidence boundaries

The [data/](data/README.md) directory contains small Umbra-owned XML fixtures.
It does not hold unreviewed external corpora. Requirements Lab contracts and
evidence inputs live under [compliance/](../../compliance/README.md), while
generated local evidence belongs under .compliance/ or out/.

The preceding ordinary Attribute Relevance Advisory slice is independently
buildable in `attribute_relevance_advisory_catch2.cpp:265`. It records 106
assertions under both callback models and maps nine Requirements-Lab anchors
directly to clauses 6.23, 6.24, and 10.37.1:

    python tools/query_rti_work.py focus attribute-relevance-scope-transition --summary --compact
    python tools/query_rti_work.py trace "Embedded attribute relevance advisories follow scope transitions" --summary --compact
    python tools/query_rti_work.py matrix "Embedded attribute relevance advisories follow scope transitions" --summary --compact
    python tools/query_rti_work.py check --lane attribute-relevance-scope-transition --summary --compact
    ctest --test-dir <build-dir> -C Release -R "^umbra\.attribute_relevance_advisory\.catch2\.Embedded attribute relevance advisories follow scope transitions$" --output-on-failure

It proves default/no-rate and explicit-rate-bearing Turn Updates On callbacks,
switch-disabled suppression, and active update-rate reissue. The existing
regional callback-entry case remains separate under
`regional-attribute-relevance`.

The joined-federate MOM `HLAobjectInstancesDeleted` projection is independently
buildable at
`joined_federate_mom_deleted_object_count_periodic_catch2.cpp:108`. It records
66 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor directly to
§11.4.1. Direct requests and `HLAsetTiming` prove accepted deletion history
0 → 1 → 2, including a zero periodic snapshot before deletion and two after:

    python tools/query_rti_work.py focus joined-federate-mom-deleted-object-count-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAobjectInstancesDeleted count" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-deleted-object-count-periodic --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-deleted-object-count-periodic --summary --compact
    cmake --build .build --config Release --target umbra_joined_federate_mom_deleted_object_count_periodic_catch2
    ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_deleted_object_count_periodic\.catch2\.Embedded joined-federate MOM exposes HLAobjectInstancesDeleted count$" --output-on-failure

The receiving-federate MOM `HLAobjectInstancesRemoved` projection is
independently buildable at
`joined_federate_mom_removed_object_count_periodic_catch2.cpp:145`. It records
78 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor to §11.4.1
plus 14 official C++ API surfaces. It proves the recipient-scoped committed
Remove Object Instance callback ledger 0 → 1 → 2 and periodic snapshots before
and after removal:

    python tools/query_rti_work.py focus joined-federate-mom-removed-object-count-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAobjectInstancesRemoved counts committed callbacks" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-removed-object-count-periodic --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-removed-object-count-periodic --summary --compact
    cmake --build .build --config Release --target umbra_joined_federate_mom_removed_object_count_periodic_catch2
    ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_removed_object_count_periodic\.catch2\.Embedded joined-federate MOM HLAobjectInstancesRemoved counts committed callbacks$" --output-on-failure

The receiving-federate MOM `HLAobjectInstancesDiscovered` projection is
independently buildable at
`joined_federate_mom_discovered_object_count_periodic_catch2.cpp:127`. It
records 66 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor to
§11.4.1 plus 14 official C++ API surfaces. It proves the eligible discovery
ledger 0 → 2 → 3, including Local Delete Object Instance followed by a
subscription rediscovery:

    python tools/query_rti_work.py focus joined-federate-mom-discovered-object-count-periodic --summary --compact
    python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAobjectInstancesDiscovered counts eligible callbacks" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-discovered-object-count-periodic --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-discovered-object-count-periodic --summary --compact
    cmake --build .build --config Release --target umbra_joined_federate_mom_discovered_object_count_periodic_catch2
    ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_discovered_object_count_periodic\.catch2\.Embedded joined-federate MOM HLAobjectInstancesDiscovered counts eligible callbacks$" --output-on-failure

The official handle-normalization support-services lane is independently
buildable at `handle_normalization_catch2.cpp:60`. It records 52 `HLA_EVOKED`
assertions, maps six Requirements-Lab anchors directly to §§10.1.3 and
10.29–10.33, and covers all five `RTIambassador` normalizers. The case keeps
connection/member fences and typed invalid-designator failures separate from
the successful path, then proves equal execution-scoped point coordinates
through two joined ambassadors and after the owner resigns:

    python tools/query_rti_work.py focus handle-normalization --summary --compact
    python tools/query_rti_work.py trace "Embedded handle normalization supplies stable DDM point-range coordinates" --summary --compact
    python tools/query_rti_work.py matrix object-ddm-ownership --summary --compact
    python tools/query_rti_work.py check --lane handle-normalization --summary --compact
    cmake --build .build --config Release --target umbra_handle_normalization_catch2
    ctest --test-dir .build -C Release -R "^umbra\.handle_normalization\.catch2\.Embedded handle normalization supplies stable DDM point-range coordinates$" --output-on-failure

The joined-federate MOM `HLAobjectInstancesUpdated` request/report lane is
independently buildable at
`joined_federate_mom_object_instances_updated_report_catch2.cpp:137`. It
records 42 `HLA_EVOKED` assertions, maps one §11.4.1 Requirements-Lab anchor,
and exercises 12 official C++ API surfaces. The Subscribe-only request
snapshots accepted updates by registered object class, keeps repeated updates
to one object at one distinct count, and decodes the nested
`HLAobjectClassBasedCounts` value from the reliable RTI-originated report:

    python tools/query_rti_work.py focus joined-federate-mom-object-instances-updated-report --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestObjectInstancesUpdated reports class-grouped counts" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-object-instances-updated-report --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-object-instances-updated-report --summary --compact
    cmake --build .build --config Release --target umbra_joined_federate_mom_object_instances_updated_report_catch2
    ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instances_updated_report\.catch2\.Embedded MOM requestObjectInstancesUpdated reports class-grouped counts$" --output-on-failure

HLA_IMMEDIATE, transport/timestamp variants, the remaining MOM request/report
families, and promotion evidence remain separate lanes.

The joined-federate MOM `HLArequestObjectInstancesThatCanBeDeleted` report lane
is independently buildable at
`joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp:137`.
It records 53 `HLA_EVOKED` assertions, maps one §11.4.1 Requirements-Lab
anchor, and exercises 12 official C++ API surfaces. It derives nested class
counts from live `HLAprivilegeToDeleteObject` ownership, proving two classes
before deletion and one remaining class after an accepted deletion:

    python tools/query_rti_work.py focus joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestObjectInstancesThatCanBeDeleted reports live owner counts" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
    cmake --build .build --config Release --target umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
    ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instances_that_can_be_deleted_report\.catch2\.Embedded MOM requestObjectInstancesThatCanBeDeleted reports live owner counts$" --output-on-failure

Keep HLA_IMMEDIATE, transport/timestamp variants, the remaining MOM
request/report families, and promotion evidence as separate lanes.

The regional explicit-rate Attribute Relevance Advisory slice is independently
buildable at `regional_attribute_relevance_rate_designator_catch2.cpp:105`.
It records 100 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps eight
Requirements-Lab anchors to §§6.23/6.24/10.37.1, and exercises seven official
regional C++ API surfaces. It proves source-region overlap, disjoint-region
suppression, the Off boundary after overlap loss, and one High-rate On after
the original overlap is restored.

    python tools/query_rti_work.py focus regional-attribute-relevance-rate-designator --summary --compact
    python tools/query_rti_work.py trace "Embedded regional attribute relevance advisories retain explicit update-rate designators" --summary --compact
    python tools/query_rti_work.py matrix regional-attribute-relevance-rate-designator --summary --compact
    python tools/query_rti_work.py check --lane regional-attribute-relevance-rate-designator --summary --compact
    cmake --build .build --config Release --target umbra_regional_attribute_relevance_rate_designator_catch2
    ctest --test-dir .build -C Release -R "^umbra\.regional_attribute_relevance_rate_designator\.catch2\.Embedded regional attribute relevance advisories retain explicit update-rate designators$" --output-on-failure

Keep ordinary active-maximum rate reissue, other DDM variants, process
transport, review, validation, interoperability, and conformance as separate
lanes.

The known-class-disabled Attribute Relevance Advisory slice is independently
buildable at `attribute_relevance_known_class_disabled_subscription_catch2.cpp:97`.
It records 35 `HLA_EVOKED` assertions, maps 18 Requirements-Lab anchors to 13
canonical 2025 sections, and exercises 18 official C++ API surfaces. It proves
discovery through `Employee`, then proves that a later Server-only subscription
still produces the owner-directed Turn Updates On advisory when the static
Advisories Use Known Class policy is disabled, followed by Turn Updates Off on
unsubscription.

    python tools/query_rti_work.py focus known-class-disabled --summary --compact
    python tools/query_rti_work.py trace "Embedded attribute relevance advisories use subscriptions when known-class policy is disabled" --summary --compact
    python tools/query_rti_work.py matrix known-class-disabled --summary --compact
    python tools/query_rti_work.py check --lane known-class-disabled --summary --compact
    cmake --build .build --config Release --target umbra_attribute_relevance_known_class_disabled_subscription_catch2
    ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_known_class_disabled_subscription\.catch2\.Embedded attribute relevance advisories use subscriptions when known-class policy is disabled$" --output-on-failure

Keep known-class-enabled, ordinary active-maximum, regional/DDM, process
transport, review, validation, interoperability, and conformance as separate
lanes.

The paired known-class-enabled Attribute Relevance Advisory slice is
independently buildable at
`attribute_relevance_known_class_enabled_subscription_catch2.cpp:97`. It
records 30 `HLA_EVOKED` assertions, maps 15 Requirements-Lab anchors to 13
canonical 2025 sections, and exercises 17 official C++ API surfaces. It proves
the initial Employee advisory remains active while the enabled static policy
suppresses a later Server-only advisory for a subscriber that has not learned
the Server class, then verifies the Employee Off transition:

    python tools/query_rti_work.py focus known-class-enabled --summary --compact
    python tools/query_rti_work.py trace "Embedded attribute relevance advisories honor known class when the static policy is enabled" --summary --compact
    python tools/query_rti_work.py matrix known-class-enabled --summary --compact
    python tools/query_rti_work.py check --lane known-class-enabled --summary --compact
    cmake --build .build --config Release --target umbra_attribute_relevance_known_class_enabled_subscription_catch2
    ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_known_class_enabled_subscription\.catch2\.Embedded attribute relevance advisories honor known class when the static policy is enabled$" --output-on-failure

Keep the disabled-policy, ordinary active-maximum, regional/DDM, process,
review, validation, interoperability, and promotion/conformance queues
separate.

The active-maximum Attribute Relevance Advisory lane is independently
buildable at `attribute_relevance_rate_reissue_catch2.cpp:108`. It records 108
assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps seven Requirements-Lab
anchors to §§6.23/6.24, and proves High/Medium/Low maximum selection,
lower-rate peer suppression, rate-bearing reissue, Low refresh after higher-rate
removal, and final Turn Updates Off delivery:

    python tools/query_rti_work.py focus attribute-relevance-rate-reissue --summary --compact
    python tools/query_rti_work.py trace "Embedded attribute relevance advisories reissue turn-on when the active update rate changes" --summary --compact
    python tools/query_rti_work.py matrix attribute-relevance-rate-reissue --summary --compact
    python tools/query_rti_work.py check --lane attribute-relevance-rate-reissue --summary --compact
    cmake --build .build --config Release --target umbra_attribute_relevance_rate_reissue_catch2
    ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_rate_reissue\.catch2\.Embedded attribute relevance advisories reissue turn-on when the active update rate changes$" --output-on-failure

Keep regional/DDM variants, process transport, and promotion evidence as
separate lanes.

The latest known/NULL object-information MOM lane is independently buildable
at `joined_federate_mom_object_instance_information_report_catch2.cpp:146`.
It records 71 `HLA_EVOKED` assertions, maps one §11.4.1 Requirements-Lab
anchor, and exercises 13 official C++ API surfaces. The Subscribe-only
`HLArequestObjectInstanceInformation` decodes the nested
`HLAattributeHandleList`, proves registered/known classes and the registering
federate's owned attributes, then uses Local Delete Object Instance to verify
the MIM NULL response shape:

    python tools/query_rti_work.py focus joined-federate-mom-object-instance-information-report --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestObjectInstanceInformation reports known and NULL object state" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-object-instance-information-report --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-object-instance-information-report --summary --compact
    cmake --build .build --config Release --target umbra_joined_federate_mom_object_instance_information_report_catch2
    ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instance_information_report\.catch2\.Embedded MOM requestObjectInstanceInformation reports known and NULL object state$" --output-on-failure

Keep HLA_IMMEDIATE, transport variants, the remaining MOM request/report
families, and promotion evidence as separate lanes.

The reflected-object MOM request/report lane is independently buildable at
`joined_federate_mom_object_instances_reflected_report_catch2.cpp:160`. It
records 51 `HLA_EVOKED` assertions, maps one §11.4.1 Requirements-Lab anchor,
and exercises 14 official C++ API surfaces. The Subscribe-only
`HLArequestObjectInstancesReflected` request is scoped to the requesting /
receiving federate's accepted application reflection ledger; repeated
reflections of one object remain one distinct count, and two registered
classes decode from the nested `HLAobjectClassBasedCounts` value:

    python tools/query_rti_work.py focus joined-federate-mom-object-instances-reflected-report --summary --compact
    python tools/query_rti_work.py trace "Embedded MOM requestObjectInstancesReflected reports distinct reflected instances" --summary --compact
    python tools/query_rti_work.py matrix joined-federate-mom-object-instances-reflected-report --summary --compact
    python tools/query_rti_work.py check --lane joined-federate-mom-object-instances-reflected-report --summary --compact
    cmake --build .build --config Release --target umbra_joined_federate_mom_object_instances_reflected_report_catch2
    ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instances_reflected_report\.catch2\.Embedded MOM requestObjectInstancesReflected reports distinct reflected instances$" --output-on-failure

Keep HLA_IMMEDIATE, transport/timestamp variants, the remaining MOM
request/report families, and promotion evidence as separate lanes.
