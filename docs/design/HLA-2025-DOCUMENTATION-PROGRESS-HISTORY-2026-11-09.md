# IEEE 1516.1-2025 Documentation Progress History — 2026-11-09

[History index](HLA-2025-DOCUMENTATION-PROGRESS-HISTORY.md) · [active roadmap](HLA-2025-DOCUMENTATION-ROADMAP.md)

## 2025 Save/Restore status-query evidence audit, increment 334 — 2026-11-09

Traced `queryFederationRestoreStatus` from registry snapshot selection through configured-process service routing, client delivery, callback-bridge projection, and the embedded adapter. Corrected a stale guide statement claiming there was no configured-process `HLA_EVOKED` status-query case. Focused tests cover pushed `HLA_IMMEDIATE` status queries and polling `HLA_EVOKED` queries, including callback deferral until Evoke and one- and two-member active, waiting, and idle status vectors. The other two process transport/callback combinations remain untested. Query status callbacks remain distinct from terminal Restore result callbacks.

The documentation checker resolves 4,489 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. All 500 diagram entries still match their source lines and body hashes. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Audit the sibling 2025 `queryFederationSaveStatus` path from registry snapshot selection through configured-process encoding and callback delivery, then through the embedded adapter. Check focused assertions for active and idle SaveStatus vectors across callback models and event-delivery modes. Keep the query callback distinct from terminal Save failure callbacks and record unsupported profile combinations. Continue source and standards review; exact normative callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. After publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded, and do not run C++ builds or tests for this documentation pass.


## 2025 Save/Restore Save-status query evidence audit, increment 335 — 2026-11-09

Traced `queryFederationSaveStatus` from the registry's requester-only snapshot through configured-process service routing, client delivery, callback projection, and the embedded adapter. The configured-process tests show an active one-member snapshot under polling with `HLA_EVOKED` (`FEDERATE_INSTRUCTED_TO_SAVE`, then `FEDERATE_SAVING`), an idle one-member snapshot after failure under pushed `HLA_IMMEDIATE`, and two-member idle snapshots after failure under polling `HLA_EVOKED`. The guide now separates those cases and states the missing Save-query profiles and successful-terminal snapshot evidence. The embedded integration continues to cover two members at instructed, saving, and idle states.

The documentation checker resolves 4,493 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. Reconciled 31 shifted Save/Restore diagram positions; all 500 line/fence/body identities match and no Mermaid body changed. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Trace configured-process successful Save from the final participant completion through image encoding/commit and public completion callbacks. Compare only focused process and embedded tests, identify which participant and callback-model paths they prove, and record remaining profile gaps without inferring normative behavior. Continue the broader 2025 source and standards review; exact callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. After publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded, and do not run C++ builds or tests for this documentation pass.


## 2025 configured-process Save success-profile evidence, increment 336 — 2026-11-09

Rechecked the successful configured-process Save path from the last participant's `federateSaveComplete` through registry commit, notification construction, process-service routing, and callback projection. The image is committed before the registry plans `federationSaved`; an encoding or store failure instead plans `federationNotSaved(RTI_UNABLE_TO_SAVE)`. The process service routes the planned terminal event before its correlated response, and the client matches that response before draining buffered pushed events. This remains an Umbra source trace, not a normative callback-order claim.

Found that the guide's profile matrix omitted direct successful Save evidence for pushed delivery with `HLA_IMMEDIATE`. Added the two-member test that checks both `federationSaved` counters after completion. The guide now distinguishes three directly observed configured-process success profiles: polling/`HLA_EVOKED` with one member, pushed/`HLA_IMMEDIATE` with two members, and pushed/`HLA_EVOKED` with two members. Polling with `HLA_IMMEDIATE` remains without a focused successful Save case; the pushed/`HLA_EVOKED` test does not assert callback counts immediately after each Complete call.

The documentation checker resolves 4,497 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. All 500 diagram line/fence/body identities still match; no Mermaid body or diagram position changed in this increment. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Audit configured-process successful Restore completion from the final member's `federateRestoreComplete` through state application, terminal notification routing, and public `federationRestored` callbacks. Compare focused process tests across polling/pushed delivery and `HLA_IMMEDIATE`/`HLA_EVOKED`; distinguish directly asserted callback timing and recipients from source-only paths, and record the missing combinations. Keep Restore failure reasons separate and do not infer normative behavior. Continue the broader 2025 source and standards review; exact callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. After publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded, and do not run C++ builds or tests for this documentation pass.


## 2025 configured-process Restore success-profile evidence, increment 337 — 2026-11-09

Traced the final configured-process `federateRestoreComplete` through registry state application, successful completion notification construction, process-service routing, and the callback bridge. The registry applies the restored image before it constructs one `federationRestored` completion record for each current member with a live route. The process handler enqueues those lifecycle records before saved ownership-assumption work. The callback bridge selects `federationRestored` from the record's success flag. This is Umbra source behavior, not a normative callback-order claim.

The guide's process profile evidence now explicitly covers pushed delivery with `HLA_IMMEDIATE` for both one-member and two-member successful Restore cases, polling with `HLA_EVOKED` for one- and two-member cases, and pushed delivery with `HLA_EVOKED` for two members. The tests directly assert the one-member immediate callback before service return, both immediate two-member completion counts after both Complete calls, deferred polling callbacks after Evoke, and the pushed/evoked callback observations. Polling with `HLA_IMMEDIATE` remains without a focused successful Restore case. No test reruns were needed or performed.

The documentation checker resolves 4,502 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. All 500 diagram line/fence/body identities still match; no Mermaid body or diagram position changed. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Audit successful process-local Restore's service-side event queues that are not part of the registry image. Trace how polling queues are cleared or rebuilt and how saved receive-order interactions and attribute updates are re-enqueued; compare with focused tests for duplicate suppression, delivery count, and event ordering. Keep transport-specific queue behavior separate from HLA-wide Restore obligations and from failure callbacks. Continue the broader 2025 source and standards review; exact callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. After publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded, and do not run C++ builds or tests for this documentation pass.


## 2025 configured-process receive-order queue replay evidence, increment 338 — 2026-11-09

Audited how configured-process Save captures ordinary receive-order work and how successful Restore projects it back to live service queues. The guide now records three focused private service/client cases: a polling interaction, a polling attribute update, and a pushed attribute update. Each observes one restored event/update without an extra delivery. A public configured-process `HLA_EVOKED` attribute case also observes exactly one restored reflection. Pushed attribute replay retains `pushDeliveryId`; the service inserts the event into its pending map and sends it only when that identifier is new.

