# HLA Behavior Flow Guides: Review Ledger

This file preserves the dated visual-render audits, source-complexity reviews,
reader-gap surveys, and bounded evidence notes that support the active HLA
guide map. For current priorities and the next handoff, use the [HLA behavior-
flow guide index](HLA-BEHAVIOR-FLOW-GUIDES.md). The focused ordinary and
directed interaction class/parameter routing reviews are in the
[interaction-routing ledger](HLA-BEHAVIOR-FLOW-GUIDES-INTERACTION-ROUTING-REVIEW-LEDGER.md).
These records describe the evidence available when each review was made; they
do not upgrade source observations or focused tests into normative or
conformance claims.

## 2025 Delete-privilege transfer and callback identity review — 2026-10-16

Connected the ownership and object-removal guides at a concrete 2025 reader
seam. The current worktree contains an embedded `HLA_EVOKED` scenario in which
the original registrant transfers both an ordinary attribute and
`HLAprivilegeToDeleteObject`; the focused source asserts that only the new
privilege owner can delete and that the observer's removal callback retains
the original registrant as `producingFederate`, with the new deleter's tag.
A paired source scenario checks that acquisition fails with
`AttributeNotPublished` when the prospective owner has not published the
special attribute at the object's known class, then succeeds after publication.
These are checked-in test-source assertions, not test results from this review.

Added one 2025 sequence to the attribute-ownership guide and linked its test
and embedded registry evidence. The process timestamped-delete path builds
separate producer metadata; this chart makes no process-profile parity claim,
and process callback identity is the next evidence review. Mermaid 12.1.0
syntax parsing accepted the new sequence. No image was produced and visual
review remains pending because browser launch fails with Windows
`STATUS_ACCESS_DENIED` (`0xC0000022`). No build, test, CI, or Requirements Lab
work was performed.

## 2025 Process callback stream and Resign timing audit — 2026-10-14

The Resign review separates event frames buffered by the client, tasks already
in the shared callback dispatcher, and the public service return. Source shows
that the default request path drains frames available at its explicit dispatch
point before returning; enabled HLA_IMMEDIATE work may run within public
Resign, while evoked/disabled tasks may remain queued. Frames arriving after
that drain resume through the background dispatch loop after request
suppression lifts, but source does not establish their order against the
public return. Server-side queued session events are cleared; client membership
and exception-projection state are cleared later, and the shared dispatcher is
not reset by Resign. No HLA permission or post-Resign delivery test is claimed.

Corrected the process-endpoint guide's stale claim that there is no background
reader: source starts dedicated stream-reader and callback-dispatch threads.
Mermaid CLI 12.0.0 rendered and visual review passed for the updated receiver
stream sequence, event-dispatch flow, and callback/Resign flow. A semicolon in
a sequence message initially failed parsing; the final wording rendered
successfully. No tests/build/CI, Requirements Lab, or 2010 change. GitHub
rendering and the post-Resign behavior remain unverified.

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
[`federation_registry_directed_interactions.cpp`](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L140),
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

The 2026-10-14 reader-gap audit sharpened the process boundary: process
receive-order ordinary and directed sends re-resolve deferred candidates while
preparing outgoing events, whereas process ordinary and directed TSO paths
re-project at queued delivery. The callback bridge receives projected event
data; this source mapping does not establish a callback-time receive-order
recheck. No focused process DSE case was found. The sequence diagram's
embedded callback/grant scope is now stated beside it; Mermaid source was
unchanged, so the prior chart render evidence still applies to the same
diagram source.

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
embedded-path behavior, or 2010 parity. The 34-line sequence rendered in the
installed VS Code Mermaid feature bundle and was screenshot-reviewed in a local
loopback preview on 2026-10-10. This verifies rendering and layout only;
combined two-switch behavior and callback delivery order remain source-derived,
and 2010 parity is not claimed.

### Process TSO callback-acknowledgment boundary

Added a 2025 process-specific sequence for the seam not shown by the broader
time and retraction state machines: after an admitted recipient event is sent,
the process ledger remains `inTransit` through callback admission and
completion, then the client sends a private per-recipient ACK. The sequence
distinguishes default post-response dispatch, immediate versus evoked callback
execution, pre-start callback suppression, deferred ACK submission while a
client request is active, and coordinator-ledger completion. It does not
restate TSO eligibility/GALT/LITS or the broader retraction machine. A 2025
public fanout test source covers per-recipient in-transit bounds and ACK order;
a service-level attribute case checks an `applied` ACK; a separate public
retraction case checks suppression before callback admission and absence of
an ACK frame. These tests were not rerun in this documentation increment.
The new sequence was rendered and screenshot-reviewed in the installed VS Code
Mermaid feature bundle on 2026-10-10. Keep the explanation within the 2025
process endpoint guide, infer no 2010 or embedded parity, and leave the
Requirements Lab untouched.

### 2025 names and handle-resolution reader review — 2026-10-10

A bounded content review of the existing names/handle guide found its main
reader distinctions already explicit: model declarations resolve in a joined
execution; attribute and parameter lookup is class-scoped and follows declared
inheritance; object-instance lookup depends on each federate's knowledge after
discovery; federate-name and federate-handle lookup have distinct directions
and lifetimes; public handle categories are distinct types; and encoding a
handle is not the same as using a hash or private numeric representation.
The normalization section is clearly presented as an Umbra support-service
case study, not as the meaning of HLA handle identity. A new synthesis diagram
would repeat the guide's existing flows, so this topic is dispositioned with
no added chart. This was a documentation review only: no implementation or test
re-run, build, CI, fresh normative verification, or Requirements Lab edit.

Next, review the 2025 federation save/restore guide from the HLA mental model
outward. Check whether the portable participant/application checkpoint and
RTI federation checkpoint, request-admission boundary, participant completion
barrier, and terminal federation outcome are visually distinguished from
Umbra-specific coordinator, storage, and profile behavior. Add or revise a
diagram only for a real learner gap, and keep 2010 separate.

### 2025 federation save/restore HLA-first review — 2026-10-10

The save/restore guide already explains the two responsibilities in prose:
each federate checkpoints application-owned state, while the RTI coordinates
the federation operation and its RTI-owned state; a participant's local report
is not the federation-level result. Its detailed diagrams are valuable, but
their coordinator, registry, and storage participants foreground Umbra's
architecture. Added a compact orientation flow to
[`HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md`](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md)
that keeps those responsibility and completion scopes visible without naming
Umbra components. It is explicitly conceptual, not a normative sequence; exact
2025 admission, participant, ordering, and failure rules remain with the
edition-specific guide and authoritative standard. No new requirements,
2010 behavior, or cross-profile equivalence is asserted. This was a
documentation-only change: no C++ build, test rerun, CI, source change, or
Requirements Lab edit. Local/GitHub diagram rendering remains pending.

Next, review the 2025 federation/federate lifecycle guide for a durable HLA
reader model of execution membership, join, resignation, and terminal
membership effects. Keep save/restore and synchronization as linked but
separate state machines; only add a diagram for an uncovered reader question.

### 2025 federation/federate lifecycle reader review — 2026-10-10

The existing guide already explains three independent scopes and includes
separate local-ambassador and federation-execution state diagrams, plus the
current implementation's Create/Join/Destroy and resignation flows. The
reader-facing gap was their relationship: a reader could see each lifetime
but not at a glance which public operation changes which scope. Added a small
orientation map relating connection, membership, and execution, while leaving
preconditions, forced-cleanup rules, and profile behavior in the existing
focused flows. Its empty-execution note is explicitly limited to the current
embedded 2025 profile. The guide remains 2025-only; no 2010 equivalence or new
normative claim is made. The file also contains other uncommitted work, which
was preserved. No build, C++ test, CI, or Requirements Lab edit. Diagram
render/readability review remains pending.

Next, preview the new conceptual save/restore and lifecycle maps using a
GitHub-compatible Mermaid renderer, check their readability, and record that
visual verification separately from any syntax/parser acceptance. Keep the
remaining topic survey moving afterward; do not start a C++ build to validate
documentation.

### 2025 DataElement byte-order reader review — 2026-10-10

A bounded review of the existing 2025 encoding guide confirms the HLA-facing
distinctions are already visible: the concrete value type selects BE or LE,
host endianness is not an implicit policy, each composite delegates child
bytes to the child type, and public value octets are distinct from Umbra's
private process-frame metadata. The guide also keeps its 2010 implementation
and composite rules separate. A second endian chart would duplicate the
existing scalar and composite diagrams; no chart or normative claim was added.
This was a content review only, with no source/test rerun, C++ build, CI, or
Requirements Lab change. The two newer orientation maps still need permitted
render and visual review.

### 2025 reservation and Auto Provide reader review — 2026-10-10

The object-instance name-reservation guide already separates synchronous
service admission from asynchronous reservation outcome, partial bulk reserve
from all-or-nothing bulk release, reservation from registration, owner-only
consumption at registration commit, and reservation persistence/lifetime. The
Auto Provide guide separately distinguishes discovery-triggered provider
solicitation from explicit value requests, initial reflection, and scope
advisories; it also shows the shared switch, per-provider grouping, and
callback-entry revalidation. Those are coherent but distinct HLA/API questions;
a combined chart would collapse service contracts rather than clarify them.
No new diagram or normative claim was needed. This was a bounded content review
of the guides, not a fresh source/test audit; no builds, test reruns, CI, or
Requirements Lab edits.

### 2025 attribute-value update-request reader review — 2026-10-10

The HLA-facing sequence is already clear: a requesting federate names an
object, class, or regional scope; the RTI solicits currently eligible owners;
the provider callback carries the request tag and asks for an update, but does
not itself return values; and a provider independently chooses whether and
when to send an update with its own response tag. That later update still
passes the ordinary receiver, relevance, transportation/order, and time gates.
The guide also isolates the RTI-owned MOM snapshot branch and distinguishes
pending solicitation, already-delivered receive-order data, and queued
timestamp-order data across restore. Its existing diagrams already answer
these learner questions, so another synthesis would repeat rather than clarify
the HLA model. This was a bounded API/source/test-reference review; no test was
rerun, no build or CI was started, and no normative or cross-profile claim was
added.

### 2025 order and transportation reader review — 2026-10-10

The guide already leads with the portable distinction: order describes
receive-order versus timestamp-order delivery, while transportation selects a
FOM-declared category. It explicitly says neither determines the other, and
keeps receiver relevance and time-grant eligibility as separate gates. The
limits section also avoids equating a transportation label with physical
network QoS. Umbra's class defaults, instance overrides, and confirmation
lifecycles follow only after that HLA-facing model and are labeled as current
profile behavior. The existing diagrams already separate the questions, so a
new combined chart would obscure rather than clarify them. This was a
content-first documentation review; no fresh normative, source, or test audit,
build, test rerun, or CI was performed.

### 2025 FOM-module admission and composition reader review — 2026-10-10

The guide already teaches the key model boundary before the implementation
flows: an input module can validate in isolation, the completed module set can
still fail composition/reference resolution, and an FDD artifact can fail its
own schema check. Passing those gates still does not establish that an RTI
implements every behavior described by the model. The Create and additional-
module Join diagrams then label prepare/commit atomicity as current embedded
Umbra evidence, and the 2010 compatibility lane remains separate. This is a
useful HLA-facing explanation of model admission limits without conflating
schema validity, shared model meaning, and runtime capability; another chart
or introductory paragraph would duplicate the existing three-stage table.
This was a content review only, without a fresh normative audit, test rerun,
build, or CI.

### 2025 federation and joined-federate MOM scope review — 2026-10-10

The two guides already distinguish one execution-scoped
`HLAmanager.HLAfederation` object from a separate `HLAmanager.HLAfederate`
object for each membership. They also separate object discovery/reflection
from federation request/report interactions, and link the two lifecycles
without presenting them as one state machine. The federation guide describes
the member-list attribute's Join/resign updates; the joined-federate guide
covers its own discovery, conditional state, restore, and removal. The current
cross-guide relationship is explicit enough that a combined diagram would
duplicate the guides and risk obscuring their different lifetimes. This was a
content review only; no source/test rerun, build, CI, 2010 behavior claim, or
Requirements Lab edit.

### 2025 resignation and automatic-resign reader review — 2026-10-10

The existing lifecycle and ownership guides already cover the useful departure
model: a voluntary `resignFederationExecution(ResignAction)` request is distinct
from connection loss, the per-member automatic directive is distinct from an
explicit request argument, and neither is equivalent to destroying the
federation execution. The effects table names all six API actions while
explicitly labeling their outcomes as current embedded-profile behavior. The
flows and evidence map cover ownership guards/cancellation, delete/divest
effects, per-member reservations and declarations cleanup, survivor callbacks,
the forced-loss acquisition cleanup overlay, and the last-member delete pass.
The focused tests are selected action scenarios, not the full
action-by-ledger/profile matrix. No additional chart would improve the current
division between the lifecycle sequence, action table, and ownership
preflight companion.

The pinned API exposes the §4.12 service, six `ResignAction` values, and
possible exceptions; that interface shape is not the full normative decision
table. IEEE's official 1516.1-2025 page provides subscription/purchase access,
so the exact action preconditions and ordering remain unverified until the
authorized text can be checked. Keep those details labeled as Umbra
observations and do not imply 2010 or process-profile parity. This was a
documentation/source/test-reference review only; no tests were rerun, no C++
build or CI was started, and no Requirements Lab edit was made.

### 2025 class-hierarchy reader review — 2026-10-10

The comparison found a real reader-facing gap: “inheritance” can misleadingly
sound like one propagation rule, while HLA services ask different questions
of the object-class and interaction-class hierarchies. The cross-cutting guide
now has one synthesis map linking inherited attribute/parameter lookup,
class-scoped publication withdrawal, receiver-known-class attribute
projection, interaction subscription matching, and parameter projection.
This map describes selected 2025 embedded behavior; it is not a universal
inheritance algorithm. The interaction case's closest-class behavior for one
receiver with multiple qualifying subscriptions is source-derived; its cited
test uses separate receivers. No process-endpoint or 2010 parity is claimed.

The new Mermaid flowchart passed the installed VS Code 1.141.0 flowchart
grammar without rendering. Visual layout and GitHub rendering remain
unverified. This was a bounded documentation/source/test-reference review;
no C++ build, test rerun, CI, or Requirements Lab edit was made.

### 2025 Delay Subscription Evaluation reader review — 2026-10-10

The existing DSE guide already has three complementary diagrams for switch
lifetime, per-send/per-receiver candidate handling, and receive-order versus
timestamp-order delivery boundaries. A fourth chart would repeat those paths.
The HLA-first review did find a wording risk: “route-only candidate” is an
Umbra work-item shape, not HLA vocabulary, and saying the actual projection is
always computed later blurred Umbra's mechanism with the reader-level rule.
The DSE guide now separates its conservative §8.1.8 paraphrase from the
embedded implementation: a currently eligible receiver may already have a
projection, while a receiver missing the send-time predicate can be retained
route-only and evaluated/projected later. It also links subscription
declaration state separately from evaluation timing.

The bounded source survey checked the existing receive-order planners,
directed-interaction candidate path, the subscription-state guide, and the
DSE evidence map. The focused DSE tests remain the guide's existing embedded
evidence; none were rerun. The official IEEE product page confirms purchase
and subscription access but does not expose §8.1.8, so exact normative wording
and full applicability remain unverified. No 2010 or process parity is
claimed. This was documentation/source review only: no C++ build, test rerun,
CI, or Requirements Lab edit.

### 2025 authorization and credentials reader review — 2026-10-10

The existing guide's two diagrams already separate Connect authorization from
the later Create/Destroy/Join gates. The useful HLA-facing point is not another
diagram: an `AUTHORIZED` result clears that authorization gate but is not the
service result or state commit. The guide now states this reader model before
the callback-scope table, while retaining the implementation-specific RID,
password-file, exception-mapping, and mutex details in their existing lanes.
This also makes clear that the 2025 interface defines three operation scopes,
not a blanket authorization callback for every HLA service.

The bounded review checked the pinned `Authorizer` and
`AuthorizationResult` declarations, service `@throws` lists, current guide,
and the official IEEE listing. The listing identifies IEEE 1516.1-2025 and
offers purchase/subscription access, but does not expose §4.2; the overload
exception discrepancy remains unresolved pending an authorized copy. Existing
authorization test outcomes are preserved as previously recorded and were not
rerun. No process parity or 2010 authorizer behavior is inferred. No C++
build, tests, CI, or Requirements Lab edit was made.

### 2010 reference-profile object lifetime and callback fan-out review — 2026-10-10

The separate 2010 guide now makes the reader-facing distinction between a
federation-wide object record and what each receiving application has actually
learned explicit. Its object flow distinguishes registration/discovery,
owner-resignation cleanup, unavailable delete/local-forget services, and the
absence of a removal callback on the implemented cleanup path. The service
stubs throw `RTIinternalError`; the reference runtime ignores `ResignAction`
when erasing owned records. These are 2010 profile limits, not normative
descriptions of IEEE 1516.1-2010 object-deletion or local-forget semantics.

The source also establishes a useful state/effect boundary: registration stores
the object before inline discovery fan-out. A discovery callback failure is
translated to `RTIinternalError`, leaves the object record committed, and
prevents later recipients from being visited. The interaction flow now shows
the related partial-observation case: a receiver exception stops inline
fan-out after any earlier callbacks have run. The binding-shell smoke covers
the successful single-subscriber discovery/reflection and interaction paths;
it does not test deletion, resignation removal, or callback-failure outcomes.
These error edges are source-derived only. Source/API links were checked; no
C++ build, tests, or CI were run. No 2025 behavior or Requirements Lab change
is inferred.

### 2025 object/interaction state and observation reader review — 2026-10-10

The bounded survey found the core reader model already present, so another
parallel flow would be redundant. The 2025 information-flow guide opens with
four separate questions—producer permission, recipient relevance, per-receiver
object knowledge, and application observation—and explicitly separates an
accepted/planned service, queued event, callback dispatch, and callback-visible
state. Its embedded accepted-send flow distinguishes one invocation's report
and counter boundary from zero or more recipient callbacks, including
`HLA_IMMEDIATE` versus `HLA_EVOKED` and callback-time eligibility rechecks.
The deletion and local-delete sections separately track federation object
state and per-receiver knowledge.

The process-endpoint guide keeps another necessary boundary: the sender's
correlated request/response and each recipient's event stream are separate
exchanges. It documents that push events can precede the sender response while
callback submission occurs after the correlated exchange releases its lock;
polling and public Evoke paths have their own scoped test evidence. Source
inspection confirmed the embedded accepted-send report/counter occurs before
unlocked callback fan-out, while the process push branch writes recipient
event frames before returning the sender result. Those are distinct
implementation paths, not cross-profile guarantees. Existing focused-test
results were reused, not rerun; the source and test evidence maps already state
where scenarios are incomplete or failed before assertions.

I added direct links from the information-flow overview to the cross-cutting
state/event/observation and callback/service-ordering guides. No new diagram
was needed. This review makes no new normative claim and does not infer 2010
or process/embedded parity beyond each guide's explicit evidence. No C++ build,
tests, CI, or Requirements Lab edit was made.

### 2025 callback exception call-stack review — 2026-10-10

The callback/service-ordering guide now answers a reader question the generic
dispatcher chart left open: which application-to-RTI call is active when a
callback throws, and what accepted work preceded the callback? Its HLA-first
flow distinguishes HLA_IMMEDIATE stack exposure from a later HLA_EVOKED
dispatch, including the case where another thread already owns the immediate
drainer. This is a call-stack mental model, not a universal exception policy.

For one bounded embedded 2025 Send Interaction path, source shows an attempted
successful service report and accepted-send counter before callback fan-out.
If a Receive Interaction exception unwinds on that same immediate service
stack, the service catch attempts HLAreportException and a failed service
report, then rethrows; the accepted-send counter and any successfully appended
report record are not rolled back. Report sinks are independent fallible
effects. The Evoke entry points have no local report catch. Process
request/response and pushed-event handling remain a separate path; no
embedded/process equivalence is inferred. The 2025
API declaration for Receive Interaction lists FederateInternalError, but the
exact service/report outcome is source-derived, not a normative guarantee.

Existing dispatcher tests cover normal callback completion, not a callback
exception crossing Send Interaction or Evoke, report-sink outcomes, or rollback
behavior. No tests were rerun, no C++ was built, no CI was started, and no 2010
or Requirements Lab behavior was changed or inferred.

The exception-reporting guide now cross-links this callback-origin example at
its public-service exception boundary. The service-invocation guide already
separates its record/file sinks and switches from both exception-report paths;
no second synthesis chart was needed.

Rendering remains pending. The in-app browser rejected the local-preview URL;
no alternate browser route was attempted. The installed VS Code Mermaid module
loaded locally in Node, but the non-rendering parse harness did not initialize
its DOMPurify hook and produced no syntax result or SVG. This is an environment
setup limitation, not a diagram assertion. A read-only scan found that all
relative Markdown file destinations in `docs/design` exist; heading anchors
were not part of that check. The roadmap JSON parses.

Next, visually inspect this Mermaid flow through a permitted local Markdown
preview and record the preview/GitHub status. If no preview-capable local
surface is available, do not route around the browser restriction; keep this
check pending and continue with a content-first review of the prioritized
2025 topic inventory for a distinct HLA reader gap. Avoid duplicate diagrams,
preserve process/embedded and 2010/2025 boundaries, and do not build, rerun
tests, start CI, or edit the HLA Requirements Lab. Preserve the pending
visual/GitHub review of the class-hierarchy synthesis chart.

Before calling a topic guide-ready, verify that its diagrams render in GitHub's
Mermaid Markdown, all relative source/test links resolve, behavior statements
are traceable to the named implementation and focused tests, normative
statements are checked against the correct official edition, and the explicit
edition boundary remains intact. Record what is not modeled and leave the next
topic as a concrete handoff here and in
[ROADMAP-INDEX.json](../planning/ROADMAP-INDEX.json).

### 2025 DDM and RTI Support Services reader-gap survey — 2026-10-10

A content-first pass through the DDM guide found its distinct reader questions
already separated: pending versus committed region specifications, strict
overlap versus Umbra's optional touching rule, object-attribute versus
interaction routing, discovery versus later value relevance, per-recipient
process fan-out, and the independent time/callback gates. No additional
portable HLA state transition was missing from the guide, so another overview
diagram would repeat existing material.

A bounded method-surface check of the pinned 2025 RTI Support Services group
found its main reader concerns already routed to the names/handles, DDM,
update-rate, advisory, lifecycle, reporting, callback, time, and DataElement
guides. This is service-family navigation coverage, not a clause-by-clause
contract audit or a claim that every overload/profile is implemented. The
existing guides already distinguish typed HLA identity, federate-local object
knowledge, handle serialization, dimension/range queries, callback entry, and
factory-defined logical time. No new diagram was warranted. The survey reused
the pinned API and existing documentation; no C++ source, build, tests, CI,
2010 behavior, or Requirements Lab content was changed.

Next bounded review: compare the pinned 2025 Federation Management method
group with the lifecycle, FOM admission, synchronization, save/restore, and
name-reservation guides. Add or revise a diagram only for a distinct portable
HLA question, keeping Umbra-specific execution paths as explicitly bounded
case evidence.

### 2025 Federation Management surface survey — 2026-10-10

A method-level pass over the pinned 2025 Federation Management group mapped
execution creation and listing, membership listing and resignation, FOM-aware
creation, synchronization-point coordination, and save/restore controls to
existing lifecycle, FOM-admission, synchronization, and save/restore guides.
The lifecycle guide already explains member listing as a point-in-time report,
including the difference between capturing a snapshot and later callback
delivery. No additional portable HLA diagram was indicated by this surface
review.

The lifecycle guide also explicitly preserves one evidence gap: the focused
member-list test covers EVOKED delivery, while its IMMEDIATE-mode assertion is
for listFederationExecutions instead. No test was run to close that gap. This
does not change the documented source observation or establish a normative
requirement. No C++ source, build, tests, CI, 2010 behavior, or Requirements
Lab content was changed.

Next bounded review: compare the pinned 2025 Object Management method group
with the existing object/information-flow, name-reservation, directed-
interaction, update-request, ownership, order/transportation, and DDM guides;
look for a distinct HLA reader question rather than diagramming every method.

### 2025 Object Management and delivery-property reader survey — 2026-10-10

A bounded pass over the pinned 2025 Object Management methods matched name
reservation, object registration/update/deletion, ordinary and directed
interaction sends, explicit attribute-value requests, and transportation
changes/queries to existing topic guides. The order/transportation guide
already leads with the durable distinction between delivery order and
transportation category, then keeps receiver relevance and logical-time
eligibility on separate axes. The relevance and scope-advisory guides also
already contrast publisher-, owner-, and receiver-facing signals. No missing
portable HLA transition justified another diagram. This was a guide-surface
review, not a clause audit or claim of full overload/profile coverage; no C++
source, build, tests, CI, 2010 behavior, or Requirements Lab content changed.

Next bounded review: inspect the process-endpoint guide for any reader
confusion between HLA service/callback boundaries and Umbra-private request,
event-frame, or acknowledgement machinery. Promote only reusable behavior
concepts; label transport architecture as implementation evidence.

### 2025 process-endpoint HLA-versus-transport review — 2026-10-10

The process-endpoint guide already keeps the layers explicit: HLA service and
callback behavior is described separately from Umbra-private request IDs,
event frames, and acknowledgements. It distinguishes event push/poll from
HLA_IMMEDIATE/HLA_EVOKED dispatch, and distinguishes both from logical-time
eligibility. The TSO acknowledgement trace begins after recipient delivery has
been admitted by time management, so it does not present the private ACK as an
HLA time or retraction rule. No new portable behavior gap or diagram was
identified; these transport details remain a bounded 2025 implementation
case. No C++ source, build, tests, CI, 2010 behavior, or Requirements Lab
content changed.

Next bounded review: compare the 2025 Connect/Disconnect guide with the
federate-membership lifecycle and authorization guide. Check that connection,
authorization, and execution membership remain separate reader concepts, and
revise only if a real HLA-facing confusion remains.

### 2025 Connect, authorization, and membership boundary review — 2026-10-10

The three existing guides make the key lifetime boundaries explicit and
cross-link them: Connect establishes a local, unjoined connection;
authorization clears only an operation-specific admission gate; Join/Resign
changes membership; and Disconnect is distinct from resignation and is allowed
only after leaving the execution. Connection loss remains a separate forced
cleanup path. The diagrams and opening mental models already distinguish these
concepts, so a fourth overview would duplicate them.

One normative question remains deliberately open in the authorization guide:
the pinned 2025 API lists different Connect exception annotations across
overloads, while Umbra uses a shared result mapper. The licensed §4.2 wording
is not available in this checkout, so this survey makes no conclusion about
the standard contract. Existing test outcomes were reused; no tests were run
and no C++ source, build, CI, 2010 behavior, or Requirements Lab content was
changed.

Next bounded review: read the 2025 federation-scoped MOM guide for its
standard-facing object and observation model, checking where its lifecycle
callbacks and Umbra's snapshot/storage details need clearer separation.

### 2025 federation- and member-scoped MOM boundary review — 2026-10-10

Cross-reading the federation-scoped and joined-federate MOM guides confirms
they answer different HLA reader questions: one RTI-owned HLAfederation
instance represents the execution, while a separate HLAfederate instance is
associated with each joined membership. Both use ordinary object discovery
and reflection, while HLArequest/HLAreport interactions are a separate
receive-interaction path. The current guides already keep these lifetimes,
observation mechanisms, and profile evidence apart; another synthesis diagram
would duplicate them.

The joined-federate guide's official 1516.2-2025 link contained an incorrect
standard-number segment. The IEEE listing confirms the OMT specification's
direct page is 1516.2-2025; corrected the URL without changing any behavioral
claim. Full 1516.1 normative review and Mermaid/GitHub rendering remain
separate completion checks. No C++ source, build, tests, CI, 2010 behavior, or
Requirements Lab content changed.

Next bounded review: survey the remaining 2025 MOM statistics and periodic
counter surface in the pinned MIM, source, tests, and current guides. Decide
whether it exposes a distinct portable monitoring concept worth documenting;
do not conflate it with either MOM object's lifecycle or invent a diagram
where no reader gap exists.

### 2025 joined-federate MOM cadence and measurement-semantics review — 2026-10-10

The pinned 2025 standard MIM exposes a useful distinction that was not yet
visible in the joined-federate lifecycle guide: explicit Request Attribute
Value Update is independent of periodic reporting and must supply values for
Static, Periodic, and Conditional attributes; `HLAsetTiming` separately
controls the wall-clock refresh cadence of Periodic attributes on a specified
other joined federate's `HLAfederate` instance. Never-set and zero intervals
disable automatic periodic updates. Added this portable two-trigger flow to
the existing guide, keeping callback observation on the ordinary attribute
update path.

The guide now illustrates why a shared “counter” mental model is insufficient:
service-invocation totals, object-instance counts, queue-length observations,
and time-state durations have different meanings even when all are periodically
refreshed. The paired embedded test explicitly requests values before
periodic reporting, then shows two updates to one object produce two service
invocations but one tracked object, while a second object advances the counts
to three and two. The source/test example is labeled as embedded evidence; no
process parity or 2010 behavior is inferred. No C++ build, tests, CI, or
Requirements Lab changes were made.

Next bounded review: return to the 2025 timing reader path and cross-read the
time-management, order/transportation, callback-ordering, and TSO-retraction
guides around the boundary between a time-advance request, timestamped-message
eligibility, callback observation, and grant. The existing guides already
cover these topics, so look for one specific HLA-level relationship that a
reader still has to mentally reconstruct; do not redraw the whole timing
system or foreground private scheduler mechanics. Reuse existing evidence
without building or rerunning tests, and retain the strict 2010/2025 and
embedded/process boundaries.

### 2025 time-advance callback-observation boundary review — 2026-10-10

The time-management guide already explains temporal grant eligibility and
TSO-before-grant ordering, while the callback guide separately explains that
HLA_EVOKED work is not application-visible until an Evoke service dispatches
it. Their separate diagrams left one useful mental join implicit: a time
advance may be accepted and its callbacks eligible while the federate still
observes its previous logical time; the callback model controls when the
ordered TSO work and matching grant cross into application code. Added a
compact HLA-facing flow to the existing time-management guide, abstracting
private dispatcher mechanics.

The source comment at the embedded time-advance dispatch boundary orders TSO
callbacks before the matching grant. An existing HLA_EVOKED mixed TSO/TARA/
NMRA case leaves the TARA/NMRA callback order empty before each receiver
evokes, then asserts interaction-before-grant order. This is evidence for
that embedded 2025 scenario only; no process or 2010 parity is inferred. The
previous focused C++ results were reused; no build, test rerun, CI, or
Requirements Lab change was made. Visual/GitHub rendering remains pending.

Next bounded review: survey the separate 2025 federation-synchronization
guide from the HLA mental model outward. Cross-read registration, announcement,
achievement, synchronized notification, late Join, and resignation edges;
look for a reader-facing barrier or completion distinction not already clear
in that guide and the cross-cutting lifecycle model. Reuse existing evidence
without building or rerunning tests, keep 2010 separate, and do not edit the
HLA Requirements Lab.

### 2025 synchronization-barrier completion review — 2026-10-10

The synchronization guide already distinguishes participant selection,
announcement, achievement fan-in, failure reporting, late-join expansion,
resignation, save/restore, and callback execution. The cross-cutting guide
likewise warns that participant-set changes are protocol-specific, so a new
barrier overview would repeat existing coverage. One state-chart edge did need
to be made explicit: the guide's `Pending -> Waiting -> Completed` picture
could imply that every point needs two achievements. Updated the existing
fan-in chart to show a one-member set completing on its first report and to
show non-final reports as a `Waiting` self-loop. Clarified that a false report
fills a slot and contributes to the eventual failure set only while that
participant remains in the set at completion.

The registry inserts each participant's first result and completes when the
result count reaches the current participant-set size. The focused public
example exercises a two-member point whose second report is false; the bounded
test search found no focused one-member completion or early-false-then-later-
achievement case. Those edges remain source-derived, not test-observed. This
review reused source/tests without running them, did not edit the HLA
Requirements Lab, and makes no 2010 or process-parity claim. Visual/GitHub
rendering remains pending.

Next bounded review: survey the separate 2025 advisory guides—relevance,
scope, and update-rate—against object-attribute subscription and DDM. Check
whether readers can tell what each advisory means, what state change triggers
it, and whether it changes recipient eligibility or only reports a condition.
Preserve each service's distinct lifecycle; synthesize only a useful HLA-level
comparison, not a merged state machine. Reuse existing evidence without
building or rerunning tests, keep 2010 separate, and leave the HLA Requirements
Lab untouched.

### 2025 advisory-versus-admission cross-guide review — 2026-10-10

The relevance, scope, update-rate, subscription-state, and DDM guides already
preserve their own triggers and state machines; another combined state diagram
would risk implying a single advisory protocol. One reader-facing comparison
was less direct: the existing relevance overview distinguished the callback
recipients, but did not place the subscriber-side update-rate admission gate
beside those notifications. Expanded that small overview table to map class
relevance, object-attribute relevance, receiver scope, update-rate admission,
discovery, and reflection by trigger, target, and effect boundary. It explicitly
distinguishes owner-facing rate-designator advice from the receiver-side
best-effort filter without merging their lifecycles.

The existing linked guides provide the underlying 2025 source/test evidence:
relevance transitions and callback switches, receiver-local scope and stale
work checks, active/passive subscription state, region-based DDM, and
per-attribute update-rate admission. This increment reused those results; no
C++ build, test rerun, CI, 2010 claim, process-parity claim, or Requirements
Lab change was made. Visual/GitHub rendering remains pending.

This handoff is complete: the object-information guide now compares accepted
federated deletion, caller-local forgetting, action-driven resignation
cleanup, and forced connection-loss cleanup by execution object state,
per-member knowledge, and callback observation. Its existing distinct delete
and local-delete flows remain authoritative; the new table is only a reader
map. The lifecycle guide links back to the comparison, including the
final-member case where object cleanup does not destroy the execution. The
comparison is explicitly current-2025 implementation evidence, with embedded
and process test limits kept visible. No tests were rerun, no build/CI was
started, and no 2010 or Requirements Lab claim was added.

### 2025 interaction-recipient mental-model survey — 2026-10-10

This survey found one meaningful comparison gap despite the two detailed flows:
ordinary `active` and directed `universally` are both booleans at the API
surface but control different questions. Added a compact side-by-side map to
the object/information overview: class-level relevance plus receiver matching
for ordinary interactions versus target-object knowledge and the directed
universal/by-ownership selector. It also distinguishes the sender's
publication declarations and the target context present in the directed
callback. The directed guide links back to the comparison.

No combined state machine was added; the guides' detailed ordinary and directed
flows remain separate. Their existing source maps and focused scenarios ground
the comparison, including ordinary active/passive and class-projection cases,
directed selector/known-target cases, and selected process multi-recipient
delivery. These do not constitute an exhaustive cross-product or normative
conformance proof. This increment reused that evidence without building,
rerunning tests, or starting CI; it makes no 2010 or cross-profile equivalence
claim and does not edit the HLA Requirements Lab. Visual/GitHub rendering
remains pending.

### 2025 class-inheritance reader-gap survey — 2026-10-10

The individual service guides described class ancestry in their own contexts,
but a reader could still assume that every lookup walks the same tree or has the
same callback effect. Added a reader map to the 2025 object/information guide:
object discovery chooses the receiver's known class; attribute reflection is
filtered by that class; ordinary interaction matching may promote the received
class and parameter set; directed interaction routing walks the target object's
class chain while retaining the sent interaction class in the current path.
The diagram distinguishes object-class from interaction-class ancestry and
links to the existing focused flows rather than replacing them.

Bounded source/test evidence supports discovery, known-class attribute
projection, and ordinary interaction promotion. Directed target-class ancestor
resolution is source-observed, but the reviewed focused tests cover exact-class
targets rather than an ancestor-declaration case. The public IEEE listing
does not expose the licensed clause text, so the new map avoids a normative
hierarchy claim and leaves exact rule validation open. No test/build/CI was
run; no 2010 or process-parity claim or Requirements Lab change was made.
GitHub Mermaid rendering remains pending.

### 2025 directed hierarchy profile-boundary survey — 2026-10-10

The process send handler calls the same registry planner as the embedded
directed path, so the target-class ancestor declaration/selector lookup is
shared in source. The process multi-recipient test uses exact-class
declarations and verifies callback payload/context in both callback models;
it does not test ancestor lookup or a subscription/target mutation between
send acceptance and callback delivery. A source comparison found that the
embedded receive-order callback re-plans eligibility at callback time, while
the process receive-order path materializes an event at send time and later
dequeues/bridges that stored projection without a matching registry re-plan.
This is a profile-specific source observation, not a normative claim or a
tested stale-delivery outcome. Added the boundary diagram and updated the
cross-service class-ancestry map. No build, test rerun, CI, 2010 inference, or
Requirements Lab edit; GitHub Mermaid rendering remains pending.

### 2025 ordinary interaction projection profile-boundary survey — 2026-10-10

The shared registry planner selects the closest eligible interaction class and
its visible parameter handles for both profiles. In the embedded path the
callback closure recomputes that recipient projection immediately before user
code. In the process pull path the send service stores the chosen class and
parameter handles in an event, and receive/decode dispatches that stored view
without a registry re-plan. Process declaration mutations do not rewrite that
already-materialized ordinary event. This is source evidence only; the public
process test inspects an exact-class event's parameter value and tag, not
class promotion or post-send declaration changes. Added the profile-boundary
sequence and process evidence row. No build/test/CI, 2010 inference, or
Requirements Lab edit; GitHub Mermaid rendering remains pending.

### 2025 advisory cross-guide mental-model review — 2026-10-10

The 2025 relevance guide's comparison table already distinguishes publisher-
facing declaration interest, owner-facing per-instance attribute demand,
receiver-facing scope transitions, discovery, reflection, and embedded
per-attribute update-rate admission. The scope, DDM, and subscription guides
retain their own state machines and profile/test boundaries. The useful reader
gap was the cross-service consequence of one subscription mutation: separate
calculations can plan callbacks for different federation roles, while any
later reflection still needs its own value-delivery path. Expanded the existing
“one object subscription, separate calculations” flow to show those role-
directed branches, object discovery, and a dashed later-value path. The diagram
does not prescribe callback order or assert profile parity; update-rate
admission remains explicitly profile-specific. No build/test/CI, 2010 inference,
or Requirements Lab edit. GitHub Mermaid rendering remains pending.

### 2025 lifecycle cross-guide participant review — 2026-10-10

The lifecycle, object-removal, and ownership guides already cover the detailed
state machines: voluntary ResignAction validation and member-scoped pending
work; forced connection-loss cleanup and its member-specific automatic
directive; receiver-specific object-removal commit gates; and stale ownership
callback suppression. The missing reader view was how those effects differ
between the departing participant, continuing peers, object state, and the
federation execution. Replaced the lifecycle guide's compact exit table with a
five-path participant matrix: voluntary resignation with peers or as the last
member, connection loss with peers or as the last member, and explicit Destroy.
The final-member connection-loss row is grounded in a focused embedded 2025
case that configures NO_ACTION, observes local loss reporting without a
resignation description, then rejoins the still-existing execution and reuses
the deleted object's name. This does not establish process-endpoint parity or
an edition-wide normative rule. Detailed callback gates remain in their topic
guides, avoiding duplicate low-level flows. No build/test/CI, 2010 inference, or
Requirements Lab edit; GitHub Mermaid rendering remains pending.

### 2025 ownership and pending-TSO reader bridge — 2026-10-10

The ownership guide already says ownership is not update delivery, while the
time and TSO-retraction guides explain temporal queues, per-recipient delivery,
and producer-bound retraction. A cross-read of those guides found two focused
embedded cases that make the boundary concrete: one has a subscribed,
time-constrained acquirer receive an already-queued update after divestiture;
the other has a survivor acquire after the update source resigns and later
receive that same producer's queued update. Both assertions retain original
producer, value/tag/time/order, and valid retraction-designator data. Added a
reader-facing parallel-timeline diagram and reciprocal ownership-guide link.
Clarified that recipient fan-out was planned at send time, current recipient
eligibility is an independent gate, and a valid handle observed after source
resignation does not prove a departed producer can invoke Retract. These are
embedded 2025 observations, not process parity or a general promise against
callback suppression. No build/test/CI, 2010 inference, or Requirements Lab
edit; JSON parsing, added local links/anchors, Mermaid fence balance, and diff
whitespace checks passed. A Mermaid renderer is unavailable locally, so visual
GitHub rendering remains pending.

### 2025 callback and service-report application-timeline review — 2026-10-10

Cross-read the callback/service-ordering, service-invocation-reporting,
exception-reporting, and cross-cutting state/event/observation guides from the
application's point of view. The existing cross-cutting guide already provides
the useful reusable sequence of questions—request validity, admission/result,
state transition, effect planning, eligibility, application observation, and
completion—and explicitly warns that these milestones are not one universal
transaction timeline. It also already distinguishes caller outcome, committed
or admitted work, a selected report path, and later observation as separate
traces. No additional diagram was warranted.

The focused directed-invocation report test observes an RTI-originated report
before the directed application callback for its selected HLA_EVOKED scenario.
The dispatcher tests cover Umbra's immediate synchronous dispatch and evoked
one-at-a-time/FIFO behavior; neither establishes a general cross-service HLA
ordering rule. Optional service reports, ordinary exception reports, and
callback dispatch remain separate mechanisms. No source changes, builds, test
reruns, CI, 2010 inference, or Requirements Lab edit; GitHub Mermaid rendering
remains pending.

Next bounded review: survey the 2025 federation/federate lifecycle, FOM
composition, declarations, and object/interaction guides as one
application-facing HLA usage path. Determine whether an overview can teach the
durable progression from model vocabulary to joined membership,
publication/subscription, runtime object/message exchange, callbacks, and exit
while making optional services and non-linear choices explicit. Add a
cross-guide diagram only if it clarifies real HLA concepts without claiming a
mandatory service sequence or duplicating existing flows. Reuse existing
evidence; no builds, test reruns, or CI. Keep 2010 separate and leave the HLA
Requirements Lab untouched.

### 2025 federation-execution application map — 2026-10-10

The lifecycle, FOM composition, and object/interaction guides already explain
their own transitions well. The remaining reader gap was orientation across
them: the suggested reading route linked topics but did not show how model
vocabulary, execution existence, member participation, declarations, exchange,
eligibility, observation, and exit relate. Added two 2025-only diagrams: one
for execution/member lifetimes and one for object/interaction information
paths. A local VS Code Markdown-preview review found both layouts readable,
with no Mermaid error panels; splitting the orientation into two diagrams
keeps lifecycle choices apart from receiver-observation paths. This was a
visual check of these two diagrams only, not a whole-doc syntax/layout audit or
GitHub verification. No C++ changes, build, test rerun, CI, 2010 inference, or
Requirements Lab edit.

### HLA-first discovery and 2025 API-family map — 2026-10-10

A fresh read of the current guide set confirmed that the proposed service-
failure survey was already represented: the cross-cutting guide separates the
caller result, state/admitted work, report effect, and observer view; the
exception, service-invocation, and callback guides retain their own detailed
rules and evidence. No duplicate synthesis diagram was added. A bounded read
of the pinned 2025 `RTIambassador` family headings against the topic index
found a reader path for each public service family; the new table is an
orientation map, not an overload-coverage claim. `docs/README.md` now links
directly to that HLA-first index and the portable cross-cutting model. No C++
source, tests, build, CI, 2010 behavior, or Requirements Lab content changed.

Next bounded review: follow one FOM dimension from declared model vocabulary,
through class-available dimension queries, into the dimension set and range
bounds of a region, and finally into DDM matching. Compare the FOM, DDM, and
name/handle guides with the pinned 2025 API, source, and already-recorded test
evidence. Decide whether this model-to-runtime boundary is a real reader gap
that merits one HLA-first diagram; do not restate region lifecycle or overlap
rules already covered. Keep time eligibility and callback dispatch as separate
filters, preserve 2010 separately, and do not build, rerun tests, or start CI.
Leave the HLA Requirements Lab untouched.

### 2025 FOM-dimension-to-region bridge and hierarchy evidence — 2026-10-10

Added the missing reader bridge from FOM dimension declarations and
class-associations through class-availability queries and application-chosen
region dimensions/ranges to DDM scope. The flow is conceptual, not a mandatory
call sequence: `createRegion` has no class argument, while later
class-specific services may impose separate compatibility checks. The guide
also distinguishes an empty class-availability result from a zero-dimensional
region, which is not a wildcard in the documented 2025 development profile.

A compact test-results table records the embedded 2025 `Drink` → `Soda` →
`Light` available-dimension sets and two interaction-class query results. The
registry source walks the class parent chain; the focused embedded test asserts
those results. A configured process-endpoint test covers the query results
only, not regional delivery. The embedded case also adds a separate class and
dimension through a later module join and observes the new lookup while an
existing dimension handle remains stable; it does not test changing an
existing class's dimension associations or the effect on already-created
regions. A source-only follow-up found that recursive composition can append a
new dimension reference to an existing class; the successful Join swaps the
composed definition and handle directories without rewriting region records.
A later object update-region association request checks region dimensions
against the class's current available set. The existing tests reviewed here do
not exercise same-class extension with a live region, so this remains an
explicitly source-derived branch, not normative or process-endpoint evidence.
The DDM and FOM guides are cross-linked for the API model and module-admission
boundaries.

This remains 2025-only. It makes no 2010 inference and does not claim process
DDM parity. JSON parsing, local link targets, Mermaid fence balance, and
`git diff --check` pass for this increment. No C++ builds, test reruns, CI, or
Requirements Lab changes were made. Mermaid syntax/rendering and GitHub visual
review remain pending: `mmdc` is absent, the current computer-use surface has
no editor window, and the prior ledger records that the in-app browser rejected
the loopback preview. The preview policy says not to route around that
restriction; no alternate browser was used.

The bounded registration-to-reflection survey found that the object/information
guide already traces publication, registration, receiver-local discovery,
attribute projection, and callback boundaries; the subscription and relevance
guides add the active/passive and advisory branches, while the cross-cutting map
already shows the independent gates. Another universal end-to-end diagram would
duplicate these paths and risk collapsing distinct predicates. A narrower gap
remained: readers could read “passive-only does not deliver” as “passive never
receives,” although the embedded source separates federation-wide active
relevance from each receiver's own matching declaration. Added a source-labeled
diagram and an explicit test-coverage limit in the subscription guide; no
normative or process-parity claim was added.

### 2025 passive-subscription receiver relevance — 2026-10-10

The registration-to-reflection survey confirmed that the object/information
guide, subscription-state guide, relevance-advisory guide, and cross-cutting
map already provide the broad path and its independent gates; an extra
universal pipeline would have duplicated them. A more specific reader gap was
worth diagramming: the passive-only test establishes no discovery/reflection
when there is no other active declaration, but current embedded source first
computes active relevance across non-producer members and then checks each
receiver's own subscription. A matching passive receiver can therefore
qualify after another member establishes relevance, subject to its own
class/region path and the later delivery gates.

Added a source-only two-lane flow to the object-attribute subscription guide
for registration/discovery and later update/reflection, and cross-linked it
from the relevance-advisory guide so Turn Updates On/Off is not mistaken for
reflection. The cited passive integration test has one publisher and one
subscriber; it covers passive-only suppression and the subscriber's own
passive-to-active transition, not a separate active establisher plus passive
receiver. The new branch is not normative, process-endpoint, or 2010 evidence.
Static inventory was reconciled to 191 Mermaid fences across 32 guide files
(2010: 11 separate; 2025: 180). The new chart is not parsed or rendered; no
C++ build, test rerun, CI, or Requirements Lab edit was made.

Next bounded review: compare the 2025 process-endpoint registration,
subscription, and attribute-update path with the embedded passive-receiver
branch. Use source and existing focused tests only; state separately what is
shared, what is process-tested, and what remains unknown. Documentation-only;
no build/test/CI, no 2010 inference, and no Requirements Lab changes.

### 2025 process service reuse of passive-relevance selection — 2026-10-10

The process-service comparison found that the 2025 service holds an
`EmbeddedFederationRegistry` reference. Its ordinary subscription handler calls
the registry declaration updater and existing-object discovery planner; its
registration handler calls the same registry registration and per-instance
discovery planner; its receive-order attribute update handler calls the shared
registry update planner before the process service forms recipient events.
The event route remains process-specific: selected work is queued per session
or sent over the process connection and then submitted by the client callback
bridge.

The existing public process TSO attribute-retraction fan-out test traverses
active subscription, registration, and timestamped reflection through a TCP
endpoint, but it does not exercise a passive receiver. The focused embedded
passive test still has only a publisher and one subscriber. Thus the shared
selector call chain is source evidence, not an end-to-end process test or a
general embedded/process parity claim. No 2010 or normative conclusion was
added. The subscription guide now records this distinction and links to the
separate process callback guide; no duplicate transport diagram was added.

Next bounded review: inspect the process service's late-subscription path for
an already-registered object. Trace discovery enqueue, scope-change and
relevance-advisory enqueue, push versus polling, and callback submission order;
check existing source/tests for what is observed versus source-only. Add a
focused diagram only if the existing subscription and process guides do not
already explain this ordering. Documentation-only: no builds, test reruns, or
CI; preserve the separate 2010 guide and leave the Requirements Lab untouched.

### 2025 late-subscription effects and initial-value boundary — 2026-10-10

The reader-level distinction is now explicit: changing interest can affect
receiver knowledge (Discover Object Instance), receiver attribute scope,
initial values for a newly known object, and provider-facing attribute
relevance advice. These are different state/effect boundaries, and the owner
advisory is not data delivered to the subscriber. Added a compact HLA-first
flow to the 2025 subscription guide; it intentionally does not assert one
global callback order across federates or blend this behavior into the 2010
stream.

The embedded 2025 path commits known-object state at discovery callback time,
invokes Discover Object Instance, then plans and reflects the current eligible
subscribed values. A focused regional-routing test observes an already-
registered object become discoverable after overlap and sees its latest
accepted value reflected before a later publisher update. A separate ordinary
subscription test registers first, subscribes later, and checks the provider's
relevance advisory.

The process source commits registry-known state before exposing a discovery
event and separately plans owner advisories; it contains a MOM-only initial
value branch. The ordinary initial-reflection planner callsite appears only in
the embedded runtime source in the bounded search. The process tests examined
cover subscription-before-registration discovery/advisory, or a later
attribute subscription on an already-known object, not ordinary late
discovery plus initial values. This is an unresolved process/source/test and
normative-review question, not a conformance verdict. The exact licensed
1516.1-2025 §5.8 text was not available for independent verification; the
official IEEE listing is cited in the guide. No C++ build, test rerun, CI, or
Requirements Lab edit was made.

### 2025 late-subscription path inventory — Auto Provide separation — 2026-10-10

A cross-guide check against the Auto Provide flow found that it already
compares four distinct paths: discovery-triggered provider solicitation,
explicit value requests, direct initial reflection, and scope changes. The
late-subscription sketch now includes a small Auto Provide branch and links to
that existing comparison rather than redrawing its switch/fan-out/revalidation
state machine. This makes the provider's relevance advisory distinct from a
separate provider request to send values, and keeps a later update as the
actual data-delivery event.

The additional branch is bounded to the embedded 2025 path documented by the
Auto Provide guide; it adds no process or 2010 inference and no normative
clause claim. The exact ordinary late-subscription §5.8 text remains an open
review item because the local checkout contains no full standard copy and the
official page offers purchase/subscription access. No C++ build, test rerun,
CI, or Requirements Lab edit was made.

### 2025 HLA callback model and application observation — 2026-10-10

The callback/service-ordering guide already explained Umbra's queue, dispatch,
exception, cancellation, and profile-specific paths in detail, but readers
encountered dispatcher mechanics before a concise HLA-facing explanation of
what the callback model means. Added a separate conceptual flow first: the
service contract determines whether a particular federate has a callback
effect; the selected callback model determines how application code enters
that callback path; and state/result/completion remain operation-specific.
The flow explicitly disclaims universal recipient rules, global order, or a
single commit boundary. It uses the 2025 API's public callback vocabulary and
links to the focused time, ownership, and synchronization guides, leaving
Umbra's dispatcher diagram as implementation evidence below it.

This is a reader-oriented synthesis, not a new normative interpretation or
2010/2025 equivalence claim. The new diagram has not yet been parsed or
rendered; GitHub layout is unverified. No C++ build, test rerun, CI, or
Requirements Lab edit was made.

Next bounded review: compare the HLA meaning and ordering of ordinary
late-subscription discovery/current values in IEEE 1516.1-2025 §5.8 against
this source map when an authorized copy is available. Keep embedded and
process observations separate; if the normative text is unavailable, preserve
the open question without inferring a defect. Documentation-only; no builds,
tests, CI, 2010 inference, or Requirements Lab changes.

### 2025 guide-opening survey and inventory reconciliation — 2026-10-10

A bounded HLA-first opening survey checked the callback, cross-cutting,
time-management, object/interaction, ownership, DDM, synchronization, and
federation-lifecycle guides. Each puts a reader-facing HLA concept before its
implementation evidence. These openings already provide distinct mental
models; adding another generic cross-service overview would repeat the existing
state/eligibility/observation maps rather than explain a new behavior.

Recounting the current fenced blocks exposed a classification error in the
previous inventory: two flowcharts in this index had been counted as 2025
behavior diagrams. The corrected inventory is 193 diagrams across 32
diagram-bearing Markdown files: 2010 has 11 diagrams in 2 separate guides;
2025 has 180 in 29 separate guides; the index has 2 edition-neutral navigation
maps. Totals by syntax are 104 flowcharts, 67 sequences, and 22 state diagrams.
The callback guide's new HLA-first diagram remains unparsed and unrendered; a
bare-Node attempt to use VS Code's bundled Mermaid module stopped during its
browser DOMPurify setup, so this is an environment limitation, not a syntax
result. No C++ build, test rerun, CI, 2010 inference, or Requirements Lab edit
was made.

Next handoff: visually inspect that callback diagram in the local Markdown
preview and record its layout separately from grammar acceptance and GitHub
rendering. Then continue with a bounded HLA-first survey for a genuine reader
gap; keep ordinary late-subscription §5.8 semantics open until an authorized
standard copy is available. Documentation-only; do not spend the implementation
worker's build capacity.

### 2025 callback chart preview attempt — 2026-10-10

The supported Codex file-open request for the callback guide returned
`queued`; it did not return a rendered preview. The computer-use surface then
reported no native application targets and an empty Codex in-app browser. Its
other browser tabs were unrelated and were left untouched. The bundled VS Code
Mermaid module still cannot initialize its DOMPurify dependency in bare Node.
These are preview/parser-environment limitations, not evidence that the chart
is valid or invalid. Grammar and visual verification remain pending; no
alternate browser or loopback workaround was used.

Next handoff: use the supported local Markdown preview when a targetable
preview surface is available, and record grammar, local layout, and GitHub
rendering as separate checks. Until then, keep the callback chart visibly
pending and continue only with bounded HLA concept work that does not depend on
that preview. Preserve the unresolved ordinary late-subscription §5.8 wording
until an authorized standard copy is available. No C++ builds, tests, or CI.

### 2025 Join during save/restore boundary — 2026-10-10

The federation save/restore guide now records a bounded contract/evidence
comparison for Join while either protocol is active. Both pinned 2025 Join
overloads list `SaveInProgress` and `RestoreInProgress`; the inspected embedded
membership path and process-service path are called out separately. The
exception annotations and source review do not settle exact admission
conditions or participant-set effects, so the guide makes no standard outcome
or conformance claim. Resolve those details from §4.11 and the applicable
save/restore clauses when an authorized standard copy is available. No build,
test rerun, CI, 2010 inference, or Requirements Lab edit was made.

Next bounded topic: survey the 2025 FOM-module admission/composition guide for
a distinct HLA-facing concept gap before adding any chart. Keep this review
documentation-only; retain the open callback-preview and late-subscription
items without using unrelated browser surfaces or inferring standard text.

### 2025 FOM model-edition and RTI-interface boundary — 2026-10-10

A bounded survey found that the FOM guide already distinguishes module
validation, complete-set composition, FDD materialization, and Create/Join
commit boundaries, so no duplicate composition chart was needed. One useful
reader distinction was only stated in prose: model edition is separate from
the RTI interface edition. Added a small diagram showing that the opt-in
`fomEdition=2010` path builds a 2010 catalog under the 2025 API; it is not a
2010 RTI path, mixed-edition composition is rejected, and this compatibility
slice produces no 2025 FDD or MOM projection. The diagram is supported by the
edition type/composer branch and bounded external 2010 tests for catalog
composition, mixed-edition rejection, and selected 2025-API lookup,
publication, and registration. It claims no standard equivalence or general
simulation. Mermaid grammar and layout remain pending because the local
preview/parser surface is unavailable. No builds, test reruns, CI, or
Requirements Lab edits were made.

Next bounded topic: survey the 2025 service-invocation/reporting guide for a
distinct HLA reader gap around service outcomes and asynchronous reports; do
not redraw an existing exception/callback path. Keep 2010 and 2025 evidence
separate and reuse current test results.

### 2025 service outcomes and report-path survey — 2026-10-10

The bounded survey found that the service-invocation guide already covers
subject versus observer, independent reporting switches, sink selection,
eligible-receiver routing, callback-time revalidation, and several service-
specific commit/report boundaries. The cross-cutting guide separately maps a
caller result, committed state, report, and observer visibility; the exception
guide has a decision map distinguishing `HLAreportServiceInvocation`,
`HLAreportException`, and `HLAreportMOMexception`, plus the combined process
report-path failure boundary. A new chart would duplicate those existing
models. Clarified the service guide's scope: its exception-report mentions are
selected companion traces, with a direct link to the full exception-reporting
guide. No behavioral claim or test result was added; no builds, test reruns,
CI, 2010 inference, or Requirements Lab edit was made.

Next bounded topic: survey the 2025 federation/federate lifecycle guide for
reader-facing separation between execution lifetime, member lifetime, and
observable lifecycle callbacks. Reuse current source/test evidence, preserve
2010/2025 boundaries, and add no diagram unless the survey finds a real gap.

### Federation and federate lifetime boundary survey — 2026-10-10

The 2025 lifecycle guide already makes three scopes explicit—RTI ambassador
connection, federation membership, and federation execution—and gives each
its own scope map/state view. It also traces Create/Join/Destroy, voluntary
ResignAction, forced connection-loss cleanup, asynchronous member-list
snapshots, survivor-visible effects, and the zero-member-but-still-existing
execution state. Its source/test table keeps selected embedded evidence and
known callback/test gaps distinct. The separate 2010 reference guide has its
own connection/membership/execution model and explicitly different limitations
(including its local callback behavior and ignored ResignAction); no 2025
transition was projected onto it. These guides already cover the survey's
reader question, so no duplicate lifecycle diagram was added. No builds, test
reruns, CI, or Requirements Lab edits were made.

Next bounded topic: compare the 2025 federation MOM and joined-federate MOM
guides for a distinct explanation of execution-scoped versus member-scoped
state, update sources, and observation callbacks. Preserve their separate
lifetimes and profile/evidence limits; add a diagram only if a genuine gap
remains.

### 2025 federation and joined-federate MOM scope survey — 2026-10-10

The bounded comparison confirms the guides explain different HLA object
lifetimes rather than duplicate one another. The federation MOM guide follows
the execution-scoped `HLAfederation` object, membership-driven conditional
values, subscription/reflection discovery, and request/report interactions.
The joined-federate MOM guide follows one member's `HLAfederate` snapshot,
join-scoped values, current-value requests, periodic refresh, save/restore
state, and resignation removal. They distinguish attribute reflections from
report interactions and keep callback observation separate from service
completion. Their existing diagrams, MIM references, and bounded evidence
tables cover the reader question; no duplicate diagram was added. The separate
2010 reference guide remains outside both 2025 MOM models. No builds, test
reruns, CI, or Requirements Lab edits were made.

Next bounded topic: survey the 2025 attribute-value update request and
object-attribute subscription-state guides for the HLA distinction between
subscription/knowledge, eligibility to receive values, explicit requests for
current values, and ordinary value updates. Preserve the unresolved §5.8
late-subscription wording until an authorized standard copy is available;
add a diagram only if the reader path is not already clear.

### 2025 subscription state and explicit value-request survey — 2026-10-10

The guides already draw the important HLA boundary. Subscription declarations
are per attribute (with active/passive and regional state), influence knowledge
and value-reception eligibility, and may lead to discovery, initial-value,
scope, or provider-advisory effects. An explicit object/class/regional request
instead solicits current providers; the callback is not the value, and a later
provider update independently passes normal ownership, subscription, DDM,
order, and time gates before a receiver can observe a reflection. The request
guide also separates pending provider work, already delivered receive-order
responses, and queued timestamped responses across restore. Existing diagrams
and cross-links cover these distinctions, so no duplicate chart was added.
Ordinary late-subscription initial-value behavior remains an open §5.8
question: embedded source shows its path, while the process source/test review
does not settle ordinary initial values or normative semantics. No standard
claim was inferred and no builds, test reruns, CI, or Requirements Lab edits
were made.

Next bounded topic: survey the 2025 attribute-ownership and ownership
persistence/resignation guides for the HLA distinction between current owner,
ownership-transfer state, and what resignation or deletion makes observable.
Keep ownership state separate from callback delivery and preserve 2010/2025
and embedded/process evidence boundaries; add a diagram only for a real gap.

### 2025 ownership transfer, resignation, and deletion boundary survey — 2026-10-10

The [core attribute-ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md)
already follows requester-side acquisition, owner-side transfer paths, query
snapshots, and accepted-delete invalidation of queued ownership work. The
[persistence/resignation companion](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md)
separately traces selected pending work across save/restore and the
member-scoped reject-or-cancel preflight; the
[federation lifecycle guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md)
owns the broader `ResignAction` object/divestiture effects table and final-member
path. Their links and evidence limits make the boundaries clear, so no
combined diagram was added. The 2010 stream remains separate, and focused
tests/source establish only their named 2025 scenarios.

### Edition-separated data-element byte-order audit — 2026-10-10

The [2025 encoding guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) and its
[2010 companion](HLA-2010-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) already diagram
concrete-type-selected BE/LE value octets independently. The 2025 guide
explicitly says not to infer an element's order from process-frame order; the
2010 guide excludes transport framing and keeps its wrappers, composites, and
smoke evidence separate. The bounded source check confirms both sets of
wrappers pass an explicit order to the shared unsigned-octet primitive, while
the process frame has separate fixed-BE metadata helpers. Existing 2025
Catch2 vectors and 2010 smoke assertions anchor the labels. No new chart or
code change was needed; tests were not rerun.

Next bounded topic: survey the 2025 DDM and region-relevance guides for the
reader-level relationship among region state, region associations, matching
interest, committed changes, and observable discovery/scope/reflection
effects. Reuse existing evidence; preserve edition and embedded/process
boundaries, and add diagrams only for a distinct conceptual gap.

### 2025 DDM region-state and observer-effects survey — 2026-10-10

The DDM guide already gives a cohesive reader path from FOM dimensions and
class-available dimensions, through application-owned region specifications
and pending-versus-committed bounds, to source/receiver matching. It separates
object-attribute routing from interaction routing, default from explicit
region realizations, strict overlap from the profile's optional exact-touch
extension, and DDM scope from later time-management/callback gates. Its object
flows distinguish discovery, current attribute scope/relevance, advisories,
and value reflection. The linked relevance and Attribute Scope guides own
their separate publisher-, owner-, and receiver-facing callbacks. Their
existing charts and bounded source/test maps cover the proposed reader path;
no duplicate diagram was added. The survey makes no 2010 inference and does
not assert embedded/process parity. A supported local Markdown preview request
was queued but returned no rendered output, so diagram grammar/layout and
GitHub rendering remain unverified here. No builds, test reruns, CI, or
Requirements Lab edits were made.

Next handoff: visually inspect the DDM diagrams and the already-pending FOM
edition-axis and callback conceptual diagrams in a supported Markdown preview.
Record parser/grammar, local layout, and GitHub rendering as separate checks;
do not claim a render pass until the diagrams are actually shown. If no
preview surface is targetable, continue bounded HLA documentation work and
leave rendering visibly pending.

### 2025 Join effects across independent protocol boundaries — 2026-10-10

The lifecycle, synchronization, and save/restore guides already established
different parts of the Join story, but a reader had to synthesize them across
three files. Added a small cross-guide flow map to the
[2025 lifecycle guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#a-join-does-not-define-every-active-protocols-participant-set):
after a successful Join, the embedded 2025 synchronization hook expands only
pending points whose synchronization-set argument was omitted; explicit-set
points, including explicit empty, do not expand. This is labeled as an Umbra
source observation, not a normative IEEE rule. The Join-during-save/restore
lane only records that both pinned 2025 Join overloads list `SaveInProgress`
and `RestoreInProgress`; exact preconditions and participant effects remain
open for §4.11 and the save/restore clauses. The map explicitly warns against
transferring synchronization-point behavior to save/restore or inferring 2010
or process-profile parity. Direct links now identify the embedded Join hook,
the registry's omitted-set flag and expansion gate, and the focused test. The
test covers omitted versus explicit non-empty sets, not explicit empty or
partial achievement before Join; no new runtime claim or test evidence was
added.

The whole current Mermaid set parses, but no supported Markdown render was
available. The new Join map and previously pending DDM/FOM/callback diagrams
still need visual review. No C++ build, test rerun, CI, or Requirements Lab
edit was made.

### 2025 periodic MOM refresh and explicit-request independence — 2026-10-10

The joined-federate MOM guide now separates two different HLA reasons values
may be supplied: an explicit Request Attribute Value Update and policy-driven
periodic refresh. It explains that `HLAsetTiming` configures the cadence for a
specified joined federate, zero disables that target's periodic refresh, and
the normal callback path still determines application observation. A new
sequence diagram uses the focused embedded 2025 multi-target test to show that
changing one target's period does not stop the other target's schedule. The
pinned 2025 MIM anchors the MOM semantics and units; source links identify
current decoding/state, but remain implementation evidence, not portable HLA
semantics. The paired test also covers explicit requests and invocation versus
distinct-object counts. No 2010 or process-endpoint parity is inferred, no
runtime tests were rerun, and no builds, CI, or Requirements Lab edits were
made.

The updated current inventory is 196 Mermaid diagrams across 32 chart-bearing
Markdown files (2010: 11; 2025: 183; index: 2), comprising 106 flowcharts, 68
sequences, and 22 state diagrams. VS Code 1.141.0 Mermaid core accepted all
196 blocks in a non-rendering parser pass. This is grammar evidence only; no
SVG or screenshot was produced and local layout/GitHub rendering remain
unverified.

Next handoff: visually inspect the new per-target MOM sequence, the Join-boundary
map, and pending DDM, FOM edition-axis, and callback conceptual diagrams in a
supported preview. Check local layout and GitHub rendering separately. If the
preview remains unavailable, continue bounded HLA-first documentation work and
leave rendering visibly pending; keep §5.8 late-subscription and §4.11
Join-during-save/restore questions open. Do not run C++ builds, tests, or CI.

### 2025 cross-cutting state, event, and observation guide audit — 2026-10-10

The new [cross-cutting reasoning guide](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md)
provides a portable HLA reading lens rather than another RTI architecture or
universal service state machine. Its diagrams separate authority/admission,
state and ownership, recipient eligibility, and application observation or
completion; they also distinguish model vocabulary from live declarations,
participant-local knowledge from federation facts, protocol-specific
completion, and caller result/state/report effects. Its service examples link
back to focused guides rather than absorbing their rules. The guide is
explicitly 2025-only, adds no normative interpretation or clause mapping, and
does not claim 2010 or embedded/process parity.

The current whole-docs VS Code 1.141.0 Mermaid-core parse accepted 196/196
diagrams, including the guide's eight charts. A bounded link check found all
58 local targets present and all 25 line-number anchors within their target
files; this verifies path/line bounds, not anchor meaning, visual layout, or
conformance. No supported preview was available, so local rendering and
GitHub rendering remain separate pending checks. No C++ builds, tests, CI, or
Requirements Lab edits were made. The new guide is currently untracked in the
worktree despite being linked from the reader map; preserve it and include it
with the index/backlog at a deliberate staging checkpoint, without staging
unrelated in-flight documentation.

Next bounded review: survey the separate 2010 reference-RTI and encoding
guides using the portable state/event/observation questions, relying only on
2010 source and test evidence. Identify a genuine 2010 reader gap or record
why existing coverage is sufficient; do not translate a 2025 protocol or
implementation observation into the 2010 stream. Keep the outstanding 2025
visual-preview checks visible, and do not run C++ builds, tests, or CI.

### 2010 cross-cutting lens boundary survey — 2026-10-10

The separate [2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md)
already applies state/event/observation reasoning within its actual bounded
profile: it distinguishes connection and execution membership, object
existence from receiver knowledge, interaction delivery, synchronization
barrier state, direct callback observation and local exception conversion,
and unavailable time-managed execution. The separate
[2010 DataElement guide](HLA-2010-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) covers
its own typed value and byte-order boundaries. These are source-backed 2010
descriptions, not parallels to the 2025 synchronization, queue, or callback
models. The binding-shell smoke establishes one successful HLA_IMMEDIATE
scenario; synchronization behavior and callback-failure/retry effects remain
source-only or explicitly untested. A second 2010 cross-cutting diagram would
repeat the existing guide's boundary map and risk implying unsupported
services, so no duplicate was added. Keep the portable reasoning guide
2025-only and retain the 2010 test gaps as gaps.

This bounded source/documentation audit did not run C++ tests, builds, or CI.
It adds no 2010 normative claim and does not change the current 196-diagram
inventory. Visual review remains pending for the cross-cutting and listed
2025 diagrams; parser acceptance is not a render pass.

Next bounded review: compare the 2025 API-family reader map with the current
guide headings and evidence boundaries to find a genuinely missing HLA
behavior area, not just an API-method mention. Keep 2010 on its separate map,
do not transfer protocols or test results between editions, and do not run
C++ builds, tests, or CI.

### 2025 service-family survey and logical-time factory bridge — 2026-10-10

A reader-path survey mapped the eight public service families in the pinned
2025 `RTIambassador` interface to the current HLA-first guides. It found no
missing service family or whole behavior area. One useful conceptual link was
too indirect: readers could find logical-time value encoding and time
advancement separately, but not the distinction between an execution's
selected logical-time representation and a member's evolving logical-time
state. The Time Management guide now connects those ideas and the API-specific
`getTimeFactory()` path. Its central lesson is about typed time values versus
clock/protocol progress; it is not a claim that the factory itself grants or
advances time.

The guide uses the existing public-float64 focused test as a bounded case
study: it observes the selected factory, confirms logical time remains at the
initial value while an advance is pending, and then observes the grant and
updated value. The accompanying source trace is explicitly the embedded
profile. Neither this source/test trace nor the new diagram asserts process
endpoint parity, 2010 behavior, or a new normative rule. The 2010 map and
guides remain separate, and no Requirements Lab edits were made.

At this survey the static inventory was 197 diagrams across 32 chart-bearing
Markdown files (2010: 11, 2025: 184, index: 2). The non-rendering VS Code
1.141.0 Mermaid-core parse accepted that snapshot as grammar; it did not
verify visual layout or GitHub rendering. The current inventory is reconciled
in the audit below. No C++ builds, test reruns, CI, or Requirements Lab work
were performed. Existing dirty/untracked guide work remains unstaged; do not
stage unrelated documentation.

Next bounded review: survey the 2025 DDM guide for a durable reader-facing
distinction that is not merely a list of region API calls—especially how
region bounds, association/subscription state, and delivery eligibility
relate. Add or revise a flow only if a real conceptual gap remains, and label
the relevant embedded/process evidence boundary. Keep the 2010 stream
untouched and do not run builds, tests, or CI.

### 2025 DDM reader-model coverage audit — 2026-10-10

The existing [DDM and regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
already gives a reader-centered path through the durable HLA concepts that
motivated this review: FOM dimension declarations, class-available dimensions,
application-chosen region dimensions and ranges, committed region snapshots,
source/receiver overlap, object-attribute relevance and discovery, interaction
routing, and the separate time-management callback gate. It also explains why
object-attribute and interaction subscription selection must not be collapsed
into one rule. The diagrams lead with those concepts; Umbra source and focused
tests are bounded evidence, not the diagram's subject.

No new DDM diagram was justified: the requested bounds/association/subscription
relationship is already explicit, with the 2025 embedded/process evidence
limits and the optional Umbra relaxed-touch policy called out separately from
portable model vocabulary. The documented open branches (such as full
inheritance combinations and interaction subscription replacement) remain
bounded follow-ups rather than invitations to infer universal rules. This was
a documentation audit only; no C++ build, test rerun, or CI was started, and
the 2010 stream was not changed.

The synchronization-point question in the preceding handoff was already
covered by the current guide and its earlier completion audit: participant
selection, announcement, per-member results, one-member completion, failed
results, resignation, and callback dispatch are distinct. The advisory-guide
survey also found the relevance guide's signal table and fan-out diagram,
Attribute Scope guide, and update-rate admission guide already separate
publisher/owner/receiver advisories from actual value eligibility. No duplicate
overview was added.

The current worktree also contains a focused sequence for a queued regional
TSO interaction when its receiver replaces the regional subscription. Its
existing embedded test and source path support the bounded distinction: the
accepted source-region snapshot is retained, current receiver eligibility is
checked at delivery, and later sends use the replacement subscription. This
trace is not a normative or cross-profile claim. It is included in the current
205-block Mermaid grammar pass; visual layout and GitHub rendering remain
unverified.

Next bounded review: inspect the updated DDM path in a supported Markdown
preview and verify GitHub rendering. Keep those visual checks distinct from
the completed grammar pass. If preview support is unavailable, leave rendering
explicitly unverified and move on to the next distinct HLA reader gap using
existing source and test evidence. Preserve 2010/2025 and embedded/process
boundaries, and do not run C++ builds, tests, or CI.

### 2025 identity and value-snapshot reader-model review — 2026-10-10

A bounded comparison of the [names and handle guide](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md)
and [object/interaction information guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md)
found no distinct reader gap for another overview. The names guide already
separates execution-model symbols, federate-local known-object state, and
federate-name lifetime; it also distinguishes typed handles, handle encoding,
hash values, and Umbra's execution-scoped normalized coordinates. Its
process-endpoint identity observations are labeled separately from embedded
coverage. The object/interaction guide already diagrams caller-input copying,
revalidation, recipient projection, and later callback eligibility for an
embedded 2025 receive-order attribute update. Those are deliberately separate
service/profile stages, not a universal “all HLA values are snapshotted this
way” rule.

The bounded source/test review did not establish a public update/send test that
mutates the caller's input container after the service returns. The separate
`VariableLengthData` value-copy tests cover that type's storage semantics, not
the public service's complete acceptance contract. Keep that distinction
visible rather than promoting a source path into a normative HLA rule or
repeating the existing diagrams. This survey made no 2010 claim and changed no
C++ source, tests, CI, or Requirements Lab content.

### 2025 regional-interaction context and rejected-operation effects — 2026-10-10

The focused
[`mixed_region_interaction_validation_catch2.cpp` cases](../../cpp/tests/mixed_region_interaction_validation_catch2.cpp#L120)
provide two independent embedded 2025 `HLA_EVOKED` examples of the same
class-scope constraint. A valid region can contain a dimension not available
to the selected interaction class, so region validity alone is insufficient.
The send case supplies both an available-dimension region and a valid region
with an unavailable dimension; `InvalidRegionContext` is thrown and draining
the receiver observes no delivery. A later send using only the available
dimension is observed once. The regional-subscription case rejects its
unavailable-dimension region, then verifies a later send does not behave as if
the rejected declaration were active; subscribing with an available-dimension
region makes the next matching send observable.

Added a flow in the DDM guide to keep the sender and receiver service effects
distinct while teaching the portable question: “Are all dimensions in this
region set available to this interaction class?” The test comments map the
constraints to §§9.1.3.3 and 9.1.4. The full normative text is not in the
checkout; the guide therefore records the test's narrow case and makes no
general all-or-nothing or rollback claim. Source was inspected, not rerun.
This is selected embedded 2025 evidence only; no process or 2010 parity is
claimed. The new chart still needs a permitted grammar pass and actual rendered
review.

A bounded source trace adds implementation context without changing the HLA
explanation: the send planner can accumulate temporary local snapshots before
a later region fails, while the ambassador stops before recipient delivery; the
regional-subscription path checks dimensions before changing its declaration
map. The guide states only the externally observed test outcomes and does not
generalize those internal orderings into an HLA transaction guarantee. No
build, test, or CI work was needed or performed.

Next concrete handoff: continue the long-running documentation goal by choosing
a durable HLA behavior question not already answered by the guides. Explain
that portable rule first, then use Umbra source/test evidence only to clarify
one bounded realization and label any implementation-only nuance. Preserve
2010/2025 and embedded/process boundaries. Do not compile, rebuild, run C++
tests, or CI for this work. As secondary housekeeping, after the active
implementation thread stops editing `ROADMAP-INDEX.json`, reconcile its stale
diagram count with the current 202-block inventory. Then grammar-check the three blocks added after the prior
205-block pass: the two regional-update/available-advance sequences and the
interaction-region context flow. Keep actual visual review and GitHub rendering
separate; the prior preview request returned no rendered output, so do not claim
visual verification unless the diagrams are actually shown. Continue from a
concrete learner question or newly identified HLA concept not already
represented by the topic guides. Reuse the service-family map and existing
evidence; do not add a diagram merely to grow the inventory. Preserve
edition/profile boundaries and do not run C++ builds, tests, or CI.


### 2025 name, handle, and model-catalog lookup review — 2026-10-10

A bounded review of the existing 2025 name/handle guide found that its encoding
flow began with a generic “Typed handle” node but ended at
`decodeAttributeHandle` and `AttributeHandle`. That mismatch could make a
category-specific API look generic. The diagram now uses `AttributeHandle`
throughout as an explicit example; adjacent prose says each handle category has
its own public class and matching decoder. The pinned 2025 `Handle.h` defines
separate public handle classes and states that unequal handles may share a
hash; the RTIambassador API supplies the matching `decodeAttributeHandle`
operation. This is API-documentation evidence, not an Umbra-specific numeric
identity claim. The flow's syntax shape did not change, but the revised label
has not been rendered again. No code, test, build, CI, or Requirements Lab
change was made.

### 2025 save/restore Join boundary evidence check — 2026-10-10

The 2025 API lists `SaveInProgress` and `RestoreInProgress` on both Join
overloads, but that exception list does not supply the trigger or participant
effects. A bounded read of the embedded ambassador, process Join handler, and
shared registry membership path found no explicit active-save/restore predicate
in those paths. The representative [interlock test](../../cpp/tests/ieee1516_2025_representative_save_restore_interlock_catch2.cpp#L34)
joins its two participants before the save request, then tests other service
gates during save and restore; it does not attempt an in-flight Join. This
source/test audit therefore leaves the standard question open and makes no
conformance or profile-parity finding. The guide now records that test limit.
No C++ build, test, CI, or Requirements Lab change was made.

### 2025 additional-FOM Join to name-lookup bridge — 2026-10-10

The FOM admission guide and Names/Handle guide already covered their own
transitions, but a reader had to infer how an accepted additional-module Join
changes the lookup view of a member that was already joined. Added a bounded
sequence to the 2025 Names/Handle guide: an existing member resolves a base
class, another member joins with a compatible extension, and the first member
then resolves a newly contributed class while observing equality for its old
class handle. The focused embedded object-class test exercises this; a separate
interaction-class test exercises the parallel case. The sequence explicitly
labels the embedded profile and does not claim process parity, cross-execution
numeric identity, or any discovery callback. It links the FOM composition
guide's replacement-model/membership path instead of repeating that diagram.

The preceding bounded Join audit remains separate: save/restore exception
timing and membership effects are still open pending the full §4.11 text. No
2010 behavior is inferred. The added sequence has not yet had a grammar or
rendered review; no build or test was run.

### 2025 reservation and known-object visibility boundary — 2026-10-10

A bounded comparison found the underlying state distinction already represented
in the two guides, but their reader paths did not connect it. The reservation
guide covers a federate-specific right to register a name; the Names/Handle
guide covers receiver-local object knowledge after discovery. Added reciprocal
cross-links and one short explanation that reservation success is not object
discovery and that registration does not make every federate able to resolve
the instance.

The evidence remains deliberately split. The named-registration test observes
reservation success, owner registration, and a peer discovery callback delayed
until `HLA_EVOKED` delivery; it does not assert receiver name lookup in that
same scenario. A separate registration/discovery test observes
`ObjectInstanceNotKnown` before discovery and successful lookups after. These
are complementary cases, not a single end-to-end assertion. Existing diagrams
already express both lifecycles, so no combined chart was added. No C++ build,
test, CI, or Requirements Lab change was made.

The planned comparison of registered-object removal/name reuse with receiver
known-object cleanup and pending callbacks is completed in the dated review
below; see its evidence boundary and next handoff.

### 2025 object-removal, known-view, and name-availability review — 2026-10-10

The bounded comparison found that the reader-facing pieces already exist in
separate places: the 2025 object guide distinguishes execution-wide deletion,
each receiver's known-object transition, and callback observation; the
cross-cutting state/event guide explains operation-scoped invalidation; and the
name-reservation guide points its broad Registered-to-Available lifecycle edge
back to object deletion. A new all-purpose diagram would mostly duplicate those
flows, so none was added.

The useful conceptual frame is to ask three questions independently: is the
object/name still registered in the federation, does this particular federate
currently know the object, and has its application observed the corresponding
callback? These are distinct state/observation dimensions. The pinned 2025 API
separates receive-order and timestamped `deleteObjectInstance`, the
`removeObjectInstance` callback, and `localDeleteObjectInstance` into distinct
interfaces (§§6.16–6.18); that establishes the service boundaries, not the
precise timing or cross-service ordering. The [official IEEE edition page](https://standards.ieee.org/ieee/1516.1/6688/)
identifies the full standard as subscription access, so the licensed normative
text is still needed before turning a callback/name-reuse timing edge into a
portable HLA claim.

One 2025 embedded implementation illustrates why the three questions must not
be collapsed: accepting receive-order deletion clears the deleting member's
known-object entry and invalidates selected pending work, while each other
known receiver retains its own removal reservation until its backend gate. The
registry clears that receiver's known-object entry at its gate, then purges the
deleted object/name only when the deleted-object purge predicate is satisfied;
named reservation and registration consult the name index. Thus local
unknown-state and name-index availability are not the same implementation
predicate. This is an Umbra-specific storage/cleanup observation, not a claim
that HLA requires the same deferred-purge strategy.

Evidence remains scenario-bounded. The [embedded deletion case](../../cpp/tests/object_instance_deletion_catch2.cpp#L163)
observes the deleter as unknown after the service returns, a peer as known
before its evoked removal callback, and the peer as unknown afterward. The
[ownership-query case](../../cpp/tests/attribute_ownership_query_catch2.cpp#L359)
shows the selected removal path suppressing stale ownership reports. The
[reservation case](../../cpp/tests/object_instance_name_reservation_catch2.cpp#L149)
tests reservation conflicts and release, but none of these cases attempts
reservation or named registration in the interval between deletion acceptance
and the last receiver-removal gate. Source pointers are in the object guide's
deletion evidence map. No tests were rerun, and no C++ build, CI, 2010 source,
or Requirements Lab change was made.

Next handoff: keep the main worker's build lane free. If the exact 2025
name-reuse/removal timing becomes important, first verify §§6.7 and 6.16–6.18
against licensed normative text; do not fill the gap from Umbra's purge logic.
Otherwise, continue with a learner-facing HLA timing or state question not
already answered by the current guides, and add a diagram only for a genuine
portable reader gap. Keep edition and implementation-profile boundaries
explicit; documentation work does not call for C++ rebuilds or tests.

### 2025 support-switch control-surface survey — 2026-10-10

A bounded comparison of the 2025 public RTIambassador support-switch methods,
the FDD parsing/Join paths, and the DDM, time, and lifecycle guides found a
reader gap across otherwise documented service-specific effects: the word
“switch” hides different authorities and lifetimes. Added a portable reading
map to the [cross-cutting state/event/observation guide](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#a-support-switch-has-an-owner-a-lifetime-and-a-control-surface).
It asks who supplies a value, whether the public API exposes a setter, when it
is established, and which later participant/service behavior it can affect.
The map distinguishes (1) selected getter-only FDD policies, (2) an FDD-seeded
per-member automatic-resign directive that the member can override, and (3)
member-local runtime getter/setter controls. This is not a universal taxonomy
for every HLA switch or a claim that every setting shares one persistence rule.

The pinned API shows `getAllowRelaxedDDMSwitch` and
`getNonRegulatedGrantSwitch` as getter-only (§§10.55–10.56), while exposing
setters for `ConveyRegionDesignatorSets`, `AutomaticResignDirective`, and
member reporting controls (§§10.42–10.51). Umbra's FDD composer reads the
selected static values, and the Join path seeds the automatic-resign value for
each member. The existing [support-switch test](../../cpp/tests/support_switch_state_catch2.cpp#L4)
observes that changing one member's directive/reporting/convey-region settings
does not change its peer's values. The existing [NRG test](../../cpp/tests/ieee1516_2025_non_regulated_grant_switch_catch2.cpp#L4)
shows a later additional module does not change that federation's creation-time
value or release the already-pending TAR. The existing [relaxed-DDM case](../../cpp/tests/allow_relaxed_ddm_object_attribute_catch2.cpp#L102)
compares both members' getter values and demonstrates the selected touching-
range behavior. These examples are 2025 embedded evidence; no process or 2010
parity is inferred. Existing operation-specific guides remain authoritative for
the actual DDM, grant, resign, advisory, and reporting rules.

The new flowchart brings the verified static inventory to 204 diagrams (2010:
11 kept separate; 2025 guides: 191; two index maps). The inventory comprises
109 flowcharts, 73 sequences, and 22 state diagrams across 32 Markdown files.
Its grammar has not yet been checked, and no SVG, screenshot, or GitHub render
was produced. The documented earlier 205-block whole-set parse is from a
different snapshot and does not cover this chart or the two regional-update/
available-advance diagrams. No C++ source changed;
no build, test, CI, or Requirements Lab work was performed.

The save-image comparison is complete; its source-only evidence and remaining
normative question are recorded in the dated audit below.

### 2025 support-switch save-image boundary — 2026-10-10

Compared the existing switch-control map with the 2025 embedded state-image
capture and restore paths. Capture places selected federation control bits
(including Non-Regulated Grant and Allow Relaxed DDM) on the federation image,
and selected member switch bits plus `automaticResignAction` on each member
record. Restore writes the federation values back and applies member values
only after matching the saved member's identity metadata against a live member.
The cross-cutting guide now records this as a source-observed storage map,
separate from the HLA control/lifetime model.

A bounded search of the federation save/restore and state-image codec tests
found no focused assertion that the corresponding public getters expose those
values after restore. Source assignments do not establish that external
observation, nor do they establish a portable 2025 normative rule. The durable
reader lesson is to separate control authority, RTI image ownership, and
post-restore observable behavior. The wider [save/restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md)
remains the protocol reference; no 2010 or process-endpoint parity is inferred.

No C++ source changed and no build, test, CI, or Requirements Lab work was
performed. The existing support-switch Mermaid diagram was not changed, so the
static inventory remains 204; grammar/render and GitHub visual review remain
pending as recorded in the backlog.

Next handoff: audit the 2025 save/restore Join/Resign membership boundary
against licensed §4.11 normative text before documenting a portable flow.
The public [IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
identifies the active edition but offers subscription access to the standard;
the public page is not clause evidence. If the exact normative text is not
available, preserve the open question and move to the next HLA-first backlog
survey rather than inferring the rule from Umbra's implementation.

### 2025 service-specific commit-grain survey — 2026-10-10

A bounded cross-guide survey found that the focused guides separately explain
important but contrasting transaction grains: multi-name reservation commits
an available per-name subset while bulk release is all-or-nothing; additional-
FOM Join prepares a whole replacement definition and commits it with new
membership; ownership outcomes are tracked per object-attribute. These are
selected 2025 service paths, not a shared HLA transaction guarantee. Added a
cross-cutting map and linked the existing deep dives and focused test evidence;
the new Mermaid diagram is not yet parsed or visually reviewed.

The §4.11 source audit did not uncover normative text: the official IEEE page
lists the 2025 edition but provides subscription access, and its public
metadata does not expose Join/Resign clauses. The pinned C++ API lists
`SaveInProgress` and `RestoreInProgress` for Join, while Resign lists a
different exception set; that contrast is useful evidence to investigate, not
a complete statement of the normative membership rule. The save/restore guide
already preserves the question without inferring either permission or
prohibition. No need to duplicate or weaken that section.

No C++ source changed and no build, test, CI, or Requirements Lab work was
performed. A stateful scan of fenced Mermaid blocks now counts 205 diagrams
across 32 Markdown files (110 flowcharts, 73 sequences, 22 state diagrams):
11 remain in separate 2010 guides, 192 are in 2025 guides, and two are index
maps. The older 205-block parser pass is a different snapshot; current syntax,
local visual review, and GitHub rendering of the new chart remain open.

The ownership/regional-declaration survey and its update-rate follow-up are
complete in the dated entry below; the backlog records the next contract-first
save/restore review.

### 2025 ownership, regional declarations, and update-rate boundary — 2026-10-10

Compared the commit-grain map with the ownership, subscription-state, DDM, and
update-rate guides. No additional flow diagram was warranted: the focused
guides already show per-object-attribute ownership outcomes, distinct
publisher per-instance update-region associations and receiver
class-attribute/region subscription entries, later overlap consequences, and
the path from applicable subscription designators to per-attribute callback
admission. The cross-cutting map links those boundaries rather than merging
their state machines.

The public 2025 API accepts an active/passive flag and an update-rate
designator for ordinary and regional attribute subscriptions; regional calls
use attribute/region pairs and return no per-pair result collection. In the
selected embedded implementation, active ordinary declarations and
overlapping active regional declarations contribute to a projected
attribute's rate selection, while passive regional entries do not contribute
to that delivery-rate choice. Focused callback evidence verifies independent
per-attribute filtering in a mixed-rate reflection. A separate public API test
queries passive regional rates; it is not the same observation as delivery.

The regional subscription guide also traces declaration generation into the
private admission-history key. Its source and unit-test chain imply that a
nonempty regional call containing empty region sets can leave pair state
unchanged while causing a new generation key and fresh rate history. No
public callback test isolates a preexisting rate window across that exact
call. Treat this as an embedded source/unit-test inference, not an HLA
contract, an end-to-end callback observation, or process-profile evidence.
Likewise, the implementation's steady-clock interval and aggregation policy
are not portable cadence guarantees. The authorized 2025 clause text needed
to settle those normative questions is not present in this checkout; the
public API and IEEE listing do not settle them.

This review reused existing documentation, API declarations, source, and
focused tests. No C++ build, test rerun, CI, Requirements Lab edit, or 2010
behavior claim was made. No Mermaid block was added or changed in this
increment, and the global inventory was not rescanned while the implementation
worker is editing the shared documentation tree.

The contract-first federation save/restore review is complete in the dated
entry below; the backlog records the next order/transport survey.

### 2025 federation save/restore responsibilities and participant boundaries — 2026-10-10

The current save/restore guide already leads with a portable responsibility
model: a federate checkpoints application-owned state; the RTI coordinates
federation-owned state; a participant's completion report is not the
federation-level terminal outcome. Its conceptual map is explicitly
orientation, not a normative participant sequence. The detailed save and
restore sequences then separate that HLA-facing model from the current
embedded coordinator, time gates, commit store, rehydration, and selected
process-endpoint callback scenarios. Status-query diagrams also distinguish a
read-only callback snapshot from the operation's completion signal.

A bounded read of the pinned 2025 callback interface, embedded save/restore
control paths, and existing public integration assertions supports the
document's evidence layering: participant status changes precede the
federation result in the selected cases; the last participant completion
precedes the embedded save commit/success path; request failure is separate
from a restore accepted and later failed. Those are API-shape and selected
implementation observations, not a full clause audit or proof for every
callback model. The existing worked traces further explain how a saved image
can retain queued timestamped work, but the internal image schema and queue
rehydration are Umbra mechanisms, not mandatory HLA architecture.

No additional diagram was needed. The existing responsibility map, participant
sequences, operation state diagrams, failure comparison, and worked timing
cases already cover the reader's HLA-visible milestones without requiring the
portable map to contain implementation storage or queue components. The
Join/Resign membership boundary during save/restore remains unresolved: the
pinned API exception list and bounded adapter/registry inspection do not
establish the normative participant-set rule, and no in-flight Join case was
found in the reviewed focused evidence. Keep §4.11 open until licensed clause
text is available. Do not infer permission or prohibition from the current
Umbra path.

This was a documentation/source/test review only. Existing test assertions
were inspected but not rerun; no C++ build, CI, Requirements Lab edit, 2010
behavior claim, or broad embedded/process parity claim was made. No Mermaid
block was added or changed, and the global diagram inventory was not rescanned
while the implementation worker remains active.

Next handoff: survey the remaining 2025 MOM statistics and periodic-counter
surface in the pinned MIM, source, tests, and current guides. Decide whether
it exposes a portable monitoring concept distinct from the already-documented
federation/member object lifecycles. Lead with the HLA model; keep Umbra's
counters, scheduling, and snapshot machinery as evidence or implementation
detail rather than required HLA architecture. Add a flow only for a
demonstrated reader gap. Reuse results without builds or test reruns, and
preserve 2010 and embedded/process boundaries.

### 2025 joined-federate periodic MOM measurement units — 2026-10-10

The pinned 2025 MIM declares 21 Periodic attributes on `HLAfederate`. The
joined-federate guide now groups all 21 by portable measurement meaning:
logical-time state, current RO/TSO queue lengths, service/reception totals,
object-lifecycle invocation totals, object-instance/ownership measures, and
wall-clock time in state. The inventory makes a practical reader distinction
explicit: a periodic refresh cadence does not make every value an interval
delta. In particular, invocation totals differ from per-instance counts;
queue lengths and delete privilege are current gauges; time-in-state values
measure elapsed duration since the prior attribute update.

The existing cadence diagrams cover how `HLAsetTiming` selects a target
member's periodic refresh and how that refresh reaches the ordinary callback
boundary. The missing information was the unit and semantics of each periodic
value, so a six-row MIM-linked reference table closes the reader gap without
another diagram. The adjacent focused update-count case grounds the
service-versus-instance distinction in the selected embedded profile. The
multi-target test remains evidence for independent target schedules only; no
process-profile parity or exact callback-latency guarantee is inferred.

This was a documentation update with a bounded read of the pinned MIM and
existing source/test evidence. No C++ source changed, no build, test rerun, or
CI was started, and no Requirements Lab or 2010 behavior claim was added. The
full Mermaid inventory was not rescanned because the implementation worker is
active; no diagram changed in this increment.

Next handoff: compare the 2025 `HLAfederation` statistics with the
joined-federate statistics now cataloged in the member-MOM guide. Establish
portable scope and measurement units from the pinned MIM first; use source
and existing tests only to label the selected embedded profile, never to
infer HLA-wide aggregation rules. Determine whether the federation-MOM guide
leaves a concrete learner gap before editing it. Do not build, rerun tests, or
start CI; preserve 2010 and embedded/process boundaries.

### 2025 federation-versus-member MOM measurement scope — 2026-10-10

A bounded enumeration of the pinned 2025 MIM found no Periodic attributes on
`HLAfederation`; its attributes are Static or Conditional. The current
federation-MOM guide already separates the execution-scoped object and its
conditional membership/FOM/save values from each joined member's private
`HLAfederate` object. I made that scope boundary explicit: `HLAsetTiming` and
Periodic counters belong to the member object, not the federation object.

This closes the natural federation-level counterpart to the member-statistics
inventory, but does not imply that conditional federation values are periodic
metrics or that Umbra implements every MIM attribute. Existing federation MOM
diagrams already distinguish object reflections from report interactions and
show the conditional value triggers. No new diagram was warranted. No source
or tests changed or ran; no C++ build, CI, Requirements Lab edit, or 2010
behavior claim was made.

Next handoff: review the 2025 names-and-handle guide for a junior-friendly
distinction between declaration names, federation/member-scoped identities,
category-typed opaque API handles, and encoded handle values. Start with the
pinned API/model contract; use Umbra's internal handle representation only as
clearly labeled implementation evidence. Revise only for a demonstrated
reader gap, with no builds, test reruns, CI, or 2010/2025 boundary crossing.

### 2025 names, scoped identities, and typed handles — 2026-10-10

The bounded review found the existing guide already teaches the key identity
domains: names in the joined execution's FOM, class/member lookup through the
declared hierarchy, federate-local known object instances, active-name versus
known-handle federate lookup, and execution-scoped normalization coordinates.
Its seven diagrams plus the category/failure matrix cover these distinct
lookup paths; adding another combined diagram would duplicate them.

One wording refinement makes the public C++ contract harder to confuse with
Umbra's private storage: the pinned handle API exposes category-specific
opaque classes, comparison, hashing, and encoding—not an arithmetic numeric
identity. The guide now says that directly and links the pinned `Handle.h`.
It still labels the embedded registry's numeric representation as private;
this does not imply that internal handles may never be arithmetic or prescribe
their implementation type.

No C++ source changed, no build or test was run, and no 2010 behavior or
process-profile parity was inferred. The Mermaid inventory was not rescanned;
no diagram changed in this audit.

Next handoff: review the 2025 DataElement/value-encoding guide for the
distinction between API-level data conversion, FOM-declared representation
and byte order, public handle encoding, and transport framing. Start with the
pinned HLA model/API; keep Umbra codec behavior as implementation evidence and
do not infer one serialization boundary from another. Revise or add a flow
only for a demonstrated learner gap; no builds, test reruns, or CI, and keep
the 2010 and 2025 streams separate.

### 2025 data-representation boundaries — 2026-10-10

The 2025 DataElement guide already explains explicit per-type byte order,
composite shape/alignment, logical-time factory encoding, and the separation
between value bytes and Umbra's private process-frame metadata. Its existing
charts teach those rules within each layer, but a reader still had to infer
which codec to use when several APIs all produce octets. Added a compact
boundary map that routes the question by representation: DataElement value,
public typed handle, Umbra process frame, or FOM/OMT model input. The prose
links each boundary to its existing guide/source map and explicitly limits
BE/LE reasoning to the selected DataElement representation.

This is a 2025 documentation map, not a claim that process framing or FOM
composition is part of the IEEE DataElement codec contract. The separate 2010
guide and implementation stream are unchanged. Mermaid syntax and visual
readability of the new map remain unverified; the repository-wide inventory
was not rescanned while the implementation worker is active. No C++ source
changed, and no build, test rerun, CI, or Requirements Lab edit occurred.

Next handoff: compare the 2025 DataElement guide with the object/interaction
information-flow guide. Determine whether a learner needs a small map from an
application's typed value to the HLA update/interaction service and onward to
receiver observation, while keeping routing and delivery rules separate from
encoding rules. Reuse existing API and test evidence; add no chart unless a
real gap remains. Do not build, rerun tests, or start CI; preserve the separate
2010 and 2025 streams.

### 2025 typed application value to HLA service boundary — 2026-10-10

The DataElement guide defines how an application value becomes value octets;
the object/interaction guide begins its service flows with already-formed
value maps. A bounded read of the pinned 2025 API confirms those maps pair
`AttributeHandle` or `ParameterHandle` with `VariableLengthData`, and that
the send services and corresponding callbacks carry those maps while the
user-supplied tag remains a separate argument. Added an HLA-first bridge map
from an FOM member's declared data type through a compatible DataElement and
the handle-keyed map to the update/send service and callback-side decoding.
The existing publication, ownership, subscription, DDM, and callback gates
remain in their focused routing guides.

This is a public API/value-boundary explanation, not a claim about RTI wire
framing, byte-for-byte preservation across an implementation, or when a
callback runs. No C++ source changed and no source build, test rerun, CI, or
Requirements Lab edit occurred. The new map's Mermaid syntax and visual
readability are pending; the global inventory was not rescanned while the
implementation worker is active.

Next handoff: after the implementation worker has finished editing the shared
documentation tree, run targeted Mermaid syntax and rendered visual reviews
of the new DataElement representation-boundary and handle-keyed value-path
maps, then reconcile the inventory. Continue from a concrete learner
question not already covered; keep HLA semantics primary, source/test
evidence scoped, 2010 and 2025 separate, and the C++ build lane free.

### 2025 discovery versus initial attribute values — 2026-10-10

A bounded learner-gap comparison of the object/information-flow,
object-attribute-subscription, and Auto Provide guides found this distinction
already covered without needing another diagram. The object path separates a
registered instance and producer knowledge from the receiver's later discovery
and known-object state. The subscription guide separately distinguishes
discovery, reflection of currently held eligible values for a newly known
receiver, owner-facing relevance advisories, and Auto Provide solicitation.
It labels post-discovery initial reflection as embedded 2025 source behavior,
not a total callback-order guarantee; process-profile late-discovery behavior
and the normative §5.8 ordering remain open. The focused embedded regional
routing test is cited there for the stored-value example.

Disposition: do not add a standalone chart or rewrite these flows; that would
duplicate existing coverage. No C++ source was changed, and no build, test,
CI, or Requirements Lab work was performed. The main implementation worker
remains active, so the existing next handoff—targeted review of the two pending
maps after shared-doc editing settles, then a new gap survey—remains in force.

### 2025 class-ancestry learner-gap review — 2026-10-10

A bounded review of the object/interaction guide and cross-cutting state guide
found the core class-hierarchy distinction already taught: object-class and
interaction-class chains are separate, and discovery, attribute projection,
ordinary interaction promotion, and directed target lookup ask different
questions. The guides explicitly reject one universal nearest-ancestor rule
and scope the observed lookup behavior to current 2025 implementation paths;
directed ancestor lookup lacks focused test evidence, and no complete normative
inheritance interpretation is claimed. A separate hierarchy chart would
repeat this coverage, so none was added. No source, build, test, CI, or
Requirements Lab change was made.

Next handoff remains: after the active implementation worker stops editing the
shared tree, visually review the two pending 2025 boundary maps, reconcile the
inventory, then survey a distinct portable HLA learner question.

### 2025 DataElement guide: standard-first framing — 2026-10-10

The guide's opening sentence had described value encoding primarily as
“Umbra's” API behavior. The pinned IEEE 1516.1-2025 `DataElement` header
directly exposes the application-facing `encode`, `encodeInto`, `decode`, and
`decodeFrom` operations, so the introduction now leads with that standard C++
value-encoding boundary. It then distinguishes handles, model input, and
private transport framing, with Umbra source and focused tests clearly scoped
as implementation examples rather than normative authority. No Mermaid block
changed. The targeted `git diff --check` passed; no C++ build or test ran.

Next handoff remains: once shared-tree editing settles, syntax-check and
visually review the pending DataElement boundary and handle-keyed value-path
maps; refresh the diagram inventory only if the documentation tree changed.

### Current Mermaid inventory snapshot — 2026-10-10

A read-only scan of `docs/design/*.md` found 207 Mermaid blocks across 32
diagram-bearing Markdown files (41 Markdown files total): 11 in the separate
2010 guides, 184 in 2025-specific guides, and 12 in the cross-cutting guide
and index. The diagram types are 112 flowcharts, 73 sequence diagrams, and 22
state diagrams. This is a fence/type inventory only; it does not establish
syntax validity or visual readability. The guide index now reflects this
207-block snapshot. Refresh that count after the active implementation work
only if the documentation tree changed.

The current environment exposes no `mmdc` command or repository Mermaid
package, and the computer-use surface returned no targetable desktop apps.
The two boundary maps therefore remain unparsed and visually unreviewed here;
no external renderer or upload was used. Next handoff: after shared-tree edits
settle, re-scan only if diagram-bearing Markdown changed, then use the
permitted local Markdown preview for syntax and screenshot review of those
two maps; if no preview window is available, keep the visual check explicitly
pending and request an available preview path rather than routing around it.

### HLA guide navigation check — 2026-10-10

The documentation landing page links to the HLA behavior index and the
portable cross-cutting guide. A bounded link-target comparison found direct
index links for all 33 `HLA*.md` topic guides in `docs/design`; the index also
links this review ledger. No additional guide list or navigation edit is
needed. The canonical path remains landing page → guide index → focused guide,
which keeps the 2010 and 2025 streams discoverable without duplicating their
catalog. No source/build/test/CI or Requirements Lab change was made.

### 2025 DDM guide: HLA-first orientation — 2026-10-10

The DDM guide already contains detailed model-to-region, overlap, object, and
interaction paths. Its opening nevertheless led with Umbra's current paths,
which made the implementation sound like the subject rather than a case study.
The introduction now starts with the portable HLA distinction between a
federate's data interest and the RTI's DDM-scope decision, and separates that
scope decision from time eligibility and callback dispatch. Umbra 2025 source
and focused tests remain bounded implementation examples; no 2010 equivalence
is implied. No diagram changed, so this does not change the Mermaid inventory.
No C++ source changed, and no build, test, CI, or Requirements Lab work was
performed.

Next handoff: after shared documentation edits settle, continue a bounded
learner-gap survey across the existing HLA guides for a portable concept that
is not already taught. Keep the standard's model primary, reuse only existing
source/test evidence for implementation examples, and do not start C++ builds,
tests, or CI for the documentation lane.

### 2025 object and interaction flow: HLA-first orientation — 2026-10-10

A bounded read of the guide's opening and topic map found its service flows
already separate object-instance knowledge, attribute ownership, interaction
parameters, recipient eligibility, and callback observation. The opening,
however, described the document as a map of Umbra's implementation. It now
starts with the portable HLA distinction between object-attribute updates and
interaction messages, and makes clear that publication, ownership, DDM, time,
and callback rules affect different boundaries. The selected 2025 source and
tests remain examples only; 2010 behavior is not inferred. No diagram changed,
so the inventory is unchanged. No C++ build, test, CI, or Requirements Lab
work was performed.

Next handoff: continue the bounded learner-gap survey from a service family
not already covered, and prefer a distinct HLA concept over another overview
chart. Keep source and existing tests as bounded evidence and leave the
implementation build lane free.

### 2025 FOM admission guide: distinguish model-validity layers — 2026-10-11

A bounded read of the FOM admission/composition guide found that its diagrams
already distinguish per-module schema validation, whole-model composition and
reference resolution, and FDD materialization. The opening framed that useful
model-layer distinction mainly as Umbra's Create/Join pipeline. It now starts
with the HLA purpose of a shared Federation Object Model and makes the
validity-layer distinction the learner-facing concept; the embedded 2025
pipeline remains a bounded example. The 2010 input/model lane stays separate.
No diagram changed and no Mermaid inventory update was needed. No C++ source,
build, test, CI, or Requirements Lab work occurred.

Next handoff: continue the service-family gap survey by comparing the current
guide map with the HLA Support Services model/handle lookup concepts. Look for
a learner question not already answered by the names-and-handle or FOM guides;
do not add another chart if their existing flows already make the distinction
clear.

### 2025 support-service identities: scope already covered — 2026-10-11

The bounded survey of the names-and-handle guide found its three central
identity scopes already explicit: model symbols belong to the joined
execution's composed FOM, federate name/handle lookups have distinct active
and retained-identity lifetimes, and object-instance lookup depends on the
calling federate's known-object state. The guide also already distinguishes
typed public handles from hash values, encoded handle data, and normalized
coordinates. A summary chart would duplicate the existing mental model and
lookup flows, so none was added. The opening now teaches the HLA identity and
lifetime distinctions before describing selected 2025 Umbra evidence; the
2010 stream remains separate. No diagram inventory change, C++ build, test,
CI, or Requirements Lab work occurred.

Next handoff: survey the 2025 attribute-value update-request guide for the
portable distinction between an owner-facing request/advisory and the later
value transfer. Compare with the object-information guide first; change only
if the existing flows leave a distinct learner question unanswered. Keep
implementation details as bounded 2025 evidence and avoid builds, tests, or CI.

### 2025 attribute-value update requests: solicitation versus response — 2026-10-11

The cross-guide check found the core HLA distinction already present in both
the object-information guide and the focused request guide: requesting a value
solicits a provider callback; any value travels in a later update, and a
receiver's reflection is a further, separately gated event. The request guide
already diagrams scope expansion, provider filtering/rechecks, the later
update, and receive-order versus timestamped reflection. No extra chart was
warranted. Its opening now leads with the portable solicitation-versus-transfer
model before identifying Umbra's 2025 path as the example. No diagram inventory
change, source build, test, CI, or Requirements Lab work occurred.

Next handoff: survey the 2025 attribute-ownership guide for the portable
distinction between attribute ownership as authority to update and the
service-specific acquisition/divestiture protocols that change that authority.
Keep RTI callbacks and Umbra's pending-work machinery as separately labeled
implementation detail; add no flow that duplicates the existing ownership
state machines.

### 2025 attribute ownership: authority versus transfer protocol — 2026-10-11

The guide already models ownership per object-instance attribute and separates
it from publication, subscription/discovery, delivery eligibility, accepted
requests, and callback outcomes. Its state and sequence diagrams also keep
acquisition, If Available, negotiated divestiture, unconditional divestiture,
and ownership queries on their distinct protocol paths. A new summary chart
would duplicate that coverage. The opening now states the portable HLA meaning
of ownership and distinguishes current authority from the protocols that can
change it; selected Umbra 2025 behavior stays example-only. No Mermaid block
changed, no inventory refresh was needed, and no C++ build, test, CI, or
Requirements Lab work occurred.

Next handoff: review the companion 2025 ownership-persistence/resignation guide
for a learner-facing separation between HLA ResignAction semantics and the
selected implementation's saved ownership ledger/callback-route cleanup.
Keep the unresolved normative membership/action edge explicit; do not infer
the 1516.1-2025 resignation matrix from Umbra source alone.

### 2025 ownership persistence/resignation: boundary review — 2026-10-11

The companion guide already describes itself as a selected 2025 implementation
case study and explicitly does not reconstruct the full ResignAction matrix.
Its three-layer model separates saved ownership-ledger facts, live callback
routes, and current membership; the diagrams keep those implementation states
distinct from save/restore status callbacks. The guide already preserves the
unresolved normative Join/Resign membership edge rather than inferring it
from Umbra source. No new chart or wording change is warranted until the
authorized IEEE 1516.1-2025 §4.11 text can settle that question. No build, test,
CI, or Requirements Lab work occurred.

Next handoff: perform a bounded coverage audit between the pinned 2025
RTIambassador service groups and the behavior-guide index. Identify any major
HLA service family with no reader path, then survey its standard concepts and
existing implementation/test evidence before proposing a guide. Keep the
2010 reference stream separate and do not run C++ builds, tests, or CI.

### 2025 RTIambassador family coverage and known-class lookup — 2026-10-11

A bounded comparison of the pinned public interface's six service-family
headings with the index's reader paths found no major family without a guide:
Federation Management, Declaration Management, Object Management, Ownership
Management, Time Management, and RTI Support Services all have paths. The
family map is appropriately not a method-by-method coverage claim. Within the
Support Services lookup family, however, the similarly named
`getObjectClassHandle(className)` and `getKnownObjectClassHandle(instance)`
represent different scopes: a joined-model declaration versus the calling
federate's known-instance view, with different not-found exceptions. The
names-and-handle guide now states that distinction in a compact API table and
connects it to its existing discovery flow and test. No new diagram or family
guide was warranted. No C++ build, test, CI, or Requirements Lab work occurred.

Next handoff: review the two 2025 MOM-reporting guides for whether their
portable HLA distinction between a service result and a separately observed
report leads the reader; do not add a summary diagram if the cross-cutting
guide already answers the learner question.

### 2025 MOM reporting: service result versus report observation — 2026-10-11

A bounded reader review found that the exception-reporting and
service-invocation-reporting guides already distinguish report types, member
switches, recipients, sinks, and callback delivery, but their opening
sentences led with Umbra's implementation path. Both now lead with the
portable HLA reader model: caller-visible result and state effect, report
production and routing, and observer receipt are separate questions. The
exception guide clarifies that no observed report does not establish service
success, because reporting and observation have separate gates; a received
report must be interpreted by its type. The existing type tables, protocol
guards, and operation-specific state flows remain unchanged. No Mermaid block,
source, or test changed; no C++ build, test, CI, or Requirements Lab work
occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025
authorization guide. Check that the portable Connect/authorization decision
and its caller-visible outcome lead, while Umbra's authorizer/profile wiring
stays labeled as implementation evidence; add no diagram unless a distinct
learner question is missing. Keep pending Mermaid parse/render checks as a
separate documentation-validation backlog and preserve the 2010/2025 boundary.

### 2025 authorization: operation admission versus service outcome — 2026-10-11

The bounded review confirmed that the guide already covers the three
authorization callback scopes, each operation's later admission and commit
gates, the pinned API's overload-specific exception declarations, and the
limits of the existing embedded test evidence. Its opening nevertheless led
with Umbra's four operation paths. It now leads with the portable reading of
the pinned HLA API: authorization admits a specific RTI, federation, or
federate operation; it neither guarantees the HLA service succeeds nor commits
that service's later state changes. The operation table and Umbra-specific
credential, profile, and exception-mapping detail remain below as a bounded
case study. No diagram, source, or test changed; no C++ build, test, CI, or
Requirements Lab work occurred.

### Correction: 2025 API service-family count — 2026-10-11

A direct heading check of the pinned
[`RTIambassador.h`](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L43)
shows seven named service-family sections: Federation Management,
Declaration Management, Object Management, Ownership Management, Time
Management, Data Distribution Management, and RTI Support Services. The file
also has a separate API-specific services section at
[`RTIambassador.h`](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L2294).
The earlier six-family summary omitted DDM; the index now states the correct
count and distinguishes the API-specific section. This corrects navigation
inventory only and does not claim method-by-method coverage. No C++ build,
test, or CI was run.

### Prior diagram inventory checkpoint (superseded) — 2026-10-11

At this checkpoint, a recount of fenced Mermaid blocks in `docs/design` found
207 diagrams across 32 diagram-bearing files (41 design Markdown files total).
The breakdown is 11 diagrams in 2 separate 2010 guides, 184 in 28
2025-specific topic guides, and 12 reader-model/navigation flowcharts in the
cross-cutting guide and index. The totals are 112 flowcharts, 73 sequences,
and 22 state diagrams. This is inventory evidence only, not grammar or render
validation. This count was superseded when the HLA-first subscription-interest
reader map was added later on 2026-10-11; the active index now records 208.

The `mmdc` CLI is not installed. A direct Node import of the installed VS Code
Mermaid preview bundle stopped before parsing with `ReferenceError: document is
not defined`; no syntax result, SVG, or screenshot was produced. Treat this as
a browser-context setup limitation, not a diagram failure. All six named maps
remain pending targeted parse and visual review. No C++ build, test, CI, or
Requirements Lab work occurred.

Next handoff: continue the content-first HLA learner-gap survey with the 2025
attribute update-rate guide. Keep subscription rate designators, owner-facing
rate advisories, and logical-time advancement conceptually separate; treat
Umbra's batching terms and callback filter as implementation evidence.

### 2025 update-rate admission: policy and delivery boundaries — 2026-10-11

The existing guide already documents the pinned update-rate API/MIM vocabulary,
per-receiver/per-attribute selection, best-effort admission, reliable/default
bypass, and the distinct RO/TSO delivery points with focused embedded evidence.
No new chart was warranted. Its opening now leads with the HLA/API distinction
between a subscription update-rate designator, the owner's Attribute
Relevance Advisory, the attribute update itself, and logical-time advancement.
It labels the detailed combination and admission behavior as selected 2025
embedded evidence; it also explains that `passel` is Umbra batching vocabulary,
not an HLA value or wire unit. No Mermaid block, source, or test changed; no
C++ build, test, CI, or Requirements Lab work occurred.

The map-preview limitation remains as recorded above, so the current pass
continued with content-first HLA review instead of repeating the failed
renderer setup.

### 2025 resignation: action choice versus evidence scope — 2026-10-11

The pinned API shows `ResignAction` as an explicit parameter to
`resignFederationExecution`; the guide already declines to reconstruct the full
normative effect matrix and scopes its save/restore and pending-acquisition
cases to current 2025 embedded evidence. Its opening now leads with the
reader-facing rule that a selected implementation scenario does not establish
the standard's effects for unexercised actions, then narrows to the ownership
work the guide actually traces. No normative action semantics were added, no
diagram/source/test changed, and the separate 2010 boundary remains explicit.
No C++ build, test, CI, or Requirements Lab work occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025
object-instance name-reservation guide. Check whether its diagrams clearly
separate synchronous service rejection from later callback outcome and bulk
partial reservation from all-or-nothing release; do not add a chart if those
distinctions are already easy to follow.

### 2025 object-instance name reservation: request result versus callback outcome — 2026-10-11

A bounded comparison of the pinned API and current guide found the core
learner distinction already clear: a void reservation call can return normally
while availability is reported later by one callback, whereas invalid
arguments/preconditions fail synchronously. The guide also distinguishes
partial-success bulk reservation from all-or-nothing bulk release and named
registration's reservation-consumption point. Its opening and diagrams are
already HLA-first with a separate 2010 boundary; the embedded/process details
remain explicitly bounded. No new chart or wording change was warranted. The
API declarations and existing focused-test map were inspected, not rebuilt or
rerun.

### 2025 object-attribute subscriptions: interest versus observable outcomes — 2026-10-11

A bounded learner review found that the guide's earlier opening led with the
embedded registry, even though its core lesson is HLA-facing: a subscription is
receiver interest, not a promise that every update is observed. Reframed the
opening around the pinned 2025 API's class, selected-attribute, active/passive,
and optional update-rate inputs, then added a concept map that separates
subscription interest, object discovery, per-attribute value eligibility, and
later routing/delivery gates. The map does not claim a full decision algorithm
or ordering guarantee. Implementation-specific mutation, test evidence, and
known process/normative gaps remain in the following sections. The 2010 stream
remains explicitly out of scope. One Mermaid flowchart was added; static counts
were updated, but grammar/render review remains pending. No C++ build, test, CI,
or Requirements Lab work occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025
order-and-transportation guide. Check whether it teaches the difference between
message order (receive order versus timestamp order) and transport choice
(reliable versus best effort) before showing implementation paths; keep logical
time/grant behavior in the time-management guide. Add or revise diagrams only
where they clarify that protocol-level distinction. Preserve the 2010/2025
boundary and do not run C++ builds, tests, or CI.

### 2025 order versus transportation: independent delivery properties — 2026-10-11

A bounded reader review found the existing guide already explains the central
protocol distinction: order selects receive-order versus timestamp-order
handling, while transportation selects a separate federation-declared
category. Its control table, default-capture flow, confirmation-callback state
chart, and delivery sequence already keep those settings apart from DDM
recipient selection and logical-time grant eligibility. The note that a
timestamped service overload may still produce receive-order delivery, plus the
explicit warning that tests do not establish wire-level QoS, address the most
likely reader traps. No extra diagram or new behavior claim was warranted.
Updated the guide's boundary note to point readers to the separate declaration,
attribute-relevance, and scope lifecycles, and to identify the next review. The
guide remains 2025-specific with its separate 2010 reference intact. No C++
build, test, CI, or Requirements Lab work occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025 federation
synchronization guide. Check that the participant set, announcement, each
federate's achievement result, and barrier completion are taught as distinct
protocol concepts before Umbra's omitted-versus-explicit set and late-join
details. Do not duplicate its existing fan-in diagrams; revise or add only for
a concrete reader gap. Preserve the 2010/2025 boundary and do not run C++
builds, tests, or CI.

### 2025 federation synchronization: HLA barrier before implementation — 2026-10-11

The guide already has a useful HLA-level barrier model and distinguishes the
current participant set, announced participants, achieved results, and the
failed-to-synchronize set. Its existing flows then label the narrower Umbra
rules for omitted versus explicit sets, late joins, resignation, callback
dispatch, and save/restore; source-only interleavings and missing focused cases
remain explicit. No additional diagram was justified. Reframed the opening to
start with the named HLA coordination barrier and its separation from logical
time and application transactions, then present Umbra transitions and evidence
as a bounded case study. The 2010 compatibility stream remains separate. No
C++ build, test, CI, or Requirements Lab work occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025 federation
and federate lifecycle guide. Check that ambassador connection, federate
membership, and federation-execution lifetimes stay distinct, and that
ResignAction implementation observations do not become an unsupported
normative effect matrix. Keep authorization, synchronization, and save/restore
state machines linked but separate. Revise only for a concrete reader gap.
Preserve the 2010/2025 boundary and do not run C++ builds, tests, or CI.

### 2025 federation lifecycle: resignation is not disconnection — 2026-10-11

The guide already separates ambassador connection, federation membership, and
execution lifetime, and its local/execution state diagrams preserve those
different transitions. The HLA-facing introduction had one misleading phrase:
it said a resigned member could "connect again" before joining a new membership,
even though the pinned API has separate Resign and Disconnect services and the
focused embedded scenario rejoins on the same ambassador without Disconnect.
Clarified that tested path and marked it as embedded evidence; a later Connect
is relevant only after a separate disconnect. The ResignAction table continues
to identify itself as a compact implementation aid, not a normative effect
matrix, and the zero-member execution rule remains explicitly 2025 embedded
behavior. No additional diagram was needed. The 2010 reference remains
separate. Updated the reader-path label so it no longer implies that Disconnect
must follow every Resign. No C++ build, test, CI, or Requirements Lab work
occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025 federation
save/restore guide. Check that the federate-owned application checkpoint is
distinct from the RTI-owned federation checkpoint, and local completion reports
from federation-wide outcomes, before the guide descends into image/state
implementation details. Keep synchronization and logical-time state separate;
revise or add diagrams only for a concrete reader gap. Preserve the 2010/2025
boundary and do not run C++ builds, tests, or CI.

### 2025 federation save/restore: responsibility before storage mechanics — 2026-10-11

A bounded HLA-first reader review confirmed that the guide opens with the key
responsibility boundaries: each federate owns its application checkpoint,
local completion is not the federation-wide result, and request admission is
not terminal success. The existing responsibility map is appropriately
labeled conceptual rather than a normative sequence, with embedded image,
commit, and process-callback detail kept in later, explicitly bounded views.
One phrase in the opening said the RTI "captures and commits" federation state,
which foregrounded storage mechanics as if they were part of the portable HLA
model. Reworded it to say the RTI coordinates saving RTI-managed federation
state and reports the federation-wide outcome; also removed an exact restore
ordering claim from the responsibility summary while keeping the standard as
authority for participant and callback-order rules. No diagram was added or
restructured, so the inventory count is unchanged and the existing six
parse/render checks remain separate. The 2010 stream and process/embedded
evidence boundaries remain intact. No C++ build, test, CI, or Requirements Lab
work occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025 core
attribute-ownership guide. Check whether ownership of attribute update/delete
rights is distinct from attribute values, publication/subscription, and the
negotiation protocol before Umbra's queues and callbacks. Do not duplicate the
persistence/resignation companion; revise only for a concrete reader gap.
Preserve the 2010/2025 boundary and do not run C++ builds, tests, or CI.

### 2025 attribute ownership: update authority, values, and delete privilege — 2026-10-11

A bounded HLA-first review confirmed that the core guide already starts with
per-object-attribute ownership, separates ownership from publication,
subscription/discovery, and delivery, and teaches accepted acquisition as
distinct from transfer. Its requester, owner, query, and deletion-invalidation
views already cover the main selected 2025 paths, while the persistence/
resignation companion owns those adjacent scenarios. One reader distinction
needed to be explicit: ordinary attribute update ownership is not the special
object-delete privilege. Added a short boundary note grounded in the pinned
2025 MIM's `HLAprivilegeToDeleteObject` declaration and the 2025 API's
`DeletePrivilegeNotHeld` exception, linking to the separate object-deletion
flow instead of duplicating it. No new diagram or behavioral claim was added;
the edit adds a reader-facing standards boundary, not a new Umbra transition or
callback claim. The 2010 stream and implementation-profile boundaries remain
separate. No C++ build, test, CI, or Requirements Lab work occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025
data-distribution and regions guide. Check that dimensions, regions, and
recipient relevance are taught separately from ownership and logical-time
ordering before Umbra's association, matching, and delivery paths. Avoid
duplicating the object-attribute subscription-state and time-management guides;
revise or add diagrams only for a concrete reader gap. Preserve the 2010/2025
boundary and do not run C++ builds, tests, or CI.

### 2025 DDM and regions: HLA scope before implementation routing — 2026-10-11

The current guide already leads with the portable DDM questions: which model
dimensions exist, which dimensions apply to a class, which ranges an
application places in its regions, and whether the applicable source/receiver
realizations match. It then keeps object-attribute and interaction routing
separate, distinguishes discovery/relevance from value reflection, labels the
exact-touch relaxation as Umbra-specific, and leaves timestamped eligibility
and callback dispatch to the time/callback guides. A focused inspection of the
mixed-advance regional-attribute test and queued regional-interaction
subscription-replacement test confirms their diagrams stay within their
embedded 2025 observations; no cross-profile or normative replacement rule is
inferred. The reader model is already complete enough; no duplicate overview
or DDM guide-content edit was warranted.

A fresh static fence count remains 208 diagrams in 32 files, including 14 in
this DDM guide. Corrected the prioritized-backlog row, which still said 12.
The last recorded whole-set grammar pass covered 205 blocks, so it cannot
establish syntax or visual readability for the current 208-block set. Preserve
the pending per-diagram parser/layout/GitHub checks; do not convert an earlier
local render into current GitHub verification. No C++ build, test, CI, or
Requirements Lab work occurred, and 2010 behavior remains separate.

Next handoff: perform a bounded HLA-first reader review of the 2025
authorization guide. Check the separation between credential presentation,
the three operation-scoped authorizer callbacks, each HLA service's own
preconditions/commit, and caller-visible outcomes. Keep Umbra profile wiring
and exception mapping labeled as implementation evidence; revise only for a
concrete reader gap. Preserve the 2010/2025 boundary and do not run C++ builds,
tests, or CI.

### 2025 authorization: admission gate versus service result — 2026-10-11

A bounded HLA/API reader review confirmed that the guide distinguishes
authorization from the operation's later success, but its only diagrams were
Umbra implementation flows. Added a portable API-level flow first: the pinned
2025 `Authorizer` interface has RTI-, federation-, and federate-operation
callbacks; an `AUTHORIZED` result clears only that gate, after which the HLA
service still applies its own preconditions and determines its state effect
and caller-visible result. The “other service” branch is explicitly limited to
this callback interface and does not claim other RTI admission policy is
absent. Kept RID selection, no-authorizer behavior, exception mapping, and
embedded/process observations in the bounded Umbra case studies.

Clarified that the guide's 7/10 focused CTest outcome is previously recorded
and was not rerun during this documentation review. The Connect `@throws`
annotation discrepancy remains open pending the full pinned §4.2 text; no
broader exception claim was added. The 2010 guide and behavior remain separate.
The static inventory is now 209 diagrams (114 flowcharts, 73 sequences, 22
state diagrams); eight maps, including this new authorization model, await
fresh syntax/layout review. No C++ build, test, CI, or Requirements Lab work
occurred.

Next handoff: perform a bounded HLA-first reader review of the 2025 federation
synchronization guide. Check that synchronization-point participation and
achievement, later callback/application observation, logical-time advancement,
and ordinary federation membership remain distinct concepts. Use the pinned
API and existing guide/source/test evidence; add or revise a diagram only for
a real learner gap, and do not infer 2010 behavior. Keep the eight pending
Mermaid syntax/layout checks visible. Preserve the 2010/2025 boundary and do
not run C++ builds, tests, or CI.

### 2025 synchronization: barrier, time, and membership are separate — 2026-10-11

A bounded HLA/API-first review found the guide already defines a synchronization
point as a named coordination barrier, explicitly says it does not advance
logical time, and separately labels the embedded 2025 registration,
late-join, achievement, resignation, and save/restore paths. The six protocol
diagrams already explain those mechanics; another overview would duplicate the
portable barrier model. Added a short comparison of the three distinct
questions and API paths: achievement for a named point, logical-time requests
and grants, and execution membership through Join/Resign. Linked the pinned
2025 API and the time-management/lifecycle guides. Clarified that a membership
change during a pending point is governed by synchronization rules and is not
an achievement report; implementation-specific effects remain labeled below.

No new diagram was warranted. The partial-result/late-join sequence remains a
source-derived scenario not exercised by the cited tests, and its visual review
is still pending. Reconciled it into the current syntax/layout backlog: nine
maps need current verified review, while the latest whole-set grammar result
covers a different 205-block snapshot. No C++ build, test, CI, or Requirements
Lab work occurred; the 2010 stream remains separate.

### 2025 order and transportation: delivery order is not transport category — 2026-10-11

A bounded reader review found the 2025 guide already distinguishes RO/TSO
order, transport selection, time-management eligibility, callback delivery,
and class defaults versus per-instance overrides. The missing HLA-level
anchor was the actual standard transportation vocabulary: the guide now names
`HLAreliable` and `HLAbestEffort`, links the pinned 2025 standard MIM, and
summarizes their reliable versus best-effort semantics. It keeps the MIM's
TCP/IP and UDP comparisons as category analogies, not claims about an RTI's
wire protocol, performance, or deployed QoS. Existing focused test/source
evidence remains unchanged and profile-bounded.

No new diagram was warranted: the four existing diagrams already explain the
default/override, pending-confirmation, delivery, and query paths. The local
transportation pending-state syntax repair still needs GitHub visual review;
this prose change does not alter the 209-diagram inventory or nine outstanding
syntax/layout reviews. No C++ build, test, CI, or Requirements Lab work
occurred. The 2010 stream remains separate.

### 2025 FOM admission: schema validity is not semantic content — 2026-10-11

The guide now states the portable boundary that IEEE 1516.2 defines object-
model format and syntax, not model content; DIF/FDD schema validation is
therefore structural, not a judgment that the selected federation model is
semantically appropriate. A targeted composer read found the existing
composition flow placed Umbra's same-edition/source-profile guard after the
Annex C merge, while the composer compares each module with the first module
before revalidation and merge. Reordered that gate, labeled it implementation-
specific, and linked the source check and existing mixed-edition test. The
separate 2010 compatibility lane remains catalog-only and is not treated as
the 2010 RTI stream.

No new diagram or test mapping was added. The modified composition chart still
needs syntax/layout and GitHub-render verification; it remains within the
existing FOM chart review item, so the inventory remains 209 diagrams and nine
tracked reviews. No C++ source, build, test, CI, or Requirements Lab work
occurred.

### 2025 attribute-value request: callback entry is not request completion — 2026-10-11

A bounded comparison confirmed that both the request guide and the object-
information guide already explain the central portable sequence: a request
solicits provider work, a later update transfers values, and reflection is a
separate receiver-eligibility result. The request sequence could still be read
as one shared call stack. Added a short boundary note: its arrows show causal
order, while the provider's selected `HLA_IMMEDIATE`/`HLA_EVOKED` callback
model controls application entry. Linked the callback/service-ordering guide
and pinned 2025 callback-model/Evoke API declarations. This does not change
the request-versus-transfer rule or claim that one request service synchronously
waits for the provider. The 2010 stream remains separate.

No new diagram, test mapping, build, test, CI, or Requirements Lab edit was
added. The 209-diagram inventory and nine tracked syntax/layout reviews are
unchanged.

Next handoff: perform a bounded HLA-first reader review of the 2025 exception-
reporting guide, contrasting the caller's service outcome with
`HLAreportException`, `HLAreportMOMexception`, and the separately configured
`HLAreportServiceInvocation` path. Check report switches and observer
eligibility without turning Umbra's selected hooks into universal HLA rules.
Reuse existing API/source/test evidence; add a diagram only for a genuine
learner gap. Preserve 2010/2025 boundaries and do not run C++ builds, tests,
or CI.

### 2025 exception reporting: service outcome and report subject — 2026-10-11

A bounded HLA-first review found that the guide already gives readers the
needed model: the service result, report production, report type, recipient
eligibility, and callback observation are separate questions. Its decision
map distinguishes public-service exceptions, rejected/malformed MOM requests,
and the separately configured service-invocation record; the route sections
then show only selected Umbra behavior. The pinned 2025 MIM confirms the
separate interaction declarations. It also contains a wording ambiguity: the
`HLAservice` parameter semantics nested under `HLAreportException` explicitly
describe `HLAreportMOMexception`. The guide points this out and defers
interpretation to the official artifact instead of treating Umbra's code as a
correction. No additional diagram or prose change was justified.

No C++ source, build, test, CI, or Requirements Lab change was made. The 2010
reference stream remains separate; the 209-diagram inventory and nine tracked
syntax/layout reviews are unchanged.

Next handoff: perform a bounded HLA-first reader review of the 2025
[connection and configuration guide](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md).
Check that factory construction, Connect, Join, Resign, and Disconnect read as
distinct lifecycle transitions, and keep API-defined caller-visible rules
separate from Umbra-specific configuration, RID, and teardown mechanics. Revise
only for a genuine learner gap; do not duplicate the federation lifecycle
guide. Preserve the 2010/2025 boundary and do not run C++ builds, tests, or CI.

### 2025 Connect and Disconnect: membership is not execution lifetime — 2026-10-11

The bounded comparison with the separate federation-lifecycle guide confirmed
that the detailed connection guide already leads with the local-session versus
Join distinction and appropriately keeps configuration/RID mechanics in the
Umbra case study. One wording gap remained: “left the execution” could make
Disconnect sound as though it ended the federation execution. The opening now
states the narrower boundary: Disconnect is distinct from Resign, the pinned
§4.3 API declares `FederateIsExecutionMember`, and the execution lifetime is a
separate scope. It links the three-lifetime model and its selected same-
ambassador rejoin example. No extra lifecycle diagram was added because the
companion already owns that state map.

No C++ source, build, test, CI, or Requirements Lab change was made. The 2010
reference stream remains separate. The diagram count remains 209; the nine
tracked syntax/layout reviews are unchanged.

Next handoff: perform a bounded HLA-first reader review of the 2025
[directed-interaction guide](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md).
Clarify how its explicit recipient set differs from ordinary interaction
subscription/DDM selection, and keep standard service semantics distinct from
Umbra's embedded/process evidence and callback timing. Reuse existing sources
and focused tests; add a diagram only for a real learner gap. Preserve the
2010/2025 boundary and do not run C++ builds, tests, or CI.

### 2025 directed interactions: target context is not receiver selection — 2026-10-11

The bounded comparison confirmed that the directed-interaction guide already
leads with the portable mental model: a send names one target object, while the
RTI selects zero or more receivers. Its companion comparison explicitly
contrasts ordinary class relevance/region matching with directed target
knowledge and the universal/by-ownership selector; it also warns that
ordinary `active` and directed `universally` are not synonyms. Existing flows
separate declaration validity, sender publication, recipient selection,
callback payload, receive-order callback-time rechecking, and TSO/retraction.
The embedded/process receive-order difference is identified as a source-level
observation; the process tests are not used to claim mutation-fence or
ancestor-declaration parity. No new diagram or prose edit was warranted.

No C++ source, build, test, CI, or Requirements Lab change was made. The 2010
reference stream remains separate. The inventory remains 209 diagrams with
nine tracked syntax/layout reviews.

Next handoff: perform a bounded HLA-first reader review of the 2025
[Auto Provide guide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md). Keep discovery-
triggered provider solicitation distinct from explicit Request Attribute Value
Update, initial reflection, and scope advisories; clarify which switch, owner,
scope, and timing gates are HLA concepts versus current Umbra profile evidence.
Reuse existing tests and label the unverified process path; add a diagram only
for a real learner gap. Preserve the 2010/2025 boundary and do not run C++
builds, tests, or CI.

### 2025 Auto Provide: initial reflection is a separate path — 2026-10-11

The HLA-first review confirmed that the guide already distinguishes automatic
provider solicitation, explicit Request Attribute Value Update, reflection,
and scope advisories, and labels the discovery planner and its callback-time
fences as current embedded behavior. The source comparison exposed one reader
gap: the discovery sequence jumped from `discoverObjectInstance` to Auto
Provide even though Umbra's embedded path handles eligible initial reflections
and initial attribute-relevance work before planning Auto Provide. Updated the
existing sequence and evidence table to show initial reflection as separate
data delivery, not the provider's response. The initial-value test exercises
that route independently; no combined ordering result is claimed. The process
discovery-triggered route remains unverified, and the 2010 implementation is
not used to fill the gap.

The diagram inventory stays at 209; no new Mermaid block was added. All three
current diagrams were rendered and visually reviewed in VS Code's Markdown
Preview on 2026-10-11. The switch chart was simplified to show only switch
initialization and transitions; discovery effects now sit in a compact table.
The discovery sequence includes the separate initial-reflection path, and the
wide sequence and tall eligibility flow remain readable while scrolling.
GitHub's post-publication rendering remains pending. No C++ source, build,
test, CI, or Requirements Lab change was made.

Next handoff: review the 2025 names/handle guide for a junior-friendly
distinction between declaration names, execution/member-scoped identities,
typed opaque API handles, and encoded handle values. Reuse the pinned API,
source, and existing tests; revise diagrams only for a real learner gap. Keep
the HLA API model distinct from Umbra's internal representation, preserve the
2010/2025 boundary, and do not run C++ builds, tests, or CI. Check GitHub
rendering of Auto Provide after publication.

### 2025 names and handles: seven existing views cover the identity boundaries — 2026-10-11

Reviewed the existing guide in the local VS Code Markdown Preview. All seven
diagrams rendered: joined-model name resolution; additional-module Join and
later lookup; federate name/handle lifetime paths; class-scoped inherited
member lookup; per-federate object-instance knowledge; typed `AttributeHandle`
encoding versus hashing; and normalization-service coordinates. The existing
prose and diagrams already distinguish model declarations, execution/member
identity, receiver-local instance knowledge, opaque typed handles, encoded
values, and execution-scoped normalization coordinates. No additional chart
or semantic rewrite was justified by this review. The parallel class and
normalization charts remain readable at preview width, and taller flows remain
readable while scrolling. This is local visual verification, not a fresh
standalone grammar audit or GitHub-render check. The guide's process-endpoint
post-resignation observation remains profile/test-bounded; no 2010 equivalence
is inferred. The inventory remains 209 diagrams. No C++ build, test, CI, or
Requirements Lab change was made.

Next handoff: review the new seventh, 2025-only boundary map in the
[DataElement encoding guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) for
local syntax/render readability. Keep its public-value, typed-handle,
FOM/OMT-input, and process-frame boundaries distinct, and keep the separate
2010 companion untouched. Reuse existing API/source/test evidence; do not run
C++ builds, tests, or CI. GitHub rendering remains a post-publication check.

### 2025 DataElement encoding: representational boundaries fit a vertical reader path — 2026-10-11

The new seventh 2025 chart initially rendered with four parallel branches; at
the available preview width, its labels were compressed. Reworked the same
map into a narrow decision path and rendered it again in VS Code Markdown
Preview. It now separates an HLA DataElement value and its selected format,
category-specific public-handle encoding, Umbra's private process frame,
FOM/OMT model input, and other protocol-owned representations. The labels are
readable while scrolling. The final catch-all avoids presenting the listed
paths as an exhaustive classification, and the process-frame branch remains
explicitly an Umbra detail rather than an HLA wire-format claim.

The new map rendered without a Mermaid error panel; the six existing 2025
diagrams retain their earlier local visual reviews. The guide now records
seven locally reviewed diagrams. The static inventory remains 209, and eight
other maps remain in the targeted local syntax/render backlog. GitHub rendering
remains pending after publication. The separate 2010 encoding companion was
not changed; the shared unsigned-octet helper is not used to infer parity. No
C++ build, test, CI, or Requirements Lab work occurred.

Next handoff: visually review the HLA-first subscription-interest reader map
in the [2025 object-attribute subscription guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md).
Check readability and whether its portable “receiver interest is not a
delivery promise” model stays distinct from selected Umbra implementation and
test evidence. Add no diagram unless a real learner gap remains. Preserve the
2010/2025 boundary, do not run C++ builds, tests, or CI, and check GitHub
rendering after publication.

### 2025 object-attribute subscription: receiver interest is not a delivery promise — 2026-10-11

Rendered the new HLA-first reader-interest map in the local VS Code Markdown
Preview. The diagram clearly separates declared receiver interest, object
registration, and publisher updates; it then distinguishes the discovery
selection question from per-attribute value eligibility and later delivery
gates. The compact map fits the available preview width and its labels remain
readable. Its adjacent prose correctly limits the chart to a conceptual map,
not a complete discovery/delivery algorithm or callback-ordering guarantee.

The guide keeps selected 2025 embedded source and test evidence in separate
sections and labels the process late-initial-value edge as unresolved; it
does not import 2010 behavior or claim process parity. This was a bounded
reader/visual review, not a fresh source audit or normative-conformance
finding. The other source-derived passive-recipient and late-subscription
diagrams in this guide still need local visual review. The static inventory
remains 209, with seven other maps pending targeted review. No Mermaid count
change, C++ build, test, CI, or Requirements Lab edit was made.

Next handoff: visually review the new HLA/API authorization-admission map in
the [2025 authorization guide](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md). Check
whether the portable operation-scoped gate is readable and remains distinct
from overload-specific API exceptions, Umbra's selected Connect/operation
paths, and caller-visible outcomes. Preserve the documented evidence gaps;
do not run C++ builds, tests, or CI. Keep 2010 separate and check GitHub
rendering after publication.

### 2025 authorization and credentials: admission versus service outcome — 2026-10-11

Rendered all three authorization diagrams in the local VS Code Markdown
Preview. The HLA-first flowchart is legible at a wide preview size and keeps
the three pinned Authorizer callback scopes separate from other services; its
result branches show that `AUTHORIZED` permits the operation to continue but
does not decide the service result. The Connect sequence and connected-
operation flowchart are separately introduced as Umbra case studies. Both are
readable while scrolling and make the profile, credential, authorization, and
later operation boundaries visible without presenting those implementation
details as portable HLA rules.

The adjacent table and notes preserve the overload-specific Connect
`@throws` mismatch as an open §4.2 question. The guide retains its prior
focused-test accounting (7/10 selected cases passed, with three setup failures
before their target assertions), and its process authorization integration
gap. This was a bounded local render/reader review, not a source re-audit, test
rerun, or normative finding. No 2010 behavior is inferred. The static
inventory remains 209 diagrams; six maps remain for targeted local
parse/render review. GitHub rendering and the full §4.2 check remain pending.
No C++ build, test, CI, or Requirements Lab work occurred.

Next handoff: visually review the sequence “One regional update, three
available-advance choices” in the [2025 DDM and regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#one-regional-update-three-available-advance-choices). Check that the receiver-specific choices and shared grant result are legible without implying the services are equivalent. Preserve its 2025 embedded test boundary; do not run C++ builds, tests, or CI. Keep 2010 separate and check GitHub rendering after publication.

### 2025 DDM: regional eligibility stays separate from three advance gates — 2026-10-11

Locally rendered and screenshot-reviewed the “One regional update, three
available-advance choices” sequence in the DDM guide. The layout keeps the
publisher, RTI, and three receivers distinct; each receiver's FQR, TARA, or
NMRA request remains its own path, while the scenario's common time-7 result is
visible without implying that the services are interchangeable. The callback
order is legible: each receiver observes the regional reflection before its
matching grant callback. The neighboring explanation keeps source/receiver
region overlap separate from receiver-specific time progress.

Evidence remains bounded to the single embedded 2025 test
[`timestamped_regional_attribute_update_mixed_advance_catch2.cpp`](../../cpp/tests/timestamped_regional_attribute_update_mixed_advance_catch2.cpp#L170):
its chosen values converge at time 7 and do not establish general service
boundaries or normative interpretation. The diagram adds no process or 2010
claim. This is a local render/reader review, not a C++ build, test rerun, fresh
standard audit, or GitHub render check. The static inventory remains 209; five
maps remain in the targeted current syntax/render backlog. No C++ build, test,
CI, or Requirements Lab work occurred.

Next handoff: continue the portable-HLA reader review with the source-derived
partial-result/late-join sequence in the [2025 federation synchronization
guide](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#late-joiners-change-only-an-expandable-barrier).
Keep the standard synchronization-point barrier distinct from Umbra's
omitted-set late-join expansion, and mark what the focused test does not cover.
Do not run C++ builds, tests, or CI. Keep 2010 separate and check GitHub
rendering after publication.

### 2025 federation synchronization: partial achievement survives an expanding join — 2026-10-11

Locally rendered and screenshot-reviewed the late-join state chart and the
“A partial result survives an expanding join” sequence in the 2025
synchronization guide. The state chart reads cleanly while scrolling and keeps
the expandable/fixed participant-set fork distinct. The sequence makes the
state change easy to follow: A reports `false` (1 of 2), C joins and is added
without discarding A's report, B reports (2 of 3), then C's report closes the
barrier with `{A}` as the failed set. The final per-member notification plan
and callback-model timing note remain visually distinct.

This is explicitly a source-derived Umbra 2025 trace, not a standard-mandated
late-join rule. The pinned 2025 API keeps registration, announcement,
achievement, and `federationSynchronized` as distinct application-facing steps.
The registry join hook grows the synchronization and announced sets without
resetting the achievement map, and achievement completion compares recorded
reports against the current set size. The focused direct and public tests join
before any achievement; neither covers the partial-result-before-join
interleaving. The guide's caveat is accurate and visible next to the sequence.
No 2010 or process parity is inferred. This was a bounded source/test and local
render review—not a fresh normative audit, build, test rerun, or GitHub render
check. The inventory remains 209 diagrams; four maps remain in the targeted
syntax/render backlog. No C++ build, test, CI, or Requirements Lab work
occurred.

Next handoff: visually review “Encoded values enter services as handle-keyed
octets” in the [2025 object and interaction information-flow
guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#encoded-values-enter-services-as-handle-keyed-octets).
Check that the public `AttributeHandleValueMap`/`ParameterHandleValueMap` value
path is distinct from handle identity, DataElement encoding, and private
transport framing. Reuse the pinned API and existing focused-test evidence;
make no broad process/embedded parity claim. Do not run C++ builds, tests, or
CI. Keep 2010 separate and check GitHub rendering after publication.

### 2025 HLA value maps at the API boundary — 2026-10-11

Locally rendered and screenshot-reviewed “Encoded values enter services as
handle-keyed octets” in the 2025 object/interaction guide. The tall flow remains
readable while scrolling: the shared DataElement setup branches into separate
attribute and parameter maps, keeps the user tag as a separate argument, then
returns to the corresponding callback map and receiver decoding. The chart
describes a public API value path, not an RTI-wide or network serialization.

The pinned API confirms `DataElement::encode()` returns `VariableLengthData`;
the two value-map typedefs pair that value type with typed attribute or
parameter handles. `RTIambassador` uses these maps for `updateAttributeValues`
(§6.10) and `sendInteraction` (§6.12), each with a separate
`userSuppliedTag`; `FederateAmbassador` carries the matching maps in
`reflectAttributeValues` (§6.11) and `receiveInteraction` (§6.13). Corrected
the guide's links to point at each actual signature. Its adjacent explanation
now explicitly separates the API handle key/identity from DataElement bytes
and separates HLA value payloads from Umbra's private process frame.

The existing process-endpoint send test constructs a handle-keyed parameter
map from raw test octets and checks that the pulled event retains that value
and its separate tag. This is evidence for that configured Umbra process path;
it neither tests DataElement encoding nor specifies HLA wire framing or
process/embedded parity. Pinned headers and existing test source were inspected,
not rerun. No code, C++ build, test, CI, 2010 behavior, or Requirements Lab
work was changed. The static diagram inventory remains 209; three targeted
maps remain. GitHub rendering is pending.

Next handoff: visually review “Service contracts—not argument shape—define
outcomes” in the [2025 cross-cutting state/event/observation
guide](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md).
Keep portable HLA service outcomes distinct from the selected Umbra commit
points, and avoid implying a universal transaction rule. Reuse existing
evidence and revise only if the reader review finds a real gap; do not build,
rerun tests, start CI, or fold in 2010 semantics.

### Historical authoring and render audit notes — checkpoint archive

The following dated inventory and visual-review notes were moved from the guide
index so its completion checklist can stay reader-centered. They preserve
historical snapshots, not the current queue; use the guide index and roadmap for
current counts and the next handoff.


The local VS Code installation is 1.141.0; its built-in Markdown preview
renders Mermaid fenced blocks (available since 1.121). Open a guide's preview
with `Ctrl+Shift+V`; see the [official Markdown preview documentation](https://code.visualstudio.com/docs/languages/markdown)
and [official Windows shortcut reference](https://code.visualstudio.com/shortcuts/keyboard-shortcuts-windows.pdf).
An earlier static inventory found 193 Mermaid blocks across 32
diagram-bearing Markdown files, with balanced fences: 104 flowcharts, 67
sequences, and 22 state diagrams. The separate 2010 guides contain 11 diagrams
(8 flowcharts, 3 sequences); the 2025 guides contain 180 (94 flowcharts, 64
sequences, 22 state diagrams). The two flowcharts in this index are
edition-neutral navigation maps, not behavior evidence. The 2026-10-10
non-rendering syntax pass accepted the
prior 173-block snapshot using the Mermaid modules bundled with VS Code
1.141.0; targeted parser passes cover only named later subsets, not all current
blocks. The two conceptual maps in this guide have since been visually checked
in the local VS Code Markdown preview; this does not establish whole-worktree
layout or GitHub parity. The DDM scope/time/callback chart, publication-
withdrawal flows, and other additions still need their own visual review. The
prior review ledger's pending count also needs reconciliation against current
worktree revisions.
The prior comparable whole-set audit checked
877 file-targeting local Markdown links. The 2026-10-10 bounded scan checked
1,201 local inline/reference destinations across the 33 HLA topic/index files
for target-file existence, line anchors, and heading fragments; it corrected
one stale heading fragment and its rerun was clean. The process-DDM references
and the new cross-cutting guide's local targets added after that scan were
checked individually, so 1,201 is not a fresh current total. This lightweight
scan does not replace GitHub's renderer or a
full CommonMark link parser. Static fence and link checks alone do not establish
diagram grammar or rendered readability.
The 2026-10-10 bundled-grammar sweep accepted the 155 then-current Mermaid
blocks. Two thematic splits brought that increment's count to 157; the worked
regional-TSO restore sequence brought it to 158, and the timed directed-TSO
save-boundary trace brought that earlier inventory checkpoint to 159. The 12
formerly pending charts, the added combined process-report chart, and all
split/revised views in this increment were subsequently rendered and visually
inspected in the installed VS Code Mermaid Markdown feature bundle.
The current local audit
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
earlier render-review backlog remained pending.
After that 66-diagram checkpoint, the callback exception and process-Resign
charts were added, parsed, and screenshot-reviewed locally; the current
local-only/revised reviewed count is 68. The process-Resign chart was revised
after the membership-gate audit and re-rendered; the count remains 68. The new
regional-TSO restore sequence was rendered and visually inspected, bringing
the local-only/revised reviewed count to 69. The new timed directed-TSO
save-boundary trace has now also been rendered and visually inspected,
bringing that count to 70. The two fresh-registry object-deletion restore
traces were then rendered and screenshot-reviewed, bringing the local-only/
revised reviewed count to 72. The
The twelve charts that remained pending after the syntax sweep—Modify Lookahead,
regional subscription state, regional active/passive relevance, process and
embedded ConnectionLost, two authorization charts, factory/RID, federation
switch seeding, and the three process-endpoint diagrams—were rendered and
screenshot-reviewed in the installed VS Code Mermaid Markdown feature bundle.
The federation-scope and embedded ConnectionLost charts were then each split
into two views at their distinct ownership/commit boundaries; all resulting
views rendered cleanly and were visually inspected. This closes the local
visual-review backlog for those cases only. GitHub verification of locally
changed diagrams remains a separate check.

Five 2025 sequences currently need local screenshot review. The regional-TSO
restore trace was visually reviewed earlier, but its punctuation-only Mermaid
syntax repair means that review must be repeated. The test-shaped TSO producer-
boundary trace's dedicated target was freshly rebuilt, and its focused embedded
CTest passes all 61 assertions when run with filesystem access enabled. The
sandboxed run failed during `CreateFederationExecution` because source-path
canonicalization was denied; treat that as an environment restriction, not a
retraction failure. This verifies only the named embedded `HLA_EVOKED` case.
The synchronization trace shows partial achievement followed by omitted-set late-
join expansion; source confirms the transition, but focused tests do not cover
that exact interleaving. The new lifecycle sequence traces an `HLA_EVOKED`
resignation attempt inside an active save callback, retry after callback
return, survivor failure callbacks, and a later successful save; its focused
CTest passes. The new DDM process-endpoint sequence traces strict and relaxed
per-recipient regional interaction fan-out across the public process endpoint
and both callback models. It passes the current bundled sequence grammar.
The earlier fresh whole-inventory pass accepted 169/169 blocks (2010: 10,
2025: 159). At a later 2026-10-10 checkpoint, the inventory was 178 (2010
remained 10 and separate; 2025 was 168), with 87 flowcharts, 67 sequences, and
24 state diagrams. A whole-worktree syntax pass accepted those 178 blocks,
including the three newer cross-cutting flowcharts. This parser-only check generated no SVGs and
does not establish visual verification.
Parser acceptance does not prove rendered readability. No test or Requirements Lab
mapping was added for the source-derived synchronization edge. The in-app
browser rejected the attempted `data:` preview URL, and no alternate browser
route was attempted. A later local VS Code review attempt found the app in the
window inventory, but screenshot capture showed the Windows lock screen and
keyboard navigation did not reach VS Code. Stop there; do not unlock the
desktop or route through another browser. Ten screenshot reviews are explicitly
pending from the previous audit: five 2025 sequences and five cross-cutting
flowcharts. The sixth and seventh cross-cutting flowcharts also need permitted
visual review, bringing the screenshot-review backlog to twelve. All seven
cross-cutting diagrams now pass the grammar check, but the whole-worktree pass
does not create or substitute for their screenshots. The current source
checkpoint inventory is 178 diagrams (87 flowcharts, 67 sequences, 24 state
diagrams; 2010 had 10 separate blocks and 2025 had 168). This 2026-10-10
checkpoint is superseded by the verified 2026-10-11 inventory above. The
earlier tally of 71 locally reviewed charts belongs to a prior checkpoint and
has not been reconciled to the current set.


### 2025 cross-cutting outcomes: service contract versus commit point — 2026-10-11

Reader review found that the generic “batch or composite HLA request” entry
and “commit grain” framing could make selected implementation commit units
look like portable HLA outcome categories. Reframed the section around the
exact service contract first: its specified results, exceptions, and state or
callback effects. The chart now explicitly labels its branches as selected
2025 Umbra implementation traces and asks which state unit that profile
updates. The ownership branch now says explicitly that a handle set is not one
ownership outcome. The durable lesson is to consult the operation-specific
contract, then treat internal commit points as profile-specific unless they
affect a required observable effect. Existing service examples and evidence
links are preserved; no HLA requirement or 2010 equivalence is inferred.

A bounded pass over the prioritized guide index found a cross-cutting
synthesis, baseline timing, and 29 numbered behavior areas already represented.
No additional umbrella guide was justified by topic coverage alone; continue
from concrete learner gaps and the existing review queue rather than adding
diagrams to meet a count.

Reconciled the generic regional-update/available-advance backlog label against
the guide rows: the non-retracted DDM case (“One regional update, three
available-advance choices”) already has local render and screenshot review;
the distinct pending map is “Retract a regional update before three
available-advance callbacks” in the TSO-retraction guide. Its existing
embedded `HLA_EVOKED` test asserts no receiver reflections, three completed
grants, successful first Retract, terminal repeated-Retract exception, and the
scenario-specific FQR actual/optimistic values 7/10. This was a source/test
inspection only; no test was rerun, and the sequence remains explicitly
scenario-scoped rather than a general advance-service or 2010 rule.

Final local screenshot review of the updated contract/outcome diagram remains
pending. The TSO guide now includes three additional diagrams: a strict
producer-boundary sequence, a regional update/retraction sequence, and a
producer-versus-current-owner timeline. The first two have explicit review
notes; the third has no render-review record and remains open. Together with
the support-switch and contract/outcome maps, five targeted diagrams need
fresh local parsing and visual review. The inventory remains 209. No C++
build, test rerun, CI, Requirements Lab edit, or 2010 change occurred.


The guide index's completion section is now a concise HLA-first authoring
checklist: lead with the portable service contract and state/eligibility/
observation model, label evidence and edition/profile limits, and use Umbra
internals only when they explain an HLA invariant, observable effect, or
implementation boundary. It explicitly discourages diagram-count growth and
default C++ builds/tests/CI for documentation-only work. The 113-line dated
renderer/audit checkpoint block was moved to the separate ledger and labeled
historical, leaving the index focused on navigation and current priorities.
No HLA behavior claim or source code changed.

Next handoff: keep the HLA reader model first. In the local Markdown preview,
review the contract/outcome map, then the three TSO diagrams (strict producer
Retract boundary; regional update retraction before available-advance
callbacks; producer identity versus current owner), followed by the support-
switch map. State the portable HLA question first and mark embedded/process
evidence and gaps separately; do not turn Umbra behavior into a standard rule
or merge 2010 and 2025. Continue by concrete reader gaps, not diagram count.
Do not build, rerun tests, or start CI.

### 2025 contract/outcome map: separate portable contract from profile evidence — 2026-10-11

Reworked the map's entry into two distinct reading questions: first identify
the results, exceptions, and observable effects specified by the HLA service
contract; separately inspect the commit unit in selected 2025 Umbra traces.
The caption states that the branches are conceptual, not runtime order, and the
map warns against inferring outcomes from the shape of a set-valued or
composite argument. Existing case-study branches, sources, and test links are
unchanged. This keeps the durable service-contract lesson visually primary
without presenting Umbra's commit granularity as portable HLA semantics.

The roadmap JSON parsed successfully; scoped staged/unstaged diff checks and
the untracked guide's trailing-whitespace check passed. No local Mermaid render
or screenshot review was performed, so that visual check remains open. No C++
source, build, tests, CI, Requirements Lab, or 2010 material changed.

Next handoff: visually review the revised contract/outcome map in the local
Markdown preview. If readable, proceed to the strict producer Retract-boundary
sequence in the 2025 TSO guide, checking that its portable legality question
leads and its scenario-specific Umbra behavior remains labeled as evidence.
Do not build, rerun tests, start CI, or combine 2010/2025 behavior.

### 2025 Retract legality map: HLA question before profile trace — 2026-10-11

Revised the producer-side flow so it first asks the portable clause question
(IEEE 1516.1-2025 §8.22.3), then labels the following validation and recipient-
ledger sequence as current Umbra 2025 behavior. The caption states that these
implementation edges are not a required RTI call sequence. The concrete
`t_base + L < T` predicate remains explicitly source/test-observed below the
map; no new normative interpretation was added. Existing scenario values,
source links, and test evidence were preserved.

No C++ build, test rerun, CI, Requirements Lab edit, or 2010 change occurred.
The local Mermaid render/screenshot review remains pending. The roadmap JSON
parses, scoped diff checks pass, and the changed Mermaid labels/fences were
statically inspected.

Next handoff: review the revised contract/outcome and producer-boundary maps
in Markdown preview, ensuring the HLA question is visually primary and profile
evidence remains clearly secondary. Then review the regional TSO sequence.
Do not infer normativity from the scenario, run builds/tests/CI, or combine
2010 and 2025.

### Regional TSO Retract case: separate recipient state from grant result — 2026-10-11

Added an HLA-first reader question to the regional-update retraction walkthrough:
for each receiver, distinguish whether the update was still pending when
Retract took effect from the result its independent time-advance request later
reported. The text labels the sequence as one embedded 2025 case, not a
universal FQR/TARA/NMRA schedule. The linked test source was inspected and
confirms the first Retract succeeds, the repeated Retract is terminal, no
regional reflections are reported, and the three grants complete with their
asserted values; tests were not rerun.

The roadmap JSON parses and the edited guides retain balanced Mermaid fences
and clean whitespace; scoped diff checks pass. Local visual review remains
open because this session exposes no desktop app surface. No C++ source/build,
test, CI, Requirements Lab, or 2010 changes occurred.

Next handoff: visually review the revised contract/outcome, producer-boundary,
and regional-TSO maps when Markdown preview is available. Confirm labels and
sequence remain readable, then review the producer-versus-current-owner map.
Do not build, rerun tests, start CI, or combine 2010 and 2025.

### Bounded content review: TSO ownership identity and support-switch scope — 2026-10-11

The producer-versus-current-owner section already leads with the durable
distinction between the accepted update's producing federate and the
attribute's current owner. It ties the callback's `producingFederate` field to
two selected embedded 2025 scenarios (divestiture/acquisition and source
resignation), while limiting the retained recipient work to those cases. No
broader process, 2010, or receiver-state guarantee is inferred.

The support-switch map already organizes the HLA reader's questions around
authority, scope, and sampling time, distinguishing getter-only federation
policy, a per-member FDD-seeded directive, and member-local setters. It marks
state-image behavior as Umbra-specific and the public post-restore getter
assertion as unverified. The bounded review found no separate learner gap
that would justify another diagram or content expansion.

The current computer-use surface exposes no desktop app, and the attempted
temporary browser render was rejected by browser URL policy. No alternate
render route was attempted; the five-item local visual-review queue remains
open. A static Markdown link-path audit checked 286 local targets across the
cross-cutting, TSO-retraction, save/restore, and guide-index documents; it
found no missing paths (anchor fragments and visual rendering were not part
of that check). No C++ build, test rerun, CI, Requirements Lab, or 2010 changes
occurred.

Next handoff: when a supported local Markdown preview is available, visually
review the three revised TSO/cross-cutting maps, then the producer-versus-
current-owner and support-switch diagrams. Keep content unchanged unless a
specific rendering or reader issue is visible. Do not build or rerun tests.

### HLA-first documentation priority checkpoint — 2026-10-11

The ongoing documentation goal is to translate reusable HLA behavior and
infrastructure concepts learned while implementing the standard into durable
reader-facing diagrams—not to document Umbra internals for their own sake or
to spend the implementation worker's capacity on repeated builds. A bounded
read of the callback, exception-reporting, and service-invocation guides found
that they already lead with useful portable distinctions: participant-specific
callback effects versus callback-delivery model; service outcome/state versus
report production and later observation; and the separate meanings of the
three MOM report paths. Their Umbra source and focused-test references are
bounded case studies, not universal HLA rules. No duplicate chart or broad
source/test inventory is justified by this review.

Next handoff: prioritize the next concrete HLA reader question from the
existing guide set. Begin with the standard/API contract and the behavior a
federate can observe; consult existing Umbra evidence only where it explains
an invariant, consequence, or explicit implementation limit. Do not infer
normative rules from source, conflate 2010 and 2025, or rerun C++ builds/tests.
The five pending visual checks remain a separate queue for a supported
Markdown preview surface.

### HLA-first reservation outcomes and embedded transaction detail — 2026-10-11

A bounded review of the 2025 public API and existing reservation guide found
the durable service distinction already present: reservation methods return
void, admissibility failures are synchronous, and a reservation outcome is
reported by callback. Bulk reserve yields per-name success/failure subsets;
bulk release instead validates the caller's ownership of the entire set. The
existing source/test map supports these selected 2025 behaviors, but exact
licensed standard prose remains the authority for normative use.

Refined the bulk-reserve diagram to describe candidate availability and the
result subsets without placing Umbra's registry lock in the HLA-facing flow.
The embedded profile's one-lock update and rollback mechanism now appears as
implementation evidence beside the embedded/process routing notes. The
process boundary, the 2010 separation, and all existing test/source pointers
remain unchanged. No runtime claim or new diagram was added.

A second bounded read of the FOM-admission guide found its opening already
separates 1516.2 syntax/schema validity, completed-model composition, FDD
materialization, and runtime capability; it also labels the edition/profile
gate as Umbra-specific. The 2010-model-input branch is explicitly not the
separate 2010 RTI stream. No duplicate chart was warranted.

No C++ build, test rerun, CI, Requirements Lab edit, or 2010 change occurred.
The edited reservation flow has not yet received a current Mermaid parser or
visual-preview check; the existing visual backlog remains open.

Next handoff: review the 2025 federation/federate lifecycle guide for the
HLA-facing membership and execution-lifetime distinction, especially
voluntary Resign versus connection loss and final-member exit. Separate the
service contract from Umbra cleanup/callback behavior and add a chart only if
the current guide leaves a concrete learner gap.

### HLA-first federation and federate lifecycle review — 2026-10-11

The existing lifecycle guide already distinguishes ambassador connection,
federate membership, and federation-execution lifetime. It states that
successful Resign ends membership but does not itself Destroy the execution,
and has separate paths for voluntary Resign, forced connection loss, and
explicit Destroy. Its summary table compares peer versus final-member exits
for both voluntary and lost-member paths, including the tested case where the
final member's connection loss still leaves the execution present until
explicit Destroy. It labels action cleanup, automatic directives, callback
routes, and the final-member delete pass as current implementation evidence;
it does not promote them to normative rules. The 2010 reference stream remains
separate.

The pinned 2025 API declarations confirm Resign (§4.12) and the
`connectionLost` callback (§4.4) vocabulary and their exception/callback
surfaces. The full licensed service prose is still needed to independently
verify the normative action table, so that gap remains open. The guide already
marks it and links the distinct implementation/test evidence; no duplicate
diagram or wording change was warranted. Existing evidence was inspected, not
rerun. A local `mmdc` command or repository Mermaid checker was not present;
this review did not render any diagram.

No C++ build, test rerun, CI, Requirements Lab edit, or 2010 change occurred.
Next handoff: review the per-attribute ownership guide's distinction between
request admission, ownership transition, and callback observation. Keep
callback-dispatch commit points scoped to current 2025 profile evidence, and
add a chart only if the existing HLA-first state model leaves a concrete gap.

### HLA-first per-attribute ownership review — 2026-10-11

The guide already opens with the object-instance attribute as the ownership
unit and separates publication, subscription/discovery, ownership, accepted
request, and callback outcome. Its requester state chart places the tested
2025 ownership commit at acquisition-notification dispatch and explicitly
disclaims a complete normative state machine. The regular and If-Available
flows distinguish admission, pending work, revalidation, and public callback;
the owner-side diagrams separate negotiated confirmation from unconditional
divestiture and an assumption offer. Query Attribute Ownership is separately
identified as a snapshot that does not transfer ownership.

The focused evidence and known-limits sections scope callback-time mutation to
the current profile, distinguish selected embedded cases from the process
confirmation case, and leave the full standard's action/arbitration wording
open. The 2010 stream is excluded explicitly. This satisfies the handoff's
reader question; no new chart or wording change was justified. The existing
source and focused-test evidence was inspected, not rerun.

No C++ build, test rerun, CI, Requirements Lab edit, or 2010 change occurred.
Next handoff: review the 2025 attribute update-rate guide to see whether the
HLA reader can distinguish rate admission from ownership, transportation
category, receive/timestamp order, and time-management eligibility. Reuse the
existing flows and evidence; add no chart unless a concrete concept is missing.

### HLA-first update-rate admission orientation — 2026-10-11

The existing guide already separates the owner-facing Attribute Relevance
Advisory from subscriber-side update-rate selection, puts receiver projection
before per-attribute filtering, treats reliable/best-effort transportation
separately from RO/TSO order, and places queued-TSO grant eligibility before
the steady-clock filter. One useful reader distinction was dispersed across
sections: ownership answers who may update the attribute, while the rate gate
answers whether a projected receiver value is admitted now.

Added a compact six-question table connecting ownership, receiver candidate
selection, order, transportation, logical-time eligibility, and rate
admission. It labels reliable bypass/best-effort filtering and steady-clock
mechanics as selected embedded 2025 behavior, not portable guarantees. It
links existing guides rather than duplicating their state machines; no new
diagram or runtime claim was added. The existing test/source map is reused,
not rerun. The new table has not received visual preview, and it does not
verify the exact normative rate-combination algorithm.

No C++ build, test rerun, CI, Requirements Lab edit, or 2010 change occurred.
Next handoff: review the object/interaction guide's update-to-reflection path
for any remaining reader confusion among sender authority, receiver
eligibility, per-attribute rate filtering, and callback observation. Avoid
adding a duplicate chart.

### HLA-first update-to-reflection reader review — 2026-10-11

The guide already opens with a four-question HLA model: producer authority,
relevant receiver selection, receiver object knowledge, and application
observation. The ordinary object update flow then separates admission of the
sender's values, receiver-specific projection, callback-time revalidation, and
final reflection. The rate gate is per attribute and applies only to the
selected embedded best-effort path; the guide links its rate guide and keeps
TSO grant eligibility in the time-management guide. It neither equates
ownership with recipient eligibility nor presents Umbra's ordinary receive-
order path as the general HLA algorithm.

The source/test and scope notes already bound these details to the selected
2025 embedded path and separately identify process cases and gaps. The top of
the guide explicitly excludes 2010; no shared behavior is inferred. The
handoff's reader distinction is met, so no duplicate chart or wording change
was warranted. Existing implementation and focused-test results were
inspected only; no runtime checks were rerun.

No C++ build, test rerun, CI, Requirements Lab edit, or 2010 change occurred.
Next handoff: inspect the 2025 DataElement guide's public byte-order/value
representation boundaries and its distinctness from typed-handle encoding
and private process framing. Check the separate 2010 companion only for
edition separation, not behavioral parity. Add or revise a diagram only for
a concrete reader gap.

### HLA-first encoding and callback-model reader review — 2026-10-11

The 2025 DataElement guide begins with the portable typed-value boundary and
distinguishes value octets from public handle encoding, model input, and
private RTI framing. It explains type-selected byte order, nested cursor
composition versus whole-input decoding, composite shape/alignment,
discriminant-selected variants, and factory-defined logical-time encoding.
Umbra sources and focused cases illustrate these concepts without being
presented as normative authority. Its separate 2010 companion states its own
scope and evidence and makes no parity inference. No reader gap justified a
duplicate chart or wording change.

The callback guide likewise leads with portable HLA questions: which
participant has a callback effect, how the selected callback model exposes it
to application code, and why ordering and service completion remain
operation-specific. Dispatcher queues and process/embedded paths are isolated
as implementation evidence. This is a useful pattern for durable HLA
documentation: explain the participant-visible concept first, then show only
the implementation detail that clarifies an invariant or observation point.
No C++ source, build, test, CI, or Requirements Lab work was needed.

Next handoff: perform a bounded HLA-first review of the 2025 federation
save/restore guide. Check whether a junior can follow portable protocol phases,
participant work, completion barriers, and membership boundaries before the
Umbra case studies. Keep join-during-save/restore unresolved until the exact
normative conditions are available; do not infer them from source or tests.
Reuse existing runtime evidence and keep the 2010 stream separate.

### Federation save/restore HLA-first reader review — 2026-10-11

The guide gives a clear portable orientation before implementation detail:
each federate owns its application checkpoint, while the RTI coordinates its
federation checkpoint and the federation-level outcome. It distinguishes
request admission from protocol completion, participant completion from the
federation barrier, and status queries from state-changing services. The
conceptual diagram is labeled as an orientation rather than a normative
sequence; the detailed callback and coordinator flows are scoped to the
selected implementation profiles and tests. Its case studies do not collapse
2010, embedded 2025, process-endpoint 2025, or fresh-registry evidence.

One guidance issue was corrected: the final reading path had told contributors
to “run or inspect” focused scenarios. It now directs them to inspect existing
results and permits a fresh run only when a material claim cannot be supported
otherwise, preserving the main implementation worker's build capacity.

The Join-during-save/restore question remains unresolved. The official IEEE
product page currently routes the full standard to purchase/subscription, and
the linked public download archive contains the C++ API headers, not the
normative service text. The API's exception list and inspected implementation
paths are insufficient to infer exact preconditions or membership effects.
The guide correctly leaves the question open; do not add a guessed branch.

No new diagram was justified: the existing responsibility map and detailed
flows already answer the junior-reader questions without conflating local and
federation checkpoints. No C++ source, build, test, CI, or Requirements Lab
work occurred.

Next handoff: review the 2025 FOM admission/composition guide for its learner
path from HLA model vocabulary and module composition to Umbra schema/FDD
materialization. Keep normative model rules distinct from implementation
validation and runtime support, and add a diagram only for a concrete gap.
Do not run C++ builds or tests; leave 2010 and the Requirements Lab out of
scope.

### FOM vocabulary and runtime-admission reader review — 2026-10-11

The 2025 FOM guide leads with shared HLA model vocabulary and correctly
separates three questions: whether each module is structurally valid, whether
the complete ordered set composes and resolves references, and whether Umbra
can materialize an RTI-facing FDD and support the described behavior. It does
not equate schema validity with semantic suitability or runtime capability.
The Create prepare/commit and additional-module Join diagrams are explicitly
scoped to the selected embedded profile. The narrow `fomEdition=2010` catalog
branch is clearly under the 2025 API, with no claim that it switches to the
2010 RTI stream. Existing source and focused-test links provide bounded case
evidence; no new runtime evidence was needed.

The current wording and diagrams already answer the learner's main boundary
questions, so no duplicate chart or content edit was justified. Keep the
Requirements Lab out of scope and preserve the 2010/2025 distinction.

Next handoff: review the 2025 names/handle guide for a junior-friendly model
of public identity, scope/lifetime, typed opaque handles, and handle encoding
versus numeric identity. Keep Umbra's numeric representation as a bounded
implementation example; do not infer 2010 behavior. Revise only for a concrete
reader gap and do not run C++ builds or tests.

### Public name and handle learner-model review — 2026-10-11

The 2025 guide separates execution-scoped model symbols, federate-local object
knowledge, and federate name/handle lookup lifetimes before showing service
paths. It also distinguishes category-typed opaque API handles from hashes,
encoded payloads, normalization coordinates, and Umbra's private numeric
registry values. The latter are bounded examples, not portable numeric
identity. Its evidence table distinguishes embedded and process-endpoint
observations, and the 2010 reference-RTI guide remains separate. No substantive
reader gap or duplicate diagram was found.

Next handoff: review the separate 2010 reference-RTI guide for edition
discipline and reader clarity. Distinguish public HLA API concepts from this
reference implementation's behavior/limits; do not import 2025 transitions
or imply parity. Reuse existing 2010 evidence; no build or test reruns.

### 2010 boundary and paired MOM-scope reader review — 2026-10-11

The 2010 guide identifies the IEEE 1516.1-2010 Federate Interface
Specification as normative authority and labels its diagrams as the behavior
of Umbra's bounded in-process reference RTI. Unsupported services, source-only
barrier details, the single HLA_IMMEDIATE smoke path, and untested callback
exceptions remain explicit; no 2025 timing, callback queue, or profile
behavior is imported. A static pass checked all 72 local Markdown link targets;
all exist. No source, runtime, or visual changes were needed.

The 2025 federation-MOM and joined-federate-MOM guides already express the
durable scope distinction: HLAfederation is execution-scoped, while
HLAfederate is associated with a joined membership. Their lifetimes, update
triggers, and callback paths are independently diagrammed and cross-linked.
A combined chart would duplicate those flows rather than resolve a learner
gap, so none was added. Their evidence remains 2025-only and profile-bounded.

### Provider solicitation vs. relevance-notification reader gap — 2026-10-11

Auto Provide solicits current values from the provider after the relevant
discovery path; Turn Updates On/Off reports changed attribute relevance or
effective demand to the owner and carries no values. Because both can be
provider-facing, the guides now link to each other and state this distinction
explicitly. Their triggers and service effects remain separate, so a combined
diagram would blur rather than clarify the HLA behavior. This is a bounded
2025 guide/profile explanation, not a complete conformance claim. Existing
source and test references were reused; no builds or tests were run.

### 2025 subscription-state reader-model review — 2026-10-11

The guide's existing HLA-first map treats declared interest as an input to
separate discovery and per-attribute value-eligibility outcomes. Its later
interest-change flow distinguishes receiver discovery and embedded initial
reflection from receiver scope advice, owner relevance advice, and Auto Provide
solicitation. The implementation/profile limits are explicit, including the
unresolved ordinary process late-discovery initial-value edge. No duplicate
diagram or wording change was warranted. A bounded link audit found 60 Markdown
links with no missing local targets or out-of-range source line anchors. The
passive-recipient and late-subscription diagrams still await visual review; this
reader audit does not close that item. No builds or tests were run.

Next handoff: review the 2025 Delay Subscription Evaluation guide as a
cross-cutting eligibility-timing concept. Check the boundary between when
receiver subscription/relevance is evaluated and whether a value is delivered,
without merging declaration state, callback scheduling, logical-time grants,
or the 2010 stream. Keep the HLA explanation distinct from implementation
representations and add only a concrete reader improvement. Reuse existing
evidence without rerunning tests.

### 2025 DSE, provider-facing value paths, and Scope Advisory preview — 2026-10-11

The DSE reader review confirms the portable teaching point is evaluation
timing, not guaranteed delivery. Sender service validity, receiver
subscription/relevance, callback-model dispatch, and TSO grant eligibility are
distinct questions. The guide labels Umbra's route-only candidate as an
implementation representation and marks its §8.1.8 wording as a conservative
paraphrase pending an authorized copy of the full clause. No focused tests were
rerun.

A bounded survey compared explicit Request Attribute Value Update, Auto
Provide, Turn Updates On/Off, initial reflection, and scope notifications.
Existing guides already separate their triggers, recipients, and effects; in
particular, Auto Provide's neighboring-path table explicitly contrasts the
similar provider-facing callbacks. No combined diagram or duplicate prose was
needed.

GitHub Preview at commit `5392bde4f41e7da33a03835ff16a78c663d62022` rendered
all three Attribute Scope diagrams. The first two had been individually
visually reviewed on 2026-10-09. The repaired decision flow was screenshot-
inspected on 2026-10-11; it is tall but readable across scroll positions, and
the callback-call labels parse. This closes the earlier render/layout check for
that published revision. The modified local subscription-state diagrams are
not in that revision and remain pending visual review.

No C++ build, test rerun, CI, Requirements Lab edit, or 2010 change occurred.

Next handoff: examine the 2025 save/restore guide's unresolved Join-during-
operation boundary against the exact IEEE 1516.1-2025 §4.11 text from an
authorized source. Determine only what the normative source establishes about
participant admission and effects; do not promote Umbra implementation details
into HLA rules or conflate the issue with synchronization achievement, normal
membership, or time grants. If exact text is unavailable, preserve the
uncertainty and move to another HLA-first gap without compiling or rerunning
tests.

### 2025 save/restore Join-boundary access review — 2026-10-11

The two pinned 2025 Join overloads list `SaveInProgress` and
`RestoreInProgress`. The representative focused interlock case joins both
members before requesting save and does not test an in-progress Join. The
embedded and process source observations remain profile-bounded and do not
resolve the normative precondition or participant effects.

The public IEEE product page identifies IEEE 1516.1-2025 as the active
standard and offers purchase/subscription access, but does not expose §4.11
text; the repository's pinned IEEE 2025 package contains API headers, not the
specification PDF. The guide now records that limit and keeps the question
open rather than inferring behavior from `@throws` or implementation code.

A bounded read of the current save/restore responsibility map and four worked
cases found the architecture-neutral HLA checkpoint distinction comes first;
the selected save-cut, pending-TSO, retraction, and recipient effects are
explicitly labeled embedded 2025 case evidence. No diagram or normative claim
was added during this review. No build or test was run.

Next handoff: continue the HLA-first reader-gap audit of the save/restore map
and worked cases, ensuring the reusable save-cut/retraction questions remain
clear before embedded queue and callback details. Reuse existing test evidence;
do not build or rerun tests. Keep visual review of the modified local Mermaid
diagrams pending until publication.

### 2025 save/restore HLA-question reader audit — 2026-10-11

Reviewed the architecture-neutral save/restore responsibility map and the
four independent worked cases. Added a compact reader lens before the examples
for (1) pending work at the selected save point, (2) federation completion
versus receiver observation, (3) restored retraction state/designator, and
(4) recipient-specific observation after partial delivery. It uses the cases
to pose durable HLA questions without claiming their embedded queue, callback,
or fresh-registry mechanics are normative. The referenced test assertions
were checked in source; no tests or builds were run. Exact Join-during-save/
restore semantics remain unresolved without authorized §4.11 text. No 2010,
Requirements Lab, C++, or CI work occurred.

Next handoff: perform a bounded HLA-first reader audit of the 2025 FOM module
admission and composition guide. Focus on the distinctions among per-module
DIF validation, ordered complete-set composition, final FDD validation, and
transactional Create/additional-Join outcomes. Reuse existing focused evidence;
do not build or rerun tests, infer 2010 parity, or edit the Requirements Lab.

### 2025 FOM module admission/composition reader audit — 2026-10-11

Current guide review confirms the existing model-validity table and diagrams
already distinguish per-module DIF structure, ordered complete-set composition
and cross-module resolution, FDD materialization, and RTI runtime support. The
Create and additional-module Join diagrams keep Umbra's prepare/commit behavior
profile-bounded; a separate FOM-edition diagram says the 2010 compatibility
lane is a catalog under the 2025 API, not the 2010 RTI stream. No duplicate
chart or wording change was justified.

Source checks confirmed Create preparation returns before registry creation on
failure, additional-module Join reconciles replacement handle directories and
swaps the new definition only after membership writes succeed, and the existing
integration test checks that failed extension preparation leaves the original
federation joinable. Tests were inspected, not rerun. The guide has four
Mermaid diagrams; three baseline charts were previously GitHub-reviewed, while
local Create/Composition revisions and the added edition chart remain pending
GitHub review after publication. The official public 1516.2 page describes
format and syntax but does not provide the full standard clause text. No C++
build, test, CI, Requirements Lab, or 2010 RTI work occurred.

Next handoff: review the 2025 TSO message-retraction guide's producer-legality,
per-recipient phase, and producer-versus-current-owner maps as HLA teaching
tools. Check their boundary between pending withdrawal and post-delivery
Request Retraction, using existing case evidence and keeping embedded/process
claims separate. Do not build or rerun tests; leave the Requirements Lab and
2010 implementation stream untouched.

### 2025 TSO retraction: separate producer, recipient, and grant questions — 2026-10-11

Audited the producer time-boundary, recipient delivery/retraction, and
producer-versus-current-owner explanations as an HLA reader path. Added a
compact orientation table that makes the three independent questions explicit:
producer legality, each recipient's original-delivery phase, and each
receiver's later time-request result. This synthesizes existing sections; it
does not add another state machine or claim that the request modes are
equivalent.

Bounded source/test inspection confirmed the embedded 2025 boundary uses the
current time or pending advance-request time plus actual lookahead with strict
less-than; interaction callback admission changes that recipient's state
before entering user code; suppressed callbacks do not become delivered; and
the focused regional case's grant values are scenario-specific. The ownership
transfer and source-resignation cases retain the original producer identity
and designator for their already-accepted update. These are implementation and
focused-case observations, not new normative claims or process/2010 parity.
Tests were inspected, not rerun. No C++ build, CI, Requirements Lab, or 2010
work occurred.

The three targeted TSO diagrams remain pending local parser/render and
screenshot review. Next handoff: review those existing diagrams in an available
Markdown preview, focusing on strict producer-time boundary, per-recipient
delivery phase, and producer identity versus current owner. Reuse the evidence
already indexed; do not compile or rerun tests. Keep the 2025 embedded/process
boundary explicit, preserve the separate 2010 stream, and leave the
Requirements Lab untouched.

### 2025 object-name reservation and peer-visibility synthesis — 2026-10-11

A bounded comparison of the 2025 names/handle, object-name reservation,
FOM-to-region, and Delay Subscription Evaluation guides found their principal
reader models already explicit. The FOM/DDM bridge distinguishes the dimension
catalog, class availability, application-selected region dimensions, and
later scope matching. The names/handle guide distinguishes declarations,
member-scoped typed handles, and per-federate known instances. The reservation
guide separately showed an owner's reservation and registration, while its
known-instance cross-reference explained that peer knowledge begins after
discovery delivery. That last distinction was clear in prose but split across
two diagrams and two focused cases, so a bounded two-view synthesis was added
to make the different participant-local transitions visible. The guide index
and name/handle cross-link now expose it directly.

The new flow is limited to the 2025 embedded `HLA_EVOKED` observations. The
named-registration case asserts the owner's reservation callback and handle,
plus the peer's queued/later discovery; the separate known-instance case
asserts `ObjectInstanceNotKnown` before discovery delivery and successful
lookup afterward. The chart explicitly says no single test asserts the whole
combined path. No normative extension, process parity, or 2010 behavior is
claimed. The static inventory is now 210 diagrams across 32 files (115
flowcharts); the new chart and five previously tracked diagrams remain
unparsed/unrendered. The local Mermaid CLI and Node package are unavailable,
and no editor preview was available in this task. No C++ build, test rerun,
CI, Requirements Lab edit, or 2010 RTI work occurred.

Next handoff: perform a bounded HLA-first reader audit of the 2025 Order and
Transportation guide. Check that order versus transportation, sender scope,
and the different effective-state boundaries are explained before Umbra's
implementation details, and remain separate from DDM and time eligibility.
Reuse existing evidence; revise only for a concrete learner gap. Leave the
pending Mermaid visual checks visible and do not build or rerun tests.

### 2025 joined-federate MOM: member identity versus observer knowledge — 2026-10-11

The guide already has a detailed embedded Join/discovery/value/resign trace,
but its opening moved straight from scope into Umbra's private snapshot. A
reader-first pass added a small HLA concept table before that trace. The table
separates execution membership from observer discovery, the `HLAfederate`
instance per joined member from the `HLAfederation` instance per execution,
attribute value freshness from object knowledge, and the resign transition
from a particular observer's removal callback.

The cardinalities are grounded in the pinned 1516.2-2025 MIM class semantics
at `HLAstandardMIM-2025.xml:52` and `:658`; the text links directly to both.
The rest is deliberately framed as a reader model, with exact attribute update
policies left to the MIM and callback mechanics left to the separately labeled
embedded profile. No additional state diagram was justified because the
existing lifecycle sequence already shows the transition path. No C++ build,
test, CI, 2010, or Requirements Lab work occurred; the static Mermaid
inventory did not change.

Next handoff: audit the 2025 federation/federate lifecycle guide for the
HLA-level boundaries between local RTI connection, federation-execution
lifetime, federate membership, and per-member MOM lifetime. Use existing
pinned API/MIM and test evidence and change the guide only for a concrete
reader gap. Keep 2010 separate; do not build or rerun tests. The six targeted
Mermaid parse/render checks remain open.

### 2025 federation lifecycle: three scopes and MOM representation — 2026-10-11

The lifecycle guide already presents RTI ambassador connection, federate
membership, and federation-execution existence as separate lifetimes, with
distinct local and execution state diagrams. The bounded cross-read against
the joined-federate MOM guide found a useful link to make explicit: a
`HLAfederate` MOM instance represents each joined member under the 2025 MIM,
but object visibility to any particular observer remains a distinct
object-discovery question. Added a cross-link at the scope boundary. The
existing charts already explain the transitions, so no duplicate chart was
added.

No implementation or test source was re-read for this link-only increment;
the MIM-backed lifecycle explanation remains scoped to its current evidence.
No build, test, CI, 2010, or Requirements Lab work occurred. Six Mermaid
parse/render checks remain pending.

Next handoff: audit the 2025 federation synchronization guide for the
HLA-level barrier sequence—participant selection, per-participant achievement
result, and federation-level completion—and confirm it remains distinct from
logical-time progress, execution membership, and callback observation. Reuse
existing evidence; do not infer unresolved standard clauses or run builds,
tests, or CI. Keep 2010 separate and the Requirements Lab untouched.

### 2025 synchronization: map API events to barrier roles — 2026-10-11

The opening already provides the HLA-level barrier model and separates it from
time management, execution membership, and callback timing. A bounded read of
the pinned API showed a smaller teaching gap: the reader had to infer how the
registration overloads, registration result callbacks, participant
announcement, boolean achievement report, and failed-set completion callback
fit together. Added a role/boundary table with direct links to the public
§§4.14–4.18 declarations. It labels itself as API vocabulary, not the full
normative service rules; participant-set changes and exact ordering remain
unresolved unless verified from the full standard.

The existing charts already depict registration, participant fan-in, and
completion, so a fifth conceptual chart would repeat their path. No C++
implementation or test source was inspected for this table, and no build,
test, CI, 2010, or Requirements Lab work occurred. The diagram inventory did
not change.

Next handoff: audit the separate 2010 synchronization flow in the bounded
reference-RTI guide for strict edition/profile boundaries and its source-only
test limits. Do not infer 2010/2025 equivalence from API names, do not enable
2010 CI, and do not edit the Requirements Lab.

### 2010 synchronization: preserve the profile boundary — 2026-10-11

The separate 2010 reference guide already labels its synchronization barrier
as an implementation observation, notes the lack of a focused 2010 test, and
states that it does not claim 2025 equivalence. A targeted source check
confirmed a material reason to keep the flows separate despite the shared API
method name: the 2010 reference implementation inserts only successful
achievement reports and returns on `false`; the 2025 embedded registry stores
the first result either way and forms a failed-to-synchronize set at barrier
completion. This is a source/profile contrast only, not a claim about either
edition's full normative service rule. No guide change was needed, and no 2010
test/build/CI or Requirements Lab work occurred.

Next handoff: review the 2025 directed-interaction guide as a portable
participant-role model. Ensure the sender, target object/owner, and recipients
are distinct from publication/subscription/DDM gates and from callback
observation. Reuse existing evidence; do not build or rerun tests, and keep
the 2010 stream separate.

### 2025 directed interactions: distinguish target identity from DDM region — 2026-10-11

The existing guide already leads with the portable HLA roles—producer,
target object/owner, and RTI-selected receivers—and separates directed
publication from the receiver selector. Its missing reader cue was the
boundary with regional DDM. Added a short API-level comparison: the 2025
directed send carries an object-instance handle and directed subscription
selects universal/by-ownership, while regional interaction services carry
region sets. The text explicitly limits the conclusion to API shape and
does not assert an unverified normative rule about all directed/DDM
interactions. No diagram was added because the existing role table and
selector chart already cover the path.

No Umbra C++ source, build, test, or CI was touched; the API header and the
existing DDM guide were the bounded evidence. No 2010 or Requirements Lab
changes occurred. Six targeted Mermaid render checks remain open.

Next handoff: audit the 2025 order/transportation guide for a concise,
portable explanation of two independent dimensions—delivery order and
transportation reliability—and how they meet time-managed eligibility and
callback observation. Reuse the time guide and available pinned standard/API
evidence; label any implementation-only details and unresolved normative
claims. Do not spend main-worker capacity on compilation or test reruns, and
preserve the 2010/2025 boundary.

### 2025 order and transportation: separate callback observation — 2026-10-11

The guide already explains that receive/timestamp order, transportation
category, receiver eligibility, and TSO time gating answer different
questions. The cross-read found that its combined delivery sequence ended
with a callback arrow that could imply immediate application invocation.
Added an HLA-level note: after a participant-specific callback effect is due,
HLA_IMMEDIATE/HLA_EVOKED governs application entry; callback model does not
choose recipients or change message order or transportation. Added this
prose boundary beside the existing delivery flow; no duplicate diagram was
needed and Mermaid syntax was not changed.

No implementation source, build, test, or CI was touched. This was a bounded
cross-read of existing 2025 documentation only; 2010 and Requirements Lab
remain unchanged. Six targeted Mermaid visual checks remain open.

Next handoff: review the 2025 FOM admission/composition guide for a portable
HLA account of model validity and composition, clearly separated from
Umbra's chosen Create/Join handling. Reuse existing diagrams and pinned
standard/API vocabulary; only open implementation evidence for a specific
unresolved claim. Keep the edition boundary and no-build/no-CI constraint.

### 2025 FOM guide: separate model validity from RTI admission — 2026-10-11

The FOM guide already distinguishes per-module DIF structure, complete-set
composition, and RTI-facing FDD materialization, and explicitly says schema
validity does not establish semantic suitability or feature support. Its
Create/Join diagrams are scoped to the selected embedded profile, with a
separate narrow 2010 model-catalog lane. Added exact pinned 1516.1 API links
for single/vector Create inputs and optional Join modules. Also clarified
that “MIM-first” names the current Umbra preparation order; no normative
ordering rule is inferred without the applicable standard clause. This is
consistent with IEEE's official 1516.2-2025 description of OMT as defining
object-model format and syntax, not content.

All 31 local links in the guide resolve and its four Mermaid fences are
balanced. No C++ source or test evidence was re-read; no build, test, or CI
was run. No 2010 or Requirements Lab changes occurred. Mermaid visual review
remains pending for the existing six targeted diagrams.

2026-10-11 review: The 2025 object/interaction information-flow guide gives
readers four distinct questions—authorization, recipient relevance,
per-receiver object knowledge, and callback observation—and follows their
different paths through ordinary/directed interactions, object discovery,
ownership, updates, and callbacks. It already separates embedded and process
behavior where evidence differs. No additional diagram or prose was justified
for those covered paths. The guide deliberately leaves the complete class
declaration lattice and all active/passive transitions to focused companions;
that is the useful next review boundary, not a reason to duplicate their state
machines here.

Structural audit: all 141 local Markdown links resolve; 17 Mermaid blocks have
balanced fences. Mermaid visual review is not claimed; the existing six
targeted visual checks remain open. No implementation source, tests, build, or
CI were inspected/run. The 2010 stream and Requirements Lab were untouched.

Next handoff: compare the 2025 object-attribute subscription-state and
relevance-advisory guides against this overview to see whether they jointly
explain the HLA distinction between declared subscription interest, active /
passive advisories, effective recipient eligibility, per-federate object
knowledge, and reflection delivery. Reuse their detailed flows; add only a
crosslink or a small synthesis if a real reader gap remains. Start from pinned
API/standards material and existing documentation, label evidence limits, and
do not build, rerun tests, start CI, cross into 2010, or edit the Requirements
Lab.

### 2025 directed interactions: clarify default selector wording — 2026-10-11

The directed-interaction guide already leads with the portable role
distinction: the sender chooses one target object, while the RTI selects zero
or more receivers; the target owner can be a different federate. It separates
producer publication from receiver selection, and the target/selector from
discovery, parameter selection, ownership transfer, and regional DDM. Its
receive-order and timestamped paths retain distinct time/callback boundaries,
with embedded/process differences stated rather than generalized.

One wording risk remained: “default false selector” could blur the API's
default argument with Umbra's mapping. Clarified that the pinned API defaults
`universally` to false, Umbra maps false to by-ownership and true to
universal, and the cited test uses omitted false versus explicit true. All 43
local links resolve and six Mermaid fences are balanced. This is structural
validation, not visual rendering. Existing test and API evidence was
inspected, not rerun; no implementation source, build, test, or CI work
occurred. No 2010 or Requirements Lab content changed.

Next handoff: review the 2025 cross-service class-ancestry map against the
ordinary-object, ordinary-interaction, and directed-interaction guides. Check
that readers see one composed class model but distinct service-specific uses
for discovery, attribute projection, interaction-class promotion/parameter
projection, and directed target selectors. Keep object-class and
interaction-class trees separate; label Umbra-derived lookup details rather
than presenting them as unverified normative rules. Reuse existing API/source/
test evidence and change the docs only for a concrete learner gap. Do not
build, rerun tests, start CI, cross into 2010, or edit the Requirements Lab.

### 2025 class ancestry: one model, service-specific lookup — 2026-10-11

The object/interaction overview already states that there are separate
object-class and interaction-class chains, then shows why there is no single
reusable “nearest ancestor” rule: discovery sets each receiver's known object
class; reflection projects through that known class; ordinary interaction
matching may promote the sent class and project parameters; directed matching
searches the target object's ancestry for its pair and selector while
preserving the sent interaction class for the callback. These claims are
explicitly source-observed 2025 paths, not a complete normative inheritance
rule.

The evidence map distinguishes the embedded focused cases, an internal-only
ordinary-promotion test, and the process exact-class directed test, which does
not establish ancestor lookup. No new crosswalk was needed; the existing map
and linked service flows make the different jobs legible without duplicating
their state machines. No source, test, build, or CI work occurred in this
documentation review. No 2010 or Requirements Lab behavior was inferred or
changed; existing Mermaid visual checks remain pending and no rendering is
claimed.

Next handoff: audit the 2025 cross-cutting guide's “Service contracts—not
argument shape—define outcomes” map as an HLA-first teaching tool. Check that
the portable question (what outcomes and observable effects the specific
service contract defines) stays distinct from Umbra's selected state-commit
units in bulk reservation, bulk release, additional-module Join, and
attribute-level ownership. Keep the examples profile-scoped and avoid
generalizing behavior from set/composite argument shape. Reuse existing
evidence; change content only for a concrete reader gap. Do not build, rerun
tests, start CI, cross into 2010, or edit the Requirements Lab.

### 2025 service contracts versus profile commit points — 2026-10-11

The four examples remain clearly scoped to selected 2025 Umbra behavior: bulk
name reservation reports per-name subsets, bulk release validates the whole
requested set before removal, additional-FOM Join prepares then commits model
and membership together, and ownership state is per object-attribute. The
prose and chart distinguish these profile commit units from the portable HLA
question of required outcomes and externally observable effects; they
explicitly reject inferring atomicity from argument shape.

One reader ambiguity remained in the preceding service-result diagram: its
“state change or admitted work; locate its commit boundary” node could make a
private implementation boundary look contract-prescribed. Reworded it to name
the HLA-required observable effect and say that the implementation's private
commit point is separate. Added an index reader path to make the reasoning
findable alongside the service-specific guides.

The cross-cutting guide's 85 local file links resolve and its 10 Mermaid
fences are balanced; this is structural validation only, not visual rendering
or grammar acceptance. The tenth map remains in the documented parse/render
backlog. Existing source and test references were reviewed, not rerun. No C++
builds, tests, CI, 2010 behavior, or Requirements Lab changes occurred.

Next handoff: audit the 2025 cross-cutting guide's support-switch map for the
reader distinction among federation policy, FDD-seeded member defaults,
member-local setters, and save/restore storage. Check authority, scope, and
sampling time against the pinned API and existing focused evidence; keep
profile behavior separate from normative claims and do not infer 2010 or
process parity. Reuse existing docs, and do not build, rerun tests, start CI,
or edit the Requirements Lab.

### 2025 support-switch scope versus sampling time — 2026-10-11

The support-switch map already distinguishes read-only federation policy,
FDD-seeded per-member defaults, and member-local setters, but its generic local
branch went straight from changing a switch to an effect. Added an
operation-specific consumer/read boundary and a short HLA-first explanation:
control authority and scope do not determine when an operation samples the
value, whether pending work is reconsidered, or when an effect is observable.
The examples stay bounded to selected embedded 2025 evidence: NRG creation
and the pending TAR, automatic-resign Join seeding and connection-loss use,
and service-report, exception-report, and DDM callback-metadata consumers.
They are explicitly not universal HLA timing or retroactivity rules.

The pinned API and linked time, lifecycle, DDM, reporting, and exception guides
were inspected as existing evidence; no source was changed and no tests were
rerun. Updated the API-family reader path to call out the sampling question.
No C++ build, test, or CI work occurred. No 2010 or Requirements Lab behavior
was inferred or changed. The support-switch map remains in the Mermaid
visual-review backlog; no rendering or grammar acceptance is claimed.

Next handoff: review the 2025 cross-cutting completion-boundary map as an
HLA-first explanation. Check that time-advance completion, synchronization
achievement, federation save/restore outcomes, and application callback
observation remain distinct contracts; attach exact standard evidence where
available and label implementation/test evidence separately. Do not infer
2010 parity or merge the protocols into one state machine. Reuse current
runtime evidence; do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 protocol completion versus callback observation — 2026-10-11

The original map grouped callback dispatch beside time advance,
synchronization, and save/restore as though all four coordinated federation
state. Reframed it as three distinct protocol outcomes plus an orthogonal
per-federate callback-delivery/observation boundary. The table now says which
callback reports each public outcome, and separately identifies the callback
model and Evoke services that determine when application code enters it. This
avoids treating dispatch as a fourth protocol or implying that one callback's
delivery policy changes the meaning of its originating result.

Added clause anchors from the pinned 2025 API declarations (§8.14, §4.18,
§§4.23/4.32, §4.2, and §§10.57–10.58). The official IEEE landing page confirms
the current 1516.1-2025 publication but does not expose the normative clause
text; the guide retains its instruction to verify full conditions in an
authorized copy. Source/test evidence remains in the linked time, callback,
synchronization, and save/restore guides; none was rerun or promoted to a
normative claim. Updated the reader index. No C++ source, build, tests, CI,
2010 stream, or Requirements Lab content changed.

The completion-boundary Mermaid remains in the six-diagram visual-review backlog.
The cross-cutting guide's local link/fence checks are structural only; no
Mermaid grammar acceptance or rendered readability is claimed.

Next handoff: review the 2025 cross-cutting guide's “Keep delivery properties
on separate axes” matrix as a durable HLA-first reading tool. Confirm that
sender authority, recipient relevance, order, transportation, and callback
dispatch remain separate questions, then identify which constraints are
service-specific and which are only profile evidence. Use the existing object,
DDM, time, and callback guides; do not invent a universal delivery pipeline or
merge 2010 behavior. No C++ builds, tests, CI, or Requirements Lab work.

### 2025 delivery axes: declarations, instance state, and observation — 2026-10-11

The matrix already kept order, transportation, DDM relevance, time eligibility,
and callback dispatch conceptually separate, but it merged two producer-side
questions into “Sender authority” and called callback dispatch a delivery
property. Expanded the checklist into separate rows for publication,
per-instance attribute ownership, receiver relevance, receiver object
knowledge, order, transportation, TSO/time eligibility, and application
callback observation. Added the explicit object-versus-interaction contrast:
an interaction has no object instance, so it has no per-instance attribute
ownership or receiver-known-object state. The surrounding prose now says these
are service-selected questions, not a universal linear pipeline.

The object/interaction and ownership guides directly support the publication,
ownership, relevance, and object-knowledge distinctions; the time,
order/transportation, DDM, and callback guides remain the bounded service paths.
Those guides and pinned headers were inspected, not rerun. The 2010 stream was
not consulted as parity evidence. No C++ source, build, tests, CI, or
Requirements Lab content changed. The cross-cutting update is documentation;
the existing flow diagrams were not edited. Local link/table checks remain
structural only: all local file targets resolve, the reader-path heading is
present, and all eight matrix rows are present. No Markdown table renderer or
GitHub visual preview was run.

Next handoff: audit the 2025 object/interaction guide's HLA-first four-question
orientation against the refined matrix. Check that publication, instance
ownership, receiver interest/relevance, receiver-known-object state, and
callback observation remain distinct in its ordinary object-update,
ordinary-interaction, and directed-interaction paths; identify where the
specific service contract selects different gates. Do not combine these into
one delivery pipeline, infer 2010 parity, or repeat source/test work. No C++
builds, tests, CI, or Requirements Lab edits.

### 2025 object and interaction orientation: five service questions — 2026-10-11

The object/interaction guide did keep the required concepts visible, but its
four-question list combined publication and per-instance ownership under
“May this federate produce it?” and treated object knowledge as if it were a
receiver-only question. The list now separates publication from attribute
ownership, and states the participant scope of object knowledge: object
producer and receiver state differ, while a directed send requires target
knowledge at both sender and eligible receiver. It also says ordinary
interactions have no object-instance knowledge and that directed publication
and recipient selection use their own target/pair rules. Callback observation
remains separate; order, transportation, and TSO eligibility are explicitly
left to their own service guides rather than forced into this short orientation.

The ordinary object, ordinary interaction, and directed paths were checked
against their existing diagrams and bounded evidence maps. The object guide
already contrasts publication/ownership and producer/receiver known-object
state; ordinary interaction has no instance ownership; directed routing has a
target-specific publication pair and receiver selector. This is an HLA-first
navigation clarification; the existing detailed flow charts and test/source
claims were not changed or rerun. Updated the cross-cutting matrix link to the
renamed section. No C++ source, build, tests, CI, 2010 behavior, or Requirements
Lab content changed.

Validation is limited to documentation structure: check the renamed heading
and all local link targets, then confirm the exact prose paths. No Mermaid
blocks changed or were rendered.

Next handoff: audit the 2025 object/interaction guide's object-removal section
for the separate HLA meanings of delete, receiver removal, and one federate's
local forget. Verify the API vocabulary and existing embedded/process examples
stay separately labeled, and that no object-ownership shortcut or 2010 parity
is implied. Reuse current evidence and do not build, rerun tests, start CI, or
edit the Requirements Lab.

### 2025 object-removal API roles and receiver state — 2026-10-11

The audit confirmed three distinct 2025 API roles: `deleteObjectInstance`
(§6.16) is the application's delete request, `removeObjectInstance` (§6.17)
is the RTI's receiver-side callback, and `localDeleteObjectInstance` (§6.18)
is a separate local service. Added these pinned-header links before the
implementation trace so readers meet the HLA/API distinction first. The
existing receive-order deletion and local-forget diagrams already show the
different state transitions; an additional overview chart would repeat them.

The bounded source review confirmed the embedded registry requires the delete
privilege owner, clears the deleting federate's known-object entry at accepted
deletion, and revalidates each receiver's reservation and knowledge before
committing removal. Local deletion checks pending ownership work and remaining
attribute ownership, then erases only the caller's known-object and pending
discovery entries. The callback gate is separate in embedded queued dispatch,
process pull, and optional process push. Focused evidence remains narrower:
the embedded receive-order case checks unknown-before-evoke and one recorded
`removeObjectInstance` callback; the process case checks the callback's object,
producer, and tag; the embedded local-delete case checks rejection guards,
rediscovery, and a later reflection;
the process local-delete test exercises a successful forwarded request and
recipient-local state removal. Process push is source-observed only. Resign
and connection-loss scenarios are separately labeled and do not establish a
complete action matrix.

No new diagram was added because the three existing object-removal views
already separate delete planning, recipient callback commit, and caller-local
forget. API names alone were not used to infer state effects; implementation
claims remain 2025/profile-bounded. No C++ build, test rerun, CI, 2010 behavior,
or Requirements Lab content was changed.

Validation: confirm the three pinned API references, target files, and section
anchors; run JSON parsing and `git diff --check`. Mermaid blocks were unchanged,
so no render was run.

Next handoff: review the 2025 FOM-module admission/composition guide for the
learner-facing boundary between per-module DIF validity, complete-model
composition, FDD validation, and the Create/additional-Join transaction. Use
existing pinned API, source, and focused-test evidence; identify a real HLA
reader gap before adding a diagram. Keep 2010 separate, reuse test results, and
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 HLA-first reader pass: name reservation and delivery properties — 2026-10-11

The reservation/peer-visibility synthesis now states the central conceptual
boundary directly: a name reservation authorizes its owner to attempt named
registration, but is not itself a live object or a peer-discovery event. The
owner's registration path and an eligible peer's known-object path remain
separate participant-local transitions. The combined chart still cites two
focused embedded `HLA_EVOKED` tests, not a single end-to-end assertion; its
local Mermaid and GitHub preview checks remain pending.

A bounded reader audit of the 2025 order/transportation guide found no further
content gap: it defines delivery order separately from transportation
category, distinguishes both from recipient selection, logical-time delivery,
and callback entry, and presents Umbra's scoped/effective-state examples only
after the HLA-facing model. The timestamped-service-with-`RECEIVE` result is
explicitly a focused implementation observation, not a standards ruling. No
duplicate diagram was added.

No C++ build, test rerun, CI, 2010 behavior, or Requirements Lab content was
changed.

Next handoff: audit the HLA-facing explanation in the 2025 TSO retraction guide.
Check that producer-side Retract time permission, per-recipient delivery and
Request Retraction, and each receiver's later time-advance result remain
separate questions. Keep profile-specific predicates labeled as Umbra
evidence, reuse existing tests, and do not build, rerun tests, start CI, or edit
the Requirements Lab.

### 2025 TSO retraction: separate reasoning lenses from RTI processing order — 2026-10-11

The bounded reader audit found the guide already distinguishes the HLA/API
roles for `Retract` and `requestRetraction`, the producer-side §8.22.3
question, recipient-specific original-delivery state, and receiver-specific
time-request outcomes. It also keeps the strict `current-or-pending time +
actual lookahead < message timestamp` predicate inside the Umbra evidence
boundary and leaves exact normative interpretation pending the licensed
clause text.

One wording could mislead a reader: “follow three independent questions, in
order” sounded like a required RTI execution sequence. It now says the table's
ordering is a reader aid, not a mandated processing order. The existing
focused-test evidence remains scenario-bounded; this edit did not rerun tests
or change any diagram. The six targeted Mermaid visual checks remain open.

No C++ build, test rerun, CI, 2010 behavior, or Requirements Lab content was
changed.

Next handoff: review the 2025 federation save/restore guide's HLA responsibility
map and service/callback vocabulary. Check that application-owned checkpoint,
RTI-owned federation checkpoint, request acceptance, participant completion,
and federation-level result stay distinct, while Join's unresolved §4.11
preconditions and participant effects remain explicitly unknown. Reuse current
evidence; do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 save/restore: service return and callback outcome — 2026-10-11

The HLA responsibility map and detailed save/restore flows already distinguish
application-owned state from RTI-owned federation state, local participant
reports from federation-level completion, request admission from later protocol
failure, and status-query snapshots from lifecycle progress. The exact Join
preconditions and participant effects remain explicitly unresolved under
§4.11; no exception-list-only conclusion is drawn.

Added a short API-grounded note: the pinned save/restore request methods return
`void`; restore admission and final restoration are distinct
`FederateAmbassador` callbacks. The prose does not infer that callback entry
must follow method return, since callback timing belongs to the selected
delivery model. Existing diagrams already express the checkpoint/responsibility
model, so no duplicate chart was added.

No C++ build, test rerun, CI, 2010 behavior, or Requirements Lab content was
changed. No Mermaid block changed; visual validation of existing diagrams is
not expanded by this text-only edit.

### 2025 exception reporting: caller and observer API boundary — 2026-10-11

A bounded reader review found the existing three-path table and decision maps
already separate the caller's service result from report production, recipient
eligibility, and callback observation. The guide also correctly bounds
`HLAreportException` to instrumented public-service catch sites, keeps
`HLAreportMOMexception` independent of the Exception Reporting switch, and
defers `HLAreportServiceInvocation` lifecycle to its adjacent guide. The pinned
2025 MIM declares the three interactions separately and records a wording
ambiguity under `HLAreportException`; the guide preserves that ambiguity rather
than silently treating implementation behavior as a correction.

Added a compact API-boundary explanation with links to the pinned
`RTIambassador::getObjectClassHandle` return/throws contract,
`RTI::Exception`, and `FederateAmbassador::receiveInteraction` overloads. It
states that an immediate callback may run before the service call finishes
unwinding without becoming the caller's service result. Existing diagrams
already communicate the routes, so no duplicate chart was added. The guide
index now records this API link and reader distinction.

No C++ build, test rerun, CI, 2010 behavior, or Requirements Lab content was
changed. No Mermaid block changed and no new rendering claim is made; keep the
six existing targeted visual checks open.

### 2025 ownership request/result boundary and callback timing — 2026-10-11

A bounded reader review found the ownership guide already explains
per-attribute state, request admission, callback-time state changes, and the
separate outcomes of mixed attribute sets. It did not explicitly connect those
ideas to the public C++ contract: the acquisition methods return `void`,
precondition failures are synchronous typed exceptions, and transfer outcomes
arrive through `FederateAmbassador` callbacks. Added that distinction and a
callback-model caveat, linking the pinned 2025 service/callback declarations
and the existing callback-ordering guide.

The cited If-Available case shows the requester still does not own an attribute
after the request returns, then observes ownership at its acquisition
notification. A focused HLA_IMMEDIATE process-boundary case shows the
confirmation-derived notification arriving during a subsequent handle lookup,
before that lookup returns; it does not show the requester callback running
inside the owner's earlier Confirm Divestiture call. These two observations
are explicitly bounded and do not establish a universal callback schedule.
Existing diagrams already show the state/outcome paths, so no chart was added.

No C++ build, test rerun, CI, 2010 behavior, or Requirements Lab content was
changed. No Mermaid block changed or was newly rendered; keep the six targeted
visual checks open.

### 2025 ownership persistence: HLA facts versus recovery machinery — 2026-10-11

A bounded reader review found the companion already keeps its three selected
save/restore and resignation traces narrow, explicitly separates 2010, and
defers the full normative `ResignAction` matrix to the standard and the
implementation summary to the lifecycle guide. Its state table did not label
the ownership ledger and callback route as Umbra implementation categories,
though, which could make those private recovery structures look like
HLA-defined stores. The table now says so directly while retaining membership
and attribute ownership as federation facts exposed through the API.

Added an API-boundary note: `resignFederationExecution(ResignAction)` returns
`void`; `OwnershipAcquisitionPending` is a synchronous typed service failure,
not an ownership callback or partial result. The linked embedded test shows
one rejected action followed by a successful retry using the combined
cancellation action. This remains an implementation/test observation, not a
complete normative action rule. The existing flowchart and sequences already
show the pending-work and surviving-recipient cases, so no duplicate diagram
was warranted.

No C++ build, test rerun, CI, 2010 behavior, or Requirements Lab content was
changed. No Mermaid block changed or was newly rendered; keep the six targeted
visual checks open.

### 2025 lifecycle API results and membership scope — 2026-10-11

A bounded HLA-first review found the lifecycle guide already distinguishes
three scopes: the local RTI ambassador connection, one federate membership,
and the federation execution. Its diagrams and text correctly keep Create
separate from Join, Resign separate from Disconnect and Destroy, and label the
zero-member execution and `ResignAction` effects as current embedded-profile
behavior where appropriate. The ownership pending-work guard stays in its
companion; no duplicate action chart was needed.

Added a concise pinned-API map of the caller-visible result types: Connect
returns `ConfigurationResult`, Join returns `FederateHandle`, and Create,
Destroy, Resign, and Disconnect return `void`. It explicitly separates the
returned member identity from another federate's later knowledge of the
`HLAfederate` MOM instance. The lifecycle evidence and diagrams remain bounded
to 2025; 2010 parity is not inferred.

The ninth lifecycle sequence (resignation rejected during an active save,
followed by survivor recovery) remains syntax-checked but awaits local visual
review; the existing backlog row retains this. Exact IEEE 1516.1-2025 §4.4
wording for the lost-regulator cutoff also remains unverified against the full
standard. No C++ build, test rerun, CI, 2010 behavior, or Requirements Lab
content changed. No Mermaid block changed; the six other targeted visual
checks remain open.

Next handoff: visually review the lifecycle guide's ninth
resignation/active-save sequence in a GitHub-compatible local Markdown preview.
Record parse and readability results in the guide backlog; change the chart
only if inspection finds a concrete render or comprehension defect. Do not
claim GitHub verification or resolve the §4.4 normative wording gap from local
rendering. Preserve the six other pending Mermaid checks, and do not build,
rerun tests, start CI, or edit the Requirements Lab.

### Lifecycle Mermaid preview availability checkpoint — 2026-10-11

The ninth lifecycle sequence remains syntax-checked only; this turn did not
produce a rendered image, so no visual-readability conclusion is recorded. The
browser rejected the attempted local `data:` preview under its URL policy and
explicitly prohibited alternate browser, indirect-execution, or protocol
workarounds. No chart was changed. The six other targeted Mermaid checks and
the exact IEEE 1516.1-2025 §4.4 wording question remain open. No C++ build, test
rerun, CI, 2010 behavior, or Requirements Lab content changed.

Next handoff: resume the HLA-first documentation survey at the 2025 TSO
message-retraction guide. Review its producer-legality, per-recipient
retraction, and producer-versus-current-owner explanations using existing
source and focused-test evidence only; do not compile or rerun tests. Leave the
ninth lifecycle visual review pending unless the user supplies a screenshot
from an approved Markdown preview. Preserve the six other Mermaid checks, the
§4.4 wording gap, and the 2010/2025 boundary; do not edit the Requirements Lab.

### 2025 TSO retraction: portable recipient outcomes versus embedded gates — 2026-10-11

A bounded HLA-first review found that the guide already opens with the portable
questions—producer time eligibility, each recipient's delivery phase, and each
receiver's independent grant result. Its producer-legality chart also labels
the HLA §8.22.3 question separately from Umbra's validation path, while the
ownership timeline leads with original producer identity versus current
attribute ownership. The latter stays bounded to selected embedded
`HLA_EVOKED` cases and does not claim retraction after source departure.

One reader gap remained in the per-recipient state chart: pending-versus-delivered
HLA outcomes sat next to `Suppressed` and callback-route admission without a
strong enough visual boundary. The guide now states the portable outcome model
first, then labels the detailed ledger/callback graph as embedded 2025 behavior;
implementation-only gates are explicitly marked. No additional diagram was
needed. Source and test bodies were inspected for the ledger states, live
membership gate, and ownership-transfer/resignation scenario limits; no tests
were run. Exact detailed §8.22–§8.23 timing remains subject to licensed text.

No C++ build, test rerun, CI, 2010 behavior, or Requirements Lab content
changed. The lifecycle screenshot review remains pending after the browser
refused the local preview; no alternate route was attempted. The six other
targeted Mermaid checks remain open.

Next handoff: audit the 2025 cross-cutting guide's support-switch map as an
HLA-first teaching tool. Check whether it starts from who controls federation
policy, where FDD-seeded member defaults come from, which switches are
member-local, and when values are sampled or saved before describing Umbra's
storage path. Reuse existing source/API/test evidence; do not compile, rerun
tests, start CI, infer 2010/process parity, or edit the Requirements Lab.

### 2025 support switches: authority, scope, and sampling — 2026-10-11

The cross-cutting guide already teaches the reader to classify a setting by
origin, API control surface, affected membership, and service-specific
sampling time before opening Umbra's storage map. Its examples are distinct:
getter-only federation policy, an FDD value copied into each member at Join,
and member-local setters. A separate section asks when each consumer samples
the setting; another labels save-image placement and restore matching as
embedded implementation evidence rather than a portable persistence rule.

A bounded source/test cross-check supports those boundaries: the 2025 API
declares the selected getter-only and setter operations; the composer reads the
selected federation settings; Join copies the per-member defaults; the
support-switch scenario shows one member's override does not alter its peer;
and the NRG scenario shows a later additional module does not change that
creation-time value or release the already-pending TAR. State-image capture
and restore keep selected federation and member fields in separate records.
These test bodies were inspected, not rerun. No content change or duplicate
diagram was warranted.

The support-switch Mermaid remains in the targeted render backlog; this review
does not claim parser acceptance, visual readability, or GitHub rendering. The
lifecycle screenshot review also remains pending under the browser restriction;
the six other targeted Mermaid checks remain open. No C++ build, test rerun,
CI, 2010 behavior, or Requirements Lab content changed.

Next handoff: review the 2025 save/restore guide's Join-during-active-operation
boundary. Separate the portable §4.11 contract question from the embedded
adapter/registry path and the existing interlock case, which joins before
requesting save. Keep exact §4.11 wording open unless the authorized standard
text is available; reuse current source and test evidence without rerunning
tests. Preserve the 2010 boundary and leave the Requirements Lab untouched.

## 2025 save/restore Join boundary disposition — 2026-10-11

The existing guide already separates the portable §4.11 question from the
bounded embedded adapter/registry observation, the distinct process Join
handler, and the representative interlock test. The pinned API lists
SaveInProgress and RestoreInProgress for both Join overloads, but does not
establish their precise trigger, ordering, or participant effects. The
representative test joins both participants before requesting save, so it does
not exercise Join during an active save or restore. The inspected implementation
paths contain no explicit phase predicate in the reviewed ranges; this remains
a profile/source observation, not a normative conclusion or proof of all
interleavings.

No guide text or diagram change was warranted: the learner-facing boundary and
the open question are already stated directly. The public IEEE product page
offers purchase/subscription access but exposes no §4.11 clause text, so the
exact contract remains open pending an authorized copy
([IEEE SA product page](https://standards.ieee.org/ieee/1516.1/6688/)). No
build, test rerun, or CI was performed. No 2010 behavior or Requirements Lab
content changed. Existing Mermaid render/review items remain pending; no
rendering result is claimed.

Next handoff: continue the content-first learner-gap survey with the 2025
attribute-update-rate admission guide. Check whether it clearly distinguishes
the HLA meaning of a selected rate and admission decision from actual update
delivery and callback observation, and labels implementation sampling only
where it explains that distinction. Reuse pinned API, focused source, and
existing test evidence; change the guide only for a concrete reader gap. Do
not build, rerun tests, or start CI. Preserve the 2010/2025 boundary, pending
Mermaid reviews, and Requirements Lab exclusion.

## 2025 update-rate names, values, and observations — 2026-10-11

The update-rate guide already separates the HLA/API designator from selected
embedded rate-combination/admission behavior and from delivery/callback
observation. A bounded review of the pinned API, 1516.2 update-rate row schema
and example, registry query, and existing query test found one reader gap:
the guide listed the designator and numeric query methods without explicitly
connecting the FDD name/rate pair to their different roles.

Added a small API/model vocabulary table: a subscription supplies the
model-defined name; the named query returns its numeric value; the
object/attribute query is a separate query, not evidence of a callback. Its
current embedded aggregation across active ordinary/regional declarations
and exclusion of passive declarations are explicitly labeled source-specific.
The existing three Mermaid flows remain unchanged; they already cover the
larger selection, admission, and delivery boundaries, so a fourth chart would
repeat them. The exact normative rate-combination semantics remain open
without the authorized clause text. No tests were rerun; no build, CI, 2010
behavior, or Requirements Lab content changed. No Mermaid parse/render result
is claimed.

Next handoff: review the 2025 [attribute-value update-request
guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md) for the portable
HLA distinction among request scope, provider solicitation, value response,
and application observation. Check that the MOM special case and save/restore
stages stay distinct from ordinary object update delivery. Reuse existing
API/source/test evidence; add no duplicate chart if its current flows already
teach the roles. Do not build, rerun tests, or start CI. Preserve the
2010/2025 boundary, pending Mermaid reviews, and Requirements Lab exclusion.

## 2025 attribute-value request: solicitation and response boundary — 2026-10-11

The guide already leads with the portable service distinction: an explicit
request solicits a provider but does not transfer attribute values. Its
request-scope and provider/object planning flows are separately labeled as
current embedded behavior, while the sequence keeps the provider's callback,
later update, and receiver reflection distinct. It also distinguishes the
request tag echoed to the provider from the tag on the provider's later update,
and keeps MOM snapshots and restore cut points on separate paths.

A bounded check of the pinned 2025 API declarations (§§6.21, 6.22, 6.27) and
the existing timestamped class-response test supports those boundaries: the
provider callback can be delayed under HLA_EVOKED; the provider sends a
timestamped update with a separate response tag; and the tested constrained
receiver observes reflection before its matching grant. That is a focused
embedded case, not a general ordering rule. The guide already explains these
roles and evidence limits, so no wording or diagram change was warranted. The
2010 API was not used to infer 2025 behavior or parity. Tests were inspected,
not run; no build, CI, 2010 behavior, or Requirements Lab content changed.
No Mermaid render or parse result is claimed.

Next handoff: review the 2025 attribute-ownership guide at the boundary with
pending attribute-value requests. Check that the portable ownership question
(who has authority to supply each attribute value) stays distinct from the
embedded planner's current-provider grouping and callback-boundary
revalidation. Compare the existing ownership-transfer evidence with the
request guide; add a cross-link or diagram only if it closes a concrete
learner gap. Do not build, rerun tests, or start CI. Keep the 2010/2025 and
embedded/process boundaries explicit, preserve pending Mermaid reviews, and
leave the Requirements Lab untouched.

## 2025 attribute ownership and pending-request boundary — 2026-10-11

The request guide had a concrete reader gap: “recheck eligibility” did not say
that the current embedded class-request path checks against the provider chosen
when work was planned. The guide's existing callback decision now distinguishes
the durable HLA idea (per-attribute authority to supply a value) from that
implementation example: at callback start the pending requester/provider pair
is matched, attributes no longer owned by that provider are removed from the
projection, a nonempty remainder goes to the same provider, and an empty
projection is suppressed instead of retargeted. The ownership guide is now a
direct related-guide link.

The evidence map links the pending-state check, current-owner projection, and
runtime delivery gate. The ordinary focused request baseline shows normal
callback delivery and excludes requester-owned attributes, but does not test
ownership transfer between planning and callback dispatch. This distinction is
source-derived for the embedded 2025 path; it does not state a portable HLA
retargeting rule or establish process-profile parity. No test was run, and no
build, CI, 2010 behavior, or Requirements Lab content changed. The existing
Mermaid rendering backlog remains open; no parse or visual-review result is
claimed.

Next handoff: survey the 2025 end-to-end attribute-value path across the
ownership, subscription eligibility, update-rate admission, request
solicitation, ordinary update, and time-management guides. Decide whether
readers need one HLA-first map separating authority to supply, receiver
interest, admission/scheduling, callback observation, and reflection. Reuse
existing diagrams and evidence; synthesize only if a real comprehension gap
remains. Treat Umbra source as bounded case-study evidence, not the headline.
Do not compile, rerun tests, or start CI. Keep 2010 separate, label embedded
and process boundaries, preserve pending Mermaid reviews, and leave the
Requirements Lab untouched.

## 2025 attribute-value concepts across guides — 2026-10-11

The separate 2025 guides already explained the essential attribute-value
questions, but did not present one reader-centered overview connecting them.
Added a cross-cutting concept map with an optional request-to-provider
solicitation branch, independent supplier and receiver questions, RO/TSO and
transport distinctions, the profile-specific rate example, and callback
observation. Dotted links are explicitly not service ordering; exact
applicability and order remain service-contract questions.

The map links to existing ownership, object/interaction, subscription, DDM,
request, rate, time, and callback guides rather than duplicating their diagrams
or evidence. Their existing source/test coverage remains the bounded support;
the new map makes no new implementation or normative claim. No build, test,
CI, 2010 work, or Requirements Lab edit occurred. Mermaid was not rendered or
parsed, so no visual verification is claimed.

Next handoff: review the 2025 interaction-delivery guides for ordinary
class-based publication/subscription versus directed-interaction targeting,
including receiver and parameter selection and callback observation. Change
the documentation only if a concrete learner gap remains. Do not compile,
rerun tests, or start CI. Preserve edition/profile boundaries and keep the
Requirements Lab out of scope.

### 2025 attribute delivery cross-guide synthesis review — 2026-10-11

Cross-read the attribute-value update, update-rate admission, ownership, and
subscription guides against the pinned public API and the HLA-first
object-information/cross-cutting maps. The existing material already separates
the sender's service outcome from per-attribute authority, the receiver's
interest/eligibility from admitted projection, rate admission from logical-time
eligibility, and callback invocation from application observation. Provider
solicitation is also explicitly distinct from a later value update. No new
synthesis diagram was warranted; the ownership guide now links readers to the
existing cross-cutting object-attribute map.

The public API surface supports the service/callback distinction but is not
complete normative service prose. The direct-requester entitlement question
for Request Attribute Value Update remains open in its focused guide; this
review did not infer a standard rule. Embedded/process paths and the 2010
stream remain separate. No source, diagram, build, test, CI, or Requirements
Lab changes were made. A Scoop probe still finds no `mmdc` manifest; five
diagrams added after the 242-diagram render baseline remain pending local
render/layout review.

Next handoff: render and inspect the five post-baseline diagrams: the
HLA-level reservation lifecycle in the object-instance naming guide; the
object-class regional ancestry and process-endpoint regional object flows in
the DDM guide; and the HLA-first public-effect and process-mapping sequences
in the process-endpoint guide. Keep syntax parsing, rendered output, visual
inspection, and post-publication GitHub rendering as separate evidence.
Scoop has no `mmdc` manifest; wait for authorization before using another
installer. Do not compile C++, rerun tests, start CI, or edit the Requirements
Lab.

## 2025 ordinary versus directed interaction mental model — 2026-10-11

Cross-read the interaction orientation map and the ordinary and directed
delivery flows against the pinned 2025 API and targeted registry planners.
The existing side-by-side table already teaches the durable service boundary:
ordinary sends identify an interaction class and are routed through class
subscriptions, while directed sends add a target object context and use
object-class/interaction-class declarations plus the receiver's universal or
by-ownership selector. The API callback shapes reinforce that distinction:
ordinary `receiveInteraction` has no target handle; `receiveDirectedInteraction`
does.

The more detailed routing rules are explicitly labeled as current
implementation evidence. The embedded ordinary planner has a separate
federation-wide active-relevance gate and per-recipient class matching,
including its current closest-class parameter projection. The directed planner
checks target knowledge, declaration applicability, and selector, while its
current candidate retains the sent interaction class. The API signatures alone
do not establish every routing rule, and the focused tests do not establish
ancestor-declaration parity for directed routing. The existing links lead to
those details and to separate timestamp/callback boundaries, so another
diagram or cross-link would duplicate rather than improve the current reader
path. No new normative inheritance, 2010/2025, or embedded/process parity claim
was added. No source, build, test, CI, or Requirements Lab change was made.

The five post-baseline diagrams remain pending visual inspection. Mermaid CLI
12.0.0 is present in the isolated visualization workspace, but Chrome failed to
launch there with both the existing and a fresh isolated profile. No package
was installed and no C++ build capacity was used; treat renderer troubleshooting
as a separate backlog item, not a reason to pause documentation work.

Next handoff: conduct a bounded 2025 reader-first audit of service failures and
callback failures across the exception-reporting, service-invocation-reporting,
and callback/service-ordering guides. Use the pinned API and targeted existing
source/test evidence to separate a synchronous service exception from a later
`FederateInternalError` callback path and from process-profile transport/report
details. Do not infer a general rollback or atomicity guarantee; add a diagram
only if a durable HLA distinction is missing. Do not compile, rerun tests, or
start CI. Keep 2010 separate, label 2025 embedded/process evidence, preserve
the five-diagram render backlog, and leave the Requirements Lab untouched.

## 2025 callback exception versus Evoke service exception surface — 2026-10-11

The three guides and the cross-cutting state/event guide already distinguish a
typed exception from a public service, a callback exception, any report about
either event, and the operation-specific state that may already have changed.
They also keep the immediate call stack separate from later Evoke dispatch and
avoid a general rollback or reporting rule. No additional conceptual diagram
was warranted.

One concise API boundary was missing from the callback/service-ordering guide:
the pinned `receiveInteraction` callback declares `FederateInternalError`, but
the pinned `evokeCallback` and `evokeMultipleCallbacks` `@throws` lists do not.
The guide now says to read those method contracts separately. Targeted 2025
source shows Umbra's callback session rethrows and its Evoke path does not catch
that callback exception, so it can escape in the current implementation. This
is not a normative propagation finding; the exact licensed clause text was not
available, and there is no focused throwing-callback Evoke test. The API and
source references are linked at the new paragraph. No code, build, test, CI,
2010, or Requirements Lab work was performed.

The render backlog remains five diagrams: the installed isolated Mermaid CLI
could not start Chrome with existing or fresh profiles. This does not block
documentation work.

Next handoff: survey the 2025 save/restore guides for a reader gap in what
happens to different classes of in-flight work. Compare the main save/restore
map with the focused ownership, request-response, time/retraction, and callback
guides; classify only what their current evidence supports (for example,
preserved, canceled, re-derived, or still pending), and do not imply one
universal persistence rule. Add a compact comparison or flow only if it helps
readers choose the correct protocol-specific guide. Use existing source/test
results without compiling or rerunning tests; keep 2010 and embedded/process
boundaries explicit and leave the Requirements Lab untouched.

## 2025 save-restore in-flight work comparison — 2026-10-11

The audit found a useful navigation gap rather than a missing save-state
diagram. The main guide already explains the federation-level save/restore
barrier and distinguishes the RTI image, live callback routes, selected
temporal queues, and other typed service ledgers. The focused guides then show
different outcomes at different cut points: pending provider solicitation can
be reconstructed while an already delivered receive-order response is not
replayed; selected ownership facts are validated and replanned against live
members/routes; selected TSO payload and recipient-phase/retraction state
remains subject to time and delivery gates; and an already-pushed process
callback frame can remain in a live `HLA_EVOKED` queue across service-level
save/restore without being part of the durable image.

Added a compact comparison index to the main 2025 save/restore guide, linking
each category to its focused request-response, ownership, time-management, or
TSO-retraction guide and stating the scenario boundary. A new flowchart would
falsely suggest these are mutually exclusive stages of one protocol, so the
comparison table is the clearer representation. The text expressly avoids a
universal persistence rule, crash-recovery inference, or 2010 parity claim.
No new normative interpretation was introduced. No C++ source, build, test,
CI, or Requirements Lab work occurred.

The five existing local-render checks remain separate. The installed Mermaid
CLI's Chrome launch failure is unchanged; Scoop installation was not needed
for this content audit and would not by itself establish that browser-launch
issue is resolved.

Next handoff: continue the 2025 documentation survey with the
save/restore guide's unresolved Join-during-operation boundary, using exact
IEEE 1516.1-2025 §4.11 text from an authorized source before making any
normative statement. Keep participant admission distinct from synchronization
achievement, ordinary membership, and time grants. If the clause text is not
available, do not infer the rule from Umbra source/tests; select another
bounded HLA concept instead. Reuse existing evidence without C++ builds,
test reruns, or CI; keep 2010 and embedded/process boundaries explicit, retain
the five render checks, and leave the Requirements Lab untouched.

## 2025 five-diagram render verification — 2026-10-11

The five diagrams that were missing from the 242-block baseline now render
successfully with the already-installed Mermaid CLI 12.0.0 and system Chrome:
the HLA-level object-name reservation lifecycle; DDM object-class ancestry;
the DDM process-endpoint regional-object path; the process guide's HLA-level
untimed update/reflection sequence; and its process interaction request/event
mapping. Each was rendered to PNG and screenshot-inspected. The two process
sequences initially had overlong labels; those labels were shortened and the
charts rerendered. A semicolon in one sequence message was also corrected
after Mermaid parsed it as a statement separator.

All 247 diagrams across the current 34 diagram-bearing design files now have
local renderer output (the 242-block baseline plus these five). This is not a
full visual review, and GitHub's renderer remains unverified. The first
reservation render attempt accidentally passed the Markdown fence along with
the diagram body to a `.mmd` input; passing only the body fixed that parse
error. The CLI was already available in the task's visualization workspace, so
no Scoop install was needed. No C++ source, build, test, CI, 2010, or
Requirements Lab work occurred.

Next handoff: conduct a bounded HLA-first audit of the object-information
lifecycle across registration/discovery, subscriptions, initial values, and
the first value update. Compare the object-information, subscription-state,
Auto Provide, DDM, and process-endpoint guides. Focus the existing evidence
gaps—ordinary known-object/new-attribute behavior and ordinary
process-profile initial-value outcomes—and add a comparison or flow only if it
resolves a reader gap. Reuse existing source/test results without builds or
reruns; preserve the 2010/2025 boundary, do not edit the Requirements Lab, and
keep GitHub rendering as a separate post-publication check.

## 2025 object-information lifecycle audit — 2026-10-11

Compared the HLA-facing path across the object-and-interaction information,
subscription-state, Auto Provide, DDM, and process-endpoint guides. The current
set already traces the lifecycle without collapsing the distinct transitions:

- Publication and registration do not themselves equal receiver discovery;
  the object-information guide models receiver-specific knowledge and the
  registration/discovery route.
- A subscription or DDM change can make an existing, not-yet-known object
  discoverable. The subscription guide separately describes its bounded
  embedded initial-current-value observation and the known-object/new-attribute
  question.
- Initial reflection, Auto Provide's provider solicitation, and a provider's
  later update are separate effects. The Auto Provide guide distinguishes them
  and labels its trigger/order as embedded-profile evidence.
- The process guide separates the public update result from receiver callback
  observation. The subscription guide's process lane separately records the
  ordinary initial-value evidence gap and the RTI-owned MOM exception.
- The DDM guide owns regional eligibility and value projection; it is a filter
  on the route, not another lifecycle state.

Source spot-checks support the documented limits: the shared discovery planner
skips receivers whose known-object map already contains the instance
([planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L99));
the inspected subscription mutation returns scope/advisory work rather than a
current-value recipient plan
([registry mutation](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L260));
the embedded ordinary current-value plan is invoked after discovery enters
user code
([callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L375));
and the process discovery enqueue path constructs a separate initial-value
event only for RTI-owned MOM instances
([process path](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90)).
These are bounded Umbra source findings, not the missing normative §5.8
interpretation or a process-parity conclusion. Existing diagrams/tables already
make the important reader distinctions, so this audit added no duplicate map
and changed no behavioral evidence.

No C++ source, build, test, CI, 2010, or Requirements Lab work occurred. The
next evidence needed is the exact authorized IEEE 1516.1-2025 §5.8 text, to
distinguish a subscription that newly discovers an unknown instance from a
subscription adding attributes after that same instance is already known;
then retain the ordinary process route as a separate open question.

## 2025 subscription-guide screenshot attempt — pending — 2026-10-11

The subscription-state guide's passive-recipient and late-subscription charts
remain pending screenshot review. A guide-wide run with Mermaid CLI 12.0.0
failed before producing any PNGs: Chrome reported that the configured profile
was already running. Retrying with serial jobs and then with Puppeteer's
ephemeral profile produced the same launch failure. No browser process was
terminated, and no visual finding is claimed. This does not change the prior
247-diagram local-render result; it records a renderer-startup failure, not a
diagram parse failure. Continue substantive documentation work without
repeating this launch attempt unless the browser/profile state changes.

## 2025 subscription chart Edge fallback — pending — 2026-10-11

The user authorized installing Mermaid CLI if needed. The configured Scoop
buckets had no `mmdc` package, so Mermaid CLI 12.0.0 was installed globally
through the already-installed Scoop Node.js LTS. A targeted render of the
late-subscription chart then failed before rendering with Puppeteer's browser
launch error when pointed at system Edge; the earlier Chrome-profile and
ephemeral-profile launches had also failed. No image was produced and no
browser process was terminated. The existing 247/247 local-render evidence is
unchanged; screenshot review remains pending and is not claimed. No C++ source,
build, test, CI, 2010, or Requirements Lab work occurred.

Next handoff: do not repeat renderer installation or browser launch unless the
browser/runtime state changes. Continue with a bounded HLA-first survey for a
specific unanswered reader question; do not add another diagram when existing
flows already explain the behavior. Keep the ordinary §5.8 late-subscription
normative question open until authorized text is available, and preserve
edition/profile boundaries.

## 2010 reference-profile reader-gap audit and renderer follow-up — 2026-10-11

Cross-checked the separate 2010 reference-RTI guide's flow inventory and
evidence table against its cited implementation and registered
`ieee1516_2010_binding_shell_smoke.cpp`. Existing diagrams already explain the
implemented Connect/Join, object, interaction, synchronization, callback, and
time-value support boundaries. The smoke is one successful two-member
`HLA_IMMEDIATE` scenario, not a broad state-machine test. Empty-attribute
subscription discovery, partial callback-failure fan-out, and synchronization
barrier details remain source-only or untested; time-value marshalling does
not establish time-managed execution. Disposition: no additional 2010 guide or
diagram is justified by this evidence, and no 2025 behavior fills those gaps.

For the pending 2025 subscription screenshots, Mermaid CLI 12.0.0 was installed
via the existing Scoop Node.js after no configured Scoop manifest was found.
Enabling Puppeteer's browser setup did not make managed Chromium launch (exit
code 2147483651); the earlier system Chrome and Edge launches also failed.
A loopback-only preview could not be reached from the app-managed browser and
produced no image. No browser process was terminated. Keep all screenshot
checks pending; this does not change the 247/247 local-render result or prove
GitHub rendering. No C++ source, build, tests, CI, Requirements Lab, or 2010
work was changed.

Next handoff: pursue a new guide change only for a specific HLA question not
already answered by the portfolio. Reopen 2010 scope only when a new 2010
implementation/evidence path warrants it. Keep exact 2025 §5.8 and §4.11
normative questions open until authorized text is available; do not retry the
renderer unless a supported browser/preview environment changes.

## 2025 synchronization achievement sequence revision — 2026-10-12

Revised the existing fan-in sequence to separate a normally accepted
`successfully=false` participant report, the aggregate `federationSynchronized`
callback, and a later `SynchronizationPointLabelNotAnnounced` service exception.
The diagram count is unchanged. Current checks found no `mmdc` command or
installed Scoop Mermaid package; the authorized `scoop install mmdc` attempt
found no manifest in configured sources. No Mermaid render or screenshot was
produced, so syntax/render and visual inspection remain pending; no parse failure
or GitHub rendering result is claimed. No C++ build, test rerun, CI, 2010, or
Requirements Lab work occurred.

## 2025 FOM additional-module Join failure boundary revision — 2026-10-12

Extended the existing embedded Join flow beyond the registry commit. The
registry swaps the fully prepared definition and handle directories together
with the new membership, but later ambassador service-report/lifecycle setup
can fail and its rollback resigns only the new member. The focused writer
creation failure case is an ordinary Join and asserts member-name reuse; it
does not exercise additional modules or inspect the catalog after a
post-commit failure. The revised flow is therefore explicitly source-observed
and untested at that edge, not a portable HLA guarantee. No new Mermaid fence
was added; the existing flow was revised and remains unrendered because `mmdc`
is unavailable and the configured Scoop sources have no manifest. No remote
rendering, C++ build, test rerun, CI, 2010, or Requirements Lab work occurred.

Next handoff: audit the existing 2025 Federation and Federate Lifecycle guide
for the HLA-first membership mental model across Join, resignation, connection
loss, and execution destruction. Keep 2010 separate and do not build or rerun
tests.

## 2025 federation membership state-chart correction — 2026-10-12

The local ambassador state chart now shows the rejected Disconnect self-loop
for a joined federate. The pinned 2025 API declares `FederateIsExecutionMember`,
and the selected lifecycle test asserts the exception before the same member
resigns. The lifecycle overview also cross-links the FOM guide's implementation
boundary for a post-commit additional-module Join setup failure; that edge is
source-observed and not covered by a focused additional-FOM failure test. No
new diagram was added; the existing state chart changed and remains unrendered.
Scoop's authorized network search found no `mmdc` package in configured sources,
so no renderer was installed. No C++ build, test rerun, CI, 2010, or Requirements
Lab work occurred.

Next handoff: audit the existing 2025 Data Element encoding guide against the
pinned API and focused codec evidence. Keep the 2010 codec and evidence
separate; do not build or rerun tests.

## 2025 DataElement carrier/value-length distinction — 2026-10-12

The first encoding flow now names the returned octets' public
`VariableLengthData` carrier. Added a short API map distinguishing fresh
`encode()`, source-observed replacement through `encode(VariableLengthData&)`
in selected basic/composite codecs, and append behavior for `encodeInto`.
Also made the independent length layers explicit: carrier `size()`, an
in-payload `HLAvariableArray` element count, and Umbra process-frame metadata.
The selected variable-array test asserts the encoded count and append path;
separate `VariableLengthData` tests cover copy/storage semantics but do not
assert replacement of prefilled output by the encoder overload. Copy and
callback pointer-lifetime details link to the object guide. The 2010 companion
was not edited and no parity is inferred.

Seven diagrams in the guide had prior local render/screenshot review; the
modified first diagram has not been rerendered. Authorized Scoop search found
no `mmdc` package in configured sources, so the render remains pending. No
remote render, build, test rerun, CI, 2010, or Requirements Lab work occurred.

Next handoff: audit the existing 2025 Federation MOM guide for the HLA-first
distinction between execution-scoped management objects, member-scoped objects,
and observer knowledge. Preserve 2010 separately and do not build or rerun
tests.

## 2025 Federation MOM projection versus execution lifecycle — 2026-10-12

Clarified that the existing Federation MOM state chart tracks projection
establishment, not federation-execution existence: Create may leave an empty
execution before the first Join. Linked the lifecycle guide's distinct
absent/empty/joined state flow and explicit Destroy. The chart itself did not
change, so no renderer or screenshot check was needed. 2010 remains separate;
Join ordering and failed-member-setup retention are source-observed, and no
focused failed-first-Join test or process parity is claimed.

Next handoff: audit the 2025 service invocation reporting guide for the
synchronous service outcome versus invocation-report and later MOM
request/report boundaries. Do not build or rerun tests.

## 2025 invocation reports versus requested Federation MOM reports — 2026-10-12

Added a focused three-trace comparison to the service-invocation guide and a
reciprocal link from the Federation MOM guide. The 2025 MIM's
`HLAreportServiceInvocation` describes an invocation when the subject's
Service Reporting is enabled; an explicit `HLArequest...` produces a separate
RTI `HLAreport...` about requested state. The table keeps the public call's
own return/exception distinct, while preserving the source-observed Umbra
file-append failure caveat (the cited writer test run failed before its
relevant assertions) and the separate observer callback boundary. No diagram
changed and no rendering was needed. The focused test bodies were inspected, not run;
they do not establish complete callback-model or process parity. No 2010 work.

Next handoff: audit the 2025 exception-reporting guide's boundary between
service exceptions and `HLAreportException`, `HLAreportMOMexception`, and
failed service-invocation reports. No build or test rerun.

## 2025 exception-report scope and report-family boundary — 2026-10-12

Scoped the report comparison so its three paths are not mistaken for an
exhaustive inventory of MIM report interactions. Added a short link to the
separate request-driven Federation MOM report flow, supported by the pinned
2025 MIM's request/report pair. Existing exception diagrams were unchanged;
no new render or screenshot review was needed. Test sources were inspected
but not run. The combined process two-report outcome remains source-derived
and untested; no embedded/process or 2010 parity is claimed.

Next handoff: audit the 2025 Object and Interaction Information Flow and
Object Attribute Subscription State guides together for a real cross-guide
gap; do not duplicate their flows or rerun tests.

## 2025 object information and subscription-state wayfinding — 2026-10-12

Added a scope-level backlink from the focused subscription-state guide to the
end-to-end object/interaction information map. The broader guide already
links into the focused subscription and discovery details. Existing diagrams
were unchanged; no new render or screenshot review was needed. The inspected
test evidence remains bounded to its embedded examples; ordinary process
late-value behavior and normative §5.8 remain open. No 2010 equivalence.

## 2025 explicit request versus Auto Provide boundary — 2026-10-12

Reviewed the two guides' HLA-facing distinction, detailed flows, evidence
limits, and reciprocal wayfinding. The explicit service carries the caller's
request tag and uses its own pending-request path; discovery-triggered Auto
Provide uses the current embedded path's empty callback tag and separate
switch/scope/ownership recheck. Both distinguish solicitation from the
provider's later value-bearing update, and Auto Provide's guide separately
marks initial reflection. The focused embedded baseline test sources support
the tag and discovery observations but do not establish process behavior or a
provider response guarantee. No diagram changed, so rendering and Mermaid CLI
were unnecessary. Test sources were inspected, not run; no build, CI, 2010, or
Requirements Lab work.

## 2025 object-name reservation versus local handle knowledge — 2026-10-12

Reviewed the paired guides' diagrams, evidence boundaries, and reciprocal
links. The reservation flow separates the owner's reservation/registration
commit and returned handle from peer discovery; the lookup flow treats
forward/reverse object-instance resolution as federate-local known-object
state. Two focused embedded test sources support the two-view synthesis but
do not assert it as one end-to-end trace. The process lookup evidence is an
owner round trip only and does not establish peer-discovery parity. Existing
charts already make the distinction clear, so none was added or changed and
no rendering was needed. No build, test rerun, CI, 2010, or Requirements Lab
work.

## 2025 model extension versus name-to-handle resolution — 2026-10-12

Reviewed the paired diagrams, cross-links, and bounded API/test evidence. The
FOM guide explains replacement preparation, registry commit, and the
source-only post-commit failure edge; the name/handle guide illustrates the
existing member's post-Join lookup of added declarations. Focused embedded
tests observe same-execution typed-handle equality for earlier classes and
resolution of new classes. The configured-process example covers its joining
client only. The views complement each other; no new chart or text change was
needed, so no render was required. Test sources were inspected, not run; no
build, CI, 2010, or Requirements Lab work.

## 2025 model vocabulary versus DataElement value encoding — 2026-10-12

Compared the FOM composition guide with the 2025 DataElement Encoding guide.
The latter already distinguishes FOM/OMT model input from public value-octet
encoding and links to model composition. Added the missing return pointer at
the FOM guide's model-vocabulary/runtime boundary, carefully stating that
composition does not execute a federate's encoder and avoiding any one-to-one
FOM-type/codec claim. The pinned 2025 API exposes separate encode/decode
operations. No diagram changed and no render was needed; no build or tests were
run, and no 2010 behavior is inferred.

## 2010/2025 DataElement implementation-boundary audit — 2026-10-12

Compared both edition guides with the pinned DataElement headers, distinct
codec sources, common byte-order and composite helpers, and selected focused
test bodies. The signatures differ (`auto_ptr`/`void decode` in 2010 versus
`unique_ptr`/`DataElement& decode` in 2025), while both wrappers pass explicit
BE/LE choices to the stateless byte-order helper. Found and documented the
shared checked-size/range/padding primitives behind the separate
`composite_2010` and `composite_2025` adapters. Codec translation units and
test targets remain edition-specific; no broad encoded-format parity is
claimed. No diagram changed or render was required. Test sources were
inspected, not run; no build or CI.

Next handoff: audit the 2025 Exception Reporting guide alongside the Callback
and Service Ordering guide for synchronous service exceptions,
HLAreportException callbacks, and exceptions thrown by user callback code.
Keep process and embedded paths distinct.

## Callback model versus process endpoint event delivery — 2026-10-12

The paired guides already state that callback-model dispatch (HLA_IMMEDIATE or
HLA_EVOKED) is independent of whether the process service queues or pushes a
receive-order event. The callback guide also describes its separate
recipient-side callback exception boundary. Added a scope-level pointer to the
process trace so readers can find those Umbra-specific mechanics without
mistaking them for the HLA callback-model contract. The existing diagrams
adequately teach the distinction; no diagram was added or changed and no render
was needed. No build or tests were run; 2010 and Requirements Lab work stayed
out of scope.

## Exception-report callback failure boundary — 2026-10-12

Source confirms that the 2025 `noexcept` exception-report hook catches a user
callback exception only if it propagates back through that hook's synchronous
report attempt. In embedded `HLA_IMMEDIATE`, such a throw can stop the current
recipient-submission loop; the recipient time-state gate can defer callback
entry independently. Queued, evoked, or separate process-recipient callbacks
escape through their own dispatch stack. Added a focused sequence flow and
linked it from the callback guide. The cited process integration case
uses a non-throwing receiver, so the throw behavior is not test-proven. The new
diagram remains unrendered because the configured Scoop buckets have no `mmdc`
manifest; no GitHub visual check is claimed. No build or tests were run.

## 2025 DDM empty-region-set and object/interaction boundary audit — 2026-10-12

Compared the DDM/Regions guide with Object and Interaction Information Flow and
bounded 2025 source/test evidence. Existing diagrams already separate object
attribute association/discovery/value routing from ordinary interaction
subscription/send-region routing, and keep overlap, callback observation, and
logical-time eligibility distinct. The concrete reader gap was three meanings
of “empty”: a zero-dimensional Region object, an empty RegionHandleSet passed
to Send Interaction With Regions, and an empty region-designator set conveyed
on an ordinary interaction callback. Updated the DDM guide with a three-way
comparison and revised the existing interaction routing chart to show that the
explicitly empty service-input set is a no-send operation with no ordinary
default-region fallback. The focused 2025 test holds an ordinary subscription
active and asserts that the empty regional send produces no receive callback;
the receive-order planner returns before recipient selection. Added reciprocal
scope-level wayfinding to the Object/Interaction guide. No new diagram or
inventory-count change.

The revised Mermaid chart has not been rendered or visually reviewed. The user-
authorized Scoop install of `mmdc` found no manifest; `scoop search mermaid`
also found no package. No remote render was used. No build or tests were run;
2010 and Requirements Lab work remained out of scope.

## 2025 save/restore responsibility and temporal-boundary audit — 2026-10-12

The Save/Restore, Federation and Federate Lifecycle, and Time Management guides
already distinguish application-owned checkpoints from RTI federation images
and terminal outcomes, Join membership from protocol participant sets, delayed
save initiation from request acceptance, and callback observation from temporal
eligibility. The lifecycle guide explicitly does not transfer the expandable
synchronization-point Join rule to save/restore; exact Join preconditions and
effects during those protocols remain open.

Added reciprocal, section-level wayfinding between Time Management's deferred
receive-order queue limitation and Save/Restore's selected in-flight-work
matrix. The matrix distinguishes serialized TSO state from callback frames
already admitted to a live process queue; the latter survive one selected
service-level save/restore case but are not durable-image records and do not
establish restart recovery. Tightened Save/Restore's links back to its specific
time/callback map and TSO message lifecycle. No diagrams changed or were added,
so no render was needed. The focused process test source was inspected, not run;
no build, test rerun, CI, 2010, or Requirements Lab work occurred.

## 2025 synchronization participant-set boundary audit — 2026-10-12

Compared the 2025 Synchronization, Federation and Federate Lifecycle, and
Save/Restore guides with the pinned Join declarations, bounded registry paths,
and focused late-join test. The descriptions agree: Join changes federation
membership; the embedded synchronization path separately snapshots current
members and permits expansion only when the synchronization-set overload was
omitted; Save/Restore Join admission and its participant effects remain an
unresolved contract question. The direct test covers omitted versus explicit
non-empty sets, not explicit-empty registration or partial-achievement-before-
Join; those paths remain source-derived. The guides do not transfer the
synchronization-specific expansion rule to Save/Restore or claim 2010/process
parity.

The only gap was wayfinding from the synchronization guide's membership
orientation to the two exact cross-protocol sections. Added direct links to the
lifecycle participant-set map and Save/Restore Join boundary, plus a sentence
that the barrier-specific expansion is not a general membership or
Save/Restore rule. Existing diagrams already explain the distinctions; no chart
changed or was added, so no render was needed. Source/test evidence was
inspected, not run. No build, CI, 2010, or Requirements Lab work occurred.

Completed handoff: audit the 2025 synchronization registration/achievement callback
boundary alongside Callback and Service Ordering. Distinguish a synchronous
service result from submitted registration/announcement/report callbacks and
the final `federationSynchronized` outcome; compare source submission order with
HLA_IMMEDIATE/HLA_EVOKED application observation and process endpoint queueing.
Reuse bounded API/source/test evidence and the existing dispatch view; add a
chart only for a demonstrated reader gap. Keep 2010 separate and do not build or
rerun tests.

## 2025 synchronization callback, process-event, and service-report boundary audit — 2026-10-12

The HLA-level registration map and embedded `FederationSynchronized`
dispatcher view already distinguished protocol milestones, callback-model
observation, and selected service-report records. The uncovered reader question
was how those effects route through the 2025 process profile, where report
requests start after callback submission rather than before it.

Added a source-only process sequence. The private correlated service response
is not an HLA callback or public return value. In push mode, the service writes
recipient event frames before its response; the client buffers them during the
request, matches the response, releases the transaction lock, and submits the
events before returning to the ambassador adapter. For a registering
recipient, announcement submission therefore precedes registration-result
submission. A final pushed federation outcome is similarly drained before the
adapter starts its achievement invocation-report request. Process
registration/achievement report requests follow callback submission, unlike
the embedded report-before-callback-plan path. The selected reporting sink may
be a file or an `HLAreportServiceInvocation` interaction; its observer delivery
follows the separate service-reporting guide. Pull mode defers events to
`receive_interaction`, and its push/poll choice is independent of
HLA_IMMEDIATE/HLA_EVOKED dispatch.

The public `synchronization_point_catch2.cpp` callback-model cases are
explicitly embedded. Separate process-endpoint public cases cover
HLA_EVOKED/polling and HLA_IMMEDIATE/push, including registration
success/failure, an announcement to the explicit-set receiver, and a false
final result. The registering sender is excluded from the set, so those cases
do not assert same-recipient announcement-versus-registration order or
service-report order. The chart's finer ordering is therefore source-derived;
this is not a claim that process synchronization callbacks are untested, nor a
process/embedded parity claim. `scoop install mmdc` was attempted as authorized,
but Scoop returned “Couldn't find manifest for 'mmdc'”; no package was
installed and the new chart remains unrendered. No build, test, CI, 2010, or
Requirements Lab work occurred.

2026-10-12: Audited the 2025 process receive path's polling-family selector,
notification enqueue/push branches, and client pushed-event buffering. Polling
checks synchronization announcement, federation synchronized, save, restore,
and interaction-event queues in that order before later families; the next
poll may therefore select a later-queued announcement ahead of an older
interaction. Push bypasses that selector and drains a heterogeneous client FIFO
in stream-read order after the correlated response unlocks. This does not define
HLA callback ordering, and the callback model is independent. The two focused
public process synchronization cases exercise polling and push separately but
do not assert mixed-family precedence, so the comparison remains source-derived.
Added a bounded flowchart to the process-endpoint guide. Rendering is pending:
the authorized `scoop install mmdc` returned “Couldn't find manifest for
'mmdc'”; no package was installed or remote renderer used. No build, tests, CI,
2010, or Requirements Lab work occurred.

2026-10-12: Audited the 2025 callback-ordering and process-endpoint guides for
claims of one total order across callback families. Existing diagrams and
prose scope HLA callback eligibility to service rules, callback-model behavior
to dispatch policy, and FIFO claims to one dispatcher or process stream. The
callback-ordering table now directs normative claims to the exact service and
message/time-order rule, and states that callback model is not a sequence rule.
The pinned Connect/Evoke declarations and focused dispatcher FIFO tests support
this layer distinction, not a cross-service total order. No new diagram was
warranted. No tests/build/CI, 2010, or Requirements Lab work occurred.

Next audit: review the 2025 Object and Interaction Information Flow guide at the
object-discovery versus first-attribute-reflection boundary for a newly known
object. Compare pinned authority available in the checkout with bounded
embedded/process source and focused assertions; distinguish normative sequence
from implementation commit points and test gaps. Keep 2010 separate; do not
build or rerun tests.

2026-10-12: Completed that boundary audit. The 2025 process receive selector
chooses discovery before reflection for a newly registered object and says in
its source comment that this matches official callback ordering. The focused
public process case observes both callbacks but does not assert their relative
order. The local pinned API header labels §§6.9 and 6.11 without supplying the
normative text; the official IEEE download returned HTTP 403, so independent
clause verification remains open. Clarified the existing subscription flow as
scenario chronology and linked the evidence boundary from both guides. No
Mermaid changed; render and screenshot review were not applicable. No build,
tests, CI, 2010, or Requirements Lab work occurred.

Next audit: obtain an accessible authorized copy of IEEE 1516.1-2025 and
verify the exact §6.9/§6.11 ordering wording. Keep source interpretation and
normative text separate, and preserve 2010 and embedded/process boundaries.

## Current Mermaid inventory and renderer availability — 2026-10-12

Recounted current source fences across the design Markdown: 252 total in 34
diagram-bearing files, split into 14 separate 2010 diagrams, 223 2025
topic-guide diagrams, 13 cross-cutting-guide diagrams, and 2 index diagrams.
This preserves the distinction between edition-specific flows and the
cross-cutting reader model even though that guide's filename begins with
HLA-2025.

The local Mermaid CLI 12.0.0 is already present, so no Scoop installation was
needed. A fresh all-current render was attempted with the three behavior
groups kept separate. Puppeteer/Chrome reported an already-running profile;
retrying with a new profile still failed to launch Chrome with a Windows
account-launch error. No current SVGs were produced. The 247-chart render
snapshot and its chart-specific reviews remain historical evidence only; the
252-minus-247 difference is not a reliable pending-chart count because older
charts have also been edited. Do not claim a complete current render or visual
review. GitHub presentation remains a separate check. No C++ build, test,
CI, 2010/2025 behavior sharing, or Requirements Lab work occurred.

## Current Mermaid parser pass and sequence-label repairs — 2026-10-12

A parser-only pass over the current source counted the same 252 Mermaid fences.
It accepted 81 charts. The other 171 stopped with `DOMPurify.addHook is not a
function` in the local Node environment, so their syntax remains unverified;
this runtime error is not a parse success. The pass surfaced four sequence
charts whose semicolons were treated as statement separators. Their labels were
reworded in the 2025 TSO retraction, federation synchronization, exception
reporting, and callback/service-ordering guides. A direct pass over the edited
source then reported no other parser errors, but the 171 runtime failures still
prevent a whole-set syntax claim.

The Mermaid CLI 12.0.0 was already installed. The all-current Puppeteer/Chrome
render attempt still cannot launch a browser, so it produced no SVG and the
edited charts have not been visually inspected. Do not infer GitHub render
success. No build, tests, CI, 2010, or Requirements Lab work occurred.

## Federation MOM request/report outcome boundary — 2026-10-12

Revised the existing Federation MOM request/report sequence after comparing
the pinned 2025 MIM rows with the embedded `SendInteraction` handler and
focused public-test sources. It now separates synchronous `sendInteraction`
success/typed failure, the requested RTI `HLAreport...` state interaction, and
switch-selected per-invocation reporting. Only the selected embedded
`HLA_EVOKED` callback timing is shown. The source calls per-invocation
reporting before it queues the requested Federation report; the request test
does not enable or assert both families, so observer order is not established.
The federation-scoped and member-scoped FOM request classes remain separate;
no process or 2010 parity is claimed. The diagram count did not change.

No parser verdict or render was produced for this revision. `scoop install mmdc`
was authorized and attempted, but Scoop could not update because of a
permission error and then reported no configured manifest. The previous Chrome
launch failure remains; local visual and GitHub review are pending. No build,
tests, CI, or Requirements Lab work occurred.

Next audit: bounded HLA-first review of the 2025 object-discovery versus
first-attribute-reflection boundary. Keep the exact §§6.9/6.11 normative
question separate from source ordering and test observations, and preserve
2010 and embedded/process boundaries.

## 2025 API routing identity added to value-map diagram — 2026-10-12

Updated the existing “Encoded values enter services as handle-keyed octets”
diagram in the [object/interaction guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#encoded-values-enter-services-as-handle-keyed-octets).
The map now shows the outer `ObjectInstanceHandle` for attribute updates and
`InteractionClassHandle` for ordinary interactions, and distinguishes the
corresponding reflection/interaction callback identities. This reuses the
existing diagram; the total diagram count is unchanged.

Compared the tall top-to-bottom layout with a left-to-right alternative and
adopted the latter: the common encoding path now leads into distinct object
attribute and interaction parameter lanes, with their different service and
callback identities preserved. Mermaid Live Editor 12.1.0 accepted and
rendered the exact guide source; the full flow and labels were visible without
clipping in the focused preview. This is a Mermaid Live render, not a GitHub
renderer verdict. No mmdc install was needed for this focused review; GitHub
presentation remains unverified. No C++ build, tests, CI, 2010, or Requirements
Lab work occurred.

## HLA-first discovery and first-reflection survey — 2026-10-12

Reviewed the existing object/interaction guide together with the more detailed
subscription-state guide. They already separate receiver discovery/known-object
state, the embedded late-subscription initial-value reflection after discovery
enters user code, the process selector's source-commented discovery priority,
and the process late-subscription route's MOM-only initial-value event. The
focused ordinary process update test observes both discovery and a later
explicit reflection but records no callback sequence; it does not verify the
selector ordering or an ordinary late-subscription snapshot. No additional
diagram would clarify this without repeating the existing source/profile
comparison.

The normative questions remain open: the pinned 2025 API package contains
headers rather than clause prose, and IEEE's product page lists purchase or
subscription access for the standard
([IEEE 1516.1-2025](https://standards.ieee.org/ieee/1516.1/6688/)). Keep the
process source comment distinct from verified §§6.9/6.11 text and keep the
ordinary process §5.8 late-subscription result unclaimed. No C++ build, tests,
CI, 2010, or Requirements Lab work occurred.

Next audit: examine the already-identified 2025 object-class declaration
lattice and ancestry effects on publication, subscription, and receiver
projection. Reuse the existing class-ancestry map; add a diagram only if a
portable reader gap remains after bounded source/test review. Do not infer
2010 behavior or invent normative rules where the licensed text is unavailable.

## 2025 object-class declaration and ancestry audit — 2026-10-12

Reviewed the pinned 2025 API signatures, per-class declaration storage,
registration's publication lookup, class-relative attribute validation, the
discovery resolver, and the focused child-instance/base-subscription case.
Added a bounded explanation to the existing object/interaction guide: inherited
attribute availability, the class named by a declaration, and the receiver's
selected known class are separate coordinates. The test observes publication
of base and child attributes at the child class, then a base-class discovery
and base-only reflection for a receiver subscribed at the base; it does not
establish all normative ancestry or passive/regional/process combinations.

No additional diagram was warranted: the existing ancestry and active/passive
maps already explain the selected paths, while the licensed 2025 clause prose
needed to turn the remaining declaration-lattice questions into portable HLA
rules is not present in the checkout. No 2010 behavior was inferred. No
Mermaid renderer was installed or used, and no chart was sent to an online
renderer. No C++ build, tests, CI, or Requirements Lab work occurred.

Next audit: review the existing 2025 TSO message-retraction guide as an
HLA-first teaching tool. Separate portable per-message service rules from
per-recipient event admission, callback-time retraction, and selected
embedded/process boundaries; reuse existing evidence and add no duplicate
diagram unless a concrete reader gap remains. Keep 2010 separate and do not
build, rerun tests, start CI, or edit the Requirements Lab.

## 2025 TSO retraction HLA-first boundary audit — 2026-10-12

Reviewed the portable per-designator/per-recipient overview against the
embedded recipient-state map, producer-time gate, mixed-recipient example,
timestamped-delete special case, and embedded/process comparison. The guide
already leads with three distinct HLA questions—producer permission, each
recipient's original-delivery outcome, and receiver time-request results—and
keeps Umbra queues, ledger labels, callback admission, and process transport in
separate sections. One wording seam called callback entry an HLA outcome. It
now states the portable delivered/not-yet-delivered distinction first and
labels callback entry as the embedded profile's ledger threshold. The diagrams
already cover these concepts; no diagram was added or rendered.

The pinned API provides section labels, but not the full licensed §8.22–8.23
prose. Exact normative conditions/timing remain explicitly open; source and
focused-test results remain scenario evidence. No test was rerun and no 2010
behavior or embedded/process parity was inferred. No C++ build, CI, or
Requirements Lab work occurred.

Next audit: review the 2025 DDM and regions guide from the HLA reader's
perspective, focusing on the region-template lifecycle: distinguish FOM
dimension/class availability, region creation and committed extents,
per-object update-region associations, subscription regions, and deletion or
invalidated use. Reuse existing diagrams and focused evidence; add nothing
unless a concrete reader gap remains. Keep 2010 separate and do not build,
rerun tests, start CI, or edit the Requirements Lab.

## 2025 DDM region-template lifecycle HLA-first audit — 2026-10-12

The current DDM guide already makes the lifecycle boundaries explicit. Its
FOM-to-runtime bridge separates the execution's dimension catalogue,
class-available dimensions, and a region's fixed dimension set. The lifecycle
section separates pending range edits from the committed routing snapshot and
states that commit validates a complete specification. Object routing then
distinguishes per-attribute source associations from receiver subscriptions;
interaction routing remains a separate service path.

The delete gate is also already represented: regional subscriptions and
update associations keep a region in use, successful deletion invalidates
later queries, and deferred object removal can retain an association until the
receiver consumes its removal callback. The focused 2025 lifecycle test source
shows both a live interaction subscription and object-update association
blocking deletion, plus the deferred-removal case. This was a source/test-code
inspection, not a test run. No additional flow was warranted, no guide diagram
changed, and no renderer, build, tests, CI, 2010, or Requirements Lab work was
needed. Existing GitHub/current-set visual-review gaps remain open.

Next audit: review the 2025 Order and Transportation Controls guide from the
HLA reader's perspective. Keep message order (receive order versus timestamp
order) distinct from transportation type; trace class defaults, per-instance
overrides, transportation-change callback gates, and receiver-visible
behavior. Reuse existing diagrams and bounded API/source/test evidence; add
nothing unless a concrete reader gap remains. Keep 2010 separate and do not
build, rerun tests, start CI, or edit the Requirements Lab.

## 2025 order and transportation HLA-first audit — 2026-10-12

Reviewed the current guide against the pinned 2025 API headers/MIM, the
embedded registry's bounded order/default update path, and focused order and
transportation test bodies. The diagrams already distinguish the timestamped
service overload from the effective order designation and receiver-reported
order; RO/TSO eligibility and callback observation are separate from the
transportation category. The controls table and lifecycle chart correctly
separate prospective, class-scoped defaults and instance-local state from
callback-confirmed transportation overrides. Query results are callback
reports, and the interaction query names the publisher whose effective value
is requested.

Focused evidence includes embedded order/default/instance examples, custom
attribute and interaction transportation confirmation, receiver metadata,
and a process interaction change/query scenario under both callback models.
The process case does not prove process attribute override behavior or general
embedded/process parity. Tests were inspected, not run. No diagram or guide
change was warranted. Exact normative order mapping still needs the licensed
IEEE 1516.1-2025 clause text; selected tests do not establish the full service
cross-product or physical network QoS. No render, build, tests, CI, 2010, or
Requirements Lab work.

Next audit: review the 2025 Declaration and Attribute Relevance Advisories
guide. Keep publisher-facing class declaration advisories distinct from
owner-facing per-object attribute relevance and receiver-local scope; examine
the subscription, hierarchy, switch, and update-rate edges against existing
flows and bounded evidence. Reuse diagrams unless a specific HLA reader gap
remains. Keep 2010 separate and do not build, rerun tests, start CI, or edit
the Requirements Lab.

## 2025 declaration and attribute relevance HLA-first audit — 2026-10-12

Reviewed the existing guide against its callback/API map, bounded declaration
and per-instance planners, the separate Attribute Scope guide, and focused
embedded/process test bodies. The guide already distinguishes publisher/class
declaration edges, owner/object/attribute relevance, receiver-local scope,
discovery, and later value reflection. It also keeps active/passive and
regional matching separate from delivery overlap, and calls out class
hierarchy and update-rate reissue behavior without treating advisories as
delivery gates.

One scope gap was worth clarifying: the class-level section documents its
switch-off/no-backlog result, while the per-instance switch evidence is
narrower. Added prose and a direct test pointer stating that the selected
embedded case suppresses a rate-bearing On and an Off during disabled-switch
subscription changes under its tested callback models, then re-enables before
a fresh subscription. It does not resolve whether an already-relevant state
is replayed after enabling; do not transfer the class-switch result to the
owner-side or receiver-local switches. No diagram changed. Process On/Off and
rate-reissue tests remain selected process evidence, not parity. Test bodies
were inspected, not run; no render, build, tests, CI, 2010, or Requirements
Lab work.

Next audit: review the separate 2025 Attribute Scope Advisory guide. Trace
receiver-local per-object-attribute scope across subscription and region
changes, the self-final-unsubscribe exception, callback-time stale-state
rechecks, and the HLA_EVOKED/HLA_IMMEDIATE versus process evidence boundaries.
Keep scope distinct from owner relevance and class declaration advisories;
reuse current flows unless a concrete reader gap remains. Keep 2010 separate
and do not build, rerun tests, start CI, or edit the Requirements Lab.

## 2025 Attribute Scope Advisory callback/evidence boundary audit — 2026-10-12

Compared the existing guide with the pinned callback API, embedded scope
planner/callback route, process enqueue/receive rechecks, and focused test
bodies. The receiver-local predicate, per-object/attribute grouping,
self-final-unsubscribe exception, and separate owner/producer advisory lanes
are already clear.

One test-description seam needed precision. In the embedded stale-In scenario,
HLA_EVOKED queues In, then the receiver commits a disjoint range before
dispatch; the stale In is dropped and the current Out remains. Under
HLA_IMMEDIATE, In is entered before the later region mutation and is therefore
not stale; the subsequent mutation reports Out. The process test exercises
public In/Out and self-unsubscribe paths under both callback models, but does
not stage a stale-queue race. Updated the guide and backlog to keep those
evidence scopes explicit.

Also bounded the scope-switch evidence: the embedded switch-off segment
coincides with the independent self-final-unsubscribe exception, so it does
not isolate suppression of an otherwise-reportable transition. The process
test keeps the switch enabled. Source confirms embedded and process
revalidation checks, but the cited tests do not independently exercise
switch-off suppression or a process stale-work race. The decision flow remains
unchanged; no Mermaid render was needed. Tests were inspected, not run. No
build, CI, 2010, or Requirements Lab work.

## 2025 Auto Provide HLA-first audit — 2026-10-12

The guide and neighboring request guide already separate discovery-triggered
RTI solicitation from explicit federate requests, initial reflection from
provider solicitation, and the provider callback from its later value-bearing
update. The embedded planner groups current in-scope ownership by provider;
callback-entry switch/scope/ownership checks remain distinct from receiver DDM
advisories and callback observation. One implementation boundary was missing:
the planner has a single post-discovery callsite, so later scope-in for an
already-known object does not initiate a second automatic solicitation in the
inspected embedded path. Added a source-derived guide note and evidence-table
row. No focused test isolates that no-retry edge; it is not a normative or
process-profile claim. No diagram was changed or rendered. Source/test evidence
was inspected, not run; no build, CI, 2010, or Requirements Lab work.

## 2025 timestamped object deletion/removal audit — 2026-10-12

The object-information guide already described ordinary deletion, local
forgetting, resignation cleanup, and receiver-known state, while the time and
retraction guides covered TSO delivery and deletion's special undo. The missing
reader connection was how one accepted timestamped delete produces distinct
recipient paths and when each removal is committed. Added one sequence to the
2025 object guide and linked it to the TSO lifecycle/retraction guides. The
selected embedded FQR/TARA/NMRA case gives three distinct same-time callback
orders; these remain test observations, not a universal order claim. The
focused embedded retraction scenarios cover cancellation before first delivery
and restoration after one receiver has observed removal. Pre-delivery
preservation of intervening object mutations is visible in source but is not
isolated by that focused test. The configured-process test checks the removal
payload and timing profile, not retraction after delivery or restoration; no
process parity is inferred. No 2010 behavior is inferred.

The same bounded enqueue inspection also found an empty-recipient edge: if no
other federate knows the instance, embedded deletion is accepted without a
removal callback, producer knowledge is cleared, and the object/name plus
retraction record remain until the time bound. The guide labels this source-only;
no focused test isolates it.

The new Mermaid sequence received a manual source review but no fresh parser or
visual render. The authorized Scoop install attempt failed while updating Scoop
with permission errors and then reported no `mmdc` manifest. No build, tests,
CI, or Requirements Lab work.

## 2025 save/restore responsibility-map audit — 2026-10-12

The existing guide already leads with the durable HLA distinction between a
federate's application checkpoint and the RTI's federation checkpoint. Its
save and restore sequences, separate state diagrams, vocabulary table, and
early-rejection versus begun-failure comparison cover request admission,
participant work/results, RTI commit/rehydration, and federation-level outcome.
The member-set caveat and in-flight-work index keep membership, object state,
callback queues, durable images, and process-local routes distinct. Existing
source and focused-test links identify where claims are implementation- or
scenario-bounded. No real learner gap justified another combined chart; doing
so would duplicate the existing state machines and risk conflating
application-owned and RTI-owned state. No guide changes or new source/test
inspection were required beyond the bounded content review. No charts were
rendered, and no build, tests, CI, 2010, or Requirements Lab work occurred.

## 2025 ordinary and directed interaction delivery survey — 2026-10-12

The ordinary interaction section and the separate directed-interaction guide
already teach the important portable divide: class-scoped ordinary send versus
target-object directed send, with distinct publication/subscription/selector
rules and no shared object-knowledge gate. Both route into their own parameter
and callback boundaries. The ordinary flow separately covers regional overlap
and TSO time; the directed guide keeps its target/selector distinct from DDM
regions and does not assert a region-overlap rule. The implementation notes correctly label
embedded callback-time re-evaluation versus process event projection, and
ordinary accepted-route behavior versus directed publication rechecks. Focused
test maps identify embedded inherited receipt, directed selector/target cases,
and narrower process endpoints without claiming full matrix coverage or
normative/profile parity. No learner gap justified combining the diagrams;
that could falsely imply a common routing rule. No guide edits or Mermaid
render; no build, tests, CI, 2010, or Requirements Lab work.

## 2010 reference-RTI service evidence survey — 2026-10-12

The separate 2010 reference guide already covers the source-supported service
slice: lifecycle, declarations, object and interaction flow, synchronization,
and the distinction between logical-time values and unavailable time services.
The bounded inventory contains seven 2010-named smoke executables, covering
binding, value/composite encoding, headers, and time-value/marshalling cases;
the binding-shell path does not test synchronization. Six external-FOM Catch2
cases are model validation/composition, not reference-RTI service tests. The
runtime does contain a synchronization barrier, but its participant/error and
retry behavior is already called out as source-only in the guide. Another
standalone 2010 guide would duplicate the current service map without adding
focused test or normative evidence, so broader expansion remains held. No
2010/2025 equivalence is inferred. No files or tests changed; no build, CI, or
Requirements Lab work.

## 2025 callback and service-ordering boundary audit — 2026-10-12

The existing callback guide already separates the HLA callback model from
recipient eligibility, callback-enable state, protocol effects, and
application observation. Its HLA ordering caveat directs readers to exact
service and message/time rules; local dispatcher FIFO, inline stacks, and
process event/report ordering are explicitly implementation-scoped. The
embedded and process traces preserve their different source order, with test
limits stated. One wayfinding gap remained: the guide lacked direct locators
for the callback-control clauses it told readers to consult. Added a small
map for Connect §4.2 and callback-control §§10.57–10.60; it explicitly
disclaims global ordering and replacement of normative text. No new chart or
render. No build, tests, CI, 2010, or Requirements Lab work.

## 2025 first-reflection ordering evidence audit — 2026-10-12

The embedded callback path calls discovery before planning the initial current-
value reflection. Its focused regional case asserts discovery and reflection
values in separate report collections, not their relative order. The process
selector prioritizes queued discovery before attribute reflections; however,
the public process test reflects a distinct publisher update and uses separate
discovery/reflection flags. Ordinary process discovery has no ordinary
initial-value event in the inspected enqueue path; its special initial event
is limited to RTI-owned MOM objects. The API labels §§6.9 and 6.11, but the
normative clause prose remains unavailable. Existing object/subscription flows
already show these boundaries; no additional chart or render was warranted.
No test execution, build, CI, 2010, or Requirements Lab work.

## 2025 class ancestry: path-specific behavior and evidence limits — 2026-10-12

The existing cross-path map already separates object-class and
interaction-class ancestry. The focused embedded discovery case registers a
child-class object and subscribes at its base class; the discovery reports the
base as known and the initial value projection contains only the base
attribute. An internal registry test separately demonstrates ordinary
interaction promotion to two ancestors and omission of child-only parameters.
These are embedded and registry-unit observations, not normative evidence.

Directed routing walks the target object's class ancestry for declaration and
selector lookup while retaining the sent interaction class as the parameter
namespace. The focused embedded known-target and process multi-recipient cases
use exact-class declarations, leaving ancestor behavior untested. The object
guide now spells out these evidence limits alongside its existing map; no
additional diagram or renderer pass was warranted. Licensed normative clauses
remain open; no 2010 inference, tests, build, CI, or Requirements Lab work.

Next audit: review the HLA 2025 attribute-ownership negotiation and transfer
state machine, separating HLA request/offer/acceptance outcomes from Umbra
callback, queue, and commit points. Use bounded pinned API/source/test evidence;
add or revise a diagram only for a concrete reader gap. Keep 2010 separate and
do not build, rerun tests, start CI, or edit the Requirements Lab.

## 2025 negotiated-divestiture cancellation after confirmation — 2026-10-12

The existing success sequence omitted the stale-request edge after
`requestDivestitureConfirmation` has reached the owner. In the focused embedded
2025 test, the requester cancels before the owner confirms; Confirm Divestiture
throws the API's `NoAcquisitionPending` exception, the ownership queries remain
owner=true/requester=false, the owner cancels the negotiated divestiture, and
the requester later observes cancellation confirmation. The registry's
no-active-acquirer branch also retains ownership. This is a selected embedded
service/test sequence, not a process or normative ordering claim.

Added a distinct Mermaid sequence after the existing process-boundary success
case to keep those profiles separate. Current fence census is 254 diagrams
(147 flowcharts, 84 sequences, 23 state diagrams); the new sequence has no
fresh parser, render, visual, or GitHub result. The explicitly authorized
Scoop install found no `mmdc` manifest and installed no package. No C++ code or
test was changed or run; no build, CI, 2010, or Requirements Lab work.

## 2025 ownership transfer and update-region association — 2026-10-12

Added a sequence to the ownership guide and a reciprocal pointer in the DDM
guide. The sequence compares three separate focused embedded test scenarios:
clearing the former owner's association and requiring explicit association
after reacquisition, promoting a prospective owner's deferred association, and
not promoting it after unassociation. The pinned API exposes ownership
services §§7.9/7.13 and region services §§9.6/9.7, but the full normative
clause text is unavailable; treat the detailed transition as selected
embedded source/test evidence, not an all-RTI rule or process parity claim.
The current source census is 255 diagrams (147 flowcharts, 85 sequences, 23
state diagrams). The new sequence has no parser, renderer, screenshot, or
GitHub verdict. Scoop previously found no `mmdc` manifest and installed
nothing. No code/test execution, build, CI, 2010, or Requirements Lab work.

## 2025 accepted TSO update across ownership transfer — 2026-10-12

The existing ownership/TSO trace already depicts a queued timestamped update
surviving a transfer, with original `producingFederate` identity. A bounded
test/source review found one useful clarification, not another diagram: the
current test source asserts B is owner and A is not before B advances, while a
later reflection assertion retains A as producer. Added this direct ownership
contrast to the existing test-evidence paragraph. The focused case is embedded
`HLA_EVOKED`; process parity and a universal delivery rule are not claimed.
The test and related TSO source contain independent worktree edits; they were
inspected but not modified or executed, so no runtime result for the current
test edit is claimed. No new Mermaid block, parser, render, screenshot, or
GitHub status was produced.

## 2025 Retract authority after attribute ownership transfer — 2026-10-12

Compared the pinned 2025 Retract API, bounded embedded ambassador and registry
paths, the focused ownership-transfer test source, and the existing TSO flow.
The embedded path checks joined/time-regulating status, current membership,
original-producer identity, timestamp eligibility, and retraction-ledger
state; it does not check current attribute ownership. The transfer test does
not call Retract after transfer, so the source observation is not a verified
service result or a normative HLA rule. The pinned API package lacks the full
§8.22 clause text, and no process or 2010 behavior is inferred. Updated the
TSO guide's implementation-limit note; no diagram changed, so Mermaid
rendering and Scoop `mmdc` installation were unnecessary. This docs pass made
no C++ edits and ran no build, tests, or CI; it did not change the 2010 stream
or Requirements Lab.

Next audit: verify the normative 2025 Retract authority and time-bound rules in
§§8.22–8.22.3 against the ownership boundary. If the licensed clause text is
unavailable, leave the portable outcome open and continue with another bounded
HLA-first reader gap; preserve the separate 2010/2025 and embedded/process
evidence boundaries.

## 2025 Request Attribute Value Update: requester and receiver roles — 2026-10-12

The HLA-facing request sequence already models requester, provider, and
receiver as roles, but its evidence map lacked a direct example where the
requester later receives the provider's update. Added a link to the focused
embedded `HLA_EVOKED` test's subscription, request, and reflection assertions.
This establishes role overlap only when the requester has its own subscription;
it does not answer direct response entitlement for an otherwise ineligible
requester. The process class-request test discovers its targets but does not
assert a later reflection. No new Mermaid diagram or normative claim was
warranted. IEEE's public listing exposes purchase/subscription access, not the
full §6.21 prose. No C++ edits, build, tests, CI, 2010, or Requirements Lab
work in this documentation pass.

Next audit: resolve §6.21 from an authorized standard copy if available;
otherwise leave the entitlement question open and continue a bounded
HLA-first review of another area.

## 2025 DDM pending-range query and committed-matcher distinction — 2026-10-12

Refined the existing region-template state diagram without adding a Mermaid
block. Its pending-edit self-transition now makes the two embedded views
visible: the caller's range query sees the pending value, while ordinary
regional matching still uses the previous committed specification. The
focused lifecycle test observes pending query values before and after commit;
the separate commit-triggered discovery case observes an existing object
become discoverable after the receiver's committed range enters overlap. No
test covers a value send during the uncommitted interval; that matcher detail
is explicitly source-derived. No portable standard timing rule or 2010/process
parity is claimed. The changed diagram has no fresh parser, render, screenshot,
or GitHub review. No build, tests, CI, C++ changes, or Requirements Lab edits.

Next audit remains the unresolved 2025 §6.21 requester-entitlement question
pending an authorized full-text copy; if unavailable, continue another bounded
HLA-first reader gap. Preserve the edition and profile boundaries.

## 2025 active relevance and passive receiver boundary — 2026-10-12

A bounded review of the pinned §5.8 API, the subscription-state guide, its
focused embedded test, and the shared registry paths found that the current
guide already explains the distinction a proposed extra chart would cover:
active declarations establish the source-observed discovery/update relevance
path, while a receiver's own matching active or passive declaration is a
separate local eligibility check. The two paths retain class and region
constraints. The guide keeps this multi-member rule explicitly
source-derived—not a normative HLA statement or an embedded/process parity
claim. While checking these citations, corrected the guide's stale discovery
resolver line link from 855 to the current source comment at 869.

The [pinned API signature](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L535)
provides the active flag and exception surface, not the licensed §5.8 prose.
The [focused test](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L120)
uses one publisher and one subscriber: it covers passive-only suppression and
that subscriber's transition to active, but not a different active federate
establishing relevance for a passive receiver. The [discovery resolver](../../cpp/src/internal/federation/federation_registry.cpp#L869)
and [update recipient planner](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L76)
show the separate relevance and receiver-selection checks in current 2025
source. The existing reader-interest and passive-recipient diagrams already
make this distinction and its evidence limit explicit; no duplicate chart or
normative claim was warranted. Visual review of the passive-recipient and
late-subscription diagrams remains pending. No 2010 behavior was inferred;
no C++ changes, build, tests, CI, or Requirements Lab edits occurred.

Next handoff: review those two pending diagrams in a local Markdown preview.
Use Scoop to install `mmdc` only if a usable manifest is available; keep the
render check separate from substantive HLA claims. Afterward, continue a
bounded HLA-first gap survey, leaving §6.21 open unless authorized clause text
becomes available.

## 2025 MOM reference identity versus receiver object knowledge — 2026-10-12

Clarified the Federation MOM map's evidence boundary: the MIM's
`HLAfederateReference` encodes a `FederateHandle` related to the member object's
`HLAfederateHandle` attribute; the public object-instance lookups use a
receiver-local `ObjectInstanceHandle` and may report `ObjectInstanceNotKnown`.
The membership test decodes member references but does not query the
corresponding member object. A separate joined-federate case discovers the MOM
object before using support lookups. Added a concise explanation and reciprocal
guide links; no diagram changed. No callback-order or standard-wide knowledge
rule is inferred. The existing map remains unrendered/unparsed, with no new
render verdict. No build, tests, CI, 2010, or Requirements Lab work.

## 2025 Federation MOM execution/member observation map — 2026-10-12

Added one HLA-first scope map to connect the 2025 MIM's execution-level
`HLAfederation` object and conditional `HLAfederatesInFederation` reference-list
value with each joined member's `HLAfederate` object. The diagram separates
observer receipt of the list reflection from discovery/removal of a member
object and does not impose callback ordering across those paths. Bounded
embedded tests cover the list reflection and member-object removal separately;
a process test covers ordering within the member-object removal/rejoin path
only. No cross-path order or embedded/process parity is claimed. The new
flowchart has no fresh syntax, render, screenshot, or GitHub verdict because
`mmdc` is unavailable. No build, tests, CI, 2010, or Requirements Lab work.

## 2025 Federation MOM late-observer membership value — 2026-10-12

Made the late-joining observer case explicit in the Federation MOM guide. The
2025 API requires execution membership for `subscribeObjectClassAttributes`;
the MIM says `HLAfederatesInFederation` is Conditional on Join/Resign and
requires current-value responses to Request Attribute Value Update even for
Conditional attributes. The MIM/API material cited here does not establish
that a prior Join-triggered reflection is replayed after a late observer
subscribes. The guide recommends the affirmative path—subscribe, discover the
federation object, request its current member list—without claiming the RTI
cannot send additional reflections.

The focused test joins/subscribes the observer first, explicitly requests the
initial list, then observes another member's Join and Resign. It does not
exercise the late-observer case. No diagram changed; the existing sequence
already communicates the request path. No new parser/render work, build, test,
CI, 2010, or Requirements Lab work.

## 2025 Federation MOM effective FOM and module-data views — 2026-10-12

The cross-guide audit found a concrete scope/wayfinding gap: the FOM guide
explains the composed model and FDD, while the Federation MOM guide separately
explains its attributes and module-data reports without directly linking the
two. Added a comparison that keeps four views separate: effective model
lookups; the federation object's Conditional module list and `HLAcurrentFDD`;
the member object's Static Join-supplied list; and the indexed module-content
request/report interaction. The MIM class semantics allow a current-value
request for Conditional Federation MOM attributes; the focused embedded
current-FDD test shows the post-Join changed reflection and a subsequent
request returning that same value. The member snapshot test uses a private
inspection seam; a separate public member-object lifecycle case verifies its
reflected Join-supplied list. Neither covers the federation-wide list update.
No new chart was warranted because the existing composition,
conditional-reflection, and report diagrams already explain each route. No
test/build/CI, 2010, or
Requirements Lab work; the 256-diagram inventory and existing pending render
review are unchanged.

Next handoff: determine whether a focused public observer test asserts the
federation-scoped `HLAFOMmoduleDesignatorList` update after Create/Join; keep
that distinct from the per-member Static list and the tested `HLAcurrentFDD`
path. Do not rerun tests or build for this documentation survey.

## 2025 Federation MOM federation-list update evidence — 2026-10-12

The MIM's federation-level list is Conditional on FOM additions through
Create/Join, while the same-named member list is Static. The selected embedded
projection derives the federation value from its committed definition; the
additional-module Join path queues the list and `HLAcurrentFDD` updates. The
bounded test search found no public assertion for the federation-wide list
change: the public list assertion is on `HLAfederate`, and the separate
federation test asserts `HLAcurrentFDD`. No diagram changed, so there is no new
syntax, render, or screenshot verdict; the MOM map's existing pending status
remains. No build, test rerun, CI, 2010, or Requirements Lab work.

## 2025 Federation MOM duplicate-designator evidence — 2026-10-12

The MIM requires the first designator for repeated identical modules in both
MOM list scopes. The shared embedded encoder de-duplicates canonical source
paths, and a private registry test confirms one first-designator entry for the
member snapshot when two records share that path. The public member-object test
uses one module; the federation-wide list has no direct assertion in the
focused test sources. Equal content at distinct canonical paths remains an
unresolved interpretation, not a claimed defect. No diagram changed, so the
existing MOM-map parser/render/screenshot status is unchanged.

## 2025 Federation MOM module-identity source boundary — 2026-10-12

The pinned MIM gives the first-designator outcome for identical FOM modules,
but the checked-in IEEE 1516.2-2025 resources do not include the full OMT text
needed to check whether the standard gives a more specific identity test. The
IEEE SA product page separately lists purchase or subscription access to the
specification and an additional-resource download
([1516.2-2025 publication and access](https://standards.ieee.org/ieee/1516.2/6689/)).
Accordingly, byte-identical modules at distinct canonical paths remain
unresolved; the embedded encoder's `sourcePath` comparison is not promoted to
a normative rule. No diagram or source changed; diagram inventory and the
MOM-map render status remain unchanged. No build, test rerun, CI, 2010, or
Requirements Lab work.

## 2025 Object deletion and ownership-cleanup wayfinding — 2026-10-12

The HLA-first cross-cutting map already separates a federate's local forget
from federation-wide object deletion. The object-information guide explains
the delete request, receiver removal callback, and local-delete service; the
ownership guide's accepted-delete flow covers embedded pending-work cleanup,
callback gates, and the limits of the individual focused scenarios. A
combined diagram would duplicate those service and ownership flows and risk
presenting source-only ledger invalidation as a portable HLA rule. Added a
direct cross-guide link at the accepted receive-order deletion boundary. No
chart changed; inventory and existing render status are unchanged. No build,
test rerun, CI, 2010, or Requirements Lab work.

## 2025 ResignAction cross-guide boundary — 2026-10-12

The lifecycle guide already maps the six API action names to a separate
embedded effect table/sequence, and the object and ownership guides carry the
receiver-removal and per-attribute state machines. Its focused embedded
scenarios support selected guards and effects, not the full action/record
cross-product or normative callback order. The process case is limited to a
separate `NO_ACTION` MOM-lifecycle observation. Added an ownership-to-lifecycle
link rather than combining these distinct scopes into another diagram. No
diagram or test changed; inventory stays 256 and existing render status is
unchanged. No build, test rerun, CI, 2010, or Requirements Lab work.

## 2025 Connection-loss resignation: directive versus forced cleanup — 2026-10-12

Compared the automatic directive, forced-loss cleanup, and last-member rule
against the 2025 registry and selected embedded and process scenario sources.
The guide's sequence already had the correct entry path, but its prose did not
make clear that the forced path divests attributes still owned under
`NO_ACTION` and cancels pending acquisition state independently of the
configured action. Added an implementation-only layer table and direct links
from the lifecycle guide. The embedded `NO_ACTION` scenario observes the
retained object and an ownership-assumption callback; the pending-acquisition
scenario configures cancellation itself, so it is not proof of override
independence. Process service and TCK scenario sources verify a distinct
endpoint path and selected configured actions, but do not establish the
forced `NO_ACTION` overlay or process/embedded parity. No callback ordering or
normative action matrix is claimed. No diagram changed; inventory remains 256
and prior parser/render/screenshot status is unchanged. Test sources were
inspected, not run; no build, test rerun, CI, 2010, or Requirements Lab work.

## 2025 Connection loss during active Save/Restore — 2026-10-12

The lifecycle guide now records the shared registry behavior for an active
Save/Restore when a member departs, plus profile-specific notification
projection. Embedded membership-loss code submits the returned Save/Restore
vectors. Both process connection-loss and voluntary-resign handlers use the
same result dispatcher, which currently does not forward those vectors; this
is a source-derived gap and is not runtime-confirmed. Voluntary embedded
reason-code tests do not test forced transport loss, and no focused
connection-loss-during-active-operation case was found in the targeted test
families. The Save/Restore guide links to the lifecycle treatment. No new
diagram was justified because a single chart would imply callback parity the
source does not show. Current inventory is 256 Mermaid diagrams across 34
diagram-bearing docs/design files, counting both backtick and tilde fences.
No Mermaid render, build, test rerun, CI, 2010, or Requirements Lab work.

## 2025 Connection loss during a pending synchronization point — 2026-10-12

Source review confirmed that forced `connectionLost` calls the shared 2025
member-removal transition. That transition removes the member from the barrier
sets before testing completion, excludes it from the failed set, and only
plans completion callbacks for remaining participants with usable routes. The
embedded membership-loss adapter submits the planned results; the process
connection-loss dispatcher enqueues the synchronization notifications.

The focused test scan found the existing two-member embedded HLA_EVOKED
voluntary-resignation case, but no forced-loss-during-active-synchronization
case in the targeted loss Catch2 or connection-lifecycle TCK paths. The guide
labels the new forced-loss explanation source-observed, not normative or
portable. Existing state diagrams already cover barrier closure, so none was
added or re-rendered. No build, test rerun, CI, 2010, or Requirements Lab work.

## 2025 process `DELETE_OBJECTS` resignation flow — pending review — 2026-10-12

Added one process-profile flowchart to the object/interaction guide for
producer Resign planning, per-survivor removal, pull versus push commit gates,
and the recipient's own Resign while a pull event remains queued. It is based
on bounded 2025 source; the portable C++ TCK scenario is a separate expected-
behavior example, not proof of Umbra process execution. The chart has no fresh
Mermaid syntax/parser verdict, local render, visual inspection, or GitHub
rendering. `mmdc` is unavailable in the current shell. No build, test rerun,
CI, 2010, or Requirements Lab work.

## 2025 process `DELETE_OBJECTS_THEN_DIVEST` order flow — pending review — 2026-10-12

Added a second source-derived flowchart to the object/interaction guide. It
separates delete-accepted instances from retained instances whose owned
attributes are divested, then contrasts the observed embedded callback order
with process default-pull queue priority and optional-push frame submission
order. No focused process voluntary-action test asserts callback order. The
chart has no fresh Mermaid syntax/parser verdict, local render, visual
inspection, or GitHub rendering; `mmdc` is unavailable in the current shell.
No build, test rerun, CI, 2010, or Requirements Lab work.

## 2025 process receiver Resign with pending cross-object callbacks — pending review — 2026-10-12

Added a source-derived flowchart distinguishing server-held pull queues from
push frames that crossed to client buffering/callback dispatch before receiver
Resign. Registry ledgers and joined-session queues are cleared before the
successful process response; the process client dispatches buffered frames
after the response, and the process Resign branch does not reset the callback
dispatcher. The closest focused process assumption test consumes the callback
before candidate Resign; no test establishes the pending-frame race. The chart
has no fresh Mermaid syntax verdict, local render, visual inspection, or
GitHub rendering. No build, test rerun, CI, 2010, or Requirements Lab work.

## 2025 process Request Retraction after receiver Resign — pending review — 2026-10-12

Added one sequence diagram to the 2025 TSO retraction guide for a producer
Retract after a receiver has resigned but remains connected. It distinguishes
the process service's cached transport-session route from federation
membership and the client callback bridge's per-message started-state gate.
Existing process attribute-retraction tests cover active-member suppression
before callback observation, not this post-Resign route. The chart has no
fresh Mermaid syntax verdict, render, visual inspection, or GitHub rendering.
Scoop search found no configured mmdc package. No build, test rerun, CI, 2010,
or Requirements Lab work.

## 2025 process interaction retraction-route comparison — pending review — 2026-10-12

Revised the existing receiver-Resign sequence to name its coordinator-
dispatched TSO branch and distinguish it from recipients that bypass the time
coordinator at the original service call. Added a source comparison showing
that attribute send-time pull queues register a session route, while the
ordinary interaction send-time pull queue does not in the inspected handler;
coordinator-dispatched TSO paths add routes after sending for both families.
The client also removes pending interaction events on Request Retraction but
leaves pending attribute events for callback-bridge gating. Process interaction
tests inspected cover coordinator-dispatched callback delivery and too-late
Retract, not the send-time pull or receiver-Resign races. The sequence has no
fresh Mermaid syntax verdict, render, visual inspection, or GitHub rendering;
Scoop searches for `mmdc` and `mermaid-cli` returned no matching package. No
build, test rerun, CI, 2010, or Requirements Lab work.

## Relaxed DDM: federation switch scope and boundary evidence — 2026-10-12

The pinned 2025 API exposes `getAllowRelaxedDDMSwitch()` but no per-federate
setter. In the current embedded path, FOM composition reads `allowRelaxedDDM`
and federation state retains the composed value. The focused regional
interaction and object-attribute cases use separate enabled/disabled FOM
configurations and assert that both participants observe the selected value;
the regional Auto Provide case also exercises touching-versus-gap routing.
The central predicate preserves strict overlaps, admits exact touches only
when enabled, rejects positive gaps, and requires at least one shared
dimension. The exact-touch test cases each use one shared dimension, so mixed
multi-dimensional combinations remain source-derived. Added direct API,
source, and test links and made that evidence limit explicit in the DDM guide
and policy note. The existing §§9.1.4/9.1.8 normative summary was not
independently rechecked against an authorized copy; no process or 2010 parity
is claimed. No diagram/C++ source/build/test/CI/Requirements Lab change.

## 2025 process timestamped-delete retraction route — pending review — 2026-10-12

Added one sequence to the TSO guide distinguishing coordinator-queued,
time-constrained removal from send-time RECEIVE-order removal, their different
session-route insertion points, receiver Resign versus Detach, client removal
buffer filtering, and callback-bridge admission. The focused process delete
test covers event delivery and optional-handle exposure, not Retract or the
receiver-Resign route; the actual deletion-retraction case is embedded-only.
The sequence has no fresh Mermaid syntax verdict, render, visual inspection,
or GitHub rendering. `mmdc` remains unavailable in the current shell. No build,
test rerun, CI, 2010, or Requirements Lab work.

## 2025 Disconnect profile-teardown flow — pending review — 2026-10-12

Revised the existing connection-guide flow to separate embedded transport
close, process-client socket and callback-bridge close, and conditional
server-side `ProcessFederationService::detach`. The source tree's service
header assigns Detach to a transport owner after its loop exits, but the
bounded production-source scan found no caller; the configured process
lifecycle test calls it separately in its server fixture. The flow has no
fresh Mermaid syntax verdict, render, visual inspection, or GitHub rendering.
Scoop search found no `mermaid-cli` package and `mmdc` is unavailable. No build,
test rerun, CI, 2010, or Requirements Lab work.

## 2025 committed Region Save/Restore sequence — pending review — 2026-10-12

Added a focused embedded `HLA_IMMEDIATE` sequence showing a committed,
unassociated Region deleted after Save and queried by its original handle and
bounds after Restore. It is bounded to the public integration case and does
not establish pending-edit, association, process-endpoint, host-restart, or
2010 behavior. The chart has no fresh Mermaid syntax/parser verdict, local
render, visual inspection, or GitHub rendering. `mmdc` is unavailable in the
current shell. No build, test rerun, CI, or Requirements Lab work.

## 2025 update-region association and queued TSO restore sequence — pending review — 2026-10-12

Added a separate embedded fresh-registry sequence for an active object-
attribute update-region association and an already queued TSO attribute
update. The public test runs HLA_EVOKED and HLA_IMMEDIATE, restores the
original Region bounds, mutates them, then observes the queued reflection at
both receivers with the source handle. It uses a new `EmbeddedFederationRegistry`
within the same test process; the inspected process-endpoint restore-status
case has no Region assertions. No process persistence, host-restart, normative,
or 2010 parity is claimed. The chart has no fresh Mermaid syntax/parser
verdict, local render, visual inspection, or GitHub rendering. `mmdc` remains
unavailable in the current shell. No build, test rerun, CI, or Requirements Lab
work.

## Process endpoint guide: portable routing wayfinding — 2026-10-12

The process-endpoint guide already separates the public service/callback
outcome from private request framing and labels its process-specific paths.
Its opening did not link to the 2025 object/interaction guide for the HLA-level
routing, receiver-eligibility, and per-federate object-knowledge model that the
endpoint walkthrough deliberately leaves to adjacent guides. Added that
wayfinding link instead of duplicating the existing flow. No Mermaid diagram
or C++ source changed; no tests/build/CI, 2010 inference, or Requirements Lab
edit.

## DDM: zero-dimensional Region is not the empty region-set argument — 2026-10-12

The 2025 DDM guide already distinguished an ordinary interaction, an empty
`RegionHandleSet` regional send, and a real zero-dimensional Region, but the
last case lacked direct test evidence and an explicit embedded-profile scope.
The current embedded overlap predicate returns no match when there are no
shared dimensions. The focused `HLA_EVOKED` interaction test first confirms
ordinary default-region delivery, then observes no callback for an explicit,
committed zero-dimensional source Region. Added source/test links and bounded
the prose to that implementation path; the test cites §9.1.3.2, but its comment
is not a substitute for authorized normative text. No new diagram, C++ source,
build, test rerun, CI, 2010, or Requirements Lab work.

## Relaxed DDM process-endpoint evidence correction — 2026-10-12

The earlier Relaxed DDM evidence note was too broad when it said process
behavior was not inferred. The focused configured
[process-endpoint multi-recipient interaction case](../../cpp/tests/ieee1516_2025_connection_multi_recipient_regional_interaction_catch2.cpp#L459)
composes separate enabled/disabled FDD configurations, checks the switch getter
at the sender and both receivers, and exercises per-recipient exact-touch and
positive-gap selection under HLA_EVOKED and HLA_IMMEDIATE. Its regions use one
shared dimension. The embedded interaction, object-attribute, and Auto Provide
exact-touch cases also use one shared dimension. Corrected the DDM guide and
policy note to record this selected endpoint evidence without claiming
multidimensional test coverage or broad embedded/process parity. The registry
predicate still supplies source evidence that every shared dimension must
qualify. No tests were rerun, and no build, CI, 2010, or Requirements Lab work
was performed.

## DDM and ownership: update-region state across transfer — 2026-10-12

Audited the three focused embedded cases in
[`ownership_transfer_update_region_catch2.cpp`](../../cpp/tests/ownership_transfer_update_region_catch2.cpp#L142)
against the ownership guide's existing sequence and the bounded registry
paths. The association-clearing case observes that transfer removes the
former owner's source association, a later reacquisition does not restore it,
and explicit reassociation is required. The deferred case observes that a
non-owner's region does not affect the current owner's updates, then becomes
active on transfer if it remains present. The cancellation case calls
`unassociateRegionsForUpdates` before transfer and observes the acquirer's
first update using the default source realization. These are three separate
HLA_EVOKED embedded cases, not one linear scenario. The association planner
stores non-owner state as deferred without changing scope; the selected
If-Wanted transfer path clears active state and promotes any remaining
deferred state. Expanded the DDM summary and retained the ownership guide as
the detailed sequence. The focused tests do not establish process/2010 parity
or every ownership-transfer route's normative behavior. No test/build/CI or
Requirements Lab work was performed.

## DDM and ownership: regular, negotiated, and assumption route survey — 2026-10-12

Compared the 2025 registry paths for regular acquisition of an unowned
attribute, Confirm Divestiture, and unconditional divestiture/assumption with
their focused ownership tests. The regular grant promotes the requester's
deferred update-region association and has no former active association to
clear in that branch. Confirm Divestiture clears the former owner's active
association and promotes the acquiring federate's deferred one. Unconditional
divestiture clears the active association as ownership becomes unowned, then
replans pending acquisition work; an assumption callback alone does not
transfer ownership, and a later successful acquisition uses its own promotion
path. The inspected focused ownership tests do not call update-region APIs, so
these additional association effects remain source-observed only. The earlier
three If-Available/If-Wanted cases remain the direct region-plus-transfer
evidence. Resign, unpublish, and administrative control-state mutations were
left outside this route survey. Updated the DDM guide with this boundary; no
diagram was duplicated or changed. No normative generalization, 2010 behavior,
or embedded/process parity is claimed. No tests/build/CI or Requirements Lab
work was performed.

## DDM and ownership: unpublish and Resign release routes — 2026-10-12

Audited the 2025 unpublish and selected ResignAction paths against the DDM
region-lifetime flow and focused embedded test sources. Unpublishing a
currently owned attribute clears its ownership and active update-region
association, then refreshes the region-use ledger before the service returns.
The focused region-lifecycle test source asserts that the registration's
source Region blocks deletion, unpublishing permits immediate deletion, and a
later query fails with InvalidRegion. This is direct embedded test-source
evidence for that selected sole-reference scenario; it does not establish the
outcome when other live references remain.

The selected unconditional-divestiture ResignAction enters the registry's
attribute-divestiture cleanup branch, clears active associations for surviving
instances, and replans pending acquisition work. Its focused embedded test
checks ownership loss and later assumption/acquisition behavior but does not
call region services, so the DDM effect remains source-observed only. This is
not a claim about every ResignAction. Extended the existing region-deletion
flowchart with the unpublish release route; no diagram was added or duplicated.
The edited Mermaid block was manually inspected but has no fresh parser or
render verdict. No normative generalization, 2010 behavior, embedded/process
parity, build, test run, CI, or Requirements Lab work is claimed.

## DDM and object deletion: when update-region references retire — 2026-10-12

Compared the pinned 2025 receive-order and timestamped Delete Object Instance
overloads with the embedded registry's object-purge gates and focused test
sources. Receive-order deletion can purge and refresh region usage in the
service call when no other recipient hold remains. With a known peer, the
object and its update-region references remain through the pending Remove
callback; the focused region-lifecycle cases assert same-call release in the
no-peer scenario and continued blocking until the peer crosses its final
Remove boundary.

Timestamped deletion stores a designator and recipients. The first accepted
timestamped Remove commits the deletion, but the backing object remains
available for legal reconstitution until the designator is terminal and the
object-purge gate has no remaining recipient holds. The focused embedded
timestamped-delete/retraction test covers retract-before-Remove and the later
timestamped Remove/retraction cutoff, but does not use region APIs or test a
post-callback, preterminal retraction. Therefore the TSO `deleteRegion`
release point and that intermediate restoration branch are source-observed,
not directly asserted by this test. Added a separate 2025 embedded flowchart scoped to this
region-reference lifetime; no process or 2010 parity is inferred. The Mermaid
was manually inspected but has no fresh parser/render verdict. No build, test
run, CI, or Requirements Lab work was performed.

## DDM deletion and region release: process endpoint evidence — 2026-10-12

Compared the configured process client's Delete Object Instance and Delete
Region requests with their service handlers. Both handlers call the shared
registry's corresponding operations, but focused process integration tests
cover deletion callbacks and region creation/commit/registration separately.
A separate process service-report case deletes a newly created, unused region,
but does not test an in-use rejection or release after object deletion. No
reviewed process case combines object deletion with a region-in-use query or
deletion. Therefore the public process endpoint's region-release timing is
not directly asserted. Added this evidence boundary to the DDM
guide; the embedded lifecycle chart remains explicitly embedded-only. Shared
registry call sites do not establish process/embedded parity. No build, test,
CI, or Requirements Lab work was performed.

## Save/Restore: separate public callbacks from restore-source selection — 2026-10-12

Reviewed the 2025 save/restore guide against bounded embedded restore-control
source and focused tests. Its public sequence already distinguishes request
admission, per-member begun/initiate callbacks, participant completion, and
the later federation outcome. One note compressed an Umbra implementation
choice into “After restart,” which could overstate what the tests demonstrate.
Reworded that note and added an implementation-only source-selection flow:
both restore paths validate a durable commit; a retained process-local
snapshot restores full private state, while a fresh registry applies only the
bounded durable projection and rebuilds selected work against current routes
after participant completion. Linked same-registry and fresh-registry cases;
the latter do not establish operating-system process restart, process-endpoint
parity, or 2010 behavior. The new Mermaid was manually inspected but not
parser/render checked: Scoop has no `mmdc` manifest. No builds, test reruns,
CI, or Requirements Lab work were performed.

## 2025 Federation Synchronization: clarify the pending-report loop — 2026-10-12

Reviewed the synchronization guide against the pinned 2025 API declarations,
bounded embedded registration, late-join, achievement, resignation, and
restore paths, and focused Catch2 test sources. The HLA-level flow already
separated registration result, participant announcements, individual
achievement reports, and the federation-level outcome. Clarified its pending
loop as a further outstanding participant report, not a repeat-report
instruction or a total callback-order rule.

The guide already distinguishes those public events from the embedded
registry's participant/result records and planned notification routes. The
focused sources cover explicit versus omitted participant sets, false-result
fan-in, resignation closure, one-member public save/restore, and codec
persistence. Partial-result-before-join, partially achieved barrier restore,
singleton completion, repeat achievement before completion, and forced loss during active
synchronization remain untested as stated in the guide. No 2010 behavior or
process/embedded parity is inferred. Mermaid was manually inspected, not
parsed/rendered. No build, test run, CI, or Requirements Lab work was performed.

## 2025 Order and Transportation: ownership is an effective-value boundary — 2026-10-12

Reviewed the order/transportation guide against the pinned 2025 `OrderType`
enum and callback signatures, bounded registry control and delivery paths, and
focused order, transportation, and process-endpoint test sources. The existing
guide already separates order from transportation, sender overload from
effective sending order, callback observation from delivery, and each from DDM
recipient eligibility and TSO timing. Added a pinned-enum locator and an
embedded-only ownership-transition flow: acquiring a previously unowned
attribute and Confirm Divestiture seed per-attribute effective values from the
new owner's current class defaults, while unconditional divestiture clears the
stored per-instance entries while the attribute is unowned. Qualified the existing
non-retroactive class-default statement to make the unchanged-owner boundary
explicit.

The cited ownership and effective-value paths are source evidence, not a
normative interpretation of transfer semantics. A focused embedded
`HLA_EVOKED` TSO test does deliver A's already-accepted update to B after
transfer and checks the original timestamp-order metadata, producer, and
retraction designator; it does not test a new send from B or query B's current
settings. No reviewed test directly asserts the new owner's effective order
and transportation values after transfer. The overlapping save/restore-
interlock case only verifies service rejection during save or restore. The
guide links to the TSO event-lifetime flow rather than duplicating its diagram.
It keeps the exact sender-to-receiver order mapping open for authorized
standard text and does not infer 2010 or process/embedded parity.
The new Mermaid was manually inspected, not parsed/rendered. No build, test
run, CI, or Requirements Lab work was performed.

## 2025 DataElement: byte-order ownership and representation layers — 2026-10-12

Reviewed the 2025 DataElement guide against the pinned `DataElement`,
`BasicDataElements`, and `VariableLengthData` APIs; bounded basic, fixed-record,
variable-array, logical-time, and process-frame sources; and focused Catch2
cases. Added a byte-order ownership table: callers choose a concrete BE/LE
type for the named integer and floating wrappers; one-octet elements have no
multi-octet order; other scalar formats supply fixed layouts; composites
delegate value octets to their children, with the variable-array count using
`HLAinteger32BE`; logical-time wrappers delegate encoding to the selected time
value/factory; and private frame metadata remains a distinct outer layer.
Clarified that the wrapper's one-octet boundary is neither encoded size nor a
byte-order guarantee, and linked the frame-header reader rather than relying
on the general receive entry point. Removed transient render-status prose from
the guide itself.

Focused 2025 tests demonstrate selected BE/LE vectors, fixed scalar and text
formats, array count/padding/truncation behavior, and delegation through both
reference time factories. They do not constitute a full element-by-element
conformance inventory, test replacement of a prefilled `VariableLengthData`
argument, or establish a universal representation for every logical-time
factory. The supplied-carrier replacement behavior remains source-observed.
No 2010 source or guide changed, and no behavior was inferred across editions.
No test reruns, build, CI, or Requirements Lab work was performed. The guide's
previously edited first encoding flow remains without a fresh parser/render or
screenshot verdict: `scoop install mmdc` found no manifest, and the configured-
bucket search found no Mermaid CLI package. Later unprivileged lookup attempts
could not resolve the configured hosts. No GitHub-rendering verdict is claimed.

## 2025 Authorization: credential handoff and Connect exception annotations — 2026-10-12

Rechecked the authorization guide against the pinned `Authorizer`,
`AuthorizationResult`, and `RTIambassador` declarations; bounded Umbra Connect
result mapping, RID profile loading, connected-operation gates, and focused
authorization test sources. Corrected the guide's Connect annotation summary:
both overloads without a `Credentials` parameter list `Unauthorized` but omit
`InvalidCredentials`; the two credential-taking overloads list both. Umbra's
shared Connect mapper can still map `INVALID_CREDENTIALS` to
`InvalidCredentials` for any overload, so this remains an implementation versus
header-declaration discrepancy, not a resolved normative contract. The full
licensed §4.2 text remains unavailable here. Clarified that later gates use a
retained credential value or synthesize `HLAnoCredentials` when Connect had no
credential. Corrected stale source anchors in the guide's evidence map.

The source places federation/federate authorization before the embedded versus
process service branch. The targeted repository search located dedicated
authorization coverage in `ieee1516_2025_authorization_catch2.cpp`; its
operation cases use the embedded test seam, and no process-specific
authorization integration case was found. The preexisting 7/10 focused CTest
result and three setup failures remain historical evidence; no tests were
rerun. The guide currently has four Mermaid blocks. A direct check of the
current inventory notes found dated Mermaid CLI 12.0.0 render evidence for all
four diagrams on 2026-10-11; the credential-envelope flow was also screenshot-
reviewed and found legible. This is targeted local evidence, not GitHub
verification. An interim review missed that inventory record and incorrectly
treated the fourth diagram as pending; the index and roadmap now reflect the
dated evidence. No 2010 behavior or cross-edition equivalence is inferred. No
source, test, build, CI, or Requirements Lab change was made.

Next handoff: review the 2025 service-invocation-reporting guide against its
pinned API, bounded source, and existing focused test evidence, separating the
HLA report contract from Umbra's report pipeline. Keep the full §4.2 Connect
annotation question open, preserve local-versus-GitHub rendering status, and
keep the 2010 stream separate. No builds, test reruns, CI, or Requirements Lab
work.

### 2025 service invocation reporting: interaction contract and pipeline — 2026-10-12

Reviewed the 2025 service-invocation-reporting guide against the pinned 2025
MIM, bounded embedded registry/writer/lifecycle source, and existing focused
interlock, callback-order, writer-failure, and ConnectionLost test bodies. The
MIM defines the interaction as reliable receive-order traffic scoped by the
joined subject and service group; its payload carries supplied arguments and
the invocation's success/return or exception result, with a per-member serial.
Added that interaction contract as a compact table and explicitly kept it
separate from Umbra's selected per-member file destination and the distinct
request-driven Federation MOM report path.

The source inspection confirmed the embedded interlock covers active, passive,
and regional declarations; interaction recipients use ordinary subscription
and DDM selection; an initially empty recipient set does not reserve a serial;
and callback-time eligibility is rechecked. The directed-send test observes
one specific report-before-directed-callback order under HLA_EVOKED, not a
service-wide order guarantee. The injected writer test's last recorded run
failed during federation creation before its switch/append assertions; no test
was rerun. Clarified that this example commits the Exception Reporting switch
in setExceptionReportingSwitch(true) before appending that service's success
report—the Service Reporting switch is an already-enabled precondition, not
the committed value. Corrected the reciprocal cross-guide link.

The guide retains eight diagrams. The four baseline views have dated GitHub
render/review evidence; the split embedded ConnectionLost views and matching
process producer-loss flow have local screenshot review, not GitHub
verification. Clarified the switch-failure sequence label to name Exception
Reporting explicitly; that revised sequence has no fresh Mermaid syntax,
render, or screenshot verdict. No 2010 behavior or process/embedded
equivalence is inferred. No C++ source, build, test rerun, CI, or Requirements
Lab change was made.

Next handoff: check whether an authorized copy of IEEE 1516.1-2025 clauses
5.8, 6.9, and 6.11 is available for the open question of current-value
reflection after adding a subscribed attribute to an already-known object.
If not, keep the normative question open and move to another 2025-only backlog
item without inferring the answer from Umbra source or tests. Keep the 2010
stream separate and the §4.2 authorization annotation question open. No builds,
test reruns, CI, or Requirements Lab work.

## 2025 standards access and committed Region Save/Restore reader review — 2026-10-12

Checked the repository's pinned 2025 standards tree for an authorized copy of
IEEE 1516.1-2025 clauses 5.8, 6.9, and 6.11. The tree contains the official
unmodified C++ API header distribution only, as its README states; the bounded
repository search found no full standard text or PDF. The question of whether
adding a subscribed attribute to an already-known object requires current-
value reflection therefore remains normative and unresolved. No conclusion is
inferred from implementation comments or focused tests.

Moved to the separate pending 2025 committed-Region Save/Restore item. The
existing sequence is explicitly one embedded HLA_IMMEDIATE public integration
case: commit a Region, complete Save, delete the live Region, restore, then
query the original handle, dimension set, and committed bounds. The cited test
also preserves a separate unachieved synchronization point in the same image;
the guide links that question to the synchronization guide rather than
combining the state machines. Its adjacent regional-TSO case correctly treats
queued source-region metadata as separate from the live Region object. The
source captures more Region state than this test asserts; pending edits,
associations, invalid-image paths, process endpoint, host restart, and 2010
remain outside its evidence. The current guide makes these boundaries clear,
so no content change or duplicate diagram was warranted. The Region sequence
still has no fresh Mermaid syntax/render or screenshot review, and no GitHub
rendering verdict.

No C++ source, build, test rerun, CI, or Requirements Lab work was performed.

Next handoff: render and screenshot-review the committed-Region sequence with a
supported local Mermaid preview, then record GitHub verification separately
after publication. If no supported renderer is available, leave the check
pending and continue to another 2025-only backlog item. Keep the licensed
standard question open until an authorized copy is available; preserve the
2010 boundary and make no general HLA Region-persistence claim from this case.

## 2025 process voluntary `DELETE_OBJECTS` route review — 2026-10-12

Reviewed the process-profile producer-Resign flow in the object/interaction
guide against the pinned 2025 API declarations, the shared registry resignation
preflight, process Resign handler, recipient dispatch, receive-side removal
gate, callback bridge, and the focused process delete case. The chart is
correctly labeled as Umbra 2025 process behavior: it separates producer
ResignAction intent from a surviving member's later `removeObjectInstance`,
shows route preflight before voluntary object mutation, and distinguishes the
default-pull reservation from optional-push commit. The source-level caution is
important: a survivor may poll while the producer's Resign request is still in
progress, so source call order does not establish whether the survivor callback
occurs before or after the producer observes successful return. Receiver Resign
clears still-server-queued events and removes that member's pending registry
reservations; already-pushed client-buffered callback work is a separate race.

Evidence remains deliberately asymmetric. The portable C++ TCK scenario
documents an adapter-agnostic expected lookup boundary but does not prove that
Umbra's process endpoint ran it. The focused Umbra process delete test asserts
an explicit `deleteObjectInstance`; its later `DELETE_OBJECTS` resignation is
cleanup after those assertions, not evidence for resignation-originated
delivery. The guide states this limit and does not claim process test coverage,
embedded parity, 2010 behavior, or callback ordering. No guide edit or duplicate
diagram was warranted.

The current shell has no callable `mmdc`, and its installed Node package set has
no Mermaid CLI. The prior configured-Scoop search found no Mermaid CLI
manifest, so this audit did not repeat installation attempts or substitute an
unsupported renderer. The committed-Region sequence remains without fresh
syntax/render or screenshot review; GitHub rendering is also unverified. No
build, test rerun, CI, 2010, or Requirements Lab work.

Next handoff: audit the adjacent 2025 process
`DELETE_OBJECTS_THEN_DIVEST` flow against the registry's delete pass followed
by the divest pass, the separate pull-queue and optional-push event ordering,
and the embedded test that observes callback order. Keep source ordering
distinct from HLA requirements; no focused process voluntary-action test
establishes callback order. Preserve the 2010 boundary and all pending renderer
verdicts.

## 2025 process `DELETE_OBJECTS_THEN_DIVEST` route review — 2026-10-12

Reviewed the process comparison in the object/interaction guide against the
shared registry resignation path, process result dispatch, pull selector,
optional-push ownership/removal delivery, the focused embedded Catch2 scenario,
and the portable C++ TCK case. The registry marks eligible deleted objects and
reserves known-survivor removals in its first pass, then skips those instances
while releasing ownership on retained objects and planning any eligible
assumption callbacks. The output is two distinct effect lists, not one combined
callback.

The embedded HLA_EVOKED case observes `requestAttributeOwnershipAssumption`
before `removeObjectInstance` for one recipient and two different objects.
That is an exact fixture result, not a standard-wide order. Process dispatch
submits assumption work before removal work; default pull uses distinct queues
and selects a pending removal before the later assumption-event branch (subject
to earlier event families in the poll). Optional push submits the assumption
frame before the removal frame; this establishes source frame-submission order,
not application callback order. The portable TCK scenario checks deletion vs
retained-object state and later If-Available acquisition, but not the assumption
callback or its relative order. The bounded process-test inventory contained
forced connection-loss directive cases, not a focused voluntary process
`DELETE_OBJECTS_THEN_DIVEST` callback-order case. No normative cross-service
order, 2010 behavior, or process/embedded parity is inferred.

The diagram's former decision label, “Which 2025 delivery projection?”, mixed
an embedded callback-model example with process queue modes. Renamed it to
“Which 2025 path is being described?” and labeled the embedded branch as
HLA_EVOKED evidence. No branch semantics changed. The Mermaid edit has no fresh
syntax, render, or screenshot verdict; GitHub rendering remains unverified.
No build, test rerun, CI, or Requirements Lab work.

Next handoff: audit “Receiver Resign while assumption and removal callbacks
are pending” in the same guide. Compare registry marker/reservation invalidation
and server queue cleanup with pushed-frame buffering and callback dispatch.
Keep the nearest process test's consumed-before-Resign case distinct from the
uncovered pending-frame race; do not claim callback cancellation or timing as
normative. Preserve 2010 separation and pending renderer verdicts.

## 2025 process receiver Resign with pending assumption/removal work — 2026-10-12

Reviewed the receiver-side flow against the shared registry resignation
cleanup, process session queues, client request/response buffering, callback
bridge, and the focused pushed-assumption process test. If an event remains in
the server pull queues, receiver Resign removes that member's pending
assumption marker and object-removal reservation, filters matching ownership
events, and clears the session's removal and ownership queues before the
successful response. This source trace does not establish a normative
callback-cancellation rule.

A push event has crossed a different boundary: the client may buffer a frame
while waiting for the Resign response and dispatch it after the matching
response. Already-entered application callbacks cannot be undone. The object
guide now leaves callback-model and shared-dispatcher details to the callback
guide, with reciprocal links from both that guide and the lifecycle guide.
The nearest process test consumes the assumption callback before Resign; no
focused case establishes whether the combined pending assumption/removal race
is cleared or later observed. No post-Resign delivery, global callback order,
process/embedded equivalence, or 2010 behavior is claimed.

The existing lifecycle/connection guides already explain ambassador
connection, membership, and execution as separate lifetimes, so this review
added wayfinding rather than another Resign-versus-Disconnect diagram. The
receiver flow has no fresh Mermaid syntax/render or screenshot verdict, and
GitHub rendering remains unverified. No C++ source, tests, build, CI, or
Requirements Lab work.

Next handoff: review the existing 2025 callback-exception flow in
`HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md` against the pinned API,
the bounded dispatcher/service paths, and focused test sources. Check whether
it clearly separates the active public call and its result, callback exception
escape versus containment, state effects already committed, and queued work
under HLA_IMMEDIATE/HLA_EVOKED and embedded/process paths. Add no duplicate
diagram unless the survey finds a real reader gap. Keep the 2010 stream
separate; no builds, test reruns, CI, or Requirements Lab work.

## 2025 callback exceptions: call stack, commit point, and containment — 2026-10-12

Audited the existing callback-exception section and evidence map against the
pinned `receiveInteraction` and Evoke declarations, the common callback
dispatcher/session, embedded `Send Interaction`, the process client's
post-response event drain, both `ConnectionLost` producers, and the selected
dispatcher and process-integration test sources.

The guide correctly separates three questions: which public API call is active
when a callback throws; which producer catches or reports that exception; and
what state/report effects already crossed their own commit boundary. The
generic dispatcher removes a task before invocation. A throwing session
decrements its in-flight count and rethrows; HLA_IMMEDIATE releases the drainer
election while retaining later FIFO tasks, whereas HLA_EVOKED propagates
through Evoke with the failed task consumed and later queue entries retained.
The cited unit cases exercise normal FIFO/concurrent delivery, not a throwing
task or backlog recovery.

For the bounded embedded Send Interaction example, the successful service
record and accepted-send counter precede callback fan-out. Its `Exception`
catch attempts exception reporting and a failed service record, then rethrows;
those writes do not roll back the earlier accepted effects. Under
HLA_IMMEDIATE the exception may unwind through the inducing call only when
that call's thread owns the active drainer; under HLA_EVOKED the send has
returned before the later Evoke failure. This is source-derived; no focused
throwing-callback test asserts attribution, reports, or rollback.

The process case is correctly kept separate: unsolicited frames are buffered
while a correlated recipient request waits, then dispatched after the
response is received and the stream lock released. A callback exception can
therefore escape the recipient-side request/pump or later Evoke, not the
producer's original Send Interaction stack. No reviewed process case throws
from federate code. `ConnectionLost` is a separate RTI-notification path whose
embedded and process producers both catch callback exceptions after different
local state transitions; it is not a general dispatcher policy.

No new diagram was warranted: the existing dispatcher, call-stack, embedded
Send, and `ConnectionLost` flows already distinguish these outcomes and label
their source/test limits. Changed the service-report evidence wording from
“current local run” to “a recorded local run,” since this documentation audit
did not rerun it; that recorded attempt failed during federation creation
before reaching callback assertions. The standard's precise exception
propagation rule remains unverified without the applicable licensed clauses.
No 2010 behavior, process/embedded parity, or new test result is claimed. The
existing exception diagrams have no fresh syntax/render or GitHub verdict.
No C++ source, test, build, CI, or Requirements Lab work.

Next handoff: audit the process-profile 2025 save/restore traces in
`HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md` against the process client,
service handlers, registry protocol transitions, and the focused process
success/early-failure/terminal-failure test sources. Determine whether current
diagrams distinguish a synchronous request response from later federation
callbacks and state-image effects, and whether a process-only flow adds a real
reader benefit beyond the embedded traces. Do not infer process Region or OS
restart persistence, embedded/process equivalence, or 2010 behavior. No builds,
test reruns, CI, or Requirements Lab work.

## 2025 process Save/Restore callback-to-service boundary — 2026-10-12

The existing Save and Restore protocol diagrams already teach participant
work, federation commit/rehydration, and the distinct request-failed versus
terminal-failure outcomes. A bounded process-profile audit found a reader gap
at the public call boundary: service responses are private correlated control
results, while the HLA callbacks arrive as event frames. The process client
buffers pushed frames until the matching response and drains them afterward;
`HLA_IMMEDIATE` may invoke the user callback before the inducing public method
returns. The Save guide now contrasts this with the tested HLA_EVOKED polling
case, where callbacks wait for explicit Evoke.

Added one process-specific Restore sequence to the existing 2025 guide. It is
limited to the one-member, pushed-event, HLA_IMMEDIATE public cases: accepted
request callbacks precede `requestFederationRestore` return; a missing image
produces only `requestFederationRestoreFailed`; the later `federationRestored`
or `federationNotRestored` callback follows participant completion/reporting.
The image transition is shown only at the completion barrier. The test server
runs in a thread over loopback TCP, so these cases do not demonstrate RTI
process restart. No HLA_EVOKED Restore, multi-member call-timing, Region
process-persistence, embedded parity, or 2010 result is inferred.

The working-tree inventory is 267 Mermaid diagrams in 34 design files (154
flowcharts, 90 sequences, 23 state diagrams; 14 separate 2010, 238 2025 topic,
13 cross-cutting, 2 index). The new sequence has no fresh parser/render or
GitHub verdict: `mmdc` is absent and the permitted Scoop catalog search found
no Mermaid CLI package. No source, tests, build, CI, or Requirements Lab work.

Next handoff: render and inspect the new process Restore sequence with a
GitHub-compatible Mermaid tool; then audit whether the process Restore
HLA_EVOKED polling path has focused public-test evidence. Keep its result
distinct from process HLA_EVOKED Save evidence and preserve the 2010 boundary.

## 2025 process Restore polling sequence — 2026-10-12

The HLA_EVOKED audit found direct configured-process Restore lifecycle tests
in `ieee1516_2025_connection_synchronization_restore_catch2.cpp`, including
early rejection, accepted success, terminal failure, abort, and a two-member
status/callback case. Polling tests use `ProcessFederationServiceOptions{false}`
and show the public Restore/completion call returning before the corresponding
callback is observed; explicit Evoke calls retrieve and deliver queued events.
The separate process ownership-queue test uses pushed service events with
`HLA_EVOKED`, so frame receipt and user callback invocation are not conflated.
These are selected 2025 process-profile tests, not an exhaustive mode matrix,
an OS-process restart, embedded parity, or 2010 evidence.

Added a polling sequence next to the existing one-member pushed-event
`HLA_IMMEDIATE` sequence and linked the polling selector/notification/pump
sources and focused tests. The existing Immediate diagram's negative scope
remains local to that trace; the new two-member citation does not turn it into
a multi-member timing chart. Mermaid parsing/rendering and GitHub presentation
remain unverified because `mmdc` is unavailable and Scoop has no matching
package. No C++ changes or test/build/CI runs.

Next handoff: validate both process Restore sequences with a GitHub-compatible
Mermaid renderer, then audit process-endpoint Region Save/Restore support from
the public tests and source. Do not infer process Region persistence from the
embedded saved-Region cases; keep IEEE 1516.1-2010 separate.

## 2025 process Region image sequence — 2026-10-12

Review scope: process client/service Save/Restore delegation, registry image
capture/commit, typed Region-section codec, and registry rehydration. The
sequence distinguishes the process control adapter from the Region image
implementation and calls out the older missing-section versus present-empty
compatibility behavior. Per-member regional declarations, object update-region
associations, and queued-message source snapshots remain separate image data.

Evidence: source only for the configured-process Region path. The inspected
process Save/Restore cases cover lifecycle/status/callback transport without
Region operations. The file-backed Region test restores into a second registry
in the same test process; it is neither configured-process endpoint coverage
nor an OS-process restart test. No HLA normative or IEEE 1516.1-2010 claim.

Verdict: parser/render and GitHub presentation pending. `mmdc` is unavailable
and Scoop has no matching package; repository-derived diagram text was not
sent to a hosted renderer.

## 2025 joined-federate MOM Save/Restore profile boundary — 2026-10-12

Compared the joined-federate MOM `HLAfederateState` state flow with the
embedded save/restore test and configured process-connection test sources.
The embedded test observes the initial active value and the conditional
`3 → 1` and `5 → 1` reflections; a bounded search of
`ieee1516_2025_connection_*.cpp` and `.hpp` finds no direct process test of
this attribute across Save/Restore. The process Restore callback path exists
in a separate guide, but shared registry state derivation is source-only
evidence for the process endpoint.

Added an explicit process-profile test gap and labeled the state-transition
case embedded-only. No new or revised diagram, so no new parser/render verdict
is needed. The pending process Restore and Region image diagrams remain
unrendered; the local UI exposed no targetable desktop app. Existing hosted
Mermaid tabs were not opened or changed. No build, tests, CI, 2010, or
Requirements Lab work.

## 2025 Connect finalization ordering — 2026-10-12

Compared the existing Connect validation/finalization flow with the pinned
2025 `RTIambassador` overloads and the bounded Umbra `connectImpl` path. The
implementation prepares authorization, configuration, callback session, and
endpoint first; in the final locked section it repeats the state check, applies
`not_connected → not_joined`, and installs the prepared callback/session and
connection fields before returning. The guide's two nodes could read as a
separate HLA-visible transition, so the wording now identifies this as source
ordering and the chart groups it in one final locked section. Focused tests
cover selected validation conditions separately; none asserts the internal
finalization sequence or combined-invalid exception priority. No 2010
equivalence is inferred.

The revised Mermaid flow has no fresh parse/render or GitHub verdict because
`mmdc` is unavailable and the local UI exposed no targetable desktop app. No
hosted renderer was used. No build, tests, CI, or Requirements Lab work.

## 2025 first-object reading path and additional-FOM observer boundary — 2026-10-12

Compared the 2025 connection, authorization, federation lifecycle, FOM,
name-reservation, and object-information guides. Existing focused guides
already teach the reader path; the index labels it as orientation, keeps
Create/Join distinct, and presents reservation only as the optional named
registration branch. No umbrella flow was added.

The additional-module Join source audit found atomic definition/handle-directory
replacement with membership, without an ordinary object-discovery planner call
in that registry commit path. Selected embedded and process subscription paths
explicitly plan discovery after declaration changes. Existing class/interaction
lookup tests and the process additional-FOM test do not assert callbacks to a
pre-existing subscriber after Join. The FOM guide now records this as an Umbra
source observation and an open test/standards question, not as a portable
prohibition or guarantee. No diagram changed or rendered; the pre-existing
local-render queue remains. No build, test, CI, 2010, or Requirements Lab work.

## 2025 additional-FOM extension shapes and observer-state boundary — 2026-10-12

Compared the accepted lookup fixture, official Restaurant extension, composer,
registry handle reconciliation, subscription records, and discovery resolver.
The accepted test adds a child class under the existing object root and proves
catalog lookup/old-handle equality only. The official Restaurant extension
adds a member under existing Soda and interaction parameters, but its exact
additional-module case is rejected at FDD materialization. Generic recursive
merge is not proof that every merged shape commits.

The registry retains existing explicit class/attribute subscription records
and per-instance owner state while replacing the catalog/handle directories.
A later subscription path plans against current model/runtime state; the Join
commit itself does not schedule ordinary discovery. No successful test covers
an existing-class attribute addition or the resulting pre-existing observer
callbacks. Added those distinctions to the FOM guide; they remain Umbra-source
observations and test gaps, not HLA requirements. No diagram/render/build/test/
CI/2010/Requirements Lab work.

## 2025 additional-FOM interaction subscription and later-send boundary — 2026-10-12

The interaction-class and parameter lookup cases make added classes and an
inherited parameter visible to an already-joined member, but do not publish,
subscribe, send, or receive. A separate ordinary inherited-receipt case tests
parent subscription to child send with the full hierarchy present at Create;
it does not add a module after Join.

The ordinary receive-order planner uses the current catalog parent chain and
the receiver's stored per-class subscriptions for a later send. The source
therefore permits the inference that an existing ancestor subscription may
match a future send of a newly added descendant, but this post-Join scenario
has no focused test. Catalog growth itself does not synthesize a Receive
Interaction callback. The configured-process additional-FOM test covers object
lookup, not interaction delivery. Added this boundary to the FOM guide and
cross-linked ordinary object discovery and directed routing as distinct paths.
No diagram changed/rendered; no normative or 2010 claim, build, test, CI, or
Requirements Lab work.

## 2025 HLA-first onboarding map — pending local review — 2026-10-12

Added a cross-guide flow to the conceptual map in
`HLA-BEHAVIOR-FLOW-GUIDES.md`, connecting factory construction and Connect,
Create/Join and FOM vocabulary, optional name reservation, object registration,
and peer discovery eligibility. It is a 2025-only reader synthesis; no test
asserts the entire chain, and the flow does not imply attribute reflection or
embedded/process parity. No fresh Mermaid syntax verdict, local render, visual
inspection, or GitHub render has been recorded. Keep it in the local review
queue; do not use a hosted renderer for repository-derived text.

## 2010 binding-shell onboarding trace — pending local review — 2026-10-12

Added a 2010-only smoke-traced sequence in
`HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md` from ambassador construction and
Connect through Create/Join, object registration/discovery, and
update/reflection. It is bounded to the assert-based `ieee1516_2010_binding_shell_smoke`
and the in-process reference runtime; the ignored FOM path, lazy handles, and
inline callbacks are profile observations, not portable or 2025 rules. No fresh
Mermaid syntax verdict, local render, screenshot, or GitHub review is recorded.

## 2025 process Federation MOM additional-module Join — pending local review — 2026-10-12

Added a source-mapped sequence in `HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md` for
the configured process profile: additional-module Join, the Federation MOM
Conditional-update enqueue gap, subscription-driven discovery with Static
initial values, and a separate current-value request. The pinned MIM supplies
the Conditional triggers; process source supplies the profile trace. The
existing configured-process test checks class lookup, not Federation MOM
observer discovery/reflection. This is not a conformance finding or embedded/
2010 parity claim. `mmdc` is unavailable and the prior Scoop catalog check
found no Mermaid CLI package. No fresh syntax verdict, render, screenshot, or
GitHub review is recorded; keep this as the seventh item beside the existing
six-chart local review queue. Do not use a hosted renderer for repository-derived
text.

## 2025 process Federation MOM membership-list Join and Resign — pending local review — 2026-10-12

Added a separate process-profile sequence in
`HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md` for the MIM's Conditional
`HLAfederatesInFederation` Join/Resign trigger. It distinguishes the process
service's `HLAfederate` discovery/removal events from Federation-list reflection
and from a separate current-value request for an existing observer. The pinned
MIM and source support the trigger and process-path description; the focused
membership-reflection test is embedded-only, while the selected process test
covers member-object removal/rejoin order. No process Federation-list callback
is test-verified, and no 2010 or embedded/process parity is claimed. `mmdc` is
unavailable and the prior Scoop catalog check found no Mermaid CLI package. No
fresh syntax verdict, render, screenshot, or GitHub review is recorded; this is
the eighth pending item beside the prior seven. Do not use a hosted renderer for
repository-derived text.

## 2025 process Federation Synchronization Point route audit — 2026-10-12

Audited the configured 2025 process registration and achievement handlers,
registry plans, recipient-event routing, correlated client request, callback
adapter, and the focused connection cases. The public HLA methods remain
`void`: private correlated statuses are translated by the adapter, while
`federationSynchronized` is a distinct participant event. Pushed final events
are buffered during response matching and dispatched after the request lock is
released but before the private result returns to the adapter; polling mode
retains them for a later `receive_interaction` request. Callback-model timing is
separate from that push/poll choice.

The callback/service-ordering guide already contains the detailed source trace.
Added a short process-boundary explanation and direct test links to the
Federation Synchronization guide rather than duplicating its sequence. The
selected public process cases pair HLA_EVOKED with polling and HLA_IMMEDIATE
with pushed events, use an explicit set containing only the receiver, and
exercise a false final report. They do not prove the other pairings, exact
service-return ordering, service-report ordering, or process/embedded parity.
No 2010 behavior is inferred. No Mermaid changed, so the eight-chart render
queue is unchanged; no build, test, CI, or Requirements Lab work was done.

## 2025 process Save/Restore status-query sequence — pending local review — 2026-10-12

Added a process-profile sequence to the existing Federation Save/Restore guide
after auditing its status-query section, process Save/Restore handlers, registry
snapshot plans, requester event queues, correlated response drain, callback
projection, and focused connection tests. The public queries remain `void`;
their private process response carries control status, while Save status pairs
or Restore descriptors arrive separately as a requester callback event. The
sequence labels pushed-before-response buffering and later polling separately
from callback-model dispatch; this is source-derived, not an HLA callback-order
rule.

Process Save status tests cover terminal `NO_SAVE_IN_PROGRESS` snapshots after
failure, not an active Save snapshot. Process Restore status tests cover active,
waiting, and terminal two-member descriptors under `HLA_IMMEDIATE`; they do not
cover the full transport/callback-model matrix. No 2010 or embedded/process
parity is inferred. This is the ninth pending chart beside the prior eight:
`mmdc` is unavailable, and no fresh syntax/render verdict, screenshot, or
GitHub review is recorded. No build, test, CI, or Requirements Lab work was
done.

## Local Mermaid render and visual review of the nine-chart queue — 2026-10-12

Installed Mermaid CLI 12.0.0 globally through the existing Scoop Node.js/npm
installation, including its Puppeteer Chrome. The restricted shell could not
launch Chrome; the local render required the approved elevated process launch.
Rendered artifacts remained under the user's temporary directory. No
repository-derived diagram content was sent to a hosted renderer.

The targeted render covered all 48 Mermaid diagrams in five Markdown sources:
the HLA-first index (3), the separate 2010 reference-RTI guide (13), the 2025
Federation MOM guide (10), the 2025 Connect/configuration guide (4), and the
2025 Save/Restore guide (18). This is not a render of all 275 diagrams.

All nine queued charts were rendered and visually inspected locally:

- HLA-first onboarding map — flow from ambassador construction through
  Create/Join and object-registration eligibility.
- 2010 binding-shell trace — retained as a distinct 2010 reference-profile
  sequence, with no 2025 parity claim.
- 2025 process Federation MOM additional-module Join and membership-list
  Join/Resign traces — separate Conditional-update and per-member object paths.
- 2025 Connect finalization flow — the state transition and installation of
  prepared connection state remain grouped in one final locked section.
- 2025 process Save/Restore status-query sequence — compacted to five lanes;
  the private correlated control result remains separate from the requester
  status callback, and callback model remains separate from event transport.
- 2025 process Restore polling sequence and process Region-image sequence.
- 2025 embedded committed-Region Save/Restore sequence — shortened the
  over-wide note that had clipped at the left edge; callback-protocol context
  now sits in the prose.

The renderer found sequence-diagram syntax errors where semicolons split
message/note statements in the 2010 binding-shell trace, the 2025 MOM traces,
and Save/Restore traces. Replaced those separators with em dashes, then reran
the affected full-source renders successfully. The queue is cleared for local
syntax/render and visual review. GitHub rendering remains pending until the
documentation is published; these local images are not a GitHub presentation
verdict. No C++ changes, tests, builds, CI, or Requirements Lab work.

## 2025 process Attribute Ownership Acquisition If Available route — 2026-10-14

Audited the pinned 2025 public API, ambassador, process ownership handler,
receive/poll route, callback bridge, and focused process-test source. The public
operation returns `void`; a private correlated status/count result completes
the service call and a failure status maps to a typed public exception. The
secured/unavailable per-attribute outcome is separate callback data. In the
default process pull mode, the handler queues the request ID and a later
`receive_interaction` resolves it. With the optional push mode, resolution and
event-frame submission happen during request handling. These are Umbra
transport paths, not a portable callback ordering guarantee.

The focused process test uses HLA_EVOKED and the default pull configuration. It
checks one known object with one unowned attribute, then checks the notification
and tag after callback evocation. It does not query ownership or exercise mixed
results, an unavailable attribute, push mode, or If-Available cancellation.
Nearby process tests for Release Denied and cancellation start with ordinary
`attributeOwnershipAcquisition`; they are not evidence for If-Available
coverage. The guide now labels the existing callback-time state chart as the
embedded case and adds a separate process transport flowchart.

The new chart rendered with local Mermaid CLI 12.0.0 and was visually checked.
This is one chart from the ownership guide, in addition to the prior 48-chart
targeted render across five sources; it is not a full-guide/tree render, and
GitHub presentation remains pending publication. No builds, tests, CI, 2010, or
Requirements Lab work.

Next: audit the 2025 process Release Denied callback/status route against the
pinned API, service handler, callback bridge, and focused test. Keep its
ordinary-acquisition trigger distinct from If-Available and any 2010 behavior;
update the guide only for a concrete reader gap.

## 2025 process Attribute Ownership Release Denied route — 2026-10-14

Audited the pinned API's owner-side §7.12 service and requester-side §7.10
callback against the 2025 process ambassador, service handler, registry plan,
pull receive path, callback bridge, and focused public test. Added a separate
process sequence to the ownership guide: the owner's public `void` call
completes from a private status/recipient-count result, while the requester's
`attributeOwnershipUnavailable` callback arrives through a later process poll
and HLA_EVOKED dispatch. The denial tag is carried into the callback; the
existing HLA-level chart remains the portable ownership path.

The focused process case uses HLA_EVOKED, default pull mode, one requester, and
one owned attribute. It observes the owner call return normally, no requester
report immediately afterward, then one unavailable report with the expected
object, attribute, and denial tag after requester callback evocation. It does
not query ownership, test immediate dispatch, multiple requesters, stale-event
suppression, or the denial/cancellation race. No process behavior is inferred
for 2010.

The separate source-only note records Umbra's current-state recheck before
exposing a queued pull event, its recipient grouping/follow-up planning, and
its Release-Denied-wins handling when cancellation races an in-flight owner
release callback. These implementation observations are not portable timing
claims; the focused test does not verify those branches. Mermaid CLI 12.0.0
rendered and visually checked the new sequence. A semicolon statement
separator found by the first render was replaced; the final diagram renders
cleanly. At that increment, all 103 relative Markdown targets in the ownership
guide resolved. No builds, tests, CI, 2010, or Requirements Lab work.

Next: audit the 2025 process Attribute Ownership Acquisition Cancellation
route across the public API, cancellation registry state, process service and
callback bridge, and focused tests. Compare its normal cancellation outcome
with the documented Release-Denied-wins race, keep ordinary acquisition and
If-Available cases distinct, and add only a concrete reader-facing correction.

## 2025 process Attribute Ownership Acquisition Cancellation — 2026-10-14

Audited the pinned §7.15 service and §7.16 callback against the process
ambassador, registry reservation and confirmation delivery, service handler,
receive path, callback bridge, and focused public process case. Added a
separate default-pull sequence to the ownership guide. The public `void` call
completes from a private applied-status/recipient-count result; the process
confirmation event is later consumed at `receive_interaction`, where the
reservation is committed and canceled attributes are removed from pending
requests before the no-tag callback is projected. Configured push mode commits
inside the handler before sending the unsolicited event; it is source-only in
this survey.

The focused process case uses HLA_EVOKED and one regular-acquisition attribute.
It observes no confirmation immediately after the cancel call, then checks
the confirmation's object and attribute after requester callback evocation,
with no acquisition callback reported. It performs no ownership query. The
test does not poll the owner after cancellation, so the empty owner release
report list does not prove that process delivery suppressed a queued owner
callback. It also does not exercise push/immediate delivery, empty sets, or the
Release-Denied-wins race. Current source treats an empty set as applied with no
reservation/callback; normative status was not established. The registry
allows an owner callback already entered in user code to answer after cancel
acceptance; Release Denied then wins the terminal outcome, a source observation
not tested at this process boundary.

The new sequence rendered and was visually checked with local Mermaid CLI
12.0.0. The current ownership guide's 111 relative target paths resolve with
no missing files. No builds, tests, CI, 2010, or Requirements Lab work.

Next: audit whether 2025 process object deletion or requester resignation can
suppress an accepted but not-yet-dispatched cancellation confirmation. Compare
the process receive gate and existing public tests with the embedded evidence;
do not infer process parity from the embedded invalidation cases.

## 2025 process cancellation invalidation before callback — 2026-10-14

Compared the explicit process Resign path with the pull cancellation receive
gate. When Resign completes first, connection-loss cleanup removes queued
ownership-acquisition events whose requester or recipient is the departing
member, then joined-session cleanup clears that session's remaining ownership
events. If pull receive reaches the cancellation event after object state has
changed, the registry rejects a missing object or stale cancellation; an
accepted deletion or requester membership/object-knowledge loss consumes the
cancellation but yields no confirmed attributes (or no delivery when there is
no follow-up work), which the receive loop skips before callback projection.

Added a 2025 process-source lifecycle flowchart to the ownership guide. The
focused process case covers successful confirmation only, not deletion or
Resign interleaving. The embedded deletion test's unchanged cancellation-report
count after a second cancel/delete shows that suppression in that embedded
case only. The standalone Resign test covers pending acquisition and owner
release-callback suppression, not pending cancellation confirmation. No process
parity or normative cancellation-invalidation rule is inferred.

Mermaid CLI 12.0.0 rendered the new flowchart using the installed Chrome, and
the image was visually checked. Current static `docs/design` census: 273
Mermaid fences across 43 Markdown files, 32 containing diagrams (156
flowcharts, 96 sequences, 21 state diagrams). This is not a complete render
audit or an edition-stream reconciliation. No builds, tests, CI, 2010, or
Requirements Lab work.

## 2025 process push-mode cancellation dispatch boundary audit — 2026-10-14

In push mode, the service consumes the cancellation reservation before sending
the unsolicited confirmation frame. The client buffers pushed events while it
waits for the matching service response, then submits those events to the
callback bridge before its request returns. The bridge submits a callback task:
`HLA_IMMEDIATE` may drain it synchronously, while `HLA_EVOKED` leaves it in the
local dispatcher. The submitted task does not recheck the federation registry.

The inspected process Resign branch completes the service and lifecycle
transition and clears the ambassador's joined identity, but does not reset the
callback dispatcher. Server cleanup removes still-queued service events for a
departing member; it cannot retract an event frame already sent. Consequently,
source inspection leaves a queued evoked cancellation confirmation available
after object deletion or requester Resign. This is not process-test evidence or
a portable HLA rule. The focused process cancellation case covers default-pull
confirmation only. If the push send fails, the handler returns its internal
error path after consuming the reservation and sends no confirmation frame; that
failure seam is also untested.

Added the source-mapped push-delivery flowchart to the 2025 ownership guide and
rendered it with Mermaid CLI 12.0.0 using the installed Chrome; the white-
background image was visually checked for legible branches and connectors. The
guide's 129 relative target paths resolve with no missing files. A fresh static
`docs/design` census found 280 Mermaid fences across 43 Markdown files, 34 with
diagrams (159 flowcharts, 98 sequences, 23 state diagrams). This is not a
whole-tree render or visual review. No builds, tests, CI, 2010, or Requirements
Lab work.

Next: review the existing 2025 Time Management guide for one concrete reader
gap in the relationship between time-advance requests, queued timestamped
delivery, and grant eligibility. Use existing guide/source/test evidence only;
add a cohesive flow only if it clarifies a real boundary. Do not build, run
tests, start CI, edit the Requirements Lab, or change/infer 2010 behavior.

## 2025 DDM local visual review — 2026-10-14

Rendered four existing charts with Mermaid CLI 12.0.0 and locally inspected
their PNGs: the regional Request Attribute Value Update versus later receiver
DDM flow; the Region deletion gate; the interaction-region dimension-context
flow; and the FOM-dimension-to-Region bridge. The first is tall but its
provider-solicitation and later receiver paths remain separate and readable.
The deletion gate's left-to-right release loop and ownership-unpublish path
are clear. The interaction chart's send and subscription branches are
legible as separate tests. The FOM bridge is tall but readable, with the
optional Region-set read-back distinct from range/commit/use steps.

This is four targeted 2025 chart reviews across two guides. It does not
establish full-guide or whole-tree visual review, GitHub rendering, normative
completeness, or 2010/process parity. Source and test audits are recorded in
the dated progress-log entries; no tests/build/CI were run for these reviews.

Next: review the queued-TSO receiver-state traces in the 2025 DDM guide around
lines 1388 and 1444. Keep receiver-region revalidation and the interaction's
send-time source-region snapshot versus callback-time current subscription
bounded to the cited scenarios; infer no universal callback ordering, process
parity, or 2010 behavior.

## 2025 queued-TSO receiver-state trace render review — 2026-10-14

Rendered both current Mermaid blocks in the 2025 DDM guide with Mermaid CLI
12.0.0 and visually inspected their PNGs. The attribute trace is a tall
five-participant sequence, but the publisher/registry/subscriber lanes and
time-6 suppression versus next-gate removal remain distinct. It explicitly
omits the disconnected publisher's separate `connectionLost` report, avoiding
an implied total callback order. The interaction trace reads as two sends
around one receiver-subscription replacement: source-region snapshots A/B are
retained with the sends, the current subscription B suppresses the old A send,
and the later B send produces the tested callback sequence. The added prose
clarifies that the receiver's subscription is re-resolved at the callback gate,
not stored as a frozen receiver-region snapshot.

The first revised render caught a semicolon parse error in a sequence message;
the label was corrected and both final renders succeeded. This is a targeted
2025 embedded source/test review, not a normative algorithm, a full-guide or
whole-tree render, GitHub verification, or process/2010 parity. No builds,
tests, CI, or Requirements Lab work.

Reviewed in the following DDM/time/callback-filter entry.

## 2025 DDM/time/callback filter map — 2026-10-14

Rendered the revised conceptual map from the 2025 DDM guide with Mermaid CLI
12.0.0 and visually inspected the PNG. The layout fans one service/receiver
scenario into four distinct questions—regional scope, RECEIVE/TIMESTAMP,
callback model, and callback enablement—with no arrows between them. The
focused cited process test uses `HLA_EVOKED` for both clients and separately
toggles callbacks; it does not compare the two callback models. The map and
prose preserve the case-specific process boundary and make no embedded or 2010
parity claim. This is a targeted local render, not a whole-guide/tree or GitHub
render; no build, test, or CI was run.

The ownership/update-region seam audit and next render review are recorded
below.

## 2025 ownership/update-region seam audit — 2026-10-14

Compared the three focused embedded test scenarios with the existing DDM and
Attribute Ownership explanations. The source/test distinctions and
non-normative/profile boundaries are already explicit, so no diagram or
wording change was warranted. No tests or builds were run.

## 2025 object-instance name reservation — 2026-10-14

Rendered all seven Mermaid blocks in the 2025 name-reservation guide from its
Markdown source with Mermaid CLI 12.0.0 and visually reviewed the PNGs. The
owner/peer visibility map and reservation-lifetime state diagram had
compressed horizontal layouts; both were reoriented top-down and the full
seven-chart render passed again. The revised charts are readable in the local
preview. GitHub review remains pending until publication; no build, test, or
CI was run.

Next: review the repaired transportation pending-state chart in
`docs/design/HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md` (§2). Check the
requested-versus-confirmed boundary and visual legibility; keep standard-text
gaps and the 2010 stream separate.

## 2025 transportation pending-state chart — 2026-10-14

Rendered all six Mermaid blocks in the 2025 Order and Transportation guide
with Mermaid CLI 12.0.0. The revised §2 state chart initially exposed a
semicolon parse error that produced stray states; simplifying its transition
labels fixed the parse, and the final chart was visually inspected. Its
portrait layout is readable and distinguishes admission, pending persistence,
commit-before-public-confirmation, and stale/cancelled discard.

The embedded source mapping confirms that the registry commits immediately
before invoking the public confirmation method. That is a source ordering,
not proof of a portable or normative effective time; the confirmation remains
the federate-visible boundary. `HLA_IMMEDIATE` may dispatch inline during the
request service; `HLA_EVOKED` waits for callback dispatch. Attribute ownership
loss can clear the previous override as it removes pending work, so discard is
not drawn as preserving the old effective value. Linked focused restore cases
cover both callback models, but not every scheduler interleaving. This is a
targeted local render/visual check only; no GitHub or whole-tree review, build,
test, CI, 2010, or Requirements Lab work occurred.

## 2025 subscription-state targeted chart review — 2026-10-14

Rendered all eight Mermaid blocks in the 2025 Object-Attribute Subscription
State guide with Mermaid CLI 12.0.0. Visually inspected three targeted charts:
the source-derived passive-recipient decision flow, the HLA-first late-interest
effects map, and the embedded regional late-discovery sequence. Their labels
are legible in the expanded local preview; the first two are portrait but
readable while scrolling, and the sequence notes fit without clipping. No
chart-source adjustment was warranted.

The passive-only focused test does not cover a separate active publisher
making a passive receiver eligible. The regional case observes discovery, the
stored value, and a later update as separate observations without asserting
callback order. The ordinary process initial-reflection edge remains an open
source/test/normative question; the RTI-owned MOM initial-values branch stays
separate. This targeted local preview is not GitHub verification or a
whole-guide/tree review. No builds, tests, CI, 2010, or Requirements Lab work.

## 2025 embedded/process late-subscription comparison — 2026-10-14

The comparison chart was visually inspected in the eight-diagram local render.
Its embedded and process lanes remain distinct, the RTI-owned MOM initial-value
branch is separate, and the ordinary-object process result is explicitly
unresolved. No diagram change was needed. This is not a licensed §5.8 review or
evidence of embedded/process parity; no test/build/CI was run.

## 2025 service-report failure sequence — 2026-10-14

Rendered all eight Mermaid diagrams in the Service Invocation Reporting guide
with Mermaid CLI 12.0.0. The repaired switch-append sequence parsed, but its
long participant and note labels crowded the right edge. Shortened those labels
and rerendered the guide. The resulting sequence is still wide but readable,
without clipping; the report-switch commit, failed file append, best-effort
exception hook, absent callback assertion, and skipped MOM enqueue remain
distinct. The writer test's last recorded run failed before its target
assertions during federation creation; it was not rerun. Local visual review is
not GitHub verification. No semantic behavior, tests, or C++ source changed.

## 2025 Joined-Federate MOM charts — 2026-10-14

Rendered all six diagrams in the 2025 Joined-Federate MOM guide with Mermaid
CLI 12.0.0. The repaired subscription/discovery sequence and resignation state
chart were individually visually inspected; both are legible in the expanded
local preview without clipping. The state chart cleanly shows the retained
private snapshot during pending removal and the no-known-survivor path. The
sequence keeps Join, per-observer discovery/reflections, the explicit value
request, and resignation removal separate. No diagram edit was warranted.

The 2025 MIM cardinality and reader's model remain distinct from Umbra's
embedded/process paths; the guide explicitly states that a 2010 model input
does not imply a 2010 RTI `HLAfederate` projection. Existing process Join/resign
evidence is source/test-bounded and is not generalized from embedded tests.
This is local targeted review, not GitHub or whole-tree verification. No
builds, tests, CI, or Requirements Lab work.

## 2025 FOM admission/composition chart review — 2026-10-14

Rendered all four diagrams in the FOM admission/composition guide with Mermaid
CLI 12.0.0. The Create and Composition revisions were too compressed in
left-to-right layouts; changing those two to top-down made the labels
readable at expanded preview width. The composition chart is very tall but
its merge, completed-model checks, FDD validation, and shared rejection path
remain navigable while scrolling. The Join chart is also tall but legible. The
FOM-edition chart clearly presents a 2010 catalog-only lane under the 2025 RTI
interface, without implying 2010 RTI parity. No behavioral claim or state
transition changed. Local GitHub verification remains pending publication;
this is not a whole-tree render. No builds, tests, CI, or Requirements Lab.

The TSO review below completes this handoff.

## 2025 TSO retraction diagram review — 2026-10-14

Mermaid CLI 12.0.0 rendered all ten diagrams in the TSO retraction guide. Five
were visually inspected locally: the portable outcome orientation,
producer-time/lookahead gate, regional FQR/TARA/NMRA sequence, ownership
timeline, and process-route sequence. Reoriented the ownership flow top-down
and shortened regional sequence labels. Fixed sequence parsing where adjacent
notes and semicolons were treated as statement delimiters; no behavioral
transition changed. The portable HLA map remains an orientation, not a full
§§8.22–8.23 decision table. Selected scenarios remain embedded 2025 evidence;
no process parity, normative equivalence, or 2010 behavior is inferred. The
timestamped-delete and per-recipient charts retain their 2026-10-09 GitHub
visual review; that does not verify these later revisions.

This follow-up re-rendered and visually inspected the per-recipient state,
mixed-recipient walkthrough, focused time-boundary trace, receiver-Resign
process route, and ownership timeline. Nine of ten charts now have local
visual review; the timestamped-delete chart is the only one without a current
local visual inspection and retains its 2026-10-09 GitHub review. The
per-recipient chart also received a local review after its current revision.
The mixed-fanout, time-boundary, and ownership examples remain selected
embedded 2025 scenarios; the receiver-Resign route remains process-source-only
with no test of a public post-Resign callback. No tests/builds/CI or
Requirements Lab work.

The following reviews complete this handoff.

## 2025 object-instance name reservation — 2026-10-14

Freshly rendered all seven Mermaid diagrams in the name-reservation guide and
visually inspected each. Shortened the reservation-lifetime state chart's
edge labels after the local preview showed excess horizontal stretch; its
failure, release, resignation-before-registration, named-registration,
global-purge, and save/restore transitions remain distinct. The owner/peer map
preserves reservation versus live-instance versus discovery boundaries. Its
two embedded tests are complementary, not a single end-to-end case. No
semantic or edition boundary changed; GitHub review of current revisions is
pending publication. No tests, builds, or CI.

## 2025 exception-reporting routes — 2026-10-14

All six Mermaid diagrams in the exception-reporting guide rendered and were
visually inspected locally. Split the long Route A sequence at the embedded
registry/process endpoint ownership boundary and abbreviated repeated lane
labels. The caller's typed service exception remains separate from
HLAreportException, HLAreportMOMexception, and HLAreportServiceInvocation.
The combined process two-report scenario remains source-derived because the
selected tests exercise one switch each, not both together. No report-order,
normative, profile-parity, or 2010 claim was added. Revised GitHub rendering
remains pending publication; no tests/builds/CI were run.

## 2025 process-endpoint request and callback review — 2026-10-14

Rendered all 11 diagrams in
`HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md` with Mermaid
CLI 12.0.0 and visually inspected each preview. Reorganized the interaction,
receive-order attribute, and TSO sequences at actual ownership/lifecycle
boundaries; the separate views distinguish sender request handling, recipient
event acquisition, callback-model dispatch, and private TSO completion ACK.
Top-down layouts and shorter labels improve the HLA/API-to-process identity
map, polling-family selector, and timestamped callback sequence. A nested
sequence-note placement was repaired for Mermaid parsing. These are
source-derived process details, not HLA-wide ordering guarantees. Public
MessageRetractionHandle and LogicalTime remain distinct from private
messageId/requestId. A throwing TSO callback consumes its task and skips the
private ACK, leaving the recipient ledger inTransit; no focused test covers
callback throw followed by receiver Resign and producer Retract, or
disconnect-to-detach timing. Historical 284-assertion focused process TSO
results are retained as narrow evidence, not a claim for those gaps or
embedded parity. No builds, tests, CI, Requirements Lab, or 2010 comparison.
GitHub review of the revised guide remains pending publication.

## 2025 known-object subscription scope-change chart review — 2026-10-14

Rendered only the revised known-object scope-change Mermaid chart from the
subscription-state guide with Mermaid CLI 12.0.0 and visually inspected it
against the surrounding text. The top-down chart is tall but readable. Its
receiver scope-advisory and new-attribute value branches are independent; the
embedded initial-value/Auto Provide route and process MOM-only path remain
separate. The ordinary current-value question remains unresolved; no normative
or profile-parity claim was added. No guide edit, build, test, or CI run.

## 2025 Attribute Value Update Request entitlement review — 2026-10-14

The guide's HLA-facing sequence and prose already separate provider
solicitation, a later update, and receiver eligibility. Bounded source review
confirms that ordinary receiver planning separately filters by subscription
and region; this does not decide the normative entitlement question. Focused
embedded examples use a subscribed requester. The process object-request
fixture installs subscription directly in the registry before the provider's
later ordinary update; the process class-request case only asserts provider
callbacks. No focused case covers an unsubscribed requester receiving that
response. The special RTI-owned MOM direct-reflection path is distinct. The
local 2025 API headers contain the §6.21 declaration/exception list but not
the clause prose; IEEE lists standard access by purchase/subscription. Linked
the listing from the guide and kept the normative question open. No diagram,
normative, process-parity, or 2010 claim was added; no builds/tests/CI.

## 2025 factory/RID boundary diagram review — 2026-10-14

Rendered only the fourth Mermaid chart in
`HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md` with Mermaid CLI 12.0.0
and visually inspected the local preview. Shortened participant display
labels and the HLAauthorizer branch text after the first preview showed
unnecessary horizontal stretch; the edit changes no branch, ordering, or
timing. The chart reads clearly and separates factory construction of a
disconnected ambassador from Connect-time environment/RID/profile gates and
the post-commit result. The cited focused test still failed during temporary
RID-file setup before assertions; it was not rerun and supplies no runtime
evidence. This remains a source-grounded flow. No other chart was rendered;
no build, test, CI, Requirements Lab, or 2010 comparison was performed.

## 2025 owner-switch and pending-advisory audit — 2026-10-14

The focused embedded owner-switch test
(`attribute_relevance_advisory_catch2.cpp`) suppresses On/Off callbacks while
the switch is false, but drains the owner callback queue before re-enabling;
the later On comes from a fresh subscription transition. It does not cover a
relevant state or pending callback spanning an off/on cycle. The selected
process callback test enables the owner switch during setup and does not
toggle it off.

Source review separates three things: the per-member switch, relevance state,
and admitted callback work. In the 2025 embedded path the setter changes the
member flag; the transition planner is called by declaration, discovery, and
region mutations, not by setting the switch. A planned callback closure
rechecks the switch, owner, and expected current state when dispatched. Thus
re-enabling alone does not synthesize a new edge, but a closure still pending
from an earlier transition can pass if the switch is back on and the expected
state still holds at dispatch. The selected test drains closures while off,
so it does not verify this case.

The process path is distinct: its client updates the callback-time atomic
gate before making the switch request; the service stores the per-member
switch without planning a relevance transition. Event admission checks
eligibility before queueing, the receive path checks again, and the callback
bridge checks the switch at dispatch. A transition while the server switch is
off is not enqueued; work admitted earlier can still pass a later check if
the switch is on and its expected state remains current. No selected process
test covers that timing. These are 2025 source observations, not normative
claims or embedded/process equivalence. No new diagram was warranted: the
existing relevance-state map plus a short profile-stage table expresses the
boundary without recasting internal queues as HLA state. No build, test, CI,
Requirements Lab, or 2010 comparison was performed.

## 2025 process class-level declaration advisory audit — 2026-10-14

The configured process endpoint has remote getter operations for the object-
and interaction-class relevance switches, but no dedicated SET operations in
its wire protocol. The public Set methods have no process-endpoint branch and
fall through to the embedded-registry path. The process MOM `HLAsetSwitches`
interaction is a different mutation route: its server handler applies the
member switch update, and the focused process test sends that interaction and
checks the getters. It does not call the public Set methods or assert any
declaration callbacks.

The process publication and subscription handlers mutate registry state and
return their service results; the current class-level planner call sites are
in the ambassador runtime, not these process handlers. The process event and
callback-dispatch path has no `DeclarationAdvisory` transport event or
Start/Stop Registration/Turn Interactions callback case in the surveyed
sources. All focused class-level transition tests found in this bounded search
are embedded. This is a source-only implementation gap, not a normative rule
and not process/embedded parity. The guide now scopes its class-level branch to
embedded 2025 and distinguishes process MOM switch readback from callback
delivery; no new chart was needed. The separate per-instance process
Turn-Updates event path remains supported by its own selected tests. No build,
test, CI, Requirements Lab, or 2010 comparison was performed.

## 2025 process `HLAsetSwitches` MOM projection audit — 2026-10-14

The MIM marks the joined-federate `HLAobjectClassRelevanceAdvisory` and
`HLAinteractionRelevanceAdvisory` MOM attributes Conditional on their switches
changing. The process `HLAsetSwitches` handler stores those switches in the
registry, and the MOM value projection encodes the same stored fields. The
process getter path and focused one-client test establish switch readback, not
Conditional MOM reflection to eligible recipients. The process handler's
post-apply conditional-MOM enqueue is currently gated only on
`exceptionReporting`; it does not enqueue the two advisory attributes on this
route. A separate explicit MOM
`requestAttributeValueUpdate` projects the current value to the requester, but
that does not show that a change-triggered Conditional reflection was emitted.

The embedded `HLAsetSwitches` path gathers every supplied switch attribute for
conditional work, so this is a source-observed process-path difference and a
gap against the MIM's stated update conditions to investigate—not a conformance
conclusion or profile-parity claim. The Federation MOM guide now diagrams and
separates storage, getter readback, current-value request, and automatic
reflection. No C++ changes, build, tests, CI, Requirements Lab, or 2010
comparison were performed.

## 2025 process explicit joined-federate MOM value request — 2026-10-14

The 2025 MIM requires the RTI to answer a `requestAttributeValueUpdate` for an
`HLAfederate` class/instance with current values via the normal instance
attribute update mechanism, regardless of Static, Periodic, or Conditional
type. The configured process adapter validates its handles and joined state;
the service validates the session/requester; and the registry requires current
membership, a known MOM object, and defined requested attributes before
projecting current values. This RTI-owned ledger route does not require an
active subscription in the current source and does not solicit a federate
`provideAttributeValueUpdate` callback. Its event is pushed or polled, then
submitted through the client callback bridge to `reflectAttributeValues`.

The focused two-federate process test uses `HLA_EVOKED`, subscribes and learns
the subject's `HLAfederate` object, explicitly requests the Conditional
`HLAreportServiceFile`, evokes the callback, and checks one reflection with
the current path. It does not exercise the source's no-active-subscription
branch, invalid request cases, other callback models, or the attribute's
automatic update condition. The Federation MOM guide now separates current
request-time projection from Conditional triggers and charts the request gates
and process event/callback bridge. All 13 MOM-guide diagrams rendered; the
three new MOM charts were visually inspected. No C++ changes, build, tests,
CI, Requirements Lab, or 2010 comparison.

Next: compare one ordinary 2025 process object-instance
`requestAttributeValueUpdate` against this RTI-owned MOM path: trace provider
selection/solicitation, the `provideAttributeValueUpdate` callback, and the
receiver's `reflectAttributeValues` delivery. Reuse the existing Attribute
Value Update Request guide and focused process test, adding a diagram only if
the comparison resolves a real reader gap. Keep the 2010 stream separate; no
builds, tests, CI, or Requirements Lab.

## 2025 process ordinary AVU versus RTI-owned joined-federate MOM — 2026-10-14

Closed the process-path comparison handoff. The instance request adapter enters
the process service, which checks the RTI-owned joined-federate MOM ledger first
([service branch](../../cpp/src/internal/federation/process_federation_service_attribute_update_requests.cpp#L13)).
For ordinary objects, the registry requires requester-known state and groups
remotely owned requested attributes by current provider
([planner](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L278));
the process callback bridge delivers `provideAttributeValueUpdate` and later
`reflectAttributeValues`
([Provide](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L370),
[reflection](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L261)).
The RTI-owned branch projects current member-MOM values and creates a direct
reflection event without a provider solicitation.

The focused ordinary process case is configured-process evidence for the
request-tagged Provide callback, a separate update tag, and the later reflection
under both `HLA_EVOKED` and `HLA_IMMEDIATE`
([case](../../cpp/tests/ieee1516_2025_connection_request_attribute_value_update_catch2.cpp#L459)).
The MOM process case observes the current Conditional report-file value under
`HLA_EVOKED` only
([case](../../cpp/tests/ieee1516_2025_connection_process_joined_federate_mom_report_file_catch2.cpp#L437)).
Neither test establishes all eligibility cases, a no-subscription ordinary
response, automatic Conditional MOM-change delivery, or universal callback
ordering. The AVU guide now contains a comparative flowchart; Mermaid CLI
12.0.0 rendered all six guide charts and the new chart was visually reviewed.
The fence census recorded for this 2026-10-14 review entry was 292 diagrams across 34 diagram-bearing files
(164 flowcharts, 105 sequences, and 23 state diagrams).
No C++ changes, build, test reruns, CI, Requirements Lab, or 2010 comparison.

Next: audit callback observation timing in the 2025 process AVU path against
the callback/service-ordering guide. Keep public callback-model behavior
separate from Umbra dispatch details, avoid a duplicate diagram unless a real
reader gap is found, and preserve the 2010/2025 boundary.

## 2025 process AVU callback-observation evidence boundary — 2026-10-14

The public process test observes the ordinary AVU path under both callback
models, but its checkpoints do not establish service-return timing. It invokes
the request, then uses `evokeCallback` in the HLA_EVOKED section or a later
`getObjectClassHandle` step labeled an immediate callback flush before
asserting Provide
([request and provider checkpoint](../../cpp/tests/ieee1516_2025_connection_request_attribute_value_update_catch2.cpp#L887),
[callback assertion and model sections](../../cpp/tests/ieee1516_2025_connection_request_attribute_value_update_catch2.cpp#L896)).
After the provider's separate update, it performs the analogous evoke/lookup
step before checking reflection
([update and receiver checkpoint](../../cpp/tests/ieee1516_2025_connection_request_attribute_value_update_catch2.cpp#L910)).
There is no callback-state sample at either inducing call's return boundary.

The generic callback guide distinguishes the model-level reading: HLA_EVOKED
makes Evoke Callback / Evoke Multiple Callbacks the application dispatch
point; HLA_IMMEDIATE permits entry while the inducing service is executing.
This process integration test only proves observation at its later
checkpoints; it proves neither a before-return AVU guarantee nor a total order
between the requester's service response and the provider's callback. Updated
the AVU guide prose with these limits and cross-links. No new diagram, source
change, test rerun, build, CI, 2010 comparison, or Requirements Lab work.

Next: review the 2025 Object and Interaction Information Flow guide against
the focused Object Attribute Subscription State guide at the new-object
discovery/first-reflection boundary. Use bounded pinned API, source, and test
evidence; separate normative callback sequence from implementation commit
points and application observation. Revise a diagram only if the audit finds
a real reader gap. Keep 2010 separate and do not infer profile parity.

## 2025 discovery and first-reflection boundary review — 2026-10-14

The broad object-flow guide and focused subscription-state guide already
explain the newly-known versus already-known receiver states and the separate
process MOM path. The pinned API header gives callback declarations and clause
labels, not §§5.8/6.9/6.11 normative prose. Selected embedded source plans
ordinary initial values after discovery callback return; its focused test
asserts discovery and reflection/value through separate collections. The
process receive selector places discovery ahead of the ordinary attribute
event branch after earlier-priority work, but its public update test is a
distinct later Update Attribute Values call and does not assert order. Process
discovery's follow-up initial-value event is constructed only for RTI-owned
MOM objects. Updated the broad guide with a brief embedded/process contrast;
existing focused state detail made another chart duplicative. No diagram was
changed or rendered, and no source, test, build, CI, 2010, or Requirements Lab
work was performed.

Next: audit HLA_EVOKED versus HLA_IMMEDIATE application observation timing for
the embedded discovery/initial-reflection path. Keep API callback-model rules,
Umbra dispatch points, and focused test assertions separate; no callback-order
claim or process/2010 parity without direct evidence.

## 2025 embedded discovery callback-model observation — 2026-10-14

The focused late-subscription case is HLA_EVOKED-only: it returns from
`subscribeObjectClassAttributes`, then drains callbacks. Its separate discovery
and reflection vectors establish occurrence and selected value, not relative
order or before-return timing; there is no HLA_IMMEDIATE variant. The embedded
subscription path submits discovery work after declaration commit. Its single
callback task invokes discovery and then plans/invokes ordinary initial
reflections after discovery callback return. The dispatcher can run an
immediate task on the submitting thread, but a submission behind another
thread's active immediate drainer is queued. This selected implementation path
does not support a universal before-return claim. The callback guide already
contains the generic HLA_EVOKED/HLA_IMMEDIATE model diagram; no chart was added.
Added a prose evidence note to the broad object guide. No source, test, build,
CI, 2010, or Requirements Lab changes.

Next: inspect callback-visible known-instance state during embedded 2025
`discoverObjectInstance`, including whether public object-support calls can be
made reentrantly, and identify the focused tests (if any) that assert the
boundary. Keep standard obligations separate from registry implementation
timing; add a diagram only if the reader model needs one.

## 2025 embedded discovery callback-visible support state — 2026-10-14

The embedded source commits receiver-known state and releases the
federation-state lock before entering `discoverObjectInstance`; its source
comment says matching support services are safe from inside that callback.
`getKnownObjectClassHandle` reads the committed state. The pinned API contains
the public signature and clause label, not normative reentrancy text. The
selected focused ownership-query test calls the lookup only after discovery
has been evoked and recorded; no reviewed focused case exercises it inside the
callback. Added that distinction to the broad object guide as source-supported
implementation behavior, not test-verified or normative. Existing diagrams
already show the state-commit boundary, so no new chart or render was needed.
No source, test, build, CI, 2010, or Requirements Lab changes.

Next: compare the receiver-facing discovery/initial-value path with owner
`Turn Updates On/Off` advice and the provider-facing Auto Provide solicitation
after a subscription makes an existing object newly discoverable. Reuse the
focused guides and tests; separate actors and effects, and add a chart only if
a real reader gap remains. Do not infer universal callback ordering or
process/2010 parity.

## 2025 late-subscription effects and state/event boundary — 2026-10-14

The focused subscription, relevance-advisory, and Auto Provide guides already
separate the receiver's discovery/current values, the current owner's
value-free relevance advisory, and the provider's value-free Auto Provide
request. The DSE guide now explicitly contrasts that newly known object state
with a single prior interaction send retained as an embedded delivery-time
candidate. Focused tests establish values/callback eligibility in those
selected paths, but not a universal callback order or normative §§5.8/8.1.8
interpretation. Existing diagrams cover the transitions; no chart was added.

Next: audit adding an attribute subscription for an already-known object for
an ordinary current-value reflection without a new update. Keep embedded and
process evidence separate, including the process MOM exception and the DSE
candidate path. Preserve the licensed-clause limit; no test reruns, builds,
CI, 2010 comparison, or Requirements Lab work.

## 2025 already-known receiver adding an attribute — 2026-10-14

The selected embedded registry mutation yields scope/relevance effects, the
known-object discovery planner skips this receiver, and the ordinary initial
reflection planner runs only after discovery. The process discovery enqueue
path has a separate initial-value event for RTI-owned MOM objects, not an
ordinary object snapshot. The nearest additive/mode test sends a new publisher
update before checking reflection; the selected process discovery case
subscribes before registration. Neither isolates current-value reflection
caused only by adding an attribute to an already-known object. The subscription
guide now explains this test boundary. This remains source-observed, not a
normative §5.8 conclusion; no chart was added.

Next: bounded 2010-only survey of DataElement/composite encode/decode concepts,
the reference profile implementation, and the dedicated smoke sources. Keep
that behavior separate from 2025 and draw a flow only if the guide has a real
reader gap.

## 2010 composite decoding — 2026-10-14

The separate 2010 DataElement guide already contained scalar byte-order and
composite-encoding flows. A third diagram now explains source-observed decode
placement: fixed arrays use configured slots; variable arrays reject negative
signed counts and treat zero as the count-prefix-only case; records validate
field padding; variants select by decoded discriminant, with an unmapped value
consuming only the discriminant. Full-buffer decoding rejects leftovers, and
the inspected decoders reject truncation/invalid padding. The focused smoke
source covers only selected round trips and nonzero fixed-record padding—not
all branches shown. The local review rendered all three guide diagrams and
visually inspected the new four-path decode chart after tightening its layout.
This is implementation-profile evidence, not a normative algorithm or
2025-parity claim. No test/build/CI was run and the Requirements Lab was not
changed.

Next: bounded 2010-only review of logical-time value representation, ordering,
and arithmetic; distinguish value operations from time-management services.

## 2010 logical-time value/service boundary — 2026-10-14

Replaced the generic 2010 timing-support sketch with a more informative,
source-bounded conceptual chart. The first layer now shows factory-provided
matching `LogicalTime`/interval values, the initial/final and zero/epsilon
operations, comparison, time/interval arithmetic, and serialization. A
separate layer shows the ReferenceRTI implementation limit: execution creation
does not wire the supplied time-model name to a factory, and ambassador
time-management/timestamped services reach `unavailable()`. The pinned API
defines a federation-supplied representation/factory; Umbra's integer64 and
float64 implementations are examples, not an exhaustive HLA model.

The final chart parsed and rendered locally, then received visual review. The
vertical layout keeps labels readable and makes the value layer primary. Its
selected smoke evidence does not cover ambassador calls or every source-side
type/range failure. No new diagram was added, no tests/build/CI were run, and
no 2025 or Requirements Lab behavior was changed.

Next: audit the existing 2025 federation/federate lifecycle guide for an
HLA-first explanation of membership versus execution lifetime across Join,
Resign, connection loss, and Destroy Federation Execution. Preserve
embedded/process and 2010 boundaries; add a diagram only if the source/API/test
survey establishes a genuine reader gap.

## 2025 lifecycle map and active-save sequence visual review — 2026-10-14

The HLA-first survey confirmed the current guide already separates three
lifetimes and the distinct Create/Join/Destroy, Resign/Disconnect, and forced
connection-loss paths. No new lifecycle map was justified. All 15 current
guide diagrams rendered locally. The active-save resignation sequence was
visually dense, so it was simplified to the callback re-entry rejection,
post-callback successful Resign, survivor-only save failure, and later Save
retry; this preserves the selected embedded `HLA_EVOKED` test boundary. A
first render caught semicolons in labels being parsed as Mermaid statement
separators; after correction, the sequence rendered and was visually
inspected. The separately reviewed lost-regulator cutoff flow remains legible
but tall. Exact clause 4.4 wording remains unverified without the full
standard. The source/test claims are 2025-specific; no process or 2010 parity
is inferred. No test/build/CI or Requirements Lab change was made.

## 2025 FOM admission/composition failure audit — 2026-10-14

The existing four diagrams already explain per-module validation, ordered
composition, complete-model references, FDD validation, and the separate
Create/Join commit boundaries; no duplicate chart was needed. Added an embedded
exception-translation map and checked each Create/Join overload against its
adapter path. Recorded differences in declared versus translated failures for
vector Create (default-MIM errors), Create with MIM (unavailable logical-time
factory), single-module Create (a listed but unreachable standard-MIM
designator status), and additional-module Join (retained-MIM revalidation).
These are implementation/API observations; public adapter cases do not cover
the mismatches. The pinned headers are byte-identical to the canonical IEEE
API archive per the local manifest; the full prose standard is not vendored,
so no additional normative rule was inferred. No code tests or builds were run.

Next: audit the open direct-requester entitlement question in the 2025 Request
Attribute Value Update guide against its pinned API, bounded source, and
existing tests. Keep normative status open unless exact clause text is
available; preserve the 2010/2025 boundary.

## 2025 DataElement byte-order boundary audit — 2026-10-14

The current guide already separates concrete BE/LE types, fixed-format
encodings, child-level composite bytes, factory-defined time, and private
process-frame endianness. Rechecked the shared explicit-order primitive,
variable-array `HLAinteger32BE` count, and focused vectors/malformed-input
cases. No reader gap or duplicate diagram was found; 2010 remains separate.

Next: audit the 2025 process callback queue/session boundary after Resign in the
Callback and Service Ordering guide. Compare source reachability with focused
post-Resign tests and keep standard permission distinct from implementation
behavior.

## 2025 switch-observation synthesis and TSO final local review — 2026-10-14

The existing Federation MOM guide already documents the process `HLAsetSwitches`
chain and its source-observed reflection gap. Added a cross-cutting switch-map
entry that distinguishes embedded direct RTIambassador setters, which plan
Conditional MOM work, from the process Federate MOM path, which updates member
state/getter readback without automatically reflecting these two relevance
attributes. A current-value request is a separate observation route. The
process test verifies getter values, not observer reflection; this remains a
profile-bounded source/test observation, not a conformance or 2010 claim.

Rendered all ten TSO guide charts from the current Markdown with Mermaid CLI
12.0.0 and visually inspected the previously unreviewed timestamped-delete
chart. Its two callback-entry branches and final convergence are unclipped and
legible. All ten charts now have local render and visual-review evidence;
GitHub rendering of changed versions remains pending publication. No tests,
builds, CI, or Requirements Lab work.

## 2025 DDM guide full render and visual review — 2026-10-14

Rendered and inspected all 20 current charts in
`HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md`. Fixed two inline-code
labels that blocked parsing, reoriented the Region-template lifecycle state
chart top-to-bottom for readability, and compacted the process multi-recipient
sequence from seven lanes to four. Retained its separate strict/relaxed DDM
paths, independent recipient eligibility, and distinct callback models; tags
now match the inspected focused test (A1/A2). Client/server framing is omitted,
and no cross-recipient callback-order claim is made. Local render and visual
review are complete; GitHub review awaits publication. No tests/build/CI or
2010 behavior work.

## 2025 Attribute Ownership guide full render and visual review — 2026-10-14

Mermaid CLI 12.0.0 rendered all 18 diagrams from the current ownership guide.
Visual review caught a parser-breaking semicolon in a sequence-message label;
replacing it with “and” restored parsing. The regular-acquisition,
If-Available, negotiated-divestiture, process-cancellation, and If-Wanted
sequences had long labels crowding the lifelines. Their labels now use concise
HLA action/result wording, with exact operation names retained in the
surrounding prose and references. Rerendered all 18 charts after the edits and
visually rechecked every revised diagram. The tall decision flows remain
readable while scrolling; no chart is clipped. GitHub review awaits
publication. No C++ tests/build/CI, Requirements Lab, or 2010 work.

## 2025 FOM admission/composition guide full render and visual review — 2026-10-14

Rendered all four diagrams in
`HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md` using Mermaid
CLI 12.0.0; every chart rendered and was individually inspected. The three
2025 stages—Create preparation, ordered composition/FDD validation, and
additional-module Join commit/rollback—remain visually separate. The fourth
diagram labels the 2010 branch as catalog-only and does not imply a 2010 RTI
stream or mixed-edition composition. No layout/parser defect warranted a
source change. This is local visual evidence, not renewed source/test or
conformance verification. GitHub rendering remains pending publication.

## 2025 Exception-Reporting guide full render and visual review — 2026-10-14

Rendered and visually reviewed all six current diagrams with Mermaid CLI
12.0.0. Compact labels and participant names in the embedded and process
routes, `noexcept` callback-exception boundary, and combined process-report
sequence where text had crowded the lifelines. Retained distinct routing and
reporting roles, exact report protocol names, callback timing caveats, and the
source-only status of the combined two-report path. The final six-chart render
succeeded; no claim about report callback arrival order is added. GitHub review
awaits publication. No build/tests/CI, Requirements Lab, or 2010 behavior work.

## 2025 Service Invocation Reporting guide full render and visual review — 2026-10-14

Rendered all eight diagrams with Mermaid CLI 12.0.0 and visually inspected
each. Shortened transitions in the service-reporting interlock and the
final-resignation edge, removed the redundant report-write self-loop from the per-membership file-lifecycle state
chart, and compacted the post-commit ConnectionLost trace's self-messages so
they stay within the chart. The final eight-chart render succeeded, and the
revised charts were re-rendered and visually checked. Adjacent prose retains
the exact interlock exception names, enabled file-sink append condition, stable
file identity, registry commit and post-commit cleanup split, and deferred-
error caveats. This is a local render and layout review only; no
implementation or test behavior was revalidated.
GitHub rendering remains pending publication. No builds/tests/CI, Requirements
Lab, or 2010 behavior work.

## 2025 Callback and Service Ordering guide full render and visual review — 2026-10-14

Rendered and visually reviewed all 12 current diagrams with Mermaid CLI
12.0.0. Compacted the process registration/achievement sequence's participant
and message labels, which were too small in the full-chart preview. The edited
chart remains explicit about separate registration and final-achievement
flows, correlated private responses versus recipient callbacks, pushed versus
polled delivery, and callback/report order. The final 12-chart render passed;
other diagrams had no layout defect warranting a source edit. This checks
render/layout only, not source semantics or test behavior. GitHub rendering
remains pending publication. No builds/tests/CI, Requirements Lab, or 2010
behavior work.

## 2025 object discovery and first-reflection boundary audit — 2026-10-14

Reviewed the embedded discovery callback, process discovery enqueue and pull
selector, the pinned FederateAmbassador declarations, and the focused
late-subscription/public process tests at bounded source ranges. The source
supports separate commit points and a pull-selector priority; the selected
tests establish callback/value occurrence but do not assert discovery-versus-
reflection order. The inspected process route builds ordinary initial-value
events only for RTI-owned MOM objects. No full normative §5.8/§6.9/§6.11 text
is present in the vendored API checkout, so the guide retains that question as
open rather than a conformance conclusion. Reused the existing comparison
diagram in the subscription-state guide, added a direct crosslink, and limited
the process-order wording to the pull selector. No new diagram or test run.

## 2025 Auto Provide neighboring-path audit — 2026-10-14

The existing Auto Provide guide already compares automatic discovery-triggered
owner solicitation, explicit caller-requested solicitation, current-value
reflection, and direct reflection for the joined-federate MOM exception. Its
process evidence is appropriately cautious: explicit process request routing
and shared switch mutation do not demonstrate process Auto Provide. Added a
crosslink from the object-information guide's first-reflection discussion to
the comparison table instead of duplicating its chart. No rendering or tests
were needed; this was a source/document navigation audit only.

## 2025 process object-instance request transport map audit — 2026-10-14

Verified the focused process test against the existing object-instance request
flow: the caller's tag reaches the provider callback, while a later provider
update supplies a separate tag observed at reflection. Added source pointers
for the process-only adapter/client/codec/service/callback-bridge chain and
marked it as implementation mapping, not HLA protocol. Reused the existing
diagram; no render or test rerun was needed.

## 2025 FOM module admission/composition post-Join chart — 2026-10-14

Added a fifth chart to
[`HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md`](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md),
separating atomic catalog/membership commit, pre-existing live object and
subscription state, later service eligibility, and the distinct Federation
MOM current-FDD reflection. The source/test review used
`cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp`,
`cpp/tests/process_federation_additional_fom_catch2.cpp`, and the focused
embedded lookup and `HLAcurrentFDD` cases linked from the guide. It found no
focused configured-process post-Join interaction-delivery test; source-path
observations are labeled as such and are not normative claims.

Visual review is pending. Mermaid CLI 12.0.0 was available, but Chrome could
not launch in the sandbox, so the new diagram was not rendered or visually
checked. The four earlier charts retain their prior render/review status.

## 2025 configured-process post-Join interaction evidence audit — 2026-10-14

Rechecked the configured public Join case and process service against the FOM
guide's post-Join interaction gap. The test's extension module is object-only;
its public assertions cover new object-class handle/name lookup, not an
interaction send or Receive Interaction. The separate inherited-receipt test
starts with its hierarchy already in the catalog at Create. Source inspection
shows the process send later uses the current catalog and stored per-federate
subscriptions, but no focused test establishes delivery for a descendant
added by later Join. The existing FOM prose already marks this as a source
inference, not normative or parity evidence; no diagram was changed. This
audit did not rerender the fifth chart, whose visual review remains pending.

## 2025 FOM fifth-chart renderer follow-up — 2026-10-14

Retried Mermaid CLI 12.0.0 with the installed Microsoft Edge executable after
the Chrome launch failure. Puppeteer failed before Mermaid parsing/rendering,
reporting an already-running browser profile and then a Windows credential-
launch error. No process remained visible afterward. The temporary Edge
configuration was removed; no PNG was produced. At that time the chart remained
unparsed and visually unreviewed locally. No external renderer or upload was
used.

## 2025 FOM post-Join chart local render and visual review — 2026-10-14

The chart added as the fifth historical FOM diagram is the fourth Mermaid
fence in current source order. After sandboxed Chrome/Edge launches failed, an
approved one-off local headless-browser render outside the sandbox succeeded
with Mermaid CLI 12.0.0. The rendered chart cleanly separates the atomic
catalog/membership commit, pre-existing live-object and subscription state,
later service gates, and the distinct `HLAcurrentFDD` reflection. Its tall
top-down layout is legible while scrolling; no label, branch, or layout edit
was needed. The four pre-existing FOM charts retain their prior local
render/visual-review evidence, so all five current guide diagrams are now
covered locally. GitHub review remains pending publication; no normative Join
callback conclusion is inferred.

The initial source-order fence selection also rendered the existing
2025-API/2010-model compatibility chart. It visibly keeps the selected FOM
edition separate from the RTI interface edition and makes no 2010 RTI parity
claim. This additional preview did not change that chart or its evidence.

## 2025 process callback-exception attribution sequence — 2026-10-14

Compared the process endpoint callback prose with the bounded client, stream
reader, background dispatch loop, bridge, and existing callback tests. The
stream-reader buffers only; an `HLA_IMMEDIATE` exception from a request-thread
drain escapes after that recipient request matches its response, while an
exception from background dispatch is saved once and rethrown before the next
process-client request. `HLA_EVOKED` runs the user callback later on the
recipient's Evoke stack. This corrects the earlier “receive pump” shorthand
and keeps the producer's separate Send Interaction stack out of the recipient
failure route.

Added a source-derived sequence in the Callback and Service Ordering guide and
a reciprocal link in the process endpoint guide. The existing process Evoke,
multi-recipient, and callback-bridge tests cover successful dispatch but do not
throw from user callback code. No build or test was run. The new Mermaid
sequence has not been rendered or visually reviewed locally; the previous
whole-guide render evidence covers only the 12 diagrams that existed at that
time. Outer public-adapter exception/report translation is not established and
is the next handoff. No 2010 behavior or normative exception rule is inferred.

## 2025 process callback exception at the public wrapper — 2026-10-14

Traced the request-drain and saved-next-request callback exceptions through
the process `getInteractionClassHandle` adapter. A callback-thrown
`FederateInternalError` passes the narrower ProcessFederationClientError
translation and reaches the outer API-`Exception` catch, which attempts the
noexcept `HLAreportException` hook followed by a failed service-invocation
report. If that report call completes, the original is rethrown; a process
report failure translated to `RTIinternalError` can replace it. Arbitrary
non-RTI C++ exceptions bypass this typed catch. This is bounded to one 2025
request-backed public wrapper, not a universal service or normative rule.

Added a wrapper/report flowchart and linked this example from Exception
Reporting. Existing focused process cases do not inject user callback throws
or exercise the combined report-failure sequence. No tests or builds were run.
Both process callback-exception diagrams remained pending local Mermaid
rendering and visual review; the prior full-guide render covered the twelve
diagrams that predated them. Keep 2010 and embedded behavior separate.

## 2025 DataElement coverage and process callback-diagram visual review — 2026-10-14

The 2025 DataElement guide already distinguishes public value encoding from
private process framing and states where the byte order comes from: a selected
BE/LE scalar type, a fixed element rule, a child element inside a composite, or
the selected logical-time factory. Pinned API declarations, representative
basic codecs, fixed-record and variable-array source, and focused test vectors
support those bounded claims. No further diagram was needed, and the 2010
companion was not merged or used to infer parity.

The three process callback-exception charts were rendered with Mermaid CLI
12.0.0 and visually reviewed locally: request-drain sequence, background-
dispatch sequence, and the one-wrapper reporting flowchart. The original
combined sequence was split because the two delivery routes and callback
models were too crowded in one image. A semicolon in an unquoted sequence
message caused a parser failure and was replaced with plain wording; clipped
labels in the two charts were shortened. The revised layouts are readable at
preview width/while scrolling. This is targeted visual review, not whole-tree
coverage; GitHub rendering remains pending publication. No builds, tests, CI,
Requirements Lab, or 2010 behavior work.

## 2025 process directed-TSO revalidation chart — 2026-10-14

Added one process-profile flowchart to the 2025 directed-interaction guide.
It distinguishes send-time message/recipient retention from the delivery-time
recheck of current publication, target, receiver knowledge, and selector, then
shows recipient suppression versus current projection, callback gating, and
completion acknowledgement. Legal Retract appears only as the separate
pre-dispatch cancellation path; the full recipient-state machine remains in
the TSO-retraction companion.

Targeted Mermaid CLI rendering was attempted through the existing local
renderer, but Puppeteer could not launch its browser process (exit code
2147483651). No alternate browser-launch route was tried. The chart therefore
has no syntax-acceptance or visual-review verdict yet; inspect it after the
permitted preview path is available. Source and test links were checked
individually. The process callback/grant test does not cover post-enqueue
unpublish, target departure, or queued-copy Retract; those are explicitly
source-only gaps, not a charted delivery guarantee. No 2010 equivalence is
asserted.

## 2025 process directed-TSO departure branch — 2026-10-14

Expanded the chart and guide to distinguish a joined producer's live
publication/target/receiver gates from the exact pending-message exception
after producer departure. The guide now distinguishes voluntary-Resign
`producerResigned` from forced-Connection-Lost
`deliveryRequiredAfterConnectionLoss`, which the source sets only through a
comparable captured regulator time. The departed-source path still requires a
valid target and a current receiver route, while skipping source-only gates for
that accepted message/recipient pair.

The directed Connection Lost cutoff, late TAR with automatic object cleanup,
and selector-change/unsubscribe cases inspected here are embedded-transport
tests. They do not prove process-endpoint behavior. The shared process route is
supported by source, but no focused process test combining pending directed
TSO with producer resignation or Connection Lost was found; the process
directed-retraction probe retracts before its final Resign. The connection-loss
grant exception and cutoff are linked to lifecycle/time-management guides; no
normative §4.4 wording is asserted.

The existing Mermaid chart was edited but not parsed or rendered in this pass.
A fresh CLI attempt on the revised branch also failed before parsing because
Puppeteer could not launch (exit code 2147483651); syntax and screenshot review
remain pending. No alternate launch route was used.
No chart-count change, C++ change, test run, build, CI, Requirements Lab, or
2010 edit occurred.

## 2025 attribute ownership: unpublication and update authority — 2026-10-14

The guide already models publication and instance ownership as distinct, but
did not connect that distinction to the current embedded unpublish transition.
The whole-class adapter routes unpublication through the attribute-set
declaration update; the registry removes this federate's owner entries for
matching owned attributes and clears their associated update metadata
([adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_publication.cpp#L149),
[registry](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L170)).
The focused [receive-order update case](../../cpp/tests/receive_order_attribute_update_catch2.cpp#L249)
accepts a value update before `unpublishObjectClass`; afterward, its next
update fails with `AttributeNotOwned` ([rejection](../../cpp/tests/receive_order_attribute_update_catch2.cpp#L332)).
The guide now states this embedded-profile consequence and explicitly avoids
generalizing it into a complete normative §5.3 rule or a 2010 claim.

No Mermaid source changed, so this reader note does not alter existing chart
render evidence. No C++ files, builds, test runs, CI, or Requirements Lab
content changed.

## 2025 Request Attribute Value Update: §6.21 direct-response entitlement — 2026-10-14

Rechecked the existing guide's open question against the provider planner,
ordinary receiver planner, and the focused role-overlap cases. The embedded
case subscribes the requester through the public API before the request and
later observes a separate provider reflection. The process object-request
case directly seeds subscription state in its fixture before requesting and
observes a reflection, so it does not cover the public process subscription
route; the class-request case does not assert a later reflection. No inspected
case establishes whether a requester without an independent eligible
subscription is entitled to receive a provider's later update.

The official [IEEE 1516.1-2025 product listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers purchase and subscription access but does not expose §6.21 clause text.
The repository has the pinned API signature/exception list, not the normative
prose. The guide already preserves this limit and appropriately describes the
source behavior without claiming a portable answer; no text or diagram change
was warranted. Leave the normative question open and continue another bounded
HLA-first review rather than treating implementation evidence as the rule.

## 2025 Auto Provide: process discovery boundary — 2026-10-14

The bounded source trace found that process discovery commits the recipient's
known-object state, emits a discovery event, and delivers the public
`discoverObjectInstance` callback. Ordinary discoveries can plan initial
relevance advisories; the separate RTI-owned MOM route can plan initial values.
The scanned call sites show `planAutoProvideForDiscovery` only in the embedded
callback route. The process `HLAsetSwitches` handler and its focused test verify
the shared switch mutation/state, but the test does not discover an object or
observe `provideAttributeValueUpdate`; no focused process Auto Provide test was
found. The guide now states this as an implementation evidence boundary, not a
portable-standard conclusion. Existing Auto Provide charts were not edited or
re-rendered. No C++, build, test, CI, Requirements Lab, or 2010 content changed.

## 2025 Federation Synchronization: process result evidence — 2026-10-14

The synchronization guide already covers the HLA-level roles and the embedded
participant, announcement, achievement, completion, late-join, departure, and
save/restore transitions. The focused process endpoint cases use a receiver-only
explicit set and separately observe registration success, duplicate-label
registration-failed callback, recipient announcement/tag, a one-member
`failedToSyncSet` after the receiver reports `false`, and the out-of-set registrant's
`SynchronizationPointLabelNotAnnounced` service exception. One case uses
HLA_EVOKED/polling; its counterpart uses HLA_IMMEDIATE/pushed events. Because
the registrant is outside the set, neither case verifies same-recipient
registration/announcement order; neither tests omitted-set late-join expansion
or establishes broad process parity. Clarified the guide's process evidence
rows; no diagram was added or edited for this increment. No normative rule,
2010 inference, build, test rerun, CI, or Requirements Lab change.

## 2025 Delay Subscription Evaluation: process attribute path — 2026-10-14

Added a chart for the process-profile attribute DSE path after a bounded source
comparison. The receive-order service re-resolves a route-only candidate before
event construction but passes no sent-region handles or snapshots; queued TSO
attribute dispatch re-resolves with both. The process callback bridge reflects
the projected payload without another DSE projection check. No focused process
DSE endpoint test was found, so this remains implementation-source evidence,
not an end-to-end result, normative conclusion, or confirmed defect. The new
chart has no syntax/render or visual verdict because Mermaid CLI's browser
failed to launch (exit code 2147483651). No C++, build, tests, CI, Requirements
Lab, or 2010 content changed.

## 2025 additional-module Join: later object registration under a new descendant — 2026-10-15

The FOM, subscription, and object-information guides already covered Join-time
catalog replacement, no immediate ordinary discovery for existing objects,
and ancestor-based discovery selection separately. Added the missing reader
bridge for a later registration under a Join-added descendant while an earlier
ancestor subscription remains. The FOM guide's existing flowchart now shows
that registration as a separate event, the current-catalog ancestor walk,
receiver-known-class selection, and the separate later value-projection gates.

The inference is bounded to source: the Join commit reconciles handle
directories without rewriting the per-federate declaration map; embedded
registration invokes discovery planning; the resolver walks from registered
class toward ancestors; subsequent value planning starts with the saved
receiver-known class. The focused additional-FOM lookup test and the
Create-time child/base-subscription test cover separate halves only. The
configured-process Join case checks lookup, not post-Join discovery or value
delivery. No post-Join combined test, process parity, 2010 parity, or normative
callback claim is recorded.

The existing FOM chart was modified after its 2026-10-14 render/visual review,
so that earlier review does not validate the current chart. Mermaid CLI 12.0.0
failed to launch Puppeteer's browser before parsing (exit code 2147483651); the
current chart has no syntax/render or visual verdict. No C++, build, test, CI,
Requirements Lab, or 2010 content changed.

## 2025 additional-module Join: interaction subscriptions and later sends — 2026-10-15

The bounded audit verified that the FOM guide already traces the source-only
post-Join interaction inference: the registry reconciles the interaction-class
and parameter directories while retaining the per-federate declaration map; a
later send resolves against the current catalog, walks the sent class toward
its ancestors, chooses the closest eligible receiver subscription, and
projects parameters visible through that selected class. The process send
service calls the shared registry plan, but no focused process test combines
additional-module Join, a retained ancestor subscription, and a descendant
send.

The existing 2025 object/interaction guide already diagrams the ordinary
send path, including the separate federation-wide active-relevance gate,
receiver-specific match, callback-class selection, and parameter projection.
The FOM guide now links directly to those flows instead of duplicating them.
Evidence limits remain explicit: the embedded hierarchy-receipt case uses a
Create-time class tree and an empty parameter map; a separate registry case
checks parameter filtering for distinct receivers; configured-process
additional-FOM coverage checks object-class lookup only. No post-Join combined
delivery result, process parity, 2010 parity, or normative callback duty is
claimed. No Mermaid source changed, so chart render evidence is unchanged. No
C++, build, test, CI, Requirements Lab, or 2010 content changed.

## 2025 additional-module Join: object dimensions and retained regions — 2026-10-15

The existing DDM guide already diagrams the source-derived same-class
dimension-reference extension and the unchanged dimension set of an existing
Region. The bounded audit found that a reader could mistake a later regional
context check for delivery. Added a bridge explaining that later regional
object-subscription and update-region-association calls validate against
current class availability, store their own declaration/association state,
and still require a separate later source/receiver overlap evaluation. Linked
the Join trace to the existing regional-discovery and overlap/value-routing
diagrams rather than adding another chart.

The combined behavior is source-only. The additional-module dimension test
adds a separate class/dimension and observes handle lookup; the three-
dimensional regional object test covers overlap without Join. No focused test
extends an existing class's dimension list, retains its old Regions, adds new
regional state, and observes a later update. This is not normative or
process/2010 parity evidence. No Mermaid source changed, so chart render
evidence is unchanged. No C++, build, test, CI, Requirements Lab, or 2010
content changed.

## 2025 additional-module Join: regional interaction declarations — 2026-10-15

The bounded embedded-source audit connected three existing pieces: Join
reconciles the interaction-class, parameter, and dimension handle directories
with the replacement catalog while leaving per-federate interaction
declarations in their separate state map; regional declarations stay keyed to
the exact class handle; a later regional send resolves against the current
catalog, checks source-region dimension context, and walks from the sent class
toward parents while testing region overlap. Added this bridge to the DDM
guide and linked it from the FOM guide. The existing ancestry chart already
shows the send-time decision, so no chart was added.

Tests cover only separate slices: the additional-module lookup case sees a
new interaction class/dimension, while the regional receive-order case uses
the same class for subscription and send without Join. No focused case
combines Join, a retained parent-class regional subscription, and a later
descendant send. This is not normative, process-parity, or 2010-parity
evidence. No Mermaid source changed; no C++, build, test, CI, Requirements
Lab, or 2010 work was done.

## 2025 Delay Subscription Evaluation: process regional recheck correction — 2026-10-15

A bounded comparison of the DSE guide with the current process attribute
update handler found its receive-order regional limitation had gone stale.
For an unprojected route-only candidate in the pre-event branch, the lookup
passes sent attribute handles and supplied sent-region handles/snapshots. Pull
delivery and regional push instead carry the context with the event and
re-resolve at the receiving service boundary. Updated the prose and process
attribute chart to show both paths. TSO attribute dispatch remains a separate
later recheck. The eight named focused DSE files are embedded tests. This
initial filename-focused scan missed a separate configured-process
regional-attribute test; a 2026-10-16 test-tag audit corrected that inventory
in the later addendum below. Nonregional receive-order push and process TSO
remain without focused DSE coverage. No normative, process-parity, or
2010-parity claim is made. The edited chart needs local render and visual
review; Mermaid inventory count is unchanged.
No C++, build, test, CI, Requirements Lab, or 2010 content changed.

## 2025 DataElement encoding: ordinary versus extendable variants — 2026-10-15

A bounded source/test audit found that the guide's single ordinary-variant
flow did not explain why the 2025 `HLAextendableVariantRecord` has a distinct
unknown-discriminant path. Expanded the existing chart and prose to contrast
ordinary discriminant-only handling with the extendable record's four-octet
length-field boundary, signed big-endian payload length, fixed eight-octet
alternative boundary, and cursor skip of an unknown length-delimited payload.
The local encoder emits zero length for an unmapped current discriminant; the
decoder does not retain that unknown payload for `getVariant` or re-encoding.
The focused test checks mapped values, two alignment stages, malformed length
and padding, truncation, trailing data, and unknown-payload skipping; it does
not test a negative length or a parent continuing after the skip. This remains
2025 source/test evidence and makes no 2010 or normative parity claim. The
existing variant chart was revised, not added, so the Mermaid count is
unchanged. Mermaid CLI 12.0.0 is installed, but rendering was attempted first
with its bundled browser and then with the installed Chrome; both Puppeteer
launches failed before Mermaid parsed or rendered the chart. Fresh syntax,
render, and visual review remain pending. No C++, build, test, CI, Requirements
Lab, or 2010 content changed.

## 2025 Names and Handle Resolution: normalization coordinate domains — 2026-10-15

A bounded reader-gap audit rechecked the guide's model lookup, active-versus-
known federate identity, inherited class-member lookup, per-federate object
knowledge, typed handle encoding, and support normalization against current
2025 sources and focused tests. All 54 local source/test links and line anchors
resolve. The guide already distinguished the major flows, but it left one
normalization ambiguity: the four typed operations return the same C++ scalar
type while representing values in different standard MIM dimension
categories. Added a warning against treating cross-category numeric equality
or inequality as global handle identity, backed by Umbra's category-specific
mix inputs and the tests' actual scope (point-range validity and same-category
agreement across ambassadors, not cross-category uniqueness).

No Mermaid source changed, so the seven diagrams' existing local-render review
remains applicable and the 300-diagram inventory is unchanged. The separate
2010 stream remains untouched; no build, test, CI, or Requirements Lab work was
done. Next: a bounded reader-gap audit of the 2025 Connect, Disconnect, and
configuration guide, tracing configuration/profile selection, authorization
handoff, connection lifecycle, and cleanup against current source and focused
tests.

## 2025 Connect and Disconnect: process-host cleanup boundary — 2026-10-16

A bounded source/test review found the guide's statement that no production
callsite invokes `ProcessFederationService::detach` had become stale. The
included `ProcessFederationServiceHost::serveConnection` path runs
`connectionLost(session)` and then `detach(session)` when its serving loop ends.
The public client-side `Disconnect` still sends no Disconnect service request:
it closes its local socket and does not wait for the separate host worker.
Updated the flowchart and prose to distinguish this implemented asynchronous
host cleanup path from a synchronous public-service guarantee. A repository-wide
symbol search found host construction only in focused tests, not a non-test
server launcher in this checkout. The configured lifecycle fixture explicitly
detaches its session; the host integration case disconnects clients and then
stops the host but does not assert server-session removal or timing. The
host-related test sources were inspected, not rerun.

The existing cleanup chart was revised, not added; the global inventory remains
300 diagrams. Fresh render/visual review of the revised chart is pending. The
separate 2010 reference-RTI stream is untouched. No C++, build, test, CI, or
Requirements Lab changes were made. The pinned API's omission of `NotConnected`
from the Disconnect `@throws` list remains an unresolved normative-review item.

## 2025 Federation and Federate Lifecycle: host loss versus client/session state — 2026-10-16

A bounded reader-gap audit found a stale statement in the lifecycle guide that
the process service's connection-loss helper had no caller in `cpp/src`.
`ProcessFederationServiceHost::serveConnection` calls
`service.connectionLost(session, ...)` after its service loop ends, attempts
the cleanup path when serving throws, then detaches the service session and
closes the connection. For a joined session, the service resolves its member
and applies the registry's forced-loss transition and survivor-effect
projection; an unjoined session has no membership cleanup. `detach` clears
service routing and pending pushed-retraction recipient references, not the
federation membership transition.

The host path is present in source, but there is no non-test host construction
site in this checkout. The host integration test uses unjoined clients and
checks ordinary connection/disconnection plus host shutdown; the stalled-hello
case checks shutdown interruption. Neither verifies cleanup of a joined member
or server cleanup timing. The current-process fault fixture is a distinct,
manually forced loss/detach path. The guide's revised diagram separates the
host worker, fault fixture, and client callback/buffer paths. Its final-member
loss case also clarifies that the test reuses the same `RTIambassador` object
over a new connection and creates a fresh member lifetime; it does not resume
the lost connection or membership.

The changed chart needs fresh local render and visual review. No normative
claim, process/embedded parity claim, or 2010 behavior is inferred. No C++,
build, test, CI, or Requirements Lab work was performed.

## 2025 Delay Subscription Evaluation: nonregional process push boundary — 2026-10-16

A bounded reader-gap audit confirmed the guide already distinguishes sender
service validity, receiver eligibility timing, callback scheduling, and the
time-constrained recipient's TSO grant boundary. Its embedded/process evidence
boundary was also explicit. The process attribute route needed one additional
distinction: receive-order pull and regional push use `recheckAtReceive`, but
nonregional receive-order push stays on the existing pre-event route pending a
separate DSE slice. An unprojected candidate there is re-resolved before event
construction, not at the receiving service boundary ([source branch and scope
comment](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L210)).
A broader test-tag scan found a configured-process test file omitted by
filename-only discovery: its [regional attribute cases](../../cpp/tests/ieee1516_2025_connection_delayed_regional_attribute_update_catch2.cpp#L523)
cover the HLA_EVOKED receive fence and HLA_IMMEDIATE callback-task admission; a
third [callback/resignation fence case](../../cpp/tests/ieee1516_2025_connection_delayed_regional_attribute_update_catch2.cpp#L539)
checks an in-progress reflection and fresh rejoin. Those cases do not cover
nonregional push, process interaction DSE, or process TSO DSE.

Clarified the existing prose and added this lane to the existing process
attribute chart rather than adding a duplicate diagram. The chart now needs a
fresh local syntax/render and visual review. No normative conclusion,
process-parity claim, or 2010 behavior was inferred. No C++, build, test, CI,
or Requirements Lab work was performed.

## 2025 Object-Attribute Subscription State: empty-set unsubscribe boundary — 2026-10-16

The subscription guide already separated the 2025 declaration map from active
relevance, receiver knowledge, discovery, value projection, advisories,
regional state, and DSE timing. A bounded source audit found one more
implementation distinction worth surfacing: `unsubscribeObjectClassAttributes`
with an empty set reaches the embedded registry unchanged, where its erase
loop removes no ordinary entries (while an existing class declaration still
advances the subscription generation). The process client forwards the empty
vector unchanged, but the service expands it to the currently stored ordinary
subscriptions before registry mutation; regional declarations are not part of
that expansion. The public API exposes a separate whole-class unsubscribe
service, but the pinned header comments do not settle whether empty subset
unsubscribe should mean no-op or remove-all.

The scanned public tests cover empty sets only alongside invalid handles; none
starts from valid ordinary subscriptions and checks the resulting state on
both routes. Added a compact source/profile comparison and an explicit open
question to the guide; no additional Mermaid diagram was warranted. This is
not a normative conclusion or process/embedded parity claim. The 2010 stream,
Requirements Lab, C++, builds, tests, and CI were untouched.

## 2025 additional-module Join: regional interaction handoff reconciliation — 2026-10-16

The roadmap's active Join/regional-interaction handoff was already represented
in the current DDM guide and linked from the FOM guide. Rechecked the bounded
source chain: Join reconciles interaction/parameter/dimension handle
directories and commits the replacement definition; per-federate declarations
remain separate; regional subscription pairs are keyed by the exact declared
class handle; later recipient matching walks from the sent class toward
parents and checks overlap. The cited tests cover added dimension/class lookup
and same-class regional subscription/send separately, not a combined
post-Join parent-subscription/descendant-send scenario. The existing ancestry
chart already shows the send-time walk, so no duplicate was warranted.

Added one navigation cross-link from the ordinary interaction projection
section to the DSE guide's existing object-state-versus-interaction-history
comparison. This does not claim event replay, normative wording, process
parity, or 2010 equivalence. No source or Mermaid chart changed; no build,
test, CI, or Requirements Lab work was performed.

## 2025 Auto Provide: process automatic-discovery evidence review — 2026-10-16

The existing guide already distinguishes embedded automatic provider
solicitation from process discovery delivery, process MOM initial values, and
the federation-wide HLAautoProvide switch. A targeted source call-site scan
finds the automatic planner only on the embedded callback route. The focused
process switch case submits false and true HLAsetSwitches interactions but
does not read back the switch, trigger discovery, or assert a provider
callback; no focused process Auto Provide test was found. Corrected the
test-line link and bounded the table wording to match those observations. The
existing guide already states process behavior is unverified, so no duplicate
diagram was warranted. No normative, process-parity, or 2010 claim was added;
no C++, build, test, CI, or Requirements Lab work was performed.

## 2025 object deletion and removal: timing-guide navigation review — 2026-10-16

The object-information guide already separates accepted receive-order delete,
per-recipient removal commit, timestamped deletion, local forget, process
resignation removal, and deletion-specific retraction. The ownership guide
explains stale ownership work after accepted deletion; the time and TSO
retraction guides cover generic TSO phases and service-specific undo. Source
and focused evidence keep the invocation-snapshot restoration path embedded-
only; process timestamped-delete coverage does not establish that behavior.
Found a navigation gap from the time guide's TSO service list to Delete-specific
state effects. Added links to the existing object-removal and deletion-
retraction sections, labeled as current Umbra 2025 evidence. No duplicate
diagram, process/embedded parity, or 2010 equivalence was inferred. No C++,
build, test, CI, or Requirements Lab work was performed.

## 2025 Save/Restore: timestamped object deletion and retraction review — 2026-10-16

The Save/Restore guide already has worked traces for queued timestamped object
deletion returning through a fresh-registry restore and for Retract after one
recipient has observed removal. The focused public cases use fresh embedded
registries within one test process under both callback models; `process-restart`
tags do not prove an OS-process restart. The second test verifies restoration
of the observed receiver's object lookup and suppression of the other pending
removal. Internal filesystem state-image cases remain a different evidence
layer from public process-endpoint behavior. Added links from the object
deletion section to these existing cases rather than repeating their diagrams.
No profile or edition parity is inferred; no C++, build, test, CI, or
Requirements Lab work was performed.

## 2025 Federation MOM: state ownership across save and membership lifetimes — 2026-10-16

The existing federation-scoped and joined-federate MOM guides already separate
the objects, update fields, and evidence: the federation membership list and
next/last-save values; member `HLAfederateState` transitions during Save/Restore;
and member-object pending removal plus new identity on rejoin. Embedded tests
cover each bounded path. Process coverage demonstrates member removal before
rejoin discovery/reflection, but not the federation membership-list reflection
or member `HLAfederateState` Save/Restore. Added focused cross-links instead of
duplicating the existing state diagrams. No process/embedded or 2010 parity is
inferred; no C++, build, test, CI, or Requirements Lab work was performed.

## 2025 FOM post-Join chart: grammar check and render retry — 2026-10-16

The revised chart parses as Mermaid `flowchart-v2` using the installed Mermaid
core parser. This was a syntax-only check under Node with a no-op DOMPurify
adapter to bypass the parser package's browser-oriented sanitizer initialization;
it is not a sanitizer or render check. Mermaid CLI rendering was retried with
installed Chrome and Edge using separate profiles. Both failed before creating
an image, reporting that the browser was already running for the profile and
`The user name or password is incorrect`; the default browser launch also
failed with code `2147483651`. The 2026-10-14 visual review predates the current
chart text, so visual review remains pending. No chart semantics changed. No
build, test, CI, or Requirements Lab work was performed; no 2010 behavior or
process/embedded parity is inferred.

## 2025 Auto Provide: process-profile evidence correction — 2026-10-16

The source trace keeps process discovery state/event delivery and the separate
RTI-owned MOM initial-value path distinct from the embedded-only
`planAutoProvideForDiscovery` call. A bounded process test review found the
`HLAsetSwitches` case does more than check successful service completion: its
server-side fixture reads the internal registry switch and asserts `true` after
Join, `false` after the first adjustment, and `true` after re-enabling. It does
not call the public getter, trigger discovery, or assert
`provideAttributeValueUpdate`; no focused process Auto Provide solicitation
test was found. Corrected those statements in the guide without claiming a
normative rule or process parity. No C++, test, build, CI, or Requirements Lab
changes were made.

## 2025 additional-module Join and regional interactions — 2026-10-16

The DDM guide already traces embedded current-dimension context, exact-class
regional declaration keys, and send-time parent/overlap selection; the FOM
guide directly links that section. Focused tests separately cover additional-
module class/dimension lookup and same-class regional send, not retained
parent-region subscription followed by a Join-added descendant send. Added the
configured-process source path: Join supplies its prepared definition to the
registry, regional subscription uses the shared registry declaration methods,
and receive-order send delegates to the shared planner. The reviewed public
process fan-out case joins without additional modules and does not verify the
combined route. Added this source mapping to the DDM guide; no second diagram,
normative callback rule, process-parity claim, or 2010 claim was added. No
build, test, CI, or Requirements Lab work was performed.

## 2025 Delay Subscription Evaluation: receive-order temporal gate — 2026-10-16

The DSE reader review found no need for another route chart: the guide already
distinguishes sender-side validity, subscription/relevance evaluation time,
receive-order and TSO boundaries, directed selectors, and embedded/process
evidence. It did need to prevent a reader from treating `HLA_EVOKED` or
`HLA_IMMEDIATE` as the only receive-order delivery condition. Added prose links
to the time-management guide's receive-order/asynchronous-delivery decision
and two adjacent embedded tests. Those tests show that a time-constrained
receiver can defer receive-order work while Time Granted until asynchronous
delivery is enabled or a time advance is pending, and that asynchronous
delivery does not bypass the TSO gate. This is not DSE-focused or process
evidence and does not establish an internal rule for every RTI. No normative
§8.1.8 interpretation, 2010 comparison, or 2025 profile-parity claim was
added. No C++, build, test, CI, or Requirements Lab work was performed.

## 2025 FOM post-Join render retry and §6.21 entitlement boundary — 2026-10-16

The revised FOM post-Join chart did not render locally. Mermaid CLI's default
browser launch failed before image output with code `2147483651`; system Chrome
with a fresh profile also failed before output with an already-running-profile
message and `The user name or password is incorrect`. Keep the syntax-only
`flowchart-v2` parse separate from visual validation and leave review pending.

The Request Attribute Value Update guide's direct-requester entitlement
question also remains unresolved after a bounded source/test and authority
check. Its selected requester-as-receiver scenario has a subscription, and the
process scenario seeds subscription state through its fixture; neither tests
an unsubscribed requester receiving the provider's later update. The pinned API
declaration is not the §6.21 clause text. IEEE's public product page indicates
purchase/subscription access, while the linked ZIP was not inspectable in the
browser reader. The existing guide already states this evidence limit; no
duplicate text or diagram was added and no normative conclusion was inferred.

## 2025 mixed-order timestamped attribute passels — 2026-10-16

The new time-guide flow is a bounded source composition of the embedded 2025
registry passel planner, per-recipient RO/TSO routing, the receive-order
temporal gate, and TSO callback-before-grant dispatch. It describes the
current source path only. Existing checked-in order-control, mixed-recipient
fan-out, and receive-order-gate scenarios each cover a neighboring behavior,
but no tracked focused case was found for different attribute orders reaching
the same constrained receiver in one timestamped invocation. The untracked
scenario is not treated as test evidence.

The new Mermaid block passed a syntax-only `flowchart-v2` parse through the
local Mermaid core parser with a Node-only no-op DOMPurify adapter. This did
not render an image or validate layout; render and visual review remain
pending. Do not infer a normative rule, process-profile parity, or 2010
behavior from this embedded source trace.

## 2025 process mixed-order attribute passels — 2026-10-16

The process endpoint shares the registry's per-attribute passel plan but
materializes its paths separately: a timestamped RECEIVE passel carries the
invocation timestamp/tag without retraction identity through the ordinary
push/poll event route; a constrained-recipient TIMESTAMP passel remains in the
TSO message until its time boundary, then crosses with its message identity
and is acknowledged after callback completion. The callback bridge derives
RECEIVE/RECEIVE versus TIMESTAMP/TIMESTAMP classifications from that event
identity. This is 2025 process source mapping, not HLA protocol vocabulary.

Checked-in process TSO cases cover a single timestamped attribute reflection
before its matching grant, including callback order and the TSO classification.
No checked-in focused case was found for different attribute orders in one
invocation to the same constrained receiver. The process RECEIVE path reviewed
here has no explicit lookup of the embedded `FederateTimeState` asynchronous-
delivery/time-advance gate, and the inspected tests do not establish its
Time-Granted eligibility. Keep that process timing question open; do not infer
profile parity or 2010 behavior. The added process diagram passed a
syntax-only Mermaid parse, but has no render or visual verdict.
## 2025 per-recipient interaction order conversion — 2026-10-16

Added a reader-facing sequence to the 2025 Order and Transportation guide for
one embedded `HLA_EVOKED` timestamped interaction sent to two receivers: the
unconstrained receiver observes `sentOrderType=TIMESTAMP` and
`receivedOrderType=RECEIVE`; the constrained receiver observes both fields as
TIMESTAMP after advancing to the message time, with the callback before its
grant. Both callbacks carry a valid retraction designator. The focused test
also checks payload, tag, transportation, and logical time. Source links show
the time-constrained recipient split and immediate receive-order route.

The case is implementation/test evidence only. It does not establish the full
normative order-conversion table, process-endpoint behavior, or 2010 behavior.
The chart passed a syntax-only Mermaid 12.0.0 parse using the local core parser
and an in-memory no-op DOMPurify adapter. It was not rendered or visually
reviewed; Windows browser launch remains denied. No C++ build, test, CI, or
Requirements Lab work was performed.

## 2025 saved object-name reservation ownership across restore — 2026-10-16

The object-name reservation guide now has a bounded sequence for a two-federate
embedded `HLA_IMMEDIATE` public-API case. A filesystem-backed federation save
records an unregistered reservation with its owner; after a fresh registry and
execution are created and both federates rejoin with the same handles, restore
reattaches the name to that owner. The peer receives
`ObjectInstanceNameNotReserved` when trying to release it and a reservation-
failure callback when trying to claim it. After the owner releases the name,
the peer's reservation succeeds and the former owner can no longer release it.

The cited test recreates the registry in the same test process; despite its
tags, it is not proof of an OS process restart. The source confirms saved owner
identity capture and restore validation. This is embedded implementation/test
evidence, not a normative save/restore rule, process-endpoint result, or 2010
behavior. The new sequence passed Mermaid 12.1.0 syntax parsing. It has not
been rendered or visually inspected; Windows browser launch remains denied.
No build, test, CI, or Requirements Lab work was performed.

## 2025 discovery and first-reflection review — 2026-10-16

The existing [object-information discussion](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#first-reflection-after-discovery-order-and-evidence-boundary)
and [subscription-state comparison](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge)
already distinguish discovery from initial value reflection. Embedded source
plans subscribed current values after discovery, and its focused late-
subscription test observes both callback types without proving their relative
order. Process discovery's separate initial-value event is MOM-only; the public
process discovery test asserts discovery, and the explicit-update test is not
evidence of a discovery-triggered ordinary snapshot. Thus there is no new chart
gap for this bounded question, while ordinary process late-subscription values
and exact normative ordering remain unresolved.

Evidence links: [embedded lifecycle callback](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L322),
[process MOM-only initial-value branch](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L95),
[embedded late-subscription test](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180),
and [process explicit-update test](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457).
Pinned API headers expose signatures and clause labels, not the normative prose
needed to settle ordering. No callback-order rule or 2010 behavior is inferred.
No build, test rerun, CI, or Requirements Lab work was performed.

Next review: survey the 2010 Reference RTI synchronization-point barrier only
against the separate 2010 guide, `reference_2010.cpp`, and the 2010 binding-shell
smoke. Separate exercised participant/callback transitions from source-only
behavior; clarify the 2010 guide only if a reader gap is demonstrated. Do not
borrow 2025 behavior or imply edition parity.

## 2010 synchronization-point barrier coverage review — 2026-10-16

The existing [2010 barrier section](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md#4-synchronization-point-barrier)
already explains the selected participant set, registration/announcement,
achievement and completion callbacks, plus source-only callback-failure and
retry behavior. The [reference source's registration path](../../cpp/src/internal/runtime/reference_2010.cpp#L114)
and [achievement path](../../cpp/src/internal/runtime/reference_2010.cpp#L164)
support those implementation observations. The [2010 binding-shell smoke](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp)
has no synchronization-point scenario, so none of the barrier transitions is
test-confirmed. The guide already states this limit and avoids presenting
reference-profile behavior as a 2010 normative rule; no additional diagram or
guide change was justified. No 2025 inference, build, test run, CI, or
Requirements Lab work was performed.

Next review: audit the separate 2010 boundary between logical-time value
construction/encoding and RTI time-managed services, using only the 2010
reference guide, DataElement/encoding guide, reference source, and 2010 time
smoke tests. Preserve the independent 2010 stream.

## 2010 logical-time values versus RTI time services review — 2026-10-16

The [2010 time-boundary section](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md#5-timing-values-are-not-time-managed-execution)
already distinguishes value/factory algebra from time-managed services and
shows the reference-profile boundary separately. Integer/float implementations
and their three smoke programs cover values, arithmetic, factories, and
serialization, not RTI service calls. The [generated shell](../../cpp/generated/rti_ambassador_shell_2010.hpp#L325)
routes time-management overrides to [`unavailable()`](../../cpp/generated/rti_ambassador_shell_2010.hpp#L691);
its time-boundary errors are implementation evidence, not a universal 2010 RTI
claim. The guide already states the smoke tests do not exercise those service
stubs, so no additional diagram or guide edit was needed. No build, test, CI,
2025 inference, or Requirements Lab work was performed.

Next review: audit the separate 2010 Join/Resign membership lifecycle against
its reference guide, `reference_2010.cpp`, and the 2010 binding-shell smoke.
Keep source-only callback or cleanup behavior distinct from directly exercised
membership transitions; do not infer behavior from the 2025 stream.

## 2010 Join/Resign membership evidence audit — 2026-10-16

The 2010 guide already separates ambassador connection state from execution
membership and describes Join guards and source-only resignation cleanup. Added
a brief evidence boundary: the smoke confirms the [successful named-Join path](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp#L124),
member lookup, and [NO_ACTION resign followed by disconnect and destroy](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp#L195),
but not disconnect-while-joined, duplicate/invalid Join, destruction with
members present, alternate ResignAction behavior, or removal-callback effects.
The source implements these profile behaviors, while no focused assertion
establishes their outcomes. The two existing state diagrams and prose cover the
reader's HLA distinction; no new chart was needed. No build, test run, CI,
2025 inference, or Requirements Lab work was performed.

The active handoff is now limited to IEEE 1516.1-2025 documentation and its
profile-specific evidence gaps; this historical review does not queue further
2010 work.

## 2025 process Attribute Scope: queued stale-work boundary — 2026-10-16

The 2025 Attribute Scope guide already distinguishes a queued scope advisory
from its later application callback. In the process source, candidate
attributes are rechecked against current scope before an event is sent or
queued ([planning](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L245));
the pull receive path checks current scope again before returning queued work
([receive boundary](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L154)).
The public process regional-unsubscribe case checks the unsubscribe exception
and later owner-side In/Out callbacks, but does not mutate scope after a scope
event has been queued ([case](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L457)).
The guide already records this gap. No extra diagram or behavioral claim was
needed: the implementation path is source evidence, while the stale queued
interleaving remains untested by that public case. This is an Umbra 2025 profile
observation and does not establish a normative HLA rule or parity across
delivery profiles.

This documentation review is limited to IEEE 1516.1-2025 evidence; no build,
test run, CI, or Requirements Lab work was performed.

## 2025 active-establisher/passive-receiver subscription review — 2026-10-16

The existing [object-attribute subscription guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#passive-is-not-a-hard-delivery-veto)
already separates federation-wide active relevance from the receiver's own
eligibility. The 2025 embedded discovery resolver permits a passive receiver
after another joined, non-producing member establishes a matching active
declaration
([discovery](../../cpp/src/internal/federation/federation_registry.cpp#L869));
the value recipient planner applies the same distinction per attribute while
retaining the receiver's own subscription and class/region checks
([reflection](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L76)).

The focused passive-object test has one publisher and one subscriber; it
observes passive-only suppression and that same subscriber's later
passive-to-active transition
([passive-only case](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L120)).
Its additive-mode case likewise uses one subscriber to show an active A and
passive B projection, followed by swapping those modes
([additive case](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L356)).
Neither test has a separate active establisher and passive receiver. The guide
already states this exact source/test boundary, distinguishes discovery from
per-attribute reflection, and keeps process delivery separate. A repository
search also found passive interaction-subscription cases, but those test a
different service and do not close the object-attribute gap. No reader-facing
omission justified another chart or prose change; keep the three-role result
source-derived and unverified by a focused test.

This review is limited to IEEE 1516.1-2025 documentation; no build, test run,
CI, or Requirements Lab work was performed.

Next review: audit the 2025 synchronization case where a new member joins after
a synchronization point has partial achievement. Compare the guide's source
trace with focused checked-in tests, and keep source-only interleavings distinct
from normative HLA rules.

## 2025 partial synchronization achievement before late Join review — 2026-10-16

The existing synchronization guide already has a focused
[partial-result/late-Join sequence](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#late-joiners-change-only-an-expandable-barrier)
and explicitly labels it source-derived. Current embedded source adds a new
member to an expandable pending point's synchronization and announced sets
([participant-set insertion](../../cpp/src/internal/federation/federation_registry.cpp#L250));
the achievement path keeps the first result with `emplace` and waits until the
result count reaches the current participant count
([achievement gate](../../cpp/src/internal/federation/federation_registry.cpp#L305)).
Together these source steps preserve a prior result while making the newcomer
another required reporter. They describe the current 2025 embedded profile,
not a normative late-Join rule.

The direct registry test's omitted-set path joins its third member before any
achievement call
([join and expansion](../../cpp/tests/ieee1516_2025_federation_registry_synchronization_point_late_join_explicit_set_catch2.cpp#L49)).
The public HLA_EVOKED/HLA_IMMEDIATE scenario also joins the newcomer
([Join](../../cpp/tests/synchronization_point_catch2.cpp#L199)) before its first
achievement for that point
([achievement](../../cpp/tests/synchronization_point_catch2.cpp#L205)). Neither
tests the partial-result-before-Join interleaving. The guide's existing caveat
is accurate; no new diagram or behavioral claim was warranted. I repaired
stale registry and Join-hook line anchors in its source table so the links now
land on the relevant current implementations.

This documentation review is limited to IEEE 1516.1-2025 evidence; no build,
test run, CI, or Requirements Lab work was performed.

Next review: inspect the 2025 process RECEIVE temporal gate only for new,
checked-in evidence beyond the already documented route difference. If no such
evidence exists, retain the unresolved outcome and move to another bounded
reader gap rather than inferring parity.

## 2025 process RECEIVE temporal-gate evidence recheck — 2026-10-16

The existing [time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md)
already separates the process route, which materializes RECEIVE passels as
ordinary process events, from the embedded callback path's
`receiveOrderDeliveryAllowed` gate. A current search of C++ tests for
`enableAsynchronousDelivery` and `disableAsynchronousDelivery` did not find a
process-endpoint fixture invoking either service. The documented process
receive-order test covers ordinary reflection, and the process TSO cases cover
timestamped-before-grant delivery; neither establishes a time-constrained
process receiver's RECEIVE behavior with asynchronous delivery or a pending
advance. No new evidence changes the existing boundary, and the guide already
states it clearly, so no chart or prose change was warranted. No build, test,
CI, or Requirements Lab work was performed.

Next review: audit 2025 Request Attribute Value Update eligibility across the
pinned API, embedded/process source paths, and focused tests. Explain the
source-observed ownership/requester distinctions where useful, but keep the
normative entitlement question unresolved without the applicable standard
prose.

## 2025 Request Attribute Value Update evidence-map link audit — 2026-10-16

The existing [request-flow guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md)
already handles the direct-requester entitlement question and its normative
limit, so I did not repeat that review. A bounded content check of its evidence
map found several line anchors that had drifted to blanks or unrelated code.
I corrected links to the process MOM branch
([handler](../../cpp/src/internal/federation/process_federation_service_attribute_update_requests.cpp#L40)),
process class-call setup and invocation
([case](../../cpp/tests/ieee1516_2025_connection_process_class_request_attribute_value_update_catch2.cpp#L50),
[request](../../cpp/tests/ieee1516_2025_connection_process_class_request_attribute_value_update_catch2.cpp#L400)),
the process fixture's direct registry subscription
([setup](../../cpp/tests/ieee1516_2025_connection_request_attribute_value_update_catch2.cpp#L698)),
and the process request/Provide and later response checkpoints
([request](../../cpp/tests/ieee1516_2025_connection_request_attribute_value_update_catch2.cpp#L903),
[response](../../cpp/tests/ieee1516_2025_connection_request_attribute_value_update_catch2.cpp#L926)).
I also corrected the embedded current-owner attribute projection
([projection](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L610))
and runtime empty-projection gate
([gate](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L1763)).
These are documentation-reference corrections; no behavior or normative
eligibility claim changed.

This review is limited to IEEE 1516.1-2025 documentation. No build, test run,
CI, or Requirements Lab work was performed.

Next review: audit the 2025 object-class request's multi-provider fan-out.

## 2025 object-class Request Attribute Value Update fan-out review — 2026-10-16

The existing [request-flow guide's scope comparison](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#what-changes-with-each-scope-in-the-embedded-profile)
already explains that a class request expands over registered instances and
groups requested attributes by each instance's current owner. The embedded
planner's per-instance owner grouping is explicit
([class owner partition](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L1047));
the instance planner separately excludes attributes owned by the requester
([instance owner partition](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L318)).
The embedded class test covers two instances assigned to one provider
([request and two callbacks](../../cpp/tests/object_class_attribute_value_update_request_baseline_catch2.cpp#L176)),
and the process test likewise covers two instances from one provider under
both callback models
([request and assertions](../../cpp/tests/ieee1516_2025_connection_process_class_request_attribute_value_update_catch2.cpp#L400)).
Neither test covers a single instance whose requested attributes belong to
multiple remote providers. The guide already labels that branch source-derived
and the missing test; its distinction among requester-owned attributes,
provider callbacks, and later reflection is clear. No diagram or prose change
was warranted. This is evidence about the inspected 2025 implementation and
tests, not a normative fan-out rule or full embedded/process parity claim. No
build, test run, CI, or Requirements Lab work was performed.

Next review: audit the 2025 embedded class-request owner-change boundary
between provider planning and callback dispatch. Compare the pending
requester's/provider's identity match, current-owner attribute projection,
and empty-result suppression with the focused tests; retain the missing
ownership-change test as an evidence gap and do not infer process-profile or
normative retargeting behavior.
Trace how the source groups requested attributes by current owner and whether
the guide's existing diagram makes requester-owned attributes, provider
callbacks, and later reflection distinct. Keep the one-object/multiple-remote-
provider test gap explicit.

## 2025 process timestamped-deletion callback identity trace — 2026-10-16

The process source has two distinct producer-identity points. The outbound
timestamped-delete message records the requesting/deleting federate
([message construction](../../cpp/src/internal/federation/process_federation_service_object_instance_deletion.cpp#L107)).
At receive/removal, the registry snapshot takes the object's stored
registration producer
([snapshot](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L808));
the process removal event copies that snapshot identity
([event construction](../../cpp/src/internal/federation/process_federation_service_tso_payload_dispatch.cpp#L250)),
and the callback bridge converts it to the public handle and invokes the
removal callback
([callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L633)).
Thus the current process source maps the public removal-callback identity to
the stored registrant even though its internal TSO message records the deleter.
This is an implementation trace, not a normative HLA claim.

The focused process fixture registers and deletes through the same sender
([register](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1053),
[delete](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1123))
and checks that the callback producer matches the discovery producer
([assertion](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1158)).
It therefore does not test distinct registrant and delete-privilege owner
identities. The embedded ownership-transfer assertion remains scoped to its own
profile and test. No additional diagram was warranted: the source mapping is
clear, but its distinct-actor process behavior is not established by the
focused case. No build, test run, CI, or Requirements Lab work was performed.

Next review: audit the 2025 active-establisher/passive-receiver subscription
scenario against its focused source and tests. Preserve the evidence boundary
and revise the guide only if a reader-useful distinction is established.

## 2025 embedded class-request ownership-change boundary review — 2026-10-16

The [2025 Request Attribute Value Update guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md)
now distinguishes two source checkpoints. The final public class plan is made
under the shared federation-management lock
([final plan](../../cpp/src/internal/runtime/umbra_rti_ambassador_request_attribute_value_updates.cpp#L552));
the lock is released before the ambassador persists each provider/object group
([per-group registration](../../cpp/src/internal/runtime/umbra_rti_ambassador_request_attribute_value_updates.cpp#L587)).
Registry registration rechecks the candidate and rejects it unless the current
owner projection still equals the complete planned set
([registration](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L718),
[full-set check](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L763)).
If registration fails, the public path raises `RTIinternalError` before its
successful service-report record or callback queueing
([failure](../../cpp/src/internal/runtime/umbra_rti_ambassador_request_attribute_value_updates.cpp#L595)).
The request does not retain the shared lock across the per-group loop; an
ownership-changing operation such as Confirm Divestiture uses that same lock
([transaction boundary](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_services.cpp#L634)).
Because groups are persisted one at a time and this failure path shows no
rollback for earlier groups, the source exposes a potential partial-persistence
robustness gap if a later group goes stale. This is not an HLA rule or a
verified runtime outcome.

After a group is persisted, callback entry consumes and matches the pending
request identity
([callback-entry check](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L817));
the candidate is then recomputed for the original provider
([current-owner projection](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L531)).
A remaining nonempty subset can go to that provider, while an empty projection
is suppressed at the runtime gate
([suppression](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L1763));
the source does not retarget it to a new owner.

Focused coverage does not exercise either ownership-change window. The class
restore test registers and restores a pending request with its original owner
([registration](../../cpp/tests/federation_registry_catch2.cpp#L274),
[callback entry](../../cpp/tests/federation_registry_catch2.cpp#L352)).
A separate process `HLA_IMMEDIATE` instance-request case enters the provider
callback, then requests and cancels ownership acquisition while confirming
that the original owner remains
([request](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L628),
[acquisition](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L632),
[cancellation](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L640),
[owner assertion](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L653)).
That is not a completed transfer, not a class request, and not evidence for the
embedded callback-time projection. No focused test covers a stale pre-enqueue
group, rollback of earlier persisted groups, or ownership transfer after
class-request persistence but before callback entry.

The guide's existing callback chart was expanded to show full-set persistence
validation before the later callback-time subset check. Mermaid 12.1.0 syntax
parsing accepted the changed chart; no render or visual review was performed.
No build, test run, CI, or Requirements Lab work was performed.

## 2025 Save/Restore: pending class-level Attribute Value Update routes — 2026-10-16

Compared the public embedded class-request restore case with the internal
regional class-request state-image case. The public case restores a request
with the original provider and checks callback ordering after
`federationRestored`, preserved request tag, and no duplicate callback under
both callback models. It does not change ownership, so it cannot establish
owner-change replay, retargeting, or process-endpoint parity.

The internal registry case separately restores the pending regional request's
class/attribute/region mapping and tag, then consumes an eligible route once.
It verifies that a later committed disjoint requester region suppresses stale
work and that provider resignation retires that provider's pending route. Its
requester-departure assertion is not isolated: provider resignation occurs
first and may already retire the request. The case creates a fresh registry
within one test process; despite its process-restart tag, it is not an OS
process restart or public callback test.

Updated the 2025 Attribute Value Update and Federation Save/Restore guides to
map these as separate evidence levels and make the requester-departure gap
explicit. No diagram change was justified. No build, test rerun, CI, or
Requirements Lab work was performed.

### 2025 process regional-scope stale-work boundary review — 2026-10-16

The process notification path checks current regional eligibility before
routing. Server-queued events receive another current-state filter when the
server dequeues them for Receive Interaction; pushed events are sent after the
initial check and enter the client callback pipeline without a later registry
scope check shown in the reviewed path. The public regional test couples
HLA_EVOKED with the polling route and HLA_IMMEDIATE with pushed events in that
fixture, but exercises ordinary transitions rather than a stale-event race.
This is a source/test boundary, not an HLA callback-start timing guarantee.

Added a branching process transport chart to the 2025 Attribute Scope guide,
cross-linked it from the DDM guide, and corrected stale source anchors. Mermaid
12.1.0 syntax parsing accepted the new chart; it has not been rendered or
visually reviewed. No build, test rerun, CI, 2010 work, or Requirements Lab
change was performed.

### 2025 FOM additional-module Join observer and failure boundaries — 2026-10-16

The FOM guide now distinguishes three observations that can follow an
additional-module Join: the effective catalog changes, an eligible existing
observer may discover the newly joined member's `HLAfederate` MOM object, and
the federation-scoped `HLAcurrentFDD` value may be reflected. The embedded
focused test observes the latter after an additional-module Join. The
configured-process source path does not enqueue that federation-level
conditional update; no focused process observer test checks it. This is a
profile-specific source gap, not a normative callback conclusion.

The source audit also found post-commit failure paths in both the embedded and
configured-process Join handlers. Their inspected rollback resigns the new
member but contains no restoration of a replacement definition. Existing
failure tests use ordinary Join; the process invalid-additional-FOM test fails
before registry commit. No test verifies the catalog after a post-commit
failure involving additional modules. The revised post-Join chart passed
Mermaid 12.1.0 syntax parsing, but it remains unrendered and visually
unreviewed. No build, test rerun, CI, or Requirements Lab work was performed.

### 2025 Delay Subscription Evaluation reader review — 2026-10-16

The four existing DSE diagrams cover federation-switch lifetime, the
recipient-specific eligibility state machine, process attribute recheck
boundaries, and the high-level receive-order/TSO timing sequence. The guide
already distinguishes sender-side validity from receiver eligibility and
delivery, and labels the process interaction/TSO source paths separately from
the focused process regional-attribute cases. No additional diagram was
justified by this bounded review.

Removed an unrelated cross-edition reference-profile section to keep the guide
scoped to IEEE 1516.1-2025. No diagram changed, so its existing parser/render
status is unchanged. Its 65 local file links and line anchors were checked
with no invalid targets. No build, test rerun, CI, or Requirements Lab work was
performed.

### 2025 process RECEIVE temporal-gate audit — 2026-10-16

The process receive-order attribute route materializes or retrieves an event
and sends it through `ProcessFederationCallbackBridge::submitAttributeUpdate`;
the process interaction route passes through `submitReceiveOrder`. Both
reviewed callback-bridge paths use the process dispatcher/session without
consulting the embedded `receiveOrderDeliveryAllowed` predicate or its
deferred queue. Constrained-recipient state in the process service controls
timestamped queueing, not evidence of a receive-order time gate.

The existing test set covers embedded receive-order temporal gating and
process TSO delivery separately. No focused process test combines a
time-constrained receiver with asynchronous-delivery off/on and a pending
advance. The public process behavior and normative applicability remain open.
The time guide was extended with the process interaction source trace; no
diagram changed. New source links passed validation. No build, test rerun, CI,
or Requirements Lab work was performed.

Next review: audit the 2025 Request Attribute Value Update full-set persistence
and callback-time current-owner boundary, distinct from requester entitlement.
Keep the process regional-attribute DDM sequence first in the visual-review
queue; renderer retries remain paused until launch access changes.

### 2025 Request Attribute Value Update ownership/persistence audit — 2026-10-16

The embedded class request releases the shared federation-management lock
after planning and performs individually locked per-provider/object
registrations. Registration rechecks the complete current-owner projection;
ownership work may interleave before or between registrations. Earlier pending
records may remain persisted if a later registration fails; no rollback across
the loop is shown, and service-report/callback submission follows successful
registration of all groups. Keep this as an implementation-only potential
failure, not a portable HLA transition or a tested result.

Callback entry consumes and matches the pending request identity, then
recomputes the projection for the original provider. It may deliver a reduced
nonempty attribute subset to that provider or suppress an empty projection;
it does not choose a replacement provider in the reviewed path. The embedded
restore case retains the same provider/owner, and the process instance case
cancels acquisition before transfer. No focused test covers a completed owner
change during embedded pre-enqueue persistence or after persistence before
callback entry. The guide was clarified; no chart was added. Changed-guide
links were checked. No build, test rerun, CI, or Requirements Lab work was
performed.

Next review: audit the 2025 configured-process `HLAcurrentFDD` automatic
reflection path after additional-module Join, distinct from
membership-driven `HLAfederate` discovery and embedded behavior. Leave the
process regional-attribute DDM sequence first in the visual queue; renderer
retries remain paused until launch access changes.

### 2025 Federation MOM `HLAcurrentFDD` process Join audit — 2026-10-16

The MIM conditions the federation module list on module additions through
Create/Join and Current FDD on modification of the Current FOM subset. In the
configured-process service-report-store path, the Join handler commits the
composed definition and membership, establishes the Federation MOM record,
then plans discovery for the new member's `HLAfederate` object. The inspected
path has no federation-object Conditional update enqueue. The automatic helper
resolves joined-member `HLAfederate` values and is called by reporting and
interaction-send, not Join. A direct request for the known Federation MOM
object can project current values from the live definition.

The additional-FOM process test asserts class lookup/name, not Federation MOM
discovery or reflection; the `HLAcurrentFDD` public-observer test is
embedded-only. The existing 2025 Federation MOM guide already separates these
views and limitations, so no chart or explanatory paragraph was added.
Corrected stale in-range references to the current process Join, discovery,
member-only helper and test assertion lines, and linked directly to the MIM
condition text. All 149 local guide links and 129 line anchors validated.
No build, test rerun, CI, or Requirements Lab work was performed.

Next review: inspect the 2025 process object-removal callback identity after
ownership transfer. The bounded question is whether source snapshot identity
and focused same-federate register/delete coverage support any added reader
explanation; do not infer cross-profile parity. Keep the process
regional-attribute DDM sequence first in the visual-review queue, with renderer
retries paused until launch access changes.

## 2025 process object-removal callback identity review — 2026-10-16

The process receive-order push and pull paths and timestamped payload dispatch
populate the public removal callback producer from the registry snapshot,
which stores the object's registration producer. The internal TSO deletion
message separately records its requesting federate. Focused receive-order and
timestamped process fixtures use the same sender for registration and
deletion; neither verifies the distinct registrant/delete-privilege-owner
case. The ownership guide records these limits separately from the embedded
transfer test. No process-profile parity or normative conclusion is claimed.

## 2025 passive discovery cross-attribute gate follow-up — 2026-10-16

A follow-up source trace of passive object discovery found that the resolver
stores active relevance as one boolean across its non-producer
class/attribute scan, then checks the receiver's own class/region declaration
separately. The active-establisher entry therefore need not be for the same
attribute as the passive receiver's entry in this source path
([active scan](../../cpp/src/internal/federation/federation_registry.cpp#L873),
[receiver selection](../../cpp/src/internal/federation/federation_registry.cpp#L1037)).
The focused passive-subscription test uses one publisher and one subscriber
([test](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L120));
it does not test this three-role cross-attribute case. The subscription guide
now explains this as a source-derived consequence, not an HLA claim or process
test result. Its Mermaid decision label changed after the last syntax pass;
targeted syntax recheck and rendering remain pending.

No build, test run, CI, or Requirements Lab work was performed.

Next review: audit the 2025 process regional attribute-scope event's
post-enqueue eligibility boundary. Compare pull receive-time rechecking with
the pushed callback route and focused stale-event tests; keep any untested
race source-only. Leave the process regional-attribute DDM sequence first in
the visual-review queue and pause renderer retries until launch access changes.

---
### 2025 process regional-scope post-enqueue eligibility audit — 2026-10-16

The process service rechecks current eligibility before routing each planned
scope event. For pull delivery, `process_federation_service_receive.cpp`
rechecks the queued attributes against current registry state when a later
`receive_interaction` request dequeues the event. For pushed delivery,
`process_federation_client.cpp` records the event, the client dispatcher
submits it to `ProcessFederationCallbackBridge`, and the bridge schedules the
public callback; the reviewed chain contains no subsequent registry
eligibility lookup. The bridge's structural event validation is not a
freshness check.

The public process regional-unsubscribe scenario covers ordinary scope
transitions under HLA_EVOKED and HLA_IMMEDIATE, with the fixture selecting the
pull and pushed service paths respectively. It does not stage a mutation in
the admission-to-observation interval, so it does not verify stale queued or
pushed event suppression. The existing Attribute Scope guide already states
these distinctions and the open race; no duplicate content was added. No
normative timing guarantee or embedded/process parity is inferred. No 2010
work, build, test rerun, CI, or Requirements Lab change was made.

Next review: inspect the 2025 synchronization-point partial-achievement/
late-Join boundary. Compare barrier membership expansion and achievement
retention in source with the focused test's Join timing; keep the
interleaving source-only if no test directly covers it. Preserve the
2025-only scope and the pending process DDM visual-review priority.

---
### 2025 synchronization partial-achievement/late-Join audit — 2026-10-16

The shared registry records `lateJoinExpansionAllowed` for omitted-set
registration, inserts a newly joined member into both the participant and
announced sets, and retains previously recorded achievements. Completion is
checked by comparing achieved-entry count with current participant count
([expansion flag](../../cpp/src/internal/federation/federation_registry.cpp#L188),
[participant insertion](../../cpp/src/internal/federation/federation_registry.cpp#L250),
[achievement fan-in](../../cpp/src/internal/federation/federation_registry.cpp#L305)).
The configured-process Join handler invokes this same registry hook and queues
the resulting event
([Join hook](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L801),
[queue helper](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L285)).

The embedded public scenario joins late members before any achievement
([Join and later first report](../../cpp/tests/synchronization_point_catch2.cpp#L199));
the direct registry test covers expansion policy without partial results
([focused case](../../cpp/tests/ieee1516_2025_federation_registry_synchronization_point_late_join_explicit_set_catch2.cpp#L49)).
The process rollback case queues an announcement, fails Join, then retries
([test boundary](../../cpp/tests/ieee1516_2025_connection_process_mom_resign_removal_order_catch2.cpp#L428)),
but has no achievement report before that Join. Therefore the partial-result-
before-late-Join sequence remains source-derived in both profiles; no
normative completion rule or cross-profile parity is inferred.

Corrected the synchronization guide's process evidence row to point to the
configured-process Join expansion hook and actual queue helper rather than
registration handling, and made the process test's missing prior-achievement
coverage explicit. The existing source-derived sequence remains appropriate;
no duplicate diagram was added. No 2010 work, build, test rerun, CI, or
Requirements Lab change was made.

Next review: audit the 2025 Attribute Scope Advisory switch-off evidence.
Check whether the embedded and configured-process focused cases isolate
switch-off as the sole suppressor of an otherwise-reportable transition.
Keep any confounded test as an evidence gap and preserve the 2025-only scope.

---
### 2025 Attribute Scope Advisory switch-off coverage audit — 2026-10-16

The embedded path checks the receiver switch before scheduling a scope
callback ([adapter gate](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L1208));
the shared registry eligibility predicate returns empty while the switch is
off ([predicate](../../cpp/src/internal/federation/federation_registry_object_instance_removal.cpp#L12)).
The configured-process event admission and pull receive paths use that same
predicate ([admission](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L238),
[pull receive](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L161)).

The only focused embedded switch-off scope scenario disables the switch and
then performs the receiver's own final regional unsubscribe
([test](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L302)); the
subscription planner separately suppresses Out for that final self-unsubscribe
([planner](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L613)).
It therefore does not isolate the switch. The configured-process regional
scenario sets the switch true ([setup](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L893)).
A scan of setter calls under `cpp/tests` found remaining false uses in
MOM/service-report checks and Save/Restore interlock rejection, not scope
callback suppression. Source supports switch gating for work evaluated while
disabled; no focused test demonstrates switch-off alone suppressing an
otherwise-reportable transition. The state after an event has already been
pushed remains a separate test gap.

Updated the Attribute Scope guide's switch note and evidence table with these
links and boundaries; no diagram or normative wording was added. No 2010
work, build, test rerun, CI, or Requirements Lab change was made.

Next review: verify the normative basis for the 2025 Attribute Scope Advisory
Switch and final-self-unsubscribe exception against canonical IEEE
1516.1-2025 clause text. If authoritative text is unavailable, preserve the
unverified normative status; do not use source/tests to fill that gap.

## 2025 normative-source availability and timing-guide no-change review — 2026-10-16

The official [IEEE 1516.1-2025 edition page](https://standards.ieee.org/ieee/1516.1/6688/)
confirms the active edition and publication date, and offers purchase or
subscription access. It does not expose the clause prose needed to verify the
Attribute Scope Advisory Switch or final-self-unsubscribe interpretation. The
listed downloads archive could not be inspected by the available browser
reader, and the local pinned 2025 dependency contains API declarations rather
than the full standard. Therefore the guide's clause 10.1.3 source comment
remains implementation evidence only; its normative interpretation stays
unverified.

As a separate 2025 timing review, the time-management guide's receive-order
section correctly states the embedded gate (unconstrained, asynchronous
delivery enabled, or a pending advance), its deferred-callback behavior, and
the two callback-model outcomes. The focused temporal-state scenario exercises
those edges under HLA_IMMEDIATE and HLA_EVOKED. The guide separately marks the
process callback route and lack of a process endpoint scenario as unresolved;
the reviewed process callback bridge does not itself consult the embedded
time-state predicate. This boundary is already clear, so no guide or diagram
change was warranted. No normative or cross-profile parity claim is inferred.

Next handoff: continue the active documentation pass using only IEEE
1516.1-2025 material. Keep the process RECEIVE/asynchronous-delivery timing
outcome source/test-limited until focused process evidence exists; otherwise
select the next answerable reader gap from the 2025 guide backlog. Keep
rendering paused until the environment permits browser launch.

## 2025 configured-process object-removal producer identity — 2026-10-16

The ownership guide now includes a configured-process source-only sequence for
timestamped deletion after delete-privilege transfer. It separates the TSO
message's deleting-requester identity from the original registrant returned by
the receiver's registry removal snapshot and forwarded to the public callback.
The focused embedded test observes distinct registrant and delete-privilege
owner; focused process deletion cases use the same federate for registration
and deletion, so they do not verify the A≠B process outcome.

The new Mermaid sequence parsed with Mermaid 12.0.0 core. It has not been
rendered or visually inspected; render retries remain paused after the prior
Chromium launch failure. This does not update the historical local-render
snapshot or establish GitHub rendering.

## 2025 configured-process Save notification-send failure boundary — 2026-10-16

The 2025 process Save handler calls the shared registry's completion transition
before routing its terminal notifications. In pushed mode, the notification
helper sends recipient frames sequentially and returns failure on encoding or
send failure; the handler then produces an internal-error response. If a later
send fails, earlier frames may already have been sent. The process client
drains buffered pushed events before the typed Save Complete method checks the
response status. Thus, if the requesting member's own frame is already
buffered, a returning `HLA_IMMEDIATE` callback may precede the public
`RTIinternalError`; under `HLA_EVOKED`, the task may remain queued for a later
Evoke. The public adapter maps the client error after that drain. No rollback
of the registry's resolved save state or earlier event sends is visible.

The existing process Save tests cover ordinary polling/pushed success paths,
not a partial terminal-notification send failure. This is a source-composed
possibility, not a tested public outcome, failed-connection guarantee, or HLA
callback-order rule. Expanded the Save/Restore guide prose without changing
either sequence diagram, so the 24-chart visual-review count remains intact.

The Attribute Scope process-event chart remains visually pending. A fresh
Mermaid CLI 12.0.0 attempt with the explicit installed Chromium executable
failed before image creation (process exit 3221225506); defer another attempt
until browser launch conditions change.

Next review: audit the 2025 configured-process regional class-level Request
Attribute Value Update preflight. Compare validation ordering with provider
expansion and focused public cases, specifically whether a late invalid region
is shown to suppress otherwise-eligible provider work. Keep untested effects
source-only and do not add a duplicate chart if the current guide already
answers the reader question. No build, test, CI, 2010, or Requirements Lab
work.

## 2025 configured-process regional class-request preflight audit — 2026-10-16

The registry validates all supplied request-region pairs before the
class-request plan expands to object instances and provider groups. The
configured-process handler returns the typed rejection before it enters the
provider loop. The public invalid-selector case calls the API separately for
an unknown handle, another member's region, an uncommitted region, and a
committed region with an unavailable dimension; it asserts the four mapped
exceptions. The fixture does not publish the object class or register an
instance, so it does not establish an otherwise-eligible provider route whose
suppression could be observed. The no-provider-event result therefore remains
source-derived, not test-established, normative atomicity, or exception
precedence.

The existing RAVU flow already places whole-request validation before
expansion. Clarified the test limit in the 2025 guide; no duplicate diagram or
runtime/test change was warranted. No build, test, CI, 2010, or Requirements
Lab work.

Next review: audit the 2025 process three-role passive-discovery boundary.
Trace registration and subscription through the shared cross-attribute
active-relevance gate, then inspect public process cases for a separate active
establisher and passive receiver on different attributes. Do not infer a
process result from the embedded test; preserve source-only findings if no
three-role process case exists. Keep local render retries paused until a
supported browser launch path changes.

## 2025 configured-process three-role passive-discovery evidence audit — 2026-10-16

The configured-process subscription and registration handlers delegate to the
shared 2025 registry, which owns the discovery selector. The public process
object-subscription case exercises one member's active-to-passive declaration
transition and unsubscribe, but creates no ordinary object and has no separate
active establisher. It is not evidence for the three-role discovery outcome.
Updated the subscription guide with this boundary and direct source/test
links. No Mermaid changed; visual-review counts are unchanged. No build, test,
CI, 2010, or Requirements Lab work.

Next review: audit ordinary configured-process late subscription to an
already-registered object, keeping RTI-owned MOM initial-value coverage
separate from ordinary-object behavior and leaving the exact §5.8 claim open.
Defer Attribute Scope rendering until a supported browser launch path changes.

## 2025 configured-process late-subscription initial-value audit — 2026-10-16

The existing profile chart and prose accurately distinguish ordinary discovery
from initial values. The service creates an ordinary discovery event but no
ordinary initial-value event; the separate value branch is MOM-only. Pull
queues discovery and MOM values separately; Push stages and sends discovery
frames before MOM-value frames. The public MOM observer is HLA_EVOKED and does
not assert relative callback order or Push ordering. The ordinary public
process case has no reflection observer, and the precise §5.8 interpretation
remains open. Added the staging nuance to the guide without changing Mermaid.

Next review: audit configured-process untimed Update Attribute Values failure
mapping across local validation, remote planner rejection, and public
exception translation. Keep timestamped behavior separate and the Attribute
Scope visual retry deferred until browser launch conditions change.

## 2025 configured-process untimed Update Attribute Values failure mapping — 2026-10-16

The public process adapter validates local handle wrappers before sending the
request. Conversion failure maps to typed `ObjectInstanceNotKnown` or
`AttributeNotDefined`. For converted values, the shared registry planner
distinguishes unknown object, undefined attribute, and not-owned attribute;
however, the process service returns one generic rejected response for all
non-applied plans. The client converts that response to
`ProcessFederationClientError`, and the adapter surfaces `RTIinternalError`.
The embedded source translator returns corresponding typed exceptions, but no
normative conformance or cross-profile parity conclusion follows from this
comparison.

The focused public process test checks only successful untimed submission and
zero recipients; remote failure behavior remains source-derived. The new chart
passed Mermaid 12.0.0 core syntax parsing after an in-memory no-op sanitizer
bypassed the Node DOMPurify initialization problem; it has not been rendered or
visually reviewed. The 2025 inventory now counts 318 diagrams (180 flowcharts,
115 sequences, 23 state diagrams), all syntax-accepted; 24 selected charts
remain visually reviewed. No C++ build, test, CI, or Requirements Lab work.

Next review: audit 2025 configured-process discovery-triggered Auto Provide.
Trace subscription-triggered discovery, provider solicitation, Pull/Push
delivery, callback entry, and focused public tests. Keep embedded behavior
separate and classify any process path without direct public evidence as
source-derived rather than normative. Keep Attribute Scope rendering paused
until browser launch conditions change.

## 2025 configured-process Auto Provide source/test boundary — 2026-10-16

The existing Auto Provide guide already has a configured-process discovery and
switch-control chart, with an explicit source-only boundary. Rechecking the
2025 source confirms `planAutoProvideForDiscovery` is called only by the
embedded discovery callback. The process subscription handler replans visible
object discovery; the process discovery service commits known-object state and
queues or sends `discoverObjectInstance`, while ordinary relevance advisories
and RTI-owned MOM initial values are separate events. The process switch
handler updates shared HLAautoProvide state but does not trigger solicitation.

The public process discovery test records discovery only. The separate process
switch test changes the switch and verifies stored server state. Neither
observes `provideAttributeValueUpdate` as a result of discovery. These source
and test results justify no added guide content or duplicate chart; they do
not establish normative omission or process/embedded parity. Diagram count and
visual-review count are unchanged; no build, tests, or CI were run.

Next review: audit 2025 configured-process Federation MOM FOM updates on
module-adding Create/Join. Trace the standard's conditional concept against
process event production, the live MOM projection, and focused public
observers. Keep any uncovered runtime result source-derived and update the
existing guide only for a demonstrated learner gap. Keep Attribute Scope
visual review paused until browser launch conditions change.

## 2025 configured-process Federation MOM FOM update review — 2026-10-16

The existing Federation MOM guide already answers the bounded question. The
2025 MIM marks the module-designator list Conditional when Create/Join adds
FOM modules, and `HLAcurrentFDD` Conditional when those services modify the
Current FOM subset. The configured-process additional-module Join commits the
composed definition and membership, establishes the execution-scoped MOM
record, and queues discovery for the joining member's MOM object; the reviewed
path does not enqueue a federation-level Conditional reflection. Explicit
current-value requests project the live registry definition.

The public process additional-FOM case asserts Join and class lookup/name,
not Federation MOM attributes. The `HLAcurrentFDD` reflection case is
embedded-profile evidence; the process joined-member MOM observer covers a
different object and different values. The existing chart and evidence table
already distinguish these paths, so no duplicate content was added. Keep the
missing public process observer as an evidence gap, not a conformance finding
or parity result. No diagram count or visual-review change; no build, tests,
or CI.

Next review: inspect the 2025 configured-process `HLAfederatesInFederation`
projection at Join/Resign, including event production, the explicit
current-value request, and public observer coverage. Keep source, test, and
MIM statements separate; change the guide only for a demonstrated learner
gap. Keep Attribute Scope visual review deferred until browser conditions
change.

## 2025 process Federation MOM membership-list review — 2026-10-16

The existing Federation MOM section already distinguishes the MIM's
Conditional membership trigger from process member-object events. The live
Federation projection encodes the registry membership map. Process Join and
Resign handle the affected `HLAfederate` MOM object but the reviewed paths do
not enqueue an automatic Federation-list reflection; explicit requests use
the current membership projection and require the requester to remain a
member and know the Federation object.

The embedded public test observes list reflections at Join and Resign. The
configured-process resignation test checks member-object removal/rejoin
ordering, not the Federation list, and no process observer case was found for
either automatic or requested list values. Existing diagrams and test limits
were sufficient, so no duplicate chart or prose was added. This is not a
normative omission or profile-parity conclusion. No build, tests, or CI.

Next review: audit the 2025 FOM-to-`DataElement` variant-record bridge across
standard type concepts, composition/resolution, runtime construction/encoding,
and configured-process tests. Keep the catalog-to-runtime integration gap
explicit and do not infer process/embedded parity. Keep Attribute Scope visual
review deferred until browser conditions change.

### 2025 FOM variant-record value mapping — 2026-10-16

**Question:** Does the current bridge explain what application code must do to
translate a composed variant definition into the runtime `DataElement` map?

**Evidence:** The 2025 catalog projection stores the variant discriminant type
name, alternative names, enumerator-name strings, and payload type names. The
pinned `HLAextendableVariantRecord` API instead accepts a typed discriminant
prototype and runtime `DataElement` values/prototypes. The Restaurant fixture
provides a concrete example (`DrinkType`, `Margarita = 1`, `DryMartini = 2`,
`HasSaltRim`, `OliveCount`). The composer test checks model composition, the
codec test constructs a map directly, and the configured-process Create test
only checks acceptance of the Restaurant FOM; none joins these paths.

**Disposition:** Added a short application-side mapping example to the 2025
FOM composition guide. It is labeled illustrative rather than normative or
end-to-end tested. No third diagram was needed; the existing bridge diagram
already shows the architectural separation. No build, test, CI, 2010, or
Requirements Lab work.

**Next review:** Audit the 2025 public Update Attribute Values and Send
Interaction value boundary: caller-side `DataElement` encoding, RTI validation
and forwarding, receiver decoding, and configured-process framing. Amend only
for a demonstrated learner gap; keep process/embedded evidence separate.

### 2025 process Update Attribute Values payload boundary — 2026-10-16

**Question:** Does the process Update Attribute Values flow make clear whether
the RTI decodes application payloads as `DataElement`s, or only handles their
octets?

**Evidence:** The 2025 public API accepts a handle-keyed map of
`VariableLengthData`. In the configured-process update handler, the registry
plan is built from attribute handles; payload vectors are copied into accepted
state and recipient events. The event codec writes the byte vectors alongside
their handles, and the callback bridge reconstructs `VariableLengthData` values
without a `DataElement` decode. The focused process reflection test sends
literal three-octet value and tag arrays and asserts they arrive unchanged.
This does not establish general payload validation or a typed end-to-end codec
path. The process Send Interaction companion already states its ordinary
parameter-octet boundary and notes the separate predefined `HLAsetSwitches`
control path.

**Disposition:** Added a source-bounded paragraph and direct test references to
the 2025 process guide's receive-order update section. Kept frame decoding,
HLA value decoding, and callback map reconstruction as separate steps. No
diagram change or normative claim; no build, test, CI, 2010, or Requirements
Lab work.

**Next review:** Audit the 2025 predefined `HLAsetSwitches` interaction for
parameter interpretation, switch-state update, and MOM projection across the
existing API/source/test evidence. Keep it separate from ordinary application
interaction fan-out and amend only for a demonstrated learner gap.

### 2025 predefined HLAsetSwitches scope and parameter flow — 2026-10-16

**Question:** Does the Federation MOM guide make clear that the 2025 MIM has
two distinct `HLAsetSwitches` class paths with different state and parameter
semantics?

**Evidence:** The federation-root interaction carries `HLAautoProvide` as an
`HLAswitch`; the joined-federate interaction carries eight `HLAswitch`
parameters plus `HLAautomaticResignAction` as `HLAresignAction`. The pinned
MIM defines the switch as `HLAinteger32BE` 0/1 and the resign action as
`HLAinteger32BE` 0–5. Embedded and configured-process sources decode these
paths separately. The member update stages recognized values and the shared
registry checks the Service Reporting subscription interlock before mutation.
Selected process tests cover state/getters, compatible extension parameters,
and several malformed/interlock failures; they do not cover every combined
failure or establish process Conditional reflection. The embedded setter test
is recorded as failing before its switch assertions.

**Disposition:** Added a 2025-only class-path comparison and flowchart to the
existing Federation MOM guide, including special RTI consumption and current
MOM projection. The section's 67 local paths/line anchors resolve. No ordinary
interaction fan-out, cross-profile parity, or normative conclusion is
inferred. No tests/build were run. Syntax and visual validation remain
pending: the direct Mermaid core parser encounters `DOMPurify.addHook is not a
function`, and local mmdc rendering cannot launch Chromium.

**Next review:** Audit the 2025 `HLAreportMOMexception` flow for rejected
predefined `HLAsetSwitches` adjustments: process rejection, report
construction, recipient selection, and related embedded evidence. Keep it
distinct from `HLAreportException`, ordinary `HLAserviceInvocation` reports,
and public service exceptions. Amend only for a demonstrated gap; do not build
or rerun tests.

### 2025 HLAreportMOMexception and rejected HLAsetSwitches review — 2026-10-16

**Question:** Does the existing 2025 Exception Reporting guide explain the
MOM report produced when a predefined `HLAsetSwitches` request is malformed or
rejected by a service precondition, including report recipients and how this
differs from other failure reports?

**Evidence:** The pinned MIM declares `HLAreportMOMexception` as a reliable,
receive-order interaction on the `HLAfederate` dimension, with `HLAservice`,
`HLAexception`, and `HLAparameterError` (plus inherited `HLAfederate`). The
process server plans the report and recipient events before returning the
rejection; its recipient planner applies subscription, class projection, and
the report's subject-scoped dimension route. The embedded Send Interaction
catch attempts the MOM report, then the independent `HLAreportException` hook
and failed service-invocation writer. The configured-process client rejection
branch returns before those embedded-only catch steps. Focused embedded tests
cover malformed and precondition failures; process cases verify the typed
payload, exception switch off, parent-class parameter projection, and selected
callback/event modes. Neither profile's cited cases assert all report routes
for one failure.

**Disposition:** The existing guide already contains this end-to-end
distinction, recipient boundary, source mapping, and test limits. No new flow
or prose was warranted. The bounded Route B link check did find one stale
embedded catch-order anchor; corrected it from line 2396 to the actual catch at
line 2395. All 27 local Route B links now resolve. No build or tests.

**Next review:** Audit the two 2025 `HLArequestFOMmoduleData` class paths and
their module-index/report projection across the existing Federation MOM and
FOM admission guides plus focused evidence. Keep federation/member request
scope and returned report content distinct; amend only for a demonstrated
reader gap. Do not build or rerun tests.

### 2025 FOM module-data request/report profile boundary — 2026-10-16

**Question:** Do the existing 2025 MOM guides make the distinct
`HLArequestFOMmoduleData` paths clear, and is there evidence that the
configured-process endpoint performs the paired report operation?

**Evidence:** The MIM defines separate federation- and joined-member requests.
Both are reliable, receive-order `Subscribe` interactions; the federation
request takes `HLAFOMmoduleIndicator`, while the member request also inherits
the `HLAfederate` target. The existing Federation MOM guide's flow/table and
embedded source/tests already explain those separate module lists and report
classes. Its federation test exercises the retained federation module list;
the member test targets its own member and observes selected report content.
For configured process, the public ambassador forwards the request through a
private parameter/tag envelope. The reviewed server handler handles only the
two `HLAsetSwitches` roots specially before using the generic published
interaction planner. The bounded source search found no process-specific FOM
request/report planner, and the test search found no process case for either
scope.

**Disposition:** Added a source-only process-boundary note to the existing MOM
guide. It marks actual process outcome, report production, exact public error,
and parity unverified; it does not claim nonconformance or infer behavior from
the missing branch. The edited subsection's 29 local citations and anchors
resolve. No new diagram, build, or test.

**Next review:** Audit the 2025 `HLArequestMIMdata` path across the embedded
report handler, configured-process Send Interaction route, and focused
evidence. Compare with the existing Federation MOM guide; document only a
demonstrated gap and keep process behavior source-bounded. Do not build or
rerun tests.
