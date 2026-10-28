# Requirements Lab issue queue and acceptance history

As of 2026-10-26, the authoritative register has **142 records and no open mapping complaints**. The corrected Lab revision is pinned in `requirements-lab.lock.json`; 84 Lab-content complaints and four Umbra-local complaints from this 2026-10-24 queue are accepted. The tables below preserve the original 89-issue queue for review history. `known-issues.json` remains authoritative for current status and evidence.

## The split-statement issue from the original queue

**RL-381** is the page-216 §9.1.1 range-definition complaint: four candidate records split one continuous definition. They break across the interval definition, its conjunction, the minimum-span clause, and the final upper-bound condition. Taken separately, none is a complete test oracle. The nearby **RL-380** is a separate page-level Clause 8/Clause 9 crosswalk defect affecting overlapping candidates.

Other prominent multi-record splits include **RL-250** (Receive Interaction, p.224), **RL-309/RL-311** (ownership rules, p.148), **RL-315** (Federation Saved, p.69), **RL-318** (regional discovery, p.233), **RL-332** (Delete Object Instance ordering, p.180), **RL-335/RL-337** (time grant and Flush Queue rules, pp.197/200), **RL-374** (order types, p.213), **RL-376/RL-377** (ownership, pp.148/161), and **RL-378** (OLT guarantee, p.178).

## How to use the queue

- Every row below was marked `open` on 2026-10-24; this includes four Umbra-local plan/traceability follow-ups, shown separately from Lab-content complaints.
- Use the exact issue query for the full affected candidate IDs, symptom, impact, workaround, reproduction command, and any verification: `python tools/query_rti_work.py lab-issues RL-381 --summary --compact` (replace the ID).
- The reviewed Lab revision in the lock is the current source of truth. Resynchronize only after a reviewed Lab revision change.
- “Candidate records: 0” means the issue is a missing-candidate/service-flow report, not a record with a candidate ID.

## Reviewed source conflict

RL-369 is dispositioned for mapping: supplied Table 5 prints `FederateHandleRestoreStatusSet`, while the official MIM uses `FederateRestoreStatusSet` for type 20. Umbra uses the MIM value for its service report and keeps the literal Table 5 candidate r007 unmapped. This bounded source conflict does not block other RTI work.

The additional source-page ownership and TCK section corrections are recorded as resolved RL-388, RL-389, and RL-390.

## Original 2026-10-24 complaints

### Umbra-local plan and traceability follow-ups (4)

| ID | Source | Complaint | Candidate records |
|---|---|---|---:|
| `RL-261` | `catch2-test-plan.json`, lines 35514–35521 | Catch2 plan JSON syntax and row identity corruption recurred after RL-260 repair | 0 |
| `RL-268` | `catch2-test-plan.json`, validator-reported rows: timestamped attribute update, negotiated/confirmed divestiture, timestamped directed interaction, and process restore status | Global Catch2 plan audit reports stale mappings and missing next actions in unrelated rows | 4 |
| `RL-354` | `available_time_advance_inclusive_galt_catch2.cpp`, case `umbra-cpp-time-advance-grant-requested-logical-time` | Time Advance Grant case claimed a Time Constrained Enabled argument rule | 1 |
| `RL-371` | p.51, id=umbra-cpp-connect-requirement-traceability, requirements_lab_api_surface_status | Connect API traceability row cites unresolvable RL-045 | 2 |

### Cross-source name discrepancy (1)

| ID | Source | Complaint | Candidate records |
|---|---|---|---:|
| `RL-369` | p.297, content-15161-page-297 layout_body, Table 5 restore-status collection row; semantic/requirements.json candidate r007 | Table 5 restore-status set name conflicts with the MIM; prior RL-083 citation | 1 |

### Missing requirement or service-flow candidates (3)

| ID | Source | Complaint | Candidate records |
|---|---|---|---:|
| `RL-253` | p.259, 144-146 | Get Dimension Handle NameNotFound is declared by the 2025 C++ API but not extracted as a separate requirement | 1 |
| `RL-321` | p.199, 145-150 | Next Message Request Available backward-time precondition and exception lack candidates | 0 |
| `RL-322` | p.200, 11, 23 | Next Message Request Available Time Advancing precondition and exception lack candidates | 0 |

### Table and structured-record extraction (10)

