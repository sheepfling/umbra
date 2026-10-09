# Umbra development entry point

Implement the IEEE 1516.1-2025 C++ RTI against the official API and the pinned
requirements. Start ordinary implementation work with:

```powershell
python tools/query_rti_work.py resume
```

Use its current handoff. If detail is needed, run its exact `case` handle or
`ready --summary --compact`, then open the returned source location. Aim for
one resume query and one detail query before implementation. This is a
discovery budget, not permission to skip necessary correctness checks.

- Do not repeatedly synchronize the Requirements Lab or increment revisions
  when its inputs have not changed. Re-sync only when requested or when an
  actual upstream change is established.
- Do not read the full roadmap, implementation plan, query guide/history,
  Catch2 plan JSON, corpus export, or large C++ translation units to resume.
  Use bounded queries and targeted source ranges. The command card is
  [docs/planning/QUERY-CARD.md](docs/planning/QUERY-CARD.md).
- Once a concrete task is identified, implement it. If its pointer is broken,
  fix that pointer rather than collecting another broad context inventory.
- New behavioral evidence belongs in C++ against the official API. Give each
  case a focused lane tag and an exact plan row with direct requirement-to-
  canonical-2025-section/subsection mappings. Explicitly classify internal
  tests without normative mappings; never invent requirements or validation.
- Build the returned target, run focused CTest selectors, and check changed
  lane mappings. Expand regression scope when shared runtime changes warrant
  it; the full suite is not the default discovery step.
- Keep the current task in `docs/planning/ROADMAP-INDEX.json`, test mappings in
  `compliance/requirements-lab/catch2-test-plan.json`, and leave one concrete
  next handoff after verified progress. Do not append completion history to
  quickstarts or command cards.
- Preserve unrelated dirty work. Log newly observed Requirements Lab defects
  in the existing issue ledger; recurrence of a supposedly fixed issue gets
  a new issue number, not a silent rewrite of the old issue.

## Source organization

- Split oversized files along cohesive ownership and responsibility boundaries,
  not just to lower line-count metrics. Treat 3,000 lines as a warning; above
  5,000, actively look for a sensible extraction. Aim to keep files below
  10,000 lines, but keep tightly related code together when a split would make
  navigation, dependencies, or testing worse. Record justified size exceptions
  in `docs/development/source-size-policy.json` with a growth guard and reason.
- Use ordinary `.cpp` and `.hpp` files for implementation and declarations.
  Use `.inc` only when the same fragment has multiple genuine consumers; fold
  single-consumer fragments into their owning source instead of hiding code in
  an include. Do not create fragments solely to satisfy size checks.
- Preserve the 2010 and 2025 implementation streams as separate compatibility
  boundaries. Share helpers only when their behavior is genuinely common, and
  verify both the source ownership and relevant boundary checks when changing
  shared code.
- Keep new source files, build integration, focused tests/mappings, and the
  roadmap handoff together in version control so another contributor can resume
  the work. Stage only task-related paths and preserve unrelated dirty changes.
