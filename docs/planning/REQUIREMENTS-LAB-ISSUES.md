# Requirements Lab issue ledger

This note retains the 2026-10-24 issue history for the IEEE 1516.1-2025 Requirements Lab export. Umbra now pins the reviewed Lab backflow revision: 84 Lab-content complaints and four Umbra-local complaints are resolved. RL-369 is dispositioned for mapping, although the supplied Table 5 and official MIM names conflict. Historical symptoms and workarounds below describe the old pin; the machine-readable ledger carries current status and acceptance evidence.
Historical observations such as RL-030 remain in
`docs/testing/REQUIREMENTS-LAB-OBSERVATIONS.md`; this ledger retains the original issue descriptions and records current status in `known-issues.json`.

The machine-readable ledger is
[`compliance/requirements-lab/known-issues.json`](../../compliance/requirements-lab/known-issues.json).
For a concise, current work queue of every open record, start with the
[consolidated open-issues index](REQUIREMENTS-LAB-OPEN-ISSUES.md). It highlights
the multi-candidate statement-fragmentation complaints and keeps local
Umbra plan/traceability findings separate from Lab-content issues.
Use the bounded query from the repository root:

```powershell
python tools/query_rti_work.py lab-issues --summary --compact
python tools/query_rti_work.py lab-issues RL-381 --summary --compact
```

## Policy

- The pinned 2025 export remains the requirements source of truth.
- Do not resynchronize merely because a query exposes a defect; resynchronize
  only when the pinned export changes.
- A recurrence after the existing issue range gets a new issue id rather than
  being folded into an older entry.
- Keep both the exported Lab clause and the reviewed normative clause visible
  until the upstream extraction is corrected.

## Ledger completeness note — 2026-10-24

The 2026-10-24 register contained 139 records, 89 then open. Its 110 prose headings included all 89 open records. The prose-only RL-336 and RL-338 entries remain local mapping notes. The reviewed Lab revision is now pinned; the [queue index](REQUIREMENTS-LAB-OPEN-ISSUES.md) retains the original complaints and the machine-readable `known-issues.json` reports no open mapping complaints (RL-369 is a reviewed source conflict).

## Reviewed source conflict

RL-369 is dispositioned as a reviewed source conflict. The Table 5 candidate retains its printed spelling; the MIM type-20 service report uses its own spelling. Do not use the disputed Table 5 candidate as direct type-20 evidence.

