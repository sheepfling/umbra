# IEEE 1516.1-2025 Documentation Progress — 2026-11-15

## Increment 530 — finish-gate and citation refresh

The working completion estimate remains **91%**. This is a roadmap estimate, not a score derived from the number of files or diagrams.

### Verified state

- The 32-guide worktree contains 515 Mermaid diagrams. All 515 ledger entries match their source guide, fence type, source line, and normalized body hash. The ledger records local render and screenshot-review evidence for all 515.
- The latest full authenticated GitHub GFM pass covered the 32 guides and eight supporting pages (40 pages) and registered 515/515 Mermaid containers. Increment 530 re-rendered the updated roadmap, progress-history index, and this archive; combined evidence covers all 41 current 2025 Markdown pages.
- The published candidate branch still points to e10fa19a. Its latest full browser audit hydrated 508/508 diagrams; the seven additions and 21 revised diagram bodies remain unpublished.
- Source-link checker results: candidate worktree 4,805/4,805; candidate HEAD 4,796/4,805; initial-work 4,796/4,805; current local primary commit 2c0194c 4,805/4,805.
- The nine candidate-HEAD misses fall in process_federation_service_host.cpp, rti_initialization_data.cpp, ieee1516_2025_connection_process_federation_service_host_catch2.cpp, and tck_connection_lifecycle.cpp.
- Semantic source review confirmed two lifecycle targets move between the candidate worktree and primary commit: the Federate-Lost scenario starts at candidate worktree line 2389 and primary line 2607; the post-loss HLAfederatesInFederation assertion is at candidate worktree line 2765 and primary line 2983.
- No open PR targets the candidate branch, and the remote candidate branch remains at e10fa19a.

### Next handoff

Review and publish the relevant 2025 source/test changes on the candidate branch. Recheck all four citation revisions and verify semantic anchors against the exact published source revision. Then publish the seven new diagrams and 21 revised bodies and verify hosted hydration for all 515 diagrams.

No C++ build, tests, or CI ran. The current primary branch contains unrelated unpushed work, including Requirements Lab changes; keep it outside this 2025 documentation publication. Keep 2010 separate.

## Increment 531 — refreshed primary-source citation check (2026-11-15)

The local primary branch advanced to 95a3d133, eight commits ahead of its remote tracking ref. The full candidate-documentation path/line checker still resolves 4,805/4,805 references against that primary commit. The candidate worktree also resolves 4,805/4,805; candidate HEAD and initial-work remain at 4,796/4,805 because the candidate branch does not contain the four active 2025 source/test changes. The remote candidate branch remains at e10fa19a, with no open PR.

The two lifecycle TCK targets were rechecked semantically at primary commit 95a3d133: scenarioFederateLostMomReportPortable starts at line 2607, and the post-loss HLAfederatesInFederation assertion is at line 2983. Their candidate-worktree counterparts remain at lines 2389 and 2765. The aggregate primary link pass does not make that source revision publicly available on the candidate branch.

Next: publish the reviewed 2025 source/test changes on the candidate branch, then verify path/line and semantic references there before publishing the seven new and 21 revised diagrams. The 91% working estimate and 508/508 published hydration baseline remain unchanged. No C++ build, tests, or CI ran; keep 2010 and Requirements Lab separate.


## Increment 532 — published diagram and hydration closeout (2026-11-15)

The working completion estimate is **approximately 95%**; this is a qualitative
estimate, not a fraction of checklist items. Candidate branch
`feature/hla-2025-docs-candidate-20261107` is published at
`431152817283718923c06a07e4154ca785e7033a`. Its 32 guides contain all 515
current diagrams, including the seven additions and 21 revised bodies.

The hosted hydration check is complete for 515/515 diagrams. The earlier
all-guide pass covered 508 charts. This increment checked the five changed
pages individually: Data Distribution and Regions 36/36, Service Invocation
Reporting 17/17, Federation Save and Restore 43/43, FOM Admission and
Composition 13/13, and Federation Synchronization 21/21. The other 27 pages
contain 385 unchanged charts covered by the earlier full audit. No page-count
mismatches were found. This revision-aware combination does not claim a single
reload of every page or full-width visual review of every chart.

