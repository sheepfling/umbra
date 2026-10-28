# IEEE 1516.1-2025 Documentation Roadmap

This roadmap tracks the remaining work to make Umbra's 2025 HLA discoveries
clear and durable for readers. The guides lead with HLA concepts; implementation
and focused-test evidence is included only to explain an observable boundary or
to show what the current evidence does—and does not—establish.

## Current baseline

As of 2026-10-17, the 2025 topic set contains 31 guides and 340 Mermaid
diagrams (199 flowcharts, 118 sequences, and 23 state diagrams). This is an
inventory, not a completion target. A 2025-only syntax pass accepted the 336
diagrams that existed at that sweep; later source-bounded and revised charts
have focused syntax or render checks.
These checks establish syntax only—not rendering, visual layout, or GitHub's
renderer. Earlier selected visual-review totals remain 15 local render/
screenshot reviews and 39 local visual reviews; they predate recent edits and
have not been recomputed.

Thirteen diagrams were already in the visual queue. This 2025-only audit added
one confirmed-unrendered service-report chart, bringing that queue to fourteen,
and opened a separate guide-level chart-status reconciliation. The revised
DDM region-commit/discovery sequence below now needs fresh visual review, so
fifteen charts were pending; the reporting-guide reconciliation remained separate.
The 2025 subscription-state chart was then revised to show the embedded
discovery-callback reentrancy checkpoint, bringing the current visual queue to
sixteen charts plus the separate reporting-guide reconciliation.

The existing 2025 order/transportation pending-state chart now also distinguishes
unconditional divestiture removing the last versus only part of a pending
attribute-change request. Mermaid syntax and the added source/test line anchors
pass. Fresh render/screenshot review is pending, bringing the current queue to seventeen
charts at that checkpoint while the reporting-guide reconciliation remained
open; the later reconciliation below raises the itemized queue to nineteen.

After the latest screenshot reviews recorded below, five chart items
remain in the queue. The separate reporting-guide status reconciliation is
closed.

- Cross-federate pending TAR released after a regulating peer advances in the
  [time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md). It passed a
  syntax-only Mermaid core parse, but is not yet rendered or visually reviewed.
- Restore-time state versus live callback queues in the
  [time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md). The new flow
  separates restored time/TSO ledgers from already-admitted callback work and
  shows pending-grant rebinding; focused Mermaid core parsing passed. Render
  and visual review are pending. No general deferred-RECEIVE replay or
  host-process restart claim is made.
