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
development profile; RTI-initiated Resign and Connection Lost remain private
model transitions until their real event slices are ready.

### 3. Federation-management vertical slice

Before broader object or time services, implement one end-to-end standard path:
connection, federation creation/destruction, join/resign, listing reports,
disconnect, and the connection-lost callback. The initial embedded path now covers all four
standard C++ Connect overloads, Disconnect, and the four callback-control
services. The aggregate Connect crosswalk is still ambiguous in the
Requirements Lab, so its tests cannot yet enter the compliance catalog;
Disconnect has a selected C++ surface and raw JUnit evidence. An opt-in
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
Basic-data `representation` values and special reference-data instance-
identifier handling remain for later work. The preflight rejects a
reference-data type whose named object class is absent from the complete
hierarchy and, for ordinary attributes, resolves the named attribute through
ancestors and compares its type. The two standard instance identifier names
remain outside that predicate until their implicit type semantics have a
reviewed rule. Directed-interaction names are resolved against the completed
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
  upper bound. The interaction regional declaration/send slice consumes those
  committed specs for independent subscriptions and 2025 overlap filtering.
  The bounded object-attribute regional slice now consumes the same committed
  specs for no-name regional registration, additive association/unassociation,
  regional attribute subscriptions, overlap-filtered discovery and no-time
  reflection, optional sent-region callback metadata, and reservation-consuming
  named regional registration. The per-federate Attribute Scope Advisory Switch
  now gates grouped in/out callbacks for known-object committed-overlap,
  update-region-association, and subscription transitions, with immediate/evoked
  delivery and stale-work suppression. Default-region synthesis, timestamped sends, broader
  DDM routing, save/restore,
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
surface contains three bounded non-regional timestamped interaction,
attribute-update, and object-deletion/removal slices.
A private recipient-scoped queue and temporal coordinator now feed queued, in-transit, and
delivered-since-last-advance timestamps into the GALT/LITS snapshot and
limited scheduler. The scheduler now registers and
re-evaluates limited cross-federate TAR grants under strict defined-GALT and
undefined-GALT/NRG policy after relevant role, membership, and additional-FOM
definition changes, then rechecks the bound at callback delivery. The profile still has no
connection-lost or other federation-event callbacks, complete object/ownership
state, broader time-management services, or transport. `getTimeFactory` is exposed
there only to return a factory for the immutable selected federation time
representation. The same profile exposes `getFederateHandle` and
`getFederateName` only for active members of the caller's joined federation;
it keeps invalid handles distinct from valid handles not known to that
federation. It also exposes `getObjectClassHandle`, `getObjectClassName`,
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
declaration sides. The official `universally` argument is outside this
bounded semantic slice; timestamped/retraction,
directed DDM, ordering, sharing-policy enforcement, and remote transport are
not implemented. It
also implements the bounded regional interaction declaration and no-time send
overloads: regional subscriptions are independent, committed region-set
overlap gates delivery, empty sent sets suppress it, and queued callbacks
recheck the overlap. The separate object-attribute regional boundary is
implemented only for no-name registration, association/unassociation, regional
subscriptions, overlap-filtered no-time reflection, reservation-consuming
named regional registration, and the bounded Attribute Scope Advisory path for
known-object overlap, association, and subscription transitions; additional
regional request edge cases, timestamped/retraction behavior,
broader DDM routing, directed DDM, FOM sharing-policy enforcement, custom
transportation, full
object delivery, or MOM behavior. The official
Restaurant extension remains correctly rejected because its directed-interaction
merge cannot be encoded by the vendored FDD schema.

The same development profile now retains explicit publication and active/passive
subscription state for available attributes of object classes through the
four non-region 2025 object-class attribute declaration services. The state is
per-federate and per-class, validates inherited handles against the composed
FOM, and clears at resign. The current object-registration slice consumes it
to determine whether a class is published and to snapshot the currently
published attributes, including the limited implicit privilege-to-delete rule.

The same profile implements the unnamed `Register Object Instance`
overload, generated private object-instance names, non-region Discover Object
Instance callbacks, and `getKnownObjectClassHandle`,
`getObjectInstanceHandle`, and `getObjectInstanceName`. The private registry
captures a registered class, producer, owned published-attribute snapshot,
recipient-local known class, and pending discovery reservation. Discovery uses
the closest subscribed class, rechecks a queued recipient's eligibility before
callback delivery, and honors immediate versus evoked callbacks. It also
implements only the no-time `Delete Object Instance` overload and no-time
`Remove Object Instance` callback. The registry records the current owner of
each registration-established attribute, requires the deleting federate to own
`HLAprivilegeToDeleteObject`, makes that federate unknown immediately, and
keeps another recipient known only until its removal callback starts. Named
registration, timestamped/retraction, DDM scope, ownership
transfer, FOM sharing policy, save/restore, and resign-action object
disposition are explicitly not implemented.

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
only.

The registry now also implements the bounded 2025 `Local Delete Object
Instance` service. It removes only the invoking federate's known-instance
state, rejects a federate that still owns instance attributes or has a pending
ownership acquisition, leaves the federation-wide object and other federates'
knowledge untouched, and permits a later eligible subscription to rediscover
the object. Timestamped/local-delete interaction behavior, DDM, save/restore,
resign-action object disposition, and remote transport remain separate work.

