# Requirements Lab issue ledger

This note records defects and usability rough edges in the pinned IEEE
1516.1-2025 Requirements Lab export. It is deliberately separate from the
export and from the C++ test plan: issue entries explain why a requirement or
section may need review, but they never rewrite a Lab identifier or silently
move evidence between subsections.
Historical observations such as RL-030 remain in
`docs/testing/REQUIREMENTS-LAB-OBSERVATIONS.md`; this live ledger lists only
issues that still apply to the current pinned export.

The machine-readable ledger is
[`compliance/requirements-lab/known-issues.json`](../../compliance/requirements-lab/known-issues.json).
Use the bounded query from the repository root:

```powershell
python tools/query_rti_work.py lab-issues --summary --compact
python tools/query_rti_work.py lab-issues RL-267 --summary --compact
```

## Policy

- The pinned 2025 export remains the requirements source of truth.
- Do not resynchronize merely because a query exposes a defect; resynchronize
  only when the pinned export changes.
- A recurrence after the existing issue range gets a new issue id rather than
  being folded into an older entry.
- Keep both the exported Lab clause and the reviewed normative clause visible
  until the upstream extraction is corrected.

## Open issues

### RL-177 — page-132 object-management clause boundary

The Lab records
`requirement-candidate-content-clauses-06-object-management-page-132-l107-30`
and
`requirement-candidate-content-clauses-06-object-management-page-132-l113-32`
under exported subsection **6.18.1**. Their statements describe the Delete
Object Instance postconditions on page 132, including the Flush Queue Request
and disable-time-constrained cases; the source heading places those
postconditions in **§6.17.4**. The following §6.18 heading belongs to the Local
Delete Object Instance service.

Source: `content/clauses/06-object-management-page-132.tex`, lines 107–118
(page 132), document `hla-1516.1-2025`.

Impact: a forward gap query can display the requirements under the wrong
subsection, making correct evidence appear uncovered or encouraging an
incorrect test-to-standard mapping.

Workaround: retain the stable Lab ids and exported `6.18.1` value in plan
data, cite the source-heading correction as `§6.17.4` in review notes, and do
not mutate the Requirements Lab export in this repository.

Reproduce the bounded observation with:

```powershell
python tools/query_rti_work.py gaps --family object-ddm-ownership --clause 6.18 --summary --compact --limit 40
```

### RL-267 — page-67 Initiate Federate Save label clause ownership recurs

The pinned candidate
`requirement-candidate-content-clauses-04-federation-management-page-067-l22-3`
describes the Initiate Federate Save label, whose source belongs to **§4.20**,
but the export attaches it to **§4.21.1**. The mismatch surfaced again while
mapping the single-federate process save-not-complete test. The test verifies
that the label reaches the callback, but its focused mapping deliberately
leaves that assertion unmapped rather than attributing it to Federate Save
Begun.

This is a new recurrence of the page-67 service-boundary issue documented as
RL-082, not a replacement of that historical record. Preserve the candidate
id and exported clause until the Lab refinement is reviewed; do not resync or
mutate the pinned export as part of RTI work.

Source: `content/clauses/04-federation-management-page-067.tex`, line 22
(page 67), document `hla-1516.1-2025`.

### RL-336 — changed-lookahead contracts map Query Lookahead to Modify Lookahead §8.20.4

Candidate
`requirement-candidate-content-clauses-08-time-management-page-208-l66-15`
states a requirement for the **Modify Lookahead** service. The changed-lookahead
requirements contract instead assigns that candidate and §8.20.4 to the
`queryLookahead` assertion. The associated native case only queries the active
lookahead after disabling and re-enabling Time Regulation with a new interval;
it does not invoke Modify Lookahead. The same misattribution recurs in the
timestamped attribute, interaction, deletion, and regional changed-lookahead
contracts. This is a traceability mismatch, not a conformance finding.

Workaround: do not use this candidate as evidence for Query Lookahead. Keep
that assertion explicitly unmapped until a Query Lookahead-specific normative
anchor is verified; leave the pinned Requirements Lab export unchanged and do
not resync it for this issue.

Source: pinned `semantic/requirements.json`, candidate above; the native
contract `compliance/requirements-lab/timestamped-attribute-update-regulation-reenable-changed-lookahead-requirements-contract.json`;
and `cpp/tests/timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2.cpp`.

## Resolved local harness rough edge

While adding the next bounded ownership handoff, the Catch2 plan validator
treated a deliberately **planned** row as executable and rejected its
not-yet-declared `TEST_CASE` selector. That made a valid roadmap contract fail
the traceability CTest before implementation could begin. The validator now
recognizes `status: "planned"` alongside the existing disabled and
source-reconciliation states, while `ready` and `focus` continue to expose the
row as planned and source-unlocated. This is a local validation usability fix,
not a change to the pinned Requirements Lab export; keep the planned row's
requirement and 2025 subsection mapping intact until its C++ case is added.

### RL-268 — unrelated global Catch2 plan validation drift

The global plan audit
`python tools/requirements_lab.py check-plan --plan compliance/requirements-lab/catch2-test-plan.json`
reports eight findings in other work lanes: two rows lack a non-empty
`next_action`; three references point to the absent API surface
`api.2025.cpp.rtiambassador.setservicereportingswitch.86951e8b3e77`; and one
timestamped directed-interaction row contains four requirement/API identifiers
absent from the pinned Lab surfaces. The affected rows concern timestamped
attribute update, negotiated/confirmed divestiture, timestamped directed
interaction, and process restore status.

