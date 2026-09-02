# Python API coverage plan

## Scope and accounting

The target is the edition-specific public package
`hla.rti1516_2025`, shaped after the official IEEE 1516.1-2025 Java API.
The downloaded API source currently contains 206 `RTIambassador` method
declarations (overloads included) and 62 `FederateAmbassador` callback
declarations. Those counts are a planning baseline, not a claim of completed
Python coverage.

Coverage is counted only when a service has all of the following:

1. a public Python signature and public value types in `hla-rti-api`;
2. a real native pybind boundary reaching the C++ RTI;
3. a Java-provider boundary that invokes the matching Java API and converts
   its result or callback into the same public Python types. For Umbra's JNI
   Java provider, that Java API call must then reach the matching C++ service;
4. provider-neutral contract checks plus a real native integration test; and
5. a JVM fixture/integration test whenever the Java fixture implements that
   service; and a separate JNI integration test before counting a service as
   available through Umbra's Java façade.

An unavailable operation must be absent from the shared API, or raise the
standard typed exception from an actual provider call. It is never counted
merely because a Python stub or mock can accept its arguments.

The promotion order is native-first: a Python/JPype scenario is a harvested
stress or adapter case until its behavior has been translated into an exact
official C++ API Catch2 test. Only after that C++ lane is green do we use the
same scenario to verify the Java surface and the Python bridge. Adapter tests
therefore strengthen the evidence matrix but never replace the authoritative
C++ RTI test.

## External Java API evidence

