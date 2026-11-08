# IEEE 1516.1-2025 Documentation Roadmap: Archived Progress

Completed progress entries moved verbatim from the active roadmap on 2026-11-07 to keep the actionable roadmap within its documentation size limit. Their order and substantive content are preserved. This file remains beside the guides so relative source links retain their original bases.

## 2025 topic-guide content audit — first increment — 2026-11-06

Set a concrete audit denominator of the 32 current 2025 topic guides. For each
guide, review five reader needs: (1) HLA concepts and the available standard or
API vocabulary, (2) state transitions and observable callbacks, (3) important
edge cases, (4) source and focused-test evidence with limits, and (5) open
questions and the edition/profile boundary. A guide counts as reviewed only
after a content read, not because a keyword or diagram is present.

Completed the first content pass for Auto Provide, Attribute Scope Advisory,
and Attribute Update-Rate Admission: **3/32 guides (9.4%) in this audit**. All
three distinguish standard/API vocabulary from implementation findings, show
the relevant decision or callback path, give concrete edge cases, and tie
claims to focused evidence and explicit limits. No documentation correction
was warranted in this increment. The audit preserved unresolved normative
questions: process Auto Provide solicitation is unverified; scope's cited
unsubscribe exception is not checked against licensed clause prose; and the
update-rate guide leaves the empty-inner-region interpretation pending
authorized §9.8.4 text. This percentage measures only this new review pass; it
does not measure overall documentation-goal completion or HLA coverage.

**Handoff completed in this increment:** Content-audit the 2025 Authorization
guide against the same five reader needs. Trace its public authorization
surfaces, decision points, callbacks or service outcomes, focused test
evidence, and explicit gaps. Keep the review within the 2025 stream and do not
infer normative requirements from the API declarations alone.

The Authorization guide has now completed that pass. It distinguishes the
three API authorization callback scopes, credential payloads, returned-result
mapping, the later service's own state checks, process caller-side gating,
observer reporting, thrown-authorizer behavior, and credential lifetime across
Disconnect. Its test table preserves the historical 7/10 result as historical
and explicitly identifies setup failures before the intended Destroy/Join
assertions. A targeted local-link check validated 94 references with no missing
files, invalid heading anchors, or out-of-range line anchors. No guide change
was warranted; no test was rerun.

The audit now stands at **4/32 guides (12.5%)**: Auto Provide, Attribute Scope
Advisory, Attribute Update-Rate Admission, and Authorization. This is only the
completion rate of the defined content-audit pass, not overall documentation
completion or HLA coverage.

**Handoff completed in this increment:** Content-audit the 2025 Exception
Reporting guide, especially the distinction between operation outcomes,
observer-side exception reports, and per-invocation service reports. Trace its
callback eligibility, switches, focused test evidence, and known gaps using
the same five reader needs.

## 2025 topic-guide content audit, increment 3 — Exception Reporting — 2026-11-06

Completed the Exception Reporting guide pass. It separates the caller-visible
service outcome from `HLAreportException`, MOM rejection
`HLAreportMOMexception`, and `HLAreportServiceInvocation`; maps switch,
membership, observer subscription/DDM, callback, and process transport gates;
and labels the combined-route and callback-failure schedules as source-derived
where focused tests do not cover them. It also preserves the MIM wording
ambiguity instead of resolving normative text from implementation behavior.
The guide's test descriptions distinguish passing scenarios from the missing
three-route combined cases. A targeted local-link audit checked 92 references
with no missing files, invalid heading anchors, or out-of-range line anchors.
No guide correction was warranted, and no tests or builds were run.

The content audit now stands at **5/32 guides (15.6%)**. This percentage
measures only the defined guide-review pass; it is not overall documentation
completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Service Invocation Reporting guide.
Trace which service outcomes generate a report, how file and interaction
sinks differ, what can change the caller-visible result, and which callback,
MOM, or Save/Restore paths remain separate or untested. Use the same five
reader needs and keep the 2010 stream out of scope.

**Handoff completed in this increment:** Content-audit Service Invocation
Reporting against those five reader needs.

## 2025 topic-guide content audit, increment 4 — Service Invocation Reporting — 2026-11-06

The guide cleanly separates the public service result, an invocation report,
and a request-driven MOM report. It explains the subject's switches and sink
selection separately from observer subscription/DDM eligibility, the exact
interaction interlock, per-member file lifetime, report serial reservation,
and callback timing. It also shows wrapper-specific failure behavior: some
report-writing errors can affect a caller-visible result after state has
committed, while the exception-report hook has a separate failure boundary.
Embedded and configured-process paths are explicitly distinct, and
source-only combined schedules remain labeled as such. The guide ties these
paths to focused tests and carefully records when test setup failed before the
target assertions. A targeted local-link audit checked 135 references with no
missing files, invalid heading anchors, or out-of-range line anchors. No guide
correction was warranted; no tests or builds were run.

The content audit now stands at **6/32 guides (18.8%)**. This percentage
measures this audit pass only, not overall documentation completion or HLA
coverage.

**Next handoff:** Content-audit the 2025 MOM Object and Interaction Reports
guide. Check how its MIM-defined request/report concepts connect to Umbra's
state and callback paths, edge cases, focused test evidence, and known gaps.
Keep request-driven MOM reports separate from Exception Reporting and Service
Invocation Reporting, and retain the 2025/2010 boundary.

## 2025 topic-guide content audit, increment 5 — MOM Object and Interaction Reports — 2026-11-06

The guide distinguishes request-driven MOM reports from Exception Reporting
and Service Invocation Reporting. It connects the 2025 MIM request/report
contracts to Umbra's per-federate activity ledgers, report planners, request
routing, callback payloads, and NULL/empty behavior. It keeps embedded test
evidence distinct from configured-process paths and labels source-only
conclusions and missing coverage. The review found and corrected five test
links that pointed into helper implementations instead of focused test case
declarations. Static checking validated 206 local references, including file
targets and line/heading anchors. No tests or builds were run.

