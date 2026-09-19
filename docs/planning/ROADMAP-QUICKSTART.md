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

The written implementation plan is indexed separately, so its outline and
heading breadcrumbs are queryable without opening the full document:

```powershell
python tools/query_rti_work.py plan --summary --compact
python tools/query_rti_work.py plan "current indexed" --summary --compact
```

Use the reported line only after the bounded roadmap/lane query selects the
slice; the command does not print the plan prose.

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

The family `work` card includes live distinct-case, mapped-case, assertion,
requirement, canonical-subsection, and direct-pair totals. It also prints
copyable family crosswalks, so the implementation handoff and standards
mapping stay on one screen:

```powershell
python tools/query_rti_work.py matrix <family-id> --group-by requirement --summary --compact
python tools/query_rti_work.py matrix <family-id> --group-by section --summary --compact
```

The older `mapped work handles` line is retained for pointer compatibility;
use `work_mapping` for live family totals.

If the indexed queues are exhausted, `resume`/`dashboard` prints the indexed
active handoff when one is queued, otherwise a small family choice. The text
and JSON cards identify `recommended_family_id` and emit the exact
requirement- and subsection-grouped matrix commands for each choice, so the
mapping is available in the first bounded query. Do not replace that with a
repository-wide search. A family
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
python tools/query_rti_work.py matrix <family-or-lane> --group-by requirement --summary --compact
python tools/query_rti_work.py matrix <family-or-lane> --group-by section --summary --compact
python tools/query_rti_work.py requirement <lab-requirement-id> --summary --compact
python tools/query_rti_work.py section <document:clause-or-clause-number> --summary --compact
```

`requirement_section_mappings`/`requirement -> standard subsection` is the
direct pair list. It is not inferred by zipping separate requirement and
section arrays. A section query accepts either a canonical key such as
`hla-1516.1-2025:clause-8.18.1` or the exact clause shorthand `8.18.1`.
Use `matrix --group-by requirement` for a requirement-to-case crosswalk or
`matrix --group-by section` for a subsection-to-case crosswalk; both retain
bounded case ids and assertion totals.

For example, the embedded Connect configuration slice is already a complete,
bounded crosswalk: eight assertions, two direct Requirements-Lab-to-§4.2.4
pairs, and an exact source/CTest handle. Its companion optional-settings case
is five assertions with its own parse-failure requirement. These commands are
the intended resume path:

```powershell
python tools/query_rti_work.py case umbra-cpp-connect-configuration-fallback-integration --summary --compact
python tools/query_rti_work.py requirement requirement-candidate-content-clauses-04-federation-management-page-051-l23-4 --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-connect-configuration-fallback-integration --group-by requirement --summary --compact
python tools/query_rti_work.py section hla-1516.1-2025:clause-4.2.4 --summary --compact --limit 4
```

Keep this bounded query path separate from a Requirements-Lab resync; the
checked-in plan and pinned 2025 corpus are the sources for this crosswalk.

The current focused DDM lane is directly queryable without reopening the Lab:

```powershell
python tools/query_rti_work.py case umbra-cpp-regional-object-attribute-subscription-isolation-integration --summary --compact
python tools/query_rti_work.py focus regional-object-attribute-subscription-isolation --summary --compact
python tools/query_rti_work.py check --lane regional-object-attribute-subscription-isolation --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.regional_object_attribute_routing\.catch2\.Embedded regional object-attribute subscriptions remain independent from ordinary declarations$" --output-on-failure
python tools/query_rti_work.py gaps --family object-ddm-ownership --clause clause-9.8 --summary --compact --limit 8
```

This command is the short handoff for the ordinary/regional independence slice
(40 assertions, §9.8 lines 57 and 63). The companion regional advisory slice
is also directly queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-regional-declaration-relevance-advisory-integration --summary --compact
python tools/query_rti_work.py focus declaration-relevance-advisory-regional --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.regional_declaration_relevance_advisory\.catch2\.Embedded regional declaration relevance advisories follow active subscriptions$" --output-on-failure
python tools/query_rti_work.py case umbra-cpp-regional-passive-subscription-turn-updates-integration --summary --compact
python tools/query_rti_work.py focus regional-passive-subscription-turn-updates --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.regional_declaration_relevance_advisory\.catch2\.Embedded regional passive subscription changes gate registered-object turn updates per region$" --output-on-failure
```

