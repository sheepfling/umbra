# Umbra roadmap quickstart

Start here to continue the C++ RTI work. The checked-in index and plan supply
the current handoff; do not copy changing case titles or counts into this page.

## First read

```powershell
python tools/query_rti_work.py resume --summary --compact
```

`resume` shows one current task or, if none is queued, a family scope choice.
When no executable case is ready, narrow with one `roadmap <topic>` and focused
`lanes` query; if none fits, add an exact handoff (API behavior, target,
acceptance, mapping seed). Do not scan source or expand a whole family. Use only
the follow-up you need:

```powershell
# Only if resume offers a family choice: resolve that one family
python tools/query_rti_work.py work <family-id> --summary --compact
# If no task is queued, narrow one topic to its roadmap family and focused lanes
python tools/query_rti_work.py roadmap <topic-or-official-API> --summary --compact
python tools/query_rti_work.py lanes --family <family-id> --focused --summary --compact --limit 12
# One case's source, direct requirement -> 2025 subsection pairs, and focused CTest
python tools/query_rti_work.py case <exact-plan-id> --summary --compact
# Reverse views for the selected case, lane, or family
python tools/query_rti_work.py matrix <exact-handle> --group-by requirement --summary --compact
python tools/query_rti_work.py matrix <exact-handle> --group-by section --summary --compact
```

For a topic that may match existing tests, use one bounded search:

```powershell
python tools/query_rti_work.py search <terms> --summary --compact --limit 5
```

Each hit shows its source, total direct-pair count, and one exact requirement
→ 2025 subsection pair as a compact preview, followed by its exact `case`
command for the complete mapping and focused test selector. For roadmap-family
discovery rather than test discovery, use
`roadmap <topic-or-official-API> --summary --compact`; it returns family counts
and a direct `work` handle. To locate an implementation-plan heading without
opening the full plan, use `plan <topic> --summary --compact`; it returns the
matching section path and line. `item <family-id> --summary --compact`
previews five cases with exact `case` handles.

For reverse traceability, query the pinned requirement id or canonical
`document:clause` subsection directly—don’t search the source tree or load the
whole test plan:

```powershell
python tools/query_rti_work.py requirement requirement-candidate-content-clauses-09-data-distribution-management-page-236-l143-42 --summary --compact
python tools/query_rti_work.py section hla-1516.1-2025:clause-9.10 --summary --compact
```

Both return matching Catch2 plan ids; open one result with `case <plan-id>` to
see its exact direct requirement-to-subsection pairs and focused CTest handle.

Open only the returned source location and run the printed focused CTest
selector. A new-case mapping seed is not evidence: the new row needs its own
direct pairs. No Requirements-Lab rescan or broad guide read is needed.
Use [QUERY-CARD.md](QUERY-CARD.md) or [QUERY-GUIDE.md](QUERY-GUIDE.md) only for
a specific command question.

## Traceability contract

Each mapped case exposes this direct join:

`plan id → C++ test/source → Lab requirement → canonical 2025 subsection → API → focused CTest`

Pairs are explicit, not paired by list position. Passing cases are development
evidence until separate conformance review/promotion gates are satisfied.

The checked-in [roadmap index](ROADMAP-INDEX.json) owns the current task;
[the Catch2 plan](../../compliance/requirements-lab/catch2-test-plan.json) owns
case mappings. Change those records when work changes, not this quickstart.

See [historical query examples](QUERY-EXAMPLES.md) only when an example is
needed. They are reference material, not a work queue.