The polling interaction test consumes the live event after the image is committed and before Restore. The successful Restore service path appends the saved interaction to `interactionEvents`; unlike the polling attribute queue, the inspected path has no equivalent visible clear. The guide records this as an unverified retained-copy boundary, not a confirmed defect. The public embedded case that discards a pre-Restore interaction remains a separate profile.

Reconciled the stale body hash for the already locally rendered and screenshot-reviewed Callback and Service Ordering diagram 019; no diagram body or position changed in this increment. All 500 ledger line/fence/body identities match. The documentation checker resolves 4,509 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Cross-check the retained polling-interaction queue boundary against the public embedded case that discards an undelivered pre-Restore interaction and the configured-process service test that drains its live copy before Restore. Keep those profiles separate. If no existing configured-process evidence covers a still-queued copy at Restore completion, retain that as an explicit implementation/test gap; do not infer a defect or normative behavior. Continue the broader 2025 source and standards review; exact callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. After publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded; do not run C++ builds or tests for this documentation pass.


## 2025 configured-process polling interaction Restore queue boundary, increment 339 — 2026-11-09

Cross-checked all current queued receive-order interaction Save/Restore cases. The private configured-process service/client test consumes the original live interaction after the image commits and before Restore, then observes the restored copy once and checks for no second result. The public embedded discard case destroys its source registry and restores into a new one; it does not establish how a live configured-process session queue is reconciled.

The current process Restore handler appends the saved interaction to `interactionEvents` without clearing that queue. The Receive Interaction handler pops FIFO entries without identity deduplication. If the original event is still queued at Restore completion, the source path permits both the live and restored copies to be returned. This remains source-derived and untested; it is not a claim about an HLA-wide persistence rule or a confirmed public RTIambassador callback outcome. Added a focused explanation and sequence diagram to the guide. Mermaid CLI 12.0.0 rendered it successfully, and screenshot review found both the tested and untested branches legible.

The ledger now records 501 diagrams (291 flowcharts, 186 sequences, and 24 state diagrams); all 501 line/fence/body identities match. The documentation checker resolves 4,512 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Audit ordinary receive-order interactions in configured-process push mode across Save/Restore. Trace the send route, client-side buffered event queue, state-image capture inventory, and focused tests. Distinguish service-session queues from client callback queues and do not infer parity with polling or embedded behavior. Record unsupported and untested paths without running C++ builds or tests. Continue the broader 2025 source and standards review; exact callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. After publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded.


## 2025 configured-process pushed receive-order interaction Save boundary, increment 340 — 2026-11-09

Traced the configured-process pushed interaction route across the service and client. The service sends a `receive_interaction` frame and does not also put the event in its polling `interactionEvents` queue. The client buffers the frame in `pendingEvents_` and later submits it to the callback bridge. These are separate client-owned and service-owned queues.

The process Save handler snapshots polling interactions only when push delivery is disabled. Its push-mode capture loop serializes pending pushed attribute updates; it does not inspect client FIFO or callback-bridge queues and has no pushed-interaction collection. The Restore replay block appends returned interaction records to `interactionEvents` and does not emit a pushed interaction frame. This narrows the previous polling Restore explanation to its actual mode and does not establish an HLA-wide Save rule.

The existing private process interaction Save/Restore case configures polling and consumes the live event before Restore. The pushed process Save/Restore case asserts exactly-once attribute reflection, not interaction delivery. The public queued-interaction case covers the embedded route. A targeted scan found no focused configured-process test for a pushed ordinary interaction buffered across Save/Restore. This remains an evidence gap; no delivery outcome is inferred. Added a focused section and sequence diagram, rendered with Mermaid CLI 12.0.0 and screenshot-reviewed. Also re-rendered Callback guide diagram 019 after its current source body and line differed from the previous ledger entry, then refreshed its hash, line, and screenshot record.

All 502 diagram line/fence/body identities match (291 flowcharts, 187 sequences, 24 state diagrams). The worktree documentation checker resolves 4,522 unique path/line references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Trace the receiving client's already-buffered pushed interaction while Save/Restore service requests and lifecycle callbacks are interleaved. Follow request-depth suppression, FIFO dispatch, and callback-bridge admission; identify which callback models and existing tests directly cover each boundary. Keep this as configured-process evidence, make no claim about HLA-wide queue persistence, and do not run C++ builds or tests. Continue the broader 2025 source and standards review; exact callback-order and Save/Restore trigger wording remains open pending licensed 2025 text. After publication authorization, inspect all 32 topic pages for visible diagrams and links. Keep 2010 and Requirements Lab excluded.

## 2025 configured-process buffered interaction callback admission at Save/Restore response, increment 341 — 2026-11-09

Traced the client callback path for a pushed receive-order interaction already buffered while a Save or Restore request is outstanding. The request-depth gate prevents background draining while the stream reader continues appending frames. After a correlated response, the requesting thread temporarily suppresses background draining and attempts to take the dispatch lease. If another thread owns that lease, the request-owned drain returns; the wrapper does not wait for that thread to hand off every frame. Therefore no universal callback-before-request-return guarantee follows from the request helper's drain-before-return intent.

For frames that reach the callback bridge, `HLA_IMMEDIATE` and `HLA_EVOKED` determine when the shared dispatcher runs the task. Process receive-order interactions then pass through a separate timing gate: a constrained receiver with asynchronous delivery disabled and no time advance pending can leave the application callback in the bridge's deferred queue. Enabling asynchronous delivery or starting a time advance can release eligible work. This distinction connects the Save/Restore response boundary to the existing configured-process receive-order gate in the Time Management guide; it does not assert normative callback ordering.

Existing pushed interaction tests cover adjacent conditions: with a constrained receiver and pending time advance, the immediate case checks callback count after an unrelated class lookup, while the evoked case explicitly Evokes before checking. Separate Save/Restore tests cover pushed lifecycle callbacks under `HLA_IMMEDIATE` and polling lifecycle callbacks under `HLA_EVOKED`. No focused case combines an ordinary pushed interaction buffered at a Save/Restore response with lifecycle frames in the same client FIFO. The gap remains unverified; no callback order or delivery count is inferred. Added the evidence section and flowchart 039 to the Save/Restore guide; Mermaid CLI 12.0.0 rendering and screenshot review found the current chart readable.

