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

- Time Advance Request / Time Advance Request Available -> Time Advance Grant;
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

## Bounded queued-message advance requests

The embedded profile also exposes the official `Next Message Request` service.
At request acceptance the registry inspects the recipient-scoped TSO queue. If
the earliest currently queued timestamp is no greater than the caller's
requested logical time, that timestamp becomes the effective grant target;
otherwise the supplied request remains the target. The original request
boundary is retained separately from the effective target so federation GALT
calculation and timestamp-send validation do not silently become the earlier
message time.

The existing grant dispatcher then delivers the selected timestamp cohort,
including equal-timestamp messages, before invoking the ordinary `Time Advance
Grant` callback. The service is callback-gated in both immediate and evoked
modes. This is an intentionally bounded slice over currently queued in-process
TSO input; future transport arrivals, asynchronous delivery, `Flush Queue
Request`/`Flush Queue Grant`, and full
cross-federate coordination remain separate work. Traceability is recorded in
`compliance/requirements-lab/next-message-request-requirements-contract.json` and
`compliance/requirements-lab/next-message-request-api-contract.json`.

The same private state and grant dispatcher now implement the bounded
`Time Advance Request Available` and `Next Message Request Available` forms.
TARA uses the supplied request as its effective target; NMRA selects the
earliest currently queued recipient TSO timestamp when it is no greater than
the request. Both forms deliver the selected queued cohort before
`Time Advance Grant` and use an explicit inclusive defined-GALT decision,
unlike strict TAR/NMR. Future transport arrivals are not considered, and
these methods do not claim the optimistic-time behavior of Flush Queue Grant.
Their source/API traceability is recorded in the four
`time-advance-request-available-*` and `next-message-request-available-*`
contracts.

The bounded `Flush Queue Request` path uses the same callback-gated dispatcher
but invokes the official `Flush Queue Grant` callback. It flushes all currently
queued in-process TSO payloads, computes the actual grant as the minimum of the
supplied request, the available GALT, and the earliest delivered timestamp,
and reports an optimistic logical time that excludes a regulator-only GALT
constraint. The optimistic value remains a private lower bound for the next
advance. Future transport arrivals, in-transit network coordination, and
Request Retraction behavior outside normal timestamped Send Interaction remains
outside this slice. Its source/API
traceability is recorded in
`compliance/requirements-lab/flush-queue-request-requirements-contract.json` and
`compliance/requirements-lab/flush-queue-request-api-contract.json`.

## Bounded asynchronous receive-order delivery

The embedded profile now implements the official per-federate asynchronous
delivery switch. It is disabled by default. A time-constrained federate may
enable it to receive receive-order callbacks while not in Time Advancing; the
adapter retains those callbacks as deferred closures and releases them through
the federate's existing immediate/evoked callback route. Disable restores the
normal time-advance-only receive-order gate. Entering Time Advancing also
flushes eligible deferred callbacks before the corresponding grant.

The gate is applied at callback execution rather than only when a message is
queued, so unsubscribe, resignation, and other existing callback-entry
rechecks remain authoritative. It covers the bounded in-process receive-order
interaction, reflection, directed-interaction, and object-removal routes. TSO
messages remain time-advance gated, and the deferred closures are live-session
state rather than durable save/restore data. Source/API traceability is recorded
in `compliance/requirements-lab/asynchronous-delivery-requirements-contract.json` and
`compliance/requirements-lab/asynchronous-delivery-api-contract.json`.

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

## Bounded untimed save admission at a grant boundary

An untimed `Request Federation Save` cannot use the ordinary control-callback
queue whenever a joined member is time constrained: the 2025 source says that
member expects `Initiate Federate Save` only while Time Advancing. The embedded
registry therefore retains the requested label rather than immediately
starting a `SaveOperation`. At an ordinary, non-Flush-Queue grant dispatch it
checks a federation snapshot. Once every currently constrained joined member
has a pending advance, it creates the operation, marks the current constrained
member instructed, and returns its label to the grant dispatcher.

The dispatcher invokes that recipient's `Initiate Federate Save` callback
directly before it changes the private `FederateTimeState` to Time Granted and
before it invokes `Time Advance Grant`. Each remaining constrained member is
admitted at its own already-pending grant dispatch. Only after the final
constrained admission does the registry queue the normal initiate callbacks
for non-time-constrained members. The Catch2 scenario uses two constrained
members and one non-time-constrained regulator to prove both callback-order
properties.

