# FOM stress-corpus intake backlog

## Purpose

This is Umbra's curated intake register for richer FOM families and scenario
plans observed in the adjacent legacy Python RTI.  It exists to make the
future FOM, declaration, object, interaction, DDM, TSO, and save/restore
slices face realistic model pressure early enough to shape their design.

It is not an import manifest, a claim that the listed models are IEEE
1516.2-2025-compatible, or a claim that Umbra can host them. No third-party
source XML or generated artifact from that source tree is vendored here. A
small Umbra-owned DIF fixture derived from the sibling's two-dimensional
RegionalThing scenario is the deliberate exception to the broader corpus
policy: `packages/umbra-rti-native/tests/data/
regional-two-dimensional-multi-attribute-fom.xml` exists only to drive the
focused native C++ DDM stress lane, and its source/API/traceability boundary is
recorded in the dedicated regional multi-attribute contracts.

### Implemented external-corpus guardrails

Umbra has an optional, developer-configured **2025 schema-positive,
semantic-negative** lane for four external modules: a base plus MessageTest,
TimeMgmtTest, and SpaceLite extensions.
`compliance/fom/external-2025-fom-corpus.json` locks a reviewed local snapshot's
paths, digests, namespace, and schema location without copying the XML into
this repository. Every module validates with the official 2025 DIF schema.

The pinned prototype snapshot nevertheless uses raw basic-data representations
in object-attribute and interaction-parameter data-type columns. That violates
the explicit IEEE 1516.2 table rule that requires a declared simple,
enumerated, reference, array, fixed-record, or variant-record data type. The
lane now asserts the resulting MIM-first preflight rejection is deterministic
and that embedded federation creation reports `InconsistentFOM`; it does not
weaken Umbra's 2025 semantic rule to preserve a prototype-only positive claim.
This is a corpus finding, not a Requirements-Lab defect. The fixtures stay
unvendored because their source identifies them as prototype engineering
material, and the 2025-only project scope precludes an adapter or conversion
path for other editions.

The same optional 2025 manifest also pins a small standalone source named
DirectedTSORestore2025. It is DIF-valid and composes successfully with the
official MIM, making it a small schema/composition guard only; its historical
filename must not be interpreted as proof of a directed-interaction, TSO, or
restore runtime scenario. The paired DirectedDDMRestore2025 source is a
separate **expected schema rejection**: it omits required Dimension-table
input data fields and places dimensions beneath attributes/interactions rather
than the official class-level containers. Umbra records its exact digest and
rejects it through the official 2025 DIF schema instead of repairing or
converting it. Neither source is copied into Umbra.

Umbra now has an optional, developer-configured SISO boundary lane. It keeps
the external XML out of this repository while pinning a local snapshot's paths,
digests, namespace, and schema location in
`compliance/fom/external-siso-fom-corpus.json`. The manifest covers 22 inputs:
the five-module SISO Space family, an RPR 3.0 foundation module, the RPR 2.0
family, and its Link 16/Link 11 extensions. All declare the IEEE 1516-2010
namespace, so the 2025 DIF test asserts **expected rejection** for every
pinned input. This is deliberately a schema-edition regression guard, not a
third-party interoperability result.

The test is enabled only by setting
`UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY` to a reviewed local corpus root in
the embedded libxml2 development profile. Umbra does not reuse the 2025
schemas or silently convert the source XML. When the external 2010 resource
root is also configured, the lane positively validates and composes the Space
family and RPR Foundation into lookup catalogs. It records the strict-schema
boundary for the full RPR 2.0 family: 15 of 16 modules validate, while
`RPR-Enumerations_v2.0.xml` remains rejected by the strict schema path because
its reference-identification value is not an `xs:anyURI`. The full family now
composes through the explicit RPR 2010 normalization path, and the 2025 API
creates/registers a representative RPR object without simulation. No
wire-codec, simulation, or runtime interoperability claim follows from these
tests.

The detailed classification of RPR load failures and open implementation
questions is tracked in [RPR FOM load-gap backlog](RPR-FOM-LOAD-GAP-BACKLOG.md).

The first explicit 2010 compatibility slice is now available through
`UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY` and the exact
`fomEdition=2010` runtime setting. Its reviewed resource manifest,
`compliance/fom/external-2010-fom-resources.json`, pins the official IEEE
2010 schemas/MIM by SHA-256 while keeping them external. An optional
`UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH` pins the sibling Target Radar module,
and an external SISO root supplies the Restaurant/RPR test inputs. These tests
validate/combine modules into a lookup catalog and
create named/anonymous objects; they deliberately do not claim 2010 API,
FDD, MOM, simulation, or interoperability coverage.

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

