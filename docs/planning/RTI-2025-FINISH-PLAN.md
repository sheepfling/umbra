# IEEE 1516.1-2025 C++ RTI finish plan

Status: execution plan, 2026-10-14. This is a C++ 2025 plan, not a conformance
claim. The pinned Requirements Lab is an input; do not resynchronize it without
an established upstream change. Keep the 2010 implementation and the later
Java adapter/TCK outside the C++ release gate.

## Baseline and meaning of progress

The live query is `python tools/query_rti_work.py resume`, followed by one
bounded `work`, `case`, `focus`, or `ready --summary --compact` detail query.
The current roadmap checklist has 235 complete and 8 open entries. Its 10
open indexed families are a different grouping: four `evidence-complete`, five
`new-case-needed`, and one `external-review`. There are no executable or
planned candidates in the indexed queue. The last active handoff is complete;
do not recycle its process TSO work as a supposedly missing case.

The C++ Catch2 plan has 1,489 cases: 1,392 requirement-mapped and 97 explicitly
classified without a normative mapping. Direct mappings touch 976 of 2,220
pinned 2025 requirement IDs (43.96%). This is a traceability statistic, not
implementation or conformance completion. The portable C++ TCK catalogs
1,034 promoted scenarios; catalog/API audits are inventory checks, not a fresh
all-green provider run. The last documented full provider matrix recorded six
failures across three behaviors, each in both callback models. Re-run against
a clean installed package before treating those failures as current.

## Work lanes

| Lane | Indexed families | Deliverable and exit evidence |
| --- | --- | --- |
| A. Base semantics | `fom-module-declaration-management`, `annex-c-and-reference-resolution`, `object-ddm-ownership` | Official 2025 C++ API behavior for FOM/OMT admission, declaration, objects, interactions, DDM and ownership. Use standard FOM/schema checks plus focused embedded and process C++ cases for each selected obligation. Close the first two evidence-complete families after a bounded evidence/scope review, or name a specific uncovered clause and executable case. |
| B. Temporal and durable state | `time-save-restore`, `multi-federate-callback-ordering` | Time advances, TSO delivery, callback ordering, save/restore and restarted-process state preserve the required behavior. Review and close evidence-complete holding entries unless a precise counterexample remains. Distinguish one-authority/multiple-ingress implementation evidence from any unproven topology; the standard does not require multiple independent authorities. |
| C. MOM and reporting | `mom-after-base-services` | MOM attributes/interactions, switches, service and exception reporting work through the public 2025 surface, with the required filesystem-backed service-report lifecycle. Add cases only after the corresponding base service is stable, including failure and disable/re-enable paths. |
| D. Process and package | `transport-and-conformance` | Independent-process endpoints, loss/restart, public callbacks and installed-package behavior match the corresponding embedded semantics. Run clean MinGW builds, process/package CTests and build-quality capture; resolve source-size/quality blockers or record justified policy exceptions. Hosted header/lifecycle smoke alone is not this gate. |
| E. Portable C++ evidence | `portable-cpp-tck-2025`, `disconnect-protected-review` | A fresh installed-package TCK run passes every promoted scenario in HLA_EVOKED and HLA_IMMEDIATE, with validated direct JSON/JUnit, API/catalog/mapping audits, and no unexplained provider failure. Raw Disconnect evidence remains unverified until protected review accepts it. |

The table above groups the release scope; it is not the assignment board.
The simultaneous first-wave assignments below divide actual source ownership.
`portable-java-tck-2025` is a separate post-C++ milestone; its 502 cataloged
parity IDs and mock-fixture CI do not establish Java-provider behavior.

## Requirement and case selection

For each runtime packet, carry a concrete case or closure review. The singular
`active_handoff` in `ROADMAP-INDEX.json` is only the integration team's global
resume recommendation; parallel workers use their assigned family `work`
query and exact case/lane checks, not that one rotating pointer. Before setting
a `new-case-needed` handoff, verify that the
behavior is not already covered with bounded `focus`, `case`, `trace`, and
targeted source lookup. Supply the intended source/CTest target and exact
requirement-to-canonical-2025-subsection pairs from the pinned text. Add its
Catch2 plan row only when the actual C++ case exists. Do not use an open-ended
`new evidence` label as a permanent next action.

Adjudicate the 1,244 currently unlinked requirement IDs in bounded subsection
batches. Classify each as: testable C++ behavior needing a direct case;
existing behavior needing a precise evidence link; static FOM/API/schema
contract needing an appropriate validator; Requirements Lab extraction issue
needing a numbered ledger entry; or explicitly inapplicable/deferred with a
rationale. Report both the direct-mapping ratio and the adjudicated ratio.
Do not chase 2,220 Catch2 mappings indiscriminately or mark candidates
validated from implementation code alone.

