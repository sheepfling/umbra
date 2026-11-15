# IEEE 1516.1-2025 Documentation Roadmap

This roadmap tracks the remaining work to make Umbra's 2025 HLA discoveries
clear and durable for readers. The guides lead with HLA concepts; implementation
and focused-test evidence is included only to explain an observable boundary or
to show what the current evidence does—and does not—establish.

## Current finish gates and next handoff — 2026-11-15 (increment 541)

**Overall completion: approximately 95% (working estimate, not a gate-count calculation).** The 2025 set has 32 guides and 515 diagrams. All diagrams are published and revision-aware GitHub hydration covers 515/515. Increment 537 added a selected published-layout review of DSE #003 in GitHub’s expanded Mermaid dialog: its route lanes and outcomes are legible without clipping, though the tall chart requires vertical scrolling. This is a representative sample, not an all-diagram layout claim. The body-matched visual ledger retains local screenshots for all 515 diagrams.

Increment 538 separated dated reviews from the living [behavior backlog](HLA-BEHAVIOR-FLOW-GUIDES.md) into the [historical archive](HLA-BEHAVIOR-FLOW-GUIDES-HISTORY.md). The files are 244,539 and 229,965 bytes, respectively, and both passed authenticated GitHub GFM conversion. The index’s three diagrams converted 3/3; the archive is prose-only. Its 2010 notes retain explicit edition labels and do not describe 2025 behavior.

**Citation status rechecked at increment 541:**

- Current published candidate HEAD `5f35d503d225def9dab2f97d46910551bf4439e8` resolves 4,796/4,805 source-line references. Nine are out of range in four 2025 source/test files. Its source tree is unchanged from source-bearing revision `25b9055f9424fe73a391f758c2a53c3e35f15d0d`; the intervening commits are documentation-only, and the remote candidate branch matches this revision.
- The candidate worktree resolves all 4,805 references. Increment 541 rechecked the exact cited windows: `serveConnection`, the timeout parser, five Catch2 cases, the Federate-Lost MOM scenario, and its post-loss update. Their source/test edits remain uncommitted.
- Remote `origin/feature/initial-work` at `c9c5a9e410539b039cd8bba35b53d0ef2b24d180` also resolves 4,796/4,805.
- Local `feature/initial-work` at `6265bca881f23b995f7eef76f89f9d3af3e90d6f` passes line bounds only. `tck_connection_lifecycle.cpp#L2389` is an automatic delete-then-divest assertion and `#L2765` is a fault-parameter lookup; the Federate-Lost scenario’s post-loss assertion is at `#L2983`. The local branch is 19 commits ahead of its remote and is not a safe citation target.
- The current GitHub query found no open pull requests. The [source-dependency audit](HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json) records the refreshed revisions and all nine pending targets. Do not retarget citations to unrelated code to make line checks pass.

1. **Stabilize the nine source citations.** Have the source owners publish the reviewed 2025 source/test edits on a suitable revision. Then rerun all 4,805 checks and verify each target’s meaning against that exact revision.
2. **Keep evidence limits explicit.** Nonregional receive-order push still lacks focused DSE test evidence. The DSE #003 review remains one selected hosted sample; full-width hosted layout review is not claimed for all 515 diagrams.
3. **Close out the handoff.** After source stabilization, refresh the source-dependency and render audits. No C++ build, tests, or CI are needed for this documentation closeout. Keep 2010 and the Requirements Lab separate.

**Next handoff:** have source owners publish the reviewed 2025 source/test edits without importing unrelated primary-branch work. Verify the nine line and semantic targets at that revision, then refresh the dependency and render audits. The qualitative completion estimate remains 95% until that citation closeout is complete.

## Current baseline

As of 2026-11-15, the 2025 topic set contains 32 guides (the timing guide and
31 other behavior areas) with 515 Mermaid diagrams: 304 flowcharts, 186
sequences, and 25 state diagrams. All seven added diagrams and 21 revised
bodies are published on the candidate branch. The hosted hydration audit covers
all 32 current pages and all 515 diagrams: 385 diagrams on 27 unchanged pages
carry forward from the earlier full-page audit, and 130 diagrams on the five
changed pages were checked at revision 4311528. This is a combined revision-aware
audit, not a claim that every page was reloaded in one browser pass.

The [archived five-needs review](HLA-BEHAVIOR-FLOW-GUIDES-ROADMAP-HISTORY-2026-11-07.md)
reached 32/32 guides: standard/API vocabulary, state transitions and callbacks,
important edge cases, source and focused-test evidence with limits, and open
questions. The thematic reading paths link all 32 guides directly. There are 69
selected GitHub visual reviews across all 32 guides; these samples do not
constitute a full-width readability review of every hosted chart.

The source-link checker covers 4,805 unique references across 41 Markdown
pages. All resolve in the current worktree and at primary commit `063e5eae`; the
published candidate HEAD 9e1bc4fe resolves 4,796, with nine out-of-range anchors in four
active 2025 source/test files. Those exact targets remain pending publication
and semantic recheck; see the [source-dependency audit](HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json).

Increment 425 rechecked the 39-file source-link set: the current worktree still
resolves all 4,765 references, while candidate HEAD resolves 4,756 and misses
nine anchors in four in-flight implementation/test files. A diff-only boundary
screen of the nine modified 2025 topic guides found no newly added `2010`,
`1516e`, `IEEE 1516-2000`, or `HLA 1.3` references. This is a regression screen
for those edits, not a semantic audit of every guide. The 90% completion figure
remains a working estimate.

Increment 426 clarified the FOM Admission chart's Big/Little Create/Join node to
say “Primary-only.” The primary result artifact contains four passing rows for
the scenario and contract twin under `installed-package`, but has no source
revision or executable identity; its runner and two fixtures are absent from
this candidate. Mermaid CLI 12.0.0 rendering and screenshot review passed, and
the guide converted through GitHub GFM with 13/13 Mermaid containers. A full
current-source check also reconciled the stale ledger hash for Callback and
Service Ordering #019: all 515 guide fences now match 515 ledger entries. This
does not extend hosted hydration beyond the last 508-diagram audit.

Increment 427 corrected the missing-name `getObjectClassHandle` coverage note.
A tracked, target-listed embedded Catch2 case asserts `NameNotFound` and exact
failed/successful file-report records. No run result was found, and the
configured-process missing-name path still lacks a matching test/result. The
guide now states those boundaries explicitly; no C++ build or test was run.

Increment 428 found a target-listed configured-process matrix case for the
same unknown-name lookup. It asserts `NameNotFound` and a failed MOM
interaction report after explicit observer evocation; no run artifact was
located. The process file-sink path remains untested. The lookup diagram now
shows both test-source paths and labels the remaining sink gap. Mermaid CLI
12.0.0 screenshot review and authenticated GitHub GFM conversion passed for
the updated guide (17/17 containers, 20 headings). A full ledger check
reconciled ten shifted Service Invocation source lines and the stale TSO
Retraction #008 hash: all 515 diagrams now match their recorded line and body
hash. The worktree source-link check resolves 4,776/4,776; candidate HEAD
resolves 4,767/4,776, with nine anchors in the same four in-flight files.

The current overall estimate is **approximately 95% complete**, a working
estimate rather than a fraction derived from gate count. The 515-diagram set
and its seven additions/21 revised bodies are published; later documentation
commits changed no Mermaid bodies. Hosted hydration remains verified for all
515. Both guide indexes now link all 32 current guides, and every guide has at
least one Mermaid diagram. The remaining closeout is source traceability: nine
references resolve in the current worktree and at local primary HEAD 063e5eae,
but not at the candidate source revision checked at 9e1bc4fe. The primary
checkout is eleven commits ahead of remote initial-work and includes unrelated
Requirements Lab changes, so do not use it as a docs publication route. After
source owners stabilize the cited changes, rerun path and semantic checks. No
C++ builds, tests, or CI ran for this documentation work; keep 2010 and the
Requirements Lab separate.

Increment 440 reviewed Object and Interaction Information Flow diagram #020,
“The delivery route carries that captured view to the callback,” in GitHub's
full-width dialog. The six participant lanes show Pull (`HLA_EVOKED`, push
disabled) and Push (`HLA_IMMEDIATE`, push enabled) delivery carrying the same
captured interaction view to callback dispatch; the timing caveat remains
explicit. Its body hash matches the published source at branch head
`e10fa19a7bf48415e00a14ca7301613a84f637a3`. This is another targeted sample,
not full hosted hydration; the 91% working estimate and remaining closure gates
are unchanged.

Increment 441 reviewed Authorization Flow diagram #001, “Authorization is a
gate, not the service result,” in GitHub's full-width dialog. Its three
callback scopes, outside-interface branch, authorization outcomes, and
service-specific gate after AUTHORIZED were readable. The body hash matches
the published file at the candidate branch head. This is another targeted
sample; the 91% estimate and remaining closure gates are unchanged.

Increment 442 reviewed Attribute Ownership diagram #002, “Ownership transfer and update-region association are separate boundaries,” in GitHub's full-width dialog. The selected embedded 2025 flow's owner, prospective-owner, and regional-receiver lanes were readable, including deferred R-B, the cancellation branch, transfer, and R-B-versus-default-region outcomes. The raw Mermaid body hash matches the published source at the candidate branch head. This adds a targeted sample; the 91% estimate and remaining closure gates are unchanged.

Increment 443 reviewed Callback and Service Ordering diagram #025, “Process endpoint: service responses and synchronization events are separate,” in GitHub's full-width dialog. The five process lanes and separate registration/achievement paths were readable while scrolling, including pushed versus polled outcomes, the pending-barrier case, callback-model dispatch, and separate service-report requests. This is implementation-profile evidence, not a normative total-order rule. The 91% estimate and remaining closure gates are unchanged.

Increment 444 rechecked all 515 current Mermaid bodies against the visual-review ledger. It found stale ledger hashes for Callback and Service Ordering #019 and TSO Retraction #008; both were corrected after Mermaid CLI 12.0.0 rendering and screenshot review. The other 513 entries already matched. Fresh source-link checks resolve 4,776/4,776 in the worktree and 4,767/4,776 at candidate HEAD, primary local HEAD, and origin/feature/initial-work; the same nine anchors in four files remain. The 91% estimate and hosted blockers are unchanged.

Increment 445 confirmed that GitHub renders Object-Instance Name Reservation
#001 inline on the published 2025 branch. The visible screenshot covered its
request and admissibility decision, with the remaining branches below the
viewport; no full-chart readability or dialog review is claimed. This targeted
sample leaves the 91% estimate and remaining closure gates unchanged.

Increment 446 opened the same published chart in GitHub's full-width dialog and
reviewed it while scrolling. All branches—from synchronous admission through
reservation outcome, named registration, owner handle, and peer discovery—were
readable. The candidate HEAD Mermaid-body hash matches the reviewed chart. The
worktree source-link check passes at 4,776/4,776; HEAD still resolves 4,767/4,776
because nine anchors point into four in-flight source/test files. This targeted
readability review leaves the 91% estimate and publication gate unchanged.

Increment 447 refreshed authenticated GitHub GFM conversion for all 39 current
2025 Markdown pages: the 32 topic guides and seven supporting pages returned
nonempty responses. All 515 Mermaid fences produced 515
`section[data-type="mermaid"]` containers, with 1,082 headings, 8,371 rendered
links, and zero API failures or count mismatches. This confirms GFM parsing and
container registration, not hosted-page hydration or link click-through. The
candidate remains unpublished; the nine HEAD-only source anchors and evidence
gaps remain. The 91% working estimate is unchanged.

Increment 448 reviewed Federation and Federate Lifecycle #022, “A resignation
during an active save fails the save for survivors,” in GitHub’s full-width
Mermaid dialog. The four participant lanes and entire sequence remained
readable, including the in-callback Resign rejection, retry after callback
return, failure notification to surviving members, suppression of a stale
callback to the departing member, and a later successful save. The raw Mermaid
body SHA-256 `f5991e78a4bd7e01ffd86de16f337b7d37bd0e6c0374dfc7411376f8c0969537`
matches the hosted candidate branch and local review ledger. This is one focused
embedded 2025 `HLA_EVOKED` schedule, not a global callback-order rule. It adds a
targeted readability sample only; the hosted full-hydration baseline remains
508/508, the 515-diagram candidate and nine source anchors remain open, and the
91% working estimate is unchanged. No C++ build, tests, or CI ran; keep 2010
and Requirements Lab excluded.

Increment 449 reviewed Callback and Service Ordering #005, “A pushed
interaction can arrive during a single Evoke wait,” in GitHub’s full-width
Mermaid dialog. Its six participant lanes and complete sequence remained
readable while scrolling, including the pre-wait poll, event buffered during
the wait, receive-order gate, conditional post-wait drain, dispatch-lease
contention, and one-task follow-up. Raw body SHA-256
`b2bc3c4e9e936502da89eb13ff39f3f5140acdd1a0071f12eb103ec62afbfe17` matches
the hosted candidate branch and local review ledger. This is selected 2025
process-profile behavior, not an HLA-wide callback-order rule. The targeted
sample does not change the 91% estimate or close publication, source-link, or
full-hydration gates. No C++ build, tests, or CI ran; keep 2010 and Requirements
Lab excluded.

Increment 450 reviewed Federation Save/Restore diagram #028, “Worked case: queued timestamped object deletion returns after fresh-registry restore,” in GitHub’s full-width dialog. Its four participant lanes and entire sequence remained readable through the time-7 save, fresh-registry restore, pending deletion rebind, and later time-9 removal callbacks. The hosted body SHA-256 `c7ccde58319883d8c216e3c4e9898399a3c6b1589db6ad962692d0eaf4313ad9` matches the candidate branch and local visual ledger. This is an embedded 2025 development-profile chart, not process-restart evidence or a general callback-order rule. The 508/508 hosted baseline, unpublished 515-diagram candidate, nine source anchors, and evidence gaps remain open; the 91% working estimate is unchanged. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 451 (2026-11-11) reviewed Process Endpoint Request and Callback diagram #020, “FQR publishes FQG without waiting for the TSO completion ACK,” in GitHub’s full-width Mermaid dialog. The five-lane sequence remained readable across the recipient inTransit state, actual and optimistic grant commit, TSO-before-FQG stream order, HLA_EVOKED and HLA_IMMEDIATE callback branches, callback-throw paths, and Resign/ConnectionLost cleanup. The body SHA-256 d9b9247fddff374a3578bd9cb8023a482f075b1d0107d26c90fdcc083f8849cc matches candidate branch e10fa19a7bf48415e00a14ca7301613a84f637a3 and the visual ledger. The callback-throw interleavings are explicitly source-derived, not focused tested behavior. This is a targeted 2025 process-profile readability sample only; the older 508/508 hosted hydration baseline, unpublished 515-chart candidate, nine source anchors, and documented evidence gaps remain. Overall estimate stays 91%. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 452 (2026-11-11) reviewed Object-Instance Name Reservation diagram #002, “1. Single-name reservation: synchronous admission, callback outcome,” in GitHub’s full-width Mermaid dialog. All five participant lanes and the full sequence remained readable while scrolling, with no clipping or overlap observed. The trace separates synchronous rejection from callback outcomes and HLA_IMMEDIATE from HLA_EVOKED delivery; the release-before-callback branch is explicitly an embedded-source observation. The raw Mermaid body SHA-256 2fa7627f93dae3a69c12444e571462397445031300cf931a06854f4646c1a11c matches candidate branch e10fa19a7bf48415e00a14ca7301613a84f637a3 and the visual ledger. This targeted 2025 readability sample does not change the 91% estimate or the remaining source-link, publication, full-hosted-hydration, and evidence-gap gates. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 453 reviewed Callback and Service Ordering #016, “Background dispatch runs without an active request,” in GitHub’s full-width Mermaid dialog. All seven participant lanes and the three exception paths remained readable while scrolling, with no clipping or overlap. The chart distinguishes a dispatchLoop-owned HLA_IMMEDIATE callback exception saved for rethrow before the next request lock/send, an exception exiting another thread’s active immediate-drainer call path, and an HLA_EVOKED exception exiting Evoke Callback. These are source-derived 2025 process behaviors, not normative HLA callback-order rules. Raw Mermaid body SHA-256 98e06f655801be90b2efcc72cbfcf871e74882534718cd868415a2d6a001df52 matches candidate branch e10fa19a7bf48415e00a14ca7301613a84f637a3 and the visual ledger. This targeted sample leaves the 91% estimate and source-link, publication, hosted-hydration, and evidence-gap gates unchanged. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 454 reviewed Federation Save/Restore diagram #006, “Save flow:
request, participant work, and the commit barrier,” in GitHub’s full-width
Mermaid dialog. The six participant lanes and both upper and lower branches
remained readable while panning, without clipping or overlap. The raw body
SHA-256 e0e496cf9b5b66e9409026b69c25c350788ada4294229aa1a8362f8d138d4a5d
matches the hosted candidate source and visual ledger. It distinguishes
accepted request from initiation and participant-local checkpoint completion
from RTI image commit; this is a targeted implementation-profile sample. The
91% estimate and source-link, publication, full-hosted-hydration, and
evidence-gap gates remain unchanged. No C++ build, tests, or CI ran; keep 2010
and Requirements Lab excluded.


Increment 455 reviewed Exception Reporting diagram #005, “When both process
report routes apply,” in GitHub’s full-width dialog. Its six lanes and upper
and lower report and failure branches remained readable while panning, with no
clipping or lane overlap. The raw body SHA-256
3df0edafa7680ebfa709247091925b557dbcb93df5a05431ea20947a07cf6356 matches the
hosted candidate source and visual ledger. The combined two-switch scenario
remains source-derived and untested. This visual sample leaves the 91% estimate
and source-link, publication, full-hosted-hydration, and evidence-gap gates
unchanged. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab
excluded.

Increment 456 reviewed Data Distribution and Regions diagram #006, “Inherited dimensions in a child-class regional path,” in GitHub’s full-width dialog. The vertical hierarchy-to-subscription flow remained readable across upper and lower branches while scrolling, with no clipping or overlap. The query result is observed; the focused test fails at subscription before registration or callbacks, and the overlap and callback branches remain authored expectations. The guide does not claim parent-keyed subscription behavior for a child-registered object. The body hash matches the hosted candidate and local visual ledger. This sample leaves the 91% estimate and source-link, publication, full-hosted-hydration, and evidence-gap gates unchanged. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

All 515 diagram identities have local render and screenshot evidence. Increment
374 refreshed the published candidate branch at
revision 270f302536ddb2f2bd56a35b565d2251a45f03e2: all 32/32 guide pages
loaded, and every one of the 508/508 hosted frames contained a rendered SVG.
All render responses returned HTTP 200, with zero request failures. Two
callback-ordering frames needed an additional wait and then rendered
successfully. The [visual ledger](HLA-2025-DIAGRAM-VISUAL-REVIEW-LEDGER.json)
and [GitHub render audit](HLA-2025-GITHUB-RENDER-AUDIT.json) hold the identity
and hosted evidence; older increment-356 and increment-360 results remain as
historical records.

