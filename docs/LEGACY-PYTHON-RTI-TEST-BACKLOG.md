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
in-transit, delivered, and retraction state. The public profile now adds five
bounded timestamped producers—non-regional interaction, attribute update,
object deletion/removal, directed interaction, and region-context interaction—
with retraction, plus bounded TAR/TARA/NMR/NMRA/FQR dispatch. Remaining
timestamped families, future transport input, and full cross-federate time
coordination are still outside the enabled scope. The timestamped
`Request Federation Save(label, LogicalTime)` control slice is now covered by
an official-C++ Catch2 scenario; durable save/restore and the sibling's wider
rollback/time-window families remain future work.

The source locations below are therefore a staged backlog rather than a list
of tests ready to copy verbatim.

## Time-management scenarios

| ID | Priority and phase | Sibling source | Umbra intake outcome |
| --- | --- | --- | --- |
| `SB-TIME-001` | **Adjudicated**, no-TSO/TSO boundary | `tests/time/test_galt.py`; `packages/hla-backend-common/src/hla/backends/common/time_management.py` | Do not port the sibling's `valid_tso_lower_bound` assertion.  Its `min(current, pending) + lookahead` behavior differs from its own GALT helper.  The Requirements Lab's 2025 source resolves the outgoing-TSO case: the normal current-time rule is overridden while a federate is Time Advancing, when requested logical time plus lookahead applies (`requirement-candidate-content-clauses-08-time-management-page-184-l91-26`, with the base rule at `...page-182-l25-7`).  Umbra's existing no-TSO pending-time candidate is aligned; a direct Catch2 regression covers it.  This does not make the no-TSO calculator a complete GALT implementation because queued and in-transit TSO messages are still absent. |
| `SB-TIME-002` | **Implemented**, embedded no-TSO | `tests/time/test_galt.py`; `tests/scenarios/test_time_management_federation.py` | Umbra now has an official-C++-API three-federate Catch2 scenario for the minimum across active other regulators, a pending/granted regulator advance, and regulator resignation.  It confirms membership removal changes the no-TSO GALT/LITS view; it does not exercise TSO traffic or full time coordination. |
| `SB-TIME-003` | **Implemented**, embedded no-TSO | `tests/time/test_grant_decision.py`; `tests/time/test_time_management_algorithms.py` | Umbra's focused NRG/role-transition matrix now covers disabled/default NRG, enabled NRG, regulator enable/disable/resignation, constrained-mode disable, and static-NRG retention when an additional FOM joins. The strict GALT boundary remains a limited no-TSO policy traced to the Requirements Lab; none of this exercises a timestamped queue. |
| `SB-TIME-004` | **Implemented**, private coordinator | `tests/time/test_lits.py`; `tests/time/test_time_management_algorithms.py` | Umbra's coordinator snapshot now feeds delivered-since-last-advance and queued/in-transit TSO timestamps into GALT/LITS. Catch2 covers the minimum incoming timestamp, recipient isolation through the coordinator, and LITS with an undefined GALT. |
| `SB-TIME-005` | **Implemented**, private TSO foundation | `tests/time/test_tso_queue.py` | Umbra's TsoMessageQueue and FederationTimeCoordinator Catch2 slices cover stable `(timestamp, sequence)` ordering, recipient isolation, pending-fanout withdrawal after another recipient delivery, in-transit/completed callback state, exclusive boundaries, and same-timestamp groups. They use official C++ LogicalTime values; the queue itself does not define public RTI semantics. |
| `SB-TIME-006` | **Implemented**, bounded public TSO advances | `tests/time/test_grant_decision.py`; `tests/time/test_time_management_algorithms.py` | Umbra now has in-process TAR/TARA/NMR/NMRA/FQR paths. Catch2 covers strict versus inclusive defined-GALT decisions, selection of the next currently queued timestamp, and FQR delivery/optimistic-time boundaries. Future transport input, complete multi-federate coordination, and remaining timing races remain outside the slice. |
| `SB-TIME-007` | Partially implemented, TSO integration | `tests/scenarios/test_time_management_federation.py`; `packages/hla-verification/src/hla/verification/section8_matrix.py` | The two-federate next-message and FQR flow shapes informed the bounded C++ scenarios. Retain simultaneous-timestamp, transport-arrival, and cross-process variants as follow-on fixtures; their ordering/tie-break expectations must be specified independently for Umbra. |
| `SB-TIME-008` | Partially implemented, save/restore phase | `tests/test_rti1516_2025_python1516_2025_runtime.py`; sibling time-window proof families | The bounded timestamped save boundary, source-linked untimed and timestamped time-constrained save-admission paths, and process-local untimed restore now exist in the official C++ profile. A two-federate Catch2 case proves a TSO interaction at the exact TAR save timestamp is delivered before direct Initiate Federate Save and the matching grant; a three-member case proves two constrained members are admitted before a non-time-constrained member; TARA and NMRA cases prove equal versus strictly-later save boundaries; and an NMR case proves direct initiation at the matching timestamp. A three-member TARA/NMRA case proves both strict ordinary forms are ready before non-time-constrained notification. A dedicated FQR case proves an equal actual Flush Queue Grant leaves a timestamped save pending while a later FQG follows queued TSO and direct initiation; mixed FQR/TAR cases prove readiness whether FQR or TAR dispatches first. A six-member scenario now combines TAR, NMR, TARA, NMRA, and FQR before non-time-constrained notification. A cross-member TSO case proves a delayed constrained federate's queued interaction still precedes its own initiation after another constrained member begins saving. C++-native cases also prove a post-save retraction designator cannot alias fresh traffic, a live interaction/ledger returns after rollback, a terminal tombstone retains its classification, a saved logical-time/actual-lookahead window is restored after post-save mutation, and one deferred lookahead decrease target survives rollback. Add in-transit and multi-mode queued-TSO, role/resignation churn, future-exclusion, pending-advance restore, broader family, durable snapshot, and transport restore stress only after the corresponding C++ state boundaries are specified. |
| `SB-TIME-009` | **Implemented**, bounded Request Retraction | `tests/time/test_tso_queue.py`; `tests/scenarios/test_time_management_federation.py` | The sibling's fanout and flush scenario shapes informed Umbra Catch2 proofs: delivered nonconstrained normal, directed, and region-context timestamped interaction recipients receive Request Retraction while constrained recipients' queued copies are suppressed; normal and bounded regional timestamped attribute updates likewise notify their delivered immediate recipient. The normal update retains two passels under one retraction designator; the regional update preserves recipient-gated sent-region metadata. Dedicated one-federate normal-interaction and qualifying attribute-update regressions now prove a TSO invocation returns a valid designator even when no recipient is eligible; for attributes, Clause 6.10 requires at least one submitted attribute with TSO preferred order. Two-federate disjoint-region interaction and attribute-update regressions now prove the same returned-designator behavior when a regional subscription exists but does not overlap. The lightweight ledger permits legal retraction and later terminal classification without retaining typed payload. A bounded non-regional deletion scenario now adds a delivered nonconstrained removal, a constrained pending removal, execution-owned object/name/known-state and committed-ownership restoration, then Request Retraction only for the delivered recipient; a follow-on case proves a departed delivered owner is not restored or notified. The lifecycle now retains a lightweight terminal record after successful retraction or a producer boundary that makes a designator no longer legal, while releasing normal typed payload after pending delivery drains and deletion state/name after terminal deletion drains. Focused cases cover a queued interaction that still delivers after terminalization and a no-recipient deletion that frees its name. The rule itself is taken from the 2025 source and not from the sibling. Complete alternate-advance, regulation-disable/re-enable, save/restore, in-flight ownership, other resignation, recovery, and transport matrices remain future work. |

