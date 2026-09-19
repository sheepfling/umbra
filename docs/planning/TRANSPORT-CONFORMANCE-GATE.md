# Transport and conformance evidence gate

Status: design gate for the next transport/conformance slice. The current
embedded endpoint now has ordinary, timestamped, parameterized-envelope,
connection-loss, object-registration, named-registration, ordinary
object-class-subscription, and ordinary attribute-update installed-profile
process evidence, including a read-only Query Logical Time baseline;
it remains foundation evidence until the
CTest/JUnit/configuration artifacts receive protected review. It must not be
promoted as conformance by itself.

## Current executable baseline

Run the bounded lane before changing the transport implementation:

```powershell
.build-fom-services\Debug\umbra_ieee1516_2025_catch2.exe "[transport]" --reporter compact
python tools/query_rti_work.py check --lane transport --summary --compact
.build-fom-services\Debug\umbra_ieee1516_2025_catch2.exe "[process-boundary]" --reporter compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
python tools/query_rti_work.py test "Private process service binds create join and receive-order interaction to the federation registry" --summary --compact
python tools/query_rti_work.py test "Private registry-bound service exchanges federation traffic across independently launched processes" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Create, Join, and Resign through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador obtains the selected logical-time factory after a configured process join" --summary --compact
python tools/query_rti_work.py test "RTIambassadors resolve federate handles and names through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes instance transportation type change and query through a configured process endpoint" --summary --compact
python tools/query_rti_work.py focus process-transportation-instance-control --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-instance-integration --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador routes instance transportation type change and query through a configured process endpoint$" --output-on-failure
python tools/query_rti_work.py test "RTIambassador routes interaction transportation type change and query through a configured process endpoint" --summary --compact
python tools/query_rti_work.py focus process-transportation-interaction-control --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-interaction-integration --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador routes interaction transportation type change and query through a configured process endpoint$" --output-on-failure
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-timestamped-regional-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-timestamped-regional-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve a timestamped regional interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-timestamped-regional-interaction-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-timestamped-regional-interaction-control --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassadors preserve a timestamped regional interaction transportation override through a configured process endpoint$" --output-on-failure
python tools/query_rti_work.py test "RTIambassador resolves interaction and parameter handles through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador publishes object-class attributes and registers an object through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador reserves a name and registers a named object through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador projects the 2025 region lifecycle and regional registration through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes a remote regional subscription and scoped update through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador preserves a timestamped regional update through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador removes a regional subscription through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador suppresses a disjoint regional update through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers regional Attribute Relevance Advisory subscription transitions through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers an initial regional Attribute Relevance Advisory after discovery through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers regional Attribute Relevance Advisory subscription transitions through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes ordinary interaction declarations through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador receives a process interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py test "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers a directed interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py focus process-time-advance-malformed-encoding --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
python tools/query_rti_work.py focus process-time-advance-federation-scheduler --summary --compact
python tools/query_rti_work.py trace "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
python tools/query_rti_work.py focus process-tso-interaction-before-grant --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a deferred timestamped process interaction before the grant$" --output-on-failure
python tools/query_rti_work.py focus process-time-advance-next-message-queued-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors select queued timestamped process messages for next-message advances$" --output-on-failure
python tools/query_rti_work.py lane federate.callback.request-retraction --summary --compact
python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
python tools/query_rti_work.py lane rti.service.subscribe-object-class-attributes --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Update Attribute Values through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers a process Update Attribute Values event through the official Reflect callback" --summary --compact
python tools/query_rti_work.py test "RTIambassador selects a configured tcp process endpoint through the official address field" --summary --compact
python tools/query_rti_work.py test "RTIambassador rejects malformed tcp process addresses before connecting" --summary --compact
cmake --build <build-dir> --config Debug --target umbra_test_installable_package
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-connection-loss --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-connection-loss-delete-objects --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-parameterized --output-on-failure
  ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-object-registration --output-on-failure
  ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-named-registration --output-on-failure
  ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-attribute-update --output-on-failure
  ctest --test-dir <build-dir>/package-smoke-consumer -C Debug --output-junit <build-dir>/package-smoke-consumer/Testing/package-process.xml --output-on-failure
  python tools/verify_process_package_lanes.py --ctest ctest --test-dir <build-dir>/package-smoke-consumer --index docs/planning/ROADMAP-INDEX.json --config Debug --junit <build-dir>/package-smoke-consumer/Testing/package-process.xml
```

