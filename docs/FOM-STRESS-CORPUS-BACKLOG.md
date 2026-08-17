# FOM stress-corpus intake backlog

## Purpose

This is Umbra's curated intake register for richer FOM families and scenario
plans observed in the adjacent legacy Python RTI.  It exists to make the
future FOM, declaration, object, interaction, DDM, TSO, and save/restore
slices face realistic model pressure early enough to shape their design.

It is not an import manifest, a claim that the listed models are IEEE
1516.2-2025-compatible, or a claim that Umbra can host them.  No FOM XML,
third-party source, generated artifact, or test result from that source tree
has been copied into Umbra.

### Implemented external-corpus guardrails

Umbra now has an optional, developer-configured **2025-positive** lane for
four external modules: a base plus MessageTest, TimeMgmtTest, and SpaceLite
extensions. `compliance/external-2025-fom-corpus.json` locks a reviewed local
snapshot's paths, digests, namespace, and schema location without copying the
XML into this repository. The lane validates every module with the official
2025 DIF schema, composes standard MIM plus all four modules into a private
FDD/catalog twice for a deterministic result, and exercises the resulting
ordered module list through the embedded Create/Join development profile.

Those results are deliberately only `schema/preflight accepted` plus the
bounded embedded lifecycle scenario. They do not claim the MessageTest,
TimeMgmtTest, or SpaceLite runtime scenarios work: Umbra has not yet
implemented their object/interaction, callback, TSO, DDM, or save/restore
requirements. The fixtures stay unvendored because their source identifies
them as prototype engineering material, and the 2025-only project scope
precludes an adapter or conversion path for other editions.

Umbra now has an optional, developer-configured SISO boundary lane. It keeps
the external XML out of this repository while pinning a local snapshot's paths,
digests, namespace, and schema location in
`compliance/external-siso-fom-corpus.json`. The initial seven files are the
five-module SISO Space family, an RPR 3.0 foundation module, and a Link 16
extension. All declare the IEEE 1516-2010 namespace, so the current 2025 DIF
test asserts **expected rejection** for every one. This is deliberately a
schema-edition regression guard, not a third-party positive-validation or
interoperability result.

The test is enabled only by setting
`UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY` to a reviewed local corpus root in
the non-installable libxml2 development profile. Umbra deliberately has no
positive 2010 lane; it must not reuse the 2025 schemas or silently convert the
source XML.

The official IEEE 1516.2-2025 MIM, schemas, and Restaurant examples remain
the authoritative baseline for Umbra's current FOM validator.  The corpus
below complements those small canonical inputs with more demanding future
inputs; it does not replace them.

## Snapshot and provenance

- Source inspection: adjacent legacy Python RTI at revision
  `7d92a5e24563ad996d2ec7ea5c1840c7c4a9c9bc`, read on 2026-08-16.
- That source worktree had unrelated local changes.  Re-check both revision
  and worktree state before any later reading or acquisition.
- Paths below are relative to that source tree.  A recorded SHA-256 is a
  discovery snapshot only, not an Umbra-managed dependency digest.  Rehash a
  candidate at the point of approved acquisition.
- Every model must pass the intake gate before it becomes a test input.  In
  particular, a model can be useful for parser stress while being unsuitable
  as a 2025 federation root or runtime interoperability case.

### Selected source-tree snapshots

| Source-relative model | Size | SHA-256 at inspection | Classification |
| --- | ---: | --- | --- |
| `packages/hla-fom-target-radar/src/hla/foms/target_radar/resources/foms/TargetRadarFOMmodule.xml` | 6,827 bytes | `919DCC2FC52DCB5179AA995FE2B445CBB31688BFEF55A784BB94EBBA774793B1` | Cross-edition source; the sibling's validator selects a 2010 path. |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_Base.xml` | 26,035 bytes | `B6E883B68375A7D97C05A0CEDD98EBE07577DBA76593FC4318237BDDDEB08D6F` | 2025 base module. |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_MessageTest.xml` | 32,789 bytes | `328A5ABA4EF346DB5DA682B58E81498D4D737AD99136CD8E105F5573AF608E31` | 2025 extension over the Proto base. |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_TimeMgmtTest.xml` | 23,760 bytes | `74033B39B4AA1BF84AE7BBCD567C2362F2CBD1161AC73E1F90C7CEAE332BC870` | 2025 extension over the Proto base. |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_SpaceLite.xml` | 29,132 bytes | `D6DF966B6623CB9B6AA91EC65E75DA20B6248F3D2E5D77ECC1AF8F34BA7AEFFB` | 2025 extension over the Proto base. |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/DirectedTSORestore2025.xml` | 1,603 bytes | `672757D28088D8C87056F228AD6995FFEFF764D6DEFEA5F949BA0F2BFD11867D` | Small 2025 directed TSO/save-restore fixture. |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/DirectedDDMRestore2025.xml` | 1,830 bytes | `742ED390ABC0A059099EB92FEA904F4E5494A21F74DA433CF031E838E2A0B711` | Small 2025 directed DDM/save-restore fixture. |

