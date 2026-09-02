# HLA Requirements Lab observations

This is Umbra's decision log for observations about the adjacent HLA
Requirements Lab corpus and its exported `CorpusBundle`. It deliberately
separates verified data facts from proposed Lab refinements. An observation is
not a claim that the Lab, IEEE source, or Umbra is non-conformant.

## Pinned input

- Lab repository: `../Document-Recreation`
- Reviewed release: `v0.1.0.a1` (`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`)
- Umbra lock: `compliance/requirements-lab/requirements-lab.lock.json`
- Editions exported: 2010 and 2025
- Last reviewed: 2026-08-26

Umbra consumes only the Lab's exported portable JSON bundle. The generated
bundle under `.compliance/` is intentionally ignored, so each observation
below names the pinned revision and durable source identifiers rather than
depending on an uncommitted export artifact.

The current synchronization target is the `v0.1.0.a1` all-edition export. It
contains six documents: 2010 and 2025 editions of Parts 1, 1.1, and 1.2. The
2025 Part 1.1 corpus now exports 1,860 requirements (up from 1,697 in the
historical revision cited by the older observations below). Existing Umbra
contracts remain implementation-scoped to 2025; the 2010 documents are
included in the portable bundle and lock so their numbering remains available
for future edition-specific contracts.

### Recurrence-ledger decision — 2026-08-25

The first 157 entries in this file are the immutable historical observation
baseline; they are not reused when an old problem comes back. A go-back of a
Requirements Lab or Umbra consumer issue after a fix was expected—including a
finding that the issue was never actually closed—must be appended under the
next unused `RL-###` identifier, cite the earlier observation, and record the
current reproduction and mitigation state. This is local observation
numbering, not a renumbering of the Lab's immutable requirement/API IDs.

An unresolved old issue is still a recurrence when the workflow reaches it
again. The absence of a Lab export change, or the fact that the original
mitigation was incomplete, does not permit folding the new reproduction back
into RL-001 through RL-157; it must receive the next post-157 identifier.

The current audit leaves RL-172 through RL-176 as the earlier recorded
post-RL-157 go-backs: RL-172 cites RL-156, RL-173 through RL-175 are the
distinct resignation/lifetime consumer recurrences, and RL-176 records the
Catch2-plan selector recurrence linked to RL-160. RL-158 through RL-171 are
audit or coverage records, not hidden recurrences. The export-only r15 2025
resync found no additional Lab-content recurrence; the later local plan guard
did reproduce the RL-160 consumer go-back and consumed RL-176. RL-177 records
the separate 2025 source-artifact tension, and RL-178 records the subsequent
focused-lane traceability recurrence linked to RL-160/RL-176. RL-179 records
the subsequent native Catch2 selector recurrence, so RL-180 is now the next
available identifier for a future post-RL-157 go-back. An unchanged
export, additive test coverage, or a local fixture/API setup correction does
not consume the next identifier unless it reproduces a Requirements Lab or
Umbra consumer defect.

Earlier dated audit and plan notes were written while RL-177 or RL-178 was
still available and may therefore say that one of those identifiers was
“reserved.” Those statements are historical snapshots, not current allocation
instructions. RL-177 now holds the separate source-artifact tension, RL-178
holds the focused-lane traceability recurrence, and RL-179 holds the native
selector recurrence recorded below, so RL-180 is the next unused post-RL-157
identifier. A newly reproduced old issue must use RL-180 (or the next unused identifier), cite its earlier observation, and
include the current reproduction and mitigation state; RL-001 through RL-157
remain immutable and must never be reused.

### 2026-08-24 re-sync audit

A fresh export from the adjacent checkout at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` was compared with the existing
ignored `.compliance/corpus-bundle.json`. The 2025 requirements, API surfaces,
mappings, immutable IDs, clause IDs, and record ordering are unchanged. The
fresh export did contain 32 state-machine `source_state`/`target_state` field
changes in three 2010 Part 1.1 figures, caused by uncommitted parser/semantic
changes in the adjacent Lab checkout; the refreshed ignored bundle now records
those fields. No 2025 requirement-number rewrite is warranted. The 241
checked-in `*-contract.json` files (145 requirements references, 95 API
references, and one implementation contract), plus the API baseline, therefore
continue to resolve without ID or clause-number edits. The older
`4f012fb1c21367cfde67aab8498ae00e2a64c615` citations below remain historical
observations; they are not silently relabelled as current 2025 export facts.
The preceding lock migration had already normalized 295 exported clause IDs
across 40 requirements contracts; this audit found no additional 2025
clause-number drift.

As a second check, the adjacent checkout's current working tree was exported
without forcing the pinned revision and compared with the refreshed ignored
bundle. The six document inventories still have the same requirement counts,
immutable IDs, ordinals, clause IDs, API-surface IDs, and mapping IDs; the
2025 Part 1.1 counts remain 1,860 requirements, 1,263 API surfaces, and 282
mappings. This confirms that the apparent numbering concern is not a current
2025 renumbering event. The working-tree export remains an audit artifact only
and does not replace the pinned lock or the canonical contract baseline.

The Lab checkout was not clean during this export. The exporter accepts a
revision label but reads the working tree, so a dirty checkout can change
derived transition metadata without changing the locked commit hash. Umbra
records the working-tree condition and the field-level delta in RL-157 rather
than treating the revision label alone as a content digest.

A subsequent export from the same working tree (the local audit artifact
`.compliance/corpus-bundle-resync-2026-08-24-r5-2025.json`) reproduced this
result. `requirements_lab.py resync --edition 2025 --fail-on-diff` reports one
changed document because of those fifteen added state-chart transitions plus
the one API surface and one mapping; it reports no missing content and no
renumbered records. All 241 checked-in `*-contract.json` files resolve cleanly
against that candidate. The candidate remains non-canonical: the pinned
`.compliance/corpus-bundle.json` and lock file are intentionally unchanged.

The audit also found a duplicate local observation heading, `RL-022`. The
temporal state-chart label observation is now `RL-155`; existing cross-
references to the Turn Updates On and automatic-resign observations remain
stable. The checker now exercises the duplicate-heading guard instead of
silently accepting malformed observation numbering.

The configured Debug multi-configuration CTest lane was then run by test name
against the fresh bundle: all 235 Requirements-Lab traceability tests passed.
Invoking CTest without `-C Debug` does not execute these tests; CTest reports
them as `Not Run` because the generated Visual Studio test file has
configuration-qualified entries. This is a tooling usability edge, not
requirements drift, and future Lab refresh checks should use the explicit
configuration (or a single-configuration generator).

### 2026-08-25 re-sync audit

The follow-up 2025 export (`r11`) was compared with the prior `r10` audit
candidate at the same locked revision. `requirements_lab.py resync --edition
2025 --fail-on-diff` reports `changed_documents: 0`: all three 2025 document
digests, requirement/API/mapping/transition inventories, immutable IDs, and
clause bindings are unchanged. The exported inventories remain 20 Part 1,
1,860 Part 1.1, and 340 Part 1.2 requirements; the dirty working-tree
overlay remains 1,264 Part 1.1 API surfaces, 283 mappings, and 297
transitions. This is routine audit confirmation, not a new Requirements Lab
issue and not a requirement-numbering event.

The same-day r12 export was then generated directly from `../Document-Recreation`
and compared with r11. It reports `changed_documents: 0` and identical SHA-256
content for all three 2025 documents: 20/1,860/340 requirements, 1,264 API
surfaces, 283 mappings, 297 transitions, and 1,894 requirement/API bindings.
No requirement, ordinal, clause ID, API-surface ID, mapping ID, or pre-existing
transition was added, removed, renumbered, or content-replaced. The normal
Requirements Lab check passes against r12, so no contract or test-plan remap is
warranted. Per the recording rules below, this unchanged expected resync does
not consume RL-176; a future reproduced Lab/consumer defect must use that next
post-157 identifier and cite its earlier observation.

The recurrence audit also rechecked the post-RL-157 rule: RL-172 remains the
confirmed FOM consumer-semantics recurrence linked to RL-156. RL-173 and RL-174
are distinct object-deletion and regional-interaction lifecycle records, and
RL-175 is the adjacent regional-attribute recurrence; none claims that the Lab
renumbered anything. An unchanged export alone must not consume an observation
number; RL-175 consumes one because a reproducible Umbra consumer defect was
found while adding the new coverage. At the r12 audit point, the local Catch2
plan contained 556 entries, all reusing current 2025 requirement and C++ API
IDs. A later clean default-region resignation companion is recorded below as
local coverage growth, bringing the plan to 557 entries without consuming an
observation number.

The regulation-role companion was a clean implementation slice rather than a
new Requirements Lab complaint: an accepted queued timestamped interaction
survived producer Disable Time Regulation and callback-gated re-enable at the
same lookahead, then arrived once before the recipient grant. No prior issue
was re-exposed, so that slice consumed no observation. This distinction is
intentional: a new Catch2 plan entry is local coverage growth, while a new
post-RL-157 observation is reserved for a reproducible Lab or consumer issue;
the regional-resignation regressions are recorded as RL-174 and RL-175 below.

The direct timestamped-directed TAR/NMR companion was likewise a clean local
coverage slice. The r11 export still reports the same immutable requirement and
API records, and the native runtime delivered one target-qualified timestamped
callback before each direct TAR(7) and NMR(10) grant. No Lab or Umbra defect was
reproduced, so this coverage addition consumed no observation.

The same-day r13 export was generated directly from `../Document-Recreation` and
compared with r12. It again reports `changed_documents: 0` at revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`, with the same 20/1,860/340
requirements, 1,264 API surfaces, 283 mappings, 297 transitions, and 1,894
requirement/API bindings. The new regional declaration-relevance slice reuses
those immutable IDs and adds one local Catch2 plan entry (559 total). The
follow-on tag-free Cancel Negotiated Attribute Ownership Divestiture MOM slice
reuses the same immutable IDs and adds one more local plan entry (560 total);
its lock-free HLA_IMMEDIATE report route also reproduced no Lab or
runtime-semantics defect, so RL-176 remains unused.
The first DDM regression pass did expose stale local test assumptions that
regional active subscriptions do not enqueue declaration-relevance callbacks;
the affected timing/service-report fixtures now suppress that independent
callback stream explicitly, while the dedicated regional-advisory case keeps
it enabled. This is an Umbra test-fixture correction, not a Lab recurrence or
numbering event.

The timestamped `Retract` public-MOM companion is likewise clean local
coverage: the accepted designator is reported through an `HLA_IMMEDIATE`
observer before the separate `Request Retraction` callback is evoked, with
the official type-4 service and type-33 supplied argument. The production file
remains unchanged while file reporting is disabled. The r13 export still
reports unchanged 2025 identifiers, and the local Catch2 plan grows from 560
to 561 entries; no Requirements Lab or Umbra defect was reproduced, so RL-176
remains unused.

The follow-on multiple-object-instance-name release MOM companion was also
clean local coverage. It reuses the unchanged r13 requirement/API identifiers,
performs an atomic §6.7 release, and delivers one type-54 `StringSet`
`HLAreportServiceInvocation` to an HLA_IMMEDIATE observer after the registry
mutation and outside native locks. The focused case passes 55 assertions and
the local Catch2 plan grows from 561 to 562 entries. No Requirements Lab or
Umbra consumer defect was reproduced; RL-176 therefore remains unused. This
is public interaction evidence, separate from the existing private filesystem
record case, and does not promote either lane to validation or conformance.

The accepted `Cancel Attribute Ownership Acquisition` public-MOM companion was
also a clean local slice. It emits one reliable ownership-management service
report after the §7.15 cancellation plan and outside native locks, before the
separately queued confirmation callback, and decodes the official type-3,
type-37, and type-1 forms. The focused case passes 78 assertions and the local
Catch2 plan grows from 562 to 563 entries. The r13 export remains identifier-
stable and `requirements_lab.py check` has no changed Lab record; no Requirements
Lab or Umbra consumer defect was reproduced, so RL-176 remains unused. This is
public interaction evidence only, not Lab validation or conformance.

The ordinary timestamped-interaction fanout companion was also a clean local
coverage slice. It accepts one timestamp-6 interaction for two constrained
recipients, admits independent TAR(6) and NMR(10), then resigns the producer
with `NO_ACTION`; each recipient drains its own callback before its grant and
retains the original producer, payload, tag, timestamp/order, and retraction
metadata. A post-resignation `Retract` correctly stops at the official
membership precondition (`FederateNotExecutionMember`), so this is not a Lab
or Umbra regression. The local plan therefore grows from 552 to 553 entries
without consuming an observation; the post-RL-157 recurrence rule remains
unchanged.

The adjacent timestamped-attribute fanout companion was likewise clean. It
accepts one timestamp-7 `Update Attribute Values` passel for two constrained
recipients, admits direct TAR(7) and NMR(10), resigns the producing owner with
`UNCONDITIONALLY_DIVEST_ATTRIBUTES`, and drains both reflections independently
before their grants with the original producer, payload, tag, timestamp/order,
and retraction metadata. The focused Catch2 case passes 87 assertions; the
plan grows from 553 to 554 entries with no changed Lab record and no
observation. This is local multi-recipient evidence, not a recurrence or a
conformance promotion.

The follow-on single-recipient explicit-source regional-attribute companion was
not clean coverage: its first callback-boundary run reproduced the same
invocation-time regional-scope loss after the producer resigned. The runtime now
uses the committed source-region snapshot and accepted update-region association
only after the producer has left the live membership ledger, while preserving
live association-replacement suppression. The focused case passes 66 assertions
and this new post-RL-157 recurrence is recorded as RL-175, citing RL-174 rather
than rewriting it.

The adjacent default-source/default-region attribute-update companion was clean
coverage. It keeps one timestamped `Update Attribute Values` passel queued while
the producer resigns with `UNCONDITIONALLY_DIVEST_ATTRIBUTES`, then uses an
independent regulator to release the recipient. The callback precedes its
matching grant, preserves the payload/tag/producer/timestamp/order and supplied-
empty callback-region marker, and rejects post-resignation `Retract` with
`FederateNotExecutionMember`. The focused case passes 63 assertions; the local
Catch2 plan grows from 556 to 557 entries. No Requirements Lab or Umbra defect
was reproduced, so this clean adjacent slice does not consume RL-176.

The local `requirements_lab.py check` now enforces the documentation side of
this rule: post-RL-157 identifiers must remain ascending and contiguous, any
post-RL-157 entry using recurrence, re-exposure, reintroduction, reappearance,
reoccurrence, or go-back language must cite an earlier `RL-###` record, and the
duplicate-heading guard continues to protect the historical numbering. This is
a consumer-side audit guard; it does not change the immutable Requirements Lab
identifiers.

The same audit found and corrected a local ledger-formatting defect: the RL-175
post-157 recurrence heading used a plain hyphen instead of the canonical em dash,
so the heading parser did not count it. RL-175 was already the correct new
recurrence linked to RL-174; after normalization the checker recognizes all
RL-158 through RL-175 entries in order. The checker now rejects any future
`RL-###` heading with a non-canonical delimiter. This was a documentation/tooling
defect, not a new Lab or Umbra runtime recurrence, so RL-176 remains reserved.

The follow-up r14 export (`.tmp/corpus-bundle-resync-2026-08-25-r14-2025.json`)
was generated directly from `../Document-Recreation` at the locked revision and
compared with r13. The 2025 export remains unchanged: 20/1,860/340
requirements, 1,264 API surfaces, 283 mappings, 297 transitions, and 1,894
requirement/API bindings, with no missing, added, renumbered, or
content-replaced records. The normal Lab checker also remains green. This is
an expected re-sync, not a recurrence or a numbering event, so RL-176 remains
unused. Any later go-back that reproduces a previously recorded Lab or Umbra
consumer defect must still receive the next post-157 identifier and cite the
earlier record under the recording rule below.

The immediate r15 export (`.tmp/corpus-bundle-resync-2026-08-25-r15-2025.json`)
was generated again from `../Document-Recreation` at the same locked revision
and compared with r14. It reports `changed_documents: 0` with the same
2025 inventories (20/1,860/340 requirements, 1,264 API surfaces, 283
mappings, 297 transitions, 1,894 requirement/API bindings, and 1,410 API
crosswalks). The scoped bundle check passes, and no requirement, ordinal,
clause ID, API-surface ID, mapping ID, transition, or binding was added,
removed, renumbered, or content-replaced. The recurrence audit found no new
Lab-content reproduction beyond the already recorded post-157 go-backs RL-172
through RL-175, so RL-176 was still reserved at this export-only checkpoint.
The subsequent Catch2 plan guard reproduced a consumer-side go-back from
RL-160 and is recorded below as RL-176. This r15 export remains a clean
numbering/content re-sync, not a new Requirements Lab issue.

### 2026-08-25 local slice — changed-lookahead timestamped attribute update

The native C++ companion for non-regional timestamped `Update Attribute Values`
is additive local coverage, not a Requirements Lab recurrence. It queues a
passel at lookahead one, disables and callback-gated re-enables producer Time
Regulation at lookahead three, verifies `Query Lookahead`, and proves the
reflection-before-grant boundary at the timestamp-five GALT frontier with the
original payload, tag, producer, timestamp/order, and retraction metadata. The
focused Catch2 selector, exact 2025 requirements/API contracts, and dedicated
CTest label all pass against the locked export. No Lab content changed and no
old issue was re-exposed, so this slice consumes no `RL-###` identifier;
RL-177 remains the separate source-artifact tension and RL-178 remains the
next post-157 recurrence slot.

The matching native C++ changed-lookahead Delete Object Instance companion is
also additive local coverage. It retains one timestamped deletion across the
producer's lookahead-one disable and lookahead-three callback-gated re-enable,
checks `Query Lookahead`, and proves `Remove Object Instance` before the
timestamp-five grant with the original object, tag, producer, timestamp/order,
and retraction metadata. Its focused Catch2 selector, exact 2025
requirements/API contracts, and dedicated CTest lane pass against the locked
export. No Lab content changed and no prior issue was re-exposed, so this
companion consumes no observation number; RL-178 remains the next genuine
post-157 recurrence slot.

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

The new strict-OMT lane keeps that boundary explicit: a complete Umbra-owned
DIF fixture passes the official OMT schema after materialization, while the
official MIM-plus-Restaurant composition remains a negative because its
Restaurant custom basic-data row is partial and the four higher-level
representation names above do not satisfy the OMT XSD's basic-data-only
`representationRef`. No encoding metadata or representation alias is invented
to force the official examples through OMT. This is a reproducible
source/schema/example tension, not evidence of a Requirements-Lab defect.

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

### RL-155 — The temporal state-chart label says “Disable Asynchronously Delivery”

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
`../design/RELAXED-DDM-POLICY.md` and the Catch2 plan. This remains
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
`compliance/requirements-lab/handle-normalization-requirements-contract.json` retains the
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

