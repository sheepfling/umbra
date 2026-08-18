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

### RL-018 — Requirement titles use a stale annex source path

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

**2026-08-18 follow-up:** the bounded default-region contract uses the same
source-path-over-title rule for its page-216 default-region candidate. The
exported clause metadata and immutable IDs were coherent enough to trace the
receive-order and direct time-constrained timestamped object/interaction
behavior; no additional Requirements Lab defect was discovered during that
work.

**2026-08-18 delayed-subscription follow-up:** the same stale-title pattern
affects the page-187 Delay Subscription Evaluation candidates. Their
authoritative source block visibly labels the behavior as §8.1.8, while the
exported candidate metadata labels it `clause-8.1.10`. Umbra's dedicated
delayed-subscription contract preserves the immutable IDs and checker-facing
metadata, but cites §8.1.8 in its human-facing scope note. This is a Lab
metadata defect to refine, not a standards or implementation defect. The later
ordinary known-object Update Attribute Values extension reuses the same three
candidates and exposed no additional Requirements Lab discrepancy.

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
source/test anchors. The bounded normal timestamped Send Interaction, Send
Interaction With Regions, Update Attribute Values (including the bounded
regional path), and Send Directed Interaction Request Retraction contracts
also use the available Retract/Request Retraction anchors, but record that the
strict Clause 8.22.3 time-plus-actual-lookahead paragraphs are absent as
standalone candidate records. The implementation verifies that condition from
the authoritative reconstructed 2025 source rather than inferring it from the
fragments. No conformance claim is based on this export.

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

### RL-021 — Get Update Rate Value For Attribute has a clause-number drift

**Status:** verified export/source-layout inconsistency; possible Lab refinement,
not a standards defect.

At pinned revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the requirement candidate
`requirement-candidate-content-clauses-10-support-services-page-252-l17-1`
is exported with `clause_id` `clause-10.13.2`, while the authoritative
source-content block for page 252 labels the service `10.12` and its returned
arguments `10.12.2`. The separate C++ API surface carries `clause-10.12`.
The candidate statement itself is complete across the exported source span,
but the clause labels disagree across the Lab artifacts.

**Umbra impact:** the update-rate-value requirements contract uses the exact
immutable candidate ID and records the exported `clause-10.13.2`; the API
contract uses the exact API surface and its `clause-10.12`. This preserves
checker acceptance without pretending the two clause labels are equivalent.
The bounded C++ test and implementation remain development-profile
traceability only.

**Possible Lab refinement:** reconcile the generated requirement clause with
the source-content service heading (or export an explicit source-heading ID
alongside the semantic owner) while preserving the immutable candidate ID.
A cross-artifact consistency check should flag a requirement/API pair when
their service names match but their exported clause labels disagree.

### RL-022 — Turn Updates On C++ crosswalk leaves the overload choice ambiguous

**Status:** verified crosswalk ambiguity; possible Lab refinement, not a standards
defect.

At pinned revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the generated C++ worklist records
both `cpp:m16.transition.ownership-updates-on` and
`cpp:m16.transition.ownership-updates-on-again` with `crosswalk_status`
`ambiguous`, candidate API surfaces
`api.2025.cpp.federateambassador.turnupdatesonforobjectinstance.45e8b672fc3e`
and
`api.2025.cpp.federateambassador.turnupdatesonforobjectinstance.b721f45f5535`,
and no `selected_api_surface_ids`. The two candidates are the official
rate-bearing and no-rate C++ overloads, while the transition names do not carry
enough information to choose one.

**Umbra impact:** Umbra's source/API contracts pin both immutable API IDs and
the Catch2 cases verify the overload selection, including callback-time
re-evaluation and a regional explicit-rate transition. The cases remain
development-profile traceability only and cannot enter the Lab's real catalog
until a mapping selects the appropriate overload(s).

**Possible Lab refinement:** split the transition mapping by callback contract
(`ownership.updates-on.no-rate` and `ownership.updates-on.rate-bearing`) or allow
one mapping to select both overload IDs with explicit applicability predicates.
Preserve the existing immutable mapping IDs and expose the selected overload
semantics in the generated worklist.

### RL-023 — Dynamic Auto Provide semantics are present in 1516.2 content but not a 1516.2 requirement record

