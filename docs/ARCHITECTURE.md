# Architecture

Umbra is organized around the official C++ API and a private runtime. There is
no Umbra-authored public RTI interface. The current native artifact has a small
embedded connection runtime; other languages will be downstream consumers of
the completed C++ RTI.

~~~text
official IEEE C++ API
          |
private standard-binding adapter
          |
embedded lifecycle + callback queue + reference time -> FOM preflight -> development registry / transport (next)
~~~

## Current API baseline and connection slice

The vendored third_party/ieee1516.1-2025/include/RTI tree is the only public
interface input. It is compiled as the CMake
umbra::ieee1516_2025_headers target, installed with Umbra, and checked against
a committed digest manifest. umbra::rti implements the standard factory,
rtiName, and rtiVersion directly.

The separately vendored third_party/ieee1516.2-2025 resource tree contains the
official 2025 XML schemas, standard MIM, and supplied example FOMs. It is a
digest-checked runtime input set, installed with the SDK; it introduces no new
public Umbra API.

umbra::rti is a static library that defines the official factory, identity
functions, the Exception base, and all 109 standard exception types declared
by the vendored 2025 `Exception.h`, plus the support types required by the
initial connection API: VariableLengthData,
ConfigurationResult, and RtiConfiguration. It also implements the official
FederateHandle, ObjectClassHandle, ObjectInstanceHandle, InteractionClassHandle, AttributeHandle, and ParameterHandle value types used by the embedded
federation-management and lookup slices; construction, decoding, and value
extraction remain private to the runtime. The same
target now implements the official HLAinteger64Time/HLAfloat64Time values,
intervals, factories, encodings, and HLAlogicalTimeFactoryFactory. The
separate static umbra::fedtime target owns the standard libfedtime forwarding
entry point and propagates STATIC_FEDTIME to consumers.

The private generated RtiAmbassadorShell overrides all 181 RTIambassador pure
virtual members and throws RTIinternalError by default. UmbraRtiAmbassador
derives from it and overrides nine declarations in the default profile: the
four Connect overloads, Disconnect, Evoke Callback, Evoke Multiple Callbacks,
Enable Callbacks, and Disable Callbacks. The opt-in embedded
federation-management profile adds the two Create overloads, Create With MIM,
Destroy, both List services, both Join overloads, Resign, getTimeFactory,
getFederateHandle, getFederateName, getObjectClassHandle, getObjectClassName,
getInteractionClassHandle, getInteractionClassName,
getAttributeHandle, getAttributeName,
getParameterHandle, getParameterName,
Publish/Unpublish Interaction Class, Subscribe/Unsubscribe Interaction Class,
Publish/Unpublish Object Class Attributes,
Subscribe/Unsubscribe Object Class Attributes,
Reserve/Release Object Instance Name and Reserve/Release Multiple Object
Instance Names,
Register Object Instance With Regions,
Associate/Unassociate Regions For Updates,
Subscribe/Unsubscribe Object Class Attributes With Regions,
the unnamed Register Object Instance overload,
the no-time Delete Object Instance overload and bounded non-regional timestamped
Delete Object Instance overload,
the no-time Update Attribute Values overload,
the regional Subscribe/Unsubscribe Interaction Class With Regions overloads,
and the no-time Send Interaction With Regions overload,
the object-instance Request Attribute Value Update overload,
the object-class Request Attribute Value Update overload,
Request Attribute Value Update With Regions,
Query Attribute Ownership,
Is Attribute Owned By Federate,
Unconditional Attribute Ownership Divestiture,
Attribute Ownership Acquisition,
Attribute Ownership Release Denied,
Negotiated Attribute Ownership Divestiture,
Confirm Divestiture,
Cancel Negotiated Attribute Ownership Divestiture,
Attribute Ownership Divestiture If Wanted,
Cancel Attribute Ownership Acquisition,
Attribute Ownership Acquisition If Available,
getKnownObjectClassHandle, getObjectInstanceHandle, getObjectInstanceName,
the non-timestamped non-region Send Interaction overload, the bounded
non-regional timestamped Send Interaction, Update Attribute Values, and Delete
Object Instance overloads, their matching callbacks, Retract, and
MessageRetractionHandle decoding,
the object-class directed-interaction declaration overloads and bounded
non-timestamped non-DDM Send Directed Interaction overload,
Time Advance Request, Query Logical Time, Enable/Disable Time Regulation,
Enable/Disable Time Constrained, Query Lookahead, Query GALT, and Query LITS.
This makes the remaining scope explicit rather than returning placeholder
success.

The connection slice owns one FederateLifecycle and protects its transition
with a mutex. It accepts HLA_IMMEDIATE and HLA_EVOKED, establishes an embedded
connection from not_connected to not_joined, and rejects repeated connects with
AlreadyConnected. Optional configuration fields are not yet consumed by the
embedded backend, so all Connect overloads truthfully return a
ConfigurationResult with configuration/address false and settings ignored.
Credentials are accepted because the initial backend has no authorizer; it
does not claim credential validation. A valid Disconnect moves an unjoined
federate back to not_connected; the obvious invalid states map to NotConnected
and FederateIsExecutionMember.