## Intake gate

Before XML from any entry can enter Umbra, record all of the following in a
dedicated dependency/corpus manifest and review it:

1. upstream owner, source URL or archive identity, license, attribution, and
   whether redistribution in Umbra is permitted;
2. exact source revision or archive checksum, individual SHA-256, and the
   source path or ordered module list;
3. namespace, schema edition, MIM relationship, module ordering, and whether
   the model is intended to be a root, extension, expected rejection, parser
   stress input, or runtime scenario;
4. the particular Requirements-Lab source record for each public-service
   expectation; and
5. an Umbra-native Catch2 oracle with its profile, cleanup boundary, and
   verdict category.

The verdict categories must remain separate:

| Verdict | Meaning |
| --- | --- |
| `schema/preflight accepted` | Umbra accepted the model only through the private FOM preparation boundary. |
| `expected rejection` | Umbra rejected an intentionally unsupported, invalid, or cross-edition input for a recorded reason. |
| `embedded runtime scenario` | Implemented Umbra services exercised the model through the official C++ API. |
| `interoperability scenario` | A separately configured cross-process or external-RTI test has evidence. |

None of these categories alone establishes a broad conformance claim.

## Candidate families

| ID | Candidate and stress value | First admissible Umbra use | Important boundary |
| --- | --- | --- | --- |
| `FOM-STR-001` | **Target Radar.** The sibling's MIM-merged overview contains six object rows and 86 interaction rows, and its scenarios cover lifecycle, declaration, object, DDM, ownership, synchronization, and save/restore shapes.  This is a useful broader-than-tutorial model and scenario vocabulary. | Design reference only; excluded from Umbra's current 2025-only implementation plan. | The sibling records this as cross-edition and selects an effective 2010 validation path. It is not a default 2025 input, and no direct import is planned. |
| `FOM-STR-002` | **Proto base + MessageTest.** A 2025-native base/extension pair with a concrete multi-federate plan: discovery and attributes, stimulus/response correlation, 100 cases with 1,000 steps, timeout/fault paths, late join, and replay. | The external 2025 lane now proves DIF validation, full MIM-first composition, and Create/Join with the complete four-module set. Build the scenario only once Umbra owns FOM handles, declaration, object/interaction delivery, and callbacks; recreate the smallest legal Umbra-owned fixture first. | The source plan is a scenario design, not an oracle for C++ callback order or fault semantics. |
| `FOM-STR-003` | **Proto base + TimeMgmtTest.** A 2025-native time scenario family covering RO load, out-of-wall-clock TSO order, equal-timestamp batches, TAR/NER cycles, positive/zero lookahead, past events, retraction/flush probing, replay, and late join. | The external 2025 lane now proves only its schema/preflight and embedded lifecycle admission. Use receive-order and TAR portions only after the corresponding private service exists; reserve broader public TSO portions for the coordinator/service phase. | Umbra's private queue is integrated into the temporal snapshot and GALT/LITS calculator, including retraction and in-transit state, and the public profile has three bounded non-regional timestamped paths: interaction, attribute update, and object deletion/removal. NER, flush, and the remaining timestamped families are not enabled; no broader TSO claim may be inferred. |
| `FOM-STR-004` | **Proto base + SpaceLite.** A 2025-native hierarchy/data-model scenario: reference-frame graph validity, entity/sensor state, attach/detach, freeze/resume, pacing, and late join. | The external 2025 lane now proves only schema/preflight and embedded lifecycle admission. Add graph, discovery, and callback assertions after object/interaction state is real. | It is a compact scenario model, not a substitute for the independent SISO Space FOM family. |
| `FOM-STR-005` | **Directed TSO Restore.** A deliberately small 2025 fixture with one user object class and a timestamped interaction, aimed at directed-TSO cleanup across restore. | TSO plus save/restore phase, after recipient-scoped queues and restore state exist. | It must not be used to paper over missing TSO delivery or restore semantics. |
| `FOM-STR-006` | **Directed DDM Restore.** A deliberately small 2025 fixture with one user object class, a routing dimension, routed object attribute/interaction, and restore focus. | DDM plus save/restore phase, after regions, routing, and restoration state exist. | It is useful specifically because it combines capability groups; do not add it until both groups have real invariants. |
| `FOM-STR-007` | **RPR 2.0 ordered family.** A large tactical-model family with Foundation through Warfare modules; the sibling also exercises an integrated Link 16 shape.  It pressures ordered composition, datatypes, and deep class trees. | Later third-party parser/composition lane only, starting from a licensed immutable acquisition. | Its source manifest requires preservation of upstream SISO attribution.  The model family is treated there as 2010-shaped, not a ready 2025 runtime root. |
| `FOM-STR-008` | **Link 16 extension.** A companion to the RPR 2.0 family that is valuable as an incomplete-extension negative case and as an explicit ordered-composition test. | Controlled expected-rejection fixture first; integrated family only after ordered RPR 2.0 composition is supported. | Standalone Link 16 is not a normal federation root.  A green test must say whether it proves rejection or full integrated composition. |
| `FOM-STR-009` | **RPR 3.0.** The adjacent corpus separates an Annex A ordered family from an informative merged 1516-2010 packet.  Together they offer current-family and single-large-document stress shapes. | License/provenance review, then parser-only experiments under an explicitly stated edition policy. | A 2025-labelled family name does not by itself establish that every module or merged packet is valid in Umbra's 2025 FDD pipeline. |
| `FOM-STR-010` | **SISO Space FOM ordered family.** Separate datatype, hierarchy, and ordered-module pressure with datatypes, environment, management, switches, and entity modules. | Later parser/composition lane; scenario concepts can inform the smaller SpaceLite tests earlier. | It is independent of RPR, has its own ordering and attribution review, and is not an add-on to the tactical family. |
| `FOM-STR-011` | **NETN merged-with-RPR.** A high-volume merged HLA Evolved XML stress input, useful for resource limits, diagnostics, and non-mutating preflight. | Only after a license and immutable-input review; parser robustness rather than federation creation. | The recorded upstream license note is CC BY-ND 4.0 and calls for preserving XML unchanged.  Do not transform, merge, or vendor a derivative without a reviewed policy. |

