# HLA Requirements Lab observations

This is Umbra's decision log for observations about the adjacent HLA
Requirements Lab corpus and its exported `CorpusBundle`. It deliberately
separates verified data facts from proposed Lab refinements. An observation is
not a claim that the Lab, IEEE source, or Umbra is non-conformant.

## Pinned input

- Lab repository: `../Document-Recreation`
- Reviewed revision: `4f012fb1c21367cfde67aab8498ae00e2a64c615`
- Umbra lock: `compliance/requirements-lab.lock.json`
- Last reviewed: 2026-08-21

Umbra consumes only the Lab's exported portable JSON bundle. The generated
bundle under `.compliance/` is intentionally ignored, so each observation
below names the pinned revision and durable source identifiers rather than
depending on an uncommitted export artifact.

## How future consumers should interpret the Lab

The Requirements Lab is a requirements-corpus and traceability layer. It is
good at answering:

- what normative or source-derived statement was found;
- where that statement came from; and
- which API surface or implementation relationship the Lab can identify.

It is not, by itself, an exhaustive behavior specification or an edge-case
test suite. A complete test obligation can come from the interaction of
several clauses, an API's declared exception behavior, callback timing, a
multi-federate state transition, or an invalid-input path. Those obligations
must be derived into explicit scenarios and tested separately. A missing
candidate in the export must not be silently replaced with an invented Lab
ID, and a passing local test must not be described as verified conformance
until the sidecar evidence has received protected review.

For each new service slice, consumers should keep these layers distinct:

1. **Requirement traceability:** immutable Lab candidate IDs, source spans,
   clause ownership, and any Lab-provided API mapping.
2. **Derived behavior coverage:** positive, negative, boundary, callback-order,
   race, and multi-federate scenarios derived from the requirements and API.
3. **Evidence status:** implementation and test results, raw sidecar output,
   protected review, and final verification.

This separation lets a future Lab revision improve extraction or mappings
without turning a local interpretation into a false normative requirement.

## Proposed improvements for the Requirements Lab

The following are durable improvement requests, not findings of IEEE or
Umbra non-conformance. They are ordered roughly by their value to future
consumers:

1. **Export granular behavioral candidates.** Preserve the existing immutable
   IDs, but add candidates for explicit preconditions, postconditions,
   exceptions, callback timing, continuation sentences, table rows, and
   parameter-level semantics. RL-011, RL-012, RL-014, and RL-033 are concrete
   examples where a broad or missing candidate forces a consumer to retain
   direct source/API traceability.
2. **Make provenance internally consistent.** Every candidate should carry a
   source path, source line span, source-content page, rendered page when
   available, enclosing heading, and semantic owner. Display titles should be
   generated from those same fields so a title cannot point to a different
   source file than the inventory.
3. **Separate relationship kinds.** Distinguish API mappings from policy or
   state-machine relationships, schema constraints, exception relationships,
   and derived test obligations. A `no-api` source requirement can still be
   important without being forced into an API mapping.
4. **Optionally export derived scenario metadata.** A separately labelled,
   non-normative scenario layer could identify actors, pre-state, trigger,
   post-state, observable callback/result, ordering constraints, and negative
   or boundary categories. This would help consumers cover edge cases without
   implying that the Lab has rewritten the standard's prose.
5. **Add extraction quality gates.** Export should flag truncated statements,
   joined words, empty `record_ids` where a table or continuation is expected,
   source spans assigned to a later heading, and source/page convention
   mismatches. The warnings should be visible in the bundle and checker
   output, not discovered only by downstream users.
6. **Publish an edition-labelled resource inventory.** Identify which source,
   schema, API, and example resources are present for each HLA edition, and
   state explicitly when a companion artifact is not a substitute for the
   current source-licensed resource set.

When implementing one of these improvements, preserve existing IDs and make
the export change reviewable against the pinned revision. Consumers should be
able to update contracts deliberately rather than having an apparently small
Lab regeneration silently change clause ownership or evidence scope.

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

**2026-08-21 follow-up:** while composing the alternate FQR/TARA/NMRA regional
object-update retraction case, an initial timestamp equal to the producer's
requested time plus actual lookahead was correctly rejected as no longer
retractable. The Lab did not cause the behavior or expose a new extraction
failure; the scenario needed to compose the strict inequality from the
fragmented §8.22.3 anchors and now records that setup explicitly. This is a
repeatable consumer workflow rough edge, not a Requirements Lab defect or a
conformance finding.

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
Requirements-Lab normative candidate (`CancelThenDeleteThenDivest`) for an
omitted setting, while retaining an explicitly present schema-valid `NoAction`
value as that distinct configured action. This is recorded as an intentional
standards-traceability choice, not as conformance evidence.

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

### RL-034 — Table 5 service-report record structures have no immutable candidates

**Status:** verified export-shape limitation; possible extraction refinement,
not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the source blocks for pages
300--302 expose Table 5's `ServiceReportInitialRecord` and
`ServiceReportRecord` type/value structures, but their content elements carry
`record_ids: []`. The prose candidate
`requirement-candidate-content-clauses-11-management-object-model-page-294-l26-8`
does establish that an initial record precedes service-report records, yet it
does not identify the record fields or JSON-like layout.

**Umbra impact:** the MOM traceability contract can cite the §11.5.1 routing
candidates, while the Table 5 formatter retains direct source and MIM
references. Umbra does not represent its record-layout vectors as
candidate-level Requirements Lab evidence.

**Possible Lab refinement:** emit stable table-row or table-structure
candidates for `ServiceReportInitialRecord`, `ServiceReportRecord`, and their
named fields. Preserve the prose candidate separately so consumers can trace
record ordering without conflating it with format structure.

### RL-035 — State-machine records are not yet executable behavioral requirements

**Status:** verified semantic-model capability boundary; possible Lab model
refinement, not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, a state-machine transition records
its source state, event, target state, and optional guard, action, precondition,
postcondition, clause, requirement, and implementation links. The generic
state-machine schema and semantic model represent the guard, action,
precondition, and postcondition as descriptive text. The generic state-trace
replayer checks the supplied trace's machine, source state, event, transition,
and target state; it does not evaluate those text fields, mutate a typed state
store, or prove an invariant.

**Umbra impact:** a modeled transition can establish that a path is recorded,
but it cannot by itself generate the true and false guard cases, validate the
state effects, or identify every invalid-input and callback-boundary path.
Umbra therefore builds private state holders for lifecycle, declarations,
ownership, time, delivery, and object knowledge, and uses Catch2 scenarios to
finish the behavioral interpretation. Those tests currently perform two jobs:
they validate the implementation and they expose obligations that the Lab
model did not make explicit. This is why important edge cases can appear first
in testing even when the source prose or diagram was already present.

**Possible Lab refinement:** add a separately labelled executable-obligation
layer. It should provide typed state variables, predicates for guards,
transition effects, explicit error/rejection branches, callback and delivery
obligations, temporal/order constraints, and invariants. Each obligation should
retain source evidence and an interpretation status so a derived model is not
silently presented as normative source text. The Lab should be able to generate
positive, negative, boundary, cancellation, and callback-race trace seeds from
that layer, and mark unresolved interpretation instead of silently omitting a
case.

### RL-036 — Cross-machine edge cases have no first-class composition model

**Status:** verified model-composition boundary; possible Lab model refinement,
not a standards defect.

The pinned semantic model contains individual state machines and state traces;
each trace targets one `machine_id`. The generic state-machine record does not
currently declare shared state variables, synchronization points, event
interleavings, or a product/composition relationship between machines.

**Umbra impact:** many HLA edge cases are not properties of one machine. They
combine lifecycle membership, publication or subscription, known-instance
state, ownership, pending callbacks, time state, resignation, and callback
delivery boundaries. A single ownership or declaration diagram cannot enumerate
those combinations. Umbra has to discover and encode the combinations in
private registry state and multi-federate tests, which makes coverage dependent
on implementation work rather than on a complete requirements-derived scenario
set.

**Possible Lab refinement:** add an explicit composition layer that names
shared variables, event ownership, enabled-state conditions, synchronization
boundaries, callback queue effects, and permitted interleavings. The Lab need
not generate the full Cartesian product; it should support reviewed pairwise or
risk-based scenario generation and record why a combination is covered,
deferred, impossible, or intentionally out of scope.

### RL-037 — Transition coverage does not equal behavioral-obligation coverage

**Status:** verified coverage-semantics boundary; possible Lab refinement, not a
standards defect.

The current readiness and transition-coverage surfaces can require that each
modeled transition has a planned verification slot and implementation mapping.
That is valuable structural coverage, but a transition-level row does not
necessarily require coverage of every guard outcome, declared exception,
repeated invocation, cancellation race, callback recheck, teardown path, or
state invariant associated with the transition. A transition may therefore be
marked mapped or planned while important branches remain unmodeled or
untested.

**Umbra impact:** the presence of a transition ID in a contract or worklist is
not evidence that all of its behavioral obligations are covered. Umbra's local
contracts consequently add explicit Catch2 selectors and scope notes, but the
edge-case matrix is still assembled manually and remains separate from the
Lab's transition coverage result.

**Possible Lab refinement:** expand transition coverage into a typed obligation
matrix. At minimum, each transition should identify its success path, each
guard-failure path, each declared exception path, observable state changes,
callback ordering, cancellation or teardown behavior, and any required
cross-machine scenario. Every row should receive a trace/test disposition such
as verified, implemented-but-unreviewed, planned, blocked-by-interpretation,
or explicitly out of scope. This would make a green structural coverage check
meaningful without turning it into an unsupported conformance claim.

### RL-038 — Readiness and compliance are multidimensional, not one status

**Status:** verified workflow boundary; possible Lab reporting refinement, not a
standards defect.

The Lab and Umbra already distinguish several useful states, including source
mapping, API mapping, planned implementation, raw test evidence, protected
review, and verification. In practice, however, a consumer can still see words
such as `mapped`, `implemented`, `passed`, or `verified` without immediately
knowing which layer they describe. Structural readiness of a corpus is not
implementation readiness, and a passing test is not reviewed conformance
evidence.

**Umbra impact:** contracts must repeat scope notes to prevent a local test or
API match from being mistaken for complete HLA compliance. This makes the
workflow harder to understand and leaves room for future consumers to promote
an intermediate result too far.

**Possible Lab refinement:** expose independent, machine-readable status axes
for source coverage, semantic-model coverage, behavioral-obligation coverage,
implementation, raw evidence, protected review, package support, and
conformance. An aggregate status should be derived from those axes and should
never collapse `implemented`, `reviewed`, and `conformant` into one label.

### RL-039 — Omitted behavior needs an explicit disposition

**Status:** verified coverage-reporting boundary; possible Lab reporting
refinement, not a standards defect.

When a requirement, transition branch, cross-machine combination, or edge case
does not appear in a generated test or trace set, absence alone does not explain
why. It may be not yet modeled, not applicable, impossible under the standard,
deferred pending interpretation, intentionally out of scope, or simply missed
by extraction. The current source, transition, and test records do not provide
one uniform disposition vocabulary for all of those cases.

**Umbra impact:** downstream consumers have to infer whether an uncovered case
is unfinished work or a deliberate boundary by reading contract notes and
implementation prose. That is exactly the kind of rediscovery the Lab should
prevent.

**Possible Lab refinement:** require an explicit disposition for every omitted
behavioral obligation, with an owner, rationale, source references, and a
follow-up condition where applicable. Reports should make “not modeled,”
“interpretation unresolved,” “covered elsewhere,” “out of scope,” and “missed
coverage” visibly different states.

### RL-040 — The Lab should generate the first test matrix before implementation

**Status:** proposed workflow improvement based on the preceding verified
boundaries; not a standards or implementation-conformance claim.

For a new service family, the desired order should be:

1. extract and review the source requirements;
2. build or correct the state and composition model;
3. generate the transition, branch, exception, callback, and edge-case matrix;
4. assign every row a trace/test disposition; and
5. implement the service and attach evidence to those pre-existing rows.

Umbra currently often reaches the third step while implementing a vertical
slice, which means Catch2 tests help discover the matrix instead of merely
executing it. That is understandable during bootstrap, but it should not be
the mature Lab workflow.

**Possible Lab refinement:** prove this workflow on one bounded pilot, such as
federate lifetime or attribute ownership. The pilot should generate a review
packet containing the source links, typed state transitions, branch and
exception obligations, composed edge cases, trace seeds, and explicit omissions
before any implementation is called complete. The pilot can then become the
template for other service families without requiring the Lab to pretend that
every derived scenario is direct normative prose.

### RL-041 — Service-report file sources need cross-artifact reconciliation metadata

**Status:** verified source/export discrepancy; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the reconstructed Table 8 source
block `content-block-content-15161-page-311.tex` identifies
`HLAreportServiceFile` as `Static`. The vendored official 2025 MIM resource
`third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml`, however,
identifies the same attribute as `Conditional`, with the update condition
“The first time that both HLAserviceReporting and HLAsendServiceReportsToFile
become true.” The §11.5.2 candidate records used for file logging also have
titles pointing to `content/clauses/annexes-page-439.tex` rather than their
actual page-293/294 semantic source blocks; for example,
`requirement-candidate-content-clauses-11-management-object-model-page-293-l128-42`.
The same title/source split is present in the independent C++ handle-helper
candidate `requirement-candidate-content-clauses-11-management-object-model-page-382-l24-7`:
its title names `content/clauses/annexes-page-439.tex:24`, while its structured
`source.path` correctly identifies
`content/clauses/11-management-object-model-page-382.tex`.

**Umbra impact:** Umbra chooses a report-file path and writes the initial
record at join, retaining that identity until resignation. Its private
RTI-owned joined-federate snapshot now encodes that exact path as an initial
value under the IEEE 1516.1-2025 Table 8 `Static` decision, and the public
MOM discovery/reflection lane exposes it for ordinary subscribers and
matching immutable-point regional subscribers. The contrary MIM field remains
recorded rather than silently discarded. The filesystem and snapshot
lifecycle tests are private source-level traceability only; the public Catch2
lane is still not promoted to Lab validation or conformance results.

The handle-encoding contract likewise retains the immutable candidate ID and
the structured source path rather than relying on the stale generated title.

**Possible Lab refinement:** associate each extracted table cell and
requirement candidate with its exact source block and upstream artifact, then
flag divergent facts for the same named MIM item. A report should distinguish
an extraction/source-provenance mismatch from a genuine standards
interpretation question, while retaining both observed values for review.

### RL-042 — Table 5 service-report log return representation needs a declared mapping

**Status:** verified table/log-format ambiguity; the interaction wire shape is
resolved by matching prose and MIM evidence. This is a possible Lab refinement,
not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, §11.5.1 on
`content-block-content-15161-page-292.tex` explicitly says that both
`HLAsuppliedArguments` and `HLAreturnedArgument` use the `HLAargument` fixed
record, and that a multiple-return service uses an appropriate composite
`HLAargumentType`. The vendored 2025 MIM and Table 20's parameter semantics
agree. That resolves the interaction wire representation.

The remaining issue is log formatting. Table 5's reconstructed
`ServiceReportRecord` on `content-block-content-15161-page-301.tex` declares
`HLAreturnedArgument:ReturnArgument` and depicts the field as an array whose
no-return example is `[null]`. `ReturnArgument` is not a separately emitted
Table 5 type. The same table's earlier `HLAreturnedArgument` entry on page 298
has a two-field record shape. Section 11.5.2.1 directs service-report log
records to Table 5 but does not state the mapping from that log-only notation
to the interaction's three-field `HLAargument` textual depiction. The Table 5
type row also spells `HLAservce`, whereas its example and the MIM use
`HLAservice`. The §11.5.1 implementation-dependent wording is specifically
limited to the textual depiction in the `HLAargumentName` field; it does not
declare `ReturnArgument` or provide a general file-record return mapping.

**Umbra impact:** the private payload encoder follows the matching §11.5.1/MIM
wire rule. Umbra deliberately has no generic `ServiceReportRecord` formatter:
the existing Table 5 primitive and initial-record formatters must not be
combined with the interaction `HLAargument` rendering to create a guessed
file record. Twenty-four bounded exceptions are now implemented: successful-void
records for seven Support Services (six Boolean setters—the four relevance/
scope advisory setters, `Set Convey Region Designator Sets Switch`, and `Set
Exception Reporting Switch`—plus `Set Automatic Resign Directive`) and the
five no-argument Time Management services `Disable Time Regulation`,
`Enable/Disable Asynchronous Delivery`, and `Enable/Disable Time
Constrained`, plus the one-argument `Enable Time Regulation`, `Modify
Lookahead`, `Time Advance Request`, `Time Advance Request Available`, `Next
Message Request`, `Next Message Request Available`, `Flush Queue Request`, and
`Retract`, use Table 5's
explicitly depicted `[null]` return form. The automatic-resign case
additionally uses the table's explicit `ResignAction` argument type 44 and
C++ enum spelling; the no-argument services use the explicitly required empty
supplied-argument list, while Enable Time Regulation uses its standard
`Lookahead` name with `LogicalTimeInterval` argument type 32 and the
table's quoted `interval.toString()` form. Modify Lookahead uses its
`Requested lookahead` name with the same type/form and records an accepted
lower request before the actual value changes. Time Advance Request, Time
Advance Request Available, Next Message Request, Next Message Request
Available, and Flush Queue Request use the `Logical time` name with
`LogicalTime` argument type 31 and the table's quoted `time.toString()` form
while their accepted requests still await their later grant. The two Next
Message Request reports and Flush Queue Request preserve their supplied
boundaries even when queued TSO input produces earlier grants; Flush Queue
Grant separately carries actual and optimistic times. The
`Retract` record uses the table's type-33 `MessageRetractionDesignator` text
form `MessageRetractionHandle<decimal-identity>` before its separately
callback-gated Request Retraction consequence. `Change Attribute Order Type`
adds type-37 `Object instance designator` text from quoted
`ObjectInstanceHandle::toString()`, type-1 `Set of attribute designators` as a
bracketed array of quoted `AttributeHandle::toString()` values, and type-38
`Order type` text from quoted `RECEIVE`/`TIMESTAMP` after a successful
owned-attribute invocation. `Change Default Attribute Order Type` adds type-36
`Object class designator` text from quoted `ObjectClassHandle::toString()`,
with the same type-1 attribute-set array and type-38 quoted
`RECEIVE`/`TIMESTAMP` form after a successful class-default invocation. `Change
Default Attribute Transportation Type` uses that same type-36/type-1 form with
type-59 `Transportation type` text from quoted
`TransportationTypeHandle::toString()` after an accepted prospective
class-default invocation. `Change Interaction Order Type`
adds type-27 `Interaction class designator` text from quoted
`InteractionClassHandle::toString()` and type-38 `Order type` text from quoted
`RECEIVE`/`TIMESTAMP` after a successful published-class invocation. The
`Request Interaction Transportation Type Change` record adds that same type-27
interaction-class text and type-59 `Transportation type` text from quoted
`TransportationTypeHandle::toString()` at accepted request time, before its
separately queued confirmation changes the preference. `Request Attribute
Transportation Type Change` adds type-37 `Object instance designator` text from
quoted `ObjectInstanceHandle::toString()`, type-1 `Set of attribute designators`
as a bracketed array of quoted `AttributeHandle::toString()` values, and type-59
`Transportation type` text from quoted `TransportationTypeHandle::toString()` at
accepted request time, before its separately queued confirmation changes the
preference. The
time-regulation and time-constrained enable records do not collapse
their distinct callback completions into successful requests. That does not
establish a general return mapping and does not enable composite-return,
non-void, or failed-service file records.

