# RTI roadmap query card

This is the detailed query card for resuming Umbra. The genuinely compact
first-read is [ROADMAP-QUICKSTART.md](ROADMAP-QUICKSTART.md); use it before
opening this longer reference. This card's commands
below read the checked-in roadmap index, Catch2 plan, pinned IEEE 1516.1/1516.2
2025 corpus, and derived C++ source locations. They are read-only and do not
rescan or rewrite the Requirements Lab.

The detailed reference is [QUERY-GUIDE.md](QUERY-GUIDE.md). Open it only after
one of the bounded commands below identifies the exact slice.

## One-command resume

Run these from the repository root:

```powershell
python tools/query_rti_work.py resume
```

`resume` is the compact first-read card: it returns the current roadmap and
Catch2 mapping counts, one exact next slice (or one recommended family), and
the bounded requirement/section plus execution handles. It keeps the first
read to one queue choice. Use JSON for automation or the richer diagnostic
card only when needed:

```powershell
python tools/query_rti_work.py resume --json
python tools/query_rti_work.py dashboard --summary --compact
python tools/query_rti_work.py status --summary --compact
python tools/query_rti_work.py ready --summary --compact
python tools/query_rti_work.py work --summary --compact
python tools/query_rti_work.py queue --summary --compact
python tools/query_rti_work.py lab-issues --summary --compact
```

`resume` is the preferred first command: it prints the live roadmap
checklist, Catch2/mapping queues, source health, snapshot freshness, latest
completed slice, one next work handoff, and only one open-family choice. Use
`resume --json` when a script needs the same bounded card. The richer
`dashboard --summary --compact` view retains the first three open-family rows
for diagnostics. Descriptive family prose in both cards is bounded; use the
emitted `work`, `focus`, `trace`, or `matrix` handle for the full bounded view.
Neither command reopens the Requirements Lab or prints the whole roadmap.

## Current bounded handoff

The latest completed slice is the 52-assertion, nine-requirement,
seven-subsection, 11-API `HLA_IMMEDIATE` process-boundary restored
ownership-assumption case. It is now a direct mapped row rather than a pending
active handoff; use these exact handles rather than searching the full source
tree:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver restored ownership-assumption work under HLA_IMMEDIATE through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --group-by requirement --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --group-by section --summary --compact
python tools/query_rti_work.py focus process-federation-restore-work-item-ownership-assumption-immediate --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-work-item-ownership-assumption-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors deliver restored ownership-assumption work under HLA_IMMEDIATE through a configured process endpoint$" --output-on-failure
```

The case keeps the completed HLA_EVOKED public restore-work-item row as a
mapping seed, but its callback-gate behavior and direct requirement/subsection
pairs are independent. It does not claim Evoke Callback behavior, the
already-pushed work-item lane, other restore work-item families, or package,
review, validation, interoperability, or conformance evidence.

The configuration-fallback slice is independently queryable and demonstrates
the intended case-to-standard crosswalk: one Catch2 case, eight assertions,
the exact `requirement-candidate-content-clauses-04-federation-management-page-051-l18-3`
configuration-fallback requirement plus
`requirement-candidate-content-clauses-04-federation-management-page-051-l23-4`
for the no-`RtiAddress` default embedded path, and canonical subsection
`hla-1516.1-2025:clause-4.2.4`. Its matrix therefore exposes two direct
requirement/subsection pairs:

```powershell
python tools/query_rti_work.py case umbra-cpp-connect-configuration-fallback-integration --summary --compact
python tools/query_rti_work.py focus configuration-fallback --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-connect-configuration-fallback-integration --summary --compact
python tools/query_rti_work.py check --lane configuration-fallback --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.Embedded Connect falls back to its default configuration for absent and unknown names$" --output-on-failure
```

This remains development-profile evidence; it does not claim named external
configuration catalogs, package/JUnit/protected-review, validation,
interoperability, or conformance.

The companion additional-settings parse-failure slice keeps the §4.2.4 rule
executable and separately addressable. It maps five Catch2 assertions to
`requirement-candidate-content-clauses-04-federation-management-page-051-l28-5`
and reports `SETTINGS_FAILED_TO_PARSE` while Connect completes:

```powershell
python tools/query_rti_work.py case umbra-cpp-connect-additional-settings-parse-failure-integration --summary --compact
python tools/query_rti_work.py focus configuration-additional-settings --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-connect-additional-settings-parse-failure-integration --summary --compact
python tools/query_rti_work.py check --lane configuration-additional-settings --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.Embedded Connect completes when an optional additional setting cannot be parsed$" --output-on-failure
```

Keep this parse-failure boundary separate from named-configuration fallback,
service-report-directory validation, process endpoint selection, credentials,
and aggregate Connect overload traceability.

The current bounded DDM slices cover the §9.8 ordinary/regional subscription
independence and passive-indicator/advisory boundaries. The independence case
is a 40-assertion development-profile regression mapped directly to lines 57
and 63. The regional declaration-advisory case is a 61-assertion regression
mapped to lines 90, 102, and 120, proving no-Start for passive subscriptions,
active-to-passive Stop behavior, and per-region triple-basis state. The
registered-object companion is a 54-assertion regression that proves the same
passive/active triples gate object discovery and Turn Updates On/Off without a
second discovery on re-enable. Together they cover all 20 requirements
currently filtered for §9.8 in the object-DDM family; timestamped/retraction
paths remain a separate implementation slice:

```powershell
python tools/query_rti_work.py case umbra-cpp-regional-object-attribute-subscription-isolation-integration --summary --compact
python tools/query_rti_work.py focus regional-object-attribute-subscription-isolation --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-regional-object-attribute-subscription-isolation-integration --group-by requirement --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-regional-object-attribute-subscription-isolation-integration --group-by section --summary --compact
python tools/query_rti_work.py check --lane regional-object-attribute-subscription-isolation --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.regional_object_attribute_routing\.catch2\.Embedded regional object-attribute subscriptions remain independent from ordinary declarations$" --output-on-failure
python tools/query_rti_work.py case umbra-cpp-regional-passive-subscription-turn-updates-integration --summary --compact
python tools/query_rti_work.py focus regional-passive-subscription-turn-updates --summary --compact
python tools/query_rti_work.py matrix regional-passive-subscription-turn-updates --summary --compact
python tools/query_rti_work.py check --lane regional-passive-subscription-turn-updates --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.regional_declaration_relevance_advisory\.catch2\.Embedded regional passive subscription changes gate registered-object turn updates per region$" --output-on-failure
python tools/query_rti_work.py gaps --family object-ddm-ownership --clause clause-9.8 --summary --compact --limit 8
```

`work <family-id> --summary --compact` is the bounded family handoff when a
new case is needed. Its `work_mapping` line is derived from the live distinct
Catch2 rows (not stale pointer metadata) and includes case/mapped/assertion,
requirement, canonical-subsection, and direct-pair totals. The same card emits
`work_matrix_requirement` and `work_matrix_section` commands for the exact
family crosswalks.

The card also includes `latest_verified_restore_slice`, a bounded ledger for
implementation fixes that reuse existing mapped cases (for example, the
negotiated-ownership fresh-registry restore fix). Its plan IDs, direct-query
handles, and focused CTest selectors are kept beside the roadmap index so a
regression can be resumed without rediscovering the slice.

`requirement <id-or-text>` and `section <document:clause>` are valid reverse
lookups even before a C++ case is mapped. If no Catch2 row exists, they return
the bounded uncovered pinned-2025 records and normative source statement so a
new test can start from an exact standard anchor.
With `--summary`, mapped reverse-lookups render the same compact matrix rows
as `matrix`: each row keeps the source/assertion counts and direct
`requirement -> standard subsection` pairs without repeating full contract
provenance. Use `test <query>` or `case <exact-handle>` when the full row is
needed.
For standards review, `matrix --group-by requirement` emits one aggregate row
per Requirements-Lab id, while `matrix --group-by section` emits one row per
canonical 2025 subsection. Each row keeps direct pair/test/assertion counts and
a bounded exact case preview, so a reviewer can pivot the crosswalk without
opening the full plan. Passing an exact requirement or subsection query keeps
only that direct key; pass a family or lane for the full bounded slice.

```powershell
python tools/query_rti_work.py section 8.18.1 --summary --compact --limit 8
python tools/query_rti_work.py requirement requirement-candidate-content-clauses-08-time-management-page-206-l149-37 --summary --compact --limit 8
```

The ownership/DDM transfer slice is a useful exact crosswalk example: its
same-federate reacquisition assertions cover the §9.6 non-restoration rule,
while the lane command runs only the three ownership-transfer cases:

```powershell
python tools/query_rti_work.py case umbra-cpp-ownership-transfer-update-region-integration --summary --compact
python tools/query_rti_work.py matrix requirement-candidate-content-clauses-09-data-distribution-management-page-230-l178-55 --group-by requirement --summary --compact
python tools/query_rti_work.py focus ownership-transfer-update-region --summary --compact
python tools/query_rti_work.py check --lane ownership-transfer-update-region --summary --compact
ctest --test-dir .build -C Debug -L "ownership-transfer-update-region" --output-on-failure
```

The multi-acquirer Release Denied case is indexed as its own focused lane, so
its callback-drain regression can be resumed without searching the ownership
catalog. Its case card exposes the four direct Lab-to-2025-subsection pairs,
the three canonical sections, 53 Catch2 assertions, and the exact CTest filter:

```powershell
python tools/query_rti_work.py case umbra-cpp-attribute-ownership-release-denied-multi-acquirer-integration --summary --compact
python tools/query_rti_work.py focus attribute-ownership-release-denied-multi-acquirer --summary --compact
python tools/query_rti_work.py check --lane attribute-ownership-release-denied-multi-acquirer --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded Attribute Ownership Release Denied reaches all 2025 regular acquirers$" --output-on-failure
```

`lab-issues` is the cheap reverse path for Requirements Lab complaints and
recurrences. It reads `compliance/requirements-lab/known-issues.json` only,
so issue lookup never triggers a Lab export resync or a C++ source scan. Use an
exact issue id, requirement id, or clause when reviewing one defect:

```powershell
python tools/query_rti_work.py lab-issues RL-177 --summary --compact
python tools/query_rti_work.py lab-issues 6.17.4 --json --summary
```

The same dashboard exposes `source_only_reconciliation` (or the text
`Source-only reconciliation head`) with the first unplanned C++ declaration,
its source location, and the explicit `ready --include-source-only` opt-in;
reconciliation work is therefore visible without a second source scan.

Use `check --lane <tag>` or `check --family <id>` for a scoped integrity gate.
Scoped checks validate only the selected roadmap owner and its live Catch2
rows; the no-selector form is the full live-plan diagnostic. The append-only
completion ledger is available separately with `check --historical` when a
strict historical audit is needed. Superseded ledger entries stay available
for audit but are omitted from the live result.
The live check also confirms that each selected Requirements-Lab id appears in
the row's derived canonical 2025 `document:clause` subsection list; mapping
drift is therefore reported at the query boundary instead of being rediscovered
by a broad Lab search.

When the indexed source and planned-row queues are exhausted, `dashboard` and
`ready` deliberately stop showing the completed active pointer as the next
task. They show a bounded family selector instead, with mapping counts. Pick
one family with the printed `ready --family <id>`, `next --family <id>`, or
`work <id>` handle; the family-scoped response then includes
requirement/canonical 2025-section previews and direct requirement-to-section
pairs, plus exact `matrix --group-by requirement` and
`matrix --group-by section` commands (and `gaps --family <id>` when a new case
is needed). Use `focus`/`matrix` only after that bounded choice. The text
dashboard prints the first family as a recommendation; JSON consumers can read
`next.recommended_family_id`, `next.selection_required`, and
`next.family_choice_command` without parsing prose.

The index can promote one deliberate `active_handoff` above that family
fallback. This is the preferred way to queue the next implementation slice
when it is a new C++ case rather than an already-planned row. `ready` returns
the proposed test title, target source file, lane, CTest filter, acceptance
criteria, and the exact mapped seed row. The seed's requirement IDs,
canonical 2025 subsections, API surfaces, and direct pairs are resolved from
the checked-in Catch2 plan; they are not copied from broad tags and they do
not require a Lab rescan. After implementation, add the new plan row and
replace/retire the handoff so the live plan becomes authoritative.
The handoff id, proposed lane, and exact test title are also accepted by
`roadmap <query>`; that reverse lookup returns the owning family and the same
bounded handoff metadata.

For a family whose `action_state` is `new-case-needed`, the row also carries a
bounded `gap_preview` (shown as `gap_head` in text): one uncovered pinned-2025
record in canonical corpus order, its `document:clause` subsection, and exact
`gap_requirement`/`gap_section` commands. It is a coverage pointer, not a
semantic priority decision. Use the emitted `gaps --family` handle for the
remaining inventory; this handoff is derived from the checked-in corpus and
does not rescan the unchanged Requirements Lab.

Queue rows expose an `action_state` so a completed evidence family cannot be
mistaken for runnable implementation work: `implementation`, `mapping`,
`source-reconciliation`, `external-review`, and `new-case-needed` are queued
actions; `evidence-complete` is a deliberate waiting state. The older `state`
field is retained as a coarse indexed status; use `action_state` to decide
what to do next. Diagnostic
`source_drift` also has an `actionable_source_drift` count; disabled or
reconciled historical rows do not block `ready`. JSON consumers also get
`action_counts`, `queued_action_family_count`, and
`evidence_complete_family_count` without counting queue rows themselves.

The source-only C++ queue is currently empty: all discovered declarations have
an explicit plan row. One retained historical plan row is source-unlocated but
non-actionable under the default policy (`actionable=0`). `ready` and `next`
therefore have no source-reconciliation head. `unplanned` remains the bounded
source-to-plan reconciliation view after a new C++ declaration is added:

```powershell
python tools/query_rti_work.py unplanned --summary --compact --limit 8
```

The live snapshot in `ROADMAP-INDEX.json` records these counts. Two retained
FOM/MIM MOM declarations are intentionally marked `disabled-source-artifact`
because they remain inside `#if 0`; query them by title to see their re-enable
action, but do not count them as executable evidence.

The live check and offline regression guard reject duplicate JSON keys and
repeated selectors in the roadmap index or Catch2 plan, so a malformed edit
cannot silently change which mapping the bounded queries return:

```powershell
python tools/query_rti_work_regression.py
```

Use `ready` as the active implementation handoff when it reports a concrete
planned row or source declaration. It names one exact test/source, its mapping
handles, and the focused execution/check commands. Use `work` for family
context, completed baselines, and package/JUnit/process handles; it may retain
an active historical baseline while `ready` advances through the queues. If
`ready` queues are exhausted, it prints bounded family choices; select one with
`ready --family <family-id>` or `work <family-id>`.

For a broad family without a single `next_lane`, `work <family-id>` also emits
bounded `lanes --family ...` and `ready --family ...` commands, keeping new-case
selection local to the indexed family.

An exact lane handle (normally a Catch2 lane tag; the plan's
`primary_lane`/`focus_lane` aliases are accepted too) can also be entered
directly in `roadmap`. The result includes a `lane_match` with its
representative source/test mapping, `trace`, `focus`, and CTest handles, so
lane ownership is resolved without a second family or plan search:

```powershell
python tools/query_rti_work.py roadmap joined-federate-mom-deleted-object-count-periodic --summary --compact
```

`status --summary --compact` also prints the latest completed slice, its source
location, and a copyable exact trace command before the open-family list.

Focused lane totals are consistent across `focus` and `lanes`: when the index
contains a verified JUnit aggregate, both commands report that total. A lane
listing also exposes the smaller `plan_row_assertions` sum when legacy rows do
not carry individual assertion counts, so the difference is visible and not a
hidden search trap.

The indexed snapshot is 1,285 Catch2 cases (1,221 mapped), including 155
process-boundary cases and 6,717 indexed assertions (6,869 assertions recorded
on the individual process-boundary plan rows). The live check leaves 64
intentional explicit-disposition rows and zero unclassified mappings.

The latest completed slice is the HLA_IMMEDIATE restored ownership-assumption
companion in `cpp/tests/attribute_ownership_acquisition_catch2.cpp`; its nine
requirements, seven canonical 2025 sections, and 11 official C++ API surfaces
remain separate from the completed restore-abort, successful/failed lifecycle,
restore-request-failure, restore-status, and already-pushed work-item slices:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --summary --compact
python tools/query_rti_work.py focus process-federation-restore-work-item-ownership-assumption-immediate --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver restored ownership-assumption work under HLA_IMMEDIATE through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --group-by requirement --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --group-by section --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-work-item-ownership-assumption-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors deliver restored ownership-assumption work under HLA_IMMEDIATE through a configured process endpoint$" --output-on-failure
```

The restore-status, restore-abort, already-pushed work-item, connection-loss,
and cancellation lanes remain separately indexed rather than being folded into
this restore family.

The preceding mixed cancel-delete-divest companion remains separately queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
```

The earlier final-federate directive-two companion remains separately queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-final-federate-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-final-federate-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-final-federate-immediate --summary --compact
```

The earlier bounded NoAction companion remains separately queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-no-action-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-no-action-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-no-action-immediate --summary --compact
```

The pending-acquisition cancellation companion remains separately
queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py trace umbra-cpp-connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py matrix connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.connection_loss_automatic_cancel_pending_acquisition\.catch2\.Immediate callbacks cancel a lost federate's pending ownership acquisition synchronously$" --output-on-failure
python tools/query_rti_work.py case umbra-cpp-process-endpoint-pushed-ownership-assumption-evoked-integration --summary --compact
python tools/query_rti_work.py focus process-pushed-ownership-assumption-evoked --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver an unconsumed pushed ownership-assumption callback through HLA_EVOKED" --summary --compact
python tools/query_rti_work.py matrix process-pushed-ownership-assumption-evoked --summary --compact
python tools/query_rti_work.py check --lane process-pushed-ownership-assumption-evoked --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors deliver an unconsumed pushed ownership-assumption callback through HLA_EVOKED$" --output-on-failure
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-push-integration --summary --compact
python tools/query_rti_work.py focus process-federation-restore-work-item-ownership-assumption-push --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve pushed ownership-assumption delivery across save and restore" --summary --compact
python tools/query_rti_work.py matrix process-federation-restore-work-item-ownership-assumption-push --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-work-item-ownership-assumption-push --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors preserve pushed ownership-assumption delivery across save and restore$" --output-on-failure
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-integration --summary --compact
python tools/query_rti_work.py case umbra-cpp-federation-save-commit-process-local-ownership-assumption-work-unit --summary --compact
```

The embedded ownership-assumption search is split into three exact cards so
the later-join, callback-return, and fresh-epoch boundaries can be run without
opening the entire federation-management test file:

```powershell
python tools/query_rti_work.py case umbra-cpp-resign-action-assumption-discovery-continuation-integration --summary --compact
python tools/query_rti_work.py focus resign-action-assumption-discovery-continuation --summary --compact
python tools/query_rti_work.py matrix resign-action-assumption-discovery-continuation --summary --compact
python tools/query_rti_work.py check --lane resign-action-assumption-discovery-continuation --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded assumption search continues after a later join and discovery$" --output-on-failure