Impact: the global Catch2 requirements-plan gate is not green, but these
unrelated findings do not invalidate the independently passing promoted
portable TCK interaction-publication slice or its IEEE 1516.1-2025 and
1516.2-2025 contracts.

Workaround: repair each affected row only after checking its exact 2025 source
and Lab identity. Keep this drift separate from portable TCK evidence; do not
resynchronize or rewrite the pinned Requirements Lab export to silence the
validator.

### RL-269 — ownership-query test selectors retained the old source path

The ownership-query requirements/API contracts and ownership-check requirements
contract still selected the RTI-owned MOM query case from the large federation
management test file after it had been extracted to
`cpp/tests/ieee1516_2025_rti_owned_mom_ownership_query_catch2.cpp`. This made the
focused ownership-check requirements traceability test fail with contract drift;
the requirement IDs and clause mappings themselves were unchanged.

Resolution: updated the seven stale selectors to the focused 2025 test path.
The four requirements/API traceability tests for the ownership-query and
ownership-check contracts now pass, as do the mapping checks for both lanes.
No pinned Requirements Lab export or normative mapping was changed.

### RL-270 — extracted service-report tests retained old selectors

The MOM service-reporting requirements contract contained 64 unique selectors
(176 references) that still named the federation-management monolith after the
corresponding 2025 tests had moved into focused units. The Attribute Ownership
Acquisition API contract also retained one selector for the focused multi-acquirer
Release Denied case.

Resolution: changed only those selectors to the unique source files containing
the matching `TEST_CASE` declarations. The MOM service-reporting, ownership-
acquisition API, and negotiated-divestiture requirements/API traceability tests
now pass 4/4. Requirement IDs and normative mappings are unchanged; the pinned
Requirements Lab export was not synchronized or modified.

### RL-276 — federation MOM content-report case remained classified as disabled

The bounded query reported the federation-scoped FOM/MIM content-report case as
`disabled-source-artifact`, source-unlocated, and zero-assertion, with a note
claiming its declaration remained inside `#if 0`. The current tree instead has
an active focused test at
`cpp/tests/federation_mom_current_fdd_catch2.cpp:324`; its target builds and its
exact CTest passes.

Resolution: reconciled the existing plan row with the active 53-assertion
development-profile case, its `HLA_EVOKED` callback model and focused tag. The
direct clause-4 mapping and seven API surfaces are unchanged. The focused test,
trace, MOM lane check, and federation content-report check pass; no pinned
Requirements Lab export or IEEE 1516.1-2010 source or mapping changed.

### RL-277 — connection-loss and support-switch contracts retained old paths

The connection-loss lane sweep found stale source-qualified test selectors for
extracted connection-loss, Federate Lost, and support-switch cases, as well as
implementation-symbol paths still naming the old federation registry and RTI
ambassador translation units.

Resolution: updated only the local selector and implementation-source paths.
The #510/#521/#529 behavior tests and seven related requirements/API
traceability CTests pass 10/10. The #510 mapping still has eight direct pairs
across six canonical 2025 clauses and seven API surfaces. No requirement or API
mapping, pinned Lab export, or IEEE 1516.1-2010 file changed.

### RL-278 — timestamped-interaction contracts retained pre-extraction paths

The timestamped-interaction requirements and API traceability tests found old
test selectors still pointing into the federation-management and connection
monoliths, plus registry source-symbol paths that no longer contained the
referenced definitions.

Resolution: corrected only the local selectors and implementation-source
paths. Traceability CTests #2092/#2093 and the related connection-loss gates
#1910/#1911 pass 4/4; strict-less-than and later-TAR cases #514/#531 and all 26
cutoff-lane tests pass. Requirement, clause, and API mappings and the pinned
Lab export are unchanged; no IEEE 1516.1-2010 source or mapping changed.

### RL-279 — asynchronous-delivery contracts retained monolith selectors

The asynchronous-delivery requirements and API traceability gates still
pointed two extracted cases at the federation-management monolith instead of
their current focused source files.

Resolution: corrected only the eight source-qualified selector occurrences.
The asynchronous-delivery gates #2071/#2072 and all eight affected traceability
gates pass. The #530 cutoff case and all 26 cutoff-lane tests pass; no
requirement, clause, or API mapping, pinned Lab export, or 2010 source changed.

### RL-280 — federate-lookup and support-switch contracts retained moved source references

The federate-lookup requirements gate referenced `federateNameFor` in the old
`federation_registry.cpp`, and the support-switch table contract still named
two tests in the federation-management monolith after their extraction.

Resolution: updated the registry source path and the two selectors to their
current definitions. CTests #1917 and #1989 pass, along with the extracted
DELETE_OBJECTS case #505 and its other selected traceability gates. No
requirement, clause, or API mapping, pinned Lab export, or IEEE 1516.1-2010
source changed. This recurrence is tracked separately from RL-277.

### RL-281 — declaration-failure plan rows retained duplicate monolith locations

Three extracted declaration-failure rows each contained a focused-file
`source_location` plus a second stale location in the federation-management
monolith. Consumers could silently select the stale duplicate.

Resolution: removed only the three duplicate monolith locations. The case
queries resolve to the focused source files; requirement, clause, and API
mappings are unchanged. No pinned Lab export or IEEE 1516.1-2010 source or
mapping changed.

### RL-282 — object-name reservation contracts retained pre-split source paths

The object-instance-name reservation requirements and API traceability gates
found implementation symbols and test selectors still pointing at the former
registry, RTI-ambassador, and test monoliths after those definitions were split
into focused files.

