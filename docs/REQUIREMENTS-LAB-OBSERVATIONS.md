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
value under the IEEE 1516.1-2025 Table 8 `Static` decision, but does not yet
expose the value through public MOM discovery/reflection. The contrary MIM
field remains recorded rather than silently discarded. The filesystem and
snapshot lifecycle tests are private source-level traceability only; they are
not promoted to Lab validation or conformance results.

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
file record. It will not enable generated report-record appends until a
source-backed, per-service return mapping is established.

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

A direct source pass over §11.1, §11.2, and §11.4 confirms rather than
resolves the tension: §11.1 says that MOM access/interchange uses predefined
HLA objects and interactions in the same way as participating federates;
§11.2 requires the RTI to publish and register the `HLAmanager.HLAfederate`
instances; and §11.4 directs the RTI to update those instances with their
private federate points. None supplies an exception or an RTI-to-joined-
federate mapping for the §6.9 callback argument. This must not be inferred
from the represented federate's `HLAfederateHandle`.

**Umbra impact:** Umbra will not copy the sibling prototype's unjoined
`FederateHandle(0)` sentinel into its public callback path. A private,
registry-owned joined-federate MOM snapshot may reserve a common object
identity, retain complete MIM metadata, an immutable point, and encoded
initial values, but it is deliberately outside public object discovery and
reflection. The complete public RTI-owned MOM-object lifecycle,
requested-value work, and callback behavior remain pending a source-backed
producer-designator rule. The private service-report routing plan now models
its origin explicitly as RTI-owned rather than using a numeric sentinel in the
ordinary federate-sender eligibility helper; that model still cannot be
passed to the public `Receive Interaction` callback queue.

**Possible Lab refinement:** add a cross-clause MOM implementation note or
relationship that records the applicable producer-designator rule (including
any explicit RTI exception) for RTI-registered object instances. The packet
should preserve the two ordinary requirements and identify whether a further
source controls their interaction, rather than letting consumers invent a
handle sentinel or silently substitute the represented federate's designator.

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
