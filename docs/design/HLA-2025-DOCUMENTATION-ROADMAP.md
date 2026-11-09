# IEEE 1516.1-2025 Documentation Roadmap

This roadmap tracks the remaining work to make Umbra's 2025 HLA discoveries
clear and durable for readers. The guides lead with HLA concepts; implementation
and focused-test evidence is included only to explain an observable boundary or
to show what the current evidence does—and does not—establish.

## Current baseline

As of 2026-11-09, the 2025 topic set contains 32 guides (the timing guide and
31 other behavior areas) with 507 Mermaid diagrams: 295 flowcharts, 187
sequences, and 25 state diagrams. The five-needs content audit is complete for
32/32 guides. All 507 diagram identities have local render and screenshot
evidence. Increment 356 verified GitHub-page hydration for 507/507 diagrams
after viewport scrolling; the [visual ledger](HLA-2025-DIAGRAM-VISUAL-REVIEW-LEDGER.json)
and [GitHub render audit](HLA-2025-GITHUB-RENDER-AUDIT.json) record the results.

Increment 343 audited additional-module Join preparation, commit, process
setup, and rollback against candidate source and focused tests. It corrected
FOM guide source/test anchors and distinguished queued observer notifications
from callback completion.

Increment 344 audited configured-process Restore admission and pushed
notification delivery. A frame-send failure after registry admission returns
`internalError` without a handler-level rollback; earlier successful sends may
remain. The guide labels this as source-only and records that the focused
success and missing-image cases do not inject a send failure. Added one
flowchart and corrected Restore test links to direct assertion lines. The
existing late state-application failure and receive-order replay explanations
were also checked against their cited source and test boundaries.

Increment 345 audited configured-process Restore completion after registry
state replacement. Recipient/session failures during saved-event replay and a
failed pushed attribute-frame send return `internalError` without handler-level
rollback; the pushed attribute entry is inserted before its send. The guide
distinguishes interaction queueing from pushed attribute delivery and traces
request-buffered callback dispatch before public error mapping. Existing focused
tests cover successful replay, not these failure branches. The full ledger audit
also found two earlier chart-body edits in the Callback and TSO Retraction
guides with stale hashes; both were rerendered and screenshot-reviewed, and
their ledger identities were reconciled.

Increment 346 reviewed the boundary between configured-process socket resume
and Federation Save/Restore. The Save/Restore and Process Endpoint guides now
link their explanations and compare retained live-host transport history with
supported image state and explicitly captured queue records. They identify
client FIFO and callback-bridge tasks as separate runtime state and keep
host-process restart recovery unproven. The recovery source and tests remain
labeled as current-worktree evidence.

Mermaid CLI 12.0.0 rendered the new chart and screenshot review found it
readable without clipping. The ledger now matches all 505 diagram bodies,
headings, types, fences, and source lines. The worktree checker resolves 4,566
unique path/line/guide references across 35 Markdown files, with zero
unresolved. No C++ source, builds, tests, CI, 2010, or Requirements Lab content
changed. Overall progress remains 80%; formal gates remain 3/4 (75%). The
current candidate remains unpublished.

Increment 347 audited terminal configured-process ConnectionLost across the
Process Endpoint and Federation/Federate Lifecycle guides. A normal serving-loop
exit marks the host session inactive and retains it for same-session Resume
until `RTI_TIMEOUT_PURGE`; registry loss is not immediate. Rejected Resume and
serving errors use terminal cleanup, while host shutdown has separate detach
behavior. The lifecycle guide now separates a session state diagram from the
service/registry cleanup flow and links the client-local public callback to the
Process Endpoint recovery timeline. Both revised diagrams rendered with Mermaid
CLI 12.0.0 and passed screenshot review. Existing private recovery and purge
cases test their paths separately; a public coordinator-stop case observes
local callbacks after shutdown but does not test ordering against the purge
deadline. No C++ source, builds, tests, CI, 2010, or Requirements Lab content
changed.

Increment 348 audited forced ConnectionLost observer effects across the Lifecycle,
Federation MOM, and Joined-Federate MOM guides. The Lifecycle overview now separates
the Federation-scoped `HLAfederatesInFederation` reflection, survivor
`HLAfederate` object removal, the `HLAreportFederateLost` interaction, and the
failed member's local `connectionLost` callback. It distinguishes their audiences
and preserves profile-specific source ordering without claiming a public callback
order. The process sequence now names the two survivor paths, and the companion
Federation MOM and Joined-Federate MOM sections link back to the overview.

The Federation MOM audit corrected a stale statement: the current worktree
process TCK source contains an assertion for a post-loss membership reflection,
but no runtime result was reviewed. The inspected process loss dispatcher has no
explicit enqueue for that conditional update; the 1516.2 MIM's Join/Resign
condition does not resolve the forced-loss normative question. The embedded loss
reflection and forced-loss `HLAfederate` object removal remain without focused
runtime assertions. Both revised diagrams rendered with Mermaid CLI 12.0.0 and
passed screenshot review. The ledger still matches all 506 diagram identities;
the worktree checker resolves 4,568 references across 35 Markdown files. No C++
source, builds, tests, CI, 2010, or Requirements Lab content changed. Overall
progress remains 80%; formal gates remain 3/4 (75%). The candidate remains
unpublished.

Increment 349 traced the Federation MOM membership-list projection through the
2025 process loss entry, shared registry result, and adapter queues. Both
profiles call the shared forced resignation transition, but the process
`FederationRegistryResult` has no Federation-object membership-update record
and its loss dispatcher does not enqueue one; the embedded adapter explicitly
queues the update. The MIM condition remains “Federate joins or resigns,” so
this source difference does not by itself resolve whether forced
`ConnectionLost` is normatively covered. The current process TCK source waits
for that reflection and then makes a separate member-list query; no saved CTest
result was present in the worktree's `out` directory. No C++ source or test
files were edited, and no builds or tests were run in this increment. No 2010
or Requirements Lab content changed in this increment. The diagram ledger still
matches all 506 identities; the worktree checker resolves 4,575 unique
references across 35 Markdown files. Diagram source lines are refreshed with
this increment. Overall progress remains 80%; formal gates remain 3/4 (75%).
The candidate remains unpublished.

Historical cross-revision inventory (increment 328 and earlier): The docs-only
candidate has 41 selected paths: 32 topic guides and nine
documentation and handoff assets. After increment 328, the source-line check
resolves all 4,431 unique relative path/line/guide references (5,744 link
occurrences) across 35 Markdown files in the worktree, at the pinned hosted base
c9c5a9e410539b039cd8bba35b53d0ef2b24d180, and at local source revision
e6321e842f333427349166df8c5b35350a2ae473. The private resume-test evidence
remains plain text explicitly labeled as current-worktree-only. A separate
local Git-object check resolves all 153 immutable GitHub source permalinks
(106 unique revision/path/line pairs across 41 target files), with zero missing
lines. The increment-328 semantic review checked callback-dispatch and bridge
anchors against the pinned candidate source; cross-revision checks establish
line existence, not semantic equivalence or support for every claim. The wider
source and standards citation review remains open. A refreshed source inventory
compares those guides with the 56 source
and test paths changed from the pinned base to the current source revision. The
increment-288 inventory found 854 links into 35 changed targets across 33
Markdown documents: 816 relative links and 38 immutable GitHub permalinks. It
supersedes the earlier 971-link/50-target screen, which used a different
candidate worktree change set. The Time Management guide adds nine distinct
relative source-line references; the roadmap also links to the same focused
bounds test. Refresh the occurrence-level
cross-revision inventory during the remaining semantic audit. The Process
Endpoint guide alone had 110 links into 18 changed targets at that inventory;
increments 320–324 reviewed private request correlation, callback lock
semantics, process polling-family priority, and the discovery/removal merge;
semantic citation review remains open. The last authenticated
GitHub Markdown API GFM pass in increment 319 returned all 35 scoped pages (32 guides,
the roadmap, thematic reading paths, and progress-history archive) with
498/498 Mermaid containers, 979 headings, and 7,663 links, with no failures or
fence/container mismatches. That is the last authenticated pass and remains a
historical 498-diagram result; increment 327's 500-diagram candidate is
unpublished and unchecked on GitHub. Both backtick and tilde Mermaid fences are
included in the source count. GFM parsing and container registration do not
prove visible hydration on published file pages.
The large behavior index was excluded because it exceeds the endpoint's 400 KB
input limit.

Increment 311
Increment 311 corrected the Names and Handle Resolution guide's implementation
description of typed-handle bytes. The current 2025 encoder writes twelve bytes:
a big-endian count of eight followed by the eight-byte big-endian identity; the
bytes contain no handle-category discriminator, and the reviewed decoder checks
shape and count rather than category. Focused tests establish the byte vector
and matching-type round trip, not cross-kind rejection. An untracked, uncalled
TCK helper's cross-kind expectation remains unverified. The 2010 adapter reuses
the private byte helper, while the public types and RTI behavior remain separate.

Increment 312 audited configured-process recovery and terminal ConnectionLost.
The internal resume loop may restore the same live host session before the
public ambassador loses membership; terminal loss instead marks the local
ambassador Not Connected while retaining its client and shared callback path.
An external-thread Connect can replace and close that old client under the
ambassador mutex. If an old callback is active and re-enters a service that
needs the mutex, the source permits a wait cycle. The new callback sequence
labels this as a source-derived, untested risk; the public process loss test and
private socket-resume test cover separate schedules. Added Callback and Service
Ordering diagram 021 for this possible wait cycle. Diagram 019 was also
rendered and screenshot-reviewed again after its body changed; the full ledger
now accounts for 497 current diagrams. The worktree source-line check resolves
4,319/4,319 references; authenticated GFM passes 35/35 pages with 497/497
containers, 969 headings, and 7,589 links.

Increment 313 reconciled configured-process terminal member loss across the
Process Endpoint, Lifecycle, Federation MOM, Joined-Federate MOM, and
Save/Restore guides. The process `dispatchConnectionLossResult` currently
consumes neither registry Save nor Restore notification vector for explicit
Resign or forced ConnectionLost; this is a source-level finding, not a tested
callback outcome or normative conclusion. Corrected the stale guide claims.
The Lifecycle sequence now shows the failed member's local callback and host
cleanup as parallel paths, with survivor object removals and the optional
`HLAreportFederateLost` report as separate effects. The MOM guides retain their
recipient-selection and `HLAfederate` observer-route details. Rendered and
screenshot-reviewed Lifecycle diagrams 025 and 030, plus the changed Callback
and Service Ordering diagram 019; refreshed the 497-entry ledger lines and
hashes. The current worktree source-line check resolves 4,312/4,312 links;
4,309 resolve at the candidate base and 4,310 at the primary source revision.
The last authenticated GFM check predates this increment.

Increment 314 resolved the three Callback guide source-line citations that were
unavailable at the pinned documentation base or primary source revision. The
background-dispatch and client-close claims now point to equivalent published
source lines, including the reader and callback-dispatch joins. The private
socket-resume test is described in plain text as current-worktree-only and is
narrowed to its private-client assertions; it is not presented as evidence for
the public ambassador's Not Connected transition. The source-line checker now
resolves all 4,309 unique relative references against the worktree, candidate
base, and primary source revision. The separate immutable-link audit resolves
148 occurrences (102 unique path/line pairs across 40 targets). The prose edits
shifted six later Callback diagram fences; refreshed those ledger line fields
without changing their bodies or screenshots. GitHub's GFM endpoint returned
HTTP 200/nonempty HTML for all six pages changed in increments 313–314, with
113/113 Mermaid containers. The other 29 scoped pages remain covered by the
full post-increment-312 GFM pass and have no later content edits. No C++
builds/tests, CI, 2010 changes, or Requirements Lab edits were made.

Increment 315 reviewed the Save/Restore failure boundary against the pinned
2025 API, current registry selectors, and focused embedded and configured-
process tests. The Restore guide now distinguishes admission rejection from a
failure after Restore is accepted, and records all five
`RestoreFailureReason` values with source selectors and bounded test evidence.
It identifies `RTI_DETECTED_FAILURE_DURING_RESTORE` as declared but unselected
in current source, and notes that no focused failure assertion was found for
`RTI_UNABLE_TO_RESTORE`; the same enum value also appears in a successful
internal completion notification, so the success flag matters. The Save guide's
reason map remains explicitly implementation-derived. Exact normative
Save/Restore reason triggers remain unresolved without licensed 2025 clause
text. The guide also distinguishes pre-admission decode, identity, and
membership rejection from post-admission image-application failure.

The prose change moved 18 Save/Restore diagram fences; all 35 ledger entries
for that guide were updated by ordinal, with every body hash unchanged. The
source-line checker resolves 4,331/4,331 unique references (5,613 Markdown
link occurrences) at the worktree, pinned candidate base, and primary source
revision. The authenticated GFM pass returned 35/35 pages, 497/497 diagrams,
974 headings, and 7,608 links with no failures. Published-page hydration
remains unverified. No C++ tests/builds, CI, 2010 changes, or Requirements Lab
edits were made.

Increment 316 separated configured-process event transport (push versus poll)
from callback invocation (`HLA_IMMEDIATE` versus `HLA_EVOKED`) in the Save/Restore
guide. Its profile matrix records embedded success coverage under both callback
models, process polling coverage under `HLA_EVOKED`, and process push coverage
under both models at different evidence depths. No focused public accepted-
request-to-terminal Save/Restore case combines process polling with
`HLA_IMMEDIATE`; the guide calls this a coverage gap, not an unsupported
profile. The new diagram was rendered and screenshot-reviewed; all 498 ledger
identities, headings, types, fences, source lines, and hashes match. The source-
line check resolves 4,332 unique references (5,619 link occurrences) across
the worktree, candidate base, and primary source revision. The authenticated
GFM pass covers all 35 pages and all 498 diagrams; visible published-page
hydration remains unverified. No C++ tests/builds, CI, 2010 changes, or
Requirements Lab edits were made.

Increment 317 audited Restore callback recipients across the requester and
member lanes. Current source routes request failure/success to the requester,
begun/initiate to each active member, and terminal results to operation members
who remain joined and have callback routes. A two-member configured-process
polling test with `HLA_EVOKED` checks requester success, begun/initiate on both
members, and successful terminal callbacks on both. This corrects the former
polling/`HLA_EVOKED` test-coverage gap; only polling with `HLA_IMMEDIATE` remains
uncovered by a focused public accepted-request-to-terminal case. The guide
labels recipient scope as current Umbra routing, not a normative claim. The
Restore sequence was rendered and screenshot-reviewed. The audit also found
and reviewed stale body hashes in Callback and Service Ordering diagram 019 and
TSO Retraction diagram 008. Reconciled 23 later Save/Restore line fields and
updated three diagram hashes; all 498 entries remain current. The worktree,
candidate-base, and primary-source line checks
resolve all 4,365 references. Authenticated GitHub GFM parsing was refreshed for
all 35 scoped pages; published-page hydration remains unverified. No C++ tests/
builds, CI, 2010 changes, or Requirements Lab edits were made.

Increment 318 completed the terminal Restore route review. `federateRestoreNotComplete`
and abort plan terminal notifications for operation members who remain joined
and have callback routes; focused configured-process polling tests under
`HLA_EVOKED` assert the failure reason after each operation, complementing the
existing `HLA_IMMEDIATE` cases. Each process failure/abort case has one member,
so multi-member process distribution is still untested. The member-exit route
excludes the departing member and plans survivor failures in the registry, but
the configured-process departure dispatcher does not enqueue that Restore
vector; this remains source-derived and lacks a process departure test. The
[Save/Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md) now records
these distinctions. Its existing member-exit diagram already shows the branch,
so no diagram body changed. The integrity check corrected a stale ledger hash
for the previously rendered and screenshot-reviewed TSO Retraction diagram
008; all 498 entries now match current bodies. Normative failure triggers
remain open pending licensed 2025 text. The source-line check resolves 4,370
unique references. No C++ tests/builds, CI, 2010 changes, or Requirements Lab
edits were made.

