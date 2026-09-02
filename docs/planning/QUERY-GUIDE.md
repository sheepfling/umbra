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

`work` is the one-screen work selector: it follows the highest-priority open
roadmap item's `next_work_id`, then prints the concrete work item, task,
exact lane/CTest filter, bounded lane state, package target/test/label handles,
one baseline test with its source and requirement/section handles, and
copy/paste query commands (including a direct `trace` command for the
baseline). `lane_state` makes a completed pointer explicit: when it is
`complete` with zero executable candidates, the next step is a newly indexed
C++ case or another indexed family, not a rescan. Pass an item id to inspect a
different indexed slice (`work transport-and-conformance`). `next --summary`
remains useful when you want the parent roadmap item's handle fields without
the joined baseline record; it reports the same bounded lane state when a lane
is present.
Use `next --json` when the full, unabridged work query or complete handle lists
are needed.

`queue --summary --compact` is the bounded family selector. It prints one row
per open roadmap item with its exact work/lane handle and classifies the row as
`ready`, `complete-pointer`, `source-drift-only`, or `new-case-needed`. This
prevents the broad overlapping tag set from becoming the work-selection
interface. Family counts intentionally overlap when a case belongs to more than
one roadmap family; use `coverage` for global totals. Enter a row through its
printed `work_query` or `focus_query`; only then inspect the exact test and
clause mapping.

`focus` is the one-screen lane selector. It resolves the active indexed lane
when no tag is supplied, or one exact Catch2 tag when supplied. The result
separates implemented cases from source-located executable candidates and
historical source-drift rows, then prints the lane owner, requirement,
canonical 2025 subsection, and assertion counts, plus the lane's
Catch2/CTest/JUnit handles.
This is the preferred query after `work` so a completed baseline cannot be
mistaken for new implementation work. Use `focus --json` for scripts and
`focus <tag> --limit 0` only for an intentional unbounded candidate list.

```powershell
python tools/query_rti_work.py focus --summary --compact
python tools/query_rti_work.py focus process-boundary --summary --compact
python tools/query_rti_work.py focus service-report-store --json
```
Use `work --summary` for the bounded handoff: it keeps the active task, lane,
baseline source/mapping counts, and the first executable commands in view while
leaving the full package/JUnit handle set available from plain `work`,
`work --compact`, or `work --json`.
If the catalog marks an historical case as implemented but its exact C++
declaration is absent, `next --summary` reports `test_pointer=source-missing`;
that is a reconciliation target, not executable evidence.

`check --compact` is the bounded integrity gate. It verifies roadmap anchors,
Catch2 plan IDs and tags, 2025 requirement references, exact standard
subsection handles, and whether each non-placeholder plan case still has a
matching C++ `TEST_CASE` declaration. When the dirty checkout has many
source-drift rows it prints only a small error sample; use `check --json` for
the complete machine-readable diagnostic.

Use `check --lane <exact-tag> --compact` while implementing one focus lane. It
keeps the roadmap, requirement, subsection, and tag checks, but limits
test/source and completion-ledger validation to that lane. Unlocated
historical rows remain listed as visible source drift without blocking the
focused gate; the unscoped check remains the strict whole-plan reconciliation
gate.

`lane <exact-tag> --json` also returns the indexed execution handles for that
lane (Catch2 target or CTest label, CTest regex when applicable, JUnit target,
and artifact path), so the next command can be copied without searching the
build files.

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

Lane-level assertion totals come from `mapping.lane_assertion_counts` when a
focused JUnit artifact has verified the lane; exact `trace`/`test` queries keep
the per-plan-entry assertion count beside the source and requirement mapping.

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

Current status is intentionally split: `check --lane process-boundary` is
green (38 mapped/source-located cases / 1617 focused-JUnit assertions), while the unscoped gate reports the
known catalog backlog (63 source-unlocated cases and 94 cases without a Lab
requirement mapping), plus 276 source declarations without an exact plan row
globally (0 in the current FOM-composer file).
The current indexed `next_source_state=planned` slice is the HLA_IMMEDIATE
companion for timestamped regional Update Attribute Values through the completed
public process endpoint. `next --summary --compact` prints its
source target, exact requirement/section/API counts, and focused CTest filter;
the corresponding plan row is added only when the C++ `TEST_CASE` is written.

The embedded Attribute Relevance Advisory regression is
also mapped now; use its exact title with `trace` to inspect the eight pinned
requirements and three canonical 2025 sections. Use `unlocated --summary` and `unmapped --summary` to
work those queues one exact title at a time; do not treat them as a reason to
rescan the unchanged Requirements Lab. The directed process slice now covers
timestamped retraction both before and after receive; query its exact title or
the `federate.callback.request-retraction` tag before selecting the next task.

