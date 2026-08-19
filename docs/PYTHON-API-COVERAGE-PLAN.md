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
   its result or callback into the same public Python types;
4. provider-neutral contract checks plus a real native integration test; and
5. a JVM fixture/integration test whenever the Java fixture implements that
   service.

An unavailable operation must be absent from the shared API, or raise the
standard typed exception from an actual provider call. It is never counted
merely because a Python stub or mock can accept its arguments.

## Current completed verticals

| Family | Public services | Public callbacks/value boundary | Status |
| --- | --- | --- | --- |
| Connection foundation | `connect` forms, disconnect, callback control/evocation | `ConfigurationResult`, credentials/configuration, `connectionLost` | Native and Java foundation covered |
| Federation discovery | scalar/vector FOM create, create-with-MIM, destroy/list federation, list members | typed immutable federation/member reports and missing-federation callback | Native and Java covered, including Java `URL[]`/`URL` mappings |
| Membership | named/unnamed join with optional additional FOM modules; resign | encoded immutable `FederateHandle`, `ResignAction`, and `federateResigned` reason callback | Native and Java adapter seams cover scalar and `URL[]`/C++ vector forms, including the RTI-originated resignation callback |
| Synchronization points | register for current members or an explicit federate-handle set; achieve | registration success/failure, announcement bytes, completion `FederateHandleSet` | Native and real-JVM fixture lifecycles cover both registration overloads |
| Save status discovery | query federation save status | ordered immutable `FederateHandleSaveStatusPair` response with typed `SaveStatus` | Native two-member integration and real-JVM fixture status callbacks cover no-save, active-saving, and post-completion transitions |
| Scalar save success | request, begun, complete | `initiateFederateSave`, `federationSaved` | Native two-member lifecycle and real-JVM fixture lifecycle covered; the real-JVM fixture now coordinates initiation and completion across two members and verifies the federation-wide completion barrier |
| Timestamped save request | request with provider `LogicalTime` | provider-created timestamp crosses the native decode boundary and Java overload; initiation is held until the fixture's time boundary | Native C++ timed-save admission covers immediate and constrained two-federate grant boundaries, an in-transit timestamped interaction delivered before save admission, and a re-entrant save requested from the TSO callback (initiation waits for callback return); Java fake/real-JVM overloads cover pending status and the same receive -> initiate-save -> grant sequencing |
| Scalar save failure/abort | not-complete; abort | typed `federationNotSaved` `SaveFailureReason` | Native two-member failure/abort and real-JVM fixture failure/abort paths covered; the real-JVM fixture now broadcasts federate-reported failure and abort outcomes across the shared save transaction |
| Restore status discovery | query federation restore status | ordered immutable `FederateRestoreStatus` response with typed handles and `RestoreStatus` | Native two-member and real-JVM fixture status callbacks cover no-restore, active-restoring, and post-completion transitions |
| Scalar restore success | request; complete | request acceptance/rejection, begin, per-federate initiation with a typed handle, `federationRestored` | Native two-member and real-JVM fixture lifecycles covered; real-JVM restore now coordinates per-member completion and rewinds distinct saved member times, while native restore also rewinds lookahead and reinstates a live timestamped interaction/retraction ledger |
| Scalar restore failure/abort | not-complete; abort | typed `federationNotRestored` `RestoreFailureReason` | Native two-member and real-JVM fixture failure/abort paths covered; the real-JVM fixture now broadcasts federate-reported failure and abort outcomes across the shared restore transaction |
| Basic encoding | `getEncoderFactory`; four scalar factory forms | provider-owned `DataElement`: `HLAinteger32BE`, `HLAunsignedInteger32BE`, `HLAboolean`, `HLAunicodeString`; typed encoder/decoder errors | Native C++ and real-JVM fixture vectors covered, including UTF-16 element-count Unicode vectors |
| Support name/handle lookup | federate, object class, attribute, interaction class, parameter, transportation type, and dimension `get*Handle` / `get*Name` pairs plus known object-class lookup | distinct immutable encoded public handle domains | Native Restaurant FOM and fake/real-JVM fixture round trips covered |
| Handle, set, and map factories | Java-declared handle decoder accessors, federate/dimension/region/attribute set factories, and attribute/parameter value-map factories | provider-backed handle decoders plus mutable Java-shaped builders that are accepted by service calls and copied at the boundary | Native C++ codec/RTI decoders and fake/real-JVM Java factory lifecycles covered; message-retraction decoding remains an internal provider boundary because Java exposes no public factory |
| C++-only codec/reporting services | C++ `decode*Handle` methods and `setSendServiceReportsToFileSwitch` | Java-shaped Python uses the corresponding handle factories; no Java `MessageRetractionHandle` decoder or send-report-file setter exists in the 1516.1 Java surface | Deliberately kept out of the shared contract and recorded as an edition-specific boundary rather than exposing a provider-only Python method |
| Support value lookups | update-rate value/designator queries, order-type/name conversion, available dimensions for object/interaction classes, and dimension upper bounds | typed `OrderType`, immutable `DimensionHandleSet`, and provider numeric values | Native Restaurant FOM and fake/real-JVM Java adapter mappings covered |
| Support normalization | service-group and federate/object-class/interaction-class/object-instance normalization coordinates | typed `ServiceGroup` plus strict handle-domain validation and integer provider coordinates | Native C++ and fake/real-JVM Java adapter mappings covered |
| Timestamped object/interaction services | timestamped update, interaction send (regional and non-regional), object deletion, and message retraction | provider-created `LogicalTime` input and typed immutable `MessageRetractionHandle` results; timed reflect/interaction/removal metadata, copied region designators, and `requestRetraction` callback | Native C++ and fake/real-JVM Java service/retraction mappings plus native/real-JVM timed callback conversion covered; the real-JVM fixture now fans timestamped object and interaction traffic to unconstrained and constrained subscribers, preserves RECEIVE versus TIMESTAMP order, routes retractions only to delivered recipients, and covers regional interaction/object designator convey on mixed recipients; native restore reinstates a live TSO retraction ledger |
| Basic declaration management | object-class attribute and interaction publish/unpublish/active-or-passive subscribe forms | immutable `AttributeHandleSet`; typed object/interaction relevance advisories | Native two-federate lifecycle and real-JVM fixture covered |
| Receive-order object management | reserve/release single or multiple object-instance names; named or provider-named registration; object-instance name/handle lookup; receive-order and local deletion plus attribute update | immutable `ObjectInstanceHandle`/`ObjectInstanceNameSet` and byte-copying `AttributeHandleValueMap`; discovery, removal, reflection, and single/batch reservation callbacks with typed source/transport handles; local-delete preconditions map to typed ownership exceptions | Native two-federate Restaurant FOM lifecycle and fake/real-JVM fixtures covered; real-JVM fixture now fans one registration/update out to two independent subscribers; timestamped update/reflection is covered in the dedicated DDM rows |
| Receive-order interaction management | `sendInteraction` with published parameters | byte-copying `ParameterHandleValueMap`; interaction receipt with typed source/transport handles | Native two-federate Restaurant FOM lifecycle and real-JVM fixture covered; real-JVM fixture now fans one send out to two independent subscribers; timestamped interaction is covered in the dedicated timestamped-service row |
| Logical time management | provider `LogicalTimeFactory`, initial/final/zero/epsilon values, encode/decode, provider-owned add/subtract/difference arithmetic, regulation/constrained mode, asynchronous delivery, lookahead query/modification, all five time-advance/queue request variants, GALT/LITS queries, and timestamped service inputs | immutable provider-created `HLAinteger64Time`/`HLAinteger64Interval` and `HLAfloat64Time`/`HLAfloat64Interval` snapshots, Java-shaped `TimeQueryResult`, and typed regulation/constrained/grant/`flushQueueGrant` plus timed object/interaction callbacks | Native C++ and fake/real-JVM Java adapter lifecycles cover integer time; native and real-JVM restore tests preserve saved logical time, actual lookahead, and deferred lookahead decreases; the C++ and Java providers now delegate add/subtract/difference to their native logical-time implementations and preserve exact floating values through factory, decode, arithmetic, query, grant, and timed callbacks, including smallest-positive-subnormal epsilon parity and an epsilon-sized floating time-advance/grant; broader overflow and cross-implementation arithmetic cases remain |
| Region substrate | `createRegion`, `commitRegionModifications`, `deleteRegion`, dimension-set and range-bound services | immutable `RegionHandle`/dimension and region sets plus mutable Java-shaped `RangeBounds` | Native C++ and fake/real-JVM Java adapter lifecycles covered |
| Regional interaction management | regional subscribe/unsubscribe, regional receive/timestamp-order send, and convey-region-designator switch | optional `RegionHandleSet` plus timed callback metadata on interaction callbacks | Native C++ overlap filtering and fake/real-JVM Java adapter callback mappings covered; both providers now reproject an existing subscription as an overlapping region is added and suppress delivery again when that region is removed, re-evaluate explicit regional subscriptions at the callback boundary when delayed subscription evaluation is enabled, preserve mixed region-designator convey, and honor the bounded Allow Relaxed DDM exact-boundary policy |
| Directed interaction management | object-class directed publish/unpublish and subscribe/unsubscribe overloads; receive-order and timestamped directed sends | immutable `InteractionClassHandleSet`, typed target object/source/transport callback payloads, and optional time/order/retraction metadata | Native C++ receive-order delivery and three-federate timestamped retraction fanout prove only delivered recipients receive `requestRetraction`; fake/real-JVM Java declaration, send, and timed callback mappings are covered, including real-JVM multi-recipient directed fanout and constrained pending retraction |
| Regional object management | Java-shaped `AttributeSetRegionSetPairList`; regional object registration, regional subscription, association/unassociation, and regional value requests | immutable attribute-set/region-set pair values; C++ typedef aliases remain available | Native C++ overlap/discovery/update lifecycle includes mixed-fanout timestamped reflection and convey-switch behavior plus per-attribute multi-region association, selective unassociation, default-region fallback, existing-object discovery reprojection as a subscription gains an overlapping region, scope-advisory out/in transitions across regional unsubscription and re-subscription, default-region callback realization, and exact-boundary relaxed DDM; both providers now re-evaluate explicit regional object updates at the callback boundary when delayed subscription evaluation is enabled; fake Java callback conversion covers present/absent region metadata, and the real-JVM adapter carries per-attribute registration/association/unassociation region sets into region-relevant discovery and timestamped multi-recipient reflection, conveys an empty `RegionHandleSet` for receive-order and timestamped default-region realizations (while ordinary non-regional callbacks remain `None`), filters disjoint ranges before delivery, honors passive regional subscriptions until reactivated, and exercises exact-boundary relaxed DDM updates |
| Attribute value update requests | class- and instance-targeted `requestAttributeValueUpdate` overloads | typed class/instance handle dispatch, immutable `AttributeHandleSet`, copied request tags, and `provideAttributeValueUpdate` callback | Native C++ and fake/real-JVM Java adapter mappings covered, including callback tag/attribute conversion |
| Object-management advisory callbacks | scope entry/exit and per-object update relevance (plain and named-rate forms) | typed `ObjectInstanceHandle`/`AttributeHandleSet` callback payloads with optional update-rate designator | Native C++ regional scope transitions and fake/real-JVM Java callback fixtures covered |
| Order and transportation management | `changeAttributeOrderType`, default-attribute and interaction order changes; attribute/interaction transportation change and query services | strict `OrderType`/`TransportationTypeHandle` domains plus typed confirmation/report callbacks | Native C++ and fake/real-JVM Java adapter mappings covered |
| Attribute ownership management | `queryAttributeOwnership`, ownership status, unconditional and negotiated divestiture, divestiture confirmation/cancellation, regular/if-available acquisition, acquisition cancellation, release-denied, and divestiture-if-wanted services | typed ownership callbacks for assumption, divestiture confirmation, acquisition/unavailability/release, cancellation, and ownership reports; encoded federate owner handles | Native C++ and fake/real-JVM Java adapter service mappings and typed callback surfaces covered; native and real-JVM two-member tests now prove negotiated acquisition ordering, confirmation-tag propagation, ownership transfer, unavailable acquisition, unconditional release/reacquisition, and pending-acquisition cancellation |
| Advisory/reporting support switches | object-class, attribute, scope, and interaction relevance switches; automatic-resign directive; service/exception reporting switches; provider support-switch queries | typed `ResignAction` plus boolean state with provider-defined initial values | Native C++ and fake/real-JVM Java adapter mappings covered; remaining DDM services remain |

