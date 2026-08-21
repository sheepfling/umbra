# Native C++ RTI implementation plan

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
Resign are also bound in the opt-in, non-installable federation-management
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
  the Set Range Bounds lane covers its type-42/type-10/type-35 forms; Create
  Region and Get Range Bounds report forms, generic non-void returns, broader
  DDM routing, and conformance remain open. A neighboring focused lane now
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
semantic work.
User-supplied and synchronization tag types are likewise restricted to their
permitted 2025 data-type families (or `NA`), including reference data but not
basic data.
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
non-installable profile with two event-producing service paths: List
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
and remote transport are not implemented. It
also implements the bounded regional interaction declaration and no-time send
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
additional regional request edge cases, timestamped/retraction behavior beyond
that bounded path,
broader DDM routing, directed DDM, FOM sharing-policy enforcement, custom
transportation, full
object delivery, or MOM behavior. The official
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
  separate Attribute Scope Advisory slice
covers switch-gated in/out callbacks for known-object overlap,
update-region-association, and subscription transitions. Timestamped/retraction
updates now have a separate bounded non-regional path: time-regulating sends
use the federation-owned TSO queue, preserve recipient-specific transportation
passels, and deliver timestamped `Reflect Attribute Values` callbacks before
the matching grant. Its Catch2/Requirements Lab record is
`compliance/timestamped-attribute-update-requirements-contract.json`.
Timestamped deletion now has a separate bounded non-regional path: the
registry retains a pending object-removal payload, queues constrained
recipients, supports retraction-before-delivery reconstitution, and delivers
the timestamped `Remove Object Instance` callback before the grant. Its
recipient-local filesystem route now appends the corresponding private §6.17
record before each callback: immediate recipients use `TIMESTAMP`/`RECEIVE`,
while TSO recipients use `TIMESTAMP`/`TIMESTAMP`, with the timestamp and
supplied retraction designator preserved. This does not yet add a timestamped
`Delete Object Instance` invocation record. Its
Catch2/Requirements Lab records are
`compliance/timestamped-object-deletion-requirements-contract.json` and
`compliance/timestamped-object-deletion-api-contract.json`. A second Catch2
scenario proves a legal post-delivery Retract reconstitutes the object/name/
known state and committed split ownership before Request Retraction reaches a
delivered nonconstrained recipient, while a constrained recipient's pending
removal is suppressed. A third scenario proves a departed delivered owner is
not reconstituted or notified, and its former attribute remains unowned. A
terminal no-recipient case retains `MessageCanNoLongerBeRetracted` while
releasing the deletion snapshot, marker, and object name for a fresh named
registration. A focused normal-interaction regression now covers one Disable
Time Regulation/re-enable lifetime path at unchanged lookahead. Complete
alternate advances, broader re-enable, in-flight ownership, other resignation
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
automatic provision: the provider supplies the response explicitly. Regional
requests, timestamped/retraction behavior, DDM, update-rate reduction, ownership
transfer, FOM sharing policy, save/restore, and remote transport remain outside
this slice.

The bounded Auto Provide path now retains the federation-wide dynamic switch
from the creation FDD and exposes `getAutoProvideSwitch`. After a newly
completed discovery, the registry groups the discovered recipient's in-scope
owned attributes by current provider and schedules the standard
`Provide Attribute Value Update` callback with an empty RTI-invoked tag. The
provider's selected service-report route now follows that queued work and
appends its private §6.22 type-37/type-1/type-63 successful-void record before
the callback; the focused filesystem regression verifies the zero-length tag
and ordering at callback entry. This does not add a report for discovery or
for changing the switch itself.
standard federation-wide `HLAsetSwitches` MOM interaction can now change that
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

