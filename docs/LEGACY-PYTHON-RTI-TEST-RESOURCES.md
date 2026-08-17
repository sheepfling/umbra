# Legacy Python RTI test-resource register

This register records high-value test-design resources in the adjacent legacy
Python RTI project. It complements the actionable
[legacy scenario backlog](LEGACY-PYTHON-RTI-TEST-BACKLOG.md): this file says
what is worth revisiting, while the backlog says what Umbra should implement
next.

## Snapshot and use boundary

- Inspected source revision: `7d92a5e24563ad996d2ec7ea5c1840c7c4a9c9bc`
  on 2026-08-16.
- The sibling worktree was not clean at inspection time.  Re-check its revision
  and status before using a later reading or importing any third-party asset.
- These are scenario designs, fixtures, and test-oracle ideas.  They are not
  imported Python code, Umbra API design, Requirements-Lab mappings, test
  results, or portable conformance evidence.
- The sibling's direct Python and hosted-transport bounded proofs remain claims
  for its own runtime lanes.  Umbra must create independent Catch2 evidence.

For every selected item, first identify the pertinent IEEE 1516.1/1516.2-2025
source in the Requirements Lab, state the Umbra runtime boundary, and write a
fresh C++ expectation.  Do not inherit a pytest marker, helper algorithm, or
vendor-specific transport outcome as a standard rule.

## Harvested resource register