- Composite shape, cursor, and padding in the
  [Data Element guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md#composite-values-shape-cursor-and-alignment).
  The revised chart separates the variable-array prefix pad from padding
  between children, makes the composite-relative cursor origin explicit, and
  leaves child byte order with each child type. Focused Mermaid core syntax
  parsing passed; render and visual review are pending.
- Factory selection, value-copy, and opaque logical-time encoding in the
  [Data Element guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md#logical-time-is-factory-defined-not-assumed-to-be-be-or-le).
  The revised sequence distinguishes initial/zero creation, a compatible
  caller-value copy, value-owned bytes, and cursor-based decode. Mermaid core
  syntax parsing passed; render and visual review are pending.
- Configured-process regional Send Interaction With Regions reporting flow in
  the [service-invocation reporting guide](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#process-regional-sends-separate-public-preflight-from-server-rejection).
  The 2026-10-16 progress entry says this chart remains unrendered.
The Service Invocation Reporting review-ledger reconciliation is complete.
The current guide has eleven diagrams: the eight-chart 2026-10-14 review set
includes seven unchanged diagrams and the report-destination chart whose
callback-recheck label has since changed; the ordinary Send Interaction
failure chart has a separate 2026-10-16 visual review. The report-destination
and configured-process recipient-snapshot charts now have fresh local render
and screenshot review. The configured-process regional-send chart remains
unrendered and is the only reporting chart still pending. This closes the
status reconciliation, not the guide's visual review.

## Completed visual review batch (2026-10-17)

Twenty-one queued 2025 charts rendered to local PNGs with Mermaid CLI 12.0.0 and were
visually inspected: the transportation pending-state chart in the
[order/transportation guide](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md#2-transportation-change-requests-have-a-pending-lifecycle),
the [report-destination chart](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#a-reportable-service-call-selects-one-destination),
the [configured-process recipient-snapshot chart](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#configured-process-recipient-snapshot-and-callback-boundary),
the [process stale-work chart](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md#process-event-transport-has-two-stale-work-checkpoints),
the [regional request Save/Restore chart](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#process-request-metadata-is-separate-from-notification-delivery),
the [configured-process synchronization callback-exception chart](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#in-the-process-push-path-callback-failure-follows-barrier-closure),
the [configured-process Delete/Retract ownership-buffer chart](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#configured-process-callback-buffers-are-a-separate-boundary),
the [Connect-through-Disconnect credential-lifetime chart](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md#disconnect-ends-this-source-level-credential-scope),
and the Federation MOM guide's [execution-scoped object lifetime chart](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#one-object-belongs-to-the-federation-execution).
The forced-connection-loss
[membership-list flow](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#forced-connection-loss-has-its-own-membership-list-path)
keeps the embedded enqueue path, inspected process-dispatch boundary, and untested outcomes distinct. The Federation MOM
[synchronization list/status query flow](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#federation-synchronization-queries-active-point-list-and-roster-status)
is exceptionally tall, but preserves the distinction between the two MIM requests, embedded status mapping, and the
unverified configured-process result. The forced-loss Push topic is represented by two charts: the
[configured-process Push removal sequence](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#configured-process-push-crosses-the-registry-boundary-before-sending)
and
[independent host/local loss sequence](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#transport-closure-can-start-another-process-loss-pass).
The forced-loss
[observer route-availability chart](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#forced-loss-can-skip-an-observer-whose-callback-route-is-unavailable)
was revised after checking the registry callback-entry transition: it loops through remaining planned removals before
ending in the skipped-known-observer state. The final saved chart rendered with legible branches and no clipping.
The
[object-existence, receiver-knowledge, DDM, and removal chart](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#object-existence-receiver-knowledge-and-ddm-eligibility-are-separate-states)
was also rendered and inspected. It keeps the known-object ledger separate from current update eligibility and shows
deletion planning without a current-overlap recheck. The regional and deletion tests remain component evidence; no
focused test combines disjointness followed by removal.
The [receive-order value-routing sequence](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#receive-order-value-routing-after-regional-overlap-changes)
was also rendered and inspected. Its overlap phases and callback-queue events
are legible and unclipped; the switch annotation clearly changes sent-region
callback metadata without changing regional eligibility. Discovery and the
latest-value reflection are intentionally grouped because the focused case
stores those callbacks separately and does not establish their relative order.
The final disjoint phase asserts no reflection but does not query whether the
object remains known.
The [interaction-class ancestry and declaration-lookup flow](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#interaction-class-ancestry-available-context-and-declaration-lookup)
was rendered and inspected. Although tall, its separate subscription-declaration
lane and per-receiver candidate walk are traceable; labels are readable without
clipping. The chart preserves the implemented ordinary/default-before-regional
candidate order and the explicit parent step. Its cited tests do not cover
ancestor-only dimensions, parent regional subscription with descendant send,
or configured-process parity.
The [ordinary/default versus explicit regional interaction flow](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#interactions-explicit-and-default-region-paths)
was rendered and inspected. The three invocation forms remain distinct, and
the shared ordinary/default-first eligibility check, regional fallback, and
overlap decision are easy to follow. Labels are readable without clipping;
the empty regional set clearly ends at a no-send result without default-region
fallback. Its 2025 evidence remains embedded and scenario-bounded, not a
normative §9.1.5 interpretation or process-parity claim.
The [embedded discovery-reentrancy flow](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#embedded-discovery-reentrancy-precedes-initial-value-planning)
was also rendered and inspected. It is very tall but remains traceable and
unclipped: the new-discovery callback and post-callback current-state recheck
are distinct from the already-known advisory paths and the separate Auto
Provide path. No cited test mutates RTI state from inside discovery, so the
reentrancy fence remains source-derived rather than test-proven.
The [commit-triggered discovery sequence](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-committed-range-change-can-discover-an-existing-object)
was also rendered and inspected. The pending-bounds update, commit that installs
the new snapshot and plans discovery, callback queue, and explicit `evokeCallback`
are visually distinct and unclipped. The cited test does not drain or assert
callbacks between `setRangeBounds` and `commitRegionModifications`; the chart
does not claim that intermediate timing.
The [bounded Evoke Multiple wait-window flow](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#the-bounded-drain-uses-elapsed-time-not-hla-logical-time)
was rendered and inspected. Though tall, the enabled/disabled, queued/empty,
minimum-deadline, and post-callback maximum-deadline branches remain traceable
and unclipped. The diagram keeps the steady-clock service window separate from
HLA logical time; it reflects the current dispatcher, not a complete §10.58
interpretation. The cited test does not cover empty-queue waiting, late arrival,
maximum overshoot, or the public adapter.
Text and branches were legible without clipping. The transportation chart's
three long pending-state labels were shortened, then the final chart was
re-rendered and inspected; its self-loop labels remain compact but readable.
The reporting, process, and regional Save/Restore diagrams are tall, but their
decision paths remain traceable at the rendered size. The synchronization
exception chart is tall but traceable. The Delete/Retract chart was changed
from left-to-right to top-to-bottom so its pull, push, and deletion lanes remain
legible at the rendered width; its final saved form was re-rendered and
inspected. The credential-lifetime chart is tall but unclipped. The
synchronization-query chart is exceptionally tall, but its paths remain
traceable. The former nine-participant forced-loss sequence compressed its
labels, so it was split into two focused sequences: one for registry preparation
and sequential Push sends, and one for the independent host and local RTI
consequences. Both final charts render with readable labels and no clipping.
The forced-loss membership-list chart is tall, but its runtime branches and
evidence limits remain readable. This records local screenshot review only,
not GitHub renderer verification. Twenty-one charts leave the pending queue;
five remain.

The four earlier render attempts produced no images, and a targeted attempt on
the new DDM chart using cached Chrome Headless Shell also failed to launch
(exit code 2147483651). A follow-up through the available interactive Chromium
window also failed to accept address-bar input after refreshing the selected
window, so no diagram was rendered in those attempts. A later Mermaid CLI
render successfully produced the DDM chart now recorded above. The new
pending-TAR sequence parses with the local Mermaid core, but `mmdc` likewise
failed before rendering because its browser could not launch. A 2026-10-19
retry after reboot still produced no image: the bundled browser failed before
launch; system Chrome/Edge attempts with isolated temporary profiles ended in
profile-lock/credential errors; and a Chrome pipe-transport retry created no
PNG. A further focused stdin-fed mmdc retry on 2026-10-19 also failed before
image creation with Chromium launch code 2147483651. The TAR chart remains
pending; this is not a visual review. The new
joined-federate process Push failure chart
also passed a Mermaid 12.1 core parse, but its targeted `mmdc` render failed
before image creation with the same Chromium launch error. Retry visual QA when
the browser environment changes;
the two earlier Federation MOM charts also parsed but remain visually
unreviewed; the revised synchronization list/status chart separately passed
syntax parsing and is also awaiting visual review.
Earlier screenshot attempts could not activate Chromium, and initial Mermaid
CLI/browser launches failed before creating images. A later local render run
successfully produced screenshots for the four-chart batch above, so the earlier
launch failure is no longer a blocker for those charts. At that earlier
checkpoint, twenty pending diagrams still needed rendering and
visual review; do not treat
the completed batch as whole-guide or GitHub-renderer verification.
At that earlier checkpoint, the forced-loss route-availability and object/DDM
lifecycle flows had syntax-only checks and twenty items remained. Since then,
the observer-route chart was revised to show repeated planned callback entries
and locally rendered/screenshot-reviewed; the object/DDM chart has since passed
local screenshot review. The current itemized queue is above.
After publication, check GitHub rendering for changed diagrams.

## Remaining work, in priority order

1. **Keep mixed ordinary/default and regional interaction routing evidence
   bounded.** The current embedded planner and updated focused test sources
   agree that an eligible ordinary/default route is not suppressed by a
   disjoint With Regions declaration. The focused 2025 CTest passed 1/1 with
   50 assertions; the [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
   now shows that source/test route order. This establishes only the named
   embedded scenario, not process parity or a normative interpretation. The
   exact §9.1.5 rule remains open until authorized standard text is reviewed.

2. **Finish visual QA for changed diagrams.** Render and inspect the five
   confirmed-pending diagrams above, checking clipping, label readability,
   branch direction, and whether each chart teaches the intended HLA
   distinction. The reporting guide's eleven chart statuses are now reconciled;
   one of its charts remains in the pending queue. Record each visual result
   separately; a syntax parse is not a visual review.

3. **Resolve standards-text-dependent questions when the canonical text is
   available.** Existing guides deliberately leave exact wording unverified for
   the Flush Queue Request delivery boundary (§8.12), regional-subscription
   rules, voluntary `ResignAction` applicability and effect ordering
   (§§4.12.3–4.12.4), automatic-resign action semantics, and object-lifecycle
   cutoff wording (§4.4). The [official IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
   identifies the active standard and offers purchase/subscription access, but
   does not expose clause prose. Once an authorized copy is available, compare
   each statement with its exact clause before making it normative. Source code
   and tests are not substitutes for that text.

4. **Keep implementation-evidence gaps visible without turning them into HLA
   rules.** The existing backlog records bounded questions such as configured-
   process callback-failure outcomes, regional request failure schedules, and
   ordinary late-subscription initial values. The joined-federate MOM review
   now records the source-only embedded behavior for a survivor's throwing
   `Remove Object Instance` callback and the separate process callback boundary;
   no focused throwing-callback test was found. Revisit a claim only when new
   focused evidence arrives; distinguish embedded from configured-process
   observations, and do not infer parity from one path.

5. **Continue reader-gap reviews selectively.** The 2025 name-and-handle guide
   was rechecked: it already distinguishes execution-scoped model symbols,
   federate identity, federate-local object knowledge, inherited member lookup,
   typed opaque handles, encodings, hashes, and normalization coordinates.
   The 2025 attribute update-rate guide was also rechecked: its four flows
   already separate designator lookup, per-attribute best-effort admission,
   subscription-generation history, and the steady-clock gate from TSO grant
   eligibility. It also distinguishes embedded observations from the
   configured-process evidence limit. Both reviews produced no-change
   dispositions; another chart would repeat existing explanations. The latest
   callback/service-ordering review adds a source-bounded flow for the
   `HLA_EVOKED` multi-callback elapsed-time window: the implementation checks
   its maximum after a callback returns. Its focused dispatcher test starts
   with queued work and verifies FIFO completion, but does not cover an empty
   queue wait, late arrival, or deadline overrun; exact §10.58 semantics remain
   dependent on licensed standard text. For future candidates, add or revise a
   diagram only when a specific HLA question is hard to answer from the current
   guide set; otherwise record why the existing explanation is sufficient.
     Bounded 2025 Attribute Scope reader-gap review (2026-10-17): its existing
    guide already leads with receiver/object/attribute scope, separates scope
    advisories from owner relevance and value reflection, and maps mutation,
    grouping, switch gates, stale-work checks, and callback observation. The
    focused evidence limits switch-only suppression and post-send race outcomes
     explicitly. No new semantic diagram was needed, but the fourth process-event
     transport chart added on 2026-10-16 lacked visual review and was missing
     from the itemized queue; it is now the twentieth visual-QA item. That chart
     remains source-bounded and does not assert an HLA stale-event cutoff.
    Bounded 2025 joined-federate MOM reader-gap review (2026-10-17): the guide's
    existing state view and explicit-Resign/process-Push traces already separate
    member removal, observer knowledge, pending callbacks, and callback-time
    snapshot cleanup. A bounded route audit found an additional forced-loss
    branch: if no known observer has a usable route, the registry can erase the
    private snapshot while skipping a still-known observer; if another removal
    is planned, a skipped known ID can remain after planned callback entries
    without a queued retry. Added a source-labeled flow to make this boundary
    visible. No focused test exercises the missing-route branch, so this is not
    normative or tested behavior. Mermaid core syntax parsing passed; render and
    visual review remain pending, raising the visual queue from twenty to
    twenty-one.
     Bounded 2025 object-information reader-gap review (2026-10-17): the existing
     registration/discovery, regional-relevance, and deletion views explained
     their local transitions but did not place object existence, per-receiver
     knowledge, current DDM eligibility, and removal in one mental model. Added
     a source-bounded flow: current regional relevance may gate discovery and
     later values, while the inspected embedded delete planner selects removal
     recipients from known receivers rather than current region overlap. The
     regional and embedded deletion tests cover separate component scenarios;
     no reviewed focused test combines a disjoint commit followed by deletion.
     Mermaid core syntax parsing passed; render and visual review remain
     pending, raising the current queue from twenty-one to twenty-two.
    No-change disposition (2026-10-17): A bounded 2025 Directed Interaction
    review compared the guide's target/recipient model with the current shared
    recipient predicate and focused embedded/process cases. The guide already
    distinguishes producer, target object, and receiving federates; uses the
    registered object class for FOM/publication checks and each receiver's
    known class for selector lookup; and separates universal/by-ownership
    selection from DDM regions. Existing flows cover class projection and the
    embedded/process callback boundary. The inherited-receipt test exercises a
    base-known target plus a base interaction subscription; selector and known-
    target cases exercise ownership, sender exclusion, and stale-work fences.
    The process multi-recipient test covers exact-class routing under both
    callback models, not ancestor selection. No demonstrated reader gap needs
    another chart. Keep normative inheritance and process ancestor behavior
    unclaimed; no build or test was rerun. Next bounded review: 2025 Federation
    Save/Restore, focusing restore-source choice and the distinction between
    restore admission, membership/Join, and participant/federation completion.
   No-change disposition (2026-10-17): the 2025 FOM module-admission guide
   already distinguishes composition/preflight from registry admission, the
   atomic definition-plus-membership commit, and source-only post-commit
   rollback effects. It also separates catalog visibility from object
   discovery and value reflection; another chart would repeat it. The time
   guide now adds one source/test-bounded process sequence showing a pending
   TAR becoming grantable after a regulating peer's TAR. It does not claim a
   normative trigger list or callback timing. A follow-up found that process
   Resign and embedded forced member removal both trigger grant re-evaluation
   in source, but no reviewed focused case shows a departure alone releasing a
   pending advance. The existing lifecycle guide already isolates the forced-
   loss TSO cutoff; no extra chart was warranted from this source-only gap.
    Bounded 2025 cross-cutting increment (2026-10-17): the state/event synthesis
    now uses the paired Federation MOM synchronization queries to show how a
    query/report can expose current facts without itself advancing the
    coordination protocol. The synchronization guide now links this distinction
    back to barrier registration, achievement, and completion. The embedded
    handlers and focused test support the example; configured-process behavior
    remains unverified. This adds source/test links, not another diagram or a
    universal pipeline.
    Bounded DDM region-commit update (2026-10-17): refined the existing
    relevance sequence to separate a pending `setRangeBounds` edit from
    `commitRegionModifications`, which installs the committed snapshot and
    plans discovery in the embedded 2025 path. The focused test observes one
    discovery after Set then Commit, but does not drain or assert between those
    calls; the intermediate old-committed-range behavior remains source-derived,
    not a normative or process-endpoint result. This clarifies an existing flow;
    no duplicate diagram or runtime change was needed.
   No-change disposition (2026-10-17): the reservation guide connects the
   owner's claim, named-registration commit, and peer discovery; the object
   guide separately shows producer knowledge, receiver discovery, federation
   deletion, per-receiver removal, and local-forget; the lifecycle and
   cross-cutting guides explain resignation cleanup and object-existence versus
   known-object state. Prior focused audits already connect these guides and
   identify that no single test covers reservation through peer discovery. The
   evidence boundary is clear, so another lifecycle overview would duplicate
   existing maps. No-change disposition (2026-10-17): the 2025 DataElement
   guide already explains the API-to-octets path, per-element byte-order
   ownership, fixed-format scalar and string encodings, composite cursor and
   padding rules, ordinary versus extendable variant branches, logical-time
   factory delegation, and the boundary between HLA value octets and private
   transport framing. Its focused source/test links make the claims verifiable;
   another overview would repeat the existing diagrams. The subsequent bounded
   survey selected the 2025 Federation Save/Restore guide; its no-change result
   is recorded immediately below.
   No-change disposition (2026-10-17): the 2025 Save/Restore guide already
   separates application-owned checkpoints from RTI federation state, request
   admission from protocol completion, participant reports from federation
   commit, and retained-snapshot restore from bounded durable-image rehydration.
   It also maps selected status, failure, callback, and in-flight-work cases to
   their focused evidence. The Join-during-save/restore contract remains open
   pending authorized standard text; implementation traces and tests do not
    replace it. No additional flow is needed. Bounded status update (2026-10-17):
    the current 2025 embedded DDM planner and two focused test sources now agree
    on the mixed ordinary/default route; the current focused CTest passes 1/1
    with 50 assertions. The two DDM charts were updated and syntax-parsed, but
    browser launch failed before rendering, so visual QA is pending. The
    normative §9.1.5 interpretation remains open. No-change disposition
    (2026-10-17): the 2025 authorization and connection guides already explain
    typed credentials, Connect authorization and local commit, connection-scope
    credential snapshots used by Create/Destroy/Join, joined Disconnect
    rejection, and snapshot/authorizer cleanup after successful Disconnect.
    Rechecked current 2025 source anchors and repaired stale helper/commit links;
    no second flow is warranted. The authorization test source uses Disconnect
    for cleanup but does not assert same-ambassador reauthorization, so that
    remains a source-derived lifecycle detail, not a tested outcome or
    HLA-mandated retention rule. Bounded Federation MOM review (2026-10-17):
    the embedded forced-connection-loss adapter queues the federation
    membership-list Conditional update, while the inspected process loss
    dispatcher has no such enqueue; the focused membership test covers ordinary
    Join/Resign only. Added a source-bounded flow and clarified that the
    execution-scoped Federation MOM record survives the shared member-loss
    transition. Mermaid 12.1 syntax parsing passed for the new flow and revised
    state chart, but neither has fresh visual review. This is not a normative
    interpretation of connection loss or a process-parity claim. Next bounded
    reader-gap survey (completed 2026-10-17): the 2025 joined-federate MOM guide
    now distinguishes explicit Resign from forced ConnectionLost. Both reach
    the shared registry removal plan for the RTI-owned member object, but the
    forced path may skip unavailable survivor routes; the focused object tests
    cover Resign only. The existing state chart remains scoped to explicit
     Resign, so no diagram or visual-QA item was added. The inventory then stood
     at 335 diagrams. A follow-up callback-order
    review found that embedded source submits survivor removals before the
    departing member's local terminal callback; HLA_IMMEDIATE may enter a
    survivor callback inline, while HLA_EVOKED and process delivery use separate
    queues. No focused test compares both streams, and the pinned API header
    does not state a relative order. The callback guide now records this source
    boundary and cross-links the MOM lifecycle guide; no new diagram was needed.
    Completed callback-failure review (2026-10-17): the embedded registry moves
    a recipient across the callback-time removal boundary before application
    code runs. An inline `HLA_IMMEDIATE` throw cancels remaining pending
    reservations and unwinds the producer before its later membership-list and
    terminal-callback submissions; an evoked throw happens after submission and
    the dequeued task is not requeued. The process callback bridge has no
    callback-failure acknowledgement or rollback/retry in the inspected path.
     No focused throwing-callback test was found, and no new diagram was
     warranted. Keep these as implementation observations, not normative
     callback guarantees. Completed process Push failure audit (2026-10-17): the
     service begins registry removal for every reachable recipient while it
     builds the local event list, before the first send. A failed send stops the
     remaining send loop without restoring already-cleared known/pending state;
     the snapshot may already be erased. Pull retains the reservation until
     receive polling, and a missing session cancels only the pending reservation.
     No focused forced-loss send-failure test was found. The new sequence passed
     Mermaid 12.1 core syntax parsing; targeted rendering failed before image
     creation because Chromium did not launch, so it joins the visual-QA queue.
      Cascade review complete (2026-10-17): a failed process Push send closes
      the server connection; that recipient's host worker can then route its
      session through a second registry connection-loss transition while the
      recipient-side client independently queues local `connectionLost`
      notification. The earlier MOM begin transition removes object observer
      state, not federation membership. This remains source-derived: the nearby
      process transport test checks framed exchange, and the automatic
      forced-resign case is embedded; no focused process-host cascade test was
      found. The integrated sequence passes Mermaid 12.1 core syntax parsing,
      but is unrendered and visually pending. Completed (2026-10-17): the
      callback/service-ordering guide already covers local process cleanup and
      callback submission from a generic ConnectionLost boundary, so it now
      cross-links to the specific reader/host-worker sequence instead of
      duplicating that flow.

      No-change disposition (2026-10-17): a bounded review of the 2025 directed-
      interaction guide found its existing receive-order chart already explains
      the relevant profile boundary: embedded delivery re-plans at callback
      entry, while the configured-process path materializes a recipient event
      during send and the inspected delivery path does not re-plan it. The guide
      also states that no focused process test mutates declarations or deletes
      the target after send acceptance; the exact-class multi-recipient case
      does not fill that gap. No second chart is warranted. The review also
      repaired stale Catch2 evidence anchors: push-mode configuration now points
      to line 582, and the callback-model sections to line 1027. Keep the process
      stale-work outcome source-only and §4.4's normative scope open pending
      authorized standard text.

      2025 MOM synchronization-query increment (2026-10-17): revised the
      existing chart to distinguish the parameterless active synchronization-
      point list from the named-point status roster, and to show the shared
      embedded/configured-process boundary. The pinned 1516.2-2025 MIM specifies
      each request/report pair; reviewed embedded source and the focused test
      establish the list and roster behavior. The configured-process route
      reaches generic receive-order planning, whose publication check can map a
      rejected response to public `RTIinternalError`; no dedicated query planner
      or focused process request test was found, so process report outcomes
      remain unclaimed. Mermaid 12.1 syntax parsing passed; visual review is
      pending, and the backlog was thirteen at that point. The local browser-state check
      found the Windows desktop locked, so no screenshot interaction was
      attempted.

      Bounded follow-up (2026-10-17): gave the configured-process caveat its own
      heading in the Federation MOM guide and linked it from the synchronization
      protocol guide. This makes the MIM Subscribe versus generic publication-
      checked process-send mismatch and conditional exception mapping easier to
      find; it does not establish an actual process request result or profile
      parity. No duplicate diagram was added.

      Bounded 2025 service-invocation reader-gap review (2026-10-17): the existing
      guide already separates a service's synchronous outcome, its invocation
      report, and a later requested MOM report, and its failure diagrams
      distinguish embedded catches, configured-process rejection, and regional
      public preflight. Current source and focused-test scopes match those
      caveats; no duplicate flow was warranted. The review found the guide's
      inventory row stale (11 current diagrams versus eight in the full-review
      record) and identified an explicitly unrendered regional process chart
      plus one unmatched chart-review status; these are now in the visual-QA
      backlog.

      Bounded 2025 subscription-state update (2026-10-17): the existing
      late-subscription chart now shows the embedded discovery callback as a
      reentrancy point before initial-value planning. Source commits known-object
      state before the callback, then checks current membership and recipient
      eligibility afterward; the source comment names unsubscribe, resignation,
      and region change. The cited tests do not exercise those reentrant changes,
      so this remains source-derived, not a normative callback-order or process-
      parity claim. Mermaid 12.1 parsed all eight diagrams in the guide. The
      revision reopens visual review, bringing the queue to sixteen charts plus
      the separate Service Invocation Reporting reconciliation. After desktop
      access returns, review this chart first, then the revised DDM region-
      commit/discovery flow, MOM synchronization query, joined-federate Push-
      failure, and regional-report charts; reconcile all eleven service-report
      blocks. Preserve 2025-only scope and the remaining standards-text gaps.

      Bounded 2025 Save/Restore reader-gap review (2026-10-17): the existing
      guide already separates request admission from participant work and
      federation completion. Restore admission validates the durable commit,
      source image, current membership, and callback routes before success and
      begun/initiate notifications; participant completion forms a barrier,
      after which RTI state is rehydrated and the federation-wide outcome is
      reported. It also distinguishes the retained full in-process snapshot
      from the bounded route-free image applied to current live membership.
      Join during an active Save/Restore remains unresolved: the pinned 2025
      API annotations and reviewed embedded/process paths do not establish its
      exact rule, and the focused interlock case does not attempt an in-flight
      Join. No additional chart was warranted. Next bounded review: the 2025
      time-management guide's Save/Restore seam—restored logical time and TSO
      work versus live-session deferred callbacks, grant behavior, and
      retraction state. No builds, tests, or CI.

      Reviewed and revised (2026-10-17): the 2025 time-management guide's broad
      Save/Restore caveat could blur restored time-coordinator requests with
      callback closures already waiting in a live queue. Added a focused flow
      separating saved role/time/lookahead/pending-TAR state, selected TSO and
      retraction ledgers, deferred embedded RECEIVE closures, and configured-
      process pushed frames. Current source and focused tests support pending
      grant rebinding from a live route after restore, but do not establish
      general deferred-RECEIVE replay or host-process restart. The new chart
      passed focused Mermaid core syntax parsing; render and visual review are
      pending, bringing the itemized visual queue to twenty-three. No builds or
      test reruns. Next bounded 2025 reader-gap review: distinguish per-attribute
      ownership from the separate delete-object privilege across ownership
      transfer and object deletion; add a cross-guide chart only if the survey
      finds a real navigation gap.

       No-change disposition (2026-10-17): Rechecked the 2025 ownership and
       object-information guides at that boundary. The ownership guide already
       distinguishes ordinary per-attribute ownership from the special
       `HLAprivilegeToDeleteObject`, separates class-schema availability from
       per-instance authority, and diagrams a transfer that lets B delete while
       A remains the registrant. The object guide separately explains accepted
       deletion and receiver-local removal, and links back to the transfer
       sequence. The selected embedded test proves A's rejection, B's successful
       deletion, and the observer callback naming A; configured-process source
       keeps requester and registrant identities distinct, but its focused
       deletion tests use the same federate for registration and deletion.
       Existing cross-links and complementary diagrams answer the reader
       question without another chart. Keep process parity and the exact
       normative producer-identity attribution unclaimed; no build or test was
       rerun. Next bounded 2025 reader-gap review: compare exception reporting
       with service outcomes—typed API exceptions, callback exceptions after a
       state commit, and asynchronous report callbacks—and add or revise a chart
       only if the existing guides leave a concrete question unanswered.

       No-change disposition (2026-10-17): The 2025 reporting guides already
       answer this comparison at complementary levels. The [cross-cutting
       guide](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#a-service-result-committed-state-and-a-report-are-separate-traces)
       separates public result, operation-specific state/effect, selected
       report path, and later observation without presenting a universal
       transaction timeline. The [exception guide](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md#the-three-report-paths-relevant-to-exception-reporting)
       distinguishes `HLAreportException`, `HLAreportMOMexception`, and
       `HLAreportServiceInvocation`, including independent switches and eligible
       recipients. The [service-invocation guide](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#three-distinct-traces-call-outcome-invocation-report-requested-mom-report)
       defines its MIM payload, switches/sinks, and wrapper-specific writers. The
       [callback guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#which-public-call-is-active-when-the-callback-throws)
       shows when HLA_IMMEDIATE versus HLA_EVOKED changes the active call stack
       and how a selected embedded Send Interaction may already have accepted
       effects before a callback exception. Their cross-links preserve the
       separate report families and process boundaries; another synthesis chart
       would collapse service-specific policies. Combined exception/report
       outcomes and callback-failure schedules remain source-derived or untested
       where the guides say so. No build or test was rerun. Next bounded 2025
       review: audit the Data Element Encoding guide for the reader's path from
       a FOM member type through public value encoding/transport to callback
       decoding, and add a diagram only for a demonstrated gap.

        Reviewed and revised (2026-10-17): The 2025 Data Element guide now states
        the exact BE/LE type set: signed/unsigned 16-, 32-, and 64-bit integers,
        32-/64-bit floats, and `HLAoctetPair`; no 8-bit integer BE/LE pair exists
        in the pinned API. Direct literal-vector tests cover each listed family.
        The review also confirmed fixed BE encoding for `HLAboolean`, UTF-16BE
        units and BE counts for Unicode strings, a BE count for ASCII strings
        and variable arrays, factory-owned logical-time bytes, and the separate
        private BE frame header. Added exact type/test/code references; no byte
        order is inferred from host or frame order. No Mermaid diagram changed.
        The 2025 inventory remains 339 diagrams across 31 guides, with 23
        pending visual review. No builds or tests were run. Next: review the
        2025 composite shape/cursor/padding explanation against its record,
        array, and variant code and focused vectors, especially final-child
        padding versus child type byte order. No 2010 work, builds, tests, or CI.

         Reviewed and revised (2026-10-17): The 2025 Data Element composite
         section now distinguishes composite-relative layout from the returned
         shared-buffer cursor, the variable-array count and non-empty prefix
         padding from between-element padding, and child-defined payload byte
         order from structural framing. Focused fixed/variable-array vectors
         cover nonzero decode cursors, zero-padding validation, and the absence
         of a final inter-element pad; the extendable-variant vector shows a
         little-endian child payload after its big-endian length. The revised
         Mermaid chart passed local core syntax parsing but has no fresh render
         or visual review, raising the pending queue to twenty-four. No builds,
         tests, CI, or 2010 work. Next bounded review: trace factory-selected
         logical-time wire bytes through the 2025 DataElement path without
         inferring byte order from the time representation name.

         Reviewed and revised (2026-10-17): The 2025 Data Element logical-time
         section now distinguishes the pinned value/factory encoding contract
         from Umbra's wrapper behavior and bundled reference-type byte layout.
         It records factory capture and compatibility checks, encode/decode
         copying on `set`, exact-length cursor slicing, and the one-octet
         composite boundary without treating it as payload size or endianness.
         The reference integer64/float64 formats are labeled source-specific;
         focused vectors show `HLAfloat64Time(1.5)` and
         `HLAinteger64Interval(9)`. The revised Mermaid sequence passed local
         core syntax parsing but has no fresh render or visual review, raising
         the pending queue to twenty-five. No builds, tests, or CI. Next bounded
         2025 review: trace the object-instance knowledge lifecycle across
         registration/discovery, subscription and reflection, deletion/removal,
         and local forget; reuse existing guides and tests, and add synthesis
         only for a demonstrated reader gap.

          No-change disposition (2026-10-17): The 2025 object-information,
          name-reservation, and subscription-state guides already connect the
          public lifecycle without conflating its state: registration creates
          the execution-wide instance and producer knowledge; receiver
          discovery is separately planned and committed; later reflection has
          distinct embedded callback-after-discovery and configured-process
          service-staging paths; accepted deletion leads to receiver-local
          removal, while `localDeleteObjectInstance` only forgets the caller's
          view and permits later rediscovery. The object guide already has the
          cross-state/DDM/removal chart and a disappearance comparison table,
          with links to focused evidence and explicit untested/profile
          boundaries. The focused tests cover complementary transitions, not
          one end-to-end trace; the disjoint-region-then-delete relation remains
          source-only. No synthesis chart or test rerun is warranted. Next:
          begin visual QA with the existing 2025 queue, starting at the
          [transportation pending-state chart](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md#2-transportation-change-requests-have-a-pending-lifecycle);
          retain all unreviewed items.

Bounded 2025 reader-gap review (2026-10-19): The Connect/configuration guide
already distinguishes factory construction from Connect-time RID profile
selection, authorization and configuration-result gates, local Disconnect
teardown, and asynchronous process-host cleanup; its overload and cleanup
evidence limits are explicit, so no duplicate chart is warranted. The
Federation MOM guide already diagrams the parameterless embedded
HLArequestMIMdata request/report and separately marks the configured-process
generic receive-order route as unverified. The bounded source/test search found
no process-specific MIM-report planner or process case. No new behavioral claim
or chart was added. The first visual-QA item remains cross-federate pending TAR:
the latest local mmdc retry failed before producing a PNG, so all five items
remain pending. No builds, tests, CI, 2010, or Requirements Lab work.

## Completion checks for each future update

- Start from the HLA question and state what changes, who is affected, and when
  an effect becomes observable.
- Separate normative/API statements from Umbra-source observations and
  focused-test results. Cite the exact standard section for normative claims.
- Show profile differences only where evidence demonstrates them; do not imply
  behavior for an untested path.
- Use Mermaid where it clarifies decisions or event order. Prefer a cohesive
  diagram over a chart added only to increase the count.
- Check changed links and Mermaid syntax, then record render and visual-review
  status honestly. No whole-tree visual sign-off or fixed diagram count is
  required.

The living topic inventory and detailed evidence notes remain in the
[behavior-flow guide index](HLA-BEHAVIOR-FLOW-GUIDES.md). The current active
handoff is tracked in `docs/planning/ROADMAP-INDEX.json`.
