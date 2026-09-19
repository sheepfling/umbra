# Roadmap and test traceability query guide

This is the detailed reference. Start with the small
[QUERY-CARD.md](QUERY-CARD.md) so a normal resume does not load the full
example catalog.

Use this page as the detailed reference for selecting work. The checked-in
roadmap index, Catch2 plan, pinned 2025 corpus, and derived C++ source
locations are joined by `tools/query_rti_work.py`. The command is read-only;
it does not rescan or rewrite the Requirements Lab. Once `next` identifies a
slice, query that exact lane or test before opening source; a repository-wide
search is not part of normal work selection.

## Start here

Run these in order from the repository root:

```powershell
python tools/query_rti_work.py resume
python tools/query_rti_work.py lab-issues --summary --compact
python tools/query_rti_work.py status --summary --compact
python tools/query_rti_work.py ready --summary --compact
python tools/query_rti_work.py work
python tools/query_rti_work.py queue --summary --compact
python tools/query_rti_work.py focus --summary --compact
python tools/query_rti_work.py recent --summary --compact --limit 10
python tools/query_rti_work.py check --lane process-boundary --compact
python tools/query_rti_work.py trace "RTIambassador removes a regional subscription through a configured process endpoint" --summary
python tools/query_rti_work.py test "Private process transport exchanges framed data after endpoint handshake" --summary --compact
python tools/query_rti_work.py unplanned --path libxml2_fom_composer_catch2.cpp --summary
python tools/query_rti_work.py check --compact
python tools/query_rti_work.py check --json
```

A preceding process Flush Queue slice remains intentionally one-command
queryable. It is a 110-assertion native C++ case mapped to 22 pinned 2025
Requirements-Lab requirements, 12 canonical `document:clause` subsections,
and 22 official C++ API surfaces. It sends timestamp-5, timestamp-8, and
exact requested-frontier timestamp-10 interactions, plus future timestamps 12
and 14; it retracts timestamp 14 before the first frontier, verifies FIFO
delivery of the admitted records before the HLA_EVOKED or HLA_IMMEDIATE Flush
Queue Grant, and
delivers retained timestamp 12 on the later frontier:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-flush-queue-multiple-records-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver multiple queued timestamped process messages in order through Flush Queue" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-flush-queue-multiple-records-integration --summary --compact
python tools/query_rti_work.py focus process-flush-queue-multiple-records --summary --compact
python tools/query_rti_work.py check --lane process-flush-queue-multiple-records --summary --compact
python tools/query_rti_work.py focus process-flush-queue-retraction --summary --compact
python tools/query_rti_work.py check --lane process-flush-queue-retraction --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver multiple queued timestamped process messages in order through Flush Queue$" --output-on-failure
```

The companion process GALT-frontier case is separately indexed under
`process-flush-queue-galt-frontier`; do not combine its actual/optimistic
grant assertions with the FIFO lane when selecting the next work slice.

`resume` is the smallest bounded resume card. It combines the live roadmap
checklist, Catch2 plan totals, requirement/subsection mapping queues, source
health, index-snapshot freshness, latest completed slice, and one active work
handoff or family choice. Use `resume --json` for a script. The richer
`dashboard --summary --compact` card retains a three-row open-family preview
for diagnostics. Both views intentionally avoid enumerating every plan row or
reopening the unchanged Requirements Lab.

`lab-issues` is a separate, cheap issue lookup. It reads only
`compliance/requirements-lab/known-issues.json`, so known extraction defects
and recurrences are visible without a Lab resync or source scan. Use an exact
issue id, requirement id, or clause to review one bounded complaint (for
example `lab-issues RL-177 --summary --compact`).

If both executable queues are exhausted, the dashboard's `next` card first
uses the indexed `active_handoff` when one is deliberately queued; otherwise
it is a family-selection card rather than a stale completed pointer. An active
handoff names one proposed C++ test, its target source file, acceptance
criteria, and a mapping seed. The seed's requirement/canonical-section join
is resolved from the checked-in plan, and the new row must be remapped before
evidence is claimed. Without an active handoff, bounded family options include
requirement and canonical 2025-section counts, with `ready --family <id>` and
`work <id>` handles. In both modes the next implementation choice and its
traceability context are available without a broad plan search.
The proposed lane is marked pending until its plan row exists; the card emits
`post_mapping_focus_command` and `post_mapping_check_command` for use after
that row is added.

`requirement <id-or-text>` and `section <document:clause>` remain useful when
the reverse lookup has no mapped C++ row yet: each falls back to the bounded
uncovered pinned-2025 records and normative source statements. That gives a
new Catch2 case a precise standard anchor without a second corpus search.
For mapped rows, add `--summary` to use the compact matrix renderer; it keeps
the direct `requirement -> standard subsection` pairs, source, and assertion
counts while avoiding repeated contract detail. Use `test` or `case` for the
full per-case record.

```powershell
python tools/query_rti_work.py section 8.18.1 --summary --compact --limit 8
python tools/query_rti_work.py requirement requirement-candidate-content-clauses-08-time-management-page-206-l149-37 --summary --compact --limit 8
```

For a `new-case-needed` family, the same response includes a bounded
`gap_preview` (text: `gap_scope` and `gap_head`) with one uncovered pinned-2025
record in canonical corpus order, its exact requirement and section lookup
commands, and the family coverage counts. Treat this as a coverage pointer,
not semantic prioritization; use the emitted `gaps --family <id>` command for
the remaining inventory.

The dashboard also exposes the first source-only reconciliation declaration as
`source_only_reconciliation`, including its source location and a bounded
`ready_command`; use that opt-in only when reconciling an existing C++ case.

Each queue row also carries an `action_state`: `implementation`, `mapping`,
`source-reconciliation`, `external-review`, `new-case-needed`, or
`evidence-complete`. The first five are deliberate queued actions;
`evidence-complete` means the family has no runnable indexed work and is not
recommended by `ready`. Diagnostic source drift is split into total
`source_drift` and `actionable_source_drift`, so disabled/reconciled historical
rows stay visible without blocking the handoff. JSON consumers can use the
queue's aggregate `action_counts` and family totals without enumerating rows.

`check --lane <tag>` and `check --family <id>` are intentionally scoped
integrity gates: they validate only the selected owner and its live Catch2
rows. Run the default `check` without a selector for the current live plan;
use `check --historical` only when auditing the append-only completion ledger,
which may retain stale legacy plan/tag diagnostics by design.
The live gate also rejects repeated requirement, API, section, or tag selectors
inside a single plan row, keeping test-to-standard joins deterministic.
It additionally verifies that every selected requirement resolves to its
canonical 2025 `document:clause` subsection in the derived test mapping, so a
missing requirement-to-subsection join fails locally before implementation
work proceeds.

After a source split or focused-test refresh, run the local selector guard
before opening a contract. It compares native contract references with the
current C++ path/title declarations, classifies portable TCK symbols as
external, and never reopens or resynchronizes the Requirements Lab:

```powershell
python tools/query_rti_work.py contract-drift --summary --compact
python tools/query_rti_work.py contract-drift timestamped-directed-interaction --summary --compact
```

Zero findings means the contract-to-Catch2 selectors are exact; a finding gives
the stale contract owner and the current source locations needed for a bounded
repair.

The offline query regression treats the C++ file path and exact test title as
the stable source identity. Line numbers are emitted live by the query tool and
may move when a neighboring `TEST_CASE` is inserted; such a line shift does
not invalidate the requirement/section mapping. A missing declaration or a
changed source path still fails the regression.

The portable TCK contract/catalog boundary is a separate CTest entry
(`umbra.cpp_tck.catalog_traceability`); keep those contracts in the C++ TCK
catalog rather than registering them as Requirements-Lab mappings.

Use `roadmap` when the family is known only by a word or theme. It searches the
indexed family id/title/tags/anchors/next-action fields and joins live Catch2
counts without printing the roadmap prose:

```powershell
python tools/query_rti_work.py roadmap --summary --compact
python tools/query_rti_work.py roadmap process --summary --compact --limit 8
python tools/query_rti_work.py roadmap "save restore" --status all --summary --compact
```

The default scope is the eight open families. Each row includes the exact
`work`, `focus`, and family `matrix` handles plus the next-test requirement and
canonical 2025 subsection previews. Focused lanes may use the index's compact
`focused_lane_tags` aliases, which keeps exact lane discovery separate from
the large family tag list. `--status all` is opt-in so a normal resume cannot
expand into historical completed families.
For the bounded family-owned lane list, use `lanes --family <id> --focused`;
the default `lanes` view intentionally retains broad cross-cutting tags for
taxonomy review.
When the query exactly matches an indexed family id, that id is preferred over
overlapping tags or prose fields; for example, `roadmap transport-and-conformance`
returns one family row even when another family carries the same cross-cutting
tag.
Family membership is resolved consistently across the family card, bounded
`gaps --family`, and reverse test links: broad family tags are joined with
exact indexed lane-owner and `next_lane` handles. This prevents requirement or
case totals from disagreeing between the summary and its gap preview.

When the query is an exact lane handle (normally a Catch2 lane tag; the plan's
`primary_lane`/`focus_lane` aliases are accepted too), the family row also
carries a `lane_match` with its bounded test/mapping counts, representative
source case, representative `trace` command, and focused CTest command. This
makes a lane lookup one command even when the lane itself belongs to a broad
family:

```powershell
python tools/query_rti_work.py roadmap joined-federate-mom-deleted-object-count-periodic --summary --compact
```

The same family query accepts an exact Requirements-Lab id, canonical
`document:clause` subsection, or official C++ API surface. If a family owns a
queued source declaration, the row also prints its `next_source` state,
location, and lane; use `ready --summary --compact` for the single active
handoff rather than opening the aggregate translation unit.

Trace and matrix JSON/text summaries expose `roadmap_owner` separately from
the broader `roadmap_items` context list. Generic tags such as `callbacks` can
therefore remain useful cross-family context without making lane ownership
ambiguous.

When the exact plan id or full Catch2 title is already known, use `case` for a
single bounded handoff. It combines the source line, owner, API surfaces,
direct requirement-to-2025-subsection pairs, and focused execution commands;
it never falls back to fuzzy matching:

```powershell
python tools/query_rti_work.py case <exact-plan-id-or-test-title> --summary --compact
```

Use `test` or `search` only to discover an exact handle, then switch to `case`
for implementation and review. This keeps a common one-case lookup from
requiring separate plan, trace, matrix, and CTest searches.

The current indexed snapshot is 1,285 Catch2 cases (1,221 mapped), including
155 process-boundary cases and 6,717 indexed assertions; 6,869 assertions are
recorded by the plan rows; no mapped row is
currently planned, 64 rows are explicit no-standalone-surface dispositions,
and zero mappings are unclassified. The latest completed 2025 slice is the
HLA_IMMEDIATE restored ownership-assumption work-item lane; retrieve its full
case/trace/matrix/check card with:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --summary --compact
python tools/query_rti_work.py focus process-federation-restore-work-item-ownership-assumption-immediate --summary --compact
python tools/query_rti_work.py trace umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-work-item-ownership-assumption-immediate --summary --compact
```

The other completed 2025 slices are the
durable HLA_EVOKED pushed ownership-assumption save/restore lane, its no-save
HLA_EVOKED companion, the public pushed process restore work-item lane, and the
configured-
process timestamped regional, regional, interaction, and instance
transportation lanes. These other lanes are independently queryable by their
exact case id, focus lane, test title, matrix, and CTest filter:

A separate bounded transport slice is the ordinary two-member HLA_IMMEDIATE
configured `DELETE_OBJECTS` companion:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-delete-objects-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-delete-objects-immediate --summary --compact
python tools/query_rti_work.py trace umbra-cpp-connection-lost-automatic-delete-objects-immediate --summary --compact
python tools/query_rti_work.py matrix connection-lost-automatic-delete-objects-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-delete-objects-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Immediate callbacks apply the configured automatic delete-objects directive synchronously$" --output-on-failure
```

This slice is 25 assertions, five requirements, five canonical sections, and
three official C++ API surfaces. The preceding mixed cancel-delete-divest
companion remains separately queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py trace umbra-cpp-connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py matrix connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Immediate callbacks apply the configured automatic cancel-then-delete-then-divest directive synchronously$" --output-on-failure
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

The earlier pending-acquisition cancellation companion remains separately
queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py trace umbra-cpp-connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py matrix connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.connection_loss_automatic_cancel_pending_acquisition\.catch2\.Immediate callbacks cancel a lost federate's pending ownership acquisition synchronously$" --output-on-failure
```

The preceding automatic-divestiture companion remains independently queryable
through its exact case and lane handles.

The no-save HLA_EVOKED fence remains independently queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-pushed-ownership-assumption-evoked-integration --summary --compact
python tools/query_rti_work.py focus process-pushed-ownership-assumption-evoked --summary --compact
python tools/query_rti_work.py matrix process-pushed-ownership-assumption-evoked --summary --compact
python tools/query_rti_work.py check --lane process-pushed-ownership-assumption-evoked --summary --compact
```

The already-pushed restore lane remains independently queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-push-integration --summary --compact
python tools/query_rti_work.py focus process-federation-restore-work-item-ownership-assumption-push --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve pushed ownership-assumption delivery across save and restore" --summary --compact
python tools/query_rti_work.py matrix process-federation-restore-work-item-ownership-assumption-push --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-work-item-ownership-assumption-push --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors preserve pushed ownership-assumption delivery across save and restore$" --output-on-failure
```

The embedded ownership-assumption continuation family is now split into three
small, independently runnable lanes. Each lane has an exact source pointer,
direct requirement-to-2025-section pairs, and no inherited process/package or
conformance claim:

```powershell
python tools/query_rti_work.py case umbra-cpp-resign-action-assumption-discovery-continuation-integration --summary --compact
python tools/query_rti_work.py focus resign-action-assumption-discovery-continuation --summary --compact
python tools/query_rti_work.py trace umbra-cpp-resign-action-assumption-discovery-continuation-integration --summary --compact
python tools/query_rti_work.py matrix resign-action-assumption-discovery-continuation --summary --compact
python tools/query_rti_work.py check --lane resign-action-assumption-discovery-continuation --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded assumption search continues after a later join and discovery$" --output-on-failure

python tools/query_rti_work.py case umbra-cpp-ownership-assumption-search-continuation-integration --summary --compact
python tools/query_rti_work.py focus ownership-assumption-search-continuation --summary --compact
python tools/query_rti_work.py trace umbra-cpp-ownership-assumption-search-continuation-integration --summary --compact
python tools/query_rti_work.py matrix ownership-assumption-search-continuation --summary --compact
python tools/query_rti_work.py check --lane ownership-assumption-search-continuation --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded ownership assumption search advances after a declined callback$" --output-on-failure

python tools/query_rti_work.py case umbra-cpp-ownership-assumption-search-epoch-integration --summary --compact
python tools/query_rti_work.py focus ownership-assumption-search-epoch --summary --compact
python tools/query_rti_work.py trace umbra-cpp-ownership-assumption-search-epoch-integration --summary --compact
python tools/query_rti_work.py matrix ownership-assumption-search-epoch --summary --compact
python tools/query_rti_work.py check --lane ownership-assumption-search-epoch --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded ownership assumption search starts a fresh epoch after transfer$" --output-on-failure
```

The three cases are 26, 36, and 39 HLA_EVOKED assertions respectively. The
first is the §4.12/§4.12.4 later-join/discovery trigger; the other two are the
§7/§7.2 callback-return continuation and fresh-unowned-epoch boundaries.

The RTI-owned MOM ownership-query baseline has its own exact lane. It is
embedded-only evidence; the process endpoint's separate MOM-establishment and
filesystem-report lane remains open:

```powershell
python tools/query_rti_work.py case umbra-cpp-rti-owned-mom-ownership-query-integration --summary --compact
python tools/query_rti_work.py focus rti-owned-mom-ownership-query --summary --compact
python tools/query_rti_work.py trace umbra-cpp-rti-owned-mom-ownership-query-integration --summary --compact
python tools/query_rti_work.py matrix rti-owned-mom-ownership-query --summary --compact
python tools/query_rti_work.py check --lane rti-owned-mom-ownership-query --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded RTI-owned MOM attributes participate in ownership queries$" --output-on-failure
```

The transportation slices remain
independently queryable by their exact case id, focus lane, test title, matrix,
and CTest filter:

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
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-regional-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-regional-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve a regional interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-regional-interaction-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-regional-interaction-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassadors preserve a regional interaction transportation override through a configured process endpoint$" --output-on-failure
```

The process directed transportation query/report slice is independently
queryable as `process-directed-interaction-transportation-query` at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27231`. It carries 44
assertions under both callback models, 17 direct Lab requirements, ten
canonical 2025 sections, and ten official C++ API surfaces:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py focus process-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py trace "RTIambassador reports a directed interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py check --lane process-directed-interaction-transportation-query --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador reports a directed interaction transportation override through a configured process endpoint$" --output-on-failure
```

The newest timestamped regional transportation companion is at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27745`: 122 assertions, 27
direct Lab requirements, 15 canonical 2025 sections, and 21 official C++ API
surfaces. It proves the confirmed HLAbestEffort override, timestamp and
retraction metadata, callback-before-grant ordering, and conveyed source-region
metadata across a regional Send Interaction With Regions under both callback
models:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-timestamped-regional-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-timestamped-regional-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve a timestamped regional interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-timestamped-regional-interaction-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-timestamped-regional-interaction-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassadors preserve a timestamped regional interaction transportation override through a configured process endpoint$" --output-on-failure
```

The interaction row is at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27030`: 40 assertions, ten
direct Lab requirements, four canonical 2025 sections (6.30.3, 6.31.3, 6.32.5,
and 6.33.6), and nine official C++ API surfaces. It proves the public process
endpoint routes both interaction transportation callbacks under HLA_EVOKED and
HLA_IMMEDIATE. It remains private foundation evidence, not a conformance claim.

The regional interaction transportation companion is at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27247`: 84 assertions, 23
direct Lab requirements, 11 canonical 2025 sections, and 17 official C++ API
surfaces. It proves a confirmed HLAbestEffort override survives a regional
send and that the receiver retains source-region metadata under both callback
models. It remains private foundation evidence, not a conformance claim.

The instance row is at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:26822`: 30 assertions, five
direct Lab requirements, four canonical 2025 sections, and 10 official C++ API
surfaces. It proves one registered instance can change an attribute's
transportation type and query the current type through the same process
endpoint under both callback models. It remains private foundation evidence,
not a conformance claim. The preceding
directed row at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:15757` carries 62 assertions,
21 direct Lab requirements, 16 canonical 2025 sections, and 18 official C++ API
surfaces; it remains separately queryable. The preceding
configured-process two-receiver timestamped interaction fan-out lane remains
independently queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-interaction-fanout-integration --summary --compact
python tools/query_rti_work.py focus process-tso-interaction-fanout --summary --compact
python tools/query_rti_work.py trace "RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-interaction-fanout-integration --summary --compact
python tools/query_rti_work.py check --lane process-tso-interaction-fanout --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant$" --output-on-failure
```

The row is at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:24629`: 133 assertions, 20 direct
Lab requirements, 13 mapped 2025 sections, and 15 official C++ API surfaces.
Its `primary_lane` is `process-tso-interaction-fanout`, so case/trace output
names the owning `process-boundary` family while retaining interaction-
management, time-management, transport, FIFO, fan-out, distinct-timestamp,
and GALT/LITS tags as cross-cutting context. The preceding same-timestamp multiple-message FIFO
case remains independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:24182`: 51 assertions, 20
requirements, 13 sections, and 15 APIs. The preceding configured-process
changed-lookahead
interaction row remains independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:23702`: 67 assertions, 28
requirements, 15 sections, and 18 APIs. The preceding timestamped-attribute row
remains independently queryable at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:23195`.
The preceding configured-process retraction-lifetime case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:22808` remains separately
queryable with 45 assertions, five requirements, four sections, and 15 APIs.
The preceding regional,
fan-out, suppression, and single-receiver ordering cases remain separately
queryable. The regional TSO callback-gating row is now complete at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:26210` with 69 assertions; use
`case`, `trace`, `matrix`, `focus`, and `check --lane` on its exact id/title to
inspect the 28 Lab requirements, 16 canonical 2025 sections, 20 official C++
API surfaces, and CTest filter. `ready --summary --compact` now returns
bounded family choices for the next slice without reopening the unchanged
Requirements Lab.

`work` is the one-screen work selector: it follows the highest-priority open
roadmap item's `next_work_id`, then prints the concrete work item, task,
exact lane/CTest filter, bounded lane state, package target/test/label handles,
one baseline test with its source and requirement/section handles, and
copy/paste query commands (including a direct `trace` command for the
baseline). `lane_state` makes a completed pointer explicit: when it is
`complete` with zero executable candidates, the next step is a newly indexed
C++ case or another indexed family, not a rescan. Pass an item id to inspect a
different indexed slice (`work transport-and-conformance`). `next --summary`
is the compact compatibility alias for the authoritative `ready` handoff: it
selects one executable/planned row or bounded family choice and never presents
a completed historical pointer as new work. Use `next --pointer` when the
parent roadmap item's source-pointer fields are specifically needed, and use
`next --json` for the machine-readable ready handoff.

If a broad family has no single `next_lane`, `work <family-id>` still emits
bounded `lanes --family ...` and `ready --family ...` commands so choosing a
new case never requires a repository-wide search.

`ready --summary --compact` is the direct handoff selector. It resolves one
deterministic planned-row head or the indexed active new-case handoff and
prints the exact test title, source target, requirements, canonical 2025
sections/subsections, and API surface count before any source inspection.
Source-only declarations are not promoted by
default because they may belong to an external or compatibility corpus; use
`ready --include-source-only --summary --compact` or
`next --include-source-only --summary --compact` only when deliberately
reconciling that queue. Use its `trace_command` to view
the one-to-one mapping and its `implementation_command` to enter the bounded
queue. The output labels the queued roadmap family as `roadmap_owner` and the
broader pointer that selected it as `active_pointer` when those differ, so a
cross-family handoff is not mistaken for contradictory ownership. A planned
row is a deliberate contract, not executable evidence, until
its C++ `TEST_CASE` declaration exists. When both indexed queues are exhausted,
and no active handoff is queued, the command prints up to three bounded
open-family options with exact `work`
commands rather than forcing a broad search. Each option also carries a
copyable `lane_discovery` command for the family-scoped unmapped lane index,
so choosing the next exact Catch2 tag does not require reopening the plan. It
also prints `mapping_lane_discovery` for the unclassified-only mapping queue;
intentional dispositions can be reviewed separately with the matching
`--disposition explicit` command.
After choosing a family from `queue`, pass `--family <id>` to keep the ready
response scoped to that family. This is useful when the global source and
planned queues are exhausted: the command reports only the selected family’s
state and next action, without reopening the full plan.
If that selected family still has an unmapped, source-located plan row, the
same command returns it as `state=mapping` with its exact source line, plan id,
and trace/focus/check handles. This makes the mapping backlog a direct
handoff rather than a second lane-inventory search.

`plan [<heading-query>]` is the bounded implementation-plan index. With no
query it prints the heading outline; with a query it matches heading titles or
derived breadcrumb paths only and returns line numbers plus paths. It never
prints the plan prose. Use `--limit 0` only when every matching heading is
needed:

```powershell
python tools/query_rti_work.py plan --summary --compact
python tools/query_rti_work.py plan "current indexed" --summary --compact
python tools/query_rti_work.py plan "service families" --summary --compact
```

This keeps the written plan queryable without another repository-wide search;
open the reported line only after the exact slice is selected.

`status --summary --compact` is deliberately shorter than the default status
view: it prints the live queue counts, one row per open roadmap family, and
only the exact work/lane/source/test handles. Use plain `status` or
`status --json` when the family prose and full metadata are needed.
Each compact family row includes plan-derived mapping counts: mapped cases,
explicit no-standalone-surface dispositions, still-unclassified cases,
source-unlocated cases, distinct canonical 2025 sections, and official C++ API
surfaces, plus deduplicated direct requirement-to-2025-subsection pair totals.
If it carries a baseline test pointer, the row also includes that
pointer's derived tuple (`requirements`, `standard_sections`, `api_surfaces`)
and a copyable exact `trace` command. These values come from the Catch2 plan,
so a resume can identify the scope of the family and baseline without a second
repository-wide lookup; use `trace` for the individual requirement statements.
The row's collapsed `next` line is the current implementation handoff, even
when the family has no executable candidate; it is intentionally separate from
the historical roadmap narrative.
The status header also splits unmapped cases into explicit dispositions and
unclassified rows, keeping intentional no-standalone-surface decisions
separate from missing mappings.
It also reports C++ source-index health. If the value is `attention`, use the
reported hint and treat unmatched preprocessor conditionals or control-byte
damage in the named translation unit as a source-reconciliation task; do not
rescan or revise the unchanged Lab. An `attention` source index is not
executable evidence, even when a plan title can still be textually located.
The default status focus line is derived from the current Catch2 plan (mapped,
unmapped, unlocated, and executable-candidate counts); archival roadmap prose
is retained for context but cannot make the live queue look current.

`queue --summary --compact` is the bounded family selector. It prints one row
per open roadmap item with its exact work/lane handle and classifies the row as
`ready`, `complete-pointer`, `source-drift-only`, or `new-case-needed`, plus the
separate `action_state` described above. This
prevents the broad overlapping tag set from becoming the work-selection
interface. Family counts intentionally overlap when a case belongs to more than
one roadmap family; use `coverage` for global totals. Enter a row through its
printed `work_query` or `focus_query`; only then inspect the exact test and
clause mapping. Each row also exposes its source-unlocated planned-row count
and deterministic head title, plan id, mapping counts, and (when the row points
at a lane) the live indexed assertion total. It also prints a copyable `trace`
command; these are backlog pointers, never executable evidence until a matching
C++ declaration exists.
When `next_test` is present, the row additionally reports that baseline's
status, assertions, requirement count, canonical 2025 section count, and
official C++ API-surface count. This is intentionally aggregate and bounded;
`trace`/`matrix` remain the detailed reverse maps.

When a selected family has no remaining executable source row, use the pinned
2025 requirement-gap card to choose a deliberate new C++ case without reopening
the Requirements Lab:

```powershell
python tools/query_rti_work.py gaps --summary --compact --limit 8
python tools/query_rti_work.py gaps hla-1516.1-2025:clause-7.2 --summary --compact --limit 8
python tools/query_rti_work.py gaps --family object-ddm-ownership --clause clause-9.1.3.3 --summary --compact --limit 8
python tools/query_rti_work.py requirement <lab-requirement-id> --summary --compact
```

`gaps` reports mapped/unmapped totals and groups uncovered requirements by
canonical `document:clause` subsection. Summary output stays at the subsection
and sample-id level; the individual requirement statement is an explicit
follow-up query. That follow-up remains bounded: if the requirement has no
mapped Catch2 row, `requirement <id> --summary --compact` prints its canonical
subsection, normative statement, and source path so a new C++ case can be
planned directly. An unknown requirement remains a non-zero lookup failure.
Add `--family <roadmap-family-id>` to scope coverage to one roadmap owner and
select a family-local requirement gap without enumerating the full plan.

If the active source pointer is exhausted, `work` and `next --pointer` include
the global unplanned-source head for diagnostics. The default `ready`/`next`
handoff does not select that global source-only head; use the explicit
`--include-source-only` opt-in when reconciling it. If that source queue is
empty, they instead show the overlapping planned-row head with its plan/mapping
counts and a copyable `trace` command. If both queues are exhausted, `next --pointer`
prints the bounded family-selector command so a new service family can be chosen
without expanding or rescanning the full source catalog.

`focus` is the one-screen lane selector. It resolves the active indexed lane
when no tag is supplied, or one exact Catch2 tag when supplied. The result
separates implemented cases from source-located executable candidates and
reports the lane's `action_state` alongside its coarse lane state, so
diagnostic-only source drift is not mistaken for work; it also
historical source-drift rows, then prints the lane owner, requirement,
canonical 2025 subsection, and assertion counts, plus the lane's
Catch2/CTest/JUnit handles.
Compact/summary text also prints bounded requirement IDs, canonical
`document_id:clause_id` subsection keys, and the deduplicated direct-pair count,
so the lane-to-standard relationship is visible without a second trace query.
The lane state is `needs-mapping` whenever an unclassified plan row remains,
even if all of its source cases are implemented; `source-drift` identifies a
planned row whose declaration is not currently locatable. Explicit
no-standalone-surface dispositions remain queryable without being mistaken for
normative requirement coverage. A lane is `complete` only when no executable
candidate, unclassified row, source-drift row, or indexed execution gate
remains. `execution-blocked` is a source/build-integrity status, not a Lab
mapping state; it prevents an aggregate binary from being mistaken for a
runtime pass while preserving the exact trace handles.
When a lane has an indexed `ctest_filter`, text output also prints a complete
copyable `ctest_command`; JSON retains the handle without the rendered command.
Cross-target lanes may instead carry an indexed `ctest_label`; those cards
render an exact `-L "^label$"` command. For example, the directed-interaction
regression card joins its 42 indexed/mapped-plan rows to the 40 executable
tests currently labeled `directed`:

```powershell
python tools/query_rti_work.py focus directed --summary --compact
python tools/query_rti_work.py matrix directed --summary --compact
python tools/query_rti_work.py check --lane directed --summary --compact
ctest --test-dir <build-dir> -C Debug -L "^directed$" --output-on-failure
```

The two explicit dispositions remain visible in the mapping card but are not
counted as runnable CTest cases. This is the preferred query after `work` so a
cross-target regression check does not require a repository-wide search.
Use `focus --json` for scripts and
`focus <tag> --limit 0` only for an intentional unbounded candidate list.

`lanes` is the bounded lane-discovery index. Use it when a family has no
executable source queue or when the next exact tag is not yet known:

```powershell
python tools/query_rti_work.py lanes --family object-ddm-ownership --unmapped --summary --compact --limit 12
python tools/query_rti_work.py lanes --family object-ddm-ownership --focused --summary --compact --limit 12
python tools/query_rti_work.py lanes --family object-ddm-ownership --disposition unclassified --summary --compact --limit 12
python tools/query_rti_work.py lanes --family object-ddm-ownership --summary --compact --limit 12
```

For interactive use, the shorter `lanes object-ddm-ownership` spelling is
equivalent to `lanes --family object-ddm-ownership`; do not combine the
positional family with `--family`. The flag form remains the preferred
scriptable spelling because the scope is named explicitly.

The family-scoped view groups the already-indexed Catch2 rows by exact tag and
prints mapped/unclassified/explicit-disposition/source-drift/candidate counts,
assertions, distinct requirement and 2025-subsection counts, plus one
deterministic `next` test and its `trace`/`focus`/CTest handles. `--unmapped`
keeps tags containing either explicit no-standalone-surface dispositions or
unclassified rows. Add `--disposition unclassified` to select only tags that
still need a mapping decision, or `--disposition explicit` to review only
intentional dispositions. Neither filter infers a new requirement. Use
`--focused` to restrict a family query to its explicit `focused_lane_tags`
aliases; this avoids expanding normal work selection across broad
cross-cutting taxonomy tags. Use
`lanes --json` for automation. This is deliberately bounded lane discovery,
not a repository or Requirements-Lab rescan.

The declared-custom-transportation forms have an exact taxonomy handle. Use it
to inspect their explicit no-standalone-surface dispositions without reopening
the full plan:

```powershell
python tools/query_rti_work.py focus custom-transportation --summary --compact
python tools/query_rti_work.py unmapped --lane custom-transportation --disposition explicit --summary --compact
python tools/query_rti_work.py trace "Embedded ordinary delivery accepts a declared custom FOM transportation" --summary --compact
```

The ordinary receive-order slice is now extracted from the damaged aggregate
and has its own exact lane. Use the narrower handle for implementation and
runtime checks; the remaining timestamped/regional aggregate rows stay a
separate historical gate:

```powershell
python tools/query_rti_work.py focus custom-transportation-delivery --summary --compact
python tools/query_rti_work.py trace "Embedded ordinary delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py matrix "Embedded ordinary delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py check --lane custom-transportation-delivery --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.custom_transportation_delivery\.catch2\.Embedded ordinary delivery accepts a declared custom FOM transportation$" --output-on-failure
```

The extracted ordinary regional companion is an equally bounded DDM slice:

```powershell
python tools/query_rti_work.py focus custom-transportation-ordinary-regional-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded ordinary regional delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py matrix "Embedded ordinary regional delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py check --lane custom-transportation-ordinary-regional-interaction --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.custom_transportation_ordinary_regional_interaction\.catch2\.Embedded ordinary regional delivery accepts a declared custom FOM transportation$" --output-on-failure
```

The ordinary regional-attribute companion is also independently runnable:

```powershell
python tools/query_rti_work.py focus custom-transportation-ordinary-regional-attribute --summary --compact
python tools/query_rti_work.py trace "Embedded ordinary regional attribute delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py matrix "Embedded ordinary regional attribute delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py check --lane custom-transportation-ordinary-regional-attribute --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.custom_transportation_ordinary_regional_attribute\.catch2\.Embedded ordinary regional attribute delivery accepts a declared custom FOM transportation$" --output-on-failure
```

The federation-scoped Current FDD MOM projection is independently runnable
through its own focused target:

```powershell
python tools/query_rti_work.py focus federation-mom-current-fdd --summary --compact
python tools/query_rti_work.py trace "Embedded federation MOM exposes and refreshes HLAcurrentFDD" --summary --compact
python tools/query_rti_work.py matrix "Embedded federation MOM exposes and refreshes HLAcurrentFDD" --summary --compact
python tools/query_rti_work.py check --lane federation-mom-current-fdd --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.federation_mom_current_fdd\.catch2\.Embedded federation MOM exposes and refreshes HLAcurrentFDD$" --output-on-failure
```

This 60-assertion HLA_EVOKED case maps the clause-4 current-FDD content-access
candidate to the official HLAfederation object, HLAunicodeString encoding,
Join-triggered conditional refresh, and direct requested-value path. It is a
development-profile traceability slice; remote/package/JUnit/protected-review
and conformance evidence remain separate.

The companion federation-scoped FOM-module/MIM report behavior is also a
separate, bounded lane:

```powershell
python tools/query_rti_work.py focus federation-mom-content-reports --summary --compact
python tools/query_rti_work.py trace "Embedded federation MOM content reports FOM module and MIM data through a focused lane" --summary --compact
python tools/query_rti_work.py matrix federation-mom-content-reports --summary --compact
python tools/query_rti_work.py check --lane federation-mom-content-reports --summary --compact
ctest --test-dir <build-dir> -C Debug -L "^federation-mom-content-reports$" --output-on-failure
```

This 55-assertion HLA_EVOKED case maps the clause-4 content-access candidate
to the official request/report interactions, proves the FOM-module and MIM
unicode payloads, callback-time subscription gating, and strict malformed
request handling. It remains development-profile traceability rather than
remote/package/JUnit/protected-review or conformance evidence.

The latest FOM-composition slice is the independently runnable transportation-
handle stability case:

```powershell
python tools/query_rti_work.py focus transportation-handle-stability --summary --compact
python tools/query_rti_work.py trace "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
python tools/query_rti_work.py matrix "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
python tools/query_rti_work.py check --lane transportation-handle-stability --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.custom_transportation_handle_stability\.catch2\.Embedded custom transportation handles remain stable across an additional FOM join$" --output-on-failure
```

This 21-assertion HLA_EVOKED case maps 12 Requirements-Lab anchors to 11
canonical 2025 sections and eight official C++ API surfaces. It proves that a
later Join may add an earlier-sorting transportation without renumbering the
existing handle, and that both joined federates resolve the new shared handle.
Keep FOM composition/identity separate from delivery, timestamped, DDM,
save/restore, package, and conformance evidence. `ready --summary --compact`
selects the next indexed handoff; an unlocated row is a source-reconciliation
queue item until a focused C++ declaration exists.

The latest object-lifecycle slice is the independently runnable unnamed
object-instance registration/discovery case:

```powershell
python tools/query_rti_work.py focus object-instance-registration-discovery --summary --compact
python tools/query_rti_work.py trace "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle" --summary --compact
python tools/query_rti_work.py matrix "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle" --summary --compact
python tools/query_rti_work.py check --lane object-instance-registration-discovery --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.object_instance_registration_discovery\.catch2\.Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle$" --output-on-failure
```

This 81-assertion case runs under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps eight
Requirements-Lab anchors to clauses 5.1.2, 6.1.2, 6.8.4, and 6.9.3, and
proves generated instance identity, superclass discovery promotion, callback
re-evaluation after unsubscribe, and known-instance lookup. Keep deletion,
named registration, regional DDM, save/restore, package, and conformance
evidence in their own lanes.

Use `check --family <roadmap-family-id> --summary --compact` when the scope is
an indexed roadmap family rather than an exact Catch2 tag. It applies the same
bounded source and mapping checks to that family’s tagged plan rows while
leaving the global roadmap structure visible; `--lane` and `--family` cannot be
combined.

`matrix` is the bounded reverse map. It accepts an exact test/id, C++ API
surface id or natural method shorthand, Lab requirement, canonical 2025 subsection, or lane tag and emits
one row per matching test with source, status, assertion, requirement-ID, and
`document_id:clause_id` counts/lists. Each JSON row also exposes
`requirement_section_mappings`, the direct Lab-requirement → canonical
subsection pairs; this avoids asking a consumer to infer relationships by
joining separate arrays. With no handle it follows the active indexed lane.
For a standard subsection, the canonical key
(`hla-1516.1-2025:clause-9.5.4`), `clause-9.5.4`, and the numeric shorthand
`9.5.4` are equivalent exact handles; this keeps a printed section number
copyable without another lookup.
An exact indexed roadmap family id (for example `object-ddm-ownership`) is
accepted as an aggregate matrix handle as well. The family fallback happens
only after exact test/API/requirement/section/lane handles, so `trace` remains
strict and deterministic. The `--summary` rows emitted by `test`, `lane`,
`requirement`, `section`, and `search` carry the same compact pair preview,
making the mapping visible regardless of the starting handle.
Use `--json` for complete arrays/pairs; use `trace` when one direct
test-to-requirement statement is needed.
For family-level totals without printing the member rows, use
`coverage --family <roadmap-family-id> --summary --compact`.
Coverage/status family cards include deduplicated direct
`lab_requirement_id -> document_id:clause_id` pair totals (and JSON reports
unresolved rows separately), so separate requirement and section lookups are
not needed just to size a lane or family.
For a review-ready crosswalk rather than one row per case, add
`--group-by requirement` to `matrix` for one row per Requirements-Lab id, or
`--group-by section` for one row per canonical 2025 subsection. Each aggregate
row retains the direct pair count, assertion total, and bounded exact case-id
preview; no relationship is inferred from tag names or array ordering. For
example:

```powershell
python tools/query_rti_work.py matrix transport-and-conformance --group-by requirement --summary --compact --limit 12
python tools/query_rti_work.py matrix transport-and-conformance --group-by section --summary --compact --limit 12
```

When the query itself is an exact requirement id or subsection handle, the
aggregate is narrowed to that one direct key even if the matching case carries
other mappings. Use a family or lane id when the whole-slice crosswalk is
wanted.

For an official API surface, pass its exact id or a natural method shorthand to
either command, for example
`python tools/query_rti_work.py matrix api.2025.cpp.rtiambassador.querylogicaltime.cb29c063787c --summary --compact`
or
`python tools/query_rti_work.py matrix query_logical_time --summary --compact`.

When a source-located case is still unclassified, inspect existing contract
evidence directly instead of searching the Lab export again:

```powershell
python tools/query_rti_work.py unmapped --disposition unclassified --show-contract-candidates --summary --compact --limit 8
```

The bounded output lists exact local contract records, candidate Lab ids, and
clause ids without assigning a plan mapping. A `[source-mismatch]` marker means
the contract still names the test title but points at an older or alternate
translation unit; review that relocation before mapping.

The federation-listing Support Services case now has an exact focused lane:

```powershell
python tools/query_rti_work.py focus federation-listing --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.federation_listing\.catch2\." --output-on-failure
ctest --test-dir .build -C Debug -L "^federation-listing$" --output-on-failure
python tools/query_rti_work.py trace "Embedded federation-list services dispatch standards reports in both callback models" --summary --compact
python tools/query_rti_work.py matrix "Embedded federation-list services dispatch standards reports in both callback models" --summary --compact
```

The source case records 41 assertions under HLA_EVOKED and HLA_IMMEDIATE. Its
two mapped plan rows cover the four Lab requirement anchors in clauses 4.8.4
and 4.10.3 plus all five official C++ API surfaces. Because both service rows
share one executable test, the index records the source assertion total once;
`trace` and `matrix` retain the per-row requirement/API split.

The object-class lookup slice is an independently runnable, directly mapped
example of the same workflow:

```powershell
python tools/query_rti_work.py focus object-class-lookup --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.object_class_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded object-class lookup services use stable handles from the joined federation FOM" --summary --compact
```

It is mapped to the exact 2025 Lab candidates for Get Object Class Handle
(10.4.6) and Get Object Class Name (10.6.2); the focused source and CTest
selector remain the executable evidence boundary.

The symmetric interaction-class lookup boundary is available through the same
bounded handles:

```powershell
python tools/query_rti_work.py focus interaction-class-lookup --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.interaction_class_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded interaction-class lookup services use stable handles from the joined federation FOM" --summary --compact
```

It maps Get Interaction Class Handle and Get Interaction Class Name to the
exact 2025 Lab candidates in clauses 10.13.2 and 10.14.5.

The inherited attribute lookup boundary is also independently runnable:

```powershell
python tools/query_rti_work.py focus attribute-lookup --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded attribute lookup resolves inherited definitions in the joined federation FOM" --summary --compact
```

It maps Get Attribute Handle and Get Attribute Name to exact 2025 Lab
candidates in clauses 10.9.1 and 10.10.3.

The inherited parameter lookup boundary is also independently runnable:

```powershell
python tools/query_rti_work.py focus parameter-lookup --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.parameter_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded parameter lookup resolves inherited definitions in the joined federation FOM" --summary --compact
```

It maps Get Parameter Handle and Get Parameter Name to the exact 2025 Lab
candidates in clause 10.16.1.

The dimension metadata and lookup boundary is also independently runnable:

```powershell
python tools/query_rti_work.py focus dimension-lookup --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.dimension_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded 2025 dimension lookup follows FOM hierarchy and upper bounds" --summary --compact
```

It maps the FOM upper-bound and available-dimension records to the four local
2025 Requirements Lab candidates in clause 9.1.2. Region lifecycle, range
state, and DDM routing remain separate lanes.

The mandatory transportation lookup pair is independently runnable as well:

```powershell
python tools/query_rti_work.py focus transportation-type-lookup --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.transportation_type_lookup\.catch2\." --output-on-failure
ctest --test-dir .build -C Debug -L "^transportation-type-lookup$" --output-on-failure
python tools/query_rti_work.py trace "Embedded transportation type lookup exposes the mandatory 2025 support pair" --summary --compact
python tools/query_rti_work.py trace "Embedded transportation type lookup resolves a declared FOM transportation per execution" --summary --compact
python tools/query_rti_work.py matrix "Embedded transportation type lookup resolves a declared FOM transportation per execution" --summary --compact
```

It maps both the mandatory standard pair and the declared-FOM lookup case to
the exact 2025 Lab candidates in clauses 10.19 and 10.20.4. Custom
transportation composition and delivery are intentionally separate. The CTest
label runs the focused mandatory executable plus both traceability contracts;
the declared-case `trace`/`matrix` commands keep its aggregate source scenario
queryable without a broad plan search.

The time-role control lane is similarly bounded:

```powershell
python tools/query_rti_work.py focus time-role --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded time-role services keep enable requests callback-gated before TSO support" --summary --compact
python tools/query_rti_work.py matrix "Embedded time-role services keep enable requests callback-gated before TSO support" --summary --compact
ctest --test-dir .build -C Debug -L "^time-role$" --output-on-failure
```

This lane maps the official regulation/constrained enable and disable services,
Query Lookahead, and both completion callbacks to eight exact 2025 Lab
candidates (clauses 8, 8.2, 8.3.1, 8.5.5, 8.6.3, 8.7.5, and 8.21.5). The
aggregate case has 51 assertions across `HLA_EVOKED` and `HLA_IMMEDIATE`.
Timestamped delivery, Modify Lookahead, save/restore, packaging/JUnit, and
conformance remain separate. Use the exact `trace`/`matrix` handles when a
focused manifest is unavailable; the repaired aggregate Catch2 target is now
available through its printed CTest handle.

The next time-management handoff is Modify Lookahead:

```powershell
python tools/query_rti_work.py focus modify-lookahead --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded Modify Lookahead applies increases immediately and decreases gradually" --summary --compact
python tools/query_rti_work.py matrix "Embedded Modify Lookahead applies increases immediately and decreases gradually" --summary --compact
ctest --test-dir .build -C Debug -L "^modify-lookahead$" --output-on-failure
```

The case maps the official Modify Lookahead service to seven exact 2025 Lab
candidates in clause 8.20.4 and has 28 `HLA_EVOKED` assertions. It covers
immediate increases, gradual decreases at grants, Query Lookahead, and
time-advance/not-enabled fences. The aggregate source gate is cleared; future-
input coordination, save/restore, timestamped delivery, and conformance remain
separate.

The next time-management slice is Flush Queue Request:

```powershell
python tools/query_rti_work.py focus flush-queue-request --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded Flush Queue Request flushes queued TSO and reports optimistic time" --summary --compact
python tools/query_rti_work.py matrix "Embedded Flush Queue Request flushes queued TSO and reports optimistic time" --summary --compact
ctest --test-dir .build -C Debug -L "^flush-queue-request$" --output-on-failure
```

The case maps Flush Queue Request/Grant and Receive Interaction to five exact
2025 Lab candidates in clauses 8.12 and 8.12.3 and has 42 `HLA_EVOKED`
assertions. It covers queued TSO delivery before FQG, minimum/optimistic grant
selection, callback order, and logical-time fences. Use `focus` for the exact
CTest filter; regional/future-input, save/restore, transport, and conformance
remain separate.

The no-TSO GALT scheduler handoff is a separate, directly queryable slice:

```powershell
python tools/query_rti_work.py focus no-tso-galt-scheduler --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded constrained TAR waits for GALT and is released by a regulator advance" --summary --compact
python tools/query_rti_work.py matrix "Embedded constrained TAR waits for GALT and is released by a regulator advance" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded constrained TAR waits for GALT and is released by a regulator advance$" --output-on-failure
```

This case maps Time Advance Request, Enable Time Regulation, Enable Time
Constrained, and Time Advance Grant to two exact clause-8 Lab candidates and
records 30 `HLA_EVOKED` assertions. It proves the constrained TAR is held at
GALT and released by a regulator advance; NRG/undefined-GALT variants,
timestamped queues, alternate advance modes, save/restore, transport, and
conformance are intentionally separate. The repaired aggregate target and
exact title are directly runnable through the printed CTest handle.

The read-only Query GALT/Query LITS bounds slice is independently queryable:

```powershell
python tools/query_rti_work.py focus query-galt-lits --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded Query GALT and Query LITS observe other regulator time and pending advances" --summary --compact
python tools/query_rti_work.py matrix "Embedded Query GALT and Query LITS observe other regulator time and pending advances" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded Query GALT and Query LITS observe other regulator time and pending advances$" --output-on-failure
```

It maps Query GALT and Query LITS to five exact 2025 Lab candidates across
clauses 8, 8.1.5, 8.18.1, and 8.19.3 and records 35 `HLA_EVOKED` assertions.
The case covers undefined bounds before/after regulation, current lookahead,
pending-advance bounds, matching GALT/LITS values, and the mismatched logical-
time fence. It is read-only no-TSO bounds evidence; scheduler release,
timestamped delivery, alternate advance modes, save/restore, transport, and
conformance remain separate. The repaired aggregate target is directly
runnable through the printed CTest handle.

The configured process endpoint has a separate read-only temporal-bounds
handoff:

```powershell
python tools/query_rti_work.py focus process-query-time-bounds --summary --compact --limit 8
python tools/query_rti_work.py trace "RTIambassador queries GALT and LITS through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador queries GALT and LITS through a configured process endpoint" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries GALT and LITS through a configured process endpoint$" --output-on-failure
```

This 12-assertion `HLA_EVOKED` case maps the official Query GALT and Query
LITS surfaces through the process seam to the same five 2025 Lab anchors as
the embedded no-TSO baseline. It currently proves undefined bounds; available
multi-federate bounds, queued TSO inputs, grants, save/restore, package/JUnit,
validation, and conformance remain separate.

The immediate-callback companion is independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-immediate-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-immediate --summary --compact
python tools/query_rti_work.py trace "RTIambassador queries GALT and LITS through a configured process endpoint with immediate callbacks" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-immediate-integration --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries GALT and LITS through a configured process endpoint with immediate callbacks$" --output-on-failure
```

This 14-assertion `HLA_IMMEDIATE` case maps the same five Lab anchors to four
canonical 2025 sections and adds the official Enable Time Regulation and Time
Regulation Enabled callback surfaces. It proves immediate callback delivery
and undefined GALT/LITS output preservation while keeping available and
multi-federate scheduler behavior, queued/in-transit TSO, grants, save/restore,
package/JUnit, validation, and conformance separate.

The available multi-federate companion is independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-multi-federate-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-multi-federate --summary --compact
python tools/query_rti_work.py trace "RTIambassadors expose defined GALT and LITS across configured process federates" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-multi-federate-integration --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-multi-federate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors expose defined GALT and LITS across configured process federates$" --output-on-failure
```

This 24-assertion `HLA_IMMEDIATE` case maps the same five Lab anchors to four
canonical 2025 sections, proves self-regulator exclusion, and proves an
observer receives defined GALT/LITS from the other regulator's lookahead.
Queued/in-transit TSO, grants, save/restore, package/JUnit, validation, and
conformance remain separate.

The queued-TSO LITS companion is independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-queued-tso-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-queued-tso --summary --compact
python tools/query_rti_work.py trace "RTIambassador queries LITS from queued timestamped process input after regulator disable" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-queued-tso-integration --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-queued-tso --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries LITS from queued timestamped process input after regulator disable$" --output-on-failure
```

This 31-assertion `HLA_IMMEDIATE` case maps the same five Lab anchors to four
canonical sections plus twelve official C++ API surfaces. It queues
timestamped input for a constrained observer, preserves undefined GALT after
regulator disable, and reports the queued timestamp as defined LITS.
In-transit TSO, zero-lookahead epsilon, grants, retraction, save/restore,
package/JUnit, validation, and conformance remain separate.

The zero-lookahead companion is independently selectable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-query-time-bounds-zero-lookahead-integration --summary --compact
python tools/query_rti_work.py focus process-query-time-bounds-zero-lookahead --summary --compact
python tools/query_rti_work.py trace "RTIambassador exposes a zero-lookahead exclusive GALT and LITS boundary through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-query-time-bounds-zero-lookahead-integration --summary --compact
python tools/query_rti_work.py check --lane process-query-time-bounds-zero-lookahead --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador exposes a zero-lookahead exclusive GALT and LITS boundary through a configured process endpoint$" --output-on-failure
```

This 28-assertion `HLA_EVOKED` case maps six Lab anchors to four canonical
sections plus six official C++ API surfaces. It advances a zero-lookahead
regulator through an evoked Time Advance Grant and proves the second federate
observes the exclusive integer GALT/LITS boundary. In-transit TSO,
queued-TSO delivery, constrained grant scheduling, retraction, save/restore,
package/JUnit, validation, interoperability, review, and conformance remain
separate.

The process time-role callback baseline has its own handoff:

```powershell
python tools/query_rti_work.py focus process-time-role --summary --compact --limit 8
python tools/query_rti_work.py trace "RTIambassador enables time constrained through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador enables time constrained through a configured process endpoint" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador enables time constrained through a configured process endpoint$" --output-on-failure
```

This 11-assertion `HLA_EVOKED` case maps Enable Time Constrained and its
Time Constrained Enabled callback through the process seam to two existing
clause-8 time-role anchors. It proves callback gating for the endpoint-owned
role transition; process grants, multi-federate bounds, timestamped delivery,
save/restore, package/JUnit, validation, and conformance remain separate.

The process Time Advance Request/grant baseline has a separate handoff:

```powershell
python tools/query_rti_work.py focus process-time-advance --summary --compact
python tools/query_rti_work.py trace "RTIambassador requests time advance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador requests time advance through a configured process endpoint" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests time advance through a configured process endpoint$" --output-on-failure
```

This 14-assertion `HLA_EVOKED` case maps the request, grant, and Query Logical
Time surfaces to four exact 2025 Lab anchors. It proves the endpoint-owned
single-federate transition and callback queueing; distributed GALT/LITS/TSO
scheduling, save/restore, package/JUnit, validation, and conformance remain
separate.

The process Query Lookahead readback is a separate focused handoff:

```powershell
python tools/query_rti_work.py focus process-query-lookahead-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassador queries lookahead through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador queries lookahead through a configured process endpoint" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador queries lookahead through a configured process endpoint$" --output-on-failure
```

This 13-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:11962` maps Query Lookahead and
callback-gated Enable Time Regulation to two exact 2025 Lab anchors. It proves
the not-enabled exception mapping and official interval encoding/readback over
the process seam; Modify Lookahead, multi-federate bounds, TSO scheduling,
save/restore, package/JUnit, validation, and conformance remain separate.

The process Modify Lookahead mutation is a separate focused handoff:

```powershell
python tools/query_rti_work.py focus process-modify-lookahead-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassador modifies lookahead through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador modifies lookahead through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-modify-lookahead-focused --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador modifies lookahead through a configured process endpoint$" --output-on-failure
```

This 17-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12058` maps three exact Lab
anchors to clause 8.20.4. It proves the not-enabled fence, callback-gated
enablement, official interval encoding, immediate increase, and retention of a
lower request before a time advance. Grant-time decrease application,
multi-federate scheduling, TSO/retraction, save/restore, package/JUnit,
validation, and conformance remain separate lanes.

The grant-boundary companion is a separate, two-federate process handoff:

```powershell
python tools/query_rti_work.py focus process-modify-lookahead-grant-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassadors apply a deferred lower lookahead at a configured process grant" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors apply a deferred lower lookahead at a configured process grant" --summary --compact
python tools/query_rti_work.py check --lane process-modify-lookahead-grant-focused --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors apply a deferred lower lookahead at a configured process grant$" --output-on-failure
```

This 34-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12164` maps six exact Lab
anchors across clauses 8.5.5, 8.6.3, 8.8.3, and 8.20.4. It retains a lower
lookahead before a pending grant, coordinates a constrained TAR with the
regulating federate, and verifies application of the lower interval at the
grant boundary. GALT/LITS/TSO ordering, role-disable, resignation, save/restore,
package/JUnit, validation, and conformance remain separate.

The Available-form process request is a separate, bounded handoff:

```powershell
python tools/query_rti_work.py focus process-time-advance-available-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassador requests available time advance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador requests available time advance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-time-advance-available-focused --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests available time advance through a configured process endpoint$" --output-on-failure
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
    ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador requests next message advances through a configured process endpoint$" --output-on-failure
```

This 22-assertion `HLA_EVOKED` case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:12492` maps four exact Lab
anchors across clauses 8.10.2, 8.11, and 8.11.3. It proves distinct process
operations for NMR and NMRA, the official logical-time codec,
callback-gated grants, and Query Logical Time readback. Queued TSO selection,
inclusive GALT coordination, Flush Queue, multi-federate ordering,
save/restore, package/JUnit, validation, and conformance remain separate.

The queued-TSO companion is independently selectable across both callback models:

```powershell
python tools/query_rti_work.py focus process-time-advance-next-message-queued-focused --summary --compact
python tools/query_rti_work.py trace "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors select queued timestamped process messages for next-message advances" --summary --compact
python tools/query_rti_work.py check --lane process-time-advance-next-message-queued-focused --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors select queued timestamped process messages for next-message advances$" --output-on-failure
```

The case is a 101-assertion aggregate over 19 Lab requirements and 12 canonical
2025 sections at `cpp/tests/ieee1516_2025_connection_catch2.cpp:12600`.

The preceding process lookup slice is the reverse-FOM lookup lane. Keep this as a
separate queryable unit rather than reopening the full declaration-management
family:

```powershell
python tools/query_rti_work.py focus reverse-fom-lookup --summary --compact
python tools/query_rti_work.py trace "RTIambassador reports reverse FOM lookup errors through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix reverse-fom-lookup --summary --compact
python tools/query_rti_work.py check --lane reverse-fom-lookup --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador reports reverse FOM lookup errors through a configured process endpoint$" --output-on-failure
```

The lane now contains four source-located cases and 76 aggregate assertions
(m102 contributes 20; m103 contributes 24; m104 contributes 16; m105
contributes 16) under `HLA_EVOKED` and `HLA_IMMEDIATE`. The m105 case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:13785` records 16 assertions,
maps six Lab requirements to clauses `9.1.2`, `10.19`, and `10.20.4`, and
exercises the unknown-name/invalid-handle error fence across four official API
surfaces. The m104 round-trip, m103, m102, and m101 cases remain available
through their exact titles; multi-federate declaration management,
package/JUnit, review, validation, interoperability, and conformance remain
separate lanes.

The newest bounded process callback-ordering slice is independently selectable:

```powershell
python tools/query_rti_work.py focus process-multi-recipient-callback-ordering --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-multi-recipient-callback-ordering --summary --compact
python tools/query_rti_work.py check --lane process-multi-recipient-callback-ordering --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient interaction FIFO through a configured process endpoint$" --output-on-failure
```

The case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:14106` records 79
assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps ten Lab requirements to six canonical 2025
subsections and five official C++ API surfaces, and proves independent
per-recipient FIFO interaction delivery with preserved identity/tag/parameter
projection and sender exclusion. Immediate delivery, callback-disable,
timestamped/region/directed fanout, package/JUnit, review, validation,
interoperability, and conformance remain separate.

The newest bounded process TSO/DDM slice is independently selectable:

```powershell
python tools/query_rti_work.py focus timestamped-process-regional-interaction --summary --compact
python tools/query_rti_work.py trace m109.embedded-process-tso-regional-interaction --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassadors deliver a timestamped regional interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane timestamped-process-regional-interaction --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a timestamped regional interaction through a configured process endpoint$" --output-on-failure
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
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve per-recipient regional interaction scope through a configured process endpoint$" --output-on-failure
```