It adds 61-assertion evidence for §9.8 lines 90, 102, and 120, plus a
54-assertion registered-object companion for the same passive/active triples
and Turn Updates On/Off callbacks. The filtered object-DDM §9.8 inventory is
now 20/20 mapped; timestamped/retraction paths remain a separate
implementation slice.

The ownership-assumption continuation family is also split into exact lanes,
so it can be run without searching the large federation-management source:

```powershell
python tools/query_rti_work.py focus resign-action-assumption-discovery-continuation --summary --compact
python tools/query_rti_work.py focus ownership-assumption-search-continuation --summary --compact
python tools/query_rti_work.py focus ownership-assumption-search-epoch --summary --compact
```

Use the corresponding `case` card for the source line, direct
Requirements-Lab-to-2025 subsection pairs, and exact CTest filter. These are
26-, 36-, and 39-assertion HLA_EVOKED development-profile slices.

The embedded RTI-owned MOM ownership-query baseline is a separate 17-assertion
lane. It does not claim the process endpoint's MOM establishment or report-file
behavior:

```powershell
python tools/query_rti_work.py case umbra-cpp-rti-owned-mom-ownership-query-integration --summary --compact
python tools/query_rti_work.py focus rti-owned-mom-ownership-query --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded RTI-owned MOM attributes participate in ownership queries$" --output-on-failure
```

If an exact handle is not known, use the indexed search only as a bounded
discovery fallback. A quoted phrase is matched as one phrase; multiple words
are ANDed by default, so this stays narrow and does not reopen the source tree:

```powershell
python tools/query_rti_work.py search restore work --summary --compact --limit 12
python tools/query_rti_work.py search restore --any-term --summary --compact --limit 12
```

Once a row is identified, switch back to `case`, `trace`, or `matrix` for the
exact requirement/section crosswalk and to `focus`/`check` for the execution
gate.

### Current completed slice: HLA_IMMEDIATE restored ownership-assumption work-item

The former active handoff is now a green, separately mapped process-boundary
C++ case for restored ownership-assumption delivery under `HLA_IMMEDIATE`.
Callbacks are disabled before divestiture/save, the pending reservation stays
durable through restore, and the public candidate receives exactly one
assumption callback after `Federation Restored` when the gate is reopened. It
has 52 assertions, nine Requirements-Lab requirements, seven canonical 2025
subsections, and 11 official C++ API surfaces; it does not claim Evoke Callback
behavior or the separate already-pushed work-item lane. Start with the exact
case/lane handles and do not reopen the Requirements Lab:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors deliver restored ownership-assumption work under HLA_IMMEDIATE through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --group-by requirement --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-immediate-integration --group-by section --summary --compact
python tools/query_rti_work.py focus process-federation-restore-work-item-ownership-assumption-immediate --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-work-item-ownership-assumption-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors deliver restored ownership-assumption work under HLA_IMMEDIATE through a configured process endpoint$" --output-on-failure
```

### Previous completed slice: HLA_IMMEDIATE restore abort

The latest green process-boundary slice runs a complete restore-abort lifecycle
under `HLA_IMMEDIATE`: requester success, restore begun, initiate restore,
abort, and federation not restored with `RESTORE_ABORTED`. It is a
25-assertion, six-requirement, six-subsection, 11-API foundation slice with no
`evokeCallback` call; the E_VOKED lifecycle, failed-request, success/failed
lifecycle, restore-status, work-item, and conformance lanes remain separate:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-abort-immediate-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve federation restore abort under HLA_IMMEDIATE through the configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-abort-immediate-integration --group-by requirement --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-abort-immediate-integration --group-by section --summary --compact
python tools/query_rti_work.py focus process-federation-restore-abort-immediate --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-abort-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve federation restore abort under HLA_IMMEDIATE through the configured process endpoint$" --output-on-failure
```

### Previous completed slice: HLA_IMMEDIATE successful restore lifecycle

The preceding green process-boundary slice runs a complete successful
federation restore lifecycle under `HLA_IMMEDIATE`: requester success, restore
begun, initiate restore, and federation restored. It remains a separate
24-assertion, six-requirement, six-subsection, 11-API foundation slice with no
`evokeCallback` call.

### Earlier completed slice: HLA_IMMEDIATE restore-failure lifecycle

The preceding failed lifecycle remains separately queryable as a 25-assertion,
six-requirement, six-subsection, 11-API foundation slice.

