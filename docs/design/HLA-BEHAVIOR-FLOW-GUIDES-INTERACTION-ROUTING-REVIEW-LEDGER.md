# HLA Behavior Flow Guides: Interaction Routing Review Ledger

This companion preserves the focused review history for ordinary and directed
interaction class/parameter routing. For current priorities, use the [HLA
behavior-flow guide index](HLA-BEHAVIOR-FLOW-GUIDES.md); general and cross-
service review history remains in the [main review ledger](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md).
Each dated entry retains its original evidence limits.

## Ordinary and directed interaction class/parameter routing — 2026-10-11

The existing 2025 materials already make the main service distinction clear:
ordinary interactions use class-level active relevance plus per-receiver
subscription matching, while directed interactions name a target object and
use an object-class/interaction-class declaration pair with a receiver
selector. The focused comparison review found that callback-class and
parameter behavior was explained in an adjacent ancestry map but absent from
the concise side-by-side table. Added a row summarizing the reviewed paths:
ordinary receive-order delivery may promote to the closest eligible
subscribed class and project parameters; the current directed receive-order
candidate retains the sent class and resolves parameters in that class's
namespace.

Current source supports the distinction. The ordinary registry test asserts
exact, promoted, and ancestor callback classes and projections, but is an
internal plan test. The directed test validates parameters for an exact-class
send and rejects a foreign parameter; it does not test ancestor declarations.
The process multi-recipient case also uses exact-class declarations. The guides
already label directed embedded callback-time revalidation versus process
event materialization and leave stale process delivery untested. No extra
diagram was warranted, and no new normative inheritance, parity, or 2010 claim
was introduced. No build, test, CI, or rendering was performed.

### 2025 FOM vocabulary-to-runtime boundary — 2026-10-11

The FOM guide already separates per-module DIF validation, completed-model
composition/reference resolution, and FDD materialization from claims about
runtime support. A bounded reader check found one useful local bridge missing:
the accepted model supplies shared vocabulary, but is not itself a federate's
publication/subscription state, an object registration, or peer knowledge of
an instance. Added a short note linking the existing cross-cutting
model-vocabulary/runtime-state map and the object/interaction flow. The pinned
2025 API presents those actions as separate services, and the linked guide
already illustrates the registration-versus-discovery boundary; no duplicate
diagram or new normative claim was needed.

This was documentation-only. No implementation/test audit, build, test rerun,
CI, or Mermaid render was performed. The 2010 RTI stream and Requirements Lab
remain untouched.

### 2025 name-and-handle identity-scope audit — 2026-10-11

The current guide already follows the requested learner path: model names
resolve in a joined execution; federate names and handles have distinct active
versus-known identity paths; and object-instance lookups are limited to the
calling federate's known-object state. Its matrix records category-specific
forward/reverse behavior and absence outcomes, while the diagrams explicitly
avoid implying discovery from a model declaration or a registration alone.
Targeted source review confirmed `getFederateHandle` consults active membership,
`getFederateName` uses the execution's identity mapping, and object-instance
lookups query the member-scoped known-object index. The focused process test's
post-resignation asymmetry and embedded test's evoked-discovery threshold are
kept scoped to those scenarios. The guide already marks process/embedded gaps
and keeps 2010 separate, so no content change was warranted.

No build, test rerun, CI, or Mermaid render was performed. The 2010 RTI stream
and Requirements Lab remain untouched.

### 2025 reservation callback timing and registration boundary — 2026-10-11

The owner-reservation/live-instance/peer-knowledge distinctions were already
clear. The single-name callback sequence had one misleading implication: its
normal-return arrow preceded callback delivery for every callback model. The
shared Umbra federate callback route submits to `CallbackDispatcher`; the current
immediate dispatcher drains synchronously, so callback code can run inline
before `reserveObjectInstanceName` returns. Under `HLA_EVOKED`, the callback
remains queued until an evoke call. This is current source behavior, not a
general HLA schedule.

Added pinned 2025 API landmarks for the void reservation/release calls, the
separate reservation callbacks, named registration's returned handle, and the
peer discovery callback. Revised the single-name Mermaid sequence to branch
between immediate and evoked delivery, and changed “asynchronous failure” to
“reported through the failure callback” so immediate dispatch is not obscured.
The focused embedded case confirms pending outcomes before evoke. The process
integration case includes both callback models, but its immediate branch does
not directly assert the exact service-return boundary; that point is source-
derived. Bulk partial/atomic semantics, reservation consumption at commit,
and the separate peer-discovery evidence remain unchanged.

