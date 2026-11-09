# IEEE 1516.1-2025 Documentation Progress History — 2026-11-07

[History index](HLA-2025-DOCUMENTATION-PROGRESS-HISTORY.md) · [active roadmap](HLA-2025-DOCUMENTATION-ROADMAP.md)

## 2025 chart-level visual-review ledger, increment 81 — 2026-11-07

Created `HLA-2025-DIAGRAM-VISUAL-REVIEW-LEDGER.json` with one entry per
current Mermaid body. Each entry records its source path, line, ordinal, nearest
section heading, fence style, Mermaid type, and SHA-256 body hash. All 478
entries have current local-render evidence. The nine current tilde-fenced charts
with explicit screenshot evidence are marked confirmed; the remaining 469 are
marked `historical-status-unreconciled`, because the earlier 236/463 aggregate
does not identify chart bodies. That status means identity is unresolved, not
that a chart was not reviewed. The ledger distinguishes 9/478 (1.9%) directly
confirmed by current body identity from the unresolved historical reviews.
GitHub rendering remains unverified. No tests, builds, CI, Requirements Lab, or
2010 work.

**Next handoff:** Map chart-specific visual-review notes in the dated archive
onto ledger entries using source guide, section heading, and body identity.
Promote only unambiguous matches; where a note cannot identify a current body,
keep it unresolved and screenshot-review the next coherent guide batch. Refresh
the ledger whenever a Mermaid body changes or is added.


## 2025 historical visual-review identity reconciliation, increment 82 — 2026-11-07

Mapped six additional screenshot-reviewed chart identities from roadmap
increments 68, 70, 71, 75, and 40 into the current-body ledger: Connect
validation (1), embedded and configured-process Join/discovery sequences (2),
the Join discovery exception flow (1), the configured-process FQR sequence
(1), and the Federation MOM conditional-reflection flow (1). Each mapped
current body matches the later 477-body render snapshot, which confirms that
the reviewed source was not subsequently changed before the current inventory
reconciliation. The ledger now has 15 explicitly identified current screenshot
reviews out of 478 bodies (3.1% by confirmed identity). The older 236/463
aggregate remains unmapped; 463 ledger rows therefore retain the status
`historical-status-unreconciled`, which is not equivalent to unreviewed. All
478 current diagrams have local render evidence; GitHub rendering remains
unverified. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Continue mapping named review notes from the dated roadmap
and archive onto current ledger identities, checking body hashes against the
render snapshots. Promote only exact matches. Keep ambiguous historical reviews
unreconciled and screenshot-review a coherent guide batch to increase the
current-body confirmed count.


## 2025 Authorization guide screenshot review, increment 83 — 2026-11-07

Rendered and screenshot-reviewed all seven current diagrams in the 2025
Authorization and Credentials guide with Mermaid CLI 12.0.0. All labels and
branches are visible without clipping. The configured-process authorization
flow (2,561 px high) and authorizer-exception flow (1,795 px high) are tall,
but traceable at the rendered width; retain them pending an actual GitHub-size
review rather than splitting on height alone. The rendered inputs match the
source body hashes in the chart-level ledger. Confirmed identity coverage is
now 22/478 (4.6%); 456 other entries retain historically unresolved status,
not a claim that they were never reviewed. No tests, builds, CI, Requirements
Lab, or 2010 work.

**Next handoff:** Screenshot-review the four current 2025 Ownership, Persistence,
and Resignation diagrams as one thematic batch. Record only current-body hash
matches, note any chart that is hard to follow at full width, and keep the
historical 236/463 aggregate separate until its identities can be recovered.


## 2025 Ownership guide screenshot review, increment 84 — 2026-11-07

Rendered and screenshot-reviewed all four current diagrams in the 2025
Ownership, Persistence, and Resignation guide with Mermaid CLI 12.0.0. The two
dense sequences and two decision flows are visible without clipping; the
477-pixel-wide restore/resignation decision chart remains legible while
scrolling. All four inputs match the current source-body hashes in the visual
ledger. Confirmed current-body screenshot reviews now total 26/478 (5.4%); 452
entries retain historically unresolved status, not a claim that they were never
reviewed. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review the four current 2025 Delay
Subscription Evaluation diagrams as a coherent batch. Record source-hash
identity and any readability issue; do not infer process/embedded equivalence
from visual similarity. Continue keeping the historical 236/463 aggregate
separate until individual chart matches are established.


## 2025 Delay Subscription Evaluation guide screenshot review, increment 85 — 2026-11-07

Rendered and screenshot-reviewed all four current DSE diagrams with Mermaid
CLI 12.0.0. The compact federation-switch flow has small labels but no
clipping at its 784-pixel width. The tall recipient-eligibility and process
attribute-projection flows remain readable at native width; no layout change
was needed. Each rendered input matches the ledger's current source hash.
Confirmed current-body screenshot reviews now total 30/478 (6.3%); 448 other
entries remain historically unresolved rather than assumed unreviewed. No tests,
builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review all four current diagrams in the
2025 Relevance Advisory guide. Preserve its distinction between owner-side
update relevance and receiver-side scope, and check the current source hashes
before promoting review status. Keep the 236/463 historical checkpoint
separate until identities can be recovered.


## 2025 Relevance Advisory guide screenshot review, increment 86 — 2026-11-07

Rendered and screenshot-reviewed all four current diagrams in the 2025
Relevance Advisory guide with Mermaid CLI 12.0.0. All charts render without
clipping. The opening fan-out overview is dense and uses small labels at its
784-pixel width, but the separate publisher, owner, receiver, discovery, and
reflection paths remain distinguishable; retain it for now and revisit if
reader feedback confirms a legibility problem. The rendered inputs match the
current source hashes. Confirmed current-body screenshot reviews now total
34/478 (7.1%); 444 other rows remain historically unresolved, not assumed
unreviewed. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review all four 2025 Attribute Update
Rate Admission diagrams. Preserve the guide's receiver-local rate gate and do
not conflate suppressed best-effort values with DSE reevaluation, scope, or an
owner-facing relevance advisory. Keep the historical 236/463 checkpoint
separate until chart identities can be recovered.


## 2025 Attribute Update Rate Admission guide screenshot review, increment 87 — 2026-11-07

Rendered and screenshot-reviewed all four current Attribute Update Rate
Admission diagrams with Mermaid CLI 12.0.0. The projection, receive-order
admission, subscription-generation, and timestamped candidate flows are
visible without clipping at native width. The narrow 360-pixel projection
chart and tall 2,244-pixel receive-order flow remain readable while scrolling.
The diagrams keep the receiver-local best-effort rate gate distinct from
subscription DSE, scope, and owner-facing relevance advisories. All rendered
inputs match the current source hashes in the ledger. Confirmed current-body
screenshot reviews now total 38/478 (7.9%); 440 entries retain historically
unresolved status, not an assertion of no prior review. No tests, builds, CI,
Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review the nine 2025 Data Element
Encoding diagrams as one coherent guide batch, paying particular attention to
composite and variant layouts and preserving octet/endian boundaries. Validate
source hashes before promoting entries. Keep the 236/463 historical aggregate
separate until chart identity is recovered.


## 2025 Data Element Encoding guide screenshot review, increment 88 — 2026-11-07

Rendered all nine current Data Element Encoding diagrams with Mermaid CLI
12.0.0 and reviewed each native-width screenshot. The codec entry flow, value
classification, scalar endian conversion, string handling, composite
boundaries, fixed-record variant dispatch, extendable-variant cursor behavior,
logical-time factory delegation, and implementation-boundary overview all
render without clipping. The 360-pixel extendable-variant flow is readable at
native width while scrolling. The final chart keeps 2010 and 2025 implementation
lanes visibly separate and shows only their shared unsigned-octet primitive;
this is a boundary illustration, not a merged stream. Rendered bodies match all
nine current source hashes. Confirmed current-body screenshot reviews now total
47/478 (9.8%); 431 entries retain historically unresolved status, not an
assertion of no prior review. No tests, builds, CI, Requirements Lab, or 2010
implementation work.

**Next handoff:** Render and screenshot-review all six diagrams in the 2025
Connection and Configuration guide. Check that connection setup, configuration
selection, and failure paths remain legible and that labels distinguish public
API effects from Umbra implementation observations. Validate source hashes
before promoting entries; keep the 236/463 historical aggregate separate.


## 2025 Connection and Configuration visual review, increment 89 — 2026-11-07

Rendered and screenshot-reviewed all six current diagrams in the 2025
Connection and Configuration guide with Mermaid CLI 12.0.0. The Connect gate,
configuration/profile selection, TLS handshake, factory-to-RID lookup, ordered
Disconnect teardown, and reconnect/session-queue trace are visible without
clipping. The compact reconnect chart remains readable at native width. Five
identities were newly confirmed; the first chart already had current screenshot
evidence and was rechecked. During the ledger audit, callback/service-ordering
chart 18 was found to have a stale source hash. Recomputed the hash from its
current body, rendered it, and reviewed the screenshot; its service-report and
callback-exception paths remain readable. Confirmed current-body screenshot
reviews now total 53/478 (11.1%); 425 entries remain historically unresolved,
not presumed unreviewed. The 2010 aggregate remains separate and unchanged.
No tests, builds, CI, Requirements Lab, or 2010 implementation work.

**Next handoff:** Render and screenshot-review all seven diagrams in the 2025
Exception Reporting guide. Preserve the distinction between synchronous
service-report delivery, callback dispatch, and queued or asynchronous reports;
keep source-derived conclusions separate from focused-test evidence. Validate
all seven source hashes before promoting review status. Keep the 236/463
historical checkpoint separate.


## 2025 Exception Reporting visual review, increment 90 — 2026-11-07

Rendered and screenshot-reviewed all seven current Exception Reporting diagrams
with Mermaid CLI 12.0.0. The operation-to-report-family decision flow and the
embedded and configured-process sequences distinguish HLAreportException,
HLAreportMOMexception, and HLAreportServiceInvocation behavior. Additional
charts cover callback-thrown exceptions, synchronous MOM rejection reporting,
and ordering of receive-order reports around request results. All seven images
are unclipped; the sequence charts use compact labels but remain readable at
native width. Each rendered body matches its current ledger hash. Confirmed
current-body screenshot reviews now total 60/478 (12.6%); 418 rows retain
historically unresolved status, not presumed unreviewed. No tests, builds, CI,
Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review the nine diagrams in the 2025
Object Instance Name Reservation guide. Check the reserve, release, and
registration lifecycles as separate ownership states, and keep public HLA
behavior distinct from Umbra-specific storage and collision handling. Validate
source hashes before promoting entries; keep the 236/463 historical aggregate
separate.


## 2025 Object Instance Name Reservation visual review, increment 91 — 2026-11-07

Rendered and screenshot-reviewed all nine current Object Instance Name
Reservation diagrams with Mermaid CLI 12.0.0. The charts distinguish single
and multiple reservation outcomes, release preconditions, named registration
commit, peer discovery, the reservation state machine, save/restore behavior,
and the timestamped deletion/name-purge boundary. Tall callback and deletion
flows remain readable at native width; the state chart and peer-discovery view
keep owner and observer state separate. All nine rendered bodies match their
current ledger hashes. Confirmed current-body screenshot reviews now total
69/478 (14.4%); 409 entries remain historically unresolved, not presumed
unreviewed. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review all nine diagrams in the 2025
Directed Interaction guide. Preserve the distinction between interaction-class
publication/subscription, parameter encoding, routing, and callback delivery;
check labels for any implied ordering the source or tests do not establish.
Validate source hashes before promoting entries and keep the 236/463 historical
aggregate separate.


## 2025 Directed Interaction visual review, increment 92 — 2026-11-07

Rendered and screenshot-reviewed all nine current Directed Interaction diagrams
with Mermaid CLI 12.0.0. The charts cover declaration admission, target/selector
recipient planning, directed-pair publication and removal, queued callback
revalidation, timestamped delivery and retraction, and embedded/process routing.
The process-routing diagram was changed from a left-to-right layout to a
top-to-bottom layout after its labels rendered too small; the revised layout is
readable at native width and keeps the embedded and process paths distinct. The
wide recipient-planning sequence is dense, but its labels remain readable at
full image size. All nine rendered bodies match their current ledger hashes.
Confirmed current-body screenshot reviews now total 78/478 (16.3%); 400 entries
remain historically unresolved, not presumed unreviewed. No tests, builds, CI,
Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review the ten diagrams in the 2025
Name and Handle Resolution guide. Keep name strings, category-typed public
handles, and private process identifiers distinct; check that every diagram
labels the boundary it describes. Validate source hashes before promoting
entries and keep the 236/463 historical aggregate separate.


## 2025 Name and Handle Resolution visual review, increment 93 — 2026-11-07

Rendered and screenshot-reviewed all ten current Name and Handle Resolution
diagrams with Mermaid CLI 12.0.0. The flows distinguish service name lookup,
joined-model resolution, class/member hierarchy checks, receiver discovery,
public handle encoding/decoding, stable identity mapping, federation directory
reconciliation, and typed-handle dispatch to embedded versus process paths.
The random handle-allocation chart originally rendered left-to-right with tiny
labels; changing it to top-to-bottom made its retry and collision paths legible
while preserving the algorithm. The wide six-participant portability trace is
dense but readable at full image size. All ten rendered bodies match current
source hashes. Confirmed current-body screenshot reviews now total 88/478
(18.4%); 390 entries remain historically unresolved, not presumed unreviewed.
No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review the eleven diagrams in the 2025
Order and Transportation guide. Keep message order, delivery timing, transport
selection, and retraction as separate concepts; check that diagrams do not
imply callback or delivery guarantees beyond their evidence. Validate source
hashes before promoting entries and keep the 236/463 historical checkpoint
separate.


## 2025 Order and Transportation visual review, increment 94 — 2026-11-07

Rendered and screenshot-reviewed all eleven current Order and Transportation
diagrams with Mermaid CLI 12.0.0. The charts distinguish effective send order,
transportation metadata, receiver-side TSO gating, ownership effects, pending
transport-change admission/commit/discard paths, callback timing, and query
results. The sender-setting comparison chart was changed to a top-to-bottom
layout, and the pending-request chart was recast as a flowchart because its
state-diagram labels were too compressed. Both revised charts are legible at
native width. The other charts render without clipping; wide sequence views
remain readable at full size. All eleven bodies match current ledger hashes.
Confirmed current-body screenshot reviews now total 99/478 (20.7%); 379 entries
remain historically unresolved, not presumed unreviewed. No tests, builds, CI,
Requirements Lab, or 2010 work.

**Next handoff:** Render and screenshot-review the twelve diagrams in the 2025
FOM Module Admission and Composition guide. Check module-level versus
whole-model validation, dependency ordering, and commit boundaries; do not imply
transactional rollback or composition guarantees beyond the source and tests.
Validate source hashes before promoting entries and keep the 236/463 historical
checkpoint separate.


## 2025 FOM Module Admission and Composition visual review, increment 95 — 2026-11-07

Rendered and screenshot-reviewed all twelve current FOM Module Admission and
Composition diagrams with Mermaid CLI 12.0.0. They cover Create and Join
preparation, per-module and completed-model validation, variant composition,
handle-directory reconciliation, commit and rollback boundaries, join-added
class/attribute discovery, and FDD artifact checks. The guide's final model
edition chart distinguishes the default 2025 resource path from the bounded
2010 model-compatibility catalog under the 2025 API; this is a stream-boundary
view, not review of the 2010 RTI implementation. All twelve bodies match their
current ledger hashes and render without clipping. Confirmed current-body
screenshot reviews now total 111/478 (23.2%); 367 entries remain historically
unresolved, not presumed unreviewed. No tests, builds, CI, Requirements Lab,
or 2010 implementation work.

**Next handoff:** Render and screenshot-review the twelve diagrams in the 2025
Object Attribute Subscription State guide. Track passive registration, active
subscriptions, discovery, value reflection, and unsubscription as separate
state transitions. Check that process and embedded evidence remain distinct
and that no callback ordering is inferred beyond the source and tests. Validate
source hashes before promoting entries and keep the 236/463 historical
checkpoint separate.


## 2025 Object Attribute Subscription State visual review, increment 96 — 2026-11-07

Rendered and screenshot-reviewed all twelve current subscription-state diagrams
with Mermaid CLI 12.0.0. They distinguish receiver interest, discovery, value
reflection, passive and active subscriptions, selected-subset unsubscribe,
regional declarations, class ancestry, and embedded/process discovery paths.
The subscription-change overview and the embedded/process late-discovery
comparison were changed to top-to-bottom layouts because their left-to-right
labels rendered too small. Both revised charts now read clearly at native width.
All twelve rendered bodies match current ledger hashes. Confirmed current-body
screenshot reviews now total 123/478 (25.7%); 355 entries remain historically
unresolved, not presumed unreviewed. No tests, builds, CI, Requirements Lab, or
2010 work.

**Next handoff:** Render and screenshot-review all fourteen diagrams in the
2025 Attribute Value Update Request guide. Keep the producer's accepted update,
receiver-specific eligibility, delivery order, ownership, rate filtering, and
callback dispatch as distinct gates. Check for implied ordering beyond source
and focused-test evidence. Validate hashes before promoting entries and keep
the 236/463 historical checkpoint separate.


## 2025 Federation MOM payload-shape and departure review, increment 116 — 2026-11-07

The status-query section already described request inputs, current state
snapshots, the paired callbacks, and the configured-process generic-route
evidence boundary. Added the precise report payload shapes from the pinned
MIM: `HLAsyncPoints` is a variable array of Unicode labels, while
`HLAsyncPointFederates` is a variable array of fixed records containing an
encoded federate reference and synchronization status. The focused test decodes
both and compares status entries by federate handle; it does not establish list
ordering.

Cross-checked the status snapshot with embedded resignation cleanup. Departure
removes the member from the synchronization, announced, and achieved sets; it
can leave a pending roster without that member or complete and erase the point.
No focused test combines resignation with a subsequent status query, so those
query-after-departure outcomes remain source-derived. The existing flowchart
was updated and rendered with Mermaid CLI 12.0.0; local screenshot review found
its branches and labels legible. Source anchors, test anchors, and the existing
synchronization-guide section link were checked. No tests, builds, CI,
Requirements Lab, or 2010 content changed.

Progress measures: the defined content audit is 32/32 guides (100%); the
current-body visual ledger records 486/486 diagrams (100%). Neither is an
overall completion percentage for the continuing documentation goal. GitHub
page rendering remains unverified.

**Next handoff:** Cross-review the 2025 Federation MOM `HLAreportFederateLost`
flow with the Lifecycle guide. Trace the lost member-list snapshot, local
`connectionLost` callback, member-scoped MOM object retirement, and federation
report delivery. Compare embedded and configured-process evidence separately;
add a cross-link or clarification only for a material reader gap. Keep 2010
separate and leave the Requirements Lab untouched.

## 2025 Federate-Lost process evidence cross-review, increment 117 — 2026-11-07

Corrected the configured-process evidence boundary in the Federation MOM and
Lifecycle guides. The promoted adapter-managed installed-package C++ TCK
scenario covers the reliable four-parameter report and cutoff timestamp,
eventual local `connectionLost`, the federation membership-list update, and a
public member-list query showing only the survivor. It runs through the
process fixture's Push receive mode under both callback models; it does not
test Pull, relative callback ordering, or retirement of the lost
member's `HLAfederate` MOM object. The guides now state those limits and link to
the direct TCK source and promoted-run record. No diagrams changed, so the
diagram visual ledger remains unchanged. No tests/builds, Requirements Lab,
or 2010 work.

Progress gates: bounded survey 1/1; content audit 32/32; local diagram review
486/486; GitHub-rendered verification not yet verified. Overall gate progress
is 3/4 (75%). Cross-guide reader-gap reviews remain an active backlog and do
not change this fixed gate denominator.

**Next handoff:** Verify GitHub's rendering of the 32 2025 topic guides against
the local render ledger, logging any renderer-specific failures. Continue
bounded cross-guide reader-gap reviews in parallel with that final gate. Keep
2010 separate and leave the Requirements Lab untouched.

## 2025 hosted-render gate version check, increment 118 — 2026-11-07

Read-only inspection of GitHub's `feature/initial-work` pages found that the
hosted roadmap is behind the current checkout: GitHub reports 31 topic guides
and 340 diagrams, while the local inventory is 32 guides and 486 diagrams.
The page extraction exposes Mermaid source with `Loading` placeholders, which
does not establish whether GitHub's browser renderer successfully displays
the diagrams. The hosted-render gate therefore cannot yet be evaluated
against the current deliverables. No branch or remote changes were made.

Progress remains 3/4 gates (75%). The final gate requires reconciling the
current documentation state with the GitHub branch, then visually checking
the hosted rendering of all 32 current guides. Continue the bounded reader-gap
backlog; preserve unrelated staged and unstaged work when preparing the branch
update. No tests/builds, Requirements Lab, or 2010 work.

**Next handoff:** Reconcile the current 2025 documentation changes and visual
ledger with `feature/initial-work`, preserving all unrelated staged and
unstaged work. Once the branch exposes the current 32-guide / 486-diagram
inventory, inspect the actual GitHub-rendered pages and record chart failures
or successful screenshot evidence. Keep 2010 separate and leave the
Requirements Lab untouched.

## 2025 hosted evidence-link compatibility audit, increment 119 — 2026-11-07

Compared source-line links in the current 2025 Markdown set against the
hosted `feature/initial-work` tree. Across 4,383 unique path/line/guide
references in 33 Markdown files (32 topic guides plus this roadmap), 159 do
not resolve against the hosted tree: some line numbers exceed the hosted
file's current length and some target files are absent. This confirms that a
guide-only update would publish broken evidence links. A headless Chrome
capture confirmed the GitHub preview renders prose and tables; the targeted
chart viewport did not establish Mermaid rendering. The remote branch and
worktree were not changed.

Progress remains 3/4 gates (75%). Before hosted visual review, reconcile the
documentation with a branch containing the source/test/catalog versions that
its evidence links target. Do not publish an isolated documentation snapshot
that leaves those links broken, and preserve the unrelated mixed worktree.
No tests/builds, Requirements Lab, or 2010 work.

**Next handoff:** Determine the smallest coherent branch point that contains
the current documentation and its cited implementation/test/catalog sources.
Compare local dirty paths with `feature/initial-work`, preserving unrelated
staged and unstaged work. Once source anchors resolve on the candidate branch,
check the actual GitHub Mermaid rendering for all 32 2025 guides and record
visual evidence. Keep 2010 separate and leave the Requirements Lab untouched.

## 2025 hosted source-link dependency inventory, increment 120 — 2026-11-07

Resolved the 4,383-reference comparison against the current checkout. All
source-line references in the 32 topic guides and roadmap resolve locally; the
159 hosted-branch failures affect 65 distinct target files. Of these, 44 are
modified in the worktree, 17 are untracked, and four already differ from the
hosted tree in local commits. The current checkout therefore has the evidence
needed to support the guides, but a docs-only branch from GitHub's current
commit would omit cited implementation, test, and catalog targets. No files
were staged or committed by this audit, and no remote was changed.

Progress remains 3/4 gates (75%). The next reconciliation must account for
those 65 cited targets while excluding unrelated dirty paths; do not copy all
source changes merely to make a documentation branch appear self-contained.
No tests/builds, Requirements Lab, or 2010 work.

**Next handoff:** Recompute the 65-target source-link dependency set after the
current implementation work is committed or otherwise available on the
candidate branch. Verify all 4,383 guide/roadmap anchors against that branch.
Only then publish or preview the 2025 documentation set and visually inspect
Mermaid rendering for all 32 guides. Preserve unrelated staged and unstaged
work; keep 2010 separate and leave the Requirements Lab untouched.

## 2025 evidence-link checker, increment 121 — 2026-11-07