The installable process-package handoff is queryable from the indexed work
card and emits the deterministic
`<build-dir>/package-smoke-consumer/Testing/package-process.xml` artifact.
`verify_process_package_lanes.py --junit` checks that every indexed process
projection appears exactly once and that the CTest report has no failures or
errors; the artifact is evidence for review, not a conformance claim.

The current query baseline is 155 mapped `process-boundary` plan rows / 6,717
indexed Catch2 assertions (6,869 assertions recorded on the individual plan
rows), all mapped and source-located. The stable label has
207 registered CTest executions; the JUnit target executes the 119
independently buildable registrations. The current merged JUnit report contains
214 emitted section-level testcases and 5,458 assertions with zero failures/errors.
The aggregate umbrella registration remains CTest-only because its federation-
management translation unit is not an evidence source.
The newest HLA_IMMEDIATE restored ownership-assumption slice is independently
queryable at
`cpp/tests/attribute_ownership_acquisition_catch2.cpp:3437`: 52 assertions,
nine direct Requirements-Lab anchors, seven canonical 2025 sections, and 11
official C++ API surfaces. It retains the pending reservation while callbacks
are disabled and uses a post-enable class lookup only as a receive-order fence;
the already-pushed ownership-assumption companion remains a separate lane.
The newest timestamped regional interaction-transportation slice contributes
122 assertions, 27 direct Lab anchors, 15 canonical 2025 sections, and 21
official C++ API surfaces; it is green under both callback models and remains
private foundation evidence until the separate review, validation,
interoperability, and conformance gates are satisfied.
The local-delete codec
adds 9 direct assertions, the private registry integration 44, and the public
two-federate endpoint integration 18; receive-order Delete Object Instance adds
a 25-assertion codec contract and a 25-assertion public endpoint/removal-
callback integration; the timestamped public endpoint slice adds 131 assertions
across regulated and non-regulated variants under HLA_EVOKED and HLA_IMMEDIATE
and returns the valid public `MessageRetractionHandle` backed by the process
execution identity only for the time-regulating producer; the timestamped regional Update Attribute
Values endpoint adds 86 assertions under both models; the regional
subscription-removal endpoint adds 112 assertions under both models; the disjoint
regional-update endpoint adds 50, the remote regional subscription/update
endpoint adds 66, the regional-registration endpoint adds 34, and the
object-registration endpoint adds 20 under both models, and the public object-class
subscription endpoint adds 24 under both callback models; the public object-discovery
endpoint adds 44 assertions under both callback models and exercises the official
`discoverObjectInstance` callback. The private registry-binding case now adds a
four-assertion Query Logical Time protocol round-trip plus a ten-assertion
official encoded `Enable Time Regulation`/retained-state check, and the public
Create/Join/Resign case adds a two-assertion official-factory reconstruction
check plus a four-assertion callback-gating check. The focused remote
`getTimeFactory` slice adds nine assertions and verifies that the selected
logical-time implementation crosses Join without consulting a process-local
registry; FOM-catalog distribution remains a separate protocol slice. The
federate-identity lookup slice adds eleven assertions across two configured
ambassadors, proving active-name lookup and post-resignation designator-name
retention through the same process service. These
are bounded process role/state baselines. The process TAR pending-role slices add 28 callback-gated
assertions, the malformed logical-time decode fence adds 9 assertions mapped to
clause `8.8.3`, and the two-federate process federation scheduler adds 30
assertions for deferred constrained TAR, regulator-driven release, unsolicited
grant transport, and Evoke delivery. The focused timestamped process
interaction-before-grant slice adds 62 `HLA_EVOKED` assertions, 20
Requirements-Lab anchors, and 13 canonical 2025 sections; query it with
`process-tso-in-transit`. The public pre-grant attribute-retraction slice is
now independently complete with 51 `HLA_EVOKED` assertions, 20 Lab anchors,
13 canonical sections, and 16 official C++ API surfaces:

