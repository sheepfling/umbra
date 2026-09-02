# Developer tools

Run these utilities from the repository root. They keep generation, standards
verification, test-lane checks, and compliance-sidecar work out of the runtime
libraries.

For command arguments, use:

    python tools/<tool-name>.py --help

## Common recipes

### Run a route-aware CI lane

The Python-first `tools.ci` module is the easiest path for a junior
developer or a hosted runner. It fans out by standard and transport without
requiring PowerShell:

    python -m tools.ci list
    python -m tools.ci test --standard 2010 --route cpp
    python -m tools.ci test --standard 2025 --route java
    python -m tools.ci test --standard 2010 --route java --api-jar C:\path\to\api.jar --provider-jar C:\path\to\vendor.jar --dependency-jar C:\path\to\vendor-dependency.jar --factory-name "Vendor RTI"
    python -m tools.ci test --standard 2010 --route jni --api-jar C:\path\to\api.jar
    python -m tools.ci test --standard all --route all

Run the repository hygiene commands from the same entry point:

    python -m tools.ci lint
    python -m tools.ci fix --scope changed
    python -m tools.ci clean --dry-run

The 2010 C++ lane includes the exact integer/float logical-time marshal gate.
The 2010 JNI lane runs the Java carrier matrix; after staging the optional
native Python extension, the Python-side boundary evidence is:

    python -m pytest -q packages/umbra-rti-native/tests/test_native_2010.py
    python -m unittest packages/umbra-rti-jpype/tests/test_jpype_2010_jni_type_roundtrip.py -v

When the corresponding `UMBRA_ENABLE_*` switch is set, the route-aware Python
command also includes that opt-in JVM/JNI suite automatically; with no switch,
the default route remains independent of optional toolchains and artifacts.

The 2010 Java API and provider archives are intentionally caller-supplied.
Use `--dependency-jar` for provider libraries that are not bundled with the
provider JAR. See the
[CI contract](../docs/development/CI.md) for provider JAR, FOM, MIM, and
capability-profile options.

### Check a native change

Use the repeatable baseline first. CI services delegate to this exact command;
it can also rerun one named stage after an initial pass:

    python tools/ci.py native
    python tools/ci.py native --stage test
    python tools/ci.py --list-profiles

For a focused Catch2 domain test, configure the Catch2 profile once, then
filter by its test name:

    python tools/ci.py native-catch2 --stage configure
    python tools/ci.py native-catch2 --stage build
    ctest --test-dir out/cmake/catch2 -C Debug -R federation_registry --output-on-failure

### Inspect or run one tool

Do this before using a tool that can write files:

    python tools/tool_name.py --help

The command help is the authority for arguments. Send ordinary reports and
plots to out/; do not write generated output into a source directory.

### Regenerate a checked-in source artifact

The private fallback ambassador is the only generated source artifact in the
native tree:

    python tools/generate_rti_ambassador_shell.py --help

Run it only when the pinned IEEE binding inventory changes, then review the
generated diff and run the native baseline.

### Work with Requirements Lab inputs

For day-to-day implementation selection, start with the short
[roadmap query card](../docs/planning/QUERY-CARD.md); use the detailed
[roadmap and test query guide](../docs/planning/QUERY-GUIDE.md) only when the
bounded command output points to a deeper reference:

    python tools/query_rti_work.py work --summary --compact
    python tools/query_rti_work.py focus --summary --compact
    python tools/query_rti_work.py next --summary
    python tools/query_rti_work.py recent --summary --compact --limit 20
    python tools/query_rti_work.py trace "RTIambassador removes a regional subscription through a configured process endpoint" --summary
    python tools/query_rti_work.py lane request-retraction-suppressed-interaction --summary --compact
    python tools/query_rti_work.py test "Embedded suppressed timestamped interaction callback does not request retraction" --summary --compact
    python tools/query_rti_work.py lane transport --summary --compact
    python tools/query_rti_work.py test "Private process transport exchanges framed data after endpoint handshake" --summary --compact
    python tools/query_rti_work.py test "Private process service binds create join and receive-order interaction to the federation registry" --summary --compact
    python tools/query_rti_work.py test "Private registry-bound service exchanges federation traffic across independently launched processes" --summary --compact
    python tools/query_rti_work.py lane process-boundary --summary --compact
    python tools/query_rti_work.py lane callback-controls --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public Send Interaction through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador resolves interaction and parameter handles through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador publishes object-class attributes and registers an object through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador reserves a name and registers a named object through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes ordinary interaction declarations through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador receives a process interaction through the official Evoke callback surface" --summary --compact
    python tools/query_rti_work.py test "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface" --summary --compact
    python tools/query_rti_work.py test "RTIambassador projects the 2025 region lifecycle and regional registration through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes a remote regional subscription and scoped update through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador preserves a timestamped regional update through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador removes a regional subscription through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador suppresses a disjoint regional update through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "Private process service routes ordinary Update Attribute Values to a subscribed receiver" --summary --compact
    python tools/query_rti_work.py lane rti.service.subscribe-object-class-attributes --summary --compact
    python tools/query_rti_work.py test "RTIambassador routes public Update Attribute Values through a configured process endpoint" --summary --compact
    python tools/query_rti_work.py test "RTIambassador delivers a process Update Attribute Values event through the official Reflect callback" --summary --compact
    python tools/query_rti_work.py coverage --lane process-boundary --summary --compact
    cmake --build <build-dir> --config Debug --target umbra_test_installable_package
    ctest --test-dir <build-dir>/package-smoke-consumer -C Debug -L package-process --output-on-failure
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
    python tools/verify_process_package_lanes.py --ctest ctest --test-dir <build-dir>/package-smoke-consumer --index docs/planning/ROADMAP-INDEX.json --config Debug
    python tools/query_rti_work.py test "Embedded transport loss applies the bounded automatic NoAction forced-resign policy" --summary --compact
    python tools/query_rti_work.py next --summary --compact
    python tools/query_rti_work.py focus process-boundary --summary --compact
    python tools/query_rti_work.py test "Embedded terminal timestamped deletion tombstone releases its object name" --summary --compact
    python tools/query_rti_work.py test "Embedded Flush Queue Request admits TSO input queued after submission" --summary --compact
    python tools/query_rti_work.py test "Embedded timestamped Update Attribute Values flushes queued passels with optimistic time" --compact
    python tools/query_rti_work.py test "Embedded timestamped Update Attribute Values honors available and next-message available grants" --compact
    python tools/query_rti_work.py test "Embedded timestamped regional Update Attribute Values carries recipient-gated regions across mixed fanout" --compact
    python tools/query_rti_work.py test "Embedded regional timestamped attribute updates deliver before TAR and NMR grants" --compact
    python tools/query_rti_work.py recent --lane regional-automatic-provision-switch-mutation --summary --compact --limit 5
    python tools/query_rti_work.py check --compact

