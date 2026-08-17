# Legacy Python RTI scenario backlog

This is Umbra's intake backlog for useful scenarios found in the adjacent
legacy Python RTI project. It is a source of test ideas and adversarial inputs,
not a source of public API, runtime semantics, or conformance evidence.

The companion [test-resource register](LEGACY-PYTHON-RTI-TEST-RESOURCES.md)
preserves the wider harvested scenario, fixture, and oracle inventory.  The
[FOM stress-corpus backlog](FOM-STRESS-CORPUS-BACKLOG.md) separately records
richer model families, provenance gates, and their intended capability phase.
This backlog contains only the items chosen for Umbra implementation planning.

## Provenance and boundary

- Inspected source revision: `7d92a5e24563ad996d2ec7ea5c1840c7c4a9c9bc`
  on 2026-08-16.
- The sibling worktree had unrelated local changes at inspection time.  Refresh
  both the revision and worktree state before taking a later reading from it.
- No Python implementation, pytest requirement marker, FOM XML, or test result
  has been copied into Umbra or executed as an Umbra test.
- Its own bounded time-management proof is useful context, but it is evidence
  for that Python runtime only.  It is not portable evidence for the official
  C++ binding or an IEEE 1516.1-2025 conformance claim.

The immutable IEEE 1516.1/1516.2 assets and the HLA Requirements Lab remain
Umbra's normative inputs.  A selected scenario becomes an Umbra test only
after its expected outcome has been checked against those inputs, expressed
through the official C++ API where applicable, and given its own Catch2
evidence and Requirements-Lab traceability.

## Intake rules

1. Treat a sibling assertion as a hypothesis.  Locate the pertinent 2025
   source-derived requirement in the Requirements Lab before fixing an Umbra
   expectation.
2. State the intended runtime boundary explicitly: private kernel, embedded
   no-TSO profile, TSO profile, or eventual transport/interoperability suite.
3. Start with deterministic `HLAinteger64Time` cases.  Add floating-time and
   cross-process variants only after the integer kernel behavior is stable.
4. Keep an unsupported or unimplemented scenario in this backlog (or a named
   exploratory target); do not add expected failures to the normal green
   Catch2 target.
5. Before importing a FOM, record its upstream source, license/attribution,
   exact digest, edition/schema compatibility, module order, and the intended
   verdict.  A parser-stress corpus is not automatically a federation root or
   a 2025-compatible runtime scenario.

## Current overlap

Umbra already has private Catch2 coverage for the no-TSO GALT/LITS calculation,
strict TAR eligibility, the Non-Regulated-Grant (NRG) switch, zero-lookahead
TAR behavior, scheduler re-evaluation after key role, membership, and
additional-FOM definition changes, and the queue/coordinator's queued,
in-transit, delivered, and retraction state. The public profile now adds three
bounded non-regional timestamped producers—interaction, attribute update, and
object deletion/removal—with retraction; the remaining timestamped families, alternate advance modes,
save/restore, and transport are still outside the enabled scope.

The source locations below are therefore a staged backlog rather than a list
of tests ready to copy verbatim.

## Time-management scenarios