python tools/query_rti_work.py case umbra-cpp-ownership-assumption-search-continuation-integration --summary --compact
python tools/query_rti_work.py focus ownership-assumption-search-continuation --summary --compact
python tools/query_rti_work.py matrix ownership-assumption-search-continuation --summary --compact
python tools/query_rti_work.py check --lane ownership-assumption-search-continuation --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded ownership assumption search advances after a declined callback$" --output-on-failure

python tools/query_rti_work.py case umbra-cpp-ownership-assumption-search-epoch-integration --summary --compact
python tools/query_rti_work.py focus ownership-assumption-search-epoch --summary --compact
python tools/query_rti_work.py matrix ownership-assumption-search-epoch --summary --compact
python tools/query_rti_work.py check --lane ownership-assumption-search-epoch --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded ownership assumption search starts a fresh epoch after transfer$" --output-on-failure
```

These are 26, 36, and 39 assertion HLA_EVOKED cases, mapped respectively to
§4.12/§4.12.4, §7/§7.2, and §7/§7.2. They remain development-profile
traceability evidence only.

The RTI-owned MOM query is separately addressable and deliberately does not
inherit the process endpoint's still-open MOM-establishment/report-file work:

```powershell
python tools/query_rti_work.py case umbra-cpp-rti-owned-mom-ownership-query-integration --summary --compact
python tools/query_rti_work.py focus rti-owned-mom-ownership-query --summary --compact
python tools/query_rti_work.py matrix rti-owned-mom-ownership-query --summary --compact
python tools/query_rti_work.py check --lane rti-owned-mom-ownership-query --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded RTI-owned MOM attributes participate in ownership queries$" --output-on-failure
```

An earlier bounded runtime correction is the timestamped process Delete Object
Instance baseline at `cpp/tests/ieee1516_2025_connection_catch2.cpp:9166`.
Its regulated and non-regulated variants preserve the process queue identity
while projecting a public `MessageRetractionHandle` only for the
time-regulating producer; the exact case carries 283 assertions, maps 14 Lab
requirements to eight canonical 2025 sections, and exercises seven official
C++ API surfaces, including queued constrained-recipient and preferred
receive-order variants under both callback models. Query it without scanning the full plan:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
```

An earlier completed slice is the process federation-save abort case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:961`. It carries 13 assertions
under `HLA_EVOKED`, maps three Lab requirements to two canonical 2025 sections,
and exercises five official C++ API surfaces. It carries Abort Federation Save
and the `Federation Not Saved(SAVE_ABORTED)` callback through the configured
process endpoint. Query it directly:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-save-abort-integration --summary --compact
python tools/query_rti_work.py focus process-federation-save-abort --summary --compact
python tools/query_rti_work.py trace umbra-cpp-process-endpoint-federation-save-abort-integration --summary --compact
python tools/query_rti_work.py matrix process-federation-save-abort --summary --compact
python tools/query_rti_work.py check --lane process-federation-save-abort --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassadors carry federation save abort through the configured process endpoint$" --output-on-failure
```

Timed save, not-complete failure, restore, push-mode, multi-federate save,
package/JUnit, review, validation, interoperability, and conformance remain
separate lanes. The preceding untimed and status process save baselines remain
separately queryable as `process-federation-save-untimed` and
`process-federation-save-status`. The earlier embedded
evoked synchronization save/restore case remains separately queryable as
`evoked-synchronization-save-restore`.

The preceding process-boundary delivery slice remains separately queryable as
`process-directed-interaction-transportation` and continues to carry 66
assertions under both callback models.

The timestamped process Delete Object Instance baseline is independently
queryable through `process-boundary` at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:9166`. It carries 283
assertions, 14 direct Lab requirements, eight canonical 2025 sections, and
seven official C++ API surfaces; the public return value is a valid
`MessageRetractionHandle` only for the regulated producer, while the
non-regulated variant verifies the required absent designator. Use the exact
case and trace handles below; broader retraction and regional/DDM remain
separate:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
```

The preceding two-recipient directed interaction lane remains separately
queryable as `process-directed-interaction-multi-recipient`; it carries 104
assertions and the broader Receive Directed Interaction callback fan-out
evidence. Keep that lane separate from transportation override behavior.

The preceding completed slice is the timestamped regional configured-process
transportation lane at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27745`. It carries 122
assertions, maps 27 Lab requirements to 15 canonical 2025 sections and 21
official C++ API surfaces, and proves that a publisher's
confirmed `HLAbestEffort` override survives a timestamped regional send with
TIMESTAMP classifications, retraction identity, callback-before-grant ordering,
and source-region metadata under both callback models. It remains private
foundation evidence, not a conformance claim. The preceding interaction
transportation lane at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27030` carries 40 assertions,
ten Lab requirements, four canonical sections, and 9 official API surfaces.
The preceding instance transportation lane at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:26822` carries 30 assertions,
five Lab requirements, four canonical sections, and 10 official API surfaces.
The preceding directed TSO callback-gating, multi-recipient fanout,
multiple-message FIFO, changed-lookahead interaction, and filesystem
restore-baseline lanes remain independently queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-instance-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-instance-control --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes instance transportation type change and query through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-instance-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-instance-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador routes instance transportation type change and query through a configured process endpoint$" --output-on-failure
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes interaction transportation type change and query through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-interaction-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-interaction-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador routes interaction transportation type change and query through a configured process endpoint$" --output-on-failure
```

The newest timestamped regional companion is independently addressable as
`process-transportation-timestamped-regional-interaction-control`:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-timestamped-regional-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-timestamped-regional-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve a timestamped regional interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-timestamped-regional-interaction-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-timestamped-regional-interaction-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassadors preserve a timestamped regional interaction transportation override through a configured process endpoint$" --output-on-failure
```

The preceding regional companion is independently addressable as
`process-transportation-regional-interaction-control` at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27247`. It carries 84
assertions, 23 direct Lab requirements, 11 canonical 2025 sections, and 17
official C++ API surfaces. It proves that a publisher's confirmed
`HLAbestEffort` interaction transportation override survives a regional send
and that the receiver retains the source-region metadata under both callback
models:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-regional-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-regional-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve a regional interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-regional-interaction-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-regional-interaction-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassadors preserve a regional interaction transportation override through a configured process endpoint$" --output-on-failure
```

The preceding configured-process multi-recipient fanout lane remains
independently queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-interaction-fanout-integration --summary --compact
python tools/query_rti_work.py focus process-tso-interaction-fanout --summary --compact
python tools/query_rti_work.py trace "RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-interaction-fanout-integration --summary --compact
python tools/query_rti_work.py check --lane process-tso-interaction-fanout --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant$" --output-on-failure
python tools/query_rti_work.py ready --family transport-and-conformance --summary --compact
python tools/query_rti_work.py focus process-tso-regional-interaction-callback-gating --summary --compact
python tools/query_rti_work.py trace "RTIambassadors retain a timestamped regional interaction while callbacks are disabled" --summary --compact
```

The regional TSO callback-gating companion is now complete at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:26210` with 69 assertions. It
maps 28 Requirements-Lab IDs to 16 canonical 2025 sections and 20 official
C++ API surfaces, and proves that a timestamped regional interaction and its
matching grant remain queued while callbacks are disabled, then arrive exactly
once in interaction-before-grant order after re-enable. It remains private
foundation evidence, not a conformance claim:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-regional-interaction-callback-gating --summary --compact
python tools/query_rti_work.py focus process-tso-regional-interaction-callback-gating --summary --compact
python tools/query_rti_work.py trace "RTIambassadors retain a timestamped regional interaction while callbacks are disabled" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-regional-interaction-callback-gating --summary --compact
python tools/query_rti_work.py check --lane process-tso-regional-interaction-callback-gating --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors retain a timestamped regional interaction while callbacks are disabled$" --output-on-failure
```

The indexed source/planned queues are now exhausted again; `ready` returns
bounded family choices for the next slice. Use the transport family card when
continuing, and keep regional/DDM, directed interactions, subscription
mutation, resignation, immediate callbacks, save/restore, package/JUnit,
protected review, interoperability, validation, and conformance as separate
lanes.

This slice carries 133 assertions across 20 Lab requirements, 13 canonical
2025 sections, and 15 official C++ API surfaces. The preceding configured-
process changed-lookahead interaction slice carries 67 assertions, the
timestamped-attribute slice carries 70 assertions, the retraction-lifetime
slice carries 45
assertions; regional two-receiver FIFO, fan-out,
suppression, single-receiver ordering, and queued-TSO GALT/LITS companions remain
independently selectable as well:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-queued-tso-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassador queries LITS from queued timestamped process input after regulator disable" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-queued-tso-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-queued-tso --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-queued-tso --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries LITS from queued timestamped process input after regulator disable$" --output-on-failure
```

This companion carries 31 assertions across five Lab requirements, four
canonical 2025 sections, and twelve official C++ API surfaces. The preceding
multi-federate available-bound companion remains independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-multi-federate-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors expose defined GALT and LITS across configured process federates" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-multi-federate-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-multi-federate --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-multi-federate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors expose defined GALT and LITS across configured process federates$" --output-on-failure
```

This companion carries 24 assertions across five Lab requirements, four
canonical 2025 sections, and four official C++ API surfaces. The preceding
immediate undefined-bound companion remains independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-immediate-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassador queries GALT and LITS through a configured process endpoint with immediate callbacks" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-immediate-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-immediate --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries GALT and LITS through a configured process endpoint with immediate callbacks$" --output-on-failure
```

This companion carries 14 assertions across five Lab requirements, four
canonical 2025 sections, and four official C++ API surfaces. The preceding
configured-process multiple-record Flush Queue lane remains independently
queryable:

The plan row declares `primary_lane`, so the exact `case` card resolves to the
dedicated multiple-record FQR lane even though the test also carries broad
overlapping tags such as `flush-queue-request` and `process-boundary`.

```powershell
python tools/query_rti_work.py case umbra-cpp-process-flush-queue-multiple-records-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver multiple queued timestamped process messages in order through Flush Queue" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-flush-queue-multiple-records-integration --summary --compact
python tools/query_rti_work.py focus process-flush-queue-multiple-records --summary --compact
python tools/query_rti_work.py focus process-boundary --summary --compact
python tools/query_rti_work.py check --lane process-flush-queue-multiple-records --summary --compact
python tools/query_rti_work.py focus process-flush-queue-retraction --summary --compact
python tools/query_rti_work.py check --lane process-flush-queue-retraction --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver multiple queued timestamped process messages in order through Flush Queue$" --output-on-failure
```

The slice carries 110 assertions across 22 2025 Lab requirements, 12 canonical
sections, and 22 official C++ API surfaces. It proves timestamp-5, timestamp-8,
and exact requested-frontier timestamp-10 TSO callbacks arrive FIFO before one
Flush Queue Grant; a timestamp-14 record is retracted before that frontier and
never appears; retained timestamp-12 delivery occurs on the later frontier; and
the reported actual/optimistic values remain separate from the requested flush
frontier. The preceding 57-assertion GALT-frontier and 16-assertion
single-endpoint FQR baselines remain independently queryable by plan id and
their dedicated lanes.

The preceding configured-process federate-identity lookup handoff remains
independently queryable as
`umbra-cpp-process-federate-identity-lookup-integration` (11 assertions,
active-name handle resolution, and retained name lookup after resignation).

The preceding configured-process Confirm Divestiture receive-order,
push-receive, and pushed-assumption lanes remain independently queryable. The
assumption companion is source-located and green; `ready --family
object-ddm-ownership` reports that family as evidence-complete. Keep these
handles as the bounded ownership handoff:

```powershell
python tools/query_rti_work.py ready --family object-ddm-ownership --summary --compact
python tools/query_rti_work.py case umbra-cpp-confirm-divestiture-process-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver Confirm Divestiture through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-confirm-divestiture --summary --compact
python tools/query_rti_work.py focus process-confirm-divestiture --summary --compact
python tools/query_rti_work.py check --lane process-confirm-divestiture --summary --compact
python tools/query_rti_work.py case umbra-cpp-confirm-divestiture-process-push-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver Confirm Divestiture through a configured process endpoint in push receive mode" --summary --compact
python tools/query_rti_work.py matrix process-confirm-divestiture-push --summary --compact
python tools/query_rti_work.py focus process-confirm-divestiture-push --summary --compact
python tools/query_rti_work.py check --lane process-confirm-divestiture-push --summary --compact
python tools/query_rti_work.py case umbra-cpp-confirm-divestiture-process-assumption-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors continue Confirm Divestiture with a pushed assumption candidate" --summary --compact
python tools/query_rti_work.py matrix process-confirm-divestiture-assumption --summary --compact
python tools/query_rti_work.py focus process-confirm-divestiture-assumption --summary --compact
python tools/query_rti_work.py check --lane process-confirm-divestiture-assumption --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors continue Confirm Divestiture with a pushed assumption candidate$" --output-on-failure
```

The completed push Confirm lane records 41 HLA_IMMEDIATE assertions, four
direct ownership requirements, four canonical 2025 subsections (`7.2`,
`7.3.4`, `7.6.3`, and `7.8`), and 15 official C++ API surfaces. The completed
assumption lane records 60 HLA_IMMEDIATE assertions, one direct `7.2`
requirement, and 16 official C++ API surfaces; its three-federate process case
preserves the negotiated tag through the pushed assumption callback and final
acquisition notification. The negotiated/cancellation case
remains available by its exact plan id and lane, as do the
acquisition-cancellation, owner-release, regular, If Available, Query
Attribute Ownership, and read-only `process-ownership-check` cases.

The preceding strict-versus-relaxed DDM boundary card remains available by its
exact plan id and lane; it is no longer the latest process handoff.

The `case` command accepts an exact plan id or full TEST_CASE title and emits
the source, owner, API surfaces, direct requirement-to-subsection pairs,
traceability state, and focused execution commands in one bounded card. The
state is `requirements-mapped`, `explicit-disposition`, or `unclassified`, so
an intentional no-standalone-surface decision is not mistaken for missing
traceability. This row is at
`cpp/tests/regional_auto_provide_response_catch2.cpp:148`: 240 assertions, 15
direct Lab requirements, ten mapped 2025 sections, and 18 official C++ API
surfaces. It proves the selected exact-touch relaxed boundary, strict filtering,
positive-gap suppression, and strict-overlap restoration under both callback
models. The current pinned export contains the immutable line-71 candidate;
RL-030's omission is historical to revision 4f012fb1.

The preceding time-axis-independence regional attribute lane remains available
with its exact plan id or `focus time-axis-independence` handle.

The object-name reservation lane is also a compact reverse-mapping example:
its 68-assertion C++ case now carries 12 direct requirement IDs across five
canonical 2025 sections, including the §6.7.5 multiple-release postcondition.
Use these bounded commands to go from the requirement, to the test, to the
focused CTest run without searching the full plan:

```powershell
python tools/query_rti_work.py requirement requirement-candidate-content-clauses-06-object-management-page-117-l36-6 --summary --compact
python tools/query_rti_work.py case umbra-cpp-object-instance-name-reservation-integration --summary --compact
python tools/query_rti_work.py focus object-instance-name-reservation --summary --compact
python tools/query_rti_work.py matrix requirement-candidate-content-clauses-06-object-management-page-117-l36-6 --summary --compact
python tools/query_rti_work.py check --lane object-instance-name-reservation --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.object_instance_name_reservation\.catch2\.Embedded 2025 object-instance name reservation commits and reports asynchronously$" --output-on-failure
```

The preceding no-common-dimension object-attribute lane remains directly
queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-regional-object-attribute-no-common-dimension-integration --summary --compact
python tools/query_rti_work.py focus no-common-dimension --summary --compact
python tools/query_rti_work.py trace "Embedded regional object attributes with no common dimensions never overlap" --summary --compact
python tools/query_rti_work.py matrix no-common-dimension --summary --compact
python tools/query_rti_work.py check --lane no-common-dimension --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.regional_object_attribute_routing\.catch2\.Embedded regional object attributes with no common dimensions never overlap$" --output-on-failure
```

The preceding subscription-dimension-validation sibling remains directly
queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-regional-interaction-subscription-dimension-validation-integration --summary --compact
python tools/query_rti_work.py focus subscription-dimension-validation --summary --compact
python tools/query_rti_work.py matrix subscription-dimension-validation --summary --compact
```

The related empty-set sibling remains directly queryable with the
`subscription-empty-set` focus handle.

The preceding whole-class default-region removal sibling is separately
queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-whole-class-interaction-unsubscribe-default-region-integration --summary --compact
python tools/query_rti_work.py focus whole-class-unsubscribe --summary --compact
python tools/query_rti_work.py matrix whole-class-unsubscribe --summary --compact
```

The preceding mixed-dimensional validation sibling remains directly queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-mixed-region-interaction-validation-integration --summary --compact
python tools/query_rti_work.py focus mixed-region-interaction-validation --summary --compact
python tools/query_rti_work.py matrix mixed-region-interaction-validation --summary --compact
```

The preceding multi-region union sibling remains directly queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-multi-region-interaction-routing-integration --summary --compact
python tools/query_rti_work.py focus multi-region-regional-interaction --summary --compact
python tools/query_rti_work.py matrix multi-region-regional-interaction --summary --compact
```

It has 107 HLA_EVOKED assertions, four direct Lab requirements, one mapped
2025 section, and nine official C++ API surfaces. The positive-dimensional/
default-region sibling remains separately queryable with `focus
focused-positive-dimensional-regional-interaction`; select the next
implementation row from `object-ddm-ownership` with `work
object-ddm-ownership` or `ready --family object-ddm-ownership`; do not reopen
the full Lab or plan.

The directed-interaction regression lane is a cross-target CTest slice. Its
focus card is the mapping summary (42 indexed cases, 40 mapped and two
explicit dispositions); the rendered label command runs the 40 tests that
CTest currently assigns to the lane:

```powershell
python tools/query_rti_work.py focus directed --summary --compact
python tools/query_rti_work.py matrix directed --summary --compact
python tools/query_rti_work.py check --lane directed --summary --compact
ctest --test-dir .build -C Debug -L "^directed$" --output-on-failure --output-junit directed-lane-final.xml
```

Keep the indexed mapping counts and the CTest execution count distinct: the
two explicit dispositions are intentionally queryable but do not add runnable
tests to the label slice.

The foundational declaration baseline is the focused whole-object-class
declaration case. It is directly mapped to eight Requirements-Lab anchors,
three canonical 2025 clauses, and six official C++ API surfaces, with 34
`HLA_EVOKED` assertions. Use these handles without opening the aggregate test;
the current implementation handoff is the transportation-control slice listed
later in this card, while this declaration baseline remains directly queryable:

```powershell
python tools/query_rti_work.py focus whole-object-class-declaration --summary --compact
python tools/query_rti_work.py trace "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries" --summary --compact
python tools/query_rti_work.py matrix "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries" --summary --compact
python tools/query_rti_work.py check --lane whole-object-class-declaration --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.fom_declaration_management\.catch2\.Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries$" --output-on-failure
```

`ready` keeps disabled source artifacts in the diagnostic counts but skips them
as executable handoffs, so a malformed or `#if 0` historical declaration cannot
stall the next bounded implementation slice.

The preceding extracted slice is the federation-scoped `HLAcurrentFDD` MOM case.
It is mapped to the pinned clause-4 content-access candidate and can be
queried and run without the aggregate translation unit:

```powershell
python tools/query_rti_work.py focus federation-mom-current-fdd --summary --compact
python tools/query_rti_work.py trace "Embedded federation MOM exposes and refreshes HLAcurrentFDD" --summary --compact
python tools/query_rti_work.py matrix "Embedded federation MOM exposes and refreshes HLAcurrentFDD" --summary --compact
python tools/query_rti_work.py check --lane federation-mom-current-fdd --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.federation_mom_current_fdd\.catch2\.Embedded federation MOM exposes and refreshes HLAcurrentFDD$" --output-on-failure
```

The case records 60 HLA_EVOKED assertions, verifies the composed base FDD,
observes a reliable conditional refresh after an additional FOM Join, and
checks that a direct request returns the refreshed official HLAunicodeString.
The prior ordinary custom-transportation lanes remain queryable as
`custom-transportation-delivery`,
`custom-transportation-ordinary-regional-interaction`, and
`custom-transportation-ordinary-regional-attribute`; keep those explicit
development-profile/API dispositions separate from this clause-mapped MOM
slice. `ready --summary --compact` advances to the next indexed row after
this case is recorded.

The federation-scoped FOM-module/MIM MOM reports now have their own focused
native lane, so this behavior does not require reopening the aggregate
federation-management test:

```powershell
python tools/query_rti_work.py focus federation-mom-content-reports --summary --compact
python tools/query_rti_work.py trace "Embedded federation MOM content reports FOM module and MIM data through a focused lane" --summary --compact
python tools/query_rti_work.py matrix federation-mom-content-reports --summary --compact
python tools/query_rti_work.py check --lane federation-mom-content-reports --summary --compact
ctest --test-dir .build -C Debug -L "^federation-mom-content-reports$" --output-on-failure
```

The case carries 55 HLA_EVOKED assertions and directly maps the clause-4
content-access requirement to `hla-1516.1-2025:clause-4`; it covers both FOM
module and MIM request/report interactions, callback-time subscription
gating, and strict invalid-request handling.

The preceding extracted slice is the custom transportation-handle stability case.
It is independently runnable and keeps FOM composition/handle identity
separate from delivery and transportation-control lanes:

```powershell
python tools/query_rti_work.py focus transportation-handle-stability --summary --compact
python tools/query_rti_work.py trace "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
python tools/query_rti_work.py matrix "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
python tools/query_rti_work.py check --lane transportation-handle-stability --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.custom_transportation_handle_stability\.catch2\.Embedded custom transportation handles remain stable across an additional FOM join$" --output-on-failure
```

The case records 21 HLA_EVOKED assertions, 12 Requirements-Lab anchors, 11
canonical 2025 sections, and eight official C++ API surfaces. It proves that
adding an earlier-sorting transportation in a later FOM Join does not renumber
an existing handle and that both federates resolve the new shared handle/name.
Run `ready --summary --compact` for the next indexed handoff; source-unlocated
rows are source-reconciliation work, not executable evidence.

The latest extracted slice is the unnamed object-instance registration/discovery
case. It is independently runnable and keeps publication, discovery promotion,
and known-instance lookup separate from deletion, named registration, and DDM:

```powershell
python tools/query_rti_work.py focus object-instance-registration-discovery --summary --compact
python tools/query_rti_work.py trace "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle" --summary --compact
python tools/query_rti_work.py matrix "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle" --summary --compact
python tools/query_rti_work.py check --lane object-instance-registration-discovery --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.object_instance_registration_discovery\.catch2\.Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle$" --output-on-failure
```

The case records 81 assertions under both callback models and maps eight
Requirements-Lab anchors to four canonical 2025 sections. Run `ready
--summary --compact` for the next indexed handoff.

To find a roadmap family by title, tag, next-action text, Requirements-Lab id,
canonical 2025 subsection, or official C++ API surface without opening the
roadmap prose, use the bounded family index:

```powershell
python tools/query_rti_work.py roadmap --summary --compact
python tools/query_rti_work.py roadmap process --summary --compact --limit 8
python tools/query_rti_work.py roadmap hla-1516.1-2025:clause-6.1.13 --summary --compact
python tools/query_rti_work.py roadmap api.2025.cpp.rtiambassador.updateattributevalues --summary --compact
```

Each row carries live Catch2 case/assertion/mapping counts, the next-test
requirement and canonical 2025 subsection previews, an exact `next_source`
handoff when one is queued, and copyable `work`, `focus`, and family `matrix`
commands. Use `--status all` only when completed families are needed. Focused
lanes may use the index's compact
`focused_lane_tags` aliases, so a new exact lane can be added without editing
the large family tag list.

For one exact test-to-standard join, use `trace`; for reverse lookup by a
requirement, canonical `document:clause` subsection, API surface, or lane, use
`matrix` (both print direct Requirements-Lab-to-subsection pairs):

```powershell
python tools/query_rti_work.py trace "<exact Catch2 test title>" --summary --compact
python tools/query_rti_work.py matrix "<requirement-id-or-document:clause>" --summary --compact
```

Before trusting a contract selector after a source split, run the bounded
local drift guard. It checks exact native C++ path/title (and legacy line)
references without reopening or resynchronizing the Requirements Lab; external
portable TCK symbols are reported separately:

```powershell
python tools/query_rti_work.py contract-drift --summary --compact
python tools/query_rti_work.py contract-drift timestamped-directed-interaction --summary --compact
```

Do not infer new work from a full-suite scan. A `complete` lane with zero
executable candidates is a completed indexed slice; choose another family or
add one deliberately mapped C++ case.

When `ready` reports exhausted source/planned queues, it prints bounded open
family options; inspect only the bounded explicit/unclassified disposition queue
before selecting another family:

```powershell
python tools/query_rti_work.py unmapped --disposition unclassified --summary --compact --limit 12
```

Use `ready --family <family-id>` to turn one selected unclassified row into an
exact source/trace handoff; do not reopen the full Requirements Lab export.

When a family is complete and a deliberately new C++ case is needed, use the
pinned-corpus gap card to choose an exact normative anchor without searching
the Lab:

```powershell
python tools/query_rti_work.py gaps --summary --compact --limit 8
python tools/query_rti_work.py gaps hla-1516.1-2025:clause-7.2 --summary --compact --limit 8
python tools/query_rti_work.py gaps --family object-ddm-ownership --clause clause-9.1.3.3 --summary --compact --limit 8
python tools/query_rti_work.py requirement <lab-requirement-id> --summary --compact
```

`gaps` reports mapped/unmapped totals plus the most uncovered canonical
document:clause subsections. Its summary form omits full requirement
statements; query one sample id when the source text is needed. If that id has
no mapped Catch2 case, `requirement` returns the bounded gap record (subsection,
statement, and source) instead of a dead-end no-tests message.
Add `--family <roadmap-family-id>` to compute coverage only from that family’s
tagged C++ rows; this gives a deterministic family-local candidate list when
the global source queue is exhausted.

## Choose and run one lane

```powershell
python tools/query_rti_work.py lanes --family <family-id> --summary --compact --limit 12
python tools/query_rti_work.py lanes --family <family-id> --focused --summary --compact --limit 12
python tools/query_rti_work.py focus <exact-catch2-lane> --summary --compact
python tools/query_rti_work.py check --lane <exact-catch2-lane> --summary --compact
```

For a quick interactive lookup, `lanes <family-id>` is equivalent to
`lanes --family <family-id>`; the two forms may not be combined. Keep the
family scope explicit when scripting.

`lanes --family` is bounded discovery over already indexed rows. It prints
mapping state, source/candidate counts, assertions, requirement/2025-section
counts, and copyable `focus`/CTest handles. Add
`--disposition unclassified` to keep only lanes with a missing mapping
decision, or `--disposition explicit` to review intentional
no-standalone-surface dispositions; plain `--unmapped` keeps both visible.
The default inventory includes every broad cross-cutting tag. Use `--focused`
to restrict the result to the owning family's explicit `focused_lane_tags`
aliases; this is the preferred small selector for normal slice work.
In the plain inventory, `next` is reserved for an actionable source-located
candidate or mapping/source-drift handoff. If a lane contains only explicit
no-standalone-surface rows, it reports `review_only` plus `review_trace`
instead; use `--disposition explicit` when that review is intentional.
`focus` is the lane decision card:

- `complete`: no executable candidate, unclassified mapping, source drift, or
  indexed execution gate remains;
- `needs-mapping`: a source case still lacks a Requirements-Lab mapping
  decision;
- `source-drift`: a plan row has no current C++ declaration;
- `execution-blocked`: the mapping is queryable but a build/source-integrity
  gate prevents a runtime claim.

Explicit no-standalone-surface dispositions remain visible and do not count as
normative requirement coverage. Use the exact CTest selector printed by
`focus`; do not run the aggregate target when the card identifies a dedicated
target.

The process-boundary evidence is intentionally split into independently
buildable targets:

```powershell
python tools/query_rti_work.py focus process-boundary --summary --compact
ctest --test-dir <build-dir> -C Debug -L process-boundary --output-on-failure
```

Its JUnit artifact is
`compliance/process-boundary/process-boundary.xml`, merged from the five
independently buildable process-boundary targets by
`tools/merge_junit_reports.py`. The live query card currently reports 155 mapped
process-boundary cases and 6,717 indexed Catch2 assertions (6,869 assertions
recorded by plan rows); the stable CTest label selects
209 registrations. The JUnit target executes the 119 independently buildable
registrations and its merged aggregate contains 214 emitted section-level
testcases and 5,458 assertions, with zero failures/errors; the
aggregate umbrella registration remains CTest-only because its federation-
management translation unit is not an evidence source. These remain distinct
traceability, assertion, execution, and report metrics.

The installed-profile child build is a separate bounded check. Verify its
indexed catalog first, then emit the deterministic JUnit report from the
thirteen independently addressable package-process lanes:

```powershell
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug --output-on-failure --output-junit <build-dir>/package-smoke-consumer/Testing/package-process.xml
python tools/verify_process_package_lanes.py --ctest ctest --test-dir <build-dir>/package-smoke-consumer --config Debug --junit <build-dir>/package-smoke-consumer/Testing/package-process.xml
```

The verifier joins the report back to the thirteen `next_process_package_*`
handles in `ROADMAP-INDEX.json`; a missing/duplicated process name or any
JUnit failure/error fails the handoff.

The focused DELETE_OBJECTS connection-loss lane is selected without a catalog
search:

```powershell
python tools/query_rti_work.py lane package-process-connection-loss-delete-objects --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-connection-loss-delete-objects --output-on-failure
```
The indexed handle points back to
`umbra-cpp-connection-lost-automatic-delete-integration` for its five
Requirements-Lab ids and five canonical 2025 section keys; the package lane
is installed-process foundation evidence, not a conformance promotion.

The focused federation save/restore process lane is selected without a catalog
search:

```powershell
python tools/query_rti_work.py lane package-process-federation-save-restore --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-federation-save-restore --output-on-failure
```
Its indexed handle points to the native process restore-success case and its six
Requirements-Lab requirement/section mappings; it remains installed-process
foundation evidence.

The focused federation restore-failure process lane is selected without a
catalog search:

```powershell
python tools/query_rti_work.py lane package-process-federation-save-restore-failure --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-federation-save-restore-failure --output-on-failure
```
It is mapped to the native restore-failure case and verifies the official
`FEDERATE_REPORTED_FAILURE_DURING_RESTORE` reason.

The focused federation restore-abort process lane is selected without a catalog
search:

```powershell
python tools/query_rti_work.py lane package-process-federation-save-restore-abort --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-federation-save-restore-abort --output-on-failure
```
It is mapped to the native restore-abort case and verifies `RESTORE_ABORTED`.

The focused federation restore-status process lane is selected without a
catalog search:

```powershell
python tools/query_rti_work.py lane package-process-federation-save-restore-status --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-federation-save-restore-status --output-on-failure
```
It is mapped to the native restore-status case and verifies the in-progress
`FEDERATE_RESTORING` status response before restore completion.

The federation-listing Support Services boundary is also independently
runnable:

```powershell
python tools/query_rti_work.py focus federation-listing --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.federation_listing\.catch2\." --output-on-failure
ctest --test-dir <build-dir> -C Debug -L "^federation-listing$" --output-on-failure
python tools/query_rti_work.py trace "Embedded federation-list services dispatch standards reports in both callback models" --summary --compact
python tools/query_rti_work.py matrix "Embedded federation-list services dispatch standards reports in both callback models" --summary --compact
```

The single focused case records 41 assertions under both HLA_EVOKED and
HLA_IMMEDIATE. Its two plan rows map List/Report Federation Executions to
clause 4.8.4 and List/Report Federation Execution Members (including the
missing-federation callback) to clause 4.10.3; the shared source case is
counted once by the indexed lane.

The synchronization-point service has its own fast public lane, independent of
the aggregate federation-management executable:

```powershell
python tools/query_rti_work.py focus synchronization --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.synchronization_point\.catch2\." --output-on-failure
```

Use `trace` on either synchronization test title to see the exact 2025
requirement and subsection anchors.

The filesystem Register Federation Synchronization Point argument forms are
also independently queryable; this is the focused service-report lane for the
two public C++ overloads:

```powershell
python tools/query_rti_work.py trace "Embedded service reporting preserves Register Federation Synchronization Point arguments" --summary --compact
python tools/query_rti_work.py focus register-federation-synchronization-point-service-report --summary --compact
python tools/query_rti_work.py check --lane register-federation-synchronization-point-service-report --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.service_report_file_confirm_synchronization_point_registration\.catch2\.Embedded service reporting preserves Register Federation Synchronization Point arguments$" --output-on-failure
```

The 18-assertion case maps the two-argument omitted-set form to Table 5 Null
and the explicit `FederateHandleSet` form to Table 5 type 18 without changing
the joined-federate report file.

The filesystem service-report join lifecycle is also an isolated public case;
it runs from the small service-report-store target rather than the aggregate
federation-management executable:

```powershell
python tools/query_rti_work.py focus join-lifecycle --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.service_report_store\.catch2\.Embedded service-report files are allocated at join and remain stable across switch cycles and rejoin$" --output-on-failure
```

Its plan row carries the exact source pointer, assertion count, and the
`11.5`/`11.5.2` requirement-to-subsection pairs. Use `trace` on the title to
inspect that mapping without opening the aggregate test file.

The adjacent writer-failure lane keeps join-time creation failure and
post-join append failure together, while retaining separate plan rows and
requirement mappings:

```powershell
python tools/query_rti_work.py focus service-report-writer-failure --summary --compact
python tools/query_rti_work.py trace "Service-report append failure surfaces an RTI error without an in-memory fallback" --summary --compact
python tools/query_rti_work.py check --lane service-report-writer-failure --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.service_report_writer_failure\.catch2\." --output-on-failure
```

The focused target currently carries 27 native C++ assertions across the two
failure cases; production filesystem permission/full-disk and cross-process
failure matrices remain explicitly separate follow-up evidence.

The current bounded implementation slice is the focused attribute-transportation
control case at `cpp/tests/custom_transportation_type_control_catch2.cpp:102`.
It carries 43 native HLA_EVOKED assertions mapped to eight Requirements-Lab
anchors and canonical 2025 clauses 6.26, 6.27, 6.28, and 6.29.1. It covers
declared custom transportation handles, class defaults, per-instance change,
confirmation/report callbacks, query state, and sender-only isolation. The
historical aggregate transportation row has now been rebuilt and the bounded
transportation lane is green; it remains separate from this focused mapping
card:

```powershell
python tools/query_rti_work.py trace "Embedded focused attribute transportation control preserves declared FOM defaults" --summary --compact
python tools/query_rti_work.py matrix custom-transportation-type-control --summary --compact
python tools/query_rti_work.py focus custom-transportation-type-control --summary --compact
python tools/query_rti_work.py check --lane custom-transportation-type-control --summary --compact
cmake --build <build-dir> --config Debug --target umbra_custom_transportation_type_control_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.custom_transportation_type_control\.catch2\.Embedded focused attribute transportation control preserves declared FOM defaults$" --output-on-failure
```

Keep this lane separate from ordinary delivery, regional DDM, timestamped
delivery, save/restore, package, validation, interoperability, and conformance
evidence. `ready --summary --compact` advances from the indexed lane rather
than reopening the Requirements Lab.

The joined-federate `HLAsetSwitches` control path is now its own bounded lane
at `cpp/tests/mom_federate_set_switches_catch2.cpp:67`. It carries 45 native
HLA_EVOKED assertions mapped to five Requirements-Lab candidates and canonical
2025 clauses 11.4.1, 11.4.2, and 11.5. The case covers sender-only state,
all nine getter surfaces, promoted predefined subclasses, ignored extension
parameters, malformed resign values, empty parameter maps, and the
Service-Reporting/subscription interlock:

```powershell
python tools/query_rti_work.py trace "Embedded MOM HLAsetSwitches updates the sending federate's switch subset" --summary --compact
python tools/query_rti_work.py matrix mom-federate-set-switches --summary --compact
python tools/query_rti_work.py focus mom-federate-set-switches --summary --compact
python tools/query_rti_work.py check --lane mom-federate-set-switches --summary --compact
cmake --build <build-dir> --config Debug --target umbra_mom_federate_set_switches_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.mom_federate_set_switches\.catch2\.Embedded MOM HLAsetSwitches updates the sending federate's switch subset$" --output-on-failure
```

This is development-profile evidence for the joined-federate control path;
full MOM failure records, federation-wide parameter matrices, distributed
delivery, package/JUnit/protected-review evidence, validation, interoperability,
and conformance remain separate.

