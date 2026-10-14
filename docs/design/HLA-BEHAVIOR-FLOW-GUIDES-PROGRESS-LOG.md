# HLA Behavior Flow Guides: Historical Progress Log

This companion retains the dated reader-gap audits, evidence reviews, and prior handoffs that accumulated after the living guide index. The records are preserved for traceability; their historical “next handoff” notes are not the active queue.

For the current goal, prioritized topics, and active handoff, use [the living guide index](HLA-BEHAVIOR-FLOW-GUIDES.md) and [roadmap handoff](../planning/ROADMAP-INDEX.json). Mermaid rendering and visual-inspection records remain in the [review ledger](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md).

---
### 2025 interaction event versus object-attribute identity — 2026-10-12

Audited the 2025 object/interaction guide's HLA distinction against the
pinned RTI/FederateAmbassador signatures, selected embedded/process cases, and
the existing detailed recipient flows. The guide already separates
per-instance attribute authority and receiver object knowledge from ordinary
interaction-class routing; the missing wayfinding detail was the *outer* API
identity alongside the shared typed value maps. Updated the existing
"Encoded values enter services" diagram rather than adding a duplicate:
object updates/reflections name an `ObjectInstanceHandle`, while ordinary
interaction send/receipt name an `InteractionClassHandle` and have no target
object. Directed interaction stays a distinct exception/service path.

Evidence remains layered: the embedded case exercises the actual
`receiveInteraction` callback; the process Send Interaction case decodes a
pulled event envelope rather than asserting a public callback; the process
attribute case observes discovery and a later explicit update reflection, not
a discovery-triggered snapshot. These are not matched profile-parity tests.
The pinned headers expose clause labels and signatures, not normative clause
prose; no new service rule or conformance claim was added. The chart count did
not change.

The 2026-10-11 screenshot review applies to the prior chart source only. In
this shell `mmdc` is not discoverable and the prior Scoop install attempt found
no configured manifest, so this revision has no fresh parser, render, or
screenshot verdict. No C++ build, test, CI, 2010, or Requirements Lab work
occurred.

Next handoff: render and screenshot-review the updated existing map with a
GitHub-compatible Mermaid preview; record that result separately from the
historical 2026-10-11 screenshot. Then continue the prioritized HLA-first
guide audit without compiling or rerunning tests. Preserve 2010/2025 and
embedded/process boundaries.

---
### 2025 Federation MOM request/report outcome boundary — 2026-10-12

Reviewed the request-driven Federation MOM path against the pinned 2025 MIM,
the embedded `SendInteraction` handler, and existing focused Catch2 sources.
Revised the existing sequence to separate the caller's synchronous
`sendInteraction` result, the requested RTI `HLAreport...` state response, and
switch-selected per-invocation reporting (`HLAreportServiceInvocation` for the
interaction sink, otherwise a file record). The test supports normal return
before observer evocation and no report after its invalid-index and unexpected-
parameter cases, only in the selected embedded `HLA_EVOKED` path. Source places
the selected per-invocation report step before federation-report enqueue; the
test does not assert both report families or their observer order. Federation
and member request roots remain distinct; no process or 2010 parity is claimed.

No fresh tests, build, CI, or Requirements Lab work. The revised chart has no
new syntax/render verdict: the current shell has no `mmdc`, and the authorized
`scoop install mmdc` failed during a permission-denied Scoop self-update and
then found no manifest. Visual review remains pending in the chart ledger.

Next handoff: bounded HLA-first review of the 2025 object-discovery versus
first-attribute-reflection boundary. Keep the exact §§6.9/6.11 normative
sequence question separate from implementation ordering and focused-test
observations; preserve 2010 and 2025 embedded/process boundaries. Do not build,
rerun tests, or start CI.

### 2025 Service Invocation Reporting guide review — 2026-10-12

Reviewed the guide's HLA model against the pinned 1516.2 MIM definition,
bounded embedded routing/reservation source, and its focused evidence map. It
already separates the public service return or typed exception, the
`HLAreportServiceInvocation` record/interaction, and request-driven Federation
MOM reports. The embedded planner selects a sink from the subject's switches,
scopes interaction reports by `HLAfederate` and `HLAserviceGroup`, and reserves
a serial only when an eligible observer exists. The directed-send test proves
report-before-application-callback order only for its selected HLA_EVOKED case.
The guide appropriately labels its file, timestamp, and ConnectionLost failure
edges as profile-specific and keeps untested schedules open. Its 51 Markdown
links resolve, with no missing file targets or out-of-range line anchors.
No reader gap justified another diagram or content edit; no test was run.

Next handoff: bounded HLA-first review of the 2025 Federation MOM guide. Make
the request-driven `HLArequest...` / `HLAreport...` exchange distinct from
per-invocation reports: identify the request trigger, state source, response
service outcome, and later observer callback. Reuse existing pinned API,
source, and test evidence; add a diagram only for a genuine gap. Preserve 2010
separation and embedded/process evidence boundaries; do not build, rerun tests,
start CI, or edit the Requirements Lab.

### Current Mermaid syntax pass and label repairs — 2026-10-12

The local Mermaid CLI 12.0.0 was already available, so Scoop installation was
unnecessary. A parser-only check over the current 252 Mermaid fences found four
sequence-chart syntax failures caused by semicolon separators. Reworded those
labels in the 2025 TSO retraction, federation synchronization, exception
reporting, and callback/service-ordering guides without changing behavior.
Rechecking the current source found no additional parser errors: 81 diagrams
were accepted, while 171 stopped with `DOMPurify.addHook is not a function` in
the Node parser environment and therefore have no syntax verdict. This is not a
full syntax pass. The separate all-current SVG render still cannot launch
Puppeteer/Chrome, so the four edited charts remain visually unreviewed.

Also reviewed the callback/service-ordering guide against bounded dispatcher,
session, and process-Resign source. It already separates callback-model and
service-contract questions from dispatcher mechanics, consumed-task exceptions
from producer-specific catches, and projection cancellation from queue/session
reset. Its possible post-Resign dispatch path is explicitly source-derived;
focused public evidence and normative permission remain open. No new diagram or
behavior claim was warranted.

No new diagram was warranted; no C++ build, tests, CI, 2010, or Requirements
Lab work occurred.

Next handoff: content-first, bounded HLA-first review of the 2025 Service
Invocation Reporting guide. Separate the operation's public return/typed
exception from the recorded service report, report routing/correlation, and MOM
state; compare the pinned API, bounded source, and existing test evidence. Add
a diagram only for a genuine portable reader gap. Reuse results without builds
or test reruns, preserve 2010 and 2025 embedded/process boundaries, and keep
the Requirements Lab out of scope.

### 2025 discovery versus first reflection — 2026-10-12

Audited the object-information and subscription-state guides against the
pinned 2025 `FederateAmbassador` declarations, the process receive selector,
the embedded callback boundary, and the focused public process update case.
The process selector explicitly selects pending discovery before reflection
for a newly registered object; its comment says this matches official
callback ordering. The focused process case registers, sends an update, and
checks that both `discoverObjectInstance` and `reflectAttributeValues` ran,
but stores independent booleans and does not record callback order. The
vendored interface labels the callbacks §6.9 and §6.11 but does not include
the normative clause prose. The official IEEE downloads page links a
personal-use archive that was retrievable, but its nested package contains
API sources and schemas rather than the standard text; the API header repeats
the clause labels without an ordering rule. The process comment therefore
remains an implementation interpretation, not an independently verified
normative claim. The focused process update's reflection follows a separate,
explicit publisher `Update Attribute Values` call; it must not be conflated
with a discovery-triggered current-value snapshot. In the inspected process
discovery enqueue path, a follow-up initial-value event is built only for
RTI-owned MOM objects. The subscription-state guide records this separately
from the embedded ordinary initial-reflection path and its process test gap.
These are profile-specific source observations, not a 2025 normative rule or
an embedded/process parity claim. See the living index's normative
source-access audit.

Added an explicit evidence boundary in the object guide and clarified that
the subscription chart's vertical callback placement is scenario chronology.
No Mermaid changed, so no render or screenshot review was needed. No build,
test rerun, CI, 2010, or Requirements Lab work occurred.

Next handoff: perform a bounded 2025 HLA-first audit of interaction receipt
versus object-attribute reflection. Verify recipient eligibility and callback
boundaries against the authorized standard, then compare selected
embedded/process source and focused tests. Add a diagram only if this reveals a
portable reader gap. Carry forward the open §5.8 and §§6.9/6.11 questions
without treating Umbra's process selector or tests as normative evidence.
Preserve 2010/2025 and embedded/process boundaries; do not build, rerun tests,
start CI, or edit the Requirements Lab.

---
### 2025 DataElement carrier versus encoded-value length — 2026-10-12

Audited the existing 2025 DataElement guide against the pinned `DataElement`
and `VariableLengthData` APIs, representative basic/fixed-record/variable-array
codecs, the variable-array encoding case, and the separate VariableLengthData
storage tests. The existing guide already covers explicit byte order, nested
composites, counts/padding, time factories, and separation from handles, model
input, and private process framing. The missing reader distinction was between
the public `VariableLengthData` carrier's `size()` and a length/count encoded
inside the value. Added a compact API-surface map: `encode()` returns a new
carrier; selected current codecs replace data in a supplied carrier via
`setData`; `encodeInto(vector&)` appends. A three-layer explanation now
separates carrier octet count, `HLAvariableArray`'s signed BE element count,
and private process-frame metadata. The overwrite detail is source-observed;
the reviewed focused tests do not seed a prefilled carrier then test that
overload. The callback-buffer copy/pointer-lifetime rule links to the object
guide instead of being duplicated. The first of seven existing diagrams was
revised to name the carrier; no new diagram was added. The 2010 guide was read
only to check separation and was not changed; no behavior parity is inferred.

The seven diagrams had prior render/screenshot evidence, but the edited first
flow has not been rerendered. Authorized `scoop search mmdc` found no package in
configured sources; no renderer was installed. No C++ build, test rerun, CI,
2010, or Requirements Lab work occurred.

Next handoff: audit the existing 2025 Federation MOM guide for the HLA-first
distinction between execution-scoped management objects, member-scoped
objects, and observer knowledge. Compare its existing flows with the pinned
2025 MIM/API and bounded source/test evidence; keep implementation-specific
projections labeled, preserve 2010 separately, add no diagram unless a
concrete reader gap appears, and do not build or rerun tests.

---
### 2025 federation membership: rejected Disconnect and post-commit Join effects — 2026-10-12

Compared the local ambassador state chart with the pinned 2025 Disconnect
contract and selected federation-lifecycle test. The chart omitted the
`FederateIsExecutionMember` rejection when a joined caller attempts Disconnect;
added the rejected self-loop and direct API/test links. The guide now also
cross-links the embedded additional-FOM Join edge documented in the FOM guide:
post-commit report/lifecycle setup may fail and remove the new member without
restoring the replacement catalog. That additional-FOM failure is source-only;
the focused writer-failure case covers an ordinary Join, and no test asserts
catalog state after the additional-module failure. No normative rule or
process/2010 parity is inferred.

The existing Mermaid state diagram was revised but not rendered. `scoop search
mmdc` found no package in configured sources even with the authorized network
lookup; no renderer was installed. Markdown fence checks are balanced. No C++
build, test rerun, CI, 2010, or Requirements Lab work occurred.

Next handoff: audit the existing 2025 Data Element encoding guide against the
pinned API and focused codec evidence. Clarify the HLA-facing boundaries among
value encoding, byte order, variable-length data, and RTI-specific framing;
keep the 2010 codec and evidence separate, add no diagram unless a concrete
reader gap remains, and do not build or rerun tests.

---
### 2025 FOM module admission and additional-module Join commit boundary — 2026-10-12

Reviewed the module-admission/composition guide against the pinned 2025 API
and bounded embedded registry, ambassador, and test paths. Its DIF,
complete-model composition, and FDD boundaries remain clear. The Join diagram
previously stopped at the atomic registry commit, while the prose implied that
all later public-service failures preserve the old catalog. It now shows the
post-commit service-report/lifecycle setup: a failure there resigns the new
member but does not restore the replacement definition. This is an
implementation-specific source observation, not portable HLA behavior. The
existing writer-creation failure test covers ordinary Join and member-name
reuse only; it does not test an additional-module failure after commit or
inspect the catalog state. Existing successful-addition lookup cases remain
embedded 2025 evidence; the process case covers the joining client's lookup
only. No 2010 parity is inferred.

The revised diagram was not rendered: `mmdc` is unavailable, and the prior
authorized Scoop install attempt found no configured manifest. Syntax/render
and visual inspection remain pending. No C++ build, test rerun, CI, 2010, or
Requirements Lab work occurred.

Next handoff: audit the existing 2025 Federation and Federate Lifecycle guide
for the HLA-first membership mental model across Join, resignation, connection
loss, and execution destruction. Separate portable participant/execution state
from Umbra service rollback and callback-observation mechanics; preserve 2010
separately and do not build or rerun tests.

---
### 2025 Federation Synchronization: report result vs service failure — 2026-10-12

Audited the announce/achieve/federation-synchronized path against the pinned
2025 API, the embedded achievement service, the notification submission path,
and the focused public synchronization scenario. The existing diagrams already
separate registration and announcement from participant reports and aggregate
completion. The reader gap was at the `synchronizationPointAchieved` call
boundary: `successfully=false` is accepted result data, whereas an
unannounced/removed label raises the typed
`SynchronizationPointLabelNotAnnounced` exception. Added this distinction and
direct API/test/source links to the fan-in section, and clarified the existing
sequence so the service return and aggregate callback are not presented as one
event.

The test accepts both true and false reports, observes the false reporter in
`failedToSyncSet`, and checks the typed exception on a report after completion.
The embedded service submits final notifications inside its service method after
the registry operation; paired HLA_EVOKED/HLA_IMMEDIATE cases do not assert
callback-entry order against the final service return. Process and 2010 parity
remain unclaimed. `scoop install mmdc` failed because no configured manifest was
found, so the revised existing sequence remains unrendered. No C++ build, test
rerun, CI, or Requirements Lab work.

Next handoff: audit the 2025 FOM module admission/composition guide's module
dependency and Join boundaries. Keep portable module-composition rules distinct
from Umbra parser/registry mechanisms and callback timing; compare the current
diagrams with pinned API and bounded implementation/test evidence. Preserve
2010 separately, add no chart unless a concrete reader gap appears, and do not
build or rerun tests.

---
### 2025 Attribute Ownership: synchronous service boundaries — 2026-10-12

Audited the If-Available and If-Wanted paths against the pinned 2025 API,
ownership-service entry points, and their focused test bodies. The diagrams
already separate pending acquisition from callback-time outcomes, and
If-Wanted's output-set transfer from later notification delivery. The reader
gap was that synchronous typed exceptions and subsequent state-query evidence
were distributed across the prose and diagrams. Added a comparison table to
the ownership guide: service exception/return, per-attribute callback, and
`isAttributeOwnedByFederate` are now explicit independent boundaries.

The If-Available case checks typed publication, handle, and ownership errors;
it observes false ownership after normal return and true ownership after the
notification callback begins. The If-Wanted case checks typed service errors,
an empty result with retained ownership, and a synchronous nonempty transferred
set whose new owners are observable before callbacks are evoked. These are
selected embedded `HLA_EVOKED` examples, not a complete exception matrix or
process-parity claim. 2010 remains separate. No chart changed, so no Mermaid
render was needed; no C++ build, test rerun, CI, or Requirements Lab work.

Next handoff: audit the 2025 federation-synchronization guide's announce,
achieve, and synchronized milestones. Keep the portable HLA protocol distinct
from callback dispatch and service-return timing; compare the existing diagrams
with the pinned API and bounded source/test evidence. Preserve 2010 separately,
add no chart unless a concrete reader gap appears, and do not build or rerun
tests.

---
### 2025 known-object subscription: independent effect gates — 2026-10-12

Audited the late-subscription comparison against the bounded source/test
evidence. The known-object diagram previously sent a scope-change decision to
both the receiver's scope-advisory path and the unresolved new-attribute
current-value question as mutually exclusive branches. It now computes a
per-attribute scope delta and evaluates the two effects independently; the
provider-facing relevance advisory remains a separate path. This avoids
implying that a scope notice carries a value or rules out a value callback.

The source trace shows subscription mutation returns receiver scope-change and
owner relevance-advisory plans, not current-value recipients
([registry](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L261)).
The discovery planner skips instances already known to the receiver
([planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L83)),
while the embedded ordinary initial-value planner runs after discovery enters
user code
([callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L375)).
The focused unknown-to-known test asserts discovery and the selected stored
value
([test](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180)).
The additive-subscription test instead changes per-attribute modes after
discovery and checks a later publisher update, not an intervening value-only
callback
([test](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L355)).
Process evidence remains bounded: the private advisory case tests owner
notifications, and the process initial-value enqueue has a separate RTI-owned
MOM path. No focused ordinary process late-subscription value test settles the
question. Licensed §5.8 wording remains unreviewed; no normative result is
claimed.

No new chart was added and the inventory count is unchanged. The revised
diagram remains unrendered because the configured Scoop buckets have no
`mmdc` manifest. No C++ build, test rerun, CI, 2010 work, or Requirements Lab
work occurred.

Next handoff: audit the 2025 Attribute Ownership guide's If-Available and
If-Wanted acquisition/divestiture outcomes. Separate service acceptance and
typed exceptions from per-attribute callbacks and later ownership queries;
check existing diagrams against the pinned API, bounded source, and focused
tests. Keep 2010 separate and do not build or rerun tests.

### TSO retraction walkthrough: embedded interaction boundary — 2026-10-12

Compared the portable retraction orientation with the mixed-recipient example
and its interaction test body. The portable chart stays at the HLA question
level; the sequence uses an embedded queue and recipient ledger, so it now
labels itself as one 2025 development-profile interaction example. Replaced
the ambiguous `B+L<T` notation (where B was also recipient B) with the
producer-side §8.22.3 legality gate. The test uses `HLA_EVOKED`; recipient A
evokes the original callback before Retract, so the sequence describes
callback ordering rather than a synchronous callback from `sendInteraction`.
It now links the separate attribute-update recipient comparison in the time
guide.

The interaction test body asserts delivered-recipient callback order and
queued-recipient suppression, but no fresh test result is recorded. The test
is source evidence only in this increment; no C++ build or test ran. The
diagram's sequence structure changed only in its participant/step labels and
one explanatory note. `scoop install mmdc` found no manifest, and Scoop search
could not reach its remote sources; the revised diagram remains unrendered.
No 2010 or process-profile parity is inferred, and the Requirements Lab is
untouched.

Next handoff: audit the 2025 object/interaction information guide's
known-object/new-attribute path against the pinned API, bounded implementation
sources, and existing focused tests. Distinguish initial-value behavior from
DSE-delayed delivery of an earlier send; keep embedded/process evidence and
2010 separate. Add no chart unless a concrete reader gap requires it, and do
not build or rerun tests.

### 2025 timestamped attribute mixed fan-out — 2026-10-12

Audited the time-management guide's queue lifecycle against the focused
embedded timestamped attribute-update case. The queue state diagram now says
it follows an individual TSO-queued copy, not every recipient of a timestamped
service. Added a compact recipient table: the time-constrained recipient's
copy is withdrawn before its grant and gets no reflection or Request
Retraction; the non-time-constrained recipient receives the timestamped
reflection with sent order TIMESTAMP and received order RECEIVE, then receives
Request Retraction after the reflection in the observed callback order.

The [focused test](../../cpp/tests/timestamped_default_region_attribute_mixed_fanout_catch2.cpp#L155)
asserts those outcomes and the shared valid retraction designator; the
[attribute adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_attribute_update_services.cpp#L637)
shows the constrained-recipient queue selection. The existing [TSO
retraction-guide mixed-recipient timeline](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#a-mixed-recipient-walkthrough)
already teaches the generic delivered-versus-pending split, so no duplicate
chart was added and the diagram inventory remains 249. This is embedded
attribute evidence only, not process-profile or 2010 parity.

No diagram was changed, so no renderer was needed. No C++ build, test rerun,
CI, 2010 work, or Requirements Lab work occurred.

Next handoff: audit the TSO retraction guide's mixed-recipient walkthrough
against the available interaction and attribute evidence. Keep its generic
HLA distinction separate from the selected embedded attribute case and from
process event routing; preserve the existing caveat that the interaction
mixed-fanout test has no fresh result. Do not rebuild or rerun it.

### 2025 Delay Subscription Evaluation: recipient-specific TSO boundary — 2026-10-12

The DSE guide already separated eligibility from sender validity, regional
predicates, callback dispatch, and time advance. The audit found one wording
risk: “receiver TSO delivery/grant boundary” could read as a gate for every
timestamped recipient. The two affected diagrams now limit that lane to a
time-constrained receiver. Added evidence that a non-time-constrained recipient
can follow a receive-order callback path in the embedded profile; embedded
interaction/attribute callback source rechecks the current projection at
callback entry, while the process interaction service re-resolves a deferred
recipient before event construction. The eight focused DSE test files are all
embedded; their timestamped cases enable time constraint, so the unconstrained
DSE path and process behavior remain source-observed, not DSE-test-demonstrated.
The 2010 guide remains separate and no cross-edition rule is inferred.
Two changed diagrams were not rerendered: configured Scoop buckets still have
no mmdc or mermaid-cli manifest. No C++ build, tests, CI, 2010, or Requirements
Lab work occurred.

Next handoff: audit the 2025 time-management guide and bounded mixed-fanout
evidence for timestamped messages delivered to time-constrained and
non-time-constrained recipients. Check that TSO queue/grant, receive-order
callback, retraction, and per-recipient order distinctions are explicit without
treating them as a single pipeline. Keep all evidence 2025-specific and do not
build or rerun tests.

### 2025 DDM region-template deletion gate — 2026-10-12

Compared the 2025 region-template state chart with the focused embedded
region-lifecycle tests and registry usage ledger. The existing chart already
separates a region's fixed dimension set, pending bounds, and committed routing
snapshot, but omitted the distinct handle-lifetime gate. Added a small adjacent
flow: live regional subscriptions or object update-region associations cause
`RegionInUseForUpdateOrSubscription`; release of the last dependency permits
deletion, after which a later query fails with `InvalidRegion`. Some
object-removal paths retain an association until their final removal callback.
Evidence remains embedded 2025-only: passive regional subscription
([test](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L134)),
registered-object update association
([test](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L221)),
callback-delayed association release
([test](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L369)),
and registry usage guard/ledger
([delete guard](../../cpp/src/internal/federation/federation_registry_regions.cpp#L320),
[usage ledger](../../cpp/src/internal/federation/federation_registry_regions.cpp#L732)).
The same test confirms a stale-handle query fails after deletion
([assertion](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L161)).

I did not install a renderer. `scoop install mmdc` and `scoop install
mermaid-cli` found no configured-bucket manifest; the first command refreshed
Scoop bucket metadata but installed no package. I did not send chart text to a
remote renderer. The new chart remains unrendered and requires visual review.
No C++ build, test rerun, CI, 2010 work, or Requirements Lab work occurred.

Next handoff: audit the 2025 Delay Subscription Evaluation guide's distinction
between the time eligibility is evaluated, sender-service validity, receiver
DDM eligibility, receive-order callback dispatch, and TSO grant eligibility.
Reuse existing evidence and diagrams if they already teach this clearly; keep
the 2025 embedded/process and 2010 boundaries intact, and do not build or rerun
tests.

### 2025 cross-cutting diagram set: static audit — 2026-10-11

Completed a static content pass across all 11 maps. The delivery axes are
questions rather than a fixed pipeline; update-rate admission is labeled as a
bounded embedded example; completion and participant-set maps keep protocols
separate; and stale-work outcomes remain scoped by operation and recipient
phase. Service-result/report and contract/commit maps keep implementation
observations distinct from portable HLA rules. No additional synthesis map
was warranted.

Shortened the model/runtime and class-hierarchy charts, retaining registration
versus discovery, separate object/interaction paths, inherited lookup, and
receiver-known-class projection. The prose identifies the selected 2025
embedded/receive-order scope and distinguishes test evidence from source-only
behavior. Link anchors, `git diff --check`, and roadmap JSON parsing were
verified. Screenshot review remains pending: external preview would transmit
repository-derived chart text, configured Scoop sources lack Mermaid CLI, and
the local file-panel request was queued without a visible render. No C++
build/tests/CI, 2010 change, or Requirements Lab edit.

Next handoff: compare the current HLA guide portfolio with pinned 2025
API/service categories to find any major learner-facing behavior area that is
missing or only explained as Umbra implementation detail. Inspect a bounded
relevant API/docs and source/test slice; add an HLA-first guide only if a real
reader gap is demonstrated. Keep 2010 separate and defer visual review until
an approved local renderer is available.

### 2025 API-family coverage survey — 2026-10-11

Compared the current flow-guide inventory with the seven service families in
the pinned `RTIambassador` interface: Federation Management, Declaration
Management, Object Management, Ownership Management, Time Management, Data
Distribution Management, and RTI Support Services. The existing reader-path
matrix links each family to one or more topic guides; the 29 2025 flow guides
plus two separate 2010 guides leave no service-family-level gap. A targeted
read of RTI Support Services found corresponding paths for names/handles,
update rates, order/transportation, dimensions/ranges, advisories, callbacks,
time factories, and switch/report controls. This is a topical survey, not a
method/overload or exception-contract completeness claim; the index already
warns that its table is not method-by-method coverage.

Next handoff: do a bounded method-level audit of the 2025 RTI Support Services
surface against the existing topic paths. Identify any public operation whose
HLA meaning is genuinely absent (not merely not spelled with its C++ method
name); add only the smallest HLA-first explanation that closes a demonstrated
reader gap. Keep 2010 separate and leave visual review pending until an
approved local renderer is available.

### 2025 Support query distinctions — 2026-10-11

Continued that method-level survey with object-instance lookup and DDM
dimension/range queries. The names-and-handle guide now links
`getObjectInstanceHandle(name)` and `getObjectInstanceName(handle)` to the
pinned API and the existing configured-process test; owner-side round-trip
and unknown-name/handle results are process evidence, while peer discovery
timing remains a separate embedded case.

The DDM guide now gives separate names and meanings to class-available
dimensions, a region's fixed dimension set, and its per-dimension range
bounds. It links the pinned class-availability, `getDimensionHandleSet`, and
`getRangeBounds` declarations; the conceptual FOM-to-region chart includes
region-dimension read-back. Embedded lifecycle evidence establishes that the
shape is queryable after creation before ranges are committed, that an empty
shape is zero-dimensional rather than a wildcard, and that pending bounds can
be read before commit. This does not establish process DDM parity.

No new guide was warranted. This is a focused semantic clarification, not a
complete RTI Support method, overload, exception, or normative-clause audit.
The visual review remains pending: standard configured Scoop buckets have no
Mermaid CLI manifest, and no external preview of repository-derived diagram
text was used. No build/test/CI, 2010, or Requirements Lab work.

### 2025 RTI Support switch-control distinction — 2026-10-11

The bounded follow-up compared remaining Support query, advisory, and callback
controls with their existing guide paths. The cross-cutting switch map now
distinguishes four getter-only federation switches marked `Static` in the
2025 MIM from federation-wide Auto Provide, which is `Conditional` in the MIM
and is changed through Umbra's embedded federation MOM path. The existing
Auto Provide and Federation MOM guides hold the behavior details; the main
service-family map links readers to the clarified control taxonomy. This is
not a method/overload/exception audit or a process/2010 parity claim. No new
diagram was warranted or changed, and no C++ source, build, tests, CI, or
Requirements Lab work occurred. Scoop's configured-host lookup failed DNS;
because no diagram changed, `mmdc` was not needed for this increment.

### 2025 Ownership completion signals — 2026-10-11

The bounded 2025 ownership survey found a real reader gap in two completion
signals. The attribute-ownership guide now distinguishes regular-acquisition
cancellation acceptance from the later confirmation callback, including the
publication/If-Available gates that persist until confirmation in the focused
embedded case. It also documents `attributeOwnershipDivestitureIfWanted`'s
synchronous `divestedAttributes` output separately from the later
acquisition-notification callback; the test observes ownership changed before
evocation and checks the divestiture tag. The RTI Support API declarations and
the existing 2025 embedded target/tests anchor the new material; no normative
arbitration rule or process/2010 parity is inferred.

Replaced the overlapping generic state chart with a compact regular-request
and cancellation flow, and added one scenario sequence for Divestiture If
Wanted. Both changed charts were rendered locally with Mermaid CLI and
screenshot-reviewed. Scoop has no configured `mmdc` manifest, so the CLI was
run from a temporary install outside the repository. No C++ source, build,
test rerun, CI, 2010 stream, or Requirements Lab work occurred.

Next handoff: do a bounded 2025 Object Management lifetime survey against the
pinned API and the object/interaction, attribute-ownership, and lifecycle
guides. Focus on the distinction among federation-wide delete, caller-local
forget, delete privilege, and receiver removal observation; add only a
demonstrated reader gap. Keep 2010 separate and preserve embedded/process
evidence boundaries. Do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 Delete-privilege publication epoch — 2026-10-11

The Object Management lifetime review confirmed that the guides already
separate federation-wide deletion, caller-local forgetting, delete privilege,
and receiver-side removal. The additional gap was narrower: `HLAprivilegeToDeleteObject`
is an instance attribute whose implicit publication follows a class
publication epoch, so unpublishing it can split registrations of one class
into different delete-eligibility cohorts. Added a sequence to the object /
interaction guide using the existing 31-assertion embedded
`DELETE_OBJECTS_THEN_DIVEST` case: a retained object loses the privilege,
republishing affects a later registration but not the old instance, and the
resignation scenario removes only the later object while offering the
retained object's `Efficiency` ownership to a peer. The chart labels the
observed assumption-before-removal callback order as case-specific.

The pinned 2025 MIM, the registry's §5.1.2 / §5.3.3(f) source comments, and
the test-plan row (mapped to §4.12) anchor the explanation. The test does not
prove the complete standard publication predicate or process parity. All 18
Mermaid charts in the object / interaction guide rendered locally; the new
sequence was visually checked. No C++ source, build, test rerun, CI, 2010
stream, or Requirements Lab work occurred.

### 2025 Join-during-save/restore audit and synchronization-map visual review — 2026-10-11

The bounded Join audit confirmed an evidence gap, not a resolved runtime or
conformance result. Both pinned 2025 Join overloads list `SaveInProgress` and
`RestoreInProgress`. The inspected embedded Join transaction and process Join
handler do not show an explicit active-save/restore admission predicate or
update to the operation's participant-status cohort; the existing mapped
save/restore interlock case joins its participants before requesting save and
does not attempt a Join during either protocol. These are bounded source/test
observations, not proof that all paths lack a gate or that a late Join is
accepted. The exact §4.11 preconditions and participant effects remain
unresolved because the pinned checkout does not include authorized normative
text. The save/restore guide already records that boundary, so no new Join
diagram is justified.

The same visual pass found a real clarity issue in the HLA-level
synchronization map: successful registration was drawn as two parallel `Yes`
outcomes, making a registrar result and participant announcements look
mutually exclusive. The diagram now shows registration accepted before the
two distinct notification paths, without imposing callback order. All seven
diagrams in the synchronization guide rendered with the temporary local
Mermaid CLI; the revised HLA-level map was visually inspected. This is a
conceptual API-event map, not a claim about exact normative callback ordering.
No C++ source, build, test rerun, CI, 2010 stream, or Requirements Lab work
occurred.

### 2025 Lifecycle ResignAction intent and member-list visual review — 2026-10-11

The local render exposed a readability and sequencing issue in the 2025
ResignAction intent map: six peer branches made the enum labels too small to
scan, and the flow placed the service call before selection of its argument.
The map now presents the six API values and their requested cleanup intent in
one readable vertical choice block, then shows the service call, abstract
acceptance/rejection decision, successful membership end, and separate
execution lifetime. It does not turn the embedded action matrix into a
normative decision table or claim that a requested cleanup always completes.
The existing prose keeps exact guards and effects bounded to available
standard/profile evidence.

All 14 diagrams in the lifecycle guide rendered locally. I visually reviewed
the revised ResignAction map and the adjacent member-list snapshot flow; both
are legible, and the snapshot flow's API callback outcomes remain distinct
from its embedded connected/unjoined caller evidence. No content change was
needed for that second map. No C++ source, build, test rerun, CI, 2010 stream,
or Requirements Lab work occurred.

### 2025 federation MOM, update-request, and lifecycle visual review — 2026-10-11

The queued Federation MOM review found the conditional-update trigger map was
too wide: four producer/field branches compressed its labels. Replaced that
fan-out with a four-row representative embedded-producer-to-value table and a
compact vertical pipeline from authoritative snapshot update to reflection
planning and callback-model dispatch. The table remains profile-bounded; the
MIM's Conditional metadata and the selected tests are linked separately. All
seven diagrams in the federation MOM guide rendered, and the revised pipeline
was visually inspected.

The four diagrams in the attribute-value update-request guide also rendered.
The HLA-facing sequence had a Mermaid parse failure around nested annotations
and was visually crowded; it now uses concise participant and event labels,
without auto-numbering or note boxes that made a causal model look more
prescriptive. Its surrounding prose still distinguishes solicitation from a
value-bearing update, request and response tags, callback timing, eligibility,
and timestamped delivery. The final sequence was visually inspected. All 14
diagrams in the lifecycle guide rendered; its revised ResignAction map, the
member-list snapshot flow, and the federation/member MOM-lifetime overlay were
visually reviewed. Only the ResignAction map needed adjustment. These are
local preview checks, not GitHub-render claims. No C++ source, build, test
rerun, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 object/interaction publication and callback-boundary visual review — 2026-10-11

Locally rendered and visually inspected the ordinary publication-withdrawal-
after-send sequence and callback-boundary flowchart in
`HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md`. The sequence was
too wide at its prior five-lifeline layout, so it now uses sender, embedded
RTI, and receiver lanes. It keeps successful send return, later unpublication,
the already-accepted route remaining pending, `evokeCallback()`, and the
receiver-side eligibility/projection recheck distinct. The ordinary path does
not recheck sender publication at callback time; this remains a 2025 embedded
source observation, with no focused send → unpublish → evoke test. The separate
directed-interaction publication fence is not merged into this ordinary path.

The callback-boundary chart is tall but readable: `HLA_EVOKED` queues work,
`HLA_IMMEDIATE` may enter receiver code synchronously, and both converge on
receiver-projection re-evaluation before best-effort per-attribute update-rate
admission and reflection or suppression. This is not a universal callback
ordering or process-parity claim. GitHub rendering remains pending. No C++
source, build, test rerun, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 callback, observation, and time-management cross-guide survey — 2026-10-11

Compared the callback/service-ordering guide's callback model and dispatcher
sections with the time guide's temporal-eligibility/callback-observation and
receive-order/TSO flows, then checked the cross-cutting guide's delivery versus
observation model and service milestones. Together they distinguish service
acceptance and return, callback-task submission, eligibility, callback entry,
callback return, application observation, and service-specific state or
protocol completion. The time guide shows the extra temporal gate and the
HLA_EVOKED wait; the callback guide keeps HLA_IMMEDIATE's possible
service-stack invocation distinct. The cross-cutting guide defines callback
entry as the application-observation boundary and links the focused guides.

Finding: the specific learner gap in the previous handoff is already addressed;
another generic flow would duplicate the existing HLA mental model. No diagram
or source change was justified. The evidence remains service- and
profile-bounded; no total cross-service order, complete normative commit
matrix, process/embedded equivalence, or 2010 parity is inferred. No C++
source, build, test rerun, CI, or Requirements Lab work occurred.

### 2025 ownership and delivery cross-guide survey — 2026-10-11

Compared the core attribute-ownership guide's per-attribute regular,
If-Available, negotiated, and output-set divestiture examples with the
ownership persistence/resignation companion and the cross-cutting
state/event/observation map. The core guide separates void service return,
typed synchronous exceptions, request-pending state, per-attribute callback
outcomes, and callback-time ownership observations; the companion keeps
durable ownership records distinct from live callback routes and current
membership. The object/request/DDM guides keep provider solicitation,
ownership, receiver relevance, and eventual reflection on separate service
paths. The cross-cutting map already links these questions without asserting a
universal pipeline. No additional ownership or cross-service flow was
warranted. Exact normative arbitration/action rules and unsupported profile
behavior remain outside the selected evidence.

### 2025 DDM scope/time/callback content review — 2026-10-11

Re-read the diagram and adjacent prose in the “Keep time and DDM as separate
filters” section. The chart distinguishes region-pair overlap, receive-order
versus timestamp-order, service-specific callback gates, and the selected
callback model; adjacent prose explicitly says the arrows are not a required
internal RTI order. No content change was indicated. This was a source/prose
review only, not a visual or Mermaid-parser pass.

The visual review remains pending. Scoop could not find an `mmdc` manifest;
the temporary bundled-pnpm renderer failed before startup because of missing
transitive modules. The local browser rejected the attempted `data:` preview
URL, so no alternate browser surface or external editor was used. No diagram,
C++ source, or project dependency was changed. No build, test rerun, CI, 2010
stream, or Requirements Lab work occurred.

### 2025 federation/federate exit-boundary survey — 2026-10-11

Compared the lifecycle guide's voluntary ResignAction and ConnectionLost
sections, callback notes, evidence map, and deliberate limits. The existing
flows distinguish the caller's void service result/typed rejection, the
successful membership transition, the continuing federation-execution
lifetime, local stale-callback cleanup, and independently routed survivor
notifications. Connection loss is a distinct forced entry path: the local
`connectionLost` report is not sent over the failed connection, while
survivor callbacks depend on the directive, known objects, switches, routes,
and profile. The companion guides own save/restore, ownership, and time-gate
details. No additional generic lifecycle map was warranted.

The full normative ResignAction decision table and a total ordering for
survivor callbacks remain explicitly unclaimed; the guide points to the pinned
2025 authority and keeps the 2010 lifecycle in a separate reference guide.
The DDM scope/time/callback diagram remains source-reviewed but not visually
reviewed because an available local renderer could not be reached under the
current tool constraints. No C++ source, build, test rerun, CI, 2010 stream,
or Requirements Lab work occurred.

Next handoff: make a bounded HLA-first audit of the 2025 save/restore guide's
Join-during-progress boundary around §4.11. Separate when a new member joins
from the participant set and callback outcomes of an already active protocol;
check the pinned normative authority, existing guide, and cited source/test
evidence, and preserve uncertainty if the normative wording is unavailable.
Revise documentation only for a concrete reader gap. Keep the 2010 stream
separate and do not edit the Requirements Lab or run builds/tests/CI.

### 2025 late-subscription normative access and diagram-renderer follow-up — 2026-10-11

The official [1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers purchase/subscription access. IEEE SA's [access-options
page](https://standards.ieee.org/access-standards/) also offers read-only
access to selected standards, but this check did not establish that
1516.1-2025 is among them or expose §5.8. Keep the late-subscription rule
unresolved until an authorized copy or excerpt is available; the embedded
source comment is not normative verification.

The user authorized a Scoop install if needed. With socket access, searches
for mmdc and mermaid-cli returned no matches in the configured Scoop catalogs;
no package was installed. The late-subscription chart and three earlier
charts remain pending render/visual review. Do not switch package managers
without approval. This access/tooling check adds no HLA behavior claim and
does not justify another diagram by itself.

Next handoff: continue a bounded HLA-first source/test review only if it finds
a concrete reader question not already answered by the guide set. Keep the
ordinary process late-subscription outcome unresolved. Verify §5.8 only from
authorized text, and review pending diagrams only with an available renderer.
Do not build, rerun tests, start CI, edit the Requirements Lab, or merge the
2010 and 2025 behavior streams.

### 2025 callback re-entry: HLA permission versus dispatcher capability — 2026-10-11

A bounded review of the callback/service-ordering guide, dispatcher/session
source, and focused unit cases found that the main reader trap is already
handled: a recursive dispatcher can support nested same-thread callback work,
but that implementation capability does not grant blanket permission to call
RTI services from callbacks. The guide separately identifies public guards
for Connect, Disconnect, Join, Resign, and Evoke, and distinguishes internal
session close from the public Disconnect service. The focused tests cover
Evoke rejection and session close behavior, not a complete service-permission
matrix. The 2025 normative matrix remains unverified without the exact
standard text.

No new diagram or duplicate prose is warranted. Next content audit: trace the
HLA-facing question of a FederateAmbassador exception during immediate versus
evoked delivery—what call boundary observes it, and which accepted service,
report, or state effects remain—using exact API authority where available and
separate embedded/process case studies. Draw only if the existing exception,
service-outcome, and reporting guides leave a real conceptual gap. Keep 2010
separate; do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 callback data lifetime: borrow during the call, own what you retain — 2026-10-11

The proposed callback-exception audit found its HLA-facing call-stack and
report/state boundaries already described in the callback, exception-reporting,
and process-endpoint guides; no duplicate exception chart was warranted.
Instead, the value-path survey found a smaller but practical gap for federate
implementers: callback handle/value maps and tags are passed by `const&`, and
the 2025 `VariableLengthData::data()` pointer is not promised beyond its
object's lifetime or next replacement. Added a receiver-side flow to the
object/interaction guide: decode through callback arguments while active, or
copy needed data into application-owned storage before returning. The diagram
is explicitly 2025 C++ API guidance, not a private-frame or 2010 claim.

The embedded receive-order interaction test supplies a concrete safe pattern:
it copies the map and tag inside `receiveInteraction` and checks the saved
bytes after callback draining. This is selected test evidence, not a general
callback-lifetime conformance test or process-profile proof. Static census is
now 233 Mermaid blocks (133 flowcharts, 77 sequences, 23 state diagrams); the
new chart has not been rendered or syntax-checked, and the existing visual
review queue remains open. No C++ source, test, build, CI, or Requirements Lab
change occurred.

Next handoff: audit the 2010 FederateAmbassador and VariableLengthData API,
source, and focused tests independently for callback payload lifetime. Add a
separate 2010 explanation only if its own evidence supports one; do not reuse
the 2025 chart or infer edition parity. Keep the new 2025 chart and the prior
three pending diagrams on the render-review queue; Scoop has no Mermaid CLI
manifest, so use another installer only with explicit approval. Continue to
leave the ordinary 2025 late-subscription rule unresolved until authorized
§5.8 text is available.

### 2010 callback payload lifetime: use borrowed values now or copy to retain — 2026-10-11

The independent 2010 API/source pass confirmed its own reader-facing lifetime
guidance: reflection and interaction callbacks receive value maps and tags by
const reference; `VariableLengthData` permits deletion of the source after a
copy, but its raw `data()` pointer is valid only while its owning object remains
alive and unchanged. The 2010 reference runtime invokes callbacks inline, a
boundary already described in its guide. Added a separately labeled 2010
flowchart there: inspect borrowed values during the callback or copy the
needed value objects before returning. Its binding-shell smoke covers callback
delivery, not retention after callback return; this is API-use guidance, not a
test claim or inference from the 2025 stream.

The current static census is 229 Mermaid blocks across 32 of 41 design files:
14 diagrams in two 2010 guides, 213 in 31 2025 guides, and two in the index;
133 flowcharts, 75 sequences, and 21 state diagrams. The new 2010 chart and
the 2025 credential-envelope chart have not been locally rendered or
parser-checked. The user-authorized Scoop install/search found no Mermaid CLI
manifest, so no renderer was installed and no alternate installer was used.
No C++ source, test, build, CI, or Requirements Lab change occurred.

Next handoff: continue with the HLA timing mental model, surveying existing
2025 time-management diagrams for one specific learner gap around how
regulating/constrained federates, advance grants, and timestamp-ordered events
interact. Reuse existing source and focused-test evidence; add a chart only if
the current timing guide leaves a real conceptual gap. Keep 2010 independent,
do not resolve normative questions without authorized text, and do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 FOM composition and identity: existing reader flows cover the seam — 2026-10-11

The bounded FOM/name-resolution review found no missing reader mental model
that warrants another chart. The guides already separate per-module schema
admission, complete-set composition, FDD materialization, and atomic additional-
module Join; then distinguish class-hierarchy member lookup, typed opaque
handles, execution-scoped model symbols, and federate-local object knowledge.
They cross-link the composition-to-lookup path without treating a successful
model update as discovery or instance visibility.

The reviewed evidence includes cross-module reference-resolution cases,
embedded object-class lookup after an additional-module Join (including
stability of an earlier handle), inherited attribute lookup through a child
class, and the corresponding inherited-parameter cases. These are selected
embedded 2025 results, not proof of process parity or general conformance. The
FOM and name-resolution guides already state those bounds; no guide edits or
new chart were needed, and no tests were run.

Next handoff: review the separate 2025 Join-during-save/restore boundary. The
pinned API lists `SaveInProgress` and `RestoreInProgress`, while the inspected
embedded/process paths and interlock test do not settle exact Join timing or
participant effects. Compare only with authorized §4.11 and save/restore text;
do not infer the normative outcome from the exception list or Umbra source. If
the text is unavailable, retain the open question and proceed to another
documentable HLA area. Keep the 2010 stream separate; no build, test rerun, CI,
or Requirements Lab edit.

### 2025 timing and synchronization gap check — 2026-10-11

The timing scan found the existing guide already teaches the target
relationship: regulating and constrained roles; request versus grant; grant
bounds and modes; TSO eligibility/order; callback observation; and lookahead
changes/re-evaluation are handled as distinct but connected steps. The
synchronization guide also already has a source-labeled partial-achievement /
late-join sequence and plainly marks that exact interleaving as untested. No
new chart was justified; the latter remains an Umbra source observation, not a
normative 2025 claim. These were reader-gap checks, not builds or test runs.

Next handoff: survey the existing FOM composition and name/handle guides for a
genuine HLA mental-model gap around module composition, class inheritance,
model identity, and runtime handle resolution. Keep schema admission,
inheritance, and runtime lookup distinct; reuse existing evidence and add no
diagram if current guides already make those boundaries clear. Do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 authorization gate: no duplicate flow warranted — 2026-10-11

Audited the authorization guide against the pinned 2025 Authorizer interface,
Connect lifecycle path, shared connected-operation gates, and the Join
embedded/process split. The existing guide already makes the important HLA
distinction: the three authorizer callbacks are operation-scoped admission
decisions; an AUTHORIZED result does not guarantee the subsequent service's
own checks, state transition, or result. Its existing flows also show
operation-specific result mapping, credential inputs, and where Connect
commits. No second authorization chart was justified. Added direct links to
the separate service-invocation and exception-reporting explanations.

Source review confirms the connected-operation helper snapshots credentials
under the state lock, releases it before plugin code, and serializes calls
with Disconnect under the authorizer lock. The common gate precedes both
backend branches, but the cited operation cases are embedded-profile evidence;
the guide correctly does not claim process parity. Existing test results were
not rerun or upgraded. The [IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers purchase/subscription access; the [access options page](https://standards.ieee.org/access-standards/)
describes Reading Room access to select standards and sponsored GET downloads,
but neither confirms the exact normative clauses needed here are publicly
available. Keep the save/restore Join and Connect-overload questions open.
No C++ work, build, test, CI, or Requirements Lab change.

Next handoff: audit the existing 2025 callback-dispatch exception and
cancellation explanations for a reader gap around callback failure, already
committed protocol state, and withdrawn pending work. Reuse the callback/
service-ordering and exception-reporting guides plus their already indexed
source and focused-test evidence. Add no duplicate flow if those boundaries
are already clear; do not infer normative ordering or process/2010 parity.

### 2025 callback failure and cancellation: no duplicate flow warranted — 2026-10-11

The callback/service-ordering and exception-reporting guides already make the
important distinctions: a callback exception can unwind the inducing service
under HLA_IMMEDIATE or a later Evoke call under HLA_EVOKED; the failed
dispatcher task is consumed while later FIFO work remains; and producer-local
catches can change that path. The embedded Send Interaction example places
accepted reporting/counter effects before callback fan-out and labels
exception attribution and rollback behavior as source-derived, not tested or
normative. Cancellation, disabling, and callback exceptions are treated as
different transitions. No additional diagram or prose expansion was
justified.

The focused dispatcher suite covers normal completion and queue behavior but
has no throwing-task case. No cited public integration test throws from
Receive Interaction, and the process Resign cases do not assert the outcome
of unrelated queued work after resignation. The guides already state these
evidence limits. mmdc is absent, and the authorized Scoop search returned no
Mermaid package match; no alternate installer was used. Existing pending
charts remain unrendered and must not be described as visually verified. No
build, test, CI, or Requirements Lab work.

Next handoff: survey the 2025 ownership-transfer and resignation guides for
one learner-facing timeline that connects ownership state, ResignAction
effects, and callback invalidation. Compare only their existing API, source,
and focused-test evidence; keep action-specific claims and embedded/process
boundaries distinct. Add no overview if the current guides already explain
the transitions without conflation, and do not infer normative ordering or
2010 parity.

### 2025 ownership and resignation: existing flows stay separate — 2026-10-11

The bounded review found no need for a combined ownership/resignation diagram.
The lifecycle guide explains ResignAction vocabulary and selected cleanup; its
ownership companion isolates the role-sensitive pending-acquisition guard and
member-scoped cancellation; the core ownership guide covers its separate
transfer and invalidation cases. Those are adjacent effects, not one atomic
HLA state machine. Combining them with post-Resign callback routing would also
risk implying embedded/process parity.

Added a direct link from the lifecycle guide to the process callback guide and
labeled the preceding dispatcher-reset description as embedded-specific. The
process trace is source-derived: it shows a possible route for queued work but
does not establish post-Resign delivery or normative permission. The inspected
focused tests do not assert unrelated queued-task behavior after process
Resign. No normative action matrix or 2010 claim was added. No diagram was
created, so mmdc was not needed; existing render/visual-review gaps remain.
No build, test, CI, or Requirements Lab work.

Next handoff: survey the 2025 object-instance identity/lifetime path across
registration, deletion, timestamped delete/retraction, and name reuse. Compare
the object-management, name-reservation, TSO-retraction, and time guides with
their existing source and focused tests. Decide whether a single HLA-first
timeline clarifies execution-side existence versus federate-visible state;
keep receive-order/TSO and embedded/process evidence distinct, and add no
diagram if the current guides and links already explain the lifecycle.

### 2025 object identity and timestamped deletion: existing views suffice — 2026-10-11

The bounded review found the core identity/lifecycle distinctions already
explained in their owning guides: the name-reservation guide models Available,
Reserved, and Registered and ties reusable-name state to global object/name
purge; local deletion forgets one federate's view; accepted global deletion
cleans each known receiver independently; and timestamped deletion has a
separate queue/retraction path, including a source-specific reconstitution
branch.
Combining these into one lifecycle chart would conflate reservation state,
execution-wide object existence, per-federate knowledge, and TSO delivery.

Added cross-links and a focused note in the name-reservation guide: current
source can retain backing object/name state while a timestamped delete is
retractable, and its delivered-delete retraction path restores invocation-time
state before Request Retraction. The cited focused test checks receiver-side
lookup during a pending delete and after pre-delivery retraction, then unknown
status after a later removal; it does not assert reuse after purge or exercise
the delivered-delete restoration branch. These remain source/test-bounded 2025
observations, not a universal HLA timing or name-reuse rule. No 2010 parity.
No new chart, build, test, CI, or Requirements Lab operation.

Next handoff: survey the 2025 attribute update-rate admission and time guides
for whether one learner view should connect requested update intervals,
per-attribute admission/callback filtering, and TSO time eligibility. Compare
the pinned API, existing diagrams, source, and already indexed focused tests.
Keep rate admission distinct from timestamp-ordered delivery, embedded and
process evidence separate, and add no combined chart if current guides already
make those boundaries clear. Do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 attribute update rate and TSO: existing synthesis is sufficient — 2026-10-11

The bounded reader review found the relationship already explicit in the
attribute update-rate guide's third flow: logical-time rules select eligible
TSO work, then the selected embedded receiver path applies a per-attribute
steady-clock gate to best-effort values. A reflection may be omitted when all
values are suppressed. The focused test still permits one later public
`retract` call and rejects a repeat; it does not show that rate suppression
alone invalidates the sender's retraction handle at the recipient boundary.
The time guide explains the TSO queue and grant boundary but does not itself
show this later value filter. A second combined diagram would duplicate the
rate guide's receive-order/TSO comparison.

Added a reciprocal note and direct links from the time guide to the focused
mixed reliable/best-effort TSO test and the rate guide's cross-boundary flow.
The focused embedded test suppresses the callback but still allows one later
public `retract` call; a second call is rejected. The guide now states that
narrow observation instead of claiming suppression makes the message no
longer retractable at the recipient boundary.
The sources and tests cited by that guide establish selected embedded
behavior, not normative cross-service ordering or process parity. A bounded
process scan found public update-rate query coverage, but those tests check
query results rather than per-value callback admission; process delivery
filtering remains unclassified here. No 2010 inference, new chart, build,
test, CI, or Requirements Lab work.

Next handoff: trace 2025 process-endpoint attribute reflection and TSO delivery
to determine whether/how subscription update rates affect delivered attribute
values. Compare process client/server projection and callback paths with
existing process integration tests; distinguish public rate-query coverage
from delivery-admission evidence. Record any unverified path narrowly, and do
not infer non-conformance or parity from a symbol search. No build, test
rerun, CI, or Requirements Lab change.

### 2025 process endpoint update-rate boundary — 2026-10-11

The bounded process trace found that subscription rate metadata is transported
and stored: the public process-subscription case checks an active `High` rate,
then a passive subscription with an empty designator, and the resulting server
registry snapshots. Separate process cases exercise named and per-attribute
numeric rate queries, but not delivery under different rates.

The inspected receive-order and TSO event builders select projected values by
the recipient's received-attribute set; they do not consume the projection's
`maximumUpdateRatesByAttribute` or call `UpdateRateGate`. Client dispatch sends
the event through the process callback bridge to `reflectAttributeValues`.
The process delivery-reduction lane covers event routing, overlapping
subscription deduplication, and disjoint regional scope, not rate throttling.
Updated the rate guide with this source-only process boundary and direct test
links. No process conformance conclusion or 2010 inference was added. No new
chart was warranted: the existing process-endpoint guide already shows the
callback route, while this audit identifies an unresolved profile behavior.
No build, test rerun, CI, or Requirements Lab work.

Next handoff: survey the 2025 late-subscription path for an already registered
object. Compare the existing object-attribute subscription guide with clause
5.8, embedded and process discovery/initial-value sources, and focused tests.
Keep object discovery, initial reflection, and later value eligibility
separate; decide whether the process initial-value edge is merely unverified
or a source-level discrepancy, without making a conformance claim. Reuse the
existing HLA-first diagrams unless the survey finds a real learner gap. Do not
build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 late-subscription callback boundary — 2026-10-11

The existing subscription guide already separates discovery, per-receiver
known-object state, initial values, and later updates. A current-source check
confirmed that the process subscription path emits a discovery event; the
process callback bridge maps discovery and attribute-update events to distinct
public callbacks. The focused MOM case observes discovery and reflected values
but does not assert callback ordering. The ordinary process initial-value
result and the licensed §5.8 rule remain open. Expanded the existing process
comparison diagram to show these callback boundaries without claiming process
parity or a portable ordering rule. No C++ source, test, build, CI, 2010
behavior, or Requirements Lab content changed.

Next handoff: audit the separate 2025 and 2010 DataElement guides as an
edition-boundary review. Keep HLA's typed-value/encoding contract distinct
from Umbra-specific byte-order and serialization choices; verify that any
shared explanation is independently supported by each stream. Reuse existing
codec source and test evidence, leave normative claims open without authorized
standard text, and do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025/2010 DataElement boundary and cross-service gap recheck — 2026-10-11

Compared both DataElement guides with their pinned `RTI/encoding` header
inventories. Both declare `HLAvariantRecord`; only the 2025 inventory includes
`HLAextendableVariantRecord`, `HLAlogicalTime`, and
`HLAlogicalTimeInterval` DataElement headers. The separate 2010
`LogicalTime`/`LogicalTimeInterval`/`LogicalTimeFactory` API remains outside
that encoding-header package. Updated both guide boundaries without changing
either edition's codec description or adding a diagram.

A follow-up survey found the proposed cross-service synthesis already covered
by the cross-cutting guide and its reader paths: it distinguishes request
admission, state, per-recipient eligibility, pending work, protocol completion,
and application observation. The object/interaction guide also connects FOM
member type to a compatible DataElement, encoded octets, and the typed-handle
value map. No umbrella chart was warranted. No Mermaid diagram changed, so
`mmdc` was not needed; no build, test rerun, CI, or Requirements Lab work.

### 2025 Join/save-restore admission boundary — 2026-10-11

The existing HLA-first lifecycle and save/restore guides already separate
membership admission from save/restore participation; a combined diagram
would duplicate them. The pinned Join overloads list `SaveInProgress` and
`RestoreInProgress`, but the current focused interlock test joins before save
and does not exercise an in-flight Join. Source review found no explicit
save/restore-phase predicate in the inspected embedded adapter/registry path
or process service/registry path. The process handler serializes Join requests
with one another, which does not establish arbitration with save/restore.
Keep the public admission rule, interleaving result, and callback effects
unresolved without the relevant normative clauses or a focused runtime case.
This is source/profile mapping, not a conformance finding. No 2010 inference,
diagram, build, test, CI, or Requirements Lab change.

Next handoff: survey the separate 2025 resignation-action explanations across
the lifecycle, ownership-persistence, object/interaction, and time guides.
Distinguish the selected action from cleanup of owned objects, pending
timestamped work, active protocols, and effects already observed by peers.
Decide whether a concise HLA-first decision view fills a real reader gap;
preserve embedded/process limits and do not claim a complete normative action
matrix without the relevant standard text. No build, test rerun, CI, or
Requirements Lab edits.

### 2025 resignation and survivor callback-observation boundary — 2026-10-11

The existing action map and evidence already cover caller intent, embedded
cleanup guards, final-member behavior, connection-loss cleanup, ownership
records, and the time cutoff, so a combined action matrix would add no safe
HLA-level rule. A focused review did find a useful observation boundary: the
embedded adapter resets only the departing member's callback queue, then
submits induced work to surviving members' callback paths. One
`HLA_EVOKED` `DELETE_OBJECTS_THEN_DIVEST` case returns from Resign before the
peer observes callbacks; later Evoke observes assumption before removal. A
separate process `HLA_EVOKED` `NO_ACTION` case observes old joined-federate MOM removal
before fresh-object discovery/reflection after rejoin. These are distinct
profile/scenario observations, not a general action-order rule.

Refined the existing lifecycle sequence to show task submission and
application observation separately; added the process MOM case to its
evidence map. No new diagram, normative matrix, 2010 inference, build, test,
CI, or Requirements Lab change. Local visual verification remains pending:
`mmdc` is not installed and the authorized Scoop search failed at DNS lookup.

Next handoff: survey the 2025 federation-member listing operation across the
embedded and process endpoints. Compare the pinned API, public adapters,
process operation routing, and focused tests to determine whether the guide's
one-time snapshot boundary is evidenced in each profile; do not infer a
membership event feed or process parity from query names. Keep 2010 separate
and do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 member-list, process-callback, and late-subscription follow-up — 2026-10-11

The bounded member-list survey found a one-time snapshot and callback path in
the embedded adapter; no matching process transport operation or client route
was found. The lifecycle guide now labels that profile boundary and leaves the
process invocation outcome untested. A separate process callback review
clarified that a matched response remains inside the client request until
buffered event dispatch succeeds; a throwing HLA_IMMEDIATE callback can
therefore prevent the service method from receiving the response, while the
dequeued event is not retried. The current focused tests do not inject that
callback failure.

The subscription-state guide now separates a subscription that makes an
existing, not-yet-known object discoverable from adding an attribute after the
receiver already knows the object. Embedded focused evidence observes selected
stored values in the first case; the second case's ordinary current-value
behavior remains unresolved. Explicit Request Attribute Value Update and
RTI-owned MOM direct reflection are distinct paths. The process ordinary
late-subscription result and licensed §5.8 interpretation remain open. These
are separate 2025 evidence boundaries; no 2010 behavior is inferred.

No new Mermaid chart was added in these increments. The revised lifecycle
ResignAction sequence still needs local syntax/render and visual review.
`mmdc` is absent from PATH and the available Node runtime cannot resolve the
Mermaid CLI, Mermaid renderer, Puppeteer, or Playwright packages. The registered
Scoop buckets contain no Mermaid manifest, and the user-authorized
`scoop install mmdc` returned no manifest. Approval to use npm/npx was requested
and is pending. Do not claim visual verification or switch package managers
without authorization. No C++ build, test rerun, CI, or Requirements Lab work.

Next handoff: if npm/npx is approved, obtain the Mermaid CLI for a local render
and visual review of the revised 2025 ResignAction sequence. If not, keep that
check explicitly pending and continue the bounded HLA topic survey, selecting
a real reader gap from existing guides and focused evidence. Preserve the
2010/2025 and embedded/process boundaries. Do not build, rerun tests, start CI,
or edit the Requirements Lab.

### 2025 name/handle identity domains — existing guide coverage is sufficient — 2026-10-11

The bounded audit followed the order/transportation guide's reader-review
handoff. The names/handle guide already distinguishes execution-scoped FOM
symbols, hierarchy-scoped members, federate name/handle lifetimes, and
federate-local object-instance visibility. Its public-handle section separates
typed opaque handle classes and their comparison/hash/encode operations from
Umbra's private numeric representation; normalization outputs are a different
execution-scoped coordinate, not a handle. The focused evidence map states
what is exercised in embedded versus process paths and where parity is still
unknown. No additional diagram or prose was justified, and no 2010 behavior is
inferred.

Next handoff: if npm/npx is approved, render and visually review the pending
2025 ResignAction sequence. Otherwise leave that check pending and continue a
bounded survey for a genuinely uncovered HLA reader question. Do not infer
normative behavior from Umbra source, merge edition/profile evidence, build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 ordinary interaction-subscription path — existing guide coverage is sufficient — 2026-10-11

The bounded review compared the object/interaction information guide with the
declaration-relevance and DDM guides. Together they already distinguish the
federation-wide active-subscription relevance gate (excluding the sender), a
receiver's own active or passive match, required regional overlap, closest
matching ancestor/class promotion, and parameter projection. The process
endpoint companion separately documents when the promoted class/parameter
view is materialized versus the embedded callback-time re-evaluation path;
that difference is explicitly source-derived, not promoted to a general HLA
rule. The evidence map also makes clear that process class-promotion and
post-send declaration-change behavior lack a focused public integration case.

No additional ordinary-interaction chart or prose was justified. This is a
2025-only guide-coverage audit; no 2010 behavior or process/embedded equivalence
is inferred. No C++ source, build, test, CI, or Requirements Lab content
changed, and no Mermaid block changed.

Next handoff: survey the timestamped-interaction lifecycle across the 2025
time-management, order/transportation, interaction, and TSO-retraction guides.
Look specifically for a reader gap connecting timestamp/order selection,
recipient eligibility, queued delivery, and producer retraction; add no
cross-profile or normative rule without direct evidence. If npm/npx is
approved, the separate pending 2025 ResignAction render may be completed; if
not, leave visual review pending. Keep 2010 separate and do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 timestamped-interaction projection and grant boundary — 2026-10-11

The bounded cross-guide survey compared the time-management,
order/transportation, object/interaction, DDM, TSO-retraction, and process
endpoint guides. Their existing diagrams already explain order selection,
recipient/region eligibility, TSO grant timing, callback observation, and
per-recipient retraction. A second end-to-end algorithm would duplicate those
models. A narrower reader gap remained: the ordinary process interaction
section explains send-time projection, but did not warn readers not to carry
that snapshot boundary into timestamped interaction delivery.

Added a profile-labeled flow and evidence entry to the object/interaction
guide. In the inspected 2025 source, the embedded TSO callback closure resolves
the recipient projection during callback dispatch, while the process service
resolves it before constructing and sending the event; the process callback
bridge then uses the event's stored projection. The time guide now labels its
five-step dispatcher sequence as the embedded path, links the regional DDM
replacement example as a separate recipient gate, and cites a process
`HLA_EVOKED` test body that asserts two same-time interactions before one
grant. The embedded regional replacement case exercises one subscription
change; no focused process case mutating subscription state between TSO send
and event materialization was identified. These are bounded source/test
observations, not normative projection semantics, all-service guarantees, or
2010/process-embedded equivalence.

The new Mermaid chart was not locally rendered: the user-authorized
`scoop install mmdc` again found no manifest, and npm/npx remains unauthorized.
Its visual review is pending. This turn inspected existing test bodies but
did not rerun tests or build; no C++ source, CI, or Requirements Lab content
changed.

Next handoff: audit whether timestamped attribute updates have analogous or
different projection-freeze boundaries by comparing their embedded/process
delivery paths with the attribute-subscription, DDM, and time guides. Keep
attribute reflection distinct from interaction delivery; add a synthesis
only for a demonstrated reader gap. Preserve the 2010 boundary, reuse current
test results, and do not build, rerun tests, start CI, or edit the Requirements
Lab. If an approved renderer becomes available, review the new chart and the
pending ResignAction sequence separately.

### 2025 timestamped-attribute projection and delivery gate — 2026-10-11

The cross-guide audit found a focused timing gap rather than a need to repeat
the whole attribute or TSO lifecycle. The existing DDM guide already shows
that an embedded regional timestamped update can be suppressed when the
receiver changes its selector before the callback boundary. The source paths
also expose a process-profile distinction that was not synthesized alongside
the recipient's time gate: embedded delivery resolves the current projection
inside callback dispatch, while process delivery resolves it before creating
and sending the event. A later change cannot rewrite that materialized process
event, but no focused process test was found that mutates the subscription or
region in this interval.

Added one profile-labeled flow and evidence-map row to the 2025 time guide. It
keeps the logical-time eligibility gate separate from current receiver
projection, retained source-region scope, embedded best-effort rate
admission, and the process event-before-grant boundary. A public process
endpoint `RTIambassador` test asserts the attribute reflection before the
matching grant; a separate private transport-service test confirms the event
frame order. Neither test mutates subscription state between acceptance and
event creation. The embedded selector-mutation evidence links to the existing
regional DDM guide. These selected cases and source observations establish no
normative projection-freeze rule or 2010/2025, embedded/process, or
interaction/attribute parity. The process update-rate caveat links to its
existing guide rather than duplicating that analysis.

The new Mermaid chart remains visually unreviewed: the authorized Scoop
attempt could not find an `mmdc` manifest, and no alternate package manager was
used. No C++ source, build, tests, CI, or Requirements Lab content changed.
The working-tree census is 231 Mermaid blocks across 32 of 41 design Markdown
files (135 flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides,
215 in 29 2025 guides, and 2 in the index); this is a fence/type count only.

Next handoff: audit same-recipient ordering between timed federation-save
notifications and TSO payloads at a grant boundary. Prefer a portable timing
model, use Umbra source and already-existing focused tests only as bounded
evidence, and add a diagram only if the current guides leave a real reader
gap. Preserve the separate 2010 and 2025 streams; do not build, rerun tests,
start CI, or edit the Requirements Lab. Keep the timestamped-interaction,
timestamped-attribute, and ResignAction visual reviews pending until a
renderer is available.

### 2025 object existence and federate-local knowledge — 2026-10-11

The portfolio survey selected a useful reader gap beyond timing: the detailed
object guide explains registration/discovery, federation deletion, and local
forgetting in separate service flows, while the cross-cutting guide's short
orientation was prose-only. Added a relationship map that separates execution
existence from each federate's knowledge, then points to later eligibility,
local forgetting, and the distinct federation-wide deletion/removal path.
The map deliberately does not merge those service timelines or their callback
and profile-specific gates. Its source/evidence map remains in the 2025
[object/interaction guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#evidence-map-implementation-versus-exercised-scenarios).

The cited discovery, local-delete/rediscovery, process local-delete, and
removal-payload tests are separate scenarios, not an end-to-end proof; in
particular, the process local-delete case covers success only. This remains a
2025-only synthesis and does not establish process/embedded parity. Its new
chart is not visually reviewed because the authorized Scoop install found no
mmdc manifest; no alternate package manager was used. No C++ build, test
rerun, CI, or Requirements Lab edit was made.

Next handoff: audit the existing 2025 synchronization/save-restore
participant-set comparison against the current source and focused-test
evidence. Keep the two protocols distinct, recheck late-join and partial-
achievement limits, and leave the active Join/save-restore normative question
unresolved absent the relevant standard text. Add no duplicate diagram if the
current map already explains the reader's question.

### 2025 ownership callback queue versus restored work — 2026-10-11

The bounded review found that the core ownership guide and its persistence
companion already explain request-versus-transfer, selected ledger recovery,
recipient validation, and resignation cleanup. A narrower process-profile gap
remained: “pending ownership callback” could mean either an already-pushed
callback waiting in a live HLA_EVOKED queue or value-only ownership work that
the process restore service reconstructs and enqueues after federation-
restore lifecycle notifications.

Added a compact comparison flow to the ownership-persistence companion and a
reader-path note to the federation save/restore guide. The first process test
shows pre-save discovery and ownership-assumption frames left uninvoked through
service-level save/restore, then observed by later Evoke calls before that
scenario's queued save/restore status callbacks. A separate configured
process-endpoint test plus the restore-service source shows reconstructed
assumption work scheduled after federation-restored notifications. The tests
are separate and do not establish process restart, process/embedded parity, or
a normative HLA callback-order rule.

The chart is unrendered: mmdc is not available in this environment and the
previous authorized Scoop attempt found no manifest; no alternate package
manager was used. Static census is now 239 Mermaid blocks across 34 of 41
design Markdown files (139 flowcharts, 77 sequences, 23 state diagrams; 14 in
two 2010 files, 223 in 31 2025 files, and 2 in the index). This is a fence/type
count, not syntax, rendering, visual, or GitHub verification. No C++ source,
build, test rerun, CI, or Requirements Lab content changed.

Next handoff: audit the existing 2025 synchronization/save-restore
participant-set comparison against source and focused tests. Do not merge its
protocol rules; preserve the late-join/test gaps and the unresolved Join
normative question. Keep 2010 separate and make no duplicate diagram if the
existing map already answers the reader's question.

### 2025 synchronization versus save/restore participant sets rechecked — 2026-10-11

The existing cross-cutting map remains the right-sized explanation: it keeps
the expandable synchronization barrier separate from restore roster admission
and from save/restore failure on resignation. Current source still supports
those boundaries. Omitted-set synchronization registration is distinguished
from an explicitly supplied set, and the late-Join hook adds a member only to
an expandable pending point. Retaining a prior achievement when a late Join
expands the barrier remains a source-derived trace; the focused late-Join case
joins before any participant reports achievement. Save participant statuses
are formed from members present when save begins; restore compares saved and
current participant identities before beginning the restore.

The focused interlock case joins both participants before Save and does not
attempt Join during an active save or restore. A bounded review of the related
2025 test cases found no focused active-Join interleaving. The pinned API lists
`SaveInProgress` and `RestoreInProgress` on both Join overloads, but the
inspected Join paths expose no matching active-operation gate; neither source
nor those tests establish acceptance, rejection, or cohort effects. Keep this
as an unresolved API/source question until the exact §4.11 conditions are
available. No new chart was warranted, no Mermaid block changed, and no tests,
build, CI, or Requirements Lab work was performed.

Next handoff: survey the existing 2025 DDM/region guide for a genuine learner
gap in how region extents, subscriptions, region updates, and delivery
eligibility relate. Keep the HLA model primary, use source and existing tests
only as bounded evidence, and add a diagram only if it clarifies that gap.
Keep 2010 separate.

### 2025 DDM concepts and clause navigation — 2026-10-11

Reviewed the current [DDM and regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
as a reader-oriented HLA explanation. Its current flows already connect
dimension availability, region shape and committed bounds, object source
regions, receiver subscriptions, discovery/relevance, interaction selection,
and the independent time/callback gates. No duplicate diagram was warranted.

Added a service-clause navigation table linking the HLA questions to the
pinned 2025 API headings for region lifecycle (§§9.2–9.4), object and
interaction regional services (§§9.5–9.13), and dimension/range queries
(§§10.21–10.28). The table is an index, not a paraphrase of normative rules.
The full exception matrix and combinations of class inheritance with regional
declarations remain open. The guide keeps source/test observations bounded and
does not imply 2010 or process/embedded parity.

Current static census: 233 Mermaid fences across 32 of 41 design Markdown
files (137 flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides,
217 in 29 2025 guides, and 2 in the index). The DDM guide's 14 diagrams were
not parsed or rendered in this pass: `mmdc` was unavailable and the authorized
Scoop install found no `mermaid-cli` manifest. This is not syntax, rendering,
visual, or GitHub verification. No C++ source, build, test rerun, CI, or
Requirements Lab content changed.

Next handoff: audit the remaining 2025 object/interaction class-inheritance
and regional-declaration combinations across the DDM, object/interaction, and
FOM guides. Identify the portable HLA distinctions and exact clause pointers,
compare existing source/test evidence, and add a diagram only if a real reader
gap remains. Preserve the separate 2010 stream.

### 2025 class ancestry feeding a regional object path — 2026-10-11

The bounded audit found a concrete bridge between class-dimension ancestry
and regional object services. The DDM guide now shows the Restaurant FOM
`Food.Drink` → `Soda` → `Light` chain, the three-dimension availability
result for `Light`, and one embedded regional registration/subscription case
where disjointing each source-range dimension suppresses that update's
reflection. Cross-links connect the case to the object/interaction and FOM
composition guides.

Keep its evidence scope narrow: both publisher and subscriber use the same
`Light` class in this case. It demonstrates ancestor-associated dimensions in
that descendant class's available set and regional path; it does not prove a
regional declaration made on a superclass is itself inherited by subclasses.
The analogous interaction-class parent-chain behavior is visible in current
source, but the reviewed query and invalid-context tests do not combine an
ancestor-only interaction dimension with regional send/subscription. Do not
infer process-profile, 2010, or normative parity.

Current static census: 234 Mermaid fences across 32 of 41 design Markdown
files (138 flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides,
218 in 29 2025 guides, and 2 in the index). All local links and line anchors
in the three reviewed DDM, object/interaction, and FOM guides resolve. The
new chart remains unparsed/unrendered: `mmdc` is unavailable and Scoop has no
`mermaid-cli` manifest. No C++ source, build, test rerun, CI, or Requirements
Lab content changed.

Next handoff: survey the 2025 interaction-class ancestor-dimension path in
regional subscription and send, then separate that from whether a regional
declaration on an ancestor class applies to descendant classes. Use the
§10.22, §9.10, and §9.12 API entry points as navigation, compare existing
focused source/test evidence, and keep any unsupported normative consequence
explicitly open. Preserve 2010 as a separate stream.

### 2025 interaction-class ancestry across regional DDM — 2026-10-11

The DDM guide now distinguishes three embedded-source operations that use
class ancestry for different reasons: available-dimension accumulation for a
selected class, regional declarations stored under the exact class handle,
and recipient lookup that begins at a sent class and checks its parents.
For a non-empty regional send, an ancestor-keyed regional entry can be
considered for a descendant send by the current candidate walk when the
regional match is eligible. That is not a child declaration copied at
subscription time, and no normative inheritance rule is asserted. The map
also keeps the federation-wide active-relevance gate separate from each
receiver's class/overlap match and shows the empty-set no-op branches.

Evidence is bounded: source links point to the class-dimension helper,
regional-subscription validation and exact-key storage, send validation, and
the receive-order candidate walk. The Restaurant query's `ServerId` is
declared on `MainCourseServed` itself; the regional case uses that same class
for subscription and send. The reviewed tests do not cover ancestor-only
interaction dimensions through regional services or an ancestor subscription
with a descendant send. API §§10.22, 9.10, and 9.12 are navigation pointers;
the licensed normative text remains necessary for an HLA conformance claim.
The object/interaction and FOM guides cross-link the distinction. No 2010 or
process-profile parity is inferred.

Current static census: 235 Mermaid fences across 32 of 41 design Markdown
files (139 flowcharts, 75 sequences, 21 state diagrams; 14 in two 2010 guides,
219 in 29 2025 guides, and 2 in the index). Local file and line links in the
three reviewed guides resolve. The new chart is not parsed or rendered;
`mmdc` remains unavailable and the authorized Scoop attempt found no
`mermaid-cli` manifest. No C++ source, build, test rerun, CI, or Requirements
Lab content changed.

Next handoff: resume the known 2025 late-subscription initial-reflection
documentation gap. Compare the HLA mental model and clause entry point with
existing embedded/process source and focused tests; decide whether a
reader-centered flow can clarify object discovery versus initial attribute
values without overstating implementation evidence. Keep 2010 separate.

### 2025 late-subscription initial reflection: reader-gap audit — 2026-10-11

The existing object-attribute subscription-state guide already explains the
reader-facing distinction between discovering an object and receiving its
current attribute values, including an HLA-first flow, an embedded V1/V2
scenario, and a separate process-profile comparison. Rechecking the current
tests found no focused ordinary process late-subscription-value case: the
public process discovery path subscribes before registration, the regional
advisory path also subscribes before registration, the private advisory case
tests owner notification after the object is known, and the process initial
value test is specific to RTI-owned MOM objects. The guide correctly leaves
ordinary process initial values unresolved pending licensed §5.8 text and a
focused process observation; no duplicate chart was added.

The static diagram census remains 235 Mermaid fences across 32 of 41 design
Markdown files (139 flowcharts, 75 sequences, 21 state diagrams; 14 in two
2010 guides, 219 in 29 2025 guides, and 2 in the index). No chart was added
or rendered. A network-enabled Scoop search returned no Mermaid CLI package;
`mmdc`, npm, and npx are unavailable. No build, test rerun, CI, or Requirements
Lab work was done.

Next handoff: audit the 2025 Join-during-save/restore admission question in
the save/restore guide. Separate the pinned API exception list, embedded and
process source paths, interlock-test coverage, and licensed §4.11 authority;
add a diagram only if it clarifies participant membership without stating an
unverified outcome. Preserve the 2010 boundary.

### 2025 Join during save/restore: participant-set audit — 2026-10-11

Confirmed that the existing save/restore evidence table accurately separates
the two Join exception annotations from what the available package can prove.
The bounded embedded/process paths show no explicit save/restore admission
predicate, while the focused interlock test has both members join before save;
neither establishes in-flight admission or rejection. The broader lifecycle
guide already has the useful cross-protocol flow for federation membership,
expandable synchronization participants, and unresolved save/restore
participants. The detailed guide now links to that overview instead of adding
a duplicate chart. Licensed §4.11 text remains necessary for a normative
outcome; keep synchronization, save/restore, process, and 2010 boundaries
separate.

Static diagram census remains 235 Mermaid fences. No diagram was added,
parsed, or rendered; the full-set renderer check is still open. No build, test
rerun, CI, or Requirements Lab work was done.

Next handoff: review the existing embedded 2025 resignation-during-save flow
for clarity about survivor notifications and recovery, without conflating
HLA-level membership effects with callback machinery or process/2010 behavior.

### 2025 resignation during active save: callback-model audit — 2026-10-11

The embedded `HLA_EVOKED` sequence correctly shows the participant
consequence: after the callback returns, resignation fails the current save
for the survivors, omits a stale failure for the leaver, and leaves survivors
able to complete a later save. Its callback guard and queued delivery are
scoped to that focused schedule. The `HLA_IMMEDIATE` companion confirms the
same failure reason and later-save recovery for one survivor, but not the
multi-survivor or in-callback guard details. Added that distinction to the
guide's evidence map; no second diagram was needed. The restore counterpart
remains separately sourced and tested; no process or 2010 parity is claimed.

No build, test rerun, CI, or Requirements Lab work was done. Whole-set Mermaid
rendering remains open.

Next handoff: audit 2025 callback exceptions at the boundary between callback
execution, HLA_EVOKED/HLA_IMMEDIATE delivery, and service-level reporting or
accepted-state effects. Preserve profile and edition boundaries; do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 callback exceptions: accepted-send boundary — 2026-10-11

The callback/service-ordering guide now includes a source-labeled embedded
`Send Interaction` sequence. It connects the HLA_IMMEDIATE/HLA_EVOKED timing
distinction to the service's already-accepted report/counter effects: an inline
callback exception can escape the Send Interaction call after acceptance,
whereas an HLA_EVOKED exception arrives later from Evoke. A pre-existing
immediate drainer may own the exception instead. The generic dispatcher and
call-stack diagrams remain complementary, so the new sequence is service-
specific rather than another generic callback chart.

No focused 2025 throwing-callback test was found; the example remains
source-derived, with process event delivery and 2010 callback behavior kept
separate. The current fence census is 242 Mermaid blocks across 34 of 41
design Markdown files (141 flowcharts, 78 sequences, 23 state diagrams; 14 in
two 2010 guides, 226 in 31 2025-named guides, and 2 in the index). Whole-set
syntax/render validation remains open. No build, test rerun, CI, or Requirements
Lab work was done.

Next handoff: establish a supported Mermaid parse/render workflow for the
complete diagram inventory, then syntax-check all blocks and visually inspect
representative dense charts. Do not claim GitHub rendering from local output;
keep 2010 and 2025 separate, and avoid builds, tests, CI, and Requirements Lab
changes.

### Whole-set Mermaid local render audit — 2026-10-11

Mermaid CLI 12.0.0 rendered all 242 diagrams across 34 design Markdown files.
The audit extracted both backtick and tilde fences explicitly because the
CLI's Markdown mode skips tilde fences. Two semicolon-related syntax errors
were fixed in 2025 sequence labels; all diagrams in both affected guides were
rerendered, and the callback-ordering guide's 11 diagrams also render after
the new exception flow was reflowed. A representative 2025 callback sequence
and a separate 2010 DataElement flow were visually inspected. This is local
render evidence only: full-set visual review and GitHub rendering remain open.
No C++ build, test rerun, CI, or Requirements Lab work was performed.

Next handoff: conduct a bounded HLA-first review of the 2025 object-instance
naming flow. Distinguish reservation, its later outcome callback, use at
registration, and peer discovery/local knowledge; check existing API-clause
links and edit only if a genuine reader gap remains. Preserve embedded/process
and 2010 boundaries, keep Umbra behavior bounded, and do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 object-instance naming: portable API lifecycle — 2026-10-11

The bounded review found a reader gap: the guide's first diagram moved
directly from its summary into Umbra's registry/queue trace. Added a compact
API-level lifecycle first, separating reservation admission/outcome,
successful named registration and the owner's handle, and the distinct peer
discovery callback path. It deliberately leaves callback timing relative to
service return unspecified and leaves registration eligibility/visibility
details to the focused sections. The existing detailed diagrams remain
implementation-profile evidence; 2010 behavior is untouched.

The 2025 API-header landmarks were checked for the void reservation service,
separate outcome callbacks, named-registration handle return, and
`discoverObjectInstance` callback. A local re-render was attempted, but the
installed Mermaid CLI could not launch Chromium in this run, and the direct
parser path also failed inside its DOMPurify setup. The new chart is therefore
not render-verified; do not claim GitHub rendering. No C++ source, build, test
rerun, CI, or Requirements Lab work was performed.

Next handoff: resume the known 2025 ordinary-object late-subscription initial-
reflection gap. Compare the HLA mental model and pinned API clause entry point
with existing embedded/process evidence; determine whether a reader-centered
flow would clarify subscription commit, object discovery, and initial value
reflection. Keep the 2010 RTI stream separate and avoid builds, test reruns,
CI, and Requirements Lab edits.

### 2025 late subscription versus delayed update delivery — 2026-10-11

The bounded review found that the subscription-state guide already has an
HLA-oriented interest-change flow, an embedded discovery/initial-reflection
trace, and a separate embedded/process comparison. A new state-machine chart
would repeat that coverage. The actual reader risk was a terminology collision:
an object can be late relative to the receiver's discovery, while an update can
be late relative to send-time eligibility because DSE retains it for later
evaluation. Added a short comparison table separating those clocks and
clarifying that a discovery-time reflection may contain a value previously
set by an update without replaying the publisher's update invocation. The
already-known-object/new-attribute reflection question remains explicitly
unresolved; DSE does not answer it.

The bounded source/test check still supports only an embedded ordinary
initial-reflection path after discovery returns, and a process initial-value
branch for RTI-owned MOM objects. No ordinary process result is established.
Pinned API §5.8 is a navigation point, not a substitute for the unavailable
licensed clause text. No new Mermaid block was added. The current diagram count
remains 243, with the prior HLA-level reservation chart still awaiting local
render verification. No C++ source, build, test rerun, CI, or Requirements Lab
work was performed.

Next handoff: audit the existing Auto Provide and explicit Request Attribute
Value Update comparison against the new initial-state-versus-pending-send
distinction. Check that a reader can follow discovery-time reflection,
provider solicitation, explicit requests, and DSE-delayed delivery without
confusing which federate receives each callback. Add only a concise cross-link
or map if a real gap remains; do not duplicate the existing state machines.
Preserve embedded/process and 2010 boundaries and avoid builds, tests, CI, and
Requirements Lab edits.

### 2025 value-request paths: cross-guide comparison — 2026-10-11

The Auto Provide guide already compares discovery-triggered provider
solicitation, explicit Request Attribute Value Update, initial reflection,
scope changes, and relevance advisories by trigger, recipient, and payload.
The request guide separately provides the HLA-facing solicitation/update/
reflection sequence and its tag and eligibility caveats; the DSE guide
explains delivery-time reevaluation. The substantive comparison was already
clear, so I added only a cross-link from the initial-reflection row to the
subscription-state guide's initial-state-versus-DSE table. No duplicate path
diagram or claim that a requester is itself entitled to receive values was
added.

The four guides keep HLA-level flow distinct from embedded implementation
details and process evidence gaps. No 2010 flow was reused. This was a
documentation-only link change: no new Mermaid block, C++ source, build, test
rerun, CI, or Requirements Lab edit. The inventory remains 243 diagrams; the
previously added reservation overview is still pending render verification.

Next handoff: review the 2025 Request Attribute Value Update guide's open
requester-versus-receiver question. Compare the pinned §6.21 API surface,
object/class/regional implementation paths, and focused tests. Without an
authorized copy of the exact clause, keep any normative entitlement claim
open. Add a diagram only if the bounded survey finds a real reader gap; keep
embedded/process and 2010 boundaries explicit and avoid builds, tests, CI, and
Requirements Lab edits.

### 2025 Request Attribute Value Update: requester and receiver boundary — 2026-10-11

The bounded review found that the request guide already explains the key
implementation distinction: a request solicits provider callbacks, while a
provider's later update uses ordinary receiver planning. The pinned 2025 API
shows the request overloads and the separate `provideAttributeValueUpdate`
callback surface. The embedded baseline first subscribes the requester and
shows provider solicitation (including exclusion of requester-owned
attributes); the process class-request case seeds subscription state directly
in the registry fixture and observes provider callbacks, not a response
reflection. Neither test establishes behavior for an unsubscribed requester.
Regional request filtering also remains distinct from later response DDM.

The guide sufficiently warns that requesting alone does not make the
requester a receiver in the selected implementation. The exact normative
entitlement rule remains open without authorized §6.21 clause text. No extra
diagram or guide edit was justified. Mermaid CLI 12.0.0 is available in the
isolated visualization workspace, but Chromium failed to launch with both its
existing profile and a fresh profile; the reservation overview remains
unrendered. No C++ source, build, test, CI, or Requirements Lab work.

Next handoff: review the 2025 ordinary known-object/new-attribute subscription
case. Compare the HLA/API entry point with embedded current-value reflection,
focused tests, and the process path; keep any normative outcome open without
the licensed clause text. Add a diagram only if it clarifies a real reader
gap. Preserve 2010 and 2025 boundaries, and avoid builds, test reruns, CI, and
Requirements Lab edits.

### 2025 known-object subscription and current-value reflection — 2026-10-11

The bounded review found the subscription guide already gives the reader the
needed three-way distinction: an unknown existing object becoming discoverable
with stored values, an already-known object receiving a new attribute
subscription, and a separate explicit Request Attribute Value Update. Its
state table and receiver-state flow explicitly leave ordinary current-value
reflection for the already-known-object case unresolved rather than implying
that a scope advisory carries a value.

The pinned API anchors ordinary subscription at §5.8 and value reflection at
§6.11. In the embedded 2025 path, the discovery planner skips instances already
known to that receiver, and the current-value planner is scoped to the
post-Discover callback path. The embedded focused tests exercise an object
becoming newly discoverable and its stored value, not adding an attribute to
an already-known object. The process subscription handler also plans
discoveries, while the process initial-value event inspected here is
RTI-owned-MOM-only; the ordinary-process outcome is not established. The
private process test for a late second-attribute subscription observes an
owner relevance advisory, not a receiver value reflection. These are bounded
source/test findings, not a normative conclusion on §5.8 or process/embedded
parity.

No extra chart or guide edit was justified: the existing comparison and
profile-separated flows already expose this exact evidence gap. The normative
question remains open pending licensed §5.8 text and a focused ordinary
process observation. Mermaid CLI 12.0.0 is available locally, but Chrome and
Edge both failed to launch from isolated profiles; the reservation overview
remains unrendered. No C++ source, build, test, CI, or Requirements Lab work.

Next handoff: conduct a bounded reader-first review of the 2025 process
callback-exception path. Compare the public callback exception surface with
the existing process callback and service-ordering guides, and inspect only
the callback bridge/coordinator source and existing focused tests. Keep HLA
callback consequences distinct from Umbra's private completion ACK and do not
infer retry or normative failure semantics from implementation state. Add or
revise a flow only if the cross-guide survey finds a genuine learner gap;
preserve embedded/process and 2010 boundaries, and do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 process callback exception and private TSO completion — 2026-10-11

The bounded cross-guide review found that the two existing guides already
divide this explanation at the right boundary. The callback/service-ordering
guide covers the public `FederateInternalError` surface, callback model, and
which public call may observe an escaping exception. The process-endpoint guide
then traces the private timestamped-delivery consequence. Both guides link to
the other; a synthesis diagram would repeat their existing public-call and
per-recipient process-ACK flows.

The inspected process callback bridge invokes the public callback before its
completion hook. If user callback code throws, control does not reach that
hook; the dispatcher has already extracted the task, and it is not retried.
The selected TSO coordinator entry therefore remains in transit until an
acknowledgment or recipient cleanup. Source shows that in-transit timestamps
participate in time-bound calculations and can block a timed-save readiness
check at or before its save time. Those are Umbra process-profile consequences,
not HLA callback state names or normative error/retry rules.

The focused process fan-out tests exercise normal callback completion and
per-recipient acknowledgment; the pre-callback retraction test exercises
suppression before application callback entry. The inspected tests do not
throw from a public callback or verify post-throw recovery/cleanup. Keep that
as an implementation/test evidence gap, not a missing explanation or a
conformance finding. No guide or diagram edit, C++ source change, build, test,
CI, or Requirements Lab work.

Next handoff: compare the 2025 HLA-facing participant-set explanations for
Join during save/restore and late Join during an expandable synchronization
point. Check the lifecycle map, save/restore Join evidence table, synchronization
flow, and their existing cross-links. Add a comparison only if it prevents a
real reader from carrying synchronization late-join behavior into the
save/restore protocol. Treat API exception annotations as navigation, keep
§4.11 unresolved without licensed clause text, preserve 2010 boundaries, and
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Join and active-protocol participant sets — 2026-10-11

The bounded cross-guide review found the current presentation already prevents
the key category error. The lifecycle guide has a two-branch HLA-facing map:
embedded synchronization-point expansion is labeled as source-derived, while
the save/restore branch stops at the pinned Join exception annotations and an
explicit unresolved participant-set question. The save/restore guide links to
both the synchronization flow and that lifecycle map, then presents separate
API, embedded, process, and test evidence. The synchronization guide separately
explains its expandable-point gate, explicit-set exclusion, achievement
fan-in, and evidence limits.

The synchronization late-join behavior is an Umbra 2025 embedded observation,
not a portable rule to transfer to save/restore. The save/restore Join contract
remains unresolved: both Join overloads list `SaveInProgress` and
`RestoreInProgress`, but the licensed §4.11 text is unavailable and the
focused interlock test does not Join during an active protocol. The existing
map and cross-links are sufficient; another comparison chart would duplicate
them. No guide or diagram edit, source change, build, test, CI, or Requirements
Lab work.

Next handoff: survey the 2025 seam between additional FOM modules admitted
during Join and name-to-handle resolution. Compare the FOM composition and
name/handle guides, pinned API entry points, and existing source/test evidence;
clarify only what the added module can change for joined federates and how
profile boundaries affect that result. Add a cross-link or flow only if a
reader gap remains. Preserve the separate 2010 stream, avoid unsupported
handle-stability claims, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 additional-FOM Join and name lookup visibility — 2026-10-11

The FOM-composition guide now links the atomic replacement/handle-directory
transition to the name-and-handle guide's existing lookup sequence. Focused
embedded tests show an already-joined member resolving added object and
interaction classes and comparing its earlier typed handle equal within the
same execution. The configured-process additional-FOM test checks the joining
client's object-class lookup, not visibility from an already-connected member.
No cross-execution numeric-handle stability or backend-parity claim is made;
the existing name/handle sequence was sufficient, so no duplicate diagram was
added. No source, build, test, CI, or Requirements Lab work.

Next handoff: review the 2025 ordinary process initial-value path at the
subscription/discovery boundary. Compare the subscription, object-management,
and delivery guides, then inspect only the pinned API, targeted implementation
paths, and existing focused tests. Distinguish a missing explanation from a
missing test; add a reader-facing flow or cross-link only where the current
guides leave a real gap. Preserve embedded/process and 2010/2025 boundaries;
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 process late-subscription initial-value boundary — 2026-10-11

The subscription-state guide already has the needed HLA-facing comparison and
an embedded/process sequence, so a second diagram would be redundant. The
process subscription handler plans discovery for ordinary and MOM instances
and routes both through the same enqueue function. In that function, a
separate initial attribute-update event is constructed only for an
RTI-owned MOM object; the inspected ordinary-object discovery route enqueues
no initial-value event. The public process discovery test subscribes before
registration and checks discovery under both callback models, while its
ambassador does not observe reflections. The MOM test proves only the special
MOM values path and does not assert callback order. No focused process test
establishes ordinary late-subscription value delivery or its absence, and
licensed §5.8 text remains unreviewed. This is a source/test/normative
question, not a conformance finding or profile-parity conclusion.

The existing local Mermaid CLI could not launch its browser for rendering in
this workspace, so visual render verification remains open. No package
installation was needed.

Next handoff: review the 2025 already-known-object/newly-subscribed-attribute
boundary. Determine whether a subscription that adds an attribute without a
new discovery has a current-state snapshot effect, keeping that distinct from
explicit Request Attribute Value Update and DSE delivery of a previously sent
update. Compare the subscription-state guide, targeted embedded/process
source and tests, and authorized §5.8 text if available. Preserve the separate
2010 stream; do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 already-known object and newly subscribed attribute — 2026-10-11

The current subscription-state diagram already separates a receiver that is
newly discovering an object from one that already knows it, so no second flow
was added. The source trace now makes the implementation boundary explicit:
subscription mutation returns scope/relevance effects; discovery replanning
skips known instances; the embedded initial-value planner is called only from
the post-Discover callback path; and the process discovery route creates a
separate initial-value event only for RTI-owned MOM objects. Thus the surveyed
ordinary source paths show no subscription-time current-value snapshot for an
already-known object, without turning that observation into an HLA rule.

Focused evidence remains incomplete. The embedded active/passive test changes
per-attribute modes after discovery but only observes the following publisher
update, and the process post-discovery test checks owner relevance advice but
not subscriber reflection. The [official IEEE product listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers access via subscription; licensed §5.8 clause text was not available to
this review or in the local checkout.
The exact standard obligation and ordinary current-value outcome remain open;
this is not a conformance finding. No source, build, test, CI, or Requirements
Lab work; 2010 remains separate.

Next handoff: survey 2025 explicit Request Attribute Value Update as a
provider-solicitation flow. Compare its API and object-value guide with the
Auto Provide distinction, then inspect targeted source/tests to establish
which provider is asked, what a successful request does not deliver by itself,
and whether requester subscription/known-object state affects the route. Keep
HLA-required behavior separate from embedded/process evidence, preserve 2010
boundaries, and do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 explicit Request Attribute Value Update and provider solicitation — 2026-10-11

The existing request guide already starts with an HLA-facing causal model:
request, optional provider solicitation, a later provider-supplied update, and
separate receiver reflection. It distinguishes object-instance, class, and
regional request scope in the selected embedded profile, notes the requester-
known-object precondition for the object form, and keeps requester response
entitlement normatively open without authorized §6.21 text. Auto Provide and
the RTI-owned MOM direct-reflection path remain separate. The focused process
class case is not treated as profile parity. No duplicate diagram or prose
change was needed.

Next handoff: review the 2025 ownership guide's service-acceptance, per-
attribute outcome, completed-transfer, and later value-delivery boundaries.
Compare its HLA-facing map with the pinned API and existing focused tests; add
no duplicate flow if the current guides already make the distinction clear.
Keep normative and profile claims bounded, preserve 2010, and do not build,
rerun tests, start CI, or edit the Requirements Lab.

### 2025 ownership and later attribute-value delivery — 2026-10-11

The ownership guide separates service return, pending work, per-attribute
outcomes, transfer, cancellation, and later delivery. Its If-Wanted path also
shows a service-result transfer before callbacks, so no one callback-time rule
is implied. The object-information guide already diagrams the later value-
bearing update and receiver gates. The ownership glossary now links to that
path; a third synthesis would duplicate the existing flows. No conformance or
profile-parity claim was added.

Next handoff: compare the 2025 time, ownership, object-information, and TSO-
retraction guides for an accepted timestamped attribute update that remains
queued across a later ownership change. Verify only the cited API/source/test
edges, and add navigation or a flow only if a genuine reader gap remains.
Keep 2010 separate; do not build, rerun tests, start CI, or edit the
Requirements Lab.

2026-10-12: HLA-first Attribute Scope review separated the embedded stale-In
result by callback model: HLA_EVOKED queues In until after a region move, so
the stale In is suppressed; HLA_IMMEDIATE enters In before that later move and
then reports the subsequent Out. The process test covers public unsubscribe
and owner-side In/Out under both models, but does not create a stale-queue
race. The process stale filter remains source-only evidence. Also corrected
the switch boundary: the embedded test's disabled-switch segment is the
self-final-unsubscribe case, already exempt from Out; the process test keeps
the scope switch enabled. Source confirms rechecks, but switch-off suppression
for an otherwise-reportable transition lacks an isolating test. Clarified the
guide and backlog without changing any diagram. Source/test code inspected,
not executed; no render, build, tests, CI, 2010, or Requirements Lab work.

Next: review the 2025 Auto Provide guide as an HLA-first flow. Keep
discovery-triggered RTI solicitation distinct from explicit requests, provider
callback distinct from a later value response, and switch/scope/ownership
checks distinct from DDM and callback observation. Reuse existing diagrams
unless a concrete reader gap remains. Keep 2010 separate; do not build, rerun
tests, start CI, or edit the Requirements Lab.

### 2025 accepted TSO attribute update across a later ownership change — 2026-10-11

The TSO-retraction guide already contains a focused flow for an update accepted
before later divestiture or source resignation. Two embedded `HLA_EVOKED`
tests assert that the queued recipient can receive the original value, tag,
time, producer identity, and retraction designator after the ownership event;
the registry keeps producer identity and per-recipient delivery state. This is
selected implementation evidence only: it establishes neither process parity
nor a portable rule for every receiver-state change. The ownership and time-
management guides now link to that comparison. No new diagram was needed.

Next handoff: survey 2025 object-class ancestry across publication,
subscription/discovery, ownership, and attribute projection. Compare the
object-information, FOM-composition, and name/handle guides; use pinned API and
focused tests only to distinguish portable class relationships from selected
Umbra projection behavior. Add a synthesis only if a reader gap remains.
Preserve the 2010 boundary; do not build, rerun tests, start CI, or edit the
Requirements Lab.

### 2025 class ancestry: lookup and runtime projection are different questions — 2026-10-11

The existing object-information guide already has a cross-path ancestry map:
it keeps object-class and interaction-class chains separate and contrasts
discovery's known-class choice, attribute projection, ordinary interaction
promotion/parameter projection, and directed target/selector lookup. Focused
2025 cases support selected branches: discovery reports the subscribed class;
an attribute-projection case shows a superclass-known receiver omitting a
child-only attribute; an internal registry test demonstrates ordinary
interaction promotion and parameter projection. That internal test is not
public conformance evidence. Directed ancestor declaration and selector
lookup are visible in the shared source walk, but the bounded focused-test
search found no ancestor-class declaration case. The map labels this as
source-only rather than a normative inheritance rule.

The name-and-handle flow correctly describes inherited member-name lookup but
did not directly warn that lookup does not predict callback projection. The
FOM-composition and directed-interaction guides likewise pointed to nearby
flows without linking to the cross-path map. Added direct links from all three
guides to that existing map instead of duplicating it. No source, behavior, or
test changes, and no Mermaid chart changed.

Next handoff: review the source-only 2025 directed-interaction
ancestor-declaration path. Compare the API, shared registry resolver, existing
directed flow, and focused tests to clarify target-class ancestor lookup and
nearest explicit selector behavior, especially child-versus-ancestor
precedence. Keep uncovered behavior source-only; make no normative or 2010
claim. Do not build, run tests, start CI, or edit the Requirements Lab.

### 2025 directed-interaction target-class ancestor precedence — 2026-10-11

The shared 2025 resolver treats FOM pair validity and producer/receiver
declaration applicability as a matching-pair search from the target's
registered object class toward its ancestors. Receiver selector lookup is
different: it returns the first explicit matching class/interaction pair on
that walk, so a child entry precedes an ancestor fallback in the current
source. The focused embedded known-target and process multi-recipient cases
use exact-class declarations; neither tests inherited declarations or
conflicting child/ancestor selectors. The directed guide now says this
explicitly and links the relevant resolver functions. This is source-only
Umbra behavior, not a normative inheritance rule; no new diagram was needed.

Next handoff: survey 2025 regional interaction class ancestry. Compare the
DDM guide and cross-path ancestry map, then check the pinned regional API,
shared resolver, and focused tests for the separate roles of inherited
dimension availability, exact-class declaration storage, and the sender's
class walk used for recipients. Look specifically for parent/child regional
cases. Keep uncovered behavior source-only and do not infer an HLA-wide or
2010 rule. Do not build, run tests, start CI, or edit the Requirements Lab.

### 2025 regional interaction class ancestry — 2026-10-11

The DDM guide already distinguishes ancestor-accumulated interaction
dimensions, exact-class regional declaration storage, and the sender-class-
to-parent recipient walk. The pinned 2025 API and shared resolver support
those separate source paths; the focused regional case uses the same class
for subscription and send, while the dimension query's parent class has no
dimension. Thus the parent/child regional consequences remain source-only,
not an HLA-wide or 2010 rule. Added a reciprocal link to the existing
cross-path ancestry map; no duplicate diagram or behavior claim was needed.
No build, test run, CI, or Requirements Lab edit.

Next handoff: survey 2025 object-class regional declarations across
parent/child registration, subscription, and recipient routing. Compare the
DDM object and interaction guides, pinned API, shared resolver, and focused
cases; distinguish class-available dimensions from declaration keys and
recipient selection. Keep untested consequences source-only, preserve the
2010 boundary, and add a flow or cross-link only for a demonstrated reader
gap. Do not build, run tests, start CI, or edit the Requirements Lab.

### 2025 object-class regional ancestry — 2026-10-11

The DDM guide now separates inherited class-dimension availability from
per-instance source-region associations and exact class/attribute regional
declarations. Its embedded-source flow shows discovery walking from the
registered class toward parents to select the receiver's closest qualifying
known class, then later value projection using that saved class. Parent-keyed
regional declarations may be considered for a child-registered object in this
source path, but are not copied to a child key. Reviewed regional cases use
the same class for registration and subscription; the cross-class behavior
remains source-only, with no normative or 2010 claim. The cross-path ancestry
map links to this focused DDM view. No C++ builds, test runs, CI, or
Requirements Lab edits.

The new Mermaid flow has not been locally rendered. Scoop updated its buckets
but has no `mmdc` manifest; do not claim render verification. Keep it and the
previously pending lifecycle chart in the diagram backlog until a supported
renderer is available or approved.

Next handoff: compare the 2025 process-endpoint object DDM path with the
embedded path documented here. Inspect the regional-declaration handler,
shared registry planning, process recipient/event fan-out, and existing
focused cases to distinguish shared semantics from profile-specific behavior.
Add navigation or clarification only for a demonstrated reader gap. Keep
uncovered behavior source-only, preserve the 2010 boundary, and do not build,
run tests, start CI, or edit the Requirements Lab.

### 2025 process-endpoint regional object boundary — 2026-10-11

Corrected the DDM guide's stale statement that process-endpoint regional
delivery was not covered: separate configured-endpoint cases cover initial
regional registration/discovery, subscription changes, and source-region
relevance transitions. Added a source-bounded flow that separates the shared
registry's state/recipient plan from process-session validation and event
routing, and explains the tested HLA_IMMEDIATE versus HLA_EVOKED initial
advisory boundary. All three regional object cases use the same Soda class on
publisher and receiver; none establishes parent-subscription/child-registration
behavior. The guide explicitly excludes ordinary/TSO update routing and makes
no embedded parity or 2010 claim.

The Mermaid flow is not locally rendered. Scoop has no `mmdc` manifest; no
npm/npx installation was performed. The inventory is now 245 diagrams across
34 design files (144 flowcharts); the three 2025 flowcharts added after the
242-block rendered baseline remain pending render verification. No build,
test run, CI, or Requirements Lab edit.

Next handoff: trace the 2025 process receive-order object-attribute update
from the shared registry's recipient/passel plan through process event routing
and callback visibility. Compare existing focused cases and the embedded
guide, separating receiver eligibility from process delivery mechanics. Add
documentation only for a verified reader gap; keep timestamped routing and
the 2010 stream separate. Do not build, run tests, start CI, or edit the
Requirements Lab.

### 2025 receive-order attribute update: public effect and process mapping — 2026-10-11

Added an HLA-first sequence that separates the producer's untimed, `void`
Update Attribute Values result from an eligibility-conditioned receiver
`reflectAttributeValues` callback. A separate, explicitly process-profile
sequence maps the 2025 endpoint's shared registry plan, recipient projection,
push frame or polling result slot, private recipient count, and callback
bridge. It clarifies that the current public polling pump uses
`receive_interaction` with an adjacent `attributeEvent` slot, while
`receive_attribute_update` names the pushed event; neither detail is HLA wire
protocol. The public focused case asserts one reflection for overlapping
active parent/child subscriptions under both callback models. Its fixture pairs
HLA_IMMEDIATE with push and HLA_EVOKED with polling; that does not establish a
general callback-model/transport rule. Passive three-role, DSE, regional
process projection, and embedded/process parity remain unproven here. The
timestamped route remains separate. No code, build, test, CI, 2010, or
Requirements Lab changes were made.

The inventory is 247 diagrams across 34 files: 144 flowcharts, 80 sequences,
and 23 state diagrams. The two new sequences are not locally rendered; five
diagrams added since the 242-block rendered baseline remain pending visual
verification. Scoop has no `mmdc` manifest, and no npm/npx installation was
performed.

The planned 2025 cross-read of attribute-value updates, update-rate admission,
ownership, and subscriptions is complete. The cross-cutting HLA-first map and
focused object-information guide already connect sender authority and service
outcome, receiver eligibility, attribute projection/rate admission, and
callback observation. No duplicate synthesis is warranted; provider
solicitation and a later value update remain distinct in the request guide.
This disposition adds no normative claim and does not imply embedded/process
or 2010/2025 parity.

Next handoff: render and inspect the five diagrams added after the 242-diagram
baseline: the HLA-level reservation lifecycle in the object-instance naming
guide; the object-class regional ancestry and process-endpoint regional object
flows in the DDM guide; and the HLA-first public-effect and process-mapping
sequences in the process-endpoint guide. Record parse, rendered output,
screenshot inspection, and GitHub rendering as separate evidence. The isolated
workspace has Mermaid CLI 12.0.0, but Chrome failed to launch with existing and
fresh profiles; no install was performed. Keep this render backlog separate
from documentation work.

### 2025 ordinary and directed interaction reader map — 2026-10-11

The current interaction guides already make the service-level contrast and
their implementation evidence legible: an ordinary send has no target object
and uses class subscriptions; a directed send names a target and evaluates
target knowledge, directed declaration pairs, and a receiver selector. The
ordinary and directed callback signatures make the observable target context
explicit. The detailed flows label active relevance, class promotion, selector
lookup, and parameter projection as bounded current-runtime behavior rather
than universal standard rules. No duplicate chart was warranted. See the
[interaction review ledger](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-ordinary-versus-directed-interaction-mental-model--2026-10-11)
for evidence and limitations. No source, build, test, CI, 2010, or Requirements
Lab work occurred.

Next handoff: audit the 2025 reader-facing distinction between a synchronous
RTI service exception and a later callback exception across the exception,
service-reporting, and callback/service-ordering guides. Add a conceptual map
only if it resolves a real gap; do not imply a general rollback guarantee.
Continue using existing source/test evidence without compilation or test
reruns, keep 2010 and embedded/process boundaries explicit, preserve the five
pending local-render checks, and leave the Requirements Lab out of scope.

### 2025 callback exception versus Evoke service exception surface — 2026-10-11

The exception, service-reporting, callback-ordering, and cross-cutting guides
already separate caller outcomes, committed effects, reports, callback entry,
and application observation; no fifth overview diagram was needed. A smaller
API-reader gap is now clarified in the callback guide: the callback's
`FederateInternalError` annotation is distinct from the Evoke methods' own
`@throws` lists. Current Umbra source can propagate an escaping callback error
through Evoke, but the exact normative treatment remains unresolved and the
focused throwing-callback test is absent. See the [review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-callback-exception-versus-evoke-service-exception-surface--2026-10-11).
No build, test, CI, 2010, or Requirements Lab work occurred.

Next handoff: compare save/restore treatment of in-flight work across the main
save/restore guide and focused ownership, request-response, time/retraction,
and callback guides. Do not assume all pending work is persisted or canceled;
classify only documented evidence, and add a comparison or flow only if it
closes a reader gap. Use existing code/test evidence without builds or test
reruns, keep 2010 and embedded/process boundaries explicit, preserve the five
pending render checks, and leave the Requirements Lab untouched.

### 2025 save/restore in-flight work comparison — 2026-10-11

The main save/restore guide now includes a compact index for the distinct
representations readers may call “pending”: provider request/response stages,
typed ownership work, queued TSO delivery/retraction, and callback frames
already admitted to a live `HLA_EVOKED` queue. It links the focused guides and
marks each implementation/test boundary. The audit found that a table was
more accurate than a new flowchart because these are parallel state categories,
not consecutive states in one lifecycle. It makes no universal HLA persistence,
crash-recovery, or 2010 parity claim. See the [review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-save-restore-in-flight-work-comparison--2026-10-11).
No C++ source, build, test, CI, or Requirements Lab work occurred.

Next handoff: review the save/restore guide's unresolved Join-during-operation
boundary using exact IEEE 1516.1-2025 §4.11 text from an authorized source.
Keep participant admission distinct from synchronization achievement,
ordinary membership, and time grants; do not infer normative behavior from
Umbra source or tests. If the clause is unavailable, choose another bounded
HLA concept. Reuse current source/test evidence without compilation or reruns,
keep 2010 and embedded/process boundaries explicit, preserve the five pending
render checks, and leave the Requirements Lab untouched.

### 2025 five-diagram render verification — 2026-10-11

The five diagrams missing from the 242-block baseline now render locally and
have been screenshot-inspected: the object-name reservation lifecycle, two
DDM regional-object flows, and two process-endpoint sequences. The process
sequences were reflowed to shorten labels after the first screenshot showed
crowding; all five then rendered successfully with the installed Mermaid CLI.
All 247 current diagrams have local render evidence, but the complete set has
not received visual review and GitHub rendering remains unverified. See the
[review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-five-diagram-render-verification--2026-10-11).
No C++ source, build, test, CI, or Requirements Lab work occurred.

Next handoff: audit the HLA object-information lifecycle across registration,
discovery, subscriptions, initial values, and the first value update. Compare
the object-information, subscription-state, Auto Provide, DDM, and
process-endpoint guides. Focus the known evidence gaps around ordinary
known-object/new-attribute behavior and ordinary process-profile initial
values; add a cross-guide map only if it closes a reader gap. Use existing
source/test evidence without builds or reruns, keep the 2010/2025 boundary
explicit, and leave the Requirements Lab untouched.

### 2025 object-information lifecycle audit — 2026-10-11

The existing 2025 guides already connect publication and registration,
per-receiver discovery, subscription and DDM eligibility, discovery-time
current values, distinct Auto Provide solicitation, later provider updates,
and reflection. The subscription guide already contrasts a not-yet-known
object with adding an attribute to an already-known object and separately
maps the embedded/process late-subscription evidence. No duplicate overview
diagram was warranted; the ordinary known-object/new-attribute result and
ordinary process-profile initial values remain explicitly unresolved. See the
[review record](HLA-BEHAVIOR-FLOW-GUIDES-REVIEW-LEDGER.md#2025-object-information-lifecycle-audit--2026-10-11).
The active portfolio and DDM backlog rows now agree that 247/247 diagrams have
local render output, while full visual review and GitHub rendering remain
pending. A targeted screenshot attempt for the subscription guide produced no
images: Mermaid CLI's Chrome launch failed before rendering even with serial
jobs and an ephemeral profile. Keep its passive-recipient and late-subscription
visual checks pending. No source, build, test, CI, 2010, or Requirements Lab
work occurred.

Next handoff: review the exact IEEE 1516.1-2025 §5.8 text from an authorized
copy to distinguish initial values when a subscription makes an unknown
object discoverable from adding attributes after that object is already
known. Keep normative wording separate from the embedded implementation
trace and the unresolved ordinary process route. Do not infer the rule from
source comments; avoid builds and test reruns, preserve the 2010/2025 boundary,
and leave the Requirements Lab untouched.

## 2025 subscription chart renderer setup follow-up — 2026-10-11

The configured Scoop buckets contain no `mmdc`/Mermaid CLI package, so the
user-authorized fallback installed Mermaid CLI 12.0.0 globally through the
already-installed Scoop Node.js LTS. A targeted render of the late-subscription
flow still failed before producing an image: Puppeteer could not launch system
Chrome, and a second attempt using system Edge also exited before page startup.
No browser process was terminated. This is a browser-launch failure, not a
Mermaid parse failure, and it does not invalidate the prior 247 local-render
results. The passive-recipient and late-subscription screenshot checks remain
pending; do not reinstall or repeat the same launches unless browser/runtime
state changes.

Next handoff: continue the bounded HLA-first portfolio review and select a
specific unanswered reader question before changing a guide; the current
object-information flows already connect registration/discovery, receiver
eligibility, initial values, Auto Provide, later updates, and reflection. Keep
the exact §5.8 normative distinction open until its authorized text is
available. Reuse existing evidence, retain 2010/2025 and embedded/process
boundaries, and avoid builds, test reruns, CI, and Requirements Lab changes.

## 2010 reference-profile coverage review — 2026-10-11

A bounded review of the separate [2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md),
its cited `reference_2010.cpp` paths, and the registered binding-shell smoke
found no missing major reader flow within the currently implemented profile.
The guide already separates Connect/Join, object information, interactions,
synchronization, timing values versus unavailable time-managed services, and
callback delivery/failure. Its single binding smoke supports only the selected
two-member, successful `HLA_IMMEDIATE` path; empty-subscription discovery,
callback-failure fan-out, and synchronization-barrier details remain
source-only or untested as labeled. No 2010 diagram was duplicated, no 2025
behavior was imported, and no 2010 test or CI was run.

The follow-up subscription preview also remains unverified: Mermaid CLI 12.0.0
still could not launch Puppeteer's managed Chromium; the app-managed browser
could not reach the loopback preview. No screenshot was produced. Leave local
visual review and GitHub rendering pending, and do not repeat browser attempts
unless a supported preview environment changes.

Next handoff: continue from a specific uncovered HLA reader question rather
than expanding either stream for diagram count. The current 2010 reference
profile is adequately mapped for its existing evidence; expand it only when a
new 2010 implementation path or focused evidence creates a distinct reader
need. Keep unresolved 2025 normative text and visual-review gaps explicit;
preserve the 2010/2025 and embedded/process boundaries.

## 2025 explicit Request Attribute Value Update boundary review — 2026-10-11

The existing [attribute-value request guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md)
already explains the solicitation → provider callback → optional value update
→ independently eligible reflection sequence and distinguishes explicit
requests from discovery-triggered Auto Provide. No duplicate diagram was
warranted. The guide now makes the requester-state nuance explicit: the
object-instance path requires a requester-known object, while the shared 2025
class planner expands registered descendants without consulting per-instance
known-object or subscription state when forming provider groups. The process
service calls that same planner but delivers callbacks through its own event
path. Its focused class test makes no public subscription call, yet first
observes discovery of the target objects; neither class test isolates an
undiscovered target or a value response to an unsubscribed requester. Keep
these as bounded source/test observations, not portable §6.21 conclusions.
The portable sequence now labels its lifelines as roles rather than distinct
federates and adds an accessible title/description. The sequence body parsed
with the installed Mermaid library after excluding that metadata; the full
parse path still needs browser-backed verification because the Node-only API
has no usable DOMPurify context. Mermaid CLI 12.0.0 is already installed under
Scoop Node but is not on this shell's PATH, so no reinstall was needed. No
screenshot or GitHub-render claim is made.

A cross-link audit of all 42 `docs/design/*.md` files found no missing local
paths and no broken anchors among 186 local fragment links; seven stale
anchors were corrected. Screenshot review and GitHub rendering remain
unverified. This documentation pass made no C++ source edits, builds, test
runs, CI changes, 2010 changes, or Requirements Lab changes.

Next handoff: obtain authorized IEEE 1516.1-2025 §6.21 text and resolve whether
the explicit request service gives its caller any direct response entitlement
beyond ordinary receiver eligibility. Update the portable model only from
that clause; keep the shared planner, separate process delivery, test limits,
Auto Provide, and 2010 boundary distinct.

## 2025 callback re-entry: API annotations and selected evidence — 2026-10-11

A bounded review of the pinned 2025 `RTIambassador` declarations, current
adapter guards, and existing test bodies made the distinction between API
exception annotations and the complete HLA call-permission rules more
actionable. The API lists `CallNotAllowedFromWithinCallback` on all four
Connect overloads, Disconnect, both Join overloads, Resign, and both Evoke
services. The current adapter has guards for those service groups. Existing
focused cases exercise both Evoke calls from an immediate callback and Resign
from selected embedded `HLA_EVOKED` save callbacks, including timed-save
advance variants. The bounded test scan found no focused Connect, Disconnect,
or Join re-entry cases; the save tests do not establish a universal Resign rule,
process-profile parity, or total callback permission matrix.

The callback guide now places those API/source/test facts in one compact map
and explicitly warns that a missing `@throws` annotation is not permission to
call a service from every callback. No new diagram was warranted: the existing
recursive-dispatch flow already separates implementation capability from
service permission. The complete normative matrix remains open until the exact
standard service clauses are available. No test was rerun, and no C++ source,
build, CI, 2010 behavior, or Requirements Lab content was changed.

Next handoff: continue the bounded HLA-first review for a distinct reader gap;
do not block other documentation on unavailable normative text. Keep the
callback matrix, explicit-request entitlement, §5.8 subscription, and §4.11
Join/save-restore questions clearly unresolved until their authorized clauses
are checked. Preserve the 2010/2025 and embedded/process boundaries; no builds,
test reruns, CI, or Requirements Lab edits.

### 2025 connection and model-to-runtime orientation audit — 2026-10-12

A bounded reader review found that the existing lifecycle guide already
separates ambassador connection, federate membership, and execution lifetime,
including their distinct API results and local state transitions. The FOM
guide separately explains per-module validation, complete-set composition,
FDD materialization, and the later runtime-service boundary. The names/handles
guide then distinguishes execution-scoped model symbols, inherited
class-member lookup, federate-local known-object state, typed handles, and
normalization coordinates. The cross-cutting guide supplies the compact
model-vocabulary-to-live-state overview.

These views form a useful reading path but are intentionally not one universal
state machine. Merging them would conflate model validity with runtime state,
or model declarations with one federate's object knowledge. No additional
diagram or prose was warranted. This review used the existing HLA/API and
evidence maps only; it adds no normative claim or new 2010/2025 or
embedded/process equivalence. No source, build, test, CI, or Requirements Lab
work occurred.

Next handoff: keep the 2025 new-attribute-for-known-object rule open pending
authorized §5.8 text. If that text is unavailable, select a different concrete
HLA reader question from the existing portfolio rather than repeating the
connection/model survey. Continue to lead with portable HLA concepts, use
Umbra only as bounded evidence, preserve edition/profile boundaries, and keep
the implementation worker's build capacity free.

### 2025 normative-text access check and handoff cleanup — 2026-10-12

The official [IEEE 1516.1-2025 page](https://standards.ieee.org/ieee/1516.1/6688/)
currently exposes purchase and subscription access, not the exact §5.8, §6.21,
or §4.11 clause text needed for the open questions. Keep those normative
outcomes unresolved; do not infer them from the public API, Umbra source, or
focused tests.

The order/transportation guide's prior next-topic pointer sent readers to a
name/handle audit that has already been completed. Replaced it with a durable
cross-link path to the time, DDM, and callback guides, plus an explicit note
that the sender-to-receiver order mapping remains open pending authorized
standard text. No new diagram or normative claim was added. No C++ build, test,
CI, 2010 behavior, or Requirements Lab work occurred.

### 2025 cross-cutting HLA reader-map audit — 2026-10-12

Completed the bounded audit of the delivery matrix, object-attribute path,
protocol-completion map, service-result/report map, and service-contract map.
Their existing distinctions are useful and already HLA-first: delivery axes
are not a fixed pipeline; a value request solicits rather than supplies a
provider's response; callback entry is separate from protocol completion; and
the HLA service contract, not set-valued argument shape, determines specified
outcomes. No new chart was justified. One generic-map ambiguity was corrected:
the service-result diagram now marks an HLA-required state/effect as optional
when the contract specifies one and separates it from an RTI-private commit
point. Its updated Mermaid text has not been rerendered or visually reviewed.

A bounded cross-guide deletion check found deletion authority, federation-wide
removal, per-federate known-object cleanup, and callback timing already covered
in the ownership and object/interaction guides; no duplicate deletion guide
was warranted. The concrete next handoff is a bounded reader audit of 2025
interaction delivery across the object/interaction, DDM, DataElement, time,
and callback guides, focused on keeping parameter meaning, receiver
eligibility, and callback observation separate from object-instance state.
No C++ source, build, test, CI, 2010 stream, or Requirements Lab work occurred.

### 2025 ordinary interaction receipt evidence update — 2026-10-12

The bounded interaction-delivery survey found no need for another end-to-end
diagram: existing guides already separate the FOM class/parameter and encoded
value path, ordinary recipient relevance, regional scope, TSO eligibility, and
callback observation. The worktree contains a newly verified embedded public
API receipt case (27 assertions; five direct requirement-section pairs per
the roadmap). It sends a descendant interaction to one receiver while the
receiver subscribes successively at base, base-plus-middle, and exact-child
levels; each time there is one callback, and its class equals the selected
candidate. The case uses an empty parameter map and no regions. It therefore
does not test inherited parameter projection, regional matching, passive
relevance, DSE, process behavior, or the normative inheritance rule.

Updated the object/interaction guide's diagram and evidence map to show the
selected callback class and link that focused case, while keeping projection
evidence separate. Updated the DDM guide to link the ordinary case but state
that it does not establish the regional overlap or available-dimension paths.
This is embedded 2025 development-profile evidence, not conformance or 2010
evidence. The updated Mermaid diagrams have not been rerendered or visually
reviewed. No C++ source, build, test, CI, or Requirements Lab work occurred in
that documentation increment.

### 2025 directed-interaction receipt: two class axes — 2026-10-12

Completed the bounded source-and-test audit of the directed receive-order
path. The guide now distinguishes the target's registered object class, used
for FOM directed-pair validity and producer publication, from each receiver's
known object class, used to find an applicable directed subscription. It also
shows the interaction search from sent class toward its superclasses and the
subscription's paired object-class lookup as separate axes.

The current retained `LastTest.log` records the embedded
`directed_interaction_inherited_receipt` case passing with 35 assertions. Its
fixture registers a child target, discovers it at a base object class, gives
the receiver both a base-object/base-interaction subscription and a deeper
child-object/child-interaction subscription, and sends the child interaction.
It observes one callback with the base interaction class. This is bounded
embedded 2025 evidence with empty parameters; it does not establish the
normative inheritance rule, object-ancestor fallback, parameter projection,
TSO behavior, process-endpoint parity, or anything about 2010. The retained
passing log supersedes an earlier failing local result observed during review.

Updated Mermaid has not been rendered or screenshot-reviewed. The passing
CTest result is the implementation worker's retained worktree result; I did
not build or rerun it. No CI, 2010, or Requirements Lab work was performed for
this documentation increment. Next: audit the existing process-endpoint
directed path and test against the new class-axis explanation, without
building or rerunning tests;
keep the process callback boundary and evidence gaps explicit. The linked test
source and FOM fixture are currently untracked in the shared worktree; their
implementation owner still needs to include them in version control before
this evidence link is durable on GitHub. This documentation pass did not stage
or alter those C++ files.

### 2025 process directed receipt: push/pull callback boundary — 2026-10-12

Completed the bounded comparison of the process receive-order route with the
embedded callback path. The process send handler uses the same registry
recipient planner, materializes a recipient-specific event, and either pushes
an event frame or places the event in a recipient queue for `Receive
Interaction`. The callback bridge forwards the projected event payload; the
process path does not repeat the embedded callback-time registry eligibility
check. Updated the guide's process flow to show the push/pull fork explicitly
and corrected prose that had described pull as the only process path.

The existing public process test uses an exact-class fixture with two
receivers and excludes the sender. It configures pushed receive-order event
delivery for `HLA_IMMEDIATE` and uses pull receive for `HLA_EVOKED`; assertions
check each receiver's class, target, tag, producer, transport, and empty
parameter set. It does not test inherited declarations, post-acceptance
mutation, or callback timing relative to service return. This is 2025 process
source/test evidence only, not normative text, not proof of stale-work
behavior, and not an equivalence claim with the embedded callback boundary.

The process Mermaid diagram has not been rendered or screenshot-reviewed. I
did not build or rerun C++; no CI, 2010, or Requirements Lab work occurred.
The focused inherited-receipt source and FOM fixture remain untracked in the
shared worktree and still need to be included by their implementation owner.
Next: audit timestamped directed delivery across per-recipient admission, TSO
queue/grant/retraction, and callback boundaries, reusing the existing time and
retraction guides without duplicating them or claiming profile parity.

### 2025 timestamped directed-interaction recipient split — 2026-10-11

Completed the bounded source/test audit of timestamped directed delivery. The
embedded send path plans target-specific recipients, retains all planned
recipients in the retraction record, and queues only time-constrained
recipients for TSO. Non-constrained recipients instead take the receive-order
callback path, with sent order `TIMESTAMP` and received order `RECEIVE`; a
constrained recipient is rechecked at its TSO callback boundary. The guide's
flowchart now names the pending-recipient retraction outcome without implying
that every undelivered copy must be a TSO queue entry.

The mixed-fanout test source observes Request Retraction for the already
delivered recipient and suppression of the queued constrained recipient
without either its original callback or a Request Retraction. A separate
embedded case observes accepted directed TSO work survive producer resignation
for FQR, TARA, and NMRA recipients. The process endpoint remains a separate
boundary: its TSO dispatcher rechecks and sends an event frame, while the
client callback gate and completion acknowledgement govern the in-transit
path. Its focused `HLA_EVOKED` test observes directed callback before grant
after callbacks are enabled. These are selected source/test observations, not
complete normative rules or embedded/process equivalence. I reused recorded
test sources and prior results only; no C++ build or test rerun occurred.

The updated Mermaid chart was not rendered. Mermaid CLI is already installed,
but the review ledger records browser-launch failures and explicitly defers
retries until runtime conditions change; I did not reinstall or retry it. No
2010, Requirements Lab, or implementation-source changes were made.

Next: perform a bounded HLA-first reader-gap survey and select one concrete
question not already answered by the portfolio; prefer tightening an existing
guide to adding another diagram when that is clearer. Keep exact-standard
uncertainties open, use existing evidence without compiling, and preserve the
edition and profile boundaries.

### 2025 HLA-first portfolio survey and inventory reconciliation — 2026-10-11

A bounded scan of the current guide map and 2025 RTI service families found
existing coverage across Federation, Declaration, Object, Ownership, Time,
DDM, RTI Support, and callback controls; another umbrella guide would repeat
the topic guides. I statically reviewed the cross-cutting service-outcome/report
map and traced its three case-study rows to current source/test locations. The
map correctly makes required HLA effects conditional on the operation
contract, treats profile commit points as private implementation facts, and
keeps selected report routing separate from the caller-visible result. The
report-writer failure case has assertions in its body but its last recorded
run failed before reaching them, so that branch remains source-derived. No
universal transaction, rollback, 2010, or embedded/process equivalence is
claimed.

The refreshed inventory is 248 Mermaid diagrams across 34 diagram-bearing
files: 145 flowcharts, 80 sequences, and 23 state diagrams. The separate 2010
stream has 14; the 2025 stream has 232; the index has 2. The previous
247-diagram local-render and syntax evidence does not cover the current
addition. The current cross-cutting guide contains 13 diagrams, correcting
its former 12-diagram backlog count. Full visual and GitHub review remain
pending. No C++ source was changed and no build, test, CI, 2010, or Requirements
Lab work occurred.

Next: source-check the six-example “When work becomes stale” synthesis against
the linked DSE, subscription, deletion, ordinary/directed interaction,
resignation, and retraction guides. Verify that withdraw, invalidate,
preserve, and account remain operation- and recipient-specific—not a shared
cancellation or rollback rule. Correct only a concrete mismatch; wait for a
permitted preview/runtime change before retrying visual review of the current
diagram set.

### Six-example stale-work evidence audit — 2026-10-11

Source-checked the cross-cutting “When work becomes stale” examples against
the linked subscription/DSE, deletion, ordinary/directed interaction,
ownership, retraction, and lifecycle guides. The distinctions hold:
active/passive changes affect later per-attribute projections. DSE defers
eligibility only for certain service-specific candidates. Accepted deletion
invalidates selected object work while each receiver's removal has its own
commit gate. Ordinary and directed publication withdrawal use different
callback fences; resignation cancellation is member-scoped; TSO retract
separates pending delivery from an already-entered callback; and lifecycle
cleanup distinguishes the departing member's route from survivor routes. No
shared HLA cancellation/rollback rule is implied. The cross-cutting table now
distinguishes active/passive projection from DSE candidates, and its lifecycle
row limits callback-queue reset to the selected embedded voluntary-resign
path while linking forced connection loss separately. These remain bounded
2025 source/test observations, not normative or cross-profile equivalence
claims. No C++ build, test, CI, 2010, or Requirements Lab work occurred.

### Already-known object subscription evidence audit — 2026-10-11

Rechecked the open 2025 §5.8 question against current source and focused test
bodies. The embedded callback path plans ordinary initial values only after
`Discover Object Instance`; its focused late-discovery test observes the
selected stored value, but no reviewed test isolates adding a new attribute to
an object the receiver already knows. The process subscription path replans
discovery, while its separate initial-value event is gated to RTI-owned MOM
objects. The public process discovery test subscribes before registration and
asserts discovery; the private known-object/second-attribute test asserts the
owner's relevance advisory, not a subscriber reflection. Thus the inspected
source/tests do not establish an ordinary current-value snapshot for an
already-known object, and they do not resolve the normative result or profile
parity. The public [IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers purchase and subscription access, not the licensed §5.8 clause text;
leave that interpretation open. The existing subscription guide already
states this gap, so no duplicate chart or speculative rule was added. No C++
source, tests, build, CI, 2010 stream, or Requirements Lab content changed.

### Attribute Value Update Request guide evidence audit — 2026-10-11

Checked the guide's HLA-facing sequence and scope diagrams against the current
registry, ambassador, process-service, and focused-test evidence. They keep the
request tag/provider callback separate from the provider's optional later
value-bearing update and each receiver's ordinary eligibility decision; the
roles may coalesce, and the request does not guarantee callback-before-return.
The implementation diagrams explicitly separate object-known validation,
class expansion/provider partitioning, regional request filtering, and the
embedded callback-entry recheck. The process service uses its own event seam;
its class-request test covers registered subclass instances, not complete
profile parity. The RTI-owned MOM direct-reflection path is separately
implemented and exercised for a selected report-file value, not generalized
to ordinary provider solicitation. Existing focused evidence does not settle
direct response entitlement for an unsubscribed requester; the public IEEE
listing exposes purchase/subscription access, not §6.21 text. The guide already
states these boundaries, so no duplicate chart or normative guess was added.
No C++ build, tests, CI, 2010, or Requirements Lab work occurred.

Next: audit the 2025 [DDM and regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md), focusing on the HLA-first region-template lifecycle: distinguish dimension/region declaration, committed extents, per-object update associations, subscription regions, and deletion/invalidated use. Check its diagrams and current focused evidence for a genuine reader gap; do not add another diagram if the existing lifecycle/overlap flows already teach the distinction. Preserve its 2025-only scope, keep the 2010 stream separate, and do not build or rerun tests. Current-set visual review remains pending until an approved preview runtime is available.

### 2025 Federation MOM projection versus execution lifecycle — 2026-10-12

The Federation MOM guide's `NotEstablished` state could be read as meaning
that no federation execution exists. Added prose clarifying that the chart
tracks only the 2025 federation-scoped MOM projection: Create may already
leave an execution present with zero members. Linked the separate lifecycle
guide's absent/empty/joined execution chart and explicit Destroy path. The
embedded source establishes the projection during Join before per-member
setup finishes and retains it if that later setup rolls membership back; this
ordering is source-observed, with no focused failed-first-Join test or process
parity claimed. The chart did not change, so no render or diagram-count update
was needed. No 2010, build, test rerun, CI, or Requirements Lab work occurred.

Next: audit the 2025 service invocation reporting guide for the HLA-first
boundary between synchronous service outcome, invocation-report record, and
later requested MOM report interaction. Reuse existing receive-order evidence;
keep embedded/process and 2010 boundaries explicit, avoid duplicate diagrams,
and do not build or rerun tests.

### 2025 invocation reports versus requested Federation MOM reports — 2026-10-12

Compared the service-invocation and Federation MOM guides with the 2025 MIM,
cross-cutting result map, and bounded focused-test sources. Added an explicit
three-trace distinction: the public service's return/exception, the
subject-switch-controlled `HLAreportServiceInvocation` record/interaction,
and an explicit request that causes a separate `HLAreport...` interaction
about Federation MOM state. Added a reciprocal link to the Federation MOM
guide. Kept the selected embedded file-append failure caveat visible, and did
not imply a synchronous RPC reply or a universal report-before-return rule.
The focused `HLA_EVOKED` sources show invocation-report-before-directed
callback and requested MOM report observation after callback draining; these
are individual examples, not broad profile evidence. The file-append failure
effect is source-observed; the cited test run failed during federation
creation before its append assertions. No diagram changed, so no Mermaid
render was required. No build, tests, CI, 2010, or Requirements Lab work
occurred.

Next: audit the 2025 exception-reporting guide at the boundary between public
service exceptions, `HLAreportException`, `HLAreportMOMexception`, and failed
`HLAreportServiceInvocation` reporting. Use bounded 2025 evidence only; keep
2010 separate and do not build or rerun tests.

### 2025 exception-report scope and report-family boundary — 2026-10-12

Reviewed the exception-reporting guide's API boundary, three-report
comparison, decision map, two failure routes, process combined-report trace,
MIM declarations, and selected source/test references. The caller's typed
exception, advisory `HLAreportException`, rejected-MOM
`HLAreportMOMexception`, and `HLAreportServiceInvocation` are already kept
separate; callback timing and the source-only process two-report attempt order
are carefully qualified. Added a scope note and cross-link distinguishing
successful request-driven Federation MOM reports such as
`HLArequestFOMmoduleData` → `HLAreportFOMmoduleData` from the three routes in
the exception comparison. This closes the risk that the table reads as an
exhaustive MIM report inventory. No diagram changed; the existing decision
map and sequences remain the right home for the failure flows. Test sources
were inspected, not run. No process/embedded or 2010 parity is inferred; no
build, tests, CI, or Requirements Lab work occurred.

Next: audit the 2025 Object and Interaction Information Flow and Object
Attribute Subscription State guides together for a genuine reader gap in the
path from publication/send through receiver eligibility, known-object state,
and callback observation. Prefer linking over duplicating existing flows;
keep 2010 separate and do not build or rerun tests.

### 2025 object information and subscription-state wayfinding — 2026-10-12

Compared the broad Object and Interaction Information Flow guide's five
delivery questions with the focused Object Attribute Subscription State
guide. Their concepts and evidence boundaries are consistent: publication,
ownership, receiver qualification, object knowledge, and application
observation remain separate, while the focused guide details ordinary and
regional declaration mutation plus the discovery/value gates. The broad guide
already deep-links to the focused one at object-discovery and attribute-value
request boundaries; the focused guide lacked a return path to the end-to-end
map. Added a short scope-level cross-link instead of copying its routing
diagrams. The pinned 2025 subscription signature and inspected test sources
support the limited examples; they do not settle active-establisher/passive-
receiver coverage, ordinary process late-value behavior, or licensed §5.8
wording. No diagram changed or render was needed. No test/build/CI rerun, 2010
change, or Requirements Lab work.

### 2025 explicit request versus Auto Provide boundary — 2026-10-12

Compared the 2025 Attribute Value Update Request and Auto Provide guides.
Their existing flows and reciprocal links already distinguish a federate-
invoked request from discovery-triggered RTI solicitation, and both keep the
provider's later value-bearing update separate. Explicit requests preserve
their caller-supplied request tag and use their own pending-request checks;
Auto Provide's current embedded path uses an empty callback tag and a separate
switch/scope/ownership recheck. Initial reflection is also separate from either
solicitation. The pinned API declarations and inspected embedded baseline test
bodies support those bounded distinctions: the explicit test observes its
nonempty tag at the provider callback, while the Auto Provide test observes a
zero-length tag after discovery. These are not process-parity or universal
provider-response claims. No reader gap justified another diagram or guide;
existing diagrams already show the later provider update. No build, test rerun,
CI, 2010, or Requirements Lab work occurred; Mermaid CLI was not needed.

### 2025 object-name reservation versus local handle knowledge — 2026-10-12

Audited the 2025 Name and Handle Resolution and Object Instance Name
Reservation guides together. They already distinguish symbolic reservation
from successful object registration, the owner's returned object handle, and a
peer's later discovery-driven known-object state. Their links are reciprocal,
and the reservation guide includes a two-view synthesis: the owner path and an
eligible peer path are separate and are supported by two focused embedded
tests, not one combined end-to-end assertion. The pinned lookup API scopes
forward/reverse object-instance resolution to objects known by that federate;
the test sources show lookup failing while discovery is queued and succeeding
after the callback is evoked. The process lookup test cited by the guide covers
the owner's round trip, not peer discovery parity. Existing diagrams and
wayfinding already explain this boundary; no duplicate chart or text change
was warranted. No rendering, build, test rerun, CI, 2010, or Requirements Lab
work occurred.

### 2025 model extension versus name-to-handle resolution — 2026-10-12

Audited the 2025 FOM Module Admission and Composition and Name and Handle
Resolution guides together. Their diagrams cover complementary layers: the
FOM flow prepares and atomically commits a replacement model with the new
membership, then marks the observed post-commit Join failure edge as
source-derived and untested; the lookup sequence shows an already-joined
member resolving added declarations after a successful additional-module Join.
Both guides cross-link these views. The embedded object-class and
interaction-class tests observe stable typed-handle equality within that
execution while newly contributed class handles become resolvable. The
configured-process test covers lookup by the joining client only, not a prior
member's view. These are implementation observations, not cross-execution
numeric-handle guarantees or profile parity. Existing flows and reciprocal
links already explain the boundary; no additional diagram or prose change was
warranted. No rendering, build, test rerun, CI, 2010, or Requirements Lab work.

### 2025 model vocabulary versus DataElement value encoding — 2026-10-12

Compared the FOM composition and DataElement Encoding guides at the boundary
between model-declared type vocabulary and application-side value octets. The
DataElement guide already routes FOM/OMT input to model/schema composition and
links back to the FOM guide. The FOM guide explains model vocabulary and later
runtime transitions but did not identify the separate public `DataElement`
encoding layer. Added a concise reciprocal pointer stating that model
admission does not execute a federate's value encoder and linking to the
DataElement guide's representation-boundary flow. This does not assert a
one-to-one mapping from every OMT data type to a C++ codec. The pinned 2025
`DataElement` API exposes its own encode/decode operations; no implementation
or test behavior was changed. No diagram or inventory count changed, so no
render was needed. No build, test rerun, CI, or 2010 change occurred.

### 2010/2025 DataElement implementation-boundary audit — 2026-10-12

Compared the separate 2010 and 2025 DataElement guides with their pinned API
headers, edition-specific scalar/composite sources, shared helper headers, and
selected existing test sources. The guides correctly keep BE/LE selection at
the concrete element wrapper, keep encoded-layout examples tied to separate
test targets, and identify the public signature differences (`auto_ptr`/
`void decode` versus `unique_ptr`/`DataElement& decode`). Neither shared
byte-order primitive selects an order by itself, nor do those API similarities
establish a shared format contract.

The source audit exposed a second limited reuse point: both edition-specific
composite adapters import the same type-free checked-size, range, and padding
functions from `composite_primitives.hpp`. Added this fact to both edition
guides with links to the separate adapters and common helper. The codec
translation units and tests remain edition-specific; no composite parity is
inferred. Existing diagrams already show the two streams separately, so none
changed and no rendering was needed. Test sources were inspected, not run; no
build, CI, or Requirements Lab work occurred.

### Callback model versus process endpoint event delivery — 2026-10-12

Audited the 2025 Callback and Service Ordering guide against the Process
Endpoint Request and Callback guide and bounded implementation ranges. Both
guides already distinguish the HLA_IMMEDIATE/HLA_EVOKED callback policy from
the process service's queued/pushed event acquisition modes, and the callback
guide already explains the recipient-side request/pump exception boundary.
Added a scope-level link from the callback guide to the concrete process event
trace, explicitly labeling that guide as Umbra implementation mechanics rather
than authority for HLA callback semantics. No diagram was needed. Test evidence
was not rerun; no build, CI, 2010, or Requirements Lab work occurred.

### 2025 exception-report callback failure boundary — 2026-10-12

Compared the Exception Reporting and Callback and Service Ordering guides with
bounded hook, recipient-submission, dispatcher, process request, and focused
report-test evidence. `emitExceptionReport` is `noexcept` and suppresses a
callback exception only if it unwinds through that hook's own synchronous
report attempt. In the embedded recipient loop, an inline throw stops later
route submissions; the receive-order/time-state gate may defer callback entry
independently. Deferred/evoked or separate process-recipient stacks follow their
own callback exception path. Added a focused sequence diagram and a reciprocal
link. The process report test uses a non-throwing receiver, so the
throw boundary remains source-derived and untested. A render check is pending:
the configured Scoop buckets have no `mmdc` manifest. No build, test rerun, CI,
2010, or Requirements Lab work occurred.

### 2025 DDM empty-region-set and object/interaction boundary audit — 2026-10-12

Compared the DDM/Regions guide with Object and Interaction Information Flow and
bounded 2025 source/test evidence. Existing maps already separate object
attribute association/discovery/value routing from ordinary interaction
subscription/send-region routing, then keep overlap, callback observation, and
logical-time eligibility as distinct questions. The review found one concrete
reader gap: “empty” could mean a zero-dimensional Region object, an empty
RegionHandleSet supplied to Send Interaction With Regions, or an empty
designator set conveyed on an ordinary interaction callback. Updated the DDM
guide's interaction routing flow and added a small comparison table: the
explicitly empty service-input set is a no-send operation, does not fall back
to an ordinary default-region subscription, and is not the zero-dimensional
Region case. The cited 2025 planner and focused case support the implementation
example; no cross-edition inference is made. Added reciprocal scope-level
wayfinding to the end-to-end Object/Interaction guide. No duplicate diagram or
diagram-count change. The edited Mermaid chart is not rendered: Scoop has no
`mmdc` or `mermaid-cli` manifest, and the permitted install/search attempts found
no package. No build, tests, CI, or Requirements Lab work occurred.

### 2025 save/restore responsibility and temporal-boundary audit — 2026-10-12

Compared the Save/Restore, Federation and Federate Lifecycle, and Time
Management guides with bounded 2025 API/test evidence. Their existing diagrams
already distinguish federate application checkpoints from the RTI's federation
image and terminal result; Join membership from a protocol's own participant
set; accepted save requests from delayed initiation; and time eligibility from
callback observation. The lifecycle guide correctly leaves the exact Join
precondition/participant effect during save or restore unresolved rather than
transferring the expandable synchronization-point rule.

One navigation gap remained: Time Management labels deferred receive-order
callback closures as live-session state and says their general save/restore
behavior is unproven, but did not point readers to Save/Restore's selected
in-flight-work dispositions. Added that cross-link, explicitly warning that an
already-pushed process `HLA_EVOKED` frame surviving service-level save/restore
does not prove these deferred closures are in the durable image or survive a
restart. Tightened Save/Restore's reciprocal links to the exact time/callback
map and timestamped-message lifecycle. No diagram changed or was added; no
render was needed. The focused process-test source was inspected, not run. No
build, CI, 2010, or Requirements Lab work occurred; no process/embedded parity
or general callback-persistence claim is made.

### 2025 synchronization participant-set boundary audit — 2026-10-12

Compared the 2025 Synchronization, Federation and Federate Lifecycle, and
Save/Restore guides with the pinned Join declarations, bounded registry paths,
and focused late-join test. The descriptions agree: Join changes federation
membership; the embedded synchronization path separately snapshots current
members and permits expansion only when the synchronization-set overload was
omitted; Save/Restore Join admission and its participant effects remain an
unresolved contract question. The direct test covers omitted versus explicit
non-empty sets, not explicit-empty registration or partial-achievement-before-
Join; those paths remain correctly labeled source-derived. The guides do not
transfer the synchronization-specific expansion rule to Save/Restore or claim
2010/process parity.

The only gap was wayfinding from the synchronization guide's membership
orientation to the two exact cross-protocol sections. Added direct links to the
lifecycle participant-set map and Save/Restore Join boundary, plus a sentence
that the barrier-specific expansion is not a general membership or
Save/Restore rule. Existing diagrams already explain the distinctions; no chart
changed or was added, so no render was needed. Source/test evidence was
inspected, not run. No build, CI, 2010, or Requirements Lab work occurred.

Completed handoff: audit the 2025 synchronization registration/achievement callback boundary
alongside Callback and Service Ordering. Distinguish a synchronous service
result from submitted registration/announcement/report callbacks and the final
`federationSynchronized` outcome; compare source submission order with
HLA_IMMEDIATE/HLA_EVOKED application observation and process endpoint queueing.
Reuse bounded API/source/test evidence and the existing dispatch view; add a
chart only for a demonstrated reader gap. Keep 2010 separate and do not build or
rerun tests.

### 2025 synchronization callback, process-event, and service-report boundary audit — 2026-10-12

The HLA-level map and embedded callback zoom already separated registration,
announcement, achievement, and federation completion. A process-profile
reader gap remained: push event frames and correlated service responses travel
through different paths, and the process adapter orders selected service
reports differently from the embedded adapter.

Added a process-specific sequence and reciprocal cross-guide links. In pushed
mode, the process client buffers event frames until the matching response,
then releases its stream lock and submits the events before returning to the
public adapter. Thus a registrant who is also an announcement recipient can
have `announceSynchronizationPoint` submitted before the registration-result
callback. A final pushed `federationSynchronized` can likewise be submitted
before the adapter starts the achievement's invocation-report request. The
selected report sink may be a file or an `HLAreportServiceInvocation`
interaction, handled by the separate reporting guide. Polling-mode
notifications remain queued until a later `receive_interaction`.
The push/poll setting is independent from `HLA_IMMEDIATE`/`HLA_EVOKED`; the
trace labels all source ordering as Umbra-specific, not an HLA guarantee.

The paired `synchronization_point_catch2.cpp` cases are embedded-profile tests.
Separate public process-endpoint cases cover HLA_EVOKED with polling and
HLA_IMMEDIATE with pushed events, asserting registration success/failure,
announcement to the explicit-set receiver, and a false final result. Because
the registering sender is excluded from the set, the cases do not assert
same-recipient callback order or service-report order; these finer sequence
claims remain source-derived, not an absence of process callback coverage or
process/embedded parity. The new Mermaid sequence is not rendered because
`scoop install mmdc` found no manifest. Source/tests were inspected, not run.
No build, CI, 2010, or Requirements Lab work occurred.

2026-10-12: Completed the process polling-versus-push audit. The 2025 process
service keeps event-family queues per session; `receive_interaction` checks
synchronization announcement, federation synchronized, save, restore, and
interaction events in that order before reaching discovery/removal and later
families. A later-queued synchronization announcement can therefore be chosen
ahead of an older queued interaction event. Push mode bypasses this selector;
the process client buffers received event frames in one heterogeneous FIFO and
drains after the correlated response unlocks the stream exchange. Both paths
submit through the callback bridge, while `HLA_IMMEDIATE`/`HLA_EVOKED` is an
independent dispatch setting. This is an Umbra 2025 process-profile observation,
not an HLA-wide ordering guarantee. Paired public process synchronization cases
cover polling and push separately but do not assert mixed-family precedence, so
that distinction is source-derived. Added a bounded comparison flowchart to the
process endpoint guide. Mermaid rendering remains pending: the authorized
`scoop install mmdc` returned “Couldn't find manifest for 'mmdc'”; no package
was installed and no remote renderer was used. No tests/build/CI, 2010, or
Requirements Lab work occurred.

2026-10-12: Audited the callback-ordering guide and process-endpoint guide for
cross-family total-order implications. The HLA-first model already assigns
recipient eligibility to the service, dispatch timing to the callback model,
and local ordering to the admitted dispatcher/stream queue; synchronization
and process-specific ordering claims are separately bounded. Reworded the
callback-order concept to require an exact service and message/time-order rule,
and to say explicitly that callback model is dispatch policy, not sequence.
The pinned Connect signature and separate Evoke/enable/disable APIs support
that distinction; focused dispatcher tests cover local FIFO, not cross-service
ordering. No duplicate diagram was needed. No tests/build/CI, 2010, or
Requirements Lab work occurred.

2026-10-12: Compared the existing value-map diagram's tall layout with a left-to-right alternative and adopted the more compact layout. Mermaid Live Editor 12.1.0 accepted and rendered the exact guide source; the distinct object-attribute update/reflection and ordinary-interaction send/receive paths remain clear. This was not a GitHub render, so GitHub presentation remains unverified. No mmdc install, C++ build, test run, CI, 2010, or Requirements Lab work occurred.

2026-10-12: Audited discovery versus first reflection against the existing object/interaction and subscription-state guides. Their diagrams already distinguish receiver-known state, embedded late-subscription values, the process selector's discovery priority, and the process MOM-only initial-value branch. The focused ordinary process update test observes discovery and a later explicit reflection but does not assert order or an ordinary late-subscription snapshot; another diagram would duplicate the current source/profile comparison. The pinned API headers do not provide normative clause prose, and IEEE lists purchase/subscription access for IEEE 1516.1-2025; §§6.9/6.11 ordering and the ordinary §5.8 process result remain unverified. No C++ build, test run, CI, 2010, or Requirements Lab work occurred.

2026-10-12: Audited the 2025 object-class declaration and ancestry question
against the pinned API signatures, embedded registry source, and focused
child-instance/base-subscription integration case. Added a bounded guide note
separating inherited attribute availability, the class explicitly named by a
declaration, and the receiver's selected known class. The focused test shows a
child-class publication containing base and child attributes, followed by a
base-class discovery and base-only reflection for a base subscriber. It does
not prove the licensed standard's complete ancestry rules or passive,
regional, and process combinations. Existing ancestry and active/passive
diagrams already cover the selected paths, so no duplicate chart was added.
No rendering, C++ build, test run, CI, 2010, or Requirements Lab work occurred.

Next: review the existing 2025 TSO message-retraction guide as an HLA-first
teaching tool. Separate portable per-message service rules from per-recipient
event admission, callback-time retraction, and selected embedded/process
boundaries; reuse existing evidence and add no duplicate diagram unless a
concrete reader gap remains. Keep 2010 separate and do not build, rerun tests,
or edit the Requirements Lab.

2026-10-12: Reviewed the 2025 TSO retraction guide's HLA-first structure and
evidence boundaries. The portable per-recipient delivered/not-yet-delivered
outcome was already distinct from producer legality and recipient time-request
results; embedded and process queues, callback admission, and deletion-specific
effects were separately identified. Found one wording seam where the embedded
callback-entry threshold was labeled as an HLA outcome. Rephrased the guide to
lead with the portable outcome and identify callback entry as the embedded
ledger threshold. The existing diagrams already teach the relevant distinction,
so no diagram was added or rendered. Exact licensed §§8.22–8.23 prose remains
unavailable for normative verification. No test rerun, build, CI, 2010, or
Requirements Lab work occurred.

2026-10-12: Reviewed the 2025 DDM region-template lifecycle against the current
guide, bounded region-registry source, and focused test bodies. The existing
FOM/runtime bridge separates class-available dimensions from a region's fixed
dimension set; the lifecycle chart and prose separate pending range edits from
the committed routing snapshot. Object attribute source associations and
receiver subscriptions are distinct from each other and from the interaction
regional path. The existing deletion gate covers live associations and
subscriptions, stale-handle queries, and deferred object-removal cleanup. The
focused test source confirms an object association remains live until the peer
consumes its removal callback. No reader gap justified another diagram or
prose edit. Source/test code was inspected, not executed; no render, build,
tests, CI, 2010, or Requirements Lab work. Existing full-set/GitHub visual
review remains pending.

Next: audit the 2025 Order and Transportation Controls guide as an HLA-first
teaching tool. Distinguish message ordering from transportation type; trace
class defaults, per-instance overrides, callback-gated transportation
changes, and receiver-visible effects. Reuse existing diagrams and bounded
evidence; add no chart without a concrete reader gap. Keep 2010 separate and
do not build, rerun tests, start CI, or edit the Requirements Lab.

2026-10-12: Audited the 2025 order/transportation guide against the pinned API
headers and MIM, bounded embedded registry logic, and focused test bodies.
Existing flows already distinguish a timestamped service overload from the
effective sent/received order, keep receive-order and timestamp-order delivery
separate from transportation metadata, and place callback observation after
eligibility/time gates. Class defaults are prospective; attribute order and
transportation state are instance-scoped; transportation overrides take
effect at their confirmation callback. Query calls report effective values
through callbacks. The process evidence is a selected interaction
change/query case under both callback models, not process attribute parity.
Exact normative order mapping and the full service cross-product remain open
without licensed clause text and broader tests. Existing diagrams were
sufficient, so no chart or guide text changed. Source/tests inspected only;
no render, build, test run, CI, 2010, or Requirements Lab work.

Next: review the 2025 Declaration and Attribute Relevance Advisories guide as
an HLA-first model. Keep publisher class advisories distinct from owner
per-object relevance and receiver-local scope; trace subscription, hierarchy,
switch, and update-rate boundaries. Reuse existing flows unless a concrete
reader gap remains. Keep 2010 separate; do not build, rerun tests, start CI,
or edit the Requirements Lab.

2026-10-12: HLA-first review of the 2025 declaration/relevance guide found its
existing flows already separate publisher class edges, owner per-instance
relevance, receiver-local scope, discovery, and later reflection. A specific
switch-evidence boundary was unclear: declaration-level no-backlog behavior
must not be transferred to the per-instance owner switch. Added bounded
embedded evidence: the owner switch suppresses a rate-bearing On and an Off
during subscription transitions in the selected callback-model cases; after
reenabling, the test creates a fresh subscription and does not decide replay
of an already-relevant state. Added a direct test link and updated the backlog.
No diagram changed. Source and tests were inspected, not executed; no render,
build, tests, CI, 2010, or Requirements Lab work.

2026-10-12: Completed the 2025 Attribute Scope audit against subscription and
region transitions, the final-self-unsubscribe exception, stale callback
rechecks, and separate embedded/process evidence. Clarified that the queued
stale In is suppressed only in the embedded HLA_EVOKED case; HLA_IMMEDIATE
enters In before the later mutation. Process tests exercise both callback
models but not a stale-queue race. The embedded switch-off segment overlaps
the self-unsubscribe exception, so it does not isolate switch-only suppression;
the process case keeps the switch enabled. No diagram changed. Source/test
evidence was inspected, not run; no build, CI, 2010, or Requirements Lab work.

2026-10-12: HLA-first audit of Auto Provide confirmed the existing diagrams
already separate discovery-triggered solicitation, explicit requests, initial
reflection, and the provider's later value-bearing update. Added one
source-derived embedded boundary: a later scope-in for an already-known object
does not re-enter the discovery-only planner. The focused tests do not isolate
this no-retry edge, so the guide does not claim it as a normative HLA rule or
process behavior. No diagram was changed or rendered. No build, tests, CI, 2010,
or Requirements Lab work.

2026-10-12: Audited timestamped deletion across the 2025 object-information,
time-management, and TSO-retraction guides. Added one receiver-oriented
sequence to connect accepted producer request, receive-order versus queued-TSO
recipient paths, callback-entry state commit, and deletion-specific retraction.
The selected embedded FQR/TARA/NMRA case demonstrates three callback orders at
the same timestamp; these are scoped test observations, not a universal rule.
Embedded tests cover pre-delivery cancellation and mixed-recipient
post-delivery restoration. Source indicates pre-delivery retraction preserves
intervening mutations, but the cited test does not stage one. The process test
does not cover retraction/restoration, so no profile parity is inferred. No
2010 behavior is inferred. Also documented the source-only empty-recipient
case: deletion is accepted without a removal callback when no other federate
knows the object, while the name and retraction record are retained to the time
bound; no focused test isolates this. The Mermaid chart was manually reviewed but not
parsed/rendered because the authorized Scoop install failed with permission
errors and no mmdc manifest. Source and existing test bodies were inspected,
not run; no build, CI, or Requirements Lab work.

2026-10-12: Bounded HLA-first review of the existing 2025 save/restore guide
found its responsibility map and separate detailed save/restore machines
already cover application versus RTI checkpoints, request admission,
participant reports, RTI commit/rehydration, federation-level outcomes,
member-set limits, object state, and callback/durable-image boundaries. No
additional combined diagram was warranted; it would duplicate existing flows
and risk merging distinct HLA responsibilities or profile-specific behavior.
Existing source/test links were reviewed but no tests were run. No guide edit,
render, build, CI, 2010, or Requirements Lab work.

2026-10-12: Surveyed the ordinary and directed 2025 interaction guides,
implementation maps, and focused-test references. The existing guides already
explain ordinary class-scoped sends without object-instance knowledge versus
directed sends with a target object and separate receiver selectors. The
ordinary flow separately covers regional overlap and TSO time, while the
directed guide keeps its target/selector distinct from DDM regions and does not
assert a region-overlap rule. Both keep parameter projection and callback
boundaries distinct, including embedded/process event-commit differences.
Focused evidence is narrow and explicitly labeled; no
universal routing, process parity, or 2010 equivalence is claimed. No combined
diagram would add clarity, so no guide edit or render was warranted. No tests,
build, CI, 2010, or Requirements Lab work.

2026-10-12: Bounded 2010-only survey compared the current reference-RTI guide,
pinned 2010 API, generated shell, reference runtime, and focused test inventory.
The guide already covers the implemented lifecycle, declaration, object,
interaction, synchronization, and logical-time-value boundaries. Seven
2010-named smoke executables focus on binding, headers, encoding, and time
values/marshalling; the binding-shell scenario does not test the implemented
synchronization barrier. Six separate external-FOM Catch2 cases cover
2010 model validation/composition, not reference-RTI behavior. Since the
existing guide already labels the source-only synchronization edge and the
time-management services are unavailable, a second standalone 2010 guide would
duplicate the current map without stronger evidence. No files changed. Source
and test bodies were inspected, not run; no build, CI, 2025/2010 sharing, or
Requirements Lab work.

2026-10-12: Reviewed the existing 2025 callback guide's HLA mental model,
callback enablement and dispatch flow, ordering cautions, callback-gated state,
embedded Send Interaction and FederationSynchronized traces, and process
registration/achievement sequence. It already separates callback dispatch
policy from recipient eligibility, protocol effects, and application
observation. HLA ordering is tied to service/message/time rules; dispatcher and
process ordering are labeled as implementation evidence. The review found one
navigation gap: the guide lacked direct locators for the callback-control
clauses it asks readers to consult. Added a compact map for Connect §4.2 and
callback-control §§10.57–10.60, explicitly warning those locators imply no
global order. No new diagram; no build, tests, CI, 2010, or Requirements Lab
work.

2026-10-12: Rechecked discovery/initial-reflection order against the bounded
2025 embedded and process sources and test bodies. Embedded discovery precedes
initial-value planning, while its focused regional test records discovery and
reflection separately. Process polling prioritizes discovery before attribute
events, but the public process case's value comes from a distinct publisher
update and its recorder stores separate flags; ordinary process discovery
does not enqueue a current-value snapshot (MOM is a separate path). Neither
focused case asserts ordinary initial-reflection order. The pinned API gives
§§6.9/6.11 labels but not the normative clause text, so the issue remains open
and source comments are not elevated to HLA rules. Existing diagrams already
explain the source/test boundary; no chart changed. No build, test, CI, 2010,
or Requirements Lab work.

### 2025 class ancestry: path-specific behavior and evidence limits — 2026-10-12

The bounded audit confirmed that the existing cross-path map already separates
object-class ancestry from interaction-class ancestry and gives each service a
different job. A focused embedded discovery case registers a child-class
object, subscribes at its base class, and observes the base as the receiver's
known class with only the base attribute in the initial reflection. An internal
registry case checks ordinary interaction delivery at the exact class and
promotion to two ancestors; child-only parameters are omitted from the promoted
projections. These tests establish their respective embedded/registry behavior,
not a normative inheritance rule.

Directed interaction routing instead uses the target object's class ancestry
for declaration and receiver-selector lookup, while its parameter namespace is
the sent interaction class. The focused embedded known-target and process
multi-recipient cases use exact-class declarations; neither verifies ancestor
lookup. Keep that path source-observed. The existing map now states these test
limits directly, so another diagram would repeat it. The licensed normative
clauses remain unavailable, and no 2010 inference is made. No C++ changes,
tests, builds, CI, or Requirements Lab edits.

Next handoff: audit the HLA 2025 attribute-ownership negotiation and transfer
state machine. Separate the HLA-level request/offer/acceptance and ownership
outcomes from Umbra callback, queue, and commit points; use pinned API and
bounded source/test evidence, adding or revising a diagram only for a concrete
reader gap. Keep the 2010 stream separate; do not build, rerun tests, start CI,
or edit the Requirements Lab.

### 2025 negotiated divestiture: cancellation after confirmation — 2026-10-12

A bounded review found a reader gap in the owner-side negotiated-divestiture
flow: it showed the successful transfer after confirmation but not when the
request selected for that confirmation disappears before the owner responds.
The pinned 2025 API lists `NoAcquisitionPending` for Confirm Divestiture
(§7.6). A focused embedded case cancels the acquisition after the owner has
received `requestDivestitureConfirmation`; the owner's later confirm throws
that exception, ownership remains with the owner, the owner clears the pending
negotiation, and the requester later observes cancellation confirmation. The
registry path and test assertions support this as selected embedded behavior,
not a process-parity or universal timing claim.

Added a separate sequence diagram so the race is not blended into the
successful process-boundary negotiated-divestiture example. The source-fence
census now finds 254 Mermaid blocks across 34 diagram-bearing files: 14 2010,
225 2025 topic-guide, 13 cross-cutting, and 2 index diagrams. The new sequence
has no parser/render verdict: the authorized Scoop install found no `mmdc`
manifest and installed nothing. Evidence was inspected, not rerun. No C++
changes, builds, tests, CI, 2010 edits, or Requirements Lab work.

### 2025 ownership transfer and update-region association — 2026-10-12

A focused cross-guide review found that the DDM flow already explains how
source regions affect per-attribute routing, while the ownership flow explains
transfer outcomes; neither showed how those state questions meet at the
transfer boundary. Added a separate embedded-2025 sequence comparing three
existing focused test scenarios: the former owner's association is cleared and
not restored on reacquisition; a non-owner's still-pending deferred association
is promoted at transfer; and unassociating it first prevents promotion. The
tests are separate, and the diagram says so. API headers identify §§7.9,
7.13, and 9.6–9.7, but the full normative clause prose is unavailable, so the
association-survival detail is presented as selected embedded source/test
evidence—not a universal HLA or process-profile rule. The DDM guide now links
back to the ownership diagram. The source census is now 255 diagrams (147
flowcharts, 85 sequences, 23 state diagrams); this sequence has no fresh parser,
render, visual, or GitHub verdict. No C++ code/test execution, build, CI, 2010,
or Requirements Lab work.

### 2025 accepted TSO update across ownership transfer — 2026-10-12

The existing TSO guide already shows that an update accepted while A owns an
attribute can remain queued after B acquires it; the ownership and TSO guides
keep current authority separate from producer identity and receiver delivery.
The current focused embedded test source asserts B owns and A no longer owns
before B advances, then asserts the queued reflection still names A as
`producingFederate`. Added that direct ownership contrast beside the existing
trace. The test source has a concurrent worktree edit and was inspected only,
not executed; no runtime result for that current edit is claimed. These are
`HLA_EVOKED` source assertions, not process parity or a universal
receiver-state rule. No new diagram or count change was warranted. The test
and related TSO source were left untouched. No build, CI, 2010, or Requirements
Lab work.

Next handoff: review whether a producer that remains a member may invoke
`retract` for an already-accepted TSO update after attribute ownership
transfers. Compare the pinned Retract API, the focused
`timestamped_attribute_update_ownership_transfer_catch2.cpp` case, the
producer-bound retraction check, and the existing TSO guide. The transfer test
does not attempt Retract after transfer; do not infer normative authority from
producer identity alone. Keep 2010/2025 and embedded/process boundaries, and do
not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Retract authority after attribute ownership transfer — 2026-10-12

The embedded ambassador requires its caller to remain joined and
time-regulating; the registry additionally checks membership, that the caller
is the message's original producer, timestamp eligibility, and retraction
ledger state. The inspected path has no current-attribute-owner check. This is
a source observation only: the ownership-transfer test does not invoke Retract
after transfer, the API header lacks the full §8.22 prose, and no process-route
parity is established. The existing TSO guide now states this nuance without
turning it into a portable HLA rule. No new diagram was warranted; this docs
pass made no C++ edits and ran no build, tests, or CI, and did not change the
2010 stream or Requirements Lab.

Next handoff: verify the normative 2025 Retract authority and time-bound rules
in §§8.22–8.22.3 against the ownership boundary. If the licensed clause text
is unavailable, keep the portable outcome open and continue with another
bounded HLA-first reader gap. Consider fresh focused runtime evidence only if
a material implementation claim cannot otherwise be documented.

### 2025 Request Attribute Value Update: requester and receiver roles — 2026-10-12

A bounded read of the HLA-facing sequence, API declarations, provider planner,
and focused test bodies confirmed that requester, provider, and receiver are
roles, not necessarily distinct federates. Added a missing evidence-map entry
for one embedded `HLA_EVOKED` case: the requester has a subscription, invokes
the request, and later observes the provider's separate update. This supports
role overlap only in that subscribed scenario. The process class-request case
discovers its target objects but does not check for a later value reflection;
neither test establishes direct response entitlement for an otherwise
ineligible requester. No new diagram was warranted and no normative or
process-parity claim was added. The official IEEE listing provides purchase
and subscription access but not §6.21 clause text. No C++ edit, build, test,
CI, 2010, or Requirements Lab work occurred in this documentation pass.

Next handoff: verify the §6.21 requester-entitlement question against an
authorized full-text copy. If unavailable, keep the portable outcome open and
continue with another bounded HLA-first reader gap; preserve 2010/2025 and
embedded/process boundaries, and do not build or rerun tests for documentation
work.

### 2025 DataElement encoding guide: representation boundaries — 2026-10-12

A bounded HLA-first review of the existing 2025 encoding guide, pinned
`DataElement` API, and its linked focused-test map found no reader gap that
needs another umbrella diagram. The guide already distinguishes HLA value
octets from typed-handle encoding, private process framing, and model input;
then explains the selected element's explicit byte order, composite child
placement/cursor rules, variant discriminants, and factory-defined logical
time. Its source and test links keep the implementation evidence separate from
the portable explanation. A new general encoding chart would duplicate those
flows, so no diagram or guide change was warranted. This survey did not merge
the separate 2010 encoding stream or claim cross-edition parity; no renderer
was needed because no chart changed. No C++ build, test, CI, or Requirements
Lab work occurred.

Next handoff remains the authorized-text check for 2025 §6.21 requester
entitlement. If the full clause is unavailable, keep that outcome open and
continue with another bounded HLA-first reader gap. Preserve 2010/2025 and
embedded/process boundaries; do not build or rerun tests for documentation
work.

### 2025 DDM: pending range query versus committed routing scope — 2026-10-12

Refined the existing region-template state diagram to show an important
two-view distinction in the embedded profile: `getRangeBounds` can expose a
pending edit while regional matching continues to use the previous committed
specification. Added nearby evidence links for the focused lifecycle case and
the separate discovery-after-commit case. The tests observe the query values
and the post-commit discovery, but do not send a value between edit and commit;
continued use of the prior committed range in that interval is explicitly
labeled source-derived from the region matcher. This updates an existing chart,
so the diagram count is unchanged. It has no fresh syntax, render, screenshot,
or GitHub verdict. No C++ edit, build, test, CI, 2010, or Requirements Lab work.

Next handoff remains the authorized-text check for 2025 §6.21 requester
entitlement. If that clause is unavailable, continue a bounded HLA-first reader
audit elsewhere; preserve 2010/2025 and embedded/process boundaries and keep
documentation work off the C++ build/test path.

### Goal focus: portable HLA behavior first — 2026-10-12

Refined the ongoing flow-guide objective: lead with the durable HLA model—its
states, events, decisions, and obligations—and use Umbra source plus existing
focused tests to explain difficult semantics or bound what the implementation
currently demonstrates. Keep implementation-only mechanics visibly separate;
do not turn internal code structure into the primary diagram. Prefer recorded
evidence and do not compile, rerun tests, or start CI for documentation work.
Preserve the 2010 and 2025 compatibility streams as separate boundaries.

The deferred update-region association filesystem case remains a useful
implementation-specific save/restore example, but it does not alone establish
a portable HLA rule, so it is not promoted as the next conceptual flow. The
next handoff remains the authorized-text check for the 2025 §6.21 requester
entitlement question; if that text is unavailable, choose another bounded
HLA-first reader gap. No diagram or C++ behavior changed in this scope update.

### HLA guide local-link integrity check — 2026-10-12

Scanned the 43 Markdown files under `docs/design` for local Markdown link
destinations. All 2,498 relative file targets exist. This is a path-existence
check only; heading and line fragments, Mermaid parsing/rendering, visual
layout, and GitHub rendering remain separate validation work. A fresh fence
census still matches the roadmap at 255 diagrams in 34 files (147 flowcharts,
85 sequences, 23 state diagrams); no inventory correction was needed.

The §6.21 requester-entitlement question remains open pending authorized
normative text; the official public listing/API archive does not provide that
clause. Next handoff: validate heading fragments for local Markdown links
across `docs/design` (the 2,498-target scan checked file paths, not anchors),
then continue a bounded HLA-first reader-gap survey and change a guide only
where the portable concept is not already clearly covered. Keep
implementation-only behavior, 2010, 2025, embedded/process profiles, and the
Requirements Lab within their stated boundaries. No C++ change, build, test,
or CI was performed.

### HLA guide heading-fragment audit — 2026-10-12

Checked 179 local Markdown heading fragments referenced from the 43 files in
`docs/design`, resolving destinations across `docs`. The audit found one stale
anchor: the object-information guide linked to `normative-source-access`, but
the target heading includes its date and therefore has a dated heading slug.
Updated the link; the follow-up audit found zero unresolved heading fragments.
The audit does not validate source line anchors, Mermaid syntax/rendering,
visual layout, or GitHub's live renderer. No diagram or C++ behavior changed.

Next handoff: keep the 2025 §6.21 requester-entitlement question open pending
authorized normative text; continue a bounded HLA-first reader-gap survey and
change a guide only when it resolves a real gap. Preserve 2010/2025 and
embedded/process boundaries; do not build, rerun tests, start CI, or edit the
Requirements Lab for documentation work.

### 2025 active relevance and passive receiver survey — 2026-10-12

Rechecked the subscription-state guide against the pinned §5.8 signature, the
single-publisher/single-subscriber passive test, and the shared 2025 discovery
and update-recipient source. The existing guide already separates federation-
wide active relevance from each receiver's own active/passive eligibility and
labels the multi-member behavior as source-derived. The cited test does not
cover a separate active establisher plus passive receiver, and the local API
header does not contain the full normative clause. Because the existing HLA-
first and source-derived diagrams already explain the distinction and its
evidence limits, no duplicate diagram was added. Citation verification also
found and corrected a stale discovery-resolver source line link (855 to 869).
Their visual review remains pending. No 2010 behavior was inferred; no code, build, test, CI, or
Requirements Lab work occurred.

Next handoff: visually review the passive-recipient and HLA-first late-
subscription diagrams with a local Markdown renderer. If `mmdc` is needed,
use Scoop only if a usable manifest is available; then resume the bounded
HLA-first topic survey. Keep §6.21 unresolved without authorized text and
preserve edition/profile boundaries.

### Design-doc source/test line-link range audit — 2026-10-12

Checked 1,758 local `#L...` source/test links from the 43 Markdown files in
`docs/design` against the current worktree. Every target file exists and every
linked line is within its target file. This range check does not prove that a
line still describes the cited symbol: the earlier discovery-resolver link was
in-range but stale, so it was corrected separately. Network-enabled Scoop
searches for both `mmdc` and `mermaid` found no usable manifest; no package was
installed. Visual review of the passive-recipient and late-subscription charts
is still pending. No diagram or C++ behavior changed; no build, tests, or CI.

Next handoff: try the existing local Markdown preview path for those two
subscription-state diagrams when a supported preview surface is available. If
not, keep visual status pending and continue a bounded HLA-first survey from
the prioritized backlog; leave §6.21 open without authorized text and keep
2010/2025 and embedded/process boundaries separate.

### HLA-first documentation focus and preview restriction — 2026-10-12

Reconfirmed the goal's reader-first scope: explain portable HLA roles, states,
events, eligibility decisions, and observable outcomes, using Umbra source and
existing focused tests as bounded evidence rather than making C++ architecture
the primary model. The subscription guide already separates federation-level
active relevance from receiver-local active/passive eligibility, and keeps the
known-object/new-attribute value question open; another diagram would duplicate
that explanation. No C++ builds, tests, or CI were run.

The allowed Scoop search found no usable `mmdc` manifest. A local in-app
preview attempt was blocked because `data:` URLs are not supported by the
browser policy. I did not try a different browser surface or route around that
restriction. Visual review of the passive-recipient and late-subscription
diagrams remains pending; no chart was changed or rendered.

Next handoff: continue a bounded, content-first HLA reader-gap survey from the
prioritized backlog, making a guide change only for a real conceptual gap.
Keep the 2025 §6.21 requester-entitlement interpretation unresolved without
authorized normative text, preserve 2010/2025 and embedded/process boundaries,
and keep builds, tests, CI, and Requirements Lab edits out of documentation
work.

### Current HLA guide link and class-ancestry audit — 2026-10-12

Rechecked 1,795 inline local source/test line links across the 43 Markdown
files in `docs/design`. Every target file exists and every referenced line is
within bounds. This is only a range/path check, not proof that each line still
describes the linked behavior. A bounded HLA-first review of class ancestry
also checked the current guide's selected discovery, attribute-projection,
ordinary-interaction-promotion, and directed-target lookup distinctions
against their cited source and test examples. The guide keeps them as
path-specific 2025 observations, distinguishes focused test evidence from
source-only edges, and does not claim one universal inheritance rule. No new
diagram or guide edit was warranted. No C++ build, tests, CI, 2010 inference,
or Requirements Lab edit occurred.

Next handoff: continue with one specific reader gap from the prioritized
backlog; avoid umbrella duplication and keep the HLA behavior model primary.
Visual review of the passive-recipient and late-subscription diagrams remains
pending until a supported local preview is available. Keep §6.21 unresolved
without authorized clause text and preserve the separate edition/profile
boundaries.

### HLA report and identity boundary audit — 2026-10-12

A bounded HLA-first survey compared the existing 2025 TSO retraction,
attribute-value request, federation MOM, service-invocation reporting,
exception-reporting, and name/handle guides. Their current maps already keep
the portable distinctions visible: producer acceptance versus recipient
outcome; provider solicitation versus a later value-bearing update; the
execution-scoped `HLAfederation` object versus member-scoped state; a public
service result versus requested MOM report delivery and invocation reporting;
and public typed/opaque handles versus Umbra's private numeric representation.
The existing explanations also label their embedded/process evidence limits.
No new diagram or guide edit was warranted, and unresolved 2025 §§6.21 and
8.22.3 questions remain open without authorized normative text.

Network-enabled `scoop search mmdc` and `scoop search mermaid` returned no
matches, so no renderer was installed and no chart was rendered. The passive-
recipient and late-subscription visual reviews remain pending; no alternate
browser route was attempted. No C++ build, test, CI, 2010, or Requirements Lab
work occurred.

Next handoff: compare the 2025 federation-scoped MOM and joined-federate MOM
guides around execution-level versus membership-level state, observer
knowledge/value paths, and resignation effects. Keep the HLA model primary,
consult the pinned 2025 MIM/API and existing bounded evidence, preserve
embedded/process and 2010 boundaries, and add or revise a diagram only for a
demonstrated learner gap. Keep the licensed-text questions and pending visual
review explicit; do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 Federation MOM execution/member observation map — 2026-10-12

The guide comparison exposed one synthesis gap: the Federation and joined-
federate guides explained their projections separately but did not put their
relationship and observer-side paths in one HLA-first view. Added a scope map
showing the MIM's one-per-execution `HLAfederation` projection, conditional
`HLAfederatesInFederation` reference-list value, and one-per-member
`HLAfederate` projection. It distinguishes list reflection from member-object
discovery/removal and explicitly makes no cross-path callback-order claim.
The embedded focused cases observe membership-list reflection and member
object removal separately; a process case orders removal before rediscovery/
reflection of that member object but does not cover the list reflection. These
tests were inspected, not run; there is no embedded/process parity claim. The
2025 joined-federate guide now links to the map. The new chart has no parser or
render verdict because `mmdc` is unavailable; the current inventory is 256
diagrams (148 flowcharts, 85 sequences, 23 states). No build, tests, CI, 2010,
or Requirements Lab work occurred.

Next handoff: audit whether a receiver of `HLAfederatesInFederation` may treat
an `HLAfederateReference`/`HLAfederateHandle` as knowledge of that member's
`HLAfederate` object. Compare the pinned 2025 MIM type with the existing
name/handle and joined-federate guides and only bounded evidence. Keep
referential identity separate from discovery/object knowledge; add text or a
diagram only if a concrete learner gap remains. Preserve 2010 and embedded/
process boundaries; leave §6.21 and §8.22.3 open, and do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 MOM reference identity versus receiver object knowledge — 2026-10-12

Audited whether a member reference in `HLAfederatesInFederation` implies that
the receiving federate knows the corresponding `HLAfederate` object. The
pinned MIM says the reference is represented by a `FederateHandle` and points
to the object's `HLAfederateHandle` attribute; it does not describe the
receiver's discovery state. The pinned API's object-instance support lookups
take `ObjectInstanceHandle` values and can report `ObjectInstanceNotKnown`.
Thus the list provides federate identity, not evidence that a particular
receiver has the member object's public instance handle/knowledge. This is a
typed API and receiver-scope distinction, not a newly asserted callback rule.

The membership test decodes the list and checks its federate handles but does
not test support lookups for the referenced member object. A separate joined-
federate MOM case obtains the object-instance handle through discovery and
then checks support lookups. Added a bounded explanation and reciprocal links
between the Federation MOM and name/handle guides; no duplicate diagram was
needed. No test was run, no 2010 behavior was inferred, and the existing MOM
scope map still lacks a fresh parser/render verdict.

Next handoff: audit the 2025 Federation MOM newcomer path. Compare the MIM's
`Conditional` `HLAfederatesInFederation` update-on-Join/Resign semantics with
the explicit initial value request in the focused membership test and the
current Federation MOM guide. Separate the current membership value from
automatic Join/discovery callbacks; document any gap only with a pinned
contract or bounded evidence. Preserve 2010 and embedded/process boundaries;
do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Federation MOM late-observer membership value — 2026-10-12

The Federation MOM guide already showed subscribe → discovery → current-value
request → later conditional Join/Resign reflections, but did not state how that
path applies to a federate joining an already populated execution. The pinned
API requires execution membership for object-class subscription; the MIM marks
the member list Conditional on Join/Resign and separately requires a response
to Request Attribute Value Update for Conditional attributes. The evidence
does not establish that an earlier conditional update is replayed to a member
that subscribes after its own Join. Added an explicit late-observer note: after
discovery, request the current list rather than assuming discovery supplied a
snapshot. It expressly avoids asserting that no implementation can deliver an
extra reflection.

The existing focused test is observer-first: it subscribes, requests the
initial current list, and then observes a second member's Join/Resign updates.
It does not test the late-observer case. No new diagram was needed because the
existing sequence already shows the affirmative path. No test, build, CI, 2010,
or Requirements Lab work occurred; the diagram inventory remains 256 and its
new MOM map still has no parser/render verdict.

Next handoff: audit the Federation MOM's conditional FOM-composition values
(`HLAFOMmoduleDesignatorList` and `HLAcurrentFDD`) against the 2025 FOM module
admission/composition guide. Compare their MIM update triggers with what a
joining member can resolve from its effective FOM and what a current-value
request returns. Keep model declarations separate from MOM projections and
request/report interactions; add a cross-link or diagram only if a concrete
reader gap remains. Preserve 2010 and embedded/process boundaries; do not
build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Federation MOM effective FOM and module-data views — 2026-10-12

Compared the current Federation MOM guide with the FOM module
admission/composition guide, pinned 2025 MIM rows, and focused test sources.
The MIM uses `HLAFOMmoduleDesignatorList` on two different object scopes: the
federation object's list is Conditional on FOM additions through Create/Join,
while each joined-federate object's same-named list is Static and contains
modules supplied for that member's Join. `HLAcurrentFDD` is the federation's
Conditional current-FDD value. The HLAfederation class semantics separately
require a current-value response to Request Attribute Value Update regardless
of update type. The embedded current-FDD test observes a changed reflection
after an additional-module Join and verifies that a subsequent explicit
request returns the same value. A separate module-data report is a
receiveInteraction carrying selected module content, not the composed FDD.

Added a scope-comparison table in the Federation MOM guide and reciprocal
cross-links to the FOM guide. Existing diagrams already show the distinct
composition, attribute-reflection, and report paths, so no duplicate diagram
was added. The per-member snapshot test uses a private inspection seam and is
identified as such. A separate public member-object lifecycle test verifies
that the reflected member list matches its Join-supplied module. Neither test
proves the public federation-wide list update.
No tests were rerun, no build or CI was started, and the 2010 stream and
Requirements Lab were untouched. The diagram inventory remains 256; no new
parser/render verdict is claimed.

Next handoff: check the bounded 2025 tests and source for a public observer
assertion that the federation-scoped `HLAFOMmoduleDesignatorList` changes
after Create/Join. Keep that distinct from the per-member Static list,
`HLAcurrentFDD`, and module-data request/report interactions. Do not build,
rerun tests, start CI, or edit the Requirements Lab; preserve 2010 and
embedded/process boundaries.

## 2025 Federation MOM federation-list update evidence — 2026-10-12

The pinned MIM makes the federation object's `HLAFOMmoduleDesignatorList`
Conditional on new FOM modules supplied at Create or Join; the same-named
`HLAfederate` value is Static and records only that member's Join-supplied
modules. In the selected embedded source, the federation attribute is derived
from the current federation definition, filters out the standard MIM, and
retains the first designator for each canonical module-source identity. The
additional-module Join path queues Conditional updates for this list and
`HLAcurrentFDD`. This is source evidence for that embedded Join path, not
evidence of a Create update path or process parity.

A bounded repository-wide test-source search found public assertions for the
member-scoped list and a separate assertion for changed `HLAcurrentFDD`, but no
focused public observer assertion for the federation-wide list changing after
Create/Join. Neither adjacent test closes that gap: the public lifecycle test
observes the `HLAfederate` object, while the current-FDD test asserts only the
FDD attribute. Existing diagrams already distinguish composition, attribute
reflection, and module-data reporting; no new chart was warranted. Diagram
inventory remains 256 and the existing MOM-map render status is unchanged.
Tests were inspected, not rerun. No build, CI, 2010, or Requirements Lab work.

Next handoff: compare the 2025 MIM's repeated-identical-module first-designator
rule with the embedded list encoder's canonical-source de-duplication, then
check whether focused tests assert that behavior separately for federation
and member lists. Keep object scopes and 2010/process boundaries explicit; do
not build, rerun tests, start CI, or edit the Requirements Lab.

## 2025 Federation MOM duplicate-designator scope — 2026-10-12

The 2025 MIM's first-designator rule applies to both the federation-level
Conditional list and member-level Static list. The embedded implementation
uses the same list encoder for both projections and de-duplicates by canonical
`sourcePath`, retaining the first supplied designator. A private registry test
passes two joined-member module records with a shared canonical source path and
different designators, then asserts a one-entry member snapshot containing
the first designator. This tests the private member projection, not public
Join behavior. The public lifecycle test verifies only the one-module member
list, and no federation-level list assertion was found. Existing composition
tests for equivalent module declarations do not assert MOM designator-list
output. Whether byte-identical module content at distinct canonical paths is
“identical” for the MIM rule remains unresolved; no conformance gap is claimed.

No diagram changed; this source/test distinction is explanatory and the
current flows already separate the two MOM scopes. The 256-diagram inventory
and pending MOM-map render status are unchanged. No build, test rerun, CI,
2010, or Requirements Lab work occurred.

Next handoff: inspect the pinned 2025 standard's definition and examples for
what makes two FOM modules “identical,” then compare that meaning with Umbra's
canonical-path identity. Treat same-content files at distinct paths as an open
semantic question until the normative text answers it; preserve 2010 and
embedded/process boundaries and do not build or rerun tests.

## 2025 Federation MOM module-identity evidence boundary — 2026-10-12

The MIM's first-designator rule is explicit, but it does not define how two
FOM modules are judged identical. The checked-in 1516.2-2025 package has the
MIM, schemas, and Restaurant examples; it does not include the full OMT prose
needed to check whether the standard supplies a more specific identity test
for byte-identical modules at distinct canonical paths. IEEE SA lists the full
standard through purchase or subscription, separately from its additional
resources
([publication and access](https://standards.ieee.org/ieee/1516.2/6689/)).
Umbra's canonical-`sourcePath` de-duplication remains a source-level behavior,
not a normative definition. The question stays open without a conformance
finding. No diagram changed; inventory remains 256 and MOM-map parser/render
status is unchanged. No build, test rerun, CI, 2010, or Requirements Lab work.

Next handoff: audit the 2025 object-deletion/attribute-ownership boundary
across the object-information and ownership guides. Compare portable service
and callback concepts with exact focused test evidence; keep accepted
federation deletion, ownership cleanup, and receiver-local forgetting
distinct. Revise a diagram only if the survey identifies a genuine reader
gap. Preserve 2010 and embedded/process boundaries; no builds, test reruns,
CI, or Requirements Lab edits.

## 2025 Object deletion and ownership-cleanup wayfinding — 2026-10-12

The existing cross-cutting map distinguishes federation-wide delete from
federate-local forgetting. The object guide gives the `deleteObjectInstance`,
`removeObjectInstance`, and `localDeleteObjectInstance` API roles and the
current receiver-removal gate; the ownership companion covers accepted-delete
ledger cleanup and callback-time checks. Its query, regular release-request,
If-Available, and cancellation branches have separate focused HLA_EVOKED test
scenarios, while If-Wanted, negotiated divestiture, and Confirm Divestiture
remain source-observed after deletion. Local forget is separately gated when
relevant ownership work is pending; the cited process local-delete test covers
success only. No additional state machine was warranted. Added a direct link
from the object-removal guide to the ownership cleanup section. No diagram
changed; inventory remains 256, with no new parser/render result. No build,
test rerun, CI, 2010, or Requirements Lab work.

Next handoff: audit the 2025 ResignAction boundary across federation
lifecycle, object-information, and ownership guides. Compare portable
service/callback concepts with focused evidence; keep membership departure,
object deletion/removal, and attribute divestiture as distinct effects. Revise
a flow only if a genuine reader gap appears. Preserve the separate 2010
reference-RTI path and embedded/process boundaries; do not build, rerun tests,
start CI, or edit the Requirements Lab.

## 2025 ResignAction cross-guide boundary — 2026-10-12

The current guides separate the action vocabulary, successful member departure,
execution lifetime, object deletion/removal, and per-attribute divestiture.
The lifecycle guide's embedded table and sequence give the action-level
overview; the ownership companion carries the pending-acquisition role scan
and selected cancellation evidence; the object guide covers delete privilege
and per-receiver removal. The focused embedded scenarios inspected here show
pending-acquisition rejection, `NO_ACTION` rejection while attributes remain
owned, `DELETE_OBJECTS_THEN_DIVEST` producing separate removal and assumption
callbacks, and final-member object cleanup without destroying the execution.
The assumption-before-removal order is only one `HLA_EVOKED` test observation.
The selected process case covers `NO_ACTION` MOM-object lifecycle, not the
action matrix. Normative guard/effect details and callback ordering remain
unverified without the licensed standard. Added a direct link from the core
ownership guide to the lifecycle action table; a combined diagram would
duplicate separate service and callback state machines. Inventory remains
256; no diagram, test, build, CI, 2010, or Requirements Lab work.

## 2025 Connection-loss resignation: directive versus forced cleanup — 2026-10-12

The lifecycle guide's sequence and prose identified the per-member automatic
directive and forced cleanup, but left the boundary between the configured
action and forced-loss overrides too implicit. Added a concise implementation
map separating the stored action, forced acquisition cancellation/owned-
attribute divestiture, and the last-member delete pass. Explicitly stated that
transport loss is not an application `resignFederationExecution(action)` call.
The automatic directive remains membership-local and is seeded from the
composed FDD; the current registry reads that member value on connection loss.

The focused embedded `NO_ACTION` case shows a retained object, an ownership-
assumption callback, and no removal callback when owned attributes leave the
departing member. The pending-acquisition loss case uses
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, so it does not alone prove that the
forced cancellation overlay is independent of the directive; that independence
is source-observed in the shared registry transition. The process entry routes
endpoint loss through that registry and event projection. Adapter-managed
process TCK scenario sources cover configured unconditional divestiture and
configured acquisition cancellation, but not the process forced overlay under
`NO_ACTION`. These sources were inspected, not run. No process/embedded parity,
callback-order guarantee, full normative action matrix, or 2010 equivalence is
claimed.

No diagram changed, so the inventory remains 256 and Mermaid render status is
unchanged. No build, test rerun, CI, 2010 change, or Requirements Lab work.

Next handoff: audit 2025 connection loss during active Federation Save or
Federation Restore. Compare the forced-loss entry with the shared save/restore
failure transitions and available embedded/process evidence; keep connection
loss distinct from voluntary resignation, label untested profile behavior,
and add a flow only for a concrete reader gap. Preserve 2010 separately; do
not build, rerun tests, start CI, or edit the Requirements Lab.

## 2025 Connection loss during active Save/Restore — 2026-10-12

Compared forced `connectionLost` and voluntary process `handleResign` with
the shared 2025 registry transition. The registry fails an active Save with
`FEDERATE_RESIGNED_DURING_SAVE` or an active Restore with
`FEDERATE_RESIGNED_DURING_RESTORE`, excludes the departing participant and
clears the operation before membership removal. Save completion additionally
skips participants still at `NO_SAVE_IN_PROGRESS` and missing callback routes;
Restore skips the departing or unrouteable member. The automatic ResignAction
does not select this operation failure.

The embedded membership-loss adapter extracts and submits both notification
vectors. The process loss path and voluntary resign path both pass results to
`dispatchConnectionLossResult`, whose current source handles ownership work,
object removals, synchronization notifications, and an eligible federate-lost
report but not Save/Restore notifications. The process profile therefore has
a source-observed result-projection gap for either member-departure entry
path; runtime behavior remains unverified. Existing save/restore reason-code
tests exercise voluntary embedded resignation, and no focused connection-loss
during-active-save/restore test was found among the loss-focused Catch2 cases
or the process connection-lifecycle TCK source. Sources were inspected, not
run. No 2010 rule or profile parity is inferred.

Updated the lifecycle guide and added a reciprocal link from Save/Restore.
No new flow was justified: a single diagram would blur the embedded callback
submission and process dispatcher omission. The current inventory is 256
Mermaid diagrams across 34 diagram-bearing docs/design files, counting both
backtick and tilde fences. No Mermaid changed, so prior render state is
unchanged. No build, test rerun, CI,
2010, or Requirements Lab work.

Next handoff: audit 2025 forced connection loss during an active
synchronization point, focusing on participant removal, success/failure
notification eligibility, embedded/process event projection, and available
focused evidence. Change only the guide(s) with a concrete reader gap; keep
2010 separate and do not build, rerun tests, start CI, or edit the Requirements
Lab.

### 2025 Forced connection loss during a pending synchronization point — 2026-10-12

The synchronization guide now distinguishes the portable barrier vocabulary
from Umbra's 2025 forced-loss transition. The registry removes the departed
member from the participant, announced, and achieved sets before checking
completion; it does not add that member to the failed-to-synchronize set. The
point remains pending if a remaining member has not achieved, closes when all
remaining members have achieved, and is erased without a notification when no
participants remain. Notifications are planned only for remaining participants
with usable callback routes.

The embedded membership-loss adapter submits the resulting synchronization
notifications. The process connection-loss handler passes the registry result
through a dispatcher that enqueues those notifications. The bounded scan found
no focused forced-loss-during-active-synchronization case; the existing
two-member HLA_EVOKED resignation test covers voluntary departure only. This is
source-observed 2025 behavior, not a normative transport-loss rule or 2010
claim. The existing barrier chart already covers the closure transition, so no
duplicate diagram was added.

The complete inventory is 256 Mermaid fences across 34 design files: 250
backtick-fenced and six tilde-fenced. No diagram changed or was rendered. No
build, test rerun, CI, 2010, or Requirements Lab work occurred.

Next handoff: audit 2025 delivery of a planned FederationSynchronized callback
when its recipient resigns or loses connection before application observation.
Compare embedded pending-callback cancellation/staleness behavior with process
queued-event handling and focused tests; document only a concrete reader gap.
Keep 2010 separate, do not infer profile parity, and do not build, rerun tests,
start CI, or edit the Requirements Lab.

### 2025 FederationSynchronized recipient departure before observation — 2026-10-12

Compared the completed synchronization callback's lifetime across the 2025
embedded and process paths. Embedded Resign and forced connection loss reset
pending tasks on the departing ambassador's callback dispatcher; reset does
not undo an invocation already claimed or entered. Process Resign clears the
server's joined-session synchronization event queue, but it does not clear
events already buffered in the client or close the callback bridge. The bridge
can therefore retain work for later Evoke/Enable dispatch after Resign; client
close clears the buffered and dispatched queues. A frame still in transit on
connection loss remains outside this source/test review.

The request ordering sharpens this boundary: the client dispatches buffered
events after matching a Resign response but before the Resign method clears its
local joined identity. An enabled HLA_IMMEDIATE callback may run inline while
the API call is still in progress; HLA_EVOKED or disabled-immediate work can
remain queued for later dispatch. This timing is source-derived, and the
focused cases do not exercise it for FederationSynchronized.

Focused tests cover a survivor receiving completion because another member
resigns, plus ordinary process callback delivery before the tested members
later resign. None asserts delivery or suppression after the callback
recipient's own resignation or connection loss. This is source-observed 2025
implementation behavior, not a portable post-Resign rule, runtime proof, or
2010/process parity claim. The callback guide now carries the comparison and
the synchronization guide links to it. Existing diagrams already cover
barrier closure and callback dispatch separately; no new chart was warranted.
The inventory remains 256 Mermaid fences across 34 docs/design files. No
diagram changed or was rendered. No build, test rerun, CI, 2010, or Requirements
Lab work occurred.

### 2025 Process object-instance discovery after recipient Resign — 2026-10-12

Audited the 2025 process object-discovery event after recipient Resign. The
server's joined-session cleanup clears discovery events that remain in its
queue. For events already received by the client or submitted to its bridge,
the bridge submission checks only its open session/dispatcher; the discovery
delivery routine validates payload identities and name, but does not recheck
current federation membership or current registry object state. This leaves
an event-specific source path through the generic process post-Resign callback
queue, not proof of a callback after Resign or a normative HLA rule.

The public process object-discovery test runs both HLA_EVOKED and
HLA_IMMEDIATE, asserts the discovered handle/class/name/producer, and only
then resigns sender and receiver. It does not test discovery pending across
the recipient's Resign. A separate process failed-Join scenario discards a
new MOM discovery queued for a failed membership; that rollback is a
different transition and not resignation evidence. The callback guide now
records this distinction, and the object/interaction guide links to it. No
new diagram was justified because the existing post-Resign flow already maps
the shared queue gate; no diagram changed or was rendered. Inventory remains
256 Mermaid fences across 34 docs/design files. No build, test rerun, CI, 2010,
or Requirements Lab work.

Next handoff: audit 2025 process DELETE_OBJECTS resignation effects for a
surviving receiver. Trace per-receiver known-object cleanup and
removeObjectInstance callback gating/order against the focused process
resignation/deletion cases; distinguish producer Resign from the recipient's
own Resign and document only proven process-profile behavior. Keep embedded
and 2010 paths separate; no builds, test reruns, CI, or Requirements Lab work.

### 2025 Process `DELETE_OBJECTS` resignation to a surviving receiver — 2026-10-12

Audited the process Resign handler, registry per-recipient removal planning,
pull/push commit gates, receiver session cleanup, and callback bridge. The
object guide now distinguishes producer Resign from recipient Resign and
includes one source-derived flowchart. The portable C++ TCK scenario expects a
survivor callback followed by unknown-object lookups; the focused Umbra process
deletion test covers direct deletion, not the Resign trigger. Removal events
still in a receiver's server queue are cleared by its own Resign; outcomes for
already-pushed callback work are unresolved. No process test was run, no 2010
or embedded parity is claimed, and no Requirements Lab work occurred. The new
chart has no fresh syntax/render verdict.

Next handoff: audit 2025 process `DELETE_OBJECTS_THEN_DIVEST` for a surviving
member when the departing producer has both a delete-privileged object and
owned attributes. Trace object-removal and ownership-assumption planning,
recipient gates, and callback ordering across process source and the portable
TCK/selected embedded tests; label every profile-specific or untested edge.
Keep 2010 separate; no builds, test reruns, CI, or Requirements Lab work.

### 2025 process `DELETE_OBJECTS_THEN_DIVEST` callback ordering — 2026-10-12

Audited the registry's delete-then-divest passes, the embedded voluntary
scenario, the portable C++ TCK cross-object case, and process result
dispatch/polling. The embedded HLA_EVOKED test observes assumption before
removal. Process dispatch queues assumptions before removals, but default-pull
receive selection prioritizes the removal queue; optional push submits the
assumption frame first. No focused process test establishes callback order.
The portable TCK checks deleted versus retained object state and later
If-Available acquisition, not the assumption callback or its ordering. Added a
second source-derived flowchart; both process charts remain unrendered and
unparsed. The shared roadmap had a missing comma before an unrelated
`active_handoff` field; repaired only that separator so the JSON parses. No
build, test rerun, CI, 2010, or Requirements Lab work.

Next handoff: audit 2025 process receiver Resign after producer
`DELETE_OBJECTS_THEN_DIVEST` when ownership-assumption and object-removal work
are pending. Trace registry callback-ledger invalidation and session queue
clearing; distinguish pull-queued work from frames already pushed to the
client, and identify focused evidence. Keep this separate from producer
Resign and the embedded HLA_EVOKED order; no builds, test reruns, CI, 2010, or
Requirements Lab work.

### 2025 process receiver Resign with pending assumption and removal — 2026-10-12

Added a third source-derived process flowchart to the 2025 object/interaction
guide. Registry Resign invalidates the receiver's pending assumption marker
and removal reservation; process dispatch filters queued ownership events for
the departed ID, and joined-session cleanup clears the ownership and removal
event queues for that session before the
successful response. Push has a different boundary: frames read during the
Resign exchange are buffered, then submitted to the callback bridge after the
response. The process branch does not reset the callback dispatcher, unlike
the embedded branch. The nearest focused process ownership test consumes the
assumption before Resign; no focused test asserts the receiver-Resign race for
pending assumption/removal frames. This is source-derived, not a normative or
cross-profile callback guarantee. The chart has no fresh syntax/render review.
No build, test rerun, CI, 2010, or Requirements Lab work.

### 2025 process Request Retraction after receiver Resign — 2026-10-12

Audited the process TSO attribute-update recipient cache, receiver-Resign
cleanup, producer Retract handler, registry recipient ledger, client event
buffer, and callback-bridge admission. Receiver Resign clears the server
session queues but does not prune that attached session from other messages'
retraction-route cache; Detach does. A later producer Retract can therefore
send a `request_retraction` transport frame to a session that has resigned.
The callback bridge separately requires tracked message state and an original
callback-started marker before queuing the public callback; this check is not a
current-membership check. It is a source-supported process route, not a
normative permission or a proven post-Resign callback. The focused process
attribute tests cover retraction before callback observation/fan-out, not a
receiver-Resign race. Added a sequence to the TSO guide and a cross-link from
the callback guide. No test or build was run. Scoop search found no configured
`mmdc` package, so syntax/render review remains pending. Inventory is now 260
diagrams (151 flowcharts, 86 sequences, 23 state diagrams) across 34 files.
No CI, 2010, or Requirements Lab work.

Next handoff: compare 2025 process timestamped-interaction retraction with the
attribute-update route documented here. Trace session-cache insertion,
receiver-Resign cleanup, client buffering, callback-state admission, and
focused process evidence; record any divergence without assuming behavior is
shared just because both use a retraction designator. Keep embedded and 2010
separate; no builds, test reruns, CI, or Requirements Lab work.

### 2025 process timestamped interaction retraction-route comparison — 2026-10-12

Compared ordinary process interaction and attribute retraction paths without
assuming shared behavior from their common designator. For coordinator-
dispatched TSO delivery, both event families reserve registry recipient state,
send the event frame, then cache the session. The send-time branches differ:
timestamped attribute updates cache pull-queued recipients, while ordinary
interactions cache only the pushed branch; the pull interaction path queues an
event without a route insertion in that handler. Their client filters differ
too: Request Retraction removes a still-buffered interaction and defers its
TSO acknowledgement, but leaves a buffered attribute event for the bridge's
per-message `RetractionState` gate. The process registry's `delivered` marker
is earlier than public callback entry; the callback bridge tracks that entry
separately. A bounded search found process interaction evidence for a
coordinator-delivered callback followed by a too-late Retract, but no focused
pre-callback, send-time-pull, or receiver-Resign interaction case. These are
source/test-coverage observations, not an HLA conformance conclusion. Narrowed
the existing TSO sequence to the coordinator-delivered attribute route and
added a comparison table. Mermaid syntax/render review remains pending; Scoop
has no matching `mmdc` or `mermaid-cli` package. No build, test rerun, CI, 2010,
or Requirements Lab work. Diagram inventory remains 260 across 34 files.

Next handoff: audit the 2025 process timestamped object-removal retraction
route against the attribute/interaction comparison. Trace recipient-route
insertion, receiver Resign versus detach, client event filtering, callback
admission, and the focused removal tests. Keep embedded and 2010 evidence
separate; document source/test gaps without inferring a normative result. No
builds, test reruns, CI, or Requirements Lab work.

### 2025 process timestamped-delete retraction-route comparison — 2026-10-12

The process Delete Object Instance path splits a retractable timestamped
deletion's recipients: time-constrained receivers stay in the coordinator TSO
queue, while other planned receivers take a send-time RECEIVE-order removal
path. The coordinator path caches a process session after sending the removal
frame; the send-time path records it while queueing or preparing the event.
Receiver Resign clears its joined identity and server queues but does not prune
that attached session from other messages' retraction routes; Detach does. A
later valid producer Retract can send `request_retraction` without a live-
membership recheck. The client removes a still-buffered removal event and
defers its private acknowledgement; once the event is submitted, the callback
bridge's per-message state suppresses an unstarted pair or admits Request
Retraction after the original callback has started. A missing bridge state
produces no public Request Retraction callback. This is a source trace, not a
post-Resign runtime result or normative permission.

The focused process delete test covers callback-model, regulation,
time-constrained, preferred-order, and callback-gating scenarios and checks
whether a public handle is exposed; it does not call Retract or test receiver
Resign after route insertion. The separate embedded delete-retraction case
does not establish process parity. No test or build was run. Added one
sequence to the TSO guide; it has no fresh syntax/render verdict. Inventory is
261 Mermaid fences across 34 files (151 flowcharts, 87 sequences, 23 state
diagrams). No CI, 2010, or Requirements Lab work.

### 2025 process Disconnect versus server Detach — 2026-10-12

Expanded the existing connection guide's Disconnect flow, not the membership
state map. It now separates the adapter `CallbackSession` from the process
callback bridge and the local `ProcessFederationClient::close` from
server-side `ProcessFederationService::detach`. The public process path closes
the client socket; there is no Disconnect service message in the protocol
operation enum. The service header assigns Detach to a transport owner after
its session loop exits, but a bounded search found no production callsite in
this checkout. The process-endpoint lifecycle test calls Detach explicitly in
its server fixture after the planned requests, so it does not prove API-caused
or synchronous server cleanup. This remains an Umbra implementation/test
boundary, not an HLA rule or 2010 claim. No C++ source, build, test, CI, or
Requirements Lab work occurred. The revised diagram does not change the 261
diagram count and has no fresh syntax/render verdict.

Next handoff: survey the 2025 data-distribution and regions guide with its
object/interaction companions. Check whether update-region association,
subscription-region, and interaction-region decisions are clearly distinct
recipient-eligibility paths and remain separate from logical-time delivery.
Use bounded source/test evidence; do not infer DDM overlap rules for directed
interactions. Revise an existing flow or add one only for a concrete reader
gap. Keep 2010 separate; no builds, tests, CI, or Requirements Lab edits.

### 2025 DDM recipient-path survey — 2026-10-12

Compared the DDM, object/interaction, relevance-advisory, and
directed-interaction guides with the pinned 2025 API, selected registry paths,
and focused test sources. Existing flows distinguish per-attribute object
source-region association from receiver subscription, ordinary interaction
explicit/default-region selection from object routing, and DDM eligibility
from logical-time/callback gates. The focused ordinary interaction case
supports the existing effective-regional-subscription behavior and empty
regional-send no-op; tests were inspected, not run.

The source also confirms a separate publisher-side rule: an active regional
declaration can establish class-level relevance without an object instance or
overlap result. That advisory does not imply delivery. Directed sends identify
a target object and use a receiver selector; the API shape and guide keep them
separate from region-set routing. The source's regional declaration check is
not evidence for a directed-delivery overlap rule, which remains unclaimed.
Existing guides suffice, so no chart or diagram count changed (261). No build,
test rerun, CI, 2010, or Requirements Lab work.

Next handoff: survey the 2025 Federation and Federate Lifecycle guide alongside
the updated Disconnect/Detach flow. Compare Create, Join, Resign, Destroy,
connection loss, final-member behavior, and member-scoped automatic-resign
outcomes. Keep member-visible HLA state distinct from Umbra teardown evidence;
revise a flow only for a concrete gap. Keep 2010 separate and avoid builds,
tests, CI, and Requirements Lab edits.

### 2025 federation lifecycle versus connection teardown — 2026-10-12

Audited the current 2025 lifecycle and connection guides together. They already
separate ambassador connection, federate membership, and execution lifetime;
voluntary Resign, unjoined-only Disconnect, and RTI-forced connection-loss
cleanup are not conflated. Selected embedded tests show the final-member
cleanup path on both resignation and transport loss, while the federation
execution remains available until explicit Destroy. The member's automatic
directive is distinct from the forced-loss safety overlay. The revised
connection chart separately treats process-client close and conditional
server-side Detach; no API-caused Detach or process/embedded parity is claimed.
Save/restore, synchronization, and TSO consequences remain in their dedicated
guides. The source and focused test bodies were inspected, not run. No content
or diagram edit was warranted; no build, tests, CI, 2010, or Requirements Lab
work. Existing diagram count remains 261.

### 2025 DDM and saved Region state — 2026-10-12

Reviewed the DDM/regions guide with the object/interaction and Save/Restore
companions. Existing maps already separate object-attribute update-region
associations, regional receiver subscriptions, ordinary interaction source
regions, directed target selection, regional provider solicitation, and time
eligibility. No general routing diagram was added. A cross-service gap did
remain: the DDM guide explained the live committed Region object, and
Save/Restore already showed a queued TSO message's source-region metadata, but
there was no direct Region-object restore trace.

Added a focused 2025 embedded `HLA_IMMEDIATE` sequence to Save/Restore and a
reciprocal DDM link. The public API case deletes a committed unassociated
Region after Save, restores, and queries the same handle, dimension set, and
committed bounds. Source shows the image also stores owner, dimensions, pending
and committed bounds, and flags; the restore helper requires a joined owner
and validates the specification against the restored catalog. Pending edits,
active associations, process endpoint, host restart, and 2010 remain outside
the test claim. The same image carries a separate unachieved synchronization
point; that state remains in its own guide. The new Mermaid sequence has no
fresh syntax or visual verdict, and GitHub rendering remains unverified. No
C++ source, builds, tests, CI, or Requirements Lab changes.

Next handoff: inspect bounded 2025 process-endpoint and fresh-registry restore
tests for direct evidence about standalone Region handles/ranges and active
update-region associations. Compare with the embedded case and queued TSO
source-region snapshots; if process coverage is absent, keep it explicitly
unverified instead of claiming parity. Keep 2010 separate; no builds, tests,
CI, or Requirements Lab edits.

### 2025 Region association and queued attribute TSO restore — 2026-10-12

The bounded follow-up found two separate fresh-embedded-registry cases. The
existing one-member `HLA_IMMEDIATE` test directly queries a committed,
unassociated Region after deleting it and restoring. A second public case runs
`HLA_EVOKED` and `HLA_IMMEDIATE`: it associates an object attribute with a
source Region, queues a time-9 update to two receivers, saves at time 7,
mutates the old live bounds, destroys the original execution, then restores
using a new `EmbeddedFederationRegistry` backed by the same file save store.
The test checks one object, three Regions, one queued TSO attribute message,
two recipient queue entries, and one source-region snapshot in the image. It
then verifies restored `(2,4)` bounds, changes the Region to `(0,2)`, and
observes one time-9 reflection per receiver carrying the original Region
handle. Cleanup calls Unassociate Regions for Updates with the original
object/attribute/Region pair. The test does not directly assert the serialized
association field or send a new update through it; source capture and restore
code independently show the per-attribute association is preserved.

This fresh registry is inside the same test process. The inspected configured
process-endpoint restore-status case asserts restore callbacks/status and
operation order but has no Region setup or assertions; process-endpoint Region
state persistence, host-process restart, and 2010 behavior remain unverified.
Added an association-plus-queued-TSO sequence to the 2025 Save/Restore guide
and a reciprocal DDM link. Both new Save/Restore sequences lack fresh Mermaid
syntax/render review and GitHub rendering remains unverified. No C++ source,
build, test, CI, or Requirements Lab work.

Inventory is 263 diagrams across 34 files: 151 flowcharts, 89 sequences, and
23 state diagrams (14 separate 2010, 234 2025 topic, 13 cross-cutting, and 2
index diagrams).

2026-10-12: Completed the bounded 2025 object/interaction survey against the
discovery, subscription-state, DDM, DSE, time-management, and TSO-retraction
guides. Existing material already keeps unknown-object discovery and its
initial reflection distinct from an already-known object's subscription
change, a previously issued update with deferred eligibility, regional
overlap, timestamp-order eligibility, and application callback observation.
The embedded source invokes its ordinary initial-value planner after
discovery; the process subscription handler replans discovery and the process
discovery enqueue path builds initial values only for RTI-owned MOM objects.
The focused embedded late-discovery case checks the selected stored value;
current focused tests do not isolate a value-only callback for a newly added
attribute on an already-known ordinary object. The pinned headers expose API
signatures, not the normative prose. IEEE's public listing routes standard
access through subscription. No distinct diagram gap was found, so no chart
was duplicated or changed; 2010 remains separate.

Next handoff: resolve the already-known-object/new-attribute current-value
question and the related discovery/reflection ordering only from an authorized
copy of IEEE 1516.1-2025 clauses 5.8, 6.9, and 6.11. Reconcile the clauses
with the current embedded/process evidence without elevating source comments or
tests into normative proof. If licensed text is unavailable, retain the open
boundary and continue with another 2025-only backlog topic; do not infer from
2010. No build, test, CI, or Requirements Lab work.

## 2025 process Save/Restore call and callback boundaries — 2026-10-12

Audited the process Save/Restore handlers, public adapter branches, client
response/event drain, and selected configured-endpoint tests. The HLA_EVOKED
untimed Save case uses polling and observes initiation/saved callbacks only
after explicit Evoke calls; the HLA_IMMEDIATE Save-failure case uses pushed
events and observes initiation/not-saved callbacks by the corresponding public
call returns. Those are separate tested pairings, not a rule tying one
transport mode to one callback model.

Added a bounded process Restore sequence to the 2025 Save/Restore guide. It
separates the process control response from HLA request/federation callbacks,
shows early rejection versus begun success/terminal failure, and places RTI
image rehydration at the completion barrier. The Restore evidence is limited
to one-member pushed-event HLA_IMMEDIATE tests over a loopback endpoint served
by a thread. It does not establish HLA_EVOKED Restore timing, multi-member
call boundaries, OS-process restart, embedded/process parity, or 2010 behavior.

The diagram inventory is 267 across 34 design files: 154 flowcharts, 90
sequences, and 23 state diagrams (14 separate 2010; 238 2025 topic; 13
cross-cutting; 2 index). No C++ source, tests, build, CI, or Requirements Lab
work occurred. `mmdc` was unavailable and the authorized Scoop catalog search
found no Mermaid CLI package, so the new sequence has no fresh syntax/render
or GitHub verdict.

Next handoff: validate the new sequence with a GitHub-compatible Mermaid
renderer, then inspect the configured process endpoint's HLA_EVOKED Restore
polling path and focused public-test coverage. Do not infer Restore behavior
from the HLA_EVOKED Save tests or claim 2010/embedded parity.

## 2025 process Restore polling and HLA_EVOKED callback boundary — 2026-10-12

The continuation audit found direct HLA_EVOKED process Restore lifecycle tests
that were missed by the prior narrow test inventory. This corrects the previous
handoff's statement that HLA_EVOKED Restore coverage was absent. The configured
endpoint tests use polling (`ProcessFederationServiceOptions{false}`) with
`HLA_EVOKED`: the request and completion methods return while the callback
counters are still unchanged, then explicit `evokeCallback` calls poll and
deliver Restore success/begun/initiate and terminal outcome callbacks. Separate
cases cover early request rejection, terminal participant failure, abort, and
two-member status/callback projection. Another test configures pushed event
frames together with `HLA_EVOKED`, demonstrating that pushed receipt can still
leave user callbacks pending until Evoke. This is not a complete transport by
callback-model matrix.

Added a dedicated 2025 polling sequence beside the existing pushed-event
`HLA_IMMEDIATE` sequence in the Save/Restore guide. Both are public configured
endpoint traces hosted by a test-server thread over loopback TCP; neither is an
OS-process restart demonstration. Process Restore is not generalized to
embedded behavior or 2010. The existing immediate chart remains explicitly
one-member; separate two-member evidence is cited without broadening that
chart's timing claim.

The working-tree inventory is 268 diagrams in 34 design files: 154 flowcharts,
91 sequences, and 23 state diagrams (14 separate 2010; 239 2025 topics; 13
cross-cutting; 2 index). The new polling sequence and the prior immediate
sequence still lack a fresh parser/render and GitHub verdict; `mmdc` is absent
and the authorized Scoop search found no package. No C++ source, tests, build,
CI, or Requirements Lab work.

Next handoff: validate both process Restore call-boundary sequences with a
GitHub-compatible Mermaid renderer. Then resume prioritized backlog item 5 by
auditing process-endpoint Region Save/Restore coverage and its public-test
limits; do not infer it from embedded Region-image tests. Keep 2010 separate
and skip builds, test reruns, CI, and Requirements Lab work.

## 2025 process Region image path and test boundary — 2026-10-12

A bounded source audit found that the configured 2025 process client and
service delegate Save/Restore control operations to the shared
`EmbeddedFederationRegistry`. Save completion captures Region handles, owners,
dimensions, pending and committed bounds, and state flags into the versioned
typed image before Save success is exposed. Restore decodes that section and,
at final member completion, rebuilds a candidate Region map against the
restored dimension catalog and live joined-member set. The decoder preserves
the distinction between an older image with no typed Region section and a
current typed section containing zero Regions. Regional subscriptions, object
update-region associations, and queued-message source-region snapshots are
separate image sections.

The inspected configured-process public tests cover Save/Restore lifecycle,
status, and callback transport but do not create or query Regions. The
file-backed federation-registry Region test creates a second registry object
inside the same test process and verifies handle, dimensions, and committed
bounds; it does not exercise the public process endpoint or an OS process
restart. Existing embedded API Region examples remain separate evidence.

Added a source-derived process adapter-to-image sequence to the 2025
Save/Restore guide and linked it from the DDM/Regions guide. It is explicitly
not an HLA normative flow or process-endpoint Region test. The Mermaid syntax
and render verdict remain pending: `mmdc` is unavailable, Scoop has no
matching package, and repository-derived diagram text was not sent to a
hosted renderer. No C++ source, tests, build, CI, or Requirements Lab work.

The working-tree inventory is 269 diagrams across 34 files: 154 flowcharts,
92 sequences, and 23 states (14 separate 2010; 240 2025 topics; 13
cross-cutting; 2 index).

Next handoff: when a compatible local renderer is available, validate the
two process Restore callback-boundary sequences and the new Region image
sequence. Then select another 2025-only reader gap. Keep 2010 separate and
avoid builds, test reruns, CI, and Requirements Lab work.

## 2025 joined-federate MOM Save/Restore profile boundary — 2026-10-12

Compared the `HLAfederateState` state projection in the joined-federate MOM
guide with its focused test and the configured 2025 process-endpoint tests.
The embedded case directly observes the conditional `3 → 1` Save and `5 → 1`
Restore reflections. A bounded search of `ieee1516_2025_connection_*.cpp` and
`.hpp` test sources found no direct query/assertion of `HLAfederateState`.
Although the process Restore API/callback path is separately documented, its
shared registry implementation is not endpoint test evidence. Added this
explicit limitation and labeled the state test embedded-only; no diagram or
count changed. No C++ source, tests, build, CI, 2010, or Requirements Lab work.

The local preview remains unavailable: the UI exposes no targetable desktop
application, and existing hosted Mermaid tabs were left untouched. Keep the
two process Restore callback-boundary diagrams, Region image diagram, and
revised Connect finalization flow on the unrendered queue.

## 2025 Connect finalization ordering — 2026-10-12

A bounded source comparison found a readability/ordering ambiguity in the
Connect validation flow: it showed the `not_connected → not_joined` edge and
callback/resource installation as separate-looking steps, while the prose
said the transition was the final commit. The source applies the lifecycle
edge and then installs the prepared callback/session and connection fields in
the same final locked section. Clarified the text and collapsed those details
into one diagram node; this is Umbra source order, not a distinct HLA-visible
protocol event. Focused tests exercise the callback-model and AlreadyConnected
conditions separately and do not assert combined-invalid precedence or this
internal commit order. The revised Mermaid remains unrendered; no diagram
count changed. No C++ source, tests, build, CI, 2010, or Requirements Lab work.

Next handoff: survey the HLA-first onboarding path across the 2025 connection,
federation lifecycle, FOM admission, name-reservation, and object-visibility
guides: from ambassador construction and Connect through Create/Join to an
eligible observer's discovery. Determine whether the existing guide chain
already teaches the portable transitions or whether a small orientation flow
would close a real navigation gap. Keep 2010 and process/embedded evidence
separate; add no umbrella chart unless the bounded comparison establishes a
reader need. Keep the four pending Mermaid diagrams on the local-render queue;
continue docs work without builds, test reruns, CI, or Requirements Lab edits.

## 2025 first-object reading path and additional-FOM observer boundary — 2026-10-12

The HLA-first reading-path survey found the existing focused guides already
cover Connect/configuration, authorization, distinct Create and Join
transitions, FOM admission, object registration, and receiver-specific
discovery. The guide index now labels that chain as a reading path rather than
a required call sequence and identifies name reservation as optional for
named registration; unnamed registration has no reservation step. No umbrella
diagram was needed.

A bounded additional-FOM Join audit found that the shared registry commit
reconciles and installs the replacement definition and handle directories
with the new membership, but does not itself call ordinary object-discovery
planning. The embedded and configured-process subscription paths do explicitly
replan discovery after subscription changes. Existing embedded class and
interaction tests verify catalog lookup and prior-handle equality; the process
case verifies the joining client's lookup. None tests a pre-existing
subscriber's callback for an already-registered object after model replacement.
Added this source/test boundary and open observer question to the FOM guide;
it is not an HLA requirement claim. No diagram or count changed, and the
existing local-render queue is unchanged. No C++ source, tests, build, CI,
2010, or Requirements Lab work.

Next handoff: audit which compatible additional-FOM compositions can extend an
existing object-class hierarchy or member set, then trace how the current
discovery resolver and existing subscription records interpret those cases.
Use existing focused tests only; keep standard, Umbra-source, and test claims
separate. Add guide detail only for demonstrated reader value, preserve the
2010/2025 boundary, and skip builds, test reruns, CI, and Requirements Lab work.

## 2025 additional-FOM extension shapes and observer-state boundary — 2026-10-12

The bounded composition survey found that the successful additional-module
lookup fixture adds a child class beneath the existing HLAobjectRoot; the
already-joined member resolves it while the earlier Employee-class handle
remains equal. That test does not extend existing attributes or assert
callbacks. The official Restaurant extension adds an attribute under the
existing Food.Drink.Soda class plus interaction parameters, but this exact
additional-module set is rejected at FDD materialization, so it is not a
successful Join example. The composer has recursive identity-based merging,
but successful materialization remains a separate gate.

The registry reconciles class/attribute handles with the replacement catalog
without rewriting existing explicit per-class subscriptions or per-instance
attribute ownership. A later declaration-triggered discovery plan uses the
current catalog plus those stored states. The Join commit itself does not
create subscriptions, instance ownership, or an ordinary discovery plan. The
FOM guide now distinguishes the tested additive-class case, the rejected
existing-member example, and the source-only runtime consequences. No new
diagram or count change; no standard guarantee or 2010 parity is inferred. No
C++ source, tests, build, CI, or Requirements Lab work.

Next handoff: audit the distinct 2025 interaction-class counterpart: how an
additional module that introduces interaction classes or parameters relates
to existing per-class subscriptions and send-time hierarchy resolution, and
what existing tests show about receives before versus after Join. Keep
interaction routing distinct from object discovery; use bounded source/test
evidence, make no 2010 or normative claim without exact support, and skip
builds, test reruns, CI, and Requirements Lab work.

## 2025 additional-FOM interaction subscription and later-send boundary — 2026-10-12

The embedded interaction-class lookup case adds a child class under
HLAinteractionRoot; an already-joined member resolves it while the earlier
interaction handle remains equal. The parameter lookup case adds a base/child
interaction chain and resolves the base-defined Identifier through the child.
Neither case subscribes, publishes, sends, or receives. A separate embedded
receipt case loads the full hierarchy at Create and checks that a parent
subscription receives a child send under the closest subscribed class; it is
not an additional-Join scenario.

The shared registry stores explicit per-federate interaction-class
subscriptions and the ordinary send planner walks the current catalog's
parent chain at send time. Thus source indicates a pre-existing ancestor
subscription can be considered for a future send after the catalog grows, but
no focused test covers ancestor subscription + additional Join + descendant
send. Join does not synthesize a Receive Interaction for the newly added class.
The configured-process additional-FOM case checks object-class lookup only;
it is not process interaction-delivery evidence. Added this bounded distinction
to the FOM guide, cross-linked the separate object-discovery and directed
interaction paths, and did not add another chart. No normative or 2010 claim,
diagram-count change, C++ source/test/build/CI, or Requirements Lab work.

Next handoff: audit the 2025 configured-process ordinary interaction route for
an existing ancestor-subscription/descendant-send test. Trace subscription,
send-frame creation, polling, and callback delivery, and identify which edges
are source-only versus asserted under HLA_EVOKED and HLA_IMMEDIATE. Do not infer
the additional-FOM Join scenario or embedded/process parity from shared
registry code alone. Keep 2010 and Requirements Lab separate; skip builds,
test reruns, and CI.

## 2025 configured-process ordinary interaction delivery and callback-model boundary — 2026-10-12

The configured-process interaction declaration case exercises publish,
active/passive subscribe, unsubscribe, and unpublish state transitions, but it
does not send. The public Send Interaction case sends an exact-class receive-
order event and uses a raw process receiver to inspect the parameter handle,
value, and tag; it does not assert a FederateAmbassador callback. A separate
configured-process multi-recipient test does assert two `receiveInteraction`
callbacks per exact-class receiver, FIFO tag order, and the selected class,
producer, valid transportation, parameter count, and region-presence fields
under both HLA_EVOKED and HLA_IMMEDIATE.

The implementation trace now distinguishes stored active subscriptions,
shared recipient planning, process event materialization, push-frame versus
pull-queue delivery, and callback-bridge submission. In the focused test,
HLA_EVOKED uses the default polling service and explicit evoke calls;
HLA_IMMEDIATE enables the private push option and uses a later receiver-side
class lookup as an exchange fence. This is the fixture's transport selection,
not a normative coupling of callback model to transport, and the test does not
establish ordering against the sender's public call return. Its exact-class
subscriptions do not test ancestor-subscription promotion, and it is not an
additional-FOM Join test. Updated the existing process interaction sequence
and its test/source map; no new chart or 2010 inference. Mermaid syntax/render
review remains pending. Test sources were inspected, not run; no build, CI, or
Requirements Lab work.

Next handoff: inspect the configured-process additional-module Join path only
as it relates to the ordinary interaction question: whether the server accepts
the added module, how the shared current catalog reaches later send planning,
and whether any focused process test combines that Join with a prior ancestor
subscription and descendant send. Keep source behavior distinct from the
existing embedded Create-time hierarchy case; make no parity, normative, or
2010 claim, and skip builds, test reruns, CI, and Requirements Lab work.

## 2025 process additional-FOM Join and interaction-route boundary — 2026-10-12

The process server accepts a Join carrying FOM modules only when its optional
`additionalFomPreparation` callback is configured; otherwise the service
rejects the request. The configured-process test and process service probe
install the 2025 FederationManagementCoordinator callback. The service
serializes preparation with registry commit, and the registry reconciles the
existing interaction-class handle directory before swapping the replacement
definition/directories with the new membership. Stored per-federate
interaction declarations are a separate map and are not rewritten by this
commit. A later process Send Interaction reaches the shared planner, whose
current-catalog resolver walks the sent class's parent chain against those
subscriptions. That is a source-only inference that a retained ancestor
subscription can be considered for an added descendant; no process delivery
test exercises it.

The configured-process additional-FOM test adds an object-only class and
checks object-class lookup/name resolution. It has no interaction-class or
parameter addition, subscription, Send Interaction, or Receive Interaction
callback. The broad process probe installs the same preparation callback but
does not supply missing receive-coverage evidence. Expanded the FOM guide's
existing additional-module flow with this profile-specific source/test
boundary and linked it from the object/interaction guide; the existing
replacement/interaction ancestry diagrams already carry the necessary concepts,
so no chart was added. No normative, 2010, or process/embedded parity claim.
Sources/tests were inspected, not run; no build, CI, or Requirements Lab work.

Next handoff: audit the 2010-only interaction section of the separate
Reference RTI guide against the pinned 2010 interaction API, its reference
runtime route, and focused 2010 test sources. Check whether the guide's current
send/relevance/receive explanation has a genuine reader gap; if so, update
only the 2010 guide and keep every 2025 routing claim separate. Do not build,
rerun tests, start CI, or edit the Requirements Lab. Preserve the existing
local Mermaid render queue; use no hosted renderer for repository text.

## 2010 ordinary interaction send validation and handle boundary — 2026-10-12

Audited the 2010 interaction flow against the pinned `sendInteraction` API,
`ReferenceRuntime2010`, and the focused binding-shell smoke. The diagram had
collapsed three distinct send checks into a single generic exception branch.
It now separates an unknown runtime class handle
(`InteractionClassNotDefined`), missing sender publication
(`InteractionClassNotPublished`), and a supplied nonmember parameter handle
(`InteractionParameterNotDefined`) before recipient selection. The source
performs these checks in that order, then calls `receiveInteraction` inline
for other members subscribed to the exact handle. Callback failure remains a
separate `FederateInternalError` to `RTIinternalError` partial-fanout path.

The reference profile lazily creates interaction-class and parameter
directory entries during name lookup and does not load the FOM path passed to
creation. The smoke uses `ignored-fom.xml`, performs lookup/publication/
subscription/send, and verifies one receiving callback; it does not establish
FOM admission, multi-recipient fan-out, or callback-failure outcomes. The
guide now cross-links this distinction and keeps the three API-declared
send-specific errors separate from the complete service-entry exception
surface. This was a 2010-only documentation change with no 2025 routing
inference. The revised Mermaid chart has no fresh local syntax/render verdict;
counts are unchanged. Sources and tests were inspected, not run; no build, CI,
or Requirements Lab work.

Next handoff: audit the 2025 data-element encoding guide against its pinned
encoding API, scalar/composite/array codec paths, and existing tests. Clarify
caller-selected versus type-fixed byte order, string/text contracts, and the
boundary between HLA value encoding and Umbra process-frame serialization.
Keep the 2010 encoding guide separate and do not infer edition parity. Skip
builds, test reruns, CI, and Requirements Lab work; preserve the local Mermaid
render queue.

## 2025 string encoding units and C++ value contract — 2026-10-12

The 2025 encoding guide already separates caller-selected BE/LE types, fixed
type formats, composite child encodings, factory-defined logical time, and
private process framing. The audit found a narrower reader gap: it named the
ASCII and UTF-16 string formats but did not explain what the public C++ string
types mean at the codec boundary. Added an implementation-bounded comparison
and flowchart to the [2025 encoding guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md).

The pinned API pairs `HLAASCIIstring` with `std::string` and
`HLAunicodeString` with `std::wstring`. Umbra validates every ASCII byte as
0x00–0x7F (preserving embedded NULs in the length-delimited payload); Unicode
input is normalized to UTF-16 units, with surrogate/scalar validation, and
the prefix counts units rather than bytes or Unicode scalar values. The
focused Catch2 examples verify an embedded-NUL ASCII value, rejection of
high-bit bytes, and a supplementary Unicode character encoded as three
UTF-16 units after `A`; malformed UTF-16 and an unmatched wide surrogate are
also covered. These are selected implementation tests, not exhaustive Unicode
conformance evidence. Value payload length remains distinct from the private
process-frame header.

Recounted all current Markdown Mermaid fences (both backtick and tilde): 270
diagrams across 34 files, comprising 155 flowcharts, 92 sequences, and 23
state diagrams; the edition-aware totals remain separate at 14 for 2010, 241
for 2025 topic guides, 13 cross-cutting, and 2 index diagrams. The new string
flowchart has no fresh local syntax or visual render verdict. No source, test,
build, CI, Requirements Lab, or 2010 encoding change.

## 2025 process Resign and queued ownership-cancellation callback — 2026-10-14

A bounded scan of the guide portfolio confirmed Federation Synchronization
already separates barrier registration, participant selection, late join,
achievement fan-in, member departure, and save/restore; its current diagrams
and prior source/test mappings make another synchronization flow duplicative.
The lifecycle guide already links readers to the general process Resign
callback-queue caveat, but lacked a concrete ownership example. Added a
cross-link from its exit-state discussion to the push-mode Attribute Ownership
Acquisition Cancellation trace.

Rechecked the relevant 2025 process sources and focused test. The push handler
consumes the cancellation before sending the event frame. The client buffers
that frame while awaiting the service response, then submits it to the callback
bridge before returning; the `HLA_EVOKED` task has no cancellation-registry
recheck. The process Resign branch does not reset the dispatcher. The focused
process cancellation test uses default `pushReceiveOrderEvents=false` with
`HLA_EVOKED`, so it establishes pull confirmation only. The queued-push result
after deletion or Resign remains source-derived, untested behavior and is not a
portable HLA rule. The ownership guide retains the detailed flow; the lifecycle
guide now points to it without copying its state machine. No diagram changed,
so no render was required. No 2010 inference or Requirements Lab work.

Next handoff: survey the 2025 process Connection Lost/forced-resignation path
against voluntary Resign in the lifecycle and callback guides. Compare the
cleanup of server-queued events with client-buffered and callback-queued work;
use focused existing tests, preserve the HLA/implementation distinction, and
add or revise a diagram only for a genuine reader gap. Do not build, rerun tests,
start CI, edit the Requirements Lab, or infer 2010 behavior.

## 2025 process Connection Lost and independent work queues — 2026-10-14

The lifecycle guide now distinguishes the process fault fixture's forced
registry transition from the later client-side transport failure. The fixture
applies forced loss and survivor projections, detaches the session (removing
server-side queues), then closes the receiver socket. A bounded search found no
production C++ caller of the process service's `connectionLost` helper, so the
fixture sequence is not generalized into an automatic EOF policy.

Added a process-specific lifecycle flowchart for the three independent stores:
server-session event queues, client-buffered `pendingEvents_`, and tasks already
submitted to the shared callback dispatcher. The local failure handler marks
the endpoint Not Connected and submits `FederateAmbassador::connectionLost`;
it does not commit server registry loss or reset the shared dispatcher. The
failed-request path dispatches transport failure rather than pending pushed
events, and client close clears its buffer. Focused process TCK evidence covers
selected survivor effects and the local callback under both callback models,
but does not assert post-loss delivery of buffered events or unrelated queued
callbacks. This is an implementation-profile explanation, not an HLA queue
guarantee or a 2010 inference.

Rendered the added Mermaid chart with Mermaid CLI 12.0.0 and visually checked
the result, including explicit yes/no outcomes for both queue checks. The
Markdown inventory is now 281 diagrams across 34 files: 160 flowcharts, 98
sequences, and 23 state diagrams (15 separate 2010, 250 2025 topic, 13
cross-cutting, and 3 index diagrams). No source/tests, build, test reruns, CI,
Requirements Lab, or 2010 guide changes.

Next handoff: audit whether 2025 attribute ownership transfer changes an
object's attribute update-region association. Compare the bounded ownership
service path, object-region state, DDM guide, and focused tests; identify what
transfers and what remains attached to the object, and mark any source-only or
untested edge. Update a diagram only for a genuine reader gap. Keep 2010
separate; do not infer cross-profile parity. No builds, test reruns, CI, or
Requirements Lab work.

## 2025 ownership/update-region state and process-route boundary — 2026-10-14

The current Ownership and DDM guides already cover the ownership-to-region
association boundary, so no duplicate flow was needed. Rechecked the selected
embedded registry transition and three focused HLA_EVOKED test bodies: an
owner-to-owner transfer clears the former owner's active association; a
prospective owner's deferred association becomes active only if it remains at
transfer; and unassociating before transfer prevents promotion, leaving the
acquirer's first update on the default source realization. The former owner
must associate again after reacquisition. These are embedded test-source
observations, not a complete normative rule; the checkout lacks the full
clause prose for independent verification.

A bounded 2025 process-route inventory found a distinct adapter boundary:
`attributeOwnershipDivestitureIfWanted` is declared by the pinned API, but the
Umbra public method directly consults and mutates `embeddedFederationRegistry()`.
The process transport/client/service inventory has no matching operation or
handler, unlike the explicit process Confirm Divestiture and Unconditional
Divestiture routes. Added this limitation to the ownership guide as an Umbra
implementation finding, not an HLA exception. No process update-region test
was inferred from ownership-only process cases.

No diagram changed or needed rendering. Inspected sources and test bodies; did
not run tests or builds, start CI, edit the Requirements Lab, or change the
2010 stream.

Next handoff: map the 2025 process ownership-service routes for Acquisition If
Available, Confirm Divestiture, Unconditional Divestiture, and cancellation.
Trace public adapter, transport, registry plan, event delivery, and focused
process TCK assertions. Separate a missing Umbra route from normative HLA
requirements; label shared registry code as source evidence rather than
integration-test parity. Update the ownership guide only for a genuine reader
gap. Keep the 2010 stream separate; no builds, test reruns, CI, or Requirements
Lab work.

## 2025 process ownership routes and unconditional-divestiture result boundary — 2026-10-14

Completed the bounded process-route comparison for Acquisition If Available,
Confirm Divestiture, Unconditional Divestiture, and acquisition cancellation.
The ownership guide now has a route/evidence table that keeps private service
results distinct from recipient notifications and states exactly what the
selected process test sources do and do not exercise. The pinned API declares
Divestiture If Wanted, but the process protocol/client/service route is absent;
the public implementation uses the embedded registry. This remains an Umbra
profile gap, not an HLA rule.

Added one 2025 process Unconditional Divestiture flowchart. It distinguishes
the private accepted Boolean from recipient acquisition work and assumption
offers, push versus pull event transport, and the callback bridge's distinct
public projections. Source inspection shows registry state is committed
before recipient enqueue/send work; a later failure returns an internal
error and the inspected path has no rollback. No focused process test covers
that edge. The focused Catch2 case is embedded; a portable TCK scenario in
source does not establish a process-profile execution. Selected process
acquisition/cancellation tests cover pull-mode cases, while the Confirm
Divestiture case uses HLA_IMMEDIATE and is bounded to its selected callback
assertions. No HLA timing or callback-before-return rule is inferred from
these implementation paths.

Mermaid CLI 12.0.0 rendered the chart, and its layout was visually reviewed.
The Markdown recount is 282 diagrams in 34 files: 161 flowcharts, 98
sequences, and 23 state diagrams, comprising 15 separate 2010, 251 2025, 13
cross-cutting, and 3 index diagrams. The full guide and tree were not rendered.
No build, test rerun, CI, Requirements Lab, or 2010 change.

Next handoff: review whether the 2025 ownership guide needs one HLA-first
comparison of negotiated Confirm Divestiture, Unconditional Divestiture,
Divestiture If Wanted, and Acquisition If Available—especially which paths
leave attributes unowned, how later acquisition/assumption is represented,
and which outcome is a return value versus callback. Reuse existing diagrams
if they already teach this clearly; create a compact decision/transition
chart only for a real reader gap. Verify normative statements against pinned
2025 clauses and keep implementation/test evidence separately labeled. Keep
2010 separate; no builds, test reruns, CI, or Requirements Lab work.

## 2025 HLA-first ownership service-selection map — 2026-10-14

The four detailed ownership paths were already individually documented, but
the guide did not give readers one place to see who initiates each path and
how its outcome is represented. Added a compact selector for requester-led
Acquisition If Available, owner-led Unconditional Divestiture and Divestiture
If Wanted, and the negotiated owner-confirmation path. The map labels the
If-Wanted output set separately from acquisition notifications and makes the
selected If-Available callback-entry observation explicit without asserting a
universal callback order. Detailed per-service diagrams remain separate.

Checked the path distinctions against pinned 2025 RTIambassador signatures
(§§7.2, 7.3, 7.6, 7.9, and 7.13), callback declarations, and the four linked
focused test sources. The process Confirm Divestiture test does not establish
ownership-query timing; embedded tests are not one combined conformance
matrix. The checkout lacks complete clause prose. Mermaid CLI 12.0.0 rendered
the new chart, and the layout was visually reviewed. No tests/build/CI,
Requirements Lab, or 2010 work.

Next handoff: audit the open 2025 §6.21 Request Attribute Value Update
entitlement question. Compare the full normative clause with the current
guide's separation of provider solicitation from receiver eligibility and
the focused embedded/process test scenarios, especially whether a requester
without ordinary receiver eligibility is entitled to a provider's later
update. The official [IEEE 1516.1-2025 listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers access by purchase or subscription; do not infer the missing rule from
API declarations or implementation. If no authorized clause text is
available, preserve the open question and move to another evidence-resolvable
HLA reader gap. Keep 2010 separate; no builds, tests, CI, or Requirements Lab.

### 2025 regional Request Attribute Value Update and later receiver DDM — 2026-10-14

The full authorized 1516.1-2025 §6.21 clause text is still not present in the
checkout. The official [IEEE listing](https://standards.ieee.org/ieee/1516.1/6688/)
offers access by purchase or subscription, so the question of direct-response
entitlement for a requester without ordinary receiver eligibility remains
open; no normative conclusion was inferred from API or implementation.

Expanded the existing fifth 2025-only regional request-versus-response flow
to show the selected embedded rules more clearly: an empty request-region set
does not solicit a provider; an explicit provider update-region association
must overlap a committed request region; a default-region attribute remains
eligible; and queued work is rechecked at callback delivery so a stale
explicit-region request can be suppressed after a committed range change.
The later update enters separate ordinary per-receiver planning and does not
inherit request-region state. Source review covered the embedded request plan
([request planner](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L999),
[regional filter/default-region rule](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L579)),
the callback-entry projection
([recheck](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L1751)),
and the later receiver DDM evaluation
([ordinary update recipients](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L221)).
The focused [regional-request test](../../cpp/tests/ieee1516_2025_regional_request_attribute_value_update_filtering_catch2.cpp#L178)
checks the empty-region no-op, disjoint explicit versus default-region
eligibility, overlap after recommit, and stale callback suppression
([scenario details](../../cpp/tests/ieee1516_2025_regional_request_attribute_value_update_filtering_catch2.cpp#L100));
it does not send a subsequent value update, prove process parity, or resolve
the unsubscribed-requester entitlement question. Tests were inspected, not run.

Mermaid CLI 12.0.0 rendered the revised chart; local visual review found its
tall layout readable, with the provider-solicitation and later receiver paths
distinct and no crossing branches. The whole-tree inventory remains 283
diagrams (162 flowcharts, 98 sequences, 23 state diagrams), including the
separate 2010 stream; no 2010 material changed. No build, tests, CI, or
Requirements Lab work.

Next handoff: visually review the existing 2025 DDM region-template deletion
gate in `HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md` against its
focused embedded region-lifecycle cases and registry guard. Keep its
RegionInUseForUpdateOrSubscription, deferred-association release, and stale
InvalidRegion outcomes bounded to that 2025 embedded evidence; do not infer
process or 2010 parity. Preserve the open §6.21 entitlement question unless
authorized clause text becomes available. No build, test, CI, or Requirements
Lab work.

### 2025 DDM Region deletion gate render review — 2026-10-14

Rendered the existing Delete Region gate in the 2025 DDM and Regions guide
with Mermaid CLI 12.0.0. Screenshot review found its left-to-right layout
readable: the in-use rejection, dependency-release loop, owned-attribute
unpublish path, successful deletion, and stale-handle result are distinct.
The source spot-check confirmed the embedded registry checks the owned live
region's in-use state before erasing it
([delete guard](../../cpp/src/internal/federation/federation_registry_regions.cpp#L339)).
Focused embedded 2025 test sources cover a passive regional interaction
subscription blocking deletion, an object update-region association released
by unpublishing, and deferred association release after the final object
removal callback; the same test verifies InvalidRegion after deletion
([region lifecycle tests](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L134),
[object association](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L221),
[deferred removal](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L369)).
These sources were inspected, not run. The chart remains an embedded-profile
explanation, not normative text or evidence of process/2010 parity.

No diagram count changed; the whole-tree inventory remains 283 (162
flowcharts, 98 sequences, 23 state diagrams), with 2010 and 2025 streams
separate. No build, test, CI, or Requirements Lab work.

Next handoff: review the existing 2025 interaction-region dimension-context
flow in `HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md` against the two
focused mixed-region interaction cases. Preserve their separate send versus
subscription scenarios and the specific available-dimension checks; do not
imply one combined transaction, universal service rollback, or process/2010
parity. The §6.21 entitlement question remains open pending authorized clause
text. No build, test, CI, or Requirements Lab work.

### 2025 interaction-region dimension-context review — 2026-10-14

Rendered and visually reviewed the existing interaction-region
dimension-context chart in the 2025 DDM and Regions guide with Mermaid CLI
12.0.0. The two vertical branches are legible and clearly separated: one
tests Send Interaction With Regions, the other tests Subscribe Interaction
Class With Regions. The guide explicitly says they are separate embedded
cases, not one combined service sequence.

Rechecked both focused `HLA_EVOKED` test scenarios and their implementation
boundaries. The send case commits one region using a class-available
dimension and one using a valid but unavailable dimension; the mixed send
throws `InvalidRegionContext` and produces no observed receiver callback,
then the valid-only send is observed. The subscription case rejects the
unavailable-dimension region; a later send produces no callback, and a valid
regional subscription then permits a matching callback. The embedded send
planner validates each region against class-available dimensions before
capturing region snapshots
([planner check](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L82),
[send failure mapping](../../cpp/src/internal/runtime/umbra_rti_ambassador_regional_interaction_send_services.cpp#L709)).
The regional subscription validates before mutating its declaration map
([registry validation](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L593)).
Focused evidence is in the separate
[mixed-region send](../../cpp/tests/mixed_region_interaction_validation_catch2.cpp#L120)
and [regional subscription](../../cpp/tests/mixed_region_interaction_validation_catch2.cpp#L229)
cases. These tests do not establish general transaction or rollback rules;
source/test files were inspected, not run. No process or 2010 parity is
claimed.

No diagram count changed: the inventory remains 283 (162 flowcharts, 98
sequences, 23 state diagrams), and the separate 2010 stream is unchanged. No
build, tests, CI, or Requirements Lab work.

Next handoff: visually inspect the existing 2025 FOM-dimension-to-runtime-
region bridge in the same DDM guide. Preserve the three distinct sets already
identified there—the execution dimension catalogue, dimensions available to
a class, and dimensions selected for a Region—and verify its query map against
the pinned `RTIambassador` declarations. Render the chart locally and record
any source/test limits; do not turn the conceptual bridge into a required
service-call order or infer 2010 behavior. §6.21 entitlement remains open.
No build, test, CI, or Requirements Lab work.

### 2025 FOM-dimension-to-Region bridge render review — 2026-10-14

Rendered and visually reviewed the existing conceptual bridge in the 2025
DDM and Regions guide with Mermaid CLI 12.0.0. The tall layout remains
readable at preview width; the optional `getDimensionHandleSet(region)` path
is visibly separate from setting bounds, committing, and using the Region.
The chart already distinguishes FOM declarations/class associations from
the Region's fixed dimension set and explicitly avoids prescribing an
application call order.

Checked its API map against the pinned 2025 `RTIambassador` declarations:
`createRegion` §9.2 accepts a dimension set and has no class argument;
`commitRegionModifications` is §9.3; class-availability queries are
§§10.21–10.22; `getDimensionHandleSet` is §10.26; `getRangeBounds` is §10.27;
and `setRangeBounds` is §10.28. This supports the guide's distinction between
class availability and a Region's selected dimensions, not a required order
for application calls.

The focused embedded region-lifecycle test observes a Region's dimension set
immediately after creation and before commit, and reads pending bounds before
commit and committed bounds afterward
([dimension-set query](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L65),
[pending bounds](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L88),
[committed bounds](../../cpp/tests/ieee1516_2025_federation_management_region_lifecycle_catch2.cpp#L117)).
A separate embedded regional-object test checks the class-available dimension
query before using different valid class dimensions for source and receiver
Regions ([query assertions](../../cpp/tests/regional_object_attribute_routing_catch2.cpp#L636)).
These are bounded development-profile observations, not a test of the entire
conceptual chart or evidence of process/2010 parity. The process-endpoint
query-only case documented in the guide does not establish process DDM
behavior. Sources and tests were inspected, not run.

No diagram count changed: 283 total (162 flowcharts, 98 sequences, 23 state
diagrams), with the 2010 stream separate. No build, tests, CI, or Requirements
Lab work.

Next handoff: review the two existing queued-TSO receiver-state traces in the
2025 DDM guide (around lines 1388 and 1444). Recheck their cited source/test
boundaries, then render each current Mermaid block and record whether the
receiver-region revalidation and the interaction's send-time source-region
snapshot versus callback-time current subscription are visually and
conceptually distinct. Keep each scenario/profile bounded;
do not infer universal callback ordering or 2010 behavior. The §6.21
entitlement question remains open pending authorized clause text. No build,
test, CI, or Requirements Lab work.

### 2025 queued-TSO receiver-state trace audit and visual review — 2026-10-14

Rechecked the two existing receiver-state traces in the 2025 DDM guide against
their focused embedded tests and current source. The connection-loss attribute
case injects embedded transport loss after the publisher's time-6 grant, then
commits the surviving receiver's region from `[1,3)` to `[3,4)` before its
pending time-6 callback gate. The test observes no reflection, then the time-6
grant; on the next request it observes the independent `DELETE_OBJECTS`
removal and an unknown object name. Its separate publisher-side `connectionLost`
report is drained and checked later in the test, so the chart now says it is
omitted and does not imply a cross-ambassador callback order
([focused test](../../cpp/tests/connection_loss_regional_selector_mutation_catch2.cpp#L159),
[callback recheck](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_advance_dispatch.cpp#L349),
[loss cutoff](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L29)).

The interaction replacement test starts with overlapping source/receiver
regions, queues one timestamped send, replaces the receiver's regional
subscription before time 6, and observes suppression without a `Request
Retraction` callback. A later source-B send is delivered under subscription B
with the tested callback order `grant(6), interaction(7), grant(7)`
([focused test](../../cpp/tests/ieee1516_2025_timestamped_regional_interaction_subscription_replacement_catch2.cpp#L91)).
The embedded message stores send-time source-region snapshots; the TSO dispatch
asks the registry for a recipient projection at the callback boundary
([message shape](../../cpp/src/internal/federation/federation_registry_tso_types.hpp#L14),
[projection call](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_advance_dispatch.cpp#L260)).
Thus the receiver subscription is current state, not a frozen receiver-region
snapshot. This corrects the earlier handoff's ambiguous use of “subscription
snapshot.” It is an embedded implementation observation, not a general HLA
rule or process/2010 parity claim.

Rendered both current Mermaid blocks with Mermaid CLI 12.0.0 and visually
inspected their PNGs. A first revised render caught a sequence-label semicolon
parse error; replacing it with “and” fixed the chart, and the final render
succeeded. Shorter message labels improved legibility. The diagrams remain
scenario-specific; no new HLA callback ordering is inferred. Sources/tests
were inspected, not run. The current `docs/design` census is 283 diagrams in
34 diagram-bearing Markdown files (162 flowcharts, 98 sequences, 23 state
diagrams), counting backtick and tilde fences; six targeted 2025 DDM charts
now have local visual review. No full-guide or
whole-tree render, GitHub verification, build, test, CI, 2010 change, or
Requirements Lab work.

The conceptual DDM/time/callback-filter handoff is completed in the following
entry. The §6.21 direct-response entitlement question remains open pending
authorized clause text.

## 2025 DDM/time/callback filter map review — 2026-10-14

Rechecked the conceptual “Keep time and DDM as separate filters” map against
the cited 2025 process timestamped regional-interaction callback-gating test
and the pinned `CallbackModel` API enum. Reworked the map to branch four
independent questions from one service/receiver trace: regional scope,
RECEIVE-versus-TIMESTAMP, callback model, and callbacks-enabled state. No arrows
join those dimensions, and the prose explicitly says the diagram is not an
RTI/service execution order. Clarified that the focused process test connects
both clients with `HLA_EVOKED`; `disableCallbacks()` / `enableCallbacks()` is a
separate gate, and the observed interaction-before-grant result is limited to
that trace. The test does not compare callback models or establish embedded or
2010 parity.

Mermaid CLI 12.0.0 rendered the revised chart and its PNG was visually reviewed;
the four branches and labels remain legible. This is now the seventh targeted
2025 DDM chart with local visual review. The `docs/design` census remains 283
diagrams across 34 diagram-bearing Markdown files (162 flowcharts, 98
sequences, 23 state diagrams); whole-guide/whole-tree rendering and GitHub
verification remain pending. Sources and the existing test were inspected,
not run. No builds, tests, CI, 2010 changes, or Requirements Lab work.

The ownership/update-region handoff is audited below. The existing DDM and
Attribute Ownership guides already cover its three focused cases without
overstating their normative or profile scope, so no duplicate diagram was
needed.

## 2025 ownership/update-region boundary audit — 2026-10-14

Checked `cpp/tests/ownership_transfer_update_region_catch2.cpp` and the
corresponding DDM and Attribute Ownership guide sections. The three
`HLA_EVOKED` embedded development-profile cases separately demonstrate:

- The former owner's active association is cleared at If-Wanted transfer; the
  new owner's first update uses the default source region, explicit association
  selects its own region, and later reacquisition by the former owner does not
  restore its old association.
- A prospective owner's association can remain deferred without retargeting
  the current owner's update, then activate at transfer.
- Unassociating that deferred state before transfer prevents its promotion, so
  the new owner's update uses the default source region.

The registry path agrees: an association by a non-owner is recorded as
deferred without producing scope changes; the If-Wanted transfer path clears
the former owner's association and promotes the new owner's deferred state.
The guides already link each case, state that the sequence compares separate
tests rather than one combined scenario, and distinguish tested If-Wanted
behavior from source-only regular, Confirm, unconditional-divestiture, and
Resign paths. They also avoid stating this as a complete portable HLA rule or
process/2010 parity. No guide change was warranted; no test was rerun and no
implementation file was changed.

## 2025 object-instance name reservation chart review — 2026-10-14

Rendered all seven Mermaid blocks from
`HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md` as Markdown with
Mermaid CLI 12.0.0 and visually inspected the resulting PNGs. The owner/peer
visibility comparison and reservation-lifetime state diagram were too
compressed horizontally at preview width. Changed both to top-down layouts,
rerendered the whole guide successfully, and visually rechecked the revised
charts. No state transitions or claims were changed. The seven local renders
do not substitute for GitHub review; that remains pending publication. No
builds, tests, CI, 2010 work, or Requirements Lab changes.

## 2025 transportation pending-state chart review — 2026-10-14

Compared the chart with the focused request/confirmation and save/restore
evidence and the embedded registry/callback-dispatch paths. The service
admission records pending state; dispatch rechecks attribute ownership or
interaction membership/publication, commits the effective selection, and only
then invokes the public confirmation callback. A stale or cancelled request is
dropped without confirmation; loss of attribute ownership can also clear its
effective override, so the chart does not imply the prior value survives every
discard path. In this embedded implementation, the registry write occurs
immediately before public callback invocation; the callback remains the
federate-visible boundary, and this source ordering alone does not establish an
earlier portable or normative effective time. `HLA_EVOKED` work waits for
callback dispatch while `HLA_IMMEDIATE` work may run inline before the request
service returns. The linked focused restore cases exercise both callback
models; they are not a
complete scheduling-interleaving matrix.

Rendered all six Mermaid blocks in
`HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md` as Markdown with Mermaid CLI
12.0.0. The initial targeted render exposed a semicolon parsing problem in the
revised state labels; replacing those separators yielded a clean rerender. The
repaired pending lifecycle chart was visually inspected and is legible. This
is targeted local visual review, not GitHub verification or whole-guide/tree
coverage. No C++ files changed; no builds, tests, CI, 2010 work, or Requirements
Lab changes.

## 2025 subscription-state targeted chart review — 2026-10-14

Rendered all eight Mermaid blocks in
`HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md` with Mermaid CLI
12.0.0. Visually inspected the source-derived passive-recipient flow, the
HLA-first late-interest effects map, and the embedded regional evidence
sequence. All three are legible in the expanded local preview; the portrait
flows remain readable while scrolling, and the sequence notes/labels are not
clipped. No diagram source change was warranted.

The evidence boundary remains explicit: the passive test covers passive-only
suppression and a later active transition, not a separate active establisher
making a passive receiver eligible. The regional test records discovery and
the stored value separately from a later update but does not assert callback
order. The process initial-value comparison stays source-only for ordinary
objects, with the RTI-owned MOM path separate; the normative §5.8 question and
ordinary process observation remain open. No 2010 or profile-parity inference
was added. No builds, tests, CI, or Requirements Lab work occurred.

## 2025 embedded/process late-subscription comparison review — 2026-10-14

Visually inspected the process-versus-embedded comparison chart in the same
eight-diagram local render. Its two profile lanes are easy to distinguish; the
RTI-owned MOM initial-value event stays in its own branch, while the ordinary
process path is explicitly unresolved. No chart edit was warranted. This
remains Umbra source mapping, not a licensed §5.8 interpretation or proof of
profile parity; the ordinary process runtime observation is still open.

## 2025 service-report failure sequence visual review — 2026-10-14

Rendered all eight diagrams in
`HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md` with Mermaid CLI 12.0.0.
The switch-append failure sequence parsed but its long participant and note
labels crowded the right edge. Shortened those labels, rerendered all eight,
and visually inspected the revised chart. It remains wide but readable, with
no edge clipping; the commit-before-append, best-effort exception hook,
unasserted callback path, and skipped post-append MOM enqueue remain separate.
No semantic transition changed. The focused injected-writer test was not
rerun: its last recorded attempt failed at federation creation before the
target assertions. GitHub review remains pending publication. No builds,
tests, CI, 2010 work, or Requirements Lab changes.

## 2025 Joined-Federate MOM diagram review — 2026-10-14

Rendered all six Mermaid diagrams in
`HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md` with Mermaid CLI
12.0.0. Visually inspected the repaired subscription/discovery sequence and
resignation state chart. Both render cleanly and remain legible; the state
chart's pending-removal loop and snapshot-retention note are distinct, and the
sequence keeps Join, observer discovery/reflection, explicit value request,
and resign removal in separate steps. No chart change was needed.

The prose and evidence table preserve `HLAfederation` execution scope versus
per-member `HLAfederate` objects, and distinguish membership transition from
receiver observation. The process Join/resign cases remain separate from the
embedded sequence, and the 2010 section explicitly avoids projecting the 2025
MOM object into the separate 2010 RTI stream. This is targeted local review,
not a complete GitHub rerender. No builds, tests, CI, or Requirements Lab work.

## 2025 FOM admission/composition visual review — 2026-10-14

Rendered all four Mermaid diagrams in
`HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md` with Mermaid CLI
12.0.0. The local Create and Composition charts were too compressed in their
left-to-right layouts, so both were reoriented top-down and rerendered. Their
labels are now readable while scrolling; the Join chart remains tall but
legible, and the FOM-edition chart clearly keeps the 2010 catalog-only lane
under the 2025 RTI interface. No state gates or claims changed. The edition
chart does not imply 2010 RTI behavior or cross-stream parity. GitHub review of
local revisions remains pending publication. No builds, tests, CI, or
Requirements Lab work.

The TSO review below completes this handoff.

## 2025 TSO retraction diagram review — 2026-10-14

Rendered all ten Mermaid diagrams in `HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md`
with Mermaid CLI 12.0.0. Visually inspected five: the portable HLA outcome
orientation, producer-time/lookahead gate, regional FQR/TARA/NMRA sequence,
ownership timeline, and corrected process-route sequence. The ownership flow
was reoriented top-down and the regional sequence labels shortened. Fixed
Mermaid sequence syntax in notes and labels; no transition or evidence claim
changed. The diagrams keep the HLA-level orientation separate from selected
embedded 2025 scenarios. Do not infer normative equivalence, process parity,
or 2010 behavior. GitHub verification of later revisions remains pending
publication; the timestamped-delete and per-recipient charts retain their
2026-10-09 GitHub visual review. No builds, tests, CI, or Requirements Lab.

The reservation review below completes this handoff.

## 2025 object-instance name reservation diagram review — 2026-10-14

Freshly rendered all seven diagrams in
`HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md` with Mermaid CLI
12.0.0 and visually inspected every chart. The owner/peer map clearly keeps
reservation consumption separate from later discovery. The vertical lifetime
state chart still spanned too widely because of long edge labels, so shortened
those labels while preserving failed attempt, explicit release, resignation
before registration, named registration, global object/name purge, and
save/restore edges. No state transition or evidence changed. The 2010 section
remains separate; the two focused embedded tests are complementary, not one
end-to-end case. GitHub rendering of the current revisions remains pending
publication. No builds, tests, CI, or Requirements Lab work.

## 2025 exception-reporting route review — 2026-10-14

Rendered and visually inspected all six Mermaid diagrams in
`HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md`. The two long Route A paths were
separated at the actual backend boundary into embedded-registry and
process-endpoint sequences, with shorter participant labels. This makes the
shared HLAreportException result easier to follow without suggesting common
internal scheduling. The caller's typed exception, observer-side
HLAreportException, rejected-MOM HLAreportMOMexception, and separate
HLAreportServiceInvocation remain distinct. The combined process two-report
trace is still source-derived; its selected tests do not jointly enable both
switches or establish report arrival order. No test/build/CI, 2010, or
Requirements Lab work. GitHub review of the revised six-chart guide remains
pending publication.

Next handoff: locally render and inspect the diagrams in
`HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md`. Focus on
private request correlation versus pushed event frames and explicit polling,
callback-model timing, and the per-recipient TSO callback/ack boundary. Keep
this 2025 process path distinct from the separate 2010 reference RTI; do not
rerun builds, tests, or CI for this documentation review.

## 2025 process-endpoint request and callback diagram review — 2026-10-14

Rendered all 11 Mermaid diagrams in
`HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md` with Mermaid
CLI 12.0.0 and visually inspected each local preview. Reorganized sequences at
real ownership boundaries: sender request versus recipient stream; event
acquisition versus callback-bridge scheduling; receive-order attribute
planning/commit versus recipient delivery; and TSO callback outcome versus the
private completion-ACK exchange. Reoriented the HLA/API-to-process identity map
and polling-family selector top-down, and shortened long labels. Fixed a nested
sequence-note placement required by the renderer. These are navigation and
presentation changes only; the requestId, TSO messageId, timestamp,
MessageRetractionHandle, and public-void/private-result distinctions remain
separate. The callback-throw/no-ACK path and post-Resign pushed-session cleanup
remain source-derived and explicitly unverified by a focused runtime case; the
historical 284 focused public process-TSO assertions do not cover those gaps.
No 2010 inference, builds, tests, CI, or Requirements Lab work. GitHub review
of this revised guide remains pending publication.

## 2025 known-object subscription scope-change chart review — 2026-10-14

Extracted and rendered only the revised scope-change Mermaid block from
`HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md` with Mermaid CLI
12.0.0, then visually inspected the preview against the adjacent explanation.
Its tall top-down layout is readable. The receiver scope-advisory gate and
the newly added subscribed-attribute value question visibly branch
independently from the same per-attribute scope delta. The text keeps embedded
initial reflection on a newly discovered object and Auto Provide distinct;
the already-known ordinary-object value result remains open. The separate
process path still identifies only the MOM initial-value event and does not
imply ordinary process behavior. No source edit or normative inference was
warranted. No builds, tests, CI, Requirements Lab, or 2010 comparison.

## 2025 Attribute Value Update Request entitlement review — 2026-10-14

Compared the pinned §6.21 API declaration, bounded provider-planning and
ordinary-receiver source, and the focused embedded/process request tests. The
vendored header provides the method signature and exception list, not the
normative clause prose; the official IEEE listing presents standard access by
purchase or subscription. Added that access pointer beside the guide's open
question. The existing HLA-facing sequence and prose already separate the
provider solicitation from a later value-bearing update and its receiver
eligibility; no diagram change was warranted.

The inspected ordinary provider planner selects current attribute owners;
the later ordinary update recipient planner separately applies subscription
and regional criteria. This is source evidence, not a normative ruling.
Embedded request/response examples subscribe the requester. The configured
process object-request fixture seeds subscription state directly in the
registry before the provider's later ordinary update. The process class-request
case ends after the provider callbacks and has no later value response. No
focused case tests an unsubscribed requester receiving a provider response.
The RTI-owned MOM direct-reflection exception remains separate from ordinary
provider solicitation. Keep direct response entitlement under §6.21 open
until authorized clause text is available; do not infer it from code or these
tests. No builds, tests, CI, Requirements Lab, or 2010 comparison.

## 2025 factory/RID boundary diagram review — 2026-10-14

Rendered only the fourth Mermaid chart in
`HLA-2025-CONNECTION-AND-CONFIGURATION-FLOW-GUIDE.md` with Mermaid CLI 12.0.0
and visually inspected the local preview. Shortened participant display
labels and the HLAauthorizer branch text after the first preview showed
unnecessary horizontal stretch; the edit changes no branch, ordering, or
timing. The chart reads clearly and separates factory construction of a
disconnected ambassador from Connect-time environment/RID/profile gates and
the post-commit result. The cited focused test still failed during temporary
RID-file setup before assertions; it was not rerun and supplies no runtime
evidence. This remains a source-grounded flow. No other chart was rendered;
no build, test, CI, Requirements Lab, or 2010 comparison was performed.

## 2025 owner-switch and pending-advisory audit — 2026-10-14

The selected embedded owner-switch case drains callbacks while the switch is
off, then re-enables it before a fresh active subscription. It does not cover
a persistent relevant state or pending callback across off/on. Selected
process callback tests enable the switch during setup only. Source review
shows the setter itself does not run the relevance transition planner, but
embedded and process delivery have distinct later checks: embedded rechecks
when its queued callback runs; process checks at event admission/receive and
again at callback dispatch. Previously planned/admitted work may pass if the
switch and expected state satisfy those gates when reached. This is a 2025
source observation, not normative evidence or profile equivalence. The guide
now records that distinction in prose and its evidence map; no diagram or
runtime test was added. No 2010 comparison, build, test, CI, or Requirements
Lab work.

## 2025 process class-level declaration advisory audit — 2026-10-14

Bounded process-source review found remote getter operations for the object-
and interaction-class relevance switches, but no dedicated wire SET
operations; the public Set methods use the embedded-registry path. The process
HLAsetSwitches MOM route updates stored member switches, and its focused test
sends that interaction and checks getters, not direct Set APIs or declaration
callbacks. Process declaration handlers do not call the class-level planner,
and the surveyed process callback route contains no class-level advisory event
or callback case. Existing focused class-level advisory tests are embedded;
the separate process per-instance Turn Updates tests do not establish this
callback family. Updated the guide to scope the class-level flow accordingly.
Source-only, not normative or parity evidence; no new diagram, build, test,
CI, 2010, or Requirements Lab work.

## 2025 process explicit joined-federate MOM value request — 2026-10-14

The 2025 MIM requires a current-value response for a request on the
`HLAfederate` MOM class/instance even when the requested attribute is
Conditional. The configured process path validates the public handles and
membership, checks server-side object-known state and attribute definitions,
projects the current MOM/member values, and returns an RTI-owned attribute
event without soliciting a federate `provideAttributeValueUpdate` callback.
The event is pushed or polled and then reaches `reflectAttributeValues` through
the callback bridge. A focused two-federate `HLA_EVOKED` test requests
`HLAreportServiceFile` and asserts one current-value reflection. It subscribes
before the request and does not test the no-subscription path, rejection cases,
other callback models, or automatic Conditional update triggers. The Federation
MOM guide now distinguishes the request gates/projection from the transport and
callback-delivery stages; the 13 charts in that guide rendered, and the three
new MOM charts were visually inspected. No C++ changes, build, tests, CI,
Requirements Lab, or 2010 comparison.

Next handoff: compare a normal 2025 process object-instance
`requestAttributeValueUpdate` with the RTI-owned joined-federate MOM branch.
Trace provider selection and `provideAttributeValueUpdate` solicitation versus
the direct MOM projection, then receiver `reflectAttributeValues` delivery.
Reuse the existing Attribute Value Update Request guide and focused process
test; add or revise a diagram only for a concrete reusable reader gap. Keep
2010 separate. No builds, tests, CI, or Requirements Lab.

## 2025 process ordinary AVU versus RTI-owned joined-federate MOM — 2026-10-14

Compared the configured-process object-instance request paths. For ordinary
objects, the registry requires a requester-known instance and groups requested
attributes by current remote owner; the service delivers a request-tagged
`provideAttributeValueUpdate`, and any provider response is a separate update
with its own tag before receiver reflection. For an RTI-owned joined-federate
MOM object, the service checks membership, known-object state, and defined
attributes, projects current values, and directly reflects them to the
requester without a federate Provide callback. This is source-derived process
behavior, not a portable response-entitlement rule or a process/embedded
parity claim.

The existing configured-process ordinary instance test observes Provide,
later update, and reflection under both `HLA_EVOKED` and `HLA_IMMEDIATE`; the
joined-federate `HLAreportServiceFile` test observes the direct current-value
reflection under `HLA_EVOKED` only. The tests were inspected, not rerun. Added
one ownership-fork flowchart to the AVU guide and linked the process handler,
registry planner, callback bridge, and focused tests. Mermaid CLI 12.0.0
rendered all six charts in that guide; the new comparison chart was visually
reviewed. A fresh docs/design census counts 292 diagrams in 34 of 43 Markdown
files: 164 flowcharts, 105 sequences, and 23 state diagrams. No C++ changes,
build, test reruns, CI, Requirements Lab, or 2010 comparison.

Next handoff: audit callback observation timing for the 2025 process AVU path
against the callback/service-ordering guide. Use the existing ordinary-request
case under both callback models to distinguish the service request, callback
observation, and separate provider update; keep API-level behavior separate
from Umbra dispatch details. Add a chart only for a demonstrated reader gap.
Keep 2010 separate. No builds, tests, CI, or Requirements Lab.

## 2025 process AVU callback-observation evidence boundary — 2026-10-14

Compared the ordinary process AVU test sequence with the existing callback
guide. The test returns from `requestAttributeValueUpdate` before its next
checkpoint: `HLA_EVOKED` explicitly calls `evokeCallback`, while
`HLA_IMMEDIATE` uses a later `getObjectClassHandle` lookup that the harness
labels an immediate-callback flush before asserting Provide. The same pattern
is used after the provider's later `updateAttributeValues` call before the
reflection assertion. The test therefore shows the callback is observed by
those later checkpoints in both selected configurations; it does not record a
before-return/after-return callback boundary or cross-federate order.

The callback guide already explains the generic distinction: HLA_EVOKED leaves
callback dispatch to an Evoke service, while HLA_IMMEDIATE can enter callback
code while its inducing service is still executing. That rule does not settle
this AVU-specific integration timing. Added this evidence boundary and links
to the AVU guide; no duplicate chart was warranted. No source changes, test
reruns, build, CI, 2010 comparison, or Requirements Lab work.

Next handoff: audit the 2025 Object and Interaction Information Flow guide at
the boundary between per-federate object-known/discovery state and the first
attribute reflection. Cross-read the focused Object Attribute Subscription
State guide and bounded pinned API, source, and test evidence. Separate the
normative callback sequence from source/test commit points and observation;
add or revise a diagram only for a demonstrated reader gap. Preserve the
2010/2025 boundary and do not infer process/embedded parity. No builds, tests,
CI, or Requirements Lab.

## 2025 discovery and first-reflection boundary review — 2026-10-14

Cross-read the broad Object and Interaction Information Flow guide and the
focused Object Attribute Subscription State guide. The pinned
`FederateAmbassador` header exposes callback signatures and labels §§6.9/6.11,
not normative clause text. In the selected embedded 2025 path, ordinary-object
initial values are planned after `discoverObjectInstance` returns; the focused
late-subscription test observes discovery and the selected stored value in
separate callback collections, so it proves occurrence/value but not relative
callback order. The process receive selector gives a pending discovery its
branch before the ordinary attribute-event slot after earlier-priority event
checks, while the focused public process test sends a separate explicit update
and records discovery/reflection as independent booleans. The process discovery
enqueue's follow-up initial-value event is MOM-only. These are profile-scoped
source/test observations, not a normative callback-order or embedded/process
parity claim; exact §5.8/§6.9/§6.11 prose remains unverified.

The detailed receiver-state table and callback chronology already live in the
focused subscription guide; no duplicate chart was warranted. Added a concise
embedded-versus-process summary to the broad guide. No diagram changed, so no
render or visual review was needed. No source edits, test reruns, build, CI,
2010 comparison, or Requirements Lab work.

Next handoff: audit application observation timing for the embedded 2025
discovery/initial-reflection path under HLA_EVOKED and HLA_IMMEDIATE. Trace the
public subscription call, callback admission/dispatch, and focused tests;
separate the public callback-model contract from Umbra's implementation timing
and what the selected tests assert. Add a chart only for a demonstrated reader
gap. Keep process and 2010 evidence separate; no builds, test reruns, CI, or
Requirements Lab.

## 2025 embedded discovery callback-model observation — 2026-10-14

The focused late-subscription test connects with HLA_EVOKED and drains callbacks
only after `subscribeObjectClassAttributes` returns. It observes discovery and
the filtered stored-value reflection, but stores each callback family in a
separate vector; it has no HLA_IMMEDIATE variant and asserts neither their
relative order nor callback entry before the service returns. In the embedded
source, the subscription commits declarations and plans discovery before
submitting a callback task; that task enters `discoverObjectInstance`, then
plans and invokes ordinary initial reflections after the discovery callback
returns. The callback route uses the ambassador dispatcher: HLA_IMMEDIATE can
drain on the submitting thread, while a submission made during another
thread's active immediate drain remains queued. Thus the source and this
HLA_EVOKED test do not establish an unconditional Subscribe-before-return
observation rule for HLA_IMMEDIATE. The generic callback guide already charts
the model distinction; no duplicate chart was warranted. Added a concise
callback-model evidence note to the broad object guide. No source edits, test
reruns, builds, CI, 2010 comparison, or Requirements Lab work.

Next handoff: audit the callback-visible known-instance boundary for embedded
2025 discovery. Check which instance state is committed when
`discoverObjectInstance` begins, whether an application can use matching
object support services reentrantly from that callback, and which focused
tests demonstrate either behavior. Separate the portable HLA-facing effect
from Umbra's registry commit timing; add a diagram only for a demonstrated
reader gap. Keep process and 2010 evidence separate; no builds, tests, CI, or
Requirements Lab.

## 2025 embedded discovery callback-visible support state — 2026-10-14

The embedded discovery task calls the registry transition, releases the
federation-state lock, and then enters `discoverObjectInstance`. The source
comment explicitly says matching 2025 support services may be used from that
callback. `getKnownObjectClassHandle` reads the committed receiver-known
entry. The pinned API provides the support-service signature and clause label,
but not the normative callback-reentry prose. The focused ownership-query test
evokes and records discovery before calling `getKnownObjectClassHandle`; a
bounded scan found no focused case making that call from inside
`discoverObjectInstance`. Thus reentrant usability is source-supported Umbra
behavior, not independently test-verified or a portable normative conclusion.

Added this source/test boundary to the existing object guide. Its discovery
sequence and commit-point notes already cover the state transition; another
chart would duplicate those visuals. No diagram changed or required rendering.
No source changes, test reruns, builds, CI, 2010 comparison, or Requirements
Lab work.

Next handoff: audit the distinct receiver- and owner-facing effects when a
2025 subscription makes an existing object newly discoverable: receiver
discovery/initial values, owner `Turn Updates On/Off` advice, and the separate
Auto Provide solicitation. Cross-read the focused subscription, relevance
advisory, and Auto Provide guides with their selected tests. Preserve callback
actor, payload, and observation boundaries; do not infer a universal order or
process/2010 parity. Add or revise a diagram only for a demonstrated reader
gap. No builds, tests, CI, or Requirements Lab.

### 2025 late-subscription effects and state/event boundary — 2026-10-14

Rechecked the object-information, subscription-state, relevance-advisory, and
Auto Provide guides against the selected source/test evidence. Their existing
flows distinguish receiver discovery and embedded initial values, owner-only
`Turn Updates On/Off` advice (no values), and provider-facing Auto Provide
solicitation (which itself supplies no values). The focused evidence does not
establish a universal callback order, and the process route remains a separate
boundary: ordinary initial values are not demonstrated by the reviewed process
case; the process initial-value event is limited to RTI-owned MOM objects.

Added a short HLA-first comparison to the DSE guide: newly discovered object
state versus one specific interaction send retained for delivery-time
eligibility. The embedded late-object test observes discovery and selected
values but not their relative order; the receive-order DSE interaction test
shows a later subscription admitting the retained send only in its enabled
case. Exact §§5.8 and 8.1.8 wording remains unverified against licensed text.
The existing diagrams already show each state transition, so no new chart or
render was warranted. No source changes, test reruns, builds, CI, 2010
comparison, or Requirements Lab work.

Next handoff: audit the 2025 already-known-object case where a receiver adds an
attribute subscription after discovery. Determine, from bounded embedded and
process source/test evidence, whether a current value is reflected without a
new publisher update; keep the RTI-owned MOM exception and DSE-retained sends
separate. Preserve the §5.8 verification limit and add a diagram only if a
reader gap remains. No builds, tests, CI, or Requirements Lab.

### 2025 already-known receiver adding an attribute — 2026-10-14

The inspected embedded subscription mutation returns scope-change and
attribute-relevance plans, not ordinary value recipients. Its discovery
replanner skips an instance already in that receiver's known-object map, while
the ordinary initial-value planner is invoked only after a discovery callback.
The reviewed process handler also replans discovery; the process discovery
enqueue path constructs an initial-value event only for RTI-owned MOM objects.
No ordinary subscription-time snapshot path is visible in these bounded source
paths. This is not a normative conclusion: the licensed §5.8 text remains
unreviewed.

The nearest embedded additive/mode test declares its attributes before object
registration; the later mode changes are followed by explicit publisher
updates before reflection is asserted. The selected process discovery test
subscribes before registration, and the separate MOM test exercises its own
initial-value path. These tests do not isolate an already-known ordinary
object receiving a current value solely because a new attribute was
subscribed. Added the nearest embedded-test limit to the subscription guide;
no additional chart was justified. No source changes, test reruns, builds, CI,
2010 comparison, or Requirements Lab work.

Next handoff: survey the 2010-only DataElement and composite encode/decode
behavior against the pinned 2010 API, reference implementation, and its
encoding/composite smoke sources. Decide whether the existing 2010 guide has a
real reader gap in the data-representation path; add a separate 2010 flow only
if it clarifies HLA concepts and source-backed behavior. Do not compare or
borrow from 2025. No builds, tests, CI, or Requirements Lab.

## 2010 composite decoding — 2026-10-14

The bounded survey found that the separate 2010 DataElement guide already had
two appropriate diagrams for scalar order and composite encoding; the stale
handoff had mistaken this as missing work. Added a third, compact flow for
shape-driven composite decoding: fixed arrays use configured slots, variable
arrays read a signed big-endian count (reject negative; zero stops after the
prefix), fixed records decode declared fields and validate padding, and
variants decode the discriminant before selecting an alternative. Unknown
variant discriminants leave the cursor after the discriminant, while the
public full-buffer decode rejects any remaining bytes. Truncation and invalid
padding also fail in the inspected 2010 implementation.

The chart is source-observed profile behavior, not a normative algorithm
claim. The dedicated smoke source covers fixed/variable round trips and count
or encoded-length observations plus rejection of nonzero fixed-record
padding; it does not independently cover negative counts, truncation, trailing
bytes, or unknown variant discriminants. All three diagrams in the guide
rendered locally; the new chart was visually reviewed after a layout revision.
No tests, build, CI, Requirements Lab, or 2025 code or behavior were touched.

Next handoff: bounded 2010-only review of logical-time value representation,
ordering, and arithmetic against the pinned time API, separate 2010
implementation, and focused smoke sources. Decide whether those value
semantics have a genuine reader gap; do not conflate them with time regulation,
advancement, or grants, and do not compare with or borrow behavior from 2025.
No builds, tests, CI, or Requirements Lab.

## 2010 logical-time values and the RTI service boundary — 2026-10-14

The 2010 reference guide already said that value operations do not implement
time advance, but its diagram reduced all value behavior to a generic
construct/arithmetic/encode box. Reworked that existing diagram into two
clearly separated layers. The first follows the pinned abstract value/factory
API and Umbra's integer64/float64 models through matching `LogicalTime` and
interval values, initial/final and zero/epsilon sentinels, comparison and
time/interval arithmetic, and value serialization. The second shows only the
2010 ReferenceRTI boundary: execution creation accepts but does not wire a
time-implementation name, and time-management/timestamped ambassador calls
reach the generated `unavailable()` path.

The pinned headers state that the federation provides the time representation
and factory; this is an API contract, not a complete clause-level standard
interpretation. The integer, float, and marshalling smokes cover selected
sentinels, arithmetic, factory selection, and encodings, but do not exercise
the ambassador stubs. The reference-profile source shows type/range checks;
uncovered type-mismatch and error branches remain source-only. The final chart
was rendered locally and visually inspected in a compact, readable vertical
layout. It replaces the earlier chart; the diagram count is unchanged. No
tests, build, CI, Requirements Lab, or 2025 behavior were touched.

Next handoff: audit the existing 2025 federation/federate lifecycle guide for
an HLA-first membership/execution mental model across Join, Resign, connection
loss, and Destroy Federation Execution. Compare only bounded 2025 API, source,
and existing focused-test evidence; revise or add a diagram only for a real
reader gap. Preserve embedded/process distinctions and keep 2010 separate.
No builds, tests, CI, or Requirements Lab.

## 2025 federation lifecycle audit and visual review — 2026-10-14

The bounded guide/API/source/test survey found no missing top-level lifecycle
map: the guide already distinguishes ambassador connection, federate
membership, and execution lifetime, then shows Create/Join/Destroy separately
from Resign/Disconnect and forced connection loss. That is clearer than another
combined state chart, so no duplicate HLA map was added.

Rendered all 15 Mermaid diagrams in the current lifecycle guide. Visual review
found its active-save resignation sequence too dense: registry internals and
callback-polling calls obscured the key HLA teaching point. Reworked the
sequence around the focused `HLA_EVOKED` scenario: a Resign attempted inside
the save-initiation callback fails, the retry after callback return succeeds,
the active Save fails for survivors but not the leaver, and the survivors can
begin a later Save. Removed semicolons from Mermaid labels after the renderer
identified them as statement separators. The corrected sequence renders and
was visually reviewed. The separately reviewed connection-loss TSO cutoff
flow remains tall but readable; its exact §4.4 wording is still unverified
against the full standard. The revised sequence is one selected embedded 2025
test, not a global callback-order or process/2010 parity rule.

No tests, builds, CI, Requirements Lab, or 2010 behavior were changed. Local
rendering does not replace the post-publication GitHub check.

## 2025 FOM admission/composition failure audit — 2026-10-14

Audited the existing 2025 guide against the pinned Create/Join API, module
coordinator, composer, embedded lifecycle path, and focused tests. Its four
diagrams already distinguish module-local DIF validation, ordered full-set
composition and reference resolution, FDD validation, Create's
prepare-before-reserve boundary, and the registry-atomic Join from later
post-commit setup. No new flow was justified. Added an embedded
preparation-to-exception map and compared all pinned Create/Join overloads with
the selected adapter paths. Recorded four source/API differences: vector
Create can translate selected-MIM errors omitted from its API list; Create with
an explicit MIM can translate unavailable-time errors omitted from its list;
additional-module Join can translate failures from revalidating its retained
MIM although Join lists no MIM exceptions; and single-module Create lists a
standard-MIM-designator exception its embedded delegation path cannot produce.
These are implementation/API observations, not HLA-wide behavior; focused
public-adapter coverage is absent. The pinned headers are byte-identical to
the canonical IEEE API archive per the local manifest; the full prose standard
is not vendored, so no additional normative rule was inferred. No build, test,
CI, Requirements Lab, or 2010 change.

Next handoff: continue the bounded 2025 Request Attribute Value Update reader-
gap review in `HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md`, focusing
on the open direct-requester response-entitlement question. Reuse the pinned
API, current state-planning source, and existing focused cases; keep its
normative status unresolved unless exact 1516.1 clause text is available. Add
no duplicate diagram without a demonstrated reader gap. Keep 2010 separate;
no broad builds, tests, CI, or Requirements Lab.

## 2025 DataElement byte-order boundary audit — 2026-10-14

Rechecked the 2025 encoding guide's endian story against the pinned API, the
explicit `ByteOrder` primitive, integer/float helpers, variable-array codec,
and focused vectors. The guide already distinguishes element-selected BE/LE,
fixed-format BE encodings, one-octet values, child-owned composite encoding,
factory-defined logical time, and Umbra's separate big-endian process frame.
The variable-array count is `HLAinteger32BE`; the source/test path also rejects
negative counts and malformed alignment padding. Existing diagrams cover the
representation and composite decisions, so no new flow or wording change was
justified. No C++ checks, builds, CI, or 2010 changes were made.

Next handoff: boundedly audit the 2025 process-endpoint callback-after-Resign
boundary in `HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md`. Trace
queued-task/session cancellation through Resign and public Evoke/Enable against
the focused bridge tests; separate source-supported reachability from HLA
permission, and revise a diagram only for a concrete teaching gap. Keep 2010
separate; no builds, test reruns, CI, or Requirements Lab.

## 2025 Process callback stream and Resign timing audit — 2026-10-14

The bounded Resign trace found that process callback work crosses three
different cut points: server-side session queues, client-buffered event frames,
and tasks already submitted to the shared callback dispatcher. The server
clears its still-queued joined-session event state before returning success.
The process client's default correlated-request path drains frames already
buffered at its explicit dispatch point before the client Resign method returns;
enabled HLA_IMMEDIATE work may invoke before the public Resign returns, while
HLA_EVOKED or disabled-immediate work can remain queued. Frames arriving after
that explicit drain resume through the background dispatch loop once request
suppression lifts, with no source-established ordering against the public
return. The client clears its joined identity and cancels exception-report
projections; the adapter then applies Resign and clears its membership fields
without resetting the shared dispatcher. `close()` is the path that closes the
callback session and resets that queue.

No focused test establishes unrelated pending-task delivery after public
Resign or the timing of a pushed frame racing the explicit drain. The bridge
test covers exception-projection cancellation only; the process Evoke test
drains its selected callbacks before Resign. No normative post-Resign service
permission was inferred. This audit also corrected stale process-endpoint-guide
text that said the client lacked a background reader: the client starts a
stream-reader thread and a separate callback-dispatch thread. Updated two
existing process-endpoint diagrams and the callback/Resign diagram; Mermaid
CLI 12.0.0 rendered all three and they were visually reviewed. One sequence
label's semicolons were replaced after the first render caught Mermaid parsing
them as separators. No tests/build/CI, Requirements Lab, or 2010 changes.

Next handoff: render the four additional diagrams and inspect the open
ownership-timeline chart in `HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md`. Check the
portable receiver outcomes separately from producer-time, regional, and
ownership test boundaries; update backlog row 12 and the review ledger. Keep
2010 separate; no builds, test reruns, CI, or Requirements Lab.

## 2025 TSO retraction visual-review follow-up — 2026-10-14

Rechecked the current TSO guide handoff against the dated ledger: the ownership
timeline had already been locally reviewed, so that part of the pointer was
stale. Refreshed that chart and locally rendered/inspected four additional
charts: the per-recipient state machine, mixed-recipient interaction sequence,
focused producer-time boundary sequence, and process receiver-Resign route.
All ten Mermaid blocks have local render evidence; nine of ten now have local
visual review. The timestamped-delete diagram retains its older 2026-10-09
GitHub visual review and is the only chart without current local visual
inspection. The portable outcome model remains an orientation, not a complete
§§8.22–8.23 decision table. Mixed fan-out and producer-time examples are
selected embedded 2025 cases; the process receiver-Resign route is source-only
and does not prove a post-Resign public callback. No C++ tests/build/CI, 2010,
or Requirements Lab work was done.

Next handoff: trace the 2025 process `HLAsetSwitches` path from per-member
switch storage and getter readback to the `HLAfederate` MOM attribute
projection. Separate server state from observer reflection, check the focused
test gap, and update the cross-cutting state/event observation guide only for
a demonstrated reader gap. Keep embedded, process, and 2010 boundaries
explicit; no builds, test reruns, CI, or Requirements Lab work.

## 2025 process switch-observation map and TSO final local review — 2026-10-14

The detailed Federation MOM guide and review ledger already traced process
`HLAsetSwitches` storage, getter readback, current `HLAfederate` value
projection, and the missing automatic Conditional reflection for object-class
and interaction-relevance switches. The cross-cutting guide did not make the
control-surface distinction easy to find, so its switch map now contrasts the
embedded direct RTIambassador setter/reflection path with the process Federate
MOM message path and links to the detailed trace. This is a source/test-scoped
process observation, not a conformance conclusion or backend/2010 parity
claim.

Rendered the complete TSO retraction guide with Mermaid CLI 12.0.0 and
visually inspected its remaining timestamped-delete restoration chart. Its
callback-entered restoration/filter/retraction route and no-callback pending-
reservation cancellation route are legible without clipping. All ten TSO
diagrams now have local render and visual-review evidence; GitHub rendering of
current revisions remains pending publication. No C++ tests/build/CI,
Requirements Lab, or 2010 behavior work was performed.

Next handoff: render and visually audit all current diagrams in the 2025
Attribute Ownership guide. Check the ownership service-selection map and
process-specific flows for readable layout and clear portable-HLA versus
implementation-evidence boundaries; fix only concrete problems. Keep 2010
separate and do not build, rerun tests, start CI, or access the Requirements
Lab.

## 2025 DDM guide full local render and visual review — 2026-10-14

Rendered all 20 current Mermaid blocks in the Data Distribution and Regions
guide with Mermaid CLI 12.0.0 and visually inspected every chart. The complete
pass found two inline-code labels in the Region deletion gate that prevented
parsing; removing only the label formatting allowed the chart to render. The
Region-template lifecycle state chart was changed from an excessively wide
left-to-right layout to a compact top-down layout. The multi-recipient process
sequence was narrowed from seven lanes to four by combining client/service
roles while retaining strict and relaxed DDM outcomes, source and receiver
ranges, independent recipient selection, and the separate callback models.
The focused test source shows A1/A2 as the two message tags, so the diagram now
matches those values. Its explanatory note explicitly omits client/server
framing and does not claim cross-receiver callback ordering. All 20 charts
render and are locally visually reviewed; GitHub rendering remains pending
publication. The cited test was inspected but not rerun. No build, test, CI,
Requirements Lab, or 2010 behavior work was performed.

## 2025 Attribute Ownership guide full render and visual review — 2026-10-14

Rendered all 18 current Mermaid diagrams in
`HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md` with Mermaid CLI 12.0.0 and
visually inspected the complete set. The full pass found a sequence-message
semicolon that broke Mermaid parsing; replacing it with “and” fixed the chart
without changing its meaning. The images also showed dense API-name labels in
the regular acquisition, If-Available, negotiated-divestiture, process
cancellation, and If-Wanted sequences. Shortened those diagram labels to the
HLA action/result while retaining exact API names and test/source citations in
the surrounding guide. The revised charts were re-rendered and visually
checked. Long charts remain scrollable but have no clipped content; GitHub
rendering is still pending publication. This was a documentation-only pass:
the focused test sources were not rerun, and no build, CI, Requirements Lab,
or 2010 behavior work was performed.

## 2025 FOM admission/composition guide full local render review — 2026-10-14

Rendered all four current Mermaid diagrams in
`HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md` with Mermaid CLI
12.0.0 and visually inspected each. All parse successfully and are legible at
full size or while scrolling. The Create-preparation, ordered-composition and
FDD-validation, and additional-module Join commit/rollback paths remain
distinct. The edition-selection chart clearly isolates the optional 2010
catalog-only route from the 2025 FDD path. No visual or syntax correction was
warranted. This review does not revalidate implementation semantics or tests;
no build, test, CI, Requirements Lab, or 2010 behavior work was performed.

Next handoff: render and visually audit all six current diagrams in the 2025
Exception-Reporting guide. Check caller typed exceptions separately from
observer-side exception/MOM/service-invocation reports, and retain the known
combined-process-report and arrival-order evidence limits. Keep 2010 separate;
do not build, rerun tests, start CI, or access the Requirements Lab.

## 2025 Exception-Reporting guide full local render and visual review — 2026-10-14

Rendered all six current Mermaid charts in
`HLA-2025-EXCEPTION-REPORTING-FLOW-GUIDE.md` with Mermaid CLI 12.0.0 and
visually inspected the complete set. All charts parse. PNG review found
crowded labels and participant-name overflow in the embedded/process report
sequences, callback-exception boundary, and combined process-report trace.
Shortened natural-language messages and lane names while preserving exact
report protocol names, distinct caller/API/hook/client/server/observer roles,
the `noexcept` catch boundary, and the caveat that combined request order does
not prove callback arrival order. Rerendered all six and visually rechecked
the revised charts. No C++ tests/build/CI, Requirements Lab, or 2010 behavior
work; GitHub rendering awaits publication.

## 2025 Service Invocation Reporting guide full local render and visual review — 2026-10-14

Rendered all eight current Mermaid diagrams with Mermaid CLI 12.0.0 and
visually reviewed the complete set. Shortened labels in the service-reporting
interlock and final-resignation transition, removed a redundant report-write self-loop from
the per-membership report-file lifecycle, and compacted the post-commit
ConnectionLost self-messages after previews showed overlap or clipping. The
switch/sink gate remains in prose alongside the exact interlock exceptions,
stable membership path, and registry-commit versus RTI post-commit
cleanup/failure boundary. The final eight-chart render succeeded and revised
charts were visually rechecked.
This documents existing source/test evidence; it is not a fresh semantic or
test verification. GitHub review awaits publication. No builds, tests, CI,
Requirements Lab, or 2010 behavior work.

## 2025 Callback and Service Ordering guide full render and visual review — 2026-10-14

Rendered all 12 current diagrams with Mermaid CLI 12.0.0 and visually reviewed
the full set. The process synchronization sequence was especially dense, so
shortened participant labels and message text using the adjacent explanation
for detail. Its registration and achievement paths remain distinct, including
private request responses versus recipient events, push versus polling, report
submission order, callback-model dispatch, and the source/test boundary. All
12 diagrams rendered after the revision. This is a layout review, not a fresh
source or test audit. GitHub review awaits publication. No builds, test reruns,
CI, Requirements Lab, or 2010 behavior work.

## 2025 object discovery and first-reflection boundary audit — 2026-10-14

Bounded inspection confirmed that the embedded path commits receiver-known
state as discovery dispatch begins, calls `discoverObjectInstance`, and then
plans current subscribed values. Its HLA_EVOKED late-subscription test asserts
one discovery and one stored-value reflection but records them separately, so
the test itself does not assert order. The process pull/`receive_interaction`
selector prioritizes its discovery event over the later attribute-reflection
slot when that part of the selector is reached; other event-family priorities
remain separate. The focused public process case sends an explicit update and
asserts both callbacks using separate flags, not their order. The ordinary
process discovery path has no initial-value event in the inspected source;
that branch is limited to RTI-owned MOM objects. This is source/test evidence,
not a conformance finding: the checkout provides API declarations and clause
labels, but not normative §5.8/§6.9/§6.11 text for independent verification.

The companion subscription-state guide already contains a visual
embedded/process late-subscription comparison, so no duplicate chart was
added. Clarified that this guide's process ordering claim is pull-selector
specific and linked directly to that existing chart. No tests were rerun,
builds or CI started, or 2010/Requirements Lab work performed.

Next handoff: audit the 2025 discovery-triggered Auto Provide path against
initial-value reflection and explicit Request Attribute Value Update. Compare
the callback purpose, trigger, tag, value-delivery boundary, process route, and
focused evidence; reuse existing diagrams and add or revise one only for a
demonstrated reader gap. Keep 2010 separate; no builds, test reruns, CI, or
Requirements Lab.

## 2025 Auto Provide neighboring-path audit — 2026-10-14

Reviewed the existing Auto Provide comparison, its discovery/provider-fan-out
diagram, focused embedded cases, and bounded process request-routing evidence.
The guide already distinguishes discovery-triggered owner solicitation (empty
automatic tag), explicit Request Attribute Value Update (caller tag), current
value reflection, and the RTI-owned joined-federate MOM direct-reflection
exception. Its evidence section correctly limits verified Auto Provide
planning to the embedded path; process explicit-request routing and switch
mutation do not establish process automatic-discovery parity. No redundant
diagram was added. Added a direct link from the object-information guide's
first-reflection boundary to the existing neighboring-path comparison. No
tests/build/CI, Requirements Lab, or 2010 work.

Next handoff: trace the 2025 process object-instance Request Attribute Value
Update from the public service through request encoding, process handler,
provider callback, and later provider update/reflection. Compare tag ownership
and the joined-federate MOM direct-reflection exception with focused evidence;
do not infer class/regional parity or Auto Provide parity from this route.
Reuse existing diagrams and change them only for a demonstrated reader gap.
Keep 2010 separate; no builds, test reruns, CI, or Requirements Lab.

## 2025 process object-instance request transport map audit — 2026-10-14

The current request guide already explains the process HLA-level object/MOM
branch, request-tag versus provider-update-tag ownership, and the separate
value-delivery step. Its focused configured-process case checks both
`HLA_EVOKED` and `HLA_IMMEDIATE`, the request tag at Provide, and the distinct
response tag at reflection. Bounded inspection confirmed the private chain:
public adapter → process client/codec → service decode/membership check →
per-provider request event → callback bridge. Added this as an explicitly
implementation-only map, not an HLA wire-protocol diagram or claim. Existing
HLA flow diagram was sufficient; no extra chart or test run.

Next handoff: audit the 2025 FOM module admission/composition guide across
validation, ordered merge, cross-module resolution, and Create/Join commit
boundaries. Compare HLA-first diagrams with bounded API/source/test evidence;
keep 2010 separate and avoid builds, test reruns, CI, and Requirements Lab.

## 2025 FOM module admission/composition reader audit — 2026-10-14

Rechecked the Create, per-module admission, ordered complete-set merge,
cross-module resolution, and additional-module Join commit boundaries against
bounded implementation and focused-test evidence. The commit atomically
replaces the federation definition/handle directories with membership, while
catalog lookup, existing object knowledge/subscriptions/ownership, ordinary
discovery/delivery, and Federation MOM `HLAcurrentFDD` reflection remain
distinct observations. Existing embedded tests cover new catalog lookup and
the MOM reflection; the configured-process test only checks the joining
client's new object-class lookup. The inspected Join commit does not directly
invoke the ordinary discovery planner or synthesize Receive Interaction; that
is a source observation, not a normative conclusion. No focused configured-
process post-Join interaction-delivery case was found.

Added one fifth Mermaid flowchart to make the catalog/runtime/MOM separation
visible. The previous four charts were already rendered and visually
reviewed, but the new chart remains unrendered: Mermaid CLI 12.0.0 is present,
yet Chrome failed to launch in the sandbox. No C++ build, test rerun, CI,
Requirements Lab, or 2010 behavior work occurred.

Next handoff: audit the configured-process post-Join interaction path, keeping
tested catalog lookup separate from the source-only send-time
parent/subscription route. Do not infer delivery without a focused post-Join
case; keep 2010 separate and avoid builds, test reruns, CI, and Requirements
Lab.

## 2025 configured-process post-Join interaction audit — 2026-10-14

Verified the guide's process boundary against the configured public-endpoint
case, private service probe, registry Join commit, and shared interaction
recipient planner. The public case composes an object-only extension, rejects
an invalid extension without changing the base module count, then proves the
new object-class handle/name lookup after Join. It sends no interaction. The
separate inherited-receipt test starts with its complete class hierarchy at
Create and therefore does not test a class introduced by a later Join.

Source inspection confirms that the configured service requires an
`additionalFomPreparation` hook, serializes preparation plus registry Join,
and passes the replacement definition to the same registry commit that
reconciles and swaps interaction-class/parameter directories with membership.
Per-federate interaction declarations are separate state. A later send
resolves the class in the current catalog and walks its parent chain against
stored subscriptions; this suggests a prior ancestor subscription can be
considered for an added descendant, but no focused post-Join send/receive case
verifies delivery. The FOM guide already labels this inference and the test
gap, so no new chart or test was added. No build, test rerun, CI, Requirements
Lab, or 2010 behavior work occurred.

Renderer follow-up: the installed Mermaid CLI was retried with the existing
Edge runtime. Puppeteer failed before parsing or rendering, reporting a running
browser profile and a Windows credential-launch error, as it had with Chrome.
The temporary config was removed. That attempt produced no PNG, leaving the
fifth historically added chart unparsed/unrendered at that point; the failure
did not establish a Mermaid syntax error. Its later successful local render
and visual review are recorded in the post-Join validation section below.

## 2025 Save/Restore Join-contract access recheck — 2026-10-14

Rechecked the official IEEE 1516.1-2025 product page. It presents purchase and
subscription access and links a downloads ZIP, but not the §4.11 clause text.
The prior archive inspection found the machine-readable API package, not the
normative standard. The Save/Restore guide already labels Join admission and
participant effects as unresolved; no diagram or HLA claim was changed. No
build, test rerun, CI, Requirements Lab, or 2010 behavior work occurred.

Next handoff: if an authorized full 1516.1-2025 copy containing §4.11 becomes
available, compare Join exception conditions with Save/Restore participant
effects and update only where the normative text supports it. Do not repeat
the public-page access check or infer HLA rules from code. If the text remains
unavailable, leave the question open and continue the next source/test-backed
reader gap in the portfolio. Keep 2010 separate; no builds, test reruns, CI,
or Requirements Lab.

## 2025 FOM post-Join chart validation and callback-boundary audit — 2026-10-14

The FOM chart described in the earlier handoff as the fifth chart added is the
fourth Mermaid fence in current source order. An initial source-order selection
rendered the existing 2025-API/2010-model lane chart instead; that chart was
also visually checked and remains clearly separated from the 2010 RTI stream.
After matching the current guide's diagram description and source location,
Mermaid CLI 12.0.0 rendered the intended additional-module post-Join chart in a
one-off local headless-browser run outside the sandbox. Visual review found
the catalog, existing live state, Federation MOM reflection, and later-delivery
branches legible without edits. The four older charts retain their recorded
render/review evidence, so all five current FOM-guide diagrams now have local
render and visual-review evidence. GitHub rendering remains pending
publication; this does not resolve the normative Join callback question.

A separate source/test check confirmed both 2025 `ConnectionLost` producers
contain exceptions from `connectionLost` after their respective membership or
local lifecycle transitions. The embedded path also attempts its final
service-report append and completes cleanup/routing before raising an append
error; the process path clears local joined/time state while retaining the
client for its in-flight request to unwind. The focused successful embedded
report case does not throw from the callback, and its previously recorded local
run failed during federation creation before the assertions. The existing
callback and service-report guides already separate these cases and the
untested boundaries, so no duplicate chart was added. No build, test rerun, CI,
Requirements Lab, or 2010 behavior work occurred.

Next handoff: audit 2025 process endpoint callback-exception attribution
across correlated request drains, pushed-event pumps, and HLA_EVOKED against
the callback, exception-reporting, and process request/callback guides plus
their focused tests. Keep producer-side Send Interaction outcomes separate
from recipient request/Evoke call stacks; revise or add a chart only if a
distinct source-backed decision boundary is missing. Do not infer embedded or
2010 parity, repeat the IEEE access check, or rerun local browser rendering
unless the renderer environment changes. No builds, test reruns, CI, or
Requirements Lab.

## 2025 process callback-exception attribution — 2026-10-14

Audited the process receive path across the stream reader, correlated request
drain, background pushed-event dispatcher, callback bridge, and existing
process callback tests. Corrected the wording that attributed inline callback
throws to a “receive pump”: the stream-reader only decodes and buffers. If a
request-backed recipient service drains the event after matching its own
response and releasing the exchange lock, an escaping `HLA_IMMEDIATE` callback
can prevent the process-client request from returning that response. If the
background dispatcher admits the callback, its `noexcept` loop saves the first
exception and the next call to `ProcessFederationClient::request` takes and
rethrows it before sending a request; the pump resumes, and later throws do
not replace the pending first exception. Under `HLA_EVOKED`, the bridge queues
the task and the user callback exception exits the later recipient Evoke path.
The producer's correlated Send Interaction response remains on a separate
process session and is not unwound by recipient callback code.

Added a process-specific sequence diagram to
[`HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md`](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-endpoint-callback-exception-attribution)
and a reciprocal link from the [process endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#2-from-a-stream-frame-to-an-official-callback).
The source chain is linked there. The reviewed focused process cases exercise
successful callback dispatch, not a throwing `FederateAmbassador`; no tests or
builds were run. This is implementation evidence, not a normative HLA
exception-propagation rule. The diagram has not been locally rendered or
visually reviewed, and the public adapter's reporting/exception mapping has
not been traced. No Requirements Lab changes, CI, or 2010 behavior work.

Next handoff: trace one request-drain callback exception and one deferred
background exception through their outer 2025 `RTIambassador` service wrappers.
Determine whether `HLAreportException` or
`HLAreportServiceInvocation` is attempted and which public exception surface
is reached; do not infer a normative rule from wrapper behavior. Keep these
recipient routes separate from producer `Send Interaction`, and preserve the
2010 boundary. Do not build, rerun tests, or start CI; the new diagram's local
render/visual review remains pending.

## 2025 process callback exception at the public wrapper — 2026-10-14

Followed both `HLA_IMMEDIATE` process-client throw routes into the concrete
2025 `getInteractionClassHandle` wrapper. Its process-client-specific catches
translate `ProcessFederationClientError`, not a `FederateInternalError` thrown
from `receiveInteraction`; the outer `catch (rti1516_2025::Exception const&)`
does catch the latter. That catch attempts `HLAreportException` through the
`noexcept` advisory hook, then attempts the failed service-invocation report
before `throw;`. If the report request completes or is suppressed, the
original callback exception is rethrown. A failed process service-report
request is converted to `RTIinternalError` and can replace it. An arbitrary
non-RTI C++ exception bypasses this wrapper's typed catch and those report
hooks. This is one request-backed API implementation path, not a universal
HLA exception rule or evidence for `HLA_EVOKED`.

Added a focused wrapper/report flowchart beside the process callback sequence
and linked the concrete case from the Exception Reporting guide. Source
references distinguish the ambassador catch, advisory exception-report hook,
process report requests, and failed-report conversion. A local Mermaid render
review exposed a syntax-breaking semicolon in the process sequence and clipped
labels in the combined timeline and wrapper chart. Split the sequence into
separate request-drain and background-dispatch views, replaced punctuation
that Mermaid treated as control syntax, and shortened the clipped branch
label. Mermaid CLI 12.0.0 rendered the two sequences and wrapper flowchart; all
three were visually reviewed locally. Existing public process tests still do
not inject a throwing callback or the combined report-failure schedule. No
tests or builds were run. No Requirements Lab, CI, or 2010 behavior work.

## 2025 DataElement encoding coverage and process callback render review — 2026-10-14

Audited the existing 2025 DataElement guide against the pinned `DataElement`
and `VariableLengthData` declarations, representative integer/boolean/string
codecs, the fixed-record and variable-array implementations, logical-time
factory wrappers, and focused byte-vector/composite tests. The guide already
states that there is no public global endian option: the application selects a
BE/LE wrapper when the type offers one; fixed-format elements determine their
own representation; single-octet elements have no multi-octet order; composite
children own their value encoding and alignment; and the logical-time factory
supplies the contained representation while the wrapper's one-octet boundary
is only alignment metadata. Public value octets and private process-frame
metadata/counts are distinguished. The 2010 companion remains separate. No
encoding-guide edit or additional chart was justified by this review.

The global source-fence count is now 297 diagrams across 34 of 43 design
Markdown files (167 flowcharts, 107 sequences, and 23 state diagrams). This is
not a whole-tree render. No C++ builds, tests, CI, or Requirements Lab work
were performed.

Next handoff: compare the pinned 2025 public service API surface with the
existing reader-guide map. Look for missing HLA decision, state, eligibility,
or observation boundaries—not method-name coverage—and identify only a real
reader gap worth explaining. Keep the 2010 implementation and guide separate;
do not use source as normative authority. No builds, test reruns, CI, or
Requirements Lab changes.

## 2025 API service-family coverage and Non-Regulated-Grant reader path — 2026-10-14

Compared the pinned IEEE 1516.1-2025 `RTIambassador` group headings with the
guide index: Federation Management, Declaration Management, Object Management,
Ownership Management, Time Management, Data Distribution Management, RTI
Support, and API-specific services all have reader paths. The bounded index
check found 28 linked guide targets and no missing target files. This is
family-level wayfinding, not a method-by-method conformance inventory.

Checked the RTI Support Services subfamilies against their existing guides:
name/handle and normalization queries, update-rate values, order and
transportation types, available dimensions/region ranges, callback controls,
support switches, exception/service reporting, logical-time factory access,
and handle decoding each have an existing explanatory path. `getNonRegulatedGrantSwitch()`
was a plausible cross-family gap, but the cross-cutting guide already identifies
it as a getter-only static federation policy and the time guide already explains
its effect when GALT is undefined. Added a direct link in the time glossary to
the pinned API declaration and cross-cutting switch-scope map. No new diagram
was warranted; the 2010 path remains separate and no normative claim was added.

The 28 linked 2025 service-family reader-map targets and their heading
fragments were validated with no broken references.

## 2010 guide navigation and evidence-boundary review — 2026-10-14

Checked relative Markdown links in the separate 2010 reference-RTI and
DataElement guides. All 147 references across 100 unique relative targets
resolve; every cited source line is within its target file. The source/test
boundary notes remain explicit: the reference-runtime synchronization barrier
has no focused 2010 test, the binding smoke covers a successful `HLA_IMMEDIATE`
path rather than an `HLA_EVOKED` path, and time-value coverage does not establish
a time-managed execution state machine. The existing diagrams already expose
these limits, so no extra 2010 chart or inferred 2025 parity was warranted.

Next handoff: audit discovery-triggered Auto Provide across the 2025
object-and-interaction information and Auto Provide guides. Compare the
trigger, owner solicitation versus direct initial-value reflection, tag,
value-delivery boundary, process route, and focused evidence. Reuse existing
diagrams; revise or add only for a demonstrated reader gap. Keep 2010 separate;
no builds, tests, CI, or Requirements Lab work.

## 2025 Auto Provide neighboring-path cross-guide audit — 2026-10-14

Compared the object/interaction guide with the dedicated Auto Provide guide,
then checked the cited embedded and process source/test paths. The guides
already distinguish discovery-triggered owner solicitation from stored-value
reflection and federate-issued Request Attribute Value Update; they also
separate the automatic empty tag, request tag, and provider-selected update
tag. The embedded discovery call path performs the discovery callback, handles
separate initial reflections, and then plans Auto Provide; the focused baseline
asserts discovery and provider callbacks separately and checks the automatic
tag is empty. Process explicit requests have a distinct event route, while the
inspected process discovery initial-value branch is limited to RTI-owned MOM
objects; process automatic discovery remains unverified. No chart collapses
these profiles, and no normative ordering or 2010 parity is claimed. The
existing comparison table and diagrams close this bounded reader question, so
no duplicate diagram or guide edit was warranted. Tests were inspected, not
rerun.

Next handoff: audit whether readers need one HLA-first overview of ordinary
object-attribute update eligibility across the existing Ownership,
Object/Interaction, Subscription State, DDM, Update Rate, Order/Transportation,
Time, and Callback guides. Trace sender authority/service acceptance, the
receiver's candidate projection, per-attribute admission, receive-order versus
TSO delivery, and callback observation. Reuse the detailed diagrams and add a
summary only if it reveals a real cross-guide gap. Keep embedded, process, and
2010 evidence distinct; no builds, tests, CI, or Requirements Lab work.

## 2025 object-attribute update eligibility overview audit — 2026-10-14

Compared the proposed cross-guide overview with the current object/interaction
and cross-cutting guides plus the update-rate guide's boundaries. The
cross-cutting guide already has an HLA-first relationship map for optional
provider solicitation versus an independent update, sender authority,
per-receiver knowledge/relevance, order and transportation, TSO eligibility,
the embedded rate example, reflection, and callback observation. It explicitly
marks those connectors as questions rather than a universal execution order,
then links the topic guides that hold their own source and test evidence. The
object/interaction guide separately starts with five service-independent
questions and expands receive-order projection and callback-time boundaries.
The proposed overview therefore already exists; no duplicate diagram or
content was needed. The chart does not assert process/2010 parity or a single
normative pipeline.

Next handoff: audit ordinary versus directed interaction routing across the
object/interaction, directed-interaction, DDM, time, and callback guides. Check
whether readers can follow the distinct sender declarations, target-object
knowledge, receiver selector/subscription rules, region role, recipient set,
and receive-order/TSO observation boundary without conflating a directed target
with a receiver or a region. Compare the focused ordinary and directed cases;
add or revise a summary only for a demonstrated reader gap. Keep embedded,
process, and 2010 evidence distinct; no builds, tests, CI, or Requirements Lab.

## 2025 ordinary-versus-directed interaction routing review — 2026-10-14

Compared the object/interaction routing table and class-ancestry map with the
directed-interaction, DDM, time, and callback guides. Existing material
distinguishes ordinary class subscriptions from directed object-class /
interaction-class publication pairs and per-receiver selectors; it states
that a directed target is context, not a receiver list, and is not a DDM Region.
The ordinary and directed flows also show their different callback payloads and
class/parameter projections, while their time and callback boundaries stay in
the dedicated guides. Focused embedded and process tests remain explicitly
bounded to their selected declaration and callback cases.

The accepted-work edge is already compared in the cross-cutting guide: the
ordinary receive-order route's behavior after producer unpublication is
source-derived and has no focused test; the directed route rechecks its
publication pair and its stale-work case is tested. The directed guide marks
process delivery as a separate materialized-event path without an embedded-
style callback-time registry re-plan, and does not infer post-acceptance
mutation results for process. The existing table, diagrams, evidence maps, and
limits therefore answer the HLA-first comparison; no duplicate summary chart,
normative withdrawal rule, or 2010 claim was added. Tests were inspected, not
rerun.

Next handoff: audit the 2025 process endpoint's ordinary and directed
interaction behavior when the producer withdraws its publication after send
acceptance. Compare send-time event projection, declaration mutation, queued or
pushed event delivery, callback-time revalidation, and focused test coverage.
Clarify only a real process-profile reader gap; keep embedded and 2010 behavior
separate and avoid inferring normative cancellation rules. No builds, tests, CI,
or Requirements Lab work.

## 2025 process interaction publication-withdrawal review — 2026-10-14

Followed the process-profile handoff across ordinary and directed receive-order
interactions. The ordinary guide already records its send-time promoted class
and parameter snapshot, declaration-update path, lack of callback-time
replanning, and absence of a focused post-send mutation test. For directed
interactions, the process send handler creates recipient-specific events with
the selected target/class/parameter projection; the unpublish handler updates
the declaration registry, and the inspected push/queue and callback paths do
not re-plan eligibility for that materialized event. No focused process test
checks producer unpublication between send acceptance and callback delivery,
so the process delivery result remains source-only and unverified. The embedded
accepted-send/unpublish test does suppress the queued callback, but that result
is not generalized across the process boundary. Updated the directed guide to
make this evidence distinction explicit and to cite the relevant construction,
declaration, and push/queue paths. No duplicate chart was needed; no normative
cancellation rule or 2010 behavior was inferred. Sources and tests were
inspected, not rerun.

## 2025 process directed-TSO dispatch revalidation review — 2026-10-14

Traced the time-constrained recipient path from process send acceptance to
TSO dispatch and callback completion. The server retains the directed message
facts and planned recipient entries, but at each recipient's delivery boundary
the dispatcher calls the timestamped directed-recipient resolver again and
constructs the event from the current class/parameter projection. With the
producer still joined, the current publication pair, target record, receiver
knowledge, and selector are live gates; a failed recheck suppresses that
recipient and completes its queue entry without a directed callback. A legal
producer Retract is a distinct path that removes pending TSO entries and
terminalizes their recipient states. The shared registry has a narrower
accepted-message exception for an exact pending recipient after producer
resignation or marked connection loss; it is not ordinary unpublish semantics.

Focused process evidence covers the queued directed TSO callback/grant path
under NMR/NMRA callback gating, but not publication withdrawal, target
departure, or Retract of a constrained queued copy. A separate process
directed-send case does exercise timestamped Retract before and after callback
under both callback models, but its received order is RECEIVE, so it is not
queued behind a recipient's grant. An embedded target-departure case observes
a time-six removal before a queued time-seven directed interaction and then
suppresses the stale target callback; it is explicitly not process evidence.
The directed guide now has a process-specific TSO flowchart and these bounded
evidence distinctions. No builds, tests, CI, Requirements Lab, normative
cancellation rule, or 2010 behavior was added or inferred.

The chart's Mermaid CLI PNG attempt failed before rendering because the
Puppeteer browser process would not launch (reported exit code 2147483651).
No alternate launch route was tried; syntax acceptance and visual review remain
pending.

Next handoff: audit the 2025 process-side producer-resignation and Connection
Lost paths for accepted timestamped directed work. Compare the exact
message/recipient preservation markers with the lifecycle and TSO guides and
identify whether any focused process case exists; keep embedded evidence and
licensed §4.4 wording separate. Document only a demonstrated reader gap, keep
2010 separate, and do not run builds, tests, CI, or Requirements Lab work.

## 2025 process directed-TSO producer departure gates — 2026-10-14

Compared the process directed-TSO delivery resolver with the shared registry's
Resign and Connection Lost markers, the lifecycle/time guides, and focused
tests. The exact departed-source exception requires the stored message ID,
producer, target, and sent class to match; the accepted message must name the
recipient, and that recipient's ledger state must still be pending. Voluntary
Resign sets `producerResigned`; forced Connection Lost instead sets
`deliveryRequiredAfterConnectionLoss` only for a timestamp at or before the
regulating producer's captured, comparable current time. The former is not
cutoff-limited; the latter does not authorize later or uncomparable timestamps.

For that exact exception the registry skips departed-source membership,
publication, and known-object checks, and does not reject an extant target only
because `deleteAccepted` is set. It still requires the target record/type and
the receiver's live membership, target knowledge, selector, and projection.
The connection-loss grant helper separately allows a surviving requester to
reach a pending marked delivery boundary if ordinary GALT became undefined.
The updated flowchart splits the joined-source and departed-source paths and
labels the marker gate. A fresh Mermaid CLI attempt on this revision again
failed before parsing because Puppeteer could not launch (exit code 2147483651);
no alternate launch route was used, so syntax and visual review remain pending.

The focused directed cutoff, late-TAR/automatic-delete, and selector-change or
unsubscribe tests found by this survey explicitly use embedded transport. They
confirm selected outcomes, including continued receiver-selector evaluation,
but do not establish process integration behavior. Process source routes share
the resolver; no focused process test combining a queued directed TSO copy with
producer Resign or Connection Lost was found. The broad process probe's
directed-retraction case retracts its work before Resign and is not such
coverage. The full normative §4.4 text remains unavailable, so the guide states
implementation behavior only. No C++ files, builds, test runs, CI, Requirements
Lab, or 2010 documentation were changed.

Next handoff: do a bounded reader-gap audit of the 2025 Delay Subscription
Evaluation guide. Clarify deferred subscription/relevance evaluation versus
service validity and delivery guarantees; compare receive-order, timestamped,
directed, and process routes against existing focused tests; revise only a
demonstrated gap. Keep the development-profile boundary and 2010 stream
separate, and do not build, rerun tests, start CI, or edit the Requirements
Lab.

## 2026-10-14 — Delay Subscription Evaluation process-route review

The DSE guide already separates sender-side service validity, receiver
eligibility timing, and callback/grant timing. The focused receive-order,
timestamped, and directed cases found in this bounded audit are embedded
2025 tests; they show late-subscription admission and current-state
suppression for their covered paths. The normative §8.1.8 paraphrase remains
explicitly subject to verification against an authorized copy.

The reader gap was the process timing boundary. Process receive-order ordinary
and directed interactions re-resolve deferred candidates while preparing an
outgoing event, which is distinct from the embedded callback-entry check.
Process ordinary and directed TSO delivery use separate projection checks at
queued delivery. These are source observations only: no focused process DSE
case was found, so the source does not establish test-demonstrated process
behavior or normative equivalence. The guide now says which boundary each
process citation supports. Mermaid source was not changed. No build, test
rerun, CI, Requirements Lab, or 2010 documentation work was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Attribute Ownership
guide. Focus on the reader-visible boundaries between negotiated transfer,
ownership acquisition/divestiture, and update authority; compare the guide's
existing focused tests and source evidence, and revise only a demonstrated
gap. Keep the development-profile and 2010/2025 boundaries explicit; do not
build, rerun tests, start CI, or edit the Requirements Lab.

## 2026-10-14 — Attribute ownership and unpublication boundary

The bounded review found one reader-facing gap in the otherwise explicit
distinction between publication and per-instance ownership. In the current
embedded 2025 route, unpublishing an owned attribute set synchronously removes
the caller's owner entries and associated update metadata. A focused update
case accepts a value update before whole-class unpublication and observes
`AttributeNotOwned` on the next update. The guide now connects that concrete
transition to the conceptual distinction while labeling it as bounded source
and test evidence, not an exhaustive normative §5.3 interpretation or a 2010
claim. No Mermaid source, C++, build, test rerun, CI, or Requirements Lab
content changed.

Next handoff: do a bounded reader-gap audit of the 2025 Request Attribute
Value Update guide. Focus on whether an explicit value request changes
ordinary receiver eligibility for a later provider update; compare the
guide's source and focused tests with authorized §6.21 text. If the clause
text is unavailable, leave the portable HLA outcome open and document only
demonstrated implementation behavior. Preserve 2010/2025 and embedded/process
boundaries; do not build, rerun tests, start CI, or edit the Requirements Lab.

## 2026-10-14 — Request Attribute Value Update §6.21 entitlement review

The guide already separates provider solicitation from a later value update
and states that the update uses ordinary receiver planning. The bounded source
and test review supports that implementation description, but does not settle
the portable HLA entitlement question. The embedded role-overlap case
subscribes the requester through the public service before requesting and
later receiving the provider's separate reflection. The process object-request
case seeds subscription state directly in its fixture before the request and
also observes a later reflection; it does not test the public process
subscription route. The process class-request case does not test a later value
reflection. None isolates an otherwise-ineligible, unsubscribed requester.

The IEEE 1516.1-2025 public listing identifies Purchase and Access via
Subscription, while the checkout contains the API signature and exception
list but no full clause copy. Therefore §6.21 remains open; no normative
answer, process parity, or 2010 inference was added. The existing guide was
already appropriately cautious, so no guide or diagram edit was warranted.
No build, test rerun, CI, Requirements Lab, or C++ work was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Auto Provide guide.
Trace discovery-triggered provider work separately from explicit Request
Attribute Value Update, and compare the embedded discovery path with process
service/source and focused-test coverage. Keep process support unverified
unless a direct process call/test is found; preserve the separate 2010/2025
boundary. Do not build, rerun tests, start CI, or edit the Requirements Lab.

### 2025 Auto Provide: process discovery boundary — 2026-10-14

Traced the 2025 process discovery service through its callback bridge. The
service commits known-object state, can plan initial relevance advisories for
ordinary discovery, and separately plans initial values for RTI-owned MOM
objects; the callback bridge delivers `discoverObjectInstance`. A source
call-site scan found `planAutoProvideForDiscovery` only in the embedded
callback path. The process HLAsetSwitches handler and focused process test
establish switch mutation/state only; the test does not arrange discovery or
observe a provider solicitation. No focused process Auto Provide test was
found. Updated the guide to make these route and evidence limits explicit,
without making a normative claim or inferring 2010 parity. No diagram source
changed, so prior chart-render evidence is unaffected. No C++, build, test,
CI, or Requirements Lab work was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Federation
Synchronization guide, focusing on the state and observability path from
synchronization-point registration through announcement, participant
achievement, and completion/failure callbacks. Keep the 2010 stream separate,
distinguish 2025 embedded/process evidence, and do not build, rerun tests, start
CI, or edit the Requirements Lab.

### 2025 Federation Synchronization: process result evidence — 2026-10-14

Reviewed the existing guide's HLA-level sequence, embedded participant-set
state, late-join and partial-result traces, false-achievement semantics,
departure transition, and save/restore evidence. The concepts and diagrams
already make registration outcome, announcement, participant report, and
federation completion distinct; no extra chart was justified.

The paired configured-process public cases use an explicit receiver-only set.
They observe the registrant's successful registration result, duplicate-label
`synchronizationPointRegistrationFailed`, receiver announcement/tag, a
one-member `failedToSyncSet` after the receiver reports `false`, and
`SynchronizationPointLabelNotAnnounced` when the out-of-set registrant tries
to achieve. One uses HLA_EVOKED with polling and one HLA_IMMEDIATE with pushed
events. Since the registrant is excluded from the receiver set, neither case
establishes same-recipient registration/announcement ordering; they do not test
omitted-set expansion or prove profile parity. Clarified the evidence rows and
kept the standard boundary explicit. No diagram source was changed in this
increment. No build, test rerun, CI, Requirements Lab, or 2010 work was
performed.

Next handoff: do a bounded reader-gap audit of the 2025 Delay Subscription
Evaluation guide. Clarify deferred subscription/relevance evaluation versus
service validity and delivery guarantees; compare receive-order, timestamped,
directed, and process routes against existing focused evidence, revising
diagrams only for a demonstrated reader gap. Keep 2010 and 2025 separate,
distinguish embedded/process evidence, and do not build, rerun tests, start CI,
or edit the Requirements Lab.

### 2025 Delay Subscription Evaluation: process attribute flow — 2026-10-14

The bounded DSE review found the existing HLA-level timing model and embedded
callback/grant diagrams sufficient, but the process attribute route deserved a
separate source map. For a route-only process receive-order candidate,
`process_federation_service_attribute_value_updates.cpp:201` re-resolves the
recipient before event construction while passing sent attribute handles only;
the retained source-region handles and snapshots are not passed to that lookup.
The queued TSO attribute route instead re-resolves at recipient dispatch with
the sent-region handles and snapshots
(`process_federation_service_tso_delivery.cpp:64`). The callback bridge
reflects the projected event (`process_federation_callback_bridge.cpp:361`),
without another DSE projection step. This is a source-observed difference and
an evidence limit, not a normative conclusion or confirmed defect. No focused
process-endpoint DSE test was found. Added a process attribute flowchart and
implementation links. Mermaid CLI's browser could not launch (exit code
2147483651), so the new chart has no syntax/render or visual verdict. No C++,
build, tests, CI, Requirements Lab, or 2010 work was performed.

Next handoff: do a bounded reader-gap audit of the 2025 Object-Attribute
Subscription State guide. Trace inherited and regional subscription
declarations through their effects on active relevance, discovery/known-object
state, advisories, and callbacks. Compare existing diagrams with focused
source/test evidence, keeping declaration intent, update relevance, and object
knowledge distinct. Keep the 2010 and 2025 streams separate, distinguish
embedded/process evidence, and do not build, rerun tests, start CI, or edit the
Requirements Lab.