```powershell
python tools/query_rti_work.py focus process-tso-attribute-retraction-before-callback --summary --compact
python tools/query_rti_work.py trace "RTIambassadors suppress a retracted timestamped process attribute before the callback" --summary --compact
python tools/query_rti_work.py check --lane process-tso-attribute-retraction-before-callback --summary --compact
```

The directed TSO callback-gating slice is independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-directed-interaction-callback-gating --summary --compact
python tools/query_rti_work.py focus process-tso-directed-interaction-callback-gating --summary --compact
python tools/query_rti_work.py trace "RTIambassadors retain a timestamped directed interaction while callbacks are disabled" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-directed-interaction-callback-gating --summary --compact
python tools/query_rti_work.py check --lane process-tso-directed-interaction-callback-gating --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors retain a timestamped directed interaction while callbacks are disabled$" --output-on-failure
```

It records 62 `HLA_EVOKED` assertions, 21 Requirements-Lab anchors, 16
canonical 2025 sections, and 18 official C++ API surfaces. It keeps the
timestamped directed interaction and matching grant queued while callbacks are
disabled, then delivers the directed interaction before the grant after
re-enable with TIMESTAMP/TIMESTAMP metadata. It is private foundation evidence,
not a conformance claim.

The preceding process TSO fan-out slice is independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-interaction-fanout-integration --summary --compact
python tools/query_rti_work.py focus process-tso-interaction-fanout --summary --compact
python tools/query_rti_work.py trace "RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-interaction-fanout-integration --summary --compact
python tools/query_rti_work.py check --lane process-tso-interaction-fanout --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant$" --output-on-failure
```

It records 133 `HLA_EVOKED` assertions, 20 Requirements-Lab anchors, 13
canonical 2025 sections, and 15 official C++ API surfaces. It queues two
same-timestamp interactions and one later timestamped interaction once, fans
each out to two subscribed receivers, proves FIFO delivery across the
timestamp-five and timestamp-seven grants at the shared GALT/LITS boundary,
and acknowledges all six deliveries through the process seam. The preceding
multiple-message FIFO lane remains independently selectable with 51 assertions
at `cpp/tests/ieee1516_2025_connection_catch2.cpp:23633`. Changed-lookahead,
directed/regional DDM, resignation, save/restore,
package/JUnit, protected review, validation, interoperability, and
conformance remain separate lanes.

The regional callback-gating companion is now green at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:26210` with 69 `HLA_EVOKED`
assertions, 28 Lab anchors, 16 canonical 2025 sections, and 20 official C++
API surfaces. It uses one sender/receiver regional overlap, disables the
receiver callback gate before the timestamped send is admitted, proves that
the interaction and matching grant remain withheld, then re-enables callbacks
and observes exactly one interaction before one grant. Its focused handles are:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-regional-interaction-callback-gating --summary --compact
python tools/query_rti_work.py focus process-tso-regional-interaction-callback-gating --summary --compact
python tools/query_rti_work.py trace "RTIambassadors retain a timestamped regional interaction while callbacks are disabled" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-regional-interaction-callback-gating --summary --compact
python tools/query_rti_work.py check --lane process-tso-regional-interaction-callback-gating --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors retain a timestamped regional interaction while callbacks are disabled$" --output-on-failure
```

This remains private foundation evidence. CTest/JUnit packaging, protected
review, interoperability, Requirements-Lab validation, and conformance are
still separate gates.