**2026-08-18 update:** the final intake sentence in `SB-TIME-008` predates
the C++-native in-transit callback regression and the mixed
TAR/NMR/TARA/NMRA/FQR queued-TSO scenario. Those two shapes are now covered;
the remaining intake is role/resignation churn, future exclusion,
pending-advance restore, broader message-family, durable-snapshot, and
transport-restore stress after their C++ state boundaries are specified.

The initial normal-interaction Disable Time Regulation/re-enable regression
now proves that a live pending designator becomes temporarily unauthorized
while regulation is disabled, survives the callback-gated re-enable at the
same lookahead, and can be legally retracted before terminal classification.
Broader re-enable coverage remains backlog work rather than an inferred
conformance result.

The initial normal-interaction restore-lifetime regression now complements the
directed stale-handle case: it saves a live queued interaction, terminalizes it
after the save, restores the saved payload and recipient ledger, and verifies
delivery plus a legal restored retraction. This is a narrow C++-native rollback
case; the broader restore/tombstone matrix remains backlog work.

A complementary one-federate terminal-tombstone regression now saves a
successful-Retract record, creates distinct post-save traffic, and restores the
snapshot. It proves the saved handle stays `MessageCanNoLongerBeRetracted` while
the discarded handle is invalid; all remaining family and transport combinations
remain backlog work.