The ledger now contains 503 diagrams (292 flowcharts, 187 sequences, and 24 state diagrams). All 503 line/fence/body identities match. The documentation checker resolves 4,536 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Continue the broader 2025 semantic source-citation review, prioritizing high-interaction timing and lifecycle claims and checking cited tests against their actual assertions. Keep exact normative Save/Restore trigger and failure-reason mapping open until licensed 2025 text is available. After explicit publication authorization, publish the 41-path docs-only candidate and inspect all 32 pages for links and visible Mermaid diagrams. Keep 2010 and Requirements Lab excluded; do not run C++ builds or tests for this documentation pass.

## 2025 configured-process receive-order gate citation audit, increment 342 — 2026-11-09

Cross-checked the configured-process receive-order timing gate in the Time Management guide against current callback-bridge state and the checked-in HLA_IMMEDIATE and HLA_EVOKED cases. The bridge defers a receive-order callback only when the receiver is constrained, asynchronous delivery is off, and no advance is pending. Enabling asynchronous delivery or marking a time advance pending causes eligible deferred work to be resubmitted; successful Disable Time Constrained updates the role state and releases work that becomes eligible. Grant delivery clears the pending flag but is not itself the gate-opening transition.

The guide now points to the actual asynchronous-enable, pending-on, and Disable release branches. Its test notes link to callback-count assertions for initial delivery while TAR is pending, post-grant hold, release after asynchronous enable, and the repeated switch-off/on case. The Save/Restore guide now points to those same release branches when explaining its separate request, dispatcher, and receive-order timing boundaries. No Mermaid body changed; the four later Time Management source-line fields in the ledger were refreshed. No tests or builds were rerun.

All 503 ledger identities match current bodies, headings, types, fences, and source lines. The documentation checker resolves 4,550 unique relative path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%).

**Next handoff:** Next: Audit the additional-module Join transition in the IEEE 1516.1-2025 FOM Module Admission and Composition and Federation/Federate Lifecycle guides. Verify model preparation and commit boundaries, membership/catalog side effects, observer notification/discovery, and what the focused embedded/process tests actually assert. Keep source-derived atomicity and callback ordering separate from normative HLA claims. Do not run C++ builds or tests; keep 2010 and Requirements Lab excluded. After explicit publication authorization, inspect all 32 GitHub pages.


## 2025 additional-module Join source and test citation audit, increment 343 — 2026-11-09

Audited the additional-module Join flow in the FOM Module Admission guide against the current candidate source and focused assertions. Corrected source links that had landed on adjacent functions or unrelated lines, including registry commit and handle reconciliation; corrected test links that had landed on setup, request payloads, or response status instead of state readbacks. The guide distinguishes preparation rejection, the registry commit of membership plus replacement definition/handle directories, and later process setup. It notes that the process path enqueues joined-federate discovery and pending synchronization announcements before forming a successful Join response; this call order does not establish observer callback completion before that response. The source rollback removes member/session/event state without restoring the committed definition. The focused additional-module cases cover pre-commit missing-module rejection and successful object-class lookup, not post-commit failure or observer discovery. These remain source-derived implementation observations, not HLA-wide rules.

No Mermaid body changed. Refreshed the FOM guide diagram source lines; all 12 of its diagram identities match their current bodies and lines. The worktree checker resolves 4,553 unique path/line/guide references across 35 IEEE 1516.1-2025 Markdown files, with zero unresolved. No C++ source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The authenticated GFM result remains historical increment 319 at 498 diagrams; the current candidate remains unpublished.

**Next handoff:** Review the configured-process Restore failure path in the 2025 Save/Restore guide. Trace request validation, restore-participant state, terminal callbacks, replay admission, and rollback across source and focused test assertions. Keep implementation and test evidence separate from normative 1516.1-2025 wording. After explicit publication authorization, inspect all 32 GitHub pages. Keep 2010 and Requirements Lab excluded; do not run C++ builds or tests.

## 2025 configured-process Restore notification-send failure audit, increment 344 — 2026-11-09

Traced the accepted Restore request through registry admission and process
notification projection. The registry creates `restoreOperation` and marks
participants restoring before the process service attempts to enqueue the
accepted Restore notifications. In pushed mode, the service stages notifications
for currently matched sessions and sends them one at a time. A failed send
returns `internalError`; the request handler does not reset the registry
operation, and earlier successful sends are not recalled by this path. A later
connection-loss or resignation transition may still alter federation state.
This is a source-derived implementation boundary, not a normative callback
rule.

Added one source-only flowchart to the Save/Restore guide. The focused public
process tests cover successful one-member pushed admission and missing-image
rejection; they do not inject a failed event send or verify partial multi-member
delivery. Corrected the related test links to the direct callback assertions.
Mermaid CLI 12.0.0 rendered the flowchart; screenshot review found it legible
without clipping.

Rechecked the existing post-application failure discussion: an exception after
snapshot replacement can leave no active Restore operation for the notification
builder, so the failure result has no `federationNotRestored` record and no
rollback. Its successful replay discussion matches the registry's returned
queue records and the private test, which consumes the original live interaction
before Restore. The test does not cover a replay recipient/send failure.

The ledger now records 504 diagrams (293 flowcharts, 187 sequences, and 24
state diagrams), all identity-matched to current bodies and source lines. The
worktree checker resolves 4,555 unique path/line/guide references across 35
2025 Markdown files, with zero unresolved. No C++ source, builds, tests, CI,
2010, or Requirements Lab content changed. Overall progress remains 80%; formal
gates remain 3/4 (75%). The current candidate remains unpublished.

**Next handoff:** Trace the configured-process Restore completion path from
registry state application through terminal-notification enqueue, saved-event
replay, and client error mapping. Focus on recipient/session or send failures
that can return `internalError` after state has changed; distinguish this
source path from successful replay tests and do not imply rollback. Keep 2010
and Requirements Lab excluded; do not run C++ builds or tests. After explicit
publication authorization, inspect all 32 GitHub pages.

## 2025 configured-process Restore replay-failure audit, increment 345 — 2026-11-09

