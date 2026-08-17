# HLA Requirements Lab observations

This is Umbra's decision log for observations about the adjacent HLA
Requirements Lab corpus and its exported `CorpusBundle`. It deliberately
separates verified data facts from proposed Lab refinements. An observation is
not a claim that the Lab, IEEE source, or Umbra is non-conformant.

## Pinned input

- Lab repository: `../Document-Recreation`
- Reviewed revision: `4f012fb1c21367cfde67aab8498ae00e2a64c615`
- Umbra lock: `compliance/requirements-lab.lock.json`
- Last reviewed: 2026-08-16

Umbra consumes only the Lab's exported portable JSON bundle. The generated
bundle under `.compliance/` is intentionally ignored, so each observation
below names the pinned revision and durable source identifiers rather than
depending on an uncommitted export artifact.

## Active observations

### RL-001 — Time-management ownership is clause-granular

**Status:** verified; possible granularity refinement, not a defect.

The zero-lookahead statement is exported as
`requirement-candidate-content-clauses-08-time-management-page-182-l43-13`.
Its raw semantic-inventory source is
`content/clauses/08-time-management-page-182.tex:43-45`, and its semantic
owner is `clause-8`. The statement belongs under the standard's §8.1.5
discussion, but the bundle does not export `clause-8.1.5` as its owner.

**Umbra impact:** the time-bounds contract initially assumed `clause-8.1.5`.
The Requirements-Lab checker correctly rejected that assumption; Umbra now
uses the exported owner, `clause-8`.

**Possible Lab refinement:** preserve the broad `owner_id` for stable
semantic grouping, and additionally export an optional section-level field
(for example, `semantic_section_id`) when the source has a reliable section
anchor. That would support finer reporting without changing existing owner
semantics or inventing a normative claim.

### RL-002 — One exported title has mismatched source provenance

**Status:** verified export presentation mismatch; root cause not yet known.

For the same zero-lookahead record, the exported title reads
`Normative statement at content/clauses/annexes-page-439.tex:43`, while the
Lab inventory records the source as
`content/clauses/08-time-management-page-182.tex:43-45`. The stable record
ID, owner, and source evidence remain usable; the concern is the human-facing
title/provenance label.

**Umbra impact:** none on contract matching, because Umbra identifies the
record by immutable ID and checks its exported owner. The title is misleading
for a reviewer trying to navigate to the source text.

**Possible Lab refinement:** generate the display title from the same source
path and line span carried by the inventory/export record, or explicitly
explain why a different provenance path is intentional.

### RL-003 — Conservative candidates often have no API mapping

**Status:** verified coverage boundary; not a defect.

Several temporal requirement candidates, including the zero-lookahead
statement, are exported with a `no-api` relationship and a cross-cutting
classification. The Lab therefore gives Umbra source-level traceability but
does not assert an API/service or state-transition mapping for those facts.

**Umbra impact:** current contracts may link a private time-bound or grant
policy kernel to the source-derived requirement and a Catch2 selector, but
they cannot honestly promote that link as a Lab-provided public API mapping or
catalog/JUnit conformance evidence.

**Possible Lab refinement:** optionally provide a separately labeled
policy/state-machine relationship for requirements that are genuinely
cross-cutting. It must remain distinct from an API mapping so the Lab's
current conservative interpretation is preserved.

### RL-004 — Source-page and printed-page conventions need an explicit note

**Status:** verified documentation opportunity; not a defect.

The time-management source includes `\\HLASourcePage{183}{182}`. Umbra's
record ID uses the source-content page (`page-182`), while the rendered IEEE
page is 183. Both references are useful, but the difference can look like a
bad citation when no convention is stated.

**Umbra impact:** contracts use the immutable record ID rather than a page
number. Human-facing design notes should say whether a citation is referring
to the rendered standard page or the Lab source-content page.

**Possible Lab refinement:** document this convention in the bundle/readme or
export both labels explicitly.

### RL-005 — Support-type declarations are outside the exported API surface

**Status:** verified coverage boundary; not a defect.

At the pinned revision, the Lab's `api-surfaces.json` and exported bundle use
`VariableLengthData` as a parameter type for service declarations, but export
no standalone API surface whose owner is `VariableLengthData`.  The raw
semantic source `content-15161-page-421` lists `VariableLengthData.h` and the
`RTI/encoding/` headers as part of the normative C++ API.

