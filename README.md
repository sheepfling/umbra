# Umbra

Umbra is a standards-first C++20 foundation for building an HLA Run-Time
Infrastructure (RTI). Its current state is an official IEEE 1516.1-2025 API
baseline plus small embedded connection, callback-control, federate, object-class,
interaction-class, inherited-attribute, inherited-parameter, dimension/upper-bound, region/range-state, transportation-type lookup, and
the mandatory Receive/TimeStamp order-type lookup,
interaction declaration, bounded directed-interaction delivery, object-class attribute declaration, limited object-instance registration/discovery/deletion, limited receive-order and bounded timestamped interaction, attribute-update/reflection, object-deletion, directed-interaction, and region-context delivery, limited object-instance and object-class request/provide attribute-value delivery, federation-listing, and temporal-control slices, with an opt-in
federation-management development profile—not a complete RTI. Python
and Java, if added later, will be adapters around the native C++ implementation.

The official factory, `rtiName`, and `rtiVersion` link from `umbra::rti`.
That target also provides the two mandated reference logical-time types and
factories; the standard `libfedtime` forwarding entry point is packaged as the
separate static `umbra::fedtime` target. The default packaged profile keeps
public `getTimeFactory`, federate, object-class, interaction-class, attribute, parameter, transportation-type, and order-type lookup, interaction declaration, object-class attribute declaration, receive-order interaction and attribute-update/reflection, object-instance and object-class request/provide attribute-value update, federation creation,
object-instance registration/discovery/deletion, and time-management services unavailable.
`RTIambassador::connect` (all four official C++ overloads), `disconnect`,
`evokeCallback`, `evokeMultipleCallbacks`, `enableCallbacks`, and
`disableCallbacks` have real single-process behavior. The callback controls
operate on a private dispatcher. The default packaged profile has no federation
event producer.
Every other RTI service remains explicitly unavailable through
`rti1516_2025::RTIinternalError` in that profile. An explicit, non-installable
source-tree profile can additionally execute Create/Destroy/Join/Resign through
the official methods after MIM-first XML/XSD validation, FDD materialization,
and reference-time selection. It also returns a caller-owned `getTimeFactory`
for the joined federation's selected reference implementation, resolves joined
federate names to active handles and returned active/departed handles to stable
names within that federation, resolves object-
and interaction-class names and handles plus inherited attributes and parameters
from its current composed FOM catalog. The embedded profile also implements
the three official 2025 order-control services: per-federate class defaults
are captured by future object instances, explicit instance changes affect
future owned-attribute updates, ownership transitions reset preferred order
from the acquiring federate's default, and a publisher-scoped interaction
override affects future ordinary, regional, and timestamped sends. Mixed
Receive/TimeStamp delivery is exercised by Catch2; the source/API contracts
remain development-profile traceability rather than conformance evidence. It
retains independent per-federate
interaction publication and subscription declarations and implements the
non-timestamped, non-region `Send Interaction` overload with the matching
no-time `Receive Interaction` callback. It also implements the three bounded
2025 regional-interaction services: independent regional subscription and
unsubscription plus no-time `Send Interaction With Regions`. Regional delivery
uses committed official region ranges and the 2025 region-set overlap rule;
ordinary and regional subscriptions remain independent, passive subscriptions
cannot arrange delivery, and an empty sent region set produces no callback.
That limited path selects each receiver's closest active subscribed class,
projects available parameters, suppresses the sender,
rechecks a queued receiver's subscription, preserves the tag and producing
federate, and uses the receiver's immediate or evoked callback model. It also
implements the bounded 2025 object-class directed-interaction declarations and
the non-timestamped, non-DDM `Send Directed Interaction` overload with the
matching no-time `Receive Directed Interaction` callback. A directed send is
planned only for a known target object and a declared directed publication /
subscription pair; the sender is excluded, the tag, producer, and mandatory
FOM-selected transportation are preserved, and queued delivery rechecks target
lifecycle and declaration state. A missing or false `universally` selector is
by ownership, while true is universal; the empty class-set and supplied-class
mode boundaries are covered by Catch2. Directed DDM and conformance remain
outside this bounded semantic slice. It also
retains explicit publication and active/passive subscription state for available
object-class attributes, including inherited handles; only active declarations
arrange discovery, scope, or reflection. It supports the unnamed
`Register Object Instance` overload, generated private object-instance names,
non-region discovery to eligible actively subscribed federates, and the three official
known-instance lookup methods. It also implements the bounded 2025 object-instance
name reservation services: single and multiple reservations reject empty or
`HLA.` names, report success/failure through the official callbacks, release
owned names atomically, and return names to the federation-wide pool on
resignation. The named `Register Object Instance` overloads now require and
consume a reservation owned by the registering federate, preserve a reservation
across a publication failure, and report the standard name-in-use/not-reserved
exceptions. The named regional overload consumes the same reservation state
after regional validation.
It also implements the no-time `Delete Object
Instance` overload and matching no-time `Remove Object Instance` callback: the
current `HLAprivilegeToDeleteObject` owner may delete, the deleting federate is
immediately unknown, other known recipients are removed through their selected
callback model, and the tag and producing federate are retained. It also
implements the bounded 2025 `Local Delete Object Instance` service: only the
invoking federate forgets the known instance, ownership/acquisition preconditions
are enforced, and the federation-wide object can later be rediscovered. This
does not claim timestamped local-delete interactions or broader DDM. The same 2025 FOM
catalog retains dimension associations and upper bounds, allocates stable
`DimensionHandle` values, and implements the official available-dimension,
name, and upper-bound lookup services. It also implements the metadata-only
2025 region-template/specification lifecycle: official `RegionHandle` and
`RangeBounds` values, create/commit/delete, owner-scoped dimension/range support
lookups, pending range updates, complete-commit validation against FOM dimension
upper bounds, and handle decoding. It also implements a bounded 2025
object-attribute regional slice: no-name `Register Object Instance With Regions`,
additive `Associate Regions For Updates`/`Unassociate Regions For Updates`, and
regional `Subscribe/Unsubscribe Object Class Attributes With Regions`. Committed
active region sets gate discovery and no-time attribute reflection by overlap,
ordinary and regional declarations remain independent, passive triples remain
declared without arranging delivery, empty region sets are no-ops, and
the optional sent-region set reaches the official reflection callback. The
per-federate Attribute Scope Advisory Switch and official `attributesInScope` /
`attributesOutOfScope` callbacks now track committed overlap, update-region
association, and ordinary/regional subscription transitions for known object
instances, with grouped
immediate/evoked delivery and callback-time stale-work suppression.
Default-region synthesis, broader DDM routing, and timestamped/retraction
behavior for regional object-attribute forms remain outside this slice. It also
implements the no-time `Update Attribute Values` overload
and matching no-time `Reflect Attribute Values` callback. That narrow path
requires source ownership, keeps each FOM transportation passel separate,
projects values at each receiver's known class and current active subscription,
suppresses the source, rechecks queued delivery, and preserves the tag,
producer, and mandatory transportation type through either callback model.
It also implements the object-instance `Request Attribute Value Update`
overload and matching `Provide Attribute Value Update` callback. The requester
must know the instance and requested attributes at its known class; currently
owned attributes are grouped by provider, requester-owned attributes do not
induce its own callback, unowned attributes induce none, the tag is preserved,
and a queued provider is rechecked before invocation. A bounded response test
also lets the provider invoke non-timestamped `Update Attribute Values` from
inside `Provide Attribute Value Update` and verifies requester-side `Reflect
Attribute Values`, including tag, producer, and mandatory transport. This is
explicit provider behavior, not RTI-automatic provision.
The sibling object-class overload validates the selected class and attributes,
then expands across current instances registered at that class and its
subclasses without requiring those instances to be known to the requester. It
groups callbacks per provider and particular object instance, preserving the
same tag and delivery recheck boundary.
The class-level 2025 `Request Attribute Value Update With Regions` overload is
also wired through the same registry: committed request regions filter provider
solicitations by update-region overlap, default-region attributes remain
eligible, empty region pairs are no-ops, and invalid region ownership/context
maps to the official exceptions. The provider still explicitly chooses whether
to answer through `Update Attribute Values`; this does not claim automatic
provision, resulting reflection beyond that response path, broader DDM, or
conformance.
It also implements the bounded `Query Attribute Ownership` service for the
actual ownership states the current 2025 runtime can represent: a joined
federate owner is reported through `Inform Attribute Ownership`, and an
available attribute through `Attribute Is Not Owned`. The requester must know
the instance and requested attributes at its known class; reports are grouped
by result and a queued report is nullified when `Remove Object Instance` begins.
It deliberately does not invent an RTI-owned sentinel: that state awaits the
ownership acquisition/divestiture lifecycle.
The adjacent `Is Attribute Owned By Federate` service is a read-only boolean
over that same snapshot: it answers only whether the invoking joined federate
owns one available attribute of an instance it knows, with no callback or
transfer effect.
The bounded 2025 `Attribute Ownership Acquisition If Available` service now
records a private pending willing-to-acquire request, transfers only an
attribute that remains unowned immediately before its matching `Attribute
Ownership Acquisition Notification` callback, and reports a joined remote
owner through `Attribute Ownership Unavailable` without inducing a release
callback. A repeat for an already-pending attribute leaves that WTA state
unchanged, while an additional eligible attribute is separately pending;
`Unpublish Object Class Attributes` rejects withdrawal of either required
publication until its terminal callback. The path preserves the user tag and
nullifies an undelivered request when the requester resigns or receive-order
removal begins.
The adjacent bounded 2025 regular `Attribute Ownership Acquisition` path keeps
a separate private pending request. It transfers an unowned attribute at its
`Attribute Ownership Acquisition Notification` callback, asks a joined remote
owner through `Request Attribute Ownership Release` with the original
acquisition tag, and does not duplicate that callback for a repeated request.
`Attribute Ownership Release Denied` preserves the owner's attribute and
terminates matching regular requests through `Attribute Ownership Unavailable`
with the denial tag. `Cancel Attribute Ownership Acquisition` moves a
still-pending regular request to a separate cancellation reservation, makes
queued acquisition/release work stale, and retains required publication until
`Confirm Attribute Ownership Acquisition Cancellation` begins. It covers only
the accepted serial cancellation path, not a complete regular-acquisition
lifecycle.
The paired bounded 2025 `Attribute Ownership Divestiture If Wanted` path
returns only attributes for which a still-pending regular or If Available
acquirer is selected, transfers those attributes synchronously, and passes the
divestiture tag to `Attribute Ownership Acquisition Notification`. The selected
acquirer's publication remains protected until that callback starts; stale work
addressed to the former owner is suppressed and later regular requests are
replanned against the new owner. Its mixed-request selection is an explicit
deterministic development-profile policy, not an IEEE arbitration claim.
The adjacent bounded 2025 `Unconditional Attribute Ownership Divestiture`
path makes its fully validated supplied set unowned immediately. It keeps
existing regular and If Available requests on their standard callback paths
and sends one grouped `Request Attribute Ownership Assumption` offer to each
currently eligible non-pending federate, with the divestiture tag. That offer
does not transfer ownership; the candidate must subsequently invoke an
acquisition service. Before callback entry, Umbra rechecks the candidate's
known-instance, publication, pending-state, and unowned boundaries. This is
not yet a continuing owner-search loop for federates that become eligible
after the original divestiture call.
The bounded 2025 `Negotiated Attribute Ownership Divestiture` transition keeps
the current owner while the attribute waits, selects an already pending regular
acquirer, and sends the owner `Request Divestiture Confirmation` with the
regular acquisition tag. `Confirm Divestiture` transfers the confirmed
attribute synchronously to that acquirer and sends `Attribute Ownership
Acquisition Notification` with the confirmation tag. `Cancel Negotiated
Attribute Ownership Divestiture` removes the private waiting state and returns
the attribute to ordinary regular-release planning. The implementation also
handles a later regular acquisition, `NoAcquisitionPending` after its
cancellation, and stale queued callback suppression. It deliberately does not
yet perform the complete 2025 owner search or select a willing-to-acquire
candidate.
It deliberately does not include additional regional request edge cases,
  default-region synthesis, timestamped/retraction, broader DDM
  region associations/realizations
  and routing, full regular or negotiated