**Additional Lab observation (2026-08-19):** Table 5's extracted type-33 row
does explicitly depict `MessageRetractionDesignator` as
`MessageRetractionHandle<2345>` on page 302, while its type-1
`AttributeHandleSet` row uses Table 4's bracketed array grammar with quoted
`AttributeHandle` values, its type-27 `InteractionClassHandle` row calls for
quoted `handle.toString()` text, its type-36 `ObjectClassHandle` row and type-37
`ObjectInstanceHandle` row call for the same quoted form on page 299, its
type-38 `OrderType` row depicts quoted `RECEIVE`/`TIMESTAMP` values on pages
298--299, and its type-59
`TransportationTypeHandle` row on page 304 calls for quoted `handle.toString()`
text. Those content elements currently have no generated row-level
requirement-candidate identifiers. Umbra therefore anchors the bounded
`Retract`, `Change Attribute Order Type`, `Change Default Attribute Order Type`,
`Change Default Attribute Transportation Type`, `Change Interaction Order Type`,
`Request Interaction Transportation Type Change`, and `Request Attribute
Transportation Type Change` invocations to
available service candidates and records the Table 5 representations only as
private source-level traceability; they are not Lab validation records or
conformance claims.

**Additional Lab observation (2026-08-20):** the same rotated Table 5
continuation is the only pinned source location that exposes the `StringSet`
row: page 302's raw source (`content-block-content-15161-page-302.tex:101-117`)
defines its encoding as `Array<String>` and illustrates two quoted names. The
unmodified 2025 MIM independently assigns `StringSet` argument type 54, but
the Requirements Lab's structured Table 5 export has no row-level record for
this continuation. The source/API contracts therefore preserve both the raw
coordinates and the MIM value; they do not treat an absent structured row as
evidence that the multiple-name services lack a reportable supplied argument.
The composite callback value—names paired with reservation-success
indicators—remains deferred because neither the raw Table 5 log example nor a
stable row candidate supplies a file-record mapping for it.
The same row-level export gap affects the regional DDM pair shape: page 295's
raw Table 5 source (`content-block-content-15161-page-295.tex:84-147`) calls
the value `AttributeRegionAssociationList` and encodes it as
`Array<AttributeRegionAssociation>`, while the unmodified MIM exposes the
corresponding argument type as `AttributeSetRegionSetPairList` (type 4).
Umbra retains both names as an explicit Table 5/MIM alias rather than treating
the spelling difference as a new local type.

**Possible Lab refinement:** emit stable row- or cell-level requirement
candidates for Table 5 examples, including page/source coordinates and the
argument type/name/value triple. That would let type-specific file-report
formatters be traced directly instead of borrowing a neighboring service
candidate while preserving the existing RL-042 boundary around generic return
arguments.

**Published-correction check:** on 2026-08-19, the official active-standard
page for IEEE 1516.1-2025 listed only the downloads bundle under Additional
Resources and exposed no errata/corrigendum link. This is a time-bounded
publication-status observation, not evidence that no correction can ever be
issued; check the official page before implementing the blocked mapping.

**Possible Lab refinement:** emit table-structure facts with a relationship to
the corresponding MIM parameter/type and flag an undeclared table alias or
container-shape difference in log-only data. A review packet should preserve
the rendered example, the type row, §11.5.1 wire prose, §11.5.2.1 log rule,
and the MIM semantics together instead of requiring a consumer to infer a
mapping.

### RL-043 — RTI-created MOM-object producer designator lacks a joined source mapping

**Status:** verified cross-clause mapping gap; possible Lab refinement, not a
standards defect or conformance finding.

The pinned Lab independently exports the ordinary discovery rule as
`requirement-candidate-content-clauses-06-object-management-page-119-l103-29`:
the `producing joined federate` argument contains the designator of the joined
federate that registered the object. It also exports the MOM requirement as
`requirement-candidate-content-clauses-10-support-services-page-287-l102-33`:
the RTI publishes `HLAmanager.HLAfederate` and registers one object instance
for every joined federate. The corresponding source blocks are §6.9 and
§11.2, respectively. The Lab does not currently provide a relationship or
source note that resolves what discovery/reflect callback producer designator
is valid for an RTI-created MOM object, which is not registered by a joined
federate.

The broader source confirms that this is not merely a hypothetical edge case:
the §1 candidate
`requirement-candidate-content-clauses-01-overview-page-019-l62-7` expressly
uses MOM `HLAmanager.HLAfederate.HLAreport` interactions as examples of
RTI-invoked services that need not result from a joined-federate invocation.
Together with §11.1--§11.2, that establishes RTI origin, but it still does not
name a `FederateHandle` or a callback-specific exception to §6.9's producing
joined-federate rule. RTI origin must therefore not be converted into an
invented sentinel or a selected joined-federate identity.

A follow-on source/API pass establishes the same boundary for generic
RTI-originated MOM **interactions**, not only RTI-created MOM objects. The
pinned §6.13 candidate
`requirement-candidate-content-clauses-06-object-management-page-125-l127-38`
requires the `producing joined federate` argument of *Receive Interaction* to
contain the designator for the sending joined federate. In contrast, the §11.5
candidate
`requirement-candidate-content-clauses-11-management-object-model-page-292-l9-2`
requires `HLAreportServiceInvocation` to be generated by the RTI, including
when the RTI invokes a service at a particular joined federate. The official
C++ callback surfaces
`api.2025.cpp.federateambassador.receiveinteraction.74e918c917c1` and
`api.2025.cpp.federateambassador.receiveinteraction.06a1022b8731` each require
a non-optional `FederateHandle` `producingFederate` parameter. Annex C only
says that handles are normally generated by the RTI and that a default
constructor exists for convenience; it does not designate that invalid handle
as an RTI producer. No exported candidate, C++ mapping, MIM declaration, or
official header maps the report subject to the callback producer field or
authorizes an invalid handle for RTI origin.

A direct source pass over §11.1, §11.2, and §11.4 confirms rather than
resolves the tension: §11.1 says that MOM access/interchange uses predefined
HLA objects and interactions in the same way as participating federates;
§11.2 requires the RTI to publish and register the `HLAmanager.HLAfederate`
instances; and §11.4 directs the RTI to update those instances with their
private federate points. None supplies an exception or an RTI-to-joined-
federate mapping for the §6.9 callback argument. This must not be inferred
from the represented federate's `HLAfederateHandle`.

**Umbra impact:** Umbra will not copy an unjoined `FederateHandle(0)` numeric
sentinel into its public callback path. The first public RTI-owned
joined-federate MOM object-management slice now uses the dedicated ledger for
active ordinary and immutable-point-matching regional discovery, reliable initial
`HLAreportServiceFile` reflection, direct known-object requested-value
reflection, event-driven current-value projection for all nine predefined
conditional switch attributes, and resignation removal. Because the Lab still supplies no
callback-visible producer mapping, this bounded development-profile route uses
a default-invalid `FederateHandle{}` for its RTI-originated discovery,
reflection, and removal callbacks. That is an explicit local adapter policy,
not a standards interpretation or conformance evidence. The private snapshot
still retains complete MIM metadata, an immutable point, and all seven encoded
initial values; the seven-value initial public projection, immutable-point
regional discovery, event-driven switch projection, four bounded temporal-state
projections, and the save/restore-driven `HLAfederateState` enumeration are now
covered. The bounded `HLAsetTiming` deadline pump now also exercises the
catalog-declared periodic subset at an `HLA_EVOKED` callback boundary. Idle
`HLA_IMMEDIATE` background delivery, remaining periodic/other conditional
scheduling, optional/inherited non-initial attributes, and generic RTI-created
traffic remain open.
Implementation note: the Lab's callback-model distinction makes conditional
MOM timing observable. A queued reflection that recomputes its value only when
the callback runs can miss a short-lived event state when a later grant has
already committed; Umbra now snapshots the event-time encoded value while
still revalidating object existence and subscription eligibility at delivery.
The same save/restore case now exercises the MIM's save-state self-reflection
boundary: the event planner suppresses the save-state reflection at the
federate that is itself saving, while a direct known-object AVU remains
available. This is an implementation rule covered by native Catch2, not a
new Lab producer mapping.
This is an implementation hazard worth retaining in any future Lab test
generator for conditional attributes.

The bounded fault-only `HLAreportFederateLost` route makes one explicit local
adapter choice: its
normal non-timestamped `Receive Interaction` callback receives a
default-invalid `FederateHandle{}` to represent no joined-federate producer.
It is not a numeric sentinel, not the lost federate's handle, not a
standard-defined mapping, and not conformance evidence. RL-065 retains that
interaction-specific boundary; the private service-report routing plan itself
continues to model its origin as RTI-owned.

**Possible Lab refinement:** add a cross-clause MOM implementation note or
relationship that records the applicable producer-designator rule (including
any explicit RTI exception) for both RTI-registered object instances and
RTI-originated MOM interactions. The packet should preserve the ordinary
producer requirements, identify whether a further source controls their
interaction, and distinguish the represented/report-subject federate from a
callback producer rather than letting consumers invent a handle sentinel or
silently substitute either identity.

### RL-044 — Constructed-encoding candidates lose their precise subclause provenance

**Status:** verified export-provenance and source-candidate coverage boundary;
possible Lab refinement, not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the source block
`content-block-content-15162-page-084.tex` visibly heads subsection
`4.14.10.5 HLAvariableArray` and states that the leading
`number_of_elements` is an `HLAinteger32BE`. The corresponding detailed
candidate,
`requirement-candidate-sections-semantic-clause-4c-page-084-l95-11`, is
exported with `clause` and `clause_id` both reduced to `4` / `clause-4`.
Likewise, the leading-padding and inter-element-padding rules on page 085 are
exported as `requirement-candidate-sections-semantic-clause-4c-page-085-l51-1`
and `requirement-candidate-sections-semantic-clause-4c-page-085-l63-4` with
the same broad clause identifier. The source supplies Equation (6)'s boundary
definition—the maximum of the element and `HLAinteger32BE` boundaries—and the
general constructed-data source says padding bytes are zero, but neither detail
has a separate immutable candidate. The text is usable and independently
verified, but the precise subclause and every equation detail cannot be
recovered from the candidate metadata.

**Umbra impact:** Umbra records the immutable count, leading-padding, and
inter-element-padding IDs in its private basic-data-element and official
`HLAvariableArray` encoding contracts, and cites the rendered source's
`4.14.10.5` heading and Equation (6) in local design material. The tests
correct an earlier byte-count interpretation of `HLAunicodeString`, retain
element-count plus inter-element-padding vectors for `HLAargumentList` and the
private `HLAmoduleDesignatorList` encoder used by the unpublished
joined-federate MOM snapshot, and now exercise the public-header array class.
This is source/test traceability only, not a conformance finding.

**Possible Lab refinement:** preserve the nearest semantic heading as the
candidate clause identifier for constructed-encoding prose, or add a
`source_subclause` field separate from coarse section grouping and stable
paragraph/equation records for Equation (6) and the zero-padding statement.
That would let encoding contracts name `4.14.10.5` without inventing a new
requirement ID.

### RL-045 — Aggregate C++ Connect crosswalk leaves exact overloads unselected

**Status:** verified crosswalk-aggregation gap; possible export refinement,
not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the exported
`crosswalk.rti.service.connect.cpp` record has status `ambiguous`, lists all
four official C++ `RTIambassador::connect` overload IDs as candidates, and has
an empty `selected_api_surface_ids` array. In the same pinned export,
`api-requirement-bindings.json` independently contains four `matched`
`req-connect-service-establishes-connection` C++ bindings, one for each of
those exact overload IDs. The source facts therefore identify every overload,
but the aggregate implementation crosswalk cannot express that all four are
valid members of one overloaded service family.

**Umbra impact:** Umbra retains the aggregate crosswalk as unresolved and does
not use it for catalog or conformance claims. Its separate API-only contract
pins each exact declaration to the existing Catch2 overload scenario, while
the Catch2 plan records those individual API surfaces solely for declaration
traceability. This local mitigation does not relabel the Lab crosswalk as
selected or resolved.

**Possible Lab refinement:** allow a native implementation crosswalk to select
multiple overload surfaces when each has a matched requirement-to-API binding,
or export an explicit overloaded-service group that carries the four selected
IDs. Preserve the current candidate IDs and ambiguity history so consumers can
distinguish a deliberate multi-overload selection from a one-to-many mapping
uncertainty.

### RL-046 — Static update-condition prose conflicts with supplied 2025 examples

**Status:** verified cross-artifact tension; the Static/NA direction remains
deferred, while the independent Conditional/Periodic non-NA predicate is
bounded in Umbra. This is not a Requirements-Lab defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidate
`requirement-candidate-sections-semantic-clause-4b-page-049-l26-2` says that
the attribute-table Update Condition column shall contain `NA` when Update Type
is `Static` or `NA`. The vendored official 2025
`RestaurantFOMmodule-2025.xml` instead gives its `ChefName` attribute
`<updateType>Static</updateType>` and
`<updateCondition>On change</updateCondition>` (lines 92–93). The supplied
Restaurant SOM repeats the same pair, and the MIM has at least one `Static` /
`N/A` pair. The 2025 DIF XSD accepts `updateCondition` as a general string and
does not encode this cross-field predicate.

**Umbra impact:** Umbra does not introduce a generic Static/NA rejection rule
that would reject supplied official 2025 material. It does enforce a separate,
bounded direction from the same candidate: a supplied Update Condition must be
nonempty, non-NA text when a composed attribute supplies Conditional or
Periodic Update Type. An omitted condition remains a partial DIF row, and the
predicate does not parse periodic-rate grammar or initial-condition prose.
The source candidate remains usable traceability, but its Static/NA direction
is explicitly deferred until the table wording, XML spelling, and official
example set have a reviewed reconciliation. This is distinct from the
completed attribute/parameter data-type rule and does not weaken that rule.

**Possible Lab refinement:** associate the candidate with the corresponding
2025 example rows and schema field type in a non-normative reconciliation
note. A review packet should preserve the exact table prose, Restaurant FOM
and SOM rows, MIM variation, and XSD declaration so a consumer can make a
deliberate policy decision without silently treating an example conflict as an
implementation error.

### RL-047 — Fixed/variant record type-column candidates lose precise clause provenance

**Status:** verified export-provenance drift; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidates
`requirement-candidate-sections-semantic-clause-4c-page-076-l52-2` (fixed
record Field Type) and
`requirement-candidate-sections-semantic-clause-4c-page-077-l78-7` (variant
record Alternative Type) both export `clause` / `clause_id` as `4` /
`clause-4`. Their source statements are specific, and the Lab's canonical
source navigation identifies the surrounding headings as `4.14.7 Fixed record
data type table` (page 75) and `4.14.8 Variant record data type table` (page
76), respectively. The candidate provenance therefore loses the precise
subclause even though the source content retains it.

**Umbra impact:** Umbra may use the immutable candidate IDs for bounded private
source/test contracts, but must retain the exporter-provided `clause-4` in the
machine-checked contract rather than inventing `clause-4.14.7` or
`clause-4.14.8`. Local design text can cite the verified source headings as
context. This is traceability only, not a conformance result.

**Possible Lab refinement:** extend the clause-context reconstruction used for
the `4.14.10.5` cases in RL-044 to these table-column candidates, or export a
separate `source_subclause` field. Preserve the immutable candidate IDs and
their extraction history so consumers can distinguish an export provenance
fix from a change in normative source text.

### RL-048 — Dimension-table input candidates lose precise clause provenance

**Status:** verified export-provenance drift; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidates
`requirement-candidate-sections-semantic-clause-4b-page-056-l10-2` (Input data
type) and `requirement-candidate-sections-semantic-clause-4b-page-056-l14-3`
(Input data type description) both export `clause` / `clause_id` as `4` /
`clause-4`. The Lab's canonical source navigation identifies their surrounding
heading as `4.7.2 Table format` on page 55. The candidate statements therefore
remain precise, but their exported provenance loses the Dimension-table
subclause.

**Umbra impact:** Umbra binds the immutable Input data type candidate to a
bounded private category check, NA-exclusivity predicate, and separate
no-named-input description predicate, while retaining exporter-provided
`clause-4` in each machine-checked contract. The DIF `inputDataTypes` sequence
maps the latter directly: an empty sequence (or retained explicit `NA` marker)
requires non-`NA` description text, while an explicit `NA` marker cannot be
mixed with a named type. Umbra does not infer whether a suitable type exists,
whether the text is unambiguous, a cardinality or duplicate-name rule, or a
blanket description rule for named types. This is traceability only, not a
conformance result.

**Possible Lab refinement:** reconstruct the nearest table-format heading for
these candidates or add a `source_subclause` field while preserving candidate
IDs and extraction history. That would permit consumers to distinguish the
specific `4.7.2` context from generic Clause 4 content without changing the
normative text.

### RL-049 — Variant discriminant-enumerator semantic candidate loses precise clause provenance

**Status:** verified export-provenance drift; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidate
`requirement-candidate-sections-semantic-clause-4c-page-077-l70-5` exports
`clause` / `clause_id` as `4` / `clause-4`. Its source statement
contains the substantive variant-record Discriminant Enumerator rules:
membership in the named enumerated data type, bracketed range meaning, and
non-extendable `HLAother` semantics. The Lab's canonical source navigation
identifies the surrounding heading as `4.14.8 Variant record data type table`
on page 76. The semantic candidate is precise, but its exported provenance
loses that subclause.

