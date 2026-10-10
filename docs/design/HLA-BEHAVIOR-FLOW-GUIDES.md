# HLA Behavior Flow Guides: Umbra Backlog

This is the living index for contributor-facing HLA flow diagrams in Umbra.
The guides explain difficult behavior before asking a reader to navigate the
implementation; they are not replacements for IEEE standards, implementation
contracts, or test evidence.

## Guide pattern and boundaries

Use the [2025 time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md) as the
first structural example. Each new guide should:

- State its API edition and implementation profile at the top. A 2010 guide
  must be separate from a 2025 guide; never combine state machines or imply
  compatibility unless that equivalence has been demonstrated.
- Use GitHub-renderable Mermaid diagrams, with service calls, callbacks, and
  state-commit points labeled. Prefer several cohesive diagrams over one
  all-encompassing chart.
- Separate standard concepts, current Umbra behavior, test observations, and
  known limits. The standard is normative; code and tests describe only the
  implementation and scenarios exercised.
- Link implementation claims to the relevant source and focused tests. Treat
  a passing test as evidence for its scenario, not proof of complete support or
  conformance.
- Identify interactions with other areas (time, ownership, regions, save and
  restore) without silently absorbing those state machines into the guide.
- Keep this work in Umbra documentation. Any later translation into or
  expansion of the HLA Requirements Lab is a separate phase and is not part of
  this backlog's current authorization.

## Edition-boundary text audit — 2026-10-09

A bounded scan of the 29 current 2025 guides for `2010` and
`rti1516_2010` markers found only explicit separation notes, legacy-model input
or catalog-only compatibility descriptions, and clearly labeled parallel
implementation references. The DataElement guide's comparison diagram gives
2010 and 2025 separate subgraphs and source paths; it does not combine codec
behavior or tests. In particular, `fomEdition=2010` is described as a
compatibility input to the 2025 ambassador, not as the 2010 RTI stream. The
separate 2010 reference-RTI guide and the new 2010 encoding companion remain
edition-specific; neither is used to fill evidence gaps in a 2025 guide. This is
a documentation-marker review, not evidence of cross-edition equivalence;
future shared explanations still need direct evidence from both streams.

## Suggested reader paths

The guides are standalone topic explanations. These paths are a navigation
aid, not a claim that adjacent topics are one normative state machine or that
the implementation profiles are equivalent.

- **Federation setup, model, and visibility:** [federation lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md) → [FOM admission/composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md) → [object-instance name reservation](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md) → [object/interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md) → [federation-scoped MOM](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md) → [joined-federate MOM lifecycle](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md).
- **Connect before joining; disconnect after resigning:** [Connect, Disconnect, and configuration](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md) explains the local RTI session gates; [authorization and credentials](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md) follows the 2025 authorizer from Connect through Create/Destroy/Join; [federation lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md) follows the separate membership and execution lifetimes.
- **Why an update or interaction reached a federate:** start with [object/interaction information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md), then choose the relevant lens: [object-attribute subscription state](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md), [explicit attribute-value requests](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md), [ownership transfer](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md) or its [persistence and resignation companion](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md), [DDM/regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md), [attribute update-rate admission](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md), [relevance advisories](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md), [attribute scope](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md), [Auto Provide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md), [Delay Subscription Evaluation](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md), or [directed interactions](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md).
- **How timestamp, order, transport, callbacks, and retraction relate:** [time management](HLA-2025-TIME-MANAGEMENT-GUIDE.md) → [callback/service ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md) → [order and transportation](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md) → [TSO retraction](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md). Then read [DSE](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md) or [directed interactions](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md) for their recipient boundaries.
- **How a service and callback cross the 2025 process endpoint:** [process request/callback flow](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md) separates private request correlation, pushed frames, explicit receive polling, callback-model dispatch, and HLA logical time.
- **What is coordinated or restored across save, join, and resign:** [federation lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md) → [synchronization points](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md) → [save/restore](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md) → [ownership persistence and resignation](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md) → [joined-federate MOM lifecycle](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md); add [TSO retraction](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md) when in-flight timestamped work is involved.
- **How a failure becomes a report or callback:** [exception reporting](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md) → [service invocation reporting](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md) → [callback/service ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md).
- **Model declarations, identity, and encoded values:** [FOM module admission/composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md) covers model assembly; [names and handle resolution](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md) covers typed lookup, active federate names, and known-object scope; [2025 DataElement encoding](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) and its [separate 2010 companion](HLA-2010-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) cover edition-specific public C++ value bytes. Keep model representation, API encoding, and RTI transport framing distinct.
- **Bounded 2010 reference behavior:** read the [separate 2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md) on its own. The 2025 paths above are not evidence of 2010 behavior.

## Local Mermaid rendering audit — 2026-10-09

The current worktree inventory is 160 Mermaid diagrams across 32 diagram-bearing
guide files (30 2025 guides and 2 separate 2010 guides): 79 flowcharts, 57
sequence diagrams, and 24 state diagrams. The prior 102
diagrams were rendered through the Mermaid Markdown renderer bundled with the
installed VS Code build and produced SVGs with zero renderer error nodes. The
three ownership-persistence/resignation diagrams were separately rendered
through that same bundle in a loopback-only Edge preview; all three produced
SVGs with zero diagram errors and were visually inspected. One semicolon in a
sequence-note label was replaced after the first parse attempt, then all three
were rendered and inspected again. The added object-removal invalidation
flowchart and revised deletion sequence were rendered through the same bundle
in a loopback-only Edge preview; both produced SVGs with no diagram errors and
were visually inspected. The accepted-send,
interaction-relevance/projection, and three attribute-update validation,
projection, and callback-boundary flowcharts all parsed and were visually
inspected in local Chrome. The three update-rate charts and four
attribute-value-request charts also parsed and were visually inspected. The
request-scope planning view was split into scope expansion and per-group
filtering after the first preview exposed intertwined loops; both views are
now separately readable. The request-response sequence's logical-time queue
note was revised to avoid implying an internal self-call. The
handle-normalization support-service flowchart, the
automatic-resign directive lifecycle sequence, and all three subscription-state
diagrams were rendered with the Mermaid bundle shipped in VS Code and
individually screenshot-reviewed in local Chrome. The passive/active gate
sequence needed two semicolon-to-comma repairs. Visual review also caught
registry self-call labels crossing the automatic-resign chart's lifeline; the
chart now distinguishes its registry-owned per-member directive field from
external service calls. All five final charts parse; normalization and regional
subscription are tall but readable when scrolled. The six remaining charts—
federate-local object forget, connection-loss regional TSO, FQR EVOKED delivery
frontier, directed-interaction unpublish, directed queued-callback fence, and
federation member-list snapshot—now all parse and have been visually reviewed
in local Chrome. The regional sequence's semicolon parse failure was repaired;
registry/queue self-loops were replaced with boundary notes; and the member-
list sequence now places EVOKED invocation after `evokeCallback` and IMMEDIATE
invocation inside submission. The object-forget and unpublish flowcharts are
tall but remain readable while scrolling. The no-time regional object-
attribute update sequence was also rendered and visually inspected locally
after two note-label punctuation repairs; it is tall but readable when
scrolled, with no text or lifeline collisions. These are local renderer
findings only, not GitHub verification.
The five diagrams in the 2025 federation-synchronization guide were parsed by
the VS Code Mermaid bundle and individually screenshot-reviewed in local
Chrome. The registration chart is tall but readable while scrolling; the
late-join chart separates expandable from fixed participant sets; and the
achievement state and sequence views make resignation completion, false-result
fan-in, and callback dispatch legible. The persistence sequence traces the
saved achievement ledger but labels multi-member partial progress as
source-derived: public API tests restore one-member unachieved points, while the
codec test only round-trips a false result. Late join after a partial report
also remains source-derived because the focused late-join test joins before any
achievement. GitHub verification remains pending publication.
The callback and service-ordering guide's new FederationSynchronized flow was
also parsed and screenshot-reviewed locally. It connects registry completion
and point removal to per-recipient report logging and callback submission,
including immediate-drainer contention and the HLA_EVOKED evoke gate. Its
service-specific IMMEDIATE-before-return ordering remains source-derived, not
asserted by the public synchronization-point test. GitHub verification is
pending publication.

