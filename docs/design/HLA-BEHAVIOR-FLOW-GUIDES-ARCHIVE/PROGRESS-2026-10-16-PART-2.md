# Progress History Archive — 2026-10-16 (part 2)

Detailed entries are indexed in the [historical progress index](../HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md).

---

## 2025 process regional-scope event routing — 2026-10-16

The process notification path filters regional eligibility before routing.
The server polling queue filters again at Receive Interaction dequeue; the
pushed-event path sends after its initial check and enters client callback
dispatch without a later registry scope check shown in the reviewed source.
The public test couples HLA_EVOKED to polling and HLA_IMMEDIATE to pushed
events in that fixture, but covers ordinary transitions, not a stale-event
race. Treat this only as a bounded source/test distinction, not an HLA
callback-start timing rule.

Added a process transport flowchart to the 2025 Attribute Scope guide,
cross-linked it from the DDM guide, and corrected stale source anchors. The
chart passed Mermaid 12.1.0 syntax parsing, but has no render or visual review.
No build, test rerun, CI, 2010 work, or Requirements Lab edit was performed.

## 2025 FOM additional-module Join observer and failure boundaries — 2026-10-16

Reviewed the existing FOM Admission and Composition guide against the current
2025 sources and focused evidence. Clarified that catalog replacement does not
itself discover ordinary application objects, while Join's separate membership
effect can plan discovery of the new member's `HLAfederate` MOM object for
eligible observers. The embedded additional-module case observes refreshed
`HLAcurrentFDD`; the configured-process source does not enqueue the matching
federation-level conditional update, and no process observer test verifies
that path. The MOM and lifecycle guides document these as distinct observation
routes.

Extended the post-commit failure discussion to both embedded and configured
process source paths: the inspected rollback removes the new membership but
does not restore a prior definition. The embedded writer-failure and process
setup-failure tests cover ordinary Join; the process invalid-additional-FOM
case covers rejection before registry commit. No focused test asserts catalog
state after post-commit failure with added modules. The revised post-Join chart
passed Mermaid 12.1.0 syntax parsing; no render, build, test rerun, CI, or
Requirements Lab work was performed.

## 2025 Delay Subscription Evaluation reader review — 2026-10-16

The existing guide already distinguishes the HLA-level eligibility-timing
concept from sender-side service validation and callback/delivery guarantees.
Its diagrams cover federation-switch lifetime, recipient eligibility, the
embedded callback/grant boundaries, and the separate process attribute route.
Process interaction and TSO timing differences remain source observations;
the guide preserves the missing focused process cases instead of implying
profile parity.

Removed the unrelated cross-edition reference-profile section so this guide is
consistently scoped to IEEE 1516.1-2025. No diagram change was warranted. All
65 local file links and line anchors checked successfully. No build, test
rerun, CI, or Requirements Lab work was performed.

## 2025 process RECEIVE temporal-gate audit — 2026-10-16

The existing time guide already documented the embedded receive-order gate.
The bounded process-source trace extends the comparison to both attribute and
interaction callbacks: process events go through the callback bridge and
dispatcher, and the reviewed paths do not consult
`FederateTimeState::receiveOrderDeliveryAllowed` or its deferred asynchronous-
receive queue. Process constrained-member checks instead govern timestamped
queueing, which is a separate TSO boundary.

Focused tests cover the embedded receive-order gate and process TSO delivery,
but no checked-in process case combines a time-constrained receiver,
asynchronous-delivery off/on, and a pending advance. The public process result
and normative applicability remain unresolved. Expanded the existing time
guide's source-only note to include interaction callbacks; no new diagram was
needed. New source links were checked; no build, test rerun, CI, or Requirements
Lab work was performed.

Next handoff: review the 2025 Request Attribute Value Update full-set
persistence and callback-time current-owner boundary, separate from the
already reviewed requester-entitlement question. Retain the source-only limit
if no focused test covers the ownership/persistence edge. Keep the process DDM
sequence first in the visual-review queue and renderer retries paused until
launch access changes.

## 2025 Request Attribute Value Update ownership/persistence audit — 2026-10-16

The embedded class request releases its shared federation-management lock after
the final plan returns, then persists provider/object groups through separate
registry calls. Each registration recalculates the full current-owner
projection. Ownership work can therefore interleave between planning and a
registration or between groups; if a later group is stale, earlier records may
remain persisted even though the service report and callback submissions have
not happened. The runtime path shows no rollback across this loop. This is an
implementation risk, not an HLA rule or a tested outcome.

At callback entry, the registry consumes and matches the pending identity,
then computes the original provider's current-owner projection. A reduced
nonempty subset remains with that provider; an empty subset is suppressed,
not retargeted. The focused embedded restore case preserves the pending
request with the same provider and owner. A separate process instance case
cancels acquisition before transfer; neither covers completed ownership
change at the embedded class pre-enqueue or callback-entry boundary. The guide
now states the lock-release gap directly; no diagram change was warranted.
Changed-guide links were checked; no build, test rerun, CI, or Requirements
Lab work was performed.

Next handoff: review the 2025 configured-process `HLAcurrentFDD` automatic
reflection after additional-module Join, keeping it distinct from
membership-driven `HLAfederate` discovery and the embedded tested path. Keep
the process regional-attribute DDM sequence first in the visual-review queue;
renderer retries remain paused until launch access changes.

## 2025 Federation MOM `HLAcurrentFDD` process Join audit — 2026-10-16

The 2025 MIM makes the federation module-designator list Conditional when
Create/Join adds FOM modules and `HLAcurrentFDD` Conditional when those
services modify the Current FOM subset. The configured-process
service-report-store Join path prepares and commits the effective definition,
establishes the execution-scoped `HLAfederation` MOM record, then plans the
joining member's `HLAfederate` discovery. The reviewed handler does not enqueue
a federation-level module-list or Current-FDD update. Its automatic
conditional-update helper resolves a member's `HLAfederate` object and is
called from reporting and interaction-send paths, not Join.

A direct request for a known Federation MOM object projects the current values
from the live federation definition. The configured-process additional-FOM
test asserts class lookup/name after Join, not federation-object discovery or
reflection. The public `HLAcurrentFDD` observer test is embedded-only. The
existing Federation MOM guide already includes the MIM condition, process
Join/discovery trace, explicit current-value request, and embedded test limit,
so no new prose or chart was needed. Corrected stale/inaccurate-in-context
anchors for the process Join setup, member discovery, member-only update
helper, process test assertion, and the exact MIM condition text. The guide's
149 local links and 129 line anchors passed validation. No build, test rerun,
CI, or Requirements Lab work was performed.

Next handoff: audit the 2025 process object-removal callback identity after an
ownership transfer. Compare the stored registrant snapshot with the focused
test's same-federate register/delete setup; keep callback identity claims
bounded to direct process evidence. Leave the process regional-attribute DDM
sequence first in the visual-review queue; renderer retries remain paused
until launch access changes.

## 2025 process object-removal callback identity review — 2026-10-16

Receive-order process removal takes the producing-federate identity from the
registry snapshot in both push and pull delivery; timestamped payload dispatch
does the same. The internal TSO deletion message separately records the
requesting federate. The focused receive-order and timestamped process tests
register and delete through the same sender, so neither establishes callback
identity when a different federate owns delete privilege. Added the receive-
order path, snapshot source, and both test limits to the ownership guide; the
separate-owner transfer case remains embedded-test evidence only. Checked the
guide's local links and line anchors. No build, test, CI, or Requirements Lab
work was performed.

The resulting AVU handoff was already covered by the existing 2025
class-request ownership/persistence audit above: final planning releases the
shared lock before per-group registration, registration requires the complete
planned owner projection, and callback entry rechecks the original provider's
current projection. The current source/test review found no new behavior or
coverage beyond that recorded limit, so the AVU guide needed no additional
change.

## 2025 active-establisher/passive-receiver discovery review — 2026-10-16

The 2025 discovery resolver scans joined non-producer declarations across the
registered class hierarchy and discoverable instance attributes, stores a
single active-relevance boolean, and then selects the receiver using its own
matching declaration and region conditions. The current source path does not
pair the active-establisher declaration with the receiver's passive
declaration by attribute; an active declaration on one discoverable attribute
can establish the gate for a passive receiver of another attribute. This is a
source-derived consequence, not normative HLA behavior.

The focused embedded passive-subscription test has one publisher and one
subscriber; it covers passive-only suppression and that subscriber's later
active transition, not the separate active-establisher/passive-receiver
combination. The process service uses the shared registry selector, but no
focused process three-role case was found. Clarified the discovery gate and
its separate receiver-selection step in the subscription guide, with direct
source and test links. The edited Mermaid decision label has not been syntax-
rechecked; rendering remains paused until browser launch access changes. No
build, test rerun, CI, or Requirements Lab work was performed.

Next handoff: audit the 2025 process regional attribute-scope event's
post-enqueue eligibility boundary. Compare the pull receive-time recheck with
the pushed callback path and focused stale-event tests; explain only what the
evidence shows, and keep an untested race source-only. Keep the process DDM
sequence first in the visual-review queue; renderer retries remain paused
until launch access changes.

## 2025 process regional-scope post-enqueue eligibility audit — 2026-10-16

Traced the 2025 process scope event from admission through both delivery modes.
The service checks current registry eligibility before routing. A pull-queued
event is checked again when a later `receive_interaction` dequeues it. The
pushed event is transferred to client pending-event dispatch and submitted to
the callback bridge; the reviewed client/bridge chain shows no further
registry eligibility lookup. The bridge validates the event structure and
invokes the direction-specific public callback, but that is not a freshness
check.

The focused process endpoint test checks ordinary regional unsubscribe,
resubscription/In, and owner-association Out/In transitions in both callback
models. It does not mutate subscription, region, switch, or membership state
between event admission and later callback observation. The Attribute Scope
guide already distinguishes the pull dequeue checkpoint from the pushed
path and labels the stale-work race untested, so no extra diagram or prose was
warranted. This remains a bounded 2025 source/test observation, not a
normative stale-callback guarantee. No 2010 work, build, test rerun, CI, or
Requirements Lab change was made.

Next handoff: review the 2025 synchronization-point partial-achievement/
late-Join boundary. Compare barrier participant expansion and achievement
retention in source with the focused test's Join timing; keep that
interleaving source-only unless direct test evidence exists. Continue
2025-only; keep the process regional-attribute DDM sequence first in the
visual-review queue and renderer retries paused until launch access changes.

## 2025 synchronization partial-achievement/late-Join audit — 2026-10-16

The shared 2025 registry permits expansion for an omitted-set point. On a
late join it inserts the new federate into the participant and announced sets;
achievement results remain in the existing map, and completion compares
recorded-result count with the expanded participant count. The configured-
process Join handler calls that same registry expansion hook and enqueues its
announcement. This supports the partial-result-before-late-Join sequence as
source evidence only.

Coverage stops short of that sequence. The embedded public scenario adds
late joiners before the first achievement; the direct registry case checks
explicit-set versus omitted-set expansion without achievements. The process
rollback case injects failure after a late-join announcement is queued and
checks rollback/retry delivery, but no participant had reported achievement
before that Join. The synchronization guide already marked the sequence
source-derived; this pass corrected its process source anchors and stated the
process test limit beside the partial-achievement row. No new diagram or HLA
normative claim was added. No 2010 work, build, test rerun, CI, or Requirements
Lab change was made.

Next handoff: audit the 2025 Attribute Scope Advisory switch-off evidence.
Compare embedded and configured-process switch gates with focused test
sequences. Determine whether any test isolates switch-off as the sole reason
an otherwise-reportable transition is suppressed; retain a gap if a separate
rule also suppresses the callback. Continue 2025-only and keep renderer
retries paused until launch access changes.

## 2025 Attribute Scope Advisory switch-off coverage audit — 2026-10-16

The source paths contain switch gates: the embedded route checks the
recipient's switch before scheduling; the shared registry scope-eligibility
predicate also returns no attributes when disabled, and the process service
uses that predicate before routing and on pull receive. A scan and
classification of `setAttributeScopeAdvisorySwitch` calls under `cpp/tests`
found no focused test where switch-off is the sole suppressor of an otherwise
reportable transition. The embedded scope test disables the switch and then
performs the receiver's final regional unsubscribe; the independent
self-final-unsubscribe rule also suppresses Out. The process endpoint case
keeps the switch enabled. Other setter cases test MOM/service reporting,
unrelated behavior, or rejected Save/Restore interlocks.

Clarified the 2025 Attribute Scope guide with links to the embedded and shared
registry gates, the process pre-route/pull checks, and the confounded test
sequence. It distinguishes suppression while disabled from the separate
post-admission pushed-event question. No HLA normative claim was added. No
2010 work, build, test rerun, CI, or Requirements Lab change was made.

Next handoff: verify the normative basis for the 2025 Attribute Scope
Advisory Switch and final-self-unsubscribe exception against canonical IEEE
1516.1-2025 clause text. Use authoritative standard wording only; if it is
unavailable, keep the normative interpretation unverified. Preserve the
switch-only test gap as a separate implementation/test question. Continue
2025-only; renderer retries remain paused until launch access changes.

## 2025 documentation source check and timing no-change review — 2026-10-16

Checked the official IEEE 1516.1-2025 edition listing: it identifies the
active edition and offers purchase/subscription access, but does not provide
the clause prose needed to verify the Attribute Scope Advisory Switch or
final-self-unsubscribe interpretation. The downloadable archive was not
readable through the available browser path, and the local pinned dependency
contains API declarations rather than the full standard. Keep the guide's
clause 10.1.3 statement source-derived and normatively unverified.

Reviewed the 2025 time-management guide's receive-order gate against the
focused temporal-state scenario and the process callback bridge. The guide
already distinguishes the embedded rule and its tested callback-model edges
from the unresolved process outcome; no text or diagram change was needed.
No normative or profile-parity claim was added.

Next handoff: continue with 2025-only reader-gap reviews. Keep process
RECEIVE/asynchronous-delivery timing source/test-limited until focused process
evidence exists; otherwise choose the next answerable reader gap from the 2025
guide backlog. Leave rendering paused until browser launch access changes.

## 2025 exception and service-invocation reporting overlap audit — 2026-10-16

The embedded ordinary `Send Interaction` catch attempts its recognized MOM
exception report, invokes the general exception-report hook, then calls the
failed service-invocation writer. These use independent gates, and source call
order does not establish callback-arrival order. The configured-process
malformed `HLAsetSwitches` server path plans the MOM report, but its rejected
response is translated by the public wrapper before either the general
exception hook or failed-invocation writer is reached. This is an
implementation-path difference, not a normative rule or profile-parity claim.

The process service-report matrix covers successful ordinary `SendInteraction`
and failed `SendInteractionWithRegions` public preflight checks for invalid
region/parameter handles; it does not establish reporting for a server-rejected
send. Added the wrapper-specific failure diagram to the service-invocation
guide and linked the third report route from the exception guide. Mermaid
12.1.0 syntax parsing accepted 24 diagrams across the changed service,
exception, and subscription-state guides. Checked 17 local links, including 16
source line anchors, in the new reporting sections. No render or visual review
was performed because browser launch remains denied. No build, test, CI, 2010,
or Requirements Lab work was performed.

Next handoff: audit failed configured-process `Send Interaction With Regions`
beyond the tested preflight errors, tracing receive-order and timestamped
wrappers through server rejection and identifying which paths call the failed
invocation writer. Keep claims source/test-bounded and do not infer 2010/2025
parity.

## 2025 configured-process inherited interaction projection review — 2026-10-16

The shared 2025 registry planner promotes a sent interaction to the closest
matching subscribed ancestor and projects that recipient's parameter handles.
The configured-process send path stores the selected class and projected
parameters in the event before enqueue; pull delivery consumes that snapshot.
Focused process public-send and multi-recipient cases subscribe to the exact
sent class. The embedded inherited-receipt case checks nearest-class promotion
with an empty parameter map. No focused process case combines inherited-class
promotion with parameter projection. The existing interaction guide already
explains the shared-planner/source boundary and the missing process evidence,
so no additional diagram or prose was justified. No parity claim is made. No
build, test, CI, or Requirements Lab work was performed.

Next handoff: review 2025 configured-process cross-class regional interaction
routing and recipient projection against the DDM guide, shared planner, and
focused process cases. Add explanation only if evidence answers a reader gap;
otherwise preserve the exact evidence boundary. Do not infer untested runtime
outcomes or embedded/process parity. Documentation only; keep the visual-review
queue paused.

## 2025 configured-process cross-class regional interaction review — 2026-10-16

The DDM guide already maps the embedded 2025 dimension-availability,
exact-class declaration, and sent-class-to-parent candidate walk, and labels
the cross-class consequences source-only. The process regional-send handler
uses the shared registry planner; deferred candidates are resolved before event
construction, and each event carries its recipient-specific received class
and parameter projection. The focused multi-recipient process case uses the
same interaction class for sender and receivers and asserts that class in the
callbacks. It tests per-recipient region selection, not ancestor-class
promotion or parameter pruning at an ancestor. Added this source/test boundary
to the DDM guide without adding a new chart or normative inheritance claim. No
build, test, CI, or Requirements Lab work was performed.

Next handoff: review the 2025 configured-process Auto Provide and ordinary
initial-value boundaries against the Auto Provide and subscription-state
guides. Compare process discovery and event paths with the focused process and
embedded evidence; retain untested provider-solicitation or initial-value
outcomes as gaps. Add explanation only for a demonstrated reader gap.
Documentation only; keep the visual-review queue paused.

## 2025 process Auto Provide and ordinary initial-value boundary review — 2026-10-16

The current source has one `planAutoProvideForDiscovery` call site, on the
embedded discovery callback path. Configured-process discovery emits the
discovery event and constructs a separate initial-value event only for
RTI-owned MOM objects. The focused process class-request case observes
`provideAttributeValueUpdate` after an explicit class-scope Request Attribute
Value Update, including the caller's tag across two registered instances;
that does not establish discovery-triggered Auto Provide. The public process
ordinary-discovery case subscribes before registration and checks discovery,
not initial values. No focused process case covers ordinary late-subscription
values. The existing Auto Provide and subscription-state guides already keep
these boundaries distinct; added a direct process class-request evidence link
to the Auto Provide table, with no new diagram or normative inference. No build,
test, CI, or Requirements Lab work was performed.

Next handoff: review the 2025 configured-process automatic
`HLAcurrentFDD`-update path after additional-module Join against the Federation
MOM guide, process Join/FOM-module source, and focused observers. Keep it
separate from embedded reflection and the tested process `HLAfederate` initial
values; retain the process outcome as source-only unless a focused case
establishes it. Documentation only; keep the visual-review queue paused.

## 2025 process Federation MOM Current-FDD Join review — 2026-10-16

The current configured-process Join handler establishes the federation MOM
object as needed, records the joining member's service-report state, then
queues discovery for that member's `HLAfederate` object. It does not enqueue a
federation-object `HLAFOMmoduleDesignatorList` or `HLAcurrentFDD` Conditional
update in the inspected Join path. The process discovery route can discover
the Federation MOM object for a late observer and supplies its Static initial
attributes; the explicit request path projects the current federation values.
The focused configured-process additional-FOM Join case checks successful
class lookup/name after Join, not federation-object discovery or reflection.
The `HLAcurrentFDD` reflection test is embedded, and the joined-federate MOM
snapshot case concerns per-member module designators. The Federation MOM guide
already captures these distinctions and the missing process observer evidence;
no additional diagram or prose was justified. This is not a conformance or
profile-parity conclusion. No build, test, CI, or Requirements Lab work was
performed.

