# IEEE 1516.1-2025 Documentation Progress — 2026-11-18

## Increment 542 — reconcile the advanced source-branch snapshot

The published documentation candidate remains at `097a5156d5c75fd168c511153f2ba839e9c951f6`. It contains 32 behavior guides and 515 diagrams, with the existing revision-aware hosted hydration audit covering all 515. This increment changed no diagram bodies.

The source-link checker covers 4,805 unique references across 42 IEEE 1516.1-2025 Markdown files. Candidate HEAD resolves 4,796; the active documentation worktree resolves all 4,805. The remote `feature/initial-work` branch remains at `c9c5a9e410539b039cd8bba35b53d0ef2b24d180` and has the same nine unresolved anchors.

Local `feature/initial-work` has advanced to `7fbf5c9fa275179365fc8bbfe0751c8ce1e4ca7d`, 230 commits ahead of its remote. Its source lines are all in range, and targeted inspection confirms the service-host function, timeout parser, and five Catch2 behaviors around their intended citations. The two TCK locations still identify unrelated assertions: the Federate-Lost scenario begins at line 2607 and its post-loss update is at line 2983. No open GitHub PR exists, so this local branch is not a published, reviewed citation base.

The working completion estimate remains **approximately 95%**. The remaining closeout is to publish a reviewed 2025 source/test revision with the TCK citations aligned to their actual scenario and update, then verify all source references and meanings at that exact revision. No C++ build, tests, or CI ran. Keep 2010 and Requirements Lab separate.

**Next handoff:** coordinate a source revision that publishes the reviewed implementation/test evidence without importing unrelated local-branch history; correct the two TCK targets against that revision and rerun the full source-reference check.

## Increment 543 — rule out the other published branch as a citation base

The remote repository has three branches: the documentation candidate, `feature/initial-work`, and `master`. The citation check still resolves 4,796/4,805 references at the candidate and remote `feature/initial-work` source revisions. Local primary remains 230 commits ahead of its remote; seven citation regions match the intended host, timeout, and Catch2 behavior, but the two TCK locations still point to other assertions.

I checked all 4,805 source references against published `master` at `e1f0f61ab6371d978375c26283aa7f6a2b5bdde9`. It resolves none: the 702 referenced target files are absent from that branch. It is not a viable citation base for these 2025 guides.

The active worktree still resolves all 4,805 references. The 2025 documentation set remains 32 guides and 515 diagrams; no diagram body changed. No open PR exists. Completion remains approximately 95%; the remaining closeout requires a reviewed, published 2025 source revision with the two TCK targets aligned to the actual Federate-Lost scenario and update. No C++ build, tests, or CI ran. Keep 2010 and Requirements Lab separate.

**Next handoff:** source owners should publish a scoped, reviewed source/test revision that includes the citation targets, reconcile the two TCK links on that revision, and rerun the source-reference and semantic checks. Do not use `master` or import the unreviewed 230-commit local tip.

## Increment 544 — inspect a large Save/Restore chart on GitHub

Reviewed Save/Restore diagram #023 in GitHub’s expanded Mermaid dialog at candidate revision `4fd82dd2424938663dd7e75c14256e1d213f9460`. Its published body hash matches the trimmed-body digest in the visual ledger. The sequence remains legible across the inspected top, middle, and end sections; the dialog’s vertical scrolling is needed to follow the full chart, and no text clipping was observed. This raises the selected hosted visual-review sample to 70 charts across the 32 guides; it is not a full-width review of all 515 diagrams.

No diagram body, citation, or C++ source changed. The citation closeout is unchanged: candidate HEAD still has nine out-of-range source references, and the two TCK anchors require a reviewed source/test revision that publishes the intended Federate-Lost scenario and post-loss update. No build, tests, or CI ran. Keep the 2010 stream and Requirements Lab outside this 2025 documentation work.

**Next handoff:** source owners should publish the scoped 2025 source/test revision; align the two TCK citations on that revision, then rerun the source-reference and semantic checks. The 95% completion estimate remains pending that closeout.

