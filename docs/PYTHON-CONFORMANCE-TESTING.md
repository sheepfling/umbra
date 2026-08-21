# Python provider conformance testing

## Purpose

The Python binding has a small public surface today, so it should test that
surface as a provider contract rather than repeat provider-specific examples.
`hla.rti1516_2025.testing.ConnectionFoundationConformanceMixin` and
`ConnectionOverloadConformanceMixin` are reusable `unittest` mixins. The
`ProviderBindingParityConformanceMixin` adds a provider-neutral gate for the
same factory, encoder, and ambassador identity/lifecycle calls. A provider
supplies `make_factory()`; together the mixins verify the observable RTI
connection state machine, all four currently supported `connect` forms, and
that native and Java-backed providers expose the shared Python value types.

This is intentionally a binding conformance suite, not an assertion that any
provider implements the full IEEE standard.

## C++ test mapping

The initial contract mirrors the available portions of
`cpp/tests/ieee1516_2025_connection_catch2.cpp`.

| C++ assertion family | Python conformance assertion | Status |
| --- | --- | --- |
| Base `connect` with immediate and evoked models | Both `CallbackModel` values connect and yield a well-formed `ConfigurationResult` | Implemented |
| Duplicate `connect` | Raises `AlreadyConnected` at the Python provider edge | Implemented |
| Unsupported callback enum | A non-`CallbackModel` value raises `UnsupportedCallbackModel`, then the ambassador remains connectable | Implemented |
| `disconnect` before connection | Raises `NotConnected` | Implemented |
| Callback enable/disable controls | Native and real-JVM providers suppress queued callback delivery while disabled and deliver retained callbacks after re-enabling; the boolean evoke result remains provider-defined | Implemented |
| Disconnect then reconnect | A fresh connection succeeds after disconnect | Implemented |
| C++ configuration/credential overloads | Base, `RtiConfiguration`, `HLAnoCredentials`, and both together each connect and yield a well-formed `ConfigurationResult`; the JNI route also forwards arbitrary standard Java `Credentials` type/data to C++ so a rejected credential maps back as `Unauthorized`, while the external callback vector proves C++ rejects re-entrant Connect and Disconnect before lifecycle or credential/configuration processing with `CallNotAllowedFromWithinCallback` | Implemented |
| Java authorization factory | The exact Java `AuthorizerFactoryFactory` discovers `HLAauthorizer`; RTI, federation, and federate decisions preserve C++ authorization codes for matching, wrong, missing, unknown, malformed, and unconfigured credentials | Implemented in the raw Java/JNI route; excluded from the shared Python contract |
| C++ `RtiConfiguration` value semantics | Java-shaped Python builder retains name, address, and additional-settings values | Implemented |
| `listFederationExecutions` / `reportFederationExecutions` | Both callback models deliver a typed `FederationExecutionInformationSet`; unconnected use raises `NotConnected` | Implemented |
| Scalar federation create/destroy with a FOM module | Native provider creates a real federation, lists its typed record, then destroys it; Java adapters perform the same method path against their configured RTI | Implemented |
| Federation lifecycle exception mapping | The JNI/JPype route preserves C++ `FederationExecutionAlreadyExists`, `FederationExecutionDoesNotExist`, `FederateAlreadyExecutionMember`, `FederateNameAlreadyInUse`, `FederatesCurrentlyJoined`, and `CouldNotOpenFOM` through the standard Java exceptions into Python, with duplicate-create, duplicate-join, and destroy-while-joined failures independently decoded as standard `HLAreportException` interactions; the compatible FOM/MIM duplicate-create overload also preserves `FederationExecutionAlreadyExists` with its distinct C++ service label | Implemented |
| Federation FOM/MIM and membership overloads | Native C++ vectors and MIM creation, the exact 2025 Java `String[]`/`String` overloads, additional join FOM modules, and explicit synchronization `FederateHandleSet` registration all cross the shared Python contract, including a duplicate-create report vector through the standard MIM overload | Implemented |
| Synchronization achievement result | The Java fixture and external IEEE-JAR JNI route both preserve the standard `successfully` bit: an unsuccessful member appears in the typed `FederateHandleSet` delivered to `federationSynchronized` | Implemented |
| Synchronization lifecycle edges | The external IEEE-JAR JNI route preserves disconnected/unjoined preconditions, unknown-label `SynchronizationPointLabelNotAnnounced`, asynchronous duplicate-label registration failure, and one-shot completion after repeated achievement; joined-member unknown-label achievements also produce independently decoded C++ `HLAreportException` interactions through the exact Java encoder/JPype route | Implemented |
| `listFederationExecutionMembers` / member-or-missing callbacks | Both callback models report a missing federation through `reportFederationExecutionDoesNotExist`; native and JVM integration tests also convert the typed empty-member report for a real federation | Implemented |
| Scalar join/resign | C++ and Java bindings return a portable encoded `FederateHandle`; native integration proves that a joined federate appears in the typed member report, while native and real-JVM fixtures convert the RTI-originated `federateResigned` reason callback; the external IEEE-JAR JNI route also proves `DELETE_OBJECTS` resignation removes the departing member's known object through the standard `removeObjectInstance` callback, final-federate `NO_ACTION` releases an object name for reuse after rejoin, and the C++ RTI-originated resignation callback removes only the controlled member while leaving its Java connection reusable; its lifecycle edge vector now preserves `FederateOwnsAttributes` for `NO_ACTION` with a member-owned object, decodes a distinct `Resign Federation Execution` `HLAreportException` before corrective resignation, and preserves `CallNotAllowedFromWithinCallback` for re-entrant Connect, Disconnect, Join, and Resign calls from the standard Java callback | Implemented |
| `queryFederationSaveStatus` / response callback | Native provider queries a two-member real federation before, during, and after a save; the fake and real-JVM Java fixture also converts the ordered `(FederateHandle, SaveStatus)` response | Implemented |
| Scalar federation save success | Two real federates receive save initiation, begin/complete their saves, and each receives `federationSaved`; the real-JVM fixture and external IEEE-JAR JNI vector exercise the same lifecycle and enforce the federation-wide completion barrier | Implemented |
| Timestamped federation save request | Native Python drives a provider `LogicalTime` through the C++ overload; native coverage includes immediate, constrained two-federate grant boundaries, an in-transit timestamped interaction delivered before save admission, and a save requested re-entrantly from a TSO callback (initiation waits for callback return); fake Java and the real JVM fixture receive the decoded Java `LogicalTime`, hold initiation until time advance, and assert the same receive -> initiate-save -> grant order; the external IEEE-JAR JNI vector also saves a constrained member while a time advance is pending and completes the save from the standard Java callback, while its HLA_IMMEDIATE companion captures and restores a pending flush-queue request and the six-case matrix covers TARA, NMR, and NMRA in both callback modes; a dedicated standard-Java NMR vector now orders two queued timestamped interactions at successive C++ grants (5 then 7), preserving tags, order metadata, and retraction handles | Implemented; vendor-specific TSO scheduling edge cases remain |
| Scalar federation save failure/abort | A real federate-reported failure and an explicit abort both report matching typed `SaveFailureReason` values to each member; the real-JVM fixture covers both reasons and broadcasts each outcome across two members; the external IEEE-JAR JNI route additionally maps `FEDERATE_RESIGNED_DURING_SAVE`, rejects stale completion, and proves a later save succeeds for the remaining member | Implemented |
| `queryFederationRestoreStatus` / response callback | Native provider queries a two-member real federation before, during, and after restore; the real-JVM fixture also converts typed pre/post handles and `RestoreStatus` records across those transitions | Implemented |
| Scalar federation restore success | A saved two-member federation reports missing-snapshot rejection or acceptance, requester-scoped acceptance, restore-begin, typed per-federate initiation, and `federationRestored` only after both members complete; native and real-JVM fixtures rewind saved logical time/lookahead, and native plus the external IEEE-JAR JNI vector restore multiple ordered timestamped interactions with distinct retraction handles and prove post-restore retraction/request-retraction for each; the external route enforces the same two-member completion barrier and reconstructs a saved pending time advance only after `federationRestored` | Implemented |
| Scalar federation restore failure/abort | A real federate-reported failure and an explicit abort both report matching typed `RestoreFailureReason` values to each member; the real-JVM fixture broadcasts both outcomes across the shared restore transaction; the external IEEE-JAR JNI route also maps `FEDERATE_RESIGNED_DURING_RESTORE` when a participant resigns during the active restore | Implemented in native and real-JVM fixture runs |
| Save/restore precondition and overlap errors | The external IEEE-JAR JNI route preserves `SaveNotInitiated`, `FederateHasNotBegunSave`, `SaveNotInProgress`, `RestoreNotRequested`, `RestoreNotInProgress`, `SaveInProgress`, and `RestoreInProgress` for uninitiated completion/abort calls and overlapping active operations; opposite-operation status queries (`queryFederationRestoreStatus` during save and `queryFederationSaveStatus` during restore) are also typed and independently reported, while scalar and timestamped federation-save requests, synchronization registration/achievement, declaration publication, named object registration, object update, object deletion, direct/regional/directed/timestamped interaction sends, attribute-value requests, ownership queries, order changes, transportation changes, region lifecycle, regional declaration, and region association/unassociation map both active-save `SaveInProgress` and active-restore `RestoreInProgress` with separate C++ `HLAreportException` records through the exact Java API and JPype (the standard MOM uses `Send Interaction` for both untimed and timestamped `sendInteraction` overloads) | Implemented |
| Basic data-element encoding | Native C++ and fake/real-JVM fixture factories produce the same signed 16/32/64-bit, binary32/64 in both byte orders, unsigned 16/32/64-bit in both available byte orders, distinct one-octet byte/octet and ASCII/UTF-16 character vectors, ASCII element-count strings, BE/LE octet-pair vectors, four-byte-count opaque-data vectors, canonical four-octet boolean values, and UTF-16BE Unicode vectors; malformed native and Java-adapter fixed-width scalar, non-canonical boolean, variable-length string, and opaque-data encodings map to `DecoderException`; the external IEEE-JAR JNI lane also drives the exact standard Java `ByteWrapper` cursor overloads for primitive, fixed-record, variable-length ASCII/unicode-string, opaque-data, and variable-array carriers with non-zero offsets, consumed-byte advancement, malformed length-prefix handling, malformed fixed-record/fixed-array/variable-array child/count payloads, and signed-integer decoder exception mapping | Implemented |
| Variable-array encoding | Native C++ and fake/real-JVM Java factories agree on the signed BE element count, leading alignment, zero inter-element padding, no final padding, copied additions, factory-created decoded children, and malformed count/padding/trailing behavior | Implemented |
| Fixed-array encoding | Native C++ and fake/real-JVM Java factories agree on factory-created cardinality, copied slot replacement, scalar encoding/decoding, iteration, prototype alignment, and malformed bounds/size/trailing behavior | Implemented |
| Fixed-record encoding | Native C++ and fake/real-JVM Java factories agree on heterogeneous scalar, nested fixed-record/fixed-array/variable-array append/copy behavior, record-offset alignment before later components, iteration/get/set behavior, and malformed padding/trailing rejection | Implemented |
| Variant-record encoding | Native C++ and fake/real-JVM Java factories agree on copied discriminant/alternative mappings, nested fixed-record alternatives, composite fixed-record discriminants, discriminant selection, alternative alignment, unmapped discriminant wire forms, and malformed padding/trailing rejection | Implemented |
| Native extendable-variant encoding | Native C++ exposes a provider-specific `HLAextendableVariantRecord` factory extension with copied mappings, length-prefixed alternatives, mapped decode, unknown-alternative skipping, and malformed length/padding/trailing rejection; the Java JNI adapter consumes the standard creator through its provider-scoped Python façade and registers mappings on first `setVariant`, while the Java-shaped shared factory remains intentionally provider-neutral | Implemented in native C++ and the Java/JNI façade extension; excluded from the shared Python contract |
| Shared RTI ambassador surface audit | Native and Java provider classes are checked against every abstract method in the shared `RTIambassador` contract; the audit currently reports no missing declarations | Implemented |
| Direct C++/Java provider parity | The same shared mixin invokes factory identity, shared encoder-value creation, and RTIambassador connect/evoke/disconnect behavior against the pybind11 C++ provider and the JPype provider backed by the external IEEE Java API; the external JNI vector additionally proves that Python reaches an actual standard `hla.rti1516_2025.RtiFactory` returned by Java `ServiceLoader`, and that its ambassador is the standard Java `RTIambassador` interface before JPype wrapping | Implemented |
| Support name/handle lookups | A joined native Restaurant FOM federation and the fake/real JVM fixtures round-trip federate, object class, attribute, interaction class, parameter, transportation type, and dimension handles, including known object-class lookup; Java input handles are recreated through their standard `get*HandleFactory().decode` methods; the external IEEE-JAR JNI vector also preserves typed unknown-name/unknown-handle and update-rate failures as independently decoded C++ `HLAreportException` interactions, including known-object-class, object-instance-name, and object-instance-handle failures for an unknown federate-local object | Implemented |
| Handle, set, and map factories | Native decoders and fake/real-JVM Java handle factories round-trip each declared opaque handle domain; interaction-class and attribute/region pair-list factories create mutable Java-shaped Python builders, copy byte values, and feed those builders back through provider services | Implemented; the external IEEE JNI lane invokes the standard message-retraction, interaction-set, and pair-list factories directly |
| Support value lookups | Native Restaurant FOM and fake/real JVM fixtures resolve update rates, order types/names, available object/interaction dimensions, and dimension upper bounds through typed Python values; the external IEEE-JAR JNI vector preserves `InvalidDimensionHandle` for unknown but structurally valid dimension identities through name, upper-bound, and region-creation services | Implemented |
| Support normalization | Native C++ and fake/real JVM fixtures resolve service-group and typed handle normalization coordinates through strict public domains; the external IEEE-JAR JNI lane preserves malformed federate/object-class/interaction-class/object-instance normalization failures and decodes each distinct C++ `HLAreportException` service through Java and JPype | Implemented |
| Timestamped object/interaction services | Native C++ and fake/real JVM fixtures invoke timestamped update, interaction send, regional interaction send, object deletion, and retraction through typed logical-time and message-retraction boundaries; native and real-JVM fixtures also assert typed timed callback metadata, non-null region designators, and `requestRetraction` conversion; the real-JVM fixture fans timestamped object and interaction traffic to unconstrained and constrained subscribers, preserves retraction identity for delivered recipients, and exercises mixed regional designator convey for interactions and associated object updates; the external IEEE-JAR lane fans timestamped object deletion and directed interaction services to FQR/TARA/NMRA subscribers before their grants, preserves removal/direct-target/time/order metadata and terminal `MessageCanNoLongerBeRetracted`, and proves an overlapping regional timestamped update can be retracted while all three grant requests remain pending without reflection or `requestRetraction`; a matching regional object update now arrives before FQR/TARA/NMRA available-advance grants with the conveyed source region, timestamp/order metadata, and shared retraction handle; it also decodes nonregional and region-context timestamped `SendInteraction` MOM reports' logical-time, region-set, and message-retraction arguments through the standard Java `EncoderFactory`, and verifies terminal/malformed retraction failures are C++-originated `HLAreportException` records | Implemented; native regional timestamp callback is now exercised end-to-end |
| Basic declaration management | A native two-federate scenario publishes/subscribes object attributes and interaction classes, then receives typed start/stop and interaction on/off advisories; Java creates its `AttributeHandleSet` and `InteractionClassHandleSet` via the selected RTI's standard factories and exercises exact passive/universal overloads alongside the normalized Python flags; the external JNI vector also drives invalid object/interaction, directed-declaration, and regional interaction subscription handles and decodes each C++ failure as `HLAreportException` through Java and JPype | Implemented |
| Receive-order object management | Native and real-JVM Restaurant FOM scenarios reserve, callback-confirm, and release a single name as well as batch names, register a named object, prove typed discovery and name/handle lookup from the subscribing federate, reflect an opaque attribute-value map with tag, transport, and producer, then delete it and verify the typed removal callback and state transition; native local-delete ownership preconditions map to `FederateOwnsAttributes`, while Java fake/JVM fixtures exercise successful local removal and matching set/map factory, service, and callback conversions; the external IEEE-JAR JNI matrix additionally preserves `NotConnected`, `FederateNotExecutionMember`, `IllegalName`, `NameSetWasEmpty`, `ObjectInstanceNameNotReserved`, asynchronous single-name contention failure, and partial batch success/failure callbacks, with synchronous reservation/release failures independently decoded as C++ `HLAreportException` through Java and JPype; the real JVM fixture also fans one registration/update out to two independent subscribers; the external IEEE-JAR JNI vector proves C++ automatic provision queues the standard empty-tag `provideAttributeValueUpdate` callback after discovery through the Java `String[]` FOM-create overload and that the standard `HLAsetSwitches` interaction toggles later discovery-triggered callbacks for both members | Implemented; timestamped update/reflection and regional multi-recipient routing remain in the dedicated DDM rows |
| Receive-order interaction management | A native two-federate Restaurant FOM scenario publishes/subscribes a parameterized interaction, sends an opaque parameter-value map, and verifies typed interaction, transport, producer, tag, and value conversion at the subscribed federate; Java exercises its standard map factory, send service, and callback conversion through the JVM fixture, including two-subscriber fanout; the external IEEE-JAR lane also decodes receive-order regional `SendInteraction` reporting, including the standard region-set argument | Implemented; timestamped interaction is covered in the dedicated timestamped-service row |
| Logical time management | Native C++ and Java adapter tests obtain the provider time factory, round-trip integer and floating time/interval encodings, delegate factory construction/decode and add/subtract/difference through the provider implementation, verify smallest-subnormal and largest-finite floating arithmetic, map implementation mismatch and overflow/underflow exceptions, map truncated/trailing/negative/non-finite integer and floating encodings to typed `CouldNotDecode`, exercise an epsilon-sized floating time advance/grant, enable/disable regulation and constrained mode, toggle asynchronous delivery, query/modify lookahead, exercise time-advance/next-message/flush request variants, query GALT/LITS with valid and undefined results, request grants, query logical time, and convert regulation/constrained/grant/`flushQueueGrant` plus timed object/interaction callbacks; restore tests also prove saved logical time, actual lookahead, and deferred decreases are reinstated | Provider-neutral integer/floating boundaries implemented; native pybind, fake adapter, and real-JVM fixture arithmetic and malformed-decode tests pass; the external IEEE-JAR JNI vector additionally proves C++-owned float64 arithmetic at representable and initial/final boundaries, exact and one-step-overflow additions at the largest finite value, symmetric exact and one-step-overflow additions at signed `Long.MAX_VALUE`, integer64 precision above 2^53, direct standard Java factory construction/decode and interval arithmetic, standard Java carrier predicates/comparison/equality/hash-code, exact Java short-payload `IllegalArgumentException` versus C++ negative/non-finite `CouldNotDecode`, trailing-byte offset behavior, and typed `IllegalTimeArithmetic` mapping; the C++ report boundary now preserves `SaveInProgress` for `Query Logical Time`, `Query GALT`, and `Query LITS` and decodes each separate failure as an `HLAreportException` through Java and JPype |
| Region substrate | Native C++ and Java adapter tests create a typed region, round-trip its dimension set, set/query range bounds, commit, and delete it through the provider boundary; the external IEEE-JAR JNI vector preserves typed `InvalidRegion`, `RegionDoesNotContainSpecifiedDimension`, and `InvalidRangeBound` for incomplete regions, unrelated dimensions, invalid bounds, and repeated deletion, plus `InvalidRegion` for foreign read-only lookups and `RegionNotCreatedByThisFederate` for foreign mutating set/commit/delete calls; the same vector now decodes the C++ create/lifecycle/range failures, including unknown-dimension `Create Region`, as standard `HLAreportException` interactions through Java `EncoderFactory` and JPype | Implemented |
| Regional interaction management | Native C++ verifies overlapping versus disjoint regional recipients on receive-order sends, timestamp-order sends, retraction routing, and conveyed region designators, plus existing-subscription reprojection as multiple overlapping regions are added/partially removed without duplicate delivery, and exact-boundary Allow Relaxed DDM; Java fake and real-JVM fixtures forward regional subscription/send calls and convert optional sent-region and timed callbacks, including the same bounded relaxed-DDM boundary policy; the external IEEE Java-API/JNI lane now drives a three-federate overlap/disjoint fanout through the official region/set carriers and preserves `RegionNotCreatedByThisFederate`, `InvalidRegion`, and `InvalidRegionContext` for foreign, uncommitted, and wrong-dimension subscription/send regions; dedicated report vectors enable C++ exception reporting on both originating federates and decode seven separate regional subscription/send/unsubscription `HLAreportException` records plus the region-lifetime `Delete Region` `HLAreportException` through the official Java encoder and JPype; both providers now re-evaluate an explicit regional subscription at the callback boundary when delayed subscription evaluation is enabled, while the real-JVM fixture also applies range-overlap filtering before fanning a timestamped regional interaction with mixed convey settings; the external Java lane now exercises the three-stage regional timestamped interaction sequence (pre-grant retraction, post-grant delivery, and conveyed source-region metadata); a dedicated external two-dimensional matrix requires overlap in both dimensions and then proves independent X-only and Y-only recipient transitions after source-range mutation; the external IEEE-JAR JNI lane also proves timestamped default-region interaction delivery with the standard empty `RegionHandleSet`, `TIMESTAMP` sent/received order metadata, and consumed retraction handle, plus receive-order and timestamped region-context `HLAreportServiceInvocation` payloads | Implemented |
| Directed interaction management | Native C++ verifies object-class directed declaration and receive-order delivery to a registered target object, plus three-federate timestamped fanout/retraction behavior; Java fake and real-JVM fixtures forward directed declaration overloads, receive-order/timestamped sends, typed target/source/transport callbacks, and retraction metadata; the real-JVM fixture now exercises multi-recipient directed fanout with a constrained pending recipient | Implemented |
| Regional object management | Native C++ verifies Java-shaped attribute-set/region-set pair vectors through regional registration, discovery, update, association/unassociation, and regional value request, including mixed-fanout timestamped reflection with the convey-region-designator switch, multi-region selective-unassociation/default-region behavior, independent per-attribute region association with one attribute retained while another becomes disjoint, existing-object discovery reprojection when a subscription gains multiple overlapping regions without duplicate discovery, scope-advisory out/in transitions across partial and complete regional unsubscription and re-subscription, and a receive-order two-subscriber recipient-isolation transition where source-region association brings only a previously out-of-scope known recipient back into scope; default-region callback realization and exact-boundary relaxed DDM are covered; native and real-JVM tests now re-evaluate a regional object update at the callback boundary after a committed range change when delayed subscription evaluation is enabled; Java fake callback conversion covers present/absent region metadata and the real-JVM fixture carries per-attribute registration/association/unassociation region sets into region-relevant discovery and timestamped multi-recipient reflection, broadcasts association scope changes to the affected recipient only, conveys an empty `RegionHandleSet` for receive-order and timestamped default-region callbacks while preserving `None` for ordinary non-regional callbacks, filters disjoint ranges before delivery, routes regional value requests only to the owning publisher on overlap while suppressing a disjoint requester and preserving the copied request tag, proves the standard Java provide callback can answer a regional request and return a region-conveyed reflection with copied values/tag/transport/producer metadata, registers an object while passive regional subscription is active and proves discovery remains suppressed until the declaration is replaced by an active subscription, exercises exact-boundary relaxed DDM updates; the external IEEE-JAR JNI vector additionally keeps two known subscribers isolated as source associations are added and selectively removed, preserving typed `attributesInScope`/`attributesOutOfScope` callbacks and default-region fallback delivery; the same lane now delivers an associated timestamped object update before FQR/TARA/NMRA available-advance grants with source-region conveyance, timestamp/order metadata, and one shared retraction handle; a two-dimensional external object vector first establishes known-object discovery, then filters regional reflections through independent X-only and Y-only source-range mutations; the same vector decodes C++ `HLAreportException` records for invalid foreign, uncommitted, wrong-context, and unknown-object/region regional registration, association/unassociation, and subscription teardown calls, plus the live-registration `Delete Region` `RegionInUseForUpdateOrSubscription` report through an observer federate | Implemented; broader regional association matrices remain follow-on |
| Attribute value update requests | Native C++ and fake/real-JVM fixtures exercise both class- and instance-targeted requests and verify copied request tags plus typed `provideAttributeValueUpdate` callback attributes | Implemented |
| Object-management advisory callbacks | Native C++ regional scope transitions and fake/real-JVM fixtures convert scope entry/exit plus plain and named-rate per-object update-relevance callbacks | Implemented |
| Order and transportation management | Native C++ and fake/real-JVM fixtures invoke object/interaction order changes, attribute/interaction transportation changes and queries, and convert typed confirmation/report callbacks | Implemented |
| Attribute ownership management | Native C++ and real-JVM fixtures query a registered object's attribute ownership, execute unconditional/negotiated divestiture and acquisition transitions, exercise confirmation/cancellation/release-denied/divestiture-if-wanted services, and convert typed owner callbacks; Java fake runtime verifies encoded handle forwarding and service tags; the callback proxy test covers all nine Java ownership callback conversions with copied tags and typed domains; native and real-JVM two-member scenarios prove confirmation ordering, transfer tags, release-denied state preservation, denial-tagged unavailable callbacks, the if-available/already-owned unavailable path, both regular and if-available federate-owned self-acquisition preconditions, overlapping pending-acquisition rejection, confirm-without-request and duplicate-divestiture typed errors, no-pending cancellation errors, release/reacquisition, pending cancellation, and `AttributeNotOwned` through release-denied, ordinary update/order/transport services, and all three divestiture forms after transfer; the external IEEE-JAR route also proves `FederateOwnsAttributes` on `NO_ACTION` resignation, typed ownership assumption/acquisition after `UNCONDITIONALLY_DIVEST_ATTRIBUTES`, deferred assumption search after a known candidate publishes later, and `OwnershipAcquisitionPending` on a pending-acquisition resignation until `CANCEL_THEN_DELETE_THEN_DIVEST` is selected; mixed external `If Available` and regular acquisition requests split one C++ plan into typed secured/unavailable and acquirer/owner release callbacks for unowned versus remote-owned attributes while preserving copied tags and handles; the mixed query vector also separates owner-report and not-owned callbacks from one standard request | Implemented; broader provider edge cases remain |
| Advisory/reporting support switches | Native C++ and fake/real-JVM Java fixtures toggle and read relevance, automatic-resign, service/exception-reporting, and provider support switches through the shared Python contract; the external IEEE-JAR JNI lane also verifies C++ service-report JSON output after Java-routed time-regulation services, both standard MOM report-service declaration conflicts (`FederateServiceInvocationsAreBeingReportedViaMOM` and `ReportServiceInvocationsAreSubscribed`), and duplicate/initial-state Java `enableTimeRegulation`/`enableTimeConstrained`/`enableAsynchronousDelivery`/`disableAsynchronousDelivery`/`disableTimeRegulation`/`disableTimeConstrained` plus unregulated `queryLookahead`/`modifyLookahead` and pending `timeAdvanceRequest`/`timeAdvanceRequestAvailable`/`nextMessageRequest`/`nextMessageRequestAvailable` conflicts preserving their typed exceptions while delivering separate `HLAreportException` callbacks | Implemented; broader regional association and vendor-specific floating-time arithmetic matrices remain |
| C++ `VariableLengthData` value semantics | Provider-private conversion to the future portable Python `bytes` boundary; do not expose C++ pointer ownership | Deferred |