Use `coverage --lane <exact-tag> --summary --compact` to see the bounded
requirement, subsection, and source-location counts for one focus lane; the
source-location count keeps retained historical rows distinct from executable
Catch2 cases.

The Lab is optional for ordinary source work. When a requirement mapping or
source-path contract changes, start with:

    python tools/requirements_lab.py --help
    python tools/requirements_lab_sidecar.py --help

Keep exports and unreviewed evidence under .compliance/. Read the
[compliance workflow](../docs/testing/COMPLIANCE-WORKFLOW.md) before promoting
or describing any evidence.

Only when the pinned export changes, or when a numbering/semantic discrepancy
is under review, compare a fresh edition-scoped export with the pinned bundle
using the read-only re-sync report. Ordinary implementation work should stay
on the checked-in index and mappings above:

    python tools/requirements_lab.py resync \
      --baseline .compliance/corpus-bundle.json \
      --candidate .tmp/corpus-bundle-resync-2026-08-24.json \
      --edition 2025 --fail-on-diff

The report includes normalized document digests and separate content,
regenerated-ID, and ordinal-numbering deltas. An ordinal move is reported even
when the normative text and opaque record ID stay unchanged; this prevents a
requirements-numbering change from disappearing inside the semantic
fingerprint. Each selected document is also checked for malformed requirement
ordinals (missing, invalid, duplicate, or non-contiguous values), and those
findings are included in the report's `ordinal_integrity` fields. This catches
an exporter numbering defect even when there is no trustworthy pairwise record
to compare. `--fail-on-diff` is suitable for a review/CI gate; without it,
the command always prints the complete report and exits zero. When an
intentional working-tree overlay adds semantic records, use
`--fail-on-numbering-drift` to gate only regenerated IDs, ordinal moves, or
ordinal-integrity findings. The focused offline guard also exercises the
ordinal-only, combined ID/ordinal, and malformed-ordinal cases:

    python tools/requirements_lab_resync_regression.py

The active native Catch2 planning catalog has its own non-mutating reference
guard. It resolves every supplied requirement/API ID across the 2025 bundle
and checks each human-readable selector against the C++ test sources:

    python tools/requirements_lab.py check-plan \
      --bundle .compliance/corpus-bundle.json

The observation ledger has a separate guard. It freezes the RL-001..RL-157
historical section blocks and requires every post-RL-157 recurrence (including
an old issue found still unresolved/not fixed, or a failed expected mitigation)
to use the next identifier and cite an existing earlier observation. The
command prints that next local identifier:

    python tools/requirements_lab.py check-observations

An unchanged export or additive test slice is recorded as an audit/local
coverage note and does not consume the next recurrence identifier. Only a
reproduced Lab or Umbra-consumer go-back should append the next `RL-###`
entry.

### Query the implementation roadmap and C++ mappings

