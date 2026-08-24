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
entry point and propagates STATIC_FEDTIME to consumers. `umbra::rti` also
contains the reference `HLAauthorizer` and factory foundation, while the
separate static `umbra::authorizer` target owns the corresponding
library-level factory forwarding entry point and propagates
STATIC_AUTHORIZER.

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
non-regional timestamped Send Interaction, Update Attribute Values, Delete
Object Instance, and Send Directed Interaction overloads, the bounded
timestamped regional Send Interaction With Regions and regional attribute
update extension, their matching callbacks, Retract, and
MessageRetractionHandle decoding,
the object-class directed-interaction declaration overloads and bounded
non-timestamped non-DDM Send Directed Interaction overload,
Time Advance Request and the two Available forms, Query Logical Time,
Enable/Disable Time Regulation, Enable/Disable Time Constrained, Query
Lookahead, Modify Lookahead, the bounded currently-queued-message Next Message
Request forms, the bounded Flush Queue Request/Grant pair, Query GALT, and
Query LITS, and the bounded federation synchronization-point registration /
announcement / achievement / completion services.
This makes the remaining scope explicit rather than returning placeholder
success.

The connection slice owns one FederateLifecycle and protects its transition
with a mutex. It accepts HLA_IMMEDIATE and HLA_EVOKED, establishes an embedded
connection from not_connected to not_joined, and rejects repeated connects with
AlreadyConnected. Optional configuration fields are not yet consumed by the
embedded backend, so all Connect overloads truthfully return a
ConfigurationResult with configuration/address false and settings ignored.
The initial backend has no configured authorizer. The no-credentials Connect
overloads and an explicit empty `HLAnoCredentials` envelope remain usable, but
another supplied credential is rejected with `Unauthorized` rather than being
silently treated as authenticated. `HLAplainTextPassword` has its standard
credential-value encoding, and a reference authorizer/factory exists behind an
internal test configuration seam; secure RID configuration and a live runtime
`HLAauthorizer` remain separate work. The JNI bridge has a raw Java
ServiceLoader conformance adapter that delegates to that C++ reference
authorizer, but it does not change the embedded profile. A valid Disconnect moves an unjoined
federate back to not_connected; the obvious invalid states map to NotConnected
and FederateIsExecutionMember.

CallbackDispatcher is a private FIFO queue with explicit immediate and evoked
models. In immediate mode enabled tasks are delivered synchronously; in evoked
mode the Evoke services deliver one or more tasks according to their wait
arguments. Disable preserves pending tasks and Enable releases them. The
development profile now uses it for real federation-listing and
synchronization-point registration/announcement/completion (including a
focused direct-delivery regression for `HLA_IMMEDIATE`),
object-discovery,
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
non-regional interaction form, with bounded timestamped attribute, deletion,
and directed-interaction extensions tracked separately. An accepted
non-timestamped Send Interaction
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
recipient must invoke an acquisition service. The embedded profile also
retains unowned search state and rechecks it when a later join, discovery, or
publication change makes a federate eligible, suppressing duplicate offers.
Terminal assumption-callback re-search and full owner arbitration remain
separate work.
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
unique active federate names, active membership identities, and destroy/resign
invariants. It resolves joined names only to active members while retaining a
separate lifetime identity index for returned federate designators after
resignation, and retains immutable per-federation object-class, interaction-class,
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
chooses a recipient's closest active subscribed superclass and filters parameters to
the received class. For object attributes, it validates source ownership,
groups submitted values by their FOM transportation type, then projects each
passel at the recipient's known class and current active subscription without mixing
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
RTI-owned state, and complete resign-action ownership disposition remain absent. The adjacent `Is Attribute Owned By
Federate` lookup uses that same known-instance/known-class validation but
returns only whether the invoking joined federate owns the selected attribute;
it does not produce a callback or change state. The
opt-in profile maps the mandatory 2025 `HLAreliable`
and `HLAbestEffort` transportation-type names to private opaque handle values.
Its bounded transportation-control state keeps per-federate prospective
attribute defaults, captures an effective type on registration and ownership
transfer, commits instance changes at confirmation callbacks, and feeds
future ordinary/regional interaction sends and attribute updates. Queries
recompute the current effective type, falling back to the FDD when an
interaction is not published or an attribute is unowned. The adjacent order
state follows the same ownership boundary: class defaults are stored per
federate, object instances capture an effective preferred order, explicit
instance changes affect only future owned updates, ownership transfers reset
the capture from the new owner's class default, and interaction overrides are
scoped to the invoking publisher. Planners carry that order into timestamped
payloads and split mixed Receive/TimeStamp attribute passels before queuing or
delivering callbacks. It does not implement custom transportation or a
transport. It can take value snapshots
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
stored selection and resolves joined federate names to active handles while
retaining valid departed-handle name identity within that same joined federation.
It also resolves object- and interaction-class names/handles
and inherited attribute/parameter names/handles from the current composed FOM catalog, using stable
directories when an additional-FOM join extends that catalog. `FederateTimeState` assigns its initial official `LogicalTime`
at Join, retains an embedded Time Advance Request until Time Advance Grant is
dispatched, and services Query Logical Time. The same catalog retains each
declared dimension's upper bound and class associations.
The composer also resolves table-level representation references after the
complete module set is merged. Simple/enumerated representations must resolve;
ordinary reference-data representations must name a higher-level data type;
and the two standard object-instance identifier references use their explicit
`HLAunicodeString`/`HLAobjectInstanceHandle` exception. The MIM/Restaurant
`HLAboolean` case follows the reviewed RL-009 compatibility interpretation.
The same preflight rejects supplied non-positive update rates and validates
dimension default ranges against `[0, upperBound)`, while retaining incomplete
DIF rows for later composition; the 2025 FOM XSD enforces positive dimension
upper bounds before this stage. Non-negative-lookahead inference and other
table-specific rules remain separate validation work.
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
An explicit object-attribute update-region association is owner-scoped: the
registry erases it when the current owner divests, transfers, unpublishes, or
otherwise loses ownership; without an explicit replacement, subsequent updates
use the derived default region. A later owner can associate a new owned region
to replace that realization. This cleanup is centralized in
`clearUpdateRegionAssociation` and is exercised by the bounded ownership/DDM
transfer scenario; it is not a claim that all ownership or regional forms are
complete.

