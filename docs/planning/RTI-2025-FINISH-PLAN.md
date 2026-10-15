# IEEE 1516.1-2025 C++ RTI finish plan

Status: execution plan, 2026-10-15. This is a C++ 2025 plan, not a conformance
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

The C++ Catch2 plan has 1,507 cases: 1,410 requirement-mapped and 97 explicitly
classified without a normative mapping. The mapping check records 981 direct
requirement-to-canonical-section pairs and 2,220 pinned 2025 requirement IDs;
pair count is not a requirement-completion percentage. The portable C++ TCK
catalogs 1,036 promoted scenarios. The fresh installed-provider checkpoint
passed all 2,048 configured CTests and 2,072 direct both-model replays with
valid JSON/JUnit, including 24 process-loss fixture cases. Later 2025 runtime
and TCK-source commits supersede that binary, so full native and frozen-commit
package regressions remain release gates. The current source-size check is red:
42 hard findings and 18 soft-limit findings.
Cohesive callback-control and MOM-query extractions removed a further 5,105
lines from portable TCK `main.cpp` and 3,374 from `portable_tck.cpp`; both
drivers remain above the hard ceiling and need further ownership work. Later
federation-lifecycle and regional-service-report extractions removed another
1,173 and 1,026 lines, respectively. Inherited/derived delivery and regional
Auto Provide families removed another 1,416 and 1,913 lines. The Delay
Subscription Evaluation family removed another 1,590 lines from `main.cpp`.
Five additional cohesive extractions moved 1,800 custom-transportation,
1,880 timestamped-interaction, 1,639 MOM-interaction, 1,052
resignation/ownership, and 1,160 service-report lines out of the two portable
TCK drivers. They now measure 29,600 and 24,764 expanded lines. The latest
ignored partial MinGW TCK-quality snapshot records 94 translation units,
351.5 summed compiler seconds, and 10.23 seconds for the TCK link; the
history remains queryable in `out/build-metrics/history.jsonl`.

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
| 1. Timed multi-constrained federation save | Official Evoke receive is integrated and passes 2/2 on main, freeing this native worker. The next exact three-federate timed-save case times out in both native registrations when run serially at 25 seconds. | Locate its first wire/service/callback stall against pinned 2025 timed-save and advance obligations. Preserve each constrained recipient's own initiate-before-TAG boundary and successful completion; keep the queued-TSO variant separate. Adjudicate the seven currently selected mapping pairs after behavioral repair: three use unrelated sync/untimed clauses, and two timed-save candidate fragments are logged as RL-326/327. |
| 2. Directed interaction transportation override | Regional class Request Attribute Value Update and private control pass 3/3 on main, freeing this native worker. The next directed override case is independently unstable: the earlier partition timed out in the connection registration, while the serial retest timed out in core. | Pin down the nondeterministic wire/callback stall without weakening directed publication, confirmation, target-object and transportation metadata assertions across EVOKED/IMMEDIATE; retain seven direct 2025 pairs. Own a source file distinct from the timed-save worker. |
| 3. Timed-save traceability integrity | Six cohesive TCK owner extractions are integrated; `main.cpp` is down from 29,600 to 26,181 lines and `portable_tck.cpp` from 24,765 to 23,574. The latest eight both-model registrations and 1,036/1,036 catalog pass on main, freeing this worker. | Audit only the timed multi-constrained save plan row against its C++ assertions and pinned 2025 text. Remove unrelated synchronization/untimed anchors, handle RL-326/327 fragments without inventing IDs, and leave exact truthful direct section pairs. Keep this metadata packet separate from the behavioral worker's source edits. |
| 4. Integration and release gates | Integrate verified commits, maintain the exact roadmap handoff, and repeat targeted MinGW checks before a frozen native/package run. | FQR/GALT plus multiple-record Flush Queue passes 4/4; timed/untimed regional transport passes 4/4; regional advisory plus embedded controls passes 4/4; official Evoke receive passes 2/2; regional class Request Attribute Value Update plus private control passes 3/3; six TCK extraction slices pass 72/72 focused registrations. The eight-way process snapshot was 517/548 before the latest fixture repairs. Timed multi-constrained save is red in both registrations; directed transportation override is unstable in isolated registrations. These are not a clean bill; the installed-package checkpoint predates later source changes and needs refresh at the frozen state. |