Multiple-message ordering, directed/regional TSO, save/restore, package/JUnit,
validation, interoperability, and conformance remain separate.
The queued-TSO NMR/NMRA companion is a separate two-federate proof with 101
`HLA_EVOKED`/`HLA_IMMEDIATE` assertions and 19 Lab anchors across 12 canonical
2025 sections;
it selects timestamp 5, then 8, from the receiver queue and checks each
interaction callback before its grant. Keep GALT/LITS, Flush Queue, retraction,
multi-recipient ordering, save/restore, package/review, validation,
interoperability, and conformance in their own lanes.
The immediate-callback GALT/LITS companion is independently selectable with
14 `HLA_IMMEDIATE` assertions, five Lab anchors, and four canonical 2025
sections:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-immediate-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-immediate --summary --compact
python tools/query_rti_work.py trace "RTIambassador queries GALT and LITS through a configured process endpoint with immediate callbacks" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-immediate-integration --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-immediate --summary --compact
```

It proves the Time Regulation Enabled callback crosses the process endpoint
before the immediate call returns and that undefined GALT/LITS preserve caller
output values; multi-federate bounds, queued/in-transit TSO, grants,
save/restore, and conformance remain separate.
Each completed slice records its exact
per-case assertion count in `ROADMAP-INDEX.json`.

The available multi-federate companion is independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-multi-federate-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-multi-federate --summary --compact
python tools/query_rti_work.py trace "RTIambassadors expose defined GALT and LITS across configured process federates" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-multi-federate-integration --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-multi-federate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors expose defined GALT and LITS across configured process federates$" --output-on-failure
```

This 24-assertion `HLA_IMMEDIATE` case maps the same five Lab anchors to four
canonical 2025 sections, proves self-regulator exclusion, and proves that an
observer receives defined GALT/LITS from the other regulator's lookahead.
Queued/in-transit TSO, grants, save/restore, package/JUnit, validation, and
conformance remain separate. The queued-TSO LITS companion is independently
selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-queued-tso-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-queued-tso --summary --compact
python tools/query_rti_work.py trace "RTIambassador queries LITS from queued timestamped process input after regulator disable" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-queued-tso-integration --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-queued-tso --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries LITS from queued timestamped process input after regulator disable$" --output-on-failure
```

This 31-assertion `HLA_IMMEDIATE` case maps the same five Lab anchors to four
canonical sections, queues timestamped input for a constrained observer,
preserves undefined GALT after regulator disable, and reports the queued
timestamp as defined LITS. In-transit TSO, zero-lookahead epsilon, grants,
retraction, save/restore, package/JUnit, validation, and conformance remain
separate.

The zero-lookahead GALT/LITS companion is independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-zero-lookahead-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-zero-lookahead --summary --compact
python tools/query_rti_work.py trace "RTIambassador exposes a zero-lookahead exclusive GALT and LITS boundary through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-zero-lookahead-integration --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-zero-lookahead --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador exposes a zero-lookahead exclusive GALT and LITS boundary through a configured process endpoint$" --output-on-failure
```

This 28-assertion `HLA_EVOKED` case maps six Lab anchors to four canonical
sections plus six official C++ API surfaces. It advances a zero-lookahead
regulator through an evoked Time Advance Grant and proves the observer sees
the exclusive integer boundary. In-transit TSO, queued-TSO delivery, grant
scheduling, retraction, save/restore, package/JUnit, validation, and
conformance remain separate.
The latest reverse-FOM lookup slice is independently selectable:

```powershell
python tools/query_rti_work.py focus reverse-fom-lookup --summary --compact
python tools/query_rti_work.py trace "RTIambassador reports reverse FOM lookup errors through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix reverse-fom-lookup --summary --compact
python tools/query_rti_work.py check --lane reverse-fom-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador reports reverse FOM lookup errors through a configured process endpoint$" --output-on-failure
```

The lane now contains four source-located cases and 76 aggregate assertions
(m102 contributes 20; m103 contributes 24; m104 contributes 16; m105
contributes 16) under both `HLA_EVOKED` and `HLA_IMMEDIATE`. The m105 public
process case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:13738` records
16 assertions and maps six Lab requirements to canonical 2025 clauses `9.1.2`,
`10.19`, and `10.20.4`; it exercises four official C++ API surfaces and keeps
unknown-name/invalid-handle error behavior separate from m104's successful
dimension/transportation round trips. The m104, m103, m102, and m101 cases
remain available through exact title/lane handles; multi-federate declaration
management, package, review, validation, interoperability, and conformance
evidence remain separate.
The latest process callback-ordering slice is independently selectable:

```powershell
python tools/query_rti_work.py focus process-multi-recipient-callback-ordering --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-multi-recipient-callback-ordering --summary --compact
python tools/query_rti_work.py check --lane process-multi-recipient-callback-ordering --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint$" --output-on-failure
```

The m107 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:14059` records
79 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps ten Lab anchors to six canonical 2025
subsections (`4`, `5.1.4`, `6.12`, `6.12.4`, `6.13`, and `10.60.6`), and
exercises five official C++ API surfaces. Two subscribed receivers each observe
their own FIFO tag sequence with preserved parameters/producer identity, one
receive callback per Evoke Callback, and sender exclusion. Immediate delivery,
callback-disable, timestamped/region/directed fanout, package/JUnit, protected
review, validation, interoperability, and conformance remain separate.