**Umbra impact:** the `VariableLengthData` support-type tests can cite the
byte-identical official header and its API comments, but cannot honestly use
an `api-reference` contract or claim a Lab-exported standalone support-type
mapping.  The Catch2 plan records that boundary explicitly.

**Possible Lab refinement:** optionally export support-type declarations and
their members in a clearly separate API-surface category.  That would make
header-level tests traceable without suggesting that a value-type method is a
public RTI service or a conformance-mapping record.

### RL-006 — The Requirements Lab is not an IEEE 1516.2-2025 schema source

**Status:** verified scope boundary; not a defect.

The pinned Lab has useful IEEE 1516.2-2010 semantic material and an old FDD
schema companion, but it does not export the complete IEEE 1516.2-2025 DIF,
FDD, and OMT schema resource set. Umbra therefore verifies those three
unmodified 2025 XSDs against its separately reviewed IEEE archive and does not
use the Lab's 2010 companion schema as a substitute.

**Umbra impact:** Requirements-Lab contracts may trace Annex C behavior and
the native API, while the private XML/XSD validator must independently retain
its 2025 resource provenance, schema-policy tests, and source-license
attribution. A SISO FOM declaring the 2010 namespace is an expected
cross-edition rejection in Umbra's current profile, regardless of a product or
publication date in its title.

**Possible Lab refinement:** optionally expose an edition-labelled inventory of
companion XML/XSD artifacts that are present in the Lab, with an explicit note
that it is not a substitute for source-licensed current-edition resources.

### RL-007 — Annex C.8 duplicate-switch warning text is grammatically ambiguous

**Status:** verified source-text/export ambiguity; possible normalization
refinement, not a standards defect.

At the pinned revision, requirement
`requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-113-l50-9`
has the same text in `statement` and `machine_statement`, including the joined
tokens `beprovided`, `beinserted`, and `insertedfail`. The associated semantic
content element, `content-element-content-15162-page-113-list-item-019`,
restores the first two spaces but still reads "a warning the merge shall be
provided" and ends in `insertedfail`. The neighbouring C.8 records make the
limited intent usable: compare a switch name, ignore a duplicate, retain the
existing setting, and warn when the duplicate is not equivalent.

**Umbra impact:** `mergeSwitches` records a private preflight warning and
retains the first matching switch. Umbra does not infer a public warning API,
JUnit result, or conformance claim from the ambiguous wording.

**Possible Lab refinement:** preserve the source-faithful statement, but add a
separately labelled normalized-reading field or reviewer note for obvious token
joins and grammatical truncation. That would help implementers without
silently rewriting the normative source.

### RL-008 — Data-type wording needs a schema-constraint cross-reference

**Status:** verified terminology/constraint boundary; possible mapping
refinement, not a standards defect.

The source-derived 4.14 records, including
`requirement-candidate-sections-semantic-clause-4b-page-068-l41-3`, enumerate
the higher-level simple, enumerated, reference, array, fixed-record, and
variant-record data-type tables. The vendored official
`IEEE1516-OMT-2025.xsd` has a broader executable `dataTypeKey`: it includes
`basicData` names as well, and its `dataTypeRef` applies to every `dataType`
element. A complete 2025 model can therefore use a declared basic-data name
such as `HLAinteger32BE` as a direct `dataType` reference.

**Umbra impact:** the private composed-model resolver follows the official
2025 OMT key and accepts all seven declared name families. The Requirements-Lab
records remain source traceability, not a claim that the Lab itself exports all
schema constraints or that this preflight is a conformance result.

**Possible Lab refinement:** link relevant 1516.2 semantic records to a
separately labelled schema-constraint inventory, or annotate the basic-data
name inclusion. That would let a reviewer distinguish prose table terminology
from the complete XML key/keyref rule without rewriting either source.

### RL-009 — Representation-reference enforcement needs a reviewed interpretation

**Status:** verified cross-source tension; deferred implementation decision, not
a Requirements-Lab defect.

