# Umbra roadmap quickstart

This is the short handoff for continuing the C++ RTI work. It reads the pinned
roadmap index and Catch2 plan; it does not rescan or re-sync the Requirements
Lab. Use one bounded query, select one lane, then run that lane's gate.

## First read

```powershell
python tools/query_rti_work.py resume
```

The card reports live roadmap counts, mapped/unmapped Catch2 cases, source
health, one next slice (or one recommended family), and copyable handles. The
same card is available as JSON:

```powershell
python tools/query_rti_work.py resume --summary --compact --json
```

Use the detailed card only when needed:

```powershell
python tools/query_rti_work.py dashboard --summary --compact
```

## One bounded work loop

```powershell
# Choose a family or see its current action state
python tools/query_rti_work.py roadmap <family-or-api-or-section> --summary --compact
python tools/query_rti_work.py work <family-id> --summary --compact

# Choose one exact lane and inspect its cases
python tools/query_rti_work.py focus <lane-tag> --summary --compact
python tools/query_rti_work.py check --lane <lane-tag> --summary --compact

# Run only the CTest filter printed by the lane/case card
ctest --test-dir .build -C Debug -R "<printed-filter>" --output-on-failure
```

If the indexed queues are exhausted, `resume`/`dashboard` prints the indexed
active handoff when one is queued, otherwise a small family choice. Do not
replace that with a repository-wide search. A family
with `evidence-complete` is traceability history, not a new implementation
target; `new-case-needed`, `implementation`, `mapping`, and `external-review`
are explicit next-action states.

The index may also carry one deliberate `active_handoff`. When present, the
default `ready`/`resume` card names exactly one new C++ case, its target source
file, acceptance criteria, and a mapping seed from an existing case. The seed
is only a starting point: add the new plan row with its own direct
Requirements-Lab-to-2025-subsection pairs before claiming evidence. This keeps
new-case selection explicit without turning broad coverage gaps into an
accidental work queue. The proposed lane is pending until that row exists;
the card emits post-mapping focus/check commands for the follow-up gate.

## Exact traceability joins

Every mapped Catch2 row joins these fields directly:

`plan/test id → C++ source → Requirements-Lab id → canonical 2025 document:clause subsection → C++ API surface → lane/CTest handle`

Use the shortest exact query that you already have:

```powershell
python tools/query_rti_work.py case <plan-id-or-exact-test-title> --summary --compact
python tools/query_rti_work.py trace <test-id-or-lab-id-or-2025-section> --summary --compact
python tools/query_rti_work.py matrix <test-id-or-lab-id-or-2025-section-or-lane> --summary --compact
python tools/query_rti_work.py requirement <lab-requirement-id> --summary --compact
python tools/query_rti_work.py section <document:clause-or-clause-number> --summary --compact
```

`requirement_section_mappings`/`requirement -> standard subsection` is the
direct pair list. It is not inferred by zipping separate requirement and
section arrays. A section query accepts either a canonical key such as
`hla-1516.1-2025:clause-8.18.1` or the exact clause shorthand `8.18.1`.

When a requirement or section has no mapped case yet, the query prints a
bounded uncovered-record preview with the pinned statement and source. This
is the handoff for adding one deliberately mapped C++ case:

```powershell
python tools/query_rti_work.py gaps --family <family-id> --clause <document:clause> --summary --compact
```

## Mapping and source queues

```powershell
python tools/query_rti_work.py unmapped --disposition unclassified --summary --compact --limit 12
python tools/query_rti_work.py unmapped --disposition explicit --summary --compact --limit 12
python tools/query_rti_work.py unlocated --summary --compact --limit 12
python tools/query_rti_work.py unplanned --path ieee1516_2025 --summary --compact --limit 12
```

`unclassified` means a mapping decision is still needed. `explicit` means the
case intentionally has no standalone Lab requirement. Source-only and older
fixtures are reconciliation diagnostics; they do not become 2025 evidence
until a plan row carries explicit requirements and sections.

## Requirements-Lab issue policy