The bounded default-region realization is intentionally derived rather than
stored as a synthetic public `RegionHandle`. `regionOverlapsDefault` recognizes
only a committed, non-empty explicit region as overlapping the RTI-provided
full-range default. Object discovery, scope/relevance, no-time reflection, and
interaction routing use that predicate alongside the independent declaration
state: an ordinary declaration is effective through the default only while no
explicit regional declaration exists for the same supported context. The
adapter preserves a separate `defaultRegionUsed` marker through receive-order
and timestamped payloads, allowing an enabled Convey Region Designator Sets
callback to receive a supplied empty set for a default-region event. The
receive-order and direct time-constrained timestamped behavior are backed by
`compliance/requirements-lab/default-region-requirements-contract.json`; the remaining
timestamped default-region matrix and broader DDM matrix remain intentionally
open.
Named regional registration is now layered on the same registry state. The
per-federate Attribute Scope Advisory Switch gates a committed-overlap,
update-region-association, and subscription planner for known object instances;
the adapter queues grouped `attributesInScope` /
`attributesOutOfScope` callbacks after releasing locks and rechecks scope at
callback entry to suppress stale work. Regional request edge cases, remaining
timestamped default-region matrix entries and regional object/attribute
forms, and broader DDM routing remain separate work. The bounded timestamped
regional attribute-update extension now reuses the committed update-region
associations and recipient-side Convey Region Designator Sets gate described
below. It also retains time-regulation
and time-constrained requests until their respective enable callback, stores an
enabled federate's official `LogicalTimeInterval` lookahead, and services Query
Lookahead. The private temporal coordinator now exposes queued, in-transit,
and delivered-since-last-advance TSO state to the GALT/LITS snapshot. A limited
no-TSO scheduler uses those bounds and the FDD NRG switch to release eligible
TAR callbacks across federates, then rechecks its decision at delivery. The
bounded public TSO consumers include timestamped Send Interaction, Update
Attribute Values (including its committed regional update-region extension),
Delete Object Instance, directed Send Interaction, and region-context Send
Interaction; they queue
constrained-recipient payloads and order their callbacks before the matching
grant. The bounded currently-queued-message Next Message Request reuses that
grant path and retains the caller's requested boundary separately. The two
Available forms reuse the grant path with their explicit inclusive GALT
policy. The bounded Flush Queue Request/Grant path reuses that dispatcher to
flush currently queued in-process TSO payloads and retain the optimistic time
floor. The normal timestamped Send Interaction, Send Interaction With Regions,
Update Attribute Values (including the bounded regional path), and Send
Directed Interaction paths share a recipient ledger: a legal post-delivery
Retract queues Request Retraction for a delivered recipient while suppressing
still-pending fanout. The separately contracted deletion/removal path uses the
same ledger plus an execution-owned reconstitution snapshot. Its terminal
tombstone keeps `MessageCanNoLongerBeRetracted` classification while typed
payload drains and terminal deletion state can be reclaimed. Future transport
input and full time coordination remain unimplemented.
A separate per-federate asynchronous-delivery switch gates receive-order
callbacks for time-constrained members. The switch defaults disabled, queues
receive-order interaction, reflection, directed-interaction, and object-removal
callbacks while an idle constrained federate is not time-advancing, and releases
them when asynchronous delivery is enabled or the federate enters Time
Advancing. Disabling the switch restores the normal time-advance gate. The
bounded integration regression exercises those two release points under both
`HLA_EVOKED` and `HLA_IMMEDIATE`. The deferred callback closures belong to the live embedded session rather than a
durable save/restore payload, and timestamped messages remain time-advance
gated while the federate is idle.
A parallel support-switch projection retains the remaining 2025 FDD entries.
Convey Region Designator Sets, Automatic Resign Action, Service Reporting,
Exception Reporting, and Send Service Reports To File are copied into each
federate's membership at join and can be changed independently through the
official support-service accessors. Delay Subscription Evaluation and Allow
Relaxed DDM are captured once at federation creation and returned consistently
to every member. Automatic-resign values are checked against the official enum
before storage. The bounded regional reflection and interaction projections
also carry the recipient's Convey Region Designator Sets policy to callback
delivery: disabled recipients receive a null optional sent-region argument,
while enabled recipients receive the sent update-region realization. This now
also drives bounded ordinary interaction and known-object attribute-update
filtering for the federation-wide Delay Subscription Evaluation switch: when
enabled, a joined non-source recipient with no current subscription is retained
as a callback/TSO candidate and reprojected at its actual delivery boundary.
The bounded ordinary receive-order and timestamped regressions exercise that
path under both `HLA_EVOKED` and `HLA_IMMEDIATE`: the former temporarily
suspends direct dispatch to establish a concrete callback boundary, while the
latter uses direct Time Advance Grant delivery. When disabled, that original
ineligibility is not retained. Both paths use
current subscriptions before calling user code. Explicit regional interactions
and Update Attribute Values passels associated with explicit update regions
deliberately retain planning-time DDM selection until their complete matrices
are specified. The exact `HLAreportServiceInvocation` subscription and Service
Reporting-switch mutual exclusion is enforced for ordinary and regional
declarations. The interlock itself does not generate service reports. Separate
file-selected wrappers persist the explicit Table 5 successful-void `[null]`
record for seven successful-void support services: six Boolean setters (the
four relevance/scope advisory setters, `Set Convey Region Designator Sets
Switch`, and `Set Exception Reporting Switch`) plus `Set Automatic Resign
Directive`; generic report generation and delivery remain deferred. The
five no-argument Time Management wrappers—`Disable Time Regulation`,
`Enable/Disable Asynchronous Delivery`, and `Enable/Disable Time
Constrained`—append empty supplied-argument lists using the same explicit
successful-void return form. `Enable Time Regulation` appends its one
`Lookahead` argument using Table 5's `LogicalTimeInterval` type 32 and
quoted `interval.toString()` form. `Modify Lookahead` appends its one
`Requested lookahead` with the same type/form when accepted, including a lower
request whose actual value remains deferred. Both enable records preserve the
later callback-gated state completion. `Time Advance Request`, `Time Advance
Request Available`, `Next Message Request`, `Next Message Request Available`,
and `Flush Queue Request` each append one `Logical time` using Table 5's
`LogicalTime` type 31 and quoted `time.toString()` form when accepted. The
first four reports precede their Time Advance Grant callbacks; the Flush Queue
report precedes its distinct actual/optimistic Flush Queue Grant. The two Next
Message Request forms and Flush Queue Request preserve the supplied request
when queued TSO input yields an earlier grant target. `Retract` appends its
one `MessageRetractionDesignator` using Table 5's type-33
`MessageRetractionHandle<decimal-identity>` representation before the separate,
callback-gated Request Retraction consequence. `Change Attribute Order Type`
appends type-37 `Object instance designator`, type-1 `Set of attribute
designators` as a quoted-handle array, and type-38 `Order type` using quoted
`RECEIVE` or `TIMESTAMP` after a successful owned-attribute invocation. `Change
Default Attribute Order Type` appends type-36 `Object class designator` as a
quoted `ObjectClassHandle::toString()` value, type-1 `Set of attribute
designators` as the same quoted-handle array, and type-38 `Order type` using
quoted `RECEIVE` or `TIMESTAMP` after a successful class-default invocation.
`Change Default Attribute Transportation Type` appends the same type-36 object
class and type-1 attribute-set representations with type-59 `Transportation
type` as quoted `TransportationTypeHandle::toString()` text after an accepted
prospective class-default invocation. `Change Interaction Order Type`
appends `Interaction class designator` as type 27 with the quoted exact
`InteractionClassHandle::toString()` value and `Order type` as type 38 with
quoted `RECEIVE` or `TIMESTAMP` after the class is published. `Request
  Interaction Transportation Type Change` appends type-27 `Interaction class
  designator` and type-59 `Transportation type` as quoted exact handle
  `toString()` values when the request is accepted; the separately queued
  confirmation remains the transport-preference boundary. `Request Attribute
  Transportation Type Change` appends type-37 `Object instance designator`,
  type-1 `Set of attribute designators` as a quoted-handle array, and type-59
  `Transportation type`, using the same exact quoted handle `toString()` forms
  when accepted; its separately queued confirmation remains the
  transport-preference boundary. The
