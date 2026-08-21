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

## External Java API evidence

Umbra's repository fixture remains a fast compatibility harness; it is not
the authority for Java API compatibility. On 2026-08-21, the JNI façade was
compiled and smoke-tested against an independently obtained IEEE
1516.1-2025 Java API JAR, then exercised through C++ → JNI → Java → JPype →
the public Python API. The resulting integration suite now collects 159 test
cases (all 159 passed, including 141 functional
vectors, the standard named and no-argument `RtiFactoryFactory` discovery
checks, and the structural/runtime gates). The exception-surface check loads every C++ exception whose exact
standard Java class is present in the external API; the independently supplied
API remains authoritative for its vocabulary and intentionally omits a few
legacy advisory exception names. Running Python's complete JPype test-package discovery against the same
artifact also executes 202 discovered tests (180 passed, 22 deliberate skips,
and 78 subtests). An artifact gate
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
object class. The same run now exercises the
official `getHLAversion()` Java method, the standard send-report-file switch,
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
`InvalidLookahead` rejection before mismatched bytes reach a time service.
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
Its `DELETE_OBJECTS_THEN_DIVEST` companion drives the same standard directive
through C++ transport loss with one retained transferred object and one
lost-member-owned object, proving the survivor receives exactly one typed
`removeObjectInstance` and one ownership-assumption callback while the retained
object remains name-resolvable and the deleted object does not.
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
reflect states 3/1 and 5/1 to the observer through JPype.
After the peer resigns, the same route reuses its still-structurally-valid
standard `FederateHandle` in a later explicit set and preserves the asynchronous
`SYNCHRONIZATION_SET_MEMBER_NOT_JOINED` failure for the former member.
The timed-save vector also proves that admission remains pending after the
regulating member reaches the boundary, then admits both members only after
the constrained member reaches its qualifying grant, with initiation ordered
before that grant callback.
The in-transit TSO vector additionally proves the timestamped interaction is
delivered before save initiation and that initiation still precedes the peer's
grant callback.
The re-entrant TSO vector also requests a save from inside the JPype receive
callback, confirms the request is accepted without initiation during the
callback, and verifies initiation follows callback return.
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
The companion float64 restore vector performs the same rewind through the
standard Java `HLAfloat64Time` and `HLAfloat64Interval` carriers.
The terminal-retraction restore vector also preserves a saved terminal
retraction classification while rejecting a post-save handle as invalid.
The live-retraction restore vector additionally saves two queued timestamped
interactions in a two-member federation, preserves their timestamp order and
distinct retraction handles through restore, flushes both through the external
Java time/order carriers, and routes each restored handle's `requestRetraction`
callback back to the receiving Python federate.
The pending-time restore vector also saves a constrained member while its
time-advance request is still pending, completes that save re-entrantly from
the standard Java callback, rejects time services during restore with the
typed `RestoreInProgress`/`InTimeAdvancingState` states, and proves that the
restored C++ request produces the saved time-5 grant only after
`federationRestored`.
The companion HLA_IMMEDIATE vector saves a pending `flushQueueRequest`,
verifies `save-initiate`/`save-complete`/`flush-grant` callback ordering,
reconstructs the saved time-6 flush grant synchronously after restore, and
accepts a fresh time-7 flush request afterward.
The six-case advance-variant matrix extends this to pending
`timeAdvanceRequestAvailable`, `nextMessageRequest`, and
`nextMessageRequestAvailable` requests under both HLA_EVOKED and
HLA_IMMEDIATE, preserving the saved time-5 grant and post-restore usability
for every standard Java overload.
The same external vector now proves float64 `add`, `subtract`, and `distance`
at representable-value, smallest-positive-subnormal, and initial/final boundaries, integer64 arithmetic
above 2^53, and the standard Java interval `add`/`subtract` mutators,
including typed `IllegalTimeArithmetic` mapping. The raw Java carriers also
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
the normalized Python factory rejects them before delegation.
It also exercises a malformed payload matrix across all twenty-six primitive
encoder classes and all four standard composite forms, including nested
fixed-record, fixed-array, variable-array, and variant children, preserving
typed `DecoderException` results through the external Java API.