At increment 389, an exact comparison found 507 of 508 hosted chart bodies
unchanged; Federation Synchronization #006 was revised and Save/Restore #043
was added. Later changes added FOM Admission #001 and Service Invocation
Reporting #006 and #007, revised Data Distribution #002, and added Service
Invocation Reporting #008, #009, and #011. The current 515-chart set therefore
has 506 bodies unchanged from the last hosted set, two revised bodies, and
seven additions. All nine current chart deltas have local render and screenshot
evidence. Increment 423's fresh authenticated GitHub Markdown API pass covered
all 32 topic guides: all 515 source Mermaid fences produced 515 Mermaid
containers, with 814 headings and 7,919 links and no API failures or container
mismatches. This verifies GFM conversion and container registration, not visible
hosted hydration or link click-through. The six supporting Markdown pages were
not part of this refresh. Hosted page hydration still needs refresh after
publication. Increment 424 inspected the current remote candidate branch head
(e10fa19a7bf48415e00a14ca7301613a84f637a3) in Microsoft Edge. The Time
Management guide's Role state machines charts #002 and #003 visibly rendered;
#002 opened as a full-width diagram, and both body hashes match the committed
branch source. This is a two-chart sample from one section, not a full 508-page
audit. The inline viewer controls partly overlap #003's right edge, so wide-chart
readability should be checked in the dialog during the full pass. The old
headless Chromium placeholder result remains historical and does not describe
this current Edge sample. The updated roadmap and both progress-history pages
also returned nonempty HTTP 200 GFM responses, with zero Mermaid-count
mismatches (25 headings and 141 links combined). The increment-420
missing-class lookup chart is #008; later Service Invocation Reporting chart
ordinals shifted by one. Inserting the FOM Admission chart as #001 also shifts
its guide's 12 existing chart ordinals. Callback/Ordering #019 and
TSO-Retraction #008 already match the hosted audit.

Increment 417 (2026-11-11) added an object-attribute declaration report flow to
the Service Invocation Reporting guide. Its nine-record table distinguishes
five rejected calls from four successful declaration calls, including the
empty-set versus omitted optional argument, the public `active` flag versus the
MOM passive indicator, and typed Null returns. It bounds embedded callback
ordering separately from configured-process behavior. The source comment and
README coverage mismatch were identified; the package README now includes the
two-attribute publication record. Mermaid CLI 12.0.0 rendered the new chart and
the screenshot review found it readable. GitHub's Markdown API returned HTTP
200 for the full guide with 15/15 Mermaid containers, 19 headings, and 174
links. The 2025 link checker resolves 4,741/4,741 worktree references; candidate
HEAD still has the same nine unrelated source anchors unresolved. No C++ build
or test was run; 2010 and Requirements Lab remain out of scope.

Increment 418 (2026-11-11) added a synchronization-report flow to the Service
Invocation Reporting guide. Its seven-record serial stream distinguishes a
normally returned registration call from the later callback result: the
duplicate-label registration callback reports failure while the service report
still records invocation success and an empty exception. The guide bounds
callback/report ordering claims and keeps embedded and configured-process
source ordering separate. Mermaid CLI 12.0.0 rendered and screenshot review
confirmed the two-track chart. GitHub's Markdown API returned HTTP 200 for the
complete guide with 16/16 Mermaid containers, 20 headings, and 189 links. The
39-file source-link check resolves 4,752/4,752 worktree references; candidate
HEAD, primary local HEAD, and origin/feature/initial-work each resolve 4,743,
with the same nine anchors in four in-flight source/test files unresolved.
The ledger now contains 514 locally rendered and screenshot-reviewed charts;
eight chart changes still need hosted hydration after publication. The working
estimate remains 90% complete. No C++ build or test was run; 2010 and
Requirements Lab remain out of scope.

Increment 419 (2026-11-11) clarified a nearby lookup-report coverage boundary.
The 2025 API lists `NameNotFound` for `getObjectClassHandle`, but the cited
object/attribute/update-rate scenario reports `ObjectInstanceNotKnown` for an
invalid object-instance handle and later checks only a successful class-name
lookup at serial 7. The guide now states that the missing-class-name failure
and its failed service report remain untested. GitHub's Markdown API returned
HTTP 200 for the updated Service Invocation Reporting guide with 16/16 Mermaid
containers, 20 headings, and 194 links. The 39-file source-link check resolves
4,757/4,757 worktree references; candidate HEAD, primary local HEAD, and
origin/feature/initial-work each resolve 4,748, with the same nine anchors in
four in-flight source/test files unresolved. No diagram changed; the ledger
remains 514/514 locally rendered and screenshot-reviewed. The working estimate
remains 90%. No C++ build or test was run; keep 2010 and Requirements Lab
excluded.

Increment 420 (2026-11-11) added a source-backed flowchart for the 2025
getObjectClassHandle missing-name path. It follows the distinct embedded
registry-miss and configured-process rejection paths to NameNotFound, shows the
separate Exception-report attempt and failed Service Invocation report route,
and does not claim report delivery or callback order. Mermaid CLI 12.0.0
rendered the chart, and screenshot review found both paths readable. GitHub's
Markdown API returned HTTP 200 for the complete guide with 17/17 Mermaid
containers, 20 headings, and 202 links. The 39-file source-link check resolves
4,765/4,765 worktree references; candidate HEAD, primary local HEAD, and
origin/feature/initial-work each resolve 4,756, leaving nine anchors in four
in-flight source/test files. The ledger now contains 515 locally rendered and
screenshot-reviewed diagrams; nine chart deltas need hosted hydration after
publication. The working estimate remains 90% complete. No C++ build or test
was run; keep 2010 and Requirements Lab excluded.

Increment 421 (2026-11-11) linked the missing-class-name lookup flow from
the 2025 thematic reading path for service reports. The direct guide-section
link now leads readers from the report-family distinction to the tested
interaction/parameter failure trace and the distinct embedded/configured-process
class-name lookup paths; the latter's failed-report case remains an explicit
test gap. GitHub Markdown API conversion returned HTTP 200 for the thematic
paths with two headings and 168 links. The Service Invocation Reporting guide
still converts with 17/17 Mermaid containers. The 39-file source-link check
remains 4,765/4,765 in the worktree. No diagram, C++ source, or test changed.
The working estimate remains 90%; keep 2010 and Requirements Lab excluded.

Increment 422 (2026-11-11) audited direct guide coverage in the 2025 thematic
reading paths against the 32 guide sources in the visual ledger. All 32 are
explicit local Markdown targets; no guide is missing. This verifies guide
discoverability only, not section-anchor correctness or HLA content coverage.
The source-link checker still resolves 4,765/4,765 references in the worktree.
No diagram, C++ source, or test changed. The working estimate remains 90%; keep
2010 and Requirements Lab excluded.

Increment 423 (2026-11-11) completed a fresh authenticated GitHub GFM pass over
all 32 current 2025 topic guides. All 32 returned nonempty HTTP 200 HTML, and
all 515 source Mermaid fences matched 515 actual Mermaid containers; the pages
contained 814 headings and 7,919 links, with no conversion or count failures.
The six supporting Markdown pages were not refreshed. The source-link checker
resolves 4,765/4,765 references in the worktree and 4,756/4,765 at candidate
HEAD, with the same nine anchors in four in-flight source/test files. The GFM
check does not verify hosted-page hydration or link click-through. No source,
test, or diagram changed. The working estimate remains 90%; keep 2010 and
Requirements Lab excluded.

Increment 424 (2026-11-11) inspected the current published candidate branch
head (e10fa19a7bf48415e00a14ca7301613a84f637a3) in Microsoft Edge. The Time
Management guide's Role state machines diagrams #002 and #003 visibly rendered
in GitHub Preview; #002 opened in a full-width diagram dialog. Both remote
bodies match the visual-ledger hashes. The inline viewer controls partly
overlap #003's right edge, so the full audit should inspect wide diagrams in
the dialog. This one-section/two-diagram sample does not verify all 508 charts
on the published branch or the 515-diagram worktree. The previous headless
Chromium placeholder finding remains a historical result from a different
browser path. The updated roadmap and both progress-history pages also
returned nonempty HTTP 200 GFM responses, with zero Mermaid-count mismatches
(25 headings and 141 links combined). Source links still resolve 4,765/4,765
in the worktree and 4,756/4,765 at candidate HEAD. No source, test, or diagram
changed. The working estimate remains 90%; keep 2010 and Requirements Lab
excluded.

Increment 375 (2026-11-09) reconciled the roadmap's next handoff and current
hosted-render summary with the active roadmap index and increment 374 evidence.
Increment 373 had already completed the configured-process host-outage review;
the next review is configured-process `ConnectionLost`, after the current
host/client recovery and process-lifecycle changes are committed and have a
recorded focused run. A bounded scan of all 2025 guides found that every 2010
mention explicitly marks the edition boundary or links to its separate
reference-RTI guide; no 1516e implementation path was cited. No guide content
changed.

Increment 376 (2026-11-09) checked the current inventory of 32 2025 guide files
against the archived final five-needs audit entry (32/32) and confirmed that
the audit's five criteria are recorded there. This verifies the review-pass
denominator and scope; it does not close the separate semantic and evidence
backlog. No guide change was needed.

