# Umbra development entry point

Implement the IEEE 1516.1-2025 C++ RTI against the official API and pinned requirements. Start ordinary work with:

```powershell
python tools/query_rti_work.py resume
```

Use the current handoff. For detail, run its exact `case` handle or `ready --summary --compact`, then open the returned
source. Aim for one resume and one detail query before implementation; this is a discovery budget, not a correctness limit.

- Do not resynchronize the Requirements Lab or increment revisions unless requested or an actual upstream change is established.
- Do not read the full roadmap, implementation plan, query guide/history, Catch2 plan, corpus export, or large C++ files
  to
  resume. Use bounded queries and targeted source ranges; command card: [docs/planning/QUERY-CARD.md](docs/planning/QUERY-CARD.md).
- Once a concrete task is identified, implement it. If its pointer is broken, fix that pointer instead of making another
  broad inventory.
- New behavioral evidence belongs in C++ against the official API. Give each case a focused lane tag and exact plan row
  with
  direct requirement-to-canonical-2025-section/subsection mappings. Mark internal tests with no normative mapping; never
  invent
  requirements or validation.
- Build the returned target, run focused CTest selectors, and check changed lane mappings. Expand regression scope when
  shared
  runtime changes warrant it; the full suite is not the default discovery step.
- Keep the current task in `docs/planning/ROADMAP-INDEX.json`, test mappings in
  `compliance/requirements-lab/catch2-test-plan.json`, and one concrete next handoff after verified progress. Do not append
  completion history to quickstarts or command cards.
- Preserve unrelated dirty work. Log newly observed Requirements Lab defects in the existing issue ledger; a recurrence
  of a
  supposedly fixed issue gets a new issue number, not a rewrite of the old issue.

## Source organization

- Split oversized files along cohesive ownership boundaries, not just to lower line counts. Treat 3,000 lines as a warning;
  above 5,000, actively seek a sensible extraction. Aim below 10,000, but keep tightly related code together when splitting
  would harm navigation, dependencies, or testing. Record justified exceptions in `docs/development/source-size-policy.json`
  with a growth guard and reason.
- Use `.cpp`/`.hpp` for implementation and declarations. Use `.inc` only for a fragment with multiple genuine consumers;
  fold
  single-consumer fragments into their owning source. Do not create fragments solely to satisfy size checks.
- Preserve the 2010 and 2025 implementation streams as separate compatibility boundaries. Share only genuinely common
  behavior, and verify source ownership and relevant boundary checks when changing shared code.
- Keep new source, build integration, focused tests/mappings, and roadmap handoff together in version control. Stage only
  task-related paths and preserve unrelated dirty changes.

## Worktree lifecycle

- Reuse the current checkout for short sequential work. Create an isolated worktree only for actual parallel/conflicting
  work;
  record its path and owning task in the handoff.
- At the end of a short-lived work stream, inspect tracked, untracked, ignored output, and commits unreachable from the
  primary
  branch. A clean `git status` does not mean a worktree is disposable.
- Archive Codex-managed worktrees through the Codex app and Claude Desktop worktrees through their owning session. For manual
  worktrees, preserve unique work, then use `git worktree remove` and `git worktree prune`; never delete directories directly
  or force removal based on age alone.
- Run the read-only `quality_gates/bin/worktree_hygiene.py` inventory during workstation cleanup. If a worktree remains,
  record
  its owner and reason in the handoff. Treat unexplained growth in worktree count or logical bytes as a quality finding.