The m108 case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:14552`
records 126 assertions under both callback models, maps thirteen Lab anchors
to seven canonical 2025 subsections, and exercises thirteen official C++ API
surfaces. It proves overlap-filtered delivery to two disjoint regional
recipients, send-time source-region metadata through Convey Region Designator
Sets, and sender exclusion. Keep timestamped/retraction, directed,
relaxed-DDM, callback-disable, package/JUnit, review, validation,
interoperability, and conformance in separate lanes.

The preceding bounded process DDM metadata slice is the available-dimensions
hierarchy lane. Keep it independently selectable:

```powershell
python tools/query_rti_work.py focus process-available-dimensions-hierarchy --summary --compact
python tools/query_rti_work.py trace "RTIambassador resolves available FOM dimensions through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-available-dimensions-hierarchy --summary --compact
python tools/query_rti_work.py check --lane process-available-dimensions-hierarchy --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador resolves available FOM dimensions through a configured process endpoint$" --output-on-failure
```

The case at `cpp/tests/ieee1516_2025_connection_catch2.cpp:13933` records 28
assertions under both callback models and maps four Lab requirements to
`hla-1516.1-2025:clause-9.1.2`. It proves inherited object/interaction
dimension sets, an empty set, and official invalid-handle errors through the
federation-owned process catalog. Region lifecycle/routing, package/JUnit,
review, validation, interoperability, and conformance remain separate.

This 56-assertion two-federate `HLA_EVOKED` case sends timestamped
interactions at logical times 5 and 8, then proves the first NMR selects 5
and the following NMRA selects 8. Each interaction callback precedes its
matching grant; Query Logical Time confirms the effective targets. Keep GALT,
LITS, Flush Queue, retraction, multi-recipient ordering, save/restore,
package/JUnit, review, validation, interoperability, and conformance in
separate lanes.

The matching process rejection fence is independently runnable:

```powershell
python tools/query_rti_work.py focus process-time-advance-rejection --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects a backward time advance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects a backward time advance through a configured process endpoint" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a backward time advance through a configured process endpoint$" --output-on-failure
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
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects an incompatible logical time through a configured process endpoint$" --output-on-failure
```

This 9-assertion `HLA_EVOKED` case proves the official `InvalidLogicalTime`
exception for an HLAfloat64Time supplied to an HLAinteger64Time federation and
confirms that no grant callback is created.

The callback-gated pending-role rejection is a separate, exact handoff:

```powershell
python tools/query_rti_work.py focus process-time-advance-time-regulation-pending --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects a process time advance while time regulation enable is pending" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects a process time advance while time regulation enable is pending" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a process time advance while time regulation enable is pending$" --output-on-failure
```

This 14-assertion `HLA_EVOKED` case keeps the official
`RequestForTimeRegulationPending` exception active until the role-enable
callback crosses the shared dispatcher. The process endpoint's eager private
state transition is therefore not exposed as an early public role completion;
the analogous constrained-role case is independently mapped below; malformed
encoding, distributed scheduling, and conformance remain separate.

The constrained-role pending rejection has its own exact handoff:

```powershell
python tools/query_rti_work.py focus process-time-advance-time-constrained-pending --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects a process time advance while time constrained enable is pending" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects a process time advance while time constrained enable is pending" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects a process time advance while time constrained enable is pending$" --output-on-failure
```

This 14-assertion `HLA_EVOKED` case keeps the official
`RequestForTimeConstrainedPending` exception active until the queued
`timeConstrainedEnabled` callback crosses the shared dispatcher. It maps the
Enable Time Constrained request/callback and Time Advance Request to clauses
8.5.5, 8.6.3, and 8.8.3; malformed encoding, distributed scheduling, and
conformance remain separate.

The malformed process logical-time decode fence has its own exact handoff:

```powershell
python tools/query_rti_work.py focus process-time-advance-malformed-encoding --summary --compact
python tools/query_rti_work.py trace "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix "RTIambassador rejects malformed logical-time encoding through a configured process endpoint" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador rejects malformed logical-time encoding through a configured process endpoint$" --output-on-failure
```

This 9-assertion `HLA_EVOKED` case injects a one-byte malformed logical-time
encoding at the process boundary, proves the official `InvalidLogicalTime`
exception, and confirms that no grant callback is emitted. The direct
crosswalk is clause 8.8.3; valid grants, pending-role fences, distributed
scheduling, and conformance remain separate slices.

The two-federate process scheduler has its own exact handoff:

```powershell
python tools/query_rti_work.py focus process-time-advance-federation-scheduler --summary --compact
python tools/query_rti_work.py trace "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors coordinate deferred process time advances through the federation scheduler" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors coordinate deferred process time advances through the federation scheduler$" --output-on-failure
```

This 30-assertion `HLA_EVOKED` case maps the Enable Time Regulation, Enable
Time Constrained, Time Advance Request, and Time Advance Grant surfaces to
clauses 8.2, 8.5.5, 8.6.3, and 8.8.3. It proves constrained TAR deferral,
regulator-driven release, unsolicited grant transport, and Evoke delivery;
timestamped TSO scheduling, save/restore, package/JUnit, validation,
interoperability, and conformance remain separate.

The timestamped process-interaction-before-grant slice has its own exact
handoff:

```powershell
python tools/query_rti_work.py focus process-tso-interaction-before-grant --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process interaction before the grant" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors deliver a deferred timestamped process interaction before the grant$" --output-on-failure
```

This 60-assertion `HLA_EVOKED` case maps 15 Requirements-Lab anchors to nine
canonical 2025 sections and proves timestamped `MainCourseServed` delivery
crosses the process boundary before the matching grant while preserving the
official parameter/tag/transportation/producer/time/order callback surface.
Pre-grant retraction, multi-message ordering, directed/regional TSO,
save/restore, package/JUnit, validation, interoperability, and conformance
remain separate slices.

The timestamped process-attribute-update slice has one bounded query for both
the private service contract and the public endpoint:

```powershell
python tools/query_rti_work.py focus process-tso-attribute-before-grant --summary --compact
python tools/query_rti_work.py trace "Private process service releases timestamped Update Attribute Values before a constrained grant" --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
python tools/query_rti_work.py matrix "RTIambassadors deliver a deferred timestamped process attribute update before the grant" --summary --compact
ctest --test-dir .build -C Debug -R "^(umbra\\.process_boundary_private\\.catch2\\.Private process service releases timestamped Update Attribute Values before a constrained grant|umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassadors deliver a deferred timestamped process attribute update before the grant)$" --output-on-failure
```

These two `HLA_EVOKED` cases total 109 assertions and share 12 mapped
Requirements-Lab anchors across eight canonical 2025 sections. The private
case isolates transport/registry behavior; the public case covers the
official timestamped `Update Attribute Values`/`Reflect Attribute Values`
surface and reflection-before-grant ordering. The separately mapped public
pre-grant-retraction case is now complete:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-tso-attribute-retraction-before-callback-integration --summary --compact
python tools/query_rti_work.py focus process-tso-attribute-retraction-before-callback --summary --compact
python tools/query_rti_work.py trace "RTIambassadors suppress a retracted timestamped process attribute before the callback" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-tso-attribute-retraction-before-callback-integration --summary --compact
python tools/query_rti_work.py check --lane process-tso-attribute-retraction-before-callback --summary --compact
```

It carries 51 assertions, 20 Lab requirements, 13 canonical 2025 sections,
and 16 official C++ API surfaces. Multiple-message ordering, fanout,
regional/DDM, save/restore, package/JUnit, validation, interoperability, and
conformance remain separate slices.

The support-switch state handoff is separately indexed:

```powershell
python tools/query_rti_work.py focus support-switch-state --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded support switches are seeded per federate and retain static FDD policy" --summary --compact
python tools/query_rti_work.py matrix "Embedded support switches are seeded per federate and retain static FDD policy" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded support switches are seeded per federate and retain static FDD policy$" --output-on-failure
```

It maps the twelve official support-switch APIs to eight exact 2025 Lab
candidates across clauses 8.1.10, 9.1.8, 10.44, 10.45.3, 10.46.6, 10.48.1,
10.50.6, and 10.55.1 and records 40 `HLA_EVOKED` assertions. This bounded
case covers FDD initialization, per-federate setter isolation, static getters,
and invalid resign-action rejection. `HLAsetSwitches`, MOM interlocks,
connection-loss cleanup, delayed timestamped delivery, relaxed-DDM routing,
filesystem reporting, packaging, validation, and conformance remain separate.

The whole-object-class declaration teardown is separately indexed:

```powershell
python tools/query_rti_work.py focus whole-object-class-declaration --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries" --summary --compact
python tools/query_rti_work.py matrix "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.fom_declaration_management\.catch2\.Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries$" --output-on-failure
```

It maps whole-class unpublication/unsubscription and related declaration
teardown to eight exact 2025 Lab candidates in clauses 5.3, 5.3.3, and 5.9,
with 34 `HLA_EVOKED` assertions. The case covers invalid-handle/member fences,
inherited publication lifetime, ordinary subscription removal while regional
state remains independent, idempotent teardown, and later-update rejection.
Publication setup, ownership arbitration, regional teardown, save/restore,
transport, packaging, validation, and conformance remain separate.

The public handle-decoding API slice has an explicit no-requirement
disposition:

```powershell
python tools/query_rti_work.py focus public-handle-decoding --summary --compact --limit 8
python tools/query_rti_work.py trace "Embedded public handle decoders enforce lifecycle and preserve encoded identities" --summary --compact
python tools/query_rti_work.py matrix "Embedded public handle decoders enforce lifecycle and preserve encoded identities" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded public handle decoders enforce lifecycle and preserve encoded identities$" --output-on-failure
```

The Lab exports the eight official decoding API surfaces but no standalone
2025 requirement candidate for the codec behavior. Keep it as explicit API
traceability, not a fabricated requirement mapping; its exact case has 50
assertions for lifecycle fences, federation-scoped round trips, and malformed
values. Cross-RTI interoperability, transport, packaging, protected review,
validation, and conformance remain separate.

The mandatory order-type lookup pair is independently runnable as well:

```powershell
python tools/query_rti_work.py focus order-type-lookup --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.order_type_lookup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded order type lookup exposes the mandatory 2025 Receive and TimeStamp pair" --summary --compact
```

It maps the official Get Order Type and Get Order Name APIs, plus the legal
Receive/TimeStamp names, to the five exact 2025 Lab candidates in clauses 8.2,
10.17.4, and 10.19. Order-control services remain a separate lane.

The order-type control case is independently runnable and directly mapped:

```powershell
python tools/query_rti_work.py focus order-type-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.order_type_control\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded order type control captures defaults, instance overrides, and publisher interaction order" --summary --compact
python tools/query_rti_work.py matrix "Embedded order type control captures defaults, instance overrides, and publisher interaction order" --summary --compact
```

It maps Change Attribute Order Type, Change Default Attribute Order Type, and
Change Interaction Order Type to nine exact 2025 Lab candidates in clauses
8.24.4, 8.25.3, and 8.26.4. The focused target records 67 assertions under
HLA_EVOKED; alternate time/TSO, save/restore, remote, and conformance work
remain separate.

The ordinary declaration-relevance advisory case has the same bounded handles:

```powershell
python tools/query_rti_work.py focus declaration-relevance-advisory --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.declaration_relevance_advisory\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded declaration relevance advisories follow ordinary 2025 publication and subscription transitions" --summary --compact
python tools/query_rti_work.py matrix "Embedded declaration relevance advisories follow ordinary 2025 publication and subscription transitions" --summary --compact
```

It maps the four RTI-initiated callbacks and four relevance-advisory switch
accessors to 13 unique 2025 Lab candidates in clauses 5.8, 5.10.2, 5.14.3,
5.15.3, 5.16.5, and 5.17.6. The focused case records 77 HLA_EVOKED
assertions; regional declarations, service-report ordering, package/JUnit,
and conformance remain separate.

The regional declaration-relevance advisory case has its own bounded handles:

```powershell
python tools/query_rti_work.py focus declaration-relevance-advisory-regional --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.regional_declaration_relevance_advisory\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded regional declaration relevance advisories follow active subscriptions" --summary --compact
python tools/query_rti_work.py matrix "Embedded regional declaration relevance advisories follow active subscriptions" --summary --compact
```

It records 49 HLA_EVOKED assertions and maps region-scoped active/passive
declarations plus the four RTI-initiated callbacks to 10 exact 2025 Lab
candidates across clauses 5.8, 5.10.2, 5.14.3, 5.15.3, 5.16.5, and 5.17.6.
Complete regional DDM routing and conformance remain separate.

The paired declaration-relevance service-report boundary has its own exact
filesystem-backed lane:

```powershell
python tools/query_rti_work.py focus declaration-relevance-service-report --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.declaration_relevance_service_report\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded service reporting records declaration relevance advisories before callbacks" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records declaration relevance advisories before callbacks" --summary --compact
```

It records 143 HLA_EVOKED assertions and maps the four advisory callbacks and
the private §11.5/§11.5.2 report-file boundary to six exact 2025 Lab anchors.
The case checks Table 5 object/interaction handle forms, independent per-file
serial order, and report durability before callback dispatch; public MOM
interaction delivery, regional advisories, and conformance remain separate.

For the current object/ownership/DDM handoff, use the stable
`ownership-transfer-update-region` lane. It resolves directly to
`cpp/tests/ownership_transfer_update_region_catch2.cpp:142` (88 assertions,
five Lab anchors, four canonical 2025 sections, eight official C++ API
surfaces) and keeps the former-owner association, default-region recovery, and
new-owner replacement-region checks together. These commands are bounded and
do not reread or resynchronize the Requirements Lab:

```powershell
python tools/query_rti_work.py focus ownership-transfer-update-region --summary --compact
python tools/query_rti_work.py trace "Embedded ownership transfer clears the former owner's 2025 update-region association" --summary --compact
python tools/query_rti_work.py matrix "Embedded ownership transfer clears the former owner's 2025 update-region association" --summary --compact
python tools/query_rti_work.py check --lane ownership-transfer-update-region --summary --compact
```

The adjacent ordinary regional failure-file row is now source-located and
queryable without opening the aggregate test. The exact lane
`ordinary-regional-interaction-failure-service-report-file` resolves to
`cpp/tests/regional_interaction_failure_service_report_file_catch2.cpp:151`
(302 HLA_EVOKED assertions, one Lab anchor, canonical `11.5`, three official
C++ API surfaces). It records invalid interaction-class, parameter, and region
calls through the real configured joined-federate filesystem; its HLA_IMMEDIATE
MOM-interaction companion remains a separate row. Use these bounded handles:

```powershell
python tools/query_rti_work.py focus ordinary-regional-interaction-failure-service-report-file --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records failed regional Send Interaction With Regions invocations" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records failed regional Send Interaction With Regions invocations" --summary --compact
python tools/query_rti_work.py check --lane ordinary-regional-interaction-failure-service-report-file --summary --compact
```

The source-backed file row remains development-profile evidence only; keep
accepted regional delivery, MOM routing, timestamped/re-enable/save/restore,
transport, package/JUnit/protected-review, validation, and conformance as
separate lanes. RL-105/RL-152 are the only Requirements-Lab caveats for this
slice; no Lab resync is needed.

Its companion HLA_IMMEDIATE public-MOM row is also a standalone lane:
`ordinary-regional-interaction-failure-mom-interaction` resolves to
`cpp/tests/regional_interaction_failure_service_report_interaction_catch2.cpp:88`
(179 assertions, three Lab anchors, canonical `9.1.3.3` and `11.5`, five
official C++ API surfaces). It decodes four failure records through
`HLAreportServiceInvocation` while the file switch is disabled, including the
`InvalidRegionContext` rejection for a committed region whose dimensions are
not available to the interaction class. Query it with:

```powershell
python tools/query_rti_work.py focus ordinary-regional-interaction-failure-mom-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers failed regional Send Interaction With Regions invocations through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting delivers failed regional Send Interaction With Regions invocations through MOM interaction" --summary --compact
python tools/query_rti_work.py check --lane ordinary-regional-interaction-failure-mom-interaction --summary --compact
```

The accepted ordinary regional filesystem row is also source-located at
`cpp/tests/regional_interaction_service_report_file_catch2.cpp:185` under the
stable lane `ordinary-regional-interaction-service-report-file` (173
HLA_EVOKED assertions, one Lab anchor, canonical `11.5`, six official C++ API
surfaces). It proves the file record precedes constrained Receive Interaction
delivery for an overlap-qualified send:

```powershell
python tools/query_rti_work.py focus ordinary-regional-interaction-service-report-file --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records accepted regional Send Interaction With Regions before interaction callback" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records accepted regional Send Interaction With Regions before interaction callback" --summary --compact
python tools/query_rti_work.py check --lane ordinary-regional-interaction-service-report-file --summary --compact
```

Use that exact lane rather than the broad historical
`ordinary-regional-interaction-service-report` label, which includes its MOM
companion and traceability checks.

The accepted ordinary regional MOM companion is independently queryable as
`ordinary-regional-interaction-service-report-mom-interaction`. It is
source-located at
`cpp/tests/regional_interaction_service_report_interaction_catch2.cpp:128`
with 107 assertions across HLA_EVOKED publisher/receiver and an
HLA_IMMEDIATE observer, two Lab anchors, canonical `11.5`, and five official
C++ API surfaces. The observer decodes the accepted serial-zero
`SendInteractionWithRegions` report through `HLAreportServiceInvocation` while
file reporting is disabled, before the constrained HLA_EVOKED callback:

```powershell
python tools/query_rti_work.py focus ordinary-regional-interaction-service-report-mom-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction" --summary --compact
python tools/query_rti_work.py check --lane ordinary-regional-interaction-service-report-mom-interaction --summary --compact
```

The exact MOM lane is separate from the filesystem row, ordinary failure
matrices, timestamped/re-enable/save/restore, transport, package/JUnit/
protected-review, validation, and conformance evidence.

```powershell
python tools/query_rti_work.py focus --summary --compact
python tools/query_rti_work.py focus process-boundary --summary --compact
python tools/query_rti_work.py focus service-report-store --json
python tools/query_rti_work.py matrix --summary --compact --limit 20
python tools/query_rti_work.py matrix header-binding-shell --summary --compact --limit 20
python tools/query_rti_work.py matrix time-support --summary --compact --limit 20
python tools/query_rti_work.py matrix hla-1516.1-2025:clause-9.12 --summary --compact --limit 20
```
Use `work --summary` for the bounded handoff: it keeps the active task, lane,
baseline source/mapping counts, and the first executable commands in view while
leaving the full package/JUnit handle set available from plain `work`,
`work --compact`, or `work --json`.
If the catalog marks an historical case as implemented but its exact C++
declaration is absent, the bounded `work`/`ready` views report it as
source-missing/source-drift; that is a reconciliation target, not executable
evidence. Use `next --pointer` when the parent pointer itself is needed.

`check --compact` is the bounded live-plan integrity gate. It verifies roadmap
anchors, Catch2 plan IDs and tags, 2025 requirement references, exact standard
subsection handles, and whether each non-placeholder plan case still has a
matching C++ `TEST_CASE` declaration. It intentionally skips the append-only
historical completion ledger so source splits and replaced plan rows do not
block current work. When the dirty checkout has many source-drift rows it
prints only a small error sample; use `check --json` for the complete
machine-readable diagnostic. Add `--historical` when an audit needs strict
validation of every historical completion row:

```powershell
python tools/query_rti_work.py check --historical --summary --compact
```

Use `check --lane <exact-tag> --compact` while implementing one focus lane. It
keeps the roadmap, requirement, subsection, and tag checks, but limits
test/source and completion-ledger validation to that lane. Historical
recent-slice line numbers may drift when a large translation unit grows; both
focused and unscoped checks tolerate that drift when the source file is
unchanged, while `trace`, `matrix`, and `recent` derive the current declaration
location. Missing declarations, wrong-file pointers, unknown sections, and
other mapping errors still fail. A `disabled-source-artifact` row is retained
for history but is never an executable candidate. Queue summaries show the
diagnostic unlocated total together with the actionable count when those differ;
`ready` uses the actionable count for its next handoff.

`lane <exact-tag> --json` also returns the indexed execution handles for that
lane (Catch2 target or CTest label, CTest regex when applicable, JUnit target,
and artifact path), so the next command can be copied without searching the
build files.

The Connection Lost timestamp-cutoff slice is indexed as a transport lane;
its focused card exposes the aggregate Catch2 target, CTest label, and the
eleven standalone source files (directed interaction, single-recipient
attribute update, staggered multi-recipient update, two late-TAR cleanup cases,
the per-recipient automatic-cleanup case, the HLA_IMMEDIATE counterpart, the
single-recipient timestamped object-deletion cases, the multi-recipient
timestamped object-deletion case, the directed-selector mutation case, and the
regional-selector mutation case):

```powershell
python tools/query_rti_work.py focus connection-lost-tso-cutoff --summary --compact
python tools/query_rti_work.py check --lane connection-lost-tso-cutoff --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -L connection-lost-tso-cutoff --output-on-failure
```

The multi-recipient automatic-cleanup case also has a dedicated target, so it
can be run while the aggregate target is under source reconciliation:

```powershell
cmake --build <build-dir> --config Debug --target umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient\.catch2\." --output-on-failure
```

The single-survivor cutoff case has the same independent execution boundary
and is the exact next planned row for this slice:

```powershell
cmake --build <build-dir> --config Debug --target umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.connection_loss_attribute_update_tso_automatic_cleanup\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded transport loss delivers a cutoff timestamped attribute update before automatic object cleanup" --summary --compact
```

The directed-selector file has an independent target containing both the
by-ownership recheck and the unsubscribe-before-dispatch suppression case:

```powershell
cmake --build <build-dir> --config Debug --target umbra_connection_loss_directed_selector_mutation_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.connection_loss_directed_selector_mutation\.catch2\." --output-on-failure
python tools/query_rti_work.py trace "Embedded transport loss releases automatic cleanup after a cutoff directed interaction is suppressed" --summary --compact
```

The selector-mutation row is directly traceable without opening the aggregate
federation-management source. Its exact plan id, source line, 58 assertions,
requirements, and canonical 2025 sections are returned by:

```powershell
python tools/query_rti_work.py trace "Embedded transport loss rechecks a directed ownership selector before automatic cleanup" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded transport loss rechecks a directed ownership selector before automatic cleanup" --output-on-failure
```

The regional-selector mutation row is likewise directly traceable. Its exact
plan id, source line, 61 assertions, requirements, and canonical 2025 sections
are returned by:

```powershell
python tools/query_rti_work.py trace "Embedded transport loss rechecks a regional selector before automatic cleanup" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded transport loss rechecks a regional selector before automatic cleanup" --output-on-failure
```

`unplanned` is the source-to-plan reconciliation view. It compares derived
`TEST_CASE` declarations with exact plan titles, accepts an optional source
path substring, and returns only source names and locations. It intentionally
does not invent requirements, status, tags, or roadmap ownership; those are
added through an explicit plan row and then checked by `trace`/`check`.
Roadmap items may additionally carry `next_source_state`,
`next_source_test_query`, `next_source_location`, `next_source_lane`, and the
`next_source_requirement_ids`, `next_source_standard_sections`, and
`next_source_api_surfaces` arrays. `work --summary` renders that one bounded
future-test contract beside the mapped baseline. A `planned` source state is a
designated C++ target that has not been declared yet; it is never counted as
executable evidence or silently treated as a passing test.
The compact `status` view prints the three live local queue counts beside the
Catch2 plan total (and flags a stale index snapshot), so queue size is visible
before selecting a lane.

The strict `check --historical` audit also protects the source handoff itself:
when source locations are available and the queue is non-empty, the index must
expose exactly one unplanned-source pointer and that pointer must match the
first item from the global `unplanned` queue. When the queue is exhausted, no
unplanned pointer is allowed; the item may instead declare
`next_source_state=exhausted` with null query/location fields. The normal
unscoped `check` intentionally leaves this append-only source queue out of the
live gate, so diagnostic-only external or superseded declarations do not
block current work. In either mode `ready` and `dashboard` emit bounded open
family options when indexed queues are exhausted, so the next family can be
selected without a repository-wide search or a Requirements-Lab
resynchronization. Focused lane checks remain available for bounded work while
the live gate continues to report missing declarations, wrong-file pointers,
and other current mapping drift.

Lane-level assertion totals come from `mapping.lane_assertion_counts` when a
focused JUnit artifact has verified the lane; both `focus` and `lanes` use that
same aggregate. Exact `trace`/`test` queries keep the per-plan-entry assertion
count beside the source and requirement mapping. If a legacy plan row has no
per-case assertion count, `lanes` also prints `plan_row_assertions` as a
diagnostic instead of silently presenting a smaller total.

The Annex C directed-interaction schema guard is a useful compact example of
the same join: `focus schema-conflict --summary --compact` selects the lane,
while an exact `trace` of `The FDD materializer surfaces the
multiple-directed-class schema conflict` shows its source line, RL-081
requirement, canonical 1516.2 Clause 7 subsection, and owning roadmap family.
The lane's indexed CMake target is `umbra_fom_composer_catch2`; its exact CTest
regex runs only these two guards, so the aggregate federation-management
executable is not required for this iteration.

The same owner now exposes an eight-case `reference-resolution` lane. Its active
pointer is the transportation-name reference guard, which maps directly to
2025 clauses 4.11.2, 6.2.5, 6.2.6, and Annex C:

```powershell
python tools/query_rti_work.py focus reference-resolution --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight resolves reference-data attributes and representations" --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight resolves available dimensions after the complete module set is merged" --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight resolves transportation names after the complete module set is merged" --summary --compact
python tools/query_rti_work.py trace "The FDD materializer retains the complete 2025 support-switch table" --summary --compact
python tools/query_rti_work.py trace "The FDD catalog preserves an explicit NoAction automatic-resign setting" --summary --compact
python tools/query_rti_work.py trace "The FDD materializer is repeatable for a fixed official module set" --summary --compact
python tools/query_rti_work.py trace "The FDD catalog retains an enabled Non-Regulated-Grant switch" --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight retains the first duplicate switch and reports Annex C.8 warnings" --summary --compact
python tools/query_rti_work.py trace "The FOM composition treats omitted switch booleans as their schema default" --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight validates time-representation data type categories" --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight validates user-supplied and synchronization tag data type categories" --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight rejects inherited attribute and parameter name overloading" --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight enforces enumerated and variant-record merge invariants" --summary --compact
python tools/query_rti_work.py trace "The FDD materializer remaps referenced notes and logically ORs service usage" --summary --compact
python tools/query_rti_work.py trace "Reference logical-time selection defaults to HLAfloat64Time and rejects incompatible FDD documentation" --summary --compact
python tools/query_rti_work.py trace "Federation preparation loads MIM first and emits only an FDD-backed reference-time definition" --summary --compact
python tools/query_rti_work.py check --lane reference-resolution --summary --compact
```