| ID | Source | Complaint | Candidate records |
|---|---|---|---:|
| `RL-256` | p.296, 32-35 | Table 5 candidates may contain spelling and markup defects in ConfigurationResult fields | 2 |
| `RL-257` | p.296, 56 | Table 5 Credentials candidate omits its full local encoding and example | 1 |
| `RL-312` | p.178, 45-54 | Page 178 TAR timestamp-boundary candidate contains TeX artifacts and mixes adjacent table columns | 1 |
| `RL-358` | p.295, content-15161-page-295 layout_body, Table 5 AdditionalSettingsResultCode row | Table 5 AdditionalSettingsResultCode encoding candidate omits two enumerants | 3 |
| `RL-364` | p.296, content-15161-page-296 layout_body, Table 5 ConfigurationResult row | Table 5 ConfigurationResult candidate omits its record shape | 1 |
| `RL-365` | p.296, content-15161-page-296 layout_body, Table 5 FactoryClass row; structured candidate source lines 98-155 | Table 5 FactoryClass enumeration is fragmented across candidates | 18 |
| `RL-366` | p.296, content-15161-page-296 layout_body, Table 5 FederateAttributes row; structured candidate source lines 167-182 | Table 5 FederateAttributes record is split and column-shifted | 5 |
| `RL-367` | p.296, content-15161-page-296 layout_body, Table 5 FederateHandleSaveStatusPair row; semantic/requirements.json candidates r036-r037 | Table 5 FederateHandleSaveStatusPair record is split and column-shifted | 2 |
| `RL-368` | p.297, content-15161-page-297 layout_body, Table 5 FederateRestoreStatus row; semantic/requirements.json candidates r003-r006 | Table 5 FederateRestoreStatus record is split and column-shifted | 4 |
| `RL-387` | p.180-181, page-180 candidate l156-161; page-181 candidate l115-117; page-181 Table 2 caption and matrix | Page-181 §8.1.4 Table 2 caption is exported as a normative requirement | 2 |

### Clause, subsection, and source-boundary attribution (35)

