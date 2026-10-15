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

The C++ Catch2 plan has 1,506 cases: 1,409 requirement-mapped and 97 explicitly
classified without a normative mapping. The mapping check records 981 direct
requirement-to-canonical-section pairs and 2,220 pinned 2025 requirement IDs;
pair count is not a requirement-completion percentage. The portable C++ TCK
catalogs 1,036 promoted scenarios. A prior installed package passed all 2,048
configured CTests and 2,068 direct both-model replays with valid JSON/JUnit,
but later restore, reporting, and TCK-source changes superseded that binary.
An earlier combined-branch package passed 2,048 configured CTests and 2,072
direct both-model replays; its JSON/JUnit contains no failures. Subsequent
source changes supersede that binary. Full native regression remains a release
gate. The current source-size check is also red: 40 hard findings and 18
soft-limit findings.
Cohesive callback-control and MOM-query extractions removed a further 5,105
lines from portable TCK `main.cpp` and 3,374 from `portable_tck.cpp`; both
drivers remain above the hard ceiling and need further ownership work. Later
federation-lifecycle and regional-service-report extractions removed another
1,173 and 1,026 lines, respectively. Inherited/derived delivery and regional
Auto Provide families removed another 1,416 and 1,913 lines. The Delay
Subscription Evaluation family removed another 1,590 lines from `main.cpp`.
The drivers now measure 31,480 and 30,413 expanded lines.

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

Four concurrent seats are available, including the integration owner. Each
worker uses an isolated worktree and exclusive source ownership. The
integration owner cherry-picks verified commits serially; agents do not edit
the shared roadmap/index, merge one another's work, or synchronize the
Requirements Lab. Refill a seat only after its packet has an exact test result,
a commit, and a concrete next handoff.

| Seat | Current packet | Exclusive ownership and merge gate |
| --- | --- | --- |
| 1. 2025 callback admission | Resolve a process ownership Evoke Multiple timeout without losing a queued acquisition callback; check the serial directed-callback failure too. | Own `cpp/src/internal/runtime/umbra_rti_ambassador_callback_control_services.cpp` and focused official-C++ evidence only. Preserve the separate 2010 stream; require exact ownership and representative directed/callback-disabled tests before merge. |
| 2. Process save isolation | Reproduce the untimed federation-save callback assertion from the red process partition. | Own only a proven save fixture issue. If it shares seat 1's callback-pump root cause, report that dependency without editing the shared runtime seam. |
| 3. Portable C++ source quality | Extract a second cohesive timestamped-interaction scheduling family from oversized TCK `main.cpp`. | Own TCK scenario/CMake files only. Preserve 1,036 catalog IDs and 5,161 contract references; pass focused both-model cases and source-boundary checks. |
| 4. Integration and release gates | Integrate tested commits, maintain one roadmap handoff, and rerun combined MinGW native/package checks. | The fresh installed-provider checkpoint passed 2,048/2,048 configured CTests and 2,072/2,072 direct replays with valid JSON/JUnit and the explicit process-loss fixture. It precedes any later runtime merge, so own the frozen-commit rerun rather than calling this final. The process partition is red (56/544 with the then-known NMRA case excluded). |

Earlier durable/save, MOM-reporting, regional process, process-retraction,
queued-NMRA, and private process packets are integrated with their focused
checks. Retraction and queued NMRA pass both native registrations (2/2 each);
the private process callback case passes its exact integrated selector after
20 isolated repeats and 28 adjacent private cases.
The custom-transportation TCK source extraction moved 1,800 lines to a
cohesive ordinary source file while preserving all 1,036 catalog IDs; 28/36
focused package registrations passed and the eight failures exactly matched
the pre-extraction baseline. These packets remain subject to the combined
native and package gates below. The next vacated implementation seat should
take a bounded FOM/Annex-C evidence-closure packet or a verified 2025
counterexample confined to `cpp/src/internal/fom/**`. Requirement
adjudication is a subsection-sized read-only lane, not an open-ended search;
neither lane invents tests from candidate text. Java parity begins after the
C++ gates rather than consuming a current C++ seat.

## Current release-gate packets

A new full installed-package C++ TCK checkpoint is green against the freshly
installed provider, but it precedes pending native runtime changes and is not
a clean current native or repository-quality bill. Keep these finite packets
separate so an audit repair does not get misreported as new RTI conformance:

| Packet | Verified starting point | Close only after |
| --- | --- | --- |
| Official 2025 vendor bytes | The 44 C++ headers and seven 1516.2 resources are byte-exact against the pinned archives; narrow vendor-tree Git text exceptions preserve those bytes. Binding inventory and resource CTests pass. | Preserve manifest digests and confirm the fresh installed-package check at the frozen integration commit. |
| Test metadata and names | API IDs, duplicate roadmap key, lane tags, and 52 symbolic-name lookups are repaired. The local contract-selector guard is clean: 5,161/5,161 native references exact. A rebuilt MinGW Catch2 executable passes taxonomy; binding inventory, resource, traceability, and symbolic-name checks pass too. The long query regression still exposes stale local expectations one at a time (next known guard: latest completed-slice evidence counts versus its plan row, line 5334). The optional strict historical-completion audit reports 110 legacy reference mismatches; the live mapping check passes. | Repair the remaining bounded query-regression guards and historical ledger separately without resynchronizing the Lab or weakening normative mappings; rerun the five passing static gates at the frozen integration commit. |
| Native process regressions | The ownership case's four invalid early-absence assertions were removed and its earlier exact MinGW CTest passed (60 assertions). The regional callback-gating fixture passes all six native registrations. Retraction and queued-NMRA fixture corrections pass 2/2 integrated registrations each; the adjacent failed-NMRA-report case also passes 2/2. The private registry-bound probe race fix passes its exact integrated test, with 20 isolated repeats and 28 adjacent private cases. A newer broad process partition is red and a shared 2025 callback-drain issue is under repair. | Finish the callback-drain and save-isolation packets, then rerun the process partition serially or with proved-safe concurrency, loopback access, finite timeouts, and a named output log. |
| Source ownership and comprehensive proof | The latest 1,800-line cohesive custom-transportation extraction created no new size finding; the gate remains 39 hard/18 soft. A loopback-enabled native process partition with `-j2`, 18-second caps, and the then-known NMRA failure excluded finished red: 56/544 registrations failed, including duplicate core/connection registrations and ownership timeouts. A serial rerun reproduced the directed-interaction callback-disabled failure in both executables; other failures need individual classification. The older installed provider produced 48 failed package registrations, all of which passed focused fresh-provider replay. The full fresh-provider checkpoint then passed 2,048/2,048 configured CTests and 2,072/2,072 direct replays with valid JSON/JUnit, including 24/24 process-loss fixture cases. This provider predates any later runtime merge. | Finish partitioned native diagnosis and source ownership/size work; then repeat the combined native and installed-package gates at one frozen commit. Use explicit `--connection-loss-fixture` and preserve a named output log on the next broad native run. |

The first three packets can be developed in isolated worktrees concurrently.
The integration owner alone merges them serially and records the exact
green/red state; no tag or completeness claim precedes the final frozen-commit
checkpoint.

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