bounded joined-federate `HLAsetSwitches` adapter resolves a compatible extension
subclass to its predefined parent, ignores extension-only parameter values,
and sends one locked per-member update through the registry for any supplied
standard subset. A report-service conflict rejects that update without partial
state mutation, but currently reaches the caller as a local Send Interaction
error rather than a normal MOM failure report. The adjacent 10.29--10.33
normalization services use a federation-owned execution seed plus a stable
opaque mixer for federate, object-class, interaction-class, and live
object-instance handles; equal valid designators therefore retain equal point
coordinates without exposing the private sequential handle directories. The
seed is restored with a saved execution. `HLAserviceGroup` instead uses the
official enum coordinate because its standard MIM dimension is bounded to
seven. This is a prerequisite for generic RTI-originated MOM report regions.
The bounded embedded transport-fault path now uses the `HLAfederate` point to
deliver `HLAreportFederateLost` to ordinary or DDM-matching regional
subscribers before automatic-resign consequence callbacks, but it does not
emit generic §11.5 service-report interactions or publish
`HLAreportServiceFile`. It does create a local initial-record report file with
immutable joined-federate lifetime and retain an unpublished RTI-owned
joined-federate MOM snapshot with a common object identity, full effective-
attribute metadata, an immutable `HLAfederate` point, and the real report-file
value. Default-region and conveyed-region reuse, and
directed regional callbacks, remain bounded profile gaps. Explicit regional
interaction and object-update delivery now re-evaluates subscription overlap
at the callback boundary when the static delayed-subscription policy is
enabled. The shared explicit-region predicate also implements Umbra's
documented Allow Relaxed DDM policy: when the static federation switch is
enabled, exact boundary-touching committed ranges qualify without removing any
strict overlap; a nonzero gap never qualifies. See
`../design/RELAXED-DDM-POLICY.md`. Broader relaxed-DDM matrices remain separate.
A private recipient-scoped queue foundation
is now integrated into the coordinator, but full time coordination,
callbacks beyond the two listing
reports, three temporal reports, limited object discovery/removal, and limited
receive-order interaction, attribute-update, object-instance/object-class
request/provide delivery, and bounded ownership-query reporting, full object
lifecycle/ownership state,
save/restore, remaining object/attribute DDM region realization and broader routing, directed DDM and remaining timestamped directed delivery, FOM sharing-policy enforcement,
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
races, complete regular and negotiated acquisition, terminal-callback
continuation for Request Attribute Ownership Assumption, the complete
negotiated-divestiture lifecycle beyond a current regular candidate, RTI-owned
state, update-rate reduction,
and the remaining object effects of resign directives remain unimplemented.
A bounded resign-action transition now applies directive 1
(`UNCONDITIONALLY_DIVEST_ATTRIBUTES`) to leave owned attributes unowned and
offer the current eligible recipients, directive 2 (`DELETE_OBJECTS`) to
remove delete-privileged objects, and directive 5
(`CANCEL_THEN_DELETE_THEN_DIVEST`) to resolve the resigning federate's
pending acquisition work before those dispositions. It preserves the
official `FederateOwnsAttributes` and `OwnershipAcquisitionPending`
precondition failures, and applies directive 2 automatically when the
resigning federate is the final joined member. It queues standard
assumption/removal callbacks after the registry lock is released. Bounded
continuation after later publication, discovery, and join is covered; terminal
callback re-search, automatic directives, RTI-owned state, complete action
arbitration, and remote transport remain separate work.
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
the opt-in development profile. That profile now exposes a private embedded
transport endpoint whose one-shot fault path applies the member's Automatic
Resign Directive, removes membership, transitions the ambassador to Not
Connected, and queues the official Connection Lost callback. The endpoint is
an in-process seam for a later socket/IPC transport; it is not itself remote
transport or conformance evidence. Its separate private in-session
membership-control path can instead transition a clean joined federate to Not
Joined while retaining the connection and queueing `Federate Resigned`; that
test-only seam is reserved for a future session watchdog or RTI administration
source.

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
| FederateAmbassador::federateResigned | federate.resigned-by-rti | req-federate-resigned-callback-behavior |

The Disconnect mapping has an exact selected C++ surface and an Umbra Catch2
catalog entry. Its emitted JUnit evidence is raw and unreviewed, so it supports
an implemented status only. The aggregate Connect mapping still has four
candidate C++ surfaces and no Lab-selected aggregate surface; all four
overloads have runtime tests, but none is cataloged or called validated yet.

`compliance/requirements-lab/reference-time-requirements-contract.json` separately pins the
private reference-time implementation to the Lab's standardized-type,
symbolic-name, epsilon, creation-default, and common-implementation records.
Like the FOM composition contract, it is source/test traceability only and
creates no public-service evidence.

`compliance/requirements-lab/federation-management-embedded-requirements-contract.json` pins
the opt-in public adapter symbols to Create, Destroy, Join, Resign, and
connection-precondition source records. Its CTest runs only when the
non-installable development profile is enabled; it is not a service-catalog,
JUnit, protected-review, package, or conformance record.