### Earlier completed slice: HLA_IMMEDIATE restore-request failure

The preceding request-failure boundary remains separately queryable as a
10-assertion, four-requirement, two-subsection, two-API foundation slice.

### Earlier completed slice: HLA_IMMEDIATE multi-federate federation-restore status

The latest green process-boundary slice enables `HLA_IMMEDIATE`, joins two
federates, and verifies requester-only restore success, per-recipient begin and
initiate callbacks, both recipients' in-progress/partial/terminal status
projections, and restored callbacks without any `evokeCallback` call. The
process service pushes each callback before the corresponding service response.
It is a 107-assertion, ten-requirement, eight-subsection, 13-API foundation
slice, separate from the HLA_EVOKED, single-federate, idle/terminal,
work-item, and conformance baselines:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-status-multi-federate-immediate-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve multi-federate federation restore status projections under HLA_IMMEDIATE through the configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-status-multi-federate-immediate-integration --group-by requirement --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-restore-status-multi-federate-immediate-integration --group-by section --summary --compact
python tools/query_rti_work.py focus process-federation-restore-status-multi-federate-immediate --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-status-multi-federate-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve multi-federate federation restore status projections under HLA_IMMEDIATE through the configured process endpoint$" --output-on-failure
```

### Latest completed slice: receiver-reporter multi-federate federation-save failure

The latest green process-boundary slice joins two federates, requests one
save, delivers `Initiate Federate Save` to both, and has the non-owner receiver
report `Federate Save Not Complete` first. Both participants receive
`Federation Not Saved` with `FEDERATE_REPORTED_FAILURE_DURING_SAVE`. It remains
separate from the single-federate case, owner-reporter ordering, terminal
status, timestamped replacement, status/abort, restore, push-mode, and
conformance:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-save-not-complete-receiver-reporter-integration --summary --compact
python tools/query_rti_work.py trace umbra-cpp-process-endpoint-federation-save-not-complete-receiver-reporter-integration --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-save-not-complete-receiver-reporter-integration --group-by requirement --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-federation-save-not-complete-receiver-reporter-integration --group-by section --summary --compact
python tools/query_rti_work.py focus process-federation-save-not-complete-receiver-reporter --summary --compact
python tools/query_rti_work.py check --lane process-federation-save-not-complete-receiver-reporter --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassadors preserve receiver-reported federation save not-complete across a configured process endpoint$" --output-on-failure
```

This is a 32-assertion, seven-requirement, five-subsection, six-API slice.
The preceding single-federate failure and timestamped save-replacement slices
remain independently queryable through their own lanes.

The preceding queued-TSO timed-save slice remains independently queryable via
`process-federation-save-timed-queued-tso`; it is not folded into replacement.

### Historical reference slice: synchronous ordinary delete-objects behavior

The newest native C++ transport slice is the ordinary two-member
`HLA_IMMEDIATE` companion for the configured `DELETE_OBJECTS` policy. It proves
the lost member's Connection Lost callback and the survivor's delete-privileged
object removal arrive synchronously at the transport-fault boundary:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-delete-objects-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-delete-objects-immediate --summary --compact
python tools/query_rti_work.py trace umbra-cpp-connection-lost-automatic-delete-objects-immediate --summary --compact
python tools/query_rti_work.py matrix connection-lost-automatic-delete-objects-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-delete-objects-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Immediate callbacks apply the configured automatic delete-objects directive synchronously$" --output-on-failure
```

This is a 25-assertion, five-requirement, five-section, three-API
development-profile slice. Keep the final-federate directive-two rule,
`DELETE_OBJECTS_THEN_DIVEST` ownership continuation, mixed cancellation matrix,
other automatic-resign directives, remote transport, and conformance as
separate lanes.

### Previous bounded handoff: synchronous cancel-delete-divest behavior

The preceding native C++ transport slice is the mixed HLA_EVOKED/HLA_IMMEDIATE
companion for the configured `CANCEL_THEN_DELETE_THEN_DIVEST` policy. It proves
the lost member's Connection Lost callback is immediate, the stale evoked owner
release is canceled before service, and the survivor observes delete-privileged
removal plus retained-object ownership-assumption delivery:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py trace umbra-cpp-connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py matrix connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-cancel-delete-divest-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Immediate callbacks apply the configured automatic cancel-then-delete-then-divest directive synchronously$" --output-on-failure
```