Resolution: corrected only the local source-symbol and test-selector paths.
The eight selected connection-loss, resign-action, support-switch, and
object-name traceability CTests pass; requirement, clause, and API mappings
are unchanged. No pinned Lab export or IEEE 1516.1-2010 source or mapping
changed.

### RL-283 — NoAction plan row retained a duplicate monolith source pointer

A duplicate-key audit of the Catch2 plan found the extracted #503 NoAction
case still had both its stale federation-management monolith pointer and its
current focused-source pointer.

Resolution: removed only the stale monolith `source_location`. The case query
resolves to the focused 2025 source; requirement, clause, and API mappings
are unchanged. No pinned Lab export or IEEE 1516.1-2010 source or mapping
changed.

### RL-284 — timestamped attribute-update contracts retained moved source pointers

The timestamped default-region and regional attribute-update regulation-
reenablement traceability gates still pointed moved RTI-ambassador and
federation-registry symbols at their former monolith translation units. The
regional case's contract selectors also still named its pre-extraction test
location.

Resolution: corrected only those implementation-source pointers and the 31
exact selectors for the extracted regional case. Gates #2098/#2099 and
#2110/#2111 pass, as do both focused behavior cases (#485 and #226).
Requirement, subsection, and API mappings are unchanged; no pinned Lab export
or IEEE 1516.1-2010 source or mapping changed.

### RL-285 — ownership contracts retained pre-split source and test pointers

The regular-acquisition and negotiated-divestiture traceability gates found
registry symbols still attributed to `federation_registry.cpp` and service-
report or release-denied test selectors still naming the federation-
management monolith after those cases had been extracted.

Resolution: corrected the local implementation-source paths and selectors in
the four affected ownership contracts. Gates #2043/#2044/#2051/#2052 pass,
alongside connection-loss and support-switch gates #1910/#1911/#1990/#1992;
the extracted negotiated-cancellation case #508 passes. Requirement,
subsection, and API mappings are unchanged; no pinned Lab export or IEEE
1516.1-2010 source or mapping changed.

### RL-286 — 2025 traceability contracts retained pre-extraction paths

The asynchronous-delivery API contract still located both ambassador methods
in `umbra_rti_ambassador.cpp` after their same-translation-unit extraction.
The same focused CTest sweep exposed stale 2025 source/test selectors in the
interaction-region, Convey Region Designator Sets, and support-switch
contracts. Traceability CTests #1920, #1921, #1951, #1969, and #2031 failed
with contract drift.

Resolution: corrected only implementation-source paths and 2025 C++ test
selectors in the five local contracts. All five traceability tests and the
15-test `asynchronous-delivery` CTest label now pass. Requirement IDs,
subsections, API mappings, and IEEE 1516.1-2010 sources remain unchanged.

### RL-287 — handle-decoding API contract retained monolith source paths

The seven basic public handle-decoding methods moved into a same-translation-
unit fragment, but their API contract still located them in
`umbra_rti_ambassador.cpp`. Traceability CTest #1957 failed for all seven
source symbols.

Resolution: updated only those seven source paths to the extracted fragment;
the MessageRetractionHandle locator remains on its separate implementation.
CTests #1957 and #221 pass. The explicit no-requirement disposition and all
eight API-surface mappings are unchanged; no IEEE 1516.1-2010 source or
mapping changed.

### RL-288 — MOM service-report traceability retained extracted-source pointers

Focused 2025 traceability checks #1920 and #1953 exposed stale Requirements
Lab source locators after ambassador service/report methods had moved into
same-translation-unit fragments. The MOM service-report contract had 179
stale source paths across 66 moved symbols, and one test selector still named
the federation-management monolith for the federated-MOM content case.

Resolution: updated only implementation-source paths in the MOM service-
report and interaction-region contracts, plus the single stale test selector.
Requirement IDs, clauses, API mappings, and IEEE 1516.1-2010 records are
unchanged. CTests #1920, #1921, #1953, #48, #263, #881, and #1242 pass.

### RL-313 — page-178 pre-grant delivery statement is truncated

The candidate `requirement-candidate-content-clauses-08-time-management-page-
178-l51-14` ends at “before granting the joined”, omitting the continuation
that names the joined federate's time-advance request and says only messages
listed in Table 1's delivery column may be delivered before grant. The
reconstructed Table 1 TARA row gives all TSO messages with `ts < t_R` and
already-queued TSO messages with `ts = t_R`. Full §8.9 confirms the queued
equality rule while allowing later equal-time arrivals after the grant. The
prior conclusion that every equal-time callback follows the grant was wrong.

Resolution/workaround: retain the pinned candidate ID, but read it with the
reconstructed page-178 continuation, the specific Table 1 service row, and
full §8.9. Test evidence is scoped to already-queued TARA equality only.
RL-313 is recorded in
`known-issues.json`; this turn did not independently verify the canonical PDF,
so it is a pinned-export completeness/usability issue, not IEEE errata.

### RL-314 — page-294 service-report log-format subject is truncated

The §11.5.2.1 candidate stops after “service report,” omitting “log records.”
The pinned Lab layout retains that continuation. Keep the candidate as a Lab
extraction issue; the existing C++ file-report test does not turn this clipped
candidate into validated coverage.

### RL-315 — page-069 Federation Saved success condition is fragmented

The pinned 2025 Lab export divides one clause 4.23 Federation Saved success
condition among page-069 candidates at lines 64, 67, and 70. The last begins
“shall mean” without the success-indicator antecedent, while the first two
also stop mid-sentence. A lookup or mapping of the line-70 candidate alone
does not convey the all-participants-complete condition.

