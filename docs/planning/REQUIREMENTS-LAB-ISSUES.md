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

### RL-270 — extracted service-report tests retained old selectors

The MOM service-reporting requirements contract contained 64 unique selectors
(176 references) that still named the federation-management monolith after the
corresponding 2025 tests had moved into focused units. The Attribute Ownership
Acquisition API contract also retained one selector for the focused multi-acquirer
Release Denied case.

Resolution: changed only those selectors to the unique source files containing
the matching `TEST_CASE` declarations. The MOM service-reporting, ownership-
acquisition API, and negotiated-divestiture requirements/API traceability tests
now pass 4/4. Requirement IDs and normative mappings are unchanged; the pinned
Requirements Lab export was not synchronized or modified.

### RL-276 — federation MOM content-report case remained classified as disabled

The bounded query reported the federation-scoped FOM/MIM content-report case as
`disabled-source-artifact`, source-unlocated, and zero-assertion, with a note
claiming its declaration remained inside `#if 0`. The current tree instead has
an active focused test at
`cpp/tests/federation_mom_current_fdd_catch2.cpp:324`; its target builds and its
exact CTest passes.

Resolution: reconciled the existing plan row with the active 53-assertion
development-profile case, its `HLA_EVOKED` callback model and focused tag. The
direct clause-4 mapping and seven API surfaces are unchanged. The focused test,
trace, MOM lane check, and federation content-report check pass; no pinned
Requirements Lab export or IEEE 1516.1-2010 source or mapping changed.

### RL-277 — connection-loss and support-switch contracts retained old paths

The connection-loss lane sweep found stale source-qualified test selectors for
extracted connection-loss, Federate Lost, and support-switch cases, as well as
implementation-symbol paths still naming the old federation registry and RTI
ambassador translation units.

Resolution: updated only the local selector and implementation-source paths.
The #510/#521/#529 behavior tests and seven related requirements/API
traceability CTests pass 10/10. The #510 mapping still has eight direct pairs
across six canonical 2025 clauses and seven API surfaces. No requirement or API
mapping, pinned Lab export, or IEEE 1516.1-2010 file changed.

### RL-278 — timestamped-interaction contracts retained pre-extraction paths

The timestamped-interaction requirements and API traceability tests found old
test selectors still pointing into the federation-management and connection
monoliths, plus registry source-symbol paths that no longer contained the
referenced definitions.

Resolution: corrected only the local selectors and implementation-source
paths. Traceability CTests #2092/#2093 and the related connection-loss gates
#1910/#1911 pass 4/4; strict-less-than and later-TAR cases #514/#531 and all 26
cutoff-lane tests pass. Requirement, clause, and API mappings and the pinned
Lab export are unchanged; no IEEE 1516.1-2010 source or mapping changed.

### RL-279 — asynchronous-delivery contracts retained monolith selectors

The asynchronous-delivery requirements and API traceability gates still
pointed two extracted cases at the federation-management monolith instead of
their current focused source files.

Resolution: corrected only the eight source-qualified selector occurrences.
The asynchronous-delivery gates #2071/#2072 and all eight affected traceability
gates pass. The #530 cutoff case and all 26 cutoff-lane tests pass; no
requirement, clause, or API mapping, pinned Lab export, or 2010 source changed.

### RL-280 — federate-lookup and support-switch contracts retained moved source references

The federate-lookup requirements gate referenced `federateNameFor` in the old
`federation_registry.cpp`, and the support-switch table contract still named
two tests in the federation-management monolith after their extraction.

Resolution: updated the registry source path and the two selectors to their
current definitions. CTests #1917 and #1989 pass, along with the extracted
DELETE_OBJECTS case #505 and its other selected traceability gates. No
requirement, clause, or API mapping, pinned Lab export, or IEEE 1516.1-2010
source changed. This recurrence is tracked separately from RL-277.

### RL-281 — declaration-failure plan rows retained duplicate monolith locations

Three extracted declaration-failure rows each contained a focused-file
`source_location` plus a second stale location in the federation-management
monolith. Consumers could silently select the stale duplicate.

Resolution: removed only the three duplicate monolith locations. The case
queries resolve to the focused source files; requirement, clause, and API
mappings are unchanged. No pinned Lab export or IEEE 1516.1-2010 source or
mapping changed.

