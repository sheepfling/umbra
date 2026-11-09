# IEEE 1516.1-2025 Documentation Progress History — 2026-11-08

[History index](HLA-2025-DOCUMENTATION-PROGRESS-HISTORY.md) · [active roadmap](HLA-2025-DOCUMENTATION-ROADMAP.md)

## 2025 Process Endpoint TSO acknowledgement and retraction audit, increment 285 — 2026-11-08

Checked the configured-process TSO send, acknowledgement, and Retract claims
against the pinned 2025 source. The guide now links the attribute and
interaction frame sends, cached retraction-recipient registration, and the
service's in-transit-until-ACK boundaries directly to their source lines. The
ACK path points separately to session/member validation and coordinator-ledger
completion. Retract delivery and the registry's current-member notification
insertion also have direct anchors. This keeps callback completion, process
transport, registry notification, and public retraction state distinct.

The relative source-link checker resolves 4,240/4,240 unique references across
34 Markdown files against the candidate worktree, pinned base, and current
source revision. The current cross-revision inventory is 854 links into 35
changed targets across 33 documents (816 relative links and 38 immutable
permalinks; 56 source/test paths changed). The Process Endpoint guide has 110
links into 18 changed targets. The 123 immutable GitHub permalinks also retain
their pinned-line check. These checks establish destination and line existence;
the broader semantic citation audit and authenticated GFM refresh remain open.
Overall progress remains **80%**. No C++ build, tests, CI, 2010 paths, or
Requirements Lab paths were touched; `git diff --check` passed.

**Next handoff:** Finish the remaining Process Endpoint request-exception and
callback-delivery claims, then review Time Management, Callback and Service
Ordering, and Data Distribution and Regions. Keep source claims tied to pinned
2025 behavior, preserve all 48 unrelated worktree rows, and refresh GFM after
the remaining Markdown edits. Publish only with authorization; then inspect
all 32 hosted guide pages and Mermaid hydration.

## 2025 Process Endpoint callback flow and rendering audit, increment 286 — 2026-11-08

Reviewed the configured-process untimed Send Interaction and Send Directed
Interaction failure maps against the pinned public declarations, local adapter
checks, process-client status conversion, service rejection paths, registry
planner outcomes, and focused positive-path tests. The guide correctly
distinguishes typed local errors from remote generic rejection mapped to
`RTIinternalError`; the cited tests do not establish negative public exception
outcomes or exception precedence.

Corrected the process callback flow so pushed-event submission rejoins response
handling before a polled interaction is projected. The diagram now marks
`HLA_EVOKED` work as a later public Evoke call. Fixed a Mermaid note separator
in the FQR callback-exception sequence. Mermaid CLI 12.0.0 rendered and
screenshot review confirmed both diagrams. The visual-review ledger now matches
all 490 diagram bodies and source lines; this increment refreshed 63 line
anchors and two body hashes.

The authenticated GitHub Markdown API check passed for all 34 scoped pages
(32 topic guides, the documentation roadmap, and thematic reading paths):
490/490 Mermaid containers, 936 headings, 7,447 rendered links, and zero API
or diagram-count failures. The source-link checker still resolves 4,240/4,240
unique references across the worktree, pinned hosted base, and current source
revision. These results establish parsing and destination existence, not
semantic support for every claim or visible hydration on published file pages.
Overall progress remains **80%**; the broader source-citation audit and hosted
page inspection remain open. No C++ build, tests, CI, 2010, or Requirements Lab
work was done.

**Next handoff:** Continue the source-backed review of remaining Process
Endpoint callback and request-exception edges, then review the highest-density
2025 guides: Time Management, Callback and Service Ordering, and Data
Distribution and Regions. Preserve the 48 unrelated worktree rows. Publish
only after authorization, then inspect all 32 GitHub guide pages and visible
Mermaid hydration.

## 2025 Process Endpoint deferred-ACK gate, increment 287 — 2026-11-08

Added a flow diagram to the configured-process TSO completion section. It
separates the `requestDepth_` decision to start an ACK exchange or queue the
message ID from the later flush points: after correlated response/event
dispatch, in the background dispatch loop when both depths permit, and before
Evoke after buffered pushes are submitted. The flush path rechecks
`requestDepth_` before draining and validates that the recipient session is
still joined. The note identifies the shared queue's pushed receive-order
attribute ACKs and records that a callback exception can
skip the response path's following flush, release callback suppression, and
rethrow; the background loop remains a separate eligible path. No focused test
forces that combined queued-ACK/callback-exception edge.

Mermaid CLI 12.0.0 rendered and screenshot review confirmed the new ACK gate
diagram and the current body of the existing Embedded Send Interaction
callback-exception diagram. Reconciling the visual ledger found that diagram's
stored hash was stale; it now matches the reviewed current body. The ledger
records 491/491 current diagrams (287 flowcharts, 180 sequence diagrams, and
24 state diagrams). Two Process Endpoint source lines shifted after the new
chart; one existing body hash was refreshed, and the new diagram has a current
render and screenshot record.

The authenticated GitHub Markdown API GFM check passed all 34 scoped pages:
491/491 Mermaid containers, 937 headings, 7,447 links, and zero failures. The
relative source-link checker resolves all 4,240/4,240 references against the
worktree, pinned hosted base, and current source revision. These checks confirm
syntax/container registration and link destinations, not semantic support for
every citation or visible hydration on published pages. Overall progress
remains **80%**; the broad semantic citation review and published-page
inspection remain open. No C++ source, tests, builds, CI, 2010 paths, or
Requirements Lab paths were changed.

**Next handoff:** Review the configured-process callback exception attribution
in the Callback and Service Ordering guide against the pinned client request,
dispatch-loop, pending-exception, and callback-dispatch paths. Check which
public call receives an inline exception versus a background exception, and
keep the current test evidence separate from source-derived branches. Then
continue the Time Management and Data Distribution and Regions source reviews.
Preserve all 48 unrelated worktree changes. Publish only with authorization;
then inspect all 32 GitHub guide pages and visible Mermaid hydration.

## 2025 Callback and Service Ordering exception attribution audit, increment 288 — 2026-11-08

Completed the configured-process callback exception review against the pinned
hosted source revision. The request-backed path matches a response and releases
its exchange lock before draining pushed events. That request returns without
its matched result only when its thread owns the process event drain and the
callback dispatcher also drains synchronously on that thread. If another
thread owns the process event drain, the request returns while that owner
continues delivery. If this request owns event dispatch but another thread
owns the immediate callback drainer, submission appends to its FIFO and
returns; a later callback exception follows the existing drainer's call path.
The background dispatch loop saves the first exception only when it unwinds
through its own callback drain, then the next process request takes and
rethrows it before sending. Under `HLA_EVOKED`, the bridge queues the task for
the recipient's Evoke path. This is source-derived 2025 behavior and establishes
no 2010 parity.

The public `getInteractionClassHandle` example now points to the exact pinned
wrapper, exception-report hook, process report requests, and failed-report
error conversion. The reviewed tests prove successful callback delivery, not a
throwing `FederateAmbassador` or a combined report-failure schedule; those
remain explicit evidence gaps. Retargeted 15 old relative source/test anchors
to 18 immutable citations at the pinned revision, including three test anchors
that were each one line early. Added three more pinned source anchors for the
thread-ownership and drainer exception paths, for 21 new immutable citations.

The citation prose shifted nine later diagram lines in the visual ledger. The
two callback-attribution diagrams were revised to show process-drain and
callback-drainer ownership, rendered with Mermaid CLI 12.0.0, and screenshot
reviewed. An existing Embedded Send Interaction diagram hash was also stale;
that body was rendered and reviewed before its record was reconciled. Three
body hashes and nine later source lines were refreshed. All 491/491 ledger
entries match current source lines, diagram types, and body hashes.

The authenticated GitHub Markdown API GFM check passed all 34 scoped pages:
491/491 Mermaid containers, 938 headings, 7,453 links, and zero API or count
failures. The source-link checker resolves all 4,229 relative references; the
pinned-link audit resolves 144 immutable references across 99 path/line pairs
and 40 targets. These checks establish line existence and GFM parsing, not
support for every claim or visible hydration on unpublished GitHub pages.
Overall progress remains **80%**; the wider semantic review and hosted-page
inspection remain open. No C++ source, tests, builds, CI, 2010 paths, or
Requirements Lab paths were changed.

**Next handoff:** Review the 2025 Time Management guide's service transitions,
grant ordering, and timing-frontier claims against pinned sources and the
focused tests. Separate standard behavior from embedded/process implementation
details and record uncovered edges. Then continue the Data Distribution and
Regions review. Preserve all 48 unrelated worktree changes. Publish only with
authorization; afterward inspect all 32 GitHub guide pages and visible Mermaid
hydration.

## 2025 Time Management grant-boundary and frontier review, increment 289 — 2026-11-08

Reviewed the Time Management guide's advance lifecycle, configured-process
grant path, and GALT/LITS frontier against pinned 2025 source and focused tests.
The embedded sequence is now labeled as embedded-only: its current-time update
occurs at the matching grant callback boundary. The configured-process section
shows the service committing its time state before sending the grant event,
while the client receives and queues pushed TSO events on a separate path.
The focused process test establishes TSO-before-grant callback order but does
not query logical time from a TSO callback; the guide explicitly leaves that
callback-time query value unclaimed.