## Delivery order

The work proceeds by a dependency-first vertical slice, not by copying all
206 declarations into a Python abstract base class.

1. **Finish federation management.** Add safe federation overloads and
   synchronization forms alongside their URL/handle-set value boundaries,
   result enums, handle-pair values, and callback families.
2. **Shared handle and encoding substrate.** Complete portable typed handles,
   handle sets/maps, and the `bytes`/data-element boundary by binding the real
   C++ and Java encoders. This is a prerequisite for object, interaction, and
   ownership services; no Python reimplementation of encoding is allowed.
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
   conformance reports.

## Encoder boundary rule

The edition-specific Java API exposes `hla.rti1516_2025.encoding.EncoderFactory`.
The corresponding C++ API instead exposes concrete `DataElement` classes
directly and has no C++ encoder-factory interface. The shared Python
`EncoderFactory` will therefore be a deliberately small provider façade:
the Java provider delegates to its real Java factory, while the native
provider constructs and invokes the real C++ data-element classes. It must
not implement encodings in Python. The first set is limited to the data
elements actually implemented by Umbra C++ today—`HLAinteger32BE`,
`HLAunsignedInteger32BE`, `HLAboolean`, and `HLAunicodeString`—with exact
encode/decode conformance vectors from the C++ suite.

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