Workaround: retain the stable IDs but read and map all three fragments as one
paragraph-level condition when a C++ test observes both the callback and the
completion barrier. The new public multi-member save-commit barrier test maps
all three. RL-315 is recorded in `known-issues.json`; this is a pinned-export
usability defect, not a claim of IEEE errata or conformance.

### RL-316 — §8.9 message-set candidate omits its enumerated set

The page-195 §8.9 candidate ends immediately after introducing the messages
TARA shall deliver. It omits the enumerated items, particularly queued TSO
messages whose timestamp equals the requested time. Read the pinned full-text
§8.9 and Table 1 for this boundary. This is an extraction issue, not IEEE
errata or a conformance claim.

### RL-317 — page-122 Reflect Attribute Values preamble has the wrong subsection

The 2025 candidates `requirement-candidate-content-clauses-06-object-
management-page-122-l61-17` and `...-l79-23` label the basic Reflect
Attribute Values rule and complete transportation-passel rule as §6.11.1.
The pinned reconstructed page places both in the §6.11 preamble, before
the §6.11.1 Supplied arguments heading.

Workaround: retain the immutable Lab candidate and exported metadata, but use
the source-backed canonical §6.11 crosswalk in
`custom-regional-initial-transport-requirements-contract.json`. This is a Lab
crosswalk issue, not an IEEE erratum; the canonical PDF was not separately
checked in this turn.

### RL-318 — page-233 regional-discovery rule is split into fragments

The 2025 §9.8 regional-subscription discovery sentence is split across
`requirement-candidate-content-clauses-09-data-distribution-management-page-
233-l30-7`, `l33-8`, and `l36-9`. The first two end mid-sentence, and the
third starts with a continuation. The pinned reconstructed page presents the
continuous rule.

Workaround: treat the three IDs as one linked paragraph group when planning
focused evidence; no fragment independently establishes the full obligation.
The pinned bundle is unchanged. This is an extraction/usability issue, not
IEEE errata; the canonical PDF was not separately checked in this turn.
### RL-319 — Annex C.3 extendable-variant merge rule is clipped

The pinned §C.3 candidate for `HLAextendableVariantRecord` stops mid-sentence
at the end of semantic page 110. Page 111 continues with the discriminant
enumerator conflict rule and the `HLAother` rejection. Read both pages before
planning variant-record merge tests; do not treat the clipped candidate as
complete validation. The separate public C++ data-type-addition test addresses
only the earlier, intact §C.3 requirement. This is a Requirements Lab
extraction issue, not IEEE errata.
### RL-320 — page-115 Reserve Multiple Names mixes two distinct rules

The pinned 2025 §6.5.3 candidate at page 115, line 107 starts in the middle
of the sentence saying an illegal name aborts reservation of the entire set,
then continues into the separate result-callback rule. The adjacent line-104
and line-110 candidates supply its missing context. Read alone, line 107 can
make an unavailable but legal name look like an abort-all condition. A local
reservation requirements contract also labels this candidate as reporting
subsets, although the per-name success indicators and exclusive rights are
described with the Multiple Object Instance Names Reserved service in §6.6.4.

Workaround: keep the immutable IDs and pinned export. Read page-115 lines
104–115 together. Map mixed unavailable/free requests to the multiple-name
result and ownership rules; test an actually illegal supplied name before
claiming the abort-all rule. Correct the local crosswalk label separately.
The new public filesystem-restore case tests only the mixed-name partition.
RL-320 is recorded in `known-issues.json`; this is an extraction/crosswalk
usability issue, not IEEE errata or a conformance claim.

### RL-321 — §8.11 NMRA backward-time flow items are missing candidates

The full pinned 2025 §8.11.3(d) requires the supplied logical time to be at
least the joined federate's current time, and §8.11.5(a) names the Logical
Time Already Passed exception. The candidate bundle retains neighboring
§8.11 narrative fragments but neither service-flow item. An official-C++
backward NMRA test should cite those subsections as full-section/API evidence,
without inventing a requirement ID; its separate service-report assertions
can map to the existing §11.5 candidates. The bundle was not resynchronized.
This is a Lab extraction issue, not IEEE errata or conformance evidence.

### RL-322 — §8.11 NMRA Time Advancing flow items are missing candidates

The pinned 2025 §8.11.3(f) requires the joined federate not be in the Time
Advancing state, and §8.11.5(c) lists that state as an exception. These
items appear at page-200 semantic lines 11 and 23, before the page's first
candidate row. This is distinct from RL-321's backward-time pair. The new
official-C++ pending-advance case cites the full subsections for its public
exception and maps only its file-report assertions to real §11.5 candidates.
No candidate ID was invented and the bundle was not resynchronized. This is
a Lab extraction issue, not an IEEE erratum or conformance claim.

### RL-323 — Evoke Callback postcondition is attributed to §10.60.6

The pinned 2025 candidate `requirement-candidate-content-clauses-10-support-
services-page-286-l20-3` labels the rule to process pending callbacks on an
Evoke Callback or Evoke Multiple Callbacks invocation as §10.60.6. Its
reconstructed source at page 286, lines 18–36 places the rule under
§10.59.4 Postconditions. Section 10.60.6 is the reference-state-charts
subsection of Disable Callbacks, not this Evoke postcondition.

Workaround: keep the immutable candidate ID and pinned export, but map this
rule to canonical §10.59.4 when planning or reviewing C++ callback evidence.
Do not infer a §10.60.6 validation claim from the exported label. No Lab
resynchronization was performed. This is a source-to-clause attribution issue,
not an IEEE erratum or a new conformance result.