Use the read-only work index for a short next-step report and exact test-to-
standard lookup. It joins the pinned Catch2 plan to the pinned 2025 bundle's
clause/subsection and source metadata; it does not resync or modify the Lab:
`next --summary` includes the exact next work query, lane, focused test selector,
package target/test/label handles, canonical 2025 `standard_sections`, and
mapping counts; `next --json` exposes the
same bounded object to scripts. Run `check --compact` before a focused build to
catch stale test/source, requirement, section, or lane handles. Its text output
prints only a small sample when the checkout has many drift rows; use
`check --json` for the complete diagnostic. If the selector is an existing
regression used as the starting point for a new slice, it is labeled
`baseline_test` and reports `test_pointer=complete`; it is not presented as a
test to add or rerun. For an architectural slice that still needs its first
test, it reports the explicit `next_work_id`, `next_work_status`, and
`next_work_query` instead of inventing a selector. A completed, unlabeled
selector is reported as a roadmap-index warning by `check`, so stale pointers
do not silently become the work queue. `recent --lane <exact-tag>` narrows the
completion ledger to one focus lane. `status --compact` and `item --compact`
keep broad roadmap items bounded by reporting tag/match counts rather than
expanding every tag.

Follow `work` with `focus [<exact-catch2-tag>] --summary --compact` to get a
bounded lane decision card: implemented cases, source-located executable
candidates, historical source drift, requirement/section counts, and the
copyable Catch2/CTest/JUnit handles. With no tag, `focus` follows the active
indexed lane. Use `focus --json` for automation; it never triggers a Lab scan.

For an iteration-local integrity gate, add `check --lane <exact-tag>`; it
limits test/source and recent-slice validation to that lane and reports any
unlocated historical rows as visible source drift. The unscoped `check` remains
the strict whole-plan reconciliation check.