## 2026-10-24 issue history

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
`#1910/#1911` pass 4/4; strict-less-than and later-TAR cases #514/#531 and all 26
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
`#2110/#2111` pass, as do both focused behavior cases (#485 and #226).
Requirement, subsection, and API mappings are unchanged; no pinned Lab export
or IEEE 1516.1-2010 source or mapping changed.

### RL-285 — ownership contracts retained pre-split source and test pointers

The regular-acquisition and negotiated-divestiture traceability gates found
registry symbols still attributed to `federation_registry.cpp` and service-
report or release-denied test selectors still naming the federation-
management monolith after those cases had been extracted.

Resolution: corrected the local implementation-source paths and selectors in
the four affected ownership contracts. Gates #2043/#2044/#2051/#2052 pass,
alongside connection-loss and support-switch gates `#1910/#1911`/#1990/#1992;
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
- `requirement-candidate-content-clauses-08-time-management-page-194-l45-11`:
  exported §8.8.3; the complete callback-routing statement for Receive
  Interaction, Reflect Attribute Values, and Remove Object Instance also
  occurs in the §8.8 service description before §8.8.1, so its canonical
  location is §8.8. This is an additional affected candidate in the same
  unresolved page-boundary cluster, not a recurrence of a previously fixed
  RL-328 mapping.
- `requirement-candidate-content-clauses-08-time-management-page-206-l29-4`
  and `...page-207-l112-28`: exported §8.18.1 and §8.19.3; the Query GALT
  and Query LITS result rules belong to §8.17 and §8.19 respectively.
- `requirement-candidate-content-clauses-10-support-services-page-286-l20-3`:
  exported §10.60.6; the pending-callback rule is explicitly under the
  §10.59.4 Postconditions heading, before §10.60 begins.

Several of these candidates are also sentence fragments. Workaround: do not
copy the Lab's later-subsection attribution into a test mapping. Preserve the
pinned IDs and use a local parent-section override only when the full 2025
source location and tested behavior support it; do not treat a partial
runtime assertion as coverage of an entire service rule. This is a Lab
attribution issue, not an IEEE erratum or a conformance finding; no Lab
resynchronization was performed.

### RL-341 — Update-region ownership-loss postcondition is attributed to Register Object Instance

The candidate
`requirement-candidate-content-clauses-09-data-distribution-management-page-229-l20-5`
states that an update-region association is lost when a joined federate loses
ownership and is not restored on later reacquisition. The semantic page places
the paragraph under **§9.6 Associate Regions For Updates**, but the candidate
is exported under **§9.5.4 Postconditions for Register Object Instance With
Regions**.

Impact: gap and trace queries assign an ownership-sensitive DDM rule to the
wrong service family, which can misdirect implementation and make section-level
coverage misleading.

Workaround: preserve the immutable requirement ID and use the page-230 §9.6
service heading when reviewing source-backed test traceability. Do not change
the pinned export or infer a conformance defect.

Source: `topics/ieee/hla-1516.1-2025/output/semantic/content-block-content-15161-page-229.tex`
and `content-block-content-15161-page-230.tex`.

Reproduction: run
`python tools/query_rti_work.py requirement requirement-candidate-content-clauses-09-data-distribution-management-page-229-l20-5 --compact`
and compare the candidate with both complete page blocks.

Detected 2026-10-16. This is a Requirements Lab page-boundary extraction
defect, not IEEE errata or a conformance finding. No Lab resynchronization was
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
fragments as independent normative coverage. The official C++ matrix now covers
all eight Delete Object Instance order-classification rows and maps this rule
once to l87; l93 is not counted separately. Focused MinGW CTest passed five
repeats with 97 assertions. Existing configured-process timestamped deletion
NMR/NMRA tests passed 2/2 with local loopback enabled. The source sentence is
still split by the Lab export, so this remains an extraction/usability defect,
not IEEE errata or a conformance finding. No Lab resync was performed. Source:
pinned Lab candidate records and the source text in
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

### RL-386 — Page-198 §8.11 candidate is split across a page boundary

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

Ledger correction: this issue was previously labeled RL-341 in this Markdown
file, but the structured issue ledger already assigns RL-341 to a distinct
page-229/230 §9.6 crosswalk defect. The structured ID remains authoritative;
this page-198 issue is recorded as RL-386, with no change to pinned Lab inputs.

### RL-351 — Page-182 §8.1.5 time-management rules are exported only under Clause 8

The pinned candidates
`requirement-candidate-content-clauses-08-time-management-page-182-l7-1` and
`requirement-candidate-content-clauses-08-time-management-page-182-l88-28`
state, respectively, that only time-regulating federates may send TSO messages
and that same-producer TSO messages are received in timestamped order except
when Flush Queue Request is used. The page-182 source places both statements
under **§8.1.5 Time-regulating joined federates**, but the Lab exports them only
under broad Clause 8.

Impact: clause-only export hides the precise subsection and leaves a
crosswalk-review warning even when source-backed §8.1.5 witnesses exist.
Preserve both stable IDs and broad exports, and map local evidence to §8.1.5.
The portable C++ regulation-transition witness is limited to timestamped
interactions: before regulation the timestamped overload is received as RECEIVE
order without a retraction designator, and after regulation it is received as
TSO. The separate ordering witness is limited to same-producer interactions
observed by two constrained receivers; neither establishes attribute/deletion
coverage or the Flush Queue Request exception. This is a Requirements Lab
subsection-attribution defect, not IEEE errata or conformance evidence. No Lab
resync was performed. Source: page 182 in the pinned 2025 Requirements Lab
topic.

### RL-352 — Page-182 Enable Time Constrained transition is exported only under Clause 8

The pinned candidate
`requirement-candidate-content-clauses-08-time-management-page-182-l100-31`
states that a joined federate requests time-constrained status through Enable
Time Constrained and the RTI subsequently invokes Time Constrained Enabled.
The page-182 source places this transition under **§8.1.6 Time-constrained
joined federates**, but the Lab exports it only under broad Clause 8.

Impact: the broad export hides the exact subsection, so public C++ tests can
exercise the request/callback behavior while subsection-level gap queries fail
to show that coverage. Preserve the stable ID and map source-backed evidence
to §8.1.6. This is a Requirements Lab subsection-attribution defect, not IEEE
errata or conformance evidence. No Lab resync was performed. Source: page 182
in the pinned 2025 Requirements Lab topic.

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

Impact: unsupported direct mappings from the two Catch2 rows were removed; the
§8.24.4 ownership-transfer reset and §8.25.3 new-owner default requirements
remain open until a focused official-API behavioral test exists. The parallel
`order-type-control-requirements-contract.json` is audited alongside the plan.
This is a local traceability defect, not IEEE errata or a provider-conformance
finding. No Requirements Lab resync was performed. Sources:
`cpp/tests/order_type_control_catch2.cpp` and
`cpp/tests/ieee1516_2025_connection_object_registration_catch2.cpp`.

Resolution (2026-10-22): the contract's stale test selectors were updated to
the current focused case, and its direct mappings to the incomplete page-213
fragments were removed. The order-type requirements contract check passes;
the focused C++ test remains mapped only to assertions supported by intact
requirements. Ownership-transfer and new-owner-default behavior remain
unmapped and open, rather than being treated as completed by this resolution.

### RL-354 — Time Advance Grant case claimed a Time Constrained Enabled argument rule

The Catch2 row `umbra-cpp-time-advance-grant-requested-logical-time` selected
`requirement-candidate-content-clauses-08-time-management-page-192-l66-18`
under §8.6.3, but its assertions inspect a Time Advance Grant and a
Query Logical Time result, not the Time Constrained Enabled argument. The
page-192 requirement concerns the new logical time conveyed by Time
Constrained Enabled. The adjacent regulating-federate case does inspect that
callback argument after advancing to time five and compares it with Query
Logical Time; it also covers the page-192-l69-19 continuation tracked by
RL-333.

Impact: move the §8.6.3 candidate to the regulating-federate Catch2 row and
its already-green portable C++/Java parity, leaving the grant case mapped
only to its §8.14 result rule. This is a local traceability correction, not
IEEE errata or a new provider-conformance claim. No Requirements Lab resync
was performed. Source: `cpp/tests/available_time_advance_inclusive_galt_catch2.cpp`.

### RL-355 — Table 5 enum rows are mapped to API-result and private-formatter tests

The Lab candidates `requirement-candidate-table-5-p295-r001` through `r003`
represent the `AdditionalSettingsResultCode` type and its enumerators. Their
semantic records classify them as cross-cutting wire-shape rows and explicitly
say that the table structure is not itself a callable API mapping. Nevertheless,
the Catch2 plan attaches them to Connect cases such as
`umbra-cpp-connect-configuration-fallback-integration` (r001),
`umbra-cpp-connect-additional-settings-parse-failure-integration` (r001/r002),
and `umbra-cpp-embedded-connect-service-report-directory-configuration-integration`
(r001/r003). Those assertions check only the public
`ConfigurationResult.additionalSettingsResult` enumerator; they do not encode
or inspect a service-report record.

The internal unit `umbra-cpp-service-report-table5-configuration-result-encoding-unit`
also selects page-296 Table 5 requirement IDs, while its own plan status says
that it does not test HLA record encoding and is not conformance evidence. The
installed-package probe passed both callback models, but the joined-federate
`HLAreportServiceFile` files contained no Connect record or
`additionalSettingsResultCode` field; Connect occurs before the joined-federate
reporting scope. Keep the API-result assertion as supporting evidence and the
formatter unit as internal implementation evidence, not direct Table 5
requirement mappings.

Impact: the Catch2 traceability plan overstates §11.5.1/Table 5 coverage if
these API-result and private-formatter checks are counted as normative wire
shape evidence. This is a traceability and evidence-classification issue, not
IEEE errata or a provider-conformance finding. No Requirements Lab resync was
performed. Sources: `topics/ieee/hla-1516.1-2025/semantic/requirements.json`,
`cpp/tests/ieee1516_2025_connection_configuration_fallback_catch2.cpp`,
`cpp/tests/service_report_store_catch2.cpp`, and the public-file probe recorded
in `docs/planning/ROADMAP-INDEX.json`.

Umbra follow-up — 2026-10-21: the Catch2 plan now leaves r001-r003 unmapped;
Connect enum checks are supporting API evidence only, and the private
formatter witness is explicitly classified as internal and non-conformance
evidence. This resolves the local traceability overclaim. The underlying Lab
extraction issue remains separately tracked as RL-358.

### RL-356 — Page-175 logical-time requirement is truncated and clause-only

The pinned candidate
`requirement-candidate-content-clauses-07-ownership-management-page-175-l131-40`
ends mid-sentence at “Federation executions requiring multiple time
representations within a”. The page-175 semantic content continues with the
single-execution/TRADT requirement (including a time-type discriminator
example) and the requirement that the RTI support the standardized time
representations in Table 21. The source places this paragraph under
**§8.1.1 Logical Time**, but the candidate is exported only under broad
Clause 8. This is a recurrence of the subsection-attribution problem recorded
for other time-management paragraphs, plus a separate content-truncation
defect; it receives a new issue number rather than rewriting those records.

Impact: the incomplete candidate conceals both the remainder of the normative
statement and the scope boundary between the RTI's standardized time support
and a federation's custom multi-representation TRADT. Preserve the stable ID
and repair its source text and §8.1.1 attribution. The existing float64 C++
case is locally mapped only to the standardized HLAfloat64Time declaration and
selection subclaim; it does not establish the custom multi-representation
branch or the FDD documentation recommendation. No Requirements Lab resync was
performed. Sources: the pinned page-175 semantic content and
`cpp/tests/ieee1516_2025_public_float64_time_representation_catch2.cpp`.

### RL-357 — Page-58 time-representation requirement loses §4.8 attribution

The pinned 1516.2 page-58 paragraph
`requirement-candidate-sections-semantic-clause-4b-page-058-l34-8` says
`HLAinteger64Time` and `HLAfloat64Time` are required HLA time representations
and “shall be provided by all RTIs.” The source page is under **§4.8 Time
representation table** (the heading is on page 57), but the candidate in
`semantic/requirements.json` is assigned only to generic Clause 4. The stable
candidate ID is retained; the local Catch2 mapping corrects the subsection to
§4.8 and explicitly records the exact tested subclaim.

Impact: the generic clause obscures a useful RTI implementation requirement
and makes it harder to find the corresponding tests. Separate C++ 2025
official-API cases now exercise Umbra's float64 and integer64 built-in paths,
each mapped to §4.8 with a per-type scope note. They are embedded development-
profile implementation evidence only; they do not establish support by every
RTI, interoperability, custom multi-representation TRADT behavior, or
conformance. Do not treat the full two-type statement as covered by either
case in isolation. No Requirements Lab resync was performed. Sources: the
pinned 1516.2 §4.8 page heading and page-58 text, plus the two focused C++
time-representation cases.

### RL-358 — Table 5 AdditionalSettingsResultCode encoding candidate omits two enumerants

The structured candidate
`requirement-candidate-table-5-p295-r001` ends the
`AdditionalSettingsResultCode` encoding after `SETTINGS_IGNORED`. The adjacent
candidates `requirement-candidate-table-5-p295-r002` and `r003` export
`SETTINGS_FAILED_TO_PARSE` and `SETTINGS_APPLIED` as standalone Table 5 rows
with unspecified encodings. In the Lab's own page-295 source transcription,
Table 5 places all three values in one `Enumeration` under
`AdditionalSettingsResultCode`.

Impact: the structured requirement records do not preserve the full enum wire
shape, making the Table 5 row ambiguous and fragmenting requirement-to-test
traceability. This is distinct from RL-355, which concerns the local Catch2
plan's unsupported mappings to API-result and private-formatter tests.

Preserve the stable candidate IDs and record the complete enum shape in local
analysis without rewriting the pinned export. Confirm against the licensed
primary standard before treating this as IEEE errata or changing the Lab
source. No Requirements Lab resync was performed. Source: the pinned Lab's
`topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`, page-295
`layout_body` for Table 5, compared with the corresponding records in
`semantic/requirements.json`.

### RL-359 — Commit Region candidate truncates its rule and misattributes the subsection

The structured candidate
`requirement-candidate-content-clauses-09-data-distribution-management-page-219-l50-16`
starts mid-paragraph and stops after “The effects of the Set Range Bounds
service invocations that were made, if any,”. The pinned page-219 source
continues that these effects remain pending and take effect only after Set
Range Bounds has been invoked for every region dimension and Commit Region
Modifications is invoked for that region. The candidate assigns the rule to
§9.1.3.2, but the source places the complete region-template rule under
§9.1.3.1 “Region relationships,” before the visible §9.1.3.2 heading.

Impact: a test derived only from the exported candidate can assert that an
incomplete commit throws `InvalidRegion` while missing the required deferred-
effect behavior and the successful completion path; the exported subsection
also sends requirement-to-test mappings to the wrong canonical clause.

Preserve the stable candidate ID; use the full pinned source paragraph for
local requirement-to-test analysis without rewriting or resynchronizing the
Lab export. This is a Requirements Lab extraction defect, not IEEE errata or
provider-conformance evidence. No Requirements Lab resync was performed.
Source: the pinned Lab's
`topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`, page-219
page-219 `layout_body` and the visible subsection heading, compared with the
candidate in `semantic/requirements.json`.

Umbra follow-up — 2026-10-21: `cpp-tck.region-commit-complete-ranges` maps the
candidate to §9.1.3.1 using the visible source heading and passed focused
MinGW/Ninja CTests and direct runs in both callback models. RL-359 remains open
because the Lab's candidate text and clause attribution are still unchanged.

### RL-360 — Page-220 object-attribute continuation is assigned to the interaction subsection

The page-220 structured candidates for object-attribute update and subscription
relationships are assigned to §9.1.3.3 even though the pinned page layout places
them before the visible §9.1.3.3 “Interaction relationships” heading. The
dimension-subset candidate
`requirement-candidate-content-clauses-09-data-distribution-management-page-220-l8-2`
also starts mid-paragraph after “available dimensions of that”; the source
continues “instance attribute. The resulting region realizations shall have the
same property.” The same subsection-boundary defect affects the page-220
object-attribute candidates at l11-3, l14-4, l17-5, l26-8, l35-11, l41-13,
l50-16, l56-18, l65-21, l77-25, l86-28, l92-30, l95-31, l107-35, and l113-37.
The actual §9.1.3.3 interaction relationships begin only after the visible
heading near the bottom of the page.

Impact: requirement-to-test searches overstate §9.1.3.3 and hide the actual
§9.1.3.2 object-attribute coverage. The split dimension-subset candidate also
omits that the resulting region realization must retain the same property.

Preserve all candidate IDs; map the listed object-attribute statements to
§9.1.3.2 in local analysis using the complete pinned source. Do not rewrite or
resynchronize unchanged Lab inputs. This is a Requirements Lab extraction
defect, not IEEE errata or provider-conformance evidence. No Requirements Lab
resync was performed.
Source: the pinned Lab's
`topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`, page-220
`layout_body` and visible subsection heading, compared with the candidate in
`semantic/requirements.json`.

Umbra follow-up — 2026-10-21: `cpp-tck.regional-object-attribute-dimension-subset`
maps the selected dimension-subset behavior to §9.1.3.2 and passed the
focused MinGW/Ninja CTest matrix and direct runs for HLA_EVOKED and
HLA_IMMEDIATE. RL-360 remains open because the Lab candidates remain truncated
and attributed to §9.1.3.3.

Umbra follow-up — 2026-10-22: the exact crosswalk query reconfirmed that
candidate `l8-2` is exported as §9.1.3.3 while local analysis maps the complete
object-attribute rule to §9.1.3.2; the verbose `tck-gaps` view now exposes this
discrepancy. This is further evidence for RL-360, not a separate issue or a
recurrence after an upstream fix. The pinned Lab input is unchanged, so no
resynchronization was performed.

### RL-361 — Interaction dimension-subset rule is split and conflated with default-region behavior

The page-220 candidate
`requirement-candidate-content-clauses-09-data-distribution-management-page-220-l140-45`
ends mid-sentence after “a subset of the”. Its continuation,
`requirement-candidate-content-clauses-09-data-distribution-management-page-220-l143-46`,
begins “available dimensions of the sent interaction” and then includes the
separate default-region rule. The pinned page layout places both statements
under the visible §9.1.3.3 “Interaction relationships” heading.

Impact: a test mapped only to l140-45 can miss the end of the dimension rule;
mapping all of l143-46 to that test would overstate coverage of the separate
default-region behavior.

Preserve both candidate IDs. For local traceability, map the send-region
dimension-subset check to §9.1.3.3 using the complete sentence in the pinned
source, and track the default-region statement separately. Do not rewrite or
resynchronize unchanged Lab inputs. This is a Requirements Lab extraction
defect, not IEEE errata or provider-conformance evidence. No Requirements Lab
resync was performed.
Source: the pinned Lab's
`topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`, page-220
`layout_body` after the visible §9.1.3.3 heading, compared with the candidates
in `semantic/requirements.json`.

### RL-362 — Page-192 TCE service narrative is attributed to §8.6.3

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

### RL-363 — Stale C++ API surface identifiers caused Catch2 plan validation failures

Five entries in `catch2-test-plan.json` referenced API surface IDs absent from
the pinned 2025 bundle: four TARA/process TSO entries contained an extra
`avance` in the `timeAdvanceRequestAvailable` ID, and the per-federate
discovered-class entry had a stale `getKnownObjectClassHandle` hash. The
corresponding C++ API surfaces are present in the bundle under their correct
IDs, so this was local plan drift—not a Requirements Lab content omission.

The five references now use the IDs from `.compliance/corpus-bundle.json`.
`python tools/requirements_lab.py check-plan` passes for all 1,755 entries,
1,176 requirement IDs, and 243 C++ API surfaces. RL-363 is resolved in Umbra;
no Lab input was changed or resynchronized.

### RL-364 — Table 5 ConfigurationResult candidate omits its record shape

The pinned candidate `requirement-candidate-table-5-p296-r001` describes the
Table 5 `ConfigurationResult` row with encoding only `Record<` and example
`unspecified`. The Requirements Lab's page-296 layout contains the fields
`configurationUsed:Boolean`, `addressUsed:Boolean`,
`additionalSettingsResultCode:AdditionalSettingsResultCode`, and
`message:String>`, plus an example object. That example's misspelled key is
covered separately by RL-256; this issue tracks the candidate's missing
record fields and example disposition.

Impact: the structured candidate does not give contributors enough information
to reproduce or audit the `ConfigurationResult` record shape, and a truncated
candidate can appear covered without a meaningful wire-format check.

Preserve the stable candidate ID. Use the complete pinned page layout for local
analysis while treating the example spelling under RL-256 as unresolved; do
not claim the incomplete candidate establishes complete conformance coverage.
This is an internal Requirements Lab extraction defect, not IEEE errata or
provider-conformance evidence. No Requirements Lab resync was performed.
Source: `topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`,
`content-15161-page-296` `layout_body`, compared with the candidate in
`semantic/requirements.json`.

Reproduction: `python tools/query_rti_work.py requirement requirement-candidate-table-5-p296-r001 --summary --compact`.

Umbra follow-up — 2026-10-21: the official C++ binding exposes
`ConfigurationResult` only as the return value of the `connect` overloads.
The Table 5 `ServiceReportInitialRecord` type contains `RtiConfiguration`,
`HLAmanager.HLAfederation`, and `HLAmanager.HLAfederate`, not
`ConfigurationResult`; the private `formatMomConfigurationResult` function
has no runtime caller and is exercised only by an internal unit test. No
public service-report path for this encoding was identified, so p.296 r001
remains unmapped and the formatter test remains non-conformance evidence.
This is a testability/evidence boundary, separate from the Lab extraction
defect; do not add a Connect result to the initial record without normative
support.

### RL-365 — Table 5 FactoryClass enumeration is fragmented across candidates

The pinned candidate `requirement-candidate-table-5-p296-r011` stops its
`FactoryClass` encoding after `Enumeration<ATTRIBUTE _HANDLE _FACTORY` and
renders the example with spaces inside the enumerant. Candidates r012-r028
then export the remaining enumerants as standalone rows with unspecified
encodings and examples. The page-296 layout presents the complete set as one
`FactoryClass` enumeration.

Impact: the structured candidates lose the association between the type and
its complete enum domain, while the spaced tokens can be mistaken for literal
enumerant names. This is a recurrence of the Table 5 enum-fragmentation defect
tracked separately for `AdditionalSettingsResultCode` in RL-358; preserve this
new issue number rather than rewriting that record.

Preserve all candidate IDs and use the complete pinned page layout for local
analysis. Do not map each standalone enumerant as an independently encoded
type, claim IEEE errata, or count private formatting tests as public C++ TCK
coverage. No public C++ API or runtime service-report use of `FactoryClass`
was identified in the inspected binding/runtime sources, so these rows remain
unmapped pending a standards-backed public path. No Requirements Lab resync
was performed.

Source: `topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`,
`content-15161-page-296` `layout_body`, compared with candidates
`requirement-candidate-table-5-p296-r011` through `r028` in
`semantic/requirements.json`.

Reproduction: query p.296 r011-r028 with `python tools/query_rti_work.py requirement <candidate-id> --summary --compact`
and compare the results with the FactoryClass row in the page-296 `layout_body`.

### RL-366 — Table 5 FederateAttributes record is split and column-shifted

The pinned candidate `requirement-candidate-table-5-p296-r030` stops the
`FederateAttributes` encoding after its first field. Candidates r031-r034 then
export the remaining fields as standalone rows, with their encodings and
examples shifted across candidate columns. The page-296 layout shows one
record containing `HLAfederateHandle`, `HLAfederateName`,
`HLAfederateType`, `HLAfederateHost`, and
`HLAFOMmoduleDesignatorList`, with a single example object.

Impact: the structured requirements do not preserve the `FederateAttributes`
record shape or its field/value associations, so a contributor can encode an
invalid initial service-report record or overstate test coverage.

Preserve the candidate IDs and use the complete page layout for local analysis;
do not treat r031-r034 as independent top-level Table 5 types. This is an
internal Requirements Lab extraction defect, not IEEE errata or
provider-conformance evidence. No Requirements Lab resync was performed.

Source: `topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`,
`content-15161-page-296` `layout_body`, compared with candidates
`requirement-candidate-table-5-p296-r030` through `r034` in
`semantic/requirements.json`.

Reproduction: query p.296 r030-r034 with `python tools/query_rti_work.py requirement <candidate-id> --summary --compact`
and compare with the `FederateAttributes` row in the page-296 `layout_body`.

Umbra follow-up — 2026-10-21: §11.5.2.1 places `FederateAttributes` in
`ServiceReportInitialRecord`, making the row publicly observable through the
joined federate's `HLAreportServiceFile`. The native C++ Catch2 case
`umbra-cpp-process-endpoint-joined-federate-mom-report-file-integration` now
asserts the joined handle, federate name/type, non-empty host, and empty Join
FOM-module array, mapped directly to p.296 r030 / §11.5.1 and p.294 l20-6 /
§11.5.2.1. This is development-profile evidence only. The portable C++ TCK
still has 0/1 direct coverage for r030; r031-r034 remain unmapped as separate
types. The Lab extraction defect remains open, and no resynchronization was
performed.

### RL-367 — Table 5 FederateHandleSaveStatusPair record is split and column-shifted

Candidate `requirement-candidate-table-5-p296-r036` truncates the
`FederateHandleSaveStatusPair` encoding after
`Record<handle:FederateHandle` and omits the example. Candidate r037 exports
`status:SaveStatus>` as a standalone row and shifts the example fields into
the encoding. The page-296 layout shows one record,
`Record<handle:FederateHandle,status:SaveStatus>`, with an example object
containing handle `FederateHandle<12>` and status `FEDERATE_SAVING`.

Impact: the structured candidates lose the complete pair shape and associate
the handle/status example with a bogus top-level row, obscuring correct
Table 5 and public API-value coverage.

Preserve both candidate IDs and use the complete pinned page layout for local
analysis. Do not treat r037 as an independently encoded type or claim IEEE
errata. This is a separate Table 5 row defect from RL-366; do not rewrite that
issue or resynchronize unchanged Lab inputs.

Source: `topics/ieee/hla-1516.1-2025/semantic/content-elements.json`,
`content-15161-page-296` `layout_body`, compared with candidates r036-r037 in
`semantic/requirements.json`.

Reproduction: query `requirement-candidate-table-5-p296-r036` and
`requirement-candidate-table-5-p296-r037` and compare their fields with the
`FederateHandleSaveStatusPair` row in the page-296 `layout_body`.

Umbra follow-up — 2026-10-21: the public C++ TCK scenario
`cpp-tck.federation-mom-save-status` observes the `HLAreportServiceInvocation`
for `FederationSaveStatusResponse` and asserts its serialized
`FederateHandleSaveStatusPairSet` argument: type code 17, exact argument name,
array delimiters, and both callback-derived handle/status records. The focused
installed-package MinGW/Ninja CTest and direct runs passed under `HLA_EVOKED`
and `HLA_IMMEDIATE` (2/2); the promoted direct catalog mapping is limited to
r036 / §11.5.1 and the focused gap query reports 1/1. Candidate r037 is not
mapped as an independent type. This remains an internal Requirements Lab
extraction defect, not IEEE errata or conformance evidence; no Requirements
Lab resync was performed.

### RL-368 — Table 5 FederateRestoreStatus record is split and column-shifted

Candidate `requirement-candidate-table-5-p297-r003` truncates the
`FederateRestoreStatus` encoding to `Record<` and omits its example. Candidates
r004-r006 export `preRestoreHandle`, `postRestoreHandle`, and
`status:RestoreStatus>` as standalone rows and shift the example fields into
their encodings. The page-297 layout shows one record containing all three
fields and one example object.

Impact: the structured candidates lose the restore-status record shape and
separate its field/value associations, obscuring the standard
`FederateRestoreStatus` API value.

Preserve r003-r006 and use the complete pinned page layout for local analysis;
do not treat r004-r006 as independent top-level types or claim IEEE errata.
This is separate from RL-366 and RL-367; do not rewrite those records or
resynchronize unchanged Lab inputs.

Source: `topics/ieee/hla-1516.1-2025/semantic/content-elements.json`,
`content-15161-page-297` `layout_body`, compared with candidates r003-r006 in
`semantic/requirements.json`.

Reproduction: query `requirement-candidate-table-5-p297-r003` through r006 and
compare their fields with the `FederateRestoreStatus` row in the page-297
`layout_body`.

Umbra follow-up — 2026-10-21: the existing public C++ TCK scenario
`cpp-tck.restore-object-snapshot-status` now selects the public
`FederationRestoreStatusResponse` `HLAreportServiceInvocation` and asserts the
serialized type-20 argument, exact argument name, array framing, and all three
callback-derived record fields. Focused installed-package MinGW/Ninja CTest
and direct execution passed under `HLA_IMMEDIATE`; promoted catalog evidence
validation is valid and the focused gap query reports r003 at 1/1. Candidates
r004-r006 remain unmapped as independent types. RL-369 separately tracks the
p.297 r007 collection-name/MIM discrepancy and the formatter's prior
`RL-083` citation, which now points to RL-369. Do not map r007 from the
type-20 report until the discrepancy is dispositioned. This remains an
internal Requirements Lab extraction defect,
not IEEE errata or conformance evidence. No Requirements Lab resync was
performed.

### RL-369 — Table 5 restore-status set name conflicts with the MIM; prior RL-083 citation

The pinned page-297 Table 5 row is transcribed as
`FederateHandleRestoreStatusSet` with encoding
`Array<FederateHandleRestoreStatus>`. The pinned IEEE 1516.2-2025 MIM instead
names `FederateRestoreStatus` as HLAargumentType 19 and
`FederateRestoreStatusSet` as type 20; it has no `FederateHandleRestoreStatus`
MIM identifier. The page-layout continuation also fragments the example
object's field syntax. During discovery, formatter comments referred to
`RL-083`, which had no matching issue ID. Current source comments cite
`RL-369`; the `lab-issues RL-083` search returns full-text matches, not an
exact-ID result.

Impact: the public `HLAreportServiceInvocation` uses the MIM's type-20 name
while the Table 5 candidate supplies a different collection name. Without a
disposition, mapping r007 or encoding its literal as an `HLAargumentType`
would overstate the evidence. The historical `RL-083` source citation was
untraceable until RL-369 was logged; the source now points to RL-369.

Keep the verified direct mapping limited to singular record candidate r003,
observed nested in the actual type-20 report. Leave r007 unmapped and retain
the MIM-compatible collection spelling unless the licensed clause text
establishes otherwise. This is an unresolved cross-source discrepancy, not
IEEE errata; do not resynchronize unchanged Lab inputs.

Source: `topics/ieee/hla-1516.1-2025/semantic/content-elements.json`,
`content-15161-page-297` `layout_body` and candidate r007, compared with
`third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml`.

Reproduction: query `requirement-candidate-table-5-p297-r007`, compare its
page-297 row and example with the MIM HLAargumentType enumerators, search
`mom_service_report_encoding.hpp/.cpp` for `RL-083` and `RL-369`, and run
`python tools/query_rti_work.py lab-issues RL-083 --summary --compact`;
distinguish returned full-text matches from records whose ID is `RL-083`.

Verification — 2026-10-21: the pinned page layout and exported candidate use
`FederateHandleRestoreStatusSet`, while the MIM assigns type 20 to
`FederateRestoreStatusSet` and type 19 to `FederateRestoreStatus`. Current
formatter comments cite RL-369 and contain no RL-083 reference. The
`lab-issues RL-083` search returns RL-368 and RL-369 because their text mentions
RL-083; neither record has ID RL-083. The prior dangling source citation is
resolved, while the Table 5/MIM discrepancy remains open. This does not claim
the Lab mis-transcribed the licensed standard or that IEEE errata is required.

### RL-370 — Page-181 TSO-order rule is exported only under Clause 8

The pinned candidate
`requirement-candidate-content-clauses-08-time-management-page-181-l148-13`
states that, except when Flush Queue Request is used, TSO messages received by
a given joined federate are received in TSO regardless of producer or send
sequence, with distinct timestamps ordered by timestamp. The page-181 source
places this statement under **§8.1.4 Message order type**, but the Lab exports
it only under broad Clause 8.

Impact: clause-only export hides the precise subsection and creates a
crosswalk review for portable TCK mappings even when source-backed §8.1.4
evidence exists. Preserve the stable ID and broad export, and map local evidence
to §8.1.4 with an explicit issue override. The portable C++ witness is bounded
to the tested interaction ordering sequence; it does not claim the Flush Queue
Request exception, every message family, equal-time order, or every
sender/sequence permutation.

Source: page 181 in the pinned 2025 Requirements Lab topic, compared with the
candidate in `semantic/requirements.json`.

Verification — 2026-10-21: page 181 identifies §8.1.4 Message order type above
the affected statement, while the candidate metadata supplies only Clause 8.
The existing portable C++ primary/contract pair asserts a bounded interaction
ordering witness and records its exclusions. This is a Requirements Lab
subsection-attribution defect, not IEEE errata or conformance evidence. No Lab
resync was performed.

### RL-371 — Connect API traceability row cites unresolvable RL-045

The local Connect API traceability row in
`compliance/requirements-lab/catch2-test-plan.json` says RL-045 records the
aggregate Connect export limitation. No issue with ID RL-045 exists in the
local ledger, and a targeted search of the adjacent Document-Recreation
workspace found no RL-045 reference. The `lab-issues` query is full-text: before
RL-371 was logged it returned zero matches; it now returns RL-371 because that
record documents the dangling pointer. The prior disposition cannot be verified
from the current evidence set.

Impact: the plan relies on an unresolvable issue pointer to explain why the
aggregate Connect API mapping remains unresolved. The exact four official
Connect declaration surfaces remain API-only evidence; they do not establish
an aggregate normative service mapping.

Preserve the historical RL-045 text, and keep the case outside validation,
package, protected-review, and conformance claims until the aggregate
crosswalk is independently resolved. RL-371 does not validate the existing
§4.2.4 mapping or claim a Requirements Lab semantic defect; no Lab resync is
warranted.

Source: `compliance/requirements-lab/catch2-test-plan.json`,
`umbra-cpp-connect-requirement-traceability` row.

Verification — 2026-10-21: no issue with ID RL-045 exists locally and the
targeted adjacent-workspace search returned no matches. The local full-text
query now returns RL-371 as a mention, not as an exact-ID match. This is a
local traceability defect, not an IEEE erratum or conformance evidence.

### RL-372 — Stale RL-083 references recurred in restore-status plan notes

After RL-369 documented the Table 5 r007/MIM discrepancy and the prior
unresolvable RL-083 formatter citation, two active Catch2 plan notes still
described the same discrepancy as logged under RL-083:

- `umbra-cpp-service-report-file-query-federation-restore-status-integration`
- `umbra-cpp-mom-service-report-federation-restore-status-response-unit`

The `lab-issues RL-083` query is full-text: it returns RL-368 and RL-369
because those records mention RL-083, but neither issue has ID RL-083. This is
a recurrence of the local pointer defect, not a new standard or Lab semantic
finding. The Table 5/MIM discrepancy itself remains open under RL-369; r007
stays unmapped.

The two plan notes now distinguish the authoritative semantic issue (RL-369)
from this pointer recurrence (RL-372). RL-372 is resolved in Umbra; no
Requirements Lab resync is warranted.

Source: `compliance/requirements-lab/catch2-test-plan.json`, the two case IDs
above.

Verification — 2026-10-21: the exact-ID query returned no RL-083 record; its
full-text results were RL-368 and RL-369. Both active plan notes now cite
RL-369 for the underlying discrepancy and RL-372 for the stale-pointer
recurrence. This is a local traceability correction, not IEEE errata or
conformance evidence.

### RL-373 — Page-40 federation-lifecycle candidate is truncated and combines distinct obligations

The pinned 2025 Lab candidate
`requirement-candidate-content-clauses-04-federation-management-page-040-l33-8`
is exported as one broad clause-4 requirement over source lines 33–53. Its
statement begins with the mid-sentence fragment “been forced to the Not
Connected state” and ends with the incomplete phrase “no further intent to
join or create federation”. Between those fragments it combines the effect of
the Automatic Resign Directive (referencing §4.1.5), ordinary resignation,
forced departure from the joined state, and Federate Resigned notification.

This extraction is too coarse and textually incomplete for direct test
mapping: an individual callback or membership assertion cannot honestly stand
for all of these different lifecycle obligations. Keep the detailed
`req-federate-connection-lost` requirement mapped to §4.1.1 and test the
Connection Lost callback and post-loss membership separately. Do not count the
broad page-40 candidate as directly covered until the Lab restores the full
passage and splits or classifies its requirements with precise canonical
sections.

The Catch2 plan had selected this broad candidate for
`umbra-cpp-connection-lost-no-action-policy-integration`, although that case
does not assert every combined lifecycle obligation. That selection has been
removed; its independent detailed requirements remain mapped, while the
page-40 candidate is now globally uncovered rather than overstated.

Source: `content/clauses/04-federation-management-page-040.tex`, lines 33–53;
Lab requirement query for the candidate ID above.

Verification — 2026-10-21: the exact requirement query returned the truncated
statement, source range, and broad clause-4 assignment. No matching issue was
present in the existing Markdown or JSON ledger before RL-373 was added. The
Catch2 plan check passes after removing the overbroad selection, and the exact
gap query reports the candidate globally uncovered (1/1). This is a Lab
export/extraction usability defect, not IEEE errata or conformance evidence.
No Lab resync was performed.

## Open structured records reconciled — 2026-10-22

The following entries were open in `known-issues.json` and absent from this
prose ledger. Their IDs and statuses are preserved; exported clause labels
remain provenance, and reviewed subsection references below are local
source-backed crosswalk notes, not edits to the pinned Lab export. Except for
RL-261 (an Umbra plan-file defect), these record Requirements Lab extraction
or traceability defects.

### RL-248 — Page-236 invalid-region precondition has the wrong clause (resolved in Lab)

The Register Object Instance With Regions precondition is exported as §9.10,
although the source heading places it under §9.5.3. Keep the stable ID and
exported label; do not map it to §9.10 or silently rewrite the export.

Source: `content/clauses/09-data-distribution-management-page-236.tex`, lines
32–37 (page 236). Reviewed subsection: §9.5.3.

### RL-249 — Page-109 interaction-receipt statement is corrupted (resolved in Lab)

The candidate loses the federate/subscription notation and ends mid-sentence.
Its requirement ID and §6.1.9 association remain useful, but the text is not a
self-contained test oracle; use the separate received-class definition and
official source context rather than inferring missing words.

Source: `content/clauses/06-object-management-page-109.tex`, line 10 (page
109). Reviewed subsection: §6.1.9.

### RL-250 — Page-224 Receive Interaction rule is split (resolved in Lab)

The line-89 statement ends at “subscribed”; the adjacent line-92 record starts
with its continuation and then includes an at-most-once requirement. Do not
join or map these fragments as if the export provided one complete statement.

Source: `content/clauses/09-data-distribution-management-page-224.tex`, lines
89–100 (page 224). Reviewed subsection: §9.1.6.

### RL-251 — Page-224 DDM reinterpretation omits its service list (resolved in Lab)

The candidate introduces “the following three object management services” but
omits the enumeration. The source names Register Object Instance With Regions,
Send Interaction With Regions, and Request Attribute Value Update With
Regions; preserve the candidate as incomplete until the Lab includes that list.

Source: `content/clauses/09-data-distribution-management-page-224.tex`, lines
137–147 (page 224). Reviewed subsection: §9.1.6.

### RL-253 — Get Dimension Handle exception is missing from the requirement extraction (resolved in Lab)

The official 2025 C++ API declares `NameNotFound` for Get Dimension Handle,
but the Lab has no separate requirement candidate for that outcome. Keep the
API-backed test visible as such; do not claim the missing exception is covered
by a requirement mapping.

Source: `content/clauses/10-support-services-page-259.tex`, lines 144–146
(page 259), checked against the official §10.23 C++ declaration. Reviewed
subsection: §10.23.

### RL-256 — Table 5 ConfigurationResult text has spelling and markup defects (resolved in Lab)

The candidate contains `addtionalSettingsResultCode`, `SETTINGS _APPLIED`, and
a malformed following `message:String>` row. Literal use could produce an
incorrect report field or enum; retain the exported IDs and validate field
spelling against the official 2025 source.

Source: `content/clauses/11-management-object-model-page-296.tex`, lines 32–35
(page 296). Reviewed subsection: §11.5.1.

### RL-257 — Table 5 Credentials row is incomplete (resolved in Lab)

The candidate reduces the encoding to `Record<` and its example to
“unspecified,” omitting the local record fields and example. The source-backed
shape is `Record<Type:String, Data:BinaryData>`; do not treat the incomplete
candidate as the normative encoding.

Source: `content/clauses/11-management-object-model-page-296.tex`, line 56
(page 296). Reviewed subsection: §11.5.1.

### RL-258 — Page-187 Delay Subscription Evaluation candidate fragments are clipped (resolved in Lab)

The l16 candidate begins mid-sentence and ends at an incomplete `IEEE Std`
reference. The l22 candidate contains only `1516.2-2025. This switch setting
shall not be modifiable during a federation execution.` without its switch
antecedent. The tracked semantic page retains the surrounding paragraph. Keep
the focused behavior associated with §8.1.8, but do not select either fragment
as a complete requirement.

Candidates: `requirement-candidate-content-clauses-08-time-management-page-187-l16-3`
and `requirement-candidate-content-clauses-08-time-management-page-187-l22-5`.

Source: `content/clauses/08-time-management-page-187.tex`, lines 16–22 (page
187). Reviewed subsection: §8.1.8.

### RL-259 — Page-187 Delay Subscription Evaluation fragments repeat a crosswalk error (resolved in Lab)

Both candidate fragments export the Delay Subscription Evaluation material as
§8.1.10; the source heading is §8.1.8. This is recorded as a recurrence of
RL-252. Preserve the exported clause as provenance and use §8.1.8 only for
source-backed test traceability.

Candidates: `requirement-candidate-content-clauses-08-time-management-page-187-l16-3`
and `requirement-candidate-content-clauses-08-time-management-page-187-l22-5`.

Source: `content/clauses/08-time-management-page-187.tex`, lines 16–22 (page
187). Reviewed subsection: §8.1.8; recurrence: RL-252.

### RL-261 — Catch2 plan JSON/row corruption recurred after repair (resolved in Umbra)

This is an Umbra plan-integrity issue, not a defect in the IEEE text or Lab
export: a prior bounded query reported a JSON control character and damaged
row identity after RL-260. The current `resume` query parses successfully, so
the open structured status needs a separate status review; this reconciliation
does not silently resolve it.

Source: `compliance/requirements-lab/catch2-test-plan.json`, lines 35514–35521
at detection. Recurrence: RL-260.

### RL-263 — Page-290 new-parameters candidate is only a trailing fragment (resolved in Lab)

The candidate says only that an RTI-sent message “shall not contain the new
parameters”; it omits the antecedent, interaction, and meaning of “new
parameters.” Leave it uncovered rather than infer its scope from adjacent
requirements.

Source: `content/clauses/11-management-object-model-page-290.tex`, lines 12–14
(page 290). Reviewed subsection: §11.4.1.

### RL-264 — Page-290 ignored-interaction statement is split (resolved in Lab)

One candidate ends with “These classes of interaction”; its neighbor begins
with the detached predicate that they “shall be ignored.” The affected class
set cannot be identified from either record alone; leave both uncovered until
the full statement is preserved.

Source: `content/clauses/11-management-object-model-page-290.tex`, lines 75–78
(page 290). Reviewed subsection: §11.4.1.

### RL-265 — Page-288 HLAfederate candidate is assigned to §11.3 (resolved in Lab)

The candidate exports as §11.3, while the source page places the HLAfederate
MOM object-class text under §11.2. This repeats the crosswalk pattern in
RL-252; do not use this candidate to claim MOM removal or callback-order
coverage.

Source: `content/clauses/10-support-services-page-288.tex`, line 9 (page 288).
Reviewed subsection: §11.2; recurrence: RL-252.

### RL-309 — Page-148 ownership paragraph is split across candidates (resolved in Lab)

The ownership paragraph is fragmented across lines 94, 100, 103, and 106;
individual records terminate mid-sentence or begin with a continuation. Avoid
mapping any one fragment as the complete ownership rule.

Source: `content/clauses/07-ownership-management-page-148.tex`, lines 94–111
(page 148). Reviewed clause: Clause 7.

### RL-310 — Page-148 ownership-control exception omits its MOM list (resolved in Lab)

The RTI ownership-control exception candidate omits the associated MOM
interaction list: HLAmodifyAttributeState,
HLAunpublishObjectClassAttributes,
HLAunconditionalAttributeOwnershipDivestiture, and
HLAresignFederationExecution. Preserve the stable ID and use the source
context without claiming the candidate is complete.

Source: `content/clauses/07-ownership-management-page-148.tex`, page 148.
Reviewed clause: Clause 7.

### RL-311 — Page-148 ownership-transfer rule is truncated across candidates (resolved in Lab)

The line-112 candidate ends mid-rule; line 121 begins its continuation and
also ends mid-sentence, followed by more continuation at line 124. The split
prevents either record from standing alone as the complete transfer condition.

Source: `content/clauses/07-ownership-management-page-148.tex`, lines 112–123
(page 148). Reviewed clause: Clause 7.

### RL-312 — Page-178 TAR table candidate has TeX artifacts and mixed columns (resolved in Lab)

The request-time symbol is corrupted by font-size TeX and the candidate runs
from the zero-lookahead TAR prohibition into a separate pre-grant delivery
column. A focused test may map only the explicit zero-lookahead bound; the
other table-column rules need complete source-backed candidates.

Source: `content/clauses/08-time-management-page-178.tex`, lines 45–54 (page
178). Exported clause: Clause 8; exact subsection remains unresolved.

### RL-342 — Page-44 saved-message restore rule has only a Clause-4 label (resolved in Lab)

The candidate for saved-message delivery after restore exports only as Clause
4. The source context places it under §4.1.2; keep the broad export label as
provenance and use that subsection only as a local source-backed crosswalk.

Source: semantic content for pages 43–44, page-44 lines 57–64. Reviewed
subsection: §4.1.2.

### RL-343 — Page-204 TSO delivery cutoffs are assigned to asynchronous delivery (resolved in Lab)

The TAR/NMR equal-time cutoff and TARA/NMRA strict-time cutoff are exported
under asynchronous-delivery preconditions (§8.15.3). Their source context is
§8.14.4(d) and (e), respectively; preserve both stable IDs and do not infer
that asynchronous delivery controls the TSO cutoff.

Source: `content-15161-page-204` semantic content, immediately before the
§8.15 heading. Reviewed subsections: §8.14.4(d) and §8.14.4(e).

### RL-344 — Page-62 failed registration no-effect rule is assigned to the next service (resolved in Lab)

A negative Register Federation Synchronization Point result with no other
federation effect is exported under §4.16 Announce Synchronization Point. The
source places it in the preceding §4.15 registration service; keep that
crosswalk source-backed and the exported label unchanged.

Source: `content-15161-page-062` semantic content before the §4.16 heading.
Reviewed subsection: §4.15.

### RL-345 — Page-225 region-designator lifetime rule is assigned to §9.1.8 (resolved in Lab)

The candidate concerns preserving a conveyed Region-designator copy through
the receiving callback, including while the source region changes. The source
places it under §9.1.7, not §9.1.8 Allow Relaxed DDM; retain the export label
as provenance.

Source: page-225 semantic content between the §9.1.7 and §9.1.8 headings.
Reviewed subsection: §9.1.7.

### RL-346 — Page-193 time-constrained requirements inherit Disable exceptions (resolved in Lab)

Three candidates are exported as §8.7.5 Exceptions. The first two describe
Enable Time Constrained postconditions under §8.6.4; the third belongs to the
§8.7 Disable Time Constrained service description. Keep those obligations
separate in test mappings.

Source: page-193 semantic flow: §8.6.4 postconditions followed by the §8.7
service description. Reviewed locations: §8.6.4 and §8.7.

### RL-347 — Page-44 federate-type restore rules have only a Clause-4 label (resolved in Lab)

Per-type membership counts and same-type restore interchangeability are
exported under Clause 4 only. The page-43/44 source places both statements in
§4.1.2; retain the stable IDs and do not treat broad Clause 4 as subsection
precision.

Source: semantic content for pages 43–44, before the §4.1.3 heading. Reviewed
subsection: §4.1.2.

### RL-348 — Page-57 federate-type join rule has only a Clause-4 label (resolved in Lab)

The rule that the Join Federation Execution federate-type argument separates
save/restore categories is exported under Clause 4 only. Its source location
is the §4.11 service description, not specifically §4.11.4 postconditions.

Source: `content-15161-page-057.tex`, §4.11 service description before
§4.11.1–§4.11.4. Reviewed subsection: §4.11.

### RL-349 — Page-202 Flush Queue Grant result is assigned to preconditions (resolved in Lab)

Two adjacent candidates split one Flush Queue Grant result paragraph. Page-202
l104 is a leading fragment ending in “and”; l107 continues with the advanced
logical-time and OLT statements. Both are exported under §8.13.3 Preconditions,
although the complete paragraph follows the §8.13 service heading and precedes
§8.13.1–§8.13.3. Preserve both stable IDs and use §8.13 as the source-backed
subsection for traceability.

Affected requirements: `requirement-candidate-content-clauses-08-time-management-page-202-l104-28`
and `requirement-candidate-content-clauses-08-time-management-page-202-l107-29`.
Source: page-202 layout body between the §8.13 heading and §8.13.1–§8.13.3.
Reviewed subsection: §8.13.

### RL-350 — Page-182 optimistic Flush Queue lookahead rule has only a Clause-8 label (resolved in Lab)

The rule restricting a regulator after Flush Queue Grant from sending below
optimistic logical time plus optimistic lookahead is exported only under Clause
8. The page-182 heading places it in §8.1.5; the broad label hides focused
subsection coverage.

Source: page-182 semantic layout under §8.1.5, before §8.1.6. Reviewed
subsection: §8.1.5.

### RL-374 — Page-213 order-type paragraph is split into incomplete mixed candidates (resolved in Lab)

The four candidates
`requirement-candidate-content-clauses-08-time-management-page-213-l76-20`,
`requirement-candidate-content-clauses-08-time-management-page-213-l79-21`,
`requirement-candidate-content-clauses-08-time-management-page-213-l88-24`,
and
`requirement-candidate-content-clauses-08-time-management-page-213-l94-26`
split one continuous §8.25.3 paragraph at page-line boundaries. The first
starts with “transfer)” and ends at “corresponding class”; the next starts with
“attribute default order type” and ends with “When changing a default order
type,”; the third also begins mid-sentence and ends before the service
statement is complete; the fourth begins with “shall affect”. The paragraph
covers several distinct obligations: initialization when ownership is gained,
FDD-derived defaults at join, default-change argument rules, and the scope of a
default change on future and existing instance attributes and other
federates.