Umbra's repository fixture remains a fast compatibility harness; it is not
the authority for Java API compatibility. On 2026-08-23, the JNI façade was
compiled and smoke-tested against an independently obtained IEEE
1516.1-2025 Java API JAR, then exercised through C++ → JNI → Java → JPype →
the public Python API. The focused JNI integration module now runs 442 tests
against the exact external artifact (all passed, including the standard named
and no-argument `RtiFactoryFactory` discovery checks and the
structural/runtime gates). The exception-surface check loads every C++ exception whose exact
standard Java class is present in the external API; the independently supplied
API remains authoritative for its vocabulary and intentionally omits a few
legacy advisory exception names. The generic `JavaRtiFactory.from_jar` path
also launches a fresh JVM with the bridge treated as a vendor JAR and the
IEEE API supplied as a dependency, proving onboarding does not depend on the
JNI-specific Python subclass or a preconfigured process. Running Python's
complete JPype test-package discovery against the same artifact produces the
same 442-test result.
The same fresh-process lane copies the external API, bridge, and native
artifacts into a release directory and starts `UmbraJniRtiFactory` from that
directory alone, with no JNI path environment overrides; this verifies the
documented release-directory onboarding rather than only the explicit-path
constructor form. An artifact gate
additionally verifies that the JNI bridge JAR contains
no `hla/rti1516_2025` API classes and exposes only its expected
`NativeRtiFactory` and `NativeAuthorizerFactory` ServiceLoader entries, leaving
the independently supplied IEEE JAR authoritative. An external-interface
arity audit also covers every standard `EncoderFactory.createHLA*` overload so
a new creator cannot fall through the JNI handler silently. The federation lifecycle vector also verifies duplicate federation,
missing federation, duplicate federate name, invalid FOM, and
destroy-while-joined exception mappings. It also rejects an inconsistent
logical-time FOM before the C++ federation registry reserves its name. The
membership vector also exercises
the iterable `String[]` FOM-create overload and all six standard `ResignAction`
values, verifying that each resignation removes the member and leaves the
connection usable for a subsequent join. It also proves disconnecting while
joined preserves the typed `FederateIsExecutionMember` precondition. It also creates two simultaneous
federation executions, verifies both typed records through
`listFederationExecutions`, and supplies a compatible extension FOM through
the join-time `String[]` overload before resolving its extension-defined
object class. The join-time composed-FDD vector additionally proves that
advisory-switch defaults contributed by that additional FOM are seeded only
for the new Java member; the existing C++ member retains its prior switch
values. A companion join-time NRG vector proves that an additional module
containing `nonRegulatedGrant=true` cannot change the creation-time,
federation-wide NRG switch or release a constrained TAR already pending on
another member. A companion explicit-FOM vector proves that
`automaticResignAction="NoAction"` is seeded into the joined member rather
than being confused with an omitted-switch default. The static `Advisories Use
Known Class` companion also proves that a join-time module explicitly disabling
that switch cannot override the creation-time federation policy. The same run
now exercises the
federation-preparation rollback vector: an invalid default-time create and an
invalid join-time extension both leave the valid federation name and base FOM
usable for the subsequent standard create/join operations.
It also exercises the official `getHLAversion()` Java method, the standard send-report-file switch,
and direct standard Java two-argument scalar-`String` and `String[]`
federation-create, unnamed-join, and
three-argument standard-MIM-create overloads against the C++ default
logical-time factory,
integer/floating malformed logical-time decoding, empty/short/trailing and
structurally invalid standard handle-factory payloads, a three-federate regional
overlap/disjoint fanout, and `MessageRetractionHandleFactory.decode` through
C++ → JNI → Java → JPype.
The logical-time mismatch vector also obtains integer and float carriers from
separate C++-backed providers and preserves typed `InvalidLogicalTime` and
`InvalidLookahead` rejection before mismatched bytes reach a time service. It
now exercises the public Python `LogicalTime` and `LogicalTimeInterval`
carriers as well as the raw Java carriers, so implementation identity cannot
be lost at the JPype adapter.
The lifecycle precondition vector invokes the exact standard Java scalar,
`String[]`, and create-with-MIM federation-management overloads before
connection, plus destroy, join, and resign, and preserves C++ `NotConnected`
through the raw Java and JPype boundaries.
The transport-loss vector uses only an internal embedded-transport fault source;
the observable path remains the standard Java `connectionLost` callback. It
proves the C++ membership cleanup removes the lost federate from the surviving
member report and that subsequent services preserve `NotConnected` through
JPype. It also proves the C++ `ConnectionLost` service-report record, including
the fault-description argument, is durable before callback eviction.
The companion MOM vector subscribes to the exact standard
`HLAreportFederateLost` interaction and decodes its federate handle, federate
name, last-known logical time, and fault description through the Java
`EncoderFactory`, proving that RTI-originated MOM traffic crosses JNI and
JPype without a Python-side wire model.
The external IEEE-JAR transport/DDM companion additionally subscribes two
survivors to `HLAreportFederateLost` through matching and disjoint
`HLAfederate` regions, then injects the C++ transport fault. Only the matching
member receives the ordinary standard Java interaction callback; the disjoint
member remains silent, while the lost member receives `connectionLost`. The
callback carries the report class, empty tag, reliable transport, and no
source-region designator, preserving the C++ MOM/DDM semantics through JPype.
The companion timestamped-cutoff vector queues an interaction at logical time
6, advances the regulating publisher to 6, and injects transport loss. The
survivor receives the timestamped interaction with the original producer,
`TIMESTAMP` sent/received order, parameter bytes, and valid retraction handle
before its pending grant; the queued callback is consumed once and the lost
member receives the standard `connectionLost` callback.
The strict-before companion repeats the same route with a time-5 interaction
and a time-6 survivor grant, proving that messages strictly before the
last-known cutoff are delivered in the same interaction-before-grant order.
The post-loss-request companion goes further: with no survivor advance pending
at disconnect, a later standard TAR establishes the boundary and releases the
queued TSO before its grant.
The combined federate-lost MOM companion additionally decodes the same cutoff
timestamp from `HLAreportFederateLost` while delivering the application TSO and
grant through their standard Java callbacks.
The HLA_IMMEDIATE companion proves the federate-lost MOM interaction is
delivered synchronously during the fault call, without callback eviction, and
decodes the source federate and fault description through the Java encoder.
The matching timestamped-deletion cutoff vector applies the same boundary to
`RemoveObjectInstance`: a time-6 deletion accepted by C++ is delivered once
to the survivor before its grant with the original producer, `TIMESTAMP` order,
tag, and valid retraction handle, then the removed object name becomes unknown
through the standard Java lookup service.
The matching timestamped-update cutoff vector applies the same boundary to
`ReflectAttributeValues`: C++ delivers one reliable update before the
survivor's grant with the original producer, tag, payload, `TIMESTAMP` order,
and valid retraction handle, while the object remains resolvable after the
source transport loss.
The multi-survivor cutoff companion then queues the same C++ timestamped
attribute update for two constrained Java members, releases the first at its
pending time-6 boundary, and releases the second only when it later requests
the same boundary. Both callbacks preserve the producer, reliable transport,
timestamp/order metadata, and one shared C++ retraction handle.
The directed-cutoff companion now covers the non-cleanup path as well: a
timestamped directed interaction queued before transport loss is delivered at
the lost federate's last-known time, before the surviving member's grant,
preserving target, producer, reliable transport, timestamp/order metadata, and
the C++ retraction handle. The regional-selector cutoff companion extends that
boundary through DDM: after
the C++ transport fault, the surviving Java member moves its committed region
disjoint before requesting the cutoff grant, so the stale regional reflection
is suppressed while the separate receive-order automatic deletion still
arrives at the following boundary and removes the object name. The matching
directed-selector vector changes a universal directed-interaction subscription
to ownership-only at the callback boundary; C++ suppresses the queued
timestamped directed callback while the independent `DELETE_OBJECTS` cleanup
still removes the target at its receive-order cutoff. The explicit-unsubscribe
companion covers the same queued cutoff after the standard
`unsubscribeObjectClassDirectedInteractions` call, proving that removing the
directed declaration suppresses the stale callback without suppressing the
independent automatic cleanup.
The exception-report vectors then enable the C++ Exception Reporting Switch,
provoke typed `InteractionClassNotPublished` failures from the standard Java
`sendInteraction`, `sendInteractionWithTime`, `sendDirectedInteraction`, and
`sendInteractionWithRegions` calls, and independently
decode the RTI-originated `HLAreportException` service and exception strings
through the same Java encoder path.
The declaration-management companion also enables both C++-owned reporting
switches, invokes the standard Java `subscribeInteractionClass` conflict for
`HLAreportServiceInvocation`, preserves the typed
`FederateServiceInvocationsAreBeingReportedViaMOM`, and decodes its separate
`HLAreportException` callback through the Java encoder factory.
The complementary switch vector subscribes the report stream first, then
preserves `ReportServiceInvocationsAreSubscribed` from the standard Java
`setServiceReportingSwitch(true)` call and decodes its separate exception
report through the same Java encoder path.
An HLA_IMMEDIATE companion delivers the same standard callback synchronously,
then preserves survivor membership cleanup and the lost proxy's typed
`NotConnected` state without an explicit eviction step.
The automatic-resign vector sets the standard Java `DELETE_OBJECTS` directive,
then drives the C++ transport-loss control and verifies the survivor receives
the exact `removeObjectInstance` callback while the departed object's name is
no longer resolvable.
Its `UNCONDITIONALLY_DIVEST_ATTRIBUTES` companion retains the object and
delivers the typed ownership-assumption callback, including the delete-privilege
attribute, to the surviving Java member.
Its configured `NO_ACTION` companion covers the bounded forced-loss policy:
the lost Java member receives only `connectionLost`, while the survivor retains
the object, receives one typed ownership-assumption callback for both formerly
owned attributes, and receives no automatic remove-object callback.
Its `DELETE_OBJECTS_THEN_DIVEST` companion drives the same standard directive
through C++ transport loss with one retained transferred object and one
lost-member-owned object, proving the survivor receives exactly one typed
`removeObjectInstance` and one ownership-assumption callback while the retained
object remains name-resolvable and the deleted object does not.
The pending-acquisition companion starts a standard Java ownership request,
then applies `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` before injecting C++
transport loss. The owner receives no stale release callback, while a surviving
candidate can still receive a later unconditional-divestiture assumption,
proving that C++ registry cleanup cancels departed-requester work without
poisoning the remaining ownership state.
The negotiated-transfer companion queues both the regular release request and
the selected divestiture-confirmation callback before the same transport fault;
both are suppressed after C++ cancellation and the original owner remains the
typed owner, proving that negotiated ownership state cannot leak through Java
or JPype after requester loss.
The combined `CANCEL_THEN_DELETE_THEN_DIVEST` companion queues a regular
acquisition, deletes a lost-member-owned object, and re-offers a retained
attribute in one C++ transport-loss transition. JPype observes the standard
`connectionLost`, `removeObjectInstance`, and ownership-assumption callbacks,
with the stale owner-release callback suppressed and object-name lookup
preserving the delete/divest boundary.
The final-member companion sets `NO_ACTION`, injects transport loss as the
last joined member, and proves the standard Java proxy can disconnect, reconnect,
rejoin, reserve the deleted object's former name, and register a replacement.
This mirrors the C++ final-member directive-two override without adding a
Python-side cleanup policy.
The service-report ordering companion enables the C++ report-file path through
the standard `RtiConfiguration`, then verifies that a queued
`DiscoverObjectInstance` report is appended before the Java callback executes.
The same vector decodes its type-37 object handle, type-36 class handle,
type-53 instance name, and type-15 producing-federate arguments.
It also proves callback-time unsubscription suppresses both the stale
discovery callback and its service report, preserving C++ eligibility
re-evaluation through JNI and JPype.
The matching removal-order companion verifies that a receive-order
`RemoveObjectInstance` report, including its copied tag, is durable before the
standard Java callback executes. Its C++-owned object and federate handles and
byte tag arrive unchanged at the Python callback boundary; its type-38
receive-order slot and type-34 timestamp/order/retraction placeholders are
decoded as well.
The timestamped removal-order companion extends this to both recipient paths:
the unconstrained Java member receives `RECEIVE` metadata immediately, while a
constrained member receives `TIMESTAMP` metadata before its matching grant.
Both report files are durable before callback code, and both callbacks preserve
the original timestamp, tag, producer, and retraction handle from C++.
The timestamped file records are now decoded directly for both boundaries:
the immediate recipient retains `RECEIVE` order, while the constrained grant
retains `TIMESTAMP` receive order, type-31 time 6, and the type-33 retraction.
The retraction companion now decodes the accepted file-side `Retract` record
as the type-33 `MessageRetractionDesignator` with its successful-void return
before the standard Java `requestRetraction` callback.
The service-report-file lifetime companion uses one shared configured directory
for two joined Java members, proves their initial JSON files are distinct and
that a successful owner-side switch record leaves the peer file unchanged, then
resigns and rejoins to verify a fresh immutable lifetime file is created.
The declaration/relevance advisory companion uses the same configured private
sink and decodes `StartRegistrationForObjectClass`,
`StopRegistrationForObjectClass`, `TurnInteractionsOn`, and
`TurnInteractionsOff` records with their standard handle argument forms before
each corresponding Java callback is evoked.
The save-status response companion starts a real standard federation save,
invokes `QueryFederationSaveStatus`, and verifies that both the query and
`FederationSaveStatusResponse` service records are durable before the typed
Java `FederateHandleSaveStatusPair` callback reaches Python. It then aborts the
save through the same standard Java surface.
The save-completion companion covers both outcomes in the private file:
successful `FederateSaveComplete`/`FederationSaved` records and the false
`FederateSaveComplete` plus `FEDERATE_REPORTED_FAILURE_DURING_SAVE` pair before
the typed failure callback is evoked. The timestamped save companion also
decodes the optional type-31 logical-time overload through the same route.
The negative restore companion requests a missing save label and verifies that
the standard `RequestFederationRestore` and negative
`ConfirmFederationRestorationRequest` records are durable before
`requestFederationRestoreFailed` reaches Python, while the C++ restore state
remains correctly unrequested.
The positive restore companion creates a real snapshot, requests its restore,
and proves the positive confirmation is durable before
`requestFederationRestoreSucceeded`, followed by typed restore-begun,
initiation, and completion callbacks through the same Java/JPype route.
The time-advance report companion proves an accepted standard
`timeAdvanceRequest` is durable at admission, leaves C++ logical time at its
initial value until the grant, and exposes the report before the typed Java
`timeAdvanceGrant` callback updates the Python view.
Its paired `timeAdvanceRequestAvailable` companion preserves the distinct
standard service name and the same C++ admission/grant boundary through Java
and JPype.
The paired next-message companions preserve the supplied boundary separately
for `nextMessageRequest` and `nextMessageRequestAvailable`: each C++ service
record is durable at admission, while the queued timestamped interaction and
typed Java `timeAdvanceGrant` callback remain deferred until the scheduler
frontier reaches 7.
The provider-callback companion carries a real object-class request through
the standard Java surface and verifies that C++ `ProvideAttributeValueUpdate`
reporting is durable before the copied Python object handle, attribute set, and
tag arrive at callback entry.
The Auto Provide companion exercises the federation-wide switch through the
same standard surface, then verifies C++ emits the empty-tag
`ProvideAttributeValueUpdate` report before the late-subscription provider
callback is delivered.
The Flush Queue companion preserves the supplied boundary separately from the
effective grant and optimistic time, proving the C++ report is durable before
the typed Java `flushQueueGrant` callback delivers both logical-time values.
The synchronization companion registers a global point before a late Java
member joins, then verifies the recipient's C++ `AnnounceSynchronizationPoint`
record is present before the copied label and tag reach the JPype callback.
Its completion companion uses an explicit two-member synchronization set and
proves the C++ `FederationSynchronized` report precedes the typed failed-member
set delivered to the standard Java callback.
The registration companion verifies the global Java overload's C++ register
and confirmation records before `synchronizationPointRegistrationSucceeded`,
then drains the separately queued announcement callback with its copied tag.
The ownership companion queries one owned and one unowned attribute through
the standard Java API and verifies the C++ `QueryAttributeOwnership` record is
durable before typed owner and not-owned callbacks reach JPype.
The divestiture companion transfers an owned attribute through the standard
Java `unconditionalAttributeOwnershipDivestiture` service and verifies its C++
report precedes the candidate's typed assumption callback and copied tag.
The late-join companion extends this ownership proof across membership
boundaries: a candidate that joins after the owner resigns first discovers the
retained object without a false assumption callback, then receives the complete
assumption set after publishing.
The If Available companion requests an unowned attribute through the standard
Java overload and verifies its C++ report precedes the typed acquisition
notification with the requested set and tag intact.
The regular-acquisition companion requests an owner-held attribute and proves
the C++ `AttributeOwnershipAcquisition` report precedes the owner's typed
release callback with the copied request tag.
The release-denied companion then exercises the owner's standard denial
service and verifies its C++ report precedes the requester's typed unavailable
callback with the denied set and tag intact.
The confirmation companion completes a negotiated transfer through the
standard Java `confirmDivestiture` service and verifies the C++ report precedes
the requester's acquisition notification and ownership-state transition.
The negotiated-divestiture companion verifies the owner's C++ report precedes
its typed `requestDivestitureConfirmation` callback while retaining the
original acquisition tag.
The cancellation companion verifies `CancelNegotiatedAttributeOwnershipDivestiture`
restores the pending release callback only after its C++ report, without
emitting a stale divestiture-confirmation callback.
The acquisition-cancellation companion verifies the standard requester
callback follows the C++ `CancelAttributeOwnershipAcquisition` report while
preserving the object and attribute set.
The interaction-transport companion verifies the standard
`requestInteractionTransportationTypeChange` report precedes its typed
confirmation callback with both handles intact.
The normal Java `resignFederationExecution` path now exercises that same
directive without a transport fault, proving the standard resignation service
applies C++ deletion/divestiture policy and preserves the survivor's callbacks.
The forced-resignation vector uses a separate internal RTI membership-control
source and observes the exact standard Java `federateResigned` callback. It
proves only the affected member receives the reason, the survivor sees removal,
duplicate control is rejected, and the connected proxy can rejoin afterward.
Its HLA_IMMEDIATE companion proves synchronous callback delivery, survivor
membership cleanup, and rejoin through the same standard Java proxy.
The federate-lookup companion also exercises disconnected and unjoined
preconditions, cross-execution handle rejection, missing joined names, and the
standard guarantee that a departed member's returned `FederateHandle` still
resolves to its immutable name through the surviving Java member.
The same external suite has also passed against a freshly rebuilt clean
artifact after the build script cleared generated classes, rather than only
against the cached bridge output.
The same external run now exercises save/restore precondition errors across
uninitiated, overlapping, and active operations—including the opposite status
query, scalar and timestamped federation-save requests, synchronization registration/achievement, declaration publication, named object registration, object update, object deletion, direct/regional/directed/timestamped interaction sends, attribute-value requests, ownership queries, order changes, transportation changes, region lifecycle, regional declaration, and region association/unassociation during each active operation—preserving the standard
`SaveNotInitiated`, `FederateHasNotBegunSave`, `SaveNotInProgress`,
`RestoreNotRequested`, `RestoreNotInProgress`, `SaveInProgress`, and
`RestoreInProgress` types through JNI. It also exercises two-member federation membership reporting,
save-status records, save initiation/completion barriers, requester-scoped
restore acceptance, typed per-member restore initiation, and restore completion
only after both members report complete. It also exercises requester-only
synchronization registration success, announcements to both members, a
two-member synchronization completion barrier, and a failed achievement whose
typed failed-member handle set reaches both callbacks. It also exercises an official
three-federate regional-object pair-list route, proving overlap discovery and
reflection while filtering a disjoint subscriber in C++.
The mixed callback-model companion then runs the same two-member save/restore
barrier with one HLA_IMMEDIATE and one HLA_EVOKED Java ambassador, proving that
C++ notification ordering is preserved while each JPype callback route keeps
its own delivery mode.
The save/restore MOM companion also checks the C++ report-file records for
status queries, request/confirm, initiation, begun/complete, and restore
begin/initiation services while the corresponding standard Java callbacks are
delivered through JPype.
The joined-federate MOM state companion additionally subscribes to the
RTI-owned `HLAfederateState` object, decodes the standard Java
`HLAinteger32BE` state values, and proves the C++ save/restore callbacks
reflect states 3/1 and 5/1 to the observer through JPype. The regional
joined-federate MOM companion uses the exact Java
`AttributeSetRegionSetPairList` subscription for `HLAfederateName`, first
keeping a disjoint range and then mutating the committed range onto the
C++-owned immutable `HLAfederate` point. It proves matching discovery,
reflection, requested reflection, and removal while the disjoint observer
remains unaware, with the RTI-originated invalid producer handle and empty tag
preserved at the Python callback boundary.
The conditional-MOM companion subscribes to the complete joined-federate
attribute set and verifies that advisory switches, timing state, lookahead,
logical time, and time-manager transitions are reflected only through their
current C++ state. It drives all five standard time-advance/queue services and
the standard `HLAsetSwitches` interaction through both evoked and immediate
Java callback models, decoding typed integer/boolean values with the external
Java `EncoderFactory`. The companion `HLAsetTiming` vector sends the official
`HLAfederate`/`HLAreportPeriod` parameters through the standard Java
`sendInteraction` overload, rejects a negative period without mutation, and
proves one periodic logical-time/lookahead reflection plus clean disablement
through both `HLA_EVOKED` and `HLA_IMMEDIATE` callback models. The
`HLAGALT`/`HLALITS` vector then derives defined federation bounds from the
regulator's C++ time state, observes the same values through direct and
periodic MOM reflection, and verifies both attributes return the standard
empty logical-time encoding after regulation is disabled.
The companion `HLATSOlength` vector queues a timestamped interaction for a
constrained federate, observes the standard HLAcount through direct and
periodic MOM reflection, and verifies the count returns to zero after the
grant delivers the queued interaction. Both MOM vectors execute through the
external IEEE Java API, JNI, and JPype under `HLA_EVOKED` and `HLA_IMMEDIATE`.
The `HLAobjectInstancesThatCanBeDeleted` vector then follows the standard
ownership ledger from zero, to one after registering an `Employee.Server`,
through periodic reflection, and back to zero after deletion.
The `HLAupdatesSent` vector counts accepted untimed and timestamped
`updateAttributeValues` service invocations at the C++ admission boundary,
then observes the same `HLAinteger32BE` count through direct and periodic MOM
reflection under both callback models.
The `HLAROlength` vector queues a receive-order interaction on an evoked
federate, reads the native queue ledger through direct and periodic MOM
reflection, then drains the callback and verifies the count returns to zero.
The `HLAobjectInstancesUpdated` vector preserves the distinct accepted object
handle set: repeated updates to one object leave the count at one, a second
object raises it to two, and periodic reflection carries both that value and
`HLAupdatesSent=3` through the standard Java route. The same vector checks
`HLAobjectInstancesRegistered` at zero, one, and two across the corresponding
registration boundaries, and carries `HLAobjectInstancesDeleted` through
periodic reflection at zero before the two accepted deletions and through
direct requests at one and two afterward. The same vector verifies
`HLAobjectInstancesRemoved` at zero before the two committed Remove Object
Instance callbacks and at one and two afterward. It also verifies
`HLAobjectInstancesDiscovered` on the receiving federate’s MOM object at zero,
one, and two as the two application-object discovery callbacks commit.
The native Catch2 MOM companion additionally performs Local Delete Object
Instance followed by a repeated eligible subscription and proves the same
object is rediscovered, raising that counter to three; the Java route remains
the direct 0/1/2 bridge evidence.
It also keeps `HLAobjectInstancesReflected` distinct by object: the first
application reflection raises it to one, a repeated update remains one, and a
second object raises it to two.
The same application-reflection vector observes `HLAreflectionsReceived`
at 0/1/2/3 for the three accepted callback invocations, while the distinct
object counter remains 0/1/1/2. RTI-owned MOM reflection uses its separate
path and is not folded into this application callback ledger.
The interaction-send MOM bridge vector counts accepted ordinary and directed
`sendInteraction` invocations at the C++ service boundary. It observes
`HLAinteractionsSent` at 0/1/2 and the directed subset
`HLAdirectedInteractionsSent` at 0/0/1 through the standard Java
`HLAinteger32BE` reflection path under both callback models. The native Catch2
companion extends this evidence across ordinary, directed, timestamped,
regional, and timestamped-regional sends, proving total 0/1/2/3/4/5/6,
directed 0/0/1/1/2/2, and periodic 6/2; the bridge remains intentionally
limited to its direct Java 0/1/2 vector.
The same two-member bridge vector observes the receiver's
`HLAinteractionsReceived` at 0/1/2 and
`HLAdirectedInteractionsReceived` at 0/0/1, with the C++ ledger advanced
immediately before each accepted Java callback. The native Catch2 companion
extends that receiver evidence across ordinary, directed, timestamped,
regional, and timestamped-regional delivery, proving total 0/1/2/3/4/5/6 and
directed 0/0/1/1/2/2, plus periodic 6/2; the bridge remains intentionally
limited to its direct Java vector.
The FOM-snapshot companion uses the exact Java `String[]` Join overload with an
additional FOM module, observes the RTI-owned `HLAFOMmoduleDesignatorList`
through standard MOM reflection, decodes the `HLAmoduleDesignatorList` with the
external Java `EncoderFactory`, and checks the corresponding C++ service report
so the federate-scoped list contains only modules supplied at Join.
The federation-MOM companion now discovers the single RTI-owned
`HLAobjectRoot.HLAmanager.HLAfederation` object and decodes its static
`HLAfederationName`, `HLARTIversion`, `HLAMIMdesignator`,
`HLAtimeImplementationName`, `HLAadvisoriesUseKnownClass`,
`HLAdelaySubscriptionEvaluation`, `HLAnonRegulatedGrant`, and
`HLAallowRelaxedDDM` attributes through C++ → JNI → the external IEEE Java
API → JPype.  `HLAfederatesInFederation`, `HLAFOMmoduleDesignatorList`,
`HLAcurrentFDD`, and `HLAautoProvide` now have bounded lifecycle-triggered
update plans. The save-name/time companion now subscribes to
`HLAlastSaveName`, `HLAlastSaveTime`, `HLAnextSaveName`, and `HLAnextSaveTime`,
decodes the standard Java HLAunicodeString and HLAlogicalTime payloads, and
proves pending timed-save, Initiate, and Federation Saved boundaries through
the C++ → JNI → Java → JPype route.
The Auto Provide companion now closes the first of those conditional gaps:
the existing standard Java `HLAsetSwitches` vector subscribes only to
`HLAautoProvide`, discovers the federation object from its effective MIM
attribute metadata, and observes the C++ switch transition at 0→1→0→1 through
the normal JNI/JPype reflection callback path.
The federation-membership companion now subscribes only to
`HLAfederatesInFederation`, decodes the standard nested
`HLAfederateReferenceList` through the external Java encoder, and observes the
live C++ membership vector at 1→2→1 across Join and Resign callbacks.
The federation-FOM companion now subscribes only to
`HLAFOMmoduleDesignatorList`, decodes the standard `HLAmoduleDesignatorList`
with the external Java encoder, and observes the execution module vector
change from the Create-supplied base module to the base-plus-additional list at
an exact Java `String[]` Join boundary.
The current-FDD companion mirrors the native three-member vector: it subscribes
to `HLAcurrentFDD`, decodes the composed Restaurant FOM as an
`HLAunicodeString`, observes the reliable refresh after a compatible
`UmbraReferenceFixtureClass` additional-FOM Join, and confirms a direct request
returns the same refreshed XML through the external Java encoder.
The `HLAmodifyAttributeState` companion drives the standard MOM adjustment
interaction through the Java surface after publishing it, transfers an
application attribute to a second federate without ownership callbacks, verifies
the unowned transition and immediate re-transfer, and preserves
`RTIinternalError` when the same control targets an RTI-owned MOM object.
The `HLAsetSwitches` companion drives the standard predefined parameter subset,
proves sender-only state changes and peer isolation, preserves empty/invalid
typed failures and the report-subscription interlock, and accepts both an
extension parameter and a subclass carrying inherited standard switches.
The companion report-subscription vector now exercises both ordinary and
regional passive declarations for `HLAreportServiceInvocation`: each is
rejected while service reporting is enabled, accepted while disabled, and
blocks re-enabling until the exact declaration is removed through the standard
Java/JPype route.
The local-delete companion invokes the standard `localDeleteObjectInstance`
overload from Java, proves an unknown handle leaves the private report file
untouched, and decodes the accepted type-37 object-instance argument, null
return, success indicator, and local-forget state transition.
The declaration-report companion invokes the standard publish and unpublish
object-class-attributes overloads, verifies invalid handles do not append
records, and decodes type-36 object-class and type-1 attribute-set arguments,
including the Java-standard optional-set name on unpublication.
The object-attribute subscription companion covers passive and active
subscriptions with explicit and default update rates, invalid-handle no-op
behavior, subset unsubscription, and the whole-class Java overload; it decodes
the corresponding Boolean, String, Null, and optional attribute-set records.
The directed-declaration companion exercises publish and unpublish directed
interaction sets, invalid object/interaction handles, explicit empty sets, and
the whole-class optional-set overload, preserving type-28 and type-34 report
forms through the standard Java/JPype route.
The directed-subscription companion covers ordinary and universal selectors,
invalid handles, explicit empty sets, subset removal, and whole-class removal,
preserving the standard Boolean universal flag and type-28/type-34 optional-set
encodings.
The ordinary interaction-declaration companion covers publish, passive
subscribe, unpublish, and unsubscribe through the standard Java overloads,
preserving type-27 interaction handles and the inverse passive Boolean while
ensuring invalid declarations append no successful report.
The region-service companion drives `SetRangeBounds` and `DeleteRegion` through
the Java provider, suppresses setup reports while switches are disabled,
decodes type-42/type-10/type-35 arguments, and confirms invalid bounds and
repeated deletion append no successful record.
The timestamped regional request/response companion also answers a
`provideAttributeValueUpdate` callback with the standard Java timed update
overload, preserving constrained delivery, `TIMESTAMP` metadata, source
regions, and the consumed retraction handle.
After the peer resigns, the same route reuses its still-structurally-valid
standard `FederateHandle` in a later explicit set and preserves the asynchronous
`SYNCHRONIZATION_SET_MEMBER_NOT_JOINED` failure for the former member.
The timed-save vector also proves that admission remains pending after the
regulating member reaches the boundary, then admits both members only after
the constrained member reaches its qualifying grant, with initiation ordered
before that grant callback.
The timed-save replacement companion now mirrors the C++ pending-request rule:
an earlier timestamped save is replaced before admission, and a timestamped
interaction at the final boundary reaches the constrained Java member before
save initiation and its qualifying grant.
The untimed-save boundary companion mirrors the three-member C++ vector:
constrained members receive `initiateFederateSave` only as each reaches its
time-5 grant, the non-constrained regulator is admitted afterward, and each
member reports `federationSaved` through the standard Java callback barrier.
The three-member timed-save companion mirrors the corresponding C++ ordering:
the regulating member receives its inclusive time-5 grant first, each
constrained member receives save initiation before its own grant, and the
owner's timestamped save initiation follows both constrained boundaries.
The Available/next-message companion combines the standard
`timeAdvanceRequestAvailable` and `nextMessageRequestAvailable` overloads:
both strict time-6 grants initiate the timestamped save before their grants,
while the owner remains pending until both constrained members qualify.
The per-member TSO companion adds a third constrained participant: the clean
member initiates at its time-5 boundary, while the delayed member receives its
queued timestamped interaction before its own save initiation and grant; the
regulator then receives grant-before-initiation ordering.
The in-transit TSO vector additionally proves the timestamped interaction is
delivered before save initiation and that initiation still precedes the peer's
grant callback.
The re-entrant TSO vector also requests a save from inside the JPype receive
callback, confirms the request is accepted without initiation during the
callback, and verifies initiation follows callback return.
Its HLA_IMMEDIATE companion proves the same re-entrant request through direct
Java callback dispatch, preserving the receive-before-initiate-before-grant
ordering and completing the save barrier without an evoked drain.
The restore-state vector also saves logical time 3/lookahead 2, mutates both
to 5, and verifies the external Java route rewinds both values from the C++
saved image.
The object-management restore companion saves a completed baseline before
publishing/subscribing and registering an object, then restores that baseline
through both standard Java ambassadors and proves the post-save object is no
longer known while the joined Java members remain usable.
The regional-object restore companion saves an overlapping source/subscriber
association, moves the source range to a disjoint boundary after save, and
proves restore rewinds both the C++ range and the association before a later
standard Java update is delivered again with its source-region metadata.
The regional-interaction restore companion applies the same boundary to a
regional interaction subscription, proving the restored source range admits a
later Java `sendInteractionWithRegions` callback with its region designator.
The regional retraction fan-out companion also sends one timestamped regional
interaction to immediate and constrained recipients, retracts it after only
the immediate callback, and proves the constrained queued passel is suppressed
while the delivered recipient receives the standard `requestRetraction`.
The suppressed-regional companion moves a previously eligible receiver to a
disjoint range before its callback, proves the interaction is dropped, and
confirms the later retraction produces no stale `requestRetraction` callback.
The no-overlap regional-send companion separately proves a valid sender-side
retraction handle exists even with zero eligible recipients, then preserves
the standard terminal-expiry error after the publisher crosses its lookahead
boundary without manufacturing a receiver callback.
The mixed regional TSO matrix then fans the same explicit-source interaction
to FQR, TARA, and NMRA recipients, preserving source-region metadata and
callback-before-grant ordering for all three standard advance families.
The timed-restore vector saves at logical time 6 while a default-region
timestamped interaction at time 8 is queued, consumes the post-save handle,
restores it, and proves FQR delivery retains the empty conveyed region set,
original retraction handle, and request-retraction callback.
The directed timed-restore vector preserves target object identity and
reliable transport through the standard directed callback, while the timed
attribute-update vector preserves the object/attribute map and empty default
region projection. The live timestamped-deletion vector restores the removal,
reconstitutes the object on retraction, and exposes `ObjectInstanceNotKnown`
only while the restored removal is active.
The explicit-source regional attribute timed-restore vector additionally keeps
the publisher's source-region designator, timestamp, producer, and live
retraction handle across save/restore before the constrained receiver flushes.
The multi-recipient timed-restore vector independently flushes two constrained
receivers and proves one restored queued attribute update retains separate
recipient delivery state and sends both standard `requestRetraction` callbacks.
The terminal regional-attribute vector keeps a pre-save terminal retraction
classification across restore while rejecting a post-save handle as
`InvalidMessageRetractionHandle`.
The terminal timestamped-deletion vector additionally proves the deleted
object name can be reused before restore, while restore retains the original
tombstone and invalidates the post-save deletion handle.
The default-region attribute tombstone companion preserves the same terminal
classification when the publisher uses the standard empty source-region
projection and the receiver subscribes through a regional filter.
The directed-interaction tombstone companion preserves the same terminal
classification for a targeted timestamped send and rejects the post-save
directed-message handle after restore.
The regional-interaction tombstone companion carries explicit source-region
metadata through the same terminal classification and restore boundary.
The pending-available-mode matrix now exercises TARA, NMR, and NMRA under both
`HLA_EVOKED` and `HLA_IMMEDIATE`, proving each restored request remains pending
until its reconstructed grant and that the same service remains usable after
restore.
The timed pending-FQR matrix now performs the equivalent restore under both
callback models, preserving the saved Flush Queue Grant boundary and allowing a
fresh post-restore FQR.
The default-region interaction tombstone companion now preserves the empty
source-region projection and terminal retraction classification through the
same save/restore boundary.
The float64 pending-FQR vector now carries the saved request through the
standard `HLAfloat64Time`/`HLAfloat64Interval` Java carriers and restores its
grant boundary before admitting a fresh FQR.
Its immediate-callback companion restores the same timed FQR boundary through
the standard Java callback path and admits a fresh post-restore FQR.
The float64 pending-available-request vector restores a saved strict time
advance request under both `HLA_EVOKED` and `HLA_IMMEDIATE`, preserving the
float64 grant boundary and allowing a fresh request after restore.
The two-member float64 timed-save boundary vector also proves C++ waits for
both the regulating and constrained Java members, then carries the typed
timestamped save initiation through JNI before either grant.
The float64 timestamped-interaction vector additionally verifies the Java
callback carries the C++ time type, timestamp/order metadata, producer, and
opaque retraction handle through JPype.
The companion float64 timestamped-attribute vector verifies the same carrier
and retraction metadata on `reflectAttributeValues` callbacks.
The float64 timestamped-deletion vector completes the object-management path
with the same `HLAfloat64Time` and retraction carrier checks on removal.
The float64 directed-interaction vector completes the specialized directed
callback shape with the same timestamp/order/retraction checks.
The float64 time-factory vector also advances by one representable ULP through
the standard `timeAdvanceRequestAvailable` overload, proving the scheduler
preserves the smallest distinguishable target through Java, JNI, and JPype.
The companion float64 restore vector performs the same rewind through the
standard Java `HLAfloat64Time` and `HLAfloat64Interval` carriers.
The terminal-retraction restore vector also preserves a saved terminal
retraction classification while rejecting a post-save handle as invalid.
The directed-retraction uniqueness companion additionally creates a live
directed message after the save image, restores, and proves its stale handle
cannot alias a fresh post-restore designator; only the fresh handle retracts.
The live-retraction restore vector additionally saves two queued timestamped
interactions in a two-member federation, preserves their timestamp order and
distinct retraction handles through restore, flushes both through the external
Java time/order carriers, and routes each restored handle's `requestRetraction`
callback back to the receiving Python federate.
The saved-live-regional vector applies the same ledger to an explicit-source
regional timestamped interaction: it retracts after save, restores the source
region and interaction metadata, flushes through the standard Java FQR path,
and preserves the restored handle for a later `requestRetraction` callback.
The pending-time restore vector also saves a constrained member while its
time-advance request is still pending, completes that save re-entrantly from
the standard Java callback, rejects time services during restore with the
typed `RestoreInProgress`/`InTimeAdvancingState` states, and proves that the
restored C++ request produces the saved time-5 grant only after
`federationRestored`.
Its HLA_IMMEDIATE companion exercises the same pending-TAR snapshot on the
direct Java callback stack, preserving save-initiate/save-complete/grant
ordering, reconstructing the saved time-5 grant synchronously after restore,
and admitting a fresh time-6 request afterward.
The companion HLA_IMMEDIATE vector saves a pending `flushQueueRequest`,
verifies `save-initiate`/`save-complete`/`flush-grant` callback ordering,
reconstructs the saved time-6 flush grant synchronously after restore, and
accepts a fresh time-7 flush request afterward.
The six-case advance-variant matrix extends this to pending
`timeAdvanceRequestAvailable`, `nextMessageRequest`, and
`nextMessageRequestAvailable` requests under both HLA_EVOKED and
HLA_IMMEDIATE, preserving the saved time-5 grant and post-restore usability
for every standard Java overload.
The GALT/NRG role-transition vector also proves a default-disabled constrained
TAR wakes when a regulator becomes active, while enabled NRG releases a
boundary TAR when its sole regulator disables or resigns; all three transitions
retain the C++ grant value and typed Java callback through JPype.
The same external vector now proves float64 `add`, `subtract`, and `distance`
at representable-value, smallest-positive-subnormal, and initial/final boundaries, integer64 arithmetic
above 2^53, and the standard Java interval `add`/`subtract` mutators at exact
finite boundaries and overflow/underflow edges, including typed
`IllegalTimeArithmetic` mapping. The raw Java carriers also
round-trip time values through `add`/`subtract`, report positive `distance`,
and preserve the exact C++ `IllegalTimeArithmetic` cause inside the standard
Java `UndeclaredThrowableException` wrapper for negative distance. Those operations are
dispatched to the selected C++ time factory; the Java carriers are
standard-interface views and do not reimplement RTI arithmetic. The same run
also proves that invalid Java factory construction maps to the public
`InvalidLogicalTime` and `InvalidLogicalTimeInterval` classes, while
construction and raw-wire decoding of signed zero are
canonicalized to `+0.0` for both standard Java time carriers and their public
Python counterparts.
Direct Java `decodeTime`/`decodeInterval` calls retain the standard
offset-overload distinction between Java-side short-payload
`IllegalArgumentException` and C++-owned negative/non-finite
`CouldNotDecode`; trailing bytes remain legal to the Java offset overload while
the normalized Python factory rejects them before delegation. The logical-time
`HLAlogicalTime` and `HLAlogicalTimeInterval` data elements also reject a
carrier selected by a different factory before opaque bytes can be
reinterpreted: raw Java receives the standard `EncoderException`, while the
Python façade applies the same C++ type-identity check on both constructor and
`setValue` paths and preserves the prior carrier.
The external integer64 decode companion now proves the same distinction for
signed BE values: raw Java accepts the standard offset window and preserves
C++ `CouldNotDecode` for negative payloads, while the Python factory keeps
strict exact-length validation. The external float64 decode companion now
proves that distinction directly for
negative, NaN, and infinity payloads: raw Java preserves C++ `CouldNotDecode`,
short windows remain Java `IllegalArgumentException`, and the Python factory
retains strict exact-length validation while both paths accept the same finite
3.25 value.
The same float vector now drives the raw Java `HLAlogicalTime` and
`HLAlogicalTimeInterval` data elements through C++-selected carriers,
preserving the C++ opaque one-octet alignment, exact bytes, one-element
`ByteWrapper` cursor consumption, and the C++ `CouldNotDecode` cause through
Java's declared method boundary; Python data-element encoders emit the same
wire values. Both integer and float routes now exercise the reverse
`encode(ByteWrapper)` direction through raw Java and the provider-scoped
Python façade, checking non-zero offsets, exact cursor advancement, and an
untouched trailing sentinel so logical-time carriers are covered in both
directions at the JNI boundary.
Provider-owned wide time carriers are also written through the Python value
facade at a non-zero offset with sentinels on both sides; the facade preserves
the provider-reported wire width and adjacent caller bytes instead of
silently reducing the carrier to the reference eight-octet shape.
It also exercises a malformed payload matrix across all twenty-six primitive
encoder classes and all four standard composite forms, including nested
fixed-record, fixed-array, variable-array, and variant children, preserving
typed `DecoderException` results through the external Java API.
The standalone Java 2025 surface smoke repeats the successful carrier pass
without Python: every one of those twenty-six primitive creators and the four
composite creators is encoded and decoded through a non-zero-offset
`ByteWrapper`, with exact cursor advancement and a preserved trailing
sentinel. This keeps the Java-only JNI gate transplantable to another provider
JAR while the Python matrix verifies the additional façade conversion layer.
The latest raw-Java companion repeats the scalar and variable-envelope
malformed cases through the exact `ByteWrapper` cursor overload, proving the
decoder exception originates at the C++ boundary rather than in Python byte
normalization. The same twenty-six malformed primitive, string, opaque, and
boolean payloads also cross the exact Java `decode(byte[])` overload, retaining
the standard Java `DecoderException` before Python normalization.
The raw Java variant-record companion now mirrors the C++ mapped-alternative
alignment vector: an integer alternative is padded to the maximum mapped
boundary, a `ByteWrapper` consumes exactly one record while preserving a
following sentinel, and an unknown discriminant consumes only its own bytes.
The Python façade observes the same native record and unknown-alternative
projection.
The raw Java variable-array companion now mirrors the C++ signed-count
float64 vector: it preserves the four-octet count, zero leading padding, and
one-element cursor boundary, while negative counts, nonzero padding, and
trailing byte-array data remain typed `DecoderException` failures in both raw
Java and the Python façade.
The fixed-record/fixed-array companion now mirrors the C++ declaration-order
alignment vectors as well: raw Java and Python emit the required zero padding,
consume exactly one composite from a sentinel-bearing `ByteWrapper`, and map
nonzero padding or trailing bytes to the standard `DecoderException`.
The provider-specific Java extendable-variant carrier now has its own raw
`ByteWrapper` matrix: C++ rejects short and mapped inconsistent-length
payloads, while a well-formed unknown alternative is skipped and surfaced as
an unmapped Java value with its typed discriminant intact. The cursor overload
consumes one exact element and leaves a following sentinel for the caller;
the exact `byte[]` overload continues to reject a trailing payload.
The same known-alternative carrier now exercises `encode(ByteWrapper)` through
both raw Java and the provider-scoped Python façade, preserving the native
payload at a non-zero offset and leaving the trailing sentinel untouched.
The companion handle-factory vector applies empty, truncated, trailing, and
wrong-count variable-array envelopes to every public Java handle decoder and
preserves typed `CouldNotDecode` through JNI and JPype.
The standard Java encoder vector now calls all nine raw handle-factory
`decode(byte[], offset)` methods directly with the same malformed envelopes,
proving the typed Java `CouldNotDecode` surface before the Python adapter is
involved.
The exact Java `AttributeSetRegionSetPairListFactory` carrier is also exercised
directly: `AttributeRegionAssociation` public fields, `List` add/addAll,
contains/index lookup, iteration, array conversion, remove, clone,
clear/is-empty behavior, and a raw Java regional subscription/unsubscription
call all cross the C++ JNI boundary before the Python-normalized DDM vectors.
Undersized destinations for the live raw Java `DataElement.encode(ByteWrapper)`
overload are deliberately not invoked in the pass gates: the current bounded
C++ JNI fixture does not return a typed encoder failure for that window and can
remain inside the native call. Typed destination-window failures are therefore
covered by the provider-neutral fake/vendor carriers, while the live claims
remain limited to successful standard encode/decode transport.
The Python façade now preflights `remaining()` against the provider-reported
encoded length and raises `EncoderException` before JNI for an undersized
window, giving Python callers a deterministic boundary even while the raw Java
behavior remains bounded. Provider-neutral tracking tests for both edition
adapters assert that this guard does not call the foreign `encode` method.
The same raw Java handle carriers now exercise both `encode()` and cursor-based
`encode(ByteWrapper)`/`decode(ByteWrapper)` overloads for every public handle
domain, preserving cursor advancement and C++ handle identity. The JNI handle
and logical-time data-element proxies decode from non-advancing slices and
consume exactly one encoded element, leaving trailing sentinel bytes and the
cursor unchanged on short-input failure through both raw Java and Python
facades. The extendable-variant proxy follows the same cursor contract,
including unknown alternatives, while its exact `byte[]` overload rejects
trailing payload bytes.
The matrix now includes `createHLAmessageRetractionHandle` using a live C++
timestamped-interaction retraction returned by the exact Java RTIambassador
overload, rather than only testing the name-addressable handle domains.
The raw Java attribute- and parameter-value maps likewise exercise
`containsKey`, key/value/entry views, `getValueReference` with a fresh and
reusable `ByteWrapper`, clone isolation, and C++-backed
`UpdateAttributeValues`/`SendInteraction` calls.