### RL-282 — object-name reservation contracts retained pre-split source paths

The object-instance-name reservation requirements and API traceability gates
found implementation symbols and test selectors still pointing at the former
registry, RTI-ambassador, and test monoliths after those definitions were split
into focused files.

Resolution: corrected only the local source-symbol and test-selector paths.
The eight selected connection-loss, resign-action, support-switch, and
object-name traceability CTests pass; requirement, clause, and API mappings
are unchanged. No pinned Lab export or IEEE 1516.1-2010 source or mapping
changed.

### RL-283 — NoAction plan row retained a duplicate monolith source pointer

A duplicate-key audit of the Catch2 plan found the extracted #503 NoAction
case still had both its stale federation-management monolith pointer and its
current focused-source pointer.

Resolution: removed only the stale monolith `source_location`. The case query
resolves to the focused 2025 source; requirement, clause, and API mappings
are unchanged. No pinned Lab export or IEEE 1516.1-2010 source or mapping
changed.

### RL-284 — timestamped attribute-update contracts retained moved source pointers

The timestamped default-region and regional attribute-update regulation-
reenablement traceability gates still pointed moved RTI-ambassador and
federation-registry symbols at their former monolith translation units. The
regional case's contract selectors also still named its pre-extraction test
location.

Resolution: corrected only those implementation-source pointers and the 31
exact selectors for the extracted regional case. Gates #2098/#2099 and
#2110/#2111 pass, as do both focused behavior cases (#485 and #226).
Requirement, subsection, and API mappings are unchanged; no pinned Lab export
or IEEE 1516.1-2010 source or mapping changed.

### RL-285 — ownership contracts retained pre-split source and test pointers

The regular-acquisition and negotiated-divestiture traceability gates found
registry symbols still attributed to `federation_registry.cpp` and service-
report or release-denied test selectors still naming the federation-
management monolith after those cases had been extracted.

Resolution: corrected the local implementation-source paths and selectors in
the four affected ownership contracts. Gates #2043/#2044/#2051/#2052 pass,
alongside connection-loss and support-switch gates #1910/#1911/#1990/#1992;
the extracted negotiated-cancellation case #508 passes. Requirement,
subsection, and API mappings are unchanged; no pinned Lab export or IEEE
1516.1-2010 source or mapping changed.

### RL-286 — 2025 traceability contracts retained pre-extraction paths

The asynchronous-delivery API contract still located both ambassador methods
in `umbra_rti_ambassador.cpp` after their same-translation-unit extraction.
The same focused CTest sweep exposed stale 2025 source/test selectors in the
interaction-region, Convey Region Designator Sets, and support-switch
contracts. Traceability CTests #1920, #1921, #1951, #1969, and #2031 failed
with contract drift.

Resolution: corrected only implementation-source paths and 2025 C++ test
selectors in the five local contracts. All five traceability tests and the
15-test `asynchronous-delivery` CTest label now pass. Requirement IDs,
subsections, API mappings, and IEEE 1516.1-2010 sources remain unchanged.

### RL-287 — handle-decoding API contract retained monolith source paths

The seven basic public handle-decoding methods moved into a same-translation-
unit fragment, but their API contract still located them in
`umbra_rti_ambassador.cpp`. Traceability CTest #1957 failed for all seven
source symbols.

Resolution: updated only those seven source paths to the extracted fragment;
the MessageRetractionHandle locator remains on its separate implementation.
CTests #1957 and #221 pass. The explicit no-requirement disposition and all
eight API-surface mappings are unchanged; no IEEE 1516.1-2010 source or
mapping changed.

### RL-288 — MOM service-report traceability retained extracted-source pointers

Focused 2025 traceability checks #1920 and #1953 exposed stale Requirements
Lab source locators after ambassador service/report methods had moved into
same-translation-unit fragments. The MOM service-report contract had 179
stale source paths across 66 moved symbols, and one test selector still named
the federation-management monolith for the federated-MOM content case.

Resolution: updated only implementation-source paths in the MOM service-
report and interaction-region contracts, plus the single stale test selector.
Requirement IDs, clauses, API mappings, and IEEE 1516.1-2010 records are
unchanged. CTests #1920, #1921, #1953, #48, #263, #881, and #1242 pass.