| ID | Resource and value | Sibling anchors | Umbra intake gate |
| --- | --- | --- | --- |
| `SBR-001` | Callback-model and callback-lifetime stress: evoked versus immediate delivery, exception paths, stale-backlog cleanup, and deterministic disabled/enable/evoke/drain sequences. | `docs/callback_model_guide.md`; `tests/backends/test_python_backend_federation_extended.py`; `tests/backends/test_python_backend_time_ddm_extended.py`; `tests/scenarios/test_support_services_backend_matrix.py`; `docs/requirements/ieee-1516-2025/callback_bounded_proof.md` | The private dispatcher and `CallbackSession` already give Umbra a foundation.  Reuse scenario shape only after each affected service exists; do not assume Python's inline-immediate behavior is the complete C++ ordering rule.  Treat queued-while-disabled, first-release, ordered batch-drain, and empty-after-drain as separate C++ assertions. |
| `SBR-002` | Federation lifecycle, listing, FOM visibility, multi-participation, atomic preflight failures, and negative lifecycle paths. | `packages/hla-verification/src/hla/verification/scenario_federation_lifecycle.py`; `scenario_join.py`; `scenario_resign.py`; `scenario_connection_lost.py`; `tests/scenarios/test_federation_lifecycle_backend_matrix.py` | Current embedded Create/Join/Resign/Listing coverage can grow from these.  Keep invalid/missing/inconsistent FOM preparation, duplicate creation, destroy-with-members, disconnect-while-joined, and multi-federation participation as independently asserted state transitions.  Connection-lost cases wait for a real transport or federation event rather than a synthetic success path. |
| `SBR-003` | FOM parsing/composition negative corpus: duplicate definitions, unresolved type references, table consistency, ordered module sets, MIM merge behavior, and schema-invalid documents. | `tests/factories/test_fom_omt_parsing.py`; `test_fom_validate.py`; `test_fom_schema_baseline.py`; `test_public_fom_baseline.py` | Use as a design checklist for the private libxml2/FDD preflight.  Recreate minimal Umbra-owned fixtures first; import upstream XML only after source, license, digest, schema edition, and module order are recorded.  The selected richer-family phases are tracked in the [FOM stress-corpus backlog](FOM-STRESS-CORPUS-BACKLOG.md). |
| `SBR-004` | Small, readable scenario FOMs and test plans for message, time, and space behavior. | `docs/fom-examples/proto2025-v0.1/test_plans/Proto2025_MessageTest_TestPlan.md`; `Proto2025_TimeMgmtTest_TestPlan.md`; `Proto2025_SpaceLite_TestPlan.md`; `tests/factories/test_proto2025_fom_resources.py` | The plans are useful acceptance-test templates. A reviewed four-module 2025 XML snapshot now exercises only positive DIF/MIM-first preflight and embedded Create/Join through the optional external-corpus lane; the XML remains outside Umbra and no FOM is a default federation root. The [FOM stress-corpus backlog](FOM-STRESS-CORPUS-BACKLOG.md) records the remaining staged runtime work. |
| `SBR-005` | Compact Section 8 time-management oracle suite: state services, early timestamp rejection, GALT/LITS, ordering, availability, retraction, flush, order override, and strict TAR boundary. | `packages/hla-verification/src/hla/verification/section8_matrix.py`; `tests/time/`; `tests/scenarios/test_time_management_federation.py` | Governed by `SB-TIME-*` in the scenario backlog. The private queue/coordinator slice now covers ordering, LITS, queued/in-transit/completed state, and retraction boundaries; public TSO delivery, alternate advance modes, and flush remain deferred. |
| `SBR-006` | Object/interaction/declaration/discovery scenarios with two-federate callback oracles, plus inherited attributes, empty publication, privilege attributes, relevance/scope, update requests, orphan objects, and local/timed deletion. | `scenario_exchange.py`; `scenario_declaration.py`; `scenario_discovery_metadata.py`; `scenario_discovery_class.py`; `scenario_object_scope.py`; `tests/scenarios/test_object_management_backend_matrix.py` | Start after a real object/interaction vertical slice exists: private handles and FOM lookup, publication/subscription, discovery, reflection/receive, then callback ordering.  Promote the edge cases only once their state variables exist; do not introduce object APIs merely to mimic the Python scenario layout. |
| `SBR-007` | Data-distribution pressure: passive regions, object-region lifecycle, and declaration gating. | `scenario_ddm_passive_regions.py`; `scenario_ddm_object_regions.py`; `tests/scenarios/test_ddm_backend_matrix.py` | Defer until the object/declaration layer and region ownership model are stable.  Use its isolation assertions to shape private routing tests before public DDM services. |
| `SBR-008` | Ownership transfer and error-path oracles: acquisition, unavailability, query callback isolation, negotiated divestiture, release requests, and non-owner update rejection. | `scenario_ownership.py`; `tests/scenarios/test_ownership_management_backend_matrix.py` | Defer until object identity, attribute ownership state, and callback routing exist.  Retain ownership-related resign scenarios as a separate slice from the present membership-only resign implementation. |
| `SBR-009` | Synchronization and name-reservation flows, including failed/late join participation. | `scenario_sync.py`; `scenario_name_reservation.py` | Use after federation event routing and object-name state become real.  These are good callback-ordering and failure-cleanup fixtures, not near-term federation-management tests. |
| `SBR-010` | Save/restore gauntlets: lifecycle errors, participant state, queued callbacks, object/ownership rollback, stale directed-TSO cleanup, and time-window rollback. | `scenario_save_restore.py`; `scenario_target_radar_time.py`; `docs/requirements/ieee-1516-2025/save_restore_bounded_proof.md` | Preserve this as a late capability-family oracle.  It becomes actionable only after Umbra owns object, ownership, TSO, callback, and persistence state; do not reduce it to a superficial save-complete callback test. |
| `SBR-011` | Reusable two-federate test topology and artifact vocabulary: setup/cleanup, timelines, summaries, and scenario pair composition. | `two_federate_suite_configs.py`; `two_federate_suite_pairs.py`; `two_federate_suite_scenarios.py`; `two_federate_suite_timeline.py`; `two_federate_suite_summary.py` | Borrow the topology and evidence vocabulary when Umbra introduces broader integration suites.  Write a C++-native harness; do not bind Umbra tests to the Python runner or artifact format. |
| `SBR-012` | Remote/transport stress and cross-process seam tests, including callback isolation, cleanup, lost-federate behavior, and repeated vendor-probe stability. | `tests/transport/test_grpc_transport_2025.py`; `tests/transport/test_rest_transport.py`; `tests/scenarios/test_lost_federate_external_scenario.py`; `tests/scenarios/test_vendor_probe_stability.py`; `docs/transport_extension_playbook.md` | Reference only after the embedded runtime is stable and a transport protocol exists.  These tests encode Python/FedPro operational details and must never define Umbra's core RTI semantics. |
| `SBR-013` | Encoding, buffer, handle, and value-semantic test design: primitive and string encodings, opaque data, arrays/records/variants, offsets/slices/mutability, logical-time wrappers, and handle identity. | `tests/test_rti1516_2025_encoding_factory.py`; `tests/test_rti1516_2025_encoding_auth_contexts.py`; `tests/compat/test_upstream_datatypes_handles.py`; `tests/compat/test_upstream_enums_exceptions.py` | The first Umbra slice is implemented: an installed-SDK consumer and Catch2 test exercise official `VariableLengthData` caller-copy, borrowed-copy, and adopted-storage lifetimes, plus the existing handle boundary.  Derive later data-element expectations and golden vectors from the official binding and IEEE source; do not import Python shim snapshots, custom authentication behavior, or its allowed-difference policy. |
| `SBR-014` | FOM corpus reporting and semantic round-trip design: classify each model by edition, stress lane, intended role, expected result, and module identity; check deterministic parse/materialize/revalidate behavior. | `tests/verification/test_fom_stress.py`; `tests/factories/test_fom_roundtrip.py`; `tools/fom-roundtrip` | The first Umbra slice is implemented: both the fixed official MIM-plus-Restaurant set and an optional digest-pinned 2025 external set are composed twice and compared as immutable private FDD/catalog results. Retain the classification and reproducibility ideas, not its JSON/protobuf/URL/compression transport format. Umbra's equivalent must use an approved input, canonical private catalog/FDD form, and a fresh schema validation; byte-for-byte XML reproduction is not assumed. |
| `SBR-015` | Management Object Model (MOM) catalog and negative-traffic design: MIM-derived names/parameters, required-parameter checks, payload validation, RTI-owned state, no hidden subscriptions/time participation, and region rejection. | `tests/mom/test_mom_catalog_validation_v012.py`; `tests/verification/test_mom_observer_slice_v013.py`; `tests/verification/test_mom_negative_matrix_v013.py` | Defer until Umbra owns a complete MIM catalog, declarations, object/interaction routing, and a management-service design.  The source's management behavior is a hypothesis; each public expectation needs an independent Requirements-Lab record. |