| ID | Priority and phase | Sibling source | Umbra intake outcome |
| --- | --- | --- | --- |
| `SB-TIME-001` | **Adjudicated**, no-TSO/TSO boundary | `tests/time/test_galt.py`; `packages/hla-backend-common/src/hla/backends/common/time_management.py` | Do not port the sibling's `valid_tso_lower_bound` assertion.  Its `min(current, pending) + lookahead` behavior differs from its own GALT helper.  The Requirements Lab's 2025 source resolves the outgoing-TSO case: the normal current-time rule is overridden while a federate is Time Advancing, when requested logical time plus lookahead applies (`requirement-candidate-content-clauses-08-time-management-page-184-l91-26`, with the base rule at `...page-182-l25-7`).  Umbra's existing no-TSO pending-time candidate is aligned; a direct Catch2 regression covers it.  This does not make the no-TSO calculator a complete GALT implementation because queued and in-transit TSO messages are still absent. |
| `SB-TIME-002` | **Implemented**, embedded no-TSO | `tests/time/test_galt.py`; `tests/scenarios/test_time_management_federation.py` | Umbra now has an official-C++-API three-federate Catch2 scenario for the minimum across active other regulators, a pending/granted regulator advance, and regulator resignation.  It confirms membership removal changes the no-TSO GALT/LITS view; it does not exercise TSO traffic or full time coordination. |
| `SB-TIME-003` | **Implemented**, embedded no-TSO | `tests/time/test_grant_decision.py`; `tests/time/test_time_management_algorithms.py` | Umbra's focused NRG/role-transition matrix now covers disabled/default NRG, enabled NRG, regulator enable/disable/resignation, constrained-mode disable, and a successful MIM-first additional-FOM replacement that enables NRG and wakes an existing TAR. The strict GALT boundary remains a limited no-TSO policy traced to the Requirements Lab; none of this exercises a timestamped queue. |
| `SB-TIME-004` | **Implemented**, private coordinator | `tests/time/test_lits.py`; `tests/time/test_time_management_algorithms.py` | Umbra's coordinator snapshot now feeds delivered-since-last-advance and queued/in-transit TSO timestamps into GALT/LITS. Catch2 covers the minimum incoming timestamp, recipient isolation through the coordinator, and LITS with an undefined GALT. |
| `SB-TIME-005` | **Implemented**, private TSO foundation | `tests/time/test_tso_queue.py` | Umbra's TsoMessageQueue and FederationTimeCoordinator Catch2 slices cover stable `(timestamp, sequence)` ordering, recipient isolation, retraction before delivery, in-transit/completed callback state, exclusive boundaries, and same-timestamp groups. They use official C++ LogicalTime values but do not expose a public timestamped service. |
| `SB-TIME-006` | TSO coordinator | `tests/time/test_grant_decision.py`; `tests/time/test_time_management_algorithms.py` | Add the TAR/TARA/NMR/NMRA/FQR grant matrix, including strict versus inclusive GALT boundaries and next-message / flush boundary selection.  Do not expose these services merely to satisfy the test shape. |
| `SB-TIME-007` | TSO integration | `tests/scenarios/test_time_management_federation.py`; `packages/hla-verification/src/hla/verification/section8_matrix.py` | Use the two-federate simultaneous-timestamp and FQR flows as eventual end-to-end fixtures.  Their scenario structure is useful; ordering/tie-break expectations must be specified independently for Umbra. |
| `SB-TIME-008` | Save/restore phase | `tests/test_rti1516_2025_python1516_2025_runtime.py`; sibling time-window proof families | Add time-window, future-exclusion, and restore-rollback stress cases only after Umbra implements the underlying save/restore and timestamped-delivery state. |

`SB-TIME-001` was intentionally first.  It prevented a superficially helpful
Python helper test from silently choosing Umbra's time semantics; the official
2025 source now supplies the recorded decision.

## FOM and stress-corpus candidates

The detailed [FOM stress-corpus backlog](FOM-STRESS-CORPUS-BACKLOG.md) is the
source of record for the broader model families.  The rows here retain only
the scenario-level handoff points.

| ID | Candidate | Sibling source | Intended first use |
| --- | --- | --- | --- |
| `SB-FOM-001` | Small Proto2025 time-management scenario | `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_TimeMgmtTest.xml`; `docs/fom-examples/proto2025-v0.1/test_plans/Proto2025_TimeMgmtTest_TestPlan.md` | Borrow the scenario design first: 10,000 RO events, same-timestamp batches, 100 TAR ticks, positive/zero lookahead, replay, and late join.  Do not import the XML until its ownership, module dependency, digest, and 2025 schema compatibility are recorded. |
| `SB-FOM-002` | Ordered SISO RPR 2.0/RPR 3.0 and Space families | `third_party/fom_baseline/README.md`; `docs/fom_siso_quirks.md`; `docs/fom_siso_family_map.md` | Preserve as an out-of-scope research record while Umbra remains 2025-only. Any later parser/composition work would need source and license manifests and an explicit edition decision; the sibling distinguishes 2010-shaped stress packets from its 2025-named scenarios. The U-FOM mention remains unpromoted until a source asset and provenance package are located. |
| `SB-FOM-003` | Link 16 extension and integrated RPR 2.0 family | `docs/fom_siso_quirks.md`; `third_party/fom_baseline/siso/` | Use standalone Link 16 as a controlled negative/template fixture and the ordered RPR 2.0 family as the eventual integrated input.  Neither is a default Umbra federation root. |

## Explicit non-imports

- Do not adopt the sibling's Python public API, state model, or internal time
  algorithms as a C++ implementation blueprint.
- Do not reuse its pytest requirement IDs as Requirements-Lab mappings.
- Do not carry over its bounded-proof wording or service-completeness claims.
- Do not vendor SISO or other third-party FOM content without the provenance
  gate above.

## Next executable milestone

Keep the completed `SB-TIME-003` matrix in the embedded no-TSO profile. The
private queue and coordinator portions of `SB-TIME-004`/`SB-TIME-005` are now
complete, and three bounded public timestamped families have exact 2025
contracts. The next milestone is the next standards-backed public timestamped
family, beginning only after callback ordering, TSO eligibility, and
retraction-handle semantics have their own exact 2025 contract.