The 2025 export is pinned. Query the separate issue ledger instead of
re-reading the Lab:

```powershell
python tools/query_rti_work.py lab-issues --summary --compact
```

Do not silently change a Lab id or clause to make a test pass. Record a new
recurrence in `compliance/requirements-lab/known-issues.json` and the note in
`docs/planning/REQUIREMENTS-LAB-ISSUES.md` when the defect is genuinely in the
Lab extraction or presentation.

## Current completed seam

The indexed snapshot is 1,227 Catch2 cases (1,163 mapped), including 110
process-boundary cases and 5,260 indexed assertions. The live `check` reports
64 intentional explicit-disposition rows and zero unclassified mappings; use
the exact commands below for one slice instead of reopening the full plan.

`process-tso-directed-interaction-callback-gating` is the current
evidence-complete configured-process directed TSO slice. It retains one
timestamped directed interaction and its matching grant while the receiver's
callback gate is disabled, then delivers the interaction before the grant after
re-enable with TIMESTAMP/TIMESTAMP metadata. It carries 62 assertions, 21
direct Lab requirements, 16 canonical 2025 sections, and 18 official C++ API
surfaces. The preceding multi-recipient fanout, multiple-message FIFO,
changed-lookahead, timestamped-attribute, retraction-lifetime, restore-baseline,
regional, suppression, and single-receiver ordering cases remain separately
queryable.

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-directed-interaction-callback-gating --summary --compact
python tools/query_rti_work.py focus process-tso-directed-interaction-callback-gating --summary --compact
python tools/query_rti_work.py trace "RTIambassadors retain a timestamped directed interaction while callbacks are disabled" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-directed-interaction-callback-gating --summary --compact
python tools/query_rti_work.py check --lane process-tso-directed-interaction-callback-gating --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors retain a timestamped directed interaction while callbacks are disabled$" --output-on-failure
```

The preceding multi-recipient fanout lane remains independently queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-interaction-fanout-integration --summary --compact
python tools/query_rti_work.py focus process-tso-interaction-fanout --summary --compact
python tools/query_rti_work.py trace "RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-interaction-fanout-integration --summary --compact
python tools/query_rti_work.py check --lane process-tso-interaction-fanout --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant$" --output-on-failure
```

The regional TSO callback-gating handoff is now complete: its focused C++ case
has 69 assertions, 28 Lab requirements, 16 canonical 2025 sections, 20
official C++ API surfaces, and an exact CTest selector. `ready` therefore
returns bounded family choices for the next slice; use the family-scoped card
rather than reopening the unchanged Requirements Lab:

```powershell
python tools/query_rti_work.py ready --family transport-and-conformance --summary --compact
python tools/query_rti_work.py focus process-tso-regional-interaction-callback-gating --summary --compact
python tools/query_rti_work.py trace "RTIambassadors retain a timestamped regional interaction while callbacks are disabled" --summary --compact
```

The implementation/index files are the source of truth for routine work:

- `docs/planning/ROADMAP-INDEX.json` — bounded roadmap, queue, and mapping snapshot
- `compliance/requirements-lab/catch2-test-plan.json` — C++ case mappings
- `tools/query_rti_work.py` — read-only query surface
- `docs/planning/QUERY-GUIDE.md` — detailed command reference

The process-boundary order-control extension is also directly indexed. Its
ordinary declaration case now carries the public Change Interaction Order Type
call, an explicit source pointer, six Lab requirements, five canonical 2025
sections (including §8.26.4), and the exact owning lane. Start with:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-interaction-declaration-integration --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-interaction-declaration-integration --summary --compact
python tools/query_rti_work.py check --lane order-type-control --summary --compact
```

The configured-process object-registration case now owns the public Change
Default Attribute Order Type and Change Attribute Order Type seams as separate
query handles:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-object-registration-integration --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-object-registration-integration --summary --compact
python tools/query_rti_work.py focus order-type-control --summary --compact
ctest --test-dir .build -C Debug -L "^order-type-control$" --output-on-failure
```