The five official 10.29--10.33 normalization services now form the first DDM
coordinate bridge required by MOM. The adapter validates the standard
connection/member/input boundaries. The registry owns a stable opaque mapping
for valid federate, object-class, interaction-class, and live object-instance
handles, restores its per-execution seed with a saved federation, and preserves
equality for equal designators without promising a sequential or unique result.
`ServiceGroup` is deliberately returned as the standard in-range
`HLAserviceGroup` coordinate rather than arbitrary per-execution data. This
does not yet construct the single RTI-owned point region required for MOM
objects/reports, encode report parameters, or route reports through DDM; those
remain the next MOM-specific tranche rather than being hidden behind ordinary
federate-originated interaction delivery.

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
the joined-federate `HLAsetSwitches` update path. Seven source-backed
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
when a lower actual value remains deferred; both enable records preserve their
separate callback-gated completion. `Time Advance Request`, `Time Advance
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
timestamp. The timestamped and regional forms remain excluded until their
return/retraction and regional argument records have equivalent source-backed
file representations. The nonregional receive-order `Send Interaction`
overload now appends its accepted §6.12 successful-void record before the
separately queued `Receive Interaction` callback. Its source-backed Table 5
arguments are type-27 `Interaction class designator`, type-40
`ParameterHandleValueMap` rendered as a
`PairList<ParameterHandle:BinaryData>`, the bounded type-63 base-64
`User-supplied tag`, and a type-34 Null placeholder for the absent optional
timestamp. Timestamped, regional, and `HLAsetSwitches`-specific forms remain
excluded until their return, region, or MOM-control records have equivalent
source-backed file representations. The receive-order `Send Directed
Interaction` overload now appends its accepted §6.14 successful-void record
before the separately queued `Receive Directed Interaction` callback. Its
source-backed Table 5 arguments are type-27 `Interaction class designator`,
type-37 `Object instance designator`, type-40 `ParameterHandleValueMap`
rendered as a `PairList<ParameterHandle:BinaryData>`, the bounded type-63
base-64 `User-supplied tag`, and a type-34 Null placeholder for the absent
optional timestamp. Timestamped/retraction and directed DDM forms remain
excluded until their return and region records have equivalent source-backed
file representations. The nonregional receive-order
`Delete Object Instance` overload now appends its accepted §6.16
successful-void record after its committed deletion transition and before the
separately queued `Remove Object Instance` callback. Its source-backed Table 5
arguments are type-37 `Object instance designator`, the bounded type-63
base-64 `User-supplied tag`, and a type-34 Null placeholder for the absent
optional timestamp. The timestamped sender-invocation and regional forms
remain excluded until their return and region records have equivalent
source-backed file representations; the bounded timestamped recipient callback
record is traced separately above. The
two self-selecting setters, `Set Service Reporting Switch` and `Set Send
Service Reports To File Switch`, remain deferred pending the report-order
source relation recorded in RL-066. The
central explicit-region predicate now implements the documented
Umbra Allow Relaxed DDM policy: enabled federations add only exactly
boundary-touching committed ranges to the strict-overlap set, with no numerical
gap threshold and no loss of existing strict overlap. `docs/RELAXED-DDM-POLICY.md`
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
remain separate work. A focused class-expansion response case now has the
provider invoke timestamped `Update Attribute Values` from the official
callback; the requester remains time-constrained until its grant and receives
one reflection with the response value/tag, producer, order, time, and
retraction metadata. This is explicit provider code, not automatic provision
or complete timestamped/retraction coverage.

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
Instance nullifies stale delivery. It deliberately has no invented owner handle
for RTI-owned attributes: that branch requires its own standards-faithful
RTI-owned state model. Full ownership transfer, RTI-owned state,
resign-action ownership disposition, DDM, save/restore, and remote transport
remain separate work.

The adjacent `Is Attribute Owned By Federate` service uses the same ownership
snapshot as a read-only boolean check. It validates the invoking federate's
known instance and available attribute, returns true only for that federate's
current ownership, and makes no callback or ownership-state transition.

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
allows later regular work. It does not implement competing in-flight
cancellation race outcomes, negotiated acquisition, remaining divestiture
flows, RTI-owned state, or complete resign-action ownership disposition.
Its accepted cancellation now also appends the private service-report file's
source-backed §7.15 record with ObjectInstanceHandle type 37 and
AttributeHandleSet type 1 before the confirmation callback is queued; rejected
calls append nothing. The report slice deliberately does not generalize to
the supplied-empty, in-flight-race, public MOM, or generic return/failure
forms.

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
assumption-callback re-search and full owner arbitration remain separate work.

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
pending regular acquisition request, and delivers one confirmation callback
with that acquisition tag. Successful confirmation transfers the selected
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
the complete negotiated-divestiture lifecycle: ongoing owner search,
Willing-to-Acquire selection, and negotiated acquisition remain separate work.

The same profile now implements the mandatory 2025 transportation-type lookup
pair, `getTransportationTypeHandle` and `getTransportationTypeName`, for only
`HLAreliable` and `HLAbestEffort`. The no-region receive-order interaction and
attribute-update paths now use the effective per-federate type. The bounded
transport-control services provide prospective per-class attribute defaults,
callback-gated instance changes and queries, plus callback-gated published
interaction changes and queries for future ordinary/regional sends. This is
not an implementation of custom transportation types or message transport,
and its no-time paths do not imply the separate bounded timestamped
object-lifecycle slice.

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
proves one callback before the post-re-enable grant. The corresponding
default-source object-update companion preserves an ordinary-registration
timestamped passel across the transition and proves one reflection before the
post-re-enable grant with a supplied-empty default-region marker. A
default-source and explicit-source interaction companions also exercise
ordinary TAR/NMR plus the inclusive TARA and NMRA boundaries, preserving
callback-before-grant ordering
and the supplied-empty/source-region markers; further alternate advances remain
open. A mixed-member regional companion now drives one overlap-qualified TSO
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
The matching default-source/default-region interaction companion drives one
ordinary timestamped Send Interaction through those same three frontiers,
preserving callback-before-grant ordering, FQR actual/optimistic time 7, the
producer TAR at 2, and the supplied-empty callback region marker.

The ownership/DDM boundary now also clears explicit object-attribute
update-region associations when the current owner loses ownership. The helper is
called by If Available acquisition transfer, Divestiture If Wanted, Confirm
Divestiture, unconditional divestiture, unpublish, and resignation paths. A
focused Catch2 scenario and paired `ownership-transfer-update-region` Lab
contracts prove the If Available/If Wanted path: clearing a former owner's
explicit association restores the default source realization, and a new owner
can replace it with an owned explicit region. This is
still a bounded development-profile slice; complete ownership arbitration,
the complete timestamped default-region matrix, regional advisories, package evidence, and
conformance remain future work.

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
switches and recipient-local pre-callback service-report records,
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
cleanup. The untimed
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
grants only after Federation Restored; durable persistence, timed restore,
queued/in-transit TSO restore, role/membership churn,
remaining service interlocks, transport, and conformance remain future work.
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
still a focused development-profile lane, not deletion-family completion.
A matching positive non-regional timestamped directed-interaction companion
now drives one target-qualified payload through FQR, TARA, and NMRA. Each
Receive Directed Interaction callback precedes its own grant, FQR preserves
actual and optimistic time, the producer TAR completes at 2, and the
post-delivery Retract classification remains terminal. Directed DDM and
alternate-advance coverage beyond this bounded FQR/TARA/NMRA case remain open.
It emits raw JUnit only for the real embedded Disconnect catalog entry. The
next object-management gates are remaining timestamped default-region/re-enable
matrix coverage beyond the bounded FQR/TARA/NMRA object case and the now
bounded interaction, default-source object, and explicit-source regional object
Time Constrained transitions, the
object-class and regional response forms, remaining timestamped/retraction behavior,
remaining regular/negotiated acquisition and remaining divestiture flows, RTI-owned state, and full resign ownership
disposition, followed by the remaining
local, timestamped, and ownership-disposition lifecycle design, DDM-region and
FOM sharing-policy rules, then package/transport design before any broader
service claim.

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
