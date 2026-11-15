# IEEE 1516.1-2025 Documentation Progress — 2026-11-11

This archive records the verified documentation increments for November 11.
The active backlog and current completion estimate remain in the
[documentation roadmap](HLA-2025-DOCUMENTATION-ROADMAP.md).

## Increment 413 — Service Invocation Reporting GFM refresh

The Service Invocation Reporting guide's `GetAttributeName` failure-report
chart was rendered and screenshot-reviewed locally. The full updated guide
converted through GitHub's Markdown API with 13/13 Mermaid containers, 17
headings, and 148 links. Combined with the earlier 510-chart full pass, this
covered all 511 current diagrams for GFM container generation. The new chart
still required hosted-page hydration after publication.

## Increment 414 — Current handoff refresh

The bounded `resume` query selected
`cpp-tck.service-report-federate-object-class-lookups`, for an unknown-object-
class `GetObjectClassHandle` `NameNotFound` failure and its failed service
report. The current candidate source still covers successful lookups only, so
the documentation did not claim the failure path without a matching source
assertion and provenance-bearing result. No C++ build or test was run.

## Increment 415 — Empty class-dimension result in the DDM flow

Data Distribution and Regions diagram #002 now branches on a non-empty versus
empty class-available dimension result. It shows that an empty result is valid
for a dimensionless class, is not a wildcard, and leaves region-dimension
selection as a separate application decision. The adjacent `FoodServed` row
links to its focused 2025 empty-set assertion.

Mermaid CLI 12.0.0 rendered the chart, and the screenshot review found the new
branches readable without clipping. GitHub's public Markdown API converted the
full guide to nonempty HTML with 36/36 Mermaid containers, 47 headings, and 564
links. The changed active roadmap and both progress-history pages also returned
nonempty GFM HTML; the roadmap's Mermaid fence produced its container. The
39-file 2025 documentation link check resolves 4,718/4,718 worktree
references; candidate HEAD and the two other checked commits resolve 4,709,
with the same nine anchors in four in-flight source/test files still unresolved
against committed revisions.

At the increment-415 checkpoint, the last fully hydrated hosted audit remained
508 diagrams at revision
`270f302536ddb2f2bd56a35b565d2251a45f03e2`; five current chart bodies need
hosted verification after publication. Completion remains a 90% working
estimate (about 10% left), with source reconciliation and the final hosted
render audit as the remaining closure work. No C++ build or test was run. Keep
2010 and Requirements Lab excluded.

## Increment 416 — Interaction and parameter lookup report sequence

The Service Invocation Reporting guide now documents the 2025 interaction
class and parameter lookup sequence: four distinct public exceptions, their
failed invocation reports, and the successful lookup/report that follows.
The guide distinguishes `InvalidParameterHandle` from the separate
`InteractionParameterNotDefined` case declared by the standard API, and notes
which exception branches the TCK scenario does not exercise. Its two-track
diagram pairs subject results with the observer's report serials without
claiming callback timing relative to the caught exceptions.

Mermaid CLI 12.0.0 rendered the chart, and screenshot review found both tracks
readable without clipping or connector overlap. GitHub's public Markdown API
returned HTTP 200 and converted the complete guide to nonempty GFM HTML with
14/14 Mermaid containers, 18 headings, and 157 links. The 2025 documentation
link check resolves 4,726/4,726 worktree references; candidate HEAD, primary
local HEAD, and origin/feature/initial-work each resolve 4,717, leaving the
same nine unrelated in-flight source anchors unresolved. The current visual
ledger now has 512 locally rendered and screenshot-reviewed diagrams. Hosted
hydration still awaits publication of the six chart deltas. Completion remains
a 90% working estimate (about 10% left). No C++ build or test was run. Keep
2010 and Requirements Lab excluded.

## Increment 417 — Object-attribute declaration reports

The Service Invocation Reporting guide now documents the nine-record ordinary
object-attribute declaration matrix: five rejected calls and four successful
calls across subscription, attribute-set and whole-class unsubscription, and
two-attribute publication. It explains the report argument distinctions
(empty set versus omitted optional argument, active subscription versus passive
MOM value, and typed Null return) and limits callback-order claims to the
embedded implementation. The package README now includes the final publication
case omitted by its prior coverage summary.

Mermaid CLI 12.0.0 rendered and screenshot-reviewed diagram #008. GitHub's
Markdown API returned HTTP 200 for the full guide with 15/15 Mermaid containers,
19 headings, and 174 links. The 39-file 2025 documentation link check resolves
4,741/4,741 worktree references; candidate HEAD has 4,732 resolved and the
same nine current-only anchors unresolved. The visual ledger now has 513
locally rendered and screenshot-reviewed diagrams. Hosted hydration still
awaits publication of seven chart deltas. Completion remains a 90% working
estimate (about 10% left). No C++ build or test was run. Keep 2010 and
Requirements Lab excluded.

## Increment 418 — Synchronization service-report sequence

The Service Invocation Reporting guide now pairs the subject's seven service
report records with the synchronization outcome. The duplicate-label call
returns normally, so its report has a true invocation-success indicator and an
empty exception; the later registration callback reports failure. The diagram
keeps invocation success separate from registration outcome. Source-derived
callback ordering is bounded by implementation path, and the portable
scenario does not assert report timing or embedded/process parity.

Mermaid CLI 12.0.0 rendered the chart, and screenshot review found both tracks
readable. GitHub's Markdown API returned HTTP 200 for the complete guide with
16/16 Mermaid containers, 20 headings, and 189 links. The 39-file source-link
check resolves 4,752/4,752 worktree references; candidate HEAD, primary local
HEAD, and origin/feature/initial-work each resolve 4,743, leaving the same nine
anchors in four in-flight source/test files unresolved. The ledger now contains
514 locally rendered and screenshot-reviewed diagrams. Eight deltas still
await hosted hydration after publication. The overall estimate remains 90%
complete; this is a working estimate, not a gate-count fraction. No C++ build
or test was run. Keep 2010 and Requirements Lab excluded.

## Increment 419 — Missing object-class lookup report boundary

The Service Invocation Reporting guide now separates the API's declared
`NameNotFound` path for `getObjectClassHandle` from nearby object-lookup
coverage. The existing object/attribute/update-rate scenario reports
`ObjectInstanceNotKnown` for `GetKnownObjectClassHandle` with an invalid
object-instance handle, then checks a successful `getObjectClassHandle` report
at serial 7. It does not call the service with an unknown class name or assert
a failed service report, so that case remains an explicit coverage gap pending
a matching source assertion and provenance-bearing result.

No Mermaid body changed; the 514-entry ledger remains aligned with current
source bodies, lines, and local render/screenshot evidence. GitHub's Markdown
API returned HTTP 200 for the updated Service Invocation Reporting guide with
16/16 Mermaid containers, 20 headings, and 194 links. The 39-file source-link
check resolves 4,757/4,757 worktree references; candidate HEAD, primary local
HEAD, and origin/feature/initial-work each resolve 4,748, leaving the same nine
anchors in four in-flight source/test files unresolved. No C++ build or test
was run. Keep 2010 and Requirements Lab excluded.


