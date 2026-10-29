# Requirements Lab remapping baseline — 2026-10-29

## Source and scope

Umbra is now pinned to HLA Requirements Lab tag `remapping-baseline-2026-10-29`, commit `e8c63e7a26f562dd6a66cbaa9c8ec87608df872c`. The generated corpus bundle in `.compliance/corpus-bundle.json` was exported from that exact revision. The lock retains the existing local checkout path `../Document-Recreation`.

The `hla-1516.1-2025` export grew from 1,865 to 2,014 requirement records (+149). The record IDs remain stable for all 1,022 selected requirements checked against the previous baseline, and all 243 selected C++ API surface IDs remain available. Of those requirements, 470 now resolve to a different canonical clause. The corpus integrity check is clean; 1,861 ordinal positions changed, so ordinal values must not be used as cross-revision identity.

## Mapping changes

- Removed all 60 Catch2 `requirement_section_mapping_overrides`: 58 now agree directly with the tag export, and the two RL-357 overrides were superseded when the Lab assigned the stable requirement to §4.8.1. The two time-representation cases now use that direct mapping; `reported_normative_clause` retains the historical §4.8 suggestion.
- Remapped six stale `time-save-restore` roadmap section pointers through the stable requirement IDs: §6.11.1 → §6.11; §6.15.1 → §6.15; §8.22.3 → §8.22; §9.7.5 → §§9.7, 9.7.3, 9.7.4; §10.37.1 → §§10.35.6, 10.36.6; and §10.38.4 → §10.37.6.
- Refreshed the roadmap traceability snapshot from 2,225 to 2,374 available standard requirements and recalculated mapped coverage from 52.58% to 49.28%; all other snapshot counts already matched the live plan.
- Updated the lock revision and the RL-357 current workaround text. The issue remains `resolved-in-lab`; its earlier reported clause is preserved as history.

## Verified gates

- `python tools/requirements_lab.py check-plan --bundle .compliance/corpus-bundle.json` passes: 1,764 entries, 1,170 selected requirements, 243 C++ API surfaces, and 2,014 available 1516.1-2025 requirements.
- `python tools/query_rti_work.py check --summary --compact` passes: 84 roadmap items, 1,764 Catch2 cases, 1,643 mapped, 121 explicitly dispositioned, 0 unclassified, 331 standard clauses, and 1,170 direct requirement-to-section pairs.
- `python tools/query_rti_work.py contract-drift --summary --compact` is clean: 5,170 native references exact, 0 findings.
- The local Requirements Lab issue ledger reports 0 open issues. The 121 cases without a normative requirement mapping all have explicit dispositions.
- `git diff --check` passes.

The live checker reports one non-actionable source-location warning for `umbra-cpp-transportation-type-change-integration`. That row is `disabled-source-artifact`, intentionally excluded from evidence, and points to five source-located replacement cases; it does not make the mapping gate fail.

## Historical completion ledger reconciliation

The first strict audit found 605 errors across 467 of 709 append-only completion rows. The breakdown below records what was found before reconciliation:

| Finding | Count |
| --- | ---: |
| Historical references to 2025 sections absent from the new export | 454 |
| Unknown Catch2 plan IDs | 42 |
| Plan ID disagrees with the exact test query | 35 |
| Recorded source location differs from the derived test location | 24 |
| Test query does not resolve to exactly one case | 18 |
| No derived C++ source location | 17 |
| Assertion count missing or invalid | 11 |
| Missing `source_location` field | 4 |
| **Total** | **605** |

To preserve the audit trail, I added only `superseded_by` to those 467 rows; their recorded IDs, query text, sections, source locations, assertion counts, and status values remain unchanged. Of the supersession pointers, 446 point to the current Catch2 plan row under the tagged remap, five point to a current Catch2 row whose historical plan/source handle changed, eleven identify external C++ TCK records that have no current Catch2 plan row, four mark duplicate history entries, and one retires an unaddressable legacy entry. The 242 rows without diagnostics were left untouched. The total ledger now has 491 superseded rows, including 24 that were already marked before this work.

`python tools/query_rti_work.py check --historical --summary --compact` now passes. The old evidence remains in the ledger but is excluded from the current recent-completion view where its handles are superseded. No tests were rerun and no new completion evidence was claimed. The remap and reconciliation did not add an open Lab issue.

## Current mapping status after evidence-driven continuation

The tagged source remains unchanged. A bounded C++ source audit added 21 direct mappings on top of the initial remap snapshot: eight synchronization-point behaviors, six individual DDM/default-region requirements, two ordinary/regional object-subscription requirements, four interaction-subscription or payload requirements, and one passive/active regional-subscription rule. The mappings stay attached to focused C++ plan rows and retain the development-profile evidence boundary.