**Umbra impact:** Umbra uses the exact verification-table candidates for the
completed lexical slice and binds the source candidate to bounded membership
and declaration-order range-semantics checks. Its machine-checked contracts
retain the exporter-provided `clause-4` rather than inventing
`clause-4.14.8`. This observation does not turn any private preflight check
into a conformance claim.

**Possible Lab refinement:** apply the same nearest table-format heading
reconstruction proposed in RL-047, or export a dedicated
`source_subclause` field for these variant-record semantic candidates while
preserving their immutable IDs and extraction history.

### RL-050 — Annex C.3 continuation loses its inherited subclause provenance

**Status:** verified export-provenance drift; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidate
`requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-110-l90-20`
correctly exports `clause-C.3`, while its sentence continuation,
`requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-111-l6-1`,
exports `clause` / `clause_id` as `7` / `clause-7`. The Lab's reconstructed
page-110 content identifies the surrounding heading as `C.3 Merging data
types`; page 111 begins with the unfinished statement before the next `C.4`
heading. The continuation therefore loses the inherited Annex C.3 context
across the physical page break.

**Umbra impact:** Umbra uses the exact continuation candidate for the bounded
direct/range-overlap preflight but retains the exporter-provided `clause-7` in
its machine-checked contract. Local design text may identify the visible
Annex C.3 context, but must not substitute an invented `clause-C.3` into the
immutable candidate trace. This is traceability only, not a conformance
result.

**Possible Lab refinement:** carry the most recent Annex subclause heading
across a page break when the following page begins with a sentence fragment,
or export a separate inherited-heading field. Preserve the existing candidate
IDs and extraction history so consumers can distinguish provenance repair from
a normative-source change.

### RL-051 — Attribute-`NA` available-dimensions condition has no per-attribute DIF representation

**Status:** verified source/schema representation gap; possible Lab refinement,
not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidate
`requirement-candidate-sections-semantic-clause-4b-page-048-l109-5` says that
when an attribute data type is `NA`, its update type, update condition, and
available dimensions shall also be `NA`. The official 2025 DIF schema's
`attributeType` sequence includes no `dimensions` member: it has `name`,
`dataType`, `updateType`, `updateCondition`, `valueRequired`, `ownership`,
`sharing`, `transportation`, `order`, and `semantics`. The DIF schema instead
places `dimensions` on `objectClassType`, where it is shared by the class and
cannot be mechanically attributed to one of several attributes.

**Umbra impact:** Umbra's bounded companion predicate enforces only the direct
representable fields. Its contract explicitly scopes out the available-
dimensions phrase rather than inventing a class-level prohibition that could
reject an unrelated attribute. A partial DIF row remains usable until a later
module completes it; no local result is a conformance claim.

**Possible Lab refinement:** add a source/schema representation crosswalk or
non-mappable-column annotation for table prose whose corresponding DIF field is
not scoped to the same table row. That would clarify implementation boundaries
without changing the immutable candidate or treating the gap as a standards
defect.

### RL-052 — Attribute Value Required candidate loses its Table 10 provenance

**Status:** verified export-provenance drift; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidate
`requirement-candidate-sections-semantic-clause-4b-page-049-l58-6` exports
`clause` / `clause_id` as `4` / `clause-4`. Its statement supplies the
Value Required condition for an attribute that is neither published nor
subscribed. The immediately preceding Attribute Table candidate,
`requirement-candidate-sections-semantic-clause-4b-page-048-l93-1`, retains
the `clause-4.5.2` Table 10 context. The precise table context is therefore
visible in the source sequence but lost from this candidate's exported
provenance.

**Umbra impact:** Umbra uses the immutable candidate for a bounded private
`sharing=Neither` / `valueRequired=false` predicate, while retaining the
exporter-provided `clause-4` in its machine-checked contract. Local design text
may identify the verified Table 10 context but must not substitute an invented
`clause-4.5.2` in the contract. This is traceability only, not a conformance
result.

**Possible Lab refinement:** carry the active table/subclause context across
the page break into the continuation candidates, or export a separate
`source_subclause` field while preserving immutable IDs and extraction history.
That would distinguish a provenance repair from a change to normative text.

### RL-053 — Predefined-representation and data-type table wire forms have no individual candidate records

**Status:** verified source-candidate coverage boundary; possible Lab
refinement, not a standards defect or conformance finding.

At pinned Requirements Lab revision
4f012fb1c21367cfde67aab8498ae00e2a64c615, the canonical 1516.2 source
reconstruction at content-block-content-15162-page-069.tex and
...page-070.tex preserves Table 29 cells for the 16-bit big-/little-endian
integer, unsigned-integer, IEEE-754 floating-point, octet-pair, and octet
basic representations. The
reconstruction visibly supplies their bit widths, interpretation, endian, and
encoding fields; Table 43 at ...page-080.tex supplies their octet boundary
value. Table 32 at ...page-071.tex maps `HLAASCIIchar`, `HLAbyte`, and
`HLAunicodeChar` to `HLAoctet`/`HLAoctetPairBE`, and Table 35 at
...page-074.tex maps `HLAASCIIstring` and `HLAunicodeString` to dynamic
`HLAvariableArray` forms of their corresponding character types.
requirements.json has no immutable helper candidate mentioning
HLAinteger16BE, HLAinteger16LE, HLAunsignedInteger16BE,
HLAunsignedInteger16LE, HLAoctetPairBE, HLAoctetPairLE, HLAinteger32LE, or
HLAunsignedInteger32LE, HLAinteger64BE, HLAinteger64LE,
HLAunsignedInteger64BE, HLAunsignedInteger64LE, HLAoctet, HLAASCIIchar,
HLAbyte, HLAASCIIstring, HLAopaqueData, HLAfloat32BE, HLAfloat32LE,
HLAfloat64BE, HLAfloat64LE, or HLAunicodeChar. The Lab does export broad FOM-table candidates
that require some of these predefined types to appear in MIM/FOM/SOM tables,
but those do not establish individual C++ helper wire behavior. The closest
machine-checkable C++ candidate is the general §12.12.4.2 statement that all
encoding helpers support encode/decode.

**Umbra impact:** Umbra binds its bounded C++ helper implementation to that
general 1516.1 candidate and records the independently inspected Table
29/32/35/43 details in the contract notes and Catch2 byte vectors. It does not
represent
the generic helper candidate as a substitute for a missing per-row wire-format
candidate, and it makes no interoperability or conformance claim.

**Possible Lab refinement:** export stable table/cell records or derived
requirements for normative predefined-representation and predefined-data-type
rows, retaining the source table, row, and column identifiers. This would let
consumers trace wire-format vectors directly without changing the existing
generic C++ helper candidate.

### RL-054 — Constructed-data encoder candidates lose exact subclause provenance and equation-level coverage

**Status:** verified export-provenance drift; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
4f012fb1c21367cfde67aab8498ae00e2a64c615, immutable candidates
`requirement-candidate-sections-semantic-clause-4c-page-080-l69-6`,
`requirement-candidate-sections-semantic-clause-4c-page-080-l73-7`, and
`requirement-candidate-sections-semantic-clause-4c-page-081-l36-2` for
HLAfixedRecord, plus
`requirement-candidate-sections-semantic-clause-4c-page-083-l92-16` and
`requirement-candidate-sections-semantic-clause-4c-page-084-l71-5` for
HLAfixedArray, plus
`requirement-candidate-sections-semantic-clause-4c-page-081-l68-9`,
`requirement-candidate-sections-semantic-clause-4c-page-081-l72-10`,
`requirement-candidate-sections-semantic-clause-4c-page-081-l76-11`, and
`requirement-candidate-sections-semantic-clause-4c-page-082-l47-3` for
HLAvariantRecord, plus
`requirement-candidate-sections-semantic-clause-4c-page-082-l75-10`,
`requirement-candidate-sections-semantic-clause-4c-page-082-l79-11`,
`requirement-candidate-sections-semantic-clause-4c-page-083-l40-3`,
`requirement-candidate-sections-semantic-clause-4c-page-083-l44-4`, and
`requirement-candidate-sections-semantic-clause-4c-page-083-l64-9` for
HLAextendableVariantRecord, all export `clause`/`clause_id` as
`4`/`clause-4`. The canonical reconstructed source at
content-block-content-15162-page-080.tex instead places the fixed-record
requirements under §4.14.10.1, pages 083 and 084 place the fixed-array
requirements under §4.14.10.4, pages 081 and 082 place the variant-record
requirements under §4.14.10.2, and pages 082 and 083 place the extendable
variant-record requirements under §4.14.10.3. The fixed-array Equation (5),
the variant-record Equation (2) and its `Size`/`V` definitions, the
extendable-variant Equations (3)/(4) and their `Size`/`V` definitions, and the
universal zero-padding statement are present in the reconstructed source but
have no individual immutable candidate. The closest exported alignment
candidate for the fixed array is the more general §4.14.10 record
`requirement-candidate-sections-semantic-clause-4c-page-079-l77-9`. The
extracted normative text is useful and the immutable candidate IDs remain
valid; the exported metadata is too broad to preserve every source-level
subclause and equation.

**Umbra impact:** the fixed-record, fixed-array, variant-record, and
extendable-variant-record contracts retain the immutable IDs and their exported
attribution. They record the directly inspected §4.14.10.1, §4.14.10.4,
§4.14.10.2, and §4.14.10.3 source locations and the relevant equation details
in notes, rather than inventing a narrower `clause_id` or individual equation
candidate that the Lab did not export. This is provenance hygiene, not a claim
that Umbra has completed conformance evidence.

**Possible Lab refinement:** preserve a `source_subclause` (or equivalent)
field for section-derived candidate records and export stable paragraph/equation
records for normative constructed-data layout prose, while retaining existing
immutable IDs and extraction history. That would distinguish §4.14.10.1,
§4.14.10.2, §4.14.10.3, and §4.14.10.4 from the broad Clause 4 container
without changing the existing candidate text.

### RL-055 — Logical-time candidates point to Annex/§12.4 metadata instead of the §12.3 source

**Status:** verified export-provenance drift; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidates
`requirement-candidate-content-clauses-11-management-object-model-page-354-l129-42`,
`requirement-candidate-content-clauses-11-management-object-model-page-354-l183-60`,
and
`requirement-candidate-content-clauses-11-management-object-model-page-354-l207-68`
all export `clause_id` as `clause-12.4` and titles rooted at
`content/clauses/annexes-page-439.tex`. Their canonical reconstructed source,
`content-block-content-15161-page-354.tex`, instead has the §12.3 heading
“Logical time, timestamps, and lookahead” and contains the compact opaque
encode/decode and factory initial/zero statements. The next heading on that
page begins §12.4 only after those statements. The immutable IDs and extracted
normative text remain useful, but the exported location and clause metadata do
not preserve their source context.

**Umbra impact:**
`logical-time-encoding-requirements-contract.json` retains those immutable IDs
and their exported `clause-12.4` values because the checker correctly detects
metadata drift. Its notes and design documentation identify the directly
inspected §12.3 source instead. This is source/test traceability only, not a
claim that the Lab has supplied complete validation or conformance evidence.

**Possible Lab refinement:** retain a source-page/subclause field for the
logical-time candidate records or correct the exported source path and clause
while preserving immutable IDs and extraction history. That would distinguish
the §12.3 abstract-interface behavior from the following §12.4 standardized
time-type table without rewriting historical candidate content.

### RL-056 — Authorization candidates retain Annex/§12.8 metadata across the §§12.5-12.6 source

**Status:** verified export-provenance drift; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, immutable candidate
`requirement-candidate-content-clauses-11-management-object-model-page-357-l100-32`
for the `HLAplainTextPassword` constructor and
`requirement-candidate-content-clauses-11-management-object-model-page-357-l67-21`
for the disabled-authorization credential result both export `clause_id` as
`clause-12.8` and titles rooted at
`content/clauses/annexes-page-439.tex`. The canonical reconstructed source at
`content-block-content-15161-page-356.tex` begins §12.5 “Authorization,” and
`content-block-content-15161-page-357.tex` places the selected rules under
§12.6 “Predefined authorization service” before the later Connect and
concurrency headings. The nearby §12.5 no-credentials candidates retain
`clause-12.5`, but also retain the same Annex-rooted title. The immutable IDs
and extracted normative text remain useful; their exported source location and
§12.8 attribution do not preserve the applicable source context.

**Umbra impact:**
`authorization-requirements-contract.json` preserves the immutable IDs and
exported clause values so the checker can detect a future correction. Its
notes and `AUTHORIZATION-DESIGN.md` cite the directly inspected §§12.5-12.6
source. This is source/test traceability only, not a claim that the Lab has
supplied complete authorization validation or conformance evidence.

**Possible Lab refinement:** preserve a source-page/subclause field for the
authorization candidates or correct the exported source path and active
subclause while retaining immutable IDs and extraction history. That would
separate §12.5's credential-envelope rule from §12.6's reference-authorizer
requirements and the later §12.8 concurrency material.

### RL-057 — Authorization-library prose and official C++ header use different factory-method spellings

**Status:** verified source/header discrepancy; possible Lab refinement, not a
binding defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the reconstructed authorization-
library prose on `content-block-content-15161-page-379.tex` describes the
static member function
`AuthorizerFactoryFactory::getAuthorizationFactory(std::wstring const &
authorizerName)`. Its related immutable candidates include
`requirement-candidate-content-clauses-11-management-object-model-page-379-l8-1`,
`requirement-candidate-content-clauses-11-management-object-model-page-379-l44-13`,
and
`requirement-candidate-content-clauses-11-management-object-model-page-379-l59-18`.
The unmodified official 2025 headers vendored for the C++ binding instead
declare `getAuthorizerFactory` in both
`RTI/auth/HLAauthorizerFactoryFactory.h` and
`RTI/libauth/AuthorizerFactoryFactory.h`. The latter spelling is also what the
headers' comments prescribe for forwarding.

**Umbra impact:**
`ieee1516_2025_authorizer.cpp` follows the actual official C++ declaration
`AuthorizerFactoryFactory::getAuthorizerFactory`, and the authorization
traceability contract cites the immutable forwarding candidate only as a
source/test link. Umbra does not silently add a second, non-header method or
treat the reconstructed prose spelling as a source of public API truth.

**Possible Lab refinement:** retain the reconstructed source text verbatim but
add an explicit header-API crosswalk or discrepancy annotation to these
authorization-library candidates. That would make the spelling conflict
visible to consumers without mutating immutable candidate content or implying
that the Lab itself owns the official C++ header correction.

### RL-058 — RID-backed authorization configuration has no public C++ binding surface

**Status:** verified source/binding boundary; possible Lab inventory refinement,
not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the canonical reconstructed
1516.1 source defines RID as RTI vendor-specific information needed to run an
RTI, supplied when required at RTI initialization
(`content-block-content-15161-page-019.tex:56-57` and
`content-block-content-15161-page-032.tex:65-66`).  The authorization source
then requires both the choice to perform authorization and the selected
authorization service to be configured through that RID
(`content-block-content-15161-page-357.tex:55-58`).  The vendored official
2025 C++ `RTIambassadorFactory` exposes only a no-argument
`createRTIambassador()`, while `RtiConfiguration` is a Connect-time value with
only configuration name, RTI address, and opaque additional-settings fields.
The `RTI/libauth/AuthorizerFactoryFactory.h` comment likewise says its factory
name comes from Runtime Initialization Data, but declares no RID reader or
configuration representation.  The Lab export has useful authorization
candidates, but no distinct C++ API surface or configuration model for that
vendor-specific initialization step.

**Umbra impact:** Umbra keeps `ReferenceAuthorizerConfiguration` private and
test-only.  It deliberately does not define a plaintext-password convention in
`RtiConfiguration::additionalSettings`, which is an opaque Connect-time field
and is captured by the existing service-report initial-record model.  The
embedded runtime remains an unconfigured authorization profile until a secure
RID-backed initialization design establishes secret handling, selection,
lifecycle, and failure behavior.  This observation supports a conservative
boundary; it neither says that RID must be public C++ API nor creates a local
RID syntax.

**Possible Lab refinement:** add an edition-labelled resource/API inventory
entry for vendor-specific RID configuration boundaries, explicitly marking the
absence of a standard C++ configuration representation where applicable.  That
would help consumers distinguish a deliberately implementation-dependent
initialization concern from an omitted ordinary C++ service mapping, without
inventing an API or changing immutable authorization candidates.

### RL-059 — The Lab lacks a general authority and reconciliation policy for conflicting artifacts

**Status:** verified cross-artifact reconciliation boundary; possible Lab
design refinement, not a standards defect or conformance finding.

The recent observations show a recurring pattern across different artifact
types: reconstructed source prose, semantic tables, the official MIM, DIF/XSD
schemas, supplied 2025 examples, and unmodified C++ headers can expose
different values, shapes, spellings, or levels of detail for what appears to
be the same fact. RL-041 records the Table 8 Static/Conditional difference;
RL-042 records the Table 5 log-only return-shape ambiguity; RL-046 records the
Static/`On change` example tension; and RL-057 records a reconstructed prose
method spelling that differs from the official header.

The Lab preserves these artifacts and provides useful source identifiers, but
the portable contract does not yet provide one general reconciliation record
that says which artifact is authoritative for which question, whether the
difference is extraction drift, a schema limitation, an example deviation, an
API/header discrepancy, or an unresolved standards interpretation.

**Umbra impact:** each affected contract must manually preserve the competing
values, select a deliberately bounded local interpretation or defer the rule,
and explain why the result is not conformance evidence. Without a common
reconciliation model, two future consumers could make different decisions
from the same Lab export while both believing they had followed the available
traceability data.

**Possible Lab refinement:** add an edition- and artifact-labelled conflict
record containing the shared semantic subject, each observed artifact/value,
field or source location, relationship type, proposed precedence, review
status, and permitted consumer action. At minimum, distinguish extraction
error, source contradiction, schema representation limit, official-example
deviation, header/API mismatch, and unresolved interpretation. The checker
should require an explicit reviewed selection or a blocked/deferred status; it
should never silently choose an artifact merely because it is easier to map.

### RL-060 — Connection-loss automatic-resign candidate is split before its semantic object