CallbackDispatcher is a private FIFO queue with explicit immediate and evoked
models. In immediate mode enabled tasks are delivered synchronously; in evoked
mode the Evoke services deliver one or more tasks according to their wait
arguments. Disable preserves pending tasks and Enable releases them. The
development profile now uses it for real federation-listing, object-discovery,
object-removal, and receive-order interaction, attribute-update, and
attribute-value-request events, plus object-instance name reservation result
callbacks: a List request snapshots the registry and
invokes the matching FederateAmbassador report after both runtime locks are
released, an accepted unnamed or reservation-consuming named registration
queues a rechecked Discover Object Instance invocation for each eligible
recipient, an accepted no-time deletion
queues one Remove Object Instance invocation for each other known recipient, and an accepted
timestamped Send Interaction queues TSO payload state for active
time-constrained recipients and delivers their timestamped callback before the
matching Time Advance Grant. Its first public contract is limited to the
non-regional interaction form. An accepted non-timestamped Send Interaction
queues a rechecked Receive Interaction
invocation for each eligible recipient. An accepted non-timestamped Send
Directed Interaction queues a rechecked Receive Directed Interaction invocation
for each recipient that knows the target object and still has the directed
publication/subscription pair. An accepted no-time Update Attribute
Values request queues a rechecked Reflect Attribute Values invocation for each
eligible recipient and FOM transportation passel. An accepted no-time regional
interaction send applies the same callback fence after checking the sender's
committed region set against each receiver's independent regional declaration;
ordinary subscriptions do not implicitly satisfy regional sends. An accepted
object-instance
Request Attribute Value Update queues one rechecked Provide Attribute Value
Update invocation for each current non-requesting owner group. An accepted
object-class Request Attribute Value Update expands to registered instances at
that class and its subclasses, then queues one rechecked Provide invocation
per non-requesting owner and object instance. Its regional class form applies
the submitted request-region set to each current update association at planning
and callback entry, preserving default-region eligibility while suppressing
disjoint or empty pairs. Query Attribute Ownership queues
one rechecked result per current joined federate owner plus one for available
attributes, and suppresses queued results once a Remove Object Instance begins.
Attribute Ownership Acquisition If Available records a private pending
willing-to-acquire request, then resolves it at the requester's callback
boundary: attributes still unowned are assigned immediately before Attribute
Ownership Acquisition Notification, while attributes owned by another joined
federate receive Attribute Ownership Unavailable without a release callback.
Repeated requests preserve already-pending WTA attributes without a duplicate
callback and can admit eligible additional attributes; Unpublish Object Class
Attributes rejects removal of their required publication until a terminal
callback. Remove Object Instance and requester resignation clear undelivered
requests. The bounded regular Attribute Ownership Acquisition path instead
keeps a separate pending request: an unowned attribute transfers at acquisition
notification, while a joined remote owner receives Request Attribute Ownership
Release with the original acquisition tag. Repeated regular requests do not
duplicate that release callback. Attribute Ownership Release Denied retains the
owner's attribute and terminates matching regular requests through unavailable
callbacks carrying the denial tag. Cancel Attribute Ownership Acquisition moves
only a still-pending regular request into a separate cancellation reservation:
it nullifies stale acquisition/release work but retains the declaration
publication guard until Confirm Attribute Ownership Acquisition Cancellation
begins, then releases the guard and permits deferred follow-up work.
Attribute Ownership Divestiture If Wanted is a distinct synchronous transition:
it returns only attributes transferred to a current pending regular or If
Available acquirer, retains a notification reservation for the new owner's
publication boundary, and sends that standard notification with the divestiture
tag. In the serial profile, a shared request sequence resolves multiple
eligible requesters deterministically; it is private RTI policy, not a
standards-level arbitration priority. At notification entry, stale former-owner
work has been invalidated and any later regular request can be replanned at the
new owner.
Unconditional Attribute Ownership Divestiture is a separate immediate-unowned
transition: after validating the complete supplied owner set, the registry
removes every ownership record, retains existing standard acquisition work, and
groups a `Request Attribute Ownership Assumption` callback for each currently
eligible non-pending recipient. The callback carries the divestiture tag and
rechecks known-instance, published-at-known-class, pending-state, and unowned
conditions before user code. It is an invitation rather than a transfer; the
recipient must invoke an acquisition service. The embedded profile performs
only this single current-recipient sweep, not a continuing search following
later joins, discoveries, or publication changes.
Negotiated Attribute Ownership Divestiture is a separate bounded owned-state
transition: it preserves the divesting owner while the attribute waits and
selects an existing regular acquisition request as its candidate. The owner
receives one `Request Divestiture Confirmation` callback with the regular
acquisition tag. `Confirm Divestiture` transfers the attribute before an
Acquisition Notification callback carrying the confirm tag; its notification
reservation maintains the new owner's publication boundary. A candidate that
cancels makes `Confirm Divestiture` report `NoAcquisitionPending` and returns
the owner to waiting. Explicit cancellation removes the negotiated state and
re-plans ordinary release work. A normal release reservation remains while its
regular acquisition is pending; when its queued callback reaches negotiated
Waiting state, the registry consumes and suppresses it, so cancellation can
re-plan exactly one replacement when necessary without duplicate owner
callbacks.
The profile does not yet search for new candidates or select a
Willing-to-Acquire request.
CallbackSession borrows the
caller-owned ambassador only while a callback is active; Disconnect closes the
session, clears queued work, and waits for an externally running callback
without holding the ambassador lock. The same boundary delivers Time Advance
Grant, Time Regulation Enabled, and Time Constrained Enabled. The first leaves
the per-federate state unchanged while the request is queued, then advances it
immediately before the grant callback begins; the latter two make the requested
time role visible immediately before their callback begins. Connection-loss,
TSO, and event producers beyond these listed development slices remain
unavailable.

