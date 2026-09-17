# Requirements Lab issue ledger

This note records defects and usability rough edges in the pinned IEEE
1516.1-2025 Requirements Lab export. It is deliberately separate from the
export and from the C++ test plan: issue entries explain why a requirement or
section may need review, but they never rewrite a Lab identifier or silently
move evidence between subsections.
Historical observations such as RL-030 remain in
`docs/testing/REQUIREMENTS-LAB-OBSERVATIONS.md`; this live ledger lists only
issues that still apply to the current pinned export.

The machine-readable ledger is
[`compliance/requirements-lab/known-issues.json`](../../compliance/requirements-lab/known-issues.json).
Use the bounded query from the repository root:

```powershell
python tools/query_rti_work.py lab-issues --summary --compact
python tools/query_rti_work.py lab-issues RL-177 --summary --compact
```

## Policy

- The pinned 2025 export remains the requirements source of truth.
- Do not resynchronize merely because a query exposes a defect; resynchronize
  only when the pinned export changes.
- A recurrence after the existing issue range gets a new issue id rather than
  being folded into an older entry.
- Keep both the exported Lab clause and the reviewed normative clause visible
  until the upstream extraction is corrected.

## Open issues

### RL-177 — page-132 object-management clause boundary

The Lab records
`requirement-candidate-content-clauses-06-object-management-page-132-l107-30`
and
`requirement-candidate-content-clauses-06-object-management-page-132-l113-32`
under exported subsection **6.18.1**. Their statements describe the Delete
Object Instance postconditions on page 132, including the Flush Queue Request
and disable-time-constrained cases; the source heading places those
postconditions in **§6.17.4**. The following §6.18 heading belongs to the Local
Delete Object Instance service.

Source: `content/clauses/06-object-management-page-132.tex`, lines 107–118
(page 132), document `hla-1516.1-2025`.

Impact: a forward gap query can display the requirements under the wrong
subsection, making correct evidence appear uncovered or encouraging an
incorrect test-to-standard mapping.

Workaround: retain the stable Lab ids and exported `6.18.1` value in plan
data, cite the source-heading correction as `§6.17.4` in review notes, and do
not mutate the Requirements Lab export in this repository.

Reproduce the bounded observation with:

```powershell
python tools/query_rti_work.py gaps --family object-ddm-ownership --clause 6.18 --summary --compact --limit 40
```

## Resolved local harness rough edge

While adding the next bounded ownership handoff, the Catch2 plan validator
treated a deliberately **planned** row as executable and rejected its
not-yet-declared `TEST_CASE` selector. That made a valid roadmap contract fail
the traceability CTest before implementation could begin. The validator now
recognizes `status: "planned"` alongside the existing disabled and
source-reconciliation states, while `ready` and `focus` continue to expose the
row as planned and source-unlocated. This is a local validation usability fix,
not a change to the pinned Requirements Lab export; keep the planned row's
requirement and 2025 subsection mapping intact until its C++ case is added.