Increment 320 separated private process `requestId` correlation from the HLA
`MessageRetractionHandle` and `LogicalTime` in the Process Endpoint guide. The
guide now cites the private frame field and its zero/nonzero invariant, client
request allocation and response matching, and the client/server session checks.
The focused transport case covers distinct request IDs and operation matches
for Create, Join, and Send Interaction; it does not test HLA retraction-handle
or logical-time mapping. Added thirteen prose lines before the first diagram and
updated the 21 affected ledger locations; all 498 identities, fence lines,
types, and body hashes match, with no diagram body changed. The worktree,
candidate-base, and primary-source checks resolve 4,375 unique references
(5,668 occurrences), with zero unresolved. No C++ tests/builds, CI, 2010, or
Requirements Lab changes were made.

Increment 321 clarified the Process Endpoint callback lock boundary against the
pinned 2025 client. The guide now distinguishes `exchangeMutex_`, which
serializes request/response pairs, from `transactionMutex_`, which protects
request depth and pending-event state and is not held while waiting on the
reader. A matched response updates request depth and releases the exchange gate
before pushed-event draining; the dispatcher releases the state mutex before
bridge submission. The guide labels this as implementation behavior and notes
that the cited delivery tests do not assert callback thread identity, lock
ownership, or callback re-entry. Updated the callback flowchart and ledger: ten
Process Endpoint diagram entries moved, and one body hash was refreshed after
local Mermaid CLI 12.0.0 rendering and screenshot review. All 498 ledger entries
match. Worktree, candidate-base, and primary-source checks resolve 4,381 unique
references (5,674 relative link occurrences), with zero unresolved. No C++
tests/builds, CI, source changes, 2010 changes, or Requirements Lab edits.
Overall progress remains 80%; formal gates remain 3/4 (75%).

Increment 322 audited the Process Endpoint polling-family priority against
pinned 2025 sources and focused test cases. The guide now cites separate
per-session event-family queues and the session-to-state map, plus both the
poll-queue and pushed-frame interaction producers. It preserves the order of
synchronization announcement, federation synchronized, Save, Restore, and
interaction events as process implementation behavior. Test scope now accounts
for both the separate public polling/push synchronization cases and the private
Save/Restore case that retrieves a queued interaction before and after Restore;
that case establishes persistence and exactly-once delivery, not family
priority or starvation. Seven diagram source-line locations were refreshed;
no Mermaid body changed. All 498 ledger entries match. Worktree, candidate-base,
and primary-source checks resolve 4,391 unique references (5,684 relative link
occurrences), with zero unresolved. No C++ tests/builds, CI, source changes,
2010 changes, or Requirements Lab edits. Overall progress remains 80%; formal
gates remain 3/4 (75%). The authenticated GFM pass remains increment 319; local
edits are unpublished and unchecked on GitHub.

**Current handoff after increment 354:** The ConnectionLost observer-path
review confirmed that the Lifecycle, Federation MOM, and Joined-Federate MOM
guides already distinguish the Federation membership-list reflection,
survivor HLAfederate object removal, Federate-Lost interaction, local
callback, and public member-list query. The embedded adapter queues the
conditional membership-list update; the configured-process loss dispatcher
has no explicit enqueue. The current worktree process TCK source expects a
post-loss reflection, but no runtime result was reviewed. The focused Catch2
membership case covers Join and Resign, not forced loss; no focused case
asserts HLAfederate removal after forced loss. No normative equivalence was
inferred.

Two candidate-source citations were corrected: current-worktree process TCK
lines are now unlinked worktree-only evidence, and the process-client dispatch
pointer now resolves at the pinned candidate base. The current Callback guide
diagram 19 body differed from its ledger hash; Mermaid CLI 12.0.0 rendered it
and screenshot review found no clipping. Its current hash and review date were
recorded. Twenty-six Federation MOM diagram line locations were refreshed;
all 507 ledger bodies, types, fences, and source lines now match.

The 5,329-line progress log was split at existing date boundaries into an
index and three daily archives (increments 81–284, 285–333, and 334–354),
preserving the dated entries and keeping every file below the GitHub Markdown
API's 400 KB request limit. The authenticated GFM pass checked 38 pages: all
32 topic guides and six supporting pages returned nonempty HTML. The 507
source Mermaid fences produced 507 Mermaid containers, with 1,028 headings,
7,992 links, and no parser or diagram-count mismatches. The source-line checker
resolves all 4,591 references across the worktree, pinned candidate base, and
primary source revision, with zero unresolved. GFM parsing and container
registration do not verify visible browser hydration or published-page links.