EmbeddedFederationRegistry is also private. It stores federation definitions,
unique active federate names, membership identities, and destroy/resign
invariants, can resolve an active member by name or identity within one
federation, and retains immutable per-federation object-class, interaction-class,
defining-attribute, and defining-parameter handle-directory snapshots that preserve existing values across a compatible FOM extension. It
also owns federation-wide reserved object-instance names by reserving federate
identity; a successful reservation is committed before its standard result
callback, multiple release validates the entire set before erasing it, and
resignation returns that name pool to remaining members. It also owns
per-federate interaction publication and subscription declarations,
per-federate explicit object-attribute publication and active/passive
subscription declarations, plus binding-owned callback routes registered
atomically with membership. It also holds federation-wide object-instance
identities, generated names, registered-class/producer snapshots, currently
published per-attribute ownership snapshots, recipient-local known classes,
pending discovery reservations, pending no-time removal reservations, pending
regular-acquisition and acquisition-cancellation reservations, and pending
acquisition-if-available reservations, pending negotiated-divestiture records,
and post-confirmation acquisition-notification reservations. Its
receive-order routing kernel consumes that declaration state directly: it
chooses a recipient's closest subscribed superclass and filters parameters to
the received class. For object attributes, it validates source ownership,
groups submitted values by their FOM transportation type, then projects each
passel at the recipient's known class and current subscription without mixing
passels. For object-instance attribute-value requests, it validates attributes
at the requester's known class, groups currently owned attributes by provider,
suppresses requester-owned and unowned attributes, and rechecks the provider
route immediately before the Provide callback. For object-class requests, it
validates the selected class and attributes, expands current instances through
their registered-class parent chains, and groups callback work per provider and
object instance without requiring the requester to know those instances. The
same ownership snapshot supports bounded Query Attribute Ownership: it validates
the requester's known class, groups a joined-federate-owned attribute for
`informAttributeOwnership` and an available attribute for `attributeIsNotOwned`,
and rechecks the object lifecycle immediately before delivery. It does not
manufacture an RTI-owned owner designator; that requires a dedicated
standards-faithful RTI-owned ownership-state branch. The same state also supports the
bounded 2025 Attribute Ownership Acquisition If Available path: it retains a
requester in the Willing to Acquire state until callback delivery, atomically
assigns attributes that remain unowned, and otherwise reports the remote-owned
subset unavailable. The same ownership snapshot also supports a narrow regular
Acquisition path: a regular request overrides the same requester's WTA request
for an overlapping attribute, transfers an unowned attribute at notification,
or invokes Request Attribute Ownership Release at a joined remote owner.
Release Denied retains the owner and ends all matching regular requests with
denial-tagged unavailable callbacks. A bounded regular cancellation moves a
still-pending request into a distinct reservation, suppresses stale work, and
keeps its publication guard until grouped confirmation begins. Competing
cancellation races, negotiated acquisition, the complete negotiated owner
search and Willing-to-Acquire selection, continuing unconditional owner search,
RTI-owned state, and
complete resign-action ownership disposition remain absent. The adjacent `Is Attribute Owned By
Federate` lookup uses that same known-instance/known-class validation but
returns only whether the invoking joined federate owns the selected attribute;
it does not produce a callback or change state. The
opt-in profile maps the mandatory 2025 `HLAreliable`
and `HLAbestEffort` transportation-type names to private opaque handle values;
the no-region interaction and attribute-update paths apply a FOM-selected
mandatory type but do
not implement custom transportation or a transport. It can take value snapshots
for the list services, but accepts only prevalidated FOM descriptors. It is
deliberately not an XML or OMT parser. The opt-in libxml2 backend now validates individual
documents and performs a private Annex C-guided compatibility preflight over a
module set. For a set representable by the vendored FDD schema, it emits an
immutable schema-validated FDD with synthetic `Composed_From` metadata,
referenced-note remapping, and service-utilization OR rules alongside the
catalog projection. The private reference-time selector uses the official
empty-name HLAfloat64Time default and checks documented standard FDD time types
against HLAfloat64Time/HLAinteger64Time; it does not guess a custom fedtime
mapping. In the opt-in development profile, a MIM-first coordinator creates an
immutable definition before registry mutation, and an additional-FOM join
commits its replacement definition and membership atomically. The profile uses
a shared in-process registry and remains non-installable because libxml2 and
the vendored resource path are not yet part of the exported SDK contract. It
returns a fresh official logical-time factory for the joined federation's
stored selection and resolves active federate names/handles only within that
same joined federation. It also resolves object- and interaction-class names/handles
and inherited attribute/parameter names/handles from the current composed FOM catalog, using stable
directories when an additional-FOM join extends that catalog. `FederateTimeState` assigns its initial official `LogicalTime`
at Join, retains an embedded Time Advance Request until Time Advance Grant is
dispatched, and services Query Logical Time. The same catalog retains each
declared dimension's upper bound and class associations.
`DimensionHandleDirectory` allocates stable per-federation dimension values,
and the opt-in profile keeps region state in the same federation registry:
`RegionHandle` identifies an owner-scoped template, `RangeBounds` changes
remain pending until a complete commit, and committed ranges are checked
against the FOM upper bound. Interaction regional declarations consume those
committed specs, apply the 2025 region-set overlap rule, and remain independent
from ordinary subscriptions. The bounded object-attribute regional slice also
consumes committed specs for no-name regional registration, additive region
association/unassociation, regional attribute subscriptions, overlap-filtered
discovery and no-time reflection, and optional sent-region callback metadata.
Empty region sets are no-ops and ordinary declarations remain independent.
Named regional registration is now layered on the same registry state. The
per-federate Attribute Scope Advisory Switch gates a committed-overlap,
update-region-association, and subscription planner for known object instances;
the adapter queues grouped `attributesInScope` /
`attributesOutOfScope` callbacks after releasing locks and rechecks scope at
callback entry to suppress stale work. Regional request edge cases,
default-region synthesis, timestamped regional sends, and broader DDM routing
remain separate work. It also retains time-regulation
and time-constrained requests until their respective enable callback, stores an
enabled federate's official `LogicalTimeInterval` lookahead, and services Query
Lookahead. The private temporal coordinator now exposes queued, in-transit,
and delivered-since-last-advance TSO state to the GALT/LITS snapshot. A limited
no-TSO scheduler uses those bounds and the FDD NRG switch to release eligible
TAR callbacks across federates, then rechecks its decision at delivery. The
three bounded public TSO consumers are non-regional timestamped Send
Interaction, Update Attribute Values, and Delete Object Instance; they queue
constrained-recipient payloads and order their callbacks before the matching
grant. The remaining timestamped object/attribute services, transport, alternate advance modes,
request-retraction callbacks, and full time coordination remain unimplemented.
A private recipient-scoped queue foundation
is now integrated into the coordinator, but full time coordination,
callbacks beyond the two listing
reports, three temporal reports, limited object discovery/removal, and limited
receive-order interaction, attribute-update, object-instance/object-class
request/provide delivery, and bounded ownership-query reporting, full object
lifecycle/ownership state,
save/restore, remaining object/attribute DDM region realization and broader routing, directed DDM/timestamped directed delivery, FOM sharing-policy enforcement,
or message transport. It supports only unnamed non-region registration,
discovery, no-time deletion/removal, no-time and bounded non-regional
timestamped attribute update/reflection, object-instance/object-class
Request/Provide Attribute
Value Update, federate-owned/unowned Query Attribute Ownership reporting, the
read-only Is Attribute Owned By Federate lookup, and bounded Attribute
Ownership Acquisition If Available transfer to a requester for attributes
still unowned at callback delivery, plus the bounded regular Acquisition /
Release Denied / cancellation-confirmation callbacks for unowned or
joined-remote-owned attributes, and the bounded synchronous Divestiture If
Wanted returned-set / notification / follow-up-release transition, plus the
bounded Unconditional Divestiture / current-recipient Assumption-offer
transition, plus the bounded Negotiated Divestiture / Request Confirmation /
Confirm / Cancel transition;
additional regional request edge cases, automatic provision, remaining
timestamped/retraction families, competing cancellation
races, complete regular and negotiated acquisition, Request Attribute Ownership
Assumption owner-search after later eligibility changes, the complete
negotiated-divestiture lifecycle beyond a current regular candidate, RTI-owned
state, update-rate reduction,
and the distinct object effects of resign directives remain unimplemented.
Valid resign directives therefore have only a membership effect apart from
clearing the resigning federate's private known/pending callback state.
The default packaged profile keeps Create/Destroy/Join/Resign,
`getTimeFactory`, federate/object-class/interaction-class/attribute/parameter lookup,
transportation-type lookup, interaction declaration, receive-order interaction,
receive-order attribute-update/reflection, object-instance and object-class
request/provide attribute-value update, Query Attribute Ownership, object-class
attribute declaration, Is Attribute Owned By Federate, and Attribute Ownership
Acquisition If Available, Attribute Ownership Acquisition, Attribute Ownership
Release Denied, Attribute Ownership Divestiture If Wanted, Unconditional
Attribute Ownership Divestiture, Cancel Attribute Ownership Acquisition,
object-instance registration/discovery,
and all time-management
methods on the fallback. The supplied
Restaurant extension remains
outside the accepted model boundary because its second directed interaction for
one object class is not representable by the official FDD XSD.