This is a 45-assertion, seven-requirement, seven-section, four-API
development-profile slice. Keep the HLA_EVOKED baseline, directive-4 and
final-federate companions, bounded `NO_ACTION` survivor policy, other
automatic-resign directives, remote transport, and conformance as separate
lanes.

### Earlier bounded handoff: synchronous delete-then-divest behavior

The preceding native C++ transport slice is the HLA_IMMEDIATE companion for the
configured `DELETE_OBJECTS_THEN_DIVEST` policy. It proves Connection Lost,
delete-privileged object removal, and the survivor's ownership-assumption offer
arrive at the transport-fault boundary while a non-delete-privileged object is
retained and divested:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-delete-then-divest-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-delete-then-divest-immediate --summary --compact
python tools/query_rti_work.py trace umbra-cpp-connection-lost-automatic-delete-then-divest-immediate --summary --compact
python tools/query_rti_work.py matrix connection-lost-automatic-delete-then-divest-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-delete-then-divest-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Immediate callbacks apply the configured automatic delete-then-divest directive synchronously$" --output-on-failure
```

This is a 39-assertion, five-requirement, five-section, three-API
development-profile slice. Keep the HLA_EVOKED baseline, final-federate rule,
bounded `NO_ACTION` survivor policy, directive-5 cancellation matrix, remote
transport, and conformance as separate lanes.

### Earlier bounded handoff: synchronous final-federate directive-two behavior

The earlier HLA_IMMEDIATE companion applies the final-federate directive-two
rule and proves object-name reuse after the final joined member is lost. Query it
with the exact handles:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-final-federate-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-final-federate-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-final-federate-immediate --summary --compact
```

### Earlier bounded handoff: synchronous NoAction forced-resign behavior

The earlier HLA_IMMEDIATE companion applies the bounded `NO_ACTION` forced-resign
policy and keeps the known object while suppressing automatic removal:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-no-action-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-no-action-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-no-action-immediate --summary --compact
```

### Earlier bounded handoff: synchronous pending-acquisition cancellation

The immediately preceding transport slice is the mixed HLA_EVOKED/HLA_IMMEDIATE
companion for `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-cancel-pending-acquisition-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.connection_loss_automatic_cancel_pending_acquisition\.catch2\.Immediate callbacks cancel a lost federate's pending ownership acquisition synchronously$" --output-on-failure
```

### Earlier bounded handoff: HLA_IMMEDIATE automatic-divestiture Connection Lost

The immediately preceding transport slice proves both the lost federate's
Connection Lost callback and the survivor's ownership-assumption offer are
delivered synchronously for `UNCONDITIONALLY_DIVEST_ATTRIBUTES`:

```powershell
python tools/query_rti_work.py case umbra-cpp-connection-lost-automatic-unconditional-divest-immediate --summary --compact
python tools/query_rti_work.py focus connection-lost-automatic-unconditional-divestiture-immediate --summary --compact
python tools/query_rti_work.py check --lane connection-lost-automatic-unconditional-divestiture-immediate --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.connection_loss_automatic_unconditional_divestiture\.catch2\.Immediate callbacks deliver automatic unconditional-divestiture Connection Lost synchronously$" --output-on-failure
```

### Earlier bounded handoff: durable pushed ownership-assumption callback under HLA_EVOKED

The newest native C++ process slice is indexed as a dedicated HLA_EVOKED
push-mode save/restore lane, with the no-save callback fence, already-pushed
HLA_IMMEDIATE save/restore case, evoked public delivery case, and private
registry seam retained as neighboring lanes. Start there
instead of searching the source tree or re-reading the unchanged Lab:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-pushed-ownership-assumption-evoked-save-restore-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve an unconsumed pushed ownership-assumption callback across HLA_EVOKED save and restore" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors preserve an unconsumed pushed ownership-assumption callback across HLA_EVOKED save and restore$" --output-on-failure
```

The latest case is the bounded durable pending-callback slice. Its lane and
crosswalk are the single query path for the pushed frame that remains
uninvoked through save and restore:

```powershell
python tools/query_rti_work.py focus process-pushed-ownership-assumption-evoked-save-restore --summary --compact
python tools/query_rti_work.py matrix process-pushed-ownership-assumption-evoked-save-restore --summary --compact
python tools/query_rti_work.py check --lane process-pushed-ownership-assumption-evoked-save-restore --summary --compact
```

