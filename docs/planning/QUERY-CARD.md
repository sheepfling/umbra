# RTI roadmap query card

This is the short first-read for resuming Umbra work. It joins the checked-in
roadmap index, C++ Catch2 plan, pinned IEEE 1516.1/1516.2-2025 corpus, and
derived `TEST_CASE` source locations. It is read-only and does not rescan or
rewrite the Requirements Lab.

## Select one bounded slice

```powershell
python tools/query_rti_work.py work --summary --compact
python tools/query_rti_work.py next --summary --compact
python tools/query_rti_work.py queue --summary --compact
python tools/query_rti_work.py focus --summary --compact
python tools/query_rti_work.py recent --summary --compact --limit 5
```

`work` is the authoritative active pointer. It prints the indexed work item,
lane state (complete/candidate/source-drift counts), CTest/JUnit/package
handles, baseline source line, 2025 sections, and a copy/paste `trace`
command. A `baseline_test` is already green; implement the adjacent
`next_work_query` instead of rerunning the baseline as new work. When
`lane_state=complete` and `executable_candidates=0`, the indexed catalog has
no existing case to pick up: add one explicitly mapped C++ case or choose
another indexed family, rather than searching the unchanged Lab.

If the work card prints `next_source_state=planned`, the `next_source_*`
fields are the complete bounded test contract: source file target, exact
Catch2 lane, Requirements-Lab requirement ids, canonical 2025 sections, API
surfaces, and CTest filter. That future case is deliberately not counted as
executable evidence until its C++ `TEST_CASE` and plan row exist.

`queue` is the bounded family selector. It prints one row per open roadmap
family with exact work/lane handles and a state: `ready`, `complete-pointer`,
`source-drift-only`, or `new-case-needed`. It never expands the hundreds of
overlapping plan tags; use the row's `work` or `focus_query` handle to enter a
single slice. Family case and drift totals intentionally overlap when one case
belongs to more than one roadmap family; use `coverage` for global totals.

`focus` is the bounded lane decision card. With no argument it follows the
active work item's exact lane; with an argument it accepts one exact Catch2
lane tag. It reports the lane owner, mapped requirement/section counts,
assertion total, source-drift count, and copy/paste execution handles.
`complete` means there are no source-located unimplemented cases in that lane;
it does not reopen the full plan. Use `--json` for the same card in automation
and `--limit 0` only when every candidate is intentionally needed.

The current FOM-composer source queue is owned by the declaration-management
family, so its next case is available without opening the file:

```powershell
python tools/query_rti_work.py work fom-module-declaration-management --summary --compact
```

```powershell
python tools/query_rti_work.py focus process-boundary --summary --compact
python tools/query_rti_work.py focus regional-attribute-relevance --summary --compact
python tools/query_rti_work.py focus service-report-store --json
```

## Map one test to the standard

Use `trace` when the exact handle is known. It accepts a plan id or exact
`TEST_CASE` title, exact Requirements-Lab/contract requirement id, exact
canonical 2025 `document_id:clause_id`, or exact Catch2 lane tag. It never
falls back to fuzzy search.

```powershell
python tools/query_rti_work.py trace "<exact TEST_CASE title>" --summary
python tools/query_rti_work.py trace <plan-id> --json
python tools/query_rti_work.py trace <lab-requirement-id> --summary
python tools/query_rti_work.py trace hla-1516.1-2025:clause-<n> --summary
```

Each row is a direct `lab_requirement_id -> document_id:clause_id; title`
mapping with the C++ source location, API surfaces, and the owning roadmap
families. `--summary` bounds mapping previews; `--json` retains all rows. The
default trace result is capped at five matched tests; use `--limit 0` for an
intentional complete reverse lookup.

For the Annex C directed-interaction artifact guard, the exact lane and trace
are already indexed:

```powershell
python tools/query_rti_work.py focus schema-conflict --summary --compact
python tools/query_rti_work.py trace "The FDD materializer surfaces the multiple-directed-class schema conflict" --summary --compact
cmake --build <build-dir> --config Debug --target umbra_fom_composer_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\\.fom_composer\\.catch2\\.(The FDD materializer surfaces the multiple-directed-class schema conflict|The FDD materializer refuses the supplied extension when the official FDD schema cannot represent its directed-interaction merge)$" --output-on-failure
```

That one-screen result identifies the source line, RL-081's pinned requirement,
the canonical 1516.2 Clause 7 subsection, and the owning Annex C roadmap family.

The active bounded Annex C/reference-resolution slice is the transportation
name reference guard; the earlier data-type, class, attribute, and Available
Dimensions guards remain directly traceable. The adjacent support-switch table
guard is also indexed in the `switches` lane:

```powershell
python tools/query_rti_work.py focus reference-resolution --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight resolves data-type references after the complete module set is merged" --summary --compact
python tools/query_rti_work.py trace "The FOM composition preflight resolves reference-data classes after the complete module set is merged" --summary --compact
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

## Reverse lookup and lane gate

```powershell
python tools/query_rti_work.py requirement <lab-id-or-clause> --summary --limit 20
python tools/query_rti_work.py section hla-1516.1-2025:clause-<n> --summary --limit 20
python tools/query_rti_work.py lane <exact-catch2-tag> --summary --compact
python tools/query_rti_work.py check --lane <exact-catch2-tag> --summary --compact
```

Use `check --lane` while implementing one focus lane. It validates roadmap
anchors, plan ids/tags, 2025 requirement and subsection references, recent
completion rows, and source locations for that lane only. Use unscoped
`check --summary --compact` only for the whole-plan reconciliation gate.

Use `unplanned --path <source-file> --summary` when a C++ source file is the
starting point. It lists source `TEST_CASE` declarations with no exact plan
row, without inferring a requirement or status; map each case deliberately
with `test`/`trace` before adding it to the plan.
`status --summary --compact` prints the live source-only, source-unlocated, and
unmapped queue counts before the open-family list.
For a roadmap family that has a source reconciliation queue, `work --summary`
also prints its indexed `source_queue` title and location, so the next source
case is visible without opening the whole file or dumping the queue.

`lane <exact-tag> --json` includes the exact indexed Catch2/CTest/JUnit handles
and artifact path for that lane.
The lane assertion total is taken from the indexed focused-JUnit result when
available; the exact `trace` result retains the per-test assertion count.

## Current indexed gate

The active process-boundary slice is 38 source-located, mapped Catch2 cases
and 1617 focused-JUnit assertions. The local-delete process slice now includes
the 9-assertion codec contract, the 44-assertion private service integration,
and an 18-assertion public two-federate endpoint integration. The receive-order
Delete Object Instance process slice adds a 25-assertion codec contract and a
25-assertion public two-federate endpoint/removal-callback integration. The new
timestamped process slice adds a 64-assertion public endpoint/removal-callback
integration under both callback models. The m24 private
advisory projection and m25 public
configured-endpoint callback bridge cover both HLA_EVOKED and HLA_IMMEDIATE
dispatcher paths, while m26 covers callback-entry suppression of a stale
evoked advisory after switch disable, m27 covers public regional association
transitions, and m28 covers public regional subscription removal/restoration
under both callback models; m29 covers the initial regional registration/
discovery advisory under both callback models. The embedded m31 and m32 cases
cover active update-rate reissue and regional explicit-rate retention; public
m33 and m34 cover the corresponding process-boundary variants, and embedded m35
covers callback-entry rechecking after a queued regional scope transition. The
directed-interaction callback slice now covers ordinary, timestamped, and
callback-disabled queued official HLA_EVOKED and HLA_IMMEDIATE discovery and
directed delivery through the configured public process endpoint. Its
timestamped retraction sections issue Retract both before the receiver boundary
and after delivery: the former suppresses the pending directed callback, while
the latter proves exactly one legal official Request Retraction callback under
both callback models. This is private process-foundation evidence: the process
profile has no time-regulation service, so broader time-management claims remain
open. The installed-package directed/retraction consumer is now green and
independently addressable; it uses the public package for target registration,
directed declarations, timestamped send, positive post-delivery Request
Retraction, and negative pre-delivery suppression. The package lane therefore
checks both sides of the retraction boundary without reopening the full plan.
The completed public timestamped Delete Object Instance process slice is now
indexed with its exact source line, 64 assertions under HLA_EVOKED and
HLA_IMMEDIATE, eight requirement ids, four canonical sections, and official API
surfaces. The next bounded task is indexed explicitly as a planned HLA_IMMEDIATE
companion for timestamped regional Update Attribute Values. Its contract is the
configured `cpp/tests/ieee1516_2025_connection_catch2.cpp` target, the
`process-boundary` lane, ten exact requirement ids, six canonical sections
(`6.10`, `6.10.5`, `6.11.1`, `8.1.5`, `9.3`, and `9.5`), and the official
regional update/discovery/reflect API surfaces. The package gate and retained
package/JUnit evidence are already green. Query the completed slice and next
variant without reopening the Lab:

```powershell
python tools/query_rti_work.py next --summary --compact
python tools/query_rti_work.py work --summary --compact
```

The focused gate is:

```powershell
python tools/query_rti_work.py check --lane process-boundary --summary --compact
```

Trace the process local-delete slice without searching the service source:

```powershell
python tools/query_rti_work.py test "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py trace "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
python tools/query_rti_work.py test "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py trace "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
```

The local-delete rows map to IEEE 1516.1-2025 §6.18.1 and the same local-delete
API surface. The new receive-order Delete Object Instance rows map to §§6.16,
6.16.4, and 6.17.1; the codec row is private foundation evidence and the
integration row checks producer/recipient state plus the official removal
callback. Keep public conformance and interoperability promotion separate.
The 38-case query count is the unique mapped plan/test-declaration count; when
both the aggregate and dedicated connection executables are configured, the
same `[process-boundary]` CTest label intentionally runs both registrations.

Trace the directed-interaction callback slice directly:

```powershell
python tools/query_rti_work.py trace "RTIambassador delivers a directed interaction through the official Evoke callback surface" --summary --compact
python tools/query_rti_work.py lane timestamped-directed-retraction --summary --compact
python tools/query_rti_work.py lane federate.callback.request-retraction --summary --compact
python tools/query_rti_work.py lane callback-gating --summary --compact
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -R "^umbra_rti_package_process_directed_retraction_consumer$" --output-on-failure
ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process-directed-retraction --output-on-failure
```

For the embedded/public regional advisory slice, use the same bounded gate:

```powershell
python tools/query_rti_work.py check --lane regional-attribute-relevance --summary --compact
```

The embedded m35 regression has its own fast executable target:

```powershell
cmake --build <build-dir> --config Debug --target umbra_attribute_relevance_advisory_catch2
ctest --test-dir <build-dir> -C Debug -R "^umbra\.attribute_relevance_advisory\.catch2\." --output-on-failure
cmake --build <build-dir> --config Debug --target umbra_attribute_relevance_advisory_junit
```

For the filesystem-backed service-report store slice, use its isolated lane
gate and test target (ten source-located mapped cases, including the injected
memory seam as test-only evidence):

```powershell
python tools/query_rti_work.py lane service-report-store --summary --compact
python tools/query_rti_work.py coverage --lane service-report-store --summary --compact
python tools/query_rti_work.py check --lane service-report-store --summary --compact
ctest --test-dir <build-dir> -C Debug -R "^umbra\.service_report_store\.catch2\." --output-on-failure
cmake --build <build-dir> --config Debug --target umbra_service_report_store_junit
```
The JUnit artifact is `compliance/service-report-store/service-report-store.xml`
under the selected build directory.

Trace the store contract directly when changing allocation, append, or
failure behavior:

```powershell
python tools/query_rti_work.py trace umbra-service-report-file-lifecycle-allocates-one-joined-federate-file --summary --compact
python tools/query_rti_work.py trace umbra-service-report-file-lifecycle-switches-gate-appends-only --summary --compact
```

Trace the completed m24 transport projection directly:

```powershell
python tools/query_rti_work.py trace "Private process service projects owner-directed Attribute Relevance Advisory events" --summary --compact
```

Trace the completed m25 public callback bridge directly:

```powershell
python tools/query_rti_work.py trace "RTIambassador delivers Attribute Relevance Advisory callbacks through a configured process endpoint under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
```

Trace the callback-entry suppression seam directly:

```powershell
python tools/query_rti_work.py trace "Private process callback bridge suppresses a queued Attribute Relevance Advisory after switch disable" --summary --compact
```

Trace the public regional association transition directly:

```powershell
python tools/query_rti_work.py trace "RTIambassador delivers regional Attribute Relevance Advisory transitions through a configured process endpoint" --summary --compact
```

Trace the public regional subscription transition directly:

```powershell
python tools/query_rti_work.py trace "RTIambassador delivers regional Attribute Relevance Advisory subscription transitions through a configured process endpoint" --summary --compact
```

Trace the initial regional registration/discovery advisory directly:

```powershell
python tools/query_rti_work.py trace "RTIambassador delivers an initial regional Attribute Relevance Advisory after discovery through a configured process endpoint" --summary --compact
```

Trace the embedded advisory regression and its pinned 2025 mappings directly:

```powershell
python tools/query_rti_work.py trace "Embedded attribute relevance advisories follow scope transitions" --summary --compact
```

Trace the embedded rate-transition advisory variants directly:

```powershell
python tools/query_rti_work.py trace "Embedded attribute relevance advisories reissue turn-on when the active update rate changes" --summary --compact
python tools/query_rti_work.py trace "Embedded regional attribute relevance advisories retain explicit update-rate designators" --summary --compact
python tools/query_rti_work.py trace "RTIambassador reissues Attribute Relevance Advisory callbacks when the active update rate changes through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador preserves an explicit regional Attribute Relevance Advisory update-rate designator across configured process endpoint transitions" --summary --compact
python tools/query_rti_work.py trace "Embedded regional attribute relevance advisories recheck callbacks after an active-rate and scope transition under HLA_EVOKED and HLA_IMMEDIATE" --summary --compact
```

The current full plan has 839 rows. The unscoped gate still reports the known
historical backlog (63 source-unlocated rows and 94 without a requirement
mapping); that is visible source drift, not a reason to rescan the unchanged
Requirements Lab. The index records 276 C++ source declarations without a plan
row globally (0 in the FOM-composer file); use the bounded `unplanned --path`
command to reconcile one source slice. The
embedded Attribute Relevance Advisory regression is now
directly mapped to the same 2025 clauses as the private/public advisory slices.
Use `unlocated` or `unmapped` to take one queue item at a time; add
`--lane <exact-tag>` to keep a reconciliation queue inside one focus lane.

## Sources and handoff rule

- roadmap/work index: `docs/planning/ROADMAP-INDEX.json`
- C++ mapping plan: `compliance/requirements-lab/catch2-test-plan.json`
- pinned 2025 corpus: `.compliance/corpus-bundle.json`
- detailed query reference: `docs/planning/QUERY-GUIDE.md`
- long-form sequencing: `docs/planning/IMPLEMENTATION-PLAN.md`

Start with `work`, `trace`, and the lane-scoped `check`; open the long-form
documents only after the command output identifies the exact slice.