The prior
102-diagram audit
includes the four federation-scoped MOM diagrams; its state chart was
shortened after an initial preview showed overlapping terminal/self-loop
labels. The earlier
review also includes the six valid tilde-fenced diagrams in the Auto Provide
and Attribute Scope guides (two flowcharts, two sequence diagrams, and two
state diagrams), which were missed by the original backtick-only scan and
were then individually inspected. That review found a clipped note in the
Auto Provide discovery sequence; an explicit line break fixed it, and the
updated chart was rendered and checked again. This is local renderer evidence,
not GitHub renderer or published-page verification.

The 22 previously queued local-only or revised diagrams were opened
individually and visually inspected. The added object-removal invalidation
flowchart, revised deletion sequence, no-time regional update sequence, and the
five-chart handle-normalization/automatic-resign/subscription batch brought
that reviewed set to 30. The five object/interaction charts and seven charts
from the update-rate/request batch (including the additional request-scope
view) have since been rendered and visually inspected, bringing the reviewed
local-only/revised set to 42 diagrams. The six formerly pending charts were
then rendered and inspected, bringing that set to 48; no chart in the current
local-review backlog remains pending. Two additional 2025 ownership-query
diagrams now cover classification/result grouping and the callback-time
revalidation fence. Both parsed in the installed VS Code Mermaid core and were
individually screenshot-reviewed in local Chrome; the tall classification
flow is readable while scrolling. The reviewed local-only/revised set is now
50 diagrams. Two more diagrams in the 2025 timing guide show asynchronous
receive-order admission and a test-shaped EVOKED/IMMEDIATE callback trace
across enable, disable, and pending-advance gates. Both parsed and were
individually screenshot-reviewed in local Chrome. That brings the reviewed
local-only/revised set to 52 diagrams. Two further save/restore status-query
sequences were rendered and individually inspected in local Chrome. They
distinguish read-only callback snapshots from operation completion and are
bounded to the named embedded `HLA_IMMEDIATE` tests; this brings the reviewed
local-only/revised set to 54 diagrams. These local checks do not establish
GitHub renderer parity. A 2010-only sequence in the separate reference-RTI
guide now shows that its generated shell accepts but does not load FOM paths,
then lazily allocates class handles through the reference provider directory.
It was rendered and screenshot-reviewed locally against the 2010 shell/source
and binding-smoke evidence, bringing the reviewed local-only/revised set to 55.
It does not imply 2025 behavior or normative 2010 FOM semantics.
A separate embedded 2025 ownership flowchart now shows accepted object
deletion invalidating queued query, regular owner-release, If-Available, and
cancellation work while retaining an independently gated receiver-removal
callback. It rendered and was screenshot-reviewed locally; the four cited
tests cover those callback families separately and do not establish global
ordering or process parity, bringing the reviewed local-only/revised set to 56.
Targeted rechecks confirmed the repaired
ownership and synchronization labels/sequences and the revised TSO
recipient-state layout. The time-grant chart remains tall, but its decisions
and outcomes are legible in the expanded preview. Tall charts still require
scrolling and are called out in the backlog where relevant.

The 2025 service-invocation guide adds an embedded switch/report failure
sequence: the Exception Reporting switch commits before its selected file
append, an injected append failure reaches the caller as `RTIinternalError`,
and a getter still observes the committed switch. The source-derived MOM
projection enqueue after the append is skipped on this path; its state is not
asserted by the focused test. The diagram parsed in the local VS Code Mermaid
renderer and was screenshot-reviewed, bringing the reviewed local-only/revised
set to 57. A sixth service-reporting diagram now traces timestamped attribute
update TSO admission before file-report persistence. Its source-derived failure
branch has no TSO rollback and returns no retraction handle; the existing tests
cover the normal TSO/report route and writer failure on a different service,
not that combined schedule. It parsed and was screenshot-reviewed locally,
bringing the reviewed local-only/revised set to 58. Neither diagram establishes
process-endpoint or 2010 behavior.

An earlier isolated headless check parsed 72 flowchart and sequence blocks
with identity text sanitization. It is historical and is superseded for local
syntax/render coverage by the complete 98-chart browser pass; neither check validates
GitHub's renderer, sanitization behavior, or published-page layout. Recheck
locally changed diagrams in GitHub after publication, and do not claim GitHub
parity from this preview. The lifecycle sequence's participant ID was changed
from `Link` to `Transport` because the installed renderer rejected `Link` while
GitHub rendered it; the visible participant label is unchanged, so this is a
local renderer-compatibility adjustment, not evidence of a GitHub defect.

## Normative source access — 2026-10-09