Current status is intentionally split: the focused `check --lane` gates are
the iteration signal, while the unscoped snapshot is the repository-wide
live-plan reconciliation gate. It does not validate the append-only completion
ledger unless `--historical` is supplied; that keeps source-unlocated history
from blocking current work while preserving a strict audit command. Run
`status --summary --compact` for the live plan, disposition, source-health,
and direct-pair counts instead of relying on a hard-coded narrative snapshot.
A green focused gate does not claim the whole plan is green.
The completed public object-discovery process-endpoint slice is indexed with 44
assertions under both callback models, three exact requirement/section mappings,
and six official API surfaces. The connection support-types baseline is indexed
as an 11-assertion SDK-consumability row with an explicit
`requirements_lab_api_surface_status` explaining why its requirement and API
lists are empty.
The private registry-binding case adds a four-assertion Query Logical Time
protocol round-trip plus a ten-assertion official encoded `Enable Time
Regulation`/retained-state check, and the public Create/Join/Resign case adds a
two-assertion official-factory reconstruction check plus a four-assertion
callback-gating check. These are bounded process role/state baselines; the
broader timestamped TSO, save/restore, package/JUnit, interoperability,
and conformance work remains separate. The strict
2025-mode rejection of an IEEE 1516-2010 module and
the mixed-edition composer rejection are now mapped two- and seven-assertion
compatibility-only rows; the ambiguous 202x edition-setting guard now verifies
that Connect completes with `SETTINGS_FAILED_TO_PARSE` and is a five-assertion
explicit no-standalone-Lab-surface compatibility row. The executable mapped
§4.2.4 additional-settings evidence is kept in the focused embedded connection
lane. The callback-route
receive-order, immediate-callback reentrancy,
callback-disable, evoked one-at-a-time, concurrent-serialization,
disabled-backlog, Evoke Multiple FIFO, evoked-disabled-pending,
callback-session-close, callback-session concurrent-invocation, and
callback-session active-close rows carry the same explicit disposition when the internal seam is
the tested surface. The order and transportation MOM service-classification
slice is indexed with 66 assertions and 19 requirement anchors. The indexed
`next_source_state=unplanned-source` pointer advances one numeric source
declaration at a time. The joined-federate MOM/FOM-module snapshot slice is
indexed with 12 assertions and
6 requirement anchors; the joined-federate MOM report-file identity save/restore
slice is indexed with 46 assertions and 18 requirement anchors; the joined-
federate MOM regional discovery slice is indexed with 124 assertions and 17
requirement anchors; the
regional-unpublish and receive-order deletion update-region lifetime slices
are indexed with 17 assertions each and 18/20 requirement anchors, the final
object-removal callback slice with 27 assertions and 21 requirement anchors,
the Join advisory-switch seed slice with 16 assertions and 13 requirement
anchors, and the Join-time explicit NoAction automatic-resign slice with 7
assertions and 9 requirement anchors. `next --pointer` prints its exact source
location and `[federation-management]` filter; the completed restored-baseline
regional Request Attribute Value Update provider-response case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:3786` is now indexed
with 44 assertions and 20 requirement anchors. The timestamped Delete Object
Instance retraction case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4305` is now green
with 49 HLA_EVOKED assertions. Its callback-drain boundary consumes the
joined-federate MOM work item after Enable Time Regulation before asserting the
timestamped deletion, retraction, reconstitution, removal, and grant-order
path. The mapped known-class-
disabled attribute-relevance case at
`cpp/tests/attribute_relevance_known_class_disabled_subscription_catch2.cpp:97` is green with
35 HLA_EVOKED assertions, 18 Requirements-Lab anchors, 13 canonical 2025
sections, and 18 official C++ API surfaces. The paired static known-class-
enabled case at
`cpp/tests/attribute_relevance_known_class_enabled_subscription_catch2.cpp:97` is also green
with 30 HLA_EVOKED assertions, 15 Requirements-Lab anchors, 13 canonical 2025
sections, and 17 official C++ API surfaces. The update-rate passive-regional-
subscription case at
`cpp/tests/update_rate_passive_regional_subscription_catch2.cpp:73` is also green
with 43 HLA_EVOKED assertions, 15 Requirements-Lab anchors, 12 canonical 2025
sections, and 21 official C++ API surfaces. The custom-transportation
handle-stability case at
`cpp/tests/custom_transportation_handle_stability_catch2.cpp:44` and the
restored-baseline timestamped MOM interaction case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4538` are mapped and
green. The restored-baseline regional Provide Attribute Value Update case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4697` is green with
251 assertions, 11 Requirements-Lab anchors, 9 canonical 2025 sections, and 21
official C++ API surfaces. The three-dimensional regional object-attribute
overlap case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:47660` is green with
57 assertions, 10 Requirements-Lab anchors, 10 canonical 2025 sections, and 24
official C++ API surfaces. The restored-baseline regional Request Attribute
Value Update solicitation case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:4999` is green with
58 assertions, 9 Requirements-Lab anchors, 7 canonical 2025 sections, and 20
official C++ API surfaces. The restored-baseline timestamped regional Update
Attribute Values MOM failure case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:5461` is green with
138 assertions, 11 Requirements-Lab anchors, 9 canonical 2025 sections, and 30
official C++ API surfaces. The restored-baseline timestamped Update Attribute
Values file-failure case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:5863` is now mapped
as m45 with 218 HLA_EVOKED assertions, 11 Requirements-Lab anchors, 9 canonical
2025 sections, and 15 official C++ API surfaces. The m46 regional reflection-
order, m47 Unpublish Object Class Attributes, m48 partial ownership
cancellation, and m49 pre-delivery ownership cancellation slices are now mapped
and green with 206, 102, 48, and 33 HLA_EVOKED assertions respectively. The m50
disabled Auto Provide discovery baseline is also mapped and green with 21
assertions. The m51 Federation Synchronized-after-resignation service-report
case is also mapped and green with 23 HLA_EVOKED assertions. The m52 terminal
TSO-designator case is also mapped and green with 40 HLA_EVOKED assertions.
The m53 timestamped Update Attribute Values queue/retraction case is now mapped
and green with 68 HLA_EVOKED assertions at
`cpp/tests/timestamped_attribute_update_queued_passel_retraction_catch2.cpp:154`.
The m54
mixed update-rate subscription case is now mapped and green with 37
HLA_EVOKED assertions, eight Requirements-Lab anchors, two canonical 2025
sections, and 15 official C++ API surfaces at
`cpp/tests/mixed_update_rate_subscriptions_catch2.cpp:199`.
The current standalone federation-teardown update-rate-history case is at
`cpp/tests/federation_teardown_update_rate_history_catch2.cpp:158`.
The m55 slice is green with 46 HLA_EVOKED assertions, nine Requirements-Lab
anchors, five canonical 2025 sections, and 15 official C++ API surfaces. The
active m83 multi-recipient default-region restore slice is green with 123
HLA_EVOKED assertions, 18 Requirements-Lab anchors, 14 canonical 2025 sections,
and 24 official C++ API surfaces at
`cpp/tests/restore_live_tso_default_region_attribute_update_multi_recipient_catch2.cpp:187`. It saves one
queued timestamped default-source Update Attribute Values passel, restores
independent queue/retraction state for two regional recipients, flushes each
recipient independently, and verifies one legal Request Retraction callback per
recipient. Query it with the exact title or
`timestamped-default-region-attribute-restore-multi-recipient`; keep the
two-federate baseline, timed-save, explicit-source regional restore, alternate
advance, ownership, transport, and conformance evidence separate.

The active m84 timed explicit-source regional-interaction restore slice is green
with 55 `HLA_EVOKED` assertions, 18 Requirements-Lab anchors, 15 canonical 2025
sections, and 24 official C++ API surfaces at
`cpp/tests/timed_restore_live_tso_regional_interaction_catch2.cpp:5`. It saves
one timestamp-8 `Send Interaction With Regions` passel at logical time 6,
restores the source-region designator and retraction ledger, and proves
Flush Queue delivery before the grant plus one legal post-delivery Request
Retraction. Query it with the exact title or
`timestamped-regional-interaction-timed-restore`; keep the untimed regional
restore baseline, default-region interaction, directed interaction,
attribute-update, multi-recipient, alternate-advance, ownership, transport,
and conformance evidence separate. The next source-backed action is always
emitted by `python tools/query_rti_work.py ready --summary --compact`.

The active m85 timestamped regional-interaction source-resignation slice is
green with 60 `HLA_EVOKED` assertions, 14 Requirements-Lab anchors, 14
canonical 2025 sections, and 27 official C++ API surfaces at
`cpp/tests/timed_live_tso_regional_interaction_source_resignation_catch2.cpp:14`.
It admits one explicit-source timestamp-6 passel before the producer resigns,
then releases the queued interaction through an independent regulator and
verifies the resigned producer cannot retract it. Query it with the exact title
or `timestamped-regional-interaction-source-resignation`; keep timed restore,
multi-recipient restore, alternate-advance, ownership, transport, and
conformance evidence separate. The next source-backed action remains emitted by
`python tools/query_rti_work.py ready --summary --compact`.

The active m86 timestamped regional-interaction restore fan-out slice is green
with 128 `HLA_EVOKED` assertions, 11 Requirements-Lab anchors, 11 canonical
2025 sections, and 35 official C++ API surfaces at
`cpp/tests/restore_live_tso_regional_interaction_multi_recipient_catch2.cpp:169`.
It saves one overlap-qualified explicit-source timestamped interaction while
queued for two independently constrained regional recipients, retracts the
live message, restores both recipient queue entries and the retraction ledger,
then verifies each restored copy before its own Flush Queue grant with one
legal Request Retraction callback. Query it with the exact title or
`timestamped-regional-interaction-restore-multi-recipient`; keep timed restore,
source-resignation, alternate-advance, ownership, transport, and conformance
evidence separate. The next source-backed action remains emitted by
`python tools/query_rti_work.py ready --summary --compact`.

The m56
regional best-effort attribute-rate case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:59566` is green with
47 HLA_EVOKED assertions, 19 Requirements-Lab anchors, 11 canonical 2025
sections, and 22 official C++ API surfaces. The
next unplanned head is the regional best-effort timestamped-rate case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:59710`.
The m57 slice is green with 57 HLA_EVOKED assertions, 30 Requirements-Lab
anchors, 19 canonical 2025 sections, and 28 official C++ API surfaces. The m58
slice is green with 120 assertions, three Requirements-Lab anchors, one
canonical Section 8.1.10 mapping, and 22 official C++ API surfaces under
HLA_EVOKED and HLA_IMMEDIATE. The m59 timestamped directed-interaction slice
is green with 166 assertions under HLA_EVOKED and HLA_IMMEDIATE at
`cpp/tests/delay_subscription_evaluation_timestamped_directed_interaction_catch2.cpp:129`; its callback
drain consumes the queued Time Regulation Enabled notification before the TSO
assertions. Query it with
`python tools/query_rti_work.py trace m59.embedded-delay-subscription-evaluation-timestamped-directed-interaction --summary --compact`.
The m60 timestamped directed-interaction TSO/retraction slice is
green with 56 HLA_EVOKED assertions in the focused target at
`cpp/tests/timestamped_directed_interaction_retraction_catch2.cpp:150`; it applies
the same callback-drain boundary before verifying retraction and delivery. Query
it with
`python tools/query_rti_work.py trace m60.embedded-timestamped-directed-interaction-tso-retraction --summary --compact`.
The next source head is the immediate timestamped directed-interaction
source-resignation case at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:62516`. The m61 case
is green with 45 HLA_EVOKED assertions and proves a timestamped directed
interaction remains deliverable to a non-time-constrained receiver after the
sender resigns. The m62 declaration at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:63554` is retained as
a disabled `#if 0` malformed source artifact with no executable evidence. The
m63 regional source-region snapshot case is green with 42 HLA_EVOKED assertions
at `cpp/tests/ieee1516_2025_federation_management_catch2.cpp:69776`. The m64
timestamped regional interaction TSO/retraction case is green with 68
HLA_EVOKED assertions at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:75428`. The m65
public HLAfloat64Time representation case is green with 31 assertions at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:76784`. The m66,
m67, m68, and m69 no-TSO GALT/NRG cases are green with 21, 15, 27, and 24
assertions at lines 77307, 77349, 77382, and 77428 respectively. The source
queue for this translation unit is now exhausted; choose the next bounded
family with `queue`, `work`, or `focus` rather than rescanning the Lab.
The accepted-directory companion is already indexed as a five-assertion private
Connect/configuration slice mapped to clauses `4.1.1` and `4.2`; its deterministic
failure, AlreadyConnected ordering, unsupported-callback, absent-Disconnect, and
AttributeHandle/callback-route baselines are independently traceable by exact
title.

The exact known-class-disabled and known-class-enabled Attribute Relevance
Advisory regressions are mapped now; use either full title with `trace` to
inspect 18/15 pinned requirements and 13 canonical 2025 sections. Use
`unlocated --summary` and `unmapped --summary` to
work those queues one exact title at a time; do not treat them as a reason to
rescan the unchanged Requirements Lab. The directed process slice now covers
timestamped retraction both before and after receive; query its exact title or
the `federate.callback.request-retraction` tag before selecting the next task.

The service-report-store lane is intentionally isolated from the large
federation-management executable. It currently has 15 source-located Catch2
cases (12 requirement-mapped and three explicit test-seam dispositions) across
the §11.5 and §11.5.2 traces. The MemoryServiceReportStore cases are explicitly
test-seam evidence, while filesystem allocation remains the standards-facing
behavior. Its roadmap owner is `mom-after-base-services`;
the process-boundary and regional-advisory lanes are owned by
`transport-and-conformance`. The embedded m35 advisory regression has a
separate target (`umbra_attribute_relevance_advisory_catch2`) so it can be
validated without rebuilding the large federation-management translation
unit. Its JUnit companion is `umbra_attribute_relevance_advisory_junit` and
writes the same focused result beneath the compliance artifact directory.
Query and execute only this slice with:

```powershell
python tools/query_rti_work.py lane service-report-store --summary --compact
python tools/query_rti_work.py coverage --lane service-report-store --summary --compact
python tools/query_rti_work.py check --lane service-report-store --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.service_report_store\.catch2\." --output-on-failure
cmake --build <build-dir> --config Debug --target umbra_service_report_store_junit
python tools/query_rti_work.py trace umbra-service-report-file-lifecycle-allocates-one-joined-federate-file --summary --compact
python tools/query_rti_work.py trace umbra-service-report-file-lifecycle-switches-gate-appends-only --summary --compact
```

The `service-report-file-lifecycle` lane is the bounded public MOM/file
identity slice. It contains three source-located C++ cases, all mapped, with
232 assertions across 25 Requirements-Lab requirement references and 19
canonical 2025 sections. The public discovery/reflection/removal case is at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp:35806` (152
assertions in both HLA_EVOKED and HLA_IMMEDIATE); the switch-cycle case is at
line 35458 (34 assertions), and the save/restore companion at line 35683 (46
assertions). Use these bounded handles for implementation and evidence:

```powershell
python tools/query_rti_work.py focus service-report-file-lifecycle --summary --compact
python tools/query_rti_work.py matrix service-report-file-lifecycle --summary --compact --limit 10
python tools/query_rti_work.py trace m82.mapping.embedded-joined-federate-mom-public-object-management --summary --compact
python tools/query_rti_work.py test "Embedded joined-federate MOM objects use the public discovery reflection and removal route" --summary --compact
python tools/query_rti_work.py check --lane service-report-file-lifecycle --summary --compact
ctest --test-dir <build-dir> -C Debug -L service-report-file-lifecycle --output-on-failure
```

The new Auto Provide callback-boundary integration case is a separate, directly
indexed service-report slice. It has one HLA_EVOKED C++ case at
`cpp/tests/auto_provide_service_report_file_catch2.cpp:166`, 142 assertions,
five Requirements-Lab requirement anchors, and four canonical 2025 sections
(`1`, `6.1.10`, `11.5`, and `11.5.2`), plus six official C++ API surfaces. It
uses the production filesystem store
and verifies the serial-0 type-37/type-1/type-63 successful-void record with an
empty tag immediately before `Provide Attribute Value Update` callback
delivery. Query and run exactly this bounded case with:

```powershell
python tools/query_rti_work.py trace "Embedded Auto Provide service reporting records empty tag before its callback" --summary --compact
python tools/query_rti_work.py matrix "Embedded Auto Provide service reporting records empty tag before its callback" --summary --compact
python tools/query_rti_work.py section hla-1516.1-2025:clause-11.5.2 --summary --limit 20
python tools/query_rti_work.py lane auto-provide-service-report --summary --compact
python tools/query_rti_work.py check --lane auto-provide-service-report --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.(Embedded Auto Provide service reporting records empty tag before its callback|MOM service-report files preserve the empty Auto Provide tag descriptor form)$" --output-on-failure
```

The stable lane currently contains two mapped cases (143 aggregate assertions),
because it also includes the one-assertion formatter unit; `focus`/`lane` show
that companion without expanding the rest of the service-report catalog.

The federation-wide MOM `HLAsetSwitches` Auto Provide mutation is a separate
source-located lane at `cpp/tests/auto_provide_mom_catch2.cpp:120`. It has 41
HLA_EVOKED assertions, 18 official C++ API surfaces, and explicit canonical
section anchors `hla-1516.1-2025:clause-4`, `...:clause-6.1.10`, and
`...:clause-11.4.1`. It intentionally has zero Lab requirement rows because
RL-032 does not expose a standalone candidate for this federation-wide Table
20 parameter. Use the mapping id or lane directly:

```powershell
python tools/query_rti_work.py trace m90.embedded-mom-hlasetswitches-auto-provide --summary --compact
python tools/query_rti_work.py matrix mom-auto-provide-switch-mutation --summary --compact
python tools/query_rti_work.py focus mom-auto-provide-switch-mutation --summary --compact
python tools/query_rti_work.py check --lane mom-auto-provide-switch-mutation --summary --compact
cmake --build <build-dir> --config Debug --target umbra_auto_provide_mom_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.auto_provide_mom\.catch2\.Embedded MOM HLAsetSwitches adjusts federation-wide Auto Provide$" --output-on-failure
```

The service-report writer-failure lane is source-located at
`cpp/tests/service_report_writer_failure_catch2.cpp:92` and contains two
native HLA_EVOKED cases totaling 27 assertions: join-time writer creation
failure and post-join append failure. Together they map the Requirements-Lab
§11.5/§11.5.2 candidates and keep the six official federation-management C++
API surfaces plus the exception-reporting switch surface explicit. Their
injected internal store seam proves deterministic `RTIinternalError`, no
`memory://` fallback, join-membership rollback, and stable writer identity;
they do not claim production permission/full-disk, external-deletion,
cross-process, package, validation, interoperability, or conformance evidence:

```powershell
python tools/query_rti_work.py trace m91.embedded-service-report-writer-creation-failure --summary --compact
python tools/query_rti_work.py trace "Service-report append failure surfaces an RTI error without an in-memory fallback" --summary --compact
python tools/query_rti_work.py matrix service-report-writer-failure --summary --compact
python tools/query_rti_work.py focus service-report-writer-failure --summary --compact
python tools/query_rti_work.py check --lane service-report-writer-failure --summary --compact
cmake --build <build-dir> --config Debug --target umbra_service_report_writer_failure_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.service_report_writer_failure\.catch2\." --output-on-failure
```

The Register Federation Synchronization Point argument-form slice is now
source-backed in the focused confirm-registration target at
`cpp/tests/service_report_file_confirm_synchronization_point_registration_catch2.cpp:220`.
It has 18 HLA_EVOKED assertions, seven direct Requirements-Lab mappings, and
six canonical 2025 sections. The two public overloads are kept queryable
without reopening the aggregate synchronization test:

```powershell
python tools/query_rti_work.py trace "Embedded service reporting preserves Register Federation Synchronization Point arguments" --summary --compact
python tools/query_rti_work.py focus register-federation-synchronization-point-service-report --summary --compact
python tools/query_rti_work.py check --lane register-federation-synchronization-point-service-report --summary --compact
cmake --build <build-dir> --config Debug --target umbra_service_report_file_confirm_synchronization_point_registration_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.service_report_file_confirm_synchronization_point_registration\.catch2\.Embedded service reporting preserves Register Federation Synchronization Point arguments$" --output-on-failure
```

The joined-federate `HLAsetSwitches` control path is source-backed at
`cpp/tests/mom_federate_set_switches_catch2.cpp:67`. It is a 45-assertion
native C++ HLA_EVOKED case with five direct Requirements-Lab mappings and
canonical 2025 sections 11.4.1, 11.4.2, and 11.5. The exact handles keep the
joined-federate subset, sender-only state, promoted predefined subclass,
ignored extension parameter, malformed resign value, empty-map rejection, and
Service-Reporting/subscription interlock easy to run without a broad search:

```powershell
python tools/query_rti_work.py trace "Embedded MOM HLAsetSwitches updates the sending federate's switch subset" --summary --compact
python tools/query_rti_work.py matrix mom-federate-set-switches --summary --compact
python tools/query_rti_work.py focus mom-federate-set-switches --summary --compact
python tools/query_rti_work.py check --lane mom-federate-set-switches --summary --compact
cmake --build <build-dir> --config Debug --target umbra_mom_federate_set_switches_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.mom_federate_set_switches\.catch2\.Embedded MOM HLAsetSwitches updates the sending federate's switch subset$" --output-on-failure
```

This bounded development-profile slice does not claim the complete MOM
failure-record family, federation-wide switch matrix, remote delivery,
package/JUnit/protected-review evidence, validation, interoperability, or
conformance.

The current bounded implementation handoff is the focused whole-class
default-region unsubscription case at
`cpp/tests/default_region_interaction_routing_catch2.cpp:241`. It is a
20-assertion native C++ `HLA_EVOKED` case mapped to one Requirements-Lab
anchor, canonical 2025 section `9.1.4`, and seven official C++ API surfaces.
Its mapping keeps default-region removal in one small exact lane. The
subscription-dimension, empty-set, multi-region union, mixed-dimensional,
positive-dimensional/default-region, aggregate, and zero-dimensional
interaction/object lanes remain independent gates:

```powershell
python tools/query_rti_work.py case umbra-cpp-whole-class-interaction-unsubscribe-default-region-integration --summary --compact
python tools/query_rti_work.py focus whole-class-unsubscribe --summary --compact
python tools/query_rti_work.py trace "Embedded whole-class interaction unsubscription removes default-region subscription" --summary --compact
python tools/query_rti_work.py matrix whole-class-unsubscribe --summary --compact
python tools/query_rti_work.py check --lane whole-class-unsubscribe --summary --compact
cmake --build <build-dir> --config Debug --target umbra_default_region_interaction_routing_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.default_region_interaction_routing\.catch2\.Embedded whole-class interaction unsubscription removes default-region subscription$" --output-on-failure
```

The preceding subscription-dimension-validation sibling remains available with
`case umbra-cpp-regional-interaction-subscription-dimension-validation-integration`
and the `subscription-dimension-validation` focus handle. The related empty-set
sibling remains available with `case umbra-cpp-regional-interaction-subscription-empty-set-integration`
and the `subscription-empty-set` focus handle. The mixed-dimensional validation and
multi-region union siblings remain available with their exact `case` and
focus handles. Do not perform a
repository-wide or Requirements-Lab rescan. Passive/active,
timestamped/retraction, relaxed/direct DDM, save/restore, transport,
validation, interoperability, package, and conformance remain separate lanes.

The subscription-generation restore case is a separate source-located
`subscription-generation` lane at
`cpp/tests/libxml2_fom_composer_catch2.cpp:1742`. It is a 31-assertion native
C++ unit case mapped to the Requirements-Lab §4.32 candidate, canonical 2025
clause `4.32`, and the Request Federation Restore/Federate Restore Complete
API surfaces. It restores declaration state and proves the next subscription
mutation consumes the saved generation identity; it remains development-profile
evidence rather than public routing or conformance evidence:

```powershell
python tools/query_rti_work.py trace m92.federation-registry-subscription-generation-restore --summary --compact
python tools/query_rti_work.py matrix subscription-generation --summary --compact
python tools/query_rti_work.py focus subscription-generation --summary --compact
python tools/query_rti_work.py check --lane subscription-generation --summary --compact
cmake --build <build-dir> --config Debug --target umbra_fom_composer_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.fom_composer\.catch2\.The federation registry restores subscription-generation allocation with declaration state$" --output-on-failure
```

The filesystem save-history process-restart case is a separate source-located
`federation-save-history-process-restart` lane at
`cpp/tests/federation_registry_catch2.cpp:6661`. It is a 42-assertion
`HLA_EVOKED` native C++ case mapped to ten Requirements-Lab candidates and
canonical clauses `4.19`, `4.20`, `4.27`, and `11.4.1`. It restores a durable
state image into a fresh registry and verifies the federation save-history
conditionals before the next commit; it remains development-profile evidence
and does not claim timed, public-MOM, remote-transport, package, protected-
review, interoperability, or conformance coverage:

```powershell
python tools/query_rti_work.py trace m93.federation-save-history-process-restart --summary --compact
python tools/query_rti_work.py matrix federation-save-history-process-restart --summary --compact
python tools/query_rti_work.py focus federation-save-history-process-restart --summary --compact
python tools/query_rti_work.py check --lane federation-save-history-process-restart --summary --compact
cmake --build <build-dir> --config Debug --target umbra_federation_registry_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.federation_registry\\.catch2\\.Filesystem state image restores federation save conditionals in a fresh registry$" --output-on-failure
```

The public application-value process-restart companion is independently
source-located at
`cpp/tests/public_process_restart_application_value_catch2.cpp:189`. It is an
82-assertion HLA_EVOKED case mapped to nine Requirements-Lab anchors, five
canonical 2025 sections, and 18 official C++ API surfaces. Its exact query and
CTest handles are:

```powershell
python tools/query_rti_work.py focus public-process-restart-application-value-focused --summary --compact
python tools/query_rti_work.py trace "Embedded public fresh-registry restore rehydrates application value and retains report files" --summary --compact
python tools/query_rti_work.py matrix public-process-restart-application-value-focused --summary --compact
python tools/query_rti_work.py check --lane public-process-restart-application-value-focused --summary --compact
cmake --build <build-dir> --config Debug --target umbra_public_process_restart_application_value_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.public_process_restart_application_value\.catch2\.Embedded public fresh-registry restore rehydrates application value and retains report files$" --output-on-failure
```

The case proves fresh-registry filesystem application-value rehydration,
post-restore save continuity, and immutable joined-federate service-report
paths. It remains development-profile evidence; pending application-request
ledgers, timestamped payloads, ownership transfer, remote/package/JUnit/
protected-review evidence, interoperability, and conformance remain separate.

The enabled Auto Provide baseline is its own bounded C++ lane at
`cpp/tests/auto_provide_baseline_catch2.cpp:84`: 28 HLA_EVOKED assertions,
three Lab anchors, two canonical 2025 sections, and 15 official C++ API
surfaces. It proves the enabled FDD switch, discovery, grouped in-scope owner
solicitation, and the empty RTI-invoked tag. Keep it separate from the mapped
Disabled and MOM-mutation companions:

```powershell
python tools/query_rti_work.py focus auto-provide-baseline-state --summary --compact
python tools/query_rti_work.py trace "Embedded Auto Provide solicits in-scope owners after discovery" --summary --compact
python tools/query_rti_work.py matrix "Embedded Auto Provide solicits in-scope owners after discovery" --summary --compact
python tools/query_rti_work.py check --lane auto-provide-baseline-state --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded Auto Provide solicits in-scope owners after discovery$" --output-on-failure
```

The explicit object-instance Request Attribute Value Update baseline is a
separate focused lane at
`cpp/tests/attribute_value_update_request_baseline_catch2.cpp:93`. It passes
36 HLA_EVOKED assertions, maps eight Requirements-Lab anchors to three
canonical 2025 sections (`6.21`, `6.21.5`, `6.22`), and exercises 15 official
C++ API surfaces. It verifies known-instance targeting, grouped current-owner
solicitation, unowned/requester-owned suppression, request-tag propagation,
and evoked callback delivery:

```powershell
python tools/query_rti_work.py focus attribute-value-update-request-baseline-state --summary --compact
python tools/query_rti_work.py trace "Embedded object-instance Request Attribute Value Update solicits 2025 owners" --summary --compact
python tools/query_rti_work.py matrix "Embedded object-instance Request Attribute Value Update solicits 2025 owners" --summary --compact
python tools/query_rti_work.py check --lane attribute-value-update-request-baseline-state --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded object-instance Request Attribute Value Update solicits 2025 owners$" --output-on-failure
```

Keep the object-class overload, regional requests, automatic provision,
timestamped/retraction behavior, response delivery, and broader DDM,
ownership, save/restore, and packaging work in their own lanes.

The regional Allow Relaxed DDM slice is now source-backed in its own focused
translation unit at
`cpp/tests/regional_auto_provide_response_catch2.cpp:148`. It passes 240
assertions across HLA_EVOKED and HLA_IMMEDIATE, maps 14 Requirements-Lab
anchors to 10 canonical 2025 sections, and covers the exact-touching boundary,
positive-gap suppression, and restoration of strict overlap without duplicate
discovery or provider solicitation. Query and run only this lane with:

```powershell
python tools/query_rti_work.py focus regional-automatic-provision-relaxed-ddm --summary --compact
python tools/query_rti_work.py trace "Embedded regional Auto Provide applies Allow Relaxed DDM to touching source projections and suppresses positive gaps under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py matrix "Embedded regional Auto Provide applies Allow Relaxed DDM to touching source projections and suppresses positive gaps under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py check --lane regional-automatic-provision-relaxed-ddm --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.regional_auto_provide_response\\.catch2\\.Embedded regional Auto Provide applies Allow Relaxed DDM to touching source projections and suppresses positive gaps under HLA_EVOKED and HLA_IMMEDIATE$" --output-on-failure
```

The two-provider ownership fan-out slice is likewise isolated at
`cpp/tests/regional_auto_provide_multi_provider_catch2.cpp:193`. It passes 196
assertions under both callback models, maps 22 Requirements-Lab anchors to 14
canonical 2025 sections, and keeps ownership transfer, per-owner solicitation,
and recipient reflection in one bounded executable:

```powershell
python tools/query_rti_work.py focus regional-automatic-provision-multi-provider --summary --compact
python tools/query_rti_work.py trace "Embedded regional Auto Provide fans out one request across two provider owners under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py matrix "Embedded regional Auto Provide fans out one request across two provider owners under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py check --lane regional-automatic-provision-multi-provider --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.regional_auto_provide_multi_provider\\.catch2\\.Embedded regional Auto Provide fans out one request across two provider owners under HLA_EVOKED and HLA_IMMEDIATE$" --output-on-failure
```

The nonregional object-class Request Attribute Value Update baseline is a
separate focused lane at
`cpp/tests/object_class_attribute_value_update_request_baseline_catch2.cpp:94`.
It passes 43 HLA_EVOKED assertions, maps seven Requirements-Lab anchors to
three canonical 2025 sections (`6.21`, `6.21.5`, `6.22`), and exercises 15
official C++ API surfaces. It verifies class-designator expansion over two
concrete subclass instances, per-instance grouped owner callbacks,
unowned/requester-owned suppression, request-tag propagation, and evoked
delivery:

```powershell
python tools/query_rti_work.py focus object-class-attribute-value-update-request-baseline-state --summary --compact
python tools/query_rti_work.py trace "Embedded object-class Request Attribute Value Update solicits 2025 subclass owners" --summary --compact
python tools/query_rti_work.py matrix "Embedded object-class Request Attribute Value Update solicits 2025 subclass owners" --summary --compact
python tools/query_rti_work.py check --lane object-class-attribute-value-update-request-baseline-state --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded object-class Request Attribute Value Update solicits 2025 subclass owners$" --output-on-failure
```

Keep regional class requests, automatic provision, timestamped/retraction
behavior, response delivery, and broader DDM, ownership, save/restore, and
packaging work in their own lanes.

The class-designator service-report companion is a separate source-backed C++
lane at
`cpp/tests/object_class_attribute_value_update_service_report_catch2.cpp:196`.
It passes 403 HLA_EVOKED assertions, maps six Requirements-Lab anchors to five
canonical 2025 sections (`6.21`, `6.21.5`, `6.22`, `11.5`, and `11.5.2`), and
exercises 19 official C++ API surfaces. The production filesystem store keeps
one immutable joined-federate file, with serial-ordered Table 5 type-37,
type-1, and type-63 records durable before each provider callback; the
requester file stays unchanged. This remains development-profile evidence,
not a public MOM interaction or conformance claim:

```powershell
python tools/query_rti_work.py focus object-class-provide-attribute-value-update-service-report --summary --compact
python tools/query_rti_work.py trace "Embedded class Request Attribute Value Update reports each provider callback before delivery" --summary --compact
python tools/query_rti_work.py matrix "Embedded class Request Attribute Value Update reports each provider callback before delivery" --summary --compact
python tools/query_rti_work.py check --lane object-class-provide-attribute-value-update-service-report --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded class Request Attribute Value Update reports each provider callback before delivery$" --output-on-failure
```

Keep object-instance reporting, request-argument preservation, regional and
automatic-provision variants, timestamped/retraction behavior, public MOM,
packaging, validation, and conformance evidence separate.