**Additional Lab observation (2026-08-22):** the rendered 2025 source and the
official MIM are sufficient to implement two deliberately bounded positive
file-return forms even though the structured Lab export is not. §9.2 names
Create Region's supplied `Set of dimension designators` and returned `Region
designator`; §10.27 names Get Range Bounds' Region/Dimension supplied values
and its composite `RangeBounds` return. Umbra records those forms with MIM
types 11, 42, 10, and 41, respectively, and uses the rendered Table 5
one-element returned-argument array. This is a source-backed development
traceability decision, not a generic ReturnArgument resolution: the broad
Lab candidate covers return arguments, while the null-return candidate covers
only the no-return rule. No positive Table 5 row-level candidate exists, so
the new C++ unit and filesystem integration evidence must remain nonvalidated
and nonconforming until the Lab can export the relevant rows/cells.

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
catalog-declared periodic subset at an `HLA_EVOKED` callback boundary, and a
thread-safe observer now proves the same subset arrives automatically under
`HLA_IMMEDIATE` without an Evoke call. The GALT/LITS companion reuses the
federation time-bounds calculator for direct and periodic `HLAGALT`/`HLALITS`
values, and verifies the official empty-array form when no regulator remains.
The queued-TSO companion likewise verifies `HLATSOlength` through direct and
periodic requests before and after delivery, using the coordinator's queued
ledger rather than in-transit state. An ownership-backed companion now verifies
`HLAobjectInstancesThatCanBeDeleted` from the live
`HLAprivilegeToDeleteObject` ledger around registration, periodic reflection,
 and deletion. A successful-update-count companion now verifies `HLAupdatesSent`
 from an RTI-owned joined-membership counter at the accepted service boundary,
 with direct 0/1/2 values and periodic reflection of 2. The companion
 `HLAobjectInstancesUpdated` projection retains distinct accepted object
 handles, proving direct 0/1/1/2 values and periodic 2. The common registration
 path now also supplies `HLAobjectInstancesRegistered`, proving direct 0/1/2
 values and periodic 2 after two successful registrations. The deletion
 statistic now counts accepted receive-order deletion and timestamped
 queue-admission boundaries, proving direct 0/1/2 and periodic 0 before
deletion and 2 after two deletions. The receiving federate's
`HLAobjectInstancesRemoved` counter now advances at committed no-time and
timestamped Remove Object Instance callback boundaries for ordinary
application objects; the focused C++ vector proves direct 0/1/2 and periodic
2 for two receive-order callbacks. The same vector proves
`HLAobjectInstancesDiscovered` at 0/1/2 for two application-object callbacks,
then 3 after local deletion and an eligible rediscovery of the same object.
The interaction-send MOM vector now records the sender-side accepted-service
boundary explicitly: native Catch2 proves `HLAinteractionsSent` at
0/1/2/3/4/5/6 and `HLAdirectedInteractionsSent` at 0/0/1/1/2/2 for ordinary,
directed, timestamped, regional, and timestamped-regional sends, with periodic
6/2 reflection. The sender counter is intentionally independent of recipient
fan-out, and the Java/JPype bridge remains a separate direct 0/1/2 evidence
vector. The same native case now exercises the receiver-owned
`HLAinteractionsReceived` and `HLAdirectedInteractionsReceived` counters through
the peer's MOM object: direct total 0/1/2/3/4/5/6 and directed 0/0/1/1/2/2,
with periodic 6/2. The receiver ledger commits once at an accepted application
callback boundary; RTI-originated MOM callbacks and suppressed projections are
excluded. The Java/JPype bridge retains a smaller direct 0/1/2 and 0/0/1 vector.
Remaining
periodic/other conditional scheduling (including
traffic/statistical values beyond those bounded counts and interaction-send/
interaction-receive
projections),
optional/inherited non-initial attributes, and generic RTI-created traffic
remain open.
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
notes and `../design/AUTHORIZATION-DESIGN.md` cite the directly inspected §§12.5-12.6
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

The same limitation applies to the new non-void DDM records. The C++ slice
uses the rendered §9.2/§10.27 definitions and official MIM values for
DimensionHandleSet (11), RegionHandle (42), DimensionHandle (10), and
RangeBounds (41), then tests the Table 5 one-element returned-argument array.
Those exact positive rows are absent from the Lab bundle, so the contract
records source-backed traceability only and deliberately does not promote the
passing Catch2 evidence to validation or conformance.

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
`compliance/requirements-lab/timestamped-interaction-api-contract.json`, registered its
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

### RL-102 — MOM interaction-receipt statistics need an explicit callback relation

**Status:** verified traceability-shape limitation and local test decomposition;
not a Requirements Lab extraction defect or conformance evidence.

At the pinned Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the joined-federate MOM source
material is anchored by the generic
`requirement-candidate-content-clauses-11-management-object-model-page-290-l174-57`
candidate. The export does not provide a receiver-specific relation that joins
`HLAinteractionsReceived`/`HLAdirectedInteractionsReceived` to the accepted
application callback boundary, timestamped admission, directed-subset rule,
DDM delivery, callback suppression, and exclusion of RTI-originated MOM traffic.
The API export also lists the public send/receive surfaces independently, so a
single generated scenario cannot be recovered from the Lab bundle alone.

**Umbra impact:** the MOM contract keeps the immutable §11.4.1 candidate as its
source anchor and records the receiver statistic semantics directly. The native
Catch2 case supplies the stronger cross-federate proof: receiver totals advance
0/1/2/3/4/5/6 and directed values 0/0/1/1/2/2 across ordinary, directed,
timestamped, regional, and timestamped-regional callbacks, with periodic 6/2;
the Java/JPype bridge retains a smaller direct vector. This remains
development-profile traceability, not Lab validation or conformance evidence.

**Possible Lab/tooling refinement:** emit a cross-cutting MOM interaction
statistics relation with represented federate, sender/receiver role, callback
kind, directed flag, timestamp/queue boundary, DDM eligibility, suppression
outcome, and periodic projection. Keep the generic §11.4.1 candidate as the
normative source anchor while allowing focused sender and receiver scenario
families to be generated without inferring semantics from implementation code.

### RL-103 — Federation MOM membership needs a lifecycle relation

**Status:** verified traceability-shape limitation and local test decomposition;
not a Requirements Lab extraction defect or conformance evidence.

At the pinned Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the execution-scoped
`HLAmanager.HLAfederation` object and §11.4.1 MOM value are exported as
separate/static generic records. The bundle does not provide a federation-MOM
relation that joins `HLAfederatesInFederation` to the live Join/Resign
membership transition, the nested `HLAfederateReferenceList` representation,
the membership-only discovery condition, and the direct current-value request
path. Selecting the generic MOM candidate alone therefore does not generate
the 1→2→1 lifecycle scenario.

**Umbra impact:** the native Catch2 case discovers the federation object from
only `HLAfederatesInFederation`, decodes each nested public FederateHandle with
the official C++ decoder, checks direct values for one and two members, and
checks conditional reflections after Join and Resign. The Java/JPype companion
uses the external encoder for the same bridge vector. Both remain embedded
development-profile traceability rather than Lab validation or conformance.

**Possible Lab/tooling refinement:** emit a federation-MOM membership relation
with execution object identity, represented membership set, nested handle
encoding, discovery eligibility, direct-request value, and Join/Resign event
boundaries. Keep the generic §11.4.1 candidate as the normative source anchor
while allowing native and bridge lifecycle scenarios to be generated without
inferring the relation from runtime code.

### RL-104 — Public service-report delivery needs a producer and callback relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

At the pinned Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the §11.5.1 candidate
`requirement-candidate-content-clauses-11-management-object-model-page-292-l9-2`
states that `HLAreportServiceInvocation` is generated by the RTI, while the
ordinary §6.13 Receive Interaction API candidate independently requires a
non-optional `FederateHandle producingFederate` argument. The export does not
provide a relationship that joins the represented joined federate, the
RTI-originated report, the observer's subscription, the seven MIM parameters,
reliable receive-order delivery, or callback ordering against the ordinary
service-induced interaction. It also does not resolve which public producer
handle should be supplied for RTI-originated MOM traffic; that is the existing
RL-043 boundary.

**Umbra impact:** the native Catch2 case
`Embedded service reporting delivers HLAreportServiceInvocation to an eligible
observer` now proves one accepted untimed `SendInteraction` path. The observer
receives the seven parameters over `HLAreliable` before the ordinary
interaction, with service type 2, serial zero, preserved payload/tag, and the
documented default-invalid producer handle. This is a development-profile
projection only; timestamped, regional, directed, failure, and broader MOM
interaction matrices remain separate work.

**Possible Lab/tooling refinement:** emit a cross-cutting MOM service-report
delivery relation with represented federate, invoking service, service group,
observer subscription, MIM parameter set/encodings, transport/order,
producer-designator policy, serial sequence, and callback ordering. Keep the
immutable §11.5.1 and §6.13 candidates as source anchors rather than inferring
the relationship from Umbra's implementation.

### RL-105 — Directed service-report delivery adds target and timestamp relations

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The same pinned §11.5.1 and §6.13 candidates do not relate a directed
`SendDirectedInteraction` admission to its target object instance, directed
selector/publication, RTI-originated report interaction, serial sequence,
timestamp encoding, and the subsequent `Receive Directed Interaction`
callback. The timestamped directed API candidate is a separate source/API
record, so the Lab export also does not express the report-before-callback
ordering or the no-time-regulating timestamped callback form.

**Umbra impact:** the native Catch2 case
`Embedded service reporting delivers directed invocation reports before directed
callbacks` proves accepted untimed and timestamped directed sends, seven MIM
parameters over `HLAreliable`, serials zero and one, the default-invalid RTI
producer policy (RL-043), target fan-out, and the timestamped logical-time
callback argument. The companion external IEEE-JAR JNI/JPype regression now
decodes the same seven parameters through Java's `EncoderFactory` for untimed,
timestamped receive-order, and time-regulated timestamped directed sends. It
proves serials 0/1/2, the type-34 null return forms, the type-33
`MessageRetractionHandle` return form, and the constrained observer's
report-before-flush ordering. A native filesystem companion now exercises the
accepted timestamped sender through the shared file-or-interaction selector,
proving type-27/type-37/type-40/type-63/type-31 supplied forms, the type-34
Null return for a non-time-regulating sender, serial zero, and durable
report-before-directed-callback ordering. It remains development-profile
traceability only; RL-042's generic file `ReturnArgument` boundary for
non-null returns, directed DDM/time/transport matrices, Lab validation, and
conformance remain open.

The corresponding ordinary timestamped §6.12 `Send Interaction` filesystem
companion now covers the same backend-selection edge for the default interaction
path. It proves type-27/type-40/type-63/type-31 supplied forms, the type-34
Null return for a non-time-regulating sender, serial zero, and durable
report-before-`Receive Interaction` ordering. This is still development-profile
traceability only: RL-105/RL-152 provide no row-level report-backend or failed-
outcome relation, and time-regulated, regional, transport, Lab-validation, and
conformance evidence remain separate.
The matching timestamped regional §6.12/§9.12 filesystem companion now covers
the source-region extension. It proves type-27/type-40/type-43/type-63/type-31
supplied forms, the type-33 `MessageRetractionHandle` return, serial zero, and
durable report-before-constrained-regional-`Receive Interaction` ordering.
The paired HLA_IMMEDIATE public-MOM companion now decodes the same accepted
service type 2 report before the constrained callback, including every supplied
form, the quoted type-33 return, success/empty-exception fields, and serial zero;
its callback verifies the source RegionHandle, timestamp/order metadata, and
valid retraction. The Lab still does not relate the backend selection,
source-region argument, retraction return, or callback frontier, so both cases
remain development-profile traceability rather than Lab validation or
conformance. Regional failure, regional object-update/delete, transport, and
broader cross-service lifecycle matrices remain separate.
The paired timestamped regional Send Interaction With Regions failure matrix
now covers invalid interaction-class, parameter, region, and logical-time
inputs in both the configured filesystem and HLA_IMMEDIATE MOM routes. Each
path preserves serials zero through three, type-27/type-40/type-43/type-63/
type-31 supplied forms, a Null return, false success, and exact exception text,
with no application Receive Interaction callback. RL-105/RL-152 still provide
no row-level conditional-failure or backend/lifecycle relation, so this is
development-profile evidence only; broader regional failures remain separate.
The corresponding no-time regional Send Interaction With Regions failure matrix
now covers invalid interaction-class, parameter, and region inputs through the
configured filesystem and HLA_IMMEDIATE MOM routes. It preserves serials zero
through two, type-27/type-40/type-43/type-63/type-34 forms, a Null return,
false success, exact exception text, and no application Receive Interaction
callback. RL-105/RL-152 provide no row-level conditional-failure/backend
relation, so this remains development-profile evidence rather than Lab
validation or conformance.
The matching ordinary regional `Update Attribute Values` filesystem companion
now closes the no-time backend-selection edge for an explicit committed
object/attribute region association. It proves type-37/type-2/type-63/type-34
supplied forms, serial zero, and durable
report-before-constrained-regional-`Reflect Attribute Values` ordering; the
callback separately conveys the source `RegionHandleSet`. RL-105/RL-152 still
do not relate report-backend selection, object-region association, or callback
frontier, so this remains development-profile traceability rather than Lab
validation or conformance. Failed regional-update matrices, public MOM
interaction, regional deletion, and broader lifecycle/transport evidence remain
separate; the ordinary invalid-object/invalid-attribute file matrix is the
bounded exception-path companion, while timestamped and other failure forms
remain open.
The ordinary regional update companion now also has an HLA_IMMEDIATE public-MOM
case. It decodes service type 2, type-37/type-2/type-63/type-34 supplied forms,
the type-34 Null return, success true, empty exception, and serial zero before
the constrained reflection callback, which separately verifies the source
`RegionHandleSet` and receive-order metadata. RL-152 still provides no
row-level backend/lifecycle relation, so this remains development-profile
traceability rather than Lab validation or conformance.
The corresponding ordinary regional failure companion now also has a paired
HLA_IMMEDIATE MOM case. It decodes invalid-object and invalid-attribute
`Update Attribute Values` reports with the same type-37/type-2/type-63/type-34
forms, Null return, false indicator, exact exception text, and serials zero and
one, while proving that no reflection callback is delivered. RL-152 still
leaves the conditional-failure relation unmodeled; this is development-profile
traceability rather than Lab validation or conformance.
The corresponding timestamped regional `Update Attribute Values` filesystem
companion now covers the object/attribute source-region extension. It proves
type-37/type-2/type-63/type-31 supplied forms, the type-33
`MessageRetractionHandle` return, serial zero, and durable
report-before-constrained-regional-`Reflect Attribute Values` ordering for an
explicit committed region association; the callback separately conveys the
source `RegionHandleSet`. RL-105/RL-152 still do not relate report-backend
selection, object-region association, retraction return, or callback frontier,
so this remains development-profile traceability rather than Lab validation or
conformance. The timestamped regional failure companion now closes the
bounded exception-path gap: invalid object, invalid attribute, and invalid
logical time preserve serials zero through two, type-37/type-2/type-63/type-31
supplied forms, Null returns, false indicators, and exact exception text on the
same configured filesystem after explicit committed regional association, with
no callback delivery. RL-152 still leaves the conditional failure relation
unmodeled. A matching HLA_IMMEDIATE MOM case now decodes those three failure
records through `HLAreportServiceInvocation`; other regional failure families,
regional deletion, and broader lifecycle/transport evidence remain separate.
The accepted timestamped regional companion now also has a matching
HLA_IMMEDIATE MOM case. It decodes the successful service type 2 report,
type-37/type-2/type-63/type-31 supplied forms, quoted type-33 retraction return,
serial zero, and report-before-constrained-reflection ordering. RL-105/RL-152
still provide no row-level backend/lifecycle relation, so this is C++
development-profile traceability rather than Lab validation or conformance.

The matching ordinary regional §6.12/§9.12 companion now covers the accepted
no-time `Send Interaction With Regions` service-report boundary in both
selectors. The filesystem and HLA_IMMEDIATE MOM cases preserve service type 2,
type-27/type-40/type-43/type-63/type-34 supplied forms, a type-34 Null return,
success true, and serial zero before the constrained `Receive Interaction`
callback; the callback separately verifies the source `RegionHandleSet`.
Requirements Lab RL-105/RL-152 still do not relate backend selection, the
source-region argument, or the callback frontier, so this remains development-
profile traceability rather than Lab validation or conformance.

**Possible Lab/tooling refinement:** add a directed service-report relation that
binds the represented federate, target object instance, directed interaction
class, report subscription, timestamp/retraction slots, serial ordering, and
the report-before-directed-callback event sequence while retaining the existing
§11.5.1, §6.13, and timestamped-directed API candidates as source anchors.

### RL-106 — MOM time-state durations need a direct/periodic state relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic §11.4.1 MOM candidate but does not
relate `HLAtimeGrantedTime` or `HLAtimeAdvancingTime` to the represented
federate's temporal state, the `HLAsetTiming` target/period, the direct Request
Attribute Value Update path, or the consume-once periodic reflection boundary.
It therefore cannot generate the distinction between a non-consuming current
duration and the registry-owned interval claimed by a periodic report.

**Umbra impact:** the joined-federate MOM contract retains the generic §11.4.1
candidate and records the two duration attributes separately. Native Catch2 and
JNI/JPype cases provide the stronger development-profile evidence: official
`HLAinteger32BE` HLAmsec encodings are returned directly and periodically in
both callback models, with reliable RTI-originated reflection metadata. This is
traceability only, not Lab validation or conformance.

**Possible Lab/tooling refinement:** emit a MOM duration relation with
represented federate, duration attribute, temporal-state interval, direct versus
periodic request mode, report period/deadline, reset/consumption semantics, and
callback model. Keep the generic §11.4.1 candidate as the normative source
anchor instead of inferring the relation from implementation code.

### RL-107 — Reflection statistics need callback and distinct-object relations

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic §11.4.1 MOM candidate but does not
relate `HLAreflectionsReceived` to an accepted application
`Reflect Attribute Values` callback, or `HLAobjectInstancesReflected` to the
distinct-object set that must be retained across repeated callbacks. It also
does not distinguish application-object reflections from RTI-owned MOM
reflections, nor does it express the timestamped callback-admission boundary
or the direct-versus-periodic projection relation.

**Umbra impact:** the joined-federate MOM contract records the two attributes
against the generic candidate and the native Catch2 case proves direct values
for repeated receive-order updates, a second object, and a queued timestamped
reflection, followed by periodic `HLAsetTiming` values. The runtime counters are
advanced immediately before application callback entry; MOM-owned reflections
do not advance them. This is stronger local development-profile evidence, not
Lab validation or conformance.

**Possible Lab/tooling refinement:** emit a reflection-statistics relation with
represented federate, callback kind, application object identity, distinctness
policy, RTI-owned-object exclusion, timestamp/retraction admission, direct or
periodic request mode, and callback model. Keep the generic §11.4.1 candidate
as the normative source anchor instead of inferring these relations from
Umbra's implementation.

### RL-108 — Receive-order queue length needs a callback-queue relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic §11.4.1 MOM candidate but does not
relate `HLAROlength` to the represented federate's receive-order queue source.
In particular, it cannot state that application receive-order work remains
counted until the target callback boundary, that deferred asynchronous
receive work is included, that RTI-owned MOM traffic is excluded, or that the
same value is observable through direct Request Attribute Value Update and
`HLAsetTiming` periodic reflection under a selected callback model.

**Umbra impact:** the joined-federate MOM contract records the attribute
against the generic candidate and the native Catch2 case proves direct 0/1/0
and periodic 1 across one queued interaction and its later `HLA_EVOKED`
delivery. The Java/JPype companion covers the same transition for both
`HLA_EVOKED` and `HLA_IMMEDIATE`. This is stronger local development-profile
evidence, not Lab validation or conformance.

**Possible Lab/tooling refinement:** emit a queue-length relation with
represented federate, queue source, receive-order message family, callback
admission gate, direct versus periodic request mode, report period/deadline,
and callback model. Keep the generic §11.4.1 candidate as the normative source
anchor instead of inferring these relations from Umbra's implementation.

### RL-109 — MOM object-instance request/report needs a correlated class-count relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic §11.5.1/§11.4.1 MOM candidates but
does not relate the Subscribe-only `HLArequestObjectInstancesUpdated`
interaction to its RTI-originated `HLAreportObjectInstancesUpdated` response.
It cannot express the `HLAfederate` target parameter, the target's
joined-lifetime update ledger, class-grouped positive counts, the nested
`HLAobjectClassBasedCounts` encoding, report-subscription filtering, callback
revalidation, or the unresolved RTI producer designator.

**Umbra impact:** the native Catch2 case
`Embedded MOM requestObjectInstancesUpdated reports class-grouped counts`
consumes the request without ordinary publication validation, snapshots two
registered classes after repeated updates, decodes one reliable report with
one positive count per class, and verifies callback-gated delivery, empty tag,
and the default-invalid producer policy. The joined-federate contract and
Catch2 plan retain the generic Lab candidate as a source anchor while marking
this as development-profile traceability only.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with requester, represented federate, request and report class,
target parameter, class-count encoding, report subscription/region, callback
boundary, and producer-designator fields. Keep the generic source candidates
immutable and do not infer a public producer handle from the RTI-owned route.

### RL-110 — MOM deletable-object request/report needs a live ownership relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the same generic MOM candidate for the
Subscribe-only `HLArequestObjectInstancesThatCanBeDeleted` interaction and its
RTI-originated `HLAreportObjectInstancesThatCanBeDeleted` response, but it does
not represent that the report is derived from the current
`HLAprivilegeToDeleteObject` owner ledger rather than a historical registration
counter. It also does not connect the request's `HLAfederate` target to the
report's class-grouped `HLAobjectClassBasedCounts` value, private
`HLAfederate` point, subscription gate, callback boundary, or RTI producer.

Umbra records the generic §11.4.1 candidate in the public MOM contract and the
native Catch2 plan, while the implementation and test remain explicitly
development-profile traceability. The focused case proves two live registered
classes initially, deletes one object, and confirms the next report removes
only that class. This prevents a future requirement export from mistaking the
existing direct `HLAobjectInstancesThatCanBeDeleted` attribute projection for
the request/report interaction contract.

**Possible Lab/tooling refinement:** emit a correlated request/report relation
with represented federate, live ownership source and transition, class-count
encoding, request/report class and parameter, report subscription/region,
callback boundary, and producer-designator fields. Keep the generic source
candidate immutable and do not infer a public producer handle from the
RTI-owned route.

### RL-111 — MOM reflected-object request/report needs a distinct callback relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic MOM candidate for the Subscribe-only
`HLArequestObjectInstancesReflected` interaction and its RTI-originated
`HLAreportObjectInstancesReflected` response, but it does not relate the
request's `HLAfederate` target to the distinct-object semantics. It cannot
express that the source is the accepted application `Reflect Attribute Values`
callback boundary, that repeated reflections of one object remain one
object-instance count, that counts are grouped by registered class, or that
MOM-owned reflections are excluded. The nested
`HLAobjectClassBasedCounts` encoding, private `HLAfederate` point,
subscription/callback revalidation, and RTI producer designator are also not
represented.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan, while the native case remains explicitly development-profile
traceability. It proves two registered classes after two initial reflections
and a repeated reflection of one object, reliable RTI-originated delivery,
empty tag, default-invalid producer, and `HLA_EVOKED` callback gating.

**Possible Lab/tooling refinement:** emit a correlated request/report relation
with represented federate, callback-admission source, distinct-object versus
invocation-count semantics, registered-class grouping, nested count encoding,
request/report class and parameter, report subscription/region, callback
boundary, and producer-designator fields. Keep the generic source candidate
immutable and do not infer a public producer handle from the RTI-owned route.

### RL-112 — MOM updates-sent request/report needs a transportation-count relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic MOM candidate for the Subscribe-only
`HLArequestUpdatesSent` interaction and its RTI-originated
`HLAreportUpdatesSent` response, but it does not relate the request's
`HLAfederate` target to the report's one-per-transport semantics. It cannot
express that the source is the represented federate's accepted
`Update Attribute Values` ledger, that counts are grouped by registered object
class and effective `HLAreliable`/`HLAbestEffort` transportation, or that each
report carries the official `HLAtransportation` handle beside nested
`HLAupdateCounts`/`HLAobjectClassBasedCounts`. The private `HLAfederate` point,
report subscription, callback revalidation, and RTI producer designator are
also not represented.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan, while the native case remains explicitly development-profile
traceability. It proves two accepted best-effort Server updates and one
accepted reliable Soda update, producing two reliable reports with the
transport-specific class counts, empty tag, default-invalid producer, and
`HLA_EVOKED` callback gating.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with requester, represented federate, one-report-per-transport
cardinality, accepted-update boundary, registered-class grouping, transport
handle, nested count encoding, report subscription/region, callback boundary,
and producer-designator fields. Keep the generic source candidate immutable
and do not infer a public producer handle from the RTI-owned route.

### RL-113 — MOM interactions-sent request/report needs a regional sender relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic MOM candidate for the Subscribe-only
`HLArequestInteractionsSent` interaction and its RTI-originated
`HLAreportInteractionsSent` response, but it does not relate the request's
`HLAfederate` target to the report's one-per-transport semantics. It cannot
express that the source is the represented federate's accepted `Send
Interaction` service boundary, that the count includes interaction sends with
regions, or that positive values are grouped by sent interaction class and
effective transportation in the nested `HLAinteractionCounts` value. The
private `HLAfederate` point, report subscription, callback revalidation, and
RTI producer designator are also not represented.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan, while the native case remains explicitly development-profile
traceability. It proves one reliable ordinary `TakeOrder`, one best-effort
`TakeOrder`, and one reliable regional `MainCourseServed` send, producing two
reliable RTI-originated reports with class-specific counts, empty tag,
default-invalid producer, and `HLA_EVOKED` callback gating.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with requester, represented federate, one-report-per-transport
cardinality, regional-inclusion rule, accepted sender boundary, sent-class
grouping, transport handle, nested interaction-count encoding, report
subscription/region, callback boundary, and producer-designator fields. Keep
the generic source candidate immutable and do not infer a public producer
handle from the RTI-owned route.

### RL-114 — MOM directed-interactions-sent request/report needs a directed sender relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic MOM candidate for the Subscribe-only
`HLArequestDirectedInteractionsSent` interaction and its RTI-originated
`HLAreportDirectedInteractionsSent` response, but it does not distinguish the
directed subset from the all-interactions sender ledger. It cannot express that
the source is the represented federate's accepted `Send Directed Interaction`
service boundary, that counts are grouped by sent interaction class and
effective transportation in nested `HLAinteractionCounts`, or that one report
is produced for each transportation type used. The private `HLAfederate` point,
report subscription, callback revalidation, and RTI producer designator are
also not represented.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan, while the native case remains explicitly development-profile
traceability. It proves two accepted reliable directed `TakeOrder` sends,
one reliable RTI-originated report, empty tag, default-invalid producer, and
`HLA_EVOKED` callback gating.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with requester, represented federate, directed-subset source ledger,
one-report-per-transport cardinality, accepted directed-send boundary,
sent-class grouping, transport handle, nested interaction-count encoding,
report subscription/region, callback boundary, and producer-designator fields.
Keep the generic source candidate immutable and do not infer a public producer
handle from the RTI-owned route.

### RL-115 — MOM interactions-received request/report needs a callback receipt relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic MOM candidate for the Subscribe-only
`HLArequestInteractionsReceived` interaction and its RTI-originated
`HLAreportInteractionsReceived` response, but it does not relate the report to
the represented federate's accepted application callback ledger. It cannot
express that counts are grouped by original sent interaction class and
effective transportation, that receipt is counted only after callback-time
subscription and temporal checks pass, or that one report is emitted for each
transportation type used. The private `HLAfederate` point, report subscription,
callback revalidation, and RTI producer designator are also not represented.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan, while the native case remains explicitly development-profile
traceability. It proves one reliable and one best-effort `TakeOrder` receive,
two reliable RTI-originated report callbacks, empty tags, default-invalid
producer, and `HLA_EVOKED` callback gating.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with requester, represented federate, callback-admission boundary,
sent-class grouping, one-report-per-transport cardinality, transport handle,
nested interaction-count encoding, report subscription/region, callback
boundary, and producer-designator fields. Keep the generic source candidate
immutable and do not infer a public producer handle from the RTI-owned route.

### RL-116 — MOM directed-interactions-received needs a directed callback relation

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the same generic §11.4.1 MOM candidate for the
Subscribe-only `HLArequestDirectedInteractionsReceived` interaction and its
RTI-originated `HLAreportDirectedInteractionsReceived` response, but it does
not distinguish directed receive callbacks from ordinary interaction receives.
It cannot express the directed-subset ledger, original sent-class grouping,
the two supported transportation buckets including an empty NULL response, or
the callback admission boundary. The private `HLAfederate` point, report
subscription, callback revalidation, and RTI producer designator are also not
represented.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan, while the native case remains explicitly development-profile
traceability. It proves that an ordinary `TakeOrder` receive is excluded,
one reliable directed `TakeOrder` receive is counted, the best-effort bucket
is emitted empty, and both reports have empty tags, default-invalid producers,
and `HLA_EVOKED` callback gating.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with requester, represented federate, directed callback-admission
boundary, sent-class grouping, one-report-per-transport cardinality, transport
handle, nested interaction-count encoding, report subscription/region,
callback revalidation, and producer-designator fields. Keep the generic source
candidate immutable and do not infer a public producer handle from the
RTI-owned route.

### RL-117 — Sender-side MOM count reports need explicit NULL-bucket cardinality

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic §11.4.1 candidate for the sender-side
MOM request/report families, but it does not relate a request to one report per
supported transportation type when the represented federate has no activity in
that bucket. It cannot express the empty `HLAupdateCounts` or
`HLAinteractionCounts` NULL response, the distinction between ordinary and
directed sender ledgers, or the RTI-owned private `HLAfederate` route.

Umbra records the generic candidate in the existing three public MOM contracts
and adds a native empty-ledger Catch2 case. That case requests
`HLAreportUpdatesSent`, `HLAreportInteractionsSent`, and
`HLAreportDirectedInteractionsSent` and proves reliable and best-effort
reports for each family, empty count arrays, empty tags, default-invalid RTI
producers, and `HLA_EVOKED` delivery. The existing populated-ledger cases
continue to prove class grouping and transportation changes.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with represented federate, supported transport set, one-report-per-
transport cardinality, NULL-versus-populated count semantics, sender-ledger
kind, nested count encoding, report subscription/region, callback revalidation,
and producer-designator fields. Keep the generic source candidate immutable and
do not infer a public producer handle from the RTI-owned route.

### RL-118 — Reflection-received MOM reports need callback and NULL-bucket semantics

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic §11.4.1 candidate for the
Subscribe-only `HLArequestReflectionsReceived` interaction, but it does not
relate that request to the RTI-originated `HLAreportReflectionsReceived`
response's per-transport cardinality or to the receiving federate's accepted
`Reflect Attribute Values` callback boundary. It cannot express registered
object-class grouping, the two supported transportation buckets (including an
empty `HLAreflectCounts` NULL response), nested count encoding, the private
`HLAfederate` endpoint, callback-time subscription revalidation, or the
RTI-owned producer designator.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan while keeping the native case explicitly development-profile
traceability. It proves one reliable and one best-effort application reflection
for the represented federate, then requests the same report for a joined
federate with no reflections and proves both empty buckets. Each report uses an
empty tag, default-invalid RTI producer, and `HLA_EVOKED` delivery.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with represented federate, callback-admission boundary, registered
object-class grouping, one-report-per-transport cardinality, NULL-versus-
populated nested count semantics, report subscription/region, callback
revalidation, and producer-designator fields. Keep the generic source
candidate immutable and do not infer a public producer handle from the
RTI-owned route.

### RL-119 — Object-instance-information MOM reports need object-state semantics

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic §11.4.1 candidate for the
Subscribe-only `HLArequestObjectInstanceInformation` interaction, but it does
not relate the requested object handle to the single
`HLAreportObjectInstanceInformation` response. It cannot express the
known-versus-NULL response shape, omission of `HLAregisteredClass` and
`HLAknownClass` for an unknown object, the empty-versus-populated nested
`HLAattributeHandleList`, the represented federate's ownership/class snapshot,
the private `HLAfederate` endpoint, callback-time subscription revalidation,
or the RTI-owned producer designator.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan while keeping the native case explicitly development-profile
traceability. It proves a NULL response after local knowledge is removed, a
known response with no owned attributes, and the registering federate's known
response containing both `Efficiency` and the implicitly published
`HLAprivilegeToDeleteObject`. Reports use reliable transport, an empty tag,
the default-invalid RTI producer, the requesting federate's private dimension
point, and `HLA_EVOKED` delivery.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with requested object handle, represented federate, known/NULL
parameter-presence rules, nested attribute-handle-list semantics, ownership
and registered/known class snapshot fields, report subscription/region,
callback revalidation, and producer-designator fields. Keep the generic source
candidate immutable and do not infer a public producer handle from the
RTI-owned route.

### RL-120 — Publications MOM reports need three correlated report shapes

**Status:** verified traceability-shape limitation with a bounded local
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The pinned Lab export exposes the generic §11.4.1 candidate for the
Subscribe-only `HLArequestPublications` interaction, but it does not relate
the request to the three required report families: one
`HLAreportInteractionPublication`, one `HLAreportObjectClassPublication` per
published object class, and one `HLAreportDirectedInteractionPublication` per
object class with directed publications. It cannot express the target
federate's publication snapshot, implicit `HLAprivilegeToDeleteObject`
publication, directed class grouping, nested
`HLAattributeHandleList`/`HLAinteractionClassHandleList` encodings, the
different NULL parameter-presence rules, the private `HLAfederate` endpoint,
callback-time subscription revalidation, or the RTI-owned producer designator.

Umbra records the generic §11.4.1 candidate in the public MOM contract and
Catch2 plan while keeping the native case explicitly development-profile
traceability. It proves all three populated reports, then the object-class
count-only, empty interaction-list, and directed count-plus-empty-list NULL
responses after declarations are withdrawn. Reports use reliable transport,
empty tags, the default-invalid RTI producer, the target's private dimension
point, and `HLA_EVOKED` delivery.

**Possible Lab/tooling refinement:** emit a correlated MOM request/report
relation with target federate, three-report cardinality, publication snapshot
fields, implicit privilege publication, directed class grouping, nested list
encodings, NULL parameter-presence rules, report subscription/region,
callback revalidation, and producer-designator fields. Keep the generic source
candidate immutable and do not infer a public producer handle from the
RTI-owned route.

### RL-121 — `source_symbol` is a literal substring, not a multi-symbol field

**Status:** verified checker/workflow limitation with a local contract
mitigation; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

Against pinned Lab revision `4f012fb1c21367cfde67aab8498ae00e2a64c615`, the
contract checker treats each `source_symbol` value as one literal substring of
the file named by `source`. Existing MOM service-report entries used slash-
combined values such as `UmbraRtiAmbassador::changeAttributeOrderType /
changeDefaultAttributeOrderType / ...`. The checker therefore reported those
entries as absent even though every individual C++ method was present and the
referenced tests exercised both the attribute and interaction forms.

This made the full CTest traceability lane fail after otherwise successful
runtime and focused-lane verification. Umbra now records one concrete method
per `source_symbol`, retains the complete multi-method scope in the entry's
notes and test references, and verifies the contract with the Lab before
CTest. No runtime behavior or standards interpretation changed.

**Possible Lab/tooling refinement:** allow `source_symbol` to be either one
symbol or an explicit list of symbols, with each list member checked
independently and diagnostics naming the missing member. Preserve the current
literal-string behavior for backward compatibility, and reject slash-joined
compound values with a migration hint rather than treating them as a single
unmatchable symbol.

### RL-122 — Subscriptions MOM reports are correlated shapes, not one generic requirement

**Status:** verified traceability-shape limitation plus an official-resource
reconciliation note; not a Requirements Lab extraction defect, standards
finding, or conformance evidence.

At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the generic §11.4.1 candidate for
Subscribe-only `HLArequestSubscriptions` does not describe the three
correlated RTI-originated report families: object-class reports grouped by
`(object class, HLAactive)` with `HLAmaxUpdateRate` and nested
`HLAattributeHandleList`, one `HLAinteractionSubscription` fixed-record list
of interaction-class/active pairs, and directed object-class reports. It also
does not express related regional DDM declarations, the three distinct NULL
parameter shapes, private `HLAfederate` endpoint routing, or callback-time
subscription revalidation.

The local pinned official MIM resource
`third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml` declares
`HLAreportDirectedInteractionSubscription` with
`HLAnumberOfClasses`, `HLAobjectClass`, and
`HLAinteractionClassList`; it does **not** declare `HLAuniversal`. The
semantic reconstruction used by the Requirements Lab includes an
`HLAuniversal` field for the same report. This is an input/resource-shape
discrepancy to keep visible, not a license to invent a public parameter in
the local official binding. Umbra follows the pinned MIM XML and keeps an
optional internal route for a compatible catalog only.

**Umbra impact:** the native contract and focused Catch2 case use the single
generic Lab candidate for the request/report family while retaining the exact
MIM/XML shapes in source notes and tests. The implementation reports active
and passive ordinary/regional subscriptions, aggregates the maximum named
rate per object-class/active group, encodes the official nested interaction
subscription records, emits the local directed-report shape, and verifies
NULL responses and private callback routing. This remains development-profile
traceability, not validated evidence or a conformance claim.

**Possible Lab/tooling refinement:** add a correlated MOM request/report model
with report-family cardinality, field-presence/NULL rules, nested data-type
schemas, regional-DDM relation, and endpoint/subscription predicates. Also
surface a pinned-resource discrepancy record when the semantic reconstruction
and official MIM XML disagree, so consumers can choose the reviewed source
without silently widening the public wire contract.

### RL-123 — Working-tree MOM service contract contains unimplemented selectors

**Status:** resolved local contract drift; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned checker had rejected the working-tree contract for the original
five metadata items: the object-name reservation entry used a slash-combined
source symbol, and its two C++ test selectors were not present in the native
Catch2 registry; the Enable Time Constrained and Enable Time Regulation
entries likewise named C++ selectors that are not present. During the
subsequent source-symbol scan it also found one compound symbol for the
time-role transition entry. The contract metadata has now been corrected to
use one searchable source symbol per entry and the exact registered native
selectors. The corresponding JPype bridge selectors remain documented, and
the checker plus the MOM service-reporting traceability CTest are green again.

**Umbra impact:** this is kept separate from the `HLArequestSubscriptions`
slice. No unimplemented native test is presented as evidence; the corrected
service-reporting contract is now checked independently alongside the focused
MOM lanes. The original drift and its exact metadata repair remain visible
rather than being hidden by loosening the checker.

**Possible local refinement:** retain the exact-selector check as a guard when
new native cases are added; if a selector is intentionally retired, remove it
from the contract in the same change. The current repair is metadata-only and
does not claim the underlying service-reporting implementation is complete.

### RL-124 — Broad FOM-content candidate omits scoped report semantics

**Status:** verified local modeling gap; not a standards finding or conformance
evidence.

The Lab candidate
`requirement-candidate-content-clauses-04-federation-management-page-049-l19-4`
(clause `clause-4`) says that the RTI shall provide access to the current FDD,
individual FOM-module content, and related federation-management information.
That broad statement does not identify the separate federate-scoped
`HLArequestFOMmoduleData`/`HLAreportFOMmoduleData` interactions, the
`HLAFOMmoduleIndicator` sequence established at Join, retention of the
validated serialization, private reported-federate endpoint, callback-time
revalidation, or invalid-index behavior. It also does not distinguish the
dimensionless federation-scoped `HLArequestFOMmoduleData`/
`HLAreportFOMmoduleData` and `HLArequestMIMdata`/`HLAreportMIMdata` pairs,
their subscriber broadcast route, retained federation content, strict
parameter rules, or federation-scoped current-FDD reporting.

**Umbra impact:** the native contracts now map the broad candidate to two
bounded scoped slices: federate-level FOM-module reporting and
federation-level FOM/MIM reporting. Both retain canonical UTF-8 XML
serialization produced by schema validation and expose it through the
official `HLAunicodeString` parameters without rereading a mutable source
file. The federation-level report is dimensionless and subscriber-broadcast;
the federate-level report uses the private HLAfederate endpoint. These are
development-profile traceability results, not validated evidence or a
conformance claim. Federation-scoped current-FDD reporting remains an
explicit backlog item.

**Possible Lab/tooling refinement:** add distinct MOM request/report relation
types for federate-scoped and federation-scoped content, with fields for
module-index ordering, Join-lifetime retention, report endpoint, callback
revalidation, invalid-index handling, and source/resource provenance. Keep the
broad content-access candidate as a parent requirement rather than treating it
as a complete executable test contract.

### RL-125 — MOM table candidates do not relate synchronization requests to report state

**Status:** verified local modeling gap; not a Requirements Lab extraction defect,
standards finding, or conformance evidence.

The 2025 Lab export provides useful but independent candidates for the MOM
surface: `requirement-candidate-content-clauses-11-management-object-model-page-290-l111-36`
captures publication of the Table 7 leaf interaction classes,
`requirement-candidate-content-clauses-11-management-object-model-page-290-l138-45`
captures the empty-array encoding of an undefined array parameter, and
`requirement-candidate-content-clauses-11-management-object-model-page-290-l150-49`
captures the rule that RTI-originated MOM reports supply exactly the catalogued
parameters. The synchronization-point state requirement is separately
represented by `requirement-candidate-content-clauses-04-federation-management-page-044-l108-35`.
None of those records relates the two federation request classes to their two
report classes, or records the active-label snapshot, the
`HLAsyncPointFederateList` fixed-record payload, the unknown-label empty-array
response, the dimensionless broadcast route, or callback-time subscription
revalidation.

**Umbra impact:** the native contract and focused Catch2 case now bind those
independent candidates to one bounded federation synchronization-report slice.
The implementation uses the retained synchronization ledger, emits reliable
RTI-originated reports with the official `HLAsyncPointList` and
`HLAsyncPointFederateList` encodings, derives `MovingToSyncPoint` and
`WaitingForRestOfFederation` from achievement state, and rejects missing or
unexpected request parameters. This remains development-profile traceability
only; it does not promote the Lab records or claim MOM conformance.

**Possible Lab/tooling refinement:** add a first-class MOM request/report
relation with request/report class names, expected cardinality, required and
NULL parameters, datatype encodings, dimension/transport/order metadata,
state-snapshot provenance, and callback eligibility predicates. Keep the table
publication and empty-array candidates as reusable parent constraints.

### RL-126 — MOM failure reporting is not modeled as a request/report relation

**Status:** verified local modeling gap; not a Requirements Lab extraction defect,
standards finding, or conformance evidence.

The pinned Lab's Table 7 candidates identify MOM report interaction classes and
their catalogued parameters, but they do not relate a rejected MOM
`Send Interaction` to `HLAreportMOMexception`. In particular, no candidate
captures the fully qualified failing MOM interaction name, the exception text,
the `HLAparameterError` Boolean distinction between malformed parameters and a
validly shaped interaction whose service preconditions fail, the private
`HLAfederate` endpoint, or callback-time subscription withdrawal. The existing
generic content candidates therefore cannot express the native malformed
`HLAsetSwitches` scenario without overstating coverage.

**Umbra impact:** the native contract and focused Catch2 case keep this as a
development-profile-only slice. Umbra preserves the caller's typed
`InteractionParameterNotDefined`, emits the separate reliable
`HLAreportMOMexception` report with `HLAparameterError=true`, uses the
default-invalid RTI producer boundary, and revalidates the observer's
subscription at callback time. Generic `HLAservice` spoofing and other MOM
failure families remain open; no Lab validation or conformance claim is made.

**Possible Lab/tooling refinement:** add a MOM failure-report relation with the
failing interaction/service class, exception and parameter-error fields,
subscription/switch predicates, target-dimension route, callback-time
revalidation, and the expected report cardinality. Keep the existing Table 7
publication candidate as a parent constraint rather than treating it as the
failure behavior itself.

### RL-127 — Current-FDD access is not modeled as a MOM attribute lifecycle

**Status:** verified local modeling gap; not a Requirements Lab extraction defect,
standards finding, or conformance evidence.

The pinned Lab candidate
`requirement-candidate-content-clauses-04-federation-management-page-049-l19-4`
states only that the RTI shall provide access to the current FDD, alongside
individual FOM-module content. Its clause-scoped, cross-cutting record has no
relation to the federation-scoped `HLAcurrentFDD` MOM attribute, its official
`HLAunicodeString` representation, the RTI-owned `HLAfederation` object, or the
conditional reflection triggered when a compatible additional FOM is supplied
at Join. It also does not capture the direct Request Attribute Value Update
agreement with the refreshed value or the RTI-originated endpoint metadata.

**Umbra impact:** the native contract and focused Catch2 case bind that broad
candidate to a bounded object-management slice. The implementation exposes the
schema-validated materialized FDD, refreshes it reliably after an additional
FOM Join, and reuses the same value for direct requests while checking the
default-invalid producer, empty tag, and no-region boundary. This remains
development-profile traceability only; the Lab record is not promoted to
validated evidence or a conformance claim.

**Possible Lab/tooling refinement:** add a MOM current-FDD relation containing
the object/attribute identity, datatype, initial versus conditional update
semantics, Join-triggered state transition, direct-request response, source
artifact provenance, and callback eligibility/transport metadata. Keep the
cross-cutting content-access candidate as the parent requirement.

### RL-128 — Federation save-name/time attributes are not modeled as a lifecycle

**Status:** verified local modeling gap; not a Requirements Lab extraction defect,
standards finding, or conformance evidence.

The pinned save-service candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l100-25`,
`requirement-candidate-content-clauses-04-federation-management-page-066-l10-1`,
`requirement-candidate-content-clauses-04-federation-management-page-066-l16-3`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l37-8`,
and `requirement-candidate-content-clauses-04-federation-management-page-067-l40-9`
describe Request/Initiate/Complete Federation Save behavior. The generic MOM
candidate
`requirement-candidate-content-clauses-11-management-object-model-page-290-l174-57`
describes MOM attributes broadly. None relates the federation-scoped
`HLAnextSaveName`, `HLAnextSaveTime`, `HLAlastSaveName`, and `HLAlastSaveTime`
attributes to the save ledger, their official unicode/logical-time encodings,
the pending-request state, the admission-time clear, or the successful-snapshot
completion update. It also does not capture the reliable two-attribute
conditional reflections or their RTI-originated endpoint metadata.

**Umbra impact:** the native contract and focused Catch2 case bind these
candidates to a bounded public object-management slice. Umbra verifies empty
initial values, a pending timestamped request, next-value clearing when
Initiate Federate Save is admitted at the constrained time-advance boundary,
and last-value publication only after all joined federates complete the
snapshot. This remains development-profile traceability only; restore-operation
semantics, remote transport, JUnit/protected review, and conformance remain
open.

**Possible Lab/tooling refinement:** add a first-class federation save-MOM
relation with the four attribute identities, datatypes and empty forms,
request/pending/admitted/completed state transitions, direct-request behavior,
conditional callback cardinality, endpoint metadata, and provenance links to
the Request Federation Save, Initiate Federate Save, and Federation Saved
service records. Keep the generic MOM attribute candidate as the parent
constraint.

### RL-129 — The 2025 federation MOM has no separate restore-name/time conditional family

**Status:** verified local standards-resource audit; not a Requirements Lab
extraction defect, standards finding, or conformance evidence.

The official vendored `HLAstandardMIM-2025.xml` federation object declares
conditional save attributes `HLAlastSaveName`, `HLAlastSaveTime`,
`HLAnextSaveName`, and `HLAnextSaveTime`. Its federation-object attribute table
contains no `HLAnextRestore*`, `HLAlastRestore*`, or restore-name/time
counterparts. Restore is represented by the federation-management services and
the joined-federate `HLAfederateState` projection, not by an additional
federation-MOM value family.

**Umbra impact:** the save-conditional slice is complete with respect to the
federation object's named save attributes. Restore-operation semantics remain
an independent save/restore runtime lane; no synthetic restore MOM attributes
or homegrown FOM additions should be introduced merely to satisfy the former
roadmap wording.

**Possible Lab/tooling refinement:** expose a machine-readable inventory of
the standard MIM object attributes, including negative absence facts for named
families when a cross-cutting roadmap or contract refers to them. A checker
could then flag scope language that names a non-existent standard attribute
family before implementation work is scheduled.

### RL-130 — Alternate-advance interaction candidates do not encode a frontier matrix

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned time-management candidate
`requirement-candidate-content-clauses-08-time-management-page-185-l17-5`
expresses timestamped delivery eligibility, but the exported candidate model
does not relate one interaction payload to each alternate advance service or
to the required callback-before-grant ordering. The adjacent page-184 and
page-194 candidates provide useful queue and advance-service anchors, but they
do not form a machine-readable FQR/TARA/NMRA scenario matrix.

**Umbra impact:** the new ordinary non-regional Catch2 scenario maps those
anchors explicitly and drives one queued interaction through Flush Queue
Request, Time Advance Request Available, and Next Message Request Available.
It records FQR actual/optimistic time and official callback metadata as
development-profile evidence only; broader alternate-advance, re-enable,
remote, persistence, package, and conformance claims remain open.

**Possible Lab/tooling refinement:** add a relation for each advance service,
its grant callback, callback-before-grant ordering, GALT/optimistic-time
expectations, and payload cardinality. This would let a contract state which
frontiers are tested without implying that one source candidate proves the
entire time-management matrix.

### RL-131 — Ordinary timestamped interaction re-enable is a lifecycle relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned time-management candidates
`requirement-candidate-content-clauses-08-time-management-page-184-l91-26` and
`requirement-candidate-content-clauses-08-time-management-page-185-l17-5`
describe timestamped queue eligibility and grant-frontier delivery, but the
exported model does not relate a queued payload to a recipient's
`Disable Time Constrained` / callback-gated `Enable Time Constrained` lifecycle.
It therefore cannot express the invariant that the joined-federate queue
identity remains stable, the callback occurs exactly once, and the callback
precedes the matching grant after re-enable.

**Umbra impact:** the new ordinary non-regional Catch2 scenario binds those
anchors to a separately contracted lifecycle boundary. It intentionally omits
DDM and source-region state, checks the official timestamp/order/tag/producer
and retraction fields, and remains development-profile evidence only; broader
alternate advances, re-enable combinations, transport, persistence, package,
and conformance claims remain open.

**Possible Lab/tooling refinement:** add a lifecycle relation containing the
queued-message identity, recipient joined-federate lifetime, disable/re-enable
transition, callback cardinality/order, grant service, and terminal retraction
state. Keep the general timestamped-eligibility candidate as the parent
constraint and let contracts declare which lifecycle/advance combinations are
actually exercised.

### RL-132 — Timestamped deletion re-enable needs the same lifecycle relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The timestamped deletion candidates
`requirement-candidate-content-clauses-06-object-management-page-129-l123-34`,
`requirement-candidate-content-clauses-06-object-management-page-131-l89-25`,
and `requirement-candidate-content-clauses-08-time-management-page-185-l17-5`
cover the Delete/Remove service and timestamped eligibility, but the exported
model has no relation for a queued removal crossing a recipient's Time
Constrained disable/re-enable transition. It therefore cannot state that the
object-removal callback is delivered exactly once before the matching grant
after re-enable, nor that the terminal retraction classification is preserved.

**Umbra impact:** the new ordinary non-regional Catch2 scenario binds those
anchors to a dedicated deletion lifecycle requirement. It checks object/name
state, official timestamp/order/tag/producer/retraction metadata, and
callback-before-grant ordering as development-profile evidence only; alternate
advances, broader re-enable combinations, save/restore, ownership/resignation,
transport, package, and conformance claims remain open.

**Possible Lab/tooling refinement:** generalize RL-131's lifecycle relation to
cover message-family-specific callback payloads, including Remove Object
Instance and object/name terminal-state effects. Contracts should be able to
declare whether a lifecycle case preserves a typed payload, reconstitutes an
object, or retires a retraction designator.

### RL-133 — Save/restore of a live deletion needs a cross-service relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The save/restore candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l96-24`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l142-36`,
`requirement-candidate-content-clauses-04-federation-management-page-069-l70-17`,
`requirement-candidate-content-clauses-04-federation-management-page-072-l131-32`,
and `requirement-candidate-content-clauses-04-federation-management-page-076-l18-2`
describe federation save/restore lifecycle services, while the object-management
and time-management candidates describe Delete/Remove and retraction. The
exported model has no relation tying a live queued deletion and its
reconstitution ledger to a saved image, a post-save terminal Retract, restored
FQR delivery, and post-delivery Request Retraction.

**Umbra impact:** the new two-federate Catch2 scenario selects those anchors in
one dedicated plan entry and verifies the exact cross-service sequence. It is
development-profile traceability only; timed/durable restore, changed
membership, ownership races, transport, package, and conformance remain open.

**Possible Lab/tooling refinement:** add a save/restore relation with the
payload family, live retraction state, snapshot boundary, post-save mutation,
restore replacement semantics, delivery frontier, callback order, and object
reconstitution effects. This should be composable with the generic save and
timestamped deletion candidates without making either one appear to prove the
combined lifecycle alone.

### RL-134 — Timestamped directed interaction re-enable is a lifecycle relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned directed-interaction candidates
`requirement-candidate-content-clauses-05-declaration-management-page-101-l104-29`,
`requirement-candidate-content-clauses-08-time-management-page-184-l91-26`,
and `requirement-candidate-content-clauses-08-time-management-page-185-l17-5`
cover directed selector eligibility, timestamped queue admission, and the
grant callback boundary. The exported model does not relate one directed
payload to the recipient's `Disable Time Constrained` / callback-gated `Enable
Time Constrained` transition. It therefore cannot state that the target-
qualified queue identity remains tied to the same joined federate, that the
directed callback occurs exactly once, or that it precedes the matching grant
after re-enable.

**Umbra impact:** the new Catch2 case binds those candidates to a dedicated
directed lifecycle requirement. It checks target, tag, producer, timestamp,
order, retraction, and callback-before-grant metadata while intentionally
excluding selector mutation, directed DDM, alternate advances, save/restore,
transport, package evidence, and conformance. The contract remains
development-profile traceability only.

**Possible Lab/tooling refinement:** generalize the lifecycle relation used by
RL-131 and RL-132 with a directed-interaction payload kind and target identity.
The relation should carry the recipient joined-federate lifetime, the disable /
re-enable transition, callback cardinality and order, the grant service, and
the terminal retraction state, while remaining composable with the selector
and timestamped-eligibility candidates.

### RL-135 — Save/restore of a live directed TSO record needs a cross-service relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned save/restore candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l96-24`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l142-36`,
`requirement-candidate-content-clauses-04-federation-management-page-069-l70-17`,
`requirement-candidate-content-clauses-04-federation-management-page-072-l131-32`,
and `requirement-candidate-content-clauses-04-federation-management-page-076-l18-2`
describe save/restore services, while the directed timestamped candidates
describe queue admission and Request Retraction. The exported model does not
relate a target-qualified live directed payload and its per-recipient ledger to
the saved image, a post-save terminal Retract, restored Flush Queue delivery,
and the post-delivery Request Retraction consequence.

**Umbra impact:** the new Catch2 case selects those service anchors in one
dedicated plan entry. It proves the original directed callback and public
designator return after restore, then proves a legal Request Retraction with
the target, producer, timestamp, order, and tag preserved. This remains
development-profile traceability only; timed/durable restore, selector
mutation, alternate advances, directed DDM, changed membership/ownership,
transport, package evidence, and conformance remain open.

**Possible Lab/tooling refinement:** add a save/restore relation parameterized
by payload family, target identity, live/tombstone state, recipient ledger,
snapshot boundary, post-save mutation, restore replacement, delivery frontier,
callback order, and retraction consequence. Keep it composable with the
generic save/restore and directed timestamped candidates instead of treating
either source record as proof of the combined lifecycle.

### RL-136 — Save/restore of a live regional TSO record needs a cross-service relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned save/restore candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l96-24`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l142-36`,
`requirement-candidate-content-clauses-04-federation-management-page-069-l70-17`,
`requirement-candidate-content-clauses-04-federation-management-page-072-l131-32`,
and `requirement-candidate-content-clauses-04-federation-management-page-076-l18-2`
describe save/restore services, while the regional DDM and timestamped
interaction candidates describe overlap, source-region conveyance, queue
admission, and Request Retraction. The exported model does not relate an
overlap-qualified live regional payload and its source RegionHandle set to a
saved image, a post-save terminal Retract, restored Flush Queue delivery, and
the post-delivery Request Retraction consequence.

**Umbra impact:** the new Catch2 case selects those service anchors in one
dedicated plan entry. It proves the original regional callback and conveyed
source RegionHandle set return after restore, then proves a legal Request
Retraction with the timestamp, order, producer, and tag preserved. This remains
development-profile traceability only; timed/durable restore, region mutation,
passive/relaxed-DDM variants, alternate advances, changed membership/ownership,
transport, package evidence, and conformance remain open.

**Possible Lab/tooling refinement:** add a save/restore relation parameterized
by payload family, overlap/source-region realization, live/tombstone state,
recipient ledger, snapshot boundary, post-save mutation, restore replacement,
delivery frontier, callback order, and retraction consequence. Keep it
composable with the generic save/restore, regional DDM, and timestamped
interaction candidates instead of treating any one source record as proof of
the combined lifecycle.

### RL-137 — Save/restore of a live regional attribute TSO record needs a cross-service relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned save/restore candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l96-24`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l142-36`,
`requirement-candidate-content-clauses-04-federation-management-page-069-l70-17`,
`requirement-candidate-content-clauses-04-federation-management-page-072-l131-32`,
and `requirement-candidate-content-clauses-04-federation-management-page-076-l18-2`
describe save/restore services, while the object-management and regional DDM
candidates describe Update Attribute Values, active overlap, source-region
association, and Reflect Attribute Values. The exported model does not relate
one live queued regional attribute passel and its source RegionHandle set to a
saved image, a post-save terminal Retract, restored Flush Queue delivery, and
the post-delivery Request Retraction consequence.

**Umbra impact:** the new Catch2 case selects those service anchors in one
dedicated plan entry. It proves the restored object/update association, source
RegionHandle set, attribute values, timestamp/order/tag/producer metadata, and
legal Request Retraction after delivery. This remains development-profile
traceability only; timed/durable restore, region mutation, passive/relaxed-DDM
variants, alternate advances, changed membership/ownership, transport, package
evidence, and conformance remain open.

**Possible Lab/tooling refinement:** extend the save/restore relation with an
attribute-update payload kind, object identity, source-region realization,
recipient ledger, live/tombstone state, snapshot boundary, post-save mutation,
restore replacement, delivery frontier, callback order, and retraction
consequence. Keep it composable with the generic save/restore, object
management, and regional DDM candidates.

### RL-138 — Save/restore of a live default-source TSO interaction needs a cross-service relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned save/restore candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l96-24`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l142-36`,
`requirement-candidate-content-clauses-04-federation-management-page-069-l70-17`,
`requirement-candidate-content-clauses-04-federation-management-page-072-l131-32`,
and `requirement-candidate-content-clauses-04-federation-management-page-076-l18-2`
describe save/restore services, while the timestamped interaction and regional
DDM candidates describe queue admission, overlap, source realization, callback
projection, and Request Retraction. The exported model does not relate one
live queued interaction whose source is the private default region to a saved
image, a post-save terminal Retract, restored Flush Queue delivery with the
supplied-empty RegionHandleSet marker, and the post-delivery Request Retraction
consequence.

**Umbra impact:** the new Catch2 case selects those service anchors in one
dedicated plan entry. It proves that restore reconstructs the payload and
recipient ledger without exposing the derived default RegionHandle, preserves
timestamp/order/tag/producer/retraction metadata, and keeps the original
designator legal for Request Retraction after delivery. This remains
development-profile traceability only; timed/durable restore, explicit-source
replacement, passive/relaxed-DDM variants, alternate advances, changed
membership/ownership, transport, package evidence, and conformance remain open.

**Possible Lab/tooling refinement:** add a save/restore relation parameterized
by payload family, default-versus-explicit source realization, overlap and
conveyed-region projection, live/tombstone state, recipient ledger, snapshot
boundary, post-save mutation, restore replacement, delivery frontier, callback
order, and retraction consequence. Keep it composable with the generic
save/restore, timestamped interaction, and regional DDM candidates rather than
treating any one source record as proof of the combined lifecycle.

### RL-139 — Save/restore of a live non-regional TSO attribute passel needs a cross-service relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned save/restore candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l96-24`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l142-36`,
`requirement-candidate-content-clauses-04-federation-management-page-069-l70-17`,
`requirement-candidate-content-clauses-04-federation-management-page-072-l131-32`,
and `requirement-candidate-content-clauses-04-federation-management-page-076-l18-2`
describe save/restore services, while the object-management and time-management
candidates describe a timestamped Update Attribute Values passel, ordinary
subscription eligibility, queued reflection, and Request Retraction. The
exported model does not relate one live non-regional typed attribute passel and
its recipient ledger to a saved image, a post-save terminal Retract, restored
Flush Queue reflection, and the post-delivery Request Retraction consequence.

**Umbra impact:** the new Catch2 case selects those service anchors in one
dedicated plan entry. It proves that restore reconstructs the ordinary object
update payload and recipient state, preserves timestamp/order/tag/producer and
retraction metadata, and keeps the original designator legal for Request
Retraction after delivery. This remains development-profile traceability only;
timed/durable restore, regional/default-source variants, passive/relaxed-DDM
variants, alternate advances, changed membership/ownership, transport, package
evidence, and conformance remain open.

**Possible Lab/tooling refinement:** add a save/restore relation parameterized
by payload family, object and attribute set, ordinary versus regional source
projection, live/tombstone state, recipient ledger, snapshot boundary,
post-save mutation, restore replacement, delivery frontier, callback order,
and retraction consequence. Keep it composable with the generic save/restore,
object-management, and timestamped-attribute candidates.

### RL-140 — Save/restore of a live default-source TSO attribute passel needs a cross-service relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The pinned save/restore candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l96-24`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l142-36`,
`requirement-candidate-content-clauses-04-federation-management-page-069-l70-17`,
`requirement-candidate-content-clauses-04-federation-management-page-072-l131-32`,
and `requirement-candidate-content-clauses-04-federation-management-page-076-l18-2`
describe save/restore services, while the object-management and regional DDM
candidates describe ordinary registration, private default-source overlap,
timestamped Update Attribute Values, supplied-empty sent-region projection,
and Request Retraction. The exported model does not relate one live
default-source attribute passel to a saved image, a post-save terminal Retract,
restored Flush Queue reflection, and the post-delivery Request Retraction
consequence.

**Umbra impact:** the new Catch2 case selects those service anchors in one
dedicated plan entry. It proves that restore reconstructs the ordinary object
update payload and recipient state without exposing a synthetic RegionHandle,
preserves timestamp/order/tag/producer/retraction metadata, and keeps the
original designator legal for Request Retraction after delivery. This remains
development-profile traceability only; timed/durable restore, explicit-source
replacement, passive/relaxed-DDM variants, alternate advances, changed
membership/ownership, transport, package evidence, and conformance remain
open.

**Possible Lab/tooling refinement:** add a save/restore relation parameterized
by payload family, ordinary versus private default source realization, overlap
and conveyed-region projection, live/tombstone state, recipient ledger,
snapshot boundary, post-save mutation, restore replacement, delivery frontier,
callback order, and retraction consequence. Keep it composable with the generic
save/restore, timestamped-attribute, and regional DDM candidates.

### RL-141 — Timed save/restore of a live TSO passel needs a boundary relation

**Status:** verified local modeling gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The timed-save candidates
`requirement-candidate-content-clauses-04-federation-management-page-065-l100-25`,
`requirement-candidate-content-clauses-04-federation-management-page-065-l109-26`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l37-8`,
`requirement-candidate-content-clauses-04-federation-management-page-067-l40-9`,
and `requirement-candidate-content-clauses-04-federation-management-page-076-l18-2`
describe timed-save admission, while the object-management, time-management,
default-region, directed-interaction, and restore candidates describe the queued timestamped
attribute/interaction/directed payloads, their private default source or target,
saved image, and
reconstituted delivery. The
exported model does not relate the save timestamp boundary (before the queued
timestamp), constrained and regulating member grants, post-save terminal
Retract, restore completion, and the later FQR reflection/Request Retraction
sequence.