Traced configured-process `federateRestoreComplete` from the registry call
through terminal Restore notification projection, saved receive-order replay,
request-owned pushed-event draining, and public error mapping. The registry
replaces restored state before the service reconstructs saved events. A missing
or mismatched recipient session returns transport `internalError`; a failed
pushed attribute-frame send also returns `internalError` after its pending
update has been inserted. The handler has no rollback for the state replacement
or prior side effects.

The guide now distinguishes restored interactions, which this service block
appends to `interactionEvents`, from restored attribute updates, which use the
polling queue or the pushed attribute path. It also records that pushed Restore
notifications precede replay and that the client drains request-buffered events
before checking the correlated transport status. An enabled `HLA_IMMEDIATE`
dispatcher may enter application code inline; `HLA_EVOKED` may retain the task
for a later Evoke. These are source-derived boundaries, not IEEE callback or
rollback requirements.

Added and locally rendered a Save/Restore flowchart; screenshot inspection
found all nodes and edges legible without clipping. The full ledger audit also
found two already-edited charts in the Callback and TSO Retraction guides with
stale hashes. Both current bodies were rerendered and screenshot-reviewed, and
the ledger entries were reconciled. The successful process
interaction and attribute replay cases remain bounded evidence and do not test
missing sessions or a failed pushed attribute send. The worktree documentation
checker resolves 4,566 unique references across 35 2025 Markdown files, with
zero unresolved. All 505 diagram identities match current bodies and source
lines. No C++ source, builds, tests, CI, 2010, or Requirements Lab content
changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The
candidate remains unpublished.

**Next handoff:** Review how the 2025 Save/Restore and Process Endpoint guides
distinguish durable federation-image state from process-local service queues,
client pushed-event FIFOs, and callback-bridge tasks across reconnect or host-
lifecycle boundaries. Check existing sources and focused tests; do not claim
OS-process restart recovery unless directly covered. Keep 2010 and Requirements
Lab excluded; do not run C++ builds or tests. After explicit publication
authorization, inspect all 32 GitHub pages.

## 2025 process-session resume and Federation Restore boundary review, increment 346 — 2026-11-09

Cross-reviewed the Save/Restore and Process Endpoint explanations of recovery.
The guides now distinguish temporary socket resume against the same live host
and its retained transport history from applying a `FederationStateImage` and
rebuilding only supported registry state. They clarify that the process Save
path admits selected receive-order queue records explicitly, while the client
`pendingEvents_` FIFO and callback-bridge tasks have separate owners and are not
encoded as image state.

Added a reciprocal cross-link and a comparison table. The Process Endpoint
section continues to label its resume source and tests as current-worktree,
private process-transport evidence. Existing recovery cases exercise a live
host; fresh-registry Restore tests cover separate image paths. Neither evidence
set establishes host-process restart recovery or continuation of a pre-restart
transport session. No diagram was added; the 505-entry ledger remains unchanged.

The 2025 documentation link checker resolves 4,566 unique path/line/guide
references across 35 Markdown files, with zero unresolved. No C++ source,
builds, tests, CI, 2010, or Requirements Lab content changed. Overall progress
remains 80%; formal gates remain 3/4 (75%). The candidate remains unpublished.

**Next handoff:** Audit the configured-process terminal-loss path across the
Process Endpoint and Federation/Federate Lifecycle guides. Trace transport
resume exhaustion and host session purge through registry `connectionLost`,
session detach, client error reporting, and any public ambassador callback.
Separate the private transport result from HLA lifecycle behavior and use only
existing focused tests. Keep current-worktree-only evidence labeled; do not run
C++ builds or tests or touch 2010 or Requirements Lab. After explicit
publication authorization, inspect all 32 GitHub pages.

## 2025 configured-process terminal ConnectionLost boundary review, increment 347 — 2026-11-09

Audited terminal configured-process ConnectionLost across the Process Endpoint
and Federation/Federate Lifecycle guides. A normal serving-loop exit marks the
host session inactive but retains its server record so the same session can
Resume until `RTI_TIMEOUT_PURGE`; it does not immediately remove federation
membership. Rejected Resume or serving errors take terminal cleanup paths.
Purge and shutdown also attempt service `connectionLost`, with distinct detach
behavior when cleanup fails.

The lifecycle guide now separates the host-session state machine from the
service-to-registry cleanup flow. It links the client-local public callback to
the Process Endpoint recovery timeline and explains that client Resume exhaustion
and host registry cleanup are driven by separate events and timers. The two
revised diagrams rendered with Mermaid CLI 12.0.0 and passed screenshot review.

Existing private recovery and host-purge cases cover their paths separately; a
public coordinator-stop case observes local callbacks after shutdown, but does
not test callback ordering against the purge deadline or registry projection
failure. No C++ source, builds, tests, CI, 2010, or Requirements Lab content
changed during this increment. The ledger matches all 506 diagrams (294
flowcharts, 187 sequences, 25 state diagrams). The worktree documentation
checker resolves 4,566 unique references across 35 Markdown files, with zero
unresolved. Overall progress remains 80%; formal gates remain 3/4 (75%). The
candidate remains unpublished.

**Next handoff:** Audit 2025 process ConnectionLost observer effects across the
Federation/Federate Lifecycle, Federation MOM, and Joined-Federate MOM guides.
Trace forced registry removal against survivor `HLAfederate` object observations
and the separate `HLAreportFederateLost` interaction; distinguish registry
state, observer callbacks, and report delivery using existing focused tests.
Keep current-worktree-only evidence labeled; do not run C++ builds or tests or
touch 2010 or Requirements Lab. After explicit publication authorization,
inspect all 32 GitHub pages.

## 2025 process ConnectionLost observer reconciliation, increment 348 — 2026-11-09

Cross-reviewed the Federation/Federate Lifecycle, Federation MOM, and
Joined-Federate MOM explanations of process `ConnectionLost`. The Lifecycle
guide now places the registry state change beside four separate projections:
the federation-scoped `HLAfederatesInFederation` reflection, removal of the
survivor-visible `HLAfederate` object, the `HLAreportFederateLost` interaction,
and the failed member's local `connectionLost` callback. It explains that
these paths have different recipient sets and that source submission order is
not a public callback-order guarantee. The process sequence now names the two
survivor routes; both MOM guides link to the summary.