`test "<exact TEST_CASE title>" --summary --compact` is the bounded traceability
view: it prints the source location, API-surface IDs, service/callback tags,
Requirements-Lab IDs, and canonical 2025 section handles without expanding the
full plan. For a direct one-record relationship, use `trace`: it accepts an
exact plan id/title, Lab requirement id, canonical 2025 section key, or exact
Catch2 lane tag and prints `lab_requirement_id -> document_id:clause_id` rows
beside the C++ source location. It never falls back to fuzzy search; use
`search` explicitly for discovery. Use `requirement` or `section` for the
reverse lookup from a Lab requirement or standard subsection to its mapped
tests.

    python tools/query_rti_work.py trace umbra-cpp-process-endpoint-regional-unsubscribe-integration --json
    python tools/query_rti_work.py trace hla-1516.1-2025:clause-9.7.5 --summary --compact

    python tools/query_rti_work.py status --compact
    python tools/query_rti_work.py work --summary --compact
    python tools/query_rti_work.py next --compact
    python tools/query_rti_work.py next --summary
    python tools/query_rti_work.py next --json
    python tools/query_rti_work.py check --lane transport --summary --compact
    python tools/query_rti_work.py check --compact
    python tools/query_rti_work.py item multi-federate-callback-ordering --compact
    python tools/query_rti_work.py plan
    python tools/query_rti_work.py coverage
    python tools/query_rti_work.py lane multi-federate-callback-ordering
    python tools/query_rti_work.py lane durable-save
    python tools/query_rti_work.py lane state-image --compact
    python tools/query_rti_work.py lane tso-directed-interaction-state --compact
    python tools/query_rti_work.py lane tso-attribute-update-state --compact
    python tools/query_rti_work.py lane tso-object-deletion-state --compact
    python tools/query_rti_work.py lane timestamped-regional-attribute-update --compact
    python tools/query_rti_work.py lane process-restart-regional-pending-attribute-value-update-provider-departure --summary
    python tools/query_rti_work.py lane public-process-restart-regional-pending-attribute-value-update-provider-departure --summary
    python tools/query_rti_work.py lane timestamped-regional-request-provider-response --compact
    python tools/query_rti_work.py test "Embedded regional Request Attribute Value Update supports a timestamped provider response" --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-resignation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-live-restore-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-live-resignation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-live-restore-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-live-resignation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-multi-resignation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-delete-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-if-available-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-continuation-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-confirmation-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-mixed-candidate-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-mixed-confirmation-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-mixed-pre-delivery-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-retained-regular-pre-delivery-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-retained-regular-confirmation-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-regular-candidate-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-if-available-regular-candidate-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-regular-retained-confirmation-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-negotiated-regular-retained-pre-delivery-cancel-state --compact
    python tools/query_rti_work.py lane tso-regional-attribute-update-timed-resignation-matrix --summary
    python tools/query_rti_work.py lane timestamped-regional-attribute-timed-restore-multi-recipient --compact
    python tools/query_rti_work.py lane process-restart --summary
    python tools/query_rti_work.py lane process-restart-directed-interaction-declaration --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-declaration --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-routing --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-routing --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-ownership-handoff --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-ownership-handoff --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore follows directed by-ownership target handoff and receive-order delivery under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-tso-ownership-callback --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-callback --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses stale directed TSO callback after by-ownership handoff under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore delivers eligible directed TSO and issues Request Retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-directed-interaction-tso-fanout-positive-negative --compact
    python tools/query_rti_work.py test "Filesystem fresh-registry directed TSO fan-out preserves an eligible recipient and suppresses an unsubscribed recipient" --compact
    python tools/query_rti_work.py lane process-restart-pending-attribute-value-update --compact
    python tools/query_rti_work.py lane public-process-restart-pending-attribute-value-update --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending object-instance Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-class-pending-attribute-value-update --compact
    python tools/query_rti_work.py lane public-process-restart-class-pending-attribute-value-update --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending object-class Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-regional-pending-attribute-value-update --compact
    python tools/query_rti_work.py lane process-restart-regional-pending-attribute-value-update-negative --summary
    python tools/query_rti_work.py lane public-process-restart-regional-pending-attribute-value-update --compact
    python tools/query_rti_work.py lane public-process-restart-regional-pending-attribute-value-update-negative --summary
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending regional object-class Request Attribute Value Update through HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py section hla-1516.1-2025:clause-9.13.1 --summary --limit 20
    python tools/query_rti_work.py lane timestamped-default-region-attribute-retract-alternate-advance --compact
    python tools/query_rti_work.py test "Embedded timestamped default-region attribute updates retract before TARA and NMRA grants" --compact
    python tools/query_rti_work.py lane time-advance-request-available --summary
    python tools/query_rti_work.py lane process-restart-directed-interaction-tso-parameter-projection --compact
    python tools/query_rti_work.py lane process-restart-regional-interaction-tso-ddm --compact
     python tools/query_rti_work.py test "Filesystem fresh-registry restore preserves one timestamped regional interaction payload and source-region snapshot" --compact
     python tools/query_rti_work.py lane public-process-restart-regional-interaction-tso-ddm --compact
     python tools/query_rti_work.py test "Embedded public restore rehydrates queued timestamped regional interaction and preserves filesystem service-report identity" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds queued timestamped regional interaction and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-regional-attribute-update-tso-ddm --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds queued timestamped regional attribute update and preserves report-file lifetimes" --compact
     python tools/query_rti_work.py lane durable-save-regional-pending-attribute-value-update-response-retraction --compact
     python tools/query_rti_work.py unlocated --summary --limit 20
     python tools/query_rti_work.py lane durable-save-regional-pending-attribute-value-update-regular-response --compact
     python tools/query_rti_work.py test "Embedded regional Request Attribute Value Update supports a 2025 provider response" --compact
     python tools/query_rti_work.py lane durable-save-regional-pending-attribute-value-update-regular-response-no-replay --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore does not replay a delivered receive-order regional provider response under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py test "Embedded object-class Request Attribute Value Update supports a timestamped provider response under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py test "Embedded transport loss drains a cutoff timestamped deletion to each pending survivor" --compact
     python tools/query_rti_work.py test "Embedded local deletion isolates queued timestamped attribute deliveries per recipient" --compact
     python tools/query_rti_work.py test "Embedded local deletion isolates queued timestamped object removals per recipient" --compact
     python tools/query_rti_work.py lane regional-automatic-provision --compact
     python tools/query_rti_work.py test "Embedded regional Auto Provide solicits overlap-qualified owners and suppresses stale disjoint work" --compact
     python tools/query_rti_work.py lane regional-automatic-provision-immediate --compact
     python tools/query_rti_work.py test "Embedded regional Auto Provide preserves synchronous discovery-before-provider ordering under HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane regional-automatic-provision-response --compact
     python tools/query_rti_work.py test "Embedded regional Auto Provide provider response reflects one scoped value under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane regional-automatic-provision-timestamped-response --compact
     python tools/query_rti_work.py test "Embedded regional Auto Provide timestamped response reflects once and exposes valid retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-multi-provider --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide fans out one request across two provider owners under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-switch-mutation --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide suppresses queued work after switch mutation and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-timestamped-switch-mutation --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide preserves a queued timestamped response across switch mutation and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-switch-admission-mutation --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide consumes queued discovery work when switch changes before provider admission and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-timestamped-switch-admission-mutation --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide fences timestamped discovery work before provider admission and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-multi-source-region --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide separates independent source regions and provider callbacks under HLA_EVOKED and HLA_IMMEDIATE" --compact
      python tools/query_rti_work.py lane regional-automatic-provision-relaxed-ddm --compact
      python tools/query_rti_work.py test "Embedded regional Auto Provide applies Allow Relaxed DDM to touching source projections and suppresses positive gaps under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-object-deletion-tso --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds queued timestamped object deletion and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-object-deletion-retraction --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore retracts delivered timestamped object deletion for one recipient under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-interaction-declaration --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rehydrates mixed interaction declaration and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-fanout --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds directed TSO fan-out and retracts only the delivered recipient under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-parameter-projection --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry parameterized directed TSO preserves parameter projection and timestamped order metadata under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-attribute-ownership-acquisition --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending regular ownership release callback and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-attribute-ownership-acquisition-if-available --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending If Available ownership callback and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-pending-attribute-ownership-query --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds a pending Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-pending-attribute-ownership-query-unowned --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds a pending unowned Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-pending-rti-owned-attribute-ownership-query --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds a pending RTI-owned Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-attribute-ownership-negotiated-owner-confirmation --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending negotiated owner confirmation and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-attribute-ownership-negotiated-if-available-owner-confirmation --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending negotiated If Available owner confirmation and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-attribute-ownership-mixed-negotiated-ownership --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds mixed negotiated ownership confirmations and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-negotiated-confirmation-delivered --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves delivered negotiated owner confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane public-process-restart-negotiated-if-available-confirmation-delivered --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves delivered negotiated If Available confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-attribute-ownership-negotiated-owner-confirmation --compact
    python tools/query_rti_work.py test "Filesystem state image restores pending negotiated owner confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-attribute-ownership-negotiated-if-available-owner-confirmation --compact
    python tools/query_rti_work.py test "Filesystem state image restores pending negotiated If Available owner confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-attribute-ownership-mixed-negotiated-ownership --compact
    python tools/query_rti_work.py test "Filesystem state image restores a mixed negotiated ownership ledger in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-negotiated-confirmation-delivered --compact
    python tools/query_rti_work.py test "Filesystem state image preserves delivered negotiated owner confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-negotiated-if-available-confirmation-delivered --compact
    python tools/query_rti_work.py test "Filesystem state image preserves delivered negotiated If Available confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane process-restart-mixed-confirmation-delivered --compact
    python tools/query_rti_work.py test "Filesystem state image preserves mixed delivered negotiated confirmations in a fresh registry" --compact
    python tools/query_rti_work.py lane public-process-restart-mixed-confirmation-delivered --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves mixed delivered negotiated confirmations and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
    python tools/query_rti_work.py lane process-restart-asymmetric-mixed-confirmation --compact
    python tools/query_rti_work.py test "Filesystem state image restores an asymmetric mixed negotiated confirmation in a fresh registry" --compact
    python tools/query_rti_work.py lane public-process-restart-asymmetric-mixed-confirmation --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves asymmetric mixed negotiated confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-asymmetric-mixed-confirmation-reverse --compact
     python tools/query_rti_work.py test "Filesystem state image restores the reverse asymmetric mixed negotiated confirmation in a fresh registry" --compact
     python tools/query_rti_work.py lane public-process-restart-asymmetric-mixed-confirmation-reverse --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves reverse asymmetric mixed negotiated confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-malformed-mixed-confirmation --compact
     python tools/query_rti_work.py test "Federation restore rejects malformed mixed negotiated confirmation images" --compact
     python tools/query_rti_work.py lane process-restart-attribute-transportation-type-change --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending attribute transportation-type change in a fresh registry" --compact
     python tools/query_rti_work.py lane public-process-restart-attribute-transportation-type-change --compact
      python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending attribute transportation-type change and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-interaction-transportation-type-change --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending interaction transportation-type change in a fresh registry" --compact
     python tools/query_rti_work.py lane public-process-restart-interaction-transportation-type-change --compact
      python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending interaction transportation-type change and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-interaction-declaration --compact
     python tools/query_rti_work.py test "Filesystem state image restores a published interaction declaration in a fresh registry" --compact
     python tools/query_rti_work.py lane process-restart-interaction-transportation-type-override --compact
     python tools/query_rti_work.py test "Filesystem state image restores a committed interaction transportation-type override in a fresh registry" --compact
     python tools/query_rti_work.py lane public-process-restart-interaction-transportation-type-override --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves committed interaction transportation-type override and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-interaction-mixed-override --compact
     python tools/query_rti_work.py test "Filesystem state image restores a mixed interaction override with a subscription for multiple federates" --compact
     python tools/query_rti_work.py lane public-process-restart-interaction-mixed-override --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore preserves mixed interaction override and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-declaration --compact
     python tools/query_rti_work.py test "Filesystem state image restores a directed interaction publication and subscription for multiple federates" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-declaration --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rehydrates directed interaction declaration and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-routing --compact
     python tools/query_rti_work.py test "Filesystem state image restores a directed interaction target and receive-order route" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-routing --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds directed target routing and receive-order delivery under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-ownership-handoff --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore follows directed by-ownership target handoff and receive-order delivery under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-tso-ownership-callback --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-callback --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses stale directed TSO callback after by-ownership handoff under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore delivers eligible directed TSO and issues Request Retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-directed-interaction-tso-fanout-positive-negative --compact
     python tools/query_rti_work.py test "Filesystem fresh-registry directed TSO fan-out preserves an eligible recipient and suppresses an unsubscribed recipient" --compact
     python tools/query_rti_work.py lane tso-queue-state --compact
    python tools/query_rti_work.py lane ownership-ledger-state --compact
    python tools/query_rti_work.py lane process-restart-ownership-assumption-search --compact
    python tools/query_rti_work.py test "Filesystem state image restores ownership assumption search state and continues with a newly eligible federate" --compact
    python tools/query_rti_work.py lane public-process-restart-ownership-assumption-search --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending Request Attribute Ownership Assumption through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-ownership-acquisition-cancellation --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending ownership-acquisition cancellation in a fresh registry" --compact
     python tools/query_rti_work.py lane process-restart-divestiture-if-wanted --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending Divestiture If Wanted notification in a fresh registry" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending Divestiture If Wanted notification through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture --compact
     python tools/query_rti_work.py test "Filesystem state image restores a pending Confirm Divestiture notification in a fresh registry" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds pending Confirm Divestiture notification through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane confirm-divestiture --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture-fanout --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore fans out pending Confirm Divestiture notifications to three recipients through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture-mixed-fanout --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore fans out mixed regular and If Available Confirm Divestiture notifications through HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture-resignation --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses a resigned Confirm Divestiture candidate and preserves the surviving recipient" --compact
     python tools/query_rti_work.py lane process-restart-confirm-divestiture-post-confirmation-resignation --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses a post-confirmation resigned Confirm Divestiture recipient while preserving the surviving recipient" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore suppresses one post-confirmation resigned Confirm Divestiture recipient while preserving two surviving recipients" --compact
     python tools/query_rti_work.py test "Embedded public fresh-registry restore rebinds an eligible ownership-assumption recipient beside two Confirm Divestiture notifications" --compact
     python tools/query_rti_work.py lane public-process-restart-directed-interaction-tso-ownership-delivery-retraction --compact
    python tools/query_rti_work.py test "Embedded public fresh-registry restore delivers eligible directed TSO and issues Request Retraction under HLA_EVOKED and HLA_IMMEDIATE" --compact
     python tools/query_rti_work.py unlocated --summary --limit 20
    python tools/query_rti_work.py lane object-visibility-state --compact
    python tools/query_rti_work.py lane object-lifecycle-state --compact
    python tools/query_rti_work.py lane membership-update-state --compact
    python tools/query_rti_work.py lane membership-reflection-state --compact
    python tools/query_rti_work.py lane membership-lifecycle-state --compact
    python tools/query_rti_work.py lane membership-interaction-send-state --compact
    python tools/query_rti_work.py lane membership-interaction-receive-state --compact
    python tools/query_rti_work.py lane object-name-reservation-state --compact
    python tools/query_rti_work.py lane object-class-declaration-state --compact
    python tools/query_rti_work.py lane synchronization-state --compact
    python tools/query_rti_work.py lane region-state --compact
    python tools/query_rti_work.py lane application-value-state --compact
    python tools/query_rti_work.py lane pending-application-request-state --compact
    python tools/query_rti_work.py lanes --summary --limit 40
    python tools/query_rti_work.py search "regional timestamped" --summary
    python tools/query_rti_work.py unmapped --summary --limit 20
    python tools/query_rti_work.py test "federation save"
    python tools/query_rti_work.py requirement clause-6.12.4
    python tools/query_rti_work.py requirement hla-1516.1-2025:clause-8.22.3 --compact
    python tools/query_rti_work.py check