Next handoff: review 2025 additional-module Join failure boundaries after the
model/membership commit. Compare the FOM admission guide's transaction trace,
process Join rollback path, and focused invalid-module tests; preserve the
post-commit catalog result as source-only unless a focused test covers it.
Documentation only; keep the visual-review queue paused.

## 2025 additional-module Join failure boundary review — 2026-10-16

Rechecked the 2025 process Join path from additional-module preparation
through the registry commit and post-commit setup. Invalid or unavailable
preparation is rejected before the replacement definition reaches the
registry. The focused private-process test checks that the base module count
is unchanged, then successfully retries with the same requested federate name
and type plus a valid module; it observes the new module and class lookup.
This is direct evidence for recovery from that pre-commit rejection only.
After a successful registry commit, the process rollback paths resign the new
member but do not restore the previous definition. Existing setup-failure
tests exercise ordinary Join, not a module-bearing Join, so post-commit catalog
state remains source-only and untested. The existing admission flowchart
already distinguishes these two failure regions; I sharpened its prose and
test pointer without adding a duplicate diagram. No HLA rule or profile parity
is inferred. Documentation only; no build, test, CI, or Requirements Lab work.

Next handoff: review 2025 cross-class object-regional behavior across class
admission, retained subscription scope, and region-context matching. Compare
the DDM and object/interaction guides with the registry planner and focused
tests; label each outcome as tested or source-derived. Documentation-only and
2025-only; no conformance or profile-parity inference, no builds/tests/CI, and
keep the visual-review queue paused.

## 2025 cross-class regional object trace review — 2026-10-16

Rechecked the 2025 embedded path from available-dimension admission through
regional declaration lookup, discovery, and later attribute projection.
Available dimensions are collected along the selected class's parent chain;
each subscription or source-region request is validated against the named
class's current set. Regional declarations remain keyed by exact class and
attribute, while the embedded discovery/relevance path walks from a registered
child class toward its parents and compares committed region snapshots. A
receiver's later update projection begins from its saved known class. Added a
worked hypothetical Base/Child trace to the DDM guide to separate these stages
and show the source's shared-dimension overlap path; it is labeled source-only.
The focused child-object/base-subscription case covers the class walk without
regions, and the three-dimensional regional case covers inherited dimensions
and overlap with registration and subscription at the same class. The
inspected 2025 process regional cases also use the same class for both sides.
No focused test combines a parent-keyed regional subscription with a
child-registered object, and no process cross-class result is established.
No HLA requirement, process parity, or tested combined outcome is inferred. No
build, test, CI, or Requirements Lab work was performed.

Next handoff: review the 2025 configured-process ordinary-interaction
send-to-callback path beyond the shared recipient planner. Trace the public
send through process client/service, recipient selection, event routing, and
callback projection; compare focused tests with the existing object/interaction
guide and label outcomes as tested or source-derived. Documentation-only and
2025-only; do not infer normative behavior or profile parity, do not run
builds/tests/CI, and keep the visual-review queue paused.

## 2025 process ordinary-interaction callback routes — 2026-10-16

Traced the configured-process 2025 ordinary receive-order send from the public
adapter through the process service's shared recipient planner and event
snapshot to callback delivery. Expanded the existing object/interaction
sequence to distinguish pull delivery (queued event, public Evoke poll) from
push delivery (unsolicited frame, client buffering, callback bridge). The
focused multi-recipient fixture enables push for HLA_IMMEDIATE and uses pull
for HLA_EVOKED; it is fixture configuration, not a general rule tying
transport to callback model. Both paths assert two exact-class receiver
callbacks and FIFO tags. The immediate-mode test uses a later receiver-side
interaction-handle lookup as a fence; it does not establish callback-before-
sender-send-return timing. Ancestor-subscription promotion and post-send
declaration changes remain untested. No normative rule, profile parity, build,
test, CI, or Requirements Lab claim was added; no 2010 files were changed.

Next handoff: review 2025 configured-process ordinary-interaction
ancestor-subscription promotion and inherited parameter projection. Compare
the shared nearest-eligible-class planner with focused process tests; keep
promoted callback class and parameter-view outcomes source-derived unless a
focused process assertion proves them. Documentation-only and 2025-only; do
not infer normative behavior or profile parity, keep the visual-review queue
paused, and do not run builds, tests, or CI.

## 2025 ancestor interaction projection review — 2026-10-16

Rechecked the shared planner's class walk and parameter filter against the
composed 2025 MIM/Restaurant fixture and focused tests. The existing internal
registry example sends MainCourseServed with TemperatureOk, AccuracyOk, and
TimelinessOk: an exact subscriber receives the sent class and all three
parameters, while separate FoodServed and CustomerTransactions subscribers
receive their ancestor class with none of those three parameters. Kept that
existing table as the single worked example, linked it from the process
delivery discussion, and clarified that it is planner-level rather than
process-callback evidence. The public process callback tests remain
exact-class; the registry case does not test a nonempty inherited-parameter
subset, multiple subscription levels on one recipient, or process
transport/dispatch. No duplicate diagram or table was added, and no normative
rule, profile parity, build, test, CI, Requirements Lab, or 2010 change was
introduced.

Next handoff: review 2025 configured-process late subscription to an
already-registered object and initial-attribute reflection. Trace the public
subscription through existing-instance discovery, initial-value event
creation, and callback projection; compare focused tests with the current
HLA-first guide. Keep ordinary-object behavior distinct from MOM cases, label
source-only and tested outcomes, and do not infer normative behavior or profile
parity. Documentation-only and 2025-only; keep the visual-review queue paused
and do not run builds, tests, or CI.

## 2025 late-subscription and process MOM event-staging review — 2026-10-16

Rechecked the configured-process late-subscription path against the focused
tests and current guide. The embedded ordinary-object case subscribes after
registration and an accepted value, then observes discovery plus only the
selected stored attribute. The regional test similarly observes the latest
stored value when regional relevance first makes an existing object
discoverable. The process public discovery test subscribes before registration
and checks discovery only. The private process advisory case observes an owner
advisory after a known-object subscription change, not a current-value
reflection. The existing subscription-state guide already separates these
receiver states and marks ordinary process initial-value behavior unresolved;
no duplicate explanation was added.

The trace also sharpened the 2025 process MOM distinction. Pull mode appends
discovery and optional MOM initial-value events to separate session queues;
push mode batches discovery messages before initial-value messages. The
configured-process MOM fixture uses an `HLA_EVOKED` observer and correlates
reflection to discovery by object handle, but does not assert callback order
or cover pushed MOM initial values. Added that bounded staging explanation to
the Federation MOM guide and linked it from the subscription guide. This is
source/test evidence only; it does not establish a callback-order guarantee,
normative interpretation, or embedded/process parity. No build, test, CI,
Requirements Lab, or 2010 work was performed.

Next handoff: audit the 2025 configured-process Auto Provide path when an
existing object becomes newly discoverable. Trace the activation/relevance
gates, provider solicitation event construction, process callback projection,
and focused test coverage. Keep it distinct from explicit Request Attribute
Value Update and direct initial-value reflection; update the existing
HLA-2025 Auto Provide guide only if a reader-facing gap is found. Documentation-
only and 2025-only; keep the visual-review queue paused and do not run builds,
tests, or CI.

## 2025 configured-process Auto Provide evidence review — 2026-10-16

Traced the late-subscription discovery path against the Auto Provide guide.
The process subscription handler replans discoveries for already-registered
objects, and the common process enqueue path commits receiver-known state and
emits discovery events. For ordinary instances it plans initial relevance
advisories; the separate initial-value branch is RTI-owned MOM only. The
bounded call-site search found `planAutoProvideForDiscovery` only in the
embedded post-discovery callback path. The process HLAsetSwitches route can
change the federation-wide switch, but the focused switch test checks stored
switch state without arranging discovery or observing a provider callback.

The configured-process class-scope Request Attribute Value Update test does
observe two provider callbacks under both callback models, with the caller's
tag and one selected attribute per object. That is explicit-request evidence:
its process service uses a separate request planner/event and does not prove
discovery-triggered Auto Provide. No focused process Auto Provide solicitation
test was found. The existing guide already captured this implementation and
test boundary; added a direct source link from process subscription to
discovery planning rather than duplicating its flowchart. This does not settle
the licensed standard's process obligation or establish profile parity. No
build, test, CI, Requirements Lab, or 2010 work was performed.

Next handoff: review the 2025 configured-process Federation MOM behavior after
an additional-module Join. Trace the MIM Conditional trigger, Join lifecycle
and event-enqueue path, direct current-value request, public observer tests,
and any remaining source-only result. Keep the Join-time update separate from
post-Join discovery and explicit value requests; do not infer normative
1516.1 behavior or embedded/process parity. Documentation-only and 2025-only;
keep the visual-review queue paused and do not run builds, tests, or CI.

## 2025 process Federation MOM additional-module Join review — 2026-10-16

Revalidated the configured-process Federation MOM section against the pinned
2025 MIM, Join lifecycle, current-value request path, and focused tests. The MIM
marks the federation module-designator list Conditional when Create/Join adds
FOM modules and `HLAcurrentFDD` Conditional when Create/Join modifies the
Current FOM subset. In the inspected process Join path, the new federation
MOM object is established and discovery is queued for the joining member's
`HLAfederate` object; no federation-level list/FDD update is queued there. A
later explicit current-value request reads the live federation definition,
which is a different trigger and not evidence of the missing Conditional
Join-time reflection.

The process additional-FOM public test checks successful Join and class
lookup/name, not federation MOM discovery or reflection. The focused public
`HLAcurrentFDD` post-Join reflection test is embedded-profile only; the public
process MOM test exercises selected joined-member values, not federation-level
FOM fields. The source also confirms that the process Conditional MOM helper's
current callers update only joined-member `HLAexceptionReporting` after its
switch changes, not federation FOM attributes. Added those caller links to the
existing guide. The existing sequence and evidence table already explain the
distinction, so no duplicate diagram was added. This remains an implementation
evidence gap, not a normative or parity conclusion. No build, test, CI,
Requirements Lab, or 2010 work was performed.

Next handoff: audit 2025 configured-process Federation MOM
`HLAfederatesInFederation` updates on Join and Resign. Trace the Conditional
MIM rule, membership lifecycle/event-enqueue paths, source projections, and
public observer tests. Keep the federation membership list distinct from the
joined-member `HLAfederate` object and record any untested callback path without
inferring normative behavior or embedded/process parity. Documentation-only
and 2025-only; keep the visual-review queue paused and do not run builds, tests,
or CI.

## 2025 configured-process Federation MOM membership-list review — 2026-10-16

Traced the MIM's Conditional Join/Resign membership-list entry through the
configured-process lifecycle, shared MOM registry, current-value request, and
the focused public test sources. The execution-level `HLAfederation` object is
stored in the shared RTI-owned MOM ledger with a distinct execution-object
flag; its attribute projection builds `HLAfederatesInFederation` from the live
membership map. The process request handler uses this shared planner and checks
that the requester remains a member and knows the Federation MOM object. The
planner's default does not require an active subscription, so this establishes
source support for a direct current-value request, not an automatic Conditional
reflection.

The reviewed process Join path queues member `HLAfederate` discovery, not a
federation membership-list update. Resign removes the departed member's known
state for the retained Federation MOM object and separately removes the
departed member's `HLAfederate` object from observers that knew it. The
embedded membership test observes Federation-list reflections at Join and
Resign; the process resignation test covers only the separate member-object
removal/re-discovery sequence. No configured-process observer test asserts
either automatic `HLAfederatesInFederation` reflection or an explicit request
for that attribute. Kept the existing sequence diagram; added a source note to
explain why the shared planner covers the execution object despite its
joined-federate-oriented name. This remains an implementation evidence gap,
not a normative or cross-profile conclusion. No build, test, CI, Requirements
Lab, or 2010 work was performed.

Next handoff: review the 2025 Object and Interaction Information guide at the
newly-known-object boundary—discovery versus first attribute reflection.
Compare the applicable standard text with focused embedded/process evidence,
separate ordinary objects from MOM, and distinguish callback ordering from
implementation commit points. Add no chart if existing coverage is sufficient.
Documentation-only and 2025-only; keep the visual-review queue paused and do
not run builds, tests, or CI.

## 2025 object discovery and first-reflection evidence review — 2026-10-16

Rechecked the 2025 object-information and subscription-state guides against the
current implementation paths and focused test sources. For an embedded
ordinary object that becomes discoverable after subscription, the callback
path enters `discoverObjectInstance`, returns from user code, then plans the
currently eligible subscribed values. The focused late-subscription test
asserts one discovery and one reflection with the stored value, then a
separate later update; its report collections do not assert relative callback
order. The regional late-relevance case similarly observes the held value and
a later update as separate reports.

In the configured-process profile, the pull `receive_interaction` selector
chooses a pending discovery before its attribute-reflection result slot, while
other event families retain their own priorities. Its source comment
attributes that ordering to the standard, but the selector is a process pull
implementation boundary, not a global callback-order claim or a push-stream
result. The process subscription handler replans discovery for existing
ordinary and MOM instances; its enqueue path constructs a separate initial
value event only for RTI-owned MOM objects. The public process update case
subscribes before registration and verifies discovery plus a later explicit
update under both callback models using separate state flags. It does not
exercise late subscription, initial ordinary values, or event order. The
process MOM test covers the separate MOM branch, not ordinary-object behavior.