### RL-324 — Attribute transportation default-on-reacquisition is attributed to §6.25.1

The pinned 2025 candidate `requirement-candidate-content-clauses-06-object-
management-page-139-l150-40` exports the rule that a newly owning federate
uses the class attribute's default transportation type as §6.25.1. The
reconstructed page-139 source places that sentence in the introductory §6.25
paragraph, before the `6.25.1 Supplied arguments` heading. This is a new
occurrence of the heading-boundary attribution problem; RL-177 and RL-317 do
not cover this candidate.

Workaround: preserve the pinned candidate ID and export, but use canonical
§6.25 for this rule when reviewing ownership/transportation evidence. Do not
invent a requirement ID for the separate §6.25 callback-invocation timing
sentence, which is not directly mapped by the current process attribute-
transportation case. No Lab resynchronization was performed. This records a
Lab extraction issue, not an IEEE erratum or conformance result.

### RL-325 — Attribute transportation confirmation rule is attributed to §6.27

The pinned 2025 candidate `requirement-candidate-content-clauses-06-object-
management-page-141-l11-1` exports the one-request/one-confirmation rule as
§6.27. In the reconstructed page-141 source, that sentence precedes the
`6.27 Change Default Attribute Transportation Type` heading and belongs to
§6.26, Confirm Attribute Transportation Type Change. This candidate was not
covered by RL-324 or the older heading-boundary issues.

Workaround: retain the pinned candidate ID and export, but map the existing
attribute-transportation C++ case and its focused contract check to canonical
§6.26 for this rule. Do not claim that it covers §6.27's change-default
service. No Lab resynchronization was performed; this is a Lab extraction
attribution issue, not an IEEE erratum or conformance result.

### RL-326 — §4.20 timed Initiate Federate Save candidate ends mid-sentence

The pinned 2025 candidate
`requirement-candidate-content-clauses-04-federation-management-page-067-l37-8`
contains only “time-constrained, it shall expect to receive an Initiate
Federate Save service invocation only when it is in”. It begins without the
subject and ends before the required state is named, so the exported fragment
cannot stand alone as the timed-save obligation. This is distinct from the
page-67 clause-boundary attribution recurrence in RL-267; do not silently
rewrite that earlier issue.

Workaround: preserve the pinned ID/export, read the surrounding canonical
§4.20 text before judging the timed-save behavior, and do not promote this
fragment by itself as complete direct conformance evidence. The current
multi-constrained timed-save Catch2 row also selects several coherent but
unrelated §4.14/§4.15 and untimed §4.19 anchors; those are local test-plan
mapping defects to adjudicate separately, not additional Lab defects.
No Requirements Lab resynchronization was performed. Source: pinned
`topics/ieee/hla-1516.1-2025/semantic/requirements.json`, page-67 candidate
at `content/clauses/04-federation-management-page-067.tex:37`.

### RL-327 — §4.19 timed Request Federation Save candidate stops at page break

The pinned candidate
`requirement-candidate-content-clauses-04-federation-management-page-065-l109-26`
begins the rule for a valid timestamped Request Federation Save and each
time-constrained federate's Initiate Federate Save, but stops at “the timestamp
of”. The remainder of the sentence is absent from that candidate; the next
exported candidate starts a different rule for nontime-constrained members.
This is a separate §4.19 page-boundary recurrence from RL-326's §4.20 fragment,
not a correction to the pinned export.

Workaround: consult the complete canonical §4.19 rule when reviewing timed-save
ordering, and do not treat this candidate fragment alone as a complete direct
conformance obligation. The multi-constrained timed-save Catch2 mapping needs
its own review because it currently selects unrelated synchronization and
untimed-save candidates. No Lab resynchronization or IEEE erratum is implied.
Source: pinned `topics/ieee/hla-1516.1-2025/semantic/requirements.json`,
`content/clauses/04-federation-management-page-065.tex:109-113`.

### RL-328 — Queued-TSO save anchors recur across later-heading clause boundaries

The pinned 2025 export assigns several complete service rules to a later
subsection on the same page, although the reconstructed source places each
rule before that subsection's heading. This is a new cluster found while
auditing the queued-TSO timed-save row, distinct from the page-67 save
fragments in RL-326/RL-327 and the individual earlier boundary reports:

- `requirement-candidate-content-clauses-05-declaration-management-page-084-l32-10`:
  exported §5.1.4; the subscribed-interaction-class rule precedes that heading
  and belongs to §5.1.3.
- `requirement-candidate-content-clauses-06-object-management-page-124-l41-12`,
  `...page-124-l47-14`, and `...page-124-l53-16`: exported §6.12.4; the
  delivery, no-self-delivery, and tag rules precede §6.12.1 and belong to
  the §6.12 service description.
- `requirement-candidate-content-clauses-08-time-management-page-191-l73-17`,
  `...page-192-l63-17`, and `...page-194-l21-3`: exported §8.5.5, §8.6.3,
  and §8.8.3 respectively; their service-introduction rules belong to
  §8.5, §8.6, and §8.8.
- `requirement-candidate-content-clauses-08-time-management-page-194-l27-5`:
  exported §8.8.3; the TAR message-set rule (queued RO plus TSO timestamps
  less than or equal to the requested time) occurs under the §8.8 service
  description before §8.8.1, so it belongs to §8.8.
- `requirement-candidate-content-clauses-08-time-management-page-206-l29-4`
  and `...page-207-l112-28`: exported §8.18.1 and §8.19.3; the Query GALT
  and Query LITS result rules belong to §8.17 and §8.19 respectively.
