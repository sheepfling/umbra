# IEEE 1516.1-2025 Documentation Roadmap: Historical Reviews

Completed visual, semantic, and source/test review notes moved from the active roadmap on 2026-11-07. Entries are preserved in their original order and wording. The archive stays beside the guides so relative source links retain their original bases.

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
now compares explicit Resign's all-route preflight with forced `ConnectionLost`'s ability to skip an unavailable
observer. It carries both paths through the shared removal planner, callback-entry known/pending cleanup, and the
snapshot's erase-or-retain outcomes. The embedded and process tests cover successful explicit Resign, not the
unavailable-route or forced-loss branches. The revised chart rendered with Mermaid CLI 12.0.0 and its local screenshot
is legible without clipping; this source-only branch remains implementation evidence, not an HLA requirement.
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
not GitHub renderer verification. At the 2026-10-17 checkpoint, twenty-one
charts had left the pending queue and five remained.

The historical renderer follow-up's pending list is now reconciled: the
[regional RAVU Save/Restore chart](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#process-request-metadata-is-separate-from-notification-delivery),
[synchronization callback-exception chart](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#in-the-process-push-path-callback-failure-follows-barrier-closure),
[Delete/Retract chart](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#configured-process-callback-buffers-are-a-separate-boundary),
and [credential-lifetime chart](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md#disconnect-ends-this-source-level-credential-scope)
all have local Mermaid CLI 12.0.0 render and screenshot-review records in this
batch. Their current Mermaid blocks match `HEAD`, so no re-render was needed.
This closes local review for these four charts only; GitHub rendering remains
unverified.

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
image creation with Chromium launch code 2147483651. At that 2026-10-19
checkpoint, the TAR chart remained pending; it was subsequently rendered and
reviewed as recorded above. The new
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
   bounded.** The 2025 DDM guide separates the implicit default-region path
   from explicit source regions and shows the tested ordinary route before
   regional fallback. The current embedded planner and focused test agree
   that an eligible ordinary/default route is not suppressed by a disjoint
   With Regions declaration. The focused 2025 CTest passed 1/1 with 50
   assertions. This establishes only the named embedded scenario, not process
   parity or a normative interpretation. The exact §9.1.5 rule remains open
   until authorized standard text is reviewed; no further diagram is needed
   for the current evidence.

2. **Resolve standards-text-dependent questions when the canonical text is
   available.** Existing guides deliberately leave exact wording unverified for
   the Flush Queue Request delivery boundary (§8.12), regional-subscription
   rules, voluntary `ResignAction` applicability and effect ordering
   (§§4.12.3–4.12.4), automatic-resign action semantics, object-lifecycle
   cutoff wording (§4.4), and the exact scope of TLS 1.3 transport (§12.13.5).
   The [official IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
   identifies the active standard and offers purchase/subscription access.
   The [1516.1-2025 product page](https://standards.ieee.org/ieee/1516.1/6688/)
   lists purchase and subscription access. The official
   [Access Standards page](https://standards.ieee.org/access-standards/)
   offers free read access to selected standards but does not identify whether
   1516.1-2025 is included; automated retrieval of its Reading Room link
   returned HTTP 418 on 2026-11-06. This does not establish that an authorized
   account cannot access it. The public
   [downloads archive](https://standards.ieee.org/wp-content/uploads/2025/08/1516.1-2025_downloads.zip)
   contains the API XML package (API files, protobuf, schemas, and examples),
   not the standard clauses. Once authorized clause text is
   available, compare each statement with its exact clause before making it
   normative. Source code and tests are not substitutes for that text.

3. **Keep implementation-evidence gaps visible without turning them into HLA
   rules.** The callback-failure review found the configured-process exception
   boundaries already documented: event dequeue, dispatcher task consumption,
   callback-model differences, and the TSO completion-acknowledgment boundary.
   No focused application-callback-throw test exists, so these remain
   source-derived observations. The late-subscription review distinguishes a
   newly-known object, whose current eligible values have focused source/test
   evidence, from adding an attribute to an already-known object. The latter
   has no ordinary current-value path visible in the inspected source and no
   focused callback test; its licensed §5.8 interpretation remains open. The
   guide records this as an evidence boundary rather than implying either
   callback outcome. The RAVU guide now traces
   whole-request regional admission, per-group
   preparation, Pull queueing, Push staging and sequential-send failure
   boundaries. These failure outcomes remain source-derived because no focused
   failure-injection case was found. The DDM guide now traces
   update-region batch rejection before association-map mutation and the
   configured-process enqueue-failure boundary after registry commit; this
   does not claim an HLA exception schedule. The joined-federate MOM review
   now records the source-only embedded behavior for a survivor's throwing
   `Remove Object Instance` callback and the separate process callback boundary;
   no focused throwing-callback test was found. Revisit a claim only when new
   focused evidence arrives; distinguish embedded from configured-process
   observations, and do not infer parity from one path.

4. **Continue reader-gap reviews selectively.** The 2025 name-and-handle guide
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
or chart was added. At that checkpoint, cross-federate pending TAR was still
first and all five items remained pending. No builds, tests, CI, 2010, or
Requirements Lab work.

Bounded 2025 participant-set review and visual-QA update (2026-11-03): The
existing [lifecycle comparison](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#a-join-does-not-define-every-active-protocols-participant-set)
already distinguishes synchronization's omitted-set late-join expansion from
the unresolved Save/Restore Join contract; its source/test limits are linked
through the [synchronization guide](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#late-joiners-change-only-an-expandable-barrier)
and [Save/Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#join-crosses-a-separate-membership-admission-boundary).
The synchronization guide marks partial achievement before late join as
source-derived and untested; the Save/Restore interlock test does not attempt
Join during an active protocol. No combined algorithm or shared admission rule
is warranted.

Latest progress (2026-11-03): the final three charts in the selected queue and
the subsequently found Auto Provide process-discovery chart passed local
screenshot review. A bounded Authorization review found its guide already
separates authorization admission from service outcome, the three callback
scopes, credential lifetime, and unresolved test/normative boundaries. No new
chart was warranted there.

The 2025 TSO Retraction guide's 13 current charts were then rendered and
screenshot-reviewed with Mermaid CLI 12.0.0. Two dense process sequences were
split at the actual server/client callback boundary; the shared client/bridge
decision now has one diagram referenced by both attribute and object-removal
routes. The two timing-sequence labels were shortened to keep their boundary
notes readable. All 13 charts are legible without clipping or overlap. This
raised the selected visual-QA queue from 27 to 40 reviewed, with none pending,
and the 2025 inventory from 340 to 342 diagrams (120 sequences). No C++ tests,
builds, or CI were run. GitHub rendering and whole-tree visual quality remain
unverified.

The subsequent bounded reader-gap review of the 2025 Order and Transportation
guide found a useful state-lifetime distinction: each embedded send plan reads
the current effective order/transportation, while accepted TSO payloads retain
their per-send metadata until grant-boundary delivery. Added one source-bounded
flowchart for this distinction and marked the missing setting-change-during-
queued-delivery test explicitly. The cited focused cases cover setting-before-
send and metadata-at-grant separately; they do not test that interleaving.
Corrected stale source/test line anchors in the guide after implementation
files moved. Mermaid CLI 12.0.0 rendered the new flowchart; its local screenshot
is readable without clipping or overlap. The selected visual-QA queue is now
41 reviewed/0 pending, and the 2025 inventory is 343 diagrams (200 flowcharts,
120 sequences, and 23 state diagrams). No C++ tests, builds, or CI were run.
GitHub rendering and whole-tree visual quality remain unverified.

Previous handoff (completed): reviewed the 2025 Time Management guide's
“Lookahead changes and pending operations” and “Re-evaluation” sections for a
clear account of which time frontier changes and what gets reconsidered,
without conflating grant eligibility with callback-queue delivery.

Bounded 2025 reader-gap review (2026-11-03): The Time Management guide's
re-evaluation trigger list omitted successful Modify Lookahead. Added a
source-bounded explanation that both the embedded ambassador and configured-
process service request grant reevaluation after an applied change. The
snapshot/bounds path distinguishes an immediate equal-or-larger active value
from a smaller deferred target that the bound calculator does not yet use.
Linked the existing lookahead and timestamped-message sections; no third chart
was needed. The cited process case modifies lookahead before the peer TARs and
does not isolate the change as a release trigger; the state unit test covers
deferred grant transactionality only. The process public test's previously
recorded local run failed before its lookahead assertions. No test/build/CI was
run, and no normative or profile-parity claim was added.

Previous handoff (completed): bounded reader-gap review of the Time Management
guide's “Timestamped message lifecycle and callback order” section, tracing
how TSO queue phases affect GALT/LITS, grant reevaluation, and callback entry.

Bounded 2025 reader-gap review and visual update (2026-11-03): Corrected a
stale timing statement that included delivered-since-last-advance records in
GALT/LITS. The current bounds calculator examines only queued and in-transit
recipient entries; delivered-phase records remain for completion-history and
Save/Restore purposes until the grant boundary. Added a configured-process
sequence connecting an in-transit timestamp-5 message (GALT/LITS 5), its
HLA_EVOKED callback and acknowledgement, the ensuing reevaluation, and the
producer's time-3 plus lookahead-3 bound that permits the receiver's TAR(5).
The guide combines the public test's in-transit and callback-order assertions
with a focused bounds-unit assertion about delivered-phase exclusion; it does
not claim the process case passed in this pass. Mermaid CLI 12.0.0 rendered and
screenshot review found the revised lifecycle state diagram and new sequence
readable without clipping or overlap. The selected visual-QA queue is now 43
reviewed/0 pending and the 2025 inventory is 344 diagrams (200 flowcharts,
121 sequences, and 23 state diagrams). No builds, tests, or CI were run.
GitHub rendering and whole-tree visual quality remain unverified.

Bounded 2025 reader-gap review (2026-11-03): The NMR/NMRA matrix incorrectly
said future transport arrivals do not participate in target selection. The
guide now distinguishes the caller boundary R from effective target E and
explains the selected coordinator's pending-request retarget rule: an admitted
TSO at T may lower E only when it is in the current-time-to-R range and is
strictly earlier than E. A configured-process HLA_EVOKED sequence illustrates
NMR(10)/NMRA(10), a late timestamp-5 interaction, and grant at 5; the focused
separate-listener case asserts this order for both request modes. The guide
keeps the source/test/profile limits explicit and does not generalize this to
all arrivals or implementations. Mermaid CLI 12.0.0 rendered the new sequence;
screenshot review found it readable and unclipped. Six cited source/test links
and line anchors passed targeted checks; `git diff --check` passed. No builds,
tests, or CI were run.

Bounded 2025 reader-gap review (2026-11-03): Corrected the Time Management
guide's TAR/equality matrix and exact-boundary example. The prior wording said
a TAR could grant at an exact queued-TSO GALT, contradicting the grant policy
and its focused unit case. The guide now states: TAR is strict for every GALT
source; TARA/NMRA permit equality at available GALT; NMR permits equality only
when its effective target is the queue-derived TSO boundary. It also limits
end-to-end callback-order evidence to the focused TARA scenario and notes the
absence of a public exact-boundary NMR callback case. Updated the grant-flow
decision node to call out request mode and GALT source. Mermaid CLI 12.0.0
rendered the revised chart and screenshot review found it readable and
unclipped. The selected visual-QA queue is now 45 reviewed/0 pending; the
2025 inventory remains 345 diagrams. The implementation and three focused
policy cases plus the public TARA case are linked; no tests/builds/CI were run
and no normative claim was added.

Bounded 2025 reader-gap review (2026-11-03): The Order and Transportation guide
now shows an embedded `HLA_EVOKED` interleaving in which unconditional
divestiture edits pending transportation-change work before its queued
confirmation task runs. A partial request can continue for its remaining
owned attributes; if no request remains or live eligibility fails, the task
has nothing to confirm. The lifecycle state chart now distinguishes a pending
request narrowed by partial divestiture, and the new sequence traces both
outcomes. The focused save/restore interlock rejects both services; the custom
transportation-control test drains confirmation for `firstObject` before
divesting a different `unownedFallbackObject`. No focused test covers the
interleaving or asserts its partial-set/no-confirmation outcomes. This is
source-only implementation evidence, not a normative ordering or callback
guarantee and not a process-profile result. Mermaid CLI 12.0.0 rendered both
charts on a white background; local screenshot review found them readable
without clipping or overlap. Twenty-three local links and line anchors in the
reviewed section passed. The selected visual-QA queue is now 47 reviewed/0
pending; the 2025 inventory is 346 diagrams (200 flowcharts, 123 sequences,
and 23 state diagrams). No C++ tests, builds, or CI were run. 2010 and the
Requirements Lab were not touched.

No-change disposition (2026-11-03): Reviewed the Data Distribution and Regions
guide's parent-class regional subscription path for a child-class object. Its
inherited-dimension flow, receiver-ancestry flow, and worked source-only trace
already separate class dimension context, exact class/attribute declaration
storage, per-receiver parent lookup, and later projection through the saved
known class. The focused child-object/base-subscription case has no regions;
the three-dimensional regional case subscribes and registers at the same
class; the configured-process regional cases also use the same class. No
combined cross-class test or normative result is claimed, and another diagram
would repeat the current explanation. The bounded check found stale line
anchors after source files moved; corrected citations for the dimension helper,
discovery resolver and parent step, committed-region overlap, class-keyed
subscription storage, saved-known-class projection, and process same-class
assertions. The 21 local destinations and 20 line anchors in the cross-class
section resolve. No diagram changed and no C++ tests, builds, or CI were run.

Bounded 2025 reader-gap review (2026-11-03): The Attribute Ownership guide's
existing flowchart and source note already distinguish the divestiture-triggered
acquisition notification race. Cancellation planning and notification begin
serialize on registry state: a reservation-first path keeps the request pending
and suppresses stale notification work; a notification-begin-first path assigns
ownership and removes the pending attribute, so later cancellation is rejected.
The guide separately shows configured-process push beginning cancellation in
the service handler and pull beginning it at the receive poll. Its evidence
remains bounded: the process cancellation and divestiture tests are separate;
the embedded cancel-before-divest test concerns later assumption-search
eligibility; the Save/Restore interlock rejects both services. No focused
configured-process test covers the combined race, and no HLA race priority or
callback-order rule is claimed. No new diagram was warranted.

This review found process `receive_interaction` citations that had shifted to
unrelated lines. Corrected links for pull notification/cancellation gates,
empty-event suppression, If-Available polling, unavailable-event ordering and
recheck, and the owner release callback gate. A focused check validated all 27
local links and line anchors in the race subsection; the cited receive paths
were checked against their current function lines. No C++ tests, builds, or CI
were run. The 2025 inventory remains 346 diagrams, and the selected visual-QA
queue remains 47 reviewed/0 pending. 2010 and the Requirements Lab were not
touched.

Bounded 2025 reader-gap review (2026-11-03): The Federation Synchronization
guide's prose had the correct forced-loss behavior, but the branch between
keeping a point pending and completing it for the remaining participants was
easy to miss. Added a source-labeled flowchart: shared member removal clears
the lost ID from the synchronization, announced, and achieved sets; an empty
participant set erases the point without recipients; otherwise the point
closes only after every remaining participant has reported. The resulting
failed-to-synchronize set contains only remaining recorded `false` results.
The chart keeps the automatic ResignAction's ownership/object cleanup separate
and shows the embedded callback-work and configured-process receive-event
projections as source behavior, not portable HLA transport-loss requirements.
No focused active-barrier connection-loss test was found; the existing
voluntary resignation test does not cover forced loss or process delivery.
Corrected stale line anchors for shared cleanup and both adapter paths. The
reviewed section and evidence row have 12 local destinations, including eight
source and two test line anchors; all targets exist and anchors are in bounds.
Mermaid CLI 12.0.0 rendered the chart on white and the local screenshot is
readable without clipping. The 2025 inventory is now 347 diagrams (201
flowcharts, 123 sequences, 23 state diagrams); the selected visual-QA queue is
48 reviewed/0 pending. No C++ tests, builds, or CI were run.

Bounded 2025 callback review (2026-11-03): Corrected the recipient-departure
section after tracing successful configured-process Resign through callback
suppression, the no-drain request, local event-buffer clearing, bridge queue
discard, and suppression release. The guide distinguishes a callback already
claimed by the dispatcher from buffered work, and leaves a frame crossing the
response/cleanup boundary unresolved; failed Resign follows a separate
release-and-drain path. Rendered and screenshot-reviewed the new sequence with
Mermaid CLI 12.0.0. The diagram is readable without clipping. A bounded audit
checked 97 local destinations in the section: 59 source line anchors, 35 test
line anchors, two guide-heading links, and one path-only link; all targets and
anchors resolve. This is implementation evidence, not an HLA callback-order
guarantee.

The same visual-review batch rendered and inspected the existing 2025
Federation MOM forced-connection-loss membership-list flow and federation-object
lifetime state chart. Both are readable without clipping; their source/test
boundaries remain explicit. The selected visual-QA queue is now 51 reviewed/0
pending. The 2025 inventory is 348 diagrams (201 flowcharts, 124 sequences, 23
state diagrams; 340 backtick-fenced, 8 tilde-fenced). No C++ tests, builds, or CI
were run. Whole-tree visual quality and GitHub rendering remain unverified.

Bounded 2025 joined-federate MOM review (2026-11-03): The existing guide
explained that explicit Resign preflights every known survivor route while
forced `ConnectionLost` can skip an unavailable route, but its flow showed only
the forced-loss branch. Revised that existing chart to compare the two entry
paths and carry them through shared removal planning, callback-entry known and
pending cleanup, and snapshot erase/retain outcomes. The embedded case observes
successful explicit-Resign removal and snapshot cleanup; the configured-process
case observes removal before later rediscovery. No focused case covers an
unavailable route or forced-loss removal. Mermaid CLI 12.0.0 rendered the
revised chart; the tall local screenshot remains readable without clipping.
This is source evidence, not an HLA route-availability rule. At that checkpoint
the chart count was 348 and the selected screenshot-review queue was 51
reviewed/0 pending. No builds, tests, or CI.

Bounded 2025 callback-failure review (2026-11-03): The guide's prose described
four different exception boundaries but did not put them on a comparable
timeline. Added two focused sequences: one separates embedded `HLA_IMMEDIATE`
inline entry from `HLA_EVOKED` queue pop, and one separates configured-process
Push from Pull state-commit points before the client callback. Corrected stale
source anchors for embedded callback entry/cancellation, dispatcher pop and
invocation, and process receive/bridge delivery. The focused embedded and
process cases cover successful explicit Resign only; no test injects a throwing
Remove callback after forced loss. Mermaid CLI 12.0.0 rendered both saved
diagrams and local screenshots were inspected; both fit without clipping and
keep their backend boundaries readable. Checked the subsection's 15 local
destinations (13 source line anchors and two test anchors); all resolve. The
2025 inventory is 350 diagrams (201 flowcharts, 126 sequences, 23 state
diagrams; 342 backtick-fenced, 8 tilde-fenced); the selected visual-QA queue is
53 reviewed/0 pending. No builds, tests, or CI.

No-change disposition (2026-11-03): Reviewed the 2025 time-management guide's
receive-order versus timestamp-order boundary at a time advance. The existing
[TSO lifecycle and callback-order section](HLA-2025-TIME-MANAGEMENT-GUIDE.md#timestamped-message-lifecycle-and-callback-order)
already separates embedded TSO dispatch from the grant callback, while the
[mixed-recipient update case](HLA-2025-TIME-MANAGEMENT-GUIDE.md#one-timestamped-update-two-recipient-paths)
and [process order-split flow](HLA-2025-TIME-MANAGEMENT-GUIDE.md#process-endpoint-the-same-order-split-crosses-a-different-boundary)
show how RECEIVE and TIMESTAMP passels cross different gates. Focused cases
support bounded embedded and process observations but do not establish one
universal sequence or cross-profile parity. No duplicate chart was warranted.

Bounded 2025 FQR review (2026-11-03): The guide already separates queued TSO
delivery, actual grant, and optimistic floor, and contrasts the embedded
HLA_EVOKED callback-time extraction policy with the configured-process
acceptance-time result. It ties §§8.12–8.14 claims to the pinned Requirements
Lab transcription and does not turn either profile cutoff into a normative
rule. Fixed a semicolon that prevented Mermaid parsing, simplified the
existing embedded sequence, then rendered and screenshot-reviewed it with
Mermaid CLI 12.0.0. No second profile sequence was warranted; no tests or
builds were run. The 2025 inventory remains 350 diagrams across 31 guides;
the selected visual-QA queue remains 53 reviewed/0 pending.

Bounded 2025 NMR/NMRA review (2026-11-03): The existing section already states
that a newly admitted same-implementation TSO can move effective target E
earlier only when T is not before current time, no later than caller boundary
R, and strictly earlier than current E. Its configured-process HLA_EVOKED
sequence and focused cases distinguish R from E and observe the interaction
callback before grant at 5. A separate embedded alternate-advance test queues
TSO at 8 before NMRA(10), retracts it before callbacks, and observes grant at
8 with no reflection. The inspected coordinator retraction path removes
queued/in-transit TSO state without recomputing E, but no focused case combines
late-arrival retargeting, retraction, and the following grant trigger. The
guide records this bounded source/test distinction; no duplicate chart,
normative retarget claim, or profile-parity claim was added. No test or build
was run.

No-change disposition (2026-11-03): Reviewed the existing synchronization
guide's omitted-set late-Join flow after a partial achievement. The guide
already traces retained `false` state, participant-set growth, the new total
needed for completion, and the final failed set. It separates embedded source
state from configured-process Join announcement/rollback, and explicitly says
the direct/public late-join cases join before any achievement while the process
rollback case has no prior result. The partial-result-before-Join sequence is
therefore source-derived and remains untested; no duplicate chart or parity
claim was warranted.

Bounded 2025 ownership/removal review (2026-11-03): Revised the existing
[delete-authority transfer sequence](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#a-new-owner-can-gain-delete-authority-without-becoming-the-registrant)
to show B's accepted delete reserving removals for other known members A and
C before B becomes unknown. Each surviving receiver is still subject to its
own removal commit gate. The embedded test directly observes C's callback
carrying original registrant A and B's tag, but does not assert A's planned
callback or query B's known-object state. Configured-process source uses the
registrant snapshot, while focused process deletion tests use the same
registrant and deleter. The revised chart rendered with Mermaid CLI 12.0.0 and
passed local screenshot review; no new chart or profile-parity claim. The
inventory remains 350 diagrams across 31 guides; the selected visual-QA queue
remains 53 reviewed/0 pending. No tests or builds were run.

Bounded 2025 embedded class-form RAVU persistence review (2026-11-03): The
existing guide already separates full planned-owner projection at per-group
registration from the callback-time eligible-subset recheck. Clarified the
failure branch: if a later group fails after earlier groups were recorded, the
source shows no rollback; this is a possible partial-persistence failure, not
an HLA outcome or a tested runtime scenario. The pending-request restore test
covers one object/provider group and does not exercise a later-group failure
or a partially persisted multi-group request. The revised flow rendered with
Mermaid CLI 12.0.0 and passed local screenshot review. The inventory remains
350 diagrams across 31 guides; the selected visual-QA queue is now 54
reviewed/0 pending. No builds, tests, or CI.

Follow-up source trace (2026-11-03): The failed class-form request queues no
provider work before the later-group registration failure. The embedded state
image nevertheless captures any earlier pending class-request record; restore
validates the saved identity and current owner before reconstructing callback
work and queuing it after `federationRestored`. The existing one-group restore
case does not exercise a failed multi-group request followed by Save/Restore,
so this remains a conditional source-derived path, not a demonstrated recovery
outcome. The existing persistence chart now shows this branch; its refreshed
local render and screenshot review passed. Inventory remains 352 diagrams
across 31 guides; the selected visual-QA queue remains 58 reviewed/0 pending.
No builds, tests, or CI.

No-change disposition (2026-11-03): Rechecked configured-process regional
class-form RAVU Save/Restore against the provider notification queues and state
image path. The registry image captures request records that are still pending;
the service consumes each record before placing its notification in the pull
queue or Push path. Save snapshots ordinary pull attribute-update events, and
Restore clears/rebuilds that separate queue, but the inspected code shows no
matching Save/Restore handling for RAVU notifications. Existing process RAVU
cases cover solicitation or response separately from Restore; no focused
configured-process callback-count/replay case was found. The guide's evidence
table already states these limits. Another diagram would make an untested
implementation path look like an HLA lifecycle rule, so no diagram was added.
No builds, tests, or CI.

Bounded 2025 DSE/RAVU relationship review (2026-11-03): The guides already
separate explicit provider solicitation from ordinary receiver eligibility,
but the DSE guide lacked a direct path to that RAVU explanation. Added a short
cross-guide bridge: the focused DSE attribute test sends no RAVU request, and
the bounded RAVU cases do not hold the requester unsubscribed through the
provider's response. The direct §6.21 entitlement remains unresolved without
canonical clause text. The existing RAVU sequence and DSE recipient-state
diagram explain distinct service steps, so a combined diagram would repeat
them rather than resolve the entitlement question. Checked the two new local
link destinations; both resolve. No diagram or test was added or rerun.

Bounded 2025 late object-subscription review (2026-11-03): The existing guide
already separates an unknown receiver becoming discoverable and receiving
selected stored values, an already-known receiver adding an attribute, and
explicit RAVU. The embedded path commits known-object state, invokes discovery,
then plans current eligible values against reentrant state; its focused test
stores callback collections separately and does not assert their relative
order. The configured-process case source asserts discovery and selected
stored-value reflection, with separate pull/push staging, but was not run in
this documentation pass. Adding an attribute to an already-known object still
has only provider-advisory evidence, not a value-only reflection test; licensed
§5.8 remains unreviewed. No additional diagram was warranted. Rendered the
existing embedded/process comparison with Mermaid CLI 12.0.0 and inspected the
screenshot; it is readable without clipping. Corrected stale worktree-status
notes: the cited process source and test files are clean now. The inventory
remains 350 diagrams across 31 guides; the selected visual-QA queue is now 55
reviewed/0 pending. No builds, tests, or CI.

Bounded 2025 Auto Provide versus explicit RAVU review (2026-11-03): The
existing Auto Provide comparison table and solicitation/response sequence
already distinguish the triggers, provider callbacks, tags, and later
value-producing Update. The focused embedded object-request and class-request
cases both assert the caller's tag at Provide; the class case expands across
two registered subclass instances for one provider. Corrected the Auto Provide
guide's stale statement that the class overload lacked focused evidence and
added reciprocal evidence links in both guides. This does not establish
same-object multi-provider fan-out, process Auto Provide, or process parity.
Auto Provide remains discovery-triggered in the inspected embedded source;
ordinary process initial reflections are separate, and no process Auto Provide
planner call was found in the reviewed path. The existing views answer the
reader's comparison question, so no combined chart was added. Keep §6.21
entitlement unresolved without canonical text. Inventory remains 350 diagrams
across 31 guides; selected visual-QA queue remains 55 reviewed/0 pending. No
builds, tests, or CI.

Bounded 2025 already-known-object subscription review (2026-11-03): The
existing subscription guide and its source/test table already distinguish a
newly known object receiving selected stored values from an already-known
object whose added attribute produces scope or owner-relevance advisories.
The inspected embedded and process source paths show initial-value planning
only on accepted discovery; the focused `PayRate` test observes the owner
advisory, not a value-only reflection. The repository's 1516.1-2025 package
contains the pinned API headers but no full standard text. IEEE's [official
listing](https://standards.ieee.org/ieee/1516.1/6688/) offers purchase or
subscription access, so canonical §5.8 could not be reviewed. Added that access
boundary to the guide; the normative outcome remains open. The existing flow
already explains the source/test distinction, so no chart was added. Inventory
remains 350 diagrams across 31 guides; selected visual-QA queue remains 55
reviewed/0 pending. No builds, tests, or CI.

Bounded 2025 additional-module Join interaction review (2026-11-03): The FOM
guide already documented catalog and handle reconciliation, retained
per-federate interaction declarations, current-catalog ancestor matching, and
the split lookup/receipt test evidence. Added a separate embedded source
sequence beside the existing configured-process sequence. It makes the key
boundary visible: Join catalog growth does not itself enqueue ordinary
Receive Interaction; a later send can enter the ancestry and closest-class
projection path in current source. Embedded lookup verifies the added class
and stable prior handle, ordinary inherited receipt uses a hierarchy present
at Create and an empty parameter map, and the parameter-projection test uses
distinct receivers. The configured-process additional-FOM test uses an
object-only extension and checks object-class lookup. No focused test combines
Join, a retained ancestor subscription, and a later send/receive of the new
descendant; the sequence is therefore explicitly source inference, not tested
post-Join delivery or profile parity. Mermaid CLI 12.0.0 rendering and local
screenshot review passed; labels remain readable without clipping. Inventory
is 351 diagrams across 31 guides; the selected visual-QA queue is 56
reviewed/0 pending. No builds, tests, or CI.

Bounded 2025 additional-module Join object-registration review (2026-11-03):
The FOM guide already traced later registration of a Join-added descendant
through the current catalog and retained ancestor declarations. The object
information guide separately covered initial-value planning after discovery.
Expanded the existing FOM consequence chart to distinguish initial values
(only currently stored values that pass the receiver's gates) from later
publisher updates, with separate embedded callback and configured-process
event-staging paths. The embedded and process late-subscription tests exercise
stored values for classes present at Create; neither combines Join, retained
ancestor subscription, later registration, and initial reflection. The process
test observes both callbacks under both callback models but does not assert a
general callback order. Mermaid CLI 12.0.0 render and local screenshot review
passed without clipping. Inventory remains 351 diagrams across 31 guides; the
selected visual-QA queue remains 56 reviewed/0 pending. The ordinary-object
embedded source path cited here is outside the current diff to that file, which
is limited to joined-federate MOM region designators. No normative §5.8,
process-parity, or 2010 conclusion; no builds, tests, or CI.

Bounded 2025 Join-added attribute and already-known instance review (2026-11-03):
Added a prose bridge in the FOM guide connecting conditional module replacement
to per-instance ownership, class-relative subscription eligibility, known-object
discovery suppression, and the unresolved value-only reflection outcome. The
Restaurant extension is rejected before Join, and no focused test demonstrates
a successful add-attribute Join. The If-Available test covers an attribute
present at Create; the PayRate case observes a provider advisory, not receiver
reflection. Existing FOM, ownership, object-information, and subscription charts
already cover the relevant separate branches, so no new chart was needed.
§5.8 remains unresolved. No 2010 or Requirements Lab work, builds, tests, or CI.

Bounded 2025 If Available-to-update receiver path review (2026-11-03): Added a
source-derived embedded connector flow to the ownership guide showing the
callback-time ownership commit followed by a later, separate Update Attribute
Values call, sender validation, per-receiver class/subscription/relevance/region
projection, and the reflection path. The text also records the If-Available
callback's update-region clearing/deferred-association promotion and default
transportation/order metadata refresh. The focused If-Available test and the
ordinary receive-order update/passel test cover separate paths; neither combines
acquisition followed by reflection. Mermaid CLI 12.0.0 rendering and local
screenshot review passed. Inventory is 352 diagrams across 31 guides; the
selected visual-QA queue is 57 reviewed/0 pending. This is embedded source and
test evidence only, not process/2010 parity. No builds, tests, or CI.

Bounded 2025 RAVU direct-response entitlement review (2026-11-03): The existing
guide already separates the request, provider callback, later update, and
receiver eligibility, and records the test boundaries. The inspected embedded
update projection does not consume an earlier RAVU request; the DSE test that
changes delivery eligibility makes no RAVU request. The RAVU examples retain a
subscription context and do not test an otherwise-ineligible requester through
response. The pinned checkout contains API headers, not §6.21 prose; the
[official IEEE listing](https://standards.ieee.org/ieee/1516.1/6688/) offers
purchase or subscription access. IEEE also advertises free account-based read
access to selected standards through its [Standards Reading Room](https://standards.ieee.org/access-standards/),
but public pages do not confirm whether 1516.1-2025 is included. The normative
entitlement question remains open, and the current explanation needs no
duplicate diagram. No 2010 or
Requirements Lab work, builds, tests, or CI.

Bounded 2025 configured-process Federation MOM request-route review
(2026-11-03): Audited federation and member FOM-module-data, federation
MIM-data, and synchronization-query requests. The 2025 MIM marks these request
classes Subscribe. The process server has no request-specific report branch;
its generic send route rejects an unpublished class or sends a published
request through ordinary recipient fan-out, without reading the requested MOM
state or constructing the paired report. The configured-process publication
service accepts a defined interaction handle without checking MIM sharing, so
the published branch is source-reachable; this implementation observation is
not normative permission. No focused configured-process request/report test
was found; existing direct report tests are embedded-profile cases. The
existing sync-query chart now shows the conditional branches and no paired
report on the generic route; Mermaid CLI 12.0.0 render and screenshot review
passed. Inventory is 352 diagrams across 31 guides; selected visual QA is 58
reviewed/0 pending. No builds, tests, or CI.

No-change handoff reconciliation (2026-11-03): The proposed configured-process
Auto Provide review was already completed in the existing Auto Provide guide
and review ledger. They show the separate process discovery and switch paths,
the absence of an inspected process call to the embedded Auto Provide planner,
and the lack of a focused process provider-callback test. No duplicate chart
or normative-omission claim is needed; configured-process solicitation remains
unverified.

No-change handoff closeout (2026-11-03): Rechecked configured-process object
removal identity after ownership transfer. The source keeps registered-object
producer identity in the removal snapshot while current delete authority can
belong to another member; the focused process deletion tests register and
delete as the same federate. The ownership and object-information guides
already explain the embedded transfer example, process source identity, and
this process-test gap. No additional chart or prose was warranted. This closes
the repeated handoff without making a normative callback-identity claim.

2025 configured-process RECEIVE gate review (2026-11-03): Corrected the time
guide's stale claim that process interactions lack an asynchronous-delivery
gate and process test. The process callback bridge keeps its own constrained,
asynchronous, and pending-advance state; two focused public tests cover the
release behavior under `HLA_IMMEDIATE` and `HLA_EVOKED`. Added a process
interaction gate flow and preserved the separate source-only boundary for
process attribute reflections. The follow-up traced ordinary AttributeUpdate
submission and the separate Push readiness marker: neither calls the
interaction temporal gate. The public reflection test covers both callback
models but not combined time/async/pending-advance state. No additional diagram
was warranted; the source/test boundary is recorded in the time and process
endpoint guides. No normative timing or profile-parity claim. No tests, builds,
CI, 2010, or Requirements Lab work.

Regional class Request Attribute Value Update guide audit (2026-11-03): The
existing guide already explains whole-request preflight, object/provider
partitioning, per-group projection, Pull queueing, Push staging/sending, and
the conditional partial-progress boundaries. Focused tests cover one public
provider group, one private disjoint selector, and isolated invalid selectors;
they do not cover mixed-validity requests, multi-provider splits, or injected
later-group/send failures. The pinned 2025 C++ API declaration lists §9.13
exceptions but supplies no operation prose or exception-precedence rule. No
guide change or new chart was warranted; source/test limits remain explicit.
No tests, builds, CI, Requirements Lab, or 2010 work.

The proposed FOM module-admission/composition review was already covered by
bounded reader-gap and post-Join reviews (2026-10-11 through 2026-10-17), both
with no-change dispositions. Avoid repeating it without new evidence.

2025 region-scope cross-guide review (2026-11-03): Compared the DDM,
regional RAVU, directed-interaction, and object/interaction information guides.
They already distinguish request-region provider solicitation, current
provider source-region filtering, and receiver-side DDM. The directed guide
also states that a target object is not a DDM region and leaves any further
normative relationship open. Added a short introductory link from the DDM
guide to that explanation so the distinction is visible before its detailed
interaction section. No synthesis chart or normative claim was warranted.

2025 ordinary late-subscription gap reconciliation (2026-11-03): The
ordinary late-subscription boundary was already reviewed on 2026-10-14
and 2026-10-16. Current guides distinguish discovery-time values, adding an
attribute to an already-known object, the process MOM-only value path, and DSE;
the remaining ordinary process value and normative callback questions are
explicit evidence gaps. No new focused test or working-tree change in the
bounded test lanes was found, so no repeat chart or prose was added.

2025 object-name and typed-lookup cross-audit (2026-11-03): The reservation
guide already distinguishes an owner's reservation claim and named
registration from a peer's later discovery-based knowledge. Its process-test
evidence now records full bulk success for `{a,b}`, partial success `{c}` and
failure `{b}` for the overlapping `{b,c}` request, successful release of
`{a,b,c}`, and `ObjectInstanceNameNotReserved` on repeated release. That test
does not exercise mixed-set release atomicity or combine reservation,
registration, and peer discovery. The adjacent name-and-handle guide already
explains federate-local instance lookup, typed handle categories, and explicit
process-parity limits. No additional diagram was warranted. The 2025-only
inventory is 360 diagrams across 31 files; the selected visual queue remains
75 reviewed with none pending. No implementation changed and no tests, builds,
or CI were run.

2025 federation synchronization audit (2026-11-03): Verified the existing guide
against the focused registry paths and public scenarios. Its charts and text
distinguish HLA-level registration/announcement/achievement from Umbra's
participant sets; omitted-set late-Join expansion from explicit-set scope;
false-result fan-in; voluntary departure from source-derived forced loss; and
pending-point persistence from completion. Partial-result-before-late-Join,
forced-loss delivery, and partially achieved multi-member restore remain
explicitly untested/source-derived boundaries. No new diagram was warranted.
No guide or C++ source was changed in this review; no tests, builds, or CI were
run.

2025 federation Save/Restore audit (2026-11-03): The guide already separates
federate-owned checkpoints from RTI-owned state, save commit from participant
completion, restore admission from federation restoration, and retained
process-local state from bounded fresh-registry reconstruction. Its in-flight
work table and worked traces label profile and test limits. Clarified one
cross-guide comparison to say “captured send-time source-region snapshot,”
avoiding confusion with a saved federation image, and changed the 2026-10-10
CTest references from “current build” claims to dated historical results. No
tests, builds, or CI were run; no new diagram was warranted.

2025 Exception Reporting evidence-link audit (2026-11-03): Checked every C++
source and test anchor in the guide against its current line. Several had
drifted after implementation files moved and now pointed at unrelated code or
headers. Repaired links for the exception hook and queue, the receive-order
time-state gate, embedded and process report planning and rechecks, the
separate client/server report requests, the MOM rejection path, and focused
2025 test cases. The existing prose and diagrams already explain the HLA
report families and embedded/process evidence limits, so no behavioral
explanation or chart needed revision. No C++ source or Requirements Lab
content changed; no tests, builds, or CI were run. The 2010 guides remain
untouched.

2025 Service Invocation Reporting evidence-link audit (2026-11-03): Checked
the guide's 94 local C++ source/test line anchors and repaired references that
had drifted across report routing, switch interlocks, regional-send validation,
timestamped-update queue/report ordering, process callback delivery, and the
ConnectionLost final-report path. Reworded old local-run failures as historical
evidence. The existing diagrams already explain destination choice, recipient
eligibility, and the embedded/configured-process timing boundary, so no chart
or behavioral explanation needed revision. No source or Requirements Lab
content changed; no tests, builds, or CI were run. The 2010 guides remain
untouched.

2025 Time Management request/grant/callback reader-gap review (2026-11-03):
Rechecked the role-enable state machines, accepted-advance lifecycle, grant
decision flow and comparison matrix, NMR/NMRA late-arrival sequence, and the
separate temporal-eligibility/callback-observation gates. The guide already
distinguishes caller boundary R from effective target E, request acceptance
from grant, eligible callback selection from application observation, and its
bounded embedded/process evidence. No additional flow or behavioral prose was
warranted. The 146 local line anchors resolve to existing files and
in-range lines. No source or Requirements Lab content changed; no tests,
builds, or CI were run. The 2010 guides remain untouched.

2025 Attribute Ownership negotiated-divestiture review (2026-11-03): Rechecked
the public void-service result, owner confirmation, requester notification,
ownership-query timing, the independent assumption-offer path, and the
embedded cancellation-after-confirmation sequence. The guide already keeps
process confirmation and embedded cancellation as separate evidence and does
not claim callback-order or profile parity. Corrected a process source link
that pointed at the cancellation planner instead of negotiated-divestiture and
confirmation handlers. All 229 local source/test line anchors resolve to
existing files and in-range lines. No new diagram was warranted; no source or
Requirements Lab content changed, and no tests, builds, or CI were run. The
2010 guides remain untouched.

2025 Attribute Value Update Request reader-gap review (2026-11-03): The
same-day §6.21 entitlement review is already recorded in the ledger and remains
open: the pinned API contains the declaration and exception list, but neither
local scenarios nor the official product listing provide the clause prose.
The broader guide survey found that its 12 diagrams already distinguish
provider solicitation, later value delivery, receiver eligibility, callback
observation, TSO timing, regional scope, and profile evidence. All 225 relative
destinations and 200 source/test line anchors resolve. No additional claim or
diagram was warranted; the normative requester-entitlement question stays open.

2025 Order and Transportation timing review (2026-11-03): Reviewed the three
changed charts for accepted-TSO metadata, pending transportation-change state,
and partial divestiture while confirmation is queued. Added the source-bounded
Save/Restore bridge: a state image copies the remaining pending attribute set,
and restore validates known attributes and current ownership. No focused test
covers partial divestiture followed by Save/Restore. Mermaid CLI 12.0.0 renders
and local screenshot review show the three charts readable without clipping.
All 100 relative destinations and 85 source/test line anchors resolve. No
implementation or Requirements Lab content changed; no tests, builds, or CI
were run. The 2010 stream remains untouched.

2025 Federation MOM member-report target-lifetime review (2026-11-03): The
embedded request planner copies the indexed module content and captures the
reported member's ID. Callback-time resolution rechecks that ID and the
receiving member; if the original target resigned before that check, the old
report is suppressed. A same-name rejoin gets a fresh member ID and cannot
revive it. The private HLAfederate snapshot may remain for pending Remove
callbacks, independently of report delivery. The registry lock is released
before `receiveInteraction`, so a concurrent Resign after the check has no
established ordering. Added a focused flow and a reciprocal lifecycle-guide
link; Mermaid CLI rendering and screenshot review were clear. The focused
member-report case does not resign its target before Evoke, and the separate
process rejoin case does not cover a queued FOM report. No source, Requirements
Lab, or 2010 content changed; no tests, builds, or CI were run.

2025 Federation MOM member-report requester-lifetime review (2026-11-03): The
embedded handler validates the requester at acceptance and resolves the
inherited `HLAfederate` target separately. The queued report retains the target
ID and each receiver ID, not the requester's ID. If a distinct requester
resigns while the target remains joined, independent observers' callback checks
remain eligible; a callback addressed to the requester cannot pass its receiver
membership check if it reaches that resolver after Resign. If requester and
target are the same member, the target gate applies. The focused successful
member-report case is self-targeted, and the
Table 7 S-leaf case only checks missing parameters. Added a role-specific
sequence and lifecycle-guide cross-link. Mermaid CLI 12.0.0 rendering and
screenshot review were clear. No source, Requirements Lab, or 2010 content
changed; no tests, builds, or CI were run.

2025 Federation MOM FOM requester-lifetime comparison (2026-11-03): The
federation-scoped MIM request has no member target. The embedded handler checks
the requester's membership at acceptance, but its plan retains module index,
content, routing, and recipients without a requester ID. The queued resolver
ignores the shared helper's reported-ID sentinel and rechecks each receiver's
membership and eligibility. The focused public case sends from a subject to a
separate observer and verifies unsubscribe-before-Evoke suppression, but does
not resign the requester before Evoke. Added a concise member-versus-federation
lifetime table and cross-link; the existing diagrams were sufficient. No
normative post-acceptance cancellation claim, source change, Requirements Lab,
or 2010 content; no tests, builds, or CI were run.

2025 Federation MOM MIM-data requester and profile check (2026-11-03): The
MIM-data request uses the same requester-at-acceptance and receiver-at-callback
identity shape as federation FOM reports. Its plan stores the MIM text, routing,
and recipients without a requester ID; the shared report helper again passes
an ignored sentinel to a receiver-only resolver. The focused test receives the
MIM report through a separate observer but does not resign its requester. The
configured-process source has no paired FOM/MIM report planner or test; the
existing guide already states that process boundary. No additional guide edit
was warranted, and no new cancellation or profile-parity claim was made. No
source, Requirements Lab, or 2010 content changed; no tests, builds, or CI were
run.

2025 configured-process receive-order versus attribute-reflection timing review
(2026-11-03): The Time Management guide already distinguishes the process
bridge's constrained/asynchronous/pending-advance gate for receive-order
interactions from the separate attribute-reflection submission route. The
focused process async cases exercise interactions; the ordinary reflection
case does not combine a constrained receiver, async switch changes, and a
pending advance. The source does not establish that the interaction gate is an
HLA rule for attribute reflections. The checked-in requirement contract maps
the async clauses but does not include their full prose, and no full standard
text is present in the workspace. The guide already states these limits, so no
duplicate explanation or diagram was warranted. No source, Requirements Lab,
or 2010 content changed; no tests, builds, or CI were run.

2025 Mermaid inventory reconciliation (2026-11-03): A fresh count of the 31
2025 topic-guide files found 363 diagrams: 207 flowcharts, 132 sequences, and
24 state diagrams, with 355 backtick-fenced and 8 tilde-fenced blocks. This
reconciles two member-report Resign diagrams omitted from earlier 360/361
snapshots: the target-lifetime flowchart and requester-lifetime sequence. Both
were rendered with Mermaid CLI 12.0.0 and screenshot-reviewed in their focused
reviews. The existing 336-chart sweep, focused checks, and those dedicated
renders cover the current set; the 78-chart selected visual queue has none
pending. Updated the roadmap and backlog counts; no new chart or behavior claim
was added. No source, Requirements Lab, or 2010 content changed; no tests,
builds, or CI were run.

2025 Time Management/Callback Ordering cross-review (2026-11-03): Rechecked the Time Management guide’s configured-process attribute-reflection route and receive-order interaction gate against Callback and Service Ordering. The callback guide directs timestamped-order readers to the Time Management guide; the Time guide distinguishes process attribute-event submission from the interaction bridge’s constrained/asynchronous/pending-advance gate. No contradiction or missing reader diagram was found; a combined chart would repeat the existing route diagrams. The reflection conclusion remains source-bounded, not normative, until §§8.15.3 and 8.16.5 can be checked against the canonical text. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 configured-process transport recovery review (2026-11-03): Added a focused
recovery sequence to the Process Endpoint guide. The source shows the client
resuming the same live session using bounded, in-memory sequence history; the
guide distinguishes host-queued synchronization events from pushed-frame
replay. Existing 2026-10-30 private recovery CTest results cover host-queued
announcement delivery, an in-flight Join response handled once, terminal loss
after a bounded window, and pushed event/response replay across repeated socket
loss. The interleaved case passed 28 assertions and ten repeated rounds (30
focused executions total). The new Mermaid CLI 12.0.0 chart rendered and passed
local screenshot review. These are private process-profile findings, not an
HLA resume rule, public-provider behavior, host-restart durability, or
conformance. No tests or builds were rerun.

2025 class-hierarchy publication review (2026-11-03): the existing cross-path
map already covered subscription matching, object discovery and value
projection, inherited interaction-parameter projection, and regional dimension
availability. Added the missing publication lane to that same map rather than
creating a duplicate chart. The embedded registry stores object publication by
class and checks the registration class; interaction sends check the exact sent
class. Existing focused registry tests use same-class publication and use, so
the parent-published/child-operated outcome remains untested and no normative
inheritance rule is claimed. Eleven new API/source/test line anchors resolve. The
revised chart rendered with Mermaid CLI 12.0.0 and passed local screenshot
review. No tests/builds were rerun.

2025 timestamped-delivery cross-review (2026-11-03): the Time Management guide
traces recipient-scoped queueing and the matching grant boundary, including one
attribute update splitting into RECEIVE and TIMESTAMP passels in the embedded
profile and a separate configured-process route. The Object and Interaction
guide distinguishes embedded callback-time projection from process
event-materialization-time projection. DDM keeps regional eligibility separate
from timestamp order and callback enablement; Callback and Service Ordering
limits FIFO to a local dispatcher and points TSO readers to the time guide.
These accounts agree and already show the necessary transitions. No combined
diagram was added. This review makes no normative claim about the open
`reflectAttributeValues` clauses and no profile-parity claim. No tests/builds
were rerun.

2025 ownership cross-guide review and visual follow-up (2026-11-03): the core
Attribute Ownership guide and its persistence/resignation companion already
cover negotiated and unconditional transfer, pending acquisition work, callback-
entry ownership transitions, selected restore dispositions, and resignation
preflight/cleanup. Existing limits correctly avoid claiming every ResignAction
combination or universal restore behavior. The core guide's negotiated-
divestiture cancellation sequence had not been rendered; Mermaid CLI 12.0.0
render and local screenshot review confirm readable, unclipped layout. Updated
the selected visual-QA queue from 80 to 81. No source, tests, builds, or CI were
run.

2025 Request Attribute Value Update entitlement cross-review (2026-11-03): cross-read the RAVU, Subscription State, Attribute Ownership, and Object and Interaction guides. Existing diagrams already distinguish requester solicitation, provider callback/update, ordinary receiver eligibility, and value delivery, so no synthesis chart was warranted. Embedded request planning checks requester membership and object knowledge, validates requested attributes, then groups work by current owner; ordinary reflection planning derives recipients from subscription and region state rather than the earlier request. The focused embedded baseline subscribes through the public service before RAVU; the configured-process case seeds the requester's subscription through its registry fixture. Neither focused RAVU case leaves the requester unsubscribed through the provider response. The separate DSE case has no RAVU request and only demonstrates delivery-time reevaluation of an ordinary planned update. The full §6.21 clause prose is absent from the local canonical copy, and the official IEEE listing requires purchase/subscription access, so direct requester entitlement remains an open normative question. Retain the source/test boundary. The inventory remains 364 diagrams across 31 guides; selected visual QA is 81 reviewed, 0 pending. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 synchronization/save-restore boundary cross-review (2026-11-03): compared the Federation Synchronization and Federation Save/Restore guides. Existing diagrams separate registration/announcement, participant achievement reports, federationSynchronized, and the save/restore participant protocol. The synchronization guide separately traces a pending point as typed state in the federation image and labels its partial-progress sequence source-derived rather than end-to-end tested. Save/Restore separately traces federate application work, RTI federation-image commit, and protocol-specific dispositions for in-flight work. No combined chart was warranted; multi-member restore with partial achievement remains unproven. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 timed-save/logical-time cross-review (2026-11-03): compared the Federation Save/Restore and Time Management guides. The save guide traces the timed-save cutoff and a focused case with a directed TSO message at time 8 and a save at time 6; later work remains pending across restore, then is delivered before the flush-queue grant. The time guide separately models restored coordinator state and grant eligibility, selected TSO payload/recipient-phase/retraction records, and live callback queues. The accounts agree and existing charts preserve these as distinct boundaries; no new chart was warranted. Keep the example bounded to the embedded HLA_EVOKED test and make no universal callback-order or normative persistence claim. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 voluntary-resign/connection-loss boundary cross-review (2026-11-03): compared the Federation Lifecycle, Federation Synchronization, Attribute Ownership, and Ownership Persistence and Resignation guides. The lifecycle guide keeps voluntary Resign admission/preflight and the caller-selected ResignAction separate from RTI-side connection-loss entry, per-member automatic directives, forced cleanup, and the last-member rule. Synchronization models departure as participant/announcement/achievement-set removal followed by barrier re-evaluation; it is not a failed achievement, and failedToSyncSet comes from remaining recorded false results. The synchronization guide explicitly separates barrier completion from object/ownership cleanup, while the ownership companion owns the pending-acquisition guard omitted from the core transfer guide. No synthesis chart was warranted. Gaps remain: no focused forced-loss-during-active-synchronization test, no process-profile runtime proof for that route, and the lifecycle guide records a source-observed process Save/Restore notification projection gap without asserting runtime outcome. No normative or cross-profile inference. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 object-deletion/ResignAction cross-review (2026-11-03): compared the Object and Interaction Information, Federation Lifecycle, and Attribute Ownership guides. They keep application delete request, receiver removeObjectInstance callback, and caller-only localDeleteObjectInstance distinct; the object guide further separates execution-wide delete-accepted state from each receiver’s known-object state and callback gate. The lifecycle guide treats DELETE_OBJECTS as action-selected cleanup before membership removal and treats connection loss as a separate forced entry with a stored per-member directive, forced overlay, and last-member rule. The ownership guide explains HLAprivilegeToDeleteObject as special per-instance authority and separates inherited FOM schema availability from runtime ownership. Existing flows and the cross-trigger comparison table agree; no synthesis diagram was warranted. Focused embedded tests cover voluntary DELETE_OBJECTS, local forget/rediscovery, and automatic-delete-on-loss; process removal-payload evidence is separate. Callback order remains scenario-bound, and no full normative action matrix or profile parity is inferred. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 interaction-subscription/DDM cross-review (2026-11-03): compared the Object and Interaction Information and Data Distribution and Regions guides. Both keep a global active-subscription relevance gate separate from each receiver’s matching subscription, class promotion, regional overlap, and parameter projection. The DDM guide separately treats class-available dimensions at declaration/send time and the later parent-chain candidate walk; this remains an implementation map, not a normative inheritance rule. For timestamped regional interactions, the guides agree that accepted send-region scope, current receiver subscription, TSO eligibility, and callback projection have distinct boundaries. Embedded delivery rechecks at callback; the configured process path stores its resolved projection in the event frame earlier. Evidence limits remain explicit: the inherited-class test uses empty parameter maps and no regions; the process regional case uses the exact sent class, and no process test was identified that mutates ordinary interaction subscriptions between TSO send and event creation. Existing diagrams were sufficient; no synthesis chart or normative/cross-profile claim was added. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 FOM admission/composition and name-resolution cross-review (2026-11-03): compared the FOM Module Admission and Composition, Federation Lifecycle, and Name and Handle Resolution guides. They agree on layered boundaries: per-module DIF validity, whole-model composition/reference resolution/FDD validation, Create preparation before federation-name reservation, additional-module Join preparation and shared-state commit, then execution-scoped typed name/handle lookup only after joining. The FOM guide also distinguishes atomic replacement-definition plus membership commit from later backend report/MOM setup and its membership rollback; it does not imply that a post-commit failure restores the prior model. Embedded tests show new declarations become resolvable to an existing member and earlier typed handles remain equal in those cases. Configured-process lookup coverage is narrower and does not establish that same existing-member/stability result. The guides correctly distinguish model catalog growth from discovery of existing application objects and from the separate RTI-owned HLAcurrentFDD MOM reflection; the process Join path lacks equivalent observed conditional reflection. Existing diagrams already make these boundaries clear, so no synthesis chart was warranted. Module-order, exact standard, and process-profile limits remain explicit. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 order/transportation and time-management cross-review (2026-11-03): compared the Order and Transportation, Time Management, and Object and Interaction Information guides. They consistently separate the timestamp-bearing service overload, effective sent-order designation, per-recipient received order, transportation category, DDM recipient selection, time eligibility, and application callback observation. A focused embedded interaction test shows one TIMESTAMP send observed as RECEIVE by a non-time-constrained receiver and as TIMESTAMP after the constrained receiver advances; the Time Management guide separately models recipient-scoped TSO queue entries. For attribute updates, effective per-attribute order and transportation create passels before receiver projection, and one timestamped call can split into RECEIVE and TSO routes. The order guide also traces accepted TSO messages retaining per-send order/transportation metadata across later setting changes; this remains an embedded-source observation. No contradiction or missing synthesis chart was found. The physical QoS and exact normative mapping from sent to received order remain explicitly unresolved. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 relevance/scope advisory cross-review (2026-11-03): compared the Declaration and Attribute Relevance and Attribute Scope Advisory guides with Object and Interaction Information. The guides distinguish three state owners and audiences: class-level declaration relevance reported to a publisher, per-instance/per-attribute relevance reported to an owner, and receiver-local scope for a known object attribute. One subscription or region mutation may feed more than one independent calculation, but the resulting callbacks are not substitutes for each other. Scope is compared as a receiver/object/attribute state transition after mutation and remains distinct from value reflection, discovery, and ownership transfer. The Attribute Scope guide keeps its final-self-unsubscribe Out exception isolated to that scope path; the Relevance guide explicitly warns against copying it into Turn Updates On/Off rules. Existing diagrams and cross-links agree, so no synthesis chart was warranted. The evidence limits remain clear: focused tests cover named embedded and process scope cases, while process class-level declaration advisory behavior, concurrency/races, and the complete standard trigger predicates remain unproven. No normative or 2010 parity claim. No source, Requirements Lab, 2010, tests, builds, or CI changed.

2025 Restore callback-route invariant review (2026-11-03): confirmed that the post-apply route checks can throw into the broad catch after `restoreOperation` is cleared, but the inspected admission, resignation, snapshot replacement, and bounded identity-rebind paths preserve the expected live-member route invariant. No accepted missing-route state was demonstrated; this remains a defensive guard with no injected-failure test or public-outcome claim.

2025 update-region association review (2026-11-03): added a DDM flowchart for whole-batch validation before active/deferred association maps are swapped, plus the distinct configured-process event-enqueue phase after registry commit. The existing 2025 stale-region case verifies `InvalidRegion`, then observes an out-of-scope transition from the pre-existing association under both callback models. Process post-commit enqueue failure has no focused test and is not an HLA exception-schedule claim. Mermaid CLI 12.0.0 rendered the new chart and local screenshot review found it readable. Inventory is 31 guides / 371 diagrams (211 flowcharts, 136 sequences, 24 state diagrams; 363 backtick-fenced and 8 tilde-fenced); selected visual QA is 89 reviewed / 0 pending. Four new DDM guide destinations resolve. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 late-subscription cross-review (2026-11-03): The Subscription State, Object and Interaction, and DDM guides already distinguish subscription admission, discovery of an existing but unknown object, initial selected stored-value reflection, and adding an attribute to an already-known object. The embedded callback-after-discovery path and process service-stage event path are shown separately, with test and normative limits beside them. A bounded source/test review found no missing diagram. Removed an obsolete note claiming the process discovery source was modified; the relevant C++ source and tests are clean in the current worktree. No tests, builds, or CI were run.

2025 regional class RAVU failure-schedule review (2026-11-03): Cross-read API 9.13, the embedded and configured-process adapters, the shared planner, process registration/event delivery, and focused regional-request tests. The guide distinguishes regional preflight failures from per-group projection and the embedded callback-time recheck. Its process charts show that Pull events may already be queued when a later group fails, Push frames are staged until preparation finishes, and a sequential Push send failure may follow an already-sent prefix. The API lists possible exceptions without defining precedence; the adapters check preconditions in different orders, and no combined-fault or injected group/send failure test was found. Existing diagrams were sufficient and were already locally rendered/reviewed; this review added no guide content. No tests, builds, CI, Requirements Lab, or 2010 work was performed.

2025 FOM module-index/content projection cross-review (2026-11-03): Verified that the Federation MOM guide separates the MIM's federation/member index meanings, embedded designator-list projections, report-content vectors, and composed Current FDD. The embedded 2025 list path filters/de-duplicates qualifying module content, while report-content vectors de-duplicate by canonical source path; current one-module public cases do not establish multi-module index alignment after Join. For these MOM requests, the configured-process adapter takes the generic interaction route; process-specific source and focused tests contain no paired FOM-report planner/case. Added a side-by-side chart comparing the two MIM index contracts with the separate embedded report payload vectors; the unverified list-to-vector mapping is explicit, and Current FDD remains distinct in prose. Mermaid CLI 12.0.0 rendered the chart and local screenshot review found it readable. No tests, builds, CI, Requirements Lab, or 2010 work was performed.

2025 configured-process callback-failure cross-review (2026-11-03): The shared dispatcher consumes an extracted callback task before invocation; if its exception escapes, HLA_IMMEDIATE unwinds the active drainer and HLA_EVOKED unwinds Evoke, while later dispatcher tasks remain queued. The process client likewise removes a pushed event before bridge submission; an immediate callback can interrupt the request thread after response matching but before that response reaches the service method, while evoked submission can return before callback entry. The process TSO bridge's callback-completion acknowledgement is skipped on an escaping callback, leaving the recipient in transit. Transport loss/recovery is a separate path, and producer-specific exception catches remain exceptions to the generic rule. Existing guides cover these boundaries and mark them as source-derived; focused callback/process tests do not exercise an application callback throwing. No guide change was needed. No tests, builds, CI, Requirements Lab, or 2010 work was performed.

2025 late-subscription current-value and diagram review (2026-11-03): Rechecked the three 2025 guides, subscription adapters, shared registry mutation and discovery planning, process event enqueue, API declarations, and focused embedded/process test sources. The source distinguishes discovery-triggered initial reflection for a receiver that did not know the existing object from subscription changes for a known object, which yield scope/relevance plans and do not re-enter ordinary discovery-scoped initial-value planning. The public known-object `PayRate` test observes the provider's relevance advisory but has no subscriber reflection callback or provider update; no callback outcome is claimed, and licensed §5.8 remains unreviewed. Reworked two existing diagrams for reader-sized horizontal layouts: one follows receiver knowledge and separate advisory/value lanes; the other compares embedded and process delivery for newly discoverable objects, keeping process MOM snapshot values separate. Mermaid CLI 12.0.0 rendered both charts; local screenshot review found labels readable and unclipped. The 2025 inventory remains 31 guides / 372 diagrams; selected visual QA is 92 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.


2025 current-value/provider-solicitation cross-review (2026-11-03): Cross-read the Object Attribute
Subscription State, Auto Provide, Attribute Value Update Request, and Joined-Federate MOM guides. The Auto
Provide comparison table already distinguishes discovery-triggered ordinary initial reflection, RTI-owned
MOM snapshot reflection, Auto Provide solicitation followed by a separate provider update, and
federate-requested RAVU followed by its separate provider update; it also separates scope and relevance
advisories. Embedded ordinary discovery and Auto Provide evidence is separate, and no focused case exercises
their combined order. Process source and test sources cover selected ordinary values on new discovery and
the separate MOM snapshot path, not process Auto Provide. The RAVU guide records embedded request-tag and
response observations and a configured-process case covering the selected request/provider-update/reflection
chain under both callback models; the RTI-owned MOM request reflects its snapshot directly. Existing guides
answer the reader question, so no synthesis chart or guide edit was warranted.
Already-known-object/add-attribute current-value behavior and licensed §5.8 remain open. No source, tests,
builds, CI, Requirements Lab, or 2010 content changed.


2025 time-role enable/disable cross-review (2026-11-03): Cross-read the Time Management, Federation and
Federate Lifecycle, and Callback and Service Ordering guides; pinned IEEE 1516.1-2025 declarations; the two
time-role adapters and FederateTimeState; and existing focused-test sources. The two independent role
diagrams already distinguish an accepted pending enable from the active role at its enable callback, and an
active role from the immediate Disable transition. Existing prose explains simultaneous regulating and
constrained roles, active lookahead, enable effects on advance legality, and grant re-evaluation. The
process receive-order diagram separately shows disabling constraint releasing deferred callbacks; the
special TSO-to-RECEIVE conversion boundary stays explicit and is not generalized. The existing public
process test source demonstrates callback-model gating for time-role enable; no tests were rerun. A combined
state chart would duplicate the separate role and delivery flows. Added direct links from the role section
to the four 2025 service declarations and two enable callbacks. Exact normative callback semantics remain
bounded by the available API, not a clause-prose review. No source, tests, builds, CI, Requirements Lab, or
2010 content changed.


2025 pending time-role Save/Restore cross-review (2026-11-03): Cross-read the Time Management and Federation
Save/Restore guides, state-image capture/restore, embedded role-dispatch reconstruction, and focused tests.
The time-state image stores pending regulation/constrained role generations separately from live callback
closures. After applying the image, the embedded registry rebuilds role-enable dispatches through the
current member callback factory and callback epoch, so stale pre-restore work cannot consume the restored
request. Revised the existing Save/Restore chart to show that branch alongside the independent
pending-advance, selected-TSO, and live-callback-queue paths. Two focused public embedded cases separately
restore one pending role each from a filesystem-backed image through a fresh registry in one test process;
after explicit HLA_EVOKED delivery, each observes one role-enable callback and one federationRestored
callback, without asserting their relative order or OS-process restart. A registry test rejects the stale
callback epoch. Source sorts multiple restored role callbacks by saved generation, but no focused case saves
both enables pending together. No focused configured-process pending-role restore test was identified.
Mermaid CLI 12.0.0 rendered the revised chart and local screenshot review found the branches readable.
Inventory remains 31 guides / 372 diagrams; selected visual QA remains 92 reviewed / 0 pending. No tests,
builds, CI, Requirements Lab, or 2010 content changed.

2025 receive-order gate and TSO attribute projection cross-review (2026-11-03): Cross-read the Time
Management, Data Distribution and Regions, and Callback and Service Ordering guides and their cited
evidence. The time guide separates the constrained receive-order interaction gate from TSO logical-
time eligibility: a pending advance opens the interaction callback gate before the grant, while TSO
remains at its logical-time boundary. The configured-process interaction gate is bridge-local;
process attribute reflections use a separate submission path, and the cited interaction tests do not
combine constrained state, async-switch changes, and a pending advance with an ordinary attribute
reflection. For timestamped attributes, the guide distinguishes embedded callback-dispatch
projection from process event-materialization projection against retained source scope; no focused
process case mutates a subscription or region between acceptance and event creation. DDM owns the
detailed regional projection path, and the callback guide delegates TSO ordering. Existing
explanations and diagrams answer the reader question; no guide or diagram change was warranted. No
tests, builds, CI, Requirements Lab, or 2010 content changed.

The FOM module-index cross-review is already completed and charted in the earlier roadmap entry
above (2025 FOM module-index/content projection cross-review); its active pointer was stale, so this
update records the disposition instead of repeating that review.

2025 mixed-order attribute-passel evidence correction (2026-11-03): The cross-guide audit found
tracked embedded Catch2 cases in ieee1516_2025_mixed_order_attribute_passels_catch2.cpp; the Time
Management guide statement that no one-receiver mixed-order test existed was stale. The first
HLA_EVOKED case configures two reliable attributes with different default order, enables
asynchronous delivery for a constrained receiver, and asserts that one timestamped Update Attribute
Values call produces two separate reflections: RECEIVE/RECEIVE before receiver TAR, then
TIMESTAMP/TIMESTAMP before grant, with a retraction designator only on TSO. The second case retracts
after RECEIVE has been observed, preserves that delivered passel, suppresses queued TSO reflection,
and still observes the grant. Updated the Time Management guide with both exact cases and their
limits; added a reciprocal mixed-order link from Order and Transportation. The test source asserts
no sent region designator, so it does not exercise a region-driven split. No corresponding process
mixed-order test was found in the reviewed candidate set; the process route remains source-derived,
with no profile-parity or HLA-wide ordering claim. Read the test source but did not execute it. No
new diagram was needed. No builds, CI, Requirements Lab, or 2010 content changed.

2025 timestamped regional-interaction projection cross-review (2026-11-03): Cross-read the Data
Distribution and Regions, Time Management, Object and Interaction, and TSO Retraction guides. The
embedded subscription-replacement case retains the accepted send-region snapshot, changes the
receiver subscription before time 6, observes grant 6 without interaction or Request Retraction,
then observes a later source-B send once after grant 6 with timestamp order and a valid designator.
The Object and Interaction guide distinguishes embedded projection at callback dispatch from
configured-process projection before event construction; the process source rechecks the queued
recipient, while its focused regional test establishes the subscription before sending. Time and TSO
Retraction guides keep eligibility, callback/grant order, and per-recipient retraction separate. No
cross-guide contradiction or missing reader transition was found, so no chart or prose change was
warranted. No tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 Federation MOM HLAsetSwitches chart validation (2026-11-03): Extracted the existing 2025
HLAsetSwitches control-flow chart from the Federation MOM guide and rendered it with Mermaid CLI
12.0.0. Local screenshot review found both MIM class paths, member parameter branches, malformed-
adjustment rejection, and the Service Reporting interlock readable and unclipped. The diagram is
tall but its branches remain traceable. This validates local syntax and layout only; GitHub
rendering remains unverified. No diagram edit, source change, test, build, CI, Requirements Lab, or
2010 work.

2025 Data Element and delivery-evaluation visual review (2026-11-03): rendered and locally screenshot-reviewed all eight diagrams in the 2025 Data Element Encoding guide, plus the process-event transport chart in Attribute Scope and the configured-process attribute timing chart in Delay Subscription Evaluation. This resolves the pending composite, logical-time, revised variant, Attribute Scope transport, and DSE process-attribute chart reviews. A local Markdown reference audit checked 112 destinations in the Data Element guide with no missing files, anchors, or line targets. Local visual checks establish syntax and layout only; GitHub rendering remains unverified. The curated selected visual-QA inventory is 97 reviewed / 0 pending. No guide behavior, source, tests, builds, CI, or Requirements Lab changed.

2025 subscription/update-rate/DSE cross-review (2026-11-03): traced the route-only DSE candidate through fresh receive-order and TSO delivery projections, which copy the receiver's current subscription generation into the per-attribute rate key. The regional rate case directly observes immediate admission after unsubscribe/resubscribe; focused DSE tests separately observe late eligibility and unsubscribe suppression. No focused test combines DSE retention with rate-history admission. Added source-derived guidance and reciprocal links, clarified test scope, and corrected stale line anchors; no new diagram was warranted. The combined result is not an end-to-end observation or an HLA requirement. No tests, builds, or CI.

2025 ownership/TSO/callback cross-review (2026-11-03): the Ownership and TSO Retraction guides agree on two different events: an acquisition notification can report a new owner, while a previously accepted TSO reflection retains the former producer identity. The focused embedded `HLA_EVOKED` test observes this queued update after unconditional divestiture and If-Available acquisition; it does not exercise regular acquisition pending against a still-current owner. Callback/Service Ordering now links these protocol-specific commit and delivery boundaries without implying a global order. Time Management already links the TSO case. No chart or test run was needed.

2025 first-reflection-after-discovery review (2026-11-03): Cross-reviewed Subscription State,
Object/Interaction Information, and Callback and Service Ordering. The guides consistently separate
which current values are selected, when the embedded and process paths plan the initial reflection,
and when the federate application actually invokes callbacks. The process pull selector prioritizes
a pending discovery before the initial-reflection event slot; the HLA_EVOKED public test requests
discovery then reflection in that fixture. Its pushed HLA_IMMEDIATE branch observes both callbacks
without an ordered trace. The embedded case observes discovery and selected values but stores them in
separate reports, so it does not establish relative order. The source comment cites official ordering,
but the pinned headers do not provide clause prose to independently check §§5.8, 6.9, and 6.11.
No cross-profile or HLA-wide callback-order claim is made. Added a reciprocal link from Callback and
Service Ordering to the service-specific explanation; existing diagrams already show the state and
callback boundaries, so no new diagram was needed. No tests, builds, CI, Requirements Lab, or 2010
work.

2025 FDD Excluded-dimension review (2026-11-03): Compared the DDM guide, pinned FDD schema, composer, region registry, excluded-dimension fixture, and TCK scenario source. Clarified that `Excluded` yields no composer `defaultRange`; class-available dimensions are collected independently, and an explicitly selected region dimension remains possible. The unexecuted TCK scenario source checks `{X,Y}` class availability and expects an X-only source to discover/reflect to an X receiver while a Y-only receiver is suppressed for having no shared dimension. The schema token pattern contains no `Not Available` value; unavailable-to-class describes a dimension absent from the class associations. Added a focused flowchart and checked its local references. Mermaid CLI 12.0.0 rendered it; screenshot review found it readable and unclipped. This is local syntax/layout QA, not GitHub verification, and the fixture source was not executed. Inventory is 31 guides / 373 diagrams; selected visual QA is 98 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 ResignAction source/evidence review (2026-11-03): A bounded non-timing survey of the guide backlog and lifecycle guide identified the six-action decision path as an explicit reader gap. Added an implementation-scoped map of the six pinned API enum values to the current embedded registry's cancel/delete/divest flags, pending-acquisition and remaining-ownership guards, callback-route preflight, cleanup order, and membership removal. The final-member delete pass is a separate overlay; forced connection loss remains a distinct configured-action path. Focused embedded tests cover representative cases for every enum value, but not the full action-by-ledger cross-product or HLA_IMMEDIATE action-specific ordering. Current source comments cite §§4.12.3/.4; the pinned headers lack clause prose, so the guide makes no independent normative decision-table claim. Mermaid CLI 12.0.0 rendered the new flow and local screenshot review found it readable. Inventory is 31 guides / 374 diagrams; selected visual QA is 99 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 configured-process time-role callback boundary review (2026-11-03): The service-side constrained role commits before the enabled-time response or delayed event is sent. Delayed eligibility requires available bounds, defined GALT, and current time strictly below GALT. The client bridge has a separate receive-order gate before callback entry; HLA_EVOKED defers application observation. Added a source-labeled process sequence and clarified that the existing Time Management state chart is the embedded dispatch path. Public endpoint tests cover callback routing with controlled servers; the production process-service GALT branch remains source-traced, while the cited GALT boundary test is embedded-only. No normative or cross-profile parity claim. The new diagram was locally rendered and screenshot-reviewed; inventory: 31 guides / 378 diagrams, 139 sequences; selected visual QA: 103/0. No source, tests, builds, CI, Requirements Lab, or 2010 work.

2025 configured-process Update Attribute Values failure-map correction (2026-11-03): The earlier map incorrectly sent every non-applied registry plan through generic transport rejection. Source review shows that only the session/federation/member mismatch uses that rejected transport path and becomes `RTIinternalError`; valid-session planner statuses return as structured results and map unknown object, undefined attribute, not-owned, federation/member loss, and inconsistent status to their respective typed public exceptions. Reworked the Process Endpoint diagram to show the separate branches, and removed the duplicate failure flow from Object/Interaction in favor of a linked summary. Mermaid CLI 12.0.0 rendered the revised chart; local screenshot review found its branches readable and labels unclipped. The focused public process test covers successful untimed updates only; negative public outcomes remain source-derived. Inventory is 31 topic guides / 378 diagrams (214 flowcharts, 140 sequences, 24 state diagrams); selected visual QA remains 104 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 configured-process ordinary/directed interaction-send failure review (2026-11-03): Corrected the ordinary Send Interaction diagram, which had mistakenly labeled remote registry-plan failures with Update Attribute Values exceptions. Both a session mismatch and a non-applied ordinary plan return generic rejection and become `RTIinternalError`. Added a distinct Send Directed Interaction map: a locally invalid target handle throws `ObjectInstanceNotKnown`, while a remote non-applied plan collapses to generic rejection. Focused public process cases establish successful sends only; negative remote outcomes remain source-derived. Both diagrams rendered with Mermaid CLI 12.0.0 and passed local screenshot review. Checked 27 local destinations (26 line anchors and one heading anchor). Inventory: 31 guides / 379 diagrams (215 flowcharts, 140 sequences, 24 state diagrams); selected visual QA is 105 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 work.

2025 configured-process ordinary/directed interaction-send failure review (2026-11-03): Corrected the ordinary Send Interaction diagram, which had mistakenly labeled remote registry-plan failures with Update Attribute Values exceptions. Both a session mismatch and a non-applied ordinary plan return generic rejection and become `RTIinternalError`. Added a distinct Send Directed Interaction map: a locally invalid target handle throws `ObjectInstanceNotKnown`, while a remote non-applied plan collapses to generic rejection. Focused public process cases establish successful sends only; negative remote outcomes remain source-derived. Both diagrams rendered with Mermaid CLI 12.0.0 and passed local screenshot review. Checked 27 local destinations (26 line anchors and one heading anchor). Inventory: 31 guides / 379 diagrams (215 flowcharts, 140 sequences, 24 state diagrams); selected visual QA is 105 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 work.

2025 configured-process timestamped-send validation review (2026-11-03): The ordinary time-overload service checks the sender's applicable logical-time-plus-lookahead floor and returns `invalid_logical_time`; the public adapter preserves this as `InvalidLogicalTime`. The focused process FQR-arrival test sends at time 4 after a grant at time 5 and asserts that exception. The directed service validates finite encoded time, then selects queueing based on producer regulation, but has no visible call to the sender-floor validator; the registry queue validates finite time/implementation and recipient/message identity, not that floor. The directed adapter maps non-OK client results to `RTIinternalError`; its focused process TSO test covers successful delivery only. Added a bounded comparison and a reciprocal Time Management link; no new diagram was needed. This is source/test evidence only, not a claim that a below-floor directed send is accepted or a normative conformance finding. No tests, builds, CI, Requirements Lab, or 2010 work.

2025 three-role object-attribute discovery cross-review (2026-11-03): The Subscription State guide already explains and charts the source-derived A/B case: a federation-wide active-relevance scan and a receiver-local declaration match are separate predicates, while later value eligibility is recalculated for each updated attribute. The shared registry plans discovery and rechecks eligibility when it commits receiver-known state. Embedded commits as the callback task starts; configured-process source performs the same registry transition before exposing the event to the client. The embedded passive test has only a publisher and one subscriber; the process subscription test records one member's declarations and has no ordinary object or separate active establisher. No focused three-role test exists. The DDM guide explains independent region qualification but not the active-relevance split; DSE explains the interaction analogue and does not establish object behavior. Added cross-links and made the candidate-versus-commit boundary explicit; the existing chart was sufficient. Checked 15 changed local destinations (nine source anchors, two test anchors, and four guide references); all resolve. Inventory remains 31 guides / 385 diagrams; selected visual QA remains 111 reviewed / 0 pending. No tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 passive object-discovery class-depth cross-review (2026-11-03): The active-relevance and receiver-selection scans each walk from the registered class toward its parents. The active scan stores only a boolean; the receiver scan returns its own closest eligible class. The public C++ TCK scenario asserts base and derived subscribers receive their respective class views and matching `getKnownObjectClassHandle` values, but both subscriptions are active. It therefore supports per-receiver class views, not the passive cross-depth composition. The configured-process discovery test uses the same class and attribute at sender and receiver; no process cross-depth/passive case was identified. Added this boundary to the existing Subscription State explanation and evidence map, and corrected the TCK scenario anchor in the Object/Interaction guide and review ledger. No new chart was needed. Inventory remains 31 guides / 385 diagrams; selected visual QA remains 111 reviewed / 0 pending. No tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 discovered-class stability review (2026-11-03): Added a sequence diagram for a derived object first discovered at a subscribed base class and a later more-specific subscription. The C++ TCK catalog maps the unchanged discovery and `getKnownObjectClassHandle` result to IEEE 1516.1-2025 §5.1.3; its source scenario also checks the publisher's derived-class view and correlates ambassador-local handles by object name. The embedded discovery planner skips receivers that already know the object, and discovery-state commit has the same known-instance guard. The catalog marks this scenario adapter-required; this documentation pass inspected source only and did not run it. No configured-process runtime result or parity claim is made, and current-value reflection from a subscription-only change remains a separate unresolved question. Mermaid CLI 12.0.0 rendered the new sequence and local screenshot review found it readable. Inventory is 31 guides / 386 diagrams; selected visual QA is 112 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 object-instance knowledge lifecycle review (2026-11-03): The existing object guide already connected registration/discovery, receiver-specific value eligibility, receive-order deletion/removal, timestamped per-recipient removal, and local forget. Added the missing caller-local forget branch to its existing existence/receiver-knowledge chart and linked it to the detailed local-delete guards. The focused embedded source scenario confirms successful local deletion makes the caller's known-object query fail, leaves the federation object available for rediscovery, and permits a later owner update; the process endpoint case covers success only. The overview remains explicitly embedded-source scoped, and timestamped removal/retraction timing stays in its separate flow. Mermaid CLI 12.0.0 rendered the revised chart and local screenshot review found it readable. Diagram inventory remains 31 guides / 386 diagrams; selected visual QA remains 112 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 object-name reservation-to-registration cross-review (2026-11-03): The name-reservation guide already separates reservation outcome, successful named registration, and peer discovery; the Object/Interaction guide links back to it for reservation ownership and commit-time consumption. The registry validates the reservation owner and current name occupancy, inserts the instance and name indexes, then consumes the reservation; a failed preflight preserves it. The embedded named-registration scenario exercises the full reservation-to-peer-discovery path, failed-registration retention, and bulk partial-reserve/atomic-release cases. The configured-process tests cover reservation and named registration separately from discovery, and do not assert cross-session observation order. Added reciprocal navigation from the HLA-level reservation map to the object registration/discovery and receiver-known-state maps; no new diagram was needed. No test was run. Inventory remains 31 guides / 386 diagrams; selected visual QA remains 112 reviewed / 0 pending. No source, builds, CI, Requirements Lab, or 2010 content changed.

2025 named-registration owner-result/discovery timing review (2026-11-03): Added a profile- and callback-model-specific timing table to the name-reservation guide. The embedded `HLA_EVOKED` named test observes the owner's returned handle before the test explicitly evokes peer discovery; the embedded `HLA_IMMEDIATE` source can drain inline, but its focused immediate case uses unnamed registration. The configured-process service enqueues discovery before constructing its response; process tests observe discovery after registration returns but do not measure whether immediate callback entry could precede that return. The process named-registration case has no peer, so no named reservation-to-discovery end-to-end timing claim is made. The four event boundaries—commit, owner handle, discovery submission, and peer callback entry—remain distinct. No new diagram was needed. No test was run. Inventory remains 31 guides / 386 diagrams; selected visual QA remains 112 reviewed / 0 pending. No source, builds, CI, Requirements Lab, or 2010 content changed.

2025 discovery-callback exception/state review (2026-11-03): Added a source-labeled embedded/process comparison flow to Object and Interaction Information. Embedded discovery clears its pending route and commits receiver-known class when the callback task starts; the process service commits receiver-known state before exposing the event, and the client consumes the event before bridge submission. A throwing callback consumes its task and does not itself roll back known state or replay the event. If an embedded `HLA_IMMEDIATE` exception unwinds synchronously through the discovery-submission loop, that loop clears still-pending routes from the current and later planned recipients while preserving committed state. If a callback API `Exception` runs inline on a named-registration stack, the object commit and successful-report attempt precede the owner's catch/report/rethrow, so that call can throw after commit without returning its handle; this is source-derived and untested. `HLA_EVOKED` and other-thread immediate exceptions follow the general callback guide. Focused discovery tests cover successful callbacks only; no throwing-discovery test was found. Mermaid CLI 12.0.0 rendered the new chart and local screenshot review found it readable. Current inventory is 31 2025 guides / 387 diagrams (220 flowcharts, 143 sequences, 24 state diagrams; 379 backtick-fenced, 8 tilde-fenced); selected visual QA is 113 reviewed / 0 pending. No tests, builds, CI, Requirements Lab, or 2010 content changed.

## 2025 Save/Restore member-exit adapter review — 2026-11-03

Voluntary Resign and forced ConnectionLost share the registry's member-exit
transition. Save failure reasons differ by trigger; Restore uses
`FEDERATE_RESIGNED_DURING_RESTORE` for both. The existing Lifecycle chart now
shows the operation choice, notification planning, operation reset, and the
embedded versus configured-process projections. Save/Restore admission keeps
the protocols mutually exclusive.

The embedded paths submit both planned notification types. Configured-process
Resign and ConnectionLost use the shared dispatcher, which enqueues Save
notifications but does not enqueue Restore notifications. The latter is a
source-level projection gap before event encoding, not a runtime callback
claim. Corrected Restore-test links in the lifecycle and Save/Restore guides
and added a cross-reference from the Process Endpoint queue explanation.

Mermaid CLI 12.0.0 rendered the revised chart and local screenshot review found
it readable. No tests, builds, CI, Requirements Lab, or 2010 content changed.
The next bounded review is the process NMR/NMRA grant re-evaluation after TSO
retraction, as described in the [behavior-flow guide index](HLA-BEHAVIOR-FLOW-GUIDES.md).

## 2025 process NMR/NMRA re-evaluation after TSO Retract — 2026-11-03

The configured-process Retract handler removes queued and unacknowledged
in-transit TSO work without directly calling grant reevaluation. A later
acknowledgment for the retired identity is accepted as already completed and
still passes through scheduler reevaluation. Accepted time-advance requests,
lookahead changes, and time-role changes provide other scheduler entry points.
Queue removal does not restore the NMR/NMRA effective target lowered by an
earlier TSO admission.

The directed configured-process NMR/NMRA cases do not invoke Retract. A
separate process attribute case observes a TAR(5) request, update(5), producer
TAR(2), successful Retract, then producer TAR(5); the receiver sees no
reflection and grants at 5 after draining retracted callback work. Updated the
Time Management, TSO Retraction, and Process Endpoint evidence notes to record
that adjacent TAR result without extending it to NMR/NMRA. The existing
source-trace diagram already presents the scheduler triggers and untested
combined outcome, so no chart or render was added. No tests, builds, CI,
Requirements Lab, or 2010 content changed.

Next bounded review: Trace configured-process TSO completion after a successful
pre-callback Retract across pull polling and pushed delivery. Compare client
pending-event removal, callback-bridge suppression, retired-message
acknowledgment, and scheduler reevaluation with focused 2025 tests; keep
callback-order and process/embedded parity unclaimed.

## 2025 process TSO acknowledgment after pre-callback Retract — 2026-11-03

The process client branches on whether an original interaction/removal event
remains in `pendingEvents_` or has been submitted to the callback bridge. The
first path removes it and defers TSO acknowledgment; the bridge path suppresses
an unstarted original callback and acknowledges tracked delivery, or queues
`requestRetraction` after callback start. Attribute updates retain their
pending event and use the bridge's retraction state. Callback completion and
tracked suppression both reach the private TSO completion path, with nested
ACK requests deferred until the active service exchange releases.

The focused process interaction test covers pre-poll, post-poll/pre-start, and
after-entry Retract in pull mode. It observes the private ACK eventually in the
post-poll/pre-start phase but accepts two API fences in either relative order.
After callback entry, it observes one interaction and one
`requestRetraction`; the server waits for the callback-start fence and ACK
before producer Retract. A separate pushReceiveOrderEvents-enabled
Resign/rejoin case observes no attribute/removal callback after rejoin but does
not assert TSO acknowledgment. Updated the existing TSO Retraction guide's
test boundary; its client-buffer-versus-bridge sequence already covers the
concept, so no new chart or render was needed. No tests, builds, CI,
Requirements Lab, or 2010 content changed.

## 2025 process grouped timestamped-attribute completion — 2026-11-03

Reviewed the service projection and callback bridge for one timestamped
attribute update that produces multiple recipient callbacks. The service marks
all projected passels except the last `more` and the last `final`; the bridge
uses only `final` as the message-level TSO acknowledgment boundary. If Retract
arrives before `final` is submitted, the bridge defers ACK through suppression
of `more` and acknowledges when the `final` task is suppressed. Source has a
different earlier completion point if Retract arrives after `final` submission
but before callback start. If projection yields no events or the callback gate
declines the group, the service completes delivery directly.

The focused internal bridge test drives synthetic groups and verifies the
HLA_EVOKED Retract-before-final-submission sequence. It does not test the
after-submission branch, service projection, or transport ordering. Added this
boundary to the Process Endpoint guide and linked it from TSO Retraction; the
existing completion sequence remained sufficient, so no new chart or render
was needed. No tests, builds, CI, Requirements Lab, or 2010 content changed.

## 2025 process TSO acknowledgment transport deferral — 2026-11-03

Traced bridge completion through the process client's private ACK handler. If
`requestDepth_` is nonzero, the client deduplicates and queues the message ID;
otherwise it sends `acknowledge_tso_delivery` immediately. The depth covers an
outstanding correlated transport response, not the whole public C++ service
call. After a request response, the client drops the depth and unlocks the
exchange before dispatching buffered pushed events, so an enabled immediate
callback can send its ACK before the enclosing public method returns. Queued
ACKs flush after response/event dispatch, through the background dispatch
loop, or before Evoke One/Multiple. The Time Management guide now links both
the final-passel and client-transport boundaries from its TSO grant-frontier
discussion.

The focused public process fan-out test uses HLA_EVOKED and expects each
timestamp-5 ACK before that receiver's later TAR(7). It does not force the
active-request queue branch or measure ACK order against the public TAR(5)
return. The synthetic callback-bridge test does not instantiate the client.
Updated the existing Process Endpoint ACK sequence, rendered it with Mermaid
CLI 12.0.0, and reviewed the local screenshot. Inventory remains 31 guides / 387
diagrams; selected visual QA is 114 reviewed / 0 pending. No tests, builds, CI,
Requirements Lab, or 2010 content changed.

## 2025 Join during active Save/Restore source recheck — 2026-11-03

Rechecked the Join boundary in the Save/Restore and lifecycle guides against
the current 2025 API declarations, embedded adapter/registry, process
`handleJoin`, and representative interlock test. Both Join overloads list
`SaveInProgress` and `RestoreInProgress`; the annotations do not define the
full admission rule. The inspected embedded and process Join paths have no
visible active-save/restore phase predicate. The process `joinMutex_`
serializes Join preparation and membership commit with other Join calls; it
does not establish arbitration with either protocol. The representative
interlock case joins both members before Save and tests other service families
during Save/Restore. It does not attempt Join while either protocol is active.

The existing evidence table and lifecycle participant-set map already explain
these boundaries and correctly leave the contract open. Repaired stale process
`handleJoin` links in the Save/Restore guide, the cross-cutting guide, and its
interaction-routing review ledger; the links now target the current handler
and registry-call locations. No additional diagram or behavioral prose was
warranted. No tests, builds, CI, Requirements Lab, or 2010 content changed.

Next bounded review: Trace the 2025 configured-process Region Save/Restore
path against the DDM and Save/Restore guides. Separate region-definition and
association image state from process endpoint transport; compare the source
path with focused public tests, and update a guide only if a reader gap
remains. Keep process Region persistence unverified where no endpoint test
exists. No tests or builds.

## 2025 configured-process Region Save/Restore boundary review — 2026-11-03

Compared the configured 2025 process client and service with the registry's
typed state-image capture, codec, and final Restore-completion path. Region
records carry their handle, owner, dimensions, pending and committed ranges,
and specification flags. Restore validates the live owner, current DDM
catalog, dimensions, and bounds before installing the candidate Region map.
Regional declarations, object associations, and queued-message snapshots are
separate image state.

Made the process-service boundary explicit in the Save/Restore guide: its
per-session callback queues are handled separately, and queued regional
attribute callbacks are rejected before registry Save admission in both
polling and pushed paths. The focused process Save, successful Restore, and
Restore-status cases do not create or query Regions. The file-backed Region
case uses a second registry object in the same test process, so configured
process Region persistence remains unverified. Repaired stale source line
links in the existing process-to-image map. No diagram changed; no renderer
pass was needed. No tests, builds, CI, Requirements Lab, or 2010 content
changed.

Next bounded review: Trace 2025 subscription expansion for an object the
receiver already knows. Determine when a newly declared subscribed attribute
becomes visible and whether a current-value reflection follows. Compare
embedded and configured-process source with focused public tests and the
Object/Interaction and Subscription guides. Keep outcomes open where evidence
does not establish them; revise only if a reader gap remains. No tests or
builds.

## 2025 guide-set integrity screen — 2026-11-03

Scanned all 31 current 2025 topic guides. Across 4,916 local Markdown links,
4,067 `#L` line anchors and 435 Markdown heading anchors, no local target was
missing, no line anchor was out of range, and no heading anchor failed to
resolve. External URLs were not checked.

A static coverage screen found at least one Mermaid diagram, pinned 2025
API/section references, source links, test links, and a limits/evidence section
in every guide. Twenty-nine guides mention the 2010 boundary. The two that do
not—Time Management and Delay Subscription Evaluation—are explicitly 2025
guides. This screen does not verify every diagram's meaning or establish full
semantic completeness.

The prior next pointer named the known-object subscription expansion review.
The current roadmap already records that review, including the two rendered
and screenshot-reviewed diagrams, so no duplicate edit was made.

## 2025 three-guide semantic coverage audit — 2026-11-03

Reviewed the FOM Module Admission and Composition, Object and Interaction
Information, and Data Distribution and Regions guides for 2025 authority and
API concepts, HLA state and decision explanations, Umbra transitions and
callbacks, edge cases, focused-test evidence and limits, and edition boundaries.
All three already make these distinctions clear; no semantic diagram or
behavioral explanation was missing. The actionable reader gaps were stale validation wording in the DDM footer and
two Object/Interaction notes, plus a broken FOM-to-name-guide heading anchor.
Corrected those notes and the link, then reconciled the selected screenshot-
review count from 115 to 116. Rechecked 1,100 local links and 1,013 fragments
across the three guides; all resolve. All 387 current
2025 diagrams have syntax/local-render coverage; the 116-chart screenshot queue
is curated, not a whole-inventory visual sign-off, and GitHub rendering remains
unverified. No diagram, source, tests, builds, CI, Requirements Lab, or 2010
content changed.

The three-role discovery follow-up is completed below.

## 2025 three-role object-attribute discovery review — 2026-11-03

The Object Attribute Subscription, DDM/Regions, and Delay Subscription
Evaluation guides already explain the boundary. The subscription guide's
sequence and state flow distinguish discovery relevance from per-attribute
reflection; the DDM guide links that case while keeping region overlap
separate; the DSE guide prevents its interaction example from being read as
evidence for object discovery. No new prose or diagram was needed.

The embedded registry scans joined non-producer declarations over the
registered object's owned, class-valid attributes and applicable region
relationships to establish discovery relevance. It then independently checks
the receiver's declaration and receiver-side class/region match. The discovery
predicate does not retain the establishing attribute or class, so active
relevance through A can allow a passive receiver match through B. This is an
Umbra source-derived possibility, not a normative conclusion. A later B
reflection runs a separate per-attribute relevance check; active relevance
through A alone does not make B active for delivery.

Embedded discovery rechecks the candidate when its queued callback begins and
commits receiver knowledge before invoking the callback. The configured-process
service uses the same registry selection logic but rechecks/commits while
staging the event, before it is exposed to the client. This distinction does
not establish matching callback timing. Existing evidence is narrower: the
embedded passive-subscription test has one publisher and one subscriber; the
portable per-federate class-view case uses active subscriptions; process
subscription cases do not introduce a separate active establisher plus passive
receiver. No focused test establishes the three-role cross-attribute outcome
in either path.

Retargeted three source references from the discovery function signature to
the active-relevance scan. A link/fragment check across these three guides
found 619 local links and 558 fragments, all resolving. No diagram, source,
test, build, CI, Requirements Lab, or 2010 content changed.

The Federation Lifecycle, Synchronization, and Save/Restore audit is
completed below.

## 2025 Federation Lifecycle, Synchronization, and Save/Restore semantic audit — 2026-11-03

Reviewed the [Federation Lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#a-join-does-not-define-every-active-protocols-participant-set), [Synchronization](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#member-departure-can-close-a-pending-barrier), and [Save/Restore](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#member-exit-during-an-active-restore) guides as a related set. Their central distinction is clear: federation membership, a synchronization point's participant set, and a Save/Restore operation's participating members are separate state.

The guides trace Join separately for expandable synchronization points and active Save/Restore, identify ResignAction cleanup separately from forced ConnectionLost, and show how departure changes a pending barrier or active federation operation. They preserve embedded callback submission versus configured-process event enqueueing, label enqueue/call order without claiming callback-entry order, and keep source-only behavior distinct from HLA requirements. Save/Restore also records its process member-exit Restore-notification projection gap without claiming a runtime client outcome.

Focused evidence is bounded as stated in the guides: the synchronization departure test covers voluntary Resign under one embedded `HLA_EVOKED` schedule; no focused active-barrier ConnectionLost test was found. Active Save/Restore member-exit tests cover embedded voluntary Resign, while the forced-loss and process member-exit combinations remain untested. The late-Join registry test covers explicit non-empty versus omitted synchronization sets; explicit-empty and partial-achievement interleavings remain uncovered. No normative interpretation or 2010/process parity is inferred.

All three guides already cover these concepts and limits with focused flow diagrams and evidence maps; no semantic gap or duplicate diagram was warranted. Checked 630 local links and 591 fragments across the guides; all resolve. No guide content, diagram, source, test, build, CI, Requirements Lab, or 2010 behavior changed in this audit.

## 2025 Attribute Ownership, Persistence/Resignation, and RAVU semantic audit — 2026-11-03

Cross-read the [Attribute Ownership](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md), [Ownership Persistence and Resignation](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md), and [Attribute Value Update Request](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md) guides. Ownership remains per object-instance attribute and distinct from publication, receiver interest, value delivery, and the privilege to delete an object. Acquisition admission, per-attribute transfer, callback-time observation, and query results are separately explained. RAVU solicits provider work; a later Update rechecks current ownership and source-region state, while receiver selection follows independent subscription, DDM, and time gates. The guides do not treat a request as ownership or direct reflection entitlement.

The persistence companion separates typed ownership/request records, live callback routes, already-admitted `HLA_EVOKED` callback queues, and federation membership. Its resignation preflight and member-scoped cancellation remain source/test-bounded; it does not claim the complete `ResignAction` matrix, process/embedded parity, or RTI-host restart recovery. The RAVU guide leaves direct requester entitlement under §6.21 open because its focused cases use subscribed requesters and the local API header lacks clause prose. The review found no missing reader transition or need for a combined diagram. It removed stale in-guide handoff text and an unrelated byte-order note so the roadmap remains the single current handoff source.

A static link check across the three guides found 552 local destinations and 495 fragments; all files, line targets, and heading fragments resolved. No behavior claim or diagram changed, and no source, tests, builds, CI, Requirements Lab, or 2010 content was touched.

## 2025 Federation MOM, Joined-Federate MOM, and Reporting semantic audit — 2026-11-03

Cross-read the [Federation MOM](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md), [Joined-Federate MOM](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md), [Service Invocation Reporting](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md), and [Exception Reporting](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md) guides, and spot-checked their cited 2025 source paths. They keep the execution-scoped `HLAfederation` object distinct from each member's `HLAfederate` object; distinguish object attribute reflections from explicit MOM request/report interactions; and separate the caller-visible service result from `HLAreportServiceInvocation`, `HLAreportException`, and `HLAreportMOMexception` observations. Subject switches, observer subscriptions/DDM eligibility, report-file versus interaction sinks, callback delivery, and member/requester/observer identity each have their own boundaries.

The Restore sections distinguish live MOM projections from saved ownership/member ledgers and bound the tests to the observed embedded or process paths. Federation-list reflection on Restore, several process MOM projections, post-acceptance requester departure, multi-module FOM index alignment, and report failure schedules remain source-only or untested where stated. The Exception Reporting guide preserves the pinned MIM's `HLAservice` wording ambiguity instead of using implementation code to silently resolve it. No cross-guide contradiction or missing reader transition was found, so a fifth synthesis diagram would repeat the existing decision map and three-trace comparisons.

A static link check across the four guides found 663 local destinations and 638 fragments; all files, line targets, and heading fragments resolved. Existing chart-specific syntax/render records remain in the roadmap; this audit changed no guide content or diagram. No source, tests, builds, CI, Requirements Lab, or 2010 content was touched.

## 2025 default-region and regional interaction routing review — 2026-11-03

Rechecked the 2025 DDM guide's ordinary `sendInteraction` and explicit
`sendInteractionWithRegions` paths against the embedded registry and the
existing focused routing cases. The guide already distinguished global active
relevance and class-candidate selection from later timestamp and callback gates.
Its compact route chart did not expose one useful geometry check: an explicit
source region can select an ordinary subscription only when at least one
explicit source region overlaps the RTI's default-region realization. Explicit
regional subscriptions are compared separately against source regions. The
revised chart preserves the empty `RegionHandleSet`, the committed
zero-dimensional Region, and the empty conveyed callback designator as distinct
cases. Corrected a stale source line link to the current per-recipient selector
and default-overlap helper. It continues to mark §9.1.5's interpretation as
unverified.

Mermaid CLI 12.0.0 rendered the revised chart; screenshot review found the
branches legible without clipping. The cited embedded default-region test's
recorded result is 1/1 with 50 assertions; it was not rerun. The configured
process multi-recipient case remains separate evidence and does not establish
the mixed ordinary-subscription result or profile parity. Inventory remains 31
guides / 387 diagrams; selected visual QA is 117 reviewed / 0 pending. No source,
tests, builds, CI, Requirements Lab, or 2010 content changed.

## 2025 multi-dimensional DDM overlap example review — 2026-11-03

Rechecked the embedded `regionSnapshotsOverlap` predicate. It compares only
dimensions present in both region specifications, requires at least one shared
dimension, and rejects the pair when any shared dimension has neither strict
overlap nor an allowed exact touch. The federation-wide Allow Relaxed DDM
switch accepts a touch but never a positive gap. Added a two-dimensional
worked example to the [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#overlap-strict-rule-and-umbras-optional-expansion):
strict X plus touching Y fails with relaxation disabled and succeeds when
enabled; replacing the touch with a gap fails in both modes.

This is an explanatory source trace, not test evidence. The focused embedded
boundary case and configured-process multi-recipient case each use one shared
dimension; the multi-dimensional combination remains untested. No Mermaid
chart changed, so the selected visual-QA queue remains 117 reviewed / 0
pending. No source, tests, builds, CI, Requirements Lab, or 2010 content
changed.

## 2025 directed-interaction and DDM boundary cross-audit — 2026-11-03

Cross-read the [Directed Interaction guide](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md)
and the [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
around target and region semantics. DDM explicitly excludes
`sendDirectedInteraction` from the ordinary/regional route flow and links to
the target/region distinction. The directed guide separates the target
`ObjectInstanceHandle` from source `RegionHandleSet` values and the universal or
by-ownership receiver selector from region-overlap matching; it links back to
DDM and leaves any broader normative relationship unclaimed. Its existing
flows already explain producer publication against the target class and
receiver selection from the class at which each receiver knows the target.
No missing reader transition or duplicate diagram was found. No source, test,
render, or diagram change was needed; no claim about a shared HLA region rule
was added.

Latest 2025 ownership and sender-settings cross-audit (2026-11-03): Corrected
the Order and Transportation guide's claim that no focused test observes a
new owner's order after transfer. Its embedded If-Available case observes
`RECEIVE` on a later update after the acquirer changes its class default.
Compared acquisition, Confirm Divestiture, unconditional divestiture, and
Divestiture If Wanted source paths: the last reseeds instance order but leaves
the instance transportation map untouched, and the effective resolver prefers
that stored value over the new owner's class default. No focused test queries
transportation after that transfer, so the observable result remains
unverified. Clarified that the transportation query/report pair is distinct
from order-name-to-enum lookup. Updated the existing ownership flowchart; no
diagram was added. Mermaid CLI 12.0.0 rendered it and local screenshot review
found it readable and unclipped. Checked 16 local destinations (13 source/test
line anchors, two heading anchors, and one file destination); all resolve.
Inventory remains 31 guides / 388 diagrams; selected visual QA remains 118
reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010
content changed.

2025 attribute-transportation request and query checkpoint review
(2026-11-03): Unconditional divestiture prunes released attributes from a
pending request and can preserve the still-owned subset; Confirm Divestiture
and Divestiture If Wanted leave the request intact, so a later full-set owner
check discards it if any member transferred. No focused test covers that
pending-transfer race. Attribute-query reports also sample state at different
checkpoints: embedded callback-task execution, configured-process pushed-event
creation, and configured-process receive polling. The Order and Transportation
guide now distinguishes those paths and labels them source-derived rather than
normative. The existing pending-state diagram and query sequence rendered with
Mermaid CLI 12.0.0 and were screenshot-reviewed as readable and unclipped.
Checked 126 local destinations (109 source line anchors, two guide headings,
and 15 file destinations); all resolve. Inventory remains 31 guides / 388
diagrams; selected visual QA is 120 reviewed / 0 pending. No source, tests,
builds, CI, Requirements Lab, or 2010 content changed.

2025 configured-process attribute-transportation request audit
(2026-11-03): The push branch begins and commits the registry request before
sending its confirmation event; the poll branch queues a request ID and leaves
the request pending until a later receive poll invokes the same owner
revalidation. A stale or erased request is skipped by the poll path. The
focused configured-process test pairs polling with `HLA_EVOKED` and pushing
with `HLA_IMMEDIATE`, and checks only the normal change/confirmation/query
sequence. Added a flowchart for the two checkpoints and their ownership-transfer
window. The updated pending-state and query diagrams plus the new process
flowchart rendered with Mermaid CLI 12.0.0 and were screenshot-reviewed as
readable and unclipped. Checked 136 local destinations (119 source line
anchors, two guide headings, and 15 file destinations); all resolve. Inventory
is 31 guides / 389 diagrams (222 flowcharts, 143 sequences, 24 state diagrams);
selected visual QA is 121 reviewed / 0 pending. The behavior is source-derived,
not an HLA ordering claim. No source, tests, builds, CI, Requirements Lab, or
2010 content changed.

2025 interaction transportation publication-loss/regain and query-timing audit
(2026-11-03): Request admission accepts ordinary or directed publication, but
confirmation consumes the pending entry and rechecks current publication. If
unpublishing erases the publisher's last declaration record, both the pending
request and any stored override disappear; republishing does not restore the
request. If the record survives, a later confirmation can commit only when
ordinary or directed publication is active at that checkpoint. No-delivery
outcomes differ by path: embedded callback work returns silently, configured-
process push reports an internal service error, and receive polling skips the
queued event. Added the interaction-publication flowchart and documented
interaction-query sampling: embedded callback-time replan, process push
handler-time snapshot, and receive-poll replan. A query uses the queried
federate's committed override only while current publication is active; it
does not read a pending change. Existing cases cover normal changes and query
round trips, not publication-loss/regain or query-sampling races. Mermaid CLI
12.0.0 rendered the new chart and screenshot review found it readable and
unclipped. Checked 151 local destinations (83 source anchors, 51 test anchors,
two guide headings, 15 files), all valid. Inventory is 31 guides / 390
diagrams (223 flowcharts, 143 sequences, 24 state diagrams); selected visual
QA is 122 reviewed / 0 pending. These are source-derived observations, not
HLA publication-order or query-order rules. No source, tests, builds, CI,
Requirements Lab, or 2010 content changed.

2025 interaction transportation and callback-dispatch cross-audit
(2026-11-03): The callback guide and Order and Transportation guide agree that
callback model controls application entry, while the service path controls the
interaction-type commit checkpoint. Embedded confirmation work revalidates
and commits inside its queued task before calling the FederateAmbassador;
configured-process push commits in the handler and poll commits when the
receive poll consumes the event. The callback model governs subsequent client
dispatch. The interaction process test constructs default
`ProcessFederationServiceOptions{}` (`pushReceiveOrderEvents=false`) and covers
polling under both `HLA_EVOKED` and `HLA_IMMEDIATE`, not both event modes under
both callback models. Added reciprocal guide links and clarified that test
scope. Checked 158 local destinations in the Order and Transportation guide
(84 source anchors, 54 test anchors, five guide headings, 15 file destinations),
all valid; the reciprocal callback-guide link also resolves. No diagram
changed, so the inventory remains 31 guides / 390 diagrams (223 flowcharts,
143 sequences, 24 state diagrams) and selected visual QA remains 122 reviewed /
0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content
changed.

## 2025 object subscription, knowledge, and reflection reader-path cross-audit — 2026-11-03

Cross-read the Object and Interaction Information Flow, Object Attribute
Subscription State, and Relevance Advisory guides. Their existing diagrams
already distinguish declared interest, active/passive relevance, the receiver's
own eligibility, per-federate object knowledge, later value projection, and
publisher/owner/receiver advisory callbacks. The behavior model was present,
but the broad guide lacked a short route through those companion explanations.
Added a four-step reading path after its five-question overview: declared
interest, instance knowledge, later value delivery, and advisory side lanes. It
links to the existing focused diagrams and makes clear that an advisory is not
a reflected value or evidence that a later update will be delivered. No
duplicate diagram or new implementation claim was needed.

Checked the eight local destinations added by the reading path; all files and
heading anchors resolve. No Mermaid diagram changed, so inventory remains 31
guides / 390 diagrams (223 flowcharts, 143 sequences, 24 state diagrams), and
selected visual QA remains 122 reviewed / 0 pending. No source, tests, builds,
CI, Requirements Lab, or 2010 content changed.

## 2025 Auto Provide, explicit update requests, and MOM reflection cross-audit — 2026-11-03

Cross-read the Auto Provide, Attribute Value Update Request, and Federation
MOM guides. They already separate discovery-triggered provider solicitation,
federate-issued value requests, later provider updates, receiver reflection,
initial attribute reflection, and MOM Conditional triggers. One comparison
table described only the joined-federate MOM direct-request exception, while
the Federation MOM guide also documents an explicit current-value request for
the federation-scoped `HLAcurrentFDD`. Expanded that row to name both
`HLAmanager.HLAfederation` and `HLAmanager.HLAfederate` MOM objects.

The configured-process request handler calls a shared RTI-owned MOM planner;
the source planner contains the federation-execution object path. The focused
process request test covers joined-federate `HLAreportServiceFile`, while the
focused `HLAcurrentFDD` request case is embedded. No focused process
`HLAcurrentFDD` request case was found. The comparison now records that
profile-specific evidence boundary without turning source inspection into a
process test or a parity claim. Checked the five links in the revised table
row; all source, test-guide, and heading targets resolve. No diagram changed,
so inventory remains 31 guides / 390 diagrams (223 flowcharts, 143 sequences,
24 state diagrams), and selected visual QA remains 122 reviewed / 0 pending.
No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

## 2025 federation-scoped and joined-federate MOM lifecycle cross-audit — 2026-11-03

Cross-read the Federation MOM and Joined-Federate MOM object-lifecycle guides
for object cardinality, creation, observer knowledge, initial values,
Conditional updates, current-value requests, and Resign. The guides already
lead with the key distinction: one `HLAfederation` object belongs to the
execution, while a separate `HLAfederate` object belongs to each joined
member. Their diagrams and comparison tables keep membership separate from
observer discovery, federation-object lifetime separate from member-object
retirement, and explicit current-value requests separate from Conditional
update triggers. The process evidence sections also agree: member-object
lifecycle has focused process cases, while process federation-object
discovery/update observations remain a stated source/test gap. Existing
cross-links connect the two lifecycle views and the federation/member FOM
request/report scopes. No semantic correction or duplicate diagram was
warranted. No source, tests, builds, CI, Requirements Lab, or 2010 content
changed.

## 2025 FOM catalog and Federation MOM cross-audit — 2026-11-03

Compared the effective model, federation module-designator list,
`HLAcurrentFDD`, and Conditional reflection across successful additional-module
Join, pre-commit rejection, post-commit setup failure, and Restore. The
existing Join diagram already separates the atomic catalog/member commit from
later setup rollback. Source and focused evidence show that a pre-commit
rejection keeps the old catalog, while a post-commit failure has no inspected
prior-definition restore step and no focused module-adding failure test.

Restore has two implementation sources: the retained in-process snapshot is a
full `Federation` copy with the save-time definition; the route-free durable
state image has no FOM catalog field and its fresh-registry restore starts from
the live definition. Both Federation MOM values project the selected live
definition. The inspected Restore completion paths do not enqueue these two
attributes, and no focused test observes their values or reflections across
Restore. The pinned MIM names Create/Join as their Conditional triggers; the
Restore contract remains open pending canonical 1516.1-2025 text. Added a
comparison table to the Federation MOM guide and a reciprocal link from the
FOM composition guide. No new diagram was needed; inventory remains 31 guides /
390 diagrams and selected visual QA remains 122 reviewed / 0 pending. No source,
tests, builds, CI, Requirements Lab, or 2010 content changed.

## 2025 standard-text access and late-subscription backlog correction — 2026-11-03

The official IEEE 1516.1-2025 listing identifies the active standard and offers
purchase or subscription access, but does not expose the clause prose. The
pinned repository package contains the public API headers and manifest, not a
full standard copy. The Save/Restore effect on the Current FOM and Federation
MOM conditionals therefore remains a source-only finding, not a normative
conclusion.

A bounded cross-read of Subscription State, Auto Provide, RAVU, and the process
discovery source found stale backlog statements that ordinary configured-
process initial values were MOM-only or had no focused test. The process
planner creates ordinary initial-value events for newly accepted discoveries,
and the public process case asserts discovery plus one selected stored-value
reflection under both callback-model sections. This does not establish a
universal callback order or the value effect of adding an attribute to an
already-known object; that separate case remains unresolved. Corrected the
conflicting backlog rows and the diagram/visual-review inventory to 390 charts
and 122 curated screenshots reviewed. Existing diagrams already explain the
receiver-known boundary, so no duplicate chart was added. No source, tests,
builds, CI, Requirements Lab, or 2010 content changed.

## 2025 RAVU, DSE, and timing-boundary cross-audit — 2026-11-04

Compared the 2025 Attribute Value Update Request, Delay Subscription Evaluation,
and Time Management guides. Their existing diagrams already connect the
reader's path: RAVU solicits provider work; a provider's later Update is a
separate service; ordinary receiver selection still applies; DSE can retain a
route-only candidate and re-evaluate eligibility at the relevant delivery
boundary. The reciprocal RAVU/DSE links and the Time Management guide's
callback/grant timing sections are sufficient; a combined chart would repeat
those explanations.

The profile evidence remains intentionally bounded. The focused embedded
receive-order DSE attribute test covers late subscription and unsubscribe under
both callback models; timestamped DSE tests use the constrained receiver's
grant boundary. The focused configured-process attribute DSE case covers
regional receive-order delivery. Other process routes have distinct source
checkpoints, including event-receive rechecks, pre-event projection, and TSO
event materialization, without equivalent focused coverage for every route.
No focused case combines RAVU with DSE, so neither the existing DSE test nor
the RAVU tests settle direct response entitlement. The §6.21 question remains
open pending canonical clause text. No guide diagram or behavior text needed
revision; no source, tests, builds, CI, Requirements Lab, or 2010 content
changed.

## 2025 configured-process ConnectionLost queue cleanup review — 2026-11-04

Rechecked the callback guide's process loss chart against the client failure
handoff, local lifecycle transition, callback-session close, and dispatcher
reset. The failed client remains retained after ConnectionLost; public
`Disconnect()` is rejected in the resulting `NotConnected` state. A later
`Connect()` replaces the old client, and ambassador destruction closes it.
Old-client close clears remaining client FIFO events, closes the old callback
session and waits for an active invocation, then resets tasks still pending in
the shared dispatcher. This can remove a not-yet-selected ConnectionLost task
as well as other pending work; callback order between buffered events and the
local loss task remains unestablished. The embedded loss test demonstrates
reconnect after ConnectionLost, but no focused public configured-process test
covers queue cleanup or that notification's survival. Clarified these lifecycle
edges in the existing flowchart; no new diagram was needed. No source, tests,
builds, CI, Requirements Lab, or 2010 content changed.

## 2025 HLA_EVOKED TSO/grant boundary cross-review — 2026-11-04

Cross-read the Time Management and Process Endpoint guides' timestamped
message/grant paths. The existing timing flow separates request acceptance,
TSO eligibility, callback-model gating, callback entry, and grant completion.
The embedded HLA_EVOKED case checks that no eligible callback is observed
before Evoke, then observes the timestamped interaction before the matching
grant. Configured-process HLA_EVOKED cases independently assert FIFO
interaction callbacks before a grant and an attribute reflection before its
grant; the attribute case also observes GALT/LITS while the message is in
transit. The Process Endpoint guide separately explains that the TSO ACK
follows callback return and may wait for an active request response before
transport. These cases remain payload- and profile-specific, and do not imply
a universal callback order or embedded/process parity. Existing diagrams and
evidence map already expose these transitions, so no guide change or duplicate
chart was warranted. No tests, builds, CI, Requirements Lab, or 2010 content
changed.

**Handoff status:** The configured-process Disconnect/reconnect review named
here is complete and recorded in the Callback and Service Ordering and
Connection and Configuration guides. The concurrent-call meaning remains
unresolved: the pinned public API has no general thread-safety statement, and
the full standard text is not included in the checkout. The callback-disable
review is also complete: its guide separates stream admission, client FIFO
work, dispatcher gating, and application callback entry with distinct Push/Pull
and HLA_IMMEDIATE/HLA_EVOKED evidence. The Join-during-Save/Restore cross-guide
review is complete; the exact normative admission rule remains unresolved
without the canonical clause prose. The active 2025 TSO/grant observation
cross-review is complete with no diagram change. The active 2025 NMR/NMRA
reverse-arrival review is maintained in `documentation_flow_guide_handoff` in
[ROADMAP-INDEX.json](../planning/ROADMAP-INDEX.json).

## 2025 authorization-disabled Connect evidence review — 2026-11-04

A bounded coverage survey compared the 31 2025 topic guides by line count,
diagram count, and unique source/test links. The smaller Authorization and
Exception Reporting guides already explain their main transitions and known
limits, so no new diagram was justified by their lower test-link counts alone.
One specific evidence pointer was missing: Authorization described Connect
with no configured authorizer, but did not link the focused public test for
that branch. Added the test source link and stated its exact scope: both tested
credentials-bearing sections reject a supplied password, then successfully
retry Connect with credentials omitted. The test does not separately pass an
explicit empty `HLAnoCredentials` object. This is 2025 embedded test evidence;
no process-parity or new normative claim is made. No Mermaid diagram was
changed, and the 2025 diagram inventory remains 391. No tests, builds, CI,
Requirements Lab, or 2010 content changed.

## 2025 timestamped MOM exception-report evidence review — 2026-11-04

A targeted source/test cross-read found that the Exception Reporting guide did
not describe a timestamped `Send Interaction` rejection. The embedded focused
test enables integer64 time regulation, sends Subscribe-only `HLAsetSwitches`
at valid time 1, observes `InteractionClassNotPublished`, then receives one
`HLAreportMOMexception` after evoking callbacks; its `HLAparameterError` is
false. The timestamped implementation captures the MOM service identity before
validation, rejects the request during interaction planning before TSO enqueue,
and attempts the MOM report in its exception handler. The guide now records
that the failed application's timestamp does not make the separate report TSO,
and bounds this observation to the embedded test; it does not establish
process-profile parity, a normative timing rule, or delivery of both exception
report interactions. No new diagram was needed, and the 2025 Mermaid inventory
remains 391. The test was inspected but not rerun. No code, tests, builds, CI,
Requirements Lab, or 2010 content changed.

## 2025 Save/Restore completion-boundary evidence review — 2026-11-04

Cross-checked participant completion against federation outcomes in the existing
Save/Restore diagrams, source, and focused tests. The guide correctly separates
a member's accepted `federateSaveComplete` / `federateRestoreComplete` call from
the all-member barrier and the later `federationSaved` / `federationRestored`
callback. Embedded multi-member evidence shows the first Save completion does
not persist the image or notify either member; the final completion commits the
image and exposes the outcome. The corresponding Restore case exposes
`federationRestored` after both members complete. Configured-process evidence
separates polling with `HLA_EVOKED` from pushed/immediate callback delivery and
bounds each claim to its test. No missing behavior transition or additional
diagram was warranted. Corrected four stale links in the Save/Restore guide
that pointed to a helper before the configured-process Save test; they now
point at the test declaration. The 2025 Mermaid inventory remains 391. Tests
were inspected, not rerun. No code, tests, builds, CI, Requirements Lab, or
2010 content changed.

## 2025 ConnectionLost during active Save/Restore cross-guide audit — 2026-11-04

Cross-checked the Lifecycle, Synchronization, and Save/Restore explanations
against the shared registry transition and both adapter paths. The existing
Lifecycle flow already distinguishes voluntary Resign from forced
`ConnectionLost`: Save uses different source-selected failure reasons, while
both currently plan `FEDERATE_RESIGNED_DURING_RESTORE`; planners exclude the
leaver and members without callback routes. The embedded adapter submits both
notification families. The configured-process dispatcher enqueues Save
notifications but has no Restore-notification projection. The Synchronization
guide correctly points to this shared-removal limit without claiming process
parity. A bounded intersection of 2025 Catch2 files mentioning connection loss
and Save/Restore services found no public test covering both in one scenario;
focused member-exit tests cover voluntary Resign only. No duplicate diagram or
behavior prose was needed. Corrected eight stale `connectionLost` source links
to the method definition and three completion-planner links to their actual
function declarations. No test was run; no code, tests, builds, CI, Requirements
Lab, or 2010 content changed.

## 2025 forced ConnectionLost joined-federate MOM removal cross-audit — 2026-11-04

Cross-read the Lifecycle, Joined-Federate MOM, Federation MOM, and Callback /
Service Ordering guides against the shared registry transition, callback-time
removal, and configured-process dispatcher. Existing guide coverage is
sufficient: the departed `HLAfederate` object is independent of `ResignAction`;
only surviving known observers with usable routes receive planned removals.
Voluntary Resign preflights all routes before mutation, while forced loss may
skip an unavailable route. For planned removals, callback-entry clears that observer from the pending
and known sets; the callback path erases the snapshot once both sets are
empty.
Source inspection found no retry for a skipped known observer. When forced
cleanup plans no removals, it can erase the snapshot immediately even if a
skipped known ID remains; when another removal is planned, that skipped ID can
remain after the last planned callback. This is a source-only ledger edge, not
an HLA rule or tested result.

The guides also preserve the adapter boundary. The embedded adapter submits
survivor removal work separately from the departing ambassador's local
`connectionLost` callback; the configured-process path enqueues remote removal
work through its own session dispatch. Source submission/enqueue order does not
establish a general application callback-entry order. The focused public MOM
removal tests exercise explicit Resign; a bounded scan of the 2025
ConnectionLost test sources found no assertion for removal of the departed
`HLAfederate` object. Corrected stale source anchors across the four guides. No
diagram or behavior text needed revision. Tests were inspected, not rerun. No
source, tests, builds, CI, Requirements Lab, or 2010 content changed.

## 2025 Federate-Lost MOM report and ConnectionLost timing cross-audit — 2026-11-04

Cross-checked the Lifecycle, Federation MOM, and Callback / Service Ordering
guides against the 2025 MIM, report planner, both dispatch paths, and focused
embedded cases. The MIM defines the fault-loss trigger, reliable Receive order,
`HLAfederate` dimension, and last-known-good timestamp. The guides now separate
that survivor-side `receiveInteraction` from the lost endpoint's local
`connectionLost` callback and a survivor's `HLAfederate` object removal. The
planner's active-time and route checks are labeled as implementation details.

The embedded adapter submits report work before automatic-resignation effects;
the focused HLA_EVOKED case confirms report-before-`DELETE_OBJECTS` removal for
two survivors. The configured-process dispatcher enqueues removal and
synchronization events before the report, but the bounded test scan found no
configured-process `HLAreportFederateLost` case, so process callback order and
profile parity remain unestablished. Corrected the Lifecycle summary flow so it
no longer implies one universal report/removal order, and linked the three
observation streams in Callback and Service Ordering. Added separate report-
planning and profile-dispatch flowcharts to Federation MOM; both were rendered
with Mermaid CLI 12.0.0 and screenshot-reviewed locally. The revised Lifecycle
chart was also rendered and reviewed. Inventory is 31 guides / 393 diagrams;
selected visual QA is 127 reviewed / 0 pending. Tests were inspected, not rerun.
No source, tests, builds, CI, Requirements Lab, or 2010 content changed.
## 2025 configured-process Federate-Lost receiver-path audit — 2026-11-04

Traced `HLAreportFederateLost` from service dispatch to the survivor's
`receiveInteraction` callback and compared it with the failed member's local
`connectionLost` callback. Push events enter the process client's pending-event
FIFO; a Pull interaction is returned separately and dispatched directly by the
callback-control service. Both paths converge on the receive-order callback
bridge, while the failed ambassador independently clears its joined/time state
and queues its local callback. No process test covers the RTI-owned report; a
process service-host case observes local loss callbacks only. Application
callback order between these paths remains unestablished.

Corrected an earlier Process Endpoint statement that had grouped poll-returned
interactions into `pendingEvents_`, and clarified the Callback / Service
Ordering guide. Added the Federation MOM receiver/local-callback flowchart and
rendered and screenshot-reviewed it with Mermaid CLI 12.0.0. Inventory is 31
guides / 394 diagrams; selected visual QA is 128 reviewed / 0 pending. Tests
were inspected, not rerun. No source, tests, builds, CI, Requirements Lab, or
2010 content changed.

## 2025 Federate-Lost callback-enable gate review — 2026-11-04

Compared the survivor report callback with the failed member's local
`connectionLost` callback under disabled callback state. The gate is local to
each ambassador's dispatcher: it does not stop report planning or the local
Not Connected transition. Embedded forced loss resets the departing
ambassador's dispatcher and re-enables it before submitting `connectionLost`;
the configured-process handler retains the existing disabled state. For
survivors, embedded report callbacks recheck the route at callback entry, while
process report events use the recipient projection captured by the service and
show no report-specific callback-time recheck. A disabled interval can therefore
expose a source-level routing difference if the subscription changes before
application entry.

The process receive/Evoke test covers a disabled ordinary interaction and
delivery after re-enable; dispatcher unit tests cover the general gate. The
process service-host case covers local loss without callback disablement. No
focused test covers a Federate-Lost report or either loss callback while
disabled. Added the source-bounded comparison to Callback / Service Ordering
and linked it from Federation MOM. The existing callback-gate and loss-stream
diagrams already show the transitions, so no duplicate chart was added.
Inventory remains 31 guides / 394 diagrams and selected visual QA remains 128
reviewed / 0 pending. Tests were inspected, not rerun; no source, tests, builds,
CI, Requirements Lab, or 2010 content changed.

## 2025 object-instance name reservation reader-gap review — 2026-11-04

Rechecked the existing reservation guide against the 2025 ambassador,
registry, configured-process service, and focused Catch2 paths. The guide's
single-name admission and callback outcome, per-name bulk-reservation subsets,
all-or-nothing bulk release, reservation ownership, and separation from named
registration and peer discovery match the reviewed source and tests. The
embedded cases cover synchronous single/bulk validation errors, HLA_EVOKED
success and collision callbacks, partial bulk reservation, atomic release,
generated-name avoidance, and reservation release on Resign. The configured
process case covers reservation and release, all-success and mixed bulk
subsets, and both callback models; it does not assert cross-callback entry
order.

One evidence limit is now explicit in the guide: the registry validates all
bulk names before attempting reservations, but existing tests do not retry a
valid name from a rejected mixed-validity set to assert absence of partial
side effects. The process case does not submit invalid bulk input. This remains
source evidence, not a tested endpoint guarantee. The current diagrams already
answer the review question, so no diagram was added or revised. No source,
tests, builds, CI, Requirements Lab, or 2010 content changed.

## 2025 name reservation and receiver-known callback-entry cross-review — 2026-11-06

Cross-checked the Name Reservation and Object/Interaction guides against the
pinned 2025 API, embedded registration and discovery paths, configured-process
request/event paths, and focused cases. The existing guides already distinguish
reservation ownership, registration commit, federation-wide live-instance/name
indexes, and receiver-local known-object state. Registration validates occupancy
and reservation ownership, commits both instance indexes, then consumes the
reservation; failed pre-commit checks preserve it.

One wording gap remained in the embedded owner/peer flow: it could read as if
callback entry caused receiver-known state. The discovery task actually rechecks
eligibility and commits the receiver-known class before calling
`discoverObjectInstance`; the focused lookup case checks failure while queued
and success after callback delivery, not from inside the callback. Clarified
that source/test boundary in the existing flow and linked the Object/Interaction
callback-entry trace. The configured-process guide already keeps registry
transition, event exposure, and later client callback separate; focused process
reservation and discovery tests remain separate and do not establish one
end-to-end timeline.

No new chart was needed. Mermaid CLI 12.0.0 rendered the revised chart, and
screenshot review found it legible. No tests, builds, source, Requirements Lab,
or 2010 content changed.

## 2025 timestamped Update Attribute Values cross-guide review — 2026-11-06

Compared Time Management, TSO Retraction, Object/Interaction, and Attribute
Ownership with the relevant embedded/process source paths and existing focused
test evidence. The time guide separates one invocation into a constrained
receiver pending TSO copy and a non-constrained receiver receive-order
reflection, then distinguishes embedded callback-time projection from
configured-process event-materialization projection. The attribute-specific
embedded case observes retraction of the queued copy while the delivered
reflection carries the shared designator and is followed by Request Retraction.

Ownership transfer after acceptance is a separate timeline: selected embedded
tests keep the original producer on the accepted reflection while current
ownership changes. They do not establish process parity or place ownership
callbacks relative to a TSO grant. Focused process cases cover event/callback
order relative to grant but do not mutate receiver subscription or region
between acceptance and event materialization.

The generic object-attribute reading route lacked a direct timestamped branch.
Added a brief branch to the existing Time Management and TSO ownership
timelines. No diagram was added. No tests, builds, code, Requirements Lab, or
2010 content changed.

## 2025 ordinary receive-order Update Attribute Values reader-route review — 2026-11-06

Compared the existing embedded no-time sender snapshot, per-passel receiver
projection, and callback recheck in Object/Interaction with Subscription State
active gates, DDM per-attribute relevance and region checks, Update Rate
per-value best-effort admission, and Order/Transportation sent/received-order
distinctions. The Object/Interaction guide already contains three
responsibility-aligned diagrams for admission/snapshot, per-receiver projection,
and callback-boundary recheck. Its generic end-to-end reading route did not
point readers to those views and their companion questions.

Added a short ordinary receive-order branch to the same reader route as the
timestamped branch, with links to the existing DDM, update-rate, and
order/transport maps. Configured-process delivery remains a separate path; the
focused public reflection case covers both callback models but does not
establish implementation parity. No new diagram was needed. No tests, builds,
code, Requirements Lab, or 2010 content changed.

## 2025 reader-route review history — 2026-11-06

The twenty detailed reader-route reviews from this date are preserved in the
[dated review archive](HLA-BEHAVIOR-FLOW-GUIDES-ARCHIVE/REVIEW-2026-11-06.md).
This roadmap keeps the active backlog and handoff; the archive retains the
individual review conclusions and evidence.

## 2025 grouped timestamped attribute callback exception review — 2026-11-06

Reviewed the configured-process timestamped attribute group from service
projection through client event dispatch, callback submission, and TSO
acknowledgment. The process labels each recipient's ordered events as zero or
more more passels followed by one final passel. A throwing more callback
consumes that task but does not set the group delivery's acknowledged state or
cancel later events. In HLA_EVOKED, the callback dispatcher has already
removed the throwing task and leaves following tasks queued. In
HLA_IMMEDIATE, it preserves the remaining callback FIFO; events not yet
submitted to the bridge remain in the client's pending-event queue. A later
eligible dispatch can still reach final.

If final returns normally, or is suppressed at its final retraction boundary,
the bridge can complete the message once even after an earlier more
exception. If final throws, its completion hook is skipped and the recipient
remains inTransit. A Retract after more has started can queue the public
requestRetraction callback, but that callback does not itself acknowledge the
group. The ACK from a successful final boundary may be deferred while a process
request is active.

Added a flowchart and source-bounded explanation to the Process Endpoint guide,
and updated the overview route. The focused bridge test covers normal grouped
completion under both callback models and retraction before final submission;
the public fan-out case covers recipient-local delivery and ACK order. No
focused callback-throw case covers a more recovery or throwing final, so those
branches are marked source-reviewed rather than test-demonstrated or
normative. Mermaid CLI 12.0.0 parsed and rendered the new chart; screenshot
review found it readable and unclipped. Inventory is now 442 diagrams across 31 guides (253 flowcharts, 164 sequences, 25 state diagrams; 434 backtick-fenced and eight tilde-fenced); selected screenshot review is 183/0. Seven focused source/test links resolve
and git diff --check passes. No tests, builds, source, Requirements Lab, or
2010 content changed.

**Next handoff:** Review the existing 2025 Attribute Ownership guide's
negotiated transfer flow. Trace request, offer, acquisition notification,
divestiture confirmation or cancellation, ownership-state change, and callbacks
for both members; compare those boundaries with focused 2025 implementation
tests. Identify any reader gap or evidence limit, and update the existing
diagrams only where source and test evidence establish a useful transition.
Documentation-only, 2025-only; no tests, builds, CI, Requirements Lab, or 2010
work.

2025 negotiated ownership confirmation/cancellation review (2026-11-06):
Clarified that successful Confirm Divestiture updates ownership per attribute
before the process service enqueues recipient notifications; the focused
process test observes state before confirmation and after notification, not the
intermediate commit or concurrent response/callback order. Added an owner
cancellation sequence distinguishing cancellation before and after the owner
confirmation-callback gate. A configured-process HLA_EVOKED test demonstrates
the pre-gate reroute to one ordinary release callback while the owner retains
both attributes; the post-gate cancellation outcome remains covered by the
embedded test only. Mermaid CLI 12.0.0 render and screenshot review passed for
the negotiated-confirmation and new owner-cancellation sequences. The 2025
inventory is 443 diagrams across 31 guides (253 flowcharts, 165 sequences, 25
state diagrams; 435 backtick-fenced and eight tilde-fenced); selected screenshot
review is 188/0. Focused source/test links and git diff --check passed. No tests,
builds, source, Requirements Lab, or 2010 content changed.

**Next handoff:** Review a 2025 Confirm Divestiture set whose attributes have
distinct active acquiring federates or acquisition requests. Trace per-attribute
validation and ownership commit, per-member notification grouping, cleanup of
each acquisition request, and what happens if one candidate is no longer
active. Check focused tests for mixed-requester coverage and preserve the
confirmation failure boundary. Update the guide only for a demonstrated reader
gap. Documentation-only, 2025-only; no tests, builds, CI, Requirements Lab, or
2010 work.

2025 mixed-requester Confirm Divestiture review (2026-11-06): added a sequence
for one owner confirmation spanning different active requesters. The registry
groups notifications by receiving federate, but cleans up each regular or
If-Available acquisition request separately. Focused embedded Save/Restore
cases cover both different requesters and separate request types from one
requester; their callback observations occur after Restore. Process source
routes notification plans per recipient, but the selected multi-recipient
tests do not establish configured-process parity. Documented the source-only
mixed active/stale branch: `NoAcquisitionPending` leaves owners unchanged and
returns stale offers to owner search, while an active-plus-stale combined set
has no focused test. Mermaid CLI 12.0.0 render and screenshot review passed.
Inventory is 444 diagrams across 31 guides (253 flowcharts, 166 sequences, 25
state diagrams; 436 backtick-fenced and eight tilde-fenced); selected screenshot
review is 189/0. Focused source/test anchors and git diff --check passed. No
tests, builds, source, Requirements Lab, or 2010 content changed.

**Next handoff:** Cross-read the mixed active/stale Confirm Divestiture branch
against the exact 2025 standard text and API exception contract. Determine what
the standard guarantees for a supplied attribute set when one acquisition is
canceled or its requester departs while another remains active. Keep source-only
Umbra behavior distinct from normative requirements; update the guide only
where that comparison changes the reader's model. Documentation-only,
2025-only; no tests, builds, CI, Requirements Lab, or 2010 work.

2025 requester Resign directive 3 cross-review (2026-11-06): added a sequence
for a regular acquisition selected by negotiated divestiture, then canceled
when its acquirer resigns with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` before
the owner callback gate. Embedded and standalone 2025 cases observe neither
the release request nor the divestiture confirmation after the owner's callback
drain; the owner retains the attribute. Registry cleanup is scoped to the
departing member and clears the removed negotiated candidate's assumption-search
state. Other members' requests are retained by source, but the selected tests do
not include a second requester. Mermaid CLI 12.0.0 render, screenshot review,
focused links, and `git diff --check` passed. Inventory is 445 diagrams across
31 guides (253 flowcharts, 167 sequences, 25 state diagrams; 437 backtick-fenced
and eight tilde-fenced); selected screenshot review is 190/0. No tests, builds,
source, Requirements Lab, or 2010 content changed.

**Next handoff:** Review the 2025 combined Resign directive 5 path when the
departing federate is a prospective acquirer with a pending negotiated
divestiture. Trace acquisition cancellation, object deletion, ownership
divestiture, and callback cleanup; add a diagram only if the existing guide
misses a reader-relevant boundary. Keep the exact standard interpretation
separate from Umbra source behavior. Documentation-only, 2025-only; no tests,
builds, CI, Requirements Lab, or 2010 work.
2025 Resign directive 5 phase review (2026-11-06): Added a source-derived
lifecycle flowchart for the combined CANCEL_THEN_DELETE_THEN_DIVEST action. It
shows member-scoped acquisition and negotiated-candidate cancellation before
the prepared eligible-delete pass, then retained-object divestiture and
assumption-offer planning; deleted objects skip divestiture, and adapter
callback projection is profile-specific. The direct embedded D5 test covers an
ordinary pending acquisition, not a negotiated candidate's post-Resign callback
or ownership outcome. Component tests cover delete/divest paths separately;
Confirm Divestiture cases that use D5 during teardown assert before Resign.
Mermaid CLI 12.0.0 render and screenshot review passed. Focused links and
`git diff --check` passed. Inventory is 446 diagrams across 31 guides (254
flowcharts, 167 sequences, 25 state diagrams; 438 backtick-fenced and eight
tilde-fenced); selected screenshot review is 191/0. No tests, builds, source,
Requirements Lab, or 2010 content changed.

**Next handoff:** Review 2025 federate-local object-instance handle lookup
before and after discovery across embedded and configured-process paths. Trace
source dispatch and current-known-object checks; compare focused discovery and
lookup tests. Update the Name/Handle guide only if the reader-facing answer
depends on delivery profile, and keep process parity unverified unless selected
evidence establishes it. Documentation-only, 2025-only; no tests, builds, CI,
Requirements Lab, or 2010 work.
2025 federate-local object-instance lookup cross-review (2026-11-06): Compared the embedded adapter, configured-process service, shared registry lookup, and focused discovery/lookup tests. Both source paths query the same member-keyed known-object catalog; the process service also matches request identity to its session and current membership. The embedded test observes peer lookup fail before discovery is evoked and succeed after. The process endpoint test covers the registering member's round trip and unknown inputs under both callback models, not peer visibility before/after discovery. Added this source-versus-test boundary to the Name/Handle guide; the existing discovery flowchart was sufficient. Six focused local source/test links resolve; `git diff --check` passed. Inventory remains 446 diagrams across 31 guides (254 flowcharts, 167 sequences, 25 state diagrams; 438 backtick-fenced and eight tilde-fenced); selected screenshot review remains 191/0. No tests, builds, CI, source, Requirements Lab, or 2010 work.

**Next handoff:** Review 2025 object-instance lookup after two distinct
transitions: per-receiver `localDeleteObjectInstance` and accepted object
deletion, for a receiver that has already discovered the instance. Compare the
Name/Handle, Object/Interaction, and Lifecycle guides with the registry removal
paths and focused cases. Clarify forward/reverse lookup outcomes and keep
forgetting local knowledge separate from accepted instance removal and name
purge; add a cross-link or transition only if a reader gap remains. Distinguish
source-derived behavior from standard semantics. Documentation-only, 2025-only;
no tests, builds, CI, Requirements Lab, or 2010 work.
2025 federate-local lookup after local forget and receive-order delete review (2026-11-06): Compared the Name/Handle, Object/Interaction, and Lifecycle guides with the embedded registry and focused cases. Added a source-derived explanation that successful local forget makes both name/handle directions unknown for that caller while preserving the object, name entry, and other members' knowledge; accepted receive-order deletion makes the invoker unknown at acceptance and each receiver unknown at its own removal-callback gate, while object/name purge remains separate. The local-forget and receive-order tests assert getKnownObjectClassHandle at these boundaries, not both conversion methods; the configured-process local-delete case asserts success but performs no post-call lookup. Existing object-information charts already show the transitions, so no diagram was duplicated. Twelve local links resolve; git diff --check passed. Inventory remains 446 diagrams across 31 guides (254 flowcharts, 167 sequences, 25 state diagrams; 438 backtick-fenced and eight tilde-fenced); selected screenshot review remains 191/0. No tests, builds, CI, source, Requirements Lab, or 2010 work.

**Next handoff:** Review 2025 timestamped object deletion and retraction from
the object-instance lookup perspective. Trace producer and receiver
forward/reverse lookups across request submission, the logical-time deletion
boundary, removal callbacks, retraction before delivery, and eventual name
purge. Compare the Name/Handle, Time Management, and TSO Retraction guides with
registry paths and focused cases. Update an existing chart or add a focused
cross-link only if the lookup boundary remains implicit; keep receive-order and
timestamped paths separate, and distinguish source behavior from tested
outcomes. Documentation-only, 2025-only; no tests, builds, CI, Requirements
Lab, or 2010 work.
2025 timestamped object-deletion lookup cross-review (2026-11-06): Cross-read the Name/Handle, Time Management, and TSO Retraction guides with the embedded deletion/retraction test and registry. Added a Name/Handle section tying the pending timestamped Delete and pre-gate Retract to a receiver's successful forward lookup, then its loss of lookup at its removal gate; the producer's reverse lookup remains available until its own granted time boundary. The global name is a separate lifetime: terminal retraction cleanup can retain the deletion marker until the producer boundary, purge waits for remaining known/pending timestamped state, and name reservation checks the execution-wide name index. The focused test does not test reservation/reuse at purge or process lookup parity, so those remain source-derived/unverified. Added a reciprocal TSO-to-Name/Handle cross-link; existing charts already cover the gates, so no new diagram was warranted. Twenty focused local links resolve; git diff --check passed. Inventory remains 446 diagrams across 31 guides (254 flowcharts, 167 sequences, 25 state diagrams; 438 backtick-fenced and eight tilde-fenced); selected screenshot review remains 191/0. No tests, builds, CI, source, Requirements Lab, or 2010 work.

2025 object-instance lookup across Save/Restore rebind (2026-11-06): Cross-read
the existing identity-rebind chart with the saved object image, known-member
projection, restore installation, and the focused one- and two-object tests.
Clarified that the one-object embedded test checks producer forward/reverse
lookup after `federationRestored`, then receiver lookup after later discovery;
it does not query the receiver in the interval between those events. Updated
the existing chart and added a reciprocal Name/Handle guide note. The
configured-process restore/lookup path remains source-only for this combined
outcome. The fresh registry is within one OS process; no name-reuse or 2010
claim is made. No additional diagram was needed.

2025 configured-process lookup around Restore and discovery (2026-11-06):
Traced configured-process session/member validation, restored known-object
projection, Restore notification delivery, discovery-state commit, and both
lookup endpoints. Source order installs restored member-known state before
queueing/pushing `federationRestored`; discovery likewise commits receiver
known state before exposing its event. The HLA_EVOKED Restore test checks that
the callback remains pending until Evoke, but performs no lookup during that
interval. The process lookup test covers the registering member and unknown
inputs, not peer lookup around discovery or Restore. Added a process-only
source-order chart, clarified the Name/Handle guide, and corrected the Restore
success-test link. No normative timing claim is inferred from implementation
order. Mermaid CLI 12.0.0 render and screenshot review passed; 33 focused local
links resolve; `git diff --check` passed. Inventory is 447 diagrams across 31
guides (255 flowcharts, 167 sequences, 25 state diagrams; 439 backtick-fenced
and eight tilde-fenced); selected screenshot review is 192/0. No tests, builds,
CI, source, Requirements Lab, or 2010 work.

2025 normative object-lookup timing source review (2026-11-06): Checked the
pinned IEEE 1516.1-2025 C++ API declarations for §§10.7, 10.8, 6.9, and 4.32
against the configured-process source-order chart and focused tests. The
headers state the service/callback signatures and exceptions, but not when
either lookup must become usable relative to discovery or Restore callbacks.
IEEE's public product page lists purchase and subscription access; the pinned
tree is the API header import, not the full clause text. Added this boundary to
the Name/Handle guide; the observed timeline remains explicitly Umbra source
behavior. Thirty-eight focused local destinations resolve; `git diff --check`
passed. Diagram inventory remains 447 across 31 guides (255 flowcharts, 167
sequences, 25 state diagrams; 439 backtick-fenced and eight tilde-fenced); the
selected screenshot review remains 192/0. No diagram, tests, builds, CI,
source, Requirements Lab, or 2010 content changed.

2025 Flush Queue Request delivery-boundary review (2026-11-06): Rechecked the
Time Management and Callback/Service Ordering guides against their focused
embedded and configured-process cases. The existing maps distinguish queued
TSO eligibility, the embedded callback-time extraction frontier, the process
acceptance frontier, `HLA_EVOKED` callback entry, FQG actual/optimistic time,
and the later grant boundary. The pinned Requirements Lab transcription is
the clause-text source cited by the guide; it does not settle a universal
transport-arrival cutoff or require identical profile policy. No chart change
was needed. No tests, builds, CI, source, Requirements Lab, or 2010 work.

2025 RAVU entitlement reader-gap review (2026-11-06): Rechecked the 2025
Request Attribute Value Update guide, pinned API overloads, focused embedded
and process test setup, and existing traceability contracts. The selected
request/response cases give the requester subscription context; the separate
DSE test establishes ordinary delivery-time eligibility changes without
RAVU. The API exception lists omit a subscription exception, but that absence
does not establish a provider duty to answer or a requester's right to observe
the later update. Added that caution to the guide. No new diagram or test was
warranted; no Requirements Lab content changed.

**Next handoff:** When authorized IEEE 1516.1-2025 §6.21 clause text is
available, determine whether an explicit request creates a direct response
obligation for a requester who is otherwise ineligible to receive the later
Update. Keep request validity, provider solicitation, and receiver delivery
eligibility as separate questions; do not infer entitlement from the API
exception list. Update the RAVU guide only if the clause text changes its
reader-facing model. Documentation-only, 2025-only; no tests, builds, CI,
Requirements Lab edits, or 2010 work.



2025 joined-federate MOM report-guide split (2026-11-06): Moved the seven
joined-federate MOM report sections and their nine evidence-map rows into
[MOM Object and Interaction Reports](HLA-2025-MOM-OBJECT-AND-INTERACTION-REPORTS-FLOW-GUIDE.md).
These requester-local RTI report interactions form a coherent topic distinct
from application-object lifecycle and ordinary object/interaction delivery.
The seven Mermaid bodies were relocated unchanged; the inventory is 32 topic
guides and 447 diagrams. Updated the Object/Interaction, Joined-Federate MOM,
behavior-flow index, and API-family roadmap routes. No tests, builds, CI,
Requirements Lab, or 2010 content changed.

**Next handoff:** When authorized IEEE 1516.1-2025 §6.21 clause text is
available, determine whether an explicit request creates a direct response
obligation for a requester who is otherwise ineligible to receive the later
Update. Keep request validity, provider solicitation, and receiver delivery
eligibility as separate questions; do not infer entitlement from the API
exception list. Update the RAVU guide only if the clause text changes its
reader-facing model. Documentation-only, 2025-only; no tests, builds, CI,
Requirements Lab edits, or 2010 work.

2025 MOM exception parameter-boundary review (2026-11-06): Expanded the
Exception Reporting guide's malformed-MOM-request evidence with a current
test-source case: the test expects one `HLAreportMOMexception` for each
empty-argument rejection across its 54 parameter-required Table 7 S leaves,
with `HLAparameterError=true`; two parameterless request leaves act as
controls. The source catches API failures generically, was inspected but
not executed, and does not establish successful argument shapes, process
behavior, or broad conformance. Route B already shows the rejection/report
path, so no diagram was duplicated. Inventory remains 32 guides / 447
diagrams. No C++ source, Requirements Lab, or 2010 edits were made; no
tests, builds, or CI were run in this documentation increment.

**Next handoff:** When authorized IEEE 1516.1-2025 §6.21 clause text is
available, determine whether an explicit request creates a direct response
obligation for a requester who is otherwise ineligible to receive the later
Update. Keep request validity, provider solicitation, and receiver delivery
eligibility as separate questions; do not infer entitlement from the API
exception list. Update the RAVU guide only if the clause text changes its
reader-facing model. Documentation-only, 2025-only; no tests, builds, CI,
Requirements Lab edits, or 2010 work.

2025 embedded MOM subscription-command review (2026-11-06): Added a focused
flowchart and argument map to the Object/Interaction guide for the six 2025
MOM Subscribe/Unsubscribe interaction classes. The MIM describes them as
service-shaped requests; the current embedded source intercepts matching
Send Interaction calls and invokes the corresponding API on the sender. The
diagram contrasts this route with ordinary recipient fan-out. Its handler and
the broad empty-argument Table 7 S-leaf test are both untracked in the shared
worktree; that test checks MOM-exception reporting, not successful mutation.
Configured-process parity and success-path behavior remain unestablished.
Mermaid CLI 12.0.0 render and local screenshot review passed. Inventory is 32
guides / 448 diagrams (256 flowcharts, 167 sequences, 25 state diagrams; 440
backtick-fenced and eight tilde-fenced); selected screenshot review is 193/0.
No C++ source was edited in this documentation increment; no tests, builds,
CI, Requirements Lab, or 2010 content changed.

**Next handoff:** When authorized IEEE 1516.1-2025 §6.21 clause text is
available, determine whether an explicit request creates a direct response
obligation for a requester who is otherwise ineligible to receive the later
Update. Keep request validity, provider solicitation, and receiver delivery
eligibility as separate questions; do not infer entitlement from the API
exception list. Update the RAVU guide only if the clause text changes its
reader-facing model. Documentation-only, 2025-only; no tests, builds, CI,
Requirements Lab edits, or 2010 work.

2025 reader-review history archive split (2026-11-06): Moved the dated reader-gap summaries out of
the active guide index into
[HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md](HLA-BEHAVIOR-FLOW-GUIDES-READER-REVIEW-ARCHIVE.md).
The original history section is preserved verbatim after the archive preface (SHA-256
c3146d3cf90a1e27d1418f4ea5a7c239c5bd059364eec72a9ccb8b74c01e5ce7). All dated headings remain in the
active index as legacy anchors and link to their archived details. The active index is about 3.3k
lines, down from 6,528; the archive is 3,877 lines. The moved diagram body is unchanged, so the
inventory remains 32 guides / 448 diagrams. No source, tests, builds, CI, Requirements Lab, or 2010
content changed.

**Next handoff:** Split the dated Progress Log and Review Ledger into chronological, cohesive
archives. Keep the current files as indexes with their original headings and anchors, move detailed
entries into linked archive chunks sized for navigation, preserve cross-links, and update each path
growth guard. The current logs are about 17.2k and 17.6k lines against recorded guards of 12,650 and
10,500. This is documentation maintenance; do not change HLA conclusions or add 2010/Requirements
Lab work.

2025 dated progress/review archive split (2026-11-06): Moved detailed records into 9 progress and 10
review date archives under [the archive folder](HLA-BEHAVIOR-FLOW-GUIDES-ARCHIVE/). The progress
index is 2338 lines and the review index 2940; original heading text/order and legacy anchors
remain. Five progress-to-review deep links and four review self-fragment links resolve, with
cross-date references routed through the stable index. All 19 chunks are at or below 4710 lines and
have 5,000-line guards. Neither source ledger had Mermaid diagrams; inventory remains 32 guides /
448 diagrams. No source, tests, builds, CI, Requirements Lab, or 2010 work changed.
**Next handoff:** Cross-review one 2025 configured-process delivery sequence: AttributeUpdate
reception by a time-constrained member while asynchronous-delivery changes interact with a pending
time advance. Compare the Callback and Service Ordering and Time Management guides with
dispatcher/client source and existing focused tests. Clarify checked stages from endpoint receipt
through queue/dispatch to callback entry, and keep the combined scenario explicitly untested. Revise
or add a diagram only if that review finds a real reader gap. Documentation-only, 2025-only; no
tests, builds, CI, Requirements Lab, or 2010 work.

2025 callback-gate and provider-solicitation cross-review (2026-11-06): Existing guides already
separate configured-process AttributeUpdate dispatch from the receive-order interaction temporal
gate, discovery-triggered Auto Provide from explicit RAVU, and conditional failed-multi-group
request persistence from verified test coverage. The combined AttributeUpdate timing case,
configured-process Auto Provide solicitation, and failed-group Save/Restore recovery remain
untested. No chart or prose change was warranted. No tests, builds, CI, source, Requirements Lab,
or 2010 content changed.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE
1516.1-2025 §6.21 text. The [public IEEE record](https://standards.ieee.org/ieee/1516.1/6688/)
lists subscription access but does not expose the clause text. Determine whether a valid request
creates a direct response obligation for a requester who is otherwise ineligible to receive the
later Update. Keep request validity, provider solicitation, and receiver eligibility separate;
update the RAVU guide only if the text changes the reader-facing model. Documentation-only,
2025-only; no tests, builds, CI, Requirements Lab, or 2010 work.

2025 Connect flow local visual review (2026-11-06): Rendered the revised Connect preflight and
commit flow with Mermaid CLI 12.0.0 and inspected the local screenshot. The chart is tall but
legible, with no clipping or overlap; no layout change was needed. This closes the prior local
screenshot-review item but does not verify GitHub rendering. Inventory remains 32 guides / 448
diagrams; curated visual-QA queue is 143 reviewed / 0 pending, selected screenshot review is 194/0.
No tests, builds, CI, source, Requirements Lab, or 2010 content changed.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE
1516.1-2025 §6.21 text. The [public IEEE record](https://standards.ieee.org/ieee/1516.1/6688/)
lists subscription access but does not expose the clause text. Determine whether a valid request
creates a direct response obligation for a requester who is otherwise ineligible to receive the
later Update. Keep request validity, provider solicitation, and receiver eligibility separate;
update the RAVU guide only if the text changes the reader-facing model. Documentation-only,
2025-only; no tests, builds, CI, Requirements Lab, or 2010 work.

2025 additional-module Join and existing-instance consequences (2026-11-06): Added two
source-bounded flowcharts to the FOM module guide. One separates the newly committed catalog and
handle directories from owner maps established when existing objects were registered; the other
traces a known receiver's independent class-keyed subscription, discovery skip, and unresolved
initial-value reflection path. Mermaid CLI 12.0.0 rendering and local screenshot review passed;
GitHub rendering remains unverified. The new charts document Umbra source behavior and do not
settle the normative §5.8 outcome. There is no focused successful Join test that adds an attribute
to an existing class and follows ownership/reflection. Inventory is 32 guides / 450 diagrams (258
flowcharts, 167 sequences, 25 state diagrams; 442 backtick-fenced and eight tilde-fenced). The
curated visual-QA queue is 145/0 and selected screenshot review is 196/0. No tests, builds, CI,
source, Requirements Lab, or 2010 content changed.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE
1516.1-2025 §6.21 text. The [public IEEE record](https://standards.ieee.org/ieee/1516.1/6688/)
lists subscription access but does not expose the clause text. Determine whether a valid request
creates a direct response obligation for a requester otherwise ineligible to receive the later
Update. Keep request validity, provider solicitation, and receiver eligibility separate; update the
RAVU guide only if the text changes its reader-facing model. Documentation-only, 2025-only; no
tests, builds, CI, Requirements Lab, or 2010 work.

2025 public additional-module Join duplicate/conflict evidence review (2026-11-06): Updated the
FOM guide's Join evidence table with the promoted public C++ TCK case. It rejects selected
adapter-supplied invalid-module combinations while confirming the attempted member and extension
class remain unavailable; the catalog identifies seven standard-invalid fixtures that also
validate against the pinned DIF XSD. It accepts one matching duplicate `TckObject.Value`, rejects
an XSD-valid module changing its order from `Receive` to `TimeStamp` with `InconsistentFOM`,
observes the attempted member absent after rejection, and then accepts a valid extension retry.
Existing installed-package result JSON records EVOKED and IMMEDIATE passes (2/2). The case does
not add a new attribute to an existing class or test per-instance owner/receiver effects. Existing
diagrams were sufficient; no chart was added. Inventory remains 32 guides / 450 diagrams, curated
visual-QA queue 145/0, selected screenshot review 196/0. No tests/builds were run for this
documentation update; no Requirements Lab or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE
1516.1-2025 §6.21 text. The [public IEEE record](https://standards.ieee.org/ieee/1516.1/6688/)
lists subscription access but does not expose the clause text. Determine whether a valid request
creates a direct response obligation for a requester otherwise ineligible to receive the later
Update. Keep request validity, provider solicitation, and receiver eligibility separate; update the
RAVU guide only if the text changes its reader-facing model. Documentation-only, 2025-only; no
tests, builds, CI, Requirements Lab, or 2010 work.

2025 public additional-module Join conflict-coverage cross-review (2026-11-06): Updated the
FOM guide with the expanded public C++ TCK evidence. In addition to matching duplicate
`TckObject.Value` content, selected invalid-module rejection, and a valid retry, it now covers
schema-valid `Receive` to `TimeStamp` conflicts for both `TckObject.Value` and
`HLAinteractionRoot.TckInteraction`; each rejected Join leaves the prospective member absent.
The recorded verification confirms all three comparison fixtures pass pinned DIF XSD validation,
and the installed-package JSON/JUnit pass under EVOKED and IMMEDIATE (2/2). This does not exercise
new-attribute owner/receiver effects or exhaust conflict cases. Existing diagrams suffice, so no
chart was added. Inventory remains 32 guides / 450 diagrams; visual queues remain 145/0 and 196/0.
No tests/builds were rerun for this documentation update; no Requirements Lab or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE
1516.1-2025 §6.21 text. The [public IEEE record](https://standards.ieee.org/ieee/1516.1/6688/)
lists subscription access but does not expose the clause text. Determine whether a valid request
creates a direct response obligation for a requester otherwise ineligible to receive the later
Update. Keep request validity, provider solicitation, and receiver eligibility separate; update the
RAVU guide only if the text changes its reader-facing model. Documentation-only, 2025-only; no
tests, builds, CI, Requirements Lab, or 2010 work.

2025 public additional-module Join dimension-bound cross-review (2026-11-06):
Updated the FOM guide and evidence backlog with the selected dimension conflict:
`TckDuplicateDimension` has bound 100 in the base definition and 101 in the
additional module. Join reports `InconsistentFOM`, the prospective member is
absent, and the original bound remains 100. The case does not exercise region
behavior or exhaustive conflict rollback. The existing Join flow is sufficient;
no chart was added. The inspected installed-package JSON/JUnit records EVOKED
and IMMEDIATE passes (2/2); no tests/builds were rerun. Inventory remains 32
guides / 450 diagrams; queues remain 145/0 and 196/0. No Requirements Lab or
2010 content changed.

**Next handoff:** Resolve the explicit-request entitlement question from
authorized IEEE 1516.1-2025 §6.21 text. The [public IEEE record](https://standards.ieee.org/ieee/1516.1/6688/)
lists subscription access but does not expose the clause text. Determine whether
a valid request creates a direct response obligation for a requester otherwise
ineligible to receive the later Update. Keep request validity, provider
solicitation, and receiver eligibility separate; update the RAVU guide only if
the text changes its reader-facing model. Documentation-only, 2025-only; no
tests, builds, CI, Requirements Lab, or 2010 work.

2025 FOM handle encoding uniqueness and repeatability cross-review (2026-11-06):
Added a bounded evidence sequence to the Name/Handle guide for the portable C++
TCK. The case compares 15 representative encoded FOM handles across two
federates and checks pairwise uniqueness in one execution; it then compares 11
user-defined FDD-name encodings across two executions using the same RTI and FDD
path. `OrderType` is checked separately as an enum. The catalog maps these
observations to §§10.1.1 and 12.2. This does not establish all FOMs, RTI restart
persistence, independent-provider behavior, or unpredictability. The sequence
was rendered with Mermaid CLI 12.0.0 and screenshot-reviewed; the existing
recorded EVOKED and IMMEDIATE JSON/JUnit pass 2/2, with no test rerun.

Inventory is 32 guides / 451 diagrams (258 flowcharts, 168 sequences, 25 state
diagrams; 443 backtick-fenced and 8 tilde-fenced); all bodies have local render
coverage. The selected screenshot review is 197/0; the curated visual-QA queue
remains 145/0. The 2010 stream was not changed.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. The [public IEEE record](https://standards.ieee.org/ieee/1516.1/6688/) lists subscription access but does not expose the clause text. Determine whether a valid request creates a direct response obligation for a requester otherwise ineligible to receive the later Update. Keep request validity, provider solicitation, and receiver eligibility separate; update the RAVU guide only if the text changes its reader-facing model. Documentation-only, 2025-only; no tests, builds, CI, Requirements Lab, or 2010 work.


2025 Disable Time Constrained client-FIFO admission review (2026-11-06): Refined the existing configured-process FQR/Disable sequence to separate the callback-dispatcher queue from the earlier client `pendingEvents_` FIFO. The focused public-API process integration case uses Query GALT as an ordered stream barrier, verifies Disable does not invoke the callback, then observes the already-submitted TSO interaction delivered as receive order after Evoke; it does not include FQR/FQG. Current source shows that a TSO interaction still in `pendingEvents_` at Disable success is submitted under the new disable generation, so the generation-mismatch conversion does not strip its timestamp/order metadata and the callback bridge retains timestamped form. This second admission branch and its relative FQG order are source-only, with no normative or profile-parity claim. Updated the existing process sequence; no new chart. Mermaid CLI 12.0.0 render and screenshot review passed. 23 local link occurrences across 22 unique targets resolve; git diff --check passed. Inventory remains 451 diagrams / 32 guides; selected screenshot review is 198/0; curated visual-QA queue remains 145/0. No tests, builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. The IEEE SA product page lists purchase/subscription access but does not expose the clause; its public downloads ZIP returned HTTP 403 on 2026-11-06. Determine whether a valid request creates a direct response obligation for a requester otherwise ineligible to receive the later Update. Keep request validity, provider solicitation, and receiver eligibility separate. Documentation-only, 2025-only; no tests, builds, CI, Requirements Lab, or 2010 work.


2025 MOM Object and Interaction Reports guide post-split review (2026-11-06): Corrected a stale delete-transition link to the Object and Interaction guide. Checked all 173 Markdown links in the guide; all local targets, source line anchors, and guide-heading anchors resolve. Mermaid CLI 12.0.0 rendered the seven relocated diagrams; screenshot review found them legible and unclipped. Diagram bodies were unchanged from the prior guide. Inventory remains 32 topic guides / 451 diagrams (258 flowcharts, 168 sequences, 25 state diagrams); the selected screenshot queue remains 198/0 and GitHub layout is unverified. The dedicated 2025 guide is staged in Git. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. The public IEEE record lists subscription access but does not expose the clause. Determine whether a valid request creates a direct response obligation for a requester otherwise ineligible to receive the later Update. Keep request validity, provider solicitation, and receiver eligibility separate. Documentation-only, 2025-only; no tests, builds, CI, Requirements Lab, or 2010 work.


2025 documentation archive link-integrity review (2026-11-06): The active progress and review indexes referenced 17 dated archive files that were still untracked, including 10/11–10/16 and 11/03–11/05 records. Staged those linked files with the previously staged 11/06 archives. Checked all 1,332 archive references from the progress/review indexes against file paths, source line numbers, and GitHub-style duplicate-heading anchors; all resolve. This makes the history navigable from the staged indexes on GitHub. No HLA behavior or diagram changed. Documentation-only, 2025 documentation goal; no tests, builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. The public IEEE record lists subscription access but does not expose the clause. Determine whether a valid request creates a direct response obligation for a requester otherwise ineligible to receive the later Update. Keep request validity, provider solicitation, and receiver eligibility separate.

2025 configured-process TLS 1.3 Connect flow (2026-11-06): The Connect and
Configuration guide previously limited its configured-process flow to
`tcp://`, although the current parser also accepts `tls://`. Corrected the
address gate and documented the successful TLS 1.3 path from scheme selection,
through trusted server-certificate and host-name validation, to the private
session exchange inside TLS. The chart shows `ConnectionFailed` and no
plaintext fallback on client handshake failure. The current listener requires
a certificate-chain/private-key pair; the client verifies the server and does
not use mutual TLS.

The initial 2025 integration review covered successful `tls://` Connect and
Disconnect with a generated trusted certificate. Private tests cover TLS 1.3
byte-stream and framed-heartbeat exchanges. At that review point, the recorded
public run did not include negative TLS cases, so those branches were marked
source-derived. The later peer-validation review below records the untrusted-
peer result and preserves the host-name-mismatch and encryption-only cases as
unverified. The exact normative scope of §12.13.5 remains open pending
authorized text; this is an Umbra profile trace, not a conformance or 2010
parity claim. Thirteen new local source/test anchors resolve. Mermaid CLI
12.0.0 rendering and local screenshot review passed. Inventory is 32 guides /
452 diagrams (258 flowcharts, 169 sequences, 25 state diagrams; 444
backtick-fenced and 8 tilde-fenced); all bodies have local render coverage and
selected screenshot review is 199/0. GitHub rendering remains unverified. No
tests, builds, CI, Requirements Lab, or 2010 changes.

**Next handoff:** Resolve the explicit-request entitlement question from
authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider
solicitation, and receiver eligibility separate.


2025 TLS peer-validation-mode and negative Connect evidence review
(2026-11-06): Corrected the configured-process TLS sequence after source review
found that `tlsCertificateValidation=server` (the default) validates the
server certificate chain and host name, while `tlsCertificateValidation=none`
retains TLS encryption but skips both checks. SNI remains enabled and the
server does not request client certificates. The recorded MinGW/Ninja run built
`umbra_ieee1516_2025_catch2` and `umbra_process_boundary_private_catch2`; five
public TLS cases and two private TLS/process-boundary cases passed three
repetitions (21/21). Public evidence covers untrusted-certificate and
host-name-mismatch rejection with same-ambassador recovery, encryption-only
mode, invalid-mode rejection, and TLS default port 15165. Mermaid CLI 12.0.0
render and local screenshot review passed for the existing chart; no chart
changed in this evidence synchronization. Inventory remains 452, and selected
screenshot review remains 199/0. The exact normative scope of §12.13.5 remains
pending authorized text. This documentation update did not rerun builds or
tests. No CI, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from
authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider
solicitation, and receiver eligibility separate.


2025 RAVU §6.21 authorized-text access check (2026-11-06): The official
1516.1-2025 product record lists purchase/subscription access; IEEE's Access
Standards page offers free Reading Room access to selected standards but does
not identify whether 1516.1-2025 is included. Automated retrieval of the
Reading Room link returned HTTP 418, which says nothing about access through an
authorized account. The bounded repository filename search found the API/OMT
packages but no clause document. The entitlement question remains unresolved;
source and tests cannot replace §6.21 text.

**Next handoff:** Check §6.21 through an authorized IEEE Reading Room or
subscription account, or use authorized clause text supplied to the project.
Update the RAVU guide only if the text changes the explanation. Keep request
validity, provider solicitation, and receiver eligibility distinct.

2025 late-subscription and mixed Confirm Divestiture boundary review
(2026-11-06): Cross-checked the subscription, FOM Join, object-information,
and ownership guides against their cited source and focused test assertions.
The existing guides already distinguish a newly known object from adding an
attribute to a known object, and active per-requester Confirm Divestiture
fan-out from the source-only mixed active/stale failure. Existing diagrams
make the test and normative limits visible, so no duplicate diagram was added.
No test/build rerun, Requirements Lab, or 2010 work. The active documentation
handoff remains the authorized IEEE 1516.1-2025 §6.21 RAVU entitlement question.

2025 Confirm Divestiture fan-out visual review (2026-11-06): Rendered the existing owner/requester sequence with Mermaid CLI 12.2.0 and screenshot-reviewed the local output. Its two outcome branches, per-attribute owner commit, and per-recipient notifications remain readable and unclipped. No chart source changed. Inventory remains 32 guides / 452 diagrams; selected screenshot review is now 200/0, not a whole-inventory review. GitHub rendering remains unverified. No tests, builds, Requirements Lab, or 2010 work. The active documentation handoff remains authorized IEEE 1516.1-2025 §6.21 text.

## 2025 active Save and Restore member-exit diagram review — 2026-11-06

Refined the lifecycle guide's combined active-operation/member-exit flow into
two diagrams with separate boundaries: the shared registry transition through
membership removal, and the selected adapter's projection of Save/Restore
notifications. This makes the configured-process Restore-vector gap easier to
see without implying that a source-level omission is a tested callback result.
Mermaid CLI 12.2.0 rendered both diagrams, and local screenshot review found
their decisions and paths readable without clipping. The active-operation
transition and process dispatch findings remain bounded by the cited registry,
adapter, and focused embedded-test evidence; no normative failure-reason or
process runtime claim was added. Inventory is 32 guides / 453 diagrams (259
flowcharts, 169 sequences, 25 state diagrams; 445 backtick-fenced and eight
tilde-fenced); all 453 current bodies have local render coverage. Selected
screenshot review advances from 200 to 202, with none pending. GitHub rendering
remains unverified. No tests, builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from
authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider
solicitation, and receiver eligibility separate.

## 2025 synchronization barrier late-join diagram review — 2026-11-06

Rechecked the source-derived case where a member reports `false` before an
omitted-set synchronization point expands to a late joiner. The first result
remains stored, while the added participant increases the count required to
complete the barrier. The focused direct and public late-join tests do not
exercise this order, so the guide keeps it explicitly source-derived.

The sequence and state diagrams had crowded labels. Their text is now shorter
while retaining participant membership, first-result retention, barrier
counts, and the boundary between notification planning and callback delivery.
Mermaid CLI 12.2.0 rendering and local screenshot review passed for both.
Inventory remains 32 guides / 453 diagrams; selected screenshot review is
204/0. GitHub rendering remains unverified. No tests, builds, Requirements Lab,
or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from
authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider
solicitation, and receiver eligibility separate.

## 2025 Exception Reporting process callback timing review — 2026-11-06

Refined the existing Exception Reporting flow to separate the pushed MOM
report event from the rejected service result in the configured-process
receive-order path. Source shows the server pushing the eligible report before
the rejected response, then the client dispatching the buffered event before
`sendInteraction` maps the rejected status. The focused push-enabled
`HLA_EVOKED` test observes `RTIinternalError` first and the MOM callback on a
later `evokeCallback`. The `HLA_IMMEDIATE` inline callback path is source-only.
No combined Exception Reporting case or three-route test is claimed; this is
an implementation observation, not a normative or profile-parity claim.
Mermaid CLI 12.0.0 rendered the existing diagram; local screenshot review
found it legible and unclipped. Inventory remains 32 guides / 453 diagrams;
selected screenshot review advances from 204 to 205, with none pending.
GitHub rendering remains unverified. No tests, builds, Requirements Lab, or
2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 registry FOM handle allocation reader review — 2026-11-06

The [Name and Handle guide](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md)
now distinguishes the portable TCK's bounded public encoding observations from
the current registry allocator that supplies FOM-derived handle identities.
Two focused diagrams separate candidate selection from Create/additional-FOM
Join publication. They show the shared registry map, OS-random candidate,
reserved values and collision limit, allocation failure, replacement-directory
commit, and the source-derived possibility that a private mapping remains after
a later Join allocation fails. The focused internal test source checks
allocator-copy reuse, reserved values, failed Create, and preservation of the
published Join catalog and handle snapshots; it does not inspect the private
map and was not run in this review.

Both diagrams rendered with Mermaid CLI 12.0.0 and were screenshot-reviewed as
readable and unclipped. The 2025 inventory is 32 guides / 455 diagrams (260
flowcharts, 170 sequences, 25 state diagrams; 447 backtick-fenced, 8
tilde-fenced); all 455 bodies have local render coverage. The curated screenshot
review advances from 217 to 219, with none pending. These allocator and test
sources are current uncommitted worktree changes; no test/build result or
release-level behavior is claimed. No Requirements Lab or 2010 content changed.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 FOM composition and additional-module Join reader review — 2026-11-06

Cross-read the existing [additional-module Join guide section](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#join-with-additional-fom-modules-replace-definition-and-add-member-together),
registry commit path, and focused process case. The current diagram already
separates complete-set validation and composition, the shared registry commit
of the replacement definition/handle directories with the new membership,
and profile-specific post-commit setup and rollback. The process fixture
asserts that one rejected pre-commit Join leaves the FOM module count unchanged
and a valid retry adds a module and resolves its class; it does not observe
post-commit failure, byte-for-byte FDD preservation, or a populated member set
before and after rejection. Existing charts and evidence notes make these
limits visible. The Lifecycle guide's three-lifetime map supplies the outer
connection/membership/execution context, so no combined chart or guide text is
warranted. Keep rollback persistence source-bounded and make no normative or
profile-parity claim.

The [registry commit](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L618)
and [focused process case](../../cpp/tests/process_federation_additional_fom_catch2.cpp#L430)
remain the relevant evidence. No tests, builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 Subscription State late-subscription visual review — 2026-11-06

Rechecked the existing [Subscription State guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md)
and its focused evidence boundary. Its receiver-state map distinguishes an
object becoming newly discoverable, where selected stored values have focused
test-source support, from an already-known object receiving an added attribute,
whose value-only effect remains untested and normatively unresolved. The
companion effects map separates receiver scope advice, provider relevance
advice, and that open current-value question. The profile-specific path is
already diagrammed separately, so no duplicate chart or behavioral claim was
added.

Mermaid CLI 12.0.0 rendered both existing charts. Local screenshot review found
the decision branches and labels legible without clipping. The inventory stays
32 guides / 453 diagrams; selected screenshot review advances from 205 to 207,
with none pending. GitHub page rendering remains unverified. No tests, builds,
Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 joined-federate MOM removal/callback visual review — 2026-11-06

Rendered and visually reviewed three existing 2025 diagrams in the
[Joined-Federate MOM guide](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md):
the explicit-Resign versus forced-loss route-availability flow, the embedded
`HLA_IMMEDIATE`/`HLA_EVOKED` throwing-Remove sequence, and the configured-
process Push/Pull callback-failure sequence. Mermaid CLI 12.0.0 renders are
legible and unclipped. They preserve the callback-entry removal boundary,
callback-model-specific unwind point, and the process server/client boundary.
The throwing callback and forced-loss branches remain source-derived because
no focused test exercises them; this review makes no normative or cross-profile
claim. No chart rewrite was needed.

Inventory remains 32 guides / 453 diagrams; selected screenshot review advances
from 207 to 210, with none pending. GitHub page rendering remains unverified.
No tests, builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 Save/Restore process callback-failure visual review — 2026-11-06

Rendered and screenshot-reviewed two existing sequences in the
[Federation Save and Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md):
terminal callback observation versus the `FederateSaveComplete(true)` service-
report boundary, and an immediate Save initiation callback throwing after the
server has admitted the operation, contrasted with the deferred `HLA_EVOKED`
path. Mermaid CLI 12.0.0 renders show the decision branches and labels without
clipping. Both exception outcomes remain source-derived; no focused public
process test exercises a throwing initiation or terminal callback. This review
adds no normative claim, profile-parity inference, or new chart.

Inventory remains 32 guides / 453 diagrams; selected screenshot review advances
from 210 to 212, with none pending. GitHub page rendering remains unverified.
No tests, builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 deletion authority and producer identity visual review — 2026-11-06

Cross-read the existing embedded transfer-and-delete sequence and the
configured-process removal-identity sequence in the
[Attribute Ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md). The
embedded sequence already separates delete privilege from registration and
shows that the new owner B can delete while the callback identifies original
registrant A in the selected test. The process sequence was too wide for
comfortable page reading, so it now focuses on the key implementation boundary:
the request message records B, while the later receiver-removal snapshot and
event carry A. A note retains the callback-bridge projection and the A != B
test gap. Mermaid CLI 12.0.0 rendered both existing sequences; screenshot
review found them readable without clipping.

This remains a bounded source/embedded-test finding. The process deletion tests
use one member for registration and deletion, so the transferred-owner callback
identity has no focused process assertion. The review makes no normative
§6.17 conclusion or process-parity claim. No new diagram was added.

Inventory remains 32 guides / 453 diagrams; selected screenshot review advances
from 212 to 214, with none pending. GitHub page rendering remains unverified.
No tests, builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 ResignAction success-path callback visual review — 2026-11-06

Rendered and screenshot-reviewed the embedded 2025 ResignAction admission
flowchart and callback sequence in the
[Federation and Federate Lifecycle guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md).
The flowchart owns the action flags, final-member delete overlay, and
pre-mutation rejection gates. The sequence had repeated those branches, making
its labels small; it now follows the successful path through registry commit,
the departing ambassador's callback-queue reset, and survivor-route dispatch.
Mermaid CLI 12.0.0 screenshot review found both charts readable and unclipped.

This remains an embedded source trace. The cited `HLA_EVOKED`
`DELETE_OBJECTS_THEN_DIVEST` case covers one action and callback path; it does
not establish general callback ordering, an `HLA_IMMEDIATE` action sequence,
or configured-process parity. Exact §§4.12.3–4.12.4 interpretation remains
standards-text-dependent. No new chart was added.

Inventory remains 32 guides / 453 diagrams; selected screenshot review advances
from 214 to 216, with none pending. GitHub page rendering remains unverified.
No tests, builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 FOM-to-DataElement-to-service reader-path cross-review — 2026-11-06

Cross-read the FOM Module Admission and Composition, DataElement Encoding, and
Object and Interaction guides. The FOM guide correctly treats the composed
model as vocabulary and states that admission does not execute a federate's
encoder. The DataElement guide covers application-selected value encoding and
separates `VariableLengthData` from process framing. The Object and Interaction
guide already traces declared member type through compatible element selection,
handle-keyed service maps, callbacks, and receiver decoding, with layered
evidence and explicit limits. Its existing flow is sufficient; no duplicate
diagram was warranted. Added a direct link from the FOM boundary to that
service/callback path to make the three-guide reading route easier to follow.

This is a navigation and reader-path review, not evidence for every element
type, end-to-end transport behavior, or profile parity. Inventory remains 32
guides / 453 diagrams; selected screenshot review remains 216/0. No tests,
builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 federation execution-to-discovery orientation chart visual review — 2026-11-06

Rendered and screenshot-reviewed the 2025 [conceptual flow from ambassador
construction to observer discovery](HLA-BEHAVIOR-FLOW-GUIDES.md#from-ambassador-construction-to-observer-discovery).
The tall flow remains legible and unclipped. Its branches distinguish Connect
from federation membership, Create from Join, FOM vocabulary from live object
instances, and publisher registration from each observer's discovery decision.
No diagram edit was needed.

The chart synthesizes separate API and focused-test examples; it is not one
end-to-end test and does not establish every normative discovery-eligibility
rule or embedded/process parity. Inventory remains 32 guides / 453 diagrams;
selected screenshot review advances from 216 to 217, with none pending.
GitHub rendering remains unverified. No tests, builds, Requirements Lab, or
2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 Authorization explicit-no-credentials evidence cross-review — 2026-11-06

Cross-read the Authorization guide's no-authorizer Connect explanation with
the focused connection tests and the pinned `HLAnoCredentials` declaration.
The targeted authorization-disabled test rejects supplied passwords and then
successfully reconnects with credentials omitted. A separate all-overloads
case constructs `HLAnoCredentials` and exercises both credential-bearing
Connect overloads, expecting Connect and Disconnect to succeed. That case does
not explicitly select or assert an authorization-disabled RID profile, or
inspect payload size. Updated the Authorization evidence text to name both
scenarios and preserve their distinct limits. No diagram changed and no test
was rerun.

This is embedded test-source evidence only; it does not establish configured-
process authorization parity or resolve the pinned overload-specific §4.2
throw-list question. Inventory remains 32 guides / 453 diagrams; selected
screenshot review remains 217/0. No builds, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 configured-process Auto Provide solicitation cross-review — 2026-11-06

Cross-read the [Auto Provide guide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md) against the configured-process subscription/discovery worker, switch handler, and focused public switch case. The process route rechecks and commits receiver-known state, queues discovery, and plans ordinary initial values separately. `HLAsetSwitches(HLAautoProvide)` updates shared switch state. The focused process case confirms the switch's initial, disabled, and re-enabled values but does not arrange discovery or assert `provideAttributeValueUpdate`.

The guide's existing source-bounded chart already separates those two paths and labels process solicitation unverified; no duplicate diagram was warranted. The inspected switch handler and test have uncommitted worktree changes. Source and test code were inspected, not run. No normative omission or embedded/process parity is inferred. Inventory remains 32 guides / 455 diagrams; no chart count change. No Requirements Lab or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 MOM object-information target/handler mismatch cross-review — 2026-11-06

Compared the 2025 object-information request in the pinned MIM with the embedded handler, planner, encoder, and focused test. The MIM inherits `HLAfederate` as the selected target, while the embedded handler skips that parameter and plans from the request sender. The focused case sends only `HLAobjectInstance`, so it does not cover a target distinct from the requester. Updated the existing guide explanation and chart to separate the MIM request shape from current embedded behavior; this is a source/test gap, not a conformance conclusion.

Mermaid CLI 12 rendered the revised chart, and local screenshot review found it readable and unclipped. The existing chart count is unchanged. No tests, builds, or CI were run. Inventory remains 32 guides / 455 diagrams; selected screenshot review remains 219/0. No Requirements Lab or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question from authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider solicitation, and receiver eligibility separate.

## 2025 configured-process MOM report-query route cross-review — 2026-11-06

A bounded 2025 survey compared the joined-federate MIM report-query pairs with the configured-process Send Interaction handler and focused test sources. The reviewed report queries have no request-specific branch in that handler; they fall through to `planReceiveOrderInteraction` at the current source anchor, whose registry planner checks sender publication. The generic route prepares ordinary interaction recipients and does not invoke a paired joined-federate report planner. No focused configured-process query/report case was found. This is an implementation evidence gap; public runtime outcome and conformance remain unverified. Corrected five stale report-guide source anchors from L389 to the actual generic-planner call at L546 and added one cross-cutting source-boundary flowchart. The chart was rendered with Mermaid CLI 12.0.0 and locally screenshot-reviewed; it is readable and unclipped. Seven added local references and the corrected planner/admission anchors resolve. No test, build, CI, Requirements Lab, or 2010 work.

The report guide keeps configured-process Service Invocation, Exception, and Federation MOM reporting separate from these joined-federate query interactions. Inventory is 32 guides / 456 diagrams (261 flowcharts, 170 sequences, 25 state diagrams; 448 backtick-fenced and eight tilde-fenced); all 456 bodies have local render coverage. The selected screenshot review is 220/0; GitHub rendering remains unverified.

**Next handoff:** Cross-check inherited `HLAfederate` target handling across the 2025 joined-federate MOM report queries: request decoding, planner selection, report dimension, and focused-test target shape. Keep source/test evidence distinct from standards-text questions, including the still-open §6.21 entitlement question.

## 2025 joined-federate MOM query-target cross-review — 2026-11-06

Added a compact target-selection map to the 2025 MOM reports guide. It
distinguishes the requester, the `HLAfederate` selected for the report, and the
federate that observes the report callback. The map records that the embedded
object-instance information handler uses the sender and its focused case omits
the inherited target; the other reviewed handlers decode the target and route
to their family planner. Focused target shapes vary: selected object-instance,
update, and broad interaction-send tests use a target distinct from the
requester, while the populated interaction-receipt and reflection-activity
queries use the requester. The directed interaction-send case queries its
requesting subject. These are test-shape observations, not claims that each
test asserts the report dimension or that configured-process behavior matches.

No diagram changed. The MOM guide's 201 local links resolve and every numbered
line anchor is in range. The 32-guide/456-diagram inventory and 220/0 selected
screenshot-review count are unchanged. No tests, builds, Requirements Lab, or
2010 work.

**Next handoff:** Verify which focused joined-federate MOM tests observe the
report's `HLAfederate` dimension versus merely supplying a target parameter.
Keep those evidence types distinct, and leave RAVU entitlement in the §6.21
standards-text queue until authorized canonical text is available.

## 2025 MOM report-target dimension evidence review — 2026-11-06

Followed the target selected by each reviewed embedded MOM report family into
recipient planning. The planners construct an `HLAfederate` endpoint region
from the reported federate ID and pass it into candidate recipient selection;
the object-information planner is cited directly, and family-specific planner
rows remain in the guide's evidence map. Focused report tests subscribe to the
report class without a region and inspect the callback payload. Their
`sentRegionsSupplied == false` checks refer to the callback's sent-region
argument and do not expose the internal endpoint region. Distinct-target cases
exercise target-specific report payloads, but they do not directly assert
dimension-filtered recipient selection. Updated the guide and backlog to keep
that evidence boundary explicit.

No diagram changed. The 2025 inventory remains 32 guides / 456 diagrams, with
all 456 locally rendered and 220/0 in the curated screenshot review. No tests,
builds, Requirements Lab, or 2010 work.

**Next handoff:** Review the configured-process nonregional receive-order
delivery path from server admission through registry eligibility and callback
handoff, checking focused tests for each boundary. Keep source-only findings
separate from portable behavior claims and retain the §6.21 question in the
standards-text queue.

## 2025 configured-process nonregional receive-order path review — 2026-11-06

Cross-checked the existing Process Endpoint guide against the configured-
process Send Interaction service, registry plan, process event queues, client
poll/push paths, callback bridge, and focused 2025 tests. The guide already
shows session/member and registry admission, projected recipient events,
private push frames versus polling, callback reconstruction, and the separate
callback-model boundary. The raw public-send test checks literal parameter and
tag bytes through polling. The multi-recipient public test checks exact-class
callback FIFO in both callback-model fixtures, pairing evoked polling with
immediate push as test setup. Existing cross-links cover receiver eligibility
and callback/event ordering. These sources and tests do not establish a general
callback-before-send-return rule, inherited-class promotion, regional routing,
or a universal callback-model/transport pairing.

The current guide already presents this path and its evidence limits, so no
duplicate diagram or prose change was needed. No tests, builds, Requirements
Lab, or 2010 work.

**Next handoff:** Cross-check the open successful additional-module Join case
that adds an attribute to an existing class. Trace catalog change, existing
instance ownership, a receiver that already knows the object, and later value
observation. Keep source behavior, focused tests, and unresolved normative
questions separate; do not infer an immediate reflection.

## 2025 Join-added attribute ownership cross-review — 2026-11-06

Found a focused embedded test in
`cpp/tests/ieee1516_2025_attribute_ownership_check_catch2.cpp#L165`. Its valid
custom extension adds an attribute to an existing class with a registered
instance. The test observes discovery of that object, the added attribute's
initially unowned state, and ownership only after the acquisition notification.
It does not send a value update or assert receiver reflection. Corrected the
FOM guide's stale evidence claims, added the case-specific sequence, and
rendered it with Mermaid CLI 12.0.0. Screenshot review found the shortened
labels readable and unclipped. The Mermaid inventory is now 457 charts across
32 guides (261 flowcharts, 171 sequences, 25 state diagrams; 449 backtick-
fenced and 8 tilde-fenced); all 457 have local render coverage and 221 have
selected screenshot review. Current-value reflection and normative §5.8 remain
open. No C++ tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Review whether a receiver that already knows an object
observes a value after a successful additional-module Join adds an attribute
and ownership is acquired. Start with existing source and tests; keep
current-value reflection and canonical §5.8 interpretation explicitly open.

## 2025 Join-added attribute value-observation cross-review — 2026-11-06

Cross-checked the known-object subscription path after the successful
Join-added-attribute ownership case. Embedded subscription mutation returns
scope/advisory plans and replans discovery; the known-object filter skips a
second discovery, while the ordinary initial-value planner is reached from the
post-discovery callback path. Configured-process initial values likewise run
from newly accepted discovery, and known-object subscription does not enter
that route. A later Update Attribute Values call follows the ordinary
recipient planner, which checks the receiving member's known class and current
attribute declaration/subscription state. Existing public process test source
covers ordinary byte-preserving reflection when subscription precedes
registration and update, but does not combine this with a Join-added attribute
or prior-known receiver. Revised the Subscription State and FOM guides to
separate current-value catch-up from a later update, updated the known-object
flowchart, and rendered/reviewed it with Mermaid CLI 12.0.0. No tests/builds
were run. Normative §5.8 and the combined scenario remain open.

## 2025 provider-grouping and Auto Provide evidence cross-review — 2026-11-06

The RAVU guide already gives a source-derived same-object/two-provider grouping
example and explicitly states that focused embedded/process tests do not prove
that fan-out. Its evidence table distinguishes one-provider/two-object process
coverage, one-provider regional coverage, and the absent same-object case. The
Auto Provide guide likewise distinguishes discovery initial reflection from
provider solicitation, and its source map marks the automatic planner call as
embedded-only; the configured-process discovery chain has no corresponding
planner call and no focused automatic-solicitation test. These gaps are already
clear and bounded in the guides, so this review needed no new diagram or prose.
No tests or builds were run. No normative or cross-profile equivalence claim.

## 2025 transferred-owner removal identity cross-review — 2026-11-06

Rechecked the Object and Interaction and Attribute Ownership guides against
the embedded registry and configured-process removal paths. The embedded test
transfers `HLAprivilegeToDeleteObject` from registrant A to B, verifies A's
delete rejection and B's successful deletion, then checks that C's removal
callback names A as producer and carries B's deletion tag. Embedded source
selects removal recipients from other joined members who know the object;
receiver cleanup snapshots the original registration producer. The process
TSO message separately records the deleting requester, while receive-order
and TSO public removal events carry the registration producer from the
receiver-removal snapshot. The focused receive-order and timestamped process
tests use the same federate to register and delete, so they do not exercise a
distinct delete-privilege owner. Both guides already state these limits and
link the supporting code/tests; no content or diagram change was needed. No
tests or builds were run. This remains source evidence, not process runtime or
normative verification.

## 2025 class-request partial-persistence Save/Restore cross-review — 2026-11-06

Refined the RAVU class-request charts by separating per-group registration
and dispatch from a later conditional Save/Restore continuation. Source shows
that a later-group registration failure may leave earlier group records
pending, with no provider callbacks queued because dispatch begins only after
all group registrations succeed. State-image capture serializes those records,
but image admission accepts a class-request ledger only when the object has
nonempty values, pending-operation counts match the class-request records, no
other disallowed callback ledgers or update-region associations are present,
and the records have valid ordered identities. Restore then validates live
requester/provider membership, object/class and attribute validity, and
unchanged current ownership; restore-control reconstructs provider work, which
the ambassador queues after `federationRestored`. The public test confirms one
successful object/provider group through both callback models, but does not
exercise a later-group failure followed by Save/Restore. Updated the guide to
make these gates explicit and rendered/screenshot-reviewed both diagrams with
Mermaid CLI 12.0.0. The dispatch chart remains tall but readable when expanded;
the recovery chart is compact. Inventory is 458 diagrams across 32 guides
(262 flowcharts, 171 sequences, 25 state diagrams; 450 backtick-fenced and 8
tilde-fenced); all 458 have local render coverage, with 222 selected screenshot
reviews. No tests/builds were run. This is conditional implementation evidence,
not a tested recovery guarantee or an HLA rule.

## 2025 synchronization partial-achievement / late-Join cross-check — 2026-11-06

Rechecked the synchronization guide's existing sequence and state view against
the 2025 embedded registry and focused tests. Source confirms that a late join
is added to both participant and announced sets only for an expandable pending
point; earlier achievement entries remain intact. Achievement uses one map
entry per federate, retains the first boolean on repeats, and compares distinct
result count to the current participant-set size before forming notifications.
Thus a partial result followed by an expanding Join increases the remaining
achievement count, and the false result remains represented in the eventual
failed set. The guide already shows this behavior and identifies it as
source-derived rather than a normative completion rule.

Focused coverage is narrower: the registry late-join test verifies expansion
and the explicit-set boundary before any achievement; the public scenario has
achievements, but not a partial achievement before a late Join. Configured-
process service code calls the same registry transition, while its cited tests
do not cover that interleaving. No new diagram or source edit was warranted.
Updated the synchronization guide's implementation/test evidence row to make
the missing interleaving explicit. No tests or builds were run; no normative,
process parity, or cross-profile claim is added.

## 2025 synchronization repeated-result cross-check — 2026-11-06

The registry uses `map::emplace` for each participant's achievement result.
While the point remains pending, repeating with the opposite boolean therefore
leaves the first stored result unchanged; only that first value contributes to
the eventual failed set. The focused public scenarios report once per
participant, and their false-result assertions concern the first report. No
focused case verifies an opposite-boolean repeat before completion. Updated the
synchronization guide to label first-result-wins as source-derived and to state
the test and normative interpretation gaps. No chart was needed, and no tests
or builds were run.

## 2025 synchronization resignation after achievement cross-check — 2026-11-06

The shared 2025 resignation cleanup removes the departing federate from the
participant, announcement, and achievement sets before reevaluating the
barrier. Therefore a departing participant's stored `false` result is removed
and is not included in the eventual failed set; remaining participants close
the point only if all have already reported, otherwise it remains pending. The
focused embedded HLA_EVOKED test covers one achieved-success member followed by
the unachieved second member's Resign, producing an empty failed set and a
Federation Synchronized service report before callback delivery. It does not
cover a departing recorded result or the still-pending branch. Updated the
synchronization guide's prose and evidence row to expose these limits. No tests
or builds were run; this is implementation behavior, not a normative
resignation interpretation, and no 2010 behavior is inferred.

## 2025 explicit-set registrant departure cross-check — 2026-11-06

The stored synchronization point has no registrant identity. Resignation
cleanup changes only the participant, announcement, and achievement sets, so
an outside-set registrant's departure does not remove or resize the pending
barrier; completion notifications remain planned for the synchronization set.
The configured-process receiver-only-set scenarios resign the sender only
after the receiver completes the point. They establish distinct registrant and
participant roles, but not resignation during the pending interval. Updated
the synchronization guide to make that test timing explicit. No test/build or
normative interpretation was added, and no 2010 behavior is inferred.

## 2025 synchronization registration rejection cross-check — 2026-11-06

The embedded registry checks the pending label, requested member membership,
and callback routes before inserting a new point or returning announcement
plans. A duplicate label or nonjoined requested member therefore cannot
replace an existing point. Focused duplicate-label coverage is strong: the
embedded case verifies the original tag, late-Join expansion, and eventual
completion survive the rejection; configured-process HLA_EVOKED and
HLA_IMMEDIATE cases verify duplicate failure and that an outside-set
registrant cannot achieve. No focused implementation test was found for a
requested synchronization set containing a nonjoined federate. Updated the
guide's registration explanation and evidence table; no diagram or normative
claim was added. No tests/builds were run; no 2010 stream was inspected.

## 2025 synchronization announcement delivery cross-check — 2026-11-06

Rechecked the guide's delivery boundary against both adapters and focused
cases. Embedded registration submits the registration result before the
announcement batch, but its HLA_EVOKED/HLA_IMMEDIATE public scenarios collect
the callbacks separately and do not assert cross-callback order. The process
service queues announcement events before returning its registration response;
the client dispatches the correlated registration result and processes pushed
events at its own callback boundary. The focused HLA_EVOKED same-participant
case observes registration success after one Evoke while the sender's
announcement is still absent, then observes it after the next callback
service. Receiver-only HLA_EVOKED and HLA_IMMEDIATE tests do not establish
same-recipient order, and no configured-process HLA_IMMEDIATE same-participant
case was found. The guide already makes these limits explicit; no duplicate
diagram or prose edit was warranted. No tests/builds were run, and no general
callback-order or profile-parity claim is made.

## 2025 initial synchronization registration enqueue-failure review — 2026-11-06

The configured-process registration handler commits the point in the shared
registry before enqueueing its announcement plan. If enqueueing fails, it
returns an internal-error response without removing the stored point; a retry
with that label therefore encounters the existing point. In polling mode,
recipient events are appended one by one, so entries from earlier recipients
can remain when a later enqueue fails. In push mode, events are staged before
the send loop; the test hook can fail before any staged event is sent, while a
transport send failure in the send loop can follow earlier successful sends.
The existing injected-failure test exercises late-Join rollback only: Join
cleanup removes the joining member and retry delivers one announcement. It
does not establish initial-registration rollback or retry behavior. Added a
source-derived flow to the synchronization guide and explicitly separated the
coverage. No tests/builds were run; no HLA transactional rule or 2010 behavior
is inferred.

The process client converts a non-OK registration response to a client error,
which the 2025 ambassador maps to `RTIinternalError`; it does not dispatch a
normal registration result callback on that path. The existing injected Join
failure test uses default polling mode (`pushReceiveOrderEvents == false`) and
does not test the initial registration handler. Thus neither the polling
partial-queue effect during initial registration nor the push-mode staging and
send-failure branches have focused coverage. Added a Mermaid flow distinguishing
the committed point, the mode-dependent event effects, the error response, and
the retry path. Mermaid CLI 12.0.0 rendered it successfully and screenshot
review found the branches readable and unclipped. Updated the guide evidence
table and current diagram inventory to 459 (263 flowcharts, 171 sequences, 25
state diagrams; 451 backtick and 8 tilde fences); all bodies have local render
coverage and curated screenshot review is 223/0. Refined the guide accordingly.

Audited every relative file and numbered source-line link in the synchronization
guide after the edits: all resolve. Re-rendered the new failure flow with
Mermaid CLI 12.0.0; syntax passed and screenshot review found no clipping.
Roadmap JSON parses and `git diff --check` passes for the changed documentation.

## 2025 Auto Provide configured-process discovery cross-check — 2026-11-06

Rechecked the 2025 Auto Provide guide against the process subscription,
discovery, and callback-bridge source. The process route commits receiver-known
state, creates discovery, plans ordinary initial-value reflection separately,
and has no `planAutoProvideForDiscovery` call; the embedded discovery callback
does invoke that planner. The focused process late-subscription case asserts a
selected current-value reflection, but its receiver does not instrument or
assert `provideAttributeValueUpdate`. The separate process `HLAsetSwitches`
case checks shared switch state without arranging discovery. Thus the guide's
process solicitation gap is still accurate; no embedded/process parity or
negative test claim follows. Clarified the evidence table to state that the
reflection test proves neither solicitation nor its absence. No diagram,
tests, or builds changed.

## 2025 regional RAVU same-object multi-provider cross-check — 2026-11-06

The shared registry scans each eligible object, partitions requested remote
attributes by current owner, then applies per-attribute request-region and
provider update-region overlap filtering before emitting each nonempty
object/provider group. For configured-process dispatch, the service narrows
the persisted request-region map to the group's attributes, registers that
provider work with the original request tag, reprojects the group, and builds
one event from its provider/object/attribute subset and that same tag; the
request-region designators themselves are absent from the event type. The
focused public process regional case verifies one provider, one object, one
attribute, and the original request tag. The private process case covers one
provider/object/attribute with a disjoint selector and zero recipients. Neither
establishes same-object multi-provider fan-out, per-provider tag repetition,
or callback count/order across groups. Clarified the RAVU guide. No diagram,
tests, or builds changed; this remains source behavior rather than a normative
callback-count rule or embedded/process parity claim.

## 2025 regional RAVU whole-request invalid-pair preflight cross-check — 2026-11-06

The shared regional class planner validates every attribute/region pair before
expanding objects or provider groups. The configured-process handler rejects
the typed planner status before its provider-event loop, and the embedded
adapter likewise throws before callback submission. The embedded filtering
test submits three separate invalid single-pair requests while provider-owned
objects exist, then drains callbacks and observes none before proceeding to
later valid requests. This supports no callback for those individual invalid
requests, but does not test one request vector containing both valid and
invalid pairs. The configured-process public invalid-selector case verifies
four exception/status mappings but has no published object or provider route,
so it does not prove event suppression. No focused test inspects pending
records or retries the same request after fixing a region. Clarified the RAVU
guide's prose and evidence row; no tests/builds or normative atomicity claim.

**Next handoff:** Review the configured-process regional RAVU group loop when
one provider group is registered and a later group fails before all groups
are enqueued. Trace retained pending records, event visibility, retry behavior,
and focused failure coverage; keep this regional path distinct from the
already documented ordinary class-request partial-persistence case.

## 2025 regional RAVU per-group failure and retry cross-check — 2026-11-06

Traced configured-process regional class RAVU from per-provider registration
through pending-record consumption and Pull/Push delivery. Each group record
is consumed by the registry's begin/recheck operation before an event is
queued or staged. If a later registration fails, earlier groups therefore do
not retain pending records for automatic replay. Pull events already appended
to provider queues remain pollable; Push returns before sending the local
staged batch, so those frames are not sent. If the later failure is instead a
Push send failure, an already-sent prefix may be visible. Retrying the public
request starts a new operation and may duplicate work already visible to a
provider. No focused test injects these failures or checks retries, so these
are source-derived consequences, not observed runtime behavior or HLA
atomicity guarantees. Updated the RAVU guide and its failure explanation; no
diagram change was needed. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Resolve the explicit-request entitlement question using
authorized IEEE 1516.1-2025 §6.21 text. Keep request validity, provider
solicitation, and receiver eligibility separate; update the RAVU guide only if
the normative text changes its reader-facing model. The public IEEE record
currently lists subscription access without exposing clause text.

## 2025 §6.21 authorized-text availability check — 2026-11-06

Checked the official IEEE 1516.1-2025 product page and the repository's
officially imported 2025 API headers. The product page exposes Purchase and
Access via Subscription links and lists the machine-readable downloads; it
does not expose §6.21 prose. The local import is the C++ API header tree, not
the normative standard text. No clause interpretation can be verified from
these materials, so the direct-request entitlement question remains open. Do
not substitute the 2010 clause or infer a 2025 rule from the public service
signature or exception list.

**Next handoff:** Conduct a bounded 2025 guide-portfolio reader-gap survey.
For each of the 32 current topic guides, check whether it explains the standard
concepts, state transitions and callback effects, important edge cases,
Umbra source/test evidence, and known gaps with 2010 kept separate. Identify
one materially incomplete topic from actual omissions and update that guide
and the backlog. Do not use diagram count as a completion target.

## Completed audit and early visual-review history

The completed topic-guide audit and early visual-review entries (increments 1–80) are preserved in the
[roadmap history archive](HLA-BEHAVIOR-FLOW-GUIDES-ROADMAP-HISTORY-2026-11-07.md).
Increment 41 retains its established heading here because the guide index links to it; its full detail is archived.

## 2025 configured-process ConnectionLost TSO cutoff boundary, increment 41 — 2026-11-06

Full entry: [archived progress history](HLA-BEHAVIOR-FLOW-GUIDES-ROADMAP-HISTORY-2026-11-07.md#2025-configured-process-connectionlost-tso-cutoff-boundary-increment-41--2026-11-06).

\r\n