**Status:** verified export-shape boundary; possible cross-document export
refinement, not a standards defect.

At pinned revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the 1516.2 semantic content
record `requirement-candidate-sections-semantic-clause-4b-page-066-l6-1`
states that Auto Provide is a federation-wide dynamic switch and names the
MOM interaction
`HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches`.
The 1516.1 requirements export separately contains the normative
modifiability record
`requirement-candidate-content-clauses-06-object-management-page-110-l10-2`,
but the portable bundle does not expose the 1516.2 semantic record as a
requirement document that the current checker can cross-reference.

**Umbra impact:** the MOM implementation contract uses the 1516.1
modifiability candidate so `requirements_lab.py check` remains a same-document
validation. This observation records the 1516.2 semantic/MOM source as the
interpretive basis; it is not presented as a second Lab requirement mapping or
as conformance evidence. The Catch2 case independently verifies the standard
MIM class, parameter, four-byte `HLAswitch` values, federation-wide getter
change, and subsequent discovery behavior.

**Possible Lab refinement:** export a stable, edition-qualified relationship
from the 1516.1 Auto Provide requirement to the 1516.2 switch-table semantic
record (or add a clearly labelled 1516.2 requirements document). Extend the
checker with an explicit cross-document reference mode rather than requiring
consumers to duplicate a source ID under the wrong document.

### RL-022 — The temporal state-chart label says “Disable Asynchronously Delivery”

**Status:** verified generated-label typo; possible export refinement, not a
standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`,
`semantic/ownership-and-temporal-state-machines.json` labels the asynchronous
delivery transition `Disable Asynchronously Delivery`, while the normative
requirement candidates and official C++ API surface use `Disable Asynchronous
Delivery`. The machine still identifies the intended transition, so this is a
presentation/normalization defect rather than missing behavioral coverage.

**Umbra impact:** the asynchronous-delivery contracts use the immutable clause
IDs and exact API surface IDs, and their tests cite the official service name;
they do not treat the generated state-chart label as an API or conformance
authority.

**Possible Lab refinement:** normalize generated transition labels from the
canonical service title and add a small label-integrity check against the
requirements/API corpus, while preserving the existing immutable transition
ID.

### RL-024 — The 1516.2 automatic-resign default differs from the 2025 FDD XSD default

**Status:** verified cross-artifact default mismatch; requires an explicit
edition decision, not silent implementation drift.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the 1516.2 requirement
`requirement-candidate-sections-semantic-clause-4b-page-067-l50-4`
states that an omitted Automatic Resign Action defaults to
`CancelThenDeleteThenDivest`. The checked-in IEEE 1516.2-2025 FDD/OMT/DIF XSD
resources declare the `resignAction` attribute default as `NoAction` instead.
The XML validator accepts an omitted attribute, so validation alone does not
resolve which semantic default should be materialized.

**Umbra impact:** the catalog and embedded membership path use the
Requirements-Lab normative candidate (`CancelThenDeleteThenDivest`) and retain
the XSD lexical value only when it is explicitly present. This is recorded as
an intentional standards-traceability choice, not as conformance evidence.

**Possible Lab/refinement action:** reconcile the published 1516.2/XSD default
with the normative table (or publish an erratum/precedence rule), then update
Umbra's contract and fixture if the authoritative value changes. Until that
decision is resolved, do not claim the automatic-resign default is validated.

### RL-025 — Federate Resigned C++ crosswalk also selects Connection Lost

**Status:** verified crosswalk over-match; possible export refinement, not a
standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`,
`api-requirement-bindings.json` exports both the correct C++ callback binding
`binding.req-federate-resigned-callback-behavior.cpp.api-2025-cpp-federateambassador-federateresigned-0f7e5f7b1858`
and a second, unrelated binding ending in
`api-2025-cpp-federateambassador-connectionlost-e952ebb6c2e6` for the same
`req-federate-resigned-callback-behavior` requirement. The requirement itself
distinguishes the services: an involuntarily resigned federate remains
connected and receives `Federate Resigned`; a `Connection Lost` event implies
resignation but moves the federate to Not Connected and does not require a
Federate Resigned callback.