## Increment 420 — Embedded and configured-process missing-name lookup

The Service Invocation Reporting guide now includes a source-backed flowchart for getObjectClassHandle with an unknown name. It distinguishes the embedded registry miss from the configured-process rejection and no-handle response, then shows the common public NameNotFound result, best-effort Exception-report attempt, and selected failed-invocation report path. The chart does not claim a report was delivered or establish callback order. The cited portable scenario still does not test this failed-name case.

Mermaid CLI 12.0.0 rendered and screenshot review confirmed the chart is legible. GitHub Markdown API returned HTTP 200 for the full guide with 17/17 Mermaid containers, 20 headings, and 202 links. The 39-file source-link check resolves 4,765/4,765 worktree references; candidate HEAD, primary local HEAD, and origin/feature/initial-work each resolve 4,756, leaving nine anchors in four in-flight source/test files. The ledger now has 515 locally rendered and screenshot-reviewed charts, with nine chart deltas awaiting hosted hydration after publication. The overall estimate remains 90%. No C++ build or test was run. Keep 2010 and Requirements Lab excluded.


## Increment 421 — Link lookup failures into the thematic path

The 2025 thematic reading path for service reports now points directly to the Service Invocation Reporting lookup-failure section. This connects the tested interaction/parameter lookup trace with the separate embedded and configured-process getObjectClassHandle missing-name paths, while retaining the latter failed-report case as a test gap. GitHub Markdown API returned HTTP 200 for the thematic paths with two headings and 168 links; the reporting guide still returns 17/17 Mermaid containers. The 39-file source-link check remains 4,765/4,765 in the worktree. No diagram or C++ source/test changed. The estimate remains 90%; keep 2010 and Requirements Lab excluded.


## Increment 422 — Thematic guide coverage audit

A bounded pass compared the 32 guide-source paths recorded in the 2025 visual ledger with local Markdown targets in the thematic reading paths. All 32 guides are directly linked; no guide is missing. This checks discoverability, not section-anchor correctness or content coverage. The source-link checker remains 4,765/4,765 in the worktree. No diagram, C++ source, or test changed. The estimate remains 90%; keep 2010 and Requirements Lab excluded.


## Increment 423 — Fresh full GitHub GFM guide audit

An authenticated GitHub Markdown API pass converted all 32 current IEEE
1516.1-2025 topic guides. Every response was nonempty HTTP 200 HTML; the 515
source Mermaid fences matched 515 actual Mermaid containers. The rendered
pages contained 814 headings and 7,919 links, with zero API failures or
container-count mismatches. The largest guide input was 279,199 bytes. This
pass covered the topic guides only; it does not establish hosted-page hydration
or link click-through, and the six supporting Markdown pages were not refreshed.
The source-link checker resolves 4,765/4,765 references in the candidate
worktree and 4,756/4,765 at candidate HEAD, leaving the same nine anchors in
four in-flight source/test files. No source, test, or diagram changed. The
working estimate remains 90%; keep 2010 and Requirements Lab excluded.


## Increment 424 — Live GitHub Mermaid hydration sample