The newest process TSO/DDM slice is independently selectable:

```powershell
python tools/query_rti_work.py focus timestamped-process-regional-interaction --summary --compact
python tools/query_rti_work.py trace m109.embedded-process-tso-regional-interaction --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane timestamped-process-regional-interaction --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a timestamped regional interaction through a configured process endpoint$" --output-on-failure
```

The m109 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:15148` records
71 `HLA_EVOKED` assertions, 28 Lab anchors, 16 canonical 2025 subsections,
and 20 official C++ API surfaces. It verifies overlap-qualified timestamped
regional process delivery, time-advance gating, conveyed source-region
metadata, and timestamp/order/retraction reconstruction. Keep the broader
callback-disable, directed, relaxed-DDM, package, review, validation,
interoperability, and conformance lanes separate.

The preceding process DDM slice is independently selectable:

```powershell
python tools/query_rti_work.py focus process-multi-recipient-regional-interaction --summary --compact
python tools/query_rti_work.py trace m108.embedded-process-multi-recipient-regional-interaction --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-multi-recipient-regional-interaction --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint$" --output-on-failure
```

The m108 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:14505` records
126 assertions under both official callback models, maps thirteen Lab anchors
to seven canonical 2025 subsections, and exercises thirteen official C++ API
surfaces. It proves overlap-filtered delivery to two disjoint regional
recipients, send-time source-region metadata, and sender exclusion. Keep
timestamped/retraction, directed, relaxed-DDM, callback-disable, package/JUnit,
protected review, validation, interoperability, and conformance in separate
lanes.

The preceding process DDM metadata slice is independently selectable:

```powershell
python tools/query_rti_work.py focus process-available-dimensions-hierarchy --summary --compact
python tools/query_rti_work.py trace process-available-dimensions-hierarchy --summary --compact
python tools/query_rti_work.py matrix process-available-dimensions-hierarchy --summary --compact
python tools/query_rti_work.py check --lane process-available-dimensions-hierarchy --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador resolves available FOM dimensions through a configured process endpoint$" --output-on-failure
```