The source-derived record
`requirement-candidate-sections-semantic-clauses-5-6-page-094-l54-10` says a
Representation field shall reference a Basic Data Representation row. The
provided 2025 Restaurant FOM uses `HLAboolean` as the representation of its
`Preparation` simple data type, while the provided 2025 MIM declares
`HLAboolean` as an enumerated data type rather than `basicData`. The MIM also
uses higher-level attribute types in `referenceDataType` representation fields,
which is consistent with the distinct reference-data rules. The strict OMT XSD
has a broad `representationRef` keyref, but the supplied modules are designed
for DIF validation and are not independently complete OMT documents.

**Umbra impact:** Umbra does not add a generic rule that every `representation`
value must name `basicData`; that would reject the supplied 2025 example
module. The private resolver remains limited to the independently corroborated
`dataType` key for generic data-type references. Generic representation
validation and the two special instance identifiers stay explicitly open until
the wording, schema scope, and official examples can be reconciled. Reference
class existence and ordinary referenced-attribute/type matching are checked
separately against the completed object hierarchy.

**Possible Lab refinement:** attach a non-normative cross-reference from the
representation records to the relevant 2025 schema constraints and supplied
standard examples, with a reviewer note when a rule is not safe to apply as a
generic module-composition predicate.

### RL-010 — FOM synchronization-capability enforcement needs a reviewed interpretation

**Status:** verified cross-source tension; deferred implementation decision,
not a Requirements-Lab defect.

The source-derived record
`requirement-candidate-sections-semantic-clauses-5-6-page-093-l78-15` says
the synchronization capability field is `NA` for FOMs and FOM modules. The
provided 2025 `RestaurantFOMmodule-2025.xml` instead uses `Achieve` for three
synchronization points and `RegisterAchieve` for another. Both the 2025 DIF
and OMT XSDs enumerate those values alongside `NA`, but do not encode a
model-kind-specific assertion.

**Umbra impact:** Umbra keeps the executable XSD enumeration check and does
not add a FOM-only `NA` predicate that would reject the supplied 2025 FOM.
The source record remains useful traceability, but FOM-specific capability
enforcement is held open pending a reviewed reconciliation of the prose,
schema, and official example.

**Possible Lab refinement:** attach a non-normative cross-reference from this
record to the 2025 schema and supplied example, or record a reviewer note
about the intended scope of the FOM-only `NA` statement. That would make the
deferred predicate visible without changing the source-faithful record.

### RL-011 — The Send Interaction published-class sentence is absent from exported requirements

**Status:** verified export omission; possible extraction refinement, not a
standards defect.

At the pinned revision, the semantic content element
`content-element-content-15161-page-084-paragraph-001` contains the Clause
5.1.3 sentence: “Joined federates may invoke the Send Interaction service only
with a published interaction class as an argument.” The matching content block
`content-15161-page-084` begins its `record_ids` at
`requirement-candidate-content-clauses-05-declaration-management-page-084-l17-5`,
which is the following sent-class statement. Neither
`semantic/requirements.json` nor Umbra's exported CorpusBundle contains a
candidate for the source lines 5–8 / that published-class sentence.

**Umbra impact:** the receive-order interaction requirements contract does not
invent a source-derived record for the publication precondition. Umbra instead
pins the exact C++ `Send Interaction` declaration (including
`InteractionClassNotPublished`) and tests that behavior through the official
binding. This remains API/source traceability, not validated evidence or a
conformance claim.

**Possible Lab refinement:** emit a detailed candidate requirement for that
sentence with its true source-content line span and preserve its preceding
§5.1.3 section provenance. The page-wide export currently assigns the following
interaction-class candidates to `clause-5.1.4`, whose heading begins later on
the same source page; that broader attribution should be reviewed separately.
This would allow an implementation to trace the public publication precondition
without synthesizing a requirement ID.

### RL-012 — The available-attribute declaration rule is not exported as a granular candidate

**Status:** verified export omission; possible extraction refinement, not a
standards defect.

The Clause 5.1.2 source content represented by
`content-15161-page-082` defines an iff constraint: an attribute can be used
with the four non-region object-class attribute declaration services only when
it is available on the supplied object class. In the pinned
`semantic/requirements.json`, the page moves from the generic
`requirement-candidate-content-clauses-05-declaration-management-page-082-l35-10`
record to the later subscription-state record at `...-l59-18`; there is no
candidate whose statement captures that available-attribute constraint.