FederateLifecycle projects the top-level federate-lifetime transitions from the
pinned Requirements Lab. Connect and Disconnect are bound to the public
standard service methods in every profile; Join and Resign are also bound in
the opt-in development profile. RTI-initiated resign and connection lost remain
private model transitions until actual federation callback-event slices are
ready.

## Requirement traceability

The API baseline is checked against the local HLA Requirements Lab's
IEEE 1516.1-2025 mappings. The source IDs currently tracked are:

| Official declaration | Requirements Lab transition | Requirement IDs |
| --- | --- | --- |
| RTIambassador::connect | federate.connect | req-federate-connect-before-rti-work, req-connect-service-establishes-connection |
| RTIambassador::joinFederationExecution | federate.join | API seam only in current source model |
| RTIambassador::resignFederationExecution | federate.resign | API seam only in current source model |
| RTIambassador::disconnect | federate.disconnect | req-federate-disconnect-after-resign, req-disconnect-service-terminates-connection |
| FederateAmbassador::connectionLost | federate.connection-lost | req-federate-connection-lost, req-connection-lost-service-behavior |

The Disconnect mapping has an exact selected C++ surface and an Umbra Catch2
catalog entry. Its emitted JUnit evidence is raw and unreviewed, so it supports
an implemented status only. The aggregate Connect mapping still has four
candidate C++ surfaces and no Lab-selected aggregate surface; all four
overloads have runtime tests, but none is cataloged or called validated yet.

`compliance/reference-time-requirements-contract.json` separately pins the
private reference-time implementation to the Lab's standardized-type,
symbolic-name, epsilon, creation-default, and common-implementation records.
Like the FOM composition contract, it is source/test traceability only and
creates no public-service evidence.