**Status:** verified source-extraction granularity limitation; possible Lab
candidate-reconstruction refinement, not a standards defect or conformance
finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the reconstructed 1516.1 source
at `content-block-content-15161-page-050.tex:36-41` says that, after reporting
lost federates, the RTI shall perform a resign on behalf of each lost federate
using that federate's Automatic Resign Directive. The export retains
`requirement-candidate-content-clauses-04-federation-management-page-050-l36-11`,
but its statement ends after “the RTI shall then”; the next continuation lines
that identify the resignation and directive do not form part of that candidate.
The candidate is therefore a useful source locator, but not a self-contained
machine-readable requirement for the automatic-resign behavior.

The same continuation contains two additional obligations that should not be
lost behind the resignation sentence: federation-wide synchronized operations
with the remaining joined federates shall continue as if the lost federates
had resigned, and MOM data and advisories shall be updated to reflect the new
state. The reviewed export does not expose either continuation as its own
immutable candidate-level obligation. This means the Lab loses not only the
Automatic Resign Directive's semantic object, but also two observable
postconditions of the same loss-of-connection event.

**Umbra impact:** the embedded connection-loss contract cites that immutable
candidate only as the closest source anchor, explicitly records the truncation
in its contract notes, and adds bounded Catch2 evidence for configured
`DELETE_OBJECTS` and `UNCONDITIONALLY_DIVEST_ATTRIBUTES` paths. The generic
Connection Lost requirements remain the selected lifecycle/callback evidence;
Umbra does not overstate the split candidate as a complete semantic assertion
or conformance proof.

**Possible Lab refinement:** coalesce adjacent source fragments when a
normative verb's semantic object continues across a layout line boundary, or
publish a stable parent/continuation relation so consumers can reconstruct the
complete sentence without guessing. Preserve the current immutable candidate
ID and text, but expose the continuation relationship and a reviewable
combined semantic statement.

### RL-061 — Joined-federate guard and cross-machine semantics are hidden in unlinked prose

**Status:** verified export-shape/state-machine semantic gap; possible Lab
refinement, not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, semantic content element
`content-element-content-15161-page-041-paragraph-001` points to
`content/clauses/04-federation-management-page-041.tex:1-68` and has
`record_ids: []`. The paragraph is more than figure context. It explains
that guards enable transitions when their assertions become true, that a
guard-only transition occurs immediately when its guard is true, and that the
two parallel joined-federate state machines impose cross-machine constraints.
It includes the `if and only if` relationship between `Active` and `Normal
Activity Permitted`, the corresponding `Normal Activity Not Permitted`
states, and the rule that an `Active` federate will not receive `Initiate
Federate Save` unless it is `Not Constrained` or `Time Advancing`.

These are operational guard semantics, automatic-transition behavior, and
cross-machine invariants. They are exactly the sort of state-machine logic
that a consumer needs in order to derive edge cases, but they are not emitted
as linked guard, invariant, or transition-obligation records.

**Umbra impact:** the implementation and tests must rediscover these rules
from raw source and stateful behavior. A state-machine transition can be
linked while its guard truth conditions, automatic execution rule, or
cross-machine invariant remains absent from the traceability surface. This
makes transition coverage look stronger than behavioral coverage and makes it
easy for a future consumer to mark a path compliant without testing the
conditions that actually govern whether the path is legal.

**Possible Lab refinement:** split this explanatory prose into stable typed
records for (1) guard semantics, (2) guard-only automatic transitions, (3)
each `if and only if` invariant, and (4) the cross-machine save-delivery
constraint. Link each record to the relevant state-machine and transition
IDs, preserve the paragraph as source context, and generate trace seeds for
true, false, and boundary cases. The Lab should make the invariant explicit
even when the source presents it in prose around a figure.

### RL-062 — Generic exception precedence is hidden in service-description prose

**Status:** verified export-shape gap; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, semantic content element
`content-element-content-15161-page-020-paragraph-001` points to
`content-block-content-15161-page-020.tex:15-23` and has `record_ids: []`.
The source defines the generic service-description fields and then states a
cross-cutting rule: exceptions are listed in increasing precedence, and when
more than one exception condition is met simultaneously, the RTI shall throw
the exception appearing later in that list. The reviewed requirements export
has no candidate for this page-level rule.

This is not merely documentation of the table layout. It determines the
observable result when multiple service preconditions fail at once, and it
applies across service families. A consumer that extracts individual
exception rows but misses this sentence cannot derive the pairwise collision
behavior or know which error wins.

**Umbra impact:** local service contracts can name the expected exceptions,
but they must separately preserve the source-level precedence rule and write
collision tests. A green test for each exception in isolation does not prove
the ordering rule.

**Possible Lab refinement:** emit a stable cross-cutting exception-precedence
record, link it to the affected service and exception records, and generate
pairwise precedence obligations where multiple exception conditions can be
true. If two conditions are mutually exclusive, that should be represented
explicitly rather than inferred from the absence of a test.

### RL-063 — Object-instance state completeness is an unlinked global invariant

**Status:** verified export-shape gap; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, semantic content element
`content-element-content-15161-page-017-paragraph-009` points to
`content-block-content-15161-page-017.tex:64-68` and has `record_ids: []`.
The paragraph states that, from the federation perspective, the set of all
attribute values for an object instance shall completely define that
instance's state. It also draws the boundary that a federate may keep
additional non-communicated state, but that state is outside the HLA FOM.
Neither the completeness invariant nor that boundary is represented as a
requirement candidate.

This sentence is a global state invariant, not background prose. It affects
how a consumer interprets partial updates, missing attributes, object
discovery, persistence, and the distinction between federated state and
application-private state.

**Umbra impact:** object-management tests naturally exercise individual
attribute delivery and ownership paths, but the Lab gives no explicit
obligation for the aggregate-state invariant or its out-of-band-state
boundary. A future implementation can appear to cover all update services
while still lacking a reviewable statement of what constitutes the complete
federated object state.

**Possible Lab refinement:** emit the sentence as a typed cross-cutting
object-state invariant, link it to object-instance and attribute records, and
derive checks for complete, partial, and privately retained state. Preserve
the source distinction so local application state is not accidentally treated
as an HLA-synchronized attribute.

### RL-064 — Definition records can be present but empty or behaviorally truncated

**Status:** verified semantic-publication gap; possible Lab validation and
extraction refinement, not a standards defect.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, several generated definition
surfaces are structurally present but semantically unusable. The bodies of
`definition-available-dimensions.tex`, `definition-inherited-dimension.tex`,
`definition-known-class.tex`, `definition-overlap.tex`,
`definition-published.tex`, `definition-subscribed.tex`,
`definition-used-for-sending.tex`,
`definition-used-for-subscription-of-a-class-attribute.tex`,
`definition-used-for-subscription-of-an-interaction-class.tex`, and
`definition-used-for-update.tex` contain only the generated `%` placeholder.
The corresponding source definitions are present in content elements such as
`content-element-content-15161-page-022-paragraph-012`,
its continuation `content-element-content-15161-page-022-paragraph-013`,
`content-element-content-15161-page-029-note-008`,
`content-element-content-15161-page-033-note-002`, and
`content-element-content-15161-page-036-note-002`.

The time-advancing definition shows a second form of the problem:
`definition-time-advancing-state.tex` stops after the list introduction,
while source element `content-element-content-15161-page-034-note-002`, from
`content-block-content-15161-page-034.tex:61-77`, continues with the allowed
services and the rule that time does not actually advance until a Time
Advance Grant or Flush Queue Grant is received. A definition link therefore
exists, but its published text does not carry the full semantic object.

These are not all requirement candidates by themselves. They are definitions
and predicates on which requirements depend. The problem is that a consumer
cannot tell whether an empty body means “definition intentionally omitted,”
“definition available elsewhere,” or “publication failed,” and a truncated
definition can silently remove the edge condition that gives it operational
meaning.

**Umbra impact:** service contracts must fall back to raw source and local
interpretation for basic terms such as overlap, known class, subscription,
and time advancement. That increases the chance that two consumers derive
different predicates from the same standard and that tests cover the service
name but not the definition's boundary conditions.

**Possible Lab refinement:** fail publication or mark the record incomplete
when a definition body is empty or ends at a continuation marker such as a
colon. Preserve list children, continuation spans, and source links in the
definition record; distinguish a complete definition, an intentionally
external definition, and an extraction failure. Requirement consumers should
be able to query which obligations depend on an incomplete definition.

### RL-065 — HLAreportFederateLost has no C++ producer-designator crosswalk

**Status:** verified API/source crosswalk gap; bounded local adapter choice;
possible Lab refinement, not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the page-50 candidates
`requirement-candidate-content-clauses-04-federation-management-page-050-l18-5`
and `requirement-candidate-content-clauses-04-federation-management-page-050-l21-6`
require delivery of `HLAreportFederateLost` to subscribed surviving federates
and a last known time position for a lost time-regulating federate. The MIM
identifies that interaction as RTI-originated. The exact normal C++ callback,
`api.2025.cpp.federateambassador.receiveinteraction.74e918c917c1`, nevertheless
requires a `FederateHandle const & producingFederate`. Neither the candidates,
the MIM, nor the exported callback surface provides a mapping from RTI origin
to that field. In particular, it does not authorize a numeric zero or the
lost federate's handle. The related full TSO statement is split across
page-50 candidates l24-7 and l27-8, so this observation does not promote that
unimplemented delivery condition.

**Umbra impact:** the bounded embedded loss route uses a default-invalid
`FederateHandle{}` only in the normal `HLAreportFederateLost` callback to
represent the absence of a joined-federate producer. The integration test
asserts that exact local behavior. It is deliberately not an inferred
standards value, does not make generic RTI-created MOM traffic public, and
does not establish conformance.

**Possible Lab refinement:** add an explicit source relation or
implementation-defined mapping between MOM RTI origin and the C++ callback's
`producingFederate` field. The record should distinguish a source requirement
from a binding-local choice, preserve the exact interaction and callback
surfaces, and say whether an invalid handle is permitted, required, or merely
implementation defined.

### RL-066 — Self-selecting service-report switches lack a report-order relation

**Status:** verified Requirements Lab semantic-model gap; local defer; possible
Lab refinement, not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the direct candidates
`requirement-candidate-content-clauses-10-support-services-page-277-l28-6`
and
`requirement-candidate-content-clauses-10-support-services-page-280-l29-6`
say that `Set Service Reporting Switch` and `Set Send Service Reports To File
Switch` set their respective values. The service-report candidate
`requirement-candidate-content-clauses-11-management-object-model-page-292-l9-2`
requires a report whenever a service is invoked and the Service Reporting
Switch is enabled; the destination candidate
`requirement-candidate-content-clauses-11-management-object-model-page-292-l18-5`
selects subscribers or the file from the Send Service Reports To File Switch.
The exported candidates, their links, and the API mappings do not encode
whether those report predicates observe the pre-invocation state, the service
postcondition state, or a separately defined report-generation event for an
invocation that changes either predicate itself.

**Umbra impact:** the twenty-nine successful-void wrappers currently admitted to the
filesystem path do not change either reporting predicate. Umbra deliberately
does not infer a record for `Set Service Reporting Switch` or `Set Send Service
Reports To File Switch`: doing so would choose whether disable/enable produces
a record and, for the latter, which sink receives it. This is a deferred
standards-facing behavior, not evidence that those invocations are
unreportable.

**Possible Lab refinement:** emit a semantic transition or explicit ordering
relation that connects the invocation, successful state transition, report
eligibility, and sink selection for self-selecting services. A generated test
matrix should enumerate the four old/new-value combinations for both setters,
identify the expected serial effect, and distinguish a normative result from
an implementation-defined policy if the source leaves that choice open.

### RL-067 — Table 5 wire-format rows have no row-level requirement candidates

**Status:** verified table-granularity export gap; possible Lab refinement, not
a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the extracted Table 5 material is
represented by page-sized paragraph elements whose `record_ids` are empty.
This affects, among others, the page-302 `MessageRetractionDesignator` row
(`MessageRetractionHandle<2345>`), the type-0 `AttributeHandle` and type-1
`AttributeHandleSet` array rows on page 295, the type-15 `FederateHandle` row
on page 296, the type-27 `InteractionClassHandle` and type-38 `OrderType` rows
on pages 298--299, the type-36 `ObjectClassHandle` and type-37
`ObjectInstanceHandle` rows on page 299, the page-304 type-59
`TransportationTypeHandle` row, and the primitive and enumeration
representations used by the new service-report formatters. The service-record
examples on pages 301--302 have the same
page-level shape; the individual argument name/type/value triples are not
immutable candidate records.

**Umbra impact:** the new `Retract`, `Change Attribute Order Type`, `Change
Default Attribute Order Type`, `Change Default Attribute Transportation Type`,
`Query Attribute Transportation Type`, `Query Attribute Ownership`,
`Query Interaction Transportation Type`,
`Change Interaction Order Type`,
`Request Interaction Transportation Type Change`, and `Request Attribute
Transportation Type Change` file-record cases,
along
with the LogicalTime, LogicalTimeInterval, ResignAction, and no-argument forms,
can be anchored to nearby service candidates and retain direct Table 5 source
coordinates. They cannot claim a Lab validation record for the exact
wire-format cell they exercise. A passing `requirements_lab.py check` therefore
validates the referenced neighboring IDs and bundle revision, not the
type-specific Table 5 row.

**Possible Lab refinement:** emit stable row- or cell-level records for Table 5
with page/source coordinates and a structured `(argument type, argument name,
argument value)` relationship. Keep the page-level example for context, but
make each type-specific formatter traceable without borrowing a service or
generic Table 5 candidate. This promotes the observation already noted under
RL-042 into a reusable export requirement for all Table 5 rows, not only type
33.

### RL-068 — Save/restore persistence and saved-message delivery are unlinked fragments

**Status:** verified cross-cutting source/model gap; possible Lab refinement,
not a standards defect or conformance finding.

The pinned export contains three relevant Clause 4 candidates:

- `requirement-candidate-content-clauses-04-federation-management-page-044-l36-11`
  says saved information shall be persistent and stored on disk or another
  persistent medium;
- `requirement-candidate-content-clauses-04-federation-management-page-044-l54-17`
  says undelivered messages are lost during restore and that related saved
  messages are delivered after restore; and
- `requirement-candidate-content-clauses-04-federation-management-page-044-l78-25`
  ties federate type to the ability to restore state saved by another federate
  of that type.

Each is marked `coverage_kind: cross-cutting` with an empty `transition_ids`
array. The reconstructed page text supplies the important continuations: the
persistent state survives destruction of the federation execution, TSO
messages resume at the federate's appropriate logical-time position, receive-
order messages resume as soon as possible, and the saved-message cutoff is
different from post-restore delivery. Those are distinct obligations, not one
generic “restore completed” event.

**Umbra impact:** the new restore contract deliberately uses a process-local
snapshot and tests pending TAR/TARA/NMR/NMRA/FQR work, live and terminal TSO
retraction state, and stale-work fencing. Those cases are mapped to the
generic Federation Restored candidate rather than to a Lab obligation for
persistent storage, message loss at the restore boundary, or TSO/receive-order
redelivery. This is correctly reported as development-profile traceability,
but it means the implementation had to derive a save/restore queue model and
its edge cases from source prose and testing.

**Possible Lab refinement:** retain the immutable candidates but add stable
continuation/compound relations and explicit save/restore state variables for
persistent image lifetime, pre-restore message loss, post-restore TSO/RO
delivery, and federate-type compatibility. Generate a scenario matrix that
distinguishes those obligations and links each to the relevant state-machine
transition and callback boundary.

### RL-069 — Synchronization-point edge cases are prose packets without transition mappings

**Status:** verified state-machine/requirement crosswalk gap; possible Lab
refinement, not a standards defect or conformance finding.

The new synchronization-point slice uses the candidates
`requirement-candidate-content-clauses-04-federation-management-page-060-l139-34`,
`...page-061-l11-1`, `...page-061-l158-40`,
`...page-062-l140-35`, `...page-063-l12-1`,
`...page-063-l117-26`, and
`...page-064-l132-35`. They contain substantial source prose, but the pinned
records are clause-scoped packets with empty `transition_ids`. The generated
state-machine artifacts do not give the requirements consumer a direct
transition-level relationship for the synchronization-set lifecycle.

That lifecycle contains several independently testable rules hidden inside
the packets: multiple labels may be pending; a designated member that resigns
before registration confirmation causes failure; late joiners can be added to
the synchronization set; resigning members are removed before completion; the
original tag is propagated; an omitted achievement-success indicator means
success; no further announcement is made after the set has achieved; and the
point and set disappear after Federation Synchronized. These are not merely
different assertions of “a synchronization point exists.”

**Umbra impact:** the new Catch2 cases exercise ordinary and
`HLA_IMMEDIATE` callback paths, subset registration, tag propagation,
late-join announcement, resignation/removal, failed achievement, and final
completion. The contracts can select the immutable source IDs, but the Lab
does not generate the positive/negative/late-join/resignation matrix or show
which transition carries each obligation. The tests therefore had to rebuild
the synchronization-set state model locally.

**Possible Lab refinement:** decompose the prose packets into linked
registration, confirmation, announcement, set-membership, achievement, and
completion obligations. Add shared synchronization-set variables and explicit
late-join/resignation transitions, then generate callback-order and
callback-model scenarios from that model while preserving the source packet
IDs.

### RL-070 — Connection-loss TSO cutoff behavior is split from the lost-federate report

**Status:** verified cross-cutting export gap; possible Lab refinement, not a
standards defect or conformance finding.

The pinned Lab export provides separate candidates for the connection-loss
report and its time-dependent consequence:

- `requirement-candidate-content-clauses-04-federation-management-page-050-l18-5`
  requires subscribed federates to receive `HLAreportFederateLost`;
- `requirement-candidate-content-clauses-04-federation-management-page-050-l21-6` requires the report to contain the lost regulating
  federate's last known time position;
- `requirement-candidate-content-clauses-04-federation-management-page-050-l24-7` says that the last-known position is determined so that
  the lost federate's TSO messages can be delivered; and
- `requirement-candidate-content-clauses-04-federation-management-page-050-l27-8` requires messages timestamped less than or equal to that
  position to be delivered.

These records have no `transition_ids` and no relation tying the captured
last-known time to the report, the inclusive TSO cutoff, the subsequent
automatic resignation, or the surviving federates' callback sequence. The
source is therefore decomposed into individually plausible statements without
an executable “capture time -> drain through cutoff -> report/cleanup” object.