The Request Attribute Value Update argument-preservation case is now a
standalone filesystem-backed C++ lane at
`cpp/tests/request_attribute_value_update_service_report_catch2.cpp:204`.
It passes 242 HLA_EVOKED assertions, maps four Requirements-Lab anchors to
canonical 2025 sections (`6.21`, `6.21.5`, `11.5`, and `11.5.2.1`), and covers
the official object-instance/class request overloads plus the provider
callback. The unknown-object rejection appends nothing; each accepted request
records its type-37 or type-36 designator, complete type-1 attribute set, and
type-63 base64 tag before the queued callback. Query only this lane:

```powershell
python tools/query_rti_work.py focus request-attribute-value-update-service-report-file --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting preserves Request Attribute Value Update arguments" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting preserves Request Attribute Value Update arguments" --summary --compact
python tools/query_rti_work.py check --lane request-attribute-value-update-service-report-file --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting preserves Request Attribute Value Update arguments$" --output-on-failure
```

Keep regional requests, automatic provision, response delivery, generic
failure/return records, public MOM interaction, timestamped/retraction,
packaging, validation, and conformance evidence separate.

The provider-side companion is now a standalone filesystem-backed C++ lane at
`cpp/tests/provide_attribute_value_update_service_report_catch2.cpp:201`.
It passes 145 HLA_EVOKED assertions, maps six Requirements-Lab anchors to five
canonical 2025 sections (`6.21`, `6.21.5`, `6.22`, `11.5`, and `11.5.2`), and
records the five official request/provider API surfaces. It proves the owner
file is unchanged while the callback is pending, then appends one serial-0
successful-void Table 5 type-37/type-1/type-63 record at callback entry; the
requester file remains unchanged. Query only this lane:

```powershell
python tools/query_rti_work.py focus provide-attribute-value-update-service-report-file --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records Provide Attribute Value Update before its callback" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records Provide Attribute Value Update before its callback" --summary --compact
python tools/query_rti_work.py check --lane provide-attribute-value-update-service-report-file --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting records Provide Attribute Value Update before its callback$" --output-on-failure
```

Keep class, regional, automatic-provision, response, public MOM interaction,
timestamped/retraction, packaging, validation, and conformance evidence in
separate lanes.

The receive-order `Update Attribute Values` service-report companion is a
standalone filesystem-backed C++ lane at
`cpp/tests/update_attribute_values_service_report_catch2.cpp:224`. It passes
157 HLA_EVOKED assertions, maps four Requirements-Lab anchors to three
canonical 2025 sections (`6.10`, `11.5`, and `11.5.2.1`), and covers the two
official Update/Reflect Attribute Values surfaces. Rejected unknown-object
updates append nothing; the accepted non-timestamped update records type-37,
type-2, type-63, and type-34 Null arguments before the reflection callback,
while the receiver file remains unchanged. Query only this lane:

```powershell
python tools/query_rti_work.py focus update-attribute-values-service-report-file --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting preserves receive-order Update Attribute Values arguments" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting preserves receive-order Update Attribute Values arguments" --summary --compact
python tools/query_rti_work.py check --lane update-attribute-values-service-report-file --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting preserves receive-order Update Attribute Values arguments$" --output-on-failure
```

Keep timestamped/regional/DDM forms, return/failure records, interaction-
selected delivery, public MOM, packaging, validation, and conformance
evidence separate.

The receive-order `Delete Object Instance` service-report companion is a
standalone filesystem-backed C++ lane at
`cpp/tests/delete_object_instance_service_report_catch2.cpp:194`. It passes
106 HLA_EVOKED assertions, maps five Requirements-Lab anchors to four
canonical 2025 sections (`6.16`, `6.16.4`, `11.5`, and `11.5.2.1`), and covers
the two official Delete/Remove Object Instance surfaces. Unknown-object
deletion appends nothing; the accepted non-timestamped deletion records
type-37, type-63, and type-34 Null arguments before the removal callback,
while the receiver file remains unchanged. Query only this lane:

```powershell
python tools/query_rti_work.py focus delete-object-instance-service-report-file --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting preserves receive-order Delete Object Instance arguments" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting preserves receive-order Delete Object Instance arguments" --summary --compact
python tools/query_rti_work.py check --lane delete-object-instance-service-report-file --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting preserves receive-order Delete Object Instance arguments$" --output-on-failure
```

Keep timestamped/regional/DDM forms, return/failure records, interaction-
selected delivery, public MOM, packaging, validation, and conformance
evidence separate.

The receive-order `Delete Object Instance` failure-file matrix is a standalone
source-backed lane at
`cpp/tests/delete_object_instance_failure_service_report_catch2.cpp:135`. It
passes 137 HLA_EVOKED assertions, maps four Requirements-Lab anchors to three
canonical 2025 sections (`6.16`, `6.16.4`, and `11.5`), and covers the Delete
Object Instance service plus both service-report switch accessors. It verifies
failed serial-0 and serial-2 records around accepted serial-1 deletion, with
type-37/type-63/type-34 supplied forms, Null return, false indicators, and
exact exception text. Query only this lane:

```powershell
python tools/query_rti_work.py focus delete-object-instance-failure-file-matrix --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records failed receive-order Delete Object Instance invocations" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records failed receive-order Delete Object Instance invocations" --summary --compact
python tools/query_rti_work.py check --lane delete-object-instance-failure-file-matrix --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting records failed receive-order Delete Object Instance invocations$" --output-on-failure
```

Keep timestamped failure, regional/DDM, MOM-interaction, packaging, validation,
and conformance evidence separate.

The receive-order `Delete Object Instance` MOM failure matrix is a standalone
source-backed lane at
`cpp/tests/delete_object_instance_failure_service_report_interaction_catch2.cpp:84`.
It passes 124 HLA_IMMEDIATE assertions, maps four Requirements-Lab anchors to
three canonical 2025 sections (`6.16`, `6.16.4`, and `11.5`), and decodes
object-management service type 2 through `HLAreportServiceInvocation`. It
verifies failed serial-0 and serial-2 reports around accepted serial-1 deletion,
with type-37/type-63/type-34 supplied forms, Null return, false indicators, and
exact exception text. Query only this lane:

```powershell
python tools/query_rti_work.py focus delete-object-instance-failure-mom-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers failed receive-order Delete Object Instance invocations through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting delivers failed receive-order Delete Object Instance invocations through MOM interaction" --summary --compact
python tools/query_rti_work.py check --lane delete-object-instance-failure-mom-interaction --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting delivers failed receive-order Delete Object Instance invocations through MOM interaction$" --output-on-failure
```

Keep filesystem, timestamped, regional/DDM, packaging, validation, and
conformance evidence separate.

The ordinary provider-response spine is a standalone source-backed lane at
`cpp/tests/attribute_value_update_response_catch2.cpp:129`. It passes 37
HLA_EVOKED assertions, maps six Requirements-Lab anchors to three canonical
2025 sections (`6.10`, `6.21`, and `6.21.5`), and exercises four official C++
API surfaces. It proves the known-object request/provider callback/response
reflection chain with the request and response tags, reliable transport, and
no sent-region designator. Query only this lane:

```powershell
python tools/query_rti_work.py focus attribute-value-update-response --summary --compact
python tools/query_rti_work.py trace "Embedded Request Attribute Value Update supports a 2025 provider response" --summary --compact
python tools/query_rti_work.py matrix "Embedded Request Attribute Value Update supports a 2025 provider response" --summary --compact
python tools/query_rti_work.py check --lane attribute-value-update-response --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded Request Attribute Value Update supports a 2025 provider response$" --output-on-failure
```

Keep regional/DDM, automatic provision, timestamped/retraction, update-rate,
save/restore, packaging, validation, and conformance evidence separate.

The regional provider-response DDM recheck is a standalone source-backed lane
at `cpp/tests/regional_attribute_value_update_provider_response_recheck_catch2.cpp:136`.
It passes 39 HLA_EVOKED assertions, maps four Requirements-Lab anchors to two
canonical 2025 sections (`6.10` and `9.13.1`), and covers eight official C++
API surfaces. It queues a no-time response from an overlap-qualified regional
request, commits the requester region to a valid disjoint range before
reflection delivery, and proves callback-time DDM suppression. Query only this
lane:

```powershell
python tools/query_rti_work.py focus regional-provider-response-ddm-recheck --summary --compact
python tools/query_rti_work.py trace "Embedded regional provider response rechecks DDM eligibility at reflection delivery" --summary --compact
python tools/query_rti_work.py matrix "Embedded regional provider response rechecks DDM eligibility at reflection delivery" --summary --compact
python tools/query_rti_work.py check --lane regional-provider-response-ddm-recheck --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded regional provider response rechecks DDM eligibility at reflection delivery$" --output-on-failure
```

Keep automatic provision, timestamped/retraction, relaxed-DDM, save/restore,
package/review, validation, and conformance evidence separate.

The alternate-advance timestamped directed-interaction case is a separate
source-backed lane with 108 HLA_EVOKED assertions, 15 Lab requirements, 10
canonical 2025 sections, and 11 official C++ API surfaces. Its exact source is
`cpp/tests/timestamped_directed_interaction_alternate_advance_catch2.cpp:164`;
the lane keeps FQR, TARA, NMRA, callback ordering, and terminal retraction
traceability in one bounded card:

```powershell
python tools/query_rti_work.py focus timestamped-directed-interaction-alternate-advance --summary --compact
python tools/query_rti_work.py trace "Embedded timestamped directed interaction delivers before FQR TARA and NMRA grants" --summary --compact
python tools/query_rti_work.py matrix "Embedded timestamped directed interaction delivers before FQR TARA and NMRA grants" --summary --compact
python tools/query_rti_work.py check --lane timestamped-directed-interaction-alternate-advance --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timestamped directed interaction delivers before FQR TARA and NMRA grants$" --output-on-failure
```

The joined-federate MOM `HLAfederateState` save/restore case is a separate
source-backed lane at
`cpp/tests/joined_federate_mom_federate_state_save_restore_catch2.cpp:138`.
It passes 152 assertions under both callback models and maps one Lab
requirement to canonical §11.4.1. Its lane card keeps the save/restore state
sequence and self-suppression boundary queryable without opening the aggregate
federation-management translation unit:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-federate-state --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAfederateState follows save and restore callbacks" --summary --compact
python tools/query_rti_work.py matrix "Embedded joined-federate MOM HLAfederateState follows save and restore callbacks" --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-federate-state --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded joined-federate MOM HLAfederateState follows save and restore callbacks$" --output-on-failure
```

The timestamped Delete Object Instance failure-file matrix is a separate
source-backed lane at
`cpp/tests/timestamped_delete_object_instance_failure_service_report_catch2.cpp:127`.
It passes 134 HLA_EVOKED assertions, maps five Lab requirements to four
canonical 2025 sections (`6.16`, `6.16.4`, `8.22.3`, and `11.5`), and exercises
the three official Delete Object Instance/service-report switch surfaces. It
records the unknown-object and earlier-than-lookahead failures in the
production filesystem report with the supplied argument forms, Null returned
argument, exact exception text, and serials zero and one:

```powershell
python tools/query_rti_work.py focus timestamped-delete-object-instance-failure-service-report --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records failed timestamped Delete Object Instance invocations" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records failed timestamped Delete Object Instance invocations" --summary --compact
python tools/query_rti_work.py section hla-1516.1-2025:clause-11.5 --summary --limit 20
python tools/query_rti_work.py check --lane timestamped-delete-object-instance-failure-service-report --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting records failed timestamped Delete Object Instance invocations$" --output-on-failure
```

The accepted timestamped Delete Object Instance companion is a separate
source-backed lane at
`cpp/tests/timestamped_delete_object_instance_service_report_catch2.cpp:193`.
It passes 91 HLA_EVOKED assertions, maps four Lab requirements to four
canonical 2025 sections (`6.16`, `6.17.1`, `8.1.5`, and `11.5`), and exercises
Delete Object Instance, Remove Object Instance, and both service-report switch
surfaces. The type-34 Null-return sender record is checked before the queued
timestamped removal callback:

```powershell
python tools/query_rti_work.py focus timestamped-delete-object-instance-sender-file --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records timestamped Delete Object Instance before removal callback" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records timestamped Delete Object Instance before removal callback" --summary --compact
python tools/query_rti_work.py check --lane timestamped-delete-object-instance-sender-file --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded service reporting records timestamped Delete Object Instance before removal callback$" --output-on-failure
```

The time-regulated timestamped Delete Object Instance sender-file case is a
separate source-backed lane at
`cpp/tests/time_regulated_timestamped_delete_object_instance_service_report_catch2.cpp:206`.
It passes 172 HLA_EVOKED assertions, maps five Lab requirements to five
canonical 2025 sections (`6.16`, `6.17.1`, `8.1.5`, `8.1.6`, and `11.5`), and
exercises eight official C++ API surfaces. The type-33 MessageRetractionHandle
return, stable advertised file, and timestamped callback-before-grant ordering
are all checked:

```powershell
python tools/query_rti_work.py focus timestamped-delete-object-instance-time-regulated-sender-file --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records time-regulated timestamped Delete Object Instance with retraction handle" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting records time-regulated timestamped Delete Object Instance with retraction handle" --summary --compact
python tools/query_rti_work.py check --lane timestamped-delete-object-instance-time-regulated-sender-file --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records time-regulated timestamped Delete Object Instance with retraction handle$" --output-on-failure
```

The accepted timestamped Delete Object Instance MOM-interaction companion is
source-backed at
`cpp/tests/timestamped_delete_object_instance_service_report_interaction_catch2.cpp:145`.
It passes 104 assertions across HLA_EVOKED publisher/receiver and an
HLA_IMMEDIATE observer, maps five Lab requirements to four canonical 2025
sections (`6.16`, `6.17.1`, `8.1.5`, and `11.5`), and exercises ten
official C++ API surfaces. The type-33 MessageRetractionHandle return is
decoded through `HLAreportServiceInvocation` while file reporting is disabled,
then the receiver's timestamped callback-before-grant ordering is checked:

```powershell
python tools/query_rti_work.py focus timestamped-delete-object-instance-service-report-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers accepted timestamped Delete Object Instance through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting delivers accepted timestamped Delete Object Instance through MOM interaction" --summary --compact
python tools/query_rti_work.py check --lane timestamped-delete-object-instance-service-report-interaction --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted timestamped Delete Object Instance through MOM interaction$" --output-on-failure
```

The timestamped Delete Object Instance MOM-failure matrix is independently
queryable as `timestamped-delete-object-instance-failure-mom-interaction`. It
passes 92 HLA_IMMEDIATE assertions, maps five Lab requirements to four
canonical 2025 sections (`6.16`, `6.16.4`, `8.22.3`, and `11.5`), and exercises
five official C++ API surfaces. It records unknown-object and earlier-than-
lookahead failures through HLAreportServiceInvocation with type-37/type-63/
type-31 forms, a type-34 Null return, exact exception text, and serials zero
and one:

```powershell
python tools/query_rti_work.py focus timestamped-delete-object-instance-failure-mom-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix "Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction" --summary --compact
python tools/query_rti_work.py check --lane timestamped-delete-object-instance-failure-mom-interaction --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction$" --output-on-failure
```

The focused run is still development-profile evidence: switch-setter reports,
class/regional automatic-provision variants, public MOM interaction delivery,
failure records, package/JUnit/protected-review evidence, interoperability, and
conformance remain separate open work.

The installable-profile process package has thirteen independently addressable
checks: `package-process`, `package-process-timestamped`,
`package-process-parameterized`, `package-process-connection-loss`, and
`package-process-connection-loss-delete-objects`,
`package-process-object-registration`, `package-process-named-registration`,
`package-process-attribute-update`, `package-process-directed-retraction`, and
`package-process-federation-save-restore`,
`package-process-federation-save-restore-failure`, and
`package-process-federation-save-restore-abort`, and
`package-process-federation-save-restore-status`.
Their exact CTest names and labels are
printed by `work --summary --compact`, so selecting a package regression does
not require a repository-wide search.
The installable-package smoke runs
`tools/verify_process_package_lanes.py` immediately after configuring the
clean downstream consumer. That catalog check compares all thirteen indexed test
names and labels, plus the matching `mapping.lane_handles` entries, to the
generated CTest catalog and fails before execution if
a lane is missing, renamed, duplicated, or unindexed. Run the same check
directly when iterating on the package consumer; after the run, pass the
deterministic JUnit path to validate the emitted report as well:

```powershell
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug `
  --output-on-failure `
  --output-junit <build-dir>/package-smoke-consumer/Testing/package-process.xml
python tools/verify_process_package_lanes.py --ctest ctest `
  --test-dir <build-dir>/package-smoke-consumer `
  --index docs/planning/ROADMAP-INDEX.json --config Debug `
  --junit <build-dir>/package-smoke-consumer/Testing/package-process.xml
```

The verifier path and artifact are exposed as
`process_package_catalog_verifier` and `process_package_junit_artifact` by
`work --summary --compact`.
The parameterized check resolves the server-owned `HLAobjectRoot.Customer`
class and `TimelinessOk` parameter, then verifies the exact parameter handle
and value bytes at the official receiver callback.

The DELETE_OBJECTS connection-loss check is independently addressable as
`umbra_rti_package_process_connection_loss_delete_objects_consumer` under
`package-process-connection-loss-delete-objects`; it uses the installed public
API to set `DELETE_OBJECTS`, closes the receiver transport, verifies one
official `removeObjectInstance` callback with the empty tag and lost producer,
and confirms the deleted object name is no longer known before the surviving
sender resigns. Query its exact handle with:

```powershell
python tools/query_rti_work.py lane package-process-connection-loss-delete-objects --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-connection-loss-delete-objects --output-on-failure
```
The lane handle carries the backing C++ plan id, its five Requirements-Lab
requirement ids, and its five canonical 2025 section keys; use `case` or
`trace` on `umbra-cpp-connection-lost-automatic-delete-integration` for
the assertion-level mapping and use this package lane for installed-process
evidence only.

The attribute-update package projection now invokes the official public
`subscribeObjectClassAttributes` surface across the process seam before the
fixture registers the sender's object. The process service now projects
object discovery as well, so the installed consumer exercises the official
`discoverObjectInstance` callback before `reflectAttributeValues`.
The directed/retraction package projection is independently addressable as
`umbra_rti_package_process_directed_retraction_consumer` under
`package-process-directed-retraction`; it uses the installed public API to
register `HLAobjectRoot.Employee.Server`, publish/subscribe its directed
interaction, receive the first timestamped message through the official
directed callback, call `Retract` and verify one matching post-delivery
`requestRetraction` callback, then send a second message and verify that
pre-delivery `Retract` suppresses both the directed callback and any second
retraction callback.
The federation save/restore package projection is independently addressable as
`umbra_rti_package_process_federation_save_restore_consumer` under
`package-process-federation-save-restore`; it uses one installed public
ambassador to verify the save initiation/completion boundary and the ordered
restore success callbacks. Query its exact handle and focused rerun with:

```powershell
python tools/query_rti_work.py lane package-process-federation-save-restore --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-federation-save-restore --output-on-failure
```
The indexed handle carries the six Requirements-Lab requirement ids and six
canonical 2025 section keys from the native process restore-success case; the
installed lane remains process-boundary foundation evidence, not a conformance
promotion.
The restore-failure projection is independently addressable as
`umbra_rti_package_process_federation_save_restore_failure_consumer` under
`package-process-federation-save-restore-failure`; it verifies the official
`Federation Not Restored` callback and
`FEDERATE_REPORTED_FAILURE_DURING_RESTORE` reason. Query it with:

```powershell
python tools/query_rti_work.py lane package-process-federation-save-restore-failure --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-federation-save-restore-failure --output-on-failure
```
Its indexed handle carries the six requirement/section mappings from the native
restore-failure case and remains foundation evidence.
The restore-abort projection is independently addressable as
`umbra_rti_package_process_federation_save_restore_abort_consumer` under
`package-process-federation-save-restore-abort`; it verifies
`Abort Federation Restore`, `Federation Not Restored`, and
`RESTORE_ABORTED`. Query it with:

```powershell
python tools/query_rti_work.py lane package-process-federation-save-restore-abort --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-federation-save-restore-abort --output-on-failure
```
Its indexed handle carries the six requirement/section mappings from the native
restore-abort case and remains foundation evidence.
The restore-status projection is independently addressable as
`umbra_rti_package_process_federation_save_restore_status_consumer` under
`package-process-federation-save-restore-status`; it verifies
`Query Federation Restore Status`, an in-progress `FEDERATE_RESTORING` status
response, and the subsequent restore completion. Query it with:

```powershell
python tools/query_rti_work.py lane package-process-federation-save-restore-status --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-federation-save-restore-status --output-on-failure
```
Its indexed handle carries the nine requirement/section mappings from the native
restore-status case and remains foundation evidence.
Query that declaration mapping directly with:

```powershell
python tools/query_rti_work.py lane rti.service.subscribe-object-class-attributes --summary --compact
python tools/query_rti_work.py lane rti.service.unsubscribe-object-class-attributes --summary --compact
python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
```

The CTest catalog audit is the guard for this distinction:

```powershell
ctest --test-dir <build-dir> -C Debug -R "^umbra.ieee1516_2025.focused_service_lane_catalog$" --output-on-failure
```

Labels that currently have only Requirements-Lab/API checks remain runnable as
traceability-only lanes, but are not advertised by that audit as complete
behavior lanes until a tagged Catch2 case exists.

`recent --summary --compact` is the short handoff ledger (newest indexed slice
first). It prints only the recently completed exact test title, source line, assertion count, callback
model, lane handles, any explicit internal/API-surface disposition, and resolved
requirement/section counts. Use it to resume
work without reopening the full Catch2 plan or source file. Add
`--lane <exact-tag>` when only one focus lane is relevant, for example:

```powershell
python tools/query_rti_work.py recent --lane regional-automatic-provision-timestamped-response --summary --compact --limit 5
```

## Follow one slice

For the active process-boundary transport slice, start with the endpoint
handshake/data exchange, verify the registry-bound service baseline, verify the
independently launched process baseline, and then use the roadmap's
`next_work_query` for the installable-profile process gate:

The live `[process-boundary]` lane currently reports 155 mapped plan rows / 6,717
indexed Catch2 assertions (6,869 assertions recorded by plan rows) and has no actionable unlocated rows; use the bounded
`focus process-boundary` command above for the authoritative count. Its stable
CTest label selects 209 registered test instances. The JUnit target executes
the 119 independently buildable registrations; its merged aggregate contains
214 emitted section-level testcases and 5,458 assertions,
with zero failures/errors; the aggregate umbrella registration is intentionally
CTest-only because its federation-management translation unit is not an
evidence source. That report metric is intentionally separate from the generated
traceability-row count. The local-delete slice
contributes 9 direct codec assertions, 44 private service assertions, and 18
public two-federate endpoint assertions; receive-order Delete Object Instance
adds 25 direct codec assertions and 25 public endpoint/removal-callback
assertions; the timestamped public Delete Object Instance endpoint slice adds
283 assertions across regulated and non-regulated variants under both callback
models, including queued constrained-recipient and preferred receive-order
variants under both callback models. It
projects the process execution identity as the public
`MessageRetractionHandle` only for the time-regulating producer and verifies
the absent-designator boundary otherwise. Query that exact source-backed
contract directly:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --summary --compact
python tools/query_rti_work.py test "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint$" --output-on-failure
```

The mapped row is at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:9166`; it covers 14 Lab
requirements, eight canonical 2025 sections, and seven official C++ API
surfaces. This is still private process-foundation evidence: broader regional
or directed delivery remain separate follow-on slices. The timestamped regional Update Attribute
Values endpoint slice adds 86 assertions under both callback models.
The disjoint regional-update endpoint adds 50 assertions under both models; the
remote regional subscription/update endpoint adds 66, the regional-registration
endpoint adds 34, the object-registration endpoint adds 20 under both models, and
the public object-class subscription endpoint adds 24 under both callback models,
and the public object-discovery endpoint adds 44 under both callback models.
The private registry-binding case adds a four-assertion Query Logical Time
protocol round-trip plus a ten-assertion official encoded `Enable Time
Regulation`/retained-state check, and the public Create/Join/Resign case adds a
two-assertion official-factory reconstruction check plus a four-assertion
callback-gating check. These are bounded process role/state baselines; the
broader timestamped TSO, save/restore, package/JUnit, interoperability,
and conformance work remains separate. The process TAR
pending-role slices add 28 assertions: one keeps
`RequestForTimeRegulationPending` active and the other keeps
`RequestForTimeConstrainedPending` active until its queued role-enable callback
crosses the shared dispatcher. The malformed process logical-time decode fence
adds 9 assertions and maps directly to clause 8.8.3. The two-federate process
federation scheduler adds 30 assertions for deferred constrained TAR,
regulator-driven release, unsolicited grant transport, and Evoke delivery.
The timestamped process interaction-before-grant slice adds 62 `HLA_EVOKED`
assertions mapped to 20 Requirements-Lab anchors and 13 canonical 2025
sections; query `process-tso-in-transit` for its exact trace. The separate
public pre-grant-retraction slice is also complete and directly queryable:

```powershell
python tools/query_rti_work.py focus process-tso-attribute-retraction-before-callback --summary --compact
python tools/query_rti_work.py trace "RTIambassadors suppress a retracted timestamped process attribute before the callback" --summary --compact
python tools/query_rti_work.py check --lane process-tso-attribute-retraction-before-callback --summary --compact
```

It carries 51 assertions across 20 Lab requirements, 13 canonical sections,
and 16 official C++ API surfaces. Multi-message ordering, directed/regional
TSO, save/restore, package/JUnit, validation, interoperability, and
conformance remain separate slices.
The paired timestamped process-attribute-update cases add 109 `HLA_EVOKED`
assertions and share 12 Requirements-Lab anchors across eight canonical 2025
sections; query `process-tso-attribute-before-grant` for their combined focus
card and query each exact test title for the private/public trace.
The regional subscription-removal endpoint slice adds 112 assertions under both
callback models.
The connection support-types baseline is queryable by its exact test title and
reports its explicit no-standalone-Lab-surface disposition alongside the empty
requirement and subsection lists.
The bounded `focus transport` query remains a family-level diagnostic; use the
exact process lane above instead of reopening the full catalog.
The 117-case query count is the unique mapped plan/test-declaration count; the
207-test CTest label count is the executable cross-target count. The
`process-boundary` CTest label uses the independently buildable private target
plus the public connection target. The JUnit target runs both executables and
merges their reports, so it does not depend on the damaged
aggregate federation-management translation unit.

```powershell
python tools/query_rti_work.py lane process-boundary --summary --compact
python tools/query_rti_work.py lane callback-controls --summary --compact
python tools/query_rti_work.py test "Private process transport exchanges framed data after endpoint handshake" --summary --compact
python tools/query_rti_work.py test "Private process service binds create join and receive-order interaction to the federation registry" --summary --compact
python tools/query_rti_work.py test "Private registry-bound service exchanges federation traffic across independently launched processes" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Create, Join, and Resign through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador resolves interaction and parameter handles through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers a directed interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers a directed interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py lane timestamped-directed-retraction --summary --compact
python tools/query_rti_work.py test "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py trace "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py trace "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py lane federate.callback.request-retraction --summary --compact
python tools/query_rti_work.py lane callback-gating --summary --compact
python tools/query_rti_work.py test "RTIambassador publishes object-class attributes and registers an object through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers object discovery through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador reserves a name and registers a named object through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador projects the 2025 region lifecycle and regional registration through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes a remote regional subscription and scoped update through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador preserves a timestamped regional update through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador removes a regional subscription through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador suppresses a disjoint regional update through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "Private process service projects owner-directed Attribute Relevance Advisory events" --summary --compact
python tools/query_rti_work.py trace "Private process service projects owner-directed Attribute Relevance Advisory events" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers Attribute Relevance Advisory callbacks through a configured process endpoint under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers Attribute Relevance Advisory callbacks through a configured process endpoint under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py test "Private process callback bridge suppresses a queued Attribute Relevance Advisory after switch disable" --summary --compact
python tools/query_rti_work.py trace "Private process callback bridge suppresses a queued Attribute Relevance Advisory after switch disable" --summary --compact
python tools/query_rti_work.py test "RTIambassador delivers regional Attribute Relevance Advisory subscription transitions through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers regional Attribute Relevance Advisory subscription transitions through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers an initial regional Attribute Relevance Advisory after discovery through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories follow scope transitions" --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories use subscriptions when known-class policy is disabled" --summary --compact
python tools/query_rti_work.py search known-class-disabled --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories honor known class when the static policy is enabled" --summary --compact
python tools/query_rti_work.py search known-class-enabled --summary --compact
python tools/query_rti_work.py trace "Embedded update-rate lookup ignores passive regional subscriptions" --summary --compact
python tools/query_rti_work.py focus update-rate-passive-regional-subscription --summary --compact
python tools/query_rti_work.py trace "Embedded mixed update-rate subscriptions gate each attribute independently" --summary --compact
python tools/query_rti_work.py focus update-rate-mixed-attribute-gating --summary --compact
python tools/query_rti_work.py trace "Embedded custom transportation handles remain stable across an additional FOM join" --summary --compact
python tools/query_rti_work.py search transportation-handle-stability --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped Update Attribute Values invocations through MOM interaction (restored baseline copy)" --summary --compact
python tools/query_rti_work.py search restored-baseline-copy --summary --compact
python tools/query_rti_work.py trace "Embedded regional Provide Attribute Value Update reports before callback delivery (restored baseline copy)" --summary --compact
python tools/query_rti_work.py trace "Embedded three-dimensional regional object attributes require complete overlap" --summary --compact
python tools/query_rti_work.py search complete-overlap --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers failed timestamped regional Update Attribute Values invocations through MOM interaction (restored baseline copy)" --summary --compact
python tools/query_rti_work.py search timestamped-regional-attribute-update-failure --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting records failed timestamped Update Attribute Values invocations (restored baseline copy)" --summary --compact
python tools/query_rti_work.py search timestamped-attribute-update-failure --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories reissue turn-on when the active update rate changes" --summary --compact
python tools/query_rti_work.py trace "Embedded regional attribute relevance advisories retain explicit update-rate designators" --summary --compact
python tools/query_rti_work.py test "RTIambassador reissues Attribute Relevance Advisory callbacks when the active update rate changes through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador reissues Attribute Relevance Advisory callbacks when the active update rate changes through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador preserves an explicit regional Attribute Relevance Advisory update-rate designator across configured process endpoint transitions" --summary --compact
python tools/query_rti_work.py trace "RTIambassador preserves an explicit regional Attribute Relevance Advisory update-rate designator across configured process endpoint transitions" --summary --compact
python tools/query_rti_work.py test "Embedded regional attribute relevance advisories recheck callbacks after an active-rate and scope transition under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py trace "Embedded regional attribute relevance advisories recheck callbacks after an active-rate and scope transition under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
python tools/query_rti_work.py lane rti.service.associate-regions-for-updates --summary --compact
python tools/query_rti_work.py lane rti.service.unassociate-regions-for-updates --summary --compact
python tools/query_rti_work.py requirement requirement-candidate-content-clauses-09-data-distribution-management-page-230-l157-48 --summary --limit 20
python tools/query_rti_work.py section hla-1516.1-2025:clause-9.7.5 --summary --limit 20
python tools/query_rti_work.py search "get-object-class-handle" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes ordinary interaction declarations through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador receives a process interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py test "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py lane rti.service.subscribe-object-class-attributes --summary --compact
python tools/query_rti_work.py test "Embedded transport loss applies the bounded automatic NoAction forced-resign policy" --summary --compact
python tools/query_rti_work.py test "RTIambassador selects a configured tcp process endpoint through the official address field" --summary --compact
python tools/query_rti_work.py test "RTIambassador rejects malformed tcp process addresses before connecting" --summary --compact
python tools/query_rti_work.py work --compact
```