`compliance/federation-management-embedded-requirements-contract.json` pins
the opt-in public adapter symbols to Create, Destroy, Join, Resign, and
connection-precondition source records. Its CTest runs only when the
non-installable development profile is enabled; it is not a service-catalog,
JUnit, protected-review, package, or conformance record.

`compliance/get-time-factory-api-contract.json` uses the Lab's exact C++ API
surface record—signature and declared exceptions—to trace the same profile's
`getTimeFactory` adapter and Catch2 selector. No Lab implementation mapping is
currently exported for that method, so the contract makes no broader claim.

`compliance/federate-lookup-api-contract.json` separately pins the exact 2025
C++ declarations and exception sets for `getFederateHandle` and
`getFederateName` to their adapter symbols and active-membership Catch2 case.
The Lab exports no higher-level implementation mapping for those services, so
this is API traceability only, not catalog or conformance evidence.

`compliance/object-class-lookup-api-contract.json` separately pins the exact
2025 C++ declarations and exception sets for `getObjectClassHandle` and
`getObjectClassName` to their adapter symbols and FOM-catalog Catch2 case. The
case also preserves a previously issued class handle when a compatible
additional-FOM join extends the catalog. The Lab exports no higher-level
implementation mapping for those services, so this remains API traceability
only, not catalog or conformance evidence.

`compliance/interaction-class-lookup-api-contract.json` separately pins the
exact 2025 C++ declarations and exception sets for `getInteractionClassHandle`
and `getInteractionClassName` to their adapter symbols and FOM-catalog Catch2
case. The case also preserves a previously issued interaction handle when a
compatible additional-FOM join extends the catalog. The Lab exports no
higher-level implementation mapping for those services, so this remains API
traceability only, not catalog or conformance evidence.

`compliance/attribute-lookup-api-contract.json` separately pins the exact 2025
C++ declarations and exception sets for `getAttributeHandle` and
`getAttributeName` to their adapter symbols and FOM-catalog Catch2 case. The
case resolves inherited attributes through the defining class, preserves an
issued handle after a compatible additional-FOM join, and distinguishes invalid
class/attribute handles from a valid attribute not defined on the supplied
class. The Lab exports no higher-level implementation mapping for those
services, so this remains API traceability only, not catalog or conformance
evidence.

`compliance/parameter-lookup-api-contract.json` separately pins the exact 2025
C++ declarations and exception sets for `getParameterHandle` and
`getParameterName` to their adapter symbols and FOM-catalog Catch2 case. The
case resolves inherited parameters through the defining interaction class,
preserves an issued handle after a compatible additional-FOM join, and
distinguishes invalid interaction-class/parameter handles from a valid
parameter not defined on the supplied interaction class. The Lab exports no
higher-level implementation mapping for those services, so this remains API
traceability only, not catalog or conformance evidence.

`compliance/interaction-declaration-api-contract.json` separately pins the
exact 2025 C++ publication and subscription declarations to their adapter
symbols and a real Catch2 lifecycle/FOM-boundary case. A registry-level Catch2
case observes independent per-federate publication/subscription state and its
resign cleanup. That state is now consumed by the separate limited receive-order
delivery path; it still does not claim declaration advisory callbacks, MOM
behavior, catalog evidence, or conformance.

`compliance/object-class-attribute-declaration-requirements-contract.json` and
`compliance/object-class-attribute-declaration-api-contract.json` trace the
four non-region object-class attribute declaration APIs to the source-derived
state records and exact 2025 C++ declarations. The tests cover FOM-backed
available/inherited-handle validation, explicit publication, active/passive
subscription, class-local removal, and resign cleanup. They explicitly exclude
the later registration/discovery profile consumes its private declaration state,
including the limited implicit privilege effect at registration time. They
still do not claim declaration advisory callbacks, full ownership, update-rate
enforcement, regions, catalog evidence, or conformance.

`compliance/object-instance-registration-requirements-contract.json` and
`compliance/object-instance-registration-api-contract.json` trace the exact
2025 unnamed `Register Object Instance`, `Discover Object Instance`, and three
known-instance support APIs to the registry, adapter, and real Catch2 cases.
The tests cover publication gating, generated names, unique handles,
closest-subscribed-class promotion, callback models, cancellation before
evocation, and known-instance lookup. They are source/API traceability only;
timestamped/retraction, DDM,
ownership transfer, FOM sharing policy, save/restore, resign-action disposition,
packaging, catalog evidence, and conformance remain out of scope. The separate
receive-order update/reflection slice consumes the same registration ownership
and known-instance state.

`compliance/object-instance-name-reservation-requirements-contract.json` and
`compliance/object-instance-name-reservation-api-contract.json` trace the exact
2025 single and multiple object-instance name reservation/release services and
their four result callbacks. The embedded registry commits reservations before
callback delivery, rejects empty and `HLA.` names, reports mixed multiple
outcomes, validates multiple release atomically, avoids generated-name
collisions, and releases names on resignation. This is source/API traceability
only; protected review, packaging, catalog evidence, and conformance remain
out of scope.

`compliance/object-instance-named-registration-requirements-contract.json` and
`compliance/object-instance-named-registration-api-contract.json` trace the
exact 2025 non-region and regional named registration overloads. The registry
requires reservation ownership, preserves a reservation across a publication
failure, consumes it only after object/name indexes commit, reports the
standard name-in-use/not-reserved exceptions, and retains uniform names and
handles through discovery. This is development-profile source/API traceability
only; timestamped/retraction, broader DDM, ownership transfer,
save/restore, protected review, packaging, catalog evidence, and conformance
remain out of scope.