All 100 concrete exception names in the 2025 Java exception package are
available as Python subclasses of `RTIexception`. Provider boundaries map a
known C++ or Java simple name to its specific Python type; individual services
are still added and tested only when bound.

Federation management, logical time, handles, encoding, and the remaining
callback families receive their own corresponding conformance mixins only once
their public Python value types and provider bindings exist.

The exact IEEE-JAR JNI lane also sends empty-region pair-list subscriptions,
source associations, and matching teardown through the standard Java factories;
the C++ empty-region no-op behavior is asserted from Python. The same lane
invokes the exact named-rate regional subscription overload and verifies the
C++ scope-advisory callbacks, including the `High` update-rate designator.
The ownership-transfer vector additionally verifies C++ association reset at
the ownership boundary and the new owner's default-then-explicit region
metadata through the Java/JNI callback route.
The mixed-declaration vector additionally verifies that a retained ordinary
subscription is suppressed by a disjoint explicit regional declaration and
reprojected through the derived default region when that declaration is removed,
preserving empty conveyed-region metadata for both recipients.
The passive-subscription vector independently verifies that ordinary and
regional declarations remain non-delivering until an active replacement, then
reproject existing objects and deliver later updates through the standard Java
callbacks.
The queued-association vector additionally verifies that a timestamped update
accepted under source region A is not retargeted when the association is replaced
with region B before the callback boundary; only the later update carries B and
its exact standard retraction metadata.
The suppressed-timestamp vector also verifies that a queued regional recipient
becoming disjoint before delivery consumes the recipient ledger, so no reflection
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
source regions, isolates recipient delivery and conveyed-region metadata as
associations are removed, verifies default-region fallback after the final
explicit association is removed, and restores one source association alone.
It also repeats the complete source association and verifies the additive C++
association ledger is idempotent: each Java recipient receives one reflection
with the unchanged source-region set.
The multi-source regional-interaction matrix sends one region set through two
independently moving source ranges and proves lower-only, upper-only, both, and
fully disjoint recipient transitions through the standard Java region carrier.
The timestamped-save/NMRA vector proves the exact Java
`nextMessageRequestAvailable` boundary as well: an equal first grant delivers
the queued TSO without admitting the save, while a later strict grant invokes
`initiateFederateSave` before its matching time-advance grant.
The companion timestamped-save/TARA vector proves the same strict boundary
without a queued message: equality leaves the save pending, and the later
Available grant invokes `initiateFederateSave` before its grant callback.
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
The immediate/evoked scope vector verifies synchronous versus queued
`attributesInScope`/`attributesOutOfScope` delivery, stale-transition
suppression, and advisory-switch gating.