**Umbra impact:** the object-class attribute declaration slice still validates
each supplied handle through the composed FOM's inherited-attribute directory,
and its Catch2 tests cover both inherited validity and a child-only attribute
rejected against its parent. Its requirements contract maps only the exported
state records and does not invent a source-derived ID for this validation rule.

**Possible Lab refinement:** emit one detailed candidate for the
available-attribute iff constraint with its true line span and Clause 5.1.2
provenance. That would let an implementation link the exact standard exception
path to a source record without relying on a generic section introduction.

### RL-013 — Passelization candidates are assigned to the following clause

**Status:** verified export ownership mismatch; possible extraction refinement,
not a standards defect.

The IEEE source page for §6.1.12, *Passelization*, contains the statements
exported as
`requirement-candidate-content-clauses-06-object-management-page-112-l48-14`,
`...-l51-15`, and `...-l66-20`. At the pinned revision, each record has
`clause_id` `clause-6.1.13`, even though the source section heading on that
page is §6.1.12 and the next section is §6.1.13.

**Umbra impact:** the receive-order attribute-update contract uses the
exported `clause-6.1.13` identifier so the Requirements-Lab checker remains
truthful and reproducible. Its source-facing implementation and design notes
refer to the standard's §6.1.12 for the actual passelization rule; the
contract does not present the export ownership as a standards citation.

**Possible Lab refinement:** split candidate ownership at the source section
boundary and reassign the §6.1.12 passelization records to `clause-6.1.12`.
Preserving record IDs while correcting their `clause_id` would allow existing
consumers to update their contracts deliberately.

### RL-014 — The unpublication/pending-acquisition guard has no granular candidate

**Status:** verified export omission; possible extraction refinement, not a
standards defect.

At pinned Requirements Lab revision `4f012fb1c21367cfde67aab8498ae00e2a64c615`,
the semantic block `content-15161-page-092` contains the Clause 5.3.3(f)
precondition that prevents unpublication when a joined federate has an
unowned corresponding instance attribute with a pending ownership-acquisition
action. Its explicit list includes `Attribute Ownership Acquisition If
Available` until the matching `Attribute Ownership Unavailable` or `Attribute
Ownership Acquisition Notification` callback. The exported bundle has no
candidate whose statement represents that precondition. Its two page-092
candidates, `requirement-candidate-content-clauses-05-declaration-management-page-092-l124-39`
and `...-l130-41`, instead describe later postconditions and are assigned to
`clause-5.3.6`.

**Umbra impact:** the declaration requirements contract does not invent a
source-derived ID. Umbra pins the exact C++
`unpublishObjectClassAttributes` declaration, including
`OwnershipAcquisitionPending`, and its ownership integration case directly
checks that this exception preserves the pending request's publication.
This is source/API traceability, not validated evidence or a conformance
claim.

**Possible Lab refinement:** emit a detailed candidate for Clause 5.3.3(f)
with its true source span and `clause-5.3.3` provenance. Reassigning the two
existing page-092 candidates to their actual source components should be
considered separately, so consumers can distinguish the precondition from
the later postconditions.

### RL-015 — Ownership-service candidates are attributed to later subsections

**Status:** verified export ownership mismatch; possible extraction
refinement, not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the ownership-management export
assigns several source records to a later subsection whose heading appears
after the source text.  The continuation of the general §7.8 acquisition
semantics on source page 162 is exported as
`requirement-candidate-content-clauses-07-ownership-management-page-162-l16-4`
through `...-l76-24` with `clause_id` `clause-7.8.3`, although the §7.8.1
heading begins later on that page.  The §7.8.4 publication postcondition at
`...-page-163-l24-5` is similarly assigned to `clause-7.9.1`.  The general
§7.11 records at `...-page-166-l14-1`, `...-l20-3`, and `...-l41-10` are
assigned to `clause-7.11.6`; the corresponding general §7.12 records at
`...-page-167-l13-1` and `...-l28-6` are assigned to `clause-7.12.5`.
The general §7.15 cancellation records at `...-page-170-l13-1` through
`...-l52-14` are assigned to `clause-7.15.5`, and the general §7.16
confirmation records at `...-page-171-l46-11` and `...-l52-13` are assigned
to `clause-7.16.4`. The general §7.13 Divestiture If Wanted records at
`...-page-168-l23-3` through `...-l44-10` are likewise assigned to
`clause-7.13.5`, despite appearing before the §7.13.1 heading. The
cross-cutting user-tag record `...-page-154-l66-21`, whose source text names
Divestiture If Wanted, is assigned to `clause-7.2` even though it appears in
the §7.1.4 tag discussion. The Unconditional Attribute Ownership Divestiture
record `...-page-150-l152-47` is assigned to `clause-7.1.1`, even though the
semantic source places its immediate-unowned sentence under §7.1.2.1. The
general §7.4 ownership-assumption records `...-page-157-l79-21` and
`...-l103-29` are assigned to `clause-7.4.3`, although both appear before the
§7.4.1 heading. In each case the semantic block visibly places the service
heading and text before the later subsection heading.