Impact: each incomplete candidate is presented as a stable requirement-level
mapping unit. Mapping a test to one fragment can overstate its coverage, while
keeping the fragments open obscures real §8.25.3 progress. Preserve the IDs and
exported subsection as provenance; do not claim full requirement coverage
from a partial assertion. Keep focused C++ implementation/TCK evidence
separately described against the complete paragraph until the Lab provides
complete, usable obligation boundaries.

Source: `content/clauses/08-time-management-page-213.tex`, lines 76–102 (page
213), document `hla-1516.1-2025`. This is an extraction/usability defect, not
IEEE errata or provider-conformance evidence. No Requirements Lab
resynchronization was performed.

Follow-up on 2026-10-22: unsupported direct selections of the page-213
candidates were removed from the Catch2 plan and C++/Java TCK catalogs. The
focused C++ order-control test now verifies per-federate default isolation
under MinGW/Ninja and passes its exact CTest selector (1/1), but leaves that
behavior supplemental; portable C++ TCK §8.25.3 coverage is 0/4 until the Lab
provides usable obligation boundaries.

Follow-up on 2026-10-22: the parallel order-type requirements contract was
audited as part of RL-353 cleanup. Its stale selectors were updated, its direct
links to the incomplete page-213 candidates were removed, and the exact
requirements-contract checker passes. A separate official-API Catch2 case now
tests acquisition-time initialization from the new owner's class default after
ownership transfer; it passes the focused MinGW CTest (1/1) and remains
explicitly unmapped under this issue. The portable C++ §8.25.3 gap query
remains honestly 0/4.