**Umbra impact:** the `HLAreportFederateLost` contract deliberately proves the
ordinary and regional report path, its last-known-time parameter, and both
callback models. A separate 2026-08-20 equal-to-cutoff integration regression
now manually composes `l24-7` and `l27-8`: a time-regulating publisher is
granted time 6, faults before a constrained subscriber evokes, and its queued
timestamp-6 interaction is delivered before that subscriber's matching grant
whether the subscriber's TAR was pending at the fault or submitted after it.
The same bounded suite also queues a timestamp-5 interaction before that
time-6 boundary and delivers it before the survivor's grant to 6, exercising
the strict-less-than side of the source's inclusive wording.
One companion queues a timestamp-6 directed interaction to a target retained
through the lost publisher's automatic unconditional-divest cleanup. It proves
that the source's forced resignation does not suppress this already accepted,
cutoff-marked payload, while the recipient selector and target-lifetime checks
remain live at the callback boundary.
Another companion queues a reliable timestamp-6 attribute-update passel from a
publisher whose source object is retained through the same cleanup. It proves
the reflection's values, source, timestamp/order, and retraction designator
arrive before the surviving constrained federate's matching grant.
A paired two-survivor attribute-update case delivers the same accepted passel
to one recipient at time 6 and to the other after it requests time 6 later;
it proves the recipient-specific passel store and cutoff marker survive the
first reflection.
A further `DELETE_OBJECTS` collision uses a delete-privileged Restaurant FOM
object. It proves the accepted timestamp-6 reflection reaches the constrained
survivor before its matching grant, while the separately generated
receive-order automatic removal remains deferred by the disabled asynchronous-
delivery gate and is released by the survivor's next TAR. The two lifecycles
must not be collapsed into a replacement timestamped removal or an early
suppression of the reflection.
An asynchronous-delivery companion opens the same late TAR with that gate
enabled. It proves the accepted reflection precedes its matching grant and
the now-eligible receive-order automatic removal follows in that same advance;
the compound relation must preserve `reflect < grant < remove` without
inventing a replacement timestamped removal. An `HLA_IMMEDIATE` counterpart
exercises the same requeue while callbacks are dispatched synchronously and
preserves that ordering rather than allowing a re-entrant removal to overtake
the grant.
Two further late-TAR companions exposed the consequence that the Lab does not
model: the forced-resign cleanup can already be queued before a survivor opens
the cutoff boundary. One preserves an attribute reflection and the other a
directed interaction whose target would otherwise be removed by
`DELETE_OBJECTS`; each reaches its timestamp-6 callback before the later
grant, then releases the receive-order removal only through a subsequent gate.
A mixed two-survivor attribute case proves that this defer/release state is
per recipient: releasing one survivor's automatic cleanup cannot drain the
other survivor's still-protected object. These are not separate Lab defects;
they are concrete regressions caused by the compound lifecycle that RL-070
already identifies as absent from the export.
Callback-boundary unsubscribe companions expose the same missing state
transition from the opposite direction: queued cutoff attribute and directed
payloads are properly suppressed by their current declarations, but they have
still crossed their temporal boundary. Their recipient ledgers must therefore
become terminally suppressed, not remain pending or pretend that user code
received a callback; this lets the reserved `DELETE_OBJECTS` removal resume at
the next receive-order gate without inducing Request Retraction for a callback
that never ran.
Two companions queue timestamp-5 and timestamp-6 object deletions from that
regulator. They prove the common cutoff marker prevents automatic
unconditional-divest cleanup from discarding either accepted removal before the
surviving constrained federate receives its `Remove Object Instance` callback
and matching time-6 grant.
A third companion uses `DELETE_OBJECTS`: it proves that automatic cleanup does
not replace the accepted timestamped removal with a receive-order callback or
discard the queued deletion/retraction state before that same boundary.
A fourth companion keeps two constrained recipients. One drains at time 6,
then the other requests time 6 after that callback; it proves the object state
and cutoff marker remain until every pending recipient has crossed its own
boundary.
One 2026-08-20 composite case additionally enables asynchronous delivery for a
time-constrained survivor and proves the `HLAreportFederateLost` `HLAtimeStamp`
is that same time-6 boundary while the queued application interaction reaches
the matching grant. It deliberately makes no callback-order assertion between
the receive-order report and the timestamped interaction.
The runtime marks only at-or-before-cutoff payloads while forcing the
resignation; it intentionally makes no assertion about a later timestamp,
which the source permits to be delivered or not. The Lab's missing compound
relation still matters: the implemented scenarios are Umbra-owned
traceability, not generated Lab scenarios or conformance evidence.
Multi-recipient ordinary/direct interaction, alternate-advance, later-
timestamp, multi-passel/regional attribute-update, the remaining
automatic-resign-action collision matrix, and selector-mutation-under-loss
cases remain a testing backlog.

**Possible Lab refinement:** emit one cross-cutting connection-loss scenario
that links report construction, last-known-time capture, the inclusive TSO
delivery boundary, automatic-resign cleanup, and callback ordering. Generate
equal-to, less-than, and greater-than timestamp cases, plus report-before- or
after-cleanup ordering, including a `DELETE_OBJECTS`/timestamped-attribute
collision with pending and late-TAR receive-order defer/release transitions, a
postgrant asynchronous-delivery release transition in both callback models, a
directed target under the same collision, and independently staggered
survivors, plus a callback-boundary declaration change that consumes a queued
payload without invoking user code, while preserving the existing candidate
IDs.

### RL-071 — Time-advance request arguments and effective grants lack a compound relation

**Status:** verified cross-service modeling gap; possible Lab refinement, not a
standards defect or conformance finding.

The time-management export gives separate clause-scoped candidates for the
request and the later completion/delivery behavior. Examples include:

- TAR request `requirement-candidate-content-clauses-08-time-management-page-194-l21-3` and TAR completion `requirement-candidate-content-clauses-08-time-management-page-194-l69-19`;
- TARA request `requirement-candidate-content-clauses-08-time-management-page-195-l110-31` and its pre-grant message delivery
  `requirement-candidate-content-clauses-08-time-management-page-195-l164-49`;
- NMR request `requirement-candidate-content-clauses-08-time-management-page-197-l20-3` and its pre-grant message delivery
  `requirement-candidate-content-clauses-08-time-management-page-197-l50-13`; and
- FQR selection/minimum/optimistic-time records
  `requirement-candidate-content-clauses-08-time-management-page-200-l131-38`,
  `requirement-candidate-content-clauses-08-time-management-page-200-l140-41`, and
  `requirement-candidate-content-clauses-08-time-management-page-200-l164-49`, followed by the FQG completion record
  `requirement-candidate-content-clauses-08-time-management-page-201-l40-12`.

The records have empty `transition_ids` and do not identify a single request
instance's supplied boundary, selected target, actual grant, optimistic time,
delivered message cohort, and completion callback. That leaves an important
surface distinction to each consumer: a report may preserve the caller's
supplied boundary even when queued TSO input selects an earlier effective grant,
while FQR also exposes an optimistic time distinct from the actual grant.

**Umbra impact:** the new service-report tests had to specify locally that an
accepted request is reported before its grant and that the report retains the
supplied time. The tests also had to specify the inclusive/exclusive boundary
differences among TAR/NMR, TARA/NMRA, and FQR. These are useful source/test
decisions, but the Lab checker only validates the referenced IDs; it does not
validate the relation between the request record and the later grant result.

**2026-08-21 follow-up:** the positive timestamped Delete Object Instance
alternate-advance case had to repeat one exact Catch2 selector across the
deletion, TAR, FQR, TARA, and NMRA contracts. That fan-out is the correct local
traceability result for a compound callback-before-grant scenario, but the Lab
does not emit the compound scenario or an ownership graph for those related
candidate IDs. The consumer must assemble the relationship and keep each lane
selection synchronized; this is a workflow rough edge, not a new normative
finding.

The same export shape recurred when the directed-interaction companion added
FQR, TARA, and NMRA to a target-qualified TSO case. The directed send/receive
records, alternate-advance records, grant callbacks, and terminal Retract
classification are individually valid, but the Lab does not synthesize their
single recipient-frontier scenario. Umbra therefore repeats the exact selector
intentionally across the directed, time-management, and Request Retraction
contracts and keeps the compound relationship in the Catch2 plan's
`next_action` text. This confirms the earlier observation rather than adding a
new Lab defect.

**Possible Lab refinement:** model a typed time-advance request instance with
`supplied_time`, `selected_time`, `actual_grant`, `optimistic_time`, delivery
cohort, callback order, and mode-specific boundary polarity. Link the request,
report, message-delivery, and grant candidates and generate equality, earlier,
later, empty-queue, and future-input scenarios.

### RL-072 — “Outstanding save” replacement has no lifecycle boundary

**Status:** verified state-machine export gap; possible Lab refinement, not a
standards defect or conformance finding.

The Lab does export the useful candidate
`requirement-candidate-content-clauses-04-federation-management-page-066-l22-5`:
“At most, one requested save shall be outstanding. A new save request shall
replace any outstanding save.” However, the candidate has an empty
`transition_ids` array and no relation to the save-request, save-admission,
`Initiate Federate Save`, or save-in-progress states.

The new timed-save test had to choose and document a boundary: a second request
replaces the first while neither request has reached the `Initiate Federate Save`
boundary, and only the final label is later announced. The Lab surface does
not make clear which state ends “outstanding”—request admission, initiation,
the callback, `Federate Save Begun`, or completion—or what a replacement means
for an already queued initiation callback.

**Umbra impact:** the save contract can prove one replacement scenario, but it
cannot claim Lab-derived coverage for replacement after admission, replacement
during save-in-progress, or stale callback suppression. Those behaviors are
currently local state-machine choices.

**Possible Lab refinement:** connect the one-outstanding candidate to explicit
save lifecycle states and define replacement/cancellation effects at each
boundary. Generate traces for replacement before initiation, after initiation,
during save-in-progress, and after completion/failure, including the expected
callback label and stale-work behavior.

### RL-073 — Prospective order changes lack an acceptance/confirmation relation

**Status:** verified cross-artifact export gap; possible Lab refinement, not a
standards defect or conformance finding.

The order-management candidates are split across initialization, change, and
future-use fragments:

- attribute initialization/change: `requirement-candidate-content-clauses-08-time-management-page-212-l42-7` and
  `requirement-candidate-content-clauses-08-time-management-page-212-l48-9`;
- ownership-transfer reset: `requirement-candidate-content-clauses-08-time-management-page-213-l76-20`, `requirement-candidate-content-clauses-08-time-management-page-213-l79-21`, and
  `requirement-candidate-content-clauses-08-time-management-page-213-l94-26`; and
- interaction initialization/change: `requirement-candidate-content-clauses-08-time-management-page-214-l81-20` and
  `requirement-candidate-content-clauses-08-time-management-page-214-l87-22`.

The candidates have no transition mapping connecting service acceptance to the
confirmation callback or defining what “future” means for an update/send that
is submitted before, during, or after that callback. The export also does not
compose the class-default snapshot, per-instance override, ownership-transfer,
and publisher-scoped interaction order into one lifecycle.

**Umbra impact:** the new tests had to establish local rules: a class-default
change affects later registrations only; an instance change remains pending
until confirmation; an update submitted before confirmation uses the captured
old order; and a later interaction send uses the new order. Those are precisely
the boundary cases a future consumer would otherwise have to rediscover.

**Possible Lab refinement:** emit a prospective-change relation with
`requested`, `accepted`, `confirmed`, and `effective-for-new-operation` states.
Link it to the affected object/interaction scope and generate pre-confirmation,
confirmation-boundary, post-confirmation, ownership-transfer, and already
queued-message scenarios.

### RL-074 — Cross-page services do not link their continuation content to the opening candidates

**Status:** verified source-navigation and export-discoverability gap; possible
Lab refinement, not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, §6.27, *Change Default Attribute
Transportation Type*, starts in
`content-block-content-15161-page-141.tex:122-157`. Its four exported
candidates are all attached to the opening page:

- `requirement-candidate-content-clauses-06-object-management-page-141-l125-32`;
- `...page-141-l131-34`;
- `...page-141-l137-36`; and
- `...page-141-l143-38`.

The following source-content page,
`content-block-content-15161-page-142.tex:5-165`, contains the service's
supplied arguments, returned arguments, preconditions, postcondition, and
exceptions. Yet `content-15161-page-142` exposes only
`requirement-candidate-content-clauses-06-object-management-page-142-l173-48`,
which belongs to the next service, §6.28. Neither content block nor the four
§6.27 candidates carries an explicit continuation, service-span, or
page-neighbour relation. A page-local candidate lookup therefore makes the
continued §6.27 material appear to have no requirements, even though its
opening-page candidates exist.

The immediately following §6.28, *Query Attribute Transportation Type*,
shows the same pattern. Its opening candidate is
`requirement-candidate-content-clauses-06-object-management-page-142-l173-48`,
while `content-block-content-15161-page-143.tex:5-154` contains the continued
supplied arguments, preconditions, and exceptions. `content-15161-page-143`
instead exposes only the two §6.29 candidates
`...page-143-l156-42` and `...page-143-l162-44`. This corroborates that the
problem is the absence of cross-page service relations, rather than a
one-service extraction omission.

**Umbra impact:** while implementing the `Change Default Attribute
Transportation Type` report-file slice, Umbra initially treated page 142's
absence of §6.27 candidate IDs as a possible export omission. A manual source
pass found the four page-141 IDs and avoided creating a false Lab defect or
inventing an ID. The resulting requirement contract uses those immutable IDs,
but still retains direct source coordinates for the page-142 argument and
exception semantics. This is a workflow discoverability cost, not evidence
that the existing candidates are invalid.

**Possible Lab refinement:** expose a stable service-span relationship from a
service heading to every source-content block and candidate that belongs to
that service, including continuations across page boundaries. At minimum, add
`continues_from`/`continues_to` or a `service_id` to content blocks and emit a
checker warning when a source page begins service components but contains no
candidates for that service. Preserve all current immutable candidate IDs and
page-local source coordinates.

### RL-075 — Opening §11.5 candidates are attributed to §11.5.1 before that subclause begins

**Status:** verified clause-ownership mismatch; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the reconstructed source
`content-block-content-15161-page-292.tex` labels §11.5, *Service reporting*,
at lines 9--10 and places its report-generation, sink-selection, switch-source,
and subscription-interlock prose at lines 13--82. The §11.5.1, *Argument
encoding*, heading does not begin until lines 84--85. The semantic hierarchy
nevertheless gives all of the opening candidates
`requirement-candidate-content-clauses-11-management-object-model-page-292-l9-2`,
`...-l18-5`, `...-l21-6`, `...-l24-7`, `...-l30-9`, and `...-l57-18` the
`owner_id` `clause-11.5.1`. In particular, the Requirements Lab checker reports
the file-sink candidate `...page-292-l18-5` as clause `11.5.1`, despite its
source span preceding that subclause.

**Umbra impact:** source contracts retain the Lab's exported `clause-11.5.1`
value where the checker requires it, while the design and test records retain
the direct §11.5 page-292 source coordinates. The Query Attribute Ownership
file-report record uses that local reconciliation rather than claiming the
Lab's clause assignment is the normative section boundary.

**Possible Lab refinement:** assign candidates to the active semantic heading
at their source location, or emit an explicit `source_clause_id` separate from
the checker's ownership hierarchy. Existing candidate IDs and cross-references
can remain stable; the report should flag candidates whose source line range
precedes the heading of their assigned subclause.

### RL-076 — Rotated Table 5 continuation pages lose their data cells in the table export

**Status:** verified structured-table extraction gap; possible Lab refinement,
not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, Table 5 starts with 67 exported
cells on page 295, but each continuation artifact from pages 296 through 304
contains only the four header cells (`(continued)`, `Type`, `Encoding`, and
`Example`). For example,
`output/semantic/table-record-table-5-p303-data.tex` declares only
`table-5-p303.c0001` through `c0004`, although the raw rotated source
`content-block-content-15161-page-303.tex:77-87` visibly contains the
`UserSuppliedTag` example. Page 304 exhibits the same split: its raw source
contains the `UserSuppliedTag` and `VariableLengthData` rows at lines 179--195,
while `table-record-table-5-p304-data.tex` has only the four headers.

This is narrower and more severe than the row-level-candidate limitation in
RL-067: a consumer of the Lab's structured table export cannot discover those
continuation values at all. It must fall back to layout-oriented rotated text,
where column and row association has to be reconstructed locally.

**Umbra impact:** the first binary user-tag report slice had to source its
argument form from raw page coordinates rather than a Table 5 row/cell record.
That keeps the formatter explicitly traceable, but it prevents the checker
from validating the precise `UserSuppliedTag` type/value relationship or from
detecting a regression in the continuation-page extraction.

**Possible Lab refinement:** preserve the current page and raw-source outputs,
then add an orientation-aware table pass that emits every continuation row and
cell with stable IDs, column identity, page coordinates, and parent
`table-family-table-5`. A build check should compare the number of detected
non-header cells against the raw rotated text and flag a continuation page
whose structured export contains headers only.

### RL-077 — The Lab cannot represent a cross-document Table 5/MIM type-code conflict

**Status:** verified cross-resource consistency gap; possible Lab refinement.
The underlying source disagreement needs an IEEE/SISO errata decision, not a
local conformance conclusion.

The reconstructed IEEE 1516.1-2025 Table 5 example at
`content-block-content-15161-page-303.tex:80-87` depicts
`"HLAargumentType": 63` for `"HLAargumentName": "UserSuppliedTag"`; the
Table 5 row on page 304 identifies that value as `Binary Data`. In contrast,
the unmodified official IEEE 1516.2-2025 standard MIM vendored by Umbra names
`UserSuppliedTag` value `60` in its `HLAargumentType` enumeration at
`third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml:3455-3456`,
and its final enumerator is `MessageRetractionReturn` value `62` at
lines 3463--3464. The Table 5 report example is therefore not directly
decodable against that MIM enumeration.

The Requirements Lab exports the IEEE 1516.1 text and its candidate IDs, but
has no first-class link to the companion IEEE 1516.2 MIM resource, no source
authority/edition metadata at the field level, and no way to record a detected
cross-document numeric contradiction. RL-076 also means the Table 5
continuation row is absent from the structured table output, making this
conflict harder to discover automatically.