The no-save HLA_EVOKED callback fence remains separately queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-pushed-ownership-assumption-evoked-integration --summary --compact
python tools/query_rti_work.py focus process-pushed-ownership-assumption-evoked --summary --compact
python tools/query_rti_work.py matrix process-pushed-ownership-assumption-evoked --summary --compact
python tools/query_rti_work.py check --lane process-pushed-ownership-assumption-evoked --summary --compact
```

The already-pushed save/restore companion remains directly queryable:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-push-integration --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve pushed ownership-assumption delivery across save and restore" --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.attribute_ownership_acquisition\.catch2\.RTIambassadors preserve pushed ownership-assumption delivery across save and restore$" --output-on-failure
python tools/query_rti_work.py focus process-federation-restore-work-item-ownership-assumption-push --summary --compact
python tools/query_rti_work.py matrix process-federation-restore-work-item-ownership-assumption-push --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-work-item-ownership-assumption-push --summary --compact
```

The evoked public delivery companion and private registry seam remain directly
queryable:

```powershell
python tools/query_rti_work.py focus process-federation-restore-work-item-ownership-assumption --summary --compact
python tools/query_rti_work.py matrix process-federation-restore-work-item-ownership-assumption --summary --compact
python tools/query_rti_work.py check --lane process-federation-restore-work-item-ownership-assumption --summary --compact
python tools/query_rti_work.py case umbra-cpp-process-endpoint-federation-restore-work-item-ownership-assumption-integration --summary --compact
python tools/query_rti_work.py case umbra-cpp-federation-save-commit-process-local-ownership-assumption-work-unit --summary --compact
```