That external run proves the adapter uses the actual Java shapes where they
differ from the historical fixture: `String`/`String[]` FOM inputs; time
carriers from the `time` package and their standard `getValue` accessors;
timestamped service overloads returning `MessageRetractionReturn`; typed
interaction-class sets; and `AttributeSetRegionSetPairListFactory` with
`AttributeRegionAssociation(ahset, rhset)`. The callback gate also reflects the
loaded external `FederateAmbassador`, verifies all 56 exact callback names, and
constructs the actual standard JPype proxy before accepting the C++ callback-slot
and JPype marshaller audit. It now also checks every reflected Java callback
overload arity against the dispatcher's required/optional Python argument range.
The companion carrier matrix invokes all 62 reflected overloads once for each
integer and floating logical-time route, using C++-backed Java handles,
typed collections, enums, status arrays, information sets, byte arrays, and
time values before checking their provider-neutral Python shapes.
The exact external `RTIambassador` reflection
surface is audited separately: every overload declared by the loaded IEEE Java
interface (206 declarations, including the four `connect` forms and the
timestamped/region variants) must match an explicit `NativeRTIambassador`
`values.length` branch or one of the two deliberate compact overload helpers
(`connect` and `joinFederationExecution`). This closes the gap where a shared
Python service name could be present while one exact Java overload still fell
through to the unbound-service error.
C++ remains the only RTI semantic
implementation in all cases. The IEEE API JAR is intentionally neither
tracked nor repackaged by Umbra; users supply it when building or testing the
external consumer route.

The package-discovery route was also checked from clean wheel metadata on
2026-08-20. Installing freshly built `hla-rti-api`, `umbra-rti-jpype`, and
`umbra-rti-native` wheels into an isolated target exposed both standard API
entry points (`java` and `Umbra`). `RtiFactoryFactory.getRtiFactory("Umbra")`
loaded the installed pybind provider and reported its C++ version, while
`getRtiFactory("java")` selected the lazy JPype transport without starting a
JVM. The native source-checkout test therefore skips only when distribution
metadata is absent; installed-wheel discovery remains an explicit packaging
check rather than a provider import hidden in the pure API package.
The current native source-checkout artifact also passes all 61 pybind tests
(one metadata-only discovery skip), including a source-level inventory guard
that compares every `_UmbraRTIambassador._implementation` call with the
methods exported by the loaded `NativeAmbassador` extension. This keeps the
direct C++ → Python façade from drifting while the JNI route remains the
provider-neutral conformance path.
The Java-provider lifecycle companion also invokes the exact Java proxy's
`AutoCloseable.close()`, verifies that the native state is rejected after
destruction, and confirms repeated close calls are harmless.

## Current completed verticals

