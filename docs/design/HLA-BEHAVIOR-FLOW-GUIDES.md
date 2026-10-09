# HLA Behavior Flow Guides: Umbra Backlog

This is the living index for contributor-facing HLA flow diagrams in Umbra.
The guides explain difficult behavior before asking a reader to navigate the
implementation; they are not replacements for IEEE standards, implementation
contracts, or test evidence.

## Guide pattern and boundaries

Use the [2025 time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md) as the
first structural example. Each new guide should:

- State its API edition and implementation profile at the top. A 2010 guide
  must be separate from a 2025 guide; never combine state machines or imply
  compatibility unless that equivalence has been demonstrated.
- Use GitHub-renderable Mermaid diagrams, with service calls, callbacks, and
  state-commit points labeled. Prefer several cohesive diagrams over one
  all-encompassing chart.
- Separate standard concepts, current Umbra behavior, test observations, and
  known limits. The standard is normative; code and tests describe only the
  implementation and scenarios exercised.
- Link implementation claims to the relevant source and focused tests. Treat
  a passing test as evidence for its scenario, not proof of complete support or
  conformance.
- Identify interactions with other areas (time, ownership, regions, save and
  restore) without silently absorbing those state machines into the guide.
- Keep this work in Umbra documentation. Any later translation into or
  expansion of the HLA Requirements Lab is a separate phase and is not part of
  this backlog's current authorization.

## Prioritized guide backlog