## Current provider runs

The native provider combines the mixin with its pybind11 wheel test:

~~~text
NativeConnectionFoundationConformanceTest
    -> UmbraRtiFactory
    -> pybind11
    -> Umbra C++ RTI
~~~

The provider-parity gate is deliberately shared by both routes:

~~~text
ProviderBindingParityConformanceMixin
    -> UmbraRtiFactory (pybind11 -> C++)
    -> JavaRtiFactory (JPype -> exact hla.rti1516_2025 Java interfaces)
~~~

The mock Java vendor adapter combines the same mixin with a real JVM fixture:

~~~text
MockVendorJPypeIntegrationTest
    -> MockJavaRtiFactory
    -> JPype
    -> Java ServiceLoader / MockRtiFactory
~~~

The opt-in JNI verification lane uses the same JPype adapter but makes its
connection and federation-reporting slice reach the real C++ implementation:

~~~text
JPypeJniIntegrationTest
    -> NativeRtiFactory (Java ServiceLoader)
    -> JNI
    -> Umbra C++ RTI
    -> Java callback proxy
    -> Python FederateAmbassador
~~~

It is intentionally not counted as a replacement Java provider. It verifies
the C++/Java/Python bridge mechanics while the mock and future vendor JAR lanes
continue to verify Java API compatibility independently.

