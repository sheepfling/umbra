# IEEE 1516.1-2025 C++ RTI finish plan

Status: execution plan, 2026-10-14. This is a C++ 2025 plan, not a conformance
claim. The pinned Requirements Lab is an input; do not resynchronize it without
an established upstream change. Keep the 2010 implementation and the later
Java adapter/TCK outside the C++ release gate.

## Baseline and meaning of progress

The live query is `python tools/query_rti_work.py resume`, followed by one
bounded `work`, `case`, `focus`, or `ready --summary --compact` detail query.
The current roadmap checklist has 235 complete and 8 open entries. Its 10
open indexed families are a different grouping: five `evidence-complete`, four
`new-case-needed`, and one `external-review`. There are no executable or
planned candidates in the indexed queue. Do not recycle completed process TSO
work as a supposedly missing case.

The C++ Catch2 plan has 1,503 cases: 1,406 requirement-mapped and 97 explicitly
classified without a normative mapping. The mapping check records 980 direct
requirement-to-canonical-section pairs and 2,220 pinned 2025 requirement IDs;
pair count is not a requirement-completion percentage. The portable C++ TCK
catalogs 1,034 promoted scenarios. A pre-integration installed package passed
all 2,048 configured CTests, but its direct replay was stopped after 303
scenario chunks when subsequent runtime changes superseded that package.
Fresh post-integration JSON/JUnit and catalog validation remain a release gate.
The current source-size check is also red: 40 hard findings and 17 soft-limit
findings, including the portable TCK's 40,753-line `main.cpp` and 36,724-line
`portable_tck.cpp`. Two cohesive lookup-lifecycle and FOM-module extractions
removed 1,701 lines from `main.cpp` but did not clear its hard limit.

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

Adjudicate unlinked requirement IDs in bounded subsection batches, using the
live traceability query for the distinct-ID denominator. Classify each as:
testable C++ behavior needing a direct case;
existing behavior needing a precise evidence link; static FOM/API/schema
contract needing an appropriate validator; Requirements Lab extraction issue
needing a numbered ledger entry; or explicitly inapplicable/deferred with a
rationale. Report distinct-ID direct-mapping and adjudicated ratios separately
from requirement-section pair counts.
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

## Parallel execution board: current wave

Keep these four seats active from isolated Git worktrees. Each worker has its
own MinGW `out/` and one vertical slice; no worker uses the dirty integration
checkout. The integration owner cherry-picks verified commits one at a time.

| Seat | Start-now packet | Exclusive working area | Deliverable |
| --- | --- | --- | --- |
| 1. Object/DDM runtime | Preserve the verified process HLA_IMMEDIATE regional delivery and resign-fence behavior. Correct the test wording to describe the proven callback-task-admission boundary, then project the changed behavior through the installed-package regional TCK. Do not invent a later callback-entry recheck without a normative requirement and failing case. | Process object/DDM routing and its focused C++ tests; avoid save/restore and MOM files. | Precise 2025 clause mapping, corrected case/plan/roadmap wording, focused MinGW CTest and lane check. |
| 2. Save/restore and TCK quality | Preserve the verified fresh-registry Release Multiple Names abort-all case. The lookup-lifecycle and FOM-module scenario groups are now in ordinary `.cpp`/`.hpp` ownership; keep their scenario IDs and both callback-model behavior stable through the full installed-package run. | Portable TCK scenario declarations/implementation and CMake; avoid runtime and MOM files. | Both cohesive extractions built under MinGW and passed 20 plus 48 focused tests; the hard source-size gate remains open. |
| 3. MOM/reporting runtime | The distinct pending-advance `InTimeAdvancingState` rejection is integrated and its six adjacent CTests pass. Hold further runtime edits during the fresh-package checkpoint; afterward select one separately mapped invalid-time failure or process/MOM reporting route. | MOM time-advance reporting and official-C++ service-report tests; no TCK runner/catalog edits. | Preserve the two green NMRA failure cases and their separate RL-321/RL-322 extraction notes; make no general reporting claim. |
| 4. Integration and portable TCK | Complete the shared MinGW build and neighboring tests for each incoming slice, merge verified commits serially, update the active roadmap handoff, then rebuild/install a fresh package. Run all 1,034 promoted C++ TCK scenarios in both callback models with direct JSON/JUnit and classify failures. | Integration checkout, installed-package/TCK evidence, shared index merge resolution; no simultaneous lane-owned runtime edits. | A current pass/fail list with reproducible packets, mapped clean branch, build-quality snapshot, and explicit release blockers. |

If a fifth contributor is available, give them FOM/Annex-C evidence closure and
any verified 2025 FOM gap confined to `cpp/src/internal/fom/**`. A sixth can
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
During a full package-evidence run, freeze runtime commits that would supersede
the installed binary; workers can continue independent source-quality work in
isolation, but integrate it before the TCK executable is built.