Follow-up on 2026-10-22: the complete source paragraph was used to add a
focused case that sets inherited `Name` order through the derived `Server`
handle, then registers a base `Employee` instance. It first failed: the RTI
keyed the default to the invocation handle, so the future base instance kept
the FDD TimeStamp order. The registry now stores the federate-scoped default
at the class that declares the attribute. MinGW/Ninja rebuild and the focused
case pass. This behavior remains supplemental and unmapped; §8.25.3 stays 0/4
in the portable TCK gap query pending usable Lab requirement boundaries.

Follow-up on 2026-10-22: the existing public order-control case also now
asserts `ObjectClassNotDefined` for an undefined class handle and
`AttributeNotDefined` when `Efficiency` is supplied with the `Employee` class
handle where it is unavailable. Both error assertions pass; they remain
supplemental because RL-374 does not provide an independently usable atomic
requirement row for these mixed page-213 obligations.

Follow-up on 2026-10-22: the public process-profile class-request test now uses
`NumberCups`, declared on `Drink` and inherited by `Soda`, while setting the
order default through the `Soda` class handle. The requester sets `RECEIVE`,
while the provider retains `TIMESTAMP` for that inherited attribute; its
timestamped response confirms the defaults remain federate-local. The CTest
passes under both callback models (1/1, 92 assertions).
This confirms the inherited-attribute path through the authoritative process
coordinator, but remains supplemental/unmapped under RL-374; §8.25.3 TCK
coverage remains 0/4. No new Lab defect or resynchronization was needed.