**Umbra impact:** Umbra will not silently describe Table 5's literal `63` as a
static-MIM value. The file-report formatter will keep the Table 5 literal
separate from any future encoded MOM-interaction path, and its test/contract
will name the source conflict and remain explicitly non-conformance evidence
until an authoritative correction is available.

**Possible Lab refinement:** allow a requirement/table cell to declare
cross-document dependencies (for example, a particular MIM enumerator), the
authoritative artifact revision, and a machine-readable `conflicts_with`
record. The checker should then surface incompatible numeric values as a
review-required source conflict rather than letting consumers silently choose a
local interpretation.

### RL-078 — Coalesced service pages can assign a service-level requirement to a trailing subclause

**Status:** verified clause-ownership mismatch; possible Lab refinement, not a
standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the source text for §5.4,
*Publish Interaction Class service*, appears at page 93 lines 17--19 in
`content-block-content-15161-page-093.tex`; its candidate is
`requirement-candidate-content-clauses-05-declaration-management-page-093-l17-1`.
The §5.5 *Unpublish Interaction Class service* text follows at lines 140--145,
with candidate
`requirement-candidate-content-clauses-05-declaration-management-page-093-l140-35`.
The following headings at lines 147 and 149 introduce §5.5.1, *Supplied
arguments*, and §5.5.2, *Returned arguments*, respectively. Both service-level
candidates are nevertheless owned by `clause-5.5.2` in `hierarchy.json`, rather
than their active parents `clause-5.4` and `clause-5.5`. The focused
Requirements Lab checker reproduces the result as `expected 'clause-5.4', Lab
has 'clause-5.5.2'` and `expected 'clause-5.5', Lab has 'clause-5.5.2'`.

The same boundary effect occurs on page 90 for §5.2, *Publish Object Class
Attributes service*. Its service-body source at lines 79--81 of
`content-block-content-15161-page-090.tex` yields
`requirement-candidate-content-clauses-05-declaration-management-page-090-l79-22`,
but `hierarchy.json` owns it as `clause-5.2.4` rather than active parent
`clause-5.2`. The checker reproduces that form as `expected 'clause-5.2', Lab
has 'clause-5.2.4'`.

Page 91 exhibits the same behavior for §5.3, *Unpublish Object Class
Attributes service*. Its service-body candidates
`requirement-candidate-content-clauses-05-declaration-management-page-091-l121-35`
and
`requirement-candidate-content-clauses-05-declaration-management-page-091-l127-37`
are owned by `clause-5.3.3` rather than active parent `clause-5.3` in
`hierarchy.json`. The checker therefore reproduces the form `expected
'clause-5.3', Lab has 'clause-5.3.3'`.

The same revision's page-100 source gives §5.11, *Unsubscribe Interaction
Class service*, at lines 129--139 of
`content-block-content-15161-page-100.tex`. Its service-level candidate is
`requirement-candidate-content-clauses-05-declaration-management-page-100-l129-35`,
but `hierarchy.json` owns it as `clause-5.11.3` rather than active parent
`clause-5.11`. The checker reproduces that independent form as `expected
'clause-5.11', Lab has 'clause-5.11.3'`.

Page 99 exhibits the same behavior for §5.10, *Subscribe Interaction Class
service*. The service text at lines 75--77 of
`content-block-content-15161-page-099.tex` produces
`requirement-candidate-content-clauses-05-declaration-management-page-099-l75-20`,
but `hierarchy.json` owns it as `clause-5.10.2` rather than active parent
`clause-5.10`. The checker reproduces that third independent form as `expected
'clause-5.10', Lab has 'clause-5.10.2'`.

The same boundary effect also occurs for §4.22, *Federate Save Complete
service*. The first two lines of its service description on page 68 yield
`requirement-candidate-content-clauses-04-federation-management-page-068-l115-29`
and `...page-068-l118-30`. The source block places them after the §4.22 heading
and before §4.22.1, *Supplied arguments*, yet both exported candidates declare
`clause_id` `clause-4.22.3`, the page's trailing *Preconditions* subclause.
Their stale display titles point to `content/clauses/annexes-page-439.tex`;
that separate source-title provenance issue remains covered by RL-002. The
direct page-068 source coordinates and the rendered IEEE page make the §4.22
service association reproducible despite the structural owner.

The same boundary effect occurs for §4.23, *Federation Saved* service. Its
service-description candidates on page 69—
`requirement-candidate-content-clauses-04-federation-management-page-069-l64-15`,
`...page-069-l67-16`, `...page-069-l70-17`,
`...page-069-l76-19`, and `...page-069-l133-38`—all declare `clause_id`
`clause-4.23.2`. Their source coordinates span lines 64--135, after the
§4.23 heading and before §4.23.1, *Supplied arguments*, so the actual
service association is §4.23 rather than its trailing *Returned arguments*
subclause. The rendered IEEE 1516.1-2025 text independently confirms both
the success/failure result semantics and the recipient rule. This is the same
within-page ownership defect, not a second normative finding.

The same boundary effect occurs for §4.26, *Federation Save Status Response*
service. Its page-72 service-description candidates
`requirement-candidate-content-clauses-04-federation-management-page-072-l17-1`,
`...page-072-l20-2`, `...page-072-l23-3`, and `...page-072-l26-4` all declare
`clause_id` `clause-4.27.2`. The direct source spans lines 17--28, after the
§4.26 heading and before §4.26.1, *Supplied arguments*, and say that the RTI
shall invoke the response at the querying joined federate with its list of
joined federates and save statuses. The actual service association is §4.26,
not the following §4.27 Request Federation Restore service's *Returned
arguments* subclause. This is another reproducible instance of the same
coalesced-page ownership defect, not a distinct standards finding.

The same boundary effect occurs for §4.29, *Federation Restore Begun service*.
Its page-74 service-description candidates
`requirement-candidate-content-clauses-04-federation-management-page-074-l112-28`,
`...page-074-l115-29`, and `...page-074-l124-32` all declare `clause_id`
`clause-4.29.3`. Their direct source spans lines 112--126, after the §4.29
heading and before §4.29.1, *Supplied arguments*, and include the explicit
requirement that the RTI invoke the service at every joined federate including
the requester. The actual service association is §4.29 rather than its
trailing *Preconditions* subclause. This is another reproducible instance of
the same coalesced-page ownership defect, not a distinct standards finding.

The same boundary effect occurs for §4.30, *Initiate Federate Restore
service*. Its page-75 service-description candidates
`requirement-candidate-content-clauses-04-federation-management-page-075-l44-7`
and `...page-075-l47-8` both declare `clause_id` `clause-4.30.6`. Their direct
source spans lines 44--49 after the §4.30 heading and before §4.30.1,
*Supplied arguments*; the text instructs the joined federate to return to its
saved state and to select it using the federation, save label, designator, and
name. The actual service association is §4.30 rather than its trailing
*Reference state charts* subclause. Their stale display titles point to
`content/clauses/annexes-page-439.tex`, the separate provenance defect
recorded under RL-002. This is the same coalesced-page ownership defect, not a
distinct standards finding.

The same boundary effect occurs for §4.27, *Request Federation Restore
service*. The direct service-description candidates
`requirement-candidate-content-clauses-04-federation-management-page-072-l131-32`,
`...page-072-l134-33`, and `...page-072-l137-34` contain the §4.27 request
semantics, but each exports `clause_id` `clause-4.27.2`, the trailing
*Returned arguments* subclause rather than the active §4.27 service. Their
display titles again retain the stale `content/clauses/annexes-page-439.tex`
provenance covered by RL-002. The official rendered page 57 and the candidate
source coordinates make the intended service association reproducible.

The same boundary effect occurs for §4.31, *Federate Restore Complete
service*. Its direct service-description candidate
`requirement-candidate-content-clauses-04-federation-management-page-076-l15-1`
begins immediately after the §4.31 heading and says that the service notifies
the RTI that a joined federate completed its restore attempt. The exported
candidate instead declares `clause_id` `clause-4.32`, the following
*Federation Restored* callback service. Its display title again retains the
stale `content/clauses/annexes-page-439.tex` provenance covered by RL-002. The
direct page-076 source coordinate and official rendered page 61 make the
intended §4.31 association reproducible.

The same boundary effect occurs for §4.33, *Abort Federation Restore service*.
Its direct callback/result candidates
`requirement-candidate-content-clauses-04-federation-management-page-078-l20-4`
and `...page-078-l29-7` state that the RTI shall abort the current restore and
describe the abort result, but both export `clause_id` `clause-4.34`, the
following *Query Federation Restore Status* service. Their display titles again
retain the stale `content/clauses/annexes-page-439.tex` provenance covered by
RL-002. The source spans on page 78 and official rendered page 62 make the
intended §4.33 association reproducible.

The same boundary effect occurs for §6.22, *Provide Attribute Value Update
service*. In `content-block-content-15161-page-137.tex`, the §6.22 heading is
at line 25 and §6.23, *Turn Updates On For Object Instance service*, does not
begin until line 148. Nevertheless,
`requirement-candidate-content-clauses-06-object-management-page-137-l40-8`
for the propagated user tag (source lines 40--42) and
`...page-137-l43-9` for the at-most-one-provider-callback rule (source lines
43--45) both export `clause_id` `clause-6.23`. The direct source belongs to
§6.22, not to the later Turn Updates On service. This is another reproducible
instance of the same coalesced-page ownership defect, not a distinct standards
finding.

The same boundary rule is visible at the start of §6.2, *Reserve Object
Instance Name service*. The direct source at
`content-block-content-15161-page-113.tex` lines 9--26 places the service
heading and its supplied-name prose before §6.2.1, while
`requirement-candidate-content-clauses-06-object-management-page-113-l21-3`
and `...page-113-l24-4` both export `clause_id` `clause-6.3`, the following
*Object Instance Name Reserved* service. The source coordinates and the
official page make the §6.2 association unambiguous; the Lab's structural
owner is nevertheless the checker-visible §6.3 value. The adjacent §6.4
release candidates retain their correct `clause-6.4.3` owner, so this is a
useful asymmetric example of the same page-boundary behavior.

This looks like an extraction-unit boundary effect: the page's semantic
paragraph records §5.2, §5.3, §5.4, §5.5, §5.10, or §5.11 and the later headings
together, so ownership is resolved from the final nested heading instead of
the source span's active heading. Pages 90, 91, 99, and 100 exhibit the same
rule with §5.2/§5.2.4, §5.3/§5.3.3, §5.10/§5.10.2, and §5.11/§5.11.3,
respectively. It is analogous to RL-075, but occurs within a short service
section rather than before a subclause begins.

**Umbra impact:** the Publish Object Class Attributes, Unpublish Object Class
Attributes, Publish Interaction Class, Unpublish Interaction Class, Subscribe
Interaction Class, and Unsubscribe Interaction Class requirements contracts
use the exported `clause-5.2.4`, `clause-5.3.3`, `clause-5.5.2`,
`clause-5.10.2`, or `clause-5.11.3` values so the checker remains an honest
regression guard. Their source IDs, service names, implementation notes, and
test names continue to identify the direct §5.2, §5.3, §5.4, §5.5, §5.10, and
§5.11 service requirements; no claim is made that any wording is actually a
returned-arguments or preconditions requirement. The Federate Save Complete
slice likewise retains its immutable §4.22 source IDs while recording the
direct service semantics; it does not treat the exported `clause-4.22.3`
ownership as evidence that a service-description statement belongs solely to
the preconditions subsection. The Request Federation Restore reporting slice
likewise preserves its immutable §4.27 candidate IDs while treating their
exported `clause-4.27.2` owner as a checker-stable structural artifact, not as
evidence that the service description belongs to returned arguments. The
Federate Restore Complete reporting slice likewise retains its immutable §4.31
source candidate while using the Lab's `clause-4.32` value only as the
checker-stable structural owner; it does not reclassify the federate-to-RTI
completion service as the subsequent federation-to-federate callback. The
Abort Federation Restore reporting slice likewise retains its immutable §4.33
source candidates while using the Lab's `clause-4.34` value only as a
checker-stable structural owner; it does not reclassify the abort request and
its restore-result semantics as the subsequent status-query service. The
Federation Saved reporting slice likewise retains its immutable §4.23
candidate IDs and their checker-required `clause-4.23.2` owner while treating
the source as the §4.23 result callback; it does not reclassify success,
failure-reason, or recipient semantics as returned arguments.
The Federation Save Status Response reporting slice likewise retains its
immutable §4.26 candidate IDs and checker-required `clause-4.27.2` owner while
treating the source as the §4.26 RTI-to-querying-federate callback; it does not
reclassify the response's participant/status list as §4.27 returned arguments.
The Federation Restore Begun reporting slice likewise retains its immutable
§4.29 candidate IDs and their checker-required `clause-4.29.3` owner while
treating the source as the §4.29 all-joined-federate callback; it does not
reclassify the service's recipient rule as a preconditions requirement.
The Initiate Federate Restore reporting slice likewise retains its immutable
§4.30 candidate IDs and uses the Lab's `clause-4.30.6` value only as a
checker-stable structural owner; it does not reclassify the restore instruction
or its supplied state-selection values as a reference-state-chart requirement.
The Provide Attribute Value Update reporting slice likewise retains its
immutable §6.22 candidate IDs and uses the Lab's `clause-6.23` value only as a
checker-stable structural owner; it does not classify the provider callback's
tag propagation or at-most-one rule as a Turn Updates On requirement.

**Possible Lab refinement:** derive candidate ownership from the heading active
at the candidate's start coordinate, not the final heading in a coalesced
paragraph. When a paragraph spans headings, emit child source spans or retain
an explicit `source_clause_id` alongside any structural owner so consumers can
distinguish the extraction hierarchy from the normative section boundary.

### RL-079 — Table 5's Boolean example capitalization differs from §11.5.1

**Status:** verified source-text inconsistency; possible Lab refinement, not a
local conformance conclusion.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the §11.5.1 argument-encoding
definition in `content-block-content-15161-page-292.tex:153-157` specifies a
Boolean as the character sequence `true` or `false` (without quotes). The
structured Table 5 export preserves the Boolean row at
`table-record-table-5-p295-data.tex` cells `c0062` through `c0064`, and the
raw rotated source at `content-block-content-15161-page-295.tex:205-213`
shows its example value as `True` with an uppercase initial letter.

The Lab faithfully exposes both artifacts, but does not express that the
Table 5 example and the formal §11.5.1 value definition disagree. A consumer
that treats the example as the lexical rule can silently generate a different
file-report value than one that follows the definition.

**Umbra impact:** `formatMomBoolean` uses the explicit §11.5.1 lowercase
`true`/`false` values. The Subscribe Interaction Class report slice uses that
formatter and records the discrepancy instead of copying the table example as
a locally chosen convention.

**Possible Lab refinement:** permit a table example to reference a governing
type-definition record, then check literal examples against that definition
when it is lexical. If the example is intentionally illustrative rather than
normative, expose an explicit `example_only` qualifier so consumers can avoid
mistaking it for the encoding rule.

### RL-080 — A cross-page service continuation can be assigned to the next service

**Status:** verified clause-ownership and service-association mismatch;
possible Lab refinement, not a standards defect or conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the rendered source block for
page 97 begins as a continuation of §5.8, *Subscribe Object Class Attributes
service*. Its first text states the active/passive consequence at
`content-block-content-15161-page-097.tex:15-21`, followed by the maximum
update-rate rule at lines 24--30. The page then presents §5.8.1 through §5.8.6
at lines 32--660; the next service, §5.9, does not begin until lines 663--664.

Nevertheless, the three exported maximum-update-rate candidates
`requirement-candidate-content-clauses-05-declaration-management-page-097-l20-4`,
`...page-097-l23-5`, and `...page-097-l26-6` all declare `clause_id`
`clause-5.9` in `semantic/requirements.json`. `hierarchy.json` consequently
places them in §5.9's `record_ids`, even though their statements say that the
attributes are "subscribed" and their source precedes the §5.9 heading. The
same content element's generated record list contains those three candidates
beside the genuinely §5.9 candidate at page-097 line 191, so a consumer cannot
recover the correct service association from the element-level list alone.

This is more severe than RL-074's discoverability gap: the continuation has
been extracted into immutable candidates, but the candidates are affirmatively
owned by the following service. It is also distinct from RL-078's within-page
coalescing effect because this error crosses the page boundary and changes the
service to which a behavior appears to belong.

The same defect reproduces in the adjacent directed-interaction services. Page
102 begins with the final delivery and inheritance rules of §5.12, *Subscribe
Object Class Directed Interactions service*, before §5.12.1 begins. Its two
immutable candidates
`requirement-candidate-content-clauses-05-declaration-management-page-102-l10-1`
and `...page-102-l19-4` retain source coordinates
`content/clauses/05-declaration-management-page-102.tex:10-12` and `:19-21`,
but both declare `clause_id` `clause-5.13`. The §5.13 heading does not occur
until page-102 line 163. Page 103 then begins with §5.13.1 through §5.13.4;
its postcondition candidate
`requirement-candidate-content-clauses-05-declaration-management-page-103-l108-29`
states that a joined federate shall receive no subsequent *Receive Directed
Interaction* invocations for unsubscribed pairs, yet declares `clause_id`
`clause-5.14.3`. The §5.14 heading begins only after that source at page-103
line 132. These independent cases confirm that a continuation can be attached
to the next service/subclause even where the candidate's own source range and
statement identify the preceding service. The candidate display titles' stale
`annexes-page-439.tex` paths are the separate provenance problem already
recorded in RL-002; the structured `source.path` coordinates above are the
reproducible evidence for this issue.

**Umbra impact:** the Subscribe Object Class Attributes service-report slice
will preserve the three immutable candidate IDs only as source pointers and
will explicitly record their exported §5.9 ownership mismatch. Its update-rate
argument semantics are anchored to the direct §5.8 continuation coordinates and
the exact official C++ declaration; they are not treated as evidence about the
Unsubscribe Object Class Attributes service. This keeps the upcoming focused
test lane honest while retaining a checker-visible regression anchor.

**Possible Lab refinement:** retain a stable active `service_id` across page
breaks and assign every candidate from its source start coordinate to that
service, independently of the later heading in the same extraction element.
When a continuation and a new service share one page-level element, emit child
source spans (or a separate `source_service_id`) and flag any candidate whose
assigned service begins after its source range. Preserve current immutable
candidate IDs and expose the corrected association as explicit reconciliation
metadata.