## Increment 545 — close the pending transportation/divestiture visual review

Reviewed Order/Transportation diagram #007 (`HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md`, line 428) in the published candidate’s GitHub expanded Mermaid dialog at revision `3ad7f8a2b019f8818dc63989a280348ac3348795`. Its trimmed body hash matches the visual ledger. The embedded `HLA_EVOKED` sequence’s partial-set and missing-request branches are legible; vertical scrolling exposes the terminal outcomes without text clipping. This resolves the backlog’s explicitly pending visual review and raises the selected hosted review sample to 71 charts across 32 guides, not a full-width review of all 515.

The guide and backlog continue to mark the divestiture interleaving as source-derived: no focused 2025 test asserts the partial-set or no-confirmation outcome, and no normative ordering or configured-process parity is inferred. No diagram body, source citation, or C++ source changed. The separate citation closeout remains pending a reviewed 2025 source/test publication; no build, tests, or CI ran. Keep 2010 and Requirements Lab separate.

**Next handoff:** publish the scoped 2025 source/test revision, then reconcile the nine candidate-HEAD line gaps and the two TCK semantic targets against that exact revision. Continue visual review only for concrete changed or flagged charts; do not claim all-chart readability from the selected sample.

## Increment 546 — review late-subscription and credential-scope diagrams

Reviewed Subscription State diagram #011 (`HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md`, line 811) in the published candidate’s GitHub expanded Mermaid dialog at revision `1ac6e5a5de697cbfd8a1a8b8e9ceced0eb419b14`. Its trimmed body hash matches the visual ledger. The embedded after-discovery planner, configured-process ordinary-value path, and separate joined-federate MOM snapshot branch remain legible in the inspected top and lower sections; vertical scrolling is needed to follow the full flow, with no text clipping observed. This clears the backlog’s pending visual-review flag for this chart.

Also reviewed Authorization diagram #008 (`HLA-2025-AUTHORIZATION-FLOW-GUIDE.md`, line 469) at the same published revision. Its connection-state nodes, joined Disconnect rejection, and successful not-joined Disconnect/reset path are legible across the inspected top and lower sections; vertical scrolling is needed to follow the full chart, with no text clipping observed. This clears the guide’s explicit visual-review flag. Together, the two reviews raise the selected hosted sample to 73 charts across 32 guides; they do not establish readability for every hosted diagram.

The content remains bounded: the reentrant unsubscribe/resignation/region-change fence is source-derived and not directly tested; the known-object added-attribute value-only result and licensed §5.8 wording remain unresolved. Subscription diagrams do not establish callback-order rules, process/embedded parity, or 2010 behavior. The authorization chart describes Umbra's connection-lifetime state and does not establish a standard-mandated credential-retention policy; its focused test does not assert same-ambassador reauthorization. No diagram body or C++ source changed; no build, tests, or CI ran. The source citation publication gate remains open.

Reconciled the living backlog with the current diagram ledger: its old 23-chart pending-render queue and older review totals were stale. Current coverage is 515/515 local renders, 515/515 hosted hydration, and 73 selected hosted visual reviews. The selected sample is not an all-chart readability claim; follow-up remains targeted to changed or specifically flagged diagrams.

**Next handoff:** source owners should publish the reviewed 2025 source/test revision, then reconcile the nine candidate-HEAD line gaps and the two TCK semantic targets against that revision. Continue diagram review only for explicitly flagged or changed charts.

## Increment 547 — distinguish record and array alignment paths

Reviewed the 2025 fixed-record, fixed-array, variable-array, and variant implementations alongside their existing focused vectors. The fixed-record encoder tracks a record-local offset and pads after a field to the next field's boundary; fixed and variable arrays instead pad between elements to the shared prototype boundary. A nonempty variable array additionally pads after its signed `HLAinteger32BE` count to `max(4, prototype boundary)`; an empty one returns immediately after the count. The codecs do not append padding after the final field or element. The ordinary and extendable variant paths retain their separate discriminant and length-delimited rules in the following guide section.