For applications that want the C++-backed Java provider through normal Python
factory discovery, `packages/umbra-rti-jni-python` registers the `umbra-jni`
entry-point alias. It only assembles the independently supplied IEEE API JAR,
Umbra bridge JAR, and native library before delegating to the generic JPype
adapter; the exact Java `RtiFactoryFactory`/`ServiceLoader` remains the
provider-selection boundary.

The Java fixture queues both a `connectionLost` event and an entire restore
lifecycle, including failure and abort. Its test therefore proves actual
Java-to-Python callback conversion, not merely method invocation. Both
provider runs include the overload mixin, so they exercise the C++ and Java
connection dispatches rather than only their Python method signatures.

## Adding a provider

An adapter package adds a short test class:

~~~python
import unittest

from hla.rti1516_2025.testing import (
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    ProviderBindingParityConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    FederationExecutionMemberDiscoveryConformanceMixin,
)


class VendorConnectionConformance(
    ConnectionFoundationConformanceMixin,
    ConnectionOverloadConformanceMixin,
    FederationExecutionDiscoveryConformanceMixin,
    FederationExecutionMemberDiscoveryConformanceMixin,
    unittest.TestCase,
):
    def make_factory(self):
        return VendorRtiFactory(configured_jar)
~~~

The provider's normal integration test configures any native dependencies,
JARs, or broker endpoint before the mixin runs. This keeps the behavioral
contract the same while preserving provider-specific setup.