`compliance/object-attribute-scope-requirements-contract.json` and
`compliance/object-attribute-scope-api-contract.json` trace the official 2025
`attributesInScope` / `attributesOutOfScope` callbacks plus the per-federate
Attribute Scope Advisory Switch accessors. The integration case changes
committed subscriber-region overlap and the producing object's update-region
association for one known regional object under both HLA_IMMEDIATE and
HLA_EVOKED, verifies grouped in/out notifications, and proves callback-time
suppression of stale queued transitions. This remains development-profile
source/API traceability only; default-region synthesis, timestamped/retraction
behavior, packaging, catalog evidence, and conformance remain separate.

`compliance/object-instance-deletion-requirements-contract.json` and
`compliance/object-instance-deletion-api-contract.json` trace the exact 2025
no-time `Delete Object Instance` and `Remove Object Instance` declarations to
the bounded registry and adapter path. The integration case covers connection,
membership, known-instance and delete-privilege failures; removes the deleting
federate immediately; preserves an evoked recipient's known state until its
callback; and checks tag/producer propagation in both callback models. It does
not claim timestamped/retraction deletion, ownership transfer, save/restore,
resign-action disposition, catalog evidence, or conformance.

`compliance/local-delete-object-instance-requirements-contract.json` and
`compliance/local-delete-object-instance-api-contract.json` trace the bounded
2025 `Local Delete Object Instance` transition. Only the invoking federate's
known-instance state is removed; ownership and pending-acquisition preconditions
are enforced; the federation-wide object remains available for rediscovery.
This is development-profile source/API traceability only and does not claim
timestamped/local-delete interactions, DDM, save/restore, remote transport,
catalog evidence, validation, or conformance.

`compliance/transportation-type-api-contract.json` separately pins the exact
2025 C++ declarations and exception sets for `getTransportationTypeHandle` and
`getTransportationTypeName` to their adapter symbols and Catch2 case. The
non-installable profile recognizes only the mandatory `HLAreliable` and
`HLAbestEffort` pair. The limited receive-order interaction path applies a
FOM-selected mandatory type; the bounded attribute-update path does the same.
Neither claim custom transportation, timestamped/local object-lifecycle
delivery beyond the bounded deletion/removal slice, message transport, catalog
evidence, or conformance.

`compliance/dimension-lookup-requirements-contract.json` and
`compliance/dimension-lookup-api-contract.json` trace the metadata-only 2025
DDM foundation. `FomCatalog` retains class/interaction dimension associations
and upper bounds, `DimensionHandleDirectory` keeps stable per-federation
values, and the adapter exposes the official available-dimension/name/
upper-bound services.

`compliance/region-lifecycle-requirements-contract.json` and
`compliance/region-lifecycle-api-contract.json` trace the metadata-only 2025
region-template/specification lifecycle. The registry and adapter keep
`RegionHandle` ownership, pending and committed `RangeBounds`, complete-commit
validation, support lookups, deletion, and handle decoding aligned with the
official C++ surface. These contracts are source/API traceability only; object/
attribute regional realizations and associations, timestamped/retraction
behavior, broader DDM routing, packaging, catalog evidence, and conformance
remain out of scope.

`compliance/interaction-region-requirements-contract.json` and
`compliance/interaction-region-api-contract.json` trace the separate bounded
2025 interaction regional declaration/send boundary. The registry keeps
regional declarations independent, validates owner/context/commit state, and
selects recipients only when a sent region overlaps a subscribed region in a
shared dimension. The adapter exposes the official no-time regional overloads
and rechecks queued callbacks. The separate object-attribute regional boundary
is implemented for named and no-name registration, association/unassociation, regional
subscriptions, overlap-filtered no-time reflection, and the class-level Request
Attribute Value Update With Regions solicitation form. Timestamped regional
sends, realization beyond
explicit associations, and broader DDM routing remain separate work.

`compliance/receive-order-attribute-update-requirements-contract.json` and
`compliance/receive-order-attribute-update-api-contract.json` trace the exact
non-timestamped `Update Attribute Values` overload and no-time `Reflect
Attribute Values` callback. The integration scenario covers source ownership,
FOM transportation passels, known-class projection, passive subscriptions,
source exclusion, unsubscribe-before-delivery, tag/producer/type propagation,
and both callback models. It deliberately excludes timestamped/retraction
behavior, regions, update-rate reduction, ownership transfer, custom
transportation, FOM sharing-policy enforcement, catalog evidence, and
conformance. RL-013 records the Requirements Lab's current passelization
clause-ownership mismatch without treating it as a standards citation.

`compliance/attribute-value-update-request-requirements-contract.json` and
`compliance/attribute-value-update-request-api-contract.json` trace the exact
object-instance `Request Attribute Value Update` overload and matching
`Provide Attribute Value Update` callback. The integration scenario covers the
known-instance/attribute boundary, owner grouping, unowned-attribute
suppression, requester-owner suppression, tag propagation, at-most-one
provider callback, and a queued-provider resignation fence. It deliberately
excludes additional regional request edge cases, automatic provision, an ensuing value update,
timestamped/retraction behavior, DDM, ownership transfer, catalog evidence,
and conformance.

`compliance/attribute-value-update-response-requirements-contract.json` and
`compliance/attribute-value-update-response-api-contract.json` separately trace
the bounded provider-response case. A provider invokes the official
non-timestamped `Update Attribute Values` service from inside
`Provide Attribute Value Update`; the requester receives `Reflect Attribute
Values` with the response tag, producing federate, and mandatory transportation
type. This is explicit provider code, not automatic RTI provision, and remains
development-profile source/API traceability only.