## Next concrete slice

Federation management now covers the safe non-time-based overload family,
including vector FOM modules, explicit MIM modules, additional join FOMs, and
explicit synchronization federate sets.
The first encoder slice exposes the four basic C++ elements that Umbra
actually implements, including the standard UTF-16 element-count rule for
`HLAunicodeString`. Portable typed handles now cover federates, the standard
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
adapters. The
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
selective unassociation, default-region fallback, and regional scope-advisory
out/in transitions across subscription removal and restoration. Both providers also
realize default-region object callbacks with an empty conveyed region set for
explicit regional subscribers in receive-order and timestamped paths, while
ordinary non-regional callbacks remain unmarked. Both providers agree on the
smallest-positive-subnormal floating epsilon and canonical signed zero, and
both exercise exact-boundary relaxed DDM delivery. Provider-owned logical-time
add/subtract/difference now cross the native pybind and Java adapter boundaries
with integer and floating parity tests. The next slice is broader regional
discovery/reprojection edges and provider-specific arithmetic edge cases.
Broader time-based save/restore sequencing and finer-grained regional
object association edge cases remain deferred; receive-order and non-regional timestamped
Java fixture fanout now reach two independent subscribers, while scalar save/restore, saved
logical-time and lookahead rollback, timestamped-save input, constrained
timed-save grant ordering, in-transit and re-entrant TSO save boundaries,
native live-TSO retraction restoration, and native plus Java-adapter timestamped
regional attribute callbacks are covered.
Broader provider-specific floating-time overflow, underflow, and
cross-implementation rejection cases still remain.
