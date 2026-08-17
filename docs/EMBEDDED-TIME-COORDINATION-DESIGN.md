# Embedded time coordination boundary

## Purpose

This document prevents the initial time-management implementation from being
mistaken for a complete HLA time coordinator. Umbra uses only the official IEEE
1516.1-2025 `LogicalTime`, `LogicalTimeInterval`, and factory interfaces. Its
private state is an implementation detail behind the standard C++ binding, not
a replacement time model.

## Implemented no-TSO temporal control

The non-installable `UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=ON` profile
has one `FederateTimeState` for each joined federate. It supports these
callback-gated transitions and read-only bounds:

- Time Advance Request -> Time Advance Grant;
- Enable Time Regulation -> Time Regulation Enabled, with an official copied
  lookahead available through Query Lookahead;
- Disable Time Regulation;
- Enable Time Constrained -> Time Constrained Enabled; and
- Disable Time Constrained;
- Query GALT; and
- Query LITS.

The state mutation happens immediately before the matching callback starts.
Until then, the request is pending. A pending regulation or constrained request
causes Time Advance Request to report the corresponding official pending
exception. Resign deactivates the state, which turns any already queued grant
or enable callback into a no-op before it can call a stale federate ambassador.

## Limited no-TSO TAR scheduling

Umbra has a limited cross-federate scheduler for Time Advance Request only.
The registry retains the accepted private TAR and evaluates each pending
request from one federation-owned snapshot. A time-constrained requester is
released only when it is strictly below a defined GALT, or when GALT is
undefined and the FDD's Non-Regulated-Grant switch permits it. A
non-time-constrained requester is not GALT-bounded. Relevant temporal changes
(a TAR request or grant, a regulation callback/disable, constrained-mode
disable, resignation, or an additional-FOM definition replacement) cause a
re-evaluation.

The scheduler returns opaque dispatch actions while holding its registry lock,
but invokes them only after the caller releases runtime locks. The action is
queued on the owning ambassador's callback dispatcher and rechecks the shared
bound immediately before it mutates logical time and invokes Time Advance
Grant. This prevents an old callback from granting a request that a newer
state change made ineligible.

## Federation-owned private TSO coordination

Umbra now has a private `TsoMessageQueue` plus a federation-owned
`FederationTimeCoordinator` boundary. The queue stores only official
`LogicalTime` objects and federation identities. One federation-local message
id can own several recipient entries, and a monotonically increasing sequence
number gives equal-timestamp entries a stable order. The coordinator exposes
three private states in `FederationTimeExecutionSnapshot`: queued entries,
entries in transit to a callback, and entries delivered since the recipient's
last time advance. Retraction stops at the first delivery boundary, and resign
or disconnect cleanup removes recipient-owned pending and delivery state.

The private registry hooks are consumed by the GALT/LITS calculator. GALT
accounts for delivered-since-last-advance and queued/in-transit timestamps;
LITS is the least future queued/in-transit timestamp constrained by GALT, and
can remain defined from an incoming queue when GALT is unregulated. Catch2
tests cover fanout, retraction, callback completion, snapshot projection, and
the unregulated-LITS case. The queue and coordinator contracts pin their
narrow source/test traceability to the 2025 Lab export. The first public
timestamped interaction slice below now consumes this foundation without
turning the private kernel into a replacement API.

## First bounded public timestamped interaction slice

The embedded development profile now exposes the official
`RTIambassador::sendInteraction(..., LogicalTime const&)` overload for one
non-regional interaction family, plus `retract` and
`decodeMessageRetractionHandle`. A time-regulating sender is checked against
its current logical time (or requested time while a TAR is pending) plus the
copied lookahead. The selected official timestamp is retained in the
federation-owned payload record; time-constrained recipients are queued by the
existing coordinator and delivered at their grant boundary. The grant
dispatcher invokes the timestamped `receiveInteraction` callback before that
recipient's `timeAdvanceGrant`, including `TIMESTAMP`/`TIMESTAMP` order values
and an official retraction handle.

The Catch2 scenario covers the invalid lower-bound case, retraction before a
grant, exact-bound delivery, callback ordering, timestamp/order propagation,
and the terminal post-delivery retraction exception. The Requirements Lab
traceability for this slice is
`compliance/timestamped-interaction-requirements-contract.json`.

This is deliberately not a complete timestamped service family. The current
slice excludes timestamped object deletion, directed or regional interactions,
alternate advance modes, asynchronous delivery, request-retraction callbacks,
remote transport, and partial fanout/retraction reconciliation. A
non-time-constrained recipient uses the bounded timestamped callback
conversion path, but the queue-backed conformance boundary is only claimed for
constrained recipients in this tranche.

## Second bounded public timestamped attribute-update slice

The same development profile now exposes the official timestamped
`RTIambassador::updateAttributeValues(..., LogicalTime const&)` overload for
non-regional object updates. The accepted update retains recipient-specific
transportation passels and value payloads beside one federation-wide TSO
message id. Time-constrained recipients consume those passels at the grant
boundary; the dispatcher rechecks known-instance, ownership, and subscription
projection immediately before each timestamped `reflectAttributeValues`
callback and completes the queue entry even when user code throws.

