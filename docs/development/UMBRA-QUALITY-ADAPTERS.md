# Umbra quality adapters

The reusable gates are [C++ common quality](../../quality_gates/docs/CPP-COMMON-QUALITY.md),
[artifact budgets](../../quality_gates/docs/CPP-ARTIFACT-QUALITY.md), and
[worktree hygiene](../../quality_gates/docs/WORKTREE-HYGIENE.md). Their policies and input reports carry
project-specific paths, targets, selectors, and thresholds. Keep Umbra's
Requirements Lab, roadmap, and CMake details here or in Umbra-specific tools.

## Existing producers

- `tools/check_source_file_sizes.py` is Umbra's existing CI source-size gate.
  It counts directly included `.cpp` bodies, rejects source-file includes and
  single-consumer `.inc` files, and fails on blockers even in `--report` mode.
  The portable source gate additionally covers `.inl`, `.ipp`, and `.tpp`.
- `tools/report_compile_times.py build --write-json out/quality/compile.json`
  produces the `records` and `header_fanout` arrays consumed by the portable
  graph gate. The graph gate can read CMake's File API directly, or another
  producer can emit its normalized `targets` report.
- `tools/build_quality.py capture build --label my-profile --status passed
  --run-id $runId --snapshot-output out/quality/build-snapshot.json` writes the
  Umbra build snapshot. The portable verdict example expects a normalized
  `{"schema":1,"gate":"build","passed":true,"run_id":"...","manifest_sha256":"..."}`
  report. Use the portable `run-build` wrapper around the actual command, or
  derive that report from the captured build result and attach the exact
  manifest digest. The snapshot keeps `build_status`,
  `quality_status`, and `overall_passed` separate. Umbra's quality status
  includes source-size blockers, mapping health, source health, and roadmap
  index freshness.
- The `tests` gate's manifest must use real Umbra test selectors and direct
  requirement mappings, or explicitly classify internal tests. Its generic
  source-anchor and result checks do not replace Requirements Lab validation.

For an existing CMake package consumer, the generic package gate can use
`"cache":"out/example-consumer/CMakeCache.txt"` with
`"cache_path_keys":["ExamplePackage_DIR","EXAMPLE_API_INCLUDE_DIR"]` instead
of a normalized `binding_report`. Choose the actual cache keys from that
consumer; do not copy these example names into a policy unchanged.

## Local worktree inventory

```powershell
$runId = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')
$codexRoot = Join-Path $env:USERPROFILE '.codex/worktrees'
python quality_gates/bin/worktree_hygiene.py --repo . --scan-root "codex=$codexRoot" --scan-root "umbra-out=out/worktrees" --base-ref HEAD --max-worktrees 16 --max-unregistered 0 --max-logical-gib 100 --run-id $runId --output out/build-metrics/worktree-hygiene.json
```

If `.claude/worktrees` exists, add
`--scan-root "claude=.claude/worktrees"`; use the configured location when
Claude worktrees are stored elsewhere. Supply only roots present on the host,
because a configured missing root fails the gate.

Read the latest `out/build-metrics/worktree-hygiene.json` before acting; the
registered worktree list can change while an audit is in progress. The report
includes directories under the configured roots that are not registered to
Umbra; check ownership before interpreting them as leftovers.
Codex-managed checkouts should be archived through their owning app chat.
[Claude Desktop's documentation](https://code.claude.com/docs/en/desktop)
places local worktrees under `<project-root>/.claude/worktrees/` by default,
allows a custom location, and documents session archiving.

## Artifact example

For the measured Umbra MinGW Debug profile, a representative 937,955,643-byte
Catch2 executable contains 869,500,571 bytes of debug sections. This points
first to debug information replicated through static linkage; it does not by
itself establish how much machine code is duplicated. The portable artifact
gate measures sections and applies the profile's configured budgets.