The content audit now stands at **7/32 guides (21.9%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Callback and Service Ordering guide
against the same five reader needs, keeping the 2010 stream out of scope.

## 2025 topic-guide content audit, increment 6 — Callback and Service Ordering — 2026-11-06

The guide distinguishes HLA callback-model and service-specific ordering
concepts from Umbra dispatcher, transport, and callback-profile mechanics. It
covers callback admission, enable/disable, Evoke timing, HLA_IMMEDIATE
re-entry, state-commit cautions, lifecycle cleanup, callback exceptions, and
the limits of current implementation and test evidence. A targeted semantic
link review corrected test references that landed on nearby helper or blank
lines. It also clarified that a companion dispatcher test covers late callback
arrival during Evoke Multiple's minimum wait. Static validation checked 352
local references with no missing targets or invalid line or heading anchors.
No tests or builds were run.

The content audit now stands at **8/32 guides (25.0%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Object and Interaction Information
guide against the same five reader needs, keeping the 2010 stream out of scope.

## 2025 topic-guide content audit, increment 7 — Object and Interaction Information — 2026-11-06

The guide distinguishes object attributes, ordinary and directed
interactions, producer authority, receiver interest and instance knowledge,
DDM, order and time, and application observation. It connects those standard
concepts to separate 2025 Umbra paths and marks embedded/process, source/test,
and 2010 boundaries. A semantic test-link review corrected anchors that landed
on helper code or stale locations; the backlog heading link now uses its
GitHub-rendered slug. Static validation checked 418 local references with no
missing targets or invalid line or heading anchors. No tests or builds were
run.

The content audit now stands at **9/32 guides (28.1%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Directed Interaction guide against the
same five reader needs, keeping ordinary interaction and DDM boundaries clear
and the 2010 stream out of scope.

## 2025 topic-guide content audit, increment 8 — Directed Interaction — 2026-11-06

The guide distinguishes sender, target-object, and receiver identities; the
paired directed declaration matrix; sender registered-class checks from each
receiver's known-class selector; and directed selection from ordinary
subscriptions and DDM. It traces embedded and configured-process
receive-order and TSO routes, recipient-specific temporal and retraction state,
and explicit source/test gaps. A semantic link audit corrected stale test
references to declarations and retained assertion links where they target
specific evidence. Static validation checked 100 local references with no
missing targets or invalid line or heading anchors. No tests or builds were
run.

The content audit now stands at **10/32 guides (31.3%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 FOM Module Admission and Composition
guide against the same five reader needs, keeping the 2010 stream out of scope.

## 2025 topic-guide content audit, increment 9 — FOM Module Admission and Composition — 2026-11-06

The guide separates per-module DIF validation, ordered-set composition and
reference resolution, and FDD materialization. It traces Create preparation
before federation-name reservation and Join catalog/member replacement, and
keeps embedded behavior distinct from process evidence and the 2010 model
catalog compatibility lane. A semantic test-link review corrected two
references that landed on fixture or setup lines rather than focused tests.
Static validation checked 250 local references with no missing targets or
invalid line or heading anchors. No tests or builds were run.

The content audit now stands at **11/32 guides (34.4%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Federation and Federate Lifecycle
guide against the same five reader needs, keeping the 2010 behavior stream out
of scope.


## 2025 topic-guide content audit, increment 10 — Federation and Federate Lifecycle — 2026-11-06

The guide separates ambassador, membership, and execution lifetimes; voluntary
Resign and forced ConnectionLost; member/catalog snapshots and MOM observation;
and save/restore and TSO edge paths. It marks the 2010 boundary, embedded and
configured-process evidence, and unresolved normative limits explicitly. Static
validation checked 354 local destinations and 337 line/heading anchors with no
failures. No tests or builds were run.

The content audit now stands at **12/32 guides (37.5%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Ownership Persistence and Resignation
guide against the same five reader needs, keeping the 2010 stream
out of scope.


## 2025 topic-guide content audit, increment 11 — Ownership Persistence and Resignation — 2026-11-06

The guide separates HLA ownership facts from Umbra's saved typed ledgers, live
callback routes, and already-admitted HLA_EVOKED frames. It covers regular and
negotiated ownership work across fresh-registry restore, then contrasts that
with process callback-queue retention and restore-produced work. A semantic
link review corrected the pending-resignation test link from setup to the
rejection assertion. Static validation checked 41 local destinations and 36
line/heading anchors with no failures. No tests or builds were run.

The content audit now stands at **13/32 guides (40.6%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Attribute Ownership guide against the
same five reader needs, keeping the 2010 stream out of scope.


## 2025 topic-guide content audit, increment 12 — Attribute Ownership — 2026-11-06

The guide separates instance-attribute ownership from publication, receiver
knowledge, update eligibility, and object-delete privilege. It maps requester-
and owner-initiated transfer paths, callback/query boundaries, and distinct
embedded versus configured-process evidence. A semantic test-link review
corrected a process callback-timing claim that pointed to an unrelated
embedded test; it now links to the process post-lookup assertion. Static
validation checked 364 local destinations and 330 line/heading anchors with no
failures. The 31 existing diagrams were sufficient; no new chart was added.
No tests or builds were run.

The content audit now stands at **14/32 guides (43.8%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Federation Save/Restore guide against
the same five reader needs, keeping the 2010 stream out of scope.


## 2025 topic-guide content audit, increment 13 — Federation Save and Restore — 2026-11-06

The guide separates each federate's application checkpoint from the RTI's
federation image, request admission from protocol completion, and durable
route-free restoration from retained process-local snapshots and live callback
queues. It covers participant failure, identity and membership boundaries,
and selected Region/TSO persistence, while marking process, test, and 2010
limits. Semantic review corrected four stale test destinations and split
restore-status evidence across active and terminal assertions. Static validation
checked 408 local destinations and 390 line/heading anchors with no failures
across 32 diagrams. No tests or builds were run.

The content audit now stands at **15/32 guides (46.9%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Cross-Cutting State, Event, and
Observation guide against the same five reader needs, keeping the 2010 stream
out of scope.


## 2025 topic-guide content audit, increment 14 — Cross-Cutting State, Event, and Observation — 2026-11-06

The guide supplies a portable reasoning lens for authority/admission, state,
reachability/eligibility, and observation/completion. It distinguishes that
lens from service-specific protocols and implementation architecture, and
keeps 2010, profile, and normative boundaries explicit. A semantic link review
corrected four stale test destinations for Non-Regulated Grant, callback
dispatch, and FOM-preparation evidence. Static validation checked 151 local
destinations and 114 line/heading anchors with no failures across 13 diagrams.
No tests or builds were run.

The content audit now stands at **16/32 guides (50.0%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Data Distribution and Regions guide
against the same five reader needs, keeping the 2010 stream out of scope.


## 2025 topic-guide content audit, increment 15 — Data Distribution and Regions — 2026-11-06

Reviewed the guide against portable DDM concepts, state and recipient-eligibility
transitions, observable effects, edge cases, focused evidence, and edition/profile
boundaries. The guide explains dimensions and bounds, pending versus committed
region extents, regional associations, overlap and relaxed DDM, receiver
subscription/relevance, and time/callback gates. Its implementation details are
marked as Umbra evidence, with embedded and configured-process limits stated; it
does not claim 2010 parity. Corrected the configured-process Delete Region test
link to the actual public call at line 1375 and refreshed the guide's render and
selected screenshot inventory (459 rendered, 229 screenshot-reviewed; GitHub
rendering unverified). No new diagram was needed. Static validation checked 485
local destinations and 446 anchors with no failures across 31 diagrams. No tests
or builds were run.

The content audit now stands at **17/32 guides (53.1%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 MOM Object and Interaction Reports guide
against the same five reader needs. Keep request-driven MOM reports distinct from
Exception Reporting and Service Invocation Reporting, and retain the 2025/2010
boundary.


## 2025 topic-guide content audit, increment 16 — MOM Object and Interaction Reports — 2026-11-06

Reviewed the guide against MIM request/report concepts, state and callback paths,
edge cases, evidence, and edition boundaries. It clearly separates target
federate from requester, current object/ownership views from retained activity
ledgers, distinct-instance counts from invocation/callback counts, and broad
from directed interaction reports. The pinned MIM's conflicting NULL language
and the object-information target-parameter/source mismatch are surfaced without
claiming normative equivalence or conformance. Embedded focused cases support
selected branches; configured-process report behavior remains unverified and
source-only. Request-driven MOM reports remain distinct from Exception Reporting
and Service Invocation Reporting. Static validation found 206 local destinations
and 195 anchors resolving across eight diagrams. No tests, builds, or diagram
changes were needed.

The content audit now stands at **18/32 guides (56.3%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Federation MOM guide. Focus on execution-
scoped object state, lifecycle/report projection, callback timing, edge cases,
focused evidence and gaps, and keep it distinct from joined-federate MOM reports
and the 2010 stream.


## 2025 topic-guide content audit, increment 17 — Federation MOM — 2026-11-06

Reviewed the guide against the five reader needs. It distinguishes the federation-
scoped HLAfederation object, conditional attribute updates, request/report
interactions, and member-scoped HLAfederate objects. It explains separate roster
and member-object callback paths, execution lifetime, additional-module Join and
Restore projections, save-state fields, and synchronization-query snapshots.
The legacy-2010-FOM compatibility gate is clearly labeled as behavior inside the
2025 runtime, not evidence about the separate 2010 RTI. Source and focused-test
evidence have explicit limits, including callback-order, post-commit rollback,
and configured-process projection gaps. Static validation checked 401 local
destinations and 390 anchors with no failures across 25 diagrams. No tests,
builds, or diagram changes were needed.

The content audit now stands at **19/32 guides (59.4%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Federation Synchronization guide.
Examine synchronization-point registration and announcement, achievement and
federate synchronization, save/restore interactions, callback timing, and
focused evidence; keep the 2025 and 2010 streams separate.


## 2025 topic-guide content audit, increment 18 — Federation Synchronization — 2026-11-06

Reviewed the guide against standard vocabulary, state transitions, callback
outcomes, edge cases, evidence, and edition boundaries. It distinguishes
registration, announcement, individual achievement, and federation completion;
separates synchronization barriers from logical time and membership cleanup;
and covers expandable late-Join sets, partial results, first-result behavior,
member departure, callback failure after closure, and in-flight Save/Restore.
The guide marks source-derived process ordering and persistence details as such.
It explicitly leaves multi-member partial-result Restore, repeated-result
requirements, forced-loss delivery, and configured-process Save/Restore unproven.
Its 2010 guide remains separately linked without cross-edition inference. Static
validation checked 217 local destinations and 210 anchors with no failures across
15 diagrams. No tests, builds, or diagram changes were needed.

The content audit now stands at **20/32 guides (62.5%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Joined-Federate MOM Object Lifecycle
guide. Focus on per-member object creation, attribute state, callbacks and
removal, Join/Restore/Resign transitions, focused evidence, and its boundary with
the execution-scoped Federation MOM; keep the 2010 stream separate.


## 2025 topic-guide content audit, increment 19 — Joined-Federate MOM Object Lifecycle — 2026-11-06

Reviewed the guide against per-member object lifecycle, attribute projections,
callbacks, Join/Restore/Resign transitions, evidence, and edition boundaries.
It distinguishes membership from each observer's knowledge, the private Join
snapshot from the public HLAfederate object, Join-time values from dynamic
state, and explicit value requests from periodic refresh. It details Restore's
MOM projection, callback-time removal and snapshot retention, throwing Remove
callbacks, and process push/poll/connection-loss boundaries. The 2010 section
clearly limits its statement to the 2025 runtime's compatibility guard and
claims no separate 2010 MOM parity. Corrected two broken links to the callback
guide's HLA_EVOKED and HLA_IMMEDIATE headings. Static validation checked 196
local destinations and 192 anchors with no failures across 15 diagrams. No
tests, builds, or diagram changes were needed.

The content audit now stands at **21/32 guides (65.6%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Connection and Configuration guide.
Focus on configuration inputs, connection establishment and failure, callback
model setup, process/embedded boundaries, focused evidence and gaps, and keep
the 2025 and 2010 streams separate.


## 2025 topic-guide content audit, increment 20 — Connection and Configuration — 2026-11-06

Reviewed the guide against service vocabulary, connection state transitions,
callbacks, failure paths, source/test evidence, and edition boundaries. It clearly
separates ambassador connection from federation membership and Disconnect from
Resign. It distinguishes HLA/API behavior from Umbra configuration-name
fallbacks, authorization/RID settings, TLS URI and port choices, filesystem
settings, and process transport implementation. It covers setup ordering,
failed-connect recovery, ordered Disconnect cleanup, and the unverified concurrent
process reconnect window. The guide labels process-host cleanup evidence and
2010 compatibility boundaries without claiming process/embedded or 2010 parity.
Static validation checked 89 local destinations and 84 anchors with no failures
across six diagrams. No tests, builds, or diagram changes were needed.

The content audit now stands at **22/32 guides (68.8%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Process Endpoint Request and Callback
guide. Focus on public request/response boundaries, push and poll delivery,
callback admission and dispatch, session identity and failure, selected tests,
and process/embedded and 2025/2010 boundaries.


## 2025 topic-guide content audit, increment 21 — Process Endpoint Request and Callback — 2026-11-06

Reviewed the guide against public request/response behavior, push/poll delivery,
callback admission and dispatch, session identity/failure, evidence, and edition
boundaries. It tracks HLA service outcomes separately from private request IDs,
recipient events, and transport framing; distinguishes server push/poll from
HLA_IMMEDIATE/HLA_EVOKED callback timing; and follows interaction/update delivery
through demultiplexing, per-recipient projection, callback gates, TSO ACKs, and
recovery. The guide distinguishes source-derived mechanics from focused public
and private test results and records environment-limited failures without
relabeling them as behavior evidence. Its stated scope is 2025 process endpoint;
it does not infer embedded parity or 2010 behavior. Static validation checked
260 local destinations and 239 anchors with no failures across 19 diagrams. No
tests, builds, or diagram changes were needed.

The content audit now stands at **23/32 guides (71.9%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Attribute Value Update Request guide.
Focus on request admission, provider selection, update callback behavior,
response boundaries, important edge cases, focused evidence, and explicit
profile/edition limits.


## 2025 topic-guide content audit, increment 22 — Attribute Value Update Request — 2026-11-06

Reviewed the guide against the HLA solicitation contract, provider selection,
callback and later-response boundaries, edge cases, focused evidence, and profile
limits. It clearly states that a request is not a value transfer or an entitlement
to receive the later update. It separates request tag from response tag, provider
selection from receiver/DDM/time eligibility, and receive-order admission from
TSO eligibility. The implementation walkthrough covers grouping requested work
by object/provider, callback-time rechecks without provider reselection, regional
request scope versus the later Update's source region, and pending requests over
Save/Restore. It labels multi-provider fan-out, mid-flight ownership changes,
partial failure/replay, and process callback outcomes according to their direct
test or source-only status, without importing 2010 behavior. Fixed four broken
HLA_EVOKED/HLA_IMMEDIATE callback-guide anchors. Static validation checked 261
local destinations and 242 anchors with no failures across 14 diagrams. No
tests, builds, or diagram changes were needed.

The content audit now stands at **24/32 guides (75.0%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Object Attribute Subscription State
guide. Focus on publication, subscription, object relevance/knowledge, class
hierarchy and region effects, transition callbacks, focused evidence, and its
boundary with provider solicitation; keep the 2025/2010 streams separate.


## 2025 topic-guide content audit, increment 23 — Object Attribute Subscription State — 2026-11-06

Reviewed the guide against HLA subscription concepts, state transitions, callbacks,
edge cases, focused evidence, and edition boundaries. It separates receiver
interest from publication, object knowledge, discovery, and actual value
reflection; distinguishes active relevance from passive recipient eligibility;
and treats class ancestry as separate discovery, relevance, and value-projection
decisions. It covers ordinary and regional declaration mutation, unsubscribe
semantics, late subscription, rate-history generations, callback planning, DDM,
and process-specific delivery. Empty-set unsubscribe behavior is explicitly an
embedded/process source seam, not a normative conclusion. The guide leaves the
already-known-object/value-only case and licensed §5.8 interpretation unresolved
and makes no broad profile-parity or 2010 claim. Static validation checked 193
local destinations and 176 anchors with no failures across 12 diagrams. No tests,
builds, or diagram changes were needed.

The content audit now stands at **25/32 guides (78.1%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Delay Subscription Evaluation guide.
Focus on candidate and admitted subscription state, enable/disable transitions,
object/class/region scope, callback timing, interaction with update-rate and
relevance gates, focused evidence, and 2025/2010 boundaries.


## 2025 topic-guide content audit, increment 24 — Delay Subscription Evaluation — 2026-11-06

Reviewed the guide against the HLA-facing eligibility-timing concept, transitions,
callback/time gates, evidence, and edition/profile boundaries. It explains that
DSE changes when receiver subscription/relevance is evaluated, not sender-side
validity or guaranteed delivery. It separates callback model from eligibility,
receive-order temporal gates from TSO grant eligibility, and retained Umbra
route-only candidates from normative HLA state. It covers subscribe/unsubscribe
rechecks, regions, directed selectors, update-rate interactions, and current
process/embedded evidence limits. Configured-process focused coverage is limited
to regional receive-order attributes and one callback/Resign fence; other process
routes remain source-observed only. The §8.1.8 normative paraphrase retains its
authorized-text verification limit, and no 2010 behavior is inferred. Static
validation checked 93 local destinations and 87 anchors with no failures across
four diagrams. No tests, builds, or diagram changes were needed.

The content audit now stands at **26/32 guides (81.3%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Relevance Advisory guide. Focus on
publication/subscription relevance signals, scope and ownership boundaries,
advisory transitions, their separation from discovery/value delivery, focused
evidence, and explicit process/edition limits.


## 2025 topic-guide content audit, increment 25 — Relevance Advisory — 2026-11-06

Reviewed the guide against advisory concepts, triggers, recipients, callback
effects, evidence, and edition/profile boundaries. It clearly distinguishes
class-level declaration relevance (publisher-facing), per-instance attribute
relevance (owner-facing), receiver-local scope, discovery, reflection, and
update-rate admission. It maps switches and separate state calculations, covers
regional relevance and class-basis policy, and states that advisories are not
value delivery. Embedded and configured-process evidence remain scoped: process
attribute On/Off paths have focused tests, while process class-level declaration
callback delivery is a source-observed gap. It makes no cross-profile equivalence
or 2010 claim. Static validation checked 61 local destinations and 52 anchors
with no failures across four diagrams. No tests, builds, or diagram changes were
needed.

The content audit now stands at **27/32 guides (84.4%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Order and Transportation guide. Focus
on receive versus timestamp order, transportation selection and encoding,
service-specific constraints, callbacks and timing gates, focused evidence, and
process/profile and 2025/2010 boundaries.


## 2025 topic-guide content audit, increment 26 — Order and Transportation — 2026-11-06

Reviewed the guide against the HLA meanings of order and transportation, state
changes, callbacks, timing gates, evidence, and edition/profile boundaries. It
keeps order and transportation independent and neither as a substitute for
recipient selection, DDM, or logical-time eligibility. It covers sent versus
received order fields, distinct recipient-order conversion, class defaults versus
instance settings, pending transportation-change confirmation, ownership and
Save/Restore interactions, process push/poll checkpoints, and query snapshots.
Transportation examples are explicitly not wire-reliability measurements. The
normative sender-designation/receiver-order mapping remains open pending
authorized clause text; no 2010 or broad process/embedded inference is made.
Static validation checked 162 local destinations and 147 anchors with no failures
across 11 diagrams. No tests, builds, or diagram changes were needed.

The content audit now stands at **28/32 guides (87.5%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Data Element Encoding guide. Focus on
portable element/type contracts, octet layout and byte order, conversion and
ownership semantics, malformed input and boundary cases, focused evidence, and
2025/2010 separation.


## 2025 topic-guide content audit, increment 27 — Data Element Encoding — 2026-11-06

Reviewed the guide against the C++ DataElement contract, octet layout, byte-order
selection, ownership/cursor semantics, malformed inputs, evidence, and edition
boundaries. It separates HLA value bytes from handle encodings and private
process-frame bytes; records byte order per concrete type; and explains scalar,
string, composite/alignment, variant, and factory-defined logical-time formats.
It distinguishes whole-value decode from nested cursor decoding and marks source-
derived parent continuation after an unknown extendable-variant payload as not
covered by a combined focused test. The shared byte-order primitive is described
narrowly; 2010 wrappers and composite rules remain separate, with no parity
inference. Static validation checked 115 local destinations and 92 anchors with
no failures across nine diagrams. The current inventory records local render
coverage for all 459 diagrams, while an older per-guide note records eight
DataElement diagrams rendered on 2026-11-03; this count discrepancy is noted,
and GitHub rendering remains unverified. No tests, builds, or diagram changes
were needed.

The content audit now stands at **29/32 guides (90.6%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Content-audit the 2025 Name and Handle Resolution guide. Focus
on typed handle/name identity, lookup and normalization paths, unknown/invalid
behavior, object knowledge and lifecycle effects, focused evidence, and explicit
2025/2010 boundaries.


## 2025 topic-guide content audit, increment 28 — Name and Handle Resolution — 2026-11-06

Reviewed the guide against typed identity, lookup/normalization paths, failure
distinctions, object knowledge and lifecycle, evidence, and edition boundaries.
It separates execution-scoped model symbols, active federate identity, and
federate-local object-instance knowledge. It keeps category-typed handles
distinct from integers, hashes, normalized coordinates, and encoded bytes, and
explains class-scoped attribute/parameter resolution and typed failure results.
The guide covers additional-module Join lookup, Resign and same-name rejoin,
object discovery/forget/delete boundaries, Restore identity, and embedded versus
process lookup timing. It labels process evidence and the TCK's limited
repeatability scope, without asserting portability across RTIs or 2010 parity.
Static validation checked 137 local destinations and 125 anchors with no
failures across ten diagrams. Local render and selected screenshot status are
recorded; GitHub rendering remains pending after publication. No tests, builds,
or diagram changes were needed.

The content audit now stands at **30/32 guides (93.8%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

## 2025 topic-guide content audit, increment 29 — Object Instance Name Reservation — 2026-11-06

Reviewed the guide's 2025 name-reservation lifecycle, uniqueness and owner
scope, synchronous admission versus callback outcomes, partial-success bulk
reservation, atomic bulk release, registration commit/consumption, and release,
resign, save/restore, deletion, and timestamped-deletion boundaries. The guide
distinguishes reservation outcome from registration and peer discovery, and
marks embedded/process observations, untested interleavings, and licensed-text
limits. Its edition boundary correctly treats the 2025 API accepting a 2010 FOM
as 2025 runtime evidence, not the separate 2010 RTI stream. Static link
validation checked 87 local destinations with no failures after refreshing
stale source/test line references. Existing Mermaid diagrams and evidence
limits were coherent; no chart change was needed. No tests, builds, CI,
Requirements Lab, or 2010 implementation changes were made.

The content audit now stands at **31/32 guides (96.9%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

## 2025 topic-guide content audit, increment 30 — TSO Retraction — 2026-11-06

Completed the content audit of the final 2025 topic guide. The TSO Retraction
guide distinguishes producer time-bound legality, per-recipient pending versus
delivered outcomes, receiver-owned consequences, and independent time-advance
effects. It covers embedded ledger gates, ownership and regional delivery,
Delete-specific restoration, configured-process push/pull routes, callback
buffering, receiver Resign, and focused evidence limits. Normative orientation
remains explicitly bounded by licensed §§8.22–8.23 text; process and embedded
observations remain separate, with no 2010 parity claim. Shortened labels in
two process deletion sequences to improve preview readability. Mermaid CLI
12.0.0 rendered all 16 diagrams; all 16 local screenshots were reviewed and
remain legible. Static link validation checked 128 local destinations with no
failures. GitHub rendering and page sizing remain unverified. No tests, builds,
CI, Requirements Lab, or 2010 work.

The content audit now stands at **32/32 guides (100%)**. This measures this
audit pass only, not overall documentation completion or HLA coverage.

**Next handoff:** Cross-review the TSO Retraction guide with the 2025 Save/Restore,
Ownership Persistence and Resignation, and Federation Lifecycle guides. Verify
that timestamped deletion snapshots, recipient membership/Resign gates, and
restored callback or ownership state use consistent terms and evidence limits;
update only genuine cross-guide mismatches. Keep 2010 separate and retain the
open licensed-text boundaries.

## 2025 cross-guide coherence audit, increment 31 — TSO retraction lifecycle bridges — 2026-11-06

Cross-read the TSO Retraction guide with Save/Restore, Ownership Persistence and
Resignation, and Federation Lifecycle. No state or evidence contradiction was
found: Save/Restore has direct embedded fresh-registry cases for a queued
timestamped Delete and a later successful Retract, including one delivered and
one still-pending recipient; those cases run in one test process and do not
establish process-endpoint or OS-restart behavior. The TSO guide separately
limits ownership-callback interactions after Delete/Retract to source-derived
possibilities without a focused public test. The ownership companion
intentionally excludes TSO scheduling, while Lifecycle covers selected
voluntary `ResignAction` cleanup and distinguishes forced `ConnectionLost`.
Added reciprocal links and clarified that Resign's `DELETE_OBJECTS` effects do
not describe timestamped Delete/retraction gates. A three-guide link validation
checked 414 unique local destinations with no failures; `git diff --check`
passed. No diagrams, tests, builds, CI, Requirements Lab, or 2010 content
changed.

**Next handoff:** When authorized IEEE 1516.1-2025 §9.1.5 text is available,
compare its regional-subscription rule with the bounded default/regional routing
explanation in the DDM guide. Update the portable explanation only if the
canonical text changes it; keep the current embedded result and its evidence
separate. Until then, retain this as standards-text-dependent work and continue
the evidence-gated 2025 documentation backlog without inferring the rule.

## 2025 selected diagram visual audit, increment 32 — synchronization and Restore cases — 2026-11-06

Rendered and locally screenshot-reviewed five 2025 diagrams with Mermaid CLI
12.0.0: the configured-process synchronization registration/final-achievement
sequence, the fresh-registry timestamped Delete restore, restored-Delete
Retract, a saved regional TSO interaction, and the restored update-region
association case. The first sequence had overlapping notes; shortened them
while preserving push, polling, callback, and report ordering. The Delete/Retract
chart's clipped ownership note was shortened. The regional TSO chart's clipped
notes were shortened while keeping save initiation before the constrained grant,
the restored retraction record, and both distinct Retract outcomes. The
update-region chart was too wide to read with six lifelines; consolidated old
and fresh registry and image state while preserving the same-process boundary,
explicit rejoin of all three members, restored bounds, and two queued recipient
reflections. All five final diagrams rendered and were reviewed locally; labels
are legible and unclipped. `git diff --check` passed. Inventory remains 32 guides
/ 459 diagrams; selected screenshot review advances from 223 to 228. GitHub page
rendering and whole-inventory visual review remain unverified. No tests, builds,
CI, Requirements Lab, or 2010 work.

**Next handoff:** Visually review the 2025 Save/Restore guide diagram “Worked
case: a restored regional Update outlives its producer's Resign.” Check that the
saved update-region association, producer resignation, receiver eligibility,
and callback/grant sequence remain readable and consistent with the DDM and
Lifecycle guides. Preserve the embedded test boundary and avoid implying
configured-process parity.

## 2025 regional Update Restore/Resign coherence review, increment 33 — 2026-11-06

Cross-read the Save/Restore worked case with the DDM source-resignation example,
the Lifecycle ResignAction discussion, both focused Restore/Resign tests, and
the registry cleanup and TSO rehydration sources. The two Update tests use
`UNCONDITIONALLY_DIVEST_ATTRIBUTES`; the DDM regional-interaction case uses
`NO_ACTION` and separately verifies that a post-Resign `Retract` fails with
`FederateNotExecutionMember`. The Lifecycle guide's unconditional-divestiture
example covers accepted timestamped-deletion copies, a different message family.
The Update tests do not call `Retract` after Resign; callback handle validity
alone does not establish that the departed producer can use that service. The
existing prose already bounded the in-place embedded Restore tests, captured
source-region snapshot, callback/grant observation, per-receiver flushing, and
missing process/2010 evidence. Added direct cross-guide and action-call links,
and made the chart show the Region commit and Resign action as separate events.
Mermaid CLI 12.0.0 rendered the final chart; local screenshot review found it
readable and unclipped. Four newly added local source/test/heading targets
resolve, and `git diff --check` passes. Inventory remains 32 guides / 459
diagrams; selected screenshot review advances from 228 to 229. No tests, builds,
CI, Requirements Lab, or 2010 work.

**Next handoff:** Visually review the Save/Restore diagram “Worked case: a timed
save retains later directed TSO work.” Check that its save-time cutoff, later
message delivery, Restore and Retract states, and Flush Queue callback/grant
ordering remain readable against the Time Management guide. Keep this selected
embedded `HLA_EVOKED` trace separate from normative and configured-process claims.

## 2025 timed Save and later directed TSO visual/coherence review, increment 34 — 2026-11-06

Cross-checked the worked sequence against its embedded `HLA_EVOKED` test, the
Time Management FQR explanation, and the dispatcher. The time-8 directed
interaction remains pending when the time-6 save completes; the test retracts
it, restores the saved state, then observes time-8 delivery before FQG reports
actual 7 and optimistic 8. The dispatcher extracts the current recipient set
through final time for FQR, separately from the calculated grant and optimistic
floor. Clarified that separation, linked the exact Time Management section and
FQR extraction source, and corrected the callback label to “Flush Queue Grant
(FQG).” Mermaid CLI 12.0.0 rendered the revised sequence; local screenshot
review found it readable and unclipped. The test asserts directed-interaction
then FQG callback order and that `evokeCallback(0.0)` finds no remaining
callback. This remains one embedded implementation case, not normative or
configured-process evidence. Selected screenshot review advances from 229 to
230 of 459; whole-inventory visual and GitHub rendering remain unverified. No
tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Continue with the next unreconciled item in the selected
visual-QA queue, checking its current chart, source/test boundary, and guide
context before editing. Record its exact topic in the next handoff after
consulting the roadmap queue; do not treat the 230/459 sample as a completion
percentage.

## 2025 FQR callback-order wayfinding cross-review, increment 35 — 2026-11-06

Cross-checked the new Save/Restore FQR callback explanation against the
Callback and Service Ordering guide. The latter explains configured-process
Evoke admission and draining but had no route to the embedded FQR case, making
it easy to miss that the interaction-before-FQG observation comes from a
different profile path. Added a short, evidence-limited link paragraph to the
callback guide, tied to the existing test and paired with the Time Management
delivery-frontier explanation. This adds navigation, not a callback rule or
profile-equivalence claim. The FQR Save/Restore chart remains visually reviewed
at increment 34; no chart count change. The guide heading and referenced FQR
section resolve. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Cross-audit the configured-process FQR discussion in the
Time Management and Process Endpoint guides against current process source and
existing tests. Keep the embedded FQG-before-next-Evoke result distinct; add a
process sequence only if a concrete tested or source-grounded reader gap exists.

## 2025 configured-process FQR wayfinding and evidence review, increment 36 — 2026-11-06

Cross-checked the Process Endpoint guide's FQR-arrival note against the paired
Time Management profile comparison, the public configured-process test, and
the process grant dispatcher. The test accepts FQR(10), then submits timestamps
9 and 12 and retracts 12 before callback entry; this accepted request reports
actual 5 / optimistic 10 without delivering either later interaction. The
embedded test in the paired comparison admits timestamp 9 during callback-time
extraction and suppresses the retracted 12. The process dispatcher extracts
the recipient queue through final time before committing the FQG state and
publishing its result. Added direct links to the paired explanation, exact test
steps, and dispatcher. Kept the distinction as profile-specific implementation
evidence, not a transport-cutoff requirement or a parity claim. No new chart
was needed because Time Management already owns the comparison diagram. The
new guide and source links resolve; `git diff --check` passes. No tests, builds,
CI, Requirements Lab, or 2010 work.

**Next handoff:** Cross-audit the configured-process FQR dispatcher’s TSO
callback acknowledgment boundary against the Process Endpoint guide’s per-
recipient TSO lifecycle and callback-bridge discussion. Determine whether the
guide makes callback entry, completion acknowledgment, and FQG publication
distinct enough for a reader; add only a source/test-supported clarification.

## 2025 process FQR and TSO ACK boundary, increment 37 — 2026-11-06

The Process Endpoint guide already showed that a TSO recipient remains
`inTransit` until its callback completion ACK, but did not connect that lifetime
to the configured-process FQR grant event. Added a focused sequence showing
the coordinator sends eligible TSO frames, leaves their recipient entries
`inTransit`, commits actual/optimistic times, and publishes FQG without waiting
for callback completion. The client later invokes Receive Interaction and
sends the private ACK; a subsequent Evoke invokes FQG. The process GALT/FQR
test asserts the public callback order across two Evokes and the fixture handles
the ACK separately. Source and test links are explicit. Mermaid CLI 12.0.0
rendered the new chart and local screenshot review found it readable and
unclipped. This is configured-process implementation evidence; no HLA callback
order or wall-clock transport guarantee is claimed. Selected screenshot review
advances from 230 to 231 of 459. No tests, builds, CI, Requirements Lab, or
2010 work.

**Next handoff:** Review the configured-process FQR path when the timestamped
Receive Interaction callback throws. Trace whether the FQG event remains
deliverable, how the missing TSO ACK leaves recipient state in transit, and
what the callback dispatcher does with the remaining queued FQG callback.
Keep this source-derived unless an existing focused test covers the exact
interleaving; do not infer an HLA callback or recovery rule.

## 2025 process FQR Receive Interaction exception branch, increment 38 — 2026-11-06

Extended the configured-process FQR sequence with the throwing
`receiveInteraction` branch. The bridge invokes its private TSO completion only
after user callback return, so a throw skips ACK and leaves the coordinator
recipient `inTransit`. The server publishes the FQG event independently of
that ACK. In `HLA_EVOKED`, the dispatcher has removed the throwing task before
invoking it and preserves later tasks; if FQG has not yet reached that queue,
its event remains in the process FIFO for a later pump. Thus FQG remains
deliverable on a later callback pass under the inspected connected path, while
the TSO recipient ledger remains uncompleted. No focused process test throws
from `receiveInteraction` during FQR, so this branch is explicitly
source-derived and makes no recovery or HLA callback-order guarantee. Mermaid
CLI 12.0.0 rendered the updated sequence; screenshot review found it readable
and unclipped. Local inventory recount found 460 diagrams (263 flowcharts, 172
sequences, 25 state diagrams), all locally rendered; selected screenshot review
advances from 231 to 232. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Cross-audit the process FQR exception branch against the
`HLA_IMMEDIATE` event-pump path. Check immediate-drainer cleanup against client
event-FIFO extraction and determine whether a later FQG remains deliverable
after the throwing callback. Keep the result source-derived unless a focused
test covers the interleaving.

## 2025 process FQR exception behavior under HLA_IMMEDIATE, increment 39 — 2026-11-06

Extended the Process Endpoint FQR sequence to compare both callback models.
Under `HLA_IMMEDIATE`, the client event pump extracts Receive Interaction and
submits it to the immediate drainer inline. If user code throws, that event is
consumed, the private TSO ACK is skipped, and the current event-pump pass exits
before extracting a later FQG event. The client dispatch loop stores the
exception and continues; a buffered or later-arriving FQG remains in the client
event FIFO and can be submitted on a later pump while the recipient ledger
remains `inTransit`. The stored callback exception is rethrown at the start of
a later service request, independently of FQG callback entry. Contrasted this
with `HLA_EVOKED`, where a submitted FQG task can remain in dispatcher FIFO
after the throwing Receive Interaction task is consumed. No focused public
process test covers this throw interleaving, so both branches remain
source-derived; no recovery, callback-order, or cross-profile HLA rule is
claimed. Mermaid CLI 12.0.0 rendered the updated sequence and screenshot review
found it readable and unclipped. Inventory remains 460 diagrams; selected
screenshot review advances from 232 to 233. No tests, builds, CI, Requirements
Lab, or 2010 work.

**Next handoff:** Cross-audit the process FQR exception branch against the
forced-resignation and client-close cleanup paths while the recipient ledger
remains `inTransit`. Keep lifecycle cleanup separate from callback ACK and
delivery; source-only unless a focused test covers the interleaving.

## 2025 process FQR recipient cleanup on Resign and client close, increment 40 — 2026-11-06

Traced the terminal cleanup paths after an FQR Receive Interaction callback
throws. A successful voluntary Resign or detected `ConnectionLost` unregisters
the member; `FederationTimeCoordinator::unregisterFederate` calls
`discardTsoRecipient`, removing that member's queued, in-transit, delivered,
and retired TSO recipient entries. Successful voluntary Resign clears the
client event FIFO and pending callback tasks. A forced-resignation event clears
the client event FIFO and deferred ACKs, but does not itself clear already-
submitted dispatcher tasks. Client close clears the FIFO/deferred ACKs and
closes the bridge, which resets its dispatcher. These are lifetime cleanup
transitions, not completion ACKs; the local cleanup effect depends on which
queue holds FQG when departure occurs. No focused test combines FQR callback
throw with Resign, forced `ConnectionLost`, or close. The Process Endpoint guide
now links this implementation seam to the separate 2025 lifecycle guide and
keeps the outcome source-derived. Mermaid CLI 12.0.0 rendered the revised
sequence and screenshot review found it readable and unclipped. Inventory
remains 460 diagrams; selected screenshot coverage remains 233/460 because the
revised chart was already in the reviewed set. No tests, builds, CI,
Requirements Lab, or 2010 work.

**Next handoff:** Audit forced `ConnectionLost` from the producer side against
recipient-side `discardTsoRecipient`: identify how queued TSO messages produced
by the departing member are retained or retired for surviving recipients, and
keep IEEE 1516.1-2025 §4.4 cutoff obligations separate from recipient cleanup.
Do not infer the producer and recipient paths are symmetric.

## 2025 configured-process ConnectionLost TSO cutoff boundary, increment 41 — 2026-11-06

Compared the producer-side forced-loss path with the recipient-side TSO cleanup.
The shared registry marks a lost time-regulating producer's accepted TSO
messages for cutoff delivery at or before its last granted time; unregistering
the departed member discards only the TSO ledger addressed to that member.
Surviving recipients' marked messages are a separate ledger. The existing
at-cutoff and later-TAR tests use the embedded fixture. In the configured
process adapter, `connectionLost` calls the shared registry transition and
dispatches the `ConnectionLossResult` but does not directly re-evaluate time
grants; explicit Resign and the forced-resignation test path do. A later process
TAR enters the grant scheduler and can select the marked-message exception, but
the inspected path and tests do not establish immediate release of a survivor
TAR that was already pending at producer loss. Added this source/test boundary
to the Lifecycle guide; it is not a normative noncompliance or profile-parity
claim. No chart changed, so inventory and selected visual review remain 460 and
233/460. Local source links and `git diff --check` pass. No tests, builds, CI,
Requirements Lab, or 2010 work.

## 2025 additional-module Join evidence reconciliation, increment 42 — 2026-11-06

Reconciled the portfolio summary with the checked-in embedded test and the
detailed FOM guide. A focused 2025 embedded case successfully adds an attribute
to an existing class after an instance is registered; it observes discovery
of that instance and ownership only after the If-Available acquisition
notification. It does not send or observe a value update, so known-receiver
current-value reflection and the normative §5.8 interpretation remain open.
The configured-process additional-FOM case uses an object-only extension and
checks added object-class lookup; it does not establish add-attribute behavior
or profile parity. Corrected the stale portfolio sentence that said no
successful add-attribute Join test exists. No chart changed. No tests, builds,
CI, Requirements Lab, or 2010 work.

## 2025 configured-process Join interaction boundary, increment 43 — 2026-11-06

Reviewed the configured-process interaction path against the FOM guide's
catalog-lookup examples, retained declaration state, shared send-time ancestor
resolver, and focused Join test. The guide already separates catalog growth
from an ordinary Receive Interaction callback and labels later matching of an
added descendant against a retained ancestor subscription as source inference.
The configured-process Join test uses an object-only extension and checks
object-class lookup; the embedded class/parameter lookup examples also stop at
catalog lookup, while the inherited-receipt test has its complete hierarchy at
Create. No focused test combines module-bearing Join, interaction subscription,
and send/receive. The existing guide and chart state these limits accurately;
no duplicate diagram or prose change was warranted. No tests, builds, CI,
Requirements Lab, or 2010 work.

## 2025 configured-process Join rollback evidence, increment 44 — 2026-11-06

Expanded the FOM guide's rollback evidence with the focused 2025 process
MOM/rejoin case. It injects failure after the new member's MOM discovery and
initial reflection are queued, verifies those observer events are removed,
then verifies a successful retry. A second injected failure after a late-Join
synchronization announcement checks that retry does not duplicate it. These
assertions demonstrate selected session/observer cleanup for ordinary Join;
they do not include additional FOM modules or observe the catalog after a
post-commit failure. The replacement-definition question therefore remains a
source observation with no focused post-commit additional-module test. No chart
changed. Local links and `git diff --check` pass. No tests, builds, CI,
Requirements Lab, or 2010 work.

## 2025 process Federation MOM after failed module-bearing Join, increment 45 — 2026-11-06

Traced the configured-process request path for an already-known Federation MOM
object. The RTI-owned MOM request is handled directly, and `HLAcurrentFDD` is
encoded from the live `federation.definition.fdd`; the Federation module list
also comes from the current federation definition. These are separate from
the joining member's Join-input list. The inspected post-commit rollback
resigns the member and clears its session/event state without restoring the
replacement definition. Therefore a later explicit value request would
project the retained definition if that rollback path occurs. This is source
inference only: no focused process test combines additional-module Join,
post-commit failure, and a later observer request/reflection. It also does not
prove an automatic Conditional reflection on the failed Join. Local source
links, JSON parse, and `git diff --check` pass. No tests, builds, CI,
Requirements Lab, or 2010 work.

## 2025 Federation MOM module-list scope after Join rollback, increment 46 — 2026-11-06

Clarified the failed additional-module Join comparison in the Federation MOM
guide. The Federation object's `HLAFOMmoduleDesignatorList` and `HLAcurrentFDD`
are both projected from the current execution definition on an explicit value
request. The identically named `HLAfederate` attribute is instead captured on
the joined-member MOM object and follows that object's removal lifecycle; it
does not feed either Federation-level value. The FOM-module-data reports are
separate interactions with federation/member scopes, and the configured-
process Federation report route remains source-unverified. No test combines
post-commit additional-module failure with a later observer request or report;
no automatic Conditional reflection is inferred. JSON and local links validate;
`git diff --check` passes. No tests, builds, CI, Requirements Lab, or 2010 work.

## 2025 process Federation MOM Conditional updates after Join, increment 47 — 2026-11-06

Rechecked the pinned 2025 MIM conditions, embedded additional-module Join
adapter, configured-process Join handler, and focused observer test. The MIM
marks both Federation `HLAFOMmoduleDesignatorList` and `HLAcurrentFDD`
Conditional for the specified Join changes. Embedded code queues both after an
additional-module Join, while the focused test observes only the changed
`HLAcurrentFDD` value. The configured-process handler commits the definition,
sets up the member MOM object, and plans that member's discovery; it has no
Federation-level Conditional update enqueue. The guide already explains these
differences and evidence limits accurately, so no duplicate diagram or prose
was needed. This remains an implementation/evidence comparison, not a
conformance finding or process-parity claim. No tests, builds, CI, Requirements
Lab, or 2010 work.

## 2025 configured-process member-scoped FOM-module request, increment 48 — 2026-11-06

Added the member-scoped boundary next to the Federation request discussion in
the MOM guide. The 2025 MIM describes a target member's Join-supplied module
content. The configured-process `sendInteraction` handler has no member-report
planner call and falls through to the generic interaction publication check;
no focused configured-process test sends this request. This does not establish
the public exception or whether a report interaction is delivered. The two
request roots and their target/member scopes remain distinct. Local links and
`git diff --check` pass. No tests, builds, CI, Requirements Lab, or 2010 work.

## 2025 member FOM-module request after target Resign, increment 49 — 2026-11-06

Followed the embedded new-request planner separately from the already-queued
report callback. It checks requester membership, then reported-target
membership, before searching the RTI-owned joined-member MOM snapshot ledger.
Resign removes the target from current membership, but can retain that snapshot
while known observers still have pending Remove Object Instance callbacks.
Consequently, the retained `HLAfederate` object supports removal cleanup; it
does not authorize a new request naming the departed target. The guide now
states that boundary beside the distinct queued-report lifecycle. The focused
member-report test requests content while its target remains joined, so no
public exception or request-after-Resign behavior is claimed. Local links and
format checks pass. No tests, builds, CI, Requirements Lab, or 2010 work.

## 2025 new member-scoped FOM request after same-name rejoin, increment 50 — 2026-11-06

Traced a fresh member-scoped request separately from an old queued callback.
Embedded `GetFederateHandle(name)` resolves the currently joined member ID;
the request decodes the supplied `HLAfederate` handle and passes that ID to
the planner. Same-name rejoin allocates a new ID, and the new joined-member
MOM snapshot captures the new Join's validated FOM-module content. The planner
checks current membership before reading the indexed content from that
snapshot. The handler maps a nonmember target to `FederateNotExecutionMember`,
but no focused test verifies stale-handle rejection or a new report after
same-name rejoin. Existing old callbacks retain their original target ID and
copied payload. Added a separate sequence diagram and linked the mapping caveat
for list/index alignment. Local links and format checks pass. No tests, builds,
CI, Requirements Lab, or 2010 work.

## 2025 object-instance count reports across target rejoin, increment 51 — 2026-11-06

Reviewed the three target-scoped object-instance count reports in the 2025 MOM
reports guide. Their embedded planners require current requester and target
membership, and queued callbacks recheck the captured target ID and receiver
eligibility. Resign erases the membership record; same-name rejoin has a new
ID and cannot satisfy the old callback's target gate. The payload sources
differ: delete-capability counts inspect live privilege-to-delete ownership,
while updated and reflected counts use ledgers on the joined-member record.
Pending Remove Object Instance callbacks may retain the old `HLAfederate` MOM
object, but that does not preserve these ledgers or retarget an old report. The
three focused tests do not combine queued reports with Resign/rejoin. Added a
separate lifecycle flow and table; the configured-process request path remains
an independent evidence gap. Mermaid rendered and was visually inspected;
local links and format checks pass. No tests, builds, CI, Requirements Lab, or
2010 work.

## 2025 update/reflection activity reports across target rejoin, increment 52 — 2026-11-06

Extended the 2025 MOM reports guide with a source comparison for
`HLAreportUpdatesSent` and `HLAreportReflectionsReceived`. Each request planner
copies class/transport counts from the target's current membership record;
queued callbacks retain that target ID and recheck it with the receiver's
eligibility. Resign erases the old member counters, and same-name rejoin gets a
new ID and fresh membership record. The pinned MIM describes both report
values as activity “since the beginning of the federation execution”; the
current source stores their counters per joined-member record. The guide marks
the rejoin interpretation unresolved and makes no conformance claim. Focused
tests do not exercise counter carry-over/reset across rejoin or a pending
report crossing that boundary. The new flowchart rendered and passed visual
inspection; links and format checks pass. No tests, builds, CI, Requirements
Lab, or 2010 work.

## 2025 interaction activity reports across target rejoin, increment 53 — 2026-11-06

Audited the four embedded 2025 member-scoped interaction activity reports:
broad and directed sent, and broad and directed received. The target is the
sender for sent reports and receiver for received reports. Planners read the
corresponding per-member class/transport ledger and snapshot report values;
queued callbacks retain the original target ID and recheck it with the
receiver. Directed maps are subsets, while broad maps include directed traffic.
Resign erases the old member record and same-name rejoin uses a new ID and
fresh counters. The pinned MIM describes each count as being since the
beginning of the federation execution; no rejoin test or normative
interpretation resolves whether an identity with the same name continues the
older counts. Added a separate target-lifecycle flow; no configured-process
parity is inferred. Mermaid rendered and was visually inspected; JSON, links,
and format checks pass. No tests, builds, CI, Requirements Lab, or 2010 work.

## 2025 configured-process interaction activity report queries, increment 54 — 2026-11-06

Completed the bounded configured-process audit for the broad and directed
interaction-sent and interaction-received report requests. The process Send
Interaction handler has no family-specific report planner call for these four
requests; the route reaches generic `planReceiveOrderInteraction`. That planner
checks sender publication and, if admitted, makes ordinary receive-order
recipients. A non-applied plan returns the generic rejected transport response
at this layer; no public API exception is inferred. The focused report-query
tests identified are embedded tests; no configured-process counterpart was
found in the bounded class-name search. Expanded the process boundary section
to name these four classes, list the evidence gap, and link the embedded tests.
The chart rendered and passed visual inspection; local links and format checks
pass. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Audit the 2025 federation-synchronization guide's
synchronization-point participant snapshot across late Join and Resign. Trace
which federate IDs are captured at registration, what completion/announcement
uses that captured set, and whether a same-name rejoin can satisfy an old
participant entry. Compare only the focused tests for these transitions and
keep implementation observations separate from normative guarantees.

## 2025 synchronization-point participant identity across Resign/rejoin, increment 55 — 2026-11-06

Completed the participant-identity review in the 2025 federation-synchronization
guide. Registration and pending late-Join expansion use federate IDs; Resign
removes the departing ID from the synchronization, announcement, and achieved
sets and reevaluates completion. A same-name rejoin receives a new ID. An
omitted-set point still pending may expand to include that ID, requiring a new
achievement; an explicit-set point does not expand, so an achievement attempt
by the rejoined ID is rejected as not announced. The guide marks these as
implementation observations and makes no normative same-name rule claim.
Existing focused tests cover explicit versus expandable late Join and a
separate resignation completion case, but none combines pending-point Resign,
same-name rejoin, and achievement. Added a lifecycle flowchart; Mermaid
rendered successfully. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Audit the 2025 synchronization-point MOM roster after
participant Resign and late Join. Compare the pending barrier's participant,
announcement, and achievement sets with the reported federate/status roster;
identify which focused tests observe each transition, and keep registry state
separate from MOM query/report behavior.

## 2025 synchronization-point MOM roster after membership changes, increment 57 — 2026-11-06

Extended the 2025 federation-synchronization guide with a query-time roster
view. The embedded status planner iterates the point's current participant
IDs and classifies each from announcement and achievement state. A late-Join
expansion can add a newly announced ID; Resign removes the departing ID and
may complete and erase the point. A later query therefore omits departed IDs
and returns an empty roster after point removal. These transitions are
implementation-derived. The focused MOM test covers achievement status
changes and missing/completed-point observations, while the late-Join and
Resign tests do not query the MOM roster. Configured-process query behavior
remains unverified. Added and rendered a dedicated flowchart; local links and
format checks pass. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Review 2025 synchronization-point announcements when a
participant joins while multiple pending points exist. Trace per-point
expansion eligibility, announcement batching/order, and rollback on Join
failure; inventory focused tests for two or more pending points and preserve
the separate embedded/configured-process evidence boundary.

## 2025 synchronization-point Join expansion across pending labels, increment 58 — 2026-11-06

Documented the shared registry's per-Join walk across all pending points.
Explicit-set points are skipped; each eligible omitted-set point adds the new
ID to its participant and announcement sets and contributes its own label/tag
to the adapter batch. The configured-process adapter enqueues the batch and
rolls back the Join on failure; Resign cleanup removes the new ID from pending
barriers, while session cleanup clears its queued synchronization callbacks.
The direct registry test has two points but only one expansion-enabled point;
the process failure case queues one synchronization announcement. No test
covers a Join expanding several pending points or failure mid-multi-point
batch. Added a rendered flowchart and linked source/test limits. Local links
and format checks pass. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Audit the 2025 failure-to-synchronize set across participant
Resign and barrier completion. Trace when false results enter or leave the
aggregate, whether a Resign-triggered completion reports that member, and
which tests assert the final set; keep the result callback distinct from
membership cleanup.

## 2025 failed-to-synchronize aggregation after participant Resign, increment 59 — 2026-11-06

Documented the embedded 2025 ordering where Resign removes the departing ID
from participant, announcement, and achievement state before evaluating
barrier completion. If completion follows, the failed set is collected from
the remaining achievement entries and notifications target only remaining
participants; a departed member's earlier false result is therefore removed
from the aggregate. The focused case verifies successful achievement by the
remaining member, unachieved departure, an empty failed set, and service-report
write before callback observation. It does not test a departing false result
or the other pending branches. Added and rendered a bounded source-derived
flowchart; no standard rule or configured-process parity is inferred. Local
links and format checks pass. No tests, builds, CI, Requirements Lab, or 2010
work.

**Next handoff:** Review 2025 synchronization-point handling when several
participants Resign in one membership transition or forced-loss cleanup.
Trace whether completion can be planned more than once, which remaining IDs
receive notifications, and how adapters dispatch those plans; locate focused
evidence and keep voluntary Resign separate from forced connection loss.

## 2025 synchronization cleanup across repeated membership loss, increment 60 — 2026-11-06

Clarified that each 2025 voluntary Resign or forced connection-loss transition
removes one departing ID while scanning all pending synchronization points.
One departure may complete several points, each erased when its notification
plan is formed; a later loss is a separate transition against updated state.
Forced loss shares the registry cleanup but uses distinct embedded and
configured-process notification adapters. The focused voluntary Resign test
covers one point; no test covers multi-point completion, sequential losses
through one active barrier, or forced loss during synchronization. Added a
source-derived flowchart and evidence table links. Mermaid rendering and local
checks pass. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Begin the 2025 ownership-management guide review at negotiated
attribute acquisition. Trace the requester's pending acquisition state through
Divestiture If Wanted, acquisition notification, and Attribute Ownership
Acquisition Notification; identify which registry transitions and focused
tests establish the handoff, while keeping ownership callbacks separate from
the API's immediate service result.

## 2025 Divestiture If Wanted terminates pending negotiated ownership, increment 61 — 2026-11-07

Extended the 2025 attribute-ownership guide with a cross-service transition.
When a regular acquirer is pending and the owner has initiated negotiated
divestiture, Divestiture If Wanted can commit that acquirer as owner, consume
the pending request, and erase the negotiated confirmation entry. Its output
set and acquisition-notification callback are separate outcome channels; the
notification carries the If-Wanted tag. A later Confirm Divestiture no longer
passes the current ownership precondition. The guide keeps this implementation
observation distinct from the standard's service concepts and makes no
ordering claim between service return and callback delivery. The individual
If-Wanted and negotiated-confirmation tests exist, but no focused case combines
them. Added a rendered flowchart and linked the source/test gap. Local links and
format checks pass. No tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Review requester acquisition state when Divestiture If Wanted
transfers only part of a multi-attribute request. Trace which request members
remain pending, how stale release callbacks are suppressed, and whether the
next ownership notification remains grouped by request or recipient; compare
focused tests and keep If-Available behavior explicitly separate.

## 2025 partial Divestiture If Wanted within a regular acquisition request, increment 62 — 2026-11-07

Added a source-derived partial-request path to the 2025 ownership guide. When
one attribute in a regular `{A, B}` acquisition request is transferred through
Divestiture If Wanted, the registry consumes `A` from the request and its
notification ledger while retaining the same pending request for `B`. It
clears the former owner's queued release reservation for `A`; an unentered
queued release callback rechecks its scheduled set and can retain only still
eligible `B`. The existing test transfers the only requested attribute and
later submits a new request for an unrequested attribute, so it does not
establish the same-request partial case. A second test covers multiple
requesters for one attribute, not one request split across attributes. Added a
rendered flowchart and linked this evidence gap. Local checks pass. No tests,
builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Compare partial consumption of a regular acquisition request
with partial consumption of an Acquisition If Available request after
Divestiture If Wanted. Trace each request ledger's cleanup and callback result,
then identify whether current tests establish the remainder or leave it
source-only; do not infer equivalent behavior from shared attribute-owner
mutation.

## 2025 partial If-Available resolution after Divestiture If Wanted, increment 63 — 2026-11-07

Compared the If-Available request ledger with the regular acquisition ledger
for an If-Wanted transfer. The transfer removes only the selected attribute
from the If-Available desired set and retains a one-shot result callback. When
that callback reaches its gate, it resolves the remaining requested set and
consumes the request; if the remaining attribute is still held by another
federate, the source plans it as unavailable under the original If-Available
tag. The separately queued acquisition notification for the transferred
attribute uses the If-Wanted tag. The existing focused test covers only a
one-attribute If-Available request whose result callback becomes empty before
the direct transfer notification; no multi-attribute mixed-result case is
tested. Added a separate flowchart and bounded source/test links. Mermaid
rendering and local checks pass. No tests, builds, CI, Requirements Lab, or
2010 work.

**Next handoff:** Audit the Divestiture If Wanted interaction with a pending
acquisition cancellation. Trace when cancellation reservations are consumed,
which callback becomes the terminal response for the transferred attribute,
and whether the remaining attributes of a multi-attribute cancellation stay
pending; compare the focused cancellation/transfer tests and do not infer
callback order from source queue order alone.

## 2025 Divestiture If Wanted versus accepted acquisition cancellation, increment 64 — 2026-11-07

Documented the 2025 embedded race where a requester has accepted cancellation
while the owner is already inside the release callback. If Divestiture If
Wanted transfers an attribute from that request, the transfer consumes that
attribute from the live cancellation reservation, making its queued
cancellation-confirmation work stale; the acquisition notification with the
If-Wanted tag becomes the terminal reply for that attribute. For a
multi-attribute cancellation, the source leaves nontransferred members in the
reservation for the later cancellation gate to resolve. The focused
race test proves only a one-attribute transfer and observes only the
acquisition notification. Added a flowchart and linked the untested partial
cancellation branch. Mermaid rendering and local checks pass. No tests,
builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Review the 2025 unconditional-divestiture and ownership
assumption-search boundary for one attribute with both a pending acquisition
and an eligible assumption candidate. Trace whether relinquishment queues a
direct acquisition result, an assumption offer, or both, and how a candidate's
later acquisition terminates the search; distinguish an offer from transfer
and compare focused cases without inferring callback order.

## 2025 Unconditional divestiture requester/candidate split, increment 65 — 2026-11-07

Refined the ownership overview flowchart to make the per-attribute requester
and assumption-candidate branches explicit. An eligible federate with a
pending regular or If-Available acquisition, or accepted cancellation, stays
on that request's own resolution path and is excluded from the separate
assumption-offer set. A candidate offer remains advisory: ownership changes
only through a later acquisition path. The embedded unconditional-divestiture
test covers pending regular and If-Available requesters and verifies their
separate acquisition notifications; it verifies no assumption offer for the
If-Available requester, but not the regular requester. The corresponding
configured-process overlap is source-derived and remains untested. The guide
already contains this detail in its process section; the revised overview now
matches it without implying callback order. Mermaid rendering, local link
checks, and whitespace checks pass. No tests, builds, CI, Requirements Lab, or
2010 work.

**Next handoff:** Review the 2025 Exception Reporting and Service Invocation
Reporting guides together at their seam: trace when an RTI service exception
is delivered to the caller, when a service-invocation report is generated,
and whether either can be mistaken for an HLA callback. Compare only the
focused 2025 source/tests already linked by those guides, preserve embedded
versus process and callback-model boundaries, and add a diagram only if the
distinction is hard to follow.

## 2025 Data Distribution and Regions reader-gap review, increment 66 — 2026-11-07

Reviewed the 2025 DDM guide's region-association and attribute-update path
against the embedded recipient planner, region-overlap helpers, and the
focused `regional_object_attribute_routing` case. No diagram change was needed:
the guide already distinguishes per-object/per-attribute source association
from receiver subscription regions; dimension-context admission from later
overlap; discovery and scope effects from value reflection; and current
regional projection from timestamp eligibility and callback dispatch. The
focused case demonstrates that a disjoint receiver is neither discovered nor
reflected, then exercises a matching update path. These are embedded test
observations; no process or 2010 parity follows. Mermaid inventory did not
change. No tests, builds, CI, Requirements Lab, or 2010 work.

## 2025 Exception and Service Invocation Reporting seam review, increment 67 — 2026-11-07

Compared the caller-result, `HLAreportException`, and
`HLAreportServiceInvocation` paths across both 2025 guides. No diagram change
was warranted: the guides already distinguish the public service's return or
synchronous exception from observer callbacks; separately gate exception
reports by Exception Reporting and eligible observer routing; separately gate
invocation reports by Service Reporting and its selected file/interaction
destination; and explain that the `noexcept` exception hook can suppress its
own reporting failures while later wrapper-specific invocation-report work
may still affect the caller-visible outcome. The guides also scope embedded
and process behavior and callback models separately. Focused cited tests cover
selected cases, not every combination or a universal ordering. No tests,
builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Review the 2025 lifecycle guide's post-Join path: distinguish
membership commit, joined-federate MOM setup, discovery planning, and callback
delivery for the new `HLAfederate` object. Compare embedded and configured-
process source/test evidence without implying callback order or full profile
parity; add a diagram only if these stages are difficult to follow in the
current guide.

## 2025 Connect authorization versus HLA callbacks and reports, increment 68 — 2026-11-07

Refined the Connect flow to name the configured `Authorizer` SPI call and
added an explicit boundary note: `authorizeRtiOperation` is not a
`FederateAmbassador` callback. The inspected Connect route returns its
`ConfigurationResult` or throws to its caller before any federation
membership exists; no HLA exception-report hook or service-invocation report
writer is called on this path. This is source-derived. Focused 2025
authorization tests verify rejected Connect outcomes and subsequent successful
connection, but do not instrument the federate callback or report sinks to
assert absence. Mermaid CLI rendered and screenshot review found the revised
Connect chart readable; local source/test anchors and whitespace checks pass.
No tests, builds, CI, Requirements Lab, or 2010 work.

## 2025 Create/Join authorization gate and lifecycle commit, increment 69 — 2026-11-07

Clarified that the lifecycle guide's Create/Join sequence begins after the
public RTIambassador's local authorization gate. An accepted authorization
result opens that gate but does not establish a federation or member; FOM,
execution, and membership validation still precede the corresponding backend
transition. The focused embedded Create case uses deliberately missing FOM
paths and observes `Unauthorized`, while the Join case denies a requested name
and later succeeds with that same name after an accepted result. The configured
process handlers do not reauthorize; the process behavior remains source-only
for this gate. Updated cross-guide and test links. No diagram change was needed
because the authorization guide already diagrams the scoped gate and result
mapping; no tests, builds, CI, Requirements Lab, or 2010 work.

## 2025 Join commit and joined-federate discovery timing, increment 70 — 2026-11-07

Added a profile-comparison sequence to the lifecycle guide. The embedded path
commits report/MOM setup and local Joined state before planning observer
discovery; its source permits an `HLA_IMMEDIATE` observer callback to run inline
before the joining call returns. The configured-process service installs
session state and enqueues discovery before replying, while the public client
applies its local Joined transition after receiving the response. Event
delivery's timing relative to that response remains unasserted. Linked the
embedded callback-time known-instance gate, Join discovery submission, process
Join enqueue, local client transition, and the two focused public test cases.
The tests observe discovery for a joined-federate lifetime but do not assert
callback timing relative to Join return or complete profile parity. Mermaid
CLI 12.0.0 rendered both charts and screenshot review found each readable.
Local source/test destinations and whitespace checks pass. No builds, tests,
CI, Requirements Lab, or 2010 work.

**Next handoff:** Review the embedded 2025 `HLA_IMMEDIATE` path when an
observer's Join-triggered `discoverObjectInstance` callback throws. Trace the
caller-visible result, joining membership and local Joined state, observer
known-instance commit, and cleanup of later planned recipients. Keep the
configured-process path separate unless direct evidence establishes the same
failure behavior; add a focused chart only if the current callback and Join
flows leave the partial-commit result unclear.

## 2025 Join discovery callback exception and partial commit, increment 71 — 2026-11-07

Added a source-derived flowchart for an embedded `HLA_IMMEDIATE`
`discoverObjectInstance` exception during Join. The membership and local Joined
transition already committed; the callback-time gate has already committed
the observer's known-instance state. The discovery queue clears pending
reservations from the throwing recipient through the rest of the planned list,
but does not undo known state. Because the exception interrupts the route,
that observer's initial `HLAfederate` reflection and Join's following
conditional MOM-update submissions are not reached. An IEEE 1516.1 `Exception`
triggers Join's best-effort `HLAreportException` hook before the original
exception is rethrown; other exception types bypass that typed catch. The
cited public MOM tests cover successful discovery only and do not assert these
partial effects. Mermaid CLI 12.0.0 rendered the chart and screenshot review
found it readable. Focused source/test links and whitespace checks pass. No
builds, tests, CI, Requirements Lab, or 2010 work.

**Next handoff:** Audit failure paths after Join has provisionally committed
registry membership but before it returns success. Compare embedded and
configured-process report-file/MOM setup and rollback, including whether the
execution-scoped `HLAfederation` record can remain after a failed first Join.
Separate injected-test evidence from source-only cleanup behavior; do not
claim complete profile parity.

## 2025 Join setup rollback and residual federation MOM state, increment 72 — 2026-11-07

Updated the lifecycle guide's existing setup-failure path with the available
2025 evidence. The embedded writer-creation failure case observes
`RTIinternalError` and proves a peer can Join using the rejected requested
name. A private configured-process service test injects failures after report
file allocation and after session installation, verifies that the member and
report file are removed, then successfully retries the same name. Source shows
that both Join paths establish execution-scoped `HLAfederation` before
report-file setup and that Resign retains federation-execution MOM objects, so
that record can remain with zero members until Destroy. Neither failure test
asserts the residual MOM object or public callback behavior; the process test
also does not establish complete public endpoint parity. The existing
profile-overlay lifecycle chart already represents rollback and residual
execution-scoped state, so no additional diagram was needed. Source/test links
and whitespace checks pass. No builds, tests, CI, Requirements Lab, or 2010
work.

**Next handoff:** Review the execution-scoped `HLAfederation` object across a
failed first Join and a subsequent successful retry. Trace whether its object
instance handle and observer-known state persist, how an eligible existing
subscriber can discover it, and whether Destroy is the only retirement path.
Separate registry-source conclusions from public test evidence; do not infer
configured-process or 2010 parity beyond inspected tests.

## 2025 Federation MOM subscription and conditional-reflection reviews, increments 73–75 — 2026-11-07

Reviewed the execution-scoped `HLAfederation` identity across failed-first-Join
rollback and same-execution retry. The registry source reuses the existing
object record and handle; the public retry case proves only that the member
name is reusable, not MOM identity or discovery. The configured-process setup
failure test likewise checks member/file cleanup and retry, not federation
MOM callbacks. Existing lifecycle and discovery diagrams already cover the
retained-empty state and subscription-based discovery.

Added the embedded 2025 subscription eligibility and known-object distinction
to the Federation MOM guide. Active ordinary subscriptions or active
overlapping regional subscriptions can qualify a member for discovery;
unsubscribe changes declarations but does not clear the federation MOM
known-recipient record or schedule a MOM removal. The focused public cases do
not assert passive-only, disjoint-region, unsubscribe, or rediscovery behavior.

Expanded the conditional-value diagram to separate event-triggered reflections
from direct current-value requests. Event work plans values for known active
subscribers and rechecks eligibility at callback entry; direct requests use
the current value for a known instance without requiring an active
subscription. Membership, FOM, and save tests observe representative
reflections. The Auto Provide switch test does not observe the `HLAautoProvide`
reflection itself. Mermaid CLI 12.0.0 rendered the new diagram, and local
screenshot review found it legible. No tests, builds, CI, Requirements Lab, or
2010 work.

## 2025 documentation coverage inventory and progress measures, increment 76 — 2026-11-07

The worktree contains 32 current 2025 guides: the timing guide plus 31 other
major behavior topics. All 32 completed the roadmap's five-needs content
audit, so that defined audit is **32/32 (100%)**. This is not an overall
documentation-completion percentage. The worktree contains 477 Mermaid
diagrams (277 flowcharts, 175 sequences, and 25 state diagrams). The previous
whole-inventory render record covered 463; the current 14-diagram delta still
needs per-body reconciliation. The last recorded curated screenshot review is
236/463 (51.0% of that earlier checkpoint), not a current whole-inventory
visual audit. GitHub's actual page renderer remains unverified. These progress
measures stay separate because they have different completion criteria.

**Next handoff:** Reconcile each of the 14 current diagrams beyond the recorded
463-body local render inventory. Use the individual render and screenshot
entries to determine what is already covered, locally render any missing body
with Mermaid CLI 12.0.0, and review changed or previously unreviewed layouts.
Keep local rendering distinct from GitHub verification; update the exact
render and selected-review denominators only after the inventory matches.

## 2025 whole-inventory Mermaid render reconciliation, increment 77 — 2026-11-07

Rendered the current 32-guide corpus with Mermaid CLI 12.0.0. The Markdown
batch produced 469 artifacts from backtick-fenced diagrams. The CLI's Markdown
extractor skips tilde fences, so extracted the eight tilde-fenced bodies into
temporary backtick-wrapped inputs and rendered those separately. All 477
current Mermaid bodies therefore have local render evidence; no repository
diagram wrapper was changed. The conditional-reflection chart added in
increment 75 was also inspected from its rendered screenshot and remains
legible. This does not establish GitHub page rendering or whole-corpus visual
review. The previous curated screenshot count remains a 236/463 checkpoint;
its current per-diagram denominator needs separate reconciliation. The 32/32
content audit remains 100% of that review pass only. No tests, builds, CI,
Requirements Lab, or 2010 work.

**Next handoff:** Reconcile the current selected screenshot-review ledger
against the 477 diagram bodies, identify charts revised or added since the
236/463 checkpoint, and screenshot-review those bodies that lack current
visual evidence. Preserve the distinction between selected local review,
whole-inventory local rendering, and actual GitHub page verification. Then
resume the 2025 cross-guide backlog at its next substantive reader gap.

## 2025 tilde-fenced chart visual review, increment 78 — 2026-11-07

Screenshot-reviewed all eight tilde-fenced diagrams after rendering them from
temporary backtick-wrapped copies. The Attribute Scope and Auto Provide charts
render without clipping. The tall scope-process and Auto Provide flows remain
traceable at full image size, but their text is small in a full-page overview;
revisit their layouts during selected visual-review reconciliation rather
than treating this as whole-tree visual QA. Repository tilde wrappers remain
untouched. The exact curated screenshot aggregate is still unreconciled
because older per-chart reviews may overlap these eight. Local render coverage
remains 477/477; actual GitHub page rendering remains unverified. No tests,
builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Reconcile selected screenshot-review records against the 477
current diagrams, mark the eight tilde-fenced charts as individually reviewed,
and identify every modified or added chart still lacking visual evidence.
Review those layouts at GitHub-like width, prioritizing the tall scope-process
and Auto Provide flows. Keep the aggregate selected-review count separate
until the per-chart ledger is free of overlap.


## 2025 Auto Provide diagram readability revision, increment 79 — 2026-11-07

Reworked the discovery-triggered Auto Provide overview at its meaningful
callback boundary. The tall decision flow is now a sequence diagram showing
discovery callback return, switch and owner/scope eligibility, callback-entry
revalidation, and the provider solicitation. A second compact flow shows that
provider response is optional and that ordinary ownership, DDM, order, and
time checks happen only on a later Update Attribute Values call. This keeps
solicitation and response distinct while making the overview shorter and more
readable. Mermaid CLI 12.0.0 rendered both current diagrams; screenshot review
found their labels and branches legible and unclipped (784x697 and 699x272).
The source inventory is now 478 diagrams: 277 flowcharts, 176 sequences, and
25 state diagrams; 469 backtick and 9 tilde fences. Existing render evidence
remains applicable to unchanged bodies, and both replacement/addition bodies
were rendered. The exact selected-review aggregate remains unreconciled, and
GitHub rendering remains unverified. No tests, builds, CI, Requirements Lab,
or 2010 work.

**Next handoff:** Reconcile per-diagram screenshot records against the current
478-body inventory, account for charts replaced since the 236/463 checkpoint,
and review the remaining changed or unreviewed charts. Preserve the separate
counts for content audit, local rendering, local screenshot review, and actual
GitHub rendering. Then resume the 2025 cross-guide reader-gap backlog.


## 2025 process Attribute Scope delivery diagram readability review, increment 80 — 2026-11-07

Replaced the tall configured-process scope-delivery flowchart with a sequence
diagram across the 2025 registry, process event service, client callback
bridge, and FederateAmbassador. It keeps the two process transports distinct:
pushed events proceed after admission, while server-queued events receive a
second scope check at the later `receive_interaction` poll. The diagram also
shows suppression when the current eligible subset is empty and the callback
route only when an event reaches the client pipeline. Mermaid CLI 12.0.0
rendered the current chart; screenshot review found it legible and unclipped at
784x695, compared with 588x1955 for the previous flowchart. The replacement
retains one diagram and one thematic boundary. The 2025 inventory remains 478
diagrams: 276 flowcharts, 177 sequences, and 25 state diagrams. Local render
coverage remains 478/478 because this replacement was rendered and unchanged
bodies retain their prior render evidence. The current tilde-fenced subset is individually screenshot-reviewed: 4/4
Attribute Scope charts and 5/5 Auto Provide charts (9/9); whether these overlap
the historical checkpoint remains unknown. GitHub rendering remains unverified. No
tests, builds, CI, Requirements Lab, or 2010 work.

**Next handoff:** Continue reconciling chart-level screenshot records against
the 478 current bodies. Treat both replaced tall flowcharts (Auto Provide and
process Attribute Scope) as superseded by their reviewed current sequence
versions. Update a unique current-review total only after reconciling historical
review notes against current chart identities; then continue the 2025
cross-guide reader-gap backlog.