- `requirement-candidate-content-clauses-10-support-services-page-286-l20-3`:
  exported §10.60.6; the pending-callback rule is explicitly under the
  §10.59.4 Postconditions heading, before §10.60 begins.

Several of these candidates are also sentence fragments. Workaround: do not
promote the affected IDs as direct requirement-to-canonical-subsection pairs
in the queued-TSO test plan merely because the runtime assertion exercises
part of the service. Preserve the pinned export and inspect the full 2025
clause until the Lab fixes the extraction. This is a Lab attribution issue,
not an IEEE erratum or a conformance finding; no Lab resynchronization was
performed.

### RL-329 — Page-165 Attribute Ownership Unavailable introduction inherits 7.10.6

The pinned candidate
`requirement-candidate-content-clauses-07-ownership-management-page-165-l14-1`
says what the Attribute Ownership Unavailable service informs the joined
federate, but the export labels it §7.10.6. On the reconstructed page, this
introductory rule precedes §7.10.1 Supplied arguments; §7.10.6 is the later
Reference state charts heading at the page foot. The service introduction
belongs to §7.10, not §7.10.6.

The wrong heading can make direct C++ evidence look like coverage of reference
state charts instead of the unavailable-service rule. Preserve the stable Lab
candidate ID and exported clause, while mapping the local If Available tests
and requirements contract to canonical §7.10. The pinned export was not
resynchronized. This is a Lab source-attribution issue, not an IEEE erratum or
conformance finding; the canonical PDF was not independently checked in this
turn. Source: pinned `semantic/requirements.json` and the page-165 heading
sequence in `semantic/content-blocks.json`.

### RL-330 — Page-180 mixed-passel message rule inherits 8.1.4

Five page-180 candidates with full prefix
`requirement-candidate-content-clauses-08-time-management-page-180-`
and suffixes `l9-1`, `l21-5`, `l27-7`, `l39-11`, and `l45-13` are exported
as §8.1.4 despite appearing under §8.1.3 Messages. The reconstructed page
places the §8.1.4 Message order type heading only at line 70, after all
five. The `l27-7` separate-passel-message rule is complete and has a focused
C++ case; `l9-1` is introductory, while `l21-5`, `l39-11`, and `l45-13`
are incomplete fragments. Apply a local §8.1.3 section crosswalk but do not
invent independent coverage for those four records. Keep the pinned export
unchanged; do not resync unchanged Lab inputs. This is a Lab attribution and
fragmentation defect, not IEEE errata or conformance evidence. The
canonical PDF was not independently checked in this turn.

### RL-331 — Page-153 acquisition and privilege rules inherit 7.1.4

Twelve page-153 candidates are exported as §7.1.4 User-supplied tags. The
first six have full prefix
`requirement-candidate-content-clauses-07-ownership-management-page-153-`
and suffixes `l15-4`, `l30-9`, `l36-11`, `l45-14`, `l60-19`, and `l75-24`.
They continue §7.1.2.2 Acquisition from page 152, before the §7.1.3
heading on page 153. The next six, with suffixes `l84-26`, `l87-27`,
`l96-30`, `l99-31`, `l108-34`, and `l111-35`, are §7.1.3 privilege-to-delete
text before the §7.1.4 heading. Only page-153 `l123-38` and `l129-40`
belong to §7.1.4. Mapping by the exported subsection would falsely
attribute acquisition and privilege evidence to tag behavior. Keep the
pinned IDs and apply a local canonical-section crosswalk. No existing plan
row selects these twelve IDs; the separately covered page-154 `l66-21`
§7.1.4 mapping is correct. No Lab resync was performed; this is an
attribution defect, not IEEE errata or a conformance finding. The
canonical PDF was not independently checked in this turn.

### RL-332 — Page-180 Delete Object Instance order rule is split mid-sentence

The §8.1.4 Delete Object Instance preferred-order rule is divided between
candidate `requirement-candidate-content-clauses-08-time-management-page-180-l87-26`,
which ends at “Delete Object Instance service”, and candidate
`requirement-candidate-content-clauses-08-time-management-page-180-l93-28`,
which begins with the continuation “invocation, the preferred order type...”.
Neither candidate alone gives a clean, complete statement of the rule. The
pinned source text confirms one sentence: sent Delete Object Instance message
order is based on the specified instance's `HLAprivilegeToDeleteObject`
preferred order. The existing process C++ case directly exercises both the
default TIMESTAMP path and changing that attribute to RECEIVE, but the current
traceability mapping selects only the sentence-start candidate to avoid
counting the split twice.

Workaround: retain the candidate IDs and map the evidence to the sentence-start
candidate, with this issue as the continuation pointer; do not count both
fragments as independent normative coverage. No Lab resync was performed. This
is a Lab extraction/usability defect, not IEEE errata or a conformance finding.
The current local process test rerun failed during endpoint connection setup
before behavior assertions, so this turn does not claim fresh behavioral
verification. Source: pinned Lab candidate records and the source text in
`../Document-Recreation/topics/ieee/hla-1516.1-2025/content/clauses/08-time-management-page-180.tex`.

### RL-333 — Page-192 Time Constrained Enabled requirement is an incomplete fragment

Candidate `requirement-candidate-content-clauses-08-time-management-page-192-l69-19`
is exported with only the trailing text “of the joined federate. If the joined
federate is time-regulating, the argument shall equal the joined federate’s”.
The source page continues on the next layout block with “current logical time.”
This leaves the normative postcondition incomplete in both the requirement
query and the candidate record, obscuring the exact behavior unless the reader
opens the page-level content block and reconstructs the sentence.

