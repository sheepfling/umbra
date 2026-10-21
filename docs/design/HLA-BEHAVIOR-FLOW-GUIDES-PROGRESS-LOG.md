# HLA Behavior Flow Guides: Historical Progress Log

This companion retains the dated reader-gap audits, evidence reviews, and prior handoffs that accumulated after the living guide index. The records are preserved for traceability; their historical “next handoff” notes are not the active queue.

For the current goal, prioritized topics, and active handoff, use [the living guide index](HLA-BEHAVIOR-FLOW-GUIDES.md) and [roadmap handoff](../planning/ROADMAP-INDEX.json). Mermaid rendering and visual-inspection records remain in the [review ledger](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md).

---
### 2025 process Send Interaction failure mapping — 2026-10-16

Compared the pinned untimed Send Interaction exception list with the ordinary
configured-process path. The local adapter preserves typed failures for a
disconnected or unjoined caller and handle wrappers without local identities.
After submission, the service can reject a session/member mismatch or a
non-applied registry plan; the registry distinguishes cases such as an
unpublished class and a parameter not defined for the sent class, but the
service response is generic. The process client turns that non-OK status into
`ProcessFederationClientError`, and the public adapter maps it to
`RTIinternalError`.

The focused public process Send Interaction case verifies success only; it
does not establish those remote failure outcomes. The source path is not a
conformance conclusion. This review is scoped to ordinary, non-RTI-consumed
receive-order interactions; HLAsetSwitches and the separate push-fan-out
failure edge are documented elsewhere.

Added a failure-boundary flowchart and source links to the Process Endpoint
guide. The payload-layer and failure-boundary charts both parsed individually
with Mermaid 12.1.0 and a no-op sanitizer; neither was rendered. Eight new
source anchors were checked separately. No code, tests, build, CI, or
Requirements Lab content changed.

Next handoff: compare the 2025 configured-process Update Attribute Values
failure mapping with this Send Interaction path. Check the pinned API,
adapter, process client/service statuses, and focused public tests; infer no
cross-service rule until the sources demonstrate one. Documentation-only,
2025-only.

---
### 2025 process Send Interaction payload layers — 2026-10-16

Surveyed the configured-process receive-order Send Interaction path against the
existing Object/Interaction, DataElement, and Process Endpoint guides. Added a
flowchart separating caller-produced parameter octets from class/parameter
handle metadata, the versioned private interaction envelope, the outer
request-correlation frame, registry recipient projection, and callback-map
reconstruction. The RTI carries value octets; it does not choose or invoke a
DataElement codec for the application.

The public process send case checks literal value bytes and a tag after a raw
receiver polls the service. A separate multi-recipient test covers official
callbacks and FIFO tags. These are adjacent evidence points, not an end-to-end
DataElement-encode-to-callback test. The new Mermaid flowchart parsed
individually with Mermaid 12.1.0 and a no-op sanitizer; it was not rendered.
Thirteen new local links were checked separately and are not folded into the
existing aggregate. No C++ code, tests, build, CI, or Requirements Lab content
changed.

Next handoff: audit the 2025 configured-process Send Interaction failure
boundary, distinguishing local undefined-handle checks from service-side
membership/publication/parameter-plan rejection and public exception mapping.
Update the existing guide only if the standard, source, and focused tests show
a real reader gap. Documentation-only, 2025-only.

---
### 2025 variant model-to-codec boundary — 2026-10-16

Compared the FOM `DrinkGarnish` composition path with the DataElement codec
implementation and focused tests. The runtime API uses an explicit
discriminant/alternative map configured with `addVariant`; the selected encoder
tests construct that map directly and do not read a composed FOM catalog. The
FOM guide describes model merge/resolution, while the DataElement guide
describes runtime octets. No catalog-to-octets integration test is claimed.

Added a reciprocal boundary note in the DataElement guide and linked the API,
focused codec case, and FOM composition section. The two new source anchors and
one local guide link were checked individually. No diagrams changed, so no
Mermaid parse or render was needed. No test/build/CI rerun, Requirements Lab
edit, or 2010 work.

Next handoff: review the 2025 public process Send Interaction payload boundary
from DataElement-produced bytes through the process client/service, registry
validation, and receive callback. Compare the Interaction and DataElement
guides with focused tests; distinguish handle/parameter checks from payload
bytes and private framing. Document only verified behavior and test limits.
Documentation-only, 2025-only; no 2010 work, builds, test reruns, or CI.

---
### 2025 process variant-record Create/Join evidence boundary — 2026-10-16

Compared composer-level variant-record cases with configured-process FOM
coverage. The public additional-FOM test contains a no-exception Create
expectation for the Restaurant FOM, which includes the `DrinkGarnish`
`HLAextendableVariantRecord`. The test does not inspect the composed type or
exercise a variant-specific Join; its added module is object-only. Thus there is
incidental baseline-input evidence, but variant merge, discriminant, and range
semantics remain composer-test evidence rather than process-profile assertions.

Clarified this boundary in the 2025 FOM guide and linked composition to the
existing DataElement runtime variant flow. Four source anchors and one local
guide-section link were checked individually. No diagram changed, so no Mermaid
parse or render was needed. No test/build/CI rerun, Requirements Lab edit, or
2010 work.

Next handoff: audit the 2025 FOM-to-wire variant-record boundary. Compare the
composed `DrinkGarnish` definition, the DataElement variant codec flow, and
focused encoder/decoder cases. Check that the guides distinguish model
composition from runtime bytes and state any missing end-to-end evidence; revise
only for a demonstrated reader gap. Documentation-only, 2025-only; no 2010 work,
builds, test reruns, or CI.

---
### 2025 process additional-module Join interaction boundary — 2026-10-16

Traced the configured-process interaction path after additional-module Join.
After membership writes succeed, the registry replaces the definition and
reconciles interaction-class and parameter handle directories while leaving
per-federate interaction declarations separate. Process parameter lookup
resolves inherited parameters; subscription requests store the class handle;
send planning validates publication and current class/parameter handles,
selects an eligible class subscription, and filters parameters for the received
class. These are source observations, not a tested post-Join delivery outcome.

Added a focused sequence and source links to the existing FOM guide. The
configured-process additional-FOM case still adds an object-only module and
checks object-class lookup. Separate interaction/parameter lookup cases do not
establish configured-process subscription/send/receive behavior or profile
parity. The new sequence parsed individually with Mermaid core 12.1.0. A broader
2025-file parse attempt was not a valid portfolio check because the in-memory
DOMPurify adapter lacked an interface required by some diagrams; the earlier
297-chart portfolio parse remains the last valid full-set result. No rendering,
build, test rerun, CI, Requirements Lab, or 2010 work.

Next handoff: review the 2025 configured-process variant-record Create/Join
boundary. Compare composer merge/resolution evidence with focused public
process coverage, and update the existing FOM guide only for a demonstrated
reader gap. Do not infer process/embedded parity or normative behavior.
Documentation-only, 2025-only; no 2010 work, builds, test reruns, or CI.

---
### 2025 additional-module Join post-commit profile branches — 2026-10-16

Reconciled the replacement-definition commit and post-commit setup paths. The
registry installs the prepared definition and reconciled handle directories
only after membership writes succeed. The subsequent embedded branch sets up
report/MOM state and applies lifecycle Join; the configured-process branch
completes report/MOM setup when configured, installs session state, and queues
joined-MOM discovery and pending synchronization announcements. Their rollback
paths resign the new member, with process session/event cleanup where already
installed. No prior-definition restore call appears in the inspected rollback
sources.

Revised the existing Join flowchart to branch by implementation profile rather
than compressing these post-commit phases. The added-module post-commit catalog
outcome remains source-only: embedded/process setup-failure tests cover
ordinary Join, while the additional-FOM test covers pre-commit rejection and
valid retry, not post-commit failure after replacement. No HLA transaction
rule or tested catalog-persistence guarantee is claimed. All seven new source
links resolve; the full 2025 Mermaid set parses 297/297 with zero syntax
failures. No tests/build/CI rerun, rendering, Requirements Lab, or 2010 work.

Next handoff: review the 2025 configured-process additional-module Join
interaction boundary. Trace whether new interaction/parameter handles reach
subscription, send, and receive services after commit, and compare that source
path with the focused object-only module test. Do not infer interaction parity
from object-class lookup.

---
### 2025 configured-process delete-privilege source path and test boundary — 2026-10-16

Traced the configured-process Delete Object Instance handler. After validating
session/federate membership, its receive-order branch calls the registry delete
operation; its timestamped branch calls the TSO planner. Both source paths
resolve `HLAprivilegeToDeleteObject` for the registered class and compare the
requester with that specific instance's owner entry. The timestamped planner
also obtains the effective order before the process service queues deletion
work. This is implementation evidence, not a normative or profile-equivalence
claim.

The focused configured-process receive-order and timestamped cases exercise
register-then-delete with the same sender. They do not transfer the privilege,
assert former-owner rejection, or assert `DeletePrivilegeNotHeld`; the embedded
transfer test remains separate evidence. Added this source/test boundary to the
ownership guide. Its five new source links resolve. The existing 297-chart
2025 Mermaid parse remains current because this increment added prose only;
no test/build/CI rerun, rendering, Requirements Lab, or 2010 work occurred.

Next handoff: review the 2025 additional-module Join boundary after atomic
replacement-definition commit, reconcile current source/test evidence for
post-commit setup failure, and update the guide only if a concrete reader gap
remains. Keep catalog persistence source-only; infer no HLA transaction rule.

---
### 2025 catalog inheritance versus per-instance delete authority — 2026-10-16

Reviewed the 2025 standard MIM, catalog lookup, and focused composer/runtime
test sources. The MIM declares `HLAprivilegeToDeleteObject` at
`HLAobjectRoot`; the effective-class projection walks parent classes and merges
their declared attributes. A composer case checks the attribute is visible in
the effective map for every class in the pinned 2025 MIM plus Restaurant FOM
composition. A second case verifies that `HLAmanager.HLAfederate` retains the
inherited row's metadata rather than treating direct MOM declarations as its
whole schema. These are class-catalog facts only.

Added a two-lane flow to the ownership guide: inherited attribute availability
in the composed FOM schema is separate from the current owner tracked for one
object instance and consulted by the selected embedded deletion gate. The
existing transfer-and-delete integration test is cited as embedded runtime
evidence; no configured-process parity is inferred. The composer and runtime
test sources were inspected but not rerun. The five new local links and source
anchors resolve, and the full 2025 guide set parsed 297/297 with zero Mermaid
syntax failures. Rendering, visual review, builds, tests, CI, Requirements Lab,
and 2010 work were not performed.

Next handoff: audit the configured-process delete-privilege authorization and
transfer path plus focused test coverage. Document only directly established
outcomes; generic process deletion or TSO/removal cases do not prove privilege
transfer parity. Keep renderer retries paused until browser-launch access
changes.

---
### 2025 variant-record discriminant composition flow — 2026-10-16

The 2025 FOM module guide had no variant-record section despite current
composer logic and focused Catch2 source covering discriminant types,
enumerator syntax/membership, module extension, declaration-order range
expansion, and overlapping assignments. Added a focused flow from ordered
module merge through completed-model resolution and catalog materialization.
It distinguishes the `HLAextendableVariantRecord` extension gate, merged
enumerator expressions, `HLAother` complement handling, and the intentional
retention of incomplete DIF inputs. The explanation is implementation/test
bounded and does not claim a normative interpretation.

The cited composer test source contains cases for malformed/omitted
expressions, cross-module member resolution, range ordering/overlap, and the
Restaurant `HLAother` example. These cases were not rerun in this documentation
increment. The configured-process handler delegates FOM inputs to preparation
callbacks, but the bounded process/connection test search found no
variant-specific assertion; use that as an evidence limit, not a process
outcome or parity claim. The six Mermaid diagrams in the 2025 FOM guide parsed
successfully, and the full 2025 set parsed 296/296 with zero syntax failures.
No rendering, build, test, CI, Requirements Lab, or 2010 work was performed.

Next handoff: review the 2025 FOM catalog's `HLAprivilegeToDeleteObject`
inheritance evidence and connect it to the ownership guide without inferring
runtime ownership from catalog visibility. The first queued process DDM visual
review remains conditional on browser-launch access changing.

---
### 2025 inline-link integrity audit — 2026-10-16

Checked inline local Markdown links in all 31 `HLA-2025-*` guides. All 3,288
local destinations resolve to files; all 2,629 source line fragments are in
range; and all 264 Markdown heading fragments resolve under GitHub's documented
section-anchor rules. The scan found two stale cross-guide anchors: the
subscription guide's source/test note had changed from `source/test` to its
current generated anchor, and the process TSO retraction heading had been
renamed to describe the cached TSO attribute-delivery boundary. Updated both
links at their call sites.

This check covers inline links outside fenced code only. Reference-style links,
external URLs, full Markdown rendering, and actual GitHub presentation remain
unverified. It is a 2025-only documentation change; no builds, tests, CI,
Requirements Lab, or 2010 work was performed.

Next handoff: keep the first queued 2025 process regional-attribute DDM diagram
for visual review when browser launch access changes; otherwise resume content
review only for a demonstrated 2025 reader gap.

---
### 2025 guide portfolio syntax sweep and sequence-label repair — 2026-10-16

Revalidated the 2025 guide portfolio after the Auto Provide handoff review;
that path was already covered by the existing guide and evidence notes, so no
duplicate chart was added. Parsed every Mermaid fence in the 31
`HLA-2025-*` design guides with Mermaid 12.0.0's core parser. One interaction
information sequence failed because semicolons in alternative labels and a
note were interpreted as statement separators. Reworded those separators as
commas, then reran the full 2025 set: all 295 diagrams parsed with zero syntax
failures.

This was syntax-only using an in-memory no-op DOMPurify adapter; it does not
verify sanitization, rendering, layout, or GitHub presentation. Keep the first
queued 2025 process regional-attribute DDM sequence for visual review, but do
not retry rendering until browser launch access changes. No 2010 work, build,
test rerun, CI, or Requirements Lab work was performed.

Next handoff: after browser-launch access changes, render and visually inspect
the queued 2025 process regional-attribute DDM sequence; otherwise continue
only for a demonstrated 2025 reader gap.

---
### 2025 RAVU response after ownership change — guide clarification — 2026-10-16

Followed the active handoff through the provider's later Update Attribute
Values call. The shared registry planner checks every supplied attribute
against current ownership, then reads the current update-region association
and retains committed source-region snapshots. A callback therefore does not
authorize a later update after ownership has moved, and the request's region
does not become the response's source region.

The embedded adapter translates the unowned status to `AttributeNotOwned`;
the existing embedded passel-lifecycle test verifies a later update fails
after unpublishing, but not specifically after a Provide callback. The
configured-process handler returns a generic rejection for a failed plan; the
client/public-adapter path converts that to `RTIinternalError`. This is a
source-derived possible exception-translation gap, not a verified process
runtime outcome. The regional embedded and process response tests both keep
ownership stable; neither covers Provide, completed transfer, then response.

Updated the existing regional request-versus-response diagram and evidence
table rather than adding another chart. All seven diagrams in the RAVU guide
passed individual Mermaid 12.1.0 core syntax parses using an in-memory no-op
DOMPurify adapter. No rendering, whole-set parse, build, test rerun, CI, or
Requirements Lab work was performed. This increment is 2025-only.

### 2025 configured-process regional class request after ownership change — guide clarification — 2026-10-16

Traced the process RAVU event from per-group projection through both delivery
routes. The server either queues a pull event for later dequeue or sends the
already-created event on the push path. The event carries the selected
provider, object, projected attributes, and request tag. The client buffers
and dispatches it to the callback bridge, which constructs the Provide
callback without a registry owner lookup. The reviewed ownership handlers do
not requalify or remove RAVU events from the inspected queue paths. Therefore
the source shows no owner recheck after projection, but does not establish
whether a particular transfer/callback interleaving delivers or suppresses
the callback.

The focused regional process case observes the callback before its later
response update and contains no ownership transfer in between. The separate
configured-process ownership-cancellation case is object-instance scope and
waits for its Provide callback before starting/cancelling acquisition; it
does not cover a pending class-level event across a completed transfer. Added
this ownership cutoff and evidence distinction to the existing RAVU process
sequence and guide prose. All seven diagrams in the guide passed individual
Mermaid 12.1.0 core syntax parses using an in-memory no-op sanitizer. No
rendering, whole-set parse, build, test rerun, CI, or Requirements Lab work
was performed; this was 2025-only.

Next handoff: review the 2025 RAVU response window when ownership changes
after a Provide callback but before the provider's optional Update Attribute
Values call. Trace whether current ownership and regional associations gate
that update, and compare focused embedded/process tests. Clarify the request-
versus-response distinction only if the existing guides leave a reader gap;
keep normative and profile-boundary claims explicit.

---
### 2025 regional class-request multi-region overlap filter — guide clarification — 2026-10-16

Audited the configured-process regional class-request filter for attributes
with multiple request-region and update-region associations. The shared
candidate predicate compares the sets pairwise, stops after the first
overlap, and inserts the attribute once into the recipient's set. The
class planner forms one candidate per object/provider group, and the process
handler constructs an event from each surviving projection rather than from
each matching region pair. Thus multiple overlaps do not multiply callbacks
in this source path; this remains implementation evidence, not a portable
HLA rule.

The focused configured-process regional case and embedded regional filtering
case use one request region and one update-region association for the
attribute. They cover matching/disjoint and other request boundaries, but do
not exercise multiple overlaps and assert a single callback. Clarified the
process diagram's pairwise-any-match wording and added the source-only
deduplication/test-limit note to the existing RAVU guide; no chart was added.
All seven diagrams in that guide passed individual Mermaid 12.1.0 core syntax
parses using an in-memory no-op sanitizer. No rendering, whole-set parse,
build, test rerun, CI, or Requirements Lab work was performed; this was
2025-only.

Next handoff: audit the configured-process 2025 regional class-request
ownership window after per-group projection and before the provider callback.
Trace ownership-transfer effects on already-projected pushed and pull events,
including whether any queue invalidation or callback-time owner recheck exists;
compare focused tests. Record any remaining limit as source-only and make no
normative or embedded/process parity claim.

---
### 2025 configured-process regional class request across multiple providers — guide clarification — 2026-10-16

Traced the configured-process class planner from live object expansion through
current-owner grouping and regional filtering. For each object, remotely owned
requested attributes are partitioned by current provider; the regional
candidate filter is then applied to each provider's attribute subset. The
process handler separately registers and projects every surviving
object/provider group before creating its push or pull event. A source-shaped
example now makes the fan-out explicit: for one object, an overlapping
attribute owned by P1 can survive as `(O, P1, {A})`, a disjoint attribute
owned by P2 is filtered from that group, and a requester-owned attribute is
not sent to a provider. These paths are linked from the existing RAVU guide.

Evidence remains bounded: the regional process case exercises one object,
provider, and attribute; the separate process class-request case exercises
one provider across multiple objects. No focused public process case proves
one object's attributes split across multiple providers, and the source
example is not a portable requirement or embedded/process parity claim. The
two edited RAVU diagrams passed individual Mermaid 12.1.0 core syntax parses.
No whole-set parse, rendering, build, test rerun, CI, or Requirements Lab work
was performed; this review was 2025-only.

Next handoff: audit the configured-process 2025 regional class-request filter
when one attribute has multiple update-region associations. Trace whether any
request/update-region overlap retains that attribute and whether multiple
overlaps can duplicate a provider callback; compare existing focused tests.
Update the existing DDM/RAVU explanation only if it leaves a real reader gap,
and keep untested behavior source-only.

---
### 2025 configured-process regional class-request projection boundary — guide clarification — 2026-10-16

Rechecked the configured-process regional class-level Request Attribute Value
Update around both validation and event projection. The registry validates the
supplied region pairs before expanding provider work; a typed rejection returns
before the process handler enters its provider loop. Focused process evidence
still consists of separate invalid-selector calls and a one-object,
one-provider, one-attribute matching-region case. It does not combine an
invalid pair with otherwise-eligible work.

For accepted work, the process service registers each provider/object group,
rechecks its current projection, then creates a pushed frame or pull-queue
event. The event stores the projected attribute subset and tag, but no region
handles. The inspected pull dequeue and callback bridge do not query the
registry again. A region mutation after projection and before callback is
therefore a source-only boundary; no focused test establishes the outcome.
Clarified this event shape and untested cutoff in the existing RAVU guide;
added no diagram. The edited Mermaid sequence passed an individual Mermaid
12.1.0 core syntax parse with an in-memory no-op sanitizer. No rendering,
build, test rerun, CI, or Requirements Lab work was performed; this review was
2025-only.

Next handoff: review the configured-process 2025 regional class-request path
when one object's requested attributes have different current providers.
Trace planner partitioning, per-provider region filtering, process
per-group projection, and push/pull routing; compare the one-provider public
case and internal planner evidence. Decide whether the existing RAVU diagram
leaves a genuine reader gap. Keep multi-provider public behavior explicitly
untested, and make no normative or embedded-parity claim. Documentation-only;
do not build, rerun tests, or run CI.

---
### 2025 configured-process Unconditional Divestiture audit — existing coverage sufficient — 2026-10-16

Rechecked the ownership guide's configured-process Unconditional Divestiture
flow against the current service handler, registry transition, pending-work
planner, process push/pull event gates, callback bridge, and focused test
sources. The registry plan removes the former owner before the handler invokes
recipient helpers. A pre-existing regular request can be replanned from the
unowned state; push begins/revalidates its acquisition notification before
frame send, while pull queues it and begins it during receive polling. That
begin step can change ownership before callback projection. These are source
boundaries, not a measured callback-time rule or HLA arbitration/fairness
claim.

The selected process query/acquisition case requests only after observing the
attribute unowned. Process assumption-offer cases exercise their own callback
path; none prequeues a regular request before divestiture. No focused process
test injects failure after registry commit or recipient enqueue/send. The
existing guide already explains these limits, If-Available's separate result
slot, and the possibility of an error after committed state without rollback.
No guide edit or duplicate diagram was warranted. No build, test, CI, or
Requirements Lab work was performed; this audit remained 2025-only.

Next handoff: review configured-process 2025 regional Request Class Attribute
Value Update when one requested pair is otherwise eligible and another is
invalid or inapplicable. Trace the public call, process handler, shared
planner, result dispatch, and focused tests; determine whether evidence
supports all-or-nothing rejection or only per-pair filtering. Update existing
RAVU/DDM guides only for a reader gap; make no normative-precedence, timing, or
profile-parity claim. Documentation-only and 2025-only; keep visual review
paused and do not build, rerun tests, or run CI.

---
### 2025 configured-process ordinary late-subscription audit — no change — 2026-10-16

Reviewed the process path for a subscription that makes an already-registered
ordinary object newly visible. The subscription handler replans ordinary and
MOM discoveries; the discovery enqueue path establishes receiver-known state
before exposing discovery. A separate initial-value event is constructed only
for RTI-owned MOM objects. The process-profile chart and evidence table already
make this boundary explicit and distinguish a newly-known object from an
already-known object gaining another subscribed attribute.

Focused evidence does not close the ordinary value question: the public
process discovery test subscribes before registration and asserts discovery;
the private post-registration subscription case asserts an owner-facing
relevance advisory, not subscriber value reflection; and the configured
process MOM case covers only its separate special-value route. No focused
process test establishes ordinary late-subscription values, and licensed
§5.8 prose remains unavailable. This is an evidence gap, not a conformance
finding or process/embedded parity claim. The existing guide already explains
it clearly, so no chart or guide edit was warranted. No build, test, CI, or
Requirements Lab work was performed; the scope remained 2025-only.

Next handoff: review 2025 configured-process Unconditional Divestiture at its
commit and notification boundaries. Trace pending-acquisition state and
push/pull delivery, then compare the focused cases. Keep untested
pending-acquisition and post-commit failure paths source-only; infer no
fairness, timing, or normative priority. Update the ownership guide only for a
demonstrated reader gap; no duplicate chart. Documentation-only, 2025-only;
keep visual review paused and do not build, rerun tests, or run CI.

---
### 2025 configured-process ordinary interaction send boundary — guide clarification — 2026-10-16

Traced the configured-process ordinary Send Interaction path beyond recipient
selection. The shared planner's event-time class/parameter snapshot and push
versus pull routes were already covered, so no additional chart was needed.
The process service has a separate early control branch for predefined
federation `HLAsetSwitches` and compatible descendants; it applies the switch
update and returns before ordinary recipient planning. The object/interaction
guide now cross-links the dedicated Federation MOM explanation.

For pushed ordinary interactions, source shows recipient frames submitted
sequentially. A null session pointer or failed frame send returns an
internal-error response, with no rollback of earlier submitted frames visible.
The process client and public adapter map that response to `RTIinternalError`.
This leaves partial fan-out as a source-level possibility, not proof that any receiver
callback ran; the reviewed focused cases cover successful exact-class delivery
only and do not inject a mid-fan-out failure. Pull routing queues events rather
than crossing this frame-send boundary. No normative guarantee was inferred.
No build, test, CI, or Requirements Lab work was performed; only the 2025
documentation stream was changed.

Next handoff: review the 2025 configured-process subscription path for an
already-registered ordinary object. Trace subscription matching, discovery,
initial-value planning/queueing, and pull/push delivery against existing
focused tests. Preserve the evidence gap where tests do not observe ordinary
late-subscription values; do not infer normative behavior or embedded/process
parity. Update an existing guide only for a demonstrated reader gap. Keep
visual review paused and do not build, rerun tests, or run CI.

---
### 2025 ordinary interaction unpublication and class-promotion review — no change — 2026-10-16

Reviewed the ordinary interaction sections in the 2025 information-flow guide
against the embedded send/callback path, configured-process send/event path,
and focused interaction tests. The guide already explains that embedded
callback-time projection rechecks the receiver and does not recheck the
sender's current publication, while the process route materializes the
received class and parameter handles into an event before dequeue. A later
declaration change does not rewrite that stored event in the inspected route.
The focused-test scan found no 2025 interaction case that sends, unpublishes,
and then observes the queued delivery; the guide keeps this source-derived.

The adjacent class-promotion section distinguishes the shared nearest-eligible
class/parameter resolver from delivery timing. Embedded tests separately cover
nearest-class receipt identity and parameter projection; process public cases
use the exact sent class and do not prove inherited promotion. Existing prose,
diagrams, and test boundaries are clear; no additional chart was justified.
No build, test, CI, or Requirements Lab work was performed.

Next handoff: review configured-process 2025 ancestor-subscription promotion
and parameter projection for ordinary receive-order interactions. Compare the
shared registry resolver and process event snapshot with exact-class process
cases and embedded inherited-receipt tests; retain the process evidence gap
unless focused process evidence covers the inherited match.

---
### 2025 object-removal identity and recipient-cleanup review — no change — 2026-10-16

Compared the object-information guide's deletion and per-recipient cleanup
flow with the ownership guide's transferred-delete-authority section, the
embedded removal snapshot, and the configured-process receive-order and
timestamped paths. The identity distinction is already explicit in the
ownership guide: public removal callbacks use the original registrant from
the receiver's removal snapshot, while the internal timestamped deletion
message stores the deleting requester for its producer-side lifecycle. The
object-information guide links directly to the ownership discussion and
separates accepted deletion, per-receiver commit gates, process pull/push, and
timestamped removal.

The focused process receive-order and timestamped deletion tests use the same
federate to register and delete, so they do not test transferred delete
authority. The separate embedded transfer-and-delete case does not prove the
process callback result. These limits are already stated; no duplicate chart
or claim was added. No build, test, CI, or Requirements Lab work was done.

Next handoff: review the 2025 ordinary interaction path when publication
changes after a send is accepted but before callback delivery. Compare the
embedded accepted-send boundary and callback-time projection with the
configured-process send/dequeue path and focused cases; retain the source-only
gap if tests do not isolate this transition.

---
### 2025 callback exception/cancellation and first-reflection review — no change — 2026-10-16

Audited the callback/service-ordering guide against the current 2025 dispatcher,
callback session, process client, callback bridge, and embedded Send Interaction
path. The guide correctly separates task extraction/session cleanup from
producer-specific exception handling; it explains immediate-drainer recovery,
evoked task consumption with the remaining queue retained, process request-drain
versus background-dispatch attribution, and the accepted-send report/counter
boundary. It also distinguishes producer-contained ConnectionLost exceptions,
embedded dispatcher reset, and the narrower process projection cancellation.
The cited tests cover successful dispatch and selected lifecycle paths, not the
throwing-callback, error-report-failure, or post-Resign delivery schedules. The
guide already labels those as untested source observations; no extra chart or
claim is justified.

Also reviewed the 2025 object-information guide's first-reflection section.
It distinguishes the embedded ordinary initial-value path from the process
MOM-only initial-value branch, and bounds discovery/reflection order to the
specific source/test evidence rather than asserting global ordering or licensed
§5.8 semantics. The subscription guide supplies the deeper receiver-state
comparison. No change was needed. No build, test, CI, or Requirements Lab work
was performed.

Next handoff: audit the 2025 object-removal and per-receiver cleanup path.
Compare deleting caller versus registered producer/owner identity, embedded
registry transitions, configured-process callback projection, and focused
cases. Keep transferred-owner process identity source-only where tests use one
federate for both registration and deletion.

---
### 2025 ordinary late-subscription and initial-value review — no change — 2026-10-16

Compared the 2025 object-attribute subscription guide with the embedded
discovery/initial-reflection path, process subscription/discovery path, and
the cited focused tests. The guide already separates two receiver states:
becoming newly aware of an existing object versus adding an attribute to an
object the receiver already knows. It also distinguishes an initial snapshot
from a previously sent update whose delivery decision is still pending, and
keeps current-value reflection, scope advice, owner relevance advice, and
Auto Provide solicitation as separate effects.

The process discussion correctly limits the positive value evidence to the
RTI-owned MOM branch. Ordinary process discovery is distinct from an ordinary
initial-value event; the public process discovery case subscribes before
registration, and the private late-subscription case observes an owner
advisory rather than subscriber values. The ordinary process result and the
licensed §5.8 interpretation remain open. Existing diagrams and evidence
labels already communicate these boundaries; no change was justified. No
build, test, CI, or Requirements Lab work was performed.

Next handoff: review the 2025 callback-dispatch exception-capture,
cancellation, and per-service state effects against the pinned API, source,
and focused tests. Keep conclusions specific to their callback model and
embedded/process route; do not infer profile parity or normative meaning
beyond the evidence.

---
### 2025 Auto Provide path review — no change — 2026-10-16

Reviewed the Auto Provide guide against the embedded discovery planner,
current-owner grouping, provider callback-entry revalidation, process
discovery service, process switch handler, and focused tests. The guide
already makes the key service boundary clear: discovery-triggered Auto Provide
is distinct from explicit Request Attribute Value Update, and the provider
callback is only a solicitation, not a value response. It also marks the
configured-process boundary accurately: discovery and the shared switch are
present, but the scanned process discovery path has no Auto Provide planner
call and no focused process solicitation case was found. No diagram or guide
change was justified; no process behavior or profile parity is inferred. No
build, test, CI, or Requirements Lab work was performed.

Next handoff: review the 2025 ordinary late-subscription/initial-value path
across embedded and configured-process sources. Trace process discovery
planning and initial-value event creation, compare with embedded ordinary
initial reflection after the discovery callback, and check that ordinary
object values remain distinct from RTI-owned MOM values. Keep unresolved
process outcomes source-only absent focused evidence.

---
### 2025 interaction regional DDM and FOM ancestry review — no change — 2026-10-16

Compared the interaction-class ancestry sections in the 2025 DDM and FOM
guides with the relevant registry source and focused cases. The DDM chart
already separates three decisions: the selected class and parent chain define
available dimensions; a regional declaration is validated against that set
and stored under the exact supplied class; a later send validates source
regions against the sent class context, then evaluates each receiver by
starting at the sent class and walking toward parents, with regional overlap
checked at each exact key.

The source matches those boundaries. `availableInteractionDimensions` walks
the class parent chain; regional subscription writes are keyed by the supplied
class handle; and the receive-order planner separately validates the sent
region set and resolves candidate subscriptions along the sent-class ancestry.
Focused tests cover same-class regional interaction delivery and ordinary
nonregional candidate promotion, but not an ancestor-only regional dimension
or a parent-class regional subscription followed by a descendant send. The
FOM guide independently records the same source-only gap and links the DDM
chart. The current diagrams and limits are clear; no second chart or prose
change was justified. No build, test, CI, or Requirements Lab work was
performed.

Next handoff: review the 2025 Auto Provide guide as a distinct service path.
Trace publication, discovery, current ownership, and the embedded/process
solicitation boundary against existing focused tests; keep Auto Provide
separate from explicit Request Attribute Value Update and add no process claim
without evidence.

---
### 2025 FOM admission/composition guide review — no change — 2026-10-16

Surveyed the existing 2025 FOM module admission/composition guide against its
linked source and focused-test map. It already diagrams Create preparation,
ordered merge and completed-model resolution, and additional-module Join's
definition/membership commit. It separately explains post-commit setup failure,
catalog and handle reconciliation, later object discovery, MOM effects, and
the source-only configured-process interaction path. The guide distinguishes
normative questions from implementation observations and names the process
interaction tests it lacks.

The remaining process interaction-after-Join gap is already described with
the current-catalog parent walk and an explicit no-delivery-test limit. A new
diagram would repeat the existing FOM, interaction, and DDM flow material
without adding runtime evidence, so this bounded review made no guide change.
No build, test, CI, or Requirements Lab work was performed.

Next handoff: compare the 2025 interaction regional-DDM explanation in the
DDM guide with the FOM guide's source-derived interaction-class ancestry path.
Check whether the existing charts clearly separate class-available dimensions,
regional declaration lookup, and later send-time ancestor matching. Do not
infer normative inheritance from source, and add no chart if the guides already
make those boundaries clear.

---
### 2025 synchronization partial-achievement/late-join review — no change — 2026-10-16

Rechecked the late-join question against the current synchronization guide,
registry source, and focused 2025 tests. The guide already separates omitted
participant-set registration from explicit-set registration, diagrams the
late-join expansion, and shows A's `false` result surviving a later join by C
until B and C finish the enlarged barrier. The registry adds the newcomer to
the synchronization and announced sets without clearing achieved results;
achievement insertion is first-result-wins and completion compares recorded
results with the current set size.

The direct late-join test proves explicit-set non-expansion and omitted-set
announcement, but both that case and the public HLA_EVOKED/HLA_IMMEDIATE cases
join participants before achievement reports. The partial-result-before-join
trace remains source-derived and explicitly untested in the guide. The current
diagram and evidence boundary already answer the reader question, so no
duplicate chart or prose change was justified. No tests, build, CI, or
Requirements Lab work was performed.

Next handoff: survey the IEEE 1516.1-2025 FOM module admission/composition
guide for a concrete reader gap in module selection, validation, or merge
outcomes. Compare the existing diagrams with bounded source and focused tests;
add no diagram unless it explains a distinct state transition. Keep normative
claims separate from current Umbra behavior and test coverage.

---
### 2025 configured-process regional request dispatch/save-restore trace — 2026-10-16

Expanded the 2025 regional class-request trace beyond the ambassador and
specialized service handler. The process host obtains the service handler and
loops through the transport dispatcher; the dispatcher checks request framing
and invokes the handler; handler registration forwards to the operation
switch; that switch routes to the regional handler. The regional handler
checks session/federate identity and current membership, then calls the shared
registry planner. The planner checks membership, catalog, class, attributes,
and regions, but not save/restore state. The process `serviceOperationStatus`
calls found are in service-reporting handlers, not a generic dispatch gate.

No save/restore rejection branch appears in this request chain. The focused
process regional-request tests do not exercise those states, so the guide now
records only the source boundary and makes no runtime or conformance claim.
No diagram changed. No build, test rerun, CI, or Requirements Lab work was
performed.

---
### 2025 embedded/process regional class-request precondition ordering — 2026-10-16

Compared the two 2025 adapter paths against API 9.13 and their focused cases.
The embedded route checks connection and federation save/restore availability
before normalizing request handles; the representative interlock test confirms
the regional class request rejects the isolated save and restore states. The
configured-process route checks class/attribute/region handle values and a
nonempty attribute set before connection and membership, then delegates
registry-region validation through typed statuses. The regional failure tests
overlap on uncommitted, foreign-owned, and wrong-context regions; the process
test adds an unregistered handle.

The API declaration lists possible exceptions but establishes no precedence
for simultaneous failures. No combined-fault case was found, and the focused
process regional-request cases do not cover save/restore states. No equivalent
availability gate appears in the reviewed process adapter/handler/registry
route; this is a source/test boundary, not a claim about a normative ordering
or an untested runtime result. Added this distinction to the 2025 request-flow
guide. No diagram changed, so no Mermaid parse was needed. No build, test
rerun, CI, or Requirements Lab work was performed.

Next handoff: verify whether any configured-process dispatch layer outside the
reviewed regional request handler gates this service during save/restore, then
record the strongest source/test-bounded result in the 2025 guide. Keep this
documentation-only; do not infer API exception priority or start builds/tests.

---
### 2025 configured-process regional class-request validation and exception mapping — 2026-10-16

The configured-process regional class planner validates supplied request
region pairs before recipient expansion. Each supplied pair must name a
requested attribute and a requester-owned region with committed, complete
bounds whose dimensions are available to the requested class. The service
returns planner failures as typed statuses in a rejected response, and the
public adapter maps those statuses to the declared IEEE 1516.1-2025 exception
types.

The focused process selector case verifies four server-side mappings: an
unregistered region handle value and an owned but uncommitted region both
produce `InvalidRegion`; a region owned by another federate produces
`RegionNotCreatedByThisFederate`; and a committed region with dimensions
outside the requested class context produces `InvalidRegionContext`. It
asserts both public exceptions and returned typed statuses. This is not a test
that mixed eligible and invalid work produces no callback. A separate focused
case covers one matching region pair under both callback models; neither case
covers disjoint/multi-provider selection or a region change after the
projection checkpoint. The chart now distinguishes local handle preflight,
server registry validation, typed response, exception translation, and valid
provider routing. Its revised sequence passed an individual Mermaid 12.0.0
core-parser check. Rendering and visual review remain paused. No build, test
rerun, CI, or Requirements Lab work was performed.

Next handoff: compare 2025 embedded and configured-process regional class
request precondition/error ordering for malformed handles, federation state,
and registry region failures against the API declarations and focused cases.
Record only source/test differences; do not infer normative precedence from
the API exception list. Keep the process regional-attribute DDM chart first in
the paused visual-review queue.

---
### 2025 process Release Denied versus cancellation after owner callback entry — 2026-10-16

Audited the configured-process boundary after an owner-side release callback
has entered user code. If cancellation is already reserved before the callback
entry gate, the registry suppresses the callback; the existing HLA_IMMEDIATE
buffered-release case exercises that pre-entry path. After entry and accepted
cancellation, the denial planner can mark the still-pending attribute
unavailable and consume the cancellation reservation, while the cancellation
confirmation begin gate can instead remove the pending attribute first. The
source therefore shows competing commit gates, not an unconditional
Release-Denied winner. Configured-process HLA_EVOKED and HLA_IMMEDIATE denial
tests cover ordinary denial only; no focused test covers the after-entry race.

Corrected the guide's prior overbroad statement and stale test/source anchors,
added a source-only decision chart, and corrected the process denial evidence:
both callback models query and confirm the original owner remains owner. The
current 2025-only Mermaid 12.0.0 syntax pass accepts all 295 diagrams across
31 guides; rendering and visual review remain pending. The cancellation and
Release Denied guide sections had 72 local links/anchors checked, all valid.
No build, test rerun, CI, or Requirements Lab work was performed.

Next handoff: audit 2025 configured-process regional class-request region
filtering against the embedded-first explanation and focused process tests.
Update the guide only if bounded evidence identifies a useful reader gap.

---
### 2025 process ownership cancellation vs divestiture notification — 2026-10-16

Audited the configured-process race between Cancel Attribute Ownership
Acquisition and a regular acquisition notification created by Unconditional
Divestiture. The registry skips notification work when cancellation is already
reserved; if work was scheduled first, its begin gate checks the cancellation
reservation again. The gate runs before event send in push mode and during the
recipient's receive poll in pull mode. If cancellation wins, this notification
does not transfer ownership and the cancellation confirmation follows its
separate event path; if notification begin wins, the registry assigns the
requester before callback projection. Keep these as Umbra source-order facts,
not HLA race-priority or public callback-timing rules.

Process tests cover HLA_EVOKED/pull cancellation confirmation for an ordinary
pending request, plus HLA_IMMEDIATE/push cancellation while the requester is
idle and suppression of a buffered owner-release callback. They do not include
Unconditional Divestiture, so the combined acquisition-notification race
remains untested. The ownership guide corrects the process cancellation
evidence, adds a source-only race flowchart, and links the separate test paths.
The inventory is 313 diagrams total (178 flowcharts). A fresh 2025-only
Mermaid 12.0.0 core-parser pass accepted all 294 diagrams across 31 2025
guides, including both ownership flowcharts. This confirms syntax only; no
browser render or visual review was performed. The regional-attribute DDM
chart remains first in the paused render queue. No build, test rerun, CI, or
Requirements Lab work was performed.

Next handoff: audit the 2025 configured-process Release-Denied-versus-
cancellation race at owner callback entry. Compare when the owner response
enters user code with the requester's cancellation reservation, map the
process registry/event gates and focused tests, and update the guide only if
the source/test evidence establishes a distinct reader outcome. Keep
normative ordering claims unverified without the standard text. Documentation-
only and 2025-only; no builds, test reruns, CI, or Requirements Lab edits.

---
### 2025 process Unconditional Divestiture: pre-existing regular request path — 2026-10-16

Traced the configured-process path for a regular acquisition that is already
pending when the owner unconditionally divests. The registry removes the old
owner, then replans pending acquisition work; its source can schedule the
first eligible waiting request for an unowned attribute. The process event
path differs by transport: push begins/revalidates the notification before
sending, while pull begins it when the recipient polls. The registry can
establish ownership before the callback bridge exposes
`attributeOwnershipAcquisitionNotification`. These are implementation-path
observations, not HLA arbitration, fairness, or public callback-timing claims.

No focused configured-process test prequeues a regular request before
Unconditional Divestiture. The companion process acquisition case requests
only after querying the attribute as unowned; process assumption-offer tests
do not create regular pending work. The ownership guide now explains this
source-only branch and distinguishes it from that later-request test. Its
existing process flowchart was clarified, so a fresh syntax parse, render, and
visual review of that chart is pending; Mermaid browser-launch access remains
the known renderer limitation. No build, test rerun, CI, or Requirements Lab
work was performed.

Next handoff: audit 2025 configured-process cancellation of a regular
acquisition notification after it has been queued by the divestiture path.
Compare push pre-send and pull receive-poll revalidation with focused test
coverage, then update the ownership guide only if the current cancellation
explanation misses a distinct outcome; otherwise record a no-change
disposition. Preserve the process regional-attribute DDM sequence first in
the paused visual-review queue. Documentation-only; no builds, test reruns,
CI, or Requirements Lab edits.

---
### 2025 configured-process Unconditional Divestiture: selected recipient paths verified — 2026-10-16

Reconciled the attribute-ownership guide's process evidence against the 2025
configured-process handler, registry plan, enqueue/send helpers, callback
bridge, adapter, and focused Catch2 cases. The guide now records tested
owner-to-unowned observation, a later explicit regular acquisition, and
ownership-assumption offers under both `HLA_EVOKED` and `HLA_IMMEDIATE`.
It also corrects the process If-Available row to cite competing-request and
mixed available/still-owned attribute cases in pull and push modes, without
claiming a universal winner or callback order.

The regular acquisition test requests only after the attribute is observed
unowned; no focused process case was found for regular acquisition already
pending when unconditional divestiture executes. Source shows the registry
plan commits before recipient enqueue/send helpers, which may fail after
earlier recipient events have been inserted or sent; no rollback/compensation
or injected-failure test was found. Keep that edge source-only, not a claimed
public runtime outcome or HLA rule. The portable TCK scenario is not recorded
as a process run; its source and catalog links were refreshed after a bounded
check found stale line anchors. No chart was needed; no build, test rerun, CI,
or Requirements Lab work was performed.

Next handoff: audit the 2025 configured-process regular-acquisition-already-
pending branch during Unconditional Divestiture. Trace the pending-work plan,
recipient event and callback projection, and focused-test coverage; update the
guide only if this clarifies a distinct outcome, otherwise record a no-change
disposition. Keep the process regional-attribute DDM sequence first in the
paused visual-review queue. Documentation-only; renderer retry remains paused.

---
### 2025 ordinary process late-subscription values: existing guide matches bounded evidence — 2026-10-16

Rechecked the 2025 process object-subscription path against the service,
discovery enqueuer, initial-value planner call sites, and focused tests. The
ordinary subscription handler plans discoveries for already-registered
instances. The enqueue path commits known-object state before exposing the
discovery event, then creates an initial attribute-update event only for an
RTI-owned MOM object. A source scan finds the ordinary initial-reflection
planner only on the embedded callback route. The process event path therefore
has no ordinary initial-value event in the reviewed route; this is source
evidence, not a normative conclusion or a measured public-process result.

The public process discovery case subscribes before registration and observes
discovery only. The private post-registration subscription case first polls
discovery, then adds another attribute and checks the owner's relevance
advisory; it does not check subscriber value reflection. The process MOM case
does observe selected initial values, but only for its RTI-owned
`HLAfederate` objects. Push and pull enqueue order is not a global callback
ordering claim. The licensed §5.8 text has not been reviewed, and no focused
ordinary process late-subscription reflection observation was found.

The existing [object-attribute subscription guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge)
already separates object awareness from stored-value reflection, describes
the process MOM-only branch, cites the relevant source paths, and retains the
public-test and normative gaps. No duplicate chart or prose change was needed.
No build, test rerun, CI, or Requirements Lab work was performed.

Next handoff: audit the 2025 configured-process Unconditional Divestiture
recipient-work boundary in the attribute-ownership guide. Trace registry
commit, follow-up acquisition and ownership-assumption recipients, process
event enqueue/send, callback delivery, and focused tests. Keep the post-commit
queue/send failure edge source-only unless tests establish its outcome; add no
duplicate chart if the existing explanation is sufficient. Preserve the
process regional-attribute DDM sequence first in the paused visual-review
queue.

---
### 2025 Auto Provide after discovery: existing guide sufficient; process solicitation remains unverified — 2026-10-16

Audited the 2025 Auto Provide path across the embedded registry planner,
discovery callback, provider callback adapter, configured-process discovery
service, and focused tests. The embedded planner reads the federation-wide
Auto Provide switch, filters to current provider-owned attributes in the
receiver's current scope, and groups requests by provider. The provider
callback adapter rechecks the switch, membership, ownership, and scope before
calling user code. The automatic solicitation carries an empty tag; it does
not contain values. A later provider `updateAttributeValues` call is a
separate action with its own application-supplied reflection tag. Existing
focused tests cover the empty automatic tag, regional eligibility and a scope
change during discovery, multiple providers, and queued-work suppression after
a switch change.

The configured-process discovery path commits known-object state and emits a
discovery event, but the scanned `planAutoProvideForDiscovery` call site is
the embedded discovery route. The process `HLAsetSwitches` path updates the
shared switch value, and the process provider callback path is exercised for
explicit Request Attribute Value Update; neither establishes automatic
process solicitation. No focused process Auto Provide solicitation test was
found. Keep this as an implementation/test boundary, not a normative or
embedded/process parity conclusion.

The existing [Auto Provide guide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md) already
separates solicitation, later value reflection, profile behavior, and test
limits, so no duplicate diagram or prose change was needed. No build, test
rerun, CI, or Requirements Lab work was performed.

Next handoff: audit the 2025 ordinary process late-subscription initial-value
edge already marked unresolved in the subscription-state guide. Trace
known-object state, discovery planning, initial-value paths, process event
delivery, and focused tests. Keep this separate from Auto Provide and
RTI-owned MOM reads; update the existing guide only if the review closes a
concrete reader gap, otherwise record a bounded no-change disposition. Keep
the process regional-attribute DDM sequence first in the paused visual-review
queue.

---
### 2025 process Federation MOM after additional-module Join: existing sequence sufficient — 2026-10-16

Rechecked the Federation MOM guide's configured-process sequence against the
Join service, discovery, MOM value-planning, and focused test paths. On an
additional-module Join, the process service commits the new definition and
membership, establishes/resolves the execution-scoped `HLAfederation` record,
then queues discovery for the joining member's distinct `HLAfederate` object.
The reviewed Join path has no federation-object Conditional update enqueue.
For an observer subscribing after that Join, the process path can discover
`HLAfederation` and send only its Static initial attributes; it does not
backfill the federation module-list or Current-FDD Conditional fields.

A direct request can instead project the live federation definition through
the RTI-owned MOM value path, subject to membership and known-object checks.
The `HLAcurrentFDD` value comes from the current materialized definition, not
the joining member's per-member module list or a stale MOM snapshot. The
embedded path separately queues the federation Conditional updates after a
module-adding Join; its focused test observes a changed Current-FDD reflection
and confirms a later direct request returns the same value.

The configured-process additional-FOM public test checks class lookup/name,
not Federation MOM discovery or reflection. A bounded scan of process-named
tests found no focused observer assertion for either federation Conditional
field. The existing MOM sequence and FOM-guide cross-link already separate
federation reflection, direct current-value requests, and membership-driven
`HLAfederate` discovery, while preserving this test and normative gap. No
duplicate chart or prose change was needed. No build, test, CI, Requirements
Lab, or 2010 work was performed.

Next handoff: audit the 2025 Auto Provide discovery path across embedded and
configured-process sources and focused tests. Trace switch state, per-attribute
provider/scope filtering, tag behavior, and the separation between solicitation
and the provider's later value update. Add no chart if the current guide already
answers the reader question; keep process behavior source-only where untested.

---
### 2025 additional-module Join post-commit rollback: existing explanation sufficient — 2026-10-16

Rechecked the additional-module Join failure boundary across the embedded
ambassador, shared membership registry, configured process service, and
focused test families. The registry prepares membership writes and replacement
handle directories before swapping the new definition and membership into
shared state. After that commit returns, either runtime can fail during joined
member setup. The embedded rollback resigns the new member; the process path
also clears its installed session and pending member-MOM events when needed.
Neither reviewed rollback path restores the previous definition or handle
directories. This is implementation behavior, not an HLA rollback requirement.

The focused embedded invalid-FOM case rejects before commit and then proves the
base definition remains joinable. The embedded service-report writer failure
case proves ordinary-Join membership rollback, but supplies no additional FOM.
The configured-process rollback case injects report-file/session setup failures
on ordinary Join and checks member/session/file cleanup and retry. Process
additional-FOM cases cover successful composition and invalid-module rejection
before commit; neither combines added modules with post-commit failure and a
catalog inspection. Thus the possible retained replacement catalog remains a
source observation, not a tested outcome.

The existing FOM guide already has a flow separating preparation/rejection,
atomic catalog-plus-membership commit, post-commit setup, and member-only
rollback, plus an evidence row naming these test limits. The lifecycle guide
cross-links it. No duplicate diagram or guide edit was warranted. No build,
test, CI, Requirements Lab, or 2010 work was performed.

Next handoff: audit the 2025 configured-process Federation MOM projection
after additional-module Join. Compare the federation-level `HLAcurrentFDD`
reflection path, explicit value requests, and membership-driven `HLAfederate`
discovery against focused tests and the existing MOM/FOM guides. Keep profile
behavior source/test-bounded and add no duplicate chart if current coverage is
sufficient.

---
### 2025 process regional class-request filtering and callback checkpoint — 2026-10-16

Traced the configured-process regional class request through request-region
validation, shared class planning, per-provider persistence, event transport,
and callback entry. The registry validates supplied request regions against
requester ownership, committed bounds, and class dimensions. Its class planner
then groups each live object's requested attributes by current owner and
filters each attribute: the request region must be non-empty; an explicit
provider update-region association must overlap at least one request region;
an attribute without an explicit association uses the default region.

The process handler persists only the region pairs matching each planned
object/provider group and recomputes that group's owner/region projection
before putting an event on the push or pull route. In the reviewed pull path,
the receive service pops the saved event and the client callback bridge submits
it without another registry-region check. The focused process case uses one
object/provider/attribute with matching committed `[0,2]` regions under both
callback models; after the Provide callback, it separately subscribes and
checks a later value reflection. It does not test disjoint filtering, multiple
providers, or a region move after the pre-routing projection. Keep any such
stale-event result source-only and unverified, not a normative HLA rule.

Added a process-specific sequence and evidence row to the Request Attribute
Value Update guide; the embedded regional chart remains separate. Mermaid 12.0.0
syntax parsing accepted all 293 charts across the 31 `HLA-2025-*` guides. This
is grammar acceptance only. The first queued process regional-attribute DDM
chart still cannot render: Mermaid CLI 12.0.0 failed to launch its browser
process with code 2147483651. No build, test, CI, or Requirements Lab work was
performed; no 2010 files were touched.

Next handoff: audit the 2025 Join boundary when additional FOM modules are
accepted and a later Join stage fails. Compare the lifecycle and FOM-composition
guides, current source rollback, and focused tests; keep any unverified catalog
state source-only and avoid a duplicate chart if current coverage is sufficient.

---
### 2025 process class-request provider partitioning: source confirmed, test gap remains — 2026-10-16

Traced the process class-level Request Attribute Value Update through the shared
registry planner and configured-process service. For each live instance, the
planner groups requested attributes by current owner and emits a separate
object/provider plan entry. The process handler consumes each entry separately:
it resolves that provider's session, registers that group's attributes, then
queues or pushes an event for that provider. This source path supports one
object's requested attributes reaching multiple providers without merging the
provider groups.

The focused embedded and configured-process class tests route multiple objects
to one provider; neither checks one object's attributes split across providers.
The process regional-class request test likewise exercises one provider and one
requested attribute. No test evidence establishes the multi-provider process
callback result. Updated the existing RAVU evidence table with the process
dispatch details and retained the coverage limit. The existing flow diagram
already represents separate object/provider groups, so no chart change was
needed; this remains an implementation observation, not a portable HLA rule or
full profile-parity claim.

The first queued 2025 process regional-attribute DDM chart was also attempted
with installed Mermaid CLI 12.0.0. Its browser process failed to launch with
code 2147483651, so no visual rendering was produced and the queue remains
paused. No build, test, CI, or Requirements Lab work was performed; no 2010
files were touched.

Next handoff: audit the 2025 process regional class-request path against the
embedded-first RAVU explanation. Compare per-attribute request-region filtering,
provider update-region overlap, and process dispatch with the focused process
regional class-request case. Add no duplicate diagram if the existing flow
answers the reader question, and keep untested profile behavior explicitly
open.

---
### 2025 class Request Attribute Value Update persistence boundary: existing guide sufficient — 2026-10-16

Rechecked the class-level request path against the current embedded runtime,
registry implementation, focused tests, and the existing Request Attribute Value
Update guide. The guide already separates final request planning from per-provider
registration, states that registry registration revalidates the complete
requested attribute set against the current owner projection, and places
successful service reporting and provider callback queueing after all groups
persist. The reviewed source has no rollback for earlier groups if a later
registration rejects a stale plan. Callback-entry revalidation is documented as
a separate later boundary.

The focused embedded class-request tests cover multiple instances routed to one
provider, and the configured-process case covers multiple instances with one
attribute/provider. Neither transfers ownership in the pre-enqueue or
callback-entry window, nor verifies one object's requested attributes split
across multiple providers. Keep the possible partial persistence and stale-plan
failure source-only: it is not an asserted HLA outcome or a tested public result.
The existing chart and prose already answer the bounded reader question, so no
duplicate diagram or guide edit was warranted. No build, test, CI, or Requirements
Lab work was performed; no 2010 files were touched.

Next handoff: audit the 2025 process class-request partitioning edge for one
object whose requested attributes have multiple current providers. Compare the
planner and process dispatch source with existing focused tests; keep it as a
source/test coverage gap unless durable test evidence establishes an observable
result. Do not add a chart if the existing class-request explanation is enough.

---
### 2025 process RECEIVE temporal gate: no duplicate chart — 2026-10-16

Rechecked the 2025 time guide against the current embedded temporal gate,
process event dispatch, and focused test sources. The embedded receive-order
route checks `receiveOrderDeliveryAllowed` and retains blocked work in the
deferred asynchronous-receive queue. Process attribute and interaction events
instead enter the process callback bridge and submit to the callback dispatcher;
the reviewed bridge path does not consult that time predicate or queue.

The embedded asynchronous-delivery temporal-state test exercises a constrained,
Time-Granted receiver with the switch off, then opens the gate by enabling
asynchronous delivery and later by requesting an advance, under both callback
models. The process receive-order attribute case is not time-constrained; the
process TSO tests establish TSO-before-grant behavior, not a RECEIVE gate. A
test-search hit in the connection-loss TSO-cutoff file is embedded transport
failure coverage, not process-endpoint evidence; the time guide now makes that
distinction explicit. No tracked process test combines a constrained receiver,
asynchronous-delivery on/off, and a pending advance.

No new chart was warranted: the existing time guide already presents the
embedded temporal gate and the distinct process source path, while keeping the
process public outcome unresolved and avoiding a parity or normative claim.
No build, test, CI, or Requirements Lab work.

Next handoff: audit the 2025 class-level Request Attribute Value Update
full-set-validation and per-provider persistence boundary. Compare current
source with the existing guide and focused tests; keep the partial-persistence
interleaving source-only unless durable evidence proves its public outcome. Add
no duplicate chart if the existing explanation answers the reader question.

---
### 2025 discovery versus first reflection: no additional chart needed — 2026-10-16

Reviewed the newly-known-object handoff against the current 2025 object and
interaction guide, embedded callback path, process receive selector, and focused
tests. The existing guide already distinguishes the embedded stored-value
reflection after discovery from the process ordinary-object path, which does
not show a discovery-triggered value snapshot; it also separates a later
explicit process update and the RTI-owned MOM value path. The embedded test
asserts one discovery and one selected-value reflection, but separate recorder
collections do not prove their relative callback order. The process selector
prioritizes discovery before its attribute-reflection slot, and its source
comment cites the standard, but the focused process case uses a later explicit
update rather than proving an ordinary-object initial snapshot or order.

No new chart was warranted: the existing flow and companion subscription-state
comparison answer this bounded reader question while keeping standard
ordering, implementation sequencing, and test evidence separate. The checkout's
2025 import contains API headers only, not the canonical clause text; exact
normative ordering remains unverified. Added that source-availability limit to
the Attribute Scope Advisory guide; its §10.1.3 exception and switch-only test
gap remain source-labeled. No build, test, CI, or Requirements Lab work.

Next handoff: audit the 2025 process RECEIVE temporal-gate boundary against the
existing time guide, process receive/dispatcher paths, and focused tests. Keep
the public outcome unresolved without a focused constrained-receiver,
asynchronous-delivery, pending-advance scenario. Preserve the process
regional-attribute DDM chart as first in the paused visual-review queue.

---
### 2025 configured-process service-report recipient and callback lifecycle — 2026-10-16

Traced configured-process `HLAreportServiceInvocation` from report-request
validation through recipient selection, serial reservation, event enqueue/send,
pull polling, and callback submission. The registry replans eligibility when
reserving the per-reported-member serial. After reservation, the process server
checks only that each planned recipient still has a session in the same
federation; neither pull polling nor the callback bridge rechecks this report's
subscription/DDM eligibility. The exception-report callback recheck is a
separate mechanism and does not cover service-invocation events.

The private process push option defaults to pull. In push mode, the server
sends the event before responding to the report request. If the reporting
federate is also a selected recipient, its synchronous report request drains
the buffered event before returning to the public wrapper, so `HLA_IMMEDIATE`
can invoke the callback before that service returns. A different observer
uses an independent dispatch loop; its immediate callback may occur before or
after the reporter's return. `HLA_EVOKED` queues work for explicit evoke.
These are source-derived process-profile consequences, not normative callback
ordering rules. The shared request-backed/background dispatch routes are
described in the callback/service-ordering guide; this report guide applies
them specifically to service-report recipients.

The focused process matrix exercises a regional report with one subscribed
observer in the default pull mode and `HLA_EVOKED`, checking the invalid
producer marker and first serial. The reporter and observer both use
`HLA_EVOKED`; the server uses its default pull setting. The report-specific
process cases do not test subscription mutation after planning, push transport,
or callback timing relative to the reporting service's return. Partial
push-send failure after serial reservation is source-only. These are
configured-process implementation/test boundaries, not normative HLA rules
or claims about the embedded path.

Added a separate recipient-snapshot and callback-boundary flow to the 2025
service-invocation guide. Mermaid 12.1.0 syntax parsing accepted 26 diagrams
across the updated service-invocation, exception-reporting, and
subscription-state guides. The focused subsection link check validated 23
local links and 22 source line anchors; they remain separate from the
historical aggregate. Rendering remains paused because Chromium launch is
denied. No build, test, CI, or Requirements Lab work was performed.

Next handoff: review the 2025 Object and Interaction Information guide at the
newly-known-object boundary: discovery versus first attribute reflection. Use
available standard text and focused embedded/process evidence to distinguish
the normative callback sequence from implementation commit points; add no
chart if existing coverage answers the reader question. Preserve the process
regional-attribute DDM chart as the first item in the paused visual-review
queue.

---
### 2025 configured-process regional service-report boundary — 2026-10-16

Traced the receive-order and timestamped configured-process regional interaction
send wrappers through the shared server handler. Receive-order preflight calls
the failed-invocation writer for invalid parameter and region handles, but not
for the undefined interaction-class check. Once sent, an accepted response
triggers the success writer; a server rejection is translated to
`RTIinternalError` without a failed-invocation writer. The timestamped process
branch has no service-invocation writer for either success or failure. Existing
process report tests cover successful receive-order and local preflight paths,
not server rejection or timestamped process sends; timestamped regional report
tests located in this review are embedded-only. These are implementation/test
boundaries, not normative HLA rules or profile parity.

Added a separate process regional-send flow to the service-invocation guide.
Mermaid 12.1.0 syntax parsing accepted 25 diagrams across the service,
exception, and subscription-state guides. A focused link check validated 29
local links and 28 source line anchors in the reporting sections; those counts
are not added to the historical aggregate. The new chart remains unrendered
because browser launch is unavailable. No build, test, CI, or Requirements Lab
work was performed.

Next handoff: review configured-process `HLAreportServiceInvocation` recipient
planning, serial reservation, event enqueue, callback dispatch, and recipient
revalidation against focused process tests. Keep report creation distinct from
observer delivery and do not infer normative behavior or 2010 parity. Preserve
the process regional-attribute DDM sequence as first in the visual-review
queue; renderer retries remain paused.

---
### 2025 delete-privilege transfer cross-service reader review — 2026-10-16

The 2025 ownership and object-information guides each covered one side of the
delete-privilege lifecycle, but did not show how transferring
`HLAprivilegeToDeleteObject` changes who may delete while leaving the original
registrant identity intact in the selected embedded scenario. Added one
HLA-first sequence to the ownership guide, including the publication gate, the
If-Wanted divestiture, the failed old-owner delete, the new-owner delete, and
the observer's removal callback. Linked the current embedded test sources and
registry evidence. These tests are source evidence only here; they were not run.

The sequence is explicitly limited to the embedded `HLA_EVOKED` test path.
The process timestamped-deletion route has separate producer metadata and
remains the next focused 2025 review; no cross-profile parity is inferred.
Mermaid 12.1.0 parsing accepted the added sequence, but rendering and visual
review remain pending because browser launch is denied. Updated the working
inventory to 306 charts. No build, test, CI, or Requirements Lab work was done.

---
### Whole-tree Mermaid syntax pass using an in-memory sanitizer adapter — 2026-10-16

Followed the first scan's Node-side DOMPurify failure to Mermaid's bundled
parser initialization. For syntax checking only, an in-memory Node load hook
substituted identity `sanitize` and no-op hook functions; no installed package
or repository tooling was modified, and no diagram was rendered. Mermaid
12.0.0 then accepted all 303 fences across 43 design Markdown files. This
supersedes the initial 106-accepted/197-blocked attempt below. The scan found
one malformed note label in the timestamped-deletion sequence; after removing
the semicolon and slash, that chart parses as well. The adapter deliberately
does not provide sanitization, SVG output, layout, visual, or GitHub evidence.

No HLA behavior changed. Keep 2010 and 2025 streams separate. No build, test,
CI, Requirements Lab, or 2010 implementation work was performed.

Next handoff: continue a bounded non-timing reader-gap audit against topics not
recently surveyed. The syntax queue is clear under Mermaid 12.0.0; visual
review remains pending, with the process DDM sequence first once Chromium can
launch. Verify GitHub rendering after relevant changes are published.

---
### Whole-inventory Mermaid syntax triage and targeted repair — 2026-10-16

Scanned all 303 current Mermaid fences across the 43 design Markdown files
with the installed Mermaid 12.0.0 parser, without attempting browser rendering.
The first pass isolated one real sequence syntax error in the timestamped
deletion chart: a semicolon separator and slash in a note label. Reworded the
label as “Clear producer knowledge, retain object and name”; the focused chart
then parsed successfully. The final whole-inventory pass found 106 accepted
charts, 197 blocked before a syntax verdict by the Node-side
`DOMPurify.addHook is not a function` mismatch, and no other reported grammar
errors. The 197 blocked charts are unresolved, not verified. No SVGs or images
were produced, and visual review remains pending because portable Chromium
launch is denied with Windows `STATUS_ACCESS_DENIED` (`0xC0000022`).

No HLA behavior claim changed. Preserve the distinct 2010 and 2025 guide sets;
no build, test, CI, Requirements Lab, or 2010 implementation work was done.

Next handoff: continue bounded non-timing reader-gap audits while tracking the
197 syntax-unresolved charts separately from the render queue. Retry local
rendering only after browser launch access changes; keep the process DDM
sequence first in that queue and verify GitHub presentation after publication.

---
### 2025 callback dispatch exception/cancellation reader audit and render retry — 2026-10-16

Reviewed the existing 2025 callback/service-ordering guide for a bounded
non-timing reader gap around queueing, exception propagation, accepted service
effects, and cancellation. Its current coverage already distinguishes task
submission from callback invocation, HLA_IMMEDIATE from HLA_EVOKED, embedded
Send Interaction effects from callback failure, process request-backed versus
background dispatch, and dispatcher reset from disable/enable. The guide also
labels source-derived behavior and untested exception paths; this review did
not establish new normative or profile behavior and did not justify another
diagram.

Retried the first queued visual-review item after discovering that Scoop
Chromium is now installed. Mermaid CLI and a direct headless launch both fail
before browser startup with Windows `STATUS_ACCESS_DENIED` (`0xC0000022`); no
preview image was produced. The temporary configuration was removed. Pause
further renderer retries until launch permissions or the environment change.
No build, test, CI, Requirements Lab, or 2010 implementation work was done.

Next handoff: continue the bounded non-timing reader-gap audit against areas
not recently reviewed. Add or revise diagrams only for a demonstrated HLA
reader need. Keep the process DDM sequence first in the visual-review queue,
but defer rendering until the browser can launch; keep 2010 and 2025 separate.

---
### Documentation roadmap refresh and renderer outcome — 2026-10-16

Reconciled the living index with the completed process receive-order timing
audit and the latest passive-subscription, process DSE, DDM, and Attribute
Scope reviews. The active queue is now a bounded non-timing reader-gap audit;
each candidate must result in an HLA-first explanation or a documented
no-change decision. The roadmap separately tracks source/test-only profile
questions, normative questions awaiting canonical standard text, targeted
diagram review, and the intentionally separate 2010 stream.

Tried a changed local rendering environment after the Chrome/Puppeteer failure:
Scoop reported the portable Chromium installation failed, so no browser image
was produced. The process DDM sequence remains syntax-only; visual review is
still pending. Do not repeat renderer attempts until the environment changes.
No build, test, CI, Requirements Lab, or 2010 implementation work was performed.

Next handoff: continue a bounded non-timing reader-gap audit using existing
guides, focused tests, and source. Do not repeat already-disposed passive,
DSE, DDM, or Attribute Scope surveys without new evidence. Keep the DDM chart
first in the visual-review queue, but do not let unavailable rendering block
substantive documentation work.

---
### 2025 ResignAction and Connect/configuration reader audit — 2026-10-16

Compared the HLA-first Connect/Join/Resign/Disconnect mental model and its
focused connection cases with the lifecycle guide's ResignAction orientation,
selected effect table, embedded action cases, and the ownership-persistence
companion. Existing diagrams already separate the ambassador connection,
federation membership, execution lifetime, requested cleanup, admission, and
successful departure. The Connect guide also labels validation precedence and
host teardown as Umbra observations rather than HLA rules. No missing durable
HLA relationship justified another chart for these bounded questions.

The full voluntary ResignAction applicability/effect-ordering matrix remains
unresolved without canonical 1516.1-2025 service text; implementation tables
and selected Catch2 scenarios cannot stand in for it. The official
[IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers purchase/subscription access, and no full standard text is present in
this checkout. Retain the matrix and exact §4.12.3–§4.12.4 interpretation in
the normative follow-up queue. Connect's process host-cleanup assertion gap
and Disconnect/API exception discrepancy also remain explicitly bounded in
that guide; neither supports a general HLA claim.

Next handoff: continue a bounded non-timing reader-gap audit only where the
existing portfolio leaves a concrete HLA question unanswered. Keep the full
ResignAction matrix standards-text dependent, and do not repeat the surveyed
Connect or resignation flows absent new source, test, or reader evidence. No
diagram, source, build, test, CI, Requirements Lab, or 2010 implementation
change was made.

---
### 2025 synchronization partial-result and process Join rollback audit — 2026-10-16

Rechecked the synchronization guide's source-derived sequence for an
unsuccessful achievement followed by an omitted-set late Join. The embedded
registry retains the earlier result, adds the newcomer to the participant and
announcement sets, and waits for that newcomer before planning the final
notification; no current focused test establishes that exact interleaving.
The public embedded scenarios achieve only after the participants have joined.

The bounded source/test scan also found a different process case already in
the tree: a late-Join synchronization announcement is queued, the Join is then
forced to fail, no callback/reflection/announcement-count change leaks, and a
retry produces one announcement. It has no prior achievement result, so it
does not close the partial-result gap. Added this as process-specific evidence
to the synchronization guide; no additional chart was warranted because the
existing state/sequence maps already explain the HLA barrier and label this
uncovered interleaving as source-derived. No 2010 equivalence is inferred.

Next handoff: continue the non-timing reader-gap audit against existing guides
and focused evidence, without repeating the now-disposed synchronization,
ResignAction, or Connect/configuration reviews. The process DDM chart remains
first in the visual-review queue once a working local renderer is available.
No build, test, CI, Requirements Lab, or 2010 implementation work was done.

---
### 2025 process receive-order attribute/time gate audit — 2026-10-16

Compared the time-management and DSE guides with the process attribute
service, client callback dispatch, callback bridge, asynchronous-delivery
service, and the existing public process attribute/TSO cases. The process
attribute handler uses the constrained-recipient set to queue TIMESTAMP
passels; RECEIVE passels are materialized as ordinary process events. The
process callback bridge submits those events to the callback dispatcher
without the embedded `FederateTimeState::receiveOrderDeliveryAllowed` gate.
The checked-in process tests cover ordinary reflection and a single TSO
attribute before its grant, but not a time-constrained RECEIVE passel with
asynchronous delivery off/on, a pending advance, or a mixed RECEIVE/TIMESTAMP
attribute call. The time guide already describes this source/test difference
and keeps the public process outcome open; no duplicate diagram or claim was
warranted. This is source-scope evidence, not an HLA rule or profile-parity
finding. No build, test rerun, CI, or Requirements Lab change was made.

Next handoff: after a renderer/environment change, render and visually review
the process regional-attribute projection sequence in the DDM guide. It now
has syntax-only Mermaid acceptance; keep visual review pending until an image
is actually produced and inspected. Then resume a gap-driven non-timing audit.

---
### 2025 process regional attribute-scope transition review — 2026-10-16

Compared the Attribute Scope and DDM guides with the process notification,
receive, callback-bridge paths, and the focused configured-process test. The
existing guide already keeps producer-facing Turn Updates On/Off relevance
advisories separate from receiver-facing Attributes In/Out Of Scope callbacks
and value reflection. The process service rechecks a planned transition before
it crosses the process boundary; queued receive-order scope events are checked
again when the receiver polls. The callback bridge maps the checked direction
to the corresponding public callback.

The focused process test exercises the receiver's final regional unsubscribe,
later resubscription/In, owner-side association removal/Out and restoration/In,
under both callback models. It does not stage a stale queued-event race or
isolate suppression caused solely by disabling the switch. The existing
process source/test mapping and diagram already state these limits, so no
additional chart or prose was warranted. This is 2025 process-profile evidence,
not a normative algorithm or 2010/embedded parity claim. No build, test rerun,
CI, or Requirements Lab change was made.

Next handoff: audit process receive-order attribute callback eligibility for a
time-constrained, Time-Granted receiver with asynchronous delivery off/on and
a time advance pending. Reconcile bounded source and existing tests; preserve
untested cases as open and add no chart unless there is a demonstrated reader
gap.

---
### 2025 process regional attribute projection review — 2026-10-16

Compared the DDM guide's embedded attribute flow with the process receive-order
planner, receive service, and focused process case. The process path captures
the producer's committed source-region realization for an accepted update,
then re-evaluates the receiver's current attribute declaration and region at
the receive/callback-task boundary. The focused case checks late subscription
with DSE enabled/disabled in `HLA_EVOKED` and `HLA_IMMEDIATE`, confirms that a
later mutation of the producer's live region does not retarget the accepted
update, and suppresses a later update when the receiver moves out of overlap
before delivery in either switch mode.

Added a process-only sequence to the DDM guide for this source/test-backed
two-time distinction and linked it to the broader DSE process-path guide. It
does not assert a general HLA rule or embedded/process parity. No build, test
rerun, CI, or Requirements Lab change was made.

Next handoff: review 2025 process regional attribute-scope advisory transitions
against the Attribute Scope Advisory and DDM guides, bounded callback/source
paths, and focused tests. Keep producer-facing relevance advisories distinct
from receiver-facing in/out-of-scope callbacks and value reflection.

---
### 2025 process Delay Subscription Evaluation review — 2026-10-16

Compared the existing DSE process-path explanation with the attribute-update
planner, receive service, and focused process case. The source distinguishes
receive-order pull and regional push, which recheck at the receive/callback-task
boundary and pass sent-region context when the send supplied it, from the
existing nonregional push path, which re-resolves before event construction.
The focused process case exercises
regional receive-order attributes with DSE enabled/disabled under both
`HLA_EVOKED` and `HLA_IMMEDIATE`, including late eligibility and later
suppression. It does not establish process interaction, TSO attribute, or
nonregional-push DSE behavior.

The guide already presents this timing split and evidence boundary; no
additional chart or prose change was warranted. Keep other process routes
source-only and do not infer 2010 or embedded/process parity. No build, test
rerun, CI, or Requirements Lab change was made.

Next handoff: audit 2025 process regional-attribute projection against the
existing DDM/subscription guides, bounded source, and focused tests. Keep
receiver-region projection distinct from DSE timing, producer-region
snapshots, and process receive-order timing; add documentation only for a real
reader gap.

---
### 2025 passive three-role subscription review — 2026-10-16

Compared the subscription-state guide's “Passive is not a hard delivery veto”
flow with the discovery resolver, per-attribute recipient planner, and focused
subscription cases. The source separates two decisions: a matching active
declaration from a joined non-producer can establish relevance; a candidate
receiver must still have its own matching declaration (active or passive),
known-object/class eligibility, and any applicable regional match. The update
planner applies the same distinction per attribute. The process services reuse
this registry selection logic, but their delivery route remains a separate
profile boundary.

The existing guide already makes those roles and gates visible, so this review
did not add a redundant diagram or prose. The focused passive object-attribute
case has a publisher and one subscriber that changes passive/active state; the
regional declaration case likewise changes one receiver's regional state.
Neither directly tests a separate active establisher plus passive receiver.
Keep the three-role result source-derived, not normative or test-proven, and do
not infer process/embedded parity or 2010 behavior. No build, test rerun, CI, or
Requirements Lab change was made.

Next handoff: audit the 2025 process Delay Subscription Evaluation path against
its current guide, checked-in service/event source, and focused cases. Determine
whether the existing explanation clearly separates subscription-time
eligibility from delayed evaluation and later recipient projection. Add no
diagram unless a genuine reader gap remains; preserve the process/embedded and
2010/2025 boundaries.

---
### 2025 process receive-order attribute eligibility source audit — 2026-10-16

Compared the process mixed-order attribute path with the embedded
receive-order gate, the current process-endpoint guide, and checked-in process
cases. In the process service, time-constrained recipient selection routes
TIMESTAMP passels into the TSO queue; a selected RECEIVE passel is materialized
as a normal process event. The process client forwards that event to
`ProcessFederationCallbackBridge::submitAttributeUpdate`, which submits it to
the callback dispatcher without consulting `FederateTimeState` or its deferred
receive-order queue. The embedded `submitReceiveOrderCallback` path does check
`receiveOrderDeliveryAllowed` and retains blocked callbacks. The time guide now
states this as a bounded source difference—not a normative rule or verified
public process outcome.

Checked-in public process coverage includes one ordinary reflection with
overlapping subscriptions and one timestamped attribute reflection before its
matching grant. The private service test separately observes its TSO event
before the grant. None tests a time-constrained, Time-Granted process receiver's
RECEIVE attribute event with asynchronous delivery toggled or a pending advance.
The current mixed-order test file is untracked and embedded-profile-only; it is
not treated as durable or process evidence. No source/test/CI/Requirements Lab
work was performed, and the diagram count did not change.

Next handoff: audit passive three-role subscription eligibility against the
existing 2025 subscription-state guide, checked-in source, and focused tests.
Determine whether a real reader gap remains before changing diagrams. Keep
process DSE and regional projection as separate queued reviews; preserve the
2010 boundary and do not build or rerun tests.

---
### Documentation roadmap made explicit — 2026-10-16

Reconciled the short action view with the existing guide inventory and active
handoff. The 2025 API-family survey found reader-facing coverage across the
top-level service groups; remaining annotation is therefore gap-driven. The
ordered queue is: (1) process receive-order attribute callback eligibility
under the constrained/granted, asynchronous-delivery, and pending-advance
conditions; (2) bounded reviews of the already-known passive-subscription,
process DSE, and process regional-attribute projection seams; (3) canonical
normative-text follow-up for the specific unresolved clauses listed in the
index; and (4) targeted render/visual review of the currently pending charts.
Each topic must preserve edition and embedded/process boundaries. A review may
conclude that existing diagrams suffice or that evidence is unavailable; neither
outcome calls for a speculative chart.

The mixed-order embedded test source currently present in the worktree is
untracked. Its recorded selector result is provisional and is not treated as
checked-in or durable evidence. This does not answer the process-profile timing
question. No source, test, CI, Requirements Lab, or 2010 implementation work
was performed.

Next handoff: continue the bounded process receive-order eligibility audit in
the existing time-management and process-endpoint guides. Use only checked-in
source and focused cases as durable evidence; keep the mixed-passel result open
unless a tracked case directly establishes it. Do not build or rerun tests.

---
### 2025 interaction event versus object-attribute identity — 2026-10-12

Audited the 2025 object/interaction guide's HLA distinction against the
pinned RTI/FederateAmbassador signatures, selected embedded/process cases, and
the existing detailed recipient flows. The guide already separates
per-instance attribute authority and receiver object knowledge from ordinary
interaction-class routing; the missing wayfinding detail was the *outer* API
identity alongside the shared typed value maps. Updated the existing
"Encoded values enter services" diagram rather than adding a duplicate:
object updates/reflections name an `ObjectInstanceHandle`, while ordinary
interaction send/receipt name an `InteractionClassHandle` and have no target
object. Directed interaction stays a distinct exception/service path.

Evidence remains layered: the embedded case exercises the actual
`receiveInteraction` callback; the process Send Interaction case decodes a
pulled event envelope rather than asserting a public callback; the process
attribute case observes discovery and a later explicit update reflection, not
a discovery-triggered snapshot. These are not matched profile-parity tests.
The pinned headers expose clause labels and signatures, not normative clause
prose; no new service rule or conformance claim was added. The chart count did
not change.

The 2026-10-11 screenshot review applies to the prior chart source only. In
this shell `mmdc` is not discoverable and the prior Scoop install attempt found
no configured manifest, so this revision has no fresh parser, render, or
screenshot verdict. No C++ build, test, CI, 2010, or Requirements Lab work
occurred.

Next handoff: render and screenshot-review the updated existing map with a
GitHub-compatible Mermaid preview; record that result separately from the
historical 2026-10-11 screenshot. Then continue the prioritized HLA-first
guide audit without compiling or rerunning tests. Preserve 2010/2025 and
embedded/process boundaries.

---
### 2025 Federation MOM request/report outcome boundary — 2026-10-12

Reviewed the request-driven Federation MOM path against the pinned 2025 MIM,
the embedded `SendInteraction` handler, and existing focused Catch2 sources.
Revised the existing sequence to separate the caller's synchronous
`sendInteraction` result, the requested RTI `HLAreport...` state response, and
switch-selected per-invocation reporting (`HLAreportServiceInvocation` for the
interaction sink, otherwise a file record). The test supports normal return
before observer evocation and no report after its invalid-index and unexpected-
parameter cases, only in the selected embedded `HLA_EVOKED` path. Source places
the selected per-invocation report step before federation-report enqueue; the
test does not assert both report families or their observer order. Federation
and member request roots remain distinct; no process or 2010 parity is claimed.

No fresh tests, build, CI, or Requirements Lab work. The revised chart has no
new syntax/render verdict: the current shell has no `mmdc`, and the authorized
`scoop install mmdc` failed during a permission-denied Scoop self-update and
then found no manifest. Visual review remains pending in the chart ledger.

Next handoff: bounded HLA-first review of the 2025 object-discovery versus
first-attribute-reflection boundary. Keep the exact §§6.9/6.11 normative
sequence question separate from implementation ordering and focused-test
observations; preserve 2010 and 2025 embedded/process boundaries. Do not build,
rerun tests, or start CI.

### 2025 Service Invocation Reporting guide review — 2026-10-12

Reviewed the guide's HLA model against the pinned 1516.2 MIM definition,
bounded embedded routing/reservation source, and its focused evidence map. It
already separates the public service return or typed exception, the
`HLAreportServiceInvocation` record/interaction, and request-driven Federation
MOM reports. The embedded planner selects a sink from the subject's switches,
scopes interaction reports by `HLAfederate` and `HLAserviceGroup`, and reserves
a serial only when an eligible observer exists. The directed-send test proves
report-before-application-callback order only for its selected HLA_EVOKED case.
The guide appropriately labels its file, timestamp, and ConnectionLost failure
edges as profile-specific and keeps untested schedules open. Its 51 Markdown
links resolve, with no missing file targets or out-of-range line anchors.
No reader gap justified another diagram or content edit; no test was run.

Next handoff: bounded HLA-first review of the 2025 Federation MOM guide. Make
the request-driven `HLArequest...` / `HLAreport...` exchange distinct from
per-invocation reports: identify the request trigger, state source, response
service outcome, and later observer callback. Reuse existing pinned API,
source, and test evidence; add a diagram only for a genuine gap. Preserve 2010
separation and embedded/process evidence boundaries; do not build, rerun tests,
start CI, or edit the Requirements Lab.

### Current Mermaid syntax pass and label repairs — 2026-10-12

The local Mermaid CLI 12.0.0 was already available, so Scoop installation was
unnecessary. A parser-only check over the current 252 Mermaid fences found four
sequence-chart syntax failures caused by semicolon separators. Reworded those
labels in the 2025 TSO retraction, federation synchronization, exception
reporting, and callback/service-ordering guides without changing behavior.
Rechecking the current source found no additional parser errors: 81 diagrams
were accepted, while 171 stopped with `DOMPurify.addHook is not a function` in
the Node parser environment and therefore have no syntax verdict. This is not a
full syntax pass. The separate all-current SVG render still cannot launch
Puppeteer/Chrome, so the four edited charts remain visually unreviewed.

Also reviewed the callback/service-ordering guide against bounded dispatcher,
session, and process-Resign source. It already separates callback-model and
service-contract questions from dispatcher mechanics, consumed-task exceptions
from producer-specific catches, and projection cancellation from queue/session
reset. Its possible post-Resign dispatch path is explicitly source-derived;
focused public evidence and normative permission remain open. No new diagram or
behavior claim was warranted.

No new diagram was warranted; no C++ build, tests, CI, 2010, or Requirements
Lab work occurred.

Next handoff: content-first, bounded HLA-first review of the 2025 Service
Invocation Reporting guide. Separate the operation's public return/typed
exception from the recorded service report, report routing/correlation, and MOM
state; compare the pinned API, bounded source, and existing test evidence. Add
a diagram only for a genuine portable reader gap. Reuse results without builds
or test reruns, preserve 2010 and 2025 embedded/process boundaries, and keep
the Requirements Lab out of scope.

### 2025 discovery versus first reflection — 2026-10-12

Audited the object-information and subscription-state guides against the
pinned 2025 `FederateAmbassador` declarations, the process receive selector,
the embedded callback boundary, and the focused public process update case.
The process selector explicitly selects pending discovery before reflection
for a newly registered object; its comment says this matches official
callback ordering. The focused process case registers, sends an update, and
checks that both `discoverObjectInstance` and `reflectAttributeValues` ran,
but stores independent booleans and does not record callback order. The
vendored interface labels the callbacks §6.9 and §6.11 but does not include
the normative clause prose. The official IEEE downloads page links a
personal-use archive that was retrievable, but its nested package contains
API sources and schemas rather than the standard text; the API header repeats
the clause labels without an ordering rule. The process comment therefore
remains an implementation interpretation, not an independently verified
normative claim. The focused process update's reflection follows a separate,
explicit publisher `Update Attribute Values` call; it must not be conflated
with a discovery-triggered current-value snapshot. In the inspected process
discovery enqueue path, a follow-up initial-value event is built only for
RTI-owned MOM objects. The subscription-state guide records this separately
from the embedded ordinary initial-reflection path and its process test gap.
These are profile-specific source observations, not a 2025 normative rule or
an embedded/process parity claim. See the living index's normative
source-access audit.

Added an explicit evidence boundary in the object guide and clarified that
the subscription chart's vertical callback placement is scenario chronology.
No Mermaid changed, so no render or screenshot review was needed. No build,
test rerun, CI, 2010, or Requirements Lab work occurred.

Next handoff: perform a bounded 2025 HLA-first audit of interaction receipt
versus object-attribute reflection. Verify recipient eligibility and callback
boundaries against the authorized standard, then compare selected
embedded/process source and focused tests. Add a diagram only if this reveals a
portable reader gap. Carry forward the open §5.8 and §§6.9/6.11 questions
without treating Umbra's process selector or tests as normative evidence.
Preserve 2010/2025 and embedded/process boundaries; do not build, rerun tests,
start CI, or edit the Requirements Lab.

---
### 2025 DataElement carrier versus encoded-value length — 2026-10-12

Audited the existing 2025 DataElement guide against the pinned `DataElement`
and `VariableLengthData` APIs, representative basic/fixed-record/variable-array
codecs, the variable-array encoding case, and the separate VariableLengthData
storage tests. The existing guide already covers explicit byte order, nested
composites, counts/padding, time factories, and separation from handles, model
input, and private process framing. The missing reader distinction was between
the public `VariableLengthData` carrier's `size()` and a length/count encoded
inside the value. Added a compact API-surface map: `encode()` returns a new
carrier; selected current codecs replace data in a supplied carrier via
`setData`; `encodeInto(vector&)` appends. A three-layer explanation now
separates carrier octet count, `HLAvariableArray`'s signed BE element count,
and private process-frame metadata. The overwrite detail is source-observed;
the reviewed focused tests do not seed a prefilled carrier then test that
overload. The callback-buffer copy/pointer-lifetime rule links to the object
guide instead of being duplicated. The first of seven existing diagrams was
revised to name the carrier; no new diagram was added. The 2010 guide was read
only to check separation and was not changed; no behavior parity is inferred.

The seven diagrams had prior render/screenshot evidence, but the edited first
flow has not been rerendered. Authorized `scoop search mmdc` found no package in
configured sources; no renderer was installed. No C++ build, test rerun, CI,
2010, or Requirements Lab work occurred.

Next handoff: audit the existing 2025 Federation MOM guide for the HLA-first
distinction between execution-scoped management objects, member-scoped
objects, and observer knowledge. Compare its existing flows with the pinned
2025 MIM/API and bounded source/test evidence; keep implementation-specific
projections labeled, preserve 2010 separately, add no diagram unless a
concrete reader gap appears, and do not build or rerun tests.

---
### 2025 federation membership: rejected Disconnect and post-commit Join effects — 2026-10-12

Compared the local ambassador state chart with the pinned 2025 Disconnect
contract and selected federation-lifecycle test. The chart omitted the
`FederateIsExecutionMember` rejection when a joined caller attempts Disconnect;
added the rejected self-loop and direct API/test links. The guide now also
cross-links the embedded additional-FOM Join edge documented in the FOM guide:
post-commit report/lifecycle setup may fail and remove the new member without
restoring the replacement catalog. That additional-FOM failure is source-only;
the focused writer-failure case covers an ordinary Join, and no test asserts
catalog state after the additional-module failure. No normative rule or
process/2010 parity is inferred.

The existing Mermaid state diagram was revised but not rendered. `scoop search
mmdc` found no package in configured sources even with the authorized network
lookup; no renderer was installed. Markdown fence checks are balanced. No C++
build, test rerun, CI, 2010, or Requirements Lab work occurred.

Next handoff: audit the existing 2025 Data Element encoding guide against the
pinned API and focused codec evidence. Clarify the HLA-facing boundaries among
value encoding, byte order, variable-length data, and RTI-specific framing;
keep the 2010 codec and evidence separate, add no diagram unless a concrete
reader gap remains, and do not build or rerun tests.

---
### 2025 FOM module admission and additional-module Join commit boundary — 2026-10-12

Reviewed the module-admission/composition guide against the pinned 2025 API
and bounded embedded registry, ambassador, and test paths. Its DIF,
complete-model composition, and FDD boundaries remain clear. The Join diagram
previously stopped at the atomic registry commit, while the prose implied that
all later public-service failures preserve the old catalog. It now shows the
post-commit service-report/lifecycle setup: a failure there resigns the new
member but does not restore the replacement definition. This is an
implementation-specific source observation, not portable HLA behavior. The
existing writer-creation failure test covers ordinary Join and member-name
reuse only; it does not test an additional-module failure after commit or
inspect the catalog state. Existing successful-addition lookup cases remain
embedded 2025 evidence; the process case covers the joining client's lookup
only. No 2010 parity is inferred.

The revised diagram was not rendered: `mmdc` is unavailable, and the prior
authorized Scoop install attempt found no configured manifest. Syntax/render
and visual inspection remain pending. No C++ build, test rerun, CI, 2010, or
Requirements Lab work occurred.

Next handoff: audit the existing 2025 Federation and Federate Lifecycle guide
for the HLA-first membership mental model across Join, resignation, connection
loss, and execution destruction. Separate portable participant/execution state
from Umbra service rollback and callback-observation mechanics; preserve 2010
separately and do not build or rerun tests.

---
### 2025 Federation Synchronization: report result vs service failure — 2026-10-12

Audited the announce/achieve/federation-synchronized path against the pinned
2025 API, the embedded achievement service, the notification submission path,
and the focused public synchronization scenario. The existing diagrams already
separate registration and announcement from participant reports and aggregate
completion. The reader gap was at the `synchronizationPointAchieved` call
boundary: `successfully=false` is accepted result data, whereas an
unannounced/removed label raises the typed
`SynchronizationPointLabelNotAnnounced` exception. Added this distinction and
direct API/test/source links to the fan-in section, and clarified the existing
sequence so the service return and aggregate callback are not presented as one
event.

The test accepts both true and false reports, observes the false reporter in
`failedToSyncSet`, and checks the typed exception on a report after completion.
The embedded service submits final notifications inside its service method after
the registry operation; paired HLA_EVOKED/HLA_IMMEDIATE cases do not assert
callback-entry order against the final service return. Process and 2010 parity
remain unclaimed. `scoop install mmdc` failed because no configured manifest was
found, so the revised existing sequence remains unrendered. No C++ build, test
rerun, CI, or Requirements Lab work.

Next handoff: audit the 2025 FOM module admission/composition guide's module
dependency and Join boundaries. Keep portable module-composition rules distinct
from Umbra parser/registry mechanisms and callback timing; compare the current
diagrams with pinned API and bounded implementation/test evidence. Preserve
2010 separately, add no chart unless a concrete reader gap appears, and do not
build or rerun tests.

---
### 2025 Attribute Ownership: synchronous service boundaries — 2026-10-12

Audited the If-Available and If-Wanted paths against the pinned 2025 API,
ownership-service entry points, and their focused test bodies. The diagrams
already separate pending acquisition from callback-time outcomes, and
If-Wanted's output-set transfer from later notification delivery. The reader
gap was that synchronous typed exceptions and subsequent state-query evidence
were distributed across the prose and diagrams. Added a comparison table to
the ownership guide: service exception/return, per-attribute callback, and
`isAttributeOwnedByFederate` are now explicit independent boundaries.

The If-Available case checks typed publication, handle, and ownership errors;
it observes false ownership after normal return and true ownership after the
notification callback begins. The If-Wanted case checks typed service errors,
an empty result with retained ownership, and a synchronous nonempty transferred
set whose new owners are observable before callbacks are evoked. These are
selected embedded `HLA_EVOKED` examples, not a complete exception matrix or
process-parity claim. 2010 remains separate. No chart changed, so no Mermaid
render was needed; no C++ build, test rerun, CI, or Requirements Lab work.

Next handoff: audit the 2025 federation-synchronization guide's announce,
achieve, and synchronized milestones. Keep the portable HLA protocol distinct
from callback dispatch and service-return timing; compare the existing diagrams
with the pinned API and bounded source/test evidence. Preserve 2010 separately,
add no chart unless a concrete reader gap appears, and do not build or rerun
tests.

---
### 2025 known-object subscription: independent effect gates — 2026-10-12

Audited the late-subscription comparison against the bounded source/test
evidence. The known-object diagram previously sent a scope-change decision to
both the receiver's scope-advisory path and the unresolved new-attribute
current-value question as mutually exclusive branches. It now computes a
per-attribute scope delta and evaluates the two effects independently; the
provider-facing relevance advisory remains a separate path. This avoids
implying that a scope notice carries a value or rules out a value callback.

The source trace shows subscription mutation returns receiver scope-change and
owner relevance-advisory plans, not current-value recipients
([registry](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L261)).
The discovery planner skips instances already known to the receiver
([planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L83)),
while the embedded ordinary initial-value planner runs after discovery enters
user code
([callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L375)).
The focused unknown-to-known test asserts discovery and the selected stored
value
([test](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180)).
The additive-subscription test instead changes per-attribute modes after
discovery and checks a later publisher update, not an intervening value-only
callback
([test](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L355)).
Process evidence remains bounded: the private advisory case tests owner
notifications, and the process initial-value enqueue has a separate RTI-owned
MOM path. No focused ordinary process late-subscription value test settles the
question. Licensed §5.8 wording remains unreviewed; no normative result is
claimed.

No new chart was added and the inventory count is unchanged. The revised
diagram remains unrendered because the configured Scoop buckets have no
`mmdc` manifest. No C++ build, test rerun, CI, 2010 work, or Requirements Lab
work occurred.

Next handoff: audit the 2025 Attribute Ownership guide's If-Available and
If-Wanted acquisition/divestiture outcomes. Separate service acceptance and
typed exceptions from per-attribute callbacks and later ownership queries;
check existing diagrams against the pinned API, bounded source, and focused
tests. Keep 2010 separate and do not build or rerun tests.

### TSO retraction walkthrough: embedded interaction boundary — 2026-10-12

Compared the portable retraction orientation with the mixed-recipient example
and its interaction test body. The portable chart stays at the HLA question
level; the sequence uses an embedded queue and recipient ledger, so it now
labels itself as one 2025 development-profile interaction example. Replaced
the ambiguous `B+L<T` notation (where B was also recipient B) with the
producer-side §8.22.3 legality gate. The test uses `HLA_EVOKED`; recipient A
evokes the original callback before Retract, so the sequence describes
callback ordering rather than a synchronous callback from `sendInteraction`.
It now links the separate attribute-update recipient comparison in the time
guide.

The interaction test body asserts delivered-recipient callback order and
queued-recipient suppression, but no fresh test result is recorded. The test
is source evidence only in this increment; no C++ build or test ran. The
diagram's sequence structure changed only in its participant/step labels and
one explanatory note. `scoop install mmdc` found no manifest, and Scoop search
could not reach its remote sources; the revised diagram remains unrendered.
No 2010 or process-profile parity is inferred, and the Requirements Lab is
untouched.

Next handoff: audit the 2025 object/interaction information guide's
known-object/new-attribute path against the pinned API, bounded implementation
sources, and existing focused tests. Distinguish initial-value behavior from
DSE-delayed delivery of an earlier send; keep embedded/process evidence and
2010 separate. Add no chart unless a concrete reader gap requires it, and do
not build or rerun tests.

### 2025 timestamped attribute mixed fan-out — 2026-10-12

Audited the time-management guide's queue lifecycle against the focused
embedded timestamped attribute-update case. The queue state diagram now says
it follows an individual TSO-queued copy, not every recipient of a timestamped
service. Added a compact recipient table: the time-constrained recipient's
copy is withdrawn before its grant and gets no reflection or Request
Retraction; the non-time-constrained recipient receives the timestamped
reflection with sent order TIMESTAMP and received order RECEIVE, then receives
Request Retraction after the reflection in the observed callback order.

The [focused test](../../cpp/tests/timestamped_default_region_attribute_mixed_fanout_catch2.cpp#L155)
asserts those outcomes and the shared valid retraction designator; the
[attribute adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_attribute_update_services.cpp#L637)
shows the constrained-recipient queue selection. The existing [TSO
retraction-guide mixed-recipient timeline](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#a-mixed-recipient-walkthrough)
already teaches the generic delivered-versus-pending split, so no duplicate
chart was added and the diagram inventory remains 249. This is embedded
attribute evidence only, not process-profile or 2010 parity.

No diagram was changed, so no renderer was needed. No C++ build, test rerun,
CI, 2010 work, or Requirements Lab work occurred.

Next handoff: audit the TSO retraction guide's mixed-recipient walkthrough
against the available interaction and attribute evidence. Keep its generic
HLA distinction separate from the selected embedded attribute case and from
process event routing; preserve the existing caveat that the interaction
mixed-fanout test has no fresh result. Do not rebuild or rerun it.

### 2025 Delay Subscription Evaluation: recipient-specific TSO boundary — 2026-10-12

The DSE guide already separated eligibility from sender validity, regional
predicates, callback dispatch, and time advance. The audit found one wording
risk: “receiver TSO delivery/grant boundary” could read as a gate for every
timestamped recipient. The two affected diagrams now limit that lane to a
time-constrained receiver. Added evidence that a non-time-constrained recipient
can follow a receive-order callback path in the embedded profile; embedded
interaction/attribute callback source rechecks the current projection at
callback entry, while the process interaction service re-resolves a deferred
recipient before event construction. The eight focused DSE test files are all
embedded; their timestamped cases enable time constraint, so the unconstrained
DSE path and process behavior remain source-observed, not DSE-test-demonstrated.
The 2010 guide remains separate and no cross-edition rule is inferred.
Two changed diagrams were not rerendered: configured Scoop buckets still have
no mmdc or mermaid-cli manifest. No C++ build, tests, CI, 2010, or Requirements
Lab work occurred.

Next handoff: audit the 2025 time-management guide and bounded mixed-fanout
evidence for timestamped messages delivered to time-constrained and
non-time-constrained recipients. Check that TSO queue/grant, receive-order
callback, retraction, and per-recipient order distinctions are explicit without
treating them as a single pipeline. Keep all evidence 2025-specific and do not
build or rerun tests.

### 2025 DDM region-template deletion gate — 2026-10-12

Compared the 2025 region-template state chart with the focused embedded
region-lifecycle tests and registry usage ledger. The existing chart already
separates a region's fixed dimension set, pending bounds, and committed routing
snapshot, but omitted the distinct handle-lifetime gate. Added a small adjacent
flow: live regional subscriptions or object update-region associations cause
`RegionInUseForUpdateOrSubscription`; release of the last dependency permits
deletion, after which a later query fails with `InvalidRegion`. Some
object-removal paths retain an association until their final removal callback.
Evidence remains embedded 2025-only: passive regional subscription
([test](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L134)),
registered-object update association
([test](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L221)),
callback-delayed association release
([test](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L369)),
and registry usage guard/ledger
([delete guard](../../cpp/src/internal/federation/federation_registry_regions.cpp#L320),
[usage ledger](../../cpp/src/internal/federation/federation_registry_regions.cpp#L732)).
The same test confirms a stale-handle query fails after deletion
([assertion](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L161)).

I did not install a renderer. `scoop install mmdc` and `scoop install
mermaid-cli` found no configured-bucket manifest; the first command refreshed
Scoop bucket metadata but installed no package. I did not send chart text to a
remote renderer. The new chart remains unrendered and requires visual review.
No C++ build, test rerun, CI, 2010 work, or Requirements Lab work occurred.

Next handoff: audit the 2025 Delay Subscription Evaluation guide's distinction
between the time eligibility is evaluated, sender-service validity, receiver
DDM eligibility, receive-order callback dispatch, and TSO grant eligibility.
Reuse existing evidence and diagrams if they already teach this clearly; keep
the 2025 embedded/process and 2010 boundaries intact, and do not build or rerun
tests.

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
API/service categories to find any major learner-facing behavior area that is
missing or only explained as Umbra implementation detail. Inspect a bounded
relevant API/docs and source/test slice; add an HLA-first guide only if a real
reader gap is demonstrated. Keep 2010 separate and defer visual review until
an approved local renderer is available.

### 2025 API-family coverage survey — 2026-10-11

Compared the current flow-guide inventory with the seven service families in
the pinned `RTIambassador` interface: Federation Management, Declaration
Management, Object Management, Ownership Management, Time Management, Data
Distribution Management, and RTI Support Services. The existing reader-path
matrix links each family to one or more topic guides; the 29 2025 flow guides
plus two separate 2010 guides leave no service-family-level gap. A targeted
read of RTI Support Services found corresponding paths for names/handles,
update rates, order/transportation, dimensions/ranges, advisories, callbacks,
time factories, and switch/report controls. This is a topical survey, not a
method/overload or exception-contract completeness claim; the index already
warns that its table is not method-by-method coverage.

Next handoff: do a bounded method-level audit of the 2025 RTI Support Services
surface against the existing topic paths. Identify any public operation whose
HLA meaning is genuinely absent (not merely not spelled with its C++ method
name); add only the smallest HLA-first explanation that closes a demonstrated
reader gap. Keep 2010 separate and leave visual review pending until an
approved local renderer is available.

### 2025 Support query distinctions — 2026-10-11

Continued that method-level survey with object-instance lookup and DDM
dimension/range queries. The names-and-handle guide now links
`getObjectInstanceHandle(name)` and `getObjectInstanceName(handle)` to the
pinned API and the existing configured-process test; owner-side round-trip
and unknown-name/handle results are process evidence, while peer discovery
timing remains a separate embedded case.

The DDM guide now gives separate names and meanings to class-available
dimensions, a region's fixed dimension set, and its per-dimension range
bounds. It links the pinned class-availability, `getDimensionHandleSet`, and
`getRangeBounds` declarations; the conceptual FOM-to-region chart includes
region-dimension read-back. Embedded lifecycle evidence establishes that the
shape is queryable after creation before ranges are committed, that an empty
shape is zero-dimensional rather than a wildcard, and that pending bounds can
be read before commit. This does not establish process DDM parity.

No new guide was warranted. This is a focused semantic clarification, not a
complete RTI Support method, overload, exception, or normative-clause audit.
The visual review remains pending: standard configured Scoop buckets have no
Mermaid CLI manifest, and no external preview of repository-derived diagram
text was used. No build/test/CI, 2010, or Requirements Lab work.

### 2025 RTI Support switch-control distinction — 2026-10-11

The bounded follow-up compared remaining Support query, advisory, and callback
controls with their existing guide paths. The cross-cutting switch map now
distinguishes four getter-only federation switches marked `Static` in the
2025 MIM from federation-wide Auto Provide, which is `Conditional` in the MIM
and is changed through Umbra's embedded federation MOM path. The existing
Auto Provide and Federation MOM guides hold the behavior details; the main
service-family map links readers to the clarified control taxonomy. This is
not a method/overload/exception audit or a process/2010 parity claim. No new
diagram was warranted or changed, and no C++ source, build, tests, CI, or
Requirements Lab work occurred. Scoop's configured-host lookup failed DNS;
because no diagram changed, `mmdc` was not needed for this increment.

### 2025 Ownership completion signals — 2026-10-11

The bounded 2025 ownership survey found a real reader gap in two completion
signals. The attribute-ownership guide now distinguishes regular-acquisition
cancellation acceptance from the later confirmation callback, including the
publication/If-Available gates that persist until confirmation in the focused
embedded case. It also documents `attributeOwnershipDivestitureIfWanted`'s
synchronous `divestedAttributes` output separately from the later
acquisition-notification callback; the test observes ownership changed before
evocation and checks the divestiture tag. The RTI Support API declarations and
the existing 2025 embedded target/tests anchor the new material; no normative
arbitration rule or process/2010 parity is inferred.

Replaced the overlapping generic state chart with a compact regular-request
and cancellation flow, and added one scenario sequence for Divestiture If
Wanted. Both changed charts were rendered locally with Mermaid CLI and
screenshot-reviewed. Scoop has no configured `mmdc` manifest, so the CLI was
run from a temporary install outside the repository. No C++ source, build,
test rerun, CI, 2010 stream, or Requirements Lab work occurred.

Next handoff: do a bounded 2025 Object Management lifetime survey against the
pinned API and the object/interaction, attribute-ownership, and lifecycle
guides. Focus on the distinction among federation-wide delete, caller-local
forget, delete privilege, and receiver removal observation; add only a
demonstrated reader gap. Keep 2010 separate and preserve embedded/process
evidence boundaries. Do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Delete-privilege publication epoch — 2026-10-11

The Object Management lifetime review confirmed that the guides already
separate federation-wide deletion, caller-local forgetting, delete privilege,
and receiver-side removal. The additional gap was narrower: `HLAprivilegeToDeleteObject`
is an instance attribute whose implicit publication follows a class
publication epoch, so unpublishing it can split registrations of one class
into different delete-eligibility cohorts. Added a sequence to the object /
interaction guide using the existing 31-assertion embedded
`DELETE_OBJECTS_THEN_DIVEST` case: a retained object loses the privilege,
republishing affects a later registration but not the old instance, and the
resignation scenario removes only the later object while offering the
retained object's `Efficiency` ownership to a peer. The chart labels the
observed assumption-before-removal callback order as case-specific.

The pinned 2025 MIM, the registry's §5.1.2 / §5.3.3(f) source comments, and
the test-plan row (mapped to §4.12) anchor the explanation. The test does not
prove the complete standard publication predicate or process parity. All 18
Mermaid charts in the object / interaction guide rendered locally; the new
sequence was visually checked. No C++ source, build, test rerun, CI, 2010
stream, or Requirements Lab work occurred.

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

The same visual pass found a real clarity issue in the HLA-level
synchronization map: successful registration was drawn as two parallel `Yes`
outcomes, making a registrar result and participant announcements look
mutually exclusive. The diagram now shows registration accepted before the
two distinct notification paths, without imposing callback order. All seven
diagrams in the synchronization guide rendered with the temporary local
Mermaid CLI; the revised HLA-level map was visually inspected. This is a
conceptual API-event map, not a claim about exact normative callback ordering.
No C++ source, build, test rerun, CI, 2010 stream, or Requirements Lab work
occurred.

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

### 2025 late-subscription normative access and diagram-renderer follow-up — 2026-10-11

The official [1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers purchase/subscription access. IEEE SA's [access-options
page](https://standards.ieee.org/access-standards/) also offers read-only
access to selected standards, but this check did not establish that
1516.1-2025 is among them or expose §5.8. Keep the late-subscription rule
unresolved until an authorized copy or excerpt is available; the embedded
source comment is not normative verification.

The user authorized a Scoop install if needed. With socket access, searches
for mmdc and mermaid-cli returned no matches in the configured Scoop catalogs;
no package was installed. The late-subscription chart and three earlier
charts remain pending render/visual review. Do not switch package managers
without approval. This access/tooling check adds no HLA behavior claim and
does not justify another diagram by itself.

Next handoff: continue a bounded HLA-first source/test review only if it finds
a concrete reader question not already answered by the guide set. Keep the
ordinary process late-subscription outcome unresolved. Verify §5.8 only from
authorized text, and review pending diagrams only with an available renderer.
Do not build, rerun tests, start CI, edit the Requirements Lab, or merge the
2010 and 2025 behavior streams.

### 2025 callback re-entry: HLA permission versus dispatcher capability — 2026-10-11

A bounded review of the callback/service-ordering guide, dispatcher/session
source, and focused unit cases found that the main reader trap is already
handled: a recursive dispatcher can support nested same-thread callback work,
but that implementation capability does not grant blanket permission to call
RTI services from callbacks. The guide separately identifies public guards
for Connect, Disconnect, Join, Resign, and Evoke, and distinguishes internal
session close from the public Disconnect service. The focused tests cover
Evoke rejection and session close behavior, not a complete service-permission
matrix. The 2025 normative matrix remains unverified without the exact
standard text.

No new diagram or duplicate prose is warranted. Next content audit: trace the
HLA-facing question of a FederateAmbassador exception during immediate versus
evoked delivery—what call boundary observes it, and which accepted service,
report, or state effects remain—using exact API authority where available and
separate embedded/process case studies. Draw only if the existing exception,
service-outcome, and reporting guides leave a real conceptual gap. Keep 2010
separate; do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 callback data lifetime: borrow during the call, own what you retain — 2026-10-11

The proposed callback-exception audit found its HLA-facing call-stack and
report/state boundaries already described in the callback, exception-reporting,
and process-endpoint guides; no duplicate exception chart was warranted.
Instead, the value-path survey found a smaller but practical gap for federate
implementers: callback handle/value maps and tags are passed by `const&`, and
the 2025 `VariableLengthData::data()` pointer is not promised beyond its
object's lifetime or next replacement. Added a receiver-side flow to the
object/interaction guide: decode through callback arguments while active, or
copy needed data into application-owned storage before returning. The diagram
is explicitly 2025 C++ API guidance, not a private-frame or 2010 claim.

The embedded receive-order interaction test supplies a concrete safe pattern:
it copies the map and tag inside `receiveInteraction` and checks the saved
bytes after callback draining. This is selected test evidence, not a general
callback-lifetime conformance test or process-profile proof. Static census is
now 233 Mermaid blocks (133 flowcharts, 77 sequences, 23 state diagrams); the
new chart has not been rendered or syntax-checked, and the existing visual
review queue remains open. No C++ source, test, build, CI, or Requirements Lab
change occurred.

Next handoff: audit the 2010 FederateAmbassador and VariableLengthData API,
source, and focused tests independently for callback payload lifetime. Add a
separate 2010 explanation only if its own evidence supports one; do not reuse
the 2025 chart or infer edition parity. Keep the new 2025 chart and the prior
three pending diagrams on the render-review queue; Scoop has no Mermaid CLI
manifest, so use another installer only with explicit approval. Continue to
leave the ordinary 2025 late-subscription rule unresolved until authorized
§5.8 text is available.

### 2010 callback payload lifetime: use borrowed values now or copy to retain — 2026-10-11

The independent 2010 API/source pass confirmed its own reader-facing lifetime
guidance: reflection and interaction callbacks receive value maps and tags by
const reference; `VariableLengthData` permits deletion of the source after a
copy, but its raw `data()` pointer is valid only while its owning object remains
alive and unchanged. The 2010 reference runtime invokes callbacks inline, a
boundary already described in its guide. Added a separately labeled 2010
flowchart there: inspect borrowed values during the callback or copy the
needed value objects before returning. Its binding-shell smoke covers callback
delivery, not retention after callback return; this is API-use guidance, not a
test claim or inference from the 2025 stream.

The current static census is 229 Mermaid blocks across 32 of 41 design files:
14 diagrams in two 2010 guides, 213 in 31 2025 guides, and two in the index;
133 flowcharts, 75 sequences, and 21 state diagrams. The new 2010 chart and
the 2025 credential-envelope chart have not been locally rendered or
parser-checked. The user-authorized Scoop install/search found no Mermaid CLI
manifest, so no renderer was installed and no alternate installer was used.
No C++ source, test, build, CI, or Requirements Lab change occurred.

Next handoff: continue with the HLA timing mental model, surveying existing
2025 time-management diagrams for one specific learner gap around how
regulating/constrained federates, advance grants, and timestamp-ordered events
interact. Reuse existing source and focused-test evidence; add a chart only if
the current timing guide leaves a real conceptual gap. Keep 2010 independent,
do not resolve normative questions without authorized text, and do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 FOM composition and identity: existing reader flows cover the seam — 2026-10-11

The bounded FOM/name-resolution review found no missing reader mental model
that warrants another chart. The guides already separate per-module schema
admission, complete-set composition, FDD materialization, and atomic additional-
module Join; then distinguish class-hierarchy member lookup, typed opaque
handles, execution-scoped model symbols, and federate-local object knowledge.
They cross-link the composition-to-lookup path without treating a successful
model update as discovery or instance visibility.

The reviewed evidence includes cross-module reference-resolution cases,
embedded object-class lookup after an additional-module Join (including
stability of an earlier handle), inherited attribute lookup through a child
class, and the corresponding inherited-parameter cases. These are selected
embedded 2025 results, not proof of process parity or general conformance. The
FOM and name-resolution guides already state those bounds; no guide edits or
new chart were needed, and no tests were run.

Next handoff: review the separate 2025 Join-during-save/restore boundary. The
pinned API lists `SaveInProgress` and `RestoreInProgress`, while the inspected
embedded/process paths and interlock test do not settle exact Join timing or
participant effects. Compare only with authorized §4.11 and save/restore text;
do not infer the normative outcome from the exception list or Umbra source. If
the text is unavailable, retain the open question and proceed to another
documentable HLA area. Keep the 2010 stream separate; no build, test rerun, CI,
or Requirements Lab edit.

### 2025 timing and synchronization gap check — 2026-10-11

The timing scan found the existing guide already teaches the target
relationship: regulating and constrained roles; request versus grant; grant
bounds and modes; TSO eligibility/order; callback observation; and lookahead
changes/re-evaluation are handled as distinct but connected steps. The
synchronization guide also already has a source-labeled partial-achievement /
late-join sequence and plainly marks that exact interleaving as untested. No
new chart was justified; the latter remains an Umbra source observation, not a
normative 2025 claim. These were reader-gap checks, not builds or test runs.

Next handoff: survey the existing FOM composition and name/handle guides for a
genuine HLA mental-model gap around module composition, class inheritance,
model identity, and runtime handle resolution. Keep schema admission,
inheritance, and runtime lookup distinct; reuse existing evidence and add no
diagram if current guides already make those boundaries clear. Do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 authorization gate: no duplicate flow warranted — 2026-10-11

Audited the authorization guide against the pinned 2025 Authorizer interface,
Connect lifecycle path, shared connected-operation gates, and the Join
embedded/process split. The existing guide already makes the important HLA
distinction: the three authorizer callbacks are operation-scoped admission
decisions; an AUTHORIZED result does not guarantee the subsequent service's
own checks, state transition, or result. Its existing flows also show
operation-specific result mapping, credential inputs, and where Connect
commits. No second authorization chart was justified. Added direct links to
the separate service-invocation and exception-reporting explanations.

Source review confirms the connected-operation helper snapshots credentials
under the state lock, releases it before plugin code, and serializes calls
with Disconnect under the authorizer lock. The common gate precedes both
backend branches, but the cited operation cases are embedded-profile evidence;
the guide correctly does not claim process parity. Existing test results were
not rerun or upgraded. The [IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers purchase/subscription access; the [access options page](https://standards.ieee.org/access-standards/)
describes Reading Room access to select standards and sponsored GET downloads,
but neither confirms the exact normative clauses needed here are publicly
available. Keep the save/restore Join and Connect-overload questions open.
No C++ work, build, test, CI, or Requirements Lab change.

Next handoff: audit the existing 2025 callback-dispatch exception and
cancellation explanations for a reader gap around callback failure, already
committed protocol state, and withdrawn pending work. Reuse the callback/
service-ordering and exception-reporting guides plus their already indexed
source and focused-test evidence. Add no duplicate flow if those boundaries
are already clear; do not infer normative ordering or process/2010 parity.

### 2025 callback failure and cancellation: no duplicate flow warranted — 2026-10-11

The callback/service-ordering and exception-reporting guides already make the
important distinctions: a callback exception can unwind the inducing service
under HLA_IMMEDIATE or a later Evoke call under HLA_EVOKED; the failed
dispatcher task is consumed while later FIFO work remains; and producer-local
catches can change that path. The embedded Send Interaction example places
accepted reporting/counter effects before callback fan-out and labels
exception attribution and rollback behavior as source-derived, not tested or
normative. Cancellation, disabling, and callback exceptions are treated as
different transitions. No additional diagram or prose expansion was
justified.

The focused dispatcher suite covers normal completion and queue behavior but
has no throwing-task case. No cited public integration test throws from
Receive Interaction, and the process Resign cases do not assert the outcome
of unrelated queued work after resignation. The guides already state these
evidence limits. mmdc is absent, and the authorized Scoop search returned no
Mermaid package match; no alternate installer was used. Existing pending
charts remain unrendered and must not be described as visually verified. No
build, test, CI, or Requirements Lab work.

Next handoff: survey the 2025 ownership-transfer and resignation guides for
one learner-facing timeline that connects ownership state, ResignAction
effects, and callback invalidation. Compare only their existing API, source,
and focused-test evidence; keep action-specific claims and embedded/process
boundaries distinct. Add no overview if the current guides already explain
the transitions without conflation, and do not infer normative ordering or
2010 parity.

### 2025 ownership and resignation: existing flows stay separate — 2026-10-11

The bounded review found no need for a combined ownership/resignation diagram.
The lifecycle guide explains ResignAction vocabulary and selected cleanup; its
ownership companion isolates the role-sensitive pending-acquisition guard and
member-scoped cancellation; the core ownership guide covers its separate
transfer and invalidation cases. Those are adjacent effects, not one atomic
HLA state machine. Combining them with post-Resign callback routing would also
risk implying embedded/process parity.

Added a direct link from the lifecycle guide to the process callback guide and
labeled the preceding dispatcher-reset description as embedded-specific. The
process trace is source-derived: it shows a possible route for queued work but
does not establish post-Resign delivery or normative permission. The inspected
focused tests do not assert unrelated queued-task behavior after process
Resign. No normative action matrix or 2010 claim was added. No diagram was
created, so mmdc was not needed; existing render/visual-review gaps remain.
No build, test, CI, or Requirements Lab work.

Next handoff: survey the 2025 object-instance identity/lifetime path across
registration, deletion, timestamped delete/retraction, and name reuse. Compare
the object-management, name-reservation, TSO-retraction, and time guides with
their existing source and focused tests. Decide whether a single HLA-first
timeline clarifies execution-side existence versus federate-visible state;
keep receive-order/TSO and embedded/process evidence distinct, and add no
diagram if the current guides and links already explain the lifecycle.

### 2025 object identity and timestamped deletion: existing views suffice — 2026-10-11

The bounded review found the core identity/lifecycle distinctions already
explained in their owning guides: the name-reservation guide models Available,
Reserved, and Registered and ties reusable-name state to global object/name
purge; local deletion forgets one federate's view; accepted global deletion
cleans each known receiver independently; and timestamped deletion has a
separate queue/retraction path, including a source-specific reconstitution
branch.
Combining these into one lifecycle chart would conflate reservation state,
execution-wide object existence, per-federate knowledge, and TSO delivery.

Added cross-links and a focused note in the name-reservation guide: current
source can retain backing object/name state while a timestamped delete is
retractable, and its delivered-delete retraction path restores invocation-time
state before Request Retraction. The cited focused test checks receiver-side
lookup during a pending delete and after pre-delivery retraction, then unknown
status after a later removal; it does not assert reuse after purge or exercise
the delivered-delete restoration branch. These remain source/test-bounded 2025
observations, not a universal HLA timing or name-reuse rule. No 2010 parity.
No new chart, build, test, CI, or Requirements Lab operation.

Next handoff: survey the 2025 attribute update-rate admission and time guides
for whether one learner view should connect requested update intervals,
per-attribute admission/callback filtering, and TSO time eligibility. Compare
the pinned API, existing diagrams, source, and already indexed focused tests.
Keep rate admission distinct from timestamp-ordered delivery, embedded and
process evidence separate, and add no combined chart if current guides already
make those boundaries clear. Do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 attribute update rate and TSO: existing synthesis is sufficient — 2026-10-11

The bounded reader review found the relationship already explicit in the
attribute update-rate guide's third flow: logical-time rules select eligible
TSO work, then the selected embedded receiver path applies a per-attribute
steady-clock gate to best-effort values. A reflection may be omitted when all
values are suppressed. The focused test still permits one later public
`retract` call and rejects a repeat; it does not show that rate suppression
alone invalidates the sender's retraction handle at the recipient boundary.
The time guide explains the TSO queue and grant boundary but does not itself
show this later value filter. A second combined diagram would duplicate the
rate guide's receive-order/TSO comparison.

Added a reciprocal note and direct links from the time guide to the focused
mixed reliable/best-effort TSO test and the rate guide's cross-boundary flow.
The focused embedded test suppresses the callback but still allows one later
public `retract` call; a second call is rejected. The guide now states that
narrow observation instead of claiming suppression makes the message no
longer retractable at the recipient boundary.
The sources and tests cited by that guide establish selected embedded
behavior, not normative cross-service ordering or process parity. A bounded
process scan found public update-rate query coverage, but those tests check
query results rather than per-value callback admission; process delivery
filtering remains unclassified here. No 2010 inference, new chart, build,
test, CI, or Requirements Lab work.

Next handoff: trace 2025 process-endpoint attribute reflection and TSO delivery
to determine whether/how subscription update rates affect delivered attribute
values. Compare process client/server projection and callback paths with
existing process integration tests; distinguish public rate-query coverage
from delivery-admission evidence. Record any unverified path narrowly, and do
not infer non-conformance or parity from a symbol search. No build, test
rerun, CI, or Requirements Lab change.

### 2025 process endpoint update-rate boundary — 2026-10-11

The bounded process trace found that subscription rate metadata is transported
and stored: the public process-subscription case checks an active `High` rate,
then a passive subscription with an empty designator, and the resulting server
registry snapshots. Separate process cases exercise named and per-attribute
numeric rate queries, but not delivery under different rates.

The inspected receive-order and TSO event builders select projected values by
the recipient's received-attribute set; they do not consume the projection's
`maximumUpdateRatesByAttribute` or call `UpdateRateGate`. Client dispatch sends
the event through the process callback bridge to `reflectAttributeValues`.
The process delivery-reduction lane covers event routing, overlapping
subscription deduplication, and disjoint regional scope, not rate throttling.
Updated the rate guide with this source-only process boundary and direct test
links. No process conformance conclusion or 2010 inference was added. No new
chart was warranted: the existing process-endpoint guide already shows the
callback route, while this audit identifies an unresolved profile behavior.
No build, test rerun, CI, or Requirements Lab work.

Next handoff: survey the 2025 late-subscription path for an already registered
object. Compare the existing object-attribute subscription guide with clause
5.8, embedded and process discovery/initial-value sources, and focused tests.
Keep object discovery, initial reflection, and later value eligibility
separate; decide whether the process initial-value edge is merely unverified
or a source-level discrepancy, without making a conformance claim. Reuse the
existing HLA-first diagrams unless the survey finds a real learner gap. Do not
build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 late-subscription callback boundary — 2026-10-11

The existing subscription guide already separates discovery, per-receiver
known-object state, initial values, and later updates. A current-source check
confirmed that the process subscription path emits a discovery event; the
process callback bridge maps discovery and attribute-update events to distinct
public callbacks. The focused MOM case observes discovery and reflected values
but does not assert callback ordering. The ordinary process initial-value
result and the licensed §5.8 rule remain open. Expanded the existing process
comparison diagram to show these callback boundaries without claiming process
parity or a portable ordering rule. No C++ source, test, build, CI, 2010
behavior, or Requirements Lab content changed.

Next handoff: audit the separate 2025 and 2010 DataElement guides as an
edition-boundary review. Keep HLA's typed-value/encoding contract distinct
from Umbra-specific byte-order and serialization choices; verify that any
shared explanation is independently supported by each stream. Reuse existing
codec source and test evidence, leave normative claims open without authorized
standard text, and do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025/2010 DataElement boundary and cross-service gap recheck — 2026-10-11

Compared both DataElement guides with their pinned `RTI/encoding` header
inventories. Both declare `HLAvariantRecord`; only the 2025 inventory includes
`HLAextendableVariantRecord`, `HLAlogicalTime`, and
`HLAlogicalTimeInterval` DataElement headers. The separate 2010
`LogicalTime`/`LogicalTimeInterval`/`LogicalTimeFactory` API remains outside
that encoding-header package. Updated both guide boundaries without changing
either edition's codec description or adding a diagram.

A follow-up survey found the proposed cross-service synthesis already covered
by the cross-cutting guide and its reader paths: it distinguishes request
admission, state, per-recipient eligibility, pending work, protocol completion,
and application observation. The object/interaction guide also connects FOM
member type to a compatible DataElement, encoded octets, and the typed-handle
value map. No umbrella chart was warranted. No Mermaid diagram changed, so
`mmdc` was not needed; no build, test rerun, CI, or Requirements Lab work.

### 2025 Join/save-restore admission boundary — 2026-10-11

The existing HLA-first lifecycle and save/restore guides already separate
membership admission from save/restore participation; a combined diagram
would duplicate them. The pinned Join overloads list `SaveInProgress` and
`RestoreInProgress`, but the current focused interlock test joins before save
and does not exercise an in-flight Join. Source review found no explicit
save/restore-phase predicate in the inspected embedded adapter/registry path
or process service/registry path. The process handler serializes Join requests
with one another, which does not establish arbitration with save/restore.
Keep the public admission rule, interleaving result, and callback effects
unresolved without the relevant normative clauses or a focused runtime case.
This is source/profile mapping, not a conformance finding. No 2010 inference,
diagram, build, test, CI, or Requirements Lab change.

Next handoff: survey the separate 2025 resignation-action explanations across
the lifecycle, ownership-persistence, object/interaction, and time guides.
Distinguish the selected action from cleanup of owned objects, pending
timestamped work, active protocols, and effects already observed by peers.
Decide whether a concise HLA-first decision view fills a real reader gap;
preserve embedded/process limits and do not claim a complete normative action
matrix without the relevant standard text. No build, test rerun, CI, or
Requirements Lab edits.

### 2025 resignation and survivor callback-observation boundary — 2026-10-11

The existing action map and evidence already cover caller intent, embedded
cleanup guards, final-member behavior, connection-loss cleanup, ownership
records, and the time cutoff, so a combined action matrix would add no safe
HLA-level rule. A focused review did find a useful observation boundary: the
embedded adapter resets only the departing member's callback queue, then
submits induced work to surviving members' callback paths. One
`HLA_EVOKED` `DELETE_OBJECTS_THEN_DIVEST` case returns from Resign before the
peer observes callbacks; later Evoke observes assumption before removal. A
separate process `HLA_EVOKED` `NO_ACTION` case observes old joined-federate MOM removal
before fresh-object discovery/reflection after rejoin. These are distinct
profile/scenario observations, not a general action-order rule.

Refined the existing lifecycle sequence to show task submission and
application observation separately; added the process MOM case to its
evidence map. No new diagram, normative matrix, 2010 inference, build, test,
CI, or Requirements Lab change. Local visual verification remains pending:
`mmdc` is not installed and the authorized Scoop search failed at DNS lookup.

Next handoff: survey the 2025 federation-member listing operation across the
embedded and process endpoints. Compare the pinned API, public adapters,
process operation routing, and focused tests to determine whether the guide's
one-time snapshot boundary is evidenced in each profile; do not infer a
membership event feed or process parity from query names. Keep 2010 separate
and do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 member-list, process-callback, and late-subscription follow-up — 2026-10-11

The bounded member-list survey found a one-time snapshot and callback path in
the embedded adapter; no matching process transport operation or client route
was found. The lifecycle guide now labels that profile boundary and leaves the
process invocation outcome untested. A separate process callback review
clarified that a matched response remains inside the client request until
buffered event dispatch succeeds; a throwing HLA_IMMEDIATE callback can
therefore prevent the service method from receiving the response, while the
dequeued event is not retried. The current focused tests do not inject that
callback failure.

The subscription-state guide now separates a subscription that makes an
existing, not-yet-known object discoverable from adding an attribute after the
receiver already knows the object. Embedded focused evidence observes selected
stored values in the first case; the second case's ordinary current-value
behavior remains unresolved. Explicit Request Attribute Value Update and
RTI-owned MOM direct reflection are distinct paths. The process ordinary
late-subscription result and licensed §5.8 interpretation remain open. These
are separate 2025 evidence boundaries; no 2010 behavior is inferred.

No new Mermaid chart was added in these increments. The revised lifecycle
ResignAction sequence still needs local syntax/render and visual review.
`mmdc` is absent from PATH and the available Node runtime cannot resolve the
Mermaid CLI, Mermaid renderer, Puppeteer, or Playwright packages. The registered
Scoop buckets contain no Mermaid manifest, and the user-authorized
`scoop install mmdc` returned no manifest. Approval to use npm/npx was requested
and is pending. Do not claim visual verification or switch package managers
without authorization. No C++ build, test rerun, CI, or Requirements Lab work.

Next handoff: if npm/npx is approved, obtain the Mermaid CLI for a local render
and visual review of the revised 2025 ResignAction sequence. If not, keep that
check explicitly pending and continue the bounded HLA topic survey, selecting
a real reader gap from existing guides and focused evidence. Preserve the
2010/2025 and embedded/process boundaries. Do not build, rerun tests, start CI,
or edit the Requirements Lab.

### 2025 name/handle identity domains — existing guide coverage is sufficient — 2026-10-11

The bounded audit followed the order/transportation guide's reader-review
handoff. The names/handle guide already distinguishes execution-scoped FOM
symbols, hierarchy-scoped members, federate name/handle lifetimes, and
federate-local object-instance visibility. Its public-handle section separates
typed opaque handle classes and their comparison/hash/encode operations from
Umbra's private numeric representation; normalization outputs are a different
execution-scoped coordinate, not a handle. The focused evidence map states
what is exercised in embedded versus process paths and where parity is still
unknown. No additional diagram or prose was justified, and no 2010 behavior is
inferred.

Next handoff: if npm/npx is approved, render and visually review the pending
2025 ResignAction sequence. Otherwise leave that check pending and continue a
bounded survey for a genuinely uncovered HLA reader question. Do not infer
normative behavior from Umbra source, merge edition/profile evidence, build,
rerun tests, start CI, or edit the Requirements Lab.

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

### 2025 timestamped-attribute projection and delivery gate — 2026-10-11

The cross-guide audit found a focused timing gap rather than a need to repeat
the whole attribute or TSO lifecycle. The existing DDM guide already shows
that an embedded regional timestamped update can be suppressed when the
receiver changes its selector before the callback boundary. The source paths
also expose a process-profile distinction that was not synthesized alongside
the recipient's time gate: embedded delivery resolves the current projection
inside callback dispatch, while process delivery resolves it before creating
and sending the event. A later change cannot rewrite that materialized process
event, but no focused process test was found that mutates the subscription or
region in this interval.

Added one profile-labeled flow and evidence-map row to the 2025 time guide. It
keeps the logical-time eligibility gate separate from current receiver
projection, retained source-region scope, embedded best-effort rate
admission, and the process event-before-grant boundary. A public process
endpoint `RTIambassador` test asserts the attribute reflection before the
matching grant; a separate private transport-service test confirms the event
frame order. Neither test mutates subscription state between acceptance and
event creation. The embedded selector-mutation evidence links to the existing
regional DDM guide. These selected cases and source observations establish no
normative projection-freeze rule or 2010/2025, embedded/process, or
interaction/attribute parity. The process update-rate caveat links to its
existing guide rather than duplicating that analysis.

The new Mermaid chart remains visually unreviewed: the authorized Scoop
attempt could not find an `mmdc` manifest, and no alternate package manager was
used. No C++ source, build, tests, CI, or Requirements Lab content changed.
The working-tree census is 231 Mermaid blocks across 32 of 41 design Markdown
files (135 flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides,
215 in 29 2025 guides, and 2 in the index); this is a fence/type count only.

Next handoff: audit same-recipient ordering between timed federation-save
notifications and TSO payloads at a grant boundary. Prefer a portable timing
model, use Umbra source and already-existing focused tests only as bounded
evidence, and add a diagram only if the current guides leave a real reader
gap. Preserve the separate 2010 and 2025 streams; do not build, rerun tests,
start CI, or edit the Requirements Lab. Keep the timestamped-interaction,
timestamped-attribute, and ResignAction visual reviews pending until a
renderer is available.

### 2025 object existence and federate-local knowledge — 2026-10-11

The portfolio survey selected a useful reader gap beyond timing: the detailed
object guide explains registration/discovery, federation deletion, and local
forgetting in separate service flows, while the cross-cutting guide's short
orientation was prose-only. Added a relationship map that separates execution
existence from each federate's knowledge, then points to later eligibility,
local forgetting, and the distinct federation-wide deletion/removal path.
The map deliberately does not merge those service timelines or their callback
and profile-specific gates. Its source/evidence map remains in the 2025
[object/interaction guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#evidence-map-implementation-versus-exercised-scenarios).

The cited discovery, local-delete/rediscovery, process local-delete, and
removal-payload tests are separate scenarios, not an end-to-end proof; in
particular, the process local-delete case covers success only. This remains a
2025-only synthesis and does not establish process/embedded parity. Its new
chart is not visually reviewed because the authorized Scoop install found no
mmdc manifest; no alternate package manager was used. No C++ build, test
rerun, CI, or Requirements Lab edit was made.

Next handoff: audit the existing 2025 synchronization/save-restore
participant-set comparison against the current source and focused-test
evidence. Keep the two protocols distinct, recheck late-join and partial-
achievement limits, and leave the active Join/save-restore normative question
unresolved absent the relevant standard text. Add no duplicate diagram if the
current map already explains the reader's question.

### 2025 ownership callback queue versus restored work — 2026-10-11

The bounded review found that the core ownership guide and its persistence
companion already explain request-versus-transfer, selected ledger recovery,
recipient validation, and resignation cleanup. A narrower process-profile gap
remained: “pending ownership callback” could mean either an already-pushed
callback waiting in a live HLA_EVOKED queue or value-only ownership work that
the process restore service reconstructs and enqueues after federation-
restore lifecycle notifications.

Added a compact comparison flow to the ownership-persistence companion and a
reader-path note to the federation save/restore guide. The first process test
shows pre-save discovery and ownership-assumption frames left uninvoked through
service-level save/restore, then observed by later Evoke calls before that
scenario's queued save/restore status callbacks. A separate configured
process-endpoint test plus the restore-service source shows reconstructed
assumption work scheduled after federation-restored notifications. The tests
are separate and do not establish process restart, process/embedded parity, or
a normative HLA callback-order rule.

The chart is unrendered: mmdc is not available in this environment and the
previous authorized Scoop attempt found no manifest; no alternate package
manager was used. Static census is now 239 Mermaid blocks across 34 of 41
design Markdown files (139 flowcharts, 77 sequences, 23 state diagrams; 14 in
two 2010 files, 223 in 31 2025 files, and 2 in the index). This is a fence/type
count, not syntax, rendering, visual, or GitHub verification. No C++ source,
build, test rerun, CI, or Requirements Lab content changed.

Next handoff: audit the existing 2025 synchronization/save-restore
participant-set comparison against source and focused tests. Do not merge its
protocol rules; preserve the late-join/test gaps and the unresolved Join
normative question. Keep 2010 separate and make no duplicate diagram if the
existing map already answers the reader's question.

### 2025 synchronization versus save/restore participant sets rechecked — 2026-10-11

The existing cross-cutting map remains the right-sized explanation: it keeps
the expandable synchronization barrier separate from restore roster admission
and from save/restore failure on resignation. Current source still supports
those boundaries. Omitted-set synchronization registration is distinguished
from an explicitly supplied set, and the late-Join hook adds a member only to
an expandable pending point. Retaining a prior achievement when a late Join
expands the barrier remains a source-derived trace; the focused late-Join case
joins before any participant reports achievement. Save participant statuses
are formed from members present when save begins; restore compares saved and
current participant identities before beginning the restore.

The focused interlock case joins both participants before Save and does not
attempt Join during an active save or restore. A bounded review of the related
2025 test cases found no focused active-Join interleaving. The pinned API lists
`SaveInProgress` and `RestoreInProgress` on both Join overloads, but the
inspected Join paths expose no matching active-operation gate; neither source
nor those tests establish acceptance, rejection, or cohort effects. Keep this
as an unresolved API/source question until the exact §4.11 conditions are
available. No new chart was warranted, no Mermaid block changed, and no tests,
build, CI, or Requirements Lab work was performed.

Next handoff: survey the existing 2025 DDM/region guide for a genuine learner
gap in how region extents, subscriptions, region updates, and delivery
eligibility relate. Keep the HLA model primary, use source and existing tests
only as bounded evidence, and add a diagram only if it clarifies that gap.
Keep 2010 separate.

### 2025 DDM concepts and clause navigation — 2026-10-11

Reviewed the current [DDM and regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
as a reader-oriented HLA explanation. Its current flows already connect
dimension availability, region shape and committed bounds, object source
regions, receiver subscriptions, discovery/relevance, interaction selection,
and the independent time/callback gates. No duplicate diagram was warranted.

Added a service-clause navigation table linking the HLA questions to the
pinned 2025 API headings for region lifecycle (§§9.2–9.4), object and
interaction regional services (§§9.5–9.13), and dimension/range queries
(§§10.21–10.28). The table is an index, not a paraphrase of normative rules.
The full exception matrix and combinations of class inheritance with regional
declarations remain open. The guide keeps source/test observations bounded and
does not imply 2010 or process/embedded parity.

Current static census: 233 Mermaid fences across 32 of 41 design Markdown
files (137 flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides,
217 in 29 2025 guides, and 2 in the index). The DDM guide's 14 diagrams were
not parsed or rendered in this pass: `mmdc` was unavailable and the authorized
Scoop install found no `mermaid-cli` manifest. This is not syntax, rendering,
visual, or GitHub verification. No C++ source, build, test rerun, CI, or
Requirements Lab content changed.

Next handoff: audit the remaining 2025 object/interaction class-inheritance
and regional-declaration combinations across the DDM, object/interaction, and
FOM guides. Identify the portable HLA distinctions and exact clause pointers,
compare existing source/test evidence, and add a diagram only if a real reader
gap remains. Preserve the separate 2010 stream.

### 2025 class ancestry feeding a regional object path — 2026-10-11

The bounded audit found a concrete bridge between class-dimension ancestry
and regional object services. The DDM guide now shows the Restaurant FOM
`Food.Drink` → `Soda` → `Light` chain, the three-dimension availability
result for `Light`, and one embedded regional registration/subscription case
where disjointing each source-range dimension suppresses that update's
reflection. Cross-links connect the case to the object/interaction and FOM
composition guides.

Keep its evidence scope narrow: both publisher and subscriber use the same
`Light` class in this case. It demonstrates ancestor-associated dimensions in
that descendant class's available set and regional path; it does not prove a
regional declaration made on a superclass is itself inherited by subclasses.
The analogous interaction-class parent-chain behavior is visible in current
source, but the reviewed query and invalid-context tests do not combine an
ancestor-only interaction dimension with regional send/subscription. Do not
infer process-profile, 2010, or normative parity.

Current static census: 234 Mermaid fences across 32 of 41 design Markdown
files (138 flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides,
218 in 29 2025 guides, and 2 in the index). All local links and line anchors
in the three reviewed DDM, object/interaction, and FOM guides resolve. The
new chart remains unparsed/unrendered: `mmdc` is unavailable and Scoop has no
`mermaid-cli` manifest. No C++ source, build, test rerun, CI, or Requirements
Lab content changed.

Next handoff: survey the 2025 interaction-class ancestor-dimension path in
regional subscription and send, then separate that from whether a regional
declaration on an ancestor class applies to descendant classes. Use the
§10.22, §9.10, and §9.12 API entry points as navigation, compare existing
focused source/test evidence, and keep any unsupported normative consequence
explicitly open. Preserve 2010 as a separate stream.

### 2025 interaction-class ancestry across regional DDM — 2026-10-11

The DDM guide now distinguishes three embedded-source operations that use
class ancestry for different reasons: available-dimension accumulation for a
selected class, regional declarations stored under the exact class handle,
and recipient lookup that begins at a sent class and checks its parents.
For a non-empty regional send, an ancestor-keyed regional entry can be
considered for a descendant send by the current candidate walk when the
regional match is eligible. That is not a child declaration copied at
subscription time, and no normative inheritance rule is asserted. The map
also keeps the federation-wide active-relevance gate separate from each
receiver's class/overlap match and shows the empty-set no-op branches.

Evidence is bounded: source links point to the class-dimension helper,
regional-subscription validation and exact-key storage, send validation, and
the receive-order candidate walk. The Restaurant query's `ServerId` is
declared on `MainCourseServed` itself; the regional case uses that same class
for subscription and send. The reviewed tests do not cover ancestor-only
interaction dimensions through regional services or an ancestor subscription
with a descendant send. API §§10.22, 9.10, and 9.12 are navigation pointers;
the licensed normative text remains necessary for an HLA conformance claim.
The object/interaction and FOM guides cross-link the distinction. No 2010 or
process-profile parity is inferred.

Current static census: 235 Mermaid fences across 32 of 41 design Markdown
files (139 flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides,
219 in 29 2025 guides, and 2 in the index). Local file and line links in the
three reviewed guides resolve. The new chart is not parsed or rendered;
`mmdc` remains unavailable and the authorized Scoop attempt found no
`mermaid-cli` manifest. No C++ source, build, test rerun, CI, or Requirements
Lab content changed.

Next handoff: resume the known 2025 late-subscription initial-reflection
documentation gap. Compare the HLA mental model and clause entry point with
existing embedded/process source and focused tests; decide whether a
reader-centered flow can clarify object discovery versus initial attribute
values without overstating implementation evidence. Keep 2010 separate.

### 2025 late-subscription initial reflection: reader-gap audit — 2026-10-11

The existing object-attribute subscription-state guide already explains the
reader-facing distinction between discovering an object and receiving its
current attribute values, including an HLA-first flow, an embedded V1/V2
scenario, and a separate process-profile comparison. Rechecking the current
tests found no focused ordinary process late-subscription-value case: the
public process discovery path subscribes before registration, the regional
advisory path also subscribes before registration, the private advisory case
tests owner notification after the object is known, and the process initial
value test is specific to RTI-owned MOM objects. The guide correctly leaves
ordinary process initial values unresolved pending licensed §5.8 text and a
focused process observation; no duplicate chart was added.

The static diagram census remains 235 Mermaid fences across 32 of 41 design
Markdown files (139 flowcharts, 75 sequences, 21 state diagrams; 14 in two
2010 guides, 219 in 29 2025 guides, and 2 in the index). No chart was added
or rendered. A network-enabled Scoop search returned no Mermaid CLI package;
`mmdc`, npm, and npx are unavailable. No build, test rerun, CI, or Requirements
Lab work was done.

Next handoff: audit the 2025 Join-during-save/restore admission question in
the save/restore guide. Separate the pinned API exception list, embedded and
process source paths, interlock-test coverage, and licensed §4.11 authority;
add a diagram only if it clarifies participant membership without stating an
unverified outcome. Preserve the 2010 boundary.

### 2025 Join during save/restore: participant-set audit — 2026-10-11

Confirmed that the existing save/restore evidence table accurately separates
the two Join exception annotations from what the available package can prove.
The bounded embedded/process paths show no explicit save/restore admission
predicate, while the focused interlock test has both members join before save;
neither establishes in-flight admission or rejection. The broader lifecycle
guide already has the useful cross-protocol flow for federation membership,
expandable synchronization participants, and unresolved save/restore
participants. The detailed guide now links to that overview instead of adding
a duplicate chart. Licensed §4.11 text remains necessary for a normative
outcome; keep synchronization, save/restore, process, and 2010 boundaries
separate.

Static diagram census remains 235 Mermaid fences. No diagram was added,
parsed, or rendered; the full-set renderer check is still open. No build, test
rerun, CI, or Requirements Lab work was done.

Next handoff: review the existing embedded 2025 resignation-during-save flow
for clarity about survivor notifications and recovery, without conflating
HLA-level membership effects with callback machinery or process/2010 behavior.

### 2025 resignation during active save: callback-model audit — 2026-10-11

The embedded `HLA_EVOKED` sequence correctly shows the participant
consequence: after the callback returns, resignation fails the current save
for the survivors, omits a stale failure for the leaver, and leaves survivors
able to complete a later save. Its callback guard and queued delivery are
scoped to that focused schedule. The `HLA_IMMEDIATE` companion confirms the
same failure reason and later-save recovery for one survivor, but not the
multi-survivor or in-callback guard details. Added that distinction to the
guide's evidence map; no second diagram was needed. The restore counterpart
remains separately sourced and tested; no process or 2010 parity is claimed.

No build, test rerun, CI, or Requirements Lab work was done. Whole-set Mermaid
rendering remains open.

Next handoff: audit 2025 callback exceptions at the boundary between callback
execution, HLA_EVOKED/HLA_IMMEDIATE delivery, and service-level reporting or
accepted-state effects. Preserve profile and edition boundaries; do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 callback exceptions: accepted-send boundary — 2026-10-11

The callback/service-ordering guide now includes a source-labeled embedded
`Send Interaction` sequence. It connects the HLA_IMMEDIATE/HLA_EVOKED timing
distinction to the service's already-accepted report/counter effects: an inline
callback exception can escape the Send Interaction call after acceptance,
whereas an HLA_EVOKED exception arrives later from Evoke. A pre-existing
immediate drainer may own the exception instead. The generic dispatcher and
call-stack diagrams remain complementary, so the new sequence is service-
specific rather than another generic callback chart.

No focused 2025 throwing-callback test was found; the example remains
source-derived, with process event delivery and 2010 callback behavior kept
separate. The current fence census is 242 Mermaid blocks across 34 of 41
design Markdown files (141 flowcharts, 78 sequences, 23 state diagrams; 14 in
two 2010 guides, 226 in 31 2025-named guides, and 2 in the index). Whole-set
syntax/render validation remains open. No build, test rerun, CI, or Requirements
Lab work was done.

Next handoff: establish a supported Mermaid parse/render workflow for the
complete diagram inventory, then syntax-check all blocks and visually inspect
representative dense charts. Do not claim GitHub rendering from local output;
keep 2010 and 2025 separate, and avoid builds, tests, CI, and Requirements Lab
changes.

### Whole-set Mermaid local render audit — 2026-10-11

Mermaid CLI 12.0.0 rendered all 242 diagrams across 34 design Markdown files.
The audit extracted both backtick and tilde fences explicitly because the
CLI's Markdown mode skips tilde fences. Two semicolon-related syntax errors
were fixed in 2025 sequence labels; all diagrams in both affected guides were
rerendered, and the callback-ordering guide's 11 diagrams also render after
the new exception flow was reflowed. A representative 2025 callback sequence
and a separate 2010 DataElement flow were visually inspected. This is local
render evidence only: full-set visual review and GitHub rendering remain open.
No C++ build, test rerun, CI, or Requirements Lab work was performed.

Next handoff: conduct a bounded HLA-first review of the 2025 object-instance
naming flow. Distinguish reservation, its later outcome callback, use at
registration, and peer discovery/local knowledge; check existing API-clause
links and edit only if a genuine reader gap remains. Preserve embedded/process
and 2010 boundaries, keep Umbra behavior bounded, and do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 object-instance naming: portable API lifecycle — 2026-10-11

The bounded review found a reader gap: the guide's first diagram moved
directly from its summary into Umbra's registry/queue trace. Added a compact
API-level lifecycle first, separating reservation admission/outcome,
successful named registration and the owner's handle, and the distinct peer
discovery callback path. It deliberately leaves callback timing relative to
service return unspecified and leaves registration eligibility/visibility
details to the focused sections. The existing detailed diagrams remain
implementation-profile evidence; 2010 behavior is untouched.

The 2025 API-header landmarks were checked for the void reservation service,
separate outcome callbacks, named-registration handle return, and
`discoverObjectInstance` callback. A local re-render was attempted, but the
installed Mermaid CLI could not launch Chromium in this run, and the direct
parser path also failed inside its DOMPurify setup. The new chart is therefore
not render-verified; do not claim GitHub rendering. No C++ source, build, test
rerun, CI, or Requirements Lab work was performed.

Next handoff: resume the known 2025 ordinary-object late-subscription initial-
reflection gap. Compare the HLA mental model and pinned API clause entry point
with existing embedded/process evidence; determine whether a reader-centered
flow would clarify subscription commit, object discovery, and initial value
reflection. Keep the 2010 RTI stream separate and avoid builds, test reruns,
CI, and Requirements Lab edits.

### 2025 late subscription versus delayed update delivery — 2026-10-11

The bounded review found that the subscription-state guide already has an
HLA-oriented interest-change flow, an embedded discovery/initial-reflection
trace, and a separate embedded/process comparison. A new state-machine chart
would repeat that coverage. The actual reader risk was a terminology collision:
an object can be late relative to the receiver's discovery, while an update can
be late relative to send-time eligibility because DSE retains it for later
evaluation. Added a short comparison table separating those clocks and
clarifying that a discovery-time reflection may contain a value previously
set by an update without replaying the publisher's update invocation. The
already-known-object/new-attribute reflection question remains explicitly
unresolved; DSE does not answer it.

The bounded source/test check still supports only an embedded ordinary
initial-reflection path after discovery returns, and a process initial-value
branch for RTI-owned MOM objects. No ordinary process result is established.
Pinned API §5.8 is a navigation point, not a substitute for the unavailable
licensed clause text. No new Mermaid block was added. The current diagram count
remains 243, with the prior HLA-level reservation chart still awaiting local
render verification. No C++ source, build, test rerun, CI, or Requirements Lab
work was performed.

Next handoff: audit the existing Auto Provide and explicit Request Attribute
Value Update comparison against the new initial-state-versus-pending-send
distinction. Check that a reader can follow discovery-time reflection,
provider solicitation, explicit requests, and DSE-delayed delivery without
confusing which federate receives each callback. Add only a concise cross-link
or map if a real gap remains; do not duplicate the existing state machines.
Preserve embedded/process and 2010 boundaries and avoid builds, tests, CI, and
Requirements Lab edits.

### 2025 value-request paths: cross-guide comparison — 2026-10-11

The Auto Provide guide already compares discovery-triggered provider
solicitation, explicit Request Attribute Value Update, initial reflection,
scope changes, and relevance advisories by trigger, recipient, and payload.
The request guide separately provides the HLA-facing solicitation/update/
reflection sequence and its tag and eligibility caveats; the DSE guide
explains delivery-time reevaluation. The substantive comparison was already
clear, so I added only a cross-link from the initial-reflection row to the
subscription-state guide's initial-state-versus-DSE table. No duplicate path
diagram or claim that a requester is itself entitled to receive values was
added.

The four guides keep HLA-level flow distinct from embedded implementation
details and process evidence gaps. No 2010 flow was reused. This was a
documentation-only link change: no new Mermaid block, C++ source, build, test
rerun, CI, or Requirements Lab edit. The inventory remains 243 diagrams; the
previously added reservation overview is still pending render verification.

Next handoff: review the 2025 Request Attribute Value Update guide's open
requester-versus-receiver question. Compare the pinned §6.21 API surface,
object/class/regional implementation paths, and focused tests. Without an
authorized copy of the exact clause, keep any normative entitlement claim
open. Add a diagram only if the bounded survey finds a real reader gap; keep
embedded/process and 2010 boundaries explicit and avoid builds, tests, CI, and
Requirements Lab edits.

### 2025 Request Attribute Value Update: requester and receiver boundary — 2026-10-11

The bounded review found that the request guide already explains the key
implementation distinction: a request solicits provider callbacks, while a
provider's later update uses ordinary receiver planning. The pinned 2025 API
shows the request overloads and the separate `provideAttributeValueUpdate`
callback surface. The embedded baseline first subscribes the requester and
shows provider solicitation (including exclusion of requester-owned
attributes); the process class-request case seeds subscription state directly
in the registry fixture and observes provider callbacks, not a response
reflection. Neither test establishes behavior for an unsubscribed requester.
Regional request filtering also remains distinct from later response DDM.

The guide sufficiently warns that requesting alone does not make the
requester a receiver in the selected implementation. The exact normative
entitlement rule remains open without authorized §6.21 clause text. No extra
diagram or guide edit was justified. Mermaid CLI 12.0.0 is available in the
isolated visualization workspace, but Chromium failed to launch with both its
existing profile and a fresh profile; the reservation overview remains
unrendered. No C++ source, build, test, CI, or Requirements Lab work.

Next handoff: review the 2025 ordinary known-object/new-attribute subscription
case. Compare the HLA/API entry point with embedded current-value reflection,
focused tests, and the process path; keep any normative outcome open without
the licensed clause text. Add a diagram only if it clarifies a real reader
gap. Preserve 2010 and 2025 boundaries, and avoid builds, test reruns, CI, and
Requirements Lab edits.

### 2025 known-object subscription and current-value reflection — 2026-10-11

The bounded review found the subscription guide already gives the reader the
needed three-way distinction: an unknown existing object becoming discoverable
with stored values, an already-known object receiving a new attribute
subscription, and a separate explicit Request Attribute Value Update. Its
state table and receiver-state flow explicitly leave ordinary current-value
reflection for the already-known-object case unresolved rather than implying
that a scope advisory carries a value.

The pinned API anchors ordinary subscription at §5.8 and value reflection at
§6.11. In the embedded 2025 path, the discovery planner skips instances already
known to that receiver, and the current-value planner is scoped to the
post-Discover callback path. The embedded focused tests exercise an object
becoming newly discoverable and its stored value, not adding an attribute to
an already-known object. The process subscription handler also plans
discoveries, while the process initial-value event inspected here is
RTI-owned-MOM-only; the ordinary-process outcome is not established. The
private process test for a late second-attribute subscription observes an
owner relevance advisory, not a receiver value reflection. These are bounded
source/test findings, not a normative conclusion on §5.8 or process/embedded
parity.

No extra chart or guide edit was justified: the existing comparison and
profile-separated flows already expose this exact evidence gap. The normative
question remains open pending licensed §5.8 text and a focused ordinary
process observation. Mermaid CLI 12.0.0 is available locally, but Chrome and
Edge both failed to launch from isolated profiles; the reservation overview
remains unrendered. No C++ source, build, test, CI, or Requirements Lab work.

Next handoff: conduct a bounded reader-first review of the 2025 process
callback-exception path. Compare the public callback exception surface with
the existing process callback and service-ordering guides, and inspect only
the callback bridge/coordinator source and existing focused tests. Keep HLA
callback consequences distinct from Umbra's private completion ACK and do not
infer retry or normative failure semantics from implementation state. Add or
revise a flow only if the cross-guide survey finds a genuine learner gap;
preserve embedded/process and 2010 boundaries, and do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 process callback exception and private TSO completion — 2026-10-11

The bounded cross-guide review found that the two existing guides already
divide this explanation at the right boundary. The callback/service-ordering
guide covers the public `FederateInternalError` surface, callback model, and
which public call may observe an escaping exception. The process-endpoint guide
then traces the private timestamped-delivery consequence. Both guides link to
the other; a synthesis diagram would repeat their existing public-call and
per-recipient process-ACK flows.

The inspected process callback bridge invokes the public callback before its
completion hook. If user callback code throws, control does not reach that
hook; the dispatcher has already extracted the task, and it is not retried.
The selected TSO coordinator entry therefore remains in transit until an
acknowledgment or recipient cleanup. Source shows that in-transit timestamps
participate in time-bound calculations and can block a timed-save readiness
check at or before its save time. Those are Umbra process-profile consequences,
not HLA callback state names or normative error/retry rules.

The focused process fan-out tests exercise normal callback completion and
per-recipient acknowledgment; the pre-callback retraction test exercises
suppression before application callback entry. The inspected tests do not
throw from a public callback or verify post-throw recovery/cleanup. Keep that
as an implementation/test evidence gap, not a missing explanation or a
conformance finding. No guide or diagram edit, C++ source change, build, test,
CI, or Requirements Lab work.

Next handoff: compare the 2025 HLA-facing participant-set explanations for
Join during save/restore and late Join during an expandable synchronization
point. Check the lifecycle map, save/restore Join evidence table, synchronization
flow, and their existing cross-links. Add a comparison only if it prevents a
real reader from carrying synchronization late-join behavior into the
save/restore protocol. Treat API exception annotations as navigation, keep
§4.11 unresolved without licensed clause text, preserve 2010 boundaries, and
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Join and active-protocol participant sets — 2026-10-11

The bounded cross-guide review found the current presentation already prevents
the key category error. The lifecycle guide has a two-branch HLA-facing map:
embedded synchronization-point expansion is labeled as source-derived, while
the save/restore branch stops at the pinned Join exception annotations and an
explicit unresolved participant-set question. The save/restore guide links to
both the synchronization flow and that lifecycle map, then presents separate
API, embedded, process, and test evidence. The synchronization guide separately
explains its expandable-point gate, explicit-set exclusion, achievement
fan-in, and evidence limits.

The synchronization late-join behavior is an Umbra 2025 embedded observation,
not a portable rule to transfer to save/restore. The save/restore Join contract
remains unresolved: both Join overloads list `SaveInProgress` and
`RestoreInProgress`, but the licensed §4.11 text is unavailable and the
focused interlock test does not Join during an active protocol. The existing
map and cross-links are sufficient; another comparison chart would duplicate
them. No guide or diagram edit, source change, build, test, CI, or Requirements
Lab work.

### 2025 regional class-RAVU per-group failure chart review — 2026-10-16

The original per-group failure flow was too broad and wide for comfortable
GitHub-scale reading: it combined per-group registration/reprojection with the
post-preparation Push send loop. Split it at the real "all groups prepared"
boundary. The first chart now shows registration, owner/region recheck, Pull
queueing versus Push staging, and the distinct partial-progress result if a
later registration fails. The second shows successful Pull return or
sequential Push delivery, including the already-sent frame-prefix possibility
on a later send failure.

Both vertical charts passed Mermaid CLI rendering and local visual review. The
preparation loop is tall but legible while scrolling; the completion chart
keeps the final send boundary compact and clear. Nearby prose and source links
continue to identify this as implementation-derived: no focused test injects
either failure, so the possible queued/sent prefixes are not observed callback
outcomes or an HLA all-or-nothing rule. No tests were rerun.

Replacing one chart with two raises the 2025 inventory to 317 diagrams across
31 guides (179 flowcharts, 115 sequences, 23 state diagrams). Twelve selected
charts have local visual review. Next: review the ordinary `Send Interaction`
wrapper-specific failure-reporting flow in the 2025 Service Invocation
Reporting guide, preserving the embedded/process evidence boundary. GitHub
rendering remains pending; no build, tests, CI, or Requirements Lab work.

Next handoff: survey the 2025 seam between additional FOM modules admitted
during Join and name-to-handle resolution. Compare the FOM composition and
name/handle guides, pinned API entry points, and existing source/test evidence;
clarify only what the added module can change for joined federates and how
profile boundaries affect that result. Add a cross-link or flow only if a
reader gap remains. Preserve the separate 2010 stream, avoid unsupported
handle-stability claims, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

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

### 2025 explicit Request Attribute Value Update and provider solicitation — 2026-10-11

The existing request guide already starts with an HLA-facing causal model:
request, optional provider solicitation, a later provider-supplied update, and
separate receiver reflection. It distinguishes object-instance, class, and
regional request scope in the selected embedded profile, notes the requester-
known-object precondition for the object form, and keeps requester response
entitlement normatively open without authorized §6.21 text. Auto Provide and
the RTI-owned MOM direct-reflection path remain separate. The focused process
class case is not treated as profile parity. No duplicate diagram or prose
change was needed.

Next handoff: review the 2025 ownership guide's service-acceptance, per-
attribute outcome, completed-transfer, and later value-delivery boundaries.
Compare its HLA-facing map with the pinned API and existing focused tests; add
no duplicate flow if the current guides already make the distinction clear.
Keep normative and profile claims bounded, preserve 2010, and do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 ownership and later attribute-value delivery — 2026-10-11

The ownership guide separates service return, pending work, per-attribute
outcomes, transfer, cancellation, and later delivery. Its If-Wanted path also
shows a service-result transfer before callbacks, so no one callback-time rule
is implied. The object-information guide already diagrams the later value-
bearing update and receiver gates. The ownership glossary now links to that
path; a third synthesis would duplicate the existing flows. No conformance or
profile-parity claim was added.

Next handoff: compare the 2025 time, ownership, object-information, and TSO-
retraction guides for an accepted timestamped attribute update that remains
queued across a later ownership change. Verify only the cited API/source/test
edges, and add navigation or a flow only if a genuine reader gap remains.
Keep 2010 separate; do not build, rerun tests, start CI, or edit the
Requirements Lab.

2026-10-12: HLA-first Attribute Scope review separated the embedded stale-In
result by callback model: HLA_EVOKED queues In until after a region move, so
the stale In is suppressed; HLA_IMMEDIATE enters In before that later move and
then reports the subsequent Out. The process test covers public unsubscribe
and owner-side In/Out under both models, but does not create a stale-queue
race. The process stale filter remains source-only evidence. Also corrected
the switch boundary: the embedded test's disabled-switch segment is the
self-final-unsubscribe case, already exempt from Out; the process test keeps
the scope switch enabled. Source confirms rechecks, but switch-off suppression
for an otherwise-reportable transition lacks an isolating test. Clarified the
guide and backlog without changing any diagram. Source/test code inspected,
not executed; no render, build, tests, CI, 2010, or Requirements Lab work.

Next: review the 2025 Auto Provide guide as an HLA-first flow. Keep
discovery-triggered RTI solicitation distinct from explicit requests, provider
callback distinct from a later value response, and switch/scope/ownership
checks distinct from DDM and callback observation. Reuse existing diagrams
unless a concrete reader gap remains. Keep 2010 separate; do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 accepted TSO attribute update across a later ownership change — 2026-10-11

The TSO-retraction guide already contains a focused flow for an update accepted
before later divestiture or source resignation. Two embedded `HLA_EVOKED`
tests assert that the queued recipient can receive the original value, tag,
time, producer identity, and retraction designator after the ownership event;
the registry keeps producer identity and per-recipient delivery state. This is
selected implementation evidence only: it establishes neither process parity
nor a portable rule for every receiver-state change. The ownership and time-
management guides now link to that comparison. No new diagram was needed.

Next handoff: survey 2025 object-class ancestry across publication,
subscription/discovery, ownership, and attribute projection. Compare the
object-information, FOM-composition, and name/handle guides; use pinned API and
focused tests only to distinguish portable class relationships from selected
Umbra projection behavior. Add a synthesis only if a reader gap remains.
Preserve the 2010 boundary; do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 class ancestry: lookup and runtime projection are different questions — 2026-10-11

The existing object-information guide already has a cross-path ancestry map:
it keeps object-class and interaction-class chains separate and contrasts
discovery's known-class choice, attribute projection, ordinary interaction
promotion/parameter projection, and directed target/selector lookup. Focused
2025 cases support selected branches: discovery reports the subscribed class;
an attribute-projection case shows a superclass-known receiver omitting a
child-only attribute; an internal registry test demonstrates ordinary
interaction promotion and parameter projection. That internal test is not
public conformance evidence. Directed ancestor declaration and selector
lookup are visible in the shared source walk, but the bounded focused-test
search found no ancestor-class declaration case. The map labels this as
source-only rather than a normative inheritance rule.

The name-and-handle flow correctly describes inherited member-name lookup but
did not directly warn that lookup does not predict callback projection. The
FOM-composition and directed-interaction guides likewise pointed to nearby
flows without linking to the cross-path map. Added direct links from all three
guides to that existing map instead of duplicating it. No source, behavior, or
test changes, and no Mermaid chart changed.

Next handoff: review the source-only 2025 directed-interaction
ancestor-declaration path. Compare the API, shared registry resolver, existing
directed flow, and focused tests to clarify target-class ancestor lookup and
nearest explicit selector behavior, especially child-versus-ancestor
precedence. Keep uncovered behavior source-only; make no normative or 2010
claim. Do not build, run tests, start CI, or edit the Requirements Lab.

### 2025 directed-interaction target-class ancestor precedence — 2026-10-11

The shared 2025 resolver treats FOM pair validity and producer/receiver
declaration applicability as a matching-pair search from the target's
registered object class toward its ancestors. Receiver selector lookup is
different: it returns the first explicit matching class/interaction pair on
that walk, so a child entry precedes an ancestor fallback in the current
source. The focused embedded known-target and process multi-recipient cases
use exact-class declarations; neither tests inherited declarations or
conflicting child/ancestor selectors. The directed guide now says this
explicitly and links the relevant resolver functions. This is source-only
Umbra behavior, not a normative inheritance rule; no new diagram was needed.

Next handoff: survey 2025 regional interaction class ancestry. Compare the
DDM guide and cross-path ancestry map, then check the pinned regional API,
shared resolver, and focused tests for the separate roles of inherited
dimension availability, exact-class declaration storage, and the sender's
class walk used for recipients. Look specifically for parent/child regional
cases. Keep uncovered behavior source-only and do not infer an HLA-wide or
2010 rule. Do not build, run tests, start CI, or edit the Requirements Lab.

### 2025 regional interaction class ancestry — 2026-10-11

The DDM guide already distinguishes ancestor-accumulated interaction
dimensions, exact-class regional declaration storage, and the sender-class-
to-parent recipient walk. The pinned 2025 API and shared resolver support
those separate source paths; the focused regional case uses the same class
for subscription and send, while the dimension query's parent class has no
dimension. Thus the parent/child regional consequences remain source-only,
not an HLA-wide or 2010 rule. Added a reciprocal link to the existing
cross-path ancestry map; no duplicate diagram or behavior claim was needed.
No build, test run, CI, or Requirements Lab edit.

Next handoff: survey 2025 object-class regional declarations across
parent/child registration, subscription, and recipient routing. Compare the
DDM object and interaction guides, pinned API, shared resolver, and focused
cases; distinguish class-available dimensions from declaration keys and
recipient selection. Keep untested consequences source-only, preserve the
2010 boundary, and add a flow or cross-link only for a demonstrated reader
gap. Do not build, run tests, start CI, or edit the Requirements Lab.

### 2025 object-class regional ancestry — 2026-10-11

The DDM guide now separates inherited class-dimension availability from
per-instance source-region associations and exact class/attribute regional
declarations. Its embedded-source flow shows discovery walking from the
registered class toward parents to select the receiver's closest qualifying
known class, then later value projection using that saved class. Parent-keyed
regional declarations may be considered for a child-registered object in this
source path, but are not copied to a child key. Reviewed regional cases use
the same class for registration and subscription; the cross-class behavior
remains source-only, with no normative or 2010 claim. The cross-path ancestry
map links to this focused DDM view. No C++ builds, test runs, CI, or
Requirements Lab edits.

The new Mermaid flow has not been locally rendered. Scoop updated its buckets
but has no `mmdc` manifest; do not claim render verification. Keep it and the
previously pending lifecycle chart in the diagram backlog until a supported
renderer is available or approved.

Next handoff: compare the 2025 process-endpoint object DDM path with the
embedded path documented here. Inspect the regional-declaration handler,
shared registry planning, process recipient/event fan-out, and existing
focused cases to distinguish shared semantics from profile-specific behavior.
Add navigation or clarification only for a demonstrated reader gap. Keep
uncovered behavior source-only, preserve the 2010 boundary, and do not build,
run tests, start CI, or edit the Requirements Lab.

### 2025 process-endpoint regional object boundary — 2026-10-11

Corrected the DDM guide's stale statement that process-endpoint regional
delivery was not covered: separate configured-endpoint cases cover initial
regional registration/discovery, subscription changes, and source-region
relevance transitions. Added a source-bounded flow that separates the shared
registry's state/recipient plan from process-session validation and event
routing, and explains the tested HLA_IMMEDIATE versus HLA_EVOKED initial
advisory boundary. All three regional object cases use the same Soda class on
publisher and receiver; none establishes parent-subscription/child-registration
behavior. The guide explicitly excludes ordinary/TSO update routing and makes
no embedded parity or 2010 claim.

The Mermaid flow is not locally rendered. Scoop has no `mmdc` manifest; no
npm/npx installation was performed. The inventory is now 245 diagrams across
34 design files (144 flowcharts); the three 2025 flowcharts added after the
242-block rendered baseline remain pending render verification. No build,
test run, CI, or Requirements Lab edit.

Next handoff: trace the 2025 process receive-order object-attribute update
from the shared registry's recipient/passel plan through process event routing
and callback visibility. Compare existing focused cases and the embedded
guide, separating receiver eligibility from process delivery mechanics. Add
documentation only for a verified reader gap; keep timestamped routing and
the 2010 stream separate. Do not build, run tests, start CI, or edit the
Requirements Lab.

### 2025 receive-order attribute update: public effect and process mapping — 2026-10-11

Added an HLA-first sequence that separates the producer's untimed, `void`
Update Attribute Values result from an eligibility-conditioned receiver
`reflectAttributeValues` callback. A separate, explicitly process-profile
sequence maps the 2025 endpoint's shared registry plan, recipient projection,
push frame or polling result slot, private recipient count, and callback
bridge. It clarifies that the current public polling pump uses
`receive_interaction` with an adjacent `attributeEvent` slot, while
`receive_attribute_update` names the pushed event; neither detail is HLA wire
protocol. The public focused case asserts one reflection for overlapping
active parent/child subscriptions under both callback models. Its fixture pairs
HLA_IMMEDIATE with push and HLA_EVOKED with polling; that does not establish a
general callback-model/transport rule. Passive three-role, DSE, regional
process projection, and embedded/process parity remain unproven here. The
timestamped route remains separate. No code, build, test, CI, 2010, or
Requirements Lab changes were made.

The inventory is 247 diagrams across 34 files: 144 flowcharts, 80 sequences,
and 23 state diagrams. The two new sequences are not locally rendered; five
diagrams added since the 242-block rendered baseline remain pending visual
verification. Scoop has no `mmdc` manifest, and no npm/npx installation was
performed.

The planned 2025 cross-read of attribute-value updates, update-rate admission,
ownership, and subscriptions is complete. The cross-cutting HLA-first map and
focused object-information guide already connect sender authority and service
outcome, receiver eligibility, attribute projection/rate admission, and
callback observation. No duplicate synthesis is warranted; provider
solicitation and a later value update remain distinct in the request guide.
This disposition adds no normative claim and does not imply embedded/process
or 2010/2025 parity.

Next handoff: render and inspect the five diagrams added after the 242-diagram
baseline: the HLA-level reservation lifecycle in the object-instance naming
guide; the object-class regional ancestry and process-endpoint regional object
flows in the DDM guide; and the HLA-first public-effect and process-mapping
sequences in the process-endpoint guide. Record parse, rendered output,
screenshot inspection, and GitHub rendering as separate evidence. The isolated
workspace has Mermaid CLI 12.0.0, but Chrome failed to launch with existing and
fresh profiles; no install was performed. Keep this render backlog separate
from documentation work.

### 2025 ordinary and directed interaction reader map — 2026-10-11

The current interaction guides already make the service-level contrast and
their implementation evidence legible: an ordinary send has no target object
and uses class subscriptions; a directed send names a target and evaluates
target knowledge, directed declaration pairs, and a receiver selector. The
ordinary and directed callback signatures make the observable target context
explicit. The detailed flows label active relevance, class promotion, selector
lookup, and parameter projection as bounded current-runtime behavior rather
than universal standard rules. No duplicate chart was warranted. See the
[interaction review ledger](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-ordinary-versus-directed-interaction-mental-model--2026-10-11)
for evidence and limitations. No source, build, test, CI, 2010, or Requirements
Lab work occurred.

Next handoff: audit the 2025 reader-facing distinction between a synchronous
RTI service exception and a later callback exception across the exception,
service-reporting, and callback/service-ordering guides. Add a conceptual map
only if it resolves a real gap; do not imply a general rollback guarantee.
Continue using existing source/test evidence without compilation or test
reruns, keep 2010 and embedded/process boundaries explicit, preserve the five
pending local-render checks, and leave the Requirements Lab out of scope.

### 2025 callback exception versus Evoke service exception surface — 2026-10-11

The exception, service-reporting, callback-ordering, and cross-cutting guides
already separate caller outcomes, committed effects, reports, callback entry,
and application observation; no fifth overview diagram was needed. A smaller
API-reader gap is now clarified in the callback guide: the callback's
`FederateInternalError` annotation is distinct from the Evoke methods' own
`@throws` lists. Current Umbra source can propagate an escaping callback error
through Evoke, but the exact normative treatment remains unresolved and the
focused throwing-callback test is absent. See the [review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-callback-exception-versus-evoke-service-exception-surface--2026-10-11).
No build, test, CI, 2010, or Requirements Lab work occurred.

Next handoff: compare save/restore treatment of in-flight work across the main
save/restore guide and focused ownership, request-response, time/retraction,
and callback guides. Do not assume all pending work is persisted or canceled;
classify only documented evidence, and add a comparison or flow only if it
closes a reader gap. Use existing code/test evidence without builds or test
reruns, keep 2010 and embedded/process boundaries explicit, preserve the five
pending render checks, and leave the Requirements Lab untouched.

### 2025 save/restore in-flight work comparison — 2026-10-11

The main save/restore guide now includes a compact index for the distinct
representations readers may call “pending”: provider request/response stages,
typed ownership work, queued TSO delivery/retraction, and callback frames
already admitted to a live `HLA_EVOKED` queue. It links the focused guides and
marks each implementation/test boundary. The audit found that a table was
more accurate than a new flowchart because these are parallel state categories,
not consecutive states in one lifecycle. It makes no universal HLA persistence,
crash-recovery, or 2010 parity claim. See the [review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-save-restore-in-flight-work-comparison--2026-10-11).
No C++ source, build, test, CI, or Requirements Lab work occurred.

Next handoff: review the save/restore guide's unresolved Join-during-operation
boundary using exact IEEE 1516.1-2025 §4.11 text from an authorized source.
Keep participant admission distinct from synchronization achievement,
ordinary membership, and time grants; do not infer normative behavior from
Umbra source or tests. If the clause is unavailable, choose another bounded
HLA concept. Reuse current source/test evidence without compilation or reruns,
keep 2010 and embedded/process boundaries explicit, preserve the five pending
render checks, and leave the Requirements Lab untouched.

### 2025 five-diagram render verification — 2026-10-11

The five diagrams missing from the 242-block baseline now render locally and
have been screenshot-inspected: the object-name reservation lifecycle, two
DDM regional-object flows, and two process-endpoint sequences. The process
sequences were reflowed to shorten labels after the first screenshot showed
crowding; all five then rendered successfully with the installed Mermaid CLI.
All 247 current diagrams have local render evidence, but the complete set has
not received visual review and GitHub rendering remains unverified. See the
[review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-five-diagram-render-verification--2026-10-11).
No C++ source, build, test, CI, or Requirements Lab work occurred.

Next handoff: audit the HLA object-information lifecycle across registration,
discovery, subscriptions, initial values, and the first value update. Compare
the object-information, subscription-state, Auto Provide, DDM, and
process-endpoint guides. Focus the known evidence gaps around ordinary
known-object/new-attribute behavior and ordinary process-profile initial
values; add a cross-guide map only if it closes a reader gap. Use existing
source/test evidence without builds or reruns, keep the 2010/2025 boundary
explicit, and leave the Requirements Lab untouched.

### 2025 object-information lifecycle audit — 2026-10-11

The existing 2025 guides already connect publication and registration,
per-receiver discovery, subscription and DDM eligibility, discovery-time
current values, distinct Auto Provide solicitation, later provider updates,
and reflection. The subscription guide already contrasts a not-yet-known
object with adding an attribute to an already-known object and separately
maps the embedded/process late-subscription evidence. No duplicate overview
diagram was warranted; the ordinary known-object/new-attribute result and
ordinary process-profile initial values remain explicitly unresolved. See the
[review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-object-information-lifecycle-audit--2026-10-11).
The active portfolio and DDM backlog rows now agree that 247/247 diagrams have
local render output, while full visual review and GitHub rendering remain
pending. A targeted screenshot attempt for the subscription guide produced no
images: Mermaid CLI's Chrome launch failed before rendering even with serial
jobs and an ephemeral profile. Keep its passive-recipient and late-subscription
visual checks pending. No source, build, test, CI, 2010, or Requirements Lab
work occurred.

Next handoff: review the exact IEEE 1516.1-2025 §5.8 text from an authorized
copy to distinguish initial values when a subscription makes an unknown
object discoverable from adding attributes after that object is already
known. Keep normative wording separate from the embedded implementation
trace and the unresolved ordinary process route. Do not infer the rule from
source comments; avoid builds and test reruns, preserve the 2010/2025 boundary,
and leave the Requirements Lab untouched.

## 2025 subscription chart renderer setup follow-up — 2026-10-11

The configured Scoop buckets contain no `mmdc`/Mermaid CLI package, so the
user-authorized fallback installed Mermaid CLI 12.0.0 globally through the
already-installed Scoop Node.js LTS. A targeted render of the late-subscription
flow still failed before producing an image: Puppeteer could not launch system
Chrome, and a second attempt using system Edge also exited before page startup.
No browser process was terminated. This is a browser-launch failure, not a
Mermaid parse failure, and it does not invalidate the prior 247 local-render
results. The passive-recipient and late-subscription screenshot checks remain
pending; do not reinstall or repeat the same launches unless browser/runtime
state changes.

Next handoff: continue the bounded HLA-first portfolio review and select a
specific unanswered reader question before changing a guide; the current
object-information flows already connect registration/discovery, receiver
eligibility, initial values, Auto Provide, later updates, and reflection. Keep
the exact §5.8 normative distinction open until its authorized text is
available. Reuse existing evidence, retain 2010/2025 and embedded/process
boundaries, and avoid builds, test reruns, CI, and Requirements Lab changes.

## 2010 reference-profile coverage review — 2026-10-11

A bounded review of the separate [2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md),
its cited `reference_2010.cpp` paths, and the registered binding-shell smoke
found no missing major reader flow within the currently implemented profile.
The guide already separates Connect/Join, object information, interactions,
synchronization, timing values versus unavailable time-managed services, and
callback delivery/failure. Its single binding smoke supports only the selected
two-member, successful `HLA_IMMEDIATE` path; empty-subscription discovery,
callback-failure fan-out, and synchronization-barrier details remain
source-only or untested as labeled. No 2010 diagram was duplicated, no 2025
behavior was imported, and no 2010 test or CI was run.

The follow-up subscription preview also remains unverified: Mermaid CLI 12.0.0
still could not launch Puppeteer's managed Chromium; the app-managed browser
could not reach the loopback preview. No screenshot was produced. Leave local
visual review and GitHub rendering pending, and do not repeat browser attempts
unless a supported preview environment changes.

Next handoff: continue from a specific uncovered HLA reader question rather
than expanding either stream for diagram count. The current 2010 reference
profile is adequately mapped for its existing evidence; expand it only when a
new 2010 implementation path or focused evidence creates a distinct reader
need. Keep unresolved 2025 normative text and visual-review gaps explicit;
preserve the 2010/2025 and embedded/process boundaries.

## 2025 explicit Request Attribute Value Update boundary review — 2026-10-11

The existing [attribute-value request guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md)
already explains the solicitation → provider callback → optional value update
→ independently eligible reflection sequence and distinguishes explicit
requests from discovery-triggered Auto Provide. No duplicate diagram was
warranted. The guide now makes the requester-state nuance explicit: the
object-instance path requires a requester-known object, while the shared 2025
class planner expands registered descendants without consulting per-instance
known-object or subscription state when forming provider groups. The process
service calls that same planner but delivers callbacks through its own event
path. Its focused class test makes no public subscription call, yet first
observes discovery of the target objects; neither class test isolates an
undiscovered target or a value response to an unsubscribed requester. Keep
these as bounded source/test observations, not portable §6.21 conclusions.
The portable sequence now labels its lifelines as roles rather than distinct
federates and adds an accessible title/description. The sequence body parsed
with the installed Mermaid library after excluding that metadata; the full
parse path still needs browser-backed verification because the Node-only API
has no usable DOMPurify context. Mermaid CLI 12.0.0 is already installed under
Scoop Node but is not on this shell's PATH, so no reinstall was needed. No
screenshot or GitHub-render claim is made.

A cross-link audit of all 42 `docs/design/*.md` files found no missing local
paths and no broken anchors among 186 local fragment links; seven stale
anchors were corrected. Screenshot review and GitHub rendering remain
unverified. This documentation pass made no C++ source edits, builds, test
runs, CI changes, 2010 changes, or Requirements Lab changes.

Next handoff: obtain authorized IEEE 1516.1-2025 §6.21 text and resolve whether
the explicit request service gives its caller any direct response entitlement
beyond ordinary receiver eligibility. Update the portable model only from
that clause; keep the shared planner, separate process delivery, test limits,
Auto Provide, and 2010 boundary distinct.

## 2025 callback re-entry: API annotations and selected evidence — 2026-10-11

A bounded review of the pinned 2025 `RTIambassador` declarations, current
adapter guards, and existing test bodies made the distinction between API
exception annotations and the complete HLA call-permission rules more
actionable. The API lists `CallNotAllowedFromWithinCallback` on all four
Connect overloads, Disconnect, both Join overloads, Resign, and both Evoke
services. The current adapter has guards for those service groups. Existing
focused cases exercise both Evoke calls from an immediate callback and Resign
from selected embedded `HLA_EVOKED` save callbacks, including timed-save
advance variants. The bounded test scan found no focused Connect, Disconnect,
or Join re-entry cases; the save tests do not establish a universal Resign rule,
process-profile parity, or total callback permission matrix.

The callback guide now places those API/source/test facts in one compact map
and explicitly warns that a missing `@throws` annotation is not permission to
call a service from every callback. No new diagram was warranted: the existing
recursive-dispatch flow already separates implementation capability from
service permission. The complete normative matrix remains open until the exact
standard service clauses are available. No test was rerun, and no C++ source,
build, CI, 2010 behavior, or Requirements Lab content was changed.

Next handoff: continue the bounded HLA-first review for a distinct reader gap;
do not block other documentation on unavailable normative text. Keep the
callback matrix, explicit-request entitlement, §5.8 subscription, and §4.11
Join/save-restore questions clearly unresolved until their authorized clauses
are checked. Preserve the 2010/2025 and embedded/process boundaries; no builds,
test reruns, CI, or Requirements Lab edits.

### 2025 connection and model-to-runtime orientation audit — 2026-10-12

A bounded reader review found that the existing lifecycle guide already
separates ambassador connection, federate membership, and execution lifetime,
including their distinct API results and local state transitions. The FOM
guide separately explains per-module validation, complete-set composition,
FDD materialization, and the later runtime-service boundary. The names/handles
guide then distinguishes execution-scoped model symbols, inherited
class-member lookup, federate-local known-object state, typed handles, and
normalization coordinates. The cross-cutting guide supplies the compact
model-vocabulary-to-live-state overview.

These views form a useful reading path but are intentionally not one universal
state machine. Merging them would conflate model validity with runtime state,
or model declarations with one federate's object knowledge. No additional
diagram or prose was warranted. This review used the existing HLA/API and
evidence maps only; it adds no normative claim or new 2010/2025 or
embedded/process equivalence. No source, build, test, CI, or Requirements Lab
work occurred.

Next handoff: keep the 2025 new-attribute-for-known-object rule open pending
authorized §5.8 text. If that text is unavailable, select a different concrete
HLA reader question from the existing portfolio rather than repeating the
connection/model survey. Continue to lead with portable HLA concepts, use
Umbra only as bounded evidence, preserve edition/profile boundaries, and keep
the implementation worker's build capacity free.

### 2025 normative-text access check and handoff cleanup — 2026-10-12

The official [IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
currently exposes purchase and subscription access, not the exact §5.8, §6.21,
or §4.11 clause text needed for the open questions. Keep those normative
outcomes unresolved; do not infer them from the public API, Umbra source, or
focused tests.

The order/transportation guide's prior next-topic pointer sent readers to a
name/handle audit that has already been completed. Replaced it with a durable
cross-link path to the time, DDM, and callback guides, plus an explicit note
that the sender-to-receiver order mapping remains open pending authorized
standard text. No new diagram or normative claim was added. No C++ build, test,
CI, 2010 behavior, or Requirements Lab work occurred.

### 2025 cross-cutting HLA reader-map audit — 2026-10-12

Completed the bounded audit of the delivery matrix, object-attribute path,
protocol-completion map, service-result/report map, and service-contract map.
Their existing distinctions are useful and already HLA-first: delivery axes
are not a fixed pipeline; a value request solicits rather than supplies a
provider's response; callback entry is separate from protocol completion; and
the HLA service contract, not set-valued argument shape, determines specified
outcomes. No new chart was justified. One generic-map ambiguity was corrected:
the service-result diagram now marks an HLA-required state/effect as optional
when the contract specifies one and separates it from an RTI-private commit
point. Its updated Mermaid text has not been rerendered or visually reviewed.

A bounded cross-guide deletion check found deletion authority, federation-wide
removal, per-federate known-object cleanup, and callback timing already covered
in the ownership and object/interaction guides; no duplicate deletion guide
was warranted. The concrete next handoff is a bounded reader audit of 2025
interaction delivery across the object/interaction, DDM, DataElement, time,
and callback guides, focused on keeping parameter meaning, receiver
eligibility, and callback observation separate from object-instance state.
No C++ source, build, test, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 ordinary interaction receipt evidence update — 2026-10-12

The bounded interaction-delivery survey found no need for another end-to-end
diagram: existing guides already separate the FOM class/parameter and encoded
value path, ordinary recipient relevance, regional scope, TSO eligibility, and
callback observation. The worktree contains a newly verified embedded public
API receipt case (27 assertions; five direct requirement-section pairs per
the roadmap). It sends a descendant interaction to one receiver while the
receiver subscribes successively at base, base-plus-middle, and exact-child
levels; each time there is one callback, and its class equals the selected
candidate. The case uses an empty parameter map and no regions. It therefore
does not test inherited parameter projection, regional matching, passive
relevance, DSE, process behavior, or the normative inheritance rule.

Updated the object/interaction guide's diagram and evidence map to show the
selected callback class and link that focused case, while keeping projection
evidence separate. Updated the DDM guide to link the ordinary case but state
that it does not establish the regional overlap or available-dimension paths.
This is embedded 2025 development-profile evidence, not conformance or 2010
evidence. The updated Mermaid diagrams have not been rerendered or visually
reviewed. No C++ source, build, test, CI, or Requirements Lab work occurred in
that documentation increment.

### 2025 directed-interaction receipt: two class axes — 2026-10-12

Completed the bounded source-and-test audit of the directed receive-order
path. The guide now distinguishes the target's registered object class, used
for FOM directed-pair validity and producer publication, from each receiver's
known object class, used to find an applicable directed subscription. It also
shows the interaction search from sent class toward its superclasses and the
subscription's paired object-class lookup as separate axes.

The current retained `LastTest.log` records the embedded
`directed_interaction_inherited_receipt` case passing with 35 assertions. Its
fixture registers a child target, discovers it at a base object class, gives
the receiver both a base-object/base-interaction subscription and a deeper
child-object/child-interaction subscription, and sends the child interaction.
It observes one callback with the base interaction class. This is bounded
embedded 2025 evidence with empty parameters; it does not establish the
normative inheritance rule, object-ancestor fallback, parameter projection,
TSO behavior, process-endpoint parity, or anything about 2010. The retained
passing log supersedes an earlier failing local result observed during review.

Updated Mermaid has not been rendered or screenshot-reviewed. The passing
CTest result is the implementation worker's retained worktree result; I did
not build or rerun it. No CI, 2010, or Requirements Lab work was performed for
this documentation increment. Next: audit the existing process-endpoint
directed path and test against the new class-axis explanation, without
building or rerunning tests;
keep the process callback boundary and evidence gaps explicit. The linked test
source and FOM fixture are currently untracked in the shared worktree; their
implementation owner still needs to include them in version control before
this evidence link is durable on GitHub. This documentation pass did not stage
or alter those C++ files.

### 2025 process directed receipt: push/pull callback boundary — 2026-10-12

Completed the bounded comparison of the process receive-order route with the
embedded callback path. The process send handler uses the same registry
recipient planner, materializes a recipient-specific event, and either pushes
an event frame or places the event in a recipient queue for `Receive
Interaction`. The callback bridge forwards the projected event payload; the
process path does not repeat the embedded callback-time registry eligibility
check. Updated the guide's process flow to show the push/pull fork explicitly
and corrected prose that had described pull as the only process path.

The existing public process test uses an exact-class fixture with two
receivers and excludes the sender. It configures pushed receive-order event
delivery for `HLA_IMMEDIATE` and uses pull receive for `HLA_EVOKED`; assertions
check each receiver's class, target, tag, producer, transport, and empty
parameter set. It does not test inherited declarations, post-acceptance
mutation, or callback timing relative to service return. This is 2025 process
source/test evidence only, not normative text, not proof of stale-work
behavior, and not an equivalence claim with the embedded callback boundary.

The process Mermaid diagram has not been rendered or screenshot-reviewed. I
did not build or rerun C++; no CI, 2010, or Requirements Lab work occurred.
The focused inherited-receipt source and FOM fixture remain untracked in the
shared worktree and still need to be included by their implementation owner.
Next: audit timestamped directed delivery across per-recipient admission, TSO
queue/grant/retraction, and callback boundaries, reusing the existing time and
retraction guides without duplicating them or claiming profile parity.

### 2025 timestamped directed-interaction recipient split — 2026-10-11

Completed the bounded source/test audit of timestamped directed delivery. The
embedded send path plans target-specific recipients, retains all planned
recipients in the retraction record, and queues only time-constrained
recipients for TSO. Non-constrained recipients instead take the receive-order
callback path, with sent order `TIMESTAMP` and received order `RECEIVE`; a
constrained recipient is rechecked at its TSO callback boundary. The guide's
flowchart now names the pending-recipient retraction outcome without implying
that every undelivered copy must be a TSO queue entry.

The mixed-fanout test source observes Request Retraction for the already
delivered recipient and suppression of the queued constrained recipient
without either its original callback or a Request Retraction. A separate
embedded case observes accepted directed TSO work survive producer resignation
for FQR, TARA, and NMRA recipients. The process endpoint remains a separate
boundary: its TSO dispatcher rechecks and sends an event frame, while the
client callback gate and completion acknowledgement govern the in-transit
path. Its focused `HLA_EVOKED` test observes directed callback before grant
after callbacks are enabled. These are selected source/test observations, not
complete normative rules or embedded/process equivalence. I reused recorded
test sources and prior results only; no C++ build or test rerun occurred.

The updated Mermaid chart was not rendered. Mermaid CLI is already installed,
but the review ledger records browser-launch failures and explicitly defers
retries until runtime conditions change; I did not reinstall or retry it. No
2010, Requirements Lab, or implementation-source changes were made.

Next: perform a bounded HLA-first reader-gap survey and select one concrete
question not already answered by the portfolio; prefer tightening an existing
guide to adding another diagram when that is clearer. Keep exact-standard
uncertainties open, use existing evidence without compiling, and preserve the
edition and profile boundaries.

### 2025 HLA-first portfolio survey and inventory reconciliation — 2026-10-11

A bounded scan of the current guide map and 2025 RTI service families found
existing coverage across Federation, Declaration, Object, Ownership, Time,
DDM, RTI Support, and callback controls; another umbrella guide would repeat
the topic guides. I statically reviewed the cross-cutting service-outcome/report
map and traced its three case-study rows to current source/test locations. The
map correctly makes required HLA effects conditional on the operation
contract, treats profile commit points as private implementation facts, and
keeps selected report routing separate from the caller-visible result. The
report-writer failure case has assertions in its body but its last recorded
run failed before reaching them, so that branch remains source-derived. No
universal transaction, rollback, 2010, or embedded/process equivalence is
claimed.

The refreshed inventory is 248 Mermaid diagrams across 34 diagram-bearing
files: 145 flowcharts, 80 sequences, and 23 state diagrams. The separate 2010
stream has 14; the 2025 stream has 232; the index has 2. The previous
247-diagram local-render and syntax evidence does not cover the current
addition. The current cross-cutting guide contains 13 diagrams, correcting
its former 12-diagram backlog count. Full visual and GitHub review remain
pending. No C++ source was changed and no build, test, CI, 2010, or Requirements
Lab work occurred.

Next: source-check the six-example “When work becomes stale” synthesis against
the linked DSE, subscription, deletion, ordinary/directed interaction,
resignation, and retraction guides. Verify that withdraw, invalidate,
preserve, and account remain operation- and recipient-specific—not a shared
cancellation or rollback rule. Correct only a concrete mismatch; wait for a
permitted preview/runtime change before retrying visual review of the current
diagram set.

### Six-example stale-work evidence audit — 2026-10-11

Source-checked the cross-cutting “When work becomes stale” examples against
the linked subscription/DSE, deletion, ordinary/directed interaction,
ownership, retraction, and lifecycle guides. The distinctions hold:
active/passive changes affect later per-attribute projections. DSE defers
eligibility only for certain service-specific candidates. Accepted deletion
invalidates selected object work while each receiver's removal has its own
commit gate. Ordinary and directed publication withdrawal use different
callback fences; resignation cancellation is member-scoped; TSO retract
separates pending delivery from an already-entered callback; and lifecycle
cleanup distinguishes the departing member's route from survivor routes. No
shared HLA cancellation/rollback rule is implied. The cross-cutting table now
distinguishes active/passive projection from DSE candidates, and its lifecycle
row limits callback-queue reset to the selected embedded voluntary-resign
path while linking forced connection loss separately. These remain bounded
2025 source/test observations, not normative or cross-profile equivalence
claims. No C++ build, test, CI, 2010, or Requirements Lab work occurred.

### Already-known object subscription evidence audit — 2026-10-11

Rechecked the open 2025 §5.8 question against current source and focused test
bodies. The embedded callback path plans ordinary initial values only after
`Discover Object Instance`; its focused late-discovery test observes the
selected stored value, but no reviewed test isolates adding a new attribute to
an object the receiver already knows. The process subscription path replans
discovery, while its separate initial-value event is gated to RTI-owned MOM
objects. The public process discovery test subscribes before registration and
asserts discovery; the private known-object/second-attribute test asserts the
owner's relevance advisory, not a subscriber reflection. Thus the inspected
source/tests do not establish an ordinary current-value snapshot for an
already-known object, and they do not resolve the normative result or profile
parity. The public [IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers purchase and subscription access, not the licensed §5.8 clause text;
leave that interpretation open. The existing subscription guide already
states this gap, so no duplicate chart or speculative rule was added. No C++
source, tests, build, CI, 2010 stream, or Requirements Lab content changed.

### Attribute Value Update Request guide evidence audit — 2026-10-11

Checked the guide's HLA-facing sequence and scope diagrams against the current
registry, ambassador, process-service, and focused-test evidence. They keep the
request tag/provider callback separate from the provider's optional later
value-bearing update and each receiver's ordinary eligibility decision; the
roles may coalesce, and the request does not guarantee callback-before-return.
The implementation diagrams explicitly separate object-known validation,
class expansion/provider partitioning, regional request filtering, and the
embedded callback-entry recheck. The process service uses its own event seam;
its class-request test covers registered subclass instances, not complete
profile parity. The RTI-owned MOM direct-reflection path is separately
implemented and exercised for a selected report-file value, not generalized
to ordinary provider solicitation. Existing focused evidence does not settle
direct response entitlement for an unsubscribed requester; the public IEEE
listing exposes purchase/subscription access, not §6.21 text. The guide already
states these boundaries, so no duplicate chart or normative guess was added.
No C++ build, tests, CI, 2010, or Requirements Lab work occurred.

Next: audit the 2025 [DDM and regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md), focusing on the HLA-first region-template lifecycle: distinguish dimension/region declaration, committed extents, per-object update associations, subscription regions, and deletion/invalidated use. Check its diagrams and current focused evidence for a genuine reader gap; do not add another diagram if the existing lifecycle/overlap flows already teach the distinction. Preserve its 2025-only scope, keep the 2010 stream separate, and do not build or rerun tests. Current-set visual review remains pending until an approved preview runtime is available.

### 2025 Federation MOM projection versus execution lifecycle — 2026-10-12

The Federation MOM guide's `NotEstablished` state could be read as meaning
that no federation execution exists. Added prose clarifying that the chart
tracks only the 2025 federation-scoped MOM projection: Create may already
leave an execution present with zero members. Linked the separate lifecycle
guide's absent/empty/joined execution chart and explicit Destroy path. The
embedded source establishes the projection during Join before per-member
setup finishes and retains it if that later setup rolls membership back; this
ordering is source-observed, with no focused failed-first-Join test or process
parity claimed. The chart did not change, so no render or diagram-count update
was needed. No 2010, build, test rerun, CI, or Requirements Lab work occurred.

Next: audit the 2025 service invocation reporting guide for the HLA-first
boundary between synchronous service outcome, invocation-report record, and
later requested MOM report interaction. Reuse existing receive-order evidence;
keep embedded/process and 2010 boundaries explicit, avoid duplicate diagrams,
and do not build or rerun tests.

### 2025 invocation reports versus requested Federation MOM reports — 2026-10-12

Compared the service-invocation and Federation MOM guides with the 2025 MIM,
cross-cutting result map, and bounded focused-test sources. Added an explicit
three-trace distinction: the public service's return/exception, the
subject-switch-controlled `HLAreportServiceInvocation` record/interaction,
and an explicit request that causes a separate `HLAreport...` interaction
about Federation MOM state. Added a reciprocal link to the Federation MOM
guide. Kept the selected embedded file-append failure caveat visible, and did
not imply a synchronous RPC reply or a universal report-before-return rule.
The focused `HLA_EVOKED` sources show invocation-report-before-directed
callback and requested MOM report observation after callback draining; these
are individual examples, not broad profile evidence. The file-append failure
effect is source-observed; the cited test run failed during federation
creation before its append assertions. No diagram changed, so no Mermaid
render was required. No build, tests, CI, 2010, or Requirements Lab work
occurred.

Next: audit the 2025 exception-reporting guide at the boundary between public
service exceptions, `HLAreportException`, `HLAreportMOMexception`, and failed
`HLAreportServiceInvocation` reporting. Use bounded 2025 evidence only; keep
2010 separate and do not build or rerun tests.

### 2025 exception-report scope and report-family boundary — 2026-10-12

Reviewed the exception-reporting guide's API boundary, three-report
comparison, decision map, two failure routes, process combined-report trace,
MIM declarations, and selected source/test references. The caller's typed
exception, advisory `HLAreportException`, rejected-MOM
`HLAreportMOMexception`, and `HLAreportServiceInvocation` are already kept
separate; callback timing and the source-only process two-report attempt order
are carefully qualified. Added a scope note and cross-link distinguishing
successful request-driven Federation MOM reports such as
`HLArequestFOMmoduleData` → `HLAreportFOMmoduleData` from the three routes in
the exception comparison. This closes the risk that the table reads as an
exhaustive MIM report inventory. No diagram changed; the existing decision
map and sequences remain the right home for the failure flows. Test sources
were inspected, not run. No process/embedded or 2010 parity is inferred; no
build, tests, CI, or Requirements Lab work occurred.

Next: audit the 2025 Object and Interaction Information Flow and Object
Attribute Subscription State guides together for a genuine reader gap in the
path from publication/send through receiver eligibility, known-object state,
and callback observation. Prefer linking over duplicating existing flows;
keep 2010 separate and do not build or rerun tests.

### 2025 object information and subscription-state wayfinding — 2026-10-12

Compared the broad Object and Interaction Information Flow guide's five
delivery questions with the focused Object Attribute Subscription State
guide. Their concepts and evidence boundaries are consistent: publication,
ownership, receiver qualification, object knowledge, and application
observation remain separate, while the focused guide details ordinary and
regional declaration mutation plus the discovery/value gates. The broad guide
already deep-links to the focused one at object-discovery and attribute-value
request boundaries; the focused guide lacked a return path to the end-to-end
map. Added a short scope-level cross-link instead of copying its routing
diagrams. The pinned 2025 subscription signature and inspected test sources
support the limited examples; they do not settle active-establisher/passive-
receiver coverage, ordinary process late-value behavior, or licensed §5.8
wording. No diagram changed or render was needed. No test/build/CI rerun, 2010
change, or Requirements Lab work.

### 2025 explicit request versus Auto Provide boundary — 2026-10-12

Compared the 2025 Attribute Value Update Request and Auto Provide guides.
Their existing flows and reciprocal links already distinguish a federate-
invoked request from discovery-triggered RTI solicitation, and both keep the
provider's later value-bearing update separate. Explicit requests preserve
their caller-supplied request tag and use their own pending-request checks;
Auto Provide's current embedded path uses an empty callback tag and a separate
switch/scope/ownership recheck. Initial reflection is also separate from either
solicitation. The pinned API declarations and inspected embedded baseline test
bodies support those bounded distinctions: the explicit test observes its
nonempty tag at the provider callback, while the Auto Provide test observes a
zero-length tag after discovery. These are not process-parity or universal
provider-response claims. No reader gap justified another diagram or guide;
existing diagrams already show the later provider update. No build, test rerun,
CI, 2010, or Requirements Lab work occurred; Mermaid CLI was not needed.

### 2025 object-name reservation versus local handle knowledge — 2026-10-12

Audited the 2025 Name and Handle Resolution and Object Instance Name
Reservation guides together. They already distinguish symbolic reservation
from successful object registration, the owner's returned object handle, and a
peer's later discovery-driven known-object state. Their links are reciprocal,
and the reservation guide includes a two-view synthesis: the owner path and an
eligible peer path are separate and are supported by two focused embedded
tests, not one combined end-to-end assertion. The pinned lookup API scopes
forward/reverse object-instance resolution to objects known by that federate;
the test sources show lookup failing while discovery is queued and succeeding
after the callback is evoked. The process lookup test cited by the guide covers
the owner's round trip, not peer discovery parity. Existing diagrams and
wayfinding already explain this boundary; no duplicate chart or text change
was warranted. No rendering, build, test rerun, CI, 2010, or Requirements Lab
work occurred.

### 2025 model extension versus name-to-handle resolution — 2026-10-12

Audited the 2025 FOM Module Admission and Composition and Name and Handle
Resolution guides together. Their diagrams cover complementary layers: the
FOM flow prepares and atomically commits a replacement model with the new
membership, then marks the observed post-commit Join failure edge as
source-derived and untested; the lookup sequence shows an already-joined
member resolving added declarations after a successful additional-module Join.
Both guides cross-link these views. The embedded object-class and
interaction-class tests observe stable typed-handle equality within that
execution while newly contributed class handles become resolvable. The
configured-process test covers lookup by the joining client only, not a prior
member's view. These are implementation observations, not cross-execution
numeric-handle guarantees or profile parity. Existing flows and reciprocal
links already explain the boundary; no additional diagram or prose change was
warranted. No rendering, build, test rerun, CI, 2010, or Requirements Lab work.

### 2025 model vocabulary versus DataElement value encoding — 2026-10-12

Compared the FOM composition and DataElement Encoding guides at the boundary
between model-declared type vocabulary and application-side value octets. The
DataElement guide already routes FOM/OMT input to model/schema composition and
links back to the FOM guide. The FOM guide explains model vocabulary and later
runtime transitions but did not identify the separate public `DataElement`
encoding layer. Added a concise reciprocal pointer stating that model
admission does not execute a federate's value encoder and linking to the
DataElement guide's representation-boundary flow. This does not assert a
one-to-one mapping from every OMT data type to a C++ codec. The pinned 2025
`DataElement` API exposes its own encode/decode operations; no implementation
or test behavior was changed. No diagram or inventory count changed, so no
render was needed. No build, test rerun, CI, or 2010 change occurred.

### 2010/2025 DataElement implementation-boundary audit — 2026-10-12

Compared the separate 2010 and 2025 DataElement guides with their pinned API
headers, edition-specific scalar/composite sources, shared helper headers, and
selected existing test sources. The guides correctly keep BE/LE selection at
the concrete element wrapper, keep encoded-layout examples tied to separate
test targets, and identify the public signature differences (`auto_ptr`/
`void decode` versus `unique_ptr`/`DataElement& decode`). Neither shared
byte-order primitive selects an order by itself, nor do those API similarities
establish a shared format contract.

The source audit exposed a second limited reuse point: both edition-specific
composite adapters import the same type-free checked-size, range, and padding
functions from `composite_primitives.hpp`. Added this fact to both edition
guides with links to the separate adapters and common helper. The codec
translation units and tests remain edition-specific; no composite parity is
inferred. Existing diagrams already show the two streams separately, so none
changed and no rendering was needed. Test sources were inspected, not run; no
build, CI, or Requirements Lab work occurred.

### Callback model versus process endpoint event delivery — 2026-10-12

Audited the 2025 Callback and Service Ordering guide against the Process
Endpoint Request and Callback guide and bounded implementation ranges. Both
guides already distinguish the HLA_IMMEDIATE/HLA_EVOKED callback policy from
the process service's queued/pushed event acquisition modes, and the callback
guide already explains the recipient-side request/pump exception boundary.
Added a scope-level link from the callback guide to the concrete process event
trace, explicitly labeling that guide as Umbra implementation mechanics rather
than authority for HLA callback semantics. No diagram was needed. Test evidence
was not rerun; no build, CI, 2010, or Requirements Lab work occurred.

### 2025 exception-report callback failure boundary — 2026-10-12

Compared the Exception Reporting and Callback and Service Ordering guides with
bounded hook, recipient-submission, dispatcher, process request, and focused
report-test evidence. `emitExceptionReport` is `noexcept` and suppresses a
callback exception only if it unwinds through that hook's own synchronous
report attempt. In the embedded recipient loop, an inline throw stops later
route submissions; the receive-order/time-state gate may defer callback entry
independently. Deferred/evoked or separate process-recipient stacks follow their
own callback exception path. Added a focused sequence diagram and a reciprocal
link. The process report test uses a non-throwing receiver, so the
throw boundary remains source-derived and untested. A render check is pending:
the configured Scoop buckets have no `mmdc` manifest. No build, test rerun, CI,
2010, or Requirements Lab work occurred.

### 2025 DDM empty-region-set and object/interaction boundary audit — 2026-10-12

Compared the DDM/Regions guide with Object and Interaction Information Flow and
bounded 2025 source/test evidence. Existing maps already separate object
attribute association/discovery/value routing from ordinary interaction
subscription/send-region routing, then keep overlap, callback observation, and
logical-time eligibility as distinct questions. The review found one concrete
reader gap: “empty” could mean a zero-dimensional Region object, an empty
RegionHandleSet supplied to Send Interaction With Regions, or an empty
designator set conveyed on an ordinary interaction callback. Updated the DDM
guide's interaction routing flow and added a small comparison table: the
explicitly empty service-input set is a no-send operation, does not fall back
to an ordinary default-region subscription, and is not the zero-dimensional
Region case. The cited 2025 planner and focused case support the implementation
example; no cross-edition inference is made. Added reciprocal scope-level
wayfinding to the end-to-end Object/Interaction guide. No duplicate diagram or
diagram-count change. The edited Mermaid chart is not rendered: Scoop has no
`mmdc` or `mermaid-cli` manifest, and the permitted install/search attempts found
no package. No build, tests, CI, or Requirements Lab work occurred.

### 2025 save/restore responsibility and temporal-boundary audit — 2026-10-12

Compared the Save/Restore, Federation and Federate Lifecycle, and Time
Management guides with bounded 2025 API/test evidence. Their existing diagrams
already distinguish federate application checkpoints from the RTI's federation
image and terminal result; Join membership from a protocol's own participant
set; accepted save requests from delayed initiation; and time eligibility from
callback observation. The lifecycle guide correctly leaves the exact Join
precondition/participant effect during save or restore unresolved rather than
transferring the expandable synchronization-point rule.

One navigation gap remained: Time Management labels deferred receive-order
callback closures as live-session state and says their general save/restore
behavior is unproven, but did not point readers to Save/Restore's selected
in-flight-work dispositions. Added that cross-link, explicitly warning that an
already-pushed process `HLA_EVOKED` frame surviving service-level save/restore
does not prove these deferred closures are in the durable image or survive a
restart. Tightened Save/Restore's reciprocal links to the exact time/callback
map and timestamped-message lifecycle. No diagram changed or was added; no
render was needed. The focused process-test source was inspected, not run. No
build, CI, 2010, or Requirements Lab work occurred; no process/embedded parity
or general callback-persistence claim is made.

### 2025 synchronization participant-set boundary audit — 2026-10-12

Compared the 2025 Synchronization, Federation and Federate Lifecycle, and
Save/Restore guides with the pinned Join declarations, bounded registry paths,
and focused late-join test. The descriptions agree: Join changes federation
membership; the embedded synchronization path separately snapshots current
members and permits expansion only when the synchronization-set overload was
omitted; Save/Restore Join admission and its participant effects remain an
unresolved contract question. The direct test covers omitted versus explicit
non-empty sets, not explicit-empty registration or partial-achievement-before-
Join; those paths remain correctly labeled source-derived. The guides do not
transfer the synchronization-specific expansion rule to Save/Restore or claim
2010/process parity.

The only gap was wayfinding from the synchronization guide's membership
orientation to the two exact cross-protocol sections. Added direct links to the
lifecycle participant-set map and Save/Restore Join boundary, plus a sentence
that the barrier-specific expansion is not a general membership or
Save/Restore rule. Existing diagrams already explain the distinctions; no chart
changed or was added, so no render was needed. Source/test evidence was
inspected, not run. No build, CI, 2010, or Requirements Lab work occurred.

Completed handoff: audit the 2025 synchronization registration/achievement callback boundary
alongside Callback and Service Ordering. Distinguish a synchronous service
result from submitted registration/announcement/report callbacks and the final
`federationSynchronized` outcome; compare source submission order with
HLA_IMMEDIATE/HLA_EVOKED application observation and process endpoint queueing.
Reuse bounded API/source/test evidence and the existing dispatch view; add a
chart only for a demonstrated reader gap. Keep 2010 separate and do not build or
rerun tests.

### 2025 synchronization callback, process-event, and service-report boundary audit — 2026-10-12

The HLA-level map and embedded callback zoom already separated registration,
announcement, achievement, and federation completion. A process-profile
reader gap remained: push event frames and correlated service responses travel
through different paths, and the process adapter orders selected service
reports differently from the embedded adapter.

Added a process-specific sequence and reciprocal cross-guide links. In pushed
mode, the process client buffers event frames until the matching response,
then releases its stream lock and submits the events before returning to the
public adapter. Thus a registrant who is also an announcement recipient can
have `announceSynchronizationPoint` submitted before the registration-result
callback. A final pushed `federationSynchronized` can likewise be submitted
before the adapter starts the achievement's invocation-report request. The
selected report sink may be a file or an `HLAreportServiceInvocation`
interaction, handled by the separate reporting guide. Polling-mode
notifications remain queued until a later `receive_interaction`.
The push/poll setting is independent from `HLA_IMMEDIATE`/`HLA_EVOKED`; the
trace labels all source ordering as Umbra-specific, not an HLA guarantee.

The paired `synchronization_point_catch2.cpp` cases are embedded-profile tests.
Separate public process-endpoint cases cover HLA_EVOKED with polling and
HLA_IMMEDIATE with pushed events, asserting registration success/failure,
announcement to the explicit-set receiver, and a false final result. Because
the registering sender is excluded from the set, the cases do not assert
same-recipient callback order or service-report order; these finer sequence
claims remain source-derived, not an absence of process callback coverage or
process/embedded parity. The new Mermaid sequence is not rendered because
`scoop install mmdc` found no manifest. Source/tests were inspected, not run.
No build, CI, 2010, or Requirements Lab work occurred.

2026-10-12: Completed the process polling-versus-push audit. The 2025 process
service keeps event-family queues per session; `receive_interaction` checks
synchronization announcement, federation synchronized, save, restore, and
interaction events in that order before reaching discovery/removal and later
families. A later-queued synchronization announcement can therefore be chosen
ahead of an older queued interaction event. Push mode bypasses this selector;
the process client buffers received event frames in one heterogeneous FIFO and
drains after the correlated response unlocks the stream exchange. Both paths
submit through the callback bridge, while `HLA_IMMEDIATE`/`HLA_EVOKED` is an
independent dispatch setting. This is an Umbra 2025 process-profile observation,
not an HLA-wide ordering guarantee. Paired public process synchronization cases
cover polling and push separately but do not assert mixed-family precedence, so
that distinction is source-derived. Added a bounded comparison flowchart to the
process endpoint guide. Mermaid rendering remains pending: the authorized
`scoop install mmdc` returned “Couldn't find manifest for 'mmdc'”; no package
was installed and no remote renderer was used. No tests/build/CI, 2010, or
Requirements Lab work occurred.

2026-10-12: Audited the callback-ordering guide and process-endpoint guide for
cross-family total-order implications. The HLA-first model already assigns
recipient eligibility to the service, dispatch timing to the callback model,
and local ordering to the admitted dispatcher/stream queue; synchronization
and process-specific ordering claims are separately bounded. Reworded the
callback-order concept to require an exact service and message/time-order rule,
and to say explicitly that callback model is dispatch policy, not sequence.
The pinned Connect signature and separate Evoke/enable/disable APIs support
that distinction; focused dispatcher tests cover local FIFO, not cross-service
ordering. No duplicate diagram was needed. No tests/build/CI, 2010, or
Requirements Lab work occurred.

2026-10-12: Compared the existing value-map diagram's tall layout with a left-to-right alternative and adopted the more compact layout. Mermaid Live Editor 12.1.0 accepted and rendered the exact guide source; the distinct object-attribute update/reflection and ordinary-interaction send/receive paths remain clear. This was not a GitHub render, so GitHub presentation remains unverified. No mmdc install, C++ build, test run, CI, 2010, or Requirements Lab work occurred.

2026-10-12: Audited discovery versus first reflection against the existing object/interaction and subscription-state guides. Their diagrams already distinguish receiver-known state, embedded late-subscription values, the process selector's discovery priority, and the process MOM-only initial-value branch. The focused ordinary process update test observes discovery and a later explicit reflection but does not assert order or an ordinary late-subscription snapshot; another diagram would duplicate the current source/profile comparison. The pinned API headers do not provide normative clause prose, and IEEE lists purchase/subscription access for IEEE 1516.1-2025; §§6.9/6.11 ordering and the ordinary §5.8 process result remain unverified. No C++ build, test run, CI, 2010, or Requirements Lab work occurred.

2026-10-12: Audited the 2025 object-class declaration and ancestry question
against the pinned API signatures, embedded registry source, and focused
child-instance/base-subscription integration case. Added a bounded guide note
separating inherited attribute availability, the class explicitly named by a
declaration, and the receiver's selected known class. The focused test shows a
child-class publication containing base and child attributes, followed by a
base-class discovery and base-only reflection for a base subscriber. It does
not prove the licensed standard's complete ancestry rules or passive,
regional, and process combinations. Existing ancestry and active/passive
diagrams already cover the selected paths, so no duplicate chart was added.
No rendering, C++ build, test run, CI, 2010, or Requirements Lab work occurred.

Next: review the existing 2025 TSO message-retraction guide as an HLA-first
teaching tool. Separate portable per-message service rules from per-recipient
event admission, callback-time retraction, and selected embedded/process
boundaries; reuse existing evidence and add no duplicate diagram unless a
concrete reader gap remains. Keep 2010 separate and do not build, rerun tests,
or edit the Requirements Lab.

2026-10-12: Reviewed the 2025 TSO retraction guide's HLA-first structure and
evidence boundaries. The portable per-recipient delivered/not-yet-delivered
outcome was already distinct from producer legality and recipient time-request
results; embedded and process queues, callback admission, and deletion-specific
effects were separately identified. Found one wording seam where the embedded
callback-entry threshold was labeled as an HLA outcome. Rephrased the guide to
lead with the portable outcome and identify callback entry as the embedded
ledger threshold. The existing diagrams already teach the relevant distinction,
so no diagram was added or rendered. Exact licensed §§8.22–8.23 prose remains
unavailable for normative verification. No test rerun, build, CI, 2010, or
Requirements Lab work occurred.

2026-10-12: Reviewed the 2025 DDM region-template lifecycle against the current
guide, bounded region-registry source, and focused test bodies. The existing
FOM/runtime bridge separates class-available dimensions from a region's fixed
dimension set; the lifecycle chart and prose separate pending range edits from
the committed routing snapshot. Object attribute source associations and
receiver subscriptions are distinct from each other and from the interaction
regional path. The existing deletion gate covers live associations and
subscriptions, stale-handle queries, and deferred object-removal cleanup. The
focused test source confirms an object association remains live until the peer
consumes its removal callback. No reader gap justified another diagram or
prose edit. Source/test code was inspected, not executed; no render, build,
tests, CI, 2010, or Requirements Lab work. Existing full-set/GitHub visual
review remains pending.

Next: audit the 2025 Order and Transportation Controls guide as an HLA-first
teaching tool. Distinguish message ordering from transportation type; trace
class defaults, per-instance overrides, callback-gated transportation
changes, and receiver-visible effects. Reuse existing diagrams and bounded
evidence; add no chart without a concrete reader gap. Keep 2010 separate and
do not build, rerun tests, start CI, or edit the Requirements Lab.

2026-10-12: Audited the 2025 order/transportation guide against the pinned API
headers and MIM, bounded embedded registry logic, and focused test bodies.
Existing flows already distinguish a timestamped service overload from the
effective sent/received order, keep receive-order and timestamp-order delivery
separate from transportation metadata, and place callback observation after
eligibility/time gates. Class defaults are prospective; attribute order and
transportation state are instance-scoped; transportation overrides take
effect at their confirmation callback. Query calls report effective values
through callbacks. The process evidence is a selected interaction
change/query case under both callback models, not process attribute parity.
Exact normative order mapping and the full service cross-product remain open
without licensed clause text and broader tests. Existing diagrams were
sufficient, so no chart or guide text changed. Source/tests inspected only;
no render, build, test run, CI, 2010, or Requirements Lab work.

Next: review the 2025 Declaration and Attribute Relevance Advisories guide as
an HLA-first model. Keep publisher class advisories distinct from owner
per-object relevance and receiver-local scope; trace subscription, hierarchy,
switch, and update-rate boundaries. Reuse existing flows unless a concrete
reader gap remains. Keep 2010 separate; do not build, rerun tests, start CI,
or edit the Requirements Lab.

2026-10-12: HLA-first review of the 2025 declaration/relevance guide found its
existing flows already separate publisher class edges, owner per-instance
relevance, receiver-local scope, discovery, and later reflection. A specific
switch-evidence boundary was unclear: declaration-level no-backlog behavior
must not be transferred to the per-instance owner switch. Added bounded
embedded evidence: the owner switch suppresses a rate-bearing On and an Off
during subscription transitions in the selected callback-model cases; after
reenabling, the test creates a fresh subscription and does not decide replay
of an already-relevant state. Added a direct test link and updated the backlog.
No diagram changed. Source and tests were inspected, not executed; no render,
build, tests, CI, 2010, or Requirements Lab work.

2026-10-12: Completed the 2025 Attribute Scope audit against subscription and
region transitions, the final-self-unsubscribe exception, stale callback
rechecks, and separate embedded/process evidence. Clarified that the queued
stale In is suppressed only in the embedded HLA_EVOKED case; HLA_IMMEDIATE
enters In before the later mutation. Process tests exercise both callback
models but not a stale-queue race. The embedded switch-off segment overlaps
the self-unsubscribe exception, so it does not isolate switch-only suppression;
the process case keeps the switch enabled. No diagram changed. Source/test
evidence was inspected, not run; no build, CI, 2010, or Requirements Lab work.

2026-10-12: HLA-first audit of Auto Provide confirmed the existing diagrams
already separate discovery-triggered solicitation, explicit requests, initial
reflection, and the provider's later value-bearing update. Added one
source-derived embedded boundary: a later scope-in for an already-known object
does not re-enter the discovery-only planner. The focused tests do not isolate
this no-retry edge, so the guide does not claim it as a normative HLA rule or
process behavior. No diagram was changed or rendered. No build, tests, CI, 2010,
or Requirements Lab work.

2026-10-12: Audited timestamped deletion across the 2025 object-information,
time-management, and TSO-retraction guides. Added one receiver-oriented
sequence to connect accepted producer request, receive-order versus queued-TSO
recipient paths, callback-entry state commit, and deletion-specific retraction.
The selected embedded FQR/TARA/NMRA case demonstrates three callback orders at
the same timestamp; these are scoped test observations, not a universal rule.
Embedded tests cover pre-delivery cancellation and mixed-recipient
post-delivery restoration. Source indicates pre-delivery retraction preserves
intervening mutations, but the cited test does not stage one. The process test
does not cover retraction/restoration, so no profile parity is inferred. No
2010 behavior is inferred. Also documented the source-only empty-recipient
case: deletion is accepted without a removal callback when no other federate
knows the object, while the name and retraction record are retained to the time
bound; no focused test isolates this. The Mermaid chart was manually reviewed but not
parsed/rendered because the authorized Scoop install failed with permission
errors and no mmdc manifest. Source and existing test bodies were inspected,
not run; no build, CI, or Requirements Lab work.

2026-10-12: Bounded HLA-first review of the existing 2025 save/restore guide
found its responsibility map and separate detailed save/restore machines
already cover application versus RTI checkpoints, request admission,
participant reports, RTI commit/rehydration, federation-level outcomes,
member-set limits, object state, and callback/durable-image boundaries. No
additional combined diagram was warranted; it would duplicate existing flows
and risk merging distinct HLA responsibilities or profile-specific behavior.
Existing source/test links were reviewed but no tests were run. No guide edit,
render, build, CI, 2010, or Requirements Lab work.

2026-10-12: Surveyed the ordinary and directed 2025 interaction guides,
implementation maps, and focused-test references. The existing guides already
explain ordinary class-scoped sends without object-instance knowledge versus
directed sends with a target object and separate receiver selectors. The
ordinary flow separately covers regional overlap and TSO time, while the
directed guide keeps its target/selector distinct from DDM regions and does not
assert a region-overlap rule. Both keep parameter projection and callback
boundaries distinct, including embedded/process event-commit differences.
Focused evidence is narrow and explicitly labeled; no
universal routing, process parity, or 2010 equivalence is claimed. No combined
diagram would add clarity, so no guide edit or render was warranted. No tests,
build, CI, 2010, or Requirements Lab work.

2026-10-12: Bounded 2010-only survey compared the current reference-RTI guide,
pinned 2010 API, generated shell, reference runtime, and focused test inventory.
The guide already covers the implemented lifecycle, declaration, object,
interaction, synchronization, and logical-time-value boundaries. Seven
2010-named smoke executables focus on binding, headers, encoding, and time
values/marshalling; the binding-shell scenario does not test the implemented
synchronization barrier. Six separate external-FOM Catch2 cases cover
2010 model validation/composition, not reference-RTI behavior. Since the
existing guide already labels the source-only synchronization edge and the
time-management services are unavailable, a second standalone 2010 guide would
duplicate the current map without stronger evidence. No files changed. Source
and test bodies were inspected, not run; no build, CI, 2025/2010 sharing, or
Requirements Lab work.

2026-10-12: Reviewed the existing 2025 callback guide's HLA mental model,
callback enablement and dispatch flow, ordering cautions, callback-gated state,
embedded Send Interaction and FederationSynchronized traces, and process
registration/achievement sequence. It already separates callback dispatch
policy from recipient eligibility, protocol effects, and application
observation. HLA ordering is tied to service/message/time rules; dispatcher and
process ordering are labeled as implementation evidence. The review found one
navigation gap: the guide lacked direct locators for the callback-control
clauses it asks readers to consult. Added a compact map for Connect §4.2 and
callback-control §§10.57–10.60, explicitly warning those locators imply no
global order. No new diagram; no build, tests, CI, 2010, or Requirements Lab
work.

### 2025 Attribute Scope Advisory switch-boundary review — 2026-10-16

Compared the embedded and configured-process switch gates, event admission,
queued delivery, and focused C++ setter uses. The guide already distinguished
three source checkpoints: embedded queued closures recheck at callback entry;
process pull events remain queued and are filtered at receive; process pushed
events are checked before send, with no later registry recheck visible in the
client callback path. The setter changes switch state but does not purge the
process scope-event queue or client callback tasks.

The focused embedded switch-off segment also removes the receiver's final
regional subscription, so the self-final-unsubscribe exception prevents an
otherwise-reportable Out advisory independently of the switch. The process
regional-unsubscribe case enables the switch. Other setter tests concern MOM,
service reports, unrelated behavior, or interlocked failures. No focused case
isolates switch-only suppression or toggles the switch around queued/pushed
work. Keep those as source/test gaps; do not infer §10.1.3 meaning, pending-work
cancellation, or embedded/process parity.

No new chart was needed. Tightened the existing decision-flow node to say that
switch-off suppresses the current plan and enqueues no new callback work, not
that previously admitted work is flushed. No build, test rerun, CI, or
Requirements Lab work was performed.

Next handoff: once renderer launch access is available, visually inspect the
first queued 2025 chart, the configured-process regional-attribute DDM
sequence. Keep source/implementation checkpoints distinct from HLA callback
timing rules and do not start a renderer retry while access remains blocked.

2026-10-12: Rechecked discovery/initial-reflection order against the bounded
2025 embedded and process sources and test bodies. Embedded discovery precedes
initial-value planning, while its focused regional test records discovery and
reflection separately. Process polling prioritizes discovery before attribute
events, but the public process case's value comes from a distinct publisher
update and its recorder stores separate flags; ordinary process discovery
does not enqueue a current-value snapshot (MOM is a separate path). Neither
focused case asserts ordinary initial-reflection order. The pinned API gives
§§6.9/6.11 labels but not the normative clause text, so the issue remains open
and source comments are not elevated to HLA rules. Existing diagrams already
explain the source/test boundary; no chart changed. No build, test, CI, 2010,
or Requirements Lab work.

### 2025 class ancestry: path-specific behavior and evidence limits — 2026-10-12

The bounded audit confirmed that the existing cross-path map already separates
object-class ancestry from interaction-class ancestry and gives each service a
different job. A focused embedded discovery case registers a child-class
object, subscribes at its base class, and observes the base as the receiver's
known class with only the base attribute in the initial reflection. An internal
registry case checks ordinary interaction delivery at the exact class and
promotion to two ancestors; child-only parameters are omitted from the promoted
projections. These tests establish their respective embedded/registry behavior,
not a normative inheritance rule.

Directed interaction routing instead uses the target object's class ancestry
for declaration and receiver-selector lookup, while its parameter namespace is
the sent interaction class. The focused embedded known-target and process
multi-recipient cases use exact-class declarations; neither verifies ancestor
lookup. Keep that path source-observed. The existing map now states these test
limits directly, so another diagram would repeat it. The licensed normative
clauses remain unavailable, and no 2010 inference is made. No C++ changes,
tests, builds, CI, or Requirements Lab edits.

Next handoff: audit the HLA 2025 attribute-ownership negotiation and transfer
state machine. Separate the HLA-level request/offer/acceptance and ownership
outcomes from Umbra callback, queue, and commit points; use pinned API and
bounded source/test evidence, adding or revising a diagram only for a concrete
reader gap. Keep the 2010 stream separate; do not build, rerun tests, start CI,
or edit the Requirements Lab.

### 2025 negotiated divestiture: cancellation after confirmation — 2026-10-12

A bounded review found a reader gap in the owner-side negotiated-divestiture
flow: it showed the successful transfer after confirmation but not when the
request selected for that confirmation disappears before the owner responds.
The pinned 2025 API lists `NoAcquisitionPending` for Confirm Divestiture
(§7.6). A focused embedded case cancels the acquisition after the owner has
received `requestDivestitureConfirmation`; the owner's later confirm throws
that exception, ownership remains with the owner, the owner clears the pending
negotiation, and the requester later observes cancellation confirmation. The
registry path and test assertions support this as selected embedded behavior,
not a process-parity or universal timing claim.

Added a separate sequence diagram so the race is not blended into the
successful process-boundary negotiated-divestiture example. The source-fence
census now finds 254 Mermaid blocks across 34 diagram-bearing files: 14 2010,
225 2025 topic-guide, 13 cross-cutting, and 2 index diagrams. The new sequence
has no parser/render verdict: the authorized Scoop install found no `mmdc`
manifest and installed nothing. Evidence was inspected, not rerun. No C++
changes, builds, tests, CI, 2010 edits, or Requirements Lab work.

### 2025 ownership transfer and update-region association — 2026-10-12

A focused cross-guide review found that the DDM flow already explains how
source regions affect per-attribute routing, while the ownership flow explains
transfer outcomes; neither showed how those state questions meet at the
transfer boundary. Added a separate embedded-2025 sequence comparing three
existing focused test scenarios: the former owner's association is cleared and
not restored on reacquisition; a non-owner's still-pending deferred association
is promoted at transfer; and unassociating it first prevents promotion. The
tests are separate, and the diagram says so. API headers identify §§7.9,
7.13, and 9.6–9.7, but the full normative clause prose is unavailable, so the
association-survival detail is presented as selected embedded source/test
evidence—not a universal HLA or process-profile rule. The DDM guide now links
back to the ownership diagram. The source census is now 255 diagrams (147
flowcharts, 85 sequences, 23 state diagrams); this sequence has no fresh parser,
render, visual, or GitHub verdict. No C++ code/test execution, build, CI, 2010,
or Requirements Lab work.

### 2025 accepted TSO update across ownership transfer — 2026-10-12

The existing TSO guide already shows that an update accepted while A owns an
attribute can remain queued after B acquires it; the ownership and TSO guides
keep current authority separate from producer identity and receiver delivery.
The current focused embedded test source asserts B owns and A no longer owns
before B advances, then asserts the queued reflection still names A as
`producingFederate`. Added that direct ownership contrast beside the existing
trace. The test source has a concurrent worktree edit and was inspected only,
not executed; no runtime result for that current edit is claimed. These are
`HLA_EVOKED` source assertions, not process parity or a universal
receiver-state rule. No new diagram or count change was warranted. The test
and related TSO source were left untouched. No build, CI, 2010, or Requirements
Lab work.

Next handoff: review whether a producer that remains a member may invoke
`retract` for an already-accepted TSO update after attribute ownership
transfers. Compare the pinned Retract API, the focused
`timestamped_attribute_update_ownership_transfer_catch2.cpp` case, the
producer-bound retraction check, and the existing TSO guide. The transfer test
does not attempt Retract after transfer; do not infer normative authority from
producer identity alone. Keep 2010/2025 and embedded/process boundaries, and do
not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Retract authority after attribute ownership transfer — 2026-10-12

The embedded ambassador requires its caller to remain joined and
time-regulating; the registry additionally checks membership, that the caller
is the message's original producer, timestamp eligibility, and retraction
ledger state. The inspected path has no current-attribute-owner check. This is
a source observation only: the ownership-transfer test does not invoke Retract
after transfer, the API header lacks the full §8.22 prose, and no process-route
parity is established. The existing TSO guide now states this nuance without
turning it into a portable HLA rule. No new diagram was warranted; this docs
pass made no C++ edits and ran no build, tests, or CI, and did not change the
2010 stream or Requirements Lab.

Next handoff: verify the normative 2025 Retract authority and time-bound rules
in §§8.22–8.22.3 against the ownership boundary. If the licensed clause text
is unavailable, keep the portable outcome open and continue with another
bounded HLA-first reader gap. Consider fresh focused runtime evidence only if
a material implementation claim cannot otherwise be documented.

### 2025 Request Attribute Value Update: requester and receiver roles — 2026-10-12

A bounded read of the HLA-facing sequence, API declarations, provider planner,
and focused test bodies confirmed that requester, provider, and receiver are
roles, not necessarily distinct federates. Added a missing evidence-map entry
for one embedded `HLA_EVOKED` case: the requester has a subscription, invokes
the request, and later observes the provider's separate update. This supports
role overlap only in that subscribed scenario. The process class-request case
discovers its target objects but does not check for a later value reflection;
neither test establishes direct response entitlement for an otherwise
ineligible requester. No new diagram was warranted and no normative or
process-parity claim was added. The official IEEE listing provides purchase
and subscription access but not §6.21 clause text. No C++ edit, build, test,
CI, 2010, or Requirements Lab work occurred in this documentation pass.

Next handoff: verify the §6.21 requester-entitlement question against an
authorized full-text copy. If unavailable, keep the portable outcome open and
continue with another bounded HLA-first reader gap; preserve 2010/2025 and
embedded/process boundaries, and do not build or rerun tests for documentation
work.

### 2025 DataElement encoding guide: representation boundaries — 2026-10-12

A bounded HLA-first review of the existing 2025 encoding guide, pinned
`DataElement` API, and its linked focused-test map found no reader gap that
needs another umbrella diagram. The guide already distinguishes HLA value
octets from typed-handle encoding, private process framing, and model input;
then explains the selected element's explicit byte order, composite child
placement/cursor rules, variant discriminants, and factory-defined logical
time. Its source and test links keep the implementation evidence separate from
the portable explanation. A new general encoding chart would duplicate those
flows, so no diagram or guide change was warranted. This survey did not merge
the separate 2010 encoding stream or claim cross-edition parity; no renderer
was needed because no chart changed. No C++ build, test, CI, or Requirements
Lab work occurred.

Next handoff remains the authorized-text check for 2025 §6.21 requester
entitlement. If the full clause is unavailable, keep that outcome open and
continue with another bounded HLA-first reader gap. Preserve 2010/2025 and
embedded/process boundaries; do not build or rerun tests for documentation
work.

### 2025 DDM: pending range query versus committed routing scope — 2026-10-12

Refined the existing region-template state diagram to show an important
two-view distinction in the embedded profile: `getRangeBounds` can expose a
pending edit while regional matching continues to use the previous committed
specification. Added nearby evidence links for the focused lifecycle case and
the separate discovery-after-commit case. The tests observe the query values
and the post-commit discovery, but do not send a value between edit and commit;
continued use of the prior committed range in that interval is explicitly
labeled source-derived from the region matcher. This updates an existing chart,
so the diagram count is unchanged. It has no fresh syntax, render, screenshot,
or GitHub verdict. No C++ edit, build, test, CI, 2010, or Requirements Lab work.

Next handoff remains the authorized-text check for 2025 §6.21 requester
entitlement. If that clause is unavailable, continue a bounded HLA-first reader
audit elsewhere; preserve 2010/2025 and embedded/process boundaries and keep
documentation work off the C++ build/test path.

### Goal focus: portable HLA behavior first — 2026-10-12

Refined the ongoing flow-guide objective: lead with the durable HLA model—its
states, events, decisions, and obligations—and use Umbra source plus existing
focused tests to explain difficult semantics or bound what the implementation
currently demonstrates. Keep implementation-only mechanics visibly separate;
do not turn internal code structure into the primary diagram. Prefer recorded
evidence and do not compile, rerun tests, or start CI for documentation work.
Preserve the 2010 and 2025 compatibility streams as separate boundaries.

The deferred update-region association filesystem case remains a useful
implementation-specific save/restore example, but it does not alone establish
a portable HLA rule, so it is not promoted as the next conceptual flow. The
next handoff remains the authorized-text check for the 2025 §6.21 requester
entitlement question; if that text is unavailable, choose another bounded
HLA-first reader gap. No diagram or C++ behavior changed in this scope update.

### HLA guide local-link integrity check — 2026-10-12

Scanned the 43 Markdown files under `docs/design` for local Markdown link
destinations. All 2,498 relative file targets exist. This is a path-existence
check only; heading and line fragments, Mermaid parsing/rendering, visual
layout, and GitHub rendering remain separate validation work. A fresh fence
census still matches the roadmap at 255 diagrams in 34 files (147 flowcharts,
85 sequences, 23 state diagrams); no inventory correction was needed.

The §6.21 requester-entitlement question remains open pending authorized
normative text; the official public listing/API archive does not provide that
clause. Next handoff: validate heading fragments for local Markdown links
across `docs/design` (the 2,498-target scan checked file paths, not anchors),
then continue a bounded HLA-first reader-gap survey and change a guide only
where the portable concept is not already clearly covered. Keep
implementation-only behavior, 2010, 2025, embedded/process profiles, and the
Requirements Lab within their stated boundaries. No C++ change, build, test,
or CI was performed.

### HLA guide heading-fragment audit — 2026-10-12

Checked 179 local Markdown heading fragments referenced from the 43 files in
`docs/design`, resolving destinations across `docs`. The audit found one stale
anchor: the object-information guide linked to `normative-source-access`, but
the target heading includes its date and therefore has a dated heading slug.
Updated the link; the follow-up audit found zero unresolved heading fragments.
The audit does not validate source line anchors, Mermaid syntax/rendering,
visual layout, or GitHub's live renderer. No diagram or C++ behavior changed.

Next handoff: keep the 2025 §6.21 requester-entitlement question open pending
authorized normative text; continue a bounded HLA-first reader-gap survey and
change a guide only when it resolves a real gap. Preserve 2010/2025 and
embedded/process boundaries; do not build, rerun tests, start CI, or edit the
Requirements Lab for documentation work.

### 2025 active relevance and passive receiver survey — 2026-10-12

Rechecked the subscription-state guide against the pinned §5.8 signature, the
single-publisher/single-subscriber passive test, and the shared 2025 discovery
and update-recipient source. The existing guide already separates federation-
wide active relevance from each receiver's own active/passive eligibility and
labels the multi-member behavior as source-derived. The cited test does not
cover a separate active establisher plus passive receiver, and the local API
header does not contain the full normative clause. Because the existing HLA-
first and source-derived diagrams already explain the distinction and its
evidence limits, no duplicate diagram was added. Citation verification also
found and corrected a stale discovery-resolver source line link (855 to 869).
Their visual review remains pending. No 2010 behavior was inferred; no code, build, test, CI, or
Requirements Lab work occurred.

Next handoff: visually review the passive-recipient and HLA-first late-
subscription diagrams with a local Markdown renderer. If `mmdc` is needed,
use Scoop only if a usable manifest is available; then resume the bounded
HLA-first topic survey. Keep §6.21 unresolved without authorized text and
preserve edition/profile boundaries.

### Design-doc source/test line-link range audit — 2026-10-12

Checked 1,758 local `#L...` source/test links from the 43 Markdown files in
`docs/design` against the current worktree. Every target file exists and every
linked line is within its target file. This range check does not prove that a
line still describes the cited symbol: the earlier discovery-resolver link was
in-range but stale, so it was corrected separately. Network-enabled Scoop
searches for both `mmdc` and `mermaid` found no usable manifest; no package was
installed. Visual review of the passive-recipient and late-subscription charts
is still pending. No diagram or C++ behavior changed; no build, tests, or CI.

Next handoff: try the existing local Markdown preview path for those two
subscription-state diagrams when a supported preview surface is available. If
not, keep visual status pending and continue a bounded HLA-first survey from
the prioritized backlog; leave §6.21 open without authorized text and keep
2010/2025 and embedded/process boundaries separate.

### HLA-first documentation focus and preview restriction — 2026-10-12

Reconfirmed the goal's reader-first scope: explain portable HLA roles, states,
events, eligibility decisions, and observable outcomes, using Umbra source and
existing focused tests as bounded evidence rather than making C++ architecture
the primary model. The subscription guide already separates federation-level
active relevance from receiver-local active/passive eligibility, and keeps the
known-object/new-attribute value question open; another diagram would duplicate
that explanation. No C++ builds, tests, or CI were run.

The allowed Scoop search found no usable `mmdc` manifest. A local in-app
preview attempt was blocked because `data:` URLs are not supported by the
browser policy. I did not try a different browser surface or route around that
restriction. Visual review of the passive-recipient and late-subscription
diagrams remains pending; no chart was changed or rendered.

Next handoff: continue a bounded, content-first HLA reader-gap survey from the
prioritized backlog, making a guide change only for a real conceptual gap.
Keep the 2025 §6.21 requester-entitlement interpretation unresolved without
authorized normative text, preserve 2010/2025 and embedded/process boundaries,
and keep builds, tests, CI, and Requirements Lab edits out of documentation
work.

### Current HLA guide link and class-ancestry audit — 2026-10-12

Rechecked 1,795 inline local source/test line links across the 43 Markdown
files in `docs/design`. Every target file exists and every referenced line is
within bounds. This is only a range/path check, not proof that each line still
describes the linked behavior. A bounded HLA-first review of class ancestry
also checked the current guide's selected discovery, attribute-projection,
ordinary-interaction-promotion, and directed-target lookup distinctions
against their cited source and test examples. The guide keeps them as
path-specific 2025 observations, distinguishes focused test evidence from
source-only edges, and does not claim one universal inheritance rule. No new
diagram or guide edit was warranted. No C++ build, tests, CI, 2010 inference,
or Requirements Lab edit occurred.

Next handoff: continue with one specific reader gap from the prioritized
backlog; avoid umbrella duplication and keep the HLA behavior model primary.
Visual review of the passive-recipient and late-subscription diagrams remains
pending until a supported local preview is available. Keep §6.21 unresolved
without authorized clause text and preserve the separate edition/profile
boundaries.

### HLA report and identity boundary audit — 2026-10-12

A bounded HLA-first survey compared the existing 2025 TSO retraction,
attribute-value request, federation MOM, service-invocation reporting,
exception-reporting, and name/handle guides. Their current maps already keep
the portable distinctions visible: producer acceptance versus recipient
outcome; provider solicitation versus a later value-bearing update; the
execution-scoped `HLAfederation` object versus member-scoped state; a public
service result versus requested MOM report delivery and invocation reporting;
and public typed/opaque handles versus Umbra's private numeric representation.
The existing explanations also label their embedded/process evidence limits.
No new diagram or guide edit was warranted, and unresolved 2025 §§6.21 and
8.22.3 questions remain open without authorized normative text.

Network-enabled `scoop search mmdc` and `scoop search mermaid` returned no
matches, so no renderer was installed and no chart was rendered. The passive-
recipient and late-subscription visual reviews remain pending; no alternate
browser route was attempted. No C++ build, test, CI, 2010, or Requirements Lab
work occurred.

Next handoff: compare the 2025 federation-scoped MOM and joined-federate MOM
guides around execution-level versus membership-level state, observer
knowledge/value paths, and resignation effects. Keep the HLA model primary,
consult the pinned 2025 MIM/API and existing bounded evidence, preserve
embedded/process and 2010 boundaries, and add or revise a diagram only for a
demonstrated learner gap. Keep the licensed-text questions and pending visual
review explicit; do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 Federation MOM execution/member observation map — 2026-10-12

The guide comparison exposed one synthesis gap: the Federation and joined-
federate guides explained their projections separately but did not put their
relationship and observer-side paths in one HLA-first view. Added a scope map
showing the MIM's one-per-execution `HLAfederation` projection, conditional
`HLAfederatesInFederation` reference-list value, and one-per-member
`HLAfederate` projection. It distinguishes list reflection from member-object
discovery/removal and explicitly makes no cross-path callback-order claim.
The embedded focused cases observe membership-list reflection and member
object removal separately; a process case orders removal before rediscovery/
reflection of that member object but does not cover the list reflection. These
tests were inspected, not run; there is no embedded/process parity claim. The
2025 joined-federate guide now links to the map. The new chart has no parser or
render verdict because `mmdc` is unavailable; the current inventory is 256
diagrams (148 flowcharts, 85 sequences, 23 states). No build, tests, CI, 2010,
or Requirements Lab work occurred.

Next handoff: audit whether a receiver of `HLAfederatesInFederation` may treat
an `HLAfederateReference`/`HLAfederateHandle` as knowledge of that member's
`HLAfederate` object. Compare the pinned 2025 MIM type with the existing
name/handle and joined-federate guides and only bounded evidence. Keep
referential identity separate from discovery/object knowledge; add text or a
diagram only if a concrete learner gap remains. Preserve 2010 and embedded/
process boundaries; leave §6.21 and §8.22.3 open, and do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 MOM reference identity versus receiver object knowledge — 2026-10-12

Audited whether a member reference in `HLAfederatesInFederation` implies that
the receiving federate knows the corresponding `HLAfederate` object. The
pinned MIM says the reference is represented by a `FederateHandle` and points
to the object's `HLAfederateHandle` attribute; it does not describe the
receiver's discovery state. The pinned API's object-instance support lookups
take `ObjectInstanceHandle` values and can report `ObjectInstanceNotKnown`.
Thus the list provides federate identity, not evidence that a particular
receiver has the member object's public instance handle/knowledge. This is a
typed API and receiver-scope distinction, not a newly asserted callback rule.

The membership test decodes the list and checks its federate handles but does
not test support lookups for the referenced member object. A separate joined-
federate MOM case obtains the object-instance handle through discovery and
then checks support lookups. Added a bounded explanation and reciprocal links
between the Federation MOM and name/handle guides; no duplicate diagram was
needed. No test was run, no 2010 behavior was inferred, and the existing MOM
scope map still lacks a fresh parser/render verdict.

Next handoff: audit the 2025 Federation MOM newcomer path. Compare the MIM's
`Conditional` `HLAfederatesInFederation` update-on-Join/Resign semantics with
the explicit initial value request in the focused membership test and the
current Federation MOM guide. Separate the current membership value from
automatic Join/discovery callbacks; document any gap only with a pinned
contract or bounded evidence. Preserve 2010 and embedded/process boundaries;
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Federation MOM late-observer membership value — 2026-10-12

The Federation MOM guide already showed subscribe → discovery → current-value
request → later conditional Join/Resign reflections, but did not state how that
path applies to a federate joining an already populated execution. The pinned
API requires execution membership for object-class subscription; the MIM marks
the member list Conditional on Join/Resign and separately requires a response
to Request Attribute Value Update for Conditional attributes. The evidence
does not establish that an earlier conditional update is replayed to a member
that subscribes after its own Join. Added an explicit late-observer note: after
discovery, request the current list rather than assuming discovery supplied a
snapshot. It expressly avoids asserting that no implementation can deliver an
extra reflection.

The existing focused test is observer-first: it subscribes, requests the
initial current list, and then observes a second member's Join/Resign updates.
It does not test the late-observer case. No new diagram was needed because the
existing sequence already shows the affirmative path. No test, build, CI, 2010,
or Requirements Lab work occurred; the diagram inventory remains 256 and its
new MOM map still has no parser/render verdict.

Next handoff: audit the Federation MOM's conditional FOM-composition values
(`HLAFOMmoduleDesignatorList` and `HLAcurrentFDD`) against the 2025 FOM module
admission/composition guide. Compare their MIM update triggers with what a
joining member can resolve from its effective FOM and what a current-value
request returns. Keep model declarations separate from MOM projections and
request/report interactions; add a cross-link or diagram only if a concrete
reader gap remains. Preserve 2010 and embedded/process boundaries; do not
build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Federation MOM effective FOM and module-data views — 2026-10-12

Compared the current Federation MOM guide with the FOM module
admission/composition guide, pinned 2025 MIM rows, and focused test sources.
The MIM uses `HLAFOMmoduleDesignatorList` on two different object scopes: the
federation object's list is Conditional on FOM additions through Create/Join,
while each joined-federate object's same-named list is Static and contains
modules supplied for that member's Join. `HLAcurrentFDD` is the federation's
Conditional current-FDD value. The HLAfederation class semantics separately
require a current-value response to Request Attribute Value Update regardless
of update type. The embedded current-FDD test observes a changed reflection
after an additional-module Join and verifies that a subsequent explicit
request returns the same value. A separate module-data report is a
receiveInteraction carrying selected module content, not the composed FDD.

Added a scope-comparison table in the Federation MOM guide and reciprocal
cross-links to the FOM guide. Existing diagrams already show the distinct
composition, attribute-reflection, and report paths, so no duplicate diagram
was added. The per-member snapshot test uses a private inspection seam and is
identified as such. A separate public member-object lifecycle test verifies
that the reflected member list matches its Join-supplied module. Neither test
proves the public federation-wide list update.
No tests were rerun, no build or CI was started, and the 2010 stream and
Requirements Lab were untouched. The diagram inventory remains 256; no new
parser/render verdict is claimed.

Next handoff: check the bounded 2025 tests and source for a public observer
assertion that the federation-scoped `HLAFOMmoduleDesignatorList` changes
after Create/Join. Keep that distinct from the per-member Static list,
`HLAcurrentFDD`, and module-data request/report interactions. Do not build,
rerun tests, start CI, or edit the Requirements Lab; preserve 2010 and
embedded/process boundaries.

## 2025 Federation MOM federation-list update evidence — 2026-10-12

The pinned MIM makes the federation object's `HLAFOMmoduleDesignatorList`
Conditional on new FOM modules supplied at Create or Join; the same-named
`HLAfederate` value is Static and records only that member's Join-supplied
modules. In the selected embedded source, the federation attribute is derived
from the current federation definition, filters out the standard MIM, and
retains the first designator for each canonical module-source identity. The
additional-module Join path queues Conditional updates for this list and
`HLAcurrentFDD`. This is source evidence for that embedded Join path, not
evidence of a Create update path or process parity.

A bounded repository-wide test-source search found public assertions for the
member-scoped list and a separate assertion for changed `HLAcurrentFDD`, but no
focused public observer assertion for the federation-wide list changing after
Create/Join. Neither adjacent test closes that gap: the public lifecycle test
observes the `HLAfederate` object, while the current-FDD test asserts only the
FDD attribute. Existing diagrams already distinguish composition, attribute
reflection, and module-data reporting; no new chart was warranted. Diagram
inventory remains 256 and the existing MOM-map render status is unchanged.
Tests were inspected, not rerun. No build, CI, 2010, or Requirements Lab work.

Next handoff: compare the 2025 MIM's repeated-identical-module first-designator
rule with the embedded list encoder's canonical-source de-duplication, then
check whether focused tests assert that behavior separately for federation
and member lists. Keep object scopes and 2010/process boundaries explicit; do
not build, rerun tests, start CI, or edit the Requirements Lab.

## 2025 Federation MOM duplicate-designator scope — 2026-10-12

The 2025 MIM's first-designator rule applies to both the federation-level
Conditional list and member-level Static list. The embedded implementation
uses the same list encoder for both projections and de-duplicates by canonical
`sourcePath`, retaining the first supplied designator. A private registry test
passes two joined-member module records with a shared canonical source path and
different designators, then asserts a one-entry member snapshot containing
the first designator. This tests the private member projection, not public
Join behavior. The public lifecycle test verifies only the one-module member
list, and no federation-level list assertion was found. Existing composition
tests for equivalent module declarations do not assert MOM designator-list
output. Whether byte-identical module content at distinct canonical paths is
“identical” for the MIM rule remains unresolved; no conformance gap is claimed.

No diagram changed; this source/test distinction is explanatory and the
current flows already separate the two MOM scopes. The 256-diagram inventory
and pending MOM-map render status are unchanged. No build, test rerun, CI,
2010, or Requirements Lab work occurred.

Next handoff: inspect the pinned 2025 standard's definition and examples for
what makes two FOM modules “identical,” then compare that meaning with Umbra's
canonical-path identity. Treat same-content files at distinct paths as an open
semantic question until the normative text answers it; preserve 2010 and
embedded/process boundaries and do not build or rerun tests.

## 2025 Federation MOM module-identity evidence boundary — 2026-10-12

The MIM's first-designator rule is explicit, but it does not define how two
FOM modules are judged identical. The checked-in 1516.2-2025 package has the
MIM, schemas, and Restaurant examples; it does not include the full OMT prose
needed to check whether the standard supplies a more specific identity test
for byte-identical modules at distinct canonical paths. IEEE SA lists the full
standard through purchase or subscription, separately from its additional
resources
([publication and access](https://standards.ieee.org/ieee/1516.2/6689/)).
Umbra's canonical-`sourcePath` de-duplication remains a source-level behavior,
not a normative definition. The question stays open without a conformance
finding. No diagram changed; inventory remains 256 and MOM-map parser/render
status is unchanged. No build, test rerun, CI, 2010, or Requirements Lab work.

Next handoff: audit the 2025 object-deletion/attribute-ownership boundary
across the object-information and ownership guides. Compare portable service
and callback concepts with exact focused test evidence; keep accepted
federation deletion, ownership cleanup, and receiver-local forgetting
distinct. Revise a diagram only if the survey identifies a genuine reader
gap. Preserve 2010 and embedded/process boundaries; no builds, test reruns,
CI, or Requirements Lab edits.

## 2025 Object deletion and ownership-cleanup wayfinding — 2026-10-12

The existing cross-cutting map distinguishes federation-wide delete from
federate-local forgetting. The object guide gives the `deleteObjectInstance`,
`removeObjectInstance`, and `localDeleteObjectInstance` API roles and the
current receiver-removal gate; the ownership companion covers accepted-delete
ledger cleanup and callback-time checks. Its query, regular release-request,
If-Available, and cancellation branches have separate focused HLA_EVOKED test
scenarios, while If-Wanted, negotiated divestiture, and Confirm Divestiture
remain source-observed after deletion. Local forget is separately gated when
relevant ownership work is pending; the cited process local-delete test covers
success only. No additional state machine was warranted. Added a direct link
from the object-removal guide to the ownership cleanup section. No diagram
changed; inventory remains 256, with no new parser/render result. No build,
test rerun, CI, 2010, or Requirements Lab work.

Next handoff: audit the 2025 ResignAction boundary across federation
lifecycle, object-information, and ownership guides. Compare portable
service/callback concepts with focused evidence; keep membership departure,
object deletion/removal, and attribute divestiture as distinct effects. Revise
a flow only if a genuine reader gap appears. Preserve the separate 2010
reference-RTI path and embedded/process boundaries; do not build, rerun tests,
start CI, or edit the Requirements Lab.

## 2025 ResignAction cross-guide boundary — 2026-10-12

The current guides separate the action vocabulary, successful member departure,
execution lifetime, object deletion/removal, and per-attribute divestiture.
The lifecycle guide's embedded table and sequence give the action-level
overview; the ownership companion carries the pending-acquisition role scan
and selected cancellation evidence; the object guide covers delete privilege
and per-receiver removal. The focused embedded scenarios inspected here show
pending-acquisition rejection, `NO_ACTION` rejection while attributes remain
owned, `DELETE_OBJECTS_THEN_DIVEST` producing separate removal and assumption
callbacks, and final-member object cleanup without destroying the execution.
The assumption-before-removal order is only one `HLA_EVOKED` test observation.
The selected process case covers `NO_ACTION` MOM-object lifecycle, not the
action matrix. Normative guard/effect details and callback ordering remain
unverified without the licensed standard. Added a direct link from the core
ownership guide to the lifecycle action table; a combined diagram would
duplicate separate service and callback state machines. Inventory remains
256; no diagram, test, build, CI, 2010, or Requirements Lab work.

## 2025 Connection-loss resignation: directive versus forced cleanup — 2026-10-12

The lifecycle guide's sequence and prose identified the per-member automatic
directive and forced cleanup, but left the boundary between the configured
action and forced-loss overrides too implicit. Added a concise implementation
map separating the stored action, forced acquisition cancellation/owned-
attribute divestiture, and the last-member delete pass. Explicitly stated that
transport loss is not an application `resignFederationExecution(action)` call.
The automatic directive remains membership-local and is seeded from the
composed FDD; the current registry reads that member value on connection loss.

The focused embedded `NO_ACTION` case shows a retained object, an ownership-
assumption callback, and no removal callback when owned attributes leave the
departing member. The pending-acquisition loss case uses
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, so it does not alone prove that the
forced cancellation overlay is independent of the directive; that independence
is source-observed in the shared registry transition. The process entry routes
endpoint loss through that registry and event projection. Adapter-managed
process TCK scenario sources cover configured unconditional divestiture and
configured acquisition cancellation, but not the process forced overlay under
`NO_ACTION`. These sources were inspected, not run. No process/embedded parity,
callback-order guarantee, full normative action matrix, or 2010 equivalence is
claimed.

No diagram changed, so the inventory remains 256 and Mermaid render status is
unchanged. No build, test rerun, CI, 2010 change, or Requirements Lab work.

Next handoff: audit 2025 connection loss during active Federation Save or
Federation Restore. Compare the forced-loss entry with the shared save/restore
failure transitions and available embedded/process evidence; keep connection
loss distinct from voluntary resignation, label untested profile behavior,
and add a flow only for a concrete reader gap. Preserve 2010 separately; do
not build, rerun tests, start CI, or edit the Requirements Lab.

## 2025 Connection loss during active Save/Restore — 2026-10-12

Compared forced `connectionLost` and voluntary process `handleResign` with
the shared 2025 registry transition. The registry fails an active Save with
`FEDERATE_RESIGNED_DURING_SAVE` or an active Restore with
`FEDERATE_RESIGNED_DURING_RESTORE`, excludes the departing participant and
clears the operation before membership removal. Save completion additionally
skips participants still at `NO_SAVE_IN_PROGRESS` and missing callback routes;
Restore skips the departing or unrouteable member. The automatic ResignAction
does not select this operation failure.

The embedded membership-loss adapter extracts and submits both notification
vectors. The process loss path and voluntary resign path both pass results to
`dispatchConnectionLossResult`, whose current source handles ownership work,
object removals, synchronization notifications, and an eligible federate-lost
report but not Save/Restore notifications. The process profile therefore has
a source-observed result-projection gap for either member-departure entry
path; runtime behavior remains unverified. Existing save/restore reason-code
tests exercise voluntary embedded resignation, and no focused connection-loss
during-active-save/restore test was found among the loss-focused Catch2 cases
or the process connection-lifecycle TCK source. Sources were inspected, not
run. No 2010 rule or profile parity is inferred.

Updated the lifecycle guide and added a reciprocal link from Save/Restore.
No new flow was justified: a single diagram would blur the embedded callback
submission and process dispatcher omission. The current inventory is 256
Mermaid diagrams across 34 diagram-bearing docs/design files, counting both
backtick and tilde fences. No Mermaid changed, so prior render state is
unchanged. No build, test rerun, CI,
2010, or Requirements Lab work.

Next handoff: audit 2025 forced connection loss during an active
synchronization point, focusing on participant removal, success/failure
notification eligibility, embedded/process event projection, and available
focused evidence. Change only the guide(s) with a concrete reader gap; keep
2010 separate and do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 Forced connection loss during a pending synchronization point — 2026-10-12

The synchronization guide now distinguishes the portable barrier vocabulary
from Umbra's 2025 forced-loss transition. The registry removes the departed
member from the participant, announced, and achieved sets before checking
completion; it does not add that member to the failed-to-synchronize set. The
point remains pending if a remaining member has not achieved, closes when all
remaining members have achieved, and is erased without a notification when no
participants remain. Notifications are planned only for remaining participants
with usable callback routes.

The embedded membership-loss adapter submits the resulting synchronization
notifications. The process connection-loss handler passes the registry result
through a dispatcher that enqueues those notifications. The bounded scan found
no focused forced-loss-during-active-synchronization case; the existing
two-member HLA_EVOKED resignation test covers voluntary departure only. This is
source-observed 2025 behavior, not a normative transport-loss rule or 2010
claim. The existing barrier chart already covers the closure transition, so no
duplicate diagram was added.

The complete inventory is 256 Mermaid fences across 34 design files: 250
backtick-fenced and six tilde-fenced. No diagram changed or was rendered. No
build, test rerun, CI, 2010, or Requirements Lab work occurred.

Next handoff: audit 2025 delivery of a planned FederationSynchronized callback
when its recipient resigns or loses connection before application observation.
Compare embedded pending-callback cancellation/staleness behavior with process
queued-event handling and focused tests; document only a concrete reader gap.
Keep 2010 separate, do not infer profile parity, and do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 FederationSynchronized recipient departure before observation — 2026-10-12

Compared the completed synchronization callback's lifetime across the 2025
embedded and process paths. Embedded Resign and forced connection loss reset
pending tasks on the departing ambassador's callback dispatcher; reset does
not undo an invocation already claimed or entered. Process Resign clears the
server's joined-session synchronization event queue, but it does not clear
events already buffered in the client or close the callback bridge. The bridge
can therefore retain work for later Evoke/Enable dispatch after Resign; client
close clears the buffered and dispatched queues. A frame still in transit on
connection loss remains outside this source/test review.

The request ordering sharpens this boundary: the client dispatches buffered
events after matching a Resign response but before the Resign method clears its
local joined identity. An enabled HLA_IMMEDIATE callback may run inline while
the API call is still in progress; HLA_EVOKED or disabled-immediate work can
remain queued for later dispatch. This timing is source-derived, and the
focused cases do not exercise it for FederationSynchronized.

Focused tests cover a survivor receiving completion because another member
resigns, plus ordinary process callback delivery before the tested members
later resign. None asserts delivery or suppression after the callback
recipient's own resignation or connection loss. This is source-observed 2025
implementation behavior, not a portable post-Resign rule, runtime proof, or
2010/process parity claim. The callback guide now carries the comparison and
the synchronization guide links to it. Existing diagrams already cover
barrier closure and callback dispatch separately; no new chart was warranted.
The inventory remains 256 Mermaid fences across 34 docs/design files. No
diagram changed or was rendered. No build, test rerun, CI, 2010, or Requirements
Lab work occurred.

### 2025 Process object-instance discovery after recipient Resign — 2026-10-12

Audited the 2025 process object-discovery event after recipient Resign. The
server's joined-session cleanup clears discovery events that remain in its
queue. For events already received by the client or submitted to its bridge,
the bridge submission checks only its open session/dispatcher; the discovery
delivery routine validates payload identities and name, but does not recheck
current federation membership or current registry object state. This leaves
an event-specific source path through the generic process post-Resign callback
queue, not proof of a callback after Resign or a normative HLA rule.

The public process object-discovery test runs both HLA_EVOKED and
HLA_IMMEDIATE, asserts the discovered handle/class/name/producer, and only
then resigns sender and receiver. It does not test discovery pending across
the recipient's Resign. A separate process failed-Join scenario discards a
new MOM discovery queued for a failed membership; that rollback is a
different transition and not resignation evidence. The callback guide now
records this distinction, and the object/interaction guide links to it. No
new diagram was justified because the existing post-Resign flow already maps
the shared queue gate; no diagram changed or was rendered. Inventory remains
256 Mermaid fences across 34 docs/design files. No build, test rerun, CI, 2010,
or Requirements Lab work.

Next handoff: audit 2025 process DELETE_OBJECTS resignation effects for a
surviving receiver. Trace per-receiver known-object cleanup and
removeObjectInstance callback gating/order against the focused process
resignation/deletion cases; distinguish producer Resign from the recipient's
own Resign and document only proven process-profile behavior. Keep embedded
and 2010 paths separate; no builds, test reruns, CI, or Requirements Lab work.

### 2025 Process `DELETE_OBJECTS` resignation to a surviving receiver — 2026-10-12

Audited the process Resign handler, registry per-recipient removal planning,
pull/push commit gates, receiver session cleanup, and callback bridge. The
object guide now distinguishes producer Resign from recipient Resign and
includes one source-derived flowchart. The portable C++ TCK scenario expects a
survivor callback followed by unknown-object lookups; the focused Umbra process
deletion test covers direct deletion, not the Resign trigger. Removal events
still in a receiver's server queue are cleared by its own Resign; outcomes for
already-pushed callback work are unresolved. No process test was run, no 2010
or embedded parity is claimed, and no Requirements Lab work occurred. The new
chart has no fresh syntax/render verdict.

Next handoff: audit 2025 process `DELETE_OBJECTS_THEN_DIVEST` for a surviving
member when the departing producer has both a delete-privileged object and
owned attributes. Trace object-removal and ownership-assumption planning,
recipient gates, and callback ordering across process source and the portable
TCK/selected embedded tests; label every profile-specific or untested edge.
Keep 2010 separate; no builds, test reruns, CI, or Requirements Lab work.

### 2025 process `DELETE_OBJECTS_THEN_DIVEST` callback ordering — 2026-10-12

Audited the registry's delete-then-divest passes, the embedded voluntary
scenario, the portable C++ TCK cross-object case, and process result
dispatch/polling. The embedded HLA_EVOKED test observes assumption before
removal. Process dispatch queues assumptions before removals, but default-pull
receive selection prioritizes the removal queue; optional push submits the
assumption frame first. No focused process test establishes callback order.
The portable TCK checks deleted versus retained object state and later
If-Available acquisition, not the assumption callback or its ordering. Added a
second source-derived flowchart; both process charts remain unrendered and
unparsed. The shared roadmap had a missing comma before an unrelated
`active_handoff` field; repaired only that separator so the JSON parses. No
build, test rerun, CI, 2010, or Requirements Lab work.

Next handoff: audit 2025 process receiver Resign after producer
`DELETE_OBJECTS_THEN_DIVEST` when ownership-assumption and object-removal work
are pending. Trace registry callback-ledger invalidation and session queue
clearing; distinguish pull-queued work from frames already pushed to the
client, and identify focused evidence. Keep this separate from producer
Resign and the embedded HLA_EVOKED order; no builds, test reruns, CI, 2010, or
Requirements Lab work.

### 2025 process receiver Resign with pending assumption and removal — 2026-10-12

Added a third source-derived process flowchart to the 2025 object/interaction
guide. Registry Resign invalidates the receiver's pending assumption marker
and removal reservation; process dispatch filters queued ownership events for
the departed ID, and joined-session cleanup clears the ownership and removal
event queues for that session before the
successful response. Push has a different boundary: frames read during the
Resign exchange are buffered, then submitted to the callback bridge after the
response. The process branch does not reset the callback dispatcher, unlike
the embedded branch. The nearest focused process ownership test consumes the
assumption before Resign; no focused test asserts the receiver-Resign race for
pending assumption/removal frames. This is source-derived, not a normative or
cross-profile callback guarantee. The chart has no fresh syntax/render review.
No build, test rerun, CI, 2010, or Requirements Lab work.

### 2025 process Request Retraction after receiver Resign — 2026-10-12

Audited the process TSO attribute-update recipient cache, receiver-Resign
cleanup, producer Retract handler, registry recipient ledger, client event
buffer, and callback-bridge admission. Receiver Resign clears the server
session queues but does not prune that attached session from other messages'
retraction-route cache; Detach does. A later producer Retract can therefore
send a `request_retraction` transport frame to a session that has resigned.
The callback bridge separately requires tracked message state and an original
callback-started marker before queuing the public callback; this check is not a
current-membership check. It is a source-supported process route, not a
normative permission or a proven post-Resign callback. The focused process
attribute tests cover retraction before callback observation/fan-out, not a
receiver-Resign race. Added a sequence to the TSO guide and a cross-link from
the callback guide. No test or build was run. Scoop search found no configured
`mmdc` package, so syntax/render review remains pending. Inventory is now 260
diagrams (151 flowcharts, 86 sequences, 23 state diagrams) across 34 files.
No CI, 2010, or Requirements Lab work.

Next handoff: compare 2025 process timestamped-interaction retraction with the
attribute-update route documented here. Trace session-cache insertion,
receiver-Resign cleanup, client buffering, callback-state admission, and
focused process evidence; record any divergence without assuming behavior is
shared just because both use a retraction designator. Keep embedded and 2010
separate; no builds, test reruns, CI, or Requirements Lab work.

### 2025 process timestamped interaction retraction-route comparison — 2026-10-12

Compared ordinary process interaction and attribute retraction paths without
assuming shared behavior from their common designator. For coordinator-
dispatched TSO delivery, both event families reserve registry recipient state,
send the event frame, then cache the session. The send-time branches differ:
timestamped attribute updates cache pull-queued recipients, while ordinary
interactions cache only the pushed branch; the pull interaction path queues an
event without a route insertion in that handler. Their client filters differ
too: Request Retraction removes a still-buffered interaction and defers its
TSO acknowledgement, but leaves a buffered attribute event for the bridge's
per-message `RetractionState` gate. The process registry's `delivered` marker
is earlier than public callback entry; the callback bridge tracks that entry
separately. A bounded search found process interaction evidence for a
coordinator-delivered callback followed by a too-late Retract, but no focused
pre-callback, send-time-pull, or receiver-Resign interaction case. These are
source/test-coverage observations, not an HLA conformance conclusion. Narrowed
the existing TSO sequence to the coordinator-delivered attribute route and
added a comparison table. Mermaid syntax/render review remains pending; Scoop
has no matching `mmdc` or `mermaid-cli` package. No build, test rerun, CI, 2010,
or Requirements Lab work. Diagram inventory remains 260 across 34 files.

Next handoff: audit the 2025 process timestamped object-removal retraction
route against the attribute/interaction comparison. Trace recipient-route
insertion, receiver Resign versus detach, client event filtering, callback
admission, and the focused removal tests. Keep embedded and 2010 evidence
separate; document source/test gaps without inferring a normative result. No
builds, test reruns, CI, or Requirements Lab work.

### 2025 process timestamped-delete retraction-route comparison — 2026-10-12

The process Delete Object Instance path splits a retractable timestamped
deletion's recipients: time-constrained receivers stay in the coordinator TSO
queue, while other planned receivers take a send-time RECEIVE-order removal
path. The coordinator path caches a process session after sending the removal
frame; the send-time path records it while queueing or preparing the event.
Receiver Resign clears its joined identity and server queues but does not prune
that attached session from other messages' retraction routes; Detach does. A
later valid producer Retract can send `request_retraction` without a live-
membership recheck. The client removes a still-buffered removal event and
defers its private acknowledgement; once the event is submitted, the callback
bridge's per-message state suppresses an unstarted pair or admits Request
Retraction after the original callback has started. A missing bridge state
produces no public Request Retraction callback. This is a source trace, not a
post-Resign runtime result or normative permission.

The focused process delete test covers callback-model, regulation,
time-constrained, preferred-order, and callback-gating scenarios and checks
whether a public handle is exposed; it does not call Retract or test receiver
Resign after route insertion. The separate embedded delete-retraction case
does not establish process parity. No test or build was run. Added one
sequence to the TSO guide; it has no fresh syntax/render verdict. Inventory is
261 Mermaid fences across 34 files (151 flowcharts, 87 sequences, 23 state
diagrams). No CI, 2010, or Requirements Lab work.

### 2025 process Disconnect versus server Detach — 2026-10-12

Expanded the existing connection guide's Disconnect flow, not the membership
state map. It now separates the adapter `CallbackSession` from the process
callback bridge and the local `ProcessFederationClient::close` from
server-side `ProcessFederationService::detach`. The public process path closes
the client socket; there is no Disconnect service message in the protocol
operation enum. The service header assigns Detach to a transport owner after
its session loop exits, but a bounded search found no production callsite in
this checkout. The process-endpoint lifecycle test calls Detach explicitly in
its server fixture after the planned requests, so it does not prove API-caused
or synchronous server cleanup. This remains an Umbra implementation/test
boundary, not an HLA rule or 2010 claim. No C++ source, build, test, CI, or
Requirements Lab work occurred. The revised diagram does not change the 261
diagram count and has no fresh syntax/render verdict.

Next handoff: survey the 2025 data-distribution and regions guide with its
object/interaction companions. Check whether update-region association,
subscription-region, and interaction-region decisions are clearly distinct
recipient-eligibility paths and remain separate from logical-time delivery.
Use bounded source/test evidence; do not infer DDM overlap rules for directed
interactions. Revise an existing flow or add one only for a concrete reader
gap. Keep 2010 separate; no builds, tests, CI, or Requirements Lab edits.

### 2025 DDM recipient-path survey — 2026-10-12

Compared the DDM, object/interaction, relevance-advisory, and
directed-interaction guides with the pinned 2025 API, selected registry paths,
and focused test sources. Existing flows distinguish per-attribute object
source-region association from receiver subscription, ordinary interaction
explicit/default-region selection from object routing, and DDM eligibility
from logical-time/callback gates. The focused ordinary interaction case
supports the existing effective-regional-subscription behavior and empty
regional-send no-op; tests were inspected, not run.

The source also confirms a separate publisher-side rule: an active regional
declaration can establish class-level relevance without an object instance or
overlap result. That advisory does not imply delivery. Directed sends identify
a target object and use a receiver selector; the API shape and guide keep them
separate from region-set routing. The source's regional declaration check is
not evidence for a directed-delivery overlap rule, which remains unclaimed.
Existing guides suffice, so no chart or diagram count changed (261). No build,
test rerun, CI, 2010, or Requirements Lab work.

Next handoff: survey the 2025 Federation and Federate Lifecycle guide alongside
the updated Disconnect/Detach flow. Compare Create, Join, Resign, Destroy,
connection loss, final-member behavior, and member-scoped automatic-resign
outcomes. Keep member-visible HLA state distinct from Umbra teardown evidence;
revise a flow only for a concrete gap. Keep 2010 separate and avoid builds,
tests, CI, and Requirements Lab edits.

### 2025 federation lifecycle versus connection teardown — 2026-10-12

Audited the current 2025 lifecycle and connection guides together. They already
separate ambassador connection, federate membership, and execution lifetime;
voluntary Resign, unjoined-only Disconnect, and RTI-forced connection-loss
cleanup are not conflated. Selected embedded tests show the final-member
cleanup path on both resignation and transport loss, while the federation
execution remains available until explicit Destroy. The member's automatic
directive is distinct from the forced-loss safety overlay. The revised
connection chart separately treats process-client close and conditional
server-side Detach; no API-caused Detach or process/embedded parity is claimed.
Save/restore, synchronization, and TSO consequences remain in their dedicated
guides. The source and focused test bodies were inspected, not run. No content
or diagram edit was warranted; no build, tests, CI, 2010, or Requirements Lab
work. Existing diagram count remains 261.

### 2025 DDM and saved Region state — 2026-10-12

Reviewed the DDM/regions guide with the object/interaction and Save/Restore
companions. Existing maps already separate object-attribute update-region
associations, regional receiver subscriptions, ordinary interaction source
regions, directed target selection, regional provider solicitation, and time
eligibility. No general routing diagram was added. A cross-service gap did
remain: the DDM guide explained the live committed Region object, and
Save/Restore already showed a queued TSO message's source-region metadata, but
there was no direct Region-object restore trace.

Added a focused 2025 embedded `HLA_IMMEDIATE` sequence to Save/Restore and a
reciprocal DDM link. The public API case deletes a committed unassociated
Region after Save, restores, and queries the same handle, dimension set, and
committed bounds. Source shows the image also stores owner, dimensions, pending
and committed bounds, and flags; the restore helper requires a joined owner
and validates the specification against the restored catalog. Pending edits,
active associations, process endpoint, host restart, and 2010 remain outside
the test claim. The same image carries a separate unachieved synchronization
point; that state remains in its own guide. The new Mermaid sequence has no
fresh syntax or visual verdict, and GitHub rendering remains unverified. No
C++ source, builds, tests, CI, or Requirements Lab changes.

Next handoff: inspect bounded 2025 process-endpoint and fresh-registry restore
tests for direct evidence about standalone Region handles/ranges and active
update-region associations. Compare with the embedded case and queued TSO
source-region snapshots; if process coverage is absent, keep it explicitly
unverified instead of claiming parity. Keep 2010 separate; no builds, tests,
CI, or Requirements Lab edits.

### 2025 Region association and queued attribute TSO restore — 2026-10-12

The bounded follow-up found two separate fresh-embedded-registry cases. The
existing one-member `HLA_IMMEDIATE` test directly queries a committed,
unassociated Region after deleting it and restoring. A second public case runs
`HLA_EVOKED` and `HLA_IMMEDIATE`: it associates an object attribute with a
source Region, queues a time-9 update to two receivers, saves at time 7,
mutates the old live bounds, destroys the original execution, then restores
using a new `EmbeddedFederationRegistry` backed by the same file save store.
The test checks one object, three Regions, one queued TSO attribute message,
two recipient queue entries, and one source-region snapshot in the image. It
then verifies restored `(2,4)` bounds, changes the Region to `(0,2)`, and
observes one time-9 reflection per receiver carrying the original Region
handle. Cleanup calls Unassociate Regions for Updates with the original
object/attribute/Region pair. The test does not directly assert the serialized
association field or send a new update through it; source capture and restore
code independently show the per-attribute association is preserved.

This fresh registry is inside the same test process. The inspected configured
process-endpoint restore-status case asserts restore callbacks/status and
operation order but has no Region setup or assertions; process-endpoint Region
state persistence, host-process restart, and 2010 behavior remain unverified.
Added an association-plus-queued-TSO sequence to the 2025 Save/Restore guide
and a reciprocal DDM link. Both new Save/Restore sequences lack fresh Mermaid
syntax/render review and GitHub rendering remains unverified. No C++ source,
build, test, CI, or Requirements Lab work.

Inventory is 263 diagrams across 34 files: 151 flowcharts, 89 sequences, and
23 state diagrams (14 separate 2010, 234 2025 topic, 13 cross-cutting, and 2
index diagrams).

2026-10-12: Completed the bounded 2025 object/interaction survey against the
discovery, subscription-state, DDM, DSE, time-management, and TSO-retraction
guides. Existing material already keeps unknown-object discovery and its
initial reflection distinct from an already-known object's subscription
change, a previously issued update with deferred eligibility, regional
overlap, timestamp-order eligibility, and application callback observation.
The embedded source invokes its ordinary initial-value planner after
discovery; the process subscription handler replans discovery and the process
discovery enqueue path builds initial values only for RTI-owned MOM objects.
The focused embedded late-discovery case checks the selected stored value;
current focused tests do not isolate a value-only callback for a newly added
attribute on an already-known ordinary object. The pinned headers expose API
signatures, not the normative prose. IEEE's public listing routes standard
access through subscription. No distinct diagram gap was found, so no chart
was duplicated or changed; 2010 remains separate.

Next handoff: resolve the already-known-object/new-attribute current-value
question and the related discovery/reflection ordering only from an authorized
copy of IEEE 1516.1-2025 clauses 5.8, 6.9, and 6.11. Reconcile the clauses
with the current embedded/process evidence without elevating source comments or
tests into normative proof. If licensed text is unavailable, retain the open
boundary and continue with another 2025-only backlog topic; do not infer from
2010. No build, test, CI, or Requirements Lab work.

## 2025 process Save/Restore call and callback boundaries — 2026-10-12

Audited the process Save/Restore handlers, public adapter branches, client
response/event drain, and selected configured-endpoint tests. The HLA_EVOKED
untimed Save case uses polling and observes initiation/saved callbacks only
after explicit Evoke calls; the HLA_IMMEDIATE Save-failure case uses pushed
events and observes initiation/not-saved callbacks by the corresponding public
call returns. Those are separate tested pairings, not a rule tying one
transport mode to one callback model.

Added a bounded process Restore sequence to the 2025 Save/Restore guide. It
separates the process control response from HLA request/federation callbacks,
shows early rejection versus begun success/terminal failure, and places RTI
image rehydration at the completion barrier. The Restore evidence is limited
to one-member pushed-event HLA_IMMEDIATE tests over a loopback endpoint served
by a thread. It does not establish HLA_EVOKED Restore timing, multi-member
call boundaries, OS-process restart, embedded/process parity, or 2010 behavior.

The diagram inventory is 267 across 34 design files: 154 flowcharts, 90
sequences, and 23 state diagrams (14 separate 2010; 238 2025 topic; 13
cross-cutting; 2 index). No C++ source, tests, build, CI, or Requirements Lab
work occurred. `mmdc` was unavailable and the authorized Scoop catalog search
found no Mermaid CLI package, so the new sequence has no fresh syntax/render
or GitHub verdict.

Next handoff: validate the new sequence with a GitHub-compatible Mermaid
renderer, then inspect the configured process endpoint's HLA_EVOKED Restore
polling path and focused public-test coverage. Do not infer Restore behavior
from the HLA_EVOKED Save tests or claim 2010/embedded parity.

## 2025 process Restore polling and HLA_EVOKED callback boundary — 2026-10-12

The continuation audit found direct HLA_EVOKED process Restore lifecycle tests
that were missed by the prior narrow test inventory. This corrects the previous
handoff's statement that HLA_EVOKED Restore coverage was absent. The configured
endpoint tests use polling (`ProcessFederationServiceOptions{false}`) with
`HLA_EVOKED`: the request and completion methods return while the callback
counters are still unchanged, then explicit `evokeCallback` calls poll and
deliver Restore success/begun/initiate and terminal outcome callbacks. Separate
cases cover early request rejection, terminal participant failure, abort, and
two-member status/callback projection. Another test configures pushed event
frames together with `HLA_EVOKED`, demonstrating that pushed receipt can still
leave user callbacks pending until Evoke. This is not a complete transport by
callback-model matrix.

Added a dedicated 2025 polling sequence beside the existing pushed-event
`HLA_IMMEDIATE` sequence in the Save/Restore guide. Both are public configured
endpoint traces hosted by a test-server thread over loopback TCP; neither is an
OS-process restart demonstration. Process Restore is not generalized to
embedded behavior or 2010. The existing immediate chart remains explicitly
one-member; separate two-member evidence is cited without broadening that
chart's timing claim.

The working-tree inventory is 268 diagrams in 34 design files: 154 flowcharts,
91 sequences, and 23 state diagrams (14 separate 2010; 239 2025 topics; 13
cross-cutting; 2 index). The new polling sequence and the prior immediate
sequence still lack a fresh parser/render and GitHub verdict; `mmdc` is absent
and the authorized Scoop search found no package. No C++ source, tests, build,
CI, or Requirements Lab work.

Next handoff: validate both process Restore call-boundary sequences with a
GitHub-compatible Mermaid renderer. Then resume prioritized backlog item 5 by
auditing process-endpoint Region Save/Restore coverage and its public-test
limits; do not infer it from embedded Region-image tests. Keep 2010 separate
and skip builds, test reruns, CI, and Requirements Lab work.

## 2025 process Region image path and test boundary — 2026-10-12

A bounded source audit found that the configured 2025 process client and
service delegate Save/Restore control operations to the shared
`EmbeddedFederationRegistry`. Save completion captures Region handles, owners,
dimensions, pending and committed bounds, and state flags into the versioned
typed image before Save success is exposed. Restore decodes that section and,
at final member completion, rebuilds a candidate Region map against the
restored dimension catalog and live joined-member set. The decoder preserves
the distinction between an older image with no typed Region section and a
current typed section containing zero Regions. Regional subscriptions, object
update-region associations, and queued-message source-region snapshots are
separate image sections.

The inspected configured-process public tests cover Save/Restore lifecycle,
status, and callback transport but do not create or query Regions. The
file-backed federation-registry Region test creates a second registry object
inside the same test process and verifies handle, dimensions, and committed
bounds; it does not exercise the public process endpoint or an OS process
restart. Existing embedded API Region examples remain separate evidence.

Added a source-derived process adapter-to-image sequence to the 2025
Save/Restore guide and linked it from the DDM/Regions guide. It is explicitly
not an HLA normative flow or process-endpoint Region test. The Mermaid syntax
and render verdict remain pending: `mmdc` is unavailable, Scoop has no
matching package, and repository-derived diagram text was not sent to a
hosted renderer. No C++ source, tests, build, CI, or Requirements Lab work.

The working-tree inventory is 269 diagrams across 34 files: 154 flowcharts,
92 sequences, and 23 states (14 separate 2010; 240 2025 topics; 13
cross-cutting; 2 index).

Next handoff: when a compatible local renderer is available, validate the
two process Restore callback-boundary sequences and the new Region image
sequence. Then select another 2025-only reader gap. Keep 2010 separate and
avoid builds, test reruns, CI, and Requirements Lab work.

## 2025 joined-federate MOM Save/Restore profile boundary — 2026-10-12

Compared the `HLAfederateState` state projection in the joined-federate MOM
guide with its focused test and the configured 2025 process-endpoint tests.
The embedded case directly observes the conditional `3 → 1` Save and `5 → 1`
Restore reflections. A bounded search of `ieee1516_2025_connection_*.cpp` and
`.hpp` test sources found no direct query/assertion of `HLAfederateState`.
Although the process Restore API/callback path is separately documented, its
shared registry implementation is not endpoint test evidence. Added this
explicit limitation and labeled the state test embedded-only; no diagram or
count changed. No C++ source, tests, build, CI, 2010, or Requirements Lab work.

The local preview remains unavailable: the UI exposes no targetable desktop
application, and existing hosted Mermaid tabs were left untouched. Keep the
two process Restore callback-boundary diagrams, Region image diagram, and
revised Connect finalization flow on the unrendered queue.

## 2025 Connect finalization ordering — 2026-10-12

A bounded source comparison found a readability/ordering ambiguity in the
Connect validation flow: it showed the `not_connected → not_joined` edge and
callback/resource installation as separate-looking steps, while the prose
said the transition was the final commit. The source applies the lifecycle
edge and then installs the prepared callback/session and connection fields in
the same final locked section. Clarified the text and collapsed those details
into one diagram node; this is Umbra source order, not a distinct HLA-visible
protocol event. Focused tests exercise the callback-model and AlreadyConnected
conditions separately and do not assert combined-invalid precedence or this
internal commit order. The revised Mermaid remains unrendered; no diagram
count changed. No C++ source, tests, build, CI, 2010, or Requirements Lab work.

Next handoff: survey the HLA-first onboarding path across the 2025 connection,
federation lifecycle, FOM admission, name-reservation, and object-visibility
guides: from ambassador construction and Connect through Create/Join to an
eligible observer's discovery. Determine whether the existing guide chain
already teaches the portable transitions or whether a small orientation flow
would close a real navigation gap. Keep 2010 and process/embedded evidence
separate; add no umbrella chart unless the bounded comparison establishes a
reader need. Keep the four pending Mermaid diagrams on the local-render queue;
continue docs work without builds, test reruns, CI, or Requirements Lab edits.

## 2025 first-object reading path and additional-FOM observer boundary — 2026-10-12

The HLA-first reading-path survey found the existing focused guides already
cover Connect/configuration, authorization, distinct Create and Join
transitions, FOM admission, object registration, and receiver-specific
discovery. The guide index now labels that chain as a reading path rather than
a required call sequence and identifies name reservation as optional for
named registration; unnamed registration has no reservation step. No umbrella
diagram was needed.

A bounded additional-FOM Join audit found that the shared registry commit
reconciles and installs the replacement definition and handle directories
with the new membership, but does not itself call ordinary object-discovery
planning. The embedded and configured-process subscription paths do explicitly
replan discovery after subscription changes. Existing embedded class and
interaction tests verify catalog lookup and prior-handle equality; the process
case verifies the joining client's lookup. None tests a pre-existing
subscriber's callback for an already-registered object after model replacement.
Added this source/test boundary and open observer question to the FOM guide;
it is not an HLA requirement claim. No diagram or count changed, and the
existing local-render queue is unchanged. No C++ source, tests, build, CI,
2010, or Requirements Lab work.

Next handoff: audit which compatible additional-FOM compositions can extend an
existing object-class hierarchy or member set, then trace how the current
discovery resolver and existing subscription records interpret those cases.
Use existing focused tests only; keep standard, Umbra-source, and test claims
separate. Add guide detail only for demonstrated reader value, preserve the
2010/2025 boundary, and skip builds, test reruns, CI, and Requirements Lab work.

## 2025 additional-FOM extension shapes and observer-state boundary — 2026-10-12

The bounded composition survey found that the successful additional-module
lookup fixture adds a child class beneath the existing HLAobjectRoot; the
already-joined member resolves it while the earlier Employee-class handle
remains equal. That test does not extend existing attributes or assert
callbacks. The official Restaurant extension adds an attribute under the
existing Food.Drink.Soda class plus interaction parameters, but this exact
additional-module set is rejected at FDD materialization, so it is not a
successful Join example. The composer has recursive identity-based merging,
but successful materialization remains a separate gate.

The registry reconciles class/attribute handles with the replacement catalog
without rewriting existing explicit per-class subscriptions or per-instance
attribute ownership. A later declaration-triggered discovery plan uses the
current catalog plus those stored states. The Join commit itself does not
create subscriptions, instance ownership, or an ordinary discovery plan. The
FOM guide now distinguishes the tested additive-class case, the rejected
existing-member example, and the source-only runtime consequences. No new
diagram or count change; no standard guarantee or 2010 parity is inferred. No
C++ source, tests, build, CI, or Requirements Lab work.

Next handoff: audit the distinct 2025 interaction-class counterpart: how an
additional module that introduces interaction classes or parameters relates
to existing per-class subscriptions and send-time hierarchy resolution, and
what existing tests show about receives before versus after Join. Keep
interaction routing distinct from object discovery; use bounded source/test
evidence, make no 2010 or normative claim without exact support, and skip
builds, test reruns, CI, and Requirements Lab work.

## 2025 additional-FOM interaction subscription and later-send boundary — 2026-10-12

The embedded interaction-class lookup case adds a child class under
HLAinteractionRoot; an already-joined member resolves it while the earlier
interaction handle remains equal. The parameter lookup case adds a base/child
interaction chain and resolves the base-defined Identifier through the child.
Neither case subscribes, publishes, sends, or receives. A separate embedded
receipt case loads the full hierarchy at Create and checks that a parent
subscription receives a child send under the closest subscribed class; it is
not an additional-Join scenario.

The shared registry stores explicit per-federate interaction-class
subscriptions and the ordinary send planner walks the current catalog's
parent chain at send time. Thus source indicates a pre-existing ancestor
subscription can be considered for a future send after the catalog grows, but
no focused test covers ancestor subscription + additional Join + descendant
send. Join does not synthesize a Receive Interaction for the newly added class.
The configured-process additional-FOM case checks object-class lookup only;
it is not process interaction-delivery evidence. Added this bounded distinction
to the FOM guide, cross-linked the separate object-discovery and directed
interaction paths, and did not add another chart. No normative or 2010 claim,
diagram-count change, C++ source/test/build/CI, or Requirements Lab work.

Next handoff: audit the 2025 configured-process ordinary interaction route for
an existing ancestor-subscription/descendant-send test. Trace subscription,
send-frame creation, polling, and callback delivery, and identify which edges
are source-only versus asserted under HLA_EVOKED and HLA_IMMEDIATE. Do not infer
the additional-FOM Join scenario or embedded/process parity from shared
registry code alone. Keep 2010 and Requirements Lab separate; skip builds,
test reruns, and CI.

## 2025 configured-process ordinary interaction delivery and callback-model boundary — 2026-10-12

The configured-process interaction declaration case exercises publish,
active/passive subscribe, unsubscribe, and unpublish state transitions, but it
does not send. The public Send Interaction case sends an exact-class receive-
order event and uses a raw process receiver to inspect the parameter handle,
value, and tag; it does not assert a FederateAmbassador callback. A separate
configured-process multi-recipient test does assert two `receiveInteraction`
callbacks per exact-class receiver, FIFO tag order, and the selected class,
producer, valid transportation, parameter count, and region-presence fields
under both HLA_EVOKED and HLA_IMMEDIATE.

The implementation trace now distinguishes stored active subscriptions,
shared recipient planning, process event materialization, push-frame versus
pull-queue delivery, and callback-bridge submission. In the focused test,
HLA_EVOKED uses the default polling service and explicit evoke calls;
HLA_IMMEDIATE enables the private push option and uses a later receiver-side
class lookup as an exchange fence. This is the fixture's transport selection,
not a normative coupling of callback model to transport, and the test does not
establish ordering against the sender's public call return. Its exact-class
subscriptions do not test ancestor-subscription promotion, and it is not an
additional-FOM Join test. Updated the existing process interaction sequence
and its test/source map; no new chart or 2010 inference. Mermaid syntax/render
review remains pending. Test sources were inspected, not run; no build, CI, or
Requirements Lab work.

Next handoff: inspect the configured-process additional-module Join path only
as it relates to the ordinary interaction question: whether the server accepts
the added module, how the shared current catalog reaches later send planning,
and whether any focused process test combines that Join with a prior ancestor
subscription and descendant send. Keep source behavior distinct from the
existing embedded Create-time hierarchy case; make no parity, normative, or
2010 claim, and skip builds, test reruns, CI, and Requirements Lab work.

## 2025 process additional-FOM Join and interaction-route boundary — 2026-10-12

The process server accepts a Join carrying FOM modules only when its optional
`additionalFomPreparation` callback is configured; otherwise the service
rejects the request. The configured-process test and process service probe
install the 2025 FederationManagementCoordinator callback. The service
serializes preparation with registry commit, and the registry reconciles the
existing interaction-class handle directory before swapping the replacement
definition/directories with the new membership. Stored per-federate
interaction declarations are a separate map and are not rewritten by this
commit. A later process Send Interaction reaches the shared planner, whose
current-catalog resolver walks the sent class's parent chain against those
subscriptions. That is a source-only inference that a retained ancestor
subscription can be considered for an added descendant; no process delivery
test exercises it.

The configured-process additional-FOM test adds an object-only class and
checks object-class lookup/name resolution. It has no interaction-class or
parameter addition, subscription, Send Interaction, or Receive Interaction
callback. The broad process probe installs the same preparation callback but
does not supply missing receive-coverage evidence. Expanded the FOM guide's
existing additional-module flow with this profile-specific source/test
boundary and linked it from the object/interaction guide; the existing
replacement/interaction ancestry diagrams already carry the necessary concepts,
so no chart was added. No normative, 2010, or process/embedded parity claim.
Sources/tests were inspected, not run; no build, CI, or Requirements Lab work.

Next handoff: audit the 2010-only interaction section of the separate
Reference RTI guide against the pinned 2010 interaction API, its reference
runtime route, and focused 2010 test sources. Check whether the guide's current
send/relevance/receive explanation has a genuine reader gap; if so, update
only the 2010 guide and keep every 2025 routing claim separate. Do not build,
rerun tests, start CI, or edit the Requirements Lab. Preserve the existing
local Mermaid render queue; use no hosted renderer for repository text.

## 2010 ordinary interaction send validation and handle boundary — 2026-10-12

Audited the 2010 interaction flow against the pinned `sendInteraction` API,
`ReferenceRuntime2010`, and the focused binding-shell smoke. The diagram had
collapsed three distinct send checks into a single generic exception branch.
It now separates an unknown runtime class handle
(`InteractionClassNotDefined`), missing sender publication
(`InteractionClassNotPublished`), and a supplied nonmember parameter handle
(`InteractionParameterNotDefined`) before recipient selection. The source
performs these checks in that order, then calls `receiveInteraction` inline
for other members subscribed to the exact handle. Callback failure remains a
separate `FederateInternalError` to `RTIinternalError` partial-fanout path.

The reference profile lazily creates interaction-class and parameter
directory entries during name lookup and does not load the FOM path passed to
creation. The smoke uses `ignored-fom.xml`, performs lookup/publication/
subscription/send, and verifies one receiving callback; it does not establish
FOM admission, multi-recipient fan-out, or callback-failure outcomes. The
guide now cross-links this distinction and keeps the three API-declared
send-specific errors separate from the complete service-entry exception
surface. This was a 2010-only documentation change with no 2025 routing
inference. The revised Mermaid chart has no fresh local syntax/render verdict;
counts are unchanged. Sources and tests were inspected, not run; no build, CI,
or Requirements Lab work.

Next handoff: audit the 2025 data-element encoding guide against its pinned
encoding API, scalar/composite/array codec paths, and existing tests. Clarify
caller-selected versus type-fixed byte order, string/text contracts, and the
boundary between HLA value encoding and Umbra process-frame serialization.
Keep the 2010 encoding guide separate and do not infer edition parity. Skip
builds, test reruns, CI, and Requirements Lab work; preserve the local Mermaid
render queue.

## 2025 string encoding units and C++ value contract — 2026-10-12

The 2025 encoding guide already separates caller-selected BE/LE types, fixed
type formats, composite child encodings, factory-defined logical time, and
private process framing. The audit found a narrower reader gap: it named the
ASCII and UTF-16 string formats but did not explain what the public C++ string
types mean at the codec boundary. Added an implementation-bounded comparison
and flowchart to the [2025 encoding guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md).

The pinned API pairs `HLAASCIIstring` with `std::string` and
`HLAunicodeString` with `std::wstring`. Umbra validates every ASCII byte as
0x00–0x7F (preserving embedded NULs in the length-delimited payload); Unicode
input is normalized to UTF-16 units, with surrogate/scalar validation, and
the prefix counts units rather than bytes or Unicode scalar values. The
focused Catch2 examples verify an embedded-NUL ASCII value, rejection of
high-bit bytes, and a supplementary Unicode character encoded as three
UTF-16 units after `A`; malformed UTF-16 and an unmatched wide surrogate are
also covered. These are selected implementation tests, not exhaustive Unicode
conformance evidence. Value payload length remains distinct from the private
process-frame header.

Recounted all current Markdown Mermaid fences (both backtick and tilde): 270
diagrams across 34 files, comprising 155 flowcharts, 92 sequences, and 23
state diagrams; the edition-aware totals remain separate at 14 for 2010, 241
for 2025 topic guides, 13 cross-cutting, and 2 index diagrams. The new string
flowchart has no fresh local syntax or visual render verdict. No source, test,
build, CI, Requirements Lab, or 2010 encoding change.

## 2025 process Resign and queued ownership-cancellation callback — 2026-10-14

A bounded scan of the guide portfolio confirmed Federation Synchronization
already separates barrier registration, participant selection, late join,
achievement fan-in, member departure, and save/restore; its current diagrams
and prior source/test mappings make another synchronization flow duplicative.
The lifecycle guide already links readers to the general process Resign
callback-queue caveat, but lacked a concrete ownership example. Added a
cross-link from its exit-state discussion to the push-mode Attribute Ownership
Acquisition Cancellation trace.

Rechecked the relevant 2025 process sources and focused test. The push handler
consumes the cancellation before sending the event frame. The client buffers
that frame while awaiting the service response, then submits it to the callback
bridge before returning; the `HLA_EVOKED` task has no cancellation-registry
recheck. The process Resign branch does not reset the dispatcher. The focused
process cancellation test uses default `pushReceiveOrderEvents=false` with
`HLA_EVOKED`, so it establishes pull confirmation only. The queued-push result
after deletion or Resign remains source-derived, untested behavior and is not a
portable HLA rule. The ownership guide retains the detailed flow; the lifecycle
guide now points to it without copying its state machine. No diagram changed,
so no render was required. No 2010 inference or Requirements Lab work.

Next handoff: survey the 2025 process Connection Lost/forced-resignation path
against voluntary Resign in the lifecycle and callback guides. Compare the
cleanup of server-queued events with client-buffered and callback-queued work;
use focused existing tests, preserve the HLA/implementation distinction, and
add or revise a diagram only for a genuine reader gap. Do not build, rerun tests,
start CI, edit the Requirements Lab, or infer 2010 behavior.

## 2025 process Connection Lost and independent work queues — 2026-10-14

The lifecycle guide now distinguishes the process fault fixture's forced
registry transition from the later client-side transport failure. The fixture
applies forced loss and survivor projections, detaches the session (removing
server-side queues), then closes the receiver socket. A bounded search found no
production C++ caller of the process service's `connectionLost` helper, so the
fixture sequence is not generalized into an automatic EOF policy.

Added a process-specific lifecycle flowchart for the three independent stores:
server-session event queues, client-buffered `pendingEvents_`, and tasks already
submitted to the shared callback dispatcher. The local failure handler marks
the endpoint Not Connected and submits `FederateAmbassador::connectionLost`;
it does not commit server registry loss or reset the shared dispatcher. The
failed-request path dispatches transport failure rather than pending pushed
events, and client close clears its buffer. Focused process TCK evidence covers
selected survivor effects and the local callback under both callback models,
but does not assert post-loss delivery of buffered events or unrelated queued
callbacks. This is an implementation-profile explanation, not an HLA queue
guarantee or a 2010 inference.

Rendered the added Mermaid chart with Mermaid CLI 12.0.0 and visually checked
the result, including explicit yes/no outcomes for both queue checks. The
Markdown inventory is now 281 diagrams across 34 files: 160 flowcharts, 98
sequences, and 23 state diagrams (15 separate 2010, 250 2025 topic, 13
cross-cutting, and 3 index diagrams). No source/tests, build, test reruns, CI,
Requirements Lab, or 2010 guide changes.

Next handoff: audit whether 2025 attribute ownership transfer changes an
object's attribute update-region association. Compare the bounded ownership
service path, object-region state, DDM guide, and focused tests; identify what
transfers and what remains attached to the object, and mark any source-only or
untested edge. Update a diagram only for a genuine reader gap. Keep 2010
separate; do not infer cross-profile parity. No builds, test reruns, CI, or
Requirements Lab work.

## 2025 ownership/update-region state and process-route boundary — 2026-10-14

The current Ownership and DDM guides already cover the ownership-to-region
association boundary, so no duplicate flow was needed. Rechecked the selected
embedded registry transition and three focused HLA_EVOKED test bodies: an
owner-to-owner transfer clears the former owner's active association; a
prospective owner's deferred association becomes active only if it remains at
transfer; and unassociating before transfer prevents promotion, leaving the
acquirer's first update on the default source realization. The former owner
must associate again after reacquisition. These are embedded test-source
observations, not a complete normative rule; the checkout lacks the full
clause prose for independent verification.

A bounded 2025 process-route inventory found a distinct adapter boundary:
`attributeOwnershipDivestitureIfWanted` is declared by the pinned API, but the
Umbra public method directly consults and mutates `embeddedFederationRegistry()`.
The process transport/client/service inventory has no matching operation or
handler, unlike the explicit process Confirm Divestiture and Unconditional
Divestiture routes. Added this limitation to the ownership guide as an Umbra
implementation finding, not an HLA exception. No process update-region test
was inferred from ownership-only process cases.

No diagram changed or needed rendering. Inspected sources and test bodies; did
not run tests or builds, start CI, edit the Requirements Lab, or change the
2010 stream.

Next handoff: map the 2025 process ownership-service routes for Acquisition If
Available, Confirm Divestiture, Unconditional Divestiture, and cancellation.
Trace public adapter, transport, registry plan, event delivery, and focused
process TCK assertions. Separate a missing Umbra route from normative HLA
requirements; label shared registry code as source evidence rather than
integration-test parity. Update the ownership guide only for a genuine reader
gap. Keep the 2010 stream separate; no builds, test reruns, CI, or Requirements
Lab work.

## 2025 process ownership routes and unconditional-divestiture result boundary — 2026-10-14

Completed the bounded process-route comparison for Acquisition If Available,
Confirm Divestiture, Unconditional Divestiture, and acquisition cancellation.
The ownership guide now has a route/evidence table that keeps private service
results distinct from recipient notifications and states exactly what the
selected process test sources do and do not exercise. The pinned API declares
Divestiture If Wanted, but the process protocol/client/service route is absent;
the public implementation uses the embedded registry. This remains an Umbra
profile gap, not an HLA rule.

Added one 2025 process Unconditional Divestiture flowchart. It distinguishes
the private accepted Boolean from recipient acquisition work and assumption
offers, push versus pull event transport, and the callback bridge's distinct
public projections. Source inspection shows registry state is committed
before recipient enqueue/send work; a later failure returns an internal
error and the inspected path has no rollback. No focused process test covers
that edge. The focused Catch2 case is embedded; a portable TCK scenario in
source does not establish a process-profile execution. Selected process
acquisition/cancellation tests cover pull-mode cases, while the Confirm
Divestiture case uses HLA_IMMEDIATE and is bounded to its selected callback
assertions. No HLA timing or callback-before-return rule is inferred from
these implementation paths.

Mermaid CLI 12.0.0 rendered the chart, and its layout was visually reviewed.
The Markdown recount is 282 diagrams in 34 files: 161 flowcharts, 98
sequences, and 23 state diagrams, comprising 15 separate 2010, 251 2025, 13
cross-cutting, and 3 index diagrams. The full guide and tree were not rendered.
No build, test rerun, CI, Requirements Lab, or 2010 change.

Next handoff: review whether the 2025 ownership guide needs one HLA-first
comparison of negotiated Confirm Divestiture, Unconditional Divestiture,
Divestiture If Wanted, and Acquisition If Available—especially which paths
leave attributes unowned, how later acquisition/assumption is represented,
and which outcome is a return value versus callback. Reuse existing diagrams
if they already teach this clearly; create a compact decision/transition
chart only for a real reader gap. Verify normative statements against pinned
2025 clauses and keep implementation/test evidence separately labeled. Keep
2010 separate; no builds, test reruns, CI, or Requirements Lab work.

## 2025 HLA-first ownership service-selection map — 2026-10-14

The four detailed ownership paths were already individually documented, but
the guide did not give readers one place to see who initiates each path and
how its outcome is represented. Added a compact selector for requester-led
Acquisition If Available, owner-led Unconditional Divestiture and Divestiture
If Wanted, and the negotiated owner-confirmation path. The map labels the
If-Wanted output set separately from acquisition notifications and makes the
selected If-Available callback-entry observation explicit without asserting a
universal callback order. Detailed per-service diagrams remain separate.

Checked the path distinctions against pinned 2025 RTIambassador signatures
(§§7.2, 7.3, 7.6, 7.9, and 7.13), callback declarations, and the four linked
focused test sources. The process Confirm Divestiture test does not establish
ownership-query timing; embedded tests are not one combined conformance
matrix. The checkout lacks complete clause prose. Mermaid CLI 12.0.0 rendered
the new chart, and the layout was visually reviewed. No tests/build/CI,
Requirements Lab, or 2010 work.

Next handoff: audit the open 2025 §6.21 Request Attribute Value Update
entitlement question. Compare the full normative clause with the current
guide's separation of provider solicitation from receiver eligibility and
the focused embedded/process test scenarios, especially whether a requester
without ordinary receiver eligibility is entitled to a provider's later
update. The official [IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers access by purchase or subscription; do not infer the missing rule from
API declarations or implementation. If no authorized clause text is
available, preserve the open question and move to another evidence-resolvable
HLA reader gap. Keep 2010 separate; no builds, tests, CI, or Requirements Lab.

### 2025 regional Request Attribute Value Update and later receiver DDM — 2026-10-14

The full authorized 1516.1-2025 §6.21 clause text is still not present in the
checkout. The official [IEEE listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers access by purchase or subscription, so the question of direct-response
entitlement for a requester without ordinary receiver eligibility remains
open; no normative conclusion was inferred from API or implementation.

Expanded the existing fifth 2025-only regional request-versus-response flow
to show the selected embedded rules more clearly: an empty request-region set
does not solicit a provider; an explicit provider update-region association
must overlap a committed request region; a default-region attribute remains
eligible; and queued work is rechecked at callback delivery so a stale
explicit-region request can be suppressed after a committed range change.
The later update enters separate ordinary per-receiver planning and does not
inherit request-region state. Source review covered the embedded request plan
([request planner](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L999),
[regional filter/default-region rule](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L579)),
the callback-entry projection
([recheck](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L1751)),
and the later receiver DDM evaluation
([ordinary update recipients](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L221)).
The focused [regional-request test](../../cpp/tests/ieee1516_2025_regional_request_attribute_value_update_filtering_catch2.cpp#L178)
checks the empty-region no-op, disjoint explicit versus default-region
eligibility, overlap after recommit, and stale callback suppression
([scenario details](../../cpp/tests/ieee1516_2025_regional_request_attribute_value_update_filtering_catch2.cpp#L100));
it does not send a subsequent value update, prove process parity, or resolve
the unsubscribed-requester entitlement question. Tests were inspected, not run.

Mermaid CLI 12.0.0 rendered the revised chart; local visual review found its
tall layout readable, with the provider-solicitation and later receiver paths
distinct and no crossing branches. The whole-tree inventory remains 283
diagrams (162 flowcharts, 98 sequences, 23 state diagrams), including the
separate 2010 stream; no 2010 material changed. No build, tests, CI, or
Requirements Lab work.

Next handoff: visually review the existing 2025 DDM region-template deletion
gate in `HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md` against its
focused embedded region-lifecycle cases and registry guard. Keep its
RegionInUseForUpdateOrSubscription, deferred-association release, and stale
InvalidRegion outcomes bounded to that 2025 embedded evidence; do not infer
process or 2010 parity. Preserve the open §6.21 entitlement question unless
authorized clause text becomes available. No build, test, CI, or Requirements
Lab work.

### 2025 DDM Region deletion gate render review — 2026-10-14

Rendered the existing Delete Region gate in the 2025 DDM and Regions guide
with Mermaid CLI 12.0.0. Screenshot review found its left-to-right layout
readable: the in-use rejection, dependency-release loop, owned-attribute
unpublish path, successful deletion, and stale-handle result are distinct.
The source spot-check confirmed the embedded registry checks the owned live
region's in-use state before erasing it
([delete guard](../../cpp/src/internal/federation/federation_registry_regions.cpp#L339)).
Focused embedded 2025 test sources cover a passive regional interaction
subscription blocking deletion, an object update-region association released
by unpublishing, and deferred association release after the final object
removal callback; the same test verifies InvalidRegion after deletion
([region lifecycle tests](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L134),
[object association](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L221),
[deferred removal](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L369)).
These sources were inspected, not run. The chart remains an embedded-profile
explanation, not normative text or evidence of process/2010 parity.

No diagram count changed; the whole-tree inventory remains 283 (162
flowcharts, 98 sequences, 23 state diagrams), with 2010 and 2025 streams
separate. No build, test, CI, or Requirements Lab work.

Next handoff: review the existing 2025 interaction-region dimension-context
flow in `HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md` against the two
focused mixed-region interaction cases. Preserve their separate send versus
subscription scenarios and the specific available-dimension checks; do not
imply one combined transaction, universal service rollback, or process/2010
parity. The §6.21 entitlement question remains open pending authorized clause
text. No build, test, CI, or Requirements Lab work.

### 2025 interaction-region dimension-context review — 2026-10-14

Rendered and visually reviewed the existing interaction-region
dimension-context chart in the 2025 DDM and Regions guide with Mermaid CLI
12.0.0. The two vertical branches are legible and clearly separated: one
tests Send Interaction With Regions, the other tests Subscribe Interaction
Class With Regions. The guide explicitly says they are separate embedded
cases, not one combined service sequence.

Rechecked both focused `HLA_EVOKED` test scenarios and their implementation
boundaries. The send case commits one region using a class-available
dimension and one using a valid but unavailable dimension; the mixed send
throws `InvalidRegionContext` and produces no observed receiver callback,
then the valid-only send is observed. The subscription case rejects the
unavailable-dimension region; a later send produces no callback, and a valid
regional subscription then permits a matching callback. The embedded send
planner validates each region against class-available dimensions before
capturing region snapshots
([planner check](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L82),
[send failure mapping](../../cpp/src/internal/runtime/umbra_rti_ambassador_regional_interaction_send_services.cpp#L709)).
The regional subscription validates before mutating its declaration map
([registry validation](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L593)).
Focused evidence is in the separate
[mixed-region send](../../cpp/tests/mixed_region_interaction_validation_catch2.cpp#L120)
and [regional subscription](../../cpp/tests/mixed_region_interaction_validation_catch2.cpp#L229)
cases. These tests do not establish general transaction or rollback rules;
source/test files were inspected, not run. No process or 2010 parity is
claimed.

No diagram count changed: the inventory remains 283 (162 flowcharts, 98
sequences, 23 state diagrams), and the separate 2010 stream is unchanged. No
build, tests, CI, or Requirements Lab work.

Next handoff: visually inspect the existing 2025 FOM-dimension-to-runtime-
region bridge in the same DDM guide. Preserve the three distinct sets already
identified there—the execution dimension catalogue, dimensions available to
a class, and dimensions selected for a Region—and verify its query map against
the pinned `RTIambassador` declarations. Render the chart locally and record
any source/test limits; do not turn the conceptual bridge into a required
service-call order or infer 2010 behavior. §6.21 entitlement remains open.
No build, test, CI, or Requirements Lab work.

### 2025 FOM-dimension-to-Region bridge render review — 2026-10-14

Rendered and visually reviewed the existing conceptual bridge in the 2025
DDM and Regions guide with Mermaid CLI 12.0.0. The tall layout remains
readable at preview width; the optional `getDimensionHandleSet(region)` path
is visibly separate from setting bounds, committing, and using the Region.
The chart already distinguishes FOM declarations/class associations from
the Region's fixed dimension set and explicitly avoids prescribing an
application call order.

Checked its API map against the pinned 2025 `RTIambassador` declarations:
`createRegion` §9.2 accepts a dimension set and has no class argument;
`commitRegionModifications` is §9.3; class-availability queries are
§§10.21–10.22; `getDimensionHandleSet` is §10.26; `getRangeBounds` is §10.27;
and `setRangeBounds` is §10.28. This supports the guide's distinction between
class availability and a Region's selected dimensions, not a required order
for application calls.

The focused embedded region-lifecycle test observes a Region's dimension set
immediately after creation and before commit, and reads pending bounds before
commit and committed bounds afterward
([dimension-set query](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L65),
[pending bounds](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L88),
[committed bounds](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L117)).
A separate embedded regional-object test checks the class-available dimension
query before using different valid class dimensions for source and receiver
Regions ([query assertions](../../cpp/tests/regional_object_attribute_routing_catch2.cpp#L636)).
These are bounded development-profile observations, not a test of the entire
conceptual chart or evidence of process/2010 parity. The process-endpoint
query-only case documented in the guide does not establish process DDM
behavior. Sources and tests were inspected, not run.

No diagram count changed: 283 total (162 flowcharts, 98 sequences, 23 state
diagrams), with the 2010 stream separate. No build, tests, CI, or Requirements
Lab work.

Next handoff: review the two existing queued-TSO receiver-state traces in the
2025 DDM guide (around lines 1388 and 1444). Recheck their cited source/test
boundaries, then render each current Mermaid block and record whether the
receiver-region revalidation and the interaction's send-time source-region
snapshot versus callback-time current subscription are visually and
conceptually distinct. Keep each scenario/profile bounded;
do not infer universal callback ordering or 2010 behavior. The §6.21
entitlement question remains open pending authorized clause text. No build,
test, CI, or Requirements Lab work.

### 2025 queued-TSO receiver-state trace audit and visual review — 2026-10-14

Rechecked the two existing receiver-state traces in the 2025 DDM guide against
their focused embedded tests and current source. The connection-loss attribute
case injects embedded transport loss after the publisher's time-6 grant, then
commits the surviving receiver's region from `[1,3)` to `[3,4)` before its
pending time-6 callback gate. The test observes no reflection, then the time-6
grant; on the next request it observes the independent `DELETE_OBJECTS`
removal and an unknown object name. Its separate publisher-side `connectionLost`
report is drained and checked later in the test, so the chart now says it is
omitted and does not imply a cross-ambassador callback order
([focused test](../../cpp/tests/connection_loss_regional_selector_mutation_catch2.cpp#L159),
[callback recheck](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_advance_dispatch.cpp#L349),
[loss cutoff](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L29)).

The interaction replacement test starts with overlapping source/receiver
regions, queues one timestamped send, replaces the receiver's regional
subscription before time 6, and observes suppression without a `Request
Retraction` callback. A later source-B send is delivered under subscription B
with the tested callback order `grant(6), interaction(7), grant(7)`
([focused test](../../cpp/tests/ieee1516_2025_timestamped_regional_interaction_subscription_replacement_catch2.cpp#L91)).
The embedded message stores send-time source-region snapshots; the TSO dispatch
asks the registry for a recipient projection at the callback boundary
([message shape](../../cpp/src/internal/federation/federation_registry_tso_types.hpp#L14),
[projection call](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_advance_dispatch.cpp#L260)).
Thus the receiver subscription is current state, not a frozen receiver-region
snapshot. This corrects the earlier handoff's ambiguous use of “subscription
snapshot.” It is an embedded implementation observation, not a general HLA
rule or process/2010 parity claim.

Rendered both current Mermaid blocks with Mermaid CLI 12.0.0 and visually
inspected their PNGs. A first revised render caught a sequence-label semicolon
parse error; replacing it with “and” fixed the chart, and the final render
succeeded. Shorter message labels improved legibility. The diagrams remain
scenario-specific; no new HLA callback ordering is inferred. Sources/tests
were inspected, not run. The current `docs/design` census is 283 diagrams in
34 diagram-bearing Markdown files (162 flowcharts, 98 sequences, 23 state
diagrams), counting backtick and tilde fences; six targeted 2025 DDM charts
now have local visual review. No full-guide or
whole-tree render, GitHub verification, build, test, CI, 2010 change, or
Requirements Lab work.

The conceptual DDM/time/callback-filter handoff is completed in the following
entry. The §6.21 direct-response entitlement question remains open pending
authorized clause text.

## 2025 DDM/time/callback filter map review — 2026-10-14

Rechecked the conceptual “Keep time and DDM as separate filters” map against
the cited 2025 process timestamped regional-interaction callback-gating test
and the pinned `CallbackModel` API enum. Reworked the map to branch four
independent questions from one service/receiver trace: regional scope,
RECEIVE-versus-TIMESTAMP, callback model, and callbacks-enabled state. No arrows
join those dimensions, and the prose explicitly says the diagram is not an
RTI/service execution order. Clarified that the focused process test connects
both clients with `HLA_EVOKED`; `disableCallbacks()` / `enableCallbacks()` is a
separate gate, and the observed interaction-before-grant result is limited to
that trace. The test does not compare callback models or establish embedded or
2010 parity.

Mermaid CLI 12.0.0 rendered the revised chart and its PNG was visually reviewed;
the four branches and labels remain legible. This is now the seventh targeted
2025 DDM chart with local visual review. The `docs/design` census remains 283
diagrams across 34 diagram-bearing Markdown files (162 flowcharts, 98
sequences, 23 state diagrams); whole-guide/whole-tree rendering and GitHub
verification remain pending. Sources and the existing test were inspected,
not run. No builds, tests, CI, 2010 changes, or Requirements Lab work.

The ownership/update-region handoff is audited below. The existing DDM and
Attribute Ownership guides already cover its three focused cases without
overstating their normative or profile scope, so no duplicate diagram was
needed.

## 2025 ownership/update-region boundary audit — 2026-10-14

Checked `cpp/tests/ownership_transfer_update_region_catch2.cpp` and the
corresponding DDM and Attribute Ownership guide sections. The three
`HLA_EVOKED` embedded development-profile cases separately demonstrate:

- The former owner's active association is cleared at If-Wanted transfer; the
  new owner's first update uses the default source region, explicit association
  selects its own region, and later reacquisition by the former owner does not
  restore its old association.
- A prospective owner's association can remain deferred without retargeting
  the current owner's update, then activate at transfer.
- Unassociating that deferred state before transfer prevents its promotion, so
  the new owner's update uses the default source region.

The registry path agrees: an association by a non-owner is recorded as
deferred without producing scope changes; the If-Wanted transfer path clears
the former owner's association and promotes the new owner's deferred state.
The guides already link each case, state that the sequence compares separate
tests rather than one combined scenario, and distinguish tested If-Wanted
behavior from source-only regular, Confirm, unconditional-divestiture, and
Resign paths. They also avoid stating this as a complete portable HLA rule or
process/2010 parity. No guide change was warranted; no test was rerun and no
implementation file was changed.

## 2025 object-instance name reservation chart review — 2026-10-14

Rendered all seven Mermaid blocks from
`HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md` as Markdown with
Mermaid CLI 12.0.0 and visually inspected the resulting PNGs. The owner/peer
visibility comparison and reservation-lifetime state diagram were too
compressed horizontally at preview width. Changed both to top-down layouts,
rerendered the whole guide successfully, and visually rechecked the revised
charts. No state transitions or claims were changed. The seven local renders
do not substitute for GitHub review; that remains pending publication. No
builds, tests, CI, 2010 work, or Requirements Lab changes.

## 2025 transportation pending-state chart review — 2026-10-14

Compared the chart with the focused request/confirmation and save/restore
evidence and the embedded registry/callback-dispatch paths. The service
admission records pending state; dispatch rechecks attribute ownership or
interaction membership/publication, commits the effective selection, and only
then invokes the public confirmation callback. A stale or cancelled request is
dropped without confirmation; loss of attribute ownership can also clear its
effective override, so the chart does not imply the prior value survives every
discard path. In this embedded implementation, the registry write occurs
immediately before public callback invocation; the callback remains the
federate-visible boundary, and this source ordering alone does not establish an
earlier portable or normative effective time. `HLA_EVOKED` work waits for
callback dispatch while `HLA_IMMEDIATE` work may run inline before the request
service returns. The linked focused restore cases exercise both callback
models; they are not a
complete scheduling-interleaving matrix.

Rendered all six Mermaid blocks in
`HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md` as Markdown with Mermaid CLI
12.0.0. The initial targeted render exposed a semicolon parsing problem in the
revised state labels; replacing those separators yielded a clean rerender. The
repaired pending lifecycle chart was visually inspected and is legible. This
is targeted local visual review, not GitHub verification or whole-guide/tree
coverage. No C++ files changed; no builds, tests, CI, 2010 work, or Requirements
Lab changes.

## 2025 subscription-state targeted chart review — 2026-10-14

Rendered all eight Mermaid blocks in
`HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md` with Mermaid CLI
12.0.0. Visually inspected the source-derived passive-recipient flow, the
HLA-first late-interest effects map, and the embedded regional evidence
sequence. All three are legible in the expanded local preview; the portrait
flows remain readable while scrolling, and the sequence notes/labels are not
clipped. No diagram source change was warranted.

The evidence boundary remains explicit: the passive test covers passive-only
suppression and a later active transition, not a separate active establisher
making a passive receiver eligible. The regional test records discovery and
the stored value separately from a later update but does not assert callback
order. The process initial-value comparison stays source-only for ordinary
objects, with the RTI-owned MOM path separate; the normative §5.8 question and
ordinary process observation remain open. No 2010 or profile-parity inference
was added. No builds, tests, CI, or Requirements Lab work occurred.

## 2025 embedded/process late-subscription comparison review — 2026-10-14

Visually inspected the process-versus-embedded comparison chart in the same
eight-diagram local render. Its two profile lanes are easy to distinguish; the
RTI-owned MOM initial-value event stays in its own branch, while the ordinary
process path is explicitly unresolved. No chart edit was warranted. This
remains Umbra source mapping, not a licensed §5.8 interpretation or proof of
profile parity; the ordinary process runtime observation is still open.

## 2025 service-report failure sequence visual review — 2026-10-14

Rendered all eight diagrams in
`HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md` with Mermaid CLI 12.0.0.
The switch-append failure sequence parsed but its long participant and note
labels crowded the right edge. Shortened those labels, rerendered all eight,
and visually inspected the revised chart. It remains wide but readable, with
no edge clipping; the commit-before-append, best-effort exception hook,
unasserted callback path, and skipped post-append MOM enqueue remain separate.
No semantic transition changed. The focused injected-writer test was not
rerun: its last recorded attempt failed at federation creation before the
target assertions. GitHub review remains pending publication. No builds,
tests, CI, 2010 work, or Requirements Lab changes.

## 2025 Joined-Federate MOM diagram review — 2026-10-14

Rendered all six Mermaid diagrams in
`HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md` with Mermaid CLI
12.0.0. Visually inspected the repaired subscription/discovery sequence and
resignation state chart. Both render cleanly and remain legible; the state
chart's pending-removal loop and snapshot-retention note are distinct, and the
sequence keeps Join, observer discovery/reflection, explicit value request,
and resign removal in separate steps. No chart change was needed.

The prose and evidence table preserve `HLAfederation` execution scope versus
per-member `HLAfederate` objects, and distinguish membership transition from
receiver observation. The process Join/resign cases remain separate from the
embedded sequence, and the 2010 section explicitly avoids projecting the 2025
MOM object into the separate 2010 RTI stream. This is targeted local review,
not a complete GitHub rerender. No builds, tests, CI, or Requirements Lab work.

## 2025 FOM admission/composition visual review — 2026-10-14

Rendered all four Mermaid diagrams in
`HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md` with Mermaid CLI
12.0.0. The local Create and Composition charts were too compressed in their
left-to-right layouts, so both were reoriented top-down and rerendered. Their
labels are now readable while scrolling; the Join chart remains tall but
legible, and the FOM-edition chart clearly keeps the 2010 catalog-only lane
under the 2025 RTI interface. No state gates or claims changed. The edition
chart does not imply 2010 RTI behavior or cross-stream parity. GitHub review of
local revisions remains pending publication. No builds, tests, CI, or
Requirements Lab work.

The TSO review below completes this handoff.

## 2025 TSO retraction diagram review — 2026-10-14

Rendered all ten Mermaid diagrams in `HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md`
with Mermaid CLI 12.0.0. Visually inspected five: the portable HLA outcome
orientation, producer-time/lookahead gate, regional FQR/TARA/NMRA sequence,
ownership timeline, and corrected process-route sequence. The ownership flow
was reoriented top-down and the regional sequence labels shortened. Fixed
Mermaid sequence syntax in notes and labels; no transition or evidence claim
changed. The diagrams keep the HLA-level orientation separate from selected
embedded 2025 scenarios. Do not infer normative equivalence, process parity,
or 2010 behavior. GitHub verification of later revisions remains pending
publication; the timestamped-delete and per-recipient charts retain their
2026-10-09 GitHub visual review. No builds, tests, CI, or Requirements Lab.

The reservation review below completes this handoff.

## 2025 object-instance name reservation diagram review — 2026-10-14

Freshly rendered all seven diagrams in
`HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md` with Mermaid CLI
12.0.0 and visually inspected every chart. The owner/peer map clearly keeps
reservation consumption separate from later discovery. The vertical lifetime
state chart still spanned too widely because of long edge labels, so shortened
those labels while preserving failed attempt, explicit release, resignation
before registration, named registration, global object/name purge, and
save/restore edges. No state transition or evidence changed. The 2010 section
remains separate; the two focused embedded tests are complementary, not one
end-to-end case. GitHub rendering of the current revisions remains pending
publication. No builds, tests, CI, or Requirements Lab work.

## 2025 exception-reporting route review — 2026-10-14

Rendered and visually inspected all six Mermaid diagrams in
`HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md`. The two long Route A paths were
separated at the actual backend boundary into embedded-registry and
process-endpoint sequences, with shorter participant labels. This makes the
shared HLAreportException result easier to follow without suggesting common
internal scheduling. The caller's typed exception, observer-side
HLAreportException, rejected-MOM HLAreportMOMexception, and separate
HLAreportServiceInvocation remain distinct. The combined process two-report
trace is still source-derived; its selected tests do not jointly enable both
switches or establish report arrival order. No test/build/CI, 2010, or
Requirements Lab work. GitHub review of the revised six-chart guide remains
pending publication.

Next handoff: locally render and inspect the diagrams in
`HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md`. Focus on
private request correlation versus pushed event frames and explicit polling,
callback-model timing, and the per-recipient TSO callback/ack boundary. Keep
this 2025 process path distinct from the separate 2010 reference RTI; do not
rerun builds, tests, or CI for this documentation review.

## 2025 process-endpoint request and callback diagram review — 2026-10-14

Rendered all 11 Mermaid diagrams in
`HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md` with Mermaid
CLI 12.0.0 and visually inspected each local preview. Reorganized sequences at
real ownership boundaries: sender request versus recipient stream; event
acquisition versus callback-bridge scheduling; receive-order attribute
planning/commit versus recipient delivery; and TSO callback outcome versus the
private completion-ACK exchange. Reoriented the HLA/API-to-process identity map
and polling-family selector top-down, and shortened long labels. Fixed a nested
sequence-note placement required by the renderer. These are navigation and
presentation changes only; the requestId, TSO messageId, timestamp,
MessageRetractionHandle, and public-void/private-result distinctions remain
separate. The callback-throw/no-ACK path and post-Resign pushed-session cleanup
remain source-derived and explicitly unverified by a focused runtime case; the
historical 284 focused public process-TSO assertions do not cover those gaps.
No 2010 inference, builds, tests, CI, or Requirements Lab work. GitHub review
of this revised guide remains pending publication.

## 2025 known-object subscription scope-change chart review — 2026-10-14

Extracted and rendered only the revised scope-change Mermaid block from
`HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md` with Mermaid CLI
12.0.0, then visually inspected the preview against the adjacent explanation.
Its tall top-down layout is readable. The receiver scope-advisory gate and
the newly added subscribed-attribute value question visibly branch
independently from the same per-attribute scope delta. The text keeps embedded
initial reflection on a newly discovered object and Auto Provide distinct;
the already-known ordinary-object value result remains open. The separate
process path still identifies only the MOM initial-value event and does not
imply ordinary process behavior. No source edit or normative inference was
warranted. No builds, tests, CI, Requirements Lab, or 2010 comparison.

## 2025 Attribute Value Update Request entitlement review — 2026-10-14

Compared the pinned §6.21 API declaration, bounded provider-planning and
ordinary-receiver source, and the focused embedded/process request tests. The
vendored header provides the method signature and exception list, not the
normative clause prose; the official IEEE listing presents standard access by
purchase or subscription. Added that access pointer beside the guide's open
question. The existing HLA-facing sequence and prose already separate the
provider solicitation from a later value-bearing update and its receiver
eligibility; no diagram change was warranted.

The inspected ordinary provider planner selects current attribute owners;
the later ordinary update recipient planner separately applies subscription
and regional criteria. This is source evidence, not a normative ruling.
Embedded request/response examples subscribe the requester. The configured
process object-request fixture seeds subscription state directly in the
registry before the provider's later ordinary update. The process class-request
case ends after the provider callbacks and has no later value response. No
focused case tests an unsubscribed requester receiving a provider response.
The RTI-owned MOM direct-reflection exception remains separate from ordinary
provider solicitation. Keep direct response entitlement under §6.21 open
until authorized clause text is available; do not infer it from code or these
tests. No builds, tests, CI, Requirements Lab, or 2010 comparison.

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

The selected embedded owner-switch case drains callbacks while the switch is
off, then re-enables it before a fresh active subscription. It does not cover
a persistent relevant state or pending callback across off/on. Selected
process callback tests enable the switch during setup only. Source review
shows the setter itself does not run the relevance transition planner, but
embedded and process delivery have distinct later checks: embedded rechecks
when its queued callback runs; process checks at event admission/receive and
again at callback dispatch. Previously planned/admitted work may pass if the
switch and expected state satisfy those gates when reached. This is a 2025
source observation, not normative evidence or profile equivalence. The guide
now records that distinction in prose and its evidence map; no diagram or
runtime test was added. No 2010 comparison, build, test, CI, or Requirements
Lab work.

## 2025 process class-level declaration advisory audit — 2026-10-14

Bounded process-source review found remote getter operations for the object-
and interaction-class relevance switches, but no dedicated wire SET
operations; the public Set methods use the embedded-registry path. The process
HLAsetSwitches MOM route updates stored member switches, and its focused test
sends that interaction and checks getters, not direct Set APIs or declaration
callbacks. Process declaration handlers do not call the class-level planner,
and the surveyed process callback route contains no class-level advisory event
or callback case. Existing focused class-level advisory tests are embedded;
the separate process per-instance Turn Updates tests do not establish this
callback family. Updated the guide to scope the class-level flow accordingly.
Source-only, not normative or parity evidence; no new diagram, build, test,
CI, 2010, or Requirements Lab work.

## 2025 process explicit joined-federate MOM value request — 2026-10-14

The 2025 MIM requires a current-value response for a request on the
`HLAfederate` MOM class/instance even when the requested attribute is
Conditional. The configured process path validates the public handles and
membership, checks server-side object-known state and attribute definitions,
projects the current MOM/member values, and returns an RTI-owned attribute
event without soliciting a federate `provideAttributeValueUpdate` callback.
The event is pushed or polled and then reaches `reflectAttributeValues` through
the callback bridge. A focused two-federate `HLA_EVOKED` test requests
`HLAreportServiceFile` and asserts one current-value reflection. It subscribes
before the request and does not test the no-subscription path, rejection cases,
other callback models, or automatic Conditional update triggers. The Federation
MOM guide now distinguishes the request gates/projection from the transport and
callback-delivery stages; the 13 charts in that guide rendered, and the three
new MOM charts were visually inspected. No C++ changes, build, tests, CI,
Requirements Lab, or 2010 comparison.

Next handoff: compare a normal 2025 process object-instance
`requestAttributeValueUpdate` with the RTI-owned joined-federate MOM branch.
Trace provider selection and `provideAttributeValueUpdate` solicitation versus
the direct MOM projection, then receiver `reflectAttributeValues` delivery.
Reuse the existing Attribute Value Update Request guide and focused process
test; add or revise a diagram only for a concrete reusable reader gap. Keep
2010 separate. No builds, tests, CI, or Requirements Lab.

## 2025 process ordinary AVU versus RTI-owned joined-federate MOM — 2026-10-14

Compared the configured-process object-instance request paths. For ordinary
objects, the registry requires a requester-known instance and groups requested
attributes by current remote owner; the service delivers a request-tagged
`provideAttributeValueUpdate`, and any provider response is a separate update
with its own tag before receiver reflection. For an RTI-owned joined-federate
MOM object, the service checks membership, known-object state, and defined
attributes, projects current values, and directly reflects them to the
requester without a federate Provide callback. This is source-derived process
behavior, not a portable response-entitlement rule or a process/embedded
parity claim.

The existing configured-process ordinary instance test observes Provide,
later update, and reflection under both `HLA_EVOKED` and `HLA_IMMEDIATE`; the
joined-federate `HLAreportServiceFile` test observes the direct current-value
reflection under `HLA_EVOKED` only. The tests were inspected, not rerun. Added
one ownership-fork flowchart to the AVU guide and linked the process handler,
registry planner, callback bridge, and focused tests. Mermaid CLI 12.0.0
rendered all six charts in that guide; the new comparison chart was visually
reviewed. A fresh docs/design census counts 292 diagrams in 34 of 43 Markdown
files: 164 flowcharts, 105 sequences, and 23 state diagrams. No C++ changes,
build, test reruns, CI, Requirements Lab, or 2010 comparison.

Next handoff: audit callback observation timing for the 2025 process AVU path
against the callback/service-ordering guide. Use the existing ordinary-request
case under both callback models to distinguish the service request, callback
observation, and separate provider update; keep API-level behavior separate
from Umbra dispatch details. Add a chart only for a demonstrated reader gap.
Keep 2010 separate. No builds, tests, CI, or Requirements Lab.

## 2025 process AVU callback-observation evidence boundary — 2026-10-14

Compared the ordinary process AVU test sequence with the existing callback
guide. The test returns from `requestAttributeValueUpdate` before its next
checkpoint: `HLA_EVOKED` explicitly calls `evokeCallback`, while
`HLA_IMMEDIATE` uses a later `getObjectClassHandle` lookup that the harness
labels an immediate-callback flush before asserting Provide. The same pattern
is used after the provider's later `updateAttributeValues` call before the
reflection assertion. The test therefore shows the callback is observed by
those later checkpoints in both selected configurations; it does not record a
before-return/after-return callback boundary or cross-federate order.

The callback guide already explains the generic distinction: HLA_EVOKED leaves
callback dispatch to an Evoke service, while HLA_IMMEDIATE can enter callback
code while its inducing service is still executing. That rule does not settle
this AVU-specific integration timing. Added this evidence boundary and links
to the AVU guide; no duplicate chart was warranted. No source changes, test
reruns, build, CI, 2010 comparison, or Requirements Lab work.

Next handoff: audit the 2025 Object and Interaction Information Flow guide at
the boundary between per-federate object-known/discovery state and the first
attribute reflection. Cross-read the focused Object Attribute Subscription
State guide and bounded pinned API, source, and test evidence. Separate the
normative callback sequence from source/test commit points and observation;
add or revise a diagram only for a demonstrated reader gap. Preserve the
2010/2025 boundary and do not infer process/embedded parity. No builds, tests,
CI, or Requirements Lab.

## 2025 discovery and first-reflection boundary review — 2026-10-14

Cross-read the broad Object and Interaction Information Flow guide and the
focused Object Attribute Subscription State guide. The pinned
`FederateAmbassador` header exposes callback signatures and labels §§6.9/6.11,
not normative clause text. In the selected embedded 2025 path, ordinary-object
initial values are planned after `discoverObjectInstance` returns; the focused
late-subscription test observes discovery and the selected stored value in
separate callback collections, so it proves occurrence/value but not relative
callback order. The process receive selector gives a pending discovery its
branch before the ordinary attribute-event slot after earlier-priority event
checks, while the focused public process test sends a separate explicit update
and records discovery/reflection as independent booleans. The process discovery
enqueue's follow-up initial-value event is MOM-only. These are profile-scoped
source/test observations, not a normative callback-order or embedded/process
parity claim; exact §5.8/§6.9/§6.11 prose remains unverified.

The detailed receiver-state table and callback chronology already live in the
focused subscription guide; no duplicate chart was warranted. Added a concise
embedded-versus-process summary to the broad guide. No diagram changed, so no
render or visual review was needed. No source edits, test reruns, build, CI,
2010 comparison, or Requirements Lab work.

Next handoff: audit application observation timing for the embedded 2025
discovery/initial-reflection path under HLA_EVOKED and HLA_IMMEDIATE. Trace the
public subscription call, callback admission/dispatch, and focused tests;
separate the public callback-model contract from Umbra's implementation timing
and what the selected tests assert. Add a chart only for a demonstrated reader
gap. Keep process and 2010 evidence separate; no builds, test reruns, CI, or
Requirements Lab.

## 2025 embedded discovery callback-model observation — 2026-10-14

The focused late-subscription test connects with HLA_EVOKED and drains callbacks
only after `subscribeObjectClassAttributes` returns. It observes discovery and
the filtered stored-value reflection, but stores each callback family in a
separate vector; it has no HLA_IMMEDIATE variant and asserts neither their
relative order nor callback entry before the service returns. In the embedded
source, the subscription commits declarations and plans discovery before
submitting a callback task; that task enters `discoverObjectInstance`, then
plans and invokes ordinary initial reflections after the discovery callback
returns. The callback route uses the ambassador dispatcher: HLA_IMMEDIATE can
drain on the submitting thread, while a submission made during another
thread's active immediate drain remains queued. Thus the source and this
HLA_EVOKED test do not establish an unconditional Subscribe-before-return
observation rule for HLA_IMMEDIATE. The generic callback guide already charts
the model distinction; no duplicate chart was warranted. Added a concise
callback-model evidence note to the broad object guide. No source edits, test
reruns, builds, CI, 2010 comparison, or Requirements Lab work.

Next handoff: audit the callback-visible known-instance boundary for embedded
2025 discovery. Check which instance state is committed when
`discoverObjectInstance` begins, whether an application can use matching
object support services reentrantly from that callback, and which focused
tests demonstrate either behavior. Separate the portable HLA-facing effect
from Umbra's registry commit timing; add a diagram only for a demonstrated
reader gap. Keep process and 2010 evidence separate; no builds, tests, CI, or
Requirements Lab.

## 2025 embedded discovery callback-visible support state — 2026-10-14

The embedded discovery task calls the registry transition, releases the
federation-state lock, and then enters `discoverObjectInstance`. The source
comment explicitly says matching 2025 support services may be used from that
callback. `getKnownObjectClassHandle` reads the committed receiver-known
entry. The pinned API provides the support-service signature and clause label,
but not the normative callback-reentry prose. The focused ownership-query test
evokes and records discovery before calling `getKnownObjectClassHandle`; a
bounded scan found no focused case making that call from inside
`discoverObjectInstance`. Thus reentrant usability is source-supported Umbra
behavior, not independently test-verified or a portable normative conclusion.

Added this source/test boundary to the existing object guide. Its discovery
sequence and commit-point notes already cover the state transition; another
chart would duplicate those visuals. No diagram changed or required rendering.
No source changes, test reruns, builds, CI, 2010 comparison, or Requirements
Lab work.

Next handoff: audit the distinct receiver- and owner-facing effects when a
2025 subscription makes an existing object newly discoverable: receiver
discovery/initial values, owner `Turn Updates On/Off` advice, and the separate
Auto Provide solicitation. Cross-read the focused subscription, relevance
advisory, and Auto Provide guides with their selected tests. Preserve callback
actor, payload, and observation boundaries; do not infer a universal order or
process/2010 parity. Add or revise a diagram only for a demonstrated reader
gap. No builds, tests, CI, or Requirements Lab.

### 2025 late-subscription effects and state/event boundary — 2026-10-14

Rechecked the object-information, subscription-state, relevance-advisory, and
Auto Provide guides against the selected source/test evidence. Their existing
flows distinguish receiver discovery and embedded initial values, owner-only
`Turn Updates On/Off` advice (no values), and provider-facing Auto Provide
solicitation (which itself supplies no values). The focused evidence does not
establish a universal callback order, and the process route remains a separate
boundary: ordinary initial values are not demonstrated by the reviewed process
case; the process initial-value event is limited to RTI-owned MOM objects.

Added a short HLA-first comparison to the DSE guide: newly discovered object
state versus one specific interaction send retained for delivery-time
eligibility. The embedded late-object test observes discovery and selected
values but not their relative order; the receive-order DSE interaction test
shows a later subscription admitting the retained send only in its enabled
case. Exact §§5.8 and 8.1.8 wording remains unverified against licensed text.
The existing diagrams already show each state transition, so no new chart or
render was warranted. No source changes, test reruns, builds, CI, 2010
comparison, or Requirements Lab work.

Next handoff: audit the 2025 already-known-object case where a receiver adds an
attribute subscription after discovery. Determine, from bounded embedded and
process source/test evidence, whether a current value is reflected without a
new publisher update; keep the RTI-owned MOM exception and DSE-retained sends
separate. Preserve the §5.8 verification limit and add a diagram only if a
reader gap remains. No builds, tests, CI, or Requirements Lab.

### 2025 already-known receiver adding an attribute — 2026-10-14

The inspected embedded subscription mutation returns scope-change and
attribute-relevance plans, not ordinary value recipients. Its discovery
replanner skips an instance already in that receiver's known-object map, while
the ordinary initial-value planner is invoked only after a discovery callback.
The reviewed process handler also replans discovery; the process discovery
enqueue path constructs an initial-value event only for RTI-owned MOM objects.
No ordinary subscription-time snapshot path is visible in these bounded source
paths. This is not a normative conclusion: the licensed §5.8 text remains
unreviewed.

The nearest embedded additive/mode test declares its attributes before object
registration; the later mode changes are followed by explicit publisher
updates before reflection is asserted. The selected process discovery test
subscribes before registration, and the separate MOM test exercises its own
initial-value path. These tests do not isolate an already-known ordinary
object receiving a current value solely because a new attribute was
subscribed. Added the nearest embedded-test limit to the subscription guide;
no additional chart was justified. No source changes, test reruns, builds, CI,
2010 comparison, or Requirements Lab work.

Next handoff: survey the 2010-only DataElement and composite encode/decode
behavior against the pinned 2010 API, reference implementation, and its
encoding/composite smoke sources. Decide whether the existing 2010 guide has a
real reader gap in the data-representation path; add a separate 2010 flow only
if it clarifies HLA concepts and source-backed behavior. Do not compare or
borrow from 2025. No builds, tests, CI, or Requirements Lab.

## 2010 composite decoding — 2026-10-14

The bounded survey found that the separate 2010 DataElement guide already had
two appropriate diagrams for scalar order and composite encoding; the stale
handoff had mistaken this as missing work. Added a third, compact flow for
shape-driven composite decoding: fixed arrays use configured slots, variable
arrays read a signed big-endian count (reject negative; zero stops after the
prefix), fixed records decode declared fields and validate padding, and
variants decode the discriminant before selecting an alternative. Unknown
variant discriminants leave the cursor after the discriminant, while the
public full-buffer decode rejects any remaining bytes. Truncation and invalid
padding also fail in the inspected 2010 implementation.

The chart is source-observed profile behavior, not a normative algorithm
claim. The dedicated smoke source covers fixed/variable round trips and count
or encoded-length observations plus rejection of nonzero fixed-record
padding; it does not independently cover negative counts, truncation, trailing
bytes, or unknown variant discriminants. All three diagrams in the guide
rendered locally; the new chart was visually reviewed after a layout revision.
No tests, build, CI, Requirements Lab, or 2025 code or behavior were touched.

Next handoff: bounded 2010-only review of logical-time value representation,
ordering, and arithmetic against the pinned time API, separate 2010
implementation, and focused smoke sources. Decide whether those value
semantics have a genuine reader gap; do not conflate them with time regulation,
advancement, or grants, and do not compare with or borrow behavior from 2025.
No builds, tests, CI, or Requirements Lab.

## 2010 logical-time values and the RTI service boundary — 2026-10-14

The 2010 reference guide already said that value operations do not implement
time advance, but its diagram reduced all value behavior to a generic
construct/arithmetic/encode box. Reworked that existing diagram into two
clearly separated layers. The first follows the pinned abstract value/factory
API and Umbra's integer64/float64 models through matching `LogicalTime` and
interval values, initial/final and zero/epsilon sentinels, comparison and
time/interval arithmetic, and value serialization. The second shows only the
2010 ReferenceRTI boundary: execution creation accepts but does not wire a
time-implementation name, and time-management/timestamped ambassador calls
reach the generated `unavailable()` path.

The pinned headers state that the federation provides the time representation
and factory; this is an API contract, not a complete clause-level standard
interpretation. The integer, float, and marshalling smokes cover selected
sentinels, arithmetic, factory selection, and encodings, but do not exercise
the ambassador stubs. The reference-profile source shows type/range checks;
uncovered type-mismatch and error branches remain source-only. The final chart
was rendered locally and visually inspected in a compact, readable vertical
layout. It replaces the earlier chart; the diagram count is unchanged. No
tests, build, CI, Requirements Lab, or 2025 behavior were touched.

Next handoff: audit the existing 2025 federation/federate lifecycle guide for
an HLA-first membership/execution mental model across Join, Resign, connection
loss, and Destroy Federation Execution. Compare only bounded 2025 API, source,
and existing focused-test evidence; revise or add a diagram only for a real
reader gap. Preserve embedded/process distinctions and keep 2010 separate.
No builds, tests, CI, or Requirements Lab.

## 2025 federation lifecycle audit and visual review — 2026-10-14

The bounded guide/API/source/test survey found no missing top-level lifecycle
map: the guide already distinguishes ambassador connection, federate
membership, and execution lifetime, then shows Create/Join/Destroy separately
from Resign/Disconnect and forced connection loss. That is clearer than another
combined state chart, so no duplicate HLA map was added.

Rendered all 15 Mermaid diagrams in the current lifecycle guide. Visual review
found its active-save resignation sequence too dense: registry internals and
callback-polling calls obscured the key HLA teaching point. Reworked the
sequence around the focused `HLA_EVOKED` scenario: a Resign attempted inside
the save-initiation callback fails, the retry after callback return succeeds,
the active Save fails for survivors but not the leaver, and the survivors can
begin a later Save. Removed semicolons from Mermaid labels after the renderer
identified them as statement separators. The corrected sequence renders and
was visually reviewed. The separately reviewed connection-loss TSO cutoff
flow remains tall but readable; its exact §4.4 wording is still unverified
against the full standard. The revised sequence is one selected embedded 2025
test, not a global callback-order or process/2010 parity rule.

No tests, builds, CI, Requirements Lab, or 2010 behavior were changed. Local
rendering does not replace the post-publication GitHub check.

## 2025 FOM admission/composition failure audit — 2026-10-14

Audited the existing 2025 guide against the pinned Create/Join API, module
coordinator, composer, embedded lifecycle path, and focused tests. Its four
diagrams already distinguish module-local DIF validation, ordered full-set
composition and reference resolution, FDD validation, Create's
prepare-before-reserve boundary, and the registry-atomic Join from later
post-commit setup. No new flow was justified. Added an embedded
preparation-to-exception map and compared all pinned Create/Join overloads with
the selected adapter paths. Recorded four source/API differences: vector
Create can translate selected-MIM errors omitted from its API list; Create with
an explicit MIM can translate unavailable-time errors omitted from its list;
additional-module Join can translate failures from revalidating its retained
MIM although Join lists no MIM exceptions; and single-module Create lists a
standard-MIM-designator exception its embedded delegation path cannot produce.
These are implementation/API observations, not HLA-wide behavior; focused
public-adapter coverage is absent. The pinned headers are byte-identical to
the canonical IEEE API archive per the local manifest; the full prose standard
is not vendored, so no additional normative rule was inferred. No build, test,
CI, Requirements Lab, or 2010 change.

Next handoff: continue the bounded 2025 Request Attribute Value Update reader-
gap review in `HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md`, focusing
on the open direct-requester response-entitlement question. Reuse the pinned
API, current state-planning source, and existing focused cases; keep its
normative status unresolved unless exact 1516.1 clause text is available. Add
no duplicate diagram without a demonstrated reader gap. Keep 2010 separate;
no broad builds, tests, CI, or Requirements Lab.

## 2025 DataElement byte-order boundary audit — 2026-10-14

Rechecked the 2025 encoding guide's endian story against the pinned API, the
explicit `ByteOrder` primitive, integer/float helpers, variable-array codec,
and focused vectors. The guide already distinguishes element-selected BE/LE,
fixed-format BE encodings, one-octet values, child-owned composite encoding,
factory-defined logical time, and Umbra's separate big-endian process frame.
The variable-array count is `HLAinteger32BE`; the source/test path also rejects
negative counts and malformed alignment padding. Existing diagrams cover the
representation and composite decisions, so no new flow or wording change was
justified. No C++ checks, builds, CI, or 2010 changes were made.

Next handoff: boundedly audit the 2025 process-endpoint callback-after-Resign
boundary in `HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md`. Trace
queued-task/session cancellation through Resign and public Evoke/Enable against
the focused bridge tests; separate source-supported reachability from HLA
permission, and revise a diagram only for a concrete teaching gap. Keep 2010
separate; no builds, test reruns, CI, or Requirements Lab.

## 2025 Process callback stream and Resign timing audit — 2026-10-14

The bounded Resign trace found that process callback work crosses three
different cut points: server-side session queues, client-buffered event frames,
and tasks already submitted to the shared callback dispatcher. The server
clears its still-queued joined-session event state before returning success.
The process client's default correlated-request path drains frames already
buffered at its explicit dispatch point before the client Resign method returns;
enabled HLA_IMMEDIATE work may invoke before the public Resign returns, while
HLA_EVOKED or disabled-immediate work can remain queued. Frames arriving after
that explicit drain resume through the background dispatch loop once request
suppression lifts, with no source-established ordering against the public
return. The client clears its joined identity and cancels exception-report
projections; the adapter then applies Resign and clears its membership fields
without resetting the shared dispatcher. `close()` is the path that closes the
callback session and resets that queue.

No focused test establishes unrelated pending-task delivery after public
Resign or the timing of a pushed frame racing the explicit drain. The bridge
test covers exception-projection cancellation only; the process Evoke test
drains its selected callbacks before Resign. No normative post-Resign service
permission was inferred. This audit also corrected stale process-endpoint-guide
text that said the client lacked a background reader: the client starts a
stream-reader thread and a separate callback-dispatch thread. Updated two
existing process-endpoint diagrams and the callback/Resign diagram; Mermaid
CLI 12.0.0 rendered all three and they were visually reviewed. One sequence
label's semicolons were replaced after the first render caught Mermaid parsing
them as separators. No tests/build/CI, Requirements Lab, or 2010 changes.

Next handoff: render the four additional diagrams and inspect the open
ownership-timeline chart in `HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md`. Check the
portable receiver outcomes separately from producer-time, regional, and
ownership test boundaries; update backlog row 12 and the review ledger. Keep
2010 separate; no builds, test reruns, CI, or Requirements Lab.

## 2025 TSO retraction visual-review follow-up — 2026-10-14

Rechecked the current TSO guide handoff against the dated ledger: the ownership
timeline had already been locally reviewed, so that part of the pointer was
stale. Refreshed that chart and locally rendered/inspected four additional
charts: the per-recipient state machine, mixed-recipient interaction sequence,
focused producer-time boundary sequence, and process receiver-Resign route.
All ten Mermaid blocks have local render evidence; nine of ten now have local
visual review. The timestamped-delete diagram retains its older 2026-10-09
GitHub visual review and is the only chart without current local visual
inspection. The portable outcome model remains an orientation, not a complete
§§8.22–8.23 decision table. Mixed fan-out and producer-time examples are
selected embedded 2025 cases; the process receiver-Resign route is source-only
and does not prove a post-Resign public callback. No C++ tests/build/CI, 2010,
or Requirements Lab work was done.

Next handoff: trace the 2025 process `HLAsetSwitches` path from per-member
switch storage and getter readback to the `HLAfederate` MOM attribute
projection. Separate server state from observer reflection, check the focused
test gap, and update the cross-cutting state/event observation guide only for
a demonstrated reader gap. Keep embedded, process, and 2010 boundaries
explicit; no builds, test reruns, CI, or Requirements Lab work.

## 2025 process switch-observation map and TSO final local review — 2026-10-14

The detailed Federation MOM guide and review ledger already traced process
`HLAsetSwitches` storage, getter readback, current `HLAfederate` value
projection, and the missing automatic Conditional reflection for object-class
and interaction-relevance switches. The cross-cutting guide did not make the
control-surface distinction easy to find, so its switch map now contrasts the
embedded direct RTIambassador setter/reflection path with the process Federate
MOM message path and links to the detailed trace. This is a source/test-scoped
process observation, not a conformance conclusion or backend/2010 parity
claim.

Rendered the complete TSO retraction guide with Mermaid CLI 12.0.0 and
visually inspected its remaining timestamped-delete restoration chart. Its
callback-entered restoration/filter/retraction route and no-callback pending-
reservation cancellation route are legible without clipping. All ten TSO
diagrams now have local render and visual-review evidence; GitHub rendering of
current revisions remains pending publication. No C++ tests/build/CI,
Requirements Lab, or 2010 behavior work was performed.

Next handoff: render and visually audit all current diagrams in the 2025
Attribute Ownership guide. Check the ownership service-selection map and
process-specific flows for readable layout and clear portable-HLA versus
implementation-evidence boundaries; fix only concrete problems. Keep 2010
separate and do not build, rerun tests, start CI, or access the Requirements
Lab.

## 2025 DDM guide full local render and visual review — 2026-10-14

Rendered all 20 current Mermaid blocks in the Data Distribution and Regions
guide with Mermaid CLI 12.0.0 and visually inspected every chart. The complete
pass found two inline-code labels in the Region deletion gate that prevented
parsing; removing only the label formatting allowed the chart to render. The
Region-template lifecycle state chart was changed from an excessively wide
left-to-right layout to a compact top-down layout. The multi-recipient process
sequence was narrowed from seven lanes to four by combining client/service
roles while retaining strict and relaxed DDM outcomes, source and receiver
ranges, independent recipient selection, and the separate callback models.
The focused test source shows A1/A2 as the two message tags, so the diagram now
matches those values. Its explanatory note explicitly omits client/server
framing and does not claim cross-receiver callback ordering. All 20 charts
render and are locally visually reviewed; GitHub rendering remains pending
publication. The cited test was inspected but not rerun. No build, test, CI,
Requirements Lab, or 2010 behavior work was performed.

## 2025 Attribute Ownership guide full render and visual review — 2026-10-14

Rendered all 18 current Mermaid diagrams in
`HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md` with Mermaid CLI 12.0.0 and
visually inspected the complete set. The full pass found a sequence-message
semicolon that broke Mermaid parsing; replacing it with “and” fixed the chart
without changing its meaning. The images also showed dense API-name labels in
the regular acquisition, If-Available, negotiated-divestiture, process
cancellation, and If-Wanted sequences. Shortened those diagram labels to the
HLA action/result while retaining exact API names and test/source citations in
the surrounding guide. The revised charts were re-rendered and visually
checked. Long charts remain scrollable but have no clipped content; GitHub
rendering is still pending publication. This was a documentation-only pass:
the focused test sources were not rerun, and no build, CI, Requirements Lab,
or 2010 behavior work was performed.

## 2025 FOM admission/composition guide full local render review — 2026-10-14

Rendered all four current Mermaid diagrams in
`HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md` with Mermaid CLI
12.0.0 and visually inspected each. All parse successfully and are legible at
full size or while scrolling. The Create-preparation, ordered-composition and
FDD-validation, and additional-module Join commit/rollback paths remain
distinct. The edition-selection chart clearly isolates the optional 2010
catalog-only route from the 2025 FDD path. No visual or syntax correction was
warranted. This review does not revalidate implementation semantics or tests;
no build, test, CI, Requirements Lab, or 2010 behavior work was performed.

Next handoff: render and visually audit all six current diagrams in the 2025
Exception-Reporting guide. Check caller typed exceptions separately from
observer-side exception/MOM/service-invocation reports, and retain the known
combined-process-report and arrival-order evidence limits. Keep 2010 separate;
do not build, rerun tests, start CI, or access the Requirements Lab.

## 2025 Exception-Reporting guide full local render and visual review — 2026-10-14

Rendered all six current Mermaid charts in
`HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md` with Mermaid CLI 12.0.0 and
visually inspected the complete set. All charts parse. PNG review found
crowded labels and participant-name overflow in the embedded/process report
sequences, callback-exception boundary, and combined process-report trace.
Shortened natural-language messages and lane names while preserving exact
report protocol names, distinct caller/API/hook/client/server/observer roles,
the `noexcept` catch boundary, and the caveat that combined request order does
not prove callback arrival order. Rerendered all six and visually rechecked
the revised charts. No C++ tests/build/CI, Requirements Lab, or 2010 behavior
work; GitHub rendering awaits publication.

## 2025 Service Invocation Reporting guide full local render and visual review — 2026-10-14

Rendered all eight current Mermaid diagrams with Mermaid CLI 12.0.0 and
visually reviewed the complete set. Shortened labels in the service-reporting
interlock and final-resignation transition, removed a redundant report-write self-loop from
the per-membership report-file lifecycle, and compacted the post-commit
ConnectionLost self-messages after previews showed overlap or clipping. The
switch/sink gate remains in prose alongside the exact interlock exceptions,
stable membership path, and registry-commit versus RTI post-commit
cleanup/failure boundary. The final eight-chart render succeeded and revised
charts were visually rechecked.
This documents existing source/test evidence; it is not a fresh semantic or
test verification. GitHub review awaits publication. No builds, tests, CI,
Requirements Lab, or 2010 behavior work.

## 2025 Callback and Service Ordering guide full render and visual review — 2026-10-14

Rendered all 12 current diagrams with Mermaid CLI 12.0.0 and visually reviewed
the full set. The process synchronization sequence was especially dense, so
shortened participant labels and message text using the adjacent explanation
for detail. Its registration and achievement paths remain distinct, including
private request responses versus recipient events, push versus polling, report
submission order, callback-model dispatch, and the source/test boundary. All
12 diagrams rendered after the revision. This is a layout review, not a fresh
source or test audit. GitHub review awaits publication. No builds, test reruns,
CI, Requirements Lab, or 2010 behavior work.

## 2025 object discovery and first-reflection boundary audit — 2026-10-14

Bounded inspection confirmed that the embedded path commits receiver-known
state as discovery dispatch begins, calls `discoverObjectInstance`, and then
plans current subscribed values. Its HLA_EVOKED late-subscription test asserts
one discovery and one stored-value reflection but records them separately, so
the test itself does not assert order. The process pull/`receive_interaction`
selector prioritizes its discovery event over the later attribute-reflection
slot when that part of the selector is reached; other event-family priorities
remain separate. The focused public process case sends an explicit update and
asserts both callbacks using separate flags, not their order. The ordinary
process discovery path has no initial-value event in the inspected source;
that branch is limited to RTI-owned MOM objects. This is source/test evidence,
not a conformance finding: the checkout provides API declarations and clause
labels, but not normative §5.8/§6.9/§6.11 text for independent verification.

The companion subscription-state guide already contains a visual
embedded/process late-subscription comparison, so no duplicate chart was
added. Clarified that this guide's process ordering claim is pull-selector
specific and linked directly to that existing chart. No tests were rerun,
builds or CI started, or 2010/Requirements Lab work performed.

Next handoff: audit the 2025 discovery-triggered Auto Provide path against
initial-value reflection and explicit Request Attribute Value Update. Compare
the callback purpose, trigger, tag, value-delivery boundary, process route, and
focused evidence; reuse existing diagrams and add or revise one only for a
demonstrated reader gap. Keep 2010 separate; no builds, test reruns, CI, or
Requirements Lab.

## 2025 Auto Provide neighboring-path audit — 2026-10-14

Reviewed the existing Auto Provide comparison, its discovery/provider-fan-out
diagram, focused embedded cases, and bounded process request-routing evidence.
The guide already distinguishes discovery-triggered owner solicitation (empty
automatic tag), explicit Request Attribute Value Update (caller tag), current
value reflection, and the RTI-owned joined-federate MOM direct-reflection
exception. Its evidence section correctly limits verified Auto Provide
planning to the embedded path; process explicit-request routing and switch
mutation do not establish process automatic-discovery parity. No redundant
diagram was added. Added a direct link from the object-information guide's
first-reflection boundary to the existing neighboring-path comparison. No
tests/build/CI, Requirements Lab, or 2010 work.

Next handoff: trace the 2025 process object-instance Request Attribute Value
Update from the public service through request encoding, process handler,
provider callback, and later provider update/reflection. Compare tag ownership
and the joined-federate MOM direct-reflection exception with focused evidence;
do not infer class/regional parity or Auto Provide parity from this route.
Reuse existing diagrams and change them only for a demonstrated reader gap.
Keep 2010 separate; no builds, test reruns, CI, or Requirements Lab.

## 2025 process object-instance request transport map audit — 2026-10-14

The current request guide already explains the process HLA-level object/MOM
branch, request-tag versus provider-update-tag ownership, and the separate
value-delivery step. Its focused configured-process case checks both
`HLA_EVOKED` and `HLA_IMMEDIATE`, the request tag at Provide, and the distinct
response tag at reflection. Bounded inspection confirmed the private chain:
public adapter → process client/codec → service decode/membership check →
per-provider request event → callback bridge. Added this as an explicitly
implementation-only map, not an HLA wire-protocol diagram or claim. Existing
HLA flow diagram was sufficient; no extra chart or test run.

Next handoff: audit the 2025 FOM module admission/composition guide across
validation, ordered merge, cross-module resolution, and Create/Join commit
boundaries. Compare HLA-first diagrams with bounded API/source/test evidence;
keep 2010 separate and avoid builds, test reruns, CI, and Requirements Lab.

## 2025 FOM module admission/composition reader audit — 2026-10-14

Rechecked the Create, per-module admission, ordered complete-set merge,
cross-module resolution, and additional-module Join commit boundaries against
bounded implementation and focused-test evidence. The commit atomically
replaces the federation definition/handle directories with membership, while
catalog lookup, existing object knowledge/subscriptions/ownership, ordinary
discovery/delivery, and Federation MOM `HLAcurrentFDD` reflection remain
distinct observations. Existing embedded tests cover new catalog lookup and
the MOM reflection; the configured-process test only checks the joining
client's new object-class lookup. The inspected Join commit does not directly
invoke the ordinary discovery planner or synthesize Receive Interaction; that
is a source observation, not a normative conclusion. No focused configured-
process post-Join interaction-delivery case was found.

Added one fifth Mermaid flowchart to make the catalog/runtime/MOM separation
visible. The previous four charts were already rendered and visually
reviewed, but the new chart remains unrendered: Mermaid CLI 12.0.0 is present,
yet Chrome failed to launch in the sandbox. No C++ build, test rerun, CI,
Requirements Lab, or 2010 behavior work occurred.

Next handoff: audit the configured-process post-Join interaction path, keeping
tested catalog lookup separate from the source-only send-time
parent/subscription route. Do not infer delivery without a focused post-Join
case; keep 2010 separate and avoid builds, test reruns, CI, and Requirements
Lab.

## 2025 configured-process post-Join interaction audit — 2026-10-14

Verified the guide's process boundary against the configured public-endpoint
case, private service probe, registry Join commit, and shared interaction
recipient planner. The public case composes an object-only extension, rejects
an invalid extension without changing the base module count, then proves the
new object-class handle/name lookup after Join. It sends no interaction. The
separate inherited-receipt test starts with its complete class hierarchy at
Create and therefore does not test a class introduced by a later Join.

Source inspection confirms that the configured service requires an
`additionalFomPreparation` hook, serializes preparation plus registry Join,
and passes the replacement definition to the same registry commit that
reconciles and swaps interaction-class/parameter directories with membership.
Per-federate interaction declarations are separate state. A later send
resolves the class in the current catalog and walks its parent chain against
stored subscriptions; this suggests a prior ancestor subscription can be
considered for an added descendant, but no focused post-Join send/receive case
verifies delivery. The FOM guide already labels this inference and the test
gap, so no new chart or test was added. No build, test rerun, CI, Requirements
Lab, or 2010 behavior work occurred.

Renderer follow-up: the installed Mermaid CLI was retried with the existing
Edge runtime. Puppeteer failed before parsing or rendering, reporting a running
browser profile and a Windows credential-launch error, as it had with Chrome.
The temporary config was removed. That attempt produced no PNG, leaving the
fifth historically added chart unparsed/unrendered at that point; the failure
did not establish a Mermaid syntax error. Its later successful local render
and visual review are recorded in the post-Join validation section below.

## 2025 Save/Restore Join-contract access recheck — 2026-10-14

Rechecked the official IEEE 1516.1-2025 product page. It presents purchase and
subscription access and links a downloads ZIP, but not the §4.11 clause text.
The prior archive inspection found the machine-readable API package, not the
normative standard. The Save/Restore guide already labels Join admission and
participant effects as unresolved; no diagram or HLA claim was changed. No
build, test rerun, CI, Requirements Lab, or 2010 behavior work occurred.

Next handoff: if an authorized full 1516.1-2025 copy containing §4.11 becomes
available, compare Join exception conditions with Save/Restore participant
effects and update only where the normative text supports it. Do not repeat
the public-page access check or infer HLA rules from code. If the text remains
unavailable, leave the question open and continue the next source/test-backed
reader gap in the portfolio. Keep 2010 separate; no builds, test reruns, CI,
or Requirements Lab.

## 2025 FOM post-Join chart validation and callback-boundary audit — 2026-10-14

The FOM chart described in the earlier handoff as the fifth chart added is the
fourth Mermaid fence in current source order. An initial source-order selection
rendered the existing 2025-API/2010-model lane chart instead; that chart was
also visually checked and remains clearly separated from the 2010 RTI stream.
After matching the current guide's diagram description and source location,
Mermaid CLI 12.0.0 rendered the intended additional-module post-Join chart in a
one-off local headless-browser run outside the sandbox. Visual review found
the catalog, existing live state, Federation MOM reflection, and later-delivery
branches legible without edits. The four older charts retain their recorded
render/review evidence, so all five current FOM-guide diagrams now have local
render and visual-review evidence. GitHub rendering remains pending
publication; this does not resolve the normative Join callback question.

A separate source/test check confirmed both 2025 `ConnectionLost` producers
contain exceptions from `connectionLost` after their respective membership or
local lifecycle transitions. The embedded path also attempts its final
service-report append and completes cleanup/routing before raising an append
error; the process path clears local joined/time state while retaining the
client for its in-flight request to unwind. The focused successful embedded
report case does not throw from the callback, and its previously recorded local
run failed during federation creation before the assertions. The existing
callback and service-report guides already separate these cases and the
untested boundaries, so no duplicate chart was added. No build, test rerun, CI,
Requirements Lab, or 2010 behavior work occurred.

Next handoff: audit 2025 process endpoint callback-exception attribution
across correlated request drains, pushed-event pumps, and HLA_EVOKED against
the callback, exception-reporting, and process request/callback guides plus
their focused tests. Keep producer-side Send Interaction outcomes separate
from recipient request/Evoke call stacks; revise or add a chart only if a
distinct source-backed decision boundary is missing. Do not infer embedded or
2010 parity, repeat the IEEE access check, or rerun local browser rendering
unless the renderer environment changes. No builds, test reruns, CI, or
Requirements Lab.

## 2025 process callback-exception attribution — 2026-10-14

Audited the process receive path across the stream reader, correlated request
drain, background pushed-event dispatcher, callback bridge, and existing
process callback tests. Corrected the wording that attributed inline callback
throws to a “receive pump”: the stream-reader only decodes and buffers. If a
request-backed recipient service drains the event after matching its own
response and releasing the exchange lock, an escaping `HLA_IMMEDIATE` callback
can prevent the process-client request from returning that response. If the
background dispatcher admits the callback, its `noexcept` loop saves the first
exception and the next call to `ProcessFederationClient::request` takes and
rethrows it before sending a request; the pump resumes, and later throws do
not replace the pending first exception. Under `HLA_EVOKED`, the bridge queues
the task and the user callback exception exits the later recipient Evoke path.
The producer's correlated Send Interaction response remains on a separate
process session and is not unwound by recipient callback code.

Added a process-specific sequence diagram to
[`HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md`](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-endpoint-callback-exception-attribution)
and a reciprocal link from the [process endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#2-from-a-stream-frame-to-an-official-callback).
The source chain is linked there. The reviewed focused process cases exercise
successful callback dispatch, not a throwing `FederateAmbassador`; no tests or
builds were run. This is implementation evidence, not a normative HLA
exception-propagation rule. The diagram has not been locally rendered or
visually reviewed, and the public adapter's reporting/exception mapping has
not been traced. No Requirements Lab changes, CI, or 2010 behavior work.

Next handoff: trace one request-drain callback exception and one deferred
background exception through their outer 2025 `RTIambassador` service wrappers.
Determine whether `HLAreportException` or
`HLAreportServiceInvocation` is attempted and which public exception surface
is reached; do not infer a normative rule from wrapper behavior. Keep these
recipient routes separate from producer `Send Interaction`, and preserve the
2010 boundary. Do not build, rerun tests, or start CI; the new diagram's local
render/visual review remains pending.

## 2025 process callback exception at the public wrapper — 2026-10-14

Followed both `HLA_IMMEDIATE` process-client throw routes into the concrete
2025 `getInteractionClassHandle` wrapper. Its process-client-specific catches
translate `ProcessFederationClientError`, not a `FederateInternalError` thrown
from `receiveInteraction`; the outer `catch (rti1516_2025::Exception const&)`
does catch the latter. That catch attempts `HLAreportException` through the
`noexcept` advisory hook, then attempts the failed service-invocation report
before `throw;`. If the report request completes or is suppressed, the
original callback exception is rethrown. A failed process service-report
request is converted to `RTIinternalError` and can replace it. An arbitrary
non-RTI C++ exception bypasses this wrapper's typed catch and those report
hooks. This is one request-backed API implementation path, not a universal
HLA exception rule or evidence for `HLA_EVOKED`.

Added a focused wrapper/report flowchart beside the process callback sequence
and linked the concrete case from the Exception Reporting guide. Source
references distinguish the ambassador catch, advisory exception-report hook,
process report requests, and failed-report conversion. A local Mermaid render
review exposed a syntax-breaking semicolon in the process sequence and clipped
labels in the combined timeline and wrapper chart. Split the sequence into
separate request-drain and background-dispatch views, replaced punctuation
that Mermaid treated as control syntax, and shortened the clipped branch
label. Mermaid CLI 12.0.0 rendered the two sequences and wrapper flowchart; all
three were visually reviewed locally. Existing public process tests still do
not inject a throwing callback or the combined report-failure schedule. No
tests or builds were run. No Requirements Lab, CI, or 2010 behavior work.

## 2025 DataElement encoding coverage and process callback render review — 2026-10-14

Audited the existing 2025 DataElement guide against the pinned `DataElement`
and `VariableLengthData` declarations, representative integer/boolean/string
codecs, the fixed-record and variable-array implementations, logical-time
factory wrappers, and focused byte-vector/composite tests. The guide already
states that there is no public global endian option: the application selects a
BE/LE wrapper when the type offers one; fixed-format elements determine their
own representation; single-octet elements have no multi-octet order; composite
children own their value encoding and alignment; and the logical-time factory
supplies the contained representation while the wrapper's one-octet boundary
is only alignment metadata. Public value octets and private process-frame
metadata/counts are distinguished. The 2010 companion remains separate. No
encoding-guide edit or additional chart was justified by this review.

The global source-fence count is now 297 diagrams across 34 of 43 design
Markdown files (167 flowcharts, 107 sequences, and 23 state diagrams). This is
not a whole-tree render. No C++ builds, tests, CI, or Requirements Lab work
were performed.

Next handoff: compare the pinned 2025 public service API surface with the
existing reader-guide map. Look for missing HLA decision, state, eligibility,
or observation boundaries—not method-name coverage—and identify only a real
reader gap worth explaining. Keep the 2010 implementation and guide separate;
do not use source as normative authority. No builds, test reruns, CI, or
Requirements Lab changes.

## 2025 API service-family coverage and Non-Regulated-Grant reader path — 2026-10-14

Compared the pinned IEEE 1516.1-2025 `RTIambassador` group headings with the
guide index: Federation Management, Declaration Management, Object Management,
Ownership Management, Time Management, Data Distribution Management, RTI
Support, and API-specific services all have reader paths. The bounded index
check found 28 linked guide targets and no missing target files. This is
family-level wayfinding, not a method-by-method conformance inventory.

Checked the RTI Support Services subfamilies against their existing guides:
name/handle and normalization queries, update-rate values, order and
transportation types, available dimensions/region ranges, callback controls,
support switches, exception/service reporting, logical-time factory access,
and handle decoding each have an existing explanatory path. `getNonRegulatedGrantSwitch()`
was a plausible cross-family gap, but the cross-cutting guide already identifies
it as a getter-only static federation policy and the time guide already explains
its effect when GALT is undefined. Added a direct link in the time glossary to
the pinned API declaration and cross-cutting switch-scope map. No new diagram
was warranted; the 2010 path remains separate and no normative claim was added.

The 28 linked 2025 service-family reader-map targets and their heading
fragments were validated with no broken references.

## 2010 guide navigation and evidence-boundary review — 2026-10-14

Checked relative Markdown links in the separate 2010 reference-RTI and
DataElement guides. All 147 references across 100 unique relative targets
resolve; every cited source line is within its target file. The source/test
boundary notes remain explicit: the reference-runtime synchronization barrier
has no focused 2010 test, the binding smoke covers a successful `HLA_IMMEDIATE`
path rather than an `HLA_EVOKED` path, and time-value coverage does not establish
a time-managed execution state machine. The existing diagrams already expose
these limits, so no extra 2010 chart or inferred 2025 parity was warranted.

Next handoff: audit discovery-triggered Auto Provide across the 2025
object-and-interaction information and Auto Provide guides. Compare the
trigger, owner solicitation versus direct initial-value reflection, tag,
value-delivery boundary, process route, and focused evidence. Reuse existing
diagrams; revise or add only for a demonstrated reader gap. Keep 2010 separate;
no builds, tests, CI, or Requirements Lab work.

## 2025 Auto Provide neighboring-path cross-guide audit — 2026-10-14

Compared the object/interaction guide with the dedicated Auto Provide guide,
then checked the cited embedded and process source/test paths. The guides
already distinguish discovery-triggered owner solicitation from stored-value
reflection and federate-issued Request Attribute Value Update; they also
separate the automatic empty tag, request tag, and provider-selected update
tag. The embedded discovery call path performs the discovery callback, handles
separate initial reflections, and then plans Auto Provide; the focused baseline
asserts discovery and provider callbacks separately and checks the automatic
tag is empty. Process explicit requests have a distinct event route, while the
inspected process discovery initial-value branch is limited to RTI-owned MOM
objects; process automatic discovery remains unverified. No chart collapses
these profiles, and no normative ordering or 2010 parity is claimed. The
existing comparison table and diagrams close this bounded reader question, so
no duplicate diagram or guide edit was warranted. Tests were inspected, not
rerun.

Next handoff: audit whether readers need one HLA-first overview of ordinary
object-attribute update eligibility across the existing Ownership,
Object/Interaction, Subscription State, DDM, Update Rate, Order/Transportation,
Time, and Callback guides. Trace sender authority/service acceptance, the
receiver's candidate projection, per-attribute admission, receive-order versus
TSO delivery, and callback observation. Reuse the detailed diagrams and add a
summary only if it reveals a real cross-guide gap. Keep embedded, process, and
2010 evidence distinct; no builds, tests, CI, or Requirements Lab work.

## 2025 object-attribute update eligibility overview audit — 2026-10-14

Compared the proposed cross-guide overview with the current object/interaction
and cross-cutting guides plus the update-rate guide's boundaries. The
cross-cutting guide already has an HLA-first relationship map for optional
provider solicitation versus an independent update, sender authority,
per-receiver knowledge/relevance, order and transportation, TSO eligibility,
the embedded rate example, reflection, and callback observation. It explicitly
marks those connectors as questions rather than a universal execution order,
then links the topic guides that hold their own source and test evidence. The
object/interaction guide separately starts with five service-independent
questions and expands receive-order projection and callback-time boundaries.
The proposed overview therefore already exists; no duplicate diagram or
content was needed. The chart does not assert process/2010 parity or a single
normative pipeline.

Next handoff: audit ordinary versus directed interaction routing across the
object/interaction, directed-interaction, DDM, time, and callback guides. Check
whether readers can follow the distinct sender declarations, target-object
knowledge, receiver selector/subscription rules, region role, recipient set,
and receive-order/TSO observation boundary without conflating a directed target
with a receiver or a region. Compare the focused ordinary and directed cases;
add or revise a summary only for a demonstrated reader gap. Keep embedded,
process, and 2010 evidence distinct; no builds, tests, CI, or Requirements Lab.

## 2025 ordinary-versus-directed interaction routing review — 2026-10-14

Compared the object/interaction routing table and class-ancestry map with the
directed-interaction, DDM, time, and callback guides. Existing material
distinguishes ordinary class subscriptions from directed object-class /
interaction-class publication pairs and per-receiver selectors; it states
that a directed target is context, not a receiver list, and is not a DDM Region.
The ordinary and directed flows also show their different callback payloads and
class/parameter projections, while their time and callback boundaries stay in
the dedicated guides. Focused embedded and process tests remain explicitly
bounded to their selected declaration and callback cases.

The accepted-work edge is already compared in the cross-cutting guide: the
ordinary receive-order route's behavior after producer unpublication is
source-derived and has no focused test; the directed route rechecks its
publication pair and its stale-work case is tested. The directed guide marks
process delivery as a separate materialized-event path without an embedded-
style callback-time registry re-plan, and does not infer post-acceptance
mutation results for process. The existing table, diagrams, evidence maps, and
limits therefore answer the HLA-first comparison; no duplicate summary chart,
normative withdrawal rule, or 2010 claim was added. Tests were inspected, not
rerun.

Next handoff: audit the 2025 process endpoint's ordinary and directed
interaction behavior when the producer withdraws its publication after send
acceptance. Compare send-time event projection, declaration mutation, queued or
pushed event delivery, callback-time revalidation, and focused test coverage.
Clarify only a real process-profile reader gap; keep embedded and 2010 behavior
separate and avoid inferring normative cancellation rules. No builds, tests, CI,
or Requirements Lab work.

## 2025 process interaction publication-withdrawal review — 2026-10-14

Followed the process-profile handoff across ordinary and directed receive-order
interactions. The ordinary guide already records its send-time promoted class
and parameter snapshot, declaration-update path, lack of callback-time
replanning, and absence of a focused post-send mutation test. For directed
interactions, the process send handler creates recipient-specific events with
the selected target/class/parameter projection; the unpublish handler updates
the declaration registry, and the inspected push/queue and callback paths do
not re-plan eligibility for that materialized event. No focused process test
checks producer unpublication between send acceptance and callback delivery,
so the process delivery result remains source-only and unverified. The embedded
accepted-send/unpublish test does suppress the queued callback, but that result
is not generalized across the process boundary. Updated the directed guide to
make this evidence distinction explicit and to cite the relevant construction,
declaration, and push/queue paths. No duplicate chart was needed; no normative
cancellation rule or 2010 behavior was inferred. Sources and tests were
inspected, not rerun.

## 2025 process directed-TSO dispatch revalidation review — 2026-10-14

Traced the time-constrained recipient path from process send acceptance to
TSO dispatch and callback completion. The server retains the directed message
facts and planned recipient entries, but at each recipient's delivery boundary
the dispatcher calls the timestamped directed-recipient resolver again and
constructs the event from the current class/parameter projection. With the
producer still joined, the current publication pair, target record, receiver
knowledge, and selector are live gates; a failed recheck suppresses that
recipient and completes its queue entry without a directed callback. A legal
producer Retract is a distinct path that removes pending TSO entries and
terminalizes their recipient states. The shared registry has a narrower
accepted-message exception for an exact pending recipient after producer
resignation or marked connection loss; it is not ordinary unpublish semantics.

Focused process evidence covers the queued directed TSO callback/grant path
under NMR/NMRA callback gating, but not publication withdrawal, target
departure, or Retract of a constrained queued copy. A separate process
directed-send case does exercise timestamped Retract before and after callback
under both callback models, but its received order is RECEIVE, so it is not
queued behind a recipient's grant. An embedded target-departure case observes
a time-six removal before a queued time-seven directed interaction and then
suppresses the stale target callback; it is explicitly not process evidence.
The directed guide now has a process-specific TSO flowchart and these bounded
evidence distinctions. No builds, tests, CI, Requirements Lab, normative
cancellation rule, or 2010 behavior was added or inferred.

The chart's Mermaid CLI PNG attempt failed before rendering because the
Puppeteer browser process would not launch (reported exit code 2147483651).
No alternate launch route was tried; syntax acceptance and visual review remain
pending.

Next handoff: audit the 2025 process-side producer-resignation and Connection
Lost paths for accepted timestamped directed work. Compare the exact
message/recipient preservation markers with the lifecycle and TSO guides and
identify whether any focused process case exists; keep embedded evidence and
licensed §4.4 wording separate. Document only a demonstrated reader gap, keep
2010 separate, and do not run builds, tests, CI, or Requirements Lab work.

## 2025 process directed-TSO producer departure gates — 2026-10-14

Compared the process directed-TSO delivery resolver with the shared registry's
Resign and Connection Lost markers, the lifecycle/time guides, and focused
tests. The exact departed-source exception requires the stored message ID,
producer, target, and sent class to match; the accepted message must name the
recipient, and that recipient's ledger state must still be pending. Voluntary
Resign sets `producerResigned`; forced Connection Lost instead sets
`deliveryRequiredAfterConnectionLoss` only for a timestamp at or before the
regulating producer's captured, comparable current time. The former is not
cutoff-limited; the latter does not authorize later or uncomparable timestamps.

For that exact exception the registry skips departed-source membership,
publication, and known-object checks, and does not reject an extant target only
because `deleteAccepted` is set. It still requires the target record/type and
the receiver's live membership, target knowledge, selector, and projection.
The connection-loss grant helper separately allows a surviving requester to
reach a pending marked delivery boundary if ordinary GALT became undefined.
The updated flowchart splits the joined-source and departed-source paths and
labels the marker gate. A fresh Mermaid CLI attempt on this revision again
failed before parsing because Puppeteer could not launch (exit code 2147483651);
no alternate launch route was used, so syntax and visual review remain pending.

The focused directed cutoff, late-TAR/automatic-delete, and selector-change or
unsubscribe tests found by this survey explicitly use embedded transport. They
confirm selected outcomes, including continued receiver-selector evaluation,
but do not establish process integration behavior. Process source routes share
the resolver; no focused process test combining a queued directed TSO copy with
producer Resign or Connection Lost was found. The broad process probe's
directed-retraction case retracts its work before Resign and is not such
coverage. The full normative §4.4 text remains unavailable, so the guide states
implementation behavior only. No C++ files, builds, test runs, CI, Requirements
Lab, or 2010 documentation were changed.

Next handoff: do a bounded reader-gap audit of the 2025 Delay Subscription
Evaluation guide. Clarify deferred subscription/relevance evaluation versus
service validity and delivery guarantees; compare receive-order, timestamped,
directed, and process routes against existing focused tests; revise only a
demonstrated gap. Keep the development-profile boundary and 2010 stream
separate, and do not build, rerun tests, start CI, or edit the Requirements
Lab.

## 2026-10-14 — Delay Subscription Evaluation process-route review

The DSE guide already separates sender-side service validity, receiver
eligibility timing, and callback/grant timing. The focused receive-order,
timestamped, and directed cases found in this bounded audit are embedded
2025 tests; they show late-subscription admission and current-state
suppression for their covered paths. The normative §8.1.8 paraphrase remains
explicitly subject to verification against an authorized copy.

The reader gap was the process timing boundary. Process receive-order ordinary
and directed interactions re-resolve deferred candidates while preparing an
outgoing event, which is distinct from the embedded callback-entry check.
Process ordinary and directed TSO delivery use separate projection checks at
queued delivery. This survey found no focused process interaction or TSO DSE
case; a later 2026-10-16 test-tag audit found configured-process regional
attribute DSE cases that this search missed. Those tests do not establish the
process interaction/TSO paths or general process equivalence. The guide now
says which boundary each process citation supports. Mermaid source was not
changed in that increment. No build, test
rerun, CI, Requirements Lab, or 2010 documentation work was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Attribute Ownership
guide. Focus on the reader-visible boundaries between negotiated transfer,
ownership acquisition/divestiture, and update authority; compare the guide's
existing focused tests and source evidence, and revise only a demonstrated
gap. Keep the development-profile and 2010/2025 boundaries explicit; do not
build, rerun tests, start CI, or edit the Requirements Lab.

## 2026-10-14 — Attribute ownership and unpublication boundary

The bounded review found one reader-facing gap in the otherwise explicit
distinction between publication and per-instance ownership. In the current
embedded 2025 route, unpublishing an owned attribute set synchronously removes
the caller's owner entries and associated update metadata. A focused update
case accepts a value update before whole-class unpublication and observes
`AttributeNotOwned` on the next update. The guide now connects that concrete
transition to the conceptual distinction while labeling it as bounded source
and test evidence, not an exhaustive normative §5.3 interpretation or a 2010
claim. No Mermaid source, C++, build, test rerun, CI, or Requirements Lab
content changed.

Next handoff: do a bounded reader-gap audit of the 2025 Request Attribute
Value Update guide. Focus on whether an explicit value request changes
ordinary receiver eligibility for a later provider update; compare the
guide's source and focused tests with authorized §6.21 text. If the clause
text is unavailable, leave the portable HLA outcome open and document only
demonstrated implementation behavior. Preserve 2010/2025 and embedded/process
boundaries; do not build, rerun tests, start CI, or edit the Requirements Lab.

## 2026-10-14 — Request Attribute Value Update §6.21 entitlement review

The guide already separates provider solicitation from a later value update
and states that the update uses ordinary receiver planning. The bounded source
and test review supports that implementation description, but does not settle
the portable HLA entitlement question. The embedded role-overlap case
subscribes the requester through the public service before requesting and
later receiving the provider's separate reflection. The process object-request
case seeds subscription state directly in its fixture before the request and
also observes a later reflection; it does not test the public process
subscription route. The process class-request case does not test a later value
reflection. None isolates an otherwise-ineligible, unsubscribed requester.

The IEEE 1516.1-2025 public listing identifies Purchase and Access via
Subscription, while the checkout contains the API signature and exception
list but no full clause copy. Therefore §6.21 remains open; no normative
answer, process parity, or 2010 inference was added. The existing guide was
already appropriately cautious, so no guide or diagram edit was warranted.
No build, test rerun, CI, Requirements Lab, or C++ work was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Auto Provide guide.
Trace discovery-triggered provider work separately from explicit Request
Attribute Value Update, and compare the embedded discovery path with process
service/source and focused-test coverage. Keep process support unverified
unless a direct process call/test is found; preserve the separate 2010/2025
boundary. Do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Auto Provide: process discovery boundary — 2026-10-14

Traced the 2025 process discovery service through its callback bridge. The
service commits known-object state, can plan initial relevance advisories for
ordinary discovery, and separately plans initial values for RTI-owned MOM
objects; the callback bridge delivers `discoverObjectInstance`. A source
call-site scan found `planAutoProvideForDiscovery` only in the embedded
callback path. The process HLAsetSwitches handler and focused process test
establish switch mutation/state only; the test does not arrange discovery or
observe a provider solicitation. No focused process Auto Provide test was
found. Updated the guide to make these route and evidence limits explicit,
without making a normative claim or inferring 2010 parity. No diagram source
changed, so prior chart-render evidence is unaffected. No C++, build, test,
CI, or Requirements Lab work was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Federation
Synchronization guide, focusing on the state and observability path from
synchronization-point registration through announcement, participant
achievement, and completion/failure callbacks. Keep the 2010 stream separate,
distinguish 2025 embedded/process evidence, and do not build, rerun tests, start
CI, or edit the Requirements Lab.

### 2025 Federation Synchronization: process result evidence — 2026-10-14

Reviewed the existing guide's HLA-level sequence, embedded participant-set
state, late-join and partial-result traces, false-achievement semantics,
departure transition, and save/restore evidence. The concepts and diagrams
already make registration outcome, announcement, participant report, and
federation completion distinct; no extra chart was justified.

The paired configured-process public cases use an explicit receiver-only set.
They observe the registrant's successful registration result, duplicate-label
`synchronizationPointRegistrationFailed`, receiver announcement/tag, a
one-member `failedToSyncSet` after the receiver reports `false`, and
`SynchronizationPointLabelNotAnnounced` when the out-of-set registrant tries
to achieve. One uses HLA_EVOKED with polling and one HLA_IMMEDIATE with pushed
events. Since the registrant is excluded from the receiver set, neither case
establishes same-recipient registration/announcement ordering; they do not test
omitted-set expansion or prove profile parity. Clarified the evidence rows and
kept the standard boundary explicit. No diagram source was changed in this
increment. No build, test rerun, CI, Requirements Lab, or 2010 work was
performed.

Next handoff: do a bounded reader-gap audit of the 2025 Delay Subscription
Evaluation guide. Clarify deferred subscription/relevance evaluation versus
service validity and delivery guarantees; compare receive-order, timestamped,
directed, and process routes against existing focused evidence, revising
diagrams only for a demonstrated reader gap. Keep 2010 and 2025 separate,
distinguish embedded/process evidence, and do not build, rerun tests, start CI,
or edit the Requirements Lab.

### 2025 Federation and Federate Lifecycle: host-loss lifecycle boundary — 2026-10-16

The bounded lifecycle review found the guide's process endpoint section still
claimed there was no source caller of `ProcessFederationService::connectionLost`.
The host worker does call the session overload when its request loop exits and
attempts it on request-serving exceptions, then detaches and closes the
connection. Joined sessions enter registry loss cleanup and eligible survivor
effect dispatch; unjoined sessions need no membership cleanup. This is source
behavior, but this checkout has no non-test host construction site and the
focused host tests do not assert cleanup for a joined member or its timing. The
separate current-process fault fixture remains manually driven.

The guide now separates those host and fixture paths from client-local
ConnectionLost callback, buffered event, and queued callback state. The
final-member transport-loss test reconnects through the same RTI ambassador
object using a new connection, then joins as a fresh membership; it does not
resume the old session. Revised the existing process-loss diagram and evidence
map. Its fresh render and visual review are pending. No 2010 or normative
parity claim was inferred; no C++, build, test, CI, or Requirements Lab work
was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Delay Subscription
Evaluation guide. Clarify deferred subscription/relevance evaluation versus
service validity and delivery guarantees; compare receive-order, timestamped,
directed, and process routes against existing focused evidence, revising
diagrams only for a demonstrated reader gap. Keep 2010 separate, distinguish
embedded/process evidence, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Delay Subscription Evaluation: process attribute flow — 2026-10-14

The bounded DSE review found the existing HLA-level timing model and embedded
callback/grant diagrams sufficient, but the process attribute route deserved a
separate source map. For a route-only process receive-order candidate,
`process_federation_service_attribute_value_updates.cpp:201` re-resolves the
recipient before event construction while passing sent attribute handles only;
the retained source-region handles and snapshots are not passed to that lookup.
The queued TSO attribute route instead re-resolves at recipient dispatch with
the sent-region handles and snapshots
(`process_federation_service_tso_delivery.cpp:64`). The callback bridge
reflects the projected event (`process_federation_callback_bridge.cpp:361`),
without another DSE projection step. This is a source-observed difference and
an evidence limit, not a normative conclusion or confirmed defect. No focused
process-endpoint DSE test was found. Added a process attribute flowchart and
implementation links. Mermaid CLI's browser could not launch (exit code
2147483651), so the new chart has no syntax/render or visual verdict. No C++,
build, tests, CI, Requirements Lab, or 2010 work was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Object-Attribute
Subscription State guide. Trace inherited and regional subscription
declarations through their effects on active relevance, discovery/known-object
state, advisories, and callbacks. Compare existing diagrams with focused
source/test evidence, keeping declaration intent, update relevance, and object
knowledge distinct. Keep the 2010 and 2025 streams separate, distinguish
embedded/process evidence, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Delay Subscription Evaluation: nonregional process push boundary — 2026-10-16

The bounded audit found that the DSE guide already explains the HLA-level
separation between service validity, receiver eligibility, delivery timing,
and callback scheduling, with embedded/process evidence separated. It omitted a
specific process attribute boundary:
receive-order pull and regional push use the receiving-boundary recheck, while
nonregional receive-order push remains on the existing pre-event path pending
a separate DSE slice. An unprojected candidate is re-resolved before event
construction, not at the receiving boundary ([source scope note](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L210)).
A broader DSE test-tag search found a configured-process fixture missed by
filename-only discovery: its [regional attribute cases](../../cpp/tests/ieee1516_2025_connection_delayed_regional_attribute_update_catch2.cpp#L523)
cover HLA_EVOKED receive-fence and HLA_IMMEDIATE callback-task-admission
rechecks, plus an [in-progress reflection/Resign fence](../../cpp/tests/ieee1516_2025_connection_delayed_regional_attribute_update_catch2.cpp#L539).
No focused process test covers nonregional push, process interaction DSE, or
process TSO DSE.

Clarified the existing prose and labeled the existing chart, without adding a
duplicate diagram. The revised chart's fresh render and visual review remain
pending. No normative, process-parity, or 2010 claim was inferred; no C++,
build, test, CI, or Requirements Lab work was performed.

Next handoff: perform a bounded reader-gap audit of the 2025 Object-Attribute
Subscription State guide. Trace subscription declaration and mutation through
effective state, object knowledge/discovery, update relevance, advisories, and
callbacks; compare embedded/process evidence and keep declaration intent
distinct from DSE eligibility timing. Revise only for a demonstrated reader
gap. Keep 2010 separate; do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Object-Attribute Subscription State: empty-set unsubscribe boundary — 2026-10-16

The bounded reader-gap audit found the guide already explains per-attribute
ordinary and regional declarations, active relevance versus passive recipient
selection, discovery/known-object state, value projection, advisory effects,
class ancestry, and the distinction between discovery-time values and DSE.
The embedded/process comparison also clearly marks the unresolved ordinary
late-subscription value path.

One additional source-only profile seam was not documented: an empty set sent
to `unsubscribeObjectClassAttributes` is forwarded unchanged and removes no
selected ordinary entries in the embedded route, though the registry advances
the subscription generation when class state exists. The process client also
transmits an empty vector unchanged, but the process service expands it to the
currently stored ordinary subscription attributes for that class before
calling the registry; this expansion does not enumerate regional entries.
The pinned API header has a separate whole-class unsubscribe service but does
not define this empty-set interpretation in its declaration comments. A
bounded test scan found no valid-state public test comparing the behavior, so
the guide now records an open API/source/test question rather than asserting
normative meaning or profile parity. No extra diagram was warranted.

No 2010 behavior was inferred. No C++, build, test, CI, or Requirements Lab
work was performed.

### 2025 additional-module Join: regional interaction handoff reconciliation — 2026-10-16

The active handoff was already satisfied in the current tree: the DDM guide
contains the retained regional-interaction declaration trace, the FOM guide
links to it, and the earlier review ledger records the bounded comparison.
Rechecked the cited implementation path: additional-module Join reconciles
interaction, parameter, and dimension handle directories before committing
the replacement definition; per-federate declaration state remains separate;
regional pairs stay keyed by the supplied interaction-class handle; and later
recipient selection walks the sent class toward its parents with region
overlap checks. The focused Join test establishes new-class/dimension lookup,
while the focused regional send test subscribes and sends on the same class.
No test combines Join, a retained parent-keyed regional declaration, and a
later descendant send. The existing ancestry chart already explains the
send-time walk, so no duplicate chart was added.

Added a direct link from the ordinary interaction projection flow to the DSE
guide's existing object-state-versus-interaction-history comparison. This
helps readers distinguish a retained, previously sent interaction from
current object state exposed after discovery. No new normative or profile
claim was added. No 2010 behavior was inferred; no C++, build, test, CI, or
Requirements Lab work was performed.

Next handoff: perform a bounded HLA-first reader-gap audit of the 2025 Auto
Provide guide, focusing on the process-profile automatic-discovery path.
Compare process discovery/initial-value routing and switch handling with the
embedded provider-solicitation planner, and the exact focused-test evidence.
Revise only if the existing source/test boundary is unclear; do not infer
process parity from explicit Request Attribute Value Update. Keep 2010
separate; do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Auto Provide: process automatic-discovery evidence review — 2026-10-16

The bounded reader review found the guide already separates the embedded
discovery-triggered provider-solicitation planner from process discovery,
process MOM initial-value delivery, and the federation-wide switch. A source
call-site scan still finds the Auto Provide planner only on the embedded
callback route. The focused process switch case submits `false` then `true`
through `HLAsetSwitches`, but does not read the switch back, arrange discovery,
or assert a provider callback. The guide's existing statement that automatic
process-profile behavior is unverified remains appropriately bounded; the
explicit request route and shared switch do not establish parity.

Corrected the focused-test link and narrowed its description to what the case
actually observes. No duplicate diagram was warranted because the existing
guide already shows the distinction and labels the process path unverified.
No normative or 2010 claim was added; no C++, build, test, CI, or Requirements
Lab work was performed.

Next handoff: perform a bounded HLA-first reader-gap audit of 2025
object-instance deletion/removal across Receive Order, timestamped deletion,
and retraction. Reconcile object knowledge and removal callbacks, ownership
consequences, and timing gates against the existing object-information,
lifecycle, ownership, and time guides plus focused evidence. Revise only for a
concrete learner gap; avoid duplicate flows. Keep embedded/process and
2010/2025 boundaries separate; do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 object deletion and removal: timing-guide navigation review — 2026-10-16

The bounded cross-guide survey found the object-information guide already
contains separate flows for accepted receive-order deletion, recipient-local
removal commits, timestamped deletion, local forget, and process resignation
removals. It links the timestamped path to the time grant-boundary matrix and
the deletion-specific TSO retraction branch. The ownership guide separately
shows stale ownership work invalidated by accepted deletion, while the time
guide and TSO retraction guide explain generic queue phases and service-specific
retraction respectively. Source and focused-test references maintain the
important boundary: restoration after a removal callback begins is demonstrated
only for the embedded path; the process timestamped-delete test does not prove
that restoration behavior.

The remaining reader gap was a missing route from the timing guide's TSO
service-family list to Delete-specific state effects. Added a concise
cross-link to the object-removal and deletion-retraction sections, explicitly
labeling those as current Umbra 2025 details and keeping embedded/process and
2010 separate. No duplicate diagram was warranted. No build, test, CI, or
Requirements Lab work was performed.

Next handoff: perform a bounded HLA-first reader-gap audit of 2025 Save/Restore
interactions with timestamped object deletion and retraction. Compare
quiescence and captured/restored state claims with the existing save/restore,
time, object-removal, and retraction guides plus focused evidence; do not imply
every pending TSO or retraction handle is serialized. Add navigation or
clarify a flow only for a demonstrated learner gap. Keep embedded/process and
2010/2025 boundaries separate; do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Save/Restore: timestamped object deletion and retraction review — 2026-10-16

The Save/Restore guide already contains separate worked cases for a queued
timestamped object deletion returning through a fresh-registry restore and for
retracting that restored deletion after one of two receivers has observed
removal. The latter test checks that the observed receiver regains object
lookup and receives Request Retraction while the other receiver's pending
removal is suppressed. The public tests exercise both callback models against
new embedded registries in one process; their `process-restart` tags do not
mean an OS-process restart. Internal filesystem state-image evidence is
separate from public process-endpoint behavior. No extra Save/Restore diagram
was warranted.

Added a direct link from the object-removal guide to both Save/Restore cases and
made the fresh-registry/process-restart distinction explicit there. The source
and focused tests support the existing guide's profile boundary; no process
or 2010 parity is inferred. No C++, build, test, CI, or Requirements Lab work
was performed.

Next handoff: perform a bounded HLA-first reader-gap audit of the 2025
Federation MOM guide's execution-scoped `HLAfederation` and member-scoped
`HLAfederate` state across Save/Restore and Resign/rejoin. Compare field
ownership, report/update timing, removal and reinitialization, and focused
evidence with the lifecycle and Save/Restore guides. Revise only for a
demonstrated learner gap; do not infer process parity or 2010 behavior. Do not
build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Federation MOM: state ownership across save and membership lifetimes — 2026-10-16

The bounded review found the two MOM projections and their transition evidence
already documented in separate guides. The federation guide maps
`HLAfederatesInFederation` on Join/Resign and the federation-scoped next/last
save values; the joined-federate guide maps each member object's
`HLAfederateState` through `1 → 3 → 1` for Save and `1 → 5 → 1` for Restore,
then tracks pending removal and fresh instance identity after rejoin. Focused
embedded tests cover the membership list, save conditionals, member state, and
member-object removal/rejoin. Process evidence covers member-object removal
before rejoin discovery/reflection, but not the Federation membership-list
reflection or the `HLAfederateState` Save/Restore projection; the guides mark
those gaps rather than infer parity.

Added direct links from the federation-scoped guide to the member object's
resign/rejoin section and to the member state / process-evidence sections for
Save/Restore. The existing diagrams already express the separate lifetimes and
observation paths, so no combined chart was warranted. No normative, process,
or 2010 behavior was broadened; no C++, build, test, CI, or Requirements Lab
work was performed.

Next handoff: retry local Mermaid syntax/render and visual review for the
revised 2025 FOM post-Join chart in the FOM admission/composition guide. Inspect
the rendered layout and record the exact verdict; if browser launch still
fails, keep visual validation pending rather than changing chart semantics to
suit the renderer. Preserve 2010/2025 boundaries; do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 Auto Provide: process-profile evidence correction — 2026-10-16

The bounded source scan found `planAutoProvideForDiscovery` is called only on
the embedded discovery callback route. The process discovery service commits
known-object state and emits a discovery event; ordinary discovery may plan
initial relevance advisories, while a separate RTI-owned MOM branch plans
initial values. Neither is a process provider solicitation. The process
HLAsetSwitches case sends `false` and then `true` through the public service;
its server-side fixture asserts the internal registry value is `true` after
Join, `false` after disabling, and `true` after re-enabling. It does not call
the public switch getter, arrange discovery, or assert a provider callback.
Corrected the guide's evidence table and profile-boundary summary; no normative
or 2010 claim was added.

### 2025 additional-module Join and regional interactions — 2026-10-16

The DDM guide already maps current interaction-dimension context, exact-class
regional declaration storage, and the later sent-class-to-parent walk with
region-overlap checks. The FOM guide cross-links that trace. Focused embedded
evidence separately covers new class/dimension lookup and a regional send on
the same class; neither combines Join, a retained parent-keyed regional
subscription, and a later added-descendant send. Added a process-profile source
mapping: configured Join passes its prepared definition to the shared
registry, regional subscription uses the same registry declaration methods,
and receive-order send delegates to the same planner. The reviewed public
process fan-out case joins without extra FOM modules, so the combined route
remains unverified end to end. Added this mapping to the DDM guide; no duplicate
diagram, process-parity, normative callback, or 2010 claim was added.

The revised FOM post-Join diagram now has a syntax-only Mermaid
`flowchart-v2` parse; Chrome/Edge CLI rendering still fails before image
output, so visual review remains pending. No C++ source, tests, builds, CI, or
Requirements Lab content changed.

Next handoff: perform a bounded reader-gap audit of the 2025 Delay Subscription
Evaluation guide. Clarify deferred subscription/relevance evaluation versus
service validity and delivery guarantees; compare receive-order, timestamped,
directed, and process routes against existing focused evidence. Revise diagrams
only for a demonstrated reader gap. Keep 2010 and 2025 separate, distinguish
embedded/process evidence, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Delay Subscription Evaluation: receive-order temporal gate — 2026-10-16

The bounded reader-gap review found that the DSE guide already separates
service validity, receiver-eligibility timing, receive-order/TSO delivery, and
embedded/process evidence. One phrase risked making callback model appear to
be the only receive-order gate. Added a concise explanation and cross-links to
the time-management guide and its focused asynchronous-delivery temporal-state
and TSO-exclusion tests: in the embedded 2025 profile, a time-constrained
receiver's receive-order work can wait while Time Granted until asynchronous
delivery is enabled or a time advance is pending; asynchronous delivery does
not release TSO early. These are adjacent time-management cases, not DSE test
cases or process evidence. The time guide already diagrams the gate, so no
duplicate DSE chart was warranted. No normative §8.1.8 wording, process parity,
or 2010 behavior was inferred. No C++, build, test rerun, CI, or Requirements
Lab work was performed.

Next handoff: retry local render and visual review of the revised 2025 FOM
post-Join chart. Its syntax-only Mermaid check passed, but the previous
Chrome/Edge CLI attempts failed before producing an image. Record the exact
result and leave visual review pending if the browser cannot launch. Preserve
the 2010/2025 boundary; do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 FOM render retry and Request Attribute Value Update entitlement audit — 2026-10-16

Retried the revised FOM post-Join chart with Mermaid CLI 12.0.0. Default
browser launch failed before image output with code `2147483651`; system Chrome
with a fresh temporary profile also failed before output, reporting an already
running profile and `The user name or password is incorrect`. The prior
syntax-only `flowchart-v2` parse remains the only current validation, so visual
review stays pending and chart semantics were not changed.

The bounded §6.21 review confirmed that the request guide's entitlement
question remains open: its selected embedded requester-as-receiver case has a
subscription, while the process case seeds subscription below the public API;
neither tests a provider response to an unsubscribed requester. The vendored
API declaration gives the signature and exception list, not the clause prose.
The official IEEE product page offers purchase/subscription access, and its
linked ZIP was not inspectable in the browser reader. No authorized clause text
was reviewed, so no normative entitlement rule or guide change was warranted.

Next handoff: review whether the current 2025 mixed-order attribute-passel
scenario exposes a reader gap between per-attribute order and one service
invocation's delivery timing. Use stable source and already-verified cases;
the newly untracked test source is not a passing result. Keep any addition
bounded and preserve edition/profile distinctions. Do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 mixed-order timestamped attribute passels — 2026-10-16

Added a source-derived flow to the 2025 time guide for one embedded
timestamped `Update Attribute Values` call whose per-attribute effective order
produces both receive-order and timestamp-order passels for the same
time-constrained receiver. The chart makes their separate asynchronous/advance
and logical-time gates visible, preserves the shared timestamp/tag, and notes
that retraction can withdraw queued TSO work but cannot undo an RO callback
already observed. The object-information guide now links to this timing view.

The trace follows the registry's per-attribute passel grouping, the adapter's
per-recipient RO/TSO split, the existing receive-order gate, and the TSO
reflection-before-grant dispatch. Checked-in cases cover order controls,
per-recipient timestamped fan-out, and an interaction's receive-order temporal
gate separately; no tracked focused case combining mixed attribute orders for
one receiver in one call was found. This addition is not a normative HLA rule,
process-profile evidence, or evidence about 2010. The new Mermaid chart passed
a syntax-only `flowchart-v2` parse through the local Mermaid core parser using
a Node-only no-op DOMPurify adapter. It was not rendered or visually reviewed;
no C++ build, test, CI, or Requirements Lab work was done.

Next handoff: survey the 2025 process endpoint's timestamped attribute-update
route for per-attribute effective order and compare only stable source and
existing checked-in evidence with this embedded flow. Document a boundary
difference only if directly supported; do not infer parity. Do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 process endpoint mixed-order attribute passels — 2026-10-16

The process endpoint uses the same registry passel planner, but routes a
timestamped call's passels across different process boundaries. For a
time-constrained receiver, the RECEIVE passel becomes a normal process
attribute event carrying the call's timestamp and tag without retraction
identity; push mode sends an event frame, while pull mode holds it for the
receiver's poll. TIMESTAMP passels instead remain in the per-recipient TSO
message until the logical-time delivery boundary, then cross as events with
the message identity and remain in transit until callback acknowledgement.
The callback bridge maps the two process event forms to RECEIVE/RECEIVE and
TIMESTAMP/TIMESTAMP respectively. A source-linked flow was added to the 2025
time guide and cross-linked from the object-information guide.

The checked-in process TSO scenarios verify the timestamped reflection before
the matching grant for a single TSO attribute, but no tracked focused test was
found combining both attribute orders for the same constrained receiver.
Neither those tests nor the reviewed process RECEIVE event path establishes
when that RO passel is eligible for a Time-Granted receiver with asynchronous
delivery disabled or enabled; this source route does not check the embedded
`FederateTimeState` gate. The guide records this as an open process-profile
timing question and does not infer embedded/process, normative, or 2010
equivalence. The new process chart passed a syntax-only Mermaid parse; local
render and visual review remain pending. No C++ build, test, CI, or Requirements
Lab work was done.

Next handoff: review process-endpoint receive-order attribute callback
eligibility for a time-constrained, Time-Granted receiver with asynchronous
delivery off/on and with a time advance pending. Reconcile the process event
route with existing source and checked-in cases; leave the exact mixed-passel
timing open unless directly established. Do not build, rerun tests, start CI,
or edit the Requirements Lab.

### HLA guide local-link integrity audit — 2026-10-16

Scanned all 43 Markdown files under `docs/design`, excluding fenced code, for
local inline Markdown destinations. All 3,453 local link targets resolve, and
all 2,587 GitHub `#L...` line anchors fall within the referenced files. This
does not validate external URLs, reference-style link definitions, heading
fragments, rendered diagrams, or GitHub presentation. No broken file or line
target was found, so no link repair or diagram change was needed. No C++,
build, test, CI, 2010, or Requirements Lab work was performed.

Next handoff: continue the bounded non-timing reader-gap audit in the roadmap,
selecting a guide-specific HLA question not covered by the recently completed
reviews. Add or revise a diagram only when source, focused tests, and the
existing guide establish a real reader gap; keep unresolved profile and
standards-text questions explicitly open. The process DDM sequence remains
first in the visual-review queue when local browser launch becomes available.

### 2025 per-recipient interaction order-conversion example — 2026-10-16

The delivery guide previously distinguished sent and received order fields
without showing why a single send can produce different received-order reports
at different recipients. Added a bounded sequence to the 2025 Order and
Transportation guide: one embedded `HLA_EVOKED` timestamped interaction reaches
an unconstrained receiver as RECEIVE and a time-constrained receiver as
TIMESTAMP after its advance, while both report sent order TIMESTAMP and a valid
retraction designator. The cited focused test checks the payload, tag,
transportation, logical time, callback timing, and both order fields; the
implementation mapping links the recipient split and receive-order callback
path. This is not a complete conversion matrix, a normative interpretation of
§8.1.4 Table 3, process-profile evidence, or 2010 behavior.

The new sequence passed Mermaid 12.0.0 syntax parsing through the local core
parser with an in-memory no-op DOMPurify adapter. It was not rendered or
visually reviewed; selected render checks remain pending after Windows denied
browser launch. A follow-up local-link scan across all 43 `docs/design`
Markdown files validated 3,459 inline destinations and 2,593 GitHub line
anchors with no missing targets or out-of-range anchors. This does not validate
external URLs, heading fragments, rendering, or GitHub presentation. No C++
build, test, CI, Requirements Lab, or 2010 work was done.

Next handoff: continue the bounded non-timing reader-gap audit already listed
in the guide backlog. For each candidate, compare the HLA reader question with
existing guide coverage and checked-in source/tests; write a focused
explanation or record a no-change disposition. Keep profile, edition, and
normative-source boundaries explicit. The process DDM sequence remains first
in the visual-review queue when browser launch becomes available.

### 2025 saved object-name reservation ownership across restore — 2026-10-16

A bounded non-timing review found a useful detail missing from the existing
reservation state chart: whether a saved unregistered name remains bound to its
original federate after restoration. Added a sequence to the reservation guide
and an evidence cross-link in the Save/Restore guide. The checked-in public
`HLA_IMMEDIATE` case records the name/federate identity in a filesystem-backed
image, tears down the source registry and execution, creates a fresh registry
and execution in the same test process, rejoins both federates with the same
handles, and restores. The peer cannot release the reservation; its attempted
reservation produces the failure report. After the owner releases, the peer's
reservation succeeds and the former owner can no longer release it.

Registry source captures and validates the saved federate ID, and the focused
test checks the public API results. This is an embedded implementation/test
observation, not a general normative rule, OS-process restart proof, process-
endpoint result, or 2010 behavior. The new sequence passed Mermaid 12.1.0
syntax parsing; local render and visual review remain pending. The whole-tree
syntax scan now accepts 305 diagrams across 43 Markdown files. A post-edit
local-link scan validated 3,467 inline destinations and 2,600 GitHub line
anchors across the 43 guides with no missing targets or out-of-range anchors.
No C++ build, test, CI, Requirements Lab, or 2010 work was performed.

### 2025 discovery and first-reflection review — 2026-10-16

The bounded review found the existing Object and Interaction Information and
Object Attribute Subscription State guides already explain the reader-facing
distinction, including the embedded late-subscription path, the separate
process MOM initial-value event, and the unresolved ordinary process case.
Embedded lifecycle code performs discovery before planning current subscribed
values; the focused embedded late-subscription test observes discovery and a
value reflection but stores them separately, so it does not establish relative
callback order. The process discovery enqueue path creates its separate initial-
value event only for RTI-owned MOM objects. The public process discovery test
asserts discovery, while the process update test exercises a later explicit
update; neither proves an ordinary initial-value snapshot or callback ordering.

The pinned API headers provide callback and service signatures/clause labels,
not the normative prose needed to decide the standard's exact ordering rule.
No duplicate diagram was warranted, and no normative conclusion was inferred.
The late-subscription process behavior and standard-text question remain open.
Evidence: [object-information first-reflection discussion](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#first-reflection-after-discovery-order-and-evidence-boundary), [subscription profile comparison](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge), [embedded callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L322), [process MOM-only initial-value branch](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L95), [embedded test](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180), and [process explicit-update test](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457).

This was documentation/source/test inspection only; no build, test rerun, CI,
Requirements Lab, or 2010 behavior work was performed.

Next handoff: audit the 2010 Reference RTI synchronization-point barrier using
only its separate 2010 guide, `cpp/src/internal/runtime/reference_2010.cpp`,
and `cpp/tests/ieee1516_2010_binding_shell_smoke.cpp`. Identify which
participant-state and callback transitions are exercised versus source-only;
add or clarify a separate 2010 explanation only if a reader gap is demonstrated.
Do not borrow 2025 behavior or imply edition parity. The process DDM sequence
remains first in the visual-review queue when browser launch becomes available.

### 2010 synchronization-point barrier coverage review — 2026-10-16

The existing [2010 barrier section](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md#4-synchronization-point-barrier)
already maps registration, captured participants, announcement, achievement,
completion, and the important source-only failure/retry effects. Its flowchart
and sequence are explicitly scoped to `ReferenceRuntime2010`; the prose
distinguishes source observations from normative claims and says the relevant
branches are not test-confirmed. In the bounded [2010 reference source](../../cpp/src/internal/runtime/reference_2010.cpp#L114)
and [achievement path](../../cpp/src/internal/runtime/reference_2010.cpp#L164),
the point is stored before callbacks, the set is fixed at registration, and
completion erases only after all selected callbacks return. The [2010
binding-shell smoke](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp)
contains no synchronization-point service or callback case; it therefore
does not verify any barrier transition. No more portable or test-backed rule
can be added from these sources, and no separate 2010 chart is warranted.

This was a bounded documentation/source/test survey only; no build, test run,
CI, 2025 behavior, or Requirements Lab work was performed.
The follow-up local-link scan after recording both 2010 reviews checked 3,496
inline destinations and 2,617 GitHub line anchors across 43 design Markdown
files, with no missing targets or out-of-range line anchors.

Next handoff: audit the separate 2010 distinction between usable logical-time
values and an RTI that actually provides time-managed services. Compare only
the 2010 reference guide's time section, the separate 2010 DataElement/encoding
guide, `reference_2010.cpp`, and the 2010 integer-time, float-time, and time-
marshalling smoke tests. Clarify only a demonstrated reader gap; do not infer
2025 behavior or edition parity. The process DDM sequence remains first in the
visual-review queue when browser launch becomes available.

### 2010 logical-time values versus RTI time services review — 2026-10-16

The existing [2010 time-boundary section](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md#5-timing-values-are-not-time-managed-execution)
already separates the abstract value/factory API from the reference profile's
unimplemented time-management services, and its diagram keeps those two layers
visibly apart. The integer and float implementations provide value algebra and
serialization; the integer, float, and marshal smoke programs exercise factory
selection, initial/final/epsilon values, arithmetic, encode/decode, and selected
invalid-buffer handling. They do not call ambassador services. The [generated
2010 service overrides](../../cpp/generated/rti_ambassador_shell_2010.hpp#L325)
route time-management methods to the shared [`unavailable()` implementation](../../cpp/generated/rti_ambassador_shell_2010.hpp#L691),
which throws `RTIinternalError`; timestamped service overloads follow the same
profile boundary. `ReferenceRuntime2010` does not wire federation creation to
the time factory. The guide already labels those effects as implementation
evidence, not a general 2010 RTI rule, and identifies the absence of runtime
service tests. No additional diagram or clarification was warranted.

Evidence: [integer value/factory implementation](../../cpp/src/ieee1516_2010_integer_time.cpp#L128),
[float value implementation](../../cpp/src/ieee1516_2010_float_time.cpp#L109),
[integer smoke](../../cpp/tests/ieee1516_2010_integer_time_smoke.cpp),
[float smoke](../../cpp/tests/ieee1516_2010_float_time_smoke.cpp),
and [time marshal smoke](../../cpp/tests/ieee1516_2010_time_marshal_smoke.cpp).
This was documentation/source/test inspection only; no builds, tests, CI,
2025 behavior, or Requirements Lab work were performed.

Next handoff: audit the separate 2010 Join/Resign membership lifecycle against
the 2010 reference guide's connection/membership section, `reference_2010.cpp`,
and `ieee1516_2010_binding_shell_smoke.cpp`. Identify which membership and
callback transitions are directly exercised versus source-only, and revise
only if a reader-facing HLA distinction is missing. Do not infer 2025 behavior
or edition parity. The process DDM sequence remains first in the visual-review
queue when browser launch becomes available.

### 2010 Join/Resign membership evidence audit — 2026-10-16

The separate 2010 guide already has distinct ambassador and execution
membership state diagrams, plus source-profile explanations of Join guards,
ResignAction handling, and owned-object cleanup. I added a concise smoke-evidence
boundary to make clear what the connected [test prefix](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp#L99),
[two named joins](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp#L124),
[member lookup](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp#L158),
and [NO_ACTION resign/cleanup sequence](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp#L195)
actually exercise. The smoke does not test disconnect while joined, duplicate
or invalid Join, destruction while members remain, alternate ResignAction
effects, or object-removal callbacks. Those are source-only reference-profile
observations, not tested HLA behavior. The existing diagrams already explain
the needed HLA distinction, so no additional diagram was added.

This was a bounded documentation/source/test inspection; no build, test run,
CI, 2025 behavior, or Requirements Lab work was performed.

### 2025 process Attribute Scope: queued stale-work boundary — 2026-10-16

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
interleaving remains untested by that public case.

This documentation review is limited to IEEE 1516.1-2025 evidence; no build,
test run, CI, or Requirements Lab work was performed.

Next handoff: continue the 2025-only evidence-gated closeout. Revisit the
process RECEIVE temporal gate only when a focused durable 2025 case or other
direct evidence changes the current source/test boundary. Until then, keep the
outcome unresolved and do not add an unverified diagram transition.

### 2025 active-establisher/passive-receiver subscription review — 2026-10-16

The existing 2025 object-attribute guide already explains that an active
declaration by a joined, non-producing member can establish relevance while a
separate passive receiver still needs its own matching declaration and
class/region eligibility. The discovery resolver and value recipient planner
support this source-derived distinction. The focused object-attribute tests
cover passive-only suppression and one subscriber changing its own active and
passive modes; neither covers a separate active establisher plus passive
receiver. Passive-interaction tests are a separate service and are not evidence
for this object-attribute case. The guide already records these boundaries, so
no chart or prose edit was warranted. No build, test, CI, or Requirements Lab
work was performed.

Next handoff: audit the 2025 synchronization scenario in which a new member
joins after partial achievement. Compare the current guide's source trace with
the focused checked-in test coverage; do not turn source-only ordering into a
normative HLA rule.

### 2025 partial synchronization achievement before late Join review — 2026-10-16

The existing synchronization guide already includes the source-derived path:
an expandable Join grows the pending participant set without replacing prior
achievement results, and later reports are compared with the new set size. The
direct registry test joins before any achievement; the public HLA_EVOKED and
HLA_IMMEDIATE scenarios also join the newcomer before the first report for that
point. No focused test covers partial achievement followed by late Join. The
guide's caveat was accurate, but three stale registry/Join-hook source links in
its evidence table were repaired to current implementation lines. No diagram
or behavior change, build, test, CI, or Requirements Lab work was needed.

Next handoff: check whether new checked-in 2025 evidence exists for the process
RECEIVE temporal gate beyond the route difference already documented. If none
exists, preserve the unresolved outcome and move to another bounded 2025 reader
gap; do not infer embedded/process parity.

### 2025 process RECEIVE temporal-gate evidence recheck — 2026-10-16

A fresh C++ test search for the asynchronous-delivery services found no
process-endpoint fixture invoking them. The current time guide already records
the embedded receive-order gate, the process ordinary-event route, and the
limits of the public process RECEIVE and timestamped-before-grant tests. No new
evidence resolves process callback eligibility, and the existing explanation
is sufficient; no chart or prose change was warranted. No build, test, CI, or
Requirements Lab work was performed.

Next handoff: audit 2025 Request Attribute Value Update eligibility across the
pinned API, embedded/process source paths, and focused tests. Add HLA-first
explanation only where the bounded evidence supports it; leave normative
requester entitlement unresolved without the relevant standard prose.

### 2025 Request Attribute Value Update evidence-map link audit — 2026-10-16

The current request guide already contains a bounded direct-requester review.
A separate link-content pass found and corrected stale references to the
process MOM handler, process class request case, process subscription fixture,
request/Provide checkpoints, current-owner projection, and runtime delivery
gate. The corrected anchors were checked against their current source/test
lines; no behavior, diagram, or normative claim changed. No build, test, CI, or
Requirements Lab work was performed.

Next handoff: audit 2025 object-class request fan-out across multiple current
attribute owners. Confirm the existing guide clearly distinguishes requester-
owned attributes, provider callbacks, and later reflection; retain the
one-object/multiple-remote-provider test gap.

### 2025 process timestamped-deletion callback identity trace — 2026-10-16

The process source records the deleting requester in the outbound TSO message,
then constructs the delivered removal event from the registry snapshot's
stored object producer and maps that identity into the public callback. The
focused process fixture registers and deletes through one sender, so it cannot
distinguish registrant from a different delete-privilege owner. The ownership
guide now records this source path and its test limit separately from the
embedded transfer scenario. No extra diagram was justified; no build, test,
CI, or Requirements Lab work was performed.

Next handoff: review the 2025 active-establisher/passive-receiver subscription
scenario against its focused source and tests. Add or revise guide material
only if the evidence establishes a useful distinction; otherwise record a
bounded no-change disposition. Keep the process regional-attribute DDM chart
first in the visual-review queue and wait for browser-launch access to change.

### 2025 object-class Request Attribute Value Update fan-out review — 2026-10-16

The 2025 guide already explains class expansion across instances and grouping
requested attributes by each instance's current owner. The source implements
that grouping; embedded and process focused tests each cover two instances
routed to one provider, not a single instance split across multiple providers.
The guide accurately calls that missing case source-derived and separates
requester-owned attributes, provider callbacks, and later reflection. No
diagram or prose change was needed. No build, test, CI, or Requirements Lab
work was performed.

Next handoff: audit the 2025 embedded class-request owner-change boundary
between provider planning and callback dispatch. Verify the pending
requester/provider match, current-owner projection, and empty-result
suppression against focused tests, while keeping the missing ownership-change
scenario explicit. Do not infer process-profile or normative retargeting
behavior.

### 2025 embedded class-request ownership-change boundary review — 2026-10-16

The embedded class request has a full-set owner check when each planned
provider/object group is persisted, followed later by callback-entry identity
matching and current-owner projection. A stale registration raises
`RTIinternalError` before the successful service report or callback queueing;
groups are persisted individually and the source path shows no rollback if a
later registration fails. This is a potential source-level atomicity risk, not
an HLA rule or verified runtime result. The registry restore test uses the
original owner; the adjacent process instance case blocks an already-entered
Provide callback while an acquisition is cancelled, so it does not test a
completed transfer or the embedded class path. The guide chart now depicts
both checkpoints and passed Mermaid 12.1.0 syntax parsing. No render, visual
review, build, test, CI, or Requirements Lab work was performed.

### 2025 pending class-level AVU restore evidence review — 2026-10-16

Compared the public embedded class-request restore case with the internal
regional class-request state-image test. The public case retains its original
provider and verifies the callback follows `federationRestored`, preserves the
request tag, and is not duplicated under both callback models; it does not
exercise ownership change or process-endpoint delivery. The internal case
restores the regional class/attribute/region mapping and tag, consumes an
eligible route once, suppresses stale work after committed disjoint region
bounds, and retires pending provider work on provider resignation. Its
requester-departure assertion is confounded because provider resignation
occurs first. The test creates a fresh registry in the same test process, not
an OS-process restart. Updated the AVU and Save/Restore evidence maps; no new
diagram was warranted. No build, test rerun, CI, or Requirements Lab work was
performed.

Next handoff: audit the 2025 Join-with-additional-FOM-modules path in the FOM
Admission and Composition guide. Reconcile model replacement, member
admission, phase-specific failures, and observer effects against focused
source/tests; keep normative and implementation claims separate. The process
regional-attribute DDM sequence remains first in the visual-review queue, and
the new process scope-event chart awaits rendering behind it.

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

The existing [subscription-state profile chart](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge)
already distinguishes the embedded stored-value path from process MOM-only
initial events and leaves the ordinary process outcome open; the
[object-information guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#first-reflection-after-discovery-order-and-evidence-boundary)
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
([test](../../cpp/tests/libxml2_fom_composer_catch2.cpp#L1291)); the runtime
codec case builds its map directly and asserts octets
([test](../../cpp/tests/ieee1516_2025_basic_data_elements_catch2.cpp#L994));
the configured-process Create case accepts the Restaurant FOM but does not
exercise variant encoding
([test](../../cpp/tests/process_federation_additional_fom_catch2.cpp#L363)).
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
[2025 attribute-value update request guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#configured-process-path-region-check-before-event-routing).
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
[2025 ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md). It already
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
the [2025 request guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md)
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

The [2025 scope guide](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md)
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
existing [Federation MOM guide](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#2025-process-profile-federation-mom-after-an-additional-module-join)
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
[Federation MOM guide](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#2025-process-profile-membership-list-updates-at-join-and-resign)
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