**Umbra impact:** dedicated Catch2 cases select those service anchors: one for
the default-source attribute passel, one for the default-source interaction
companion, and one for the target-qualified directed companion. Each proves its
timestamp-eight passel is still absent when the timestamp-six save begins,
survives the completed save, is terminalized by a post-save Retract, and is
restored for FQR delivery with the original metadata and post-delivery Request
Retraction. This remains
development-profile traceability only; durable restore, other advance modes,
timed boundaries for the other payload families, changed membership/ownership,
transport, package evidence, and conformance remain open.

**Possible Lab/tooling refinement:** add explicit save-boundary parameters for
scheduled save time, queued payload timestamp, per-member admission frontier,
post-save mutation, restore replacement, and the delivery/retraction frontier
to the generic save/restore relation.

### RL-142 — HLAmodifyAttributeState has no row-level Requirements Lab candidate

**Status:** verified local traceability gap; not a Requirements Lab extraction
defect, standards finding, or conformance evidence.

The 2025 Requirements Lab export currently contains 1,697 requirement
candidates. A bounded search of the exported `requirements.json` found no
candidate whose statement or identifier names `HLAmodifyAttributeState` or
`HLAattributeState`. The closest exported records are the generic MOM receive
and leaf-class candidates
`requirement-candidate-content-clauses-11-management-object-model-page-290-l18-5`
and
`requirement-candidate-content-clauses-11-management-object-model-page-290-l36-11`,
the HLAadjust information-shape candidate
`requirement-candidate-content-clauses-10-support-services-page-287-l108-35`,
the clause-level ownership transition candidate
`requirement-candidate-content-clauses-07-ownership-management-page-148-l121-38`,
and the overview ownership/RTI-owned-state candidate
`requirement-candidate-content-clauses-01-overview-page-018-l4-1`.

**Umbra impact:**
`compliance/requirements-lab/mom-modify-attribute-state-requirements-contract.json` and the
matching Catch2 plan entry select only those exported IDs. The contract keeps
the Table 18/20 leaf name, inherited parameters, target-publication guard,
callback-free direct transition, and RTI-owned MOM guard in explicit notes
instead of fabricating a candidate ID. The contract check passes with
`requirements-lab: baseline or contract matches the supplied bundle`.

**Possible Lab/tooling refinement:** export row-level candidates for the MOM
interaction and parameter tables, including the target-federate coordinate,
Owned/Unowned state encoding, target-known/publication preconditions,
RTI-owned predefined-attribute exclusion, no-notification behavior, and the
required MOM exception-report consequence. Those facets should remain linked
to the generic MOM receive/subscribe candidates rather than replacing them.

### RL-143 — MOM current-FDD contract exposed a stale source-file association

**Status:** verified local contract drift caught by the CTest traceability
gate; not a Requirements Lab extraction defect, standards finding, or
conformance evidence.

The generated `mom_service_reporting_requirements_traceability` test rejected
`umbra-mom-current-fdd-refresh-delivery` because its contract named
`cpp/src/internal/runtime/umbra_rti_ambassador.cpp` while declaring the symbol
`EmbeddedFederationRegistry::joinedFederateMomObjectAttributeValue`. The
symbol is defined in `cpp/src/internal/federation/federation_registry.cpp`; the source
path was corrected there and the contract remains otherwise unchanged.

**Umbra impact:** a source-file/symbol mismatch can remain hidden until a CMake
reconfigure regenerates the traceability catalog. The correction restores the
contract's source evidence and keeps the existing current-FDD Catch2 and
JPype test selectors intact. Rerun the failed traceability test and the full
serial catalog after any contract source move.

**Possible Lab/tooling refinement:** have the contract checker report the
resolved symbol's first matching file (or offer a repository-wide symbol
lookup) alongside the mismatch, so a stale source path is distinguishable from
an absent implementation symbol. Keep explicit source paths in contracts for
reviewability.

### RL-144 — The lookahead non-negative-data-type rule has no executable DIF predicate

**Status:** verified source/schema underspecification; implementation boundary
decision, not a standards or conformance failure.

The 2025 semantic record
`requirement-candidate-sections-semantic-clauses-5-6-page-093-l42-8` says only
that “Representations of lookahead shall be drawn from non-negative data
types.” The adjacent time-table record
`requirement-candidate-sections-semantic-clauses-5-6-page-093-l46-9` defines the
allowed declaration families, but neither record defines “non-negative data
type” in terms of a DIF element, an XSD type, a basic-representation property,
or a recursively checkable data-domain rule. The 2025 DIF/XSD likewise has no
sign/domain marker that a validator could use for this purpose.

The ambiguity is observable in the supplied authority corpus: the standard
MIM and Restaurant FOM use `HLAinteger64Time` for `logicalTimeInterval`, while
the underlying standard basic representation is `HLAinteger64BE`. A generic
signed-versus-unsigned inference would therefore reject the official sample
without evidence that it is non-conforming. The runtime interval factories
already reject negative lookahead values; that runtime value rule does not
provide a FOM declaration-level domain predicate.

**Umbra impact:** the private composer enforces the precise, executable
time-table category rule (simple, enumerated, array, fixed-record, or
variant-record, or `NA`) and deliberately does not invent a signedness or
semantics-text convention. The open lookahead item remains explicitly scoped
as unresolved; no contract, Catch2 result, or public conformance claim may
represent it as implemented.

**Possible Lab refinement:** add a normative definition or machine-readable
mapping for non-negative data types. At minimum, identify the permitted
standard time/interval types and provide positive and negative DIF fixtures;
for user-defined types, specify whether the determination is based on a
declared domain annotation, enumerator values, recursive member domains, or
reviewed semantics text. Export that interpretation alongside the candidate
record so tools can generate a deterministic test rather than guessing from
the type name.

### RL-145 — The large C++ Catch2 translation unit exceeds MSVC's default section limit

**Status:** verified test-build usability rough edge; toolchain mitigation, not
a standards or conformance finding.

The focused 2025 federation-management Catch2 translation unit grew beyond
MSVC's default object-file section limit. A normal Release build therefore
failed with `C1128: number of sections exceeded object file format limit` at
`cpp/tests/ieee1516_2025_federation_management_catch2.cpp`, even though the
production `umbra_rti` library compiled successfully. The test lane is
intentionally kept as one source file so its service matrix and Catch2
selectors remain easy to discover; splitting it would create a separate
navigation and ownership burden.

**Umbra impact:** the CMake build applies `/bigobj` to the standards-shaped
production RTI target and the focused Catch2 executable on MSVC. The focused
test-target setting restores a reproducible build without changing the public
headers or runtime semantics. It should be retained as a documented
development-profile requirement and should not be mistaken for evidence that
the Requirements Lab or IEEE source is incomplete.

**Possible Lab/tooling refinement:** the Lab's C++ test-generation guidance
could recommend a platform-aware large-object-file option (or a source-file
sharding convention) once generated conformance suites exceed compiler object
limits. The generated test metadata should keep stable case IDs when a file is
split or compiled with a large-object option.

### RL-146 — Support-service Table 5 return forms are not exported as row-level candidates

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

The rendered 2025 §10.21--§10.26 pages define the supplied and returned
arguments for the six support lookups: available dimensions for object and
interaction classes, dimension handle/name/upper-bound conversion, and Get
Dimension Handle Set. The official MIM binds those forms to types 36, 27, 11,
53, 10, 35, and 42. At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the generic §11.5.1 service-report
candidates (`requirement-candidate-content-clauses-11-management-object-model-page-292-l18-5`
and its neighboring report candidate) do not expose a row-level relation for
the Table 5 returned-argument type/name/value cells.

**Umbra impact:** the focused C++ unit and filesystem integration lane can
anchor each report to the exact rendered service definition and official MIM
enumerator while remaining honest about provenance. The contract checker
passes because the neighboring candidate IDs are valid, but that pass is not
validation of the private file `ReturnArgument` shape or of public MOM
interaction delivery.

**Possible Lab/tooling refinement:** export immutable Table 5 cell records and
a structured relation from each §10 service's supplied/returned argument row to
the corresponding type/name/value cell. Preserve the page-level candidate for
context, but allow a generated test to select the exact six support forms
without inferring them from implementation code or a generic service-report
sink candidate.

### RL-147 — Order and transportation lookup return forms are not exported as row-level candidates

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

The rendered 2025 §10.17--§10.20 pages define the supplied and returned
arguments for `Get Order Type`, `Get Order Name`, `Get Transportation Type
Handle`, and `Get Transportation Type Name`. The official MIM and Table 5
forms bind those services to type-53 `String`, type-38 `OrderType`, and type-59
`TransportationTypeHandle`; the order value is the quoted `RECEIVE` or
`TIMESTAMP` spelling and the transportation value is quoted
`handle.toString()` text. At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the generic §11.5.1 service-report
candidates do not expose row-level relations for those Table 5 returned-
argument type/name/value cells.

**Umbra impact:** the focused C++ unit and filesystem integration lane anchors
the four accepted lookups to the rendered service definitions and official MIM
helper types while explicitly remaining nonvalidated and nonconforming. Paired
filesystem and HLA_IMMEDIATE failure matrices now preserve the type-53/type-38/
type-59 supplied forms, Null returns, false indicators, public exception text,
and serial order; an invalid `OrderType` uses a deterministic `UNSUPPORTED`
diagnostic because the closed encoder has no canonical invalid-enum spelling.
The contract checker can verify the neighboring page-level candidate and
source symbol, but that pass does not validate the private file `ReturnArgument`
shape or public MOM interaction delivery.

**Possible Lab/tooling refinement:** export immutable Table 5 cell records and
service-level relations for each §10 supplied/returned argument row, including
the order enum spellings and transportation handle `toString()` rule. Preserve
the page-level candidate for context while allowing generated tests to select
these four forms without inferring them from implementation code or a generic
service-report sink candidate.

### RL-148 — Handle-normalization Table 5 return forms are not exported as row-level candidates

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

The rendered 2025 §10.29--§10.33 pages define the supplied and returned
arguments for `Normalize Service Group`, `Normalize Federate Handle`,
`Normalize Object Class Handle`, `Normalize Interaction Class Handle`, and
`Normalize Object Instance Handle`. The supplied forms are respectively type
50 `ServiceGroup`, type 15 `FederateHandle`, type 36 `ObjectClassHandle`, type
27 `InteractionClassHandle`, and type 37 `ObjectInstanceHandle`; each returned
`Normalized value` is represented by the Table 5 type-35 `Number` form. The
MIM's `HLAnormalizedServiceGroup` and `HLAnormalized*Handle` datatype names
describe normalized data dimensions, not additional Table 5 service-report
argument types. At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the generic §11.5.1 service-report
candidates do not export row-level relations for these supplied/returned
cells.

**Umbra impact:** the focused C++ unit and filesystem integration lane anchors
all five services to the rendered source pages and official MIM types while
remaining explicitly nonvalidated and nonconforming. The contract checker can
verify the neighboring page-level candidate and source symbol, but that pass
does not validate the private file `ReturnArgument` shape, normalized value
semantics, or public MOM interaction delivery.

**Possible Lab/tooling refinement:** export immutable Table 5 cell records and
service-level supplied/returned relations for §10.29--§10.33, including the
distinction between type-35 Number service-report values and the separate
`HLAnormalized*` MIM datatypes. Preserve the page-level candidate for context
while allowing generated C++ tests to select these five forms without
inferring them from implementation code or a generic service-report sink
candidate.

### RL-149 — Federate and object-class lookup Table 5 forms are not exported as row-level candidates

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

The rendered 2025 §10.2--§10.5 pages define the supplied and returned
arguments for `Get Federate Handle`, `Get Federate Name`, `Get Object Class
Handle`, and `Get Object Class Name` (source pages 245--247). The official MIM
and Table 5 forms bind those pairs to type-53 `String` with type-15
`FederateHandle`, and type-53 `String` with type-36 `ObjectClassHandle`, with
the reverse services exchanging the same forms. At pinned Requirements Lab
revision `4f012fb1c21367cfde67aab8498ae00e2a64c615`, the generic §11.5.1
service-report candidates do not export row-level relations for these Table 5
supplied/returned type, name, and value cells.

**Umbra impact:** the focused C++ unit and filesystem integration lane anchors
all four lookups to the rendered service definitions and official MIM types
while explicitly remaining nonvalidated and nonconforming. The contract
checker can verify the neighboring page-level candidate and source symbol,
but that pass does not validate the private file `ReturnArgument` shape or
public MOM interaction delivery.

**Possible Lab/tooling refinement:** export immutable Table 5 cell records and
service-level supplied/returned relations for §10.2--§10.5, including the
federate/object-class name and handle type pairs. Preserve the page-level
candidate for context while allowing generated C++ tests to select these four
forms without inferring them from implementation code or a generic
service-report sink candidate.

### RL-150 — Interaction-class and parameter lookup Table 5 forms are not exported as row-level candidates

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

The rendered 2025 §10.13--§10.16 pages define the supplied and returned
arguments for `Get Interaction Class Handle`, `Get Interaction Class Name`,
`Get Parameter Handle`, and `Get Parameter Name` (source pages 252--255).
The official MIM and Table 5 forms bind the first pair to type-53 `String`
and type-27 `InteractionClassHandle`. The parameter pair uses the
interaction-class handle plus type-53 `Parameter name` in one direction and
type-27 plus type-39 `ParameterHandle` in the reverse direction. At pinned
Requirements Lab revision `4f012fb1c21367cfde67aab8498ae00e2a64c615`, the
generic §11.5.1 service-report candidates do not export row-level relations
for these Table 5 supplied/returned type, name, and value cells.

**Umbra impact:** the focused C++ unit and filesystem integration lane anchors
all four lookups to the rendered service definitions and official MIM types
while explicitly remaining nonvalidated and nonconforming. The contract
checker can verify the neighboring page-level candidate and source symbol,
but that pass does not validate the private file `ReturnArgument` shape or
public MOM interaction delivery.

**Possible Lab/tooling refinement:** export immutable Table 5 cell records and
service-level supplied/returned relations for §10.13--§10.16, including the
interaction-class/parameter handle and name pairs. Preserve the page-level
candidate for context while allowing generated C++ tests to select these four
forms without inferring them from implementation code or a generic
service-report sink candidate.

### RL-151 — Known-object, object-instance, attribute, and update-rate lookup Table 5 forms are not exported as row-level candidates

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

The rendered 2025 §10.6--§10.12 pages define the supplied and returned
arguments for `Get Known Object Class Handle`, `Get Object Instance Handle`,
`Get Object Instance Name`, `Get Attribute Handle`, `Get Attribute Name`, `Get
Update Rate Value`, and `Get Update Rate Value For Attribute` (source pages
247--253). The official Table 5/MIM forms use type-37 ObjectInstanceHandle,
type-36 ObjectClassHandle, type-53 String names, type-0 AttributeHandle, and
type-35 Number maximum-update-rate values. At pinned Requirements Lab revision
`4f012fb1c21367cfde67aab8498ae00e2a64c615`, the generic §11.5.1 service-report
candidates do not export row-level relations for these supplied/returned type,
name, and value cells.

**Umbra impact:** the focused C++ unit and filesystem integration lane anchors
all seven lookups to the rendered service definitions and official MIM values
while explicitly remaining nonvalidated and nonconforming. The contract checker
can verify the neighboring page-level candidate and source symbol, but that
pass does not validate the private file `ReturnArgument` shape or public MOM
interaction delivery. Setup lookups are switch-gated so the focused operation
keeps a deterministic serial sequence without hiding the newly reportable
successful calls from the broader regression lane.

**Possible Lab/tooling refinement:** export immutable Table 5 cell records and
service-level supplied/returned relations for §10.6--§10.12, including the
object/object-instance/attribute handle/name pairs and the type-35 maximum
update-rate value. Preserve the page-level candidate for context while allowing
generated C++ tests to select these seven forms without inferring them from
implementation code or a generic service-report sink candidate.

### RL-152 — Failed service-report return/exception relation is not exported as a row-level candidate

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

IEEE 1516.1-2025 §11.5.1 states that `HLAreturnedArgument` uses the Null
`HLAargumentType` when `HLAsuccessIndicator` is false, while the service-report
record carries the textual `HLAexception` description (source page 292; the
MOM parameter semantics are summarized on source page 349). At pinned
Requirements Lab revision `4f012fb1c21367cfde67aab8498ae00e2a64c615`, the
exported §11.5.1 candidates identify service reporting generally but do not
export a row-level conditional relation tying a failed invocation to its Null
return and exception fields.