Append `--json` to any command for scripting (before or after the subcommand);
`next` reports the highest-priority open indexed slice, while `item` joins one
roadmap item to its tagged Catch2 cases and their standard mappings. `search`
is the forgiving one-query path across test IDs, names, semantic tags, API
surface IDs, Lab requirement IDs, and canonical 2025 clause/subsection keys.
For an installable profile, `work --summary --compact` also prints the exact
package target, CTest label, and installed profile-manifest path; this keeps
the package gate queryable beside the Catch2 lane without treating it as a
synthetic Catch2 requirement row.
It also reports the bounded JUnit target and artifact path for the active
process lane.
`section` is the exact standard-mapping path: it accepts a canonical
`document-id:clause-id`, the renderer's space-separated form, or a clause-only
key such as `clause-9.13.1`, and returns only tests mapped to that section.
Use `lanes --summary --limit 40` to discover the most-used exact tags without
dumping the full inventory (use `--limit 0` when a complete list is intended),
and use `unmapped` to make the remaining
requirement-less planning cases explicit.
Test, lane, requirement, and item results include a derived C++ source
location (`cpp/tests/...cpp:line`). The location index is rebuilt from
`TEST_CASE` declarations for each query, so it cannot drift into a second
source of truth; `check` reports any plan entries that are not locatable.
lane/test/requirement/section/item/search/unmapped output is capped at 20 cases by default, with
`--limit 0` available when a complete export is needed. Add `--compact` to
retain each test's Lab requirement IDs and 2025 clause/subsection mappings
while omitting repeated contract-symbol provenance; this is the preferred
bounded traceability view. For roadmap selection, `next --compact` also keeps
the output bounded by reporting only actionable lane/test/ctest handles plus
standard-section and plan-id counts; use `next --json` or the noncompact view
when the complete arrays are needed. Add `--summary` when selecting work to
keep one short record per case (status, mapping id, counts, and focus tags)
without printing every requirement row. The canonical
bounded command set is duplicated in
`docs/planning/ROADMAP-INDEX.json` under `bounded_query_commands`.