`compliance/requirements-lab/get-time-factory-api-contract.json` uses the Lab's exact C++ API
surface record—signature and declared exceptions—to trace the same profile's
`getTimeFactory` adapter and Catch2 selector. No Lab implementation mapping is
currently exported for that method, so the contract makes no broader claim.

`compliance/requirements-lab/federate-lookup-api-contract.json` separately pins the exact 2025
C++ declarations and exception sets for `getFederateHandle` and
`getFederateName` to their adapter symbols and active-membership Catch2 case.
The Lab exports no higher-level implementation mapping for those services, so
this is API traceability only, not catalog or conformance evidence.

`compliance/requirements-lab/object-class-lookup-api-contract.json` separately pins the exact
2025 C++ declarations and exception sets for `getObjectClassHandle` and
`getObjectClassName` to their adapter symbols and FOM-catalog Catch2 case. The
case also preserves a previously issued class handle when a compatible
additional-FOM join extends the catalog. The Lab exports no higher-level
implementation mapping for those services, so this remains API traceability
only, not catalog or conformance evidence.

`compliance/requirements-lab/interaction-class-lookup-api-contract.json` separately pins the
exact 2025 C++ declarations and exception sets for `getInteractionClassHandle`
and `getInteractionClassName` to their adapter symbols and FOM-catalog Catch2
case. The case also preserves a previously issued interaction handle when a
compatible additional-FOM join extends the catalog. The Lab exports no
higher-level implementation mapping for those services, so this remains API
traceability only, not catalog or conformance evidence.

`compliance/requirements-lab/attribute-lookup-api-contract.json` separately pins the exact 2025
C++ declarations and exception sets for `getAttributeHandle` and
`getAttributeName` to their adapter symbols and FOM-catalog Catch2 case. The
case resolves inherited attributes through the defining class, preserves an
issued handle after a compatible additional-FOM join, and distinguishes invalid
class/attribute handles from a valid attribute not defined on the supplied
class. The Lab exports no higher-level implementation mapping for those
services, so this remains API traceability only, not catalog or conformance
evidence.

`compliance/requirements-lab/parameter-lookup-api-contract.json` separately pins the exact 2025
C++ declarations and exception sets for `getParameterHandle` and
`getParameterName` to their adapter symbols and FOM-catalog Catch2 case. The
case resolves inherited parameters through the defining interaction class,
preserves an issued handle after a compatible additional-FOM join, and
distinguishes invalid interaction-class/parameter handles from a valid
parameter not defined on the supplied interaction class. The Lab exports no
higher-level implementation mapping for those services, so this remains API
traceability only, not catalog or conformance evidence.

`compliance/requirements-lab/interaction-declaration-api-contract.json` separately pins the
exact 2025 C++ publication and subscription declarations to their adapter
symbols and a real Catch2 lifecycle/FOM-boundary case. A registry-level Catch2
case observes independent per-federate publication/subscription state and its
resign cleanup. That state is now consumed by the separate limited receive-order
delivery path, where passive interaction subscriptions remain declared but do
not arrange callbacks. The ordinary declaration relevance slice additionally walks the
published/subscribed class hierarchies and queues Turn Interactions On/Off for
active transitions, gated by the publisher's Interaction Relevance Advisory
Switch. Its initial value is taken from the composed FDD switch table for each
joining federate; omitted entries use the 1516.2 Disabled default. Regional
advisories, MOM behavior, catalog evidence, and conformance remain outside the
slice.

`compliance/requirements-lab/object-class-attribute-declaration-requirements-contract.json` and
`compliance/requirements-lab/object-class-attribute-declaration-api-contract.json` trace the
four non-region object-class attribute declaration APIs to the source-derived
state records and exact 2025 C++ declarations. The tests cover FOM-backed
available/inherited-handle validation, explicit publication, active/passive
subscription state, passive delivery suppression, class-local removal, and
resign cleanup. They explicitly exclude
the later registration/discovery profile consumes its private declaration state,
including the limited implicit privilege effect at registration time. They
now also feed ordinary Start/Stop Registration relevance advisories planned
from the same effective publication/subscription state and gated by the
publisher's Object Class Relevance Advisory Switch. Its initial value is
seeded from the composed FDD for each joining federate, with omitted entries
defaulting to Disabled. Regional advisory behavior, full ownership, update-rate
enforcement, regions, catalog evidence, and conformance remain outside the
slice.

`compliance/requirements-lab/whole-object-class-declaration-requirements-contract.json` and
`compliance/requirements-lab/whole-object-class-declaration-api-contract.json` trace the two
whole-class teardown services. `unpublishObjectClass` expands over the current
ordinary publication set, removes the implicit delete-privilege publication,
clears matching ownership on registered instances, and makes later updates fail;
`unsubscribeObjectClass` removes only ordinary declarations and leaves the
independent regional declaration map intact. This is still a bounded
development-profile source/API slice: regional whole-class teardown, regional
advisory callbacks, save/restore, transport, package evidence, and conformance
remain outside it.

`compliance/requirements-lab/object-instance-registration-requirements-contract.json` and
`compliance/requirements-lab/object-instance-registration-api-contract.json` trace the exact
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

`compliance/requirements-lab/object-instance-name-reservation-requirements-contract.json` and
`compliance/requirements-lab/object-instance-name-reservation-api-contract.json` trace the exact
2025 single and multiple object-instance name reservation/release services and
their four result callbacks. The embedded registry commits reservations before
callback delivery, rejects empty and `HLA.` names, reports mixed multiple
outcomes, validates multiple release atomically, avoids generated-name
collisions, and releases names on resignation. This is source/API traceability
only; protected review, packaging, catalog evidence, and conformance remain
out of scope.

`compliance/requirements-lab/object-instance-named-registration-requirements-contract.json` and
`compliance/requirements-lab/object-instance-named-registration-api-contract.json` trace the
exact 2025 non-region and regional named registration overloads. The registry
requires reservation ownership, preserves a reservation across a publication
failure, consumes it only after object/name indexes commit, reports the
standard name-in-use/not-reserved exceptions, and retains uniform names and
handles through discovery. This is development-profile source/API traceability
only; timestamped/retraction, broader DDM, ownership transfer,
save/restore, protected review, packaging, catalog evidence, and conformance
remain out of scope.