The public joined-federate MOM projection has its own fast lane. It exercises
the official discovery/reflection and requested-value surfaces for the static
`HLAreportServiceFile` attribute:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-report-file --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_report_file\.catch2\.Embedded HLAreportServiceFile is published as a stable joined-federate MOM attribute$" --output-on-failure
```

Use `trace` on the title to see the two direct Requirements Lab pairs and the
source/assertion metadata without reopening the aggregate MOM test.

The object-class handle/name lookup now has its own fast lane and maps directly
to 2025 clauses 10.4.6 and 10.6.2:

```powershell
python tools/query_rti_work.py focus object-class-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.object_class_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded object-class lookup services use stable handles from the joined federation FOM" --summary --compact
```

The focused executable reports the official pre-join error paths, stable
handle/name round-trips, and compatible additional-FOM extension without
depending on the aggregate federation-management target.

The matching interaction-class lookup lane uses the same handles:

```powershell
python tools/query_rti_work.py focus interaction-class-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.interaction_class_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded interaction-class lookup services use stable handles from the joined federation FOM" --summary --compact
```

Its direct 2025 subsection pairs are 10.13.2 and 10.14.5.

The inherited attribute handle/name boundary is likewise isolated:

```powershell
python tools/query_rti_work.py focus attribute-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.attribute_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded attribute lookup resolves inherited definitions in the joined federation FOM" --summary --compact
```

Its direct 2025 subsection pairs are 10.9.1 and 10.10.3.

The inherited parameter handle/name boundary follows the same compact path:

```powershell
python tools/query_rti_work.py focus parameter-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.parameter_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded parameter lookup resolves inherited definitions in the joined federation FOM" --summary --compact
```

Its direct 2025 subsection is 10.16.1 (the Lab exports both handle/name
statements under that clause).

The dimension metadata and lookup boundary is now independently runnable and
maps to the four local 2025 Lab candidates under clause 9.1.2:

```powershell
python tools/query_rti_work.py focus dimension-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.dimension_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded 2025 dimension lookup follows FOM hierarchy and upper bounds" --summary --compact
```

This lane covers FOM upper bounds, inherited object/interaction available
dimensions, stable handles and names, and the compatible extension join. It
does not stand in for region lifecycle or DDM routing.

The mandatory transportation lookup pair has the same bounded handles:

```powershell
python tools/query_rti_work.py focus transportation-type-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.transportation_type_lookup\.catch2\." --output-on-failure
ctest --test-dir <build-dir> -C Debug -L "^transportation-type-lookup$" --output-on-failure
python tools/query_rti_work.py trace "Embedded transportation type lookup exposes the mandatory 2025 support pair" --summary --compact
python tools/query_rti_work.py trace "Embedded transportation type lookup resolves a declared FOM transportation per execution" --summary --compact
python tools/query_rti_work.py matrix "Embedded transportation type lookup resolves a declared FOM transportation per execution" --summary --compact
```

Both the mandatory standard pair and the declared-FOM lookup case map the
official Get Transportation Type Handle and Get Transportation Type Name APIs
to clauses 10.19 and 10.20.4. Custom transportation composition/delivery
remains a separate lane. The CTest label runs the focused mandatory executable
plus both traceability contracts; use the declared-case `trace`/`matrix` handles
when you need that aggregate source scenario without reopening the full plan.

The time-role control lane is queryable without reopening the temporal backlog:

```powershell
python tools/query_rti_work.py focus time-role --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded time-role services keep enable requests callback-gated before TSO support" --summary --compact
python tools/query_rti_work.py matrix "Embedded time-role services keep enable requests callback-gated before TSO support" --summary --compact
ctest --test-dir <build-dir> -C Debug -L "^time-role$" --output-on-failure
```

The mapped aggregate covers the official Enable/Disable Time Regulation,
Enable/Disable Time Constrained, Query Lookahead, and completion-callback
surfaces. It is pinned to eight Lab candidates across clauses 8, 8.2, 8.3.1,
8.5.5, 8.6.3, 8.7.5, and 8.21.5, with 51 assertions in both callback models.
The label includes the time-role traceability contracts; timestamped delivery,
Modify Lookahead, save/restore, and conformance remain separate. If aggregate
Catch2 discovery is unavailable while the known federation-management source
corruption is being repaired, use the exact `trace` handle above and run the
native title directly from the built executable.

The adjacent Modify Lookahead slice has the same bounded handles:

```powershell
python tools/query_rti_work.py focus modify-lookahead --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded Modify Lookahead applies increases immediately and decreases gradually" --summary --compact
python tools/query_rti_work.py matrix "Embedded Modify Lookahead applies increases immediately and decreases gradually" --summary --compact
ctest --test-dir <build-dir> -C Debug -L "^modify-lookahead$" --output-on-failure
```

It maps the official Modify Lookahead service to seven Lab candidates in clause
8.20.4 and records 28 `HLA_EVOKED` assertions covering immediate increases,
grant-boundary decreases, Query Lookahead, and lifecycle fences. Future-input
coordination, save/restore, timestamped delivery, and conformance remain
separate; the repaired aggregate target is directly runnable through the exact
title or label.

The next Flush Queue Request slice is directly queryable as well:

```powershell
python tools/query_rti_work.py focus flush-queue-request --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded Flush Queue Request flushes queued TSO and reports optimistic time" --summary --compact
python tools/query_rti_work.py matrix "Embedded Flush Queue Request flushes queued TSO and reports optimistic time" --summary --compact
ctest --test-dir <build-dir> -C Debug -L "^flush-queue-request$" --output-on-failure
```

It maps the official Flush Queue Request/Grant and Receive Interaction path to
five Lab candidates across clauses 8.12 and 8.12.3, with 42 `HLA_EVOKED`
assertions. The case proves queued delivery before FQG, minimum and optimistic
grant values, callback ordering, and logical-time fences; regional,
future-input, save/restore, transport, and conformance evidence remain
separate. `focus` reports the direct aggregate CTest command.

The bounded no-TSO GALT scheduler slice is independently queryable:

```powershell
python tools/query_rti_work.py focus no-tso-galt-scheduler --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded constrained TAR waits for GALT and is released by a regulator advance" --summary --compact
python tools/query_rti_work.py matrix "Embedded constrained TAR waits for GALT and is released by a regulator advance" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded constrained TAR waits for GALT and is released by a regulator advance$" --output-on-failure
```

It maps the official Time Advance Request, Enable Time Regulation, Enable Time
Constrained, and Time Advance Grant surfaces to two clause-8 Lab candidates,
with 30 `HLA_EVOKED` assertions. The case holds a constrained TAR at the
regulator's GALT and releases it only after the regulator advances; NRG/
undefined-GALT transitions, timestamped ordering, other advance modes,
save/restore, transport, and conformance remain separate. The exact filter is
the focused native handle, and aggregate discovery is now available from the
repaired federation-management target.

The read-only Query GALT/Query LITS bounds slice is separate from scheduler
release:

```powershell
python tools/query_rti_work.py focus query-galt-lits --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded Query GALT and Query LITS observe other regulator time and pending advances" --summary --compact
python tools/query_rti_work.py matrix "Embedded Query GALT and Query LITS observe other regulator time and pending advances" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded Query GALT and Query LITS observe other regulator time and pending advances$" --output-on-failure
```

The case maps the official Query GALT and Query LITS APIs to five exact Lab
candidates across clauses 8, 8.1.5, 8.18.1, and 8.19.3, with 35
`HLA_EVOKED` assertions. It covers undefined bounds before an active
regulator, current lookahead, pending-advance bounds, matching GALT/LITS
values, a mismatched logical-time fence, and undefined bounds after disable.
This is read-only no-TSO evidence; scheduler release, timestamped delivery,
other advance modes, persistence, transport, and conformance are separate.

The configured process endpoint now has its own read-only temporal-bounds
baseline:

```powershell
python tools/query_rti_work.py focus process-query-time-bounds --summary --compact --limit 8
python tools/query_rti_work.py trace "RTIambassador queries GALT and LITS through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador queries GALT and LITS through a configured process endpoint" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries GALT and LITS through a configured process endpoint$" --output-on-failure
```

This 12-assertion `HLA_EVOKED` case maps the official Query GALT and Query
LITS surfaces through the private process seam to the same five 2025 Lab
anchors as the embedded no-TSO case. It currently proves the undefined-bound
baseline; multi-federate available bounds, queued TSO inputs, grants,
save/restore, package/JUnit, validation, and conformance stay separate.

The process time-role callback baseline is independently queryable:

```powershell
python tools/query_rti_work.py focus process-time-role --summary --compact --limit 8
python tools/query_rti_work.py trace "RTIambassador enables time constrained through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador enables time constrained through a configured process endpoint" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador enables time constrained through a configured process endpoint$" --output-on-failure
```

This 11-assertion `HLA_EVOKED` case maps the official Enable Time Constrained
request and Time Constrained Enabled callback to two existing clause-8
time-role Lab anchors. It proves the endpoint-owned role transition and
callback gating; process grants, multi-federate bounds, timestamped delivery,
save/restore, package/JUnit, validation, and conformance remain separate.

The process Time Advance Request/grant baseline is independently queryable:

```powershell
python tools/query_rti_work.py focus process-time-advance --summary --compact
python tools/query_rti_work.py trace "RTIambassador requests time advance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador requests time advance through a configured process endpoint" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests time advance through a configured process endpoint$" --output-on-failure
```

This 14-assertion `HLA_EVOKED` case maps the official Time Advance Request,
Time Advance Grant, and Query Logical Time surfaces to four exact 2025 Lab
anchors. It proves the endpoint-owned request/grant transition and callback
queueing; distributed GALT/LITS/TSO scheduling, save/restore, package/JUnit,
validation, and conformance remain separate.

The process Query Lookahead readback is a separate focused lane:

```powershell
python tools/query_rti_work.py focus process-query-lookahead-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassador queries lookahead through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador queries lookahead through a configured process endpoint" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries lookahead through a configured process endpoint$" --output-on-failure
```

This 13-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:11962` maps Query Lookahead and
callback-gated Enable Time Regulation to two exact 2025 Lab anchors. It proves
the not-enabled exception mapping and official interval encoding/readback over
the process seam; Modify Lookahead, multi-federate bounds, TSO scheduling,
save/restore, package/JUnit, validation, and conformance remain separate.

The process Modify Lookahead mutation is a separate focused lane:

```powershell
python tools/query_rti_work.py focus process-modify-lookahead-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassador modifies lookahead through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador modifies lookahead through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-modify-lookahead-focused --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador modifies lookahead through a configured process endpoint$" --output-on-failure
```

This 17-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12058` maps three exact Lab
anchors to clause 8.20.4. It proves the not-enabled fence, callback-gated
enablement, official interval encoding, immediate increase, and retention of a
lower request before a time advance. Grant-time decrease application,
multi-federate scheduling, TSO/retraction, save/restore, package/JUnit,
validation, and conformance remain separate lanes.

The grant-boundary companion is a separate, two-federate process lane:

```powershell
python tools/query_rti_work.py focus process-modify-lookahead-grant-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassadors apply a deferred lower lookahead at a configured process grant" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors apply a deferred lower lookahead at a configured process grant" --summary --compact
python tools/query_rti_work.py check --lane process-modify-lookahead-grant-focused --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors apply a deferred lower lookahead at a configured process grant$" --output-on-failure
```

This 34-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12164` maps six exact Lab
anchors across clauses 8.5.5, 8.6.3, 8.8.3, and 8.20.4. It keeps a lower
lookahead deferred before a pending grant, coordinates a constrained TAR with
the regulating federate, and verifies application of the lower interval at
the grant boundary. GALT/LITS/TSO ordering, role-disable, resignation,
save/restore, package/JUnit, validation, and conformance remain separate.

The Available-form process request is independently selectable:

```powershell
python tools/query_rti_work.py focus process-time-advance-available-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassador requests available time advance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador requests available time advance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-time-advance-available-focused --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests available time advance through a configured process endpoint$" --output-on-failure
```

This 15-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12396` maps four exact Lab
anchors across clauses 8.9, 8.14.3, and 8.18.1. It proves the dedicated
process TARA route, official logical-time encoding, callback-gated grant, and
Query Logical Time readback. Inclusive GALT, queued TSO, FQR,
multi-federate coordination, save/restore, package/JUnit, validation, and
conformance remain separate lanes.

The matching process NMR/NMRA routes are independently selectable:

```powershell
python tools/query_rti_work.py focus process-time-advance-next-message-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassador requests next message advances through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador requests next message advances through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-time-advance-next-message-focused --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests next message advances through a configured process endpoint$" --output-on-failure
```

This 22-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12492` maps four exact Lab
anchors across clauses 8.10.2, 8.11, and 8.11.3. It proves distinct process
operations for Next Message Request and Next Message Request Available, the
official logical-time codec, callback-gated grants, and Query Logical Time
readback. Queued TSO selection, inclusive GALT coordination, Flush Queue,
multi-federate ordering, save/restore, package/JUnit, validation, and
conformance remain separate lanes.

The queued-TSO companion is independently selectable:

```powershell
python tools/query_rti_work.py focus process-time-advance-next-message-queued-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
python tools/query_rti_work.py check --lane process-time-advance-next-message-queued-focused --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors select queued timestamped process messages for next-message advances$" --output-on-failure
```

This 101-assertion two-federate `HLA_EVOKED`/`HLA_IMMEDIATE` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12600` sends timestamped
interactions at 5 and 8, proves NMR(10) selects the earliest queued message,
then proves NMRA(10) selects the later one. Each interaction callback precedes
its matching grant, and Query Logical Time confirms 5 then 8. It remains
development-profile process evidence; GALT/LITS, Flush Queue, retraction,
multi-recipient ordering, save/restore, package/JUnit, review, validation,
interoperability, and conformance remain separate lanes.

The preceding process lookup slice is the reverse-FOM lookup lane and is
independently selectable:

```powershell
python tools/query_rti_work.py focus reverse-fom-lookup --summary --compact
python tools/query_rti_work.py trace "RTIambassador reports reverse FOM lookup errors through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix reverse-fom-lookup --summary --compact
python tools/query_rti_work.py check --lane reverse-fom-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador reports reverse FOM lookup errors through a configured process endpoint$" --output-on-failure
```

The lane now contains four source-located cases and 76 aggregate assertions
(m102 contributes 20; m103 contributes 24; m104 contributes 16; m105
contributes 16) under `HLA_EVOKED` and `HLA_IMMEDIATE`. The m105 case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13785` maps six Lab
requirements to clauses `9.1.2`, `10.19`, and `10.20.4`, and exercises the
unknown-name/invalid-handle error fence across four official C++ API surfaces.
The m104 dimension/transportation round-trip, m103, m102, and m101 cases remain
queryable by their exact titles. Multi-federate declaration management,
package/JUnit, review, validation, interoperability, and conformance remain
separate.

The newest bounded process callback-ordering slice is independently selectable:

```powershell
python tools/query_rti_work.py focus process-multi-recipient-callback-ordering --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-multi-recipient-callback-ordering --summary --compact
python tools/query_rti_work.py check --lane process-multi-recipient-callback-ordering --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint$" --output-on-failure
```

This one source-located case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:14106` records 79 assertions
under `HLA_EVOKED` and `HLA_IMMEDIATE`. It maps ten Lab requirements to six canonical 2025
subsections and five official C++ API surfaces, proving independent per-recipient
FIFO interaction delivery, preserved tags/parameters/producer identity, one
receive callback per Evoke Callback, and sender exclusion. Immediate delivery,
callback-disable, timestamped/region/directed fanout, package/JUnit/protected
review, validation, interoperability, and conformance remain separate.

The newest bounded process TSO/DDM slice is independently selectable:

```powershell
python tools/query_rti_work.py focus timestamped-process-regional-interaction --summary --compact
python tools/query_rti_work.py trace m109.embedded-process-tso-regional-interaction --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane timestamped-process-regional-interaction --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a timestamped regional interaction through a configured process endpoint$" --output-on-failure
```

The m109 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:15195`
records 71 assertions under `HLA_EVOKED`, maps 28 Lab anchors to 16 canonical
2025 subsections, and exercises 20 official C++ API surfaces. It proves an
overlap-qualified timestamped regional interaction crosses the process boundary,
remains gated until the constrained receiver advances, and preserves region,
timestamp, order, tag, producer, and retraction metadata. Keep callback-disable,
directed fanout, relaxed DDM, package/JUnit/protected review, validation,
interoperability, and conformance in separate lanes.

The preceding bounded process DDM slice is independently selectable:

```powershell
python tools/query_rti_work.py focus process-multi-recipient-regional-interaction --summary --compact
python tools/query_rti_work.py trace m108.embedded-process-multi-recipient-regional-interaction --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-multi-recipient-regional-interaction --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint$" --output-on-failure
```

The m108 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:14552`
records 126 assertions under both callback models, maps thirteen Lab anchors
to seven canonical 2025 subsections, and exercises thirteen official C++ API
surfaces. It proves overlap-filtered delivery to two disjoint regional
recipients, send-time source-region metadata through Convey Region Designator
Sets, and sender exclusion. Keep timestamped/retraction, directed,
relaxed-DDM, callback-disable, package/JUnit, review, validation,
interoperability, and conformance in separate lanes.

The preceding available-dimensions hierarchy slice is independently selectable:

```powershell
python tools/query_rti_work.py focus process-available-dimensions-hierarchy --summary --compact
python tools/query_rti_work.py trace "RTIambassador resolves available FOM dimensions through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-available-dimensions-hierarchy --summary --compact
python tools/query_rti_work.py check --lane process-available-dimensions-hierarchy --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador resolves available FOM dimensions through a configured process endpoint$" --output-on-failure
```

This one source-located case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13933` records 28 assertions
under `HLA_EVOKED` and `HLA_IMMEDIATE`. It resolves inherited object- and
interaction-class dimension sets through the process-owned FOM catalog,
preserves an empty set, and maps unknown class handles to the official
invalid-handle exceptions. It is development-profile process evidence;
process region lifecycle/routing, package/JUnit/protected review, validation,
interoperability, and conformance remain separate.

The matching process rejection fence is independently runnable:

```powershell
python tools/query_rti_work.py focus process-time-advance-rejection --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects a backward time advance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects a backward time advance through a configured process endpoint" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a backward time advance through a configured process endpoint$" --output-on-failure
```

This 12-assertion `HLA_EVOKED` case proves the official
`LogicalTimeAlreadyPassed` exception after an endpoint-owned grant and keeps
the grant callback count stable. Pending-role cases and distributed
GALT/LITS/TSO scheduling remain separate lanes.

The incompatible-time decode fence is independently runnable:

```powershell
python tools/query_rti_work.py focus process-time-advance-invalid-time --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects an incompatible logical time through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects an incompatible logical time through a configured process endpoint" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects an incompatible logical time through a configured process endpoint$" --output-on-failure
```

This 9-assertion `HLA_EVOKED` case proves the official `InvalidLogicalTime`
exception for an HLAfloat64Time supplied to an HLAinteger64Time federation and
confirms that no grant callback is created.

The process callback-gated pending-role fence has its own selector:

```powershell
python tools/query_rti_work.py focus process-time-advance-time-regulation-pending --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects a process time advance while time regulation enable is pending" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects a process time advance while time regulation enable is pending" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a process time advance while time regulation enable is pending$" --output-on-failure
```

This 14-assertion `HLA_EVOKED` case keeps the official
`RequestForTimeRegulationPending` guard active until the queued
`timeRegulationEnabled` callback crosses the dispatcher. The analogous
Time-Constrained pending case is independently mapped below; malformed
encoding, distributed scheduling, and conformance remain separate slices.

The matching process callback-gated constrained-role fence has its own
selector:

```powershell
python tools/query_rti_work.py focus process-time-advance-time-constrained-pending --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects a process time advance while time constrained enable is pending" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects a process time advance while time constrained enable is pending" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a process time advance while time constrained enable is pending$" --output-on-failure
```

This 14-assertion `HLA_EVOKED` case keeps the official
`RequestForTimeConstrainedPending` guard active until the queued
`timeConstrainedEnabled` callback crosses the dispatcher. It maps the
Enable Time Constrained request/callback and Time Advance Request to clauses
8.5.5, 8.6.3, and 8.8.3; malformed encoding, distributed scheduling, and
conformance remain separate slices.

The malformed process logical-time decode fence is independently runnable:

```powershell
python tools/query_rti_work.py focus process-time-advance-malformed-encoding --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects malformed logical-time encoding through a configured process endpoint$" --output-on-failure
```

This 9-assertion `HLA_EVOKED` case injects a one-byte malformed logical-time
encoding at the process boundary, proves the official `InvalidLogicalTime`
exception, and confirms that no grant callback is emitted. It maps directly to
clause 8.8.3; valid grants, pending-role fences, distributed scheduling, and
conformance remain separate slices.

The process federation scheduler slice is independently runnable:

```powershell
python tools/query_rti_work.py focus process-time-advance-federation-scheduler --summary --compact
python tools/query_rti_work.py trace "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors coordinate deferred process time advances through the federation scheduler$" --output-on-failure
```

This 30-assertion `HLA_EVOKED` two-federate case maps the Enable Time
Regulation, Enable Time Constrained, Time Advance Request, and Time Advance
Grant surfaces to clauses 8.2, 8.5.5, 8.6.3, and 8.8.3. It proves that a
constrained request remains deferred until the regulating federate advances,
then travels as an unsolicited grant event through the process boundary and
the official callback bridge. Timestamped TSO delivery, save/restore,
package/JUnit, validation, interoperability, and conformance remain separate.

The timestamped process-interaction-before-grant slice is independently
runnable:

```powershell
python tools/query_rti_work.py focus process-tso-interaction-before-grant --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a deferred timestamped process interaction before the grant$" --output-on-failure
```

This 60-assertion `HLA_EVOKED` case maps 15 Requirements-Lab anchors to nine
canonical 2025 sections and proves timestamped `MainCourseServed` delivery
crosses the process boundary before the matching grant while preserving the
official parameter/tag/transportation/producer/time/order callback surface.
Pre-grant retraction, multi-message ordering, directed/regional TSO,
save/restore, package/JUnit, validation, interoperability, and conformance
remain separate slices.

The timestamped process-attribute-update slice is independently runnable at
both the private service and public endpoint levels:

```powershell
python tools/query_rti_work.py focus process-tso-attribute-before-grant --summary --compact
python tools/query_rti_work.py trace "Private process service releases timestamped Update Attribute Values before a constrained grant" --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^(umbra\\.process_boundary_private\\.catch2\\.Private process service releases timestamped Update Attribute Values before a constrained grant|umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassadors deliver a deferred timestamped process attribute update before the grant)$" --output-on-failure
```

The two `HLA_EVOKED` cases total 109 assertions and share 12 mapped
Requirements-Lab anchors across eight canonical 2025 sections. The private
case is the fast transport/registry contract; the public case proves the
official `Update Attribute Values` and `Reflect Attribute Values` surfaces,
including the reflection-before-grant ordering. Keep retraction stress,
multiple-message ordering, fanout, regional/DDM, save/restore, package/JUnit,
validation, interoperability, and conformance as separate slices.

The support-switch state slice is queryable independently of MOM interactions:

```powershell
python tools/query_rti_work.py focus support-switch-state --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded support switches are seeded per federate and retain static FDD policy" --summary --compact
python tools/query_rti_work.py matrix "Embedded support switches are seeded per federate and retain static FDD policy" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded support switches are seeded per federate and retain static FDD policy$" --output-on-failure
```

It maps the twelve official support-switch APIs to eight exact Lab candidates
across clauses 8.1.10, 9.1.8, 10.44, 10.45.3, 10.46.6, 10.48.1, 10.50.6,
and 10.55.1, with 40 `HLA_EVOKED` assertions. The case proves FDD-seeded
per-federate state, mutation isolation, static getters, and invalid resign
action handling. `HLAsetSwitches`, report-service subscription interlocks,
connection-loss cleanup, delayed timestamped delivery, relaxed-DDM routing,
filesystem reporting, and conformance remain separate lanes.

The whole-object-class declaration teardown has a dedicated handle:

```powershell
python tools/query_rti_work.py focus whole-object-class-declaration --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries" --summary --compact
python tools/query_rti_work.py matrix "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.fom_declaration_management\.catch2\.Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries$" --output-on-failure
```

It maps whole-class unpublication/unsubscription and the associated attribute
declaration lifecycle to eight exact Lab candidates in clauses 5.3, 5.3.3,
and 5.9, with 34 `HLA_EVOKED` assertions. The case verifies invalid-handle
and membership fences, inherited publication lifetime, ordinary-subscription
removal with regional state kept independent, idempotent teardown, and update
rejection after unpublication. Broader publication setup, ownership
arbitration, regional teardown, save/restore, transport, packaging, and
conformance remain separate.

The FOM-module and Annex C reference-resolution foundations are separate
bounded lanes:

```powershell
python tools/query_rti_work.py focus fom-module-management --summary --compact
python tools/query_rti_work.py focus reference-resolution --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight resolves transportation names after the complete module set is merged" --summary --compact
python tools/query_rti_work.py matrix reference-resolution --summary --compact
python tools/query_rti_work.py check --lane fom-module-management --summary --compact
python tools/query_rti_work.py check --lane reference-resolution --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.fom_declaration_management\.catch2\." --output-on-failure
ctest --test-dir <build-dir> -C Debug -L "^reference-resolution$" --output-on-failure
```

The indexed foundation slices contain three FOM-management cases and eight
reference-resolution cases, with 36 and 50 assertions respectively. Keep
their FOM 2025/1516.2 mappings separate from later declaration, DDM, transport,
and conformance claims.

The public handle-decoding API slice is explicitly dispositioned and queryable:

```powershell
python tools/query_rti_work.py focus public-handle-decoding --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded public handle decoders enforce lifecycle and preserve encoded identities" --summary --compact
python tools/query_rti_work.py matrix "Embedded public handle decoders enforce lifecycle and preserve encoded identities" --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded public handle decoders enforce lifecycle and preserve encoded identities$" --output-on-failure
```

The Requirements Lab exports the eight official decoding API surfaces but no
standalone 2025 requirement candidate for this codec behavior, so this is an
explicit API-traceability disposition rather than an invented mapping. The
case records 50 assertions for lifecycle fences, federation-scoped round
trips, and malformed-value rejection; cross-RTI interoperability, transport,
packaging, protected review, and conformance remain separate.

The mandatory order-type lookup pair has its own focused C++ target:

```powershell
python tools/query_rti_work.py focus order-type-lookup --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.order_type_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded order type lookup exposes the mandatory 2025 Receive and TimeStamp pair" --summary --compact
```

The case maps Get Order Type/Get Order Name and the legal Receive/TimeStamp
pair to five exact 2025 Lab candidates in clauses 8.2, 10.17.4, and 10.19.
Order-control services remain separate.

The order-type control slice has its own focused C++ target:

```powershell
python tools/query_rti_work.py focus order-type-control --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.order_type_control\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded order type control captures defaults, instance overrides, and publisher interaction order" --summary --compact
python tools/query_rti_work.py matrix "Embedded order type control captures defaults, instance overrides, and publisher interaction order" --summary --compact
```

It maps the three official order-control APIs to nine exact 2025 Lab
candidates in clauses 8.24.4, 8.25.3, and 8.26.4. The focused harness drains
the runtime's legitimate registration callback; alternate time/TSO modes,
save/restore, and conformance remain separate.

The ordinary declaration-relevance advisory slice is independently runnable:

```powershell
python tools/query_rti_work.py focus declaration-relevance-advisory --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.declaration_relevance_advisory\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded declaration relevance advisories follow ordinary 2025 publication and subscription transitions" --summary --compact
python tools/query_rti_work.py matrix "Embedded declaration relevance advisories follow ordinary 2025 publication and subscription transitions" --summary --compact
```

It maps the four RTI-initiated declaration callbacks and four relevance-switch
accessors to 13 unique 2025 Lab candidates in clauses 5.8, 5.10.2, 5.14.3,
5.15.3, 5.16.5, and 5.17.6. Regional declarations and service-report ordering
remain separate lanes.

The regional declaration-relevance advisory slice is independently runnable:

```powershell
python tools/query_rti_work.py focus declaration-relevance-advisory-regional --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.regional_declaration_relevance_advisory\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded regional declaration relevance advisories follow active subscriptions" --summary --compact
python tools/query_rti_work.py matrix "Embedded regional declaration relevance advisories follow active subscriptions" --summary --compact
```

It maps the region-scoped declaration services and four RTI-initiated callbacks
to 13 exact 2025 Lab candidates across the six declaration-management clauses
and §9.8. Complete regional DDM routing remains separate.

The registered-object regional passive-triple slice is independently runnable:

```powershell
python tools/query_rti_work.py case umbra-cpp-regional-passive-subscription-turn-updates-integration --summary --compact
python tools/query_rti_work.py focus regional-passive-subscription-turn-updates --summary --compact
python tools/query_rti_work.py trace "Embedded regional passive subscription changes gate registered-object turn updates per region" --summary --compact
python tools/query_rti_work.py matrix regional-passive-subscription-turn-updates --summary --compact
python tools/query_rti_work.py check --lane regional-passive-subscription-turn-updates --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.regional_declaration_relevance_advisory\.catch2\.Embedded regional passive subscription changes gate registered-object turn updates per region$" --output-on-failure
```

It adds 54 HLA_EVOKED assertions for §9.8 lines 90, 102, and 120, proving
object discovery and Turn Updates On/Off gating for one overlapping triple
while an independent disjoint triple remains active. Timestamped/retraction,
broader regional overlap, package/JUnit/protected-review, validation, and
conformance remain separate.

The declaration-relevance service-report boundary is independently runnable:

```powershell
python tools/query_rti_work.py focus declaration-relevance-service-report --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.declaration_relevance_service_report\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded service reporting records declaration relevance advisories before callbacks" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records declaration relevance advisories before callbacks" --summary --compact
```

It records 143 HLA_EVOKED assertions against the production filesystem-backed
service-report store. The lane maps the four advisory callbacks plus the two
MOM service-report clauses (11.5 and 11.5.2) to six exact 2025 Lab anchors,
including Table 5 handle forms, per-file serial order, and durable-before-
callback ordering. Public MOM interaction delivery remains a separate lane.

The core asynchronous-delivery and time-advance slices now carry direct Lab
maps as well. They remain aggregate-plan evidence (there is no small dedicated
runtime target yet), so use the bounded trace/focus handles rather than a full
suite run:

```powershell
python tools/query_rti_work.py focus asynchronous-delivery --summary --compact
python tools/query_rti_work.py trace "Embedded asynchronous delivery gates receive-order callbacks by temporal state" --summary --compact
python tools/query_rti_work.py focus time-advance-request --summary --compact
python tools/query_rti_work.py trace "Embedded Time Advance Request changes logical time only at Time Advance Grant dispatch" --summary --compact
python tools/query_rti_work.py trace "Embedded Create Federation Execution accepts a validated explicit MIM path" --summary --compact
python tools/query_rti_work.py trace "Embedded federation shares the static Advisories Use Known Class switch" --summary --compact
python tools/query_rti_work.py trace "Embedded Next Message Request grants at the next queued TSO timestamp" --summary --compact
python tools/query_rti_work.py trace "Embedded Available time advances use inclusive GALT and queued TSO delivery" --summary --compact
python tools/query_rti_work.py trace "Embedded federate lookup services preserve departed designator identities within the joined federation" --summary --compact
```

These handles expose the exact source line, assertion count, official C++ API
surfaces, Requirements-Lab IDs, and canonical 2025 subsection keys without
reloading the unchanged Lab export.

The ownership-transfer/update-region cleanup case is likewise isolated:

```powershell
python tools/query_rti_work.py focus ownership-transfer-update-region --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ownership_transfer_update_region\.catch2\.Embedded ownership transfer clears the former owner's 2025 update-region association$" --output-on-failure
```

This lane reports its five requirement-to-subsection pairs and 88 assertions;
the focused target is `umbra_ownership_transfer_update_region_catch2`.

The current time-management handoff is the joined-federate MOM reflection-count
slice. It is a standalone 103-assertion `HLA_EVOKED` case, mapped to one Lab
requirement and clause 11.4.1:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-reflection-counts --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM separates reflection totals from distinct objects" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-reflection-counts --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-reflection-counts --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_reflection_count\.catch2\.Embedded joined-federate MOM separates reflection totals from distinct objects$" --output-on-failure
```

It proves distinct-object values 0/1/1/2 versus callback-invocation totals
0/1/2/3 for repeated receive-order updates, then 2/4 after a queued
timestamped reflection and one periodic `HLAsetTiming` reflection. The
preceding time-state-duration, optimistic Flush Queue, joined-owner,
delivered-recipient, and no-recipient lanes remain directly queryable as
`joined-federate-mom-time-state-duration`, `flush-queue-request-optimistic-time`,
`tso-object-deletion-joined-owner-retraction`,
`tso-object-deletion-retraction-reconstitution`, and
`tso-attribute-update-no-fanout-designator`. After the current slice, run
`ready --summary --compact` for the next exact planned row; do not infer work
from an unscoped Catch2 or Requirements-Lab scan.

The next completed time-management slice is the joined-federate MOM
updates-sent case. It is a standalone 98-assertion `HLA_EVOKED` case at
`joined_federate_mom_updates_sent_catch2.cpp:145`, mapped to one Lab
requirement and clause 11.4.1:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-updates-sent --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestUpdatesSent reports class and transportation counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-updates-sent --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-updates-sent --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_updates_sent\\.catch2\\.Embedded MOM requestUpdatesSent reports class and transportation counts$" --output-on-failure
```

It proves two best-effort `Server` updates and one reliable `Soda` update,
one reliable RTI-originated report per supported transportation, nested class
count decoding, and empty NULL buckets for an idle joined federate. The
reflection-count slice and earlier time-state-duration, optimistic Flush
Queue, joined-owner, delivered-recipient, and no-recipient lanes remain
directly queryable; run `ready --summary --compact` for the next exact planned
row.

The completed time-management slice is the joined-federate MOM
interactions-sent case. It is a standalone 107-assertion `HLA_EVOKED` case at
`joined_federate_mom_interactions_sent_catch2.cpp:153`, mapped to one Lab
requirement and clause 11.4.1:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-interactions-sent --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestInteractionsSent reports class and transportation counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-interactions-sent --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-interactions-sent --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_interactions_sent\\.catch2\\.Embedded MOM requestInteractionsSent reports class and transportation counts$" --output-on-failure
```

It proves one reliable and one best-effort `TakeOrder`, one reliable regional
`MainCourseServed`, one reliable RTI-originated report per supported
transportation, nested interaction-count decoding, and empty NULL buckets for
an idle joined federate. The updates-sent, reflection-count, and earlier
time-state-duration, optimistic Flush Queue, joined-owner, delivered-recipient,
and no-recipient lanes remain directly queryable; run `ready --summary --compact`
for the next exact planned row.

The cross-family MOM sender-count NULL-bucket slice is independently buildable
at `mom_sender_count_reports_null_buckets_catch2.cpp:116`. It is a standalone
111-assertion `HLA_EVOKED` case, mapped to one Lab requirement and clause
11.4.1. It requests updates-sent, interactions-sent, and directed-interactions-
sent reports for an idle federate, and verifies two reliable RTI-originated
transportation buckets per family with official empty nested count arrays:

```powershell
python tools/query_rti_work.py focus mom-sender-count-reports-null-buckets --summary --compact
python tools/query_rti_work.py trace "Embedded MOM sender count reports emit NULL buckets for empty ledgers" --summary --compact
python tools/query_rti_work.py matrix mom-sender-count-reports-null-buckets --summary --compact
python tools/query_rti_work.py check --lane mom-sender-count-reports-null-buckets --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.mom_sender_count_reports_null_buckets\.catch2\.Embedded MOM sender count reports emit NULL buckets for empty ledgers$" --output-on-failure
```

The updates-sent and interactions-sent lanes remain directly queryable; run
`ready --summary --compact` for the next exact planned row.

The completed receiver-ledger MOM reflections-received slice is independently
buildable at `joined_federate_mom_reflections_received_catch2.cpp:194`. It is a
standalone 129-assertion `HLA_EVOKED` case, mapped to one Lab requirement and
clause 11.4.1. It records two best-effort Server reflections and one reliable
Soda reflection, decodes one populated `HLAreportReflectionsReceived` bucket
per supported transportation, and verifies two empty `HLAreflectCounts` NULL
buckets for an idle federate:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-reflections-received --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestReflectionsReceived reports class and transportation counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-reflections-received --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-reflections-received --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.joined_federate_mom_reflections_received\.catch2\.Embedded MOM requestReflectionsReceived reports class and transportation counts$" --output-on-failure
```

The NULL-bucket and sender-count lanes remain directly queryable; run
`ready --summary --compact` for the next exact planned row.

The completed receiver-ledger MOM interactions-received slice is independently
buildable at `joined_federate_mom_interactions_received_catch2.cpp:149`. It is
a standalone 113-assertion `HLA_EVOKED` case, mapped to one Lab requirement and
clause 11.4.1. It records one reliable and one best-effort `TakeOrder` receive
at the represented federate, decodes the official `HLAtransportation` and
nested `HLAinteractionCounts` report values, and verifies two empty NULL
buckets for an idle federate:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-interactions-received --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestInteractionsReceived reports class and transportation counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-interactions-received --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-interactions-received --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.joined_federate_mom_interactions_received\.catch2\.Embedded MOM requestInteractionsReceived reports class and transportation counts$" --output-on-failure
```

The completed directed receiver-ledger MOM slice is independently buildable at
`joined_federate_mom_directed_interactions_received_catch2.cpp:180`. It is a
standalone 119-assertion `HLA_EVOKED` case, mapped to one Lab requirement and
clause 11.4.1. It proves an ordinary `TakeOrder` receive stays out of the
directed ledger, counts one directed reliable receive, decodes nested
`HLAinteractionCounts`, and verifies empty best-effort and idle NULL buckets:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-directed-interactions-received --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestDirectedInteractionsReceived reports directed counts and NULL buckets" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-directed-interactions-received --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-directed-interactions-received --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.joined_federate_mom_directed_interactions_received\.catch2\.Embedded MOM requestDirectedInteractionsReceived reports directed counts and NULL buckets$" --output-on-failure
```