## Extraction order

The source tree is deliberately broader than Umbra's present runtime.  To keep
the harvest useful rather than sprawling, apply the resources in this order:

1. Finish the current no-TSO time matrix (`SB-TIME-003`) and private FOM
   preflight negatives from `SBR-003`.
2. Add a C++-native support/encoding target informed by `SBR-013`, but derive
   every public expectation from the official headers and IEEE source rather
   than a Python compatibility snapshot.
3. Build object/interaction and declaration slices, then take the smallest
   `SBR-006` discovery/reflection scenarios and use `SBR-014` to make the
   private FOM catalog/FDD result deterministic and inspectable.
4. Promote the relevant `SBR-005` cases one at a time from the completed
   private coordinator: ordering, LITS, and retraction first; alternate
   advance modes follow only after their public service semantics exist.
5. Add DDM and ownership only after their supporting object state exists.
6. Use save/restore, MOM, and transport resources as cross-family regression suites,
   not early implementation targets.

## Umbra-specific test work not supplied by the source corpus

The adjacent corpus is valuable, but it cannot supply the tests that protect
Umbra's core C++ contract.  Add these as Umbra-native work rather than trying
to translate Python tests:

1. Compile/install tests that build small federate clients against the exact
   vendored official headers and the installed SDK, including exception and
   overload use.  The official headers, not a locally frozen API snapshot,
   are the oracle.
2. Property-based and fuzz testing for XML/FOM parsing, hostile entity/schema
   inputs, datatype graphs, and encoded buffers; record each minimized corpus
   input with its expected validator result.
3. Deterministic scheduler and fault-injection tests for lock ordering,
   callback re-entrancy, cancellation, teardown, and all state transitions;
   run suitable targets under platform sanitizers as the C++ runtime grows.
4. Black-box cross-process and external-RTI interoperability tests, with
   separately pinned versions, topology, timeouts, and artifacts.  A local
   embedded pass must never stand in for this evidence.

## Evidence discipline

When a resource becomes an Umbra test, record all of the following next to the
test or contract:

- sibling source revision and path that inspired the scenario;
- the independently reviewed Requirements-Lab record(s);
- the declared Umbra scope and known exclusions;
- a deterministic C++ oracle and cleanup boundary; and
- actual Catch2 result provenance, without promoting it to a catalog or
  conformance claim before the established review workflow allows it.

This preserves the sibling project as a valuable adversarial corpus while
keeping Umbra standards-first and independently testable.