**Umbra impact:** the embedded Connection Lost contracts select only
`FederateAmbassador::connectionLost` for the two explicit Connection Lost
requirements. A future Federate Resigned slice must select only the exact
`FederateAmbassador::federateResigned` API surface and retain the different
connected/not-joined lifecycle. Umbra will not treat the extra crosswalk row
as evidence that the callbacks are interchangeable.

**Possible Lab refinement:** constrain callback crosswalk generation by the
canonical service title and modeled transition, or label the Connection Lost
association as contextual rather than a matched C++ API binding. Preserve the
immutable IDs while removing the false selected surface from the Federate
Resigned worklist.

### RL-026 — Delete Object Instance scope is limited to its two official overloads

**Status:** verified Umbra scope correction; not a Requirements-Lab defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the exact C++ API surface
`api.2025.cpp.rtiambassador.deleteobjectinstance.c05d20bc2175` corresponds to
the receive-order and timestamped `RTIambassador::deleteObjectInstance`
overloads. Umbra's imported official `RTI/RTIambassador.h` baseline contains
those two overloads and no directed or region-context Delete Object Instance
service. Earlier Umbra roadmap wording referred to "regional/directed deletion
forms" as if they were future public 1516.1-2025 APIs.

**Umbra impact:** that wording is removed from the timestamped deletion and
Request Retraction contracts, plans, and design notes. Actual remaining
deletion work is limited to lifecycle and delivery conditions—alternate time
advance modes, remaining resignation and ownership states, save/restore,
transport, and evidence promotion—not invented directed or regional public
delete services.

**Possible Lab refinement:** none required. A future API inventory display
could make overload families more visually explicit, but the current immutable
surface and the imported official header already provide sufficient evidence.

### RL-027 — The time-constrained Initiate Federate Save condition spans two standalone candidates

**Status:** verified source-fragmentation edge; possible export refinement,
not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the semantic requirements export
splits the operative time-constrained save condition across
`requirement-candidate-content-clauses-04-federation-management-page-067-l37-8`
(`"time-constrained, it shall expect ... only when it is in"`) and
`requirement-candidate-content-clauses-04-federation-management-page-067-l40-9`
(`"the Time Advancing state"`). Each immutable source span is accurate, but
neither candidate is independently a complete normative condition.

**Umbra impact:** the bounded untimed and timestamped time-constrained
save-admission contracts map both IDs to the same coordinator/dispatcher
behavior. The timestamped TAR Catch2 cases now prove direct initiation after
the applicable TSO boundary and before the recipient's grant, including a
three-member admission sequence; a TARA case covers the available-mode
exclusive boundary; and a next-message case covers NMR's inclusive and NMRA's
exclusive boundaries. A three-member TARA/NMRA case now proves both strict
ordinary forms are ready before non-time-constrained notification. Dedicated
FQR cases now prove direct initiation after queued TSO and before Flush Queue
Grant when the calculated actual grant is strictly beyond the save time,
including mixed FQR/TAR member readiness. A six-member case now combines TAR,
NMR, TARA, NMRA, and FQR before the non-time-constrained notification. The
cross-member TSO case additionally proves that each constrained recipient's
payload delivery remains before its own direct initiation; an in-transit case
now requests the save during the recipient's TSO callback and proves direct
initiation waits for callback completion. The source-fragment
issue remains: the paired records, rather than either one in isolation,
express the complete Time Advancing condition.

**Possible Lab refinement:** retain the immutable source-fragment IDs but add a
stable continuation/compound relationship for statements split across adjacent
line spans. That would let a consumer select the complete temporal condition
without manually discovering and pairing both records.

### RL-028 — Directed-subscription delivery predicates have no granular candidate records

**Status:** verified export-shape limitation; possible refinement, not a
standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the rendered semantic source for
1516.1-2025 Declaration Management page 101 explicitly states that a missing
or by-ownership selector delivers a directed interaction only when the joined
federate owns at least one target-object attribute, while a universal selector
delivers for every target object known to that federate. The candidate export
does not produce a requirement record for either delivery predicate. It emits
the generic selector lead-in
`requirement-candidate-content-clauses-05-declaration-management-page-101-l104-29`,
then independent fragments for the supplied-class effect (`l116-33`), empty-set
preservation (`l128-37`), and additive subscription wording (`l134-39`).