This is a deliberately narrow coordinator seam, not general save conformance.
It assumes stable membership and time roles while admission is pending, does
not reconcile resignation/role changes or a restored pending grant, and does
not cover `Flush Queue Request`. Source/API traceability is recorded in
`compliance/requirements-lab/save-control-requirements-contract.json` and
`compliance/requirements-lab/save-control-api-contract.json`.

## Bounded timestamped save admission at a grant boundary

The timestamped `Request Federation Save(label, LogicalTime)` path uses the
same pre-grant principle, with an additional TSO boundary. A pending request
is retained until a time-constrained recipient has a qualifying ordinary
advance and has received every queued or in-transit TSO payload at or below the
scheduled save time. The dispatcher begins the shared grant transition, stages
TSO delivery against the prospective requested time while the private
`FederateTimeState` still remains Time Advancing, and completes each payload
callback before asking the registry to admit the save.

The registry opens the save operation only from a qualifying constrained
recipient, after every constrained member has a qualifying already-scheduled
advance boundary. `Time Advance Request` and `Next Message Request` use the
inclusive timestamp boundary; `Time Advance Request Available` and `Next
Message Request Available` use the exclusive boundary. `Flush Queue Request`
also uses an exclusive boundary, but against its calculated actual `Flush
Queue Grant`, rather than its requested time. A shared private calculator
derives that actual grant from the request, the available GALT, and current
queued TSO input; the registry uses the same calculation to prequalify another
pending FQR while an ordinary recipient is about to admit the operation. This
prevents an operation from beginning without a reachable boundary for every
constrained member.

It marks the current recipient instructed and returns the label for a direct
callback. Once the last constrained recipient has been admitted at its own
boundary, it returns ordinary queued notifications for non-time-constrained
members. The dispatcher then applies the actual grant and invokes either `Time
Advance Grant` or `Flush Queue Grant`.

The focused Catch2 cases prove a TSO callback at the exact TAR save timestamp
precedes `Initiate Federate Save`, which itself precedes the recipient's grant,
and prove a three-member TAR sequence in which the non-time-constrained
regulator waits for both constrained admissions. A cross-member TSO case proves
that a second constrained member still receives its own queued payload at the
save timestamp before its direct initiation after the first member has entered
the operation. A two-member in-transit case issues the timestamped request
while the constrained recipient's TSO callback is active and proves direct
initiation waits for that callback to return. A TARA case proves the
available-mode branch is exclusive: a grant equal to the scheduled save does
not initiate it, while a later grant does. A companion next-message case proves
the inclusive NMR branch at the scheduled timestamp and the exclusive NMRA
branch, where an equal grant remains pending until a later grant. A
three-member TARA/NMRA case proves the two strict modes can be prequalified
together and that the regulator waits for both direct callbacks. The FQR case
proves that a queued TSO payload at the save timestamp is delivered before an
equal actual FQG, which leaves the request pending; a later strictly-greater
actual FQG invokes direct `Initiate Federate Save` before the FQG callback.
Two mixed-member cases prove both FQR-first and ordinary-first admission with
FQR and TAR members, while non-time-constrained notification remains delayed
until every constrained admission. A six-member case combines TAR, NMR, TARA,
NMRA, and FQR to prove the full bounded five-mode readiness rule before that
notification. Multi-member/multi-mode in-transit TSO, membership/role churn,
pending-grant restore, durable snapshots, transport, and conformance remain outside this
seam. Source/API traceability is recorded in
`compliance/requirements-lab/timed-save-requirements-contract.json` and
`compliance/requirements-lab/timed-save-api-contract.json`.

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

The Catch2 scenarios cover the invalid lower-bound case, retraction before a
grant, exact-bound delivery, callback ordering, timestamp/order propagation,
the strict equality rejection, and a legal post-delivery Request Retraction.
The Requirements Lab traceability for this slice is
`compliance/requirements-lab/timestamped-interaction-requirements-contract.json`, with the
post-delivery behavior isolated in
`compliance/requirements-lab/request-retraction-requirements-contract.json`.

This is deliberately not a complete timestamped service family. The current
slice excludes timestamped object deletion, regional attribute updates, and
directed regional interactions, remaining alternate advance modes,
asynchronous delivery, Request Retraction for message families other than
normal non-regional interactions, region-context interactions, attribute
updates, and directed interactions, remote transport, and complete
fanout/retraction reconciliation. The implemented normal-interaction path
includes a delivered nonconstrained recipient and a queued constrained
recipient; it remains development-profile evidence rather than a conformance
claim.

