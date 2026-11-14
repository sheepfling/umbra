# Progress History Archive — 2026-10-16 (part 1)

Detailed entries are indexed in the [historical progress index](../HLA-BEHAVIOR-FLOW-GUIDES-PROGRESS-LOG.md).

---

## 2025 process Send Interaction failure mapping — 2026-10-16

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

The existing [object-attribute subscription guide](../HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge)
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

The existing [Auto Provide guide](../HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md) already
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
([registry](../../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L261)).
The discovery planner skips instances already known to the receiver
([planner](../../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L83)),
while the embedded ordinary initial-value planner runs after discovery enters
user code
([callback path](../../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L375)).
The focused unknown-to-known test asserts discovery and the selected stored
value
([test](../../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180)).
The additive-subscription test instead changes per-attribute modes after
discovery and checks a later publisher update, not an intervening value-only
callback
([test](../../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L355)).
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

The [focused test](../../../cpp/tests/timestamped_default_region_attribute_mixed_fanout_catch2.cpp#L155)
asserts those outcomes and the shared valid retraction designator; the
[attribute adapter](../../../cpp/src/internal/runtime/umbra_rti_ambassador_attribute_update_services.cpp#L637)
shows the constrained-recipient queue selection. The existing [TSO
retraction-guide mixed-recipient timeline](../HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#a-mixed-recipient-walkthrough)
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
([test](../../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L134)),
registered-object update association
([test](../../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L221)),
callback-delayed association release
([test](../../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L369)),
and registry usage guard/ledger
([delete guard](../../../cpp/src/internal/federation/federation_registry_regions.cpp#L320),
[usage ledger](../../../cpp/src/internal/federation/federation_registry_regions.cpp#L732)).
The same test confirms a stale-handle query fails after deletion
([assertion](../../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L161)).

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
[object/interaction guide](../HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#evidence-map-implementation-versus-exercised-scenarios).

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

Reviewed the current [DDM and regions guide](../HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
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
[interaction review ledger](../HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-ordinary-versus-directed-interaction-mental-model--2026-10-11)
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
focused throwing-callback test is absent. See the [review record](../HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-callback-exception-versus-evoke-service-exception-surface--2026-10-11).
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
crash-recovery, or 2010 parity claim. See the [review record](../HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-save-restore-in-flight-work-comparison--2026-10-11).
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
[review record](../HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-five-diagram-render-verification--2026-10-11).
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
[review record](../HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-object-information-lifecycle-audit--2026-10-11).
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