acquisition, the continuing unconditional owner-search lifecycle, the complete
negotiated-divestiture lifecycle beyond this regular-candidate transition,
update-rate reduction, FOM sharing policy, save/restore,
distinct resign-action object disposition, RTI-owned ownership reports, further
regional request edge cases, or automatic provision without federate code.
Joined federates can also resolve only the
mandatory 2025 `HLAreliable` and `HLAbestEffort` transportation-type names and
handles and the mandatory 2025 `Receive` and `TimeStamp` order names/types.
The embedded profile now implements bounded attribute default/change/
query services and interaction change/query services: instance defaults are
captured per federate, accepted changes commit at their confirmation callback,
and future ordinary/regional interaction sends and attribute updates use the
effective type. Remaining timestamped/retraction behavior beyond
these bounded services,
further regional request edge cases, broader object/attribute DDM
region lifecycle/routing, the FDD-representability issue for a complete
multi-class directed-subscription matrix and remaining timestamped
directed-interaction forms,
directed-interaction transport policy, FOM sharing-policy enforcement, custom transportation, timestamped/local
object-lifecycle delivery beyond the bounded deletion/removal slice, and message
transport remain unimplemented. It implements
List Federation Executions / List Federation Execution Members with their
official report callbacks in immediate and evoked modes. It also initializes a
joined federate's selected logical time; implements Time Advance Request and
the Available forms / Query Logical Time / Time Advance Grant; callback-gates
Enable/Disable Time Regulation, Enable/Disable Time Constrained, Query
Lookahead, bounded Modify Lookahead, the currently-queued-message forms of
Next Message Request, and the bounded Flush Queue Request/Grant pair; and
exposes
read-only Query GALT / Query LITS in the current bounded public TSO profile. Those
bounds use other regulators' current or pending time plus actual lookahead and,
inside the private coordinator, queued/in-transit/delivered TSO timestamps;
the selected factory's epsilon handles a forward zero-lookahead TAR boundary.
There are bounded public TSO send/receive services for interactions,
attribute updates, object deletion/removal, directed interactions, and
region-context interactions. Next Message Request can select a currently
queued TSO timestamp and deliver its cohort before the grant. The Available
forms apply the inclusive defined-GALT rule, but only the currently queued
in-process input is considered. Flush Queue Request/Grant now flushes the
current in-process queue and reports its optimistic-time floor; future
transport input and broader coordination are not implemented. A limited
scheduler can release TAR callbacks across federates only when its GALT/NRG policy allows;
it is not a full time coordinator. The development profile now has a private
embedded one-shot transport-fault path that applies Automatic Resign cleanup,
queues the official Connection Lost callback, and permits reconnect. A separate
private in-session control seam now removes a clean joined member while keeping
its connection alive and queues the official Federate Resigned callback; it is
test-only until a real session/admin source exists. Remote transport and other
federation-event callbacks remain open. Complete object/ownership state,
broader time-management services, and a conformance claim remain out of scope.
The repository makes no whole-RTI or
standards-conformance claim; the implemented Disconnect slice has only raw,
unreviewed Requirements-Lab evidence.

