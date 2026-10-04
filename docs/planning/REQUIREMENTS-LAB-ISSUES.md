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
python tools/query_rti_work.py lab-issues RL-267 --summary --compact
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

### RL-267 — page-67 Initiate Federate Save label clause ownership recurs

The pinned candidate
`requirement-candidate-content-clauses-04-federation-management-page-067-l22-3`
describes the Initiate Federate Save label, whose source belongs to **§4.20**,
but the export attaches it to **§4.21.1**. The mismatch surfaced again while
mapping the single-federate process save-not-complete test. The test verifies
that the label reaches the callback, but its focused mapping deliberately
leaves that assertion unmapped rather than attributing it to Federate Save
Begun.

This is a new recurrence of the page-67 service-boundary issue documented as
RL-082, not a replacement of that historical record. Preserve the candidate
id and exported clause until the Lab refinement is reviewed; do not resync or
mutate the pinned export as part of RTI work.

Source: `content/clauses/04-federation-management-page-067.tex`, line 22
(page 67), document `hla-1516.1-2025`.

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

### RL-268 — unrelated global Catch2 plan validation drift

The global plan audit
`python tools/requirements_lab.py check-plan --plan compliance/requirements-lab/catch2-test-plan.json`
reports eight findings in other work lanes: two rows lack a non-empty
`next_action`; three references point to the absent API surface
`api.2025.cpp.rtiambassador.setservicereportingswitch.86951e8b3e77`; and one
timestamped directed-interaction row contains four requirement/API identifiers
absent from the pinned Lab surfaces. The affected rows concern timestamped
attribute update, negotiated/confirmed divestiture, timestamped directed
interaction, and process restore status.

Impact: the global Catch2 requirements-plan gate is not green, but these
unrelated findings do not invalidate the independently passing promoted
portable TCK interaction-publication slice or its IEEE 1516.1-2025 and
1516.2-2025 contracts.

Workaround: repair each affected row only after checking its exact 2025 source
and Lab identity. Keep this drift separate from portable TCK evidence; do not
resynchronize or rewrite the pinned Requirements Lab export to silence the
validator.

### RL-269 — ownership-query test selectors retained the old source path

The ownership-query requirements/API contracts and ownership-check requirements
contract still selected the RTI-owned MOM query case from the large federation
management test file after it had been extracted to
`cpp/tests/ieee1516_2025_rti_owned_mom_ownership_query_catch2.cpp`. This made the
focused ownership-check requirements traceability test fail with contract drift;
the requirement IDs and clause mappings themselves were unchanged.

Resolution: updated the seven stale selectors to the focused 2025 test path.
The four requirements/API traceability tests for the ownership-query and
ownership-check contracts now pass, as do the mapping checks for both lanes.
No pinned Requirements Lab export or normative mapping was changed.