**Umbra impact:** the bounded C++ failure lane now covers the seven
§10.6--§10.12 known-object, object-instance, attribute, and update-rate lookup
services, the four §10.13--§10.16 interaction-class and parameter lookups, the
four §10.17--§10.20 order and transportation lookups, and the six §10.21--§10.26
dimension and region lookups, and the five §10.29--§10.33 normalization
lookups. Each paired matrix preserves its official
supplied argument forms, records a Null returned argument,
`HLAsuccessIndicator:false`, and the public exception description, and advances
the same per-federate serial stream. The §10.17--§10.20 invalid
`OrderType` case uses a deterministic `UNSUPPORTED` type-38 diagnostic because
the closed encoder has no canonical invalid-enum spelling. The exact file
HLA_IMMEDIATE public MOM matrices decode service type 6 and the same typed
forms, including type-36/type-27/type-53/type-10/type-42 for §10.21--§10.26
and type-50/type-15/type-36/type-27/type-37 for §10.29--§10.33. The latter
also preserves deterministic `UNSUPPORTED` text for an invalid ServiceGroup
enum. The adjacent DDM pair now covers failed §9.2 Create Region and §10.27
Get Range Bounds with type-11 or type-42/type-10 supplied forms, Null returns,
false indicators, exception text, and serials zero/one before successful
serials two/three; its public interaction matrix uses service type 5.
The adjacent §9.3/§9.4/§10.28 mutation matrix covers invalid region sets,
invalid region handles, invalid dimensions, and invalid range bounds for
Commit Region Modifications, Delete Region, and Set Range Bounds with the same
Null/false/exception relation and serials zero through three; the same public
interaction matrix now carries successful SetRangeBounds,
CommitRegionModifications, and DeleteRegion records with true indicators,
Null returns, and serials four through six.
The paired §9.10/§9.11 regional interaction-subscription matrices now use the
same bounded evidence shape: invalid interaction-class and region-set inputs
retain type-27/type-43 (and Subscribe's type-6 passive-subscription) supplied
forms, Null returns, false indicators, exception text, switch-gated
suppression, and stable serial ordering in both filesystem and HLA_IMMEDIATE
MOM lanes, with accepted regional Subscribe/Unsubscribe calls visible as
successful records. The dedicated accepted-success HLA_IMMEDIATE case also
proves the type-6 passive indicator is inverted for a passive subscription,
then returns to false for an active replacement before the unsubscription
record. The Lab still has no row-level conditional failure relation for this
pair, so the new evidence remains development traceability.
The paired §9.8/§9.9 regional object-class subscription matrices now extend
that evidence to type-36/type-4 object and attribute/region-pair forms, with
Subscribe's type-6 passive and type-53/type-34 update-rate slots, Null
returns, false indicators, exception text, switch-gated suppression, and
stable serial ordering in both filesystem and HLA_IMMEDIATE MOM lanes. The
same missing conditional failure relation (RL-152) applies, so this remains
development traceability rather than Lab validation.
The adjacent §9.6/§9.7 regional object-attribute association matrices add
type-37/type-4 object and attribute/region-pair forms, Null returns, false
indicators, exception text, switch-gated suppression, and stable serial
ordering in both filesystem and HLA_IMMEDIATE MOM lanes, with accepted
Associate/Unassociate calls visible as successful records. The same missing
conditional failure relation (RL-152) applies, so this remains development
traceability rather than Lab validation.
The paired §5.4/§5.5 interaction-class declaration matrices apply the same
conditional relation to type-27 Publish/Unpublish supplied forms. Filesystem
and HLA_IMMEDIATE lanes preserve Null returns, false indicators, exact public
exception text, and serials zero/one before accepted records at serials two and
three. This is useful C++ evidence, but the Lab still exports only the generic
§11.5.1 service-report candidates and no row-level failed-outcome relation.
The adjacent §5.10/§5.11 Subscribe/Unsubscribe matrices extend that evidence
with Subscribe's type-6 passive indicator, while preserving the same Null,
false, exception, and serial relation in both sinks. The Lab export remains
generic, so these are development traceability cases rather than Lab
validation.
The directed §5.6/§5.7 overload matrices extend it across explicit-set
type-36/type-28 forms and the whole-class Unpublish type-34 Null slot, with
serials zero through three for failures and four through six for accepted
records in both sinks. The Lab still has no row-level conditional relation,
so this remains development traceability rather than validation.
The paired directed §5.12/§5.13 subscription matrices extend the same
source/export gap to Subscribe/Unsubscribe Object Class Directed Interactions:
type-36 object-class and type-28 directed-interaction-set forms, Subscribe's
type-6 universal selector, and whole-class Unsubscribe's type-34 Null slot are
preserved with Null returns, false indicators, exception text, and serials
zero through three before accepted records four through six in both sinks.
Because the Lab still exports only the generic §11.5.1 candidate and no
row-level conditional failure relation, these remain development traceability
cases rather than Lab validation.
The paired §5.2/§5.3 object-attribute declaration matrices expose the same
source/export gap for type-36 ObjectClassHandle and type-1
AttributeHandleSet forms. They preserve Null returns, false indicators,
exception text, and failure serials zero through three before accepted
Publish/Unpublish records four and five in both filesystem and HLA_IMMEDIATE
sinks. The generic §11.5.1 candidate still has no row-level conditional
failure relation, so these remain development traceability rather than Lab
validation.
The companion §5.8/§5.9 object-class attribute subscription matrices extend
the same gap across subset and whole-class unsubscription. They preserve
type-36/type-1 forms, Subscribe's type-6 passive and type-53 update-rate
arguments, the whole-class type-34 Null slot, Null returns, false indicators,
exception text, and failure serials zero through four before accepted records
five through seven in both sinks. The generic candidate still has no
row-level conditional failure relation, so these remain development
traceability rather than Lab validation.
The new §6.2/§6.4 single-name reservation failure matrices apply the same
source/export gap to `Reserve Object Instance Name` and `Release Object
Instance Name`. Filesystem and HLA_IMMEDIATE lanes preserve the type-53
`Name` supplied value, Null returned argument, false indicator, exact
`IllegalName`/`ObjectInstanceNameNotReserved` exception text, and serials zero
and two around accepted records at one and three. The generic §11.5.1
candidate still has no conditional failure relation, so this remains
development traceability rather than Lab validation or conformance.
The §6.18 Local Delete Object Instance matrix applies the same missing
conditional relation to a one-argument object-management service. Filesystem
and HLA_IMMEDIATE lanes preserve type-37 supplied handles, Null returns, false
indicators, exact `ObjectInstanceNotKnown` descriptions, and serials zero and
two around the accepted serial-one local-forget record. The generic §11.5.1
candidate still has no row-level failed-outcome relation, so this remains
development traceability rather than Lab validation or conformance.
The paired §6.16 receive-order `Delete Object Instance` matrices expose the
same Lab gap for the three-slot object-removal service. Filesystem and
HLA_IMMEDIATE evidence preserves type-37 object handles, type-63 base-64 user
tags, type-34 Null optional timestamps, Null returns, false indicators, exact
`ObjectInstanceNotKnown` descriptions, and serials zero and two around the
accepted serial-one deletion. Accepted emission is outside native locks and
precedes the queued removal callback. Because the Lab still exports no
row-level conditional failure relation, this is development traceability only
(RL-152), not Lab validation or conformance.
The timestamped §6.16 failure pair records the same conditional relation gap
at the TSO pre-admission boundary. Invalid object and lower-than-lookahead
timestamp calls preserve type-37/type-63/type-31 supplied forms, Null returns,
false indicators, exact `ObjectInstanceNotKnown`/`InvalidLogicalTime` text, and
serials zero and one in both the filesystem and HLA_IMMEDIATE sinks. The
accepted timestamped sender file path is not inferred from this failure-only
case; public interaction success and recipient-local §6.17 callback reporting
remain separate. A subsequent native filesystem companion now covers that
accepted sender path through the shared selector, proving type-37/type-63/
type-31 supplied forms, the type-34 Null return for a non-time-regulating
sender, serial zero, and durable sender-report-before-Remove Object Instance
callback ordering. This is development traceability only (RL-152), not Lab
validation or conformance.
The paired timestamped §6.10 `Update Attribute Values` failure matrices extend
the same missing conditional relation to TSO object updates. Invalid object and
attribute designators and a timestamp below current logical time plus lookahead
preserve type-37/type-2/type-63/type-31 supplied forms, Null returns, false
indicators, exact `ObjectInstanceNotKnown`/`AttributeNotDefined`/
`InvalidLogicalTime` descriptions, and serials zero through two in both sinks.
The Lab still exports no row-level failed-outcome relation, so accepted sender
output and broader TSO delivery are intentionally separate; this is development
traceability only (RL-152), not Lab validation or conformance.
The accepted timestamped §6.10 `Update Attribute Values` filesystem companion
now covers that sender backend through the shared selector. It proves
type-37/type-2/type-63/type-31 supplied forms, the type-34 Null return for a
non-time-regulating sender, serial zero, and durable sender-report-before-
`Reflect Attribute Values` callback ordering. RL-105/RL-152 still provide no
row-level report-backend or failed-outcome relation, so regional/default-region,
time-regulated, callback-file, Lab-validation, and conformance evidence remain
separate.
The paired timestamped §6.12 `Send Interaction` failure matrices extend the
same missing conditional relation to TSO interactions. Invalid interaction-class
and parameter designators and a timestamp below current logical time plus
lookahead preserve type-27/type-40/type-63/type-31 supplied forms, Null returns,
false indicators, exact `InteractionClassNotDefined`/
`InteractionParameterNotDefined`/`InvalidLogicalTime` descriptions, and serials
zero through two in both sinks. The Lab still exports no row-level failed-
outcome relation, so accepted sender output, retraction, and recipient delivery
are intentionally separate; this is development traceability only (RL-152),
not Lab validation or conformance.
The paired timestamped §6.14 `Send Directed Interaction` failure matrices
extend the same missing conditional relation to directed TSO sends. Invalid
interaction-class, target-object, and parameter designators and a timestamp
below current logical time plus lookahead preserve type-27/type-37/type-40/
type-63/type-31 supplied forms, Null returns, false indicators, exact
`InteractionClassNotDefined`/`ObjectInstanceNotKnown`/
`InteractionParameterNotDefined`/`InvalidLogicalTime` descriptions, and serials
zero through three in both sinks. The Lab still exports no row-level failed-
outcome relation, so accepted sender output, retraction, and directed delivery
are intentionally separate; this is development traceability only (RL-152),
not Lab validation or conformance.
This remains development-profile source traceability rather than Lab
validation or conformance; other service families and broader public
MOM-interaction matrices remain open.

**Possible Lab/tooling refinement:** export an immutable service-report field
relation for success and failure, including the conditional Null returned
argument and non-null exception text, so generated tests can exercise both
outcomes without inferring the relationship from the implementation or a
generic service-report sink candidate.

### RL-154 — FOM transportation declarations are not exported as a runtime handle mapping

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

The 2025 DIF/FDD transportation table declares names beyond the two mandatory
`HLAreliable` and `HLAbestEffort` rows.  The pinned Requirements Lab exports the
transportation-reference and API surfaces, but it does not provide a generated
relation from a composed transportation row to the execution-scoped
`TransportationTypeHandle` value returned by `Get Transportation Type Handle`
or back from that handle through `Get Transportation Type Name`.

**Umbra impact:** the focused C++ lane now parses the composed transportation
table, resolves a declared `UmbraTransportationFixture` name, assigns a
deterministic opaque value after the mandatory pair, and proves that both
joined federates observe the same name/handle mapping.  A companion native
case carries that mapping through ordinary receive-order interaction and
no-time attribute delivery, a second companion carries it through ordinary
regional interaction plus nonregional timestamped interaction and attribute
callbacks, and a third carries it through ordinary/timestamped regional
interaction and attribute callbacks plus ordinary/timestamped directed
interaction callbacks with source-region/target metadata.  The API contract
and Catch2 plan still keep remote paths, plus conformance, open; the passing traceability
check is not a Lab-backed validation of the private handle allocation or
delivery policy.

**Possible Lab/tooling refinement:** export immutable transportation-definition
rows and an execution-scoped lookup relation so generated tests can distinguish
mandatory standard handles from FOM-declared implementation-specific names and
can trace both lookup directions without inferring the mapping from source.

### RL-153 — FOM source-readability diagnostics have no row-level Lab relation

**Status:** verified source/export granularity gap; possible Lab refinement, not
a standards defect or conformance finding.

The 1516.1 federation-management candidate
`requirement-candidate-content-clauses-04-federation-management-page-048-l57-18`
requires the RTI to reject an attempt to load a FOM module in the listed
failure cases, but the exported record does not distinguish a missing
designator from an existing path that cannot be consumed as an XML source. It
also does not provide a row-level relation for the corresponding
`CouldNotOpenFOM` versus `ErrorReadingFOM` outcomes.

**Umbra impact:** the private libxml2 validator now makes that boundary
deterministic: a missing path remains `source_not_found`, while an existing
non-regular path is `source_unreadable`; the embedded Create Federation
Execution adapter maps the latter to `ErrorReadingFOM` and proves that no
federation is committed before a later valid create. The focused native unit,
integration, contract, and CTest lane remain development-profile traceability
only. The local mitigation does not turn the broad Lab candidate into a
row-level conformance relation.

**Possible Lab/tooling refinement:** export immutable FOM-load failure rows
that distinguish source availability, source readability, XML parsing, and
model/schema validity, together with their official exception outcomes. This
would let generated tests bind the diagnostic boundary without inferring it
from implementation-specific filesystem probes.

### RL-156 — FOM P/S metadata is capability classification, not a declaration fence

**Status:** verified source-semantics boundary; possible Lab refinement, not a
standards defect or conformance finding.

The 2025 IEEE 1516.2 source describes the object-class, interaction-class, and
class-attribute `P/S` field as a capability designation. For a FOM or FOM
module, a class is classified as Publish or Subscribe when at least one
federate is capable of publishing or subscribing in that class context. The
relevant exported candidates are the object-structure continuation
`requirement-candidate-sections-semantic-clause-4a-page-040-l26-5`, the
interaction-structure continuation
`requirement-candidate-sections-semantic-clause-4a-page-045-l6-1`, and the
attribute-table continuation
`requirement-candidate-sections-semantic-clause-4b-page-050-l34-2`, with the
same broad `clause-4` ownership in the pinned `v0.1.0.a1` bundle.

**Umbra impact:** the composed catalog retains the supplied `sharing` values
as OMT metadata. The live publication and subscription sets remain owned by
the 1516.1 declaration-management services. Umbra must not reject a runtime
`Publish` or `Subscribe` call merely because a FOM row says `Subscribe`,
`Publish`, or `Neither`; doing so would invent a per-federate exception rule
that the cited OMT text does not state. Runtime declaration tests should
instead trace the official service preconditions and exceptions separately.

**Possible Lab/tooling refinement:** export table-row capability semantics as
a distinct relation from 1516.1 service preconditions, so generated tests can
distinguish FOM/SOM model classification from live declaration state without
requiring consumers to infer that distinction from source prose.

### RL-157 — Export revision labels do not freeze a dirty Lab working tree

**Status:** verified reproducibility/tooling boundary; possible Lab refinement,
not a standards defect or conformance finding.

The Umbra exporter passes the locked revision string through to the Lab's
public exporter, but it does not create a detached worktree or verify that the
Lab checkout is clean. At the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`, a fresh all-edition export from
the current adjacent checkout changed 32 state-machine endpoint fields in
2010 Part 1.1 figures while leaving all 2025 requirement/API/mapping IDs and
clause IDs unchanged. The commit hash alone would not explain that bundle
delta.

**Umbra impact:** the re-sync workflow now compares the new bundle against the
previous ignored export at field level, promotes only the fresh generated
bundle, and checks all 239 contract files plus the API baseline (240 typed
contracts total). It records the source checkout's dirty state in this log and
does not rewrite 2025 contracts merely because a revision label was reused.
The 235 Debug traceability tests remain green after the refresh.
A subsequent same-day export to the normal ignored bundle produced the same
2025 requirement/API/mapping records and no additional clause-number drift;
the repository-wide reference audit resolved all 1,594 typed Lab references
and all 243 Requirements-Lab JSON contracts parsed successfully.

The requested re-sync was repeated against the current adjacent checkout on
2026-08-24 with a fresh all-edition export. Its SHA-256 is
`4e0160b1e2dd81ba3df4f36911735f2cb04848d3ea3f45e1652943c6a0f255c2`, exactly
matching the ignored canonical bundle. A field-level comparison found zero
added, removed, or changed requirement, API-surface, and mapping records in
all six documents, including zero changes to 2025 ordinals or clause IDs.
All 240 typed contracts resolve against that fresh export with zero findings.
The adjacent checkout is still dirty, so this result is an audit of the
working tree rather than a new immutable Lab release; no Umbra requirement
references were renumbered.

**Possible Lab/tooling refinement:** make the exporter refuse a dirty checkout
unless explicitly overridden, or include a content digest and the resolved
source commit/working-tree status in the bundle. That would make revision
pinning reproducible for downstream consumers and distinguish committed
renumbering from local semantic regeneration.

### RL-158 — Export output-path errors are not actionable

**Status:** verified workflow/usability issue; possible Lab refinement, not a
standards or conformance finding.

During the 2026-08-24 re-sync, invoking the public exporter with `--output`
pointing at an existing directory caused a raw Python `PermissionError` when
the exporter attempted to write the JSON bundle. The supported invocation
requires a file path, but the command-line failure does not state that
requirement or identify the path-kind mismatch clearly.

**Umbra impact:** the re-sync was retried with an explicit `.json` file path;
the resulting bundle matched the canonical hash and no 2025 numbering or
clause references were changed. This did not affect the Requirements Lab
checker or any Umbra contract.

**Possible Lab/tooling refinement:** validate `--output` before export and
return a concise usage error when it is an existing directory (or explicitly
support directory output). The error should distinguish path-kind mistakes
from permission failures so a downstream consumer can correct the invocation
without reading a Python traceback.

### RL-159 — Multi-member pending-advance mapping re-sync

**Status:** verified traceability update; not a standards or conformance finding.

The new two-member HLA_EVOKED save/restore case is mapped to the current
2025 Requirements Lab IDs in `catch2-test-plan.json` and the restore-control
contracts. The 12 selected C++ API-surface IDs and 15 selected requirement IDs
all resolve against the fresh 2026-08-24 bundle; the repository-wide 243
contract checks remain green. No 2025 ordinal or clause ID changed during this
slice. The test evidence remains process-local development-profile evidence,
not a conformance result.

### RL-160 — Active Catch2 plan carried stale Lab references after re-sync

**Status:** verified traceability/tooling defect; possible Lab/tooling
refinement, not a standards or conformance finding.

The fresh all-edition export from the adjacent Requirements Lab checkout on
2026-08-24 was byte-for-byte identical to the pinned Umbra bundle
(`4e0160b1e2dd81ba3df4f36911735f2cb04848d3ea3f45e1652943c6a0f255c2`). It
contains six documents, 4,394 requirements, 2,077 API surfaces, 494 mappings,
494 transitions, 3,505 requirement/API bindings, and 2,258 API crosswalks.
Field-level comparison found no changed 2025 requirement ID, ordinal, clause
ID, API ID, or mapping ID.

The active `catch2-test-plan.json` nevertheless contained four requirement
references that resolved in no document of the fresh bundle: three stale
object-management IDs (`page-134-l21-3`, `page-134-l36-8`, and
`page-134-l39-9`) and one stale DDM ID (`page-217-l92-30`). It also contained
one nonexistent `createdimension` C++ API reference in four custom-transport
scenarios and one directed-subscription API hash missing its final `8`.
These were plan-side references, not bundle renumbering. The object-management
references were remapped to the current Clause 6.10 service candidates
`page-120-l83-22`, `page-120-l86-23`, and `page-120-l98-27`; the DDM reference
was remapped to the current same-semantics `page-219-l92-30` candidate. The
custom-transport cases now select the official `getDimensionHandle` API, which
the C++ test actually invokes, and the directed-subscription reference now
uses the canonical `...4c460a3c84d8` API ID.

**Umbra impact:** after the remap, all 536 Catch2 plan entries resolve 460
distinct requirement IDs and 243 distinct C++ API IDs with zero unresolved
references. The repository contains 239 typed `*-contract.json` files plus
the API baseline; all 240 pass the Requirements Lab checker against the fresh
export, and all 243 Requirements-Lab JSON files parse successfully. No source
implementation change was required.

**Possible Lab/tooling refinement:** add a first-class checker for
`catch2-test-plan.json` (including `selected_cpp_api_surface_ids`) and the
other planning/catalog JSON artifacts. The current contract checker validates
typed contracts but does not catch stale plan references, so a plan can appear
green while pointing at removed requirement or API IDs.

### RL-161 — Re-sync confirms plan growth, not Requirements Lab renumbering

**Status:** verified traceability audit; not a standards or conformance finding.

On 2026-08-24, Umbra exported the all-edition corpus again from the adjacent
Requirements Lab checkout into an ignored working bundle. The export retained
revision `4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` and SHA-256
`4e0160b1e2dd81ba3df4f36911735f2cb04848d3ea3f45e1652943c6a0f255c2`, exactly
matching `.compliance/corpus-bundle.json`. All six documents still contain
4,394 requirements, 2,077 API surfaces, 494 mappings, 494 transitions, 3,505
requirement/API bindings, and 2,258 API crosswalks; field-level ID comparison
found zero additions, removals, or changes, including for 2025 ordinals and
clause IDs.

The current `catch2-test-plan.json` resolves 536 entries, 463 distinct
requirement IDs, and 243 distinct C++ API IDs with zero unresolved references.
The increase from the 460 requirement IDs recorded in RL-160 is caused by new
ownership and DDM test-plan entries (with the four previously stale IDs removed),
not by a Requirements Lab numbering change. All 239 typed contracts plus the
API baseline pass the checker against this fresh export.

**Possible Lab/tooling refinement:** keep the proposed plan/catalog checker from
RL-160 and report plan-reference counts by source revision, so new test slices
are distinguishable from corpus renumbering in routine re-sync reports.

### RL-162 — TAR/NMR timestamped-deletion slice uses current 2025 IDs

**Status:** verified development-profile traceability and test evidence; not a
standards or conformance finding.

After the re-sync, the C++ lane added
`Embedded timestamped Delete Object Instance delivers before TAR and NMR grants`.
It queues one timestamp-7 deletion, advances one constrained recipient with
direct `Time Advance Request(7)`, advances a second with
`Next Message Request(10)`, and proves both `Remove Object Instance` callbacks
precede their grants at logical time 7 while the producer completes its
`Time Advance Request(2)`. The callback metadata assertions cover object,
tag, producing federate, timestamp, timestamp order, and valid retraction
designators; the producer's post-delivery `Retract` is terminal.

The plan entry selects the current 2025 APIs
`deleteObjectInstance`, `retract`, `timeAdvanceRequest`, `nextMessageRequest`,
`removeObjectInstance`, and `timeAdvanceGrant`, plus current Clause 6 and
Clause 8 requirement candidates. Against the fresh bundle, all 537 Catch2
plan entries resolve 463 distinct requirement IDs and 243 distinct C++ API
IDs. The focused Catch2 test passes 75 assertions; its existing
FQR/TARA/NMRA sibling remains green at 95 assertions. The registered CTest
entry passes, the time-management lane now contains 249 Catch2 tests, and the
two timestamped-deletion contracts pass the Requirements Lab checker.

This closes only the direct-TAR/NMR alternate-advance evidence slice. It does
not promote the development profile to conformance or complete the remaining
alternate-time, fanout, DDM, recovery, ownership/resignation, or transport
families.

### RL-163 — All-edition re-sync is blocked by an unrelated 2010 state-machine reference

**Status:** verified Requirements Lab exporter defect; not a 2025 numbering or
standards finding.

On 2026-08-24, a fresh all-edition export at Lab revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` failed during semantic validation
before writing a bundle. The Lab's state-machine validator reports that
transition `hla-1516.1-2010:figure-14.e01` targets the unknown state
`hla-1516.1-2010:figure-04.s01`. The source record is in the 2010 native
state-machine corpus (`figure-14.e01`, `figure-14.tex`, line 44); the target
belongs to the separate `figure-04` machine. This is a cross-machine endpoint
reference, not a changed 2025 requirement, ordinal, clause, API, or mapping ID.

The scoped 2025 export completed successfully and retained the same revision.
Its three documents are byte-for-byte equal to the 2025-document projection
of Umbra's canonical bundle: 20/1,860/340 requirements, 1,263 C++ API
surfaces, 282 mappings, and no changed 2025 IDs or clause bindings. The
all-edition failure therefore does not invalidate the current 2025 traceability
baseline, but it prevents a clean six-document re-sync until the Lab corpus is
repaired or the exporter supports excluding invalid unrelated editions.

**Umbra impact:** 2025 contracts and focused-lane planning remain checked
against the pinned canonical bundle; no Umbra references were renumbered or
silently rewritten. The failed all-edition command and successful 2025-only
command are retained as reproducible audit evidence for this revision.

**Possible Lab/tooling refinement:** either correct the 2010 transition target
to a state owned by `figure-14`, model an explicit cross-machine transition
namespace if that reference is intentional, or make edition-scoped export
validate only the selected documents. The exporter should identify the owning
state-machine IDs in its diagnostic and return a concise corpus-data error
instead of requiring downstream users to inspect a Pydantic traceback.

### RL-164 — 2025 re-sync and complete C++ evidence remain green

**Status:** verified 2025 traceability/test audit; not a standards or
conformance finding.

The scoped 2025 export from the adjacent Lab checkout completed at revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` and retained the canonical 2025
inventories: 20 Part 1 requirements, 1,860 Part 1.1 requirements, 340 Part
1.2 requirements, 1,263 Part 1.1 API surfaces, and 282 Part 1.1 mappings.
The current Catch2 plan contains 537 entries selecting 460 distinct
requirement IDs and 243 distinct C++ API-surface IDs; all resolve against the
2025-only bundle. All 240 typed Umbra contracts (including the API baseline)
also pass the checker with zero findings.

The rebuilt official-header C++ target then passed the complete embedded
Catch2 catalog: 770 test cases and 42,730 assertions. The focused CTest
catalog guards for tag taxonomy and service-lane slicing also pass in the
Debug configuration. This confirms that the requirements-numbering re-sync did
not leave stale references or a runtime regression in the current development
profile. It does not promote any development-profile result to conformance;
the all-edition export remains blocked by the unrelated 2010 state-machine
endpoint recorded in RL-163.

**Possible Lab/tooling refinement:** expose an edition-scoped export and plan
reference checker as one documented command, and make the generated CTest
catalog state its required Visual Studio configuration. That would turn this
repeatable audit into a less error-prone refresh gate for downstream users.

### RL-165 — Timed live-deletion restore is now traced with the re-synced 2025 IDs

**Status:** verified development-profile traceability and test evidence; not a
standards or conformance finding.

The next focused C++ slice adds
`Embedded timed federation restore restores a live timestamped object deletion at the save boundary`.
The test schedules a timestamp-six `Delete Object Instance` while a
logical-time-four `Request Federation Save` crosses the constrained receiver
and regulating publisher boundaries. It proves the live deletion and
retraction ledger survive the save, the post-save designator is terminal, the
restored removal is delivered before the Flush Queue Grant with the official
object/tag/producer/time/order/handle fields, and post-delivery `Retract`
reconstitutes the object and reaches the receiver through `Request Retraction`.

The plan now contains 538 entries selecting 460 distinct requirement IDs and
243 distinct C++ API-surface IDs. The new test references only current
2025 IDs from the scoped export at Lab revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`; the timestamped object-deletion
requirements and API contracts both resolve against the bundle. Its focused
Catch2 lane passes all 72 assertions in one test case. This is a bounded
development-profile recovery slice: durable persistence, general timed
restore, other payload/advance modes, changed membership/ownership, remote
transport, package/JUnit/protected-review evidence, and conformance remain
open. After the focused run, the complete official-header C++ catalog also
passes: 771 test cases and 42,802 assertions.

**Possible Lab/tooling refinement:** let a plan entry declare a timed-save
boundary and the preserved live payload/retraction ledger as structured
evidence, rather than requiring those semantics to remain only in the free-form
test description and notes.

### RL-166 — Current re-sync confirms the numbering concern is not a 2025 renumbering

**Status:** verified re-sync and traceability audit; not a standards or
conformance finding.

On 2026-08-24, Umbra exported the 2025 documents again from the adjacent
Requirements Lab working tree at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. The fresh export contains the same
2025 inventories as the pinned bundle: 20 Part 1 requirements, 1,860 Part
1.1 requirements, 1,263 Part 1.1 API surfaces, 282 mappings, and 282
transitions. A field-level comparison found zero added, removed, or changed
2025 requirement, ordinal, clause, API-surface, mapping, or transition IDs.

The active C++ Catch2 plan now resolves 539 entries, 461 distinct 2025
requirement IDs, and 243 distinct C++ API-surface IDs with no unresolved
references. The additional entry is Umbra's private per-module
Dimension-table name-uniqueness preflight; it uses an existing immutable 2025
candidate ID and does not represent a Lab renumbering. All 240 typed Umbra
contracts pass against the fresh 2025 export. No contract or source
renumbering was therefore necessary.

For completeness, a separate 2010-only export succeeds and reports 22
working-tree transition-field changes relative to the ignored canonical
bundle, all in the 2010 Part 1.1 state-machine records for Figures 04 and 07;
their immutable requirement and API IDs remain unchanged. The all-edition
export is still blocked by the unrelated cross-machine endpoint recorded in
RL-163. The 2010 transition-field delta is retained as audit evidence only and
does not alter Umbra's 2025 implementation traceability.

**Local mitigation implemented:** `tools/requirements_lab.py resync` now emits
edition-scoped document content SHA-256 values and per-collection deltas. It
normalizes only regenerated record identity/source-location fields, so a
requirement or API ID change with identical normative content is reported as
`renumbered`, while real content changes are reported separately as added or
missing content. The command is read-only and does not rewrite contracts:

```text
python tools/requirements_lab.py resync \
  --baseline .compliance/corpus-bundle.json \
  --candidate .tmp/corpus-bundle-resync-final-2026-08-24.json \
  --edition 2025 --fail-on-diff
```

The pinned-tag candidate comparison returns `changed_documents: 0` for all
three edition-scoped documents. This mitigation keeps the Lab refinement
proposal visible without treating that release export as a Lab defect.

### RL-167 — Lab working-tree state additions are not 2025 requirement renumbering

**Status:** verified working-tree export drift; not a standards or conformance
finding.

After the pinned-tag comparison above, a fresh 2025 export was generated from
the adjacent Requirements Lab checkout as it currently exists on disk. The
checkout still reports the locked release revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`, but it has uncommitted semantic
state work. The requirement collections remain unchanged (20 Part 1 records,
1,860 Part 1.1 records, and 340 Part 1.2 records), and no existing 2025
requirement, API, mapping, or transition ID was renumbered or removed.

The working-tree export does add one API surface
`surface:m16.transition.joined-federate-statechart`, one mapping
`m16.transition.joined-federate-statechart`, and fifteen joined-federate
state-chart transitions (including save/restore and activity-permission
events), changing the Part 1.1 totals from 1,263 to 1,264 API surfaces, 282 to
283 mappings, and 282 to 297 transitions. These additions come from the
working-tree semantic state artifacts, not from a change to the numbered
requirements used by Umbra's contracts.

**Umbra impact:** the pinned `.compliance/corpus-bundle.json`, lock revision,
and checker-facing contracts remain unchanged. The fresh candidate is ignored
and retained only as local audit material; no implementation mapping is
silently promoted from an uncommitted Lab tree. Once the Lab publishes the
state-chart work under a new reviewed revision, Umbra can re-export it and
update the baseline deliberately. Until then, existing contracts continue to
resolve against the pinned release and the numbering concern is closed for the
2025 requirement tranche.

**Possible Lab/tooling refinement:** have the export report distinguish a
clean tagged checkout from a dirty working tree when a caller supplies a
release revision. A revision/hash field alone is insufficient to identify the
source state when semantic files have uncommitted additions; an explicit
working-tree marker would prevent downstream consumers from mistaking these
additions for a released corpus update.

### RL-168 — Re-sync after the native ownership-transfer slice changes only local coverage

**Status:** verified current-bundle audit; not a standards or conformance
finding.

The 2025 re-sync was repeated after adding the native C++ timestamped
attribute-update/ownership-transfer Catch2 slice and its requirements contract.
The current audit artifact is
`.compliance/corpus-bundle-resync-2026-08-24-r6-2025.json`.
The pinned release remains `4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` and the
candidate still contains 20 Part 1, 1,860 Part 1.1, and 340 Part 1.2
requirements. Requirement IDs, clause IDs, API-surface IDs, mapping IDs, and
their ordering remain unchanged. The only candidate delta is the previously
recorded dirty-working-tree state-chart addition in Part 1.1 (one API surface,
one mapping, and fifteen transitions); it is not a numbering change.

The repository now has 241 checked-in `*-contract.json` files (145 requirement
contracts, 95 API contracts, and one implementation contract), plus the API
baseline. The active Catch2 plan has 541 entries, 461 distinct requirement
IDs, and all 243 distinct C++ API-surface IDs. Every contract resolves against
the candidate with zero findings. The new ownership-transfer contract reuses
existing immutable requirement IDs; it does not introduce or rename a Lab
requirement.

**Umbra impact:** no contract, source reference, or clause-number edit was
needed for the re-sync. The candidate remains an ignored audit artifact; the
pinned bundle and lock remain the standards-facing baseline.

**Possible Lab/tooling refinement:** include a compact plan/contract inventory
in the re-sync report so local coverage growth is visibly separated from
corpus ID changes.

### RL-169 — Fresh 2025 re-sync confirms stable requirement numbering

**Status:** verified current-bundle audit; not a standards or conformance
finding.

On 2026-08-24, Umbra exported a fresh 2025 candidate from the adjacent
Requirements Lab checkout to
`.compliance/corpus-bundle-resync-2026-08-24-r7-2025.json` and compared it with the
pinned `.compliance/corpus-bundle.json`. The exporter reported the same locked
revision, `4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. Part 1, Part 1.1, and Part
1.2 still contain 20, 1,860, and 340 requirements respectively. No requirement
ID, API-surface ID, mapping ID, or existing transition ID was renumbered,
removed, or content-replaced.

The only reported change is the already-known dirty-working-tree state-chart
addition in Part 1.1: one API surface, one mapping, and fifteen transitions
(1,263→1,264 APIs, 282→283 mappings, and 282→297 transitions). The normalized
document digest for the other two editions is unchanged. All 241 local
`*-contract.json` files resolve against the candidate with zero findings.

**Umbra impact:** no source reference, contract ID, pinned bundle, or lock-file
edit is required. The candidate remains temporary audit evidence; the released
2025 baseline is unchanged. The state-chart additions should be promoted only
after the Requirements Lab publishes them from a reviewed revision.

**Possible Lab/tooling refinement:** keep the export's clean/dirty checkout
state explicit in the bundle metadata and emit the compact contract/plan
inventory suggested in RL-168. This lets downstream users distinguish genuine
numbering changes from uncommitted semantic additions without treating either
as a released corpus update.

### RL-170 — Follow-up 2025 re-sync confirms no new numbering drift

**Status:** verified current-bundle audit; not a standards or conformance
finding.

On 2026-08-24, Umbra repeated the 2025 export as
`.compliance/corpus-bundle-resync-2026-08-24-r8-2025.json` after the request to
re-check the Requirements Lab numbering. The candidate still reports the
locked revision `4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` and contains 20 Part
1, 1,860 Part 1.1, and 340 Part 1.2 requirements. Compared with the pinned
bundle, no requirement, API-surface, mapping, or existing transition ID was
renumbered or removed. Compared with the previous r7 audit candidate, all three
2025 documents are unchanged (`changed_documents: 0`).

The pinned-versus-candidate difference remains the known dirty working-tree
state-chart addition in Part 1.1: one API surface, one mapping, and fifteen
transitions (1,263→1,264 APIs, 282→283 mappings, and 282→297 transitions).
Every local contract still resolves against the fresh candidate, and the API
baseline check passes. No Umbra source, contract, pinned bundle, or lock-file
edit is required for this re-sync.

**Possible Lab/tooling refinement:** expose the export's source working-tree
revision and dirty state in the re-sync summary so a consumer can distinguish a
stable published revision from an unchanged local state-chart overlay without
repeating a second candidate comparison.

### RL-171 — Fresh 2025 re-sync still has stable requirement numbering

**Status:** verified current-bundle audit; not a standards or conformance
finding.

On 2026-08-24, Umbra exported the adjacent Requirements Lab working tree again
as `.compliance/corpus-bundle-resync-2026-08-24-r9-2025.json` using the locked
revision `4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. The fresh candidate still
contains 20 Part 1, 1,860 Part 1.1, and 340 Part 1.2 requirements. Compared
with the pinned bundle, the normalized requirement collections have no missing,
added, or renumbered records; clause IDs, API-surface IDs, mapping IDs, and all
pre-existing transition IDs likewise remain stable. The API baseline and all
local typed contracts resolve against the candidate with zero findings.

The candidate-versus-pinned difference is unchanged from RL-170: the dirty Lab
working tree contributes one Part 1.1 API surface, one mapping, and fifteen
state-chart transitions (1,263→1,264 APIs, 282→283 mappings, and 282→297
transitions). Comparing r9 with r8 reports `changed_documents: 0`, confirming
that this is not a new numbering event. No Umbra source, contract, pinned
bundle, or lock-file edit is required; r9 remains ignored audit evidence only.

**Possible Lab/tooling refinement:** include the compact pinned-versus-working-
tree inventory and a clean/dirty source marker in the exporter output so users
can identify this repeated state-chart overlay without retaining several
timestamped candidate files.

### RL-172 — Repeated FOM P/S declaration-fence interpretation re-exposed RL-156

**Status:** verified implementation/traceability recurrence; not a 2025
numbering change or a conformance finding.

At the locked Requirements Lab revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`, the fresh 2025 r9 export still
contains the same 20 Part 1, 1,860 Part 1.1, and 340 Part 1.2 requirement
inventories, with no missing, added, or renumbered requirements, API surfaces,
mappings, or pre-existing transitions. This entry is therefore a new
post-171 recurrence log for an Umbra consumer mistake, not evidence that the
Lab changed its numbering.

The follow-up 2026-08-25 r10 export and resync against r9 report the same locked
revision and `changed_documents: 0`, with the same 2025 requirement/API/mapping/
transition inventories. The numbering remains stable after this recurrence;
the r10 result is audit confirmation, not another observation.

While attempting a bounded C++ declaration slice, Umbra temporarily treated
the FOM/DIF `sharing` field as a runtime Publish/Subscribe precondition for
ordinary, regional, and class-directed declaration services. The current
regression corpus immediately reproduced the RL-156 boundary: the official
Restaurant FOM declares
`HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed` as
`Publish`,
yet existing regional, TSO, and DDM tests legitimately subscribe to it. The
official MIM likewise declares RTI-owned `HLAfederate` attributes and
`HLAreportServiceInvocation` as `Publish`, while MOM observer tests must
subscribe. The attempted guard consequently produced early “Unknown exception”
failures across the region/template, relaxed-DDM, regional interaction,
timestamped regional, restore, passive-subscription, and MOM lanes. The broad
CTest run was stopped during its early segment; this was not a Lab checker
failure.

**Umbra impact:** the guard, its homegrown FOM fixture, and its focused test
plan entry are being removed. FOM/DIF sharing remains retained capability
metadata only; live declaration state remains owned by the 1516.1 declaration
services. RL-156 remains the source-semantics decision and is intentionally not
rewritten or marked resolved a second time.

**Possible Lab/tooling refinement:** make the capability-versus-runtime
distinction machine-visible in generated requirement metadata, or add a checker
warning when a downstream test plan selects FOM P/S candidates as
declaration-service preconditions. This would prevent the repeated consumer
misinterpretation without changing immutable requirement or API IDs.

### RL-173 — Voluntary source resignation lacks a typed timestamped-deletion fanout relation

**Status:** verified Umbra consumer regression and cross-service modeling gap;
not a 2025 numbering change or a conformance finding.

The 2026-08-25 r11 2025 export at the locked Requirements Lab revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` is unchanged: the 20/1,860/340
requirement inventories, immutable IDs, clause IDs, API surfaces, mappings,
and transitions remain stable. The relevant source anchors are the existing
resignation candidate
`requirement-candidate-content-clauses-04-federation-management-page-058-l112-33`,
the timestamped object-delivery candidates
`requirement-candidate-content-clauses-06-object-management-page-129-l138-39`
and `requirement-candidate-content-clauses-06-object-management-page-131-l89-25`,
and the time-management candidate
`requirement-candidate-content-clauses-08-time-management-page-185-l17-5`.

The new native C++ scenario
`Embedded queued timestamped object deletion survives source resignation for
each recipient` accepted one timestamp-7 Delete Object Instance for two
constrained recipients, admitted TAR(7) and NMR(10), and then resigned the
producing owner with `UNCONDITIONALLY_DIVEST_ATTRIBUTES`. The first run threw
an internal error when the independent regulator released the recipients: the
recipient-scoped temporal queue still contained the accepted deletion entries,
but `resignLocked` had already erased the typed deletion payload,
reconstitution record, and retraction ledger. This was a genuine Umbra
consumer cleanup defect, not a changed Lab record.

Umbra now retains the typed deletion payload and per-recipient pending ledger
for a voluntary producer departure whenever the resign action does not delete
objects. The common producer-departure ledger marks the source as departed,
and each recipient consumes its own removal before its matching grant. The
focused C++ case passes 78 assertions. The new contract entries and Catch2
plan entry select the current 2025 IDs; the plan grows by one entry without
renumbering any Lab record.

The Lab export does not currently express the compound relation between
voluntary resignation, typed Delete/Remove Object Instance payload retention,
and independent per-recipient grant frontiers. This extends the cross-service
composition concern already documented in RL-070 and RL-071; it is a new
object-deletion lifecycle record rather than a claim that either earlier
observation was fixed by the Lab. A useful Lab refinement would be a generated
fanout transition that keeps the accepted payload, producer-departure marker,
and each recipient's pending/delivered state explicit across resignation and
TAR/NMR/FQR boundaries.

### RL-174 — Voluntary source resignation loses invocation-time regional TSO scope

**Status:** verified Umbra consumer recurrence in an adjacent payload family and
cross-service modeling gap; not a 2025 numbering change or a conformance
finding.

The 2026-08-25 r11 2025 export at the locked Requirements Lab revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` remains unchanged: the 20/1,860/340
requirement inventories, immutable IDs, clause IDs, API surfaces, mappings, and
transitions are stable. The relevant source records are the voluntary
resignation candidate
`requirement-candidate-content-clauses-04-federation-management-page-058-l112-33`,
the regional overlap and sent-region candidates
`requirement-candidate-content-clauses-09-data-distribution-management-page-236-l152-45`
and `...page-239-l150-43`, and the timestamped callback-before-grant candidate
`requirement-candidate-content-clauses-08-time-management-page-185-l17-5`.

The new native C++ scenario
`Embedded queued timestamped regional interaction survives source resignation`
accepted one timestamp-6 `Send Interaction With Regions` passel for an
overlap-qualified constrained recipient, admitted its TAR(6), resigned the
producing federate, and then released the recipient through an independent
regulator. The first run delivered no callback at the grant boundary even
though the typed interaction payload and recipient queue entry were still
present. Source resignation released the producer's live RegionHandle before
the callback-time regional overlap check, so the queued passel was suppressed
as if its invocation-time source region had never existed.

Umbra now retains committed invocation-time `RegionSpecificationSnapshot`
values in each queued timestamped interaction and supplies those snapshots to
the callback-boundary regional selector after source resignation. The public
opaque RegionHandle and sent-region metadata remain unchanged. The focused C++
case passes 60 assertions, including callback-before-grant ordering, producer,
payload, tag, timestamp/order, source-region metadata, and the post-resignation
`FederateNotExecutionMember` retraction boundary. This is a real consumer
regression adjacent to RL-173's typed-deletion retention fix, not an export or
numbering change; RL-173 remains immutable and is cited rather than rewritten.

The Lab still does not express the compound relation among voluntary producer
departure, invocation-time regional realization, callback-time overlap, and an
independent recipient grant frontier. A useful refinement would parameterize
the existing regional TSO relation with a producer-lifetime boundary and an
immutable source-region snapshot, while leaving live region mutation and
post-departure region handles separate from the queued message's delivery
semantics.

### RL-175 — Voluntary source resignation loses invocation-time regional attribute-update scope

**Status:** verified Umbra consumer recurrence in an adjacent payload family and
cross-service modeling gap; not a 2025 numbering change or a conformance
finding.

The 2026-08-25 r11 2025 export at the locked Requirements Lab revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` remains unchanged. RL-174 recorded the
same invocation-time regional-scope failure for timestamped regional
interactions; this new observation is deliberately additive and does not rewrite
RL-001...RL-174 or consume a historical requirement identifier.

The new native C++ scenario
`Embedded queued timestamped regional attribute update survives source resignation`
accepted one timestamp-7 explicit-source regional `Update Attribute Values`
passel for an overlap-qualified constrained recipient, admitted the recipient's
`TAR(7)`, resigned the producing federate with
`UNCONDITIONALLY_DIVEST_ATTRIBUTES`, and then released the recipient through an
independent regulator. The first implementation delivered no reflection at the
grant boundary even though the typed value and queue entry were present:
resignation removed the producer's live region and update-region association
before callback-time overlap evaluation.

Umbra now retains the committed invocation-time `RegionSpecificationSnapshot`
and accepted explicit source-region association in the queued passel. The
callback-boundary selector uses those immutable overrides after source
resignation, while the public source `RegionHandle`, producer, timestamp/order,
tag, and retraction metadata remain intact. The focused C++ regression passes
with callback-before-grant ordering and rejects post-resignation retraction with
`FederateNotExecutionMember`. This is bounded development-profile evidence, not
Lab validation, remote transport, JUnit/protected-review evidence, or a
conformance claim; the focused Catch2 case passes 66 assertions.

The Lab still does not express the compound relation among voluntary producer
departure, invocation-time regional attribute realization, callback-time
overlap, and an independent recipient grant frontier. A useful refinement would
parameterize the regional timestamped-update relation with a producer-lifetime
boundary, immutable source-region snapshot, and retained attribute association;
the live region lifecycle should remain a separate relation. RL-174 is cited as
the earlier recurrence rather than being edited.

### 2026-08-25 local slice — Federate Resigned public MOM companion

The RTI-initiated `Federate Resigned` public-MOM companion was clean local
coverage. It reserves the final federation-management report before membership
removal and delivers one HLA_IMMEDIATE report before the evoked
`federateResigned` callback. The focused case decodes type 0, type-53 Reason for
resigning (including its quoted Table 5 string value), Null return, success,
reliable transport, invalid producer, empty exception, and serial zero. The
local Catch2 plan grows from 563 to 564 entries; the pinned r13 export and
canonical Lab IDs remain unchanged. No Requirements Lab or Umbra consumer
defect was reproduced, so this slice does not consume RL-176; it is additive
development-profile evidence only.

### 2026-08-25 local slice — successful support lookup public MOM companion

The accepted §10.2--§10.5 support lookup public-MOM companion was clean local
coverage. With file reporting disabled, the native HLA_IMMEDIATE observer
received four reliable service type-6 reports in serial order and decoded
type-53/type-15 `FederateHandle`/`FederateName` plus type-53/type-36
`ObjectClassHandle`/`ObjectClassName` supplied/returned forms, success, empty
exception, unresolved RTI producer, and no regions. The local Catch2 plan
grows from 564 to 565 entries; the pinned r13 export and canonical Lab IDs
remain unchanged. No Requirements Lab or Umbra consumer defect was
reproduced, so this slice does not consume RL-176; it is additive
development-profile evidence only.

### 2026-08-25 local slice — §10.6–§10.12 successful support lookup public MOM companion

The accepted §10.6–§10.12 support lookup public-MOM companion was clean local
coverage. With file reporting disabled, the native HLA_IMMEDIATE observer
received seven reliable service type-6 reports in serial order and decoded
type-37/type-36 object-instance/object-class forms, type-53 object and
attribute names, type-0 `AttributeHandle` forms, and type-35 update-rate
`Number` returns, plus success, empty exception, unresolved RTI producer, and
no regions. The local Catch2 plan grows from 565 to 566 entries; the pinned r13
export and canonical Lab IDs remain unchanged. No Requirements Lab or Umbra
consumer defect was reproduced, so this slice does not consume RL-176; it is
additive development-profile evidence only.

### 2026-08-25 local slice — §10.13–§10.16 successful support lookup public MOM companion

The accepted §10.13–§10.16 support lookup public-MOM companion was clean local
coverage. With file reporting disabled, the native HLA_IMMEDIATE observer
received four reliable service type-6 reports in serial order and decoded
type-53/type-27 interaction-class name/handle plus type-27/type-53 or type-39
parameter supplied/returned forms, success, empty exception, unresolved RTI
producer, and no regions. The local Catch2 plan grows from 566 to 567 entries;
the pinned r13 export and canonical Lab IDs remain unchanged. No Requirements
Lab or Umbra consumer defect was reproduced, so this slice does not consume
RL-176; it is additive development-profile evidence only.

### 2026-08-25 local slice — §10.17–§10.20 successful support lookup public MOM companion

The accepted §10.17–§10.20 support lookup public-MOM companion was clean local
coverage. With file reporting disabled, the native HLA_IMMEDIATE observer
received four reliable service type-6 reports in serial order and decoded
type-53/type-38 order forms plus type-53/type-59 transportation forms in both
directions, success, empty exception, unresolved RTI producer, and no regions.
The local Catch2 plan grows from 567 to 568 entries; the pinned r13 export and
canonical Lab IDs remain unchanged. No Requirements Lab or Umbra consumer
defect was reproduced, so this slice does not consume RL-176; it is additive
development-profile evidence only.

### 2026-08-25 local slice — §10.21–§10.26 successful dimension/region lookup public MOM companion

The accepted §10.21–§10.26 dimension and region lookup public-MOM companion was
clean local coverage. With file reporting disabled, the native HLA_IMMEDIATE
observer received six reliable service type-6 reports in serial order and
decoded type-36/type-27 class handles with type-11 dimension-set returns,
type-53/type-10 name/handle forms, the type-35 upper-bound Number, and the
type-42/type-11 region dimension-set form, plus success, empty exception,
unresolved RTI producer, and no regions. The local Catch2 plan grows from 568
to 569 entries; the pinned r13 export and canonical Lab IDs remain unchanged.
No Requirements Lab or Umbra consumer defect was reproduced, so this slice
does not consume RL-176; it is additive development-profile evidence only.

### 2026-08-25 local slice — §9.2/§10.27 DDM non-void public MOM companion

The accepted Create Region/Get Range Bounds DDM non-void public-MOM companion
was clean local coverage. With file reporting disabled, the native
HLA_IMMEDIATE observer received two reliable service type-5 reports in serial
order and decoded the type-11/type-42 Create Region form plus the
type-42/type-10/type-41 Get Range Bounds form, success, empty exception,
unresolved RTI producer, and no regions. The local Catch2 plan grows from 569
to 570 entries; the pinned r13 export and canonical Lab IDs remain unchanged.
No Requirements Lab or Umbra consumer defect was reproduced, so this slice
does not consume RL-176; it is additive development-profile evidence only.

### 2026-08-25 local slice — §10.29–§10.33 handle-normalization public MOM companion

The accepted five-service handle-normalization public-MOM companion was clean
local coverage. With file reporting disabled, the native HLA_IMMEDIATE observer
received five reliable service type-6 reports in serial order and decoded the
official type-50 ServiceGroup and type-15/type-36/type-27/type-37 supplied
forms, type-35 Number normalized returns, success, empty exception, unresolved
RTI producer, and no regions. The local Catch2 plan grows from 570 to 571
entries; the pinned r13 export and canonical Lab IDs remain unchanged. No
Requirements Lab or Umbra consumer defect was reproduced, so this slice does
not consume RL-176; it is additive development-profile evidence only.

### 2026-08-25 local slice — §6.12 timestamped Send Interaction public MOM companion

The accepted timestamped Send Interaction public-MOM companion was clean local
coverage. With file reporting disabled, the native HLA_IMMEDIATE observer
received one reliable object-management service type-2 report before the
queued timestamped Receive Interaction callback and decoded the official
type-27/type-40/type-63/type-31 supplied forms, the type-34 Null return for a
non-time-regulating sender, success, empty exception, unresolved RTI producer,
no regions, and serial zero. The receiver then preserved the timestamp,
receive-order metadata, parameter bytes, and tag with no retraction. The local
Catch2 plan grows from 571 to 572 entries; the pinned r13 export and canonical
Lab IDs remain unchanged. No Requirements Lab or Umbra consumer defect was
reproduced, so this slice does not consume RL-176; it is additive
development-profile evidence only.

### 2026-08-25 local slice — time-regulated §6.12 Send Interaction public MOM companion

The accepted time-regulated timestamped Send Interaction public-MOM companion
was clean local coverage. After the publisher enabled time regulation and the
receiver enabled time constraint, TSO admission assigned a valid retraction
identity; the native HLA_IMMEDIATE observer received one reliable service
type-2 report before the constrained callback and decoded the official
type-27/type-40/type-63/type-31 supplied forms, the quoted type-33
MessageRetractionHandle return, success, empty exception, unresolved RTI
producer, no regions, and serial zero. The callback preserved timestamp,
TIMESTAMP/TIMESTAMP order, tag, parameter bytes, and a valid retraction. The
focused case passed 101 assertions and the local Catch2 plan grows from 572 to
573 entries; the pinned r13 export and canonical Lab IDs remain unchanged. No
Requirements Lab or Umbra consumer defect was reproduced, so this slice does
not consume RL-176; it is additive development-profile evidence only.

### 2026-08-25 local slice — time-regulated §6.10 Update Attribute Values public MOM companion

The accepted time-regulated timestamped Update Attribute Values public-MOM
companion was clean local coverage. After the publisher enabled time regulation
and the receiver enabled time constraint, TSO admission assigned a valid
retraction identity; the native HLA_IMMEDIATE observer received one reliable
service type-2 report before the constrained Reflect Attribute Values callback
and decoded the official type-37/type-2/type-63/type-31 supplied forms, the
quoted type-33 MessageRetractionHandle return, success, empty exception,
unresolved RTI producer, no regions, and serial zero. The callback preserved
object/attribute values, tag, timestamp, TIMESTAMP/TIMESTAMP order, and a valid
retraction. The focused case passed 106 assertions and the local Catch2 plan
grows from 573 to 574 entries; the pinned r13 export and canonical Lab IDs
remain unchanged. No Requirements Lab or Umbra consumer defect was reproduced,
so this slice does not consume RL-176; it is additive development-profile
evidence only.

### 2026-08-25 local slice — time-regulated §6.14 Send Directed Interaction public MOM companion

The accepted time-regulated timestamped Send Directed Interaction public-MOM
companion was clean local coverage. After the publisher enabled time regulation
and the target recipient enabled time constraint, TSO admission assigned a
valid retraction identity; the native HLA_IMMEDIATE observer received one
reliable object-management service type-2 report before the constrained
Receive Directed Interaction callback and decoded the official
type-27/type-37/type-40/type-63/type-31 supplied forms, the quoted type-33
MessageRetractionHandle return, success, empty exception, unresolved RTI
producer, no regions, and serial zero. The callback preserved target,
timestamp/order/tag/parameter metadata and a valid retraction. The focused
case passed 115 assertions and the local Catch2 plan grows from 574 to 575
entries; the pinned r13 export and canonical Lab IDs remain unchanged. No
Requirements Lab or Umbra consumer defect was reproduced, so this slice does
not consume RL-176; it is additive development-profile evidence only.

### 2026-08-25 local slice — time-regulated §6.14 Send Directed Interaction filesystem companion

The accepted time-regulated timestamped Send Directed Interaction filesystem
companion was clean local coverage of the standards-facing production store.
After the publisher enabled time regulation and the target recipient enabled
time constraint, TSO admission assigned a valid retraction identity; the
configured joined-federate file received one immutable service type-2 record
with the official type-27/type-37/type-40/type-63/type-31 supplied forms and
quoted type-33 MessageRetractionHandle before the constrained Receive Directed
Interaction callback. The callback preserved target, timestamp/order/tag/
parameter metadata and valid retraction, and disabling both reporting switches
left the file unchanged. The focused case passed 214 assertions and the local
Catch2 plan grows from 575 to 576 entries; the pinned r13 export and canonical
Lab IDs remain unchanged. No Requirements Lab or Umbra consumer defect was
reproduced, so this slice does not consume RL-176; it is additive
development-profile evidence only.

### 2026-08-25 local slice — receive-order §6.12 Send Interaction public MOM companion

The accepted nonregional, non-timestamped receive-order `Send Interaction`
public-MOM companion was clean local coverage. With file reporting disabled, an
`HLA_IMMEDIATE` observer received one reliable object-management service type-2
`HLAreportServiceInvocation` before the receiver's queued `HLA_EVOKED`
`Receive Interaction` callback. The focused case decoded the official
type-27/type-40/type-63/type-34 supplied forms, the type-34 Null successful-void
return, empty exception, unresolved RTI producer, no regions, and serial zero;
the receiver then preserved the parameter bytes, receive-order metadata, and
tag. The focused case passed 81 assertions and the local Catch2 plan grows from
576 to 577 entries; the pinned r13 export and canonical Lab IDs remain
unchanged. No Requirements Lab or Umbra consumer defect was reproduced, so this
slice does not consume RL-176; it is additive development-profile evidence only.
While registering its focused CTest lane, the MOM contract check also exposed
five pre-existing local entries whose key was misspelled as singular
`requirement_lab_requirement_id`; those keys were normalized to the contract's
required `requirements_lab_requirement_id` spelling. This was a local contract
formatting/tooling defect rather than a Lab export or runtime recurrence, so
RL-176 remains unused.

### 2026-08-25 local slice — directed Send Interaction public MOM exact C++ forms

The existing accepted untimestamped/timestamped directed public-MOM companion
was strengthened rather than counted as a new plan case. Its native
`HLA_IMMEDIATE` observer now decodes the five official directed supplied forms
for each overload (type-27 interaction class, type-37 target object, type-40
parameter map, type-63 tag, and type-34 Null or type-31 timestamp) plus the
successful type-34 Null return, instead of checking encoded payload sizes only.
The focused case passes 140 assertions, and the new
`directed-send-interaction-service-report-interaction` CTest label keeps its
directed requirements/API and MOM checks in a bounded lane. The r14 2025
re-sync remains identifier/content-stable, the local Catch2 plan stays at 577
entries, and no Requirements Lab or Umbra consumer defect was reproduced, so
RL-176 remains unused.

### 2026-08-25 local slice — ordinary timestamped interaction restore fan-out

The new C++-only recovery slice passed its focused
`timestamped-interaction-restore-multi-recipient` CTest lane. It saves one
ordinary timestamped `Send Interaction` with two constrained recipients,
terminalizes the post-save designator, restores the saved image, delivers the
two recipient-local queue copies independently through `Flush Queue Request`,
and then verifies `Request Retraction` reaches both delivered recipients. The
test also asserts callback-before-grant ordering after clearing the preceding
save/restore lifecycle callbacks. The local Catch2 plan now contains 578
entries. The pinned r15 2025 export is still identifier/content-stable and
`tools/requirements_lab.py check` passes; no Lab or Umbra consumer defect was
reproduced, so this additive development-profile slice does not consume
RL-176.

### 2026-08-25 local slice — timestamped object-deletion restore fan-out

The adjacent C++ recovery slice also passed its focused
`timestamped-object-deletion-restore-multi-recipient` CTest lane. It saves one
queued timestamped `Delete Object Instance` with two constrained recipients,
terminalizes the post-save designator, restores the object-reconstitution
record and both recipient-local removal ledgers, delivers each
`Remove Object Instance` independently through `Flush Queue Request`, and
then verifies that one legal `Request Retraction` reconstitutes the object for
both recipients. The local Catch2 plan now contains 579 entries. The pinned
r15 export remains identifier/content-stable and `tools/requirements_lab.py
check` passes; no Lab or Umbra consumer defect was reproduced, so this is
additive development-profile evidence and does not consume RL-176.

### 2026-08-25 local slice — directed-interaction restore fan-out

The target-qualified directed-interaction recovery slice also passed its
focused `timestamped-directed-interaction-restore-multi-recipient` CTest lane.
It saves one queued timestamped `Send Directed Interaction` targeted at an
object known by two constrained recipients, terminalizes the post-save
designator, restores both recipient-local ledgers, delivers each directed
callback independently through `Flush Queue Request`, and verifies the
original designator reaches both recipients through `Request Retraction`. The
local Catch2 plan now contains 580 entries. The pinned r15 export remains
identifier/content-stable and `tools/requirements_lab.py check` passes; no Lab
or Umbra consumer defect was reproduced, so this remains additive
development-profile evidence and does not consume RL-176.

### 2026-08-25 local slice — timestamped regional-interaction restore fan-out

The explicit-source regional recovery slice passed its focused
`timestamped-regional-interaction-restore-multi-recipient` CTest lane. It
saves one overlap-qualified timestamped `Send Interaction With Regions` for
two constrained recipients, terminalizes the post-save designator, restores
both recipient-local ledgers, delivers each callback independently through
`Flush Queue Request` with the original source `RegionHandleSet`, and then
verifies that `Request Retraction` reaches both delivered recipients. The
local Catch2 plan now contains 581 entries. The r15 export has no requirement
renumbering or source change; the known derived statechart overlay is recorded
in the re-sync audit below. No Requirements Lab or Umbra consumer defect was
reproduced, so RL-176 remains reserved.

### 2026-08-25 local slice — timestamped default-region attribute restore fan-out

The default-source recovery companion passed its focused
`timestamped-default-region-attribute-restore-multi-recipient` CTest lane. It
saves one queued timestamped `Update Attribute Values` passel for two
constrained regional subscribers, terminalizes the post-save designator,
restores both recipient-local ledgers, delivers each reflection independently
through `Flush Queue Request` with the supplied-empty sent-region marker, and
then verifies `Request Retraction` reaches both recipients. The local Catch2
plan now contains 582 entries. The r15 export remains requirement- and
source-content stable; no Requirements Lab or Umbra consumer defect was
reproduced, so RL-176 remains reserved. The first local run also exposed a
test-fixture assumption that `Enable Time Regulation` would leave no callback;
the fixture now drains that accepted callback before admitting the passel.
This was a local harness correction, not a Lab or runtime recurrence, and does
not consume RL-176.

### 2026-08-25 local slice — timed explicit-source regional-interaction restore

The focused `timestamped-regional-interaction-timed-restore` CTest lane now
passes. It schedules a logical-time-6 save while a timestamp-8 explicit-source
`Send Interaction With Regions` remains queued, crosses the boundary for both
federates, terminalizes the post-save designator, restores the committed source
`RegionHandle` and recipient ledger, then delivers through `Flush Queue Request`
at actual time 7 with optimistic time 8 before `Request Retraction`. The local
Catch2 plan now contains 583 entries. The initial draft used a five-unit
lookahead with timestamp 8, which correctly made the post-save Retract illegal;
the fixture was corrected to the one-unit lookahead required for the intended
timed boundary. This was a local scenario setup correction, not a Requirements
Lab or Umbra runtime recurrence, and RL-176 remains reserved.

### 2026-08-25 local slice — timed explicit-source regional-attribute restore

The adjacent explicit-source regional object-update case now has a timed
save-boundary companion. The focused
`timestamped-regional-attribute-timed-restore` CTest lane schedules save at
logical time 6 while a timestamp-8 `Update Attribute Values` passel remains
queued, crosses the boundary for both federates, terminalizes the post-save
designator, restores the object/update association, source `RegionHandle`,
recipient ledger, and retraction identity, then proves `Flush Queue Request`
reflection at actual time 7 with optimistic time 8 before `Request Retraction`.
The C++ target build and focused test passed; the local Catch2 plan now
contains 584 entries. The requirements and API contracts now reference this
test alongside the untimed explicit-source and timed default-source
companions. The focused-tag verifier reports 818 Catch2 cases, the scoped
2025 traceability suite remains 238/238 green, and the pinned-bundle check
passes.

This is an additive adjacent recovery slice, not a Requirements Lab or Umbra
consumer recurrence: the pinned r15 export remains requirement- and
source-content stable, no old RL issue was reintroduced, and RL-176 remains
reserved. The broader durable/alternate-advance/region-mutation,
changed-membership/ownership, transport, package, protected-review, and
conformance matrix remains open.

### 2026-08-25 local slice — timed explicit-source regional-attribute restore fan-out

The focused `timestamped-regional-attribute-timed-restore-multi-recipient`
CTest lane now passes. It schedules a logical-time-6 save while one
timestamp-8 explicit-source `Update Attribute Values` passel is queued for two
constrained regional recipients, crosses the boundary for both constrained
members and the non-constrained regulator, terminalizes the post-save
designator, restores both recipient-local ledgers and the committed source
`RegionHandle`, and delivers each reflection independently through `Flush Queue
Request` at actual time 7 with optimistic time 8 before `Request Retraction`
reaches both recipients. The local Catch2 plan now contains 585 entries.

The first local run exposed only a callback-phase assumption in the new test:
the non-constrained regulator's save-initiation callback is queued after all
constrained members cross the timed boundary. The fixture now drains the
regulator again after the constrained members and asserts the established
`grant` then `save-initiate` ordering. This was a local scenario-ordering
correction, not a Requirements Lab or Umbra consumer recurrence. The r15
export remains requirement- and source-content stable, so no new post-RL-157
observation is consumed; RL-176 remains reserved.

### 2026-08-25 r15 re-sync audit — no requirement renumbering

The fresh 2025 candidate
`.tmp/corpus-bundle-resync-2026-08-25-r15-2025.json` was compared with the
pinned `.compliance/corpus-bundle.json` at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. The Part 1, Part 1.1, and Part
1.2 requirement collections remain 20, 1,860, and 340 records respectively.
The re-sync reports no added, missing, renumbered, or content-replaced
requirement, API-crosswalk, API-binding, or pre-existing transition record.
`tools/requirements_lab.py check` passes against the candidate.

The candidate still contains the known dirty-working-tree overlay in the 2025
Part 1.1 semantic model: one derived API surface, one mapping, and fifteen
state-chart transitions (1,263→1,264 API surfaces, 282→283 mappings, and
282→297 transitions). Those additions have no requirement IDs and are not a
numbering change; they remain ignored audit evidence until the adjacent Lab
publishes them from a reviewed revision. No Umbra contract or test-plan remap
is warranted, and RL-176 remains reserved for the next genuine post-RL-157
Lab or consumer recurrence.

### 2026-08-25 local slice — negotiated willing-to-acquire continuation

The focused `negotiated-willing-to-acquire-continuation` CTest lane now
passes. It keeps two If Available acquisition requests pending while the
owner enters negotiated Waiting, selects the earliest request in the serial
embedded profile, supersedes that selected WTA reservation with a regular
acquisition, cancels the regular request before the owner callback begins, and
then verifies that stale first-candidate work is suppressed while the retained
second WTA candidate receives Request Divestiture Confirmation with its own
acquisition tag. Confirm Divestiture transfers ownership and emits one normal
Acquisition Notification to the second candidate.

The first draft attempted to cancel the If Available request directly through
`Cancel Attribute Ownership Acquisition`. The official 2025 C++ surface has no
separate WTA-cancellation service, and the existing local contract correctly
rejects that call as `AttributeAcquisitionWasNotRequested`. The fixture now
uses the specified regular-acquisition supersession followed by cancellation;
this was a local scenario/API-use correction, not a Requirements Lab or Umbra
runtime recurrence. The local Catch2 plan now contains 586 entries, the r15
export remains requirement- and source-content stable, and RL-176 remains
reserved.

### 2026-08-25 local slice — four-member mixed timestamped restore composition

The focused `restore-mixed-tso-object-interaction-four-member` CTest lane now
passes with 108 assertions. It saves one timestamped `Update Attribute Values`
passel and one timestamped `Send Interaction` in a four-member embedded
federation, terminalizes both post-save designators, restores the image, and
delivers the restored object reflection and interaction independently through
Flush Queue before each recipient's grant. The original payloads, tags,
producer handles, timestamp, retraction metadata, and post-delivery Request
Retraction callbacks are all checked.

The first drafts exposed only local fixture/API-use corrections: named object
registration requires the official preceding `Reserve Object Instance Name`
service; save admission requires the request-before-TAR sequence and both
regulators' callback drains; and the owner must resign with
`CANCEL_THEN_DELETE_THEN_DIVEST` while it still owns the registered attribute.
Those corrections did not reproduce a Requirements Lab or Umbra runtime
defect. The r15 export remains requirement- and source-content stable, the
local Catch2 plan advances from 586 to 587 entries, and RL-176 remains reserved.
This slice is bounded development-profile evidence only: it does not claim
durable persistence, remote transport, arbitrary post-restore handle remapping,
package/JUnit/protected-review evidence, validation, or conformance.

### 2026-08-25 local slice — timestamped attribute ordering and equal-timestamp cohort

The focused `Embedded timestamped attribute updates preserve different-timestamp
order for each constrained recipient` Catch2 case passes with 79 assertions.
One publisher submits timestamp 7 before a timestamp-5 update and a second
timestamp-5 update; two independent constrained recipients advance first to 5
and then to 7. Each receives the complete timestamp-5 cohort before timestamp 7,
with the equal-timestamp tie-break intentionally left unspecified, and each
reflection precedes its matching time-advance grant. The local Catch2 plan now
advances from 587 to 588 entries, while the pinned r15 export and canonical Lab
IDs remain requirement- and source-content stable.

The first local draft exposed only a fixture/API-use assumption: enabling Time
Regulation legitimately queued the `TimeRegulationEnabled` callback, so the
fixture now drains that callback before submitting the passels. This was a
local harness correction, not a Requirements Lab or Umbra runtime recurrence;
no post-RL-157 observation is consumed and RL-176 remains reserved. The case
is bounded embedded development-profile evidence only. Cross-process transport
arrival ordering, alternate advance forms, ownership/resignation churn,
save/restore, package/JUnit/protected-review evidence, validation, and
conformance remain open.

### 2026-08-25 local slice — timestamped attribute ordering through TARA/NMRA

The focused `Embedded timestamped attribute updates preserve ordering through
available advances` Catch2 case passes with 71 assertions. It uses the same
timestamp-7-before-two-timestamp-5 cohort shape as the TAR companion, but
drives two independent constrained recipients through exact
`Time Advance Request Available` and `Next Message Request Available`
boundaries. Both alternate forms deliver the complete timestamp-5 cohort
before timestamp 7 and preserve reflection-before-grant ordering; the
equal-timestamp tie-break remains unspecified. The local Catch2 plan now
advances from 588 to 589 entries, and the pinned r15 export remains
requirement- and source-content stable.

This slice exposed no Requirements Lab or Umbra runtime recurrence. RL-176
remains reserved under the post-RL-157 rule. The evidence is bounded embedded
development-profile coverage only; transport-arrival/cross-process ordering,
other advance combinations, ownership/resignation, save/restore, package/JUnit,
protected review, validation, and conformance remain open.
While registering this companion, the checker also rejected an initially
over-specific local `clause-8.1.5` value for the selected requirement; the
canonical Lab field is `clause-8`. The contract now uses the exported value,
with no bundle/source change and no new observation number consumed.

### 2026-08-25 local slice — service-reporting disabled suppresses both destinations

The focused `Embedded service reporting suppresses accepted services while the
reporting switch is disabled` Catch2 case passes with 27 assertions. It uses
the official Restaurant FOM's default-disabled `HLAserviceReporting` state,
explicitly enables `HLAsendServiceReportsToFile`, accepts a real
`Get Dimension Handle` lookup, and proves that the eagerly-created joined-
federate file keeps its initial size while an eligible observer receives no
`HLAreportServiceInvocation` interaction. The local Catch2 plan advances from
589 to 590 entries; the MOM, support-switch, dimension-lookup, subscription,
and receive-order API contracts now reference the same C++ selector.

This slice closes only the reporting-disabled routing arm in the embedded
development profile. The existing ordinary `Send Interaction` MOM case remains
the interaction-selected arm. Self-switch transition ordering, generic
return/failure forms, broader MOM families, remote transport, package/JUnit,
protected review, validation, and conformance remain open.

The first fixture draft used the all-switches-enabled extension FOM and hit an
untyped exception when a second federate subscribed to the report interaction
after the subject was disabled. The official Restaurant FOM plus an explicit
file-switch enable is the stable, standards-facing fixture for this slice.
That is a local Umbra fixture/runtime rough edge—not a Requirements Lab
numbering or export recurrence—so no post-RL-157 observation number is
consumed; the extension-switch topology remains a separate follow-up.

The first broad `service-reporting` label run also exposed a stale precondition
in the adjacent time-regulated directed-interaction MOM fixture: it advanced
the sender only to logical time 2 while using lookahead 1, so GALT 3 correctly
held the timestamp-6 delivery. The fixture now uses lookahead 5, matching its
filesystem companion's valid admission boundary; the isolated case passes 115
assertions and the full 294-test label is green. This is a local test-harness
correction, not a Requirements Lab or runtime recurrence, so RL-176 remains
reserved.

### 2026-08-25 local slice — Connect aggregate-to-overload crosswalk

The local `connection-implementation-contract.json` now selects the four
official C++ `RTIambassador::connect` API-surface records under the Lab's single
aggregate `rti.service.connect` mapping. `tools/requirements_lab.py check`
validates that every selected ID exists, is an `RTIambassador` C++ surface, and
is a Connect declaration; the new
`umbra.ieee1516_2025.connection_implementation_traceability` CTest exercises
the same guard. This resolves Umbra's local traceability gap without editing
the immutable Lab mapping or claiming that its sidecar/catalog aggregate
selection ambiguity is fixed. It is not a Requirements Lab or Umbra consumer
recurrence, so RL-176 remains reserved.

### 2026-08-25 local slice — default-region interaction retraction through alternate advances

The focused `Embedded timestamped default-region Send Interaction retracts
before TARA and NMRA grants` Catch2 case now passes. It queues an ordinary
timestamped interaction from the private default source against two committed
regional subscribers, accepts TARA(7) and NMRA(10), retracts before either
recipient callback boundary, and proves that both alternate grants complete
without `Receive Interaction` or `Request Retraction`. The captured NMRA
frontier remains timestamp 8 even though the queued interaction is withdrawn;
the producer's second Retract is classified as
`MessageCanNoLongerBeRetracted`.

During this slice the runtime exposed a separate scheduler edge: removing the
last queued TSO passel did not re-evaluate already-pending alternate time
advances, leaving NMRA stranded until another federation event. Umbra now
re-evaluates pending grants after an accepted Retract. This was an Umbra
runtime defect found by the new C++ test, not a Requirements Lab export or
numbering recurrence; no post-RL-157 observation is consumed and RL-176
remains reserved. The local Catch2 plan advances from 590 to 591 entries;
the default-region, timestamped-interaction, Request Retraction, TAR/TARA,
and NMRA contracts reference the same selector. Broader FQR, transport,
save/restore, package/JUnit, protected review, validation, and conformance
remain open.

### 2026-08-25 local slice — Register Object Instance service-report coverage

The embedded runtime had a consumer-side coverage gap rather than a Requirements
Lab export defect: all four 2025 `Register Object Instance` overloads committed
object state and queued discovery, but did not emit their §11.5 service-report
records. The source-backed fix now emits ordinary and regional success forms
after the registry transaction, plus the Null/false/exception form for an
invalid object class, while preserving the same ordering before discovery
callbacks. The focused native Catch2 case passes with 484 assertions and exact
filesystem records; the plan now contains 592 entries and the pinned/r15/r16
`check-plan` guards resolve the four official C++ overload surfaces and the
object-management/DDM/MOM requirement candidates.

This was newly implemented Umbra behavior discovered by direct use of the
official MIM/service-report contract, not a reproduced Lab or previously logged
consumer issue. It therefore consumes no post-RL-157 recurrence identifier;
RL-176 remains the latest go-back and RL-177 remains reserved for a future
reproduced recurrence. Timestamped/retraction, public MOM interaction,
broader DDM/ownership, remote/package/JUnit, protected review, validation, and
conformance remain open.

The first broad `[service-report-file]` run after this fix exposed four stale
fixture expectations: those cases registered their setup object while file
reporting was enabled, so the newly correct `RegisterObjectInstance` record
shifted the target service's serial. Each fixture now disables both reporting
switches only around setup registration and re-enables them before the service
under test; the lane passes all 142 cases and 17,401 assertions. This was a
local test-harness correction caused by newly covered behavior, not a
Requirements Lab recurrence, so it does not consume RL-177.

### 2026-08-25 r16 re-sync audit — no 2025 requirement renumbering

The fresh 2025 candidate
`.tmp/corpus-bundle-resync-2026-08-25-r16-2025.json` was exported from the
adjacent Requirements Lab at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. Comparing r16 with r15 reports
`changed_documents: 0`; the 2025 inventories remain 20 Part 1, 1,860 Part
1.1, and 340 Part 1.2 requirements, with no added, missing, renumbered, or
content-replaced requirement, API-crosswalk, API-binding, mapping, or
pre-existing transition record. `tools/requirements_lab.py check` and
`check-plan` both pass against r16 (592 plan entries, 480 selected
requirements, 243 C++ API surfaces).

Comparing r16 with the pinned bundle still shows only the known dirty
Part 1.1 working-tree overlay: one API surface, one mapping, and fifteen
transitions (1,263→1,264 APIs, 282→283 mappings, and 282→297 transitions).
These additions carry no new requirement IDs and are not a 2025 numbering
change. RL-176 remains the latest verified post-RL-157 recurrence; RL-177 is
still reserved for the next genuinely reproduced old-issue go-back.

### 2026-08-25 r17 re-sync audit — no 2025 requirement renumbering

The current-session 2025 export from `../Document-Recreation` was compared
with the prior r12 candidate after the numbering concern was raised again.
`requirements_lab.py resync --edition 2025` reports `changed_documents: 0`:
Part 1 remains 20 requirements, Part 1.1 remains 1,860 requirements with
1,264 working-tree API surfaces, 283 mappings, and 297 transitions, and Part
1.2 remains 340 requirements. No requirement, ordinal, clause ID, API-surface
ID, mapping ID, binding, or pre-existing transition was added, removed,
renumbered, or content-replaced. The known pinned-versus-working-tree
additive overlay remains the only difference from the canonical bundle.

The local Catch2 plan and contract checks remain valid after the whole-class
Unpublish Object Class Attributes service-report slice was added; its new
coverage reuses the immutable Lab IDs and the official `unpublishObjectClass`
API surface. This is local coverage growth, not a Lab or Umbra consumer
recurrence. RL-176 remains the latest verified post-RL-157 go-back, and RL-177
remains reserved for the next genuinely reproduced old issue.

### 2026-08-25 local slice — whole-class Unpublish Object Class Attributes report

The embedded C++ `unpublishObjectClass()` overload now emits the §5.3
`UnpublishObjectClassAttributes` service-report record after the accepted
whole-class registry teardown and before separately queued declaration
advisories. Its optional attribute-set position is encoded as Table 5 type-34
Null; the attribute-set overload remains type-1, and a supplied-empty set is
not conflated with the whole-class form. The failed path uses the same service
name and Null slot without introducing a memory/file fallback.

The focused Catch2 case passes, the 145-test `[service-report-file]` lane and
the 71-test `declaration-management` lane are green, and the full native
executable passes 48,101 assertions in 827 test cases. The API/requirements
traceability checks resolve the official whole-class C++ surface. This is new
local coverage, not a reproduced Lab or consumer recurrence; RL-176 remains
the latest verified post-RL-157 go-back and RL-177 remains reserved.

### 2026-08-25 local slice — synchronization-point save/restore state

The focused native C++ case `Embedded federation restore preserves
synchronization-point state from the saved image` passes. It saves an announced
but unachieved point, completes the live point after the save, restores the
process-local image, and achieves the point again; the second
`Federation Synchronized` callback proves the saved synchronization ledger was
reconstituted rather than the post-save live state. The case is traced through
the synchronization, save, and restore contracts and the Catch2 plan now has
593 entries.

This is clean new Umbra coverage, not a reproduced Requirements Lab or
consumer issue. No historical defect was observed, no Lab export changed, and
no recurrence identifier is consumed: RL-176 remains the latest verified
post-RL-157 recurrence and RL-177 remains reserved for the next genuinely
reproduced go-back.

### RL-176 — Catch2 plan selector drift re-exposes RL-160

**Status:** verified Umbra consumer/traceability recurrence; not a Requirements
Lab numbering change or a conformance finding.

The 2026-08-25 r15 2025 export at the locked Requirements Lab revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` retains the same 20/1,860/340
requirement inventories and the same pre-existing requirement, API, mapping,
and transition IDs. Its only pinned-versus-working-tree difference remains
the already recorded dirty Part 1.1 state-chart overlay. A new local Catch2
plan guard, added in response to the stale-plan gap recorded by RL-160,
resolved the 591 plan entries against the complete 2025 requirement set and
the 243 selected C++ API surfaces. It exposed two stale human-readable
selectors: one aggregate handle-encoding name and one aggregate
`VariableLengthData` name no longer occurred in the native C++ sources after
those tests were split into individual Catch2 cases.

Umbra corrected the plan to list the exact current selectors (semicolon-
separated where one planning row intentionally covers a family). The guard is
now registered as `umbra.ieee1516_2025.catch2_plan_traceability`; it checks
supplied IDs, duplicate references, C++ API language, plan metadata, and
whitespace-normalized selectors without editing contracts or the pinned Lab
bundle. The pinned and r15 candidate checks both pass after this mitigation.

**Possible Lab/tooling refinement:** export a structured test-selector or
source-symbol relation for planning catalogs. Until that exists, the local
guard remains a consumer-side stale-selector check and does not promote the
plan to catalog or conformance evidence.

### 2026-08-25 local slice — failed federate lookup service reports

The embedded C++ `GetFederateHandle` and `GetFederateName` support lookups now
append their failed service-report forms through both standards-facing sinks.
The filesystem case and the HLA_IMMEDIATE MOM case preserve the official
type-53/type-15 supplied arguments, type-34 Null returned argument, false
success indicator, exact `NameNotFound`/`InvalidFederateHandle` descriptions,
and serials zero and one before successful lookups continue at serials two and
three. The focused cases pass, the `[service-report-file]` lane is green at
146 tests, and the `[service-report-interaction]` lane is green at 62 tests.

This was a newly observed Umbra consumer coverage omission, not a reproduced
Requirements-Lab defect or a numbering change. It consumes no post-RL-157
recurrence identifier: RL-176 remains the latest verified go-back and RL-177
remains reserved for the next genuinely reproduced old issue. RL-152 still
records the Lab's conditional failure-mapping gap, so this remains
development-profile traceability rather than validation or conformance.

### 2026-08-25 local slice — Query Attribute Ownership public MOM report

The native C++ Query Attribute Ownership path already selected the public
interaction sink after releasing its ownership-planning locks, but only the
filesystem route had a focused report-form proof. The new HLA_IMMEDIATE
observer case decodes service type 3, type-37/type-1 supplied arguments,
type-34 Null return, true success, empty exception, and serial zero, then
confirms the grouped owner/unowned callbacks follow the report. This is a local
evidence completion, not a reproduced Requirements-Lab defect or recurrence;
RL-176 remains the latest verified go-back and RL-177 remains reserved. It is
development-profile C++ traceability only.

### 2026-08-25 r18 re-sync audit — no 2025 requirement renumbering

The current 2025 export from `../Document-Recreation` at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` again reports no requirement,
ordinal, clause ID, API-surface ID, mapping ID, binding, or pre-existing
transition renumbering. The candidate differs from the pinned bundle only by
the known dirty Part 1.1 working-tree overlay: one additive API surface, one
mapping, and fifteen additive transitions. The 2025 requirement inventories
remain 20 Part 1, 1,860 Part 1.1, and 340 Part 1.2 requirements. `check` and
`check-plan` pass against the candidate; the local Catch2 plan contains 596
entries, 480 selected requirements, and 243 selected C++ API surfaces.

The first package-CTest invocation in the managed Windows sandbox also hit a
Visual Studio SDK lookup permission error (`ToolLocationHelper` could not read
the user's Microsoft SDK directory). Rerunning the identical six focused
CTest lanes with the required build permission passed; this is an environment
permission edge, not Lab drift or a consumer recurrence.

This is an expected re-sync and not a Requirements-Lab or Umbra consumer
recurrence. It consumes no post-RL-157 identifier: RL-176 remains the latest
verified go-back and RL-177 remains reserved for the next genuinely reproduced
old issue. The additive working-tree records remain audit-only until the Lab
revision is intentionally refreshed.

### 2026-08-25 local slice — installable embedded resource/dependency contract

The embedded federation-management profile is now installable. The package
exports the private LibXml2-backed validation backend as an internal target,
declares `find_dependency(LibXml2 2.15 CONFIG)` for downstream consumers, and
installs the reviewed IEEE 1516.2-2025 schemas, MIM, Restaurant examples,
notice, and digest manifest. Runtime resource selection prefers the checked-in
source tree for development and falls back to the installed reviewed payload;
it fails deterministically if neither root is complete. The registered package
CTest stages the install, verifies resource digests, configures a clean
consumer, and builds/runs it successfully. A focused embedded Create-FOM case
also remains green.

This is packaging and SDK-consumability evidence, not a new Lab requirement
mapping, service catalog, protected review, or conformance claim. No old Lab or
Umbra consumer issue was reproduced, so no post-RL-157 recurrence identifier is
consumed: RL-176 remains the latest verified go-back and RL-177 remains
reserved.

### 2026-08-25 r19 re-sync and recurrence audit — no new go-back

The fresh candidate
`.tmp/corpus-bundle-resync-2026-08-25-r19-2025.json` was exported directly
from `../Document-Recreation` at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. Comparing r19 with the immediately
preceding r18 candidate reports `changed_documents: 0`: the 2025 inventories
remain 20 Part 1, 1,860 Part 1.1, and 340 Part 1.2 requirements, with 1,264
working-tree API surfaces, 283 mappings, and 297 transitions. Comparing r19
with the pinned bundle still shows only the known dirty Part 1.1 additive
overlay (one API surface, one mapping, and fifteen transitions); no requirement,
ordinal, clause ID, API-surface ID, mapping ID, binding, or pre-existing
transition was added, removed, renumbered, or content-replaced. The baseline
`check` and Catch2 `check-plan` both pass (596 plan entries, 480 selected
requirements, and 243 selected C++ API surfaces).

The recurrence audit also reran the post-RL-157 boundaries that previously
required new identifiers: the FOM sharing-metadata composition case passes 23
assertions, the timestamped regional attribute resignation case passes 66,
the timestamped object-deletion resignation fanout case passes 78, and the
timestamped regional interaction resignation case passes 60. RL-176's stale
selector guard remains green. These are clean reproductions after the recorded
mitigations; none re-exposes an earlier Lab or Umbra consumer defect, so RL-177
is not consumed. If any of these slices fails on a later workflow pass, that
fresh reproduction must be appended as RL-177 (or the next unused identifier)
and cite the earlier RL entry rather than editing RL-172 through RL-176.

### 2026-08-25 local slice — independent HLAsetTiming target deadlines

The embedded `HLA_IMMEDIATE` MOM scheduler now has a focused native Catch2
vector for two simultaneous joined-federate targets. It arms one target for
one second and another for two seconds, observes the first target's reliable
`HLAlogicalTime`/`HLAlookahead` reflection while the second remains pending,
then disables only the first target and confirms the second target still
reflects. The case also checks that each reflection retains its target MOM
object identity and the normal RTI-originated reliable callback metadata. The
focused executable passes 41 assertions in one test case.

The full native C++ Catch2 executable also passes 48,400 assertions in 831
test cases after the slice was added.

This is bounded C++ evidence for target isolation in an already implemented
per-ambassador scheduler, not a reproduced Lab or Umbra consumer recurrence.
The r19 candidate remains content-stable against r18, RL-176 remains the latest
verified go-back, and RL-177 remains reserved. The broader multi-federate
callback-ordering matrix, other periodic attributes, remote transport, and
conformance remain open. If a later workflow pass regresses this target
isolation or re-exposes an earlier issue, append the fresh reproduction as
RL-177 (or the next unused identifier) and cite the earlier observation;
do not alter the first 157 entries.

### 2026-08-25 r20 re-sync and recurrence audit — no new go-back

The fresh 2025 candidate
`.tmp/corpus-bundle-resync-2026-08-25-r20-2025.json` was exported directly
from `../Document-Recreation` at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. Comparing r20 with r19 reports
`changed_documents: 0`. The three 2025 inventories remain 20 Part 1, 1,860
Part 1.1, and 340 Part 1.2 requirements; Part 1.1 still has 1,264 working-tree
API surfaces, 283 mappings, and 297 transitions. Comparing r20 with the
pinned bundle still shows only the known dirty Part 1.1 overlay: one additive
API surface, one mapping, and fifteen transitions. No requirement, ordinal,
clause ID, API-surface ID, mapping ID, binding, or pre-existing transition was
added, removed, renumbered, or content-replaced.

The local baseline and Catch2-plan guards pass (597 plan entries, 480 selected
requirements, and 243 selected C++ API surfaces). The post-RL-157 regression
boundaries also pass: the twelve queued timestamped resignation slices
(including RL-173, RL-174, and RL-175's object-deletion, regional-interaction,
and regional-attribute cases), the independent HLAsetTiming target slice, the
FOM sharing-metadata composition case, and the four timestamped regional
traceability checks. None re-exposes an earlier Lab or Umbra consumer issue;
RL-176 remains the latest numbered post-157 recurrence and RL-177 remains
reserved for the next genuinely reproduced go-back. If a later pass finds an
old issue still unresolved after its expected mitigation, it must append RL-177
(or the next unused identifier), cite the original observation, and record the
current reproduction and mitigation rather than editing RL-001 through
RL-157—or leaving the recurrence in an unnumbered audit note.

### 2026-08-25 local slice — HLAsetTiming target isolation under both callback models

The existing two-target HLAsetTiming case now runs under both official
callback models. The HLA_IMMEDIATE branch uses the per-ambassador scheduler;
the HLA_EVOKED branch explicitly pumps the same registry-owned deadline at the
caller’s Evoke boundary. Each branch arms one joined-federate MOM object for
one second and another for two seconds, observes only the first target before
the later deadline, disables the first target, and then observes the second
target without a replacement or cross-target cancellation. The combined
Catch2 selector passes 82 assertions in one test case.

This is clean C++ development-profile coverage of an already implemented
callback-model boundary, not a Requirements Lab or Umbra consumer recurrence.
The r20 export remains content-stable against r19, RL-176 remains the latest
numbered post-RL-157 go-back, and RL-177 remains reserved for the next genuine
reproduction. The broader periodic-attribute, callback-ordering, remote
transport, protected-review, and conformance matrix remains open.

### 2026-08-25 local slice — queued TSO keeps LITS after source resignation

The new `Embedded Query LITS remains defined for a queued TSO after source
resignation` Catch2 case exercises the official Query GALT and Query LITS
surfaces against a real queued timestamped interaction. Before resignation,
the source regulator's current-time-plus-lookahead candidate supplies GALT/LITS
1 while the future message is at timestamp 5. After the source resigns with
`NO_ACTION`, the accepted recipient-local queue remains intact: Query GALT is
undefined and Query LITS returns 5. The focused CTest lane passes, and the two
new Requirements-Lab contracts plus the Catch2 plan entry resolve against the
locked r20 2025 export.

This is a clean C++ development-profile traceability completion, not a
reproduced Lab or Umbra consumer recurrence. The r20 export remains
content-stable, RL-176 remains the latest numbered post-157 go-back, and RL-177
remains reserved. If this boundary later fails after an expected mitigation,
append that fresh reproduction as RL-177 (or the next unused identifier) with
an earlier-observation citation; do not edit RL-001 through RL-157 or fold the
failure into this unnumbered slice note.

### 2026-08-25 r21 re-sync and full-suite audit — no new go-back

The fresh candidate
`.tmp/corpus-bundle-resync-2026-08-25-r21-2025.json` was exported directly
from `../Document-Recreation` at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. Comparing r21 with r20 reports
`changed_documents: 0`: the 2025 inventories remain 20 Part 1, 1,860 Part
1.1, and 340 Part 1.2 requirements, with 1,264 working-tree API surfaces,
283 mappings, and 297 transitions. Every normalized collection is unchanged;
there are no added, missing, renumbered, or content-replaced records.

The complete configured Debug CTest run executed all 2025 lanes successfully.
The only non-green entries were the intentionally out-of-scope 2010 marshal
lane (`Not Run`) and the installed-package smoke test's managed-sandbox
Windows SDK permission error. Re-running that identical package test with the
required build permission passed. The Requirements Lab baseline, Catch2 plan,
and observation-ledger checks all pass. This is an expected re-sync and an
environment-permission resolution, not a reproduced Lab or Umbra consumer
recurrence: RL-176 remains the latest numbered post-157 go-back and RL-177
remains reserved. If a later pass finds an old issue still unresolved after
its expected mitigation, append a new post-157 identifier with the earlier
observation citation and current reproduction; never edit RL-001 through
RL-157 or silently reuse an earlier post-157 identifier.

### 2026-08-25 local slice — default-region regulation re-enable

The new `Embedded timestamped default-region interaction survives
time-regulation disable and re-enable` Catch2 case keeps one default-source
timestamped interaction queued for a regional time-constrained recipient while
the producer disables and callback-gated re-enables Time Regulation at the
same lookahead. The callback arrives exactly once before the grant with the
supplied-empty region marker, original payload/tag/producer/timestamp/order,
and valid retraction metadata. The focused CTest selector passes after a clean
target rebuild; the complete native 2025 Catch2 matrix then passed 833/833
cases. The default-region, timestamped-interaction, and temporal-role contracts
plus plan entry resolve against r21. This is clean local
coverage, not a reproduced Requirements Lab or Umbra consumer recurrence; it
consumes no post-RL-157 identifier. If this boundary later fails after the
expected mitigation, append RL-177 (or the next unused identifier) with a
citation to the earlier observation rather than editing RL-001 through
RL-157 or folding the failure into this slice note.

### 2026-08-25 r22 re-sync and recurrence audit — no new go-back

The fresh 2025 candidate
`.tmp/corpus-bundle-resync-2026-08-25-r22-2025.json` was exported directly
from `../Document-Recreation` at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. Comparing r22 with r21 reports
`changed_documents: 0`; the 2025 inventories remain 20 Part 1, 1,860 Part
1.1, and 340 Part 1.2 requirements, with 1,264 working-tree API surfaces,
283 mappings, and 297 transitions. Every normalized collection is unchanged;
there are no added, missing, renumbered, or content-replaced records.

The observation ledger remains valid with 176 numbered entries and RL-176 as
the latest post-RL-157 go-back. The Catch2 plan remains valid with 599 entries,
485 selected requirements, and 243 selected API surfaces. This expected
re-sync and clean recurrence audit reproduced no old issue, so it consumes no
new identifier and RL-177 remains reserved. If a later pass finds an old issue
still unresolved after its expected mitigation, append RL-177 (or the next
unused identifier) with an earlier-observation citation, the current
reproduction, and mitigation status; never edit RL-001 through RL-157 or
silently reuse an earlier post-RL-157 identifier.

### 2026-08-25 r23 re-sync and recurrence audit — no new go-back

A fresh 2025 export from `../Document-Recreation` at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` was compared with r22 using the
Requirements Lab resync checker. It reports `changed_documents: 0`: the 20
Part 1, 1,860 Part 1.1, and 340 Part 1.2 requirements, 1,264 API surfaces,
283 mappings, 297 transitions, 1,410 API crosswalks, and 1,894 requirement/API
bindings are unchanged. No requirement, clause, API, mapping, binding, or
transition was added, removed, renumbered, or content-replaced.

The clean export comparison and the live observation/guard checks reproduced
no Requirements Lab or Umbra consumer go-back. This is an expected numbering
audit, not a new observation: RL-176 remains the latest post-RL-157 recurrence
and RL-177 remains reserved. If a later workflow pass reproduces an earlier
issue—or finds it still unresolved after its expected mitigation—append RL-177
(or the next unused identifier) with the earlier citation, current evidence,
and mitigation state; do not fold it into this audit or edit RL-001 through
RL-157.

### 2026-08-25 observation-guard regression — no new go-back

The new offline `tools/requirements_lab_observations_regression.py` lane runs
the production checker against temporary ledger variants. It accepts a
post-RL-157 “remains unresolved” reproduction only when it cites an earlier
observation, rejects the same reproduction without a citation, rejects a
skipped identifier, and rejects edits to the immutable RL-001..RL-157 text.
The live ledger is unchanged and RL-177 remains reserved; this is guard
coverage rather than a newly reproduced Lab or Umbra consumer issue.

### 2026-08-25 local slice — official DIF class-member uniqueness

The native 2025 validator lane now exercises the official
`IEEE1516-DIF-2025.xsd` `xs:unique` constraints for duplicate direct
object-class attribute names and interaction-class parameter names. Both
schema-negative fixtures are rejected before Umbra's identity-based
composition maps run. The focused CTest label
`fom-member-name-uniqueness` runs the Catch2 case and its Requirements-Lab
contract together; both pass against the locked 2025 export.

This is clean schema-boundary coverage, not a reproduced Requirements Lab or
Umbra consumer recurrence. It consumes no observation number: RL-176 remains
the latest post-RL-157 recurrence and RL-177 remains reserved. If a later
workflow pass re-exposes a prior issue or finds it still unresolved after its
expected mitigation, append that fresh reproduction as RL-177 (or the next
unused identifier) with an earlier citation, current evidence, and mitigation
status rather than editing RL-001 through RL-157.

### 2026-08-25 local slice — default-region attribute regulation re-enable

The native C++ `Embedded timestamped default-region attribute update survives
time-regulation disable and re-enable` case now covers the producer-side role
transition that was not represented by the earlier time-constrained
re-enable companion. A default-source/default-region timestamped passel stays
in the regional recipient queue while Time Regulation is disabled and then
callback-gated re-enabled at the same lookahead; the focused case preserves
payload, tag, producer, timestamp/order, supplied-empty region metadata, and
terminal retraction classification before the receiver's matching grant.
The new Requirements-Lab contract and exact Catch2 plan entry resolve against
the locked r23 2025 export; the plan now contains 601 entries. The focused
test and contract checker pass, and no Lab or Umbra consumer defect was
reproduced. This is additive development-profile coverage and consumes no
observation number: RL-176 remains the latest post-RL-157 recurrence and
RL-177 remains reserved. If this boundary later fails after the expected
mitigation, append that reproduction as RL-177 (or the next unused identifier)
with an earlier-observation citation, current evidence, and mitigation status;
do not edit RL-001 through RL-157 or fold the failure into this local note.

### 2026-08-25 local regression audit — lane catalog and environment follow-up

The broad configured Debug CTest pass reached all 1,100 registered tests. The
new attribute-regulation lane initially exposed one catalog wiring omission:
because it was declared as a service lane, the focused-lane audit required a
matching API-contract traceability check. Adding
`timestamped-attribute-update-regulation-reenable-api-contract.json` and its
CTest registration corrected that bookkeeping issue; the focused lane now
passes its Catch2 case plus both requirements/API checks, and the catalog audit
passes. The other broad-run exceptions were expected infrastructure scope: the
2010 marshal test is intentionally not run in this 2025 tranche, and the
installed-package smoke test hit the managed Windows SDK permission boundary;
rerunning that same test with the required build permission passed. No Lab or
Umbra consumer defect was reproduced. This is an unnumbered local audit and
consumes no observation number: RL-176 remains the latest post-RL-157
recurrence and RL-177 remains reserved. If a future pass re-exposes the lane
catalog issue or another previously mitigated problem, append a new post-RL-157
entry with an earlier-observation citation and current evidence; do not edit
RL-001 through RL-157.

### 2026-08-25 r24 re-sync and full-suite recurrence audit — no new go-back

The fresh 2025 export
`.tmp/corpus-bundle-resync-2026-08-25-r24-2025.json` was exported directly
from `../Document-Recreation` at the locked revision
`4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`. Comparing r24 with the immediately
preceding r23 candidate reports `changed_documents: 0`: the 20 Part 1, 1,860
Part 1.1, and 340 Part 1.2 requirements, 1,264 API surfaces, 283 mappings,
297 transitions, 1,410 API crosswalks, and 1,894 requirement/API bindings are
unchanged. No requirement, ordinal, clause, API, mapping, binding, or
transition was added, removed, renumbered, or content-replaced. Comparing the
fresh candidate with the older pinned `.compliance/corpus-bundle.json` still
shows only the documented additive Part 1.1 working-tree overlay (1,263 to
1,264 API surfaces, 282 to 283 mappings, and 282 to 297 transitions); that is
baseline lag, not a Requirements Lab numbering change.

The complete configured Debug CTest run before this slice reached all 1,106
tests. Its only non-green entries were the intentionally out-of-scope 2010
time-marshal test (`Not Run`) and the installed-package smoke test's managed
Windows SDK permission error (`ToolLocationHelper` could not read
`C:\Users\peanu\AppData\Local\Microsoft SDKs`). Re-running the
identical package test with the required build permission passed. The increase
from 1,101 to 1,106 is the five newly registered native 2025 slices; no
previous test was removed or relabeled. This is the same environment
permission edge previously noted in r18/r21, not a Lab or Umbra consumer
defect, so it consumes no observation number.

The direct r23-to-r24 comparison and the post-RL-157 regression boundaries
reproduced no old Lab or Umbra consumer issue. RL-176 remains the latest
numbered post-RL-157 go-back; RL-177 is the separate source-artifact tension
recorded below, not a recurrence. If a future pass reproduces an earlier issue
or finds it still unresolved after its expected mitigation, append that fresh
reproduction as RL-178 (or the next unused identifier) with its earlier
citation and current evidence; do not edit RL-001 through RL-157 or hide the
recurrence in an unnumbered audit note.

### 2026-08-25 local slice — changed-lookahead regulation re-enable

The native C++ `Embedded queued timestamped interaction survives
time-regulation disable and re-enable with changed lookahead` case now covers
the producer transition left open by the same-lookahead companions. It queues
one timestamp-five interaction under lookahead one, disables regulation,
callback-gated re-enables it at lookahead three, confirms the new value through
`Query Lookahead`, and advances the producer to logical time two so the changed
current-lookahead boundary releases the constrained recipient. The recipient
receives exactly one callback before its grant with the original payload, tag,
producer, timestamp/order, and valid retraction metadata; a second retract is
classified as `MessageCanNoLongerBeRetracted`.

The paired 2025 requirements/API contracts resolve against both the pinned
bundle and the direct r24 export. The focused
`timestamped-interaction-regulation-reenable-changed-lookahead` service lane
runs the Catch2 case plus both traceability checks, and the focused service
lane catalog passes; the direct selector reports 61 assertions in one test
case. This is additive native C++ development-profile traceability, not a Lab
or Umbra consumer recurrence, so it consumes no observation number: RL-176
remains the latest post-RL-157 recurrence. RL-177 was reserved when this slice
was recorded; it is consumed by the separate source-artifact entry below, not
by this slice.
Alternate advance forms, other timestamped families, transport, save/restore,
package/JUnit/protected-review evidence, validation, and conformance remain
open.

### 2026-08-25 local slice — changed-lookahead regional interaction companion

The native C++ `Embedded queued timestamped regional interaction survives
time-regulation disable and re-enable with changed lookahead` case now covers
the explicit source-region form of the same producer transition. It queues an
overlap-qualified timestamp-five `Send Interaction With Regions` under
lookahead one, disables regulation, callback-gated re-enables it at lookahead
three, confirms the changed value through `Query Lookahead`, and advances the
producer to logical time two. The constrained recipient receives exactly one
callback before its grant, with the original source `RegionHandle` set,
payload, tag, producer, timestamp/order, and valid retraction metadata intact.

The existing regional Requirements-Lab/API contracts now reference this test,
and the dedicated CTest service lane runs those checks together with the
Catch2 selector. The r23-to-r24 2025 re-sync remains content-stable and the
recurrence guard still reports RL-178 as the next available identifier. This
is additive native C++ development-profile evidence, not a reproduced Lab or
Umbra consumer issue; no observation number is consumed. Alternate advance
forms, subscription mutation, ownership/resignation, save/restore, transport,
package/JUnit/protected-review evidence, validation, and conformance remain
open.

### 2026-08-25 local slice — Clause 3.3.1 name-convention preflight

The native composition preflight now checks the 2025 Clause 3.3.1 naming
boundary using libxml2's NCName validation plus the HLA period, reserved-prefix,
and `NA` rules. The focused Catch2 case passes 35 assertions, and the
`fom-name-conventions` lane passes its Catch2 and direct r24 traceability tests.
The complete FOM-labeled slice also passes all 122 tests after one ordering
correction: the first run allowed the generic reserved-`NA` name diagnostic to
mask the existing, more precise `UmbraNaInvalidTransportation` companion
diagnostic. The composer now evaluates that table-specific companion rule
before the generic name pass, preserving both validations.

This was a local implementation ordering correction, not a Requirements Lab
or historical consumer go-back. It consumes no recurrence identifier; the
separate RL-177 entry below records only the official Restaurant source
tension.

### 2026-08-25 local tooling slice — ordinal-numbering re-sync guard

The read-only `requirements_lab.py resync` report now exposes an
`ordinal_drift` collection alongside semantic additions/removals and
regenerated opaque IDs. This closes a bookkeeping blind spot: a Requirements
Lab exporter can preserve a requirement's text and ID while moving its
presentation ordinal, and that numbering change must remain visible to a
reviewer. The new offline
`tools/requirements_lab_resync_regression.py` guard proves ordinal-only and
combined ID/ordinal changes are classified separately without modifying the
pinned bundle. The optional `--fail-on-numbering-drift` switch lets a review
gate reject only ID/ordinal movement when a deliberate working-tree export
also contains additive semantic records.

The live r23-to-r24 2025 comparison still reports `changed_documents: 0`, with
zero requirement ordinal drift across all 1,860 Part 1.1 records (and zero
drift in the other 2025 collections). The focused Requirements-Lab CTest label
now passes 248/248 tests, including the new guard; the configured build
contains 1,107 tests in total. This is an additive local traceability/tooling
slice, not a Requirements Lab or Umbra consumer recurrence, so it consumes no
observation number. RL-176 remains the latest numbered go-back, RL-177 is the
separate source-artifact tension, and RL-178 is the next identifier for a
future reproduced post-RL-157 recurrence.

### 2026-08-25 local slice — Table 1 modification-date lexical form

The native FOM composition preflight now enforces the exact
`YYYY-MM-DD` presentation required for a supplied object-model
`modificationDate`. The official DIF schema continues to own XML Schema
calendar validity; the new check closes the narrower gap where `xs:date`
would otherwise accept a timezone suffix such as `2025-02-10Z`. The focused
Catch2 selector passes 15 assertions, and the paired
`fom-modification-date` Requirements-Lab traceability test resolves against
the pinned/direct 2025 exports.

This is an additive local C++/FOM preflight slice, not a Requirements Lab or
Umbra consumer go-back, so it consumes no recurrence identifier. RL-176
remains the latest numbered go-back, RL-177 remains the separate Restaurant
`NA` source tension, and RL-178 remains reserved for the next genuine
post-RL-157 recurrence.

### 2026-08-25 local full-suite audit — expected environment boundaries

The configured Debug CTest run now enumerates 1,109 tests. All newly added
2025 FOM and traceability selectors pass. The only non-green results are the
intentionally out-of-scope 2010 time-marshal smoke test (`Not Run` because its
optional executable was not built) and the installed-package smoke test's
managed Windows SDK permission failure. The latter is the same environment
boundary already recorded in the earlier audit notes; rerunning that one test
with the required build permission passes. It is not a Requirements Lab or
Umbra consumer regression. No old issue was re-exposed, so this audit
consumes no post-RL-157 identifier: RL-176 remains the latest numbered
go-back, RL-177 remains the separate source-artifact tension, and RL-178 is
reserved for the next genuine recurrence.

### RL-177 — Clause 3.3.1 `NA` reservation conflicts with the official Restaurant enumerator

**Status:** verified 2025 source-artifact tension; not a Requirements Lab
numbering change or conformance finding.

The direct 2025 export
`.tmp/corpus-bundle-resync-2026-08-25-r24-2025.json` from the adjacent Lab at
revision `4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` is content-stable against
r23: no requirement, clause, API, mapping, binding, or transition identity
changed. The relevant immutable 1516.2 record is
`requirement-candidate-sections-semantic-clause-4c-page-073-l91-1`, the
enumerator-name row used by the private name-convention contract.

The reconstructed 1516.2-2025 Clause 3.3.1 page 32 says a name consisting of
case-insensitive `na` is reserved as the non-applicable marker and cannot be a
user-defined name. The supplied official
`third_party/ieee1516.2-2025/resources/examples/RestaurantFOMmodule-2025.xml`
nevertheless declares the `Modifiable` enumerator with `<name>NA</name>` at
line 897. The official DIF XSD accepts that value, so a literal global `NA`
rejection would make the packaged Restaurant example fail its own official
resource validation.

Umbra's bounded 3.3.1 preflight therefore rejects `NA` for ordinary named
declarations but preserves a narrowly scoped enumerator exception for this
source compatibility boundary. The exception is covered by the native
`The FOM composition preflight enforces HLA 3.3.1 XML names` case and is
explicitly marked private traceability; it is not evidence that either the
prose rule or the example has been resolved.

**Possible Lab/artifact refinement:** publish an erratum or an explicit
enumerator-marker exception for the Restaurant example, and expose that
exception in the exported semantic metadata. Until an edition decision is
made, the source conflict must remain visible here and in
`compliance/requirements-lab/fom-name-conventions-requirements-contract.json`.

### 2026-08-25 local slice — rejected HLAsetSwitches count boundary

The native MOM interaction-count lane now submits an invalid
`HLAsetSwitches` `HLAresignAction` value before any application traffic. The
typed `RTIinternalError` is raised and the sender's direct
`HLAinteractionsSent` value remains zero; accepted ordinary, directed,
timestamped, and regional sends then advance the count from the normal service
boundary. This closes a focused no-positive-result test gap in the existing
MOM count scenario. It is additive C++ development-profile evidence, not a
Requirements Lab or Umbra consumer recurrence, so it consumes no observation
identifier. RL-177 remains the separate source-artifact tension and RL-178 is
still the next unused post-RL-157 recurrence slot.

### 2026-08-25 local slices — changed-lookahead attribute and deletion companions

The non-regional timestamped `Update Attribute Values` and `Delete Object
Instance` companions now cover the same producer transition independently:
each passel is accepted at lookahead one, retained through Disable Time
Regulation, re-enabled at lookahead three, checked through `Query Lookahead`,
and released at timestamp five after the producer advances to logical time two.
Each focused C++ case proves its callback before the matching grant and retains
the original payload/object, tag, producer, timestamp/order, and retraction
metadata. Their exact 2025 Requirements-Lab/API contracts and dedicated CTest
lanes pass against the locked export. The configured Debug build now
enumerates 1,115 tests; the complete `requirements-lab` label also passes.
These are additive development-profile evidence, not Lab or Umbra consumer
recurrences, so no observation number is consumed: RL-177 remains the separate
source-artifact tension and RL-178 remains the next genuine post-157 go-back.

### 2026-08-25 local slice — changed-lookahead directed interaction companion

The non-regional timestamped directed-interaction companion covers the producer
transition independently: a target-qualified passel is accepted at lookahead
one, retained through Disable Time Regulation, re-enabled at lookahead three,
checked through `Query Lookahead`, and released at timestamp five after the
producer advances to logical time two. The focused C++ case proves the directed
callback before the matching grant and retains the original target, tag,
producer, timestamp/order, and retraction metadata. Its exact 2025
Requirements-Lab/API contracts and dedicated CTest lane pass against the locked
export. This is additive development-profile evidence, not a Lab or Umbra
consumer recurrence, so no observation number is consumed: RL-177 remains the
separate source-artifact tension and RL-178 remains the next genuine post-157
go-back.

While authoring the directed contract, the first local draft labeled the
`Send Directed Interaction` anchor as clause 6.14; the locked export checker
correctly resolved that requirement to clause 6.16, and the contract was
corrected before the lane ran. This was a local contract-authoring mismatch,
not a Lab content change or a recurrence, and therefore consumes no `RL-###`
identifier.

### 2026-08-25 recurrence-ledger guard tightening

The observation checker and its offline regression lane now treat explicit
“not fixed,” “not actually fixed,” and failed-mitigation wording as recurrence
markers, in addition to the existing re-exposure/regression/unresolved forms.
The temporary-ledger cases accept a cited post-RL-157 reproduction, reject the
same not-fixed reproduction without an earlier citation, reject a skipped
identifier, and reject reuse of an immutable RL-001..RL-157 identifier. The
live ledger still has no new reproduction: the next available local identifier
is RL-178. This guard change is workflow hardening, not a new Lab or Umbra
consumer issue, so it consumes no observation number.

### 2026-08-25 local slice — changed-lookahead regional attribute update companion

The native C++ development profile now has the matching explicit-source
regional `Update Attribute Values` changed-lookahead companion. The producer
queues one overlap-qualified timestamped passel at lookahead one, disables
Time Regulation, callback-gated re-enables it at lookahead three, verifies the
new value through `Query Lookahead`, and advances to logical time two so the
timestamp-five GALT boundary releases `Reflect Attribute Values` before the
recipient's grant. The callback preserves the object/update payload, source
`RegionHandle` set, tag, producer, timestamp/order, and retraction identity.
The focused C++ test passes 64 assertions; its exact Requirements-Lab/API
contracts and CTest label pass 3/3, and the shortened convenience target
avoids the managed Windows MSBuild path-length boundary without changing the
standards-facing selector.

The r23-to-r24 2025 export remains unchanged (1,860 Part 1.1 requirements,
1,264 API surfaces, and zero ordinal drift). This is additive development-
profile evidence, not a Requirements Lab or Umbra consumer recurrence, so no
observation number is consumed: RL-177 remains the separate source-artifact
tension and RL-178 is the next genuine post-RL-157 recurrence slot.

### 2026-08-26 r25 re-sync audit — no 2025 requirement-number drift

The adjacent Requirements Lab was exported again at the pinned
`v0.1.0.a1` revision `4bafa0619cf8c777a79294c9e0e78f2a38ee55b4` and compared
with the previous r24 candidate. The 2025 Part 1, Part 1.1, and Part 1.2
documents are content-stable: `changed_documents: 0`, with 20, 1,860, and 340
requirements respectively. Part 1.1 still contains 1,264 API surfaces, 283
mappings, 297 transitions, 1,410 API crosswalks, and 1,894
requirement/API bindings. No requirement, API-surface, mapping, transition,
crosswalk, binding, clause, immutable ID, or ordinal changed, so no contract or
Catch2-plan remap is warranted.

The configured Debug `requirements-lab` CTest label remains green at 259/259,
including the plan, resync, immutable-history, and recurrence guards. The
candidate-specific plan check also resolves all 609 active entries (506
requirements and 243 C++ API surfaces) against the unchanged 2025 export.

The observation checker still reports 177 numbered entries, latest `RL-177`,
and next available `RL-178`. This unchanged expected re-sync is not a Lab or
Umbra-consumer recurrence and consumes no observation identifier. If a prior
RL-001..RL-157 issue is reproduced or is found still unresolved on a later
workflow pass, append the next post-157 identifier and cite the earlier entry
under the recording rule below; do not edit the historical record or treat a
numbering concern as a renumbering event.

### 2026-08-26 current-checkout r26 re-sync audit — no 2025 numbering drift

The adjacent checkout has advanced to `ef10c87d911f33018470d355c0aa942b953c4119`
since the locked `v0.1.0.a1` release. I exported its 2025 documents as the
ignored r26 candidate and compared them with the r25 candidate. All three
documents remain content-stable (`changed_documents: 0`): Part 1 has 20
requirements, Part 1.1 has 1,860 requirements, 1,264 API surfaces, 283
mappings, 297 transitions, 1,410 API crosswalks, and 1,894 requirement/API
bindings, and Part 1.2 has 340 requirements. There are no added, missing,
renumbered, or ordinal-drifted requirement, API, mapping, transition,
crosswalk, binding, or clause records.

The lock remains intentionally pinned to `4bafa0619cf8c777a79294c9e0e78f2a38ee55b4`
until the adjacent Lab publishes a reviewed release; r26 is comparison
evidence, not a replacement canonical bundle. The candidate-specific plan,
resynchronization, and observation guards remained green at the time of this
audit, and the full Debug `requirements-lab` label was 259/259. No old
Lab/Umbra consumer issue had been reproduced at that point, so the audit itself
consumed no observation identifier. The later RL-178 entry below records the
focused-lane recurrence found by the complete native-suite audit.

### RL-178 — Focused lane omitted traceability labels for a changed-lookahead slice

**Status:** verified Umbra consumer/traceability recurrence; not a Requirements
Lab numbering change or a conformance finding.

The r26 current-checkout export remains content-stable against r25: all 2025
requirement, API-surface, mapping, transition, crosswalk, binding, clause, and
ordinal identities are unchanged. The active Catch2 plan and both new
changed-lookahead contracts resolved successfully. However, the complete
native CTest run exposed a focused-lane catalog failure: the
`timestamped-attribute-update-regulation-reenable-changed-lookahead` Catch2
case was present, but its two Requirements-Lab/API traceability tests were not
assigned the lane label. This was the same class of consumer-side
traceability-selection weakness guarded by RL-160 and re-exposed by RL-176,
although the IDs themselves were valid here.

Umbra corrected the CMake label registration with an exact
`timestamped_attribute_update_regulation_reenable_changed_lookahead_(requirements|api)_traceability`
match. After reconfiguration, the focused catalog passed, and the lane now
runs all three members (one Catch2 behavior case plus the Requirements-Lab and
API-contract checks) successfully. No Lab artifact changed; the recurrence is
therefore recorded as an Umbra consumer regression with its mitigation rather
than folded into the unchanged r25/r26 resynchronization audit.

### RL-179 — Native Catch2 source refresh dropped mapped ownership selectors

**Status:** verified Umbra consumer/traceability recurrence; not a Requirements
Lab numbering change or a conformance finding.

The pinned `v0.1.0.a1` Lab export and all 2025 requirement/API identifiers were
unchanged. On 2026-08-27 the local Catch2 plan guard reported 18 mapped plan
entries whose ownership/MOM selectors were absent from the native C++ source
(17 unique selectors). The corresponding contracts also failed their local
source/test selector checks, so the issue blocked the normal focused planning
guards even though no Lab content had drifted. This is the same class of
consumer-side selector recurrence recorded by RL-160 and RL-176.

Umbra restored the exact missing baseline Catch2 cases, removed the accidental
duplicate selector, and kept the new public fresh-registry restore case mapped
through the plan and its six affected API/requirements contracts. The plan
now resolves 646 entries; all 260 Requirements Lab contracts, the plan guard,
the roadmap index guard, the tag taxonomy guard, and the observation ledger
pass. No Lab artifact changed. The mitigation is to keep the source-selector
guard in the focused workflow and reconcile a source/test refresh before
advancing the roadmap lane.

### 2026-08-28 local slice — directed TSO ownership callback boundary

The focused `process-restart-directed-interaction-tso-ownership-callback`
case now passes in the native C++ lane. It saves one queued by-ownership
directed timestamped payload, restores the directed declarations, one
application-value target, ownership ledger, retraction ledger, and queue in a
fresh registry, transfers the marker and implicit delete privilege before the
old owner's callback boundary, suppresses the stale callback, completes the
in-transit queue entry, and proves a legal Retract is terminal without a
Request Retraction callback. This exposed and closed an Umbra admission gap:
the restart predicate previously rejected a valid combined image containing
directed declarations plus directed TSO/retraction/queue state even though
each section was independently restartable. The predicate now admits only
that narrow directed combination and continues to reject ordinary,
attribute-update, and object-deletion TSO mixtures. The case is mapped through
the existing selector, object, ownership, save/restore, TSO, and retraction
requirements; the pinned Requirements Lab export did not change, so no new
RL observation number is consumed.

### 2026-08-28 local slice — directed TSO eligible by-ownership delivery/retraction

The focused
`process-restart-directed-interaction-tso-ownership-delivery-retraction` case
now passes in the native C++ lane. It saves one queued directed timestamped
interaction for the current by-ownership subscriber, restores the image into a
fresh registry, proves that the owner remains eligible, delivers the payload,
and applies a legal producer `Retract` that emits exactly one `Request
Retraction` notification. The same file/queue/retraction identity is retained
through the terminal save state. This is a local Umbra evidence slice using
the pinned 2025 Lab IDs and standard mappings; the Lab export did not change,
so no new RL observation number is consumed. The next bounded implementation
target is a multi-recipient positive/negative timestamped fan-out case.

### 2026-08-28 local slice — directed TSO multi-recipient fan-out

The focused
`process-restart-directed-interaction-tso-fanout-positive-negative` case now
passes in the native C++ lane. It saves one directed timestamped payload with
two recipient entries, restores both entries in a fresh registry, removes the
directed subscription from one recipient before its callback boundary, delivers
the still-eligible recipient, suppresses the unsubscribed recipient, and
applies a legal producer `Retract` that emits exactly one `Request Retraction`
for the delivered recipient. The recipient ledger preserves the distinct
retracted and suppressed terminal states through the terminal save. This is a
local Umbra evidence slice using the pinned 2025 Lab IDs and standard mappings;
the Lab export did not change, so no new RL observation number is consumed.
The latest bounded target is a non-empty directed TSO parameter projection with
per-recipient transportation/order validation using a dedicated 2025 fixture.

### 2026-08-28 local slice — directed TSO parameter projection

The focused
`process-restart-directed-interaction-tso-parameter-projection` case now passes
in the native C++ lane. A dedicated 2025 DIF fixture defines one directed
interaction parameter and `TimeStamp` order. The case resolves that official
parameter handle, saves the non-empty parameter bytes and recipient projection,
restores them in a fresh registry, verifies HLAreliable transport plus
timestamped order metadata, and delivers the projected parameter before a
legal producer `Retract` emits one `Request Retraction`. This is a local Umbra
evidence slice using pinned Lab IDs and standard mappings; the Lab export did
not change, so no new RL observation number is consumed.

### 2026-08-28 local slice - ordinary regional interaction TSO/DDM restore

The official 2025 C++ binding exposes `sendInteractionWithRegions` but no
`sendDirectedInteractionWithRegions` overload. The roadmap therefore advances
the supported ordinary regional interaction path rather than inventing a
directed-with-regions API. The focused
`process-restart-regional-interaction-tso-ddm` case composes the official
Restaurant FOM's dimensioned `MainCourseServed` interaction, captures one
committed `[2,4)` source-region snapshot and two overlap-qualified recipients,
saves one recipient in transit and one queued, mutates and resigns the source,
and restores into a fresh registry. It proves the saved snapshot remains
available for callback-boundary evaluation even after a disjoint live source
range, while both recipient queue phases and timestamped order metadata survive.
This is a local Umbra evidence slice using pinned Lab IDs and standard mappings;
the Lab export did not change, so no new RL observation number is consumed.
Directed DDM remains a separate future design boundary.

### 2026-08-28 local slice — public regional TSO/DDM restore and report-file identity

The public C++ companion
`public-process-restart-regional-interaction-tso-ddm` now passes in the
development profile. It uses the configurable filesystem service-report
directory, commits a timestamped `Send Interaction With Regions` payload at
time 9 beyond a timed save at time 7, mutates the live source region, restores
the saved joined federation, and verifies the original `[2,4)` bounds, queued
payload, parameter/tag bytes, timestamped order, and sent `RegionHandle`.
The joined federate's report pathname remains the same file and receives the
save/restore service records; switches gate appends without replacing the
file. This is a local Umbra evidence slice using the same pinned Lab IDs and
standard mappings as the private regional case. The Lab export did not change,
so no new RL observation number is consumed. The public-facade fresh-registry
route-rebinding companion is now green as a separate bounded slice.

### 2026-08-28 local slice — public fresh-registry regional TSO/DDM

The public C++ fresh-registry companion
`public-process-restart-regional-interaction-tso-ddm` now passes against a
new `EmbeddedFederationRegistry` and the durable filesystem save store. It
creates three public ambassadors (one publisher and two regional recipients),
saves a timestamped interaction at time 9 across the time-7 save boundary,
tears down the source routes, recreates the federation and callback routes,
and verifies both recipient-specific queue entries deliver the original
`[2,4)` invocation snapshot and payload. The source report file remains
present while the new joined-federate lifetime receives a distinct immutable
report path.

This slice also exposed and closed a real runtime rough edge: the regional
route-free admission previously rejected a persisted publisher
`Change Interaction Order Type(TIMESTAMP)` entry, and the restore materializer
did not rehydrate that order override. The predicate and state-image restore
now admit only the single publisher TIMESTAMP override for this narrow image
and restore it before delivery. This was an Umbra implementation defect, not a
Requirements Lab export change; no new RL observation number is consumed.
The exact plan entry carries the same 15 pinned Lab requirement IDs, 12
standard clause mappings, and 29 official C++ API surfaces as the private and
public process-local regional companions.

### 2026-08-28 local slice — public fresh-registry regional attribute-update TSO/DDM

The public C++ companion
`public-process-restart-regional-attribute-update-tso-ddm` now passes through
the same filesystem save and fresh-registry boundary. It establishes a durable
baseline object value, queues one timestamped regional `Update Attribute
Values` passel for two overlap-qualified recipients, saves at logical time 7,
mutates the live source region to a disjoint range, tears down the source
ambassadors, and restores into a new registry with new public callback routes.
Both recipients then receive exactly one reflection with the saved `[2,4)`
source-region snapshot, original payload/tag, timestamped order, and valid
retraction metadata. The source report file remains present and the fresh
joined lifetime receives a distinct immutable report path.

This slice exposed a local restore-admission edge rather than a Requirements
Lab defect: an object ledger with a live regional update association was being
classified as route-free only by the generic application-value predicate. The
registry now admits this narrow image only when its persisted regional passel,
source-region snapshot, and recipient passel agree; the association remains
live through save and is removed during teardown. The pinned Requirements Lab
export and requirement/API IDs did not change, so no new numbered RL
observation is consumed.

### 2026-08-28 local slice — public fresh-registry timestamped object deletion

The public C++ companion
`public-process-restart-object-deletion-tso` now passes through the filesystem
save and fresh-registry boundary. It publishes the official
`HLAprivilegeToDeleteObject` attribute, establishes an object-value ledger,
queues one timestamped `Delete Object Instance` for two recipients, saves at
logical time 7, tears down the source ambassadors, and restores into a new
registry with new callback routes. Both recipients then receive exactly one
`Remove Object Instance` callback at timestamp 9 with the original object
identity, tag, producing federate, timestamped order, and valid retraction.
The source report file remains present and the fresh joined lifetime receives
a distinct immutable path.

### 2026-08-28 local slice — public fresh-registry object-deletion retraction

The public C++ companion
`public-process-restart-object-deletion-retraction` is now green without a
Requirements Lab rescan or a new numbered observation. It saves one
timestamped `Delete Object Instance` with two recipient ledger entries,
recreates the public routes in a fresh registry, flushes only receiver A's
copy, and uses the saved `MessageRetractionHandle` through the fresh owner.
Receiver A receives one valid `Request Retraction` after its historical
`Remove Object Instance`; receiver B receives neither stale removal nor
retraction. The object/name reconstitution and stable filesystem report-file
identity are checked in the same bounded lane.

This is a bounded public-facade use of the existing route-free object-deletion
invocation contract; no new Requirements Lab extraction issue or numbering
change was observed. The pinned Lab export and its requirement/API IDs are
unchanged, so no new numbered RL observation is consumed.

### 2026-08-28 local slice — public fresh-registry directed TSO fan-out

The public C++ companion
`public-process-restart-directed-interaction-tso-fanout` is green without a
Requirements Lab rescan. It restores one timestamped directed interaction
through a fresh registry with two explicit directed subscribers and one neutral
joined member retained by delayed subscription evaluation. The saved publisher
`TIMESTAMP` order, two concrete queue entries, three recipient states, and
filesystem report-file identities remain stable; Flush Queue delivers only the
eligible subscriber and a fresh producer `Retract` emits one recipient-local
`Request Retraction` while the other subscriber and neutral route stay
suppressed.

The implementation uncovered a bounded restore-admission edge: the directed
route-free predicate rejected neutral declarations and the directed publisher's
saved order override. The runtime now admits those two states explicitly while
keeping ordinary interaction declarations outside this predicate. This is an
Umbra implementation correction, not a Requirements Lab defect or recurrence;
the pinned 2025 export and all mapped IDs remain unchanged, so no new numbered
RL observation is consumed.

### 2026-08-28 local slice — public directed TSO parameter projection

The public C++ companion
`public-process-restart-directed-interaction-tso-parameter-projection` is green
without a Requirements Lab rescan. It composes the dedicated 2025
one-parameter directed-interaction fixture, saves a queued non-empty parameter
payload at a timestamped boundary, restores the declaration and parameter
projection through a fresh registry, and verifies HLAreliable plus sent/received
`TIMESTAMP` metadata on the official callback surface. The configured sender
report pathname remains stable across the source and fresh joined-federate
lifetimes. The pinned Lab export and all mapped IDs are unchanged; no new
numbered RL observation is consumed.

## Recording rules

### Post-RL-157 recurrence rule

The first 157 observations are historical records and remain immutable. If a
previously recorded Lab or consumer issue is encountered again on a new
workflow pass after it was believed fixed—or is found to remain unresolved
when a fix was expected—record the go-back/reoccurrence under the next unused
`RL-###` identifier rather than editing the earlier entry or treating the
recurrence as a numbering change. The new identifier must remain strictly
after RL-157, link to the earlier observation, state whether the Lab export
changed, and describe the current reproduction and mitigation status.
This applies even when the original issue was still unresolved: the fresh
reproduction is useful evidence that the expected fix did not hold. RL-172 is
the first explicit example of this rule: it links back to RL-156 because the
FOM/DIF declaration-fence interpretation was reintroduced by a downstream
guard even though the locked 2025 Lab revision and requirement IDs were
unchanged.

An unchanged, expected re-sync is not by itself a recurrence and does not
consume an observation number; record that result in the relevant audit entry
or dated audit section instead. Do not reuse an old ID for a new reproduction
after a mitigation was expected to close the issue.

When Umbra finds a new issue while exporting, checking, or using the Lab, add
an entry here with:

1. the pinned Lab revision and immutable record/mapping IDs;
2. the exact observed fields or checker result;
3. the effect on Umbra's traceability or test workflow; and
4. a clearly marked refinement proposal, if any.

The local `tools/requirements_lab.py check` also verifies that every `RL-###`
heading is unique and that a post-RL-157 recurrence cites its earlier issue,
so renumber an entry rather than reusing an existing observation identifier or
folding a recurrence back into the historical record.

The recurrence check evaluates each numbered observation only through the next
level-3 heading. This keeps an unnumbered local-slice note from accidentally
providing a citation for a different numbered entry. A recurrence marker must
cite an earlier numbered observation that actually exists; a reference to a
missing earlier `RL-###` is a ledger error. The narrower standalone command
`tools/requirements_lab.py check-observations` is also registered as a CTest
lane so this boundary is checked without requiring a corpus export.
The marker vocabulary includes explicit regression language as well as plain
statements that an old requirement, issue, defect, problem, or finding is
“still unresolved,” “remains unresolved,” “not fixed,” or that its expected
fix/mitigation did not hold. This catches a go-back even when a new entry does
not use the word “recurrence.” The checker reports the next available local
identifier (currently RL-180); use that identifier for the next genuine
reproduction and cite the earlier observation rather than editing it.

The checker also compares the normalized RL-001..RL-157 section blocks and
their file order with
`compliance/requirements-lab/observations-historical-baseline.json`. A
historical edit or reorder therefore fails the local check instead of being
mistaken for a new post-RL-157 issue; new recurrences must be appended under
the next unused identifier.

Use **verified** only for reproducible data/checker facts. Use **possible
refinement** for a proposed Lab change. Do not relabel an observation as a
standards or conformance failure without independent evidence.

Entries stay in this log after Umbra has a local mitigation, so a future Lab
revision can be compared against the original decision and the relevant
Umbra contracts can be updated deliberately.

### 2026-08-28 local slice — public regular ownership-acquisition restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves a pending regular Attribute Ownership
Acquisition with its owner-side release reservation, restores it through fresh
official callback routes, and verifies the object/attribute set, acquisition
tag, and distinct filesystem service-report lifetimes. This is development-
profile evidence only; it does not promote Lab validation or conformance.

### 2026-08-28 local slice — public If Available ownership-acquisition restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves a pending If Available acquisition for an
owner-held attribute, restores it through fresh official callback routes, and
verifies the requester-side unavailable callback's object/attribute set,
acquisition tag, and distinct filesystem service-report lifetimes. This is
development-profile evidence only; it does not promote Lab validation or
conformance.

### 2026-08-28 local slice — public negotiated owner-confirmation restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves a pending regular acquisition selected by
negotiated Attribute Ownership Divestiture, restores it through fresh official
callback routes, and verifies the owner-side Request Divestiture Confirmation
callback's object/attribute set and acquisition tag, the negotiated ledger, and
distinct filesystem service-report lifetimes. This is development-profile
evidence only; it does not promote Lab validation or conformance.

### 2026-08-28 local slice — public negotiated If Available owner-confirmation restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves an If Available acquisition selected by
negotiated Attribute Ownership Divestiture, restores the owner-side confirmation
callback through fresh official routes, suppresses the duplicate requester WTA
callback, and preserves the negotiated ledger and distinct filesystem
service-report lifetimes. This is development-profile evidence only; it does
not promote Lab validation or conformance.

### 2026-08-28 local slice — public delivered negotiated owner-confirmation restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now delivers the owner-side Request Divestiture
Confirmation before the durable save, verifies the persisted
`confirmationDelivered` marker, restores into a fresh registry without replaying
the callback, and completes the retained transfer through Confirm Divestiture
with one requester acquisition notification. It also verifies distinct,
immutable filesystem service-report identities across the two joined-federate
lifetimes. This is development-profile evidence only; it does not promote Lab
validation or conformance. The next bounded slice is the private mixed
delivered negotiated-confirmation case.

### 2026-08-28 local slice — public delivered negotiated If Available confirmation restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now delivers the owner-side Request Divestiture
Confirmation for an If Available acquisition while leaving the requester WTA
route untouched for the save boundary, verifies the persisted
`confirmationDelivered` marker, restores into a fresh registry without replaying
either callback, and completes the retained transfer through Confirm Divestiture
with one requester acquisition notification. It also verifies distinct,
immutable filesystem service-report identities across the two joined-federate
lifetimes. This is development-profile evidence only; it does not promote Lab
validation or conformance. The next bounded slice is the private mixed delivered
negotiated-confirmation case.

### 2026-08-28 local slice — public mixed negotiated-ownership restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves regular and If Available acquisition
requests together behind one negotiated Attribute Ownership Divestiture,
restores both pending confirmation routes through a fresh registry, completes
one grouped Confirm Divestiture transfer, and preserves both immutable
filesystem service-report lifetimes. The case uses only attributes defined by
the selected fixture FOM (`ReliableBaseA` and `ReliableBaseB`); the earlier
draft names `Efficiency` and `Cheerfulness` were not present in that fixture
and were corrected before evidence was accepted. This is development-profile
evidence only; it does not promote Lab validation or conformance.

### 2026-08-28 local slice — public mixed delivered negotiated-confirmation restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now delivers both owner-side Request Divestiture
Confirmation callbacks before the durable save while leaving the requester
If Available route pending, verifies both `confirmationDelivered` markers in
the filesystem image, restores into a fresh registry without replaying either
callback, and completes one grouped Confirm Divestiture transfer with a mixed
acquisition notification. It also verifies distinct immutable filesystem
service-report identities across joined-federate lifetimes. This is
development-profile evidence only; it does not promote Lab validation or
conformance. The next bounded slice is the private asymmetric mixed
negotiated-confirmation case.

### 2026-08-28 local slice — public asymmetric mixed negotiated-confirmation restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now preserves a delivered regular owner confirmation
without replay, reconstructs exactly one pending If Available confirmation in a
fresh registry, completes one grouped Confirm Divestiture transfer, and keeps
the joined-federate filesystem report identities distinct and immutable. The
case uses the official 2025 facade and remains development-profile evidence; it
does not promote Lab validation or conformance. The next bounded slice was the
private reverse asymmetric mixed negotiated-confirmation case, followed by its
public companion below.

### 2026-08-28 local slice — public reverse asymmetric mixed negotiated-confirmation restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now preserves a delivered If Available owner
confirmation without replay, reconstructs exactly one pending regular
confirmation in a fresh registry, completes one grouped Confirm Divestiture
transfer, and keeps the joined-federate filesystem report identities distinct
and immutable. The callback planner orders mixed confirmations by the shared
acquisition sequence, so the reverse request order is deterministic. This is
development-profile evidence only; it does not promote Lab validation or
conformance. The malformed mixed-confirmation rejection, Divestiture-If-Wanted
notification, Confirm Divestiture notification, and ownership-cancellation
restore gates are now green. The next bounded validation is the attribute
transportation-type-change restore boundary; query it with the indexed
`process-restart-attribute-transportation-type-change` lane and its exact
Catch2 test name.

### 2026-08-28 local slice — public pending attribute transportation-type-change restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves one durable application value alongside a
pending Attribute Transportation Type Change, restores the image into a fresh
registry, rebinds exactly one owner confirmation callback, commits
`HLAbestEffort` at that callback boundary, and proves that a subsequent update
uses the restored effective type. It also verifies that source and fresh joined
federates retain distinct immutable filesystem service-report identities. This
is development-profile evidence only; it does not promote Lab validation or
conformance. The next bounded validation is the private interaction
transportation-type-change restore lane; query it with
`process-restart-interaction-transportation-type-change` and its exact Catch2
test name.

### 2026-08-28 local slice — public pending interaction transportation-type-change restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now admits the bounded two-member publication and
ordinary-subscription declaration image, restores one pending publisher
transportation change into a fresh registry, rebinds exactly one confirmation
callback, commits `HLAbestEffort` at that callback boundary, and proves a
post-restore interaction delivery uses the effective type. It also verifies
distinct immutable filesystem service-report identities. The admission change
is intentionally narrow: regional, directed, multiple-pending, and
committed-override-plus-pending combinations remain outside this lane. This is
development-profile evidence only; it does not promote Lab validation or
conformance. The next bounded validation is the private interaction declaration
restore lane; query it with `process-restart-interaction-declaration` and its
exact Catch2 test name.

### 2026-08-30 local slice — timestamped Send Interaction service reports

No Requirements Lab resynchronization or new numbered observation was needed.
The accepted timestamped `SendInteraction` filesystem case now passes 153
assertions and proves the production report is durable before the queued
`Receive Interaction` callback; disable/re-enable cycles preserve the same
joined-federate file identity. Its public-MOM companion passes 91 assertions
under an immediate observer, decoding the type-27/type-40/type-63/type-31
supplied forms and the type-34 Null return. The time-regulated companion passes
115 assertions, decodes the type-33 `MessageRetractionHandle` return, and
verifies the timestamped callback precedes its grant. These are source-backed
2025 C++ development-profile slices; they do not promote Lab validation or
conformance. The broad service-report label still contains retained
source-missing catalog entries, which remain separate from these exact green
selectors. The next bounded slice is the indexed time-regulated timestamped
`Update Attribute Values` MOM report.

### 2026-08-28 local slice — public mixed interaction declaration restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves a two-member interaction publication and
ordinary subscription declaration image, restores it into a fresh registry,
and proves post-restore interaction delivery while preserving distinct
immutable filesystem service-report identities. The declaration admission is
kept separate from pending and committed transportation overrides. This is
development-profile evidence only; it does not promote Lab validation or
conformance. The next bounded validation is the committed interaction
transportation-type override restore lane; query it with
`process-restart-interaction-transportation-type-override` and its exact Catch2
test name.

### 2026-08-28 local slice — public committed interaction transportation-type override restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now commits a publisher-scoped `HLAbestEffort`
interaction transportation override before saving, restores a two-member
publication/subscription image into a fresh registry, proves the effective
transport through the public query and post-restore delivery, and confirms
that no already-delivered confirmation callback is replayed. Source and fresh
joined federates retain distinct immutable filesystem service-report
identities. This is development-profile evidence only; it does not promote
Lab validation or conformance. The next bounded validation is the private
mixed interaction override restore lane; query it with
`process-restart-interaction-mixed-override` and its exact Catch2 test name.

### 2026-08-28 local slice — public mixed interaction transportation-type override restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves a publisher-scoped committed
`HLAbestEffort` override alongside an independent subscriber declaration,
restores both declaration ledgers into a fresh registry, proves the override
through the public transportation query without replaying its confirmation,
and proves the independent subscriber through a new post-restore publication
and delivery. Source and fresh joined federates retain distinct immutable
filesystem service-report identities. This is development-profile evidence
only; it does not promote Lab validation or conformance. The next bounded
validation is the directed interaction declaration restore lane; query it with
`process-restart-directed-interaction-declaration` and its exact Catch2 test
name.

### 2026-08-28 local slice — public directed interaction target routing restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves a target object with a published marker and
a directed publication/universal subscription pair, restores the object and
declaration ledgers into a fresh registry, and sends a receive-order directed
interaction to the restored target. Only the subscribed recipient receives the
callback, with the restored object/interaction handles, producer identity,
transport, and receive-order metadata intact. Source and fresh joined
federates retain distinct immutable filesystem service-report identities. This
is development-profile evidence only; it does not promote Lab validation or
conformance. By-ownership selectors, target departure, directed DDM,
timestamped/retraction behavior, and distributed transport remain separate
lanes. The next bounded validation is the indexed directed interaction
ownership-handoff lane.

### 2026-08-28 local slice — public directed interaction declaration restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves one directed publication and one universal
directed subscription for the same object-class/interaction-class pair on
separate joined federates, restores both declaration ledgers into a fresh
registry, and proves they serialize back through a second public durable
round-trip. Source and fresh joined federates retain distinct immutable
filesystem service-report identities. This is development-profile evidence
only; it does not promote Lab validation or conformance. Directed send/target
routing, ownership selectors, directed DDM, ordering, timestamped/retraction
behavior, and distributed transport remain separate lanes. The next bounded
validation is directed interaction target routing; query it with
`process-restart-directed-interaction-routing` and its exact Catch2 test name.

### 2026-08-28 local slice — public directed interaction ownership-handoff restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves a target object whose application marker and
implicit `HLAprivilegeToDeleteObject` are owned by the initial by-ownership
subscriber, restores one directed publisher and two by-ownership subscribers
into a fresh registry, and proves the initial owner is selected before the
handoff. It transfers the complete registered ownership set through regular
Attribute Ownership Acquisition and Divestiture If Wanted, then proves the
receive-order directed route follows the new owner after the callback boundary
and survives the public fresh-registry restore. Source and fresh joined
federates retain distinct immutable filesystem service-report identities.

The fixture deliberately transfers the implicit delete privilege as well as
the named marker: the public object class inherits that standard attribute, so
leaving it with the old owner would correctly keep the old subscriber eligible
for a by-ownership directed interaction. An initial source-handle capture
omission in the companion was corrected before accepting the green evidence.
This remains development-profile evidence only; it does not promote Lab
validation or conformance. The next bounded validation is the private
directed timestamped ownership-callback lane; query it with
`process-restart-directed-interaction-tso-ownership-callback` and its exact
Catch2 test name.

### 2026-08-28 local slice — public directed timestamped ownership-callback restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves a timestamped directed interaction admitted
for the initial by-ownership recipient, restores the declaration/object/TSO
ledgers into a fresh registry, transfers the marker plus implicit
`HLAprivilegeToDeleteObject`, and crosses the old recipient's Flush Queue
callback boundary. The public switch-support fixture persists two candidate
recipient routes and two queue entries; the current ownership check suppresses
the stale application callback after handoff. The producer has reached the
strict timestamped Retract boundary by then, so `Retract` correctly reports
`MessageCanNoLongerBeRetracted` and no Request Retraction callback is emitted.
This remains development-profile evidence only; it does not promote Lab
validation or conformance. The next bounded validation is the private directed
timestamped ownership-delivery/retraction lane; query it with
`process-restart-directed-interaction-tso-ownership-delivery-retraction` and
its exact Catch2 test name.

### 2026-08-28 local slice — public directed timestamped ownership-delivery/retraction restore

No Requirements Lab resynchronization or new numbered observation was needed.
The public C++ companion now saves one timestamped directed interaction for an
eligible by-ownership recipient, restores the declaration/object/payload/
retraction ledgers into a fresh registry, delivers it at Flush Queue with
timestamp/order/retraction metadata intact, and proves a legal producer
`Retract` emits exactly one `Request Retraction` for that recipient. Source and
fresh joined federates retain distinct immutable filesystem report identities.
This remains development-profile evidence only; it does not promote Lab
validation or conformance. The next bounded validation is the directed TSO
fan-out lane; query it with
`process-restart-directed-interaction-tso-fanout-positive-negative` and its
exact Catch2 test name.

### 2026-08-28 local slice — default-region alternate-advance retraction

No Requirements Lab resynchronization or new numbered observation was needed.
The C++ suite now has a focused default-source regional object-update case
that queues one timestamp-8 passel for `Time Advance Request Available` and
`Next Message Request Available`, legally retracts it before either callback,
and proves both grant callbacks complete without stale reflection or Request
Retraction. The case uses ordinary registration (there is no public source
`RegionHandle`) while preserving the private full-range default realization.
This is development-profile evidence only; it does not promote Lab validation
or conformance. The next bounded selection should be made from the indexed
time/save/restore queue rather than by re-reading the Requirements Lab.

### 2026-08-28 local slice — Catch2 callback-queue maintenance

The mixed default-region FQR/TARA/NMRA test initially asserted TARA and NMRA
reports after draining only the FQR ambassador. That was a test-harness
omission, not an RTI or Requirements Lab discrepancy: each ambassador owns an
independent callback queue. The test now drains all three recipients explicitly
before asserting their grant-boundary callbacks. Keep this as a local
maintenance note; it does not create a new Lab issue or requirement number.

### 2026-08-30 local slice — timed regional restore red edge retained

The older `Embedded timed federation restore restores a live timestamped
regional interaction at the save boundary` case remains a known one-assertion
red edge: 39 of 40 assertions pass, but the restored report does not yet mark
`sentRegionsSupplied` as expected. The clean default-region counterpart is
green and is the indexed 76-assertion baseline. This is a local Umbra behavior
gap, not evidence of Requirements Lab content or numbering drift; it remains
outside the completion ledger and must not block the next focused plain
Send-Interaction service-report slice.

### 2026-08-30 local slice — receive-order Send Interaction filesystem report

The focused `send-interaction-service-report` filesystem case now passes 145
assertions. It records the accepted non-timestamped `SendInteraction` service
before the separately queued `Receive Interaction` callback, preserves the
official type-27/type-40/type-63/type-34 argument forms, and proves that
disabling and re-enabling the reporting switches does not replace or mutate
the joined federate's report file. The case uses the pinned 2025 requirement
and subsection mappings; no Lab artifact or numbering changed. The next
bounded slice is the source-missing receive-order Send Interaction MOM-
interaction companion.

### 2026-08-30 local slice — receive-order Send Interaction MOM interaction

The focused `receive-order-send-interaction-service-report-interaction`
companion now passes 85 assertions. It routes one accepted, nonregional,
non-timestamped `SendInteraction` through the public
`HLAreportServiceInvocation` interaction to an `HLA_IMMEDIATE` observer,
decodes the type-27/type-40/type-63/type-34 supplied forms and successful-void
type-34 return, and verifies that the ordinary recipient remains queued until
its `HLA_EVOKED` boundary with the original payload, tag, reliable transport,
producer, and null region set. The receive-order requirements and API
contracts now resolve this exact C++ selector; no Requirements Lab artifact or
numbering changed. The broad service-report CTest label still contains older
source-missing selectors, so its traceability failures remain separate local
catalog drift rather than evidence against this green behavior case. The next
bounded slice is the source-missing timestamped Send Interaction filesystem
report.

### 2026-08-30 local slice — timestamped Update Attribute Values MOM report

No Requirements Lab resynchronization or new numbered observation was needed.
The indexed time-regulated timestamped `Update Attribute Values` MOM companion
is now source-backed and green at 119 Catch2 assertions under
`HLA_EVOKED`/`HLA_IMMEDIATE`. It decodes the accepted type-2 object-management
service report with the official type-37/type-2/type-63/type-31 supplied forms,
the quoted type-33 `MessageRetractionHandle` return, success/empty-exception/
serial-zero fields, and verifies the constrained reflection callback's object,
attribute, tag, reliable transport, timestamp/order, producer, and retraction
metadata. The immediate MOM report is observed before the constrained callback;
the callback then precedes its matching grant. This remains
development-profile evidence only and does not promote Lab validation or
conformance.

The focused source-state check is intentionally still non-green only for
retained historical/source-drift entries: the current ledger reports 40 such
errors and 52 plan cases without a C++ source location. Those are catalog
reconciliation items, not a reason to rescan the unchanged Lab. That immediate-
only retraction slice is now complete; the following note records its evidence
and the next mixed-fanout lane.

### 2026-08-30 local slice — immediate-only timestamped Update Attribute Values retraction

No Requirements Lab resynchronization or new numbered observation was needed.
The focused `request-retraction-attribute-update` regression is now
source-backed and green at 69 Catch2 assertions under `HLA_EVOKED`. One
time-regulating publisher sends reliable and best-effort timestamped attribute
passels to a non-time-constrained recipient; both reflections preserve the
shared valid retraction designator, timestamp/order, payload, tag, producer,
transport, and empty-region metadata. A single `Request Retraction` callback
then carries that same designator, and a second producer retract is correctly
terminalized. This remains development-profile evidence only; it does not
promote Lab validation or conformance.

The next bounded slice is the source-missing mixed-fanout timestamped
`Update Attribute Values` retraction case. Query it by the exact lane
`request-retraction-attribute-update-mixed-fanout` and its indexed test title;
do not widen into regional or suppressed-callback variants.

### 2026-08-30 local slice — mixed-fanout timestamped Update Attribute Values retraction

No Requirements Lab resynchronization or new numbered observation was needed.
The focused `request-retraction-attribute-update-mixed-fanout` regression is now
source-backed and green at 75 Catch2 assertions under `HLA_EVOKED`. One
time-regulating publisher sends a qualifying timestamped attribute passel to a
non-time-constrained recipient and a time-constrained recipient. The immediate
recipient receives one reflection with the shared valid retraction designator;
the constrained recipient remains queued. A legal `Retract` produces exactly
one recipient-local `Request Retraction` for the delivered copy and removes the
constrained pending copy, which receives only its grant. This remains
development-profile evidence only; it does not promote Lab validation or
conformance.

The next bounded slice is the source-missing suppressed timestamped interaction
callback case. Query the exact lane
`request-retraction-suppressed-interaction` and its indexed test title; keep
this callback-boundary suppression case separate from regional suppression and
the completed mixed-fanout retraction case.

### 2026-08-30 local slice — suppressed timestamped interaction callback

No Requirements Lab resynchronization or new numbered observation was needed.
The focused `request-retraction-suppressed-interaction` regression is now
source-backed and green at 31 Catch2 assertions under `HLA_EVOKED`. A
non-time-constrained recipient queues a qualifying timestamped interaction;
unsubscribing before callback dispatch terminalizes that recipient without a
`Receive Interaction` callback. A later legal `Retract` consequently emits no
`Request Retraction`, and a second retract is rejected as already terminal.
This remains development-profile evidence only; it does not promote Lab
validation or conformance.

The next bounded slice is the source-missing mixed regional timestamped
attribute update case. Query the exact lane
`timestamped-regional-attribute-update-mixed-advance` and its indexed test
title; keep it separate from the completed callback-boundary suppression and
regional retraction cases.

### 2026-08-30 local slice — suppressed timestamped attribute callback

No Requirements Lab resynchronization or new numbered observation was needed.
The focused `request-retraction-suppressed-attribute-update` regression is now
source-backed and green at 33 Catch2 assertions under `HLA_EVOKED`. An
immediate recipient queues a qualifying timestamped attribute update;
unsubscribing before callback dispatch terminalizes that recipient without a
`Reflect Attribute Values` callback. A later legal `Retract` consequently emits
no `Request Retraction`, and a second retract is rejected as already terminal.
This remains development-profile evidence only; it does not promote Lab
validation or conformance.

The source build also exposed malformed, pre-existing test splices in the dirty
Catch2 translation unit: a transport-loss attribute case contained a displaced
save/restore fragment, the cutoff-deletion case had a truncated callback body,
and the regional resignation helper definition was absent before its callers.
Those localized blocks were restored from the intact repository snapshot
without changing Requirements Lab numbering or widening the current
implementation slice. A follow-up source comparison also caught five timed
regional resignation matrix cases dropped by that splice; those cases were
restored before the final rebuild. Keep this as a source-hygiene note, not as a
new Requirements Lab issue.

### 2026-08-30 local slice — mixed regional timestamped attribute advances

No Requirements Lab resynchronization or new numbered observation was needed.
The explicit-source regional mixed-advance companion is source-backed and green
at 131 Catch2 assertions under `HLA_EVOKED`. One overlap-qualified timestamped
attribute passel reaches the FQR, TARA, and NMRA frontiers before their grants,
preserving the source region and callback ordering metadata.

### 2026-08-30 local slice — mixed regional timestamped attribute retraction

The paired negative companion is source-backed and green at 88 Catch2
assertions under `HLA_EVOKED`. A legal pre-callback `Retract` suppresses all
three regional reflections and Request Retraction callbacks while the FQR,
TARA, NMRA, and producer TAR grants still complete. These remain
development-profile evidence only; they do not promote Lab validation or
conformance.

### 2026-08-30 local slice — ordinary regional timestamped attribute TAR/NMR

No Requirements Lab resynchronization or new numbered observation was needed.
The explicit-source regional TAR/NMR companion is now source-backed and green
at 94 Catch2 assertions under `HLA_EVOKED`. One overlap-qualified timestamped
attribute passel is delivered to both ordinary `Time Advance Request(7)` and
`Next Message Request(10)` recipients at the inclusive timestamp-7 frontier;
each `Reflect Attribute Values` callback precedes its corresponding grant and
preserves the source RegionHandle, tag, timestamp, order, and valid retraction
metadata. This remains development-profile evidence only; it does not promote
Lab validation or conformance.

### 2026-08-30 local slice — recipient-gated regional timestamped attribute fanout

No Requirements Lab resynchronization or new numbered observation was needed.
The mixed-fanout companion is now source-backed and green at 114 Catch2
assertions under `HLA_EVOKED`. It delivers one timestamped regional update to
an immediate recipient and queues the overlap-qualified constrained copy, then
proves the immediate reflection and Request Retraction remain recipient-local
while the constrained recipient receives its later reflection before its grant.
The callback metadata retains the invocation-time source-region set. This
remains development-profile evidence only; it does not promote Lab validation
or conformance.

### 2026-08-30 local slice — timestamped attribute available advances

No Requirements Lab resynchronization or new numbered observation was needed.
The focused nonregional timestamped Update Attribute Values available-advance
companion is now source-backed and green at 57 Catch2 assertions under
`HLA_EVOKED`. It exercises TARA at the GALT boundary and NMRA at the
next-message boundary, proving reflection-before-grant ordering and preserving
source, time, order, retraction, and tag metadata for both callbacks. This
remains development-profile evidence only; it does not promote Lab validation
or conformance.

### 2026-08-30 local slice — timestamped attribute Flush Queue passels

No Requirements Lab resynchronization or new numbered observation was needed.
The focused nonregional timestamped Update Attribute Values Flush Queue
companion is now source-backed and green at 63 Catch2 assertions under
`HLA_EVOKED`. It proves two queued passels are delivered before Flush Queue
Grant, computes actual grant 5 from the current GALT, retains optimistic floor
7, and admits a later grant at 7 after the regulator advances. This remains
development-profile evidence only; it does not promote Lab validation or
conformance.

### 2026-08-30 local slice — Flush Queue future input

No Requirements Lab resynchronization or new numbered observation was needed.
The indexed `flush-queue-future-input` companion is now source-backed and green
at 48 Catch2 assertions under `HLA_EVOKED`. It accepts Flush Queue Request
before the producer submits two timestamped interactions, then proves both
future-input callbacks precede Flush Queue Grant, computes actual grant 5 from
the current GALT, retains optimistic floor 9, and preserves interaction
payload, tag, order, producer, and retraction metadata. This remains
development-profile evidence only; it does not promote Lab validation or
conformance.

### 2026-08-30 local slice — terminal timestamped-deletion tombstone

No Requirements Lab resynchronization or new numbered observation was needed.
The indexed `timestamped-deletion-tombstone` case is now source-backed and green
at 24 Catch2 assertions under `HLA_EVOKED`. It drives a timestamped deletion to
the strict Retract eligibility boundary, verifies the designator is classified
as `MessageCanNoLongerBeRetracted`, and confirms that terminal deletion releases
the retained object/name state so the same reserved name can be registered
again. This remains development-profile evidence only; it does not promote Lab
validation or conformance.

### 2026-08-30 local slice — timestamped Delete Object Instance TAR/NMR

No Requirements Lab resynchronization or new numbered observation was needed.
The indexed `timestamped-delete-object-instance-tar-nmr` case is now
source-backed and green at 75 Catch2 assertions under `HLA_EVOKED`. It queues a
time-7 timestamped removal, drives one constrained recipient through TAR(7) and
another through NMR(10), proves each removal precedes its own grant, preserves
the official object/tag/producer/time/order/retraction metadata, and completes
the producer's TAR(2). This remains development-profile evidence only; it does
not promote Lab validation or conformance.

### 2026-08-30 local slice — public durable-save regional provider response

No Requirements Lab resynchronization or new numbered observation was needed.
The indexed public durable-save regional provider-response/retraction case is
now source-backed and green at 100 Catch2 assertions under both
`HLA_EVOKED`/`HLA_IMMEDIATE`. It saves a timestamped regional Request Attribute
Value Update response before constrained delivery, restores it into a fresh
registry, and verifies the response value/tag/source-region/order/time and
retraction identity at the matching grant. This remains development-profile
evidence only; it does not promote Lab validation or conformance.

All 773 indexed Catch2 plan entries now have explicit status; retained
historical/source-drift rows may still lack derived source locations. The next
bounded work is transport/conformance evidence, beginning with the indexed connection-
loss baseline. Query `python tools/query_rti_work.py next --summary --compact`
and keep multi-process, package/JUnit, protected-review, interoperability, and
conformance gates separate from the development-profile behavior ledger.

The indexed `[transport]` baseline was then executed directly: 32 focused Catch2
cases and 1,375 assertions passed. This validates the current embedded transport
behavior only; it is not multi-process interoperability or protected-review
conformance evidence.

### 2026-08-30 queryability guard — lane-scoped mapping check

No Requirements Lab resynchronization or new numbered observation was needed.
The whole-plan mapping check still reports retained historical/source-drift
rows, so it is intentionally a reconciliation gate rather than the iteration
gate. `query_rti_work.py check --lane <exact-tag> --compact` now validates the
roadmap structure, 2025 requirement references, standard subsection handles,
and the selected lane's test/source and completion-ledger records without
blocking on unrelated unlocated history. It lists any unlocated rows in that
lane as visible source drift; the unscoped `check` remains strict. This keeps
the active transport/conformance slice queryable without another broad Lab
search.

### 2026-08-30 local slice — transport late-cutoff directed interaction

No Requirements Lab resynchronization or new numbered observation was needed.
The indexed connection-lost-tso-cutoff directed-interaction companion is now
source-backed and green at 72 Catch2 assertions under HLA_EVOKED. It proves
that a late cutoff timestamped directed interaction reaches the constrained
survivor before its time-6 grant, retains the official directed callback
metadata and retraction identity, and leaves automatic DELETE_OBJECTS cleanup
to release at the later receive-order boundary. This remains
development-profile evidence only; it does not claim directed DDM, remote
transport, package/JUnit, protected-review, interoperability, or conformance.

### 2026-08-30 architecture slice — process transport handshake and data path

No Requirements Lab resynchronization or new numbered observation was needed.
The private process transport now opens an independently bound loopback listener,
performs the versioned hello/hello-ack identity exchange, and sends and receives
bounded data frames over the resulting socket connection. Its focused Catch2
case is source-backed and green at 12 assertions under the
`unit`/`foundation`/`transport`/`process-boundary` lane. This proves only the
transport endpoint and framing seam; federation service dispatch, remote
membership, installable-package behavior, JUnit/protected-review evidence,
interoperability, Lab validation, and conformance remain open.

### 2026-08-30 architecture slice — process-boundary framing

No Requirements Lab resynchronization or new numbered observation was needed.
The private transport protocol now has a versioned 12-byte envelope, bounded
payload length, explicit hello/hello-ack identity frames, and deterministic
malformed-frame rejection. Its Catch2 case is source-backed and green at 21
assertions under the foundation/transport lane. This is protocol-foundation
evidence only; it does not claim remote federation membership, service RPC,
package/JUnit, protected review, interoperability, or conformance.

### 2026-08-30 architecture slice — private transport contract

No Requirements Lab resynchronization or new numbered observation was needed.
The runtime now stores the private TransportConnection contract rather than the
concrete embedded endpoint; the embedded implementation remains the only
development-profile backend. This is an internal substitution seam for the
future process-boundary implementation and does not change the public IEEE
binding or promote embedded results to conformance.

The focused [transport] lane was rerun after these three restorations: 35
cases and 1,594 assertions passed. This is still embedded development-profile
evidence; the separate multi-process, package/JUnit, protected-review, and
interoperability gates remain open.

### 2026-08-30 local slice — suppressed cutoff directed interaction

No Requirements Lab resynchronization or new numbered observation was needed.
The adjacent indexed connection-loss directed-interaction suppression case is
now source-backed and green at 46 Catch2 assertions under HLA_EVOKED. It
withdraws the directed subscription at the callback boundary, proves the
timestamped payload is suppressed without a false delivery, preserves the
time-6 grant, and releases automatic DELETE_OBJECTS cleanup at the later
receive-order gate. This remains development-profile evidence only; it does
not claim directed DDM, remote transport, package/JUnit, protected-review,
interoperability, or conformance.

### 2026-08-30 queryability guard — source-pointer refresh

No Requirements Lab resynchronization or new numbered observation was needed.
Adding source-backed TEST_CASE blocks shifts later physical line numbers, so
completion-ledger source locations must be refreshed from the exact test title
after a localized insertion. The refresh found no semantic mapping changes;
the three ledger rows whose titles are still absent remain historical/source
drift and are visible through the strict global check.

### 2026-08-30 local slice — multi-recipient cutoff attribute update

No Requirements Lab resynchronization or new numbered observation was needed.
The indexed multi-recipient connection-loss attribute-update case is now
source-backed and green at 101 Catch2 assertions under HLA_EVOKED. It proves
each survivor independently receives the accepted cutoff timestamped
reflection before its own grant, preserves the official value/tag/producer/
order/retraction metadata, and releases each automatic DELETE_OBJECTS
reservation only when that survivor reaches its receive-order boundary. This
remains development-profile evidence only; it does not claim remote
transport, package/JUnit, protected-review, interoperability, or conformance.

### 2026-08-30 architecture slice — private service dispatch

No Requirements Lab resynchronization or new numbered observation was needed.
The process-boundary seam now includes a versioned private service envelope and
a synchronous request/response dispatcher. A focused Catch2 case exchanges
Create Federation Execution, Join Federation Execution, and Send Interaction
operation identities over the real socket endpoint and verifies correlation,
status, and opaque payload preservation in 22 assertions. This is still private
foundation evidence: the operation payloads are not yet bound to
`EmbeddedFederationRegistry`, and no public service semantics, installable
package, JUnit/protected review, interoperability, Lab validation, or
conformance claim is made.

### 2026-08-30 architecture slice — registry-bound process service

No Requirements Lab resynchronization or new numbered observation was needed.
The private process service now binds its Create Federation Execution and Join
Federation Execution operations to a supplied, already validated federation
definition and binds receive-order Send/Receive Interaction to the
`EmbeddedFederationRegistry` publication/subscription planner. Two real socket
sessions exchange joined identities and an opaque interaction payload; the
focused Catch2 case is source-backed and green at 22 assertions. The receiver
queue is an internal transport projection of the registry callback route, not
the final public `FederateAmbassador` callback bridge. Independently launched
process evidence, installable-package behavior, JUnit/protected review,
interoperability, Lab validation, and conformance remain open.

### 2026-08-30 architecture slice — independently launched registry-bound service

No Requirements Lab resynchronization or new numbered observation was needed.
The indexed process-boundary case now launches separate server, sender, and
receiver executables and verifies Create Federation Execution, Join Federation
Execution, and receive-order Send/Receive Interaction over the registry-bound
framed service. The source-backed Catch2 coordinator is green at 8 assertions,
and the focused `[transport]` lane is green at 40 cases / 1,679 assertions.
This is still private foundation evidence: the helper uses an internal polling
projection rather than the official `FederateAmbassador` callback bridge, and
does not claim installable-package behavior, JUnit/protected-review evidence,
interoperability, Lab validation, or conformance. The next bounded slice is the
callback bridge and reproducible installable-profile evidence; the Requirements
Lab remains unchanged.

### 2026-08-30 architecture slice — process callback bridge

No Requirements Lab resynchronization or new numbered observation was needed.
The independently launched process probe now enables the private pushed-event
service mode: the sender's receive-order interaction is encoded as an event
frame for the receiver instead of requiring a second polling request. The
receiver decodes that event and delivers it through
`ProcessFederationCallbackBridge` into the official IEEE 1516.1-2025 C++
`FederateAmbassador::receiveInteraction` surface, retaining the shared
immediate/evoked `CallbackDispatcher` and `CallbackSession` lifetime seam.
The `[process-boundary]` Catch2 coordinator remains green at 5 cases / 85
assertions, and the exact independent-process case remains green at 8
coordinator assertions. This is still private foundation evidence: the event
codec carries an opaque payload as the official user-supplied tag and empty
per-parameter values, and the process helper is not yet the installable public
RTI endpoint. The next bounded slice is public endpoint binding plus
reproducible CTest/JUnit/configuration artifacts; the Requirements Lab remains
unchanged.

### 2026-08-30 architecture slice — private process client seam

No Requirements Lab resynchronization or new numbered observation was needed.
The independently launched probe now routes its sender and receiver through
`ProcessFederationClient`, which owns request identities, response validation,
unsolicited pushed-event buffering, and the handoff into the official callback
bridge. This removes probe-specific request plumbing without promoting the
private service payloads to a public API. The process-boundary lane remains
green at 5 cases / 85 assertions and the independent case at 8 assertions;
the public/installable endpoint, complete parameter-value encoding, generated
JUnit/configuration artifacts, interoperability, and conformance remain open.

### 2026-08-30 architecture slice — public process endpoint lifecycle

No Requirements Lab resynchronization or new numbered observation was needed.
The official `RtiConfiguration::rtiAddress` now accepts the narrow
`tcp://host:port` process form and reports `addressUsed` after a real socket
handshake. The public C++ `RTIambassador` route covers Connect, Create
Federation Execution, Join Federation Execution, NoAction Resign, and clean
Disconnect through the private `ProcessFederationClient`; malformed endpoint
syntax is rejected before any socket attempt. Three source-backed Catch2
cases add 17 assertions, bringing the focused `[process-boundary]` lane to
8 cases / 102 assertions and `[transport]` to 43 cases / 1,696 assertions.
The server still supplies a prevalidated definition and the process wire
payloads remain private; message-service parity, installable packaging,
JUnit/protected review, interoperability, Lab validation, and conformance are
still open. The Requirements Lab remains unchanged.

### 2026-08-30 architecture slice — public process Send Interaction

No Requirements Lab resynchronization or new numbered observation was needed.
The public `RTIambassador::sendInteraction` route now crosses the configured
`tcp://host:port` endpoint through the private `ProcessFederationClient`. A
versioned private envelope preserves one official parameter handle/value pair
and the user-supplied tag for a second process participant; the receiver
projects that payload through the official C++ interaction callback shape. The
focused Catch2 case is source-backed and green at 14 assertions, bringing the
`[process-boundary]` lane to 9 cases / 116 assertions and `[transport]` to 44
cases / 1,710 assertions. This remains private foundation evidence: public
handle lookup, declaration, receive/evoke, timestamped/region/directed
variants, installable packaging, JUnit/protected review, interoperability, Lab
validation, and conformance remain open.

### 2026-08-30 queryability guard — bounded active-slice pointers

The roadmap index, Catch2 plan, and derived C++ source locations now expose the
public process message slice through one exact plan ID, test title, lane tag,
three 2025 requirement IDs, and two canonical 2025 subsection handles. The
`work`, `test`, `lane`, `requirement`, `section`, `recent`, and lane-scoped
`check` commands are the supported lookup path; they avoid a repository-wide or
Requirements-Lab resynchronization. Counts in the short roadmap references are
refreshed from the focused executable (44 transport cases / 1,710 assertions;
9 process-boundary cases / 116 assertions).
This paragraph is a historical snapshot; current live counts are maintained in
`docs/planning/ROADMAP-INDEX.json` and the query-guide tail entries below.

### 2026-08-30 architecture slice — public process handle lookup

No Requirements Lab resynchronization or new numbered observation was needed.
The public `RTIambassador::getInteractionClassHandle` and
`RTIambassador::getParameterHandle` routes now cross the configured
`tcp://host:port` endpoint through the private `ProcessFederationClient` and
resolve against the server-owned composed FOM. The parameter request carries
the official interaction-class handle, so this slice does not invent a public
reverse-lookup API. The focused Catch2 case is source-backed and green at 8
assertions, bringing the `[process-boundary]` lane to 10 cases / 124 assertions
and `[transport]` to 45 cases / 1,718 assertions. Its plan row maps the exact
2025 support-service requirements for Get Interaction Class Handle
(§10.13.2) and Get Parameter Handle (§10.16.1). Reverse lookup,
declaration, receive/evoke, timestamped/region/directed variants, installable
packaging, JUnit/protected review, interoperability, Lab validation, and
conformance remain open. The Requirements Lab remains unchanged.

### 2026-08-30 architecture slice — public process interaction declaration

No Requirements Lab resynchronization or new numbered observation was needed.
The public `RTIambassador` Publish, active/passive Subscribe, Unsubscribe, and
Unpublish interaction-class routes now cross the configured `tcp://host:port`
endpoint through the private `ProcessFederationClient` and mutate the
server-owned declaration registry. The focused Catch2 case is source-backed
and green at 12 assertions, bringing the `[process-boundary]` lane to 11 cases
/ 136 assertions and `[transport]` to 46 cases / 1,730 assertions. Its plan
row maps the exact 2025 declaration-management requirements for Publish,
Unpublish, Subscribe, and Unsubscribe (§5.4, §5.5, §5.10, and §5.11).
Advisories/callbacks, receive/evoke, timestamped/region/directed variants,
installable packaging, JUnit/protected review, interoperability, Lab
validation, and conformance remain open. The Requirements Lab remains
unchanged.

### 2026-08-30 architecture slice — public process Receive/Evoke baseline

No Requirements Lab resynchronization or new numbered observation was needed.
The public process callback bridge now carries two receive-order interactions
from the configured endpoint into the official C++
`FederateAmbassador::receiveInteraction` surface: one through `Evoke Callback`
and one through `Evoke Multiple Callbacks`. The source-backed case is green at
32 assertions; after the timestamped companion was added, the focused
`[process-boundary]` lane is 13 cases / 196 assertions. EVOKED callback
enable/disable gating is covered by the same process case; immediate and
approximate-wait behavior plus the installable/reproducible evidence gate
remain open. The Requirements Lab remains unchanged.

### 2026-08-30 architecture slice — public process timestamped Receive/Evoke

No Requirements Lab resynchronization or new numbered observation was needed.
The public timestamped `RTIambassador::sendInteraction` overload now carries
the caller's official logical-time implementation name and encoded
`LogicalTime` through the private process envelope. The server validates the
implementation against the federation definition, and the callback bridge
delivers the official timestamped `FederateAmbassador::receiveInteraction`
overload through `Evoke Callback`. The exact mapped Catch2 case is green at 28
assertions (source `cpp/tests/ieee1516_2025_connection_catch2.cpp:1163`) and
maps 11 Lab requirement IDs to five canonical 2025 sections. The focused
process-boundary lane is 13 cases / 196 assertions with no source drift; the
broader transport query reports 53 plan entries (49 mapped, 48 source-located,
and five historical source-drift rows). This is still private foundation
evidence: no time regulation, queueing, time advances, or retraction service is
invented at the process boundary; packaging, JUnit/protected review,
interoperability, Lab validation, and conformance remain open. The Requirements
Lab remains unchanged.

### 2026-08-30 queryability maintenance — source-pointer drift caught locally

Adding the timestamped process case shifted five earlier `TEST_CASE` line
numbers. The lane-scoped mapping check caught the stale pointers before they
could become silent navigation errors; the completion ledger now points to the
derived declarations at lines 207, 312, 548, 675, 834, and 1163. This is a
local index-maintenance issue, not a Requirements Lab recurrence, so it does
not consume a new `RL-###` identifier. Future changes should run
`python tools/query_rti_work.py check --lane process-boundary --summary
--compact` immediately after inserting or moving a C++ test.

### 2026-08-30 architecture slice — public process Evoke Multiple and callback gating

No Requirements Lab resynchronization or new numbered observation was needed.
The public process receive path now drains all currently available polling
events before `Evoke Multiple Callbacks` enters the shared callback dispatcher.
The mapped C++ case keeps the first receive-order callback on `Evoke Callback`,
disables EVOKED callbacks before the second send, verifies that
`Evoke Multiple Callbacks` reports pending work without invoking user code,
then re-enables callbacks and drains the retained callback. It is green at 32
assertions and maps the existing process interaction requirements plus the
2025 callback clauses `10.58` and `10.60.6`; it remains private foundation
evidence rather than conformance evidence. The focused process lane is now 13
cases / 196 assertions. The next bounded work is the installable process
profile and reproducible CTest/Catch2/JUnit/configuration gate. The Requirements
Lab remains unchanged.

### 2026-08-30 installable-profile gate — package smoke made self-contained

No Requirements Lab resynchronization or new numbered observation was needed.
The installable package smoke now builds every exported static runtime target
before staging, so it no longer depends on a prior full-build target order. The
install rules include the public `umbra/embedded_profile_configuration.hpp`
helper and a generated `share/umbra_rti/umbra_rti-profile.json` manifest. The
smoke validates the manifest's IEEE 1516.1-2025 provider/profile, filesystem
service-report model, process-address contract, and the reviewed 1516.2
resource digests before compiling a clean `find_package(umbra_rti)` consumer.
The installed consumer also checks the official `RtiConfiguration` address and
typed filesystem-directory construction without exposing a backend choice.
`umbra_test_installable_package` and the `installable-package` CTest label now
provide the bounded entry point; the installable smoke passes in the
`windows-fom-services` profile. The focused
`umbra_process_boundary_junit` target now emits the same 13-case process lane
as a bounded JUnit artifact under the configured compliance directory. This is
package/evidence plumbing only: the public
two-process interoperability run, protected review, Lab validation, and
conformance promotion remain open. The Requirements Lab remains unchanged.

### 2026-08-30 installable-profile gate — public two-client process smoke

The embedded-profile package smoke now launches the source-tree process
fixture only as a private server and exercises the installed package from a
clean downstream executable. Two independent official C++ RTIambassadors use
`RtiConfiguration::rtiAddress` to Connect/Create/Join, resolve the server-owned
Restaurant interaction class, Send a receive-order interaction, drain the
receiver through `Evoke Multiple Callbacks`, and complete NoAction Resign.
The `package-process` CTest label is separate from the 13-case
`process-boundary` Catch2 lane, and the probe is never installed or presented
as a public backend. The focused package target is green; timestamped public
interoperability now has its own `package-process-timestamped` CTest case, while
connection-loss recovery, protected review, and conformance promotion remain
open. No Requirements Lab resynchronization or new numbered observation was
needed.

### 2026-08-30 installable-profile gate — timestamped public two-client smoke

No Requirements Lab resynchronization or new numbered observation was needed.
The installed public package consumer now runs a separate timestamped mode
against the same private server fixture. It constructs an official
`HLAinteger64Time`, sends through the public timestamped `Send Interaction`
overload, and verifies the receiver's timestamped `FederateAmbassador` callback
preserves the implementation name and encoded time, uses RECEIVE order, and
does not fabricate a retraction handle. The ordinary and timestamped process
tests remain separately queryable by CTest name and label; the result is still
foundation evidence until connection-loss recovery and protected review are
complete.

### 2026-08-30 installable-profile gate — connection-loss recovery and bounded handles

No Requirements Lab resynchronization or new numbered observation was needed.
The installed public process consumer now has a third explicit mode,
`umbra_rti_package_process_connection_loss_consumer`, under the
`package-process-connection-loss` label. A private server fixture closes the
receiver transport and removes its remote membership; the installed receiver
observes the transport failure through `EvokeMultipleCallbacks`, receives the
official `connectionLost` callback with a non-empty fault description, and the
surviving sender continues with handle lookup and Send Interaction. The
receiver is not resigned a second time after the loss, so the test exercises
the actual lifecycle transition rather than hiding it behind cleanup.

The query index now exposes the exact loss test, CTest label, target, and its
baseline Catch2 mapping. The baseline maps five existing Lab requirements to
`hla-1516.1-2025:clause-4.1.1`, `clause-4.4`, `clause-4`, `clause-10.44`, and
`clause-10.45.3`. The package target, the loss-only CTest case, and the
ordinary/timestamped/loss label set all pass; this remains installable-profile
foundation evidence, not protected review, Lab validation, or conformance.

### 2026-08-30 installable-profile gate — parameterized interaction envelope

No Requirements Lab resynchronization or new numbered observation was needed.
The installed public process consumer now has a separate
`umbra_rti_package_process_parameterized_consumer` mode under the
`package-process-parameterized` label. It resolves the Restaurant FOM's
`HLAobjectRoot.Customer` class and `TimelinessOk` parameter through the public
process endpoint, sends a non-empty value and user tag, and verifies the
receiver callback preserves the official object/parameter handles, value
bytes, interaction handle, producer handle, and tag. The private probe only
supplies the process service; the client side uses the installed 2025 C++
headers and runtime. This is a focused extension of the existing mapped
process interaction envelope, not a Requirements-Lab recurrence or a new
requirement number. The corresponding Catch2 support-lookup mapping now
includes the 2025 object-class lookup requirement at clause 10.4.6. The
parameterized package lane passes alongside the ordinary, timestamped, and
connection-loss lanes and remains installable-profile foundation evidence
pending protected review and conformance promotion.

### 2026-08-30 queryability maintenance — service-lane catalog audit

The executable CTest service-lane audit initially found ten labels that had
Requirements-Lab/API traceability checks but no tagged Catch2 behavior case:
object-instance provider response, two Connection Lost selector-mutation
labels, delete failure, ordinary and timestamped regional interaction report/
failure labels, and timestamped delete report/failure labels. These were lane
registration mismatches, not Requirements-Lab changes or new requirement
recurrences. They are now retained as narrow traceability-only CTest lanes and
are excluded from the complete-service-lane catalog until each has a real
behavior case. The audit passes for every remaining advertised service lane;
future behavior additions should switch the corresponding registration back to
`umbra_add_ctest_service_lane` only when a tagged Catch2 case exists.

### 2026-08-30 queryability maintenance — process object-registration slice

No Requirements Lab resynchronization or new numbered observation was needed.
The process-boundary catalog now includes the exact C++ case
`RTIambassador publishes object-class attributes and registers an object through
a configured process endpoint`, mapped to the four 2025 requirement IDs and
the canonical `clause-5.1.2`, `clause-5.2`, and `clause-6.8.4` subsection keys.
The installed package has a matching
`umbra_rti_package_process_object_registration_consumer` test under the
`package-process-object-registration` label. It uses the official public
object/attribute lookup, publication, and unnamed registration calls and
resigns with `DELETE_OBJECTS` while a second federate is still joined.

The initial package draft used `NO_ACTION`; the registry correctly rejected
that action because the sender still owned the registered object's published
attributes, and the smoke cleanup then waited on a second unserved request.
This was a fixture/lifecycle sequencing defect, not a Requirements Lab issue;
the explicit delete-on-resign action and bounded package lane now pass. Keep
the rejection semantics visible when adding future multi-federate object
registration tests rather than weakening cleanup or silently retrying a failed
request.

### 2026-08-31 queryability/protected-slice maintenance — ordinary attribute update package lane

No Requirements Lab resynchronization or new numbered requirement was needed.
The installed-profile process package now has a separate
`umbra_rti_package_process_attribute_update_consumer` test under the
`package-process-attribute-update` label. It uses only the installed 2025 C++
`RTIambassador::subscribeObjectClassAttributes`,
`RTIambassador::updateAttributeValues`, and official
`FederateAmbassador::reflectAttributeValues` callback surfaces; the private
source-tree fixture now supplies only the receiver's discovery state until
that callback event receives its own process protocol projection. The callback preserves the object handle, one attribute value,
user tag, producer identity, and a valid transportation handle.

The eight installed-profile process labels (ordinary, timestamped,
parameterized-envelope, connection-loss, object-registration, named-registration,
ordinary attribute-update/Reflect, and directed-retraction) pass as a bounded
CTest set. The exact test/label
handles are recorded in `ROADMAP-INDEX.json` and emitted by
`python tools/query_rti_work.py next --summary --compact`; this is package
foundation evidence only, not Lab validation or conformance promotion. A
single-callback Evoke is used for this fixture because the server intentionally
advances to the resign phase after one receive poll; no runtime behavior is
weakened and no Requirements Lab numbering changed.

### 2026-08-31 queryability/protected-slice maintenance — package lane catalog guard

No Requirements Lab resynchronization or new numbered requirement was needed.
The clean downstream package consumer now runs
`tools/verify_process_package_lanes.py` after configuration and before the
process smokes. The guard reads the generated CTest JSON catalog and compares
the eight indexed `next_process_package_*` test/label pairs in
`ROADMAP-INDEX.json`; missing, renamed, duplicated, or unindexed package lanes
fail the installable-package gate before execution. This is a local
traceability/protection improvement, not a change to the Lab numbering or a
conformance claim.

### 2026-08-31 process declaration slice — ordinary object-class subscription

The process service now carries ordinary `Subscribe Object Class Attributes`
declaration state through a dedicated private operation, including normalized
attribute handles and the FDD update-rate designator. The installed
attribute-update consumer invokes the official public subscription API before
the private fixture establishes discovery, so declaration transport and
callback discovery remain separately visible. The process-boundary Catch2
case retains the 2025 §5.8 requirement references alongside the existing
§6.10/§6.11.1 update/reflection references. No Requirements Lab resync or new
Lab numbering was needed for this implementation slice.

### 2026-09-01 queryability maintenance — directed-retraction package lane

No Requirements Lab resynchronization or new numbered requirement was needed.
The installed-profile package now has a dedicated
`umbra_rti_package_process_directed_retraction_consumer` test under
`package-process-directed-retraction`. It exercises the public installed C++
surface for a nested directed object/interaction pair, timestamped send,
valid retraction handle, and the official directed callback. It proves both
directions of the retraction boundary: a first message crosses Evoke before
`Retract` and produces exactly one matching `requestRetraction` callback, while
a second message is retracted before Evoke and produces neither a directed
callback nor a second retraction callback. The private process probe supplies
only the fixture and protocol endpoint; it is not presented as a second public
API.

The exact package test and label are indexed beside the process-boundary
baseline and are checked by `tools/verify_process_package_lanes.py`. The full
`installable-package` gate passed with all eight package lanes, so this slice
is independently runnable evidence rather than a prose-only roadmap item.

### 2026-09-01 queryability maintenance — source-pointer reconciliation

The strict unscoped query check exposed stale line numbers in the append-only
`recent_completed_slices` ledger after the large federation-management Catch2
translation unit moved. Forty-one source locations were mechanically
reconciled to the current `TEST_CASE` declarations from the existing plan;
this did not rescan or modify the Requirements Lab. The remaining global
diagnostic is limited to the known source-unlocated historical rows and their
corresponding recent-ledger entries. Lane-scoped checks remain the iteration
gate and now report zero pointer drift for the active process-boundary lane.

### 2026-09-01 queryability maintenance — public Request Retraction callback

No Requirements Lab resynchronization or new numbered requirement was needed.
The existing `m16.transition.process-directed-interaction-callback` mapping now
indexes the public directed-interaction case's official
`FederateAmbassador::requestRetraction` surface, the
`federate.callback.request-retraction` lane tag, and the
`timestamped-retraction-after-receive` delivery mode. The focused case records
387 Catch2 assertions at its exact `TEST_CASE` source line and covers both
HLA_EVOKED and HLA_IMMEDIATE: Retract-before-receive suppresses the pending
delivery, while Retract-after-receive produces exactly one legal callback with
the original handle. This is private process-foundation evidence; it does not
promote the process profile to time-management conformance or replace the
installable-package negative-retraction consumer.

### 2026-09-01 queryability maintenance — push-mode fixture ordering edge

The first installable-package projection exposed a fixture-ordering hazard,
not a Requirements Lab defect: subscribing to object attributes queues the
target-discovery callback before directed traffic. In push receive-order mode,
an early receiver `Evoke` can therefore consume discovery rather than the
directed interaction/retraction event. The package lane now explicitly drains
and validates discovery before sending the directed pair, and the private
probe places receiver polls only at the public receive fences. This keeps the
positive post-delivery and negative pre-delivery assertions deterministic,
without adding a duplicate requirement number or triggering a Lab
resynchronization.

### 2026-09-01 queryability maintenance — Annex C.2 plan reconciliation

An existing private FOM-composition Catch2 case was present in the source and
in the Lab contract, but was absent from the checked-in Catch2 plan. The case
`The FOM composition preflight permits equivalent duplicates and rejects a real
class conflict` is now indexed as
`umbra-cpp-fom-annex-c-class-conflict-composition`, with its exact source line,
Lab requirement `requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-109-l46-6`,
and canonical `hla-1516.2-2025:clause-C.2` mapping. This was an append-only
traceability repair; no runtime behavior changed, no Lab number was added, and
the focused `annex-c` check is green.

### 2026-09-01 queryability maintenance — bounded roadmap/test selection

Repeated full-catalog searches were a local workflow failure: the roadmap's
overlapping tags made it too easy to re-open the entire Catch2 plan instead of
selecting one implementation lane. This is not a Requirements Lab numbering or
corpus defect, so no Lab resynchronization was performed. The read-only
`tools/query_rti_work.py queue --summary --compact` view now emits one bounded
row per open roadmap family, an explicit state (`ready`, `complete-pointer`,
`source-drift-only`, or `new-case-needed`), exact `work`/`focus` handles, and a
short next action. `work` and `next` also report lane state, while `trace`,
`requirement`, and `section` preserve the direct C++ test → Lab requirement →
canonical IEEE 1516.1/1516.2-2025 subsection path. Family totals may overlap;
`coverage` remains the global count. This local index is the intended resume
surface and should be used before considering any broader catalog search.

### 2026-09-01 queryability maintenance — bounded FOM declaration-management slice

The next implementation slice was selected from the indexed roadmap without
resynchronizing the unchanged Requirements Lab. IEEE 1516.1-2025 §4.5.5
requires at least one FOM module for Create Federation Execution. The focused
C++ case `Embedded Create Federation Execution rejects an empty FOM module set`
now maps directly to
`requirement-candidate-content-clauses-04-federation-management-page-053-l55-9`,
the canonical section `hla-1516.1-2025:clause-4.5.5`, and the official Create
Federation Execution/Create Federation Execution With MIM API surfaces. It
passes six assertions for both rejected overloads and verifies that no partial
federation remains.

The case is built and run through the standalone target
`umbra_fom_declaration_management_catch2`, so the exact lane remains runnable
while the pre-existing aggregate federation-management test translation unit
is malformed near `makeRtn)` (around line 62816). That aggregate compile issue
is recorded as an existing checkout problem, not attributed to this slice and
not treated as conformance evidence. Query the lane with
`python tools/query_rti_work.py focus fom-module-management --summary --compact`
or trace the exact case; broader FOM composition, package/JUnit, protected
review, interoperability, Lab validation, and conformance remain open.

### 2026-09-01 queryability maintenance — additional-FOM Join composition slice

The next source-backed FOM slice extends the focused lane without reopening the
Requirements Lab. `Embedded Join Federation Execution composes an additional
FOM module for all members` exercises the official Join Federation Execution
overload with an additional validated module. It verifies that the new object
class becomes visible to the joining federate and an existing member, that the
pre-existing class handle remains stable, and that the official reverse lookup
returns the new class name. The plan maps the case to the 2025 clause-4
requirements for FDD combination on Join and availability of supplied modules,
with direct C++ API-surface and source-line pointers.

The standalone `umbra_fom_declaration_management_catch2` target now runs two
focused cases (24 assertions) and the exact CTest label
`fom-module-management` is green. The malformed aggregate federation-
management translation unit remains a separate checkout issue at `makeRtn)`
around line 62816; this focused evidence does not promote package, protected
review, interoperability, Lab validation, or conformance status.

### 2026-09-02 queryability maintenance — lane-pointer ownership and aggregate source integrity

No Requirements Lab resynchronization or new numbered requirement was needed.
The exact trace view now includes the owning roadmap families beside each
Catch2 case, its selected Lab requirements, and canonical 2025 subsection
keys. Lane-scoped `check` also counts a roadmap family when the lane is carried
by its `next_lane` pointer rather than duplicated in broad `query_tags`; this
keeps the FOM and process-boundary work cards truthful without widening their
test selection.

While rebuilding the aggregate federation-management translation unit, the
original malformed `makeRtn)` region was followed by additional incomplete
multi-recipient and timed DDM helper material. The affected legacy cases remain
visible in the plan but are not treated as executable evidence until their
helper/source boundaries are restored; the focused FOM lane remains the
iteration gate. This is a checkout/source-integrity issue, not a Requirements
Lab numbering change, and it should not trigger another Lab resynchronization.

### 2026-09-02 queryability maintenance — Annex C directed-interaction guards

The existing source regressions `The FDD materializer surfaces the
multiple-directed-class schema conflict` and `The FDD materializer refuses the
supplied extension when the official FDD schema cannot represent its
directed-interaction merge` were present in
`cpp/tests/libxml2_fom_composer_catch2.cpp` and described by RL-081, but had no
row in the Catch2 plan. They are now indexed as
`umbra-cpp-fom-directed-interaction-schema-conflict-composition` (three
assertions) and
`umbra-cpp-fom-restaurant-extension-directed-interaction-schema-conflict`
(two assertions), both with the exact pinned requirement
`requirement-candidate-sections-semantic-clause-7-annexes-a-c-page-110-l22-5`,
and its canonical `hla-1516.2-2025:clause-7` mapping. The `schema-conflict`
lane is owned by `annex-c-and-reference-resolution`, so `focus`, `trace`, and
lane-scoped `check` reach either guard without reopening the full plan.

This is an append-only traceability repair. The expected rejection remains a
guard for the supplied DIF/FDD cardinality conflict; it is not runtime selector
coverage or a conformance claim, and no Requirements Lab resynchronization or
new numbered requirement was performed.

The two guards now also have an independently discoverable
`umbra_fom_composer_catch2` target and exact CTest regex, because the aggregate
`umbra_ieee1516_2025_catch2` executable includes an unrelated malformed
federation-management translation unit. Both focused guards pass (5 Catch2
assertions total); the source tags and plan tags are aligned so the lane can be
run by a bounded query rather than by an aggregate build.

The new read-only `query_rti_work.py unplanned --path <source-file>` view also
exposes source `TEST_CASE` declarations that have no exact Catch2 plan row. It
does not infer requirements or status; for the current FOM-composer file it
reports the remaining source-only declarations as a bounded reconciliation
queue. This closes the source-to-plan discovery gap without reopening the
Requirements Lab.

### 2026-09-02 queryability maintenance — composed data-type reference guard

The source-only case `The FOM composition preflight resolves data-type
references after the complete module set is merged` is now an explicit Catch2
plan row: `umbra-cpp-fom-composed-data-type-reference-resolution` (eight
assertions). Its two selected Lab candidates are the exact 2025 statements for
documenting referenced data types (`4.14.2`) and for requiring an array Element
Type name from a data-type table (`6.2.18`). The query trace resolves both
requirements to canonical `hla-1516.2-2025:clause-4.14.2` and
`hla-1516.2-2025:clause-6.2.18`, with the source line and the existing private
composition contract visible in the same bounded record.

The `reference-resolution` lane is now owned by
`annex-c-and-reference-resolution` and has an exact
`umbra_fom_composer_catch2` CTest handle covering its five mapped guards. The
roadmap pointer advanced to this data-type guard before the adjacent
reference-data-class guard became the next pointer; the source-only
reconciliation queue for `libxml2_fom_composer_catch2.cpp` consequently drops
from 23 to 22. This is a local plan/index repair only: no Requirements Lab
resynchronization or new numbered requirement was performed, and the guard
remains private
development-profile traceability rather than FOM conformance evidence.

The adjacent source-only case `The FOM composition preflight resolves
reference-data classes after the complete module set is merged` is now indexed
as `umbra-cpp-fom-reference-data-class-resolution` (eight assertions). Its
exact Lab candidates map to 2025 clauses 6.2.17 and 4.14.9, and the bounded
trace exposes both canonical subsection keys beside the source line. The
At the previous pointer the `reference-resolution` lane had five mapped cases;
its exact CTest handle remained independent of the malformed aggregate
translation unit and all five tests passed. The source-to-plan queue for the
FOM-composer file was then 21
remaining declarations. This is another local mapping/index repair only; no
Lab resynchronization or new numbered requirement was performed.

The next source-only reference-resolution case, `The FOM composition preflight
resolves reference-data attributes and representations`, is now indexed as
`umbra-cpp-fom-reference-data-attribute-resolution` (eleven assertions). Its
five selected Lab candidates resolve to 2025 clauses 6.2.17, 4.14.9, and 4.5.2;
the exact trace keeps the referenced attribute, representation, and inherited
attribute checks together. The lane now has six mapped cases, and its exact
CTest handle runs all six without touching the malformed aggregate. The
FOM-composer source-to-plan queue is now 20 remaining declarations. No Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Available Dimensions reference guard

The next bounded source-only case, `The FOM composition preflight resolves
available dimensions after the complete module set is merged`, is now indexed
as `umbra-cpp-fom-available-dimension-reference-resolution` (nine assertions).
Its four selected Lab candidates resolve to the exact 2025 statements for the
DDM Available Dimensions subset (`4.7.1`), object-class and interaction-class
table references (`6.2.5` and `6.2.6`), and top-down Annex C module merging
(`Clause C`). The seven-case `reference-resolution` lane has an exact
`umbra_fom_composer_catch2` handle; all seven tests pass in 4.58 seconds, and
the lane-scoped mapping check reports 7/7 mapped, 39 assertions, and 0 source
drift. The FOM-composer source-to-plan queue is now 19 remaining declarations.
No Requirements Lab resynchronization or new numbered requirement was
performed.

The aggregate federation-management target also now builds after restoring the
real `runTimedMultiRecipientRegionalResignationAfterRestore` helper from the
existing source snapshot; the exact 11-test timed multi-recipient DDM/save-
restore matrix passes. The restoration shifted later source line numbers, so
the affected recent-completion pointers in `ROADMAP-INDEX.json` were refreshed
from the derived `TEST_CASE` locations. Historical source-unlocated rows remain
visible in the unscoped check and are not treated as executable evidence.

### 2026-09-02 queryability maintenance — Transportation reference guard

The next source-only reference-resolution case, `The FOM composition preflight
resolves transportation names after the complete module set is merged`, is now
indexed as `umbra-cpp-fom-transportation-reference-resolution` (eleven
assertions). Its six selected Lab candidates resolve to 2025 clauses 4.11.2,
6.2.5, 6.2.6, and Annex C; the exact trace keeps object-attribute and
interaction-class transportation references together. The lane now has eight
mapped cases and its source-to-plan queue is 18 remaining declarations. No
Requirements Lab resynchronization or new numbered requirement was performed.

The roadmap index now carries a bounded `next_source_test_query` /
`next_source_location` pair for the next source-only declaration. `work
annex-c-and-reference-resolution --summary --compact` renders that queue head
beside the mapped baseline and emits a one-case `unplanned` command; the
mapping check verifies that the recorded title and source location still match
the derived C++ `TEST_CASE`. This keeps source-only selection queryable without
dumping the full reconciliation queue.

### 2026-09-02 queryability maintenance — Support-switch table guard

The next bounded source-only FOM-composer case, `The FDD materializer retains
the complete 2025 support-switch table`, is now indexed as
`umbra-cpp-fom-support-switch-table-composition-integration` (nine
assertions). Its four selected Lab candidates resolve to the 2025 support
switch names/settings and automatic-resign value in clauses 4.13.2, 4.13.3,
and 6.2.13. The exact CTest title passes independently in 0.37 seconds. The
plan total is now 818 rows; the derived source-only queue is 293 declarations
globally and 17 in `libxml2_fom_composer_catch2.cpp`. The next queue head is
the explicit NoAction automatic-resign setting at
`cpp/tests/libxml2_fom_composer_catch2.cpp:1230`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Explicit NoAction FOM guard

The source-queue head `The FDD catalog preserves an explicit NoAction
automatic-resign setting` is now indexed as
`umbra-cpp-fom-explicit-no-action-automatic-resign-composition` (one
assertion). The plan total is now 819 rows. Its two selected Lab candidates resolve to the FOM switch-setting
and automatic-resign table statements in clauses 4.13.3 and 6.2.13. The
bounded source-only queue is now 292 declarations globally and 16 in the
FOM-composer file. Ownership of the next exact source head is now explicit in
`fom-module-declaration-management`: `The federation registry retains
independent interaction declarations until removal or resign` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:1251`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Interaction declaration registry guard

The declaration-management source head `The federation registry retains
independent interaction declarations until removal or resign` is now indexed
as `umbra-cpp-federation-registry-interaction-declaration-lifecycle-unit` (34
assertions). Its four selected Lab candidates resolve to IEEE 1516.1 clauses
5.4, 5.5, 5.10, and 5.11 for Publish, Unpublish, Subscribe, and Unsubscribe
declaration state. The exact FOM-composer CTest passes in 0.44 seconds. The
plan total is now 820 rows; the source-only queue is 291 declarations
globally and 15 in the FOM-composer file. The next exact source head remains
owned by `fom-module-declaration-management`: `The federation registry
retains 2025 object class attribute declarations independently` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:1383`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Object-class attribute registry guard

The declaration-management source head `The federation registry retains 2025
object class attribute declarations independently` is now indexed as
`umbra-cpp-federation-registry-object-class-attribute-declaration-lifecycle-unit`
(44 assertions). Its four selected Lab candidates resolve to IEEE 1516.1
clauses 5.2, 5.3, 5.8, and 5.9.4 for Publish, Unpublish, Subscribe, and
Unsubscribe Object Class Attributes. The exact FOM-composer CTest passes in
0.43 seconds. The plan total is now 821 rows; the source-only queue is 290
declarations globally and 14 in the FOM-composer file. The next exact source
head remains owned by `fom-module-declaration-management`: `The federation
registry allocates 2025 object identities only from current publication` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:1869`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Object-instance registration guard

The declaration-management source head `The federation registry allocates 2025
object identities only from current publication` is now indexed as
`umbra-cpp-federation-registry-object-instance-registration-unit` (23
assertions). Its three selected Lab candidates resolve to IEEE 1516.1 clauses
5.1.2 and 6.8.4 for registration preconditions and object-instance identity
allocation. The exact FOM-composer CTest passes in 0.43 seconds. The plan
total is now 822 rows; the source-only queue is 289 declarations globally and
13 in the FOM-composer file. The next exact source head remains owned by
`fom-module-declaration-management`: `The federation registry releases
undelivered 2025 discovery reservations` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:1947`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Discovery reservation guard

The declaration-management source head `The federation registry releases
undelivered 2025 discovery reservations` is now indexed as
`umbra-cpp-federation-registry-object-instance-discovery-reservation-unit`
(15 assertions). Its four selected Lab candidates resolve to IEEE 1516.1
clauses 5.1.2, 6.8.4, and 6.9.3 for publication-gated registration and
recipient-local discovery promotion. The exact FOM-composer CTest passes in
0.43 seconds. The plan total is now 823 rows; the source-only queue is 288
declarations globally and 12 in the FOM-composer file. The next exact source
head remains owned by `fom-module-declaration-management`: `The federation
registry plans 2025 interaction promotion and parameter projection` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:2024`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Interaction promotion guard

The declaration-management source head `The federation registry plans 2025
interaction promotion and parameter projection` is now indexed as
`umbra-cpp-federation-registry-interaction-routing-promotion-unit` (42
assertions). Its four selected Lab candidates resolve to IEEE 1516.1 clauses
5.4, 5.10, 6.12, and 6.12.4 for interaction declaration, Send Interaction,
and parameter projection semantics. The exact FOM-composer CTest passes in
0.43 seconds. The plan total is now 824 rows; the source-only queue is 287
declarations globally and 11 in the FOM-composer file. The next exact source
head remains owned by `fom-module-declaration-management`: `The FDD materializer
is repeatable for a fixed official module set` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:2227`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — FDD materializer repeatability guard

The declaration-management source head `The FDD materializer is repeatable for
a fixed official module set` is now indexed as
`umbra-cpp-fom-materializer-repeatability-unit` (17 assertions). It composes
the fixed official 2025 MIM plus Restaurant module order twice and verifies the
same valid result, materialized FDD bytes, composed-module names, catalog
cardinality, and key object/interaction lookups. The selected existing Lab
candidates provide the closest FDD materialization/composition anchors in
IEEE 1516.2 clauses 4.14.2 and C.1; the repeatability comparison itself is an
explicit private implementation invariant and is not promoted to arbitrary
FOM canonicalization, reordered-module equivalence, schema conformance, or
interoperability. The exact FOM-composer CTest passes independently in 0.71
seconds. The plan total is now 825 rows; the source-only queue is 286
declarations globally and 10 in the FOM-composer file. The next exact source
head remains owned by `fom-module-declaration-management`: `The FDD catalog
retains an enabled Non-Regulated-Grant switch` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:2258`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Non-Regulated-Grant switch guard

The declaration-management source head `The FDD catalog retains an enabled
Non-Regulated-Grant switch` is now indexed as
`umbra-cpp-fom-non-regulated-grant-switch-composition-integration` (11
assertions). It composes the official 2025 MIM with an explicitly enabled
Non-Regulated-Grant FOM and verifies the valid materialization plus the
catalog's retained switch setting. Its selected Lab candidate resolves to
IEEE 1516.2 clause 4.13.3; the runtime TAR scheduler, omitted-setting default,
Annex C.8 duplicate handling, JUnit/protected review, and conformance remain
separate. The exact FOM-composer CTest passes independently in 0.39 seconds.
The plan total is now 826 rows; the source-only queue is 285 declarations
globally and 9 in the FOM-composer file. The next exact source head remains
owned by `fom-module-declaration-management`: `The FOM composition preflight
retains the first duplicate switch and reports Annex C.8 warnings` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:2274`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Annex C.8 duplicate-switch guard

The declaration-management source head `The FOM composition preflight retains
the first duplicate switch and reports Annex C.8 warnings` is now indexed as
`umbra-cpp-fom-switch-duplicate-annex-c8-composition` (35 assertions). It
compares equivalent and non-equivalent support/advisory switch duplicates
across module order, retains the first setting, applies the Disabled default to
omitted advisory entries, and emits the deterministic Annex C.8 warning. The
six selected Lab candidates resolve to IEEE 1516.2 clauses 4.13.3 and C.8;
runtime switch mutation, complete Annex C merge behavior, JUnit/protected
review, and conformance remain separate. The exact FOM-composer CTest passes
independently. The plan total is now 827 rows; the source-only queue is 284
declarations globally and 8 in the FOM-composer file. The next exact source
head remains owned by `fom-module-declaration-management`: `The FOM composition
treats omitted switch booleans as their schema default` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:2338`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Omitted-switch default guard

The declaration-management source head `The FOM composition treats omitted
switch booleans as their schema default` is now indexed as
`umbra-cpp-fom-switch-defaults-composition-integration` (14 assertions). It
composes omitted and explicit-false 2025 switch settings in both equivalent
module orders, requires the Disabled default, and proves that no spurious
Annex C.8 warning is emitted. Its two selected Lab candidates resolve to IEEE
1516.2 clauses 4.13.3 and C.8; runtime switch mutation, broader Annex C merge
behavior, JUnit/protected review, and conformance remain separate. The exact
FOM-composer CTest passes independently. The plan total is now 828 rows; the
source-only queue is 283 declarations globally and 7 in the FOM-composer file.
The next exact source head remains owned by `fom-module-declaration-management`:
`The FOM composition preflight validates time-representation data type
categories` at `cpp/tests/libxml2_fom_composer_catch2.cpp:3444`. No Requirements
Lab resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Time-representation category guard

The declaration-management source head `The FOM composition preflight validates
time-representation data type categories` is now indexed as
`umbra-cpp-fom-time-representation-category-validation` (26 assertions). It
accepts the official integer64 and float64 logical-time representations,
rejects basic and reference data types that are not permitted for time
representation, and preserves the exact diagnostics. Its selected Lab
candidate resolves to IEEE 1516.2 clause 6.2.8; reference-time selection,
runtime time coordination, JUnit/protected review, and conformance remain
separate. The exact FOM-composer CTest passes independently. The plan total is
now 829 rows; the source-only queue is 282 declarations globally and 6 in the
FOM-composer file. The next exact source head remains owned by
`fom-module-declaration-management`: `The FOM composition preflight validates
user-supplied and synchronization tag data type categories` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:3774`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Tag datatype category guard

The declaration-management source head `The FOM composition preflight validates
user-supplied and synchronization tag data type categories` is now indexed as
`umbra-cpp-fom-tag-data-type-category-validation` (18 assertions). It accepts a
permitted user-supplied tag datatype, rejects a basic datatype for both
user-supplied and synchronization tags, and preserves the exact diagnostics.
Its two selected Lab candidates resolve to IEEE 1516.2 clauses 6.2.9 and
6.2.10; runtime tag transport, broader composition, JUnit/protected review,
and conformance remain separate. The exact FOM-composer CTest passes
independently. The plan total is now 830 rows; the source-only queue is 281
declarations globally and 5 in the FOM-composer file. The next exact source
head remains owned by `fom-module-declaration-management`: `The FOM composition
preflight rejects inherited attribute and parameter name overloading` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:3814`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Inherited-member name guard

The declaration-management source head `The FOM composition preflight rejects
inherited attribute and parameter name overloading` is now indexed as
`umbra-cpp-fom-inherited-member-name-validation` (12 assertions). It rejects an
object-class attribute and interaction-class parameter that overloads an
inherited member name and preserves the exact diagnostics and fixture
identities. Its two selected Lab candidates resolve to IEEE 1516.2 clauses 6.2.5
and 6.2.6; broader hierarchy composition, runtime declaration behavior,
JUnit/protected review, and conformance remain separate. The exact
FOM-composer CTest passes independently. The plan total is now 831 rows; the
source-only queue is 280 declarations globally and 4 in the FOM-composer file.
The next exact source head remains owned by
`fom-module-declaration-management`: `The FOM composition preflight enforces
enumerated and variant-record merge invariants` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:3887`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Enumerated/variant merge invariants

The declaration-management source head `The FOM composition preflight enforces
enumerated and variant-record merge invariants` is now indexed as
`umbra-cpp-fom-enumerated-variant-merge-invariants` (16 assertions). It rejects
conflicting enumerated values, non-extendable variant-record alternatives, and
prohibited HLAother expansion while preserving the exact diagnostics. Its two
selected Lab candidates resolve to IEEE 1516.2 clause C.3 and clause 7;
broader variant-record semantics, hierarchy composition, JUnit/protected
review, and conformance remain separate. The exact FOM-composer CTest passes
independently. The plan total is now 832 rows; the source-only queue is 279
declarations globally and 3 in the FOM-composer file. The next exact source
head remains owned by `fom-module-declaration-management`: `The FDD materializer
remaps referenced notes and logically ORs service usage` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:3922`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Notes and service-usage merge

The declaration-management source head `The FDD materializer remaps referenced
notes and logically ORs service usage` is now indexed as
`umbra-cpp-fom-notes-service-usage-merge` (16 assertions). It remaps note
labels and logically ORs service usage while preserving both contributing note
texts in the materialized FDD. Its two selected Lab candidates resolve to IEEE
1516.2 clauses C.9 and C.10; broader composition, dependency packaging,
JUnit/protected review, and conformance remain separate. The exact
FOM-composer CTest passes independently. The plan total is now 833 rows; the
source-only queue is 278 declarations globally and 2 in the FOM-composer file.
The next exact source head remains owned by `fom-module-declaration-management`:
`Reference logical-time selection defaults to HLAfloat64Time and rejects
incompatible FDD documentation` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:3951`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Reference logical-time selection

The declaration-management source head `Reference logical-time selection
defaults to HLAfloat64Time and rejects incompatible FDD documentation` is now
indexed as `umbra-cpp-reference-logical-time-selection` (21 assertions). It
defaults an unconstrained catalog to HLAfloat64Time, accepts a compatible
HLAinteger64Time selection, and rejects incompatible or unavailable
implementations with deterministic statuses. Its three selected Lab
candidates resolve to IEEE 1516.2 clause 4.8.3 and IEEE 1516.1 clauses 4.5.5
and 4; public time-service behavior, broader federation preparation,
JUnit/protected review, and conformance remain separate. The exact
FOM-composer CTest passes independently. The plan total is now 834 rows; the
source-only queue is 277 declarations globally and 1 in the FOM-composer file.
The next exact source head remains owned by `fom-module-declaration-management`:
`Federation preparation loads MIM first and emits only an FDD-backed
reference-time definition` at
`cpp/tests/libxml2_fom_composer_catch2.cpp:3998`. No Requirements Lab
resynchronization or new numbered requirement was performed.

### 2026-09-02 queryability maintenance — Federation preparation and queue exhaustion

The declaration-management source head `Federation preparation loads MIM first
and emits only an FDD-backed reference-time definition` is now indexed as
`umbra-cpp-federation-preparation-mim-and-reference-time` (22 assertions). It
loads the standard MIM first, materializes an FDD-backed definition, validates
reference-time selection, and rejects invalid or incompatible additional-module
inputs with deterministic statuses. Its three selected Lab candidates resolve
to IEEE 1516.1 clauses 4.5.5 and 4.11.4; public federation-management behavior,
runtime join/create services, JUnit/protected review, and conformance remain
separate. The exact FOM-composer CTest passes independently. The plan total is
now 835 rows; the source-only queue is 276 declarations globally and 0 in the
FOM-composer file. The FOM-composer reconciliation queue is now exhausted;
select the next bounded C++ service family rather than rescanning this file or
the unchanged Requirements Lab. No Requirements Lab resynchronization or new
numbered requirement was performed.

### 2026-09-02 implementation slice — Process local-delete transport contract

The process-boundary implementation now carries `local-delete-object-instance`
through the private transport service seam. The request codec validates the
federation, federate, and object identity; the result codec preserves every
official `LocalObjectInstanceDeletionStatus` value; and the service handler
returns the registry status without inventing a second vocabulary. The public
`RTIambassador::localDeleteObjectInstance` path uses that endpoint when the
configured process profile is active and retains the existing embedded path
otherwise. The registry-bound integration now proves the recipient-local
object removal (44 assertions), while the standalone codec contract provides
9 deterministic assertions. Both plan rows map to the local-delete API
surface and IEEE 1516.1-2025 §6.18.1; they remain private foundation evidence,
not protected conformance evidence. The focused process lane is now 34 mapped
cases / 1,294 focused-JUnit assertions, and the full Catch2 plan is 837 rows.
The bounded lane check also exposed a maintenance edge in the completion
ledger: three older source-line pointers in the same process test file had not
moved when earlier cases were inserted, and a duplicate completion row had
been recorded. Those ledger entries were corrected and the duplicate removed;
this is traceability-ledger drift, not a Requirements Lab change. No
Requirements Lab resynchronization or new numbered requirement was performed;
the exact queries are:

```powershell
python tools/query_rti_work.py test "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py trace "Private process local-delete request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
```

### 2026-09-02 implementation slice — Public process receive-order Delete Object Instance

The public process endpoint now carries ordinary `RTIambassador::deleteObjectInstance`
through the private service seam. The bounded two-federate case registers and
discovers one object, deletes it with an opaque tag, confirms that the producer
and receiver no longer retain the object in their respective ledgers, confirms
that the producer receives no removal callback, and verifies the receiver's
official `removeObjectInstance` callback preserves the object, tag, and
producing federate. It is green under the HLA_EVOKED callback model with 25
assertions. The companion private codec case round-trips the request identity,
tag, recipient count, and all six `ObjectInstanceDeletionStatus` values with 16
assertions. The two rows are mapped to the pinned 2025 candidates for §§6.16,
6.16.4, and 6.17.1; they remain development-profile foundation evidence, not
protected conformance evidence.

The focused process-boundary lane is now 37 mapped/source-located cases / 1,544
focused-JUnit assertions, and the full C++ Catch2 plan is 839 rows. Adding the
codec declarations and the public case shifted several existing completion-ledger
source pointers; the bounded lane check caught and repaired those pointers
without any Requirements Lab resynchronization or new numbered requirement.
The exact bounded queries are:

```powershell
python tools/query_rti_work.py test "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py trace "Private process Delete Object Instance request and result preserve the official status vocabulary" --summary --compact
python tools/query_rti_work.py test "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
```

### 2026-09-02 implementation slice — Public process local-delete endpoint

The public two-federate process case now drives `RTIambassador::localDeleteObjectInstance`
through the configured endpoint: one federate registers the object, the other
discovers it and requests recipient-local deletion, and the service verifies
the registry transition before both federates resign. The Catch2 case provides
18 focused assertions and is indexed separately from the 9-assertion codec
contract and 44-assertion private service integration. The focused
process-boundary lane is now 35 mapped cases / 1,294 focused-JUnit assertions;
the full Catch2 plan is 837 rows. No Requirements Lab resynchronization or new
numbered requirement was performed. Query this slice directly with:

```powershell
python tools/query_rti_work.py test "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py trace "RTIambassador routes Local Delete Object Instance through a configured process endpoint" --summary --compact
python tools/query_rti_work.py check --lane process-boundary --summary --compact
```