| ID | Source | Complaint | Candidate records |
|---|---|---|---:|
| `RL-177` | p.132, 107-118 | Page-132 object-management postconditions inherit the following 6.18.1 heading | 2 |
| `RL-248` | p.236, 32-37 | Page-236 regional-registration invalid-region precondition is exported as clause 9.10 | 1 |
| `RL-259` | p.187, 16-22 | Page 187 Delay Subscription Evaluation candidate fragments repeat the §8.1.10 crosswalk error | 2 |
| `RL-265` | p.288, 9 | Page 288 HLAfederate MOM candidate is assigned to 11.3 instead of 11.2 | 1 |
| `RL-267` | p.67, 22 | Page-67 Initiate Federate Save label candidate recurs under Federate Save Begun | 1 |
| `RL-317` | p.122, 73-80 | Page 122 Reflect Attribute Values preamble candidates are labeled 6.11.1 instead of 6.11 | 2 |
| `RL-323` | p.286, 18-36 | Evoke Callback postcondition candidate has Disable Callbacks clause label | 1 |
| `RL-324` | p.139, 2920-2925 | Attribute transportation default-on-reacquisition candidate has supplied-arguments clause label | 1 |
| `RL-325` | p.141, 2963-2968 | Attribute transportation one-request/one-confirmation candidate has change-default clause label | 1 |
| `RL-328` | pp.84, 124, 191–192, 194, 206–207, 286 | Queued-TSO save anchors recur across later-heading clause boundaries | 11 |
| `RL-329` | p.165, 14-19 | Page-165 Attribute Ownership Unavailable introduction inherits later 7.10.6 heading | 1 |
| `RL-330` | p.180, 9-68 | Page-180 mixed-passel message rule inherits later 8.1.4 heading | 5 |
| `RL-331` | p.153, 15-111 | Page-153 acquisition and privilege rules inherit later 7.1.4 heading | 12 |
| `RL-339` | p.203, 13-50 | Page-203 time-management candidates inherit §8.14.3 across service boundaries | 5 |
| `RL-340` | p.229 and 231, page-229 preconditions j; page-231 preconditions j | Pages 229 and 231 DDM invalid-region requirements are assigned to postconditions/reference-state subsections | 2 |
| `RL-341` | p.229 and 230, page-229 candidate l20; page-230 Associate Regions For Updates text | Update-region ownership-loss postcondition is attributed to Register Object Instance | 1 |
| `RL-342` | p.43-44, page-043 heading clause:4.1.2; page-044 lines 57-64 | Page-44 saved-message restore requirement is exported only under Clause 4 | 1 |
| `RL-343` | p.204, content-15161-page-204: §8.14.4(d) and (e) precede the §8.15 service heading; §8.15.3 is Preconditions | Page-204 TSO delivery cutoffs are exported under asynchronous-delivery preconditions | 2 |
| `RL-344` | p.62, content-15161-page-062: failed Register Federation Synchronization Point indication and no-effect rule precede the 4.16 Announce Synchronization Point heading | Page-62 failed synchronization-registration no-effect rule is assigned to the following service | 1 |
| `RL-345` | p.225, content-15161-page-225: region-copy lifetime guarantee follows the §9.1.7 Convey Region Designator Sets Switch material and precedes the §9.1.8 Allow Relaxed DDM heading | Page-225 region-designator copy-lifetime guarantee is assigned to §9.1.8 instead of §9.1.7 | 1 |
| `RL-346` | p.193, page 193 semantic flow: the first two statements are under §8.6.4 Postconditions; the Disable Time Constrained description is under §8.7 before its numbered subsections | Page-193 time-constrained requirements are assigned to Disable Time Constrained exceptions | 3 |
| `RL-347` | p.43-44, page-043 heading clause:4.1.2 Federation execution save and restore; page-044 per-federate-type count and same-type interchangeability statements precede the next §4.1.3 heading | Page-44 federate-type restore requirements continue to export only under Clause 4 | 2 |
| `RL-348` | p.57, page-057 §4.11 Join Federation Execution service description: the federate-type argument distinguishes categories for save and restore, before §4.11.1–§4.11.4 | Page-57 federate-type join requirement is exported only under Clause 4 | 1 |
| `RL-349` | p.202, content-15161-page-202 layout_body: Flush Queue Grant result paragraph after the §8.13 heading and before §8.13.1 through §8.13.3 | Page-202 Flush Queue Grant result is assigned to §8.13.3 preconditions | 2 |
| `RL-350` | p.182, content-15161-page-182 layout_body: FQG optimistic-lookahead restriction under §8.1.5 before the §8.1.6 heading | Page-182 optimistic Flush Queue lookahead rule is exported only under Clause 8 | 1 |
| `RL-351` | p.182, content-15161-page-182 layout_body: time-regulating TSO sender rule and timestamped-order statement within §8.1.5, before the §8.1.6 heading | Page-182 §8.1.5 time-management rules are exported only under Clause 8 | 2 |
| `RL-352` | p.182, content-15161-page-182 layout_body: Enable Time Constrained request/callback statement within §8.1.6, following its heading | Page-182 Enable Time Constrained transition is exported only under Clause 8 | 1 |
| `RL-356` | p.175, logical-time paragraph continuing beyond the truncated candidate | Page-175 logical-time requirement is truncated and clause-only | 1 |
| `RL-357` | p.57-58, page 57 §4.8 heading and page 58 required-representations paragraph | Page-58 time-representation requirement loses §4.8 attribution | 1 |
| `RL-360` | p.220, content-15161-page-220 layout_body, object-attribute update/subscription relationship paragraphs before the visible §9.1.3.3 heading | Page-220 object-attribute continuation is assigned to the interaction subsection | 16 |
| `RL-362` | p.192, TCE service narrative after §8.6 heading and before §8.6.1 supplied-arguments heading | Page-192 TCE service narrative is attributed to §8.6.3 | 5 |
| `RL-370` | p.181, content-15161-page-181 layout_body: TSO ordering statement under §8.1.4 Message order type | Page-181 TSO-order rule is exported only under Clause 8 | 1 |
| `RL-380` | p.216, content-element-content-15161-page-216-paragraph-001 | Page-216 DDM content element is classified as Clause 8 | 6 |
| `RL-384` | p.143, page-143 service paragraph 42-46; next heading at 47 | Page-143 Report Attribute Transportation Type candidate truncates its unowned rule and inherits §6.29.1 | 2 |
| `RL-385` | p.191, page 191 service description before the 8.5.1 heading; candidate lines 76-81 | Page-191 Enable Time Constrained result is assigned to §8.5.5 | 1 |

### Incomplete, fragmented, or corrupted candidate text (36)

