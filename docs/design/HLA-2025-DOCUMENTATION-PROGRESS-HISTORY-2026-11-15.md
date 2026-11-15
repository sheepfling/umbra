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