| Family | Public services | Public callbacks/value boundary | Status |
| --- | --- | --- | --- |
| Connection foundation | `connect` forms, disconnect, callback control/evocation | `ConfigurationResult`, credentials/configuration, `connectionLost` | Native and Java foundation covered; the external IEEE-JAR JNI vector now executes all four standard `connect` overloads (bare, configuration, credentials, and configuration-plus-credentials), copies arbitrary standard Java `Credentials` type/data into the C++ `Credentials` value, and proves a rejected non-`HLAnoCredentials` credential returns the typed C++-originated `Unauthorized`; it also proves disconnect-while-member maps to `FederateIsExecutionMember`, Python callback failures map to `FederateInternalError`, and the C++ Connect/Disconnect callback-context guards preserve `CallNotAllowedFromWithinCallback` before lifecycle or credential/configuration work |
| Java authorization factory | IEEE `AuthorizerFactory` / `AuthorizerFactoryFactory`, `Credentials`, and `AuthorizationResult` | raw Java ServiceLoader surface backed by the C++ reference authorizer; the common Python contract remains provider-neutral | The external IEEE-JAR JNI vector discovers `HLAauthorizer`, exercises RTI/federation/federate authorization calls, matching/wrong/no/unknown/malformed credentials, and unconfigured-policy results; no Java authorization semantics are reimplemented |
| Federation discovery | scalar/vector FOM create, create-with-MIM, destroy/list federation, list members | typed immutable federation/member reports and missing-federation callback | Native and Java covered using the exact 2025 Java `String[]`/`String` FOM and MIM overloads; the external IEEE-JAR JNI vector lists two simultaneous executions and resolves a class from a compatible join-time additional FOM; duplicate create and destroy-while-joined failures now preserve typed exceptions and emit separately decoded `HLAreportException` records from the C++ lifecycle boundary, including the MIM-specific duplicate-create overload after compatible FOM/MIM validation |
| Membership | named/unnamed join with optional additional FOM modules; resign | encoded immutable `FederateHandle`, `ResignAction`, `federateResigned` reason callback, and object-removal consequences | Native and Java adapter seams cover scalar and exact-standard `String[]` FOM-module sequences, including the RTI-originated resignation callback; the external IEEE-JAR JNI vector now exercises the iterable `String[]` create form and all six standard resignation actions, with member reports after every join/resign transition, proves `DELETE_OBJECTS` removes the departing member's known object through the standard `removeObjectInstance` callback with the C++ owner handle and empty resign tag, proves final-federate `NO_ACTION` still releases an object name for reuse after rejoin, and proves the C++ RTI-originated resignation callback removes only the controlled member while leaving its Java connection reusable for rejoin; duplicate join on an already-member Java ambassador now preserves `FederateAlreadyExecutionMember`, a member-owned object preserves `FederateOwnsAttributes` from `NO_ACTION` resignation while emitting a separately decoded C++ `Resign Federation Execution` `HLAreportException` through JNI/JPype before the standard corrective action completes, and callback-driven Connect, Join, and Resign calls preserve `CallNotAllowedFromWithinCallback` without poisoning the subsequent normal resignation; the federate-lookup companion preserves disconnected/unjoined preconditions, cross-federation `FederateHandleNotKnown`, missing joined names, and the departed member's immutable name through the standard Java handle/name services |
| Synchronization points | register for current members or an explicit federate-handle set; achieve | registration success/failure, announcement bytes, completion `FederateHandleSet` | Native and real-JVM fixture lifecycles cover both registration overloads; the external IEEE-JAR JNI vector proves requester-only registration success, typed `SYNCHRONIZATION_SET_MEMBER_NOT_JOINED` failure, a two-member explicit `FederateHandleSet`, announcements to both members, completion only after both members achieve, and the standard `successfully=false` achievement path returning the failed member handle set to both callbacks; the edge matrix preserves disconnected/unjoined preconditions, unknown-label `SynchronizationPointLabelNotAnnounced`, asynchronous duplicate-label failure, and idempotent repeated achievement with one-shot completion callbacks; the two joined-member unknown-label achievements now also decode separate C++ `HLAreportException` records through the official Java encoder and JPype |
| Save status discovery | query federation save status | ordered immutable `FederateHandleSaveStatusPair` response with typed `SaveStatus` | Native two-member integration and real-JVM fixture status callbacks cover no-save, active-saving, and post-completion transitions |
| Scalar save success | request, begun, complete | `initiateFederateSave`, `federationSaved` | Native two-member lifecycle, real-JVM fixture lifecycle, and external IEEE-JAR JNI vector covered; both Java routes coordinate initiation and completion across two members and verify the federation-wide completion barrier |
| Timestamped save request | request with provider `LogicalTime` | provider-created timestamp crosses the native decode boundary, Java overload, and distinct `initiateFederateSave(label, time)` callback; initiation is held until the fixture's time boundary | Native C++ timed-save admission covers immediate and constrained two-federate grant boundaries, an in-transit timestamped interaction delivered before save admission, and a re-entrant save requested from the TSO callback (initiation waits for callback return); the C++ → JNI → Java → JPype vectors prove integer and floating callback times retain their type/value, and the external IEEE-JAR vectors prove pending admission at the regulator boundary for TAR and strict TARA, TSO receive → initiate-save → grant ordering for TAR/NMRA, all five constrained TAR/NMR/TARA/NMRA/FQR advance modes before their qualifying grants, each of those five modes' restore-complete-before-reconstructed-grant ordering, and save-request deferral until callback return |
| Scalar save failure/abort | not-complete; abort | typed `federationNotSaved` `SaveFailureReason` | Native two-member failure/abort and real-JVM fixture failure/abort paths covered; the real-JVM fixture and external IEEE-JAR JNI vector now broadcast federate-reported failure and abort outcomes to both members with the exact typed reason; the external route also maps `FEDERATE_RESIGNED_DURING_SAVE` when a member resigns, rejects stale `federateSaveBegun`, and proves the remaining member can complete a later save |
| Save/restore state preconditions | save begun/complete/abort and restore complete/abort before acceptance; overlapping save/restore requests | standard typed state-machine exceptions | The external IEEE-JAR JNI vector preserves `SaveNotInitiated`, `FederateHasNotBegunSave`, `SaveNotInProgress`, `RestoreNotRequested`, `RestoreNotInProgress`, `SaveInProgress`, and `RestoreInProgress` through C++ → JNI → Java → JPype → Python; each representative failure is also emitted as a separately decoded standard `HLAreportException` with the C++ service label, covering both save-request overloads' shared boundary and all save/restore control/status methods, including `Register Federation Synchronization Point` while a save is active; the two-member exact-JAR interlock matrix holds a Java ambassador inside active save and restore barriers and proves declaration, object registration, region creation, time query, synchronization, interaction, order-change, and every attribute-ownership service receive the correct typed barrier exception before the operation completes, with separate C++ MOM reports for the ownership controls |
| Restore status discovery | query federation restore status | ordered immutable `FederateRestoreStatus` response with typed handles and `RestoreStatus` | Native two-member and real-JVM fixture status callbacks cover no-restore, active-restoring, and post-completion transitions |
| Scalar restore success | request; complete | request acceptance/rejection, begin, per-federate initiation with a typed handle, `federationRestored` | Native two-member, real-JVM fixture, and external IEEE-JAR JNI lifecycles covered; the Java routes coordinate per-member completion, preserve requester-scoped acceptance, return typed initiation handles, and the external route rewinds saved logical time/lookahead and preserves terminal retractions while invalidating post-save handles, while native and external restore reinstate a live timestamped interaction/retraction ledger with multiple ordered messages; the external ownership-state vector saves an object while federate A owns an attribute, transfers it to federate B after save, and proves restore reinstates A's C++ ownership through Java and JPype; the object-management companion saves before post-save declarations/object registration and proves restore removes the post-save object through the standard Java lookup boundary; the regional-object companion saves an overlapping association, mutates its source range to disjoint, and proves restore rewinds the source region and delivers a later update with the restored region designator; the two-constrained-member timed-restore companion now restores one queued TSO and its shared retraction ledger to both Java recipients, preserving per-member interaction-before-flush-grant ordering and identical C++ retraction bytes |
| Scalar restore failure/abort | not-complete; abort | typed `federationNotRestored` `RestoreFailureReason` | Native two-member and real-JVM fixture failure/abort paths covered; the real-JVM fixture and external IEEE-JAR JNI vector broadcast federate-reported failure and abort outcomes across the shared restore transaction with exact `FEDERATE_REPORTED_FAILURE_DURING_RESTORE` and `RESTORE_ABORTED` reasons, and map `FEDERATE_RESIGNED_DURING_RESTORE` when a participant resigns during an active restore |
| Basic encoding | `getEncoderFactory`; twenty-six provider-owned factory forms | provider-owned `DataElement`: `HLAinteger16BE`, `HLAinteger16LE`, `HLAinteger32BE`, `HLAinteger32LE`, `HLAinteger64BE`, `HLAinteger64LE`, `HLAfloat32BE`, `HLAfloat32LE`, `HLAfloat64BE`, `HLAfloat64LE`, `HLAunsignedInteger16BE`, `HLAunsignedInteger16LE`, `HLAunsignedInteger32BE`, `HLAunsignedInteger32LE`, `HLAunsignedInteger64BE`, `HLAunsignedInteger64LE`, `HLAbyte`, `HLAoctet`, `HLAASCIIchar`, `HLAASCIIstring`, `HLAunicodeChar`, `HLAoctetPairBE`, `HLAoctetPairLE`, `HLAopaqueData`, `HLAboolean`, `HLAunicodeString`; typed encoder/decoder errors | Native C++ and fake/real-JVM fixture vectors cover signed 16/32/64-bit, binary32/64 in both byte orders, unsigned 16/32/64-bit in both byte orders, distinct byte/octet and ASCII/Unicode character identities, ASCII element-count strings, BE/LE octet-pair order, four-byte-count opaque vectors, canonical four-octet boolean values, UTF-16 element-count Unicode vectors, and typed `DecoderException` mapping for malformed fixed-width, canonical-boolean, and variable-length scalar inputs; the external IEEE-JAR JNI vector also drives the exact standard Java `ByteWrapper` encode/decode cursor overloads through raw Java and the provider-scoped Python façade for primitive, fixed-record, fixed-array-varargs, variable-length ASCII/unicode-string, opaque-data, and variable-array carriers with non-zero offsets, consumed-byte validation, malformed length-prefix handling, and malformed fixed-record/fixed-array/variable-array child/count payloads |
| Variable-array encoding | `EncoderFactory.createHLAvariableArray(DataElementFactory, elements...)`; exact Java `resize(int)` | provider-owned `HLAvariableArray`: `DataElementFactory.createElement`, initial varargs/copy additions, `addElement`, `resize`, `size`, `get`, iteration, aligned count/padding, and typed decoder errors | Native C++, fake/real-JVM, and C++ → JNI → Java → Python vectors cover primitive and nested fixed-record prototypes, count/prototype alignment, copied elements, decoded child factories, type checks, standard Java resize through the provider-scoped Python façade, and malformed count/padding/trailing input |
| Fixed-array encoding | `EncoderFactory.createHLAfixedArray(DataElementFactory, size)` and exact Java `DataElement...` overload | provider-owned `HLAfixedArray`: fixed cardinality, `size`, `get`, iteration, copied `set` convenience, prototype alignment, and typed decoder errors | Native C++, fake/real-JVM, and C++ → JNI → Java → Python vectors cover default primitive and nested fixed-record elements, copied slot replacement, C++ inter-element alignment, bounds/type/size validation, trailing-data rejection, and the provider-scoped Python façade's exact Java varargs carrier |
| Fixed-record encoding | `EncoderFactory.createHLAfixedRecord()` | provider-owned heterogeneous `HLAfixedRecord`: copied `appendElement`, `size`, `get`, iteration, copied `set` convenience, per-component alignment, and typed decoder errors | Native C++, fake/real-JVM, and C++ → JNI → Java → Python vectors cover heterogeneous scalars, recursively cloned fixed-record components, record-offset padding, copied append semantics, replacement/type/bounds checks, and malformed padding/trailing input |
| Variant-record encoding | `EncoderFactory.createHLAvariantRecord(DataElement)` | provider-owned `HLAvariantRecord`: copied `setVariant`, `setDiscriminant`, `getDiscriminant`, `getValue`, discriminant/alternative alignment, and unmapped-discriminant encoding | Native C++, fake/real-JVM, and C++ → JNI → Java → Python vectors cover scalar and nested fixed-record alternatives, copied mappings/replacements, discriminant selection, unmapped wire forms, and malformed padding/trailing input |
| Native extendable-variant encoding | C++ `HLAextendableVariantRecord`; the Java 2025 `EncoderFactory` declares a creator but omits the C++ mapping-registration operations | provider-specific native `DataElement` façade with `addVariant`, `setVariant`, `setDiscriminant`, `getDiscriminant`, `getValue`, length-prefixed alternatives, and unknown-alternative skipping | Native pybind and C++ vectors cover copied mappings, alternative length/padding, mapped decode, unknown-alternative skipping, and malformed length/padding/trailing input. The Java JNI adapter now binds the standard creator through its provider-scoped Python façade, registers a mapping on its first `setVariant`, and clones the carrier inside a standard Java fixed record; the shared Python-shaped `EncoderFactory` deliberately remains narrower because the native provider has no matching standard factory method |
| Support name/handle lookup | federate, object class, attribute, interaction class, parameter, transportation type, and dimension `get*Handle` / `get*Name` pairs plus known object-class lookup | distinct immutable encoded public handle domains | Native Restaurant FOM and fake/real-JVM fixture round trips covered; the external IEEE-JAR JNI vector now preserves typed failures for unknown federate/class/attribute names and structurally valid unknown handles, and decodes their C++ `HLAreportException` service records through JPype; update-rate designator and object-instance lookup failures are covered through the same boundary, including known-object-class, object-instance-name, and object-instance-handle lookups for an unknown federate-local object |
| Handle, set, and map factories | Java-declared handle decoder accessors, including `MessageRetractionHandleFactory`, interaction-class and attribute/region pair-list factories, federate/dimension/region/attribute set factories, and attribute/parameter value-map factories | provider-backed handle decoders plus mutable Java-shaped builders (`MutableInteractionClassHandleSet`, `MutableAttributeSetRegionSetPairList`) accepted by service calls and copied at the boundary; `AttributeRegionAssociation(ahset, rhset)` mirrors the exact Java value shape | Native C++ codec/RTI decoders and fake/real-JVM Java factory lifecycles covered; the external JNI vector directly invokes the official interaction-set and pair-list factories, decodes a live timestamped retraction through the official message-retraction factory, and exercises the exact Java subscription aliases |
| C++-only codec extensions | C++ `decode*Handle` helpers without a corresponding Java standard service | provider-owned native codec convenience only; standard Java-shaped Python uses the public factories | Kept out of the shared Java-shaped surface where IEEE 1516.1-2025 has no matching method |
| Support value lookups | update-rate value/designator queries, order-type/name conversion, available dimensions for object/interaction classes, and dimension upper bounds | typed `OrderType`, immutable `DimensionHandleSet`, and provider numeric values | Native Restaurant FOM and fake/real-JVM Java adapter mappings covered; the external IEEE-JAR JNI vector preserves `InvalidDimensionHandle` for unknown but structurally valid dimension identities through name, upper-bound, and region-creation services |
| Support normalization | service-group and federate/object-class/interaction-class/object-instance normalization coordinates | typed `ServiceGroup` plus strict handle-domain validation and integer provider coordinates | Native C++ and fake/real-JVM Java adapter mappings covered; the external IEEE-JAR JNI vector now preserves malformed/unknown handle failures and decodes distinct `Normalize Federate Handle`, `Normalize Object Class Handle`, `Normalize Interaction Class Handle`, and `Normalize Object Instance Handle` `HLAreportException` records through Java and JPype |
| Timestamped object/interaction services | timestamped update, interaction send (regional and non-regional), object deletion, and message retraction | provider-created `LogicalTime` input and typed immutable `MessageRetractionHandle` results; timed reflect/interaction/removal metadata, copied region designators, and `requestRetraction` callback | Native C++ and fake/real-JVM Java service/retraction mappings plus native/real-JVM timed callback conversion covered; the real-JVM fixture now fans timestamped object and interaction traffic to unconstrained and constrained subscribers, preserves RECEIVE versus TIMESTAMP order, routes retractions only to delivered recipients, and covers regional interaction/object designator convey on mixed recipients; the external IEEE-JAR JNI route now restores multiple ordered timestamped interactions and their live retraction ledger through a two-member save/restore barrier, proves timestamped default-region interaction delivery with the standard empty `RegionHandleSet`, `TIMESTAMP` sent/received order metadata, and consumed retraction handle, fans timestamped object deletion and directed interaction services to FQR/TARA/NMRA recipients before their grants with matching removal/direct-target/time/order metadata, proves a regional timestamped update can be retracted while all three grant requests remain pending without reflection or `requestRetraction`, and maps terminal retraction consistently, decodes both nonregional and region-context timestamped `SendInteraction` MOM reports with logical-time supplied arguments, region-set arguments, and message-retraction return arguments, preserves terminal/malformed `Retract` failures as C++ `HLAreportException` records through Java and JPype, and proves an accepted C++ timestamped attribute update retains its original producer, values, timestamp, order metadata, and retraction handle when ownership transfers before the constrained recipient's grant |
| Basic declaration management | object-class attribute and interaction publish/unpublish plus exact Java active/passive/universal subscribe overloads | immutable `AttributeHandleSet`, mutable interaction-class factory builders; typed object/interaction relevance advisories | Native two-federate lifecycle and real-JVM fixture covered; external JNI vectors invoke the Java-named passive and universal methods while the normalized Python flags remain available. FOM `sharing` values remain authoritative catalog/SOM capability metadata; the 2025 RTI API does not define a generic declaration exception for `Publish`/`Subscribe` mismatches, so Umbra enforces only concrete service preconditions such as the standard MOM report-service conflict; the exact Java/JPype lookup vector now drives invalid object/interaction declaration, directed-declaration, and regional interaction subscription handles through C++ exception-report boundaries. |
| Receive-order object management | reserve/release single or multiple object-instance names; named or provider-named registration; object-instance name/handle lookup; receive-order and local deletion plus attribute update | immutable `ObjectInstanceHandle`/`ObjectInstanceNameSet` and byte-copying `AttributeHandleValueMap`; discovery, removal, reflection, and single/batch reservation callbacks with typed source/transport handles; local-delete preconditions map to typed ownership exceptions | Native and real-JVM Restaurant FOM lifecycles now prove single-name reserve→callback→release plus batch reservation; fake/real-JVM fixtures cover matching set/map factory, service, and callback conversions; the external IEEE-JAR JNI vector also preserves `NotConnected`, `FederateNotExecutionMember`, `IllegalName`, `NameSetWasEmpty`, `ObjectInstanceNameNotReserved`, asynchronous single-name contention failure, and partial batch success/failure callbacks, with the synchronous reservation/release failures decoded as C++ `HLAreportException` interactions through Java and JPype; real-JVM fixture now fans one registration/update out to two independent subscribers; timestamped update/reflection is covered in the dedicated DDM rows |
| Receive-order interaction management | `sendInteraction` with published parameters | byte-copying `ParameterHandleValueMap`; interaction receipt with typed source/transport handles | Native two-federate Restaurant FOM lifecycle and real-JVM fixture covered; real-JVM fixture now fans one send out to two independent subscribers; the external IEEE-JAR JNI lane also proves accepted direct and regional `Send Interaction` calls emit the exact seven-parameter `HLAreportServiceInvocation` through C++ → JNI → Java → JPype, with Java `EncoderFactory` decoding, serial zero, reliable receive-order delivery, the RTI-originated invalid producer handle for the direct path, and the regional region-set argument; the regional private-file vector additionally decodes the type-27/type-40/type-43/type-63/type-34 successful-void record before the standard regional callback |
| Logical time management | provider `LogicalTimeFactory`, initial/final/zero/epsilon values, encode/decode, provider-owned add/subtract/difference arithmetic, standard carrier predicates/comparison/equality, regulation/constrained mode, asynchronous delivery, lookahead query/modification, all five time-advance/queue request variants, GALT/LITS queries, and timestamped service inputs | immutable provider-created `HLAinteger64Time`/`HLAinteger64Interval` and `HLAfloat64Time`/`HLAfloat64Interval` snapshots, Java-shaped `TimeQueryResult`, and typed regulation/constrained/grant/`flushQueueGrant` plus timed object/interaction callbacks | Native C++ and fake/real-JVM Java adapter lifecycles cover integer time; native and real-JVM restore tests preserve saved logical time, actual lookahead, and deferred lookahead decreases; the C++ and Java providers now delegate factory construction, factory decode, add/subtract/difference, and validation to their native logical-time implementations and preserve exact floating values through factory, decode, arithmetic, query, grant, and timed callbacks, including smallest-positive-subnormal epsilon parity, exact and one-step-overflow additions at the largest finite float64 value, the symmetric exact and one-step-overflow additions at signed `Long.MAX_VALUE`, an epsilon-sized floating time-advance/grant, and typed `CouldNotDecode` mapping for truncated, trailing, negative, and non-finite integer/floating encodings; the external IEEE-JAR JNI vector also proves representable-boundary float64 add/subtract/distance, integer64 arithmetic above 2^53, direct standard Java factory construction/decode, interval arithmetic, and standard Java carrier `isInitial`/`isFinal`/`isZero`/`isEpsilon`/`compareTo`/`equals`/`hashCode` methods, typed `IllegalTimeArithmetic` mapping through C++ → JNI → Java → JPype, and `SaveInProgress` HLAreportException records for `Query Logical Time`, `Query GALT`, and `Query LITS`; the private file vector additionally decodes a type-31 `TimeAdvanceRequest` logical-time argument before the standard Java grant callback; broader vendor-specific arithmetic matrices remain |
| Region substrate | `createRegion`, `commitRegionModifications`, `deleteRegion`, dimension-set and range-bound services | immutable `RegionHandle`/dimension and region sets plus mutable Java-shaped `RangeBounds` | Native C++ and fake/real-JVM Java adapter lifecycles covered; the external IEEE-JAR JNI vector also preserves `InvalidRegion`, `RegionDoesNotContainSpecifiedDimension`, and `InvalidRangeBound` for incomplete regions, unrelated dimensions, invalid bounds, and repeated deletion; a two-member foreign-region matrix preserves `InvalidRegion` for read-only lookups and `RegionNotCreatedByThisFederate` for mutating set/commit/delete services, and an unknown-but-well-formed dimension now proves typed `InvalidDimensionHandle` from `Create Region`; each C++ region lifecycle/range failure is decoded as an `HLAreportException` through the official Java carriers and JPype |
| Regional interaction management | regional subscribe/unsubscribe, regional receive/timestamp-order send, and convey-region-designator switch | optional `RegionHandleSet` plus timed callback metadata on interaction callbacks | Native C++ and fake/real-JVM Java adapter callback mappings cover overlapping versus disjoint recipients, including receive-order fanout, timestamped sends, typed retraction routing, and conveyed region designators; both providers now reproject an existing subscription as overlapping regions are added and removed without duplicate delivery, re-evaluate explicit regional subscriptions at the callback boundary when delayed subscription evaluation is enabled, preserve mixed region-designator convey, and honor the bounded Allow Relaxed DDM exact-boundary policy; the external IEEE-JAR JNI vector proves two simultaneous overlapping subscriptions remain active after selective unsubscription and stop delivery only after the final overlap is removed, and preserves `RegionNotCreatedByThisFederate`, `InvalidRegion`, and `InvalidRegionContext` for foreign, uncommitted, and wrong-dimension subscription/send regions; its dedicated report vector enables exception reporting on both originating federates and decodes seven separate C++ `HLAreportException` records for regional subscription/send/unsubscription failures through the official Java encoder and JPype; a two-dimensional external vector requires overlap in both X and Y, then mutates one committed source region to exercise X-only and Y-only recipient transitions through the standard Java region carriers; the external IEEE-JAR JNI vector also proves timestamped default-region interaction delivery with the standard empty `RegionHandleSet`, `TIMESTAMP` sent/received order metadata, and consumed retraction handle, plus receive-order and timestamped region-context `HLAreportServiceInvocation` payloads through the exact Java encoder path; the explicit-source and default-region re-enable vectors queue timestamped regional interactions, disable and re-enable the constrained Java member, and prove one preserved callback each with source-region/empty-region realization, order metadata, and consumed retraction handles; the regional-interaction restore vector saves an overlapping subscription, mutates the source range to disjoint, and proves restore rewinds the range and delivers a later interaction with its restored region designator |
| Directed interaction management | object-class directed publish/unpublish and subscribe/unsubscribe overloads; receive-order and timestamped directed sends | immutable `InteractionClassHandleSet`, typed target object/source/transport callback payloads, and optional time/order/retraction metadata | Native C++ receive-order delivery and three-federate timestamped retraction fanout prove only delivered recipients receive `requestRetraction`; fake/real-JVM Java declaration, send, and timed callback mappings are covered, including real-JVM multi-recipient directed fanout and constrained pending retraction. The external IEEE-JAR JNI/JPype MOM companion now decodes untimed, timestamped receive-order, and time-regulated timestamped `SendDirectedInteraction` reports, including type-34 null and type-33 `MessageRetractionHandle` return arguments and serial ordering |
| Regional object management | Java-shaped `AttributeSetRegionSetPairList`; regional object registration, regional subscription, association/unassociation, and regional value requests | immutable attribute-set/region-set pair values; C++ typedef aliases remain available | Native C++ overlap/discovery/update lifecycle includes mixed-fanout timestamped reflection and convey-switch behavior, independent per-attribute region association with selective retention/disjoint filtering, default-region fallback, existing-object discovery reprojection as a subscription gains multiple overlapping regions without duplicate discovery, scope-advisory out/in transitions across partial and complete regional unsubscription/re-subscription, and a two-subscriber receive-order association matrix that brings only a previously out-of-scope known recipient back into scope before selectively removing it; default-region callback realization and exact-boundary relaxed DDM are covered; both providers now re-evaluate explicit regional object updates at the callback boundary when delayed subscription evaluation is enabled; fake Java callback conversion covers present/absent region metadata, and the real-JVM adapter carries per-attribute registration/association/unassociation region sets into region-relevant discovery and timestamped multi-recipient reflection, broadcasts association scope changes to the affected recipient only, conveys an empty `RegionHandleSet` for receive-order and timestamped default-region realizations (while ordinary non-regional callbacks remain `None`), filters disjoint ranges before delivery, routes `requestAttributeValueUpdateWithRegions` to the owning federate only when source and requester ranges overlap while suppressing disjoint requesters and copying the request tag, proves the standard Java `provideAttributeValueUpdate` callback can answer a regional request and return a region-conveyed reflection with copied values/tag/transport/producer metadata, proves a registered object remains undiscovered under a passive regional subscription until an active declaration replaces it, exercises exact-boundary relaxed DDM updates, maps deletion of a still-referenced source region to `RegionInUseForUpdateOrSubscription`, and proves an already-known object re-enters scope after a complete regional unsubscription; the C++ → JNI → Java → Python vectors now prove live association mutation, two-subscriber recipient isolation, selective in/out callbacks, empty default-region fallback, restoration of both recipients after the final source unassociation, and two-dimensional known-object reflection filtering with independent X-only and Y-only source-range transitions; the external timestamped regional object-update vectors also survive a constrained-member disable/re-enable transition once, preserving explicit source-region and empty default-region realizations, order metadata, and retraction handles, and the dedicated TAR/NMR vector proves both standard request modes deliver one regional TSO before their grant at logical time 7 with identical C++ retraction bytes; invalid foreign, uncommitted, wrong-context, and unknown-object/region pair-list cases also emit decoded C++ `HLAreportException` records for regional registration, association/unassociation, and regional subscription teardown, while the live-registration guard independently decodes `RegionInUseForUpdateOrSubscription` from `Delete Region` through an observer's standard Java MOM subscription |
| Attribute value update requests | class- and instance-targeted `requestAttributeValueUpdate` overloads and federation-defined automatic provision | typed class/instance handle dispatch, immutable `AttributeHandleSet`, copied request tags, and `provideAttributeValueUpdate` callback | Native C++ and fake/real-JVM Java adapter mappings covered, including callback tag/attribute conversion; the external IEEE-JAR JNI vector also proves C++ automatic provision queues the standard empty-tag callback after discovery through the Java `String[]` FOM-create overload and that standard `HLAsetSwitches` interaction parameters enable/disable later discovery-triggered callbacks federation-wide; class, instance, and `requestAttributeValueUpdateWithRegions` failures now preserve typed C++ exceptions and emit the corresponding `HLAreportException` through the official Java set/region carriers; the timestamped regional request/response vector answers the standard Java `provideAttributeValueUpdate` callback with a C++-queued timed update and proves constrained grant ordering, `TIMESTAMP` sent/received metadata, source-region conveyance, and retraction consumption |
| Object-management advisory callbacks | scope entry/exit and per-object update relevance (plain and named-rate forms) | typed `ObjectInstanceHandle`/`AttributeHandleSet` callback payloads with optional update-rate designator | Native C++ regional scope transitions and fake/real-JVM Java callback fixtures covered; the external IEEE-JAR JNI vector preserves a named `High` update-rate designator while C++ source-region association moves out of and back into overlap |
| MOM request/report interactions | Subscribe-only `HLArequestObjectInstancesUpdated` / `HLArequestObjectInstancesThatCanBeDeleted` / `HLArequestObjectInstancesReflected` / `HLArequestUpdatesSent` / `HLArequestInteractionsSent` / `HLArequestDirectedInteractionsSent` / `HLArequestInteractionsReceived` / `HLArequestDirectedInteractionsReceived` / `HLArequestReflectionsReceived` / `HLArequestObjectInstanceInformation` / `HLArequestPublications` / `HLArequestFOMmoduleData` / federation-scoped `HLArequestFOMmoduleData` / `HLArequestMIMdata` / `HLArequestSynchronizationPoints` / `HLArequestSynchronizationPointStatus` / `HLArequestSubscriptions` and RTI-originated `HLAreportObjectInstancesUpdated` / `HLAreportObjectInstancesThatCanBeDeleted` / `HLAreportObjectInstancesReflected` / `HLAreportUpdatesSent` / `HLAreportInteractionsSent` / `HLAreportDirectedInteractionsSent` / `HLAreportInteractionsReceived` / `HLAreportDirectedInteractionsReceived` / `HLAreportReflectionsReceived` / `HLAreportObjectInstanceInformation` / `HLAreportObjectClassPublication` / `HLAreportInteractionPublication` / `HLAreportDirectedInteractionPublication` / `HLAreportFOMmoduleData` / federation-scoped `HLAreportFOMmoduleData` / `HLAreportMIMdata` / `HLAreportSynchronizationPoints` / `HLAreportSynchronizationPointStatus` / `HLAreportObjectClassSubscription` / `HLAreportInteractionSubscription` / `HLAreportDirectedInteractionSubscription` | standard interaction callback metadata plus nested `HLAobjectClassBasedCounts`, `HLAupdateCounts`, `HLAinteractionCounts`, `HLAreflectCounts`, `HLAattributeHandleList`, publication-report handle lists, subscription-report handle/active/rate records, synchronization-label/status arrays, and HLAunicodeString FOM/MIM payloads, with transportation-handle grouping and separate ordinary/directed send, receive, reflection, object-information, publication, joined-module, federation-content, synchronization, and subscription projections | External IEEE-JAR C++ → JNI → Java → JPype vectors covered: the RTI consumes all seventeen standard requests, reports one count per registered class over reliable transport, preserves the invalid RTI producer identity, decodes nested class-handle/count records through the standard Java encoder, removes a deleted class from the live ownership-count response, separates distinct reflected objects from repeated updates, groups accepted updates into best-effort versus reliable transportation buckets, groups ordinary plus dimensioned interaction sends by transportation, keeps directed sends separate with an empty best-effort bucket, reports reliable/best-effort ordinary receives plus reliable directed receives with the ordinary receive excluded, reports reliable/best-effort application reflections plus empty buckets for a federate with no reflections, preserves NULL/known/registering-federate object-information shapes and owned attribute lists, decodes publication reports plus zero-count/empty-list responses after unpublication, decodes both retained joined FOM XML and federation-scoped FOM/MIM text, tracks synchronization labels and per-federate status before/after achievement, and decodes active/passive/directed subscription records plus zero/empty projections after unsubscription; malformed `HLAsetSwitches` and `HLArequestObjectInstancesUpdated` requests now also preserve typed `InteractionParameterNotDefined` and decode `HLAreportMOMexception` through the standard Java encoder; all standard MOM request/report families now have positive or malformed external evidence, with only broader provider-specific state-space combinations remaining |
| Order and transportation management | `changeAttributeOrderType`, default-attribute and interaction order changes; attribute/interaction transportation change and query services | strict `OrderType`/`TransportationTypeHandle` domains plus typed confirmation/report callbacks | Native C++ and fake/real-JVM Java adapter mappings covered; the external IEEE-JAR JNI vector now drives all eight standard Java order/transport overloads through JPype, preserving `ObjectInstanceNotKnown`, `AttributeNotDefined`, `InteractionClassNotPublished`, and `InteractionClassNotDefined` while each C++ failure emits the matching `HLAreportException` service name; the declared-transport companion loads the independent consumer/provider FOM modules, resolves `UmbraTransportationFixture` consistently in both Java ambassadors, and preserves that handle through ordinary interaction/attribute callbacks, timestamped interaction/attribute callbacks, and timestamped regional interaction metadata; the order-control companion proves the Restaurant FOM timestamp default, prospective class-default changes, per-instance overrides, RECEIVE-versus-TIMESTAMP reflection metadata, and publisher-scoped RECEIVE interaction delivery through the exact Java overloads |
| Attribute ownership management | `queryAttributeOwnership`, ownership status, unconditional and negotiated divestiture, divestiture confirmation/cancellation, regular/if-available acquisition, acquisition cancellation, release-denied, and divestiture-if-wanted services | typed ownership callbacks for assumption, divestiture confirmation, acquisition/unavailability/release, cancellation, and ownership reports; encoded federate owner handles | Native C++ and fake/real-JVM Java adapter service mappings and typed callback surfaces covered; native and real-JVM two-member tests now prove negotiated acquisition ordering, confirmation-tag propagation, ownership transfer, release-denied state preservation, denial-tagged unavailable callbacks, the if-available/already-owned unavailable path, both regular and if-available federate-owned self-acquisition preconditions, overlapping pending-acquisition rejection, confirm-without-request and duplicate-divestiture typed errors, no-pending cancellation errors, unconditional release/reacquisition, pending-acquisition cancellation, and `AttributeNotOwned` preservation for release-denied, ordinary update/order/transport services, and all three divestiture forms after transfer; the external IEEE-JAR route additionally proves `NO_ACTION` rejects a member-owned object with `FederateOwnsAttributes`, `UNCONDITIONALLY_DIVEST_ATTRIBUTES` emits a typed assumption callback and permits peer acquisition, a deferred assumption search resumes when a known candidate publishes later, and a pending acquisition rejects that action with `OwnershipAcquisitionPending` while `CANCEL_THEN_DELETE_THEN_DIVEST` completes the resignation path; mixed external `If Available` and regular acquisition requests now split one C++ plan into recipient-local secured/unavailable and acquirer/owner release callbacks for unowned versus remote-owned attributes, preserving copied tags and handles; the same external vector now queries both states in one request and proves separate standard owner-report and not-owned callbacks; all remaining ownership request/control failures (`queryAttributeOwnership`, `isAttributeOwnedByFederate`, negotiated/unconditional divestiture, confirmation/cancellation, regular/if-available acquisition, release-denied, divestiture-if-wanted, and acquisition cancellation) now preserve typed C++ exceptions and emit standard `HLAreportException` interactions through JNI and JPype; the timestamped-update ownership-transfer vector proves the official ownership transition does not rewrite or suppress an already accepted TSO payload before its grant; the external If Available order-reset vector proves an old owner's per-instance `TIMESTAMP` override is cleared at transfer, with the acquiring Java member returning an invalid retraction while its timestamped update callback carries `RECEIVE` sent/received metadata |
| Advisory/reporting support switches | object-class, attribute, scope, and interaction relevance switches; automatic-resign directive; service/exception reporting switches; provider support-switch queries | typed `ResignAction` plus boolean state with provider-defined initial values | Native C++ and fake/real-JVM Java adapter mappings covered; the external IEEE-JAR JNI vector proves C++ service-report JSON records after Java-routed time-regulation services, direct and regional `Send Interaction` standard `HLAreportServiceInvocation` payload/callback paths, timestamped `DeleteObjectInstance` sender-report payload and ordering before `removeObjectInstance`, C++ `HLAreportException` callbacks for typed `InteractionClassNotPublished` failures from direct, timestamped, directed, non-timestamped region-context, and timestamped region-context `Send Interaction`, `AttributeNotDefined` from `publishObjectClassAttributes`, `ObjectClassNotPublished` from both `registerObjectInstance` overloads, `ObjectInstanceNotKnown` from both untimed and timestamped `updateAttributeValues` overloads and receive-order, timestamped, and local `deleteObjectInstance` overloads, both declaration-management report-service conflicts, duplicate `enableTimeRegulation` and `enableTimeConstrained` conflicts, both duplicate/initial-state `enableAsynchronousDelivery` and `disableAsynchronousDelivery` conflicts, initial-state `disableTimeRegulation`/`disableTimeConstrained` conflicts, unregulated `queryLookahead`/`modifyLookahead` conflicts, and pending `timeAdvanceRequest`/`timeAdvanceRequestAvailable`/`nextMessageRequest`/`nextMessageRequestAvailable` conflicts; `subscribeInteractionClass` preserves `FederateServiceInvocationsAreBeingReportedViaMOM`, `setServiceReportingSwitch(true)` preserves `ReportServiceInvocationsAreSubscribed`, and the time-state methods preserve `TimeRegulationAlreadyEnabled`/`TimeConstrainedAlreadyEnabled`/`AsynchronousDeliveryAlreadyEnabled`/`AsynchronousDeliveryAlreadyDisabled`/`TimeRegulationIsNotEnabled`/`TimeConstrainedIsNotEnabled`/`InTimeAdvancingState`; each delivers a separate exception report; the private file vector additionally decodes the exact type-44 `AutomaticResignDirective` JSON record through C++ → JNI → standard Java → JPype; standard report families are covered, with broader DDM and state-space matrices remaining |