`compliance/requirements-lab/object-attribute-scope-requirements-contract.json` and
`compliance/requirements-lab/object-attribute-scope-api-contract.json` trace the official 2025
`attributesInScope` / `attributesOutOfScope` callbacks plus the per-federate
Attribute Scope Advisory Switch accessors. The integration case changes
committed subscriber-region overlap and the producing object's update-region
association for one known regional object under both HLA_IMMEDIATE and
HLA_EVOKED, verifies grouped in/out notifications, and proves callback-time
suppression of stale queued transitions. This remains development-profile
source/API traceability only; full timestamped default-region coverage, timestamped/retraction
behavior, packaging, catalog evidence, and conformance remain separate.

`compliance/requirements-lab/attribute-relevance-advisory-requirements-contract.json` and
`compliance/requirements-lab/attribute-relevance-advisory-api-contract.json` pin the separate
Attribute Relevance Advisory boundary. The registry's effective-scope
planner is independent of the Attribute Scope Advisory switch: ordinary and
regional subscriptions, associations, and unassociations can produce a
Turn Updates On/Off plan for the owning federate. An omitted/default
designator selects the official no-rate callback; an explicitly retained
designator selects the rate-bearing overload. At callback entry, the adapter
re-resolves the current retained designator for each still-eligible attribute,
including regional overlap, then checks that federate's Attribute Relevance
switch and revalidates ownership, known-instance state, and current overlap
before invoking the callback. This keeps scope
computation distinct from whether scope callbacks are emitted. Advisories Use
Known Class is now exposed as an FDD-seeded, per-federate read-only switch;
the switch is not yet consumed by complete known-class advisory calculation.
Initial registration/discovery advisories, complete DDM, update-rate
enforcement/reduction, and conformance remain outside the bounded
implementation.

`compliance/requirements-lab/advisories-use-known-class-requirements-contract.json` and
`compliance/requirements-lab/advisories-use-known-class-api-contract.json` pin the official
`getAdvisoriesUseKnownClassSwitch` declaration and the two source-derived
return-value statements. The FOM composer recognizes the 1516.2
`advisoriesUseKnownClass` switch and applies the Disabled omission default;
the registry captures the value at federation creation, returns one shared
value to every member, and retains it across an additional-FOM join. This is still
development-profile source/API traceability only.

`compliance/requirements-lab/update-rate-value-requirements-contract.json` and
`compliance/requirements-lab/update-rate-value-api-contract.json` pin the FDD update-rate
metadata boundary and the two official support queries. `FomCatalog` retains
the composed `updateRates` table; the registry resolves named rates and the
standard `HLAdefault` no-reduction value, while the adapter maps membership,
invalid-designator, known-object, and defined-attribute outcomes to the
official exceptions. Ordinary and regional subscription declarations retain
their normalized FDD designators, and the attribute query returns the
corresponding rate (taking the bounded maximum across applicable regional
declarations) or the default `0.0` when no declaration applies. Subscription
throttling, rate reduction, and MOM/transport remain separate runtime work.
The subscription API contract pins the exact official C++ designator-bearing
declarations.

`compliance/requirements-lab/object-instance-deletion-requirements-contract.json` and
`compliance/requirements-lab/object-instance-deletion-api-contract.json` trace the exact 2025
no-time `Delete Object Instance` and `Remove Object Instance` declarations to
the bounded registry and adapter path. The integration case covers connection,
membership, known-instance and delete-privilege failures; removes the deleting
federate immediately; preserves an evoked recipient's known state until its
callback; and checks tag/producer propagation in both callback models. It does
not claim timestamped/retraction deletion, ownership transfer, save/restore,
resign-action disposition, catalog evidence, or conformance.

`compliance/requirements-lab/local-delete-object-instance-requirements-contract.json` and
`compliance/requirements-lab/local-delete-object-instance-api-contract.json` trace the bounded
2025 `Local Delete Object Instance` transition. Only the invoking federate's
known-instance state is removed; ownership and pending-acquisition preconditions
are enforced; the federation-wide object remains available for rediscovery.
This is development-profile source/API traceability only and does not claim
timestamped/local-delete interactions, DDM, save/restore, remote transport,
catalog evidence, validation, or conformance.

`compliance/requirements-lab/transportation-type-api-contract.json` separately pins the exact
2025 C++ declarations and exception sets for `getTransportationTypeHandle` and
`getTransportationTypeName` to their adapter symbols and Catch2 case. The
non-installable profile recognizes only the mandatory `HLAreliable` and
`HLAbestEffort` pair. The separate transportation-type-change contracts pin
the official change/default/query methods and four callback surfaces to the
per-federate state kernel and a real Catch2 case. Instance attribute changes
commit at confirmation; interaction changes feed future ordinary/regional
sends for the invoking publisher. Neither contract claims custom transportation,
timestamped/local object-lifecycle
delivery beyond the bounded deletion/removal slice, message transport, catalog
evidence, or conformance.

`compliance/requirements-lab/order-type-api-contract.json` and
`compliance/requirements-lab/order-type-requirements-contract.json` pin the corresponding
2025 `getOrderType`/`getOrderName` declarations, exceptions, and legal-name
requirements. The embedded profile recognizes only `Receive` and `TimeStamp`.
Per-instance preferred-order changes and the broader time-management state
machine remain separate work; this lookup slice is source/API traceability
only.

`compliance/requirements-lab/dimension-lookup-requirements-contract.json` and
`compliance/requirements-lab/dimension-lookup-api-contract.json` trace the metadata-only 2025
DDM foundation. `FomCatalog` retains class/interaction dimension associations
and upper bounds, `DimensionHandleDirectory` keeps stable per-federation
values, and the adapter exposes the official available-dimension/name/
upper-bound services.

`compliance/requirements-lab/region-lifecycle-requirements-contract.json` and
`compliance/requirements-lab/region-lifecycle-api-contract.json` trace the metadata-only 2025
region-template/specification lifecycle. The registry and adapter keep
`RegionHandle` ownership, pending and committed `RangeBounds`, complete-commit
validation, support lookups, deletion, and handle decoding aligned with the
official C++ surface. These contracts are source/API traceability only; object/
attribute regional realizations and associations, timestamped/retraction
behavior, broader DDM routing, packaging, catalog evidence, and conformance
remain out of scope.

`compliance/requirements-lab/interaction-region-requirements-contract.json` and
`compliance/requirements-lab/interaction-region-api-contract.json` trace the separate bounded
2025 interaction regional declaration/send boundary. The registry keeps
regional declarations independent, validates owner/context/commit state, and
selects recipients only when a sent region overlaps an active subscribed region
in a shared dimension; passive regional pairs remain declared but do not
arrange delivery. The adapter exposes the official no-time regional overloads
and rechecks queued callbacks. The separate object-attribute regional boundary
is implemented for named and no-name registration, association/unassociation,
active/passive regional subscriptions, active-overlap-filtered no-time
reflection, and the class-level Request
Attribute Value Update With Regions solicitation form. Timestamped regional
sends, realization beyond
explicit associations, and broader DDM routing remain separate work.

