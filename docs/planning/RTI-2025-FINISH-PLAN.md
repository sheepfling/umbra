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

The C++ Catch2 plan has 1,520 cases: 1,423 requirement-mapped and 97 explicitly
classified without a normative mapping. The mapping check records 983 direct
requirement-to-canonical-section pairs and 2,220 pinned 2025 requirement IDs;
neither mapped-case count nor pair count is a requirement-completion percentage.
A mapping records traceability, not implementation, Lab validation, or conformance.
The indexed direct-mapping coverage is 44.28% of pinned requirement IDs; the
remaining 55.72% have no direct plan pair, and neither share can be called an
implementation-completeness figure. The bounded `gaps` query should supply
candidate breadth work after each concrete behavior slice, not be used to
retroactively award validation to existing tests.
The portable C++ TCK
catalogs 1,036 promoted scenarios. The fresh installed-provider checkpoint
passed all 2,048 configured CTests and 2,072 direct both-model replays with
valid JSON/JUnit, including 24 process-loss fixture cases. Later 2025 runtime
and TCK-source commits supersede that binary, so full native and frozen-commit
package regressions remain release gates. The current source-size check is red:
41 hard findings and 18 soft-limit findings.
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
TCK drivers. At that earlier checkpoint they measured 29,600 and 24,764
expanded lines; the current measurements are in the execution board. The latest
ignored partial MinGW TCK-quality snapshot records 94 translation units,
351.5 summed compiler seconds, and 10.23 seconds for the TCK link; the
history remains queryable in `out/build-metrics/history.jsonl`.
The latest 2026-10-15 partial embedded MinGW capture
`20261015T224747.268588Z-e0f43668` records 1,521 translation units,
4,709.6 summed compiler seconds, 40,166 first-party include edges, no
unmeasured compile commands, 18 soft size findings, and 41 size blockers.
Its nonzero exit is the open quality gate, not a build failure; the full build
completed before capture.

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
| 1. Remote regional subscription/scoped update | Integrated: exact core/connection registrations retain 66 assertions, pass five repeats each, and have the focused `process-regional-subscription-update` lane. | Keep the regional overlap, remote discovery/reflection, sent-region metadata, and recipient assertions green; no new native repair is queued here. |
| 2. Timestamped regional interaction and advisory callbacks | Integrated: the regional TSO pair now serves the sender report and pushed-event acknowledgement in order; the advisory rate-reissue pair uses synchronized callback snapshots rather than fabricated service calls. The new 62-assertion official C++ HLA_IMMEDIATE regional NMR case covers two OS ingress proxies, passed 20/20 repeats, and the four-case cross-endpoint process lane passed 4/4. | Preserve the official 2025 timestamp, region, rate-designator, and callback assertions; the new case's three direct pairs cover only NMR selection/order and positive regional-overlap delivery. The §9.12 send-scope exclusion requirement remains unmapped without a disjoint receiver. Proxy topology and the bounded 150 ms no-third-event check are implementation evidence, not general TSO, DDM, interoperability, or conformance proof. |
| 3. Portable C++ TCK source ownership | Sixteen cohesive extractions are integrated; `main.cpp` is 22,216 lines and `portable_tck.cpp` is 22,614. The latest ordinary multi-attribute subscription/value-update family passed 8/8 affected both-model installed-provider registrations; the catalog remains 1,036 valid IDs with full 164-method/56-callback API audit. | Continue cohesive ordinary `.cpp/.hpp` ownership splits. The 5,161-selector audit and full package run must be refreshed at the frozen runtime commit; no broad mechanical line-count split. |
| 4. Integration and release gates | The buffered-cancellation process case passed 100/100 isolated and 20/20 focused repeats; its private callback-admission claim rejected the stale owner release before projection. The process-client wait-notification repair passed 800/800 isolated class-wide and 800/800 concurrent regional starts. The 81-assertion process NMRA failed-report MOM interaction case passed 20/20 focused repeats. After repairing three scripted ownership fixtures for the new private claim, those cases passed 60/60 concurrent repeats. Earlier expanded partitions had intermittent fixture failures; the save-not-complete and synchronization-point callback waits are now bounded. After the final MinGW build, the current 562-case process partition passed 562/562 at both eight-way and two-way concurrency. | The callback claim does not prove atomicity with entry into the federate's C++ method. A minor diagnostic follow-on must name `claim_attribute_ownership_release` in `operationName()` before its next runtime edit. Continue migrating callback-await fixtures by focused lane, preserving negative and ordering assertions; investigate timeouts without an assertion separately. Source-size still has 41 hard findings; full native outside this partition, installed package, query-regression, interoperability, and protected conformance gates stay open. Do not tag or claim a whole-repository clean bill yet. |

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
combined native and package gates. The 560-case process checkpoint passed at
eight-way concurrency after a response/callback wait-notification repair, a
bounded callback-admission ownership claim, and matching scripted fixture
updates. The expanded 561-case partition had a 561/561 two-way checkpoint,
but its later two-way rerun passed only 557/561 and eight-way reruns also
failed; this is not a clean release gate. The four two-way timeouts include
two callback-count assertions after zero-duration Evoke and two without a
reported assertion. Three passed an immediate four-case replay; the idle
restore-status fixture failed again on a different immediate-count check. That
exact case then passed 40/40 focused repeats after bounded callback waits and
fail-fast cleanup; the full partition has not been rerun since that repair.
The ownership slice proves idle HLA_IMMEDIATE delivery and suppresses a
buffered owner release after cancellation in the covered process route.
A bounded FOM/Annex-C
evidence-closure packet remains a separate follow-on. Requirement
adjudication is a subsection-sized read-only lane, not an open-ended search;
neither lane invents tests from candidate text. Java parity begins after the
C++ gates rather than consuming a current C++ seat.