`compliance/object-class-attribute-value-update-request-requirements-contract.json`
and `compliance/object-class-attribute-value-update-request-api-contract.json`
trace the exact object-class `Request Attribute Value Update` overload and the
same `Provide Attribute Value Update` callback. The integration case validates
the selected class and attributes, expands a base-class request across two
registered subclass instances without requester discovery, preserves the tag,
suppresses requester-owned callbacks, and rechecks queued owner work after
resignation. It deliberately excludes additional regional request edge cases, automatic provision,
an ensuing value update, timestamped/retraction behavior, DDM, ownership
transfer, catalog evidence, and conformance.

`compliance/attribute-value-update-with-regions-requirements-contract.json`
and `compliance/attribute-value-update-with-regions-api-contract.json` trace
the exact 2025 class-level `Request Attribute Value Update With Regions` form
and its standard `Provide Attribute Value Update` callback. The integration case
checks committed region ownership/context, empty-pair no-op behavior,
overlap-consistent solicitation, default-region eligibility, copied tags, and
callback-entry rechecks. Provider responses remain explicit user code; the
contracts exclude automatic provision, resulting reflection, timestamped/
retraction behavior, broader DDM, catalog evidence,
and conformance.

`compliance/attribute-ownership-query-requirements-contract.json` and
`compliance/attribute-ownership-query-api-contract.json` trace the exact 2025
`Query Attribute Ownership`, `Inform Attribute Ownership`, and `Attribute Is
Not Owned` declarations. The integration case verifies known-instance and
defined-attribute boundaries, owner/result grouping, the concrete owner handle,
and nullification of queued reports after receive-order removal begins. The
current state model has no RTI-owned branch and does not fabricate one; ownership
acquisition/divestiture, RTI-owned reporting, resign-action disposition, and
conformance remain outside the slice.

`compliance/attribute-ownership-check-requirements-contract.json` and
`compliance/attribute-ownership-check-api-contract.json` trace the exact 2025
`Is Attribute Owned By Federate` declaration. Its integration case verifies
that only the invoking current owner receives `true`, while a remote owner and
an unowned attribute yield `false`, with the same official known-instance,
known-class, and removal boundaries. It is read-only and does not broaden the
ownership-transfer claim.

`compliance/attribute-ownership-acquisition-if-available-requirements-contract.json`
and `compliance/attribute-ownership-acquisition-if-available-api-contract.json`
trace the exact 2025 `Attribute Ownership Acquisition If Available`, `Attribute
Ownership Acquisition Notification`, and `Attribute Ownership Unavailable`
declarations. The integration case covers publication and ownership exception
boundaries, the private pending willing-to-acquire reservation, callback-time
transfer of an unowned attribute, repeat-WTA and additional-attribute behavior,
the paired `OwnershipAcquisitionPending` unpublication boundary, a remote-owned
unavailable result, user-tag propagation, and deletion cancellation. It
deliberately excludes the broader regular and negotiated acquisition lifecycle,
remaining divestiture forms, RTI-owned state, full resign
ownership disposition, save/restore, and conformance.

`compliance/attribute-ownership-acquisition-requirements-contract.json` and
`compliance/attribute-ownership-acquisition-api-contract.json` trace the exact
2025 C++ declarations for regular `Attribute Ownership Acquisition`, `Request
Attribute Ownership Release`, `Attribute Ownership Release Denied`, `Cancel
Attribute Ownership Acquisition`, and their terminal callbacks. The Catch2
cases cover callback-time transfer of an
unowned attribute, repeated-request suppression, WTA override, release-request
tag propagation, multi-acquirer release denial, denial-tagged unavailable
callbacks, accepted regular cancellation, grouped cancellation confirmation,
unpublication guarding, and removal cancellation. They do not claim competing
cancellation races, negotiated acquisition, remaining divestiture flows, RTI-owned state, full
resign-action ownership disposition, catalog evidence, or conformance.

`compliance/attribute-ownership-divestiture-if-wanted-requirements-contract.json`
and `compliance/attribute-ownership-divestiture-if-wanted-api-contract.json`
trace the exact 2025 C++ `Attribute Ownership Divestiture If Wanted` declaration
and its `Attribute Ownership Acquisition Notification` callback. The Catch2
cases verify owner/known-instance/defined-attribute failures, the exact returned
subset, synchronous transfer to regular and If Available requests, divestiture-tag
propagation, the publication fence through notification entry, stale former-owner
work suppression, and bounded mixed-request follow-up routing. They do not claim
the separate Unconditional/Assumption path, negotiated divestiture,
standards-level arbitration, RTI-owned state, full resign disposition, catalog
evidence, or conformance.

`compliance/unconditional-attribute-ownership-divestiture-requirements-contract.json`
and `compliance/unconditional-attribute-ownership-divestiture-api-contract.json`
trace the exact 2025 C++ `Unconditional Attribute Ownership Divestiture`
declaration and `Request Attribute Ownership Assumption` callback. Its Catch2
case verifies full-set owner validation, immediate unowned state, preservation
of existing regular and If Available acquisition paths, one grouped tagged offer to a currently
eligible non-pending federate, suppression for an unpublished/stale candidate,
and a later standard If Available acquisition. It does not claim a continuing
search for later eligible federates, negotiated divestiture/confirmation,
RTI-owned state, full resign disposition, catalog evidence, or conformance.