That external run proves the adapter uses the actual Java shapes where they
differ from the historical fixture: `String`/`String[]` FOM inputs; time
carriers from the `time` package and their standard `getValue` accessors;
timestamped service overloads returning `MessageRetractionReturn`; typed
interaction-class sets; and `AttributeSetRegionSetPairListFactory` with
`AttributeRegionAssociation(ahset, rhset)`. The callback gate also reflects the
loaded external `FederateAmbassador`, verifies all 56 exact callback names, and
constructs the actual standard JPype proxy before accepting the C++ callback-slot
and JPype marshaller audit. C++ remains the only RTI semantic
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
| Scalar restore success | request; complete | request acceptance/rejection, begin, per-federate initiation with a typed handle, `federationRestored` | Native two-member, real-JVM fixture, and external IEEE-JAR JNI lifecycles covered; the Java routes coordinate per-member completion, preserve requester-scoped acceptance, return typed initiation handles, and the external route rewinds saved logical time/lookahead and preserves terminal retractions while invalidating post-save handles, while native and external restore reinstate a live timestamped interaction/retraction ledger with multiple ordered messages; the external ownership-state vector saves an object while federate A owns an attribute, transfers it to federate B after save, and proves restore reinstates A's C++ ownership through Java and JPype; the object-management companion saves before post-save declarations/object registration and proves restore removes the post-save object through the standard Java lookup boundary; the regional-object companion saves an overlapping association, mutates its source range to disjoint, and proves restore rewinds the source region and delivers a later update with the restored region designator |
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
| Receive-order interaction management | `sendInteraction` with published parameters | byte-copying `ParameterHandleValueMap`; interaction receipt with typed source/transport handles | Native two-federate Restaurant FOM lifecycle and real-JVM fixture covered; real-JVM fixture now fans one send out to two independent subscribers; the external IEEE-JAR JNI lane also proves accepted direct and regional `Send Interaction` calls emit the exact seven-parameter `HLAreportServiceInvocation` through C++ → JNI → Java → JPype, with Java `EncoderFactory` decoding, serial zero, reliable receive-order delivery, the RTI-originated invalid producer handle for the direct path, and the regional region-set argument |
| Logical time management | provider `LogicalTimeFactory`, initial/final/zero/epsilon values, encode/decode, provider-owned add/subtract/difference arithmetic, standard carrier predicates/comparison/equality, regulation/constrained mode, asynchronous delivery, lookahead query/modification, all five time-advance/queue request variants, GALT/LITS queries, and timestamped service inputs | immutable provider-created `HLAinteger64Time`/`HLAinteger64Interval` and `HLAfloat64Time`/`HLAfloat64Interval` snapshots, Java-shaped `TimeQueryResult`, and typed regulation/constrained/grant/`flushQueueGrant` plus timed object/interaction callbacks | Native C++ and fake/real-JVM Java adapter lifecycles cover integer time; native and real-JVM restore tests preserve saved logical time, actual lookahead, and deferred lookahead decreases; the C++ and Java providers now delegate factory construction, factory decode, add/subtract/difference, and validation to their native logical-time implementations and preserve exact floating values through factory, decode, arithmetic, query, grant, and timed callbacks, including smallest-positive-subnormal epsilon parity, exact and one-step-overflow additions at the largest finite float64 value, the symmetric exact and one-step-overflow additions at signed `Long.MAX_VALUE`, an epsilon-sized floating time-advance/grant, and typed `CouldNotDecode` mapping for truncated, trailing, negative, and non-finite integer/floating encodings; the external IEEE-JAR JNI vector also proves representable-boundary float64 add/subtract/distance, integer64 arithmetic above 2^53, direct standard Java factory construction/decode, interval arithmetic, and standard Java carrier `isInitial`/`isFinal`/`isZero`/`isEpsilon`/`compareTo`/`equals`/`hashCode` methods, typed `IllegalTimeArithmetic` mapping through C++ → JNI → Java → JPype, and `SaveInProgress` HLAreportException records for `Query Logical Time`, `Query GALT`, and `Query LITS`; broader vendor-specific arithmetic matrices remain |
| Region substrate | `createRegion`, `commitRegionModifications`, `deleteRegion`, dimension-set and range-bound services | immutable `RegionHandle`/dimension and region sets plus mutable Java-shaped `RangeBounds` | Native C++ and fake/real-JVM Java adapter lifecycles covered; the external IEEE-JAR JNI vector also preserves `InvalidRegion`, `RegionDoesNotContainSpecifiedDimension`, and `InvalidRangeBound` for incomplete regions, unrelated dimensions, invalid bounds, and repeated deletion; a two-member foreign-region matrix preserves `InvalidRegion` for read-only lookups and `RegionNotCreatedByThisFederate` for mutating set/commit/delete services, and an unknown-but-well-formed dimension now proves typed `InvalidDimensionHandle` from `Create Region`; each C++ region lifecycle/range failure is decoded as an `HLAreportException` through the official Java carriers and JPype |
| Regional interaction management | regional subscribe/unsubscribe, regional receive/timestamp-order send, and convey-region-designator switch | optional `RegionHandleSet` plus timed callback metadata on interaction callbacks | Native C++ and fake/real-JVM Java adapter callback mappings cover overlapping versus disjoint recipients, including receive-order fanout, timestamped sends, typed retraction routing, and conveyed region designators; both providers now reproject an existing subscription as overlapping regions are added and removed without duplicate delivery, re-evaluate explicit regional subscriptions at the callback boundary when delayed subscription evaluation is enabled, preserve mixed region-designator convey, and honor the bounded Allow Relaxed DDM exact-boundary policy; the external IEEE-JAR JNI vector proves two simultaneous overlapping subscriptions remain active after selective unsubscription and stop delivery only after the final overlap is removed, and preserves `RegionNotCreatedByThisFederate`, `InvalidRegion`, and `InvalidRegionContext` for foreign, uncommitted, and wrong-dimension subscription/send regions; its dedicated report vector enables exception reporting on both originating federates and decodes seven separate C++ `HLAreportException` records for regional subscription/send/unsubscription failures through the official Java encoder and JPype; a two-dimensional external vector requires overlap in both X and Y, then mutates one committed source region to exercise X-only and Y-only recipient transitions through the standard Java region carriers; the external IEEE-JAR JNI vector also proves timestamped default-region interaction delivery with the standard empty `RegionHandleSet`, `TIMESTAMP` sent/received order metadata, and consumed retraction handle, plus receive-order and timestamped region-context `HLAreportServiceInvocation` payloads through the exact Java encoder path; the explicit-source and default-region re-enable vectors queue timestamped regional interactions, disable and re-enable the constrained Java member, and prove one preserved callback each with source-region/empty-region realization, order metadata, and consumed retraction handles; the regional-interaction restore vector saves an overlapping subscription, mutates the source range to disjoint, and proves restore rewinds the range and delivers a later interaction with its restored region designator |
| Directed interaction management | object-class directed publish/unpublish and subscribe/unsubscribe overloads; receive-order and timestamped directed sends | immutable `InteractionClassHandleSet`, typed target object/source/transport callback payloads, and optional time/order/retraction metadata | Native C++ receive-order delivery and three-federate timestamped retraction fanout prove only delivered recipients receive `requestRetraction`; fake/real-JVM Java declaration, send, and timed callback mappings are covered, including real-JVM multi-recipient directed fanout and constrained pending retraction |
| Regional object management | Java-shaped `AttributeSetRegionSetPairList`; regional object registration, regional subscription, association/unassociation, and regional value requests | immutable attribute-set/region-set pair values; C++ typedef aliases remain available | Native C++ overlap/discovery/update lifecycle includes mixed-fanout timestamped reflection and convey-switch behavior, independent per-attribute region association with selective retention/disjoint filtering, default-region fallback, existing-object discovery reprojection as a subscription gains multiple overlapping regions without duplicate discovery, scope-advisory out/in transitions across partial and complete regional unsubscription/re-subscription, and a two-subscriber receive-order association matrix that brings only a previously out-of-scope known recipient back into scope before selectively removing it; default-region callback realization and exact-boundary relaxed DDM are covered; both providers now re-evaluate explicit regional object updates at the callback boundary when delayed subscription evaluation is enabled; fake Java callback conversion covers present/absent region metadata, and the real-JVM adapter carries per-attribute registration/association/unassociation region sets into region-relevant discovery and timestamped multi-recipient reflection, broadcasts association scope changes to the affected recipient only, conveys an empty `RegionHandleSet` for receive-order and timestamped default-region realizations (while ordinary non-regional callbacks remain `None`), filters disjoint ranges before delivery, routes `requestAttributeValueUpdateWithRegions` to the owning federate only when source and requester ranges overlap while suppressing disjoint requesters and copying the request tag, proves the standard Java `provideAttributeValueUpdate` callback can answer a regional request and return a region-conveyed reflection with copied values/tag/transport/producer metadata, proves a registered object remains undiscovered under a passive regional subscription until an active declaration replaces it, exercises exact-boundary relaxed DDM updates, maps deletion of a still-referenced source region to `RegionInUseForUpdateOrSubscription`, and proves an already-known object re-enters scope after a complete regional unsubscription; the C++ → JNI → Java → Python vectors now prove live association mutation, two-subscriber recipient isolation, selective in/out callbacks, empty default-region fallback, restoration of both recipients after the final source unassociation, and two-dimensional known-object reflection filtering with independent X-only and Y-only source-range transitions; the external timestamped regional object-update vectors also survive a constrained-member disable/re-enable transition once, preserving explicit source-region and empty default-region realizations, order metadata, and retraction handles; invalid foreign, uncommitted, wrong-context, and unknown-object/region pair-list cases also emit decoded C++ `HLAreportException` records for regional registration, association/unassociation, and regional subscription teardown, while the live-registration guard independently decodes `RegionInUseForUpdateOrSubscription` from `Delete Region` through an observer's standard Java MOM subscription |
| Attribute value update requests | class- and instance-targeted `requestAttributeValueUpdate` overloads and federation-defined automatic provision | typed class/instance handle dispatch, immutable `AttributeHandleSet`, copied request tags, and `provideAttributeValueUpdate` callback | Native C++ and fake/real-JVM Java adapter mappings covered, including callback tag/attribute conversion; the external IEEE-JAR JNI vector also proves C++ automatic provision queues the standard empty-tag callback after discovery through the Java `String[]` FOM-create overload and that standard `HLAsetSwitches` interaction parameters enable/disable later discovery-triggered callbacks federation-wide; class, instance, and `requestAttributeValueUpdateWithRegions` failures now preserve typed C++ exceptions and emit the corresponding `HLAreportException` through the official Java set/region carriers |
| Object-management advisory callbacks | scope entry/exit and per-object update relevance (plain and named-rate forms) | typed `ObjectInstanceHandle`/`AttributeHandleSet` callback payloads with optional update-rate designator | Native C++ regional scope transitions and fake/real-JVM Java callback fixtures covered |
| Order and transportation management | `changeAttributeOrderType`, default-attribute and interaction order changes; attribute/interaction transportation change and query services | strict `OrderType`/`TransportationTypeHandle` domains plus typed confirmation/report callbacks | Native C++ and fake/real-JVM Java adapter mappings covered; the external IEEE-JAR JNI vector now drives all eight standard Java order/transport overloads through JPype, preserving `ObjectInstanceNotKnown`, `AttributeNotDefined`, `InteractionClassNotPublished`, and `InteractionClassNotDefined` while each C++ failure emits the matching `HLAreportException` service name |
| Attribute ownership management | `queryAttributeOwnership`, ownership status, unconditional and negotiated divestiture, divestiture confirmation/cancellation, regular/if-available acquisition, acquisition cancellation, release-denied, and divestiture-if-wanted services | typed ownership callbacks for assumption, divestiture confirmation, acquisition/unavailability/release, cancellation, and ownership reports; encoded federate owner handles | Native C++ and fake/real-JVM Java adapter service mappings and typed callback surfaces covered; native and real-JVM two-member tests now prove negotiated acquisition ordering, confirmation-tag propagation, ownership transfer, release-denied state preservation, denial-tagged unavailable callbacks, the if-available/already-owned unavailable path, both regular and if-available federate-owned self-acquisition preconditions, overlapping pending-acquisition rejection, confirm-without-request and duplicate-divestiture typed errors, no-pending cancellation errors, unconditional release/reacquisition, pending-acquisition cancellation, and `AttributeNotOwned` preservation for release-denied, ordinary update/order/transport services, and all three divestiture forms after transfer; the external IEEE-JAR route additionally proves `NO_ACTION` rejects a member-owned object with `FederateOwnsAttributes`, `UNCONDITIONALLY_DIVEST_ATTRIBUTES` emits a typed assumption callback and permits peer acquisition, a deferred assumption search resumes when a known candidate publishes later, and a pending acquisition rejects that action with `OwnershipAcquisitionPending` while `CANCEL_THEN_DELETE_THEN_DIVEST` completes the resignation path; mixed external `If Available` and regular acquisition requests now split one C++ plan into recipient-local secured/unavailable and acquirer/owner release callbacks for unowned versus remote-owned attributes, preserving copied tags and handles; the same external vector now queries both states in one request and proves separate standard owner-report and not-owned callbacks; all remaining ownership request/control failures (`queryAttributeOwnership`, `isAttributeOwnedByFederate`, negotiated/unconditional divestiture, confirmation/cancellation, regular/if-available acquisition, release-denied, divestiture-if-wanted, and acquisition cancellation) now preserve typed C++ exceptions and emit standard `HLAreportException` interactions through JNI and JPype; the timestamped-update ownership-transfer vector proves the official ownership transition does not rewrite or suppress an already accepted TSO payload before its grant; the external If Available order-reset vector proves an old owner's per-instance `TIMESTAMP` override is cleared at transfer, with the acquiring Java member returning an invalid retraction while its timestamped update callback carries `RECEIVE` sent/received metadata |
| Advisory/reporting support switches | object-class, attribute, scope, and interaction relevance switches; automatic-resign directive; service/exception reporting switches; provider support-switch queries | typed `ResignAction` plus boolean state with provider-defined initial values | Native C++ and fake/real-JVM Java adapter mappings covered; the external IEEE-JAR JNI vector proves C++ service-report JSON records after Java-routed time-regulation services, direct and regional `Send Interaction` standard `HLAreportServiceInvocation` payload/callback paths, C++ `HLAreportException` callbacks for typed `InteractionClassNotPublished` failures from direct, timestamped, directed, non-timestamped region-context, and timestamped region-context `Send Interaction`, `AttributeNotDefined` from `publishObjectClassAttributes`, `ObjectClassNotPublished` from both `registerObjectInstance` overloads, `ObjectInstanceNotKnown` from both untimed and timestamped `updateAttributeValues` overloads and receive-order, timestamped, and local `deleteObjectInstance` overloads, both declaration-management report-service conflicts, duplicate `enableTimeRegulation` and `enableTimeConstrained` conflicts, both duplicate/initial-state `enableAsynchronousDelivery` and `disableAsynchronousDelivery` conflicts, initial-state `disableTimeRegulation`/`disableTimeConstrained` conflicts, unregulated `queryLookahead`/`modifyLookahead` conflicts, and pending `timeAdvanceRequest`/`timeAdvanceRequestAvailable`/`nextMessageRequest`/`nextMessageRequestAvailable` conflicts; `subscribeInteractionClass` preserves `FederateServiceInvocationsAreBeingReportedViaMOM`, `setServiceReportingSwitch(true)` preserves `ReportServiceInvocationsAreSubscribed`, and the time-state methods preserve `TimeRegulationAlreadyEnabled`/`TimeConstrainedAlreadyEnabled`/`AsynchronousDeliveryAlreadyEnabled`/`AsynchronousDeliveryAlreadyDisabled`/`TimeRegulationIsNotEnabled`/`TimeConstrainedIsNotEnabled`/`InTimeAdvancingState`; each delivers a separate exception report; other declared exception-report service families and DDM state-space matrices remain |

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