The completed federation-wide MOM `HLAsetSwitches` Auto Provide mutation is a
standalone 41-assertion `HLA_EVOKED` case at
`cpp/tests/auto_provide_mom_catch2.cpp:120`. It exercises 18 official C++ API
surfaces and is explicitly anchored to canonical 2025 clauses `4`, `6.1.10`,
and `11.4.1`. RL-032 has no standalone Lab requirement row for this Table 20
parameter, so its zero Lab-requirement count is intentional and remains an
explicit development-profile disposition:

```powershell
python tools/query_rti_work.py trace m90.embedded-mom-hlasetswitches-auto-provide --summary --compact
python tools/query_rti_work.py matrix mom-auto-provide-switch-mutation --summary --compact
python tools/query_rti_work.py focus mom-auto-provide-switch-mutation --summary --compact
python tools/query_rti_work.py check --lane mom-auto-provide-switch-mutation --summary --compact
cmake --build .build --config Debug --target umbra_auto_provide_mom_catch2
ctest --test-dir .build -C Debug -R "^umbra\.auto_provide_mom\.catch2\.Embedded MOM HLAsetSwitches adjusts federation-wide Auto Provide$" --output-on-failure
```

The completed service-report writer-creation failure slice is a standalone
10-assertion `HLA_EVOKED` case at
`cpp/tests/service_report_writer_failure_catch2.cpp:92`. It maps one
Requirements-Lab candidate to canonical 2025 clause `11.5.2` and exercises six
official federation-management C++ API surfaces. The injected internal store
seam proves deterministic `RTIinternalError`, no `memory://` fallback, and
join-membership rollback; production permission/full-disk, cross-process,
package/JUnit/protected-review, validation, interoperability, and conformance
remain separate:

```powershell
python tools/query_rti_work.py trace m91.embedded-service-report-writer-creation-failure --summary --compact
python tools/query_rti_work.py matrix service-report-writer-failure --summary --compact
python tools/query_rti_work.py focus service-report-writer-failure --summary --compact
python tools/query_rti_work.py check --lane service-report-writer-failure --summary --compact
cmake --build .build --config Debug --target umbra_service_report_writer_failure_catch2
ctest --test-dir .build -C Debug -R "^umbra\.service_report_writer_failure\.catch2\.Service-report writer creation failure rejects the join without an in-memory fallback$" --output-on-failure
```

The completed subscription-generation restore slice is independently
addressable at `libxml2_fom_composer_catch2.cpp:1742`. It is a 31-assertion
native C++ unit case mapped to the Requirements-Lab §4.32 candidate and
canonical clause `4.32`; it restores declaration state and proves the next
subscription mutation consumes the saved generation identity:

```powershell
python tools/query_rti_work.py trace m92.federation-registry-subscription-generation-restore --summary --compact
python tools/query_rti_work.py matrix subscription-generation --summary --compact
python tools/query_rti_work.py focus subscription-generation --summary --compact
python tools/query_rti_work.py check --lane subscription-generation --summary --compact
cmake --build .build --config Debug --target umbra_fom_composer_catch2
ctest --test-dir .build -C Debug -R "^umbra\.fom_composer\.catch2\.The federation registry restores subscription-generation allocation with declaration state$" --output-on-failure
```

The completed filesystem save-history process-restart slice is independently
buildable in `federation_registry_catch2.cpp:6661`. It is a 42-assertion
`HLA_EVOKED` native C++ case mapped to ten Requirements-Lab candidates and
canonical clauses `4.19`, `4.20`, `4.27`, and `11.4.1`. It writes two durable
state images, restores the second image into a fresh registry, and proves the
application-visible federation save-history conditionals survive before a
subsequent durable commit; timed save-history, public MOM delivery, remote
transport, package/JUnit/protected-review evidence, interoperability, and
conformance remain separate:

```powershell
python tools/query_rti_work.py trace m93.federation-save-history-process-restart --summary --compact
python tools/query_rti_work.py matrix federation-save-history-process-restart --summary --compact
python tools/query_rti_work.py focus federation-save-history-process-restart --summary --compact
python tools/query_rti_work.py check --lane federation-save-history-process-restart --summary --compact
cmake --build .build --config Debug --target umbra_federation_registry_catch2
ctest --test-dir .build -C Debug -R "^umbra\.federation_registry\.catch2\.Filesystem state image restores federation save conditionals in a fresh registry$" --output-on-failure
```

The public application-value process-restart companion is independently
source-located at `cpp/tests/public_process_restart_application_value_catch2.cpp:189`.
It is an 82-assertion HLA_EVOKED case mapped to nine Requirements-Lab anchors,
five canonical 2025 sections, and 18 official C++ API surfaces. Use the exact
lane handles below for fresh-registry value rehydration, post-restore save
continuity, and immutable service-report-file identity:

```powershell
python tools/query_rti_work.py focus public-process-restart-application-value-focused --summary --compact
python tools/query_rti_work.py trace "Embedded public fresh-registry restore rehydrates application value and retains report files" --summary --compact
python tools/query_rti_work.py matrix public-process-restart-application-value-focused --summary --compact
python tools/query_rti_work.py check --lane public-process-restart-application-value-focused --summary --compact
cmake --build .build --config Debug --target umbra_public_process_restart_application_value_catch2
ctest --test-dir .build -C Debug -R "^umbra\.public_process_restart_application_value\.catch2\.Embedded public fresh-registry restore rehydrates application value and retains report files$" --output-on-failure
```

This is development-profile evidence only; pending application-request
ledgers, timestamped payloads, ownership transfer, remote/package/JUnit/
protected-review evidence, interoperability, and conformance remain separate.

The completed m59 timestamped Delay Subscription Evaluation slice is
independently buildable at
`delay_subscription_evaluation_timestamped_directed_interaction_catch2.cpp:129`.
It is a 166-assertion `HLA_EVOKED`/`HLA_IMMEDIATE` case mapped to the three
Requirements-Lab candidates in canonical clause `8.1.10`:

```powershell
python tools/query_rti_work.py trace m59.embedded-delay-subscription-evaluation-timestamped-directed-interaction --summary --compact
python tools/query_rti_work.py matrix delay-subscription-evaluation-timestamped-directed-interaction --summary --compact
python tools/query_rti_work.py focus delay-subscription-evaluation-timestamped-directed-interaction --summary --compact
python tools/query_rti_work.py check --lane delay-subscription-evaluation-timestamped-directed-interaction --summary --compact
cmake --build .build --config Debug --target umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2
ctest --test-dir .build -C Debug -R "^umbra\.delay_subscription_evaluation_timestamped_directed_interaction\.catch2\.Embedded Delay Subscription Evaluation defers timestamped directed interaction eligibility$" --output-on-failure
```

The completed m60 timestamped directed-interaction TSO/retraction slice is
independently buildable at
`timestamped_directed_interaction_retraction_catch2.cpp:150`. It is a
56-assertion `HLA_EVOKED` case mapped to the 15 Requirements-Lab anchors
across canonical clauses `4.1.1`, `4.2`, `4.5.5`, `4.6.5`, `4.11.4`, `4.12`,
`5.1.5`, `6.13`, `8.1.5`, `8.1.6`, `8.8.3`, `8.22.3`, and `8.23.3`:

```powershell
python tools/query_rti_work.py trace m60.embedded-timestamped-directed-interaction-tso-retraction --summary --compact
python tools/query_rti_work.py matrix timestamped-directed-interaction-tso-retraction --summary --compact
python tools/query_rti_work.py focus timestamped-directed-interaction-tso-retraction --summary --compact
python tools/query_rti_work.py check --lane timestamped-directed-interaction-tso-retraction --summary --compact
cmake --build .build --config Debug --target umbra_timestamped_directed_interaction_retraction_catch2
ctest --test-dir .build -C Debug -R "^umbra\.timestamped_directed_interaction_retraction\.catch2\.Embedded timestamped directed interaction queues TSO before the grant and supports retraction$" --output-on-failure
```

The completed timestamped regional attribute source-resignation slice is
independently buildable at
`timestamped_regional_attribute_update_resignation_catch2.cpp:146`. It is a
standalone 58-assertion `HLA_EVOKED` case mapped to eight Lab requirements and
six canonical 2025 sections. It queues one timestamped update from an ordinary
registration's private default source, keeps the regional subscriber pending
while the source resigns, releases the callback through an independent
regulator before the receiver's TAR(7) grant, preserves the supplied-empty
`RegionHandleSet` and retraction metadata, and rejects post-resignation
`Retract` with `FederateNotExecutionMember`:

```powershell
python tools/query_rti_work.py focus timestamped-regional-attribute-update-resignation --summary --compact
    python tools/query_rti_work.py trace "Embedded queued timestamped regional attribute update survives source resignation" --summary --compact
    python tools/query_rti_work.py matrix timestamped-regional-attribute-update-resignation --summary --compact
    python tools/query_rti_work.py check --lane timestamped-regional-attribute-update-resignation --summary --compact
    cmake --build .build --config Debug --target umbra_timestamped_regional_attribute_update_resignation_catch2
    ctest --test-dir .build -C Debug -R "^umbra\.timestamped_regional_attribute_update_resignation\.catch2\.Embedded queued timestamped regional attribute update survives source resignation$" --output-on-failure
```

The requirements-facing `timestamped-default-region-attribute-update-resignation`
lane is an explicit alias of this same executable evidence; query it for the
default-region requirement view without creating or rerunning a duplicate case.

The completed timed negotiated-cancellation-after-restore slice is independently
buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore_catch2.cpp:263`.
It is a standalone 135-assertion `HLA_EVOKED` case mapped to 44 Lab
requirements and 23 canonical 2025 sections. It restores one saved logical-
time-8 explicit-source regional update, queues an If Available willing-to-
acquire reservation, enters negotiated divestiture, and has the requester
resign with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` before confirmation. The
stale negotiated confirmation path is suppressed, ownership stays with the
source, and the surviving constrained recipient receives exactly one saved
reflection at its Flush Queue boundary:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending negotiated ownership transfer after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending negotiated ownership transfer after restore$" --output-on-failure
```

The case is bounded cancellation evidence; negotiated transfer completion,
multi-candidate arbitration, alternate callback models, passive/relaxed DDM,
remote transport, package/JUnit/protected review, Lab validation, and
conformance remain separate.

The completed timed negotiated-continuation-after-restore slice is independently
buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore_catch2.cpp:283`.
It is a standalone 158-assertion `HLA_EVOKED` case mapped to 44 Lab
requirements and 23 canonical 2025 sections. It restores one saved logical-
time-8 explicit-source regional update, queues two If Available requests before
negotiated divestiture, and has the first requester resign with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`. The second request's callback remains
pending; reissued negotiation forwards its acquisition tag to the owner,
surviving regional delivery occurs before the Flush Queue grant, and Confirm
Divestiture transfers ownership with one terminal acquisition notification.
The original If Available callback is drained only after confirmation so stale
callback work cannot consume the candidate first:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues to a retained negotiated ownership candidate after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues to a retained negotiated ownership candidate after restore$" --output-on-failure
```

This is bounded callback-order and negotiated-transfer evidence; it does not
claim a persistent Willing-to-Acquire reservation after an If Available callback
has been delivered, complete arbitration, alternate callback models,
passive/relaxed DDM, remote transport, package/JUnit/protected review, Lab
validation, or conformance.

The completed timed negotiated-confirmation-cancel-after-restore slice is
independently buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore_catch2.cpp:283`.
It is a standalone 161-assertion `HLA_EVOKED` case mapped to 48 Lab
requirements and 26 canonical 2025 sections. It restores the saved
logical-time-8 explicit-source regional update, queues two If Available
candidates, reselects the retained candidate after the first requester
resigns with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, then cancels the
negotiated divestiture after Request Divestiture Confirmation. The owner keeps
ownership, stale Confirm Divestiture is rejected, the queued clock callback
reports unavailable, and no owner-release callback is emitted:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels retained negotiated owner confirmation after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update cancels retained negotiated owner confirmation after restore$" --output-on-failure
```

This is bounded cancellation evidence; it does not claim a persistent
post-callback Willing-to-Acquire reservation, negotiated transfer completion,
alternate callback models, passive/relaxed DDM, remote transport,
package/JUnit/protected review, Lab validation, interoperability, or
conformance.

The completed timed mixed regular-to-If-Available continuation-after-restore
slice is independently buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore_catch2.cpp:15`.
It is a standalone 161-assertion `HLA_EVOKED` case mapped to 46 Lab
requirements, 24 canonical 2025 sections, and 31 selected official C++ API
surfaces. It restores the logical-time-8 explicit-source regional update,
queues a regular candidate for the first constrained recipient and an If
Available candidate on the independent clock, then reissues negotiated
divestiture after the first requester resigns. The registry retains the If
Available reservation until `Confirm Divestiture`; the stale ordinary callback
is suppressed, surviving regional reflections precede the common Flush Queue
grant, and the retained clock receives one terminal acquisition notification:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to an If Available candidate after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to an If Available candidate after restore$" --output-on-failure
```

This remains bounded development-profile evidence; passive/relaxed DDM,
alternate callback models, remote transport, package/JUnit/protected review,
Requirements-Lab validation, interoperability, and conformance are separate.

The completed timed mixed If-Available-to-retained-regular pre-delivery
cancellation slice is independently buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore_catch2.cpp:15`.
It is a standalone 161-assertion `HLA_EVOKED` case mapped to 50 Lab
requirements, 26 canonical 2025 sections, and 32 selected official C++ API
surfaces. It restores the logical-time-8 explicit-source regional update,
queues an If Available candidate for the first constrained recipient and a
regular candidate on the independent clock, reissues negotiation after
requester resignation, and cancels before `Request Divestiture Confirmation`
enters user code. Stale confirmation work is consumed without an acquisition
notification, ownership remains with the publisher, and both surviving
regional recipients reflect before the common Flush Queue grant:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation before delivery after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation before delivery after restore$" --output-on-failure
```

This remains bounded development-profile evidence; alternate callback models,
passive/relaxed DDM, remote transport, package/JUnit/protected review,
Requirements-Lab validation, interoperability, and conformance are separate.

The regular-to-regular negotiated continuation is independently source-backed
through a direct wrapper at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_lane_catch2.cpp:11`.
It is a 164-assertion `HLA_EVOKED` case mapped to 46 Lab requirements, 24
canonical 2025 sections, and 30 selected official C++ API surfaces. The
wrapper exposes the existing macro-based fixture as a standalone target so
source queries do not lose the declaration:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
cmake --build .build --config Release --target umbra_tso_regional_regular_continuation_restore_catch2
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore$" --output-on-failure
```

The mixed If-Available-to-retained-regular confirmation-cancellation slice is
source-backed at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore_catch2.cpp:15`.
It is a 163-assertion `HLA_EVOKED` case mapped to 50 Lab requirements, 26
canonical 2025 sections, and 32 selected official C++ API surfaces. It
delivers the owner's confirmation, preserves both surviving regional
reflections through the common Flush Queue boundary, then cancels; publisher
ownership remains, stale Confirm Divestiture is rejected, and the regular
reservation emits no acquisition callback:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
cmake --build .build --config Release --target umbra_tso_mixed_confirmation_cancel_catch2
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation after restore$" --output-on-failure
```

This remains bounded development-profile evidence; alternate callback models,
passive/relaxed DDM, remote transport, package/JUnit/protected review,
Requirements-Lab validation, interoperability, and conformance are separate.

The completed timestamped-attribute-order-cohort slice is independently
buildable at
`timestamped_attribute_order_cohort_catch2.cpp:152`. It is an 87-assertion
`HLA_EVOKED` case mapped to five Requirements-Lab candidates and five canonical
2025 sections. It submits timestamp 7 before the timestamp-5 cohort, advances
two constrained recipients first to 5 and then to 7, and verifies each
recipient receives the complete equal-timestamp cohort before the grant while
preserving different-timestamp order and callback metadata. The tie-break
within the equal-timestamp cohort remains intentionally unspecified:

```powershell
python tools/query_rti_work.py focus timestamped-attribute-order-cohort --summary --compact
python tools/query_rti_work.py trace "Embedded timestamped attribute updates preserve different-timestamp order for each constrained recipient" --summary --compact
python tools/query_rti_work.py matrix timestamped-attribute-order-cohort --summary --compact
python tools/query_rti_work.py check --lane timestamped-attribute-order-cohort --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.timestamped_attribute_order_cohort\.catch2\.Embedded timestamped attribute updates preserve different-timestamp order for each constrained recipient$" --output-on-failure
```

This is bounded in-process ordering evidence; alternate advances, simultaneous
transport-arrival ordering, save/restore composition, ownership/resignation,
remote transport, package/JUnit/protected review, Lab validation, and
conformance remain separate.

The completed timestamped-directed-interaction-regulation-reenable-
changed-lookahead slice is independently buildable at
`timestamped_directed_interaction_regulation_reenable_changed_lookahead_catch2.cpp:151`.
It records 57 `HLA_EVOKED` assertions, maps 11 Requirements-Lab candidates to
eight canonical 2025 sections, and runs through the short target
`umbra_tso_directed_reenable_changed_lookahead_catch2`. It queues one
target-qualified reliable timestamped directed interaction at time five under
lookahead one, disables and callback-gated re-enables Time Regulation at
lookahead three, verifies Query Lookahead, and advances the producer to time
two so the directed callback arrives at time five before the constrained
recipient's grant. Target, tag, producer, timestamp/order, transportation,
callback, and terminal retraction metadata remain intact:

```powershell
python tools/query_rti_work.py focus timestamped-directed-interaction-regulation-reenable-changed-lookahead --summary --compact
python tools/query_rti_work.py trace "Embedded queued timestamped directed interaction survives time-regulation disable and re-enable with changed lookahead" --summary --compact
python tools/query_rti_work.py matrix timestamped-directed-interaction-regulation-reenable-changed-lookahead --summary --compact
python tools/query_rti_work.py check --lane timestamped-directed-interaction-regulation-reenable-changed-lookahead --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.timestamped_directed_interaction_regulation_reenable_changed_lookahead\.catch2\.Embedded queued timestamped directed interaction survives time-regulation disable and re-enable with changed lookahead$" --output-on-failure
```

This is bounded in-process directed-TSO evidence; directed DDM breadth,
alternate advances, ownership/resignation, save/restore composition, remote
transport, package/JUnit/protected review, Lab validation, interoperability,
and conformance remain separate.

