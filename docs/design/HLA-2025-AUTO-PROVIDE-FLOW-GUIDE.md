# IEEE 1516.1-2025 Auto Provide: Flow Guide

Auto Provide is the RTI-driven request for current attribute values that can
follow a federate learning about an object. The callback asks a provider to
send values; it is not the values, a subscription, a scope advisory, or a
promise that a provider will respond.

This guide focuses on the discovery-triggered path in Umbra: federation-wide
switch state, per-object/per-attribute scope, grouping by current owner,
callback-time fences, and the separate provider response. Explicit Request
Attribute Value Update remains a different service path.

**Edition:** IEEE 1516.1-2025 implementation profile only. Nothing here
claims equivalence with Umbra's separate 2010 reference RTI.

The pinned 2025 API identifies the provider callback as
[Provide Attribute Value Update, clause 6.22](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L348)
and the public switch query as
[Get Auto Provide Switch, clause 10.52](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L2213).
Those interface declarations name the surfaces; the lifecycle and gating
below describe current Umbra behavior from its source and selected tests.
Use the official [IEEE 1516.1-2025 edition page](https://standards.ieee.org/ieee/1516.1/6688/)
as the standards starting point and consult the standard for normative rules.

## Do not conflate these four paths

| Path | Trigger and recipient | Payload or result | Key distinction |
| --- | --- | --- | --- |
| Auto Provide | A receiver newly discovers an object; each current owner of relevant in-scope attributes may receive a provider callback. | Object, that provider's attribute subset, and an empty tag in the current automatic path. | RTI-triggered solicitation; no application request tag initiated it. |
| Explicit Request Attribute Value Update | A federate invokes the service for an object or class; eligible providers receive the request callback. | The caller-supplied request tag is carried to providers. | A separate service; Auto Provide's switch and callback-time fence must not be applied to it by assumption. |
| Initial attribute reflection | A receiver discovers an object and eligible values already exist in the current implementation. | Values are reflected directly under the initial-reflection path. | This is data delivery, not a provider solicitation. |
| Attributes In/Out Of Scope | A known receiver/object/attribute changes scope. | Scope callback with an attribute set. | It reports scope, not a request for current values. See the [scope-advisory guide](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md). |

The broader [object and interaction information-flow guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md)
shows explicit request/response flow. The [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
owns region geometry and general routing, while the
[time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md) owns timestamped
delivery and grants.

## Switch state is federation-wide

The current embedded registry seeds Auto Provide from the federation's
composed FDD switch value at creation. A member can read the value with
Get Auto Provide Switch. The current MOM HLAsetSwitches path updates the
federation-wide HLAautoProvide value, so all joined members observe the same
state. This is not the per-receiver Attribute Scope Advisory Switch.

~~~mermaid
stateDiagram-v2
  [*] --> Enabled: creation FDD seeds HLAautoProvide true
  [*] --> Disabled: creation FDD seeds HLAautoProvide false
  Enabled --> Disabled: HLAsetSwitches(HLAautoProvide=false)
  Disabled --> Enabled: HLAsetSwitches(HLAautoProvide=true)
  Enabled --> Enabled: discovery may plan provider solicitations
  Disabled --> Disabled: discovery proceeds without provider solicitation
~~~

Changing the switch does not itself rediscover existing objects. In the
current implementation, the automatic planner is entered after discovery;
turning the switch back on permits later qualifying discovery work but does
not reconstruct a solicitation already suppressed while disabled.

## Discovery-triggered planning and provider fan-out

The embedded path performs ordinary object discovery first. After the
Discover Object Instance callback has run (including any reentrant service
calls made by that callback), Umbra asks the registry to plan Auto Provide.
If enabled, the planner checks that the object is still known and not deleted,
then examines its current per-attribute owners.

An attribute enters the plan only when its owner is a different current
federate, the attribute belongs to the receiver's known class hierarchy, and
the receiver's current subscription/region state makes that attribute in
scope. Multiple qualifying attributes owned by the same provider are grouped
for that provider and object. Different owners get separate provider
callbacks. The Attribute Scope Advisory Switch does not control this planner:
Auto Provide follows actual scope, not whether the receiver requested
Attributes In/Out Of Scope notifications.

~~~mermaid
sequenceDiagram
  autonumber
  actor R as Receiver
  participant RTI as 2025 Umbra RTI
  actor P1 as Provider A
  actor P2 as Provider B

  Note over R,RTI: Registration or subscription/DDM change<br/>makes a known object discoverable
  RTI-->>R: discoverObjectInstance(object, class, name, producer)
  Note over R,RTI: After discovery callback returns, its reentrant changes are visible
  RTI->>RTI: Check federation Auto Provide switch and current object/scope state
  RTI->>RTI: Group each provider's owned in-scope attributes
  opt Provider A has a nonempty eligible attribute set
    RTI-->>P1: provideAttributeValueUpdate(object, attributesA, empty tag)
    P1->>P1: Application decides whether and when to provide values
    opt Provider A sends an update
      P1->>RTI: updateAttributeValues(object, values, provider-selected tag)
      RTI-->>R: reflect only if independent update-delivery rules pass
    end
  end
  opt Provider B has a nonempty eligible attribute set
    RTI-->>P2: provideAttributeValueUpdate(object, attributesB, empty tag)
  end
~~~

The arrows show the causal path, not a total ordering guarantee across
different federates' callback queues. The focused tests record discovery and
provider callback observations under controlled callback pumping; do not
generalize that observation into a global cross-federate order.

## Plan once, revalidate at provider callback entry

The queued provider callback is not irrevocable. Immediately before calling
provider code, the embedded adapter asks the registry to project the eligible
attributes again. For Auto Provide, that check requires the federation switch
still to be enabled and the receiver's current scope still to include at least
one requested attribute. If the object, owner, member, switch, or scope is no
longer eligible, the automatic callback is suppressed. Explicit pending
requests use their own pending-request path and should not be silently folded
into this rule.

~~~mermaid
flowchart TD
  A[Object is newly discovered by receiver] --> B{Auto Provide switch enabled?}
  B -->|No| C[Keep normal discovery; plan no provider callback]
  B -->|Yes| D[Collect current owners of receiver's in-scope attributes]
  D --> E{Any eligible owner/attribute groups?}
  E -->|No| F[No Provide Attribute Value Update callback]
  E -->|Yes| G[Queue one grouped solicitation per provider and object]
  G --> H{At provider callback entry: switch still on?}
  H -->|No| I[Consume stale Auto Provide work]
  H -->|Yes| J{Object, provider membership, and receiver scope still eligible?}
  J -->|No| I
  J -->|Yes| K[Invoke provider callback with the current eligible subset and empty tag]
  K --> L{Does provider call Update Attribute Values?}
  L -->|No| M[No value response is implied]
  L -->|Yes| N[Run independent ownership, DDM, order, and time delivery checks]
~~~

Two timing edges are worth keeping separate:

1. A receiver can change its region or subscription from inside its discovery
   callback. The later Auto Provide planner sees that new state and can avoid
   planning a solicitation that was already out of scope by callback return.
2. A solicitation can already be queued for a provider when the switch or
   scope changes. The provider callback-entry recheck can remove it before
   provider code runs. A different provider's response that was admitted
   before the switch changed is not retroactively undone.

The switch-mutation test deliberately drains one provider callback, disables
the federation-wide switch from that callback, then proves the later queued
provider solicitation is suppressed while the first provider's already-sent
value remains reflected. A fresh discovery after re-enabling can produce new
solicitation work.

## What the focused tests establish

| Concern | Source or test evidence | Bounded observation |
| --- | --- | --- |
| FDD-seeded switch and discovery-triggered plan | [Membership initialization](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L121); [registry planner](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L17); [embedded discovery callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L424) | Switch is read before planning; current owned, in-scope attributes are grouped by provider. |
| Default enabled path, grouping, and mandatory empty automatic tag | [Embedded baseline](../../cpp/tests/auto_provide_baseline_catch2.cpp#L84) | Discovery and provider solicitation are distinct callbacks; the provider receives the object's owned attribute set and zero-length tag. |
| Disabled switch does not block discovery | [Disabled baseline](../../cpp/tests/auto_provide_disabled_discovery_only_catch2.cpp#L84) | Receiver becomes aware of the object while no provider callback is recorded. |
| Regional scope and change during discovery | [Regional overlap case](../../cpp/tests/regional_auto_provide_overlap_catch2.cpp#L120) | Initial overlap solicits; moving the receiver out of overlap inside discovery suppresses the stale solicitation without undoing discovery. |
| Separate providers and queued switch mutation | [Two-provider switch case](../../cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp#L203); [multi-provider case](../../cpp/tests/regional_auto_provide_multi_provider_catch2.cpp#L193) | Provider attributes are split by current owner; a switch-off during the first callback fences a later queued callback; re-enabled fresh discovery can solicit again. |
| Switch mutation through the 2025 MOM | [Embedded HLAsetSwitches path](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_mom_controls.cpp#L559); [MOM test](../../cpp/tests/auto_provide_mom_catch2.cpp#L120) | A member changes the shared HLAautoProvide setting and all members observe it. |
| Callback-time switch, scope, membership, and ownership fence | [Provider projection recheck](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L547); [provider/owner projection](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L436); [provider callback adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L1590) | Auto Provide work is revalidated immediately before provider code; explicit pending requests take a separate path. |
| Timestamped admission edge | [Regional timestamped switch test](../../cpp/tests/regional_auto_provide_timestamped_switch_admission_catch2.cpp#L212) | This test adds a timestamped regional admission scenario; it is not a substitute for the full time-management/retraction guides. |

The cited Auto Provide scenarios are embedded-profile tests, including both
HLA_EVOKED and HLA_IMMEDIATE in their relevant cases. The bounded process
survey found explicit Request Attribute Value Update routing and
HLAautoProvide switch mutation in process-service code, but did not find a
process-side call to the discovery-triggered planner or a focused process
Auto Provide test. Treat automatic process-profile behavior as unverified and
do not infer parity from the explicit-request path or the shared switch value.

## Limits and edition boundary

- Auto Provide asks a provider to send; only a later provider update can
  produce a value reflection. This guide does not guarantee an application
  response or delivery.
- The figures abstract away the complete class-inheritance, update-rate,
  ownership-transfer, service-reporting, failure, and TSO/retraction matrices.
  Follow their dedicated guides and focused tests for those branches.
- The pinned API and tests here are IEEE 1516.1-2025. The separate
  [2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md) remains a
  distinct profile; no shared behavior is claimed.
- Mermaid syntax and relative references have not yet received visual
  renderer review. Keep this guide marked Drafted until GitHub's renderer or
  another approved renderer confirms the diagram layout.
