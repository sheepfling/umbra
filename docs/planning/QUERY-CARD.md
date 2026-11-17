# RTI query card

Run at repository root; queries use the pinned baseline, not a Lab sync. Pick one row.

## Fast path

Use `resume` once, then `ready`, `case <handle>`, and `matrix <handle>` for the selected slice; these bounded lookups replace opening the roadmap or Catch2 plan wholesale. Use `requirement <Lab-id-or-text>` or `section <document:clause>` for reverse lookup. If `resume` repeats a case already in the plan/source, repair only the stale handoff in `ROADMAP-INDEX.json`; do not resync unchanged Lab inputs.

| Question | Command |
| --- | --- |
| What do I implement next? | `python tools/query_rti_work.py resume` |
| Which independent 2025 files are currently claimed for parallel work? | `python tools/query_parallel_rti_lanes.py` |
| Is the next item runnable, backlog shaping, or external review? | `python tools/query_rti_work.py queue --summary --compact` |
| What are its acceptance criteria? | `python tools/query_rti_work.py ready --summary --compact` |
| Where is one test, and what does it prove? | `python tools/query_rti_work.py case <plan-id> --summary --compact` |
| Which tests cover a requirement? | `python tools/query_rti_work.py requirement <requirement-id> --summary --compact --limit 5` |
| Which tests cover a subsection? | `python tools/query_rti_work.py section hla-1516.1-2025:clause-11.4.1 --summary --compact --limit 5` |
| Which cases exercise an API/service or tag? | `python tools/query_rti_work.py search <API-name-or-tag> --summary --compact --limit 5` |
| Which plan rows are in one C++ test file? | `python tools/query_rti_work.py source <path-substring> --summary --compact --limit 5` |
| Which C++ tests still lack plan rows? | `python tools/query_rti_work.py unplanned --summary --compact --limit 5` |
| Which plan rows need a mapping or explicit disposition? | `python tools/query_rti_work.py unmapped --summary --compact --limit 5` |
| What is the direct requirement crosswalk? | `python tools/query_rti_work.py matrix <case-or-lane> --group-by requirement --summary --compact --limit 5` |
| What is the subsection crosswalk? | `python tools/query_rti_work.py matrix <case-or-lane> --group-by section --summary --compact --limit 5` |
| What are the 2025 requirement mapping totals for C++ and Java, by requirement and section? | `python tools/export_2025_requirement_coverage.py` |
| Which promoted C++ TCK scenarios lack saved passing evidence for a subsection? | `python tools/query_cpp_tck_evidence.py --results <results-file-or-directory> --section hla-1516.1-2025:clause-8.6` |
| Which mapped C++ TCK subsection is the smallest remaining evidence slice? | `python tools/query_cpp_tck_evidence.py --results <current-evidence-directory> --next-sections --limit 10` |
| Which plan heading discusses a topic? | `python tools/query_rti_work.py plan <topic> --summary --compact` |
| I know a topic, not its identifier | `python tools/query_rti_work.py search <terms> --summary --compact --limit 5` |
| Did my lane's mappings drift? | `python tools/query_rti_work.py check --lane <tag> --summary --compact` |
| Is the saved roadmap/traceability snapshot current? | `python tools/query_rti_work.py dashboard --summary --compact` |

Replace placeholders and quote multi-word titles. `roadmap` finds families; `item` previews five cases. Case cards show source, direct 2025 clauses, APIs, and focused CTest selectors. `focus`/`check --lane` take lane tags. `unplanned` and `unmapped` report missing plan rows or mappings. Keep family tags on real Catch2 rows; `ready --include-source-only` includes unmapped source tests. `gaps --family` is family-local; omit it for global gaps. `check` audits the live plan; `check --historical` audits history. Supersede old history instead of rewriting it. Use exact case/matrix handles; reserve `--verbose` for detail. `requirement`/`matrix` also show C++ TCK catalog crosswalks, not execution or conformance evidence.

The parallel-lane query defaults to active work; use `--state ready` to find unclaimed lanes or `--state all` for completed history too.

The 2025 export reads the pinned IEEE 1516.1/1516.2-2025 corpus, Catch2 plan,
C++ TCK catalog, and Java TCK catalog/contracts without syncing the Lab; it
writes row-per-requirement, by-section, and by-document CSVs under
`out/reports/`. It reports C++ Catch2/TCK and Java TCK mapping totals side by
side. Mappings and catalog status are traceability metadata, not test-run or
conformance results.

`query_cpp_tck_evidence.py` compares saved result records with each catalog
scenario's declared callback models. Its passing witnesses are local execution
evidence only, not independent-provider interoperability or conformance.
Pass every relevant saved-evidence directory with another `--results`; the
ranked subsection view considers promoted `cpp-tck.*` scenarios with canonical
IEEE 1516.1-2025 or IEEE 1516.2-2025 section identifiers. Other identifiers are
listed separately instead of being treated as current-standard coverage.
Catalog scenarios with an explicit adapter blocker are shown separately and do
not become runnable gaps until that adapter is available.

For a source-verified Lab attribution defect, preserve its requirement ID and
record both clauses in `known-issues.json`; cite that issue in the mapping
override. Queries retain the exported and corrected clauses; never rewrite the
pinned mapping silently. Internal tests need a lane and explicit no-normative
mapping disposition.

## Work loop

1. Read `resume` once. Use a returned `new-case-needed` slice; if it asks for a
   family, take the recommended `work` handle. Query a topic only if the pointer
   is stale. A gap inventory is a coverage pointer, not an implementation task.
2. Read the selected `case` or `matrix` only when source or crosswalk detail is
   needed; open its source, then implement one bounded RTI behavior.
3. Build the named target, run focused C++ tests, update its mapping/evidence,
   and leave one concrete next handoff.

If a handoff is missing, repair it; avoid broad searches and Lab re-syncs.
Unmapped internal tests need a disposition, not an invented requirement.
Passing development tests are not conformance evidence.

Sources: [roadmap index](ROADMAP-INDEX.json) for tasks; [Catch2 plan](../../compliance/requirements-lab/catch2-test-plan.json) for mappings. The [command guide](QUERY-GUIDE.md) and [history](QUERY-CARD-HISTORY.md) are optional; search a heading instead of loading them whole.