## Map a test to requirements and standard subsections

Use an exact handle whenever possible:

```powershell
python tools/query_rti_work.py trace "<exact TEST_CASE title>" --summary --compact
python tools/query_rti_work.py matrix "<exact TEST_CASE title>" --summary --compact
python tools/query_rti_work.py matrix <exact-lane> --summary --compact
python tools/query_rti_work.py matrix <family-id> --summary --compact --limit 20
python tools/query_rti_work.py requirement <lab-id-or-clause> --summary --limit 20
python tools/query_rti_work.py section <document-id:clause-id> --summary --limit 20
python tools/query_rti_work.py coverage --lane <exact-lane> --summary --compact
```

`trace` is the strict one-test reverse map. `matrix` is the bounded reverse
map for a test, official C++ API surface, Requirements-Lab id, canonical
2025 subsection, lane, or indexed family. Every row includes source, status,
assertion count, requirement ids, canonical `document_id:clause_id` keys, and
direct `lab_requirement_id -> standard subsection` pairs. Numeric section
shorthands such as `9.5.4` and `clause-9.5.4` are accepted.
Matrix/trace summaries also expose `roadmap_owner` separately from the broader
`roadmap_items` context list, so a generic tag overlap cannot obscure the
family that owns the focused lane.

`requirement` is the quick reverse lookup for a Lab id or subsection. If the
query has no mapped Catch2 row but does match the pinned 2025 corpus, it
returns a bounded requirement-gap record with the canonical subsection,
normative statement, and source path so the next C++ case can be planned
without a Requirements-Lab rescan. An unknown query still returns a non-zero
status.

For the active ownership slice, these exact handles jump straight to the
source-backed cases that establish the §7.2 tag and unownership rules:

```powershell
python tools/query_rti_work.py roadmap negotiated-assumption --summary --compact
python tools/query_rti_work.py focus negotiated-assumption --summary --compact
python tools/query_rti_work.py trace "Embedded Negotiated Attribute Ownership Divestiture forwards its tag to an assumption candidate" --summary --compact
python tools/query_rti_work.py matrix requirement-candidate-content-clauses-07-ownership-management-page-154-l9-2 --summary --compact
python tools/query_rti_work.py check --lane negotiated-assumption --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.negotiated_willing_to_acquire_candidate\.catch2\.Embedded Negotiated Attribute Ownership Divestiture forwards its tag to an assumption candidate$" --output-on-failure
python tools/query_rti_work.py requirement requirement-candidate-content-clauses-07-ownership-management-page-154-l51-16 --summary --compact
python tools/query_rti_work.py requirement requirement-candidate-content-clauses-07-ownership-management-page-154-l117-36 --summary --compact
python tools/query_rti_work.py section hla-1516.1-2025:clause-7.2 --summary --limit 20
```

The regional object region-context boundary is a separate 2025 DDM slice
(34 HLA_EVOKED assertions, five §9.1.3.3 requirements, four C++ APIs):

```powershell
python tools/query_rti_work.py roadmap regional-object-attribute-region-context --summary --compact
python tools/query_rti_work.py focus regional-object-attribute-region-context --summary --compact
python tools/query_rti_work.py trace "Embedded regional object services reject region dimensions outside available object dimensions" --summary --compact
python tools/query_rti_work.py matrix requirement-candidate-content-clauses-09-data-distribution-management-page-220-l8-2 --summary --compact
python tools/query_rti_work.py check --lane regional-object-attribute-region-context --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.regional_object_attribute_routing\.catch2\.Embedded regional object services reject region dimensions outside available object dimensions$" --output-on-failure
```

Its contract is limited to deterministic `InvalidRegionContext` rejection at
regional registration, update association, regional subscription, and
regional Request Attribute Value Update boundaries. Query the valid subset,
realization-property, and delivery lanes separately.

If an older aggregate row and a focused row share one Catch2 title, `trace`
returns both plan ids; pass the returned plan id on the next query when one
evidence scope must be selected exactly.

For a source file or a mapping queue, keep the scope equally narrow:

```powershell
python tools/query_rti_work.py source cpp/tests/<file>.cpp --summary --limit 20
python tools/query_rti_work.py unmapped --disposition unclassified --summary --limit 20
python tools/query_rti_work.py unmapped --disposition unclassified --show-contract-candidates --summary --compact --limit 8
python tools/query_rti_work.py unmapped --lane <exact-lane> --summary --compact
python tools/query_rti_work.py unlocated --family <family-id> --summary --limit 20
python tools/query_rti_work.py unplanned --path cpp/tests/<file>.cpp --summary
```

These commands report existing index state; they never invent requirements or
silently resynchronize the Lab.

For an unmapped C++ case, add `--show-contract-candidates` before editing the
plan. This displays exact Requirements-Lab contract records that already name
the test, including candidate Lab ids and clause ids; it is a bounded discovery
view only and does not assign the mapping. A `[source-mismatch]` marker means
the contract names the exact test title but still points at an older/alternate
translation unit, so review that relocation before mapping. Use `--json` when
the candidate list is longer than the compact preview.

## Query the written plans

The roadmap is queried through `status`, `queue`, `next`, `ready`, `work`, and
`item`. The implementation-plan outline is queried by heading rather than by
loading its prose:

```powershell
python tools/query_rti_work.py plan --summary --compact
python tools/query_rti_work.py plan "current indexed" --summary --compact
python tools/query_rti_work.py plan "service families" --summary --compact
```

The filtered form returns matching heading lines and their breadcrumb paths;
`--limit 0` is the explicit opt-in for every matching heading. Use the printed
line only to open the relevant part of `IMPLEMENTATION-PLAN.md`.

## Integrity and evidence rules

```powershell
python tools/query_rti_work_regression.py
python tools/query_rti_work.py check --summary --compact
python tools/query_rti_work.py recent --summary --compact --limit 10
```

The query regression also guards the three core service handles above, so a
future plan edit that drops a direct mapping, source pointer, or assertion
count fails before work selection drifts.

Run the focused `check --lane` or `check --family` before editing a slice.
Use unscoped `check` only as a deliberate whole-plan reconciliation gate.
`Source index: attention` means a bounded C++ source-reconciliation issue;
inspect the named translation unit with `git diff --check -- <file>` before
changing mappings. Protected-review, package, interoperability, and
conformance promotion remain separate evidence gates.

The preceding attribute-relevance slice is independently runnable from the
focused advisory target:

```powershell
python tools/query_rti_work.py focus attribute-relevance-scope-transition --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories follow scope transitions" --summary --compact
python tools/query_rti_work.py matrix "Embedded attribute relevance advisories follow scope transitions" --summary --compact
python tools/query_rti_work.py check --lane attribute-relevance-scope-transition --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_advisory\.catch2\.Embedded attribute relevance advisories follow scope transitions$" --output-on-failure
```

It records 106 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps nine
Requirements-Lab anchors directly to clauses 6.23, 6.24, and 10.37.1, and is
source-located at `cpp/tests/attribute_relevance_advisory_catch2.cpp:265`.
`ready --summary --compact` now advances past this source-backed row.

The preceding Query Attribute Ownership service-report interaction slice is
independently runnable from the ownership target:

```powershell
python tools/query_rti_work.py focus query-attribute-ownership-service-report-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers Query Attribute Ownership through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix query-attribute-ownership-service-report-interaction --summary --compact
python tools/query_rti_work.py check --lane query-attribute-ownership-service-report-interaction --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.attribute_ownership_query\.catch2\.Embedded service reporting delivers Query Attribute Ownership through MOM interaction$" --output-on-failure
```

It records 67 assertions with an HLA_IMMEDIATE MOM observer and an HLA_EVOKED
ownership-result requester, maps 11 Requirements-Lab anchors directly to
clauses 7.17.5, 7.18.4, 11.5, 11.5.1, 11.5.2, and 11.5.2.1, and is
source-located at `cpp/tests/attribute_ownership_query_catch2.cpp:296`.
`ready --summary --compact` now advances to the next source-unlocated row.

The preceding Cancel Attribute Ownership Acquisition service-report interaction
slice is independently runnable from the ownership-acquisition cancellation
target:

```powershell
python tools/query_rti_work.py focus cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers Cancel Attribute Ownership Acquisition through MOM" --summary --compact
python tools/query_rti_work.py matrix cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
python tools/query_rti_work.py check --lane cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.attribute_ownership_acquisition_cancellation\.catch2\.Embedded service reporting delivers Cancel Attribute Ownership Acquisition through MOM$" --output-on-failure
```

It records 77 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps six
Requirements-Lab anchors directly to clauses 7.15, 7.16, 11.5, 11.5.2, and
11.5.2.1, and is source-located at
`cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp:387`.
`ready --summary --compact` now advances to the next source-unlocated row.

The preceding Cancel Negotiated Attribute Ownership Divestiture service-report
interaction slice remains independently runnable from the negotiated ownership
target:

```powershell
python tools/query_rti_work.py focus cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers Cancel Negotiated Attribute Ownership Divestiture through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
python tools/query_rti_work.py check --lane cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.negotiated_attribute_ownership_divestiture_pending\.catch2\.Embedded service reporting delivers Cancel Negotiated Attribute Ownership Divestiture through MOM interaction$" --output-on-failure
```

It records 87 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps five
Requirements-Lab anchors directly to clauses 7.8, 7.14.6, 11.5, 11.5.2, and
11.5.2.1, and is source-located at
`cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp:547`.
`ready --summary --compact` now advances to the next source-unlocated row.

The preceding service-reporting handoff is the focused subscription interlock:

```powershell
python tools/query_rti_work.py focus service-reporting-interlock --summary --compact
python tools/query_rti_work.py trace "Embedded MOM service-reporting state excludes report-service subscriptions" --summary --compact
python tools/query_rti_work.py matrix service-reporting-interlock --summary --compact
python tools/query_rti_work.py check --lane service-reporting-interlock --summary --compact
cmake --build .build --config Release --target umbra_mom_service_reporting_interlock_catch2
ctest --test-dir .build -C Release -R "^umbra\.mom_service_reporting_interlock\.catch2\.Embedded MOM service-reporting state excludes report-service subscriptions$" --output-on-failure
```

It records 41 `HLA_EVOKED` assertions, maps three Requirements-Lab anchors
directly to clauses 5.10.2, 9.10.3, and 11.5, and is source-located at
`cpp/tests/mom_service_reporting_interlock_catch2.cpp:50`. The exact lane
rejects active and passive ordinary/regional report-service subscriptions while
reporting is enabled, preserves the switch on failed enables, and verifies
removal-before-enable recovery. `ready --summary --compact` now advances to
the next bounded object/DDM MOM row.

The latest source-backed handoff is the joined-federate MOM deletable-object
count lane at `cpp/tests/joined_federate_mom_deletable_object_count_catch2.cpp:108`:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-deletable-object-count --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes deletable object count" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-deletable-object-count --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-deletable-object-count --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_deletable_object_count_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_deletable_object_count\.catch2\.Embedded joined-federate MOM exposes deletable object count$" --output-on-failure
```

It records 55 `HLA_EVOKED` assertions, maps one Requirements-Lab anchor
directly to clause 11.4.1, and verifies the live
`HLAprivilegeToDeleteObject` count 0 → 1 → 1 → 0 through direct MOM request,
periodic `HLAsetTiming` reflection, and owner deletion. `ready --summary
--compact` selects the next exact lane; remaining MOM statistics, regional or
transport variants, filesystem reporting, package/JUnit/protected review,
validation, and conformance remain separate.

The latest source-backed handoff is the joined-federate MOM receive-order queue
length lane at `cpp/tests/joined_federate_mom_ro_length_periodic_catch2.cpp:134`:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-ro-length-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes receive-order queue length" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-ro-length-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-ro-length-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_ro_length_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_ro_length_periodic\.catch2\.Embedded joined-federate MOM exposes receive-order queue length$" --output-on-failure
```

It records 63 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor
directly to clause 11.4.1. The direct MOM request samples `HLAROlength` before
delivery (0 → 1), periodic `HLAsetTiming` reflection preserves the queued value,
and the count returns to 0 after the receive-order callback. The ledger is
recipient-scoped and excludes RTI-owned MOM traffic. Deferred asynchronous/TSO
variants, remaining MOM statistics, regional/transport variants, package/JUnit,
validation, and conformance remain separate.

The latest source-backed handoff is the joined-federate MOM `HLAupdatesSent`
count lane at
`cpp/tests/joined_federate_mom_updates_sent_periodic_catch2.cpp:108`:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-updates-sent-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAupdatesSent count" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-updates-sent-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-updates-sent-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_updates_sent_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_updates_sent_periodic\.catch2\.Embedded joined-federate MOM exposes HLAupdatesSent count$" --output-on-failure
```

It records 56 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor
directly to clause 11.4.1. Direct MOM requests expose the accepted
`Update Attribute Values` invocation count as `HLAinteger32BE` (0 → 1 → 2),
and periodic `HLAsetTiming` reflection preserves 2. The counter is advanced at
the accepted service boundary, not per value or downstream callback. Remaining
MOM statistics, timestamped/update-rate and regional/transport variants,
package/JUnit, validation, and conformance remain separate.

The next source-backed handoff is the joined-federate MOM
`HLAobjectInstancesUpdated` count lane at
`cpp/tests/joined_federate_mom_updated_object_count_periodic_catch2.cpp:109`:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-updated-object-count-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAobjectInstancesUpdated count" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-updated-object-count-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-updated-object-count-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_updated_object_count_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_updated_object_count_periodic\.catch2\.Embedded joined-federate MOM exposes HLAobjectInstancesUpdated count$" --output-on-failure
```

It records 77 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor
directly to clause 11.4.1. Direct MOM requests prove the distinct-object
ledger 0 → 1 → 1 → 2, and periodic `HLAsetTiming` reflection preserves 2
alongside `HLAupdatesSent=3`. The count is per object instance, not per update
invocation, value, or callback; remaining MOM statistics and promotion gates
remain separate.

The next source-backed handoff is the joined-federate MOM
`HLAobjectInstancesRegistered` count lane at
`cpp/tests/joined_federate_mom_registered_object_count_periodic_catch2.cpp:115`:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-registered-object-count-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAobjectInstancesRegistered count" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-registered-object-count-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-registered-object-count-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_registered_object_count_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_registered_object_count_periodic\.catch2\.Embedded joined-federate MOM exposes HLAobjectInstancesRegistered count$" --output-on-failure
```

It records 112 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor
directly to clause 11.4.1. Direct MOM requests prove the successful
registration ledger 0 → 1 → 2, while three accepted updates establish
`HLAupdatesSent=3` and `HLAobjectInstancesUpdated=2`; periodic reflection
preserves all three values. Invalid registration paths and other promotion
gates remain separate.

The `HLAobjectInstancesDeleted` lane is source-backed at
`cpp/tests/joined_federate_mom_deleted_object_count_periodic_catch2.cpp:108`
with 66 `HLA_EVOKED` assertions, one §11.4.1 requirement, and 12 official C++
API surfaces. Query and run it with:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-deleted-object-count-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAobjectInstancesDeleted count" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-deleted-object-count-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-deleted-object-count-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_deleted_object_count_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_deleted_object_count_periodic\.catch2\.Embedded joined-federate MOM exposes HLAobjectInstancesDeleted count$" --output-on-failure
```

The latest receiving-federate `HLAobjectInstancesRemoved` lane is source-backed
at `cpp/tests/joined_federate_mom_removed_object_count_periodic_catch2.cpp:145`
with 78 `HLA_EVOKED` assertions, one §11.4.1 requirement, and 14 official C++
API surfaces. It proves the committed callback ledger 0 → 1 → 2 and periodic
snapshots on the observer's own MOM object:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-removed-object-count-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAobjectInstancesRemoved counts committed callbacks" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-removed-object-count-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-removed-object-count-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_removed_object_count_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_removed_object_count_periodic\.catch2\.Embedded joined-federate MOM HLAobjectInstancesRemoved counts committed callbacks$" --output-on-failure
```

The latest receiving-federate `HLAobjectInstancesDiscovered` lane is
source-backed at
`cpp/tests/joined_federate_mom_discovered_object_count_periodic_catch2.cpp:127`
with 66 `HLA_EVOKED` assertions, one §11.4.1 requirement, and 14 official C++
API surfaces. It proves eligible discovery values 0 → 2 → 3 through ordinary
callbacks, Local Delete Object Instance, and subscription rediscovery:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-discovered-object-count-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAobjectInstancesDiscovered counts eligible callbacks" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-discovered-object-count-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-discovered-object-count-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_discovered_object_count_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_discovered_object_count_periodic\.catch2\.Embedded joined-federate MOM HLAobjectInstancesDiscovered counts eligible callbacks$" --output-on-failure
```

The current source-backed handoff is the official handle-normalization lane at
`cpp/tests/handle_normalization_catch2.cpp:60`. It records 52 `HLA_EVOKED`
assertions, six requirements, six canonical sections (§§10.1.3 and 10.29–10.33),
and five C++ API surfaces. Query and run only this slice with:

```powershell
python tools/query_rti_work.py focus handle-normalization --summary --compact
python tools/query_rti_work.py trace "Embedded handle normalization supplies stable DDM point-range coordinates" --summary --compact
python tools/query_rti_work.py matrix object-ddm-ownership --summary --compact
python tools/query_rti_work.py check --lane handle-normalization --summary --compact
cmake --build .build --config Release --target umbra_handle_normalization_catch2
ctest --test-dir .build -C Release -R "^umbra\.handle_normalization\.catch2\.Embedded handle normalization supplies stable DDM point-range coordinates$" --output-on-failure
```

It proves connection/member fences, typed invalid-designator rejection, stable
same-execution coordinates through two joined ambassadors, the seven-value
`HLAserviceGroup` domain, and post-resign federate-coordinate stability. Broader
DDM routing, save/restore, transport, review, validation, and conformance stay
separate.

The two-dimensional independent-source DDM stress case is also a standalone
queryable lane at
`cpp/tests/regional_multi_attribute_ddm_catch2.cpp:116`. It records 78
`HLA_EVOKED` assertions, maps four Requirements-Lab anchors to §§9.5, 9.6, 9.8,
and 9.9.3, and proves that independent Flavor and Organic source regions are
filtered and restored independently.

```powershell
python tools/query_rti_work.py focus ddm-regional-multi-attribute --summary --compact
python tools/query_rti_work.py trace "Embedded two-dimensional regional object updates filter independent attribute sources" --summary --compact
python tools/query_rti_work.py matrix ddm-regional-multi-attribute --summary --compact
python tools/query_rti_work.py check --lane ddm-regional-multi-attribute --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.regional_multi_attribute_ddm\\.catch2\\.Embedded two-dimensional regional object updates filter independent attribute sources$" --output-on-failure
```