A distinct-OS-process companion now exercises the same inherited-attribute
default isolation through one authoritative coordinator. The requester process
sets `RECEIVE`; the provider process retains `TIMESTAMP`, and the timestamped
response is observed before the requester's TAR grant. Its focused MinGW CTest
passes 1/1. This is additional supplemental evidence only: it does not map the
four incomplete §8.25.3 candidates, and portable C++ TCK coverage remains 0/4.

Follow-up on 2026-10-22: the existing promoted `cpp-tck.order-type-controls`
primary/contract pair already exercises the acquisition-time behavior through
the official C++ API: the prior owner sets the instance attribute to
`TIMESTAMP`, the acquiring federate's class default is `RECEIVE`, and its
post-acquisition timestamped update is delivered as receive-order without a
retraction handle. The installed-package MinGW CTest and direct JSON/JUnit
matrix passed 4/4 across HLA_EVOKED/HLA_IMMEDIATE. Catalog notes now classify
this as partial evidence against the complete §8.25.3 paragraph, not a direct
mapping to any fragment; coverage remains 0/4. No duplicate scenario or Lab
resynchronization was needed.

### RL-375 — Page-150 unconditional-divestiture candidate omits its service invocation (resolved in Lab)

The candidate
`requirement-candidate-content-clauses-07-ownership-management-page-150-l152-47`
starts with the dependent phrase “which case, the instance attribute shall
immediately become unowned...” and omits the preceding subject and trigger:
“The joined federate may invoke the Unconditional Attribute Ownership
Divestiture service, in”. The source continues that the attribute becomes
unowned by that federate and, in fact, by all joined federates. The exported
row is therefore not a self-contained statement of the service rule.