| Order | Area | Purpose and likely diagrams | Status |
| --- | --- | --- | --- |
| Baseline | Time management | Role enable/disable, advance requests, grant bounds, TSO queue delivery, callback ordering. | Drafted; [time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md) also needs GitHub Mermaid rendering verification |
| 1 | Attribute ownership | Per-attribute regular and If-Available acquisition, denial, negotiated/unconditional divestiture, assumption offers, callback-time state changes, and cancellation/staleness edges. | Drafted; [guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md) needs GitHub Mermaid rendering verification |
| 2 | DDM and regions | Region create/commit/delete, association and subscription lifetimes, overlap/relevance evaluation, and update/interaction routing. | Drafted; [guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md) needs GitHub Mermaid rendering verification |
| 3 | Save and restore | Federate save coordination, callback gates, save completion/failure, restore selection/rejoin, and restoration of deferred runtime work. | Drafted; [guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 4 | Object and interaction information flow | Publication/subscription, registration/discovery, update/request, interaction delivery, and removal/resignation effects. | Drafted; [guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 5 | Federation and federate lifecycle | Create/join/resign/destroy, connection loss, final-member behavior, and cleanup of member-owned state. | Drafted; [guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 6 | Callback and service ordering | Queueing, callback models, service re-entry boundaries, ordering guarantees, and interactions with time-managed delivery not already covered elsewhere. | Drafted; [guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 7 | Federation synchronization points | Registration and participant selection, omitted versus explicit sets, late joins, achievement fan-in, resignation completion, callbacks, and in-flight save/restore state. | Drafted; [guide](HLA-2025-FEDERATION-SYNCHRONIZATION-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 8 | Service invocation reporting | Per-member reporting/subscription interlock, interaction-vs-file routing, DDM recipient selection, serial reservation, callback ordering, and report-file lifetime. | Drafted; [guide](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 9 | FOM module admission and composition | Per-module DIF validation, ordered Annex C merge, whole-model reference resolution, FDD materialization, and atomic Create/Join commit boundaries. | Drafted; [guide](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 10 | Joined-federate MOM object lifecycle | Private RTI snapshot versus public discovery/reflection, Join-scoped values, save/restore `HLAfederateState`, and pending resign-removal lifetime. | Drafted; [guide](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 11 | Exception reporting | Separate `HLAreportException`, `HLAreportMOMexception`, and `HLAreportServiceInvocation`; switch gates, subscriber/DDM selection, process rechecks, payload projection, and caller-visible failure. | Drafted; [guide](HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 12 | TSO message retraction | Producer legality boundary, per-recipient pending/delivered/suppressed states, Request Retraction callbacks, mixed fan-out, and timestamped-delete restoration. | Drafted; [companion guide](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md) deepens the queue overview in the time guide and needs Mermaid rendering verification |
| 13 | Object-instance name reservation | Single and bulk reservation callbacks, partial reserve versus atomic release, owner-only named registration, reservation consumption, resignation, and restored reservation state. | Drafted; [guide](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 14 | Order and transportation controls | Keep RO/TSO order separate from transport type; trace class defaults, per-instance overrides, callback-gated transportation changes, queries, and what receivers observe. | Drafted; [guide](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 15 | Declaration and attribute relevance advisories | Distinguish class-level publisher notifications from per-instance owner notifications; explain active/passive subscriptions, switches, class hierarchy, regional boundaries, known-object state, and update-rate reissues. | Drafted; [guide](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 16 | Attribute Scope Advisory | Model receiver-local per-object-attribute scope, region and subscription transitions, the self-final-unsubscribe exception, receiver switch, grouping, and stale callback rechecks; keep it separate from owner relevance and data delivery. | Drafted; [guide](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 17 | Auto Provide | Trace the federation-wide FDD/MOM switch, discovery-triggered provider grouping, mandatory empty tag, regional eligibility, provider response separation, and callback-time switch/scope fences; call out the unverified process automatic path. | Drafted; [guide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md) needs Mermaid rendering verification |
| 18 | Directed interactions | Separate sender, target object, and receiver identities; explain object-class/interaction-class publication, universal versus by-ownership selection, receiver fan-out, callback-time rechecks, and the TSO/retraction boundary. | Drafted; [2025 guide](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md) needs Mermaid rendering verification |
| Separate 2010 profile | Bounded reference RTI service slice | Federation membership, discovery/update/interaction, synchronization barrier, and explicit unsupported-flow map for the 2010 shell only. | Drafted; [separate 2010 guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md) needs Mermaid rendering verification |

This order is a working prioritization, not a statement that one area is more
normative or more complete than another. At the outset of this documentation
effort, a bounded 2025 survey found dedicated runtime and focused test families
for ownership, DDM/regions, save/restore, information flow, and federation
membership, but few cohesive contributor flow guides beyond timing. A focused
synchronization-point survey then identified
participant-set expansion, achievement fan-in, resignation, callback, and
state-image boundaries suitable for their own 2025 guide. A bounded
service-report survey found an existing implementation dossier and focused
tests for the report-subscription interlock, file sink, interaction sink, and
callback order, but no learner-facing flow guide; this guide narrows itself to
invocation reports rather than the whole MOM. The FOM-admission survey found
an extensive validation design and focused tests, but no compact flow guide
for the distinction between per-module DIF checks, complete-set composition,
FDD validation, and transactional Create/additional-Join behavior. The joined-
federate MOM survey found public-lifecycle and save/restore evidence for the
RTI-owned `HLAfederate` instance, but no learner-facing account of its private
snapshot, subscriber-visible callbacks, conditional state values, and
callback-time removal lifetime. An exception-report survey then found separate
focused routes and tests for `HLAreportException` and `HLAreportMOMexception`,
plus an existing design note documenting an inconsistency in the vendored 2025
MIM wording; the new guide makes the reporting-switch and subscription rules,
process callback rechecks, payload projection, and known source ambiguity
explicit without editing the Requirements Lab. The separate 2010 survey found a narrower but
real in-process reference service slice: the binding-shell smoke exercises
membership, object information, and interaction delivery, while synchronization
has source but no focused test in the surveyed family. The 2010 guide documents
only that slice and its known absences; it does not infer parity from the 2025
guides. A retraction survey found that the timing overview's queue diagram did
not fully explain the producer's strict time/lookahead boundary, recipient
ledger, or the distinction between withdrawing pending fan-out and requesting
reversal after delivery. Focused interaction, attribute, regional, process,
delete, and save/restore tests support a separate 2025 companion deep dive;
it links back to the overview instead of restating all time-management
behavior. Test-file-name searches are topic-discovery signals, not coverage
measurements. The object-instance-name survey then found a distinct state
machine not explained by the generic registration flow: single-name collisions
report asynchronously, multi-name reserve permits a successful subset while
multi-name release validates atomically, and named registration consumes only
the owner's reservation at commit. It also found focused embedded, process,
restore, and restart tests. A test that feeds a 2010 FOM to the 2025 API is
explicitly treated as 2025 behavior with legacy-model input, not evidence about
the separate 2010 reference RTI. The order/transportation survey then found a
separate control plane with synchronous order changes and callback-gated
transportation changes, class defaults captured by later registrations,
per-instance overrides, callback-visible order and transport metadata, and
selected pending-change restore coverage. Existing time and DDM guides explain
the adjacent grant and recipient-selection boundaries, so this guide focuses
on the control decisions rather than duplicating those state machines.

The next advisory survey found a distinct class-level declaration signal and
per-object-attribute relevance signal. A 2025 object subscription can feed both
planners, but callbacks differ in recipient, granularity, region criteria, and
switch behavior. The new guide separates them and links regional details back
to the DDM guide instead of repeating its overlap state machine.

The Attribute Scope Advisory survey found distinct embedded and public
process-endpoint test cases, both covering HLA_EVOKED and HLA_IMMEDIATE. The
new 2025-only guide gives scope its own receiver-centered state machine,
documents the implementation's self-final-unsubscribe exception and
delivery-time stale-work filtering, and links back to the broader DDM and
relevance guides instead of merging their state machines. All 2025 and 2010
diagrams still require a visual Mermaid-rendering pass; static syntax and link
checks are not visual verification. The next topic should be selected after
that review from an uncovered runtime/test family, preserving the separate
2010 profile boundary.

The Auto Provide survey found a distinct discovery-triggered path beside
explicit Request Attribute Value Update: the 2025 embedded planner groups
currently owned in-scope attributes by provider, supplies an empty tag, and
rechecks switch and scope before provider code. Focused tests cover disabled
discovery-only behavior, regional/reentrant changes, multi-provider fan-out,
MOM switch mutation, and timestamped admission. The process profile exposes
explicit requests and switch mutation, but no automatic discovery planner or
focused process Auto Provide test was found in this bounded survey; the guide
marks that path unverified rather than claiming edition/profile parity.
Visual Mermaid review remains the next handoff for the guide set as a whole.

The directed-interaction survey found a distinct object-context routing model:
the sender supplies a target instance, while the RTI fans out according to
object-class/interaction-class declarations and each receiver's universal or
by-ownership selector. Focused evidence includes embedded selector, known-
target, timestamped delayed-subscription and retraction tests, plus a process
multi-recipient endpoint case. The new guide keeps embedded callback-time
revalidation separate from the process evidence and links timestamp/grant and
retraction state back to their dedicated guides.

## Completion check for each guide

The current workstation has no local Mermaid CLI or Mermaid JavaScript
renderer, so static syntax/link checks do not count as visual rendering
verification. Keep that gate open until a GitHub Markdown/Mermaid preview or
an approved local renderer has been used.

Before calling a topic guide-ready, verify that its diagrams render in GitHub's
Mermaid Markdown, all relative source/test links resolve, behavior statements
are traceable to the named implementation and focused tests, normative
statements are checked against the correct official edition, and the explicit
edition boundary remains intact. Record what is not modeled and leave the next
topic as a concrete handoff here and in
[ROADMAP-INDEX.json](../planning/ROADMAP-INDEX.json).