The m106 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:13886` records
28 assertions under both callback models, maps four Lab anchors to clause
`9.1.2`, and covers two official C++ API surfaces. It verifies inherited
object-class dimensions, an empty interaction-class set, unknown classes, and
official invalid-handle mapping through the federation-owned process catalog.
Process region lifecycle/routing and multi-federate hierarchy behavior remain
separate lanes.
The completed public object-discovery slice is indexed in that source file with
three requirement ids, three canonical 2025 sections, six official API surfaces,
and 44 assertions under both callback models. The connection support-types,
filesystem service-report configuration, AttributeHandle, callback-route,
immediate-callback reentrancy, callback-disable, evoked one-at-a-time,
concurrent-serialization, disabled-backlog, Evoke Multiple FIFO,
evoked-disabled-pending, callback-session-close, callback-session concurrent-invocation,
and callback-session active-close baselines are independently indexed with mapped or
explicit internal-boundary dispositions. The external IEEE 1516-2010 ordered-
catalog compatibility case is now indexed with 13 passing assertions and
clauses `4.5.5`/`4.11.4`, explicitly separate from 2025 service conformance.
The strict 2025-mode rejection and mixed-edition composer guard are also indexed
as two- and seven-assertion compatibility-only clause-4 slices. The ambiguous
202x edition-setting guard is indexed as a one-assertion explicit
no-standalone-Lab-surface policy row. The RPR full-family object-registration
and custom-payload compatibility slices and the official FederateHandle
SDK-consumability/encoding baselines are now indexed with explicit
dispositions, and the private federate lifecycle state-machine slices are now
mapped. The regional-unpublish update-region lifetime slice is indexed with 17
assertions and 18 requirement anchors. The receive-order deletion update-region
lifetime slice is indexed with 17 assertions and 20 requirement anchors. The
final object-removal callback update-region lifetime slice is indexed with 27
assertions and 21 requirement anchors. The Join advisory-switch seed slice is
indexed with 16 assertions and 13 requirement anchors. The Join-time explicit
NoAction automatic-resign slice is indexed with 7 assertions and 9 requirement
anchors. The completed federation-management
order and transportation MOM service-classification case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:34242`; that slice is
indexed with 66 assertions and 19 requirement anchors. The joined-federate MOM
regional discovery slice is indexed with 124 assertions and 17 requirement
anchors. The next bounded source action is the unplanned federation-management
regional Request Attribute Value Update provider-response case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:3786`; the
joined-federate MOM/FOM-module snapshot slice is indexed with 12 assertions and
6 requirement anchors, and the report-file identity save/restore slice is
indexed with 46 assertions and 18 requirement anchors;
its exact source location and `[federation-management]` filter are emitted by
`python tools/query_rti_work.py next --pointer` and it must be mapped
as provider-response and regional-overlap evidence before it is treated as
evidence.
The restored-baseline provider-response slice is indexed with 44 assertions
and 20 requirement anchors. The timestamped Delete Object Instance retraction
slice at `cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4305` is
mapped but marked `failing-test-harness`: the HLA_EVOKED run stops at line 4354
after 14/15 assertions on a stale `EvokeCallback` expectation. Repair that
harness before promoting deletion/retraction/removal evidence. The known-class-
disabled attribute-relevance case at
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
sections, and 15 official C++ API surfaces. The next source action is the
unplanned restored-baseline timestamped regional Update Attribute Values
reflection-order case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:6297`.
The 40-case query count is the unique mapped plan/test-declaration count; when
both the aggregate and dedicated connection executables are configured, the
`process-boundary` CTest label intentionally runs both registrations. The m24
private and
m25 public Attribute Relevance Advisory, m26 callback-entry suppression, m27
public regional association-transition, and m28 public regional subscription
removal/restoration, m29 initial regional registration/discovery advisory, m33
public active-rate reissue, and m34 public regional explicit-rate designator
slices are directly queryable by their exact test titles; the directed process
slice proves timestamped Retract-before-receive suppression and exactly one
legal post-delivery Request Retraction callback under both callback models. The
installed-package directed/retraction consumer is now
green as its own `package-process-directed-retraction` lane. The installable
package profile is
now a named `umbra_test_installable_package` target and `installable-package`
CTest label; it stages the exported runtime set, validates
`share/umbra_rti/umbra_rti-profile.json` and the reviewed 1516.2 resources,
then builds and runs a clean downstream consumer. In the embedded profile,
that consumer also runs `umbra_rti_package_process_consumer` under the
`package-process` label: two independently created public ambassadors use the
staged package for Connect/Create/Join, server-owned object-class/interaction/
parameter lookup,
receive-order Send Interaction, Evoke delivery, and NoAction Resign.
The same installed executable also runs
`umbra_rti_package_process_timestamped_consumer` under the
`package-process-timestamped` label; it verifies the official timestamped
`FederateAmbassador` callback, encoded `HLAinteger64Time`, RECEIVE order, and
the intentionally absent process-slice retraction handle. The same installed
executable also runs `umbra_rti_package_process_connection_loss_consumer` under
the `package-process-connection-loss` label; it closes the receiver transport,
verifies the official `connectionLost` callback through
`EvokeMultipleCallbacks`, and proves the surviving sender can continue using
the federation. The installed executable also runs
`umbra_rti_package_process_connection_loss_delete_objects_consumer` under
the `package-process-connection-loss-delete-objects` label; it configures
`DELETE_OBJECTS`, closes the receiver transport, verifies the surviving
sender receives exactly one official `removeObjectInstance` callback with the
empty tag and lost producer, and verifies the deleted object name is no
longer resolvable before a normal sender resign. The installed executable
also runs
`umbra_rti_package_process_parameterized_consumer` under the
`package-process-parameterized` label; it resolves the server-owned
`HLAobjectRoot.Customer` class and `TimelinessOk`, then checks the exact
object/parameter handle/value envelope at the receiver.
The connection-loss projections use an explicit `connection-loss-ready.ok`
handshake from the public consumer before the fixture closes the receiver
socket; this keeps the two processes from waiting on one another and makes
the failure paths bounded. The exact package gate handles are indexed as
`installable-package`, `package-process-connection-loss`, and
`package-process-connection-loss-delete-objects` in
`ROADMAP-INDEX.json`.
That package handle reuses the mapped embedded C++ DELETE_OBJECTS companion's five
Requirements-Lab ids and five canonical 2025 section keys; query the backing
plan for assertion-level traceability and keep the package run as installed
process foundation evidence.
The installed executable also runs
`umbra_rti_package_process_object_registration_consumer` under the
`package-process-object-registration` label; it resolves the official object
and attribute handles, publishes `HLAprivilegeToDeleteObject`, registers an
unnamed object, and uses `DELETE_OBJECTS` when the publishing sender resigns
while a second federate remains joined.
The installed executable also runs
`umbra_rti_package_process_named_registration_consumer` under the
`package-process-named-registration` label; it reserves a legal name through
the official callback surface, registers the named object, and verifies
duplicate registration (`ObjectInstanceNameInUse`) and illegal reservation
(`IllegalName`).
The installed executable also runs
`umbra_rti_package_process_attribute_update_consumer` under the
`package-process-attribute-update` label; it uses the installed public
`RTIambassador::subscribeObjectClassAttributes` and
`RTIambassador::updateAttributeValues` surfaces plus the official
`FederateAmbassador::reflectAttributeValues` callback, with automatic
process-backed `FederateAmbassador::discoverObjectInstance` delivery. The
installed executable also runs `umbra_rti_package_process_directed_retraction_consumer`
under `package-process-directed-retraction`; it registers a nested Server target,
crosses the public directed declaration APIs, receives one timestamped directed
interaction through the official callback, calls `Retract`, verifies one matching
post-delivery `requestRetraction` callback, and then verifies a second
pre-delivery `Retract` suppresses the directed callback and any second
retraction callback.
The installed executable also runs
`umbra_rti_package_process_federation_save_restore_consumer` under
`package-process-federation-save-restore`; it uses one installed public
ambassador to drive Request Federation Save, the save callbacks, Request
Federation Restore, and the ordered restore-success callback sequence through
the process boundary before a normal Resign.
The installed executable also runs
`umbra_rti_package_process_federation_save_restore_failure_consumer` under
`package-process-federation-save-restore-failure`; it repeats the same public
save/restore setup and verifies `Federate Restore Not Complete` produces the
official `Federation Not Restored` callback with
`FEDERATE_REPORTED_FAILURE_DURING_RESTORE` before normal Resign.
The installed executable also runs
`umbra_rti_package_process_federation_save_restore_abort_consumer` under
`package-process-federation-save-restore-abort`; it verifies the explicit
`Abort Federation Restore` service and the terminal `RESTORE_ABORTED` callback
reason through the installed public API.
The installed executable also runs
`umbra_rti_package_process_federation_save_restore_status_consumer` under
`package-process-federation-save-restore-status`; it verifies
`Query Federation Restore Status`, the in-progress `FEDERATE_RESTORING`
status response, and the subsequent restore completion through the installed
public API.
Before those thirteen consumers execute,
`verify_process_package_lanes.py` compares their exact names and labels with the
thirteen `next_process_package_*` handles and matching `mapping.lane_handles` entries
in `ROADMAP-INDEX.json`; a catalog mismatch
fails the installable-package gate. The same run writes
`<build-dir>/package-smoke-consumer/Testing/package-process.xml` with CTest's
JUnit schema, and the verifier checks that all thirteen indexed process names occur
exactly once with zero reported failures or errors. `coverage --lane transport` reports
82 transport plan entries (76 mapped, 82 source-located, and no retained
source-drift rows; 6 have no Lab requirement mapping); use the
bounded query and lane check to select work without reopening the full catalog.
The public connection-loss portion of the first reviewable gate is now green;
protected review and final conformance promotion remain open.