## Build and test

Requirements: CMake 3.23+, a C++20 compiler, and Python for the optional
header-integrity test.

```powershell
cmake -S . -B .build -G "Visual Studio 17 2022" -A x64
cmake --build .build --config Debug
ctest --test-dir .build -C Debug --output-on-failure
```

The baseline compiles the complete official header tree and verifies its
committed SHA-256 file set. It also checks the generated inventory of 244
official pure virtual members, verifies that all 109 official exception types
have binding definitions, and checks the private fallback ambassador. Tests use
the official NullFederateAmbassador callback helper. To add the pinned Catch2
v3.15.3 test runner, configure with `-DUMBRA_FETCH_CATCH2=ON`; the dependency is
fetched only into the ignored build directory. Catch2 exercises the real
embedded connection and callback-control behavior, the official reference-time
values/factories and encodings, private federation/handle kernel invariants,
all four official C++ Connect overloads, and—in the development profile—the
standards-shaped federation-listing, time-advance, and metadata-only region
template/range paths.
CTest also installs and consumes the exported `umbra::rti` and
`umbra::fedtime` targets in a clean package-smoke build.

To exercise the development-only federation-management vertical slice, use a
separate build tree. It is deliberately non-installable until libxml2 and the
vendored resource path are part of the SDK dependency contract.