```powershell
python tools/query_rti_work.py lane <exact-catch2-tag> --compact
python tools/query_rti_work.py test "<exact TEST_CASE title>" --compact
python tools/query_rti_work.py requirement <lab-id-or-clause> --summary --limit 20
python tools/query_rti_work.py section <document-id>:<clause-id> --summary --limit 20
python tools/query_rti_work.py coverage --lane process-boundary --summary --compact
```

The lane query is the normal starting point. The exact test query resolves to
the C++ file and line, status, assertion count, callback models/delivery modes,
tags, selected API surfaces, owning roadmap families, Lab requirement IDs, and
canonical 2025 clause/subsection mappings. Its bounded `--summary` form previews the
API-surface IDs and requirement/section keys (up to eight per field) and prints
service/callback tags; the regular `--compact` form retains
every mapping. Use `requirement` for the reverse
lookup from a Lab requirement (or clause) to every mapped test. Use `section`
when the standard subsection is the starting point.

For one direct, unambiguous trace use `trace`. It accepts an exact Catch2 plan
id or `TEST_CASE` title, an exact Requirements-Lab (or contract) requirement id,
an exact canonical 2025 `document_id:clause_id` key, or an exact Catch2 lane
tag. The command never falls back to fuzzy search. Each returned row keeps the
source location and API surfaces beside explicit
`lab_requirement_id -> document_id:clause_id; title` pairs, including a bounded
normative statement preview, so a test's requirement and subsection mapping can
be read without opening the 992-entry plan. JSON summary records also retain
the exact `lab_requirement_ids` array and canonical
`standard_sections`; each summary also carries `traceability_state` so an
explicit no-standalone-surface disposition is not confused with an unclassified
row. Summary text abbreviates long API-surface prose to keep work-selection
output bounded; use `--json` (or non-summary `--compact`) for the complete prose.
Add `--summary` to show at most eight mapping rows per test; use
`--json` for every row. Trace limits the number of matched tests to five by
default; use `--limit 0` when a complete reverse lookup is intentional. Use
`search` explicitly when the handle is not known.

```powershell
python tools/query_rti_work.py trace "RTIambassador removes a regional subscription through a configured process endpoint" --summary
python tools/query_rti_work.py trace umbra-cpp-process-endpoint-regional-unsubscribe-integration --json
python tools/query_rti_work.py trace requirement-candidate-content-clauses-09-data-distribution-management-page-230-l157-48 --summary
python tools/query_rti_work.py trace hla-1516.1-2025:clause-9.7.5 --summary --compact
python tools/query_rti_work.py trace "RTIambassador delivers regional Attribute Relevance Advisory subscription transitions through a configured process endpoint" --summary --compact
```

Use `coverage --lane <exact-tag> --summary --compact` for a one-screen
requirement/section/source-location count for the active lane; unqualified
`coverage` reports the whole indexed Catch2 plan. A lane's plan-entry count can
be larger than its executable count when historical source-missing rows are
retained; the source-location line makes that distinction explicit.
The assertion line uses the indexed lane total when a focused JUnit artifact
has verified it, while an exact `trace`/`test` query reports the assertion count
recorded for that plan entry.

The current indexed baseline can always be rediscovered without a full-suite
search:

```powershell
python tools/query_rti_work.py lane process-restart-confirm-divestiture-post-confirmation-resignation --compact
python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses one post-confirmation resigned Confirm Divestiture recipient while preserving two surviving recipients" --compact
```

The installable-profile smoke is a separate packaging gate rather than a
Catch2 lane. Run it by target or label after `work` selects the process slice:

```powershell
cmake --build <build-dir> --config Debug --target umbra_test_installable_package
ctest --test-dir <build-dir> -C Debug -L installable-package --output-on-failure
cmake --build <build-dir> --config Debug --target umbra_process_boundary_junit
cmake --build <build-dir> --config Debug --target umbra_process_federation_service_probe
cmake --build <build-dir>/package-smoke-consumer --config Debug --target umbra_rti_package_process_smoke
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_timestamped_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-timestamped --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_parameterized_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-parameterized --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_connection_loss_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-connection-loss --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_connection_loss_delete_objects_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-connection-loss-delete-objects --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_object_registration_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-object-registration --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_named_registration_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-named-registration --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_attribute_update_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-attribute-update --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_directed_retraction_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-directed-retraction --output-on-failure
```

That gate builds every exported runtime target, stages the package, validates
`share/umbra_rti/umbra_rti-profile.json` and the reviewed 1516.2 resources,
then compiles and runs a clean downstream consumer against `find_package`.
The final command writes the bounded lane artifact to
`<build-dir>/compliance/process-boundary/process-boundary.xml`.
That artifact is assembled from the five independently buildable process-boundary
reports; the current merged report contains 214 emitted section-level testcases
and 5,458 assertions with zero failures/errors. It is the only path used for
JUnit lane evidence; the indexed 117 process-boundary plan rows and the captured
207 CTest registrations remain separately queryable metrics. The aggregate
umbrella registration is intentionally excluded from JUnit because its
federation-management translation unit is not an evidence source.
In the embedded profile, the `package-process` label is the downstream public
interoperability check: its client uses only installed IEEE 1516.1-2025
headers and `umbra::rti`, while the source-tree probe is explicitly retained as
private test infrastructure.  The CMake target is
`umbra_rti_package_process_smoke`; the CTest test name is
`umbra_rti_package_process_consumer`, and the stable CTest label is
`package-process`.  The timestamped projection is separately addressable as
`umbra_rti_package_process_timestamped_consumer` /
`package-process-timestamped`; it uses the same executable with a bounded
timestamped-client mode. The parameterized-envelope projection is separately
addressable as `umbra_rti_package_process_parameterized_consumer` /
`package-process-parameterized`; it resolves the Restaurant
`HLAobjectRoot.Customer` class and `TimelinessOk` parameter, then verifies the
official C++ object/parameter handles and value envelope at the receiver. The
connection-loss projection is separately
addressable as `umbra_rti_package_process_connection_loss_consumer` /
`package-process-connection-loss`; it closes the receiver transport from the
private fixture, verifies the official `connectionLost` callback through
`EvokeMultipleCallbacks`, and then proves the surviving sender can still use
the federation. The public client emits `connection-loss-ready.ok` before the
fixture applies the close, so this lane has a bounded handshake rather than a
timeout-based race. Use the indexed package handles or run
`ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L
package-process-connection-loss --output-on-failure` directly. The
object-registration projection is separately addressable as
`umbra_rti_package_process_object_registration_consumer` /
`package-process-object-registration`; it resolves the official object and
attribute handles, publishes `HLAprivilegeToDeleteObject`, registers an
unnamed object, and uses `DELETE_OBJECTS` on resignation while a second
federate remains joined. The `work --summary --compact` output exposes the
probe, target, ordinary test, timestamped test, parameterized test,
connection-loss test, object-registration test, named-registration test,
ordinary attribute-update test, and labels separately so they
can be copied without searching.
The named-registration projection is separately addressable as
`umbra_rti_package_process_named_registration_consumer` /
`package-process-named-registration`; it reserves a legal name through the
official callback surface, registers the named object, and checks duplicate
registration (`ObjectInstanceNameInUse`) plus illegal reservation (`IllegalName`)
through the installed public API.
The attribute-update projection uses the official installed
`RTIambassador::subscribeObjectClassAttributes`,
`RTIambassador::updateAttributeValues`, and
`FederateAmbassador::reflectAttributeValues` surfaces, with automatic
process-backed `FederateAmbassador::discoverObjectInstance` delivery. The
public process endpoint now has directly queryable region lifecycle,
regional-registration, remote regional subscription/update, and timestamped
regional update/reflect cases. The
regional-registration case covers dimension lookup/upper-bound, region create,
dimension-set/range-bound support lookups, set/commit, and the
registration-established attribute/region map. The remote case covers the
two-federate subscription/update handoff and sent-region callback metadata;
the timestamped case preserves logical time through the official callback while
scope/advisory projection remains separate follow-on work. Query the exact case
before changing that seam. The process Catch2 case and installed package projection both map
duplicate named registration to `ObjectInstanceNameInUse` and invalid
reservation input to `IllegalName`; query either exact lane before changing
the implementation.

The latest completed regional provider-response slice is also directly
indexed. It proves that a delivered receive-order response is not replayed by
a fresh-registry restore, while a later request still delivers normally under
both callback models:

```powershell
python tools/query_rti_work.py lane public-durable-save-regional-pending-attribute-value-update-regular-response-no-replay --compact
python tools/query_rti_work.py test "Embedded public fresh-registry restore does not replay a delivered receive-order regional provider response under HLA_EVOKED and HLA_IMMEDIATE" --compact
```

The completed companion that combined all three routes is directly queryable:

```powershell
python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds an eligible ownership-assumption recipient beside two Confirm Divestiture notifications" --compact
```

The completed public timestamped directed-interaction and object-deletion
save/restore slices are directly queryable regression baselines. Keep these
handles available while the active slice advances; no repository-wide search
is needed:

```powershell
python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
python tools/query_rti_work.py test "Embedded public fresh-registry restore delivers eligible directed TSO and issues Request Retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-post-delivery-resignation --compact
python tools/query_rti_work.py test "Embedded public fresh-registry restore keeps an ownership-qualified directed TSO route after a peer's post-delivery resignation under HLA_EVOKED and HLA_IMMEDIATE" --compact
python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-alternate-advances --compact
python tools/query_rti_work.py test "Embedded public fresh-registry directed TSO restores through FQR TARA and NMRA under HLA_EVOKED and HLA_IMMEDIATE" --compact
```

The focused embedded explicit-source regional TSO restore row is independently
queryable at its dedicated C++ source. It records 78 HLA_EVOKED assertions,
including the restored source-region designator, reflection-before-grant order,
and the post-restore retraction callback:

```powershell
python tools/query_rti_work.py trace "Embedded federation restore restores a saved live explicit-source regional timestamped attribute update" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded federation restore restores a saved live explicit-source regional timestamped attribute update" --output-on-failure
```

The timed explicit-source regional attribute-update restore companion is
source-backed at `cpp/tests/timed_restore_live_tso_regional_attribute_update_catch2.cpp:190`.
It records 84 HLA_EVOKED assertions across 23 Requirements-Lab anchors, 15
canonical 2025 sections, and 23 official C++ API surfaces. A timestamp-8
overlap-qualified update crosses the logical-time-6 save boundary, restores
its source-region association and retraction ledger, and is reflected at
actual time 7 with optimistic time 8. Use the exact indexed lane handles:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-live-restore-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-live-restore-state --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-live-restore-state --summary --compact
cmake --build .build --config Debug --target umbra_timed_restore_regional_attr_catch2
ctest --test-dir .build -C Debug -R "^umbra\\.timed_restore_live_tso_regional_attribute_update\\.catch2\\.Embedded timed federation restore restores a live explicit-source regional timestamped attribute update at the save boundary$" --output-on-failure
```

The timed explicit-source regional attribute-update fan-out companion is
source-backed at
`cpp/tests/timed_restore_live_tso_regional_attribute_update_multi_recipient_catch2.cpp:190`.
It records 157 HLA_EVOKED assertions across 23 Requirements-Lab anchors, 15
canonical 2025 sections, and 23 official C++ API surfaces. The same timestamp-8
passel is restored into two constrained recipient ledgers, each released by an
independent Flush Queue Request at actual time 7 with optimistic time 8, before
one Request Retraction reaches both recipients:

```powershell
python tools/query_rti_work.py focus timestamped-regional-attribute-timed-restore-multi-recipient --summary --compact
python tools/query_rti_work.py trace "Embedded timed federation restore restores one queued explicit-source regional timestamped attribute update to multiple recipients" --summary --compact
python tools/query_rti_work.py matrix "Embedded timed federation restore restores one queued explicit-source regional timestamped attribute update to multiple recipients" --summary --compact
python tools/query_rti_work.py check --lane timestamped-regional-attribute-timed-restore-multi-recipient --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed federation restore restores one queued explicit-source regional timestamped attribute update to multiple recipients$" --output-on-failure
```

The timed source-resignation companion is independently indexed at
`cpp/tests/timed_live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp:180`.
It records 106 HLA_EVOKED assertions across 27 Requirements-Lab anchors, 18
canonical 2025 sections, and 23 official C++ API surfaces. It preserves one
timestamp-8 explicit-source passel through the logical-time-6 save, source
mutation before and after restore, producer resignation, and independent
survivor Flush Queue delivery:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-live-resignation-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
python tools/query_rti_work.py matrix "Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-live-resignation-state --summary --compact
cmake --build .build --config Debug --target umbra_timed_regional_attr_source_resign_restore_catch2
ctest --test-dir .build -C Debug -R "^umbra\\.timed_live_tso_regional_attribute_update_source_resignation_after_restore\\.catch2\\.Embedded timed live regional timestamped attribute update survives source mutation and resignation after restore$" --output-on-failure
```

The four-member timed multi-recipient source-resignation companion is
source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_source_resignation_after_restore_catch2.cpp:190`.
It records 167 HLA_EVOKED assertions across 27 Requirements-Lab anchors, 18
canonical 2025 sections, and 22 official C++ API surfaces. The producer's
timestamp-8 explicit-source passel survives source-region mutation and
unconditional-divestiture resignation; an independent regulating clock lets
each constrained recipient flush its restored copy:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-multi-resignation-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-multi-resignation-state --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives source mutation and resignation after restore$" --output-on-failure
```

The four-member timed delete-then-divest companion is source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_delete_then_divest_after_restore_catch2.cpp:218`.
It records 124 HLA_EVOKED assertions across 36 Requirements-Lab anchors, 19
canonical 2025 sections, and 25 official C++ API surfaces. It restores the
saved source region, applies the disjoint post-restore mutation, then proves
that `DELETE_OBJECTS_THEN_DIVEST` suppresses the stale timestamp-8 reflection
and delivers one receive-order removal per constrained recipient at each
independent Flush Queue boundary:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-delete-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update is suppressed after delete-then-divest resignation following restore" --summary --compact
python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update is suppressed after delete-then-divest resignation following restore" --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-delete-state --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update is suppressed after delete-then-divest resignation following restore$" --output-on-failure
```

The matching four-member timed cancel-then-delete-then-divest companion is
source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_cancel_then_delete_then_divest_after_restore_catch2.cpp:218`.
It records 124 HLA_EVOKED assertions across 36 Requirements-Lab anchors, 19
canonical 2025 sections, and 25 official C++ API surfaces. It restores the
saved explicit-source passel after source-region mutation, applies
`CANCEL_THEN_DELETE_THEN_DIVEST`, and verifies stale timestamped reflection is
suppressed while each constrained recipient receives one removal before its
own grant:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-cancel-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update is suppressed after cancel-then-delete-then-divest resignation following restore" --summary --compact
python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update is suppressed after cancel-then-delete-then-divest resignation following restore" --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-cancel-state --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "^umbra\\.ieee1516_2025\\.catch2\\.Embedded timed multi-recipient regional timestamped attribute update is suppressed after cancel-then-delete-then-divest resignation following restore$" --output-on-failure
```

The pending-ownership cancellation companion is source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_cancel_pending_ownership_after_restore_catch2.cpp:251`.
It records 130 HLA_EVOKED assertions across 39 Requirements-Lab anchors, 21
canonical 2025 sections, and 28 official C++ API surfaces. It queues a regular
ownership acquisition after restore, then consumes the owner's queued release
request as stale work when the requester resigns with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`; the surviving recipient still gets
one saved reflection before its grant:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-cancel-pending-ownership-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending ownership acquisition after restore" --summary --compact
python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending ownership acquisition after restore" --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-cancel-pending-ownership-state --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending ownership acquisition after restore$" --output-on-failure
```

The If Available pending-ownership cancellation companion is source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_if_available_cancel_pending_ownership_after_restore_catch2.cpp:251`.
It records 130 HLA_EVOKED assertions across 41 Requirements-Lab anchors, 22
canonical 2025 sections, and 29 official C++ API surfaces. It queues an If
Available acquisition after restore, retains only private Willing-to-Acquire
state without an owner-release callback, and consumes the requester resignation
fence before stale callback/reflection delivery; the surviving recipient still
receives one saved reflection before its grant:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-if-available-cancel-pending-ownership-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending If Available ownership acquisition after restore" --summary --compact
python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending If Available ownership acquisition after restore" --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-if-available-cancel-pending-ownership-state --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending If Available ownership acquisition after restore$" --output-on-failure
```

The negotiated regular-candidate continuation companion is source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_lane_catch2.cpp:11`.
It records 164 HLA_EVOKED assertions across 46 Requirements-Lab anchors, 24
canonical 2025 sections, and 30 official C++ API surfaces. It keeps a regular
request on the first constrained recipient and a second regular candidate on
the independent clock, consumes the first candidate's stale negotiated path
after `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, and completes the retained
candidate only after the surviving recipient's saved reflection:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
cmake --build <build-dir> --config Release --target umbra_tso_regional_regular_continuation_restore_catch2
ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore$" --output-on-failure
```

The mixed If Available-to-regular continuation companion is source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_if_available_regular_candidate_continuation_after_restore_catch2.cpp:307`.
It records 163 HLA_EVOKED assertions across 48 Requirements-Lab anchors, 25
canonical 2025 sections, and 31 official C++ API surfaces. The first constrained
recipient queues If Available while the independent clock queues a regular
candidate; requester cancellation, reissued negotiation, and Confirm
Divestiture leave only the retained regular candidate and preserve the saved
regional reflection:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-if-available-regular-candidate-continuation-state --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from an If Available request to a regular candidate after restore" --summary --compact
python tools/query_rti_work.py matrix "Embedded timed multi-recipient regional timestamped attribute update continues from an If Available request to a regular candidate after restore" --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-if-available-regular-candidate-continuation-state --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from an If Available request to a regular candidate after restore$" --output-on-failure
```

The Attribute Scope Advisory slice is independently queryable through
`object-attribute-scope-advisory-state`. Its source is
`cpp/tests/attribute_scope_advisory_catch2.cpp:97`; both callback models pass
with 187 assertions, nine Requirements-Lab anchors, five canonical 2025
sections, and eleven official C++ API surfaces. The case groups two attributes
per transition and covers region, association, ordinary/regional subscription,
stale evoked, switch-gate, and default-region boundaries:

```powershell
python tools/query_rti_work.py focus object-attribute-scope-advisory-state --summary --compact
python tools/query_rti_work.py trace "Embedded regional object scope callbacks follow 2025 region, association, and subscription changes" --summary --compact
python tools/query_rti_work.py matrix "Embedded regional object scope callbacks follow 2025 region, association, and subscription changes" --summary --compact
python tools/query_rti_work.py check --lane object-attribute-scope-advisory-state --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded regional object scope callbacks follow 2025 region, association, and subscription changes$" --output-on-failure
```

Its three-federate source-resignation companion is also independently indexed.
It records 91 HLA_EVOKED assertions across 20 Requirements-Lab anchors, 18
canonical 2025 sections, and 23 official C++ API surfaces. The case keeps one
explicit-source timestamped passel queued through an untimed save, mutates and
restores the source region, resigns the producer with unconditional divestiture,
and proves the survivor's Flush Queue reflection precedes its grant with the
saved source-region and retraction metadata:

```powershell
python tools/query_rti_work.py trace "Embedded live regional timestamped attribute update survives source mutation and resignation after restore" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded live regional timestamped attribute update survives source mutation and resignation after restore" --output-on-failure
```

The timed default-region interaction companion is independently indexed at its
standalone source. It records 72 HLA_EVOKED assertions across 13
Requirements-Lab anchors, 10 canonical 2025 sections, and 24 official C++ API
surfaces. The timestamp-8 default-source passel crosses a timestamp-6 save
boundary, is restored after a post-save Retract, and is delivered by Flush
Queue with the supplied-empty callback RegionHandleSet before the grant:

```powershell
python tools/query_rti_work.py trace "Embedded timed federation restore restores a live timestamped default-region interaction at the save boundary" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded timed federation restore restores a live timestamped default-region interaction at the save boundary" --output-on-failure
ctest --test-dir out/cmake/fom-services -C Debug -L timestamped-default-region-interaction-timed-restore --output-on-failure
```

The timed directed-interaction companion is independently indexed at its
standalone source. It records 72 HLA_EVOKED assertions across 10
Requirements-Lab anchors, 8 canonical 2025 sections, and 21 official C++ API
surfaces. A target-qualified timestamp-8 directed interaction crosses the
timestamp-6 save boundary, is restored after a post-save Retract, and is
delivered by Flush Queue before the grant with target and retraction metadata:

```powershell
python tools/query_rti_work.py trace "Embedded timed federation restore restores a live timestamped directed interaction at the save boundary" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded timed federation restore restores a live timestamped directed interaction at the save boundary" --output-on-failure
ctest --test-dir out/cmake/fom-services -C Debug -L timestamped-directed-interaction-timed-restore --output-on-failure
```

The timed default-region attribute-update companion is independently indexed at
its standalone source. It records 76 HLA_EVOKED assertions across 16
Requirements-Lab anchors, 10 canonical 2025 sections, and 23 official C++ API
surfaces. A default-source timestamp-8 Update Attribute Values passel crosses
the timestamp-6 save boundary, is restored after a post-save Retract, and is
delivered by Flush Queue before the grant with supplied-empty sent-region
metadata:

```powershell
python tools/query_rti_work.py trace "Embedded timed federation restore restores a live timestamped default-region attribute update at the save boundary" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded timed federation restore restores a live timestamped default-region attribute update at the save boundary" --output-on-failure
ctest --test-dir out/cmake/fom-services -C Debug -L timestamped-default-region-attribute-timed-restore --output-on-failure
```

The untimed default-region attribute-update companion is independently indexed
at `cpp/tests/restore_live_tso_default_region_attribute_update_catch2.cpp:165`.
It records 74 HLA_EVOKED assertions across 18 Requirements-Lab anchors, 14
canonical 2025 sections, and 23 official C++ API surfaces. A queued timestamp-7
default-source Update Attribute Values passel crosses an untimed save, is
restored after a post-save Retract, and is delivered by Flush Queue before the
grant with supplied-empty sent-region metadata:

```powershell
python tools/query_rti_work.py trace "Embedded federation restore restores a saved live timestamped default-region attribute update" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded federation restore restores a saved live timestamped default-region attribute update" --output-on-failure
ctest --test-dir out/cmake/fom-services -C Debug -L timestamped-default-region-attribute-restore --output-on-failure
python tools/query_rti_work.py check --lane timestamped-default-region-attribute-restore --summary --compact
```

The non-regional timestamped attribute-update restore companion is independently
indexed at `cpp/tests/restore_live_tso_attribute_update_catch2.cpp:165` with 69
HLA_EVOKED assertions, 15 Requirements-Lab anchors, 11 canonical 2025 sections,
and 20 official C++ API surfaces. It uses ordinary subscription and explicit
timestamped default-order selection, then proves the untimed save/restore
ledger and reflection-before-grant delivery:

```powershell
python tools/query_rti_work.py trace "Embedded federation restore restores a saved live timestamped attribute update" --summary --compact
ctest --test-dir out/cmake/fom-services -C Debug -R "Embedded federation restore restores a saved live timestamped attribute update" --output-on-failure
ctest --test-dir out/cmake/fom-services -C Debug -L timestamped-attribute-update-restore --output-on-failure
python tools/query_rti_work.py check --lane timestamped-attribute-update-restore --summary --compact
```

The public FQR/TARA/NMRA save/restore case is now green under both
HLA_EVOKED and HLA_IMMEDIATE (462 assertions) and remains directly queryable
as one callback-model comparison. The parameterized directed-TSO save/restore
projection case is green under both callback models (202 assertions), the
fan-out/retraction save/restore case is green under both callback models (302
assertions), and the post-delivery-resignation case is green under both models
(300 assertions). The eligible directed-TSO delivery/retraction case is green
under both callback models (220 assertions). The fresh-registry regional timestamped-interaction DDM
companion is now green under HLA_EVOKED and HLA_IMMEDIATE (315 assertions),
preserving its source-region snapshot, recipient-specific queue entries, and
immutable filesystem report-file identity. The regional timestamped-
attribute-update DDM companion is now green under both callback models (375
assertions), preserving source-region snapshots, recipient-specific delivery,
valid retraction metadata, and filesystem report-file identity. The timestamped
object-deletion save/restore and delivered-retraction companions are green at
321 and 242 assertions. The mixed interaction-declaration, transportation
override, mixed-override, and directed-declaration companions are green under
both callback models (146, 186, 176, and 172 assertions respectively). The
directed target-routing, ownership-handoff, directed TSO ownership-callback,
regional multi-source Auto Provide, regular ownership-release, pending If
Available ownership-callback, pending negotiated owner-confirmation, pending
negotiated If Available owner-confirmation, mixed negotiated ownership,
delivered negotiated owner-confirmation, delivered negotiated If Available, and
mixed delivered negotiated-confirmation, asymmetric mixed negotiated-
confirmation, and reverse asymmetric mixed negotiated-confirmation companions
are green under both callback models (216, 264, 282, 144, 178, 193, 223, 207,
242, 228, 223, 273, 274, 274, 220, 196, 136, 150, and 142 assertions). The
pending attribute-transportation-type-change, pending interaction-
transportation-type-change, and all three Query Attribute Ownership companions
are now complete under both callback models. The regional Auto Provide
baseline, HLA_IMMEDIATE ordering, receive-order response, timestamped
response/retraction, timestamped switch-mutation (206), multi-provider (196),
switch-mutation (224), switch-admission (114), timestamped switch-admission
(177), relaxed-DDM (240), independent source-region (144), TSO
retraction-designator uniqueness (71), and directed TSO multi-recipient restore
(116), directed TSO single-recipient restore (74), and default-region
multi-recipient TSO attribute restore (123), timed explicit-source regional
multi-recipient TSO attribute restore (147), timestamped directed-interaction
subscription-kind (117), and timestamped directed target-departure (121)
slices are now green and recorded in the completion ledger. The time-regulated
timestamped directed-interaction MOM-report companion is also green (124
assertions), and its paired production-filesystem report companion is green
(236 assertions), and the non-time-regulating timestamped filesystem report
companion is green (163 assertions), the receive-order Send Interaction
filesystem case is green at 145 assertions, its MOM-interaction companion is
green at 85 assertions, the timestamped Send Interaction filesystem/MOM pair is
green at 153/91 assertions, the time-regulated timestamped Send Interaction
MOM companion is green at 115 assertions, and the receive-order Send Directed
Interaction filesystem case is green at 158 assertions. The time-regulated
timestamped Update Attribute Values MOM companion is now green at 119
assertions. The immediate-only timestamped Update Attribute Values retraction
case is green at 69 assertions, the mixed-fanout companion is green at 75
assertions, the suppressed timestamped interaction callback is green at 31
assertions, and the suppressed timestamped attribute callback is green at 33
assertions. The mixed regional timestamped attribute delivery and pre-callback
retraction companions are green at 131 and 88 assertions. The ordinary
regional TAR/NMR frontier companion is green at 94 assertions, and the
recipient-gated mixed-fanout companion is green at 114. The timestamped
attribute available/next-message-available frontier companion is now green at
57 assertions. The timestamped attribute Flush Queue Request passel companion
is now green at 63 assertions, and its future-input companion is green at 48
assertions. The terminal timestamped-deletion tombstone case is now green at 24
assertions. The timestamped Delete Object Instance TAR/NMR companion is green at
75 assertions, and the public durable-save regional provider-response/retraction
companion is green at 100 assertions under both callback models. All indexed plan
entries now have explicit status; retained historical rows may still lack derived
source locations. The public process callback surface now covers ordinary and
timestamped Receive/Evoke, Evoke Multiple, and EVOKED callback enable/disable
gating, plus object-class publication, unnamed and reservation-consuming named
object registration/reservation callbacks, and ordinary Update Attribute
Values/Reflect delivery; the
focused Catch2 case remains private foundation evidence while the
separate installed-package smoke is public-surface foundation evidence. The
thirteen installed-profile process lanes, named-registration semantics, and the
public regional lifecycle/registration, remote regional subscription/update, and
timestamped regional update/reflect, regional unsubscription suppression, and
disjoint suppression cases are now protected by their reproducible
CTest/Catch2/JUnit/configuration mapping gate. The private m24 and public m25
Attribute Relevance Advisory projections are now indexed and green under both
callback models, m26 covers callback-entry suppression of a stale evoked
advisory after switch disable, m27 covers public regional association
transitions, m28 covers public regional subscription removal/restoration, and
m29 covers the initial regional registration/discovery advisory under both
callback models; embedded m31 covers active update-rate reissue and m32 covers
regional explicit-rate retention; public m33 covers the active-rate reissue
variant and public m34 covers the regional explicit-rate designator variant;
embedded m35 covers callback-entry rechecking after a queued regional scope
transition; the next bounded task is the remaining
regional advisory variants.
`coverage --lane transport` reports 82 plan entries (76 mapped, 82
source-located, and no source-drift rows; 6 have no Lab requirement mapping).
Keep its evidence separate from
the completed Flush Queue, tombstone, available-advance, mixed-fanout, TAR/NMR,
suppression, retraction, service-report, Send Interaction, directed,
timestamped-directed, target-departure, subscription-kind, save/restore, and
other regional lanes:

The next gate's evidence contract is recorded in
`docs/planning/TRANSPORT-CONFORMANCE-GATE.md`.

```powershell
python tools/query_rti_work.py lane process-boundary --summary --compact
python tools/query_rti_work.py test "Private process service dispatch correlates federation operations over framed data" --summary --compact
python tools/query_rti_work.py test "Private process service binds create join and receive-order interaction to the federation registry" --summary --compact
python tools/query_rti_work.py test "Private registry-bound service exchanges federation traffic across independently launched processes" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Create, Join, and Resign through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador resolves interaction and parameter handles through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador receives a process interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py test "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py test "RTIambassador selects a configured tcp process endpoint through the official address field" --summary --compact
python tools/query_rti_work.py test "RTIambassador rejects malformed tcp process addresses before connecting" --summary --compact
python tools/query_rti_work.py test "Embedded transport loss applies the bounded automatic NoAction forced-resign policy" --summary --compact
python tools/query_rti_work.py next --pointer
python tools/query_rti_work.py test "Embedded terminal timestamped deletion tombstone releases its object name" --summary --compact
python tools/query_rti_work.py test "Embedded Flush Queue Request admits TSO input queued after submission" --summary --compact
python tools/query_rti_work.py test "Embedded timestamped Update Attribute Values flushes queued passels with optimistic time" --compact
python tools/query_rti_work.py test "Embedded timestamped Update Attribute Values honors available and next-message available grants" --compact
python tools/query_rti_work.py test "Embedded timestamped regional Update Attribute Values carries recipient-gated regions across mixed fanout" --compact
python tools/query_rti_work.py test "Embedded regional timestamped attribute updates deliver before TAR and NMR grants" --compact
python tools/query_rti_work.py section hla-1516.1-2025:clause-8.22.3 --summary --limit 20
```

The latest source-backed object/time/transport/regional-DDM slices are directly queryable
from the bounded completion ledger:

```powershell
python tools/query_rti_work.py recent --summary --compact --limit 10
python tools/query_rti_work.py test "Embedded object-class Request Attribute Value Update supports a timestamped provider response under HLA_EVOKED and HLA_IMMEDIATE" --compact
python tools/query_rti_work.py test "Embedded transport loss drains a cutoff timestamped deletion to each pending survivor" --compact
python tools/query_rti_work.py test "Embedded local deletion isolates queued timestamped attribute deliveries per recipient" --compact
python tools/query_rti_work.py test "Embedded local deletion isolates queued timestamped object removals per recipient" --compact
python tools/query_rti_work.py trace "Embedded local deletion suppresses a queued timestamped attribute reflection" --summary --compact
python tools/query_rti_work.py check --lane local-delete-timestamped-attribute-suppression --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.local_delete_timestamped_attribute\\.catch2\\.Embedded local deletion suppresses a queued timestamped attribute reflection$" --output-on-failure
python tools/query_rti_work.py test "Embedded regional Auto Provide timestamped response reflects once and exposes valid retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
```

The accepted-TSO ownership-transfer slice is also independently buildable and
has a unique lane handle. It records 60 HLA_EVOKED assertions across nine
Requirements-Lab anchors and four canonical 2025 sections. The producer's
timestamped Update Attribute Values passel remains queued while the source
owner divests and a new federate completes If Available acquisition; the
recipient then reflects the original producer, payload, timestamp/order, and
retraction before its constrained grant:

```powershell
python tools/query_rti_work.py focus timestamped-attribute-update-ownership-transfer --summary --compact
python tools/query_rti_work.py trace "Embedded accepted timestamped attribute update survives ownership transfer before constrained grant" --summary --compact
python tools/query_rti_work.py matrix "Embedded accepted timestamped attribute update survives ownership transfer before constrained grant" --summary --compact
python tools/query_rti_work.py check --lane timestamped-attribute-update-ownership-transfer --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timestamped_attribute_update_ownership_transfer\\.catch2\\.Embedded accepted timestamped attribute update survives ownership transfer before constrained grant$" --output-on-failure
```

The declared custom-transportation timestamped-delivery case is independently
buildable as a deliberate API-traceability disposition. It records 48
HLA_EVOKED assertions, has no standalone Lab requirement candidate, and keeps
the absence of normative mapping explicit while proving both interaction and
attribute timestamped callbacks preserve the user-declared transportation:

```powershell
python tools/query_rti_work.py focus custom-transportation-timestamped-delivery --summary --compact
python tools/query_rti_work.py trace "Embedded timestamped delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py matrix "Embedded timestamped delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py check --lane custom-transportation-timestamped-delivery --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.custom_transportation_timestamped_delivery\\.catch2\\.Embedded timestamped delivery accepts a declared custom FOM transportation$" --output-on-failure
```

The directed variant is indexed beside it as a separate 42-assertion
HLA_EVOKED case. It keeps the same explicit no-standalone-Lab-requirement
disposition while exercising target routing, timestamp/order, retraction, and
custom transportation preservation through Receive Directed Interaction:

```powershell
python tools/query_rti_work.py focus custom-transportation-timestamped-directed-delivery --summary --compact
python tools/query_rti_work.py trace "Embedded timestamped directed delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py matrix "Embedded timestamped directed delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py check --lane custom-transportation-timestamped-directed-delivery --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.custom_transportation_timestamped_directed_delivery\\.catch2\\.Embedded timestamped directed delivery accepts a declared custom FOM transportation$" --output-on-failure
```

The regional-attribute companion is independently buildable at
`custom_transportation_timestamped_regional_attribute_delivery_catch2.cpp:163`.
It records 60 HLA_EVOKED assertions and keeps the same explicit
no-standalone-Lab-requirement disposition while proving DDM region designator
propagation, timestamp/order, retraction, and declared custom transportation
through the timestamped attribute callback:

```powershell
python tools/query_rti_work.py focus custom-transportation-timestamped-regional-attribute-delivery --summary --compact
python tools/query_rti_work.py trace "Embedded timestamped regional attribute delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py matrix "Embedded timestamped regional attribute delivery accepts a declared custom FOM transportation" --summary --compact
python tools/query_rti_work.py check --lane custom-transportation-timestamped-regional-attribute-delivery --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.custom_transportation_timestamped_regional_attribute_delivery\\.catch2\\.Embedded timestamped regional attribute delivery accepts a declared custom FOM transportation$" --output-on-failure
```

The MOM transportation-type request slice is independently buildable at
`mom_transportation_type_change_request_catch2.cpp:189`. It records 168
assertions across both HLA_EVOKED and HLA_IMMEDIATE, maps to the six canonical
2025 sections 6.25.1, 6.26, 6.27, 6.30.3, 6.31.3, and 11.5, and checks the
official MIM request payloads, callback confirmations, malformed-handle
failures, invoker-local behavior, and the final service-report interaction:

```powershell
python tools/query_rti_work.py focus mom-transportation-type-change-request --summary --compact
python tools/query_rti_work.py trace "Embedded MOM transportation-type request interactions invoke public changes" --summary --compact
python tools/query_rti_work.py matrix "Embedded MOM transportation-type request interactions invoke public changes" --summary --compact
python tools/query_rti_work.py check --lane mom-transportation-type-change-request --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.mom_transportation_type_change_request\\.catch2\\.Embedded MOM transportation-type request interactions invoke public changes$" --output-on-failure
```

The multi-recipient timestamped directed-interaction source-resignation slice
is independently buildable at
`timestamped_directed_interaction_multi_recipient_source_resignation_catch2.cpp:172`.
It passes 113 HLA_EVOKED assertions, maps seven Requirements-Lab candidates to
canonical 2025 clauses 4.12, 5.1.5, 8.1.5, 8.1.6, and 8.8.3, and drains the
FQR/TARA/NMRA recipients independently after NO_ACTION source resignation. It
preserves target/producer/tag/time/order/transport/retraction metadata and
checks the post-resignation `FederateNotExecutionMember` boundary:

```powershell
python tools/query_rti_work.py focus timestamped-directed-interaction-multi-recipient-source-resignation --summary --compact
python tools/query_rti_work.py trace "Embedded queued timestamped directed interaction survives source resignation for each recipient" --summary --compact
python tools/query_rti_work.py matrix "Embedded queued timestamped directed interaction survives source resignation for each recipient" --summary --compact
python tools/query_rti_work.py check --lane timestamped-directed-interaction-multi-recipient-source-resignation --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timestamped_directed_interaction_multi_recipient_source_resignation\\.catch2\\.Embedded queued timestamped directed interaction survives source resignation for each recipient$" --output-on-failure
```

The direct TAR/NMR timestamped directed-interaction frontier is independently
buildable at `timestamped_directed_interaction_tar_nmr_catch2.cpp:150`. It
passes 79 HLA_EVOKED assertions, maps seven Requirements-Lab candidates to
canonical 2025 clauses 5.1.5, 8.1.5, 8.1.6, 8.8.3, and 8.22.3, and proves that
two constrained recipients receive the target-qualified payload before their
TAR(7) and NMR(10) grants. It preserves target/producer/tag/time/order/
transport/retraction metadata and keeps post-delivery Retract terminal:

```powershell
python tools/query_rti_work.py focus timestamped-directed-interaction-tar-nmr --summary --compact
python tools/query_rti_work.py trace "Embedded timestamped directed interaction delivers before TAR and NMR grants" --summary --compact
python tools/query_rti_work.py matrix "Embedded timestamped directed interaction delivers before TAR and NMR grants" --summary --compact
python tools/query_rti_work.py check --lane timestamped-directed-interaction-tar-nmr --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timestamped_directed_interaction_tar_nmr\\.catch2\\.Embedded timestamped directed interaction delivers before TAR and NMR grants$" --output-on-failure
```

The federation-scoped MOM save-conditionals slice is independently buildable
at `federation_mom_save_conditionals_catch2.cpp:183`. It passes 95
HLA_EVOKED assertions, maps six Requirements-Lab candidates to canonical 2025
clauses 4.19, 4.19.6, 4.20, and 11.4.1, and checks empty initial
HLAnextSaveName/Time and HLAlastSaveName/Time, pending timestamped save
encoding, clear-on-admission, and last-on-completion reflection:

```powershell
python tools/query_rti_work.py focus federation-mom-save-conditionals --summary --compact
python tools/query_rti_work.py trace "Embedded federation MOM save conditionals follow pending admission and completion" --summary --compact
python tools/query_rti_work.py matrix "Embedded federation MOM save conditionals follow pending admission and completion" --summary --compact
python tools/query_rti_work.py check --lane federation-mom-save-conditionals --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.federation_mom_save_conditionals\\.catch2\\.Embedded federation MOM save conditionals follow pending admission and completion$" --output-on-failure
```

The joined-federate MOM GALT/LITS projection is independently buildable at
`joined_federate_mom_galt_lits_periodic_catch2.cpp:138`. It passes 58
HLA_EVOKED assertions, maps the MOM projection to canonical 2025 clause 11.4.1,
and checks direct Query GALT/Query LITS agreement, one HLAsetTiming periodic
reflection, and the official empty-array undefined form after disabling the
sole regulator:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-galt-lits-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes federation GALT and LITS" --summary --compact
python tools/query_rti_work.py matrix "Embedded joined-federate MOM exposes federation GALT and LITS" --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-galt-lits-periodic --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_galt_lits_periodic\\.catch2\\.Embedded joined-federate MOM exposes federation GALT and LITS$" --output-on-failure
```

The joined-federate MOM queued-TSO-length projection is independently buildable
at `joined_federate_mom_tso_length_periodic_catch2.cpp:184`. It passes 65
HLA_EVOKED assertions, maps the MOM projection to canonical 2025 clause 11.4.1,
and checks HLATSOlength=1 for a pending timestamped interaction, one
HLAsetTiming periodic reflection, and HLATSOlength=0 after the matching grant:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-tso-length-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes queued TSO length" --summary --compact
python tools/query_rti_work.py matrix "Embedded joined-federate MOM exposes queued TSO length" --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-tso-length-periodic --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_tso_length_periodic\\.catch2\\.Embedded joined-federate MOM exposes queued TSO length$" --output-on-failure
```

The joined-federate MOM time-state duration projection is independently
buildable at `joined_federate_mom_time_state_duration_catch2.cpp:120`. It passes
192 assertions across HLA_EVOKED and HLA_IMMEDIATE, maps to canonical 2025
clause 11.4.1, and checks direct HLAinteger32BE HLAtimeGrantedTime and
HLAtimeAdvancingTime reads, one HLAsetTiming periodic reflection, and the
non-consuming direct versus consume-once periodic interval boundary:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-time-state-duration --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes time-state durations directly and periodically" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-time-state-duration --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-time-state-duration --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_time_state_duration\\.catch2\\.Embedded joined-federate MOM exposes time-state durations directly and periodically$" --output-on-failure
```

The joined-federate MOM reflection-count projection is independently buildable
at `joined_federate_mom_reflection_count_catch2.cpp:140`. It passes 103
HLA_EVOKED assertions, maps to canonical 2025 clause 11.4.1, and checks the
distinct-object sequence 0/1/1/2 against callback-invocation totals 0/1/2/3,
then 2/4 after a queued timestamped reflection and one periodic HLAsetTiming
reflection:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-reflection-counts --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM separates reflection totals from distinct objects" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-reflection-counts --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-reflection-counts --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_reflection_count\\.catch2\\.Embedded joined-federate MOM separates reflection totals from distinct objects$" --output-on-failure
```

The joined-federate MOM updates-sent projection is independently buildable at
`joined_federate_mom_updates_sent_catch2.cpp:145`. It passes 98
HLA_EVOKED assertions, maps to canonical 2025 clause 11.4.1, and records two
best-effort `Server` updates plus one reliable `Soda` update. It decodes the
official `HLAtransportation` and nested `HLAobjectClassBasedCounts` parameters,
checks callback-gated RTI-originated metadata, and verifies empty NULL arrays
for both transportation buckets when the requested federate has no updates:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-updates-sent --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestUpdatesSent reports class and transportation counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-updates-sent --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-updates-sent --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_updates_sent\\.catch2\\.Embedded MOM requestUpdatesSent reports class and transportation counts$" --output-on-failure
```

The joined-federate MOM interactions-sent projection is independently
buildable at `joined_federate_mom_interactions_sent_catch2.cpp:153`. It passes
107 HLA_EVOKED assertions, maps to canonical 2025 clause 11.4.1, and records
one reliable and one best-effort `TakeOrder` plus one reliable regional
`MainCourseServed` send. It decodes the official `HLAtransportation` and
nested `HLAinteractionCounts` parameters, checks callback-gated RTI-originated
metadata, and verifies empty NULL arrays for both transportation buckets when
the requested federate has no sends:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-interactions-sent --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestInteractionsSent reports class and transportation counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-interactions-sent --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-interactions-sent --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_interactions_sent\\.catch2\\.Embedded MOM requestInteractionsSent reports class and transportation counts$" --output-on-failure
```

The joined-federate MOM removed-object history/retraction slice is independently
buildable at
`joined_federate_mom_removed_object_count_tso_retraction_catch2.cpp:197`.
It passes 89 HLA_EVOKED assertions, maps nine Requirements-Lab candidates to
canonical 2025 clauses 6.16, 6.17.1, 8.22.3, 8.23.3, and 11.4.1, and checks
that the receiving federate's HLAobjectInstancesRemoved count advances at a
delivered timestamped removal, remains historical after legal Retract, and
does not fan out to the still-pending constrained recipient:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-removed-object-count-tso-retraction --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAobjectInstancesRemoved preserves history after timestamped retraction" --summary --compact
python tools/query_rti_work.py matrix "Embedded joined-federate MOM HLAobjectInstancesRemoved preserves history after timestamped retraction" --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-removed-object-count-tso-retraction --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_removed_object_count_tso_retraction\\.catch2\\.Embedded joined-federate MOM HLAobjectInstancesRemoved preserves history after timestamped retraction$" --output-on-failure
```

The TSO retraction-lifetime slice is independently buildable at
`timestamped_interaction_regulation_reenable_catch2.cpp:256`. It passes 30
HLA_EVOKED assertions, maps five Requirements-Lab candidates to four canonical
2025 §8 clauses, and keeps one `MessageRetractionHandle` across a disabled
time-regulation interval: retract is rejected while regulation is disabled,
the same handle is accepted after the callback-gated re-enable, and a second
retract reports `MessageCanNoLongerBeRetracted`:

```powershell
python tools/query_rti_work.py focus tso-retraction-disable-reenable-lifetime --summary --compact
python tools/query_rti_work.py trace "Embedded Disable Time Regulation preserves a live TSO retraction designator across re-enable" --summary --compact
python tools/query_rti_work.py matrix "Embedded Disable Time Regulation preserves a live TSO retraction designator across re-enable" --summary --compact
python tools/query_rti_work.py check --lane tso-retraction-disable-reenable-lifetime --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timestamped_interaction_regulation_reenable\\.catch2\\.Embedded Disable Time Regulation preserves a live TSO retraction designator across re-enable$" --output-on-failure
```

The no-recipient timestamped attribute-update designator slice is independently
buildable at `timestamped_attribute_update_no_fanout_catch2.cpp:54`. It passes
20 HLA_EVOKED assertions, maps four Requirements-Lab candidates to canonical
2025 clauses 6.10, 8.1.5, and 8.22.3, and proves that a qualifying TSO
`Update Attribute Values` call returns a valid designator even when no
subscriber is eligible. The first retract succeeds; a later designator is
terminalized by the producer's advance boundary:

```powershell
python tools/query_rti_work.py focus tso-attribute-update-no-fanout-designator --summary --compact
python tools/query_rti_work.py trace "Embedded timestamped Update Attribute Values returns a retraction designator without recipient fanout" --summary --compact
python tools/query_rti_work.py matrix "Embedded timestamped Update Attribute Values returns a retraction designator without recipient fanout" --summary --compact
python tools/query_rti_work.py check --lane tso-attribute-update-no-fanout-designator --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timestamped_attribute_update_no_fanout\\.catch2\\.Embedded timestamped Update Attribute Values returns a retraction designator without recipient fanout$" --output-on-failure
```

The delivered-recipient timestamped object-deletion retraction slice is
independently buildable at `timestamped_object_deletion_retraction_catch2.cpp:147`.
It passes 57 HLA_EVOKED assertions, maps eight Requirements-Lab candidates to
canonical 2025 clauses 6.16, 6.17.1, 8.22.3, and 8.23.3, and exercises the
three-federate boundary: an immediate recipient receives Remove Object
Instance, Retract restores the invocation-time object/name/ownership state and
queues Request Retraction only for that delivered recipient, and the
constrained recipient's pending removal is suppressed:

```powershell
python tools/query_rti_work.py focus tso-object-deletion-retraction-reconstitution --summary --compact
python tools/query_rti_work.py trace "Embedded Request Retraction reconstitutes a delivered timestamped object deletion" --summary --compact
python tools/query_rti_work.py matrix "Embedded Request Retraction reconstitutes a delivered timestamped object deletion" --summary --compact
python tools/query_rti_work.py check --lane tso-object-deletion-retraction-reconstitution --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timestamped_object_deletion_retraction\\.catch2\\.Embedded Request Retraction reconstitutes a delivered timestamped object deletion$" --output-on-failure
```

The joined-owner timestamped object-deletion retraction slice is independently
buildable at `timestamped_object_deletion_joined_owner_retraction_catch2.cpp:170`.
It passes 65 HLA_EVOKED assertions, maps four Requirements-Lab candidates to
canonical 2025 clauses 6.16 and 8.22.3, and proves survivor-only Request
Retraction plus `Attribute Is Not Owned` after the delivered owner resigns:

```powershell
python tools/query_rti_work.py focus tso-object-deletion-joined-owner-retraction --summary --compact
python tools/query_rti_work.py trace "Embedded Request Retraction reconstitutes timestamped deletion only for joined owners" --summary --compact
python tools/query_rti_work.py matrix tso-object-deletion-joined-owner-retraction --summary --compact
python tools/query_rti_work.py check --lane tso-object-deletion-joined-owner-retraction --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.timestamped_object_deletion_joined_owner_retraction\\.catch2\\.Embedded Request Retraction reconstitutes timestamped deletion only for joined owners$" --output-on-failure
```

The optimistic-time Flush Queue Request slice is independently buildable at
`flush_queue_request_optimistic_time_catch2.cpp:140`. It passes 58
`HLA_EVOKED` assertions, maps five Requirements-Lab candidates to canonical
2025 clauses 8.12 and 8.12.3, and proves FIFO delivery of two queued TSO
interactions before the grant, actual grant 5, optimistic time 7, and the
earlier-than-grant fence:

```powershell
python tools/query_rti_work.py focus flush-queue-request-optimistic-time --summary --compact
python tools/query_rti_work.py trace "Embedded Flush Queue Request flushes queued TSO and reports optimistic time" --summary --compact
python tools/query_rti_work.py matrix flush-queue-request-optimistic-time --summary --compact
python tools/query_rti_work.py check --lane flush-queue-request-optimistic-time --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.flush_queue_request_optimistic_time\\.catch2\\.Embedded Flush Queue Request flushes queued TSO and reports optimistic time$" --output-on-failure
```

The cross-family MOM sender-count NULL-bucket slice is independently buildable
at `mom_sender_count_reports_null_buckets_catch2.cpp:116`. It records 111
`HLA_EVOKED` assertions, maps one Requirements-Lab candidate to canonical
2025 clause 11.4.1, and requests updates-sent, interactions-sent, and
directed-interactions-sent reports for an idle joined federate. Each family
must emit reliable and best-effort buckets with official empty nested arrays;
the reports are RTI-originated and callback-gated:

```powershell
python tools/query_rti_work.py focus mom-sender-count-reports-null-buckets --summary --compact
python tools/query_rti_work.py trace "Embedded MOM sender count reports emit NULL buckets for empty ledgers" --summary --compact
python tools/query_rti_work.py matrix mom-sender-count-reports-null-buckets --summary --compact
python tools/query_rti_work.py check --lane mom-sender-count-reports-null-buckets --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.mom_sender_count_reports_null_buckets\\.catch2\\.Embedded MOM sender count reports emit NULL buckets for empty ledgers$" --output-on-failure
```

The receiver-ledger MOM reflections-received slice is independently buildable
at `joined_federate_mom_reflections_received_catch2.cpp:194`. It records 129
`HLA_EVOKED` assertions, maps one Requirements-Lab candidate to canonical
2025 clause 11.4.1, and records two best-effort Server reflections plus one
reliable Soda reflection at the receiving federate. The report decodes the
official `HLAtransportation` and nested `HLAobjectClassBasedCounts` values,
then verifies two empty `HLAreflectCounts` NULL buckets for an idle federate:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-reflections-received --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestReflectionsReceived reports class and transportation counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-reflections-received --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-reflections-received --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_reflections_received\\.catch2\\.Embedded MOM requestReflectionsReceived reports class and transportation counts$" --output-on-failure
```

The completed receiver-ledger MOM interactions-received slice is independently
buildable at `joined_federate_mom_interactions_received_catch2.cpp:149`. It
records 113 `HLA_EVOKED` assertions, maps one Requirements-Lab candidate to
canonical 2025 clause 11.4.1, and delivers one reliable plus one best-effort
`TakeOrder` callback to the represented receiver. The report decodes the
official `HLAtransportation` and nested `HLAinteractionCounts` values, checks
one populated bucket per supported transportation, two empty NULL buckets for
an idle federate, and RTI-originated callback metadata:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-interactions-received --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestInteractionsReceived reports class and transportation counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-interactions-received --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-interactions-received --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_interactions_received\\.catch2\\.Embedded MOM requestInteractionsReceived reports class and transportation counts$" --output-on-failure
```

The completed directed receiver-ledger MOM slice is independently buildable at
`joined_federate_mom_directed_interactions_received_catch2.cpp:180`. It records
119 `HLA_EVOKED` assertions, maps one Requirements-Lab candidate to canonical
2025 clause 11.4.1, proves an ordinary `TakeOrder` receive stays out of the
directed ledger, and counts one directed reliable receive. The report decodes
the official `HLAtransportation` and nested `HLAinteractionCounts` values,
checks the empty best-effort bucket and two idle NULL buckets, and preserves
RTI-originated callback metadata:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-directed-interactions-received --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestDirectedInteractionsReceived reports directed counts and NULL buckets" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-directed-interactions-received --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-directed-interactions-received --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.joined_federate_mom_directed_interactions_received\\.catch2\\.Embedded MOM requestDirectedInteractionsReceived reports directed counts and NULL buckets$" --output-on-failure
```

The completed timestamped regional attribute source-resignation slice is
independently buildable at
`timestamped_regional_attribute_update_resignation_catch2.cpp:146`. It records
58 `HLA_EVOKED` assertions, maps eight Requirements-Lab candidates to six
canonical 2025 clauses, and proves a queued timestamped update from an ordinary
registration's private default source survives source resignation. The
independent regulator releases the regional subscriber before its TAR(7) grant;
the callback retains the original producer, payload, tag, timestamp/order,
valid retraction metadata, and supplied-empty `RegionHandleSet`, while a
post-resignation `Retract` raises `FederateNotExecutionMember`:

```powershell
python tools/query_rti_work.py focus timestamped-regional-attribute-update-resignation --summary --compact
python tools/query_rti_work.py trace "Embedded queued timestamped regional attribute update survives source resignation" --summary --compact
python tools/query_rti_work.py matrix timestamped-regional-attribute-update-resignation --summary --compact
python tools/query_rti_work.py check --lane timestamped-regional-attribute-update-resignation --summary --compact
cmake --build .build --config Debug --target umbra_timestamped_regional_attribute_update_resignation_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.timestamped_regional_attribute_update_resignation\.catch2\.Embedded queued timestamped regional attribute update survives source resignation$" --output-on-failure
```

The requirements-facing `timestamped-default-region-attribute-update-resignation`
lane is an explicit alias of this executable evidence; use its focus/trace
handle for the default-region view without adding a duplicate runtime case.

The timed negotiated-cancellation-after-restore slice is independently
buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore_catch2.cpp:263`.
It records 135 `HLA_EVOKED` assertions, maps 44 Requirements-Lab candidates to
23 canonical 2025 sections, and uses the short standalone target
`umbra_tso_regional_negotiated_cancel_restore_catch2`. The four-federate case
restores one saved logical-time-8 explicit-source regional update, queues an If
Available willing-to-acquire reservation, enters negotiated divestiture, and
has the requester resign with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` before
`Request Divestiture Confirmation`. The stale negotiated path is suppressed,
the owner remains authoritative, and the surviving constrained recipient
receives one saved reflection at its Flush Queue boundary:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending negotiated ownership transfer after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-cancel-pending-after-restore --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update survives requester cancellation of pending negotiated ownership transfer after restore$" --output-on-failure
```

Keep negotiated transfer completion, multi-candidate arbitration, alternate
callback models, passive/relaxed DDM, remote transport, package/JUnit/protected
review, Lab validation, and conformance as separate evidence lanes.

The timed negotiated-continuation-after-restore slice is independently
buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore_catch2.cpp:283`.
It records 158 `HLA_EVOKED` assertions, maps 44 Requirements-Lab candidates to
23 canonical 2025 sections, and uses the short target
`umbra_tso_regional_negotiated_continuation_restore_catch2`. The four-federate
case restores a saved timestamped regional update, queues two If Available
requests before negotiated divestiture, and has the first requester resign with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`. The second request callback remains
pending; reissued negotiation forwards its tag to the owner, the surviving
recipient reflects before its Flush Queue grant, and Confirm Divestiture emits
one terminal acquisition notification. The original If Available callback is
drained only after confirmation so stale work is suppressed:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues to a retained negotiated ownership candidate after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-continuation-after-restore --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore\\.catch2\\.Embedded timed multi-recipient regional timestamped attribute update continues to a retained negotiated ownership candidate after restore$" --output-on-failure
```

Keep persistent post-callback Willing-to-Acquire semantics, broader arbitration,
alternate callback models, passive/relaxed DDM, remote transport,
package/JUnit/protected review, Lab validation, and conformance separate.

