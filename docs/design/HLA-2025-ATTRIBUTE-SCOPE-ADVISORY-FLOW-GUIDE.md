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

The official [IEEE 1516.1-2025 edition page](https://standards.ieee.org/ieee/1516.1/6688/)
is the normative starting point. In the pinned API, the
[FederateAmbassador declarations](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L333)
identify attributesInScope as callback 6.19 and attributesOutOfScope as 6.20.
Umbra source comments cite clause 10.1.3 for the special case where a
federate removes its own final applicable subscription. Treat that as a
source-derived rule to verify against the standard, not as a substitute for
the official text.

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
direction. The process-service path performs a registry eligibility check
before work crosses the process boundary; when receive-order events are
queued, the receive path checks eligibility again before returning the event.
A later commit, unsubscribe, switch change, resignation, or object deletion
can therefore make previously planned work ineligible.

That means “the RTI planned an In callback” is not the same claim as “the
application will receive that callback.” The embedded stale-In test commits a
receiver-region move before callback entry and verifies that the obsolete In
report is suppressed while the valid Out report remains. The process test
separately checks switch, subscription, association, and public endpoint
behavior in both callback models.

## Decision flow for one planned change

~~~mermaid
flowchart TD
  A[Mutation committed or candidate state evaluated] --> B{Old scope equals new scope?}
  B -->|Yes| C[Keep state; no advisory]
  B -->|No| D{Is this the receiver's own final unsubscribe causing Out?}
  D -->|Yes| E[Implicit loss of scope; suppress Out advisory]
  D -->|No| F{Receiver's Attribute Scope Advisory Switch enabled?}
  F -->|No| G[Suppress callback; no disabled-switch backlog]
  F -->|Yes| H[Group attribute with same receiver, object, and direction]
  H --> I[Queue or route callback work]
  I --> J{At delivery, does current scope match planned direction?}
  J -->|No| K[Drop stale attribute work]
  J -->|Yes, In| L[attributesInScope(object, attributes)]
  J -->|Yes, Out| M[attributesOutOfScope(object, attributes)]
~~~

The self-unsubscribe branch applies specifically to the subscription-mutation
planner. Other mutations should be evaluated by their own transition path;
for example, removing the owner's overlapping source association can produce
an Out advisory for a still-subscribed receiver.

## Implementation and test evidence

| Concern | Current 2025 implementation | Focused test evidence |
| --- | --- | --- |
| Effective in-scope predicate: known object, class ancestry, ordinary and regional declarations, default and explicit region realizations | [Registry scope predicate](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L16) | [Embedded regional scenario](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L97) |
| Pre/post association comparison and recipient/object/direction grouping | [Association planner](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L517); [region-association service](../../cpp/src/internal/federation/federation_registry_update_region_associations.cpp#L190) | [Embedded region movement, grouped attributes, and stale evoked callback](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L201) |
| Pre/post receiver-region commit comparison | [Committed region planner](../../cpp/src/internal/federation/federation_registry_regions.cpp#L255) | [Embedded committed-region moves](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L209) |
| Subscription transition and the final-self-unsubscribe Out exception | [Subscription planner](../../cpp/src/internal/federation/federation_registry_attribute_relevance.cpp#L577) | [Embedded exception and switch scenario](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L238); [process endpoint exception](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L457) |
| Embedded switch gate, callback-time filtering, and callback dispatch | [Scope callback route](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L1190) | [Embedded test runs HLA_EVOKED and HLA_IMMEDIATE](../../cpp/tests/attribute_scope_advisory_catch2.cpp#L334) |
| Process boundary queue and receive-time stale-work filtering | [Process event planning](../../cpp/src/internal/federation/process_federation_service_notifications.cpp#L147); [receive path](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L125) | [Public process endpoint test, including both callback models](../../cpp/tests/ieee1516_2025_connection_regional_unsubscribe_catch2.cpp#L457) |
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
- These diagrams have passed static source/link review only. The current
  workstation does not have a local Mermaid renderer, so verify visual
  rendering in GitHub's Markdown preview (or an approved local renderer)
  before treating the guide as visually reviewed.