## Delivery order

The work proceeds by a dependency-first vertical slice, not by copying all
206 declarations into a Python abstract base class.

## JNI Java-provider completion path

The mock Java RTI is a signature, marshalling, and vendor-adapter test fixture;
it is not an Umbra implementation target. Umbra JNI work proceeds directly
against the C++ RTI in this order:

1. Connection, callback dispatch, and federation reporting — complete.
2. Federation lifecycle and membership with portable handle conversion —
   complete for create/destroy, all current exact-standard FOM/MIM `String[]`/`String` overloads, named and
   unnamed join, resign, membership reporting, `FederateHandle` bytes, and
   duplicate/missing/invalid lifecycle exception mappings, including the
   same-ambassador `FederateAlreadyExecutionMember` path.
3. Native-backed handle/set/map and encoder factories. The JNI façade now
   exposes native `FederateHandle` decoding and set construction for explicit
   synchronization sets, plus a native-only encoder factory with all twenty-six
   provider-owned C++ primitive elements bound. This includes signed and
   unsigned 16/32/64-bit and binary32/64 scalar values in both byte orders,
   byte/octet/pair values, canonical booleans, ASCII values, UTF-16BE Unicode
   values, and opaque data. The four standard composite forms are also
   C++-owned through JNI: variable/fixed arrays, fixed records, and variant
   records retain their prototypes, copies, alignment, and wire validation in
   C++, while Java and Python expose only the standard typed shells.
4. Service and callback families in the dependency order below, each with a
   direct Java smoke test and an opt-in JPype JNI integration test.
   The first composite encoder was native `HLAvariableArray`: its Java
   `DataElementFactory` selects a C++ primitive or native-composite prototype,
   while all element copies, count/padding, encode/decode, and returned child
   octets remain C++ owned. Native fixed arrays, fixed records, and variants
   now follow the
   same C++-owned pattern; recursively nested native composites are cloned at
   the JNI boundary and Python fixed-array/record setters update C++ directly.
5. A standalone Java API artifact, packaged native library, and compatibility
   policy before describing the façade as distributable.

An unbound Java method must raise `RTIinternalError`; a fixture implementation
or an inherited mock method never counts as JNI coverage.

1. **Federation-management edge vectors.** Expand save/restore and membership
   matrices with more negative/ordering cases while retaining the completed
   synchronization public-MOM, URL/handle-set, and lifecycle mappings.
2. **Shared handle and encoding depth.** Extend the already provider-backed
   typed handles, sets/maps, and C++/Java encoder boundary with additional
   composite and malformed-wire matrices; no Python reimplementation of
   encoding is allowed.
3. **Declaration and object management.** The foundational class/attribute/
   parameter lookup pairs, basic publish/subscribe, the receive-order object
   registration/discovery/deletion/update lifecycle, and receive-order
   interaction send/receive are complete. Timestamped services and the
   provider-backed DDM-region forms now cross both adapters; remaining work is
   broader callback ordering and state-space expansion rather than unbound
   standard services.
4. **Time management.** Bind logical-time factory use, provider-owned arithmetic, time advance requests,
   and grant callbacks as one lifecycle. Time values stay provider-created;
   Python does not invent a competing clock implementation. Integer and
   floating-time factory boundaries plus the minimal timestamped-service
   lifecycles are complete; standard integer/floating arithmetic is covered,
   while broader provider-specific arithmetic edge cases remain.
5. **Ownership and data distribution management.** The provider-backed region
   substrate, regional interaction declaration/send, regional object
   registration/update pair-vector lifecycle, class/instance attribute-value
   update requests, directed-interaction declaration/send, order/transportation services, and the complete attribute
   ownership service surface and relevance/reporting support-switch families
   now cross both adapters. Continue with callback sequencing/edge cases and
   broader DDM state-space combinations.
6. **Save/restore and support services.** Scalar and timestamped-save state
   callbacks, status queries, advisory switches, and name/handle support
   operations are complete; broader time-based save/restore sequencing remains
   follow-on work.
7. **Completeness pass.** Implement remaining overloads, all callback
   families, factory/encoder contracts, packaging matrix, and edition-specific
   conformance reports. The shared RTIambassador factory audit finds no
   unimplemented Java-declared methods; the external 2025 EncoderFactory now
   binds standard handle and logical-time carriers through provider-scoped
   Python façade methods and C++-validated factories, including the Java
    extendable-variant creator. It remains a deliberate edition-specific
   extension boundary in the shared Python contract because the native
   provider has no matching standard factory method.

### Current state-space and packaging tranche

The provider-neutral `umbra_rti_test_support.surface_matrix` module now
defines a deterministic 120-point matrix across callback model, integer versus
floating logical time, scalar versus timestamped save, all five standard
advance-service forms (`timeAdvanceRequest`, `timeAdvanceRequestAvailable`,
`nextMessageRequest`, `nextMessageRequestAvailable`, and
`flushQueueRequest`), and one/two/three independently bound federate
ambassadors. The 2025
and 2010 Java adapters execute the complete matrix through their normal
Python-to-Java forwarding paths (including both 2010 logical-time families)
and assert the required
`timeRegulationEnabled → timeConstrainedEnabled → timeAdvanceGrant` partial
order. This is binding evidence; it deliberately does not claim that the
lightweight fakes implement federation-wide scheduling.

The companion `iter_callback_provenance_matrix()` contains five stable
semantic callback envelopes.  The fake 2010 and 2025 provider tests consume
the same discovery, reflection, and interaction vectors and normalize their
edition-specific Java layouts before checking payload bytes, tags, producer
handles, optional regions, logical-time implementation/value, order metadata,
and retraction handles.  The 2010 and 2025 JNI lanes additionally construct
the carriers through the C++-backed Java factories before passing them through
the Python callback boundary. This closes a cross-route callback transport gap
without turning the matrix into a claim about callback scheduling.

`CallbackDeliveryObservation` and `normalize_callback_delivery()` provide a
small differential layer above those vectors.  The edition-specific fixture
tests feed two independently proxied recipients, retain a strict sequence per
recipient, and compare semantic handles/payload/provenance after provider
decoding.  Cross-recipient interleaving is intentionally not compared because
the provider owns that scheduling choice; duplicate or missing sequence
numbers remain binding failures.  The catalog records this as a ten-observation
normalization matrix (five vectors × two recipients), keeping the check
transplantable without treating private handle bytes as normative.

The opt-in `test_jpype_2010_mock_integration.py` lane adds a real-JVM
stateful check for the 2010 route: it builds the repository fixture, discovers
the named provider through the standard `ServiceLoader`, and fans object and
interaction traffic from one publisher to two or three members under both
callback models. The fixture is intentionally small; this lane verifies Java
carrier/callback conversion, URL adaptation, and event ordering rather than
claiming complete RTI scheduling or vendor behavior. Enable it with
`UMBRA_ENABLE_JPYPE_2010_MOCK_INTEGRATION=1`; it remains outside the default
gate because it requires a local JDK/JVM.

The companion `umbra_rti_test_support.wire_matrix` module supplies the same
fixed-width logical-time vectors to both edition adapters. It covers offset
decode, signed zero, values above the IEEE-754 exact-integer range, finite
final boundaries, negative/non-finite rejection, truncation/trailing-octet
failures, and provider-owned add/subtract/distance arithmetic. These vectors
are carrier evidence only; vendor-specific extensions remain explicitly
deferred when no standard Java/C++ type can represent them.

`compliance/catalogs/python-surface-completeness-catalog.json` is the
transplantable map from each edition and adapter route to its package metadata,
required tests, and Requirements Lab references. Its `matrix_evidence` section
records the shared support modules, deterministic vector counts, exact
`path#test_method` links used by each route, and the normative Requirements Lab
contracts that the matrix exercises. Run
`tools/verify_python_surface_report.py` (also included in the Python integrity
gate) to validate the namespace constants, entry-point groups, JNI
ServiceLoader descriptor, required test paths, matrix anchors, matrix
requirement references, and explicit status vocabulary; the JSON output carries
those counts and references for CI and downstream transplant tooling.
The report keeps direct-native routes `bounded` and vendor-specific floating or
non-time malformed matrices `deferred`; neither status is silently promoted to
implementation conformance.

The representable provider-extension boundary is tracked separately in
`provider-extension-wire-boundaries`. The shared
`provider_extension_matrix.py` vectors drive the native 2025
`HLAextendableVariantRecord` test (known alternatives, unknown future and
empty alternatives, discriminator-padding corruption, malformed lengths,
truncation, zero/overlong mapped values, signed and positive length extremes,
and trailing bytes). This is matrix-covered evidence for that explicit
extension only; it does not turn the extension into a provider-neutral factory
or imply coverage for vendor-defined arithmetic that has no standard carrier.
The support module also records seven provider-owned time/interval operation
shapes and four fixed-payload unknown-`DataElement` probes. The latter also
has a four-case destination-window matrix (exact offset, exact origin,
truncated, and empty) that checks typed encoder failures without partial
cursor/buffer mutation. The former checks
that a custom Java time carrier receives `add`, `subtract`, `distance`, and
`compareTo` without Python arithmetic substitution. The 2025 Java-shaped
provider test now wraps a provider-defined `LogicalTimeFactory` as an opaque
Python carrier and exercises all seven operation shapes, including deliberately
nonstandard arithmetic results; this proves delegation without claiming a
portable vendor domain. The latter checks generic carrier/cursor preservation
and provider-selected malformed rejection through both the 2010 and 2025 JNI
lanes. Dedicated 12-octet fixtures also prove that unknown 2010 and 2025
factories own variable-width decode and malformed-wire decisions; the Python
edge does not truncate them to the eight-octet reference shape. Both are
explicitly shape/transport evidence, not portable IEEE arithmetic or wire
semantics.
The arithmetic edge also records the edition distinction: the 2025 opaque
wrapper translates a provider `IllegalTimeArithmetic` into the Python
exception, while the 2010 adapter intentionally leaves an unknown carrier and
its Java exception untouched. Wide-carrier Python encodes use non-zero offsets
and sentinels in both editions, so the final façade step cannot overwrite
caller-owned bytes.
The companion 2025 encoder test sends the same opaque carrier through
`createHLAlogicalTime` and `createHLAlogicalTimeInterval`, checking selected
factory identity on set/get and preserving provider bytes.

The save/restore lifecycle boundary now has a separate shared
`save_restore_matrix.py`. Its 720 deterministic cases combine both callback
models, integer and float logical-time carriers, scalar and timestamped save
overloads, all five advance services, one/two-member membership, and every
complete/not-complete/abort outcome pair for save and restore. The 2025 and
2010 Java-shaped Python provider tests consume the same cases and assert the
exact standard service names forwarded to the provider. These are binding and
surface tests; they do not claim federation-wide persistence semantics for a
provider that does not implement them.

The opt-in live-JVM companion extends that evidence across the actual callback
proxy: the 2025 fixture runs complete, not-complete, and abort save outcomes,
then replays all three restore outcomes for both callback models, integer and
floating reference times, scalar and timestamped requests, and one-, two-, and
three-member federations. The 2010 fixture runs the same outcome callbacks for
the scalar 1516e route with one or two members. These lanes remain fixture
state-machine evidence; timed 2010 persistence semantics and vendor-specific
snapshot behavior stay outside the claim.

## Encoder boundary rule

The edition-specific Java API exposes `hla.rti1516_2025.encoding.EncoderFactory`.
The corresponding C++ API instead exposes concrete `DataElement` classes
directly and has no C++ encoder-factory interface. The shared Python
`EncoderFactory` will therefore be a deliberately small provider façade:
the Java provider delegates to its real Java factory, while the native
provider constructs and invokes the real C++ data-element classes. It must
not implement encodings in Python. The current scalar slice follows the data
elements implemented by Umbra C++ and mirrored in the Java fixture:
`HLAinteger16BE`, `HLAinteger32BE`, `HLAinteger32LE`, `HLAfloat64BE`,
`HLAfloat64LE`, `HLAunsignedInteger32BE`, `HLAboolean`, and
`HLAunicodeString`, with exact
encode/decode conformance vectors from the C++ and Java suites. Further endian
 nested variable-array/composite-discriminant matrices now cross both providers;
 the C++ `HLAextendableVariantRecord` is implemented as a tested native factory
 extension, while the standard Java handle and logical-time `EncoderFactory`
 carriers now delegate through provider-scoped Python façade methods to the
 selected C++ factories. The Java extendable carrier registers its C++ mapping
 on first `setVariant`; it remains a provider-specific extension rather than
 part of the provider-neutral Python factory. The exact Java creator audit now
finds all 43 standard 2025 creator signatures on the Java façade, while the
2010 JNI carrier lane reflects all 24 legacy creator names and overload
 arities at both the Java and Python boundaries. The 2010 carrier probe also
 forwards fixed-array factory/varargs, variable-array varargs, fixed-record
 additions, and variant discriminant/value arguments, checking child-carrier
 shape without introducing a Python-side composite wire codec.
 Native
and Java provider tests also audit that every abstract method in the shared
`RTIambassador` contract is declared by both provider classes.

## Portable handle boundary rule

Every public handle is an immutable, type-distinct value containing its
provider-produced encoded bytes. The bytes are portable across the Python
adapter boundary, but a caller must give a handle back to the RTI that created
it; the RTI remains responsible for validating federation scope. For the
native provider, Umbra's private C++ handle codecs reconstruct the concrete
standard handle just before the real C++ support service is called. For a Java
provider, the adapter obtains the standard `get*HandleFactory()` from the
selected Java `RTIambassador` and calls `decode(byte[], 0)` before the matching
Java service. Neither path invents a handle representation in Python.

## Per-slice checklist

Each pull-sized slice must include:

- a mapping table from official Java signatures and C++ overloads to the one
  Python signature (or explicitly documented Python overload adaptation);
- ownership/lifetime rules for every returned value and callback payload;
- C++ exception-name and Java exception-name translation tests;
- immediate and evoked callback delivery tests where callbacks are involved;
- native wheel integration against Umbra's C++ implementation; and
- Java fake-runtime coverage, followed by real-JVM fixture coverage when the
  fixture provides the service.

## JNI-backed C++ completion sequence

The JNI provider is a service-by-service execution route, not a second RTI
implementation. Work therefore proceeds in this dependency order, and a row
is complete only when its real C++ call, Java surface, Python adapter, callback
conversion where applicable, and an opt-in C++ → JNI → Java → JPype assertion
all exist.

| Order | Workstream | Carrier/dependency boundary | Completion evidence |
| --- | --- | --- | --- |
| 1 | Receive-order object and interaction base | Typed handles; mutable standard set/map factories; copied byte arrays | Two-member discovery, reflection, interaction, reservation, deletion, attribute-request, transportation, and advisory vectors |
| 2 | Remaining non-time declaration/object variants | Default order/transportation variants, update-rate/advisory variants, local deletion, and object-class update requests | Positive delivery/change vector plus the standard exception cases for each service |
| 3 | Shared time/retraction substrate | `LogicalTime`, `LogicalTimeInterval`, `MessageRetractionHandle`, order type, and their Java decoder factories | C++-encoded values round-trip through Java and Python; timestamped callback metadata is typed and copied |
| 4 | Timestamped object and interaction operations | The shared time/retraction substrate | Send/update/delete operations return retraction handles and their timed callbacks preserve sent/received order |
| 5 | DDM region substrate | Typed dimension and region handles, ranges, region-set/pair-list factories | Region creation/bounds/commit and overlap-filtered multi-member delivery |
| 6 | Region-aware declaration, update, registration, and request operations | DDM region substrate | Active/passive subscriptions, association reprojection, and region-filtered callbacks |
| 7 | Ownership and object-state management | Existing object/attribute carriers plus ownership callback variants | Multi-member transfer, cancellation, divestiture, and callback exception vectors |
| 8 | Remaining support, advisory, and time-management families | All preceding carrier types | A service inventory audit with an end-to-end test or an explicit unsupported status for every standard operation |

### JNI public-surface proof gate

The current shared `RTIambassador` contract has 184 abstract public services.
`test_jpype_jni_integration.py` now audits that every one is represented in an
opt-in C++ → JNI → Java → JPype vector. A vector may invoke a service directly
or select it as a function value in a small service-family loop; in both cases
the test performs the real public call against the C++ RTI. The gate is not a
claim that every standard state-space permutation is exhausted. It prevents a
new shared service from being counted while it has no end-to-end JNI exercise.

The same gate compares those 184 names with the Java
`NativeRTIambassador` dispatch table. This catches a service that is mentioned
by a Python test but would still fall through to `RTIinternalError` in the JNI
handler; the one deliberate Python naming adaptation is timestamped directed
interaction, which maps to the standard Java `sendDirectedInteraction`
overload with its fifth argument.

The remaining Java `unavailable(...)` and C++ “unsupported” branches are
defensive guards for malformed overload arities, impossible internal enum
values, unknown provider-specific logical-time carriers, or data-element types
outside the C++ encoder catalog. They are not standard RTI service fallbacks:
the exact external IEEE-JAR route has a dispatch branch for every public
service and the runtime gate below executes all 184 services.

With `UMBRA_JNI_REQUIRE_RUNTIME_SERVICE_COVERAGE=1`, the external suite also
wraps the concrete JPype façade for accounting and executes all 184 services
at runtime. The current external run passed this runtime gate, including
services selected through
family loops and function references. It additionally proves RTI-owned
joined-federate MOM discovery, initial/requested reflection, and removal, plus
timestamped per-attribute update-rate reduction with reliable delivery and
retraction consumption, and the two-subscriber update-rate matrix proves
default-rate delivery remains independent from a gated `Low` stream while an
unsubscribe/resubscribe projection generation admits a fresh best-effort
passel, through the independent IEEE Java API.