| Source-relative model                                                                             |         Size | SHA-256 at inspection                                              | Classification                                                                                    |
|---------------------------------------------------------------------------------------------------|-------------:|--------------------------------------------------------------------|---------------------------------------------------------------------------------------------------|
| `packages/hla-fom-target-radar/src/hla/foms/target_radar/resources/foms/TargetRadarFOMmodule.xml` |  6,827 bytes | `919DCC2FC52DCB5179AA995FE2B445CBB31688BFEF55A784BB94EBBA774793B1` | Cross-edition source; the sibling's validator selects a 2010 path.                                |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_Base.xml`                   | 26,035 bytes | `B6E883B68375A7D97C05A0CEDD98EBE07577DBA76593FC4318237BDDDEB08D6F` | 2025 base module.                                                                                 |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_MessageTest.xml`            | 32,789 bytes | `328A5ABA4EF346DB5DA682B58E81498D4D737AD99136CD8E105F5573AF608E31` | 2025 extension over the Proto base.                                                               |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_TimeMgmtTest.xml`           | 23,760 bytes | `74033B39B4AA1BF84AE7BBCD567C2362F2CBD1161AC73E1F90C7CEAE332BC870` | 2025 extension over the Proto base.                                                               |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/Proto2025_SpaceLite.xml`              | 29,132 bytes | `D6DF966B6623CB9B6AA91EC65E75DA20B6248F3D2E5D77ECC1AF8F34BA7AEFFB` | 2025 extension over the Proto base.                                                               |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/DirectedTSORestore2025.xml`           |  1,603 bytes | `672757D28088D8C87056F228AD6995FFEFF764D6DEFEA5F949BA0F2BFD11867D` | 2025 DIF-valid standalone composition guard; not a runtime directed/TSO/restore claim.            |
| `packages/hla-rti-core/src/hla/fom/resources/proto2025/foms/DirectedDDMRestore2025.xml`           |  1,830 bytes | `742ED390ABC0A059099EB92FEA904F4E5494A21F74DA433CF031E838E2A0B711` | 2025 namespace but official-DIF schema-negative source; preserved as an expected-rejection guard. |

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
| `FOM-STR-001` | **Target Radar.** The sibling's MIM-merged overview contains six object rows and 86 interaction rows, and its scenarios cover lifecycle, declaration, object, DDM, ownership, synchronization, and save/restore shapes.  This is a useful broader-than-tutorial model and scenario vocabulary. | Optional 2010 validation/composition and registration-only object creation through the 2025 API. | The sibling records this as cross-edition and selects an effective 2010 validation path. It is not a default 2025 input; the optional test does not exercise updates, interactions, DDM, ownership, synchronization, or save/restore. |
| `FOM-STR-002` | **Proto base + MessageTest.** A 2025-native base/extension pair with a concrete multi-federate plan: discovery and attributes, stimulus/response correlation, 100 cases with 1,000 steps, timeout/fault paths, late join, and replay. | The current digested prototype is DIF-valid but semantically rejected because it uses raw basic-data names in application-table columns. Repair or recreate a compact Umbra-owned legal fixture before using the plan to drive a service slice. | The source plan is a scenario design, not an oracle for C++ callback order or fault semantics. |
| `FOM-STR-003` | **Proto base + TimeMgmtTest.** A 2025-native time scenario family covering RO load, out-of-wall-clock TSO order, equal-timestamp batches, TAR/NER cycles, positive/zero lookahead, past events, retraction/flush probing, replay, and late join. | Keep as a schema-positive/preflight-negative guard until a reviewed legal snapshot or a compact Umbra-owned fixture is available. Then use the non-TSO subset only after each time service is genuinely implemented. | Umbra's private queue is integrated into the temporal snapshot and GALT/LITS calculator, including retraction and in-transit state, and the public profile has five bounded timestamped paths: interaction, attribute update, object deletion/removal, directed interaction, and region-context interaction. NER, flush, and the remaining timestamped families are not enabled; no broader TSO claim may be inferred. |
| `FOM-STR-004` | **Proto base + SpaceLite.** A 2025-native hierarchy/data-model scenario: reference-frame graph validity, entity/sensor state, attach/detach, freeze/resume, pacing, and late join. | Keep as a schema-positive/preflight-negative guard until a reviewed legal snapshot or a compact Umbra-owned fixture is available. Then add graph, discovery, and callback assertions after object/interaction state is real. | It is a compact scenario model, not a substitute for the independent SISO Space FOM family. |
| `FOM-STR-005` | **Directed TSO Restore.** A small 2025 source with one user object class and a timestamped interaction. | Current MIM-first schema/composition guard only; runtime TSO plus save/restore work remains later. | The source filename does not establish a directed interaction or a runtime restore oracle. It must not paper over missing TSO delivery or restore semantics. |
| `FOM-STR-006` | **Directed DDM Restore.** A small 2025-namespaced source with a routing dimension and restore focus. | Current official-DIF expected rejection; repair/recreation must precede any runtime use. | It places dimensions in a schema-incompatible shape and omits required Dimension-table fields. Do not silently transform it or use it to claim DDM/restore support. |
| `FOM-STR-007` | **RPR 2.0 ordered family.** A large tactical-model family with Foundation through Warfare modules; the sibling also exercises an integrated Link 16 shape.  It pressures ordered composition, datatypes, and deep class trees. | Optional 2010 per-module validation plus RPR Foundation catalog composition are implemented; full-family composition remains a later parser/composition task. | Its source manifest requires preservation of upstream SISO attribution.  The model family is treated there as 2010-shaped, not a ready 2025 runtime root. One module currently fails the strict official schema because its reference-identification value is prose rather than `xs:anyURI`. |
| `FOM-STR-008` | **Link 16 extension.** A companion to the RPR 2.0 family that is valuable as an incomplete-extension negative case and as an explicit ordered-composition test. | Included in the optional 2010 per-module validation boundary; integrated family use remains later. | Standalone Link 16 is not a normal federation root.  A green test must say whether it proves rejection, per-module validation, or full integrated composition. |
| `FOM-STR-009` | **RPR 3.0.** The adjacent corpus separates an Annex A ordered family from an informative merged 1516-2010 packet.  Together they offer current-family and single-large-document stress shapes. | License/provenance review, then parser-only experiments under an explicitly stated edition policy. | A 2025-labelled family name does not by itself establish that every module or merged packet is valid in Umbra's 2025 FDD pipeline. |
| `FOM-STR-010` | **SISO Space FOM ordered family.** Separate datatype, hierarchy, and ordered-module pressure with datatypes, environment, management, switches, and entity modules. | Optional 2010 validation and lookup-catalog composition are implemented; scenario concepts can still inform the smaller SpaceLite tests earlier. | It is independent of RPR, has its own ordering and attribution review, and the current test does not claim runtime Space behavior or interoperability. |
| `FOM-STR-011` | **NETN merged-with-RPR.** A high-volume merged HLA Evolved XML stress input, useful for resource limits, diagnostics, and non-mutating preflight. | Only after a license and immutable-input review; parser robustness rather than federation creation. | The recorded upstream license note is CC BY-ND 4.0 and calls for preserving XML unchanged.  Do not transform, merge, or vendor a derivative without a reviewed policy. |