Added the read-only [2025 documentation line-link checker](../../quality_gates/bin/check_hla_2025_documentation_links.py).
It scans the 32 topic guides and this roadmap, then checks each local file and
line target against either the worktree or a selected Git revision. The
worktree run resolves 4,383/4,383 references. The hosted branch run reproduces
4,224 resolved and 159 unresolved references across 65 targets. This checks
file existence and line bounds only; it does not validate heading links,
semantic correctness of the referenced lines, or actual GitHub Mermaid
rendering.

Run `python quality_gates/bin/check_hla_2025_documentation_links.py
--worktree` for current local evidence, or pass `--ref <revision>` to compare a
candidate branch. The latter currently returns 1 for the expected hosted
branch gaps. Python syntax compilation and `git diff --check` pass. No tests,
builds, Requirements Lab, or 2010 work.

Progress remains 3/4 gates (75%).

**Next handoff:** After the cited implementation/test/catalog changes are
available on a candidate branch, run the checker with `--ref <revision>` and
resolve every line-link failure. Then visually inspect actual GitHub Mermaid
rendering across all 32 current 2025 guides. Keep unrelated staged and
unstaged changes, 2010, and the Requirements Lab outside this documentation
update.

## 2025 hosted-link dependency split, increment 122 — 2026-11-07

Compared the hosted branch, current `HEAD`, and worktree with the checker.
Of the 159 references failing on `origin/feature/initial-work`, 11 resolve at
current `HEAD`, which is three commits ahead of the hosted branch. The other
148 require worktree versions from 61 targets (44 modified and 17 untracked).
This divides the reconciliation into committed branch lag and uncommitted
evidence dependencies. The references in the latter group should not be
retargeted to older source merely to make a docs-only update pass.

Progress remains 3/4 gates (75%). No source files were staged, committed, or
pushed. No tests/builds, Requirements Lab, or 2010 work.

**Next handoff:** When the current evidence-source work is ready to land,
recheck from a candidate branch that includes the three local commits and the
61 required modified/untracked targets. Run
`python quality_gates/bin/check_hla_2025_documentation_links.py --ref <revision>`;
then resolve any remaining failures without retargeting documentation to
older behavior. Visually inspect all 32 guides in GitHub after the target
branch is published. Preserve all other staged/unstaged work and keep 2010 and
Requirements Lab separate.

## 2025 prioritized documentation backlog, increment 123 — 2026-11-07

Rechecked the local evidence-link checker: all 4,383 line references resolve
in the worktree. Added a compact, ranked backlog near the roadmap baseline:
candidate-branch source reconciliation, GitHub rendering of all 32 guides, and
a source-coupled cross-review of the ownership, process callback-order, and
Join/MOM seams. The third item is explicitly a follow-up to confirm current
evidence boundaries, not a hidden fourth completion gate; its documented test
gaps do not make the 32-guide content audit incomplete. Overall gate progress
remains 3/4 (75%). No implementation or 2010 files, tests, builds, CI, or
Requirements Lab content changed.

**Next handoff:** After the cited source changes are available on a candidate
branch, satisfy backlog item 1 with the line-link checker. Then perform item 3
against that same candidate revision and inspect actual GitHub rendering for
all 32 guides (item 2). Preserve unrelated staged and unstaged work; keep 2010
and the Requirements Lab separate.

## 2025 ownership transfer/cancellation evidence review, increment 124 — 2026-11-07

Reviewed the Attribute Ownership guide's Divestiture If Wanted interaction
with pending acquisition cancellation against the current embedded registry
and focused race test. The source removes each transferred attribute from
matching cancellation reservations; the later cancellation gate resolves only
the attributes still reserved. The focused race test coordinates a one-
attribute request, confirms cancellation after the owner's release callback
has entered, and observes only the If-Wanted acquisition notification. The
guide correctly marks the multi-attribute split and callback observation order
as source-derived and untested, including the possibility of another terminal
response. No prose or diagram change was needed. This evidence is embedded
2025 only; no process parity is inferred. No tests, builds, CI, Requirements
Lab, or 2010 work.

**Next handoff:** Review the configured-process mixed callback-family ordering
section against current client, bridge, dispatcher, and focused-test evidence.
Check that separate FIFO mechanics, direct polled-interaction routing, tested
pairwise orders, and untested mixed schedules remain accurately distinguished.
Update the guide only for a material gap; preserve the 2025 boundary.

## 2025 configured-process callback-order evidence review, increment 125 — 2026-11-07

Rechecked the Process Endpoint guide's cross-family ordering section against
the current client, callback-control adapter, callback bridge, dispatcher,
and cited public tests. Pushed frames and non-interaction poll results use the
client's typed-event FIFO; a polled interaction is returned separately and
dispatched directly. Both paths submit through the bridge to the shared
dispatcher, while the client dispatch lease allows concurrent interleavings.
The cited public tests establish selected interaction-before-grant and
reflection-before-grant schedules. They do not combine interaction, full
reflection, and deferred marker work or establish a universal callback-entry
order. The guide already states these distinctions and the test boundary; no
change was needed. This review concerns configured-process 2025 behavior only.
No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Review the Federation Lifecycle guide's Join commit versus
new-member MOM discovery seam against embedded and configured-process source
and focused-test evidence. Confirm membership/local Joined commit, HLAfederate
setup, discovery planning, callback delivery, and failure limits remain
distinct; do not infer callback order or profile parity. Then proceed to
candidate-branch link reconciliation and GitHub rendering of all 32 guides.

## 2025 configured-process Join rollback after MOM discovery, increment 126 — 2026-11-07

The Join/MOM cross-review found a missing configured-process failure branch.
The service can stage an existing observer's new `HLAfederate` discovery and
initial reflection, then fail before returning Join success; its rollback
resigns provisional membership, discards pending events for that member object,
and clears the installed session. The public client applies its local Joined
transition only after a successful response. Added this as a separate branch
in the Join profile comparison, preserving the embedded `HLA_IMMEDIATE`
callback-throw case as a different partial-commit path.

The public configured-process regression injects failure immediately after the
joined-federate discovery work is queued. It observes no callback or initial
reflection for the failed lifetime, then successfully retries the same name;
on retry it observes the old object's removal before the new discovery and
initial reflection. That is a selected `HLA_EVOKED` test schedule, not a
general callback-order guarantee. The guide links the service rollback, client
Join transition, and test. Rendered the revised sequence with Mermaid CLI
12.0.0 and screenshot-reviewed it; the first render exposed a clipped failure
note, which was shortened and reviewed again. Updated the visual ledger's
body hash and downstream diagram line anchors. The ledger audit also found ten
stale Federation MOM diagram line anchors after prose-only edits; their body
hashes still matched, and the shifted line positions are now refreshed.
Inventory remains 486 diagrams with all 486 current bodies locally reviewed.
No tests/builds, Requirements Lab, or 2010 work.

**Next handoff:** The worktree checker resolves 4,385/4,385 source-line
references; the hosted `origin/feature/initial-work` branch still has 159
unresolved references across 65 targets. Once the required source revisions
are available together on a candidate branch, run the checker against that
revision, reconfirm the three source-coupled reviews there, and inspect actual
GitHub rendering for all 32 guides. The gate score remains 3/4 (75%) until
hosted rendering is visually verified.

## 2025 late-Join announcement rollback cross-review, increment 127 — 2026-11-07

Followed the Lifecycle Join/MOM review across its synchronization boundary.
The Synchronization guide already explains that an omitted-set late Join adds
the new member to pending participant and announced sets, and that configured-
process announcement-enqueue failure rolls back membership and clears the
joining session's queued announcements. Its cited public process case injects
failure after one late-Join announcement is queued, verifies no leaked
announcement, and retries successfully with exactly one announcement. The
guide correctly marks multi-point batch failure and multi-point rollback as
source-derived and untested. No diagram or prose change was warranted. This is
configured-process 2025 evidence and does not imply embedded/process parity.
No tests/builds, Requirements Lab, or 2010 work.

**Next handoff:** Keep the 2025 documentation goal at 3/4 gates (75%). Once
the cited source revisions are available together on a candidate branch, run
`python quality_gates/bin/check_hla_2025_documentation_links.py --ref <revision>`;
then reconfirm the source-coupled reviews and visually inspect actual GitHub
rendering for all 32 guides. Preserve unrelated staged and unstaged work;
leave 2010 and the Requirements Lab separate.

## 2025 hosted-link dependency refresh, increment 128 — 2026-11-07

Fetched `origin/feature/initial-work` at `c9c5a9e` and confirmed local `HEAD`
(`e6321e8`) is three commits ahead (zero behind). The documentation checker
reports 4,385/4,385 references resolved in the worktree, 4,237 at `HEAD`, and
4,226 on the hosted branch. Thus 11 hosted failures are covered by the three
local commits; 148 remain across 59 source targets in the worktree (42
modified, 17 untracked). Added the
dated [dependency snapshot](HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json) with each
remaining target, the exact guide/line references, worktree status, and both
revision IDs. It explicitly warns that path membership is not authorization
to stage unrelated changes in those files.

No source paths were staged or committed, no remote was changed beyond the
read-only fetch, and no tests/builds, Requirements Lab, or 2010 work occurred.
Overall completion remains 3/4 gates (75%).

**Next handoff:** After the cited source work is deliberately ready to land,
form a candidate branch from current `HEAD` plus only task-related changes
among the 59 snapshot targets. Refresh the checker against that candidate,
then visually inspect all 32 GitHub guide pages. Preserve unrelated edits and
keep 2010 and the Requirements Lab separate.

## 2025 diagram identity refresh and scope trim, increment 129 — 2026-11-07

Audited all 486 Mermaid bodies in the 32 current 2025 guides against the visual
review ledger. The audit found 186 stale line/hash identities. Rendered all 186
with Mermaid CLI and visually reviewed their screenshots; after review, trimmed
the DataElement guide's unnecessary cross-edition comparison to a 2025-only
encoding path, then rendered and reviewed that revised diagram. Reconciled the
ledger to all 486 current body hashes and source lines (282 flowcharts, 180
sequence diagrams, 24 state diagrams).

The current-worktree link check now covers 4,382 references and resolves all
4,382. The same current documentation has 148 unresolved references across 59
targets at `HEAD` (4,234 resolve), and 159 across 65 targets on the hosted
branch (4,223 resolve). The 11-reference difference is still covered by the
three local commits; the 59-target source dependency snapshot remains current.
The GitHub render gate remains unverified, so overall progress stays at 3/4
(75%). No implementation or test changes, builds, Requirements Lab edits, or
2010 implementation changes were made.

**Next handoff:** Once the cited source revisions are intentionally ready to
land, assemble a candidate from `HEAD` plus only task-related changes among the
59 targets in [the dependency snapshot](HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json).
Run the documentation link checker against that candidate, then inspect actual
GitHub rendering for all 32 guides. Preserve unrelated staged and unstaged
work, and keep the 2010 implementation stream separate.

## 2025 isolated source-link candidate, increment 130 — 2026-11-07

Built a local Git tree using a temporary index, based on `HEAD` plus the 59
source targets in the dependency snapshot, all 32 2025 guides, the roadmap,
diagram ledger, dependency snapshot, and roadmap index (95 paths total). This
left the checkout's real index and branch untouched. The documentation link
checker resolved all 4,382 references against that candidate tree. Thus the
link failure is no longer in assembling a local candidate; it is in reviewing
and promoting the cited source revisions.

The 59 source targets contain 100,796 added lines and 47,480 deleted lines,
including 80,471 additions in the generated C++ TCK scenario catalog. Those
changes need review before publication; path membership in the dependency
snapshot does not make the complete file diffs task-only. The hosted branch
still has 159 unresolved references across 65 targets, while the local
candidate resolves all 4,382. Actual GitHub rendering remains the fourth gate,
so overall progress is 3/4 (75%). No build, test, Requirements Lab edit, or
2010 implementation change was made.

**Next handoff:** Review the exact 59 target diffs for publishable scope while
preserving unrelated work. Once the intended source revisions are ready, expose
the verified documentation/source candidate on a GitHub branch, rerun the
checker against that hosted revision, and visually verify all 32 rendered
guides.

## 2025 publication-readiness recheck, increment 131 — 2026-11-07

Rechecked the current checkout after additional source and guide edits. The
worktree link checker resolves 4,382/4,382 unique path/line/guide references;
against local `HEAD`, 148 are unresolved across 59 source targets (4,234
resolve). Of the 148 references, 57 can be uniquely mapped to the same source
line and adjacent context in `HEAD`; 78 point to changed context or files absent
from `HEAD`, and 13 have ambiguous or missing exact guide-link matches. This
audit does not alter citations: only a fully unique, context-preserving mapping
would be eligible for that cleanup.

The candidate is still not ready to publish as a documentation-only change.
The dependency snapshot includes 17 new source targets, and the 59 target diffs
remain mixed with broad implementation and generated-catalog edits. Local link
resolution and local Mermaid screenshots do not verify GitHub page rendering.
Therefore the 4/4 completion gate remains open; 3/4 (75%) is still the accurate
gate count, not a percentage of HLA coverage or a measure of publication
readiness.

**Next handoff:** Review the 17 new source targets and 41 modified targets
against their guide claims, separating task-required revisions from unrelated
file changes. For each stale citation, either retain a verified current-source
dependency or map it to a unique unchanged line in `HEAD`; do not use ambiguous
matches. Rebuild the candidate tree from the reviewed scope, run the link
checker against that tree, then publish the reviewed candidate and inspect the
actual GitHub rendering of all 32 guides. Preserve unrelated staged and
unstaged work; do not touch Requirements Lab or 2010 implementation files.

## 2025 safe citation reduction, increment 132 — 2026-11-07

Three callback-dispatcher citations had unique links to source lines whose
content and adjacent context are identical in local `HEAD`. Updated those
citation anchors to the matching `HEAD` lines (337 to 279, and 367 to 309 in
two guides). All three anchors resolve both in the worktree and on the hosted
branch. No implementation source was changed.

Refreshed the dependency snapshot from the current 33 guides. The worktree
still resolves all 4,382 references. Local `HEAD` now resolves 4,237, leaving
145 references across 58 targets (41 modified, 17 untracked). The hosted branch
resolves 4,226, leaving 156 references across 64 targets; 11 references across
7 targets are supplied by local worktree changes. The source review and actual
GitHub page-render gate remain open, so progress is still 3/4 (75%).

**Next handoff:** Continue auditing only citations that have one unique link
and a source line plus neighboring context identical in `HEAD`; verify each
candidate with both the worktree and hosted-branch checker. Then scope-review
the remaining 17 new and 41 modified source targets against the guide claims,
assemble a reviewed candidate, and inspect all 32 guides on GitHub. Preserve
unrelated changes and keep Requirements Lab and the 2010 implementation stream
untouched.

## 2025 citation dependency reduction, increment 133 — 2026-11-07

Audited the remaining stale citations against local `HEAD`. Fifty-four links
had a unique Markdown citation and pointed to a line whose contents and
immediate neighboring context match `HEAD`; updated those anchors. The exact
links resolve in both the worktree and hosted branch. The update affects 19
guides and changes no implementation source.

The refreshed snapshot now records 4,328 unique references across 33 guides.
All 4,328 resolve in the worktree; `HEAD` resolves 4,237, leaving 91
references across 42 targets (25 modified and 17 untracked). The hosted branch
resolves 4,226, leaving 102 references across 49 targets. The local source
scope and GitHub rendering remain unverified; overall progress remains 3/4
gates (75%).

**Next handoff:** Review the remaining 42 targets against the specific guide
claims. Separate the 17 new files from the 25 modified files, and inspect the
exact cited portions plus surrounding diffs before including any source in a
candidate. Rebuild the candidate using only reviewed task-related content,
verify every link against it and the hosted revision, then inspect all 32
GitHub-rendered guides. Preserve unrelated work; keep Requirements Lab and the
2010 implementation stream untouched.

## 2025 dependency-scope review, increment 134 — 2026-11-07

Reviewed the remaining connection/TLS source links against their target files
and build integration. The guide's TLS claims point to the new TLS socket
implementation and focused public/private tests, plus TLS branches in
`process_transport.cpp`. The new files are coherent with those claims, but
they are not independently buildable or publishable from the current diff:
root `CMakeLists.txt` combines the TLS/OpenSSL integration with unrelated
shared-library naming and source-list changes, while `process_transport.cpp`
combines TLS framing with the broader process-session recovery path. A
whole-file candidate would therefore pull in unreviewed behavior. No source
files were changed or staged, and no build/test was run.

The current dependency audit still has 91 references across 42 targets missing
from `HEAD`; all 4,328 current worktree references resolve. The TLS group is
classified as coupled pending work, not approved for publication. The GitHub
render gate remains open and overall progress remains 3/4 (75%).

**Next handoff:** Inspect whether the TLS-related hunks in CMake and
`process_transport.cpp` can be separated into a candidate patch while keeping
their TLS preconditions and failure paths intact. Then compare each proposed
hunk with the guide's cited behavior and focused test evidence. If isolation
would make the candidate misleading or omit a required dependency, retain the
files together and continue the broader cited-source review rather than
publishing a partial TLS stack. Preserve unrelated work and edition boundaries.

## 2025 TLS dependency isolation review, increment 135 — 2026-11-07

Reviewed the exact CMake and `process_transport.cpp` hunks needed by the TLS
citations. TLS is coupled to the pending process-session protocol: the listener
replaces the old hello exchange with TLS/WebSocket setup plus new-session and
resume negotiation; accepted connections now retain TLS/WebSocket stream
ownership and session state. The same file's changed paths are interleaved, so
extracting only the TLS-looking lines would omit their new signatures and
lifetime/error handling. Root CMake likewise combines OpenSSL linkage with
shared-library naming, process-service packaging, and several unrelated source
registrations. Selective TLS publication from these two whole-file diffs would
not produce a coherent build candidate.

No source or build files were staged or edited during this review. The guide
and build context remain in the current worktree; 91 references still require
source newer than `HEAD` across 42 targets. Overall documentation progress
remains 3/4 gates (75%), with GitHub rendering unverified.

**Next handoff:** Review the remaining source families by ownership: first
process-session/runtime and lifecycle, then federation attribute/DDM behavior,
then MOM/FOM and catalog evidence. For each family, identify all coupled
source-list, API, and focused-test changes before building a candidate. Do not
publish a subset that makes a source link resolve while omitting the behavior
or dependencies that link describes. Keep the 2025/2010 boundary and unrelated
work intact.

## 2025 process-session dependency review, increment 136 — 2026-11-07

Reviewed the process-session/recovery source family cited by the configured-
process guide. Its 23 current source-line references span 11 files: client
reconnect/request replay, host session lookup and retained state, transport
sequence/message history, service runtime, and focused recovery/process tests.
The modified `process_transport_session.cpp` grows by 368 lines while adding
sequenced request/response/event frames, bounded replay, duplicate suppression,
heartbeat acknowledgement, and connection replacement; this depends on the
new message-history component and coordinated client/host protocol changes.
These pieces form one publishable review unit and should not be cherry-picked
as isolated citation targets.

The guide's evidence boundary is appropriately narrow: it describes temporary
Umbra process-transport recovery while the service host remains alive, says the
private tests are not public-API or normative HLA evidence, and records host
restart and replay-buffer exhaustion as unproven. The roadmap records the
focused recovery test run. This review did not rerun tests or change source.
The unit still depends on mixed CMake integration and needs full source-scope
review before a candidate can be built.

**Next handoff:** Review the remaining federation attribute-update/DDM group,
especially the registry recipient-planning, value-update request, region, and
focused TSO-before-grant changes. Check that each guide claim maps to a
coherent behavior/test unit and identify any unrelated hunks in the same files
before candidate assembly. Keep process recovery as a single coupled unit;
preserve unrelated work, Requirements Lab, and the 2010 boundary.

## 2025 source-line anchor repair, increment 137 — 2026-11-07

A verification pass caught that 54 source-line links retargeted in increment
133 had been serialized as URL queries (`?L...`) instead of Markdown/GitHub
fragments (`#L...`). That caused the link checker to omit those citations from
the reported unique-reference count. Repaired the separators in all 54 links
across 19 guides, regenerated the hosted dependency snapshot from current guide
links, and rechecked the full set. The worktree resolves 4,380/4,380 unique
path/line/guide references; two formerly distinct line citations now identify
the same line in one guide, accounting for the difference from the earlier
4,382 count. `HEAD` resolves 4,289, leaving 91 across 42 targets; the hosted
branch resolves 4,275, leaving 105 across 49 targets. Fourteen references
across eight targets are available in the worktree but not the hosted branch.

The earlier 4,328 totals in increments 133–136 were incomplete because of the
malformed fragments; the refreshed snapshot and these corrected totals are
authoritative. No source implementation was changed. GitHub rendering remains
unverified, so completion stays 3/4 gates (75%).

**Next handoff:** Resume the federation attribute-update/DDM source review with
the repaired full citation set. Inspect the registry recipient-planning,
regional request, region realization, and focused TSO-before-grant changes as
one behavior/test group; distinguish the two untracked TCK scenario files
from modified implementation hunks. Refresh totals again before assembling a
candidate, and preserve the 2010 boundary and unrelated work.

## 2025 DDM dependency review, increment 138 — 2026-11-07

Rechecked the 2025 documentation-link verifier after the DDM anchor review.
The authoritative current count is 4,382 references across 33 Markdown files:
4,382 resolve in the worktree, 4,293 resolve at `HEAD` (89 unresolved across
41 targets), and 4,278 resolve on `origin/feature/initial-work` (104 unresolved
across 49 targets). This supersedes increment 137's stale counts, which were
based on a regenerated snapshot that omitted two distinct current references.
Rebuilt the entire per-target dependency inventory from the current links,
including the target statuses and every failing guide/line reference. The
resulting snapshot is now consistent with the checker.

The DDM guide already explains the excluded-dimension realization scenario as
inspected but not executed, with the portable TCK implementation and FOM
fixture explicitly cited. It also preserves the boundary between per-attribute
region evaluation and time eligibility. No claim of TCK execution or 2010
equivalence was added. The regional registry and attribute-update source files
remain modified; the portable TCK source and fixture remain untracked, so these
links cannot yet be promoted to the hosted branch as verified evidence.

The refreshed inventory has 41 `HEAD` target files: 24 modified and 17
untracked. It resolves the prior count discrepancy without treating the
missing hosted citations as documentation failures; those links point into
local changes that still need deliberate candidate review.

**Next handoff:** Regenerate the entire dependency snapshot from the current
guide links and reclassify the DDM target group by changed source hunk, test
status, and guide claim. Then continue the same review across the remaining
source-coupled seams, prepare a task-only candidate, and validate it on a
GitHub-rendered branch. Preserve unrelated work, the Requirements Lab, and the
2010 boundary.

## 2025 DDM evidence-path correction, increment 139 — 2026-11-07

Revised the inherited-dimension chart in the DDM guide. It now shows the
observed subscription exception and the resulting stop before registration as
the solid path. The overlap, discovery, and reflection branches remain visible
as dashed, explicitly unobserved expectations authored by the test. This
prevents the diagram from visually presenting intended assertions as executed
behavior. The guide continues to distinguish the exception reported by the
test from the `invalid_region_context` mapping inferred from source; Catch2
reported an unknown exception type.

Rendered the changed Mermaid body with Mermaid CLI 12.2.0 and reviewed its
white-background PNG for clipped labels, branch ambiguity, and contrast. The
visual-review ledger now records the current body hash, source line, render,
and screenshot review. Diagram inventory remains 486. No implementation or
test behavior changed, and no 2010 or Requirements Lab material was touched.

**Next handoff:** Continue the source-coupled review of the DDM inherited
attribute-dimension path. Determine which claims have direct test evidence,
which are source-derived, and whether a focused portable TCK result exists;
then review the remaining cross-guide seams before assembling a candidate for
GitHub rendering verification. Preserve unrelated work and the 2010 boundary.

## 2025 DDM region-realization evidence, increment 140 — 2026-11-07