The latest card is a 73-assertion HLA_EVOKED process-endpoint test with ten
official C++ API surfaces, nine direct Requirements-Lab-to-2025 requirement
pairs, and seven canonical sections (§4.19, §4.21.1, §4.22, §4.23, §4.32,
§7.1.1, and §7.2). It proves pushed discovery and ownership-assumption
callbacks remain uninvoked through save and restore, then arrive in order after
Federation Restored. The no-save HLA_EVOKED fence is a 33-assertion companion
with four API surfaces and two sections. The already-pushed companion
is a 55-assertion HLA_IMMEDIATE process-endpoint test with eight
official C++ API surfaces, nine direct Requirements-Lab-to-2025 requirement
pairs, and seven canonical sections (§4.19, §4.21.1, §4.22, §4.23, §4.32,
§7.1.1, and §7.2). It proves pushed ownership-assumption delivery remains
single-shot across an untimed save/restore. The 46-assertion HLA_EVOKED public
case proves durable route-free work-item delivery; the 35-assertion private
registry case remains the lower-level seam. Restart/rebind and the other
restore work-item families remain separate lanes.

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
# Broad tags are useful for taxonomy review; focused aliases are the small
# family-owned lane list for selecting the next slice.
python tools/query_rti_work.py lanes --family transport-and-conformance --focused --summary --compact --limit 12
```

`unclassified` means a mapping decision is still needed. `explicit` means the
case intentionally has no standalone Lab requirement. Source-only and older
fixtures are reconciliation diagnostics; they do not become 2025 evidence
until a plan row carries explicit requirements and sections.

`lanes --family <id>` normally includes every broad cross-cutting Catch2 tag,
which is useful for taxonomy review but can be noisy. Add `--focused` to use
only that family's indexed `focused_lane_tags`; this produces the bounded
lane-selection list and keeps the broad tag inventory out of the normal work
loop.

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

The indexed snapshot count changes as bounded C++ slices land; use the live
`resume` or `dashboard` card for the current Catch2/mapping totals and
assertion totals, including the separately queryable process-boundary lanes.
The live `check` reports
64 intentional explicit-disposition rows and zero unclassified mappings; use
the exact commands below for one slice instead of reopening the full plan.

The newest bounded runtime correction is the timestamped process Delete Object
Instance path at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:9166`. Its 283 assertions map
14 Lab requirements to eight canonical 2025 sections and seven official C++ API
surfaces. The regulated and non-regulated variants preserve the process queue
identity while exposing a public `MessageRetractionHandle` only for the
time-regulating producer; both callback models prove queued
`TIMESTAMP/TIMESTAMP` delivery to a constrained recipient, and preferred
receive-order variants prove `RECEIVE/RECEIVE` delivery without retraction.
Query the exact crosswalk without scanning the catalog:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --group-by requirement --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --group-by section --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
```

The newest completed foundation slice is the process-boundary directed
transportation query/report case at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27231`. It carries 44
assertions under both callback models, maps 17 direct Lab requirements to ten
canonical 2025 sections, and exercises ten official C++ API surfaces. The
configured endpoint preserves the publisher-scoped `HLAbestEffort` override
through the confirmation and report callbacks. Query it without reopening the
plan:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py focus process-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py trace "RTIambassador reports a directed interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py check --lane process-directed-interaction-transportation-query --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador reports a directed interaction transportation override through a configured process endpoint$" --output-on-failure
```

The registry query/report foundation remains independently queryable as
`directed-interaction-transportation-query` at
`cpp/tests/federation_registry_catch2.cpp:511`.

The newest process-boundary slice is independently queryable as
`process-directed-interaction-transportation-query` at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27231`. It carries 44
assertions under both callback models, maps 17 direct Lab requirements to ten
canonical 2025 sections, and exercises ten official C++ API surfaces. It
proves that the directed publisher's `HLAbestEffort` override survives the
configured endpoint and is returned through the report callback:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py focus process-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py trace "RTIambassador reports a directed interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix process-directed-interaction-transportation-query --summary --compact
python tools/query_rti_work.py check --lane process-directed-interaction-transportation-query --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador reports a directed interaction transportation override through a configured process endpoint$" --output-on-failure
```

The preceding process-boundary transportation-delivery slice remains
independently queryable as `process-directed-interaction-transportation` and
continues to carry 66 assertions under both callback models.

The timestamped process Delete Object Instance baseline is also independently
queryable through the process-boundary lane at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:9166`. It carries 283
assertions across regulated and non-regulated variants under both callback
models, maps 14 direct Lab requirements to eight canonical 2025 sections, and
exercises seven official C++ API surfaces. It returns a valid public
`MessageRetractionHandle` only for the regulated producer and includes one
queued constrained-recipient and preferred receive-order variants under both
callback models. It remains private foundation evidence while broader regional
or directed delivery are developed separately:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --summary --compact
python tools/query_rti_work.py test "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-timestamped-delete-object-instance-integration --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\\.ieee1516_2025\\.connection_catch2\\.RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint$" --output-on-failure
```

The preceding two-recipient directed interaction lane remains separately
queryable as `process-directed-interaction-multi-recipient`; it carries 104
assertions and the broader Receive Directed Interaction callback fan-out
evidence. Keep that lane separate from transportation override behavior.

The preceding `process-transportation-interaction-control` slice routes one
published interaction class's transportation-type change and current-type
query through the public process endpoint under both HLA_EVOKED and
HLA_IMMEDIATE. It carries 40 assertions, ten direct Lab requirements, four
canonical 2025 sections, and 9 official C++ API surfaces. The preceding
instance transportation slice is also independently queryable; it carries 30
assertions, five direct Lab requirements, four canonical 2025 sections, and 10
official C++ API surfaces.
The default-transport, directed TSO,
multi-recipient fanout, multiple-message FIFO, changed-lookahead,
timestamped-attribute, retraction-lifetime, restore-baseline, regional,
suppression, and single-receiver ordering cases remain separately queryable.

The reservation/object-management process slice is also independently
addressable. It reserves two names, drives the official single and multiple
reservation callbacks, verifies a partial StringSet success/failure split,
consumes one reservation through named registration, releases the single and
multiple reservations through the same process-owned ledger, and verifies the
repeated-release errors under both callback models. It carries 77 assertions,
13 direct Lab requirements, eight canonical 2025 sections, and 11 official C++
API surfaces:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-named-object-registration-integration --summary --compact
python tools/query_rti_work.py focus process-object-instance-name-reservation --summary --compact
python tools/query_rti_work.py focus process-boundary --summary --compact
python tools/query_rti_work.py trace "RTIambassador reserves and releases a name and registers a named object through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-named-object-registration-integration --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassador reserves and releases a name and registers a named object through a configured process endpoint$" --output-on-failure
```