The initial process-local time-window regression now saves a time-regulating
member at logical time 3 with actual lookahead 2, changes both values to 5,
and restores the saved values. It is a narrow C++-native input to the
time-window backlog, not evidence for pending advances, time-constrained
state, durable snapshots, or transport recovery. A companion case saves actual
lookahead 5 with a deferred decrease to 1, consumes it after the save, restores
the snapshot, and advances again to prove the deferred target survives too.

`SB-TIME-001` was intentionally first.  It prevented a superficially helpful
Python helper test from silently choosing Umbra's time semantics; the official
2025 source now supplies the recorded decision.

## MOM service-reporting scenarios

| ID | Priority and phase | Sibling source | Umbra intake outcome |
| --- | --- | --- | --- |
| `SB-MOM-001` | Planned, one complete service-report vertical slice | `tests/mom/test_mom_catalog_validation_v012.py`; `tests/verification/test_mom_observer_slice_v013.py`; `tests/backends/test_python_backend_federation_extended.py` | Reuse the subject/witness/enable/drain topology only. Start from IEEE 1516.1-2025 §11.5.1 and prove three independently specified outcomes: no generated report while Service Reporting is disabled; exactly one eligible MOM interaction when it is enabled and file reporting is disabled; exactly one file record and no MOM interaction when both switches are enabled. The sibling's JSONL audit sink and its 2010-oriented outcome rules are not portable semantics. The file-record arm waits for RL-042's Table 5 log mapping. |
| `SB-MOM-002` | Planned, report success/failure matrix | `tests/mom/test_mom_catalog_validation_v012.py`; `tests/backends/test_python_backend_federation_extended.py`; `tests/backends/test_python_backend_object_ownership_extended.py` | Use the rejected-adjustment/no-positive-report shape and the declaration, federation, and ownership family sweeps as a checklist. For every admitted C++ wrapper, independently map supplied arguments, returned argument, declared exceptions, serial allocation, and callback cleanup to 2025 sources. A rejected MOM adjustment must not become a positive service report. Do not inherit the sibling's service names, exception encoding, or report count assumptions. |
| `SB-MOM-003` | Partially implemented, private MOM-object foundation | `tests/verification/test_mom_observer_slice_v013.py`; `tests/backends/test_python_backend_time_ddm_extended.py` | Umbra now retains a registry-owned joined-federate MOM snapshot with a common object identity, the static initial values (including the real report-file path), an immutable private federate point, switch-stable lifetime, resignation removal, and rejoin replacement. Add observer discovery/reflection, static-path identity, multi-federate isolation, requested-value, and callback-time withdrawal tests only after the public RTI-owned object route is source-backed. `HLAreportServiceFile` publication remains gated by both the Table 8/MIM timing conflict (RL-041) and the unresolved §6.9 producer-designator mapping for RTI-registered objects (RL-043); it must not be modeled after the sibling's audit sink. |
| `SB-MOM-004` | Pending source adjudication, RTI-owned callback producer | `packages/hla-backend-python1516e/src/hla/backends/python1516e/mom.py`; `state.py`; `tests/verification/test_mom_observer_slice_v013.py` | The source uses an unjoined `FederateHandle(0)` sentinel for RTI-created MOM object discovery/reflection. Record its observer topology as a negative-design artifact only. Before an Umbra callback test is written, resolve the 2025 producer-designator rule connecting §6.9 with §11.2 (RL-043); do not use a sentinel or substitute the represented federate silently. |

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
complete, and five bounded public timestamped families plus Request Retraction
paths for interaction, attribute-update, and non-regional deletion have exact
2025 contracts. A first tombstone/reclamation policy and producer
TAR/TARA/NMR/NMRA/FQR terminal-boundary matrix now exist, together with
no-recipient returned-designator evidence for normal interaction and attribute
update, plus disjoint-region interaction and attribute-update proofs. One
normal-interaction Disable Time Regulation/re-enable lifetime case and one
process-local saved-time-window rollback plus a deferred-lookahead rollback
are now evidenced. The next milestone is broader re-enable plus pending
time-window and save/restore terminal-lifetime matrices, then remaining message
families and uncovered deletion shapes.