```powershell
cmake -S . -B .build-fom-services -G "Visual Studio 17 2022" -A x64 `
  -DUMBRA_FETCH_CATCH2=ON `
  -DUMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON `
  -DUMBRA_FETCH_LIBXML2=ON `
  -DUMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=ON
cmake --build .build-fom-services --config Debug -- /m:1
ctest --test-dir .build-fom-services -C Debug --output-on-failure
```

The unmodified official IEEE 1516.1-2025 C++ headers are vendored under
[`third_party/ieee1516.1-2025`](third_party/ieee1516.1-2025) and are compiled
by the `umbra.ieee1516_2025.headers` smoke test. They are Umbra's only public
API baseline. The implementation provides the official `rti1516_2025` symbols
directly.

The immutable 2025 DIF/FDD/OMT schemas, standard MIM, and supplied example
FOMs are also vendored under
[`third_party/ieee1516.2-2025`](third_party/ieee1516.2-2025). They are
digest-checked runtime inputs for the private XML/XSD validator, Annex C-guided
module-composition preflight, and schema-validated FDD materializer. The FDD
artifact is private—not an Umbra-owned replacement format. It is consumed only
by the opt-in development profile and does not make the packaged public
Create/Join services available.

See [the architecture](docs/ARCHITECTURE.md),
[the implementation roadmap](docs/ROADMAP.md), and
[the native implementation plan](docs/IMPLEMENTATION-PLAN.md) for boundaries,
sequencing, and completion gates. The [requirements and testing
guide](docs/REQUIREMENTS-AND-TESTING.md) explains how Umbra consumes the
adjacent HLA Requirements Lab and how native Catch2 results flow through its
portable compliance contract. See the [compliance
workflow](docs/COMPLIANCE-WORKFLOW.md) for the raw-to-verified promotion path
and the [FOM validation design](docs/FOM-VALIDATION-DESIGN.md) for the next
federation-management gate. The [logical-time design](docs/LOGICAL-TIME-DESIGN.md)
records the standards-first time-library boundary before that gate is opened.