The `application-value-state` lane is the focused object-value slice: it
selects the codec and state-image cases without pulling the entire
`object-ddm-ownership` item. Use
`python tools/query_rti_work.py lane application-value-state --compact` before
opening source; its output includes the exact mapped Lab IDs and standard
clause/subsection metadata. When a plan entry selects official C++ API
surfaces, `test` and `lane` also print those exact surface IDs beside the
requirements and clauses.

The `pending-application-request-state` lane is the focused temporal request
slice. It selects the same codec case with explicit pending time-advance,
time-role-enable, and deferred-lookahead request metadata, so the exact Lab
IDs and 2025 clause mapping can be inspected without opening the entire
save/restore item. The fresh-registry TAR/deferred-lookahead case is the
current private process-restart baseline; run it with
`python tools/query_rti_work.py test "Filesystem state image restores pending
time advance and deferred lookahead in a fresh registry" --compact`. Its
filesystem fixture names are process-unique, so the focused application and
pending lanes may be run in parallel without colliding.

The small roadmap index is [ROADMAP-INDEX.json](../docs/planning/ROADMAP-INDEX.json).
Keep roadmap anchors and Catch2 tags stable so the check command remains a
fast drift detector. The active item’s `next_task` plus optional
`next_work_id`, `next_work_status`, `next_work_query`, `next_lane`,
`next_test_role`, `next_test_query`, `next_ctest_filter`, `next_standard_sections`, and
`next_plan_ids` fields are the authoritative work handles; update those
alongside each bounded implementation slice. A planned architectural work
handle may intentionally have null lane/test/CTest fields until its first
focused C++ case is added.
`next_ctest_filter` is stored in Catch2 tag form (for example,
`[public-process-restart-regional-pending-attribute-value-update]`); pass that
form to the Catch2 executable or use the tag text as a CTest label with `-L`.