Keep timestamped/retraction, relaxed DDM, transport, package/JUnit, review,
validation, and conformance evidence in separate lanes.

The current MOM request/report handoff is the joined-federate
`HLAobjectInstancesUpdated` report lane at
`cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp:137`.
It records 42 `HLA_EVOKED` assertions, one §11.4.1 requirement, one canonical
section, and 12 official C++ API surfaces. The request is Subscribe-only; the
case groups the accepted update ledger by two registered object classes,
preserves one distinct count across repeated updates, and decodes the nested
`HLAobjectClassBasedCounts` payload from the reliable RTI-originated report.

```powershell
python tools/query_rti_work.py focus joined-federate-mom-object-instances-updated-report --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestObjectInstancesUpdated reports class-grouped counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-object-instances-updated-report --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-object-instances-updated-report --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_object_instances_updated_report_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instances_updated_report\.catch2\.Embedded MOM requestObjectInstancesUpdated reports class-grouped counts$" --output-on-failure
```

Keep HLA_IMMEDIATE, transport/timestamp variants, other MOM request/report
families, and promotion evidence as separate lanes.

The live-ownership companion is independently queryable at
`cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp:137`.
It records 53 `HLA_EVOKED` assertions, one §11.4.1 requirement, one canonical
section, and 12 official C++ API surfaces. It consumes the Subscribe-only
`HLArequestObjectInstancesThatCanBeDeleted` request and verifies nested class
counts from the live `HLAprivilegeToDeleteObject` ledger: two classes before
deletion, then one remaining class after an accepted deletion.

```powershell
python tools/query_rti_work.py focus joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestObjectInstancesThatCanBeDeleted reports live owner counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instances_that_can_be_deleted_report\.catch2\.Embedded MOM requestObjectInstancesThatCanBeDeleted reports live owner counts$" --output-on-failure
```

Keep HLA_IMMEDIATE, transport/timestamp variants, the remaining MOM
request/report families, and promotion evidence as separate lanes.

The paired known-class-enabled Attribute Relevance Advisory lane is independently
queryable at `cpp/tests/attribute_relevance_known_class_enabled_subscription_catch2.cpp:97`.
It records 30 `HLA_EVOKED` assertions, 15 Requirements-Lab anchors, 13
canonical 2025 sections, and 17 official C++ API surfaces. It proves the
initial Employee advisory remains active while the enabled known-class policy
suppresses a later Server-only advisory for a subscriber that has not learned
the Server class, then verifies the Employee Off transition.

```powershell
python tools/query_rti_work.py focus known-class-enabled --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories honor known class when the static policy is enabled" --summary --compact
python tools/query_rti_work.py matrix known-class-enabled --summary --compact
python tools/query_rti_work.py check --lane known-class-enabled --summary --compact
cmake --build .build --config Release --target umbra_attribute_relevance_known_class_enabled_subscription_catch2
ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_known_class_enabled_subscription\.catch2\.Embedded attribute relevance advisories honor known class when the static policy is enabled$" --output-on-failure
```

Keep known-class-disabled, ordinary active-maximum, regional/DDM, process,
review, validation, interoperability, and promotion/conformance separate.

The regional explicit-rate Attribute Relevance Advisory lane is independently
queryable at `cpp/tests/regional_attribute_relevance_rate_designator_catch2.cpp:105`.
It records 100 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps eight
Requirements-Lab anchors to §§6.23/6.24/10.37.1, and proves overlapping source
regions, disjoint suppression, the Off transition on lost overlap, and one
High-rate re-entry after restoring the original overlap.

```powershell
python tools/query_rti_work.py focus regional-attribute-relevance-rate-designator --summary --compact
python tools/query_rti_work.py trace "Embedded regional attribute relevance advisories retain explicit update-rate designators" --summary --compact
python tools/query_rti_work.py matrix regional-attribute-relevance-rate-designator --summary --compact
python tools/query_rti_work.py check --lane regional-attribute-relevance-rate-designator --summary --compact
cmake --build .build --config Release --target umbra_regional_attribute_relevance_rate_designator_catch2
ctest --test-dir .build -C Release -R "^umbra\.regional_attribute_relevance_rate_designator\.catch2\.Embedded regional attribute relevance advisories retain explicit update-rate designators$" --output-on-failure
```

Keep ordinary active-maximum rate reissue, other DDM variants, process
transport, review, validation, interoperability, and conformance as separate
lanes.

The known-class-disabled Attribute Relevance Advisory lane is independently
queryable at `cpp/tests/attribute_relevance_known_class_disabled_subscription_catch2.cpp:97`.
It records 35 `HLA_EVOKED` assertions, 18 Requirements-Lab anchors, 13
canonical 2025 sections, and 18 official C++ API surfaces. It proves discovery
through `Employee`, the Server-only owner-directed On advisory under the
disabled static policy, and the corresponding Off advisory after unsubscription.

```powershell
python tools/query_rti_work.py focus known-class-disabled --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories use subscriptions when known-class policy is disabled" --summary --compact
python tools/query_rti_work.py matrix known-class-disabled --summary --compact
python tools/query_rti_work.py check --lane known-class-disabled --summary --compact
cmake --build .build --config Release --target umbra_attribute_relevance_known_class_disabled_subscription_catch2
ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_known_class_disabled_subscription\.catch2\.Embedded attribute relevance advisories use subscriptions when known-class policy is disabled$" --output-on-failure
```

Keep known-class-enabled, ordinary active-maximum, regional/DDM, process
transport, review, validation, interoperability, and conformance separate.

The passive regional update-rate lookup lane is independently queryable at
`cpp/tests/update_rate_passive_regional_subscription_catch2.cpp:73`. It records
43 `HLA_EVOKED` assertions, 15 Requirements-Lab anchors, 12 canonical 2025
sections, and 21 official C++ API surfaces. It proves that an active Low
regional declaration reports 0.2, a passive High declaration contributes no
second reduction, activating that High declaration reports 30.0, and removing
it restores Low. This is a focused lookup/filtering slice, not update delivery,
advisory reissue, overlap, HLA_IMMEDIATE, package, or conformance evidence.

```powershell
python tools/query_rti_work.py focus update-rate-passive-regional-subscription --summary --compact
python tools/query_rti_work.py trace "Embedded update-rate lookup ignores passive regional subscriptions" --summary --compact
python tools/query_rti_work.py matrix update-rate-passive-regional-subscription --summary --compact
python tools/query_rti_work.py check --lane update-rate-passive-regional-subscription --summary --compact
cmake --build .build --config Release --target umbra_update_rate_passive_regional_subscription_catch2
ctest --test-dir .build -C Release -R "^umbra\.update_rate_passive_regional_subscription\.catch2\.Embedded update-rate lookup ignores passive regional subscriptions$" --output-on-failure
```

Keep update delivery, attribute-relevance, regional-overlap, process transport,
review, validation, interoperability, and conformance as separate lanes.

The mixed ordinary update-rate lane is independently queryable at
`cpp/tests/mixed_update_rate_subscriptions_catch2.cpp:199`. It records 37
`HLA_EVOKED` assertions, eight Requirements-Lab anchors, two canonical 2025
sections, and 15 official C++ API surfaces. It proves per-attribute gating:
the first update delivers both best-effort attributes, while an immediate
second update suppresses only the Low-rate attribute and still delivers the
`HLAdefault`-rate attribute. This is a focused ordinary update-rate slice;
keep unsubscribe removal, regional/passive filtering, timestamped delivery,
MOM, transport, review, validation, interoperability, and conformance as
separate lanes.

```powershell
python tools/query_rti_work.py focus update-rate-mixed-attribute-gating --summary --compact
python tools/query_rti_work.py trace "Embedded mixed update-rate subscriptions gate each attribute independently" --summary --compact
python tools/query_rti_work.py matrix update-rate-mixed-attribute-gating --summary --compact
python tools/query_rti_work.py check --lane update-rate-mixed-attribute-gating --summary --compact
cmake --build .build --config Release --target umbra_mixed_update_rate_subscriptions_catch2
ctest --test-dir .build -C Release -R "^umbra\.mixed_update_rate_subscriptions\.catch2\.Embedded mixed update-rate subscriptions gate each attribute independently$" --output-on-failure
```

The federation-teardown update-rate isolation lane is independently queryable
at `cpp/tests/federation_teardown_update_rate_history_catch2.cpp:158`. It
records 46 `HLA_EVOKED` assertions, nine Requirements-Lab anchors, five
canonical 2025 sections, and 15 official C++ API surfaces. It proves that two
live executions with prefix-related names keep independent Low-rate admission
history: destroying the first execution does not clear the surviving
execution's suppression state. This is a focused per-execution lifecycle
slice; keep mixed per-attribute gating, passive lookup, unsubscribe,
timestamped/regional delivery, MOM, transport, review, validation,
interoperability, and conformance separate.

```powershell
python tools/query_rti_work.py focus update-rate-federation-teardown-isolation --summary --compact
python tools/query_rti_work.py trace "Embedded federation teardown preserves update-rate history for another live federation" --summary --compact
python tools/query_rti_work.py matrix update-rate-federation-teardown-isolation --summary --compact
python tools/query_rti_work.py check --lane update-rate-federation-teardown-isolation --summary --compact
cmake --build .build --config Release --target umbra_federation_teardown_update_rate_history_catch2
ctest --test-dir .build -C Release -R "^umbra\.federation_teardown_update_rate_history\.catch2\.Embedded federation teardown preserves update-rate history for another live federation$" --output-on-failure
```

The active timestamped default-region restore lane is independently queryable
by the aggregate 2025 Catch2 title. It records 123 `HLA_EVOKED` assertions,
18 Requirements-Lab anchors, 14 canonical 2025 sections, and 24 official C++
API surfaces. Three joined federates save one queued default-source timestamped
attribute update, restore independent recipient queue/retraction state, flush
each recipient separately, and receive one legal Request Retraction callback.

```powershell
python tools/query_rti_work.py focus timestamped-default-region-attribute-restore-multi-recipient --summary --compact
python tools/query_rti_work.py trace "Embedded federation restore restores one queued timestamped default-region attribute update to multiple recipients" --summary --compact
python tools/query_rti_work.py matrix timestamped-default-region-attribute-restore-multi-recipient --summary --compact
python tools/query_rti_work.py check --lane timestamped-default-region-attribute-restore-multi-recipient --summary --compact
cmake --build .build --config Release --target umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2
ctest --test-dir .build -C Release -R "^umbra\.restore_live_tso_default_region_attribute_update_multi_recipient\.catch2\.Embedded federation restore restores one queued timestamped default-region attribute update to multiple recipients$" --output-on-failure
```

The active timed explicit-source regional-interaction restore lane is
independently runnable at
`cpp/tests/timed_restore_live_tso_regional_interaction_catch2.cpp:5`. It records
55 `HLA_EVOKED` assertions, 18 Requirements-Lab anchors, 15 canonical 2025
sections, and 24 official C++ API surfaces. Two joined federates save a queued
timestamp-8 `Send Interaction With Regions` at logical time 6, restore the
source-region designator and retraction ledger, then deliver it before the
Flush Queue grant and receive one legal Request Retraction callback.

```powershell
python tools/query_rti_work.py focus timestamped-regional-interaction-timed-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed federation restore restores a live timestamped regional interaction at the save boundary" --summary --compact
python tools/query_rti_work.py matrix timestamped-regional-interaction-timed-restore --summary --compact
python tools/query_rti_work.py check --lane timestamped-regional-interaction-timed-restore --summary --compact
cmake --build .build --config Release --target umbra_timed_restore_live_tso_regional_interaction_catch2
ctest --test-dir .build -C Release -R "^umbra\.timed_restore_live_tso_regional_interaction\.catch2\.Embedded timed federation restore restores a live timestamped regional interaction at the save boundary$" --output-on-failure
```

The active timestamped regional-interaction source-resignation lane is
independently runnable at
`cpp/tests/timed_live_tso_regional_interaction_source_resignation_catch2.cpp:14`.
It records 60 `HLA_EVOKED` assertions, 14 Requirements-Lab anchors, 14
canonical 2025 sections, and 27 official C++ API surfaces. The producer's
timestamp-6 explicit-source passel is admitted before resignation, then an
independent regulator releases the recipient's queued delivery and the
post-resignation retraction classification is checked.

```powershell
python tools/query_rti_work.py focus timestamped-regional-interaction-source-resignation --summary --compact
python tools/query_rti_work.py trace "Embedded queued timestamped regional interaction survives source resignation" --summary --compact
python tools/query_rti_work.py matrix timestamped-regional-interaction-source-resignation --summary --compact
python tools/query_rti_work.py check --lane timestamped-regional-interaction-source-resignation --summary --compact
cmake --build .build --config Release --target umbra_timed_live_tso_regional_interaction_source_resignation_catch2
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_interaction_source_resignation\.catch2\.Embedded queued timestamped regional interaction survives source resignation$" --output-on-failure
```

The active m86 timestamped regional-interaction restore fan-out lane is
independently runnable at
`cpp/tests/restore_live_tso_regional_interaction_multi_recipient_catch2.cpp:169`.
It records 128 `HLA_EVOKED` assertions, 11 Requirements-Lab anchors, 11
canonical 2025 sections, and 35 official C++ API surfaces. The case saves one
queued explicit-source timestamped interaction for two independently
constrained regional recipients, restores both queue entries and their
retraction ledger, and verifies each copy before its own Flush Queue grant
with one legal Request Retraction callback.

```powershell
python tools/query_rti_work.py focus timestamped-regional-interaction-restore-multi-recipient --summary --compact
python tools/query_rti_work.py trace "Embedded federation restore restores one queued timestamped regional interaction to multiple recipients" --summary --compact
python tools/query_rti_work.py matrix timestamped-regional-interaction-restore-multi-recipient --summary --compact
python tools/query_rti_work.py check --lane timestamped-regional-interaction-restore-multi-recipient --summary --compact
cmake --build .build --config Release --target umbra_restore_live_tso_regional_interaction_multi_recipient_catch2
ctest --test-dir .build -C Release -R "^umbra\.restore_live_tso_regional_interaction_multi_recipient\.catch2\.Embedded federation restore restores one queued timestamped regional interaction to multiple recipients$" --output-on-failure
```

The active m87 timed regional-attribute source-resignation-after-restore lane
is independently runnable at
`cpp/tests/timed_live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp:180`.
It records 106 `HLA_EVOKED` assertions, 27 Requirements-Lab anchors, 18
canonical 2025 sections, and 23 official C++ API surfaces. The case saves an
overlap-qualified timestamp-8 explicit-source update at logical time 6,
mutates the source region before and after restore, resigns the producer with
unconditional divestiture, and verifies the surviving receiver's restored
source-region metadata and callback ordering.

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-live-resignation-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-live-resignation-state --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-live-resignation-state --summary --compact
cmake --build .build --config Release --target umbra_timed_regional_attr_source_resign_restore_catch2
ctest --test-dir .build -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_source_resignation_after_restore\.catch2\.Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore$" --output-on-failure
```

The active m88 timed explicit-source regional-attribute restore lane is
independently runnable at
`cpp/tests/timed_restore_live_tso_regional_attribute_update_catch2.cpp:190`.
It records 84 `HLA_EVOKED` assertions, 23 Requirements-Lab anchors, 15
canonical 2025 sections, and 23 official C++ API surfaces. The case saves at
logical time 6 while an overlap-qualified timestamp-8 update remains queued,
restores the source-region association, recipient ledger, and retraction
identity, and proves Flush Queue reflection at actual time 7 with optimistic
time 8.

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-live-restore-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-live-restore-state --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-live-restore-state --summary --compact
cmake --build .build --config Release --target umbra_timed_restore_regional_attr_catch2
ctest --test-dir .build -C Release -R "^umbra\.timed_restore_live_tso_regional_attribute_update\.catch2\.Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary$" --output-on-failure
```

The next source-backed action is always emitted by `ready --summary --compact`
and `unplanned --summary --compact --limit 1`; do not infer it from this card.

The active-maximum Attribute Relevance Advisory lane is independently runnable
at `cpp/tests/attribute_relevance_rate_reissue_catch2.cpp:108`. It records 108
assertions under both callback models, maps seven Requirements-Lab anchors to
§§6.23/6.24, and proves multi-federate High/Medium/Low maximum selection,
lower-peer suppression, rate-bearing reissue, Low refresh after higher-rate
removal, and final Turn Updates Off delivery:

```powershell
python tools/query_rti_work.py focus attribute-relevance-rate-reissue --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories reissue turn-on when the active update rate changes" --summary --compact
python tools/query_rti_work.py matrix attribute-relevance-rate-reissue --summary --compact
python tools/query_rti_work.py check --lane attribute-relevance-rate-reissue --summary --compact
cmake --build .build --config Release --target umbra_attribute_relevance_rate_reissue_catch2
ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_rate_reissue\.catch2\.Embedded attribute relevance advisories reissue turn-on when the active update rate changes$" --output-on-failure
```

Keep regional/DDM variants, process transport, and promotion evidence as
separate lanes.

The latest known/NULL object-information MOM slice is independently queryable
at `cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp:146`.
It records 71 HLA_EVOKED assertions, one §11.4.1 requirement, one canonical
section, and 13 official C++ API surfaces. It consumes the Subscribe-only
`HLArequestObjectInstanceInformation`, decodes the nested
`HLAattributeHandleList`, proves registered/known classes plus the registering
federate's owned attributes, and uses Local Delete Object Instance to verify
the MIM NULL response shape.

```powershell
python tools/query_rti_work.py focus joined-federate-mom-object-instance-information-report --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestObjectInstanceInformation reports known and NULL object state" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-object-instance-information-report --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-object-instance-information-report --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_object_instance_information_report_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instance_information_report\.catch2\.Embedded MOM requestObjectInstanceInformation reports known and NULL object state$" --output-on-failure
```

Keep HLA_IMMEDIATE, transport variants, the remaining MOM request/report
families, and promotion evidence as separate lanes.

The latest MOM request/report slice is independently queryable at
`cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp:160`.
It records 51 HLA_EVOKED assertions, one §11.4.1 requirement, one canonical
section, and 14 official C++ API surfaces. It consumes the Subscribe-only
`HLArequestObjectInstancesReflected` request for the requesting/receiving
federate, counts accepted application reflections by registered class, keeps
repeated reflections of one object at one distinct count, and decodes the
nested `HLAobjectClassBasedCounts` report.

```powershell
python tools/query_rti_work.py focus joined-federate-mom-object-instances-reflected-report --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestObjectInstancesReflected reports distinct reflected instances" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-object-instances-reflected-report --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-object-instances-reflected-report --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_object_instances_reflected_report_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instances_reflected_report\.catch2\.Embedded MOM requestObjectInstancesReflected reports distinct reflected instances$" --output-on-failure
```

Keep HLA_IMMEDIATE, transport/timestamp variants, the remaining MOM
request/report families, and promotion evidence as separate lanes.