The legacy map mentions a U-FOM family, but this pass did not locate a source
asset and provenance package for it.  It is deliberately not promoted to an
Umbra candidate until those facts are recorded.

## Planned extraction sequence

1. Keep using the official Restaurant modules and Umbra-owned malformed/minimal
   fixtures to complete the current 2025 preflight rules.  These are the right
   source of truth for a strict schema and Annex C behavior.
2. Turn `FOM-STR-002` into a small Umbra-owned object/interaction acceptance
   fixture and use its scenario plan to drive one service slice at a time.
3. Apply the non-TSO subset of `FOM-STR-003` only after each time service is
   genuinely implemented; introduce a separate TSO corpus phase after the
   queue and retraction model exist.
4. Use `FOM-STR-004` to exercise object graph, discovery, and callback
   semantics before attempting a full external Space family.
5. Promote `FOM-STR-005` and `FOM-STR-006` only when their paired capability
   groups (TSO/save-restore and DDM/save-restore) have real state models.
6. Open a distinct third-party parser/composition lane for `FOM-STR-007`
   through `FOM-STR-011`.  Keep it out of the normal green embedded-runtime
   target until provenance, edition policy, module order, and expected verdict
   are all recorded.

## Test-design resources to retain

The adjacent test plans are more valuable than their implementation details:

- `Proto2025_MessageTest_TestPlan.md` supplies lifecycle, pub/sub,
  correlation, load, fault, late-join, and replay scenario shapes.
- `Proto2025_TimeMgmtTest_TestPlan.md` supplies a deterministic
  `HLAinteger64Time` profile, equal-timestamp batching, advance cycles, and
  progress/replay reporting ideas.
- `Proto2025_SpaceLite_TestPlan.md` supplies graph, entity state, sensor,
  attachment, freeze/resume, pacing, and late-join acceptance shapes.
- `section8_matrix.py`, `scenario_target_radar_time.py`, and
  `scenario_save_restore.py` identify high-value negative and cross-capability
  boundaries.  They remain hypotheses until independently tied to the
  Requirements Lab and restated in C++.

No legacy pytest markers, proof wording, callback ordering assumptions, or
runtime results are imported with these ideas.

## Next action

When the object/interaction vertical slice starts, open `FOM-STR-002` as a
design task: extract a compact Umbra-owned 2025 fixture, map its public
expectations to the Requirements Lab, and add Catch2 coverage only for the
services actually implemented.  Keep `FOM-STR-001` as the first broader
cross-edition design corpus, not as a silent change to the current 2025
baseline.