The JSON result for each test includes `lab_requirement_ids`,
`standard_sections` (canonical `document_id:clause_id` keys),
`selected_cpp_api_surface_ids`, `source_locations`, and
`requirements_lab_mapping_id`. These are
derived from the checked-in plan and pinned 2025 bundle, so scripts can join a
test to its C++ declaration, a requirement, or subsection without reparsing
the human roadmap.

For the official 2010 API artifacts and portable 1516e TCK catalog, run:

    python tools/verify_1516e_artifacts.py --help
    python tools/verify_1516e_tck_catalog.py
    python tools/verify_1516e_type_roundtrip.py
    python tools/verify_1516e_tck_parity.py
    python tools/verify_1516e_tck_results.py out/java-tck-2010/script-results.json
    python tools/java_tck_2010.py out/java-tck-2010/script-results.json \
      --provider "Umbra mock Java 1516e"
    python tools/verify_1516e_tck_catalog.py \
      --catalog compliance/catalogs/python-2010-tck-scenario-catalog.json
    python tools/verify_1516e_python_tck_results.py \
      out/java-tck-2010/python-jpype-smoke.json
    python tools/python_tck_2010.py out/java-tck-2010/python-jpype-smoke.json \
      --provider "Umbra mock Java 1516e"

For a real vendor Java provider, the checkout runner accepts the API JAR,
provider JAR, optional dependency JARs, and JVM options, then verifies and
exports the Python result:

    packages/umbra-rti-java-tck-2010/run-python.ps1 -ApiJar C:\\path\\to\\api.jar \
      -ProviderJar C:\\path\\to\\vendor.jar \
      -FomPath C:\\path\\to\\RestaurantFOMmodule.xml

The same scenario runner can use the direct C++/pybind provider after it has
been staged in wheel layout:

    packages/umbra-rti-native/run-2010-tck.ps1 \\
      -NativePackageDirectory out\\native-2010-install \\
      -CapabilityProfile packages\\umbra-rti-java-tck-2010\\profiles\\native-2010-provider.properties

The 2010 contract generators consume a locally staged official Java API source
tree and write only the checked-in contract destinations when explicitly
invoked:

    python tools/generate_1516e_python_contract.py --help
    python tools/generate_1516e_exception_contract.py --help
    python tools/verify_1516e_encoding_contract.py --help
    python tools/verify_1516e_python_surface.py --check-jpype
    python tools/verify_1516e_python_types.py --help
    python tools/verify_1516e_provider_boundaries.py
    python tools/generate_1516e_cpp_shell.py --check
    python tools/verify_1516e_provider_jar.py \
      --provider-jar C:\\path\\to\\vendor-rti-2010.jar \
      --api-jar C:\\path\\to\\ieee-1516.1-2010-java-api.jar
    python tools/generate_1516e_surface_inventory.py \
      --cpp-root C:\\path\\to\\cpp\\src \
      --java-root C:\\path\\to\\java\\src \
      --output out\\java-tck-2010\\1516e-surface-inventory.json

### Validate a Java TCK result