**Umbra impact:** the directed-interaction requirements contract uses the
stable generic selector ID alongside direct source-symbol traceability for the
bounded ownership/universal implementation, and maps the two independently
exported invocation/empty-set fragments where they apply. Its Catch2 case
proves the default non-owner suppression, universal delivery, empty-set
preservation, and resubscription mode change. This is source/test traceability
only: it is not a catalog entry, validation result, or conformance claim.

**Possible Lab refinement:** preserve immutable fragment IDs, but emit either
a canonical compound requirement for the two delivery predicates or an
explicit relationship from the generic selector record to its conditional
ownership and universal effects. Consumers could then select the complete
normative rule without reconstructing page prose manually.

### RL-029 — Passive-subscription delivery semantics are source-visible but have no candidate record

**Status:** verified export-shape limitation; possible refinement, not a
standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the rendered semantic source for
1516.1-2025 page 30 defines passive subscription for object classes,
attributes, and interactions, and explicitly says that it is not used by the
RTI to arrange data delivery or the corresponding publisher advisories. The
semantic export has no `requirement-*page-030*` candidate record for that
definition. The nearby immutable service candidates record that an ordinary
attribute subscription takes the supplied active/passive mode
(`requirement-candidate-content-clauses-05-declaration-management-page-096-l117-36`)
and that a regional `(object class, attribute, region)` triple takes or replaces
its supplied mode
(`requirement-candidate-content-clauses-09-data-distribution-management-page-234-l14-3`
and
`requirement-candidate-content-clauses-09-data-distribution-management-page-234-l26-7`),
but none expresses the delivery suppression itself.

**Umbra impact:** ordinary and regional interaction and object-attribute
routing retain the immutable mode-state IDs, while their active-only delivery,
discovery, scope, and relevance predicates are documented as direct
source-level traceability to the page-30 definition. Catch2 regressions prove
passive suppression and active re-subscription restoration. These are bounded
development-profile tests, not conformance evidence.

**Possible Lab refinement:** retain all existing immutable candidates and emit
a stable definition-level requirement record (or a semantic relationship from
each active/passive service candidate) for the passive-delivery rule, including
its object-attribute and interaction applicability. That would let downstream
contracts link the predicate without reconstructing glossary prose.

### RL-030 — Allow Relaxed DDM delivery policy has no immutable candidate record

**Status:** verified export-shape limitation; possible refinement, not a
standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the 1516.1-2025 requirements
export exposes only
`requirement-candidate-content-clauses-10-support-services-page-282-l152-47`
for Allow Relaxed DDM. That candidate is the §10.55.1 getter requirement. The
semantic source content separately records the §9.1.4 strict/implementation-
specific overlap rule in
`content-element-content-15161-page-221-paragraph-001`, the §9.1.8 disabled/
enabled switch behavior in
`content-element-content-15161-page-225-paragraph-001` and
`content-element-content-15161-page-226-paragraph-001`, and the Annex E
explanation in `content-element-content-15161-page-431-paragraph-001`. None
is emitted as an immutable `requirement-*` record.

**Umbra impact:** the existing support-switch contract continues to map the
getter to its exact §10.55.1 candidate. Umbra does not misrepresent that
candidate as a routing requirement. Instead, the central committed-region
predicate implements a narrow documented exact-boundary policy and the Catch2
case proves enabled, disabled, gapped, and strict overlap behavior. The direct
source relationship and scope are recorded in
`docs/RELAXED-DDM-POLICY.md` and the Catch2 plan. This remains
development-profile traceability, not a validation or conformance claim.

**Possible Lab refinement:** retain the getter candidate and add stable
candidate records for §9.1.4's implementation-specific overlap condition and
§9.1.8's disabled strictness, enabled-superset invariant, and filtering
effects. Consumers could then link their explicit implementation policy to the
actual delivery semantics without borrowing the unrelated getter requirement.

### RL-031 — Set Service Reporting candidates are assigned to the following service

**Status:** verified export attribution mismatch; possible extraction
refinement, not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the source content element
`content-element-content-15161-page-277-paragraph-001` explicitly labels the
first half of the page as §10.47, *Set Service Reporting Switch*. Its three
candidate records—
`requirement-candidate-content-clauses-10-support-services-page-277-l28-6`,
`...-l31-7`, and `...-l37-9`—are nevertheless exported with
`clause_id: "clause-10.48.1"`, the following *Get Exception Reporting Switch*
section. Their titles also point to `annexes-page-439.tex`, rather than the
actual page-277 source.