The existing [subscription-state profile chart](../HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge)
already distinguishes the embedded stored-value path from process MOM-only
initial events and leaves the ordinary process outcome open; the
[object-information guide](../HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#first-reflection-after-discovery-order-and-evidence-boundary)
explains the callback and test limits. No duplicate chart or prose was added.
The pinned 2025 API headers provide callback names and clause labels, not the
licensed §5.8/§6.9/§6.11 prose needed to verify the exact normative ordering.
No normative or embedded/process parity conclusion is drawn. No build, test,
CI, Requirements Lab, or 2010 work was performed.

Next handoff: review 2025 object discovery with three distinct roles—an active
relevance establisher, the registered object, and a passive subscriber whose
matching attribute differs from the active declaration. Trace the source
decision and existing diagrams/tests; decide whether a clearly source-labeled
HLA-first chart adds value or whether the current explanation is sufficient.
Keep the focused-test gap and normative uncertainty explicit. Documentation-
only and 2025-only; keep the visual-review queue paused and do not run builds,
tests, or CI.

## 2025 active-establisher/passive-receiver discovery chart refinement — 2026-10-16

Completed the three-role review from the previous handoff. The discovery
resolver reduces active relevance across joined non-producer declarations to
one gate over registered-class ancestry and discoverable instance attributes,
then checks the receiver's own matching declaration and applicable region
separately. Thus an active declaration for A can establish the discovery path
while a different member's passive declaration for B satisfies its own
receiver check, subject to class/region matching. The later value planner
recomputes active relevance for each updated attribute; passive B alone does
not make a B reflection eligible through that path.

The focused embedded test remains single-publisher/single-subscriber and does
not assert the three-role case. The process service uses the same selector,
but no focused process three-role test was found. Refined the existing
discovery/value flowchart with explicit X/Y and A/B labels and made the second
lane's active gate specific to an update of B; no duplicate diagram was added.
This is an implementation-derived reading aid, not a normative rule or a
profile-parity claim. The edited Mermaid flowchart has not been syntax
rechecked; rendering remains paused until browser launch access changes. No
build, test, CI, or Requirements Lab work was performed.

Next handoff: review the 2025 configured-process RECEIVE temporal-eligibility
boundary. Trace receive-order attribute and interaction delivery through the
process dispatcher; compare that route with the embedded receive-order
temporal gate, asynchronous-delivery state, and pending time-advance state.
Recheck focused process coverage for a constrained receiver with asynchronous
delivery disabled/enabled and a pending advance. Decide whether the existing
time-management guide clearly separates source evidence from the untested
public outcome, and update it only if a concrete reader gap remains. Keep
normative meaning and profile parity unresolved without direct evidence.
Documentation-only and 2025-only; keep the visual-review queue paused and do
not build, rerun tests, or run CI.

## 2025 class-request provider fan-out evidence clarification — 2026-10-16

The configured-process class-level Request Attribute Value Update test uses one
provider and one requester. That provider registers two object instances and
publishes one attribute; the requester asks for that attribute at class scope
after both instances have been discovered. The assertions observe two
one-attribute callbacks to the same provider, one for each object, carrying the
request tag. This is per-object callback fan-out, not a test of one object's
requested attributes being split among multiple providers. The shared planner
does group each object's requested attributes by current owner, so that
same-object multi-provider branch remains source-derived.

Added fixture and assertion links in the Request Attribute Value Update guide,
and clarified the same test's scope in the Auto Provide comparison so the
explicit-request example cannot be mistaken for Auto Provide or multi-provider
evidence. The existing class-expansion/provider-partition diagram already
communicates the source grouping; no duplicate chart was added. The earlier
2025 process RECEIVE temporal-gap review is already recorded in the time guide
and its existing progress entry; no duplicate chart or prose was needed. No
normative behavior or process/embedded parity is inferred. Documentation-only;
no build, tests, CI, or Requirements Lab work.

Next handoff: review the 2025 configured-process regional class-level Request
Attribute Value Update path for a request containing both valid and invalid
attribute/region pairs. Trace adapter normalization, registry all-pairs
preflight, typed process status, and provider-event routing; verify exactly
what the focused embedded and process tests assert. Clarify source preflight
behavior separately from the untested mixed-validity/no-callback outcome,
without inferring normative exception precedence or profile parity. Refine the
existing guide only if that distinction adds reader value; do not add a
duplicate chart. Documentation-only and 2025-only; keep the visual-review
queue paused and do not build, rerun tests, or run CI.

## 2025 regional class-request mixed-validity preflight — 2026-10-16

Traced the configured-process 2025 regional class request from public handle
normalization and membership validation through registry planning and provider
event routing. The registry validates request selectors before constructing its
recipient plan and returns at the first invalid attribute/region pair; the
process handler returns the typed rejection before its provider loop. This
source path produces no provider event for that request when preflight fails.

The focused embedded and configured-process tests exercise invalid pairs in
separate calls. The process cases assert the mapped public exceptions and
server-side typed statuses, but neither profile combines an invalid pair with
otherwise-eligible work or asserts that such work is suppressed. The no-event
consequence is therefore source-derived, not a tested guarantee, normative
atomicity rule, or exception-precedence claim. Clarified this boundary in the
existing RAVU guide; its current chart already separates rejection from
recipient expansion, so no duplicate diagram was added. Documentation-only,
2025-only; no build, tests, CI, or Requirements Lab work.

Next handoff: review whether a pending regional class-level Request Attribute
Value Update survives configured-process 2025 Save/Restore. Trace the public
process lifecycle, registry state-image encoding/restoration, provider-route
reconstruction, and callback dispatch. Compare existing process-facing tests
with the internal state-image case; clarify the evidence boundary or record a
no-change disposition. Do not infer process/embedded parity, crash recovery,
or callback timing. Update the existing Save/Restore or RAVU guide only for a
demonstrated reader gap; do not add a duplicate chart. Documentation-only and
2025-only; keep the visual-review queue paused and do not build, rerun tests,
or run CI.

## 2025 regional class-request Save/Restore: registry ledger and process callback — 2026-10-16

The configured-process Save and Restore handlers delegate to the shared
registry control path. Registry save completion stores the pending regional
request's attribute-to-region mapping and tag in its state image, and the
restore path rehydrates that typed ledger against the current federation
members, object, owners, and regions. This is source evidence that the request
metadata is represented in configured-process Save/Restore; it is not by
itself provider-callback evidence.

The embedded public fresh-registry test checks the saved image fields and
observes one provider callback after `federationRestored` under both
`HLA_EVOKED` and `HLA_IMMEDIATE`. It creates a fresh registry in the same test
process, not a configured-process endpoint or restarted RTI host. In the
configured-process path, the provider notification is a separate live session
event or pushed frame, not part of the registry image. The Restore-completion
handler enqueues lifecycle notifications and selected ownership-assumption
work, but does not explicitly enqueue a new regional RAVU callback. No focused
process test combines this pending request with Save/Restore; callback replay
and exactly-once delivery in that profile remain unverified.

Updated the RAVU and Federation Save/Restore guides with the distinction
between restored registry metadata, live process events, and callback evidence.
The existing diagrams already explain the lifecycle boundary, so no chart was
added. Documentation-only, 2025-only; no build, tests, CI, or Requirements Lab
work.

Next handoff: trace the 2025 configured-process receive loop and client
callback bridge for a provider's pre-save regional class-request event across
Restore. Determine from source how pull-session queue events, pushed frames,
and client callback queues interact with the provider's restore-completion
boundary; compare only existing process queue/Restore tests. If no focused
RAVU process case exists, retain that outcome as an evidence gap. Do not infer
callback order, exactly-once delivery, RTI-host restart recovery, or embedded
parity. Update existing RAVU or Save/Restore guidance only if a reader gap
remains; no duplicate chart. Documentation-only, 2025-only; keep visual review
paused and do not build, rerun tests, or run CI.

## 2025 configured-process RAVU event queues across Restore — 2026-10-16

Traced the provider's regional class-request event through the configured-
process receive selector and callback bridge. In pull mode, pending Restore
notifications are selected before the interaction slot, while the RAVU queue
is polled later in the fallback path after scope and relevance advisories. A
RAVU event still in the server session queue may therefore wait behind Restore
notifications; an event already returned to the client has crossed that
server-side selection boundary. Push mode uses separate event frames. The
client buffers frames in arrival order and drains its pending deque FIFO into
callback submissions; the callback dispatcher also queues tasks FIFO. Under
`HLA_EVOKED`, those tasks wait for an evoke; enabled `HLA_IMMEDIATE` dispatch
drains submitted tasks immediately. These are source-observed mechanics, not a
normative callback-order rule.

The focused pushed `HLA_EVOKED` Save/Restore process test covers discovery and
ownership-assumption callbacks, not RAVU. No focused configured-process test
combines a pending regional RAVU event with Save/Restore, so replay, callback
order relative to `federationRestored`, and exactly-once delivery remain
unverified. Updated the RAVU guide and its backlog summary; no chart added.
Documentation-only, 2025-only; no build, tests, CI, or Requirements Lab work.

Next handoff: audit the 2025 configured-process Attribute Scope Advisory
switch boundary. Trace switch-off against already queued pull events, pushed
frames, and client callback tasks; compare the focused 2025 process and
embedded tests, and state only the source-backed suppression boundary. Do not
infer cancellation of already-admitted work or normative timing. Update the
existing Attribute Scope guide only if this resolves a reader question; no
duplicate chart. Documentation-only and 2025-only; keep visual review paused
and do not build, rerun tests, or run CI.

## 2025 Attribute Scope Advisory switch-off across admitted work — 2026-10-16

Traced switch setting and scope-event delivery in the 2025 embedded and
configured-process paths. The embedded callback closure rechecks the
switch-dependent registry predicate at callback entry. The process setter
changes the registry value without clearing the per-session scope-event
queue. A pull event is removed and rechecked at a later receive poll, so it
is suppressed if still disabled; if the switch is re-enabled before that poll,
it may be reconsidered against current scope. A pushed event is checked before
send, then the client callback bridge has no later scope-switch check or
switch-triggered purge visible. These are source observations, not guaranteed
callback outcomes or normative cancellation semantics.

The embedded switch-off test segment also removes the receiver's final
regional subscription, whose own Out-advisory exception confounds a switch-only
claim. The process endpoint scope test keeps the switch enabled. No focused
test toggles the switch around an already-admitted pull event, pushed frame,
or callback task. Clarified this queue-position distinction in the existing
Attribute Scope guide; no chart added. Documentation-only, 2025-only; no build,
tests, CI, or Requirements Lab work.

Next handoff: review the 2025 configured-process ordinary-interaction
send-to-callback path beyond the shared recipient planner. Trace public Send
Interaction through the process adapter/client/service, recipient selection,
event routing, callback bridge, and focused tests. Distinguish source-derived
mechanics from tested outcomes; do not infer normative behavior or embedded
parity. Update the existing Object and Interaction Information guide only for
a demonstrated reader gap; no duplicate chart. Documentation-only, 2025-only;
keep visual review paused and do not build, rerun tests, or run CI.

## 2025 configured-process regional attribute-update RECEIVE timing — 2026-10-16

Compared the process event-planning branch, receive-side projection, focused
test setup, and the existing DDM/DSE explanations. Receive-order pull and
regional push carry the sent-region context and defer projection to the
receiving boundary; the regional path also carries the committed source-region
snapshot. The focused process fixture configures `pushReceiveOrderEvents` from
its callback-model parameter: HLA_EVOKED exercises pull polling, while
HLA_IMMEDIATE exercises pushed delivery and callback-task admission. Its D2
case covers enabled/disabled late-subscription eligibility using the original
source snapshot; D3 is suppressed after the receiver moves out of overlap.

The DDM guide already explains the send-time source snapshot versus the later
receiver projection and links to the DSE guide. Added the fixture's pull/push
mapping to the existing DSE evidence row; no new chart or duplicate DDM flow
was needed. This is bounded configured-process source/test evidence, not a
portable callback-timing rule or coverage of nonregional push. That path still
rechecks before event construction and has no focused process DSE test for late
admission or suppression. Documentation-only, 2025-only; no builds, test reruns,
CI, or Requirements Lab work.

Next handoff: review the 2025 configured-process ordinary-interaction
send-to-callback path beyond the shared recipient planner. Trace the public
send through the process adapter/client/service, recipient selection, event
routing, callback bridge, and focused tests. Distinguish source-derived
mechanics from tested outcomes; do not infer normative behavior or embedded
parity. Update the existing Object and Interaction Information guide only for
a demonstrated reader gap; no duplicate chart. Documentation-only, 2025-only;
keep visual review paused and do not build, rerun tests, or run CI.

## 2025 configured-process ordinary interaction send-to-callback review — 2026-10-16

The existing Object and Interaction Information guide already traces the
configured-process ordinary receive-order route from the public adapter through
the service planner, event materialization, pull/push routing, receive polling
or unsolicited event frames, the callback bridge, and callback dispatch. The
focused public Send Interaction case inspects an exact-class pulled event's
parameter/value and tag without asserting a callback. The multi-recipient
process case asserts exact-class `receiveInteraction` callbacks and FIFO tags
under both callback models; its IMMEDIATE-side receiver query is a fence, not
evidence that remote callbacks precede the sender's return. Ancestor promotion
is covered by an internal planner test, not by a process callback test.

The guide also marks post-send declaration changes, nonempty inherited
parameter projection, and mid-fan-out push failure as untested/source-only,
and separates timestamped delivery and predefined switch-control interactions.
The source/test trace therefore answers the bounded send-to-callback reader
question already; no duplicate chart or guide edit was warranted. This is
2025 configured-process evidence only, not a normative guarantee or embedded
parity claim. Documentation-only; no builds, test reruns, CI, or Requirements
Lab work.

Next handoff: audit the 2025 configured-process ordinary receive-order
interaction temporal gate. Trace pull/push admission against constrained-
recipient state, asynchronous-delivery settings, pending advance, and callback
dispatch; compare with the existing focused tests. The backlog currently has
no process test combining those conditions. Preserve source/test/normative
boundaries and update the existing time or interaction guide only for a
demonstrated reader gap. Documentation-only, 2025-only; no builds, rerun tests,
or CI.

## 2025 configured-process ordinary interaction RECEIVE temporal-gate audit — 2026-10-16

Confirmed the existing source/test boundary and added a more precise source
explanation to the time guide. The process receive-order interaction path
queues or pushes the materialized event, then the process callback bridge
submits it through the callback session; the inspected route does not consult
the embedded `FederateTimeState::receiveOrderDeliveryAllowed` predicate.
Process service time-constrained state participates in TSO recipient queueing,
which is a separate delivery rule and does not prove an ordinary RECEIVE gate.

The public process Join branch returns after storing remote membership and the
selected logical-time implementation without assigning the ambassador's
embedded `federateTimeState_`. The public Enable/Disable Asynchronous Delivery
adapters have no process-client branch and fall through to the local time-state
and embedded service-status path. A bounded test search found no configured-
process case invoking either service, and no case combines a constrained
process receiver, asynchronous-delivery off/on, a pending advance, and an
ordinary interaction. The public process outcome and normative applicability
remain unresolved; these are source observations only. No duplicate diagram
was needed. Documentation-only, 2025-only; no builds, tests, CI, or Requirements
Lab work.

Next handoff: audit the 2025 configured-process ordinary-object
late-subscription path for initial attribute values. Trace discovery and
reflection planning through pull/push delivery; compare process MOM coverage
with ordinary-object tests. Keep source-only observations separate from tested
outcomes, and update the object-information or MOM guide only if a demonstrated
reader gap remains. Documentation-only, 2025-only; no builds, rerun tests, or
CI.

## 2025 configured-process ordinary-object late-subscription review — 2026-10-16

Rechecked the existing-object subscription path and the guides/tests already
covering it. The subscription handler replans discovery for ordinary objects
and joined-federate MOM. The enqueue path marks an ordinary object known and
creates its discovery event, but only constructs a separate initial-value event
for RTI-owned MOM. In pull mode, discovery and any MOM value are appended to
separate queues; the receive selector takes discovery before its attribute
reflection slot. In push mode, discovery frames are sent as a batch before
initial-MOM-value frames. Those are implementation staging details, not a
callback-order guarantee. The subscription-state guide already charts the
ordinary/MOM branch and keeps the ordinary result unresolved; the MOM guide
already explains both delivery modes.

The public process discovery test subscribes before registration and asserts
discovery under both callback models, not reflection. The private later-
subscription process case checks an owner advisory, not the subscriber's
values. The joined-federate MOM case subscribes after MOM objects exist and
observes initial reflections under HLA_EVOKED, without asserting callback
order; it does not cover pushed initial values. No focused process test settles
ordinary late-subscription values. Licensed §5.8 text remains unreviewed, so
this remains a source/test/normative question, not a conformance or parity
finding. Existing diagrams were sufficient; no duplicate chart or guide edit
was warranted. Documentation-only, 2025-only; no builds, test reruns, CI, or
Requirements Lab work.

Next handoff: audit the 2025 configured-process Auto Provide path for an
already-existing ordinary object made newly discoverable by subscription.
Trace discovery through provider advisories and any solicitation callback,
check the Auto Provide switch and process route, and compare with explicit
Request Attribute Value Update coverage. Update the existing Auto Provide
guide only for a demonstrated reader gap; preserve source/test/normative
boundaries. Documentation-only, 2025-only; no builds, rerun tests, or CI.

## 2025 guide portfolio syntax sweep and sequence-label repair — 2026-10-16

Revalidated the assigned Auto Provide handoff against the current source,
focused tests, and guide. Its process-profile result is already recorded: the
subscription path emits discovery and switch handling is exercised, but the
scanned automatic discovery planner is embedded-only and no focused process
Auto Provide callback case exists. The existing guide distinguishes that gap
from explicit Request Attribute Value Update and MOM reflection; no duplicate
content or chart was warranted.

A 2025-only structural pass found 31 HLA-2025 design Markdown files, 295
Mermaid diagrams, a direct C++ source/test link in every file, and 31 distinct
2025 guide targets in the living index with no missing local files. The fresh
Mermaid 12.0.0 core parse initially found one malformed sequence in the
object-and-interaction guide: semicolons in its pull/push alternative labels
and a note were parsed as statement separators. Replaced those separators
with commas; the complete 2025-only pass then accepted all 295 diagrams with
zero syntax failures. An in-memory no-op DOMPurify adapter was used only to
reach the grammar parser; this does not validate sanitization, SVG rendering,
layout, visual clarity, or GitHub presentation.

No 2010 files, implementation code, tests, CI, or Requirements Lab content were
changed. No C++ build or test run was performed. The selected render queue
remains paused after the recorded Chromium launch failure.

Next handoff: when browser-launch access changes, render and visually inspect
the first queued 2025 process regional-attribute DDM sequence. Keep that
review separate from post-publication GitHub rendering. Until then, do not
retry the renderer; continue content work only for a fresh 2025 reader gap
supported by new source/test evidence or a reader question.

2026-10-16: Compared untimed configured-process Update Attribute Values
failures with the existing Send Interaction map. The update adapter preserves
typed local connection, membership, and handle failures; after the request
crosses the process endpoint, non-applied registry plans for producer-known
instance, attribute definition, or ownership return a generic rejection that
the public adapter exposes as `RTIinternalError`. This is the same broad
local-versus-remote conversion shape, but the two services retain different
operation-specific checks. The pinned API lists `ObjectInstanceNotKnown`,
`AttributeNotDefined`, and `AttributeNotOwned` for untimed updates; the focused
public endpoint case covers success only. No public remote-failure outcome or
conformance conclusion is claimed.

Added one failure-mapping flowchart to the 2025 process endpoint guide. Its
Mermaid 12.1.0 core parse passed with an in-memory no-op sanitizer. The nine
new source anchors and one guide-heading link were checked individually. The
chart remains unrendered; the existing visual queue stays paused. No C++ code,
tests, builds, CI, Requirements Lab content, or 2010 material was changed.

Next handoff: audit configured-process Request Attribute Value Update against
the existing 2025 request-flow guide. Compare the public API/callback contract,
local adapter, process client/service path, and focused endpoint tests. Update
only for a demonstrated reader gap; keep process Auto Provide and ordinary
late-subscription limits source-labeled and do not infer embedded/process
parity.

2026-10-16: Audited remote failure mapping for the three configured-process
2025 Request Attribute Value Update overloads. Added a chart to the existing
guide: non-applied instance and class-without-regions planner results use a
generic rejection that reaches the public caller as `RTIinternalError`, while
regional-class planner status is carried in the result payload and translated
to the corresponding API exception. Local preflight and session/protocol
failures are outside this comparison. Focused process tests establish success
for the first two overloads and typed invalid-region outcomes for the regional
form; they do not assert remote planner failures for the first two. Mermaid
12.1.0 core parsing passed, and nine source anchors plus one guide heading
resolve. The chart remains unrendered. No tests, builds, CI, Requirements Lab,
or 2010 material were changed.

Next handoff: audit the 2025 configured-process regional-class request when a
request contains multiple region pairs and one invalid pair. Compare the API,
service/registry planner, and focused tests to separate source-described
effects from observed test outcomes; update the RAVU/DDM guide only if there
is a reader gap. Do not infer normative semantics or embedded/process parity.

2026-10-16: Revalidated the existing configured-process regional Request
Attribute Value Update mixed-selector explanation. The public adapter
normalizes the supplied pairs; the registry validates each resulting
attribute-to-region mapping before provider expansion; and the service returns
a typed planner rejection before entering its provider loop. The focused
process invalid-selector test sends four invalid cases separately and checks
their public exceptions and returned planner statuses. It does not combine a
bad selector with otherwise-eligible work or assert suppression. The guide
already states this source-only consequence and the test boundary accurately,
so no guide or chart change was warranted. No tests, builds, CI, Requirements
Lab, or 2010 material were changed.

Next handoff: audit the 2025 configured-process Unconditional Divestiture path
when acquisition is already pending for the same attribute. Compare the
public service/callback contract, 2025 embedded and configured-process state
transitions, and focused tests; update the ownership guide only for a
demonstrated reader gap. Keep post-commit failure and cancellation/notification
interleavings source-labeled, without inferring normative ordering or profile
parity. Documentation-only; no Requirements Lab, builds, test reruns, or CI.

2026-10-16: Audited the 2025 Unconditional Divestiture path with a regular
acquisition already pending for the same attribute. The embedded focused case
queues both a regular and an If-Available request before divestiture, then
observes each request's separate acquisition notification, original tag, and
resulting ownership. Configured-process source removes the old owner and
replans pending regular work; its focused process tests only cover later
explicit acquisition and assumption offers, so that pending-request branch
and the push/pull notification-begin checkpoints remain untested there. The
guide already separated the process evidence gap but did not call out the
positive embedded regular-request evidence; that clarification is now added.
No new chart was needed. No tests, builds, CI, Requirements Lab, or 2010
material were changed.

2026-10-16: Re-audited the configured-process Release Denied/cancellation
boundary after owner callback entry. Cancellation acceptance reserves the
attribute but leaves it pending; denial planning marks it unavailable and
consumes a reservation already present. If denial planning happens first,
cancellation can still be accepted before event delivery. Push performs a
pre-send check and a second callback-bridge claim; pull checks at receive poll
and checks Unavailable before acquisition/cancellation events. An embedded
2025 test covers one arranged after-entry ordering and observes Unavailable,
no cancellation confirmation, and retained old ownership. The process tests
cover ordinary denial and pre-entry buffered suppression, not the after-entry
race. The guide now reflects the source gates and profile boundary; its chart
passed Mermaid 12.0.0 core syntax parsing with a no-op sanitizer. Rendering
remains paused. No tests, builds, CI, Requirements Lab, or 2010 material were
changed.

Next handoff: audit the 2025 configured-process cancellation path when
Unconditional Divestiture has generated acquisition-notification work for the
same attribute. Trace registry state and event gates against focused tests;
compare embedded evidence only where directly relevant. Update the ownership
guide only for a demonstrated reader gap, and infer no normative priority or
profile parity. Documentation-only; no builds, test reruns, CI, or
Requirements Lab work. Preserve the process regional-attribute DDM sequence
as first in the paused visual-review queue.

2026-10-16: Audited the 2025 configured-process cancellation/Unconditional
Divestiture interleaving. The registry serializes cancellation planning and
notification begin: cancellation-first reserves the attribute while leaving
the request pending, so a later notification gate suppresses transfer;
notification-first assigns ownership before callback projection, so a later
cancellation is rejected. After accepted cancellation, push begins cancellation
in the service handler before sending its confirmation frame; pull queues the
confirmation and begins cancellation at receive polling. Existing process
cancellation and divestiture tests cover the paths separately, and the
embedded pending-request divestiture test does not add cancellation. A separate
embedded assumption-search test does cancel before divestiture, but checks later
assumption-callback eligibility rather than the configured-process
queued-notification race. Updated the ownership guide and flowchart to expose
these gates. The process race is source-derived; no normative ordering or
profile-parity claim is made. No builds, test reruns, CI, or Requirements Lab
work. Mermaid core syntax parsing passed with a no-op sanitizer; the chart
remains unrendered.

Next handoff: audit the 2025 configured-process Unconditional Divestiture
post-commit failure boundary. Trace registry commit, acquisition-work enqueue,
push sends, pull queue insertion, and the service result; distinguish source-
observed committed state from focused-test evidence. Update the ownership guide
only for a demonstrated reader gap; infer no normative rollback or callback-
order rule. Documentation-only; no builds, test reruns, CI, or Requirements
Lab work. Keep the renderer paused and the process regional-attribute DDM
sequence first in the visual-review queue.

2026-10-16: Audited the configured-process Unconditional Divestiture post-
commit failure boundary. The registry plan commits before recipient projection.
Push acquisition work runs registry begin gates and prepares frames before a
separate sequential send loop; pull appends events to recipient queues and
defers begin gates until receive polling. Assumption-recipient work runs only
after acquisition-work projection succeeds: push may send offers per recipient
or defer them when callbacks are disabled, while pull queues them. A later
helper failure can therefore follow committed state and earlier work. The
public adapter reports `RTIinternalError`; source shows no rollback, but no
focused failure-injection case establishes runtime residual effects. Added a
failure-phase flowchart to make those checkpoints and evidence limits visible.
The ownership overview and failure-phase charts passed individual Mermaid
core syntax parsing; neither was rendered. A bounded scan validated 29 local
source/test anchors in the divestiture subsection. No build, test rerun, CI,
Requirements Lab, or 2010 material was involved.

Next handoff: audit the 2025 configured-process regional class Request
Attribute Value Update path when a request contains multiple region/attribute
pairs and one pair is invalid. Compare embedded and process behavior only where
the existing focused evidence is directly relevant; separate source-described
whole-request validation from observed outcomes. Update the RAVU/DDM guide only
for a demonstrated reader gap, and infer no normative precedence or
cross-profile parity. Documentation-only; no builds, test reruns, CI, or
Requirements Lab work. Keep the renderer paused and the process regional-
attribute DDM sequence first in the visual-review queue.

2026-10-16: Re-audited mixed-validity regional class Request Attribute Value
Update in the configured-process profile. The adapter normalizes the supplied
pairs, and the registry validates every pair before constructing provider
recipients. A typed rejection returns before the service's provider loop, so
the inspected path has no provider event to queue or push after failed
preflight. Focused process tests exercise invalid selector cases separately;
the matching success case covers one eligible pair under both callback models.
No test combines an invalid pair with otherwise-eligible work or asserts
suppression. The existing RAVU guide already explains this source/test boundary,
so no guide edit or chart was warranted. No normative atomicity,
exception-precedence, or embedded/process parity claim is made. No builds,
test reruns, CI, or Requirements Lab work.

Next handoff: audit configured-process 2025 regional class Request Attribute
Value Update grouping when one object has requested attributes owned by
multiple providers, including a provider whose source region is disjoint from
the request regions. Compare registry grouping and regional filtering with
focused process evidence; update the RAVU/DDM guide only if a demonstrated
reader gap remains. Keep source-derived behavior separate from tested
outcomes, and infer no normative one-callback-per-provider rule or
embedded/process parity. Documentation-only; no builds, test reruns, CI, or
Requirements Lab work. Preserve the process regional-attribute DDM sequence
as first in the paused visual-review queue.

2026-10-16: Audited configured-process regional class RAVU grouping. The 2025
registry plans eligible groups per object and provider, filters each attribute
against its request/source-region overlap, and excludes requester-owned
attributes; an entirely disjoint remote-provider group is omitted. The
regional process success test covers one object/provider/attribute under both
callback models. A separate nonregional class test covers two objects for one
provider, not one object's attributes split across providers. No focused
regional process test covers multi-provider grouping or the disjoint-provider
case. The existing planner chart already explains those source rules and
labels the test gap.

The service loop exposed a separate partial-progress seam: Pull enqueues each
eligible group while iterating; Push stages frames and sends only after group
preparation completes, then sends sequentially. A later registration failure
can follow an earlier Pull queue prefix, while a later Push send failure can
follow a sent frame prefix. The handler has no visible compensation for earlier
group work. Added a source-only failure-phase chart; it does not claim callback
delivery, rollback semantics, or normative ordering. Mermaid 12.1.0 core
syntax parsing passed; the chart remains unrendered. Nine local source/test
anchors were checked. No builds, test reruns, CI, or Requirements Lab work.

Next handoff: audit 2025 configured-process class-level Request Attribute
Value Update against object awareness. Source expands live class instances
without requester-known-object filtering, while the public process class test
discovers its targets first. Check focused evidence and the applicable API text;
update the RAVU guide only if this leaves a concrete reader gap. Do not infer
normative applicability from source/tests or embedded/process parity.
Documentation-only; no builds, test reruns, CI, or Requirements Lab work.
Preserve the process regional-attribute DDM sequence first in the paused
visual-review queue.

---

### 2025 class-level RAVU and discovery-triggered solicitation review — 2026-10-16

Rechecked the configured-process class-level Request Attribute Value Update
object-awareness boundary. The instance planner requires requester-known object
state; the class planner expands live registered instances without consulting
that per-instance map. The public process class test requests two instances
only after the requester has observed discovery for both. The pinned §6.21 API
declaration lists `ObjectInstanceNotKnown` for the object overload but not the
class overload; the declaration's exception list is not the full clause prose,
so no normative applicability conclusion follows.

The RAVU guide already separates those source paths, records that no focused
class test targets an undiscovered object, and avoids claiming class-request
entitlement as an HLA rule. A bounded adjacent check found the Auto Provide and
subscription-state guides already distinguish discovery-triggered solicitation,
explicit requests, and initial reflection. They state that the scanned
`planAutoProvideForDiscovery` call is embedded-only, the process switch test
does not observe a provider callback, and ordinary process late-subscription
initial values remain unresolved. No additional chart or prose was needed.

This was a 2025-only documentation disposition. I made no 2010 changes; the
pre-existing dirty 2010 reference guide was left untouched. The Requirements
Lab, builds, test reruns, and CI were not touched. Mermaid or link counts did
not change.

Next handoff: review 2025 configured-process Federation MOM `HLAcurrentFDD`
updates after Join. Distinguish automatic federation-object value updates from
`HLAfederate` discovery, explicit current-value requests, and embedded
reflection; compare source and focused process evidence with the MOM guide and
change it only for a concrete reader gap. Infer no normative callback timing
or embedded/process parity. Documentation-only; no builds, test reruns, CI, or
Requirements Lab work.

---

### 2025 Federation MOM Current FDD Join boundary review — 2026-10-16

Compared the configured-process Join path, current-value request path,
federation MOM guide, and focused process/embedded tests. The MIM's federation
`HLAcurrentFDD` condition is documented separately from the member-scoped
`HLAfederate` object. The inspected process Join path establishes the
execution-scoped MOM record and queues discovery for the joining member's
`HLAfederate` object, but has no federation-level Conditional-update enqueue.
An explicit current-value request is membership- and known-object-checked and
projects the live Current FDD from the federation definition.

The configured-process additional-FOM test checks the joining client's class
lookup/name, not federation-object discovery or reflection. The focused
`HLAcurrentFDD` reflection test is embedded-profile evidence only. The existing
Federation MOM guide already diagrams these paths, labels the process gap as
source/test evidence rather than a conformance finding, and distinguishes it
from member discovery and direct current-value requests. No new chart or prose
was warranted.

This was a 2025-only documentation disposition. No 2010 changes, builds, test
reruns, CI, or Requirements Lab edits were made; chart and link counts are
unchanged.

Next handoff: review the 2025 variant-record model-to-runtime boundary.
Compare process Create/Join evidence in the FOM composition guide, Annex C
merge/resolution tests, and the DataElement codec guide. Ensure successful
process Create with the Restaurant FOM is not presented as proof of
DrinkGarnish alternative merging, discriminant/range validation, or runtime
codec selection. Keep model and codec claims separate; infer no profile parity
or normative behavior. Update only for a demonstrated reader gap.
Documentation-only, 2025-only; no builds, test reruns, CI, or Requirements Lab
work.

---

### 2025 variant-record model/runtime evidence boundary — 2026-10-16

Compared the variant-record composition and codec guides with the focused
Annex C, composer, process Create/Join, and DataElement test sources. The FOM
guide has a composition flow for merging extendable alternatives and resolving
the completed discriminant table, plus composer-level cases for cross-module
merge, omitted/malformed expressions, unknown enumerators and range endpoints,
declaration-order expansion, and overlap rejection.

The configured-process Restaurant-FOM test expects Create to succeed, but does
not inspect `DrinkGarnish` or assert variant-specific behavior; the additional
module supplied on its Join path is object-only. The Annex C test asserts the
merged model fields directly. Separately, the runtime codec test constructs
its own `HLAextendableVariantRecord` map using `addVariant`; it does not load a
composed FOM catalog. Both guides already state these boundaries and disclaim
an end-to-end catalog-to-codec path. No additional chart or process claim was
warranted.

This was a 2025-only documentation disposition. No 2010 changes, builds, test
reruns, CI, or Requirements Lab edits were made; diagram and link counts are
unchanged.

Next handoff: audit the 2025 additional-module Join failure boundary after
FOM composition commits. Trace commit/rollback and subsequent configured-
process MOM or service-report setup failures; compare source with focused tests
and the FOM admission guide. Determine whether member rollback is clearly
distinguished from restoration of the prior catalog. Add a source-only failure
flow only for a concrete reader gap; infer no normative outcome or
embedded/process parity. Documentation-only, 2025-only; no builds, test
reruns, CI, or Requirements Lab work.

---

### 2025 additional-module Join post-commit rollback review — 2026-10-16

Traced configured-process additional-module Join through preparation,
`joinWithDefinitionAndTimeState`, post-commit service-report/MOM setup, session
installation, and later discovery/announcement enqueue. The registry prepares
the replacement directories first, then swaps the replacement definition and
handle directories together with membership. Later setup rollback resigns the
new member; after session installation it also clears session/event state.
Neither inspected rollback path restores the prior definition or handle
directories. This is a source observation about the inspected failure path, not
a tested catalog result or an HLA rollback rule.

The focused process rollback test injects report-allocation/session-setup
failures on ordinary Join. The additional-FOM test verifies pre-commit invalid
module rejection leaves the module count unchanged, and a valid Join makes the
new class visible; it does not inject a post-commit failure after adding a
module or inspect the resulting catalog. The existing FOM admission guide
already has a flow and states this evidence gap explicitly, so no new chart or
prose was warranted.

This was a 2025-only documentation disposition. No 2010 changes, builds, test
reruns, CI, or Requirements Lab edits were made; diagram and link counts are
unchanged.

Next handoff: review the 2025 configured-process Release Denied/cancellation
boundary after owner-callback entry. Trace denial planning and existing
reservation consumption against cancellation acceptance and Pull/Push event
gates; compare embedded/process focused tests and the ownership guide. Keep
source-derived ordering distinct from normative priority; if the current flow
answers the reader questions, record a no-change disposition. Infer no profile
parity. Documentation-only, 2025-only; no builds, test reruns, CI, or
Requirements Lab work.

---

### 2025 process Release Denied/cancellation after owner callback entry — 2026-10-16

Reviewed the configured-process boundary against the embedded 2025 race case
and the existing ownership guide. Registry source shows cancellation acceptance
reserves the attributes while leaving the acquisition pending; denial planning
marks them unavailable and consumes any matching cancellation reservation.
The later cancellation-confirmation and unavailable gates revalidate that
state, so an already-queued competing event can become stale. Push checks
release eligibility before send and claims again at callback entry; Pull claims
at receive poll and checks its Unavailable queue before acquisition/cancellation
events. That queue order is a source observation, not an HLA priority rule.

The embedded race test arranges callback entry, then cancellation, then denial;
it observes Unavailable with the denial tag, no cancellation-confirmation
callback, and the original owner retaining the attribute. The focused process
denial and cancellation tests do not combine Release Denied with cancellation
after callback entry. The ownership guide already contains this exact race flow
and clearly separates the arranged embedded result from the untested process
outcome; no new diagram or prose was warranted. No normative priority,
callback-timing rule, or embedded/process parity is inferred.

This was a 2025-only documentation disposition. No 2010 changes, builds, test
reruns, CI, or Requirements Lab edits were made; diagram and link counts are
unchanged.

Next handoff: review the 2025 configured-process object-removal producer
identity after transfer of `HLAprivilegeToDeleteObject`. Compare the registered
producer in the removal snapshot with the deleting requester recorded in the
TSO message; inspect focused embedded/process receive-order and timestamped
deletion tests and the lifecycle/ownership flows. Keep the transferred-owner
callback outcome source-only where tests use one federate for registration and
deletion; infer no profile parity or normative result. Add material only for a
demonstrated reader gap. Documentation-only, 2025-only; no builds, test
reruns, CI, or Requirements Lab work.

---

### 2025 process object-removal producer identity after privilege transfer — 2026-10-16

Traced the configured-process timestamped deletion identity across the TSO
message, receiver-side registry snapshot, process removal event, and callback
bridge. The message records the deleting requester; the registry snapshot
returns the object's original registration producer; the public callback path
uses the snapshot identity. The embedded transfer-and-delete case has distinct
registrant and delete-privilege owner and observes the original registrant in
the removal callback. The focused process receive-order and timestamped cases
register and delete through the same federate, so they do not exercise that
identity distinction.

The ownership guide already contained the HLA-facing A/B/C transfer sequence
and a source/test mapping table, but not a process-specific view of how the
requester's internal TSO identity differs from the public removal producer.
Added a short configured-process sequence and labeled the A≠B outcome
source-only. Mermaid 12.0.0 core parsed the new chart; it was not rendered or
visually reviewed, and rendering remains paused after the recorded Chromium
launch failure. No source links were added with the chart. A live scan of the
31 2025-prefixed guides finds 298 diagrams (169 flowcharts, 108 sequences, 21
state diagrams); the previous 304 count and its mixed-version parser records
do not reconcile to the current file set, so no current whole-set syntax pass
is claimed.

No normative callback-order or configured-process parity claim is inferred.
No 2010 changes, builds, test reruns, CI, or Requirements Lab edits were made.

Next handoff: review the 2025 configured-process ordinary-object
late-subscription initial-reflection boundary. Trace subscription to an
already-registered object through discovery planning and initial-value event
creation; compare process source, the subscribe-before-register public test,
the private late-subscription owner-advisory test, and the embedded
initial-reflection planner. Keep the RTI-owned MOM current-value path distinct;
identify what remains source-only and whether an existing diagram answers the
question. Infer no profile parity or normative behavior. Documentation-only,
2025-only; no builds, test reruns, CI, or Requirements Lab work.

---

### 2025 ordinary-object late-subscription initial-reflection review — 2026-10-16

Rechecked the 2025 subscription-state guide against the current discovery,
subscription, callback, and process enqueue paths. Embedded discovery replans
skip instances already known or pending for that receiver; when a new
subscription makes an existing object newly discoverable, the embedded callback
path invokes `Discover Object Instance` before planning and reflecting selected
currently held values. The focused embedded case observes the discovery and
stored-value reflection, but records callback kinds separately rather than
asserting their relative order.

Configured-process Subscribe also replans existing-instance discovery. The
ordinary initial-reflection planner is not called on that process route; the
separate process initial-value event is gated to RTI-owned MOM objects. The
public process discovery case subscribes before registration and asserts
discovery only. The private late-subscription case checks an owner-facing
relevance advisory, not subscriber values. Thus process ordinary late
reflection and the normative §5.8 result remain unresolved.

The subscription-state guide already has a receiver-known/not-known table, an
embedded/process comparison flow, a separate embedded regional stored-value
trace, and exact test/source boundaries. The MOM guide keeps its direct current
values and initial-value events separate. No additional chart or prose was
warranted; no normative callback-order or process-parity claim is inferred.
No builds, test reruns, CI, or Requirements Lab work were performed.

Next handoff: review the 2025 Attribute Scope Advisory Switch boundary between
suppressing new plans and effects already queued or pushed. Compare
embedded/process switch checks, callback/transport admission, and focused
switch/scope tests; verify the guide keeps the switch-off-only test gap and
already-pushed-event gap distinct. Keep the §10.1.3 interpretation unverified
without canonical prose; infer no normative rule or profile parity.
Documentation-only, 2025-only; no builds, test reruns, CI, or Requirements Lab
work.

### 2025-only diagram census and renderer status — 2026-10-16

Recounted the 31 `HLA-2025-*` topic guides: 305 Mermaid fences, comprising 172
flowcharts, 110 sequences, and 23 state diagrams. A current Mermaid 12.0.0
core-parser pass accepted all 305 with zero syntax failures. The pass used an
in-memory no-op DOMPurify hook and identity sanitizer solely to reach grammar
parsing; it is not sanitization or rendering evidence.

Retried the first queued process regional-attribute DDM sequence with the
installed Chromium executable. The browser failed to launch with code
3221225506, so no image or visual review resulted. The previous launch failure
(2147483651) remains relevant; keep the visual queue pending and do not claim
layout or GitHub-rendering validation.

Updated the current 2025 diagram census and handoff in the living index and
roadmap. This work stayed in 2025 documentation; no 2010 files, implementation
code, tests, CI, or Requirements Lab content were changed.

Next handoff: review how a 2025 regional/class RAVU Provide callback can lead
to a timestamped provider update. Compare the existing RAVU response section
with the time-management guide's TSO ordering, grant, callback, and
response-tag paths; add prose or a chart only for a demonstrated reader gap.
Keep the process regional-attribute DDM visual review queued until Chromium
launch works.

### 2025 configured-process regional class RAVU path review — 2026-10-16

Compared the existing reader-facing guide with the configured-process source
and focused public test. The guide already separates requester-owned region
validation, per-attribute request-region/provider-update-region overlap,
provider/object grouping, and process pull-queue versus staged push routing.
The source admits a default-region provider attribute without an explicit
update-region association; with explicit associations, any committed overlap
keeps the attribute once in the group. The process handler then persists only
the group's matching request-region pairs, recomputes its current projection,
and routes the projected attribute subset.

The focused process scenario uses one object, provider, attribute, and matching
`[0,2]` request/update regions. It observes one Provide callback under both
callback models, then installs the requester's subscription and observes the
provider's later update by ordinary receiver DDM. It does not cover disjoint or
multiple overlapping region pairs, multiple providers, or a region mutation
between projection and callback. These boundaries are already documented in
the RAVU guide; no duplicate chart or additional prose was warranted. No
normative behavior or profile parity is inferred.

This was a 2025 documentation/source survey only. No C++ files, tests, builds,
CI, or Requirements Lab content were changed.

Next handoff: review how a 2025 regional/class RAVU Provide callback can lead
to a timestamped provider update. Compare the existing RAVU response section
with the time-management guide's TSO ordering, grant, callback, and
response-tag paths; add prose or a chart only for a demonstrated reader gap.
Keep the process regional-attribute DDM visual review queued until Chromium
launch works.

### 2025 RAVU-to-TSO response timing bridge — 2026-10-16

Added a focused embedded 2025 sequence to the RAVU guide connecting the
solicitation callback to the provider's optional timestamped Update. It makes
the two tags explicit: the request tag reaches `provideAttributeValueUpdate`,
while a distinct response tag travels with the later timestamped reflection.
For the subscribed, time-constrained requester in the focused class-response
test, reflection at time 2 precedes the matching grant under both
`HLA_EVOKED` and `HLA_IMMEDIATE`. The regional response test observes the same
reflection-before-grant order under `HLA_EVOKED` and checks sent-region
metadata. Neither test establishes configured-process parity or a normative
interpretation; the regional fixture does not compare different request and
source bounds. The guide links this scenario to the general TSO queue and
attribute-delivery sections instead of duplicating their state machine.

A fresh source-fence count over the current 31 `HLA-2025-*` guides finds 299
diagrams (169 flowcharts, 109 sequences, 21 state diagrams). A Mermaid 12.1.0
core parse with an in-memory no-op DOMPurify hook/sanitizer adapter accepted
all 299 with zero grammar failures. This is syntax-only evidence. The previous
current-status snapshot said 305; that count is not reproducible from the
current worktree, and this audit does not infer that six diagrams were removed.
Updated current inventory fields while preserving older log entries as dated
history. The new sequence remains unrendered; the existing Chromium launch
failure still leaves visual review pending.

No C++ implementation, tests, builds, CI, or Requirements Lab content changed.

Next handoff: review the 2025 configured-process name-reservation to
named-registration to peer-discovery path against the existing reservation
guide and focused process test. Verify what the success callback, reservation
consumption, registration commit, and peer discovery establish under
`HLA_EVOKED` and `HLA_IMMEDIATE`; update the existing guide only if a real
reader gap appears. Keep the work 2025 documentation-only; do not run builds,
tests, CI, or Requirements Lab tasks.

2026-10-16 follow-up: reviewed that configured-process path against the
2025 handlers, registry commit, callback adapter, and focused connection tests.
The named-registration test is a one-client scenario covering reservation and
named registration under both callback models. A separate discovery test uses
unnamed registration and a peer subscriber. Source connects an applied named
registration to discovery planning/enqueueing, but there is no focused test
covering reservation, named commit, and peer discovery end to end. The source
enqueues discovery work before constructing the registration response; neither
focused test asserts cross-session observation order, so no portable ordering
claim is made. Added this evidence boundary to the existing reservation guide;
no chart was warranted. No C++ implementation, tests, builds, CI, or
Requirements Lab content changed.

2026-10-16 follow-up: reviewed the 2025 configured-process discovery-to-first-
reflection transition against the existing object-information and subscription
guides and the source/test paths they cite. The process discovery enqueuer makes
an initial-value event only for RTI-owned MOM objects; the ordinary process
reflection case follows a later explicit Update. The MOM process case observes
MOM discovery and values, while the embedded late-subscription case covers its
ordinary current-value path. The guides already distinguish these profiles,
their callback evidence, and the unresolved normative/process behavior. No
additional diagram or guide change was warranted. A Mermaid CLI 12.0.0 visual
render attempt still could not launch Chromium (3221225506), so visual review
remains pending. No implementation, tests, builds, CI, or Requirements Lab
content changed.

Next handoff: review the existing 2025 configured-process Unconditional
Divestiture flow in the ownership guide. Trace an already-pending regular
acquisition versus ownership-assumption offers across registry commit, process
enqueue/begin gates, push/pull delivery, and callback dispatch. Compare focused
tests and preserve source-only boundaries where evidence is absent; update only
for a demonstrated reader gap. Keep the work documentation-only and 2025-only;
do not run builds, tests, CI, or Requirements Lab tasks.

2026-10-16 follow-up: audited the configured-process Unconditional Divestiture
flow against the 2025 registry plan, service handler, push/pull delivery helpers,
and focused tests. The registry preselects other known-instance publishers
before the unowned transition and excludes a member with a pending regular or
If-Available acquisition or cancellation for that attribute. It then commits
owner removal and replans pending acquisition work; the service projects that
work before separate assumption offers. The existing process chart showed the
latter delivery sequence but not this preselection boundary, so it now makes
that order visible and states the candidate exclusion and evidence limits.
The embedded focused case observes regular and If-Available requesters receive
their own acquisition notifications and explicitly checks no assumption for
the If-Available requester; no configured-process test covers a request
already pending at divestiture. All 24 Mermaid charts in the ownership guide
passed Mermaid-core 12.0.0 grammar parsing; browser rendering remains pending
because Chromium launch fails in this environment. No implementation, tests,
builds, CI, or Requirements Lab content changed.

Next handoff: review the 2025 FOM variant-record composition claims in the
module-admission guide. Trace table-row merge and reference resolution for
`DrinkGarnish` / `HLAextendableVariantRecord` across Create and additional-module
Join; distinguish composer tests, process test expectations, and actual
composed-model observation. Update only for a demonstrated reader gap; do not
infer process/embedded parity or validation rules. Keep the work
documentation-only and 2025-only; do not run builds, tests, CI, or Requirements
Lab tasks.

2026-10-16 follow-up: compared the variant-record evidence across the 2025
composer, embedded API, configured-process Create, and configured-process Join
paths. Composer tests establish a valid catalog containing `DrinkGarnish` and
separately exercise variant merge/resolution rules. The embedded public Create
case accepts a disjoint added alternative, rejects an overlapping one with
`InconsistentFOM`, and succeeds on a valid retry. The process Create test only
expects the Restaurant input to succeed; its additional-module Join accepts an
object-only extension and resolves the added class handle/name. Neither process
case inspects a variant-record composition. Added an evidence matrix to the FOM
guide to keep these observations separate and avoid inferring process/embedded
parity. No implementation, tests, builds, CI, or Requirements Lab content
changed.

2026-10-16 follow-up: checked the 2025 additional-module Join commit and
post-commit failure evidence against the guide. The registry swaps the prepared
replacement definition and handle directories with new membership only after
preparation and membership checks. Embedded and configured-process setup and
rollback run afterward. Existing setup-failure tests cover ordinary Join, not
additional-module Join, and do not inspect the replacement catalog after a
post-commit failure. The private process invalid-addition test does directly
assert that the FOM module count is unchanged after rejection, then increases
by one and exposes the new class after a valid retry; the embedded preparation
test only proves a subsequent ordinary Join succeeds. Added that distinction
to the FOM guide without claiming full catalog equality, rollback semantics, or
cross-profile parity. No implementation, tests, builds, CI, or Requirements
Lab content changed.

2026-10-16 review: audited the 2025 regional class-level RAVU validation and
fan-out boundaries against the existing guide, process service/registry path,
and focused tests. The guide already explains whole-request region/attribute
preflight before provider expansion, then per-group registration and projection
recheck. It distinguishes Pull's incremental event queueing from Push's staged
frames and sequential send, and labels the partial-progress chart source-only.
The public tests cover one eligible object/provider/attribute under both
callback models and separate invalid-selector calls; they do not establish
mixed-validity suppression, multiple or disjoint provider groups, mutation
after the projection checkpoint, or later-group registration/send failure
effects. Since those evidence limits and the no-atomicity caveat are already
clear, no guide change was warranted. No builds, tests, CI, or Requirements Lab
content changed.

2026-10-16 review: audited the 2025 configured-process Unconditional Divestiture
path against registry/service source and focused embedded/process tests. The
guide already bounds pending regular and If-Available requests, the
cancellation-versus-notification begin gates, Push/Pull queue/send checkpoints,
and the lack of focused process tests for the combined race or injected helper
failures. Added a separate embedded-only assumption-search epoch flowchart:
after a candidate acquires and a new owner divests with a new tag, a previously
offered candidate can receive a fresh offer after republishing. The test
asserts the second offer carries the new tag, but does not assert the first
offer's tag or configured-process parity. Mermaid core parsing accepted the
new chart; the 2025 guide census is now 300 diagrams across 31 guides (170
flowcharts, 109 sequences, 21 state diagrams). Visual rendering remains
pending because Chromium failed to launch (2147483651). No builds, tests, CI,
or Requirements Lab content changed.

2026-10-16 review: audited the 2025 configured-process Release Denied and
acquisition-cancellation gates against current registry, service, and receive
paths, plus the separate process and embedded test cases. The existing guide
already distinguished plan acceptance, callback-entry gates, Pull queue order,
and the untested combined process race. I corrected its flowchart to show a
separate source-derived order: cancellation confirmation may consume the
pending request before the owner's Release Denied plan runs; the later denial
plan then reports zero recipients and queues no Unavailable event. No focused
test covers this order. The embedded arranged race covers cancellation plan
acceptance followed by denial before cancellation confirmation begins, while
process tests cover ordinary denial and pre-entry pushed-release suppression.
The revised Mermaid chart parsed successfully; visual rendering remains
pending because Chromium failed to launch (2147483651). No builds, tests, CI,
or Requirements Lab content changed.

2026-10-16 review: audited the 2025 configured-process Auto Provide path after
object discovery. The subscription handler plans discovery for currently
visible registered objects; the process service rechecks and commits known
object state before queueing or sending discovery events. Ordinary-object
initial relevance advisories and RTI-owned MOM initial values remain separate
paths. The process `HLAsetSwitches(HLAautoProvide)` handler independently
updates federation switch state. A bounded source scan found
`planAutoProvideForDiscovery` only in its declaration, definition, and embedded
discovery call site. The process switch test toggles and inspects the registry
value; a separate process discovery test observes `discoverObjectInstance`
under HLA_EVOKED and HLA_IMMEDIATE. No focused process test connects discovery
to a provider solicitation. Added a source-only process boundary flowchart to
the Auto Provide guide; it makes no normative or cross-profile parity claim.
The 2025-only recount includes both Mermaid fence styles and finds 308 charts
across 31 guides (174 flowcharts, 111 sequences, 23 state diagrams). Mermaid
12.1.0 parsed all 308 with zero grammar failures through the in-memory no-op
DOMPurify adapter; syntax only. The new chart remains unrendered. No builds,
tests, CI, or Requirements Lab work.

2026-10-16 review: audited the 2025 configured-process name-reservation to
named-registration to peer-discovery path against the existing guide, service
and registry source, and focused process tests. The guide already separates
the local reservation-outcome callback, registration commit/consumption, and
peer discovery; its process evidence table also states that no focused test
combines a reserved name, named registration, and peer discovery. The process
reservation test verifies success callback and later named registration in
both HLA_EVOKED and HLA_IMMEDIATE using one client. A separate process test
observes peer discovery in both callback models after unnamed registration.
Source shows registration commit precedes discovery planning, and the process
service queues discovery before constructing the registration response; no
test establishes relative observation order across the owner's handle return
and peer callback. The existing guide already presents these boundaries and
limits accurately, so no guide edit was warranted. No builds, tests, CI, or
Requirements Lab work.

2026-10-16 review: audited the configured-process ordinary receive-order
interaction send-to-callback path against current 2025 source, focused tests,
and the existing object/interaction guide. The guide already follows recipient
selection through event-time class/parameter projection, event materialization,
pull polling or push framing, and callback-bridge delivery. Source confirms
route-only candidates are resolved before materialization; later declarations
do not rewrite the event. Push frames are sent sequentially, while pull mode
queues events. The focused raw process send case checks the pulled envelope,
not a public callback. The configured-process multi-recipient case observes
exact-class callbacks and per-receiver FIFO tags under HLA_EVOKED and
HLA_IMMEDIATE; it does not cover ancestor promotion, post-send declaration
changes, partial push failure, or callback-before-sender-return ordering. The
guide already labels these boundaries and gaps accurately, so no guide edit
was warranted. No builds, tests, CI, or Requirements Lab work.

2026-10-16 review: audited 2025 configured-process delete-callback producer
identity after `HLAprivilegeToDeleteObject` transfer against the ownership
guide, process source, and focused process tests. Receive-order and timestamped
removal paths take callback identity from the removal snapshot, which retains
the original registrant; the internal TSO deletion message separately records
the deleting requester. The callback bridge uses the removal snapshot. The
focused receive-order and timestamped process tests register and delete through
the same sender, so they do not distinguish these identities and do not cover a
transferred delete-privilege owner. The distinct registrant/deleter case exists
only in the embedded focused test. The guide already labels this source/test
boundary accurately, so no guide edit was warranted. No builds, tests, CI, or
Requirements Lab work.

2026-10-16 review: audited 2025 callback-exception capture and callback-control
state against the API declaration, adapter, shared dispatcher, focused unit
tests, and current guide. `enableCallbacks` sets the enabled switch before an
immediate backlog drain. If a callback throws on that drain, the dispatcher
has already removed the failing task, retains later tasks, clears its drainer
election, and rethrows without rolling back the switch (unless callback code
changes it). In `HLA_EVOKED`, enabling does not dispatch; a later Evoke claims
the task, whose exception follows the existing dispatcher boundary. The API
header lists `SaveInProgress`, `RestoreInProgress`, and `RTIinternalError` for
`enableCallbacks`; the inline callback-exception outcome remains source-only,
not a normative conclusion. Existing dispatcher tests cover successful
enable/disable gating but do not throw from a callback. Added one source-only
flowchart to the callback/service-ordering guide. Mermaid 12.0.0 core accepted
the new chart; local Chromium rendering failed with code 2147483651. The
2025-only diagram inventory is now 309 (175 flowcharts, 111 sequences, 23
states); the prior 308 passed Mermaid 12.1.0 core parsing. No builds, tests,
CI, or Requirements Lab work.

2026-10-16 review: traced configured-process `synchronizationPointAchieved`
through registry commit, pushed notification, correlated response, client
callback drain, and the public service wrapper. The registry records the final
achievement and erases the completed synchronization point before notification
delivery and response. Assuming the push frame and success response arrive, the
client matches the response and unlocks before draining the pushed event. If
the completing member's `HLA_IMMEDIATE` callback is next and throws, the local
request throws after the server-side transition: the wrapper attempts
`HLAreportException`, does not show rollback or a failed-service-invocation
report, and skips the later successful service-report append. Under
`HLA_EVOKED`, callback submission queues work, so the wrapper can finish before
a later Evoke delivers it. Push-send failure after registry transition is a
separate error path and was not folded into the callback-exception trace.

The focused configured-process synchronization test includes the receiver in
the explicit set and observes its non-throwing completion callback, but does
not assert callback-before-return or exception-after-commit. The API lists
`FederateInternalError` on the callback declaration but not on
`synchronizationPointAchieved`; this implementation trace does not resolve a
normative outcome. Added a source-only sequence to the 2025
callback/service-ordering guide. The 2025 inventory is 310 charts (175
flowcharts, 112 sequences, 23 state diagrams): 308 previously parsed with
Mermaid 12.1.0, while the callback-enable and new synchronization-point charts
separately passed Mermaid 12.0.0 core syntax parsing using the no-op
DOMPurify adapter. Chromium rendering failed with code 2147483651. No builds,
tests, CI, or Requirements Lab work.

2026-10-16 review: traced untimed configured-process `requestFederationSave`
through immediate-start admission, pushed initiation, correlated response,
client drain, and the public wrapper. This trace is bounded to a federation
with no time-constrained member; otherwise the request remains pending and
initiation is deferred to the time boundary. On immediate admission, the
registry creates the active save operation and marks participants
`FEDERATE_INSTRUCTED_TO_SAVE` before notification frames and the applied
response are sent. If the requesting member's `HLA_IMMEDIATE`
`initiateFederateSave` callback throws during the post-response drain, the
public request can throw after admission. The wrapper attempts
`HLAreportException`, skips its later success service-report append, and shows
no rollback or failed-service-invocation report. The save remains active and
the throwing member has not called `federateSaveBegun`; other members may
already have progressed. With `HLA_EVOKED`, the request can return and append
its success report while initiation waits for explicit Evoke. A push-send
failure after registry transition is a distinct path, not covered by this
trace.

The focused `HLA_IMMEDIATE` process test uses a non-throwing callback and
checks ordinary initiation/failure/status behavior. The focused `HLA_EVOKED`
process case verifies initiation remains unobserved until Evoke. Neither tests
a throwing initiation callback or the post-exception active state. Added one
source-only sequence to the 2025 save/restore guide and corrected the nearby
test/source timing description. Mermaid 12.0.0 core accepted the chart using
the no-op DOMPurify adapter; visual rendering was not attempted for this new
chart. The 2025 inventory is 311 (175 flowcharts, 113 sequences, 23 state
diagrams): the prior 308 passed Mermaid 12.1.0, while three latest charts
separately passed Mermaid 12.0.0 syntax parsing. Seventeen local destinations
in the new subsection were checked separately (16 source/API/test line anchors
and one guide-heading link). No builds, tests, CI, or Requirements Lab work.

2026-10-16 review: traced the configured-process 2025 `federateSaveComplete`
path from the last participant's barrier transition through image commit,
terminal notification, response matching, callback drain, and the public
adapter. The registry commits the encoded image before planning
`federationSaved`; encoding/store failure plans
`federationNotSaved(RTI_UNABLE_TO_SAVE)`. The save-control result remains
`applied` when notification delivery succeeds because the participant's
Complete choice was accepted. A pushed-event send failure is a separate path
that can return an internal error after registry state has changed.

For pushed events, the client drains callbacks before returning its typed
result. `HLA_IMMEDIATE` invokes the terminal callback inline, before the public
wrapper's successful `FederateSaveComplete(true)` report append; an escaping
RTI `Exception` can therefore make the public call throw after the federation
outcome is resolved, attempt `HLAreportException`, and skip that success
append. No compensating rollback is shown. `HLA_EVOKED` queues the callback,
allowing the service call/report to return before later observation. The
one-member `HLA_EVOKED` process test uses polling and directly checks that
`federationSaved` is unobserved after Complete, then delivered by Evoke. A
separate pushed-event multi-member `HLA_EVOKED` test later observes the final
member's save callback after Evoke, but does not assert a zero callback count
immediately after Complete. An internal registry test covers injected store
failure. No focused public process test covers commit failure, a throwing
terminal callback, or service-report order, so those branches remain
explicitly source-derived.

Added one 2025 sequence. The 2025 inventory is now 312 charts across 31 guides
(175 flowcharts, 114 sequences, 23 state diagrams); the new chart passed
Mermaid 12.0.0 core syntax parsing only and has not been visually rendered.
Sixteen local source/test anchors in the subsection were checked separately.
No 2010 material was changed; no builds, tests, CI, or Requirements Lab work.

### 2025 configured-process Delete Object after privilege transfer review — 2026-10-16

Reviewed the configured-process Delete Object Instance path against the
existing 2025 Object and Interaction Information and Attribute Ownership
guides. The process handler validates the session/federate, then the registry
resolves `HLAprivilegeToDeleteObject` for the instance's registered class and
compares that instance's current privilege owner with the requester. The TSO
planner applies the same owner check and derives effective order from the
special attribute. The internal TSO message records the deleting requester;
the receiver's removal snapshot supplies the original registrant identity to
`removeObjectInstance`. Accepted deletion clears the deleting federate's local
knowledge and invalidates pending work; other known receivers become unknown
only when their reserved removal passes its delivery gate.

The existing guides already show these distinctions and explicitly separate
source traces from public evidence. The focused process receive-order and
timestamped tests register and delete through the same sender; the timestamped
fixture references the special attribute for its order setup but does not
transfer ownership. Neither asserts the former owner's rejection, the new
owner's success, nor callback identity when registrant and deleting federate
differ. The embedded transfer-and-delete test remains separate evidence. No
additional chart or guide change was warranted; no process parity or normative
claim is made.

This was a documentation-only review. No builds, tests, CI, Requirements Lab,
or 2010 material were touched.

### 2025 variant-definition-to-DataElement bridge diagram — 2026-10-16

Reviewed the 2025 variant-record reader journey across FOM composition and
runtime encoding. The composer merges and validates a semantic variant
definition; a federate separately constructs `HLAextendableVariantRecord` and
registers its discriminant-to-`DataElement` map through `addVariant`. The
focused composer case checks a `DrinkGarnish` catalog entry
([test](../../../cpp/tests/libxml2_fom_composer_catch2.cpp#L1291)); the runtime
codec case builds its map directly and asserts octets
([test](../../../cpp/tests/ieee1516_2025_basic_data_elements_catch2.cpp#L994));
the configured-process Create case accepts the Restaurant FOM but does not
exercise variant encoding
([test](../../../cpp/tests/process_federation_additional_fom_catch2.cpp#L363)).
The evidence does not show a catalog-to-codec conversion or end-to-end
catalog-to-octets test.

Added one bridge flowchart to the 2025 FOM composition guide. It presents the
two paths side by side and marks the dotted connection as an evidence boundary,
not a required application architecture or normative rule. The new chart
passed Mermaid 12.1.0 core syntax parsing with an in-memory no-op sanitizer.
Mermaid CLI 12.0.0 could not launch Chromium (code 2147483651), so the chart
has no render or visual-review evidence. The 2025 inventory is now 313 charts
across 31 guides: 176 flowcharts, 114 sequences, and 23 state diagrams. Four
local destinations in the new subsection were checked separately: three
test line anchors and one guide-heading link. This check also found and fixed
two stale 2025 test anchors in the FOM and DataElement guides (the composer
assertion now links to line 1291 and the codec test to line 994).

This was documentation-only. No builds, tests, CI, or Requirements Lab work.

Next handoff: when browser launch access changes, render and visually inspect
the new 2025 FOM/runtime bridge chart and the first queued process
regional-attribute DDM chart. Do not claim layout or GitHub rendering before
visual review; keep the work 2025-only and documentation-only.

### 2025 regional class-request navigation review — 2026-10-16

A bounded review of the queued process regional-attribute DDM topic found that
the configured-process regional class-request sequence already exists in the
[2025 attribute-value update request guide](../HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#configured-process-path-region-check-before-event-routing).
It traces local adapter validation, typed process-region rejection, provider
and object expansion, request/source-region filtering, per-group projection,
and push/pull callback delivery. Its process tests and untested callback-time
region/ownership boundaries are described in that guide; the sequence is an
implementation trace, not a normative timing or profile-parity guarantee.

Added a direct link to the existing sequence from the §9.13 row in the 2025
DDM guide. No duplicate diagram was added, and the 2025 inventory remains 313
charts across 31 guides. Visual rendering remains pending: Mermaid CLI failed
to launch a browser profile, so no layout or GitHub-presentation claim is made.
This was documentation-only; no builds, tests, CI, Requirements Lab, or 2010
material were touched.

Next handoff: render and visually inspect the existing configured-process
regional class-request sequence and the FOM/runtime variant bridge when local
browser launch works. If it remains unavailable, continue bounded 2025
reader-gap review without duplicating the existing request chart.

### 2025 ownership reader-gap review — 2026-10-16

Reviewed the configured-process Unconditional Divestiture and Release
Denied/cancellation items against the existing
[2025 ownership guide](../HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md). It already
contains the Unconditional Divestiture process route, the embedded test with
regular and If-Available requests pending before divestiture, and explicit
process limits for an already-pending regular request and post-commit
enqueue/send failure. It also contains a source-only Release Denied/cancellation
race chart, separate push/pull checkpoints, and the explicit gap that no
configured-process test combines Release Denied with cancellation after owner
callback entry. The arranged after-entry test is embedded only.

These are no-change dispositions: the current evidence is already explained
without claiming a process outcome or normative priority, and another chart
would duplicate the existing guide. The gaps remain open pending new evidence.
The 2025 diagram inventory remains 313 across 31 guides. Browser rendering and
visual review remain pending; this was documentation-only, with no builds,
tests, CI, Requirements Lab, or 2010 material touched.

Next handoff: continue with the embedded class-level Request Attribute Value
Update pre-enqueue partial-persistence risk in the 2025 attribute-request
guide. Keep it labeled as a source observation unless a focused test or
normative evidence supports a stronger claim. Visual review of the existing
regional-request and FOM/runtime diagrams remains queued separately.

### 2025 embedded class-request pre-enqueue review — 2026-10-16

Reviewed the embedded class-level Request Attribute Value Update boundary in
the [2025 request guide](../HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md)
against the current ambassador and registry code. Final planning runs under
the shared federation-management lock; that lock is released before the
per-object/provider persistence loop. Each group is registered separately and
revalidates the full planned owner projection. A later stale group can fail
after earlier groups have persisted, with no rollback in the inspected path.
The guide already labels this as a source-visible potential partial-persistence
failure, not HLA behavior, and explicitly says no focused test covers an
ownership change during pre-enqueue persistence or callback entry.

No-change disposition: the existing explanation and evidence limit are
accurate, so no duplicate chart or stronger runtime claim was added. The gap
remains open pending a focused test or other authoritative evidence. No builds,
tests, CI, Requirements Lab, or 2010 material were touched.

Next handoff: review the 2025 process RECEIVE temporal gate and the
active-establisher/passive-receiver DDM boundary in the delay-subscription and
relevance-advisory guides. Keep untested interleavings explicitly marked and
add a diagram only if it improves the existing explanation without implying
unsupported process parity.

### 2025 DSE/DDM/synchronization evidence-boundary review — 2026-10-16

Cross-checked the process receive-order recheck, passive discovery, stale
regional scope, and partial-achievement-before-late-Join items against the
existing 2025 guides. The delay-subscription guide already traces the process
pre-event and RECEIVE-side regional rechecks and limits its EVOKED/IMMEDIATE
test evidence to the exercised regional path. The object-subscription guide
explains how another member's active relevance can establish discovery while
a different passive receiver passes its own declaration check, and says the
three-role process case is untested. The Attribute Scope guide records process
admission, dequeue, and callback checkpoints while noting the public process
test has no stale-event race. The synchronization guide labels partial
achievement before late Join as source-derived and not covered by either
focused profile test.

No-change disposition: these gaps are already documented with appropriate
limits across the relevant 2025 guides. No new chart or cross-profile claim is
warranted; the evidence gaps remain open. No builds, tests, CI, Requirements
Lab, or 2010 material were touched.

Next handoff: review the 2025 Attribute Scope Advisory switch-only suppression
case against its embedded and configured-process tests. Keep the public
process transition test's no-stale-event boundary distinct from the embedded
switch-gating result; add no new diagram unless the existing guide leaves a
real reader question unanswered.

### 2025 Attribute Scope Advisory switch-only review — 2026-10-16

The [2025 scope guide](../HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md)
already distinguishes the embedded callback-entry switch check from the
process pull-queue dequeue recheck, and treats both pending-event outcomes as
source-derived. Its evidence map says the embedded transition case also
performs final self-unsubscribe, the public process case leaves the switch
enabled, and no focused test isolates switch-off as the sole suppressor.

No-change disposition: the existing chart and caveats already preserve the
correct scope; the missing switch-only test remains open. No duplicate chart
or normative claim was added. No builds, tests, CI, Requirements Lab, or 2010
material were touched.

Next handoff: review the configured-process object-removal producer identity
after delete-privilege transfer in the 2025 object/interaction guide. Keep the
original registrant, current delete-privilege holder, and recipient callback
identity separate unless focused test evidence proves a stronger relationship.

### 2025 object-removal producer identity cross-link — 2026-10-16

The 2025 ownership guide already traces the process boundary after
`HLAprivilegeToDeleteObject` transfers: the internal TSO deletion message
records the deleting requester, while the public removal callback takes its
producer identity from the original registrant in the removal snapshot. The
focused process receive-order and timestamped delete tests use one federate
for both registration and deletion, so this remains source-only for a
transferred privilege.

Added a direct link from the object/interaction removal explanation to the
existing ownership sequence. No second identity diagram was needed, and the
embedded transfer-and-delete test was not generalized to process behavior.
This was documentation-only; no builds, tests, CI, Requirements Lab, or 2010
material were touched.

Next handoff: continue the 2025 clause-reference audit without treating source
or test observations as normative evidence. Mark any clause whose canonical
standard prose is not available as unverified rather than filling it by
inference.

### 2025 TSO deletion and pending ownership boundary — 2026-10-16

Compared timestamped Delete enqueue, the first accepted Remove registry gate,
legal Retract/reconstitution, and ownership callback state. For a
recipient-bearing deletion, enqueue saves the invocation snapshot and
retraction ledger without clearing pending ownership work. The first accepted
Remove commits deletion and clears the per-instance ownership-operation
ledgers plus federation-level ownership-query identities. Retract before that
gate cancels the reservation without restoring a snapshot; after it, the
reconstitution path restores the saved `ObjectInstance`, but does not visibly
rebuild the separately stored query identities.

Updated the TSO retraction chart and added a boundary note to the receive-order
ownership guide. The embedded registry gate precedes the public callback; the
process registry gate precedes event send and its client callback gate. The
existing timestamped-delete tests cover pre-delivery cancellation and
post-delivery object/ownership reconstitution, but not pending ownership query
or operation callbacks in the same scenario. Their resumption is therefore
unverified, not a normative or cross-profile conclusion. Documentation-only;
no 2010 material, builds, tests, CI, or Requirements Lab work.

Next handoff: review whether restored per-instance ownership-operation records
can admit pre-delete callback work after a delivered TSO Delete is retracted,
while separately erased federation-level ownership-query identities remain
absent. Use only 2025 callback gates and exact test cases; leave the outcome
explicitly unverified if no focused test settles it. Do not change runtime
behavior in this documentation pass.

### 2025 embedded ownership callback closures across TSO Retract — 2026-10-16

Traced callback-route closures against the 2025 TSO Delete registry cleanup
and reconstitution branches. A closure running during committed deletion is
suppressed at its registry gate. A later legal Retract reinstalls the saved
`ObjectInstance` state, but the retraction path does not visibly resubmit that
ownership callback closure. If the closure remains queued until after Retract,
its restored per-instance request identity may pass the later gate. Query
callbacks use a separate federation-level identity map that deletion erases
and Retract does not rebuild. The reviewed callback gates and state ownership
make these distinct source-observed schedules; they do not establish public
callback outcomes.

Updated the existing TSO retraction diagram and the ownership guide boundary
note. The focused timestamped-delete/retraction tests verify pre-delivery
cancellation or post-delivery object/ownership restoration, but none combines
either queued-ownership-callback ordering with TSO Delete/Retract. No source
behavior was changed; no builds, tests, CI, Requirements Lab, or 2010 work.

Next handoff: audit the 2025 configured-process timestamped Delete route for
server-side ownership event queues and client callback buffers across first
accepted Remove and Retract. Inspect source and focused test coverage, preserve
the separate process/embedded boundaries, and make no parity claim without a
combined test. Keep this documentation-only.

### 2025 configured-process ownership queues across TSO Retract — 2026-10-16

Traced the configured-process route separately from embedded callback
closures. Pull-mode ownership events stay in per-session server queues until a
Receive poll applies an operation-specific registry gate or projection;
push-mode events may already be in the client `pendingEvents_` or callback
bridge. The first accepted process TSO Remove commits deletion before sending
the removal event. Legal Retract filters TSO-identity-matched
interaction/removal/update work, but it does not generally sweep ownership
events that have a different identity. Owner-release callbacks retain a
distinct late claim; no generic callback-bridge ownership recheck was found.

Updated the TSO guide with a separate process queue/buffer diagram and linked
the source-only boundary from the ownership guide. Focused process tests cover
pushed object-removal suppression and timestamped deletion/time-advance
delivery, but none combines pending ownership callbacks with timestamped
Delete/Retract. The cross-queue callback outcome remains unverified. This was
documentation-only; no runtime, build, or test changes were made.

The new Mermaid flow passed Mermaid 12.1 core syntax parsing with an in-memory
no-op sanitizer. `mmdc` could not launch its browser (exit code 2147483651), so
the chart has not been rendered or visually reviewed. Keep the paused process
regional-attribute DDM sequence as the first visual-review item; review this
new TSO chart after that.

Next handoff: resume the paused visual review of the process regional-attribute
DDM sequence and diagnose the local browser-launch failure. Check Mermaid layout
and GitHub readability, then review the new TSO process-buffer diagram. Continue
bounded 2025 source-to-diagram reviews without building the C++ runtime.

### 2025 selected diagram visual review — 2026-10-16

Rendered and visually reviewed three selected diagrams with the local Mermaid
CLI/Chromium workaround: the configured-process regional-attribute DDM sequence,
the configured-process TSO ownership-buffer flow, and the FOM/runtime variant
bridge. The DDM chart was readable and left unchanged. The TSO chart was
rearranged into compact pull/push groups; the FOM bridge was changed from a
compressed horizontal layout to a clearer top-to-bottom layout. All three
previews were legible at review size.

This is selected local visual QA, not whole-tree sign-off or evidence of GitHub
presentation. Updated the 2025 backlog and roadmap handoff to record three
reviewed diagrams and the next selected chart: configured-process Federation
Save completion. The pending queue remains 2025-only. No implementation,
build, test, CI, or Requirements Lab work was done.

### 2025 Federation Save completion diagrams — 2026-10-16

The existing configured-process completion sequence rendered too wide for
comfortable reading. Reworked it into two sequences along the behavior's real
boundary: (1) participant completion, the federation barrier, last-member image
commit, and terminal event; (2) process callback observation and the public
service-report boundary. The first keeps an accepted Complete response
distinct from federation-wide commit success. The second contrasts
`HLA_IMMEDIATE` inline callback handling, its source-derived exception/report
path, and deferred `HLA_EVOKED` observation.

Both diagrams passed the local Mermaid CLI parse/render and were visually
reviewed at preview size. A punctuation parse error and a single-participant
note syntax issue were corrected before the final renders. The chart boundary
remains deliberately narrow: notification-send failure is omitted; no focused
public process test injects image-commit failure or a throwing terminal
callback, nor asserts service-report order. The linked registry test is
internal evidence, not a process endpoint result. These are source-derived
branches, not normative callback-order claims.

Updated the backlog and roadmap inventory from 314 to 315 2025 diagrams
(177 flowcharts, 115 sequences, 23 state diagrams). Five selected diagrams are
now locally rendered and visually reviewed; GitHub presentation and the
remaining selected queue are still pending. Next: render and review the latest
RAVU overload chart in the 2025 Request Attribute Value Update guide. No C++
builds, tests, CI, or Requirements Lab work.

### 2025 RAVU overload diagram visual review — 2026-10-16

Rendered and visually reviewed the existing configured-process remote failure
mapping chart in the 2025 Request Attribute Value Update guide. The chart is
tall, but its three overload paths and the generic-versus-typed remote failure
branches remain legible while scrolling, so no layout or content change was
warranted. Its surrounding text keeps the scope to remote planner failures
after local preflight and records the process-test limits; this visual pass
does not broaden those claims.

The 2025 inventory remains 315 diagrams across 31 guides. Six selected charts
are now locally rendered and visually reviewed. Next: review the selected
ownership/delete-privilege chart in the 2025 Attribute Ownership guide. GitHub
presentation and the remaining chart queue are pending; no C++ build, tests,
CI, or Requirements Lab work.

### 2025 ownership/delete-privilege visual review — 2026-10-16

Rendered the embedded transfer-and-delete sequence in the 2025 Attribute
Ownership guide. The first preview let the removal-callback label overrun the
right edge. Shortened participant and callback labels while retaining the key
facts in the chart: B must publish and acquire the special delete privilege,
B's deletion uses B's tag, and the observer's removal callback reports A as
producer. The revised chart fits and is legible at preview size.

The guide and linked focused test sources bound this to the selected embedded
`HLA_EVOKED` scenario. They distinguish per-attribute ownership from the
special deletion privilege; configured-process transfer behavior remains
unverified, and no test rerun or normative callback claim is added. The 2025
inventory remains 315 diagrams across 31 guides. Seven selected charts are
locally rendered and visually reviewed. Next: render the catalog-versus-instance
ownership diagram in the same 2025 guide. GitHub presentation remains pending;
no C++ build, tests, or CI.

### 2025 catalog-versus-instance ownership diagram visual review — 2026-10-16

Rendered and visually reviewed the existing catalog-versus-instance ownership
diagram in the 2025 Attribute Ownership guide. Its two lanes fit in the preview:
the composed FOM class schema establishes where the attribute is available,
while a particular registered object's current privilege owner is checked by
the instance deletion gate. The side-by-side layout makes clear that catalog
inheritance does not assign or prove per-object ownership; no diagram edit was
needed.

The guide's composer cases and selected embedded deletion test bound the two
evidence lanes. They were inspected but not rerun; this review makes no
configured-process parity or normative timing claim. The 2025 inventory remains
315 diagrams across 31 guides. Eight selected charts are now locally rendered
and visually reviewed. Next: review the configured-process producer
`DELETE_OBJECTS` identity chart in the 2025 Object and Interaction Information
guide, preserving its source-only ordering and focused-test limits. GitHub
presentation remains pending; no C++ build, tests, CI, or Requirements Lab work.

### 2025 process DELETE_OBJECTS planning and receiver-gate diagram review — 2026-10-16

The first preview of the configured-process producer `DELETE_OBJECTS` flow was
too tall and narrow for a useful page scan. Split the existing chart at the
recipient-plan handoff, a real ownership boundary: one chart now covers
producer-side route preflight, reservation, and producer-known-object removal;
the second covers each survivor's session check, pull/push path, receiver
revalidation, stale-plan suppression, and callback projection. This was a
readability and actor-ownership split, not a diagram-count target.

Both revised Mermaid charts rendered and were visually reviewed locally. They
fit without clipped labels and keep default pull, optional push, and the
receiver-resigns-before-polling case distinguishable. The guide already bounds
this as Umbra 2025 process source behavior: the focused process delete test is
for explicit deletion, not resignation-originated `DELETE_OBJECTS`; the
portable TCK scenario is not evidence that this process path ran. No callback
ordering or process/embedded equivalence is claimed, and tests were not rerun.

Replacing one chart with two raises the 2025 inventory to 316 diagrams across
31 guides (178 flowcharts, 115 sequences, 23 state diagrams). Ten selected
charts now have local visual review. Next: review the regional class-RAVU
per-group projection/failure chart in the 2025 Attribute Value Update Request
guide. GitHub rendering remains pending; no build, tests, CI, or Requirements
Lab work.

### 2025 regional class-RAVU per-group visual review — 2026-10-16

The original configured-process regional class-RAVU flow was too broad for a
useful page-scale view. Split it at the meaningful “all groups prepared”
boundary: the first chart follows per-group re-projection, registration, and
Pull queueing versus Push staging; the second follows the final Pull return or
sequential Push sends. The source-derived failure paths show possible earlier
queued/staged work if a later registration fails and a possible sent prefix if
a later Push send fails. No focused failure-injection test establishes those
runtime outcomes, so the charts claim neither HLA atomicity nor callback
effects.

Both vertical charts rendered with Mermaid CLI and were visually reviewed
locally. The preparation chart is tall but legible while scrolling; the
completion chart stays compact. The 2025 inventory is 317 diagrams across 31
guides (179 flowcharts, 115 sequences, 23 state diagrams), with twelve selected
charts visually reviewed. GitHub presentation and whole-tree visual review
remain pending. Next: review the ordinary Send Interaction wrapper-specific
failure-reporting flow in the 2025 Service Invocation Reporting guide. No C++
build, test, CI, or Requirements Lab work.

### 2025 ordinary Send Interaction failure-report chart review — 2026-10-16

Rechecked the chart against the embedded catch and configured-process early
endpoint branch, plus the focused public process service-report case. The
embedded catch attempts applicable MOM/public-exception hooks and then the
failed-invocation writer. The process wrapper translates a rejected endpoint
response before its successful writer and has no failed-invocation-writer call
on that reviewed branch; the cited process case covers success only. Process
local preflight failures and the regional/timestamped overloads are outside
this chart's comparison.

Corrected a misleading chart edge that visually placed the server's special
`HLAreportMOMexception` route after the process response translation. The
prose now keeps malformed or specifically rejected `HLAsetSwitches` handling
as a separate server-side MOM path, not an invocation report. The revised chart
passed the installed Mermaid core parser. Mermaid CLI could not launch a local
browser renderer, and the desktop launch exposed no targetable window, so no
image or visual review is claimed. Keep this chart first in the visual queue;
GitHub presentation remains unverified. No build, test, CI, or Requirements Lab
work.

### 2025 ordinary Send Interaction failure chart visual review — 2026-10-16

Rendered the revised wrapper-comparison flow with the installed local Mermaid
CLI and inspected the resulting image. The first preview was readable but
wrapped several labels into one-word lines. Shortened those labels without
changing the paths or evidence boundary, then rendered and reviewed the final
view. Its embedded and configured-process branches are distinct, all labels
fit, and the Service Reporting decision is confined to the embedded failed-
invocation writer path. The vertical chart remains scrollable and readable.

This remains an Umbra source/test comparison, not an HLA rule: the cited
configured-process public case covers success only, the embedded report paths
are not asserted together by a cited failure test, and the server's special
HLAsetSwitches MOM-exception behavior remains separately described. The 2025
inventory remains 317 diagrams across 31 guides (179 flowcharts, 115
sequences, 23 state diagrams); thirteen selected charts now have local visual
review. GitHub presentation and whole-tree review remain pending. Next: review
the regional `Send Interaction With Regions` reporting flow, preserving its
public-preflight versus server-rejection and timestamped/process boundaries.
No C++ build, test, CI, or Requirements Lab work.

### 2025 regional Send Interaction With Regions reporting chart visual review — 2026-10-16

Rendered the existing receive-order/timestamped configured-process comparison
with Mermaid CLI and visually inspected the final local image. The vertical
two-branch layout is readable while scrolling; node labels fit, and no branches
overlap or clip. Shortened the invalid-parameter/region edge label before the
final render; no behavior or evidence claim changed.

The receive-order path distinguishes the two local preflight failures that
call the failed-invocation writer from other validation failures, then separates
successful service reporting from a rejected server response translated to
`RTIinternalError`. The timestamped process branch has no service writer on
the reviewed success or failure paths. Focused process cases cover invalid
region and parameter preflight reporting, not server rejection or timestamped
process reporting; cited timestamped regional-send report tests are embedded
only. These are source/test boundaries, not normative behavior or profile
parity.

The 2025 inventory remains 317 diagrams across 31 guides (179 flowcharts, 115
sequences, 23 state diagrams); fourteen selected charts now have local visual
review. GitHub presentation and whole-tree visual review remain pending. Next:
review the revised Connect/Disconnect process-host cleanup flow. No C++ build,
test, CI, or Requirements Lab work; no 2010 stream work.

### 2025 Connect/Disconnect process-host cleanup chart visual review — 2026-10-16

Rendered the four Mermaid diagrams in the existing Connect/Disconnect guide
with Mermaid CLI and visually inspected the process-host cleanup flow. The
diagram is tall but readable while scrolling, with no clipped labels or
overlapping branches. It keeps the ambassador's committed local teardown and
public return separate from the process host's later session-exit cleanup.

The source shows public Disconnect closing the local process client without a
Disconnect protocol request or wait for a remote host worker. The host serving
loop separately handles connection loss, detaches the session, then closes its
worker connection. Focused public-process evidence calls `detach` explicitly
in its fixture; the separate host test observes successful public Disconnect
and host shutdown but does not assert server-session removal or cleanup timing.
This is an implementation/test boundary, not normative behavior or
process/embedded parity.

Updated the guide and backlog to record local visual review. The 2025 inventory
remains 317 diagrams across 31 guides (179 flowcharts, 115 sequences, 23 state
diagrams); fifteen selected charts now have local visual review. GitHub
presentation and whole-tree visual review remain pending. Next: review the
revised Federation/Federate Lifecycle host-versus-fixture cleanup flow. No C++
build, test, CI, or Requirements Lab work; no 2010 stream work.

### 2025 Federation/Federate Lifecycle host-versus-fixture cleanup review — 2026-10-16

Rendered all fifteen Mermaid diagrams in the lifecycle guide with Mermaid CLI
12.0.0 and visually inspected the revised process-host cleanup chart. The
vertical layout is tall but legible while scrolling; labels and branches are
unclipped. Source review found that the previous catch path visually rejoined
the normal membership-cleanup path, overstating what a retry proves.

The host normally calls `connectionLost` when its request loop ends. A serving
exception or false/throwing normal cleanup result enters the catch path, which
records the failure and retries once. The retry's false result is ignored; a
thrown retry error is recorded, and `detach` still follows. A false normal
result may occur before registry loss applies or after registry mutation when
survivor-effect projection fails. The revised chart separates the normal
successful/no-member path from the best-effort retry and detach path, so it
does not imply that membership cleanup or every survivor effect necessarily
completed.

Focused evidence remains limited: the public host test uses unjoined clients
and checks Disconnect plus host shutdown, not joined-member removal or cleanup
timing. The current-process fault fixture explicitly applies forced loss and
detaches; it is not evidence for automatic host cleanup. The revised flow is
source/test-derived, not a normative timing rule or process/embedded parity
claim.

Updated the lifecycle guide and backlog. The 2025 inventory remains 317
diagrams across 31 guides (179 flowcharts, 115 sequences, 23 state diagrams);
sixteen selected charts now have local visual review. GitHub presentation and
whole-tree review remain pending. Next: review the DataElement variant
comparison flow. No C++ build, test, CI, or Requirements Lab work; no 2010
stream work.

### 2025 DataElement variant comparison visual review — 2026-10-16

Reviewed the existing variant comparison against the two 2025 codecs and
their focused Catch2 cases. For `HLAvariantRecord`, a mapped discriminant
selects an alternative aligned to the maximum boundary among mapped
alternatives; an unmapped discriminant ends at the discriminator without
padding or an alternative-length field. Whole-value decode separately rejects
trailing octets. `HLAextendableVariantRecord` pads the discriminant to four
octets, writes a signed big-endian `encoded_length`, then pads to its fixed
eight-octet alternative boundary. The length covers alternative bytes only,
not either prefix pad. A mapped decoder must consume exactly that many bytes;
an unknown mapping can skip the declared payload, but the element does not
retain it and `getVariant()` still throws. The current encoder writes length
zero for an unmapped discriminant.

The focused cases cover mapped and unmapped values, alignment, malformed or
truncated input, declared-length mismatch, and skipping a three-octet unknown
payload. The source rejects negative lengths; the test does not establish a
parent decoder continuing after the skip. Runtime alternatives are configured
explicitly with `addVariant`; the cited test constructs this map directly and
does not build it from the composed FOM catalog. The revised Mermaid chart now
labels the payload-only length and is tall but legible when scrolling. All
eight diagrams in the 2025 DataElement guide rendered locally; only the
variant-comparison chart received this visual review. These are bounded
implementation/test observations, not a general normative or catalog-to-codec
claim.

Updated the DataElement guide, selected-review list, and next handoff. The
inventory remains 317 diagrams across 31 guides (179 flowcharts, 115
sequences, 23 state diagrams); seventeen selected charts now have local visual
review. GitHub presentation and whole-tree review remain pending. Next: review
the revised 2025 process DSE attribute flow. No C++ build, test, CI, or
Requirements Lab work.

### 2025 configured-process DSE attribute projection boundaries — 2026-10-16

Revised the process attribute flow after comparing its branches with the 2025
service, receive-side, TSO-dispatch, and callback-bridge code. A route-only
candidate retained under enabled DSE can reach three distinct checks:

- Receive-order pull and regional receive-order push carry the event and
  available source-region context to the receive fence or callback-task
  admission, where current attribute subscription and region overlap are
  re-evaluated.
- Nonregional receive-order push remains on the existing pre-event path. A
  route-only candidate is re-resolved before event construction, not at the
  later receive/callback boundary. Timestamped recipients outside the
  constrained-recipient TSO queue also fall through per-recipient event
  construction; the source performs its empty-projection recheck there.
- A queued timestamped passel places only currently time-constrained
  recipients on the process TSO queue. Its later recipient-dispatch projection
  uses the sent-region handles and snapshots.

Each lane may retain a subset of sent attributes. The passel is suppressed
when the current projection is empty. This makes the chart's process-specific
timing explicit without implying a common callback-time check or profile
parity.

Focused process DSE evidence remains narrow: the regional attribute test runs
an `HLA_EVOKED` pull scenario and an `HLA_IMMEDIATE` push scenario, with those
transport options selected by the fixture; that mapping is not a general
callback-model rule. It checks late-subscription eligibility, sent-region
snapshot behavior, later suppression, and a separate immediate callback/Resign
fence. It does not cover nonregional push or timestamped process attribute
delivery; those branches remain source-only. The guide states these limits and
links the branch, receive, TSO, bridge, and focused-test evidence.

Rendered all four Mermaid diagrams in the 2025 DSE guide and visually reviewed
the revised process attribute chart. The tall chart remains legible while
scrolling; its three branch labels, shared projection decision, and suppressed
path are visible without clipping. The chart count did not change. The
2025-only inventory remains 317 diagrams across 31 guides (179 flowcharts,
115 sequences, 23 state diagrams); eighteen selected charts now have local
visual review. GitHub presentation and whole-tree review remain pending. Next:
review the 2025 process directed-interaction TSO flow. No C++ build, test, CI,
or Requirements Lab work.

Reviewed and refined the 2025 process directed-interaction TSO flow. The chart
keeps delivery-time route selection and the departed-producer exception, and
now distinguishes producer Retract before dispatch from a still-legal Retract
after the process event frame is sent. The latter can remove a buffered event
or suppress it before callback entry; if the original callback has started,
the bridge queues `requestRetraction`. These are source-derived process
boundaries, not normative or cross-profile claims. No focused process test
covers Retract of a queued directed TSO message; the process send/retraction
case is receive-order only. The revised chart rendered and was visually
reviewed. The 2025 inventory remains 317 diagrams across 31 guides (179
flowcharts, 115 sequences, 23 state diagrams); nineteen selected charts now
have local visual review. GitHub presentation and whole-tree review remain
pending. Next: review the 2025 update-rate generation flow. No C++ build,
test, CI, or Requirements Lab work.

Reviewed the existing 2025 Attribute Update Rate subscription-generation
chart against the embedded declaration mutators, receiver-wide generation
field, admission-key builder, gate storage/cleanup, and focused evidence.
Ordinary Subscribe advances generation on its successful mutation path;
ordinary Unsubscribe does so when receiver/class records exist; Regional
Subscribe advances for a nonempty outer map even when an inner region set is
empty; Regional Unsubscribe advances only when it removes a stored pair. The
generation rotates the per-attribute history key. Older timestamps can remain
cached until receiver departure or federation teardown; this is source-only
storage behavior. The cited unit case constructs distinct keys rather than
invoking a declaration service, and the cited tests do not measure retained
entry growth or end-to-end public history reset. The §9.8.4 empty-region no-op
question remains unverified. Rendered and visually reviewed the generation
chart. The 2025 inventory remains 317 diagrams across 31 guides (179
flowcharts, 115 sequences, 23 state diagrams); twenty selected charts now
have local visual review. GitHub presentation and whole-tree review remain
pending. Next: review the 2025 revised FOM post-Join flow. No C++ build, test,
CI, or Requirements Lab work.

Reviewed the revised 2025 FOM post-Join consequence flow against the selected
registry, embedded/process Join, MOM, and focused-test evidence. A fresh
isolated Mermaid CLI 12.0.0 render succeeded and visual review found the
branches legible. Review caught a chronology ambiguity: “Join succeeds” was
shown before the registry commit. The chart now starts at a prepared
additional-module Join request and explicitly labels the atomic catalog,
handle-directory, and membership commit; prose links to the preceding phase
chart for validation/composition and profile-specific post-commit setup or
rollback. The consequence chart separately shows catalog vocabulary, retained
application-object state, membership-driven HLAfederate discovery, embedded
HLAcurrentFDD reflection, and later registration under the new catalog.

Evidence remains bounded: the configured-process additional-FOM test checks
catalog lookup only; embedded HLAcurrentFDD reflection has a focused test;
there is no combined process post-Join observer case, and the normative
Join-to-callback obligation remains unresolved. The 2025 inventory remains
317 diagrams across 31 guides (179 flowcharts, 115 sequences, 23 state
diagrams); twenty-one selected charts now have local visual review. GitHub
presentation and whole-tree review remain pending. Next: review the 2025
per-recipient order-conversion flow. No C++ build, test, CI, or Requirements
Lab work.

Reviewed the 2025 per-recipient sent-TSO order-conversion flow against the
embedded timestamped interaction sender and its focused public-API test. The
one `HLA_EVOKED` case observes `sentOrderType == TIMESTAMP` for both receivers;
the unconstrained receiver reports `receivedOrderType == RECEIVE`, while the
constrained receiver reports `TIMESTAMP` after its time advance. Both reports
retain a valid retraction designator and the test checks payload, tag,
transportation, logical time, and interaction-before-grant callback order.

The chart now includes callback polling: the unconstrained receiver polls after
the send, the constrained receiver polls before advancing and has no
interaction callback yet, then requests time 6, while the sender requests time
2 as part of this test schedule. Constrained polling observes the TSO callback
and grant. A local Mermaid CLI 12.0.0 render was visually reviewed; the
sequence is legible without clipping and does not imply asynchronous callback
delivery or a repeated grant on every poll. The §8.1.4 Table 3 source-comment
mapping has not been checked against licensed standard text. This is one
embedded scenario, not process parity or a complete conversion table. The
2025-only inventory remains 317 diagrams across 31 guides (179 flowcharts,
115 sequences, 23 state diagrams); twenty-two selected charts have local
visual review. GitHub presentation and whole-tree review remain pending.
Next: review the 2025 reservation-owner restore flow. No C++ build, test, CI,
or Requirements Lab work.

Reviewed the saved-reservation sequence in the 2025 Object-Instance Name
Reservation guide against state-image capture, restore validation, and the
focused two-federate public test. The image stores the name with Federate A's
identity; a fresh registry is created in the same process, both members rejoin
with their original handles, and restore validation reattaches the claim to
the live owner. Federate B cannot release or reserve that name until A releases
it; after B reserves it, A's release fails.

The sequence rendered with Mermaid CLI 12.0.0 and was visually reviewed. I
shortened its saved-image note for readability; all sequence steps remain
legible without clipping. The guide appropriately limits this to the embedded
profile and notes that callback reports are checked after a follow-up helper,
not at a universal service-return boundary. The test creates a fresh registry
within one process, so it does not prove OS-process-restart behavior or a
normative save/restore rule. The 2025-only inventory remains 317 diagrams
across 31 guides (179 flowcharts, 115 sequences, 23 state diagrams); twenty-
three selected charts have local visual review. GitHub presentation and
whole-tree review remain pending. Next: review the updated embedded Request
Attribute Value Update flow. No C++ build, test, CI, or Requirements Lab work.

### 2025 embedded Request Attribute Value Update visual review — 2026-10-16

Rendered and visually reviewed the updated embedded class-request chart with
Mermaid CLI 12.0.0. The chart is tall but legible, with no clipped nodes or
labels; its per-provider/object loop and later report/dispatch/callback-entry
recheck remain understandable, so no chart edit was warranted.

The source review confirms a regional overlap filter per group, full planned
owner-projection validation at each persistence call, then report and callback
dispatch followed by a current-eligibility recheck for the original provider.
No focused test exercises an ownership change in this pre-enqueue interval.
Earlier groups may already have been persisted if a later group fails, but
that remains a source-visible possibility—not a tested runtime outcome or an
HLA rule. The 2025 inventory remains 317 diagrams across 31 guides (179
flowcharts, 115 sequences, 23 state diagrams); twenty-four selected charts
have local visual review. GitHub presentation and whole-tree review remain
pending. Next: review the 2025 Attribute Scope process-event transport chart.
No C++ build, test, CI, 2010, or Requirements Lab work.

### 2025 Attribute Scope process-event transport source review — 2026-10-16

Rechecked the existing process-event chart against the 2025 service, receive,
client-dispatch, callback-bridge, and configured-process endpoint-test paths.
Both routes filter planned attributes against current scope immediately before
routing. Pull mode stores the event in the server session queue and recomputes
eligible attributes when a later receive poll removes it. Push mode sends the
already-filtered event across the process boundary; the traced client dispatch
and callback bridge contain no later registry scope recheck. This describes
source checkpoints only, not an HLA callback cutoff.

The public endpoint test configures pull mode for `HLA_EVOKED` and pushed mode
for `HLA_IMMEDIATE`, then checks regional transition callbacks and subsequent
delivery. It does not create a stale-event race or test a switch/region change
between admission and callback observation. The chart's distinction and its
test caveat are accurate, so its diagram logic needed no change. Three stale
source-line anchors across the chart evidence row and adjacent pushed-callback
boundary table now point to the current dispatch branch, bridge method, and
callback delivery function.

Local visual review remains pending. Mermaid CLI 12.0.0 could not launch its
browser, including with the locally installed Chromium/Edge paths, and the
already-running Chromium window could not be activated after refreshing the
window list. The Markdown extractor also skipped this guide's tilde-fenced
chart, so the attempts were made against the isolated chart source; no valid
image was produced. Keep the chart out of the 24-chart visual-review count.
Next: defer retrying this chart until a supported browser launch path changes;
the ordinary/regional Send Interaction and Service Invocation Reporting
charts are already visually reviewed. Continue the 2025-only source/test-gap
survey in the meantime. No builds, tests, CI, 2010, or Requirements Lab work.

## 2025 configured-process Save notification-send failure boundary — 2026-10-16

Reviewed the configured-process `federateSaveComplete` path across registry
completion, terminal-event routing, client response handling, and callback
dispatch. The registry resolves the federation save before the service routes
terminal notifications. In pushed mode, the service sends recipient frames
sequentially; an encoding or later send failure can produce an internal-error
response after earlier frames were sent. If the requesting member's own frame
was already buffered, the client drains pushed events before the typed Save
Complete method validates response status: a returning `HLA_IMMEDIATE`
callback may run before the ambassador raises `RTIinternalError`, while
`HLA_EVOKED` work may remain queued for a later Evoke. No rollback of the save
transition or prior frames is visible. The reviewed public process tests do
not inject a partial notification-send failure, so this remains a
source-composed possibility, not a tested outcome or HLA callback rule.

Expanded the 2025 Save/Restore guide's failure-boundary prose without changing
its Mermaid charts; the selected local-visual-review count remains twenty-four.
A fresh Attribute Scope render attempt using Mermaid CLI 12.0.0 and the
explicit installed Chromium executable failed before producing an image
(browser process exit 3221225506). Keep that chart visually pending and do not
retry until browser launch conditions change. No build, test, CI, 2010, or
Requirements Lab work.

Next handoff: audit the 2025 configured-process regional class-level Request
Attribute Value Update preflight against its validation and provider-expansion
paths and the focused public cases. Determine whether any test proves that a
late invalid region suppresses otherwise-eligible provider work; preserve the
result as source-only if not. Add no duplicate diagram if the current guide
already makes the order clear. Keep the visual retry deferred until a browser
launch path changes.

## 2025 configured-process regional class-request preflight audit — 2026-10-16

The shared registry validates every supplied attribute/region pair before
expanding class scope into instances and providers; the configured-process
handler returns a typed rejection before its provider loop. The public
invalid-selector case makes four separate calls for unknown, foreign,
uncommitted, and wrong-context regions. Its fixture does not publish the class
or register an object, so no otherwise-eligible provider route is established
whose suppression it could observe. The four public exception outcomes are
verified; whole-request no-event behavior remains source-only, not a
normative atomicity or exception-precedence claim.

Clarified this test boundary in the 2025 RAVU guide. Its existing flow already
shows preflight before provider expansion, so no duplicate diagram was added.
No Mermaid charts changed; the selected local-visual-review count remains
twenty-four. No build, test, CI, 2010, or Requirements Lab work.

Next handoff: audit the 2025 process three-role passive-discovery boundary.
Trace whether the configured-process registration/subscription path uses the
shared cross-attribute active-relevance gate and inspect focused public tests
for a separate active establisher and passive receiver on distinct attributes.
Keep process outcomes separate from the embedded example and source-only if
no public test covers the three-role case. Defer visual retries until a
supported browser launch path changes.

## 2025 configured-process three-role passive-discovery evidence audit — 2026-10-16

The configured-process subscription handler delegates declaration updates and
existing-object discovery planning to `EmbeddedFederationRegistry`; registration
delegates object creation and per-instance discovery to the same registry.
The discovery selector's cross-attribute active-relevance gate is therefore
shared source, but this does not establish an observed process outcome.

The public process object-subscription test joins one member, subscribes
actively, switches that declaration to passive, and unsubscribes while checking
the stored per-member declaration. It does not register an ordinary object or
add a separate active establisher. The test supports process API/declaration
routing only; the three-role discovery result remains source-only and is not
inferred from the embedded passive-only test. Clarified the 2025 subscription
guide and added direct source/test links; no diagram changed. No build, test,
CI, 2010, or Requirements Lab work.

Next handoff: audit ordinary configured-process late subscription to an
already-registered object. Trace existing-object discovery, initial-value
event production, Pull/Push staging, and callback observation; keep the
RTI-owned MOM path separate, and leave ordinary-object process behavior and
the exact §5.8 interpretation unresolved without focused evidence. Defer the
Attribute Scope render until a supported browser launch path changes.

## 2025 configured-process late-subscription initial-value audit — 2026-10-16

The existing subscription guide already distinguishes an object becoming newly
known from adding an attribute to an already-known object, and separates
ordinary objects from RTI-owned MOM. Bounded source review confirms the process
subscription handler replans discovery through the shared registry; the
enqueue path creates a separate initial-value event only for an RTI-owned MOM
object. The ordinary branch creates no attribute-update event.

Pull queues discovery and any MOM initial-value event separately per session.
Push stages the event types separately and sends discovery frames before MOM
value frames. The ordinary path therefore has no initial-value event in either
mode. The public ordinary process case subscribes before registration and
observes discovery under its callback-model variants, but has no reflection
observer. The MOM process case subscribes after the MOM objects exist and its
HLA_EVOKED observer verifies two discoveries and two reflections with selected
values; it does not assert their relative callback order or establish Push
ordering. The exact §5.8 interpretation remains unreviewed, so this is a
source/test boundary, not a conformance conclusion.

Added this Pull/Push staging distinction to the existing 2025 guide; the
existing profile chart already answers the reader question, so no duplicate
diagram was warranted. No build, test, or CI work.

Next handoff: audit the configured-process untimed Update Attribute Values
failure mapping. Trace local adapter handle validation, remote planner statuses,
service rejection, client response translation, and focused public tests.
Keep timestamped behavior separate and classify any untested remote failure
mapping as source-derived, not a conformance claim. Defer Attribute Scope
rendering until a supported browser launch path changes.

### 2025 configured-process untimed Update Attribute Values failure mapping — 2026-10-16

Traced the untimed configured-process adapter through its process client and
service. Invalid local object and attribute handle wrappers map to
`ObjectInstanceNotKnown` and `AttributeNotDefined`; after conversion, the
registry planner distinguishes unknown object, undefined attribute, and
not-owned attribute. The process handler collapses every non-applied planner
result (and a session/member mismatch) into a generic rejected response. The
client raises `ProcessFederationClientError`, which the public adapter exposes
as `RTIinternalError`. The selected embedded source translator maps these
planner outcomes to their typed API exceptions, but this is a source comparison,
not a normative or parity conclusion.

The focused public process endpoint case verifies successful update submission
with zero recipients under its callback-model sections; it does not exercise
the remote failure mapping. The new source-bounded flow is in the 2025
object-and-interaction guide. Mermaid 12.0.0 core parsing passed after using an
in-memory no-op sanitizer to bypass the Node-only DOMPurify initialization
problem; this is syntax evidence only, and no visual render was attempted. The
2025 source inventory is now 318 diagrams (180 flowcharts, 115 sequences, 23
state diagrams), all recorded as syntax-accepted, with 24 selected charts
visually reviewed. No C++ build, test, CI, or Requirements Lab work.

Next handoff: audit 2025 configured-process discovery-triggered Auto Provide.
Trace subscription/discovery, solicitation, Pull/Push delivery, callback entry,
and focused public tests. Keep embedded behavior distinct; classify untested
process outcomes as source-derived, not conformance claims. Keep the Attribute
Scope visual review paused until a supported browser launch path changes.

### 2025 configured-process Auto Provide source/test boundary — 2026-10-16

Re-audited the 2025 process route against the Auto Provide guide. The only
`planAutoProvideForDiscovery` call site remains in the embedded object
discovery callback. The configured-process subscription handler replans
existing-object discovery; the process discovery service commits
receiver-known state and queues or sends the discovery event, with ordinary
initial relevance advisories and RTI-owned MOM values handled separately. The
process HLAautoProvide handler updates the federation-wide switch but does not
invoke the discovery planner.

Focused process evidence is split: the public object-discovery case observes
`discoverObjectInstance`, while the process HLAsetSwitches case changes the
switch and checks its server-side registry value. Neither records
`provideAttributeValueUpdate` during discovery. The existing Auto Provide
guide already contains a source-bounded process flowchart and this evidence
matrix, so no duplicate diagram or prose change was warranted. This is not a
claim that the standard omits process behavior, nor a parity conclusion. No
build, test, or CI work; the 2025 inventory remains 318 syntax-accepted
diagrams and 24 selected local visual reviews.

Next handoff: audit 2025 configured-process Federation MOM FOM updates on
module-adding Create/Join. Trace the MIM concept, process event production,
live MOM state, and public observer tests; keep source and standard evidence
distinct and revise the existing guide only if it leaves a real learner gap.
Keep Attribute Scope visual review paused until browser launch conditions
change.

### 2025 configured-process Federation MOM FOM update review — 2026-10-16

Rechecked the existing 2025 Federation MOM guide's additional-module Join
section against the pinned MIM, process Join and request paths, and focused
tests. The MIM marks `HLAFOMmoduleDesignatorList` Conditional when Create/Join
adds FOM modules and `HLAcurrentFDD` Conditional when those services modify
the Current FOM subset. In the configured-process service-report-store Join
path, source commits the composed federation definition and membership,
establishes the execution-scoped `HLAfederation` object, then queues discovery
for the joining member's `HLAfederate` MOM object; there is no federation-level
Conditional-update enqueue on that path. An explicit current-value request
reads the live module list and Current FDD projections.

The configured-process additional-FOM API case verifies that Join succeeds
and the added class can be resolved by name, not Federation MOM callbacks.
The focused Current FDD reflection case is explicitly embedded-profile
evidence; the configured-process joined-member MOM observer covers different
attributes and does not observe either federation Conditional field. The
existing [Federation MOM guide](../HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#2025-process-profile-federation-mom-after-an-additional-module-join)
already includes the HLA trigger, process sequence, live-value route, Pull/Push
staging boundaries, test limits, and source-only caveat. No prose or diagram
change was warranted. This does not determine normative conformance or
cross-profile parity. No C++ build, test, CI, or Requirements Lab work.

Next handoff: review configured-process `HLAfederatesInFederation` updates at
Join/Resign. Trace the MIM trigger, registry membership projection, process
member discovery/removal events, explicit current-value request, and public
observer tests; revise the existing guide only for a demonstrated learner gap.
Keep the Attribute Scope visual review deferred until browser launch
conditions change.

### 2025 process Federation MOM membership-list review — 2026-10-16

Rechecked the Federation MOM membership section against the pinned 2025 MIM,
registry projection, process Join/Resign dispatch, current-value request path,
and focused tests. The MIM defines `HLAfederatesInFederation` as a
`HLAfederateReferenceList` and Conditional on member Join or Resign. The
configured-process registry encodes the live membership map for the
execution-scoped Federation MOM object. Join/Resign update registry membership
and separately discover/remove the affected member's `HLAfederate` object; the
reviewed service paths do not enqueue a Federation-list reflection. The
explicit current-value request requires a remaining member who knows the
Federation MOM object, then reads the live map.

The focused public list-reflection case is embedded-profile evidence. The
configured-process resignation case verifies member-object removal before a
later member object's rediscovery/reflection; it does not observe the
Federation membership attribute. No process public observer case for automatic
Join/Resign list reflection or explicit list request was found. The existing
[Federation MOM guide](../HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#2025-process-profile-membership-list-updates-at-join-and-resign)
already presents this boundary, so no prose or diagram change was warranted.
Keep the process outcome source-only and unresolved normatively. No C++ build,
test, CI, or Requirements Lab work.

Next handoff: audit the 2025 FOM-to-`DataElement` variant-record bridge.
Trace standard type concepts, module composition/resolution, runtime
construction and encoding, and process-facing tests; preserve the integration
evidence gap and avoid process/embedded parity claims. Keep Attribute Scope
visual review deferred until browser launch conditions change.

### 2025 FOM variant-record value mapping — 2026-10-16

Re-audited the existing composition-to-codec bridge against the 2025
`FomDataTypeDefinition` projection, the pinned `HLAextendableVariantRecord`
API, the Restaurant FOM, and the separate composer, codec, and process Create
tests. The model side retains discriminant and alternative names/type
references; the runtime API takes typed `DataElement` prototypes and values.
The Restaurant example now makes the application-side translation explicit:
`Margarita = 1` and `DryMartini = 2` in `DrinkType` must be mapped to runtime
discriminant elements and payload prototypes. It is clearly marked as
illustrative; no catalog-to-codec integration is claimed. Existing tests still
do not establish that integration, so their evidence boundaries remain
separate. No Mermaid changed; no build, tests, CI, 2010, or Requirements Lab
work was performed.

Next handoff: audit the 2025 public Update Attribute Values and Send Interaction
value boundary. Trace what the application encodes into `DataElement` octets,
what the RTI validates or forwards, and what receivers decode, including the
configured-process frame boundary. Amend an existing guide only for a
demonstrated learner gap; keep API/source/test evidence separate and do not
infer process/embedded parity. Do not build or rerun tests.

### 2025 process Update Attribute Values payload boundary — 2026-10-16

Audited the 2025 application-value path across the public attribute map, the
configured-process request/event path, callback reconstruction, and focused
test. The public map carries `VariableLengthData`; the process service validates
session/member and attribute identities, plans from the identities, and copies
the supplied value octets into accepted state and recipient events. The private
event codec frames those bytes with handles, while the callback bridge rebuilds
the public `AttributeHandleValueMap`; it does not decode a `DataElement`.
The public process reflection test checks literal `52 46 4C` value bytes and
`54 41 47` tag bytes at the callback. This verifies that tested path's byte
preservation, not a typed encode/decode round trip or a general payload
validation rule. The existing process Send Interaction guide already states
the analogous ordinary-parameter boundary and distinguishes the special
RTI-consumed `HLAsetSwitches` path. Added only the missing symmetric update
explanation; no diagram changed. No C++ build, test, CI, 2010, or Requirements
Lab work.

Next handoff: audit the 2025 predefined `HLAsetSwitches` interaction. Trace
parameter interpretation, switch-state updates, and MOM projection through
the API, embedded/process source paths, and existing focused tests. Keep the
ordinary interaction path distinct; amend an existing guide only for a
demonstrated learner gap. Do not build or rerun tests.

### 2025 predefined HLAsetSwitches scope and parameter flow — 2026-10-16

Audited the pinned 2025 MIM paths, API-level handling, embedded/process
decoders, state update, MOM projection, and focused process tests. The short
name `HLAsetSwitches` identifies two separate control interactions: the
federation-root form adjusts `HLAautoProvide`; the joined-federate form adjusts
eight `HLAswitch` values and `HLAautomaticResignAction` (`HLAresignAction`).
The existing Federation MOM guide now places these scopes side by side and
adds a flowchart for special RTI consumption, parameter decoding, extension
parameters, staged member updates, the Service Reporting interlock, and
projection to current MOM values/getters. It keeps ordinary interaction
fan-out separate and does not infer embedded/process parity. Selected process
tests cover state, compatible extension parameters, malformed values, and the
interlock; the embedded setter test still fails before reaching its switch
assertions. No test or build was run.

The edited section's 67 local paths and line anchors resolve. Mermaid syntax
validation remains pending: the direct core-parser attempt fails at the local
`DOMPurify.addHook` dependency, while mmdc rendering fails to launch Chromium.
The new chart is one addition beyond the prior 318-chart 2025 inventory; it is
not counted as syntax-accepted or visually reviewed.

Next handoff: audit the 2025 `HLAreportMOMexception` flow for rejected
predefined `HLAsetSwitches` adjustments: process rejection, report construction,
recipient selection, and related embedded evidence. Compare only with the
existing exception-reporting guide; keep MOM exception reports distinct from
`HLAreportException`, ordinary `HLAserviceInvocation` reports, and public
service exceptions. Amend only for a demonstrated gap. Do not build or rerun
tests.

### 2025 HLAreportMOMexception and rejected HLAsetSwitches review — 2026-10-16

Re-audited the 2025 malformed/precondition-rejected `HLAsetSwitches` route
against the pinned MIM, embedded catch and report helper, configured-process
server handler and recipient planner, and focused embedded/process cases. The
existing Exception Reporting guide already distinguishes the MOM report from
`HLAreportException`, `HLAreportServiceInvocation`, and the caller's public
exception. It explains malformed versus service-precondition
`HLAparameterError` values, the report payload and inherited parent projection,
the `HLAfederate` dimension-scoped recipient route, the independent exception
switch, and the embedded/process hook boundary. Its caveats correctly avoid
claiming that tests cover every combined route or callback schedule.

No additional explanation or diagram was needed. A focused check of the Route
B references found one stale source line for the embedded catch-order link; it
now points to the current catch at line 2395. All 27 local references in that
route section resolve, including heading and line anchors. No build or tests.

Next handoff: audit the two 2025 `HLArequestFOMmoduleData` class paths and
their module-index/report projection across the existing Federation MOM and
FOM admission guides plus focused evidence. Keep federation/member request
scope and returned report content distinct. Amend only for a demonstrated
reader gap; do not build or rerun tests.

### 2025 FOM module-data request/report profile boundary — 2026-10-16

Audited both 2025 MIM request classes, embedded report construction, focused
embedded tests, and the configured-process public/server Send Interaction
path. The existing guide already explains the federation versus member index
namespaces, the federation request's sole `HLAFOMmoduleIndicator` parameter,
the member request's inherited `HLAfederate` target, and their distinct paired
reports. Embedded tests cover federation and member-scoped reports; the
member-target test sends to itself and does not establish every cross-member
combination.

The configured-process public path serializes the parameters/tag into the
private envelope. The reviewed server handler special-cases federation Auto
Provide and joined-federate `HLAsetSwitches`, then passes other interactions to
the generic receive-order planner, which requires the producer to have
published the interaction class. Both FOM request classes are `Subscribe` in
the pinned 2025 MIM. The bounded source/test search found no process-specific
FOM request/report planner or focused process case. Added this source-only
boundary to the Federation MOM guide. The exact process outcome, public
exception, report production, and process/embedded parity remain unverified;
no normative conclusion is inferred.

All 29 local references in the edited request/report section resolve. No
diagram or test was added, and no build or tests were run.

Next handoff: audit the 2025 `HLArequestMIMdata` path across the embedded
report handler, configured-process Send Interaction route, and focused
evidence. Compare it with the existing Federation MOM guide; document only a
demonstrated gap, and keep process behavior source-bounded. Do not build or
rerun tests.


### 2025 Federation/member lifecycle Join-discovery boundary review — 2026-11-03

The Join/MOM diagram's existing-subscriber discovery step was ambiguous. The
embedded and configured-process Join handlers use the joined-federate MOM
planner after membership/session setup, so the arrow represents existing
eligible members discovering the new member's RTI-owned HLAfederate object.
Ordinary existing object instances follow a separate subscription-time planner
for the subscribing member. The embedded public lifecycle test observes a
second member joining after the observer subscribes; the process report-file
case observes a fresh MOM instance after the subject resigns and rejoins while
the observer remains subscribed. Those focused cases do not establish
universal callback ordering.

Clarified the chart label and added source/test links and prose distinguishing
the two audiences. Mermaid CLI 12.0.0 rendered the updated chart; local
screenshot review found the text readable. No new diagram or test, build, or CI
run. Requirements Lab and 2010 content were untouched.

Next handoff: Review 2025 Federation Synchronization participant-set changes across Join, voluntary Resign, and connection loss. Compare the Synchronization guide with Federation and Federate Lifecycle and Process Endpoint; trace waiting and announcement sets plus callback projection, and identify a concrete reader gap before changing a diagram. Keep normative barrier semantics separate from Umbra state and profile-specific evidence. No Requirements Lab, 2010, builds, tests, or CI.


### 2025 synchronization departure adapter-order review — 2026-11-03

Cross-reviewed the late-Join, voluntary-Resign, and forced-ConnectionLost
participant-set flows in the Synchronization guide against Lifecycle and
Process Endpoint. The registry removes the departing ID from all three point
sets, keeps the barrier pending while any remaining participant is
unreported, plans completion for remaining routed participants once all have
reported, and erases an empty point without callbacks.

The existing departure chart did not show the source-order difference between
the adapters. Embedded Resign and ConnectionLost submit synchronization work
before object-removal work; configured-process Resign and ConnectionLost use
the same dispatcher, which enqueues removals before synchronization events.
Those are call/enqueue orders, not callback-entry guarantees. The focused
embedded HLA_EVOKED resignation test has no removal callback, and the 38
loss-focused Catch2 sources plus process connection-lifecycle TCK source have
no direct FederationSynchronized case for forced loss. Added this boundary to
the existing chart and linked back from Lifecycle.

Mermaid CLI 12.0.0 rendered the chart and local screenshot review found it
readable. No tests, builds, CI, Requirements Lab, or 2010 content changed.

### 2025 Save/Restore member-exit adapter review — 2026-11-03

The shared registry member-exit transition fails an active Save or Restore,
plans notifications only for eligible remaining participants with usable
routes, clears the operation, then finishes membership removal. Save and
Restore cannot overlap under current admission checks. Voluntary Resign uses
`FEDERATE_RESIGNED_DURING_SAVE`; forced ConnectionLost uses
`RTI_DETECTED_FAILURE_DURING_SAVE`. Restore uses
`FEDERATE_RESIGNED_DURING_RESTORE` for either trigger. These are Umbra source
choices, not an independent decision on IEEE failure-reason semantics.

The embedded Resign and ConnectionLost adapters submit planned Save and Restore
notifications. Configured-process Resign and ConnectionLost use
`dispatchConnectionLossResult`; it enqueues `saveNotifications` but does not
enqueue `restoreNotifications`, before the otherwise implemented process
Restore event queue, codec, and callback-bridge path. The focused resignation
tests cover embedded `HLA_IMMEDIATE` Save failure/retry, embedded
`HLA_IMMEDIATE` Restore failure, and embedded `HLA_EVOKED` suppression of the
departed participant's queued Restore callbacks. The bounded connection-loss
Catch2 source scan found no active Save/Restore case. No forced-loss callback
or configured-process member-exit behavior is established.

Updated the existing Lifecycle flow to compare voluntary Resign and forced
ConnectionLost; added the process event-queue cross-reference and clarified the
Restore failure-reason comparison in the Save/Restore guide. Corrected stale
Restore-case links that pointed into the Save-resignation case. Mermaid CLI
12.0.0 rendered the chart; local screenshot review found it readable. The
inventory remains 31 2025 guides / 387 diagrams (220 flowcharts, 143
sequences, 24 state diagrams); selected visual QA is 116 reviewed / 0 pending.
No tests, builds, CI, Requirements Lab, or 2010 content changed.

Next handoff: Review what re-evaluates a process NMR/NMRA after a successfully
retracted TSO has already lowered its effective target. Trace grant
re-evaluation triggers in Retract, TSO delivery acknowledgment, and subsequent
time-advance paths; compare the source with focused 2025 tests. Keep the
combined grant/callback outcome unclaimed unless directly observed. No
Requirements Lab, 2010, builds, tests, or CI.