Corrected stale Federation MOM wording that said no process membership-list
reflection test existed. The current worktree process TCK source now contains a
post-loss reflection assertion and a survivor-only list query, but this
increment did not inspect a runtime result. The process service's inspected
loss dispatcher has no explicit enqueue for that conditional update, so the
assertion remains unverified. The embedded adapter does enqueue its update; the
1516.2 MIM's Join/Resign Conditional wording does not settle the forced-loss
normative question. The focused report-order case covers report-before-
`DELETE_OBJECTS` removal of an application object, not removal of the RTI-owned
`HLAfederate` object. The Joined-Federate MOM lifecycle case covers explicit
Resign, not forced loss.

Revised the process ConnectionLost sequence and Federation MOM membership-list
chart. Both rendered with Mermaid CLI 12.0.0 and passed screenshot review.
The ledger still matches all 506 diagrams (294 flowcharts, 187 sequences, 25
state diagrams). The worktree checker resolves 4,568 unique references across
35 Markdown files, with zero unresolved. No C++ source, builds, tests, CI,
2010, or Requirements Lab content changed. Overall progress remains 80%; formal
gates remain 3/4 (75%). The candidate remains unpublished.

**Next handoff:** Review the unresolved 2025 Federation MOM membership-list
behavior on forced ConnectionLost. Compare the 1516.2 MIM's Conditional trigger
for Join/Resign with the current process loss dispatch and the worktree TCK
assertion for a post-loss reflection. Check for an existing runtime result
before drawing a conclusion; do not run builds or tests in this documentation
pass. Keep the normative obligation, source path, and test assertion separate.
Keep 2010 and Requirements Lab excluded. After explicit publication
authorization, inspect all 32 GitHub pages.

## 2025 forced ConnectionLost membership-list source trace, increment 349 — 2026-11-09

Traced the process loss path from `ProcessFederationService::connectionLost`
through `EmbeddedFederationRegistry::connectionLost` to the shared
`resignLocked(..., forcedConnectionLoss=true)` transition. The registry result
contains resignation cleanup records but no Federation-object membership-list
update record; the process loss dispatcher does not enqueue a conditional
`HLAfederatesInFederation` reflection. The embedded adapter explicitly queues
that update after the transition. The 1516.2 MIM's exact update condition is
“Federate joins or resigns”; the source trace alone cannot determine whether
forced `ConnectionLost` is normatively included in “resigns.”

The current worktree process TCK waits for the post-loss MOM reflection and
then makes a separate `listFederationExecutionMembers` query. No saved CTest
result for that assertion was present in the worktree's `out` directory. The
guides now distinguish the source-level process projection gap, the test
expectation, the separate public query, and the open normative interpretation.
No C++ source or test files were changed by this increment, and no builds or
tests were run. The 2025 diagram ledger still matches all 506 identities; the
worktree link checker resolves 4,575 unique references across 35 Markdown
files. No 2010 or Requirements Lab content changed in this increment. Overall
progress remains 80%; formal gates remain 3/4 (75%). The candidate remains
unpublished.

**Next handoff:** Continue the 2025 semantic source-citation review with the
open Save/Restore failure mapping and callback-order wording. Use the pinned
licensed 1516.1-2025 text when available; keep standard requirements, Umbra
source behavior, and test evidence separate. Do not build or run C++ tests in
this documentation pass. Keep the ConnectionLost process reflection labeled
as a source gap until an existing runtime result is reviewed. Keep 2010 and
Requirements Lab excluded; after explicit publication authorization, inspect
all 32 GitHub pages.

## 2025 Save/Restore failure reasons and callback ordering, increment 350 — 2026-11-09

Reviewed the Save failure-reason section against its current source selectors
and focused evidence. The checked-in 2025 package manifest identifies the API
header tree; licensed clause prose is absent from this checkout. The guide keeps
the reason table implementation-derived and leaves the normative Save/Restore
trigger mapping open. The current source scan has no selector for
`RTI_DETECTED_FAILURE_DURING_SAVE` or `SAVE_TIME_CANNOT_BE_HONORED`; their
normative triggers are not inferred from the enum names.

The Save sequence now states inside the chart that P1/P2 arrow placement is
illustrative and makes no cross-federate API callback-order claim. The Restore
sequence already separates per-member callback lanes and warns that they do not
establish a global wall-clock interleaving. Mermaid CLI 12.0.0 rendered the Save
sequence and the current callback-exception sequence in the Callback and
Service Ordering guide; both were visually reviewed. The second render exposed
one stale body hash in the ledger, which was refreshed without changing that
guide's content. The ledger now matches all 506 diagrams, including source
lines, headings, types, fences, and body hashes. The worktree checker resolves
4,575 unique references across 35 2025 Markdown files.

No C++ source or test files changed, and no builds or tests were run. 2010 and
Requirements Lab were excluded. Overall progress remains 80%; formal gates
remain 3/4 (75%). The candidate remains unpublished and GitHub page rendering
remains unverified.

**Next handoff:** Audit the 2025 Save/time-management cross-guide seam: trace
the pending-save initiation boundary and queued or in-transit TSO gate against
current source and existing focused tests, then align the two guides where
their evidence supports it. Keep standard requirements separate because the
licensed clause text is absent. Do not build or run C++ tests in this
documentation pass. Preserve the process ConnectionLost projection as a source
gap, keep 2010 and Requirements Lab excluded, and inspect all 32 GitHub pages
after explicit publication authorization.


## 2025 Save/time-management cross-guide seam, increment 351 — 2026-11-09

Traced timed Save admission from request validation and replacement through
the embedded per-constrained-member callback gate. Each constrained member
reaches its own time-advance checkpoint; the current mode-specific predicate
checks its requested or actual grant against the Save time, and queued or
in-transit TSO at or before that time blocks admission. Before the first
SaveOperation opens, every other constrained member must have a qualifying
advance queued. The remaining non-constrained members are instructed after
the constrained initiations. These are embedded source observations, not
normative callback-order rules or evidence of configured-process parity.

The Save/Restore guide now explains this boundary and includes a focused
per-member flowchart. The Time Management guide links back to the details.
Mermaid CLI 12.0.0 rendered the new flowchart and screenshot review confirmed
it. The ledger matches all 507 current 2025 diagrams: 295 flowcharts, 187
sequences, and 25 state diagrams. The worktree link checker resolves 4,588
unique references across 35 Markdown files with zero unresolved.

No C++ source or test changes were made in this increment, and no builds or
tests were run. 2010 and Requirements Lab were excluded. Overall progress
remains 80%; formal gates remain 3/4 (75%). The candidate remains unpublished.