**Umbra impact:** Umbra does not attach the misclassified §10.47 candidate to
the MOM service-reporting interlock contract. It instead uses the correctly
owned §11.5.1 candidate
`requirement-candidate-content-clauses-11-management-object-model-page-292-l57-18`
for the two-way switch/subscription rule, plus the exact §5.10.2 and §9.10.3
subscription candidates. The direct Set Service Reporting C++ API contract is
still checked against the official header and a focused Catch2 selector.

**Possible Lab refinement:** preserve the immutable candidate IDs but assign
the page-277 lines before the §10.48 heading to `clause-10.47` (or the
appropriate subclause when determinable), and derive the display title from
their actual source span. That would make service-level traceability possible
without a consumer silently accepting the wrong owner.

### RL-032 — Table 20 MOM switch semantics are not exported as individual candidates

**Status:** verified export-shape limitation; possible extraction refinement,
not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the source content elements
`content-element-content-15161-page-343-paragraph-001` and
`content-element-content-15161-page-344-paragraph-001` contain the Table 20
semantics for the nine predefined
`HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches` parameters. Both exported
elements have `record_ids: []`, so no immutable requirement candidates exist
for the individual parameter effects. The generic §11.4.1 records cover
receiving/processing the MOM interaction, promoting compatible extension
subclasses, and ignoring additional parameters. The §11.4.2 record
`requirement-candidate-content-clauses-11-management-object-model-page-291-l146-48`
is also exported with the truncated statement `shall %`, although the rendered
source clearly requires at least one `HLAsetSwitches` parameter.

**Impact on Umbra:** the bounded control-path contract links the generic
§11.4.1/§11.4.2 candidates and exact §11.5.1 interlock candidate, while its
per-parameter semantics retain direct source and 2025 MIM references. Umbra
does not claim candidate-level traceability for each Table 20 parameter until
the Lab exports them.

**Possible Lab refinement:** preserve existing IDs, emit stable per-row Table
20 candidates keyed by table, interaction class, and parameter, and reconstruct
continuation-fragment candidates such as the at-least-one requirement before
publishing. This would let consumers trace individual MOM switch effects
without deriving requirements from raw table text.

### RL-033 — Normalize-service opening candidates drift to following subclauses

**Status:** verified export attribution mismatch; possible extraction
refinement, not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the reconstructed source blocks
explicitly label the five service openings as §§10.29 through 10.33. Their
opening candidates are nevertheless exported with these clause IDs:

- `requirement-candidate-content-clauses-10-support-services-page-264-l133-41`
  (`Normalize Service Group`) → `clause-10.29.5`;
- `...page-265-l45-11` (`Normalize Federate Handle`) → `clause-10.31.2`;
- `...page-265-l150-46` (`Normalize Object Class Handle`) →
  `clause-10.31.2`;
- `...page-266-l95-28` (`Normalize Interaction Class Handle`) →
  `clause-10.32.5`; and
- `...page-267-l39-9` (`Normalize Object Instance Handle`) →
  `clause-10.34.2`.

The source content elements show the actual headings and opening behavior,
while the exported records' titles also resolve to `annexes-page-439.tex`.
The object-class candidate happens to have a matching returned-argument
subclause, but the other four candidate clause IDs name a later service's
subclause or an exception subsection rather than the opening service.

**Umbra impact:**
`compliance/handle-normalization-requirements-contract.json` retains the
immutable candidate IDs and their currently exported clause IDs so the
Requirements Lab checker can detect future drift. Umbra's API contract and
implementation plan use the authoritative §§10.29--10.33 service identity;
the mismatched candidate fields are not treated as a standards classification
or conformance result.

**Possible Lab refinement:** preserve the immutable IDs, but assign source
spans before a service's first subsection to the enclosing service heading and
derive the display title from that heading. A source-span relation linking an
opening behavior candidate to its returned/precondition/exception records
would make service-level traceability usable without consumers having to
preserve known-wrong clause IDs for checker compatibility.

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