The portable TCK evidence already exists in the current worktree and was
stronger than the previous handoff stated. The
`regional-object-attribute-dimension-inheritance` scenario and its contract
twin passed four recorded executions total: each scenario passed under
`HLA_EVOKED` and `HLA_IMMEDIATE`.
The catalog records installed-package CTest plus direct JSON/JUnit evidence.
The excluded-dimension scenario and its contract twin also have recorded 4/4
passes across both callback models; the source, catalog entry, and result
summary agree that X discovers/reflects while Y receives neither. The result
JSON files are under ignored `out/`; the catalog records the run summary and
artifact locations. The guide links the tracked catalog evidence rather than
relying on those local-only files. This is distinct from the failing embedded three-dimensional
`Light`/`Flavor` case, whose supplied Region contains child-only `Sweetener`
and whose callback branches remain unreached.

Updated the DDM guide to state the contrast and added a flow chart for the
portable TCK's base `BaseValue`/X and derived `DerivedValue`/X-Y model. It
shows invalid Y-only use of the inherited attribute, then successful
per-attribute routing to X and X/Y receivers. The new and revised DDM charts
were rendered with Mermaid CLI 12.2.0 and screenshot-reviewed. The default-
region and partial-region default-range pair also has recorded 8/8 passes
across both callback models and its contract twins. Added a chart that
separates no publisher Region association from explicit partial-region
projection; the stored handle remains `{X}` while routing applies `[0,100)`
for omitted available Y. Reindexed all 34 DDM diagrams in the visual ledger;
the 2025 total is now 489 (285 flowcharts,
180 sequences, 24 state diagrams), all with current-body render and screenshot
records. This relies on recorded existing evidence; no new test or build was
run.

Also documented a portable interaction-send case with restricted default Y
range `[0,5)`: the explicit source Region stores X only, one receiver within
the default Y range gets the interaction, and an outside receiver gets none.
Its scenario and contract twin have recorded 4/4 passes across both callback
models. This distinguishes object-attribute routing evidence from the
interaction send path.

**Next handoff:** Continue cross-guide reader-gap review at the DDM/time and
subscription/process seams. Reconcile the current candidate source-link set,
then verify all 32 guide pages on GitHub. Preserve the 2010 boundary and
unrelated work.

## 2025 DDM/time and subscription/process cross-guide review, increment 141 — 2026-11-07

Reviewed the DDM/time seam across the DDM guide's “Keep time and DDM as
separate filters” section and the time-management guide's timestamped delivery
and grant paths. The DDM guide keeps spatial recipient eligibility, TSO logical-
time eligibility, and callback-model/enablement admission as independent
questions, and anchors a configured-process regional TSO sequence to focused
NMR, NMRA, and receiver-TAR cases. The observed process callbacks and grants
are described as bounded HLA_EVOKED evidence; the text does not imply that DDM
alone proves delivery or that all embedded and process profiles behave alike.

Reviewed the subscription/process seam across the object-attribute subscription
guide and process-endpoint guide. The subscription guide states that process
services reuse the registry's receiver-selection logic while event creation,
framing, and callback dispatch remain distinct. Its focused overlapping
subscription case supports one untimed reflection under both callback models,
while explicitly leaving passive three-role eligibility, delayed evaluation,
regional process projection, and embedded/process parity unproven. The process
guide's FIFO and callback-dispatch diagrams describe source scheduling and do
not elevate it to a portable HLA ordering rule.

No additional chart was warranted: the existing diagrams already show the
separate gates and their bounded evidence. The documentation link checker
resolved all 4,402 current worktree references across 33 2025 Markdown files.
No tests, builds, Requirements Lab, or 2010 content changed.

**Next handoff:** Reconcile the 4,402-reference source-link candidate with the
reviewed publication scope; preserve unrelated working changes. When the
current guide and cited source versions are available on a reviewed GitHub
revision, inspect all 32 rendered guide pages and record each result. Keep the
documentation goal at 3/4 gates (75%) until that hosted-render check is
complete.

## 2025 publication-candidate scope recheck, increment 142 — 2026-11-07

Rechecked the line-link dependency snapshot against the live checker and both
Git revisions. All 4,402 current references resolve in the worktree. The
snapshot exactly matches the current result: local `HEAD` has 103 unresolved
references across 44 targets; `origin/feature/initial-work` has 118 across 52;
15 references across 8 targets resolve at local `HEAD` but not on the hosted
branch. The worktree is three commits ahead of the hosted branch.

The 44 targets needed beyond local `HEAD` are all modified in the worktree: 25
tracked files and 19 untracked files. The tracked targets contribute 91,280
additions and 47,339 deletions versus `HEAD`; the generated C++ TCK catalog
alone contributes 80,538 additions and 46,858 deletions. The untracked target
sizes are not included in those totals. The worktree has 1,112 changed paths
overall, so a broad stage or push would mix the documentation dependencies
with substantial unrelated changes. The dependency snapshot itself is
validated; publication readiness is not.

No source or test changes were made, and no tests/builds, Requirements Lab, or
2010 material were touched. The current evidence moves the next action from
general candidate reconciliation to reviewing each cited target's ownership
and exact diff before assembling a narrow candidate.

**Next handoff:** Start with the generated catalog's cited entries and the 19
untracked cited sources; map every required target to its owning change and
review its diff for scope. Then review the 25 modified tracked targets. Only
after separating task-required changes from unrelated work should a
publication candidate be assembled. Preserve the Requirements Lab and 2010
boundaries, and keep GitHub rendering of all 32 guides as the final gate.

## 2025 roadmap history moved below the size limit, increment 143 — 2026-11-07

The active documentation roadmap had grown to 7,558 lines, above the
repository's 6,000-line documentation hard limit. Moved increments 1–80 into
the topic-audit archive and the completed visual, semantic, and source/test
review history from 2026-10-17 through 2026-11-06 into the historical-review
archive. Both retain their original ordering and wording, and stay beside the
guides so relative source links keep their original bases. The active roadmap
is now below the 3,000-line soft threshold. Preserved the exact increment 41
heading and the completed visual-review heading as short pointers because
other guides link to those anchors. No progress history was discarded.

Direct link checking found two source-line references in the historical-review
archive and both resolve; the topic-audit archive contains no source-line
references. The active 32-guide and roadmap checker remains the authoritative
check for current documentation links. This structural change does not alter
HLA behavior claims or diagram content. No tests/builds, source, Requirements
Lab, or 2010 content changed.

**Next handoff:** The active 2025 link checker resolves all 4,399 references,
and the dependency snapshot matches. Both preserved roadmap anchors resolve
from the guide index and link to the corresponding full archived entries; both
archive line-link checks pass. Resume increment 142 by reviewing the generated
catalog and 19 untracked cited targets by ownership and exact diff before
assembling a publication candidate. Keep GitHub rendering of all 32 guides as
the final completion gate.

## 2025 generated-catalog citation target repair, increment 144 — 2026-11-07

Reviewed the three catalog line references currently missing from local
`HEAD`. The cited lines had shifted into unrelated records as the generated
catalog grew: the FOM guide's additional-module Join citations landed in
timestamped-delivery records, and the interaction guide's closest-ancestor
TCK citation landed in an attribute-relevance record. Reidentified the
intended entries by their `cpp_test` IDs and normative section mappings:
`cpp-tck.fom-additional-module-join-atomicity` (Clauses 4.1.4, 4.1.4.1, and
4.11) and `cpp-tck.interaction-closest-ancestor-transition` (Clauses 5.10 and
6.1.9). Corrected all three guide links and refreshed their line numbers in
the hosted dependency snapshot. These records are beyond the current hosted
catalog's length, so the source dependencies remain pending publication.

The active 2025 line checker resolves all 4,399 worktree references; the two
FOM-guide links now share one unique path/line citation, and the corrected
catalog anchors identify the intended records. This was a
documentation citation repair only. No tests/builds, source code, Requirements
Lab, or 2010 content changed.

**Next handoff:** Review the 19 untracked cited sources by guide ownership and
evidence claim, beginning with the DDM portable-TCK fixtures/scenarios and
then the process, connection/TLS, handle-allocation, and MOM additions. Confirm
which are necessary to support the current guide text and isolate unrelated
worktree additions; do not stage or publish the broad worktree. Then review
modified tracked dependencies and assemble a narrow candidate for the 32-page
GitHub rendering check.

## 2025 DDM TCK dependency evidence review, increment 145 — 2026-11-07

Reviewed the four untracked DDM fixture/scenario sources cited by the DDM guide.
The interaction fixture declares Y as `[0..5)`; the interaction scenario
creates an explicit source Region over X only, confirms the public dimension
set remains X, expects one callback for a receiver within the FDD default Y
range, and none for the disjoint receiver. The object-attribute scenario
contains the inherited-attribute Y-only rejection (`InvalidRegionContext`) and
separate default-region, unspecified-dimension, and FDD-excluded-dimension
paths.

Read the existing, ignored TCK result JSON without rerunning tests. It records
primary/contract passes in both callback models for the inherited-dimension
case (4/4), excluded-dimension case (4/4), interaction partial-default-range
case (4/4), and the combined default/unspecified object-dimension cases (8/8).
The tracked scenario catalog identifies the corresponding §9.1.2 cases. These
are bounded Umbra adapter TCK observations, not independent-provider
conformance. The guide text and source anchors match the reviewed assertions;
the result JSON remains a local ignored artifact, so the guide's durable claim
is the recorded summary plus tracked test/catalog links.

No new test/build, source change, Requirements Lab edit, or 2010 change was
made. This closes the DDM portion of the untracked-target review, not the full
publication candidate review.

**Next handoff:** Review the remaining 13 untracked cited targets: process
resignation/runtime/recovery and transport history; TLS implementation/tests;
FOM handle allocation and encoding; and MOM subscription/report paths. Check
each guide claim against its exact source/test anchor, note any shared 2010
boundary, and keep the work read-only until the required target set is clear.
Then review modified tracked targets before assembling a publication
candidate.

## 2025 process-endpoint dependency review, increment 146 — 2026-11-07

Reviewed the process-related untracked citations across Federation Lifecycle,
Process Endpoint, and Callback/Service Ordering. The lifecycle guide's runtime
entry-point and client-probe citations match the described shared-service
startup and HLA_EVOKED callback observation, and explicitly limit those tests
to internal runtime behavior rather than portable TCK or authority-loss
continuity. The forced-resignation helper cited by the lifecycle guide calls
the registry grant re-evaluation path; the surrounding explanation preserves
the distinction between embedded tested cutoff behavior and configured-process
source trace.

The Process Endpoint guide's history citation is the bounded per-session
message-history implementation. Its four recovery-test anchors name the
documented cases: synchronization callback after recovery, terminal loss after
the retry window, Join-response replay, and pushed callback replay around a
service response. The guide labels these private transport tests and keeps
their scheduling outcomes separate from portable HLA ordering guarantees.
The recovery source is under the explicitly named 2025 integration-test file;
no 2010 behavior is inferred from shared transport internals.

This read-only review found the cited target/source roles consistent with the
guide text. No tests/builds, source, Requirements Lab, or 2010 material changed.

**Next handoff:** Review the remaining nine untracked targets: TLS socket
implementation/header and private/public TLS tests; FOM handle allocator and
its Catch2 cases; the interaction-send MOM subscription helper and S-leaf test;
and the portable TCK handle-encoding source. Verify each source/test anchor's
claim and edition boundary, then continue with the modified tracked targets.

## 2025 handle and MOM dependency evidence review, increment 147 — 2026-11-07

Reviewed the remaining untracked implementation and test references for the
FOM handle allocator, embedded MOM subscription processing, Table 7 S-leaf
parameter checks, and portable TCK handle encoding. The allocator guide's
source/test references describe collision-safe, persistent name assignments
and entropy-source failure as internal behavior; its Catch2 cases are tagged
internal-only with no normative mapping. The MOM helper dispatches the six
embedded subscription request families after validating federation membership
and decoding typed handle parameters. The S-leaf test enumerates 54 required
parameter-bearing MOM interactions and distinguishes those cases from
parameterless requests. The portable TCK helper checks HLAstandardMIM byte-array
round trips and typed public decoder round trips for each handle family.

The references are under the 2025 allocator/test and portable TCK paths. Their
guide text distinguishes internal mechanisms from public API evidence and
makes no claim of 2010 parity. The worktree link checker resolves all 4,400
references across 33 2025 Markdown files. This was a read-only documentation
review; no tests/builds, source, Requirements Lab, or 2010 material changed.

**Next handoff:** Review the modified tracked dependencies cited by the 32
guides, focusing on changes that alter claim meaning, edition ownership, or
line anchors. Reconcile the active hosted-source snapshot and backlog counts
before assembling a narrow publication candidate. The final goal gate remains
GitHub rendering verification for all 32 guides after review.
## 2025 dependency snapshot reconciliation, increment 148 — 2026-11-07

Rechecked the saved hosted-source dependency snapshot against the current
branch and ran the focused line-link checker at three levels. The worktree
resolves all 4,400 references across 33 guides. Local `HEAD` resolves 4,297;
103 references across 44 cited targets still need target versions beyond
`HEAD`. The hosted branch resolves all but 118 references across 52 targets;
15 references in eight targets are satisfied by the three local commits ahead
of hosted. The 44 local-HEAD targets comprise 25 modified tracked files and
19 untracked files. The dependency snapshot records the exact paths and line
numbers. This confirms the backlog's earlier 4,399/102/117 figures were stale
by one reference each.

The candidate remains mixed with substantial unrelated and implementation
work, so link success in the worktree does not establish a safe publication
set. The next review must reconcile each cited target with its exact source
change, identify task ownership, and keep unrelated staged/unstaged changes
out. No test/build, source, Requirements Lab, or 2010 change was made.

**Next handoff:** Review the 44 target diffs listed in the hosted-source
snapshot. Start with the generated catalog and modified source files whose
line anchors exceed `HEAD`, then distinguish guide support changes from
unrelated runtime work. Build a narrow candidate only after target ownership
is clear; then verify all 32 guide pages on GitHub.
## 2025 temporary publication candidate assembled, increment 149 — 2026-11-07

Built a temporary Git tree from `HEAD` using a separate temporary index; the
real branch and index were not changed. The candidate contains 82 paths: all
32 2025 behavior guides plus the roadmap, evidence records, and the 44 cited
target files from the hosted-source dependency snapshot. Its tree ID is
`90c128b53cac1cfec2b86d319ab47afe388437fa`. The focused checker resolves all
4,400 source-line references across 33 2025 Markdown documents against that
tree. The temporary tree therefore provides a reproducible link-complete
candidate state, but it does not prove the candidate's implementation changes
are appropriately scoped or tested, and it has not been published or checked
through GitHub's renderer.

The 44 cited targets include implementation, test, fixture, and generated
catalog changes; inclusion in this link candidate is not approval to publish
them. Review exact target diffs and ownership before creating a branch or
commit. No real index/staging changes, tests/builds, Requirements Lab edits,
or 2010 content were made.

**Next handoff:** Review the 44 source-target diffs for task ownership and
scope, beginning with the generated catalog and cross-cutting process/TLS
changes. Decide which target changes are required to support durable guide
evidence and which belong to separate implementation work. Rebuild the narrow
candidate from that reviewed set, then publish and inspect all 32 guides on
GitHub.
## 2025 process/TLS anchor meaning review, increment 150 — 2026-11-07

Reviewed the process-recovery and TLS source links against the exact worktree
statements. The 2025 client failure-handoff and callback-dispatch references
had landed on a catch-block brace and a blank line. The host-session creation
and resume-handshake links pointed to setup gaps before the corresponding
worker/resume code. TLS links labeled client setup, listener credential
consistency, server handshake, and no-plaintext-fallback pointed respectively
to an unrelated function boundary, socket close, session-state accessor, and
certificate-chain field. Corrected the nine guide references to the relevant
client TLS connect, listener validation, server accept, no-fallback contract,
client failure dispatch, background dispatch loop, host worker creation, and
resume handling statements. Updated the corresponding entries in the hosted
source dependency snapshot.

This is a source-anchor accuracy correction, not a change to the documented
behavior. 2025 worktree link checking is pending after this edit. The target
source and tests remain mixed with other implementation changes, so no
publication or 2010 parity is implied. No tests/builds or Requirements Lab
changes were made.

**Next handoff:** Rebuild the temporary 82-path candidate using the corrected
guides and dependency snapshot; verify that its line links all resolve. Then
continue exact-claim review of the remaining cited DDM, registry, FOM, MOM,
and generated-catalog targets. Keep implementation changes out of a publishable
set unless their ownership and required build/test integration are clear.
## 2025 candidate link gate after anchor repair, increment 151 — 2026-11-07

Rebuilt the temporary 82-path candidate after the process/TLS anchor repairs,
again using a separate temporary index. Candidate tree
`46c7822855c5a431d5143ed645663e29e4fe7579` resolves all 4,400 source-line
references across 33 2025 Markdown documents. The worktree checker independently
reports the same 4,400/4,400 result. The roadmap index parses as JSON and the
scoped diff whitespace check passes. The actual branch/index were not staged or
changed by candidate assembly. This validates local path/line targets only; it
does not validate build integration, implementation correctness, or GitHub
rendering.

**Next handoff:** Continue semantic source review of the 44 cited target
diffs, especially DDM region realization, registry attribute-update paths,
FOM allocator/TCK handle encoding, MOM reporting/subscription, and generated
catalog cases. Determine which source/test changes are necessary for the
documentation and which need separate implementation review. Then refresh the
82-path candidate and perform the GitHub rendering gate for all 32 guides.
## 2025 DDM/registry and catalog dependency review, increment 152 — 2026-11-07

Reviewed the outstanding DDM and registry references. The TCK fixtures and
scenarios match the guide's described FDD default ranges, unchanged stored
partial-region dimension sets, inherited-attribute dimension context, and
excluded-dimension routing. The registry additions realize omitted available
dimensions from FDD defaults and derive inherited-attribute context from the
class where the attribute is declared. The attribute-update recipient and
class-request planners call those per-attribute dimension and overlap paths.
The guide preserves the boundary that one inherited-context rule remains
source-only pending focused verification; it does not present that path as a
passing test or normative result.

The generated catalog links had also landed inside neighboring records rather
than on the cited scenario IDs. Corrected the additional-module Join scenario
to its record at line 79758 and the closest-ancestor interaction scenario to
its record at line 96274. The broad line scan also found links to a host-loop
brace and a blank scheduler-test line; corrected those to the host session
loop and test case. Together with the previous process/TLS fixes, the reviewed
source anchors now point to substantive statements or case declarations.
The worktree link checker resolves all 4,400 references across 33 guides.

These source/test/fixture and catalog changes are dependencies, not an
approved publication set. No source behavior, tests/builds, Requirements Lab,
or 2010 stream was changed in this review.

**Next handoff:** Continue reviewing the remaining cited targets: FOM allocator
and handle-encoding evidence, MOM/report tests, modified process and lifecycle
cases, and remaining TCK catalog/fixture paths. Refresh the temporary candidate
after those exact-claim reviews, then inspect all 32 guide pages on GitHub.
## 2025 FOM/handle and MOM evidence review, increment 153 — 2026-11-07

Reviewed the remaining FOM/handle and MOM targets against their guide passages.
The handle allocator references identify identity reuse and collision retries;
its focused Catch2 cases use an injected random provider and are explicitly
internal-only without normative mapping. The portable TCK reference identifies
the cross-federate encoded object-instance handle parameter scenario, including
byte preservation, typed decoding, and rejection by other handle decoders. The
embedded MOM subscription handler and S-leaf test references match the guide's
six request families and 54 parameter-required interactions, with separate
parameterless controls. The object-instance MOM report source additions expand
the described requester/target distinctions and NULL-response cases; the
related tests now contain HLA_EVOKED/HLA_IMMEDIATE variants, but this review did
not execute them and makes no pass claim.

Corrected the two MOM lifecycle citations to point at the later-Join reflection
assertion and Resign object-removal assertion. Across the reviewed process,
DDM, catalog, FOM, and MOM targets, the line checker passes; the semantic
review has found and repaired incorrect links that a line-count-only check
cannot detect. The documentation continues to mark source-only behavior,
unexecuted tests, and profile-specific evidence distinctly. No implementation,
Requirements Lab, or 2010 change was made; no tests/builds were run.

**Next handoff:** Complete semantic anchor review for the remaining cited
lifecycle, MOM-report, TCK, and generated-catalog targets, including test
integration dependencies. Review the 82-path candidate's exact path ownership
before publication; candidate link completeness alone does not establish
scope approval. Then verify all 32 guides on GitHub.
## 2025 source-link versus build-integration boundary, increment 154 — 2026-11-07

Audited build integration for the cited untracked C++ sources and tests. The
worktree registers the TLS socket, FOM allocator, process recovery, MOM S-leaf,
and DDM TCK additions through CMake source/target lists. The associated test
plan rows also exist in the current worktree. The prior 82-path temporary tree
included guide-linked sources and tests only; it omitted these CMake changes
and the test-plan changes. Its 4,400/4,400 result proves link resolution only,
not that the cited tests are part of a build or that their mappings are present
in a publishable revision.

This goal explicitly leaves the Requirements Lab untouched. I therefore will
not include its changed test plan in this documentation increment. Until the
source, build integration, and mapping work is independently reviewed and
published under an authorized implementation scope, the link-complete tree is
not a complete publication candidate. This keeps the guide's source references
accurate without treating source/test inclusion as approval to publish
behavioral changes. No Requirements Lab, implementation, 2010, test, or build
changes were made here.

**Next handoff:** Finish the exact-claim review of remaining lifecycle, MOM,
TCK, and catalog references, then reconcile which of their cited source/test
versions already exist in the hosted branch. Keep the documentation-only
candidate separate from CMake and test-plan changes. GitHub rendering can be
claimed only for the exact reviewed guide revision after all its cited targets
are available there; do not infer test/build readiness from the link checker.

## 2025 hosted-source and visual-ledger recount, increment 190 — 2026-11-07

Recounted the live hosted branch, `HEAD`, and worktree with the current
2025-only documentation link checker. The hosted branch is still at
`c9c5a9e` (three commits behind local `HEAD` `e6321e8`): 140 of 4,356 source
references fail against hosted, 123 fail against `HEAD`, and all 4,356 resolve
in the worktree. This confirms GitHub source-link readiness is still blocked by
unpublished targets; no publication or rendering claim is made.

Reconciled the diagram prose with the current identity ledger. It contains 489
diagrams across 32 guides (285 flowcharts, 180 sequences, 24 state diagrams),
and every entry records a current-body local render plus screenshot review.
Corrected the stale 486/487 wording in the visual ledger and completion
baseline. GitHub page rendering remains unverified. No builds, tests,
Requirements Lab edits, or 2010 changes were made.

**Next handoff:** Review the 123 currently failing `HEAD` source references by
guide and target ownership across the 31 modified tracked and 19 untracked
targets. Confirm each cited behavior still exists in the target revision
before choosing a hosted-safe citation; keep references to new behavior tied
to its source commit. Then reconcile the reviewed candidate against hosted and
verify GitHub rendering for all 32 current topic guides.

## 2025 citation-failure ownership classification, increment 191 — 2026-11-07

Classified the 123 references that fail against `HEAD`: all belong to 50
targets changed in the worktree. Thirty-one tracked targets are modified and
have citation lines beyond their `HEAD` length; 19 untracked targets are not
present in `HEAD`. This is a source-version mismatch, not evidence that the
current worktree guide citations are invalid: all 4,356 references still
resolve against current worktree targets. Repointing the current guides to old
line numbers could attach a claim to unrelated code, so no citation was
changed in this increment.

**Next handoff:** Review the 50 targets against their guide claims and owner
changes. Preserve references to newly introduced behavior until its source is
available on the hosted branch; for modified tracked sources, change a link
only when an equivalent behavior anchor is verified in that exact revision.
After source ownership and version are settled, rerun the hosted checker and
inspect GitHub rendering for all 32 guides at the same revision.

