# IEEE 1516.1-2025 Attribute Scope Advisory: Flow Guide

This guide follows one narrow but easy-to-misread path: how a receiving
federate's view of a known object's attribute moves **into** or **out of
scope**, and when Umbra reports that transition through
**attributesInScope** or **attributesOutOfScope**.

It is written for readers who know the HLA services and want to understand the
runtime nuance: scope is per receiving federate, object instance, and
attribute; regions can change it without a new subscription; an advisory is
not the data update; and a callback queued for an old state can become stale.

**Edition:** IEEE 1516.1-2025 only. This guide describes current Umbra paths
and selected tests. It does not claim full standard conformance or equivalence
with the separate 2010 reference RTI.

**Implementation boundary:** the surveyed 2025 embedded registry/service path
and the selected public process-endpoint path. The guide keeps those evidence
sets distinct and does not generalize beyond the cited code and tests.

The official [IEEE 1516.1-2025 edition page](https://standards.ieee.org/ieee/1516.1/6688/)
identifies the edition. The checkout's pinned 2025 import is the unmodified C++
API header tree only ([README](../../third_party/ieee1516.1-2025/README.md)); it
does not include normative clause text. In the pinned API, the
[FederateAmbassador declarations](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L333)
identify attributesInScope as callback 6.19 and attributesOutOfScope as 6.20.
Umbra source comments cite clause 10.1.3 for the special case where a
federate removes its own final applicable subscription. Treat that as a
source-derived rule that has not been independently verified against the
official clause text in this checkout.

## First separate four signals

| Signal | Whose state or callback? | What it tells you | What it does not tell you |
| --- | --- | --- | --- |
| Attribute Scope advisory | Receiving federate; object instance plus an attribute set | A known attribute crossed that receiver's current scope boundary. | It does not mean a value was sent, reflected, or changed. |
| Turn Updates On/Off | Object-attribute owner; object instance plus attributes | One or more receivers make the owner's attribute relevant or cease to do so. | It is not an Attributes In/Out Of Scope callback. |
| Declaration relevance advisory | Publisher; object or interaction class | Class-level subscription interest changed. | It does not establish object existence or regional overlap. |
| Reflect Attribute Values | Receiving federate | A publisher's value update passed the independent delivery rules. | It is not a scope transition notification. |

For the owner/receiver distinction, see the
[declaration and attribute relevance guide](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md).
For region lifecycle, overlap geometry, discovery, and routing context, see
the [DDM and regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md).

## Scope is a receiver-local predicate

For the current implementation, reason about a tuple:

**(receiving federate, known object instance, object attribute)**

The registry's scope predicate rejects the producer itself, a deleted or
unknown instance, a receiver that is no longer a federation member, and an
attribute outside the known class hierarchy. It then checks the receiver's
applicable object-class attribute declarations. Ordinary declarations and
regional declarations have different region rules:

- An ordinary subscription is in scope when the source has no explicit update
  region, or when an explicit source region overlaps the default receiver
  realization.
- A regional subscription checks its committed receiver region against the
  source's associated update region. If no explicit source association exists,
  it checks the receiver region against the default source realization.
- Multiple applicable declarations/region pairs are alternatives: one
  matching path is sufficient to keep the attribute in scope.

These bullets summarize the current registry predicate, not a complete
restatement of every normative DDM rule. See
[the predicate implementation](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L16)
and keep the [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
as the home for general overlap and delivery behavior.

## State transitions: commit first, then consider an advisory

The state machine is independent for each receiver/object/attribute tuple.
A region or declaration mutation compares the old and candidate scope state.
The callback is considered only if that state changes and the receiver's
Attribute Scope Advisory Switch permits it.

~~~mermaid
stateDiagram-v2
  [*] --> OutOfScope
  OutOfScope --> InScope: applicable declaration and region path now matches
  InScope --> OutOfScope: no applicable declaration/region path remains
  InScope --> InScope: mutation leaves at least one matching path
  OutOfScope --> OutOfScope: mutation still has no matching path
  InScope --> OutOfScope: own final unsubscribe / no Out advisory
~~~

The last edge is deliberately different from an owner-side or region-side
loss. In the embedded planner, subscription changes enqueue an In advisory
when a new declaration makes the receiver in scope, but suppress the
receiver's Out advisory when its own unsubscribe removes the final applicable
declaration. The source labels this behavior with clause 10.1.3. Owner-side
association removal and committed region movement can still produce an Out
advisory. Do not generalize the unsubscribe exception to all Out transitions.

The registry groups changed attributes by receiving member, object instance,
and direction. Thus two attributes entering scope for the same receiver and
object can be reported together in one callback, rather than as a callback
per attribute. The grouping is visible in the
[association planner](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L517)
and [subscription planner](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L577).

## Mutation-to-callback path

The mutation can originate from either side: the receiver commits new region
bounds or changes a subscription, or the owner changes update-region
associations. The mutation changes the registry's effective state before a
callback is delivered.

~~~mermaid
sequenceDiagram
  autonumber
  actor R as Receiving federate
  actor P as Object owner
  participant RTI as 2025 RTI / federation registry
  participant Q as Receiver callback route

  alt Receiver commits subscription region or subscription change
    R->>RTI: Commit region modifications or change subscription
  else Owner changes source association
    P->>RTI: Associate or unassociate update regions
  end
  RTI->>RTI: Compare old and new scope per known object attribute
  RTI->>RTI: Group changed attributes by receiver, object, and direction
  RTI->>RTI: Apply receiver Attribute Scope Advisory Switch
  alt A reportable transition remains
    RTI->>Q: Queue In Scope or Out Of Scope work
    Q->>RTI: Recheck current receiver/object/attribute scope at delivery
    alt Current state still matches queued direction
      RTI-->>R: attributesInScope or attributesOutOfScope
    else State changed, switch disabled, member left, or object deleted
      RTI-->>Q: Suppress stale work
    end
  else No transition, switch off, or self-final-unsubscribe exception
    RTI-->>R: No scope callback
  end
~~~

This is a lifecycle view, not a callback-order guarantee relative to other
queued services. Under HLA_EVOKED, application callback delivery is driven by
the callback-evocation service; under HLA_IMMEDIATE, it is delivered through
the immediate callback path. The focused tests exercise both modes, but do
not establish every possible ordering with unrelated callback families.

### Why the delivery-time recheck matters

Planning records an expected direction. Before an embedded callback is
entered, Umbra recalculates which scheduled attributes still match that
direction. The process service checks registry eligibility before routing a
scope event, but its next checkpoint depends on the configured event path:
server-queued events are checked again when a later receive request dequeues
them; pushed events are sent after the first check and then enter the client's
callback pipeline without another registry scope check shown in the reviewed
path. A later commit, unsubscribe, switch change, resignation, or object
deletion can make planned work ineligible, but the last RTI-side check is not
always at application callback entry.

That means “the RTI planned an In callback” is not the same claim as “the
application will receive that callback.” In the embedded **HLA_EVOKED** test
branch, the receiver-region move occurs before callback dispatch, so the
queued In is stale and is suppressed while the current Out report remains. In
the **HLA_IMMEDIATE** branch, the In callback is entered before that later
region move; it is valid at callback entry, and the subsequent move produces
the Out report. The process endpoint test separately exercises the enabled
scope-switch setup, regional unsubscribe, owner-side association changes, and
public In/Out callbacks under both callback models, but it does not stage a
queued-stale race. In that fixture, `HLA_EVOKED` uses the server's polling
queue and `HLA_IMMEDIATE` selects pushed events; this is how that test
configures the service, not a general equivalence between callback model and
event transport.

### Process event transport has two stale-work checkpoints

Both process paths first check whether the planned scope direction still
matches current registry state. The difference is what happens after that
admission check: a server-queued event gets a second check at dequeue, while a
pushed event is handed across the transport and on to client callback dispatch
without another registry check visible in the path.

~~~mermaid
flowchart TD
  Plan["Scope transition plan"] --> Filter["Filter planned attributes by current scope and direction"]
  Filter --> Eligible{"Any eligible attributes remain?"}
  Eligible -->|No| Drop["Suppress before routing"]
  Eligible -->|Yes: subset| Mode{"Configured process event path"}
  Mode -->|Pushed event| Push["Send eligible subset after admission check"]
  Push --> Client["Client callback pipeline"]
  Mode -->|Server queue| Queue["Store eligible subset in per-session server queue"]
  Queue --> Receive["Later process receive_interaction poll"]
  Receive --> Refilter["Filter queued attributes against current scope again"]
  Refilter --> Any{"Any eligible attributes remain?"}
  Any -->|No| Suppress["Consume stale event; continue scanning"]
  Any -->|Yes: subset| Return["Return current eligible subset"]
  Return --> Client
  Client --> Callback["Submit or dispatch scope callback"]
~~~

Here, `receive_interaction` is Umbra's private process-protocol poll name, not
the HLA `receiveInteraction` callback or an HLA service. Its result is a
multiplexed event envelope with a dedicated scope-change slot
([client poll](../../cpp/src/internal/federation/process_federation_client_value_services.cpp#L215),
[protocol result](../../cpp/src/internal/federation/process_federation_service_protocol.hpp#L1623)).

The second registry check is at server dequeue, not necessarily at the later
public callback entry. A change after that check—or after the pushed path's
admission check—before application observation has no focused race test here.
Do not claim that either source path proves an HLA-required stale-callback
cutoff; this is a bounded implementation observation and an open test gap.

## Decision flow for one planned change

~~~mermaid
flowchart TD
  A[Mutation committed or candidate state evaluated] --> B{Old scope equals new scope?}
  B -->|Yes| C[Keep state; no advisory]
  B -->|No| D{Is this the receiver's own final unsubscribe causing Out?}
  D -->|Yes| E[Implicit loss of scope; suppress Out advisory]
  D -->|No| F{Receiver's Attribute Scope Advisory Switch enabled?}
  F -->|No| G[Suppress this plan; enqueue no new callback work]
  F -->|Yes| H[Group attribute with same receiver, object, and direction]
  H --> I[Queue or route callback work]
  I --> J{At delivery, does current scope match planned direction?}
  J -->|No| K[Drop stale attribute work]
  J -->|Yes, In| L["attributesInScope(object, attributes)"]
  J -->|Yes, Out| M["attributesOutOfScope(object, attributes)"]
~~~

The self-unsubscribe branch applies specifically to the subscription-mutation
planner. Other mutations should be evaluated by their own transition path;
for example, removing the owner's overlapping source association can produce
an Out advisory for a still-subscribed receiver.

The source has switch gates at admission: the embedded route checks the
recipient's switch before scheduling, and the shared registry eligibility
predicate returns no attributes when the switch is off; the process service
uses that predicate before routing and again when pull delivery dequeues an
event ([embedded gate](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L1208),
[switch-aware predicate](../../cpp/src/internal/federation/federation_registry_object_instance_removal.cpp#L12),
[process admission](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L238),
[pull recheck](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L161)).
That supports source-level suppression for work evaluated while disabled; it
does not by itself establish what happens to an already-pushed event if the
switch changes afterward.

### Switching off is not a common queue flush

The switch-setting path changes the registry value; it does not clear the
process session's scope-event queue or the client's callback dispatcher. The
source behavior depends on how far the event has progressed:

| Work position when the switch is disabled | Source behavior | Evidence boundary |
| --- | --- | --- |
| Embedded callback closure is queued but has not entered the callback | The closure rechecks the switch-dependent registry predicate at callback entry. If the switch is off at that check, the closure returns without invoking the scope callback ([route](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L1207), [predicate](../../cpp/src/internal/federation/federation_registry_object_instance_removal.cpp#L12)). | Source-derived; no switch-only pending-callback test. |
| Process pull event remains in the server session queue | The setter changes registry state but does not purge the queue ([setter](../../cpp/src/internal/federation/process_federation_service_ddm_switches.cpp#L39), [session queue](../../cpp/src/internal/federation/process_federation_service.hpp#L149)). A later receive poll removes the event and rechecks eligibility ([receive recheck](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L157)): while still disabled it is suppressed; if re-enabled before that poll, it may be reconsidered if the current scope still matches. | Source-derived; neither switch/poll interleaving is tested. |
| Process pushed frame has been sent or its callback task submitted | The server checks eligibility before sending ([push/admission path](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L207)), but the client callback bridge has no later scope-switch check ([bridge submit](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1844), [delivery](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L684)), and the switch-setting path does not purge client callback tasks. | No post-send cancellation or recheck is visible; callback delivery across a concurrent switch change is untested. |

These are three different implementation checkpoints, not one HLA-wide
“disable cancels pending advisories” rule. “Enqueue no new callback work”
describes only the plan being evaluated while the switch is off; it does not
mean work admitted earlier is removed from every queue.

No focused test isolates switch-off as the only suppressor. The embedded test
comment calls its segment switch-gate suppression, but the action is to disable
the switch and remove the receiver's final regional subscription
([test segment](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L302));
the subscription planner's self-final-unsubscribe exception independently
suppresses the Out advisory ([planner](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L613)).
Re-enabling and subscribing later checks a fresh In callback, not whether an
otherwise-reportable Out was suppressed solely by the switch. The process
endpoint scenario explicitly enables the switch before exercising ordinary
transitions ([setup](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L893)).
Other C++ test uses of the setter cover service/MOM reporting, unrelated
behavior, or rejected save/restore-interlocked calls, not switch-only scope
suppression. No focused test toggles the switch around an event already waiting
in the process pull queue, an already-sent pushed frame, or a client callback
task.
So that test remains open.

## Implementation and test evidence

| Concern | Current 2025 implementation | Focused test evidence |
| --- | --- | --- |
| Effective in-scope predicate: known object, class ancestry, ordinary and regional declarations, default and explicit region realizations | [Registry scope predicate](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L16) | [Embedded regional scenario](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L97) |
| Pre/post association comparison and recipient/object/direction grouping | [Association planner](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L517); [region-association service](../../cpp/src/internal/federation/federation_registry_update_region_associations.cpp#L190) | [Embedded region movement, grouped attributes, and stale evoked callback](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L201) |
| Pre/post receiver-region commit comparison | [Committed region planner](../../cpp/src/internal/federation/federation_registry_regions.cpp#L255) | [Embedded committed-region moves](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L209) |
| Subscription transition and the final-self-unsubscribe Out exception | [Subscription planner](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L577) | [Embedded exception and switch scenario](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L238); [process endpoint exception](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L457) |
| Embedded switch gate, callback-time filtering, and callback dispatch | [Scope callback route](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L1190); [switch-aware eligibility predicate](../../cpp/src/internal/federation/federation_registry_object_instance_removal.cpp#L12) | [Stale-In transition and callback-model cases](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L201); [confounded switch-off segment](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L302) |
| Process event transport checkpoints | [Admission check and push/queue split](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L207); [server dequeue-time recheck](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L154); [client pushed-event dispatch](../../cpp/src/internal/federation/process_federation_client_callback_dispatch.cpp#L326); [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1844) | [Public process endpoint transition test; switch enabled, no stale-event race](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L457); [switch setup](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L893); [its callback-model-to-event-mode setting](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L595) |
| Public callback names and API clauses | [Pinned 2025 FederateAmbassador](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L333) | Both tests record callback object and attribute count; the embedded test also checks exact attribute-set equality. |

The embedded test is a focused regional transition scenario: it checks
association changes, committed receiver-region movement, grouped attributes,
stale In suppression, the final-self-unsubscribe exception, switch behavior,
and both callback models. The process-endpoint test checks public remote
service routing, scope-switch setup, a regional unsubscribe, subsequent
owner-side Out/In transitions, and both callback models. Neither test exhausts every class
hierarchy, overlapping-region combination, concurrent mutation, resignation
race, transport failure, or interleaving with time-managed callbacks.

## Boundaries and next questions

- Scope advisories describe the receiver's scope state, not the owner's
  attribute relevance state. The two may change from the same subscription or
  region operation but have different recipients and rules.
- Scope-in does not itself cause an attribute value update. Publication,
  ownership, actual update calls, DDM routing, transportation/order, and time
  management still govern data delivery.
- Discovery is a separate earlier/later boundary. This guide starts from a
  known object; see the DDM and object-information guides for discover/remove
  lifecycle details.
- The cited source and tests are all the IEEE 1516.1-2025 stream. The
  [separate 2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md)
  describes a different profile; this guide makes no cross-edition claim.
- GitHub's Markdown preview at commit
  `5392bde4f41e7da33a03835ff16a78c663d62022` rendered all three diagrams.
  The state chart and mutation sequence were individually visually reviewed
  on 2026-10-09. The repaired decision flow was screenshot-inspected on
  2026-10-11: it is a tall, scrollable chart, but its branches and callback
  outcomes remain readable. This closes the earlier parse/render check for
  that chart; the preview is evidence for this revision, not a guarantee about
  later edits.