Increment 377 (2026-11-09) rechecked official no-cost access routes for the
2025 standard. The [product page](https://standards.ieee.org/ieee/1516.1/6688/)
offers Purchase and Access via Subscription; the [IEEE GET Program's current
list](https://standards.ieee.org/products-programs/ieee-get-program/) does not
list HLA. The public [downloads bundle](https://standards.ieee.org/wp-content/uploads/2025/08/1516.1-2025_downloads.zip)
contains API/support material, not the normative standard. No no-cost full-text
route was found. Updated the active handoff index's access-route record;
§6.21 and other normative questions remain open, with no behavior inferred.

Increment 378 (2026-11-09) linked the current GET Program catalog finding from
the RAVU guide's §6.21 evidence boundary. It now distinguishes the unknown
Reading Room eligibility from the GET catalog's omission of HLA; requester
entitlement and callback timing remain unresolved. The worktree checker
resolved 4,657/4,657 references across 38 2025 Markdown files. No diagram,
source, test, build, or CI changes; no 2010 or Requirements Lab edits.

Increment 379 (2026-11-09) rechecked the object-instance name reservation
guide against the clean 2025 embedded registry implementation and existing
focused tests. The registry keeps bulk reservation partial per name and
validates a complete release set before erasing any reservation. The embedded
tests exercise partial reservation and confirm a mixed-set release failure
leaves the owned names reserved; the process test covers callback outcomes but
not mixed-set release atomicity. The guide already states this evidence limit,
so no guide edit was needed. The worktree checker resolved 4,657/4,657
references across 38 Markdown files. No builds, test reruns, CI, 2010, or
Requirements Lab work. Overall progress remains 80%; formal gates remain 4/4.

Increment 380 (2026-11-09) added a compact source-state flowchart to the
Save/Restore guide. It separates a pending timed request from checkpoint
admission, timed-request replacement, untimed-request takeover, and opening
`SaveOperation`. The chart labels the unreachable-time failure outcome as
unresolved and makes no normative claim. Mermaid CLI rendered the updated
diagram and its screenshot was visually checked. The 509-entry diagram ledger
was reconciled against current sources: 25 source-line records were refreshed,
and two previously changed chart bodies were rendered and screenshot-checked
again. The published hydration audit remains increment 374 at 508/508 and does
not include these worktree-only changes. No builds, tests, CI, 2010, or
Requirements Lab changes. Overall progress remains 80%; formal gates remain
4/4.

Increment 381 clarified that 80% is a working estimate separate from the four
completed publication/render gates, and made the current closure items visible
in the roadmap. Increment 382 reclassified the Save unreachable-time and RAVU
§6.21 questions as documented gaps: both guides identify missing evidence and
avoid normative claims. The working estimate is now 85%, with three actionable
items remaining. IEEE's current product page was rechecked; it still offers
Purchase and Access via Subscription without exposing §6.21 prose.

Increment 383 corrected the opening state in Federation Synchronization diagram
#006: the registry prepares announcement work and inserts the point before the
application callback is delivered. The label and adjacent text now describe
that source boundary without implying callback execution. Mermaid CLI 12.0.0
rendered the diagram and its screenshot was inspected; the diagram identity,
line, hash, and local review status were reconciled in the visual ledger. The
worktree link checker resolves 4,661/4,661 references across 38 Markdown files.
This increment changed no C++ source or tests and ran no builds, tests, or CI.
2010 and Requirements Lab remain out of scope. Overall progress remains 85%;
formal gates remain 4/4.

Increment 384 (2026-11-09) compared current source-line links against candidate
HEAD `e10fa19a7bf48415e00a14ca7301613a84f637a3`. The worktree check still
resolves 4,661/4,661 references, while HEAD resolves 4,654/4,661. Seven
anchors in the Connection and Configuration guide point into dirty host,
initialization, and process-test files that have not reached HEAD. After
those changes have a focused run and are committed, reconcile the anchors
against committed source and rerun the HEAD audit before publishing. No C++
source or tests changed in this documentation increment; no builds, tests,
or CI were run. Overall progress remains 85%; three action items remain and
all four formal gates are complete. Keep 2010 and Requirements Lab excluded.

Increment 385 (2026-11-09) rechecked the RAVU class-scope fan-out and delivery
boundary against the shared 2025 planner, embedded callback adapter, configured-
process service, and focused test sources. The planner groups each object’s
requested remotely owned attributes by current provider. Embedded delivery
rechecks the original provider at callback entry; configured-process delivery
rechecks before queueing or Push send, with no later ownership check in the
inspected dequeue/callback path. The guide already explains these paths and clearly marks same-object
multi-provider fan-out and ownership transfer around the process recheck as
untested, source-derived behavior; no normative callback count, ordering, or
outcome is claimed. A similarly named regional Auto Provide multi-provider
test follows the discovery-triggered path and does not exercise explicit RAVU.
The guide now links that test as a separate service path. The worktree link
check resolves 4,662/4,662 references; candidate HEAD resolves 4,655/4,662,
with the same seven Connection and Configuration anchors pending the separate
source-link handoff. No source/tests, builds, test runs, or
CI changed in this increment. Overall progress is 90%, with two actions remaining
and four formal gates complete. Keep 2010 and Requirements Lab excluded.

Increment 386 (2026-11-09) recorded a saved installed-package TCK result for
`cpp-tck.connection-fault-rti-closed-resume-fails`. The result manifest marks
both HLA_EVOKED and HLA_IMMEDIATE passed. Its scenario source asserts the
public `ConnectionLost` callback with a nonempty description, then
`NotConnected` from a later `listFederationExecutions`. The raw fault files
show an invalid transport sequence closed the RTI socket; Resume requests
continued on session 2 with no Resume Status response. The raw files report
59 evoked and 73 immediate Resume attempts, while the aggregate result's
embedded counts report 61 and 57; this mismatch is recorded and the counts
are not used to establish a behavior claim. The test scenario and dispatch
remain uncommitted in the primary checkout and are absent from candidate HEAD;
the installed-package artifact records no source revision or executable
identity. Treat this only as supporting evidence for the client's local
ConnectionLost/NotConnected terminal path. It does not cover configured-process
host membership removal, Federate-Lost reporting, HLAfederate removal, or
observer ordering. Host recovery source-link and process lifecycle review stay
open. Overall progress remains 90%, two actions remain, and all four formal
gates remain complete. No build/test/CI work was performed in this
documentation increment; 2010 and Requirements Lab remain excluded.

Increment 387 (2026-11-09) tightened the configured-process ConnectionLost
evidence in the federation/federate lifecycle guide. The source trace
separates the failed member's local callback from host session termination
and survivor projection: the host removes its worker record and closes the
connection, the service captures the Federate-Lost report plan before the
shared forced membership transition, then dispatches cleanup result families
before the optional report. That source submission order does not establish
survivor callback order. A saved 2026-11-07 current-process artifact marks
`cpp-tck.federate-lost-mom-report` passed under both callback models, but
records no source revision or executable identity and predates the current
TCK source modification; it is historical scenario-ID evidence only.
Current worktree assertions cover cutoff/report/member-list/synchronization
observations but do not establish report/local-callback/member-list order or
forced-loss removal of the lost member's `HLAfederate` object. The guide now
labels these limits explicitly. No diagram count changed. The worktree link
check remains 4,662/4,662; candidate HEAD remains 4,655/4,662 with seven
source anchors pending committed implementation changes. Overall progress
remains 90%, with two actions and four formal gates complete. No build, test,
CI, 2010, or Requirements Lab work.

Increment 388 refined the configured-process ConnectionLost evidence by
locating the latest saved current-process runs for the Federate-Lost TCK
scenario and its contract variant. The 2026-11-07 `sync-primary` artifact
marks `cpp-tck.federate-lost-mom-report` passed for HLA_EVOKED and
HLA_IMMEDIATE; `sync-contract` records the same for
`cpp-tck.federate-lost-mom-report-contract`. Neither artifact contains a
source revision or executable identity, and both predate the current
uncommitted TCK source, so they are historical scenario-ID evidence only.
The lifecycle guide now names both runs and distinguishes them from the
current source assertions. It continues to document the configured-process
source sequence and leaves host/member-object callback ordering unproven.
The two completion actions and 90% working estimate are unchanged. Worktree
references remain 4,662/4,662; candidate HEAD remains 4,655/4,662 with seven
anchors pending the focused run and commit. No build, test, CI, 2010, or
Requirements Lab work.

Increment 389 compared the hosted Mermaid audit at revision
`270f302536ddb2f2bd56a35b565d2251a45f03e2` with the current remote branch and
local 2025 guide sources. The remote branch at `e10fa19a7bf48415e00a14ca7301613a84f637a3`
has the same 508 Mermaid bodies as the audited revision. The current worktree
has 509 bodies: 507 are unchanged, Federation Synchronization #006 is revised,
and Save/Restore #043 is added. Callback/Ordering #019 and TSO-Retraction #008
are already covered by the hosted audit. The two changed bodies have local
render and screenshot evidence; refresh hosted hydration after publication.
The 90% estimate and the separate configured-process ConnectionLost handoff
are unchanged. No build, test, CI, 2010, or Requirements Lab work.

Increment 389 also reconciled 16 stale visual-ledger diagram line references
after current guide prose edits. An independent scan confirms all 509 entries
match their source line, type, fence, and body hash. The documentation link
check resolves 4,662/4,662 worktree references; seven candidate-HEAD anchors
remain into the three dirty recovery source/test files and will be reconciled
after their focused run and commit. The Attribute Value Update Request guide
now links its Auto Provide distinction to the test's region-aware subscription
trigger at line 459 and states that this automatic path does not verify the
explicit request service's fan-out. The guide's access note was also checked
against the current IEEE SA product and GET Program pages.

Increment 390 (2026-11-09) added a field-level 2025 flow to the FOM admission
guide. It separates a `basicData` Name that passes DIF identifier syntax but
is rejected by the merged-model reserved-HLA-name check from `Endian=Middle`,
which fails the pinned DIF schema's `Big`/`Little` enumeration. The two
examples are independent; the diagram does not imply a shared validation
order. Mermaid CLI rendered the chart and screenshot inspection found the
labels legible. The primary-checkout portable TCK result records the standard-
invalid corpus and its same-provider contract twin passing in EVOKED and
IMMEDIATE. The result's `scenario_foms` map is empty, so legal `Big`/`Little`
public-Create acceptance remains unverified. The field-specific fixtures and
result remain worktree-only, and the artifact records no source or executable
identity; it does not establish independent-provider behavior or exhaustive
validation. The visual ledger now reconciles 510/510 source identities and the
worktree link check resolves 4,667/4,667 references. Candidate HEAD resolves
4,660/4,667; the same seven recovery-source anchors remain pending. The published audit
still covers 508 charts; after publication, refresh hosted hydration for
Federation Synchronization #006 and the added Save/Restore #043 and FOM
Admission #001 charts. Overall progress remains 90%. No C++ source, tests,
builds, test reruns, or CI were part of this documentation increment. Keep
2010 and Requirements Lab excluded.

Increment 391 (2026-11-09) reconciled the basicData Endian evidence with a later
primary-checkout public-API run. The earlier `basic-data-representation-name-endian`
artifact still has an empty `scenario_foms` map and proves no positive Endian
case. The separate `basic-data-representation-endian-values` artifact includes
the Big and Little fixture keys and records the primary scenario and its
same-installed-provider contract twin passing under EVOKED and IMMEDIATE (4/4).
The current primary-checkout runner iterates both fixtures, calls Create and
Join, checks the federate handle, then resigns and destroys the federation.
This demonstrates those two selected values in that worktree. The updated runner
and both fixture files remain absent from the published candidate; the guide
marks that evidence as worktree-only and candidate reproducibility remains
open. The flow now shows the accepted Big/Little path alongside the distinct
reserved-Name and Middle rejection paths. Mermaid CLI 12.0.0 rendering and
screenshot inspection passed.

The visual ledger now matches all 510/510 current diagram identities. The
ledger's stale TSO Retraction #008 body hash was reconciled after a local render
and screenshot review. The worktree link checker resolves 4,667/4,667 references;
the seven candidate-HEAD Connection and Configuration anchors remain pending.
The published audit covers 508 charts, so Federation Synchronization #006,
Save/Restore #043, and FOM Admission #001 still need hosted hydration after the
current edits are published. Progress remains 90%. No C++ builds, tests,
reruns, or CI were performed; 2010 and Requirements Lab remain excluded.

Increment 401 (2026-11-09) refreshed the bounded five-category text-presence screen across all 32 2025 topic guides. Each category passes in 32/32 guides: normative basis, state and observation, edge/failure boundary, test evidence, and known limits/gaps. The check explicitly includes the timing and cross-cutting guides. After a narrower keyword screen missed it, manual review confirmed that the Ownership Persistence guide has a “Deliberate limits and next step” section. The screen confirms text presence, not semantic correctness of standard citations, source behavior, or test assertions. Updated the render-audit record and active handoff. No source, test, or diagram changed; no builds, tests, or CI ran. Keep 2010 and Requirements Lab excluded.

Increment 402 (2026-11-09) revalidated the current 2025 documentation worktree against candidate HEAD `e10fa19a7bf48415e00a14ca7301613a84f637a3` and the latest fully hydrated hosted audit at `270f302536ddb2f2bd56a35b565d2251a45f03e2`. All 510 diagram bodies match the 510-entry visual ledger. A heading-context comparison finds exactly three chart deltas: Federation Synchronization #006 is revised, and Save/Restore #043 plus FOM Admission #001 are new. Mermaid CLI 12.0.0 rendered those three charts; screenshot inspection found their labels and branches legible without clipping or overlap. The source-link checker resolves 4,710/4,710 worktree references; candidate HEAD and its remote resolve 4,701/4,710, leaving nine anchors across four in-flight source/test files. Hosted hydration remains verified only for the prior 508-chart revision, so the three candidate deltas still need publication and hosted verification. This documentation review did not edit implementation files or run builds, tests, or CI. Keep 2010 and Requirements Lab excluded.

Increment 403 (2026-11-09) checked GitHub's Markdown API GFM conversion for the three candidate pages with Mermaid deltas. The 21 Federation Synchronization, 43 Save/Restore, and 13 FOM Admission source fences produced 77/77 Mermaid render containers across three nonempty HTML responses. Together with increment 402's Mermaid CLI 12.0.0 renders and screenshot review, this verifies local rendering and GitHub GFM container generation; it does not verify browser hydration of the published pages. The active source-link and hosted-hydration gates are unchanged. No implementation files, builds, tests, or CI were part of this increment. Keep the work 2025-only; 2010 and Requirements Lab remain excluded.

Increment 404 (2026-11-09) expanded that GFM check to the complete 38-page 2025 documentation set: all 32 topic guides and six supporting pages returned nonempty HTML, and all 510 source Mermaid fences produced 510 `data-type="mermaid"` containers with no count mismatch or API failure. The largest input was 279,199 bytes, below the audit's recorded 400 KB endpoint limit. This verifies current-worktree GFM parsing and container generation, not visible hydration on the published pages. A fresh resume/ready query retains the connection-fault replay handoff, now mapped to nine requirements across four standard sections; the related source/test work remains in flight. No builds, tests, or CI ran. Keep 2010 and Requirements Lab excluded.

Increment 405 (2026-11-09) compared the active connection-fault federate-request replay evidence in the primary and documentation-candidate worktrees. The primary TCK source uses public `getFederateHandle` calls with an adapter-managed fault fixture; its new current-process adapter withholds request `[sequence=1, session=2, request=2]`, forwards heartbeat sequence 2 on session 2, drops the matching RTI Heartbeat Response `[2,2,2]`, and records that the request is resent while the dropped control response is not replayed. The saved installed-package artifact records HLA_EVOKED and HLA_IMMEDIATE as passed, but has no source revision or executable identity. The candidate TCK function has a different body and does not contain the matching control-response evidence, so the primary artifact cannot support a candidate-source or published-guide claim. No guide or diagram change was made; retain the existing general replay explanation until matching source and provenance-bearing results are available in the candidate. Rechecked source links: 4,710/4,710 resolve in the worktree, while candidate HEAD and remote remain at 4,701/4,710. No builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 406 (2026-11-09) refreshed the active handoff from `ready`: `cpp-tck.connection-fault-federate-request-replay` now maps ten direct requirement pairs across five standard sections and exercises two sequential public-request fault windows. The primary source calls `getFederateHandle` twice, and its current-process adapter requires both `withheld_request_control_fault` and `replayed_hla_service_response` evidence. The saved installed-package artifact reports both callback models passed but contains only the first evidence block; it therefore does not verify the second response-loss window or identify the current source/executable. The candidate TCK source still has only one `getFederateHandle` call, and the matching control-response adapter evidence is absent. Keep successful recovery separate from terminal ConnectionLost, and do not add either new replay detail to the guide until the candidate has matching source and a provenance-bearing focused result. The worktree link audit remains 4,710/4,710; candidate HEAD/remote remains 4,701/4,710. No guide or diagram changed; no builds, tests, or CI ran. Keep 2010 and Requirements Lab excluded.

Increment 407 (2026-11-09) reran authenticated GitHub Markdown API GFM conversion after the roadmap updates. All 38 scoped 2025 documentation pages (32 guides and six supporting pages) returned nonempty HTML; all 510 Mermaid fences produced 510 Mermaid containers, with 1,042 headings and 8,217 rendered links and no API failures or page-count mismatches. The largest input was 279,199 bytes. This verifies current-worktree Markdown parsing and Mermaid container generation, not visible hydration on published pages. No guide diagram, implementation source, test, build, or CI changed. Keep 2010 and Requirements Lab excluded.

Increment 408 (2026-11-09) checked the nine candidate-HEAD source-link failures against the primary local HEAD `e6321e842f333427349166df8c5b35350a2ae473` and the committed remote `origin/feature/initial-work` at `c9c5a9e410539b039cd8bba35b53d0ef2b24d180`. Both revisions also resolve 4,701/4,710 links; the same nine cited lines are absent, and all four target files are modified in the primary worktree. The candidate worktree resolves 4,710/4,710. Thus the current citations are useful against the in-flight source but cannot yet be published as committed-source anchors. Keep them tied to the reviewed code and wait for the source changes to be committed or for a semantic re-review to identify accurate stable anchors. No guide or diagram changed; no build, test, or CI ran. Keep 2010 and Requirements Lab excluded.

Increment 409 (2026-11-09) reran authenticated GitHub Markdown API GFM conversion for all 38 current 2025 documentation pages: 32 guides and six supporting pages returned nonempty HTML, all 510 Mermaid diagrams—including the nine tilde-fenced charts—produced one GitHub Viewscreen container each, and the pass counted 1,042 headings and 8,217 rendered links with no API failures or mismatches. The largest input was 279,199 bytes. Fresh source-link checks resolve 4,710/4,710 references in the worktree; candidate HEAD, primary local HEAD, and `origin/feature/initial-work` each resolve 4,701/4,710, with the same nine anchors absent from four in-flight source/test files. GFM conversion is not hosted-page hydration, and current-worktree anchors are not yet committed-source anchors. No guide, diagram, source, or test changed; no build, test, or CI ran. Keep 2010 and Requirements Lab excluded.

Increment 410 (2026-11-09) audited edition boundaries across the current 32 2025 topic guides. All 32 identify 2025 scope in their opening 40 lines. The bounded text/link scan found 228 `2010`/`1516e` mention occurrences and 21 explicit links to separate-edition artifacts: the bounded 2010 reference-RTI guide, narrowly scoped 2010-FOM-input tests used by the 2025 API, and one explicitly separate 2010 handle adapter that shares only a byte helper. No `third_party/ieee1516e` implementation path is linked. The FOM Admission and DataElement Encoding guides already state that FOM model admission does not serialize application payload bytes and link the separate encoding path; no chart change was warranted. The latest primary `resume/ready` now selects the FQG/non-limiting-GALT TCK case; its proposed delivered-TSO minimum-time check remains pending source/test evidence and is not added as an observed result. No guide, diagram, source, or test changed; no build, test, or CI ran. Keep 2010 and Requirements Lab excluded.

Increment 411 (2026-11-09) rechecked the previously pending connection-fault replay case after the primary `resume/ready` moved to FQG. The primary TCK source now makes two public `getFederateHandle` calls, and its catalog and saved installed-package artifact report both fault windows under HLA_EVOKED and HLA_IMMEDIATE: the first withholds request `[1,2,2]` and drops its matching heartbeat response `[2,2,2]`; the second drops response `[5,2,3]` to request `[4,2,3]`, then records that same response identity replayed after a second Resume exchange. The result artifact contains both evidence blocks for both callback models, but records only `provider_id=installed-package`, not a source revision or executable identity. The candidate worktree still has one call in the TCK scenario and its catalog retains the older single-request description, so this is primary-only evidence and does not yet support a candidate-guide claim for the second fault. Keep the two recovery windows out of the candidate guides until matching source and provenance-bearing evidence are available there. The current primary handoff is the separate FQG/non-limiting-GALT case (8 requirement pairs across two standard sections); do not document its proposed delivered-TSO minimum-time result before its source and focused evidence exist. No guide, diagram, source, or test changed; no build, test, or CI ran. Keep 2010 and Requirements Lab excluded.

Increment 412 (2026-11-09) inspected the existing primary-checkout artifact `out/mgw/cpp-tck-fqr-minimum-20261109/results.json` for the active 2025 FQG case. It reports both the request-only scenario and its contract variant passed under HLA_EVOKED and HLA_IMMEDIATE, with `provider_id=installed-package`. The artifact records no source revision or executable identity, so it cannot establish whether it matches the current modified TCK source or verifies its delivered-TSO phase. The Time Management guide remains unchanged; do not present the proposed delivered-TSO minimum-time formula as observed. No build, test, or CI was run. Keep 2010 and Requirements Lab excluded.

Increment 413 (2026-11-11) added a 2025 `GetAttributeName` failure-report trace to the Service Invocation Reporting guide. It documents the valid object-class/invalid attribute-handle call, caller-visible `InvalidAttributeHandle`, report serial 4, checked supplied arguments, false indicator, Null return, and exception field. It also distinguishes the inherited `HLAfederate` parameter from the same-named `HLAfederate` scope dimension and records that the test does not check their values. The new Mermaid chart was rendered and screenshot-reviewed locally with Mermaid CLI 12.0.0. During the identity audit, Callback/Ordering #019 was also re-rendered because its ledger hash was stale; its body matches the previously hydrated hosted revision and is now recorded accurately. Authenticated GitHub Markdown API conversion of the full Service Invocation Reporting page returned 13/13 Mermaid containers (17 headings, 148 links), bringing current GFM coverage to 511/511 when combined with Increment 411's 510/510 full-scope pass. Hosted hydration remains unverified for the four current chart deltas. The worktree source-link check resolves 4,717/4,717; candidate HEAD, primary local HEAD, and origin/feature/initial-work each resolve 4,708/4,717 with the same nine unrelated in-flight anchors. No C++ build or test was run. Keep 2010 and Requirements Lab excluded.


Increment 400 (2026-11-09) refreshed the documentation handoff from the exact RTI resume/ready result. The related active 2025 source plan is `cpp-tck.connection-fault-federate-request-replay`, titled “Exercise control-message handling during public C++ connection-fault recovery,” with the installed-package process-adapter CTest selector for `HLA_EVOKED` and `HLA_IMMEDIATE` and command `python tools/cpp_tck.py --scenario cpp-tck.connection-fault-federate-request-replay`. The candidate branch remains at `e10fa19a7bf48415e00a14ca7301613a84f637a3`; the four linked source/test files remain modified in the candidate worktree. The documentation link counts and 510 diagram hashes remain as recorded in increment 399. This increment changed documentation handoff metadata only; no source build, test, or CI ran. Keep the work 2025-only and exclude Requirements Lab.

Increment 399 (2026-11-09) reconciled the active Roadmap index source-link
snapshots with fresh audits: the worktree resolves 4,710/4,710 references and
candidate HEAD resolves 4,701/4,710. The remote candidate branch remains at
`e10fa19a7bf48415e00a14ca7301613a84f637a3`; the nine unresolved anchors cross
four modified 2025 host, initialization, process-test, and lifecycle-TCK files.
The candidate worktree contains those in-flight source changes, which still need
their focused run and commit before publication. The primary checkout has the
Big/Little public Create/Join runner case and fixtures; neither the case nor
fixtures is present in the candidate. The guide identifies that evidence as
primary-only. All 510
Mermaid bodies match the visual-ledger hashes. Updated the active index snapshots
without rewriting the historical increment records. No source, test, or diagram
changed; no builds, tests, or CI ran. Keep 2010 and Requirements Lab excluded.

Increment 398 (2026-11-09) added direct names and pinned-header links for the
handle lookup, decode, and normalization operations, advisory controls, service
report switches, and time controls discussed in their existing guides. A
bounded symbol-name scan across the 32 topic guides finds all 164 unique
RTIambassador operation names and 56 FederateAmbassador callback names. This checks
navigation only; it is not a method-by-method behavior or conformance audit.
Reconciled the visual ledger after the prose additions: all 510 diagram bodies
still match their hashes, and 33 source-line entries were updated. No Mermaid
body changed in this increment.
The source-link checker now resolves 4,710/4,710 worktree references and
4,701/4,710 against candidate HEAD. The nine current-only anchors remain seven
Connection/Configuration links and two Lifecycle links into in-flight sources.
Progress remains 90%. No C++ builds, tests, reruns, or CI; 2010 and Requirements
Lab remain excluded.

Increment 397 (2026-11-09) reran the 2025 documentation source-link audit
against the live worktree and candidate HEAD. The worktree resolves 4,669/4,669
references; HEAD resolves 4,660/4,669. The nine current-only anchors are seven
Connection/Configuration links into host, initialization, and process-test
sources, plus two Lifecycle links into the TCK source. Updated the active
backlog to these counts and locations; historical increments retain their
original snapshots. The source changes remain in progress, so no C++ build,
test, rerun, or CI was started. Progress remains 90%; 2010 and Requirements Lab
remain excluded.

Increment 396 (2026-11-09) re-rendered and screenshot-reviewed the three chart
bodies that differ from the published 508-diagram snapshot. The first local
render showed that Save/Restore #043's horizontal layout made its labels too
small, so the chart now follows the pending timed Save vertically; its later
timed Save replacement and untimed Save supersession paths remain visible.
Mermaid CLI 12.0.0 rendered all three charts, and the diagram ledger now matches
all 510 source fences, line numbers, and body hashes. The worktree link check
resolves 4,669/4,669 references; candidate HEAD remains at 4,660/4,669, with
nine links in current-only sources. Progress remains 90%. No C++ builds, tests,
reruns, or CI; 2010 and Requirements Lab remain excluded.

Increment 395 (2026-11-09) added direct, worktree-only links from the
Federation and Federate Lifecycle guide to the configured-process
`scenarioFederateLostMomReportPortable` and its post-loss membership and
survivor-roster assertions. The hosted source-dependency audit snapshot still
needs a refresh for these current guide lines after the source changes settle.
The TCK source is 2,894 lines in the worktree
and 700 at candidate HEAD; these assertions remain unrun, and the November 7
saved scenario results do not identify their source revision or executable.
The current link checker resolves 4,669/4,669 worktree references. Candidate
HEAD resolves 4,660/4,669: seven links into dirty Connection/Configuration
sources and two into the current-only TCK source. Progress remains 90%. No
C++ builds, tests, reruns, or CI; 2010 and Requirements Lab remain excluded.

Increment 365 (2026-11-09) linked the timed Save guide's embedded test for all
five advance modes and the TARA equality/strict-boundary case. The worktree
checker resolved 4,633/4,633 unique references across 38 Markdown files.

Increment 366 (2026-11-09) checked the forced-ConnectionLost membership
reflection against saved run artifacts and reviewed configured-process
Save/Restore departure projection. The initial search in the documentation
candidate found no matching result for `cpp-tck.federate-lost-mom-report`. A
later search in the primary checkout found saved JSON/XML for the primary and
contract scenario IDs, each marked passed under `HLA_EVOKED` and
`HLA_IMMEDIATE` with provider `current-process`. Those November 7 artifacts do
not record a source revision or executable identity, and the current source
copies have since changed and differ between worktrees. Record them as
historical scenario-ID evidence only; they do not verify the newer
cutoff-TSO, membership-reflection, or survivor-synchronization assertions in
the current source. The October 22 `cpp-tck.connection-loss-cleanup` result
still covers a different scenario. The configured-process departure
dispatcher omits the registry Save and Restore notification vectors, as the
existing guide documents. These remain source-derived profile gaps, not
runtime or normative conclusions. No guide or diagram change was warranted;
the source and artifact limits remain explicit.

Increment 366 was published at commit 2259a89a with progress tag
hla-2025-docs-progress-2026-11-09-increment-366. All four formal gates remain
complete (4/4); overall progress remains 80% while semantic review continues.
No builds, tests, CI, 2010, or Requirements Lab changes were made.

Increment 367 reconciles this current baseline with the active handoff in
`ROADMAP-INDEX.json` and labels the increment-354 handoff snapshot as
historical. No service-guide prose or diagram changed.

Increment 368 (2026-11-09) re-audited class-scope RAVU fan-out against the live
registry diff, configured-process handlers, and focused embedded/process test
assertions. The shared planner expands the class to live objects and groups
each object's requested attributes by current owner; the configured-process
service handles those object/provider groups separately. The existing guide
already records that one object can split across providers in source, while
the focused tests cover one provider across multiple objects and single-group
regional cases. No focused case covers a same-object multi-provider split or
ownership changes during pre-enqueue persistence or before callback entry.
The current source's per-attribute regional-dimension preflight and overlap
checks are already marked as worktree-specific in the guide. No guide or
diagram edit was warranted. The handoff is complete; no builds, test runs, CI,
2010, or Requirements Lab work were performed. Overall progress remains 80%;
all four formal gates remain complete (4/4). The worktree link checker resolved
4,633/4,633 references across 38 IEEE 1516.1-2025 Markdown files.

Increment 369 (2026-11-09) audited the 2025 timed Save acceptance and pending
state across embedded and configured-process paths. The embedded public
adapter checks GALT/current-time bounds before the registry stores the request;
the registry repeats its representation and constrained-member checks. A later
ineligible advance checkpoint returns without selecting a Save failure reason
and keeps the pending request. The configured-process request handler calls
the same registry request path, and its pre-grant lifecycle calls the same
per-member admission method; when no initiation label is returned, it emits no
Save event and continues to the grant. Embedded focused tests cover the five
advance-mode boundaries, TSO blockers, and replacement. The configured-process
test covers replacement only. Neither profile has a focused case where the
requested time becomes unreachable or asserts a terminal
`SAVE_TIME_CANNOT_BE_HONORED` notification. The only implementation use found
is the MOM reason formatter; the normative trigger remains unresolved because
the normative 2025 clause text is unavailable in the checkout. The Save guide
now distinguishes request-time rejection from post-acceptance pending state
and records the shared source boundary without claiming full profile parity.
No diagram change was needed. The worktree checker resolved 4,637/4,637
references across 38 IEEE 1516.1-2025 Markdown files. No builds, tests, or CI;
no 2010 or Requirements Lab edits. Overall progress remains 80%; all four
formal gates remain complete (4/4).

Increment 370 (2026-11-09) traced the 2025 same-object provider split through
configured-process class requests. The planner groups each object's requested
attributes by current owner. For each group, the service registers the request
and immediately consumes/rechecks it before constructing the event. Pull adds
the event to the provider queue during the loop; Push stages the event and
sends after group preparation. The later Pull poll and callback bridge do not
consult current ownership. A transfer after the recheck can therefore leave an
event for the former provider in the delivery path; this is source-derived,
not a proven callback outcome or HLA guarantee. The embedded path retains its
separate callback-entry recheck. Existing focused cases cover one provider
across two objects and one regional provider/object/attribute, but no
same-object multi-provider split or ownership-transfer window. The RAVU guide
now records that distinction and the test gap; no diagram change was needed.
The worktree checker resolved 4,638/4,638 references across 38 IEEE
1516.1-2025 Markdown files. No C++ source/tests, builds, or CI; no 2010 or
Requirements Lab edits. Overall progress remains 80%; all four formal gates
remain complete (4/4).

Increment 371 (2026-11-09) compared the pinned IEEE 1516.1-2025 C++ API
headers with the accessible official standard record for the open RAVU
requester-eligibility and callback-timing questions. The imported
`RTIambassador.h` labels both request overloads §6.21 and provides signatures
and exception annotations; `FederateAmbassador.h` labels
`provideAttributeValueUpdate` §6.22 and provides only the callback declaration
and `FederateInternalError` annotation. Neither contains clause prose about
requester entitlement, callback trigger, or order. The IEEE SA product page
identifies the active 2025 standard and provides product metadata/access
routes, but not §6.21 text; the checkout contains the API import, not the
normative standard. The guide records this evidence boundary and does not
infer semantics from an exception-list omission. Direct requester entitlement
and normative callback timing remain unresolved. No diagram change was
needed. No C++ source/tests, builds, or CI; no 2010 or Requirements Lab edits.
Overall progress remains 80%; all four formal gates remain complete (4/4).

Increment 372 (2026-11-09) reviewed the 2025 federation/federate lifecycle and
connection/configuration guides against the pinned API declarations, current
registry/service source, and focused test sources. The connection guide
already distinguishes public Disconnect (which requires an unjoined
connection) from server-side transport loss and forced resignation, so it did
not need a content change. In the lifecycle guide, the hosted portable TCK
result at commit `c9c5a9e` is now explicitly historical. Added a focused
evidence-map row and linked the tracked Catch2 assertions for report payload
and subscription filtering, HLA_IMMEDIATE delivery, matching/disjoint region
selection under both callback models, and report-before-application-object
removal for two survivors. These are checked-in test assertions, not a fresh
test result; they do not establish configured-process runtime parity or the
removal order of the RTI-owned `HLAfederate` object. The current process TCK
source remains worktree-only evidence. No diagram change or C++/test edit;
no builds or CI; no 2010 or Requirements Lab edits. Overall progress remains
80%; all four formal gates remain complete (4/4).

Increment 373 (2026-11-09) audited the configured-process host outage lifecycle
in the 2025 connection/configuration guide. Added a two-clock flowchart that
separates client heartbeat/reconnect behavior from host inactivity retention
and purge, including sequence-history replay and the two independent terminal
effects. The source identifies the host fields as §12.13.4.5 Table 31 RTI
protocol settings; the diagram does not present Umbra's resume mechanics as a
standard requirement. Linked the current candidate source and test assertions,
explicitly labeling them dirty and unexecuted; no fresh runtime evidence is
claimed. Mermaid CLI 12.0.0 rendered the chart, and screenshot inspection found
it readable while scrolling. The visual ledger now records 508 locally
rendered and reviewed charts. The worktree checker resolved 4,657/4,657
references across 38 IEEE 1516.1-2025 Markdown files. No C++ changes, builds,
tests, or CI; no 2010 or Requirements Lab changes. Overall progress remains
80%; all four formal gates remain complete (4/4).

Increment 374 (2026-11-09) reran the published GitHub Mermaid audit against the
current candidate branch. All 32 guides exposed the expected 508 Mermaid
containers, and every frame contained an actual `.mermaid-view svg#diagram`;
every renderer returned HTTP 200, with no failed requests or page/count
mismatches. Two Callback and Service Ordering diagrams rendered after a longer
wait and were explicitly rechecked. The new Connection and
Configuration diagram #007 was scrolled into view and visually reviewed on
GitHub. The audit and ledger now record the full current result. No normative
claims were added, and source/test runtime evidence remains unchanged. No C++
edits, builds, tests, or CI; no 2010 or Requirements Lab changes. Overall
progress remains 80%; all four formal gates remain complete (4/4).

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

**Handoff snapshot recorded after increment 354 (historical; superseded by increment 367):**
The ConnectionLost observer-path
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

After publication, a focused hosted refresh loaded the Save/Restore guide at
revision `8add4f2931c65c6835778697887090be96d4907f`: all 42 expected Mermaid
containers hydrated, all 42 render requests returned HTTP 200, and the updated
diagram #008 was visually inspected. This refresh complements increment 356's
507/507 all-page audit; it is not a new all-page pass.

**2025 If-Wanted cancellation reply partition, increment 361:** The existing
ownership flow now explains that the cancellation gate snapshots only the live
reservation remaining after an If-Wanted transfer. The configured-process
cancellation handler calls the same planner and gate and projects the returned
subset into its event; this source evidence does not establish end-to-end
profile parity or callback order. The embedded race test covers one attribute.
A registered HLA_EVOKED TCK scenario source asserts a three-way result across
If-Wanted transfer, Release Denied, and cancellation confirmation, with the
three subsets covering the request exactly. It waits for all reply forms but
does not assert their order, and no runtime TCK result was reviewed. The simpler
two-attribute case where one attribute transfers and the remaining attribute
is cancellation-confirmed remains source-derived. No C++ source or tests
changed; no builds or tests were run. No 2010 or Requirements Lab content
changed. Overall progress remains 80%; all four formal gates remain complete
(4/4). See the [ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md)
for the state flow and source/test links.

**Next handoff:** The four linked host, initialization, process-test, and
lifecycle-TCK files remain modified in the candidate worktree. Once their
focused results and source changes are committed, reconcile the nine
current-only source anchors and rerun the candidate-HEAD check. The worktree
resolves 4,765/4,765 references; candidate HEAD, primary HEAD, and
`origin/feature/initial-work` each resolve 4,756/4,765. Then publish the
documentation candidate and refresh hosted hydration across all 32 pages and
515 diagrams, including the nine current chart deltas. The Big/Little public
Create/Join runner and fixtures remain primary-only and must stay labeled that
way until reproducible from the candidate. Keep this a 2025 documentation
review. Do not run builds, tests, or CI as part of this handoff; keep 2010 and
Requirements Lab excluded.


The older hosted-page audit records 31 available pages and one missing guide;
339 of the 340 hosted diagrams rendered, while one timing-guide diagram failed
the GitHub Mermaid parser. Those results belong to the old `feature/initial-work`
snapshot. Increment 356 historically confirmed that all 32 pages and 507
diagrams hydrated after viewport scrolling on its then-current candidate.
Increment 374's full published audit at guide-content revision
`270f302536ddb2f2bd56a35b565d2251a45f03e2` verified all 32 pages and 508/508
diagrams with rendered SVGs, HTTP 200 responses, and no failed requests. The
candidate contains no Requirements Lab or 2010 changes.

The four-gate score is **4/4 (100%)** at its recorded checkpoints: survey and
backlog, guide content, local/GFM/source-link verification, and visible
published-page rendering. Overall progress remains **91%** as a working
estimate. The RAVU fan-out/process-delivery and configured-process
ConnectionLost reviews are documented with their current evidence limits.
Current source-link reconciliation and hosted hydration are still pending: nine
links await matching committed source, and nine chart deltas need a fresh hosted
audit. The timed-Save and section 6.21 normative questions remain clearly
bounded in their guides. Keep diagram count a quality check rather than a
growth target, and keep the 2025 and 2010 implementation streams separate.

Completed progress details for increments 1–80 are preserved in the
[topic-audit history archive](HLA-BEHAVIOR-FLOW-GUIDES-ROADMAP-HISTORY-2026-11-07.md).
The current diagram identities and review evidence are recorded in the
[2025 diagram visual-review ledger](HLA-2025-DIAGRAM-VISUAL-REVIEW-LEDGER.json).
Keep this file focused on the current baseline, actionable backlog, coverage
map, and recent handoffs; archive older dated detail before the 6,000-line
documentation hard limit is reached.

## Progress and remaining documentation work

**Overall completion as of increment 532: approximately 95% (working estimate).** The 32-guide 2025 behavior set and four formal review gates are complete at their recorded checkpoints. All 515 current diagrams match the ledger by identity, source line, type, and normalized body hash; each has local render and screenshot evidence. The complete delta of seven additions and 21 revised bodies is published at candidate revision 431152817283718923c06a07e4154ca785e7033a. Revision-aware hosted hydration evidence covers 515/515 diagrams across all 32 guides: the prior full-page pass covered 508, and this increment checked all 130 diagrams on the five changed pages. Authenticated GitHub GFM checks cover all 41 current 2025 Markdown pages. Source links resolve 4,805/4,805 in the current worktree and at primary HEAD 4b50f50; published candidate HEAD resolves 4,796/4,805, with nine anchors in four active source/test files awaiting source stabilization and semantic recheck. Configured-process service-report behavior and focused ConnectionLost append-failure evidence remain bounded gaps. The 69 selected GitHub visual reviews are samples, not a review of every chart. No C++ builds, tests, or CI ran; keep 2010 and the Requirements Lab excluded.

Increment 458 reconciled the 515-entry diagram ledger against all 32 current guide sources: every identity, opening line, and normalized body hash now matches. Ten shifted line pointers in Service Invocation Reporting #008–#017 were corrected. Callback and Service Ordering #019 and DDM/Regions #006 were freshly rendered with Mermaid CLI and screenshot-reviewed before updating their stale hashes. This closes ledger integrity only; the 515-chart worktree remains unpublished, nine candidate-HEAD source anchors remain tied to in-flight files, and the completion estimate stays at 91%.

Increment 459 reviewed Ownership Persistence and Resignation #003 on published HEAD e10fa19a. All four diagrams on the page hydrated; the five-lane surviving-recipient save/restore flow remained readable in GitHub's full-width viewer while scrolling at default and increased zoom. Its raw body hash matches the published source. This is one embedded fresh-registry scenario, not a complete resignation-action matrix or normative callback rule; publication, source-anchor reconciliation, and full hosted verification remain open.

Increment 434 opened the hosted Time Management guide's Time Constrained role state machine (#003) in GitHub's full-width dialog. Its state names and transition labels were readable, and its body hash matches the published branch source. This resolves the inline-controls issue for this chart; other wide diagrams still need review.

Increment 435 checked a non-timing topic: Auto Provide diagram #001, the federation-wide switch state machine. GitHub rendered its `~~~mermaid` fence inline and in the full-width dialog; the Enabled/Disabled states and switch transitions were readable, and the body hash matches the published file. This is another targeted sample, not a full hosted-diagram audit.

Increment 436 checked DDM/regions diagram #002, ‘From FOM dimensions to runtime region scope.’ Its backtick-fenced chart rendered inline and in the full-width dialog; scrolling the tall flow showed the optional region read-back, object and interaction delivery paths, match/no-match outcomes, and separate logical-time and callback gates. Its body hash matches the published source. This remains a targeted sample, not a full hosted-diagram audit.

Increment 437 checked Federation Synchronization diagram #001, ‘HLA-level path: registration, reports, and federation outcome.’ GitHub rendered its backtick-fenced chart inline and in the full-width dialog. The registration branches, participant achievement-report loop, federation-level result, and HLA_IMMEDIATE/HLA_EVOKED delivery paths were readable; its body hash matches the published source. This is another sample, not a full hosted-diagram audit.

Increment 438 checked Save/Restore diagram #002, ‘Callback delivery crosses two independent gates.’ Its backtick-fenced chart rendered inline and in the full-width dialog. The configured-process transport decision and the separate HLA_IMMEDIATE/HLA_EVOKED callback-dispatch choice were both readable, as were the paths joining at the common dispatcher. Its body hash matches the published source. This remains a targeted sample, not a complete hosted review.

Increment 439 checked Callback and Service Ordering diagram #016, “Background dispatch runs without an active request.” Its seven-participant sequence rendered inline and opened in the full-width dialog. Scrolling the dialog kept the immediate-drainer exception paths, first-exception retention, existing-drainer FIFO path, and HLA_EVOKED path readable. The body hash matches the published file commit and branch head. This is another targeted sample, not a full hosted review.

Increment 440 checked Object and Interaction Information Flow diagram #020,
“The delivery route carries that captured view to the callback.” Its six
participant sequence rendered inline and opened in the full-width dialog. The
Pull (`HLA_EVOKED`, push disabled) and Push (`HLA_IMMEDIATE`, push enabled)
delivery branches were readable, including the explicit fixture-fence timing
limitation. The body hash matches the published file commit and branch head.
This adds a targeted sample, not full hosted hydration.

Increment 441 checked Authorization Flow diagram #001, “Authorization is a
gate, not the service result.” The chart rendered inline and opened in GitHub's
full-width dialog. The covered callback scopes, three result outcomes, and
service-specific precondition gate remained readable. The body hash matches
the published file. This adds a targeted sample, not full hosted hydration.

Increment 442 checked Attribute Ownership diagram #002, “Ownership transfer and update-region association are separate boundaries.” Its owner, prospective-owner, and regional-receiver lanes rendered inline and in GitHub's full-width dialog; deferred R-B, the cancellation branch, transfer, and R-B-versus-default-region outcomes were readable. Its raw Mermaid body hash matches the published source. This adds a targeted sample, not full hosted hydration.

Increment 443 checked Callback and Service Ordering diagram #025, “Process endpoint: service responses and synchronization events are separate.” Its five-participant sequence rendered inline and opened in the full-width dialog. The registration and final-achievement paths, pushed/polled outcomes, pending-barrier branch, HLA_IMMEDIATE/HLA_EVOKED dispatch, and separate report requests were readable while scrolling. This is a targeted implementation-profile sample, not full hosted hydration or an HLA-wide ordering claim.

Increment 444 freshly rendered and screenshot-reviewed Callback and Service Ordering #019 and TSO Retraction #008, then corrected their stale visual-ledger hashes. The full 515-body comparison now has zero mismatches. These are local checks; hosted hydration remains at 508/508 until publication. Fresh source-link checks resolve 4,776/4,776 in the worktree and 4,767/4,776 at candidate HEAD, primary local HEAD, and origin/feature/initial-work; the same nine anchors in four files and the recorded evidence gaps remain open.

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
   cancellation confirms that each transferred attribute leaves the live
   cancellation reservation and the callback gate snapshots the remaining set.
   The embedded transfer race covers one attribute. A registered HLA_EVOKED
   TCK scenario asserts a three-way split across If-Wanted transfer, Release
   Denied, and cancellation confirmation; it checks exact subset coverage but
   does not assert callback order. The simpler two-attribute path where one
   attribute transfers and the other is cancellation-confirmed remains
   source-derived; no runtime TCK result or configured-process callback order
   is claimed.
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

### Remaining documentation work

The working completion estimate is **91%**. The 32-guide set covers the major
2025 behavior areas, and all 515 current diagrams have identity-matched local
render and screenshot evidence. Increment 433 converted all 39 current 2025
Markdown pages through authenticated GitHub GFM: 515/515 Mermaid containers,
1,068 headings, 8,329 rendered links, and zero mismatches. Source links resolve
4,776/4,776 in the worktree and 4,767/4,776 at candidate HEAD; the nine
candidate-only anchors target four in-flight source/test files. The last fully
hydrated hosted candidate remains 508/508 diagrams across 32 guides; the current
515-chart set has seven additions and two revisions awaiting publication and
hosted verification.

Increment 434 opened the hosted Time Management guide's Time Constrained role
state machine (#003) in GitHub's full-width dialog; its labels were readable
and its body hash matches the published source. Continue sampling other wide
charts after the 515-chart candidate is published.

Increment 435 sampled Auto Provide diagram #001 on the hosted branch. The
tilde-fenced Mermaid chart rendered inline and opened in GitHub's full-width
dialog, where both states and their switch transitions were readable. The
chart body hash matches the published file. Continue with other behavior areas
and complete hosted verification after the 515-chart candidate is published.

Increment 436 sampled DDM/regions diagram #002. The backtick-fenced chart
rendered inline and opened in the dialog; the entire tall flow was readable by
scrolling through the region setup, optional read-back, regional matching, and
separate service gates. Its body hash matches both the published file commit
and branch head. Continue with remaining areas and hosted verification after
the 515-chart candidate is published.

Increment 437 sampled Federation Synchronization diagram #001. GitHub rendered
the backtick-fenced report loop inline and in the full-width dialog. Registration,
participant achievement, barrier completion, and HLA_IMMEDIATE/HLA_EVOKED
delivery labels were readable. The body hash matches the published file commit
and branch head. Continue with remaining behavior areas and full hosted
verification after the 515-chart candidate is published.

Increment 438 sampled Save/Restore diagram #002 on the hosted branch. The
backtick-fenced chart shows that configured-process event transport and
callback-dispatch policy are separate gates. Both the push/queue branches and
the HLA_IMMEDIATE/HLA_EVOKED outcomes were readable in the full-width dialog.
The body hash matches the published file commit and branch head. Continue with
remaining behavior areas and full hosted verification after publication of the
515-chart candidate.

Increment 439 sampled Callback and Service Ordering diagram #016 on the hosted
branch. The sequence chart rendered inline and opened in the full-width dialog.
While scrolling, the separate callback paths remained readable: an exception
reaching the active process dispatch loop, one escaping through an existing
immediate drainer, and one escaping from HLA_EVOKED. The body hash matches the
published file commit and branch head. Continue with the source-link closure
and full hosted verification after publication of the 515-chart candidate.

Increment 440 sampled Object and Interaction Information Flow diagram #020 on
the hosted branch. Its six participant lanes show the captured interaction view
traveling by Pull queue poll or Push event stream to callback dispatch. The
fixture-fence note preserves the limit on callback-before-sender-send-return
timing. Labels and branches were readable in GitHub's full-width dialog, and
the body hash matches the published file commit and branch head. Continue with
the source-link closure and full hosted verification after candidate publication.

Increment 441 sampled Authorization Flow diagram #001 on the hosted branch. It
rendered inline and opened in the full-width dialog; its callback-scope split,
authorization outcomes, and post-authorization service gate were readable.
The body hash matches the published file commit and branch head. Continue with
source-link closure and full hosted verification after candidate publication.

Increment 442 (2026-11-11) reviewed Attribute Ownership diagram #002, “Ownership transfer and update-region association are separate boundaries,” in GitHub's full-width dialog. The selected embedded 2025 flow's current-owner, prospective-owner, and regional-receiver lanes were readable, including the deferred R-B association, its optional removal, ownership transfer, and whether R-B remains active or updates use the default source region. Raw Mermaid body SHA-256 df654c67e1f978a40e8dc01a17000d35ae2e07107908f836cfcf6ee3f35845ec matches the hosted file commit 381af28c46040ee55a902f06417a2268a58bec61 and branch head e10fa19a7bf48415e00a14ca7301613a84f637a3. This is targeted readability evidence only. The 515-diagram worktree remains unpublished against the hosted 508-diagram baseline, source-anchor reconciliation and evidence gaps remain open, and overall progress stays 91%. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 443 (2026-11-11) reviewed Callback and Service Ordering diagram #025, “Process endpoint: service responses and synchronization events are separate,” in GitHub's full-width dialog. The five App, Adapter, Client, Service, and Dispatcher lanes remained readable while scrolling through registration and final achievement. The chart separates pushed requestId-0 notifications from polling queues, shows the pending-barrier path, distinguishes HLA_IMMEDIATE inline invocation from HLA_EVOKED queuing, and keeps service-report requests separate from the synchronization callbacks. Raw Mermaid body SHA-256 813d40a360de2e729d834f7a9ab5f7bd562e5d22ce817931f327dbe376fb657c matches the hosted file commit adafb10c37b6aad8f798e49ca7007abeaaeb66fc and branch head e10fa19a7bf48415e00a14ca7301613a84f637a3. This is implementation-profile evidence, not an HLA-wide ordering rule. The 515-diagram worktree remains unpublished against the hosted 508-diagram baseline; source-anchor reconciliation, hosted hydration, and evidence gaps remain open. Overall progress stays 91%. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

1. **Connection-recovery source and link closure — next.** The current
   connection-fault replay source and result remain primary-only: the source
   exercises two sequential fault windows, but the saved artifact lacks source
   revision and executable identity, and the candidate TCK scenario still has
   one request call. Keep the additional replay detail out of the guide until
   matching source and provenance-bearing evidence are available in the
    candidate. Four host, initialization, process-test, and lifecycle-TCK files
    remain modified. The worktree link check resolves 4,776/4,776 references;
    candidate HEAD, primary local HEAD, and origin/feature/initial-work each
    resolve 4,767/4,776, with the same nine anchors in those in-flight files.
    After they are committed, reconcile the anchors and compare each guide
    claim with its focused result. The Big/Little Endian Create/Join result is
    also primary-only; its runner and two fixtures are absent from candidate
    HEAD. The [missing-name GetObjectClassHandle report section](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#a-failed-lookup-has-a-caller-result-and-a-separate-report-trace)
    now records both the embedded file-report case and configured-process MOM
    interaction case. Neither has a located run artifact; configured-process
    file-sink coverage remains untested.

2. **Publish and perform a fresh hosted audit.** The last fully hydrated
   candidate at revision 270f302536ddb2f2bd56a35b565d2251a45f03e2 was audited
   at 508/508 rendered diagrams across 32/32 pages. The current worktree has
   515 diagrams: 506 bodies match that hosted set, two are revised, and seven
   are additions. All 515 have local render and screenshot evidence, and GFM
   container evidence covers all 515. The nine hosted deltas are Federation
   Synchronization #006 (revised), Save/Restore #043 (added), FOM Admission
   #001 (added), Service Invocation Reporting #006, #007, #008, #009, and #011
   (added), and Data Distribution #002 (revised).

   The audit JSON's top-level guide rows describe a historical
   feature/initial-work snapshot, not the current candidate. After the source
   links pass against committed source and the documentation candidate is
   published, refresh the audit summary and per-page results, then verify all
   32 pages, source links, and Mermaid hydration. Keep the revision-specific
   historical results distinct. The 91% estimate remains until source
   reconciliation and this hosted audit are complete.

### Documented normative questions

These two HLA questions remain unresolved, but their evidence limits and lack
of normative claims are explicit in the guides. They are documented gaps, not
unfinished guide tasks.

- **Timed Save at an unreachable requested time:** the Save guide traces the
  current request/checkpoint source paths and focused cases. No Save-path
  selector for `SAVE_TIME_CANNOT_BE_HONORED` or focused unreachable-time
  terminal-callback case was found; the normative trigger remains unverified.
  See the [Save/Restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md).
- **RAVU §6.21 entitlement and callback timing:** the
  [attribute-value-update guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md)
  distinguishes API signatures and exception lists from the missing clause
  prose, and does not infer requester entitlement or normative callback timing.
  The current [IEEE listing](https://standards.ieee.org/ieee/1516.1/6688/)
  still offers Purchase and Access via Subscription without exposing §6.21 text.

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

## Latest checkpoints — increments 460–467

The Federation MOM guide’s “Federation synchronization queries: active point
list and roster status” diagram rendered on the published candidate branch and
remained readable in GitHub’s full-width Mermaid viewer while scrolling through
the separate query branches and callback path. The configured-process path is
labeled as generic request handling without a paired report. Its raw Mermaid
body SHA-256 matches the hosted source and local visual ledger. This targeted
2025 visual sample does not establish normative completeness or embedded/process
parity. The working completion estimate remains **91%**.

As of increment 489, the hosted source-dependency map is refreshed. The
worktree resolves 4,776/4,776 references; candidate HEAD and its remote branch
resolve 4,767/4,776, with nine anchors in four in-flight source/test files
outside the committed file lengths. The remaining closure work is to reconcile
those anchors after the source owners' reviewed changes land, publish the
515-diagram candidate, verify all 515 diagrams on GitHub, and keep documented
evidence gaps bounded. The hosted branch still has the verified 508/508
baseline; seven chart additions and two revised bodies remain local-only. Keep
2010 and Requirements Lab out of scope.

Increment 461 reviewed Object Attribute Subscription State #009, “Embedded
discovery reentrancy precedes initial-value planning,” in GitHub’s full-width
viewer. The receiver-known split, discovery and initial-value path, scope
advisory and unresolved current-value branches, and separate provider outcomes
remained readable while scrolling and panning. Its body hash matches the
published source and visual ledger. The sequence is an embedded-source
checkpoint: the cited tests do not re-enter RTI services from discovery
callbacks, so this is not tested callback-order evidence. The exact licensed
§5.8 rule and process equivalence remain open. This targeted sample leaves the
91% estimate unchanged.

Increment 462 reviewed MOM Object and Interaction Reports #008, “MOM interaction-send reports separate total sends from the directed subset,” in GitHub’s full-width viewer. The separate total and directed request paths, accepted-send accounting, report ledgers, and report delivery remained readable while scrolling. Its raw body hash matches the published source and visual ledger. This targeted visual sample leaves the working estimate at 91%; it does not close publication, full 515-chart hosted verification, source-anchor reconciliation, or the documented evidence gaps. Keep the 2010 stream and Requirements Lab excluded.

Increment 463 reviewed Relevance Advisory #001, “One object subscription, separate calculations,” in GitHub’s full-width viewer. The overview branches one committed subscription change into separate class-level relevance, known-instance owner relevance and receiver scope, discovery, and later value delivery. The full chart extends beyond one viewport; zoom and pan made the reviewed regions readable, with no visible branch overlap. The source hash matches the published chart and visual ledger. The guide keeps embedded 2025 class-level callback evidence separate from the process-profile gap and makes no callback-order or parity claim. This sample leaves the working estimate at 91% and does not close source-anchor reconciliation, publication, full hosted verification, or documented evidence gaps. Keep 2010 and Requirements Lab excluded.

Increment 464 reviewed Attribute Value Update Request diagram #001, “1. HLA-facing model: solicitation and value delivery are distinct,” in GitHub’s full-width Mermaid viewer. The role lanes and separate request, optional provider callback, optional later Update, and receive-order versus timestamped reflection paths remained readable in the upper and lower portions while scrolling; no cropped message labels or ambiguous connector paths were visible. Raw Mermaid-body SHA-256 742e55f1bbbbfabd4e03b398b35859812192452ce003f1dca6dcaa39bf36e2de matches published candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3 and the visual ledger. The surrounding guide frames the chart as a causal model, not a required RTI call sequence or provider-callback-before-return guarantee; the normative §6.21 requester-entitlement question remains open. This targeted visual sample leaves the working estimate at 91% and does not close source-anchor reconciliation, publication, full hosted hydration, or documented evidence gaps. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 465 reviewed Federation Synchronization diagram #001, “HLA-level path: registration, reports, and federation outcome,” in GitHub’s full-width Mermaid viewer. The tall flowchart remained readable while scrolling through the registration decision and failure outcome, separate registration-result and participant-announcement paths, achievement reports, the pending-versus-complete branch, and callback-model endpoints. Raw Mermaid-body SHA-256 8bbc9d3647df0cd3a3d7e14a5e3f28b77fcaef5b2406f874b7c2aa8ae3e165d5 matches published candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3 and the visual ledger. The guide says the success-notification paths are intentionally unordered and does not claim a total HLA callback order. This targeted review leaves the working estimate at 91% and does not close source-anchor reconciliation, publication, full hosted hydration, or documented evidence gaps. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 466 reviewed Cross-Cutting State, Event, and Observation diagram #001, “One behavior, four different questions,” in GitHub’s full-width Mermaid viewer. The chart rendered inline; the dialog’s floating controls crowded the far-right prompt at the initial scale, but one zoom-out fit the complete chart with all four question categories and eight prompt leaves legible. Connectors remained traceable, with no cropped labels or ambiguous branches. Raw Mermaid-body SHA-256 2b44fa0e3e2bf24dd9893d6b3fec25519f5372beabf14544d27384d40629785d matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. This is a conceptual reading checklist, not a prescribed service sequence or new normative rule. The targeted review leaves the working estimate at 91%; source-anchor reconciliation, publication, full 515-chart hosted hydration, and documented evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 467 reviewed Cross-Cutting State, Event, and Observation diagram #002, “Keep state, pending work, and observation distinct,” in GitHub’s full-width Mermaid viewer. The wide chart rendered inline; the dialog’s floating controls covered part of the final callback/result box at the initial scale, but one zoom-out brought every box, label, and connector into view. The application-to-service path, possible changes to federation facts and participant-local view, effects in flight, eligibility gates, callback/result, query paths, and application feedback were readable without ambiguous connectors. Raw Mermaid-body SHA-256 1e0dd4f8ab9b19779c206c8863d6980460bd570fe910744752c1c5770b5737ce matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. The guide explicitly says these boxes are a reading aid, not mandatory RTI architecture or one universal timeline. The targeted review leaves the working estimate at 91%; source-anchor reconciliation, publication, full 515-chart hosted hydration, and documented evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Progress history

Detailed later review increments are indexed in the [2025 documentation progress history](HLA-2025-DOCUMENTATION-PROGRESS-HISTORY.md). Earlier topic-audit notes for increments 1–80 remain in the [early history archive](HLA-BEHAVIOR-FLOW-GUIDES-ROADMAP-HISTORY-2026-11-07.md). The active roadmap keeps the current baseline, actionable backlog, and API-to-guide coverage map.

Increment 468 reviewed Cross-Cutting State, Event, and Observation diagram #003, “A federation model defines vocabulary; services create live state,” in GitHub’s full-width Mermaid viewer. The tall chart required vertical scrolling to inspect the FOM-to-model path and the lower object and interaction branches; reduced zoom kept the reviewed labels and connectors readable. The map separates composed FOM vocabulary from live federation state, then shows distinct object-service discovery/reflection and interaction-delivery outcomes. Raw Mermaid-body SHA-256 d72e66e28fd9717e031694fc7e42be75ec34af1110ec0f9d1776313392fe076f matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. It is a relationship map, not a normative call sequence. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 469 reviewed Cross-Cutting State, Event, and Observation diagram #004, “A class hierarchy does not imply one inheritance rule,” on the published candidate branch. The inline chart’s floating controls cover part of the final interaction subscription/parameter-projection box; the full-width dialog fits the whole map and keeps both class-tree branches, all boxes, and connectors readable. The chart separates object-class inherited attribute lookup, class-scoped publication withdrawal, and receiver-known-class projection from interaction-class inherited parameter lookup and subscription matching/parameter projection. Raw Mermaid-body SHA-256 39b58cd8514a90237804deb1a5b1f9316b73b710c57a147a23db2b6b40821a15 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. The guide frames this as a selected 2025 embedded comparison, not a universal inheritance algorithm. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 470 reviewed Cross-Cutting State, Event, and Observation diagram #005, “A support switch has an owner, a lifetime, and a control surface,” on the published candidate branch. The inline chart is taller than one viewport; the full-width Mermaid dialog and vertical panning expose its read-only federation-policy branch, FDD-seeded member directive with per-member override and resignation cleanup, and member-local setter with operation-specific consumers. At the original scale, labels and connectors were readable in the inspected upper and lower portions; reduced zoom fits more of the chart but makes labels small. No overlapping branches, cropped labels in the inspected portions, or ambiguous connectors were observed. Raw Mermaid-body SHA-256 3a7eccc70b7616ddc0261be1cdb033a6f29f54f55b7afb0f781206ae284fe522 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. This is a 2025 interface-classification map, not one universal support-switch state machine. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 471 reviewed Cross-Cutting State, Event, and Observation diagram #006, “An object-attribute value touches several independent boundaries,” on the published candidate branch. The tall relationship map extends beyond one viewport; the full-width Mermaid dialog and vertical scrolling exposed its optional provideAttributeValueUpdate solicitation, independent provider update, producer publication and ownership, receiver eligibility and DDM, order and transportation controls, logical-time gate, embedded best-effort update-rate example, reflection, and callback endpoint. Labels and connectors were readable across the inspected upper, middle, and lower portions at the original scale; reduced zoom exposed more at once but made text smaller. This is a cross-cutting relationship map, not a universal service pipeline. Raw Mermaid-body SHA-256 54f8f6edb63c9926c7e3f3e31607db1684125d00cc58f757ff39155c90eabce5 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 472 reviewed Cross-Cutting State, Event, and Observation diagram #007, “Different protocols have different completion boundaries,” on the published candidate branch. The inline chart’s outcome boxes extend below one viewport; in GitHub’s full-width Mermaid dialog, one vertical pan brought every branch endpoint into view. The map clearly separates a federate’s time-advance grant, a synchronization point’s federation-level outcome, save/restore outcomes, and the per-federate callback-delivery model and callback-entry boundary. All four branches, clause labels, and connectors were readable, with no overlap or ambiguous paths. The guide keeps callback dispatch separate from the three coordination protocols and does not claim a total callback order. Raw Mermaid-body SHA-256 ef08eb7748f8fc48dd91d9b05a2323dc8bb2e9fab2b74b180e1bf2276e1b1b6d matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 473 reviewed Cross-Cutting State, Event, and Observation diagram #008, “Temporal admission does not imply callback entry,” on the published candidate branch. The tall chart was readable in sections in GitHub’s full-width Mermaid dialog: the temporal gate either holds a receive-order route in the deferred queue or submits eligible work to the callback dispatcher; the independent callbacks-enabled gate either leaves work pending there or proceeds to the selected callback model; HLA_IMMEDIATE and HLA_EVOKED then have separate callback-entry paths. Gate names, queue states, and transitions were legible, with no crossed or ambiguous connectors. This is a selected embedded 2025 implementation trace, not one HLA protocol or a claim that one gate cancels work at the other. The guide explicitly says the unit tests cover separate layers, not the combined gate history. Raw Mermaid-body SHA-256 025f52b550c2b8d4377e7bb219debef1ef68897f6b5466c9a9bd5cfb061486b1 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


Increment 474 reviewed Cross-Cutting State, Event, and Observation diagram #010, “Save/restore cohort,” on the published candidate branch. In GitHub’s full-width Mermaid dialog, the upper view showed the restore-roster mismatch/reject and match/admit branches plus connection-loss entry to forced-resignation cleanup; vertical panning exposed the separate save/restore resignation outcomes and the Join-during-operation branch ending in an explicitly unresolved result. Labels and connectors were readable in the inspected portions at the original scale; reduced zoom exposed more canvas but made labels smaller. No ambiguous connectors were observed in the inspected portions. The guide bounds these as selected embedded 2025 traces, leaves the Join outcome unverified, and makes no process or 2010 parity claim. Raw Mermaid-body SHA-256 d14f3d6f18415bc454b57f3e0866e5483bf138561f365c81d340a24ed8a23134 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


Increment 475 reviewed Cross-Cutting State, Event, and Observation diagram #011, “A service result, committed state, and a report are separate traces,” in GitHub’s full-width Mermaid dialog. At the initial scale, floating controls covered part of the final application-observation box; two zoom-outs fitted the complete chart, including the caller-visible result, optional HLA-required effect, no-report branch, and separate report-policy/file-or-MOM/application-sink path. Labels were smaller but readable at the fitted scale, and connectors remained traceable without overlap. The diagram is a relationship checklist, not a universal transaction timeline. Raw Mermaid-body SHA-256 e77eab28d22ac1b48ca7fb6b68e8c60529999fb6eaa8f41378c09fbe64a60162 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 476 reviewed Cross-Cutting State, Event, and Observation diagram #012, “Service contracts—not argument shape—define outcomes,” in GitHub’s full-width Mermaid dialog. The tall chart needed scrolling: the upper view showed the portable HLA outcome contract beside the selected 2025 Umbra implementation lens; the middle exposed all four state-unit examples; and the lower view showed partial per-name reservation results, whole-set release-none/release-all, and Join replacement-model/membership outcomes. Labels and connectors remained readable in the reviewed sections, with no visible overlaps. The guide frames these as separate examples, not one runtime sequence or a universal atomicity rule. Raw Mermaid-body SHA-256 900d8aec735f7aa8bb5d326d9ffae7d154c7086bc8a08a8ddd30ef31cd5f4281 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 477 reviewed Cross-Cutting State, Event, and Observation diagram #013, “Object information: instance existence versus local knowledge,” in GitHub’s full-width Mermaid dialog. Vertical scrolling exposed the registration/existence/receiver-discovery path, the local-versus-federation-wide delete choice, and the distinct removeObjectInstance observation after global deletion. The inspected labels and connectors were readable, with no visible overlap. The rendered text includes a later-eligibility return to discovery, but that connector was not isolated in the screenshots. The guide frames this as a relationship map, not a universal service timeline or normative state machine. Raw Mermaid-body SHA-256 46f1ecbfeece4551897e6c468d910bcca88ea4eafb0719f6521c556b511668c6 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 478 reviewed Cross-Cutting State, Event, and Observation diagram #014, “When work becomes stale: withdraw, invalidate, preserve, or account,” in GitHub’s full-width Mermaid dialog. Vertical scrolling exposed the candidate-only, queued-but-unobserved, and already-observed phases, their operation-specific decisions, and the independent-work outcomes. Labels and connectors were readable in the inspected sections, with no visible overlap. The chart is a reading aid, not a prescribed RTI algorithm or universal cancellation rule. Raw Mermaid-body SHA-256 f721b19e0e2714907946abfa8a320703719e0a0520d330f4fd54e5fe309d2de8 matches the worktree, candidate HEAD e10fa19a7bf48415e00a14ca7301613a84f637a3, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


Increment 479 reviewed Attribute Ownership diagram #001, “Four ownership paths: who initiates, and what reports the outcome?”, on the published 2025 candidate branch. The inline Mermaid rendered and scrolling exposed the requester-started Acquisition If Available path and the three current-owner paths: Unconditional Divestiture, Divestiture If Wanted, and negotiated divestiture with owner confirmation. The outcome notes keep service results, per-attribute notifications, and the separate If-Wanted notification channel distinct. The chart is tall and narrow inline, with heavily wrapped labels; its main paths and connectors were traceable, but a floating viewer control partly covered the lower-right negotiated-outcome box in one viewport. This sample did not open the full-width dialog. The guide frames the chart as an orientation map, not an exhaustive ownership catalog or callback-order diagram. Raw Mermaid-body SHA-256 `c0dbf640faab2cb3bd8fada760c5bf6048a3e0d83ac9c2fbc7a466478929e768` matches the worktree, candidate HEAD `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 480 reviewed Attribute Ownership diagram #003, “Catalog inheritance and instance authority are different layers,” in GitHub’s full-width Mermaid dialog. Both complete lanes fit at once: composed FOM class-schema declaration, parent-chain lookup, and tested catalog availability; then one registered instance, its current delete-privilege owner, and the instance-specific delete gate. Labels and connectors were readable without overlap. The raw Mermaid body SHA-256 24f2c06d01ddee77a6815930cff25a0dd97e5a9dcc6d4a67d8b84697f9144328 matches the published candidate and visual ledger. This is a selected 2025 schema/runtime distinction, not process parity. The overall working estimate remains 91%; citation reconciliation, hosted dependency refresh, publication, full hosted hydration, and documented evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 481 reviewed Attribute Ownership diagram #004, “A new owner can gain delete authority without becoming the registrant,” in GitHub’s full-width Mermaid dialog. The tall sequence required vertical scrolling. The upper view shows the publication-eligibility branch and the selected ownership transfer; the lower view shows A’s DeletePrivilegeNotHeld result, B’s tagged successful deletion, and later receiver-removal work. Reduced zoom made more vertical context visible while the inspected actor lanes and messages remained readable. The chart explicitly limits its selected embedded HLA_EVOKED test assertion to C’s removal callback, while A’s removal is source-planned. Raw Mermaid body SHA-256 5df2838c0d2985be21455c704afe1c180373b4797d33afee5bbe1cdaf3d2ffab matches the published candidate and visual ledger. No process transfer parity or complete callback coverage is claimed. The overall working estimate remains 91%; citation reconciliation, hosted dependency refresh, publication, full hosted hydration, and documented evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 482 reviewed Attribute Ownership diagram #006, “Requester-side state: request is not ownership,” in GitHub’s full-width Mermaid dialog. The chart separates accepted regular acquisition and pending state from notification, unavailable, and cancellation outcomes; one vertical scroll exposed cancellation confirmation with ownership unchanged and object deletion suppressing the queued callback. Labels and connectors were readable at the original scale without overlap. The chart describes a selected embedded model, not a complete normative state machine. Raw Mermaid-body SHA-256 b9d52b2399cdf517d89832d627cad3ba78f2b2a278a3f0b43e0d77d90817f4ab matches the published candidate and visual ledger. Overall progress remains 91%; source-reference reconciliation, hosted dependency refresh, publication, full hosted hydration, and evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


Increment 483 reviewed Attribute Ownership diagram #008, “Cancellation vs a divest-triggered acquisition notification,” in the published 2025 candidate branch. The inline flowchart rendered, and the full-width Mermaid dialog exposed the upper, middle, and lower paths across several vertical scroll positions. It distinguishes a pre-existing cancellation reservation that prevents notification scheduling from a queued notification whose begin gate races with cancellation planning. Notification-begin first assigns ownership and makes later cancellation too late; cancellation-plan first reserves the attribute and leaves the request pending while the queued notification is suppressed. The configured-process branch separately shows push beginning cancellation in the service handler before confirmation send and pull beginning it at the recipient poll. Labels and connectors were readable at the dialog’s default scale without overlap. The guide identifies the combined process race and callback ordering as source-derived, not tested or normative. Raw Mermaid-body SHA-256 `138b915b2788b31ccc94e02dc5619c808c953d5c351a2fe66a29ac6c6ad02ff6` matches the worktree, candidate HEAD `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 484 reviewed Attribute Ownership diagram #007, “Process endpoint: cancellation result and confirmation are separate,” in the published 2025 candidate branch. The inline sequence rendered and its full-width dialog showed the configured HLA_EVOKED pull path: cancellation returns a private status/count while the public void call has returned, callback work is dispatched, receive polling consumes the reservation, and the confirmation callback follows. The chart notes that the focused test observed no acquisition callback and limits the sequence to this process case. The tall sequence required a vertical scroll; its longer self-message labels are dense around the numbered endpoint markers at the initial dialog scale, and one zoom-in made them easier to read. No cross-profile timing rule is claimed. Raw Mermaid-body SHA-256 `2d133b24944adad529c0108f90753a7385293d764f5949c5245c7539812b316f` matches the worktree, candidate HEAD `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


Increment 485 reviewed Attribute Ownership diagram #009, the lifecycle-invalidation flow following “Cancellation vs a divest-triggered acquisition notification,” on the published 2025 candidate branch. The inline flowchart rendered and the full-width dialog exposed the upper decision and both lower outcome groups across two vertical scroll positions. It distinguishes requester resignation removing queued events from a receive-time gate that revalidates cancellation identity and current state: a stale ID or absent object returns no delivery; accepted deletion or loss of requester eligibility consumes cancellation with no confirmed attributes; still-eligible attributes may proceed to the callback bridge. Labels and connectors were readable at the dialog’s default scale without overlap. The guide marks both branches as source-derived, not an HLA rule, and says neither has focused process-boundary test coverage. Raw Mermaid-body SHA-256 `07ff591851a0ee8a840246061c521d78ff01b7a7042e1a6636bffbd943306c3e` matches the worktree, candidate HEAD `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and visual ledger. Overall progress remains 91%. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Increment 486 reviewed Attribute Ownership diagram #010, “Push delivery commits before the client can dispatch,” in the published candidate branch. GitHub rendered it inline and in the full-width dialog; three vertical views exposed the commit-before-send boundary, send-failure branch, buffered event and callback-bridge handoff, immediate/evoked split, and the later-evoke result after deletion or requester Resign. Labels and connectors were readable in the inspected portions without visible overlap. The guide explicitly bounds this to source-derived 2025 process behavior and records the missing process tests for send failure and post-deletion/Resign delivery. Raw Mermaid-body SHA-256 `cbf938fe7ed2d8057ded71a51289fa100d61fbbb5c4711c952a784b6800a5408` matches the local file, candidate HEAD, published raw source, and visual ledger. Overall progress remains 91%; citation reconciliation, hosted dependency refresh, publication, full hosted hydration, broad readability review, and bounded evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 487 — reviewed Resign directive 3 pending-transfer cleanup

Opened Attribute Ownership diagram #011, “Resign directive 3 removes only the departing acquirer’s pending transfer,” on the published 2025 candidate branch. GitHub’s full-width Mermaid dialog showed the sequence across its upper and lower scroll positions: regular acquisition and negotiated divestiture, the requester’s Resign directive 3, member-scoped cleanup of acquisition/candidate and assumption-search state, then the owner’s callback drain with no release request or confirmation and ownership retained. The cleanup self-message labels are dense around the RTI lifeline but remain readable; no endpoint was clipped. The guide bounds its two focused cases to one departing requester and leaves other-requester outcomes, configured-process parity, and the normative scope of directive 3 open.

Raw Mermaid-body SHA-256 `5a62017dfe3923f049779ad8343cf687a4bab73c5fdfe052b94898e9fef50d71` matches the guide, candidate HEAD `e10fa19a7bf48415e00a14ca7301613a84f637a3`, published page, and visual ledger. The completion estimate remains **91%**; source-anchor reconciliation, hosted dependency refresh, publication, full 515-chart hosted hydration, broad readability review, and bounded evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 488 — reviewed regular acquisition with owner responses

Opened Attribute Ownership diagram #012, “Regular acquisition with an owner response,” on the published 2025 candidate branch. GitHub’s full-width Mermaid dialog showed the sequence across upper and lower views. The upper portion separates the accepted pending request, owner release request carrying the acquisition tag, and independent resolution of unowned attributes; the lower portion shows denial followed by an unavailable result and retained ownership, or an applicable divestiture path with acquisition notification. The callback-time notes and outcome branches remained readable, with no clipped endpoints. The guide limits the chart to cited embedded tests and does not claim multi-requester arbitration, every owner response, or process parity.

The visual-ledger Mermaid-body SHA-256 `d5b769320a0ea0891c1254234df6fc8129867432cf08131c6970b160a8bc5479` matches the unchanged guide at candidate HEAD `e10fa19a7bf48415e00a14ca7301613a84f637a3`; the published branch pointer is at the same revision. The completion estimate remains **91%**; source-anchor reconciliation, hosted dependency refresh, publication, full 515-chart hosted hydration, broad readability review, and bounded evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


## Increment 489 — refreshed hosted source-dependency snapshot

Refreshed the candidate source-reference snapshot after checking the 2025
documentation worktree. The focused link checker resolves all 4,776 unique
path/line references across 40 IEEE 1516.1-2025 Markdown files. Candidate HEAD
and its remote branch pointer remain at `e10fa19a7bf48415e00a14ca7301613a84f637a3`;
the recorded candidate-HEAD comparison resolves 4,767/4,776 references. The
nine current-worktree-only anchors are in four in-flight source/test files
whose committed versions are shorter; all nine resolve in the worktree. The
map records both revisions and their line counts, while keeping the separate
2026-11-08 root migration snapshot historical.

Overall completion remains **91%**, a working estimate rather than a fraction
derived from gate counts. Remaining closure work is to reconcile those nine
anchors after the source owners' reviewed changes land (or retarget/remove
them if they do not), publish the seven added diagrams and two revised bodies,
verify hosted rendering of all 515 charts, continue broad readability review,
and close or clearly bound the remaining evidence gaps. No C++ builds, tests,
or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 490 — reviewed If Available callback-time outcomes

Reviewed Attribute Ownership diagram #013, “If Available: evaluate at the result callback,” in the published 2025 GitHub page. The complete sequence rendered inline at 80% browser zoom. Its accepted-pending step and all three outcomes—unowned at the result-time recheck, still owned by A, and A divesting before the callback-time recheck—were visible together. Callback labels, branches, and connectors were readable with no clipping or overlap. The chart describes selected embedded-profile callback-time behavior, not a universal ordering rule.

The fetched published branch is `e10fa19a7bf48415e00a14ca7301613a84f637a3`; the page's last file commit is `381af28c46040ee55a902f06417a2268a58bec61`. The local and fetched Mermaid bodies both hash to `df4c46deb13c151ac4dcfd4c4653c84936acf9d0480aebb2bdc3c4f7f9bc2a2e`. The surrounding guide bounds the cited test to a mixed attribute set producing Notification and Unavailable callbacks. This review does not establish configured-process parity or a universal callback order.

The post-review documentation link check resolves 4,776/4,776 unique path/line references across 40 IEEE 1516.1-2025 Markdown files in the worktree; candidate HEAD remains at 4,767/4,776 with nine anchors awaiting reviewed source/test revisions. The live-browser history now has 47 targeted chart samples. Overall completion remains **91%**, a working estimate; this readability review does not close source-anchor reconciliation, publication, full 515-chart hosted hydration, broad readability review, or the documented evidence gaps. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 491 — reviewed If Available remaining-set resolution

Reviewed Attribute Ownership diagram #032, “If Available resolves its remaining set at the already-queued callback,” in the published 2025 GitHub branch. The full-width Mermaid dialog showed the tall flowchart across upper and lower scroll positions. The upper view separates the accepted `{A, B}` request and queued one-shot result from the If-Wanted transfer of `A`, its removal from the desired set, and the separate `A` acquisition notification with the If-Wanted tag. The lower view follows the original result callback through its registry gate and remaining-set check: report `B` unavailable with the original If-Available tag if it remains owned, otherwise resolve the current remaining state; then consume the one-shot request. Labels and connectors were readable, with no visible overlap or clipped endpoints.

The fetched branch remains at `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and the guide file’s last commit is `381af28c46040ee55a902f06417a2268a58bec61`. The local and fetched Mermaid bodies both hash to `8b02af3d0b000e522fd70acf3757fde0e4310f6e49f7dd01249ae54407d6acaf`. The guide explicitly bounds the combined multi-attribute result as source-derived: its focused If-Wanted pending test covers one attribute, and no focused test covers partial transfer followed by an unavailable remainder. No callback order outside that selected single-attribute HLA_EVOKED test is claimed.

The post-review documentation link check resolves 4,776/4,776 unique path/line references across 40 IEEE 1516.1-2025 Markdown files in the worktree; candidate HEAD remains at 4,767/4,776 with nine anchors awaiting reviewed source/test revisions. The live-browser history now has 48 targeted chart samples. Overall completion remains **91%**, a working estimate; this review does not close source-anchor reconciliation, publication, full 515-chart hosted hydration, broad readability review, or the documented evidence gaps. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 492 — reviewed If-Wanted transfer during pending cancellation

Reviewed Attribute Ownership diagram #033, “Divestiture If Wanted wins cancellation only for the transferred attributes,” on the published 2025 candidate branch. GitHub’s full-width Mermaid dialog showed the tall flowchart across upper and lower scroll positions. The upper view shows an accepted regular-acquisition cancellation for {A, B}, the owner release callback already in user code, the If-Wanted transfer of A, removal of A from the live cancellation reservation, and a separate acquisition notification for A. The lower view shows the queued cancellation gate: confirmation for A is suppressed, while B can still be resolved if it remains reserved and no other terminal response wins. Labels and connectors were readable with no visible overlap or clipped endpoints.

The published branch remains at `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and the guide file’s last commit is `381af28c46040ee55a902f06417a2268a58bec61`. The local and fetched Mermaid bodies both hash to `3c41788934bd3d79875a652853de88f67e6c56247de774a84b689882ef626fc5`. Evidence remains bounded: the focused transfer/cancellation test covers one attribute; the registered three-way TCK scenario is source evidence only here, with no runtime result reviewed; and the simple A-transfer/B-cancellation outcome is source-derived. No callback order or configured-process parity is claimed.

The increment 492 documentation link check resolves all 4,776 unique path/line references across 40 IEEE 1516.1-2025 Markdown files in the worktree; candidate HEAD remains at 4,767/4,776 with nine anchors awaiting reviewed source/test revisions. The live-browser history now has 49 targeted chart samples. Overall completion remains **91%**, a working estimate; source-anchor reconciliation, publication, hosted hydration of all 515 diagrams, broad readability review, and bounded evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 493 — reviewed ownership-query result classification

Reviewed Attribute Ownership diagram #034, “Query Attribute Ownership reports a snapshot; it does not transfer,” on the published 2025 candidate branch. GitHub’s full-width Mermaid dialog showed the tall flowchart across upper, middle, and lower scroll positions. The upper view showed service preconditions, validation of the requested object and attributes, and the separate RTI-owned MOM branch. The middle showed ordinary-object validation, synchronous exception paths, grouping by federate owner, separate unowned grouping, RTI-owned grouping, and routing each group to the requester. The lower view showed the three distinct outcomes—`informAttributeOwnership`, `attributeIsNotOwned`, and `attributeIsOwnedByRTI`—all ending in a report-only result with ownership unchanged. Labels and connectors were readable with no visible overlap or clipped endpoints.

The published branch remains at `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and the guide file’s last commit is `381af28c46040ee55a902f06417a2268a58bec61`. The local and fetched Mermaid bodies both hash to `c797c7d4c111acdcd54716203f8aa09a2c5e17a04a062d33af09d00080510cfd`. The guide explicitly describes the selected embedded 2025 path and keeps the configured-process endpoint separate; this visual review does not establish process parity, an exhaustive exception matrix, or callback ordering.

The increment 493 documentation link check resolves all 4,776 unique path/line references across 40 IEEE 1516.1-2025 Markdown files in the worktree; candidate HEAD remains at 4,767/4,776 with nine anchors awaiting reviewed source/test revisions. The live-browser history now has 50 targeted chart samples. Overall completion remains **91%**, a working estimate; source-anchor reconciliation, publication, hosted hydration of all 515 diagrams, broad readability review, and bounded evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 494 — reviewed query callback-entry revalidation

Reviewed Attribute Ownership diagram #035, “Query Attribute Ownership reports a snapshot; it does not transfer,” on the published 2025 candidate branch. The complete state flow fit in GitHub’s full-width Mermaid dialog in one view. It shows a query plan classifying attributes, persisting a pending identity, submitting the callback route after releasing the registry lock, and rechecking the identity and projection when callback execution begins. A stale query identity or projection suppresses the ownership callback; still-current results take distinct federate-owned, unowned, or RTI-owned MOM callback paths. All states, callback labels, and connectors were readable with no visible overlap or clipping.

The published branch remains at `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and the guide file’s last commit is `381af28c46040ee55a902f06417a2268a58bec61`. The local and fetched Mermaid bodies both hash to `f1040272b9735f090a945f1dd4867749d76e6e65b9187f50b7c6a3f42199cc56`. The guide’s focused deletion example demonstrates suppression of an already queued ownership report after object removal while the removal callback is delivered. This is selected embedded 2025 behavior; process parity and callback ordering are not claimed.

The increment 494 documentation link check resolves all 4,776 unique path/line references across 40 IEEE 1516.1-2025 Markdown files in the worktree; candidate HEAD remains at 4,767/4,776 with nine anchors awaiting reviewed source/test revisions. The live-browser history now has 51 targeted chart samples. Overall completion remains **91%**, a working estimate; source-anchor reconciliation, publication, hosted hydration of all 515 diagrams, broad readability review, and bounded evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 495 — reviewed accepted-deletion ownership cleanup

Reviewed Attribute Ownership diagram #036, “Accepted object deletion invalidates pending ownership work,” on the published 2025 candidate branch. GitHub’s full-width Mermaid dialog showed the entire flowchart in one view. Receive-order deletion is accepted before queued callbacks start, then the map branches: the embedded registry clears pending ownership-query and ownership-operation ledgers, while a separate receiver-removal reservation reaches its own member/known-object check. Query results revalidate identity and projection; queued ownership operations check their pending state at callback entry; both stale paths suppress delivery. Receiver removal either retires a stale reservation or commits the receiver to unknown and dispatches `removeObjectInstance`. Labels and connectors were readable with no visible overlap or clipped endpoints.

The published branch remains at `e10fa19a7bf48415e00a14ca7301613a84f637a3`, and the guide file’s last commit is `381af28c46040ee55a902f06417a2268a58bec61`. The local and fetched Mermaid bodies both hash to `fb9473f765f35691fc319f9149950ff31f322336e8fe11fa979b1526d8872a13`. The surrounding guide distinguishes receive-order per-instance deletion from timestamped Delete/Retract, local deletion, whole-member Resign, and DDM region-reference lifetime. Some cleared ledgers are source-observed rather than individually tested, and the separate callback gates do not establish a global order or configured-process parity.

The increment 495 documentation link check resolves all 4,776 unique path/line references across 40 IEEE 1516.1-2025 Markdown files in the worktree; candidate HEAD remains at 4,767/4,776 with nine anchors awaiting reviewed source/test revisions. The live-browser history now has 52 targeted chart samples. Overall completion remains **91%**, a working estimate; source-anchor reconciliation, publication, hosted hydration of all 515 diagrams, broad readability review, and bounded evidence gaps remain. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 496 — reviewed four-path ownership overview

Increment 496 rechecked Attribute Ownership diagram #001, “Four ownership paths: who initiates, and what reports the outcome?”, in GitHub’s full-width Mermaid dialog on the published 2025 branch. The full-height orientation flow required vertical scrolling: the upper view showed the requester/owner choice and four transfer paths, and the lower view showed the outcome boxes. The negotiated path remained readable through owner proposal (§7.3), RTI confirmation request (§7.5), owner confirmation (§7.6), and the pending requester’s separate acquisition notification. The chart also distinguishes If-Available callback outcomes, unconditional relinquishment and candidate offers, and Divestiture If Wanted’s divestedAttributes output from later notifications. Labels and connectors were readable with no visible overlap or clipped outcome boxes. Local and fetched Mermaid bodies match SHA-256 c0dbf640faab2cb3bd8fada760c5bf6048a3e0d83ac9c2fbc7a466478929e768. Published branch HEAD is e10fa19a7bf48415e00a14ca7301613a84f637a3; the guide file commit is 381af28c46040ee55a902f06417a2268a58bec61. The guide frames this as an orientation map, not an exhaustive catalog or callback-order diagram; its examples remain profile-bounded, with no process-parity or conformance claim. Completion remains 91% (working estimate). No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

The increment 496 documentation link check resolves 4,776/4,776 unique path/line references across 40 IEEE 1516.1-2025 Markdown files in the worktree; candidate HEAD remains at 4,767/4,776, with nine anchors awaiting reviewed source/test revisions. The live-browser history now contains 53 targeted chart samples. Overall completion remains 91%, a working estimate; source-anchor reconciliation, publication, hosted hydration of all 515 diagrams, broad readability review, and bounded evidence gaps remain. The next targeted review is Time Management diagram #020, the recipient-scoped timestamped-message lifecycle. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


## Increment 497 — reviewed recipient-scoped TSO message lifecycle

Rechecked Time Management diagram #020, “Timestamped message lifecycle and callback order,” in GitHub’s full-width Mermaid dialog. The complete recipient-copy state flow fit in one view: Queued branches to Retracted or InTransit; InTransit proceeds to Delivered or Suppressed; Delivered may trigger the service-specific Request Retraction rule or clear at the recipient’s next logical-time grant; terminalized suppression also clears. The retained completion-history note remains separate from GALT/LITS. Labels and connectors were readable without visible overlap or clipped states. Local and candidate-HEAD bodies match SHA-256 0b868ddcbcfa8093cd355cc21bf636e819aa70015a65f945e4bfc27d2e0e4c2d; the published guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. This is one recipient-copy coordinator path, not a universal TSO-service flow or callback-order guarantee.

## Increment 498 — reviewed directed-interaction publication

Reviewed Directed Interaction diagram #001, “Embedded publication validates, then adds, the producer's pairs,” in GitHub’s full-width Mermaid dialog across upper and lower scroll positions. The chart shows the admitted §5.6 call, object-class and interaction-handle validation, directed-pair rejection with unchanged state, empty-set success without a change, and non-empty merge followed by commit. Labels and connectors were readable without visible overlap or clipped outcomes. Local, candidate-HEAD, and fetched bodies match SHA-256 70654dec24f0d71e12d903a38fcbb980ee8a6c4dd5e05cb7d09093d6ab64139a; the published guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. This remains a selected embedded 2025 implementation path, not a normative transaction guarantee or process-parity claim.

The 55 targeted GitHub reviews now span 19 of 32 guides. The increment 496 source-link result remains 4,776/4,776 references in the worktree and 4,767/4,776 at candidate HEAD; nine references in four in-flight source/test files remain pending their reviewed revisions. Overall progress remains **91%**, a working estimate. Next review Order and Transportation diagram #001, then reconcile the nine source anchors, publish the seven added charts and two revised bodies, and verify hosted hydration for all 515 diagrams. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


## Increment 499 — reviewed order and transportation map

Increment 499 reviewed Order and Transportation diagram #001, “A delivery map: keep the questions separate,” in GitHub’s full-width Mermaid dialog. Upper, middle, and lower views showed separate inputs for service form, sender order, receiver eligibility, and transportation; then RECEIVE/TIMESTAMP delivery paths, time-management gating, and the callback-model branch. The diagram’s caveat keeps dashed links conceptual rather than a complete schedule or normative order table. Labels and connectors were readable without visible overlap or clipped endpoints. Local, candidate-HEAD, and fetched Mermaid bodies match SHA-256 e7d038cfd7eb0f114e1e901d5899b60937408765b07190e66d836b9aed42068f. This is 2025 guide evidence only; it claims neither exact normative sender-to-receiver order mapping nor 2010 behavior or process-profile parity. Increments 497–498 reviewed Time Management #020 and Directed Interaction #001; completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verifies the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 56 targeted chart reviews across 20 guides, including Order and Transportation #001 (increment 499), Time Management #020 (increment 497), and Directed Interaction #001 (increment 498). The increment 496 documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: broaden readability coverage across unsampled 2025 guides; reconcile those nine references after source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.
Next targeted visual review: docs/design/HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md#one-value-from-api-call-to-octets, “One value, from API call to octets” (diagram #001, Mermaid fence line 78; visual-ledger body SHA-256 73c60d992c6d37487e5d4db80bd10f19effecfa1d3c55c583586ceff9ccba3b5). This 2025 guide has no live-browser sample in the current audit. Inspect the GitHub full-width dialog across the chart’s full height, checking the caller-cursor `decodeFrom` route, whole-value trailing-octet rejection, and the distinction between HLA DataElement payloads, public handle encodings, and private process frames. Then reconcile the nine source anchors after the four in-flight source/test files reach reviewed revisions, publish the seven added charts and two revised bodies, verify hosted hydration for all 515 diagrams, and keep evidence gaps bounded.


## Increment 500 — reviewed DataElement API-to-octets flow

Increment 500 reviewed Data Element Encoding diagram #001, “One value, from API call to octets,” in GitHub’s full-width Mermaid dialog. Upper and lower views showed construction/value assembly, distinct encode and decode paths, caller-cursor `decodeFrom`, validation and next-cursor return, and whole-value outcomes: accept fully consumed input, reject trailing octets, or let a parent decode its next nested field. Labels and connectors were readable without visible overlap or clipped endpoints. Local, candidate-HEAD, and fetched bodies match SHA-256 73c60d992c6d37487e5d4db80bd10f19effecfa1d3c55c583586ceff9ccba3b5. The published branch is at e10fa19a7bf48415e00a14ca7301613a84f637a3 and the guide file commit is 36955e0499bb6304321a67d3ebf7954b5ffa25fd. The guide scopes this to the IEEE 1516.1-2025 C++ DataElement API and separates public value encoding from handle encoding and private process framing; no 2010 parity or complete byte-order matrix is inferred. Increments 497–499 reviewed Time Management #020, Directed Interaction #001, and Order and Transportation #001. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verifies the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 57 targeted chart reviews across 21 guides, including Data Element Encoding #001 (increment 500), Order and Transportation #001 (increment 499), Time Management #020 (increment 497), and Directed Interaction #001 (increment 498). The increment 496 documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: broaden readability coverage across unsampled 2025 guides; reconcile those nine references after source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.
Next targeted visual review: docs/design/HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md#state-transitions-commit-first-then-consider-an-advisory, “State transitions: commit first, then consider an advisory” (diagram #001, Mermaid fence line 111; visual-ledger body SHA-256 57f0abe4704a052aaa89e6855c16e29e172298fa6da4b0093bfd4ff930c65829). This 2025 guide has no live-browser sample in the current audit. Inspect its GitHub full-width dialog across the full chart height, checking the per-receiver/object/attribute in-scope transition, unchanged-scope self-loop, and the distinct final-unsubscribe edge. Then reconcile the nine source anchors after the four in-flight source/test files reach reviewed revisions, publish the seven added charts and two revised bodies, verify hosted hydration for all 515 diagrams, and continue coverage across unsampled guides.

## Increment 501 — reviewed receiver-local scope state transitions

Increment 501 reviewed Attribute Scope Advisory diagram #001, “State transitions: commit first, then consider an advisory,” in GitHub’s full-width Mermaid dialog. The complete state chart fit in one default view and remained legible when zoomed. It shows scope entering when an applicable declaration and region path matches, leaving when no applicable path remains, and self-loops when a mutation preserves the current state; the final self-unsubscribe edge is explicitly marked as producing no Out advisory. No overlap or clipped endpoints were visible in the full-chart view. Local, candidate-HEAD, and fetched Mermaid bodies match SHA-256 57f0abe4704a052aaa89e6855c16e29e172298fa6da4b0093bfd4ff930c65829. The published branch is at e10fa19a7bf48415e00a14ca7301613a84f637a3 and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. The guide scopes the final-unsubscribe exception to the reviewed embedded subscription planner and distinguishes it from owner-side or region-side loss; no process-parity or 2010 behavior is inferred. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verifies the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 58 targeted chart reviews across 22 guides, including Attribute Scope Advisory #001 (increment 501), Data Element Encoding #001 (increment 500), Order and Transportation #001 (increment 499), Time Management #020 (increment 497), and Directed Interaction #001 (increment 498). Increment 496's documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: broaden readability coverage across unsampled guides; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted review: Next targeted visual review: docs/design/HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md#1-resolve-a-rate-for-each-receiver-and-attribute, “Resolve a rate for each receiver and attribute” (diagram #001, Mermaid fence line 79; visual-ledger body SHA-256 13b8ef53119b9fcc8656296103e6ae8b98698d1155c8112d8d411ac12918ca96). This 2025 guide has no live-browser sample. Inspect the full chart for the omitted/default-rate path, explicit-rate resolution through the federation FDD, greatest-rate selection, and per-attribute projection. Then continue readability samples across the remaining unsampled 2025 guides. Reconcile the nine source anchors after the four in-flight source/test files reach reviewed revisions, publish the seven added charts and two revised bodies, and verify hosted hydration for all 515 diagrams. Keep the 2010 stream separate; no C++ builds, tests, or CI are part of this documentation pass.

## Increment 502 — reviewed per-attribute rate resolution

Increment 502 reviewed Attribute Update-Rate Admission diagram #001, “Resolve a rate for each receiver and attribute,” in GitHub’s full-width Mermaid dialog. Upper and lower views showed the per-attribute path from a sender passel through eligible receiver attributes, applicable active ordinary and overlapping regional rates, and the decision for an omitted rate. An applicable default designator takes the no-reduction path; otherwise named rates resolve in the federation FDD, the greatest resolved numeric rate is selected when present, and the result returns with per-attribute rates. Labels and connectors were readable without visible overlap or clipped endpoints. Local, candidate-HEAD, and fetched Mermaid bodies match SHA-256 13b8ef53119b9fcc8656296103e6ae8b98698d1155c8112d8d411ac12918ca96. The published branch is at e10fa19a7bf48415e00a14ca7301613a84f637a3 and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. The guide scopes the rate-combination rule to the selected embedded 2025 implementation; it does not establish a universal HLA algorithm, process callback throttling, or 2010 behavior. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verifies the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 59 targeted chart reviews across 23 guides, including Attribute Update-Rate Admission #001 (increment 502), Attribute Scope Advisory #001 (increment 501), Data Element Encoding #001 (increment 500), and Order and Transportation #001 (increment 499). Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: broaden readability coverage across unsampled guides; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted review: Next targeted visual review: docs/design/HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md#switch-state-is-federation-wide, “Switch state is federation-wide” (diagram #001, Mermaid fence line 96; visual-ledger body SHA-256 8fde8fa6e860c81c061969ae9634416c01014f619b1626e141cf273f77f35291). This 2025 guide has no live-browser sample. Inspect the two federation-wide Enabled/Disabled states, the FDD-seeded creation paths, and the HLAsetSwitches transitions; keep the later-discovery effect distinct from changing switch state. Continue readability reviews across unsampled 2025 guides, reconcile the nine source anchors after the four in-flight source/test files reach reviewed revisions, publish the seven added charts and two revised bodies, and verify hosted hydration for all 515 diagrams. Keep 2010 separate; no C++ builds, tests, or CI are part of this documentation pass.


## Increment 503 — reviewed federation-wide Auto Provide switch state

Increment 503 reviewed Auto Provide diagram #001, “Switch state is federation-wide,” in GitHub’s full-width Mermaid dialog. The complete state chart fit in one view; a zoomed view clarified transition labels. It shows the composed FDD seeding the federation-wide HLAautoProvide state as Enabled or Disabled at creation, and HLAsetSwitches changing it between those states. The labels and connectors were readable without visible overlap or clipped endpoints. Local, candidate-HEAD, and fetched Mermaid bodies match SHA-256 8fde8fa6e860c81c061969ae9634416c01014f619b1626e141cf273f77f35291. The published branch is at e10fa19a7bf48415e00a14ca7301613a84f637a3, and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. The neighboring table and prose keep discovery consequences separate: changing the switch does not rediscover existing objects, and later object discovery reads the current value. This is a selected 2025 embedded implementation path, not the per-receiver Attribute Scope Advisory Switch; no process parity or 2010 behavior is inferred. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verifies the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 60 targeted chart reviews across 24 guides, including Auto Provide #001 (increment 503), Attribute Update-Rate Admission #001 (increment 502), Attribute Scope Advisory #001 (increment 501), and Data Element Encoding #001 (increment 500). Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: broaden readability coverage across unsampled guides; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted review: Next targeted visual review: docs/design/HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md#1-connect-validates-first-prepares-then-commits, “Connect validates first, prepares, then commits” (diagram #001, Mermaid fence line 56; visual-ledger body SHA-256 adf2b0435ca29ca31a2b398b2b723e1be2fc010dd49375c86dea8247a8b37488). This 2025 guide has no live-browser sample. Inspect the precondition and failure branches, authorization and preparation gates, and the final locked connection-state commit. Continue readability reviews across unsampled 2025 guides, reconcile the nine source anchors after the four in-flight source/test files reach reviewed revisions, publish the seven added charts and two revised bodies, and verify hosted hydration for all 515 diagrams. Keep 2010 separate; no C++ builds, tests, or CI are part of this documentation pass.

## Increment 504 — reviewed Connect validation and commit flow

Increment 504 reviewed Connect and Configuration diagram #001, “Connect validates first, prepares, then commits,” in GitHub’s full-width Mermaid dialog. The upper and lower scroll views showed callback-reentry and callback-model gates, the AlreadyConnected check, authorization and preparation branches, setup failure, and final locked connection-state commit through ConfigurationResult. Labels and connectors were readable without visible overlap or clipped chart content. The visual-ledger, candidate source, and published body SHA-256 is adf2b0435ca29ca31a2b398b2b723e1be2fc010dd49375c86dea8247a8b37488; branch HEAD is e10fa19a7bf48415e00a14ca7301613a84f637a3 and the guide file commit is 270f302536ddb2f2bd56a35b565d2251a45f03e2. This selected IEEE 1516.1-2025 Umbra path does not claim normative exception priority or test simultaneous-invalid precedence.

The live-browser history now contains 61 targeted chart reviews across 25 of 32 guides. Overall completion remains 91% (working estimate). Remaining work includes readability samples in the seven unsampled guides, reconciliation of nine source anchors after four in-flight source/test files reach reviewed revisions, publication of seven added charts and two revised bodies, hosted hydration verification for all 515 diagrams, and bounded evidence gaps. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

Next targeted visual review: docs/design/HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md#switch-lifetime-federation-policy-not-a-per-send-option, “Switch lifetime: federation policy, not a per-send option” (diagram #001, Mermaid fence line 171; visual-ledger body SHA-256 392f27f4f2dd42258a2cfa820814f389b3401443af1ef862987071f504720ca5). This 2025 guide has no live-browser sample. Inspect the federation-creation default and FOM-supplied switch, then trace the shared switch through query, save, and restore; keep it distinct from a per-send option. Seven guides remain unsampled after this increment. Reconcile the nine source anchors after the four in-flight source/test files reach reviewed revisions, publish the seven added charts and two revised bodies, verify hosted hydration for all 515 diagrams, and keep evidence gaps bounded. Keep 2010 separate; no C++ builds, tests, or CI are part of this documentation pass.


## Increment 505 — reviewed Delay Subscription Evaluation switch lifetime

Increment 505 reviewed Delay Subscription Evaluation diagram #001, “Switch lifetime: federation policy, not a per-send option,” in GitHub’s full-width Mermaid dialog. The full-width dialog showed all seven nodes in one view: the composed FOM switch path, the omitted-switch catalog default of false, federation registry creation and query, and persistence of the switch bit through save and restore. Labels and connectors were readable without visible overlap or clipped chart content. The visual-ledger, candidate source, and published page identify body SHA-256 392f27f4f2dd42258a2cfa820814f389b3401443af1ef862987071f504720ca5. The published branch is at e10fa19a7bf48415e00a14ca7301613a84f637a3, and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. Selected IEEE 1516.1-2025 Umbra federation-level policy path. This chart does not model a per-send option and does not establish 2010 behavior or cross-profile parity. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verifies the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 62 targeted chart reviews across 26 guides, including Delay Subscription Evaluation #001 (increment 505), Connect and Configuration #001 (increment 504), Auto Provide #001 (increment 503), Attribute Update-Rate Admission #001 (increment 502), and Attribute Scope Advisory #001 (increment 501). Six of the 32 guides remain unsampled. Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: broaden readability coverage across the six unsampled guides; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted visual review: docs/design/HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md#a-quick-decision-map, “A quick decision map” (diagram #001, Mermaid fence line 87; visual-ledger body SHA-256 05dd52f1fe0d64d052415640b761c31d875fe2825fdddeea5fc43c78cc7c0694). Inspect the branches for public-service exception reports, rejected MOM requests, and service-invocation records, including their separate eligibility gates; preserve the guide’s cautions that these routes are not universally applicable or mutually exclusive. Six of the 32 guides remain unsampled. Reconcile the nine source anchors after the four in-flight source/test files reach reviewed revisions, publish the seven added charts and two revised bodies, verify hosted hydration for all 515 diagrams, and keep evidence gaps bounded. Keep 2010 and Requirements Lab excluded; no C++ builds, tests, or CI are part of this documentation pass.


## Increment 506 — reviewed Exception Reporting decision map

Increment 506 reviewed Exception Reporting diagram #001, “A quick decision map,” in GitHub’s full-width Mermaid dialog. GitHub’s full-width dialog required upper and lower scroll views. The upper view showed the root decision and three report categories; the lower view showed the separate switch/subscription/recipient gates and the receive-order outcomes. Labels and connectors were readable across the scroll, with no visible overlap or clipped chart content. The visual-ledger, candidate source, and published page identify body SHA-256 05dd52f1fe0d64d052415640b761c31d875fe2825fdddeea5fc43c78cc7c0694. The published branch is at e10fa19a7bf48415e00a14ca7301613a84f637a3, and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. Selected IEEE 1516.1-2025 Umbra reporting paths. The chart is an orientation aid, not universal service coverage; its categories can overlap, and public-service catch behavior is implementation-specific. No 2010 behavior or cross-profile parity is inferred. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verifies the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 63 targeted chart reviews across 27 guides, including Exception Reporting #001 (increment 506), Delay Subscription Evaluation #001 (increment 505), Connect and Configuration #001 (increment 504), Auto Provide #001 (increment 503), and Attribute Update-Rate Admission #001 (increment 502). Five of the 32 guides remain unsampled. Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: broaden readability coverage across the five unsampled guides; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted visual review: docs/design/HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#three-different-meanings-of-valid, “A `basicData` field can fail at different admission gates” (diagram #001, Mermaid fence line 102; visual-ledger body SHA-256 d984a01bb4fe1b6e24cc4668a4cb901e9118709d59b865ba00d78e3433cd61a9). This 2025 guide has no hosted sample. Inspect the independent HLA-prefixed-name composition check, Big/Little Create/Join path, and Middle-endian schema rejection; preserve the chart’s note that these are separate field examples, not sequential checks on one declaration. Five guides remain unsampled. Reconcile the nine source anchors after the four in-flight source/test files reach reviewed revisions, publish the seven added charts and two revised bodies, verify hosted hydration for all 515 diagrams, and keep evidence gaps bounded. Keep 2010 and Requirements Lab excluded; no C++ builds, tests, or CI are part of this documentation pass.


## Increment 507 — reviewed the published FOM creation flow

Increment 507 reviewed the published FOM Module Admission and Composition diagram #002, “Create: prepare the entire definition before reserving its name,” in GitHub’s full-width Mermaid dialog. The upper and lower scroll views showed module admission, 2025 MIM/schema validation, logical-time compatibility, rejection without registry mutation, and successful immutable-definition/name reservation. Labels and connectors were readable while scrolling, with no visible overlap. The diagram body SHA-256 is eff250b2e75ade6c62f547137caafd2765ec04cf8f850d0b51672629bac9e7a0 in the visual ledger, candidate source, and published page; the candidate branch is at e10fa19a7bf48415e00a14ca7301613a84f637a3, and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. The worktree-only diagram #001 is not published, so this review is explicitly of the hosted #002. The chart describes the selected IEEE 1516.1-2025 path; no 2010 behavior or cross-profile parity is inferred. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verified the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 64 targeted chart reviews across 28 guides; four of the 32 guides remain unsampled: Joined-Federate MOM, Name and Handle Resolution, Service Invocation Reporting, and TSO Retraction. Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: review the four unsampled guides; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted visual review: docs/design/HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#join-builds-the-object-before-making-the-member-visible, “Join builds the object before making the member visible” (diagram #001, Mermaid fence line 90; visual-ledger body SHA-256 363ff71ab9e251cea119f79b20f097c5d2d85b960da96c46a3f5e15944659d45). This 2025 guide remains among the four unsampled guides. Inspect provisional membership commit, federation-scoped and per-member MOM state setup, pending report-file cleanup on setup failure, Join transition, subscriber discovery ordering, and callback visibility. Preserve the distinction between federation-scoped HLAfederation state and per-member HLAfederate state. Reconcile the nine source anchors after the four in-flight source/test files are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. Keep 2010 and Requirements Lab excluded; no C++ builds, tests, or CI are part of this documentation pass.


## Increment 508 — reviewed the Joined-Federate MOM Join flow

Increment 508 reviewed Joined-Federate MOM diagram #001, “Join builds the object before making the member visible,” in GitHub’s full-width Mermaid dialog. The upper and lower scroll views showed the 2025 Join admission, distinct federation-scoped HLAfederation and per-member HLAfederate setup, report-file allocation, and captured Join inputs; the setup-failure branch rolls back provisional membership and cleans the pending file, while success completes the ambassador Join before planning eligible-subscriber discovery and queueing discovery/reflection/state update. Labels and connectors were readable across both views, with no visible overlap. The visual-ledger and hosted body SHA-256 is 363ff71ab9e251cea119f79b20f097c5d2d85b960da96c46a3f5e15944659d45; the branch is e10fa19a7bf48415e00a14ca7301613a84f637a3 and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. This is the selected 2025 embedded lifecycle path; it establishes neither process-profile parity nor a normative callback order. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verified the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 65 targeted chart reviews across 29 guides; three of the 32 guides remain unsampled: Name and Handle Resolution, Service Invocation Reporting, and TSO Retraction. Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: review the three unsampled guides; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted hosted visual review: docs/design/HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md#1-resolve-a-model-symbol-only-in-an-execution-context, “1. Resolve a model symbol only in an execution context” (diagram #001, Mermaid fence line 41; visual-ledger body SHA-256 30c27a150a0a003137b58f0d5fcf729db08061e8f2a97ed0019396b5be1c6977). The published branch’s first-chart body matches the current ledger even though the worktree guide has other edits. Inspect connected/joined service gates and the resulting NotConnected, NameNotFound, or resolved-handle outcomes; keep lookup of a model symbol separate from federate-name reservation and class-hierarchy resolution. Three guides remain unsampled. Reconcile the nine source anchors after the four in-flight source/test files are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. Keep 2010 and Requirements Lab excluded; no C++ builds, tests, or CI are part of this documentation pass.


## Increment 509 — reviewed the model-symbol lookup flow

Increment 509 reviewed Name and Handle Resolution diagram #001, “1. Resolve a model symbol only in an execution context,” in GitHub’s full-width Mermaid dialog. The upper and lower views showed distinct connection and federation-membership gates, followed by lookup in the joined model; the outcomes were NotConnected, FederateNotExecutionMember, a typed handle or enum, and a service-specific lookup exception. Text and connectors were readable without visible overlap. The worktree ledger and published body SHA-256 is 30c27a150a0a003137b58f0d5fcf729db08061e8f2a97ed0019396b5be1c6977; the branch is e10fa19a7bf48415e00a14ca7301613a84f637a3 and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. This is a model-symbol lookup path, separate from federation-name reservation and class-hierarchy lookup. No 2010 behavior or cross-profile parity is inferred. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verified the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 66 targeted chart reviews across 30 guides; two of the 32 guides remain unsampled: Service Invocation Reporting and TSO Retraction. Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: review the two unsampled guides; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted hosted visual review: docs/design/HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#the-exact-report-interaction-has-a-switchsubscription-interlock, “The exact report interaction has a switch/subscription interlock” (diagram #001, Mermaid fence line 125; visual-ledger body SHA-256 0c99d6981bf876d7f99c8dc64c3f5547418ae6e3ae195d9a6039325a5493c9bc). The published body matches the current ledger despite other local guide edits. Inspect the Service Reporting switch transitions against an exact HLAreportServiceInvocation subscription; retain the separate active, passive, and regional subscription forms and the two rejection outcomes. Two guides remain unsampled. Reconcile the nine source anchors after the four in-flight source/test files are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. Keep 2010 and Requirements Lab excluded; no C++ builds, tests, or CI are part of this documentation pass.

## Increment 510 — reviewed the Service Reporting interlock

Increment 510 reviewed Service Invocation Reporting diagram #001, “The exact report interaction has a switch/subscription interlock,” in GitHub’s full-width Mermaid view. ReportingOff and ReportingOn and the six labeled transitions were visible. The two rejection loops—enabling while subscribed and subscribing while reporting is enabled—remain distinct, and the safe unsubscribe-then-enable path is clear. Labels and connectors were readable without visible clipping or overlap. The visual-ledger and published body SHA-256 is 0c99d6981bf876d7f99c8dc64c3f5547418ae6e3ae195d9a6039325a5493c9bc; the branch is e10fa19a7bf48415e00a14ca7301613a84f637a3 and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. This reviews the switch/subscription guard only; it does not claim later report-routing behavior or 2010/cross-profile parity. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verified the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 67 targeted chart reviews across 31 guides; one of the 32 guides remains unsampled: TSO Retraction. Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: review the last unsampled guide; reconcile those nine source anchors after the source revisions are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next targeted hosted visual review: docs/design/HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#one-designator-portable-outcomes-and-2025-recipient-gates, “One designator: portable outcomes and 2025 recipient gates” (diagram #001, Mermaid fence line 62; visual-ledger body SHA-256 f3dbefba16688b32f767bbadcf18886d80c723c2e0475923a05e69a1c3a072c2). This is the final unsampled guide. Review the recipient-by-recipient distinction between preventing still-pending delivery and notifying a recipient that already received the original, including the receiving federate’s responsibility for effects already observed. Keep this as an HLA-level orientation, not Umbra queue/callback behavior; detailed normative outcomes remain subject to §§8.22–8.23. One guide remains unsampled. Reconcile the nine source anchors after the four in-flight source/test files are reviewed; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. Keep 2010 and Requirements Lab excluded; no C++ builds, tests, or CI are part of this documentation pass.

## Increment 511 — reviewed the TSO retraction outcome map

Increment 511 reviewed TSO Retraction diagram #001, “One designator: portable outcomes and 2025 recipient gates,” in GitHub’s full-width Mermaid dialog across upper and lower views. Rejected Retract has no successful effect; accepted Retract considers each intended recipient separately. A still-pending copy is prevented from future delivery, while an already-received copy follows the Request Retraction notification path when required. The final nodes preserve differing per-recipient outcomes and the receiving federate’s responsibility for effects already observed. Labels and connectors were readable without visible overlap. The visual-ledger and published body SHA-256 is f3dbefba16688b32f767bbadcf18886d80c723c2e0475923a05e69a1c3a072c2; the branch is e10fa19a7bf48415e00a14ca7301613a84f637a3 and the guide file commit is adafb10c37b6aad8f798e49ca7007abeaaeb66fc. This is an HLA-level orientation for §§8.22–8.23, not an Umbra queue/callback trace or complete normative decision table. No 2010 behavior or cross-profile parity is inferred. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). The 32-guide behavior set and all 515 local diagrams have identity-matched render and screenshot evidence; increment 447 GFM conversion produced 515/515 containers, increment 458 reconciled identities/source lines/hashes, and increment 457 verified the published 508/508 baseline. Seven chart additions and two revised bodies remain local-only. The live-browser history now contains 68 targeted chart reviews across all 32 guides; every guide has at least one sample, but the samples do not cover all 515 diagrams. Increment 496’s documentation link check resolves 4,776/4,776 worktree references across 40 files; candidate HEAD resolves 4,767/4,776, with nine anchors in four in-flight source/test files awaiting reviewed revisions. Remaining work: rerun the source-link check against the current worktree and candidate HEAD; reconcile the nine anchors after inspecting the shared source changes; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next concrete documentation action: rerun quality_gates/bin/check_hla_2025_documentation_links.py --worktree and compare its results with candidate HEAD. The latest full worktree check (increment 496) resolved 4,776/4,776 references across 40 Markdown files; candidate HEAD resolved 4,767/4,776, with nine references into four in-flight source/test files awaiting reviewed revisions. Reconcile only after inspecting those current source diffs, and avoid pinning anchors to code that is still changing. All 32 topic guides now have at least one hosted chart sample (68 samples total), but that is not an all-diagram review. After source references are stable, publish the seven added charts and two revised bodies and verify hosted hydration for all 515 diagrams. Keep the documentation pass 2025-only; no Requirements Lab edits or C++ builds/tests/CI.

## Increment 512 — reran current 2025 source-link checks

Increment 512 reran the 2025 documentation-link checker. All 4,776 unique references across 40 current Markdown files resolve against the worktree (4,776/4,776). Against candidate HEAD, 4,767/4,776 resolve; the same nine line anchors remain unresolved across four in-flight source files: process_federation_service_host.cpp, rti_initialization_data.cpp, ieee1516_2025_connection_process_federation_service_host_catch2.cpp, and tck_connection_lifecycle.cpp. The worktree source links need no correction; the nine HEAD gaps should be reconciled after those shared source/test changes settle. No builds, tests, or CI ran. All 32 2025 guides now have at least one hosted visual sample; 68 samples do not cover all 515 diagrams. Completion remains 91% (working estimate).

Overall progress remains 91% (working estimate, not a fraction derived from gate count). All 32 topic guides have at least one hosted chart sample (68 targeted samples); these do not cover all 515 charts. All 515 local diagrams have identity-matched render and screenshot evidence; the published 508-diagram baseline has a full hydration audit. Seven chart additions and two revised bodies remain local-only. The current documentation-link check resolves 4,776/4,776 unique worktree references across 40 Markdown files; against candidate HEAD it resolves 4,767/4,776, with nine anchors in four in-flight source/test files. Remaining work: reconcile those anchors after the source revisions settle; publish the seven added charts and two revised bodies; verify hosted hydration for all 515 diagrams; and keep evidence gaps bounded. No C++ builds, tests, or CI ran; 2010 and Requirements Lab remain excluded.

Next concrete action: inspect the current diffs for the four in-flight source/test targets listed in candidate_head_reference_validation and record which of the nine stale-at-HEAD anchors will become stable after those changes land. Do not move anchors to provisional lines. The latest worktree check resolves 4,776/4,776 current references; candidate HEAD resolves 4,767/4,776. After the source targets are stable, reconcile the nine anchors, publish the seven added charts and two revised bodies, and verify hosted hydration for all 515 diagrams. Keep this pass focused on IEEE 1516.1-2025; no Requirements Lab edits or C++ builds/tests/CI.

## Increment 525 — mapped embedded ConnectionLost reporting and deferred errors

Reviewed Service Invocation Reporting diagrams #016 and #017 against the embedded registry, membership-loss adapter, file writer, callback dispatcher, deterministic embedded transport, and focused tests. For forced loss, the registry plans the resignation, reserves a final serial only for the file route, suppresses a final service-report interaction reservation, then erases the member and commits the registry transition. The adapter appends while the writer is still live, releases it during cleanup, queues the separate HLAreportFederateLost interaction before automatic-resign consequences, submits the lost member's local callback, and raises a deferred file-append error only after cleanup and submission.

The sequence distinguishes HLA_IMMEDIATE callback execution when the current thread starts or re-enters the drainer from work left queued while another thread owns it. The queued callback can run before or after the deferred error; HLA_EVOKED waits for a later EvokeCallback. The successful file-record test asserts durability before its HLA_EVOKED callback, but a previously recorded local run failed at federation creation line 521 before those assertions; this review did not rerun it. A separate survivor test checks HLAreportFederateLost before automatic object-removal callbacks. No test injects a ConnectionLost append failure, and no process or 2010 parity is claimed.

Mermaid CLI 12.0.0 renders and screenshot reviews confirmed both current diagrams are legible. The full ledger audit also detected stale hashes for Callback and Service Ordering #019 and TSO Retraction #008; both were rendered, reviewed, and reconciled. All 515 current identities now match 515 source fences across 32 guides, with zero line, fence, type, or body-hash mismatches. Type counts are 304 flowcharts, 186 sequence diagrams, and 25 state diagrams.

The source-link checker resolves 4,805/4,805 references against the worktree and 4,796/4,805 against candidate HEAD, primary local HEAD, and the recorded initial-work commit. The same nine anchors in four active source/test files await stable line positions. The 515-chart worktree contains seven additions and nine revised bodies relative to the 508-chart candidate HEAD; hosted hydration remains verified only for those 508 charts. Overall progress remains 91% (working estimate). Next, inspect the four in-flight source/test diffs and reconcile their nine anchors once positions stabilize, then publish the seven additions and 21 revisions and verify hosted hydration for all 515 charts. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 526 — traced worktree-only source anchors to active code changes

The 2025 documentation-link checker resolves 4,805/4,805 references in the current worktree. Candidate HEAD, primary local HEAD, and the recorded initial-work commit each resolve 4,796/4,805; the same nine references are out of range in four active source/test files, with no missing targets. I inspected those diffs and the cited lines. The host citation at line 357 points to the shifted `serveConnection` definition; the RID line 241 is in current timeout-key parsing; five Catch2 anchors identify current host-resume, purge, heartbeat-recovery, recovery-expiry, and history-gap cases; and the two portable-TCK anchors identify the current Federate-Lost report scenario. The connection guide already marks these as worktree citations absent from the hosted base.

The current worktree contains 354 additional/changed lines in the process host, 252 in RID initialization, 1,255 in the focused test file, and 2,376 in the TCK lifecycle source. These are active shared source changes and are not part of the documentation-only publication set. The links resolve against their current sources, so I kept them rather than redirecting readers to unrelated committed code. Updated `HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json` with the current line counts, exact nine anchors, and their pending status. After source owners land reviewed changes, rerun the checker and reconcile any moved lines.

The local diagram inventory remains 515/515 with zero identity, line, fence, type, or hash mismatches. Relative to the 508-chart candidate baseline, seven diagrams are additions and nine existing bodies are revised. Overall progress remains 91% (working estimate). Next handoff: rerun source-link checks after those four source/test changes are committed; then publish the diagrams and verify hosted hydration for all 515. No builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


## Increment 527 — full candidate GFM audit — 2026-11-12

The authenticated GitHub Markdown API parsed all 40 current IEEE 1516.1-2025 documentation pages: 32 topic guides and eight supporting pages. All 515 Mermaid source fences produced 515 GFM Mermaid containers. The 40 responses were nonempty and contained 1,195 headings and 8,700 rendered links, with zero API failures or count mismatches. This verifies GFM parsing and container registration; it does not establish visible hydration on published pages or link click-through. Full hosted hydration remains verified for the 508-diagram published baseline.

The source-link checker resolves 4,805/4,805 references across 40 Markdown files in the worktree. Candidate HEAD, primary HEAD, and initial-work each resolve 4,796/4,805; nine anchors in four active source/test files await stabilization. The local 515-entry diagram ledger remains matched to all 515 source diagrams. Overall progress remains **91% (working estimate; about 9% remains)**. Next, rerun source-link checks after those four active files are reviewed and committed, then publish seven added diagrams and nine revised bodies and verify hosted hydration for all 515. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.


## Increment 528 — reconciled current roadmap status — 2026-11-12

Updated the live progress summary so it reflects the latest verified state instead of the older 39-page GFM pass and two-revision diagram count. The increment-527 GFM audit covers all 40 current 2025 documentation pages and registers 515/515 Mermaid containers; the roadmap and dated progress-history page were rechecked after that increment was recorded. The source-link check remains 4,805/4,805 in the worktree and 4,796/4,805 at candidate HEAD, primary HEAD, and initial-work, with the same nine anchors in four active source/test files. The source-dependency snapshot now records the same line counts as those current files and preserves the pending stabilization note. Overall progress remains 91% (working estimate). Next: rerun the link checks after those source files are reviewed and committed, then publish seven additions and nine revised bodies and verify hosted hydration for all 515. Keep 2010 and Requirements Lab excluded; no C++ builds, tests, or CI ran.


## Increment 529 — reconciled candidate diagram delta — 2026-11-12

A fresh candidate-HEAD comparison found that the live worktree delta had been undercounted. I parsed both backtick- and tilde-fenced Mermaid bodies from candidate HEAD and the worktree, aligned them by guide source and ordinal, and compared the body text after normalizing line endings. The baseline has 508 bodies and the worktree has 515: seven diagrams are additions and 21 existing diagram bodies are actually revised. The 21 revisions are in Data Distribution and Regions (+1), Federation Synchronization (+1), FOM Module Admission and Composition (+12), and Service Invocation Reporting (+7). Additions are Save/Restore (+1), FOM Module Admission and Composition (+1), and Service Invocation Reporting (+5).

Two other ledger SHA differences are not diagram revisions: Callback and Service Ordering #019 and TSO Retraction #008 have identical source bodies at HEAD and in the worktree; the worktree ledger contains their corrected hashes. All seven additions and 21 revised bodies have current local render and screenshot-review records. The public branch remains at e10fa19a with 508/508 hosted diagrams, so all 28 content deltas still need publication and hosted hydration.

The source-link check remains 4,805/4,805 in the worktree and 4,796/4,805 at candidate HEAD, primary HEAD, and initial-work; the same nine anchors in four active source/test files await stabilization. Overall progress remains 91% (working estimate). Next: after those source files are reviewed and committed, rerun source-link checks, then publish seven additions and 21 revised bodies and verify hosted hydration for all 515 diagrams. No C++ builds, tests, or CI ran; keep 2010 and Requirements Lab excluded.