The preceding companion is `process-transportation-regional-interaction-control`
at `cpp/tests/ieee1516_2025_connection_catch2.cpp:27247`. It carries 84
assertions, 23 direct Lab requirements, 11 canonical 2025 sections, and 17
official C++ API surfaces. The publisher commits an HLAbestEffort override,
sends a regional MainCourseServed interaction through the configured process
endpoint, and the receiver verifies the effective transport plus source-region
metadata under both callback models. Query the slice directly:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-regional-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-regional-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve a regional interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-regional-interaction-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-regional-interaction-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassadors preserve a regional interaction transportation override through a configured process endpoint$" --output-on-failure
```

The timestamped regional transportation companion is at
`cpp/tests/ieee1516_2025_connection_catch2.cpp:27745`. It carries 122
assertions, 27 direct Lab requirements, 15 canonical 2025 sections, and 21
official C++ API surfaces. The publisher's confirmed HLAbestEffort override
survives a timestamped regional send through the configured process endpoint;
the receiver verifies TIMESTAMP classifications, retraction identity,
callback-before-grant ordering, and conveyed source-region metadata under both
callback models. Query it directly:

```powershell
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-timestamped-regional-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-timestamped-regional-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassadors preserve a timestamped regional interaction transportation override through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-timestamped-regional-interaction-integration --summary --compact
python tools/query_rti_work.py check --lane process-transportation-timestamped-regional-interaction-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.RTIambassadors preserve a timestamped regional interaction transportation override through a configured process endpoint$" --output-on-failure
```

The latest verified implementation fix is the negotiated-ownership restore
boundary. Fresh-registry restore now accepts the owner-side assumption ledgers
that are seeded by negotiated divestiture, includes those ledgers in the
pending-operation fence, and preserves their queued callback tuples. It is
covered by 8 aggregate registry cases, the same 8 standalone registry cases,
8 public fresh-registry cases, and 2 standalone unowned-assumption regression
cases (26 focused C++ runs). These rows already carry direct
Requirements-Lab-to-2025-subsection mappings; query one exact row or the
bounded focus lane instead of searching the whole save/restore plan:

```powershell
python tools/query_rti_work.py case umbra-cpp-federation-save-commit-filesystem-process-restart-pending-negotiated-owner-confirmation-unit --summary --compact
python tools/query_rti_work.py focus process-restart-negotiated-confirmation-delivered --summary --compact
python tools/query_rti_work.py matrix process-restart-attribute-ownership-mixed-negotiated-ownership --summary --compact
python tools/query_rti_work.py check --lane federation-registry --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.(Filesystem state image restores pending negotiated|Filesystem state image preserves delivered negotiated|Filesystem state image restores a mixed negotiated|Filesystem state image restores an asymmetric mixed negotiated|Filesystem state image restores the reverse asymmetric mixed negotiated|Filesystem state image preserves mixed delivered negotiated)" --output-on-failure
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.catch2\.Embedded public fresh-registry restore (rebinds|preserves).*negotiated" --output-on-failure
```

The machine-readable copy lives at `mapping.latest_verified_restore_slice` in
`docs/planning/ROADMAP-INDEX.json`; `resume` and `dashboard` surface its
bounded verification card automatically. This is development-profile C++
evidence only, not Lab validation, interoperability, package/JUnit,
protected-review, or conformance evidence.

```powershell
python tools/query_rti_work.py roadmap process-transportation-interaction-control --summary --compact
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-interaction-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-interaction-control --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes interaction transportation type change and query through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-interaction-integration --summary --compact
python tools/query_rti_work.py recent --lane process-transportation-interaction-control --summary --compact --limit 2
python tools/query_rti_work.py check --lane process-transportation-interaction-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador routes interaction transportation type change and query through a configured process endpoint$" --output-on-failure

python tools/query_rti_work.py roadmap process-transportation-instance-control --summary --compact
python tools/query_rti_work.py case umbra-cpp-process-endpoint-transportation-instance-integration --summary --compact
python tools/query_rti_work.py focus process-transportation-instance-control --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes instance transportation type change and query through a configured process endpoint" --summary --compact
python tools/query_rti_work.py matrix umbra-cpp-process-endpoint-transportation-instance-integration --summary --compact
python tools/query_rti_work.py recent --lane process-transportation-instance-control --summary --compact --limit 2
python tools/query_rti_work.py check --lane process-transportation-instance-control --summary --compact
ctest --test-dir .build -C Debug -R "^umbra\.ieee1516_2025\.connection_catch2\.RTIambassador routes instance transportation type change and query through a configured process endpoint$" --output-on-failure
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
