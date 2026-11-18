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