### RL-081 — The supplied FDD XSD cannot represent DIF-permitted multiple directed interactions

**Status:** verified cross-artifact cardinality conflict; possible Lab
reconciliation refinement, not a local conformance conclusion.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, §5.12's extracted source at
`content-block-content-15161-page-101.tex:110-121` says that a joined
federate may subscribe to two directed interaction classes associated with the
same object class using different selector modes. The official IEEE 1516.2
artifacts vendored for Umbra disagree about whether the corresponding model can
be represented: `IEEE1516-DIF-2025.xsd:1912` declares
`directedInteraction` with `maxOccurs="unbounded"`, while
`IEEE1516-FDD-2025.xsd:1930` omits `maxOccurs`, so its XSD cardinality is one.

The fixtures
`cpp/tests/data/directed-interaction-selector-matrix-object-fom.xml` and
`...-interaction-fom.xml` independently validate as DIF modules and declare
the two classes described by §5.12. Umbra's composed-FDD materializer then
deterministically rejects their result against the supplied FDD XSD with
`directedInteraction: This element is not expected`. The regression
`The FDD materializer surfaces the multiple-directed-class schema conflict`
retains both facts: raw DIF acceptance and composed-FDD rejection. This is not
a Requirements Lab extraction failure—RL-006 already records that the Lab is
not a source for IEEE 1516.2 schemas—but it is a cross-document reconciliation
need analogous to RL-077's FOM/FDD dependency issue.

**Umbra impact:** Umbra retains the valid DIF fixtures and expected-rejection
guard, but does not silently select one declaration, emit an unvalidated FDD,
or claim runtime coverage for the §5.12 two-class selector case. The full
per-directed-class delivery matrix remains blocked pending an authoritative
reconciliation or corrected FDD schema interpretation.

**Possible Lab refinement:** supplement source requirement exports with a
cross-document dependency/cardinality reconciliation record when a requirement
depends on a separately published schema artifact. The record should identify
the active publication/version, relevant XSD particles, and any detected
contradiction without reclassifying it as a normative conclusion. This would
let consumers distinguish an unimplemented behavior from one that cannot be
faithfully exercised using the supplied companion artifacts.

### RL-082 — Initiate Federate Save candidates are attached to the following service

**Status:** verified clause-ownership mismatch; possible extraction refinement,
not a standards defect or local conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the page-67 source candidates for
the *Initiate Federate Save* callback—especially
`requirement-candidate-content-clauses-04-federation-management-page-067-l37-8`
and `...-l40-9`—declare `clause_id` `clause-4.21.1`. Their structured source
coordinates are instead
`content/clauses/04-federation-management-page-067.tex:37-42`, where the
active service is §4.20. The rendered IEEE 1516.1-2025 text labels that service
`4.20 Initiate Federate Save † service`, gives its supplied arguments in
§4.20.1, and begins §4.21 (*Federate Save Begun*) only afterwards. The exported
candidate titles' `annexes-page-439.tex` provenance is the separately tracked
RL-002 display problem.

**Umbra impact:** the queued recipient service-report lane retains the stable
candidate IDs as immutable source pointers, but records the local service
association as §4.20 / `InitiateFederateSave`. It does not treat the Lab's
`clause-4.21.1` field as evidence that the callback belongs to `Federate Save
Begun`; the existing direct time-constrained admission coverage remains mapped
through the same source evidence and its explicitly stated boundary.

**Possible Lab refinement:** apply the active-service owner at each candidate's
source start coordinate, rather than inheriting the following service's first
subclause. Retain the immutable IDs and expose a corrected `service_id` or
reconciled clause association, consistent with the cross-page ownership
correction proposed in RL-080.

### RL-083 — Table 5’s restore-status-set row conflicts with the official MIM and is not machine-readable

**Status:** verified cross-artifact/source-text discrepancy and structured-table
export gap; possible Lab refinement. The underlying Table 5 terminology and
example require an IEEE/SISO correction or reconciliation decision, not a
local conformance conclusion.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the raw rotated Table 5 source at
`content-block-content-15161-page-297.tex:107-112` labels the collection
`FederateHandleRestoreStatusSet` and its encoding
`Array<FederateHandleRestoreStatus>`. The preceding singular row at lines
77--105 instead correctly labels `FederateRestoreStatus` and its
`preRestoreHandle`, `postRestoreHandle`, and `status` fields. The collection
example at lines 113--139 has three malformed leading braces in its second
object, so it cannot be adopted as a mechanically parseable canonical value.
The continuation table export
`table-record-table-5-p297-data.tex` contains only its four headers, as
already characterized by RL-076, and therefore exposes no row or cell record
that a contract can cite.

The unmodified official 2025 standard MIM at
`third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml:3291-3296`
names `FederateRestoreStatus` as argument type 19 and
`FederateRestoreStatusSet` as argument type 20. The official C++ binding calls
the matching collection `FederateRestoreStatusVector`. Thus the Table 5
collection spelling and element spelling do not agree with the companion MIM
or the public binding, even before the malformed illustration is considered.

**Umbra impact:** the §4.35 private file-report formatter uses the MIM’s
type-20 `FederateRestoreStatusSet` identity and the official C++ vector, while
retaining the singular Table 5 record’s explicitly depicted field names. It
does not copy the incorrect collection spelling or malformed second example,
does not infer an encoded public MOM interaction, and records this as
source-level traceability rather than conformance evidence.

**Possible Lab refinement:** preserve the raw Table 5 source, but emit an
orientation-aware row record for it and allow that record to declare a
companion-MIM dependency plus a `conflicts_with` relation. A consistency check
should flag a Table 5 type/element name that differs from the official MIM and
flag malformed JSON-like examples separately from intended record structure.

### RL-084 — Focused-lane selectors can overmatch neighboring contracts

**Status:** verified Umbra test-integration hazard; not a Requirements Lab
extraction defect or a standards/conformance finding.

While adding focused service-report lanes, a CMake `MATCH` expression that was
not anchored at the end of the registered test/contract name selected a
neighboring requirement family sharing the same prefix. The resulting CTest
lane still appeared healthy, but its labels silently widened the slice and
made a local service test appear to carry evidence from an adjacent service.
This is particularly easy to miss because the Requirements Lab and API
contracts themselves remain individually valid; the error occurs in the
consumer-side mapping from those contracts to CTest labels.

**Umbra impact:** each focused lane now uses an exact, end-anchored selector
where a shared traceability test must be mapped intentionally, and
`focused_service_lane_catalog` is run after every mapping change. The catalog
requires a Catch2 test, Requirements Lab membership, and API-contract
membership for each lane, while also making accidental widening visible in
review. The adjacent Commit Region Modifications, Delete Region, and Set Range
Bounds lanes, plus the regional interaction subscription service-report lane,
all pass that catalog check.

**Possible tooling refinement:** make lane declarations consume exact
registered contract/test identifiers (or require an explicit `allow_prefix`
flag) instead of accepting unconstrained regular expressions. Emit the final
selected test and contract IDs in the catalog report so a widened mapping is
machine-detectable before a lane is promoted.

### RL-085 — Service candidates do not carry FOM switch-fixture defaults

**Status:** verified consumer-side fixture/traceability gap; possible Lab
refinement, not a Requirements Lab extraction defect or a conformance finding.

The Requirements Lab's service and API records identify the switch setters and
their declared Boolean arguments, but the exported contract/test mapping does
not carry the defaults supplied by the FOM fixture used to exercise those
services. Umbra's focused regional-interaction service-report case uses
`cpp/tests/data/switch-support-enabled-fom.xml`; that valid 2025 DIF module
sets `conveyRegionDesignatorSets`, `serviceReporting`, and
`sendServiceReportsToFile` to `isEnabled="true"`. A test that assumed the
binding defaults without explicitly resetting those switches therefore started
with a different state than the intended disabled-report setup. The Lab
records remain correct—the rough edge is that the test seed's switch metadata
is invisible at the traceability boundary.

**Umbra impact:** every focused service lane now declares its switch baseline
in the test body, resets report and callback switches before taking a file
baseline, and asserts the resulting getter values. This keeps the service
report serial assertions independent of a fixture's advisory defaults and
prevents a FOM-provided `true` value from being mistaken for a public API
default. The fixture remains useful for testing enabled-switch composition; no
standard default is inferred from it.

**Possible Lab refinement:** allow a test seed to publish its required FOM/SOM
module set and effective switch/default values as structured preconditions.
The lane catalog should display those preconditions beside the selected
service/API records and flag a test that relies on an unrecorded fixture
state. This would make configuration-sensitive regressions visible without
turning a consumer fixture into a normative requirement.

### RL-086 — ResignAction directives are exported as one aggregate candidate

**Status:** verified semantic/crosswalk granularity gap; possible Lab
refinement, not a standards defect or a conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the federation-management export
represents `RTIambassador::resignFederationExecution` as one aggregate C++ API
surface, `api.2025.cpp.rtiambassador.resignfederationexecution.27297bdf287e`,
and the relevant requirement candidates describe the service and its directive
values separately. The broad §4.12 service candidate
`requirement-candidate-content-clauses-04-federation-management-page-058-l112-33`
states that the action argument directs the RTI to perform zero or more
actions. The nearby directive candidates
`requirement-candidate-content-clauses-04-federation-management-page-058-l125-35`
(directive 1),
`requirement-candidate-content-clauses-04-federation-management-page-058-l147-40`
(directives 1/4/5 assumption search), and
`requirement-candidate-content-clauses-04-federation-management-page-059-l8-1`
(final-federate directive 2) have
empty `transition_ids` and `coverage_kind: cross-cutting`; none supplies a
stable directive-specific semantic facet. The candidate records therefore
cannot distinguish a test of directive 3 cancellation from a test of the
mixed directive 4 delete-then-divest ordering, even though both exercise the
same aggregate API entry point and materially different state transitions.

**Umbra impact:** the resignation contracts retain the immutable aggregate
service/API IDs and list each focused directive regression explicitly. The
standalone voluntary directive 3 cancellation test and the mixed voluntary
directive 4 test are not promoted to directive-specific Lab conformance
claims; their local Catch2 evidence is deliberately labelled as development-
profile coverage until the remaining action variants, packaging evidence, and
protected review are complete.

**Possible Lab refinement:** retain the aggregate candidate and source spans,
but export a stable `ResignAction` value matrix (or child semantic facets)
covering each directive's preconditions, ordering, and postconditions. A
crosswalk should be able to select the aggregate API plus one directive facet
without duplicating source candidates. This would preserve overload/API
identity while making focused transition coverage and gaps machine-readable.

### RL-087 — The special instance-identifier rule was implemented before its exact Lab candidate was selected

**Status:** verified consumer-side traceability mapping gap; possible Lab
refinement, not a Requirements Lab extraction defect or a standards/conformance
finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the exact IEEE 1516.2-2025
reference-data candidate
`requirement-candidate-sections-semantic-clause-4c-page-079-l57-5` states that
`HLAobjectInstanceName` identifies an instance by name and
`HLAobjectInstanceHandle` identifies it by handle. Umbra already enforced the
standardized `HLAunicodeString` / `HLAobjectInstanceHandle` representations and
had a Catch2 regression, but the test-plan entry selected only the neighboring
generic representation candidates from §6.2.15–§6.2.17. The exact §4.14.9
candidate was therefore absent from the machine-readable trace even though the
implementation and test were present. The design and roadmap also described
the exception as future work, which made the implementation state look less
complete than the evidence showed.

**Umbra impact:** a dedicated private contract now selects the exact candidate,
and the composition test-plan entry includes it. The validation design and
roadmap now describe the rule as implemented while keeping the evidence at
private-preflight scope; no public FOM conformance claim is inferred.

**Possible Lab refinement:** allow a traceability checker to flag a test whose
description explicitly names a normative exception but whose selected
candidate set contains only neighboring generic records. A source-span-aware
semantic-group check could suggest the exact candidate without changing the
immutable extraction records.

### RL-088 — Cross-service callback ordering is a derived obligation rather than an exported relationship

**Status:** verified export-shape limitation; possible Lab refinement, not a
Requirements Lab extraction defect or a conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the regional interaction candidates
and the ordinary Time Advance Request / Next Message Request candidates are
exported as separate requirement and API records. The bundle does not expose a
relationship stating that an overlap-qualified timestamped `Receive Interaction`
or `Reflect Attribute Values` callback must precede the receiving federate's
ordinary TAR or NMR grant. The regional TAR/NMR regressions therefore had to
hand-compose the interaction DDM records `...page-236-l152-45` and
`...page-239-l150-43`, and the object-management records
`...page-120-l83-22`, `...page-121-l36-10`, `...page-122-l85-25`, and
`...page-122-l103-31`, with the time-management records
`...page-184-l91-26`, `...page-185-l17-5`, `...page-194-l21-3`,
`...page-194-l69-19`, `...page-197-l20-3`, and `...page-197-l50-13`.

**Umbra impact:** the paired contracts and Catch2 plan now make that derived
composition explicit, and the tests prove the callback-before-grant ordering
at the inclusive timestamp frontier for both payload families. The linkage
remains Umbra's scenario derivation, not a Lab-provided composite behavior or
conformance claim.

**Possible Lab refinement:** add an optional, clearly non-normative composite
scenario layer that can link actors, queued work, callback events, grant
frontiers, and ordering constraints across multiple source requirements and
API surfaces. Preserve the existing immutable records and keep the composite
relationship distinct from normative clause ownership and API mappings.

### RL-089 — New focused lanes need an explicit traceability-label bridge

**Status:** verified consumer-side focused-lane integration gap; possible Lab
and tooling refinement, not a Requirements Lab extraction defect or a
standards/conformance finding.

While adding the default-source timestamped attribute-update companion, the
Catch2 test carried the exact
`timestamped-default-region-attribute-update` tag and the three selected
contracts all passed their direct `requirements_lab.py check` tests. The CTest
catalog nevertheless had no Requirements-Lab or API-contract checks under that
new lane label: the existing traceability registrations exposed only the
generic `requirements-lab`, `api-contract`, `ddm`, and `time-management` labels.
`tools/verify_ctest_service_lanes.py --lane
timestamped-default-region-attribute-update` therefore reported a missing
Requirements-Lab check and API-contract check even though the contracts were
valid. The gap was in the consumer's CMake label bridge, not in the pinned Lab
records.

**Umbra impact:** the lane now adds anchored CMake labels for the default-region
requirements check and the timestamped regional attribute-update requirement
and API checks, and registers a dedicated service lane. Its audit reports one
Catch2 behavior case, three Requirements-Lab checks, and one API-contract check;
the exact behavior plus all traceability checks run together without including
neighboring timestamped families.

The same consumer-side edge was rechecked when the focused
`timestamped-regional-attribute-update` lane was added for the explicit-source
association-replacement regression. The pinned regional requirements and API
contracts were valid without Lab changes; the CMake bridge now labels both
checks explicitly, and the lane audit reports four Catch2 cases, two
Requirements-Lab checks, and one API-contract check. This is a repository
catalog/labeling concern, not a new Lab extraction defect.

The delayed-subscription extension exposed the same routing concern rather
than a new source defect: its regional requirements contract and the existing
support-switch API contract both passed directly, while the new focused lane
needed explicit labels for the delayed-subscription requirements and API
checks. The regional interaction companion uses the same bridge; the resulting
audit reports six Catch2 cases, two Requirements-Lab checks, and one
API-contract check. The pinned Lab export still has the RL-018 stale
title/clause metadata; no additional immutable candidate was needed.

The asynchronous regional-interaction companion exposed the composite-lane
variant of the same concern. Its behavior selects both the asynchronous-
delivery contracts and the regional-interaction contracts; without an explicit
CMake bridge, the asynchronous lane would have run the behavior while omitting
the regional traceability checks. The bridge now labels the exact
`interaction_region_(requirements|api)_traceability` tests for that lane while
leaving the standalone regional service lane intact. The audit reports seven
Catch2 cases, six Requirements-Lab checks, and three API-contract checks for
the asynchronous lane; the immutable Lab records remain unchanged.

The explicit-source timestamped regional-interaction re-enable companion
rechecked the same edge with a new composite lane. Its behavior selects the
timestamped regional-interaction, temporal-role, and Convey Region Designator
Sets contracts; anchored CMake labels now keep those checks in the lane instead
of relying on the generic `time-management` or `ddm` labels. The focused audit
reports one Catch2 case, five Requirements-Lab checks, and two API-contract
checks, all passing. This is the same consumer-side routing limitation rather
than a new Lab extraction defect; the pinned bundle and immutable source
records remain unchanged.

The default-source timestamped object-update re-enable companion exposed the
same routing edge for the default-region, timestamped-attribute, and temporal-
role contracts. Anchored CMake labels now place those checks in their dedicated
lane; its audit reports one Catch2 case, five Requirements-Lab checks, and two
API-contract checks, all passing. Again, this is a consumer-side catalog/label
bridge limitation, not a new Requirements-Lab extraction defect.

The explicit-source regional timestamped object-update re-enable companion
rechecked the same edge with the object-region, timestamped-attribute, and
temporal-role contracts. Its anchored CMake bridge also keeps the regional
requirements/API checks in the focused lane; the audit reports one Catch2 case,
six Requirements-Lab checks, and three API-contract checks, all passing. The
source RegionHandle and callback-order assertions are Umbra's bounded scenario
composition; the pinned Lab records remain unchanged, so this is not a new Lab
extraction defect.

The class-level timestamped regional Request/Provide response companion
rechecked the same edge across the regional request, timestamped regional
attribute, time-role, time-advance, order-type, Convey Region Designator Sets,
and support-switch contracts. The behavior selects the official provider
callback, queues its timestamped response for a constrained requester, and
checks reflection-before-grant plus source-region/retraction metadata. Its
anchored CMake bridge keeps the composite contract checks in the focused lane;
the audit reports one Catch2 case, twelve Requirements-Lab-labelled checks,
and six API-contract checks, all passing. The twelve traceability checks are
the six requirements contracts plus their six API companions; the pinned Lab
records remain unchanged:
this is the same consumer-side label-routing limitation, not a new Lab
extraction defect. The scenario intentionally does not claim automatic
provision, alternate advances, or full timestamped/retraction semantics.
The first CTest audit also caught a local cataloging mistake: the new behavior
case carried its regional timestamped tag but not the focused lane tag, so the
traceability checks were present while the lane had no Catch2 behavior member.
Adding the explicit behavior tag and rerunning the tag verifier closed that
consumer-side gap; it did not require a Lab change.