Workaround: keep the stable candidate ID and map the focused C++ case to
§8.6.3, using the complete source-page sentence as context. Do not resync the
unchanged Lab. This is a Lab extraction/usability defect, not IEEE errata or a
conformance finding. Source: pinned `semantic/requirements.json` candidate and
the page-192 layout text in
`../Document-Recreation/topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`.

### RL-334 — Page-192 smallest-feasible-time requirement is an incomplete fragment

Candidate `requirement-candidate-content-clauses-08-time-management-page-192-l72-20`
ends at “for which all other constraints necessary to” and omits the remainder
of the smallest-feasible-logical-time condition. The full page-level Time
Constrained Enabled block carries additional constraints and delivery behavior
that the candidate query does not show.

Workaround: preserve the stable candidate ID and map focused behavior only to
§8.6.3 after checking the complete page-level source. Do not resync the
unchanged Lab or infer omitted detail from the fragment. This is a Lab
extraction/usability defect, not IEEE errata or a conformance finding. Source:
pinned `semantic/requirements.json` and page-192 sequence 192 in
`../Document-Recreation/topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`.

### RL-335 — Page-197 Time Advance Grant result statement is split mid-sentence

Candidate `requirement-candidate-content-clauses-08-time-management-page-197-l98-29`
ends at “or to the”. The following candidate,
`requirement-candidate-content-clauses-08-time-management-page-197-l104-31`,
begins “specified logical time if no TSO messages were delivered” and then
continues into separate no-future-TSO and GALT grant conditions. The page-197
source block presents one complete Time Advance Grant result sentence followed
by those additional requirements; neither exported fragment is safe to read as
the full surrounding rule in isolation.

Workaround: retain the stable IDs, use the full page-197 source block as
context, and map grant-time evidence to the directly relevant candidate without
counting the continuation candidate as an independent duplicate obligation.
The new focused NMR case covers the no-TSO branch; the existing received-TSO
case covers the delivered-message branch. This is a new occurrence of the
Lab's sentence-fragmentation defect (compare RL-332), not IEEE errata or a
conformance finding. No Lab resync was performed. Source: pinned
`semantic/requirements.json` and page-197 sequence 197 in
`../Document-Recreation/topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`.

### RL-337 — Page-200 Flush Queue Request delivery and grant rule is fragmented

The pinned §8.12 candidates with prefix
`requirement-candidate-content-clauses-08-time-management-page-200-`
split the delivery sentence across `l131-38`, `l134-39`, and `l140-41`.
The first ends at “the joined federate”; the second begins “will receive”
and ends at “smaller timestamps”; the third begins “could arrive” and
introduces the smallest-of grant rule but does not contain its three list
items. The optimistic-logical-time sentence is likewise split between
`l164-49` (ending “advancing the”) and `l170-51` (its tail). The full local
2025 semantic §8.12 page-200 block, lines 33–44, presents the continuous
delivery paragraph, the grant minimum of supplied time, GALT, and the
smallest timestamp delivered in response, and the complete optimistic-time
paragraph.

An isolated candidate query therefore cannot safely establish the full FQR
delivery or grant-value obligation. Keep the stable IDs and pinned export;
read these fragments as a group with the complete semantic page and its
page-201 continuation, and map only the tested subclaim. In particular, the
fragmentation does not justify inventing a mandatory cutoff for TSO input
arriving between request acceptance and an HLA_EVOKED grant callback. This
is a new Lab candidate-extraction/usability occurrence, not IEEE errata or
conformance evidence. The canonical PDF was not independently checked and
no Lab resync was performed. Source: pinned `semantic/requirements.json`
and `output/semantic/content-block-content-15161-page-200.tex` in the
2025 Requirements Lab topic.

### RL-338 — local directed-interaction map used Delete Object Instance §6.16 for Send Directed Interaction

The pinned candidate
`requirement-candidate-content-clauses-06-object-management-page-129-l123-34`
is a valid Delete Object Instance requirement, but the local changed-lookahead
directed-interaction contract assigned it to `sendDirectedInteraction`. The
native case invokes no Delete Object Instance service. This was a local plan /
contract mapping error, not a defect in the pinned candidate.

Resolution: removed the unrelated candidate from the focused Catch2 plan row
and local requirements contract. Retained the known-target requirement in
§5.1.5 and the directly exercised time-management mappings in the portable
C++/Java catalogs. The Query Lookahead mapping correction is tracked under
RL-336. The pinned Lab export was not changed or resynchronized.

Source: `cpp/tests/timestamped_directed_interaction_regulation_reenable_changed_lookahead_catch2.cpp`;
`compliance/requirements-lab/catch2-test-plan.json`; and
`compliance/requirements-lab/timestamped-directed-interaction-regulation-reenable-changed-lookahead-requirements-contract.json`.

### RL-339 — Page-203 time-management candidates inherit §8.14.3 across service boundaries

The pinned candidates
`requirement-candidate-content-clauses-08-time-management-page-203-l47-12`,
`...-l95-25`, `...-l104-28`, and `...-l113-31` are all exported as
**§8.14.3 Preconditions**. The complete page-203 source block places l47 under
the preceding **§8.13.4 Postconditions** heading (pending lookahead after
Flush Queue Grant), while l95, l104, and l113 are in the Time Advance Grant
result paragraph immediately following the **§8.14 service** heading—not in
its §8.14.3 preconditions.