The current remote candidate branch head (e10fa19a7bf48415e00a14ca7301613a84f637a3)
was inspected in a signed-in Microsoft Edge session. On the published Time
Management guide, the two Role state machines charts (#002 and #003) visibly
rendered in GitHub Preview. Chart #002 also opened as a full-width rendered
diagram; both remote chart bodies match the visual-ledger hashes. The inline
pan/zoom controls partly overlap #003's right edge, so the full browser audit
should check wide charts in the dialog. This sample covers one section of one
guide; it does not verify all 508 diagrams on the published branch or the
515-diagram worktree. The earlier headless Chromium placeholder result remains
historical and does not describe this Edge session. The updated roadmap and
both progress-history pages also returned nonempty HTTP 200 GFM responses,
with zero Mermaid-count mismatches (25 headings and 141 links combined).
Source links still resolve 4,765/4,765 in the worktree and 4,756/4,765 at
candidate HEAD. No source, test, or diagram changed. The working estimate
remains 90%; keep 2010 and Requirements Lab excluded.


## Increment 425 — Edition-boundary and source-link recheck

The 39-file source-link check still resolves all 4,765 references in the
current worktree. Candidate HEAD and the remote candidate branch remain at
e10fa19a7bf48415e00a14ca7301613a84f637a3; HEAD resolves 4,756 references and
misses the same nine anchors in four in-flight implementation/test files. A
diff-only scan of the nine modified 2025 topic guides found no newly added
`2010`, `1516e`, `IEEE 1516-2000`, or `HLA 1.3` references. This is a bounded
regression screen, not semantic proof of every guide's edition boundary. The
515-diagram candidate remains unpublished; hosted hydration beyond the last
508-diagram audit is still pending, including wide-chart dialog checks. No C++
build, test, or CI ran. The working estimate remains 90%; keep 2010 and
Requirements Lab excluded.

## Increment 426 — Primary-only endian evidence and full diagram-ledger check

The primary-checkout Big/Little Create/Join result artifact records four
passing rows for the public scenario and its contract twin under the same
`installed-package` provider. It has no source revision or executable
identity. The current candidate lacks the positive runner code and its two
fixture files, so the result remains primary-only. The FOM Admission guide
already states this limitation; its chart now labels the outcome
“Primary-only: selected Create/Join accepted.” Mermaid CLI 12.0.0 rendered the
updated chart and screenshot review found it legible. GitHub GFM conversion of
the full guide returned HTTP 200 with 13/13 Mermaid containers.

A full 32-guide hash audit found and corrected one stale visual-ledger hash
for Callback and Service Ordering #019; its current source body matches the
last fully audited hosted revision. The updated FOM Admission #001 and
Callback/Service Ordering #019 now bring the current inventory to 515/515
source-to-ledger hash matches, with no count mismatches. The last hosted audit
still covers 508 diagrams; current hosted hydration remains pending. No C++
build, test, or CI ran. The working estimate remains 90%; keep 2010 and
Requirements Lab excluded.

## Increment 427 — Corrected failed object-class lookup evidence

The Service Invocation Reporting guide now distinguishes the existing portable
TCK scenario from a tracked, Catch2-target-listed embedded test for a missing
`getObjectClassHandle` name. The embedded case expects `NameNotFound` and
compares exact failed serial-0 and successful serial-1 file-report records.
No provenance-bearing run result was found, and the configured-process
missing-name route still has no matching test/result. The guide now records
those limits instead of calling the embedded source assertion missing.

The 2025 documentation link checker resolves 4,768/4,768 worktree references;
candidate HEAD resolves 4,759/4,768, leaving the same nine anchors in four
in-flight source/test files. `git diff --check` completed with only the
existing CRLF advisory in the Federation Lifecycle guide. No C++ build, test,
or CI ran. The working estimate remains 90%; keep 2010 and Requirements Lab
excluded.


## Increment 428 — Reconciled failed-lookup coverage and diagram ledger

The missing-name `getObjectClassHandle` section now distinguishes two
target-listed source cases. The embedded Catch2 case asserts exact failed
serial-0 and successful serial-1 file records. A configured-process matrix
case asserts `NameNotFound` and a failed `HLAreportServiceInvocation` MOM
interaction after observer evocation. Neither case has a located run artifact;
the configured-process file-sink route remains untested. The diagram marks
these as test evidence, separate from its runtime transitions.

Mermaid CLI 12.0.0 rendered and screenshot review confirmed the updated
Service Invocation Reporting #008 and TSO Retraction #008 diagrams. An
authenticated GitHub GFM conversion of the full Service Invocation Reporting
guide returned 17/17 Mermaid containers and 20 headings. A full current-source
ledger audit reconciled ten shifted diagram lines in the Service Invocation
Reporting guide and the stale TSO Retraction #008 body hash; all 515 diagram
counts, source lines, and body hashes now match. The documentation link check
resolves 4,776/4,776 worktree references and 4,767/4,776 at candidate HEAD,
with the same nine anchors in four in-flight source/test files. No C++ build,
test, or CI ran. The working estimate remains 90%; keep 2010 and Requirements
Lab excluded.

## Increment 429 — Reconciled failed-lookup test coverage and live handoff

The Service Invocation Reporting guide no longer implies the embedded Catch2
case is the only source test for a missing-name `getObjectClassHandle`. It now
identifies both target-listed cases: the embedded file-report case asserts the
failed serial-0 and successful serial-1 records; the configured-process matrix
case asserts `NameNotFound` and a failed `HLAreportServiceInvocation` MOM
interaction after observer evocation. Neither case has a located run artifact,
and the configured-process file-sink path remains untested.

GitHub Markdown API GFM conversion of the revised guide returned HTTP 200: 17/17 source Mermaid fences produced Mermaid containers, with 20 headings and 217 links. This checks conversion only, not hosted-page hydration.

The roadmap and both progress-history pages also converted through GFM: 3/3 HTTP 200 responses, 30 headings, 148 links, and zero Mermaid diagrams.

The read-only 2025 source-link check resolves 4,776/4,776 references in the
candidate worktree. Candidate HEAD and its remote branch remain at
`e10fa19a7bf48415e00a14ca7301613a84f637a3`; they resolve 4,767/4,776, with the
same nine anchors in four in-flight source/test files. All eight source links
in the revised section resolve. No C++ build, test, or CI ran. Keep 2010 and
Requirements Lab excluded.

## Increment 430 — Verified a hosted diagram-dialog sample

In the signed-in Edge session, Time Management diagram #001 opened from the
inline GitHub preview into the full-width diagram dialog. Its flow labels and
transitions were readable. The diagram body hash
(`bc6d5f1446acdbaeafb28524e7d355353cae0030348837b9ee30e3afacb5b4d0`) matches
the candidate branch source. Together with the increment-424 sample, the live
sample now covers three charts on the published 508-diagram branch; two were
opened in the dialog and one remains partly obscured by inline controls. This
is a sample, not a full hosted readability review. The 515-diagram worktree is
still unpublished.

The 2025 source-link check again resolves 4,776/4,776 worktree references and
4,767/4,776 at candidate HEAD. The same nine anchors still point into four
in-flight source/test files. `git diff --check` reports only the existing
CRLF advisory in the Federation Lifecycle guide. No C++ build, test, or CI
ran. The working estimate remains 90%; keep 2010 and Requirements Lab
excluded.

## Increment 431 — Recorded the live source-dependency boundary

The hosted source-dependency register now includes a current snapshot for the
candidate branch: all 4,776 source-line references resolve in the worktree;
4,767 resolve at HEAD, leaving nine anchors across two guides. The affected
four source/test files have worktree line counts exceeding HEAD by
308, 172, 1,253, and 2,194 lines respectively. Their changes remain
uncommitted and outside the documentation-only publication set. The two guides
already mark these as current-worktree evidence. The register now directs the
next owner to rerun the link audit after those files are reviewed and committed,
or retarget/remove the citations if the code does not land.

No source files were staged or edited, and no C++ build, test, or CI ran. The
working estimate remains 90%; keep 2010 and Requirements Lab excluded.
## Increment 432 — Rechecked all current 2025 pages through GitHub GFM

GitHub Markdown API POST /markdown converted all 39 current `HLA-2025-*.md`
pages with nonempty HTTP 200 responses. All 515 Mermaid source fences across
the 32 topic guides produced 515 GitHub Mermaid containers; there were no
page or diagram-count mismatches. The conversion contained 1,067 heading
elements and 8,328 rendered links. This confirms GFM parsing and container
registration, not visible hydration or full visual readability of every chart.

The 515-diagram worktree remains unpublished. The 508-diagram hosted baseline
still needs the new 7 additions and 2 revisions published and hydrated. The
nine source anchors in four in-flight files remain unresolved at HEAD. No C++
build, test, or CI ran. The working estimate is now 91%; keep 2010 and
Requirements Lab excluded.

## Increment 433 — Reconciled the roadmap with the current verification state

Updated the current status summaries in the human-readable documentation
roadmap to reflect the increment 433 verification, while leaving dated historical
checkpoints intact. The current 32-guide set remains scoped to IEEE 1516.1-2025;
all 515 diagrams have local render and screenshot evidence. The full authenticated
GFM pass returned nonempty HTTP 200 for all 39 current Markdown pages: all 515
source fences matched Mermaid containers, with 1,068 headings, 8,329 rendered
links, zero mismatches, and zero API failures. The worktree source-link audit
resolves 4,776/4,776 references.

Candidate HEAD still resolves 4,767/4,776 source references. The nine remaining
anchors point into four in-flight source/test files. The remote docs branch is
still at e10fa19a and has no associated pull request; the 515-diagram worktree
remains unpublished against the 508-diagram hosted baseline.

Next: after the four source/test files reach reviewed committed revisions,
rerun the source-link checks, reconcile or retarget the nine anchors, then
publish the candidate and verify Mermaid hydration across all 32 guides and
515 charts. Continue dialog review for wide charts. Do not run C++ builds, tests,
or CI for this documentation work; keep 2010 and Requirements Lab excluded.

## Increment 434 — Opened the Time Constrained state chart in GitHub

In the signed-in Edge session, opened the published Time Management guide's
Time Constrained role state machine (diagram #003) in GitHub's full-width
Mermaid dialog. The state names and transition labels were readable there;
the inline viewer controls had obscured the chart's right edge before opening
the dialog. Its body SHA-256,
`28ef04e0af90da9990389e698a50b49477ea360d58ac035d2adac950de798478`, matches
the chart in published branch revision
`e10fa19a7bf48415e00a14ca7301613a84f637a3`.

This adds a third live readability sample alongside diagrams #001 and #002;
it does not verify every chart in the hosted 508-diagram baseline or the
unpublished 515-diagram worktree. The current estimate remains **91%**. Nine
source anchors still depend on four in-flight source/test files, and the 515
candidate charts still need publication and hosted hydration. No C++ build,
tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 435 — Verified a non-timing chart on hosted GitHub

The signed-in Edge session loaded the published Auto Provide guide and visibly
rendered diagram #001, “Switch state is federation-wide.” The chart uses a
`~~~mermaid` fence; GitHub rendered it inline and opened it in the full-width
diagram dialog. The Enabled/Disabled states and creation and `HLAsetSwitches`
transitions were readable in the dialog. Its body SHA-256,
`8fde8fa6e860c81c061969ae9634416c01014f619b1626e141cf273f77f35291`, matches
both the branch source at `e10fa19a7bf48415e00a14ca7301613a84f637a3` and the
file commit `adafb10c37b6aad8f798e49ca7007abeaaeb66fc`.

This checks one non-timing chart and confirms this tilde-fence instance
hydrates on GitHub; it is not a full readability audit. The prior increment's
Time Constrained chart review remains recorded above. The overall estimate
remains **91%**; source-anchor reconciliation, publication of the 515-chart
worktree, full hosted hydration, and the documented evidence gaps remain open.
No C++ build, tests, or CI ran. Keep 2010 and Requirements Lab excluded.

## Increment 436 — Verified a DDM/regions chart on hosted GitHub

The signed-in Edge session loaded the published Data Distribution and Regions
guide and rendered diagram #002, “From FOM dimensions to runtime region scope,”
from a backtick-fenced Mermaid block. The chart appeared inline and opened in
GitHub’s full-width diagram dialog. Scrolling through the tall flow showed
dimension declarations and class associations, region creation and ranges,
the optional `getDimensionHandleSet(region)` read-back, object and interaction
regional paths, matching and no-match outcomes, and the separate logical-time
eligibility and callback-dispatch gates. The labels were readable throughout.

Its body SHA-256,
`06574ae57df3127a992936b543258cc59eddb2d6ac9e2d7cca8e20416df6b039`, matches
both the published file commit `adafb10c37b6aad8f798e49ca7007abeaaeb66fc` and
candidate branch head `e10fa19a7bf48415e00a14ca7301613a84f637a3`.

This is a targeted readability sample, not a full hosted-diagram review. The
515-diagram worktree remains unpublished against the hosted 508-diagram
baseline. The overall estimate remains **91%**; source-anchor reconciliation,
candidate publication and hosted hydration, and the recorded evidence gaps
remain open. No C++ builds, tests, or CI ran. Keep 2010 and Requirements Lab
excluded.
## Increment 437 — Verified a synchronization flow on hosted GitHub

The signed-in Edge session loaded the published Federation Synchronization
guide and rendered diagram #001, “HLA-level path: registration, reports, and
federation outcome,” from a backtick-fenced Mermaid block. It appeared inline
and opened in GitHub’s full-width diagram dialog. Scrolling through the chart
showed registration acceptance and failure, the registrar result and
participant announcement, per-participant achievement reports, the loop while
reports remain outstanding, and the federation-level completion result. The
final callback-model branch distinguishes `HLA_IMMEDIATE` from `HLA_EVOKED`;
the labels were readable in the dialog.

Its body SHA-256,
`8bbc9d3647df0cd3a3d7e14a5e3f28b77fcaef5b2406f874b7c2aa8ae3e165d5`, matches
both the published file commit `adafb10c37b6aad8f798e49ca7007abeaaeb66fc` and
candidate branch head `e10fa19a7bf48415e00a14ca7301613a84f637a3`.

This is a targeted readability sample, not a full hosted-diagram review. The
515-diagram worktree remains unpublished against the hosted 508-diagram
baseline. The overall estimate remains **91%**; source-anchor reconciliation,
candidate publication and hosted hydration, and the recorded evidence gaps
remain open. No C++ builds, tests, or CI ran. Keep 2010 and Requirements Lab
excluded.

## Increment 438 — Verified a Save/Restore callback chart on hosted GitHub

The signed-in Edge session loaded the published Federation Save and Restore
guide and rendered diagram #002, “Callback delivery crosses two independent
gates,” from a backtick-fenced Mermaid block. It appeared inline and opened
in GitHub’s full-width diagram dialog. The tall flow distinguishes the
embedded adapter path from configured-process transport, the configured
process push-versus-queue choice, convergence at the member callback
dispatcher, and the independent `HLA_IMMEDIATE`/`HLA_EVOKED` dispatch choice.
The complete chart and labels were readable while scrolling the dialog.

Its body SHA-256,
`77d031bb88892033c2d86448b809226c288d31d6b38577e30f077b3b22c5177c`, matches
both file commit `ffeaa4ddffc3b2a59bfdb9c3b85f13479898be1a` and candidate
branch head `e10fa19a7bf48415e00a14ca7301613a84f637a3`.

This is targeted readability evidence, not a full hosted-diagram review. The
515-diagram worktree remains unpublished against the hosted 508-diagram
baseline. The overall estimate remains **91%**; source-anchor reconciliation,
candidate publication and hosted hydration, and the recorded evidence gaps
remain open. No C++ builds, tests, or CI ran. Keep 2010 and Requirements Lab
excluded.

## Increment 439 — Reviewed a callback exception flow on hosted GitHub

The signed-in Edge session loaded the published Callback and Service Ordering
guide and rendered diagram #016, “Background dispatch runs without an active
request,” from a backtick-fenced Mermaid block. It rendered inline and opened
in GitHub’s full-width diagram dialog. The seven participant lanes, the
dispatch-loop path that saves and later rethrows the first callback exception,
the separate existing-immediate-drainer path, and the HLA_EVOKED path were
readable while scrolling the dialog.

Its body SHA-256,
`98e06f655801be90b2efcc72cbfcf871e74882534718cd868415a2d6a001df52`, matches
both file commit `adafb10c37b6aad8f798e49ca7007abeaaeb66fc` and candidate
branch head `e10fa19a7bf48415e00a14ca7301613a84f637a3`.

This is targeted readability evidence, not a full hosted-diagram review. The
515-diagram worktree remains unpublished against the hosted 508-diagram
baseline. The overall estimate remains **91%**; source-anchor reconciliation,
candidate publication and hosted hydration, and the recorded evidence gaps
remain open. No C++ builds, tests, or CI ran. Keep 2010 and Requirements Lab
excluded.

## Increment 440 — Reviewed an interaction delivery chart on hosted GitHub

The signed-in Edge session loaded the published Object and Interaction
Information Flow guide and rendered diagram #020, “The delivery route carries
that captured view to the callback,” from a backtick-fenced Mermaid block. It
rendered inline and opened in GitHub's full-width diagram dialog. The six
participant lanes show the Pull route (`HLA_EVOKED`, push disabled) and Push
route (`HLA_IMMEDIATE`, push enabled) delivering the already captured class and
parameter view through separate queue or stream paths to the callback
dispatcher. The diagram explicitly notes that its later receiver
`getInteractionClassHandle` fence does not establish callback-before-sender-send-
return timing; all lanes, branches, and the limitation were readable.

Its body SHA-256,
`16428320f442d97b399c16e6d894a860b86c03b530cf376626f3f99a25c37adb`, matches
file commit `adafb10c37b6aad8f798e49ca7007abeaaeb66fc` and branch head
`e10fa19a7bf48415e00a14ca7301613a84f637a3`.

This is targeted readability evidence, not a full hosted-diagram review. The
515-diagram worktree remains unpublished against the hosted 508-diagram
baseline. The overall estimate remains **91%**; source-anchor reconciliation,
candidate publication and hosted hydration, and the recorded evidence gaps
remain open. No C++ builds, tests, or CI ran. Keep 2010 and Requirements Lab
excluded.


## Increment 441 — Reviewed the authorization gate on hosted GitHub

The signed-in Edge session loaded the published Authorization Flow guide and
rendered diagram #001, “Authorization is a gate, not the service result,” from
a backtick-fenced Mermaid block. It rendered inline and opened in GitHub's
full-width diagram dialog. The chart shows the three covered callback scopes
(Connect, Create/Destroy, and Join), the outside-interface path for other
services, the three AuthorizationResult outcomes, and the later
service-specific precondition and result branch after AUTHORIZED. The labels
and branches were readable.

Its body SHA-256,
15ba7f3f2f1edf81df19f75255c32afbf8b124be783b49c764587a8b3a719752, matches
the hosted file at commit
36955e0499bb6304321a67d3ebf7954b5ffa25fd and candidate branch head
e10fa19a7bf48415e00a14ca7301613a84f637a3.

This is targeted readability evidence, not a full hosted-diagram review. The
515-diagram worktree remains unpublished against the hosted 508-diagram
baseline. The overall estimate remains **91%**; source-anchor reconciliation,
candidate publication and hosted hydration, and the recorded evidence gaps
remain open. No C++ builds, tests, or CI ran. Keep 2010 and Requirements Lab
excluded.


## Increment 442 — Reviewed ownership and update-region boundaries on hosted GitHub

The signed-in Edge session loaded the published Attribute Ownership Flow guide
and rendered diagram #002, “Ownership transfer and update-region association
are separate boundaries,” from a backtick-fenced Mermaid block. It rendered
inline and opened in GitHub's full-width dialog. The separate current-owner,
prospective-owner, and regional-receiver lanes were readable. The flow shows
R-B as deferred while A owns the attribute, the optional removal of that
association, ownership transfer, and updates using R-B only if it remains;
otherwise updates use the default source region. This is a selected embedded
2025 test flow, not a claim of process-profile parity or a universal timing
rule.

The raw Mermaid-body SHA-256
df654c67e1f978a40e8dc01a17000d35ae2e07107908f836cfcf6ee3f35845ec matches
the hosted file at commit 381af28c46040ee55a902f06417a2268a58bec61 and
candidate branch head e10fa19a7bf48415e00a14ca7301613a84f637a3.

This is targeted readability evidence, not a full hosted-diagram review. The
515-diagram worktree remains unpublished against the hosted 508-diagram
baseline. The overall estimate remains **91%**; source-anchor reconciliation,
candidate publication and hosted hydration, and the recorded evidence gaps
remain open. No C++ builds, tests, or CI ran. Keep 2010 and Requirements Lab
excluded.


## Increment 443 — Reviewed the synchronization callback sequence on hosted GitHub

The signed-in Edge session loaded the published Callback and Service Ordering
guide and rendered diagram #025, “Process endpoint: service responses and
synchronization events are separate,” from a backtick-fenced Mermaid block. It
rendered inline and opened in GitHub's full-width dialog. While scrolling, its
five lanes (App, Adapter, Client, Service, and Dispatcher) remained readable.
The registration and final-achievement cycles distinguish pushed requestId-0
events from queued polling outcomes, and show the still-pending barrier branch.
They also distinguish HLA_IMMEDIATE inline invocation from HLA_EVOKED queuing
and keep service-report requests separate from the synchronization callbacks.
The guide bounds this as 2025 implementation-profile behavior, not a normative
total-order rule.

The raw Mermaid-body SHA-256
813d40a360de2e729d834f7a9ab5f7bd562e5d22ce817931f327dbe376fb657c matches
the hosted file at commit adafb10c37b6aad8f798e49ca7007abeaaeb66fc and
candidate branch head e10fa19a7bf48415e00a14ca7301613a84f637a3.

This is targeted readability evidence, not a full hosted-diagram review. The
515-diagram worktree remains unpublished against the hosted 508-diagram
baseline. The overall estimate remains **91%**; source-anchor reconciliation,
candidate publication and hosted hydration, and the recorded evidence gaps
remain open. No C++ builds, tests, or CI ran. Keep 2010 and Requirements Lab
excluded.

## Increment 444 — diagram-ledger hash reconciliation

Rechecked every Mermaid body in the 32 IEEE 1516.1-2025 topic guides against the 515-entry visual-review ledger. The first pass found two stale hashes in the working ledger: Callback and Service Ordering #019 and TSO Retraction #008. Mermaid CLI 12.0.0 rendered both current bodies to temporary PNGs; visual inspection confirmed the nested immediate-drainer, queued-callback, and callback-exception branches in #019, and the ownership-change, timestamp, and recipient-eligibility branches in #008. Updated the two ledger hashes to match the source. The final comparison found 515/515 body hashes matching. This confirms local rendering only; hosted hydration remains at 508/508 until candidate publication. Fresh source-link checks resolve 4,776/4,776 in the worktree and 4,767/4,776 at candidate HEAD, primary local HEAD, and origin/feature/initial-work; the same nine anchors in four in-flight files remain. Publication, hosted verification, and the recorded evidence gaps are still open. Overall progress stays 91%. No C++ build, tests, or CI ran; 2010 and Requirements Lab remain excluded.

## Increment 445 — checked a hosted 2025 diagram in GitHub

The signed-in Edge session opened the published Object-Instance Name
Reservation guide. GitHub rendered its first Mermaid flow inline. The screenshot
showed the request node, admissibility decision, and the start of both outcome
branches; the lower portion remained below the viewport, so this is a render
smoke check rather than a full readability review or dialog inspection. The
diagram is #001, “HLA-level lifecycle in one view”; its candidate-source ledger
SHA-256 is
`096777b1f056396b7e7a37752e41718271a29ade738d3a30e4bcd699f2bf972e`.

This targeted observation does not extend the full hosted hydration audit:
that still covers 508/508 diagrams, while the 515-diagram worktree remains
unpublished. The nine committed-revision source anchors and documented
evidence gaps remain. Overall progress stays **91%**. No C++ build, tests, or CI
ran; keep 2010 and Requirements Lab excluded.

## Increment 446 — reviewed the reservation flow in GitHub's dialog

Opened Object-Instance Name Reservation diagram #001 in GitHub's full-width
Mermaid dialog and scrolled through the complete chart. The admission gate,
synchronous exception, reservation success/failure outcomes, later named
registration, and distinct owner-handle and eligible-peer discovery branches
remained readable. Candidate HEAD's Mermaid-body SHA-256
`096777b1f056396b7e7a37752e41718271a29ade738d3a30e4bcd699f2bf972e` matches
the published chart.

The 2025 source-link checker still resolves 4,776/4,776 references in the
worktree; `--ref HEAD` resolves 4,767/4,776, leaving nine anchors in four
in-flight C++ source/test files. The 515-diagram candidate remains unpublished,
and full hosted hydration remains 508/508. Overall progress stays **91%**. No
C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 447 — refreshed full 2025 GitHub GFM coverage

Refreshed authenticated GitHub Markdown API conversion for all 39 current
IEEE 1516.1-2025 Markdown pages: 32 topic guides and seven supporting pages.
Every response was nonempty, and all 515 Mermaid source fences produced 515
`section[data-type="mermaid"]` containers. The responses contained 1,082
headings and 8,371 rendered links; there were zero API failures or diagram-count
mismatches. The largest input was 279,199 UTF-8 bytes.

This verifies GFM parsing and Mermaid container registration only. The full
hosted hydration audit remains 508/508 diagrams on the older published
revision; the 515-diagram candidate is still unpublished. Source links resolve
4,776/4,776 in the worktree and 4,767/4,776 at candidate HEAD, leaving nine
anchors in four in-flight source/test files. Overall progress stays **91%**.
No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 448 — reviewed the save-resignation sequence in GitHub

Microsoft Edge opened Federation and Federate Lifecycle diagram #022, “A
resignation during an active save fails the save for survivors,” on the hosted
candidate branch at `e10fa19a7bf48415e00a14ca7301613a84f637a3`. The diagram
rendered inline and opened in GitHub’s full-width Mermaid dialog. Its four
participant lanes and complete sequence remained readable: Resign from inside
the active callback is rejected; retry after callback return removes the
departing member and fails the save for survivors; surviving members receive
`FEDERATE_RESIGNED_DURING_SAVE`; the departing member receives no stale
save-failure callback; and a later save succeeds. The chart explicitly bounds
this to one embedded 2025 `HLA_EVOKED` schedule, not a global callback-order
rule.

The raw Mermaid-body SHA-256
`f5991e78a4bd7e01ffd86de16f337b7d37bd0e6c0374dfc7411376f8c0969537` matches
both the candidate branch source and the local visual-review ledger. This is a
targeted dialog sample only. Full hosted hydration remains 508/508 on the older
revision, and the current 515-chart candidate remains unpublished against that
baseline. The nine source anchors and documented evidence gaps remain open;
overall progress stays **91%**. No C++ build, tests, or CI ran; keep 2010 and
Requirements Lab excluded.

## Increment 449 — reviewed the pushed-interaction Evoke sequence in GitHub

Microsoft Edge opened Callback and Service Ordering diagram #005, “A pushed
interaction can arrive during a single Evoke wait,” on the hosted candidate
branch at `e10fa19a7bf48415e00a14ca7301613a84f637a3`. The chart rendered inline
and opened in GitHub’s full-width Mermaid dialog. All six participant lanes
and the full sequence remained readable while scrolling. The selected
process-profile trace separates the pre-wait receive-order poll, an event
buffered during the wait, the receive-order temporal gate, conditional
post-wait submission and `callbackInvoked` result, contention for the client
dispatch lease, and the optional zero-time continuation that runs at most one
task. This is implementation-profile behavior, not an HLA-wide callback-order
rule.

The raw Mermaid-body SHA-256
`b2bc3c4e9e936502da89eb13ff39f3f5140acdd1a0071f12eb103ec62afbfe17` matches
both the candidate branch source and the local visual-review ledger. This is a
targeted dialog sample only. Full hosted hydration remains 508/508 on the older
revision, and the current 515-chart candidate remains unpublished against that
baseline. Source-anchor reconciliation and documented evidence gaps remain
open; overall progress stays **91%**. No C++ build, tests, or CI ran; keep 2010
and Requirements Lab excluded.

## Increment 450 — reviewed the queued-deletion restore sequence in GitHub

Microsoft Edge opened Federation Save/Restore diagram #028, “Worked case: queued timestamped object deletion returns after fresh-registry restore,” on the hosted candidate branch at `e10fa19a7bf48415e00a14ca7301613a84f637a3`. The diagram rendered inline and opened in GitHub’s full-width Mermaid dialog. Its four participant lanes and full sequence remained readable while scrolling: an object deletion scheduled for time 9 remains pending while a save requested at time 7 completes; the saved image retains the object and deletion; a fresh registry is created within the test process and restores the image; the pending deletion is rebound to the restored object and current recipient routes; later time advances deliver the time-9 removal callbacks, after which both receivers report the object instance as unknown.

The chart identifies the embedded 2025 development profile and the exercised HLA_EVOKED and HLA_IMMEDIATE callback models. This is a fresh-registry durable-image test inside one process, not an operating-system process restart and not a general callback-order rule. The raw Mermaid-body SHA-256 `c7ccde58319883d8c216e3c4e9898399a3c6b1589db6ad962692d0eaf4313ad9` matches the candidate branch source and local visual-review ledger. Full hosted hydration remains 508/508 on the older revision, and the current 515-chart candidate remains unpublished against that baseline. The nine source anchors and documented evidence gaps remain open; overall progress stays **91%**. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 451 — reviewed FQR grant publication with outstanding TSO ACK

Microsoft Edge opened Process Endpoint Request and Callback diagram #020, “FQR publishes FQG without waiting for the TSO completion ACK,” on the hosted candidate branch at `e10fa19a7bf48415e00a14ca7301613a84f637a3`. The chart rendered inline and in GitHub’s full-width Mermaid dialog. Its five lanes remained readable while scrolling: the coordinator selects TSO delivery and leaves the recipient entry inTransit; commits actual grant and optimistic floor; then emits FQG after the TSO event frame. The HLA_EVOKED path queues Receive Interaction and FQG in stream order; a normal return triggers the private delivery ACK, while a thrown callback consumes the current task, skips ACK, leaves the recipient inTransit, and retains FQG for a later Evoke. The HLA_IMMEDIATE path submits Receive Interaction inline; a normal return ACKs before FQG submission, while a throw stops extraction before the later FQG, stores the exception, and permits a later pump to dispatch FQG. The optional Resign/ConnectionLost path unregisters the recipient and discards queued and inTransit TSO state.

The guide explicitly labels both callback-throw interleavings source-derived and not focused tested behavior. This is configured-process implementation evidence, not a normative cross-member callback-order rule. The raw Mermaid-body SHA-256 `d9b9247fddff374a3578bd9cb8023a482f075b1d0107d26c90fdcc083f8849cc` matches the candidate branch source and visual ledger. Full hosted hydration remains 508/508 on the older revision; the 515-chart candidate is unpublished, the nine source anchors and documented evidence gaps remain open, and the working estimate stays **91%**. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 452 — reviewed the single-name reservation trace on GitHub

Microsoft Edge opened Object-Instance Name Reservation diagram #002, “1. Single-name reservation: synchronous admission, callback outcome,” on the hosted candidate branch at e10fa19a7bf48415e00a14ca7301613a84f637a3. It rendered inline and opened in GitHub’s full-width Mermaid dialog. All five participant lanes and the complete 52-line sequence remained readable while scrolling, with no clipping or overlap observed. The trace shows synchronous rejection without an outcome callback, accepted success and conflict outcomes, the HLA_IMMEDIATE and HLA_EVOKED delivery branches, and the subsequent registration check. The release-before-callback branch is explicitly an embedded-source observation: a submitted callback task remains pending, delivery rechecks membership rather than current reservation ownership, and a later named registration can fail after release.

The raw Mermaid-body SHA-256 2fa7627f93dae3a69c12444e571462397445031300cf931a06854f4646c1a11c matches the candidate branch source and local visual-review ledger. This is a targeted 2025 readability sample; implementation interleavings are not HLA-wide architecture or callback-order requirements. The older 508/508 hosted hydration baseline, unpublished 515-diagram candidate, nine source anchors, and documented evidence gaps remain open. Overall progress stays 91%. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 453 — reviewed the background callback exception sequence in GitHub

The GitHub page rendered Callback and Service Ordering diagram #016 inline and in
the full-width Mermaid dialog. All seven participant lanes and the three
exception paths remained readable across the top and lower portions while
scrolling, with no clipping or overlap. The sequence distinguishes an
HLA_IMMEDIATE exception on the dispatchLoop-owned drainer, which is saved and
re-thrown before the next process request acquires its lock or sends; an
exception from a drainer owned by another thread, which exits that drainer's
call path; and an HLA_EVOKED exception, which exits Evoke Callback. These are
source-derived 2025 configured-process behaviors, not normative HLA
callback-order or exception rules.

The raw Mermaid-body SHA-256
98e06f655801be90b2efcc72cbfcf871e74882534718cd868415a2d6a001df52 matches
candidate branch e10fa19a7bf48415e00a14ca7301613a84f637a3 and the visual
ledger. This is one targeted hosted sample. The older hosted hydration audit
remains 508/508 diagrams; the unpublished candidate contains 515 diagrams.
Nine candidate-only source anchors in four in-flight files and the documented
evidence gaps remain open. Overall progress stays 91%. No C++ build, tests, or
CI ran; keep 2010 and Requirements Lab excluded.

## Increment 454 — reviewed the save request and commit barrier in GitHub

The Federation Save/Restore guide’s “Save flow: request, participant work,
and the commit barrier” sequence rendered inline and opened in GitHub’s
full-width Mermaid dialog. All six participant lanes and the upper and lower
portions remained readable while panning, with no clipping or overlap. The
flow distinguishes accepted request from initiation, each participant’s
local checkpoint from the RTI’s durable federation-image commit, and successful
commit from participant or storage failure outcomes.

The raw Mermaid-body SHA-256
e0e496cf9b5b66e9409026b69c25c350788ada4294229aa1a8362f8d138d4a5d matches
candidate branch e10fa19a7bf48415e00a14ca7301613a84f637a3 and its visual
ledger. This is a targeted 2025 readability sample, not a normative
callback-order rule. The older hosted hydration audit remains 508/508; the
515-chart candidate is unpublished, and the nine source anchors and documented
evidence gaps remain open. Overall progress stays 91%. No C++ build, tests, or
CI ran; keep 2010 and Requirements Lab excluded.

## Increment 455 — reviewed the combined exception-report flow in GitHub

The Exception-Reporting guide’s “When both process report routes apply”
sequence rendered inline and opened in GitHub’s full-width Mermaid dialog. Its
six caller, API, exception-hook, process-client, process-service, and observer
lanes remained readable while panning across the upper and lower report paths,
with no clipping or lane overlap. The flow separates the HLAreportException
request from the later HLAreportServiceInvocation request, including the
success and failure outcomes of the latter.

The raw Mermaid-body SHA-256
3df0edafa7680ebfa709247091925b557dbcb93df5a05431ea20947a07cf6356 matches
candidate branch e10fa19a7bf48415e00a14ca7301613a84f637a3 and its visual
ledger. The guide labels the combined two-switch scenario source-derived and
untested; this is visual evidence only. The older hosted hydration audit remains
508/508, the 515-chart candidate is unpublished, and source-link and documented
evidence gaps remain open. Overall progress stays 91%. No C++ build, tests, or
CI ran; keep 2010 and Requirements Lab excluded.

## Increment 456 — reviewed inherited dimensions in a regional path

The [Data Distribution and Regions guide’s “Inherited dimensions in a child-class regional path” chart](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#inherited-dimensions-in-a-child-class-regional-path) rendered inline and opened in GitHub’s full-width Mermaid dialog. The tall vertical flow remained readable from its hierarchy/query setup through the lower expected-result branches while scrolling, with no clipping or overlap.

The observed path shows the pinned Restaurant FOM hierarchy: Food.Drink declares BarQuantity, Soda adds SodaFlavor, and Light adds Sweetener. The query for Light returns all three dimensions. The test then attempts a regional subscription and fails there, before registration, discovery, or reflection; its overlap and callback branches remain authored expectations, not observations. The guide records the focused test’s 21 passing assertions and one failed subscription assertion, and explicitly leaves parent-keyed subscription against a child-registered object untested.

Raw Mermaid-body SHA-256 4647075d03a4538e2ab8e776843c97d4e9ea09640b4472df52f15d2db949f0f0 matches the hosted candidate source and local visual-review ledger. This is a targeted visual review only; it does not close the nine source anchors, publication, full 515-chart hosted hydration, or documented evidence gaps. The working estimate remains **91%**. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 457 — rechecked hosted hydration for the published 2025 branch

Navigated all 32 IEEE 1516.1-2025 topic-guide pages on the candidate branch and scrolled through each page to trigger GitHub's lazy Mermaid rendering. At the published branch head e10fa19a7bf48415e00a14ca7301613a84f637a3, all 508 tracked diagrams appeared as 508 Mermaid output containers with 508 loaded GitHub Render accessibility documents; every page matched its tracked diagram count. The remote branch pointer was confirmed with a read-only git ls-remote. This pass verifies visible render-document hydration; it did not collect individual network HTTP status codes or review every chart's full-width readability.


## Increment 458 — reconciled the current diagram ledger

Parsed the Mermaid fences in all 32 current IEEE 1516.1-2025 guide sources and matched each one to the visual-review ledger by source and ordinal. The inventory contains 515 diagrams, with no missing or extra entries. Ten Service Invocation Reporting entries (#008–#017) had opening-line pointers seven lines behind the current source; their ledger lines now point to the actual fences. Callback and Service Ordering #019 and Data Distribution and Regions #006 had stale body hashes. Both current bodies were freshly rendered with Mermaid CLI 12.0.0 and visually inspected; labels and branches remained readable without clipping or overlap. Their ledger hashes now match the source.

The reconciled ledger has 515/515 matching identities, opening lines, and normalized body hashes. This does not publish or expand hosted verification: published HEAD e10fa19a remains at 508/508 hydrated charts, while seven additions and two revised bodies remain in the worktree. Nine source anchors in four in-flight source/test files still fail at candidate HEAD. The working estimate remains 91%. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 459 — reviewed the surviving-candidate restore flow on GitHub

Opened the published Ownership Persistence and Resignation guide. After scrolling, all four charts on the page had hydrated GitHub Render documents. The “Remove one candidate without discarding the surviving recipient” sequence was opened in the full-width Mermaid dialog and reviewed across its five lanes at default and increased zoom. Its cancellation, surviving If-Available candidate, Confirm Divestiture, single saved notification, fresh-registry restore, and unchanged regular ownership remained readable while scrolling, without clipping or overlap.

The raw Mermaid-body SHA-256 `043b1d92b3db609769428a18171447640cb2346cdc8870ff3e54160b0a349c8d` matches the local source and visual ledger; the guide is unchanged from published candidate HEAD e10fa19a. This is one selected embedded fresh-registry case, not a complete ResignAction matrix, arbitration/fairness rule, process restart, or normative guarantee. It does not change the 91% estimate or close the nine candidate-only source anchors, publication, or full 515-chart hosted verification. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 460 — reviewed Federation MOM synchronization queries on GitHub

Opened Federation MOM diagram #027, “Federation synchronization queries: active
point list and roster status,” on the published candidate branch. The diagram
rendered inline and in GitHub’s full-width Mermaid dialog. Scrolling from the
request and MIM-role framing through both query branches to the queued report
and receiveInteraction callback showed readable labels and arrows, with no
clipping or overlap observed in the reviewed sections. The active-point query
returns labels; the named-point query preserves an unknown label with an empty
roster or snapshots that point’s federate/status records. The configured-process
branch is explicitly the generic send path and constructs no paired report.

Raw Mermaid-body SHA-256
285e8235307e7e416db26031a9ec5aa44540c90d0ec9524aea39e58eac6738bc matches
the hosted candidate source and visual ledger. This is targeted visual evidence
for the documented 2025 implementation profile, not normative completeness or
embedded/process parity. The 515-chart candidate remains unpublished; nine
source anchors, publication, full hosted hydration, and documented evidence
gaps remain open. Overall progress stays **91%**. No C++ build, tests, or CI
ran; keep 2010 and Requirements Lab excluded.

## Increment 461 — reviewed the embedded subscription-change flow on GitHub

Opened Object Attribute Subscription State diagram #009, “Embedded discovery
reentrancy precedes initial-value planning,” on the published candidate branch.
It rendered inline and in GitHub’s full-width Mermaid dialog. The receiver-known
split, discovery and initial-value path, known-object scope advisory, unresolved
current-value-only branch, and separate provider-advisory outcomes remained
readable while scrolling and panning at default and increased zoom, with no
clipping or overlap observed in the reviewed sections.

Raw Mermaid-body SHA-256
3efb4ec2d98147bec5004f7e1b526666781f7185d5b29b70ab17257048dccc0a matches
the published candidate source and visual ledger. The sequence is an
embedded-source checkpoint, not tested reentrancy behavior: the cited tests do
not re-enter RTI services from discovery callbacks. The exact licensed §5.8
rule and process-profile equivalence remain open. This targeted visual sample
does not alter the 91% working estimate or close the source-anchor, publication,
full 515-chart hosted-verification, or evidence-gap work. No C++ build, tests,
or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 462 — reviewed MOM interaction-send reports on GitHub

Opened MOM Object and Interaction Reports diagram #008, “MOM interaction-send reports separate total sends from the directed subset,” on the published 2025 candidate branch. It rendered inline and in GitHub’s full-width Mermaid dialog. Scrolling through the request split, accepted-send accounting, separate total and directed ledgers, and report-delivery path showed readable labels and connectors in the reviewed portions, with no visible overlap. The chart distinguishes total accepted sends from the directed-only subset and keeps the two request/report paths separate.

Raw Mermaid-body SHA-256 21802d118f96c7930dad53f36e1442ea72d3b97f7ef66fdad70bb322606a8f76 matches the published candidate source and visual ledger. The focused HLA_EVOKED observations see reports after callback draining; this targeted visual review does not establish normative completeness or behavior parity across implementation profiles. The 515-chart candidate remains unpublished; nine source anchors, full hosted verification, and documented evidence gaps remain open. Overall progress remains **91%**. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 463 — reviewed the subscription relevance fan-out on GitHub

Opened Relevance Advisory diagram #001, “One object subscription, separate calculations,” on the published 2025 candidate branch. It rendered inline and in GitHub’s full-width Mermaid dialog. The overview labels are small and the full chart extends beyond one viewport; zooming and panning made the reviewed regions readable. The subscription trigger and commit lead to separate publisher/class relevance, known-instance owner relevance and receiver-scope decisions, discovery, and a later value-delivery path. No branch overlap was visible in the reviewed regions.

Raw Mermaid-body SHA-256 e678176b24d943e75873b5c960b979e7f06013f893437b1e7fe5c9c7cbc319b7 matches the published candidate source and visual ledger. The guide explicitly bounds its class-level callback path to embedded 2025 evidence, keeps process-profile evidence separate, and makes no parity or universal callback-order claim. This targeted review does not verify the other charts on the guide or the unpublished 515-chart candidate. Overall progress remains **91%**. The source-anchor, publication, full hosted-verification, and documented evidence-gap work remains open. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.

## Increment 464 — reviewed the solicitation and later-delivery boundary on GitHub

Opened Attribute Value Update Request diagram #001, “1. HLA-facing model:
solicitation and value delivery are distinct,” on the published 2025 candidate
branch. It rendered inline and in GitHub’s full-width Mermaid viewer. The
request, optional provider callback, optional later value-bearing Update, and
receiver’s separate receive-order and timestamped reflection branches remained
readable across the upper and lower portions while scrolling. The four role
lanes and message labels were traceable, with no cropped labels or ambiguous
connector path observed.

Raw Mermaid-body SHA-256
742e55f1bbbbfabd4e03b398b35859812192452ce003f1dca6dcaa39bf36e2de matches the
published candidate source at e10fa19a7bf48415e00a14ca7301613a84f637a3 and the
visual ledger. The surrounding guide describes this as a causal model rather
than a required RTI call sequence or provider-callback-before-return guarantee;
requester entitlement and the normative §6.21 interpretation remain open.
This targeted review leaves the working estimate at **91%** and does not close
the source-anchor, publication, full hosted-hydration, or evidence-gap work.
No C++ build, tests, or CI ran; keep 2010 and Requirements Lab excluded.