The frontier wording now says non-stale incoming timestamps: a TSO timestamp
strictly below recipient current time is excluded, while one equal to current
time remains in GALT/LITS until callback acknowledgment. The focused
[bounds case](../../cpp/tests/federation_time_bounds_catch2.cpp#L249) verifies
that transition. The expanded distinctions are in the
[Time Management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md).

Added one configured-process sequence diagram, rendered with Mermaid CLI 12.0.0
and screenshot-reviewed. The ledger now matches all 492 diagram identities,
types, body hashes, and source lines (287 flowcharts, 181 sequences, 24 state
diagrams). Authenticated GitHub GFM passed all 34 scoped pages with 492/492
Mermaid containers, 940 headings, and 7,464 links. The source-line checker
resolves all 4,239 relative references against the candidate worktree, hosted
base, and source revision; the prior pinned-link audit remains 144/144. These
checks establish parsing and line destinations, not semantic support for every
remaining citation or visible hydration on unpublished pages.

Overall progress remains **80%** and the four-gate score remains **3/4 (75%)**.
The remaining work is the broader source-citation review, including Data
Distribution and Regions, followed by publication with authorization and
inspection of all 32 actual GitHub guide pages. No C++ source, tests, builds,
CI, 2010 paths, or Requirements Lab paths were changed.

**Next handoff:** Continue the 2025 Data Distribution and Regions source review.
Audit the guide's region-creation and realization lifecycle, inherited and
excluded dimensions, default-range behavior, recipient selection, and the
interaction between regional routing and TSO/time-advance eligibility. Keep
source-derived behavior distinct from focused test evidence. Then continue the
remaining configured-process and Join/MOM citation checks. Preserve all 48
unrelated worktree changes; publish only with authorization.

## 2025 Data Distribution and Regions citation review, increment 290 — 2026-11-08

Reconciled the guide's source claims against the committed candidate base and
the current worktree. Reanchored class-availability and generic-overlap links
to committed definitions. The attribute-specific dimension and temporary
default-range realization helpers exist only in current-worktree changes, so
the guide now labels those details as worktree-only evidence rather than
linking to line numbers that point at unrelated code in the publishable source
snapshot. The inherited-dimension, excluded-dimension, default-range, and
regional-interaction TCK catalog rows are named as current-worktree evidence;
stale numeric links to unrelated catalog rows were removed. The guide's old
459-diagram/223-screenshot note now points to the current roadmap and visual
ledger instead.

The authenticated GitHub Markdown API returned all 34 scoped pages with
492/492 Mermaid containers, 941 headings, 7,468 rendered links, and zero
failures. The source-line checker resolved all 4,223 unique path/line/guide
references against the worktree, hosted base, and local source revision. The
source-dependency audit records the current-only implementation and catalog
boundaries. These checks verify parsing and destinations, not every remaining
claim's semantic support. No C++ source, tests, builds, CI, 2010, or
Requirements Lab paths changed. No diagram body changed, so the Mermaid ledger
did not need a new render record.

Overall progress remains **80%** and the four-gate score remains **3/4
(75%)**. The broader semantic citation review and publication/page-render gate
remain open.

**Next handoff:** Continue the semantic source-citation audit in the 2025
configured-process Join/MOM guide. Check Join commit and rollback claims,
MOM object visibility, and callback ordering against candidate source and
focused evidence. Preserve all 48 unrelated worktree changes. Publish only
with authorization, then inspect all 32 actual GitHub guide pages and visible
Mermaid hydration.


## 2025 configured-process Join and Federation MOM semantic review, increment 291 — 2026-11-08

Reconciled the lifecycle guide's configured-process Join flow with the 2025
service handler and the Federation MOM guide. The handler commits membership
and the new member's report state, queues HLAfederate discovery, processes
pending synchronization-point announcements, then sends the successful Join
response ([discovery enqueue](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L830),
[announcement path](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L842),
[response path](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L857)).
The inspected handler does not queue a Federation-level
HLAfederatesInFederation conditional update. The lifecycle guide's previous
claim that such an update might escape rollback was removed. The shared helper
call sites are in reporting and interaction-send paths, not Join
([helper](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L132),
[reporting](../../cpp/src/internal/federation/process_federation_service_reporting.cpp#L508),
[interaction send](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L376)).

The MIM still lists HLAfederatesInFederation as Conditional on Join/Resign
([MIM row](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L684)).
This is a source-observed process-profile gap, not a conformance conclusion;
the licensed normative clauses are not present in this checkout. The focused
process rollback test subscribes only to HLAfederateName
([test setup](../../cpp/tests/ieee1516_2025_connection_process_mom_resign_removal_order_catch2.cpp#L291)),
so it does not test membership-list delivery or discovery-callback timing
relative to the Join call's return. The process/embedded distinction and this
test limitation are now stated consistently across the
[Federation MOM guide](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md) and the
[Federation lifecycle guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md).

Updated lifecycle diagram 016 to show the discovery and announcement rollback
boundary and note that the inspected process Join path queues no
HLAfederatesInFederation update. Mermaid CLI 12.0.0 rendered the diagram, and
the screenshot review confirmed readable, unclipped branches and labels. The
visual ledger matches all 492 current diagram bodies and source lines.

The complete ledger audit also found 33 stale Data Distribution guide source-line entries after recent prose edits. Those line fields are now refreshed; all 33 diagram bodies and hashes were unchanged. The lifecycle Join diagram's updated body has its own current render and screenshot record.

The authenticated GitHub Markdown API check passed all 34 scoped pages with
492/492 Mermaid containers, 942 headings, 7,479 links, and zero failures. The
source-link audit counted 5,504 local source-line link occurrences, representing
4,233 unique path/line/guide references; every unique reference resolves against
the worktree, pinned hosted base, and local source revision. These checks verify
render parsing and destinations, not every semantic claim or hydration on
unpublished GitHub file pages. Overall progress remains **80%**; formal gates
remain **3/4 (75%)**. No C++ source, tests, builds, CI, 2010, or Requirements
Lab paths changed.

**Next handoff:** Continue the broader IEEE 1516.1-2025 semantic citation review
across the remaining topic guides. Check normative wording, implementation
boundaries, edge cases, and focused test evidence; distinguish source
observations from conformance conclusions. Preserve the 48 unrelated worktree
changes. The candidate remains unpublished. After publication is authorized,
inspect all 32 actual guide pages and visible Mermaid hydration.


## 2025 additional-FOM Join post-commit rollback review, increment 292 — 2026-11-08

Reviewed the additional-module Join commit and its later setup-failure paths.
The registry swaps the replacement definition and handle directories together
with the new member ([atomic commit](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L618)).
Both implementation paths can still fail afterward and roll back the new
membership ([embedded rollback](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L573),
[configured-process rollback](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L799)).
The registry resignation path removes that member
([member removal](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L1102));
the federation entry is erased only by a separate Destroy operation whose
[guard requires an empty member set and time coordinator](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L154),
followed by the [registry erase](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L158).
If no Destroy follows, the inspected source implies that a post-commit Join
failure can leave the replacement model as the federation's current definition
while the new member has been rolled back. This is source-derived behavior,
not a normative HLA claim.

The focused additional-FOM process test checks that an invalid module rejected
before commit leaves the base module count unchanged and permits a valid retry
([state assertion](../../cpp/tests/process_federation_additional_fom_catch2.cpp#L550)).
The ordinary process Join rollback test injects a later setup failure but does
not supply extra modules or read the catalog afterward
([test](../../cpp/tests/process_federation_service_join_rollback_catch2.cpp#L39)).
The reviewed cases do not combine additional-module commit, later failure, and
catalog readback. I clarified that evidence boundary in the
[FOM module guide](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md)
and updated its Join diagram to show the source-derived outcome only when no
separate Destroy follows. Mermaid CLI 12.0.0 rendered the revised diagram; its
screenshot is readable and unclipped.

The complete ledger audit now matches all 492 diagram identities, types, body
hashes, and source lines. Seven downstream FOM source-line entries were
refreshed; one body hash and screenshot record were updated. Authenticated GFM
passed all 34 scoped pages with 492 Mermaid containers, 943 headings, 7,491
links, and zero failures. The source-link audit counted 5,515 local line-link
occurrences and 4,244 unique path/line/guide references; all unique references
resolve against the worktree, pinned base, and local source revision. No C++ source, tests,
builds, CI, 2010, or Requirements Lab paths changed.

**Next handoff:** Audit the 2025 object and interaction information guide.
Trace catalog lookup, receiver-known class state, discovery callbacks, and
initial reflections as separate transitions, especially when a model changes
after Join. Compare the source paths and focused tests; label any callback
outcome that remains source-derived or untested. Keep 2010 separate.


## 2025 object catalog and future-discovery review, increment 293 — 2026-11-08

Reviewed how an additional-module Join changes model lookup without changing
existing instance identity or receiver-known state, then separated that from
future registrations under a newly added descendant class. The current shared
registry source reconciles the replacement catalog and class handles, then
resolves later discovery from the registered class upward through stored
subscriptions. This means an earlier ancestor subscription may be considered
for a later descendant instance when the normal publication, instance
attribute, relevance, region, and receiver gates pass. Existing instances keep
their registered class, so adding a child does not make discovery search down
from an old parent-class instance. This is an implementation-source inference,
not a portable callback rule.

The focused embedded lookup test confirms the existing class handle stays
stable while the added class becomes resolvable. A separate base/child test
checks closest-class selection and initial reflection when both classes were
present before registration. The reviewed tests do not combine an
additional-module Join, a retained ancestor subscription, registration under
the added class, and discovery/initial reflection. The FOM guide already has a
diagram that traces these model-change branches, including the embedded and
configured-process initial-value distinction; no duplicate diagram was needed.

The documentation link checker resolved all 4,248 unique path/line/guide
references against the candidate worktree, pinned hosted base, and local
source revision. The diagram ledger now matches all 492 identities, types,
body hashes, and source lines; 28 downstream line entries for this guide were
refreshed. `git diff --check` passed for the edited guide. A direct GitHub GFM
API retry for this long guide timed out before returning HTML. The last
successful full-page GFM check remains the 34-page pass after increment 292;
refresh the full page counts before claiming a current GFM pass. No diagram
body changed, so the existing local render and screenshot records remain
current. Overall progress remains **80%** and formal gates remain **3/4
(75%)**. No builds, tests, CI, 2010, or Requirements Lab files were changed.

**Next handoff:** Continue the 2025 semantic audit in Data Distribution and
Regions. Trace the claims about class/dimension availability after an
additional-module Join, retained region specifications, and later discovery or
delivery against the changed-source paths and focused tests. Mark any combined
Join-to-regional-delivery outcome that remains untested as source-derived.
Preserve the 48 unrelated worktree changes. Keep 2010 and Requirements Lab out
of scope; publication and hosted-page hydration remain open.


## 2025 partial interaction-region evidence audit, increment 294 — 2026-11-08

Reviewed the partial interaction-region default-range example against the
current 2025 source. The guide previously presented the recorded installed-
package result as the current source algorithm. It now separates the catalog's
reported Y-dependent delivery result from the worktree source path: the send
planner checks explicit dimensions against the current interaction-class
availability and snapshots the dimensions actually supplied; recipient
selection then uses generic overlap, which compares only dimensions shared by
the two snapshots. The current default-range realization helper is used by the
object-attribute overlap path, not this interaction selector. The revised
diagram makes the source/test discrepancy visible without treating either
trace as normative HLA meaning.

The catalog reports 4/4 installed-package executions across both callback
models and the contract twin. Their JSON/XML run artifacts exist in the primary
worktree's ignored output tree, and the JSON records all four passes. The
associated CMake cache identifies the adapter project path but neither it nor
the run JSON records the source revision, so this result cannot be tied to the
current worktree commit. No rebuild or test rerun was done.

Mermaid CLI 12.0.0 rendered the revised diagram and its screenshot was reviewed.
The ledger now matches all 492 diagrams; 10 downstream source lines, one body
hash, and the updated screenshot review were refreshed. The documentation link
checker resolved 5,527 relative source-line link occurrences representing
4,252 unique path/line/guide references against the worktree, pinned hosted
base, and local source revision; `git diff --check` passed with only Git's
CRLF-normalization notices.
The latest authenticated full-page GitHub GFM pass remains the 34-page pass
after increment 292, so refresh it before calling rendering current. Overall
progress remains **80%** and formal gates remain **3/4 (75%)**. This increment
made no C++ source, test, build, CI, 2010, or Requirements Lab changes.

**Next handoff:** Reconcile the installed-package result's source provenance
using any retained package or build metadata. If no source revision can be
established, keep the result explicitly package-scoped and continue the 2025
semantic review of additional-module Join with retained Regions and later
regional delivery. Preserve unrelated worktree changes and keep 2010 separate.

## 2025 post-Join object-attribute overlap trace, increment 295 — 2026-11-08

Reviewed the current embedded object-attribute path after an additional-module
Join. The attribute-aware overlap code resolves dimensions from the attribute's
declaring class, copies retained Region snapshots, and adds default ranges to
the copies when a newly available dimension has an FDD default. The generic
overlap check can then compare that dimension even though neither stored Region
was rewritten. Added a flowchart that keeps this source-derived path separate
from interaction sends, dimension-context admission, and callback delivery.
No focused test combines a same-class dimension addition, retained source and
receiver Regions, and a later object-attribute match; this remains neither a
normative claim nor tested outcome.

Mermaid CLI 12.0.0 rendered and screenshot-reviewed the new DDM chart and two
current chart bodies whose ledger hashes had drifted. The ledger now matches
all 493 diagrams (288 flowcharts, 181 sequences, 24 state diagrams); 22 later
DDM diagram positions and two existing body hashes were refreshed. The source
link checker resolves all 4,252 unique relative path/line/guide references.
The current GFM pass covers all 34 scoped pages and 493 diagrams: 947 headings,
7,500 links, and zero failures. Overall progress remains **80%** and formal
gates remain **3/4 (75%)**. No C++ tests or builds, 2010 changes, or
Requirements Lab edits were made.

**Next handoff:** Audit the 2025 Federation Save/Restore participant-set
boundary: trace how Join, Resign, or ConnectionLost during an active Restore
affects the captured participants, failed-set construction, and completion
callbacks. Compare embedded and configured-process source with focused tests;
mark untested interleavings explicitly and keep the 2010 stream separate.

## 2025 active Restore participant-set and member-exit audit, increment 296 — 2026-11-08

Traced the Restore admission ledger and terminal paths. The registry captures
the members present when Restore is accepted as `FEDERATE_RESTORING`; its
completion barrier and failure notifications use that captured status map.
Resign and forced ConnectionLost enter the shared resignation transition,
which excludes the departing member, builds `FEDERATE_RESIGNED_DURING_RESTORE`
notifications for still-present routed participants, and clears the active
Restore operation. The [membership flow in the Save/Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#member-exit-during-an-active-restore)
now compares this path across adapters.

The embedded adapter submits those Restore notifications, and the existing
focused embedded case observes the survivor's failure callback while proving
the departing member receives no Restore callbacks. In the configured-process
path, `dispatchConnectionLossResult` receives the same registry result but
does not enqueue its `restoreNotifications` vector. Normal process Restore
services use the event path, but no focused process case covers Resign or
ConnectionLost during an active Restore. This is a source-level dispatch gap,
not a runtime observation or a claim about the failure reason IEEE requires.

The audit also records active Join as unresolved. The 2025 API lists
`RestoreInProgress`, but the inspected adapter, process handler, and registry
Join paths show no active-Restore phase check. If a registry Join commits, it
does not update the already captured Restore ledger; successful process-local
restore later replaces the live member map from the saved snapshot. No focused
case establishes public Join admission or its effects during active Restore,
so the guide labels this as a conditional source observation rather than an
allowed behavior or a normative rule.

Added a flowchart for the admission-time participant set and the Join,
Resign/ConnectionLost branches. Mermaid CLI 12.0.0 rendered and screenshot-
reviewed it. The visual ledger now tracks 494 diagrams across 32 2025 topic
guides (289 flowcharts, 181 sequences, 24 state diagrams); the identity, body
hash, type, source-line, render, and screenshot records are refreshed. The
source-link checker resolves all 4,257 references against the worktree,
candidate base, and primary source revision. Overall progress remains **80%**;
formal gates remain **3/4 (75%)**. No C++ tests or builds, 2010 changes, or
Requirements Lab edits were made.

**Next handoff:** Audit the 2025 active Save participant set during Resign and
ConnectionLost. Compare the shared registry's Save failure construction with
embedded and process notification projection, then identify which departure
interleavings have focused test evidence. Keep Save and Restore conclusions
separate, preserve the 2010 boundary, and continue the broader semantic source
review.

## 2025 active Save member-exit audit, increment 297 — 2026-11-08

Traced the active Save participant ledger and member-exit path. The registry's
completion builder skips `NO_SAVE_IN_PROGRESS` entries, excludes the departing
member, and requires a callback route for each remaining notification
recipient. The common transition currently supplies
`FEDERATE_RESIGNED_DURING_SAVE` for both voluntary Resign and forced
ConnectionLost, although the pinned 2025 API also lists
`RTI_DETECTED_FAILURE_DURING_SAVE`. The guide now treats that as a source
finding and makes no claim about the standard's required reason for detected
connection loss.

The embedded adapter submits the returned Save notifications. Existing public
tests cover an outstanding Save followed by Resign and Resign after an
Initiate Federate Save callback, including suppression of the departing
member's failure callback and successful later Save recovery. No focused test
covers ConnectionLost during active Save. The configured-process Resign and
ConnectionLost paths pass the registry result to `dispatchConnectionLossResult`,
but the inspected helper does not enqueue its `saveNotifications` vector; no
focused process member-exit case was found. The guide labels this a
source-level projection gap rather than a tested runtime outcome. See the
[Save member-exit flow](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#member-exit-during-an-active-save).

Added and locally rendered the Save membership-loss flowchart with Mermaid CLI
12.0.0 and reviewed its screenshot. The ledger now matches 495/495 diagrams
across 32 2025 guides (290 flowcharts, 181 sequences, 24 state diagrams).
Authenticated GitHub GFM passed all 34 scoped pages and all diagram containers;
the current source-link checker resolves every reference against the worktree,
candidate base, and primary source revision. Overall progress remains **80%**;
formal gates remain **3/4 (75%)**. No C++ tests or builds, 2010 changes, or
Requirements Lab edits were made.

**Next handoff:** Verify the exact IEEE 1516.1-2025 text governing the
`federationNotSaved` outcome after a participant Resign or RTI-detected
connection loss. Keep the API's enum list, Umbra's current source mapping, and
the normative requirement separate; leave the standard-specific conclusion
unresolved if authoritative text is unavailable.

## 2025 normative Save-failure source boundary and ledger repair, increment 298 — 2026-11-08

Checked the open Save-failure handoff against IEEE's official product record and
the pinned API package. The official record offers purchase or subscription
access to IEEE 1516.1-2025; Umbra's vendored 2025 package documents the
unmodified C++ API header tree, not the normative book text. The `SaveFailureReason`
enum therefore establishes which values the API names, but not which failure
reason the standard requires after Resign or detected connection loss. That
standard-specific question remains unresolved; no 2010 wording is used as a
substitute.

A complete visual-ledger integrity pass found one stale body hash for diagram
018 in Callback and Service Ordering. Mermaid CLI 12.0.0 rendered the current
sequence; screenshot review found its branches readable and unclipped. The
ledger hash is refreshed. The audit now matches all 495 Mermaid fences across
32 guides to their identities, source lines, types, current body hashes, local
renders, and screenshot reviews. The prior authenticated GFM pass after
increment 297 covered all 34 scoped pages and 495 Mermaid containers; this
increment adds no diagram and records the newly checked IEEE source-access
boundary.

Overall progress remains **80%** and formal gates remain **3/4 (75%)**. The
remaining work is the broader semantic source-citation review and published
GitHub file-page inspection. No C++ tests/builds, 2010 changes, or Requirements
Lab edits were made.

**Next handoff:** Audit the source-derived embedded `Send Interaction`
callback-exception sequence in the Callback and Service Ordering guide. Trace
accepted effects, callback-task consumption, recipient fan-out, and the
`HLA_IMMEDIATE` versus `HLA_EVOKED` paths against the current source; keep
untested callback behavior separate from IEEE requirements.

## 2025 embedded Send Interaction callback-exception audit, increment 299 — 2026-11-08

Traced the embedded receive-order route from the accepted `Send Interaction`
through callback submission and dispatch. The selected successful service
report and accepted-send counter are recorded before route fan-out. Immediate
submission either drains on the sender thread (including same-thread
re-entry) or appends behind another thread's active drainer; an escaping
callback consumes its extracted task, while already queued FIFO work remains.
Evoked submission returns with work queued; a throwing callback escapes the
later Evoke call after its task was removed, leaving subsequent queued work for
later dispatch. The sequential fan-out loop can stop at a synchronous throw;
when another thread owns that recipient's drainer, the sender can continue
submitting routes.

Updated the chart to distinguish a newly owned or re-entered immediate drainer
and to mark successful service-report writing as conditional on selection. The
source and callback API do not establish this as a normative exception rule,
and no focused throwing-callback integration case covers the accepted-state,
report-sink, queue, or fan-out outcomes. Rendered the revised diagram locally
and inspected the screenshot; no clipping was visible. Refreshed its ledger
body hash. A complete follow-up hash comparison found a second stale entry in
TSO Retraction diagram 008. Rendered and screenshot-reviewed that current flow,
then refreshed its ledger hash and rechecked all 495 diagram identities,
source lines, types, and body hashes. No C++ tests or builds, 2010 changes, or
Requirements Lab edits were made.

Overall progress remains **80%**; formal gates remain **3/4 (75%)**. The next
remaining gate is publication of the 41-path docs-only selection followed by
visible inspection of all 32 GitHub pages. The exact normative Save-failure
reason remains open pending the licensed IEEE text.

**Next handoff:** After explicit publication authorization, publish only the
41 documentation paths and inspect all 32 GitHub file pages for links and
visible Mermaid rendering. Record the published revision and per-page results;
keep the Save-failure mapping unresolved until the authoritative 2025 clause
is available.

## 2025 ownership callback and grant evidence audit, increment 301 — 2026-11-08

Rechecked the ownership guide's statement that no focused cross-service test
documents acquisition callbacks relative to a grant. The committed embedded
accepted-TSO ownership-transfer case provides a bounded staged example: it
observes the ownership-assumption and acquisition-notification callbacks before
either federate issues `timeAdvanceRequest`, then observes both grant callbacks
and the retained timestamped reflection. The test records callback labels but
does not assert the callback-order vector, and it does not establish precedence
for ownership work and grant work becoming eligible in one callback drain or
for the configured-process profile.

Updated the ownership guide with this precise evidence and limit. Also removed
stale “current worktree edit” wording from the TSO guide because the cited test
is committed and clean in this worktree; it remains accurately labeled as not
run during this documentation pass. Prose-only changes: no Mermaid bodies or
visual-ledger hashes changed. No C++ tests/builds, 2010 changes, or Requirements
Lab edits were made. Overall progress remains **80%**; formal gates remain
**3/4 (75%)**.

**Next handoff:** After explicit publication authorization, publish only the
41 documentation paths and inspect all 32 GitHub file pages for links and
visible Mermaid rendering. Record the published revision and per-page results;
continue the source-citation audit if publication is not yet authorized, and
keep the exact Save-failure mapping unresolved until authoritative 2025 text is
available.

## 2025 configured-process ownership and grant boundary audit, increment 302 — 2026-11-08

Traced the configured-process ownership and grant paths separately. The
ownership service retains assumption/acquisition work in its per-session
ownership-event queue for pull delivery or sends ownership callback events as
`receive_interaction` frames. The time-grant factory uses a distinct
`time_advance_grant` event. The current focused public process test verifies an
evoked ownership-assumption callback; a separate private process-service test
receives a timestamped reflection before its grant event. Neither combines
ownership notification and grant callbacks. A search across the C++ test tree
found combined ownership/time callback cases only in embedded tests.

Added this configured-process evidence boundary to the ownership guide. Separate
server event routes and separate tests do not establish a relative callback
order, so the guide makes no process-order claim. No diagram changed; no Mermaid
render was needed. No C++ tests/builds, 2010 changes, or Requirements Lab edits
were made. Overall progress remains **80%**; formal gates remain **3/4 (75%)**.

**Next handoff:** Audit the 2025 Process Endpoint guide's client callback queue
and receive-poll fencing against the candidate's committed sources, then inspect
the dirty worktree callback-dispatch changes as a separate implementation
snapshot. Keep worktree-only behavior out of claims tied to the published
candidate unless that source is committed into the candidate.

## 2025 process polling-family priority audit, increment 300 — 2026-11-08

Rechecked the Process Endpoint guide's distinction between polling queues and
pushed event frames. The process service's receive selector checks
synchronization announcement, federation synchronized, Save, Restore, and
interaction queues in a fixed first-match order. Every poll restarts that
selector; if an earlier queue remains nonempty, later-family events can be
postponed across polls. The selector has no round-robin step. Pushed frames
bypass that selector and enter the client's FIFO in stream-read order; the HLA
callback model remains a separate inline-versus-Evoke choice.

The public synchronization tests cover polling and pushed paths separately.
They do not combine event families or measure starvation, so the new note in
the guide labels this a process-source consequence, not an HLA ordering or
fairness requirement. The relevant configured-endpoint path is attached to the
2025 ambassador; no 2010 source or requirements were used. This was a prose-only
update, so no diagram render or ledger change was needed. No C++ tests/builds,
2010 changes, or Requirements Lab edits were made.

Overall progress remains **80%**; formal gates remain **3/4 (75%)**. The broader
semantic source-citation review, licensed 2025 Save-failure mapping, and
published-page verification remain open.

**Next handoff:** After explicit publication authorization, publish only the
41 documentation paths and inspect all 32 GitHub file pages for links and
visible Mermaid rendering. Record the published revision and per-page results;
keep the Save-failure mapping unresolved until the authoritative 2025 clause
is available.

## 2025 Process Endpoint callback queue and receive-poll audit, increment 303 — 2026-11-08

Compared the Process Endpoint guide with the candidate's committed sources. The client validates and buffers pushed frames while a correlated request is active. Once the operation and request ID match, `request()` drops `requestDepth_`, releases the exchange lock, and drains the buffered FIFO. The background dispatch loop admits pending events only when both `requestDepth_` and `callbackDispatchSuppressionDepth_` are zero; the dispatcher rechecks callback suppression between events. Each event leaves the client FIFO before bridge submission.

The public process Evoke pump drains pending callback work before polling with `receive_interaction`. A pulled interaction is returned separately for receive-order dispatch; other typed results can be buffered in the client queue. The candidate-tied guide already reflects these distinctions, so no guide edit was warranted.

Reviewed the dirty callback-dispatch snapshot separately. It increments `dataCallbackAdmissionDepth_` around pushed-event submissions, except request-retraction events, and defines `isPushedDataCallbackAdmissionInProgress()`. A whole-repository search found neither member declared in the client header nor any callsite that reads the counter or invokes the getter; the public callback-control poll path has no matching fence. Treat this as incomplete, uncommitted worktree code and do not describe it as candidate behavior.

No guide or diagram changed. No C++ tests/builds, 2010 changes, or Requirements Lab edits were made. Overall progress remains **80%**; formal gates remain **3/4 (75%)**. Published GitHub hydration and the licensed 2025 Save-failure mapping remain open.

**Next handoff:** After explicit publication authorization, publish only the 41 selected documentation paths and inspect all 32 GitHub file pages for link destinations and visible Mermaid hydration. Record the published revision and per-page results. If publication remains pending, audit the Callback and Service Ordering guide's process Evoke Multiple admission loop and its focused test evidence against candidate HEAD.

## 2025 process Evoke Multiple source and test review, increment 304 — 2026-11-08

Compared the Callback and Service Ordering guide with committed 2025 source. At entry, `evokeMultipleCallbacks` skips its process receive pump if the shared dispatcher already has pending work. Otherwise it passes callback-enabled state to `pumpProcessReceiveOrder`, which drains buffered process work and polls while each poll yields work when enabled. When callbacks are disabled, it stops after one work-yielding poll. The adapter then invokes the shared `evokeMultiple` dispatcher once. The post-wait FIFO drain and zero-wait retry belong to single `evokeCallback`, not `evokeMultipleCallbacks`.

The pump checks shared-dispatcher pending count before polling, but the client removes a pushed event from its FIFO before submitting it to the callback bridge. There is no admission-depth fence in committed candidate source, so a concurrent handoff can cross the queue-count observation. The focused process receive/Evoke case verifies one disabled admission and later delivery of its queued callback without another poll; it does not force the concurrent handoff or verify the enabled multi-poll drain. The generic dispatcher wait test does not cover the process reader or client FIFO. Corrected the process Evoke Multiple prose and diagram to distinguish these paths and evidence limits.

During this review, the current Callback and Service Ordering diagram 018 and TSO Retraction diagram 008 were also found to have body hashes that differed from their ledger entries. Rendered and screenshot-reviewed both current bodies, along with the revised diagram 007. Refreshed diagram source-line metadata across all 495 entries; a full identity, line, and body-hash comparison now matches.

No C++ tests/builds, 2010 changes, or Requirements Lab edits were made. Overall progress remains **80%**; formal gates remain **3/4 (75%)**. Published GitHub hydration and the licensed 2025 Save-failure mapping remain open.

**Next handoff:** Audit the 2025 single-Evoke process post-wait drain and zero-wait follow-up in the Callback and Service Ordering guide against candidate HEAD and the focused public process receive/Evoke case. Keep that path distinct from Evoke Multiple and state its test limits. After explicit publication authorization, publish only the 41 selected documentation paths and inspect all 32 GitHub file pages for link destinations and visible Mermaid hydration; record the published revision and per-page results.

## 2025 single-Evoke process push and post-wait review, increment 305 — 2026-11-08

Compared the Callback and Service Ordering guide with committed 2025 source.
The single `evokeCallback` path may admit one process poll before its initial
`evokeOne` when no dispatcher work is pending. During the wait, the stream
reader can buffer a pushed interaction and the independent client dispatch
loop can submit it to the bridge. If the dispatcher invokes a task, the
post-wait branch is skipped; whether the task enters `FederateAmbassador`
depends on the receive-order temporal gate. The `callbackInvoked` output
records dispatcher-task invocation, including a bridge task that defers the
event without calling the application.

The post-wait branch runs only when the initial `evokeOne` returns false and
invokes no dispatcher task. The adapter then attempts to dispatch client-FIFO
events and, if dispatcher work is pending, calls `evokeOne(0ms)` once. Disabled
callbacks remain queued. The client dispatch call can return early when a
different thread owns the dispatch lease; this does not fence the FIFO-removal
to bridge-submission handoff. This path is distinct from Evoke Multiple's
pre-wait process-admission path.

The focused HLA_EVOKED asynchronous-delivery test uses zero-duration Evokes and
checks the receive-order gate. The public process receive/Evoke test sends
before a zero-duration Evoke and exercises polling. The generic dispatcher
test queues tasks before Evoke Multiple. None covers a pushed ordinary
interaction arriving during a positive single-Evoke wait or the concurrent
FIFO-to-bridge handoff. Corrected two stale bridge line links and added the
dispatcher-flag source citation; all 14 links in the reviewed section resolve.
Corrected diagram 005, rendered it with Mermaid CLI 12.0.0, and screenshot-
reviewed it. Refreshed the 24 Callback guide ledger entries and found 105 stale
heading breadcrumbs across the 2025-only ledger. Updated those labels across
all 32 guides. The full 495-entry identity, heading, type, source-line, and
body-hash check now passes. The latest worktree source-line check resolves
4,283/4,283 references across 35 Markdown files.

No C++ tests/builds, 2010 changes, or Requirements Lab edits were made.
Overall progress remains **80%**; formal gates remain **3/4 (75%)**. Broader
semantic source-citation review, published GitHub page hydration, and the
licensed 2025 Save-failure mapping remain open.

**Next handoff:** Continue the bounded 2025 semantic source-citation review
against the open items in the active roadmap. Keep the exact Save-failure
reason mapping unresolved until licensed 2025 normative text is available.
After explicit publication authorization, publish only the 41 selected
documentation paths and inspect all 32 GitHub file pages for link destinations
and visible Mermaid hydration; record the published revision and page results.

## 2025 configured-process AttributeUpdate queues and callback gates, increment 306 — 2026-11-08

Compared the Callback and Service Ordering guide with committed 2025 client,
bridge, callback-dispatcher, service, and focused-test paths. The stream reader
and typed non-interaction poll results append to the client FIFO without
checking callback enablement. Background dispatch waits for request depth and
callback-dispatch suppression to clear; a callback-owning request has a
request-owned drain that can pass through its own suppression gate. A
competing client dispatch owner can return before a FIFO item is removed. Full
AttributeUpdate events carry the captured reflection; a deferred marker queues
a task that later claims the authoritative event and rechecks eligibility.

Separated that client and dispatcher flow from application callback entry.
The ordinary AttributeUpdate bridge path does not consult the configured-
process receive-order interaction time gate. The receive-order interaction
path does. The focused immediate delayed-AttributeUpdate case uses Push and
checks eventual callback outcome after disable/unsubscribe/update/resubscribe/
enable. Its HLA_EVOKED counterpart uses Pull and does not establish that the
event was already queued before resubscription, call `disableCallbacks`, or
exercise a pushed deferred marker. The evoked TSO case proves disabled work
remains pending until callbacks are enabled and a later Evoke, but uses Pull
and no deferred marker. No focused public test directly inspects either
private queue or combines ordinary AttributeUpdate with a time-constrained
receiver, asynchronous-delivery changes, and a pending time advance.

Corrected stale source and test line links; all 30 local line citations in the
reviewed section resolve against the committed HEAD and candidate base. Revised
Callback and Service Ordering diagram 006, rendered it with Mermaid CLI 12.0.0,
and screenshot-reviewed it. The full 495-entry identity, heading, type, source
line, and body-hash audit passes. The worktree source-line check resolves
4,292/4,292 references across 35 Markdown files.

No C++ tests/builds, 2010 changes, or Requirements Lab edits were made. Overall
progress remains **80%**; formal gates remain **3/4 (75%)**. Broader semantic
source-citation review, published GitHub page hydration, and the licensed 2025
Save-failure mapping remain open.

**Next handoff:** Audit “The bounded drain uses elapsed time, not HLA logical
time” in the Callback and Service Ordering guide against the committed 2025
adapter and dispatcher, pinned 2025 API/service text, and focused public tests.
Trace the minimum and maximum wait paths, empty and nonempty queue behavior,
and callback-enable gates; state what the tests do not cover. Continue 2025-only.
After explicit publication authorization, publish only the 41 selected
documentation paths and inspect all 32 GitHub file pages for link destinations
and visible Mermaid hydration; record the published revision and page results.
Keep the exact Save-failure reason mapping unresolved until authoritative
licensed 2025 normative text is available.

## 2025 Evoke Multiple elapsed-time review, increment 307 — 2026-11-08

Audited “The bounded drain uses elapsed time, not HLA logical time” against
the committed 2025 API declaration, adapter, dispatcher, and focused callback
tests. The adapter maps non-finite and nonpositive seconds to zero, truncates
positive fractional milliseconds, and saturates very large finite values. The
dispatcher normalizes the maximum to at least the minimum, acquires its shared
dispatch lock before starting the `steady_clock` window, and returns before
timing when it is disabled or not in evoked mode. An empty queue waits only to
the minimum deadline. Queued callbacks run FIFO; the maximum is checked only
after each callback returns, so lock waiting and callback execution can extend
the observed API-call duration.

Corrected the test-evidence note: the focused generic `evokeMultiple` case
starts with three callbacks queued and checks FIFO completion without asserting
elapsed time. The line previously described as an empty-queue wait case is
actually a disabled `evokeOne` test. No focused test here covers timed arrival,
argument conversion, wait normalization, a maximum overshoot, dispatch-lock
delay, or the public API adapter. Revised diagram 008, rendered it with Mermaid
CLI 12.0.0, and screenshot-reviewed it. Also re-rendered and reviewed diagram
018 after detecting its stale body hash. Refreshed the Callback guide ledger
lines for diagrams 008–024; all 495 diagram identities, headings, types,
fences, lines, and hashes now match. The worktree documentation link check
resolves 4,293/4,293 references across 35 Markdown files. IEEE's product page
requires purchase or subscription access, so normative interpretation of
§10.58 remains unresolved pending licensed text.

No C++ tests/builds, 2010 changes, or Requirements Lab edits were made. Overall
progress remains **80%**; formal gates remain **3/4 (75%)**. The broader
semantic source-citation review, published GitHub page hydration, and licensed
2025 Save-failure mapping remain open.

**Next handoff:** Audit “HLA_IMMEDIATE: callback work can be on the service
stack” in the Callback and Service Ordering guide against committed 2025
adapter, dispatcher, service paths, the pinned API, and focused tests. Trace
when callback submission enters user code on the service stack versus queues
behind an active drainer, and state which service side effects and exception
outcomes the tests actually observe. Keep implementation observations separate
from normative claims. Continue 2025-only. After explicit publication
authorization, publish only the 41 selected documentation paths and inspect
all 32 GitHub file pages for link destinations and visible Mermaid hydration;
record the published revision and page results. Keep the exact Save-failure
reason mapping unresolved until authoritative licensed 2025 normative text is
available.

## 2025 HLA_IMMEDIATE drainer and producer boundaries, increment 308 — 2026-11-08

Audited “HLA_IMMEDIATE: callback work can be on the service stack” against the
pinned 2025 callback-model enum, callback dispatcher/session, the
`listFederationExecutions` adapter, and focused tests. The sequence now separates
an enabled submitter that becomes or re-enters the immediate drainer from a
competing producer that appends to FIFO and returns before its callback enters.
The existing drainer invokes that queued task after its active callback returns
normally and delivery remains enabled. A callback exception consumes the active
task, preserves later FIFO work, and unwinds through the call that owns the
drain; that owner may differ from the RTI service that submitted the task.

The direct dispatcher test checks same-thread synchronous delivery. Its
concurrent producer case holds the first callback open, joins a second producer
after submission, verifies that the second callback has not entered, then
checks FIFO order after release. The public `listFederationExecutions` case
exercises one uncontended immediate callback and verifies that both Evoke
services reject callback re-entry. No public test covers a competing drainer
through an RTI service, an allowed nested service that creates another public
callback, or exception attribution in that concurrent schedule. Revised diagram
009, rendered it with Mermaid CLI 12.0.0, and screenshot-reviewed it. Refreshed
Callback guide ledger lines for diagrams 009–024; all 495 diagram identities,
headings, types, fences, lines, and hashes match. The worktree source-line check
resolves 4,298/4,298 references across 35 Markdown files, and new cited lines
resolve against the candidate base and source revision. The licensed standard
text is still needed for normative callback-thread and exception semantics.

No C++ tests/builds, 2010 changes, or Requirements Lab edits were made. Overall
progress remains **80%**; formal gates remain **3/4 (75%)**. The broader
semantic source-citation review, published GitHub page hydration, and licensed
2025 Save-failure mapping remain open.

**Next handoff:** Audit “Callback-session lifetime and concurrency” in the
Callback and Service Ordering guide against committed 2025 CallbackSession,
dispatcher, Disconnect, and focused-test paths. Trace the borrowed
FederateAmbassador lifetime, close from an active callback versus an external
thread, stale queued tasks, and the public Disconnect re-entry guard. Separate
private session cleanup from HLA service behavior, and state which races the
tests do not cover. Continue 2025-only. After explicit publication
authorization, publish only the 41 selected documentation paths and inspect
all 32 GitHub file pages for link destinations and visible Mermaid hydration;
record the published revision and page results. Keep the exact Save-failure
reason mapping unresolved until authoritative licensed 2025 normative text is
available.

## 2025 Callback-session close and public Disconnect boundary, increment 309 — 2026-11-08

Reviewed `CallbackSession`, the shared dispatcher, public `Disconnect`, and
focused 2025 tests. The session borrows the caller-owned
`FederateAmbassador`. It serializes entries through a recursive gate, counts
accepted invocations, rejects a waiting invocation if close wins, and detaches
the borrowed pointer after the final accepted invocation exits. Normal return
and exception both decrement the count; the exception is rethrown after
cleanup. Private self-close marks the session closed and returns to its active
callback. External close waits for accepted entries, but not every task waiting
on the gate or the full dispatcher/service stack.

Added a state diagram for those session transitions and a sequence diagram that
separates Umbra's public `Disconnect` callback guard from private session
self-close. The public guard runs before lifecycle transition. A successful
external Disconnect resets queued tasks under the ambassador lock and closes
the session after releasing that lock. The configured-process path then
continues client teardown.

The audit found public embedded evidence the guide had missed:
`runFederationListingScenario` queues an evoked federation-list report,
disconnects, then verifies a later Evoke does not deliver it. Separate unit
cases exercise external close waiting on a blocked callback, direct invocation
serialization, and private self-close. No focused test asserts public
Disconnect-from-callback rejection or external public Disconnect waiting on a
callback running on another thread. The guide also records that configured-
process pending callbacks and concurrent Connect during Disconnect cleanup
remain untested; the shared-dispatcher reset race is source-derived only.

Both changed diagrams were rendered with Mermaid CLI 12.0.0 and screenshot-
reviewed. All 496 diagram records match identities, headings, types, fences,
source lines, and body hashes. The worktree link check resolves 4,307/4,307
references across 35 Markdown files. One citation to
`process_federation_client.cpp#L1162` is worktree-only and does not resolve at
the candidate base or primary source revision. Authenticated GFM returned all 35
scoped pages with 496/496 Mermaid containers, 966 headings, 7,567 links, and
zero failures; published-page hydration remains unverified. No C++ tests/builds,
2010 changes, or Requirements Lab edits were made. Overall progress remains
**80%** and formal gates **3/4 (75%)**.

**Next handoff:** After the in-progress process-client recovery edits settle,
audit the configured-process ConnectionLost and reconnect path against the final
2025 source and focused tests. Trace the Not Connected transition, retained old
client, replacement cleanup, and possible shared-dispatcher reset. Keep
source-derived races separate from tested outcomes. Continue docs-only and
2025-only; do not rebuild C++ for this review.

## 2025 embedded exception-report partial fan-out review, increment 310 — 2026-11-08

Audited the embedded `Send Interaction` catch, `emitMomExceptionReport`,
`emitExceptionReport`, both per-recipient submission loops, the callback
dispatcher, and the focused MOM-report cases. Both report hooks are `noexcept`
and contain failures around the complete report operation. Under
`HLA_IMMEDIATE`, submitting a recipient can synchronously invoke its
`receiveInteraction` callback. If it throws, the dispatcher removes the active
task and rethrows; the report hook catches it, so the rest of that report’s
recipient loop is not submitted. Callbacks that already ran are not rolled
back. The outer `Send Interaction` catch continues to the other report hook and
then the failed-invocation writer. Under `HLA_EVOKED`, all report tasks can be
queued before user callback code runs; a later callback exception can surface
from Evoke. The existing tests use non-throwing observers and do not test this
partial-fan-out case. This is source-derived behavior, not a normative ordering
claim.

Revised Exception Reporting diagram 007, rendered it with Mermaid CLI 12.0.0,
and screenshot-reviewed it. The full body audit also found stale ledger hashes
for Synchronization diagram 017 and FOM Composition diagram 003; both were
rendered and screenshot-reviewed, and the ledger entries were corrected. All
496 diagram records now match their current identities, headings, types, fences,
source lines, and body hashes. The worktree source-line check resolves
4,312/4,312 references across 35 Markdown files. The candidate base and primary
source revision each resolve 4,311/4,312 because the known
`process_federation_client.cpp#L1162` citation points into an uncommitted source
edit. Authenticated GFM returned 35/35 pages with 496/496 Mermaid containers,
966 headings, 7,572 links, and zero failures; published-page hydration remains
unverified. No C++ tests/builds, 2010 changes, or Requirements Lab edits were
made. Overall progress remains **80%** and formal gates remain **3/4 (75%)**.

**Next handoff:** After the in-progress process-client recovery edits settle,
audit the configured-process ConnectionLost and reconnect path against the final
2025 source and focused tests. Trace the Not Connected transition, retained old
client, replacement cleanup, and possible shared-dispatcher reset. Keep
source-derived races separate from tested outcomes. Continue docs-only and
2025-only; do not rebuild C++. Then continue semantic citation review. After
explicit publication authorization, publish only the 41 selected documentation
paths and inspect all 32 GitHub file pages for link destinations and visible
Mermaid hydration; record the published revision and per-page results. Keep the
exact Save-failure reason mapping unresolved until licensed 2025 text is
available.

## 2025 typed-handle byte-layout correction, increment 311 — 2026-11-08

Corrected the Names and Handle Resolution guide against the current 2025
encoder, decoder, typed handle definitions, and focused tests. The encoded
`HLAvariableArray` is twelve bytes: a big-endian count of eight, then the
eight-byte big-endian identity. The bytes have no category discriminator. The
embedded public decoder performs connection and membership checks before the
typed decoder checks non-null data, exact length, and count; the reviewed byte
decoder does not establish that a value belongs to the requested handle kind.

The focused object-class handle test asserts the 12-byte vector and a matching-
type round trip. The public federation-management case covers matching-kind
decode and lifecycle preconditions, not cross-kind bytes. A cross-kind
`CouldNotDecode` expectation exists in an untracked TCK helper with no caller;
it is not passing-test evidence and remains unresolved. The 2010 adapter also
calls the private count-and-identity helper, but this is narrow byte-codec reuse:
public handle classes, exceptions, and RTI behavior remain edition-specific.

Rendered and screenshot-reviewed Names and Handle Resolution diagram 006; its
ledger line and body hash were refreshed. The full ledger was checked for
current identities, headings, types, fences, source lines, and body hashes. The
worktree source-line check resolves 4,312/4,312 references across 35 Markdown
files. Authenticated GFM returned 35/35 pages with 496/496 Mermaid containers,
967 headings, 7,574 links, and zero failures; published-page hydration remains
unverified. No C++ tests/builds, 2010 changes, or Requirements Lab edits were
made. Overall progress remains **80%** and formal gates remain **3/4 (75%)**.

**Next handoff:** After the in-progress process-client recovery edits settle,
audit the configured-process ConnectionLost and reconnect path against the final
2025 source and focused tests. Trace the Not Connected transition, retained old
client, replacement cleanup, and possible shared-dispatcher reset. Keep
source-derived races separate from tested outcomes. Continue docs-only and
2025-only; do not rebuild C++. Then continue semantic citation review. After
explicit publication authorization, publish only the 41 selected documentation
paths and inspect all 32 GitHub file pages for link destinations and visible
Mermaid hydration; record the published revision and per-page results. Keep the
exact Save-failure reason mapping unresolved until licensed 2025 text is
available.


## 2025 configured-process ConnectionLost cleanup and concurrent Connect review, increment 312 — 2026-11-08

Reviewed the configured-process recovery path after its worktree changes settled. A temporary socket failure can resume the retained private client session while the public ambassador remains connected. Terminal loss instead marks the local ambassador Not Connected and retains the old client and callback path for later cleanup. A subsequent public Connect can replace that client while holding the ambassador mutex; closing the old client joins its workers, then closes the bridge and waits for active callback-session entries before resetting the shared dispatcher.

This source permits a possible wait cycle if an active old-session callback re-enters an RTI service needing the ambassador mutex while another application thread is in Connect cleanup holding that mutex. It is not an observed deadlock or an HLA guarantee. The callback-thread Connect guard is thread-local, so it does not reject a different thread's Connect. The configured-process coordinator-stop test uses HLA_EVOKED but does not race Connect or re-enter an RTI service; the private socket-resume test resumes before terminal loss; public Connect-after-loss evidence is embedded-profile only. No focused test establishes this interleaving.

Added Callback and Service Ordering diagram 021 to show the wait cycle and its HLA_IMMEDIATE worker-join versus HLA_EVOKED callback-session branches. Rendered and screenshot-reviewed it. The review also found that diagram 019's body had changed since its recorded review; it was rendered again, screenshot-reviewed, and its body hash refreshed. All 497 current Mermaid bodies now match their ledger identities, types, source lines, and hashes. The worktree source-line check resolves 4,319/4,319 references across 35 Markdown files; the candidate base resolves 4,316/4,319 and the primary source revision 4,317/4,319 because three anchors point into uncommitted source/test edits. Authenticated GFM returned 35/35 pages, 497/497 Mermaid containers, 969 headings, 7,589 links, and zero failures. No C++ tests/builds, CI, 2010 changes, or Requirements Lab edits were made. Overall progress remains **80%** and formal gates remain **3/4 (75%)**.

**Next handoff:** Compare terminal process member loss across the Process Endpoint, Lifecycle, Federation MOM, and Joined-Federate MOM guides. Keep the local failed-member callback, forced registry cleanup, and surviving members' MOM/Remove notifications as distinct effects. Label source links into uncommitted process changes as worktree-only and avoid claiming tested behavior for an untested interleaving. Before docs-only publication, make the three source/test anchors resolve at the target source revision by including those source changes or updating the links to stable published lines; rerun source-link validation. Then continue the semantic citation review. The final formal gate is publication of the 41 selected documentation paths followed by inspection of all 32 GitHub file pages and Mermaid hydration after explicit publication authorization; until then, published-page rendering remains unverified. Keep the exact Save-failure reason mapping and full normative callback-order interpretation unresolved until licensed 2025 text is available.

## 2025 process member-loss cross-guide reconciliation, increment 313 — 2026-11-08

Compared configured-process terminal member loss across the Process Endpoint, Lifecycle, Federation MOM, Joined-Federate MOM, and Federation Save/Restore guides. The current `ProcessFederationService::dispatchConnectionLossResult` handles ownership work, object removals, synchronization notifications, and an optional Federate-Lost report, but it does not consume either registry `saveNotifications` or `restoreNotifications`. Both process Resign and server-side ConnectionLost call that dispatcher. The ordinary Save/Restore service queues do not consume these member-exit vectors. This is a source-level projection gap; there is no focused configured-process member-exit-during-Save/Restore test here and no normative compliance conclusion.

Corrected the stale Save/Restore claims in the Process Endpoint and Lifecycle guides. Reworked Lifecycle diagram 025 so the lost member's local `connectionLost` callback and host-side forced registry cleanup run in parallel; the survivor's object-removal projection and optional `HLAreportFederateLost` interaction remain separate. Added direct links to the Federation MOM report-selection guide and Joined-Federate MOM observer-route guide. Updated Lifecycle diagram 030 to show that neither Save nor Restore vector is consumed by the configured-process dispatcher. The MOM guides' report-recipient selection, `HLAfederate` observer ledger, and unavailable-route behavior were checked against those claims; the process path still has no focused assertion for forced-loss HLAfederate removal or callback order.

Mermaid CLI 12.0.0 rendered and screenshot review passed for Lifecycle diagrams 025 and 030. During the ledger check, Callback and Service Ordering diagram 019 was also found to have an unrecorded body change; it rendered and screenshot review passed. Refreshed 32 diagram source-line fields and three body hashes across the changed guides. All 497 diagram entries now match current identity, heading, type, fence, line, and body hash. The worktree source-line check resolves 4,312/4,312 references; 4,309 resolve at the pinned candidate base and 4,310 at the primary source revision. The last authenticated GFM check predates this increment, so it is not evidence for these uncommitted edits. No C++ tests/builds, source changes, CI, 2010 changes, or Requirements Lab edits were made. Overall progress remains **80%**; formal gates remain **3/4 (75%)**.

**Next handoff:** Resolve the three Callback guide source-line citations that depend on uncommitted process-client or process-test changes (`process_federation_client.cpp#L1162`, `process_federation_client_callback_dispatch.cpp#L672`, and `ieee1516_2025_connection_process_federation_service_host_catch2.cpp#L1081`). For each, find stable published support or narrow the claim while preserving its worktree-only status; rerun the source-link checker against the worktree, candidate base, and primary revision. Then continue the 2025 semantic citation review. The docs-only GitHub publication and inspection of all 32 topic pages remain the final gate after explicit publication authorization.


## 2025 Callback citation stabilization and GitHub GFM refresh, increment 314 — 2026-11-08

Replaced the two Callback and Service Ordering guide anchors into uncommitted
process-client implementation lines with stable pinned source citations. Kept
the private socket-resume test as prose-only evidence: it checks that the
private client can list its joined member after socket reconnection, not a
public ambassador's Not Connected transition. The broader connection-state
claim remains bounded by that distinction.

The source-line checker now resolves all 4,309 unique relative path/line/guide
references across 35 Markdown files in the candidate worktree, at the pinned
candidate base, and at the primary source revision. These correspond to 5,590
Markdown link occurrences. The immutable source-permalink audit resolves 148
occurrences (102 unique path/line pairs across 40 targets), with no missing
lines. Six source-line fields in the diagram ledger were reconciled after the
prose edits. All 497 current diagram identities, headings, types, fences, lines,
and body hashes match; no Mermaid body changed, so no render refresh was
needed.

The authenticated full GitHub GFM pass after increment 312 plus an
unauthenticated increment-314 refresh of the six pages changed in increments
313–314 cover all 35 scoped Markdown pages and all 497 diagrams. The six
refreshed pages returned HTTP 200 with non-empty HTML; their 113 Mermaid
containers matched 113 source fences, with 343 headings and 1,796 links. The
other 29 pages have no later content edits. This verifies GFM parsing and
container registration, not visible hydration on published GitHub file pages.

No C++ tests/builds, CI, 2010 changes, or Requirements Lab edits were made.
Overall progress remains **80%**; formal gates remain **3/4 (75%)**.

**Next handoff:** Continue the 2025 semantic citation review at the Federation
Save/Restore guide's failure and recovery boundaries. Cross-check portable API
declarations and available focused source/test evidence, and label
implementation-derived behavior separately from the standard. Keep the exact
Save failure-condition mapping unresolved until licensed 2025 clause text is
available. After explicit publication authorization, publish only the 41-path
documentation candidate and inspect all 32 topic pages for link destinations
and visible Mermaid hydration. Keep 2010 and Requirements Lab paths excluded.


## 2025 Save/Restore failure-boundary and reason-selector review, increment 315 — 2026-11-08

Reviewed the Save/Restore failure boundary against the pinned 2025 API enum,
registry selectors, and focused embedded and configured-process test cases.
The guide now distinguishes restore admission rejection—missing or undecodable
commit/image, failed admission identity checks, or incompatible current
membership—from a failure after a restore operation has been accepted. The
post-admission state-application path can select `RTI_UNABLE_TO_RESTORE`.

Added selector and evidence tables for all six `SaveFailureReason` values and
all five `RestoreFailureReason` values. The maps describe current Umbra source,
not normative triggers. No direct selector was found for
`RTI_DETECTED_FAILURE_DURING_RESTORE`, and no focused failure assertion was
found for `RTI_UNABLE_TO_RESTORE`; that enum also appears on a successful
internal completion notification, so its value must be read with the success
flag. The configured-process member-exit projection gap remains labeled as a
source finding without focused process-test evidence. Exact normative reason
conditions for Save and Restore remain open pending licensed 2025 text.

Updated 18 Save/Restore diagram line fields by their unchanged ordinals. All 35
diagram bodies retained their recorded hashes. The source-line checker resolves
4,331/4,331 unique references (5,613 Markdown link occurrences) at the worktree,
pinned candidate base, and primary source revision. The authenticated GitHub
GFM pass returned 35/35 pages and 497/497 Mermaid containers, with 974 headings,
7,608 links, and no failures. Published-page hydration remains unverified. No
C++ tests/builds, CI, 2010 changes, or Requirements Lab edits were made.

**Next handoff:** Audit Save/Restore callback delivery after accepted requests
across embedded and configured-process adapters. Trace request-success,
begin/initiate, and terminal notifications through `HLA_IMMEDIATE` and
`HLA_EVOKED`, then state exactly which delivery profiles the focused tests
cover. Keep normative ordering claims bounded by licensed 2025 text. After
explicit publication authorization, publish only the 41 documentation paths
and inspect all 32 GitHub topic pages for links and visible Mermaid hydration.


## 2025 Save/Restore callback-delivery gates and profile coverage, increment 316 — 2026-11-08

Added an at-a-glance explanation that separates the configured-process event
transport choice (push frames or poll the service queue) from the callback
model choice (`HLA_IMMEDIATE` or `HLA_EVOKED`). The embedded route enters the
same member dispatcher without the process transport gate. The guide also
states that Save has no successful request-admission callback, while Restore
does, and avoids deriving a normative cross-member order from Umbra's queueing.
The diagram states that it assumes callbacks are enabled and points readers to
the separate enable/disable callback gate.

The focused evidence now appears in a profile matrix. The embedded two-member
scenario runs successful Save and Restore under both callback models. Process
polling tests cover Save and successful Restore with `HLA_EVOKED`; process push
tests cover a successful Save/Restore lifecycle with `HLA_IMMEDIATE` and
selected queued Save/Restore callbacks with `HLA_EVOKED`. No focused public
accepted-request-to-terminal Save/Restore test combining polling with
`HLA_IMMEDIATE` was found. This is recorded as a test-coverage gap, not as an
unsupported implementation profile.

Added one flowchart to the Save/Restore guide, rendered it with Mermaid CLI
12.0.0, and screenshot-reviewed the result. Reconciled all 36 diagram entries
for that guide, including the new diagram; the full 2025 ledger now accounts
for 498 diagrams (291 flowcharts, 183 sequences, and 24 state diagrams). The
source-line checker resolves 4,332/4,332 unique references across the 35
scoped Markdown files (5,619 link occurrences) at the worktree, pinned
candidate base, and primary source revision. [The current authenticated GFM
pass](HLA-2025-GITHUB-RENDER-AUDIT.json) returned 35/35 pages with 498/498
Mermaid containers, 976 headings, 7,616 links, and zero failures. This verifies
GFM parsing and Mermaid container registration; published-page hydration
remains unverified.

No C++ tests/builds, CI, 2010 changes, Requirements Lab edits, or source changes
were made. Overall progress remains **80%**; formal gates remain **3/4 (75%)**.

**Next handoff:** Continue the 2025 semantic citation review by checking the
Restore callback sequence at the requester/member boundary: verify which
participant receives request-success, federation-begun, initiation, and the
terminal result in the embedded and configured-process evidence. Keep
per-member sequencing distinct from any federation-wide ordering claim. After
explicit publication authorization, publish only the 41 documentation paths
and inspect all 32 GitHub topic pages for links and visible Mermaid hydration.


## 2025 Restore callback recipients across requester and member lanes, increment 317 — 2026-11-08

Completed the requester/member recipient review in the Save/Restore guide.
The sequence diagram now routes request failure and success to the requesting
member's callback participant, while showing that requester in one of the
Restore member lanes. A recipient table separates current Umbra source routing
from focused test evidence and explicitly avoids treating API declarations or
implementation behavior as a normative receiver-scope rule.

The source maps admission outcomes to the requester
([failure](../../cpp/src/internal/federation/federation_registry_restore_control.cpp#L1089),
[success](../../cpp/src/internal/federation/federation_registry_restore_control.cpp#L1131)),
then plans begun and initiate notifications for each active member
([member loop](../../cpp/src/internal/federation/federation_registry_restore_control.cpp#L1144),
[member identity payload](../../cpp/src/internal/federation/federation_registry_restore_control.cpp#L1161)).
Terminal notifications are planned for operation members who remain joined and
have a callback route ([completion recipient](../../cpp/src/internal/federation/federation_registry.cpp#L545)).
The process service preserves each planned recipient through session selection
([process event](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L505))
and maps the event kind to the public callback ([bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L969)).

The configured-process multi-member test uses polling and `HLA_EVOKED`
([service option](../../cpp/tests/ieee1516_2025_connection_synchronization_restore_catch2.cpp#L1720),
[requester connection](../../cpp/tests/ieee1516_2025_connection_synchronization_restore_catch2.cpp#L1954),
[peer connection](../../cpp/tests/ieee1516_2025_connection_synchronization_restore_catch2.cpp#L1965)).
It observes request-success on the requester, begun/initiate on both members,
and terminal success on both after both report completion
([requester callbacks](../../cpp/tests/ieee1516_2025_connection_synchronization_restore_catch2.cpp#L1990),
[peer callbacks](../../cpp/tests/ieee1516_2025_connection_synchronization_restore_catch2.cpp#L1998),
[terminal callbacks](../../cpp/tests/ieee1516_2025_connection_synchronization_restore_catch2.cpp#L2095)).
The embedded two-member case checks both callback models and both members'
Restore callbacks ([focused assertions](../../cpp/tests/ieee1516_2025_federation_management_save_restore_catch2.cpp#L745)).
This corrects the former process-polling coverage gap; the remaining profile
gap is specifically polling with `HLA_IMMEDIATE`.

Re-rendered and screenshot-reviewed the changed Restore sequence. The full
ledger audit also found stale body hashes for Callback and Service Ordering
diagram 019 and TSO Retraction diagram 008; both were re-rendered and reviewed.
Refreshed the Save/Restore ledger lines for its 23 later diagrams and updated
three diagram hashes. All 498 entries remain identity-matched. The source-link
check and authenticated GFM results are recorded in the current roadmap and
render audit. No C++ tests/builds, CI, 2010 changes, Requirements Lab edits, or
source changes were made. Overall progress remains **80%**; formal gates remain
**3/4 (75%)**.

**Next handoff:** Audit terminal Restore notification behavior after a member
reports `federateRestoreNotComplete`, aborts, or departs during the operation.
Trace embedded and configured-process routes and label each outcome by its
available test evidence; keep normative failure triggers unresolved without
licensed 2025 text. Publication and visible GitHub hydration remain the final
formal gate after explicit authorization.


## 2025 Restore terminal failure, abort, and departure routes, increment 318 — 2026-11-08

Completed the terminal Restore route review in the
[Save/Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md). The
registry plans participant-failure and abort results for operation members who
remain joined and have callback routes. Configured-process polling tests under
`HLA_EVOKED` assert `FEDERATE_REPORTED_FAILURE_DURING_RESTORE` and
`RESTORE_ABORTED`; each case has one member. The guide now identifies this
evidence alongside the existing `HLA_IMMEDIATE` cases and explicitly leaves
multi-member process distribution untested.

On departure, the registry excludes the departing ID and prepares survivor
failure results with `FEDERATE_RESIGNED_DURING_RESTORE`. The embedded public
Resign case observes the survivor callback. The configured-process Resign and
ConnectionLost paths pass through a dispatcher that does not enqueue the
Restore result vector. This is a source-level finding only; no process
departure test establishes the callback outcome. The existing Restore
member-exit diagram already represented this split, so its body did not change.
The integrity check corrected a stale ledger hash for the previously rendered
and screenshot-reviewed TSO Retraction diagram 008; all 498 entries now match
their current bodies. Exact normative failure triggers remain unresolved
pending licensed 2025 clause text.

The worktree, candidate-base, and primary-source line checks resolve all 4,369
unique references across 35 Markdown files, with no unresolved targets. No C++
tests/builds, CI, 2010 changes, Requirements Lab edits, or source changes were
made. The authenticated GFM pass checked 35/35 pages and 498/498 Mermaid
containers, with 978 headings, 7,659 links, and zero failures. Published-page
hydration remains unverified. Overall progress remains **80%**; formal gates
remain **3/4 (75%)**.

## 2025 active-Restore Join admission and participant-ledger review, increment 319 — 2026-11-08

Reviewed both pinned 2025 Join overloads and the embedded and configured-process
Join routes. The API lists `SaveInProgress` and `RestoreInProgress` for both
overloads. Restore admission snapshots current member IDs into
`RestoreOperation::statuses` as `FEDERATE_RESTORING`; the registry Join
transaction inserts membership separately. The inspected adapters, process
handler, and registry membership path show no active-Restore phase predicate or
update to an existing Restore status map. If Join were admitted after that
snapshot, the reviewed path does not show the newcomer entering that operation
ledger. This is a conditional source observation, not a claim that Join is
accepted, rejected, or has a particular public result.

A bounded scan of 1,011 2025 Catch2 `TEST_CASE` bodies and the explicit
`SaveInProgress`/`RestoreInProgress` assertions found no focused attempt to
Join during an active Save or Restore. The representative interlock case joins
both members before requesting Save; its Restore-phase assertions cover other
services. A call-order candidate in a fresh-registry test was checked manually:
the later Join follows successful Save completion, resignation, and federation
teardown. The exact 2025 §4.11 conditions remain unresolved without licensed
clause text; the API exception list alone does not settle the rule, and no 2010
wording was used.

Corrected stale API-exception, Restore-ledger, and membership-insertion
citations in the Lifecycle, Cross-Cutting, MOM, and Save/Restore guides. The
Save/Restore prose now explains the captured participant-ledger boundary and
points directly to the first Restore interlock assertion. No Mermaid body
changed. The prose addition moved later Save/Restore diagram fences by nine
lines; updated the 35
affected ledger line entries and verified all 36 Save/Restore identities,
fence locations, types, and unchanged body hashes.

The worktree, candidate-base, and primary-source checks resolve all 4,370
unique relative path/line/guide references across 35 Markdown files, with no
unresolved targets (5,662 relative link occurrences). The immutable permalink
check remains 148 occurrences across 40 targets. The authenticated GitHub
Markdown API pass returned non-empty HTTP 200 responses for all 35 scoped
pages and registered 498/498 Mermaid containers, with 979 headings, 7,663
rendered links, and zero failures. The global ledger check confirms all 498
identities, fence lines, types, and body hashes match. No C++ tests/builds, CI,
2010 changes, Requirements Lab edits, or source changes were made. Overall
progress remains **80%**; formal gates remain **3/4 (75%)**.

**Next handoff:** Continue the semantic source-citation review in the
[Process Endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#hla-message-designator-is-not-transport-correlation).
Trace how the private `requestId` correlates a process response while the HLA
message designator remains a separate service value; verify the client/session
source links and focused evidence. Keep source transport behavior distinct
from HLA callback and logical-time semantics. Do not build or run C++ tests.
After explicit publication authorization, inspect the 32 current GitHub topic
pages for visible diagrams and links; keep 2010 and Requirements Lab excluded.

## 2025 process request correlation versus HLA message designator, increment 320 — 2026-11-08

Reviewed the Process Endpoint guide's distinction between the HLA
`MessageRetractionHandle`/`LogicalTime` and the private process `requestId`.
The pinned 2025 API declares the timestamped service result and `retract` handle;
Umbra's private `TransportServiceMessage` carries `requestId` as an outer field.
Protocol validation requires nonzero IDs for requests and responses and zero
for unsolicited events. The client assigns the ID, waits on the response map,
and checks both operation and ID; the session request helper and server
dispatcher enforce the same matching pair.

The focused transport-dispatch case sends Create, Join, and Send Interaction
requests with distinct IDs and checks the returned operation, ID, status, and
payload. This is framing-level correlation evidence; it does not test HLA
retraction-handle or logical-time mapping. Updated the guide's source citations
and clarified that test boundary. The prose addition moved 21 Process Endpoint
diagram fences by thirteen lines; refreshed their ledger locations without
changing any diagram body or hash.

The worktree, candidate-base, and primary-source checks resolve 4,375 unique
relative path/line/guide references (5,668 link occurrences), with zero
unresolved. All 498 diagram identities, fence lines, types, and body hashes
match. The last authenticated GFM pass remains increment 319; this local prose
update has not been published or checked on GitHub. No C++ tests/builds, CI,
2010 changes, Requirements Lab edits, or source changes were made. Overall
progress remains **80%**; formal gates remain **3/4 (75%)**.

## Process Endpoint callback lock boundary, increment 321 — 2026-11-08

Reviewed the Process Endpoint callback path against pinned client sources. The
guide now distinguishes `exchangeMutex_`, which serializes request/response
pairs, from `transactionMutex_`, which protects request depth and pending-event
state and is not held while waiting for the stream reader. A matched response
updates request depth and releases the exchange gate before pushed-event
draining; the dispatcher releases the state mutex before callback-bridge
submission. The guide labels this source-derived behavior and states that the
cited delivery tests do not assert callback thread identity, mutex ownership, or
callback re-entry. Updated the flowchart node and ten ledger entries; diagram
012's body hash was refreshed after Mermaid CLI 12.0.0 rendering and screenshot
review. All 498 diagram identities, lines, types, and hashes match. Worktree,
candidate-base, and primary-source checks resolve 4,381 unique source-line
references (5,674 relative link occurrences), with zero unresolved. No C++
tests/builds, CI, source changes, 2010 changes, or Requirements Lab edits were
made. Overall progress remains **80%**; formal gates remain **3/4 (75%)**. The
authenticated GFM pass remains increment 319; the candidate is unpublished and
its visible GitHub Mermaid hydration is unverified.

## Process Endpoint polling-family priority, increment 322 — 2026-11-08

Audited the Process Endpoint polling selector against pinned 2025 service
sources and focused test cases. `ProcessFederationService::SessionState` holds
separate per-family queues, and `sessions_` associates that state with each
transport session. The poll chain selects synchronization announcement,
federation synchronized, Save, Restore, then interaction events before later
families. Interaction sending appends to the per-session poll queue when push is
disabled and emits a pushed frame when enabled; notification families also have
separate push and poll producer branches. The guide now cites the queue members,
session map, and interaction poll enqueue point.

Test evidence was refined: the two public synchronization cases exercise
polling and pushed paths separately. A private Save/Restore case queues an
interaction before Save, polls until it is returned, restores, polls until it is
returned again, and checks that no duplicate remains. This supports queue
persistence and exactly-once behavior, not exact cross-family priority or
starvation. Seven later Process Endpoint diagram line locations were refreshed;
no diagram body changed. All 498 ledger identities, headings, types, fences,
lines, and body hashes match. Worktree, candidate-base, and primary-source
checks resolve 4,391 unique source-line references (5,684 relative link
occurrences), with zero unresolved. No C++ tests/builds, CI, source changes,
2010 changes, or Requirements Lab edits. Overall progress remains **80%**;
formal gates remain **3/4 (75%)**. The authenticated GFM pass remains increment
319; the candidate is unpublished and current edits are unchecked on GitHub.

## Process Endpoint discovery/removal merge, increment 323 — 2026-11-08

Reviewed the configured-process poll branch that merges per-session object
discovery and removal queues. Both producers assign from one session-local
sequence, and the selector compares the queue fronts after earlier polling
families are empty. Added a flowchart and source map. The chart distinguishes
this private poll rule from a standard-wide callback order.

Reviewed two focused test sources. The MOM resign/rejoin case records removal
before rejoined discovery and reflection, but waits for removal before the
rejoin, so it does not test two pending queue heads. The public process Delete
case observes discovery before issuing Delete and then checks removal identity,
producer, and tag; it also does not test competing heads or stale removal
skipping. The receive loop skips stale removals without re-running the
discovery comparison. A three-event arrangement could therefore let a later
valid removal pass a pending lower-sequence discovery, but the reviewed tests
do not construct it and this increment did not prove it reachable under current
producer and cleanup invariants. Recorded it as an unverified implementation
review question, not a conformance finding. The vendored API headers identify
callbacks as §§6.9, 6.11, and 6.17 but do not contain their normative ordering
prose; the [IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
lists full-standard access by subscription.

Rendered and screenshot-reviewed the new flowchart with Mermaid CLI 12.0.0.
The final identity pass also re-rendered the current Callback and Service
Ordering diagram 019 and TSO Retraction diagram 008 bodies, then reconciled all
three hashes. The ledger now has 499 identity-matched diagrams, including 292
flowcharts.
The documentation link checker resolves 4,404 unique path/line/guide
references across 35 Markdown pages at the worktree, pinned 2025 source base,
and primary source revision; the 5,848 relative link occurrences have no
unresolved targets. No C++ tests/builds, CI, source changes, 2010 changes, or
Requirements Lab edits. Overall progress remains **80%**; formal gates remain
**3/4 (75%)**. The authenticated GFM pass remains increment 319 (498 diagrams);
the candidate is unpublished and the new diagram has not been checked on
GitHub.

**Next handoff:** Trace the stale-removal producers and cleanup paths to decide
whether a stale removal can remain ahead of a pending discovery and a later
valid removal in one session. Keep the result source-only unless existing
focused evidence proves more; do not build or run C++ tests. Then continue the
2025 semantic citation review in the next prioritized guide. After publication
authorization, inspect all 32 topic pages for visible diagrams and links; keep
2010 and Requirements Lab excluded.


## Process Endpoint stale-removal retraction interleaving, increment 324 — 2026-11-08

Traced the configured-process Delete, Retract, removal-queue, and polling paths.
For a timestamped Delete, a non-time-constrained recipient enters the ordinary
removal queue with the retraction message ID. On Retract, the registry changes
a pending recipient to retracted before `handleRetract` reacquires the service
mutex to erase the queued event. The host serves accepted connections on
separate worker threads, so producer Retract and receiver polling can overlap.

A reachable queue arrangement is removal R1 (sequence 1), discovery D2 (2),
and later valid removal R3 (3), with earlier polling families empty. If the
poll acquires the service mutex after R1 has been retracted but before queue
erasure, it selects R1, receives no registry snapshot, and continues through
the removal loop without comparing D2 again. It can return R3 before D2. The
new sequence diagram shows this configured-process pull interleaving. This is
source-derived, not test-observed and not an HLA-wide callback-order rule.

The focused process retraction-gate test uses pushed receive-order events, so
it does not exercise the polling deques or cleanup race. The reviewed MOM
resign/rejoin case waits for the prior removal, and the public Delete case does
not arrange competing queue heads. No focused test covers this interleaving.

A separate review of ConnectionLost removal found that the registry can defer
forced removal while timestamped object delivery still needs receiver state.
The embedded time-advance path releases and replans deferred removals after TSO
completion. The configured-process TSO completion path has no corresponding
release call; whether the deferred process removal is retried remains
unresolved and is not presented as a verified callback outcome.

Rendered the new Mermaid sequence with Mermaid CLI 12.0.0 and screenshot-
reviewed its current body. The ledger now records 500 identity-matched diagrams:
292 flowcharts, 184 sequences, and 24 state diagrams. All 4,417 relative
source-line references (5,720 occurrences) resolve at the worktree, pinned
candidate base, and primary source revision. The independent immutable-source
permalink check passes all 149 occurrences (102 unique path/line pairs across
40 targets). No C++ tests/builds, CI, source changes, 2010 changes, or
Requirements Lab edits were made for this increment. Overall progress remains
**80%**; formal gates remain **3/4 (75%)**. The authenticated GFM pass remains
increment 319 at 498 diagrams; the candidate is unpublished.

**Next handoff:** Continue semantic citation review in the Callback and Service
Ordering guide. Check callback-order claims and source/test links against the
exact 2025 API and available standard text. Separate source queue order from
public callback observations and normative guarantees; preserve unresolved
standard wording without licensed clause evidence. Do not build or run C++
tests. After publication authorization, inspect all 32 current topic pages for
visible diagrams and links. Keep 2010 and Requirements Lab excluded.


## Callback and Service Ordering citation review, increment 325 — 2026-11-08

Verified the Callback and Service Ordering guide's public API clause locators
against the pinned 2025 `RTIambassador.h`: `connect` is §4.2,
`evokeCallback` is §10.57, `evokeMultipleCallbacks` is §10.58,
`enableCallbacks` is §10.59, and `disableCallbacks` is §10.60. The official IEEE
page lists 1516.1-2025 as active and states access is by subscription. The
vendored headers locate clauses but do not provide enough normative text to
settle callback ordering across services or callback families; the guide's
caveat against inferring such an order remains necessary.

Cross-checked the `FederationSynchronized` process sequence and evidence map.
The configured-process `HLA_EVOKED` polling and `HLA_IMMEDIATE` pushed cases
insert only the receiver into the explicit synchronization set. They establish
their selected registration, announcement, and failure callbacks, but do not
assert registration-versus-announcement order for one recipient, service-
report order, or cross-profile parity. The separate embedded resignation test
is explicitly tagged `[development-profile]`; it checks that the
`FederationSynchronized` service-report record is written before the pending
callback is evoked in that one `HLA_EVOKED` scenario. Updated the guide's
normative-source note and made the test's development-profile boundary visible.
No Mermaid body changed. The introductory prose shifted the Callback guide
diagram line locations, so refreshed all 26 ledger line entries without
changing diagram hashes or screenshot records.

The source-line check resolves all 4,417 unique relative references (5,720
occurrences) at the worktree, pinned candidate base, and primary source
revision. The recorded immutable-source check covers 149 permalinks across 40
targets. The existing ledger remains at 500 identity-matched diagrams: 292
flowcharts, 184 sequences, 24 state diagrams. No C++ tests/builds, CI, 2010
changes, or Requirements Lab edits were made. Overall progress remains **80%**;
formal gates remain **3/4 (75%)**. The last authenticated GFM pass remains
increment 319 at 498 diagrams; the candidate is unpublished.

**Next handoff:** Audit the Callback and Service Ordering guide's process-profile
Resign and queued-callback boundary against current client, server,
callback-bridge, and focused-test evidence. Distinguish work still in the server
queue, buffered by the client, pending in the dispatcher, already selected, and
already executing. Keep old-session frame correlation after a later Join
unresolved unless evidence establishes it. Do not build or run C++ tests. After
publication authorization, inspect all 32 current topic pages for visible
diagrams and links. Keep 2010 and Requirements Lab excluded.


## Process Resign queue and callback boundary review, increment 326 — 2026-11-08

Traced the configured-process Resign path across the client, service handler,
joined-session cleanup, callback bridge, shared dispatcher, callback session,
and focused tests. Before sending the correlated request, the client suppresses
pushed-event dispatch and waits for an active client dispatch to finish. A
service rejection before the registry transition releases suppression, drains
buffered events and deferred acknowledgements, and reports the failure. After
registry resignation, loss/survivor routing and grant reevaluation run before
the service clears remaining joined-session queues and sends success. A later
internal failure can therefore return an error after the registry transition
but before that cleanup is complete.

On successful response, the client clears its old joined identity and pending
event FIFO, cancels retraction/projection state, and resets the dispatcher. The
reset discards only work still in the dispatcher's pending queue. It cannot
retrieve a task already extracted by an evoke/immediate drainer or stop a user
callback already executing; process Resign leaves the callback session open.
The public adapter rejects Resign from the active callback thread. A bridge
unit case covers cancellation of one exception-report projection waiting to
enter the callback session, but no focused public case covers pending work,
concurrent extraction, or a callback already in progress during Resign.

The client frame-admission guard checks for a currently installed joined
identity. The inspected path does not compare an old frame's receiving
federate identity with a per-Join generation, so old-session correlation after
a later Join remains unverified; the source trace does not prove that such a
frame can cross server cleanup and the response. The push helper can select a
session under the service mutex and send after releasing it, outside queue
clearing. Whether that selected push can be written after the correlated
Resign response remains untested.

Replaced the tall Resign flowchart with a four-participant sequence diagram.
The first render exposed overlapping explanatory notes; after moving each note
to its relevant stage, Mermaid CLI 12.0.0 rendered the current chart and local
screenshot review found no overlap or clipping. Also re-rendered and reviewed
TSO Retraction diagram 008 after confirming its ledger hash was stale. The
ledger now matches all 500 current identities, headings, types, fences, lines,
and body hashes: 291 flowcharts, 185 sequences, and 24 state diagrams. Three
later Callback guide line entries were refreshed.

The source-line check resolves all 4,419 unique relative references (5,732
link occurrences) across 35 Markdown files at the worktree, pinned candidate
base `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`, and primary source revision
`e6321e842f333427349166df8c5b35350a2ae473`. The separate immutable-source
check resolves all 149 GitHub permalinks (102 unique path/line pairs across 40
targets). The last authenticated GFM pass remains increment 319 at 498
containers; current edits are unpublished, so visible GitHub page rendering
remains unverified. No C++ source edits, builds, tests, or CI were made for
this increment. Unrelated worktree changes remain untouched. No 2010 or
Requirements Lab paths were changed. Overall progress remains **80%**; formal
gates remain **3/4 (75%)**.

**Next handoff:** Audit semantic source anchors into `process_federation_client.cpp`
across the Callback and Service Ordering, Process Endpoint, and related 2025
guides. Verify that each cited line supports its label in the pinned 2025 source
and current worktree; distinguish buffering, request-response matching, event
draining, reader, and dispatcher paths. Continue with the next citation seam
without building or running C++ tests. After publication authorization, inspect
all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements
Lab excluded.
## 2025 process-client semantic citation audit, increment 327 — 2026-11-08

Audited 125 current links into process_federation_client.cpp across 16 2025
guides against the pinned candidate source. Corrected anchors where labels
pointed at adjacent but different operations: client frame intake, typed event
decoding, response correlation, request-owned callback drains, grant-event
branches, Request Retraction filtering, and stream/request failure dispatch.
In the Exception Reporting guide, corrected the request-depth description:
the lease remains held while request() waits for its matched response, and a
report request with callback draining disabled skips the request-owned event
drain. The independent background dispatcher can process buffered work after
its gates clear; that timing is not an atomic guarantee.

Re-rendered Callback and Service Ordering diagram 019 with Mermaid CLI 12.0.0
and screenshot-reviewed the result. Updated its current-body hash, repaired a
duplicate Process Endpoint diagram identity, and refreshed four shifted
Exception Reporting diagram source lines. The full visual-ledger check now
matches all 500 unique diagram identities, fences, types, source lines, and
body hashes (291 flowcharts, 185 sequences, 24 state diagrams).

The documentation link checker resolves all 4,430 unique relative
path/line/guide references across 35 IEEE 1516.1-2025 Markdown files in the
worktree, pinned candidate base, and primary source revision; these comprise
5,743 relative link occurrences. The separate pinned GitHub check resolves all
151 line links (104 unique path/line pairs across 41 targets). No C++ tests,
builds, or CI were run. No 2010 or Requirements Lab content was changed.
Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated
GFM pass remains increment 319 at 498 diagrams; the candidate is unpublished.

**Next handoff:** Audit process callback-dispatch and bridge citations across
the 2025 Callback, Process Endpoint, Time Management, and lifecycle guides.
Focus on event-lease ownership, bridge admission, callback-model timing, and
exception propagation. Continue the remaining 2025 semantic citation seams,
then inspect all 32 published pages after authorization. Keep 2010 and
Requirements Lab excluded.

## 2025 process callback-dispatch and callback-bridge citation audit, increment 328 — 2026-11-08

Audited 204 source references into `process_federation_client_callback_dispatch.cpp`
and `process_federation_callback_bridge.cpp` across 21 IEEE 1516.1-2025 topic
guides. Corrected 90 link occurrences that landed on adjacent operations, blank
lines, or brace-only lines. The corrections cover dispatch lease ownership,
receive-order and attribute callback admission, disable-generation conversion,
post-callback TSO acknowledgement, exception-report projection and admission,
ownership confirmation, discovery/removal/scope callbacks, and Request
Retraction state. A scan of the 204 references against the pinned candidate
source now finds zero blank, comment-only, or brace-only targets.

The documentation link checker resolves all 4,431 unique relative path/line/guide
references (5,744 occurrences) across 35 Markdown files at the worktree, pinned
candidate base `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`, and primary revision
`e6321e842f333427349166df8c5b35350a2ae473`. A local Git-object check resolves all
153 immutable GitHub source links (106 unique revision/path/line pairs across 41
target files). Cross-revision checks establish line existence; the semantic
review in this increment used the pinned candidate source and does not establish
semantic equivalence at the primary revision. No diagrams, C++ tests/builds/CI,
2010 content, or Requirements Lab content changed. Overall progress remains
80%; formal gates remain 3/4 (75%). The authenticated GFM pass remains increment
319 at 498 diagrams, and the candidate remains unpublished.

**Next handoff:** Audit 2025 process event production and transport delivery in
the Federation Synchronization, Data Distribution and Regions, and MOM/Lifecycle
guides. Trace service-side recipient/event planning, Push sends versus Pull
queue/dequeue, client intake, and callback mapping. Check each source anchor
against the pinned candidate and each test citation against the assertion it
actually makes. Continue the remaining source and standards citation review;
exact normative callback-order and Save/Restore trigger wording remains open
pending licensed 2025 text. After publication authorization, inspect all 32
topic pages for visible diagrams and links. Keep 2010 and Requirements Lab
excluded, and do not run C++ builds or tests for this documentation pass.

## 2025 process-event production and Push/Pull delivery citation audit, increment 329 — 2026-11-08

Reviewed event delivery from service and registry planning through transport routing, client intake, and public callback projection across the Federation Synchronization, Data Distribution and Regions, and Federation/Federate Lifecycle guides. Corrected stale anchors for synchronization barrier closure and late-join rollback, Push staging versus polling queueing, regional scope changes, process roster and Destroy handling, and connection-loss notifications. Tightened test links to the relevant setup and assertions, and retained explicit limits where tests do not cover multi-point rollback, partial-result interleavings, or cross-profile parity.

The 2025 documentation checker resolves all 4,439 unique relative source-line references across 35 Markdown files at the worktree. All 103 changed local line targets in the three-guide diff exist at the pinned candidate base and primary source revision; that confirms line existence, not semantic equivalence. No C++ sources, builds, tests, CI, diagrams, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM pass remains historical increment 319 at 498 diagrams; the candidate remains unpublished.

**Next handoff:** Audit IEEE 1516.1-2025 Save/Restore process-event production and delivery across the Save/Restore, Process Endpoint, and Federation/Federate Lifecycle guides. Trace registry outcomes through service Push/Pull routing, client intake, and callback projection; check focused tests for recipient sets, outcomes, and callback-model limits. Keep portable HLA rules separate from Umbra evidence. Exact normative callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. Continue the remaining source and standards review, then inspect all 32 published topic pages for visible diagrams and links after authorization. Keep 2010 and Requirements Lab excluded; do not run C++ builds or tests for this documentation pass.


## 2025 Save/Restore process-event delivery citation audit, increment 330 — 2026-11-08

Traced configured-process Save and Restore outcomes from registry planning through service Push/Pull routing, client intake, and callback projection. Corrected stale semantic source and test anchors and clarified response correlation versus callback invocation, Push versus polling, and service routing of registry-planned terminal notifications. Reconciled all 36 Save/Restore diagram ledger positions. The changed federation-outcome sequence (diagram 010) was rendered with Mermaid CLI and screenshot-reviewed. A full identity check matches all 500 current diagram entries to their source lines, headings, types, fences, and body hashes.

The documentation checker resolves all 4,456 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files in the current worktree, with zero unresolved. No C++ sources, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams, and the current candidate remains unpublished.

**Next handoff:** Audit IEEE 1516.1-2025 Save/Restore member-exit and failure boundaries across embedded and configured-process sources and focused tests. Distinguish admission failure from begun-operation and terminal outcomes; verify recipient scope, callback projection, and test limits. Keep normative obligations separate from Umbra evidence. Exact Save/Restore trigger and callback-order wording remains open pending licensed 2025 text. Continue the remaining source and standards review; after publication authorization, inspect all 32 topic pages for links and visible diagrams. Keep 2010 and Requirements Lab excluded, and do not run C++ builds or tests for this documentation pass.


## 2025 Save/Restore member-exit and failure-boundary audit, increment 331 — 2026-11-08

Traced voluntary Resign and forced ConnectionLost through the shared registry transition and both profile adapters. The embedded adapter submits planned Save/Restore failure notifications. The configured-process departure dispatcher does not consume those vectors, although explicit Resign can still return correlated `ok` if the handler completes. The guide labels that finding as source-level only: no focused configured-process case establishes the public member-exit callback, and the inspected source does not establish the normative ConnectionLost reason.

Added focused process evidence that separates pre-begin request rejection from a Restore failure after admission under HLA_IMMEDIATE and HLA_EVOKED. Clarified the one-member limits, that the process Save-failure case resigns only after its terminal failure, and corrected duplicate assertion links. Reconciled all 36 Save/Restore diagram line positions and re-rendered/screenshot-reviewed changed diagram 008. The full 500-entry ledger identity check passes; all 4,468 unique worktree path/line/guide references resolve across 35 Markdown files. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Audit the post-admission `RTI_UNABLE_TO_RESTORE` path end to end: image/state application failure selection, registry terminal notification, configured-process projection, and focused assertions. Distinguish the failure reason from the success flag, since current internal success notifications also carry `RTI_UNABLE_TO_RESTORE` as a placeholder. Keep current Umbra behavior separate from normative 2025 trigger language; exact reason semantics remain open pending licensed standard text. Then continue the remaining 2025 source and standards review. After publication authorization, inspect all 32 topic pages for links and visible diagrams. Keep 2010 and Requirements Lab excluded, and do not run C++ builds or tests for this documentation pass.


## 2025 Save/Restore `RTI_UNABLE_TO_RESTORE` failure-path audit, increment 332 — 2026-11-08

Traced the missing process-local snapshot and caught restore-stage exceptions through registry notification planning, configured-process projection, and both public adapter branches. The registry can return `restore_failed` after applying the image, but the post-apply catch cannot enqueue `federationNotRestored` because `restoreOperation` was already cleared. The current shared adapter mapper has no dedicated `restore_failed` case and falls through to `RTIinternalError`. Successful completion records also carry `RTI_UNABLE_TO_RESTORE` as a placeholder; process transport preserves `successful` and `failureReason` separately, and the callback bridge chooses the callback from `successful`.

A focused test search found no assertion selecting `RTI_UNABLE_TO_RESTORE`; reusable test-ambassador initial values are defaults. The normative reason trigger remains open pending licensed 2025 text. Reconciled all 36 Save/Restore diagram positions; three line positions moved after the prose update and body hashes are unchanged. The full 500-entry diagram line/body check passes. The documentation checker resolves 4,475 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Trace `RTI_DETECTED_FAILURE_DURING_RESTORE` from the pinned 2025 API declaration through runtime selectors and focused assertions. Record current implementation evidence and leave its normative trigger open until licensed standard wording is available. Continue the remaining 2025 source and standards review; after publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded, and do not run C++ builds or tests for this documentation pass.


## 2025 Save/Restore `RTI_DETECTED_FAILURE_DURING_RESTORE` audit, increment 333 — 2026-11-08

Compared the pinned 2025 `RestoreFailureReason` declaration with the current runtime and test trees. The enum value is declared in `RTI/Enums.h`, but exact-value searches found no selector in `cpp/src` and no use in `cpp/tests`. Updated the Save/Restore reason table with the API declaration anchor and stated the implementation/test absence without inferring a normative trigger.

The source-link checker resolves 4,476 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. All 500 diagram entries still match their source line and body hash. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Audit the 2025 `queryFederationRestoreStatus` path from registry status-vector selection through configured-process encoding and callback delivery, then through the embedded callback adapter. Compare only focused tests that assert returned participant statuses. Keep progress-status callbacks distinct from terminal `RestoreFailureReason` callbacks and record profile/test gaps. Continue the remaining source and standards review; exact normative callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. After publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded, and do not run C++ builds or tests for this documentation pass.