**Umbra impact:** ownership contracts retain the export's immutable candidate
and clause IDs so `requirements_lab.py check --contract` remains reproducible.
Source-facing implementation notes cite the actual IEEE 1516.1-2025 section
numbers, and no contract presents the exported later-subsection ID as a
standards citation.  This is traceability only, not validated evidence or a
conformance claim.

**Possible Lab refinement:** assign candidate ownership using the active
semantic section at each source span, rather than a later heading on the same
page.  Keeping the candidate IDs stable while correcting their `clause_id`
values would let consuming projects update deliberately and distinguish
general service semantics, postconditions, and subsection preconditions.

### RL-016 — The generated ownership state-chart omits the Divestiture If Wanted transition

**Status:** verified generated-diagram coverage gap; possible export refinement,
not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the generated
`output/semantic/state-machine-establishing-ownership.tex` contains no
`Divestiture If Wanted` edge or label. The adjacent semantic source block
`content-15161-page-151` explicitly describes the Figure 16 transition from
the owned to unowned state through `Attribute Ownership Divestiture If Wanted`
when an attribute is in the returned set. The generated diagram also omits
several other ownership-transfer labels, so this observation is about the
derived diagram's coverage rather than a claim about the IEEE figure itself.

**Umbra impact:** Umbra does not use the generated state chart as exclusive
implementation evidence for this transition. The Divestiture If Wanted
contracts instead cite source-derived candidate records and the exact C++ API
surface, while the runtime's notification/publication behavior is covered by
real Catch2 cases. No diagram-derived conformance claim is made.

**Possible Lab refinement:** preserve every legal source-figure transition in
the generated state-machine model, including a clearly labelled Divestiture If
Wanted transfer from owner to selected acquirer. A small source-to-generated
edge inventory would make such coverage gaps reproducible without requiring
consumers to infer behavior from a rendered diagram.

### RL-017 — The generated cancel-negotiated-divestiture edge targets Completing Divestiture