Reworked Data Element Encoding diagram #005 into separate record, fixed-array, and variable-array lanes. Mermaid CLI 12.0.0 rendered the current body; the reviewed screenshot is legible without clipping. Its body SHA-256 is `f25d8503ba9e2dceb7da54c5b60e45ffd8838fdbe54ba05cc22f29749519a329`. The fixed-record test citation points to the existing vector beginning at test line 505. The 2025 documentation link checker resolves 4,806/4,806 unique references across 42 Markdown files in the worktree.

The local diagram ledger now tracks the revised body and local screenshot. The previously published revision still has the 515/515 hydration audit and 73 selected hosted visual reviews; the revised #005 body awaits publication and hosted review. The focused vectors cover declaration order, boundary padding, nested nonzero cursors, malformed/truncated padding, trailing data, variable-array count/empty paths, and variant-specific mapped/unmapped cases. No C++ build or test ran. The 2010 guide and implementation and the Requirements Lab were unchanged. The working completion estimate remains approximately 95%; the reviewed source/test citation revision and semantic TCK targets remain the closeout gate.

**Next handoff:** publish this documentation increment, inspect the updated #005 chart on GitHub, and refresh its hosted ledger entry. Continue the source-citation closeout only against a reviewed 2025 source/test revision; then reconcile the nine moved line anchors and two TCK targets and recheck all 4,806 references.

## Increment 548 — inspect composite alignment chart on GitHub

Reviewed Data Element Encoding diagram #005 (line 297) on the published candidate branch at revision `68217e97d15cfdc4f83a32025fded1be12c8b749`. GitHub rendered the revised Mermaid body in its expanded dialog. The fixed-record, fixed-array, and variable-array lanes retain their distinct field/element alignment and final-cursor rules. The tall diagram requires vertical scrolling; the inspected top and lower sections remain readable, with no clipped text observed. This confirms hydration and layout for the changed chart only. The selected hosted visual-review sample rises to 74 charts across the 32 guides; it is not an all-chart readability claim.

The citation check resolves 4,806/4,806 references in the active worktree and 4,797/4,806 at candidate HEAD. Nine line anchors remain out of range in four source/test files, and the two TCK destinations still need semantic alignment to the Federate-Lost scenario and post-loss update on a reviewed 2025 source/test revision. The working completion estimate remains **approximately 95%** pending that source-citation closeout. No C++ build, tests, or CI ran. Keep 2010 and the Requirements Lab separate.

**Next handoff:** coordinate publication of the reviewed 2025 source/test changes; then reconcile the nine line anchors and two TCK meanings against that exact revision and rerun the source-link and semantic checks. Keep visual reviews targeted to changed or flagged charts.

## Increment 549 — correct TCK citations to assertion sites

Reviewed the current configured-process Federate-Lost scenario in the active 2025 TCK source worktree. `scenarioFederateLostMomReportPortable` begins at line 2389. The guide's former line 2765 pointed to the `waitFor` label, not the assertion. Updated the guide to link the post-loss reflection assertion at line 2768 and the survivor-only member-list check at line 2778. These exact current-worktree sites support the two bounded claims; the source changes remain unpublished and the tests were not rerun.

The 2025 documentation link checker resolves 4,807/4,807 unique references across 42 Markdown files in the worktree. Against candidate HEAD `149a5b94f0e3f2ef35666f0d8cd3984df799cbb2`, it resolves 4,797/4,807; ten moved anchors remain out of range in four source/test files, including the three TCK links. The semantic TCK review is complete for the active worktree, but exact-revision bounds and meaning must be rechecked after source owners publish the reviewed changes. The completion estimate remains **approximately 95%**. No C++ build, tests, or CI ran. Keep 2010 and the Requirements Lab separate.

**Next handoff:** after source owners publish the reviewed 2025 source/test revision, rerun all 4,807 link checks and verify the ten moved anchors, especially the scenario/reflection/member-list targets, against that exact revision. Keep further visual review targeted to changed or flagged charts.