1. **Federation-management edge vectors.** Expand save/restore,
   synchronization, and membership matrices with more negative/ordering cases
   while retaining the completed URL/handle-set and lifecycle mappings.
2. **Shared handle and encoding depth.** Extend the already provider-backed
   typed handles, sets/maps, and C++/Java encoder boundary with additional
   composite and malformed-wire matrices; no Python reimplementation of
   encoding is allowed.
3. **Declaration and object management.** The foundational class/attribute/
   parameter lookup pairs, basic publish/subscribe, the receive-order object
   registration/discovery/deletion/update lifecycle, and receive-order
   interaction send/receive are complete. Timestamped services and the
   provider-backed DDM-region forms now cross both adapters; only the
   remaining callback/service families stay on the follow-on list.
4. **Time management.** Bind logical-time factory use, provider-owned arithmetic, time advance requests,
   and grant callbacks as one lifecycle. Time values stay provider-created;
   Python does not invent a competing clock implementation. Integer and
   floating-time factory boundaries plus the minimal timestamped-service
   lifecycles are complete; broader provider-specific arithmetic edge cases remain.
5. **Ownership and data distribution management.** The provider-backed region
   substrate, regional interaction declaration/send, regional object
   registration/update pair-vector lifecycle, class/instance attribute-value
   update requests, directed-interaction declaration/send, order/transportation services, and the complete attribute
   ownership service surface and relevance/reporting support-switch families
   now cross both adapters. Continue with callback sequencing/edge cases and
   the remaining DDM service families.
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
 finds all 43 standard creator names on the Java façade.
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
so `JniRtiFactory(artifact_directory=...)` can discover it without a separate
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
`int` normalization coordinate, alongside C++ object-class lookup/name/
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
sets and copied tags. Attribute and interaction transportation query services
now return typed C++ report callbacks, and their change-request services return
typed confirmations. Time- and region-based object variants now cross the JNI
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
first timestamped interaction/retraction vector issues a C++ retraction handle,
flushes the receiving constrained federate, and proves typed time/order/handle
metadata plus the later `requestRetraction` callback across JNI and JPype. The
same carrier path now proves timestamped attribute updates, object deletions,
and their typed reflection/removal lifecycles, each followed by the matching
`requestRetraction` callback.
The JNI DDM foundation now carries opaque dimension and region handles through
the standard Java factories, including available-dimension queries, range
bounds, region commits, and deletion. Regional interaction subscriptions and
both receive-order and timestamped sends are C++-backed as well; their
overlapping region filter and the optional sent-region set are preserved in
the Python callback, together with timestamp, order, and retraction metadata.
The handle-factory companion deliberately keeps decoding at the C++ codec
boundary (the Java standard factories do not own an RTI ambassador); malformed
federate, object, interaction, attribute, parameter, dimension, region, and
message-retraction encodings therefore preserve typed `CouldNotDecode` through
JNI and JPype without inventing a service-level MOM report.
Regional object services now use the same C++ association semantics through a
typed Java `AttributeSetRegionSetPairList`: regional subscription,
registration, association/unassociation, and regional attribute-value request
all cross JNI. The two-member JNI vector proves overlapping DDM filtering,
regional discovery/reflection with its sent-region set, and the provider
callback carrying the original value-request tag.
The JNI advisory/support slice now also reaches the C++ ambassador for all
relevance/reporting switch reads and mutable switch updates, the automatic
resign directive, and `normalizeServiceGroup`. Standard Java
`ResignAction`/`ServiceGroup` enums are converted only at the Java façade;
Python observes the shared public enum types. The integration vector proves
mutable values round-trip through C++ and that every provider-defined
read-only support switch is callable through the same path.
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
subscription removal and restoration, and receive-order recipient isolation
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
It also proves that the official Java region carriers deliver boundary-touching
regional interactions and object updates when the federation's relaxed-DDM
switch is enabled, and that delayed regional interaction and object-update
delivery re-evaluate the committed range at callback time. A timestamped
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
The passive-subscription vector independently proves that both ordinary and
regional declarations remain non-delivering until an active replacement, while
existing objects are reprojected and subsequent updates cross the standard
Java callbacks only after activation.
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
suppression, and advisory-switch gating through the standard Java callbacks.
The ownership/time cross-service slice now proves that an accepted timestamped
object update retains its original C++ producer and payload across a pre-grant
ownership transfer, and the companion If Available order-reset vector proves
the acquiring member does not inherit the former owner's per-instance
`TIMESTAMP` override. The next slice is broader regional association matrices
beyond these multi-member, two-dimensional, and recipient-isolation vectors,
provider-specific arithmetic edge cases, and malformed-input matrices beyond
the now-covered logical-time, standard handle-factory, and primitive/composite
encoder payload boundaries. The external regional interaction re-enable vector
now closes one of the native callback-boundary cases by proving a queued
explicit-source interaction survives a constrained-member role transition
without duplicate delivery or source-region replacement; its companion
default-region case preserves the standard empty `RegionHandleSet` through the
same transition.
The corresponding regional-object case now preserves the queued attribute
passel and explicit source region through the same Java constrained-role
transition.
Its default-region companion preserves the same passel with an empty conveyed
`RegionHandleSet`.
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
Broader provider-specific floating-time and non-time malformed-input matrices
still remain; nested fixed-record/fixed-array/variable-array components and composite
discriminants are covered in all providers, while the C++-only extendable
variant is covered on the native provider with length-prefixed unknown-alternative
skipping.

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