Authenticated GitHub GFM checks were refreshed for the five changed Markdown
pages; combined with earlier checks, all 41 current 2025 Markdown pages are
covered. The link checker finds 4,805/4,805 references in the current worktree
and at primary HEAD `4b50f50`; published candidate HEAD resolves 4,796/4,805.
Nine out-of-range anchors remain in four 2025 source/test files absent from the
published candidate revision. The exact targets and affected guides are listed
in `HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json`; do not move them to unrelated
source lines. Source owners must stabilize the cited changes, after which the
path and semantic anchors need one final check.

No C++ build, tests, or CI ran. Keep 2010 and the Requirements Lab separate.


## Increment 533 — source revision routing audit (2026-11-15)

The working completion estimate remains **approximately 95%**. Candidate branch
feature/hla-2025-docs-candidate-20261107 is at
2eac0907917628e7f50b20654233bb97ec994d3a; this documentation-only closeout
commit did not change Mermaid bodies. Hosted hydration evidence remains valid
for all 515 diagrams because their source bodies are unchanged from checked
diagram revision 431152817283718923c06a07e4154ca785e7033a.

The current source-link check resolves 4,805/4,805 references in the candidate
worktree and at local primary HEAD b716d951; candidate HEAD resolves
4,796/4,805, with the same nine out-of-range anchors in four 2025 source/test
files. Primary feature/initial-work is ten commits ahead of remote
c9c5a9e4; those commits include unrelated Requirements Lab work. The remote
has no separate branch containing the current source changes, and the current
GitHub pull-request query returned no open PRs. Do not import the mixed primary
history into the documentation branch. Stabilize and publish only the reviewed
source changes through their owners, then verify all nine line and semantic
targets against that published revision.

No C++ build, tests, or CI ran. Keep 2010 and the Requirements Lab separate.


## Increment 534 — guide navigation coverage audit (2026-11-15)

The bounded inventory found 32 current IEEE 1516.1-2025 guide files, and all
32 contain at least one Mermaid diagram. The thematic reading paths already
linked all 32. The living behavior backlog linked 31 and omitted the MOM object
and interaction reports guide; its Covered summary now links that guide. Both
indexes now resolve all 32 guide destinations.

The backlog index is 473,249 bytes, above GitHub's Markdown API 400,000-byte
input limit, so this increment did not claim an API GFM result for that page.
The new relative destination exists. This increment also rechecked the roadmap
and both progress-history pages with the authenticated GitHub GFM API; all
three responses were nonempty, maintaining coverage of all 41 current 2025
Markdown pages. Source-link status remains 4,805/4,805 in the worktree and at
local primary HEAD 063e5eae, versus 4,796/4,805 at candidate revision 9e1bc4fe.
The nine unresolved source anchors remain pending owner publication and
semantic recheck.

No C++ build, tests, or CI ran. Keep 2010 and the Requirements Lab separate.


## Increment 535 — source-routing snapshot refresh (2026-11-15)

The candidate documentation branch is published at 141b6629. Its 41 current
2025 Markdown pages contain 4,805 unique source-line references. The published
candidate revision resolves 4,796; nine anchors across four active 2025
source/test files remain out of range. The candidate worktree resolves all
4,805 only with active, uncommitted source/test edits, so that result is not a
published-source verification.

Checked against the candidate's full 41-page documentation set, local primary
HEAD 4bfa5928 has all 4,805 cited lines in range. This is path/line evidence
only. A bounded semantic check found that primary line 2389 in the lifecycle
TCK is an automatic delete-then-divest assertion, while candidate-worktree line
2389 begins the Federate-Lost MOM scenario described by the guide. The 4,324
link result from a check run against the primary branch's own 33 Markdown
pages is not a candidate-wide result. Primary is twelve commits ahead of remote
initial-work and contains unrelated implementation and Requirements Lab
changes. Remote initial-work c9c5a9e resolves 4,796/4,805 against candidate
docs. No open source pull request was found.

A bounded review of all nine cited targets in the candidate worktree matched
the guide labels: the host connection routine handles resume/replay and
inactive sessions; the RID parser recognizes heartbeat and purge timeouts;
five Catch2 anchors name the expected resume, purge, heartbeat, reconnect, and
history-gap cases; and the TCK anchors identify the Federate-Lost scenario and
its post-loss membership update. These semantic observations still depend on
uncommitted source/test edits. Keep closeout pending until owners publish a
reviewed source revision, then repeat both line and semantic checks.

No Mermaid bodies changed. No C++ build, tests, or CI ran. Keep 2010 and the
Requirements Lab separate.