`compliance/requirements-lab/receive-order-attribute-update-requirements-contract.json` and
`compliance/requirements-lab/receive-order-attribute-update-api-contract.json` trace the exact
non-timestamped `Update Attribute Values` overload and no-time `Reflect
Attribute Values` callback. The integration scenario covers source ownership,
FOM transportation passels, known-class projection, passive-subscription
suppression, source exclusion, unsubscribe-before-delivery, tag/producer/type propagation,
and both callback models. It deliberately excludes timestamped/retraction
behavior, regions, update-rate reduction, ownership transfer, custom
transportation, FOM sharing-policy enforcement, catalog evidence, and
conformance. RL-013 records the Requirements Lab's current passelization
clause-ownership mismatch without treating it as a standards citation.

`compliance/requirements-lab/attribute-value-update-request-requirements-contract.json` and
`compliance/requirements-lab/attribute-value-update-request-api-contract.json` trace the exact
object-instance `Request Attribute Value Update` overload and matching
`Provide Attribute Value Update` callback. The integration scenario covers the
known-instance/attribute boundary, owner grouping, unowned-attribute
suppression, requester-owner suppression, tag propagation, at-most-one
provider callback, and a queued-provider resignation fence. It deliberately
excludes additional regional request edge cases, automatic provision, an ensuing value update,
timestamped/retraction behavior, DDM, ownership transfer, catalog evidence,
and conformance.

`compliance/requirements-lab/attribute-value-update-response-requirements-contract.json` and
`compliance/requirements-lab/attribute-value-update-response-api-contract.json` separately trace
the bounded provider-response case. A provider invokes the official
non-timestamped `Update Attribute Values` service from inside
`Provide Attribute Value Update`; the requester receives `Reflect Attribute
Values` with the response tag, producing federate, and mandatory transportation
type. This is explicit provider code, not automatic RTI provision, and remains
development-profile source/API traceability only.

`compliance/requirements-lab/auto-provide-requirements-contract.json` and
`compliance/requirements-lab/auto-provide-api-contract.json` separately trace the bounded Auto
Provide switch and its discovery-triggered route. The registry captures the
initial federation-wide value from the creation FDD; after discovery commits
known-instance state, it groups in-scope owned attributes by provider and
queues the official `Provide Attribute Value Update` callback with an empty
RTI-invoked tag. The standard federation-wide `HLAsetSwitches` MOM
interaction now decodes the vendored `HLAswitch` representation and applies a
single locked switch update visible to every current member. The enabled/
disabled and MOM cases are executable development-profile evidence only;
other MOM control/reporting families, complete regional and multi-owner semantics, and
conformance remain open.

`compliance/requirements-lab/object-class-attribute-value-update-request-requirements-contract.json`
and `compliance/requirements-lab/object-class-attribute-value-update-request-api-contract.json`
trace the exact object-class `Request Attribute Value Update` overload and the
same `Provide Attribute Value Update` callback. The integration case validates
the selected class and attributes, expands a base-class request across two
registered subclass instances without requester discovery, preserves the tag,
suppresses requester-owned callbacks, and rechecks queued owner work after
resignation. It deliberately excludes additional regional request edge cases, automatic provision,
an ensuing value update, timestamped/retraction behavior, DDM, ownership
transfer, catalog evidence, and conformance.

`compliance/requirements-lab/attribute-value-update-with-regions-requirements-contract.json`
and `compliance/requirements-lab/attribute-value-update-with-regions-api-contract.json` trace
the exact 2025 class-level `Request Attribute Value Update With Regions` form
and its standard `Provide Attribute Value Update` callback. The integration case
checks committed region ownership/context, empty-pair no-op behavior,
overlap-consistent solicitation, default-region eligibility, copied tags, and
callback-entry rechecks. Its focused companion has the overlap-qualified
provider explicitly invoke non-timestamped `Update Attribute Values` and checks
the requester's response tag, producer, transportation, and optional sent-region
argument. A second focused companion changes the requester subscription to a
valid disjoint range before the queued reflection boundary and verifies
suppression. Provider responses remain explicit user code; the contracts exclude
automatic provision, timestamped/retraction behavior, broader DDM, catalog
evidence, and conformance.

`compliance/requirements-lab/attribute-ownership-query-requirements-contract.json` and
`compliance/requirements-lab/attribute-ownership-query-api-contract.json` trace the exact 2025
`Query Attribute Ownership`, `Inform Attribute Ownership`, and `Attribute Is
Not Owned` declarations. The integration case verifies known-instance and
defined-attribute boundaries, owner/result grouping, the concrete owner handle,
and nullification of queued reports after receive-order removal begins. The
current state model has no RTI-owned branch and does not fabricate one; ownership
acquisition/divestiture, RTI-owned reporting, resign-action disposition, and
conformance remain outside the slice.

`compliance/requirements-lab/attribute-ownership-check-requirements-contract.json` and
`compliance/requirements-lab/attribute-ownership-check-api-contract.json` trace the exact 2025
`Is Attribute Owned By Federate` declaration. Its integration case verifies
that only the invoking current owner receives `true`, while a remote owner and
an unowned attribute yield `false`, with the same official known-instance,
known-class, and removal boundaries. It is read-only and does not broaden the
ownership-transfer claim.

`compliance/requirements-lab/attribute-ownership-acquisition-if-available-requirements-contract.json`
and `compliance/requirements-lab/attribute-ownership-acquisition-if-available-api-contract.json`
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

`compliance/requirements-lab/attribute-ownership-acquisition-requirements-contract.json` and
`compliance/requirements-lab/attribute-ownership-acquisition-api-contract.json` trace the exact
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

`compliance/requirements-lab/attribute-ownership-divestiture-if-wanted-requirements-contract.json`
and `compliance/requirements-lab/attribute-ownership-divestiture-if-wanted-api-contract.json`
trace the exact 2025 C++ `Attribute Ownership Divestiture If Wanted` declaration
and its `Attribute Ownership Acquisition Notification` callback. The Catch2
cases verify owner/known-instance/defined-attribute failures, the exact returned
subset, synchronous transfer to regular and If Available requests, divestiture-tag
propagation, the publication fence through notification entry, stale former-owner
work suppression, and bounded mixed-request follow-up routing. They do not claim
the separate Unconditional/Assumption path, negotiated divestiture,
standards-level arbitration, RTI-owned state, full resign disposition, catalog
evidence, or conformance.