## 2025 configured-process TLS evidence cross-review, increment 192 — 2026-11-07

Reviewed the TLS 1.3 section of the Connection and Configuration guide against
the current TLS socket implementation and focused test declarations. The source
sets TLS 1.3 as the only negotiated version, distinguishes validated-server
and encryption-only modes, and configures the server certificate/key without
client-certificate authentication. The test sources contain five public
Connect cases and two private TLS/process cases. The recorded three-repeat
21/21 result is documented in the reader-review archive; the guide now links
directly to that evidence record. These settings are correctly labeled as
Umbra-specific, separate from standard HLA service semantics. The TLS source,
test, and CMake integration targets are not all present in `HEAD`, so hosted
source-link reconciliation remains open. No build or test was rerun, no
normative TLS claim was added, and no Requirements Lab or 2010 files changed.

**Next handoff:** Cross-review the FOM handle allocator references in the
2025 Name and Handle Resolution guide against the current allocator source and
focused tests. Establish which allocation guarantees are HLA-defined and which
are Umbra implementation details, then keep the new source/test citations tied
to their exact source revision.

## 2025 FOM handle allocator claim and test-gap review, increment 193 — 2026-11-07

Cross-checked the Name and Handle Resolution guide's allocator diagrams,
failure boundaries, and test summary against the current allocator, registry
commit paths, and internal Catch2 fixture. Confirmed the guide distinguishes
opaque API handle categories from Umbra's private `uint64_t` values, scopes
assignment reuse to the shared RTI-registry allocator, and labels the
post-failed-Join cached mapping as source inference rather than published
federation state. The fixture checks allocator-copy reuse, predefined reserved
values, Create failure without published state, and preservation/recovery of
the existing Join catalog and directory snapshots.

Added a focused known-gap note: the cited fixture does not inject zero, a
candidate collision with another identity's allocated value, or exhaustion of
the 256-attempt loop. These rejection paths remain source-derived. No tests or
builds were run; no HLA normative behavior or 2010 parity is inferred.

**Next handoff:** Cross-review the configured-process recovery evidence in the
Process Endpoint Request and Callback guide: compare its pushed-frame replay,
in-flight Join history, and bounded terminal-loss cases with the current
client/host history implementation. Keep socket recovery distinct from HLA
federate rejoin or restored execution state, and tie each conclusion to the
exact test evidence and source revision.

## 2025 configured-process recovery claim and run-provenance review, increment 194 — 2026-11-07

Cross-checked the Process Endpoint guide's recovery sequence against the
current client reconnect loop, host session resumption, sequence/message
history, and four focused recovery test cases. The guide correctly distinguishes
the same live process session from HLA Join or Save/Restore; it separates
host-queued event polling from pushed-frame replay; and it limits the tests to a
live host, with no claim for host restart or history exhaustion.

Found that the guide attributed the 2026-10-30 result to `ROADMAP-INDEX.json`,
which does not contain the detailed run record. The actual MinGW metadata is in
ignored local `out/compile-time` files. The archived review is now the durable
summary and records both facts: selected recovery CTests passed, while the
overall quality status failed on source-size blocking findings. The active
guide and review archive now make that distinction and the missing raw-output
limitation explicit. No tests or builds were rerun; no HLA normative or 2010
parity claim was added.

**Next handoff:** Review the configured-process forced-resignation path in the
Federation and Federate Lifecycle guide. Trace the private host-triggered
ConnectionLost event through the process client, runtime adapter, registry
membership cleanup, and focused cases; keep it distinct from voluntary Resign,
public connectionLost callback guarantees, and the separate 2010 stream.

## 2025 configured-process ConnectionLost path review, increment 195 — 2026-11-07

Reworked the Federation and Federate Lifecycle guide's compact process
ConnectionLost sequence. It now shows the server host's registry cleanup and
the local adapter's Not Connected transition/callback as independent results
of terminal socket failure, without suggesting that the host sends a callback
to the disconnected application or that either side is ordered before the
other. Kept the distinction from voluntary Resign and the profile-specific
survivor effects. The revised Mermaid sequence rendered locally with the
available Mermaid CLI.

Also distinguished terminal `connectionLost` from the private
`forceFederateResignationForTesting` fixture control. That control calls
`registry_.resign(NO_ACTION)` and sends a `federate_resigned` event while the
session remains usable; the process client clears its pending events and the
runtime reports `federateResigned`. Existing process TCK evidence covers an
eventual local `connectionLost` callback and selected survivor observations
under Push/both callback models, but not Pull, callback ordering, or lost-member
MOM-object removal. No builds/tests, Requirements Lab edits, or 2010 changes.

**Next handoff:** Reconcile this guide's new and existing source/test anchors
against the exact hosted revision, then continue a source-coupled review of a
high-interaction 2025 guide seam. Preserve hosted-link reconciliation and
GitHub-rendered verification for all 32 guides as explicit open project gates.

## 2025 hosted-revision audit of the configured-process ConnectionLost guide, increment 196 — 2026-11-07

Checked the live `feature/initial-work` ref with `git ls-remote`; it still points
to `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`, while the worktree is at
`e6321e8`. The current process lifecycle claims are supported by worktree
sources, but the hosted revision lacks the new
`process_federation_service_forced_resignation.cpp` and the newer
`federate_resigned` process-client event path. Its TCK source is only 700 lines,
so the current guide's ConnectionLost scenario anchor at line 2389 is not
present there. The host-test source is 115 lines on the hosted revision, so the
current anchors at lines 185 and 1333 are also not present at that revision.
Do not repoint these claims to an older line that happens to exist; it is not
the same test. The previous probe citation at line 1036 was semantically wrong
even in the worktree (it points into a directed-TSO setup), so it has been
replaced with the current ConnectionLost fixture's actual service call and
detach/close lines. The guide now labels the usable-session resignation event
fixture separately from terminal ConnectionLost.

The process host/service and registry implementation anchors checked in this
review exist in both revisions; source/test and event-handler anchors that
depend on the unhosted worktree changes remain pending publication. This is a
revision boundary, not a reason to weaken the documented evidence or claim
2010 parity. Both updated diagrams rendered locally; the worktree documentation
link check resolves 4,362/4,362 references and the roadmap index parses as JSON.
No builds/tests, Requirements Lab edits, or 2010 changes.

**Next handoff:** Review the partial ownership transfer/cancellation seam in
the 2025 Attribute Ownership guide against current worktree source and tests.
Keep individual focused cases distinct from the full transition matrix, then
continue hosted-source reconciliation once the worktree source changes are
available on a candidate hosted revision.

## 2025 partial ownership transfer and cancellation evidence review, increment 197 — 2026-11-07

Cross-checked the Attribute Ownership guide's regular-acquisition partial
transfer, If-Available remainder, and acquisition-cancellation race against the
current embedded registry transitions and focused Catch2 cases. The source
mutates desired sets and queued callback state per transferred attribute; a
regular request can retain `B`, while an If-Available request resolves its
remainder at its already-queued one-shot callback gate. The cancellation path
consumes only transferred members of its reservation, leaving other members
for the cancellation gate if no competing terminal response wins.

The guide already labels its multi-attribute outcomes as source-derived and
limits focused If-Wanted/cancellation-race coverage to one attribute. Added a
test-boundary note distinguishing the similarly named negotiated-divestiture
partial-cancellation case: it cancels `A` before the owner's confirmation
callback and later confirms `B`, so it does not exercise If-Wanted consuming
`A` from a live `{A, B}` cancellation reservation. Existing diagrams already
show the distinct request lifetimes and per-attribute terminal outcomes; no
additional chart was needed. No builds/tests were run; no 2010 or Requirements
Lab files changed.

**Next handoff:** Review the configured-process mixed callback-family boundary
in the Process Endpoint Request and Callback guide against current source and
focused evidence. Keep callback production, client buffering, dispatch model,
and per-session service responses distinct, and preserve the hosted-revision
gaps recorded in increment 196.

## 2025 process callback-family and service-response boundary review, increment 198 — 2026-11-07

Cross-checked the Process Endpoint guide's push/polling split, correlated
service responses, heterogeneous pushed-event FIFO, per-family polling
precedence, callback bridge, and dispatch-worker descriptions against the
current service, client, and runtime paths. The guide correctly treats
polling-family precedence as source-derived, distinguishes it from pushed
stream-read FIFO, and explicitly notes that the focused synchronization cases
do not test mixed-family reordering.

Added a short clarification that `HLA_IMMEDIATE` means inline on the thread
draining a callback submission; in this process profile that may be the
request/Evoke path or the separate callback-dispatch worker. It does not by
itself establish application-thread identity or ordering against a service
response on another session. The cited public interaction case observes
callback delivery but does not assert thread identity. No 2010 rule or
cross-profile behavior is inferred. The worktree documentation link check
resolves 4,366/4,366 references; the roadmap index parses as JSON and the
scoped diff check is clean. No C++ build or test was run.

**Next handoff:** Review the Join commit versus MOM discovery seam in the
Federation and Federate Lifecycle guide against the current Join transition,
discovery queueing, process rollback branch, and focused cases. Keep membership
commit separate from callback observation and preserve the hosted-revision
evidence limits from increment 196.


## 2025 Join commit, MOM discovery, and rollback boundary review, increment 199 — 2026-11-07

Cross-checked the Federation and Federate Lifecycle guide's Join paths against
the embedded and configured-process transitions and the focused process
rollback case. The guide keeps federation membership commit, process session
installation, discovery queueing, callback observation, and the public client's
local Joined transition distinct. The focused configured-process case injects
failure after discovery events are queued and after a pending synchronization
announcement is queued; it does not inject failure at the conditional
`HLAfederatesInFederation` update step.

Documented a source-derived gap at that membership-MOM update boundary: the
rollback discard helper filters pending discovery/reflection events by the
provisional member's `HLAfederate` object handle, while the conditional
member-count update concerns a separate federation-level MOM object. The
focused test subscribes to `HLAfederateName`, not the member-count attribute,
so no escaped stale update is claimed as observed. Added the distinction to
the configured-process sequence and the prose. Rendered the changed diagram
with Mermaid CLI and visually reviewed the output; refreshed its visual-ledger
body hash. The worktree documentation link check resolves 4,372/4,372
references across 33 IEEE 1516.1-2025 Markdown files. No builds/tests, 2010
changes, or Requirements Lab edits were made.

**Next handoff:** Reconcile the current 2025 documentation links against a
candidate publication revision, then verify GitHub rendering for all 32 topic
guides. Retain the hosted-source mismatch inventory from increment 196 and
separate source-link availability from Mermaid/GitHub presentation checks.


## 2025 publication-baseline link audit, increment 200 — 2026-11-07

Rechecked the 4,372 unique local path/line/guide references against the current
worktree, committed `HEAD`, and live `origin/feature/initial-work` (`c9c5a9e`).
The worktree resolves all 4,372. `HEAD` resolves 4,246 and leaves 126 across
51 targets (19 targets absent and 32 with anchors beyond committed file
lengths). The live branch resolves 4,229 and leaves 143 across 59 targets
(20 absent and 39 with out-of-range anchors). Thus the three local commits
ahead of the live branch reduce failures by 17, but they do not contain the
current documentation batch; they are TCK changes.

This audit separates worktree link health from publication readiness. A green
worktree check alone cannot establish that the cited implementation and test
revision exists on GitHub. The current checkout also contains unrelated staged,
unstaged, and untracked work, so this increment did not assemble or publish a
candidate tree. No source/test changes, builds, or test runs were made; 2010
and Requirements Lab remain out of scope.

**Next handoff:** Classify the 126 `HEAD` failures into (a) citations that can
be corrected to an equivalent committed source location and (b) targets that
must ship with the documentation. Build a task-only candidate from the exact
2025 guide, roadmap/index, visual ledger, and genuinely required cited targets;
rerun the reference checker on that candidate before attempting the 32-guide
GitHub rendering review. Preserve unrelated changes and the hosted-revision
comparison above.


## 2025 citation-closure candidate validation, increment 201 — 2026-11-07

Classified the `HEAD` failures from increment 200 against current worktree
ownership: the 19 targets absent from `HEAD` are untracked files, and the 32
targets with out-of-range anchors are tracked but modified in the worktree.
Built an isolated temporary-index tree from the 33 current 2025 Markdown files,
the visual ledger, and those 51 cited targets (85 paths total). The line-link
checker resolves 4,372/4,372 references against this tree. No Requirements Lab
path was included, and the real Git index and working files were not changed by
the candidate assembly.

This is citation-closure evidence only, not a publishable candidate: it omits
build/test integration and does not decide whether every dirty source target
belongs in the documentation release. No commit, push, build, or test was made.
2010 and Requirements Lab remain out of scope.

**Next handoff:** Review the 51 cited targets against source ownership and
current CMake/test registration. Separate the 19 newly tracked targets from
line-shifted tracked files, identify any task-related integration files needed
for a coherent candidate, and keep unrelated work excluded. Then build a reviewed
task-only publication candidate, rerun link checks against its exact tree, and
verify GitHub rendering for all 32 guides.


## 2025 cited-target build-registration audit, increment 202 — 2026-11-07

Reviewed the 19 target files absent from `HEAD` against current build
registration without compiling or running tests. All untracked implementation
and test `.cpp` targets are named in current CMake source/target files; the two
untracked headers are included by their owning translation units. The two
untracked DDM XML fixtures are mapped in the current package-adapter CMake
configuration, and the three untracked TCK `.cpp` targets are listed by the
TCK executable. This confirms current worktree registration references, not
that the targets build or pass.

The relevant registration is interleaved with broader changes: seven tracked
CMake files contain roughly 892 added and 35 removed lines, and the process
recovery CMake helper is itself untracked. A docs-only candidate that included
those files wholesale would include unrelated source/test integration. The
85-path citation tree therefore remains useful only as link-closure evidence;
there is not yet a reviewed, coherent publication candidate. No files were
staged, and no build or test was run. 2010 and Requirements Lab remain out of
scope.

**Next handoff:** Map each of the 51 cited dirty targets to its owning source,
test, fixture, and integration change; distinguish the exact required change
from adjacent work in the same files. Identify a coherent publishable batch
that preserves build/test registration and excludes unrelated staged work.
Then assemble that batch in an isolated candidate tree, run the link check
against its exact revision, and verify GitHub rendering for all 32 guides.


## 2025 catalog-semantic and complete local-path closure review, increment 203 — 2026-11-07

Audited all 13 current Markdown occurrences of links into the generated C++
TCK scenario catalog by reading the target scenario ID/title rather than
checking only line bounds. Twelve links pointed to unrelated scenarios or
status fields; corrected them to the intended additional-module Join, FOM
handle uniqueness, closest-ancestor interaction, regional DDM, and whole-class
or selected-attribute unsubscribe records. The remaining catalog link already
matched its Allow Relaxed DDM scenario.

The semantic corrections reduce the unique line-reference total to 4,371. The
worktree resolves 4,371/4,371; committed `HEAD` resolves 4,245 (126 unresolved
across 51 targets); the hosted branch resolves 4,228 (143 unresolved across
59 targets). A separate relative-file-link census found 795 unique local file
targets across 33 Markdown files, with 31 absent from `HEAD` but present in
the worktree. Built a fresh isolated 95-path tree containing the current 2025
Markdown, the visual ledger, 51 line-failure targets, and all 31 missing path
targets. Against that exact tree, all 4,371 line references resolve and all
795 local file targets exist. No Requirements Lab paths were included.

This is stronger link-closure evidence, not a publishable candidate: it still
omits broad CMake/test integration, and heading-fragment rendering was not
checked. No files were staged; no build, test, commit, or push was made. 2010
and Requirements Lab remain out of scope.

**Next handoff:** Review dependency closure for the 95-path tree against the
exact CMake/test integration changes, separating required registration from
adjacent work. Then verify local heading fragments and assemble a coherent
task-only candidate with its required build/test integration; rerun both path
and line checks against its exact tree before the GitHub rendering review of
all 32 guides.


## 2025 local heading-fragment validation, increment 204 — 2026-11-07

Ran a GitHub-style heading-slug check over all 33 current 2025 Markdown files.
All 740 local Markdown heading-fragment links resolve, including links to headings
containing the `HLA_EVOKED` and `HLA_IMMEDIATE` labels. The focused source-line
checker also resolves all 4,371 current path/line references across those files.
These checks validate the current worktree only; they do not prove hosted rendering
or that a coherent release candidate includes the referenced implementation and
test integration.

No source, test, Requirements Lab, or 2010 files were changed for this validation;
no build, test, commit, or push was performed. Existing unrelated staged and
unstaged work remains untouched. The overall completion estimate remains 75%
because GitHub rendering of all 32 guides is still unverified.

**Next handoff:** Continue the cited-target dependency review from increment 202.
Identify exact task-owned CMake/test integration needed for a coherent candidate,
without staging broad mixed changes. Then assemble the exact reviewed candidate,
rerun path, line, and heading checks against that revision, and verify GitHub
rendering for all 32 guides. Preserve the 2010 and Requirements Lab boundaries.


## 2025 cited-target registration boundary, increment 205 — 2026-11-07

Reviewed the current hosted-source dependency inventory against CMake registration.
It lists 44 source-line targets absent from committed `HEAD`: 25 modified and 19
untracked. Direct CMake references exist for 29 C++ source/test files. Twelve of
those registrations are absent from `HEAD` and are present only in the current
worktree: forced-resignation and process-runtime/transport-history sources, the
FOM handle allocator, TLS socket source, MOM subscription source, and the related
allocator, TLS, MOM subscription, process-recovery, and process-client probe test
sources. The other directly registered code targets already have registrations
in `HEAD`. The remaining cited targets are catalog, TCK, fixture, or header files
whose inclusion/consumption is not established by direct CMake path matches alone.

The required registrations are distributed across `CMakeLists.txt`, four Catch2
source/target files, and the untracked process-recovery CMake helper. The current
tracked CMake diff spans 11 files (444 additions and 38 deletions) and includes
unrelated package-smoke, websocket, and other integration work. Therefore, this
audit identifies the registration dependency but does not establish a safely
isolatable or publishable change set. No files outside the roadmap/index were
changed for this audit; no source, tests, builds, or Requirements Lab were touched.
The 2010 stream remains outside scope.

**Next handoff:** For each of the 12 newly registered code targets, map the exact
CMake addition to its owning source/test change and determine whether a narrow
patch can be extracted without adjacent CMake behavior. Check the three TCK XML
fixtures and TCK source targets against their actual runtime fixture loading and
source aggregation. Only then assemble the task-only candidate; rerun path, line,
and heading checks on its exact tree before attempting GitHub rendering of all
32 guides. Overall completion remains 75%; the GitHub-rendering gate is open.


## 2025 TCK fixture and runner dependency closure, increment 206 — 2026-11-07

Completed the direct-registration portion of increment 205. The 12 worktree-only
CMake registrations are present in the owning source manifests: production sources
in the root and RTI service-source lists; Catch2 sources in the executable and
connection/federation test lists; and the process-recovery test in its dedicated
helper included by the root build. The relevant CMake files also contain other
current integration changes, so their whole-file diffs remain unsuitable for an
unreviewed documentation candidate.

Traced the cited TCK dependencies separately. The scenario `.cpp` files are listed
in `packages/hla-rti-cpp-tck/CMakeLists.txt`; the three DDM XML fixtures are
selected by `tools/run_cpp_tck.py` and the current-package adapter CMake file, and
the multi-attribute fixture is also named by `tools/cpp_tck.py` and package runner
scripts. Thus fixture-link closure alone is insufficient: the candidate must carry
the matching runner/adapter selection changes and scenario source registration.
This audit establishes present references and registration, not successful build or
execution of those tests. No build or test was run.

**Next handoff:** Expand the candidate dependency manifest with the exact runner,
adapter, and package CMake files needed by the cited TCK scenarios and DDM FOMs.
For mixed source/CMake files, identify whether the related changes can be isolated
by existing commits or narrow patches without including adjacent work. Assemble a
temporary task-only tree only after that review, then validate all path, line, and
heading links against the resulting tree. Keep GitHub rendering of all 32 guides
unverified until checked on the exact hosted revision. Preserve 2010 and
Requirements Lab boundaries; overall progress remains 75%.


## 2025 committed-source citation feasibility audit, increment 207 — 2026-11-07

Compared the source line numbers of all 105 references that fail against committed
`HEAD` with the current target files. All 49 references into the 25 modified,
tracked targets point beyond the corresponding `HEAD` file length; the other 56
references point into 19 untracked targets. Therefore none of these failures can
be repaired by merely substituting the same line numbers. A docs-only candidate
that excludes the current implementation/test changes would still have broken
source links unless each claim is semantically mapped to a committed equivalent.

The current worktree remains link-complete (4,371 source-line links and 740
heading fragments resolve), but those checks do not establish committed-source
closure. No implementation or test files were changed, and no build or test was
run. The publication candidate and GitHub rendering remain open; overall progress
remains 75%. The 2010 and Requirements Lab boundaries remain intact.

**Next handoff:** Build a citation migration matrix for the 44 missing targets. For
each target, identify the current cited function/test/scenario and search committed
`HEAD` for the same behavior. Mark it as (1) semantically equivalent committed
location with a precise line, (2) newly introduced behavior that requires its
source/test change to ship with the docs, or (3) claim needing a wording/evidence
revision. Make only evidence-preserving documentation edits. Then recompute the
required candidate closure and verify links against its exact tree before GitHub
rendering.


## 2025 correction: citation inventory line semantics, increment 208 — 2026-11-07

Correction to increment 207: `HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json` stores
`references[].line` as the line number in the *guide containing the link*. It is not
the destination source line. The prior comparison incorrectly used those guide
line numbers against implementation file lengths, so its claim that 49 citations
were beyond `HEAD` is withdrawn.

Recomputed directly from every `#L...` Markdown destination in the current 33
2025 Markdown files using the focused link checker’s reference parser and Git tree
line counts. Current worktree: 4,371/4,371 source-line references resolve. Committed
`HEAD`: 4,245 resolve and 126 fail across 51 target files. Of those failures, 57
references point to 19 missing targets and 69 are out of range across 32 tracked
targets. The dependency inventory snapshot listing 44 targets/105 references is
therefore stale relative to the current Markdown and must be regenerated before
using it as a migration matrix.

The correct next step is target-based: for the 19 absent files, determine whether
to include their task-owned source/test/fixture changes or map each claim to a
committed equivalent; for the 32 line-shifted targets, verify and retarget each
claim to the matching committed code before changing links. No documentation links
were changed by this correction. No source/test/build, Requirements Lab, or 2010
changes were made. GitHub rendering remains unverified; progress remains 75%.

**Next handoff:** Regenerate the target inventory from the current 4,371 Markdown
references, retaining both the guide line and the destination line. Then take one
bounded family (start with the 19 absent targets), inspect each actual link and
claim, and record its disposition with precise evidence. Do not reuse the stale
44-target inventory or the invalid comparison from increment 207.


## 2025 correction: source destination lines and current migration inventory, increment 209 — 2026-11-07

Correction to increment 208: dependency `references[].line` is the *destination
source line*, consistent with `#L...` and the checker. It was not a guide line.
Increment 207 compared this field to target-file lengths correctly, but its 105
reference/44-target snapshot was incomplete and stale. Increment 208's withdrawal of
that comparison was itself incorrect and is withdrawn here.

Regenerated the inventory directly from the checker’s Markdown link pattern, with
both destination lines and actual guide link lines recorded separately. Current
counts are: 4,371 unique references; hosted branch 143 unresolved across 59
targets; local `HEAD` 126 unresolved across 51 targets; 17 hosted failures resolve
at local `HEAD`; current worktree 4,371/4,371. The exact `HEAD` failures split into
57 references to 19 missing targets and 69 out-of-range references across 32
tracked targets. The refreshed [dependency inventory](HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json)
now records the target line, guide location, target line counts, and revision status.

