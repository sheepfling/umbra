# RTI query card

Run at repository root; queries use the pinned baseline, not a Lab sync. Pick one row.

## Fast path

Use `resume` once, then `ready`, `case <handle>`, and `matrix <handle>` for the selected slice; these bounded lookups replace opening the roadmap or Catch2 plan wholesale. Use `requirement <Lab-id-or-text>` or `section <document:clause>` for reverse lookup. If `resume` repeats a case already in the plan/source, repair only the stale handoff in `ROADMAP-INDEX.json`; do not resync unchanged Lab inputs.

| Question | Command |
| --- | --- |
| What do I implement next? | `python tools/query_rti_work.py resume` |
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
| Which plan heading discusses a topic? | `python tools/query_rti_work.py plan <topic> --summary --compact` |
| I know a topic, not its identifier | `python tools/query_rti_work.py search <terms> --summary --compact --limit 5` |
| Did my lane's mappings drift? | `python tools/query_rti_work.py check --lane <tag> --summary --compact` |
| Is the saved roadmap/traceability snapshot current? | `python tools/query_rti_work.py dashboard --summary --compact` |

Replace placeholders; quote multi-word titles. Topic lookups are count-only: `roadmap <topic> --summary --compact` finds a family; `item <family-id> --summary --compact` previews five cases. Don't expand a family to trace one test. Case cards show C++ source, direct 2025 subsection pairs, APIs, and focused handles; use their case-specific CTest selector. `focus`/`check --lane` take lane tags. `unplanned` finds tests missing plan rows; `unmapped` finds rows needing mapping or disposition. Keep family tags tied to real Catch2 rows; `ready --include-source-only` includes unmapped source tests. `check` validates the live plan; `check --historical` audits history. Mark replaced history `superseded_by`; don't rewrite it. Use exact case/matrix handles for direct mappings and `--verbose` only when needed.

## Work loop

1. Read `resume` once. If it returns a `new-case-needed` slice, treat that as
   the selected task; `ready --summary --compact` adds acceptance criteria and
   the seed's direct requirement-to-clause pairs. If it asks you to choose a
   family, use the one recommended family and its `work` handle; only use a
   targeted `roadmap <topic/API>` lookup when that pointer is missing or stale.
   A `gap_inventory_head` is a coverage pointer, not an implementation task.
2. Read the selected seed `case` or requirement/section `matrix` only when
   source detail or crosswalk detail is needed.
3. Open the returned source location. Implement one bounded RTI behavior.
4. Build its named target and run its focused C++ tests.
5. Update that case's mapping/evidence and leave one concrete next handoff.

If a handoff is missing, repair that pointer; avoid broad context collection or
Lab re-syncs. Unmapped internal tests need an explicit disposition, not an
invented requirement. Passing development tests are not conformance evidence.

Sources: [roadmap index](ROADMAP-INDEX.json) for the next task; [Catch2 plan](../../compliance/requirements-lab/catch2-test-plan.json) for mappings.
The [command guide](QUERY-GUIDE.md) and [archived examples](QUERY-CARD-HISTORY.md)
are optional; search a specific heading instead of loading them whole.