`compliance/requirements-lab/unconditional-attribute-ownership-divestiture-requirements-contract.json`
and `compliance/requirements-lab/unconditional-attribute-ownership-divestiture-api-contract.json`
trace the exact 2025 C++ `Unconditional Attribute Ownership Divestiture`
declaration and `Request Attribute Ownership Assumption` callback. Its Catch2
case verifies full-set owner validation, immediate unowned state, preservation
of existing regular and If Available acquisition paths, one grouped tagged offer to a currently
eligible non-pending federate, suppression for an unpublished/stale candidate,
and a later standard If Available acquisition. It does not claim a continuing
search for later eligible federates, negotiated divestiture/confirmation,
RTI-owned state, full resign disposition, catalog evidence, or conformance.

`compliance/requirements-lab/negotiated-attribute-ownership-divestiture-requirements-contract.json`
and `compliance/requirements-lab/negotiated-attribute-ownership-divestiture-api-contract.json`
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

The companion `compliance/requirements-lab/negotiated-willing-to-acquire-requirements-contract.json`
and `compliance/requirements-lab/negotiated-willing-to-acquire-api-contract.json` isolate the
next bounded state transition. A pending If Available request can be selected
by the private deterministic negotiated planner; its original acquisition tag
is delivered in Request Divestiture Confirmation, and successful confirmation
removes the stale If Available reservation before one normal Acquisition
Notification. The public C++ surface is unchanged. This is still a focused
development-profile policy, not complete owner search, arbitration, or
conformance evidence.

`compliance/requirements-lab/receive-order-interaction-requirements-contract.json` and
`compliance/requirements-lab/receive-order-interaction-api-contract.json` trace the
non-timestamped `Send Interaction` overload and matching no-time `Receive
Interaction` callback to source-derived rules and exact C++ declarations. The
tests cover closest-active-subscribed-class selection, parameter projection in
the private kernel, passive-subscription suppression, a sender exclusion, tag/producer/type
propagation, unsubscribe-before-delivery, and both callback models. They do not
claim timestamped/retraction behavior, regions, FOM
sharing-policy enforcement, custom transportation, catalog evidence, or
conformance.

`compliance/requirements-lab/timestamped-interaction-requirements-contract.json` separately
traces the first bounded public timestamped `Send Interaction`/`Receive
Interaction` path, `Retract`, and official `MessageRetractionHandle` value
encoding. Its Catch2 scenario covers sender lower-bound validation,
retraction-before-grant suppression, exact-bound TSO delivery before
`Time Advance Grant`, and callback timestamp/order fields. The companion
`compliance/requirements-lab/request-retraction-requirements-contract.json` captures the
post-delivery behavior for normal non-regional timestamped interactions,
region-context interactions, attribute updates (including the bounded regional
path), and directed interactions: a legal retraction produces `Request
Retraction` for a delivered recipient while still-queued fanout is suppressed.
It excludes object deletion/removal, alternate advance modes, transport,
package/catalog evidence, and conformance.

`compliance/requirements-lab/timestamped-attribute-update-requirements-contract.json` separately
traces the second bounded public timestamped `Update Attribute Values` /
`Reflect Attribute Values` path, including recipient-specific transportation
passels, sender exclusion, `Retract`, and official timestamp/order/retraction
fields. Its Catch2 scenario covers lower-bound validation,
retraction-before-grant suppression, two-passel exact-bound reflection before
`Time Advance Grant`, and a legal post-delivery `Request Retraction` for an
immediate/nonconstrained recipient with no temporal-queue fanout. It excludes
directed forms, separate region-context evidence, alternate advance modes,
transport, package/catalog evidence, and conformance.

`compliance/requirements-lab/timestamped-regional-attribute-update-requirements-contract.json`
and `compliance/requirements-lab/timestamped-regional-attribute-update-api-contract.json` trace
the bounded regional extension of that attribute-update family. The private
TSO payload retains the recipient's committed update-region projection; the
callback rechecks overlap, known-instance, ownership, subscription, and the
recipient's Convey Region Designator Sets switch at delivery. The Catch2 case
proves lower-bound validation, retraction-before-grant suppression, exact-bound
regional reflection before `Time Advance Grant`, timestamp/order/retraction
fields, pending constrained-recipient suppression, and `Request Retraction`
for the delivered non-time-constrained recipient. The same case also proves
mixed immediate/TSO fanout: the non-time-constrained recipient receives its
timestamped callback immediately while the constrained recipient remains
queued until its grant. Timestamped default-region coverage and a class-level
regional request with one explicit no-time response are separately covered;
additional regional request edges, alternate advance modes, transport,
package/catalog evidence, and conformance remain outside the contract.

`compliance/requirements-lab/timestamped-object-deletion-requirements-contract.json` and
`compliance/requirements-lab/timestamped-object-deletion-api-contract.json` trace the third
bounded public timestamped family: non-regional `Delete Object Instance` /
`Remove Object Instance`. Its private payload preserves the known-recipient
set and an execution-owned invocation snapshot, commits removal before the
recipient's timestamped callback, and excludes the deleting sender from
induced removal. The Catch2 scenarios verify lower-bound rejection,
retraction-before-grant, exact-bound callback ordering, timestamp/order/
retraction propagation, pending-delete reconstitution, and a legal
post-delivery retraction that restores object/name/known state plus committed
split ownership before `Request Retraction` reaches the delivered
nonconstrained recipient. The same test suppresses a constrained recipient's
still-pending removal. A separate case proves a departed delivered owner is not
reconstituted or notified, and its former attribute remains unowned. A focused
terminal no-recipient case then frees the deleted object's name while the
returned designator remains `MessageCanNoLongerBeRetracted`.
An adjacent focused normal-interaction regression now covers one
Disable Time Regulation/re-enable lifetime path at unchanged lookahead.
Complete alternate-advance, broader re-enable, active in-flight ownership,
other resignation, and save/restore recovery evidence, transport,
package/catalog evidence, and conformance remain outside the contract.

`compliance/requirements-lab/timestamped-directed-interaction-requirements-contract.json` and
`compliance/requirements-lab/timestamped-directed-interaction-api-contract.json` trace the
fourth bounded public timestamped family: non-regional directed interaction
send/receive with a known target and recipient-specific projection. The private
payload preserves target, declaration, transportation, tag, and retraction
state; the Catch2 scenarios verify lower-bound validation, retraction-before-
grant, exact-bound callback ordering, timestamp/order/retraction propagation,
pending constrained-recipient suppression, and post-delivery `Request
Retraction` for a nonconstrained directed recipient. The shared recipient
  planner honors the ownership/universal subscription selector. A focused
  timestamped selector scenario now distinguishes both modes for known target
  recipients and rechecks a selector change plus an unsubscription before
  later grants. Directed DDM, alternate advance modes, region-context
  evidence, transport, package/catalog evidence, and conformance remain
  outside the contract.