## Second bounded public timestamped attribute-update slice

The same development profile now exposes the official timestamped
`RTIambassador::updateAttributeValues(..., LogicalTime const&)` overload for
non-regional object updates. The accepted update retains recipient-specific
transportation passels and value payloads beside one federation-wide TSO
message id and a federation-owned recipient-retraction record. Time-constrained
recipients consume those passels at the grant boundary; nonconstrained
recipients have no temporal-queue entry but retain the same record. The
dispatcher rechecks known-instance, ownership, and subscription projection and
claims each timestamped `reflectAttributeValues` passel immediately before its
callback; a later legal Retract can therefore suppress pending fanout or queue
one `Request Retraction` callback for a recipient whose reflection began.

The Catch2 scenario covers sender lower-bound rejection, pending retraction,
two transportation passels, exact-bound reflection before `Time Advance Grant`,
timestamp/order/retraction propagation, sender exclusion, and pre-delivery
suppression. A companion immediate-only recipient scenario proves two
timestamped reflection passels under one retraction designator followed by one
legal `Request Retraction` callback. Its Requirements Lab traceability is
`compliance/requirements-lab/timestamped-attribute-update-requirements-contract.json`.
This remains a bounded normal non-regional service: directed updates, the
separately traced regional attribute-update extension below, remaining
alternate advance modes, transport, and conformance are not implied.

## Bounded timestamped regional attribute-update extension

The same timestamped attribute-update path now consumes the committed object
attribute update-region associations produced by the regional DDM slice. The
publisher's `Update Attribute Values(..., LogicalTime)` request retains the
recipient-specific region projection in its TSO payload; a time-constrained
recipient receives the resulting `Reflect Attribute Values` callback before the
matching `Time Advance Grant`, while `Retract` can remove the pending delivery
before that grant. The callback projection rechecks the recipient's current
known-instance, ownership, subscription, and Convey Region Designator Sets
state at delivery. Consequently, optional sent-region metadata is omitted for
the default-disabled recipient switch and carries the publisher's committed
update-region realization after the recipient enables the switch.

`compliance/requirements-lab/timestamped-regional-attribute-update-requirements-contract.json`
and `compliance/requirements-lab/timestamped-regional-attribute-update-api-contract.json` pin
the selected 2025 object-management, time-management, callback, and retraction
records. The Catch2 scenario covers committed update-region association,
lower-bound validation, retraction-before-grant suppression, exact-bound
regional reflection, callback ordering and timestamp/order/retraction fields,
pending constrained-recipient suppression, and `Request Retraction` for the
delivered immediate recipient. It also proves mixed immediate/TSO fanout: a
non-time-constrained recipient receives its accepted timestamped callback
immediately while a constrained recipient remains queued until its grant.
Direct timestamped default-region object-reflection and interaction callback
coverage now has separate focused regressions. Regional request forms, a
complete timestamped default-region matrix, alternate advance modes, timed or
durable restore, transport, package/catalog evidence, and conformance remain
outside this bounded extension. A focused untimed save/restore companion now
preserves one queued explicit-source regional passel, its object/update
association, source RegionHandle set, and live retraction designator through
restore, then proves Flush Queue delivery and Request Retraction; RL-137
records the missing Requirements Lab cross-service relation. A matching
default-source interaction companion preserves one queued timestamped payload,
its private source realization, supplied-empty callback projection, and live
retraction designator through the same untimed restore boundary, then proves
Flush Queue delivery and Request Retraction; RL-138 records the corresponding
default-source cross-service relation gap. A matching non-regional attribute
companion preserves one queued typed Update Attribute Values passel, its
ordinary recipient ledger, and live retraction designator through an untimed
restore boundary before Flush Queue reflection and Request Retraction; RL-139
records the corresponding non-regional attribute cross-service relation gap. A
matching default-source attribute companion preserves one ordinary-registration
passel, its private default realization, supplied-empty callback projection,
and live retraction designator through the same untimed restore boundary before
Flush Queue reflection and Request Retraction; RL-140 records the corresponding
default-source cross-service relation gap.
The timed companion schedules save at logical time 6 while retaining a
timestamp-8 default-source passel. Both the constrained recipient and
regulating publisher cross the save boundary before completion; after a
post-save terminal Retract, restore reconstitutes the passel and its live
ledger for FQR reflection and Request Retraction. RL-141 records the missing
save-boundary relation.