**Next handoff:** Audit the 2025 FOM module admission/composition boundary:
trace standalone module validation, module merging and duplicate-conflict
behavior, composed-model validation, and existing focused tests. Distinguish
normative requirements, Umbra behavior, and test evidence. Reuse existing
evidence; do not build or run C++ tests. Preserve the process
ConnectionLost membership-reflection gap, keep 2010 and Requirements Lab
excluded, and inspect all 32 GitHub pages after explicit publication
authorization.


## 2025 FOM module admission/composition boundary audit, increment 352 — 2026-11-09

Compared the guide's three validity stages with the current coordinator and
composer: per-module validation against DIF, ordered full-set composition with
table-specific merge and post-merge reference resolution, and RTI-facing FDD
materialization followed by FDD-schema validation. The source confirms that
additional-module Join revalidates retained and new modules, and that the
composer rejects mixed standard editions or source-compatibility profiles.
The guide's selected tests cover representative valid, invalid, duplicate and
conflict, FDD-rejection, and retry paths; they do not exhaust every Annex C
merge rule.

The audit found one nuance worth making explicit. PrevalidatedFomModule
retains validated XML for later MOM reporting, but that stored contents field
is not used as composition input. The composer reopens sourcePath and validates
the second read. A file changed between the coordinator's validation and
composition can therefore affect the prepared model; no focused test for that
mutation window was found. The guide labels this as current Umbra behavior,
not an HLA rule.

Twelve FOM-guide diagram source lines were reconciled after the prose change.
No Mermaid body or hash changed, so no diagram rerender was needed. The ledger
matches all 507 current diagrams (295 flowcharts, 187 sequences, 25 state
diagrams). The worktree link checker resolves 4,591 unique references across
35 Markdown files with zero unresolved.

No C++ source/test files changed, and no builds or tests were run. No 2010 or
Requirements Lab content changed. Overall progress remains 80%; formal gates
remain 3/4 (75%). The candidate remains unpublished.

**Next handoff:** Audit the 2025 Save/Restore failure-reason triggers using a
bounded, read-only lookup of the available normative text and public API
declarations, then compare each reason with current source selectors and
existing focused evidence. Update the Umbra guide only where clause-level
evidence supports a mapping; otherwise retain the explicit unresolved status.
Do not edit or resynchronize the Requirements Lab or build/run C++ tests.
Keep the 2010 and 2025 streams separate, preserve the process
ConnectionLost membership-reflection gap, and inspect all 32 GitHub pages
after explicit publication authorization.


## 2025 Save/Restore failure-reason source audit, increment 353 — 2026-11-09

An exact scan of current C++ source and tests found
RTI_DETECTED_FAILURE_DURING_SAVE and SAVE_TIME_CANNOT_BE_HONORED only in
formatMomSaveFailureReason, which converts a supplied enum to MOM text. No
focused test references either value. The Save guide now distinguishes this
formatter from runtime reason selection and leaves both normative triggers
unmapped. RTI_DETECTED_FAILURE_DURING_RESTORE remains without a direct source
selector or focused test reference.