Started the migration review with FOM handle allocation. The guide claims allocator
identity reuse, reserved transport values, and atomic failure behavior, citing the
untracked allocator implementation and test. Exact `FomHandleAllocator` and
`secureRandomHandleValue` symbols are absent from `HEAD`; no committed equivalent
has yet been established. These citations remain source/test-dependent pending a
full equivalence review, rather than being redirected speculatively.

No implementation/test or Requirements Lab changes, builds, or test runs. The 2010
stream remains excluded. GitHub rendering remains unverified; overall progress is
75%.

**Next handoff:** Continue the regenerated migration matrix, beginning with the
remaining 17 absent targets. For each, inspect actual destination lines and guide
claims, search committed 2025 code/tests for behavior-equivalent evidence, and
record whether the current target must ship or the link/claim can be revised. Then
review the 32 out-of-range targets and assemble a candidate only from verified
task-owned changes.


## 2025 DDM fixture and scenario migration review, increment 210 — 2026-11-07

Reviewed four currently missing targets behind the DDM guide's excluded-dimension
and partial-default-range claims: two TCK FOM fixtures and two scenario source
files. The excluded-dimension case checks class-available dimensions, discovery,
and reflection while both FDD dimensions are marked `Excluded`. The partial
interaction case supplies `{X}` while the interaction's FDD gives omitted Y a
restricted `[0,5)` default, then distinguishes an overlapping receiver from an
out-of-default-range receiver. The object-attribute scenario separately covers
excluded dimensions, default-region behavior, and explicit partial source-region
projection; these are bounded API/TCK observations, not a universal routing rule.

The current package CMake lists both scenario sources; the current runner and
package adapter select the fixture names. The four targets are absent from `HEAD`,
and exact scenario IDs were not found in the committed TCK sources/catalog. No
semantically equivalent committed test+fixture was established, so these are
marked as task-owned source/test/fixture dependencies in the refreshed inventory.
Their scenario, fixture, runner/adapter mapping, and package CMake changes must be
reviewed together in a candidate. No guide links were redirected to weaker analogues.

The inventory now has six targets reviewed incrementally (the two FOM handle
allocator targets plus these four DDM targets), with 53 of the 59 hosted-failure
targets still pending classification. Worktree link checks remain 4,371/4,371; no
build or test was run. The 2010 and Requirements Lab boundaries remain intact.
Overall progress is still 75% because candidate integration and GitHub rendering
remain open.

**Next handoff:** Review the remaining 13 missing-at-`HEAD` targets, grouped by
feature family and actual guide claim. For each, verify the current source/test
registration and search for equivalent committed evidence before deciding whether
the target and its integration must ship. Then classify the 32 out-of-range targets
and assemble the candidate from the resulting complete dependency closure.


## 2025 configured-process TLS citation migration review, increment 211 — 2026-11-07

Reviewed four missing targets cited by the Connection and Configuration guide:
the internal TLS socket implementation/header and its platform and public
connection tests. The claims cover TLS 1.3 negotiation, server-chain and host-name
validation modes, credential configuration, listener/client setup, handshake
failure, and the absence of plaintext fallback. The guide explicitly labels these
as Umbra configured-process profile behavior rather than normative HLA rules.

All four targets are absent from `HEAD`; a committed-source search found no TLS
implementation, `TlsSocket`, or `tls://` source/test matches under `cpp/src` or
`cpp/tests`. The current CMake lists the socket implementation and focused test
targets. No committed equivalent evidence was established, so these source/test
dependencies remain necessary if those profile-specific claims stay in the guide.
They must remain clearly separated from the portable HLA model. The migration
inventory now records this disposition.

Ten of 59 hosted-failure targets are now reviewed incrementally; 49 remain pending.
Nine of the 19 targets missing at `HEAD` remain unreviewed. Worktree source links
remain 4,371/4,371. No builds or tests were run, and no source, Requirements Lab,
or 2010 changes were made. GitHub rendering and candidate integration remain open;
overall progress remains 75%.

**Next handoff:** Review the nine remaining missing-at-`HEAD` targets, grouped into
federation process lifecycle/recovery, MOM subscription behavior, and FOM handle encoding. For each,
confirm the exact HLA or Umbra-profile claim and its source/test evidence, then
record whether the new target and its CMake/fixture integration must ship or can
be replaced by an equivalent committed source.


## 2025 configured-process lifecycle and recovery citation review, increment 212 — 2026-11-07

Reviewed six missing targets cited by the Federation Lifecycle and Process Endpoint
guides: the shared service runtime declaration/definition, its independent client
probe, the private forced-resignation test control, bounded transport message
history, and the four-case recovery test. These claims distinguish a test-forced
federate-resigned event on a still-usable session from terminal `connectionLost`,
and describe configured-process-only resumption of host events, bounded terminal
recovery failure, deduplicated in-flight Join handling, and ordered replay of pushed
frames across repeated loss. The guides label these as Umbra process-profile and
private-test behavior; they do not imply portable RTI failover or 2010 behavior.

The exact runtime/control/history symbols and recovery test titles are absent from
committed `HEAD`. The recovery guide points to four focused tests and preserves the
historical test-run scope and limits. No committed equivalent for these precise
observations was established, so the six source/test/harness targets remain
required candidate dependencies if the claims are retained. No source or tests were
changed or run.

Sixteen of 59 failure targets now have dispositions; 43 remain pending. Only three
missing-at-`HEAD` targets remain unreviewed: the MOM S-leaf subscription source/test
pair and the TCK handle-encoding case. Worktree links remain 4,371/4,371. No build
or test was run; 2010 and Requirements Lab remain out of scope. Overall progress
remains 75% while candidate integration and GitHub rendering are open.

**Next handoff:** Finish those three missing-target reviews. Then switch to the 32
out-of-range targets and verify whether each cited behavior has an equivalent line
in committed 2025 sources/tests or needs its task-owned current file. Assemble the
reviewed dependency closure only after both groups have dispositions.


## 2025 MOM and handle-value missing-target review, increment 213 — 2026-11-07

Completed the missing-at-`HEAD` review for three targets. The embedded MOM
subscription-control handler supports six MIM interaction families; the source
method is absent from `HEAD`, and the guide limits its claim to the embedded 2025
path rather than generalizing `sharing=Subscribe`. The public Table 7 S-leaf test
checks missing-parameter exception reports across its 54 listed service leaves,
plus parameterless request controls. Its coverage and exception-subtype limits
remain explicit; no exact committed equivalent was found. The portable TCK
handle-valued parameter scenario checks byte preservation, receiver-local typed
reconstruction, and rejection by a wrong decoder; its source and scenario ID are
absent from `HEAD`. Related committed encode/decode tests are not equivalent to
this cross-federate parameter-transfer case. These three targets remain required
source/test dependencies for the exact evidence claims.

All 19 targets absent from committed `HEAD` now have migration dispositions in the
refreshed inventory. The remaining HEAD failures are 69 references out of range
across 32 tracked targets. Worktree source-line references remain 4,371/4,371; no
build or test was run, and no Requirements Lab or 2010 work was done. Overall
progress remains 75% because these 32 targets, candidate integration, and GitHub
rendering remain unverified.

**Next handoff:** Audit the 32 out-of-range tracked targets by matching every
citation to the current function, test, scenario, or catalog record and a committed
HEAD equivalent. Retarget only when the committed line supports the same claim;
otherwise retain the task-owned change as a candidate dependency or revise the
claim. Then assemble the complete candidate and validate it before GitHub review.


## 2025 Process-client committed-helper retarget, increment 214 — 2026-11-07

Replaced 10 Markdown link occurrences across four guides with immutable links to verified hosted lines for the unchanged 2025 process-client helpers: `dispatchLoop`, pushed-event draining, and deferred transport-failure dispatch. The corresponding source matches committed `HEAD`, and the GitHub permalink for the dispatch loop was opened and checked. The newly introduced terminal recovery-failure branch has no committed equivalent and remains linked to current work.

The current-worktree link checker resolves all 4,365 unique path/line/guide references. The regenerated dependency inventory reports 57 missing-target references across 19 targets and 63 out-of-range references across 32 tracked targets at `HEAD`; all missing targets have dispositions. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75% with candidate integration and GitHub rendering open.

**Next handoff:** Audit the 32 out-of-range targets by cited claim and committed equivalent, including the process-client recovery branch. Retarget only exact semantic matches; otherwise retain the required current-source dependency or revise the claim. Then assemble the reviewed dependency closure, rerun link checks, and verify GitHub rendering of all 32 guides.


## 2025 Callback-dispatcher committed-line retarget, increment 215 — 2026-11-07

Audited the three unique dispatcher references that were outside committed `HEAD`. The callback-dispatcher source is identical in local `HEAD` and hosted commit `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`. Retargeted the callback invocation claim to `invoke` at committed line 296, FIFO extraction and oldest-task removal to `takeNextLocked` at line 322, and immediate-drain exception cleanup to the catch path at line 279. The cleanup citation previously pointed at the extraction helper; its label and destination now agree. Other dispatcher links remain local because their current lines are valid.

The refreshed inventory reports 4,362 unique references, all resolving in the worktree. At local `HEAD`, 117 references remain unresolved: 57 across 19 missing targets (all dispositioned) and 60 out-of-range references across 31 tracked targets. Hosted failures are 134 references across 58 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%, with candidate integration and GitHub rendering open.

**Next handoff:** Audit the 31 remaining out-of-range targets by claim and committed equivalent. Retarget only exact semantic matches, and correct citations whose destination does not support the associated claim. Then assemble the reviewed dependency closure, rerun path, line, and heading checks, and verify GitHub rendering across all 32 guides.


## 2025 Directed-interaction registry citation retarget, increment 216 — 2026-11-07

The directed-interaction guide cited `federation_registry.cpp` line 2154, outside committed `HEAD`. Audited `EmbeddedFederationRegistry::timestampedDirectedInteractionRecipientFor`: its complete current method body is identical in the worktree, `HEAD`, and hosted commit `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`. Retargeted the guide to the immutable hosted method entry at line 2031. The departure-retention predicate is present at hosted line 2067, supporting the source-departure exception described by the claim. No claim or guide content was broadened.

The refreshed dependency inventory reports 4,361 unique local source references, all resolving in the worktree. At `HEAD`, 116 references remain unresolved: 57 across 19 missing targets (all dispositioned) and 59 out-of-range references across 30 tracked targets. The hosted branch has 133 unresolved references across 57 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Progress remains 75%; candidate integration and full GitHub rendering remain open.

**Next handoff:** Audit the 30 remaining out-of-range targets against their exact claims and committed equivalents. Keep current-only dependencies where no committed equivalent supports the claim. Then assemble the reviewed candidate dependency closure, check paths, line anchors, and heading fragments, and verify GitHub rendering for all 32 guides.


## 2025 Directed-interaction registry citation retarget, increment 216 — 2026-11-07

The directed-interaction guide cited `federation_registry.cpp` line 2154, beyond committed `HEAD`. The complete `EmbeddedFederationRegistry::timestampedDirectedInteractionRecipientFor` method body is identical in the worktree, `HEAD`, and hosted commit `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`. Retargeted the source-departure exception citation to the immutable hosted method entry at line 2031; the departure-retention condition is visible at line 2067. This removes one out-of-range reference without broadening the guide's claim.

## 2025 Attribute-update registry citation review, increment 217 — 2026-11-07

The attribute-value-update guide had two current line-630 citations to `candidateAttributeValueUpdateClassProvideRecipient`, used for owner partition and current-owner projection. Retargeted both to the hosted method at line 531. The unchanged owner checks and projection support those claims. The current method differs from committed `HEAD` at regional overlap: committed code calls generic `regionsOverlap`, while current code calls `objectAttributeRegionsOverlap` with the registered object class and attribute. No `objectAttributeRegionsOverlap` symbol exists in `HEAD`. Kept the current line-695 references for per-attribute region filtering and first-overlap suppression, and recorded this as a current-source dependency in the inventory; the guide continues to mark the behavior as an Umbra source observation pending verification of the active region-registry change.

After both reviews, the refreshed inventory reports 4,360 unique local source references, all resolving in the worktree. At `HEAD`, 115 references remain unresolved: 57 across 19 missing targets (all dispositioned) and 58 out-of-range references across 30 tracked targets. The hosted branch has 132 unresolved references across 57 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and full GitHub rendering remain open.

**Next handoff:** Continue auditing the 30 out-of-range targets against exact claims and committed equivalents. Distinguish unchanged behavior from current-only evidence and retain necessary dependencies. Then assemble the reviewed candidate closure, verify file, line, and heading-fragment references against that exact tree, and render all 32 guides on GitHub.


## 2025 Attribute-update planner citation review, increment 218 — 2026-11-07

The attribute-value-update guide's broad regional planner citations at current line 1122 were retargeted to the unchanged hosted regional class planner at line 992. The owner-grouping citations at current line 1200 were retargeted to the identical committed owner-map block at line 1069. The full current planner method differs from committed code only in the regional dimension preflight: committed code validates against class dimensions; current code validates each requested attribute against its available dimensions. Retained current line-1159 citations for that new per-attribute validation and its typed error path. The inventory records the remaining line-1159 dependencies and the exact difference.

The current-worktree checker resolves all 4,358 unique source references. At `HEAD`, 113 refs remain unresolved: 57 across 19 missing targets (all dispositioned) and 56 out-of-range refs across 30 tracked targets. The hosted branch has 130 unresolved refs across 57 targets. No builds, tests, Requirements Lab, or 2010 work were performed. The overall goal remains 75% complete, with candidate integration and GitHub rendering open.

**Next handoff:** Continue exact-claim review of the 30 remaining out-of-range targets. Retarget unchanged committed behavior and keep current-only links where the changed behavior is the evidence. Then assemble the reviewed candidate closure, verify local paths, line anchors, and heading fragments on that exact tree, and verify GitHub rendering across all 32 guides.


## 2025 Connect client-replacement citation retarget, increment 219 — 2026-11-07

The callback-ordering guide cited current `umbra_rti_ambassador_connect_services.cpp` line 324 twice for Connect replacing the retained process client. The exact assignment is at committed line 284; the assignment itself is unchanged between the worktree, `HEAD`, and hosted commit `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`. Retargeted both occurrences to that immutable line. The neighboring client-close and lifecycle citations remain separate evidence for shutdown and reconnect behavior.

The link inventory and checker now report 4,357 unique source references, all resolving in the worktree. At `HEAD`, 112 references remain unresolved: 57 across 19 missing targets (all dispositioned) and 55 out-of-range refs across 29 tracked targets. Hosted failures are 129 refs across 56 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and GitHub rendering remain open.

**Next handoff:** Continue auditing the 29 remaining out-of-range targets against exact claims and committed equivalents. Retarget only semantically exact lines and retain current-only evidence where behavior changed. Then assemble the candidate dependency closure, validate path, line, and heading-fragment links, and verify GitHub rendering across all 32 guides.


## 2025 Exception-report client recheck citation retarget, increment 220 — 2026-11-07

The exception-reporting guide's client recheck link pointed to current line 668, outside committed `HEAD`. Compared the complete `ProcessFederationClient::recheckExceptionReport` method: the worktree body is identical to `HEAD` and hosted commit `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`. Retargeted the citation to the method at hosted line 565. The process service handler and the focused concurrency/public behavior tests remain independently cited.

The worktree checker resolves 4,356 unique source references. At `HEAD`, 111 remain unresolved: 57 across 19 missing targets (all dispositioned) and 54 out-of-range references across 28 tracked targets. Hosted failures are 128 references across 55 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%, with candidate integration and GitHub rendering open.

**Next handoff:** Continue exact-claim review of the 28 remaining out-of-range targets. Retarget only committed code that supports the same claim and preserve current-only dependencies where behavior changed. Then assemble the candidate closure, validate local paths, line anchors, and heading fragments on that exact tree, and verify GitHub rendering of all 32 guides.


## 2025 Embedded send-interaction catch citation retarget, increment 221 — 2026-11-07

The exception-reporting guide cited the embedded `Send Interaction` wrapper catch at current line 2438. Compared the complete catch block with committed `HEAD` and hosted commit `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`: both hooks, followed by the service-report write and rethrow, are unchanged. Retargeted the link to hosted line 2395. This supports the documented catch order without depending on adjacent uncommitted additions.

The worktree source-link checker resolves all 4,355 local references. At `HEAD`, 110 remain unresolved: 57 across 19 missing targets (all dispositioned) and 53 out-of-range references across 27 tracked targets. Hosted failures are 127 references across 54 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and full GitHub rendering remain open.

**Next handoff:** Audit the 27 remaining out-of-range targets against exact claims and committed behavior. Retarget only exact matches and keep current-only links where behavior changed. Then assemble the candidate dependency closure, validate paths, line anchors, and heading fragments on that exact tree, and verify GitHub rendering across all 32 guides.


## 2025 Membership-loss final-report citation correction, increment 222 — 2026-11-07

A semantic review found that the service-invocation guide's membership-loss row linked “adapter appends and defers an error” to the synchronization-notification submission, and “post-cleanup error raise” to a report-queue call. Corrected the append link to the committed append call at `umbra_rti_ambassador_membership_loss.cpp` line 306 and the deferred raise to line 462. The separate synchronization guide now links its notification-submission claim to the committed call at line 421. These three statements match the current code's corresponding operations; line anchors were checked in the hosted revision.

The refreshed inventory reports 4,352 unique local source references, all resolving in the worktree. At `HEAD`, 108 refs remain unresolved: 57 across 19 missing targets (all dispositioned) and 51 out-of-range refs across 26 tracked targets. Hosted failures are 125 refs across 53 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%, with candidate integration and GitHub rendering open.

**Next handoff:** Review the 26 remaining out-of-range targets against their claims and inspect for semantic misanchors even when a line is in range. Retarget exact committed behavior and retain current-only dependencies. Then assemble the candidate closure, validate local paths, line and heading anchors, and verify GitHub rendering for all 32 guides.


## 2025 Process-transport semantic citation correction, increment 223 — 2026-11-07

The joined-federate lifecycle guide said a transport write failure closes its connection, but its “socket close and failure callback” link pointed to `ProcessTransportConnection::connectClient`. Corrected the prose links to the actual current `ProcessTransportConnection::fail` close branches at line 558 and failure-handler dispatch at line 566. The current failure path closes WebSocket/TLS wrappers before the native socket and then dispatches the failure callback. Committed `HEAD` has only native-socket close handling; the TLS/WebSocket references remain a current-only dependency and are recorded in the inventory.

This semantic correction increases `HEAD`'s unresolved count by one: the accurate close-branch anchor is beyond the committed file's line count. That is a correct inventory result, not a reason to keep the earlier in-range misanchor. The worktree checker resolves all 4,352 current local references. At `HEAD`, 109 refs are unresolved: 57 across 19 missing targets (all dispositioned) and 52 out-of-range references across 26 targets. Hosted failures are 126 refs across 53 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and full GitHub rendering remain open.

**Next handoff:** Continue auditing the 26 out-of-range targets while checking semantic accuracy for apparently in-range citations too. Retain current-only source links where behavior differs. Then assemble the candidate closure, validate file paths, line anchors, and heading fragments on that exact tree, and verify GitHub rendering across all 32 guides.


## 2025 Process-transport failure-handler citation retarget, increment 224 — 2026-11-07

Separated the process-transport failure behavior into committed and current-only evidence. The `failureHandler` call is unchanged and now links to hosted `process_transport.cpp` line 293. The current TLS/WebSocket close branches and TLS server credential/accept logic remain linked to current work because committed code contains only the native-socket path.

The regenerated dependency inventory reports 4,351 unique local source refs, all resolving in the worktree. At `HEAD`, 108 refs remain unresolved: 57 across 19 missing targets (all dispositioned) and 51 out-of-range refs across 26 tracked targets. Hosted failures are 125 refs across 53 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%, with candidate integration and GitHub rendering open.

**Next handoff:** Continue reviewing the 26 remaining out-of-range targets for exact behavior and semantic anchor accuracy. Retarget only unchanged committed behavior; retain current-only source dependencies where needed. Then assemble the candidate closure, validate local file, line, and heading-fragment references, and verify GitHub rendering across all 32 guides.


## 2025 RID authorization-profile citation retarget, increment 225 — 2026-11-07

Retargeted authorization profile-name selection, RID profile lookup, and reference-password-file evidence in the Authorization and Connection/Configuration guides to immutable committed source lines. Kept the generic authorizer-factory entry linked to current source because its provider-factory path is absent from hosted `HEAD`. The current link checker resolves 4,347/4,347 local references. At `HEAD`, 104 refs remain unresolved: 57 across 19 missing targets (all dispositioned) and 47 out-of-range refs across 26 targets. Hosted failures are 121 refs across 53 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and GitHub rendering remain open.

**Next handoff:** Review the 47 out-of-range references across 26 targets for exact semantic matches in committed source. Retarget only claims the committed lines support; preserve current-only citations where behavior changed. Then assemble and validate the candidate dependency closure, including file paths, line anchors, and heading fragments, and verify GitHub rendering for all 32 guides.


## 2025 Generic process-rejection response citation retarget, increment 226 — 2026-11-07

The FOM module-admission guide's generic-response citation pointed to the current `ProcessFederationService::rejected` helper at line 427, beyond the hosted file's 426-line length. Compared the complete helper in the worktree, `HEAD`, and hosted commit `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`: each returns `responseFor(request, TransportServiceStatus::rejected)`. Retargeted the citation to the hosted helper at line 411. Refreshed the dependency inventory; it now records the exact method family as reviewed.

The current checker resolves 4,346/4,346 local references. At `HEAD`, 104 remain unresolved: 57 across 19 missing targets (all dispositioned) and 47 out-of-range refs across 26 targets. Hosted failures are 120 refs across 52 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and full GitHub rendering remain open.

**Next handoff:** Continue reviewing the 47 out-of-range references across 26 targets for exact semantic support, and inspect apparently in-range references for semantic misanchors. Then assemble the candidate dependency closure, validate file paths, line anchors, and heading fragments on that exact tree, and verify GitHub rendering across all 32 guides.


## 2025 Process-client close citation audit, increment 227 — 2026-11-07

The callback-dispatch source was cited at line 661 as though it were client close; that line is actually inside `interrupt()` and synchronizes the response wait before notification. Corrected the affected close anchors. Retargeted the generic bridge-close and event/ACK queue-clear claims to the unchanged hosted close path at lines 647 and 638. Kept the ConnectionLost close and detailed teardown claims on current `close()` line 672: the current path also interrupts active and recovery-attempt connections and joins the heartbeat thread, behavior absent from the hosted implementation. The inventory records this split and the exact source differences.

The worktree checker resolves 4,344/4,344 local references. At `HEAD`, 104 remain unresolved: 57 across 19 missing targets (all dispositioned) and 47 out-of-range references across 26 targets. Hosted failures are 118 references across 52 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and full GitHub rendering remain open.

**Next handoff:** Continue reviewing the 47 out-of-range references across 26 targets and check in-range citations for semantic misanchors. Retarget only behavior that is actually present in committed source; keep current-only references for newer recovery and teardown behavior. Then assemble the candidate documentation closure, check file paths, line anchors, and heading fragments, and verify GitHub rendering across all 32 guides.

## 2025 process Push-frame citation audit, increment 228 — 2026-11-07