**Status:** verified semantic-model/state-chart inconsistency; possible model
refinement, not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`,
`semantic/ownership-and-temporal-state-machines.json` records
`establishing.cancel-divestiture` with source state
`establishing.owned.waiting` and target state
`establishing.owned.completing`. The generated
`output/semantic/state-machine-establishing-ownership.tex` reproduces that as
an arrow from `Waiting for a New Owner to be Found` to `Completing
Divestiture`, labelled `Cancel Negotiated Attribute Ownership Divestiture`.
The adjacent generated edge sends `Confirm Divestiture [failure: no
Acquisition Pending, or Federate Willing To Acquire exception]` from
`Completing Divestiture` back to `Waiting`. This leaves the cancellation edge
incompatible with the service prose: §7.14.4 says the cancelled attributes
are unavailable for divestiture, while the source description says the joined
federate no longer wants to divest them.

**Umbra impact:** Umbra does not use this generated edge as the transition
authority. Its bounded 2025 implementation removes the private pending
negotiated-divestiture state on a successful cancellation, keeps ownership
unchanged, and resumes ordinary pending-regular-acquisition planning. The
traceability contract keeps the immutable §7.14 candidate and marks all work
as source/API traceability only, not diagram-derived validation or
conformance.

**Possible Lab refinement:** revise the semantic transition target to the
model's `establishing.owned.not-divesting` state (or otherwise record the
intended state explicitly), set its service clause attribution to §7.14, and
regenerate the Figure 16 derivative. A regression test should assert the
source state, target state, label, and clause ID together so the generated
diagram cannot silently reintroduce the contradictory path.

### RL-018 — Region-service requirement titles use a stale annex source path

**Status:** verified export metadata inconsistency; possible Lab refinement, not a
standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the immutable candidate records
used for the 2025 region-template slice have `title` values such as
`content/clauses/annexes-page-439.tex:39`, while their authoritative `source.path`
values point to `content/clauses/09-data-distribution-management-page-226.tex`
or `content/clauses/08-time-management-page-217.tex` at the matching line.
The same pattern appears across the Create Region, Commit Region Modifications,
Delete Region, Get Range Bounds, Get Dimension Handle Set, and Set Range Bounds
records.

**Umbra impact:** Umbra contracts use the immutable candidate IDs and the
record `source.path`/line data rather than the stale display title. The
Requirements-Lab checker accepts these contracts, so this is not an
implementation blocker and does not support a conformance claim.

**Possible Lab refinement:** derive each candidate title from its `source.path`
and source line, or correct the stale annex path while preserving the immutable
record ID. A metadata regression check should assert that title and source
location refer to the same exported source span.

### RL-019 — Timestamp/retraction candidates export fragment statements

**Status:** verified export-shape limitation; possible export refinement, not a
standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the source-derived candidates used
for the timestamped interaction/retraction intake include records such as
`requirement-candidate-content-clauses-06-object-management-page-123-l160-45`,
`requirement-candidate-content-clauses-06-object-management-page-125-l97-28`,
and `requirement-candidate-content-clauses-08-time-management-page-210-l30-6`.
Their exported `statement` values terminate at a TeX percent marker (`%`) and
therefore contain only the beginning of the normative sentence. The immutable
IDs and clause IDs are stable and the Requirements-Lab checker accepts a
contract that references them, but the fragment alone cannot establish the
complete ordering, timestamp, or retraction semantics.

**Umbra impact:** the private TSO queue contract uses these IDs only as narrow
source/test anchors. Its notes explicitly preserve the incomplete statement
shape, and the implementation does not infer missing public semantics from
the fragments. No timestamped RTI ambassador overload or conformance claim is
based on this export.

**Possible Lab refinement:** export the complete normalized normative sentence
for each candidate (or provide a stable source-span payload alongside the
fragment) while preserving the immutable candidate ID. A checker fixture
should reject a source-derived requirement whose statement ends at a TeX line
continuation before it is presented as a standalone semantic record.

### RL-020 — Full semantic layout clarifies the LITS/GALT fragment boundary

**Status:** verified local-workflow observation; possible export refinement,
not a standards defect.

The pinned Lab bundle contains only fragments for the Clause 8 LITS candidates
(`...page-183-l118-39`, `...page-183-l121-40`, and `...page-183-l133-44`). The
adjacent 2025 semantic layout artifact in Document-Recreation reconstructs the
complete source-page text: GALT includes delivered-since-last-advance and
queued/in-transit timestamps, while LITS is based on GALT and future queued
TSO messages. That full context was necessary to define the private C++
coordinator contract without treating a fragment as a complete rule.

**Umbra impact:** the new coordinator contract still references only immutable
Lab IDs and records the implementation as private source/test traceability.
The reconstructed semantic layout is an interpretation aid, not a promoted
Requirements-Lab mapping or conformance evidence.

**Possible Lab refinement:** export a stable normalized source-span or
cross-record context link for multi-line normative statements, so the checker
can expose the complete GALT/LITS rule while retaining each immutable
candidate ID.

## Recording rules

When Umbra finds a new issue while exporting, checking, or using the Lab, add
an entry here with:

1. the pinned Lab revision and immutable record/mapping IDs;
2. the exact observed fields or checker result;
3. the effect on Umbra's traceability or test workflow; and
4. a clearly marked refinement proposal, if any.

Use **verified** only for reproducible data/checker facts. Use **possible
refinement** for a proposed Lab change. Do not relabel an observation as a
standards or conformance failure without independent evidence.

Entries stay in this log after Umbra has a local mitigation, so a future Lab
revision can be compared against the original decision and the relevant
Umbra contracts can be updated deliberately.