## Third bounded public timestamped object-deletion slice

The development profile now exposes the official non-regional timestamped
`RTIambassador::deleteObjectInstance(..., LogicalTime const&)` overload and
matching timestamped `FederateAmbassador::removeObjectInstance` callback. At
invocation, the registry captures both the known-recipient set and an
execution-owned copy of the object state. Time-constrained recipients consume
the typed payload at their grant boundary; nonconstrained recipients use the
same accepted payload through the evoked timestamped callback path. The first
removal commits the federation-wide deletion, removes the sender's known
instance without inducing a sender callback, and advances every delivered
recipient to unknown at its own callback boundary.

A legal `Retract` distinguishes each recipient's pending and delivered state.
It withdraws pending TSO fanout and, if a removal has already crossed a
callback boundary, restores the invocation snapshot before queueing
`requestRetraction` for a delivered recipient. Restoration re-establishes the
object's name, known-instance state, and committed attribute ownership for
still-joined federates; stale departed owners and destroyed region references
are not revived. The invocation snapshot is execution-owned rather than
callback-owned and is retained at least while a legal retraction remains
possible, so callback delivery is never treated as the source of truth for
reconstitution.

Retraction has two deliberately separate lifetimes. The public result
classification must outlive heavyweight message state: after a designator
crosses the Clause 8.22.3 lower bound, or after a successful retraction, a
later `Retract` must still produce `MessageCanNoLongerBeRetracted` rather than
`InvalidMessageRetractionHandle`. The embedded registry now preserves a
lightweight, federation-owned terminal record for that classification. An
accepted producer TAR, TARA, NMR, NMRA, FQR, or immediate actual-lookahead
increase terminalizes records at or below its strict retraction boundary;
successful retraction and producer resignation also terminalize the record.

Typed normal-interaction, attribute-update, and directed-interaction payload
is retained until every still-joined pending recipient crosses its callback
boundary, then is reclaimed independently of the terminal record. A
timestamped deletion additionally retains its invocation snapshot and deleted
instance marker until the designator is terminal and no such recipient remains;
only then may the snapshot, marker, object, and reusable name be reclaimed.
Focused Catch2 cases prove both sides: a terminal normal interaction still
reaches its constrained recipient, and a terminal no-recipient deletion frees
its name while its designator remains non-retractable. Separate one-federate
normal-interaction and qualifying attribute-update cases prove that zero
eligible recipients do not turn a TSO public result into an invalid handle:
their typed payload/passels are immediately released, while the ledger still
supports a legal `Retract` and the later non-retractable classification. An
attribute update remains eligible only when at least one submitted attribute
has TSO preferred order, as required by Clause 6.10.
Both regional interaction and regional update paths now have direct
disjoint-region proofs; neither treats shared ledger code as substitute
evidence.

The tombstone itself is deliberately not yet auto-purged before federation
teardown. Disable Time Regulation also deliberately does not terminalize a
record: it removes `Retract` authority while disabled, but a later enable can
establish a new time-regulation boundary. A focused normal-interaction case
now proves that a live designator survives disable, callback-gated re-enable
at unchanged lookahead, and the second `Time Regulation Enabled` callback;
it can then be legally retracted and later receives the normal terminal
classification. Federation save/restore snapshots copy both a live TSO payload
and its recipient ledger as well as terminal state: a focused normal-
interaction regression first makes a saved live record terminal after the save,
then restores it, delivers the original callback through `Flush Queue Request`,
and legally retracts it again. The separate allocator regression prevents a
discarded post-save designator from aliasing fresh post-restore traffic.
A complementary one-federate regression saves a successful-Retract tombstone,
creates distinct post-save traffic, then restores the snapshot: the saved
handle remains `MessageCanNoLongerBeRetracted` and the discarded handle is
invalid. Another one-federate regression saves a time-regulating member at
logical time 3 with actual lookahead 2, increases both values to 5 after the
save, and restores the snapshot to observe the saved time window again. It
does not establish pending-advance, time-constrained, or transport restore
semantics. A companion regression saves actual lookahead 5 with a deferred
decrease to 1, consumes that decrease after the save, restores the visible
window, and advances again to prove the deferred target returned too. Broader
re-enable, save/restore, in-flight, and transport lifetime matrices remain
explicit future work; this is source/test traceability, not a conformance
claim.