The timed negotiated-confirmation-cancel-after-restore slice is independently
buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore_catch2.cpp:283`.
It records 161 `HLA_EVOKED` assertions, maps 48 Requirements-Lab candidates to
26 canonical 2025 sections, and uses the short target
`umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2`. The
four-federate case restores a saved timestamped regional update, queues two If
Available candidates, reselects the retained candidate after the first
requester resigns with `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, and delivers
Request Divestiture Confirmation. The owner then cancels the negotiated
divestiture, keeps ownership, rejects stale Confirm Divestiture, and emits no
owner-release callback; the queued clock callback reports unavailable:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels retained negotiated owner confirmation after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-confirmation-cancel-after-restore --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore\\.catch2\\.Embedded timed multi-recipient regional timestamped attribute update cancels retained negotiated owner confirmation after restore$" --output-on-failure
```

Keep persistent post-callback Willing-to-Acquire semantics, negotiated
transfer completion, alternate callback models, passive/relaxed DDM, remote
transport, package/JUnit/protected review, Lab validation, and conformance
separate.

The timed mixed regular-to-If-Available continuation-after-restore slice is
independently buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore_catch2.cpp:15`.
It records 161 `HLA_EVOKED` assertions, maps 46 Requirements-Lab candidates to
24 canonical 2025 sections and 31 selected official C++ API surfaces, and uses
the target `umbra_tso_regional_mixed_continuation_restore_catch2`. The
four-federate case restores the logical-time-8 explicit-source regional update,
queues a regular candidate for the first constrained recipient and an If
Available candidate on the independent clock, then reissues negotiated
divestiture after the first requester resigns. The registry retains the If
Available reservation until `Confirm Divestiture`, suppresses the stale
ordinary callback, delivers surviving regional reflections before the common
Flush Queue grant, and emits one terminal acquisition notification to the
retained clock:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to an If Available candidate after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-mixed-candidate-continuation-after-restore --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to an If Available candidate after restore$" --output-on-failure
```

Keep passive/relaxed DDM, alternate callback models, remote transport,
package/JUnit/protected review, Requirements-Lab validation, interoperability,
and conformance separate.

The timed mixed If-Available-to-retained-regular pre-delivery cancellation
slice is independently buildable at
`timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore_catch2.cpp:15`.
It records 161 `HLA_EVOKED` assertions, maps 50 Requirements-Lab candidates to
26 canonical 2025 sections and 32 selected official C++ API surfaces, and uses
the target `umbra_tso_mixed_pre_delivery_cancel_catch2`. The four-federate case
restores the logical-time-8 explicit-source regional update, queues an If
Available candidate for the first constrained recipient and a regular
candidate on the independent clock, reissues negotiation after requester
resignation, and cancels before `Request Divestiture Confirmation` enters user
code. Stale confirmation work is consumed without an acquisition notification,
ownership remains with the publisher, and both surviving regional recipients
reflect before the common Flush Queue grant:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation before delivery after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-after-restore --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation before delivery after restore$" --output-on-failure
```

Keep alternate callback models, passive/relaxed DDM, remote transport,
package/JUnit/protected review, Requirements-Lab validation, interoperability,
and conformance separate.

The regular-to-regular negotiated continuation is source-backed through a
direct wrapper at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_lane_catch2.cpp:11`.
It records 164 `HLA_EVOKED` assertions across 46 Requirements-Lab anchors, 24
canonical 2025 sections, and 30 official C++ API surfaces. The standalone
target makes the macro-based fixture visible to source indexing:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-regular-candidate-continuation-after-restore --summary --compact
cmake --build <build-dir> --config Release --target umbra_tso_regional_regular_continuation_restore_catch2
ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update continues from a regular request to a regular candidate after restore$" --output-on-failure
```

The mixed If-Available-to-retained-regular confirmation-cancellation slice is
source-backed at
`cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore_catch2.cpp:15`.
It records 163 `HLA_EVOKED` assertions across 50 Requirements-Lab anchors, 26
canonical 2025 sections, and 32 official C++ API surfaces. It cancels after
the owner confirmation and surviving reflection/Flush Queue boundary; the
publisher retains ownership, stale Confirm Divestiture is rejected, and the
regular reservation emits no acquisition callback:

```powershell
python tools/query_rti_work.py focus tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
python tools/query_rti_work.py trace "Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation after restore" --summary --compact
python tools/query_rti_work.py matrix tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
python tools/query_rti_work.py check --lane tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-after-restore --summary --compact
cmake --build <build-dir> --config Release --target umbra_tso_mixed_confirmation_cancel_catch2
ctest --test-dir <build-dir> -C Release -R "^umbra\.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore\.catch2\.Embedded timed multi-recipient regional timestamped attribute update cancels mixed retained regular owner confirmation after restore$" --output-on-failure
```

Keep alternate callback models, passive/relaxed DDM, remote transport,
package/JUnit/protected review, Requirements-Lab validation, interoperability,
and conformance separate.

The timestamped-attribute-order-cohort slice is independently buildable at
`timestamped_attribute_order_cohort_catch2.cpp:152`. It records 87 `HLA_EVOKED`
assertions, maps five Requirements-Lab candidates to five canonical 2025
sections, and uses the target `umbra_timestamped_attribute_order_cohort_catch2`.
The three-member case submits timestamp 7 before the timestamp-5 cohort,
advances two constrained recipients to 5 and then 7, and verifies the cohort
arrives before each grant while different timestamps remain ordered. The
equal-timestamp tie-break is intentionally unspecified:

```powershell
python tools/query_rti_work.py focus timestamped-attribute-order-cohort --summary --compact
python tools/query_rti_work.py trace "Embedded timestamped attribute updates preserve different-timestamp order for each constrained recipient" --summary --compact
python tools/query_rti_work.py matrix timestamped-attribute-order-cohort --summary --compact
python tools/query_rti_work.py check --lane timestamped-attribute-order-cohort --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\\.timestamped_attribute_order_cohort\\.catch2\\.Embedded timestamped attribute updates preserve different-timestamp order for each constrained recipient$" --output-on-failure
```

Keep alternate advances, simultaneous transport-arrival ordering, save/restore
composition, ownership/resignation, remote transport, package/JUnit/protected
review, Lab validation, and conformance separate.

The timestamped-directed-interaction-regulation-reenable-changed-lookahead
slice is independently buildable at
`timestamped_directed_interaction_regulation_reenable_changed_lookahead_catch2.cpp:151`.
It records 57 `HLA_EVOKED` assertions, maps 11 Requirements-Lab candidates to
eight canonical 2025 sections, and uses the short target
`umbra_tso_directed_reenable_changed_lookahead_catch2`. The case queues one
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
ctest --test-dir <build-dir> -C Release -R "^umbra\.timestamped_directed_interaction_regulation_reenable_changed_lookahead\.catch2\.Embedded queued timestamped directed interaction survives time-regulation disable and re-enable with changed lookahead$" --output-on-failure
```

Keep directed DDM breadth, alternate advances, ownership/resignation,
save/restore composition, remote transport, package/JUnit/protected review,
Lab validation, interoperability, and conformance separate. Use
`ready --summary --compact` for the next exact indexed slice after this lane;
do not reopen the Requirements Lab.

Default `next --summary` follows the same bounded ready selector and prints
canonical `standard_sections` keys when it has a concrete slice; the exact
`test --compact` query prints every selected Requirements-Lab ID and its
resolved 2025 clause/subsection. Use the exact lane/test handles above; do not
rescan the Requirements Lab. The completed
directed post-delivery lane remains directly queryable as a regression:

```powershell
python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-post-delivery-resignation --compact
```

The mixed regular/If Available fanout remains the immediately preceding
baseline when the next slice needs to compare recipient selection:

```powershell
python tools/query_rti_work.py lane process-restart-confirm-divestiture-mixed-fanout --compact
```

## Find a lane without opening source

```powershell
python tools/query_rti_work.py lanes --summary --limit 40
python tools/query_rti_work.py source cpp/tests/external_2010_fom_catch2.cpp --summary --limit 20
python tools/query_rti_work.py search "<stable concept>" --summary --limit 20
python tools/query_rti_work.py item <roadmap-item-id> --compact --limit 20
python tools/query_rti_work.py plan --summary
```

`lanes` lists exact tags and case counts. `source` is the bounded reverse
lookup for a known C++ translation unit; it lists every planned case in that
file with its source line, Lab IDs, and canonical sections. `search` is a bounded discovery
fallback across IDs, tags, API surfaces, requirements, and standard sections.
`item` joins one roadmap item to its tagged tests. `plan` prints only the
implementation-plan heading outline, not the long prose.

## Reconciliation queues

```powershell
python tools/query_rti_work.py unlocated --summary --limit 20
python tools/query_rti_work.py unmapped --summary --limit 20
python tools/query_rti_work.py unmapped --disposition explicit --summary --limit 20
python tools/query_rti_work.py unmapped --disposition unclassified --summary --limit 20
python tools/query_rti_work.py unlocated --lane <exact-catch2-tag> --summary --limit 20
python tools/query_rti_work.py unmapped --lane <exact-catch2-tag> --summary --limit 20
python tools/query_rti_work.py unlocated --family <roadmap-family-id> --summary --limit 20
python tools/query_rti_work.py unmapped --family <roadmap-family-id> --summary --limit 20
python tools/query_rti_work.py coverage --summary
```

`unlocated` contains retained plan history whose current source declaration is
missing; it is not executable evidence. `unmapped` contains C++ cases that do
not yet select a Lab requirement and prints its disposition split. `--disposition
explicit` shows rows whose metadata explicitly records a no-standalone-surface
decision; `--disposition unclassified` shows rows that still need that decision
or mapping. Both queues accept `--lane <exact-catch2-tag>` for a focus-lane-only
view. Neither queue is a reason to rescan the Lab while implementing an
unrelated lane. `coverage` gives the bounded counts
for Catch2 cases, requirement references, and standard clauses; pass
`--family <roadmap-family-id>` for family totals without listing its rows. If
`check --compact` reports additional unmarked source-location mismatches, treat
that as a source-drift repair queue before promoting a new test; do not turn it
into a repository-wide search.

The strict `tools/query_rti_work_regression.py` harness also retains historical
line baselines for selected aggregate-translation-unit cases. An insertion can
make that audit fail on a stale line even when the live source index resolves
the exact TEST_CASE; treat it as a bounded pointer-repair queue, while
`case`/`trace`/`focus`/`check` remain the authoritative current mapping views.

## Query contract

The mapping chain is:

```text
roadmap item
  -> exact Catch2 tag / test title
  -> source file and TEST_CASE line
  -> selected Requirements-Lab requirement IDs
  -> pinned IEEE 1516.1-2025 document:clause/subsection
```

Keep new work in one focused lane, add the exact test title and requirement
IDs to `compliance/requirements-lab/catch2-test-plan.json`, and run
`check --compact` before claiming evidence. If a source declaration is
renamed or removed, the check fails (or the entry must be explicitly marked
`source-missing-needs-reconciliation`) so stale mapping cannot silently look
green.

The preceding source-backed handoff is the ordinary Attribute Relevance Advisory
slice at `cpp/tests/attribute_relevance_advisory_catch2.cpp:265`:

```powershell
python tools/query_rti_work.py focus attribute-relevance-scope-transition --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories follow scope transitions" --summary --compact
python tools/query_rti_work.py matrix "Embedded attribute relevance advisories follow scope transitions" --summary --compact
python tools/query_rti_work.py check --lane attribute-relevance-scope-transition --summary --compact
ctest --test-dir <build-dir> -C Release -R "^umbra\.attribute_relevance_advisory\.catch2\.Embedded attribute relevance advisories follow scope transitions$" --output-on-failure
```

The case records 106 assertions under both callback models and maps nine
Requirements-Lab anchors directly to clauses 6.23, 6.24, and 10.37.1. The
source pointer is now present in the plan, so `ready --summary --compact`
selects the next bounded row without a broad source or Lab search.

The preceding source-backed handoff is the Query Attribute Ownership
service-report interaction slice at
`cpp/tests/attribute_ownership_query_catch2.cpp:296`:

```powershell
python tools/query_rti_work.py focus query-attribute-ownership-service-report-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers Query Attribute Ownership through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix query-attribute-ownership-service-report-interaction --summary --compact
python tools/query_rti_work.py check --lane query-attribute-ownership-service-report-interaction --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.attribute_ownership_query\.catch2\.Embedded service reporting delivers Query Attribute Ownership through MOM interaction$" --output-on-failure
```

The case records 67 assertions, maps 11 Requirements-Lab anchors directly to
clauses 7.17.5, 7.18.4, 11.5, 11.5.1, 11.5.2, and 11.5.2.1, and proves the
HLA_IMMEDIATE MOM report arrives before the HLA_EVOKED grouped ownership
callbacks. `ready --summary --compact` now advances to the next
source-unlocated row.

The preceding source-backed handoff is the Cancel Attribute Ownership
Acquisition service-report interaction slice at
`cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp:387`:

```powershell
python tools/query_rti_work.py focus cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers Cancel Attribute Ownership Acquisition through MOM" --summary --compact
python tools/query_rti_work.py matrix cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
python tools/query_rti_work.py check --lane cancel-attribute-ownership-acquisition-service-report-interaction --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.attribute_ownership_acquisition_cancellation\.catch2\.Embedded service reporting delivers Cancel Attribute Ownership Acquisition through MOM$" --output-on-failure
```

The case records 77 assertions under both callback models, maps six
Requirements-Lab anchors directly to clauses 7.15, 7.16, 11.5, 11.5.2, and
11.5.2.1, and proves the HLA_IMMEDIATE MOM report arrives before the
HLA_EVOKED cancellation confirmation. `ready --summary --compact` now advances
to the next source-unlocated row.

The preceding source-backed handoff remains the Cancel Negotiated Attribute
Ownership Divestiture service-report interaction slice at
`cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp:547`:

```powershell
python tools/query_rti_work.py focus cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
python tools/query_rti_work.py trace "Embedded service reporting delivers Cancel Negotiated Attribute Ownership Divestiture through MOM interaction" --summary --compact
python tools/query_rti_work.py matrix cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
python tools/query_rti_work.py check --lane cancel-negotiated-attribute-ownership-divestiture-service-report-interaction --summary --compact
ctest --test-dir .build -C Release -R "^umbra\.negotiated_attribute_ownership_divestiture_pending\.catch2\.Embedded service reporting delivers Cancel Negotiated Attribute Ownership Divestiture through MOM interaction$" --output-on-failure
```

The case records 87 assertions under both callback models, maps five
Requirements-Lab anchors directly to clauses 7.8, 7.14.6, 11.5, 11.5.2, and
11.5.2.1, and proves the HLA_IMMEDIATE MOM report arrives before the
HLA_EVOKED ordinary release callback. `ready --summary --compact` now advances
to the next source-unlocated row.

The preceding source-backed handoff is the Service Reporting subscription
interlock at `cpp/tests/mom_service_reporting_interlock_catch2.cpp:50`:

```powershell
python tools/query_rti_work.py focus service-reporting-interlock --summary --compact
python tools/query_rti_work.py trace "Embedded MOM service-reporting state excludes report-service subscriptions" --summary --compact
python tools/query_rti_work.py matrix service-reporting-interlock --summary --compact
python tools/query_rti_work.py check --lane service-reporting-interlock --summary --compact
cmake --build .build --config Release --target umbra_mom_service_reporting_interlock_catch2
ctest --test-dir .build -C Release -R "^umbra\.mom_service_reporting_interlock\.catch2\.Embedded MOM service-reporting state excludes report-service subscriptions$" --output-on-failure
```

This bounded `HLA_EVOKED` case records 41 assertions and maps three
Requirements-Lab anchors directly to §§5.10.2, 9.10.3, and 11.5. It rejects
active and passive ordinary/regional report-service subscriptions while the
switch is enabled, preserves state on rejected transitions, and proves
removal-before-enable recovery. `ready --summary --compact` now advances to
the next exact object/DDM MOM row.

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

This bounded `HLA_EVOKED` case records 55 assertions and maps one
Requirements-Lab anchor directly to §11.4.1. It verifies the live
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

This bounded `HLA_EVOKED` case records 63 assertions and maps one
Requirements-Lab anchor directly to §11.4.1. The direct MOM request samples
`HLAROlength` before delivery (0 → 1), periodic `HLAsetTiming` reflection
preserves the queued value, and the count returns to 0 after the receive-order
callback. The ledger is recipient-scoped and excludes RTI-owned MOM traffic.
Deferred asynchronous/TSO variants, remaining MOM statistics, regional or
transport variants, package/JUnit, validation, and conformance remain separate.

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

This bounded `HLA_EVOKED` case records 56 assertions and maps one
Requirements-Lab anchor directly to §11.4.1. Direct MOM requests expose the
accepted `Update Attribute Values` invocation count as `HLAinteger32BE`
(0 → 1 → 2), and periodic `HLAsetTiming` reflection preserves 2. The counter
advances at the accepted service boundary, not per value or downstream
callback. Remaining MOM statistics, timestamped/update-rate and
regional/transport variants, package/JUnit, validation, and conformance remain
separate.

The source-backed joined-federate MOM
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

This bounded `HLA_EVOKED` case records 77 assertions and maps one
Requirements-Lab anchor directly to §11.4.1. Direct MOM requests prove the
distinct-object ledger 0 → 1 → 1 → 2, while periodic `HLAsetTiming` reflection
preserves 2 alongside `HLAupdatesSent=3`. The value is per object instance,
not per update invocation, value, or callback; remaining MOM statistics and
promotion gates remain separate.

The preceding `HLAobjectInstancesDeleted` lane is independently queryable at
`cpp/tests/joined_federate_mom_deleted_object_count_periodic_catch2.cpp:108`
with 66 `HLA_EVOKED` assertions, one §11.4.1 requirement, and 12 official C++
API surfaces:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-deleted-object-count-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM exposes HLAobjectInstancesDeleted count" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-deleted-object-count-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-deleted-object-count-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_deleted_object_count_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_deleted_object_count_periodic\.catch2\.Embedded joined-federate MOM exposes HLAobjectInstancesDeleted count$" --output-on-failure
```

The latest source-backed handoff is the receiving-federate
`HLAobjectInstancesRemoved` callback-count lane at
`cpp/tests/joined_federate_mom_removed_object_count_periodic_catch2.cpp:145`.
It records 78 `HLA_EVOKED` assertions, one §11.4.1 requirement, and 14
official C++ API surfaces, and keeps direct/periodic recipient-scoped counts
0 → 1 → 2 queryable without rerunning the broader object-management suite:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-removed-object-count-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAobjectInstancesRemoved counts committed callbacks" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-removed-object-count-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-removed-object-count-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_removed_object_count_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_removed_object_count_periodic\.catch2\.Embedded joined-federate MOM HLAobjectInstancesRemoved counts committed callbacks$" --output-on-failure
```

The latest source-backed handoff is the receiving-federate
`HLAobjectInstancesDiscovered` lane at
`cpp/tests/joined_federate_mom_discovered_object_count_periodic_catch2.cpp:127`.
It records 66 `HLA_EVOKED` assertions, one §11.4.1 requirement, and 14
official C++ API surfaces, and keeps eligible discovery counts 0 → 2 → 3
queryable without rerunning the broader object-management suite:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-discovered-object-count-periodic --summary --compact
python tools/query_rti_work.py trace "Embedded joined-federate MOM HLAobjectInstancesDiscovered counts eligible callbacks" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-discovered-object-count-periodic --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-discovered-object-count-periodic --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_discovered_object_count_periodic_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_discovered_object_count_periodic\.catch2\.Embedded joined-federate MOM HLAobjectInstancesDiscovered counts eligible callbacks$" --output-on-failure
```

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

This bounded `HLA_EVOKED` case records 112 assertions and maps one
Requirements-Lab anchor directly to §11.4.1. Direct MOM requests prove the
registration ledger 0 → 1 → 2; three accepted updates establish
`HLAupdatesSent=3` and `HLAobjectInstancesUpdated=2`, and periodic reflection
preserves the complete snapshot. Invalid registration paths and remaining
promotion gates remain separate.

The current source-backed handoff is the official handle-normalization lane at
`cpp/tests/handle_normalization_catch2.cpp:60`. It records 52 `HLA_EVOKED`
assertions and maps six Requirements-Lab anchors directly to six canonical
2025 sections (§§10.1.3 and 10.29–10.33), with all five official C++ normalizer
surfaces selected. Keep the query bounded:

```powershell
python tools/query_rti_work.py focus handle-normalization --summary --compact
python tools/query_rti_work.py trace "Embedded handle normalization supplies stable DDM point-range coordinates" --summary --compact
python tools/query_rti_work.py matrix object-ddm-ownership --summary --compact
python tools/query_rti_work.py check --lane handle-normalization --summary --compact
cmake --build .build --config Release --target umbra_handle_normalization_catch2
ctest --test-dir .build -C Release -R "^umbra\.handle_normalization\.catch2\.Embedded handle normalization supplies stable DDM point-range coordinates$" --output-on-failure
```

The case covers NotConnected/FederateNotExecutionMember fences, typed invalid
designators, same-execution coordinate equality through two joined ambassadors,
the bounded ServiceGroup domain, and federate stability after resignation.

The two-dimensional independent-source DDM stress lane is separately indexed at
`cpp/tests/regional_multi_attribute_ddm_catch2.cpp:116` with 78 `HLA_EVOKED`
assertions, four Requirements-Lab anchors, and four canonical 2025 sections
(§§9.5, 9.6, 9.8, and 9.9.3). Use the exact lane handles when working on that
slice:

```powershell
python tools/query_rti_work.py focus ddm-regional-multi-attribute --summary --compact
python tools/query_rti_work.py trace "Embedded two-dimensional regional object updates filter independent attribute sources" --summary --compact
python tools/query_rti_work.py matrix ddm-regional-multi-attribute --summary --compact
python tools/query_rti_work.py check --lane ddm-regional-multi-attribute --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.regional_multi_attribute_ddm\\.catch2\\.Embedded two-dimensional regional object updates filter independent attribute sources$" --output-on-failure
```

Keep timestamped/retraction, relaxed DDM, transport, package/JUnit, review,
validation, and conformance as separate evidence lanes.

The regional object region-context boundary is a separate 2025 DDM lane at
`cpp/tests/regional_object_attribute_routing_catch2.cpp:291`. It has 34
`HLA_EVOKED` assertions, five direct §9.1.3.3 requirement anchors, and four
official C++ region-aware service surfaces. Use the exact bounded handles:

```powershell
python tools/query_rti_work.py roadmap regional-object-attribute-region-context --summary --compact
python tools/query_rti_work.py focus regional-object-attribute-region-context --summary --compact
python tools/query_rti_work.py trace "Embedded regional object services reject region dimensions outside available object dimensions" --summary --compact
python tools/query_rti_work.py matrix requirement-candidate-content-clauses-09-data-distribution-management-page-220-l8-2 --summary --compact
python tools/query_rti_work.py check --lane regional-object-attribute-region-context --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.regional_object_attribute_routing\.catch2\.Embedded regional object services reject region dimensions outside available object dimensions$" --output-on-failure
```

This case is deliberately limited to deterministic `InvalidRegionContext`
rejection for out-of-context committed dimensions at registration, update
association, regional subscription, and regional Request Attribute Value
Update. Valid subset realization and later routing remain separate lanes.

The next source-backed handoff is the joined-federate MOM
`HLAobjectInstancesUpdated` request/report lane at
`cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp:137`.
It records 42 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor to
§11.4.1. The case consumes the Subscribe-only request, groups the accepted
update ledger by registered class, preserves distinct-object semantics across
repeated updates, and decodes the nested `HLAobjectClassBasedCounts` report.
Use only its exact handles:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-object-instances-updated-report --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestObjectInstancesUpdated reports class-grouped counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-object-instances-updated-report --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-object-instances-updated-report --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_object_instances_updated_report_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instances_updated_report\.catch2\.Embedded MOM requestObjectInstancesUpdated reports class-grouped counts$" --output-on-failure
```

HLA_IMMEDIATE, transport/timestamp variants, the remaining MOM request/report
families, and promotion evidence remain separate query lanes.

The live-ownership companion is the joined-federate MOM
`HLArequestObjectInstancesThatCanBeDeleted` lane at
`cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp:137`.
It records 53 `HLA_EVOKED` assertions and maps one Requirements-Lab anchor to
§11.4.1. The case derives nested class counts from live
`HLAprivilegeToDeleteObject` ownership, proving two classes before deletion
and one remaining class after deletion:

```powershell
python tools/query_rti_work.py focus joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
python tools/query_rti_work.py trace "Embedded MOM requestObjectInstancesThatCanBeDeleted reports live owner counts" --summary --compact
python tools/query_rti_work.py matrix joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
python tools/query_rti_work.py check --lane joined-federate-mom-object-instances-that-can-be-deleted-report --summary --compact
cmake --build .build --config Release --target umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
ctest --test-dir .build -C Release -R "^umbra\.joined_federate_mom_object_instances_that_can_be_deleted_report\.catch2\.Embedded MOM requestObjectInstancesThatCanBeDeleted reports live owner counts$" --output-on-failure
```

Use the exact lane handles for this bounded report; HLA_IMMEDIATE,
transport/timestamp variants, the remaining MOM request/report families, and
promotion evidence remain separate.

The latest source-backed handoff is the reflected-object MOM request/report
lane at
`cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp:160`.
It records 51 HLA_EVOKED assertions, maps one Requirements-Lab anchor to
§11.4.1, and exercises 14 official C++ API surfaces. The Subscribe-only
`HLArequestObjectInstancesReflected` request is answered for the requesting
federate from its accepted application reflection callback ledger; repeated
reflections of one object stay one distinct count, while two classes decode
from the official nested `HLAobjectClassBasedCounts` value.

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

The latest known/NULL object-information slice is independently queryable at
`cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp:146`.
It records 71 HLA_EVOKED assertions, maps one Requirements-Lab anchor to
§11.4.1, and exercises 13 official C++ API surfaces. The Subscribe-only
`HLArequestObjectInstanceInformation` request decodes the nested
`HLAattributeHandleList`, proves the registering federate's owned attributes
and registered/known class values, then uses Local Delete Object Instance to
produce the MIM NULL response with only the object reference and empty list.

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

The latest source-backed handoff is the active-maximum Attribute Relevance
Advisory lane at `cpp/tests/attribute_relevance_rate_reissue_catch2.cpp:108`.
It records 108 assertions under `HLA_EVOKED` and `HLA_IMMEDIATE`, maps seven
Requirements-Lab anchors directly to §§6.23/6.24, and proves multi-federate
High/Medium/Low maximum selection, lower-rate peer suppression, rate-bearing
reissue, Low refresh after higher-rate removal, and final Turn Updates Off.

```powershell
python tools/query_rti_work.py focus attribute-relevance-rate-reissue --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories reissue turn-on when the active update rate changes" --summary --compact
python tools/query_rti_work.py matrix attribute-relevance-rate-reissue --summary --compact
python tools/query_rti_work.py check --lane attribute-relevance-rate-reissue --summary --compact
cmake --build .build --config Release --target umbra_attribute_relevance_rate_reissue_catch2
ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_rate_reissue\.catch2\.Embedded attribute relevance advisories reissue turn-on when the active update rate changes$" --output-on-failure
```

Use this exact lane handle for the bounded object-management slice; regional/
DDM variants, process transport, review, validation, interoperability, and
conformance remain separate.

The next bounded handoff is the regional explicit-rate Attribute Relevance
Advisory lane at `cpp/tests/regional_attribute_relevance_rate_designator_catch2.cpp:105`.
It has 100 assertions, both callback models, eight Requirements-Lab anchors,
three standard sections (§§6.23, 6.24, 10.37.1), and seven official API surfaces.
The scenario keeps a disjoint source region from causing duplicate callbacks,
emits one Off when overlap is removed, and restores one High-rate On when the
original overlap returns.

```powershell
python tools/query_rti_work.py focus regional-attribute-relevance-rate-designator --summary --compact
python tools/query_rti_work.py trace "Embedded regional attribute relevance advisories retain explicit update-rate designators" --summary --compact
python tools/query_rti_work.py matrix regional-attribute-relevance-rate-designator --summary --compact
python tools/query_rti_work.py check --lane regional-attribute-relevance-rate-designator --summary --compact
cmake --build .build --config Release --target umbra_regional_attribute_relevance_rate_designator_catch2
ctest --test-dir .build -C Release -R "^umbra\.regional_attribute_relevance_rate_designator\.catch2\.Embedded regional attribute relevance advisories retain explicit update-rate designators$" --output-on-failure
```

Use the exact lane handle above; process transport, review, validation,
interoperability, and promotion/conformance remain separate queues.

The known-class-disabled Attribute Relevance Advisory handoff is independently
queryable at `cpp/tests/attribute_relevance_known_class_disabled_subscription_catch2.cpp:97`.
It has 35 assertions, one callback model (`HLA_EVOKED`), 18 Requirements-Lab
anchors, 13 canonical 2025 sections, and 18 official API surfaces. The case
keeps the discovery class at `Employee` while proving that the registered
`Server` class still drives the owner-directed On/Off advisory transition when
the static known-class policy is disabled.

```powershell
python tools/query_rti_work.py focus known-class-disabled --summary --compact
python tools/query_rti_work.py trace "Embedded attribute relevance advisories use subscriptions when known-class policy is disabled" --summary --compact
python tools/query_rti_work.py matrix known-class-disabled --summary --compact
python tools/query_rti_work.py check --lane known-class-disabled --summary --compact
cmake --build .build --config Release --target umbra_attribute_relevance_known_class_disabled_subscription_catch2
ctest --test-dir .build -C Release -R "^umbra\.attribute_relevance_known_class_disabled_subscription\.catch2\.Embedded attribute relevance advisories use subscriptions when known-class policy is disabled$" --output-on-failure
```

Use this exact handle for the disabled-policy slice; the enabled policy and all
rate, regional/DDM, process, review, validation, interoperability, and
promotion/conformance queues remain separate.