The service-report-store lane is intentionally isolated from the large
federation-management executable. It currently has ten source-located mapped
Catch2 cases across the §11.5 and §11.5.2 traces (the MemoryServiceReportStore
case is explicitly test-seam evidence, while filesystem allocation remains the
standards-facing behavior). Its roadmap owner is `mom-after-base-services`;
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

The installable-profile process package has eight independently addressable
checks: `package-process`, `package-process-timestamped`,
`package-process-parameterized`, `package-process-connection-loss`, and
`package-process-object-registration`, `package-process-named-registration`,
`package-process-attribute-update`, and `package-process-directed-retraction`.
Their exact CTest names and labels are
printed by `work --summary --compact`, so selecting a package regression does
not require a repository-wide search.
The installable-package smoke runs
`tools/verify_process_package_lanes.py` immediately after configuring the
clean downstream consumer. That catalog check compares all eight indexed test
names and labels to the generated CTest catalog and fails before execution if
a lane is missing, renamed, duplicated, or unindexed. Run the same check
directly when iterating on the package consumer:

```powershell
python tools/verify_process_package_lanes.py --ctest ctest `
  --test-dir <build-dir>/package-smoke-consumer `
  --index docs/planning/ROADMAP-INDEX.json --config Debug
```

The verifier path is exposed as
`process_package_catalog_verifier` by `work --summary --compact`.
The parameterized check resolves the server-owned `HLAobjectRoot.Customer`
class and `TimelinessOk` parameter, then verifies the exact parameter handle
and value bytes at the official receiver callback.

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
model, lane handles, and resolved requirement/section counts. Use it to resume
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

The narrow `[process-boundary]` execution loop is 38 executable cases / 1617
focused-JUnit assertions and has no unlocated rows. The local-delete slice
contributes 9 direct codec assertions, 44 private service assertions, and 18
public two-federate endpoint assertions; receive-order Delete Object Instance
adds 25 direct codec assertions and 25 public endpoint/removal-callback
assertions; the timestamped public endpoint slice adds 64 assertions under both
callback models. The
bounded `coverage --lane transport`
query currently reports 57 plan entries (53 mapped, 52 source-located, and
five historical source-drift rows); use it instead of reopening the full
catalog.
The 38-case query count is the unique mapped plan/test-declaration count; when
both the aggregate and dedicated connection executables are configured, the
`process-boundary` CTest label runs both registrations.

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
`lab_requirement_id -> document_id:clause_id; title` pairs, so a test's
requirement and subsection mapping can be read without opening the 840-entry
plan. Add `--summary` to show at most eight mapping rows per test; use
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
the federation. The object-registration projection is separately addressable
as `umbra_rti_package_process_object_registration_consumer` /
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
eight installed-profile process lanes, named-registration semantics, and the
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
`coverage --lane transport` reports 57 plan entries (53 mapped, 52
source-located, and five historical source-drift rows). Keep its evidence separate from
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
python tools/query_rti_work.py next --summary --compact
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
python tools/query_rti_work.py test "Embedded regional Auto Provide timestamped response reflects once and exposes valid retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
```

`next --summary` now prints the canonical `standard_sections` keys for the
selected slice, while the exact `test --compact` query prints every selected
Requirements-Lab ID and its resolved 2025 clause/subsection. Use the exact
lane/test handles above; do not rescan the Requirements Lab. The completed
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
python tools/query_rti_work.py search "<stable concept>" --summary --limit 20
python tools/query_rti_work.py item <roadmap-item-id> --compact --limit 20
python tools/query_rti_work.py plan --summary
```

`lanes` lists exact tags and case counts. `search` is a bounded discovery
fallback across IDs, tags, API surfaces, requirements, and standard sections.
`item` joins one roadmap item to its tagged tests. `plan` prints only the
implementation-plan heading outline, not the long prose.

## Reconciliation queues

```powershell
python tools/query_rti_work.py unlocated --summary --limit 20
python tools/query_rti_work.py unmapped --summary --limit 20
python tools/query_rti_work.py unlocated --lane <exact-catch2-tag> --summary --limit 20
python tools/query_rti_work.py unmapped --lane <exact-catch2-tag> --summary --limit 20
python tools/query_rti_work.py coverage --summary
```

`unlocated` contains retained plan history whose current source declaration is
missing; it is not executable evidence. `unmapped` contains C++ cases that do
not yet select a Lab requirement. Both queues accept `--lane <exact-catch2-tag>`
for a focus-lane-only view. Neither queue is a reason to rescan the Lab while
implementing an unrelated lane. `coverage` gives the bounded counts
for Catch2 cases, requirement references, and standard clauses. If
`check --compact` reports additional unmarked source-location mismatches, treat
that as a source-drift repair queue before promoting a new test; do not turn it
into a repository-wide search.

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