Earlier durable/save, MOM-reporting, regional process, process-retraction,
queued-NMRA, and private process packets are integrated with their focused
checks. Retraction and queued NMRA pass both native registrations (2/2 each);
the private process callback case passes its exact integrated selector after
20 isolated repeats and 28 adjacent private cases.
The custom-transportation TCK source extraction moved 1,800 lines to a
cohesive ordinary source file while preserving all 1,036 catalog IDs; 28/36
focused package registrations passed and the eight failures exactly matched
the pre-extraction baseline. Timestamped, MOM, resignation/ownership, and
service-report extractions passed 28/28, 16/16, 24/24, and 238/238 focused
cases respectively, plus catalog/API/contract checks. The four Attribute
Relevance Advisory fixture packets now pass eight exact main registrations;
the complete focused acquisition/divestiture slice passes 18/18 main
registrations, including Confirm Divestiture push. The directed callback
case passes 2/2; ordinary and directed transportation callback fixtures pass
4/4 and 20/20 repeats; the new timestamped attribute-update/object-removal
retraction gate passes 2/2, pending a pre-fix regression proof. The untimed save callback case passed 20/20
isolated repeats and was not changed. These packets remain subject to
combined native and package gates. After the active native investigation seats
clear, refill with a bounded FOM/Annex-C evidence-closure packet or a verified
2025 counterexample confined to `cpp/src/internal/fom/**`. Requirement
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
| Test metadata and names | The local contract-selector guard is clean: 5,161/5,161 native references exact. The live mapping check passes with 1,410 mapped cases and no unclassified cases. Regional/timestamped query guards and the focused no-queue NMR/NMRA assertion, requirement, and section counts now match the C++ case and plan; the long regression next stops at the separate queued-TSO NMR/NMRA assertion guard near line 7672. The optional strict historical-completion audit still reports 110 legacy reference mismatches. | Repair only evidence-backed remaining query guards and historical ledger issues without resynchronizing the Lab or weakening normative mappings; rerun static gates at the frozen integration commit. |
| Native process regressions | The focused ownership acquisition/divestiture slice passes 18/18; directed callback and multi-recipient cases each pass 2/2; the transport/retraction slice passes 8/8. FQR/GALT plus multiple-record Flush Queue passes 4/4 on main, preserving 19 and 22 direct pairs. Timestamped regional transport passes 4/4; regional advisory public pair plus embedded controls passes 4/4 and preserves eight direct pairs. Official Evoke receive passes 2/2 with 17 direct pairs; regional class Request Attribute Value Update plus private control passes 3/3 with 16 direct pairs. The earlier eight-way process partition passed 517/548 and failed 31/548; a FIFO timeout and MOM failure from it passed when isolated. One earlier timed-regional stress timeout did not recur in 220 repeats per registration and remains unlocalized. | Resolve the exact timed multi-constrained federation-save handoff, then rerun a named process partition with bounded concurrency and finite timeouts. Distinguish parallel interference from independent behavioral defects. |
| Source ownership and comprehensive proof | Six cohesive TCK extractions move encoder round-trip, API-foundation, FOM-model, attribute-relevance, paired transportation/order-type changes, and transport/order delivery plus standard lookups into ordinary owner sources. `main.cpp` shrank from 29,600 to 26,181 lines and `portable_tck.cpp` from 24,765 to 23,574; main-branch focused installed-provider checks pass 8/8, 16/16, 16/16, 16/16, 8/8, and 8/8, with 1,036 catalog IDs retained. The source-size checker remains red (currently 42 hard findings, including both oversized drivers). The earlier installed-provider checkpoint passed 2,048/2,048 configured CTests and 2,072/2,072 direct replays, but predates later runtime and TCK source changes. | Resolve or justify remaining source-size findings, freeze one integration commit, then repeat native and installed-package gates with retained JSON/JUnit and a named native output log. |

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