`compliance/requirements-lab/timestamped-regional-interaction-requirements-contract.json` and
`compliance/requirements-lab/timestamped-regional-interaction-api-contract.json` trace the fifth
bounded public timestamped family: region-context interaction send/receive
using committed strict-overlap routing. Its TSO payload preserves the sent
region set, and the Catch2 scenarios verify lower-bound validation,
retraction-before-grant, exact-bound callback ordering, sent-region
propagation, timestamp/order/retraction fields, pending constrained-recipient
suppression, and post-delivery `Request Retraction` for a nonconstrained
overlap-qualified recipient. Object-region services, relaxed DDM, alternate
advance modes, directed regional interactions, save/restore, transport,
package/catalog evidence, and conformance remain outside the contract.

`compliance/requirements-lab/directed-interaction-requirements-contract.json` and
`compliance/requirements-lab/directed-interaction-api-contract.json` trace the bounded
non-timestamped, non-DDM `Send Directed Interaction` / `Receive Directed
Interaction` path and its six object-class declaration overloads. The real
Catch2 cases cover known-target discovery, sender exclusion, immediate and
evoked callbacks, selective unsubscribe, source unpublication, stale callback
suppression, republishing, tag/producer/mandatory-transport propagation, and
the ownership/universal selector: default by ownership, universal delivery,
empty-set preservation, and supplied-class mode changes. They do not claim
timestamped/retraction behavior beyond the separate bounded contract,
directed-DDM behavior, ordering, FOM sharing-policy enforcement, catalog
evidence, validation, or conformance.

`compliance/requirements-lab/federation-listing-requirements-contract.json` and
`compliance/requirements-lab/federation-listing-api-contract.json` trace the same profile's List
Federation Executions / List Federation Execution Members adapter calls and the
three FederateAmbassador report callbacks. They deliberately keep the
source-derived requirement records separate from the Lab's exact API records,
because the Lab exports no higher-level implementation mapping for this slice.

`compliance/requirements-lab/synchronization-point-requirements-contract.json` and
`compliance/requirements-lab/synchronization-point-api-contract.json` trace the bounded
registration, announcement, achievement, late-join, and Federation Synchronized
callbacks under both callback models: the direct regression fixes the immediate
registration, announcement, and completion boundaries, while the evoked case
retains the late-join and duplicate-registration paths. They remain
development-profile source/API traceability only; FOM/SOM
synchronization-table enforcement, distributed transport, save/restore
interaction, protected review, and conformance are separate work.

`compliance/requirements-lab/time-advance-requirements-contract.json` and
`compliance/requirements-lab/time-advance-api-contract.json` trace joined-federate initial time,
Time Advance Request / Grant, and Query Logical Time through source-derived
requirements and exact C++ declarations. The implementation is deliberately
limited to the embedded no-TSO path, so these contracts make no federation-wide
time-management or conformance claim.

The paired `time-advance-request-available-*` and
`next-message-request-available-*` contracts trace the two Available forms to
their exact 2025 declarations and the shared queued-TSO grant scenario. They
make the strict TAR/NMR versus inclusive TARA/NMRA GALT distinction explicit;
they remain development-profile traceability only.

`compliance/requirements-lab/flush-queue-request-requirements-contract.json` and
`compliance/requirements-lab/flush-queue-request-api-contract.json` trace the bounded
Flush Queue Request/Grant path, including its queued-TSO flush and optimistic
time floor. They remain development-profile traceability only.

`compliance/requirements-lab/asynchronous-delivery-requirements-contract.json` and
`compliance/requirements-lab/asynchronous-delivery-api-contract.json` trace the bounded
Enable/Disable Asynchronous Delivery services. Catch2 covers the default
disabled state, deferred receive-order callback release while idle or
time-advancing, and the official already-enabled/already-disabled and
membership/connection boundaries. Timestamped delivery, MOM reporting,
remote transport, save/restore persistence, package evidence, protected review,
and conformance remain outside this slice.

`compliance/requirements-lab/time-role-requirements-contract.json` and
`compliance/requirements-lab/time-role-api-contract.json` separately trace the callback-gated
Enable/Disable Time Regulation, Enable/Disable Time Constrained, and Query
Lookahead slice. `compliance/requirements-lab/time-bounds-requirements-contract.json` and
`compliance/requirements-lab/time-bounds-api-contract.json` trace the separate read-only no-TSO
Query GALT/Query LITS slice, including its exclusive zero-lookahead TAR
boundary. `compliance/requirements-lab/time-grant-scheduler-requirements-contract.json` traces
the limited TAR release path that consumes those bounds. These source/API
references do not establish timestamped-message, full time-coordination,
package, or conformance behavior.

`compliance/requirements-lab/modify-lookahead-requirements-contract.json` and
`compliance/requirements-lab/modify-lookahead-api-contract.json` trace the bounded Modify
Lookahead transition. The private time state applies increases immediately and
decreases by elapsed logical time at grant boundaries; remaining advance modes,
save/restore, transport, package, and conformance remain outside the slice.

`compliance/requirements-lab/next-message-request-requirements-contract.json` and
`compliance/requirements-lab/next-message-request-api-contract.json` trace the bounded Next
Message Request transition. The adapter separates the caller's requested
boundary from the effective currently queued TSO target and reuses the grant
dispatcher to deliver the equal-timestamp cohort before `Time Advance Grant`.
Future transport input, Flush Queue future-input coordination, and full
coordination remain outside the slice.

`compliance/requirements-lab/timed-save-requirements-contract.json` and
`compliance/requirements-lab/timed-save-api-contract.json` trace the bounded timestamped
Request Federation Save overload and its exact timestamped
`FederateAmbassador::initiateFederateSave(label, LogicalTime)` callback. The
registry stores one replaceable pending request using official LogicalTime
values, checks all current time-constrained members, and releases the ordinary
save callback fanout only after queued and in-transit TSO payloads at or below
the requested boundary have completed. This is an in-memory embedded development slice; timed
restore, durable persistence, complete save interlocks, transport, package
evidence, and conformance remain separate work.

These references identify design intent and scoped implementation traceability;
they are not evidence of overall standards conformance. The source corpus
remains external to this repository and is not copied here.