Impact: a §8.14.3 gap query groups unrelated FQG and Time Advance Grant
obligations under the wrong subsection, making correct test mappings easy to
misstate. This is a new clause-boundary issue; preserve all stable Lab IDs and
the exported label, but map evidence to the source-heading clause: l47 to
§8.13.4, and l95/l104/l113 to §8.14. Do not edit or resynchronize unchanged
Lab inputs.

Reproduce with
`python tools/query_rti_work.py gaps --document hla-1516.1-2025 --family time-save-restore --clause clause-8.14 --limit 8 --compact`;
compare the records in the pinned `semantic/requirements.json` with
`output/semantic/content-block-content-15161-page-203.tex` in the 2025 Lab
topic. This is a Requirements Lab extraction defect, not IEEE errata or a
conformance finding. No Lab resync was performed.

### RL-340 — Page-190 time-regulation candidates inherit §8.4.3 across service boundaries

The pinned candidates
`requirement-candidate-content-clauses-08-time-management-page-190-l64-15`,
`...-l67-16`, `...-l76-19`, and `...-l106-26` are all exported as
**§8.4.3 Preconditions**. The complete page-190 source places l64, l67, and
l76 in **§8.3.4 Postconditions** for Enable Time Regulation: the role-enable
callback, permission to send TSO, the logical-time/lookahead state, and the
constrained-recipient timestamp cutoff. The related candidate
`requirement-candidate-content-clauses-08-time-management-page-190-l103-25`
is also misattributed to §8.4.3 and belongs to the §8.4 Disable Time Regulation
service narrative. The source places l106 in the narrative for
the following **§8.4 Disable Time Regulation service**: subsequent messages
from the joined federate are sent as RO. The actual §8.4.3 preconditions begin
after that service heading and do not contain these statements.

Impact: a §8.4.3 gap query directs implementation and traceability toward the
wrong service. Preserve the stable candidate IDs, but map source-backed
Enable Time Regulation evidence to §8.3.4 and Disable Time Regulation evidence
to §8.4; do not claim that these candidates establish §8.4.3 preconditions.
This is a new recurrence of the Lab's clause-boundary extraction defect
(compare RL-339), not IEEE errata or a conformance finding. No Lab resync was
performed. Source: pinned `semantic/requirements.json` and
`output/semantic/content-block-content-15161-page-190.tex` in the 2025
Requirements Lab topic.

### RL-341 — Page-198 §8.11 candidate is split across a page boundary

The pinned candidate
`requirement-candidate-content-clauses-08-time-management-page-198-l192-57`
exports only the fragment that the RTI shall not guarantee delivery of all
messages timestamped `T` before a Time Advance Grant to `T`, ending with the
word “and”. The source continues on page 199: after receiving the grant to
`T`, a federate with zero lookahead can send additional messages timestamped
`T`.

Impact: the page-198 candidate is an incomplete statement and should not be
turned into a test that asserts either delivery or non-delivery of equal-`T`
messages before the grant. Preserve the candidate ID, but assess the complete
cross-page passage before creating behavioral mappings. Existing equal-time
delivery observations do not prove this non-guarantee. This is a source
fragment/extraction issue, not IEEE errata or a conformance finding. No Lab
resync was performed. Source: pinned `semantic/requirements.json` and the
page-198/page-199 source blocks in the 2025 Requirements Lab topic.

### RL-342 — Page-192 TCE service narrative is attributed to §8.6.3

The pinned candidates
`requirement-candidate-content-clauses-08-time-management-page-192-l72-20`,
`...-l78-22`, `...-l84-24`, `...-l93-27`, and `...-l99-29` are reported by
the work query as **§8.6.3**. The full page-192 source places these statements
in the narrative immediately after the **§8.6 Time Constrained Enabled
service** heading and before the §8.6.1 supplied-arguments heading. They are
service-level behavior, not §8.6.3 preconditions.

Impact: mappings for the page-192 service narrative must use §8.6. Existing
time-constrained-enable mappings that use §8.6.3 should be corrected when
their test evidence is updated. This is a new recurrence of the Lab's clause
boundary attribution problem; it also corrects the earlier RL-334 workaround,
which directed mappings to §8.6.3. Preserve all stable candidate IDs and do
not resync unchanged Lab inputs. Source: the exact pinned requirement queries
and `output/semantic/content-block-content-15161-page-192.tex` in the 2025
Requirements Lab topic.

### RL-353 — Order-control test mappings claimed untested ownership and default branches

The pinned Catch2 plan mapped the process-endpoint object-registration case and
the embedded order-type-control case to ownership-transfer reset
(`requirement-candidate-content-clauses-08-time-management-page-212-l54-11`)
and new-owner default initialization
(`requirement-candidate-content-clauses-08-time-management-page-213-l76-20`).
Neither test performs an attribute ownership transfer. The process-endpoint
case only checks that Change Default Attribute Order Type and Change Attribute
Order Type calls succeed; it does not observe an order-dependent update. The
focused order-control case does observe prospective defaults, instance order,
and later interaction order, but has one publisher and does not validate
cross-federate defaults, invalid class/attribute arguments, or initial FDD
interaction order.

Impact: remove these unsupported direct requirement mappings from the two
Catch2 rows; keep the §8.24.4 ownership-transfer reset and §8.25.3 new-owner
default requirements open until a focused official-API behavioral test exists.
Audit the parallel `order-type-control-requirements-contract.json` test links
before using that contract as evidence. This is a local traceability defect,
not IEEE errata or a provider-conformance finding. No Requirements Lab resync
was performed. Sources: `cpp/tests/order_type_control_catch2.cpp` and
`cpp/tests/ieee1516_2025_connection_object_registration_catch2.cpp`.