The current-source release was re-verified on 2026-08-22 with the exact
`hla-4-api-2.1.0.jar`: the artifact verifier passed its API/bridge/native
SHA-256 and no-API-shadowing checks plus the Java smoke test, and the full
external JPype suite passed **424 tests** (`Ran 424 tests ... OK`), including
the regional `HLAreportFederateLost` transport/DDM, timestamped-interaction,
timestamped-deletion, timestamped-update cutoff, pending ownership acquisition
cancellation, negotiated ownership-transfer cancellation, combined
cancel/delete/divest, final-member transport-loss, and service-report ordering
vectors added in this tranche, including discovery, receive-order removal, and
timestamped removal, save-status response, positive/negative restore callback
order, both accepted time-advance overloads, both accepted next-message
overloads, provider attribute-update callback ordering, Auto Provide's
empty-tag callback ordering, Flush Queue grant ordering, synchronization
registration/announcement ordering, Federation Synchronized completion
ordering, Query Attribute Ownership callback ordering, unconditional
divestiture assumption ordering, If Available acquisition ordering, regular
acquisition release ordering, release-denied unavailable ordering, Confirm
Divestiture acquisition ordering, and negotiated-divestiture confirmation
ordering, cancel-negotiated-divestiture release ordering, acquisition
cancellation ordering, interaction transportation confirmation ordering,
attribute transportation confirmation ordering, receive-order interaction
delivery ordering, receive-order directed-interaction ordering, timestamped
directed-interaction MOM ordering, receive-order attribute reflection ordering,
timestamped attribute-reflection MOM ordering, and retraction callback ordering.
The timestamped object-deletion re-enable vector also preserves one queued
`RemoveObjectInstance` callback across a constrained member's disable/re-enable
transition, including `TIMESTAMP` metadata, callback-before-grant ordering, and
the terminal C++ retraction boundary.
The delivered-deletion retraction vector now mirrors the C++ reconstitution
case: an immediate Java recipient receives the timestamped removal, a
constrained recipient retains the queued delivery, `requestRetraction` arrives
after `retract`, the object name and split attribute ownership are restored for
all members, and the queued removal is suppressed.
The no-fanout timestamped interaction and attribute-update vectors also mirror
the C++ ledger cases: both services return valid standard retraction handles
without eligible recipients, immediate `retract` consumes each handle, and a
time advance expires the second handle without fabricating callbacks.
The regional no-overlap attribute-update vector extends the same ledger proof
through `registerObjectInstanceWithRegions`, regional subscription, and
disjoint `SodaFlavor` ranges, preserving valid retraction/expiry semantics with
zero reflection or retraction-request callbacks.
The no-recipient timestamped deletion vector now also proves the strict
lookahead boundary, invocation-time tombstone retention, and reconstitution of
the object name and owned attribute state after a legal retract, with no typed
removal or retraction-request callback.
The joined-owner deletion vector additionally proves that a former owner’s
delivered removal is not retracted after resignation, while still-joined
members restore the object and only the still-valid ownership state. The
region-template vector now exercises standard region-handle-factory decoding,
pending versus committed bounds across two dimensions, foreign-federate
ownership errors, and passive-subscription lifetime through the same
C++ → JNI → standard Java → JPype route.
The timed-save scheduler vector additionally proves that an ordinary TAR
member may be admitted at the inclusive save boundary before a pending FQR
member, while both initiation callbacks and their typed grants remain ordered
by the C++ state machine.
The paired next-message vector separately proves the inclusive NMR boundary
and strict NMRA boundary: an equal available grant leaves a timestamped save
pending, while the next strictly later grant initiates the save before its
standard Java time grant.
The dimension-hierarchy vector now also proves inherited available-dimension
sets and exact upper bounds, then joins consumer/provider FOM modules through
the standard Java additional-FOM overload and resolves the newly composed
dimension, object class, and interaction class from the existing C++ catalog.
The delay-subscription matrix now also mirrors ordinary and regional
timestamped interaction and attribute-update eligibility through the Java
standard surface, including current-subscription re-evaluation at each grant.
The MOM exception vector also covers malformed `HLArequestSubscriptions`
parameters, preserving the typed C++ `InteractionParameterNotDefined` and
the decoded standard `HLAreportMOMexception` through JNI and JPype.
The all-request matrix drives every standard federate- and federation-scoped
MOM request class with a cross-class parameter and verifies the same typed
exception/report mapping for each service.
The immediate callback companion independently proves that a C++
`SendInteraction` service report is delivered through the standard Java
`HLA_IMMEDIATE` callback model before the application interaction callback;
the receive-order companion also decodes the type-27 interaction designator,
type-40 parameter/value map, type-63 tag, and type-34 Null timestamp through
the Java `EncoderFactory`.
its timestamped reflection companion proves the same ordering for the native
`UpdateAttributeValues` report and immediate `reflectAttributeValues` path.
The receive-order reflection companion now decodes the type-37 object,
type-2 attribute/value map, type-63 tag, and type-34 Null timestamp through
the same external Java encoder.
The provider-side `ProvideAttributeValueUpdate` companion likewise decodes
the type-37 object, type-1 attribute set, and type-63 request tag before the
typed provider callback.
The retraction companion verifies that `Retract` is reported before the typed
`requestRetraction` callback and that the original message-retraction handle
survives the standard Java surface. The directed-interaction
companion verifies that `SendDirectedInteraction` is reported before the
typed `receiveDirectedInteraction` callback, preserving the target object,
parameter map, tag, transportation handle, and producing federate through the
standard Java surface; its public report decoder now checks type-27 interaction,
type-37 target, type-40 parameter map, type-63 tag, and type-34/type-31 timestamp
records. The timestamped directed-interaction companion verifies
that the standard `SendDirectedInteraction` MOM report is delivered before the
typed callback, preserving target, parameter map, tag, transportation,
producer, logical time, sent `TIMESTAMP`/received `RECEIVE` order, and
retraction handle. The timestamped attribute-reflection companion verifies
that the C++ `UpdateAttributeValues` MOM report is emitted before the typed
`reflectAttributeValues` callback, preserving the attribute map, tag,
transportation, producer, logical time, sent/received order metadata, and
retraction handle through the standard Java surface. It also decodes all four
standard `HLAsuppliedArguments` records (object designator, attribute/value
map, user tag, and logical time) and the typed `HLAreturnedArgument` retraction
record with the external Java `EncoderFactory`, so the native report is
validated as a complete MOM payload rather than by service name alone. The
timestamped deletion companion proves that `DeleteObjectInstance` itself is
reported through the standard Java `HLAreportServiceInvocation` path before
the timestamped `removeObjectInstance` callback, and decodes its three
supplied-argument records plus the typed message-retraction return record. The
attribute-request companion proves object and class `RequestAttributeValueUpdate`
reports before provider callbacks, decoding the standard object/class designator,
attribute-set, user-tag, and Null-return records through the external Java
`EncoderFactory`. The ownership-query companion proves
`QueryAttributeOwnership` reports through the ownership-group MOM path with
typed object/attribute arguments and a Null return before the requester
receives its ownership result.
Its external report-file assertion now decodes the exact type-37 object
designator and type-1 attribute-set records through JPype, including the
successful-void and Null-return fields. The acquisition-cancellation companion
likewise decodes the type-37/type-1 cancellation payload before the typed
confirmation callback.
The regular acquisition companion decodes the same type-37/type-1 payload
plus the Table 5 type-63 user tag before the owner's typed release request.
The If Available and Release Denied companions now decode their corresponding
type-37/type-1/type-63 records before the standard acquisition or unavailable
callbacks, including the release-denied attribute-set wording.
The negotiated-divestiture, cancellation, and confirmation companions now
decode their type-37/type-1 forms and type-63 tags where applicable before the
standard divestiture, release, and acquisition callbacks.
The attribute and interaction transportation-request companions likewise
decode the public reports' type-37/type-1/type-59 or type-27/type-59 argument
records and Null returns before the C++ confirmation callbacks.
The matching attribute and interaction transportation-query companions decode
the public reports' type-37/type-0 or type-15/type-27 argument records and
Null returns before the typed `reportAttributeTransportationType` or
`reportInteractionTransportationType` callbacks.
The order/default-transport companion decodes the public reports for attribute,
default-attribute, and interaction order changes plus default attribute
transportation, preserving service groups, Table 5 argument types, Null
returns, and serial progression.
The object-name reservation companion decodes the single-name String and
multiple-name StringSet reports before the corresponding typed reservation
callbacks, preserving the object-management group, Null returns, and serials.
Its service-report-file companion also decodes the same type-53 and type-54
supplied-argument records from the C++ JSON report stream, proves reporting
switch gating, and verifies rejected releases do not produce false-success
records.
The configuration-recovery companion also proves that an empty or
non-directory `serviceReportDirectory` raises the standard `RTIinternalError`
before Connect changes lifecycle state, after which ordinary Connect and
Disconnect remain usable.
The Commit Region Modifications companion decodes the type-43 Set of region
designators record through the external Java/JPype route, proves switch-gated
region construction does not leak reports, and verifies an invalid committed
handle leaves the file unchanged.
The regional association companion decodes the type-37 object-instance and
type-4 nested attribute/region pair records for both association directions,
preserves serial progression, and proves a deleted-region rejection leaves
the service-report file unchanged.
The regional subscription companion decodes type-36 object-class and type-4
nested pair records plus the passive/update-rate optional slots for active and
passive subscriptions, proves file-sink gating, and verifies unsubscription
serial progression and deleted-region rejection. With reporting re-enabled it
also decodes the C++ failure records for regional subscribe and unsubscribe,
including their `InvalidRegion` exception text and preserved pair-list
arguments.
The regional interaction-subscription companion independently decodes the
type-27/type-43/type-6 private records for active subscribe and unsubscribe,
including the inverse passive-indicator mapping and monotonic serials. Its
failure companion re-enables reporting after a gated deleted-region call and
decodes C++ `InvalidRegion` records for both regional interaction subscribe
and unsubscribe, preserving the region-set argument types and serial order.
The MOM companion delivers a failed regional subscribe record to a second Java
federate and decodes its type-27/type-43/type-6 arguments, service type, null
returned record, exception text, and serial through the typed JPype callback.
The automatic-provision companion now decodes the callback-time
`ProvideAttributeValueUpdate` file record end to end, preserving the type-37
object instance, type-1 attribute set, mandatory empty type-63 tag, Null
return, success indicator, and serial number before the Java callback. This
extends the explicit request-path proof to the RTI-invoked Auto Provide path.
The time-constrained vector decodes the accepted time-management report with
an empty supplied-argument array and Null return before the typed
`timeConstrainedEnabled` callback.
The Flush Queue callback vector now decodes the private file record's type-31
requested logical-time boundary, successful-void shape, and serial before the
Java `flushQueueGrant`; it keeps that supplied boundary distinct from the
C++-computed effective and optimistic grant times.
The matching TAR, TARA, NMR, and NMRA vectors decode their distinct service
names and type-31 supplied boundaries before the shared Java `timeAdvanceGrant`
callback, preserving the distinction between request input and C++ grant time.
The file-only TAR companion independently decodes the type-31 `Logical time`
argument and successful-void metadata from the private JSON sink before the
standard Java grant callback.
The time-regulation vector decodes the type-32 LogicalTimeInterval Lookahead
record and Null return before the typed `timeRegulationEnabled` callback.
Its file-only companion independently decodes the same type-32 Lookahead
record from the selected private report sink, proving the C++ record survives
the JNI/Java boundary without changing the public interaction route.
The matching file-only time-constrained companion decodes the empty supplied
argument array, successful-void return, and serial metadata from the private
sink before `timeConstrainedEnabled` is delivered.
The asynchronous-delivery file companion similarly decodes paired empty
supplied-argument `EnableAsynchronousDelivery` and `DisableAsynchronousDelivery`
records with shared serial progression.
The paired time-role vector decodes accepted `QueryLogicalTime`, `QueryGALT`,
`QueryLITS`, `QueryLookahead`, `ModifyLookahead`, all five accepted
time-advance request forms (`TimeAdvanceRequest`, `TimeAdvanceRequestAvailable`,
`NextMessageRequest`, `NextMessageRequestAvailable`, and `FlushQueueRequest`),
`DisableTimeRegulation`, and `DisableTimeConstrained` reports through the same
public Java/JPype interaction path, preserving the type-31 logical-time return,
undefined GALT/LITS Null returns, type-32 returned/requested lookahead
arguments, the asynchronous delivery transitions, and serial progression around
the typed time-role callbacks. The same vector now decodes the support-service
group (type 6) for the object-class, attribute, scope, interaction, convey,
automatic-resign, and exception-reporting switch setters, including their
boolean or quoted resign-action supplied arguments.
The callback-gating companion now runs the same enable/query/disable lifecycle
through both `HLA_EVOKED` and `HLA_IMMEDIATE`, proving pending-request and
already-enabled exception mapping as well as the typed completion callbacks.
The federation-list companion exercises both list services through the same
two callback models, including missing-federation reporting and cancellation
of a queued report when the provider disconnects.
The time-advance companion proves that the selected logical time remains at its
initial value until `timeAdvanceGrant` in `HLA_EVOKED`, while the standard
`HLA_IMMEDIATE` route completes the same request synchronously; resignation
also discards a later queued grant.
The regional asynchronous-delivery companion repeats the overlap-qualified
receive-order gate with an immediate Java callback recipient, proving that
enabling and disabling the standard switch controls delivery without requiring
an explicit Python callback pump.
The temporal asynchronous-delivery companion mirrors the C++ gate under both
`HLA_EVOKED` and `HLA_IMMEDIATE`: a constrained receiver retains a
receive-order interaction while the switch is disabled, releases it when
asynchronous delivery is enabled, retains the next interaction after disable,
and releases that queued callback at the time-advance boundary without
manufacturing a grant.
Its file-only support-switch companion independently decodes the type-44
`AutomaticResignDirective` enum record (`NO_ACTION`) through the JNI Java
façade and JPype, preserving the C++ state update and successful-void shape.
The synchronization vector now decodes the federation-management group
(type 0) for `RegisterFederationSynchronizationPoint`, its
`ConfirmSynchronizationPointRegistration` result, and
`SynchronizationPointAchieved`; it preserves quoted labels, base-64 user tags,
type-34 Null optional slots, and serial order through the standard Java/JPype
interaction callback path before the corresponding registration, announcement,
and federation-synchronized callbacks.
Its private-file ordering companion independently parses the appended JSON
records, selects the `RegisterFederationSynchronizationPoint` and
`ConfirmSynchronizationPointRegistration` entries, and verifies the plain
label, base-64 tag, type-34 Null optional slots, successful-void return, and
integer serial metadata before the JPype registration callback.
The late-join announcement companion now parses the recipient's immutable
initial record plus the appended `AnnounceSynchronizationPoint` record,
validates its type-53 label/type-63 tag payload and successful-void metadata,
and proves the record was durable before the queued Java callback is evoked.
The Federation Synchronized callback vector also decodes the final type-53
label and type-18 failed-federate set from the recipient file before the
standard Java callback.
The achievement file companion separately selects the private sink and
decodes the type-53 label and type-6 success indicator for
`SynchronizationPointAchieved` after the registration/announcement barrier.
The same public federation-management route now covers both
`RequestFederationSave` overloads, preserving the quoted type-53 save label,
the type-34 Null or type-31 logical-time timestamp slot, and serial order
before save-initiation work is queued. The RTI-initiated
`InitiateFederateSave`, `FederationSaved`, and `FederationSaveStatusResponse`
notifications now use that same standard Java/JPype route before their
callbacks, preserving the type-53/type-34-or-type-31, type-6/type-34-or-
type-48, and type-21 payloads. The save-status callback companion additionally
decodes the adjacent
`QueryFederationSaveStatus` and `FederationSaveStatusResponse` file records,
including the type-17 joined-federate/status vector, before the Java callback.
Direct `FederateSaveBegun` and both
`FederateSaveComplete` selectors now use that public type-0 route as well;
the Java/JPype vector decodes the no-argument begun form and type-6 true/false
completion indicators before the final save result. `AbortFederationSave`
also emits its no-argument public record before the type-48 `SAVE_ABORTED`
result. Its private-file companion now decodes the same accepted successful-void
shape (`HLAsuppliedArguments=[]`, `HLAreturnedArgument=[null]`) through the
external IEEE Java API and JPype before `federationNotSaved`. The private save
completion companion likewise decodes `FederateSaveBegun`, the type-6 true
`FederateSaveComplete`, and the type-6 plus type-34-null `FederationSaved`
records before `federationSaved` is evoked. `RequestFederationRestore` now uses the
same public type-0 route with its quoted type-53 label before restore
acceptance, initiation, and completion callbacks. The positive and negative
restore-confirmation callback companions additionally decode the adjacent
type-53 label and type-6 success-indicator file records before their Java
callbacks. `FederateRestoreComplete` now
uses that same public type-0 route with its type-6 restore-success indicator
before the final restore-result callback. `QueryFederationRestoreStatus` and
`AbortFederationRestore` now use the same no-argument type-0 route before their
status and restore-aborted callbacks. The RTI-initiated restore notifications
`ConfirmFederationRestorationRequest`, `FederationRestoreBegun`,
`InitiateFederateRestore`, and `FederationRestoreStatusResponse` now use that
same standard Java/JPype route before their callbacks, preserving the exact
type-53/type-6, no-argument, type-53/type-15/type-53, and type-20 payloads.
The evoked restore-resignation companion adds the callback-queue boundary:
the surviving Java member receives restore acceptance, begin, initiation, and
`FEDERATE_RESIGNED_DURING_RESTORE` in order, while the resigning member's
queued restore callbacks are all discarded.
The private-file restore companion now decodes the same acceptance,
FederationRestoreBegun, InitiateFederateRestore, FederateRestoreComplete, and
no-restore-in-progress status records before the corresponding typed callbacks;
the AbortFederationRestore companion verifies its empty successful-void record
before `RESTORE_ABORTED`.
Ordinary `ResignFederationExecution` now uses the same final public route,
preserving its type-44 `HLAresignAction` payload separately from the
RTI-originated `FederateResigned` service.
The RTI-initiated `FederateResigned` path now reserves its final public
type-0 interaction before membership removal and delivers the type-53 reason
through the external Java/JPype observer before `federateResigned`; the private
file vectors now decode the exact type-44 `ResignFederationExecution`, type-53
`FederateResigned`, and type-53 `ConnectionLost` records before their lifecycle
callbacks. The lost transport `ConnectionLost` path remains file/callback-only
because its Java endpoint is intentionally torn down.
The receive-order interaction companion verifies that `SendInteraction` is
reported before the typed `receiveInteraction` callback,
preserving the parameter map, tag, transportation handle, and producing
federate through the standard Java surface.
The regional receive-order companion independently decodes the private
`SendInteractionWithRegions` file record, including the parameter/value map,
region set, tag, and Null timestamp, before the standard regional callback.
The reflection companion verifies that `UpdateAttributeValues` is reported before the typed
`reflectAttributeValues` callback, preserving the attribute map, tag, and
transportation handle through the standard Java surface. The attribute-transport
companion verifies that `RequestAttributeTransportationTypeChange` is reported
before the typed confirmation callback, preserving the object instance,
attribute set, and transportation handles through the standard Java surface.
Its private-file companion decodes the type-37 object, type-1 attribute-set,
and type-59 transportation arguments, while the matching interaction
transportation companion decodes the type-27 interaction and type-59
transportation arguments, with successful-void returns and serial metadata
before their Java callbacks.
The private report-file assertions now decode the exact direct-interaction
payload (`type-27` interaction class, `type-40` parameter/value map,
`type-63` tag, and `type-34` optional timestamp) and the directed-interaction
payload (the same fields plus the `type-37` target object handle) before the
callbacks, proving these maps survive the C++ → JNI → Java → JPype route.
The matching current-source C++ Catch2 executable also passed **626 test
cases** and **26,837 assertions** in the same verification window.

The companion structural gates assert that every Java fixture
`RTIambassador` name/arity has a Java proxy dispatch branch, each declared JNI
endpoint is registered by the native library, and all 62 C++
`FederateAmbassador` callback overloads have matching Java declarations, JNI
upcalls, Python marshalling, and global-reference cleanup. The IEEE C++
inventory reports 63 pure-virtual members for `FederateAmbassador` because it
also includes that class's pure virtual destructor; it is not a 63rd callback.
The same guard requires the Java ambassador façade to retain only its native
handle, preventing a second Java-side federation state model.

The JNI build also accepts `-JavaApiJar <path>`. When supplied, that JAR is
the sole compile and runtime API dependency for the bridge; no mock API output
is created or bundled. The normal Java smoke test then discovers Umbra through
`ServiceLoader` on that consumer-style class path. This keeps the legal and
versioned IEEE Java API artifact outside Umbra until its source, redistribution
terms, and digest are explicitly recorded. The checked-in fixture remains an
integration declaration set, not a replacement for that release dependency.
The JNI conformance configuration calls the standard Java
`RtiFactoryFactory` named-factory overload with Umbra's published provider
name, so an unrelated provider descriptor cannot win when an application
classpath contains several providers. The generic JPype provider still leaves
the name unset when default `ServiceLoader` selection is desired; both routes
use the standard Java factory surface rather than a Python-side provider
shortcut.
The companion `packages/umbra-rti-jni/package.ps1` stages the bridge JAR and
native library into a release directory, reruns the verifier, and writes both
the artifact manifest and an explicit Java API dependency manifest. With
`-IncludeJavaApiJar`, the supplied standard API JAR is copied beside the bridge
so `UmbraJniRtiFactory(artifact_directory=...)` can discover it without a separate
path argument; without that switch, the API remains an external dependency.
This closes artifact-directory onboarding while leaving Maven/Gradle
publication and redistribution approval to the release owner.

