# Transport and conformance evidence gate

Status: design gate for the next transport/conformance slice. The current
embedded endpoint now has ordinary, timestamped, parameterized-envelope,
connection-loss, object-registration, named-registration, ordinary
object-class-subscription, and ordinary attribute-update installed-profile
process evidence;
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
python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
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
python tools/query_rti_work.py lane federate.callback.request-retraction --summary --compact
python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
python tools/query_rti_work.py lane rti.service.subscribe-object-class-attributes --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Update Attribute Values through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers a process Update Attribute Values event through the official Reflect callback" --summary --compact
python tools/query_rti_work.py test "RTIambassador selects a configured tcp process endpoint through the official address field" --summary --compact
python tools/query_rti_work.py test "RTIambassador rejects malformed tcp process addresses before connecting" --summary --compact
cmake --build <build-dir> --config Debug --target umbra_test_installable_package
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-connection-loss --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-parameterized --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-object-registration --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-named-registration --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-attribute-update --output-on-failure
python tools/verify_process_package_lanes.py --ctest ctest --test-dir <build-dir>/package-smoke-consumer --index docs/planning/ROADMAP-INDEX.json --config Debug
```

The current query baseline is 38 executable `process-boundary` cases / 1617
focused-JUnit assertions, all mapped and source-located; the local-delete codec
adds 9 direct assertions, the private registry integration 44, and the public
two-federate endpoint integration 18; receive-order Delete Object Instance adds
a 25-assertion codec contract and a 25-assertion public endpoint/removal-
callback integration; the timestamped public endpoint slice adds 64 assertions
under HLA_EVOKED and HLA_IMMEDIATE. Each completed slice records
its exact per-case assertion count in `ROADMAP-INDEX.json`.
The completed timestamped Delete Object Instance slice is indexed in that
source file. The next bounded source slice is explicitly planned in that index:
the HLA_IMMEDIATE companion for timestamped regional Update Attribute Values
through the configured process endpoint. Its ten requirement ids, six canonical
2025 sections, official API surfaces, source-file target, and `[process-boundary]`
filter are emitted by
`python tools/query_rti_work.py next --summary --compact`; it is not counted
as executable evidence until the C++ case is added.
The 38-case query count is the unique mapped plan/test-declaration count; when
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
`umbra_rti_package_process_parameterized_consumer` under the
`package-process-parameterized` label; it resolves the server-owned
`HLAobjectRoot.Customer` class and `TimelinessOk`, then checks the exact
object/parameter handle/value envelope at the receiver.
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
Before those eight consumers execute,
`verify_process_package_lanes.py` compares their exact names and labels with the
eight `next_process_package_*` handles in `ROADMAP-INDEX.json`; a catalog mismatch
fails the installable-package gate. `coverage --lane transport` reports
57 transport plan entries (53 mapped, 52 source-located, and five retained
historical source-drift rows); use the
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