The process endpoint, service-dispatch, and registry-binding baselines are
directly queryable as
`Private process transport exchanges framed data after endpoint handshake` and
`Private process service dispatch correlates federation operations over framed
data`, plus `Private process service binds create join and receive-order
interaction to the federation registry`. The active implementation starting
point is now the installable-profile process gate for the independently
launched registry-bound path, not another Requirements-Lab scan. The
independent-process baseline is directly queryable as `Private registry-bound
service exchanges federation traffic across independently launched processes`;
its sender and receiver now use the private `ProcessFederationClient` seam;
the receiver consumes a pushed event frame and exercises the official C++
callback bridge. The staged package consumer now proves the corresponding
ordinary, timestamped, parameterized-envelope, connection-loss,
object-registration, named-registration, and ordinary attribute-update/
discovery/Reflect public client paths. The result remains foundation evidence until the
reproducible CTest/JUnit/configuration artifacts are protected-reviewed. The
public-address/process slice is now exercised by
the mapped Catch2 cases (Connect, malformed-address rejection,
Create/Join/Resign, Send Interaction, server-owned object-class/interaction/
parameter handle lookup, object-class publication/ordinary subscription/unnamed and
reservation-consuming named registration, ordinary
interaction declaration state, ordinary plus timestamped Receive/Evoke
delivery, ordinary Update Attribute Values, official reservation callback, and
official Reflect delivery, and the 2025 dimension/region lifecycle plus
regional registration);
its ordinary Receive/Evoke case also covers Evoke Multiple and
EVOKED callback enable/disable gating. It remains foundation evidence because
packaging and protected review are not complete.