Retargeted the process Push-frame emission citation from the worktree line 430 to the immutable hosted source at [line 358](https://github.com/sheepfling/umbra/blob/c9c5a9e410539b039cd8bba35b53d0ef2b24d180/cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L358). The frame-emission block is unchanged at the pinned revision; the newer regional TSO retention code remains a separate current-source behavior. The pinned GitHub source page and anchor were checked.

The worktree checker resolves 4,343/4,343 local source references. At `HEAD`, 104 remain unresolved: 57 across 19 missing targets (all dispositioned) and 47 across 26 out-of-range targets. Hosted failures are 117 references across 51 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75% (3/4 completion gates); candidate integration and GitHub verification remain open.

**Next handoff:** Audit the configured-process TSO event-send and retraction-route citations, keeping the new callback-region copy lease current-only if it has no hosted equivalent.

## 2025 process TSO delivery citation audit, increment 229 — 2026-11-07

Retargeted the DDM event-send and TSO retraction-route citations to immutable hosted lines 381 and 393. The event-send and route-insertion block at current lines 425–439 matches hosted lines 381–395 exactly. Kept the interaction callback Region-designator copy lease at current source line 410: that lease is new, is keyed by message ID, and survives until receiver acknowledgement; acknowledgement, Resign, and Retract each have their own cleanup paths. The pinned GitHub source page was checked.

The worktree checker resolves 4,341/4,341 local source references. At `HEAD`, 104 remain unresolved: 57 across 19 missing targets (all dispositioned) and 47 across 26 out-of-range targets. Hosted failures are 115 references across 51 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and GitHub verification remain open.

**Next handoff:** Audit the directed-interaction service's planner, TSO construction/enqueue, event projection, and Push/Pull source citations against the current worktree, `HEAD`, and hosted source.

## 2025 directed-interaction source citation audit, increment 230 — 2026-11-07

Audited the six process directed-interaction citations across all three source snapshots. The local line numbers had drifted across the expanded worktree and committed source, leaving three labeled links on the wrong current lines. Replaced all six with immutable hosted links to the exact planner, TSO message construction, TSO enqueue, receive-order event construction, Push/Pull selection, and pushed-frame dispatch at lines 701, 754, 816, 857, 904, and 931. Each corresponding code block matches `HEAD`; the pinned GitHub page and source anchors were checked. These external links are excluded from the local line-reference inventory.

The worktree checker resolves 4,335/4,335 local source references. At `HEAD`, 104 remain unresolved: 57 across 19 missing targets (all dispositioned) and 47 across 26 out-of-range targets. Hosted failures are 112 references across 50 targets. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75%; candidate integration and GitHub verification remain open.

**Next handoff:** Check current-only attribute-scoped overlap helpers in the DDM guide against the worktree, `HEAD`, and hosted source; retain them only if the current code is the sole accurate reference.

## 2025 attribute-scoped region-overlap citation audit, increment 231 — 2026-11-07

Verified that the DDM guide's pairwise and default-region overlap citations at current lines 882 and 929 point to `objectAttributeRegionsOverlap` and `objectAttributeRegionOverlapsDefault`. Both helpers obtain the target attribute's available dimensions before realizing the region or regions. The helpers are absent from the 895-line `HEAD` and hosted versions: line 882 is in range at `HEAD` but names unrelated code there, while line 929 is beyond its end. Kept both links on current source and recorded the current-only disposition in the dependency inventory.

The worktree checker still resolves 4,335/4,335 local source references. At `HEAD`, 104 remain unresolved: 57 across 19 missing targets (all dispositioned) and 47 across 26 out-of-range targets. Hosted failures are 112 references across 50 targets. The inventory records 30 reviewed target audits and 23 pending. No builds, tests, Requirements Lab, or 2010 work were performed. Overall progress remains 75% (3/4 completion gates); candidate integration and GitHub verification remain open.

**Next handoff:** Audit the configured-process service-host citations at current lines 332, 357, and 362 in the lifecycle, connection/configuration, and endpoint guides. Compare each claim against worktree, `HEAD`, and hosted source; preserve current-only links where behavior is new. Continue the remaining migration reviews, validate the exact candidate documentation closure and its path/line/heading links, then verify GitHub rendering across all 32 2025 guides. Keep 2010 separate and Requirements Lab out of scope.

## 2025 configured-process host retention and purge audit, increment 232 — 2026-11-07

Audited the configured-process host source across the worktree, `HEAD`, and pinned hosted revision. Retargeted the stable per-connection worker and dispatch-loop citations to immutable hosted lines 167 and 182. Kept current-only citations for resume handling, inactive-session retention, `RTI_TIMEOUT_PURGE`, terminal-resume cleanup, and expanded shutdown cleanup. Recorded two in-range semantic traps: current host line 42 configures TLS credentials while hosted line 42 is constructor delegation; current line 170 is `stop()` while hosted line 170 starts a worker.

Corrected the process-loss state description in the lifecycle and joined-federate MOM guides. A normal receive-loop exit marks the host session inactive; a successful resume can reuse that retained session before purge. Purge and terminal-resume paths erase the host record before calling the loss adapter and call `Detach` only if notification projection succeeds. If projection fails after registry membership is removed, the failure is recorded, the adapter is not retried, and the service mapping may remain. Host shutdown has a separate path that detaches captured sessions even after recording a loss failure. The host tests cover successful replacement-socket resume (line 520), joined-session purge (line 614), and insufficient-history rejection (line 1261); no focused host test injects loss-projection failure.

Expanded the process-recovery documentation to distinguish the client's `FED_TIMEOUT_RECONNECT` budget from the host's `RTI_TIMEOUT_PURGE` retention clock. They start on different events; the focused cases cover resume and purge independently and do not test their timing race or host restart. Replaced five `#Lx-Ly` references that the current checker did not recognize with checked single-line anchors.

Rendered and visually inspected the revised recovery sequence locally, then updated its visual-review ledger entry with the current source line and body hash. GitHub rendering remains unverified.

The worktree checker resolves 4,346/4,346 local references. At `HEAD`, 120 remain unresolved: 57 across 19 missing targets (all dispositioned) and 63 out-of-range references across 26 targets. Hosted failures are 128 references across 50 targets. The dependency inventory records 31 reviewed target audits and 22 pending, with the large host test file still pending separately. No builds, tests, Requirements Lab, or 2010 content changed. Overall documentation progress remains 75% (3/4 gates); candidate integration and GitHub rendering remain open.

**Next handoff:** Review all cited cases in the separate 2025 process-host Catch2 target, which is 1,368 lines in the worktree and 115 lines at `HEAD`/hosted revision. Confirm the exact claims for the unjoined multi-client, session resume, purge, insufficient-history, and stalled-hello cases; keep that test-source migration review separate from the completed host-source review. Then continue the remaining 21 pending target audits, update the dependency closure and link checks, and verify GitHub rendering across all 32 2025 guides. Keep 2010 separate and Requirements Lab out of scope.

## 2025 process-host test-citation audit, increment 233 — 2026-11-07

Reviewed all six current host-test anchors cited across the lifecycle, callback-order, Federation MOM, and Process Endpoint guides. The unjoined two-client host case, multi-client ingress partition, joined-session purge, insufficient-history rejection, and stalled-New-Session shutdown claims match their tests. Tightened the replacement-socket claim: that case confirms the resume handshake preserves the session identity and a later request succeeds, but it does not force a nonempty replay suffix. The separate in-flight Join and pushed-callback recovery tests provide the nonempty replay evidence. Projection-failure injection remains untested.

The host-test source is 1,368 lines in the worktree and 115 lines in both `HEAD` and the pinned hosted revision. Recorded its current-only disposition separately from the already reviewed host implementation. The worktree checker resolves 4,346/4,346 local references. At `HEAD`, 120 remain unresolved: 57 across 19 missing targets (all dispositioned) and 63 out-of-range references across 26 targets. Hosted failures are 128 references across 50 targets. The dependency inventory now records 32 reviewed target audits and 21 pending. No builds, tests, Requirements Lab, or 2010 content changed. Progress remains 75% (3/4 gates); candidate integration and GitHub rendering remain open.

**Next handoff:** Review the two current process-recovery citations into `process_transport_session.cpp` across the Process Endpoint and Joined Federate MOM guides. Compare current line 261 and the joined-MOM session-receive citation with the 101-line `HEAD`/hosted implementation and the 456-line worktree source; identify exact behavior that can safely use a pinned link, retain current-only references for changed resume semantics, and update the dependency snapshot. Then continue the remaining 20 pending reviews and preserve the candidate-link and GitHub-rendering gates. Keep 2010 separate and Requirements Lab out of scope.

## 2025 connection-configuration citation audit, increment 234 — 2026-11-07

Audited the three configuration citations in the connection guide. The service-report directory, federation save-directory, and FOM-edition parsing blocks at current lines 363–470 match the pinned hosted source exactly at lines 224–331. Retargeted all three links to immutable hosted anchors. This also corrected a semantic misanchor: the old local `#L265` destination is TLS certificate-validation parsing in the expanded worktree, while the cited save-directory helper is at current line 403. The 2025 guide label now states only the 2025 default behavior; it does not describe the separate 2010 implementation stream.

The local source-link inventory drops by three references (4,343 remain), with no missing worktree links. Externalizing the two out-of-range anchors reduces the unresolved counts at `HEAD` to 118 references across 44 targets and at the hosted revision to 126 references across 49 targets. The inventory now records 34 reviewed target audits and 19 pending. No builds, tests, Requirements Lab, or 2010 implementation work were performed. Overall documentation progress remains 75% (3/4 completion gates); candidate integration and GitHub page rendering remain open.

**Next handoff:** Audit the 2025 C++ scenario-catalog references in the FOM admission and composition guide. Verify the exact catalog cases and current versus pinned entry mappings without reading the catalog wholesale, update the dependency inventory, then continue the remaining 19 target reviews and validate the candidate guide closure and GitHub rendering. Keep 2010 separate and Requirements Lab out of scope.

## 2025 C++ TCK scenario-catalog citation audit, increment 235 — 2026-11-07

Reviewed both unresolved catalog anchors across the FOM admission/composition and object/interaction guides. The former `#L80426` pointed into the unrelated custom-transportation record; the additional-module Join scenario is at current line 81203. The latter `#L96942` pointed into an unrelated timestamped directed-interaction contract; the closest-ancestor scenario is at current line 97719. Corrected the four guide occurrences to those exact records.

Kept both references on current catalog entries. The additional-module Join record now documents selected mappings to §§4.1.4, 4.1.4.1, and 4.11; the HEAD/hosted record for the same scenario lacks the expanded clause mappings. The closest-ancestor record now maps receive-order parameter projection to §5.10 as well as closest-class selection to §6.1.9; HEAD/hosted document only the latter. Those current mappings are needed for the guide claims. The worktree link checker still resolves 4,343/4,343 references; HEAD remains at 118 unresolved across 44 targets and hosted at 126 across 49. The inventory records 35 reviewed targets and 18 pending. No builds, tests, Requirements Lab, or 2010 changes. Overall documentation progress remains 75% (3/4 gates); candidate integration and GitHub rendering remain open.

**Next handoff:** Audit the current `process_federation_client.cpp` citation at line 1025 in the callback/service-ordering guide against the 1,429-line worktree file and the 982-line HEAD/hosted file. Confirm the specific failure-handoff behavior at that anchor and decide whether it has an equivalent pinned source location. Then continue the remaining 18 source-target reviews and the candidate-link/GitHub-rendering gates. Keep 2010 separate and Requirements Lab out of scope.

## 2025 process, handle, and time-management citation audit, increment 236 — 2026-11-07

Completed the process-client failure-handoff review: current line 1025 records a stopped stream reader and dispatches a detailed or generic terminal transport failure; HEAD's stream reader only stores the failure and wakes waiters. Kept that behavior current-only. Reviewed the 2025 FOM handle allocator and its internal tests: copied allocators share the private identity map, random candidates are collision-checked, and entropy failure leaves published Create/Join state unchanged. The focused tests verify reuse, reserved values, failed Create, unchanged Join snapshots, and successful retry; they do not inspect the allocator's private map after failure, so that point remains source-derived. No test was run.

Corrected two more citation families. The regional callback-gating cleanup link now points to the identical pinned publisher-resignation line at hosted #1081; current #1224 had an unrelated exception rethrow at HEAD #1224. The time-management guide's TAR entry now links to the pinned test at #735, and its immediate-TAR entry to #741. The old current #740 target was an NMR branch in the helper, not the immediate-TAR test. The TAR assertions match the cited sequence across snapshots; the current helper adds synchronized callback observation.

The worktree checker now resolves 4,340/4,340 local references. HEAD has 117 unresolved references across 43 targets: 57 missing-target references across 19 dispositioned targets and 60 out-of-range references across 24 targets. Hosted failures are 124 references across 47 targets. The dependency inventory records 37 reviewed and 16 pending target audits. No builds, tests, Requirements Lab, or 2010 work were performed. Overall documentation progress remains 75% (3/4 gates); candidate integration and GitHub page rendering remain open.

**Next handoff:** Audit the three current time-management references into `cpp/tests/ieee1516_2025_connection_tso_attribute_before_grant_catch2.cpp` at lines 1188, 1190, and 1194. Compare their exact test claims with the 1,240-line worktree, 1,197-line HEAD, and 1,179-line hosted file, then pin unchanged evidence or retain current-only behavior. Continue the 16 pending source-target reviews and validate the candidate link closure and GitHub rendering across all 32 2025 guides. Keep 2010 separate and Requirements Lab out of scope.

## 2025 process timestamped-attribute TSO test citation audit, increment 237 — 2026-11-07

Audited all references to the process timestamped-attribute test file across the callback-ordering, DDM, and time-management guides. The current 1,240-line worktree file has shifted scenario anchors relative to the 1,197-line HEAD and 1,179-line hosted snapshot; several old local lines now land on nonregional cases or unrelated test blocks. Compared the corresponding test names, invocations, and callback assertions across snapshots. The unchanged hosted cases support the cited claims: regional NMR at #1153, HLA_IMMEDIATE regional NMR/NMRA at #1159, regional NMRA at #1172, callback-disabled NMRA at #1128 with the disabled-Evoke/re-enable branch at #1015 and final reflection assertion at #1045, and the deferred process TSO update at #1115. Replaced nine guide-level local line anchors with immutable hosted links. No source or test files were changed, and no tests were run.

The worktree link checker resolves 4,331/4,331 local references. HEAD has 117 unresolved references across 43 targets: 57 missing-target references across 19 dispositioned targets and 60 out-of-range references across 24 targets. Hosted failures are 121 references across 46 targets. The dependency inventory records 38 reviewed and 15 pending target audits. Overall documentation progress remains 75% (3/4 gates); candidate integration and GitHub rendering remain open.

**Next handoff:** Audit the exception-report test citation into `cpp/tests/ieee1516_2025_federation_mom_exception_report_catch2.cpp` at line 362, referenced by the exception-reporting guide at lines 501 and 687. Compare the test claim and associated assertions with the 570-line worktree file and the 215-line HEAD/hosted versions, pin unchanged evidence or keep a justified current-only link, and update the dependency inventory. Continue the remaining 15 target reviews, candidate-link closure, and GitHub rendering for all 32 2025 guides. Keep 2010 separate and Requirements Lab out of scope.

## 2025 MOM exception-report citation audit, increment 238 — 2026-11-07

Reviewed every active-guide reference to the embedded MOM exception-report test and stabilized the archived review link. The worktree test file is 570 lines, while HEAD and the pinned hosted version are 215 lines. The current `#L362` points to `malformedObjectRequestParameterError.get()` returning true; it is not the timestamped example described by the guide. The matching timestamped case is unchanged in HEAD and hosted at lines 158–193: it enables integer64 regulation, sends Subscribe-only `HLAsetSwitches` at valid time 1, receives `InteractionClassNotPublished`, then observes one report after observer Evoke with `HLAparameterError=false`. The malformed/rejected test also has a stable hosted entry at line 5. Replaced the guide’s drift-prone local anchors with pinned GitHub links. No source edits, builds, or test runs.

The worktree link checker resolves 4,329/4,329 local references. HEAD has 116 unresolved references across 42 targets: 57 missing-target references across 19 dispositioned targets and 59 out-of-range references across 23 targets. Hosted failures are 120 references across 45 targets. The dependency inventory records 39 reviewed and 14 pending target audits. Overall documentation progress remains 75% (3/4 gates); candidate integration and GitHub rendering remain open.

**Next handoff:** Audit the FOM module-data MOM report citation into `cpp/tests/ieee1516_2025_federation_mom_fom_module_data_report_catch2.cpp` at line 121, referenced by `HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md` at line 1809. Compare the claim with the 189-line worktree file and the 110-line HEAD/hosted versions, pin unchanged evidence or retain a justified current-only reference, and update the dependency inventory. Continue the remaining 14 target reviews, candidate-link closure, and GitHub rendering across all 32 2025 guides. Keep 2010 separate and Requirements Lab out of scope.

## 2025 member-scoped MOM FOM-module report citation audit, increment 239 — 2026-11-07

Compared the member-scoped FOM-module request/report evidence across the 189-line worktree test and the 110-line HEAD/hosted test. The base case is unchanged: the member-scoped request carries the `HLAfederate` target and index 0, and the observer receives the selected embedded module report after Evoke. Replaced stale local anchors (including a current `#L4` that now lands in the include block) with pinned hosted links to the matching request, subscription, and report evidence.

The worktree test also adds an observation absent from HEAD/hosted: it enables `ConveyRegionDesignatorSets` and asserts the report contains one `HLAfederate` region, with bounds from the normalized target handle to its next value. Added that as explicitly current-only test evidence at worktree lines 122 and 147; the hosted test instead asserts no sent-region designator. No normative rule or cross-member behavior is inferred, and no test was run.

The worktree link checker resolves 4,328/4,328 local references. HEAD has 117 unresolved references across 42 targets: 57 missing-target references across 19 dispositioned targets and 60 out-of-range references across 23 targets. Hosted failures are 121 references across 45 targets. The inventory records 40 reviewed and 13 pending target audits. Overall documentation progress remains 75% (3/4 gates); candidate integration and GitHub rendering remain open.


## 2025 joined-federate MOM citation and handoff reconciliation, increment 252 — 2026-11-07

Compared the embedded public lifecycle assertions at current worktree lines 418
and 504 with the 333-line `HEAD` and hosted versions. `HEAD` and hosted content
at `c9c5a9e` are identical for this test; the corresponding assertions are at
hosted lines 237 and 296–297. The first confirms discovery/reflection of a
second joined member's MOM object through an already subscribed observer; the
second confirms successful Resign produces its removal callback and an invalid
producing-federate handle. These observations do not establish callback order
across profiles. Both guides already use immutable links to the matching hosted
assertions, so no citation edits were needed.

The source-link dependency snapshot already records this citation family as
reviewed and all 53 migration-target reviews complete. The prior increment
239 handoff to repeat this audit, including its 13-pending count, is stale.
Semantic link migration is complete, while assembling a publishable candidate
is still open: `HEAD` has 109 unresolved links across 37 targets (19 missing
files and 18 targets whose current cited lines exceed `HEAD`), and the five
test sources whose plan entries exist only in the worktree remain outside this
documentation goal. The worktree currently has 1,133 mixed paths. Progress
remains 75% (3/4 gates); the current candidate still needs publication and a
fresh GitHub render check for all 32 guides.

**Next handoff:** Review the source and build-integration hunks for the 14
missing targets outside the five plan-dependent tests, plus the 18 existing
targets with line growth. Use the dependency snapshot's exact paths and
dispositions; keep the Requirements Lab plan and its five test sources out of
this candidate. Then isolate the 2025 documentation/source closure, recheck
links, and verify all 32 GitHub previews. Preserve unrelated work and the 2010
boundary.

## 2025 process-coordinator source-closure review, increment 253 — 2026-11-07

Reviewed the process-coordinator runtime citation bundle through its source
and build-integration hunks. The production runtime source is explicitly in
the `umbra_rti` source list (`CMakeLists.txt:367`); the installed coordinator
executable is built from its main and linked to `umbra::rti`
(`CMakeLists.txt:613–620`). The shared process-service probe target includes
the client and runtime probe sources and links the RTI library
(`cmake/catch2_executable_targets.cmake:40–58`). The process-boundary Catch2
target includes the lifecycle test and depends on that probe
(`cmake/ieee1516_2025_federation_management_catch2_targets.cmake:186–192`).
The runtime header is consumed by both the installed executable and the
runtime probe, so this declaration boundary has two real consumers.

The guide's current test case at line 415 is newer than the 83-line `HEAD`
version of that test file. It checks that stopping the coordinator delivers
`connectionLost` to the joined parent and independent client through
`HLA_EVOKED`, and records both process outcomes. The client probe drives its
own callbacks with `evokeMultipleCallbacks` at line 84. The case is internal
regression evidence with no normative requirement mapping; no plan changes
were made. This was a static source/CMake review only, not a build or test.

This hunk review covers three of the 14 missing source-link targets outside
the five plan-dependent tests and one of the 18 line-growth targets. Their
links remain unresolved at `HEAD` until a scoped candidate contains the source
and integration changes. Eleven other missing targets and 17 growth targets
remain for hunk-level review. The hosted render gate remains at 75% overall.

**Next handoff:** Review
`cpp/src/internal/federation/process_federation_service_forced_resignation.cpp`
against the lifecycle guide's cited process-resign paths and its source-list
entry in `CMakeLists.txt`. Confirm the service call boundary and keep the
process-profile evidence distinct from embedded behavior. Then continue the
remaining 11 missing-target and 17 growth-target reviews, keeping the five
Requirements Lab dependent tests outside this goal.

## 2025 Process message-history and forced-resignation closure, increment 254 — 2026-11-07

Reviewed the process forced-resignation source and its CMake source-list entry. The private test control requests `NO_ACTION`, reports the resignation outcome, clears joined-session state, and emits the test-only `federate_resigned` event while the connection remains usable. The guide keeps this path distinct from terminal socket `connectionLost` behavior.

Reviewed the process message-history implementation and declaration, plus the focused internal protocol test citations. The history retains HLA service-message payloads under a byte cap; control frames can leave sequence-number gaps, and oversized or evicted entries can make a replay suffix unavailable. The guide now states those boundaries and identifies the cited tests as private transport evidence, not public HLA reconnect or conformance evidence. No test mappings or normative claims were added.

Rendered the changed recovery sequence with Mermaid CLI 12.0.0 and visually checked the screenshot. Its current body hash and line are recorded in the diagram review ledger; all 489 current diagrams remain locally rendered and screenshot-reviewed. The worktree link checker resolves 4,312/4,312 source references. `HEAD` has 115 unresolved references across 39 targets (61 across 20 missing targets and 54 across 19 line-growth targets); hosted has 119 across 42 targets. Source-closure hunk review now covers six of 15 missing-target paths outside the five plan-dependent test sources and two of 19 line-growth targets. Nine missing-target paths and 17 growth targets remain. No C++ build or tests were run; Requirements Lab and 2010 remain out of scope. Overall progress remains 75% (3/4 gates), with candidate publication and a fresh GitHub render check still open.

**Next handoff:** Review `cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_mom_subscriptions.cpp` and its source/build integration against the cited 2025 MOM control behavior. Then continue the remaining nine missing-target and 17 line-growth hunk reviews, keeping the five plan-dependent tests outside this documentation candidate. Preserve unrelated work and the 2010 boundary; after candidate closure, recheck links and all 32 GitHub guide previews.

## 2025 embedded MOM and TLS source-closure review, increment 255 — 2026-11-07

Reviewed the embedded MOM subscription handler against the Object and Interaction guide's six-command table and flowchart. The handler checks membership, recognizes one of six MIM service-class families, decodes supplied parameters, checks required values, and invokes the corresponding subscription method on the sending ambassador. The optional whole-class object-attribute unsubscribe behavior is present in the implementation. Its source is listed in the service-source CMake file, which is included only under the embedded-management gate for the 2025 `umbra_rti` target; the separate 2010 library has its own source list. No focused successful command-dispatch test was found. The guide already marks the handler as embedded-only and states that success-path test gap, so no claim expansion was needed.

Reviewed the configured-process TLS implementation and its cited tests against the Connection and Configuration guide. The client and server contexts enforce TLS 1.3. The default client mode verifies the server certificate using the system or configured trust store and checks the requested host; explicit encryption-only mode disables certificate validation while retaining TLS and SNI. Handshake deadlines and failure cleanup are implemented without plaintext fallback. The guide's five public Connect scenarios and two private TLS transport cases match their current test sources; the private cases are explicitly tagged with no normative mapping. CMake wiring adds TLS sources and OpenSSL links to the 2025 and embedded process targets. These are Umbra process-profile behaviors, not HLA requirements. The guide cites an earlier archived three-repeat run; no build or tests were rerun here. The five plan-dependent test sources remain outside this documentation candidate.

The worktree link checker remains 4,312/4,312 resolved. Source-closure hunk review now covers nine of 15 missing-target paths outside the five plan-dependent test sources and two of 19 line-growth targets; six missing-target paths and 17 growth targets remain. Progress remains 75% (3/4 gates): candidate publication and a fresh GitHub render check for all 32 guides are still required. No Requirements Lab or 2010 content was changed.

**Next handoff:** Review `cpp/src/internal/handles/fom_handle_allocator.cpp` and its CMake/test integration against the current 2025 name-and-handle guide. Check the 2025 ownership boundary and preserve any internal-test/no-normative-mapping status. Then continue the six missing-target and 17 growth-target hunk reviews before candidate assembly.

## 2025 FOM handle allocation source-closure review, increment 256 — 2026-11-07

Reviewed the current FOM handle allocator against the Name and Handle Resolution guide and its internal Catch2 source. The allocator forms length-delimited identity keys from category, containing name, and symbol; preserves assignments across allocator copies through shared registry state; reserves zero and the two predefined transportation values; and uses bounded OS randomness with no counter fallback. Create and additional-FOM Join use copies sharing that private assignment state. The guide distinguishes this Umbra allocation policy from the public typed handle API and records the untested zero, cross-identity collision, and 256-rejection branches as source-derived.

CMake adds the allocator to the 2025 `umbra_rti` target and adds its test source to the 2025 Catch2 target. The separate C++14 `umbra_rti_2010` source list does not contain it. The focused cases are internal-only with no normative mapping; no C++ tests were run. This review confirmed the guide's implementation/test boundary and did not change the guide or Requirements Lab.

Source-closure hunk review now covers 11 of 15 missing-target paths outside the five plan-dependent test sources and two of 19 line-growth targets. Four missing-target paths and 17 growth targets remain. The worktree link check remains 4,312/4,312; overall goal progress remains 75% (3/4 gates) until candidate publication and all 32 GitHub previews are freshly verified.

**Next handoff:** Review the two untracked 2025 DDM TCK fixtures (`ddm-region-realization-default-range-tck.xml` and `ddm-region-realization-excluded-dimensions-tck.xml`) against the Data Distribution and Regions guide and their CMake/TCK adapter consumers. Preserve the five plan-dependent Catch2 sources, avoid Requirements Lab changes, and keep the 2010 stream separate.

## 2025 DDM source-closure and attribute-request review, increment 257 — 2026-11-07

Completed the remaining 2025 DDM source-closure reviews outside the five
plan-dependent Catch2 sources. The default-range and excluded-dimension
fixtures and their portable TCK consumers support the guide's bounded
distinction: `[0..5)` constrains the default realization, while `Excluded`
keeps dimensions available without adding an implicit range. The inheritance
scenario verifies the available-dimension set and regional outcomes. Corrected
its citations so the link to regional subscription setup at source line 282 is
separate from the update call at 326 and per-attribute reflection assertions
at 346.

Reviewed the attribute-scoped regional class-request path and focused test.
The registry validates every supplied request region against the selected
attribute's declaration context before recipient planning. An empty request
set contributes no callback; explicit request/update regions are checked
pairwise in that attribute context, and the attribute is inserted once after
the first overlap. An attribute with no explicit update-region association
uses the default region. The focused test observes the empty-set no-op,
disjoint explicit-region filtering with default-region eligibility, and a
committed request-region change that suppresses the stale explicit callback
while preserving the default-region callback. These are bounded embedded
observations, not process-parity or normative claims. The registry sources
are in the `umbra_rti` source list; `umbra_rti_2010` remains a separate target.

The worktree link checker resolves 4,314/4,314 references. `HEAD` has 117
unresolved references across 39 targets (63 references across 20 missing
targets and 54 across 19 out-of-range targets); the hosted branch has 121
across 42 targets. All 15 missing-target hunk reviews outside the five
plan-dependent tests are complete. Growth-target hunk review now covers five
of 19 targets; 14 remain. The diagram ledger's shifted line fields were
refreshed after confirming the 35 DDM Mermaid bodies are unchanged. No C++
builds or tests, Requirements Lab edits, or 2010 edits were made. Overall
progress remains 75% (3/4 gates); publishing the scoped candidate and freshly
checking all 32 GitHub guide pages remain open.

**Next handoff:** Review `cpp/src/internal/federation/process_federation_client.cpp`
against the callback and service-ordering guide's cited paths and its source
integration. Keep configured-process evidence separate from embedded behavior
and preserve the 2010 boundary. Then continue the remaining 14 growth-target
reviews; after candidate closure, rerun the link checker and freshly inspect
all 32 GitHub guide pages.

## 2025 process-client terminal failure handoff review, increment 258 — 2026-11-07

Reviewed the configured-process client failure handoff at
`process_federation_client.cpp:1025`. After the receive loop exhausts recovery,
it records the terminal stream state and wakes waiting response/event paths.
While the client is not closing, it preserves an already queued transport
failure description or supplies the generic unrecovered-transport message,
then dispatches failures through the configured handler. The process client is
in the `umbra_rti` CMake source list; the separate `umbra_rti_2010` target has
no process endpoint.

Cross-checked the callback guide's test boundary. The production
coordinator-stop test observes terminal `connectionLost` delivery to joined
clients, including an explicitly evoked parent callback, but does not seed
pending work or assert relative callback order or queue cleanup. The separate
process-service probe drives server-side registry loss and detach; that does
not test the failed client's local callback queue. The guide already describes
these limits separately, so no prose change was needed. This was a static
source/CMake/test review; no C++ build or test ran and no test mapping changed.

Source-closure review now covers six of 19 growth targets, with 13 remaining;
all 15 missing-target reviews outside the five plan-dependent test sources are
complete. Link totals remain 4,314/4,314 in the worktree, 117 unresolved at
`HEAD`, and 121 hosted. Overall progress remains 75% (3/4 gates); candidate
publication and fresh GitHub rendering of all 32 guides remain open.

**Next handoff:** Review `cpp/src/internal/federation/process_federation_service_host.cpp`
against its lifecycle, connection, MOM, and process-endpoint guide citations
and CMake integration. Keep process-host behavior distinct from embedded
runtime behavior and preserve the 2010 boundary. Then continue the remaining
13 growth-target reviews and, after candidate closure, freshly inspect all 32
GitHub guide pages.

## 2025 process-host recovery and purge source review, increment 259 — 2026-11-07

Reviewed the process-host source and its lifecycle, connection, MOM, and
process-endpoint citations. A normal worker exit marks the session inactive;
the maintenance loop closes inactive connections and, after the purge age,
removes the host record before projecting connection loss and detaching on
success. An invalid resume takes terminal cleanup through `terminateSession`.
The guide correctly distinguishes those paths from host shutdown, which
attempts detach even after recording a loss failure. The focused purge test
checks that a joined member is removed after `RTI_TIMEOUT_PURGE`; the multi-
client and stalled-startup tests cover different host boundaries. No focused
test injects the loss-projection failure described in the guide.

The host source is compiled into the 2025 `umbra_rti` target; the 2010 target
has a separate source list and no process host. The current guide citations
match the relevant state transitions and tests, so no prose change was needed.
This was a static source/CMake/test review only; no C++ build or test ran.

Source-closure hunk review now covers seven of 19 growth targets; 12 remain.
All 15 missing-target reviews outside the five plan-dependent test sources
are complete. Link totals remain 4,314/4,314 in the worktree, 117 unresolved
at `HEAD`, and 121 hosted. Overall progress remains 75% (3/4 gates), with
candidate publication and fresh GitHub rendering of all 32 guides still open.

**Next handoff:** Review `cpp/src/internal/federation/process_transport.cpp`
against the process-endpoint, connection, and joined-federate lifecycle
citations, including its transport-failure and TLS listener boundaries. Keep
this configured-process behavior separate from embedded behavior and preserve
the 2010 boundary. Then continue the remaining 12 growth-target reviews and,
after candidate closure, freshly inspect all 32 GitHub guide pages.

## 2025 process-transport failure and TLS listener review, increment 260 — 2026-11-07

Reviewed the current `process_transport.cpp` anchors against the Connection
and Configuration and Joined Federate MOM Lifecycle guides. The failure path
closes the active WebSocket, TLS socket, or raw socket before invoking the
failure handler. Listener creation rejects a mismatch between the TLS address
scheme and supplied server credentials and validates supplied credentials.
The accept path applies a 30-second TLS handshake deadline; a failed handshake
closes the pending socket and returns no connection. The guide's transport
failure, TLS credential, and server-handshake claims match these paths.

The source is in the 2025 `umbra_rti` target and absent from the distinct 2010
target. Earlier TLS review covers the client trust policy, hostname check,
SNI, TLS version, and public/private test boundary; this pass did not rerun
those tests or any build. No guide correction was needed.

Source-closure hunk review now covers eight of 19 growth targets, with 11
remaining. All 15 missing-target reviews outside the five plan-dependent test
sources are complete. The worktree link checker remains 4,314/4,314 resolved;
`HEAD` remains at 117 unresolved references and hosted at 121. Overall progress
remains 75% (3/4 gates), pending candidate publication and fresh GitHub
rendering of all 32 guides.

**Next handoff:** Review `cpp/src/internal/federation/process_transport_session.cpp`
against its process-endpoint receive, resume-validation, and history-replay
citations. Keep configured-process transport state separate from HLA service
state and preserve the 2010 boundary. Then continue the remaining 11
growth-target reviews and, after candidate closure, freshly inspect all 32
GitHub guide pages.

## 2025 process-session resume validation and replay review, increment 261 — 2026-11-07

Reviewed the session anchors in the Process Endpoint guide against current
source and focused-test citations. The host validates the resumed session ID
and sequence-number forms, then checks whether its retained outgoing service
history covers the peer's reported receive point. It returns its last
received client sequence so the client can independently validate its own
retained suffix. Replay resends retained service frames after the peer's
receive point; unavailable suffixes fail recovery instead of replaying an
incomplete prefix. Control frames share sequence numbering but are not
retained as service payloads, so numeric sequence gaps alone do not prove a
history gap. The guide preserves that distinction and labels the mechanism as
private process transport, not an HLA guarantee.

The source is in the 2025 `umbra_rti` list, separate from the 2010 target.
Existing unit and process-host integration cases cover history availability,
successful resume, replay, and insufficient-history rejection, while the
guide records the untested deadline race and host-restart boundaries. No
C++ build or test was run and no guide change was needed.

Growth-target hunk review now covers nine of 19 targets; ten remain. All 15
missing-target reviews outside the five plan-dependent test sources are
complete. Link totals remain 4,314/4,314 in the worktree, 117 unresolved at
`HEAD`, and 121 hosted. Overall goal progress remains 75% (3/4 gates), pending
candidate publication and fresh GitHub rendering of all 32 guides.

**Next handoff:** Review `cpp/src/internal/runtime/rti_initialization_data.cpp`
against the Authorization guide's cited initialization and authorizer-policy
paths and its build integration. Confirm 2025 source ownership, keep the 2010
boundary explicit, then continue the remaining ten growth-target reviews.
After candidate closure, freshly inspect all 32 GitHub guide pages.

## 2025 authorization provider selection and fallback review, increment 262 — 2026-11-07

Reviewed the authorization factory-selection path, provider loader, CMake
ownership, and existing focused-test references. `HLAauthorizer` requires the
RID global password file; disabled or absent services reject an accompanying
password file. Other service names reject that file, then try an optional
Umbra provider library or the standard `HLAauthorizerFactoryFactory` fallback.
A configured but unusable provider rejects the RID; a loaded provider that
returns no matching factory permits the standard fallback. The environment
and library are resolved on first provider lookup, and the library remains
pinned for the provider instance lifetime.

The provider bridge is implementation-specific 2025 behavior, not an HLA
requirement. Its loader is untracked in the current worktree, appears in the
2025 `umbra_rti` and `umbra_authorizer` targets, and is absent from the separate
2010 target. No focused Catch2 case covers provider loading, fallback, or
provider-configuration failures. Corrected the stale Authorization design
note and added a factory-selection flowchart to the guide. Rendered and
screenshot-reviewed the new chart; the diagram ledger now records 490 diagrams.

The documentation link check resolves 4,316/4,316 worktree references; 119
remain unresolved at `HEAD` across 40 targets, and hosted has 123 across 43.
Growth-target hunk review now covers ten of 19 sources, with nine remaining.
The 15 previously identified missing-target reviews outside the five
plan-dependent test sources remain complete. No C++ build or test ran and no
Requirements Lab or 2010 files were changed. Overall goal progress remains
75% (3/4 gates), pending candidate publication and fresh GitHub rendering of
all 32 guides.

**Next handoff:** Review
`cpp/tests/ieee1516_2025_connection_process_federation_service_host_catch2.cpp`
against its 2025 Process Endpoint and Joined Federate MOM Lifecycle guide
citation claims. Keep process-host test behavior separate from HLA guarantees
and preserve the 2010 boundary. Then continue the remaining nine growth-target
reviews; after candidate closure, freshly inspect all 32 GitHub guide pages.

## 2025 process-host Catch2 evidence and timeout-source review, increment 263 — 2026-11-07

Reviewed all seven distinct test anchors cited from the process-host Catch2
source and confirmed its inclusion in the `umbra_ieee1516_2025_catch2` target.
The 2025 source list is separate from the 2010 RTI target. The Connect/Disconnect
guide link had drifted to L108, which now points into a frame-reading helper;
retargeted it to L185, the actual two-client Connect/Disconnect case.

The source distinguishes the scenarios: L185 connects two unjoined public RTI
clients and stops the host after both disconnect; L393 partitions two clients
from one ingress while a second ingress shares the same service and remains
usable; L520 checks same-session resume and a follow-up request without forcing
a nonempty replay suffix; L614 injects private 40/160 ms host intervals and
observes joined-member removal; L680 loads 40 ms heartbeat and 1 s purge values
from a private RID, observes retention after 120 ms and successful resume, then
eventual removal within a 3-second observation window; L1261 checks
insufficient-history rejection; and L1333 checks that shutdown completes while
a raw New Session peer is stalled. The L680 case does not isolate the heartbeat
value or assert exact timeout boundaries. No test was run in this documentation
review.

Updated the Process Endpoint and Federation Lifecycle guides to distinguish
the private-option purge case from the RID-loaded timeout case. The source
link checker resolves 4,318/4,318 worktree references; 122 remain unresolved
at `HEAD` across 40 targets, and hosted has 126 across 43. Growth-target hunk
review now covers eleven of 19 sources, with eight remaining. All 15 previously
identified missing-target hunk reviews outside the five plan-dependent tests
remain complete. No Requirements Lab or 2010 files were changed. Overall
progress remains 75% (3/4 gates), pending candidate publication and fresh
GitHub rendering of all 32 guides.

**Next handoff:** Review `cpp/tests/process_federation_service_probe.cpp`
against the Process Endpoint, Federation Lifecycle, and Federation MOM guide
claims that cite its controlled connection-loss and cleanup fixtures. Separate
fixture-driven loss from the production host path; preserve the 2025/2010
boundary and do not infer HLA requirements from internal test seams. Then
continue the remaining eight growth-target reviews before candidate assembly
and fresh GitHub inspection of all 32 guide pages.

## 2025 process-service probe fixture and source-link review, increment 264 — 2026-11-07

Reviewed the process-service probe anchors used by the Ownership, Joined
Federate MOM, Lifecycle, Process Endpoint, and Time Management guides. Corrected
the ownership cleanup link that had drifted to an unrelated stop helper, the
forced-loss links that had drifted into another probe, and the receiver callback
links that pointed into an ownership fixture. The callback links now identify
`runReceiver`'s callback bridge attachment and its official callback projection
assertions. Confirmed that the lifecycle's Federate-Lost report and registry
loss/dispatch/cleanup branches are controlled fixture behavior; they do not
establish production-host behavior or an HLA requirement. Time Management's
GALT-barrier anchor still lands in the intended process probe.

The source-link checker resolves 4,319/4,319 worktree references. At `HEAD`,
124 remain unresolved across 40 targets; hosted has 128 across 43. Growth-target
hunk review now covers 12 of 19 sources, with seven remaining. All 15 previously
identified missing-target hunk reviews outside the five plan-dependent test
sources remain complete. The 2025 local diagram ledger remains at 490 diagrams
across 32 guides. No C++ build/tests or Requirements Lab edits were made, and
2010 remains separate. Overall progress is still 75% (3/4 gates), pending
candidate assembly and fresh GitHub rendering of all 32 guides.

**Next handoff:** Review `cpp/src/internal/runtime/rti_initialization_data.cpp`
against the 2025 Authorization guide's initialization and authorizer-policy
claims and source/build integration. Confirm 2025 source ownership, then
continue the remaining seven growth-target reviews before candidate assembly
and fresh GitHub inspection of all 32 guides.

## 2025 RID timeout-profile and Federate-Lost evidence review, increment 265 — 2026-11-07

Completed the Authorization-source follow-up. Documented that the shared RID
parser handles both `[authorization.<profile>]` and Umbra's `[rti.<profile>]`
sections, including timeout conversion, profile selection, duplicate/unknown
setting rejection, effective purge-versus-heart validation, defaults, and
internal-only test coverage. The timeout syntax and rules are explicitly
identified as Umbra behavior. Corrected the provider-selection guide anchor to
the actual provider lookup call. The parser remains in the 2025 `umbra_rti`
target, separate from the 2010 target.

Rechecked the current Federate-Lost process TCK against the Lifecycle and MOM
guides. Besides exact loss-cutoff timestamp equality, TSO-before-grant order,
and post-loss member reflection/list assertions, the scenario registers and
completes a survivor-only synchronization point with an empty failed-to-sync
set. Added that bounded, current-process-fixture observation to both guides.
The hosted source/run record validates only the older hosted assertions; it
does not establish execution of current-source additions. The process case
still does not cover Pull or relative callback order, or assert retirement of
the lost member's MOM object. Refreshed the dependency inventory's guide-line
metadata after the prose edits.

The documentation link checker resolves 4,319/4,319 worktree references; 124
remain unresolved at `HEAD` across 40 targets, and hosted has 128 across 43.
Growth-target hunk review now covers 13 of 19 sources, with six remaining.
All 15 previously identified missing-target hunk reviews outside the five
plan-dependent test sources remain complete. The local diagram ledger remains
at 490 diagrams across 32 guides. No C++ build/tests or Requirements Lab edits
were made; 2010 remains separate. Overall goal progress remains 75% (3/4
gates), pending candidate assembly and fresh GitHub rendering of all 32 guides.

**Next handoff:** Review
`cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp`
against the Attribute Value Update Request guide's current regional-overlap
citation and 2025 source ownership. Compare the per-attribute dimension
filter and first-overlap suppression with the committed generic-overlap path;
preserve the 2010 boundary. Then continue the remaining six growth-target
reviews before candidate assembly and fresh GitHub inspection of all 32 guides.

## 2025 attribute-value-update recipient and region-realization review, increment 266 — 2026-11-07

Reviewed the current `candidateAttributeValueUpdateClassProvideRecipient`
path against the Attribute Value Update Request guide. The method still checks
ownership and class ancestry, then filters each requested attribute against
its request-region and update-region pairs. The current candidate replaces
the committed generic `regionsOverlap` predicate with
`objectAttributeRegionsOverlap`; a match ends the nested search and the
attribute handle enters a set once. This is a source-level behavior change,
not a normative one-callback-per-region rule.

The current region helper finds dimensions from the class where an attribute
is declared and its ancestors. During comparison it fills an explicit region's
missing available dimensions with the dimension's default range. Dimensions
declared only on a more-derived class are not added as defaults for an
inherited attribute. Updated the guide to describe this precise implementation
behavior and identify the remaining focused-test gap. The public process case
covers one matching pair and observes one Provide callback in both callback
models. The private process fixture changes one request region to a disjoint
range and observes zero recipients and no provider event. Neither process case
covers the inherited-dimension edge or multiple matching pairs with a
one-callback assertion. A portable TCK does cover a regional request for an
inherited attribute; that evidence is recorded in the DDM guide. No test was
run during this documentation review.

The recipient source is in the 2025 `umbra_rti` CMake target; the separate
`umbra_rti_2010` target does not include it. The source dependency inventory
now records the current-versus-committed predicate and the bounded evidence
limits. The guide remains explicit about Umbra implementation behavior and
does not infer process/embedded parity or a normative callback count.

The link checker resolves 4,319/4,319 worktree references; 124 remain
unresolved at `HEAD` across 40 targets, and hosted has 128 across 43.
Growth-target hunk review now covers 14 of 19 sources, with five remaining.
All 15 previously identified missing-target reviews outside the five
plan-dependent test sources remain complete. The 2025 diagram ledger remains
at 490 diagrams across 32 guides. No C++ build/tests or Requirements Lab edits
were made; 2010 remains separate. Overall goal progress remains 75% (3/4
gates), pending candidate assembly and fresh GitHub rendering of all 32 guides.

**Next handoff:** Review
`cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp`
against the Attribute Value Update Request guide's owner partition, request
filtering, and per-group projection claims. Check its focused evidence and
2025 CMake ownership; keep process behavior separate from embedded behavior
and the 2010 stream. Then continue the remaining five growth-target reviews
before candidate assembly and fresh GitHub inspection of all 32 guides.

## 2025 regional class-request validation and inherited-attribute evidence review, increment 267 — 2026-11-07

Reviewed `planAttributeValueUpdateClassRequest` against the Attribute Value
Update Request and Data Distribution guides. The committed implementation
checks every request region against dimensions available to the requested
class. The current source resolves available dimensions separately for each
attribute in the supplied region map, then validates every region's existence,
owner, committed state, and dimension context before planning any provider
group. This is a source-level change in the 2025 registry planner; it preserves
whole-request preflight before provider expansion.

Corrected an evidence statement in the request guide. Its focused process
selector uses `Soda.Flavor`, directly declared on `Soda`, so that particular
Catch2 test does not distinguish class-wide from attribute-declaration
context. A separate portable 2025 TCK scenario does: a Y-only region for
inherited `BaseValue` must be rejected by regional value request with
`InvalidRegionContext`. The scenario and contract twin passed under both
callback models, as recorded in the DDM guide. This covers the public inherited
request boundary. It does not establish process/embedded parity or the
recipient planner's behavior for multiple overlapping region pairs and its
single callback projection.

The planner source is included in the 2025 `umbra_rti` list and absent from the
separate 2010 target. Refreshed the source-dependency guide-line metadata and
updated the dependency note to preserve the current-versus-committed distinction.
No test or build was run during this documentation review.

The source-link checker resolves 4,319/4,319 worktree references; 124 remain
unresolved at `HEAD` across 40 targets, and hosted has 128 across 43.
Growth-target hunk review now covers 15 of 19 sources, with four remaining.
All 15 previously identified missing-target reviews outside the five
plan-dependent test sources remain complete. The local 2025 diagram ledger
remains at 490 diagrams across 32 guides. No Requirements Lab or 2010 changes
were made. Overall goal progress remains 75% (3/4 gates), pending candidate
assembly and fresh GitHub rendering of all 32 guides.

**Next handoff:** Review
`cpp/tests/ieee1516_2025_federation_mom_fom_module_data_report_catch2.cpp`
against the Federation MOM guide's member-scoped FOM-module report and region
designator assertions. Compare current and committed test claims, confirm the
2025 test-target boundary, and keep the Requirements Lab untouched. Then
continue the remaining four growth-target reviews before candidate assembly
and fresh GitHub inspection of all 32 guides.

## 2025 MOM report callback-model evidence review, increment 268 — 2026-11-07

Reviewed the member-scoped FOM-module report case against its current and
committed forms. The current 2025 Catch2 source enables
`ConveyRegionDesignatorSets` and checks one sent region, successful region
inspection, the `HLAfederate` dimension, and bounds from the normalized target
handle to its next value. The committed baseline instead asserts that no sent
regions are supplied. The Federation MOM guide already labels the additional
assertions as current test evidence rather than a new normative rule. The case
is compiled through the 2025 Catch2 target; no test was run during this review.

Reviewed the reflected-instance report case against the MOM Object and
Interaction Reports guide. The current test adds an `HLA_IMMEDIATE` companion
to its hosted `HLA_EVOKED` class-grouping case. The immediate path observes the
RTI report before `sendInteraction` returns; the evoked path observes it only
after draining callbacks. Both paths assert one Server and one Soda instance.
The worktree also contains a distinct-target NULL-response case. Updated the
guide to distinguish hosted baseline behavior from these current-only
assertions, preserve the no-normative-inference boundary, and note that this
documentation review did not run the test. Both sources remain in separate
2025 embedded Catch2 targets; no 2010 behavior is inferred.

The link checker resolves 4,321/4,321 worktree references; HEAD has 126
unresolved references across 40 targets (64 across 21 missing files and 62
across 19 out-of-range files), while hosted has 130 across 43 targets.
Growth-target hunk review now covers 17 of 19 sources, with two remaining.
All 15 previously identified missing-target hunk reviews outside the five
plan-dependent test sources remain complete. The local diagram ledger remains
at 490 diagrams across 32 guides; prose edits did not alter Mermaid bodies,
and their source-line metadata was refreshed. No C++ build/tests or
Requirements Lab edits were made; 2010 remains separate. Overall goal progress
remains 75% (3/4 gates), pending candidate assembly and fresh GitHub rendering
of all 32 guides.

**Next handoff:** Review
`cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp`
against the 2025 MOM Object and Interaction Reports guide. Compare the current
and hosted/HEAD report assertions, especially the worktree-only NULL response
and its target federate; confirm its 2025 embedded target boundary and keep
test-source evidence distinct from normative claims. Then complete the final
growth-target review before candidate assembly and fresh GitHub inspection of
all 32 guide pages.
## 2025 MOM report and catalog semantic-anchor review, increment 269 — 2026-11-07

Reviewed the updated-object-count report test against its current and hosted/
`HEAD` forms. The current 2025 embedded case adds an `HLA_IMMEDIATE` companion:
the report is observed before `sendInteraction` returns, while the hosted
`HLA_EVOKED` class-grouping case observes it after callback draining. Both
assertions are implementation-test evidence. The worktree also adds a
separate-target NULL-response case; it does not resolve the pinned MIM's
transport-specific empty-value wording. The guide preserves these limits and
makes no normative, process-parity, or callback-model equivalence claim. No
test was run.

A semantic review caught two catalog citations that passed line-range checks but
landed inside unrelated scenario records. Retargeted the additional-module Join
and closest-ancestor guide references to the current records' `requirement_mappings`
fields. The current catalog supplies clause mappings absent from hosted/`HEAD`;
the guide text labels that evidence as current-worktree-only. This is why line
range validation alone is not sufficient evidence that a source link supports
its claim.

The focused documentation link check resolves 4,324/4,324 unique
path/line/guide references across 33 IEEE 1516.1-2025 Markdown files. `HEAD` has
127 unresolved references across 40 targets (64 across 21 missing targets and
63 across 19 out-of-range targets); hosted has 131 across 43 targets. All 56
hosted source-link migration reviews and all 19 line-growth target reviews are
complete. All 21 targets absent at `HEAD` have bounded dispositions, and the 15
missing-target hunk reviews outside five plan-dependent test sources remain
complete. The local visual ledger covers 490 diagrams across 32 guides; its
source lines were refreshed against current Mermaid fences. No C++ build/tests,
Requirements Lab changes, or 2010 changes were made. Overall progress remains
75% (3/4 gates); scoped candidate assembly and fresh GitHub rendering remain.

**Next handoff:** Assemble the scoped 2025 documentation candidate, including
the reviewed guides, diagram ledger, source-dependency snapshot, and the exact
cited source/test closure. Keep unrelated Requirements Lab and 2010 changes out.
Run the documentation path/line/heading checks against that candidate and
inspect fresh GitHub rendering for all 32 guide pages. If the candidate cannot
be published without unrelated worktree changes, record that boundary and the
available rendering evidence before calling the goal complete. Keep the goal at
75% until this final gate is verified.
## 2025 GitHub GFM render and candidate-scope audit, increment 270 — 2026-11-07

Submitted the current Markdown body of each of the 32 2025 topic guides to
GitHub's Markdown API in GFM mode using the `sheepfling/umbra` context. All 32
requests returned HTTP 200 with non-empty HTML. This verifies GitHub GFM parsing
of the submitted text. It does not establish published file-page rendering,
resolve relative links in their source-file context, or run GitHub.com's Mermaid
hydration. The local ledger continues to provide current-body Mermaid render and
screenshot evidence for all 490 diagrams.

The candidate-closure audit found that all 32 guide files are modified in the
current worktree. At the pinned hosted revision `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`,
131 source-line references across 43 target files do not resolve. The exact
current target versions are available in the worktree: 20 modified files, 21
untracked files, and two files present at local `HEAD` but different from the
hosted revision. This is the source/test/catalog closure required for a hosted
candidate; it cannot be replaced by publishing the guides alone while retaining
those current-only line claims.

The worktree currently contains 1,142 changed or untracked paths, including
157 Requirements Lab paths. None of those unrelated paths was staged or copied
as part of this assessment; Requirements Lab and 2010 remain excluded. Local
link validation still resolves 4,324/4,324 references, and the GitHub GFM API
render pass covers 32/32 guides. No C++ build or test was run.

Progress is now **80% (4/5 gates)**: bounded survey/backlog, 32-guide content
and evidence review, local diagram rendering/screenshot review, and source-link
plus GitHub GFM API review are complete. The final gate is scoped candidate
assembly and inspection of all 32 actual GitHub file pages, including their
relative links and rendered diagrams.

**Next handoff:** Assemble a candidate from the pinned hosted base with the 32
2025 guides, the documentation-specific handoff/artifacts, and the 43 current
target files in the source-dependency inventory. Preserve unrelated work; do not
include Requirements Lab or 2010 changes. Validate all local path/line/heading
references against the assembled tree, then inspect each published GitHub file
page and Mermaid diagram. The GFM API pass and local screenshots do not replace
that page-level check. Keep the goal active at 80% until this final gate is
verified.
## 2025 scoped candidate assembly and hosted-base link verification, increment 271 — 2026-11-07

Built an isolated local candidate on `feature/hla-2025-docs-candidate-20261107`
from the pinned hosted base `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`. Its
80 selected paths are the 32 2025 guides, the 43 current source/test/catalog
target files needed by 131 hosted-unresolved citations, and five documentation
or validation assets. The global roadmap index was taken from the hosted base
and only its `documentation_flow_guide_handoff` object was merged from the
current worktree, preserving unrelated roadmap entries.

The candidate link checker resolves 4,324/4,324 path/line/guide references
across 33 Markdown files. All 32 guide bodies match the text submitted to the
GitHub Markdown API, which returned non-empty GFM HTML for each. The candidate
contains no changed Requirements Lab or 2010 paths. No C++ build or tests were
run. The candidate remains local and uncommitted; actual GitHub file-page and
Mermaid rendering has not yet been verified.

The candidate worktree is `C:/Users/peanu/GIT/sheepfling/umbra-hla2025-docs-candidate`,
owned by this documentation goal. It is retained as the reviewable scoped
candidate while publication approval and page-level rendering are pending.

Progress is **90%**: the survey, 32-guide documentation, local diagram review,
semantic source-link audit, GitHub GFM API pass, and scoped candidate assembly
are complete. The final 10% is publishing this candidate and verifying all 32
actual GitHub pages, including relative source links and diagram hydration.

**Next handoff:** The candidate is assembled and locally link-validated. Ask
whether to commit and push `feature/hla-2025-docs-candidate-20261107` so the
actual GitHub pages can be inspected. This separate candidate includes 43
current source/test/catalog target files needed by the guides; the earlier push
request concerned the `.inc` consolidation task. If authorized, publish only
this branch, then inspect all 32 pages and record any rendering or link issues.
Keep the goal active at 90% until that verification is complete.

## 2025 final local reference audit and ledger reconciliation, increment 272 — 2026-11-07

The isolated candidate is now a coherent 2025 documentation set: 88 changed paths comprising 32 topic guides, 43 source/test/catalog targets, four cited FOM fixture files, and nine documentation/validation assets. It adds the 2025 thematic reading paths as a separate page, includes the current render-audit snapshot and 2025 topic-review history, and repairs broken local navigation. The candidate changes no `2010`-named paths and no Requirements Lab paths; it preserves the explicit edition boundary.

The existing source-line checker resolves 4,324/4,324 source/test references across 34 IEEE 1516.1-2025 Markdown files. A scoped local destination/heading audit resolves 6,997 links across 35 Markdown files with no missing paths, headings, or Markdown line targets. The visual-review ledger matches all 490 Mermaid body hashes and diagram types; three source line positions were refreshed. The diagram bodies did not change, so the existing local render and screenshot evidence remains identity-matched.

GitHub's Markdown API returned non-empty GFM for the 32 guide bodies in the 2026-11-07 snapshot before seven guides received final prose and anchor cleanup; that edited text was not resubmitted. Published GitHub page rendering and Mermaid hydration remain open. No C++ build, test rerun, or CI was performed.

Overall progress remains **90%**. The candidate is local and uncommitted; the final 10% is publishing it and inspecting all 32 GitHub file pages, relative links, and Mermaid diagrams.


## Authenticated GitHub Markdown and Mermaid container check, increment 273 — 2026-11-07

The final text for all 32 IEEE 1516.1-2025 guide pages, the thematic reading paths, and this roadmap was submitted to GitHub's authenticated Markdown API in GFM mode. All 34 responses succeeded. GitHub emitted 490 Mermaid render containers for the 490 current fenced diagrams, with no count mismatch; the responses contained 909 heading elements and 7,224 links. The generic behavior index exceeds the API's 400 KB input limit and was excluded from this API pass; the separate local audit covered its destinations. The API output proves Markdown parsing and Mermaid-container registration, not visible diagram hydration on published file pages.

The source-reference audit remains 4,324/4,324 resolved, the local Markdown destination audit remains 6,997/6,997 resolved, and all 490 diagram body hashes/types match the visual-review ledger. The candidate remains local and uncommitted. Overall progress remains **90%**; page-level GitHub review is still the final gate. No C++ tests, builds, CI, or Requirements Lab changes were made.

**Next handoff:** After publication authorization, publish `feature/hla-2025-docs-candidate-20261107` and inspect all 32 GitHub guide pages for relative-link resolution and visible Mermaid diagrams. Record the page-level results in `HLA-2025-GITHUB-RENDER-AUDIT.json`; keep the goal active until that verification passes.


## Candidate source-dependency boundary review, increment 274 — 2026-11-07

The 88-path candidate contains 48 non-documentation paths: 43 source/test/catalog targets, four FOM fixtures, and one documentation-link checker. It changes no path under `compliance/requirements-lab` and no 2010 path. A scope conflict remains: five current-only C++ test sources are linked from 22 guide locations and included among the source-link targets, but the render audit's publication boundary says to exclude those sources. The candidate has not been published; no implementation files were changed or removed during this review.

**Next handoff:** Reconcile the five test-source paths against the stated documentation-only and Requirements Lab boundaries while preserving their primary-worktree changes. Then settle the exact publish path list. After publication is authorized, inspect all 32 hosted guide pages and Mermaid diagrams. The goal remains at 90%; the scope conflict and actual page-level check are open.


## 2025 guide coverage cross-check, increment 275 — 2026-11-07

A bounded text-presence screen covered all 32 current IEEE 1516.1-2025 topic guides. Every guide contains material on its normative basis, state or observation behavior, edge/failure boundaries, test evidence, and known limits. This checks topic coverage only; it does not prove each claim's correctness. Standard mapping, implementation citation, and test-evidence checks remain separate.

The 4,324 source-reference audit, 6,997 local-destination audit, and 490-diagram identity ledger remain verified. The authenticated GFM API recognized all 490 Mermaid blocks on the final text. Candidate path scope is still under review, and actual GitHub page rendering has not been verified. Overall progress remains **90%**.

**Next handoff:** Resolve the candidate source/test dependency scope without changing Requirements Lab paths. After publication is authorized, inspect all 32 hosted guide pages for link resolution and visible Mermaid diagrams. Keep the goal active until that page-level check passes.


## Candidate state and primary-worktree comparison, increment 276 — 2026-11-07

The remote `feature/initial-work` base remains at the pinned revision, and the documentation candidate branch is not present remotely. Of 48 non-documentation paths in the candidate, 47 are byte-identical to files in the primary worktree; the catalog file `compliance/catalogs/cpp-tck-scenario-catalog.json` differs. All primary worktree source changes were preserved. The five test sources flagged by the publication-boundary note remain part of the unresolved scope decision.

**Next handoff:** Reconcile the exact candidate path set and keep Requirements Lab paths untouched. After the scope is settled and publication is authorized, inspect all 32 actual GitHub guide pages and Mermaid hydration. Overall progress remains **90%**.


## Docs-only citation cleanup: Name and Handle Resolution, increment 277 — 2026-11-07

Five current-worktree-only source, test, fixture, and catalog targets were converted from hyperlinks to clearly labeled current-worktree evidence in the Name and Handle Resolution guide. Its ten Mermaid diagrams remain accepted by GitHub's authenticated Markdown renderer. The complete worktree source-reference audit now resolves 4,318/4,318 references. No implementation files, test sources, catalog entries, fixtures, or Requirements Lab paths were changed. The broader candidate still carries copied source targets; exact docs-only publication validation remains open.

**Next handoff:** Continue this cleanup across the remaining guides and validate a docs-only tree against the hosted base. Preserve implementation work. After publication is authorized, inspect all 32 hosted pages and Mermaid diagrams. Overall progress remains **90%**.

## 2025 docs-only link closure and final GFM verification, increment 278 — 2026-11-07

Removed current-only source links across the guide set while preserving the
implementation and test observations as explicitly bounded prose. The five
current-only test sources are now plain-text evidence references, not Markdown
links. The source-line checker resolves 4,192/4,192 references across 34
IEEE 1516.1-2025 Markdown files against both the candidate worktree and its
pinned hosted base. No source, test, catalog, fixture, or Requirements Lab
path was changed for this documentation pass.

The authenticated GitHub Markdown API was rerun for all 34 pages after the
citation cleanup. All 32 guide pages plus the thematic reading paths and
roadmap returned non-empty GFM; all 490 Mermaid fences emitted containers,
with zero API failures or count mismatches. The output contained 930 headings
and 7,381 rendered links. The large behavior index remained excluded because
the API rejects inputs over 400 KB. This verifies GFM parsing and container
registration, not visible Mermaid hydration on a published file page.

The candidate has a local commit containing exactly 40 documentation paths.
The worktree retains 48 unrelated non-documentation changes outside that commit.
The candidate branch is still unpublished; no Requirements Lab or 2010 paths are
selected. Overall progress remains **90%**.

**Next handoff:** The local docs-only commit is ready. Keep the 48 unrelated
changes outside it. After explicit publication authorization, push the candidate
branch to its own remote branch, then inspect all 32 GitHub file pages and
Mermaid diagrams. This local branch currently tracks origin/feature/initial-work,
so use an explicit destination ref when publishing. Update the render audit and
roadmap with the published revision and per-page results. The goal remains
active until this final page-level review passes.

**2025 Joined-Federate MOM source/test review, increment 279 — 2026-11-07.**
The guide's explicit Resign and forced ConnectionLost paths were checked against
the registry transition, route preflight, snapshot cleanup, callback-time
removal entry, configured-process dispatch, and focused tests. The source
confirms that ordinary Resign rejects an unavailable known-observer route
before mutation, while forced loss may skip that observer. With no planned
removal the snapshot is erased; if another observer has a planned removal,
callback entry can leave the skipped observer in the known set without a
pending removal. The bounded source trace found no retry. Embedded and
configured-process public MOM tests cover successful explicit Resign; the
reviewed 2025 connection-loss MOM tests do not assert removal of the public
HLAfederate object. The guide already states these limits accurately, so no
behavior text needed correction. The authenticated GFM audit's final totals
are 34 pages, 490 Mermaid containers, 930 headings, and 7,381 links. No C++,
test, 2010, or Requirements Lab paths were changed; all 48 unrelated
non-documentation worktree changes remain untouched. Progress remains 90%.

**Next handoff:** Review mixed callback families in the candidate's 2025
Process Endpoint guide against the configured-process source and focused tests.
Keep delivery routes and callback ordering distinct from the embedded path.
After explicit publication authorization, inspect all 32 GitHub file pages and
Mermaid hydration; that remains the final completion gate.

**2025 partial ownership transfer/cancellation evidence review, increment 280 — 2026-11-07.**
The candidate Attribute Ownership guide was traced across cancellation
reservation creation, Divestiture If Wanted's per-attribute reservation
consumption, the queued cancellation gate, and the focused callback tests. The
single-attribute race confirms that a transfer after accepted cancellation
suppresses cancellation confirmation and returns the acquisition notification
with the If-Wanted tag. The separate negotiated-divestiture tests cover
pre-delivery cancellation and a later NoAcquisitionPending result; they do not
exercise the same If-Wanted route. No focused test covers transferring only A
from a live `{A, B}` cancellation, so the remaining-B outcome stays explicitly
source-derived and no callback order is claimed. The guide's route distinction
and evidence limits are accurate; no behavior edit was needed. No 2010 or
Requirements Lab paths were changed. Progress remains 90%.

**Next handoff:** Review mixed callback families in the configured-process
2025 Process Endpoint guide against its source and focused tests, then leave a
specific next step. The final published-page rendering gate remains pending
explicit publication authorization.

## 2025 semantic source-anchor audit and progress recalibration, increment 281 — 2026-11-07

A candidate-wide screen found 5,482 relative line-anchored links across 36
changed Markdown files. Of these, 971 point into 50 source/test paths changed
since the candidate's pinned source base; 313 point into 15 paths with current
uncommitted edits. Range checks establish only that a line exists. A targeted
review of the configured-process mixed-callback section found source links that
resolved to unrelated code, so the earlier range-only result did not prove
semantic support. Corrected that section's anchors for event buffering, poll
result splitting, post-response dispatch, callback bridge submission, queue
extraction, and focused assertions. Remaining links require the same claim-to-code
review, with candidate-base source as the reference.

The overall estimate is revised to **80%** until the broader semantic citation
audit, configured-process Join/MOM evidence review, and hosted-page inspection
are complete. Local diagram identity/render review and authenticated GitHub GFM
container checks remain complete; visible Mermaid hydration on published pages
is not verified. No C++ build, tests, CI, 2010 paths, or Requirements Lab paths
were touched.

**Next handoff:** Continue the claim-to-source review for links into changed
source/test paths, starting with the remaining Process Endpoint sections. Check
the configured-process Join/MOM boundary afterward, preserve the 48 unrelated
worktree rows, and record exact semantic-review counts. Publish only after the
required publication authorization, then inspect all 32 hosted guide pages.

## 2025 configured-process Join/MOM source-anchor review, increment 282 — 2026-11-07

Tracing the configured-process Join flow against the pinned source revision
found several line links that landed inside Destroy, TARA dispatch, or pending
synchronization-announcement code instead of the Join setup they described.
Corrected the affected 2025 guides to point at registry membership commit,
federation MOM creation, report-file allocation, joined-member MOM setup,
session installation, discovery planning, and rollback cleanup. The edits span
the Federation MOM, Joined-Federate MOM, Federation/Federate Lifecycle, FOM
Module Admission, Time Management, and Attribute Ownership guides. This closes
that targeted anchor correction; it does not complete the wider semantic audit
of links into changed source/test paths. The estimate remains **80%**. No C++
build, tests, CI, 2010 paths, or Requirements Lab paths were touched; `git diff
--check` passed.

**Next handoff:** Continue the semantic claim-to-source audit in the remaining
Process Endpoint sections, then expand to the other 2025 guides that link into
changed C++ or test paths. Preserve all 48 unrelated worktree rows. The 32-page
hosted GitHub rendering and Mermaid hydration review remains pending publication
authorization.

## 2025 Process Endpoint pinned-source link review, increment 283 — 2026-11-07

A targeted review covered 51 Process Endpoint references into seven source/test
paths changed since the pinned base. Corrected the big-endian frame-length link
to the actual decoder, moved the FQG buffering link to the enqueue operation,
separated client event-queue and deferred-ACK cleanup, and linked the request
exception cleanup to its catch boundary. The current-worktree-only
`reconnectTransport` helper is now marked as such instead of linking its line
number to unrelated code in the pinned hosted source.

The documentation link checker now resolves 4,229/4,229 unique relative
path/line references across 34 Markdown files against the worktree, pinned
hosted base, and local source revision. A bridge-state citation is pinned to
the hosted revision and its target line was manually verified. These checks
confirm destinations and line existence only; the broader semantic review of
971 links into changed source/test paths remains open. Overall progress stays
at **80%**. No C++ build, tests, CI, 2010 paths, or Requirements Lab paths were
touched.

**Next handoff:** Continue the claim-to-source audit across the remaining
Process Endpoint sections and changed 2025 source/test targets. Preserve the 48
unrelated worktree rows. After publication is authorized, inspect all 32 hosted
guide pages and visible Mermaid hydration.

## 2025 Process Endpoint poll and send source review, increment 284 — 2026-11-07

Reviewed the explicit receive and callback-pump citations against the pinned
source. The polling source map now points to the interaction FIFO removal, the
Evoke source map points to the process-pump call, and the callback-pump section
links directly to its `receiveInteraction` call site. In the configured-process
Send Interaction failure map, the former adapter citation landed on a mutex
lock; it now points to the class-handle and parameter-handle validation. The
timestamped directed-send finite-time citation now targets the actual initial
and final-time predicate. The deferred attribute-update flow now links both to
the bridge's claim-callback invocation and to the admitted task body. These
changes distinguish exact validation and dispatch operations from nearby
function entry points.

The source-link checker resolves 4,233/4,233 unique relative references across
34 Markdown files against the candidate worktree, pinned base, and current
source revision. A separate local check resolves all 123 immutable GitHub
source permalinks (79 unique path/line pairs across 35 targets) at the pinned
revision. The current cross-revision inventory is 847 references into 35
changed targets across 33 documents (809 relative links and 38 pinned
permalinks; 56 source/test paths changed). The Process Endpoint guide has 103
references into 18 changed targets. These counts describe link destinations;
they do not mean every surrounding claim has been reviewed. The authenticated
GFM check still predates these link edits. Overall progress remains **80%**.
No C++ build, tests, CI, 2010 paths, or Requirements Lab paths were touched;
`git diff --check` passed.

**Next handoff:** Complete the remaining Process Endpoint claim-to-source
review for TSO delivery, acknowledgement and retraction, plus request-exception
handling. Then review the highest-density changed-source guides: Time
Management, Callback and Service Ordering, and Data Distribution and Regions.
Keep source claims tied to the pinned 2025 revision and label any
current-worktree-only evidence. Preserve all 48 unrelated worktree rows;
refresh GFM after the remaining Markdown edits. Publish only with
authorization, then inspect all 32 hosted guide pages and Mermaid hydration.