The [official IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
identifies the standard and offers purchase/subscription access, but its public
metadata does not expose clause 4.4. Keep the lifecycle cutoff flow explicitly
scoped to Umbra's implementation until the exact normative text can be checked
in an authorized copy; do not infer the clause from implementation comments.

## Prioritized guide backlog

| Order | Area | Purpose and likely diagrams | Status |
| --- | --- | --- | --- |
| Baseline | Time management | Role enable/disable, advance requests, grant bounds, TSO queue delivery, callback ordering, and the receive-order asynchronous-delivery gate. | Four of five baseline diagrams were individually inspected on GitHub (2026-10-09): the two independent role charts are clear, though the constrained-state label sits under the preview controls; the grant-decision chart is very tall with converging/crossing branches; and the TSO queue-phase chart is readable. The advance-lifecycle sequence fails baseline parsing where semicolon prose runs into the next message; the local comma repair is present. The grant chart is simplified locally into mode/role/GALT gates, leaving exact boundary cases in the adjacent matrix. The additional source/test-bounded FQR EVOKED delivery-frontier sequence now parses and has been visually inspected locally; clause 8.12 interpretation remains unverified. The receive-order asynchronous-delivery gate flow and focused EVOKED/IMMEDIATE sequence now parse and were screenshot-reviewed locally; they distinguish callback admission from dispatcher policy and keep TSO/grant rules separate. A new 2025 embedded Modify Lookahead flow distinguishes immediate increases from deferred decreases, partial lookahead reduction at successive grants, and retry-safe grant preflight. Its focused internal state test passed locally (18 assertions); the process public-API test encodes before/after grant queries but failed at initial RTI connection in this environment, before lookahead assertions. Render and visually inspect this chart before marking it reviewed. GitHub verification of local revisions remains pending |
| 1 | Attribute ownership | Per-attribute regular and If-Available acquisition, denial, negotiated/unconditional divestiture, assumption offers, callback-time state changes, cancellation/staleness edges, query-only ownership classification, and deletion-driven invalidation of pending ownership callbacks. | All three baseline parse-clean diagrams were opened and visually reviewed on GitHub (2026-10-09); the negotiated-divestiture sequence is legible, the unconditional-divestiture flow is tall but readable while scrolling, and the requester-state chart has stray states from a semicolon label. The other two acquisition sequences failed baseline parsing; local comma repairs are in place. The 2025 embedded query classification/revalidation diagrams and the accepted-delete invalidation flow were added from source and focused-test review, rendered, and visually reviewed locally. The tall classification flow remains readable while scrolling. The deletion chart is limited to selected embedded HLA_EVOKED query, regular owner-release, If-Available, and cancellation tests; If-Wanted/negotiated/Confirm Divestiture deletion remains source-observed only. It does not claim global callback ordering or process parity. Verify all local ownership revisions in GitHub after publication |
| 2 | DDM and regions | Region create/commit/delete, association and subscription lifetimes, overlap/relevance evaluation, and update/interaction routing. | The five baseline Mermaid diagrams were GitHub-rendered and visually reviewed (2026-10-09); three tall decision charts require scrolling in the expanded preview. The sixth no-time regional attribute-update sequence and seventh connection-loss regional-TSO sequence are source/test documented, locally parsed, and visually inspected; both are readable when scrolled. Verify changed charts in GitHub after publication. The rebuilt local MinGW case passed 1/1, while a separate privileged-MinGW duplicate-reflection report remains unresolved |
| 3 | Save and restore | Federate save coordination, callback gates, save completion/failure, restore selection/rejoin, restoration of deferred runtime work, and separate read-only status-query snapshots. | All four baseline diagrams were GitHub-rendered and visually reviewed (2026-10-09). The revised restore sequence rendered and was visually reviewed in the local VS Code Mermaid renderer. Two status-query sequences were rendered and inspected locally, bounded to embedded `HLA_IMMEDIATE` test evidence; they do not claim process/embedded parity or all status transitions. Verify GitHub after publication |
| 4 | Object and interaction information flow | Publication/subscription, registration/discovery, update/request, interaction delivery, and removal/resignation effects. | The four baseline Mermaid diagrams were GitHub-rendered and visually reviewed (2026-10-09). The corrected deletion sequence and fifth 2025 removal-invalidation flowchart were rendered and visually inspected locally. The sixth accepted-send/report/counter, seventh interaction-relevance/projection, and eighth through tenth attribute-update validation, receiver-projection, and callback-boundary flowcharts now also parse and have been visually inspected locally. The federate-local object-forget flowchart also parses and is visually reviewed; it is tall but readable when scrolled. The new twelfth regional active/passive transition sequence is source/test-shaped; its focused test currently fails during federation creation before reaching the transition assertions. Its actual Markdown rendering and visual review remain pending. After publication, verify the revised removal diagrams and new flowcharts in GitHub |
| 5 | Federation and federate lifecycle | Create/join/resign/destroy, connection loss, final-member behavior, member-scoped automatic resign directives, member-list snapshots, and cleanup of member-owned state. | All five baseline diagrams were inspected in GitHub's renderer (2026-10-09). Two execution-state self-loops were consolidated locally. The sixth, source-reviewed 2025 diagram on the lost-regulator TSO cutoff rendered and was visually reviewed locally; verify GitHub after publication. The seventh diagram tracing FDD-default seeding, per-member override, and connection-loss behavior now parses and has been screenshot-reviewed locally; the chart makes the registry-owned per-member directive field explicit to avoid self-loop label collisions. Verify GitHub after publication. The eighth source/test-bounded member-list snapshot sequence now parses and is visually reviewed locally; its callback branches show EVOKED-after-evoke versus IMMEDIATE-inline submission. Direct IMMEDIATE coverage for this member-list service remains a test gap. Verify exact IEEE 1516.1-2025 clause 4.4 wording against the full standard before treating implementation flows as normative guidance |
| 6 | Callback and service ordering | Queueing, callback models, service re-entry boundaries, ordering guarantees, and interactions with time-managed delivery not already covered elsewhere. | The four baseline diagrams were GitHub-rendered and visually reviewed (2026-10-09). The fifth, source-derived FederationSynchronized flow links barrier erasure and per-recipient report/callback submission to immediate-drainer and evoke behavior; it parsed and was screenshot-reviewed locally. The sixth chart traces escaping task exceptions, consumed-task/no-retry behavior, session in-flight cleanup, and remaining FIFO work. The seventh is a separate 2025 process-profile Resign trace: the branch cancels exception-report projection generations but leaves the general queue/session open. Source shows pending EVOKED tasks can reach the dispatcher through Evoke (no joined-state gate), while disabled IMMEDIATE backlog can drain through Enable Callbacks; task-specific staleness guards remain event-dependent. The focused process tests cited do not assert unrelated queued-task behavior after Resign, and normative post-Resign service legality is not claimed. These three added charts were parsed and screenshot-reviewed locally. The new eighth 2025 ConnectionLost producer flow distinguishes process-local cleanup (no shared-dispatcher reset in that handler) from embedded forced resignation (queue reset before local callback), while showing both producer catch-all exception boundaries. It is source-derived: the cited embedded service-report test failed during federation creation before the callback assertions, and no cited test throws from ConnectionLost. Do not infer profile parity. The new flow still needs actual Markdown rendering and visual review. Focused dispatcher tests lack a generic throwing-task case. Public synchronization-point tests cover both callback models but do not assert exact IMMEDIATE-before-return timing; the selected-file report test covers its HLA_EVOKED resignation scenario. Verify the three locally reviewed callback diagrams in GitHub after publication |
| 7 | Federation synchronization points | Registration and participant selection, omitted versus explicit sets, late joins, achievement fan-in, resignation completion, callbacks, and in-flight save/restore state. | Five current diagrams were parsed with the VS Code Mermaid bundle and individually screenshot-reviewed in local Chrome. Registration is tall but readable while scrolling; the late-join flow distinguishes expandable from fixed sets; the achievement state chart and sequence clarify resignation completion, false-result fan-in, and callback dispatch. The persistence sequence traces the saved achievement ledger; multi-member partial restore remains source-derived because public API tests restore one-member unachieved points and the codec test only round-trips a false result. Late-join expansion after partial reports is likewise source-derived because the focused late-join test joins before any achievement. GitHub verification is pending publication |
| 8 | Service invocation reporting | Per-member reporting/subscription interlock, interaction-vs-file routing, DDM recipient selection, serial reservation, callback ordering, and report-file lifetime. | All four baseline diagrams were GitHub-rendered and individually visually reviewed (2026-10-09); the interlock chart is wide with long exception names near the edge, the recipient-routing chart is very tall but readable when scrolled, the callback-order sequence is clear, and the file-lifetime chart has crowded self-loop labels but remains legible. The fifth embedded sequence traces a committed switch followed by a failed report append and returned `RTIinternalError`; its test body asserts the switch remains enabled, but the current local run failed during federation creation before reaching those assertions. Source shows the later MOM projection enqueue is skipped and the best-effort exception hook consults the already-committed switch; any `HLAreportException` still depends on eligible recipients and is not asserted by the test. The sixth embedded sequence traces timestamped Update Attribute Values TSO admission before file-report persistence; its no-rollback/no-returned-handle failure branch is source-derived and has no combined focused test. A seventh, separate embedded ConnectionLost sequence now traces final file-serial reservation before member erasure, deferred append failure, continued cleanup/callback submission, and the later error raise. Its failure branch is source-derived; the focused success-path test failed during federation creation before assertions. The latest chart has not been preview-rendered in this environment; the separate producer-cleanup chart also remains pending (see row 6). Verify the locally changed diagrams in GitHub after publication |
| 9 | FOM module admission and composition | Per-module DIF validation, ordered Annex C merge, whole-model reference resolution, FDD materialization, and atomic Create/Join commit boundaries. | Baseline's three diagrams were GitHub-rendered and visually inspected (2026-10-09); Create/Composition were refactored locally around preparation/commit boundaries and need GitHub rerender |
| 10 | Joined-federate MOM object lifecycle | Private RTI snapshot versus public discovery/reflection, Join-scoped values, save/restore `HLAfederateState`, and pending resign-removal lifetime. | Three baseline parse-clean diagrams were opened and visually reviewed on GitHub (2026-10-09); the Join flow is tall but readable while scrolling, the `HLAfederateState` chart is compact and clear, and the resignation chart has stray states from a semicolon label. The subscription/discovery sequence failed baseline parsing where a semicolon separated messages. Local comma corrections are in place; GitHub rerender pending |
| 11 | Exception reporting | Separate `HLAreportException`, `HLAreportMOMexception`, and `HLAreportServiceInvocation`; switch gates, subscriber/DDM selection, process rechecks, payload projection, and caller-visible failure. | Two baseline parse-clean diagrams were GitHub-rendered and visually reviewed (2026-10-09); the tall decision map and MOM rejection flow are readable when scrolled. The locally repaired service-exception sequence rendered and was visually reviewed in the installed VS Code Mermaid renderer; verify GitHub after publication |
| 12 | TSO message retraction | Producer legality boundary, per-recipient pending/delivered/suppressed states, Request Retraction callbacks, mixed fan-out, and timestamped-delete restoration. | Two baseline parse-clean diagrams were individually inspected on GitHub (2026-10-09): the timestamped-delete flow is tall but readable while scrolling; the per-recipient ledger is too wide for legible labels in the default preview. The producer legality flow fails on an unquoted `Retract(handle)` label and the mixed-recipient sequence fails where semicolon prose merges with the next message; local syntax repairs are present. The per-recipient state chart is now top-down with shorter edge labels locally. All three edits await GitHub rerender |
| 13 | Object-instance name reservation | Single and bulk reservation callbacks, partial reserve versus atomic release, owner-only named registration, reservation consumption, resignation, and restored reservation state. | One parse-clean reservation-lifecycle diagram was GitHub-rendered and visually reviewed (2026-10-09); the other four baseline diagrams failed parsing, with local syntax repairs pending GitHub rerender |
| 14 | Order and transportation controls | Keep RO/TSO order separate from transport type; trace class defaults, per-instance overrides, callback-gated transportation changes, queries, and what receivers observe. | All four baseline Mermaid diagrams were opened and visually inspected on GitHub (2026-10-09); the order, delivery, and query flows are legible. The transportation pending-state chart rendered a semicolon-separated transition label as stray nodes; the source label is corrected locally and awaits GitHub rerender |
| 15 | Declaration and attribute relevance advisories | Distinguish class-level publisher notifications from per-instance owner notifications; explain active/passive subscriptions, switches, class hierarchy, regional boundaries, known-object state, and update-rate reissues. | All four baseline parse-clean diagrams were individually opened and visually reviewed on GitHub (2026-10-09): the combined subscription-flow chart is balanced, the declaration-relevance state chart is compact, and the switch-off decision chart is tall but readable while scrolling. The per-instance state chart has crowded self-loop labels; its redundant no-change/no-advisory self-loop is removed locally, retaining the real update-rate reissue edge. GitHub rerender is pending |
| 16 | Attribute Scope Advisory | Model receiver-local per-object-attribute scope, region and subscription transitions, the self-final-unsubscribe exception, receiver switch, grouping, and stale callback rechecks; keep it separate from owner relevance and data delivery. | Two baseline parse-clean diagrams were individually opened and visually reviewed on GitHub (2026-10-09): the state chart is clear and the mutation-to-callback sequence is wide but legible in the expanded preview. The decision flow fails baseline parsing at unquoted callback-call labels; quoted labels are already present in the local guide and await GitHub rerender |
| 17 | Auto Provide | Trace the federation-wide FDD/MOM switch, discovery-triggered provider grouping, mandatory empty tag, regional eligibility, provider response separation, and callback-time switch/scope fences; call out the unverified process automatic path. | All three baseline parse-clean diagrams were opened and visually reviewed on GitHub (2026-10-09). The switch chart has stray states from a semicolon transition label; the discovery sequence is readable but wide, with the provider actor near the preview edge; the eligibility flow is tall but legible while scrolling. The transition label now uses a comma and sequence actor names are shorter locally; GitHub rerender is pending |
| 18 | Directed interactions | Separate sender, target object, and receiver identities; explain object-class/interaction-class publication, universal versus by-ownership selection, receiver fan-out, callback-time rechecks, and the TSO/retraction boundary. | Two baseline diagrams were GitHub-rendered and visually reviewed (2026-10-09); the declaration selector and tall timestamped-routing flow are legible when expanded/scrolled. The locally repaired receive-order sequence rendered and was visually reviewed; verify GitHub after publication. The implementation-scoped unpublish-state and queued-callback-fence diagrams now both parse and are visually reviewed locally; the former is tall but clear when scrolled. Verify GitHub after publication |
| 19 | Delay Subscription Evaluation | Explain the federation-wide static switch, route-only recipient retention, actual RO callback/TSO-grant eligibility checks, late subscription versus stale unsubscribe, regional predicates, and callback-model boundaries. | All three diagrams rendered and were individually visually reviewed in the installed VS Code Mermaid renderer. The recipient-state chart is tall but readable when expanded/scrolled. Verify GitHub after publication |
| 20 | DataElement encoding | Trace explicit scalar byte order, composite counts and alignment, discriminant-selected variants, cursor-based decoding, and the logical-time factory boundary; distinguish public value encoding from transport framing and keep edition-specific codec layers separate. | The 2025 guide's six diagrams and the separate 2010 companion's two diagrams were rendered and individually visually reviewed in the installed VS Code Mermaid renderer. Both guides have their own source/test evidence; sharing the unsigned-octet primitive is not a parity claim. Verify GitHub after publication |
| 21 | Names and handle resolution | Explain distinct typed handle categories, model-name and active federate-name lookups, inherited attribute/parameter resolution, per-federate object-instance knowledge, handle encode/decode versus hashes, and 2025 support-service normalization versus execution-scoped coordinates. | All six diagrams, including the source- and focused-test-backed 2025 normalization flowchart, were rendered and visually reviewed in the installed VS Code Mermaid renderer. The post-resignation identity asymmetry remains labeled as one process-test observation; no cross-edition behavior or general process/embedded parity is inferred |
| 22 | Connect, Disconnect, and configuration | Separate factory object construction from Connect-time RID authorization-profile selection, ambassador connection from federation membership, and embedded versus process address selection; show overload/precondition/auth gates, optional-setting outcomes, result reporting, and post-resign teardown. | The three established diagrams were rendered and visually reviewed in the installed VS Code Mermaid renderer. A fourth factory/RID sequence is now source-grounded, but the factory-created RID test last failed while creating its temporary file before assertions; render the new sequence and rerun the focused case when the fixture issue is understood. The absent-connection `NotConnected` behavior remains flagged against the pinned API comment for normative review. Verify GitHub after publication |
| 23 | Federation-scoped MOM | Distinguish the execution-scoped `HLAmanager.HLAfederation` object and its conditional values from each member's `HLAfederate` object; separate object reflections from federation request/report interactions, switch scopes, and mark the embedded 2025 test boundary. | The four existing diagrams rendered through the VS Code Mermaid bundle and were visually inspected in a loopback Edge preview; the first state-chart layout was shortened after review. A fifth diagram contrasts Create-time federation-wide `Advisories Use Known Class` with Join-time per-member advisory seeding, based on registry source and mapped to two focused test bodies. Both test executables built, but the tests currently fail at `createFederationExecution` with `Unknown exception` before assertions; the chart is therefore source-derived, not runtime-verified. Render and review it, investigate the test setup failure, and do not infer general switch mutability, backend parity, or 2010 behavior. Verify changed diagrams in GitHub after publication |
| 24 | Ownership persistence and resignation | Explain the boundary between durable pending-ownership ledgers and live callback routes, the reject-versus-cancel resignation fork, and how one resigned candidate is pruned while a surviving Confirm Divestiture recipient remains restorable. | Three diagrams rendered through the VS Code-bundled Mermaid renderer in a loopback Edge preview and visually inspected. Claims are limited to selected embedded 2025 development-profile tests; verify GitHub after publication |
| 25 | Attribute update-rate admission | Resolve active ordinary/regional designators per receiver and attribute, then show best-effort wall-clock admission at receive-order and TSO callback boundaries without merging the logical-time state machine. | All three 2025-only diagrams parsed and were visually inspected locally; the admission chart is tall but readable when scrolled. Process-profile parity and full normative rate semantics are not claimed |
| 26 | Attribute-value update requests | Distinguish object-instance, class expansion, regional routing, provider callback versus later value response, and the different pending-request/RO-delivered/TSO-queued restore cut points. | Four 2025-only diagrams based on API, source, and focused tests parsed and were visually inspected locally. The scope-planning view was split into expansion and per-group filtering to untangle routing loops. Process/embedded parity and exhaustive overload coverage are not claimed |
| 27 | Object-attribute subscription state | Explain per-attribute additive ordinary declarations, active/passive coexistence, discovery versus value eligibility, and separate regional pair state, including the source-inferred empty-region/update-rate generation boundary. | The three embedded diagrams were previously parsed and visually inspected locally. The regional subscribe/unsubscribe chart has since been revised to distinguish an empty pair map from nonempty pairs with empty region sets; its updated render and screenshot review are pending. Source plus a separate generation-key unit test imply fresh admission history, but no public integration test asserts a reset across this exact call. Process parity and normative meaning are not claimed |
| 28 | 2025 authorization and credentials | Trace authorizer construction/profile selection, Connect result mapping, retained credentials, and Create/Create-With-MIM/Destroy/Join gates; distinguish unnamed Join identity and the no-authorizer path. | New 2025-only guide with a Connect load/authorize/commit sequence and a federation-operation authorization flow. The selected CTest run passed 7/10: credential-format, reference-authorizer, Connect, disabled-auth, and Create cases passed; RID fixture creation failed, and Destroy/Join setup failed before target assertions. Connect API `@throws` lists differ by overload and need full §4.2 verification. The diagrams are not preview-rendered; the process operation gate is source-observed, with no process authorization integration test. No 2010 equivalence is claimed |
| 29 | Process endpoint request and callback flow | Explain private request/response correlation, pushed event frames versus explicit receive polling, and the lock-release/callback-model boundary without conflating transport request identity with HLA time. | New 2025-only guide with a service/recipient sequence and client-demultiplex/callback flowchart. Source distinguishes sender and recipient sessions and push/poll modes. Test sources cover framing, a private pushed callback across processes, and public configured-endpoint receive paths; all six selected local CTests failed before the relevant callback assertions: public cases stopped at Connect, localhost sockets were denied, and the subprocess helper failed. Both new diagrams still need Markdown render and visual review. No 2010 or normative wire-protocol claim is made |
| Separate 2010 profile | Bounded reference RTI service slice | Federation membership, discovery/update/interaction, synchronization barrier, provider-directory handle lookup, callback dispatch/failure boundaries, and explicit unsupported-flow map for the 2010 shell only. | Three of four baseline diagrams parsed and were individually visually reviewed on GitHub (2026-10-09). The membership chart is broad but zoom-readable; the interaction-routing chart was too wide at default preview scale and is now top-down locally; the synchronization barrier is tall but legible while scrolling. The discovery/update sequence failed baseline parsing at semicolon-separated message text; its local comma repair and the interaction layout revision await GitHub rerender. The fifth source-reviewed 2010-only chart distinguishes value/factory smoke coverage from unavailable ambassador time services. The sixth source/test-backed 2010-only sequence shows accepted-but-unloaded FOM paths and lazy provider-directory class handles. A seventh source-only sequence makes the synchronization completion-callback failure/retry edge visible; no focused 2010 barrier test was found or run. The eighth, source-derived chart explains inline callback dispatch, service-local exception conversion, and the unavailable evoke path; callback-error and HLA_EVOKED tests are absent. The separate 2010 DataElement guide adds two source/test-backed, locally reviewed diagrams. These guides make no 2010 time-advancement or standard FOM-loading claim. Keep both 2010 guides and their evidence separate from 2025 behavior |

This order is a working prioritization, not a statement that one area is more
normative or more complete than another. At the outset of this documentation
effort, a bounded 2025 survey found dedicated runtime and focused test families
for ownership, DDM/regions, save/restore, information flow, and federation
membership, but few cohesive contributor flow guides beyond timing. A focused
synchronization-point survey then identified
participant-set expansion, achievement fan-in, resignation, callback, and
state-image boundaries suitable for their own 2025 guide. A bounded
service-report survey found an existing implementation dossier and focused
tests for the report-subscription interlock, file sink, interaction sink, and
callback order, but no learner-facing flow guide; this guide narrows itself to
invocation reports rather than the whole MOM. The FOM-admission survey found
an extensive validation design and focused tests, but no compact flow guide
for the distinction between per-module DIF checks, complete-set composition,
FDD validation, and transactional Create/additional-Join behavior. The joined-
federate MOM survey found public-lifecycle and save/restore evidence for the
RTI-owned `HLAfederate` instance, but no learner-facing account of its private
snapshot, subscriber-visible callbacks, conditional state values, and
callback-time removal lifetime. A separate federation-scoped MOM survey found
the existing service-reporting design dossier and focused membership,
current-FDD/FOM-module, and synchronization-report tests, while the joined-
federate guide explicitly left the execution-scoped `HLAfederation` projection
out of scope. The new companion keeps its object/reflection lifecycle distinct
from federation request/report interactions and limits implementation claims
to the tested embedded development profile. The core ownership guide explicitly
left persistence and resignation out of scope, so a follow-up survey traced
typed ownership-image capture/restore, the pending-acquisition resignation
guard, and focused fresh-registry and cancellation tests. That evidence supports
a companion on selected pending release work, candidate pruning, and the
reject-versus-cancel fork; it does not imply every ownership operation is
restartable. An exception-report survey then found separate
focused routes and tests for `HLAreportException` and `HLAreportMOMexception`,
plus an existing design note documenting an inconsistency in the vendored 2025
MIM wording; the new guide makes the reporting-switch and subscription rules,
process callback rechecks, payload projection, and known source ambiguity
explicit without editing the Requirements Lab. The separate 2010 survey found a narrower but
real in-process reference service slice: the binding-shell smoke exercises
membership, object information, and interaction delivery, while synchronization
has source but no focused test in the surveyed family. The 2010 guide documents
only that slice and its known absences; it does not infer parity from the 2025
guides. A retraction survey found that the timing overview's queue diagram did
not fully explain the producer's strict time/lookahead boundary, recipient
ledger, or the distinction between withdrawing pending fan-out and requesting
reversal after delivery. Focused interaction, attribute, regional, process,
delete, and save/restore tests support a separate 2025 companion deep dive;
it links back to the overview instead of restating all time-management
behavior. Test-file-name searches are topic-discovery signals, not coverage
measurements. The object-instance-name survey then found a distinct state
machine not explained by the generic registration flow: single-name collisions
report asynchronously, multi-name reserve permits a successful subset while
multi-name release validates atomically, and named registration consumes only
the owner's reservation at commit. It also found focused embedded, process,
restore, and restart tests. A test that feeds a 2010 FOM to the 2025 API is
explicitly treated as 2025 behavior with legacy-model input, not evidence about
the separate 2010 reference RTI. The order/transportation survey then found a
separate control plane with synchronous order changes and callback-gated
transportation changes, class defaults captured by later registrations,
per-instance overrides, callback-visible order and transport metadata, and
selected pending-change restore coverage. Existing time and DDM guides explain
the adjacent grant and recipient-selection boundaries, so this guide focuses
on the control decisions rather than duplicating those state machines.

The next advisory survey found a distinct class-level declaration signal and
per-object-attribute relevance signal. A 2025 object subscription can feed both
planners, but callbacks differ in recipient, granularity, region criteria, and
switch behavior. The new guide separates them and links regional details back
to the DDM guide instead of repeating its overlap state machine.

The Attribute Scope Advisory survey found distinct embedded and public
process-endpoint test cases, both covering HLA_EVOKED and HLA_IMMEDIATE. The
new 2025-only guide gives scope its own receiver-centered state machine,
documents the implementation's self-final-unsubscribe exception and
delivery-time stale-work filtering, and links back to the broader DDM and
relevance guides instead of merging their state machines. Keep the 2010
profile separate when repairing or reviewing diagrams, and do not infer its
behavior from 2025. The direct-render status is recorded below.

The Auto Provide survey found a distinct discovery-triggered path beside
explicit Request Attribute Value Update: the 2025 embedded planner groups
currently owned in-scope attributes by provider, supplies an empty tag, and
rechecks switch and scope before provider code. Focused tests cover disabled
discovery-only behavior, regional/reentrant changes, multi-provider fan-out,
MOM switch mutation, and timestamped admission. The process profile exposes
explicit requests and switch mutation, but no automatic discovery planner or
focused process Auto Provide test was found in this bounded survey; the guide
marks that path unverified rather than claiming edition/profile parity.

The directed-interaction survey found a distinct object-context routing model:
the sender supplies a target instance, while the RTI fans out according to
object-class/interaction-class declarations and each receiver's universal or
by-ownership selector. Focused evidence includes embedded selector, known-
target, timestamped delayed-subscription and retraction tests, plus a process
multi-recipient endpoint case. The new guide keeps embedded callback-time
revalidation separate from the process evidence and links timestamp/grant and
retraction state back to their dedicated guides.

The DSE survey found one cross-cutting 2025 state machine across ordinary
interactions, attribute updates, regional traffic, and directed interactions.
The focused embedded tests assert late-subscribe admission only when enabled,
and callback/grant-time unsubscribe suppression in either switch mode. The
guide treats HLA_EVOKED/HLA_IMMEDIATE and TSO grants as separate dispatch
boundaries, notes that the switch is initialized and saved with federation
state, and marks process behavior as source-observed rather than test-proven.

A focused ordinary attribute-update survey found a separate receive-order
delivery boundary: the sender's attributes are validated against its registered
class and ownership, while each receiver's projection is filtered through its
own known class. One API update may form distinct transportation/order/region
passels; queued delivery re-evaluates the receiver for each passel, so an
unsubscribe can suppress it without merging the passels. The embedded test
directly exercises class promotion, reliable/best-effort callbacks, immediate
versus evoked delivery, validation failures, and unsubscribe suppression. The
guide keeps source-derived copy/re-plan, report/counter, and lock-release detail
distinct from that test evidence. The focused update-rate companion now traces
applicable active ordinary/regional designators, the per-attribute steady-clock
gate, and its receive-order/TSO delivery boundaries. It separates this filter
from owner relevance advisories and logical-time grant selection, and labels
the implementation/test scope as embedded 2025 evidence.

A follow-up subscription-state survey found that ordinary declarations
accumulate by selected attribute, active/passive mode affects discovery
separately from attribute projection, and regional attribute/region entries
occupy a distinct declaration map. The new guide bounds these claims to
embedded focused tests and flags the unresolved update-rate-key effect of a
regional subscribe call whose supplied region sets are all empty.

A follow-up interaction-subscription survey compared the passive, selector,
and regional focused tests against the existing object/interaction,
directed-interaction, relevance-advisory, DDM, and DSE guides. Those guides
already separate active class-level relevance from passive recipient matching,
selector choice, class/parameter projection, and regional/time gates. A new
standalone subscription guide would currently repeat those flows, so the next
survey moves back to RTI service families to find any genuinely uncovered
state machine.

A bounded API-heading survey then checked the seven 2025 `RTIambassador`
service groups—Federation, Declaration, Object, Ownership, Time, Data
Distribution Management, and RTI Support—plus callback controls against the
existing reader paths and guide backlog. Each top-level group already has
relevant guide coverage, so this scan did not justify another umbrella guide.
It was an inventory of service headings, not proof that every overload,
callback boundary, or state transition is fully explained; future additions
should come from a specific source- and test-backed gap inside a covered area.
The 2010 reference stream was not folded into this survey.

The DataElement survey found a bounded value-encoding pipeline relevant to
the original byte-order/serialization concerns: public 2025 scalar and
composite elements, explicit BE/LE selection, cursor-returning nested decode,
composite alignment, variant discriminants, and factory-defined logical time.
One generic byte-order primitive is genuinely shared, while edition-specific
element sources and 2010/2025 composite helpers remain distinct. The new guide
documents the 2025 path only and does not infer 2010 parity or transport-wire
behavior. Its next gate is the same rendered-diagram review as the existing
set. A later Connect/Disconnect survey found a separate local-session state
boundary beside Join/Resign: four public overloads converge on callback-model,
authorization, configuration, resource-preparation, and commit gates, while
Disconnect rejects joined members and releases callback/endpoint resources
after its state transition. Focused tests cover overloads, malformed settings,
credentials, duplicate Connect, callback model, and Disconnect guards. The new
2025-only companion flags a difference between Umbra's absent-connection
`NotConnected` result and the pinned API header's 4.3 exception annotation for
comparison with the full normative clause; it does not turn that observation
into a conformance claim.

A process-endpoint survey found a distinct implementation boundary not covered
by the HLA service-flow guides: one service request can share a session stream
with unsolicited event frames, while recipient events and sender responses use
different sessions. The new 2025-only companion diagrams trace request-ID
correlation, push-versus-poll arrival, event buffering, transaction-lock
release, callback-model dispatch, and the Evoke receive fallback. Source and
test cases exist at transport, private-client, and public-ambassador layers,
but the current local focused run was blocked before callback assertions or by
localhost socket restrictions. Keep the guide implementation-scoped; do not
infer 2010 behavior or reinterpret request IDs as HLA logical time.

## Completion check for each guide

The local VS Code installation is 1.141.0; its built-in Markdown preview
renders Mermaid fenced blocks (available since 1.121). Open a guide's preview
with `Ctrl+Shift+V`; see the [official Markdown preview documentation](https://code.visualstudio.com/docs/languages/markdown)
and [official Windows shortcut reference](https://code.visualstudio.com/shortcuts/keyboard-shortcuts-windows.pdf).
The current static inventory finds 160 Mermaid blocks across the 32
diagram-bearing guides (30 2025 guides and 2 separate 2010 guides),
with balanced fences. The prior comparable whole-set audit checked 877
file-targeting local Markdown links. Current documentation changes add further
file-targeting references since that audit; each added target and line anchor
was checked individually. Refresh
the comparable whole-set link count on the next link-audit pass. Static fence
and link checks are useful, but do not establish that
Mermaid parses or that a rendered diagram is readable. The current local audit
above records charts parsed and inspected since the earlier inventory; the six
charts that were pending local review are now verified. Since then, two
ownership-query diagrams, two asynchronous-delivery diagrams, two save/restore
status-query sequences, the separate 2010 provider-directory sequence, and
the embedded 2025 object-deletion/ownership-invalidation flowchart have all
been rendered and visually inspected, bringing the local-only/revised reviewed
set to 59. The 2025 federation-synchronization guide's five diagrams were
parsed and screenshot-reviewed locally, bringing that reviewed set to 64. The
partial-achievement late-join and multi-member partial-save/restore paths
remain explicitly source-derived rather than end-to-end test-backed. The new
FederationSynchronized dispatch flow in the callback guide adds one locally
reviewed diagram, bringing the set to 65; its exact service-specific
IMMEDIATE-before-return timing remains unasserted. The 2010 DataElement companion's two diagrams, the source-only 2010 synchronization callback-retry sequence, and the new source-only 2010 callback-dispatch/failure-boundary flow were rendered and screenshot-reviewed independently of the 2025 guides. The locally reviewed/revised count is now 66. No chart in that
render-review backlog remains pending.
After that 66-diagram checkpoint, the callback exception and process-Resign
charts were added, parsed, and screenshot-reviewed locally; the current
local-only/revised reviewed count is 68. The process-Resign chart was revised
after the membership-gate audit and re-rendered; the count remains 68. The
later Modify Lookahead, regional subscription-state, regional interaction
active/passive, callback/service-ordering ConnectionLost producer, embedded
ConnectionLost final-report, two authorization charts, and the new factory/RID
sequence, the federation/member switch-scope chart, and the two process-endpoint
charts (eleven in total) remain pending actual Markdown preview and visual
review; static checks alone do not establish renderability or readability.
GitHub verification of locally changed diagrams remains a separate check.

## GitHub Mermaid render audit — 2026-10-09

The GitHub preview audit covered the current remote branch's 20 tracked guides
and 79 Mermaid blocks. GitHub rendered 64 without a parse error; 15 blocks in
10 guides reported Mermaid parse errors. The four Save/Restore diagrams, all
four Callback/Service Ordering diagrams, all five DDM/Region diagrams, all
five federation/federate-lifecycle diagrams, all four Object/Interaction
Information Flow diagrams, all three FOM Admission/Composition diagrams, both
parse-clean Directed Interaction diagrams, both parse-clean Exception
Reporting diagrams, and the parse-clean reservation-lifecycle state diagram
were opened individually and visually inspected. All 64 successful parses have
received individual visual review. This total includes three individually
reviewed diagrams from the separate 2010 reference profile; its fourth diagram
failed baseline parsing and has a local comma repair pending GitHub rerender.
One lifecycle state diagram had crowded parallel
self-loop labels; their merged local label is pending GitHub re-render. Three DDM decision charts are
tall top-down flows; their branch labels remained readable while scrolling
the expanded preview, though they do not provide a one-screen overview. The
directed-interaction timestamped route and exception-report decision maps are
also tall portrait diagrams; their key branches remain legible while scrolling.
The receive-order directed-interaction sequence and service-exception sequence
both reproduce baseline parse failures. Their local repairs have since rendered
in the installed VS Code Mermaid renderer; GitHub rerender remains pending. The
object-name-reservation guide has four baseline parse failures and one
parse-clean state diagram; its local syntax repairs still need a GitHub
rerender. The seventeen diagrams in the local-only DSE, DataElement,
Names/Handles, and Connect/Configuration guides have since been rendered and
visually reviewed. This worktree has no standalone Mermaid CLI or package; the
loopback preview used the installed renderer assets.
The Attribute Ownership guide adds three individually inspected baseline
parse-clean diagrams: the negotiated-divestiture sequence is legible, the
unconditional-divestiture flow is tall but readable while scrolling, and the
requester-state chart has two stray states caused by a semicolon in a
transition label. The regular-acquisition and If-Available sequences both
failed baseline parsing where semicolon-delimited prose ran into the next
message or note. The three semicolon-to-comma source corrections are local and
await GitHub rerender.
The Federation Synchronization guide adds two individually inspected baseline
parse-clean diagrams: the participant-registration flow is very tall but
readable while scrolling, and the late-join state chart has stray states from
a semicolon transition label. Its achievement sequence failed baseline
parsing where semicolon-separated messages ran together. The local comma
corrections await GitHub rerender.
All four Service Invocation Reporting diagrams were also opened and visually
inspected on GitHub (2026-10-09). The report-service interlock is wide, with
long exception names close to the right edge; the recipient-routing flow is
very tall but its branches remain readable while scrolling. The callback-order
sequence is clear. The per-membership file-lifetime state chart has several
crowded transitions around its JoinedFile state, but the labels are legible in
the expanded preview.
The Joined-Federate MOM Object Lifecycle guide adds three individually
inspected baseline parse-clean diagrams: its Join flow is tall but readable
while scrolling, its `HLAfederateState` flow is compact and clear, and its
resignation chart has stray states from a semicolon transition label. The
subscription/discovery sequence failed baseline parsing where semicolon prose
ran into the next message. Local comma corrections are in place; GitHub
rerender is pending.
The Auto Provide guide adds three individually inspected baseline parse-clean
diagrams. Its switch state chart rendered a semicolon transition as stray
states; that separator is now a comma locally. The provider fan-out sequence
is readable but wide, so its receiver/provider actor labels are shortened in
the local source. The callback-eligibility chart is tall but its final stale-
work, callback, and no-response branches remain readable while scrolling.
Both local diagram edits await GitHub rerender.
The Attribute Scope Advisory guide adds two individually inspected baseline
parse-clean diagrams: its receiver/object/attribute state chart is clear, and
the mutation-to-callback sequence is wide but readable in the expanded preview.
The baseline decision flow failed to parse on its unquoted callback-call
labels; the local guide already quotes those labels and awaits GitHub rerender.
All four Declaration and Attribute Relevance diagrams were individually
inspected on GitHub. The combined subscription-flow chart is balanced, the
declaration state chart is compact, and the switch-off flow is tall but clear
while scrolling. The per-instance relevance chart crowded two parallel
self-loop labels; its no-change/no-advisory loop was not a meaningful state
transition and has been removed locally, preserving the update-rate reissue
transition. GitHub rerender is pending.
The TSO Retraction guide adds two individually inspected baseline parse-clean
diagrams. The per-recipient ledger renders extremely wide with small labels and
needs a layout pass; the timestamped-delete restoration flow is tall but
readable while scrolling. The producer legality flow fails baseline parsing
at an unquoted `Retract(handle)` label, and the mixed-recipient sequence fails
on semicolon-delimited message prose. Both repairs are already in the local
guide and await GitHub rerender.
The Time Management guide adds four individually inspected baseline
parse-clean diagrams: both role state charts are clear (the constrained-state
label sits beneath the preview controls), and the TSO queue-phase chart is
readable. Its grant-decision chart is very tall and has converging/crossing
branches; the local revision keeps the sequence of mode, role, and GALT gates
in the chart while leaving exact strict/equality and queue-bound cases to the
comparison matrix. The advance-lifecycle sequence failed baseline parsing at
a semicolon-delimited message; its local comma repair and the grant-chart
redesign both await GitHub rerender.
All four Order and Transportation diagrams were also individually inspected:
the attribute-order, delivery, and query flows are readable, while the
transportation pending-state chart rendered its semicolon-separated transition
label as stray state nodes. The label now uses a comma locally; GitHub rerender
is pending publication.
The baseline Create FOM chart was about 2,700 pixels tall and repeated the
composition chart's preparation details. The local revision now presents the
Create prepare/commit boundary and leaves module validation and merge gates to
the composition chart; those changed charts still need a GitHub rerender.

The baseline failures were traced to semicolons inside sequence-diagram
messages/notes and unquoted function-call labels in four flowcharts. Local
source edits now remove semicolon separators from every Mermaid sequence block
and quote the affected flowchart labels. A static sequence-block scan is clean.
These edits have not yet been checked in the live GitHub renderer, because the
remote branch still serves the prior source; treat the repairs as pending
GitHub verification. The four local-only 2025 guides are not included in the
GitHub baseline; their local preview pass is complete, but GitHub compatibility
still needs a published-branch pass.
The earlier bounded link audit checked 58 local links, including 44 line-number
anchors, across the original DSE and DataElement drafts with no missing targets
or out-of-range lines.
The later all-guide audit above also checked heading fragments; neither audit
establishes Mermaid parsing or visual readability.

All parse-clean diagrams in the current GitHub baseline have received an
individual visual review. On 2026-10-09, the seventeen local-only diagrams in
the DSE, DataElement, Names/Handles, and Connect/Configuration guides, the
lifecycle cutoff diagram, the separate 2010 time-support boundary, and the
revised restore, directed-interaction, and exception-report sequences (22
diagrams total) were individually rendered and visually inspected with the
installed VS Code Mermaid Markdown renderer in a loopback-only preview. Each
produced an SVG without a Mermaid error. The restore sequence's rejected-request
note was too long for its two-participant span; it was shortened and rerendered
successfully. Several flows are tall, but their labels and paths remain legible
when viewed at full size or scrolled; do not split a cohesive chart just to
reduce its height.

This local check is not a claim of GitHub-renderer parity. After the changed
guides are published, rerun GitHub's parse/render check and visually inspect
the local-only diagrams and source/layout revisions there. Keep the 2010
time-support boundary independent from all 2025 timing state machines; the
encoding guide's explicit shared-byte-order-primitive path is the only common
implementation primitive shown, not a claim of codec or behavior parity. The
source-complexity queue below remains a separate supplementary review.

For the source-complexity review pass, start with the largest Mermaid blocks:
[save request/commit sequence (40 lines, visually reviewed)](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#L81),
[exception-report routing (39 source lines; local preview reviewed, GitHub rerender pending)](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md#L57),
[restore sequence (36 source lines; local preview reviewed, GitHub rerender pending)](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#L178),
[directed-interaction fan-out (35 source lines; local preview reviewed, GitHub rerender pending)](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#L97).
This is only a review-order heuristic; line count does not establish diagram
quality or rendered correctness. Review the remaining diagrams afterward in
the reader paths above, and record completion only from direct preview evidence.

### Source-complexity review: save request and commit

Reviewed the 2025 save request/commit sequence and its companion state diagram.
Keep the lifecycle together: request admission and its optional time-boundary
handoff lead directly into participant checkpoint callbacks, the all-participant
completion barrier, durable state-image commit, and the final result callbacks.
The time-advance algorithm is linked to the separate timing guide, and restore
has its own sequence; neither needs to be pulled into this chart. Splitting this
sequence merely to shorten it would obscure the commit boundary rather than
clarify ownership.

The implementation path is
[`federation_registry_save_control.cpp`](../../cpp/src/internal/federation/federation_registry_save_control.cpp#L787):
only the last participant to complete enters image encoding and durable commit,
and the completion notifications are built from whether that commit succeeded.
Focused 2025 registry tests exercise the participant barrier and persisted
state-image round trip in
[`federation_registry_catch2.cpp`](../../cpp/tests/federation_registry_catch2.cpp#L51);
the filesystem commit-envelope and encoding-failure cases are at
[`federation_registry_catch2.cpp`](../../cpp/tests/federation_registry_catch2.cpp#L374)
and [line 426](../../cpp/tests/federation_registry_catch2.cpp#L426). These are
implementation/test observations, not a substitute for checking the normative
save protocol against the correct IEEE edition.

### Source-complexity review: restore admission and completion

Reviewed the 2025 restore sequence against
[`federation_registry_restore_control.cpp`](../../cpp/src/internal/federation/federation_registry_restore_control.cpp#L47)
and its completion barrier at
[line 430](../../cpp/src/internal/federation/federation_registry_restore_control.cpp#L430).
Keep admission, per-participant application restore, the all-member barrier,
RTI-state rehydration, and terminal callbacks together: they are one restore
protocol. The chart now makes the implementation's source choice explicit:
the current embedded registry uses its retained full snapshot when available,
and its bounded durable-image path after registry restart. This is an Umbra
implementation distinction, not a 2010/2025 equivalence claim.

The focused [2025 registry restore round trip](../../cpp/tests/federation_registry_catch2.cpp#L51)
checks restored state and the participant barrier; separate process-endpoint
success and early-failure tests are linked in the guide's evidence table. The
revised diagram was rendered and visually reviewed in the local VS Code
Mermaid preview; GitHub rerender remains pending publication.

### Source-complexity review: directed-interaction fan-out

Reviewed the 2025 receive-order fan-out against the embedded planner in
[`federation_registry.cpp`](../../cpp/src/internal/federation/federation_registry.cpp#L1769),
its DSE candidate fallback at
[line 1864](../../cpp/src/internal/federation/federation_registry.cpp#L1864),
the directed recipient predicate in
[`federation_registry_directed_interactions.cpp`](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L139),
and callback-time projection in
[`umbra_rti_ambassador_interaction_callbacks.cpp`](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_callbacks.cpp#L155).
The previous chart omitted a real path: when DSE is enabled, the embedded
planner can retain a joined non-source receiver with a live route but no current
selector projection. The revised chart shows that route-only candidate and the
callback-time recheck that can admit a candidate if the current recipient
predicate now passes or suppress it if it remains ineligible. The focused
[receive-order DSE test](../../cpp/tests/delay_subscription_evaluation_directed_interaction_catch2.cpp#L95)
exercises late subscription and unsubscribe-before-callback under both
`HLA_IMMEDIATE` and `HLA_EVOKED`.

Keep this as one send-to-callback flow. The route-only candidate and callback
recheck are two phases of the same recipient decision; the DSE guide retains
the cross-service switch/state model, and the timing and retraction guides retain
the TSO grant and per-recipient retraction machines. This is embedded 2025
behavior, not a 2010 parity claim. The revised diagram passed the local
VS Code Mermaid render and visual review; verify GitHub after publication.

### Source-complexity review: object-instance name reservation

Reviewed the five 2025 name-reservation diagrams against the public service
path in
[`umbra_rti_ambassador_object_instance_name_reservation.cpp`](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_name_reservation.cpp#L14),
the embedded transaction and registration-commit logic in
[`federation_registry_object_instance_registration.cpp`](../../cpp/src/internal/federation/federation_registry_object_instance_registration.cpp#L24),
and the resignation cleanup at
[`federation_registry_resign_lifecycle.cpp`](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L747).
The source and focused tests support distinct boundaries: one-name admission
returns before its outcome callback; bulk reservation commits an available
subset while bulk release validates the entire set before erasing; named
registration consumes the owner's reservation only after both object indexes
are committed; resignation and restore cover the longer-lived reservation
state.

Keep those charts in the single name-reservation guide. They explain different
service/state contracts within one coherent topic, rather than splitting a
single chart merely to satisfy a size target. The focused embedded reservation
and named-registration cases, process-endpoint case, and restore/restart tests
remain linked in that guide's evidence map. The guide describes the 2025 API
and implementation profile only; accepting a 2010 FOM in a 2025 test is not a
2010 RTI flow or a parity claim.

### Source-complexity review: Delay Subscription Evaluation

Reviewed the three DSE diagrams against the embedded receive-order planners in
[`federation_registry_receive_order_interactions.cpp`](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L16)
and
[`federation_registry_attribute_value_update_requests.cpp`](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L93),
the directed-interaction route-only candidate in
[`federation_registry.cpp`](../../cpp/src/internal/federation/federation_registry.cpp#L1848),
and the receive-order callback projection in
[`umbra_rti_ambassador_interaction_callbacks.cpp`](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_callbacks.cpp#L77)
and TSO delivery projection in
[`umbra_rti_ambassador_time_advance_dispatch.cpp`](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_advance_dispatch.cpp#L270).
The existing split is thematic: federation-switch lifetime, per-recipient
candidate state, and receive-order/TSO delivery boundaries answer separate
reader questions. Keep the sequence together because it compares two delivery
boundaries for one delayed-eligibility rule; dividing it would hide the
callback-versus-grant distinction.

The guide's focused cases are embedded/development-profile tests. Its process
receive-order path is source-observed but lacks a focused process DSE case in
the bounded test set. Keep that evidence caveat and the explicit 2025-only
boundary; do not infer 2010 behavior. Source review and local Mermaid rendering
are complete; verify GitHub-host rendering after publication.

### Source-complexity review: `HLAreportException` routing

Reviewed the 2025 report route against the
[`emitExceptionReport` hook](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_service_report_interaction.cpp#L184),
the [embedded report plan](../../cpp/src/internal/federation/federation_registry.cpp#L1661),
the [process-service report handler](../../cpp/src/internal/federation/process_federation_service_reporting.cpp#L89),
and its [callback-time recheck](../../cpp/src/internal/federation/process_federation_service_reporting.cpp#L403).
The previous diagram put “no process client” in a shared early-stop branch,
which incorrectly implied that the embedded backend also stops: the source
checks for a process client only when the process endpoint is active. The chart
now nests that guard under the process branch and leaves the embedded path
available without a process client. The focused [process exception-report
case](../../cpp/tests/ieee1516_2025_connection_process_mom_service_exception_report_catch2.cpp#L460)
exercises the report route while preserving the caller's original typed failure.

Keep this as one service-failure-to-advisory-report lifecycle. Embedded and
process routing are genuine backend branches of the same `HLAreportException`
path; `HLAreportMOMexception` stays in Route B, and service-invocation reports
stay in their separate guide. This remains 2025 behavior and does not imply a
2010 report path. The expanded 39-line sequence passed the local VS Code
Mermaid render and visual review; verify GitHub after publication.

### Combined process failure-report boundary

The 2025 exception-reporting guide now distinguishes the process
`GetObjectClassHandle` catch's two sequential reporting requests. Source calls
the `noexcept` `HLAreportException` hook before submitting the separate failed
`HLAreportServiceInvocation` request. Failure in the latter can become
`RTIinternalError` before the catch rethrows the original `NameNotFound`.
Focused tests cover each report channel separately, but none enables both
switches or asserts combined callback order/public outcome. The sequence is
explicitly source-derived; it does not claim callback-delivery ordering,
embedded-path behavior, or 2010 parity. The 34-line sequence parses with the
installed VS Code Mermaid sequence parser. Screenshot-based visual review is
still pending: the available computer-use surface exposed no native preview
window or browser tab, so do not mark this diagram visually reviewed until an
actual Markdown render can be inspected.

Before calling a topic guide-ready, verify that its diagrams render in GitHub's
Mermaid Markdown, all relative source/test links resolve, behavior statements
are traceable to the named implementation and focused tests, normative
statements are checked against the correct official edition, and the explicit
edition boundary remains intact. Record what is not modeled and leave the next
topic as a concrete handoff here and in
[ROADMAP-INDEX.json](../planning/ROADMAP-INDEX.json).