The paired Catch2 scenarios cover lower-bound rejection, retraction before a
grant, exact-bound removal before `Time Advance Grant`, callback ordering,
timestamp/order/retraction propagation, sender exclusion, and the pre-delivery
boundary. The post-delivery scenario adds a mixed nonconstrained/time-
constrained recipient set: one recipient observes `Remove Object Instance`,
the object and split committed ownership are restored, that recipient receives
`Request Retraction`, and the constrained recipient receives neither stale
removal nor a retraction callback. Its Requirements Lab contracts are
`compliance/requirements-lab/timestamped-object-deletion-requirements-contract.json`,
`compliance/requirements-lab/timestamped-object-deletion-api-contract.json`, and the shared
Request Retraction contracts.

This remains a bounded non-regional deletion family. Alternate advance modes,
active in-flight ownership workflows,
producer/recipient resignation races beyond the tested departed-owner case, save/restore recovery evidence,
transport, package/catalog evidence, and conformance remain outside the slice.

## Fourth bounded public timestamped directed-interaction slice

The development profile now exposes the official non-regional timestamped
`RTIambassador::sendDirectedInteraction(..., LogicalTime const&)` overload and
matching timestamped `FederateAmbassador::receiveDirectedInteraction` callback.
The payload retains the known target, recipient-specific directed projection,
transportation, tag, and callback route. Active time-constrained recipients are
queued in the federation-owned TSO coordinator; non-constrained recipients use
the accepted payload through the immediate timestamped callback path.

The Catch2 scenarios cover the sender lower bound, known-target and declaration
routing, retraction before a receiver grant, exact-bound callback delivery
before `Time Advance Grant`, timestamp/order/retraction propagation, pending
constrained-recipient suppression, and `Request Retraction` for a delivered
nonconstrained directed recipient (including a no-temporal-queue-fanout case).
An additional save/restore regression saves that baseline, creates a directed
TSO message, restores the baseline, and proves the discarded post-save
designator is invalid and cannot retract a fresh post-restore message. The
federation-owned queue restores saved payload/temporal state but preserves its
external designator allocation high-water mark. A separate normal-interaction
case saves a live payload and retraction record, terminalizes it after the
save, restores it, and proves the original record is again deliverable and
legally retractable.
Its Requirements Lab contracts are
`compliance/requirements-lab/timestamped-directed-interaction-requirements-contract.json` and
`compliance/requirements-lab/timestamped-directed-interaction-api-contract.json`.

The shared directed-recipient planner honors both selector modes: absent or
false is by ownership, while true is universal. The focused timestamped
selector regression distinguishes target-owner/default, known-non-owner/default,
and known-non-owner/universal delivery, then rechecks a selector change and an
unsubscription before later grants. This slice deliberately excludes directed
DDM, remaining alternate advance modes, region-context evidence, transport,
complete save/restore semantics, package/catalog evidence, and conformance.
A dedicated lifecycle companion now queues one directed TSO payload, disables
and callback-gated re-enables the constrained recipient, and proves exactly one
directed callback before the matching grant with the original target, tag,
timestamp, order, producer, and retraction metadata. RL-134 records that the
Requirements Lab does not currently relate this directed payload to the
Time-Constrained lifecycle, so the case remains development-profile evidence
only. A separate live-record save/restore companion preserves a queued directed
payload and its retraction ledger across an untimed snapshot, delivers it via
Flush Queue Request after restore, and proves post-delivery Request Retraction
against the original designator. A timed companion now schedules save at
logical time 6 with a timestamp-8 target-qualified payload, crosses the timed
boundary for both members, restores the post-save-terminalized designator, and
proves FQR delivery plus Request Retraction. RL-135 records the untimed
directed save/restore relation and RL-141 records the timed save-boundary
relation; durable restore and broader recovery remain open.
A matching regional live-record companion saves one overlap-qualified
timestamped `Send Interaction With Regions` payload with its source RegionHandle
set and retraction ledger, terminalizes it after the snapshot, restores the
image, and proves source-region-preserving Flush Queue delivery followed by
Request Retraction from the original designator. RL-136 records the missing
regional save/restore cross-service relation; timed/durable restore, region
mutation, and broader recovery remain open.

## Fifth bounded public timestamped regional-interaction slice