## Verification staircase and release decisions

1. Per change: focused MinGW target, exact CTest selector, changed-lane
   mapping check, `git diff --check`, and appropriate source-size/build-metric
   checks. Expand focused regressions when shared runtime changes.
2. Per lane integration: relevant neighboring CTests and process/package
   projections. Run the full MinGW CTest suite at integration/release
   checkpoints, not after every narrow change.
3. C++ release candidate: build and install from a clean tree; run the full
   portable C++ TCK in both callback models and retain validated JSON/JUnit;
   run catalog/API and requirement-mapping audits; capture build-quality
   metrics and resolve or explicitly justify release blockers.
4. Conformance claim: obtain protected review and an accepted evidence
   manifest. Passing local tests and raw sidecars are not `verified` evidence.

`C++ implementation complete`, `C++ release-candidate green`, and
`conformance accepted` are separate milestones. Java parity follows these
C++ gates rather than silently blocking or inflating them.

## Parallel execution board: first wave

Start these four seats at the same time, from one pinned local integration
commit in separate Git worktrees. Each worker has its own MinGW `out/` and
one vertical slice; no worker uses the dirty integration checkout. The
integration owner merges completed branches one at a time.

| Seat | Start-now packet | Exclusive working area | Deliverable |
| --- | --- | --- | --- |
| 1. Object/DDM runtime | Verify the 2025 Delay Subscription Evaluation rule and test process receive-order regional attribute delivery. The deferred-recipient lookup in `process_federation_service_attribute_value_updates.cpp` appears to omit sent-region context that its TSO counterpart retains; this is a source observation, not yet a confirmed defect. Use `work object-ddm-ownership --summary --compact`, check for an equivalent case, then add the focused public C++ process test and fix only if it fails. | Process attribute-update and object/DDM routing implementation plus its focused C++ test; avoid state-image and MOM files. | Exact 2025 clause mapping, public API regression, MinGW target and CTest, changed-lane check. |
| 2. Durable save/restore runtime | Use `work time-save-restore --summary --compact` and one targeted ledger lookup to choose a still-pending application ledger. Test disk-only, fresh-registry restore through the official C++ API, then implement the missing persistence if proven. If the pointer is already covered, close it and take the next verified save/restore gap; do not recreate the old NMR/NMRA work. | `federation_registry_state_image*`, restore files and `federation_save_commit_store*` plus focused restore tests; coordinate any object/DDM ledger interface with seat 1 before editing it. | One durable state round-trip with exact 2025 mapping, MinGW target/CTest and changed-lane check, or an evidence-backed closure note if no gap remains. |
| 3. MOM/reporting runtime | Reproduce the installed-package Service Reporting subscription interlock discrepancy against the mapped public process anchor selected by `resume`; then fix the provider path and add the precise C++ regression if it persists. The timestamped-delete service-report failure is the next MOM packet, not simultaneous edits in the same files. | MOM reporting, switch and filesystem report-store implementation plus focused tests; no TCK runner/catalog edits. | Correct official exception/report behavior, focused native test and affected installed-package TCK cases in both callback models. |
| 4. Integration and portable TCK | Build a clean installed MinGW package, run all 1,034 promoted C++ TCK scenarios in both callback models with direct JSON/JUnit, and classify the six last-documented failures. Hand confirmed provider bugs to seats 1-3; perform serial merges, neighboring process/package regressions, mapping/quality checks, and the release checkpoint. | Installed-package/TCK harness and evidence; shared CMake, CI, `ROADMAP-INDEX.json` and mapping-index merge resolution. Avoid concurrent runtime edits. | A current pass/fail list and reproducible failure packets, an integrated green branch, and no unexplained regression. |

If a fifth contributor is available, give them FOM/Annex-C evidence closure and
any verified 1516.2 gap confined to `cpp/src/internal/fom/**`. A sixth can
adjudicate unmapped requirement rows in bounded subsection batches and record
Requirements Lab defects. Neither should invent tests from candidate text or
edit another worker's runtime files. The time/callback holding items are
`evidence-complete`; close them with a bounded review or assign a verified
counterexample before reserving another implementation seat.

Each runtime branch must include its source, focused C++ test, required CMake
registration, exact Catch2 plan mapping, and roadmap handoff as one coherent
slice. The shared CMake/plan/index edits are inevitable; the integrator
reconciles them through serial merges and reruns the changed-lane check. Any
change to `federation_registry.hpp`, generic process host/dispatch, or another
shared seam requires a short interface handoff before the second worker edits
it. Run focused tests on each branch, affected installed-package TCK cases
after each merge, and the full MinGW/TCK matrices at integration checkpoints.