## Next concrete slice

The JNI federation-management slice now executes the safe non-time-based
overload family against C++: vector FOM modules, explicit MIM modules,
additional join FOMs, resign, and explicit synchronization federate sets with
registration, announcement, and completion callbacks. It also binds
`queryFederationSaveStatus`, including the C++ vector → Java
`FederateHandleSaveStatusPair[]` → immutable Python record conversion, and a
scalar save success, federate-reported-failure, and abort lifecycle with typed
`SaveFailureReason`. It also binds the complete scalar restore-control
lifecycle: `queryFederationRestoreStatus`, request acceptance/rejection,
federation-begun, typed per-federate restore initiation, complete,
not-complete, and abort. C++ `RestoreStatus`, `RestoreFailureReason`, and both
handle fields are constructed as standard Java values and reach immutable
Python records/enums. The opt-in C++ → JNI → Java → JPype integration test
proves success, federate-reported failure, abort, and a missing-save rejection.
It additionally proves C++ federate name ↔ handle lookup and the standard
`long` normalization coordinate, alongside C++ object-class lookup/name/
normalization and object-class → attribute lookup/name calls. Object-class and
attribute values use distinct Java encoded-handle classes and factories, so
Python values re-enter C++ through their correct standard handle domain;
attribute normalization is not an IEEE 1516.1 service. Interaction-class
lookup/name/normalization and its distinct typed factory now cross the same
native path, as do parameter lookup/name and its distinct typed factory. The
Java `ParameterHandleValueMap` factory now feeds C++ receive-order interaction
publish/subscribe/send services, and a two-member JNI integration test proves
the C++ callback returns the typed interaction/parameter/transportation/
federate values and copied octets to Python. The matching JNI
object-management slice now carries native `AttributeHandleSet` and
`AttributeHandleValueMap` factory values, distinct native
`ObjectInstanceHandle` encoding/decoding, declaration publish/subscribe
teardown, provider-named registration, object name/handle lookup,
normalization, and receive-order attribute updates. Its two-member integration
vector proves C++ discovery and reflection callbacks deliver typed
instance/class/attribute/transportation/federate values and copied bytes/tags
to Python. Single object-name reservation/release and both reservation
callbacks now cross the same route, and the integration vector proves a
reservation-consuming named registration and asynchronous availability
rejection. Batch reservation/release and its set-valued callbacks now cross
the same route as standard Java `Set<String>` values. Receive-order deletion
and the corresponding removal callback now preserve typed object/federate
values, copied tags, and the standard `ObjectInstanceNotKnown` exception.
Both standard receive-order attribute-value request overloads now reach C++,
and the C++ `provideAttributeValueUpdate` callback returns typed attribute
sets and copied tags. The request vector first proves the public MOM route for
the object-instance and object-class overloads, then selects the alternative
private sink and decodes both exact type-37/type-36 records with shared serial
progression. Attribute and interaction transportation query services
now emit complete public `HLAreportServiceInvocation` payloads before their
typed C++ report callbacks, and their change-request services return typed
confirmations through the same report boundary. Time- and region-based object
variants now cross the JNI
route as C++-owned timestamped and regional carriers; attribute scope advisory
switching and its in-scope/out-of-scope callbacks are also native. The
interaction slice also proves declaration teardown plus typed
C++ `HLAreliable` transportation lookup/name/factory parity with the callback
payload.
Order-type lookup/name and the non-time attribute/default-attribute/
interaction order changes now use the C++ `OrderType` values through the
standard Java enum. Update-rate queries, default attribute transportation, and
local object deletion are also native; the integration vector proves a named
FOM update rate, copied numeric result, and per-federate local deletion.
The JNI route's first shared-time slice now decodes all five standard
advance/queue requests through the C++ time factory, reconstructs
`HLAinteger64Time` Java carriers for regular and flush-queue grants, and
delivers constrained-mode activation with the C++ current time. The same
end-to-end vector proves query state after each grant and native
asynchronous-delivery state toggles. It also carries provider-created integer
intervals for regulation and lookahead, and returns GALT/LITS as atomic C++
validity-plus-time snapshots; the single-member vector proves the standard
invalid result before a federation establishes a shared temporal bound. The
lookahead transition companion additionally proves the external Java
`modifyLookahead` route rejects calls before regulation, applies increases
immediately, defers decreases until the next grant, and rejects changes while
time advancing. The
direct GALT/LITS regulator vector now additionally proves the external
Java `TimeQueryReturn` shape tracks an active regulator's lookahead-adjusted
pending advance (2, then 7), remains stable after the grant, and returns to
the invalid/absent form when the only regulator disables regulation. Its
three-member companion proves the C++ minimum is selected across two active
regulators, remains at the right-hand candidate while the left advances, and
recomputes to the remaining left candidate after the minimum regulator
resigns. The constrained-TAR companion then proves an equal-boundary TAR
remains pending at GALT and is released by the regulator's pending advance
before the regulator's own grant callback. The
get-time-factory lifecycle companion additionally preserves `NotConnected`
before connection, `FederateNotExecutionMember` before join and after
resignation, and the selected C++ `HLAinteger64Time` factory's initial carrier
through the standard Java and JPype surfaces. The
first timestamped interaction/retraction vector issues a C++ retraction handle,
flushes the receiving constrained federate, and proves typed time/order/handle
metadata plus the later `requestRetraction` callback across JNI and JPype. The
same carrier path now proves timestamped attribute updates, object deletions,
and their typed reflection/removal lifecycles, each followed by the matching
`requestRetraction` callback.
The ordinary three-member retraction fan-out companion adds the missing queue
boundary: an unconstrained Java member receives the timestamped interaction and
its retraction callback, while a constrained member's queued copy is removed
before its grant; after that member resigns, the delivered member still receives
the next retraction through the same C++ ledger.
The JNI DDM foundation now carries opaque dimension and region handles through
the standard Java factories, including available-dimension queries, range
bounds, region commits, and deletion. Regional interaction subscriptions and
both receive-order and timestamped sends are C++-backed as well; their
overlapping region filter and the optional sent-region set are preserved in
the Python callback, together with timestamp, order, and retraction metadata.
The handle-factory companion now mirrors the C++ public decoder lifecycle at
the Python-visible standard boundary: JPype factory getters preserve
`NotConnected` and `FederateNotExecutionMember`, and all federate, object,
interaction, attribute, parameter, dimension, region, and message-retraction
identities round-trip through their encoded values. The
malformed encodings remain C++-validated and preserve typed `CouldNotDecode`
through JNI and JPype without inventing a service-level MOM report.
Regional object services now use the same C++ association semantics through a
typed Java `AttributeSetRegionSetPairList`: regional subscription,
registration, association/unassociation, and regional attribute-value request
all cross JNI. The two-member JNI vector proves overlapping DDM filtering,
regional discovery/reflection with its sent-region set, and the provider
callback carrying the original value-request tag.
The regional request/response companion now also decodes the private
`RequestAttributeValueUpdateWithRegions` record, preserving the type-36 object
class, nested type-4 attribute/region pair list, type-63 tag, successful-void
shape, and serial metadata before the standard Java provider callback.
The JNI advisory/support slice now also reaches the C++ ambassador for all
relevance/reporting switch reads and mutable switch updates, the automatic
resign directive, and `normalizeServiceGroup`. Standard Java
`ResignAction`/`ServiceGroup` enums are converted only at the Java façade;
Python observes the shared public enum types. The integration vector proves
mutable values round-trip through C++ and that every provider-defined
read-only support switch is callable through the same path. Its two-member
extension also decodes FDD-seeded defaults and proves mutable owner changes do
not leak into the peer's independently initialized support-switch state.
The normalization companion additionally mutates standard Java handle bytes,
preserves the four typed invalid-handle exceptions from C++, and decodes their
distinct service names from `HLAreportException` interactions through JNI and
JPype; valid `ServiceGroup` values continue to use the exact standard enum.
The support-switch exception companion holds a save open and invokes all 23
advisory, automatic-resign, reporting, file-reporting, auto-provide, delayed
subscription, known-class, relaxed-DDM, and non-regulated-grant services;
each typed `SaveInProgress` failure is decoded as a separate
`HLAreportException` through Java and JPype.
The JNI directed-interaction slice now accepts the standard Java
`Set<InteractionClassHandle>` declaration carrier, routes C++ receive-order
and timestamped directed sends, and calls the distinct Java
`receiveDirectedInteraction` overloads. The isolated two-member Python vector
proves target-class filtering, typed target/interaction callback values, TSO
metadata and retraction, and both set-specific and whole-class teardown
overloads.
The same vector now sends a malformed timestamped directed interaction handle,
preserving `InteractionClassNotPublished` and decoding a distinct C++
`Timestamped Send Directed Interaction` `HLAreportException` through Java and
JPype.
The JNI ownership slice now sends all eleven standard ownership services to
the C++ ambassador. Its callback target has every nine standard ownership
entry point, while ordinary instances emit owner reports, unowned reports,
assumption offers, negotiated divestiture requests, acquisition notifications,
release requests, unavailable reports, and acquisition-cancellation
confirmations. The isolated two-member Python vector proves every implemented
callback shape, binary tags, typed owner and attribute handles, the native
boolean ownership query, and the C++ out parameter reconstructed as a public
Python `AttributeHandleSet` by `attributeOwnershipDivestitureIfWanted`. The
same route now discovers RTI-owned joined-federate MOM objects and delivers
the explicit standard `attributeIsOwnedByRTI` callback without inventing a
federate handle; `isAttributeOwnedByFederate` returns false for those
attributes. The negative-path vector also proves
confirm-without-request (`AttributeDivestitureWasNotRequested`) and duplicate
negotiated divestiture (`AttributeAlreadyBeingDivested`) across the exact
Java API. A companion real-JVM negative-path vector now proves
`FederateNotExecutionMember`, `ObjectInstanceNotKnown`, and
`AttributeNotDefined` survive the C++ → JNI → Java → JPype route with
standards-valid handle encodings.
The JNI callback-target audit now also closes the declaration-management
advisory boundary: every 56 callback names in the IEEE C++ header, Java
fixture, and JNI target agree. The advisory vector proves C++ start/stop
registration and interactions-on/off notifications as typed Python class
handles, together with updates-on/off relevance transitions. It invokes both
standard `turnUpdatesOnForObjectInstance` overloads, preserving the optional
named update-rate designator through JNI, Java, and JPype. The final
`synchronizationPointRegistrationFailed` callback now has its standard Java
enum and is exercised by a duplicate-label C++ failure reaching Python as
`SynchronizationPointFailureReason.SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE`.
Exception translation is now audited against the 109 concrete exception names
in the IEEE 1516.1-2025 C++ `Exception.h` header. The shared Python registry
contains that exact set, and the Java test fixture supplies runtime classes
for all of them so JNI can retain the original C++ type rather than falling
back to `RTIinternalError`. The JNI vector loads every Java exception type and
proves `NameNotFound` and `InvalidObjectClassHandle` from real C++ service
preconditions through the Java proxy's checked-exception wrapper into their
specific public Python exception classes.
The first encoder slice exposes twenty-six provider-owned C++ elements that Umbra implements,
including the standard UTF-16 element-count rule for `HLAunicodeString`.
The JNI route now binds every one of those twenty-six C++ primitive data
elements: `HLAinteger*`, `HLAunsignedInteger*`, `HLAfloat*`, `HLAbyte`,
`HLAoctet`, `HLAoctetPair*`, `HLAboolean`, ASCII character/string, single-code-
unit UTF-16BE Unicode character/string, and `HLAopaqueData`. End-to-end Python
vectors exercise their exact C++ byte layouts, count prefixes, and variable-
length decode paths. Its unsigned Java `byte`/`short`/`int`/`long` carriers
preserve raw C++ unsigned bits, including all-one values, rather than
implementing conversion rules in Java or Python. Composite type mismatches
also remain C++-owned: the Python adapter delegates them to the standard Java
method and preserves `EncoderException` rather than reimplementing the
prototype check. C++ `EncoderException` travels through Java's
encoding package as `EncoderException` for encode paths and `DecoderException`
for decode paths, then reaches the matching Python encoding exception.
Portable typed handles now cover federates, the standard
support-service name/handle pairs, and object instances; support value lookups
cover update rates, order types, available dimensions, dimension bounds, and
service/handle normalization coordinates.
Basic declaration
management and the receive-order object-instance reservation, registration,
discovery, lookup, deletion, and attribute-update/reflect lifecycle are
covered across both providers, as are receive-order interaction send/receive
with typed parameter maps. The time-management slice now covers provider
logical-time factory values, integer time-advance lifecycle, encode/decode,
regulation/constrained mode, lookahead query/modification,
asynchronous-delivery switches, all five time-advance/queue request variants,
Java-shaped GALT/LITS results, and grant callbacks across native C++ and Java
adapters. The duplicate-regulation companion now invokes the exact Java
`enableTimeRegulation` method twice, preserves the typed
`TimeRegulationAlreadyEnabled` failure, and decodes a separate
`HLAreportException` callback through JNI and JPype. The
The pending-regulation companion invokes standard Java
`enableTimeRegulation` twice before the first callback, preserves
`RequestForTimeRegulationPending`, and decodes its separate exception report.
The
constrained-mode companion performs the same exact Java duplicate-call check,
preserving `TimeConstrainedAlreadyEnabled` and decoding its separate exception
report. The
The pending-constrained companion invokes standard Java
`enableTimeConstrained` twice before the first callback, preserves
`RequestForTimeConstrainedPending`, and decodes its separate exception report.
The asynchronous-delivery companion invokes the standard Java
`enableAsynchronousDelivery` method twice, preserves
`AsynchronousDeliveryAlreadyEnabled`, and decodes its separate exception
report. Its inverse-state companion invokes the standard Java
`disableAsynchronousDelivery` method while the switch is off, preserves
`AsynchronousDeliveryAlreadyDisabled`, and decodes its separate exception
report. The
The inverse time-role companions invoke `disableTimeRegulation` and
`disableTimeConstrained` while their modes are off, preserve
`TimeRegulationIsNotEnabled`/`TimeConstrainedIsNotEnabled`, and decode their
separate exception reports. The
The unregulated lookahead companions invoke the exact Java
`queryLookahead` and `modifyLookahead` services, preserve
`TimeRegulationIsNotEnabled`, and decode separate exception reports. The
The pending-lookahead companion invokes standard Java `modifyLookahead` while
a time advance is pending, preserves `InTimeAdvancingState`, and decodes its
separate exception report. The
The pending-request companion invokes standard Java `timeAdvanceRequest` twice
before the first grant callback, preserves `InTimeAdvancingState`, and decodes
the separate exception report. The
The regulation-pending companion invokes standard Java `timeAdvanceRequest`
and `timeAdvanceRequestAvailable` while `enableTimeRegulation` awaits its
callback, preserves `RequestForTimeRegulationPending`, and decodes separate
reports retaining each overload's service label.
The
The constrained-pending companion invokes standard Java `timeAdvanceRequest`
and `timeAdvanceRequestAvailable` while `enableTimeConstrained` awaits its
callback, preserves `RequestForTimeConstrainedPending`, and decodes separate
reports retaining each overload's service label.
The
The overload companion invokes standard Java `timeAdvanceRequestAvailable`
twice before the first grant callback, preserves `InTimeAdvancingState`, and
decodes its separately named exception report. The
The next-message companion invokes standard Java `nextMessageRequest` twice
before the first grant callback, preserves `InTimeAdvancingState`, and decodes
its separate exception report. The
The overload companion invokes standard Java `nextMessageRequestAvailable`
twice before the first grant callback, preserves `InTimeAdvancingState`, and
decodes its separately named exception report. The
The flush-queue companion invokes standard Java `flushQueueRequest` twice
before the first grant callback, preserves `InTimeAdvancingState`, and decodes
its distinct `Flush Queue Request` exception report. The
attribute-value-update request slice now covers both class and instance
overloads plus the provider callback carrying copied tags and typed attribute
sets. Timestamped callback payloads now cross both native C++ and the real JVM
fixture, including logical time, order, and retraction metadata. Directed
interaction declaration/send now crosses native C++ and the fake/real-JVM Java
adapter, including typed target-object callbacks and timestamped Java delivery.
The public, native pybind, and Java callback surfaces now have parity with all
56 unique callback names declared by the 1516.1 C++ header, including
`federateResigned`, `flushQueueGrant`, and `requestRetraction`; the header's 62
declarations still include overloads.
The Java-declared handle decoder and set/map factory accessors now return
provider-neutral Python builders across native C++ and JPype, while preserving
immutable callback snapshots and copied byte values at service boundaries.
The ownership sequencing slice now covers the first multi-member contention
path through both providers; all Java ownership callback conversion names have
proxy-level coverage, and order and transportation management crosses both
provider adapters. Save/restore now has a real-JVM two-member initiation,
completion-barrier, status, restore, per-member logical-time rewind, and
shared failure/abort test. Regional object subscriptions now also prove
passive declaration suppression and activation before discovery/delivery.
The delayed regional interaction and object-update DDM slice now crosses both
providers, including callback-boundary re-evaluation with committed range
changes. Native association coverage now proves multi-region retention,
independent per-attribute retention/disjoint filtering, selective unassociation,
default-region fallback, regional scope-advisory out/in transitions across
subscription removal and restoration, retained ordinary-versus-regional
subscription transitions, stale unsubscribe/resubscribe suppression, and
named-rate `turnUpdatesOnForObjectInstance` callback metadata and receive-order
recipient isolation
across two independently subscribed members. Both providers also
realize default-region object callbacks with an empty conveyed region set for
explicit regional subscribers in receive-order and timestamped paths, while
ordinary non-regional callbacks remain unmarked. Both providers agree on the
smallest-positive-subnormal floating epsilon and canonical signed zero, and
both exercise exact-boundary relaxed DDM delivery. Provider-owned logical-time
add/subtract/difference now cross the native pybind and Java adapter boundaries
with integer and floating parity tests, including implementation mismatch,
smallest-subnormal, largest-finite, and overflow/underflow exception mapping.
The external IEEE Java-API/JNI vector now adds a three-federate regional
interaction fanout: overlapping subscriptions receive the conveyed source
region while a disjoint subscription is filtered by the C++ DDM engine. A
second external vector keeps two equal-range subscriptions active through an
unsubscribe of the first, then verifies delivery stops only after the second
overlap is removed; this exercises the official Java region-set carrier and
the C++ subscription reprojection state through JNI.
The asynchronous-delivery companion now drives an overlap-qualified regional
receive-order send through the standard Java `enableAsynchronousDelivery` and
`disableAsynchronousDelivery` services, proving the callback remains gated
while disabled and is released after re-enable with the original producer,
tag, and conveyed source `RegionHandleSet`.
The ordinary regional-TAR/NMR companion adds the non-available request family:
two constrained Java recipients receive the same source-region TSO at 7,
each interaction precedes its own TAR or NMR grant, and both retain the
conveyed source region, timestamp/order metadata, and retraction handle.
The Flush Queue context companion then runs both derived default-region and
explicit-source regional interactions through the standard Java
`flushQueueRequest`, proving interaction-before-flush-grant ordering, actual
grant 5 versus optimistic grant 7, and preservation of empty versus explicit
source-region callback metadata under both standard `HLA_EVOKED` and
`HLA_IMMEDIATE` callback models.
The default-region interaction companion also proves that an explicit,
disjoint regional declaration overrides a retained ordinary declaration for
explicit source regions, that removing it restores ordinary default-region
effectiveness, and that an ordinary send conveys the supplied-empty region set
to both eligible Java callbacks.
Its available-advance companion drives the same default-region TSO through
`timeAdvanceRequestAvailable` and `nextMessageRequestAvailable`, proving each
interaction callback precedes its grant at 7 and 9 while preserving the
standard empty `RegionHandleSet`, timestamp/order metadata, and live
retraction handle.
It also proves that the official Java region carriers deliver boundary-touching
regional interactions and object updates when the federation's relaxed-DDM
switch is enabled, while the strict-profile companion rejects those exact
touching ranges and resumes delivery only after a real overlap is committed.
The strict vector covers both interaction callbacks and object discovery/
reflection through the same Java region carriers. Delayed regional interaction
and object-update delivery also re-evaluate the committed range at callback
time. A timestamped
regional-association vector also suppresses a queued update after source-region
unassociation and delivers a later update after reassociation with typed TSO
metadata.
The directed timestamped-retraction vector also proves that only recipients
that received a message get its `requestRetraction` callback.
It also proves multi-region regional-object discovery reprojection without
duplicate discovery, selective unassociation while another overlap remains
active, and the final out-of-scope transition through the official Java
`AttributeSetRegionSetPairList` factory.
The external IEEE-JAR route additionally keeps two independently associated
object attributes separate: a disjoint attribute is filtered while its sibling
is delivered, association of the second attribute reprojections both values,
and selective source unassociation leaves the remaining attribute active
before the default-region fallback restores both.
The restore vectors also preserve a deferred lookahead decrease across restore
and apply it only after the post-restore time advance.
The external IEEE-JAR regional-object error vector now preserves
`RegionNotCreatedByThisFederate`, `InvalidRegion`, and `InvalidRegionContext`
for foreign, uncommitted, and wrong-dimension pair lists across registration,
subscription, and association services.
The regional attribute-value request vector additionally proves the empty-set
no-op, disjoint filtering, overlap admission, and the same typed errors through
`requestAttributeValueUpdateWithRegions`.
Its owner-side callback companion now enables the standard service-report
switches and proves the C++ `ProvideAttributeValueUpdate` type-37/type-1/
type-63 record is visible at the Java callback boundary, while the requester
record remains a separate type-36/type-4/type-63 vector.
The external IEEE-JAR interaction lifetime vector also maps
`RegionInUseForUpdateOrSubscription` while a regional subscription is active,
then proves standard unsubscription releases the region for deletion; an
independent observer also decodes the originating subscriber's `Delete Region`
`HLAreportException` through the exact Java encoder and JPype.
The same external regional-object route now covers a timestamped update after
the source association is removed: C++ emits the empty standard
`RegionHandleSet`, preserves sent `TIMESTAMP` versus received `RECEIVE` order,
and returns a valid retraction handle through JNI, Java, and JPype.
The external regional-object vector also passes empty-region pair lists through
the exact Java factory for subscription, association, and teardown, preserving
the C++ no-op semantics.
It also invokes the exact Java named-rate regional subscription overload and
decodes C++ `turnUpdatesOffForObjectInstance` and rate-bearing
`turnUpdatesOnForObjectInstance` callbacks as typed Python metadata while the
source region transitions out of and back into scope.
The ownership-transfer vector then proves C++ clears the former owner's
association: the new owner's first update carries the empty/default region
designator, while a later explicit association carries the new owner's region.
The mixed-declaration vector additionally proves that an ordinary subscription
retained beside a disjoint explicit regional declaration is suppressed while
the source is explicit, then reprojected through the derived default region
when that declaration is removed, with empty conveyed-region metadata preserved
for both recipients.
The passive-subscription vectors now independently prove that ordinary and
regional interaction declarations, as well as ordinary and regional object
declarations, remain non-delivering until an active replacement. Existing
objects are reprojected and subsequent updates or interactions cross the
standard Java callbacks only after activation.
The queued-association vector additionally proves that a timestamped update
accepted under source region A is not retargeted when the association is replaced
with region B before the callback boundary; only the later update carries B and
its exact standard retraction metadata.
The suppressed-timestamp vector also proves that a queued regional recipient
becoming disjoint before delivery consumes the recipient ledger: no reflection
or `requestRetraction` callback is emitted after the publisher retracts it.
The constrained-grant vector carries the same disjoint update through a receiver
time-advance grant, proves the grant is delivered without reflection, and maps
the terminal C++ `MessageCanNoLongerBeRetracted` boundary through Java and JPype.
The mixed-default-region vector fans one ordinary registration to three regional
subscribers using `flushQueueRequest`, `timeAdvanceRequestAvailable`, and
`nextMessageRequestAvailable`; all receive the C++ timestamped update before
their grant, with empty conveyed regions and one shared retraction handle.
The matching interaction vector fans one ordinary timestamped interaction to
three regional subscribers through those same FQR/TARA/NMRA request families;
each Java callback arrives before its grant with empty sent-region metadata,
`TIMESTAMP` send/receive order, and the same consumed retraction handle.
The external-JAR future-input vector additionally accepts `flushQueueRequest`
before the producer submits either payload, then queues timestamped interactions
at 9 and 12 before HLA_EVOKED dispatch. Both callbacks arrive in timestamp order
before the C++-computed FQG, preserving actual time 5, optimistic time 9,
`TIMESTAMP` order metadata, and each standard `MessageRetractionHandle`.
The companion mixed default-region retraction vector fans one ordinary source
interaction to an immediate regional recipient and a constrained regional
recipient. The immediate callback exposes an empty conveyed-region set and
`TIMESTAMP`/`RECEIVE` order, `retract` emits exactly one standard
`requestRetraction`, and the constrained member receives only its time-6 grant;
the queued callback and its retraction callback remain suppressed.
The ordinary-interaction retraction/resignation vector now mirrors the native
three-member case through the independent IEEE JAR: a timestamped interaction
is delivered immediately to one member, queued for a constrained member, and
retracted before that grant. The constrained callback remains absent, the
immediate member receives the typed retraction callback, and a later timestamped
send after constrained resignation still retains and retires its own ledger
entry independently.
The timestamped object-update resignation vector applies the same recipient
generation rule to `ReflectAttributeValues`: after the departing constrained
member resigns, only the survivor receives the time-7 reflection with the
original producer, order metadata, and retraction handle; neither member emits
an erroneous retraction callback and the producer's post-grant retract is
rejected at the standard boundary.
The matching timestamped deletion vector applies the same stale-recipient rule
to `RemoveObjectInstance`: the departing member receives no queued removal,
the survivor receives the time-7 removal with the original retraction handle,
and the producer reaches the standard post-grant `MessageCanNoLongerBeRetracted`
boundary.
The timestamped-deletion vector similarly routes one C++ `Delete Object Instance`
to all three Java time-request modes before their grants, preserving removal
time/order metadata and mapping the consumed handle to
`MessageCanNoLongerBeRetracted`.
The directed-interaction vector exercises the standard
`sendDirectedInteractionWithTime` overload against the same three grant modes,
preserving target, reliable transport, producing-federate, timestamp/order, and
terminal retraction metadata through the directed callback.
The regional-retraction vector queues an overlapping source/receiver update,
retracts it while FQR/TARA/NMRA requests are pending, and proves all three
grants complete without reflection or `requestRetraction` callbacks.
The regional-interaction sequence adds the corresponding Java
`sendInteractionWithRegionsWithTime` path: a pre-grant retraction is suppressed,
the next timestamped interaction arrives after its grant, and a later callback
conveys the explicit source `RegionHandleSet`.
The matching regional-object vector routes a timestamped associated update
through FQR, TARA, and NMRA available-advance requests, preserving the
conveyed source region, `TIMESTAMP` order metadata, shared retraction handle,
and callback-before-grant ordering.
The multi-source regional-object matrix registers one attribute against two
source regions and proves that each recipient receives only while its source
overlap is active, that conveyed-region metadata shrinks as associations are
removed, and that the standard empty default-region designator returns when
the final explicit association is removed before one association is restored.
It also repeats the complete source association and proves the additive C++
association ledger remains idempotent: each Java recipient receives exactly
one reflection with the unchanged source-region set.
The multi-source regional-interaction matrix sends one region set through two
independently moving source ranges, proving lower-only, upper-only, both, and
fully disjoint recipient transitions through the standard Java region carrier.
The timestamped-save/NMRA vector then proves the strict Available boundary
through the exact Java `nextMessageRequestAvailable` method: an equal first
grant delivers the queued TSO without opening the save, while a later NMRA
initiates the save before its grant.
The timestamped-save/TARA vector proves the corresponding Available form with
no queued message: equality grants the constrained member without opening the
save, and a later strict grant initiates it before the Java grant callback.
The all-advance-mode timed-save vector joins five constrained Java members and
one regulator, requests TAR/NMR/TARA/NMRA/FQR at their qualifying boundaries,
and proves each constrained callback precedes its own grant while the regulator
receives save initiation after its grant.
The mixed FQR/TAR companion then requests two Flush Queue boundaries (6 and 7)
and an inclusive TAR boundary (5) in one timed save. It proves the C++ GALT
frontier clamps the second FQR's effective Java grant to 6 while preserving
initiation-before-grant ordering for both FQR members and TAR, with the
regulator's save initiation still ordered after its grant.
Its HLA_IMMEDIATE companion stages the same frontier behind the standard
callback-disable service, then re-enables each Java member and proves the
immediate callback path preserves the identical effective-grant and ordering
rules.
The FQR boundary companion adds a queued timestamped interaction at the save
time: its first inclusive FQG delivers the interaction without initiating the
save, while the later strict FQG initiates before grant and preserves the C++
retraction handle through the Java callback.
Its HLA_IMMEDIATE companion stages the same six standard requests behind the
public callback-disable boundary, then re-enables each member and proves the
immediate Java callback path preserves every C++ initiation-before-grant
ordering across TAR/NMR/TARA/NMRA/FQR while the regulator remains ordered after
its grant.
The restore matrix then runs each exact Java advance service in an isolated
saved execution, reconstructs its pending grant through C++, and proves the
standard `federationRestored` callback precedes the reconstructed grant for TAR,
NMR, TARA, NMRA, and FQR; each service is submitted again after restore to
prove the Java surface is released for continued use.
The two-member interlock matrix holds both standard Java ambassadors in active
save and restore barriers and verifies declaration, object-registration, region,
time-query, synchronization, interaction, and order-change services fail with
the corresponding C++-originated `SaveInProgress` or `RestoreInProgress` before
the peer completes the barrier.
The immediate/evoked scope vector also proves synchronous versus queued
`attributesInScope`/`attributesOutOfScope` delivery, stale queued-transition
suppression across region, association, and subscription changes, ordinary
subscription fallback, and advisory-switch gating through the standard Java
callbacks.
The ownership/time cross-service slice now proves that an accepted timestamped
object update retains its original C++ producer and payload across a pre-grant
ownership transfer, and the companion If Available order-reset vector proves
the acquiring member does not inherit the former owner's per-instance
`TIMESTAMP` override. The external IEEE-JAR lane now also covers one object
with two independently associated attributes from the repository's
multi-attribute regional FOM: each overlap produces its own standard Java
reflection with the matching source region, one disjoint attribute is omitted,
and fully disjoint source ranges suppress the update. A companion two-subscriber
matrix combines those per-attribute source associations with recipient
isolation: the Flavor-only and Organic-only Java members each receive only
their matching attribute and source region, selective source-range moves
silence exactly one member, and fully disjoint ranges suppress both. A
two-dimensional companion moves Flavor out on X and Organic out on Y before
restoring both source regions, proving conjunctive overlap and independent
attribute routing across the standard Java region carriers. The ownership
fan-out companion additionally queues regular acquisition requests from two
independent Java members and proves one C++ `AttributeOwnershipReleaseDenied`
operation emits typed unavailable callbacks to both requesters with the denial
tag while ownership remains with the original owner. Its companion
regional request matrix repeats that state space before the provider callback:
the Java `provideAttributeValueUpdate` callback receives both attributes, then
only Organic, then neither, and finally both again as requester regions move
through disjoint and restored two-dimensional ranges.
The same vector also queues an accepted request, moves both requester regions
disjoint before the evoked callback boundary, and verifies that C++ suppresses
the stale provider callback and response reflection.
The companion default-region request vector keeps an ordinary registration
eligible while an explicitly associated source is disjoint, admits both after
the requester overlaps, and suppresses only the queued explicit callback when
the requester moves away before delivery.
The order-control companion preserves the C++ order state machine through the
standard Java methods: the Restaurant FOM's timestamp default is captured by
the first registration, a prospective class-default RECEIVE change affects
later registrations, an instance override restores TIMESTAMP for one object,
and the remaining object stays RECEIVE-ordered. Java/JPype callbacks preserve
the resulting time/order/retraction metadata, while a publisher-scoped RECEIVE
interaction is delivered without a timestamped retraction handle.
The dedicated regional TAR/NMR vector now sends one timestamped associated
attribute update to two overlapping receivers and proves both `timeAdvanceRequest`
and `nextMessageRequest` deliver the reflection before their logical-time-7
grants, preserving the explicit source region, `TIMESTAMP` order metadata, and
the identical C++ retraction handle.
The mixed timestamped-source vector then fans an explicit-region object and a
default-region object to FQR, TARA, and NMRA recipients, preserving source
region versus empty-region callback metadata and reflection-before-grant order
for both payloads.
The no-time regional-object companion now mirrors the C++ overlap sequence:
an initially disjoint subscriber receives no discovery or reflection, restored
overlap admits an ordinary update without conveyed regions, the recipient-local
switch then conveys the source region, and disjoint/re-overlap transitions
suppress and restore callbacks before both source associations are removed.
The new three-dimensional `Soda.Light` regional-object matrix extends that
proof through the standard Java pair-list carrier: inherited `BarQuantity`,
`SodaFlavor`, and `Sweetener` dimensions each independently move outside the
subscriber range and suppress reflection, while restoring all three admits a
single callback with the C++ source-region designator intact.
The native Catch2 companion guards the same complete-overlap rule and
one-dimension-at-a-time suppression before the external Java route is exercised.
The time-factory matrix also exercises provider-selected integer and floating
logical-time creation, encoding, decode, add/subtract/difference, and typed
underflow/non-finite failures through the standard Java carrier. The external
JNI float-time edge matrix now additionally carries signed zero, denormal
epsilon stepping, finite-final boundaries, non-finite interval rejection, and
malformed time/interval payloads through the IEEE Java API.
The direct native 2010 route consumes the same wire and arithmetic vectors at
both the raw pybind factory and the standard Python façade. It records the
provider-owned canonicalization of signed zero and the C++ carrier's IEEE
rounding for the smallest-subnormal subtraction, so these representation and
arithmetic differences remain visible rather than being mistaken for a Java
adapter defect.
The next slice is broader regional association matrices beyond these
multi-member, two-dimensional, per-attribute, and recipient-isolation vectors,
provider-specific arithmetic beyond the standard integer/floating matrix, and
malformed-input matrices beyond the now-covered logical-time, standard
handle-factory, and primitive/composite encoder payload boundaries. The external
regional interaction re-enable vector
now closes one of the native callback-boundary cases by proving a queued
explicit-source interaction survives a constrained-member role transition
without duplicate delivery or source-region replacement; its companion
default-region case preserves the standard empty `RegionHandleSet` through the
same transition.
The two-dimensional regional interaction vector now also combines two source
regions, proving complete-dimensional overlap uses OR semantics without
duplicate delivery and conveys the full source set through Java.
The corresponding regional-object case now preserves the queued attribute
passel and explicit source region through the same Java constrained-role
transition.
Its default-region companion preserves the same passel with an empty conveyed
`RegionHandleSet`.
The regional provider-response companion now queues a receive-order response,
mutates only the requester's committed source range before callback dispatch,
and proves the C++ DDM boundary suppresses the stale reflection through the
standard Java region carrier.
The regional-association MOM companion now decodes six standard
`HLAreportServiceInvocation` callbacks for unknown-object, invalid-region, and
successful association/unassociation calls, including their nested pair-list
arguments and C++ exception text.
The regional object-subscription MOM companion likewise decodes six standard
callbacks for unknown-class, invalid-region, and successful subscribe/
unsubscribe calls, including passive/update-rate arguments and nested
attribute-region pair-list payloads.
The accepted regional-interaction MOM companion decodes passive-to-active
replacement and unsubscribe reports, preserving the inverse passive indicator,
region-set argument, null returned record, and serial ordering.
The accepted regional-send MOM companion decodes the five supplied argument
records, including constrained parameter values, source region, user tag, and
the null timestamp slot, before the receiver's conveyed-region callback.
The accepted regional-update MOM companion decodes the four supplied argument
records, including the constrained attribute/value map, user tag, and null
timestamp slot, before the receiver's conveyed-region reflection callback.
The accepted timestamped regional-update MOM companion additionally decodes
the logical-time argument and message-retraction return record, then preserves
timestamp/order/retraction metadata through the constrained reflection.
The failed ordinary regional-update MOM companion decodes two unsuccessful
service reports, preserving typed unknown-object/ownership exception text,
the constrained value-map/tag arguments, null returns, serials, and the
absence of any reflection callback. Structurally valid unknown handles are
used because the standard Java decoder rejects malformed empty handle bytes
before C++ service reporting can run.
The failed regional-send MOM companion similarly decodes three unsuccessful
`SendInteractionWithRegions` reports, including interaction/parameter/region
argument records and null returns; the Java route preserves publication,
parameter-definition, and invalid-region exception mappings without emitting
an application interaction.
The failed timestamped regional-send MOM companion extends that matrix to
four reports, retaining the logical-time argument for each failure and
mapping invalid timestamp admission to `InvalidLogicalTime` with the same
null returned record and serial ordering.
The failed timestamped regional-update MOM companion adds the corresponding
three-report object matrix, preserving object/value/tag/time arguments,
unknown-object and ownership failures, invalid-time mapping, null returns,
serial ordering, and suppression of timestamped reflection.
The timestamped DeleteObjectInstance failure companion adds the two-report
unknown-object/invalid-time matrix, preserving type-37 object, type-63 tag,
type-31 time, null return, exception, and serial fields without a removal
callback.
The receive-order DeleteObjectInstance companion adds the three-state
unknown/success/repeat matrix, proving the service report precedes removal
and repeat deletion produces no second removal callback.
The failed timestamped directed-interaction MOM companion decodes four
`SendDirectedInteraction` reports, preserving interaction/target/parameter/
tag/time records and mapping unknown class, target, parameter, and invalid
time failures without emitting a directed callback.
The accepted order-change MOM companion decodes the standard
`ChangeInteractionOrderType` service report, including declaration-management
service type, interaction-class/order arguments, Null return, success, and
serial fields through the raw Java encoder and JPype.
The native Catch2 companion now guards the same classification at the C++
boundary for all three order-change services and the default transportation
change: order transitions remain service type 1 while transportation remains
service type 2 before the Java/JPype report decoder sees them.
The latest federation-management vectors also cover mixed immediate/evoked
callback sequencing and C++ save/restore MOM status/request/confirmation/
initiation/completion reports through the external Java API.
The C++-only
extendable-variant binding is now implemented and tested as an explicit native
factory extension; it does not widen the shared Java-shaped contract.
Fixed and variable arrays now cross both provider boundaries through
the standard `DataElementFactory` shape, fixed records now cross both
provider boundaries through their heterogeneous `appendElement` shape, and
variant records through their discriminant/alternative mapping.
Broader vendor-specific time-based save/restore scheduling and finer-grained
regional object association edge cases remain deferred; receive-order and
non-regional timestamped Java fixture fanout now reach two independent
subscribers, while scalar save/restore, saved
logical-time and lookahead rollback, timestamped-save input, constrained
timed-save grant ordering, in-transit and re-entrant TSO save boundaries,
native and external-JNI live-TSO retraction restoration, and native plus
Java-adapter timestamped regional attribute callbacks and the external-JNI timestamped default-region interaction callback are covered. Regional object and interaction
reprojection now also exercises multiple overlapping subscription regions,
partial removal while another overlap remains active, and duplicate-discovery/
duplicate-delivery suppression in both providers.
Broader provider-specific floating-time arithmetic and non-time malformed-input
matrices still remain; nested fixed-record/fixed-array/variable-array components and composite
discriminants are covered in all providers, while the C++-only extendable
variant is covered on the native provider with length-prefixed unknown-alternative
skipping.