Impact: a stable requirement row that omits the service and actor makes direct
traceability ambiguous. A test mapped to it could appear to cover unconditional
divestiture even though the candidate itself states only the result fragment.
Keep this candidate unmapped as a standalone conformance unit until the Lab
restores the complete sentence or usable atomic obligation boundaries. Existing
tests may remain supplemental or map only to independently complete
requirements; do not infer validation from this fragment.

Source: `output/semantic/content-block-content-15161-page-150.tex`, lines
153–160 (page 150), document `hla-1516.1-2025`, exported/normative subsection
§7.1.2.1. This is a Requirements Lab extraction/usability defect, not IEEE
errata or provider-conformance evidence. No Requirements Lab resynchronization
was performed.

Reproduction: run
`python tools/query_rti_work.py tck-gaps --document hla-1516.1-2025 --clause clause-7 --limit 5 --verbose`, inspect the
exact candidate above, then compare it with the page-150 source around lines 153–160.

Detected 2026-10-22. The exact candidate ID was not already recorded in the
machine-readable or human issue ledger.

Follow-up on 2026-10-22: removed this unsupported candidate from five Catch2
plan entries. Their implementations and other requirement mappings remain
intact; no behavior tests were changed. The candidate remains unmapped pending
a complete Lab obligation boundary.

### RL-376 — Page-161 §7.8 acquisition requirements are split and truncated (resolved in Lab)

The candidates
`requirement-candidate-content-clauses-07-ownership-management-page-161-l152-45`,
`requirement-candidate-content-clauses-07-ownership-management-page-161-l158-47`,
and
`requirement-candidate-content-clauses-07-ownership-management-page-161-l167-50`
do not expose complete obligation boundaries. The l152 candidate begins in the
middle of the condition (“is in the Waiting for a New Owner to be Found state”)
and ends at “the owning joined”. The l158 row resumes with the orphaned
“federate, but it shall invoke...” continuation. Together they split one
state-dependent rule requiring the RTI not to issue an ordinary ownership
release request while issuing Request Divestiture Confirmation to the owner.
The l167 row starts the separate rule that an acquisition remains pending, but
cuts off at “Attribute Ownership” before identifying the denial service and
outcome.

Impact: each stable row is incomplete as an independent mapping unit. Selecting
one row can omit the state/owner condition or overstate coverage of the complete
pending-until-grant-or-denial behavior. The existing public C++ scenario tests
the Waiting-for-a-New-Owner callback behavior and the grant path, but does not
exercise the denial branch included in the l167 sentence.

Workaround: preserve these IDs as provenance, but keep the affected public TCK
rows candidate-only and do not count them as promoted requirement coverage
until the Lab supplies complete atomic boundaries. Keep the full-source §7.8
scenario as supplemental evidence, and remove unrelated internal-test
selections that do not exercise the waiting-owner transition. Other uses of the
l158 candidate remain subject to case-specific review. Do not resynchronize
unchanged Lab inputs.

Source: `content/clauses/07-ownership-management-page-161.tex`, lines 152–172
(page 161), document `hla-1516.1-2025`, subsection §7.8. This is a
Requirements Lab extraction/usability defect, not IEEE errata or
provider-conformance evidence.

Reproduction: run
`python tools/query_rti_work.py requirement <candidate-id> --json` for each
affected ID, then compare the statements and source spans with the pinned
page-161 acquisition paragraph in
`topics/ieee/hla-1516.1-2025/semantic/content-blocks.json`.

Detected 2026-10-22. The exact rows and full source were compared directly; no
matching page-161 issue was present in either ledger. The waiting-owner public
C++ TCK primary and contract each passed under HLA_EVOKED and HLA_IMMEDIATE
(4/4 total), but are now candidate-only pending usable Lab rows. The unrelated
ownership-cancellation Catch2 case no longer selects the l152/l167 candidates.
RL-376 remains open; no Requirements Lab resynchronization was performed.

### RL-377 — Page-148 §7 overview and ownership-state candidates are split (resolved in Lab)

The candidates
`requirement-candidate-content-clauses-07-ownership-management-page-148-l16-3`,
`requirement-candidate-content-clauses-07-ownership-management-page-148-l22-5`,
`requirement-candidate-content-clauses-07-ownership-management-page-148-l52-15`,
`requirement-candidate-content-clauses-07-ownership-management-page-148-l58-17`,
`requirement-candidate-content-clauses-07-ownership-management-page-148-l61-18`,
`requirement-candidate-content-clauses-07-ownership-management-page-148-l70-21`,
`requirement-candidate-content-clauses-07-ownership-management-page-148-l82-25`,
and
`requirement-candidate-content-clauses-07-ownership-management-page-148-l88-27`
do not preserve complete, self-contained statement boundaries. The l16 row
starts with the tail “among joined federates” and ends after “Only the”; l22
starts with “joined federate” and continues the owner-only service list. The
l52 row starts with the continuation “legal operations” while also including
the distinct complete exception sentence. The l58 row cuts the single-owner
rule at “may be”; l61 begins “unowned by all joined federates” and cuts the RTI
owner-search sentence at “attempt”; l70 resumes that sentence and then cuts a
new state-machine rule at “every instance”; l82 contains its continuation.
The l88 registration rule ends at “at the”; its continuation is among the
separate registration/discovery fragments already tracked by RL-309.