## Current release-gate packets

A prior full installed-package C++ TCK checkpoint was green against its then-
current provider, but it precedes the latest native runtime changes and is not
a clean current repository-quality bill. Keep these finite packets
separate so an audit repair does not get misreported as new RTI conformance:

| Packet | Verified starting point | Close only after |
| --- | --- | --- |
| Official 2025 vendor bytes | The 44 C++ headers and seven 1516.2 resources are byte-exact against the pinned archives; narrow vendor-tree Git text exceptions preserve those bytes. Binding inventory and resource CTests pass. | Preserve manifest digests and confirm the fresh installed-package check at the frozen integration commit. |
| Test metadata and names | The live mapping check passes with 1,423 mapped cases, 97 explicitly unmapped, zero unclassified, and 983 unique direct pairs. RL-329 records the page-165 Attribute Ownership Unavailable introduction misattributed to 7.10.6; local direct mappings use canonical 7.10. The optional strict historical completion-ledger audit currently fails with 113 stale or mismatched entries; this is separate from the live mapping pass. The previous 5,161/5,161 selector audit predates this integration. | Repair evidence-backed historical ledger pointers and query guards without resynchronizing the Lab; rerun static gates at the frozen commit. |
| Native process regressions | The earlier 559–562-case partitions exposed scripted ownership and zero-duration positive-callback fixture races, with one 561/561 two-way checkpoint but later red runs. Their exact failures and replay logs remain under ignored `out/mgw/embedded/`. The new cross-proxy regional NMR case passes 62 assertions and 20/20 focused repeats after bounded child-process teardown; its four-case lane passed 4/4. The save-not-complete case passes 20/20 focused repeats, a neighboring save-status case 5/5, and the synchronization-point case 40/40 core/connection repeats after bounded callback waits and failure cleanup. The final MinGW build passed. The fresh process partition now passes 562/562 with zero failures or skips at eight-way concurrency in 31.06 seconds (`process-boundary-20261015-post-posix-j8.log/.xml`) and at two-way concurrency in 87.89 seconds (`process-boundary-20261015-post-posix-j2.log/.xml`). | Both current process concurrency gates are green. Continue lane-wise migration of the remaining risky positive callback waits without weakening order or negative checks. These checkpoints are not a whole-repository clean bill; source-size, historical ledger, package, interoperability, and protected review remain separate. |
| Process callback-fixture timing | Focused ownership, save/restore, and MOM cases use bounded waits for expected callbacks. A bounded inventory of tagged 2025 connection-process tests found about 110 zero-duration `evokeCallback` calls immediately followed by positive callback-state assertions across 33 cases in 29 files; this is a risk inventory, not a count of failures. Ten cases also have intentional negative/zero checks. The eight process federation-save fixture files were inspected: five evoked save-not-complete cases already use bounded waits; the one failing single-federate case has been converted; eight zero-duration positive waits remain in the distinct timed-replacement fixture, whose scripted poll order needs separate work. | Add a small predicate/deadline `evokeUntil` test helper and migrate one focused lane at a time; keep causal callback-order and negative assertions intact. Rerun exact selectors and the eight-way process partition after each packet. Do not classify a harness timing failure as an RTI defect without a failing-first behavioral case. |
| Source ownership and comprehensive proof | Sixteen cohesive TCK extractions are integrated; `main.cpp` shrank from 29,600 to 22,216 lines and `portable_tck.cpp` from 24,765 to 22,614. The latest ordinary multi-attribute subscription/value-update family passed 8/8 affected both-model CTests; the catalog validates 1,036/1,036 and the official API audit covers 164 methods/56 callbacks. Source-size remains red at 41 hard findings, including both oversized drivers. The earlier installed-provider checkpoint passed 2,048/2,048 configured CTests and 2,072/2,072 direct replays, but predates the current runtime. | Resolve or justify remaining source-size findings, freeze one integration commit, then refresh the installed provider and repeat native/package gates with retained JSON/JUnit. |

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