## Running the current suites

The pure API and Java adapter unit suites require no Java vendor JAR:

~~~powershell
$env:PYTHONPATH = 'packages/umbra-rti-api/src;packages/umbra-rti-jpype/src'
python -m unittest discover -s packages/umbra-rti-api/tests
python -m unittest discover -s packages/umbra-rti-jpype/tests
~~~

The native conformance suite runs against an installed `umbra-rti-native`
wheel plus the API source. The mock vendor's real-JVM suite additionally needs
`JPype1`, a JDK, and its generated fixture JAR; its test builds that fixture in
a temporary directory automatically. These setup requirements are deliberate:
they ensure the test exercises the same packaging and bridge boundaries that
an application uses.

The JNI integration test is opt-in because it builds the embedded C++ profile
and starts a fresh JVM with an absolute native-library property:

~~~powershell
$env:UMBRA_ENABLE_JNI_INTEGRATION_TESTS = '1'
$env:UMBRA_JNI_REQUIRE_RUNTIME_SERVICE_COVERAGE = '1'
python -m unittest packages/umbra-rti-jpype/tests/test_jpype_jni_integration.py
~~~

The runtime-service flag wraps the concrete JPype façade only for accounting
and requires the complete external suite to execute all 184 shared
`RTIambassador` services. It complements the static Java name/arity,
JNI endpoint-registration, and native-handle-only façade-state checks with
actual Python → Java → JNI → C++ call evidence.