`compliance/negotiated-attribute-ownership-divestiture-requirements-contract.json`
and `compliance/negotiated-attribute-ownership-divestiture-api-contract.json`
trace the exact 2025 C++ `Negotiated Attribute Ownership Divestiture`,
`Confirm Divestiture`, and `Cancel Negotiated Attribute Ownership Divestiture`
declarations with the `Request Divestiture Confirmation` and resulting
`Attribute Ownership Acquisition Notification` callbacks. Its real Catch2
case verifies official connection/membership/object/attribute exceptions,
owner retention while waiting, an existing or later regular candidate, both
tag paths, one-shot stale work suppression, synchronous confirmed transfer,
the publication fence through notification entry, explicit cancellation, and
`NoAcquisitionPending` after candidate cancellation. It deliberately excludes
the ongoing owner search, Willing-to-Acquire selection, negotiated acquisition,
RTI-owned state, full resign disposition, catalog evidence, validation, and
conformance.

`compliance/receive-order-interaction-requirements-contract.json` and
`compliance/receive-order-interaction-api-contract.json` trace the
non-timestamped `Send Interaction` overload and matching no-time `Receive
Interaction` callback to source-derived rules and exact C++ declarations. The
tests cover closest-subscribed-class selection, parameter projection in the
private kernel, passive subscriptions, a sender exclusion, tag/producer/type
propagation, unsubscribe-before-delivery, and both callback models. They do not
claim timestamped/retraction behavior, regions, FOM
sharing-policy enforcement, custom transportation, catalog evidence, or
conformance.

`compliance/timestamped-interaction-requirements-contract.json` separately
traces the first bounded public timestamped `Send Interaction`/`Receive
Interaction` path, `Retract`, and official `MessageRetractionHandle` value
encoding. Its Catch2 scenario covers sender lower-bound validation,
retraction-before-grant suppression, exact-bound TSO delivery before
`Time Advance Grant`, callback timestamp/order fields, and terminal
post-delivery retraction. It excludes timestamped object/attribute updates,
directed or regional forms, alternate advance modes, request-retraction
callbacks, transport, package/catalog evidence, and conformance.

`compliance/timestamped-attribute-update-requirements-contract.json` separately
traces the second bounded public timestamped `Update Attribute Values` /
`Reflect Attribute Values` path, including recipient-specific transportation
passels, sender exclusion, `Retract`, and official timestamp/order/retraction
fields. Its Catch2 scenario covers lower-bound validation,
retraction-before-grant suppression, two-passel exact-bound reflection before
`Time Advance Grant`, and terminal post-delivery retraction. It excludes
directed or regional forms, alternate advance modes,
request-retraction callbacks, transport, package/catalog evidence, and
conformance.

`compliance/timestamped-object-deletion-requirements-contract.json` and
`compliance/timestamped-object-deletion-api-contract.json` trace the third
bounded public timestamped family: non-regional `Delete Object Instance` /
`Remove Object Instance`. Its private payload preserves the known-recipient
set, supports pending retraction with object reconstitution, commits removal
before the recipient's timestamped callback, and excludes the deleting sender
from induced removal. The Catch2 scenario verifies lower-bound rejection,
retraction-before-grant, exact-bound callback ordering, timestamp/order/
retraction propagation, and terminal retraction failure. Mixed fanout,
regional/directed forms, alternate advance modes, transport, package/catalog
evidence, and conformance remain outside the contract.

`compliance/directed-interaction-requirements-contract.json` and
`compliance/directed-interaction-api-contract.json` trace the bounded
non-timestamped, non-DDM `Send Directed Interaction` / `Receive Directed
Interaction` path and its six object-class declaration overloads. The real
Catch2 case covers known-target discovery, sender exclusion, immediate and
evoked callbacks, selective unsubscribe, source unpublication, stale callback
suppression, republishing, tag/producer/mandatory-transport propagation, and
the official `universally` signature while leaving its universal semantics
deferred. They
do not claim timestamped/retraction or directed-DDM behavior, ordering, FOM
sharing-policy enforcement, catalog evidence, validation, or conformance.

`compliance/federation-listing-requirements-contract.json` and
`compliance/federation-listing-api-contract.json` trace the same profile's List
Federation Executions / List Federation Execution Members adapter calls and the
three FederateAmbassador report callbacks. They deliberately keep the
source-derived requirement records separate from the Lab's exact API records,
because the Lab exports no higher-level implementation mapping for this slice.

`compliance/time-advance-requirements-contract.json` and
`compliance/time-advance-api-contract.json` trace joined-federate initial time,
Time Advance Request / Grant, and Query Logical Time through source-derived
requirements and exact C++ declarations. The implementation is deliberately
limited to the embedded no-TSO path, so these contracts make no federation-wide
time-management or conformance claim.

`compliance/time-role-requirements-contract.json` and
`compliance/time-role-api-contract.json` separately trace the callback-gated
Enable/Disable Time Regulation, Enable/Disable Time Constrained, and Query
Lookahead slice. `compliance/time-bounds-requirements-contract.json` and
`compliance/time-bounds-api-contract.json` trace the separate read-only no-TSO
Query GALT/Query LITS slice, including its exclusive zero-lookahead TAR
boundary. `compliance/time-grant-scheduler-requirements-contract.json` traces
the limited TAR release path that consumes those bounds. These source/API
references do not establish timestamped-message, full time-coordination,
package, or conformance behavior.

These references identify design intent and scoped implementation traceability;
they are not evidence of overall standards conformance. The source corpus
remains external to this repository and is not copied here.