The development profile now exposes the official timestamped
`RTIambassador::sendInteractionWithRegions(..., LogicalTime const&)` overload
and the timestamped `FederateAmbassador::receiveInteraction` callback. The
existing committed-region and strict overlap planner is reused at acceptance
and callback entry; the TSO payload retains the sent region set so the official
callback can report it. Time-constrained recipients are queued and receive the
callback before their matching `Time Advance Grant`, while the recipient ledger
also records delivered nonconstrained recipients for a later legal Retract.

The Catch2 scenarios cover committed overlap, lower-bound validation,
retraction-before-grant, exact-bound callback ordering, sent-region
propagation, timestamp/order/retraction fields, pending constrained-recipient
suppression, and `Request Retraction` for a delivered nonconstrained
overlap-qualified recipient, including a no-temporal-queue-fanout case. Its
Requirements Lab contracts are
`compliance/requirements-lab/timestamped-regional-interaction-requirements-contract.json` and
`compliance/requirements-lab/timestamped-regional-interaction-api-contract.json`.

This slice excludes object-region services, relaxed DDM, directed regional
interactions, remaining alternate advance modes, timed/durable restore,
transport, package/catalog evidence, and conformance. The bounded untimed
regional save/restore companion is limited to one explicit-source live record
and does not establish general persistence or region-mutation recovery.

## Remaining deliberate limits

Umbra still implements no general remaining timestamped object/attribute
service family beyond the five bounded timestamped consumers. It implements
the bounded currently-queued-message NMR/NMRA forms and the in-process Flush
Queue Request/Grant path; future transport coordination and broader
asynchronous-delivery coverage are still absent. With
no queued/in-transit messages, GALT/LITS retain the existing other-regulator
lookahead behavior. A forward TAR by a zero-lookahead regulator makes that
lower timestamp exclusive, so the selected time factory's epsilon is added.
With no other regulator and no incoming TSO message, both public query results
remain undefined. The limited TAR scheduler and the five bounded public TSO
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
3. implement the remaining time-advance variants with their distinct
   message-release and grant boundaries; and
4. make resign, disconnect, save/restore, and transport failure cancel or
   reconcile pending work without dereferencing caller-owned ambassadors.

Only after those invariants are specified should Umbra add the remaining
timestamped object/attribute services, the full Next Message Request
future-input coordination, Flush Queue future-input coordination,
Request Retraction callbacks for the remaining message families, or broader
transport behavior.

## Traceability boundary

`compliance/requirements-lab/time-advance-*.json`, `compliance/requirements-lab/time-role-*.json`,
`compliance/requirements-lab/time-bounds-*.json`,
`compliance/requirements-lab/time-grant-scheduler-requirements-contract.json` use the adjacent Requirements Lab's
source-derived requirements and exact 2025 C++ API surfaces. They are CTest
checks for traceability drift only. The Lab currently exports no higher-level
implementation mapping for these temporal surfaces, so the records must stay
outside the test catalog until an appropriate mapping, raw evidence, and
protected review exist.

The private queue foundation is traced by
`compliance/requirements-lab/tso-message-queue-requirements-contract.json`, and its temporal
coordinator integration is traced by
`compliance/requirements-lab/federation-time-coordination-tso-requirements-contract.json`.
The bounded lookahead transition is traced by
`compliance/requirements-lab/modify-lookahead-requirements-contract.json` and
`compliance/requirements-lab/modify-lookahead-api-contract.json`.
The bounded currently-queued-message advance transition is traced by
`compliance/requirements-lab/next-message-request-requirements-contract.json` and
`compliance/requirements-lab/next-message-request-api-contract.json`.
The Available-form transitions are traced by
`compliance/requirements-lab/time-advance-request-available-requirements-contract.json`,
`compliance/requirements-lab/time-advance-request-available-api-contract.json`,
`compliance/requirements-lab/next-message-request-available-requirements-contract.json`, and
`compliance/requirements-lab/next-message-request-available-api-contract.json`.
The bounded Flush Queue Request/Grant transition is traced by
`compliance/requirements-lab/flush-queue-request-requirements-contract.json` and
`compliance/requirements-lab/flush-queue-request-api-contract.json`.
The first bounded public interaction slice is traced by
`compliance/requirements-lab/timestamped-interaction-requirements-contract.json`; the second
bounded public attribute-update slice is traced by
`compliance/requirements-lab/timestamped-attribute-update-requirements-contract.json`. These
candidate records are source/test anchors only and do not promote package or
conformance evidence.