The same registry now implements the no-time `Update Attribute Values` overload
and matching `Reflect Attribute Values` callback. It accepts
only source-owned FOM-defined attributes of a known live instance, retains one
passel per FOM transportation type, projects each passel at the receiver's
known class and current subscription, excludes the source, and rechecks a
queued callback before delivery. The test fixture proves multi-transport
passels, passive superclass delivery, unsubscribe suppression, tag/producer
propagation, and both callback models. Its separate regional object-attribute
  scenario proves committed-region overlap discovery/reflection and optional
  sent-region callback metadata. The separate Attribute Scope Advisory slice
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
Catch2/Requirements Lab records are
`compliance/timestamped-object-deletion-requirements-contract.json` and
`compliance/timestamped-object-deletion-api-contract.json`. Regional updates,
default-region synthesis,
update-rate reduction, ownership transfer, FOM sharing-policy enforcement,
custom transportation implementation, save/restore, and remote transport
remain separate work.

The registry also implements the object-instance `Request Attribute Value
Update` overload and matching `Provide Attribute Value Update` callback. It
validates the request at the requester's currently known class, groups currently
owned requested attributes by provider, suppresses callbacks for requester-owned
and unowned attributes, preserves the tag, and rechecks a queued provider before
invocation. A separate bounded response scenario now exercises provider code
invoking the official non-timestamped `Update Attribute Values` service from
inside that callback and the requester receiving `Reflect Attribute Values` with
the response tag, producer, and mandatory transportation type. This is not RTI-
automatic provision: the provider supplies the response explicitly. Regional
requests, timestamped/retraction behavior, DDM, update-rate reduction, ownership
transfer, FOM sharing policy, save/restore, and remote transport remain outside
this slice.

The sibling object-class `Request Attribute Value Update` overload expands the
same owner solicitation over every current instance registered at the selected
class and its subclasses. It validates the selected class and attributes,
does not require the requester to know each expanded instance, groups work per
provider and object instance, preserves the tag, and rechecks providers at the
callback boundary. Additional regional request forms, automatic provision and a resulting value
update, timestamped/retraction behavior, DDM, ownership transfer, FOM sharing
policy, save/restore, and remote transport remain separate work.

The class-level `Request Attribute Value Update With Regions` overload now uses
the same private class expansion and owner grouping, with a request-region map
validated for committed ownership and object-class dimension context. Explicit
update associations are solicited only when they overlap the corresponding
request region; attributes using the default region remain eligible, and an
empty region pair is a no-op. The request tag and official Provide callback are
preserved, and callback-entry rechecks prevent stale region state from leaking
into provider code. Provider responses remain explicit user actions. Resulting
reflection, automatic provision, timestamped/retraction behavior, broader DDM,
save/restore, and remote transport remain
separate work.

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
tag. A bounded `Cancel Attribute Ownership Acquisition` transition moves only
a still-pending regular request into private cancellation state, suppresses its
stale notification/release work, and retains the matching publication guard
until `Confirm Attribute Ownership Acquisition Cancellation` begins. That
confirmation groups qualifying attributes for one cancellation call and then
allows later regular work. It does not implement competing in-flight
cancellation race outcomes, negotiated acquisition, remaining divestiture
flows, RTI-owned state, or complete resign-action ownership disposition.

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
This is a single current-recipient sweep rather than a continuing search after
later join, discovery, or publication events.

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
the continuing unconditional owner search, RTI-owned state, and complete
resign-action disposition remain separate work.

The next bounded ownership transition is 2025 `Negotiated Attribute Ownership
Divestiture` with `Request Divestiture Confirmation`, `Confirm Divestiture`,
and `Cancel Negotiated Attribute Ownership Divestiture`. The registry retains
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
a duplicate owner callback. This is not
the complete negotiated-divestiture lifecycle: ongoing owner search,
Willing-to-Acquire selection, and negotiated acquisition remain separate work.

The same profile now implements the mandatory 2025 transportation-type lookup
pair, `getTransportationTypeHandle` and `getTransportationTypeName`, for only
`HLAreliable` and `HLAbestEffort`. The no-region receive-order interaction and
attribute-update paths apply the FOM-selected mandatory type; this is not an
implementation of custom transportation types or message transport, and its
no-time paths do not imply the separate bounded timestamped object-lifecycle
slice.

The dimension and region foundation is intentionally bounded: it exposes the
FOM-defined dimension identity and upper bound, implements the private
region-template/specification lifecycle, and supplies committed specs to the
interaction and bounded object-attribute regional slices. It does not implement
regional realization, additional regional request edge cases, timestamped regional
sends, or broader DDM routing.

Every newly implemented public method must validate preconditions, mutate state
atomically, queue callbacks according to the selected model, and map failure to
the official exception hierarchy.

Current evidence: Catch2 calls the official symbols through the factory, covers
valid/repeated/invalid connection transitions, empty-queue callback-control
behavior, a callback-session shutdown fence, active federate name/handle
lookup, stable FOM-backed object-/interaction-class and inherited-attribute/
parameter name/handle lookup, mandatory transportation-type name/handle lookup,
per-federate interaction and object-class attribute declaration state and resign cleanup,
unnamed object registration/discovery with known-instance lookup in both callback models,
no-time object deletion/removal with tag/producer propagation in both callback models,
limited receive-order interaction and attribute-update delivery in both
callback models, the regional interaction subscription/send case with overlap
and callback-recheck assertions, the regional object-attribute registration /
association / subscription case with overlap-filtered no-time reflection, and the
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
scheduler reports.
It emits raw JUnit only for the real embedded Disconnect catalog entry. The
next object-management gates are regional attribute-value-request semantics,
default-region synthesis, the object-class and regional response forms, timestamped/retraction behavior,
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