The [official IEEE 1516.1-2025 landing page](https://standards.ieee.org/ieee/1516.1/6688/)
confirms the edition and that 1516.1 defines RTI services/interfaces, but it
does not expose the full clause text; the new header links are signature
navigation, not a substitute for licensed normative verification. No build,
test rerun, or CI was performed. `mmdc` was not available in PATH and no
repository Mermaid parser/renderer was found, so the revised chart's local
parse/render and visual review remain pending. The 2010 RTI stream and
Requirements Lab remain untouched.

Next handoff: use an available GitHub-compatible Markdown/Mermaid preview to
parse and visually inspect the revised single-name reservation sequence. If
none is available, keep local and GitHub visual verification explicitly
pending, do not install project dependencies or run C++ checks for it, and then
resume the HLA-first guide survey.

### 2025 object removal, receiver callback, and local forget — 2026-10-11

A bounded HLA-first survey compared the pinned API landmarks, the existing
object/interaction guide, current 2025 implementation paths, and its linked
focused tests. The guide already distinguishes the application's
`deleteObjectInstance` request (§6.16), the receiver-side
`removeObjectInstance` callback (§6.17), and the separate
`localDeleteObjectInstance` service (§6.18). Its flows and comparison table
also separate execution-wide delete acceptance, each receiver's later
known-object transition, caller-only local forgetting, and cleanup caused by
voluntary resignation or connection loss.

The receiver reservation/commit gates and stale-callback suppression are
explicitly labeled as current Umbra behavior. The evidence map links embedded
delete ordering, process removal payload, local-delete/rediscovery,
`DELETE_OBJECTS`, and selected connection-loss cases. The API landmarks do not
replace the full licensed normative action rules, and the guide does not claim
profile parity. Since the existing two diagrams, comparison table, and links
already answer the “which object state disappeared?” question, no additional
diagram or duplicate guide was warranted.

No C++ build, test rerun, or CI was performed. The reservation callback
sequence's Mermaid/visual preview remains pending: no permitted local or
GitHub-compatible preview surface was available, so review was not routed
through an unrelated browser or desktop path. The 2010 stream and Requirements
Lab remain untouched.

Next handoff: audit the HLA-facing chain for `requestAttributeValueUpdate`
against the ownership and object/interaction guides. Check whether the
request, provider responsibility, submitted value, receiver eligibility, and
`reflectAttributeValues` observation are explained as distinct events; revise
or add a diagram only for a demonstrated reader gap.

### 2025 explicit value request versus reflection eligibility — 2026-10-11

The bounded review found the core HLA-facing chain already clear in the
request guide: a void request solicits a provider callback, the provider may
later submit values with a separate update call/tag, and an eligible receiver
may later observe a reflection. The existing diagrams also distinguish
object-instance, class, and regional scopes, callback delivery, and separate
save/restore cut points; a duplicate overview diagram would not add a distinct
reader model.

One statement needed a sharper evidence boundary. In the current embedded 2025
path, the request planner selects provider work, while the provider's later
update uses ordinary receiver planning; the request itself does not mutate
subscription declarations or record a special reflection recipient. The
embedded baseline explicitly calls the public subscription service. The
process test fixture instead seeds the requester's subscription directly in
the registry before issuing the public request; it does not exercise the
public process subscription service. Neither case isolates an unsubscribed
requester. The guide now labels the recipient-planning point as embedded-source
behavior and
leaves the normative question of direct response entitlement for an otherwise
ineligible receiver open pending the full §6.21 text. No 2010 or
process/embedded equivalence is inferred.

A follow-on fan-out audit compared the embedded per-object owner grouping with
the focused object- and class-request tests. The tests demonstrate a single
remote provider serving multiple requested attributes or multiple expanded
objects; none demonstrates one object's requested attributes being split
across two remote providers. Keep that branch labeled source-derived and
untested, rather than promoting the implementation's grouping algorithm into a
portable HLA rule.

No new diagram or test was added; the existing four diagrams already show the
distinct service and observation events. No C++ build, test rerun, or CI was
performed. The reservation sequence's Mermaid/visual review remains pending
until a permitted preview path is available. The 2010 stream and Requirements
Lab remain untouched.

### 2025 interaction delivery across the guide set — 2026-10-11

A bounded cross-guide survey followed ordinary interaction delivery from
sender declaration through receiver observation. The object/interaction guide
already separates publication, recipient relevance, object knowledge, message
order, and callback observation; its interaction diagrams cover subscription
matching, active relevance, passive receivers, class promotion, parameter
projection, and the embedded accepted-send boundary, including a successful
zero-recipient send with no application callback. The cross-cutting guide
provides a service-neutral checklist, while the order/transportation and time
guides separately explain receive-order routing, TSO queues/grants, and
callback entry. These existing diagrams fit together without imposing one
universal service pipeline.

Directed interactions remain a distinct service path because their target
object and sender publication pair are not ordinary interaction subscription
semantics. The embedded accepted-send and callback-fan-out details remain
profile-bounded; the focused process send case does not establish full
class-promotion or delivery parity. No new diagram was warranted. This was a
bounded documentation/source review; no build, test rerun, or CI was performed.
Keep the 2010 stream and Requirements Lab untouched.

### 2025 active/passive interaction subscription audit — 2026-10-11

The existing interaction flow already explains the useful mental model: an
active non-sender establishes federation-level relevance, while each receiver
must independently match a subscribed class; passive subscriptions can qualify
for delivery only after relevance exists. Regional receivers additionally
need overlap. The registry source keeps those relevance and recipient checks
separate (`cpp/src/internal/federation/federation_registry.cpp`, ordinary
receive-order planning), and stores active/passive declaration state in the
2025 interaction-subscription path.

The pinned 2025 API exposes the `active` Boolean for ordinary and regional
subscriptions (§§5.10 and 9.10), but those signatures alone do not establish
the complete normative delivery rule. The process declaration test verifies
that `true`, `false`, and unsubscribe reach registry state; it sends no
interaction. The embedded passive-only test covers suppression followed by
activation. A separate embedded regional two-recipient transition case is
written to cover an active recipient making delivery relevant to an overlapping
passive recipient, loss of the sole active declaration, and activation of the
remaining declaration. Its last recorded local run failed during federation
creation before reaching those assertions, so that sequence remains
source-/test-intent evidence, not runtime-verified behavior. Do not infer
process/embedded or 2010 parity.

No additional chart was warranted: the existing interaction flow already
separates federation relevance from per-recipient eligibility. The 2025 guide
and evidence map now call out the process test's state-only scope and the
regional test's runtime caveat. This was a bounded source/documentation review;
no build, test rerun, or CI was performed. Keep the Requirements Lab untouched.

### 2025 save/restore work and callback cut-point audit — 2026-10-11

The bounded cross-guide review found that the federation save/restore guide
already distinguishes federate-owned checkpoints from RTI-owned federation
state, admission from terminal outcome, and still-pending timestamped work from
effects already observed by recipients. Its selected embedded traces show a
saved live TSO message and its retraction state returning after restore. The
TSO guide separately treats queue withdrawal, already-delivered callbacks,
and service-specific retraction effects; it does not collapse them into one
generic undo rule.

The ownership companion supplies a distinct recovery case: the state image
contains typed request/recipient facts rather than old executable callback
closures; the fresh-registry path validates live identities and replans selected
work onto current routes. Focused public tests assert the reconstructed release
or ownership notification after the federation-restored callbacks are observed
under their selected callback model. This is current embedded-profile evidence,
not a portable HLA persistence rule, OS-process-restart proof, or process/
embedded parity claim.

A small navigation gap remained in the main save/restore guide's “HLA
questions” index: it pointed readers to temporal-message examples but not to
the separate pending-ownership case. Added a row linking the companion and
labeling its evidence boundary. No umbrella diagram was added because the
existing ownership and TSO charts answer different service questions and
already expose their distinct callback cut points. No build, test rerun, or CI
was performed; keep 2010 separate and the Requirements Lab untouched.

Next handoff: audit the 2025 FOM module-admission and composition guide as an
HLA-first explanation of the boundary between an input module, the composed
model, and the runtime FDD. Compare its module validation, ordered merge,
reference-resolution, and materialization diagrams with the pinned API,
current embedded paths, and cited tests. Revise the existing guide only for a
demonstrated reader gap; keep implementation-specific preflight behavior and
the separate 2010 stream explicit. Do not build, rerun tests, or start CI for
documentation work by default; leave the Requirements Lab untouched.

### 2025 FOM module-admission and composition audit — 2026-10-11

The existing guide's four diagrams already separate the HLA model-input
question from Umbra's selected Create/Join transactions: individual module
validation, complete-set merge and reference resolution, RTI-facing FDD
materialization, and the distinct 2010 model-compatibility lane. Pinned API
signatures describe one/vector Create inputs and optional Join additions; they
do not specify the implementation's transaction staging. The cited embedded
tests cover representative DIF acceptance, later-module reference resolution,
FDD acceptance/rejection, pre-name Create failure, and failed additional-module
Join preserving the prior definition. These remain named-scenario evidence,
not exhaustive Annex C, process-profile, or conformance proof.

A source-artifact precision gap was worth correcting. The bundled 1516.2-2025
schema headers explicitly distinguish DIF (possibly incomplete OMT exchange),
FDD (the relaxed minimum information required by an RTI), and OMT (stricter
validation that still does not guarantee full conformance). The guide now links
the exact pinned schemas and uses those roles to explain why module DIF checks,
post-merge resolution, and output FDD validation are separate questions. This
is a guide clarification, not a new behavior claim or diagram. No build, test
rerun, or CI was performed. The 2010 lane remains separate and the Requirements
Lab untouched.

### 2025 declaration and attribute-relevance advisories audit — 2026-10-11

The existing guide already separates class declaration relevance,
per-instance attribute relevance, receiver-local scope, discovery, reflection,
and receiver-side update-rate admission. Its state/flow diagrams cover the
reader-facing distinctions, so this audit did not add another diagram. The
pinned 1516.1-2025 API now has a compact cross-reference table for the separate
FederateAmbassador callback families (§§5.14–5.17, 6.19–6.24) and RTIambassador
advisory switches (§§10.34–10.41). Those signatures establish distinct public
surfaces, not the full normative trigger, recipient, or ordering rules.

The guide also avoids conflating Turn Updates On/Off with an actual value
update: the advisory callback contains no attribute values and alone does not
establish that a value was sent or reflected. It points readers to the separate
Auto Provide path, which is the RTI-triggered request-for-values lane.
Existing embedded declaration tests and process-profile attribute-advisory
cases remain evidence for their named scenarios only; they are not treated as
profile parity or standard conformance. No build, test rerun, or CI was
performed. IEEE 1516.1-2010 remains separate and the Requirements Lab untouched.

### 2025 Auto Provide and explicit value-request boundary audit — 2026-10-11

The bounded API/source/test review found the core request/response concept
already explained across two focused guides: Auto Provide has discovery,
provider grouping, a callback-entry fence, and a later provider-response flow;
the separate explicit-request guide owns object/class/regional request scope
and its provider/response sequence. A second combined sequence would repeat
those models, so this increment adds no diagram. Instead, the Auto Provide
guide now maps the public 2025 request (§6.21), provider callback (§6.22), and
update (§6.10) surfaces and cross-links the dedicated explicit-request guide.

The comparison keeps the durable boundary visible: Auto Provide is
RTI-triggered after discovery in the current embedded profile, while explicit
request is federate-invoked; both can reach `provideAttributeValueUpdate`,
which solicits attributes rather than carrying their values. In the named
embedded object-request tests, the caller tag reaches that callback and the
later reflected value uses the provider's separate `updateAttributeValues`
tag. The API also exposes a class-request overload, but the cited request
baseline does not exercise it. The Auto Provide baseline observes its empty
automatic tag; discovery-triggered process behavior remains unverified. These
are bounded profile examples, not a complete normative interpretation or
process/embedded parity claim. The existing diagrams were not changed or
rerendered. No C++ build, test rerun, CI, or Requirements Lab edit was made;
2010 remains separate.

### 2025 object-name, instance-knowledge, and removal boundary follow-up — 2026-10-11

A prior bounded review already found that the reservation, object-information,
and state/event guides cover name reservation, registration, peer discovery,
per-receiver removal, and local forgetting; it correctly rejected an umbrella
diagram. This follow-up found one precise ambiguity in the reservation guide's
`Registered → Available` edge. The existing state diagram now identifies its
endpoint as federation-wide object/name purge and links to the removal and
local-delete explanations. No new diagram was warranted.

The source boundary is explicit: the current embedded `localDeleteObjectInstance`
path clears only the caller's known-object and pending-discovery state, while
global deletion/removal erases the object/name index only when its purge
predicate permits it. The focused local-delete case demonstrates that an
unnamed live object survives and can be rediscovered; it does not directly test
retention/reuse of a named instance's spelling. That named-name distinction is
therefore source-derived, not a test-proven or general normative claim. The
2010 implementation remains separate.

The edit changed only the existing state-transition label and explanatory
links; no C++ build, test rerun, CI, or Requirements Lab change was made. The
state diagram and the previously revised single-name reservation sequence
still need permitted Mermaid/Markdown preview and GitHub review.

Next handoff: use an available local GitHub-compatible Markdown/Mermaid preview
to parse and visually inspect the adjusted `Registered → Available` state
diagram and the pending single-name reservation sequence. Do not install a
project dependency or run C++ builds, tests, or CI for this check. If preview
is unavailable, keep visual status pending and continue the HLA-first survey.

### HLA-first documentation focus reaffirmed — 2026-10-11

The user clarified that this long-running effort should translate the durable
HLA logic and infrastructure concepts uncovered while implementing Umbra into
diagrams and documentation. The aim is not repeated C++ compilation, which
would compete with the main implementation worker. The guide charter now gives
each primary diagram a portability test: express edition-scoped HLA actors,
protocol states/events, participant eligibility, and observable effects
first; keep Umbra registries, queues, callback dispatch, and transport details
in a separately labeled implementation mapping unless they explain a general
protocol invariant. Source-only details remain source-only, not promoted to
portable HLA rules.

The bounded cross-review found the time guide already presents the portable
roles and advance concepts separately from its embedded grant/queue/callback
model, so another umbrella timing chart would duplicate coverage. In the TSO
retraction guide, portable recipient outcomes were explained in prose while the
state diagram focused on embedded callback gates. Added a compact HLA-level
orientation flow before the embedded example. It separates service acceptance,
per-recipient prior observation, and the receiver's responsibility for effects
already observed; it contains no Umbra queue or ledger states. The guide's
existing caveat remains: this is not the complete normative decision table,
and exact conditions/timing still require the licensed clause text.

The new diagram has not yet been parsed or visually reviewed. No C++ build,
test, CI, or Requirements Lab action was taken; the pending reservation diagram
preview also remains tracked.

Next handoff: review the 2025 time-management guide's
“Temporal eligibility and callback observation are separate gates” flow
against the adjacent embedded-profile advance sequence. Classify each edge as
portable HLA behavior or a selected Umbra callback/scheduler boundary, and
clarify the separation only if a reader could mistake implementation timing
for a standard rule. Reuse existing sources and verified tests, avoid a
duplicate overview, preserve the 2010/2025 boundary, and do not build, rerun
tests, or start CI.

### 2025 time-management HLA/profile boundary check — 2026-10-11

The current advance sequence is explicitly scoped to the embedded 2025
profile. The adjacent gate flow already names its portable takeaway—request
acceptance, timestamped-event eligibility, callback invocation, and grant
completion are separate events—and says its exact pending-task and
TSO-before-grant evidence is embedded-profile evidence. Tightened that
explanation to call the chart a layered view and explicitly label the drawn
eligibility-to-grant route as the current embedded path, not a universal
scheduler/queue algorithm. No diagram was duplicated and no new standard rule
was inferred.

The TSO-retraction orientation and reservation charts remain unpreviewed. A
local Mermaid command-line renderer was not found; checking the installed VS
Code CLI failed with a sandbox `EPERM` while it tried to change into its
installation directory and create external log files. Following the guide's
preview policy, did not unlock the desktop, install tools, or route around the
restriction. Markdown fences and the roadmap JSON were checked, but Mermaid
grammar and visual layout remain unverified. No C++ build, test, CI, or
Requirements Lab action was taken.

Next handoff: do a bounded HLA-first reader-gap review of the 2025 DDM/regions
guide, focusing on its distinction between source/receiver scope, receiver
relevance, and the separate time-management gate. Keep the portable DDM model
primary and Umbra routing/state details as cited case-study evidence; revise
only if a real reader gap remains. Reuse tests, keep 2010 separate, and do not
compile, rerun tests, or start CI.

### 2025 DDM handoff revalidation — 2026-10-11

The current DDM guide already expresses the portable flow clearly: source and
receiver region realizations determine regional scope; object-instance
discovery and per-attribute relevance are distinct outcomes; and time
management/callback dispatch remain separate later gates. Object-attribute
and interaction routing have separate diagrams. The exact-touch expansion is
labeled as Umbra policy, not a universal HLA rule. The cited region-lifecycle,
relevance-transition, and process TSO tests match those evidence boundaries;
the source also explicitly keeps the Attribute Scope lane independent from
Attribute Relevance advisories. The guide index and an earlier ledger review
already record this HLA-first finding and no need for another overview, so no
DDM diagram or prose change was warranted. This revalidation found the active
DDM handoff stale relative to the existing backlog.

No C++ build, test, CI, or Requirements Lab action was taken. No Mermaid
preview was attempted for DDM; its outstanding targeted syntax/render checks
remain tracked individually.

Next handoff: review the 2025 save/restore guide's open Join-during-active-
save/restore boundary. Compare its pinned API exception annotations, the
separate embedded/process Join paths, and the focused interlock test. Resolve
the contract only against authorized official IEEE §4.11 text; if that text is
unavailable, preserve the explicit unknown and do not infer whether Join is
allowed, rejected, or how participants change. Revise only if the current
evidence presentation has a reader gap. Reuse evidence and do not build, rerun
tests, or start CI; keep 2010 separate and leave the Requirements Lab alone.

### 2025 Join-during-save/restore boundary recheck — 2026-10-11

Rechecked both pinned 2025 Join overloads, the embedded adapter, process Join
handler, shared registry membership path, and the representative save/restore
interlock test. The API lists `SaveInProgress` and `RestoreInProgress`; the
focused case joins both participants before the save request and does not
exercise Join during either operation. The inspected source paths do not
establish the normative precondition or participant effects. The official
[IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/) provides
purchase and subscription access but does not expose §4.11 text. The existing
guide already makes this evidence boundary explicit, so no content change or
behavior inference was warranted. Leave the rule unresolved until an
authorized copy is available rather than repeating this same discovery pass.

No C++ build, test, CI, or Requirements Lab action was taken. The 2010 stream
remains separate.

Next handoff: review the portable HLA concept framing in the 2025 Order and
Transportation guide. Ensure order, transportation category, receiver
relevance, timestamp eligibility, and callback observation read as distinct
dimensions before implementation-specific control lifecycles. Change the
guide only for a concrete conceptual gap; reuse existing evidence, avoid
builds/test reruns/CI, and keep the 2010 boundary intact.

### 2025 order, transportation, and receiver observation map — 2026-10-11

The guide already separated sender order, transportation category, receiver
relevance, time-management gating, and callback observation in prose. The
bounded review found one reader gap inside “timestamped”: a timestamp-bearing
service overload, the effective sending-order designation, and the receiver's
delivery-order outcome could be read as one choice. Added an HLA-first map
before the Umbra control lifecycle. It keeps recipient eligibility, the
receiver's order outcome, the TSO time-management gate, transportation
metadata, and application callback observation distinct.

The pinned 2025 API's time-bearing attribute/interaction callbacks expose
`sentOrderType` and `receivedOrderType` separately; the cited embedded test
observes `RECEIVE`/`RECEIVE` and an invalid retraction after a timestamped send
with effective `RECEIVE` order. The guide presents that as a bounded case, not
the complete normative mapping. Exact sender-to-receiver rules remain open
until authorized IEEE clause text is available. The 2010 stream is untouched.

No C++ build, test rerun, CI, or Requirements Lab edit was made. The new
Mermaid diagram still needs permitted syntax/render and visual review; no
GitHub rendering is claimed.

### 2025 process endpoint: public designator versus private IDs — 2026-10-11

A bounded cross-read of the process-endpoint and TSO-retraction guides, pinned
2025 API declarations, and existing handle-conversion code found a small but
important vocabulary gap. The public HLA API returns an opaque
`MessageRetractionHandle` from a timestamped send and accepts it in `Retract`;
`LogicalTime` expresses the message's temporal value. In the current process
profile, private `messageId` carries TSO/retraction work through event and ACK
frames, while `requestId` correlates a service request with its response. The
process adapter converts at the API boundary; neither private ID is HLA
vocabulary or an HLA wire requirement.

Added a focused map to the process-endpoint guide. It is a boundary diagram,
not a second TSO state machine: the existing TSO-retraction guide owns
producer legality and recipient outcomes, while this map clarifies which
identifiers belong to the HLA API and which exist only in Umbra's process
protocol. The HLA/API side is portable; process framing and handle conversion
remain profile-specific. No standards behavior or parity claim was added.

No C++ build, test rerun, CI, or Requirements Lab edit was made. The 2010
stream remains separate. The new Mermaid diagram is not yet syntax- or
visually reviewed.

Next handoff: use an available GitHub-compatible Mermaid preview to parse and
visually inspect the new HLA/API-to-process identity map. Leave it pending if
the preview is unavailable; do not install dependencies or run C++ builds,
tests, or CI. Then continue the HLA-first guide survey with a concept that has
not already been reviewed.

### Current diagram inventory and review-state reconciliation — 2026-10-11

A static fence scan of the current worktree found 222 Mermaid blocks across
34 of 41 design Markdown files: 11 in the two 2010 guides, 198 in 30
2025-specific guides, and 13 in the cross-cutting guide and index. The block
types are 123 flowcharts, 75 sequences, and 24 state diagrams. This scan counts
fences only; it does not parse, render, visually inspect, or establish GitHub
compatibility. The prior whole-set grammar pass covered a different 205-block
snapshot, and the process-endpoint identity map remains individually pending
review.

The current cross-cutting guide contains 11 diagrams; its prioritized backlog
row still said ten and has been corrected. The previous 214-block count has no
saved per-file manifest; the six-block difference cannot be attributed to
specific guides or charts from aggregate counts alone. Earlier dated syntax
and visual reviews remain tied to their recorded snapshots. The per-diagram
status for the cross-cutting charts is reconciled below. Chart-by-chart review
status for the remainder of the 222-block inventory is unquantified. The HLA
value path across the FOM, DataElement, and object/interaction guides was
rechecked: the existing diagram already connects declared type,
application-selected encoding, typed value maps, service/callback boundaries,
and receiver decoding without merging handle identity, routing, or transport.
No duplicate overview was warranted.

| Current cross-cutting chart | Dated evidence and current limit |
| --- | --- |
| One behavior, four different questions | The 2026-10-10 whole-document parser report accepted 196 blocks and said it included the guide's then-current eight charts, but preserved no per-chart manifest. This current block cannot be revision-matched to that result; no current visual or GitHub review is recorded. |
| Keep state, pending work, and observation distinct | Same historical aggregate parser evidence only; no block-level or current-revision syntax result, screenshot, or GitHub preview is recorded. |
| A federation model defines vocabulary; services create live state | Same historical aggregate parser evidence only. The 2026-10-11 FOM vocabulary review linked this map and reported no new diagram; it did not establish current parse or render status. |
| A class hierarchy does not imply one inheritance rule | The 2026-10-10 review records a dedicated VS Code 1.141.0 flowchart-grammar pass. Visual layout and GitHub rendering were explicitly unverified; current-source revision identity is not pinned. |
| A support switch has an owner, a lifetime, and a control surface | The 2026-10-10 support-switch review added the map; the 2026-10-11 review added its operation-specific sampling boundary. That record explicitly claims no grammar acceptance or rendering after the revision. |
| An object-attribute value touches several independent boundaries | The historical 214-block guide-index queue named this map for targeted parsing and visual review. That queue has no per-file manifest, so the current block's syntax and render state are unverified. |
| Different protocols have different completion boundaries | The 2026-10-11 review revised the map and added pinned-API clause anchors. Its ledger entry explicitly says no current Mermaid grammar acceptance or rendered readability was established. |
| Participant-set changes are protocol-specific | Historical whole-worktree reports accepted seven then-current cross-cutting charts at 178 blocks and eight at 196 blocks, but did not preserve title-to-block mappings. No current-revision visual or GitHub review is recorded. |
| A service result, committed state, and a report are separate traces | The 2026-10-10 callback/service review examined the distinction conceptually and found no need for another diagram; it is not a chart-specific syntax or render check for this current block. |
| Service contracts—not argument shape—define outcomes | The 2026-10-11 record checked local links and balanced fences for the then-current ten-chart guide, explicitly not Mermaid grammar or rendering, and left this map in the parse/render backlog. |
| When work becomes stale: withdraw, invalidate, preserve, or account | This eleventh block is present in the current fence inventory, but no dated parser, screenshot, or GitHub-render record for it was found. Treat its current syntax and visual status as unverified. |

No current screenshot or GitHub preview is established for any of the 11
current blocks. Aggregate parser success is not failure evidence, but it also
does not certify these current source revisions. Preserve the distinction
between unverified and invalid; only rerun a supported visual review when a
permitted preview route is available.

No C++ source or test was changed or run. No build, test rerun, CI, Requirements
Lab edit, or 2010/2025 behavior merge occurred. The permitted preview route was
unavailable; no alternate browser route was attempted.

### Federation MOM FOM-module request scopes — 2026-10-11

Added a 2025-only conceptual flowchart to the Federation MOM guide distinguishing
the federation-scoped and joined-federate-scoped
`HLArequestFOMmoduleData`/`HLAreportFOMmoduleData` interactions. The same leaf
names are under distinct MIM roots: the federation request indexes the combined
Create/Join module list and has no target-member parameter; the member request
inherits `HLAfederate` and indexes that member's Join-supplied list. The diagram
links the pinned 1516.2 MIM hierarchy and the source's distinct qualified names.
It documents the MIM's naming/scope distinction, not the complete 1516.1
service contract or process-backend parity.

Corrected the evidence mapping: `federation_mom_current_fdd_catch2.cpp` is the
federation-scoped case; `ieee1516_2025_federation_mom_fom_module_data_report_catch2.cpp`
is the member-scoped case and explicitly supplies `HLAfederate`. The member
test selects its own sender as the target, so it does not establish every
cross-member targeting combination. The member snapshot lifecycle remains in
its separate guide.

The static fence/type inventory is now 221 diagrams across 34 Markdown files
(11 2010, 197 2025-specific, 13 cross-cutting/index; 122 flowcharts, 75
sequences, 24 state diagrams). Fence balance and local anchors can be checked
without compiling, but the new chart has not been grammar-parsed, rendered, or
visually reviewed. No C++ build, test rerun, CI, Requirements Lab edit, or
2010/2025 merge was made. Preserve the §4.11 Join-during-save/restore question
as unresolved; the accessible public page does not provide clause text.

Next handoff: if a permitted GitHub-compatible Mermaid preview becomes
available, parse and visually inspect the new MOM scope flowchart and record
its actual result. If not, leave that review pending and continue the HLA-first
survey with a different reader gap; do not infer rendering success from a
static fence scan. Keep the §4.11 question explicitly unresolved unless
authorized clause text is available. No builds, test reruns, CI, 2010/2025
merging, or Requirements Lab changes.

### 2025 ResignAction intent and membership boundary — 2026-10-11

The federation-lifecycle guide already had a source-specific resignation
sequence and a six-row implementation-effect table. Its missing reader aid was
an HLA/API-facing orientation separating the selected `ResignAction` cleanup
intent from RTI acceptance and the successful end of membership. Added one
2025-only flowchart. It groups enum names by requested effect, warns that the
map is not a normative decision table, and points back to the separate
execution-lifetime model.

The diagram is anchored to the pinned 2025 [`ResignAction` enum](../../third_party/ieee1516.1-2025/include/RTI/Enums.h#L34)
and [`resignFederationExecution` declaration](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L250).
Existing focused embedded resignation cases remain scenario evidence; none
was rerun. The source/API links and balanced Mermaid fence were checked, but
the new diagram has not been grammar-parsed, rendered, or visually reviewed.
No C++ build, tests, CI, Requirements Lab edit, or 2010/2025 behavior merge
occurred. Exact normative action guards and effects remain open.

Next handoff: audit the 2010 reference-RTI resignation path using only the
pinned 2010 API, its reference-runtime source, and existing smoke evidence.
Decide whether the guide's existing note about the ignored action argument and
owned-object erasure needs a 2010-only reader aid. Do not transfer 2025 action
effects into the 2010 stream or enable 2010 CI; keep standard claims distinct
from the bounded reference profile.

### 2010 ResignAction evidence boundary — 2026-10-11

The pinned 2010 API enumerates six `ResignAction` values. The generated shell
forwards the supplied enum to `ReferenceRuntime2010::resign`, but the reference
runtime does not inspect it: it erases object records owned by the departing
member, removes that member, and sends no object-removal callback on this path.
The binding-shell smoke invokes resignation only with `NO_ACTION` for its two
members, so it does not demonstrate the other enumerators' behavior.

The existing 2010 session/execution diagram already shows the membership
transition, while the adjacent implementation note explicitly warns against
reading this limited runtime as standard resign semantics. Added a direct link
to the sole smoke call and made its action-coverage limit explicit; a new
diagram would only depict the profile's ignored argument and was not warranted.
No C++ build, test, or 2010 CI was run; no 2025 behavior or Requirements Lab
content was introduced.

Next handoff: survey the 2025 declaration-management guides for a real reader
gap between publish/subscribe declarations, class- and instance-level
advisories, and receiver eligibility. Compare existing diagrams and evidence
before proposing a synthesis; preserve the service-specific state machines and
the 2010 boundary. Do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 Declaration-management boundary survey — 2026-10-11

Reviewed the object/interaction, object-attribute subscription,
relevance-advisory, and Attribute Scope guides as one reader path. The
object/interaction guide asks separate questions about producer declaration,
ownership, receiver qualification, object knowledge, and observation. The
subscription guide says interest is only an input to discovery and value
eligibility. The relevance guide distinguishes publisher/class callbacks,
owner/object/attribute callbacks, receiver scope, discovery, and later value
delivery in its top-level table and subscription-mutation fan-out. The scope
guide independently names four signals and states what each does not imply.

Existing implementation/test maps distinguish embedded declaration scenarios
from selected process-endpoint relevance/scope cases. A new synthesis would
repeat the existing fan-out and risk presenting profile-specific planning as
one standard-mandated pipeline; no new chart or prose was added. No C++ build,
test rerun, CI, 2010/2025 merge, or Requirements Lab edit occurred.

Next handoff: survey the HLA boundary among per-attribute ownership,
`HLAprivilegeToDeleteObject`, `deleteObjectInstance`, and
`localDeleteObjectInstance` in the 2025 ownership, object/interaction, and
federation-lifecycle guides. Decide whether their existing cross-links clearly
teach that these are distinct authorities and effects, or whether a compact
HLA-first map closes a real reader gap. Preserve edition/profile boundaries;
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Ownership, delete privilege, and receiver-local forgetting — 2026-10-11

Compared the three named 2025 guides. Their linked explanations already make
the important non-implications clear: owning an ordinary data attribute is not
delete privilege; federation-wide object deletion is not local forget; local
forget does not delete the shared instance or alter other members' ownership;
and a ResignAction cleanup label does not guarantee admission or success.
The object guide's deletion and local-forget flows plus the ownership guide's
concept table are a better HLA-first explanation than another summary chart.
The lifecycle guide separately marks its action matrix as current implementation
behavior rather than a complete normative decision table. No new diagram or
prose in the focused guides was needed. No source, test, build, CI, 2010 stream,
or Requirements Lab content changed.

Next handoff: survey the 2025 time-management, object/interaction, and
callback/service-ordering guides around timestamped interactions and receive-
order delivery. Check whether the HLA-first timeline separates message
ordering/timestamp, recipient eligibility, time-advance grant, and callback
observation without promoting Umbra's queue/frontier into a standard rule.
Add/refine a diagram only if a concrete reader gap remains; reuse existing
source/test evidence as bounded profile evidence. No builds, test reruns, CI,
2010 merge, or Requirements Lab work.

### 2025 Timestamped delivery and callback-observation boundary — 2026-10-11

Compared the time-management, object/interaction, and callback/service-ordering
guides. They already separate service-defined participant effects, message
order/transport/timestamp, temporal eligibility, and callback-model observation.
The time guide's HLA-first model distinguishes request acceptance, timestamped
event eligibility, callback invocation, and grant completion; it labels
TSO-before-grant callback routing as the current embedded profile. The object
guide links timestamped sends to that focused guide, while the callback guide
warns that local FIFO is not a universal HLA ordering rule. Existing maps form
the needed bridge, so no combined pipeline or extra chart is warranted. No
source, test, build, CI, 2010 stream, or Requirements Lab content changed.

Next handoff: review the 2025 save/restore guide with the time-management and
callback/service-ordering guides. Focus on application-owned versus RTI-owned
saved state, per-federate versus federation-level completion, and callback
observation boundaries. Add or refine a map only for a concrete reader gap;
keep Umbra snapshot/queue mechanics in bounded implementation evidence and do
not infer the unresolved Join-during-progress contract. No builds, test reruns,
CI, 2010 merge, or Requirements Lab work.

### 2025 Save/restore ownership and completion boundary — 2026-10-11

Compared the save/restore, time-management, and callback/service-ordering
guides. The existing responsibility map and separate save/restore flows already
distinguish application-owned from RTI-owned state, local participant reports
from federation outcomes, and request admission from terminal completion.
Timed-save initiation links to the time-management gate rather than duplicating
its grant algorithm. The callback guide separates service-specific completion
from callback dispatch and local FIFO. Existing implementation tables and
focused evidence remain bounded; no new synthesis map or prose is warranted.
The exact Join-during-progress rule remains open pending authorized §4.11 text.
No source, test, build, CI, 2010 stream, or Requirements Lab content changed.

The proposed FOM re-review was already covered by several recent entries in the
guide index and this ledger. A bounded recheck confirms the module/schema,
complete-model composition, FDD, Create, and additional-module Join boundaries
are represented; no new audit entry or duplicate diagram is needed.

Next handoff: visually review the revised “Service contracts—not argument
shape—define outcomes” map in the 2025 cross-cutting guide using a permitted
local Markdown/Mermaid preview. Confirm the portable HLA contract stays primary
and the selected Umbra state-unit examples remain explicitly profile-specific.
Record only the preview actually performed, not GitHub rendering. If readable,
continue to the 2025 TSO producer Retract-boundary map. No builds, test reruns,
CI, 2010 merge, or Requirements Lab work.

### 2025 lifecycle query and connection/membership boundary — 2026-10-11

Added an HLA/API-facing member-list flow to the 2025 lifecycle guide. It shows
one `listFederationExecutionMembers` request producing either the member-list
or missing-execution report, the disconnected `NotConnected` outcome, and
callback-model-dependent application observation. It explicitly says this is
not a standing membership feed and does not claim an exact snapshot instant or
callback order. The existing embedded sequence remains the implementation
trace: current member records are copied before callback submission. Existing
HLA_EVOKED success/missing-execution coverage does not test mutation between
query and evoke or direct IMMEDIATE delivery. The new map still needs Mermaid
grammar/render/visual review; no GitHub rendering is claimed.

A bounded cross-read of the lifecycle state chart, connection/configuration
guide, and pinned API declarations found that another state diagram would be
duplicative. Instead, added a compact API-vocabulary table for Connect, Join,
Resign, and Disconnect. It keeps `NotConnected` distinct from
`FederateNotExecutionMember`, makes no exception-precedence claim where
conditions overlap, and separates Umbra's Disconnect `NotConnected` behavior
from the exceptions declared by the pinned Disconnect interface.

No C++ implementation or tests were inspected for this increment; no build,
test rerun, CI, 2010 change, or Requirements Lab edit occurred. The local
preview remained unavailable, so the new flow's syntax and visual status are
still pending.

Next handoff: survey the 2025 producer-side Retract boundary across the TSO
retraction, time-management, and callback/service-ordering guides. Start with
the public HLA role of the retraction handle and the point at which a queued
timestamped send is still retractable; distinguish that model from
profile-specific producer state and callback observation. Reuse existing
source/test evidence and add or revise a chart only for a concrete reader gap.
Keep the 2010 stream separate; do not build, rerun tests, start CI, or edit the
Requirements Lab. Return to visual review of pending local charts only when a
permitted renderer is available, recording the exact check performed.

### 2025 producer Retract and recipient observation survey — 2026-10-11

The TSO guide already has separate HLA-facing recipient-outcome and
producer-legality maps. It distinguishes a pending recipient copy from an
already-received message and labels Umbra current-time/request-time plus
lookahead comparison as profile evidence rather than a required RTI call
sequence. Exact normative interpretation under §8.22.3 remains open; the guide
directs readers to licensed clause text instead of presenting the implementation
predicate as portable HLA behavior.

Cross-reading the time-management and callback/service-ordering guides found
their queue/grant and callback-admission maps already link the needed boundaries
to the TSO guide. No extra diagram is warranted. Added a direct anchor from the
TSO guide bounded producer list to the time-management guide timestamped-message
section. No C++ implementation or tests were inspected or run; no build, CI,
2010 edit, or Requirements Lab change occurred. No visual rendering was
available or claimed. The official IEEE page indicates subscription access to
the full 2025 standard text, so exact clause interpretation requires authorized
text.

Next handoff: audit the 2025 object-instance name reservation guide against
object/interaction information, name/handle resolution, and federation
lifecycle. Focus on the portable separation among a reservation claim, a live
object instance, owner-only registration, and observer discovery, including
bulk partial-reservation versus release outcomes. Reuse existing API, source,
and focused-test evidence; add a chart only for a real reader gap. Keep 2010
separate, do not run builds/tests/CI, and leave the Requirements Lab untouched.

### 2025 reservation, registration, and peer-knowledge audit — 2026-10-11

The reservation guide already distinguishes the public void service from its
later callback, reservation authority from object registration, and the
owner handle from `discoverObjectInstance` knowledge received by a peer. It has
separate diagrams for bulk reserve versus bulk release, the owner-only
registration gate, and the owner/peer visibility timeline. The name-resolution
guide links lookup to callback-visible, federate-local knowledge; the object
information guide links explicit-name registration to the reservation flow;
the lifecycle guide covers unconsumed reservations on resignation.

The pinned API declarations, embedded registry, and focused cases support the
documented partial-reserve, all-or-nothing release, failed-registration
retention, registration consumption, and queued peer-discovery boundaries.
The two-view synthesis intentionally combines two focused observations, not a
single end-to-end test. Process endpoint cases are separately labeled and do
not establish unrestricted parity. These existing diagrams already cover the
requested reader model; adding another would duplicate them. No files outside
the documentation index/ledger/roadmap were edited, and no tests, builds, or
CI were run. The existing local diagram preview queue remains pending.

### 2025 attribute update request: HLA model before implementation trace — 2026-10-11

Reordered the attribute-value update request guide so its HLA-facing sequence
comes before the detailed embedded provider-planning trace. The first diagram
shows the distinct request, provider callback, later value-bearing update, and
eligible receiver reflection without an Umbra registry or queue. The second
section retains the embedded object/class/regional planning and ownership
recheck behavior, with process-profile observations and untested source-derived
branches explicitly bounded.

Cross-reading the object-information, DDM/regions, ownership, and callback
guides found the adjacent eligibility and observation models already covered;
no additional synthesis diagram was warranted. The API declarations were
checked. Exact §6.21 requester response entitlement remains open pending
authorized standard text. No C++ source/tests were reinspected or run, and no
build, CI, 2010 change, or Requirements Lab edit occurred. Local links resolve;
the revised Mermaid sequence has not been rendered, so visual review remains
pending and GitHub rendering is not claimed.

### 2025 federation synchronization: HLA barrier path and observation — 2026-10-11

Added an HLA-facing flow to the synchronization guide between the API-vocabulary
map and the embedded state model. It connects registration, announcement,
participant success/failure reports, abstract barrier completion,
`federationSynchronized`, and callback-model observation without exposing the
registry's participant sets or queue as HLA protocol. The success-result and
announcement branches are intentionally unordered; participant-set and
completion details remain governed by the service clauses and are not inferred
from Umbra's late-join/resignation behavior.

The callback guide already explains that the embedded point is removed and
notification plans are prepared before per-recipient callback dispatch. The
save/restore guide separately describes federation-level coordination, while
the synchronization guide's saved barrier is explicitly profile evidence with
limited end-to-end coverage. These cross-reads support the added conceptual
bridge; no second implementation diagram was warranted. Pinned API declarations
were checked. No C++ source/tests, builds, test reruns, CI, 2010 edits, or
Requirements Lab changes occurred. `mmdc` is not on PATH; the new diagram has
not been rendered, so visual review remains pending and GitHub rendering is
not claimed. Static inventory: 218 diagrams across 32 files.

### 2025 directed interaction versus DDM routing — 2026-10-11

The directed-interaction guide already makes the HLA/API-facing model explicit:
the sender supplies a target object instance, producer and receiver use
object-class/interaction-class declarations, the receiver selector is
universal or by-ownership, and candidate receivers must know the target. The
object-information guide cross-checks those identities against ordinary
interaction routing and states that the target is not a recipient list.

Found one reader gap in the DDM guide: its “Interactions: explicit and
default-region paths” section and diagrams treated ordinary regional sends but
did not say that directed sends are outside that flow. Added a scope note and
link to the directed target/region explanation, preserving the existing
heading anchor. It deliberately does not assert that directed interactions
have no normative DDM relationship or add a region-overlap gate. The exact
standard relationship remains unverified from licensed text. No new diagram
was needed. Pinned API signatures were checked; no C++ implementation or tests
were inspected or run, and no build, CI, 2010 edit, or Requirements Lab change
occurred.

### 2025 relevance advisory, object information, and DDM cross-read — 2026-10-11

The relevance guide's HLA-facing table already distinguishes class-level
publisher advisories, owner-facing per-instance attribute relevance,
receiver-local scope, discovery, update-rate admission, and reflection. Its
main flow sends a subscription mutation into distinct class, owner, receiver,
and discovery calculations; it explicitly avoids a callback-order guarantee.
The DDM guide owns the geometric scope predicate and discovery/routing
transitions. The object-information guide separates object knowledge, receiver
eligibility, value delivery, and application observation. No missing state
transition warranted another flowchart.

Added a direct link from the relevance guide to the object-information guide's
cross-cutting delivery questions. API callback declarations were checked; no
C++ source/tests, builds, test reruns, or CI were involved. The 2010 stream and
Requirements Lab remain unchanged. Existing Mermaid render review remains
pending; no visual rendering was performed here.

### 2025 Attribute Scope Advisory: recipient state and callback lanes — 2026-10-11

The scope guide already gives a clear receiver/object/attribute state model,
maps `attributesInScope` and `attributesOutOfScope` to the receiving federate,
and distinguishes those callbacks from owner-facing Turn Updates, declaration
relevance, discovery, and reflection. Its flow charts cover committed
subscription/region/association changes, switch behavior, the narrow
final-self-unsubscribe exception, grouping, and callback-time stale-work
suppression. Cross-reading the DDM and relevance guides confirms that region
geometry and owner relevance remain in their own focused explanations; the
object-information guide supplies the broader knowledge/delivery context.
No concrete explanatory gap required another map or prose change.

This was a docs-only cross-read. Existing API/source/test links were reviewed
in context, but no C++ source or test file was inspected or run. No build,
test, CI, 2010, or Requirements Lab work occurred. No diagram was changed or
rendered in this pass.

### 2025 Auto Provide: solicitation versus value delivery — 2026-10-11

The Auto Provide guide already distinguishes the RTI-triggered provider
solicitation from initial reflection, explicit Attribute Value Update
Requests, and later value-bearing updates/reflections. It relates the provider-
specific attribute subset to ownership and receiver scope, without conflating
that work with scope advisories or attribute relevance. Its empty automatic
tag, discovery-triggered planning, and stale-work fences are explicitly
bounded to the current 2025 embedded profile; process behavior and the
combined ordering with initial reflection remain unverified. Object
information, scope, and relevance guides already cover and cross-link the
adjacent HLA concepts. No concrete reader gap required another diagram or
prose change.

This was a bounded docs-only review. No normative standard text, C++ source, or
tests were newly inspected; no build, test rerun, or CI occurred. No 2010 or
Requirements Lab changes were made, and no diagram was changed or rendered.

### 2025 object-attribute subscription state: interest, knowledge, delivery — 2026-10-11

The subscription-state guide already leads with a portable HLA concept map
separating declared receiver interest, object-instance knowledge, and value
reflection. Its specialized links keep DDM geometry, advisories, update-rate
admission, Auto Provide, delayed evaluation, and callback ordering from
collapsing into one universal gate. The active/passive explanation labels the
selector behavior as source-derived Umbra evidence and does not promote it to
a normative statement.

Inspected the pinned API signature, bounded discovery/update-recipient source
paths, and the focused passive-subscription test. The test covers passive-only
suppression and the same receiver's passive-to-active transition, not the
separate-active-member/passive-recipient case. The guide accurately records
that gap and the unresolved process late-subscription initial-value boundary.
No diagram/content change was needed. No test was executed, no build or CI was
run, no 2010 or Requirements Lab files changed, and no licensed standard text
was newly consulted.

### 2010 object information: profile path and empty-set edge — 2026-10-11

The 2010 reference guide already separates HLA service names and authority
from this bounded implementation profile. Its object sequence and state map
explain discovery, the single-owner federation record, per-receiver attribute
filtering, synchronous callbacks, and the profile's unavailable deletion and
silent resignation paths without borrowing 2025 transitions.

Inspected the 2010 runtime's subscription, registration, and update paths plus
the cited binding-shell scenario. The source creates a class subscription
entry before adding attributes; registration checks entry presence, whereas
an update through an empty selected set yields no projected values. Added a
small guide note identifying the empty-set discovery consequence as
source-derived, untested, and not a normative 2010 claim. The cited smoke case
uses a nonempty set. No other guide change or diagram was needed; no tests,
build, or CI were run, and no 2025 or Requirements Lab files were changed.

### 2010 object state and interaction-event boundary — 2026-10-11

The 2010 guide's interaction flow already explains sender publication,
parameter validation, exact-class subscriber selection, inline receive
callbacks, and partial observation if a callback throws. Added a concise
HLA-facing comparison ahead of that flow: object updates identify a registered
instance and attribute map; ordinary interactions identify a class and
parameter map without naming an object instance. The profile-specific row
distinguishes stored object values from direct interaction fan-out. The
existing diagrams cover the detailed paths, so a parallel conceptual
flowchart would have duplicated them.

Inspected the pinned API signatures, reference-runtime object/update and
interaction/send paths, and the successful one-subscriber smoke scenario.
That scenario does not verify exception-driven partial fan-out. No test was
run, no build/CI occurred, and 2025 and Requirements Lab files remain
untouched.

Next handoff: audit the 2010 reference-RTI synchronization-point barrier for
registration, announcement, achievement, and completion against the existing
profile flow and source. Keep callback ordering and retry behavior source-
derived rather than presenting them as normative guarantees; no focused 2010
barrier test was found in the bounded survey. Add or revise a diagram only for
a concrete learner gap, and do not borrow 2025 behavior or run builds, tests,
CI, or Requirements Lab work.

### 2010 synchronization-point barrier: roles, failures, and callback boundary — 2026-10-11

The 2010 guide already had a distinct barrier state flow and a callback-failure
sequence. Expanded the registration flow to show label/participant validation
before point insertion, then the success and announcement callbacks after
insertion. Callback failure returns `RTIinternalError` while retaining the
point; an announcement failure can stop the remaining announcement fan-out.
The pinned API also declares `synchronizationPointRegistrationFailed`, but the
2010 reference source path never invokes it. The guide now labels this as a
source/profile boundary and does not infer the standard's failure-callback
contract. The existing sequence shows completion with an empty failed-to-sync
set and the `successfully=false` no-change branch.

Inspected the 2010 API declarations and bounded registration/achievement
source; a test inventory found no 2010 synchronization-barrier test. The
similarly named Catch2 test is explicitly 2025. No code or test changed, and
no build/test/CI ran. `mmdc` is unavailable, so the revised Mermaid has not
been rendered and GitHub rendering is not claimed. No 2025 or Requirements Lab
content changed.

### 2025 save/restore Join boundary: reader-gap disposition — 2026-10-11

Rechecked the save/restore guide against the pinned Join declarations, the
bounded embedded adapter and registry path, the distinct process-service Join
path, and the existing representative save/restore interlock test. The guide
already makes the essential learner distinction: API exception annotations
do not state the exact admission predicate or membership effects. Its source
claim is bounded to the inspected embedded path; the process profile is not
inferred from it. The test's setup joins both members before starting save, so
it does not answer the in-progress Join question. No new chart or prose was
justified.

The official IEEE product listing offers purchase/subscription access rather
than the governing §4.11 text, and the pinned local package is interface
material rather than the licensed standard. Preserve the normative question
as unresolved. No code, tests, build, CI, 2010 content, or Requirements Lab
content changed.

### 2025 DataElement byte-stream boundary review — 2026-10-11

The 2025 encoding guide already models typed values, type-selected byte order,
composite count/shape/padding, discriminant-driven variants, logical-time
factory delegation, and the boundaries between DataElements, handles, model
inputs, and Umbra process framing. The separate 2010 companion contains only
its own 2010 source and smoke-test observations; sharing an unsigned-octet
primitive is not evidence of wrapper or composite equivalence.

The API exposes `getOctetBoundary`, while the reviewed composite source uses
encoded offsets and lengths to insert padding. Added a short explanation that
this is serialized-octet placement, not C++ object/struct memory alignment,
with direct API and implementation links. The existing focused test already
shows count-prefix alignment and malformed-padding rejection, so no new chart
was justified. No tests/build/CI ran; 2010 content and Requirements Lab remain
untouched.

### 2025 names and handles: identity and scope audit — 2026-10-11

The current guide already gives a cohesive HLA-first distinction among model
symbols in a joined execution, federate names/handles, and object-instance
knowledge local to a federate. It shows class-qualified inherited member lookup
and keeps name-to-handle versus handle-to-name lifetime behavior distinct.
The API's public handle classes are category typed; the guide correctly treats
hashes, serialized handles, private numeric registry values, and normalized
DDM coordinates as different representations.

The reviewed API header documents separate lookup errors and that hashes may
collide. Existing bounded sources and focused-test references cover embedded
model extension, discovery-gated object lookup, process identity lookup,
handle encode/decode, and coordinate normalization. Process-only resignation
and error-precedence observations are explicitly scoped, and 2010 remains a
separate stream. The survey found no learner gap warranting another diagram or
text change. No tests/build/CI ran; no code or Requirements Lab content changed.

### 2025 authorization: gate versus service outcome audit — 2026-10-11

The authorization guide already separates the callback's admission decision
from later HLA service validation, state transition, and caller-visible result.
It names the three Authorizer callback scopes; traces Connect credentials,
RID-selected authorizer construction, resource preparation and local commit;
and places connected Create/Destroy/Join checks before their selected
operation paths. Umbra's reference password authorizer and no-authorizer
credential rules are scoped as implementation behavior. The guide also links
adjacent connection and lifecycle flows rather than combining their states.

The pinned API exception lists and Umbra mappings are compared without
silently broadening the public throw contract. Connect overload `@throws`
annotations remain an unresolved §4.2 question because the authorized full
text is not locally available; IEEE's product page offers purchase and
subscription access, not the clause. The guide records the prior 7/10
focused-case result and that Create/Destroy/Join setup failures did not reach
their target assertions. No test was rerun; process-endpoint parity remains
unestablished. The existing diagrams adequately convey the admission/service
boundary, so no prose or diagram change was needed. No 2010 or Requirements
Lab content changed.

### 2025 Connect ordering: preflight precedence and commit — 2026-10-11

The 2025 connection/configuration guide already separates local ambassador
connection from federation membership and execution lifetime, and HLA-visible
Connect outcomes from Umbra RID/configuration/profile behavior. Its link to
authorization preserves that service's distinct gate. Comparing its first
Connect chart with `connectImpl` found the chart had `AlreadyConnected` before
callback-model validation, while source calls `validateAmbassadorConnectionCallbackModel`
first. Reordered the chart and added the observed precedence as Umbra source
behavior, not a normative exception-priority guarantee.

The focused unsupported-callback-model case and the AlreadyConnected-before-
invalid-settings case exercise their preconditions separately; there is no
combined-invalid test. No test/build/CI ran. `mmdc` is unavailable, so the
revised chart has not had local visual review; GitHub rendering is not claimed.
Other configuration, credential, callback-resource, and disconnect evidence
remains unchanged, and no 2010 content was edited.

Next handoff: audit the 2010 reference guide's “Ambassador connection and
execution membership” section against only its pinned 2010 API and reference
runtime. Compare the portable Connect-versus-Join concept without transferring
2025 callback-model or validation order; keep 2010 profile behavior separate
and do not re-enable 2010 CI. Also close the local visual review for the
revised 2025 Connect flow if an approved preview route is available; otherwise
leave it explicitly pending. Do not edit the Requirements Lab.

### 2010 connection and execution membership: Connect/Disconnect edges — 2026-10-11

Audited the 2010 connection section against the pinned 2010 API, generated
shell, reference runtime, and existing binding-shell smoke only. The guide
keeps ambassador connection, execution existence, and membership as separate
states. Its chart now includes repeat-Connect and invalid-Disconnect outcomes.

The first Connect stores the callback object and model without validating the
model, and the shell ignores `localSettingsDesignator`; the public API still
declares `UnsupportedCallbackModel` and `InvalidLocalSettingsDesignator`.
Existing smoke covers only `HLA_IMMEDIATE`, the default designator, and
`AlreadyConnected` on a repeated call. Unsupported model/settings input and
combined error precedence are therefore source-observed only. Disconnected
Disconnect throwing `RTIinternalError` is explicitly asserted by the smoke
and fits the 2010 exception specification. These are reference-binding
observations, not general HLA requirements.

To keep the two lifecycles legible, the 2010 connection section now has two
separate flowcharts: one for ambassador session state and one for execution
membership/lifetime. Their Join/Resign relationship and exact exception names
remain in the adjacent explanation. Both updated charts were rendered and
screenshot-reviewed in the official Mermaid Live Editor. The revised 2025
Connect-ordering chart was also rendered and inspected there. These are local
preview results, not GitHub-render verification. No `mmdc` package was
installed because the browser preview sufficed; no tests, build, or CI ran.
2010 CI remains disabled and the Requirements Lab was not edited.

Next handoff: boundedly survey the existing 2025 time, ownership, DDM,
synchronization, and save/restore guides for a genuine HLA-first synthesis gap
around request admission, committed protocol state, per-recipient eligibility,
pending effects, and application observation. Add or revise a conceptual map
only if it clarifies cross-service relationships that the focused guides do
not already explain. Keep all 2010 behavior separate, reuse evidence without
rebuilding, and leave the Requirements Lab untouched.

### 2025 cross-service synthesis survey — 2026-10-11

Compared the time-management, ownership, DDM, synchronization, save/restore,
and object/interaction guides against the cross-service reasoning lens. The
existing guides distinguish service admission from protocol progress, per-
receiver eligibility, pending effects, and callback-time application
observation. Their boundaries are already cross-linked in the 2025
cross-cutting guide, so no additional synthesis machine or normative rule is
warranted.

Reduced the model-vocabulary/runtime-state map to improve hierarchy and avoid
encoding service-specific gates as a universal sequence. The prose preserves
the focused facts: publication is a registration precondition in linked 2025
service evidence, registration is not receiver-local discovery, and
interaction delivery does not require object-instance knowledge. A static
audit of the class-hierarchy, participant-set, and stale-work diagrams found
their scope labels and links adequate. Shortened hierarchy-chart nodes and
made the receive-order/profile boundaries and source-versus-test distinction
explicit in prose. Verified referenced section anchors and ran
`git diff --check`. No visual render was performed: browser policy blocked an
external Mermaid URL carrying repository-derived chart text, and Mermaid CLI
is not available from configured Scoop sources. No C++ compilation, tests,
CI, 2010 changes, or Requirements Lab changes.

Next handoff: statically audit cross-cutting diagrams 5–7 and 9–10. Check that
the delivery matrix, attribute-update path, protocol completion, service
result/report distinction, and contract/argument comparison lead with HLA
concepts and keep selected Umbra evidence secondary and bounded. Clarify only
concrete reader gaps; do not add a diagram without one.

### 2025 cross-cutting diagram set: static audit — 2026-10-11

Completed a static content pass across all 11 maps. The delivery axes are
questions rather than a fixed pipeline; update-rate admission is labeled as a
bounded embedded example; completion and participant-set maps keep protocols
separate; and stale-work outcomes remain scoped by operation and recipient
phase. Service-result/report and contract/commit maps keep implementation
observations distinct from portable HLA rules. No additional synthesis map
was warranted.

Shortened the model/runtime and class-hierarchy charts, retaining registration
versus discovery, separate object/interaction paths, inherited lookup, and
receiver-known-class projection. The prose identifies the selected 2025
embedded/receive-order scope and distinguishes test evidence from source-only
behavior. Link anchors, `git diff --check`, and roadmap JSON parsing were
verified. Screenshot review remains pending: external preview would transmit
repository-derived chart text, configured Scoop sources lack Mermaid CLI, and
the local file-panel request was queued without a visible render. No C++
build/tests/CI, 2010 change, or Requirements Lab edit.

Next handoff: compare the current HLA guide portfolio with pinned 2025
API/service categories for any major learner-facing behavior area that is
missing or only explained as Umbra implementation detail. Inspect a bounded
relevant API/docs and source/test slice; add a focused HLA-first guide only if
the reader gap is real. Preserve edition/profile boundaries; defer visual
review until an approved local renderer is available.

### 2025 API-family coverage survey — 2026-10-11

Compared 31 edition-specific flow guides (29 for 2025, two for 2010) with the
seven 2025 `RTIambassador` service-family headings. The existing reader-path
matrix maps every family to focused guides. Targeted RTI Support Services
review found topic paths for names/handles, update-rate queries, order and
transportation, class-available dimensions/ranges, advisories, callback
control, time factories, and support-switch/report behavior. No major family
is unrepresented. This is not proof of method-by-method, overload, exception,
or normative clause completeness; the index says so explicitly.

Next handoff: audit the 2025 RTI Support Services methods one bounded group at
a time against those topic paths. Treat missing literal C++ names as a prompt
to inspect meaning, not as a guide gap by itself. Add HLA-first prose or a
diagram only when the public behavior is not already understandable from the
existing guides. Preserve the 2010 boundary and keep visual verification
pending until an approved local renderer is available.

### 2025 Support query semantics — 2026-10-11

Continued the bounded RTI Support Services review and found two useful
precision improvements rather than a missing service-family guide. The names
and handle guide now names the two object-instance lookup operations in its
index and links the existing configured-process test for owner-side
round-trips and unknown-name/handle errors. Peer discovery timing remains a
separate embedded observation; process lookup evidence does not establish
process discovery parity.

The DDM guide now distinguishes class-available dimensions, the fixed
dimension set selected for a region, and per-dimension region bounds. It names
the corresponding 2025 Support queries, adds the region-dimension read-back to
the conceptual flow, and separates the FOM dimension upper bound from a
region's chosen range. Focused embedded evidence shows the shape is queryable
immediately after region creation, an empty shape is zero-dimensional rather
than a wildcard, and `getRangeBounds` exposes pending values before commit as
well as values after commit. This does not establish process DDM parity.

The normal configured Scoop buckets include `extras`, but their catalog has no
Mermaid CLI manifest; no renderer was installed. External preview remains
avoided because it would transmit repository-derived diagram text. Visual
review is still pending. No C++ build, test, CI, 2010 change, or Requirements
Lab edit was made.

Next handoff: continue the bounded 2025 RTI Support Services audit with the
remaining query/advisory/callback-control concepts. Read each operation for a
distinct HLA-level rule or observable effect and add only the smallest guide
change that closes a real learner gap. Preserve the separate 2010 stream and
2025 embedded/process evidence boundaries; do not build, rerun tests, or start
CI. Revisit local visual review only when an approved renderer is available.

### 2025 temporal-admission and callback-entry composition — 2026-10-11

The bounded RTI Support Services follow-up found that advisory controls,
callback enable/disable and Evoke behavior, asynchronous delivery, and remaining
support-switch semantics already have topic-level guide paths. No additional
method-specific guide was justified. A cross-guide learner gap remained:
asynchronous delivery can make a receive-order route eligible for the callback
dispatcher, but the separate callbacks-enabled switch may still keep that task
from entering application code. Added a focused composition chart to the
cross-cutting guide. It distinguishes the time-state gate, dispatcher gate,
and `HLA_IMMEDIATE` versus `HLA_EVOKED` entry boundary.

Evidence is deliberately layered: `FederateTimeState` and its focused unit
case cover temporal admission; the public asynchronous-delivery case covers
the two callback models; `CallbackDispatcher` and its unit cases cover disabled
queue retention and re-enable behavior. A scan found no focused end-to-end case
that combines deferred asynchronous receive-order work with callbacks disabled.
The new chart is therefore a selected embedded 2025 source/test synthesis,
not a normative ordering claim or process-parity statement. Timestamped work
remains outside this receive-order flow.

A fresh static fence census of the current `docs/design` tree found 220
Mermaid blocks in 32 of 41 Markdown files: 12 in the two 2010 guides, 194 in
28 2025 topic guides, 12 in the 2025 cross-cutting guide, and two in the index.
Types: 125 flowcharts, 73 sequences, and 22 state diagrams. This supersedes the
older 217/218/222-block snapshots in earlier notes; it is a fence/type count,
not a parser, renderer, screenshot, or GitHub-compatibility result. The new
cross-cutting chart remains pending visual review. No C++ build, test, CI,
2010 change, or Requirements Lab edit was made.

Next handoff: do a bounded Object Management cross-guide review of one ordinary
receive-order interaction from publication validation and send acceptance
through receiver projection and callback entry. Compare the documented
embedded live-projection path with the process event-snapshot path, and inspect
the existing source-only send-then-unpublish edge for any learner confusion.
Keep source observations distinct from IEEE requirements, preserve the 2010
boundary, and add or revise a chart only if it closes a real gap. Do not build,
rerun tests, or start CI.

### 2025 ordinary receive-order interaction: admission, projection, and callback entry — 2026-10-11

A bounded cross-read of the Object and Interaction, Delay Subscription
Evaluation, Callback and Service Ordering, and cross-cutting guides found the
main behavior already documented without collapsing the profiles. The
embedded adapter validates membership/publication/class/parameters, copies
caller-owned values, replans before its accepted-send boundary, and submits
routes whose callback closure obtains the receiver's current class/parameter
projection immediately before application entry. The process service uses the
same receive-order planner but materializes the chosen projection into an
event before queueing/pushing it; later publish/subscribe changes mutate live
declarations and do not rewrite that ordinary event. Its pull path dequeues
the stored event and the callback bridge dispatches its stored projection.

The existing source-only send → unpublish edge is already stated plainly: the
embedded ordinary path rechecks receiver eligibility, not current sender
publication; the process path retains its earlier event snapshot. This is not
the directed-interaction rule, whose queued route has a separate sender-pair
fence. No focused ordinary send → unpublish → evoke case, post-send process
declaration-mutation case, or combined async-delivery/callback-disable case
was found in the reviewed evidence. Existing process tests establish an
exact-class event payload; embedded report tests establish selected send/report
boundaries, not these declaration-change cases.

The only reader gap found was that the embedded fan-out chart's
`HLA_IMMEDIATE`/`HLA_EVOKED` branch could be mistaken for an unconditional
callback-entry rule. Added a prose qualification and link to the cross-cutting
map: the dispatch model does not bypass the independent receive-order temporal
admission and callbacks-enabled gates. No new chart was justified because the
existing send, unpublication, process-snapshot, and cross-cutting diagrams
already cover the transitions. The source/profile comparison is not a
normative equivalence claim. No build, test, CI, 2010 change, or Requirements
Lab edit was made.

Next handoff: conduct a bounded 2025 callback-exception reader review across
the callback/service-ordering, exception-reporting, and service-invocation-
reporting guides. Check whether the `FederateInternalError` declaration,
HLA_IMMEDIATE versus HLA_EVOKED call-stack boundary, accepted-send
report/counter boundary, and process response/event path are clearly separated.
Verify current source and existing test evidence without rerunning tests; add
a clarification or diagram only for a demonstrated reader gap. Keep normative
claims and embedded/process behavior distinct. Do not build, rerun tests, start
CI, or edit the Requirements Lab.

### 2025 callback exception attribution across embedded and process paths — 2026-10-11

A bounded review of the callback/service-ordering, exception-reporting,
service-invocation-reporting, and process request/callback guides found the
generic dispatcher exception map and the embedded `Send Interaction` example
already explain task consumption, immediate versus evoked unwinding, the
post-acceptance report/counter boundary, and the limits of `HLAreportException`
attribution. Source confirms `CallbackSession` decrements in-flight state and
rethrows; `CallbackDispatcher` consumes the failed task, releases the
immediate-drainer election, and preserves later FIFO work.

The process guide already shows that a recipient client buffers unsolicited
event frames while its correlated request awaits a matching response, releases
the stream lock, then dispatches buffered events. The source shows that an
immediate user exception can therefore escape through that recipient-side
request/pump caller, not across the process session into the producer's
original `Send Interaction` C++ stack; an evoked exception exits through the
recipient's Evoke call. The enclosing recipient-side API/report policy depends
on which request or pump admitted the event, so no general exception-report
rule is inferred. Added this clarification and a direct cross-link to the
process request/callback diagram; no duplicate Mermaid chart was warranted.

The pinned 2025 `receiveInteraction` declaration names `FederateInternalError`,
but no throwing callback integration case was identified in the bounded test
scan. Existing dispatcher and process-delivery cases cover successful paths;
they do not establish callback-exception attribution, report-sink outcome, or
accepted-state rollback. The new prose is source-derived, not normative
behavior or embedded/process equivalence. No build, test, CI, 2010 change, or
Requirements Lab edit was made.

Next handoff: review the process TSO receiver callback-exception boundary
across the TSO retraction, process endpoint, and callback guides. Verify whether
a throwing callback skips the private in-transit completion acknowledgement,
what coordinator/retraction state remains, and which consequences current
source versus tests establish. Keep the private acknowledgement distinct from
the HLA callback and public service result; do not infer 2010 or embedded
parity. Do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 process TSO callback exception and completion fence — 2026-10-11

The process TSO diagram previously ended at the successful callback return,
leaving the failure consequence hidden. Source inspection confirms that the
bridge sets its private `callbackStarted` marker before invoking
`FederateAmbassador`, then marks delivery acknowledged and calls the private
completion handler only after `CallbackSession::invoke` returns. A user throw
therefore unwinds past completion. The dispatcher has already removed the
task; the immediate path releases its drainer election and rethrows, while the
evoked path also propagates the exception without putting the task back.

The two private state layers should not be collapsed: the recipient bridge
retains a callback-started, not-yet-acknowledged retraction state, while the
coordinator retains the recipient message as `inTransit`. The latter remains
visible to time-bound and grant-frontier calculations and can constrain
advancement. It blocks timed-save readiness when the in-transit timestamp is
at or before the pending save time. Recipient unregister discards queued,
in-transit, and delivered TSO entries. If a later process Request Retraction
arrives, the bridge follows its callback-started route; that notification does
not itself send the missing TSO
completion ACK. These are source-derived observations, not HLA-named states.

The existing process TSO integration cases establish ordinary post-callback
ACK ordering and pre-start retraction suppression. No focused integration case
throws from a TSO `FederateAmbassador` callback or verifies the resulting fence,
subsequent time advancement, save behavior, or cleanup. The diagram now makes
the failure branch visible and the TSO-retraction guide points to it. A push-
mode client has received the correlated response before dispatch; an immediate
callback exception may escape the recipient-side request/pump before that
client method returns the response, which is distinct from a server-side
rejection of the originating service. No general wrapper/report outcome is
claimed.

No C++ build, test rerun, or CI was run. No 2010 or embedded behavior is
inferred, and the Requirements Lab was not edited.

Next handoff: inspect process resign/disconnect after a recipient TSO callback
exception. Determine whether endpoint state remains usable, how membership
teardown clears the coordinator in-transit fence and private retraction route,
and what existing focused tests establish. Update the flow only if a reader
gap remains; keep 2025 process, 2025 embedded, and 2010 boundaries explicit.

### 2025 process TSO recipient resignation and cached retraction route — 2026-10-11

The process client does not close itself when a pushed callback exception is
re-thrown: the correlated response was received, request depth is decremented,
and the client retains its session and callback bridge. A later successful
Resign can therefore be attempted after the callback unwinds. The public
Disconnect path rejects a still-joined ambassador, so Resign must succeed
first. Process Resign unregisters the recipient from the time coordinator,
which discards queued, in-transit, and delivered TSO phases, re-evaluates
pending grants, clears that service session's event buffers, and resets the
client's joined identity. It does not close the process connection or callback
bridge; a subsequent public Disconnect closes the client side.

The private retraction route is not the coordinator TSO phase. Source shows
that the service's cached `pendingPushedRetractionRecipients_` entries are
pruned on `detach(session)`. Successful Resign removes producer-owned message
entries, but does not visibly prune a departing receiver's session from all
cached recipient lists. The registry filters nonmembers when assembling its
Request Retraction notifications, but `handleRetract` separately drains the
cached process recipient list and sends event frames to those sessions. Since
the resigned client remains connected and its callback bridge remains open,
current source appears capable of routing a later `request_retraction` frame
to that former member before server detach. No focused TSO process test covers
callback throw → receiver Resign → producer Retract or proves whether a public
Disconnect triggers service detach immediately. Treat this as an unresolved
process implementation risk, not verified runtime behavior or an HLA rule.

The process guide now separates coordinator cleanup from private route cleanup
and marks the latter as an evidence gap. The TSO retraction table now qualifies
which process recipients are selected and links to that caveat. The Mermaid
branch added in the preceding increment remains unrendered because the
configured Scoop buckets have no `mmdc` manifest. No code,
Requirements Lab, 2010 stream, build, test rerun, or CI was changed.

Next handoff: boundedly review the pinned 2025 public API/materials and existing
evidence for receiver resignation versus a later Request Retraction. Determine
what is normative, what is only the process service's cached-route behavior,
and whether the current source discrepancy remains an implementation concern;
do not add a conformance claim without authoritative clause evidence. Then
continue the guide survey with a separate major HLA topic. Do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 Resign versus Request Retraction: public-material and test boundary — 2026-10-11

The [official IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
identifies the active 2025 edition, says it supersedes 1516.1-2010, and links
the public machine-readable API bundle. That bundle supplies C++/Java API
headers, protobuf service shapes, and section identifiers: `resignFederationExecution`
maps to §4.12 and the `requestRetraction` callback to §8.23. It does not include
the normative clause prose needed to decide whether a receiver that has
Resigned remains eligible for a later Request Retraction. No such HLA rule is
inferred from the API mapping.

The existing case
[`Embedded public fresh-registry restore keeps an ownership-qualified directed TSO route after a peer's post-delivery resignation under HLA_EVOKED and HLA_IMMEDIATE`](../../cpp/tests/ieee1516_2025_public_directed_tso_post_delivery_resignation_restore_catch2.cpp#L6)
is classified `implemented-in-noninstallable-development-profile` and maps to
2025 clauses 4.12 and 8.23.3 among its broader service set. Its source exercises
both callback models: one peer receives a directed TSO callback, Resigns and
Disconnects, while a different ownership-qualified recipient's queued copy
survives save/restore; a later Retract produces one valid Request Retraction
for that still-joined recipient. This establishes only that bounded embedded
scenario. It neither asserts notification to the departed peer nor covers the
process endpoint, and this documentation pass did not rerun the case.

The process source concern remains separate. `handleResign` does not visibly
remove every departed receiver from the cached pushed-retraction recipient
lists; `handleRetract` drains that cache independently of the registry's
current-member notification list. After callback failure, the process bridge
retains callback-started state, and `submitRequestRetraction` queues on that
state without its own membership check. This is a plausible source path, not
verified wire delivery or callback execution. No code, test, Requirements Lab,
2010 stream, build, or CI changed; no diagram changed or was rendered.

Next handoff: review the 2025 cross-cutting guide's temporal-admission and
callback-entry synthesis for a reader-facing HLA explanation. Keep admission,
committed protocol state, callback eligibility, and application observation
distinct; reuse existing source and focused-test evidence, and only change the
diagram if it improves that conceptual boundary. Keep 2010 separate and do not
build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Join during Save/Restore: public-material and evidence boundary — 2026-10-11

The Save/Restore guide now records the Join question as a separate membership
admission boundary. The pinned 2025 `joinFederationExecution` overloads list
`SaveInProgress` and `RestoreInProgress` in their exception annotations (§4.11),
but the public IEEE bundle contains API headers and schemas rather than the
normative clause text. The exact preconditions, timing, and participant effects
remain unresolved; neither an allow nor reject rule is inferred from the list.

The bounded embedded source review found Join proceeding through connection,
membership, and model checks without an explicit save/restore-phase predicate
in the inspected adapter and registry sections. The process profile enters via
a separate handler, so the embedded trace is not transferred to it. The
representative interlock test joins both participants before save and does not
exercise in-flight Join. These findings do not establish behavior for every
interleaving or any normative contract. No Join state-transition diagram was
added because its decisive branch and participant-set effects are not supported
by the available normative evidence.

The guide links the checked [official IEEE 1516.1-2025 product and download
page](https://standards.ieee.org/ieee/1516.1/6688/) and identifies the public
bundle's contents and limits. No source, test, 2010 stream, Requirements Lab,
build, test rerun, or CI changed in this pass; the existing diagram was not
altered or rendered.

### 2025 temporal admission and callback entry: synthesis review — 2026-10-11

The cross-cutting chart connects two boundaries already explained separately
in the time-management and callback/service-ordering guides. A receive-order
route can leave the deferred time-state queue while asynchronous delivery is
enabled or an advance is pending, then remain in the callback dispatcher if
callbacks are disabled. The chart is therefore a useful cross-guide synthesis,
not a duplicate universal HLA state machine. Its evidence is explicitly the
selected embedded 2025 path; no 2010 or process parity is inferred.

The reader review sharpened three boundaries: matching grant clears the
pending-advance condition, so later receive-order routes are deferred again
when asynchronous delivery remains off; this does not itself withdraw tasks
already submitted to the dispatcher; and the focused integration case observes
the admitted receive-order interaction before any Time Advance Grant—during
the request-service path in HLA_IMMEDIATE and on the following Evoke in
HLA_EVOKED. The separate callback guide retains the cross-thread immediate-
drainer qualification. The existing time-management sequence is linked rather
than duplicated. No focused test combines temporal deferral with callbacks
disabled, so that combined trace remains source-composed, not end-to-end tested.

Updated the synthesis explanation and its source/test links, and clarified the
backlog's review status. The configured Scoop buckets have no Mermaid CLI
manifest, and no alternate package was installed. Mermaid grammar, rendered
layout, visual inspection, and GitHub preview therefore remain unverified for
this revised chart. No build, test rerun, CI, C++ source, 2010 stream, or
Requirements Lab change.

### 2025 ordinary receive-order interaction: live view versus event snapshot — 2026-10-11

A bounded review of the object/interaction and process-endpoint guides found
the key profile distinction already explained in adjacent, explicitly labeled
sections. Both paths use the shared send planner and check the sender's current
publication before accepting the operation. In the embedded path, the queued
route obtains the receiver's current eligibility and parameter projection at
callback time; the inspected predicate does not recheck the sender's current
publication. In the process path, the send service materializes the selected
class and visible parameter handles into an event; later declaration changes
do not rewrite that stored projection.

The accepted-send then unpublish then evoke ordering has no focused ordinary
interaction regression in the reviewed evidence. The embedded consequence is
source-only; the process public-send test inspects one exact-class event and
does not test a declaration change between send and receive. The guides label
these separately, identify the process snapshot versus embedded live view, and
state the missing test and normative limits. No further comparison chart was
warranted: it would repeat those existing diagrams and their connective
prose. No 2010 inference or cross-profile HLA rule is made.

No C++ source, build, test rerun, CI, 2010 stream, or Requirements Lab changed
in this documentation review. The substantive comparison is recorded here;
the focused guides remain the implementation evidence map.

Next handoff: do a bounded review of the 2025 Federation MOM and
Joined-Federate MOM guides as one HLA ownership question: what is execution-
scoped versus member-scoped, and how do Join/Resign change those objects and
their application-visible reports? Compare the public API, embedded registry,
process path, and existing focused tests. Add or revise a diagram only if the
current guides leave that lifecycle distinction unclear. Keep 2010 separate;
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Federation MOM and Joined-Federate MOM: process-boundary review — 2026-10-11

The pinned MIM cleanly distinguishes one execution-scoped `HLAfederation`
instance from one `HLAfederate` instance per joined member. The existing
guides' separate scope explanations and lifecycle diagrams already make the
Join/Resign distinction legible, so this review added no third, overlapping
diagram.

The process handler is separately source-traced: it establishes the shared
Federation MOM record, creates per-Join report-file and `HLAfederate` state,
commits the session, and then schedules the member-object discovery path. The
shared registry's resignation ledger retains the execution object while
planning removal of a resigning member's object. Two public process cases
verify bounded member-object observations: old-object removal before
rejoined-object discovery/reflection, and a new report-file path on rejoin
while preserving the prior report. The reviewed process tests do not directly
observe process `HLAfederation` discovery or its `HLAfederatesInFederation`
reflection on Join/Resign. That remains source evidence, not a process
public-callback result; embedded membership-reflection evidence is not
substituted. No profile parity or 2010 behavior is inferred.

The joined-federate guide now has a separately labeled process evidence
section and implementation/test links. The federation guide states the same
process observation gap at its scope and implementation boundaries. Scoop's
configured buckets had no `mmdc`/Mermaid CLI manifest; no chart changed, so no
renderer installation or diagram render was needed. No C++ source, build,
test rerun, CI, 2010 stream, or Requirements Lab changed.

Next handoff: review the 2025 object-discovery and attribute-value boundary
across the Object/Interaction Information, Object-Attribute Subscription
State, and Joined-Federate MOM guides. Keep declaration eligibility, discovery
delivery, value-update eligibility, and callback observation distinct; map
claims to the pinned API/MIM, source, and existing focused tests. Add a
cross-guide map only if it reveals a reader gap not already served by the
separate flows. Preserve 2010 and 2025 embedded/process boundaries; do not
build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 object discovery and attribute values: three-guide review — 2026-10-11

The reviewed guides already give readers separate, HLA-first views for (1)
publisher/subscriber declarations and per-receiver object knowledge, (2)
per-attribute active/passive interest and discovery/value eligibility, and
(3) the RTI-owned joined-federate MOM snapshot, public discovery, and requested
or periodic values. Their maps distinguish declaration, discovery, attribute
projection, provider callback, and application callback; another umbrella map
would repeat rather than clarify those boundaries, so none was added.

The bounded process-source/test comparison found one useful cross-guide
qualification. Process ordinary late-subscription discovery has no ordinary
initial-reflection planner call in the inspected path, and its result remains
unresolved. A configured process test subscribes after RTI-owned MOM objects
exist, observes two discoveries and two selected-value reflections, and checks
the subject's federate name and report-file path. This verifies only that
bounded MOM-specific initial-value branch; it does not settle ordinary object
behavior. The subscription-state and
joined-federate MOM guides now cross-link this positive test and the remaining
ordinary-object evidence gap. The broader object-information guide already
explains that explicit Request Attribute Value Update is a provider callback
followed by a distinct provider update, not a value response returned by the
request service.

The existing focused tests and source were read only; no build, test rerun,
CI, Requirements Lab, or 2010 change was made. No Mermaid chart changed or
needed rendering.

Next handoff: compare three ways a receiver can get current object values in
2025: direct late-discovery initial reflection, RTI Auto Provide solicitation
followed by a provider update, and explicit Request Attribute Value Update
followed by a provider update. Reconcile the Auto Provide, object-information,
explicit value-request, and Joined-Federate MOM guides against the pinned API/
MIM, source, and focused tests. Show which actor supplies the bytes and which
callback the receiver observes; synthesize a map only if the existing flows
leave that choice unclear. Keep process and embedded evidence separate, keep
2010 out of this flow, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 current-value supply paths: initial, automatic, and requested — 2026-10-11

The four relevant guides already separate the core HLA paths: existing RTI
state can be reflected with discovery; Auto Provide is an RTI-triggered
provider solicitation whose callback carries no values; an explicit
Request Attribute Value Update is caller-triggered and normally solicits
providers; and a later provider update is a separate value-bearing service
that still faces receiver eligibility and time rules. The Auto Provide guide's
comparison table was sharpened to make the joined-federate MOM exception
visible: an explicit request for an RTI-owned `HLAfederate` instance directly
reflects RTI-held values rather than invoking a provider callback.

The process/embedded review supports only bounded statements. Embedded
ordinary initial reflection precedes Auto Provide planning in source, but the
combined sequence is not focused-test-observed. The process endpoint has a
distinct joined-federate MOM initial-value branch and direct MOM request
branch; the report-file case observes selected `HLAfederateName` and
`HLAreportServiceFile` reflections and later a requested report path. Process
Auto Provide remains unverified; the source survey found no call from the
process discovery path to the discovery-triggered Auto Provide planner. No
ordinary-object process late-subscription initial-value result is inferred.
The existing causal diagrams plus comparison table explain these mechanisms,
so no extra synthesis diagram was warranted.

Cross-linked the MOM-special process paths in the attribute-subscription,
Auto Provide, and explicit request guides. Pinned API/MIM references and
existing focused-test/source links remain separate from normative claims. No
build, test rerun, CI, 2010 stream, or Requirements Lab work was performed.

### 2025 execution, membership, and MOM lifetimes — 2026-10-11

Reviewed the lifecycle guide beside the federation-scoped and joined-federate
MOM guides. The MIM's one-`HLAfederation`-per-execution and
one-`HLAfederate`-per-member cardinalities were previously documented in
separate flows; the lifecycle guide now connects them in a scoped Join/Resign
overlay. It shows that Create does not join, the execution-scoped projection
is ensured during non-legacy 2025 Join setup, and each production-backed
membership gets its own report-file location and member projection. Resign
retires the member object while pending callback-time removal may retain its
private snapshot; this does not make the federation-scoped object per-member.

The evidence remains profile-bounded. Embedded source shows setup rollback
can leave the execution-scoped MOM record, but no focused failed-first-Join
case exercises that edge. An embedded test observes conditional
`HLAfederatesInFederation` reflections at membership boundaries. The process
report-file case observes a fresh member object and distinct report path on
rejoin while preserving the previous file; process `HLAfederation` discovery
and membership reflections remain unobserved. Test-only in-memory stores and
the 2010 compatibility guard remain explicit exceptions; no 2010 MOM behavior
was inferred. The new Mermaid overview was not locally rendered: Scoop's
configured search had no `mmdc` package match and DNS resolution failed. No
C++ source, build, test rerun, CI, 2010 stream, or Requirements Lab changed.

### 2025 connection, membership, and execution service preconditions — 2026-10-11

Compared the pinned 2025 declarations and bounded implementation/test paths
for Connect, Disconnect, Create, Destroy, Join, Resign, and member listing.
Expanded the lifecycle guide's state-exception table to include all seven
operations and distinguish the local connection, an execution's existence
and membership count, the caller's membership, and the callback-based query
result. The connection/configuration guide now links back to that comparison.

The evidence does not support treating the table as a complete conformance or
test matrix. The embedded member-list helper checks `NotConnected`, then
requests a snapshot from a connected unjoined observer and checks the
missing-execution callback. The selected lifecycle test covers duplicate
Create/Join, Destroy while members remain, and successful Destroy; it does not
exercise disconnected Create/Destroy. The pinned Disconnect declaration
lists `FederateIsExecutionMember`, while Umbra additionally reports
`NotConnected` for an absent connection. Existing local-state and member-list
diagrams already show the transitions and callback path, so this increment
adds a cross-service table and cross-link rather than another chart. The
previous lifecycle-to-MOM chart still awaits local visual review. No C++
source, build, test rerun, CI, 2010 stream, or Requirements Lab changed.

### 2025 Support queries, advisories, and callback controls — 2026-10-11

Compared the remaining 2025 RTI Support query, advisory, and callback-control
declarations with the existing topic guides. Dimension/range reads, typed
handle normalization, advisory scopes and switches, and callback-model plus
enable/disable/evoke controls already have reader paths. The comparison found
one taxonomy gap in the cross-cutting switch map: it did not distinguish the
four getter-only federation switches whose MIM values are Static from the
federation-wide Auto Provide value, whose MIM update type is Conditional and
which Umbra's embedded federation MOM path can change. The control-scope table
now makes that difference explicit and links the existing Auto Provide and
MOM guides. This does not promote Umbra's mutation path to process or 2010
behavior, and no full normative or method/exception audit is claimed.

No new diagram was warranted: the existing switch map plus a scoped table
express the control-path distinction without adding a second flow. No diagram
was changed, so local `mmdc` rendering was not needed; the optional Scoop
search could not resolve its configured hosts and installed nothing. No C++
source, build, test rerun, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 Ownership completion signals — 2026-10-11

Compared the pinned 2025 Ownership Management surface with the attribute-
ownership and persistence guides. A focused embedded cancellation test showed
that `cancelAttributeOwnershipAcquisition` returning is not the terminal
confirmation: the publication fence and an If-Available rejection remain
until `confirmAttributeOwnershipAcquisitionCancellation` begins; deletion can
invalidate a still-queued confirmation. A second embedded test exposed a
different completion channel: `attributeOwnershipDivestitureIfWanted` fills
`divestedAttributes` and commits ownership before the requester later
observes its acquisition-notification callback. The callback carries the
divestiture tag, and stale owner-release work is suppressed. The guide now
documents both distinctions, updates the per-attribute request flow, and
links the exact API/callback declarations and focused tests. No normative
arbitration rule or process/2010 parity is inferred.

The prior state diagram's callbacks and multiple return edges overlapped in
rendering. It is now a left-to-right request/cancellation flow; the new
Divestiture If Wanted sequence also rendered and was visually reviewed with a
temporary local Mermaid CLI install. Scoop had no configured manifest for
`mmdc`; the temporary CLI and previews were kept outside the repository. No
C++ source, build, test rerun, CI, 2010 stream, or Requirements Lab work.

Next handoff: do a bounded 2025 Object Management lifetime survey against the
pinned API and the object/interaction, attribute-ownership, and lifecycle
guides. Focus on the distinction among federation-wide delete, caller-local
forget, delete privilege, and receiver removal observation; add only a
demonstrated reader gap. Keep 2010 separate and preserve embedded/process
evidence boundaries. Do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Delete-privilege publication epoch — 2026-10-11

The queued 2025 Object Management survey found the existing guides already
explain the separate transitions for federation-wide delete, local forget,
delete privilege, and receiver-side removal. A more specific implementation
and test review exposed an uncovered HLA nuance: the special
`HLAprivilegeToDeleteObject` instance attribute uses an implicit class
publication epoch, and explicitly unpublishing it changes the privilege of
existing instances. Republish affects later registrations without
retroactively restoring the earlier instance.

The object/interaction guide now pairs this concept with one focused sequence
from the existing 31-assertion `DELETE_OBJECTS_THEN_DIVEST` embedded scenario:
the later object is removed, the earlier object remains and divests its
ordinary `Efficiency` attribute, and this test's callback order is labeled
profile-specific. Evidence links include the 2025 MIM, the publication
projection and unpublish source paths, and the test. The test plan maps the
voluntary ResignAction to §4.12; no full §5.1.2 predicate or process parity
is claimed. All 18 diagrams in the updated guide rendered; the new one was
visually inspected. No code, builds, test reruns, CI, 2010, or Requirements
Lab changes.

### 2025 Join-during-save/restore audit and synchronization-map visual review — 2026-10-11

The bounded Join audit confirmed an evidence gap, not a resolved runtime or
conformance result. Both pinned 2025 Join overloads list `SaveInProgress` and
`RestoreInProgress`. The inspected embedded Join transaction and process Join
handler do not show an explicit active-save/restore admission predicate or
update to the operation's participant-status cohort; the existing mapped
save/restore interlock case joins its participants before requesting save and
does not attempt a Join during either protocol. These are bounded source/test
observations, not proof that all paths lack a gate or that a late Join is
accepted. The exact §4.11 preconditions and participant effects remain
unresolved because the pinned checkout does not include authorized normative
text. The save/restore guide already records that boundary, so no new Join
diagram is justified.

The visual pass found a clarity issue in the HLA-level synchronization map:
successful registration was drawn as two parallel `Yes` outcomes, making the
registrar result and participant announcements look mutually exclusive. The
diagram now shows registration accepted before the two distinct notification
paths, without imposing callback order. All seven diagrams in the
synchronization guide rendered with the temporary local Mermaid CLI; the
revised HLA-level map was visually inspected. It remains a conceptual API
event map, not a claim about exact normative callback ordering. No C++ source,
build, test rerun, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 Lifecycle ResignAction intent and member-list visual review — 2026-10-11

The local render exposed a readability and sequencing issue in the 2025
ResignAction intent map: six peer branches made the enum labels too small to
scan, and the flow placed the service call before selection of its argument.
The map now presents the six API values and their requested cleanup intent in
one readable vertical choice block, then shows the service call, abstract
acceptance/rejection decision, successful membership end, and separate
execution lifetime. It does not turn the embedded action matrix into a
normative decision table or claim that a requested cleanup always completes.
The existing prose keeps exact guards and effects bounded to available
standard/profile evidence.

All 14 diagrams in the lifecycle guide rendered locally. I visually reviewed
the revised ResignAction map and the adjacent member-list snapshot flow; both
are legible, and the snapshot flow's API callback outcomes remain distinct
from its embedded connected/unjoined caller evidence. No content change was
needed for that second map. No C++ source, build, test rerun, CI, 2010 stream,
or Requirements Lab work occurred.

### 2025 federation MOM, update-request, and lifecycle visual review — 2026-10-11

The queued Federation MOM review found the conditional-update trigger map was
too wide: four producer/field branches compressed its labels. Replaced that
fan-out with a four-row representative embedded-producer-to-value table and a
compact vertical pipeline from authoritative snapshot update to reflection
planning and callback-model dispatch. The table remains profile-bounded; the
MIM's Conditional metadata and the selected tests are linked separately. All
seven diagrams in the federation MOM guide rendered, and the revised pipeline
was visually inspected.

The four diagrams in the attribute-value update-request guide also rendered.
The HLA-facing sequence had a Mermaid parse failure around nested annotations
and was visually crowded; it now uses concise participant and event labels,
without auto-numbering or note boxes that made a causal model look more
prescriptive. Its surrounding prose still distinguishes solicitation from a
value-bearing update, request and response tags, callback timing, eligibility,
and timestamped delivery. The final sequence was visually inspected. All 14
diagrams in the lifecycle guide rendered; its revised ResignAction map, the
member-list snapshot flow, and the federation/member MOM-lifetime overlay were
visually reviewed. Only the ResignAction map needed adjustment. These are
local preview checks, not GitHub-render claims. No C++ source, build, test
rerun, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 name identity and temporal-admission chart review — 2026-10-11

Rendered and visually inspected the federate name/handle identity map in the
2025 names-and-handles guide. Its 10.2 active-name and 10.3 known-handle lookup
flows remain visually distinct from the configured-process test's post-
resignation observation. The chart is tall but readable; the surrounding text
continues to limit that asymmetry to the cited process scenario. No embedded
profile or 2010 parity is inferred, and no guide edit was needed for this map.

Rendered the cross-cutting guide's temporal-admission/callback-entry synthesis.
The Mermaid CLI found unquoted parenthetical node labels after HTML line breaks;
quoting those labels fixed parsing. Visual review confirmed the temporal queue,
callback-dispatcher pending state, and callback-model branch remain legible.
The `HLA_IMMEDIATE` endpoint now states callback entry explicitly. This is a
selected embedded source/test synthesis: there is no combined focused test for
deferred asynchronous receive-order work while callbacks are disabled, and no
normative ordering or process-parity claim is made. No C++ source, build, test
rerun, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 API value-map and callback path visual review — 2026-10-11

The local Mermaid render of the HLA/API value-path map was readable but visually
collapsed object updates and interaction sends through one generic RTI node.
Reworked it so `updateAttributeValues` continues to `reflectAttributeValues`
under the object-update path, while `sendInteraction` continues to
`receiveInteraction` under the interaction path. The branches rejoin only for
the receiver's type-compatible `DataElement` decode. The map now foregrounds
the HLA distinction while retaining the 2025 C++ API's typed maps and
`VariableLengthData` representation; it does not merge the delivery state
machines or prescribe private process framing.

The pinned 2025 `DataElement` interface, handle/value map typedefs, service
signatures, and callback signatures support the described API boundary. Added
separate test pointers for exact basic-element bytes/decode, an embedded
`receiveInteraction` callback observing parameter handles/bytes/tag, and a
configured-process attribute reflection observing map bytes/tag. These tests
cover layers independently, not every data type or both service paths across
all profiles. The revised chart parsed, rendered, and was visually inspected
locally on 2026-10-11; GitHub rendering remains unverified. No test was rerun,
and no C++ source, build, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 object/interaction publication and callback-boundary visual review — 2026-10-11

Locally rendered and visually inspected the ordinary publication-withdrawal-
after-send sequence and callback-boundary flowchart in
`HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md`. The sequence was
too wide at its prior five-lifeline layout, so it now uses sender, embedded
RTI, and receiver lanes. It keeps successful send return, later unpublication,
the already-accepted route remaining pending, `evokeCallback()`, and the
receiver-side eligibility/projection recheck distinct. The ordinary path does
not recheck sender publication at callback time; this remains a 2025 embedded
source observation, with no focused send → unpublish → evoke test. The separate
directed-interaction publication fence is not merged into this ordinary path.

The callback-boundary chart is tall but readable: `HLA_EVOKED` queues work,
`HLA_IMMEDIATE` may enter receiver code synchronously, and both converge on
receiver-projection re-evaluation before best-effort per-attribute update-rate
admission and reflection or suppression. This is not a universal callback
ordering or process-parity claim. GitHub rendering remains pending. No C++
source, build, test rerun, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 callback, observation, and time-management cross-guide survey — 2026-10-11

Compared the callback/service-ordering guide's callback model and dispatcher
sections with the time guide's temporal-eligibility/callback-observation and
receive-order/TSO flows, then checked the cross-cutting guide's delivery versus
observation model and service milestones. Together they distinguish service
acceptance and return, callback-task submission, eligibility, callback entry,
callback return, application observation, and service-specific state or
protocol completion. The time guide shows the extra temporal gate and the
HLA_EVOKED wait; the callback guide keeps HLA_IMMEDIATE's possible
service-stack invocation distinct. The cross-cutting guide defines callback
entry as the application-observation boundary and links the focused guides.

Finding: the specific learner gap in the previous handoff is already addressed;
another generic flow would duplicate the existing HLA mental model. No diagram
or source change was justified. The evidence remains service- and
profile-bounded; no total cross-service order, complete normative commit
matrix, process/embedded equivalence, or 2010 parity is inferred. No C++
source, build, test rerun, CI, or Requirements Lab work occurred.

### 2025 ownership and delivery cross-guide survey — 2026-10-11

Compared the core attribute-ownership guide's per-attribute regular,
If-Available, negotiated, and output-set divestiture examples with the
ownership persistence/resignation companion and the cross-cutting
state/event/observation map. The core guide separates void service return,
typed synchronous exceptions, request-pending state, per-attribute callback
outcomes, and callback-time ownership observations; the companion keeps
durable ownership records distinct from live callback routes and current
membership. The object/request/DDM guides keep provider solicitation,
ownership, receiver relevance, and eventual reflection on separate service
paths. The cross-cutting map already links these questions without asserting a
universal pipeline. No additional ownership or cross-service flow was
warranted. Exact normative arbitration/action rules and unsupported profile
behavior remain outside the selected evidence.

### 2025 DDM scope/time/callback content review — 2026-10-11

Re-read the diagram and adjacent prose in the “Keep time and DDM as separate
filters” section. The chart distinguishes region-pair overlap, receive-order
versus timestamp-order, service-specific callback gates, and the selected
callback model; adjacent prose explicitly says the arrows are not a required
internal RTI order. No content change was indicated. This was a source/prose
review only, not a visual or Mermaid-parser pass.

The visual review remains pending. Scoop could not find an `mmdc` manifest;
the temporary bundled-pnpm renderer failed before startup because of missing
transitive modules. The local browser rejected the attempted `data:` preview
URL, so no alternate browser surface or external editor was used. No diagram,
C++ source, or project dependency was changed. No build, test rerun, CI, 2010
stream, or Requirements Lab work occurred.

### 2025 federation/federate exit-boundary survey — 2026-10-11

Compared the lifecycle guide's voluntary ResignAction and ConnectionLost
sections, callback notes, evidence map, and deliberate limits. The existing
flows distinguish the caller's void service result/typed rejection, the
successful membership transition, the continuing federation-execution
lifetime, local stale-callback cleanup, and independently routed survivor
notifications. Connection loss is a distinct forced entry path: the local
`connectionLost` report is not sent over the failed connection, while
survivor callbacks depend on the directive, known objects, switches, routes,
and profile. The companion guides own save/restore, ownership, and time-gate
details. No additional generic lifecycle map was warranted.

The full normative ResignAction decision table and a total ordering for
survivor callbacks remain explicitly unclaimed; the guide points to the pinned
2025 authority and keeps the 2010 lifecycle in a separate reference guide.
The DDM scope/time/callback diagram remains source-reviewed but not visually
reviewed because an available local renderer could not be reached under the
current tool constraints. No C++ source, build, test rerun, CI, 2010 stream,
or Requirements Lab work occurred.

Next handoff: make a bounded HLA-first audit of the 2025 save/restore guide's
Join-during-progress boundary around §4.11. Separate when a new member joins
from the participant set and callback outcomes of an already active protocol;
check the pinned normative authority, existing guide, and cited source/test
evidence, and preserve uncertainty if the normative wording is unavailable.
Revise documentation only for a concrete reader gap. Keep the 2010 stream
separate and do not edit the Requirements Lab or run builds/tests/CI.

### 2025 Join during save/restore and synchronization-point late join — 2026-10-11

The save/restore guide already frames the Join question as unresolved: both pinned Join overloads list `SaveInProgress` and `RestoreInProgress`, while the API annotations alone do not define triggering conditions, order, or save/restore participant effects. The reviewed embedded membership path has no explicit active-save/restore predicate in the inspected sections; the process path is distinct, and the focused interlock test does not Join during either protocol.

A cross-guide comparison found the synchronization guide's late-join flow answers a different question: an omitted-set synchronization point may expand its own participant set, preserving prior achievement results in a source-derived trace. That does not establish save/restore participation or whether Join is admitted during active save/restore. Added an explicit cross-link/caution in the save/restore guide; no new diagram or normative conclusion was justified.

The official IEEE product page offers subscription/purchase access and a public machine-readable downloads bundle, but the checkout and bundle do not provide the normative §4.11 text. Keep the exact Join contract open pending an authorized standard copy. No C++ source, build, test rerun, CI, 2010 stream, or Requirements Lab work.

Next handoff: make a bounded HLA-first cross-guide audit of object registration/discovery, receiver knowledge, attribute ownership, and update relevance across the 2025 Object/Interaction Information Flow, Object Management, Object-Attribute Subscription State, and Attribute Ownership guides. Add only a cross-link or synthesis if there is a genuine learner gap; do not imply one universal pipeline. Keep 2010 and 2025 separate and distinguish embedded/process evidence. Do not build, rerun tests, start CI, or edit the Requirements Lab.
### 2025 object knowledge and discovery commit boundaries — 2026-10-11

Cross-audited the Object/Interaction Information Flow, Joined-Federate MOM, Object-Attribute Subscription State, Attribute Ownership, and DDM guides. They already separate federation membership, publication, per-attribute ownership, receiver selection, per-receiver object knowledge, DDM relevance, and application observation; no additional universal delivery diagram was warranted.

The object guide's discovery note used a general 'current source' phrase for a profile-specific state commit. In the embedded 2025 path, `beginObjectInstanceDiscovery` runs inside the callback route before calling `discoverObjectInstance`. In the process service, the shared registry transition happens before the discovery event is exposed to transport, and the client callback bridge is later. The public process discovery case covers eventual handle/class/name/producer callback delivery under both callback models; it does not assert use of the handle before callback delivery. The guide now labels this as source-derived, not a portable HLA ordering rule or parity claim.

Next handoff: make a bounded evidence audit of the open ordinary-object process late-subscription initial-value path already identified in the 2025 object-attribute subscription guide. Compare its process handler, discovery/initial-reflection route, callback bridge, and cited public tests; state exactly which scenario is and is not covered. Add documentation only if that narrows a real reader gap. Keep RTI-owned MOM evidence separate, preserve the 2010 boundary, and do not build, rerun tests, start CI, or edit the Requirements Lab.
### 2025 ordinary process late-subscription initial-value evidence audit — 2026-10-11

Compared the process ordinary-subscription handler, discovery enqueue path, embedded initial-reflection planner, public process discovery test, regional-advisory test, process MOM test, and the existing subscription-state guide. The process subscribe path replans discovery for registered objects; its discovery enqueue code plans ordinary owner advisories but creates an initial-value update event only for RTI-owned MOM objects. The public ordinary process discovery case subscribes before registration; the regional advisory case also subscribes before registration; the process MOM case is a distinct positive late-subscription/initial-value path for RTI-owned objects. The private advisory case checks an already-known receiver's owner advisory, not initial values.

The guide already captures these boundaries and correctly leaves ordinary process late-subscription initial reflection unresolved. No behavior or conformance conclusion follows from the absence of an ordinary initial-value planner in the inspected path. No guide change was needed; exact normative §5.8 resolution still requires the licensed standard text and focused process observation.

Next handoff: make a bounded HLA-first audit of 2025 interaction-class matching and per-parameter projection in the Object/Interaction Information Flow guide. Compare ordinary and directed interaction paths, their class-ancestry selection, recipient eligibility, and parameter projection; check whether the current cross-links prevent readers from treating them as one subscription rule. Revise only for a concrete learner gap. Keep 2010 separate and distinguish embedded/process evidence; do not build, rerun tests, start CI, or edit the Requirements Lab.
### 2025 interaction matching, projection, and profile-boundary survey — 2026-10-11

Reviewed the class-ancestry map, ordinary receive-order interaction matching, active relevance versus recipient-specific subscription, superclass promotion and parameter projection, directed target-class/selector lookup, and the separate process ordinary interaction sequence. The current guide already explains why object/interaction ancestry is not one reusable nearest-ancestor rule and why directed target lookup does not replace the sent interaction class as the parameter namespace.

The source evidence remains profile-scoped: embedded ordinary callbacks recheck receiver eligibility/projection at callback time; the process ordinary event stores its selected class and projected parameter handles before receive. The guide explicitly keeps the process and embedded paths separate. Exact normative inheritance and a direct declaration-change test are not established, so no extra synthesis or parity claim was added.

Next handoff: make a bounded source/test audit of declaration changes after an accepted ordinary interaction send in the 2025 embedded and process profiles. Verify the source-observed embedded callback-time receiver recheck versus the process event's stored projection, identify focused coverage or its gap, and keep this distinct from the directed-send sender-publication fence. Document only as source/test evidence, not as a general HLA rule. Keep 2010 separate; do not build, rerun tests, start CI, or edit the Requirements Lab.
### 2025 ordinary interaction send and declaration-change evidence audit — 2026-10-11

Compared the embedded accepted-send route, the process interaction event snapshot, and a bounded set of focused test candidates containing `unpublishInteractionClass`. The public process Send Interaction case sends and then immediately pulls the event; it does not change a declaration between those steps. The inspected ordinary regional cases drain or assert callback outcomes before sender unpublication. Declaration-advisory and service tests that contain `unpublishInteractionClass` do not assert an already-accepted ordinary receive route after withdrawal.

No focused ordinary send → unpublish → evoke/receive assertion was identified in this bounded review. The current guide's distinction remains source-only: the embedded callback route rechecks receiver eligibility/projection, while the process event uses its stored projection. This is not an HLA-wide cancellation or ordering rule. No diagram or source edit was warranted; no tests were run.

Next handoff: make a bounded HLA-first audit of the 2025 Directed Interaction guide's target validity, publication-pair checks, recipient selection, and queued callback invalidation. Compare embedded and process paths and the focused cases; ensure the process exact-class evidence is not generalized to ancestor lookup or embedded callback fences. Add or revise a diagram only for a distinct learner gap. Keep 2010 separate; do not build, rerun tests, start CI, or edit the Requirements Lab.
### 2025 directed-interaction target, selector, and callback-boundary audit — 2026-10-11

Compared the guide's producer/target/receiver identities, target-class ancestor lookup, declaration-pair rules, universal/by-ownership selector, embedded queued-callback invalidation, process event snapshot, and timestamped boundary with the focused evidence. The embedded known-target test explicitly covers subscription withdrawal, producer-pair unpublication, republishing for fresh work, and target deletion before callback dispatch. The process multi-recipient case checks two receivers, payload context, sender exclusion, and callback models using exact-class declarations.

The current guide already keeps the process result bounded: no ancestor-declaration process test and no process stale-delivery mutation test are claimed. It also does not apply the embedded callback recheck or tested deletion suppression to process events. No new diagram or text change was warranted.

Next handoff: make a bounded HLA-first audit of the 2025 FOM module admission/composition guide. Compare the reader's model of module inputs, merge/override, completed-model validation, and reference resolution with its existing diagrams and evidence; identify whether a real composition-versus-validation boundary is missing or already clearly linked to the object/interaction hierarchy guides. Revise only for a concrete learner gap. Keep 2010 separate and do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 FOM module boundaries and HLA reader-path recheck — 2026-10-11

Rechecked the guide's HLA-first explanation and its existing diagrams/evidence
table. It already separates model vocabulary from live declarations and
instances; per-module DIF structure from whole-set composition and reference
resolution; and merged-model checks from FDD materialization and runtime
support. The Create and additional-module Join flows then show distinct
profile-specific commit boundaries. The separate edition diagram prevents
the 2010 catalog compatibility lane from being read as a switch to the 2010
RTI behavior stream.

The guide correctly labels MIM-first order and the selected Create/Join
commit details as Umbra evidence rather than universal HLA rules. The public
[IEEE 1516.2-2025 specification page](https://standards.ieee.org/ieee/1516.2/6689/)
describes that specification as format and syntax, not model content; no
additional semantics were inferred from schema validity. Its existing four
diagrams make the composition-versus-validation distinction legible, so no
duplicate chart or prose edit was warranted. The linked source/test evidence
was reused without rereading large translation units or rerunning tests.
No build, test, CI, 2010 change, or Requirements Lab work occurred.

Next handoff: make a bounded HLA-first conceptual audit of the 2025 Order and
Transportation guide. Check that delivery order (receive-order versus
timestamp-order) stays distinct from transportation reliability, including
their relationship to time management, class defaults, per-instance
overrides, and callback observation. Reuse existing diagrams/evidence and
revise only for a concrete learner gap. Keep 2010 separate; do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 order-versus-transportation HLA reader audit — 2026-10-11

The guide already teaches the durable distinction: order describes receive-
versus timestamp-order delivery, while transportation selects a declared
category. Its delivery map keeps service overload shape, sender-side order,
receiver-side order, publication/subscription and DDM eligibility, TSO time
gating, transport metadata, and callback dispatch as separate questions.
Adjacent diagrams then explain prospective class defaults, per-instance
overrides, synchronous order changes, callback-confirmed transportation
changes, and callback-based effective-value queries.

The guide correctly avoids inferring TSO solely from a timestamped service
overload; its selected embedded case is labeled as implementation evidence,
and exact sender-to-receiver mapping remains open to the licensed 2025 text.
The focused evidence is not presented as physical network QoS or exhaustive
cross-profile coverage. No new diagram or content change was warranted, and
no C++ source, build, test rerun, or CI was used.

Next handoff: make a bounded HLA-first reader audit of the 2025 Name and Handle
Resolution guide. Check whether it clearly separates model-declaration
handles, object-instance handles, federate handles, and textual names, and
explains HLA/API-visible lifetimes without reducing those types to Umbra's
underlying representation. Use existing evidence; keep 2010 separate and do
not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 names, handle categories, and identity lifetimes audit — 2026-10-11

The guide already teaches three different questions: whether a model symbol
exists in the joined execution, whether an object instance is known to this
federate, and whether a federate name is active or a handle remains known. It
then distinguishes parent-class-scoped member lookup and inherited members,
category-specific public handle classes, numeric/hash/encoding distinctions,
and normalization coordinates. The internal typed-handle source links are
clearly marked as Umbra representation rather than an HLA identity promise.

Embedded and process evidence is kept separate, including the process-only
post-resignation name/handle observation and the untested process peer-
discovery boundary. No cross-execution numeric identity, universal lifetime,
or 2010 parity is claimed. The guide already has focused diagrams and an API,
source, and test map; no new chart or prose change was justified. No C++
source, build, test rerun, or CI was used.

Next handoff: make a bounded HLA-first audit of the 2025 DataElement encoding
guide. Check the reader boundary between API-level value bytes (including
explicit endian choices and typed handle encoding) and RTI transport framing.
Use existing examples/evidence rather than broad recompilation, and keep the
2010 DataElement guide a separate stream. Revise only for a concrete learner
gap; do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 synchronization point HLA mental-model review — 2026-10-11

The synchronization guide already starts from the portable barrier model:
selected participants are announced, each reports its result, and the
federation-level completion is separate. Its API map and diagrams keep
registration result, announcement, achievement, and `federationSynchronized`
distinct; they also separate synchronization from logical-time grants,
membership transitions, callback dispatch, and save/restore.

The implementation examples clearly label the omitted-set late-join expansion,
resignation removal, callback submission order, and pending-state persistence
as profile-specific. Partial-result-before-late-join and multi-member
partial-progress restore remain source-derived because the cited tests do not
exercise those exact interleavings. No universal callback order, 2010 parity,
or exact normative rule was inferred; no additional diagram was needed. No
build, test rerun, or CI was used.

Next handoff: make a bounded HLA-first conceptual audit of the 2025 TSO
Retraction guide. Check the separation among producer-side retraction
permission, recipient pending/delivered/suppressed state, callback observation,
and application effects already observed. Keep time-management and ownership
boundaries explicit, preserve unresolved normative/profile limits and the
separate 2010 stream, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 TSO retraction and recipient-observation boundary audit — 2026-10-11

The existing HLA-level orientation answers the key learner question without
exposing Umbra queue internals: for one accepted designator, each recipient's
copy is considered separately; a still-pending copy can be withdrawn, while
an already-received message follows the Request Retraction path and leaves
application-side consequences to the receiver. The guide distinguishes the
producer's time-bound legality question from recipient outcome and from each
receiver's outstanding time-request result.

The embedded ledger state, process event cleanup, callback admission,
timestamp-boundary formula, ownership changes, and delete reconstitution are
kept as profile-specific examples. The guide does not turn these into a total
HLA processing order or 2010 parity claim, and it routes grant calculations
back to time management. Existing diagrams already express the durable model;
no duplicate flow or prose change was warranted. No C++ source, build, test
rerun, or CI was used.

Next handoff: make a bounded HLA-first audit of the 2025 Attribute Value
Update Request guide alongside Auto Provide and Attribute Ownership. Clarify
explicit current-value requests versus the owner-side provide callback, the
`autoProvide` switch, and ordinary future updates; keep subscription,
ownership, time, and callback gates distinct. Reuse existing diagrams and
evidence, preserve the separate 2010 stream, and do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 regional request scope versus response DDM audit — 2026-10-11

The request guide already separated provider solicitation from a later
value-bearing update, while the DDM guide explained source-to-receiver region
eligibility. A reader could still mistake a regional request's screening of
providers for the later update's recipient selection. Added one embedded-2025
two-stage diagram to the request guide and a reciprocal DDM cross-link. The
diagram keeps request regions/provider source associations separate from the
later update's applicable source realization and each receiver's committed
subscription. It also notes that the provide callback has no region argument
and that the request alone does not add the requester to the later update's
ordinary recipient set. These remain selected implementation observations,
not a complete normative rule or process/embedded parity claim.

The existing API/source/test pointers were reused; no C++ files were opened.
The user-authorized `scoop install mmdc` attempt found no Scoop manifest, so
the new Mermaid block remains pending parser and visual review; no alternate
installer was used. No build, test rerun, CI, 2010 change, or Requirements Lab
edit was made.

The following subscription-state handoff was stale: the current guide and
earlier cross-guide audits already distinguish declared interest, receiver
knowledge, initial stored-value reflection, Auto Provide, explicit requests,
scope/relevance advisories, and later value eligibility. No duplicate diagram
was warranted. The ordinary process late-subscription initial-reflection
result and licensed §5.8 interpretation remain explicitly unresolved; do not
convert either gap into a conformance claim.

Next handoff: visually validate the newly added regional-request/DDM flow and
the two pending row-27 subscription diagrams when an authorized renderer is
available. Scoop has no `mmdc` manifest, VS Code's command-line preview path
could not write its profile, and the browser policy rejected a local data URL;
do not route around that policy or use another installer without approval.
Keep 2010 separate, avoid C++ builds/tests/CI, and leave the Requirements Lab
untouched.

### Diagram portfolio census and text-first design-note audit — 2026-10-11

A fresh working-tree scan counted 230 Mermaid blocks in 34 of 41
`docs/design` Markdown files: 12 blocks across the two 2010 files, 216 across
31 2025 files, and two in the living index. By diagram form, the snapshot has
130 flowcharts, 77 sequence diagrams, and 23 state diagrams. This is a fenced
block inventory only; it does not validate Mermaid syntax, rendering, or
GitHub presentation, and it supersedes the older 223-block snapshot in the
roadmap's accumulated progress paragraph.

The seven text-first files are the review ledger plus six supporting design
or policy notes. Authorization, logical time, MOM service reporting, and
relaxed DDM already have dedicated HLA-first reader guides, so duplicating
their decision flows in the implementation notes would split ownership. The
embedded time-coordination and runtime-instrumentation notes describe
implementation boundaries rather than standalone HLA reader journeys. No
missing conceptual diagram was established by this scan. The 2010 count and
2025 profile counts remain separate; no 2010 content was changed.

Next handoff: visually validate the regional-request/DDM flow and the two
pending row-27 subscription diagrams with an authorized Mermaid renderer
before adding more diagrams. The user-authorized Scoop install could not find
an `mmdc` manifest; ask before trying a different installer, and do not bypass
the browser URL policy. Keep the Requirements Lab untouched and do not build,
rerun tests, or start CI.

### 2025 DataElement value-boundary and edition audit — 2026-10-11

The 2025 guide gives the learner-facing boundaries from typed value to octets:
the concrete element type selects byte order; composites define count, child
placement, and padding; variants choose by discriminant; and logical-time
elements delegate to their time factory. It distinguishes these value bytes
from typed handles, model input, and private process framing, and anchors the
examples in focused test cases. The separate 2010 companion describes only
its own API, translation units, and smoke tests. Both editions use the shared
unsigned-octet primitive, but no wrapper, composite, or profile equivalence
follows from that fact.

No further learner gap or chart was warranted. The 2025 guide currently says
its seven diagrams were locally rendered and screenshot-reviewed; that claim
does not validate GitHub presentation or clear the three separate charts
pending visual review (regional-request/DDM and two subscription diagrams).
No C++ source, build, test rerun, or CI was used; neither edition's codec was
changed.

Next handoff: visually validate those three pending diagrams when an
authorized renderer is available, then resume the broad HLA reader-gap survey
without duplicating the already-covered encoding guide. Ask before using a
different installer; preserve the 2010/2025 boundary and leave the Requirements
Lab untouched.

### 2010 bounded reference-profile coverage audit — 2026-10-11

Compared the 2010 reference guide's service matrix and evidence limits with
the current 2010 source and test filename inventory. The existing guide
already maps the available connection/federation, declaration, object and
interaction, and synchronization paths. It separates tested logical-time
value encoding/marshalling from unavailable RTI time-management services, and
records that the synchronization path lacks a focused test. The remaining
2010 inventory also contains handle/support/null and binding/header artifacts;
their presence does not establish additional end-to-end service behavior.
None of this is evidence of 2025 service behavior. No additional 2010 flow is
justified without new HLA behavior or test evidence, and no 2010 code or CI was
touched.

Next handoff: visually validate the regional-request/DDM flow and two pending
subscription diagrams when an authorized renderer is available. Then resume
the HLA-first gap survey; keep 2010 reference-profile limits distinct from the
2025 RTI and do not infer time-management parity from shared value encoders.
Do not run builds/tests/CI or edit the Requirements Lab.

### Current Mermaid census reconciled with the guide index — 2026-10-11

The index still labeled a 223-block/32-file census as current. Recounting
fenced Mermaid blocks by file and diagram kind reconciles it with the 230-block
working-tree inventory: 12 in two 2010 guides, 204 in 30 2025 topic guides, 12
in the 2025 cross-cutting guide, and two in the index. The overall type totals
are 130 flowcharts, 77 sequences, and 23 state diagrams, across 34 of 41
design Markdown files. The index now reflects this snapshot. Counts do not
validate Mermaid syntax, local rendering, visual quality, or GitHub output;
the older whole-set grammar pass remains tied to its historical 205-block
snapshot. No HLA behavior was inferred from the census.

Next handoff: continue the HLA-first gap survey while the three identified
regional-request/DDM and subscription charts wait for an authorized renderer.
Do not install via another package manager without approval; keep editions
separate and the Requirements Lab untouched.

### 2010 update-rate API and reference-profile boundary — 2026-10-11

The pinned IEEE 1516.1-2010 API has update-rate designator parameters on
ordinary and regional attribute subscriptions and declares two update-rate
queries. The current Umbra 2010 shell forwards only class and attribute
handles for ordinary subscriptions; the reference runtime stores membership
without rate state. The regional subscription and query overrides are
unavailable. A bounded scan of 2010-named tests found no matching designator
or query case. Added a source-linked flow to the separate 2010 guide, labeling
all of this as reference-profile evidence—not a statement about 2010 normative
semantics or conformance. This increment changed no C++ source and ran no
build, test, or CI; the 2025 stream and Requirements Lab remain untouched.

The diagram census is now 231 fenced Mermaid blocks across 34 of 41 design
Markdown files: 13 in two 2010 guides, 216 in 31 2025 files, and two in the
index; 131 flowcharts, 77 sequences, and 23 state diagrams. These are counts,
not syntax or rendering validation.

Next handoff: survey the HLA meaning and consequences of active versus passive
attribute subscription as a reader question, checking exact standard
authority where available and keeping 2010 API/profile evidence separate
from 2025 behavior. Add a diagram only if the existing subscription and
update-rate guides leave a genuine conceptual gap. The three previously
identified charts still await visual review; do not run builds, tests, or CI.

### Active/passive attribute-subscription coverage audit — 2026-10-11

The current 2025 subscription-state guide already explains per-attribute
additive mode changes, the separate discovery and value-delivery gates, and
why a passive declaration is not by itself a delivery veto when another
non-producer's active declaration establishes relevance. It also labels the
three-role passive-recipient case as source-derived and not established by
the cited passive-only test. The update-rate guide separately treats passive
declarations in rate queries. These diagrams are complementary; another
active/passive chart would repeat existing material.

The separate 2010 reference guide keeps its ignored subscription flag within
the small 2010 profile and does not infer 2025 or normative behavior. Exact
2025 clause review remains open: IEEE's [standard access page](https://standards.ieee.org/ieee/1516.1/6688/)
offers the standard through purchase/subscription, and its public downloads
archive contains only the API XML package. No new normative claim or diagram
was added. This was a documentation audit only; no C++ source, test, build,
CI, or Requirements Lab change.

Next handoff: examine the 2025 process-profile late ordinary-object
subscription and initial-value path, which the existing subscription guide
marks unresolved. Separate ordinary-object evidence from RTI-owned MOM cases,
check the exact standard clause if authorized text is available, and preserve
the embedded/process boundary. Update documentation only if a real HLA reader
gap remains; do not infer conformance from profile behavior. The three charts
awaiting visual review remain a separate backlog item.

### 2025 late-subscription values: embedded/process source boundary — 2026-10-11

Rechecked the existing subscription guide's open process-profile question
against the current source and cited cases. For a late ordinary subscription,
the process handler asks the registry for existing-object discoveries and the
enqueue path commits known-object state. The initial-value event branch in
that enqueue function is specifically for RTI-owned MOM objects. Its public
joined-federate MOM case observes two discoveries and two selected reflections;
that does not prove ordinary-object behavior. The embedded callback path
separately plans eligible current values after `Discover Object Instance`;
the cited regional case observes a stored value on a later matching regional
subscription. That test does not establish the ordinary process route or a
global callback order.

Added a source-labeled comparison flow to the 2025 subscription guide. It
keeps object awareness separate from current-value reflection and leaves the
ordinary process result explicitly unresolved. The embedded source comment
cites §5.8, but the licensed normative text remains unavailable for independent
review; no conformance conclusion was added. The chart has not yet received
local visual review. A current `scoop search mmdc` returned no matches; no
alternate installer was used. No C++ source, test, build, CI, or Requirements
Lab change.

The diagram census is now 232 fenced Mermaid blocks across 34 of 41 design
Markdown files: 13 in two 2010 guides, 217 in 31 2025 files, and two in the
index; 132 flowcharts, 77 sequences, and 23 state diagrams. Counts do not
validate syntax, rendering, visual quality, or GitHub output.

Next handoff: get or locate authorized access to the exact 2025 §5.8 text and
resolve whether the ordinary late-subscription current-value path shown by
the embedded source is a normative obligation, an implementation choice, or
both. Until then, keep the ordinary process outcome unresolved and do not
infer conformance. Review the new chart and the three existing pending charts
when an authorized renderer is available; Scoop currently has no `mmdc`
manifest, so do not switch package managers without approval. Meanwhile
continue the HLA-first survey without adding duplicate coverage.

### 2025 normative-source and Mermaid-renderer access follow-up — 2026-10-11

Checked the official IEEE 1516.1-2025 listing and IEEE SA's access-options
page. The listing identifies purchase/subscription access; the Reading Room
offers free read-only access to selected standards, but this check did not
establish whether 1516.1-2025 is selected or expose §5.8. The exact normative
rule therefore remains unresolved. This is an access-path finding, not HLA
behavioral evidence.

With the user's authorization to use Scoop, searched the configured Scoop
catalogs for both mmdc and mermaid-cli; neither has a matching manifest, so no
package was installed. Leave the four pending diagrams' syntax/render/visual
status unchanged, and do not switch package managers without approval. No C++
source, test, build, CI, 2010 behavior stream, or Requirements Lab change.

Next handoff: continue the HLA-first source/test survey only where a concrete
reader question is not already answered by the guide set. To close the late-
subscription normative question, obtain an authorized §5.8 excerpt or verify
that the standard is available in the IEEE Reading Room. For visual review,
wait for a Scoop Mermaid CLI manifest or explicit approval to use another
installer. Do not infer conformance from Umbra source comments.

### 2025 callback re-entry: HLA permission versus dispatcher capability — 2026-10-11

Reviewed the callback guide's immediate re-entry sequence and session-state
diagram against the current dispatcher/session source and its focused unit
tests. It already makes the critical distinction: recursive locking enables
some same-thread nested dispatch, while permission to call an RTI service from
a FederateAmbassador remains service-specific. Public guards reject Connect,
Disconnect, Join, Resign, and Evoke; session close from an active callback is
private lifetime handling, not public Disconnect. Tests cover Evoke rejection
and same-session close/wait behavior, but not a complete re-entry permission
matrix. The exact 2025 normative matrix remains open pending authorized text.

Finding: the current guide answers this reader question without another chart.
No implementation or test change was made. Next handoff: survey the HLA-facing
exception path when FederateAmbassador code throws under HLA_IMMEDIATE versus
HLA_EVOKED. Separate the inducing service's return, accepted/state/report
effects, and the callback's actual call boundary; keep embedded and process
examples distinct and make normative claims only from authorized API clauses.
Add a diagram only if the existing exception, service-outcome, and reporting
guides leave a demonstrated conceptual gap. No build, test, or CI work.

### 2025 callback data lifetime: borrow during the call, own what you retain — 2026-10-11

The exception-path audit found that the callback/service-ordering guide already
traces HLA_IMMEDIATE versus HLA_EVOKED exception call stacks, the embedded
Send Interaction catch/report boundary, and the distinct process recipient
request/Evoke path. The exception-reporting and process-endpoint guides add
their own scoped consequences. Existing guides also mark the lack of focused
throwing-callback tests and avoid a universal report/rollback rule. No new
exception flow was warranted.

A separate value-path reread found an unaddressed C++ API-use question. The
2025 reflection/interaction signatures pass handle/value maps and tags through
const references; the official 2025 VariableLengthData header describes its
copy constructor and warns that data() pointers are tied to that object's
lifetime and replacement. Added a 2025-only receiver flow to
HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md: decode during the
callback or copy what must outlive it. The focused embedded interaction test
stores map/tag copies inside the callback and validates their bytes after
callback draining. This documents a safe use pattern from the pinned API; it
does not claim a process lifetime implementation detail, a normative clause
not present in the header, or 2010 parity.

The current static census is 233 Mermaid blocks across 34 of 41 design files:
13 in two 2010 guides, 218 in 31 2025 guides, and two in the index; 133
flowcharts, 77 sequences, and 23 state diagrams. The new chart remains
unrendered and unparsed; no C++ source/test/build/CI or Requirements Lab work.

Next handoff: independently survey the 2010 FederateAmbassador and
VariableLengthData API, source, and focused tests for callback payload
lifetime. Keep its evidence and any diagram separate from 2025, and add one
only if a real reader gap remains. Review the new 2025 chart and three earlier
pending charts when a renderer is available. Keep the 2025 §5.8 late-
subscription question unresolved until authorized normative text is available.

### 2010 callback payload lifetime: separate API-use flow — 2026-10-11

The 2010-only audit checked the pinned `FederateAmbassador` declarations,
`VariableLengthData`, the reference-runtime reflection/interaction callback
sites, and the bounded binding-shell smoke already indexed in the reference
guide. The API passes reflection/interaction maps and tags by const reference;
the `VariableLengthData` copy contract permits deleting its source after the
copy, while its `data()` pointer must not be retained beyond the owning
object's lifetime or a data replacement. The reference runtime calls back
inline. The smoke observes delivery but does not test retention after return.

Added a 2010-only use-now-or-copy flow to
`HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md`. It is pinned-API guidance, not a
normative callback-time promise, not process-lifetime evidence, and not an
inference from 2025. No focused test was added or run. The current fence census
is 234 across 34 of 41 design files: 14 2010, 218 2025, and two index blocks;
134 flowcharts, 77 sequences, and 23 state diagrams. The new diagram remains
unrendered and unparsed because the configured Scoop catalogs have no Mermaid
CLI package. No alternative installer, build, test, CI, or Requirements Lab
operation was used.

Next handoff: inspect the existing 2025 time-management guide for a concrete
learner gap in the coupling among regulation/constrained status, advance
grants, and timestamp-ordered events. Reuse current evidence and avoid another
diagram if the guide already teaches the relationship. Keep 2010 separate and
leave normative questions open absent authorized text.

### 2025 timing and synchronization: no duplicate flow warranted — 2026-10-11

The time-management guide already relates regulator/constrained roles, pending
requests and grants, federation bounds, timestamp-ordered events, callback
observation, lookahead, and re-evaluation. No additional synthesis diagram was
needed. The synchronization guide already explains late-join expansion and
has a source-labeled partial-achievement-before-late-join sequence; its exact
interleaving is clearly identified as not directly covered by the focused
tests, so it is not promoted to a normative behavior claim. No build or test
was run.

Next handoff: inspect the HLA mental model across the existing FOM composition
and name/handle guides. Look specifically for confusion between model-module
composition, inherited class identity, and runtime handle lookup. Add no chart
unless there is a distinct reader gap, and keep normative and source-derived
claims separate.

### 2025 FOM composition and identity: gap audit — 2026-10-11

Reviewed the existing FOM-composition and name/handle guides alongside their
cited embedded source and tests. The core boundaries are already explicit:
module validation versus whole-set composition/FDD materialization; atomic
definition replacement plus Join; hierarchy-scoped attribute/parameter
lookup; typed handle identity; and the distinction between model declarations
and federate-local object knowledge. The guides link these concepts rather
than implying that composition itself produces discovery.

The cited cases cover cross-module references, later lookup after an additional
module is joined, prior class-handle stability, and inherited member lookup.
The reviewed evidence is embedded-profile and selected-test evidence only.
No HLA mental-model gap justified another flow, no FOM/name guide edit was
made, and no test was run.

Next handoff: investigate the unresolved 2025 Join-during-save/restore
contract. The API exception annotations, inspected service/registry paths, and
the representative interlock test leave exact timing and participant effects
open. Use authorized normative clauses if available; otherwise preserve the
uncertainty and move to a different HLA topic rather than guess. Keep 2010
separate.

### 2025 authorization gate: bounded reader-gap review — 2026-10-11

Compared the existing authorization guide with the pinned 2025 Authorizer
interface and the Connect, federation-operation, and Join source paths. Its
three callback scopes and per-operation result mapping are clear, and the
guide separates admission from each service's later preconditions, state
change, and outcome. The common connected-operation gate is before both
embedded and process branches; focused operation-test evidence remains
embedded-only. The guide already states that limit. No duplicate diagram or
test rerun was warranted; added direct links to the separate service-
invocation and exception-reporting guides.

The public IEEE product listing describes purchase/subscription access. Its
general access page says Reading Room access covers select standards and the
GET program covers sponsored standards, without confirming that the exact
1516.1-2025 normative clauses are included. Keep Join during save/restore and
the Connect-overload throw-contract question unresolved until authoritative
clause text is available. No build, test, CI, or Requirements Lab work.

Next: review the existing 2025 callback-dispatch exception/cancellation
material against its cited source and focused tests; look for a reader gap
between callback failure, protocol state already committed, and pending work
that is withdrawn. Do not add a parallel flow unless the current guides fail
to make one of those boundaries explicit.

### 2025 callback failure and cancellation: bounded reader-gap audit — 2026-10-11

Reviewed the existing callback/service-ordering and exception-reporting guides
against the dispatcher/session path, embedded Send Interaction accepted
boundary, and indexed dispatcher/process callback tests. The guides already
distinguish HLA_IMMEDIATE unwinding from later HLA_EVOKED unwinding, failed-task
consumption from later FIFO backlog, producer-specific catches, and callback
exception handling from disable/reset/resignation cancellation. The embedded
Send Interaction state/report effects before callback fan-out are identified
as source-derived and not covered by a throwing-callback integration test.
No duplicate flow was warranted.

Evidence remains deliberately narrow: dispatcher tests do not inject a
throwing task; no cited public case throws from Receive Interaction; and the
process resignation tests do not assert unrelated pending-task behavior
after resignation. No tests were run. mmdc is not installed; the authorized
Scoop search found no matching Mermaid package, so existing pending charts
remain unrendered and visually unverified. No alternate installer, build, CI,
or Requirements Lab work.

Next: survey 2025 ownership transfer and resignation for a real learner gap
between ownership state, action-specific resignation effects, and callback
invalidation. Reuse existing guide evidence; do not merge action outcomes or
embedded/process behavior into a universal flow.

### 2025 ownership and resignation: existing flows stay separate — 2026-10-11

Compared the lifecycle, ownership-persistence, attribute-ownership, and
callback/service-ordering guides with the pinned API, selected registry and
ambassador paths, and indexed tests. Existing guides assign different jobs:
the lifecycle guide maps action names to selected effects; its companion
shows the pending-acquisition role guard and member-scoped cancellation; the
core ownership guide treats transfer and deletion invalidation separately.
A single timeline would risk presenting these as one atomic operation or
implying the embedded callback reset also occurs in the process endpoint.

Added a direct lifecycle-to-callback-guide link and made the embedded-only
scope of its callback-reset description explicit. Process Resign instead
cancels exception-report projections without resetting the shared dispatcher;
the source trace does not prove unrelated queued work is delivered afterward,
and cited tests do not assert that outcome. No unified chart was warranted.
No normative action matrix, process parity, or 2010 claim was added. No tests,
build, CI, or Requirements Lab work; mmdc was unnecessary because no new chart
was created.

Next handoff: survey 2025 object-instance identity and lifetime across
registration, deletion, timestamped delete/retraction, and name reuse. Check
whether existing object-management, reservation, TSO-retraction, and time
guides leave a genuine HLA reader gap. Keep execution existence distinct from
federate-visible state, receive-order distinct from TSO, and embedded/process
evidence separate. Do not build or rerun tests for the documentation review.

### 2025 object identity and timestamped deletion: existing views suffice — 2026-10-11

Compared the 2025 name-reservation, object-information, time-management, and
TSO-retraction guides with their cited source and focused tests. The existing
diagrams already distinguish reservation/registration, global instance and
name purge, per-federate known-object state, and recipient-scoped TSO deletion
and retraction. The TSO delete flow also separates canceling a pending removal
from reconstituting invocation-time state after removal has begun. A single
timeline would combine different state machines and could imply a universal
ordering or name-reuse rule.

Added cross-links and a narrow note tying the reservation-state endpoint to
the source-observed TSO delete/reconstitution branch. The focused timestamped
delete test observes lookup while removal is pending and after a pre-delivery
retraction, then reports the receiver as unknown after later removal; it does
not test post-purge name re-reservation or the delivered-delete restoration
branch. No normative timing/name-reuse claim or 2010 parity was added. No new
chart, test, build, CI, or Requirements Lab operation; mmdc was not needed.

Next handoff: survey the 2025 attribute update-rate admission and time guides
for a real learner gap connecting requested intervals, per-attribute
admission/callback filtering, and TSO eligibility. Reuse the pinned API,
existing diagrams, source, and indexed tests. Keep rate admission separate
from timestamp ordering and embedded/process evidence distinct; add no
combined chart if current guides already teach the boundaries. Do not compile
or rerun tests for the documentation review.

### 2025 attribute update rate and TSO: existing synthesis is sufficient — 2026-10-11

Compared the 2025 update-rate and time-management guides, selected embedded
callback/TSO source paths, focused update-rate tests, and the process update-
rate query test. The update-rate guide already has a third diagram separating
logical-time TSO selection from the subsequent per-attribute steady-clock
best-effort filter; an additional cross-guide diagram would duplicate it.
Added a reverse link from the time guide and a concise explanation of the
distinct gates. Corrected the retraction note against the focused test: after
the callback is suppressed, one later public `retract` succeeds and a repeat
is rejected, so rate suppression alone does not establish that the handle is
unavailable at the recipient boundary.

This remains selected embedded source/test evidence. The process query test
checks numeric rate lookup only, not callback admission. No process delivery
parity or absence-of-gating conclusion was drawn; that route remains the next
bounded survey. No 2010 claim, build, test rerun, CI, or Requirements Lab work.
No new chart was added, so no Mermaid rendering/install step was needed.

Next handoff: trace 2025 process-endpoint attribute reflection and TSO
delivery to determine whether/how subscription update rates affect delivered
values. Inspect process projection and callback-bridge paths plus existing
integration assertions; separate public query support from callback-admission
evidence. Treat unverified behavior as unknown, not as non-conformance or
embedded parity. Do not build, rerun tests, start CI, or edit Requirements Lab.

### 2025 process endpoint update-rate boundary — 2026-10-11

The process public-subscription case confirms that an active `High` update-rate
designator reaches the registry and that a later passive empty designator is
stored; process query cases cover numeric lookups. Neither tests per-value
callback admission. Source inspection found the receive-order and TSO event
builders project values from recipient handles without consuming the
per-attribute rate map or invoking `UpdateRateGate`, followed by direct process
callback-bridge reflection. The existing process delivery-reduction lane is
recipient/DDM routing evidence, not update-rate evidence.

Updated the rate guide's profile boundary and evidence table. No new chart was
added because the process endpoint guide already maps the event-to-callback
route and the missing rate effect is not a new portable HLA state machine.
Treat the source result as a profile-specific implementation observation, not
a normative or runtime conformance result. Keep 2010 separate. No build, test
rerun, CI, or Requirements Lab work.

Next handoff: inspect the 2025 late-subscription path for an already registered
object. Compare the existing object-attribute subscription guide with clause
5.8, embedded and process discovery/initial-value sources, and focused tests.
Keep object discovery, initial reflection, and later value eligibility
separate; determine whether the process initial-value edge is unverified or a
source-level discrepancy without claiming conformance. Reuse current diagrams
unless a real HLA-first reader gap appears. No build, test rerun, CI, or
Requirements Lab edits.

### 2025 late-subscription handoff recheck — 2026-10-11

Rechecked the existing source comparison and evidence boundaries after the
process update-rate survey. The guide already distinguishes discovery of an
existing ordinary object from its stored-value reflection and later value
eligibility. The process handler replans discovery; its initial-value event
branch is specific to RTI-owned MOM objects. The ordinary-object result remains
unresolved, and the prior source/test survey is already recorded above; no
duplicate chart or guide edit was warranted.

The user authorized a Scoop-based Mermaid CLI attempt. The command
'scoop install mmdc' returned “Couldn't find manifest for 'mmdc'”; a subsequent
Scoop search for Mermaid packages also found no matches. No CLI was installed
and no alternate package manager was used. Keep visual review of the existing
chart pending. No C++ build, test rerun, CI, or Requirements Lab work.

Next handoff: do not repeat the late-subscription source/test survey. Resolve
the normative question only if authorized IEEE 1516.1-2025 §5.8 text or a
focused ordinary-process observation becomes available. Meanwhile continue the
HLA-first documentation survey on a genuinely uncovered concept, adding or
revising diagrams only where they clarify a real learner gap. Preserve the
2010/2025 boundary and avoid builds, test reruns, CI, and Requirements Lab work.

### 2025 credentials as typed values and authorization docs reconciliation — 2026-10-11

The existing authorization guide mapped the three operation scopes and the
admission-versus-service-result boundary, but not the value that passes through
those callbacks. The pinned 2025 Credentials API stores a type name and
VariableLengthData, lists HLAplainTextPassword and HLAnoCredentials as
predefined types, and explicitly allows authorizer libraries to support
user-defined types. Umbra's password value delegates encoding to
HLAunicodeString; the source comment cites §12.6 and focused unit cases verify
the encoded non-ASCII vector and malformed payload handling. Added a separate
typed-envelope flow and linked the exact API, source, and test locations. The
full licensed §12.6 wording was not independently reviewed.

The adjacent AUTHORIZATION-DESIGN note incorrectly said the embedded RTI did
not yet authorize Create, Destroy, or Join. Current federation lifecycle source
places scoped gates before the embedded/process service branches, and the
existing operation test bodies cover Create/Create-With-MIM, Destroy, and
named/unnamed Join. Corrected the design note and linked the main guide; the
previously recorded local test setup limitations remain unchanged. This is a
documentation reconciliation only. No C++ source change, build, test rerun,
CI, or Requirements Lab edit was made.

The current static inventory is 229 Mermaid fences across 32 of 41 design
Markdown files: 133 flowcharts, 75 sequences, and 21 state diagrams (14
separate 2010 diagrams, 213 diagrams in 2025 files including the cross-cutting
guide, and two index diagrams). The new credential chart is included but not
locally rendered or GitHub-verified; the user-authorized Scoop attempt found no
Mermaid CLI manifest, and no alternate installer was used.

Next handoff: make a bounded HLA-first review of the 2025 federation
save/restore guide, focusing on the boundary between save/restore protocol
progress and Join admission/membership effects. Reuse existing diagrams and
selected evidence; leave exact §4.11 claims open without authorized normative
text. Preserve the separate 2010 stream and do not build, rerun tests, start
CI, or edit the Requirements Lab.

### 2025 process late-subscription callback boundary — 2026-10-11

Rechecked the existing late-subscription comparison against the current
process subscription handler, discovery event path, callback bridge, and the
focused joined-federate MOM case. The guide already distinguishes registry
selection, receiver-known state, discovery, and initial values; the process
MOM case observes both discovery and reflected values but does not assert
callback order. The ordinary-object initial-reflection result remains
unresolved in the inspected process path and test evidence. Updated the
existing comparison diagram to carry the discovery and value event paths to
their separate application callbacks, without implying ordering or 2010
parity. The official IEEE listing provides subscription access, not the
licensed §5.8 clause text; this remains an open question, not a conformance
finding. No C++ source, build, test rerun, CI, or Requirements Lab change.

Next handoff: review the separate 2025 and 2010 DataElement guides together
only as an edition-boundary audit. Distinguish the HLA-facing typed value and
encoding contract from Umbra's byte-order, padding, and serialization helper
choices; verify every cross-edition claim is independently evidenced. Reuse
existing codec sources and test results; make no new parity claim without
evidence and do not build or rerun tests.

### 2025/2010 DataElement API-surface boundary audit — 2026-10-11

Compared the separate 2025 and 2010 DataElement guides with the checked-in
`RTI/encoding` header inventories. Both editions declare `HLAvariantRecord`,
but only the 2025 inventory contains `HLAextendableVariantRecord`,
`HLAlogicalTime`, and `HLAlogicalTimeInterval` DataElement headers. The 2010
API has separate `LogicalTime`, `LogicalTimeInterval`, and `LogicalTimeFactory`
interfaces outside that encoding-header package; their existence is not
evidence for 2025-style DataElement wrappers. Each guide now states this
bounded surface distinction and keeps the ordinary variant-record behavior
edition-scoped. The existing scalar/composite diagrams and separate codec/test
references remain unchanged; no new flow diagram was warranted for a header
inventory distinction. No Mermaid renderer was needed because no diagram was
edited. No source change, build, test rerun, CI, or Requirements Lab work.

Next handoff: make the already recorded bounded HLA-first survey of the 2025
time, ownership, DDM, synchronization, and save/restore guides for a genuine
cross-service conceptual gap around request admission, committed protocol
state, per-recipient eligibility, pending effects, and application observation.
Reuse current source and focused-test evidence; add a map only if it clarifies
a relationship the topic guides do not already explain. Keep 2010 separate,
preserve embedded/process evidence boundaries, and do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 cross-service synthesis gap recheck — 2026-10-11

The planned survey found the synthesis already in place. The cross-cutting
guide separates authority/admission, state, recipient eligibility, and
observation; distinguishes committed facts from pending work; maps separate
time-advance, synchronization, save/restore, and callback-observation
boundaries; and links the service-specific flows. The guide index routes
readers to these maps. The object/interaction guide also already connects a
FOM member's declared type to a compatible DataElement, its encoded octets,
and the typed-handle value map. No additional umbrella chart would clarify
those relationships, so none was added. No Mermaid diagram changed; mmdc was
not needed. No source, build, test, CI, or Requirements Lab changes were made.

Next handoff: use the next concrete reader question or newly surfaced HLA
implementation discovery to select one bounded topic survey. Compare the
existing guide and evidence first, then add or revise only the smallest
HLA-first explanation that closes a demonstrated gap. Keep IEEE 1516.1-2010
separate from 2025 and preserve embedded/process evidence limits; do not build,
rerun tests, start CI, or edit the Requirements Lab.

### Mermaid fence census correction — 2026-10-11

A fresh working-tree inventory found that the active index census counted only
backtick-fenced Mermaid blocks and missed three tilde-fenced diagrams in each
of the 2025 Attribute Scope Advisory and Auto Provide guides. The two guides
each contain one flowchart, one sequence, and one state diagram. Counting both
GitHub-supported fence styles corrects the total from 229 to 235 diagrams
across 34 of 41 design Markdown files: 135 flowcharts, 77 sequences, and 23
state diagrams. The edition split is 14 diagrams in two separate 2010 files,
207 in 30 2025 topic guides, 12 in the 2025 cross-cutting guide, and two in
the guide index. The two guides' prose records their prior local screenshot
reviews; this census did not independently rerender them. A path-only scan of
2,094 local Markdown targets found none missing, but did not validate heading
anchors. No diagram syntax, visual rendering, C++ source, test, build, CI, or
Requirements Lab state was changed.

Next handoff: reconcile the remaining explicitly pending local diagram-review
items against current guides, starting with the unrendered 2025 credential
envelope chart. Do not send repository-derived chart text to an external
preview. The configured Scoop buckets have no `mmdc` manifest; use a permitted
local renderer if one becomes available, otherwise leave visual status
explicitly pending and continue semantic documentation work. Preserve the
2010/2025 and embedded/process boundaries; no C++ build or test rerun is
needed.

### Credential flow renderer availability recheck — 2026-10-11

The configured Scoop search returned no Mermaid CLI manifest. Per the user's
permission to install a local renderer, Node.js LTS and the official
`@mermaid-js/mermaid-cli` npm package were installed; `mmdc --version` reports
12.0.0. Rendering still did not complete: its expected
`chrome-headless-shell` is absent, system Chrome fails with a Puppeteer profile
lock even when given a fresh task-local profile, and the installed Edge binary
fails to launch through Puppeteer. A local HTTP page was also unreachable from
the in-app browser, whose URL policy rejects data URLs. No external preview was
used, no chart image was produced, and no render or GitHub-compatibility claim
is made. Temporary chart/config/profile files were removed; the credential
chart remains explicitly pending visual review. No repository diagram,
2010/2025 behavior, source, test, build, CI, or Requirements Lab state changed.

Next handoff: do not repeat these unchanged renderer paths unless a compatible
headless-browser runtime becomes available. Continue with a bounded 2025
federation-MOM versus joined-federate-MOM concept survey: compare the existing
guides with their focused source/test evidence, clarify which object identity,
state, and ownership scopes differ, and add or revise a single HLA-first flow
only if a reader gap remains. Keep the two MOM scopes and the separate 2010
stream explicit; do not claim process parity or add diagrams to meet a count.
No C++ build, test rerun, CI, or Requirements Lab work.

### 2025 MOM object-scope audit — existing synthesis is sufficient — 2026-10-11

The federation-MOM guide already separates three observations: subscription-
scoped discovery/reflection of the single execution-owned
`HLAmanager.HLAfederation` object, conditional current-value updates, and
request/report interactions. The joined-federate guide separately explains
one `HLAmanager.HLAfederate` object per membership, the private snapshot versus
public object, and why Join is not observer discovery. The federation lifecycle
guide already supplies the cross-scope cardinality diagram. Those existing
flows answer the proposed reader question; an umbrella MOM diagram would
repeat them, so no chart or guide prose was added.

The bounded evidence check agrees with those boundaries. Embedded Join
establishes the federation object independently of a member report-file
lifetime, while the legacy-2010-FOM compatibility branch does not synthesize
the 2025 projection. Registry resignation removes the departing member from
federation-object recipient sets while retaining the execution object; the
process Join handler also establishes it in source. The embedded membership
test observes federation-object discovery, requests the conditional member
list, and checks Join/resign reflections. The process joined-federate MOM test
exercises `HLAfederate` discovery/reflection and report-file behavior, not public
`HLAfederation` discovery or membership reflections. This is an explicit
profile-evidence difference, not proof of process behavior parity. The two MOM
guides and lifecycle synthesis retain the 2010 boundary; the 2010 reference
guide is not evidence for this 2025 object.

### 2025 API-family coverage and local authorization-chart review — 2026-10-11

The pinned 2025 `RTIambassador` service-family headings all have a reader path
in the existing guide index: federation, declaration, object, ownership, time,
DDM, RTI Support, and API-specific services. The inspected support-service
examples already route to the callback guide (`evokeCallback` and
`evokeMultipleCallbacks`), lifecycle guide (automatic-resign directive),
name/handle guide, and DDM guide (available-dimension queries). Focused public
test sources include `ieee1516_2025_connection_receive_evoke_catch2.cpp`,
`ieee1516_2025_connection_catch2.cpp`,
`ieee1516_2025_connection_handle_lookup_catch2.cpp`, and
`ieee1516_2025_connection_available_dimensions_hierarchy_catch2.cpp`. No
top-level HLA service family was found without a guide or stated boundary, so another umbrella map
or diagram would repeat the index. This is navigation coverage, not a
method-by-method API/test completeness claim or normative verification.

The user-authorized local Mermaid setup is now usable: the Scoop search had no
`mmdc` manifest, so Mermaid CLI 12.0.0 was installed via npm, followed by its
matching Chrome headless shell. With local DevTools loopback sockets permitted,
`mmdc` rendered all four charts in the 2025 Authorization guide. The
credential-envelope chart was screenshot-reviewed: its type branches and
admission boundary are readable, with no label/path overlap. This is targeted
local rendering only; the whole diagram set and GitHub rendering remain
unverified. No external preview, source/test/build/CI, or Requirements Lab
work was performed; 2010 remains a separate behavior stream.

Next handoff: audit the separate IEEE 1516.1-2025 and 2010 DataElement guides
as an edition-boundary review. Explain the HLA-facing typed-value and encoding
contract separately from Umbra byte order, padding, and serialization-helper
choices; verify shared explanations independently in both streams. Reuse
existing codec sources and focused test results, and keep normative claims
open without authorized standard text. No C++ build, test rerun, CI, or
Requirements Lab work.

### 2025/2010 DataElement boundary and local diagram review — 2026-10-11

The edition audit confirmed that both pinned public `DataElement` headers
provide the corresponding value-encoding operations (`encode`, `encodeInto`,
cursor-based `decodeFrom`, and octet-boundary queries), while their C++ surface
is not source-identical: the 2010 `clone()` returns `std::auto_ptr` and its
full-buffer `decode()` returns `void`; the 2025 header uses `std::unique_ptr`
and returns `DataElement&`. Added these distinctions to the two separate
guides, explicitly as API-signature differences rather than byte-format rules.
Each edition's byte-order, composite layout, and focused-test claims remain
anchored to that edition's own API/source/test evidence; the separate 2010
stream is not inferred from a 2025 case.

With Mermaid CLI 12.0.0 and its matching local Chrome headless shell, all nine
DataElement diagrams rendered from their source Markdown (two 2010 and seven
2025) and were visually inspected locally. The first 2025 value-to-octets
flow was too wide in its left-to-right layout; reflowing it top-to-bottom made
the text readable without changing its encode/decode or cursor-completion
claims. The other eight layouts were readable at the inspected preview size.
This verifies local rendering and layout only, not the GitHub Mermaid version.
No external preview, C++ source/test/build, CI, or Requirements Lab change.

### 2025 process callback exception and private TSO completion — 2026-10-11

The bounded review found that the public callback exception and active-call
boundary are already explained in the 2025 callback/service-ordering guide,
including the process distinction between the recipient-side request/pump,
the later HLA_EVOKED call, and the producer's separate service response. The
process-endpoint guide now links to that shared explanation and keeps its
failure branch focused on the implementation-specific consequence: a thrown
TSO callback skips the private completion ACK, leaving the coordinator entry
in transit until acknowledgement or recipient cleanup. No duplicate diagram
or second public exception narrative was added.

The source review reconfirmed the callback-started retraction state, the
receiver Resign cleanup boundary, the separate cached pushed-retraction route,
and the later producer Retract dispatch. Existing focused process tests cover
pre-callback retraction or normal callback/ACK paths, not callback-throw then
receiver Resign then producer Retract. Treat the possible post-resignation
retraction callback as an unresolved process-profile source risk, not a
verified runtime outcome or HLA rule. No C++ source, build, test, CI, 2010
behavior, or Requirements Lab content changed; no Mermaid diagram changed, so
`mmdc` was unnecessary.

### 2025 late-subscription source/test revalidation — 2026-10-11

The existing subscription guide already gives the HLA reader's model—interest,
object knowledge, and value reflection are separate—and contains an embedded
late-regional-interest trace plus a clearly labeled process-profile comparison.
Rechecking the current implementation confirmed that the embedded callback
path is the only call site for the ordinary initial-reflection planner. The
process subscription handler plans discovery for existing ordinary objects
and MOM objects; its process discovery enqueue path creates initial-value
events only for RTI-owned MOM instances. The process test for a joined
federate's late MOM subscription observes discovery and reflection, while the
public process subscription case tests declaration storage and the regional
advisory case subscribes before object registration. The private advisory case
starts with an already-known object but checks the provider advisory, not
initial values. The similarly named application-value restore case explicitly
uses the embedded development profile, so it does not fill the process gap.

The pinned API maps `subscribeObjectClassAttributes` to §5.8 but supplies no
normative late-subscription text. The [official IEEE listing](https://standards.ieee.org/ieee/1516.1/6688/)
indicates purchase or subscription access; the [access-options
page](https://standards.ieee.org/access-standards/) describes selected free
standards without establishing that this clause is available. Leave the
ordinary process initial-reflection outcome unresolved, not a conformance
finding. The source, tests, and HLA-first maps in the existing guide are
already explicit enough; no duplicate diagram or prose was warranted. The
authorized `scoop install mmdc` attempt found no manifest, so the pending
late-subscription diagrams remain unrendered locally. No C++ source, build,
test, CI, 2010 behavior, or Requirements Lab content changed.

Next handoff: survey the 2025 Join-versus-save/restore boundary using the
existing federation lifecycle and save/restore guides, pinned Join API
declarations, and focused source/test evidence. Keep Join admission, save or
restore progress, and callbacks/state restoration as separate questions; do
not claim exact §4.11 behavior without authorized clause text. Preserve the
2010 stream and embedded/process evidence boundaries; do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 Join/save-restore admission source-boundary review — 2026-10-11

The pinned Join overloads list `SaveInProgress` and `RestoreInProgress`, but
the available API declarations and current focused interlock case do not
resolve the exact normative admission rule. The test joins both participants
before save and never attempts Join during an active save or restore.

The implementation map is now explicit for both 2025 profiles. The embedded
adapter and registry membership path have no visible save/restore-phase
predicate in the inspected sections. The process service serializes Join
requests against other Join requests, checks the session, optionally prepares
an additional-FOM definition, and commits membership through the registry;
the inspected handler/commit path has no visible save/restore-phase predicate.
That Join-only lock does not establish arbitration with save/restore, and the
registry's serialized commit does not settle which operation wins a race.
These bounded observations do not establish allowed/disallowed behavior,
callback effects, or conformance. The existing HLA-first lifecycle and
save/restore diagrams already show the membership and protocol boundaries;
another combined diagram would repeat them. No normative claim or 2010
inference was added. No C++ source, build, test, CI, or Requirements Lab
content changed; no diagram changed, so `mmdc` was unnecessary.

Next handoff: survey the separate 2025 resignation-action explanations across
the lifecycle, ownership-persistence, object/interaction, and time guides.
Distinguish a federate's selected `ResignAction` from cleanup of owned
objects, pending timestamped work, active protocols, and effects already
observed by peers. Decide whether a concise HLA-first decision view fills a
real reader gap; keep each implementation/profile case bounded and do not
claim a complete normative action matrix without the relevant standard text.
No build, test rerun, CI, or Requirements Lab work.

### 2025 resignation and survivor callback-observation boundary — 2026-10-11

The guide set already distinguishes the six caller-selected 2025
`ResignAction` values, the embedded cleanup/guard matrix, the forced
connection-loss overlay, final-member deletion, pending ownership work, and
the separate TSO cutoff. A seventh combined action matrix would conflate
independent member, ownership, object, and time state or imply unverified
normative completeness.

A narrower reader gap did remain at the observation boundary. In the embedded
adapter, resignation commits member removal and resets the departing
ambassador's callback queue before submitting induced ownership/removal work
to surviving members' independent routes. The focused `HLA_EVOKED`
`DELETE_OBJECTS_THEN_DIVEST` test observes that the call has returned while
peer callbacks are still unobserved, then sees the assumption callback before
removal after Evoke. This is one embedded scenario, not an
`HLA_IMMEDIATE` or normative order claim. Separately, a process-endpoint
`HLA_EVOKED` `NO_ACTION` resign/rejoin case observes removal of the old `HLAfederate` MOM
object before discovery and reflection for the new member lifetime; that is
not process evidence for all six action-specific cleanup effects.

Updated the existing lifecycle sequence and evidence map to show callback
task submission separately from application observation and to link the
bounded process MOM case. No duplicate action diagram or 2010 claim was added.
No C++ source, build, test, CI, or Requirements Lab content changed. `mmdc`
is unavailable, and the authorized Scoop search could not resolve its host;
local syntax/render and visual review of the revised sequence remain pending.

Next handoff: survey the 2025 federation-member listing operation across the
embedded and process endpoints. Compare the pinned API, public adapters,
process operation routing, and focused tests to determine whether the guide's
one-time snapshot boundary is evidenced in each profile; do not infer a
membership event feed or process parity from query names. Preserve the
separate 2010 stream. No build, test rerun, CI, or Requirements Lab work.

### 2025 federation member listing: embedded snapshot vs process route — 2026-10-11

The pinned API exposes one public member-list request with success and
missing-execution report callbacks. The embedded adapter copies the registry
members before submitting the selected report callback, and the focused
listing test exercises the EVOKED success/missing cases. Its immediate-mode
assertion covers the sibling execution-list service, not this operation.

The profile boundary is sharper than the earlier guide note: the public member
listing adapter reads the embedded registry directly and has no process-endpoint
branch. The process transport operation enum, client federation-management
methods, and service dispatch have no member-list operation; the focused test
is explicitly tagged as embedded. So the one-time snapshot is source/test
evidence for the embedded path only. Process-owned membership retrieval and
callback timing are not established; invoking this adapter while connected to
a process endpoint would not fetch a snapshot over that process seam. The
exact outcome is untested, so this remains a routing/coverage gap rather than
a claim about a particular incorrect report. Keep the 2010 reference-RTI
stream separate.

Updated the lifecycle guide's profile-boundary prose and evidence row. No
Mermaid block changed, so no renderer was needed for this increment. The
previously revised ResignAction sequence still has its separate local visual
review pending. No C++ source, build, test, CI, or Requirements Lab content
changed.

Next handoff: review the 2025 process callback-failure follow-up boundary.
Trace what has already crossed the service response, how a throwing recipient
callback affects event consumption and any in-flight TSO completion, and what
later observation is possible. Reuse existing source/test evidence; do not
infer embedded or 2010 parity, and do not build, rerun tests, start CI, or edit
the Requirements Lab.

### 2025 process callback failure after the correlated response — 2026-10-11

The process request/callback guide already contains the TSO-specific failure
branch: a timestamped callback task is consumed if its callback throws, the
completion hook is skipped, no private ACK is sent, and the coordinator keeps
that recipient's delivery in transit until ACK or recipient cleanup. The
cross-cutting callback guide separately explains which active call can observe
an escaping exception. A further reader-relevant boundary was implicit in the
first process flow: the private response is matched and the stream lock
released before buffered event dispatch, but the request routine does not
return that response to its service method until dispatch completes.

Added prose to state that if an HLA_IMMEDIATE callback throws on the draining
thread, the matched wire response has crossed into the client but is not
returned to the process-client service method; the current event was already
removed from the pending-event FIFO and is not retried. Under HLA_EVOKED, the
request can return after queueing, while a later throw consumes the extracted
callback task and leaves subsequent dispatcher tasks queued. This remains a
source-only explanation; the process callback tests do not inject a throwing
FederateAmbassador callback, and the result-status/public-return interaction
has not been integration-tested. No new or revised Mermaid block was needed:
the existing frame/response/dispatch flow plus the cross-cutting exception
map already show the control path. No C++ source, build, test, CI, or
Requirements Lab content changed. A current PATH/Scoop check confirmed
`mmdc` is not installed, but it was not needed because no diagram changed.

Next handoff: review the 2025 ordinary-object late-subscription initial-value
boundary. Re-read the pinned §5.8 API surface, the embedded and process
subscription/initial-value paths, and the focused tests; distinguish ordinary
objects from RTI-owned MOM objects and direct Request Attribute Value Update.
Keep any source-only inference separate from normative claims, preserve the
2010 stream, and avoid builds, test reruns, CI, and Requirements Lab work.

### 2025 late subscription: newly known object vs added attribute on a known object — 2026-10-11

The focused embedded subscription case registers an object, stores two
attribute values, then subscribes a receiver that does not yet know the
instance. It observes discovery and a reflection containing only the selected
stored value; the test stores the callback reports separately and does not
assert their relative order. The embedded callback source plans ordinary
initial reflections after the discovery callback returns. The registry's
discovery planner skips an instance already known to that receiver.

That makes “late subscription” two distinct situations. When a declaration
makes an unknown existing object discoverable, the embedded path has direct
focused evidence for selected current values. When a receiver already knows
the object and adds an attribute, the bounded source path recomputes
scope/advisories but does not run the discovery-scoped ordinary initial-value
planner for that instance. The process subscription handler similarly plans
discoveries and advisories; its separate initial-value event branch is for
RTI-owned MOM objects. The private process advisory case adds `PayRate` after
discovery and observes an owner advisory, but does not assert subscriber value
delivery. Treat value delivery for that second setup as unresolved in the
survey, not as a demonstrated absence.

Explicit Request Attribute Value Update is a third, independent path: a
federate solicits provider callbacks and any value requires a later provider
update. The special RTI-owned MOM direct-reflection path remains separate.
The pinned public API surface and implementation comment identify §5.8, but
the licensed clause text is not in this checkout; this review therefore makes
no normative claim about the already-known-object case. The ordinary process
late-subscription result also remains unresolved. No 2010 inference was made.

Updated the subscription-state guide with a compact scenario table and
evidence boundary; no Mermaid block changed, so `mmdc` was not needed. No C++
source, build, test, CI, or Requirements Lab content changed.

Next handoff: review the 2025 Join-during-save/restore boundary. Re-read the
pinned §4.11 API surface, embedded and process join/save-state paths, and
focused tests. Separate normative requirements from source/test observations,
keep the 2010 stream distinct, and do not build, rerun tests, start CI, or edit
the Requirements Lab.

### 2025 name/handle identity-domain audit — 2026-10-11

The order/transportation guide's next reader-review target was the names and
handle-resolution guide. The existing guide already separates execution-FOM
model symbols, class-scoped inherited members, federate identities, and
federate-local object-instance knowledge. Its typed-handle section cites the
pinned 2025 API's distinct handle classes, equality/order comparison, hashing
with possible collisions, and encode/decode support; a separate diagram says
normalized `unsigned long` coordinates are not handles or portable numeric
identity. The lookup-category table and source/test map cover those boundaries
without claiming exhaustive process parity.

The bounded review found no missing reader model that warrants another chart
or prose block. Existing focused cases cover embedded inherited lookup and
discovery visibility, plus selected process class/object/federate lookup and
normalization paths; the guide already labels the uncovered process cases and
the one post-resignation federate-name/handle asymmetry. The pinned header
confirms the public handle classes expose comparison, hash, and encoding
operations rather than arithmetic. No C++ tests were rerun, no Mermaid block
changed, and no 2010 behavior was inferred.

Next handoff: complete the pending local syntax/render and visual review of the
revised 2025 ResignAction sequence only if npm/npx permission is granted. If
not, leave visual review pending and continue the bounded survey for another
real HLA reader gap, reusing current source and focused-test evidence. Keep
2010 separate and do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 ordinary interaction-subscription path — existing guide coverage is sufficient — 2026-10-11

The bounded review compared the object/interaction information guide with the
declaration-relevance and DDM guides. Together they already distinguish the
federation-wide active-subscription relevance gate (excluding the sender), a
receiver's own active or passive match, required regional overlap, closest
matching ancestor/class promotion, and parameter projection. The process
endpoint companion separately documents when the promoted class/parameter
view is materialized versus the embedded callback-time re-evaluation path;
that difference is explicitly source-derived, not promoted to a general HLA
rule. The evidence map also makes clear that process class-promotion and
post-send declaration-change behavior lack a focused public integration case.

No additional ordinary-interaction chart or prose was justified. This is a
2025-only guide-coverage audit; no 2010 behavior or process/embedded equivalence
is inferred. No C++ source, build, test, CI, or Requirements Lab content
changed, and no Mermaid block changed.

Next handoff: survey the timestamped-interaction lifecycle across the 2025
time-management, order/transportation, interaction, and TSO-retraction guides.
Look specifically for a reader gap connecting timestamp/order selection,
recipient eligibility, queued delivery, and producer retraction; add no
cross-profile or normative rule without direct evidence. If npm/npx is
approved, the separate pending 2025 ResignAction render may be completed; if
not, leave visual review pending. Keep 2010 separate and do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 timestamped-interaction projection and grant boundary — 2026-10-11

The bounded cross-guide survey compared the time-management,
order/transportation, object/interaction, DDM, TSO-retraction, and process
endpoint guides. Their existing diagrams already explain order selection,
recipient/region eligibility, TSO grant timing, callback observation, and
per-recipient retraction. A second end-to-end algorithm would duplicate those
models. A narrower reader gap remained: the ordinary process interaction
section explains send-time projection, but did not warn readers not to carry
that snapshot boundary into timestamped interaction delivery.

Added a profile-labeled flow and evidence entry to the object/interaction
guide. In the inspected 2025 source, the embedded TSO callback closure resolves
the recipient projection during callback dispatch, while the process service
resolves it before constructing and sending the event; the process callback
bridge then uses the event's stored projection. The time guide now labels its
five-step dispatcher sequence as the embedded path, links the regional DDM
replacement example as a separate recipient gate, and cites a process
`HLA_EVOKED` test body that asserts two same-time interactions before one
grant. The embedded regional replacement case exercises one subscription
change; no focused process case mutating subscription state between TSO send
and event materialization was identified. These are bounded source/test
observations, not normative projection semantics, all-service guarantees, or
2010/process-embedded equivalence.

The new Mermaid chart was not locally rendered: the user-authorized
`scoop install mmdc` again found no manifest, and npm/npx remains unauthorized.
Its visual review is pending. This turn inspected existing test bodies but
did not rerun tests or build; no C++ source, CI, or Requirements Lab content
changed.

Next handoff: audit whether timestamped attribute updates have analogous or
different projection-freeze boundaries by comparing their embedded/process
delivery paths with the attribute-subscription, DDM, and time guides. Keep
attribute reflection distinct from interaction delivery; add a synthesis
only for a demonstrated reader gap. Preserve the 2010 boundary, reuse current
test results, and do not build, rerun tests, start CI, or edit the Requirements
Lab. If an approved renderer becomes available, review the new chart and the
pending ResignAction sequence separately.

### Timestamped attribute projection and the recipient's grant boundary — 2026-10-11

Compared the 2025 time-management, DDM, attribute-update-rate, object/interaction,
embedded runtime, and process service paths. The existing regional DDM chart
already covers one embedded selector mutation after a timestamped attribute
update is accepted. A remaining reader gap was the relationship between the
recipient's logical-time delivery gate and when the two 2025 profiles resolve
the current attribute projection.

Added a focused comparison flow and evidence row to the time guide. Embedded
delivery resolves the receiver projection during callback dispatch, then
applies its per-attribute best-effort gate. Process delivery resolves the
projection before materializing/sending the event; the process callback bridge
forwards that stored projection. The private process-service test confirms an
attribute event frame precedes the matching grant frame. A separate public
process-endpoint `RTIambassador` test asserts the reflection callback before
the matching grant, and checks GALT/LITS while the callback is in transit.
Neither process test mutates subscription/region between accepted update and
event creation. The existing process update-rate guide says that the inspected
TSO dispatcher does not apply the embedded rate gate. These are
source/test-limited Umbra observations, not normative HLA projection rules or
process/embedded, interaction/attribute, or 2010/2025 parity claims.

No C++ source, build, tests, CI, or Requirements Lab content changed. The new
chart is not visually reviewed: Scoop could not locate an `mmdc` manifest; no
alternate package manager was used. Static census: 231 Mermaid blocks across
32 of 41 Markdown files (135 flowcharts, 75 sequences, 21 state diagrams; 14
in two 2010 guides, 215 in 29 2025 guides, and 2 in the index); no syntax or
render validation is implied by those counts.

Next handoff: audit same-recipient ordering between timed federation-save
notifications and TSO payloads at a grant boundary. Reuse source and existing
focused test results; add a diagram only if the current guides leave a genuine
reader gap. Preserve the 2010 boundary, and do not build, rerun tests, start
CI, or edit the Requirements Lab. Keep the timestamped-interaction,
timestamped-attribute, and pending ResignAction visual reviews open until a
renderer is available.

### 2025 object existence and federate-local knowledge — 2026-10-11

The guide-portfolio survey selected a non-time concept that readers otherwise
had to piece together: federation-level object existence is not the same fact
as one federate's local knowledge. The cross-cutting guide now has a compact
relationship map, while the detailed object guide retains separate
registration/discovery, global deletion/removal, and local-forget service
flows. The map links its object source/evidence section and four focused test
scenarios. Those tests are not one end-to-end proof; process local-delete
evidence covers success only. No 2010 behavior or embedded/process parity is
inferred.

The 2010 survey remains deliberately bounded to the separate reference-RTI,
DataElement, handle, and logical-time paths and their existing tests. Current
static census: 238 Mermaid fences across 34 of 41 design Markdown files (138
flowcharts, 77 sequences, 23 state diagrams; 14 in two 2010 files, 222 in 31
2025 files, and 2 in the index). This does not establish syntax, rendering, or
visual review. The new map's visual review is pending: the authorized Scoop
attempt found no mmdc manifest and no alternate package manager was used. No
C++ build, test rerun, CI, or Requirements Lab edit was made.

Next handoff: audit the existing 2025 synchronization/save-restore
participant-set comparison against source and focused tests. Keep the two
protocols' participant, completion, and callback rules distinct; preserve the
late-join/test gaps and unresolved Join normative question. Do not add a
duplicate diagram if the current map already explains the reader's question.

### 2025 ownership callback queue versus restored work — 2026-10-11

The existing ownership and ownership-persistence guides already explained
request-versus-transfer, selected ledger recovery, recipient validation, and
resignation cleanup. A smaller process-profile gap remained: a callback
already pushed into a live HLA_EVOKED queue is not the same pending work item
that a restore service reconstructs from typed state.

Added a profile-labeled comparison flow to the ownership-persistence guide and
a link from the federation save/restore guide. The process callback bridge
maps the event to requestAttributeOwnershipAssumption. In one focused
process-endpoint test, pre-save discovery and assumption callbacks remain
uninvoked through service-level Save/Restore, then are observed before that
test's save/restore status callbacks. A separate process test plus the restore
service shows reconstructed assumption work queued after federation-restore
notifications. These are separate test scenarios; they do not prove a host
process restart, process/embedded parity, or a normative HLA callback-order
rule. Tests were inspected, not rerun.

The chart remains visually unreviewed: mmdc is unavailable and the previous
authorized Scoop install found no manifest; no alternate package manager was
used. Current census: 239 Mermaid fences across 34 of 41 design Markdown
files (139 flowcharts, 77 sequences, 23 state diagrams; 14 in two 2010 files,
223 in 31 2025 files, and 2 in the index). This is a fence/type count only.
No C++ build, test rerun, CI, or Requirements Lab edit was made.

Next handoff: audit the existing 2025 synchronization/save-restore
participant-set comparison against current source and focused-test evidence.
Preserve separate participant/completion/callback rules and leave late-join,
partial-achievement, and Join normative limits explicit. Keep 2010 separate;
add no diagram unless the current map leaves a genuine learner gap.

### 2025 participant-set comparison source/test recheck — 2026-10-11

Rechecked the existing [cross-cutting participant-set map](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#participant-set-changes-are-protocol-specific), its linked [synchronization guide](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md#late-joiners-change-only-an-expandable-barrier), the [save/restore admission boundary](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#join-crosses-a-separate-membership-admission-boundary), and the current 2025 source paths. The map remains useful and does not need a duplicate chart.

The registry records whether the synchronization set was supplied separately from its initial membership expansion ([registration](../../cpp/src/internal/federation/federation_registry.cpp#L188)); its late-Join hook expands only eligible pending points ([Join hook](../../cpp/src/internal/federation/federation_registry.cpp#L246)). The focused [late-Join case](../../cpp/tests/ieee1516_2025_federation_registry_synchronization_point_late_join_explicit_set_catch2.cpp#L16) joins before achievement, so partial-achievement-before-Join remains source-derived. Save creates its participant-status map at operation start ([save control](../../cpp/src/internal/federation/federation_registry_save_control.cpp#L40)); restore checks saved/current participant identities before creating its operation ([restore admission](../../cpp/src/internal/federation/federation_registry_restore_control.cpp#L335)).

The Join adapter, shared membership transaction, and process handler still show no matching active-save/restore predicate in the inspected paths; they do not update the already-created operation status maps ([embedded adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L364), [registry transaction](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L238), [process handler](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L193)). The focused [representative interlock case](../../cpp/tests/ieee1516_2025_representative_save_restore_interlock_catch2.cpp#L34) joins both participants before Save. Related test searches did not find a focused Join attempted during an active protocol. Both pinned 2025 Join overloads list `SaveInProgress` and `RestoreInProgress`, but the exact §4.11 triggering conditions and participant effects are not established by these annotations or source inspection. Keep acceptance, rejection, and cohort effects open; do not turn this apparent API/source gap into a conformance claim. The synchronization evidence remains an embedded-registry slice and does not imply 2010 or process/embedded parity.

No source or Mermaid block changed; no tests, build, CI, or Requirements Lab work was performed. The existing diagram inventory remains 239 fenced blocks by the last static count; this audit did not render or visually review the full set.

Next handoff: survey the 2025 DDM/region guide for a learner gap in the relationship among region extents, subscriptions, region updates, and delivery eligibility. Keep portable HLA behavior primary and source/test evidence bounded; add a diagram only if it materially clarifies that relationship. Preserve the separate 2010 stream.

### 2025 DDM concepts and clause navigation — 2026-10-11

Reviewed the in-progress [DDM/regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
and kept its HLA mental model primary: available class dimensions are not a
region's dimensions; pending range edits are not the committed routing shape;
publisher source regions and receiver subscriptions contribute distinct
eligibility inputs; object discovery/relevance and interaction routing are
separate; and DDM selection does not replace time-management or callback
eligibility. The guide's existing flows already make these relationships
visible, so this pass added no Mermaid block.

Added a [clause navigation table](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#L107)
to the guide, backed by the pinned 2025 `RTIambassador.h` section headings:
region lifecycle §§9.2–9.4, regional object and interaction services
§§9.5–9.13, and dimension/range queries §§10.21–10.28. This provides precise
reader entry points but is not normative text; the official standard remains
the authority for preconditions and effects.

The existing evidence map covers selected region lifecycle, attribute
association/relevance, regional interaction, subscription replacement, and
timestamped-delivery cases. It is not a complete exception matrix or proof of
all-ordering, profile parity, or 2010 equivalence. Inheritance crossed with
regional declarations remains the next bounded survey question.

Static inventory is 233 Mermaid fences in 32 of 41 design Markdown files:
137 flowcharts, 75 sequences, and 21 state diagrams; 14 blocks in two 2010
guides, 217 in 29 2025 guides, and 2 in the index. The DDM guide's 14 blocks
were not parsed or rendered this pass. `mmdc` was unavailable, and the
authorized Scoop install found no `mermaid-cli` manifest; no alternate
package manager was tried. No source, build, test, CI, or Requirements Lab
work was performed.

Next handoff: inspect 2025 object/interaction class-inheritance × regional-
declaration combinations across the DDM, object/interaction, and FOM guides.
Map exact HLA concepts and clause pointers, compare only existing focused
source/test evidence, and add a chart only if a reader gap remains. Keep 2010
separate.

### 2025 class ancestry feeding a regional object path — 2026-10-11

The current DDM guide already stated that available dimensions for a selected
class are accumulated along its parent chain and showed the `Drink`, `Soda`,
and `Light` query results. Its regional-service section separately explained
class/region compatibility. The missing reader bridge was a single tested
case joining those two facts. Added one source/test-labeled flow for the
Restaurant FOM object-class chain and its embedded three-dimensional regional
object scenario, plus cross-links from the object/interaction and FOM
composition guides.

Evidence: the pinned FOM associates `BarQuantity` with `Food.Drink`
([fixture](../../third_party/ieee1516.2-2025/resources/examples/RestaurantFOMmodule-2025.xml#L226)),
`SodaFlavor` with child `Soda`
([fixture](../../third_party/ieee1516.2-2025/resources/examples/RestaurantFOMmodule-2025.xml#L255)),
and `Sweetener` with `Light`
([fixture](../../third_party/ieee1516.2-2025/resources/examples/RestaurantFOMmodule-2025.xml#L274)); the
embedded dimension query asserts the accumulated `{BarQuantity, SodaFlavor,
Sweetener}` set for `Light` ([query case](../../cpp/tests/dimension_lookup_catch2.cpp#L112)).
The regional test uses all three in both regions for `Light`/`Flavor`, observes
discovery/reflection, makes each source dimension disjoint in turn and sees
no additional reflection, then restores overlap ([integration case](../../cpp/tests/three_dimensional_regional_object_attribute_overlap_catch2.cpp#L110)).
Current source walks from the selected object class through parents
([availability helper](../../cpp/src/internal/federation/federation_registry_regions.cpp#L531));
regional registration and subscription each require the region's dimension
set to be included in that result ([registration check](../../cpp/src/internal/federation/federation_registry_object_instance_registration.cpp#L245),
[subscription check](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L465)).
The test targets the same `Light` class for registration and subscription, so
it does not prove that a superclass regional declaration is inherited by a
child class.

For interaction classes, current source has the corresponding parent-chain
availability helper and uses it in regional declaration/send checks. The
reviewed interaction query and invalid-region-context tests do not exercise
an ancestor-only interaction dimension through a regional service. Keep that
as source-only, and do not infer object/interaction parity, process parity, or
2010 equivalence. The linked API section numbers (§§10.21, 9.5, and 9.8) are
navigation pointers; they are not a paraphrase of the licensed normative
rules.

Static census: 234 Mermaid fences across 32 of 41 design Markdown files (138
flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides, 218 in 29
2025 guides, and 2 in the index). Link and line-anchor checks passed for the
three reviewed guides. The new chart has not been parsed or rendered; `mmdc`
is unavailable and the authorized Scoop install found no `mermaid-cli`
manifest. No source, build, test, CI, or Requirements Lab work was performed.

Next handoff: inspect interaction-class ancestor dimensions through regional
subscription and send, and independently examine superclass-versus-subclass
regional declaration applicability. Use API §§10.22, 9.10, and 9.12 as clause
entry points, compare existing source/tests, and leave normative conclusions
open unless supported by the full standard text. Keep 2010 separate.

### 2025 interaction-class ancestry across regional DDM — 2026-10-11

Completed the bounded 2025 embedded-source audit. The interaction-class
available-dimension helper unions dimensions on the selected class and its
parents. Regional subscription validates each non-empty region against that
set, then stores region state under the exact supplied class handle. Regional
send validates explicit source-region dimensions using the sent class's
available set. Recipient planning is a separate walk: after the global active
relevance gate, it starts at the sent class, checks the receiver's declaration
at each exact candidate class, requires region overlap for a regional match,
and advances toward parents until it finds the closest eligible class. A
parent-keyed declaration can therefore be considered for a descendant send
by this implementation path; the child declaration is not materialized or
copied. This is source behavior, not a normative HLA inheritance statement.

The Restaurant dimension query returns `ServerId` for `MainCourseServed`, but
the pinned FOM declares that dimension on `MainCourseServed` itself beneath a
dimensionless `FoodServed`; this is not an ancestor-dimension test. The
reviewed regional interaction case uses `MainCourseServed` for both
subscription and send. No reviewed test combines an ancestor-only interaction
dimension with regional subscription/send, or a parent regional subscription
with a child send. API §§10.22, 9.10, and 9.12 are navigation pointers only;
licensed normative text remains necessary for conformance wording. Keep the
2010 stream and process-profile behavior separate.

Added one source-labeled Mermaid flow to the DDM guide and cross-linked it
from the object/interaction and FOM guides. The flow includes empty-set
no-op branches, dimension validation, exact-class storage, the separate
relevance gate, the receiver parent walk, region overlap, and callback-class
parameter projection. Local file and line links in all three reviewed guides
resolve. Current static census: 235 Mermaid fences across 32 of 41 design
Markdown files (139 flowcharts, 75 sequences, 21 state diagrams; 14 in two
2010 guides, 219 in 29 2025 guides, and 2 in the index). The new diagram is
not parsed or rendered: `mmdc` is unavailable and the authorized Scoop attempt
found no `mermaid-cli` manifest. No C++ source, build, test rerun, CI, or
Requirements Lab content was changed.

Next handoff: resume the known 2025 late-subscription initial-reflection
documentation gap. Compare the HLA mental model and clause entry point with
existing embedded/process source and focused tests; decide whether a
reader-centered flow can clarify object discovery versus initial attribute
values without overstating implementation evidence. Keep 2010 separate.

### 2025 late-subscription initial reflection: reader-gap audit — 2026-10-11

Rechecked the existing object-attribute subscription-state guide against the
current embedded/process source and focused test cases. It already has an
HLA-first distinction between an object unknown to this receiver, an object
already known when another attribute is added, and an explicit Request
Attribute Value Update. It also includes a receiver-state table, a flowchart
separating discovery, direct initial reflection, Auto Provide, scope advice,
and provider relevance advice, an embedded V1-then-V2 sequence, and a
process-profile source comparison. A second chart would repeat existing
reader coverage, so none was added.

The unresolved evidence boundary remains accurately stated. The embedded
2025 path plans currently held eligible values after the discovery callback
returns; its focused regional test observes V1 at discovery and a later V2
update separately, but does not assert callback order. The surveyed process
ordinary cases do not establish an initial current-value reflection for an
ordinary object: public discovery subscribes before registration; public
subscription tests declaration routing; the regional advisory case subscribes
before registration; the private advisory case adds an attribute after the
object is known but asserts the owner's advisory rather than a receiver value.
The process MOM test observes initial values for RTI-owned MOM objects only.
Do not generalize that special branch or the embedded path. Licensed §5.8 text
and a focused process observation remain necessary for a normative or parity
conclusion.

Current static diagram census is unchanged at 235 Mermaid fences across 32
of 41 design Markdown files (139 flowcharts, 75 sequences, 21 state diagrams;
14 in two 2010 guides, 219 in 29 2025 guides, and 2 in the index). No chart
was added or rendered. The user-authorized `scoop search mermaid` found no
Scoop package; `mmdc`, npm, and npx are unavailable in this environment. No
build, test rerun, CI, or Requirements Lab work was performed.

Next handoff: audit the 2025 Join-during-save/restore admission question in
the existing save/restore guide. Keep the API exception list, bounded
embedded/process membership paths, focused interlock tests, and missing
licensed §4.11 text as distinct evidence; add a diagram only if it clarifies
the participant-set boundary without inventing the unresolved outcome. Keep
2010 separate and do not build, rerun tests, start CI, or edit the Lab.

### 2025 Join during save/restore: participant-set boundary audit — 2026-10-11

Rechecked the existing save/restore evidence against the pinned interface,
bounded embedded and process Join paths, and the representative interlock
case. Both 2025 Join overloads list `SaveInProgress` and `RestoreInProgress`,
but the available API annotations do not establish the exact preconditions or
what happens to save/restore membership. The inspected embedded adapter and
registry path and process handler/registry path expose no explicit
save/restore-phase admission predicate; the process Join mutex serializes Join
requests with one another and does not prove save/restore arbitration. The
focused interlock test joins both members before starting save and exercises
other services during save/restore; it does not attempt an in-flight Join.

The lifecycle guide already supplies the concise cross-protocol HLA flow:
successful Join changes federation membership, an expandable synchronization
point has its own source-observed rule, and save/restore admission remains
unresolved. The save/restore guide carries the detailed evidence table and
authority caveat. A second diagram would restate that unresolved boundary, so
none was added; the detailed guide now links back to the lifecycle overview.
The previously checked public IEEE bundle and pinned API package do not include
the licensed §4.11 text. Do not infer either admission or rejection, and do
not transfer the synchronization-point rule to save/restore or to 2010.

No build, test rerun, CI, or Requirements Lab work was performed. The working
diagram census remains 235 Mermaid fences; whole-set syntax/render validation
is still pending because no Mermaid CLI was available through the checked
Scoop catalog.

Next handoff: audit the lifecycle guide's existing resignation-during-active-
save sequence as a reader-centered HLA flow. Check that the departing member,
survivors, save-failure outcome, and later-save recovery remain distinct from
callback-dispatch details and from the unresolved Join rule. Reuse existing
source/test results, add no duplicate chart, keep process and 2010 boundaries
explicit, and do not build, rerun tests, start CI, or edit the Lab.

### 2025 resignation during active save: callback-model scope audit — 2026-10-11

Rechecked the lifecycle guide's embedded `HLA_EVOKED` sequence against its
focused test and registry path. It correctly centers the participant outcome:
after the callback returns, the member can resign; the in-flight save reports
`FEDERATE_RESIGNED_DURING_SAVE` to both survivors, not the departing member;
the survivors can begin and complete a later save. The in-callback
`CallNotAllowedFromWithinCallback` and queue reset are implementation/profile
details in the same bounded schedule, not a global callback-order rule.

Added a direct `HLA_IMMEDIATE` companion reference. That test confirms the
same save-failure reason and later-save recovery with one survivor, but does
not cover the multi-survivor delivery or in-callback resignation guard. The
current sequence already explains the participant result and the specific
callback schedule, so no second diagram was warranted. Restore resignation
remains a separate registry rule/test path, and no process or 2010 parity is
inferred.

No build, test rerun, CI, or Requirements Lab work was performed. Whole-set
Mermaid syntax/render validation remains pending. Scoop lookup failed on DNS;
the authorized `pnpm dlx` route downloaded Mermaid CLI dependencies but failed
to launch because Node could not resolve `yocto-queue` through the runtime
shim.

Next handoff: audit the 2025 callback-exception boundary in the callback and
service-ordering guide. Separate HLA_EVOKED task failure from HLA_IMMEDIATE
service-stack failure, and service report/accepted-state effects; recheck for
focused throw-path tests before proposing documentation changes. Preserve
embedded/process and 2010 boundaries, and do not build, rerun tests, start CI,
or edit the Lab.

### 2025 callback exceptions: accepted-send boundary — 2026-10-11

Rechecked the callback guide's dispatcher and public-call flows against the
current callback session, immediate drainer, 2025 `receiveInteraction`
declaration, embedded `Send Interaction` catch boundary, process response/event
drain, and available tests. The existing generic charts already explain task
consumption, HLA_IMMEDIATE versus HLA_EVOKED call stacks, later FIFO work, and
producer-specific catches. Added one service-specific embedded sequence for
the harder-to-see accepted-state edge: successful service-report and accepted-
send counter effects precede callback submission; a same-thread immediate
`FederateInternalError` can be caught and rethrown by `Send Interaction` after
those effects, while HLA_EVOKED dispatch raises later from Evoke. A pre-existing
immediate drainer may own the exception instead of the sender's call stack.

This sequence is source-derived only. The 2025 API declares
`FederateInternalError` for `receiveInteraction`, but the focused dispatcher
tests cover successful callbacks, not a throwing task; no focused integration
test establishes `Send Interaction` exception attribution, report-sink
outcomes, or accepted-state rollback. The process endpoint retains its
correlated-response/event-delivery boundary and is not mapped onto the embedded
catch. No normative exception or process/2010 parity claim was added.

Current fence census counts both backtick and tilde Mermaid blocks: 242 total
across 34 of 41 design Markdown files (141 flowcharts, 78 sequences, 23 state
diagrams; 14 in two 2010 files, 226 in 31 2025-named files, and 2 in the
index). The new sequence has not been parsed/rendered. Scoop bucket lookup
failed on DNS; the authorized `pnpm dlx` route downloaded dependencies but
could not launch the CLI because Node could not resolve `yocto-queue` through
the runtime shim. No build, test rerun, CI, or Requirements Lab work.

Next handoff: find a supported, reproducible Mermaid parser/renderer path and
run a syntax audit over the full 242-block inventory, then render and inspect
representative dense diagrams. Keep renderer evidence separate from GitHub
rendering claims; do not build, rerun tests, start CI, or edit the Lab.

### Whole-set Mermaid local render audit — 2026-10-11

Scoop search returned no Mermaid CLI package. Installed Mermaid CLI 12.0.0
with pnpm only inside the task's isolated visualization workspace; no package
dependency or lockfile was added to the Umbra repository. The CLI's Markdown
mode does not recognize tilde fences, so the audit extracted both backtick and
tilde Mermaid blocks directly from the 41 `docs/design` Markdown files and
rendered each block with the CLI's local renderer. The census matched 242
diagrams across 34 files.

The first complete render found two 2025 sequence-label syntax errors caused
by semicolons in labels. Replaced those delimiters in the object-instance-name
reservation and process request/callback guides, then rerendered all six and
all four diagrams in those affected guides. The callback/service-ordering
guide's 11 diagrams also render after its new Send Interaction exception flow
was reflowed for readability. Final local coverage is 242/242 rendered with
zero failures. Visually inspected a dense 2025 callback-exception sequence
and a separate 2010 DataElement flow; the streams remain distinct. This does
not establish visual review of every diagram or GitHub rendering. No C++
build, test rerun, CI, or Requirements Lab work was performed.

Next handoff: conduct a bounded HLA-first review of the 2025 object-instance
naming flow. Clarify reservation, later outcome callback, reservation use at
registration, and peer discovery/local knowledge; check the existing API
clause links and edit only if a real reader gap remains. Keep Umbra ordering
as bounded implementation evidence, preserve embedded/process and 2010
boundaries, and do not build, rerun tests, start CI, or edit the Requirements
Lab. GitHub rendering remains a separate check.

### 2025 object-instance naming: portable API lifecycle — 2026-10-11

The guide already distinguished the reservation outcome callback, registration,
and receiver-local discovery, but its first diagram began with Umbra's registry
and callback queue. Added a portable API-level overview ahead of that trace:
admission versus callback outcome, success granting the requester a
reservation, successful named registration returning the owner's handle, and
eligible peers using a separate discovery-callback path. It explicitly avoids
ordering the outcome callback against the service return and does not attempt
to specify all registration or discovery conditions. Existing Umbra details
remain profile-labeled, and the 2010 stream was not changed.

Checked the pinned 2025 API header entry points for reserve (§6.2), reservation
callbacks (§6.3), named registration (§6.8), and discovery (§6.9). The local
Mermaid CLI failed to launch Chromium; a direct parser check also failed in its
DOMPurify setup. The added chart remains unrendered, and GitHub rendering is
not claimed. No C++ source, build, tests, CI, or Requirements Lab work.

Next handoff: resume the known 2025 ordinary-object late-subscription initial-
reflection gap. Compare the portable HLA mental model and pinned API clause
entry point with existing embedded/process evidence; decide whether a focused
diagram can clarify subscription commit, discovery, and initial reflection.
Preserve the 2010 boundary and avoid builds, test reruns, CI, and Lab edits.

### 2025 late subscription versus delayed update delivery — 2026-10-11

The existing object-attribute subscription guide already covers the receiver's
known/unknown state, discovery, embedded current-value reflection, and the
separate process evidence boundary. No further state-machine diagram was
warranted. Added a concise comparison table because “late subscription” can
refer either to a subscription after an object's registration/discovery
opportunity or to a new subscription before a previously sent update's DSE
delivery-time eligibility decision. The table separates a discovery-time
initial-state reflection from a retained send, while noting that its value may
have originated in an earlier publisher update. It preserves the unresolved
ordinary known-object/new-attribute reflection question.

Checked the embedded callback path, process discovery enqueue path, pinned
§5.8 API entry point, and cited focused test boundaries. The source supports
ordinary initial reflection after discovery in the embedded profile; the
process initial-value branch is MOM-only in the inspected path, and ordinary
process behavior remains unestablished. Licensed §5.8 text is not in the local
checkout. No tests or builds were run. A local Mermaid retry with installed
Chrome also failed in the sandbox profile/authentication boundary; the prior
reservation chart remains unrendered and is tracked as such.

Next handoff: compare the Auto Provide and explicit Request Attribute Value
Update map with the initial-state-versus-DSE-pending-send distinction. Verify
that discovery-time values, provider solicitation, explicit requests, and a
retained update remain distinct by trigger, recipient, and callback. Add only
a concise map or cross-link if that survey finds a real learner gap; preserve
2025 embedded/process and separate 2010 evidence.

### 2025 Auto Provide, explicit request, initial reflection, and DSE links — 2026-10-11

The Auto Provide guide's neighboring-path table already differentiates
discovery-triggered provider solicitation, explicit Request Attribute Value
Update, initial reflection, scope change, and relevance advisory by trigger,
recipient, and payload. The request guide separately models request tag,
provider callback, later value-bearing update, and receiver reflection; the
DSE guide covers the previously-sent-update path. Added one cross-link from
the initial-reflection row to the subscription guide's comparison of
discovery-time state with DSE-delayed delivery. No new flow was warranted and
no direct entitlement for the requester was inferred.

The existing guides retain their separate profile evidence and open
normative/process questions. No new Mermaid block, build, test, CI, or
Requirements Lab edit. The outstanding reservation overview chart remains
unrendered, so the 243-chart current inventory is not wholly render-verified.

Next handoff: audit the 2025 Request Attribute Value Update guide's
requester-versus-receiver boundary using the pinned §6.21 interface, bounded
object/class/regional implementation paths, and focused tests. Keep the exact
normative entitlement unresolved without authorized clause text; only change
the guide if the survey finds a genuine learner gap.

### 2025 Request Attribute Value Update: requester and receiver boundary — 2026-10-11

The request guide already makes the selected implementation boundary clear:
explicit requests solicit providers, while a later update uses ordinary
receiver planning. The pinned API exposes separate request and provider
callback surfaces. The embedded focused case subscribes the requester before
the request; the process class-request fixture writes subscription state
directly to the registry and observes provider callbacks, not response
reflection. Neither establishes an unsubscribed-requester outcome. Regional
request eligibility and later response DDM remain separate decisions.

No extra diagram or request-guide edit was warranted. Keep direct normative
requester entitlement unresolved without authorized §6.21 text. Mermaid CLI
12.0.0 is locally available, but Chromium failed with existing and fresh
profiles; the reservation overview render remains pending. No builds, tests,
CI, or Requirements Lab work.

Next handoff: inspect the 2025 ordinary known-object/new-attribute subscription
case against the pinned API entry point, embedded current-value path, focused
tests, and process evidence. Do not infer the normative outcome without the
licensed clause text. Keep 2010 separate and do not build, rerun tests, start
CI, or edit the Requirements Lab.

### 2025 known-object subscription and current-value reflection — 2026-10-11

The existing subscription guide already contrasts newly discoverable objects,
new attributes on an object the receiver already knows, and explicit value
requests. Its table and diagrams label the ordinary known-object current-value
outcome unresolved. The pinned API provides §5.8 subscription and §6.11
reflection entry points, but not the licensed clause interpretation.

The embedded planner skips known instances and invokes current-value planning
only after the discovery callback; the selected embedded test verifies the
newly discoverable case, not a new attribute on a known object. The process
enqueue path's initial-value event is for RTI-owned MOM objects. The process
late-subscription test checks the owner's relevance advisory, not a subscriber
reflection. This does not establish ordinary process behavior or normative
parity. No further chart or guide edit was justified.

The local Mermaid CLI remains available, but isolated Chrome and Edge render
attempts both failed to launch; reservation-overview rendering remains
pending. No build, test, CI, or Requirements Lab work.

Next handoff: review the 2025 process callback-exception path across its
HLA-facing callback contract, process callback guide, coordinator ACK boundary,
and existing tests. Do not conflate the private completion ACK with an HLA
callback guarantee or infer retry semantics. Add only a cross-link or flow if
an actual reader gap remains; preserve all edition/profile boundaries and do
not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 process callback exception and private TSO completion — 2026-10-11

The callback/service-ordering guide already explains the public callback
exception surface and active-call boundary; the process-endpoint guide explains
the later private TSO completion effect and links back. In the inspected
process path, the completion hook follows the public callback, so an escaping
exception skips the private ACK after the dispatcher has consumed the task.
The coordinator retains that recipient delivery in transit; bounded source
shows in-transit timestamps participating in time-bound and timed-save
readiness checks. Do not present these private states as HLA state or infer
normative retry/error behavior.

Existing process TSO tests cover normal callback/ACK completion and retraction
before callback entry, not a callback throw or recovery afterward. The test
gap remains explicit; the guides already cover the reader-facing distinction,
so no new chart or cross-link was needed. No C++ source, build, test, CI, or
Requirements Lab work.

Next handoff: compare the 2025 Join/save-restore participant-set boundary with
late Join at an expandable synchronization point across the lifecycle,
save/restore, and synchronization guides. Add a synthesis only if the current
cross-links leave a genuine HLA reader gap; keep §4.11 unresolved without
licensed clause text and maintain the separate 2010 stream.

### 2025 Join and active-protocol participant sets — 2026-10-11

The lifecycle guide already juxtaposes the embedded synchronization-point
late-join rule with the unresolved save/restore Join admission boundary. The
save/restore guide links to that map and to the synchronization flow, and
separates the pinned API, embedded source, process source, and focused-test
limits. The synchronization guide provides its own expansion state flow and
does not claim it is a save/restore rule. This is enough to prevent carrying
one protocol's participant-set semantics into the other; no synthesis chart
was warranted.

The 2025 Join exception annotations do not establish exact save/restore
participant effects. Licensed §4.11 text is unavailable, and no focused test
attempts Join during active save/restore. Keep this as an open normative and
runtime-evidence question, not a documentation gap or a 2010/process parity
claim. No source, diagram, build, test, CI, or Requirements Lab changes.

Next handoff: compare the 2025 additional-FOM-module Join path with the
name-to-handle resolution guide. Establish from pinned API/source/tests what
the composed module can change for current members and preserve embedded,
process, and 2010 boundaries. Add only a cross-link or flow if existing guides
leave a genuine reader gap.

### 2025 additional-FOM Join and name lookup visibility — 2026-10-11

The FOM-composition guide now links the atomic replacement/handle-directory
transition to the name-and-handle guide's existing lookup sequence. Focused
embedded tests show an already-joined member resolving added object and
interaction classes and comparing its earlier typed handle equal within the
same execution. The configured-process additional-FOM test checks the joining
client's object-class lookup, not visibility from an already-connected member.
No cross-execution numeric-handle stability or backend-parity claim is made;
the existing name/handle sequence was sufficient, so no duplicate diagram was
added. No source, build, test, CI, or Requirements Lab work.

Next handoff: review the 2025 ordinary process initial-value path at the
subscription/discovery boundary. Compare the subscription, object-management,
and delivery guides, then inspect only the pinned API, targeted implementation
paths, and existing focused tests. Distinguish a missing explanation from a
missing test; add a reader-facing flow or cross-link only where the current
guides leave a real gap. Preserve embedded/process and 2010/2025 boundaries;
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 process late-subscription initial-value boundary — 2026-10-11

The subscription-state guide already has the needed HLA-facing comparison and
an embedded/process sequence, so a second diagram would be redundant. The
process subscription handler plans discovery for ordinary and MOM instances
and routes both through the same enqueue function. In that function, a
separate initial attribute-update event is constructed only for an
RTI-owned MOM object; the inspected ordinary-object discovery route enqueues
no initial-value event. The public process discovery test subscribes before
registration and checks discovery under both callback models, while its
ambassador does not observe reflections. The MOM test proves only the special
MOM values path and does not assert callback order. No focused process test
establishes ordinary late-subscription value delivery or its absence, and
licensed §5.8 text remains unreviewed. This is a source/test/normative
question, not a conformance finding or profile-parity conclusion.

The existing local Mermaid CLI could not launch its browser for rendering in
this workspace, so visual render verification remains open. No package
installation was needed.

Next handoff: review the 2025 already-known-object/newly-subscribed-attribute
boundary. Determine whether a subscription that adds an attribute without a
new discovery has a current-state snapshot effect, keeping that distinct from
explicit Request Attribute Value Update and DSE delivery of a previously sent
update. Compare the subscription-state guide, targeted embedded/process
source and tests, and authorized §5.8 text if available. Preserve the separate
2010 stream; do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 already-known object and newly subscribed attribute — 2026-10-11

The current subscription-state diagram already separates a receiver that is
newly discovering an object from one that already knows it, so no second flow
was added. The source trace now makes the implementation boundary explicit:
subscription mutation returns scope/relevance effects; discovery replanning
skips known instances; the embedded initial-value planner is called only from
the post-Discover callback path; and the process discovery route creates a
separate initial-value event only for RTI-owned MOM objects. Thus the surveyed
ordinary source paths show no subscription-time current-value snapshot for an
already-known object, without turning that observation into an HLA rule.

Focused evidence remains incomplete. The embedded active/passive test changes
per-attribute modes after discovery but only observes the following publisher
update, and the process post-discovery test checks owner relevance advice but
not subscriber reflection. The [official IEEE product listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers access via subscription; licensed §5.8 clause text was not available to
this review or in the local checkout.
The exact standard obligation and ordinary current-value outcome remain open;
this is not a conformance finding. No source, build, test, CI, or Requirements
Lab work; 2010 remains separate.

Next handoff: survey 2025 explicit Request Attribute Value Update as a
provider-solicitation flow. Compare its API and object-value guide with the
Auto Provide distinction, then inspect targeted source/tests to establish
which provider is asked, what a successful request does not deliver by itself,
and whether requester subscription/known-object state affects the route. Keep
HLA-required behavior separate from embedded/process evidence, preserve 2010
boundaries, and do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 explicit Request Attribute Value Update requester-state boundary — 2026-10-11

Reviewed the request guide alongside the Auto Provide guide, the pinned 2025
API declarations, shared registry request planners, process service handler,
and existing embedded/process request cases. The existing diagrams already
show solicitation, provider callback, optional later update, and independently
eligible reflection; no duplicate was added. The request tag reaches the
provider callback, while a later update has its own tag and ordinary receiver
selection. The request itself supplies neither values nor a special reflection
recipient, and Auto Provide's switch is not the explicit-request gate.

The instance overload documents `ObjectInstanceNotKnown`, and the shared
instance planner requires requester-known object state. By contrast, class
planning expands live registered instances under the requested class without
checking each instance against the requester's known-object map or subscription
when forming provider groups. The process class handler calls that same
planner and then uses a separate event-delivery path. The embedded class test
uses an active subscription and discovered target instances; the focused
process class test makes no public subscription call but also discovers both
targets before requesting. Neither test isolates an undiscovered target or
tests the provider's later reflection to an unsubscribed requester. These are
bounded implementation/test observations; exact §6.21 provider and receiver
obligations remain unverified without authorized clause text.

The design-doc cross-link pass covered 42 Markdown files: zero missing local
paths, 186 local fragment links checked, and zero broken heading anchors after
repairing seven stale links. No source edits, builds, test runs, CI, 2010, or
Requirements Lab changes occurred.

Next handoff: review authorized IEEE 1516.1-2025 §6.21 text to determine
whether making an explicit request itself grants the requester response
eligibility. Keep that normative question separate from the shared registry
planner and from embedded/process callback delivery.

### 2025 configured-process ordinary interaction callback evidence — 2026-10-12

The process subscription operation delegates to the shared registry. The
ordinary send handler uses the shared recipient planner and materializes the
received class and parameter projection before it queues or pushes an event.
The receive-order service option defaults to polling; a pushed event is an
unsolicited requestId-zero frame, while the callback bridge and callback model
remain a later client-side boundary.

Test evidence is stronger than the raw-payload case alone but remains exact
class. The configured-process multi-recipient test asserts two callback
deliveries per recipient and FIFO tags in both HLA_EVOKED and HLA_IMMEDIATE.
Its HLA_EVOKED branch polls through evoke; its HLA_IMMEDIATE fixture enables
push and uses a subsequent handle lookup as the receiver exchange fence. That
choice is fixture-specific and does not imply that the callback model defines
the transport. The separate public send test inspects a raw pulled event and
does not assert the callback API. The declaration case tests subscription
state only. None tests ancestor-subscription promotion, post-send declaration
mutation, or interaction receipt after additional-FOM Join. Updated the
process-endpoint sequence to show the distinct service-stream and callback
model boundaries and corrected the object/interaction guide's evidence map.
No normative or 2010 conclusion, build, test run, CI, or Requirements Lab work;
the edited Mermaid source has no fresh local parse/render verdict.

Next handoff: bound the configured-process additional-module Join case against
the ordinary interaction send planner, looking only for evidence that current
catalog extension plus an already-stored ancestor subscription is exercised.
Keep it separate from the embedded Create-time inherited-receipt case and do
not infer 2010/process parity.

### 2025 configured-process additional-FOM interaction-route source boundary — 2026-10-12

The configured process Join's `additionalFomPreparation` is an injected,
optional coordinator callback. The focused process test supplies it, adds an
object-only FOM module, and checks object-class lookup; it never exercises an
interaction class, declaration, or send. The service's serialized Join path
prepares a replacement and commits it with membership. The shared registry
copies/reconciles the prior interaction handle directory, leaves the separate
per-federate declaration map intact, and later plans receive-order sends
against the current catalog. Thus source supports a possible retained
ancestor-subscription match for an added descendant, but there is no process
test of that sequence. This is not callback evidence, an HLA interpretation,
or process/embedded parity. Expanded the FOM guide source/evidence map and
linked it from the 2025 object/interaction guide; no new diagram, build, test
run, CI, 2010, or Requirements Lab work.

Next handoff: review the 2010-only interaction section in the 2010 Reference
RTI guide against its pinned 2010 API, implementation path, and focused tests.
Keep the 2010 stream self-contained; do not reuse 2025 interaction-routing
rules without direct evidence of equivalence.

### 2010 ordinary send validation and lazy handle directory — 2026-10-12

In the separate 2010 reference profile, `getInteractionClassHandle` and
`getParameterHandle` create lazy name/handle entries. `sendInteraction` checks
the class handle, sender publication, and each supplied parameter handle, then
routes inline to other members subscribed to that exact class handle. The
pinned API names the corresponding send-specific exceptions; the chart does
not attempt to depict the entire public service-entry error surface. The
binding-shell smoke passes `ignored-fom.xml`, creates both handles by lookup,
and verifies one successful receive. It does not test FOM admission,
multi-recipient fan-out, or callback failure. The 2010 guide now makes these
boundaries explicit. This is not a 2025 routing claim or a parity statement;
the edited Mermaid chart remains unrendered and no tests/builds were run.

Next handoff: audit the 2025 data-element encoding guide against the pinned
encoding API, scalar/composite/array codec paths, and existing tests. Keep
value encoding separate from private process framing, document byte-order and
string contracts, and preserve the separate 2010 guide and evidence boundary.