## First independently reviewable gate

The first gate is a two-process, installable-profile interoperability slice.
It is not complete until all of the following are true:

1. Two independently started RTI/federate processes connect through the same
   configured transport endpoint; no process-local singleton registry is used
   for federation membership or message delivery.
2. The processes create/join the same federation, publish and subscribe using
   the official IEEE 1516.1-2025 C++ binding, and exchange at least one
   receive-order and one timestamped interaction or attribute update.
3. A process termination or transport close produces the specified
   connection-loss lifecycle and bounded automatic-resign behavior in the
   surviving process, with callback ordering captured by the C++ test.
4. The test runs in the installable profile without internal fault hooks or
   development-only registry replacement.
5. The run emits a reproducible CTest/Catch2 result, a JUnit artifact, the
   endpoint/configuration manifest, and the exact source/requirement/section
   mapping used for review.

The first slice should be small enough to review as one contract: endpoint
handshake and identity, federation membership, one message path, clean close,
and connection-loss recovery. DDM, save/restore, service-report files, and
additional SISO FOMs remain separate gates.

## Evidence levels

- `embedded-development`: current process-local endpoint and focused Catch2
  lane; valid for incremental behavior work only.
- `process-boundary`: two or more independently launched processes with
  captured endpoint identity and callback evidence.
- `installable-package`: the same test against the packaged public library and
  headers, with no private test seam.
- `protected-review`: immutable artifacts, reviewer identity, source revision,
  and the mapping manifest retained alongside the result.
- `conformance`: only after the applicable 2025 requirements and standard
  subsections have all been exercised by the preceding evidence levels.

The current roadmap pointer has advanced through the ordinary, timestamped,
parameterized, connection-loss, object-registration, and ordinary
attribute-update/Reflect public installable-package runs; the first gate still needs
protected review before it can be called interoperable or conformant. Do not
change the Requirements Lab numbering merely because this design artifact was
added; the Lab remains the source of requirement and subsection handles.

The current private seam is `cpp/src/internal/federation/embedded_transport.hpp`:
`TransportConnection` is the runtime-facing contract and
`EmbeddedTransportConnection` is its process-local implementation. This seam
is intentionally below the public IEEE binding and is not itself a
conformance claim.