Impact: a direct mapping to any one of these rows can omit the subject,
condition, or outcome of an ownership rule, or conflate an overview statement
with a distinct normative obligation. Existing Catch2 cases select several of
these IDs, so those selections must not be treated as standalone Lab-backed
coverage without a verified complete group boundary.

Workaround: keep the stable IDs and pinned source unchanged. Do not count these
rows as standalone direct mappings until the Lab restores complete atomic
boundaries. Keep behavior proved only by internal tests explicitly supplemental
or internal-only; retain independent, complete mappings in those tests. For
the l88 registration fragment, account for RL-309's linked continuation rather
than extending or rewriting RL-309. RL-310 and RL-311 remain separate records
for the later page-148 exception-list and ownership-transfer fragments. Do not
resynchronize unchanged Lab inputs.

Source: `content/clauses/07-ownership-management-page-148.tex`, source spans
16–93 (page 148), document `hla-1516.1-2025`, clause 7. The rendered semantic
transcription is
`output/semantic/content-block-content-15161-page-148.tex`, lines 12–46. This
is a Requirements Lab extraction/usability defect, not IEEE errata or
provider-conformance evidence.

Reproduction: query the eight candidate IDs above with
`python tools/query_rti_work.py requirement <candidate-id> --json`, then
compare their statements with the ownership overview/state text in the pinned
page-148 semantic transcription. The related candidates in RL-309 through
RL-311 remain separately tracked.

Detected 2026-10-22. The exact candidate set was compared with the reconstructed
page-148 source; these eight IDs were absent from both issue ledgers, while
the neighboring l94/l100/l103/l106, l112/l121, and l124 fragments were already
tracked by RL-309 through RL-311. The Catch2 plan had selected the eight new IDs
in 12 rows (16 selections); those affected selections have now been removed,
with unrelated requirement mappings preserved and the implementation behaviors
retained as supplemental evidence. The standalone acquisition row also no
longer selects l112/l124, consistent with the existing RL-310/RL-311 workarounds.
No Requirements Lab resynchronization was performed.

### RL-379 — Page-216 §9.1.1 region-dimension candidate drops its subject and merges the next rule (resolved in Lab)

The candidate
`requirement-candidate-content-clauses-08-time-management-page-216-l87-26`
states “shall be the dimensions of the ranges that are included in the region
specification. A region”. In the reconstructed semantic source, this is item
c) of §9.1.1: the candidate omits the subject “The dimensions contained in a
region specification”, then runs into the beginning of the next sentence,
“A region specification shall contain at most one range for any given
dimension.” The candidate is therefore neither a complete requirement nor a
clean boundary between the two rules.

Impact: a direct mapping could claim dimension-set equivalence or per-
dimension range uniqueness without testing the complete obligation. The
candidate is currently uncovered by the Catch2 plan; keep it uncovered until
the Requirements Lab provides a complete statement boundary. No behavior has
been promoted or inferred from adjacent source text.

Source: `content/clauses/08-time-management-page-216.tex`, lines 87–89, Lab
requirement section Clause 9; reconstructed semantic block
`output/semantic/content-block-content-15161-page-216.tex`, item c) under
§9.1.1 (printed source page 217).

Reproduction: run
`python -X utf8 tools/query_rti_work.py requirement requirement-candidate-content-clauses-08-time-management-page-216-l87-26 --json`,
then compare the returned statement with item c) in the pinned page-216
semantic block.

Detected 2026-10-22. The exact query returns the fragment and reports no
matching tests; the pinned semantic block confirms the omitted subject and
next-sentence spillover. The candidate was absent from both issue ledgers and
remains uncovered, so no test mapping required cleanup. This is a Requirements
Lab extraction/usability issue, not IEEE errata or conformance evidence. No
Requirements Lab resynchronization was performed.

### RL-378 — Page-178 OLT guarantee sentence is split across candidates (resolved in Lab)

The candidate
`requirement-candidate-content-clauses-08-time-management-page-178-l93-28`
contains the complete OLT calculation and explanation, then ends with the
incomplete phrase “The guarantee that the RTI makes about”. Its adjacent
candidate
`requirement-candidate-content-clauses-08-time-management-page-178-l108-33`
starts with the lowercase continuation “message delivery relative to the
provided logical time shall depend on the type of request to advance time;”.
The reconstructed page-178 source has one continuous sentence across those
rows, followed by the separate statement that the specific guarantees are
provided in each service description.

Impact: neither row is an independently complete requirement. The first
combines an OLT rule with a truncated delivery-guarantee sentence; the second
is only the latter sentence's predicate. Treating either as an atomic mapping
can overstate general Clause 8 delivery guarantees. The FQR callback-selection
scenario also cited the OLT candidate for the unrelated claim that FQR receives
Flush Queue Grant rather than Time Advance Grant.

Workaround: keep both stable IDs and the pinned Lab data unchanged, but do not
select either as direct requirement evidence until the Lab exports a complete
boundary. The affected Catch2 and C++/Java TCK mappings have been removed;
their other complete mappings remain. Keep OLT tests, message-delivery
observations, and callback-selection checks as supplemental evidence only for
these claims. RL-312 and RL-313 concern separate earlier page-178 table and
pre-grant fragments; this is a new, distinct defect, not a rewrite of those
records. Do not resynchronize unchanged Lab inputs.

Source: `content/clauses/08-time-management-page-178.tex`, lines 93–110 (page
178), document `hla-1516.1-2025`, Clause 8. The rendered semantic
transcription is
`output/semantic/content-block-content-15161-page-178.tex`.

Reproduction: query both candidate IDs with
`python -X utf8 tools/query_rti_work.py requirement <candidate-id> --json`,
then compare their statement boundaries with the continuous OLT paragraph in
the unchanged page-178 semantic transcription.

Detected 2026-10-22. The l93 query ended at “about”; the l108 query began at
“message delivery” and ended at a semicolon. The reconstructed source confirms
the sentence crosses the two records. The Catch2 plan selected l93 once and
l108 in two rows; the C++ and Java TCK catalogs selected them in three and
two scenarios respectively, and the FQ-grant-selection contract cited l93 for
an unrelated callback claim. Those direct selections have now been removed;
other mappings are preserved. This is a Requirements Lab extraction and
traceability usability issue, not IEEE errata or provider-conformance
evidence. No Requirements Lab resynchronization was performed.

### RL-380 — Page-216 DDM content element is classified as Clause 8 (resolved in Lab)

The pinned page-216 content element
`content-element-content-15161-page-216-paragraph-001` assigns the page to
`clause-8`, although its text begins with “9. Data distribution management
(DDM)” and contains the visible headings §9.1 Overview and §9.1.1 Definitions
for DDM. Both the concepts-and-terms lead-in
`requirement-candidate-content-clauses-08-time-management-page-216-l48-13` and
the dimension-definition requirement
`requirement-candidate-content-clauses-08-time-management-page-216-l54-15`
retain the `clauses-08-time-management` identifier and source path, but their
structured requirement records assign `clause_id: clause-9`.

Impact: consumers of page-level content metadata can classify this DDM page as
Clause 8 while requirement-level queries classify the same text as Clause 9.
That inconsistency can mislead section inventory, navigation, and
requirement-to-test traceability. RL-379 concerns a separate truncated item-c
boundary on this page; it does not record this page-level classification
mismatch.

Workaround: preserve the Lab IDs and exports. Use the requirement records’
Clause 9 plus the visible page headings to review both candidates under
§9.1.1. For l48-13, claim only the dimension/range/region concepts exercised
by the mapped tests, not all DDM services. Do not infer behavior or validation
from the classification mismatch. Do not resynchronize unchanged Lab inputs.

Source: `topics/ieee/hla-1516.1-2025/semantic/content-elements.json`, page-216
element `content-element-content-15161-page-216-paragraph-001`, compared with
`topics/ieee/hla-1516.1-2025/semantic/requirements.json` and the page-216
semantic text.

Reproduction: query both l48-13 and l54-15, then compare their `clause_id`
values with the page-216 content element’s `clause_id` and inspect the visible
DDM headings in the same element.

Detected 2026-10-22. The requirement record reports `clause-9`; the content
element reports `clause-8`, while its text identifies Clause 9 and §9.1.1.
Both the l48-13 and l54-15 requirement records report `clause-9`.
This is a Requirements Lab crosswalk/extraction defect, not IEEE errata or
provider-conformance evidence. No Requirements Lab resynchronization was
performed.

Additional affected page-216 records confirmed 2026-10-22: l66-19, l69-20,
l72-21, and l78-23 also retain the Clause 8 source identifier while their
structured requirement records assign Clause 9. See RL-381 for the separate
statement-fragmentation defect spanning those four candidates.

### RL-381 — Page-216 §9.1.1 range definition is fragmented across four candidates (resolved in Lab)

The page-216 §9.1.1 range definition is split across four candidate records:
l66-19 ends with “The interval is defined by an”; l69-20 ends with “and the”;
l72-21 ends with “The minimum possible difference between a range lower
bound”; and l78-23 contains only “and a range upper bound shall be 1 (one).”
Together they form continuous normative prose about the semi-open interval,
its lower/upper bound pair, the strict upper-greater-than-lower relation, and
the minimum span of one. No individual record contains a complete statement.

Impact: a query or test plan that treats any one fragment as a complete
requirement can overstate what it proves or omit a necessary condition. This
fragmentation is distinct from RL-380’s page-level Clause 8/Clause 9
crosswalk mismatch; both affect these same page-216 records.

Workaround: retain all four Lab IDs and map them only as a group when tests
collectively cover the applicable range semantics. Cite RL-381 and use the
visible §9.1.1 text for interpretation; do not invent four independent
requirements or claim that a partial assertion validates the whole passage.

Source: pinned `topics/ieee/hla-1516.1-2025/semantic/requirements.json`, the
page-216 semantic text, and the page content element identified under RL-380.

Reproduction: inspect the candidate statements at page 216, lines 66, 69, 72,
and 78 in order. Concatenating the fragments reconstructs the range definition
but demonstrates that each exported candidate is incomplete on its own.