The checked-in 2025 package manifest identifies the API header tree; licensed
clause prose is not available in this checkout. The [official IEEE
listing](https://standards.ieee.org/ieee/1516.1/6688/) presents purchase access via subscription. The available
evidence does not establish the missing trigger-to-reason clauses, so no
normative mapping was inferred. No Requirements Lab edit or resynchronization
was made.

The worktree link checker resolves 4,593 unique references across 35 Markdown
files with zero unresolved. No diagram bodies changed; all 507 ledger
identities remain aligned. No C++ source/test changes were made, and no builds
or tests were run. No 2010 or Requirements Lab content changed. Overall
progress remains 80%; formal gates remain 3/4 (75%). The candidate remains
unpublished.

**Next handoff:** Audit the current 2025 forced ConnectionLost observer path
across the Federation/Federate Lifecycle, Federation MOM, and Joined-Federate
MOM guides. Trace registry cleanup, process event dispatch, the conditional
HLAfederatesInFederation membership-list reflection, survivor HLAfederate
removal, and current focused test assertions. Keep those observable
projections distinct and do not infer whether forced ConnectionLost is
normatively included in resign. Update Umbra guides only where current
evidence requires it; do not edit or resynchronize the Requirements Lab,
build/run C++ tests, or merge the 2010 and 2025 streams. Inspect all 32
GitHub pages after explicit publication authorization.

## 2025 forced ConnectionLost observer-path and candidate GFM review, increment 354 — 2026-11-09

Rechecked the configured-process loss dispatcher, embedded adapter queue,
shared registry transition, and focused membership tests against the three
current observer-path guides. The embedded path queues the conditional
HLAfederatesInFederation update after forced loss. The configured-process
dispatcher projects cleanup events and an eligible Federate-Lost report but
has no explicit enqueue for that Federation membership-list reflection. The
current worktree process TCK source contains a post-loss reflection assertion,
but no saved runtime result was reviewed. The focused Catch2 membership case
covers Join and Resign, not forced loss; no focused case asserts survivor
HLAfederate object removal after forced loss. The guides already preserve
these behavior and evidence limits. No claim was made that forced loss is
normatively a Resign.

Two source citations needed candidate-boundary correction: the process TCK
assertion and member-list query are now explicitly described as current
worktree-only evidence without source links, and the process-client dispatch
pointer targets the matching line in the pinned candidate base. The Callback
guide's current diagram 19 body no longer matched its ledger hash. Mermaid
CLI 12.0.0 rendered it and screenshot review found no clipping; its current
hash and review date are recorded. Twenty-six Federation MOM diagram line
locations were refreshed, with the body hashes unchanged.

The 5,329-line progress archive exceeded the GitHub Markdown API's 400 KB
request limit. It is now organized as an index plus date-based archives at the
existing increment boundaries, with every previous dated entry preserved.
The authenticated GFM pass checks 38 pages (32 topic guides and six support
pages): all 38 return nonempty HTML, 507/507 source Mermaid fences produce
Mermaid containers, 1,028 headings and 7,992 links are present, and there
are no parser or diagram-count mismatches. The source-line checker resolves
all 4,591 references across the worktree, pinned candidate base, and primary
source revision, with zero unresolved. GFM parsing and container registration
do not verify visible browser hydration or published-page links.

No C++ source/test files changed, and no builds or tests were run. No 2010 or
Requirements Lab content changed. Overall progress remains 80%; formal gates
remain 3/4 (75%). The 44-path candidate remains unpublished, with unrelated
non-documentation changes preserved outside the selection.

**Next handoff:** Publish the 44-path 2025 documentation candidate on its
dedicated branch, then inspect all 32 GitHub guide pages for visible diagrams
and link destinations. Record the published revision and per-page outcomes in
the render audit. Preserve unrelated non-documentation work and keep 2010 and
Requirements Lab paths excluded.

## 2025 published GitHub page review, increment 355 — 2026-11-09

Published the 44-path 2025 documentation candidate on `feature/hla-2025-docs-candidate-20261107` at
[adafb10c](https://github.com/sheepfling/umbra/commit/adafb10c37b6aad8f798e49ca7007abeaaeb66fc). Headless Chromium inspected all 32 guide page
DOMs: every page title and branch matched, and the page-level Mermaid
container counts matched all 507 ledger entries. All 507 containers retained
GitHub's `Loading` placeholders, so no visible Mermaid hydration was observed
in this headless review. The pages exposed 11,305 anchor elements, including
7,239 links to the candidate branch and 6,080 source-line anchors; no rendered
`href` began with `./` or `../`. This checked rendered link markup, not the
click-through HTTP status of every destination.

The increment-355 authenticated GFM pass returned 38/38 nonempty responses, 507/507 Mermaid containers, 1,029 headings, and 7,994 links with zero failures. It establishes parsing and container registration, not visible hydration. The 4,591
source-line references resolve at the worktree, pinned candidate base, and
primary revision. The unrelated 48 non-documentation changes remain outside
the published candidate. No C++ source or tests changed, and no builds or tests
were run. No 2010 or Requirements Lab content changed. Overall progress remains
80%; formal gates remain 3/4 (75%) because visible published-page rendering is
not verified.

**Next handoff:** Determine whether an interactive GitHub browser session
hydrates the Mermaid containers and inspect the resulting diagrams. Keep the
current headless limitation explicit; continue the source-coupled Save/Restore
semantic review and do not infer normative failure triggers without the
authoritative clause text.

## 2025 published GitHub Mermaid hydration review, increment 356 — 2026-11-09

After increment 355's headless page inspection reported GitHub `Loading`
placeholders, a Puppeteer-controlled Chromium pass scrolled each published
Mermaid container into view to trigger GitHub's lazy hydration. On
`feature/hla-2025-docs-candidate-20261107` at
[`83958e45`](https://github.com/sheepfling/umbra/commit/83958e45037b57f3f9b9561fd054f4795d9e2b88),
all 32 guide pages loaded and matched their expected diagram counts: 507/507
containers produced SVGs, all 507 Mermaid render requests returned HTTP 200,
and no render requests failed. The earlier placeholder observation was caused
by not scrolling the containers into view.

A representative hydrated diagram on the Attribute Scope Advisory guide was
visually inspected in the GitHub page viewport; labels, connectors, and branches
were legible. This is not an individual screenshot review of all hosted diagrams.
Existing GFM parsing and 4,591 source-line reference checks remain current;
click-through HTTP status for every link was not checked. No C++ source or tests
changed, and no builds or tests were run. No 2010 or Requirements Lab content
changed. Overall progress remains 80%; all four formal gates are now complete
(4/4, 100%).

**Next handoff:** Continue the 2025 source-coupled Save/Restore semantic review.
Keep normative trigger and failure claims tied to authoritative IEEE
1516.1-2025 text; record any unavailable clause mapping as an explicit gap.
Preserve unrelated work and keep 2010 and Requirements Lab paths excluded.

## 2025 Save failure-reason source and standards-access audit, increment 357 — 2026-11-09

Refreshed the `SaveFailureReason` source table against the pinned API enum, current tracked selector files, and focused test references. Four of the six API values have current Save-path selectors. Exact-value search found no Save-path selector or direct `cpp/tests` reference for `RTI_DETECTED_FAILURE_DURING_SAVE` and `SAVE_TIME_CANNOT_BE_HONORED`; their only `cpp/src` occurrences are MOM reason-formatting cases. The guide now states that this is current Umbra evidence and does not prove those normative conditions impossible.

The official IEEE record lists purchase and subscription access. Inspection of its public download ZIP found one nested API XML archive containing 284 files: 44 C++ headers, 230 Java API files, four XML files, three protobuf files, and three XSDs; no PDF, DOC/DOCX, RTF, HTML, or plain-text standard was present. The full clause text therefore remains unavailable in this checkout, so the exact Save/Restore failure-reason mapping stays open. The link checker resolves all 4,591 unique source-line references across 38 2025 Markdown files against HEAD `7134145e`. No C++ source or tests changed; no builds or tests were run. No 2010 or Requirements Lab content changed. Overall progress remains 80%; all four formal gates remain complete (4/4, 100%).

**Next handoff:** Trace the timestamped 2025 Save request from admission through its pending-time scheduler and initiation callback, then compare the focused cases for what happens when the requested time cannot be reached. Keep `SAVE_TIME_CANNOT_BE_HONORED` as an unselected API value in current source; do not infer its normative trigger without authoritative clause text. Preserve unrelated work and keep 2010 and Requirements Lab paths excluded.

## 2025 non-timing synchronization participant-set review, increment 358 — 2026-11-09

Reviewed the 2025 federation synchronization late-join and resignation paths in the committed registry and compared the cited focused tests. For an omitted-set point, a successful late Join adds the new federate ID to the participant and announcement sets; its achievement result is independent because results are stored by ID. An explicit-set point does not expand. Resign removes the departing ID from all three sets, then tests completion against the remaining participants and forms the failed-to-synchronize set from their recorded false results. The focused registry case compares explicit-set with omitted-set expansion but has no resignation or same-name rejoin. A separate public service-report case observes an achieved survivor receiving Federation Synchronized with an empty failed set after the unachieved participant Resigns. No focused case combines same-name rejoin with Resign or has the departing member carry a false result. The existing Federation Synchronization guide already explains and diagrams these boundaries, and labels the combined path source-derived; no guide edit or new diagram was needed.

A bounded non-timing inventory found 32 current 2025 topic guides and a roadmap API-to-guide map covering the principal public service and callback families. The current documentation focus is therefore specific reader and evidence gaps rather than adding files by API count. The prior timestamped Save handoff is already covered by the Save guide's admission flow and focused TSO/FQR boundary cases; `SAVE_TIME_CANNOT_BE_HONORED` remains unselected in source and unmapped normatively. The source-line checker result remains 4,591/4,591 across 38 2025 Markdown files against HEAD `d77a1687`. No C++ source or tests changed; no builds or tests were run. No 2010 or Requirements Lab content changed. Overall progress remains 80%; all four formal gates remain complete (4/4, 100%).

**Next handoff:** Continue beyond timing with the 2025 Attribute Value Update Request flow. Reconcile the currently uncommitted shared-worktree edits in `federation_registry_attribute_update_recipients.cpp` and `federation_registry_attribute_value_update_requests.cpp`; preserve them and keep durable source claims tied to the committed snapshot actually reviewed. Then review the HLA-facing separation between provider solicitation, a later Update, and receiver reflection, and trace class-request ownership from provider planning through per-group persistence and callback entry. Compare focused evidence for ownership changes in those windows, multi-provider fan-out, and requester receiver eligibility. Keep source-derived suppression or partial-persistence behavior distinct from normative HLA requirements. Do not infer direct response entitlement or callback ordering without authoritative clause text. Keep 2010 and Requirements Lab paths excluded.

## 2025 Attribute Value Update Request source, evidence, and receiver-boundary review, increment 359 — 2026-11-09

Reviewed the current RAVU guide against the committed candidate and the active worktree. The guide already separates explicit provider solicitation, the provider's later Update call, and receiver eligibility for the resulting reflection. Its class-request trace groups current owned attributes by object/provider, excludes requester-owned work, and describes the persistence and callback-entry ownership checks without claiming retargeting or normative callback order. The guide's diagrams and evidence limits remain accurate; no additional chart or prose was needed.

The committed candidate at `ab6e9bc7` still performs regional class-request dimension preflight at the requested-class scope. The current worktree resolves the available dimension context per requested attribute, then validates the supplied region against that context. The hosted source-dependency audit records the modified registry files and current-only line references; the guide's `[^worktree-source]` note correctly keeps those claims distinct from hosted source. The portable inherited-`BaseValue` TCK scenario and contract exercise a Y-only regional request and expect `InvalidRegionContext`; their recorded result is 4/4 across `HLA_EVOKED` and `HLA_IMMEDIATE`. That establishes the public regional request-context boundary, not multi-provider fan-out or process/embedded parity.

The focused class-request cases still cover one provider across multiple objects, or one provider/object/attribute for the regional response. No focused case covers same-object attributes split across providers, an ownership change during class planning/persistence/callback entry, or a completed transfer between Provide and the provider's response. The DSE late-subscription case has no RAVU request. Direct requester entitlement under §6.21 remains unresolved because the full normative clause text is not available in this checkout; the guide does not infer it from Umbra's current receiver path.

The source-line checker resolves 4,591/4,591 references across 38 IEEE 1516.1-2025 Markdown files against HEAD `ab6e9bc7`. This documentation increment changed no C++ source or test files; no build, test, or CI was run. No 2010 or Requirements Lab paths were edited in this increment. Overall progress remains 80%; all four formal gates remain complete (4/4, 100%).

**Next handoff:** Trace the 2025 timestamped Save request from admission through pending-time scheduling and the Initiate Save callback. Compare source with the existing focused results to establish what happens when the requested time is not reached and whether Umbra selects `SAVE_TIME_CANNOT_BE_HONORED`. Keep the normative trigger unresolved without the authoritative clause text; do not build or rerun tests. Preserve unrelated work and keep 2010 and Requirements Lab paths excluded.

## 2025 timed Save pending-state review, increment 360 — 2026-11-09

Traced the 2025 timestamped Save request from registry admission to the
constrained-member callback boundary. The registry stores one pending label
and time. If the Save time has not been reached at an eligible member boundary,
or a qualifying advance/TSO condition is absent, it leaves that request pending
without opening the `SaveOperation`. A later timed request replaces the pending
label and time. When the constrained members reach qualifying checkpoints and
the admission gates pass, the dispatcher delivers `initiateFederateSave` after
eligible TSO payloads and before that member's grant
([pending slot](../../cpp/src/internal/federation/federation_registry_save_control.cpp#L525),
[checkpoint admission](../../cpp/src/internal/federation/federation_registry_save_control.cpp#L715),
[pending clear after admission](../../cpp/src/internal/federation/federation_registry_save_control.cpp#L778),
[callback before grant](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_advance_dispatch.cpp#L800)).

The focused [embedded case](../../cpp/tests/ieee1516_2025_federation_management_save_restore_catch2.cpp#L65)
requests time 5 and then 7 before either request reaches initiation; it later
observes only the final label/time. The configured-process
[replacement case](../../cpp/tests/ieee1516_2025_connection_process_federation_save_timed_replacement_catch2.cpp#L703)
checks the same boundary separately. Neither test leaves the Save time
unreachable or asserts a terminal failure. The current-source search found no
`SAVE_TIME_CANNOT_BE_HONORED` Save-path selector or test reference; the only
`cpp/src` occurrence is the [MOM reason formatter](../../cpp/src/internal/observability/mom_service_report_encoding.cpp#L549).
This pending-state observation does not establish the normative failure
trigger, which remains open without the authoritative clause text.

The Save guide's existing admission flowchart now shows request retention,
replacement, and no failure selection at an ineligible checkpoint. Mermaid
CLI 12.0.0 rendered the revised chart and its screenshot was visually reviewed.
The visual ledger now matches all 42 Save/Restore guide diagram source lines
and body hashes, including the changed chart. The worktree source-line checker
resolves 4,600/4,600 references across 38 IEEE 1516.1-2025 Markdown files. No
C++ source or test files changed; no builds, tests, or CI were run. No 2010 or
Requirements Lab content changed. Overall progress remains 80%; all four
formal gates remain complete (4/4, 100%).

**Next handoff:** Audit the non-timing 2025 attribute-ownership edge for a
multi-attribute partial If-Wanted transfer followed by cancellation. Compare
the registry's retained and removed attribute subsets and planned callbacks
with the focused one-attribute transfer case. Update the existing ownership
flow only if it obscures that state boundary; label source-derived behavior and
the missing multi-attribute test without inferring a normative callback order.
Keep 2010 and Requirements Lab paths excluded.
