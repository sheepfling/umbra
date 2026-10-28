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
[Get Auto Provide Switch, clause 10.52](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L2218).
Those interface declarations name the surfaces; the lifecycle and gating
below describe current Umbra behavior from its source and selected tests.
Use the official [IEEE 1516.1-2025 edition page](https://standards.ieee.org/ieee/1516.1/6688/)
as the standards starting point and consult the standard for normative rules.

## Keep these neighboring paths distinct

| Path | Trigger and recipient | Payload or result | Key distinction |
| --- | --- | --- | --- |
| Auto Provide | A receiver newly discovers an object; each current owner of relevant in-scope attributes may receive a provider callback. | Object, that provider's attribute subset, and an empty tag in the current automatic path. | RTI-triggered solicitation; no application request tag initiated it. |
| Explicit Request Attribute Value Update | A federate invokes the service for an ordinary provider-backed object, class, or regional scope; eligible providers may receive the request callback. | The caller-supplied request tag is carried to providers. | A separate service; Auto Provide's switch and callback-time fence must not be applied to it by assumption. The joined-federate MOM object has a direct-reflection exception below. |
| Explicit request for an RTI-owned `HLAfederate` MOM object | A federate requests selected values from the joined-federate MOM instance. | The RTI directly reflects its current MOM values. | This special object path does not invoke a provider callback; it is neither Auto Provide nor the ordinary provider-backed request flow. See the [attribute-value update request guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md#special-case-rti-owned-mom-objects). |
| Initial attribute reflection | A receiver discovers an object and eligible values already exist in RTI state. | `reflectAttributeValues` carries values available to the RTI. | In the embedded ordinary path this is separate from Auto Provide planning. The current process source also plans ordinary selected values on a new discovery; a focused public test source asserts this for a late subscription, but this documentation review did not run it. The RTI-owned `HLAfederate` path remains a distinct snapshot-based route. Neither reflection is provider solicitation, and neither establishes process Auto Provide. See the [joined-federate MOM guide](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md#separate-2025-process-endpoint-evidence), the [ordinary process late-subscription boundary](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge), and the [initial-state versus DSE pending-send comparison](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#a-subscription-can-be-late-relative-to-two-different-events). |
| Attributes In/Out Of Scope | A known receiver/object/attribute changes scope. | Scope callback with an attribute set. | It reports scope, not a request for current values. See the [scope-advisory guide](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md). |
| Attribute relevance advisory | A known object's attribute relevance or effective update-rate designator changes. | The current owner may receive Turn Updates On/Off. | It reports changed demand; unlike Auto Provide, it does not solicit current values. See the [relevance-advisory guide](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md). |

The broader [object and interaction information-flow guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md)
shows explicit request/response flow. The [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
owns region geometry and general routing, while the
[time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md) owns timestamped
delivery and grants.

## Two solicitation paths, one provider callback

At the API boundary, an explicit Request Attribute Value Update is a federate-
invoked service (§6.21). The RTI later asks eligible providers through Provide
Attribute Value Update (§6.22). In the current embedded 2025 profile, Auto
Provide reaches that same provider callback after discovery, without a
federate invoking the explicit request service. The callback names attributes
to provide; it does not carry their values. A provider response is a separate
Update Attribute Values call (§6.10), whose tag is supplied by the provider.

- **Auto Provide:** The pinned API exposes
  [Get Auto Provide Switch (§10.52)](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L2218). In the
  current embedded profile, the RTI plans the request after discovery and invokes
  [`provideAttributeValueUpdate` (§6.22)](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L350) with
  the eligible attribute subset and an empty tag.
- **Explicit Request Attribute Value Update:** A federate invokes the object-instance
  [`requestAttributeValueUpdate` overload (§6.21)](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L876)
  or its object-class [overload (§6.21)](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L891); the
  provider later receives the same
  [`provideAttributeValueUpdate` callback (§6.22)](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L350)
  with its selected attribute subset and, in the current embedded object-request path, the caller's request tag.

For either path, any values are supplied later through the provider's
[`updateAttributeValues` call (§6.10)](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L727)
with a provider-supplied tag; ordinary receiver delivery rules still apply.

The API declarations establish the operation surfaces and arguments, not the
discovery trigger, complete provider eligibility rules, or callback ordering.
The Auto Provide trigger and empty automatic tag below are current embedded
profile findings; process-profile automatic discovery planning remains
unverified. The explicit-request tag handoff and separate provider response
are backed by the focused embedded object-request cases cited later. The API
also exposes a class-request overload, but that case is not represented as
tested behavior here. For the complete explicit request-scope, provider
projection, callback, response, and save/restore discussion, see the separate
[attribute-value update request guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md).

`requestAttributeValueUpdate` and `provideAttributeValueUpdate` exchange a
solicitation tag; any later reflection carries the provider's
`updateAttributeValues` tag and follows separate subscription, ownership,
DDM, transportation, and time-management rules. This tag relationship is
shown for the named embedded test cases, not asserted as complete normative
coverage of every API overload.

## Switch state is federation-wide

The current embedded registry seeds Auto Provide from the federation's
composed FDD switch value at creation. A member can read the value with
Get Auto Provide Switch. The current MOM HLAsetSwitches path updates the
federation-wide HLAautoProvide value, so all joined members observe the same
state. This is not the per-receiver Attribute Scope Advisory Switch.

~~~mermaid
stateDiagram-v2
  direction LR
  [*] --> Enabled: creation FDD seeds HLAautoProvide true
  [*] --> Disabled: creation FDD seeds HLAautoProvide false
  Enabled --> Disabled: HLAsetSwitches(HLAautoProvide=false)
  Disabled --> Enabled: HLAsetSwitches(HLAautoProvide=true)
~~~

| Event | Switch-state consequence | Discovery consequence |
| --- | --- | --- |
| Federation creation | The composed FDD seeds one federation-wide `HLAautoProvide` value. | Not applicable until an object is discovered. |
| `HLAsetSwitches` changes the value | All joined members observe the updated shared value. | The change does not rediscover existing objects. |
| Object discovery | The current value is read by the planner. | Enabled: qualifying provider solicitation may be planned. Disabled: discovery still proceeds, without Auto Provide solicitation. |

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

In the current embedded call graph, a later scope-in for an object the
receiver already knows is not a second Auto Provide trigger. The planner is
called from the post-discovery path; changing a subscription or region later
does not re-enter it for that known object. An application can use the
separate explicit request service when it needs another solicitation. This is
a source-derived implementation boundary, not a claim about what every RTI or
the IEEE standard requires, and no focused test isolates this no-retry edge.

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
  Note over R,RTI: Embedded profile: any initial reflection is a separate path before Auto Provide planning
  opt Eligible values are already available
    RTI-->>R: reflectAttributeValues(initial subscribed values)
  end
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

In the current embedded discovery path, any initial subscribed-value
reflections and initial attribute-relevance planning occur before the Auto
Provide planner. The initial reflection is not the provider's response and
does not carry the Auto Provide request: the provider callback remains a
separate solicitation, and a later provider update is still required to
produce that provider's new values. This relative planner order is
source-observed, not a universal callback-order claim. The [initial-value
test](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180)
covers a value written before the receiver subscribes, but it does not test
the combined initial-reflection/Auto-Provide sequence; see the
[embedded discovery path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L379).

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
| Explicit request tag and provider response tag are distinct | [Embedded object-request case](../../cpp/tests/attribute_value_update_request_baseline_catch2.cpp#L94); [embedded provider-response case](../../cpp/tests/attribute_value_update_response_catch2.cpp#L130); [process class-request case](../../cpp/tests/ieee1516_2025_connection_process_class_request_attribute_value_update_catch2.cpp#L400) | The embedded cases distinguish the caller's request tag at `provideAttributeValueUpdate` from the later response tag at `updateAttributeValues`. The process case requests one attribute for two registered objects from a single provider and observes two callbacks, one per object, each with the caller's tag. That is explicit Request Attribute Value Update evidence—not discovery-triggered Auto Provide or ordinary initial reflection. It does not establish same-object multi-provider fan-out or process Auto Provide. |
| FDD-seeded switch and discovery-triggered plan | [Membership initialization](../../cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp#L121); [registry planner](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L17); [embedded discovery callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L424) | Switch is read before planning; current owned, in-scope attributes are grouped by provider. |
| Initial reflection is separate from provider solicitation | [Embedded ordinary initial-reflection path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L379); [embedded initial-value case](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180); [process ordinary-value planning](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90); [process late-subscription test source](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652); [process HLAfederate selected-value case](../../cpp/tests/ieee1516_2025_connection_process_joined_federate_mom_report_file_catch2.cpp#L359) | Embedded and current process source both have ordinary initial-value paths, through different lifecycle boundaries. The process test source asserts selected stored values for a newly discovered ordinary object, but this documentation review did not run it. The MOM test separately observes selected `HLAfederateName` and `HLAreportServiceFile` values for its RTI-owned instance. None of these tests Auto Provide, and the combined Auto Provide sequence remains untested. |
| Default enabled path, grouping, and mandatory empty automatic tag | [Embedded baseline](../../cpp/tests/auto_provide_baseline_catch2.cpp#L84) | Discovery and provider solicitation are distinct callbacks; the provider receives the object's owned attribute set and zero-length tag. |
| Disabled switch does not block discovery | [Disabled baseline](../../cpp/tests/auto_provide_disabled_discovery_only_catch2.cpp#L84), [HLA_EVOKED setup](../../cpp/tests/auto_provide_disabled_discovery_only_catch2.cpp#L97), [discovery assertion](../../cpp/tests/auto_provide_disabled_discovery_only_catch2.cpp#L126), [no-provider assertion](../../cpp/tests/auto_provide_disabled_discovery_only_catch2.cpp#L128) | In this embedded HLA_EVOKED fixture, the receiver becomes aware of the object while the owner records no provider callback. It does not provide a disabled-baseline case under HLA_IMMEDIATE or in the configured-process profile. |
| Regional scope and change during discovery | [Regional overlap case](../../cpp/tests/regional_auto_provide_overlap_catch2.cpp#L120) | Initial overlap solicits; moving the receiver out of overlap inside discovery suppresses the stale solicitation without undoing discovery. |
| Later scope-in for an already-known object | [Post-discovery planner callsite](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L432); [planner definition](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L17) | The inspected embedded call graph enters Auto Provide from discovery, not from later scope mutation. No focused test directly verifies this no-retry edge; do not generalize it to the standard or process profile. |
| Separate providers and queued switch mutation | [Two-provider switch case](../../cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp#L203), [switch-off during first provider callback](../../cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp#L467), [suppression assertions](../../cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp#L496), [switch re-enable](../../cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp#L525), [fresh-discovery assertions](../../cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp#L536), [both callback-model sections](../../cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp#L585); [multi-provider case](../../cpp/tests/regional_auto_provide_multi_provider_catch2.cpp#L193) | Provider attributes are split by current owner; the test disables the federation-wide switch from the first provider callback, then verifies the queued second solicitation is suppressed and the first response remains reflected. After re-enabling, a fresh discovery solicits again. This tests callback-time fencing under both callback models; it is distinct from the disabled-before-discovery baseline above. |
| Switch mutation through the 2025 MOM | [Embedded HLAsetSwitches path](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_mom_controls.cpp#L559); [MOM test](../../cpp/tests/auto_provide_mom_catch2.cpp#L120) | A member changes the shared HLAautoProvide setting and all members observe it. |
| Process discovery delivery (not automatic provider solicitation) | [Process subscription-to-discovery handler](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L226); [process discovery service](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L54); [ordinary initial-value planner](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90); [public discovery callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L610); [embedded planner call site](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L432) | A successful process subscription replans existing-instance discoveries, then the service commits known-object state and emits discovery events. Ordinary discoveries can plan initial relevance advisories and selected current-value events; the RTI-owned MOM branch derives values from its separate snapshot. The public ordinary late-subscription test source asserts a selected-value reflection but was not run in this documentation pass. The scanned `planAutoProvideForDiscovery` call site remains embedded-only; these initial reflections are not provider solicitation. |
| Process HLAautoProvide switch handling | [Process switch handler](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L162); [focused process switch case](../../cpp/tests/ieee1516_2025_connection_catch2.cpp#L904); [server-side registry assertions](../../cpp/tests/ieee1516_2025_connection_catch2.cpp#L951) | The handler decodes the predefined parameter and updates the federation-wide switch. The case sends `false` and then `true` through public `HLAsetSwitches`; its server-side fixture verifies the registry value is `true` after Join, `false` after the first adjustment, and `true` after re-enabling. It does not exercise the public switch getter, arrange object discovery, or assert a `provideAttributeValueUpdate` callback. |
| Callback-time switch, scope, membership, and ownership fence | [Provider projection recheck](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L547); [provider/owner projection](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L436); [provider callback adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L1590) | Auto Provide work is revalidated immediately before provider code; explicit pending requests take a separate path. |
| Timestamped admission edge | [Regional timestamped switch test](../../cpp/tests/regional_auto_provide_timestamped_switch_admission_catch2.cpp#L212) | This test adds a timestamped regional admission scenario; it is not a substitute for the full time-management/retraction guides. |

The cited Auto Provide scenarios are embedded-profile tests, including both
HLA_EVOKED and HLA_IMMEDIATE in their relevant cases. The bounded process
source trace commits discovery state, emits a discovery event, and ultimately
delivers `discoverObjectInstance`; its separate RTI-owned MOM branch plans
initial values. A source call-site scan found `planAutoProvideForDiscovery`
only on the embedded callback route. The focused process HLAsetSwitches case
sends the public switch adjustments and checks the federation registry's
stored value from its server-side test fixture; it does not exercise the public
switch getter, object discovery, or a provider callback. No focused process
Auto Provide solicitation test was found. Treat automatic
process-profile behavior as unverified; do not infer parity from explicit
Request Attribute Value Update routing, process discovery delivery, or the
shared switch value. These are current implementation observations, not a
claim about what IEEE 1516.1-2025 requires.

### Configured-process discovery route: source-only boundary

This trace makes the observed configured-process boundary explicit. A
successful subscription can plan discovery for already-registered visible
objects; the service rechecks and commits receiver-known state before
queueing or sending the discovery event. Ordinary-object initial relevance
advisories and RTI-owned MOM initial values are separate paths. In the
inspected process chain, `HLAsetSwitches(HLAautoProvide)` updates shared switch
state independently, and no `planAutoProvideForDiscovery` call connects that
discovery route to provider solicitation. This is a bounded source observation,
not a normative omission or an embedded/process parity claim.

~~~mermaid
flowchart TD
  subgraph D["Configured-process subscription and discovery"]
    S["Successful object-class attribute subscription"] --> P["Plan discovery for currently visible registered objects"]
    P --> C{"Discovery predicate still holds?"}
    C -->|No| Skip["Skip or cancel the planned discovery"]
    C -->|Yes| Commit["Commit receiver-known object state"]
    Commit --> Event["Queue or send the process discovery event"]
    Event --> Bridge["Client callback bridge invokes discoverObjectInstance"]
    Commit -->|Ordinary object| AdvisoryPlan["Plan initial attribute-relevance advisories separately"]
    AdvisoryPlan --> AdvisoryEvent["Enqueue advisories after discovery events"]
    Event -.->|Process queue order| AdvisoryEvent
    Commit -->|RTI-owned MOM object| ValuesPlan["Plan selected subscribed MOM initial values separately"]
    ValuesPlan --> ValuesEvent["Queue or send the initial-value event"]
    Event -.->|MOM ordering| ValuesEvent
  end
  subgraph W["Separate process switch-control path"]
    Adjust["HLAsetSwitches with HLAautoProvide"] --> Decode["Process handler decodes the value and updates federation switch state"]
  end
  Boundary["Scanned process discovery chain has no planAutoProvideForDiscovery call; automatic process solicitation remains unverified"]
  Bridge -.-> Boundary
  Decode -.-> Boundary
~~~

## Limits and edition boundary

- Auto Provide asks a provider to send; only a later provider update can
  produce a value reflection. This guide does not guarantee an application
  response or delivery.
- The figures abstract away the complete class-inheritance, update-rate,
  ownership-transfer, service-reporting, failure, and TSO/retraction matrices.
  Follow their dedicated guides and focused tests for those branches.
- The relative ordering of initial reflection and Auto Provide planning is
  source-derived for the embedded discovery path; no focused case exercises
  both effects together, and no process ordering is inferred.
- A later scope-in for an already-known object has no second Auto Provide
  planner entry in the inspected embedded call graph; the focused tests do not
  isolate this edge, and it is not stated as a normative HLA rule.
- The pinned API and tests here are IEEE 1516.1-2025. The separate
  [2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md) remains a
  distinct profile; no shared behavior is claimed.
- The three current diagrams were rendered and visually reviewed in VS Code's
  Markdown Preview on 2026-10-11. The switch chart now limits the state diagram
  to initialization and switch transitions, with discovery effects in an
  adjacent table; the discovery sequence includes the separate initial-
  reflection step. The wide sequence and tall eligibility flow remain readable
  while scrolling. GitHub's post-publication rendering is still unverified.