The Catch2 scenario covers sender lower-bound rejection, pending retraction,
two transportation passels, exact-bound reflection before `Time Advance Grant`,
timestamp/order/retraction propagation, sender exclusion, and terminal
post-delivery retraction. Its Requirements Lab traceability is
`compliance/timestamped-attribute-update-requirements-contract.json`.
This remains a bounded non-regional service: directed or
regional updates, alternate advance modes, request-retraction callbacks,
transport, and conformance are not implied.

## Third bounded public timestamped object-deletion slice

The development profile now exposes the official non-regional timestamped
`RTIambassador::deleteObjectInstance(..., LogicalTime const&)` overload and
matching timestamped `FederateAmbassador::removeObjectInstance` callback. The
registry snapshots the known recipients and records a pending-delete marker
without destroying the object. Time-constrained recipients consume the typed
payload at their grant boundary; other recipients use the same accepted
payload through the bounded immediate timestamped callback path. The first
removal commits the federation-wide deletion, removes the sender's known
instance without inducing a sender callback, and advances each recipient to
unknown at its own callback boundary.

Retracting before any removal callback withdraws the queue fanout and clears
the pending marker, so the previously known object is reconstituted for the
recipient. The Catch2 scenario covers the sender lower bound, retraction and
reconstitution, exact-bound removal before `Time Advance Grant`, callback
ordering, timestamp/order/retraction propagation, sender exclusion, and the
terminal post-delivery retraction exception. Its Requirements Lab contracts
are `compliance/timestamped-object-deletion-requirements-contract.json` and
`compliance/timestamped-object-deletion-api-contract.json`.

This is deliberately a bounded non-regional deletion family. Mixed
immediate/TSO fanout retraction reconciliation, directed or regional deletion
forms, alternate advance modes, request-retraction callbacks, transport,
save/restore, package/catalog evidence, and conformance remain outside the
slice.

## Remaining deliberate limits

Umbra still implements no general remaining timestamped object/attribute
service family beyond the three bounded non-regional consumers, alternate
advance mode, transport, or asynchronous-delivery algorithm. With
no queued/in-transit messages, GALT/LITS retain the existing other-regulator
lookahead behavior. A forward TAR by a zero-lookahead regulator makes that
lower timestamp exclusive, so the selected time factory's epsilon is added.
With no other regulator and no incoming TSO message, both public query results
remain undefined. The limited TAR scheduler and the three bounded public TSO
slices are still narrower than full time management.

This is a narrow staged invariant, not a claim that the algorithm would remain
correct after adding TSO traffic. No production SDK package, catalog result,
JUnit sidecar, protected review, or standards-conformance claim follows from
this slice.

## Federation-owned coordination foundation

The development profile now commits a runtime-backed federate's
`FederateTimeState` in the same private registry transaction as its membership.
`EmbeddedFederationRegistry::timeSnapshotFor` exposes a single immutable view
of the composed definition (including the FDD Non-Regulated-Grant setting),
all runtime-backed members' current/pending logical times, roles, and
actual/requested lookaheads, and each recipient's queued/in-transit/delivered
TSO state. This makes the coordinator's input ownership explicit and makes
resign remove both the time entry and recipient-owned queue state with the
membership.

The next temporal layers must extend this coordinator with:

1. delay or select enable callback times according to the standard's
   cross-federate constraints;
2. release TSO messages and grants in an order compatible with the recipient's
   time-constrained state and asynchronous-delivery mode;
3. apply decreasing lookahead changes gradually as logical time advances; and
4. make resign, disconnect, save/restore, and transport failure cancel or
   reconcile pending work without dereferencing caller-owned ambassadors.

Only after those invariants are specified should Umbra add the remaining
timestamped object/attribute services, Time Advance Request Available, Next
Message Request, Flush Queue Request, Modify Lookahead, request-retraction
callbacks, or broader transport behavior.

## Traceability boundary

`compliance/time-advance-*.json`, `compliance/time-role-*.json`,
`compliance/time-bounds-*.json`,
`compliance/time-grant-scheduler-requirements-contract.json` use the adjacent Requirements Lab's
source-derived requirements and exact 2025 C++ API surfaces. They are CTest
checks for traceability drift only. The Lab currently exports no higher-level
implementation mapping for these temporal surfaces, so the records must stay
outside the test catalog until an appropriate mapping, raw evidence, and
protected review exist.

The private queue foundation is traced by
`compliance/tso-message-queue-requirements-contract.json`, and its temporal
coordinator integration is traced by
`compliance/federation-time-coordination-tso-requirements-contract.json`.
The first bounded public interaction slice is traced by
`compliance/timestamped-interaction-requirements-contract.json`; the second
bounded public attribute-update slice is traced by
`compliance/timestamped-attribute-update-requirements-contract.json`. These
candidate records are source/test anchors only and do not promote package or
conformance evidence.