The external IEEE-JAR ownership vector also preserves `AttributeNotOwned`
through release-denied, ordinary update/order/transport services, and
unconditional, if-wanted, and negotiated divestiture calls made by the former
owner after a real C++-backed transfer.

The external IEEE-JAR regional-object vector also maps deletion of a source
region that remains referenced by an object registration to the standard
`RegionInUseForUpdateOrSubscription` exception.
The companion regional-interaction vector keeps the same typed exception while
a regional subscription is active, then proves standard unsubscription makes
the region deletable.

The external logical-time vector also invokes the standard Java carrier
`add`, `subtract`, and `distance` methods directly. Positive distances and
round trips are validated, while a negative distance retains the C++
`IllegalTimeArithmetic` cause inside Java's standard
`UndeclaredThrowableException` proxy wrapper before the Python adapter maps it.
It also obtains integer and floating carriers from separate C++-backed
providers and preserves typed `InvalidLogicalTime`/`InvalidLookahead` rejection
for mismatched time and interval inputs before encoding.

The external lifecycle vector also calls the exact standard Java federation
creation, create-with-MIM, destroy, join, and resign overloads before
connection and preserves the C++ `NotConnected` exception type through JNI.
The same lifecycle route rejects an inconsistent logical-time FOM before the
C++ federation registry reserves its name.
The transport-loss vector injects a deterministic fault only through the
embedded test control (not the standard Java API), then observes the exact
standard `connectionLost` callback, surviving-member cleanup, and typed
`NotConnected` behavior through the JPype route; the C++ `ConnectionLost`
service-report record and fault-description argument are visible before callback
eviction. An HLA_IMMEDIATE companion proves synchronous delivery of the same
standard callback and membership transition.
The companion MOM vector subscribes to the standard `HLAreportFederateLost`
interaction and decodes its four C++-encoded parameters through the Java
`EncoderFactory` before JPype exposes the callback to Python.
The joined-federate MOM vector additionally subscribes to the standard
`HLAobjectRoot.HLAmanager.HLAfederate` object class, receives the RTI-owned
discovery and initial `HLAreportServiceFile` reflection, requests the same
value through the standard attribute-value-update service, and observes the
typed removal callback when the subject resigns. Both the invalid producer
handle and reliable transport remain C++-originated values through JNI and
JPype.
The exception-report vectors enable C++ exception reporting, preserve the
typed `InteractionClassNotPublished` raised by standard Java `sendInteraction`,
`sendInteractionWithTime`, `sendDirectedInteraction`, and
`sendInteractionWithRegions`, and `sendInteractionWithRegionsWithTime`, and decode the separate
`HLAreportException`
service/exception parameters through the Java encoder factory.
The declaration-management companion also enables both C++-owned reporting
switches, invokes the standard Java `subscribeInteractionClass` conflict for
`HLAreportServiceInvocation`, preserves the typed
`FederateServiceInvocationsAreBeingReportedViaMOM`, and decodes its separate
`HLAreportException` callback through the Java encoder factory.
The timestamped attribute update-rate vector builds a FOM with the standard
`Low` update-rate designator, sends reliable and best-effort attributes at
successive logical times, and proves that reliable delivery continues while
the best-effort passel is reduced by the C++ admission gate. A suppressed
timestamped message still consumes its C++ recipient/retraction ledger and
maps the second retract to `MessageCanNoLongerBeRetracted` through Java and
JPype.
The complementary switch vector subscribes the report stream first, then
preserves `ReportServiceInvocationsAreSubscribed` from the standard Java
`setServiceReportingSwitch(true)` call and decodes its separate exception
report through the same Java encoder path.
The support-switch exception vector holds a save open and invokes all 23
advisory, automatic-resign, reporting, file-reporting, auto-provide, delayed
subscription, known-class, relaxed-DDM, and non-regulated-grant services;
each typed `SaveInProgress` failure is decoded as a separate
`HLAreportException` through Java and JPype.
The time-management companion enables regulation through the standard Java
`enableTimeRegulation` call, then preserves the typed
`TimeRegulationAlreadyEnabled` from a duplicate request and decodes its
separate `HLAreportException` callback through the same Java encoder path.
The pending-regulation companion invokes standard Java
`enableTimeRegulation` twice before the first callback, preserves
`RequestForTimeRegulationPending`, and decodes its separate exception report.
Its constrained-mode companion performs the same duplicate-call check for
`enableTimeConstrained`, preserving `TimeConstrainedAlreadyEnabled` and
decoding a separate exception report.
The pending-constrained companion invokes standard Java
`enableTimeConstrained` twice before the first callback, preserves
`RequestForTimeConstrainedPending`, and decodes its separate exception report.
Its asynchronous-delivery companion performs the same duplicate-call check for
`enableAsynchronousDelivery`, preserving
`AsynchronousDeliveryAlreadyEnabled` and decoding a separate exception report.
Its inverse-state companion invokes `disableAsynchronousDelivery` while the
switch is off, preserving `AsynchronousDeliveryAlreadyDisabled` and decoding a
separate exception report.
The inverse time-role companions invoke `disableTimeRegulation` and
`disableTimeConstrained` while their modes are off, preserving
`TimeRegulationIsNotEnabled`/`TimeConstrainedIsNotEnabled` and decoding
separate exception reports.
The unregulated lookahead companions invoke `queryLookahead` and
`modifyLookahead` through the standard Java API, preserve
`TimeRegulationIsNotEnabled`, and decode separate exception reports.
The pending-lookahead companion invokes standard Java `modifyLookahead` while
a time advance is pending, preserves `InTimeAdvancingState`, and decodes its
separate exception report.
The pending-request companion invokes `timeAdvanceRequest` twice before the
first grant callback, preserves `InTimeAdvancingState`, and decodes a separate
exception report.
The regulation-pending companion invokes standard Java `timeAdvanceRequest`
and `timeAdvanceRequestAvailable` while `enableTimeRegulation` awaits its
callback, preserves `RequestForTimeRegulationPending`, and decodes separate
reports retaining each overload's service label.
The constrained-pending companion invokes standard Java `timeAdvanceRequest`
and `timeAdvanceRequestAvailable` while `enableTimeConstrained` awaits its
callback, preserves `RequestForTimeConstrainedPending`, and decodes separate
reports retaining each overload's service label.
The overload companion invokes `timeAdvanceRequestAvailable` twice before the
first grant callback, preserves `InTimeAdvancingState`, and decodes its
separately named exception report.
The next-message companion invokes `nextMessageRequest` twice before the first
grant callback, preserves `InTimeAdvancingState`, and decodes its separate
exception report.
The overload companion invokes `nextMessageRequestAvailable` twice before the
first grant callback, preserves `InTimeAdvancingState`, and decodes its
separately named exception report.
The flush-queue companion invokes `flushQueueRequest` twice before the first
grant callback, preserves `InTimeAdvancingState`, and decodes its distinct
`Flush Queue Request` exception report.
Its automatic-resign companion sets the standard Java `DELETE_OBJECTS`
directive, then proves C++ transport-loss cleanup reaches the survivor as the
exact `removeObjectInstance` callback and removes the departed object name.
The `UNCONDITIONALLY_DIVEST_ATTRIBUTES` companion instead retains the object
and delivers the typed ownership-assumption callback, including the delete
privilege, to the surviving member.
The `DELETE_OBJECTS_THEN_DIVEST` companion covers both sides of that standard
directive in one C++-backed loss: a lost-member-owned object is removed while a
previously transferred object remains and produces exactly one typed ownership-
assumption callback for the survivor.
The companion normal-resignation vector invokes the standard Java
`resignFederationExecution` service with the same directive, independently of
the transport-fault control, and verifies the same C++ deletion/divestiture
callbacks and survivor state.
The forced-resignation vector uses a distinct internal RTI control input and
observes the exact standard `federateResigned` callback, survivor membership
cleanup, duplicate-control rejection, and successful rejoin on the still-live
Java/C++ connection. Its HLA_IMMEDIATE companion proves synchronous callback
delivery and the same survivor cleanup/rejoin behavior.

The external IEEE-JAR regional-object vector also preserves
`RegionNotCreatedByThisFederate`, `InvalidRegion`, and `InvalidRegionContext`
for foreign, uncommitted, and wrong-dimension pair lists through registration,
subscription, and association services.

The same external route proves the regional attribute-value request empty-set
no-op, disjoint filtering, later overlap admission, and typed validation errors
through `requestAttributeValueUpdateWithRegions`.

The external IEEE-JAR regional request/response vector also invokes the standard
`provideAttributeValueUpdate` callback from Python, answers through the same
Java-routed ambassador, and verifies copied values, request/response tags,
transport, producing federate, and conveyed region metadata on reflection.