Increment 355 published the 44-path candidate on `feature/hla-2025-docs-candidate-20261107` at
[adafb10c](https://github.com/sheepfling/umbra/commit/adafb10c37b6aad8f798e49ca7007abeaaeb66fc) and inspected all 32 guide page DOMs with headless
Chromium. Every page loaded with its expected title and branch, and all 507
Mermaid container counts matched the visual-review ledger. All containers
retained GitHub's `Loading` placeholder because the pass did not scroll them
into view.

Increment 356 scrolled every Mermaid container into the viewport to trigger
GitHub's lazy rendering. All 32 guide pages loaded; all 507 expected diagrams
produced SVGs, with 507/507 Mermaid render responses returning HTTP 200 and no
request failures. A representative Attribute Scope Advisory diagram was
visually inspected in the GitHub page viewport and was legible. This resolves
the increment-355 hydration uncertainty; it does not claim screenshot review
of every hosted diagram or click-through verification of every link.

Increment 357 refreshed the 2025 Save failure-reason audit. The official IEEE
record lists purchase and subscription access; its public download ZIP contains
one nested API XML archive with 284 files (44 C++ headers, 230 Java files, four
XML files, three protobuf files, and three XSDs), but no standard text. The
pinned enum and source scan confirm four SaveFailureReason values have current
selectors. `RTI_DETECTED_FAILURE_DURING_SAVE` and
`SAVE_TIME_CANNOT_BE_HONORED` appear only in the source MOM formatter and have
no direct test references. The guide records this as implementation evidence,
not a normative mapping. Its new 2025 access and selector note is in the
[Save and Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md). The
source-line checker resolves 4,591/4,591 references across 38 2025 Markdown
files against HEAD `7134145e`. No C++ source or tests changed; no builds or
tests were run. Overall progress remains 80%; formal gates remain 4/4 (100%).

Increment 358 audited a non-timing synchronization edge against committed
2025 source and focused tests. Late-join expansion on an omitted-set point
adds a new federate ID to its participant and announcement sets; achievement
results are keyed by ID, so a same-name rejoin receives no inherited result.
Resignation removes the departing ID from participant, announcement, and
result state, then reevaluates completion from the remaining participants and
builds the failed-to-synchronize set from their recorded false results. The
focused registry case compares explicit-set and omitted-set expansion; a
separate public service-report case covers an achieved survivor and an
unachieved departing member, whose departure closes the point with an empty
failed set. No focused test combines resignation with same-name rejoin or a
departing false result. The existing Synchronization guide already diagrams
these source-derived branches and states the test limits; no diagram change was
needed. A bounded inventory confirmed that all 32 current 2025 topic guides
map the principal public service/callback families, so no additional guide is
justified by API count alone. The prior Save timing handoff is already covered
by its admission diagram and focused TSO/FQR boundary cases; the two
unselected SaveFailureReason values remain explicit evidence gaps. No C++
source or tests changed; no builds or tests were run. No 2010 or Requirements
Lab content changed. Overall progress remains 80%; formal gates remain 4/4
(100%).

The authenticated increment-355 GFM pass confirms parsing and 507/507 Mermaid
container registration, with 1,029 headings and 7,994 links. The 4,591 source-line
references still resolve across the worktree, pinned candidate base, and primary
source revision. No C++ source or test files changed; no builds or tests were
run. The 2025 and 2010 streams remain separate, and no Requirements Lab content
changed. Overall progress remains 80%; formal gates are now 4/4 (100%). The 48
unrelated non-documentation changes remain outside the candidate.

**2025 RAVU review, increment 359:** The guide already explains provider
solicitation, a later Update, receiver eligibility, class-request ownership
checks, and the limits of focused tests. The current worktree's regional
preflight resolves dimension context per requested attribute; the committed
candidate remains class-wide, and the hosted source-dependency audit marks the
current-only citations. A portable inherited-attribute regional-request case
records `InvalidRegionContext` under both callback models (4/4), but does not
test multi-provider fan-out or ownership changes across request phases. Direct
§6.21 requester entitlement remains open without the normative clause text.
No guide edit or diagram was warranted.

**2025 timed Save pending-state review, increment 360:** The registry keeps an
accepted timestamped Save in one replaceable pending slot until a qualifying
constrained-member advance checkpoint opens the SaveOperation. The admission
predicate checks the member's advance mode and undelivered TSO boundary, and
the dispatcher invokes `initiateFederateSave` before that member's grant. The
embedded and configured-process replacement cases both request time 5 and then
7 before initiation, then observe only the replacement label/time at the later
boundary. Neither test leaves the requested time unreachable or asserts a
terminal failure notification. The source search found no
`SAVE_TIME_CANNOT_BE_HONORED` selector; its current occurrence is the MOM
formatter, and the normative trigger remains unresolved without the clause
text. The Save guide's flowchart now shows pending retention, replacement, and
the absence of a failure selection at an ineligible checkpoint. Mermaid CLI
12.0.0 rendered the changed chart and its screenshot was reviewed. The visual
ledger now matches all 42 Save/Restore guide diagram source lines and body
hashes; the 507-diagram inventory remains unchanged. The worktree source-line
checker resolves 4,600/4,600 references across 38 2025 Markdown files. No C++
source or tests changed; no builds or tests were run. No 2010 or Requirements
Lab content changed. Overall progress remains 80%; all formal gates remain
complete (4/4).

**Next handoff:** Audit the non-timing 2025 attribute-ownership edge for a
multi-attribute partial If-Wanted transfer followed by cancellation. Compare
the registry's retained and removed attribute subsets and planned callbacks
with the focused one-attribute transfer case. Update the existing ownership
flow only if it obscures that state boundary; label source-derived behavior and
the missing multi-attribute test without inferring a normative callback order.
Keep 2010 and Requirements Lab paths excluded.


The older hosted-page audit records 31 available pages and one missing guide;
339 of the 340 hosted diagrams rendered, while one timing-guide diagram failed
the GitHub Mermaid parser. Those results belong to the old `feature/initial-work`
snapshot. For the current published branch, increment 356 confirms that all 32
pages and all 507 diagrams hydrate after viewport scrolling. The candidate
contains no Requirements Lab or 2010 changes.

The four-gate score is **4/4 (100%)**: survey and backlog, guide content,
local/GFM/source-link verification, and visible published-page rendering are
complete. Overall progress remains **80%** because the broader semantic and
source-citation review is still underway, especially the licensed 2025
Save-failure mapping. The configured-process Join/MOM, process Evoke Multiple,
and single-Evoke reviews are complete. The goal remains active while the
remaining 2025 semantic gaps are reviewed and either resolved from authoritative
text or explicitly bounded.
Continue recording cross-guide gaps as concrete backlog items; keep diagram
count a quality check rather than a growth target. The 2025 and 2010
implementation streams remain separate.

Completed progress details for increments 1–80 are preserved in the
[topic-audit history archive](HLA-BEHAVIOR-FLOW-GUIDES-ROADMAP-HISTORY-2026-11-07.md).
The current diagram identities and review evidence are recorded in the
[2025 diagram visual-review ledger](HLA-2025-DIAGRAM-VISUAL-REVIEW-LEDGER.json).
Keep this file focused on the current baseline, actionable backlog, coverage
map, and recent handoffs; archive older dated detail before the 6,000-line
documentation hard limit is reached.

## Open documentation backlog, prioritized

These items are the remaining work. All four formal gates are complete (4/4,
100%); the broader overall estimate remains 80% because semantic review is
still underway.

1. **Publish the docs-only candidate and inspect its pages — complete:** The
   44-path candidate (32 guides and twelve supporting assets) is published on
   `feature/hla-2025-docs-candidate-20261107` at [adafb10c](https://github.com/sheepfling/umbra/commit/adafb10c37b6aad8f798e49ca7007abeaaeb66fc). All 32 guide DOMs load with
   matching titles and branch refs; their 507 Mermaid container counts match
   the ledger. The pages expose 11,305 links, including 7,239 candidate-branch
   links and 6,080 source-line anchors, with no `./` or `../` href forms. The
   source-line checker resolves all 4,591 unique references across the worktree,
   pinned candidate base, and primary revision. Link markup was inspected, but
   click-through status for every link was not. The 48 non-documentation changes
   remain excluded, and no 2010 or Requirements Lab paths were selected.
   **Done when:** the candidate is published and the 32 page outcomes are
   recorded in the render audit. Complete in increment 355.

2. **Visible Mermaid rendering gate — complete:** Increment 356 scrolled all
507 Mermaid containers on the 32 published guide pages into view. Every
container produced an SVG, each Mermaid render request returned HTTP 200, and
no requests failed. A representative hosted diagram was visually inspected.

   **Done when:** the 507 diagrams render visibly on the published guide pages
   and the audit records the result. Complete in increment 356.

3. **Source-coupled review:** Recheck evidence limits at three
   high-interaction seams: [partial ownership transfer/cancellation](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md),
   [configured-process mixed callback families](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md),
   and [Join commit versus MOM discovery](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md).
   The DDM region-realization seam was reviewed in increments 139–140: the
   guide distinguishes passing inherited, excluded, default-region, and
   partial-region object and interaction TCK paths from the failing
   `Light`/`Flavor` Catch2 case.
   Existing guides distinguish source-derived behavior from focused-test
   observations and identify untested combinations. Worktree reviews of all
   three seams are complete (increments 124–126); the Join/MOM guide now adds
   the configured-process post-discovery rollback branch and its focused test.
   The DDM/time and subscription/process cross-guide seams were reviewed in
   increment 141; both already separate eligibility from callback delivery and
   link bounded process evidence with its limits.
   Local cross-guide reviews are complete for the ownership cancellation,
   mixed callback-family, Join/MOM, DDM/time, and subscription/process seams.
   The candidate source review of the Joined-Federate MOM guide's explicit
   Resign versus forced ConnectionLost path is also complete: normal Resign
   preflights observer routes, while forced loss may skip an unavailable route.
   Cleanup erases a snapshot with no pending removals, but callback entry for
   other planned removals can leave a skipped observer known without a queued
   removal. Existing public tests assert successful explicit Resign; the
   bounded connection-loss test review found no forced-loss HLAfederate object
   removal assertion. The guide labels this behavior implementation-derived
   and untested. Candidate review of partial If-Wanted transfer versus
   cancellation also confirms that the transferred attribute leaves the live
   cancellation reservation while the callback gate resolves any remaining
   attributes. The focused transfer race covers one attribute; the multi-
   attribute split remains source-derived, with no callback order asserted.
    **Remaining:** The active Restore and Save participant-exit seams were audited
    in increments 296–297, and increment 315 added implementation selector maps
    for both Save and Restore reasons. The exact 2025 normative failure-reason mapping remains
    unresolved: the [official IEEE record](https://standards.ieee.org/ieee/1516.1/6688/)
    directs readers to purchase or subscription access. The public Downloads ZIP
    was inspected and contains the API source bundle (C++ and Java API files,
    protobuf definitions, XML schemas, and example FOMs), but no copy of the
    standard. The [pinned Umbra package](../../third_party/ieee1516.1-2025/README.md)
    likewise contains API headers, not the full normative text. The available
    API enum identifies failure reasons but does not map each reason to the
    normative Save or Restore failure conditions. Keep that mapping open until
    the licensed clause can be checked; do not substitute 2010 wording. Increment 299 completed
    the source audit of the embedded
    `Send Interaction` callback-exception path; the chart distinguishes its
    implementation behavior from normative requirements and untested outcomes.
    Increment 300 reviewed configured-process polling priority: each poll restarts
    the first-match selector, so a persistently nonempty earlier queue can delay
    later families. No HLA-wide order or fairness rule is inferred, and the cited
    public synchronization cases do not exercise mixed-family scheduling.
    Increment 301 corrected the ownership/time cross-guide claim: a focused
    embedded test observes ownership assumption and acquisition notification
    before either federate requests time advance, then observes both grants and
    the retained TSO reflection. Its callback-order vector is not asserted, so
    this documents one staged test sequence, not general grant precedence or
    configured-process behavior.
    Increment 302 added the configured-process boundary: ownership assumption
    and acquisition work uses its ownership event queue or `receive_interaction`
    frames, while the grant factory sends a distinct `time_advance_grant` event.
    Focused process tests cover ownership and TSO/grant separately; combined
    ownership/time callback cases found in the Catch2 tree are embedded. No
    configured-process callback order is inferred.
    Increment 303 audited the Process Endpoint client queue against committed
    candidate sources. Pushed frames are buffered during a correlated request;
    after response matching, request depth is released and the exchange lock
    is unlocked before buffered events are drained. The background dispatcher
    waits for request depth and callback-dispatch suppression to clear. The
    public Evoke pump polls with `receive_interaction`; interaction results
    return separately, while other typed results may enter the client FIFO.
    The Process Endpoint guide already matches this behavior. A separate dirty
    source snapshot adds a callback-admission counter and getter without a
    declaration or reader, so it does not establish a receive-poll fence and
    remains excluded from candidate behavior claims.
    Increment 304 corrected the Callback and Service Ordering Process Evoke
    Multiple flow: committed source performs one shared-dispatcher call after
    its conditional process pre-pump; only single Evoke has the post-wait
    FIFO drain and zero-wait follow-up. The source queue-count check does not
    fence the client-FIFO-to-bridge handoff, and the cited process test does
    not exercise that race or enabled multi-poll draining. Diagram 007 was
    updated, rendered, and screenshot-reviewed. Diagram 018 and TSO Retraction
    diagram 008 were also re-rendered to reconcile their current-body hashes.
    The 495-entry ledger now matches current source lines and body hashes.
    Increment 305 audited single Evoke's process push timing. The adapter's
    post-wait branch runs only when the initial `evokeOne` returns false and
    invokes no dispatcher task; it then attempts a client FIFO drain and calls
    one zero-duration `evokeOne` if dispatcher work is pending. A dispatcher
    task can be invoked while the receive-order temporal gate defers entry to
    `FederateAmbassador`; the `callbackInvoked` output tracks dispatcher-task
    invocation. A concurrent client dispatch lease can make the post-wait
    drain return early. The HLA_EVOKED async-delivery test uses zero-duration
    calls, the public receive/Evoke case sends before Evoke, and the generic
    dispatcher test queues work before Evoke Multiple; none covers an ordinary
    pushed interaction arriving during a positive single-Evoke wait or the
    FIFO-to-bridge handoff race. Corrected diagram 005, rendered it with Mermaid
    CLI 12.0.0, and screenshot-reviewed it. Refreshed all 24 Callback guide
    ledger source lines, headings, and hashes. Corrected two inherited bridge
    source-line links and added the dispatcher-flag source citation; all 14 links
    in this section resolve. The ledger audit found and corrected 105 stale
    heading breadcrumbs across the 2025-only ledger; the full 495-entry
    identity, heading, type, source-line, and body-hash check passes. The latest
    worktree source-line check resolves 4,292/4,292 references. No C++
    tests/builds, 2010 changes, or Requirements Lab edits.
    Increment 306 audited the configured-process AttributeUpdate receipt and
    callback gates. The background client drain waits for request and suppression
    depths; a callback-owning request can drain through its own suppression gate,
    while a competing dispatch owner can leave an item queued. The AttributeUpdate
    bridge path is separate from the receive-order interaction temporal gate.
    Corrected the section's citations, rendered and screenshot-reviewed diagram
    006, and verified all 495 diagram ledger entries. Thirty local line citations
    resolve against committed HEAD and the candidate base; the full worktree check
    resolves 4,292/4,292 references. The delayed-AttributeUpdate tests assert
    eventual outcomes but do not inspect private queues or test deferred markers
    under HLA_EVOKED. No C++ tests/builds, 2010, or Requirements Lab changes.
    Increment 307 audited “The bounded drain uses elapsed time, not HLA logical
    time” against the committed 2025 API declaration, adapter, dispatcher, and
    focused tests. The adapter maps non-finite and nonpositive seconds to zero,
    truncates positive fractional milliseconds, and saturates very large finite
    values. The dispatcher normalizes maximum to at least minimum and starts its
    steady-clock window only after taking the shared dispatch lock and passing
    entry gates. An empty queue waits only to the minimum deadline; queued tasks
    run FIFO, with the maximum checked after each callback returns. Lock wait
    and callback execution can extend the API-call duration. Corrected the test
    note: the focused `evokeMultiple` test starts with three queued tasks and
    asserts FIFO completion, not elapsed time; the cited disabled test exercises
    `evokeOne`, not `evokeMultiple`. No focused test covers timed arrival,
    argument conversion, normalization, maximum overshoot, lock delay, or the
    public adapter. Revised and screenshot-reviewed diagram 008, re-rendered and
    reviewed diagram 018 after finding its stale hash, and refreshed Callback
    guide ledger lines for diagrams 008–024. All 495 ledger entries match their
    identities, headings, types, fences, lines, and body hashes. The worktree
    source-line check resolves 4,293/4,293 references across 35 Markdown files.
    IEEE's product page requires purchase or subscription access; normative
    interpretation of §10.58 remains unresolved pending licensed text. No C++
    tests/builds, 2010 changes, or Requirements Lab edits. Overall progress
    remains 80%; formal gates remain 3/4 (75%).
    Increment 308 audited “HLA_IMMEDIATE: callback work can be on the service
    stack” against the pinned 2025 callback-model enum, dispatcher/session,
    `listFederationExecutions` adapter, and focused tests. Corrected the diagram
    to distinguish an enabled submitter that becomes or re-enters the immediate
    drainer from a competing producer that appends to FIFO and returns before
    its callback enters. The current drainer invokes that task after its active
    callback returns normally and delivery remains enabled. Callback exceptions
    consume the active task, preserve later FIFO work, and unwind through the
    call that owns the drain; this may differ from the service that submitted
    the task. The generic tests cover same-thread synchronous delivery and a
    competing dispatcher producer; the public `listFederationExecutions` test
    covers one uncontended callback and rejection of both Evoke services from
    that callback. No public test covers a competing drainer through an RTI
    service, a permitted nested service that produces a second callback, or
    exception attribution in that schedule. Diagram 009 was rendered and
    screenshot-reviewed; Callback guide ledger lines for diagrams 009–024 were
    refreshed, and all 495 entries match identities, headings, types, fences,
    lines, and body hashes. The worktree source-line check resolves 4,298/4,298
    references across 35 Markdown files; the changed citations also resolve
    against the pinned candidate base and source revision. The licensed text is
    still needed to state normative callback-thread and exception semantics.
    No C++ tests/builds, 2010 changes, or Requirements Lab edits. Overall
    progress remains 80%; formal gates remain 3/4 (75%).
    Increment 309 audited CallbackSession lifetime against committed 2025 source
    and focused tests. The revised state diagram distinguishes accepted entries,
    private self-close, external close, queued invocations, and final pointer
    detachment; a new sequence separates public Disconnect rejection from
    external teardown. Found public embedded evidence that Disconnect discards
    a pending evoked federation-list report, correcting the guide's former test
    gap. Unit tests separately cover waiting external close, direct-call
    serialization, and private self-close. No focused test covers public
    Disconnect from a callback, public Disconnect waiting on another thread's
    callback, or configured-process pending work during concurrent reconnect.
    Both changed diagrams were rendered and screenshot-reviewed. Progress stays
    80%; formal gates stay 3/4 (75%).
    The next semantic handoff audits configured-process ConnectionLost and
    reconnect after the in-progress process-client recovery edits settle. Check
    the Not Connected transition, retained old client, replacement cleanup, and
    possible shared-dispatcher reset against final source and focused tests;
    keep source-derived races distinct from tested outcomes. Continue 2025-only
    and docs-only. Publication and inspection of all 32 GitHub pages, including
    visible Mermaid hydration, remains the final formal gate; publish after
    explicit authorization.

Increment 310 audited partial recipient fan-out when an embedded report callback
throws. Both `HLAreportMOMexception` and `HLAreportException` hooks are no-throw.
Under `HLA_IMMEDIATE`, callback entry may occur during per-recipient submission;
a callback exception is contained by the hook but stops that report loop before
later recipients are submitted. Under `HLA_EVOKED`, callbacks remain queued until
Evoke. No focused test covers a throwing observer in either report fan-out.
Updated Exception Reporting diagram 007 and rendered and screenshot-reviewed it.
A full body-hash audit also found stale ledger records for Synchronization
diagram 017 and FOM Composition diagram 003; both were rendered and
screenshot-reviewed, and their ledger hashes were corrected. The ledger has 496
matching diagrams; 4,312/4,312 current worktree source-line references resolve,
with one older-revision gap in an unrelated uncommitted process-client citation.
Authenticated GFM returned 35/35 pages, 496/496 Mermaid containers, 966
headings, 7,572 links, and no failures. No C++ tests/builds, 2010, or
Requirements Lab changes. Overall progress remains 80%; formal gates remain 3/4
(75%).

The third item is a focused follow-up, not an additional completion gate; its
known evidence limits are already documented and do not invalidate the 32/32
content audit. Do not count publication or review of the same evidence twice.

The 409 bodies in the whole-topic render were checked with Mermaid
CLI 12.2.0. Eleven later additions and six revised charts were separately
rendered with Mermaid CLI 12.0.0. The additions were the two newest sequences,
the declaration Restore flowchart, the restored-order timing flowchart, the
synchronization barrier state diagram, the exception-report callback sequence,
the configured-process service-report recipient flowchart, the service-report
session-timing sequence, the final-Resign/Destroy snapshot sequence, the
federate/object-identity rebind flowchart, and the process Evoke wait-window
receive sequence. The six revisions were the two profile-comparison
flowcharts, the active-Restore member-exit flowchart, the federation/member
FOM-module index comparison, and the restored-barrier late-Join departure
flowchart, and the partial-report/late-Join achievement state diagram. Two
subsequent focused flowcharts bring the
rendered source inventory to 422: the `HLAreportServiceFile` MIM-versus-source
flow in the Federation MOM guide and the configured-process federation
FOM-module request flow. Both were rendered with Mermaid CLI 12.0.0 and
screenshot-reviewed; the report-file flow was rendered again during inventory
reconciliation. The revised federation/member index comparison was also
re-rendered. The latest synchronization Save/Restore review added one focused
public HLA_EVOKED sequence and re-rendered the source-derived partial-progress
sequence with Mermaid CLI 12.0.0; both were screenshot-reviewed. A new
source-derived sequence connects saved partial results through stable-ID
Restore, expandable late Join, and final failed-set construction. Mermaid CLI
12.0.0 rendered it and local screenshot review found it readable and unclipped.
At the 448-chart checkpoint, all 448 bodies had local render coverage. The
later 463-chart checkpoint had recorded local render coverage for every body;
the 2026-11-07 corpus render verified the then-current 477-body inventory; later current-body edits are tracked in the visual ledger. The
new embedded MOM subscription-command chart
was rendered with Mermaid CLI 12.0.0 and screenshot-reviewed. The configured-process pending-grant Restore
flowchart was rendered with Mermaid CLI 12.0.0 and locally screenshot-reviewed.
The configured-process TSO-bounds sequence was rendered and screenshot-reviewed
in the latest increment. The Disable Time Constrained/pending-TAR flowchart was
rendered with Mermaid CLI 12.0.0 and screenshot-reviewed; it distinguishes the
two separately tested embedded behaviors from the untested combined callback
order. The configured-process Resign projection/response flowchart was rendered
and locally screenshot-reviewed with Mermaid CLI 12.0.0; it exposes both
post-commit error exits and the success-only session cleanup. The configured-
process Destroy outcome sequence was also rendered and screenshot-reviewed;
it distinguishes typed registry outcomes from transport errors and shows the
success-only update-rate cleanup. A focused process member-list/Destroy
interleaving flowchart was rendered and reviewed for the captured empty roster,
callback-model branches, and later missing-execution result. The selected
screenshot review at that checkpoint stood at 193, with none pending. GitHub rendering remains
unverified. The Restore-to-departure cross-review revised
the restored-barrier chart to connect stable-ID Restore, late-Join expansion, and
member departure; Mermaid CLI 12.0.0 rendering and screenshot review found it
legible and unclipped. A 17-anchor focused source/test link check passed. The
combined sequence remains source-derived because no focused test covers the
Restore/late-Join/departure interleaving. A 2025 Restore-to-Send review linked
the restored pending time-role state to the existing TSO-recipient decision
flow; it found no focused
test combining a restored pending role with a new timestamped Send, and no
additional diagram was needed.
A subsequent additional-FOM Join review confirmed that the existing guides
already separate catalog replacement, the joining member's MOM object,
`HLAcurrentFDD` reporting, and ordinary application-object discovery. A
synchronization participant-departure review clarified the configured-process
test boundary; the existing source-derived branch remains, with no diagram
change. Two new test anchors were checked. Inventory remains 423 diagrams / 31
guides, and the selected screenshot review remains 161/0.
The subsequent Restore-to-departure review added a source-derived flowchart
connecting stable-ID restoration, the narrower changed-ID gate, and shared
Resign/ConnectionLost cleanup. Local screenshot review caught reversed Yes/No
branches at the empty-participant decision; those labels were corrected before
the final render. Twelve local destinations (nine source/test line anchors and
three guide headings) resolve. The cited public case restores changed IDs
without prior results and resigns only after completion; no focused test covers
departure from a pending restored point. At that checkpoint inventory was 424 diagrams across 31 guides (242
flowcharts, 157 sequences, 25 state diagrams; 416 backtick-fenced and 8
tilde-fenced); selected screenshot review is 162/0.

A subsequent Restore-to-late-Join review connected the restored expansion flag
and saved result map to the existing late-Join expansion and fan-in charts.
Both the embedded adapter and configured-process service reach the shared
registry hook. The combined path remains source-derived: one public test
restores an omitted-set, unachieved point and then achieves it, but does not
Join afterward; late-Join tests do not Restore. The focused section and
reciprocal Save/Restore row have 19 checked destinations (15 line anchors and
four guide headings). No Mermaid body
changed; at that review's checkpoint inventory was 424 diagrams / 31 guides
and selected screenshot review was 162/0.
That 409-body render included all eight tilde-fenced bodies then present, but
does not validate the original Markdown wrappers. A separate CLI pass over the original
tilde-fenced Markdown produced no artifacts because the local extractor
skipped those wrappers; it does not show that GitHub rejects them. The
[GitHub Flavored Markdown
specification](https://github.github.com/gfm/#fenced-code-blocks) accepts both
tilde and backtick fences, and [GitHub's Mermaid guidance](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/creating-diagrams)
places Mermaid syntax in a fenced block with the `mermaid` identifier. The
eight tilde-fenced bodies contain no backticks, so their wrappers are valid
under GFM and need not be changed to accommodate this local extractor. This
supports the expectation that GitHub will handle the wrappers; actual GitHub
page rendering remains unverified. Earlier 398- and 399-chart renders were
checkpoints, not the current whole-topic inventory render.
The six sequence diagrams added after the previously recorded 405-chart
checkpoint are the fresh-registry queued regional TSO interaction, restored
regional AttributeUpdate after producer Resign, conveyed Region callback
copy lifetime, receive-order Region callback-entry, accepted timestamped
deletion after producer Resign, and forced ConnectionLost cutoff for a queued
object deletion, as documented in the progress log. Two flowcharts were added
after that checkpoint: Federate-Lost subscription change and authorization
profile boundary.
Mermaid 12.2.0 matches the version string found in GitHub's public Viewscreen
renderer bundle on 2026-11-04. Local renders check diagram syntax with that
engine; they do not verify GitHub's page rendering or layout. The curated local
screenshot review has 161 charts, not a whole-inventory visual audit.
The full inline-link audit across all 31 topic guides checked 5,470 local
Markdown link occurrences, including 5,374 file targets and 5,015 line,
heading, or HTML anchors (4,460 line anchors). It found one stale GitHub
heading fragment, which was corrected. Focused follow-up checks validated the
seven links in the reserved-name reader route, two synchronization test-stream
links, and two DDM test-stream links. At that point, combined checked coverage
was 5,511 occurrences, 5,415 file targets, and 5,056 anchors (4,494 line
anchors); no unresolved local destination remained across those checks. The
Save/Restore identity subsection added 15 separately checked destinations
(12 line anchors and three guide-heading anchors), all valid and not folded
into that aggregate. The 12 links in
the ownership continuation review and the 18 links in the late-eligibility
review were checked separately. The four destinations in the timestamped
deletion/Resign review and nine in the forced-ConnectionLost follow-up were
also checked separately. Fenced and inline code examples were excluded.
The route link in the behavior index was checked separately; external URLs
were not fetched.

Mermaid CLI 12.0.0 rendered the two
member-report Resign charts, and local screenshot review found them readable.
The new failed-Restore state flow
and the new discovered-class stability sequence were also rendered and
screenshot-reviewed. The object-existence/receiver-knowledge chart was revised
and separately rendered in the current review. The new execution-catalog
snapshot flow was rendered and screenshot-reviewed. The revised ConnectionLost
lifecycle chart, two Federate-Lost report charts, and the configured-process
report/local-callback chart were also rendered and locally screenshot-reviewed.
The authorization callback-exception flowchart was rendered and screenshot-
reviewed in its increment. The Exception Reporting callback-failure sequence
was revised to show the independent configured-process dispatch loop and its
pending-exception follow-up, then rendered and screenshot-reviewed locally.
The configured-process service-report recipient flowchart was corrected and
paired with a new session-timing sequence; both were rendered and
screenshot-reviewed locally. A final-Resign/Destroy snapshot sequence was
rendered and screenshot-reviewed locally.

The selected visual-QA queue now has 161 locally rendered and screenshot-reviewed
charts, with none pending. This is a curated queue, not a whole-inventory visual
audit. Local screenshots do not establish GitHub's renderer or whole-tree visual
quality. The active-Restore member-exit flowchart was revised, rendered, and
screenshot-reviewed in the latest increment. The new process Evoke wait-window sequence was rendered with Mermaid
CLI 12.2.0 at a 1,800-pixel maximum size and screenshot-reviewed; its participant
lanes and nested callback/time-gate branches are legible and unclipped. The new
ownership fan-out/continuation comparison was inspected at its
local render size; both profile lanes and delivery branches are readable and
unclipped. The ownership late-eligibility chart was rendered at local size; its
profile branches and service transitions are readable. The newly added
late-subscription known-object comparison flow was
inspected at 1200-pixel render size; its branches are readable and unclipped.
The Cross-Cutting map of the independent boundaries around an
object-attribute update was rendered and screenshot-reviewed in this increment;
it is tall but readable and unclipped at the local renderer's 784-pixel width.
The Time Management grant-decision chart was also revised and rendered: it now
focuses on TAR, TARA, NMR, and NMRA, while the separate FQR path is linked to its
own detailed section. Its gate map is readable and unclipped; the adjacent
comparison matrix retains the mode-specific boundary rules. The regional
process TSO callback-gate sequence was rendered and screenshot-reviewed; it
shows the NMR path with callbacks enabled separately from the NMRA/TAR paths
that disable callbacks. The synchronization early-failure sequence was also
rendered and reviewed; its reversed report order is source-derived and the
matching focused test uses the opposite order. The pending-barrier state chart
was inspected with Mermaid CLI 12.2.0 at a 900-pixel canvas width; the
top-to-bottom layout keeps state and transition labels readable. GitHub page
layout and behavior remain unverified.

The latest 2025 object-name lifecycle cross-review compared the reservation,
Name/Handle, and federation lifecycle guides. It added a source-derived
flowchart separating retraction terminalization, the producer's deletion-time
grant, and the global purge gate. Mermaid CLI rendering and local screenshot
review confirmed the chart is legible. An exact two-case CTest run against the
existing `out/mgw/shared` build passed the Restore case and failed the terminal
name-reuse case at replacement registration. That build's `libumbra_rti.dll`
predates the current registry sources, so these outcomes remain provisional.
Refreshing the larger Catch2 target expanded to 40 compile steps; it was
stopped after 12 to avoid a broad build alongside the active C++ worker. The
current-source test result remains open. Inventory is now 427 diagrams, with
167 selected screenshot reviews.

A 2025 time-management reader-gap review added a focused flowchart for the
interaction among undefined GALT, the Non-Regulated Grant switch, and a queued
TSO message. It shows that NRG can permit a forward TARA while GALT is
undefined, while the queued timestamp still caps the actual grant; with NRG
disabled, the request waits for a later regulator transition. The embedded
test asserts both branches and interaction-before-grant ordering. A separate
query case checks undefined GALT/LITS after the last regulator disables
regulation when no TSO is queued. The new chart was rendered with Mermaid CLI
12.0.0 and screenshot-reviewed; the direct links from the role-state section
and the new source/test anchors resolve. This is an embedded implementation
trace, not a normative interpretation or configured-process parity claim. The
inventory is now 427 diagrams across 31 guides (244 flowcharts, 158
sequences, 25 state diagrams), and the selected screenshot queue is 167/0.

For a first read, begin with the [cross-cutting HLA model of state, events, and
observation](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md). It
introduces the portable questions that recur across services; use the API map
below to move from that lens to a topic-specific guide. The behavior-guide
index also provides [thematic reading paths](HLA-2025-THEMATIC-READING-PATHS.md)
for following a question across several service families.

## 2025 C++ API-to-guide coverage

This is a navigation map from the pinned 2025 C++
[RTIambassador](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h)
and [FederateAmbassador](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h)
interfaces to the HLA-first guides. It groups related operations and callbacks;
it is not a method-by-method conformance checklist.

| API and callback family | Primary guide coverage |
| --- | --- |
| Connect, Disconnect, callback model, and credentials | [Connection and Configuration](HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md); [Authorization](HLA-2025-AUTHORIZATION-FLOW-GUIDE.md); [Callback and Service Ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md) |
| Create, Join, Resign and automatic resign directives, Destroy, execution catalog reports, and member roster reports | [Federation and Federate Lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md); [FOM Module Admission and Composition](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md) |
| Synchronization-point registration, announcement, achievement, and completion | [Federation Synchronization](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md) |
| Save/Restore requests, participant work, status callbacks, and outcomes | [Federation Save and Restore](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md); [Process Endpoint](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md) |
| Object/interaction publication, subscription, discovery, reflection, update, and deletion | [Object and Interaction Information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md); [Object-Attribute Subscription State](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md); [Relevance Advisory](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md) |
| Explicit attribute-value update solicitation and Provide callbacks | [Attribute Value Update Request](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md); [Auto Provide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md); [Delay Subscription Evaluation](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md); [Data Distribution and Regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md) |
| Object-name reservation and reuse | [Object-Instance Name Reservation](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md); [Federation and Federate Lifecycle](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md) |
| Directed interactions and target selection | [Directed Interaction](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md); [Data Distribution and Regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md) |
| Region creation, modification, association, and regional send/subscription | [Data Distribution and Regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md); [Attribute Scope Advisory](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md) |
| Attribute ownership acquisition, divestiture, callbacks, and persistence | [Attribute Ownership](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md); [Ownership Persistence and Resignation](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md) |
| Time regulation/constrained roles, advance requests, grants, GALT/LITS, retraction, and timestamped-send order | [Time Management](HLA-2025-TIME-MANAGEMENT-GUIDE.md); [TSO Retraction](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md); [Order and Transportation](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md); [Callback and Service Ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md) |
| Order/transportation changes and queries; attribute update-rate admission | [Order and Transportation](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md); [Attribute Update-Rate Admission](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md) |
| Handle/name lookup, known-class lookup, decode/normalize operations, dimensions, and ranges | [Name and Handle Resolution](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md); [Data Distribution and Regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md); [Object and Interaction Information](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md) |
| Evoke, Evoke Multiple, callback enable/disable, and callback delivery | [Callback and Service Ordering](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md); [Process Endpoint](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md) |
| Advisory switches, Auto Provide, delayed subscription evaluation, and relaxed DDM | [Relevance Advisory](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md); [Attribute Scope Advisory](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md); [Auto Provide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md); [Delay Subscription Evaluation](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md); [Data Distribution and Regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md) |
| MOM objects/reports, service invocation reports, and exception reports | [Federation MOM](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md); [MOM object and interaction reports](HLA-2025-MOM-OBJECT-AND-INTERACTION-REPORTS-FLOW-GUIDE.md); [Joined-Federate MOM Object](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md); [Service Invocation Reporting](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md); [Exception Reporting](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md) |

The survey found one focused reader omission: the lifecycle guide covered the
member-roster query but did not explain the distinct
`listFederationExecutions()` / `reportFederationExecutions()` pair. The guide
now describes its catalog snapshot, callback-model boundary, and relationship
to member listing, with existing embedded and configured-process test evidence.
Other major service and callback families have a clear HLA-first guide or
cross-link; no extra guide is justified by API method count alone.

A 2026-11-04 symbol-level follow-up found two bulk reservation callback names
missing from the guide text, although their success/failure subset behavior
was already diagrammed, plus an explicit-MIM Create case whose pinned API
method name was not stated. The Name Reservation guide now names and links the
two callbacks, records that focused tests do not assert their relative entry
order, and distinguishes current source submission order. The FOM guide now
links `createFederationExecutionWithMIM` to the pinned interface. No major API
family was missing from the guide map; these additions improve navigation
within already-covered flows.

2025 non-timing coverage survey (2026-11-04): Cross-checked the pinned
RTIambassador/FederateAmbassador family map against the 31 topic guides and
the open reader-gap backlog. Each principal service/callback family has a
dedicated HLA-first guide or a clear cross-link; this survey found no missing
major family and does not justify another guide by API count. One lifecycle
reader gap did remain: the empty-execution state was explained separately from
the catalog and member-roster queries. The Lifecycle guide now puts their
observable results side by side. Its empty-roster result is explicitly
source-derived because the focused embedded and process cases do not query the
roster of an existing zero-member execution. Continue with specific reader or
evidence gaps rather than expanding the guide count. No tests, builds, CI,
Requirements Lab, or 2010 content changed.

2025 configured-process AttributeUpdate Push/Pull cross-audit (2026-11-04):
Expanded the Process Endpoint receiver sequence to distinguish a full Push event,
a Push readiness marker followed by authoritative-event claim and recheck, and a
Pull `receive_interaction` response with a rechecked `attributeEvent`. The
regional process RAVU case pairs Push with `HLA_IMMEDIATE` and Pull with
`HLA_EVOKED`; this is fixture configuration, not a callback-model rule. The
ordinary public process reflection case covers byte-preserving delivery under
both callback models. Added cross-links from Time Management and RAVU. The revised
Mermaid sequence rendered with Mermaid CLI 12.0.0 and passed local screenshot
review. Inventory remains 31 guides / 390 diagrams. No tests, builds, CI,
Requirements Lab, or 2010 content changed.

2025 configured-process cross-family callback queue review (2026-11-04): The
client buffers interaction events, full attribute reflections, deferred markers,
and poll results in one FIFO and submits the corresponding bridge work to the
shared callback dispatcher. The immediate dispatcher drains inline and permits
same-thread re-entry; Evoke drains queued work FIFO. Focused process tests assert
interaction-before-grant and reflection-before-grant separately. The RAVU test
checks its marker-versus-poll fixture paths, but no focused public process test
records mixed interaction/reflection/marker callback entry order. Added the
source/test boundary to the Process Endpoint guide; no new diagram was needed.
This is implementation scheduling evidence, not a normative callback-order
finding. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 configured-process Resign and ConnectionLost queue review (2026-11-04):
Corrected the Callback and Service Ordering guide's Resign account: successful
Resign suppresses client event admission, clears buffered frames, cancels
projections, and resets tasks still pending in the shared dispatcher. Reset
re-enables callbacks but does not interrupt a task already selected for
invocation. The client drops frames received while no joined identity exists;
old-session correlation after a later Join remains unverified. Process
ConnectionLost instead retains the old client and submits its local notification
without clearing either queue, so buffered event work and ConnectionLost share
the dispatcher without an established cross-thread order. Later client close
clears remaining events, closes the callback session, waits for active
invocations, and resets pending tasks. Existing loss-named public tests inject
embedded transport failure; no focused public process case covers these queue
edges. Updated the main diagrams and appendix consistently. Three existing
Mermaid diagrams rendered with CLI 12.0.0 and passed local screenshot review.
Inventory remains 31 guides / 391 diagrams; selected visual QA remains 123
reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010
content changed.

The follow-up handle/name query audit found no additional semantic gap. The
existing guides distinguish federate-local object knowledge from model lookup,
class-scoped member lookup, typed handle encoding/decoding, execution-scoped
normalization, update-rate queries, and FOM dimension availability from region
dimension/range queries. Their focused-test limits remain explicit; no duplicate
chart or guide was warranted. The subsequent bounded review of the cross-guide
boundary between Attribute Scope Advisory and regional subscription changes is
recorded below.

The Attribute Scope Advisory/DDM cross-audit likewise found no new transition
or chart to add. The current guide documents the receiver's own final-unsubscribe
exception and the different embedded, process-pull, and process-push stale-work
checkpoints. It explicitly says the focused switch-off test is confounded with
that unsubscribe exception and the process tests do not cover a stale-event
switch race. The related ownership-transfer effects on attribute order and
transportation are covered by the [Order and Transportation guide]
(HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md). The active next review is
tracked in the documentation handoff in `ROADMAP-INDEX.json`.

2025 member-exit cross-review (2026-11-05): Compared pending Synchronization
barrier departure with Save/Restore failure and the local ConnectionLost
callback path. Existing guides already show the separate state transitions,
failure-reason selection, survivor routing, and profile-specific queue limits;
there is no evidence for a single callback order across them. Added a thematic
reader route connecting those guides and their test limits. No new diagram was
needed. Active-operation ConnectionLost and configured-process member-exit
outcomes remain untested; these are source observations, not new normative
claims. No build, test rerun, CI, source, Requirements Lab, or 2010 change.

2025 Synchronization and Save/Restore terminal-outcome review (2026-11-05):
Compared the last participant report, federation operation resolution, and
per-recipient callback observation. Existing guides distinguish the HLA
federation outcome from participant completion and from the configured-process
private correlated `applied` response. The callback guide separates pushed
events from polling and `HLA_IMMEDIATE` from `HLA_EVOKED`; the Save/Restore guide
also records a source-only pushed-notification failure after save resolution.
No single cross-protocol callback order is established. Added a thematic reader
route; existing diagrams already cover the transitions. No test rerun, build,
CI, source, Requirements Lab, or 2010 change.

2025 durable Save across execution destruction review (2026-11-05): The
Lifecycle guide already diagrams successful Destroy, same-name Create, Join,
and explicit Restore. A focused public 2025 test source exercises a completed
multi-member save, destroys the original execution, constructs a fresh embedded
registry over the same filesystem store, recreates the same name, rejoins, and
restores under both callback models. This is new-registry evidence within one
OS process, not RTI process restart or arbitrary crash recovery. Added a direct
cross-link and evidence row to Save/Restore, and clarified that distinction in
the main reading route. No new diagram. The exact normative persistence scope
at §4.1.2 remains open without the authorized clause text. No test rerun, build,
CI, source, Requirements Lab, or 2010 change.

2025 Save/Restore member-exit adapter review (2026-11-03): Reviewed voluntary
Resign and forced ConnectionLost while Save/Restore is active. Active Save
failure reason depends on the trigger; active Restore uses
`FEDERATE_RESIGNED_DURING_RESTORE` for both. Embedded adapters submit planned
notifications for either path; the configured-process dispatcher enqueues Save
notifications but omits Restore notifications before the ordinary event
pipeline. Focused embedded resignation cases cover bounded paths; no
active-operation connection-loss or process member-exit outcome is established.
Updated the existing Lifecycle chart and Process Endpoint cross-reference.
Mermaid CLI 12.0.0 rendering and local screenshot review passed. Inventory is
31 guides / 387 diagrams; selected visual QA is 116 reviewed / 0 pending. No
tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 synchronization departure adapter-order review (2026-11-03): Cross-checked late Join, voluntary Resign, and forced connection loss in the Synchronization guide against Lifecycle and Process Endpoint. The participant-set flow matches the inspected registry: departure removes the ID from synchronization, announced, and achieved sets; the point remains pending if remaining members are unreported, completes for remaining routed members when they have all reported, and erases without callbacks when none remain. Clarified source adapter order on the existing departure chart: embedded Resign and ConnectionLost submit synchronization work before object-removal work; the configured-process shared dispatcher enqueues removals before synchronization events. These are source call/enqueue orders, not callback-entry guarantees. The embedded HLA_EVOKED resignation test observes barrier completion but has no object-removal callback; 38 loss-focused Catch2 sources and the process connection-lifecycle TCK source contain no direct FederationSynchronized case. Added a reciprocal Lifecycle link and focused source/test evidence. Mermaid CLI 12.0.0 rendered the revised chart and screenshot review found it readable. Inventory remains 31 guides / 387 diagrams; selected visual QA is 115 reviewed / 0 pending. No tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 Federation/member lifecycle Join-discovery boundary review (2026-11-03): Clarified the lifecycle chart's discovery arrow: eligible existing members may discover the newly joined member's RTI-owned HLAfederate MOM object. This is observer-side MOM visibility, distinct from ordinary application-object discovery planned when a member's subscription is evaluated. Embedded and configured-process Join paths use the MOM-specific planner after their respective membership/session setup; their subscription paths separately plan ordinary objects and MOM instances for the subscribing member. The embedded public lifecycle case and process rejoin/report case observe a fresh later member MOM instance through an already subscribed observer. They do not establish universal callback ordering. Updated the existing diagram and its source/test links; Mermaid CLI 12.0.0 rendered it and local screenshot review found it readable. Inventory remains 31 guides / 387 diagrams; selected visual QA is 114 reviewed / 0 pending. No tests, builds, CI, Requirements Lab, or 2010 content changed.

2025 configured-process attribute-reflection asynchronous-gate boundary review (2026-11-03): The process callback bridge sends receive-order interactions through `submitReceiveOrder`, which checks the bridge's asynchronous receive-order state. Process attribute updates use `submitAttributeUpdate` and are submitted to the callback dispatcher without that interaction gate; RAVU Provide callbacks use another direct dispatcher route. The focused process async-delivery tests exercise receive-order interactions with a constrained receiver and enable/disable/pending-advance transitions. The focused process attribute-reflection test checks callback delivery but does not set Time Constrained or asynchronous delivery. No test combines a constrained process receiver, attribute reflection, and async-switch changes. The Time Management guide already labels the interaction-only gate and explicitly declines to extend it to process attribute reflections. Added a precise cross-link from the RAVU timing synthesis; no new diagram was needed. Checked ten changed local destinations across the RAVU section, backlog, and ledger (two source anchors, three test anchors, and five guide headings); all resolve. This remains a source/test boundary, not a normative ruling or profile-parity claim. The existing RAVU section's eight destinations plus the new guide-heading link all resolve. Inventory remains 31 guides / 385 diagrams; selected visual QA remains 111 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

Latest 2025 regional RAVU/timing-gate cross-review (2026-11-03): Added a compact synthesis to the RAVU guide connecting provider solicitation, a separate later Update, actual receiver DDM qualification, RECEIVE versus TIMESTAMP timing, and callback-model observation. Existing guides covered each component but not the combined reader path. The embedded regional RAVU response case observes the timestamped reflection before its matching grant under HLA_EVOKED. The configured-process regional RAVU case covers provider solicitation and a later receive-order update under both callback models, but does not enable Time Constrained or asynchronous delivery. Process async-delivery tests exercise receive-order interactions, not RAVU. The inspected process bridge submits Provide callbacks through a separate dispatcher route from the interaction receive-order gate; do not infer process parity for Provide or attribute-reflection timing. Mermaid CLI 12.0.0 rendered the new chart and local screenshot review found it readable and unclipped. Checked eight local destinations (two source anchors, three test anchors, and three guide headings, including the backlog link); all resolve. Inventory is 31 guides / 385 diagrams; selected visual QA is 111 reviewed / 0 pending. The process attribute-reflection gate remains an explicit source/test gap. No tests, builds, CI, Requirements Lab, or 2010 content changed.

Latest 2025 configured-process Evoke Multiple pump review (2026-11-03): Updated
the Callback and Service Ordering guide to separate the initial process receive
pump, the timed callback-task drain, and pushed-event handling during or after
that wait. When no task is already queued, the enabled pump repeats receive
polls while they yield work; the disabled path permits at most one poll. A
pushed frame can be drained by another correlated client exchange during the
wait, or by the adapter's post-wait pass. The adapter may then run a zero-wait
dispatch pass if tasks remain, and its return reflects remaining work. The
focused process test covers one event admitted while callbacks are disabled;
it does not cover enabled multi-poll draining or pushed arrival during the wait.
The separate FIFO test uses Evoke Callback. Mermaid CLI 12.0.0 rendered the
new flowchart and local screenshot review found it readable. Checked 11 local
destinations (seven source anchors, two test anchors, and two guide headings);
all resolve. Inventory remains 31 guides / 384 diagrams; selected visual QA is
110 reviewed / 0 pending. This is source/test evidence, not a normative poll
count or cross-service ordering rule. No tests, builds, CI, or Requirements
Lab.

Latest 2025 passive interaction candidates with DSE review (2026-11-03): Cross-read the Delay Subscription Evaluation and Object/Interaction guides against the embedded route-only planner, current interaction recipient selector, callback-entry call site, and focused tests. Added a bounded explanation that the embedded callback recheck recomputes both federation-wide active class relevance and the receiver's current subscription/region. A retained passive route can become eligible or be suppressed as another member's active declaration changes before callback dispatch. The focused DSE interaction test changes the receiver's own declaration, not a separate active member; the three-member transition remains source-derived. Configured-process receive-order events use a separate event-construction recheck, with no equivalent callback-entry re-evaluation shown in the inspected bridge. Added a cross-link to the service-specific passive-gate comparison; no new diagram was needed. This is source/test evidence only, without a normative or process-parity claim. Checked six local destinations (four source anchors, one test anchor, one guide heading), separately from the aggregate. Inventory remains 31 guides / 383 diagrams; selected visual QA remains 109 reviewed / 0 pending. No tests, builds, CI, or Requirements Lab.

Latest 2025 configured-process ordinary interaction DSE cross-review
(2026-11-03): The existing Object/Interaction sequence already connects
send-time recipient planning, route-only candidate re-resolution before event
construction, the event-time class/parameter projection, Push/Pull delivery,
and callback-bridge dispatch. The Process Endpoint and DSE guides now link to
that sequence, so no duplicate flow was needed. The inspected process bridge
delivers the projected event without an equivalent callback-entry eligibility
recheck; this does not establish parity with the embedded DSE callback path.
Focused process ordinary-interaction and process DSE attribute-update cases
exist separately, but no focused process interaction DSE mutation test was
found. The process timing boundary remains source-derived. Checked four local
link occurrences (one file destination and three heading anchors); all resolve
and are not folded into the historical aggregate. Inventory remains 31 guides
/ 383 diagrams; selected visual QA remains 109 reviewed / 0 pending. No tests,
builds, CI, or Requirements Lab.

Latest 2025 process interaction send/callback timing review (2026-11-03): The
Process Endpoint guide already separates the sender's correlated send response
from receiver event receipt and callback dispatch. Its focused multi-recipient
case sends both messages before driving the receivers and therefore does not
establish callback ordering relative to sender return. The pinned untimed API
signature is `void`; neither it nor the test makes callback completion part of
the send result. During the follow-on callback-gate review, added a bounded
configured-process note: `HLA_EVOKED` can poll one event while callbacks are
disabled, leaving the callback task pending until callbacks are enabled and a
later Evoke; the `HLA_IMMEDIATE` test admits a pushed event while disabled and
drains it on enable. The two transport settings are test fixture choices, not
an HLA model/transport rule. Existing callback diagrams explain the general
gate, so no duplicate chart was needed. Checked five local destinations (four
source/test anchors and one guide heading); all resolve and are outside the
historical aggregate. Inventory remains 31 guides / 383 diagrams; selected
visual QA remains 109 reviewed / 0 pending. No tests, builds, CI, or
Requirements Lab.


2025 configured-process timestamped-send validation review (2026-11-03):
The ordinary timestamped Send Interaction service checks the sender's applicable
time-plus-lookahead floor and maps its `invalid_logical_time` response back to
`InvalidLogicalTime`; a focused process FQR-arrival case observes a time-4 send
rejected after a grant at 5. The directed timestamped path validates encoded
finite time and routes TSO work by the producer's regulation state, but the
inspected handler/enqueue path has no visible sender-floor check and the
directed adapter maps non-OK responses to `RTIinternalError`. No focused
configured-process directed below-floor case was found. Added this bounded
source/test comparison to Process Endpoint and linked it from Time Management;
no new diagram was warranted. This does not establish directed-send acceptance
or a normative conformance result. Inventory remains 31 guides / 379 diagrams;
selected visual QA is 105 reviewed / 0 pending. No source, tests, builds, CI,
Requirements Lab, or 2010 work.

2025 configured-process interaction-send failure-map review (2026-11-03):
Corrected the ordinary Send Interaction flow: a session mismatch or
non-applied registry plan produces generic rejection, then `RTIinternalError`;
the process service does not preserve remote publication or parameter statuses.
Added a separate Send Directed Interaction flow showing its additional target
object check. A locally invalid target handle maps to `ObjectInstanceNotKnown`,
while remote non-applied plans use the same generic rejection boundary. Both
updated flows rendered with Mermaid CLI 12.0.0 and passed local screenshot
review. Focused untimed public process sends cover successful calls only;
negative remote planner outcomes remain source-derived. Inventory: 31 guides / 379 diagrams;
selected visual QA is 105 reviewed / 0 pending. No source, tests, builds, CI,
Requirements Lab, or 2010 work.

2025 configured-process Update Attribute Values failure-map correction
(2026-11-03): the session/federation/member identity mismatch alone returns a
generic rejected transport result; a valid-session registry planner failure
returns structured status and maps to typed public exceptions. Revised the
Process Endpoint flow and replaced the duplicate Object/Interaction flow with a
linked semantic summary. Mermaid CLI 12.0.0 rendering and screenshot review
passed. The focused process test covers valid untimed updates only, so negative
public outcomes remain source-derived. Inventory: 31 guides / 378 diagrams;
selected visual QA remains 104 reviewed / 0 pending. No source, tests, builds,
CI, Requirements Lab, or 2010 work.

2025 configured-process Enable Time Regulation and implicit-TARA review (2026-11-03):
The service has a direct response path that commits the role/lookahead before
returning `enabledTime`, and a constrained-member path that records a pending
lookahead and implicit TARA. The latter dispatches eligible TSO first, commits
at the computed target, then sends a grant event marked `regulationEnabled`;
the client maps it to `TimeRegulationEnabled`, not a public
`TimeAdvanceGrant`. The real process-service `HLA_EVOKED` case uses peer time
10, lookahead 2, target 8, and a queued interaction at 5; it observes the
interaction before the regulation callback and no separate grant callback.
The `HLA_IMMEDIATE` public-endpoint case uses a controlled server and covers
callback dispatch, not target calculation. Added a source-labeled sequence to
the Time Management guide and a reciprocal Process Endpoint link. Mermaid CLI
12.0.0 rendering and screenshot review passed. Inventory: 31 guides / 379
diagrams, including 140 sequences; selected visual QA is 104 reviewed / 0
pending. Source/test observations only, without a normative target formula or
process/embedded parity claim. No source, tests, builds, CI, or Requirements Lab
changed.

2025 additional-module Join lookup review (2026-11-03): The FOM guide
already explains replacement-definition preparation, atomic definition/member
commit, post-commit setup, and the tested limits of rollback evidence. The
Object/Interaction guide already links interaction catalog growth to its
separate “no automatic Receive Interaction” boundary. The Name/Handle guide's
Join example had focused object-class and interaction-class coverage; the
review found additional embedded public cases for attributes, parameters, and
transportation types and added a compact five-family evidence matrix. Existing
handles compare equal within the tested execution; the transportation case
specifically adds a type that sorts earlier and checks both members. Registry
source also reconciles dimensions, but no focused added-dimension lookup test
was found. The configured-process additional-FOM case proves object-class
lookup only. No test injects post-commit failure with replacement modules. No
extra diagram was needed because the existing guides link these boundaries.
Checked the new local references; inventory remains 31 guides / 379 diagrams,
selected visual QA 104 / 0. No source, tests, builds, CI, Requirements Lab, or
2010 work.

Earlier 2025 pending Time Constrained Restore and GALT re-evaluation review (2026-11-03):
Cross-read the Time Management role state machine, Save/Restore guide, bounded
embedded source, and the fresh-filesystem-registry case. The saved image keeps
the role-off state and pending generation, not a callback closure. At the final
Restore barrier, the registry applies saved time state and rebuilds role work
only when bounds are available or undefined and, when GALT exists, restored
current time is strictly below GALT. Otherwise the generation remains pending.
The embedded dispatch activates the role immediately before the public callback;
under `HLA_EVOKED`, this waits until Evoke runs the queued work.

Follow-up source review traced later eligibility through the ordinary
`reevaluateTimeAdvanceGrants` path: a regulator's time-advance grant updates
its time state and rechecks pending constrained-role enables. The focused
ordinary-enable GALT case demonstrates that GALT equal to current time remains
blocked, while GALT 6 for current time 5 makes the callback eligible; the
callback remains queued until Evoke. That case does not save and restore the
pending request, so the restored blocked branch remains source-derived. Added
an eligible/blocked Restore sequence and cross-links to the role and GALT
evidence. All 19 local source/test/guide link references in the revised sections resolve. The focused one-member Restore test observes both the role callback
and `federationRestored` after completion, but does not assert their relative
order, inspect the active role flag, or cover GALT blocking. Mermaid CLI 12.0.0
render and screenshot review passed. Inventory is 31 guides / 377 diagrams,
including 138 sequences; selected visual QA is 102 reviewed / 0 pending. This
is embedded implementation/test evidence, not process-endpoint equivalence.
No source, tests, builds, CI, or Requirements Lab changed.

Earlier 2025 Restore admission and FQR/timed-Save timing review (2026-11-03):
Confirmed that the current Restore registry has no time-advance admission gate:
after image, participant-set, and route validation, it creates the active
operation and plans success/begun/initiate notifications. Clarified that the
pending time-constrained boundary belongs to Save, while restoring a saved
pending Enable Time Constrained request state is a later state-restoration concern.
Added an embedded `HLA_EVOKED` sequence showing FQR actual 5 / optimistic 7,
queried logical time 5, a timed Save at 6 remaining pending, and
`initiateFederateSave(time=6)` before `TimeAdvanceGrant(7)` when TAR(7) crosses
the boundary. The focused case asserts this order; no normative FQR or callback
ordering claim is made. Mermaid CLI 12.0.0 rendered the chart and screenshot
review found it readable. Inventory is 31 guides / 376 diagrams; selected
visual QA is 101 reviewed / 0 pending. No source, tests, builds, CI, or
Requirements Lab changed.

Latest 2025 synchronization/Save-Restore membership-boundary cross-review
(2026-11-03): Compared Federation Synchronization, Federation Save/Restore, and
Lifecycle. Synchronization registration records whether the participant set was
omitted; the embedded late-Join announcement expands only that expandable
barrier. Save and Restore initialize per-member status and callback work from the
current membership when each protocol begins. The pinned Join API annotations
list `SaveInProgress` and `RestoreInProgress`, but the inspected paths and
focused tests do not resolve Join during an active protocol. The existing
cross-guide map already keeps these participant models distinct, so no diagram
or guide edit was warranted. Inventory remains 31 guides / 374 diagrams, with
99 selected charts reviewed and none pending. No tests, builds, CI, Requirements
Lab, or other-edition work.

Latest 2025 Federation MOM/final-member lifecycle cross-review (2026-11-03): compared Federation Lifecycle, Federation MOM, and Joined-Federate MOM Object Lifecycle. Revised the existing projection state chart to distinguish a not-yet-established projection, joined membership, an empty execution, same-execution rejoin, and Destroy. Registry source retains and reuses the HLAfederation projection entry until Destroy; member HLAfederate objects have separate lifetimes. Embedded and process tests cover adjacent transitions but do not directly observe federation-object continuity through an empty-to-rejoin interval. Mermaid CLI 12.0.0 rendered and screenshot review found the updated chart readable. Inventory remains 31 guides / 365 diagrams, and selected visual QA remains 83 reviewed / 0 pending. No code, tests, builds, CI, Requirements Lab, or 2010 content changed.

Latest 2025 subscription/update-rate/DSE cross-review (2026-11-03): traced ordinary and regional declaration generations, retained DSE candidates, and receive-order/TSO delivery projections across the Subscription State, Attribute Update Rate, and DSE guides. Embedded source rebuilds the receiver projection at callback/grant delivery and uses its current generation in the per-attribute rate key. The regional rate integration case directly observes fresh admission after unsubscribe/resubscribe; separate DSE tests cover late eligibility and later suppression. No focused case combines retained DSE work with rate-history admission, so that composition remains source-derived. Corrected stale generation-field links and added reciprocal guide links. The empty-inner-region rule remains a source/test inference pending authorized §9.8.4 review. No tests, builds, CI, Requirements Lab, or 2010 work.

Latest 2025 ownership/TSO/callback cross-review (2026-11-03): compared Attribute Ownership, Time Management, TSO Retraction, and Callback/Service Ordering. The focused embedded scenario accepts a timestamped update while A owns the attribute, then divests, acquires through If Available after divestiture, and later reflects the queued value with A still named as producer. This demonstrates a queued TSO path across an ownership change; it does not test regular acquisition waiting while A remains owner or configured-process parity. Clarified that distinction in the Ownership guide and added direct callback-ordering links. Existing diagrams already show the transitions, so no new chart was needed. No tests, builds, CI, Requirements Lab, or 2010 work.

Latest 2025 encoding and deferred-evaluation visual review (2026-11-03): rendered and screenshot-reviewed the eight 2025 Data Element Encoding diagrams, the Attribute Scope process-event transport chart, and the configured-process attribute timing chart in Delay Subscription Evaluation. Local screenshots found the diagrams readable; GitHub rendering remains unverified. The curated visual-QA inventory is 97 reviewed / 0 pending. No guide behavior, source, tests, builds, CI, Requirements Lab, or 2010 implementation changed.

Latest 2025 Federation MOM/Restore cross-review (2026-11-03): cross-read Federation MOM, Federation Save/Restore, and Joined-Federate MOM Object Lifecycle. Added a Restore flow separating the retained process-local `Federation` copy, which includes the RTI-owned MOM object map, from the bounded durable `FederationStateImage`, which does not encode that map. `HLAfederateState` remains a live restore-ledger projection (`5` in progress, `1` complete); the embedded test matches both reflections to the object handle discovered before Restore. The inspected embedded completion path does not call joined-MOM discovery/removal planners, but the test does not assert the absence or ordering of those callbacks. The federation `HLAfederatesInFederation` value is encoded from current live members; the inspected successful Restore path does not enqueue its Join/Resign conditional update. Existing tests cover its request and Join/Resign observations, not Restore; configured-process list reflection evidence is absent. Mermaid CLI 12.0.0 rendered the new chart and screenshot review found it readable. Inventory is 31 guides / 366 diagrams; selected visual QA is 84 reviewed / 0 pending. No source, Requirements Lab, 2010, tests, builds, or CI changed.

Latest 2025 Federation save-history review (2026-11-03): compared the four federation-scoped HLAnextSave* and HLAlastSave* fields across the 2025 MIM, embedded timed-save callbacks, durable-image capture, and image rehydration. Next fields clear at admission; a committed image captures prior last-save values; live last-save values advance only after successful commit. The public embedded case observes pending/cleared/updated values, while a separate filesystem-image case confirms prior history is carried through Restore into a later image. Added source-only participant-failure, abort, and encode/commit-failure branches to the sequence: these paths retain the previous live last-save values, but focused tests assert terminal outcomes rather than the four MOM fields. No public after-Restore MOM value or configured-process save-history assertion is covered. The expanded sequence rendered with Mermaid CLI 12.0.0 and passed local screenshot review. Inventory is 31 guides / 367 diagrams; selected visual QA is 85 reviewed / 0 pending. No source, Requirements Lab, 2010, tests, builds, or CI changed.
Latest 2025 joined-federate HLAfederateState failure review (2026-11-03): cross-read the MIM update condition, live state encoder, and save-notification branches. Failed, aborted, and commit-failed Save paths clear the registry operation and leave the live encoder at `1`; the inspected embedded dispatcher queues the return-to-`1` reflection after successful `federationSaved`, not after `federationNotSaved`. The MIM names successful Federation Saved, but not Federation Not Saved or Abort Federation Save, as a Conditional trigger. Existing public state coverage observes success; failure tests assert terminal reasons rather than state reflections. Revised the existing state chart and linked the separate Federation-scoped save-history guide. Mermaid CLI 12.0.0 rendered the chart and local screenshot review found it readable. Inventory remains 31 guides / 367 diagrams; selected visual QA is 85 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

Latest 2025 configured-process HLAfederateState Save callback review (2026-11-03): traced registry completion through the per-recipient process event, service-report record, Push or queued ReceiveInteraction delivery, and callback bridge. The inspected process queue/bridge has no per-member conditional state-update event; the embedded success branch does queue one after `federationSaved`. The MIM names successful Federation Saved as a Conditional trigger, while the focused process failure case checks the callback reason but not MOM state. Process automatic reflection therefore remains unverified; no profile-parity or conformance claim is made. Added a separate process-profile sequence and locally rendered/screenshot-reviewed it with Mermaid CLI 12.0.0. Inventory is 31 guides / 368 diagrams; selected visual QA is 86 reviewed / 0 pending. No tests, builds, CI, Requirements Lab, or 2010 content changed.

Latest 2025 process HLAfederateState explicit-value request review (2026-11-03): after a failed Save, the cleared ledger and live encoder source predict a request-time value of `1`. The process service handles RTI-owned MOM values directly and emits an attribute-update event rather than soliciting a provider callback. Push delivery is source-traceable to `reflectAttributeValues`; the queued route has a dequeue endpoint and a dedicated ambassador pump definition, but no pump call site was found in the inspected runtime. No configured-process test asserts this request after failed Save; the embedded request test covers a successful Save/Restore flow. The direct requester-entitlement interpretation remains unresolved. Added a separate request-time sequence and locally rendered/screenshot-reviewed it with Mermaid CLI 12.0.0. Inventory is 31 guides / 369 diagrams; selected visual QA is 87 reviewed / 0 pending. No tests, builds, CI, Requirements Lab, or 2010 content changed.

Latest 2025 joined-federate HLAfederateState failed/aborted Restore review (2026-11-03): member-reported failure, explicit abort, and a preparation/application exception caught while the restore operation remains active plan failed notifications and clear the restore ledger; the live encoder then derives `ActiveFederate` (`1`). The inspected embedded and configured-process failure callback routes queue no conditional state update, and the MIM does not name `Federation Not Restored` as a trigger. Configured-process tests assert failure reason and callback order, not state values/reflections. Added a 2025-only flow distinguishing the live projection from embedded/process event delivery; Mermaid CLI 12.0.0 rendered it and local screenshot review found it readable. Follow-up source review found the post-apply route checks are defensive invariant guards: Restore admission checks every current member's callback route, resignation clears Restore before removing a route, state replacement preserves live routes, and durable identity rebinding is limited to mapped live-member ledgers. No accepted path with a missing route was demonstrated; the after-reset catch outcome remains untested. Inventory is 31 guides / 370 diagrams; selected visual QA is 88 reviewed / 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

Latest class-hierarchy reader-gap review (2026-11-03): amended the existing
Object and Interaction guide map to show publication as its own class-keyed
gate: object registration checks the selected registration class's published
attributes, and interaction send checks publication of the exact sent class.
The existing registry cases exercise same-class publication and use; they do
not test parent publication followed by a child-class operation. This is
source-bounded, not a normative inheritance conclusion. Mermaid CLI 12.0.0
rendered the revised map and local screenshot review found it readable. The
selected visual-QA count is 80, with none pending. No source, tests, builds,
Requirements Lab, or 2010 content changed.

Latest 2025 timing cross-review (2026-11-03): compared timestamped interaction
and attribute delivery across the Time Management, Object and Interaction,
DDM/Regions, and Callback and Service Ordering guides. They consistently keep
per-recipient TSO eligibility, per-attribute RECEIVE/TIMESTAMP splitting,
profile-specific projection timing, DDM scope, callback admission, and grant
observation as distinct transitions. No contradiction or missing reader
diagram was found; adding another combined chart would duplicate the existing
maps. Exact `reflectAttributeValues` clause interpretation remains open pending
an authorized canonical copy. No code, tests, builds, or CI changed.

Latest 2025 ownership review (2026-11-03): cross-read the Attribute Ownership
and Ownership Persistence and Resignation guides. Their existing diagrams
already distinguish request acceptance, transfer, callback-entry ownership
changes, queued operations, and selected save/restore and resignation paths; no
duplicate synthesis diagram was warranted. The core guide's negotiated-
divestiture cancellation sequence had stale validation guidance saying Mermaid
CLI was unavailable. Mermaid CLI 12.0.0 rendered it and local screenshot review
found the numbered service/callback path readable without clipping. This closes
that chart's local validation gap, not GitHub-renderer verification. Selected
visual QA is now 81 reviewed, 0 pending. No source, tests, builds, CI,
Requirements Lab, or 2010 content changed.

Latest 2025 authorization/reporting cross-review (2026-11-03): verified that Connect denial is pre-connection, while lifecycle catch sites invoke an exception-report hook that requires current joined membership. Per-invocation reporting is separate; no synthesis chart or profile-parity claim was added.

Latest 2025 Federation MOM request/report cross-review (2026-11-03): confirmed that conditional/current-value attribute reflections, request-triggered receive-order report interactions, and exception reports have distinct causes and recipient paths. Existing diagrams are sufficient; no new chart or process-parity claim was warranted.

Latest 2025 member-MOM cadence/time cross-review (2026-11-03): clarified that HLAsetTiming controls wall-clock refresh of per-member Periodic attributes, while logical-time grants belong to the simulation-time coordinator. Added reciprocal links between the Joined-Federate MOM and Time Management guides; existing diagrams already separate explicit requests, refresh cadence, and callback observation. Existing focused test limits and lack of process parity remain explicit. No code, tests, builds, or CI changed.

Latest 2025 periodic MOM value-semantics cross-review (2026-11-03): compared the member-MOM value groups with the MIM, Time Management, and Object and Interaction guides. Existing text distinguishes current queue gauges, simulation-time state, service totals, distinct-instance counts, and wall-clock durations from the HLAsetTiming refresh cadence. No new chart or wording was needed; exact callback latency and process parity remain unclaimed.

Latest 2025 object-registration cross-review (2026-11-03): the Object and Interaction, Subscription State, and Attribute Ownership guides keep producer registration, receiver discovery, initial-state reflection, later updates, explicit RAVU, and ownership notification separate. Existing diagrams preserve the profile and normative limits; no synthesis chart was needed.

Latest 2025 name-reservation/lifecycle cross-review (2026-11-03): reservation callbacks, successful registration commit, receiver discovery, local delete, global name purge, resignation, and timestamped deletion remain separate transitions. Existing guides cover the distinctions; the process reservation-to-peer-discovery evidence gap remains explicit. No new diagram or parity claim.

Latest 2025 timestamped-delete/name-reuse cross-review (2026-11-03): the TSO Retraction, Object and Interaction, Name Reservation, and Federation Lifecycle guides keep request acceptance, recipient removal commit, execution deletion, receiver-local forgetting, and shared-name reuse separate. The focused owner-only terminal-deletion test additionally observes successful reservation and registration of the same name after the timestamped deletion becomes non-retractable. It does not cover multi-recipient removal/callback timing or configured-process behavior. Existing maps are sufficient; no new diagram was needed.

Latest 2025 RAVU requester/receiver eligibility review (2026-11-03): cross-checked the Attribute Value Update Request, Subscription State, Object/Interaction, Auto Provide, and Delay Subscription Evaluation guides. They consistently separate explicit provider solicitation, discovery-triggered Auto Provide, and receiver eligibility for a later value-bearing Update. Focused RAVU response cases have a subscription context; the process fixture seeds that context through the registry, and the DSE case contains no RAVU. No focused case keeps an otherwise-ineligible requester through an explicit request and provider response, so direct §6.21 entitlement remains unresolved. The guide already records this limit; no diagram or wording change was warranted. No tests, builds, CI, Requirements Lab, or 2010 work.

Latest 2025 late-subscription state-boundary cross-review (2026-11-03): compared Subscription State, Object/Interaction Information, Auto Provide, DSE, and RAVU. A subscription that makes an object newly known leads to discovery and selected current-value reflection in the embedded path; the configured-process test source asserts selected stored values under both callback-model sections, but was not run here. Adding an attribute after the receiver already knows the object is separate: current embedded/process source shows no discovery-scoped initial-value route, while the focused `PayRate` process case asserts only the provider relevance advisory. The value-only effect remains untested and §5.8 remains normatively unresolved. Existing maps distinguish it from DSE and RAVU; no new diagram was needed. Removed a stale worktree-status sentence from the Object/Interaction guide. No tests, builds, CI, Requirements Lab, or 2010 work.

Latest 2025 timestamped-delete callback/time cross-review (2026-11-03): clarified the distinct coordinator completion phase, retraction-ledger callback-entry state, and deletion's registry Remove commit gate. The Time Management guide now links directly to the Delete-specific traces; existing advance/callback diagrams remain sufficient.

2025 handle-encoding cross-review (2026-11-03): compared Data Element Encoding and Name and Handle Resolution. The guides correctly separate DataElement value bytes from opaque, category-typed handles. Current Umbra 2025 handle implementations share a codec that emits a 32-bit big-endian element count of eight followed by eight big-endian identity octets; this is implementation evidence, not a portable identity or cross-RTI byte-layout claim. Expanded the existing typed-handle flow to show RTIambassador connection/membership checks and CouldNotDecode, added the exact byte vector and cross-link to DataElement byte-order guidance, and linked source plus public/unit tests. The revised chart rendered with Mermaid CLI 12.0.0 and local screenshot review found it readable. Selected visual QA is 82 reviewed, 0 pending. No source, tests, builds, CI, Requirements Lab, or 2010 content changed.

Latest non-timing increment (2026-11-03): added a configured-process transport
recovery section and sequence to the Process Endpoint guide. It explains resuming
the same live private session, validating and replaying retained request/response
and event history, and the callback boundary after recovery. The diagram rendered
with Mermaid CLI 12.0.0 and passed local screenshot review. Existing 2026-10-30
focused recovery results were reused; no tests or builds were rerun. This is
source/test-bounded process behavior, not an HLA resumption rule or host-restart
durability claim. The new chart raises the current inventory to 364 and the
selected visual-QA count to 79.

Earlier 2025 reader-gap review (2026-11-03): corrected the Federation and
Federate Lifecycle guide's stale claim that configured-process member listing
had no route. The process service snapshots the roster before returning the
response; its client submits the report under the selected callback model. A
focused scenario covers both callback models and a later post-loss snapshot,
but not membership mutation between response and callback or a separately
launched coordinator process. The embedded route remains separately described.
Reviewed the member-list API map, the new configured-process sequence, and the
coordinator-stop flow locally; all three are readable without clipping. The
guide has 17 diagrams, 203 relative destinations, and no missing or out-of-range
line anchors. Cross-reviewed the Federation MOM guide's late-observer advice:
the `HLAfederation` membership attribute carries federate-handle references and
can reflect later conditional changes, while `listFederationExecutionMembers`
returns a separate name/type report. Added this distinction and a lifecycle
cross-link; no duplicate diagram was needed. The MOM guide's 219 relative
destinations and 195 source line anchors resolve. The 2025 inventory is 364
diagrams, with 79 selected visual reviews and none pending. No source,
Requirements Lab, or 2010 content changed; no tests, builds, or CI were run.
The Connection and Configuration guide already separates Connect admission,
local connection commit, configuration/endpoint choice, and Disconnect from
Resign; it also keeps client socket closure distinct from later host cleanup.
Those timings and profile limits are source/test bounded. No new chart or
prose was warranted. All 48 relative destinations and 41 source line anchors
resolve. The Callback and Service Ordering guide already separates callback
eligibility, task submission, callback entry, and operation-specific state
commit; it distinguishes HLA_EVOKED return from later Evoke, HLA_IMMEDIATE
service-stack delivery, and Evoke Multiple's wall-clock wait window from HLA
logical time. Its evidence does not infer a global callback order. No new chart
was needed. All 224 relative destinations and 201 source line anchors resolve.
The Joined-Federate MOM guide already separates membership, the RTI's private
per-Join snapshot, and each receiver's known-object state. Its Join/discovery,
explicit-value-request, pending-removal, and fresh-rejoin traces preserve
embedded and process evidence limits. No extra chart was warranted. All 111
relative destinations and 97 source line anchors resolve. The cross-cutting
guide's four-question model treats authority, state, recipient eligibility, and
observation/completion as analysis lenses, not a universal protocol sequence.
Its delivery matrix distinguishes publication, ownership, relevance, receiver
knowledge, order, transportation, time eligibility, and callback entry; its
service examples keep commit and completion operation-specific. The
object-existence/receiver-knowledge map and Joined-Federate MOM cross-link are
consistent. No wording or chart change was warranted. All 145 relative
destinations and 74 source line anchors resolve. The FOM Module Admission and
Composition guide keeps Create preparation ahead of federation-name
reservation, resolves references over the completed merged model, and
separates the additional-module Join's atomic model/membership commit from
post-commit setup and observer effects. It distinguishes joined-member MOM
discovery, the embedded `HLAcurrentFDD` reflection, and ordinary application
object discovery; the process profile and normative callback limits are
explicit. No wording or diagram change was warranted. All 216 inline links,
local targets, source line anchors, and heading fragments resolve. No source,
Requirements Lab, or 2010 content changed; no tests, builds, or CI were run.
The Object and Interaction Information guide's registration/discovery section
now links to that FOM boundary: a catalog extension does not itself retype an
existing instance or establish ordinary rediscovery, while the joining
member's MOM discovery is separate. The source/test and normative limits remain
explicit. All 511 inline links, local targets, source line anchors, and heading
fragments resolve. No diagram change was warranted. No source, Requirements
Lab, or 2010 content changed; no tests, builds, or CI were run. The Object
Attribute Subscription State guide distinguishes an object newly known to a
receiver from an already-known object whose subscription changes. Added a note
separating both from additional-module Join; its 175 links and anchors resolve.
The Auto Provide guide already distinguishes provider solicitation from
stored-value reflection, explicit requests, MOM reflection, and advisories;
its 63 links resolve, with no edit needed. The Attribute Relevance Advisory
guide already separates owner-facing Turn Updates callbacks from receiver
scope and value delivery. Added a scope note that its subscription/scope/rate
transitions do not establish advisories from model-catalog growth on Join. All
62 links, targets, line anchors, and heading fragments resolve. The Attribute
Scope Advisory guide keeps scope as a receiver/object/attribute predicate and
separates its callback from owner relevance, declaration relevance, and value
reflection. Its embedded recheck and process pull/push checkpoints stay
distinct; clarified that switch-only and pending-event interleavings remain
untested. All 49 local links, source anchors, and heading fragments resolve.
No diagram change was warranted. No source, Requirements Lab, or 2010 content
changed; no tests, builds, or CI were run. The Data Distribution and Regions
guide keeps send-time source-region snapshots distinct from receiver-side
scope/projection checks, and routes eligible timestamped messages through a
separate logical-time boundary. The Time Management guide preserves that
split: DDM, TSO eligibility, and callback dispatch are independent questions,
while projection timing is profile-specific and not a normative freeze rule.
The DDM guide's 321 links and Time Management guide's 217 links resolve,
including source anchors and heading fragments. No chart change was needed.
No source, Requirements Lab, or 2010 content changed; no tests, builds, or CI
were run. The TSO Retraction guide keeps the producer's time-bound Retract
legality separate from each recipient's pending/delivered status and from each
receiver's outstanding advance. Its regional examples agree with the DDM and
Time Management guides' service-specific recheck boundaries; the guide's 117
local links, source anchors, and heading fragments resolve. No wording or
diagram change was warranted. No source, Requirements Lab, or 2010 content
changed; no tests, builds, or CI were run. The Federation Save/Restore guide's
regional interaction, timed-save, restored deletion, and retraction examples
align with the TSO guide's per-recipient state model. It separates durable
image records from callback frames queued in a still-live process and labels
fresh-registry tests as distinct from host restart or process-profile evidence.
All 206 local links, source anchors, and heading fragments resolve. No wording
or diagram change was warranted. No source, Requirements Lab, or 2010 content
changed; no tests, builds, or CI were run. The Ownership Persistence and
Resignation guide keeps typed saved ownership work, live callback routes, and
callbacks already admitted to a process HLA_EVOKED queue distinct. Its cases
show route-free reconstruction, member-scoped cancellation, surviving
recipients, and different order for retained versus reconstructed process
work, without claiming host restart or normative callback order. All 41 local
links, source anchors, and heading fragments resolve. No wording or diagram
change was warranted. No source, Requirements Lab, or 2010 content changed; no
tests, builds, or CI were run. The Attribute Ownership guide keeps object-
instance/attribute state, service return, transfer result, callback entry, and
ownership queries separate. Its accepted receive-order deletion path clears
pending ownership-operation ledgers, while receiver removal crosses its own
knowledge/membership gate; local forget and timestamped deletion/retraction
remain separate. The Ownership Persistence companion covers selected saved
work and live callback queues. All 287 links, source anchors, and heading
fragments resolve. No wording or diagram change was warranted. No source,
Requirements Lab, or 2010 content changed; no tests, builds, or CI were run.
The Object and Interaction Information guide confirms the same separation:
execution-wide deletion invalidates pending ownership work at acceptance,
then each receiver crosses its own membership/known-object removal gate;
`localDeleteObjectInstance` forgets only the caller's view, while timestamped
deletion and retraction use per-recipient time gates. The ownership and TSO
cross-links preserve their evidence boundaries. All 511 links, source anchors,
and heading fragments resolve. No wording or diagram change was warranted. No
source, Requirements Lab, or 2010 content changed; no tests, builds, or CI were
run. The Object Instance Name Reservation guide separates a caller-owned
reservation, registration-time consumption, and peer discovery; all 52 links,
targets, anchors, and fragments resolve. Names/handle and Federation/Joined-
Federate MOM guides agree that FOM symbols, federate handles, object-instance
handles, and observer discovery state are distinct. The synchronization guide
separates registration, announcement, achievement, and completion; the Data
Element guide keeps factory-defined time-value encoding and per-type endian
choices separate from logical-time advancement. The Directed Interaction and
DDM guides distinguish a target object handle, a region set, and a receiver
selector. The update-rate guide separates steady-clock admission from TSO grant
eligibility and transport reliability. Authorization separates its scoped
callback result from service preconditions and state commit; Delay Subscription
Evaluation keeps receiver eligibility separate from service validation, DDM,
temporal gates, and callback dispatch. The Exception Reporting guide now also
states that authorization results and mapped service outcomes are distinct
from observer-side reports; it keeps all three report families and
wrapper-specific routes separate. The Order and Transportation guide now
links its embedded best-effort attribute rate filter to the separate rate guide
and explains the filter's scope. No source, Requirements Lab, or 2010 content
changed; no tests, builds, or CI were run. Next: cross-review the 2025
Attribute Value Update Request guide now cross-links the receiver's
subscription-state distinctions and the ownership guide's per-attribute
authority model. The request, later provider update, and receiver DDM projection
remain separate; the direct-requester entitlement question under §6.21 remains
open pending authoritative clause text. No source, Requirements Lab, or 2010
content changed; no tests, builds, or CI were run. The 2025 Process Endpoint
Request and Callback cross-review confirms that the guide already separates
public service outcomes, server-side planning/commit, polling versus pushed
event delivery, and client callback entry. Cross-checking its Update Attribute
Values trace against RAVU clarified that this process section starts at the
provider's later update call; added reciprocal navigation to the RAVU guide.
Callback/Service Ordering remains the broader callback lifecycle reference,
while this guide owns process framing and delivery details. No duplicate chart
was warranted. No source, Requirements Lab, 2010, tests, builds, or CI changed.
The 2025 Federation Save/Restore cross-review confirms that the guide separates
application-owned state restoration, RTI admission and completion, selected
image rehydration, and callback observation. Its evidence map distinguishes
embedded TSO payload/retraction restoration from live process callback queues
and the separate process RAVU notification queue. Added a cross-link for the
process TSO callback-completion acknowledgement: its `inTransit` entry can
affect timed-save readiness, but no focused process Save/Restore case tests
that acknowledgement across restore. The process guide now links back to the
save-admission barrier. Existing diagrams and evidence limits were sufficient;
no duplicate diagram was warranted. No source, Requirements Lab, or 2010
content changed; no tests, builds, or CI were run. The 2025 Federation and
Federate Lifecycle cross-review confirms that voluntary Resign, transport
loss, and departure during an active Save/Restore are separate transitions.
Callback/Service Ordering already distinguishes the lost member's local
`connectionLost` callback from survivor removal callbacks and keeps embedded
dispatcher reset separate from process queued work. Save/Restore identifies
the protocol failure callbacks for remaining participants; the lifecycle guide
records that the process dispatch path does not project those events in the
inspected source, with no focused runtime test for that route. Added reciprocal
links from Callback/Service Ordering to the lifecycle cleanup and active
operation boundaries. No diagram change was warranted. No source, Requirements
Lab, or 2010 content changed; no tests, builds, or CI were run. The 2025
Federation Synchronization cross-review confirms that the guide separates
execution membership, point-specific participant/announced/achieved sets,
Save/Restore participants, and callback observation. It already links the
lifecycle Join/departure boundaries, Save/Restore's in-flight barrier, callback
response/event routing, and Federation MOM queries.

Added one process-specific caveat: connection loss projects synchronization
completion notifications through queue/push, but the lifecycle audit finds
that the same registry departure can plan active Save/Restore failures whose
notifications are not projected by the inspected process dispatcher. The
shared membership transition therefore does not imply the same process
notification behavior for each protocol; neither loss-during-active-protocol
route has focused process runtime coverage. No duplicate diagram was needed.
No source, Requirements Lab, or 2010 content changed; no tests, builds, or CI
were run. Next: cross-review the 2025 Federation MOM synchronization queries
against the Synchronization guide, keeping read-only point/status snapshots,
barrier transitions, and callback delivery as distinct observations.

2025 Federation MOM synchronization query cross-review (2026-11-03): confirmed
the paired requests are read-only and distinct from synchronization
registration/achievement/completion. Clarified that embedded report contents
are formed as request-time snapshots and delivered later through callbacks;
the focused test shows status changes across separate requests, not a state
change between one request and its callback. Added links to the broader
Synchronization guide and the cross-cutting state/observation guide. The
configured-process query path remains unverified. No source, Requirements Lab,
or 2010 content changed; no tests, builds, or CI were run. Next: cross-review
the 2025 Process Endpoint Request and Callback guide against the focused
process evidence in Attribute Value Update Requests and Callback/Service
Ordering, keeping service outcomes, server transitions, event transport, and
client callback entry distinct.

2025 Federation MOM request timing and navigation review (2026-11-03):
rechecked explicit requests for joined-federate MOM values against the member
MOM cadence, callback dispatch, subscription-state, and prior process-endpoint
reviews. Kept explicit current-value requests, event-triggered Conditional
updates, and `HLAsetTiming` periodic refresh as separate paths. Added direct
cross-links to the member-cadence and callback-dispatch explanations and to
the joined-federate MOM exception from the subscription-state comparison.
The synchronization-query flow now also states that reports carry request-
time snapshots and arrive later through callbacks; Mermaid CLI rendered the
updated chart and local screenshot review was clear. A cross-review of
Federation MOM report interactions confirmed the guide already separates the
synchronous service result, paired request-driven report, service-invocation
report, and MOM/public exception-report paths. Callback/Service Ordering and
Service Invocation Reporting reinforce those roles; no duplicate diagram was
needed. The process report route remains profile-bounded. No source,
Requirements Lab, or 2010 content changed; no tests, builds, or CI were run.

2025 Federation MOM report-recipient review (2026-11-03): clarified the two
recipient gates in the report timeline: the request plans current recipients,
then the queued callback rechecks membership and subscription before invoking
`receiveInteraction`. Report data remains fixed even when eligibility changes.
Focused federation-FOM and synchronization-points-list tests observe
unsubscribe-before-evoke suppression; status and member-scoped FOM cases do
not separately test that change. The member FOM path shares the callback
helper. The updated report diagram rendered and screenshot review was clear.
No source, Requirements Lab, or 2010 content changed; no tests, builds, or CI
were run. Next: cross-review member-scoped `HLArequestFOMmoduleData` against
Joined-Federate MOM's Join-supplied module list and the FOM Composition guide,
keeping federation module lists, member inputs, and effective Current FDD
separate.

2025 member-scoped FOM report cross-review (2026-11-03): confirmed the MIM's
inherited `HLAfederate` target parameter and index into that target's Join-
supplied module list remain distinct from the federation list and effective
Current FDD. The Joined-Federate MOM guide documents when that private member
snapshot is built; the FOM Composition guide documents how an additional-module
Join changes the effective model. Added direct links to both relevant Join
boundaries; the existing hierarchy and report diagrams were sufficient. No
source, Requirements Lab, or 2010 content changed; no tests, builds, or CI were
run. Next: review how an additional-module Join affects federation-list index
selection, keeping the input list and composed Current FDD separate and avoiding
concurrency claims that the focused tests do not establish.

2025 Federation MOM federation-index projection audit (2026-11-03): the MIM
defines the request indicator as an order number in the federation module list.
The embedded list projection filters qualifying 2025 content and de-duplicates
by validated module content, while the report planner indexes a separate vector
of retained FOM records de-duplicated by canonical source path. Those rules can
disagree for a module with no listed-content delta or identical content supplied
from different paths. The focused request case covers only index 0 of the
Create-supplied Restaurant module; the additional-module Join test observes a
changed `HLAcurrentFDD` without a later module-data request. Updated the
Federation MOM guide to separate the normative MIM contract from this
unverified embedded mapping and corrected its 2025 duplicate-rule explanation.
Existing diagrams were sufficient. No source, Requirements Lab, or 2010
content changed; no tests, builds, or CI were run. Next: audit the analogous
2025 member-scoped designator-list versus report-content index mapping, keeping
the target member, federation list, and effective Current FDD distinct.

2025 Federation MOM member-index projection audit (2026-11-03): the member MIM
defines the indicator as an order number in that federate's Join-supplied list.
The embedded list encoder filters qualifying validated content and de-duplicates
by content; its retained report snapshot includes every FOM record and
de-duplicates by canonical source path. The focused public report case requests
index 0 for a one-module Join. A private snapshot test checks one designator for
a repeated same-path record but does not compare the report payload; no focused
case verifies multi-module list-to-payload alignment. Added a separate member
boundary section to the Federation MOM guide and a cross-link from the
Joined-Federate MOM guide. No diagram was added. No source, Requirements Lab, or
2010 content changed; no tests, builds, or CI were run. Next: audit how target
member Resign and same-name rejoin affect the old member snapshot and any queued
member-scoped module-data report, without conflating it with federation reports
or `HLAcurrentFDD`.

Previous 2025 reader-gap and timing-diagram review (2026-11-03): rechecked the
passive cross-attribute discovery and Join/save-restore participant-set
boundaries. Existing guides already separate active discovery relevance from a
passive receiver's own match, and membership changes from protocol-specific
participant completion; their evidence and process/profile limits are explicit,
so no duplicate chart was needed. Rendered and screenshot-reviewed the existing
restore-source selection and FQR `HLA_EVOKED` late-arrival diagrams with Mermaid
CLI 12.0.0. Both are readable and unclipped. The FQR chart shows the embedded
callback-time extraction path; the process-coordinator policy remains in its
adjacent comparison table. A follow-up confirms the process NMR/NMRA late-arrival
case returns a retraction designator but never invokes Retract; the separate
embedded test starts with the TSO already queued. Added a cross-link clarifying
that the documented grant-reconsideration example is embedded-specific and
distinct from retargeting. The combined process outcome remains untested. Also
linked the Save/Restore failure discussion to the lifecycle guide's voluntary
embedded Resign sequence and source-only process notification boundary. No
tests, builds, or CI were run; GitHub rendering remains unverified. The
authorization guide now states that `configurationName` selects the RID
authorization profile (empty means `default`) and that `rtiAddress` and
`additionalSettings` do not select that profile. This is source-derived; its
named/default/missing-profile test source was not rerun. A targeted DDM
cross-review confirmed the guide matches the embedded test: a mixed receiver's
ordinary/default route remains eligible with or without its separate disjoint
regional declaration, while an empty `sendInteractionWithRegions` set is a
different no-send path. A separate Region-lifetime review confirmed the
embedded cases for passive-subscription holds and deferred object-removal
holds. Neither review resolves DDM §9.1.5 or establishes process parity; no
additional diagram was warranted.

The queued regional TSO interaction cross-audit found a useful profile
boundary. The embedded replacement test proves that a saved source-region
snapshot is checked against the receiver's current subscription when the TSO
reaches callback dispatch. The configured-process source also retains the
send-time source snapshot and re-resolves an already queued recipient before
emitting its event, but the process callback-gating test does not replace the
subscription while queued. Added this source-only process path and its test
limit to the DDM guide, with a reciprocal Time Management cross-link. No new
diagram or test run was needed.

The source-resignation review found a separate reader gap: an accepted explicit-
source regional interaction can remain queued after its publisher's live Region
is deleted, while the producer's retraction record becomes terminal. The focused
embedded case observes the later interaction before grant and a membership
failure on the departed publisher's `Retract` call. A new DDM sequence shows
these lifetimes, callback-scoped Region copies, and the independent time frontier;
the TSO-retraction and Save/Restore guides now link the terminal-record and
restored-live-record outcomes, with a reciprocal Time Management link for the
advance frontier. The configured-process path has analogous source
snapshots and callback-copy leases, but no focused process case resigns the
publisher while this regional interaction is pending. Mermaid CLI 12.0.0
rendered the new sequence and its local screenshot is readable without clipping.
No tests, builds, or CI were run.

The follow-up cross-audit of interaction-class ancestry found the same
separation already stated in the object/interaction guide's comparison map and
linked from the DDM guide: ordinary receipt may promote to a subscribed
ancestor and project parameters there; directed routing uses target-object
class ancestry for declaration/selector lookup while retaining the sent
interaction class in the surveyed callback path. The guides label the narrower
evidence and missing directed ancestor cases, so no duplicate chart was added.

Recent 2025 reader-gap review (2026-11-03): audited the callback guide's
`receiveInteraction` exception paths across embedded delivery and the process
endpoint. The guide distinguishes immediate service-stack callbacks, later
Evoke callbacks, request-backed process drains, and background dispatch. The
source-derived accepted/report/queue boundaries are explicit; no focused test
throws from the callback, so those outcomes remain implementation evidence.
No behavior claim or diagram content changed, and no test was rerun.

Recent 2025 documentation and visual review (2026-11-03): clarified that the
DDM send test supplies a mixed valid/unavailable-dimension set while the
subscription test supplies only the unavailable region; repaired their
focused-test and source anchors. The embedded Send Interaction exception flow
in the [callback guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md),
the [host/local forced-loss sequence](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#the-recipients-host-and-local-rti-react-independently),
the Enable Callbacks exception flow, and the process synchronization-point
callback-exception flow rendered with Mermaid CLI 12.0.0. Local screenshot
review found all four readable without clipping or overlap. GitHub presentation
remains unverified. The preceding DDM
interaction-routing correction records why a disjoint regional declaration
does not suppress the tested ordinary/default route; the exact normative
§9.1.5 interpretation and process parity remain open.

Recent 2025 documentation increment (2026-11-03): the directed-interaction
guide now includes the embedded §5.6 publication-service transition. It shows
handle and FOM-pair validation before the declaration update, the empty-set
no-op, and merging a non-empty set. Mermaid CLI 12.0.0 rendered the flow and
local screenshot review found its branches readable. The portable C++ TCK now
directly maps seven public outcomes across §§5.1, 5.6, 5.6.4, and 5.7,
including send rejection before publication, delivery after publication,
empty-set publication/unpublication behavior, and set/whole-class withdrawal.
The primary scenario passed under both callback models (2/2 focused CTests).
Failure atomicity for invalid multi-pair input is not tested; the embedded
map-copy/swap mechanics remain source-derived. No normative transaction
guarantee or process/2010 parity is claimed.

Recent 2025 documentation increment (2026-11-03): the [subscription-state guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#whole-class-and-selected-subset-unsubscribe-have-different-observed-effects)
now contrasts selective and whole-class unsubscription with separate public
C++ TCK fixtures. The subset case removes `Value`, keeps `Name` flowing across
two existing objects, and leaves the peer's full subscription intact. The
whole-class case observes no later reflection for its existing object and no
discovery for a later registration. Both recorded installed-package runs passed
under `HLA_EVOKED` and `HLA_IMMEDIATE`. The empty-set subset meaning remains
unresolved; neither test asserts an `attributesOutOfScope` advisory or
independent-provider conformance. Mermaid CLI 12.0.0 rendered the selective
sequence and comparison flowchart; local screenshot review found both readable
without clipping or overlap. The selected visual queue now has 67 reviewed
charts with none pending; GitHub rendering remains unverified. No test, build, or
CI was run during this documentation increment.

Recent 2025 reader-gap increment (2026-11-03): the [object-and-interaction guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#the-same-sent-class-at-three-subscription-depths)
now illustrates closest-class selection and inherited-parameter projection in
one ordinary receive-order example. As the receiver adds Base, Middle, then
Grandchild subscriptions, the same sent Grandchild interaction is reported at
the closest subscribed class; the parameter map contains only the parameters
defined at that class or its ancestors, with exactly one callback per send.
The recorded installed-package C++ TCK result passes under both callback
models. This resolves the specific test gap left by separate internal
projection and empty-map embedded receipt cases; it does not establish
independent-provider interoperability, directed/TSO behavior, or the licensed
clause's normative interpretation. Mermaid CLI 12.0.0 rendered the sequence and
local screenshot review found it readable without clipping or overlap. The
selected visual queue is now 68 reviewed charts with none pending; GitHub
rendering remains unverified. No test, build, or CI was run during this docs
increment.

Recent 2025 reader-gap review (2026-11-03): the ordinary interaction guide now
connects federation-wide active relevance with publisher-facing
`turnInteractionsOn`/`turnInteractionsOff` declaration advisories. It separates
those class-level callbacks from delivery of any particular `receiveInteraction`
and from receiver-specific regional overlap. The focused embedded declaration
test observes the active/passive advisory transitions. The existing interaction
flow and relevance-advisory state chart already explain the two paths, so no
duplicate diagram was added. Configured-process behavior and full normative
conditions remain unverified; no tests, build, or CI were run.

Recent 2025 receiver-Resign review (2026-11-03): the TSO guide now distinguishes
the embedded recipient gate from the configured-process event/acknowledgement
boundary and adds a source-derived flowchart. The registry drops the departed
recipient's temporal queue bookkeeping and rejects a later gate, while its
separate retraction record can retain a pending entry that no longer pins
payload. The process Resign cleanup clears joined identity and retraction-route
state but does not erase pending interaction Region-copy leases; a post-Resign
acknowledgement fails its membership/session check, while successful producer
Retract or session detach can erase the lease. Existing focused tests do not
cover receiver Resign between process event dispatch and acknowledgement.
Mermaid CLI 12.0.0 rendered the new chart and its
local screenshot review found no clipping or overlap. The selected visual queue
now has 72 reviewed charts with none pending. No tests, builds, or CI were run.

Recent 2025 visual review (2026-11-03): the 2025 Attribute Update-Rate guide's
inventory said its fourth, subscription-generation chart still needed visual
review, despite the selected queue reporting none pending. Mermaid CLI 12.0.0
rendered the chart; local screenshot review found its top-down branches legible
while scrolling. Reconciled the guide inventory and raised the selected review
count to 73. No tests, builds, or CI were run.

Completed the bounded 2025 object-instance name reservation review below. The
existing diagrams already distinguish admissible single-name outcomes,
partial-success bulk reservation, all-or-nothing bulk release, registration
commit, and the separate peer-discovery path; no diagram change was needed.
Clarified that whole-set validation before reservation mutation is source
evidence: existing tests assert synchronous invalid-input exceptions but do
not retry a valid member to check for side effects, and the process-endpoint
case does not submit an invalid bulk set. Keep DDM §9.1.5 and other
standards-text questions open until authorized text is available.

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

The final three charts in the selected queue passed local screenshot review on
2026-11-03:

- The [composite shape, cursor, and padding chart](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md#composite-values-shape-cursor-and-alignment)
  shows fixed-record and array layout, the variable-array count prefix, and the
  distinction between inter-child padding and the final cursor.
- The [factory-selected logical-time sequence](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md#logical-time-is-factory-defined-not-assumed-to-be-be-or-le)
  separates factory-owned values and opaque encoded bytes from the wrapper's
  copy and cursor behavior.
- The [configured-process regional-send reporting chart](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#process-regional-sends-separate-public-preflight-from-server-rejection)
  keeps receive-order and timestamped reporting paths distinct, including the
  reviewed evidence limits.

All three renders were readable without clipping or overlap. The Service
Invocation Reporting review-ledger reconciliation remains closed; the local
render does not change the guide's stated test and implementation boundaries.

The bounded 2025 Auto Provide reader-gap review found that a configured-process
discovery chart added on 2026-10-21 was outside the guide's older three-chart
visual-review note. That chart was rendered and screenshot-inspected locally;
its branches remain readable without clipping. The guide already distinguishes
initial reflection, provider solicitation, and explicit requests, and records
the source/test limits on the process path, so no additional behavior chart was
warranted. At that checkpoint the selected queue had 27 reviewed charts and
none pending; the later TSO Retraction review raised it to 40.
The Service Invocation Reporting guide has eleven diagrams. Its prior
chart-status reconciliation remains closed; the configured-process
regional-send chart was the final unrendered chart in the selected queue and
passed local screenshot review on 2026-11-03. This closes the selected queue,
not the guide's whole visual review.

Prior visual-QA checkpoint (2026-11-03): The cross-federate pending-TAR chart in the
[time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md) was rendered and
screenshot-reviewed locally. The first image exposed a long note overflowing
its box; after splitting the note across two lines, the saved chart rendered
without clipping or overlap. The restore-time state versus live callback
queues chart was also rendered and inspected; despite its height, the separate
state branches and evidence limits remain readable without clipping. This
checkpoint left 23 charts reviewed and three pending; the final three reviews
above brought the selected queue to 26 reviewed and none pending. Local
screenshots do not establish GitHub rendering.

## Progress history

Detailed later review increments are indexed in the [2025 documentation progress history](HLA-2025-DOCUMENTATION-PROGRESS-HISTORY.md). Earlier topic-audit notes for increments 1–80 remain in the [early history archive](HLA-BEHAVIOR-FLOW-GUIDES-ROADMAP-HISTORY-2026-11-07.md). The active roadmap keeps the current baseline, actionable backlog, and API-to-guide coverage map.