The Java runner produces provider output; this tool joins that output to the
portable catalog and Requirements Lab contracts:

    python tools/java_tck.py validate
    python tools/java_tck.py export --help

See the [Java TCK guide](../docs/testing/JAVA-RTI-CONFORMANCE-TCK.md) for
compile, run, matrix, and JPype-handoff commands.

## Generators and inventories

| Tool | Purpose |
| --- | --- |
| generate_binding_inventory.py | Builds the reviewable abstract-member inventory from the pinned C++ headers. |
| generate_cpp_conformance_worklist.py | Produces a non-evidentiary C++ worklist from a Requirements Lab bundle. |
| generate_rti_ambassador_shell.py | Regenerates the checked-in private fallback ambassador shell. |
| generate_1516e_cpp_shell.py | Regenerates the bounded IEEE 1516e-2010 C++ null ambassador shell from the official header. |
| ieee_1516_2_resources.py | Imports or verifies the pinned IEEE 1516.2 resource set and digests. |

## Compliance and provider evidence

| Tool | Purpose |
| --- | --- |
| requirements_lab.py | Exports and validates Umbra-owned baselines, implementation contracts, and the native Catch2 plan against the adjacent Requirements Lab. |
| requirements_lab_sidecar.py | Prepares, records, and verifies sidecar evidence without importing Lab code at runtime. |
| java_tck.py | Validates and exports portable Java TCK traceability and provider evidence. |
| java_tck_2010.py | Validates and exports 2010 Java TCK results against the exact 1516e scenario/Requirements Lab catalog. |
| python_tck_2010.py | Validates and exports 2010 provider-neutral Python smoke results (JPype or native) with the same Requirements Lab linkage. |
| verify_external_2025_fom_corpus.py | Verifies a configured external 2025-native FOM snapshot against its manifest. |
| verify_external_siso_fom_corpus.py | Verifies a configured external SISO FOM snapshot against its manifest. |
| verify_external_2010_fom_resources.py | Verifies the reviewed external IEEE 1516.1/1516.2-2010 schemas and MIM against their manifest. |
| verify_1516e_artifacts.py | Verifies official IEEE 1516.1/1516.2-2010 archive digests, namespaces, and API counts. |
| verify_1516e_tck_catalog.py | Verifies every 2010 TCK requirement, mapping, transition, and provider-neutral API reference against the pinned Lab export and checked-in Python contract. |
| verify_1516e_type_roundtrip.py | Verifies the focused 2010 JNI carrier matrix covers raw JVM values, all standard data elements, handles, collections, logical times, and Java-only enum/record/callback/exception carriers. |
| verify_1516e_tck_parity.py | Verifies every Java 2010 provider scenario has a Python transplant and that Python retains its normative links. |
| verify_1516e_tck_results.py | Verifies a 2010 Java TCK result artifact has the catalog's exact scenarios, standard, and status vocabulary. |
| verify_1516e_python_tck_results.py | Verifies a 2010 Python result artifact (JPype or native) has the Python catalog's exact scenarios and explicit status counts. |
| verify_1516e_encoding_contract.py | Compares the official 2010 Java encoding interface method names with the Python contract namespace. |
| verify_1516e_python_surface.py | Audits the checked-in 2010 namespaces, contract methods, nested callback records, factories, encoders, and optional JPype adapter. |
| verify_1516e_python_types.py | Compares official Java type filenames with Python module attributes/exports and nested callback aliases. |
| generate_1516e_surface_inventory.py | Generates the C++/Java/Python method-name inventory and makes intentional C++/Java surface differences explicit. |
| verify_1516e_provider_boundaries.py | Proves that the 2010 JPype/native/JNI boundary is isolated from the 2025 provider and pins the bounded native target plus JNI null bridge. |
| verify_1516e_provider_jar.py | Performs a metadata-only 2010 `ServiceLoader`/provider-class/namespace preflight for provider and API JARs; it does not claim conformance. |
| verify_1516e_capability_profile.py | Validates shared 2010 Java/Python capability-profile keys against the transplantable scenario catalogs. |

## Native test checks

| Tool | Purpose |
| --- | --- |
| verify_catch2_test_tags.py | Ensures native Catch2 tests belong to focused development lanes. |
| verify_ctest_service_lanes.py | Checks that configured CTest service lanes remain complete. |
| verify_ieee_exception_binding.py | Confirms every official exception has a binding definition. |
| verify_ieee_headers.py | Confirms the vendored IEEE 1516.1 header file set and hashes. |
| verify_hla_symbolic_names.py | Rejects raw string literals at dynamic HLA handle-lookup boundaries. |

## Reporting

plot_runtime_instrumentation.py turns a runtime instrumentation CSV into a
local image. Send its output to out/ rather than a source directory.

## Write boundaries

Most verifiers read inputs and report success or failure. The generator and
Requirements Lab tools can write outputs. Generated source remains under
cpp/generated/, ordinary temporary output belongs under out/, and local
Requirements Lab requests or raw evidence belong under .compliance/. Review a
generator diff before committing it.