The staged external-JAR vector now also queues a timestamped directed interaction
for two constrained Java recipients, resigns one recipient before its qualifying
grant, and proves that C++ suppresses that stale callback while the surviving
recipient receives the original time/order/retraction metadata.  The producer's
post-grant retract correctly remains `MessageCanNoLongerBeRetracted` at the
requested-time-plus-lookahead boundary.

The directed-interaction route intentionally has no regional variant: the
2025 standard declares neither a region-bearing directed-send overload nor a
region designator on `ReceiveDirectedInteraction`. Regional coverage therefore
continues through the ordinary interaction and object-attribute services,
without widening the shared Python contract with a non-standard directed-DDM
method.

The external float64 timed-save boundary companion now runs the same
two-member constrained admission under `HLA_IMMEDIATE` as well as
`HLA_EVOKED`. Both Java callback models receive typed `HLAfloat64Time`
initiation callbacks only after the regulating and constrained members reach
the C++ save boundary, and both complete the shared save barrier successfully.
The native C++ companion now exercises that same provider-time scheduling
directly, including the typed save timestamp, grant, and completion callbacks.
The external route also completes a float64 timed-save/restore round trip,
preserving typed save initiation and per-member restore initiation/completion
callbacks through the standard Java API.

The JNI overload audit derives every name/arity pair from the Java RTI fixture
and prevents a declaration from falling through the proxy's unbound-service
path. It now includes timestamped `requestFederationSave(label, time)`: the
public Python `LogicalTime` is encoded in Java, decoded by JNI with the
selected C++ time factory, and admitted by the C++ timed-save state machine.
The end-to-end vector proves regulation, initiation, and completion callbacks.

The real-JVM route now also selects both C++ reference time implementations:
`HLAfloat64Time` factory values, subnormal epsilon, float lookahead/query
results, and time-advance grants preserve their floating type and value through
C++, JNI, Java, and the public Python callback surface.
The vector uses the standard empty implementation selector and observes C++'s
required `HLAfloat64Time` default rather than reproducing that rule in Java.
That vector also deliberately pairs the integer Restaurant FOM with float
time, proving C++ `InconsistentFOM` retains its exact Python exception type
even when the minimal Java fixture forces the dynamic proxy to wrap a checked
exception.
The HLA_IMMEDIATE regional companion adds source-coordinate filtering: only the
matching federate-lost subscriber receives the synchronous report, while the
adjacent disjoint subscription remains silent.
Latest clean verification records 755 native Catch2 test cases (41,797
assertions) and 442 external IEEE-JAR JNI/JPype tests, all passing. The native
class-request callback regression now matches the standard zero-window
`evokeMultipleCallbacks` contract: a first call reports a remaining queued
provider callback, while the final call reports an empty queue; a resigned
provider's stale callback is consumed without invoking user code.