Detected 2026-10-22. This is a Requirements Lab extraction/segmentation defect,
not IEEE errata or provider-conformance evidence. No Requirements Lab
resynchronization was performed.

### RL-382 — Page-44 save-state identity candidate starts mid-sentence and omits §4.1.3 (resolved in Lab)

The candidate `requirement-candidate-content-clauses-04-federation-management-page-044-l15-4`
begins with “state, it shall use the federation save label” and omits the
preceding subject and Initiate Federate Save condition. Its exported clause is
only Clause 4. The pinned page-44 semantic source identifies §4.1.3,
“Synchronization discussion,” and supplies the complete sentence beginning
“When a joined federate receives an Initiate Federate Save service invocation”.

Impact: the fragment is not a self-contained test oracle, and the broad clause
assignment hides the exact save/synchronization subsection. Mapping the
fragment alone can overstate coverage or omit the condition on using the
federation save label, federate type, and federate designator.

Workaround: preserve the stable ID and leave it uncovered until the Lab
restores a complete statement or the full pinned source is used. A source-backed
local mapping should use §4.1.3 and cover the complete Initiate Federate Save
condition and identity tuple. Do not infer the missing subject or resynchronize
unchanged Lab inputs.

Source: `topics/ieee/hla-1516.1-2025/output/semantic/content-block-content-15161-page-044.tex`
and the candidate record in `topics/ieee/hla-1516.1-2025/semantic/requirements.json`.

Reproduction: run
`python tools/query_rti_work.py requirement requirement-candidate-content-clauses-04-federation-management-page-044-l15-4 --json`
and compare the candidate statement with the full page-44 semantic source.

Detected 2026-10-22. This is a new, distinct page-44 extraction/crosswalk
defect—not a rewrite of RL-342 or RL-347—and is not IEEE errata or
provider-conformance evidence. No Requirements Lab resynchronization was
performed.

### RL-383 — Page-40 Disconnect candidate omits its condition and §4.1.1 mapping (resolved in Lab)

The candidate `requirement-candidate-content-clauses-04-federation-management-page-040-l54-15`
begins “executions, it shall perform a Disconnect”, omitting the condition
that the federate has resigned and has no further intent to join or create a
federation execution. It continues with the Disconnect service explanation
and a Figure 4 summary, while the requirement record assigns only Clause 4.
The complete page-40 source is under §4.1.1 Federate lifetime.

Impact: the candidate is not a complete test oracle and its broad clause label
hides the exact subsection. Selecting it alone can overstate coverage of the
missing condition or conflate the obligation with explanatory text.

Workaround: retain the stable Lab ID but leave this candidate uncovered. Use
the detailed `req-federate-disconnect-after-resign` requirement, mapped to
§4.1.1, when assertions cover the full resignation/no-further-intent
condition and Disconnect call. Do not resynchronize unchanged Lab inputs.

Source: `topics/ieee/hla-1516.1-2025/output/semantic/content-block-content-15161-page-040.tex`
and the page-40 candidate in
`topics/ieee/hla-1516.1-2025/semantic/requirements.json`.

Reproduction: run
`python tools/query_rti_work.py requirement requirement-candidate-content-clauses-04-federation-management-page-040-l54-15 --json`
and compare the statement with the complete page-40 source paragraph.

Detected 2026-10-22. This is a distinct page-40 candidate from RL-373 and is
tracked separately rather than folded into that record. It is a Requirements
Lab extraction/crosswalk defect, not IEEE errata or provider-conformance
evidence. No Requirements Lab resynchronization was performed.

Applied local workaround 2026-10-22: the final-federate, ordinary non-final,
and RTI-forced Catch2 cases now map `req-federate-disconnect-after-resign` to
§4.1.1 and leave this candidate uncovered. The final-federate CTest entries
passed 2/2 with 18 direct assertions; the ordinary case passed with 15
assertions; and the forced case passed with 21 assertions. The forced case
uses an internal trigger, not a public resignation mechanism. Both focused
lane mapping checks passed. The Lab source remains unchanged and RL-383
remains open.

### RL-384 — Page-143 Report Attribute Transportation Type rule is truncated and assigned to §6.29.1

The pinned candidates
`requirement-candidate-content-clauses-06-object-management-page-143-l156-42`
and
`requirement-candidate-content-clauses-06-object-management-page-143-l162-44`
split the introductory §6.29 Report Attribute Transportation Type paragraph.
The first starts with the service purpose and unowned condition but truncates
after “the transportation”; the second starts mid-sentence at “type specified
in the FDD shall be reported”, without the condition or preceding context.
Both are assigned to §6.29.1, although the `6.29.1 Supplied arguments` heading
follows the complete rule at source line 47.

Workaround: preserve the Lab ID and export, but use the pinned source paragraph
and canonical §6.29 when reviewing the unowned-attribute fallback. The focused
native C++ case makes an attribute unowned after changing its effective
transportation type away from the FOM-declared value, then queries it from the
peer and checks the Report Attribute Transportation Type callback returns the
FOM value. The MinGW target built and its exact CTest selector passed 1/1; the
focused lane check reports nine direct requirement-section pairs. Do not
resynchronize unchanged Lab inputs.

Source: `topics/ieee/hla-1516.1-2025/output/semantic/content-block-content-15161-page-143.tex`
and the page-143 candidate in
`topics/ieee/hla-1516.1-2025/semantic/requirements.json`.

Reproduction: run
`python tools/query_rti_work.py requirement requirement-candidate-content-clauses-06-object-management-page-143-l156-42 --json`
and
`python tools/query_rti_work.py requirement requirement-candidate-content-clauses-06-object-management-page-143-l162-44 --json`,
then compare both fragments with the complete page-143 source paragraph before
the §6.29.1 heading.

Detected 2026-10-22. This is a Requirements Lab extraction/crosswalk defect,
not IEEE errata or provider-conformance evidence. No Requirements Lab
resynchronization was performed.

## RL-385 — Page-191 Enable Time Constrained result is assigned to §8.5.5

The candidate
`requirement-candidate-content-clauses-08-time-management-page-191-l76-18`
says the RTI indicates successful Time Constrained enablement by invoking the
Time Constrained Enabled service. The pinned semantic page places this sentence
under the §8.5 Enable Time Constrained service heading, before §8.5.1, while the
Lab exports it as §8.5.5.

Workaround: preserve the stable Lab ID and export, but map this callback result
to §8.5 only in tests that directly assert Time Constrained Enabled. Do not
select it from service-report-only evidence. The portable C++ GALT-equality
primary/contract directly observes the callback; the separate page-191 l73
candidate remains under RL-328. Do not resynchronize unchanged Lab inputs.

Source: `topics/ieee/hla-1516.1-2025/semantic/content-blocks.json` and
`topics/ieee/hla-1516.1-2025/semantic/requirements.json`, page 191.

Reproduction: run
`python tools/query_rti_work.py tck-gaps requirement-candidate-content-clauses-08-time-management-page-191-l76-18 --summary --compact`,
then compare the candidate text with the page-191 §8.5 service description
before the §8.5.1 heading.

Verification on 2026-10-22: the C++ GALT-equality primary/contract pair passed
the installed-package focused CTest and direct JSON/JUnit gates in both callback
models (4/4 each). Both catalog rows now map this callback result to §8.5 under
RL-385, and the exact `tck-gaps` query reports 1/1 mapped. The time-role
Catch2 callback case uses the same local crosswalk; the service-report-only
case no longer selects this requirement because it does not assert the callback.

Detected 2026-10-22. This is a Requirements Lab subsection-attribution defect,
not IEEE errata or provider-conformance evidence. No Requirements Lab
resynchronization was performed.

### RL-387 — Page-181 §8.1.4 Table 2 caption is exported as a normative requirement

The page-180 candidate
`requirement-candidate-content-clauses-08-time-management-page-180-l156-49`
contains the beginning of the sent-message classification rule but is clipped
after “whether a”. The page-181 candidate
`requirement-candidate-content-clauses-08-time-management-page-181-l115-2`
exports only “type of a sent message shall be determined.” In the pinned page
source, those words belong to the caption “Table 2 illustrates how the order
type of a sent message shall be determined”; the actual rule continues on page
181 before that caption. The candidate's `.tex` location is also a generated
adapter that inputs semantic content, not a file with prose at the cited line.

Impact: the page-181 candidate presents a caption fragment as uncovered
behavior and can misdirect tests toward text that states no independent RTI
behavior. Preserve both candidate IDs, but do not map the caption fragment or
derive assertions from it. Keep any page-180 mapping limited to assertions
supported by the complete source rule. Official C++ Send Interaction, Update
Attribute Values, and Delete Object Instance cases cover all eight Table 2
combinations. A separate regional Send Interaction With Regions case now also
covers all eight combinations while using overlapping explicit regions; this
does not infer regional coverage from ordinary Send Interaction. Keep the
caption unmapped and do not resynchronize unchanged Lab inputs.

Reproduction: run
`python tools/query_rti_work.py requirement requirement-candidate-content-clauses-08-time-management-page-181-l115-2 --json --limit 5`
and compare it with
`content-element-content-15161-page-181-paragraph-001` in the pinned
`semantic/content-elements.json`.

Verification on 2026-10-22: the exact query returned the caption fragment with
no mapped Catch2 case. The page-180/page-181 source blocks show the real rule
continuing before the Table 2 caption. Added all eight Table 2 combinations to
the official 2025 C++ Send Interaction case (153 assertions), Update Attribute
Values case (148), and Delete Object Instance case (97). All three MinGW shared
targets built with zero DWARF, and focused CTest passed five repeats for each.
The regional Send Interaction With Regions matrix adds all eight combinations
as a distinct C++ case (138 assertions); its MinGW g5 shared runner is
53,579,830 bytes with zero DWARF and the exact CTest passes 1/1. It proves
regional-surface coverage rather than inferring it from ordinary interaction
tests. The deletion matrix exposed and fixed a classifier bug in both embedded
and configured-process paths. The configured-process public API case also
checks the unregulated TIMESTAMP-preferred, timestamp-supplied deletion cell
under both callback models; the exact CTest passes 1/1 with local loopback
enabled. This is one process-side deletion matrix cell, not a full process
matrix; the corresponding regional configured-process cell remains queued.
Existing process deletion NMR/NMRA regressions pass 2/2 with local loopback
enabled. Mapping checks pass for the original sender-transition lanes and the
four embedded service-family matrix lanes. This is a Requirements Lab
extraction/usability defect, not IEEE errata or provider-conformance evidence.
No Requirements Lab resynchronization was performed.