| Measure | Current status |
| --- | ---: |
| Catch2 plan rows | 1,764 |
| Rows with normative mappings | 1,643 |
| Rows without a normative mapping, each explicitly dispositioned | 121 |
| Unclassified rows | 0 |
| Requirements with direct test mappings | 1,191 / 2,374 (50.17%) |
| Globally uncovered requirements | 1,183 |

All 1,764 plan rows therefore have a classified mapping disposition. Normative requirement coverage is not 100%: 1,183 requirements still lack direct mapped evidence. Do not fill those by assigning IDs based on API-name similarity, nearby tests, or section membership. Add a mapping only after reading the exact pinned requirement and the assertions in its candidate case; otherwise leave the gap visible for a new focused test or an explicit reason it is outside current scope.

The work focused on §9.1.5. Its main clause now has 16 of 28 requirements mapped; 12 remain uncovered, plus one uncovered Annex E example. Direct additions include the four individual DM default-region service interpretations and separation of ordinary/regional subscription state, the reverse DDM-to-DM independence rule, regional active/passive state, ordinary/regional interaction-subscription state, once-only receipt when both ordinary and regional interaction subscriptions are active, and assertions that the received interaction class and parameter bytes match the Send Interaction With Regions arguments.

The largest remaining clusters are §11.5.1 (110/148 uncovered), IEEE 1516.2-2025 §4 (52/94), and §12.13.4.4 (21/21). These are gap summaries, not a claim that every uncovered requirement is testable from current source.

### Prioritized coverage backlog (snapshot: 2026-10-29)

Priorities are ordered for useful evidence per focused work slice, not simply by raw gap count. Re-evaluate the counts after each slice. Every requirement needs a direct assertion-to-source basis; unsupported candidates stay uncovered and become focused-test work where applicable.

| Priority | Work package | Uncovered at snapshot | Why this order |
| ---: | --- | ---: | --- |
| 1 | Finish the active §9.1.5 object-DDM audit | 12 | A bounded handoff and focused lane already exist; it is ready for exact candidate-by-candidate review. |
| 2 | Audit §§4.1.4.1, 5.1, 5.1.2, 8.1.1, and 9.1.1 | 68 | Small, coherent FOM validation, ownership, and time-behavior groups; inspect existing assertions first and add tests only where evidence is absent. |
| 3 | Audit IEEE 1516.2-2025 §4 | 52 | Keep object-model semantics distinct from the 1516.1 API; check applicability and parser/model evidence individually. |
| 4 | Cover the MOM inventories in §§11.4.1, 11.5.1, 12.12.1.1, and 12.12.4.2 | 154 | High coverage payoff, but split work by exact table, attribute, type, and value assertions to prevent broad overclaiming. |
| 5 | Audit §§12.11.2, 12.11.4, 12.13.3.3, and 12.13.4.4–12.13.4.6 | 96 | Separate API/type contract checks from message recovery and transport behavior that may need fault-injection integration evidence. |
| 6 | Re-rank the remaining gaps from the live query | 801 | These are outside the first five coherent batches; regenerate the queue after work and prioritize direct, testable behavior. |

The current total is 1,183 uncovered requirements. This is a planning queue, not a promise that every uncovered clause is applicable to this implementation or already has an executable test seam. Do not reach 100% by assigning mappings from names, section adjacency, or unverified AI summaries.

### Resume this mapping work

Start with the bounded §9.1.5 gap query, inspect each exact requirement and candidate test, and keep the requirement uncovered when the current assertions do not support it:

```powershell
python tools/query_rti_work.py resume
python tools/query_rti_work.py gaps --document hla-1516.1-2025 --clause 9.1.5 --summary --compact --limit 12
python tools/query_rti_work.py requirement <exact-requirement-id> --summary --compact
python tools/query_rti_work.py case <exact-plan-id> --summary --compact
python tools/query_rti_work.py check --lane object-ddm-ownership --summary --compact
```

The next specific candidate is `requirement-candidate-content-clauses-09-data-distribution-management-page-223-l40-13-available`; confirm whether an existing test constrains class-attribute availability for the listed services before mapping it. The subscription failure matrices currently exercise undefined handles, which alone do not establish the full “available attribute of that object class” rule. Continue without resynchronizing unchanged Lab inputs.

Verification after this continuation: `requirements_lab.py check-plan` passes with 1,191 plan requirements; `query_rti_work.py check` passes with 1,191 direct requirement-section pairs; the focused `default_region_interaction_routing` CTest selector passes 1/1. The checked-in CTest source-location handle is current, and `git diff --check` reports no whitespace errors. The class-attribute-availability candidate remains uncovered: the existing focused service case proves only valid attributes succeed, while the failure matrices use undefined handles rather than valid handles unavailable to the selected object class.

The live mapping snapshot and `next_mapping_handoff` in `ROADMAP-INDEX.json` are refreshed to 1,191 direct requirement-section pairs, 2,374 total requirements, and 50.17%. The initial remap metrics above remain as historical evidence for the original remapping step.