The legacy map mentions a U-FOM family, but this pass did not locate a source
asset and provenance package for it.  It is deliberately not promoted to an
Umbra candidate until those facts are recorded.

## Planned extraction sequence

1. Keep using the official Restaurant modules and Umbra-owned malformed/minimal
   fixtures to complete the current 2025 preflight rules.  These are the right
   source of truth for a strict schema and Annex C behavior.
2. Keep the reviewed 2010 Target Radar/Restaurant lane focused on loading,
   catalog lookup, publication, and registration; do not widen it into a
   simulation or 2010 API claim until those contracts are separately designed.
3. Turn `FOM-STR-002` into a small Umbra-owned object/interaction acceptance
   fixture and use its scenario plan to drive one service slice at a time.
4. Apply the non-TSO subset of `FOM-STR-003` only after each time service is
   genuinely implemented; introduce a separate TSO corpus phase after the
   queue and retraction model exist.
5. Use `FOM-STR-004` to exercise object graph, discovery, and callback
   semantics separately from the now-available external Space catalog lane.
6. Promote `FOM-STR-005` and `FOM-STR-006` only when their paired capability
   groups (TSO/save-restore and DDM/save-restore) have real state models.
7. Keep the RPR full-family normalization/composition and the remaining
   third-party corpora in a distinct parser/composition lane.  Keep those
   inputs out of the normal green embedded-runtime target until provenance,
   edition policy, module order, normalization policy, and expected verdict
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

The first cross-edition loading/registration slice is complete. Keep its
external inputs opt-in and digest-checked. When the next object/interaction
vertical slice starts, open `FOM-STR-002` as a design task: extract a compact
Umbra-owned 2025 fixture, map its public expectations to the Requirements Lab,
and add Catch2 coverage only for the services actually implemented. Keep the
Target Radar scenario's richer DDM, ownership, synchronization, and
save/restore plans as separate follow-on work.