**Possible Lab/tooling refinement:** let the Catch2 test-plan entry declare its
focused lane and selected contract IDs, then generate the CTest traceability
labels from that metadata. A catalog check should flag a behavior tag with no
matching contract checks before a lane is advertised, while retaining an
explicit override for composite cross-service lanes.

### RL-090 — Direct timestamped interaction API surfaces need a family contract

**Status:** verified consumer-side traceability coverage gap; possible Lab and
artifact-generation refinement, not a standards or conformance finding.

The pinned Lab API export already supplied the official direct timestamped
`Send Interaction` and `Receive Interaction` surface IDs
(`api.2025.cpp.rtiambassador.sendinteraction.dd73a76182aa` and
`api.2025.cpp.federateambassador.receiveinteraction.06a1022b8731`), and the
Catch2 plan selected them for several timestamped scenarios. Umbra nevertheless
had only the requirements contract for the direct timestamped interaction
family; its API contract covered the separate `Send Interaction With Regions`
overload. That left a real default-region interaction lane without a direct
family API check even though the official declarations were available.

**Umbra impact:** added
`compliance/timestamped-interaction-api-contract.json`, registered its
Requirements-Lab CTest check, and included it in the dedicated
`timestamped-default-region-interaction` lane. The lane now reports one real
Catch2 behavior case, eleven Requirements-Lab checks, and five API-contract
checks.

**Possible Lab/tooling refinement:** when a test-plan entry selects an API
surface family that has no neighboring contract, generate or flag the missing
family contract instead of allowing a requirements-only slice to appear
complete.

### RL-091 — Catch2 plan identifiers need an explicit uniqueness check

**Status:** verified consumer-side catalog hygiene issue; corrected locally,
not a Requirements Lab extraction or standards finding.

The final catalog audit found two unrelated encoding entries using the same
`umbra-cpp-hla-variable-array-encoding-unit` plan identifier: one combined the
HLAunicodeString/MIM fixed-record case, while the other described the generic
HLAvariableArray implementation. The existing CTest catalog check did not
reject the collision, so the ambiguity was visible only when the plan was
parsed and its IDs were counted directly.

**Umbra impact:** renamed the former entry to
`umbra-cpp-hla-unicode-string-mim-array-encoding-unit`; the plan now has 335
unique test identifiers and all catalog checks remain green.

**Possible Lab/tooling refinement:** validate plan identifier uniqueness as a
first-class catalog check and report the colliding entries with their test
case/source metadata.

### RL-092 — Composite class-response lanes need explicit cross-contract routing

**Status:** verified consumer-side focused-lane integration gap; possible Lab
and tooling refinement, not a Requirements Lab extraction defect or a
standards/conformance finding.

The object-class Request Attribute Value Update response companion combines the
class-designator request and Provide callback with the **timestamped**
Update/Reflect and Retract surfaces plus the temporal-role, time-advance, and
order-control boundaries. The first wiring pass exposed a consumer-side
traceability hazard: the test was timestamped, but its plan and response
contract initially pointed at the non-timestamped Update/Reflect IDs. The Lab
correctly accepted both contracts because each reference was individually
valid; only the scenario/API semantic review caught the family mismatch. The
focused lane now uses a dedicated timestamped attribute-update API contract,
while the non-timestamped response contract remains limited to its original
object-instance scenario. Each individual contract passes its direct
`requirements_lab.py check`, but no single generic domain label represents that
composite behavior. The focused lane therefore requires anchored CMake labels
for the object-class request, timestamped attribute-update, time-role,
time-advance, and order-type contracts, in addition to the explicit Catch2
behavior tag.

**Umbra impact:** the new `object-class-request-provider-response` lane keeps
the class-expansion behavior and all ten traceability checks together. Its
focused audit reports one Catch2 case, ten Requirements-Lab-labelled checks,
and five API-contract checks, all passing. The lane intentionally remains a
development-profile traceability slice and does not claim automatic provision,
alternate advances, broader DDM, transport, or conformance. The pinned Lab
records and immutable API IDs remain unchanged.

**Possible Lab/tooling refinement:** allow a Catch2 plan entry to declare a
composite lane and selected contract IDs, then generate the CTest label bridge
and reject a lane whose behavior tag or selected contract family is missing.

### RL-093 — Forced-loss `NO_ACTION` behavior needs an explicit bounded policy

**Status:** verified consumer-side semantic boundary; not a Requirements Lab
extraction defect or a standards/conformance finding.

The Lab exposes the Connection Lost, automatic-resign directive, and support
service records independently, but it does not provide one executable
scenario that resolves how a disconnected federate's owned attributes behave
when its configured action is `NO_ACTION`. The embedded runtime therefore
records a bounded development-profile policy in its contracts: forced loss
retains the known object, divests the departed member's owned attributes,
offers those attributes to an eligible survivor, and does not emit an
automatic Remove Object Instance callback. The new Catch2 case sets and reads
the official directive before the fault and verifies each of those effects.

**Umbra impact:** the policy is now pinned to the Connection Lost and support
switch requirement/API contracts and has its own `connection-lost-automatic-resign`
CTest lane. The case remains source/test traceability for the embedded
development profile; it does not claim that the Lab's individual records by
themselves establish the complete forced-resignation disposition matrix,
remote transport behavior, protected review, or conformance.

**Possible Lab/tooling refinement:** provide a cross-cutting Connection Lost
scenario record (or an explicit policy-extension field) that links the
directive value to ownership, object-lifetime, and callback outcomes while
preserving the distinction between extracted standard requirements and
implementation-selected development-profile policy.

### RL-094 — Final-federate forced loss is not linked to the Connection Lost path

**Status:** verified cross-cutting traceability gap; possible Lab refinement,
not a Requirements Lab extraction defect or a standards/conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the final-member rule is exported
as `requirement-candidate-content-clauses-04-federation-management-page-059-l8-1`
(`clause-4.12.4`, `coverage_kind: cross-cutting`, with no transition IDs),
while Connection Lost and the automatic-resign continuation are separate
records: `req-federate-connection-lost`, `req-connection-lost-service-behavior`,
and `requirement-candidate-content-clauses-04-federation-management-page-050-l36-11`.
None of those immutable records says how the final-federate directive-2 rule
composes with a transport fault when the disconnected member is the last
joined federate. The ordinary voluntary final-federate test therefore cannot
serve as evidence for the forced-loss path, and selecting all four records
individually would still leave the scenario relationship implicit.

**Umbra impact:** the new embedded case sets the official automatic directive
to `NO_ACTION`, faults the sole member, verifies the Connection Lost callback,
then rejoins and reserves the deleted object's name. Its dedicated
`connection-lost-final-federate` lane keeps this matrix cell independently
runnable. The case remains development-profile source/test traceability; it
does not claim that the separate Lab records establish the complete forced-
resignation matrix, remote transport, protected review, or conformance.

**Possible Lab/tooling refinement:** add a cross-cutting scenario relation (or
policy-extension facet) that can bind a final-membership precondition to the
Connection Lost transition and the mandated directive-2 object-lifetime
outcome, without changing the immutable clause/API records or presenting the
consumer's scenario as a new normative requirement.

### RL-095 — Forced loss does not compose negotiated ownership cancellation

**Status:** verified cross-cutting traceability gap; possible Lab refinement,
not a Requirements Lab extraction defect or a standards/conformance finding.

At the same pinned revision, the Lab exports the Connection Lost and automatic-
resign records independently from the regular acquisition and negotiated
divestiture records. The relevant immutable candidates are
`req-federate-connection-lost`, `req-connection-lost-service-behavior`,
`requirement-candidate-content-clauses-04-federation-management-page-050-l36-11`,
`requirement-candidate-content-clauses-07-ownership-management-page-161-l137-40`,
`...page-161-l143-42`, `...page-156-l8-1`, `...page-156-l20-5`,
`...page-155-l173-48`, and `...page-169-l24-3`. They do not provide a
scenario relation for a requester that has already caused a regular owner
release and selected negotiated-divestiture confirmation work when that same
requester is disconnected under `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`.
Checking the records one at a time would not establish that both queued owner
callbacks become stale before ownership is changed.

**Umbra impact:** the new embedded case starts that two-stage ownership
transition, faults the requester, and verifies that neither
`Request Attribute Ownership Release` nor `Request Divestiture Confirmation`
is delivered after forced cleanup. The dedicated
`connection-lost-negotiated-cancellation` lane keeps this forced-resign matrix
cell independent of the regular acquisition and final-member lanes. It remains
development-profile source/test traceability, not a complete ownership
arbitration, remote transport, protected-review, or conformance claim.

**Possible Lab/tooling refinement:** add a cross-cutting scenario/policy facet
that binds an automatic-resign directive to pending regular and negotiated
ownership state, stale callback suppression, and the surviving owner, while
keeping the individual clause/API records immutable and normative ownership
separate from the consumer's selected forced-loss policy.

### RL-096 — Federate Lost fan-out and per-recipient callback order are not modeled together

**Status:** verified cross-cutting traceability gap; possible Lab refinement,
not a Requirements Lab extraction defect or a standards/conformance finding.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the report candidates
`requirement-candidate-content-clauses-04-federation-management-page-050-l18-5`
and `...page-050-l21-6` require `HLAreportFederateLost` delivery and the lost
time-regulating federate's last-known time, while
`req-federate-connection-lost`, `req-connection-lost-service-behavior`, and
`requirement-candidate-content-clauses-04-federation-management-page-050-l36-11`
describe the Connection Lost transition and its automatic-resign continuation.
The immutable records do not express a recipient set or a per-recipient queue
relationship that says the RTI-originated report is submitted before each
survivor's automatic `Remove Object Instance` consequence. They also do not
define a global ordering between independent survivor callback queues; those
queues are separate execution contexts and should not be collapsed into one
cross-federate sequence.

**Umbra impact:** the new embedded case uses one lost time-regulating publisher
and two subscribed survivors. It selects `DELETE_OBJECTS`, faults the
publisher, and drains each survivor one callback at a time. Both survivors see
one `HLAreportFederateLost` interaction before their own automatic removal,
with identical fault payloads and the local default-invalid RTI producer
representation. The test deliberately makes no assertion about which survivor
callback runs first globally. This is development-profile source/test
traceability, not a complete MOM fan-out, remote transport, protected-review,
or conformance claim.

**Possible Lab/tooling refinement:** add a cross-cutting fan-out relation with
an explicit recipient set and a per-recipient callback queue/order edge. Keep
global cross-federate ordering unspecified unless a source clause defines it;
generate the same report-before-cleanup scenario for one and multiple eligible
survivors while preserving the immutable clause/API records.

### RL-097 — Directed selector mutation after Connection Lost is not a Lab relation

**Status:** verified cross-cutting traceability gap; possible Lab refinement,
not a Requirements Lab extraction defect, standards finding, or conformance
evidence.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the directed-subscription selector
and its callback-time re-evaluation are exported independently through
`requirement-candidate-content-clauses-05-declaration-management-page-101-l104-29`
and `...page-101-l116-33`. The Connection Lost cutoff and automatic-resign
continuation are separate records, including
`requirement-candidate-content-clauses-04-federation-management-page-050-l27-8`
and `...page-050-l36-11`. Those immutable records do not express the compound
case in which a timestamped directed interaction is already queued for a
universal recipient, the source then faults, and the recipient changes to the
by-ownership selector before its callback boundary. Selecting the records one
at a time does not establish whether the stale directed callback is suppressed
without stranding the separately reserved automatic object removal.

**Umbra impact:** the new focused case starts with a universal constrained
recipient, faults the target-owning publisher under `DELETE_OBJECTS`, changes
the recipient to by-ownership, and proves that the directed callback is
suppressed while the target remains known through the time-6 grant. A later
receive-order gate delivers the independent automatic removal. This remains
embedded development-profile source/test traceability; it does not claim
directed DDM, multiple-class selector cardinality, remote transport, package
or protected-review evidence, or conformance.

**Possible Lab/tooling refinement:** add a cross-cutting selector/lifecycle
relation that binds the Connection Lost cutoff to the directed recipient's
current selector and the independent automatic-cleanup reservation. The
relation should distinguish selector suppression from target departure and
leave the immutable clause/API records unchanged.

### RL-098 — Regional selector mutation after Connection Lost is not a Lab relation

**Status:** verified cross-cutting traceability gap; possible Lab refinement,
not a Requirements Lab extraction defect, standards finding, or conformance
evidence.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, regional overlap and callback-time
re-evaluation are exported through records such as
`requirement-candidate-content-clauses-09-data-distribution-management-page-230-l142-43`,
`...page-233-l48-13`, and `...page-235-l100-28`; region mutation is exported
separately through `...page-217-l92-30` and `...page-226-l162-43`. Connection
Lost cutoff and automatic cleanup remain separate records,
`requirement-candidate-content-clauses-04-federation-management-page-050-l27-8`
and `...page-050-l36-11`. Those immutable records do not express the compound
case in which a timestamped regional attribute update is queued for an
overlapping constrained recipient, the source then faults, and the recipient
commits a disjoint region before its callback boundary. Selecting the records
one at a time does not establish whether the stale regional reflection is
suppressed without stranding the separately reserved automatic object removal.

**Umbra impact:** the new focused case creates source and receiver regions with
strict overlap, queues a timestamp-6 attribute update, faults the publisher
under `DELETE_OBJECTS`, commits the receiver to `[3, 4]`, and proves that no
regional reflection is delivered at the time-6 grant. A later receive-order
gate delivers the independent automatic removal. This remains embedded
development-profile source/test traceability; it does not claim regional
interaction DDM, alternate advances, advisory scope transitions, remote
transport, package or protected-review evidence, or conformance.

**Possible Lab/tooling refinement:** add a cross-cutting selector/lifecycle
relation that binds a Connection Lost cutoff to the recipient's current region
projection and the independent automatic-cleanup reservation. The relation
should distinguish regional overlap suppression from object departure and
leave the immutable clause/API records unchanged.

### RL-099 — Update-rate reduction is exported as fragments without a delivery relation

**Status:** verified cross-cutting traceability gap; possible Lab refinement,
not a Requirements Lab extraction defect, standards finding, or conformance
evidence.

The pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615` exports the update-rate behavior in
separate immutable candidates: the maximum-rate eligibility rule
(`requirement-candidate-content-clauses-06-object-management-page-112-l75-22`),
the best-effort excess-message rule
(`...page-112-l90-27` and `...page-112-l93-28`), the wall-clock spacing rule
(`...page-112-l99-30`), the reliable no-drop rule
(`...page-112-l105-32`), and the producer/subscriber rate-difference rule
(`...page-112-l138-43`). None of those records binds a subscription's retained
designator and FDD rate to the effective producer rate, transportation type,
recipient eligibility, and callback spacing as one testable delivery relation.
The existing update-rate contract therefore correctly stops at metadata and
lookup behavior; it cannot by itself express a throttling scenario.

**Umbra impact:** this gap led to a dedicated contract and real Catch2
scenarios that distinguish best-effort dropping from reliable retention and
exercise wall-clock delivery spacing. The sibling verification scenario was
used only as a non-normative test shape; it was not imported as evidence or
treated as a substitute for the official C++ requirements. The resulting
evidence remains development-profile traceability, not an update-rate or
conformance claim.

**Possible Lab/tooling refinement:** emit a cross-cutting update-rate delivery
relation with explicit producer rate, subscribed maximum, transportation type,
passel eligibility, and timing/drop outcome fields. Keep the immutable clause
records as source anchors while allowing one relation to generate separate
best-effort and reliable scenarios.

### RL-100 — Update-rate delivery relation was missing at the runtime boundary

**Status:** verified implementation boundary and local mitigation; not a
Requirements Lab extraction defect or conformance evidence.

At the pinned Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the update-rate candidates remain
the authoritative anchors: `requirement-candidate-content-clauses-06-object-
management-page-112-l75-22`, `...page-112-l90-27`, `...page-112-l99-30`,
`...page-112-l105-32`, and `...page-112-l138-43`. Before the update-rate slice,
`EmbeddedFederationRegistry::planReceiveOrderAttributeUpdate` selected
recipients and `UmbraRtiAmbassador::queueReceiveOrderAttributeUpdate` queued
each eligible passel immediately. The retained subscription designator was
available for lookup, but there was no recipient/attribute last-delivery
timestamp or wall-clock eligibility gate. Consequently, the implementation
could not prove spacing, best-effort suppression, or reliable no-drop behavior.

**Umbra impact:** the private `UpdateRateGate` supplies an internal
clock-injectable test seam and per-stream admission state without changing the
official API. It bypasses reliable transportation and is independent of
logical timestamps; integration with recipient delivery and FDD-derived rates
is present for both bounded non-timestamped and timestamped attribute callback
paths. Timestamped best-effort suppression closes the pending retraction state,
while reliable passels bypass the gate. Admission is keyed per projected
attribute, and subscription mutations advance a federation-owned generation key
so a later unsubscribe/resubscribe cannot inherit stale wall-clock history.
The bounded two-federate TSO case now supplies end-to-end subscriber-boundary
evidence for one active FDD rate and both reliable/best-effort transport
classes. A generated cross-cutting Lab relation and a complete producer-rate /
subscriber-rate timing matrix remain pending.

**Possible Lab/tooling refinement:** expose a generated relation joining the
subscription designator, FDD rate, transportation reliability, producer rate,
and delivery/drop timestamps so the five fragmented candidates can generate a
single deterministic scenario family.

### RL-101 — Timestamped ownership transition is not an official service

**Status:** verified planning gap; not a Requirements Lab extraction defect,
standards finding, or conformance evidence.

The official 2025 C++ binding exposes ownership-management services without a
`LogicalTime` argument; timestamped ownership transition is therefore not an
API that Umbra should invent. The pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615` does export ownership-management and
timestamped-delivery material as separate candidate families, but it does not
provide one relation for the *ownership effects* of a timestamped object
update, including source ownership, recipient eligibility, callback order, and
retraction or resignation outcome.

**Umbra impact:** do not add a fictional timestamped ownership API. Any future
slice will test timestamped object-update behavior across ownership changes,
using only the official ownership services and existing TSO update APIs. No
timestamped ownership-transition conformance claim is made.

**Possible Lab/tooling refinement:** emit a cross-family relation for timestamped
object updates whose source ownership changes before callback delivery, with
explicit source/target ownership state, logical time, grant/retraction boundary,
and resignation disposition fields.

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