| ID | Source | Complaint | Candidate records |
|---|---|---|---:|
| `RL-249` | p.109, 10 | Page-109 interaction-receipt requirement loses federate/subscription notation and is incomplete | 1 |
| `RL-250` | p.224, 89-100 | Page-224 Receive Interaction requirement is split across adjacent records | 2 |
| `RL-251` | p.224, 137-147 | Page-224 DDM reinterpretation requirement omits its service list | 1 |
| `RL-258` | p.187, 16-22 | Page 187 Delay Subscription Evaluation candidate fragments clip intact local source context | 2 |
| `RL-263` | p.290, 12-14 | Page 290 §11.4.1 candidate contains only a trailing new-parameters fragment | 1 |
| `RL-264` | p.290, 75-78 | Adjacent page 290 §11.4.1 candidates split the ignored-interaction statement | 2 |
| `RL-309` | p.148, 94-111 | Page 148 ownership paragraph is split between l94 and l100 requirement candidates | 4 |
| `RL-310` | p.148, 124-132 | Page 148 RTI ownership-control exception requirement omits its MOM interaction list | 1 |
| `RL-311` | p.148, 112-123 | Page 148 ownership-transfer rule is truncated across l112 and l121 | 2 |
| `RL-313` | p.178, 51-52 | Page 178 pre-grant delivery rule is truncated before its limiting clause | 1 |
| `RL-314` | p.294, 14-16 | Page 294 service-report log-format candidate truncates the record subject | 1 |
| `RL-315` | p.69, 64-75 | Page-069 Federation Saved success condition is split across three incomplete candidates | 3 |
| `RL-316` | p.195, 137-161 | Section 8.9 message-set candidate omits the enumerated delivery rules | 1 |
| `RL-318` | p.233, 30-41 | Page 233 regional-subscription discovery obligation is split across three candidate rows | 3 |
| `RL-319` | p.110, 90-91 | Annex C.3 extendable-variant candidate stops mid-conflict rule | 1 |
| `RL-320` | p.115, 104-115 | Page-115 Reserve Multiple Names candidates split illegal-name abort from callback result | 3 |
| `RL-326` | p.67, 37-44 | Timed Initiate Federate Save candidate on page 67 ends mid-sentence | 1 |
| `RL-327` | p.65, 109-113 | Timed Request Federation Save candidate on page 65 stops at page boundary | 1 |
| `RL-332` | p.180, 87-93 | Page-180 Delete Object Instance order rule is split mid-sentence | 2 |
| `RL-333` | p.192, 69-71 | Page-192 Time Constrained Enabled precondition ends mid-sentence | 1 |
| `RL-334` | p.192, 72-77 | Page-192 smallest-feasible-time requirement ends mid-sentence | 1 |
| `RL-335` | p.197, 98-110 | Page-197 Time Advance Grant result statement is split mid-sentence | 2 |
| `RL-337` | p.200, 33-44 | Page-200 Flush Queue Request delivery and grant rule is split across fragments | 5 |
| `RL-359` | p.219, page-219 layout_body, incomplete Set Range Bounds / Commit Region Modifications paragraph before the visible §9.1.3.2 heading | Commit Region candidate truncates its rule and misattributes the subsection | 1 |
| `RL-361` | p.220, content-15161-page-220 layout_body, interaction send-region dimension-subset sentence and following default-region sentence | Interaction dimension-subset rule is split and conflated with default-region behavior | 2 |
| `RL-373` | p.40, 33-53 | Page-40 federation-lifecycle candidate is truncated and combines distinct obligations | 1 |
| `RL-374` | p.213, 76-102 | Page-213 order-type paragraph is split into incomplete mixed candidates | 4 |
| `RL-375` | p.150, 153-160 | Page-150 unconditional-divestiture candidate omits its service invocation | 1 |
| `RL-376` | p.161, 152-172 | Page-161 §7.8 acquisition requirements are split and truncated | 3 |
| `RL-377` | p.148, 16-93 | Page-148 §7 overview and ownership-state candidates are split | 8 |
| `RL-378` | p.178, 93-110 | Page 178 OLT delivery-guarantee sentence is split across candidates | 2 |
| `RL-379` | p.216, 87-89 | Page 216 region-dimension candidate drops its subject and merges the next rule | 1 |
| `RL-381` | p.216, page-216 lines 66-80 | Page-216 §9.1.1 range definition is fragmented across four candidates | 4 |
| `RL-382` | p.44, page-44 source lines 10-17; candidate source range 15-26 | Page-44 save-state identity candidate starts mid-sentence and omits §4.1.3 | 1 |
| `RL-383` | p.40, page-40 source paragraph; candidate source range 54-59 | Page-40 Disconnect candidate omits its condition and §4.1.1 mapping | 1 |
| `RL-386` | p.198-199, page-198 candidate l192-57; page-199 continuation and zero-lookahead statement | Page-198 §8.11.3 equal-time grant rule is clipped before its zero-lookahead continuation | 1 |

## Full evidence and status source

- [Machine-readable issue register](../../compliance/requirements-lab/known-issues.json)
- [Detailed issue ledger](REQUIREMENTS-LAB-ISSUES.md)
- [Requirements Lab observations and historical notes](../testing/REQUIREMENTS-LAB-OBSERVATIONS.md)

The queue is a navigation and triage view, not a new requirements source. Keep issue status and evidence in sync with the machine-readable register when complaints are resolved or new ones are confirmed.
