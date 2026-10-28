# HLA 1516.1-2025 object-attribute subscription state guide

In HLA, a subscription expresses receiver interest; it does not create a
publisher, object instance, or attribute value, and it does not promise that
every update becomes visible. The 1516.1-2025 API makes the declaration's scope
explicit: an object class, a selected attribute set, an active/passive choice,
and an optional update-rate designator
([`subscribeObjectClassAttributes`](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L535)).
To reason about the result, keep three things separate: the receiver's declared
interest, whether an object becomes known to that receiver, and which values
from a later update are actually reflected.

## Reader's model: interest is an input to separate outcomes

This concept map names the HLA-facing questions without claiming a complete
decision algorithm or callback ordering. The linked guides below explain the
separate routing, rate, advisory, and callback gates.

```mermaid
flowchart TD
  DECLARE["Receiver declares interest<br/>class, selected attributes,<br/>active/passive, optional rate"] --> INTEREST["Receiver-side subscription interest"]
  REGISTER["An object instance is registered"] --> DISCOVER_Q{"Does the applicable discovery path select this receiver?"}
  INTEREST -. "one relevant input" .-> DISCOVER_Q
  DISCOVER_Q -->|Selected| DISCOVER["Discover Object Instance"]
  UPDATE["Publisher updates attribute values"] --> VALUE_Q{"Which updated attributes qualify for this receiver?"}
  INTEREST -. "per-attribute interest is one input" .-> VALUE_Q
  VALUE_Q -->|None| NO_VALUE["No value reflection from this update"]
  VALUE_Q -->|One or more| GATES["Other applicable routing and delivery gates"]
  GATES -->|Delivered now| REFLECT["Reflect selected attribute values"]
  GATES -->|Not delivered now| DEFERRED["No reflection now; the reason depends on the gate and path"]
```

Discovery and value reflection are different observations: learning that an
object exists does not, by itself, make every attribute eligible for reflection.
Conversely, this map does not define the discovery or delivery predicates; the
implementation evidence and unresolved boundaries are stated separately below.

The [2025 Object and Interaction Information Flow guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md)
provides the end-to-end map from publication through receiver eligibility and
object knowledge to callback observation. This guide zooms into the receiver's
subscription declaration state. It is deliberately narrower than the adjacent
[DDM/regions](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md),
[relevance-advisory](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md),
[attribute-scope](HLA-2025-ATTRIBUTE-SCOPE-ADVISORY-FLOW-GUIDE.md), and
[update-rate](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md) guides:
it traces object-class attribute subscription declarations, not the full
recipient-routing or advisory state machines. Its core declaration-state
diagrams use focused embedded 2025 tests and source; process source and
observations are separately labeled below. It makes no process/embedded parity
claim and does not describe or infer 2010 behavior.

## 1. Ordinary declarations accumulate and change per selected attribute

`subscribeObjectClassAttributes` names a class, an attribute set, an `active`
flag, and an optional update-rate designator. In the embedded registry, each
selected attribute is inserted or replaced in that class's declaration map;
attributes omitted from the call remain as they were. Re-declaring one
attribute does not implicitly erase another. `unsubscribeObjectClassAttributes`
removes the selected ordinary entries; it is a different mutation from a
regional unsubscribe.

```mermaid
flowchart TD
  Call["Subscribe class attributes with active mode and rate"] --> Validate["Validate membership, class, attributes, and rate"]
  Validate --> Selected["Visit only attributes in this call"]
  Selected --> Write["Insert or replace each selected attribute's mode and rate"]
  Write --> Retain["Keep unselected ordinary declarations unchanged"]
  Retain --> Replan["Recompute receiver scope and resulting callbacks"]
  Remove["Unsubscribe selected class attributes"] --> Erase["Erase only the selected ordinary entries"]
  Erase --> RetainOther["Keep unselected and regional declarations distinct"]
  RetainOther --> Replan
```

The public API calls the mode `active`; the 2025 service-report argument is an
optional *passive* indicator, so the embedded implementation reports the
inverse of the API boolean. Do not read the same boolean spelling into both
surfaces. The internal registry also uses an absent mode to represent the
ordinary unsubscribe mutation; that is not a third public subscription mode.

After a successful change, Umbra replans object scope and may enqueue discovery,
scope-change, or relevance callbacks. The declaration mutation and any later
callback are separate boundaries: callbacks are queued after the registry
transaction, and the details of callback delivery belong to the linked
[callback/service-ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md).

### Empty-set subset unsubscribe is an unresolved profile seam

The pinned API has a separate whole-class `unsubscribeObjectClass` service and
the subset-oriented `unsubscribeObjectClassAttributes` service. Do not assume
from those signatures alone that an empty set passed to the subset service
means “unsubscribe everything.” In the current 2025 source, the embedded and
process routes interpret that input differently:

| Route | Empty set with an existing class declaration | Evidence boundary |
| --- | --- | --- |
| Embedded | The ambassador forwards the supplied set unchanged; the registry's selected-attribute erase loop removes nothing. The registry still advances the receiver-wide subscription generation for an existing class declaration. | [Embedded service entry](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_subscription.cpp#L331), [registry mutation](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L302). This describes current source behavior, not the intended API meaning. |
| Process | The client transmits the empty vector unchanged; the process service expands it to the class's currently stored ordinary `subscribedAttributes` before calling the registry. That expansion does not enumerate the separate regional entries. | [Process client forwarding](../../cpp/src/internal/federation/process_federation_client_declaration_services.cpp#L461), [service-side expansion](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L200). This is a process-route behavior, not proof of embedded/process parity or a normative rule. |

The bounded test scan found empty-set unsubscribe calls only in invalid-handle
cases; it found no focused public case that starts with valid ordinary
subscriptions and compares the resulting declaration state across both
profiles. The pinned header documents the two service signatures but does not
settle the empty-set meaning ([whole-class overload](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L550),
[subset overload](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L563)).
Treat this as an open source/test/API question. Do not rely on an empty subset
as either a no-op or a whole-class unsubscribe until the applicable normative
text and a focused valid-state observation are reviewed.

## 2. Discovery and value delivery are separate active-subscription gates

Passive declarations are retained state, but the focused embedded tests show
that a passive-only object-attribute subscription does not arrange instance
discovery or ordinary/regional value delivery. An active matching declaration
can make the object known; that does not make every declared attribute active.
For each update, attribute projection still includes only the receiver's
eligible attributes.

```mermaid
sequenceDiagram
  autonumber
  actor Subscriber as Subscriber application
  participant SRTI as Subscriber RTI
  participant Registry as Federation state
  participant PRTI as Publisher RTI
  actor Publisher as Publisher application

  Subscriber->>SRTI: subscribe A as passive
  Publisher->>PRTI: register object
  PRTI->>Registry: plan discovery for this receiver
  Note over Registry,Subscriber: Passive-only declaration does not discover in the focused embedded test
  Subscriber->>SRTI: subscribe A as active
  SRTI->>Registry: update only A's mode
  Registry-->>SRTI: queue object discovery
  SRTI-->>Subscriber: discoverObjectInstance(object)
  Subscriber->>SRTI: subscribe A active, subscribe B passive
  Publisher->>PRTI: update values for A and B
  PRTI->>Registry: apply receiver-specific eligibility and projection
  Registry-->>SRTI: project active eligible attribute A
  SRTI-->>Subscriber: reflectAttributeValues(A)
  Subscriber->>SRTI: change A to passive, change B to active
  Publisher->>PRTI: update values for A and B
  PRTI->>Registry: recompute the attribute projection
  Registry-->>SRTI: project active eligible attribute B
  SRTI-->>Subscriber: reflectAttributeValues(B)
```

This sequence combines transitions from the two focused subscription tests;
it is a teaching view, not a claim that one test executes this exact combined
transcript. Its vertical placement of discovery before later reflection is
scenario chronology, not by itself proof of a general callback-order rule.
See the [object-flow ordering audit](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#first-reflection-after-discovery-order-and-evidence-boundary)
for the process-source interpretation, the public test's assertion boundary,
and the missing normative clause text in the pinned interface header.

The focused additive-subscription case observes all three important outcomes:

- Adding passive B leaves active A declared; discovery still occurs through A,
  while a later update containing A and B reflects A only.
- Changing A to passive and B to active changes only those selected entries;
  the later reflection contains B only.
- Making A active again preserves B, and the following reflection contains
  both attributes.

The passive-only case separately exercises object registration followed by a
passive-to-active transition. The object becomes known after the active
subscription, while a passive attribute update is not reflected. “Object is
known” and “this attribute is currently eligible for value delivery” are
different pieces of receiver state; becoming known does not itself activate
every attribute.

### Passive is not a hard delivery veto

That focused case has one publisher and one subscriber, so it tests a
passive-only federation: no other member establishes active relevance. It
does not establish that a passive receiver can never be discovered or receive
values. In the shared 2025 `EmbeddedFederationRegistry` source, discovery and
per-attribute reflection first consider whether a matching active declaration
among non-producer members makes the object or attribute relevant; the
recipient is then checked against its own matching declaration. A passive
recipient may therefore qualify when a different member establishes active
relevance, while its own class and applicable region match still have to pass.

For discovery, the implementation keeps the active-relevance result as one
boolean while scanning non-producer declarations across the registered class
hierarchy and discoverable instance attributes, then performs the receiver's
own declaration lookup separately ([active scan](../../cpp/src/internal/federation/federation_registry.cpp#L873),
[receiver selection](../../cpp/src/internal/federation/federation_registry.cpp#L1037)).
The gate retains no attribute or class identity, so the second scan does not
pair the active-establisher entry with the passive receiver's entry. For
example, a publisher can register an object with attributes A and B; member X's
active declaration for A can establish discovery relevance, while member Y's
passive declaration for B can separately satisfy Y's receiver check, assuming
each side's class and region conditions match. This does not activate B for
value delivery: the update planner recomputes active relevance for each
updated attribute, so Y's passive B declaration alone cannot receive a B
reflection when no active B declaration exists. This source-derived example
is not an HLA rule or a result asserted by the cited passive-only test; that
test has one publisher and one subscriber
([test setup](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L120)).

The 2025 process service also holds this registry type. Its subscription
handler delegates the declaration change and existing-object discovery plan
to the registry ([subscription route](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L214));
registration delegates instance creation and per-instance discovery to the
same registry ([registration route](../../cpp/src/internal/federation/process_federation_service_request_handlers.cpp#L486)).
This establishes shared selection logic, not a process three-role result.

There is a focused public process object-subscription test, but its fixture
joins one member and exercises active Subscribe, passive Subscribe, and
Unsubscribe while checking that member's stored declaration
([test](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L458)).
It does not register an ordinary object or introduce a separate active
establisher. It therefore covers process request/declaration routing, not the
cross-member discovery branch described above. No focused process test
establishes a separate active member making a passive receiver discoverable.

```mermaid
flowchart TD
  subgraph DISCOVERY["Registration and discovery"]
    REGISTER["Publisher registers object with discoverable attributes A and B"] --> ACTIVE_DISCOVERY{"Does any non-producer member, e.g. X, have active relevance for a discoverable attribute, e.g. A?"}
    ACTIVE_DISCOVERY -->|"No"| NO_DISCOVERY["No induced discovery through this path"]
    ACTIVE_DISCOVERY -->|"Yes"| OWN_DISCOVERY{"Does this receiver, e.g. Y, separately match a discoverable attribute, possibly B, and its class/region?"}
    OWN_DISCOVERY -->|"No"| SKIP_DISCOVERY["No discovery for this receiver"]
    OWN_DISCOVERY -->|"Yes: active or passive"| DISCOVER["Discovery can be planned"]
  end
  subgraph REFLECTION["Later update and reflection"]
    UPDATE["Later update of attribute B"] --> ACTIVE_UPDATE{"Does a non-producer active declaration make this specific updated attribute B relevant under class/region rules?"}
    ACTIVE_UPDATE -->|"No"| NO_REFLECTION["No reflection through this relevance path"]
    ACTIVE_UPDATE -->|"Yes"| KNOWN{"Does this receiver know the object and have its own declaration for attribute B?"}
    KNOWN -->|"No"| SKIP_REFLECTION["No reflection for this receiver"]
    KNOWN -->|"Yes: active or passive"| PROJECT["Apply receiver-specific class/region projection"]
    PROJECT --> GATES["Apply other path-specific gates where applicable:<br/>update-rate, logical time, callback dispatch"]
    GATES --> REFLECT["May observe Reflect Attribute Values"]
  end
```

The source points are the [discovery active-relevance scan](../../cpp/src/internal/federation/federation_registry.cpp#L873),
[receiver selection](../../cpp/src/internal/federation/federation_registry.cpp#L1037),
and the [attribute-update recipient planner](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L76).
The [focused passive-subscription test](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L120)
does establish passive-only suppression and the same subscriber's later
passive-to-active transition, but it does not test a separate active member
making a passive receiver eligible. The process-service call path is described
below; the three-role end-to-end result remains untested there.

### The process endpoint reuses selection logic, not the delivery route

The 2025 process endpoint does not define a second passive-subscription
selector. Its service stores an `EmbeddedFederationRegistry` reference and
delegates the relevant operations to it:

- Subscription changes go through `setObjectClassAttributeSubscriptionWithScopeChanges`
  and, for a subscribe, the existing-object discovery planner.
- Registration goes through `registerObjectInstance` and the same per-instance
  discovery planner.
- Attribute updates go through `planReceiveOrderAttributeUpdate`; the process
  service then creates recipient events and queues or sends them over process
  sessions. The client-side callback bridge is a separate delivery boundary.

This establishes reuse of the same registry selection source at those process
service call sites, not general embedded/process parity. The public
[timestamped process attribute-retraction test](../../cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_fanout_catch2.cpp#L928)
does traverse active subscription, registration, and timestamped reflection
through a configured TCP endpoint, but it does not exercise a passive receiver
or isolate the shared passive-relevance branch. For framing and callback
dispatch after an event has been selected, see the [process-endpoint request
and callback guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md).

A separate public [receive-order process case](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457)
subscribes to the same inherited `Name` attribute through both a parent and a
child object class, sends one untimestamped update, and [observes exactly one
`reflectAttributeValues` callback](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L817)
under both HLA_EVOKED and HLA_IMMEDIATE. The test also observes a private
service [`recipientCount` of one](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L873);
the public untimed Update Attribute Values API itself returns `void`. In this fixture,
HLA_IMMEDIATE is paired with pushed process events and HLA_EVOKED with polling,
so this is not evidence that callback model generally dictates transport mode.
It covers active overlapping subscriptions only: it does not establish the
passive three-role branch, delayed subscription evaluation, regional process
projection, or embedded/process parity. The process guide's
[receive-order update trace](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#process-mapping-receive-order-object-attribute-update)
keeps this delivery mechanism separate from the HLA eligibility model.

These are selected observations, not a complete standards or profile matrix.
The [Delay Subscription Evaluation guide](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md)
keeps evaluation timing separate from per-attribute declaration state. Class
ancestry is also not one universal “inherit the subscription” operation:
relevance, discovery, later value projection, and regional matching consult
the class chain at different stages.

### Class ancestry affects separate decisions

In the current embedded 2025 source, declarations remain keyed by the class
and attribute supplied to the service; ancestry walks inspect those entries at
different points instead of copying each declaration into descendant entries.
The table is an implementation reading aid, not a complete normative rule.

| Path | What class ancestry changes | Evidence boundary |
| --- | --- | --- |
| Class-level declaration relevance | An active match at a published class or applicable ancestor can change the publisher-facing class relevance signal. This decision does not require a particular object instance or regional overlap. | The [relevance guide](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md#class-level-declaration-relevance) separates this signal from per-instance relevance and delivery; it does not prove that a value will be delivered. |
| Discovery and receiver-known class | After a joined non-producer's active subscription establishes relevance for a discoverable instance attribute under the applicable region predicate, the embedded resolver walks from the object's registered class toward its parents for each receiver. The closest qualifying receiver declaration selects the class the receiver learns; a parent-known object does not expose child-only attributes. A passive receiver can qualify at this recipient stage, but cannot establish the active gate itself. | The [resolver](../../cpp/src/internal/federation/federation_registry.cpp#L830) and [focused child-object/base-subscription case](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L297) support this embedded path. That case uses an active receiver; the separate-active-establisher/passive-receiver combination is not covered. The [object-flow ancestry map](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#class-ancestry-has-different-jobs-in-different-paths) gives the wider context. |
| Later value projection | The embedded planner starts with this receiver's saved known class, then applies attribute visibility, subscription/relevance, and any regional overlap to the update. An eligible passive receiver still depends on active relevance being established elsewhere. | See the [recipient planner](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L49) and the [process parent/child overlap case](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457): it observes one callback for overlapping active declarations on the same inherited attribute. It does not cover passive or regional cross-class delivery, nor establish process/embedded parity. |
| Regional declaration lookup | Regional pairs are stored under the exact class/attribute key, while the embedded discovery path walks the registered class and its parents and checks entries along that walk. A parent-keyed regional declaration can therefore be considered for a child object in this source path; the declaration is not copied into the child entry. | The [DDM ancestry guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#object-class-ancestry-regional-discovery-and-value-projection) marks this cross-class result source-only: reviewed regional tests register and subscribe at the same class. No process or 2010 result is established. |

A subscription change may consequently lead to different callbacks depending
on whether this receiver knew the object already: discovery and embedded
initial-value work for a newly known object; scope advice for an already-known
receiver; and a separate owner-facing relevance advisory. The existing
[interest-change map](#one-interest-change-can-produce-distinct-observable-effects)
shows those lanes without claiming a total callback order. The [relevance
guide](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md) and [callback guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md)
own their separate trigger and delivery details.

### One interest change can produce distinct observable effects

A subscription is not merely a gate on the publisher's *next* update. When an
interest declaration changes, matching registered instances may be evaluated
again. Keep the resulting observations separate:

- If the receiver did not know a now-discoverable instance, `Discover Object
  Instance` establishes its object identity and known class. This can happen
  for an instance that was registered earlier.
- Values already held for the receiver's subscribed attributes may then be
  reflected as the initial state for that newly known object. That reflection
  is not itself a new publisher `Update Attribute Values` invocation. The
  embedded 2025 callback path explicitly performs this after the discovery
  callback returns; its source cites §5.8 for the initial values.
- If the instance was already known, a subscription/region change can instead
  change attribute scope, which is a receiver-facing scope advisory rather
  than another discovery.
- Separately, a change in whether an attribute is relevant can produce an
  owner-facing `Turn Updates On/Off` advisory when that provider's advisory
  switch allows it. This reports relevance; it carries no attribute value and
  goes to a different federate from the subscription receiver.
- In the embedded 2025 path, discovery may also trigger Auto Provide: the RTI
  asks current owners to provide values, but the callback is not the values.
  A provider must make a separate update for data to be delivered. This is
  distinct from both the direct initial reflection and the owner-facing
  relevance advisory. The [Auto Provide guide's neighboring-path table](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md#keep-these-neighboring-paths-distinct)
  compares these with explicit Request Attribute Value Update.

### “Late subscription” can mean two different receiver states

The important boundary is not just whether the object was registered before
the subscription. Ask whether this receiver already knows the object:

| Receiver state | Current-value boundary in the bounded 2025 evidence |
| --- | --- |
| Existing object, not yet known to this receiver; subscription makes it discoverable. | Embedded and configured-process paths both have focused test sources that observe discovery plus a reflection of selected stored values. The process case subscribes after registration/update, selects only `Name`, and asserts its stored bytes under both callback-model sections ([test case](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652), [assertions](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L952)). The process source change is present in the current worktree but was not built or tested in this documentation pass. Neither test independently verifies the licensed §5.8 rule or a universal callback-order guarantee. |
| Receiver already knows the object and adds another attribute. | The discovery planner skips known instances and ordinary initial-value planning is discovery-scoped ([planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L105)). The process initial-reflection planner is reached from the discovery enqueue path, not from a value-only subscription mutation ([process enqueue](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90)). A public process test adds `PayRate` after discovery and observes the provider-facing relevance advisory, but has no subscriber reflection callback or value update ([late subscription](../../cpp/tests/ieee1516_2025_connection_attribute_relevance_advisory_catch2.cpp#L819), [owner advisory](../../cpp/tests/ieee1516_2025_connection_attribute_relevance_advisory_catch2.cpp#L830)). Whether adding an attribute to an already-known object alone reflects its current value remains untested and normatively unresolved here. |
| Federate separately invokes Request Attribute Value Update. | This is a distinct provider solicitation: any value requires a later provider update. RTI-owned joined-federate MOM requests have a separate direct-reflection exception; see the [neighboring-path comparison](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md#keep-these-neighboring-paths-distinct). |

The source trace for the already-known row is more specific than the test
result. Subscription mutation returns scope-change and relevance-advisory
plans, not current-value recipients ([registry mutation](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L261)).
Replanning discovery skips an instance already present in the receiver's
known-object map ([planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L105)).
The embedded ordinary initial-value planner is called from the post-discovery
callback path, after `Discover Object Instance` has entered application code
([callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L375)).
The process subscription handler also replans discovery. The current process
enqueue path plans ordinary initial values only for a newly accepted discovery;
the known-object filter suppresses that path when a receiver merely adds an
attribute ([handler](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L176),
[discovery planner](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90)).
Thus no ordinary snapshot path is visible for an already-known object in the
inspected Umbra source. The focused `PayRate` advisory test observes the owner
notification, not a value-only reflection. This is not a normative conclusion:
the licensed §5.8 text remains unreviewed, and the already-known-object case is
not isolated by a current-value test.

The nearest embedded additive/mode test
([case](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L355))
declares both attributes before object registration. Its later active/passive
mode changes are followed by explicit publisher updates before reflection is
asserted; it does not isolate whether changing a subscription by itself
reflects an already-held value for an already-known object.

### A subscription can be late relative to two different events

The receiver-state distinction above is separate from whether a publisher has
already sent an update. A discovery-time initial reflection may carry a value
previously set by `Update Attribute Values`; it is still an initial-state
reflection for a newly known object, not a replay of that publisher invocation.
Contrast it with an earlier update whose recipient decision is still pending:

| | Existing object becomes newly discoverable | A prior send is pending later eligibility evaluation |
| --- | --- | --- |
| What is late? | The receiver's interest declaration follows object registration, and the receiver did not previously know that object. | The publisher's update precedes its delivery-time eligibility decision; HLA-level DSE defers that decision. Umbra represents this with a retained candidate. |
| What may be observed? | Discovery followed by initial reflection of currently held, eligible subscribed values in the embedded 2025 source/test path; the licensed §5.8 interpretation remains open. | Delivery of that already-issued update if it passes the later eligibility check; a TSO callback also has its time-delivery boundary. |
| What this does not mean | It is not a callback replay of a past `Update Attribute Values` call, though the current value may have come from one. | It is not a general snapshot of current state caused by every new subscription. |

See the [Delay Subscription Evaluation guide](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md)
for the HLA-level deferred-eligibility distinction and Umbra's bounded
route-only candidate mechanism. The already-known-object/new-attribute case in
the row above remains unresolved for ordinary current-value reflection; DSE
evidence does not answer it because it concerns a previously sent update, not
a discovery-time state snapshot.

For an already-known object, keep receiver scope advice, provider relevance
advice, and a possible current-value reflection as separate effects. The
inspected 2025 registry returns scope-change and owner-facing relevance plans,
not current-value recipients. In particular, a scope advisory is not a
value callback, and it does not decide the unresolved question of whether
adding a subscribed attribute to a known object yields its current value. The
diagram therefore evaluates the receiver-advisory gate and the new-attribute
value question independently.

The closest embedded additive-subscription test changes per-attribute modes
after discovery and checks the projection of a later publisher update; it does
not isolate a value-only callback between the subscription and that update
([focused case](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L355)).
The process advisory test likewise subscribes after discovery and observes an
owner relevance advisory, but does not assert subscriber value reflection
([focused case](../../cpp/tests/process_federation_service_attribute_relevance_advisory_catch2.cpp#L78)).
Keep both as bounded evidence, not proof that either endpoint must or must not
send a current value. RTI-owned MOM initial values remain a separate special
path and must not be used to fill in the ordinary-object answer.

The chart is an HLA mental model for the distinct state and recipient
boundaries, not a total callback-order guarantee across federates. The process
profile path is described separately below.

#### Embedded discovery reentrancy precedes initial-value planning

This is an embedded 2025 source checkpoint, not a total callback-order
guarantee across federates. The process-profile path is described separately
below rather than assumed equivalent. Verify the exact normative wording and
ordering against an authorized copy of IEEE 1516.1-2025 §5.8 before treating
the source comment as a conformance conclusion; the [official IEEE
listing](https://standards.ieee.org/ieee/1516.1/6688/) does not expose the
licensed clause text.

The pinned 2025 [`FederateAmbassador` API](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L229)
declares `discoverObjectInstance` under §6.9 and `reflectAttributeValues`
under §6.11 ([untimed](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L238),
[timestamped](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L249)).
These separate callback signatures identify the API boundary; they do not
establish a normative relative order between the callbacks.

```mermaid
flowchart TD
  CHANGE["Subscription or regional-interest change"] --> REEVAL["Re-evaluate each matching registered instance"]
  REEVAL --> KNOWN{"Already known to this receiver?"}
  KNOWN -->|"No; now discoverable"| DISCOVER["Invoke Discover Object Instance callback; known-object state is committed"]
  DISCOVER --> REENTRY{"Application changes current RTI state during Discover callback?"}
  REENTRY -->|"Unsubscribe, resign, or change region"| MUTATE["Apply reentrant change before snapshot planning"]
  REENTRY -->|"No relevant mutation"| CURRENT["After callback returns, recheck live membership and recipient eligibility"]
  MUTATE -->|"Callback returns"| CURRENT
  CURRENT -->|"Eligible stored values remain"| INITIAL["Reflect stored values still eligible under current state"]
  CURRENT -->|"Member left or no values remain eligible"| NOINITIAL["No stale initial reflection from this plan"]
  DISCOVER -.-> AUTOGATE{"Separate embedded Auto Provide path: enabled and qualifying owner attributes?"}
  AUTOGATE -->|"Yes"| AUTOPROVIDE["Provide Attribute Value Update solicits owner; a later Update carries any values"]
  AUTOGATE -->|"No"| NOAUTOPROVIDE["No provider solicitation"]
  KNOWN -->|"Yes"| SCOPE["Re-evaluate this receiver's attribute scope"]
  SCOPE --> DELTA["Compute the receiver's per-attribute scope delta"]
  DELTA --> SCOPEGATE{"Scope changed and receiver's scope-advisory switch enabled?"}
  SCOPEGATE -->|"Yes"| SCOPECALLBACK["Attribute Scope Advisory to receiver"]
  SCOPEGATE -->|"No"| NOSCOPE["No scope advisory from this transition"]
  DELTA --> NEWATTRIBUTE{"Did the declaration add a subscribed attribute to this known object?"}
  NEWATTRIBUTE -->|"Yes"| VALUEGAP["Open: whether a current-value callback follows is not established"]
  NEWATTRIBUTE -->|"No"| NOVALUEQUESTION["No known-object/new-attribute value question"]
  CHANGE -.-> OWNERGATE{"Separate provider-side transition: did relevance change and is its switch enabled?"}
  OWNERGATE -->|"Yes"| OWNER["Turn Updates On/Off to attribute provider; no value is carried"]
  OWNERGATE -->|"No"| NOOWNER["No owner advisory"]
```

The embedded callback commits receiver-known state before calling
`discoverObjectInstance`; only after that callback returns does it call the
initial-reflection planner. The source comment explicitly names reentrant
unsubscribe, resignation, and region change as reasons to plan afterward. The
planner rechecks current membership/object state and passes currently stored
attribute handles through the receive-order recipient selector, so this is a
source-derived stale-work fence—not a tested callback-order or process-parity
claim. See the [callback boundary](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L323),
[planning checkpoint](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L376),
[initial-value planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L153),
and [recipient selector](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L15).
The cited [embedded stored-value case](../../cpp/tests/regional_object_attribute_routing_catch2.cpp#L198)
and [process late-subscription case](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652)
do not re-enter RTI services from the discovery callback to test this fence.

The embedded regional-routing test demonstrates the distinction between a
stored value and a later update: an already-registered object becomes
discoverable after regional relevance is established, the latest
accepted value is reflected with discovery, and a subsequent publisher update
is a separate reflection ([focused test](../../cpp/tests/regional_object_attribute_routing_catch2.cpp#L198)).
The ordinary-subscription advisory test likewise registers first and subscribes
later, then checks the provider's advisory separately ([focused test](../../cpp/tests/attribute_relevance_advisory_catch2.cpp#L335)).
These are embedded 2025 examples; neither establishes process-profile behavior
or the full normative callback ordering.

#### Embedded trace: late regional relevance exposes the stored value

This focused scenario makes the timing concrete. A publisher registers an
object with a source region, while the subscriber's regional declaration is
disjoint. The publisher accepts value V1 while the object is not discoverable
to that receiver. When committed region bounds later overlap, the receiver
learns the object and the embedded callback path reflects the then-current V1;
a subsequent publisher update to V2 is a separate reflection.

```mermaid
sequenceDiagram
  actor Publisher as Publishing federate
  participant RTI as Embedded 2025 RTI
  actor Subscriber as Subscribing federate

  Publisher->>RTI: Register object using source region 0 to 1
  Subscriber->>RTI: Subscribe Flavor using receiver region 2 to 3
  Publisher->>RTI: Update Flavor with V1
  Subscriber->>RTI: Call evokeMultipleCallbacks(0, 0)
  RTI-->>Subscriber: Return false, no callback work
  Note over RTI,Subscriber: The test observes no discovery or reflection while regions are disjoint

  Publisher->>RTI: Associate the source region
  Subscriber->>RTI: Commit receiver region 0 to 1 and repeat the subscription
  Subscriber->>RTI: Drain HLA_EVOKED callbacks
  RTI-->>Subscriber: Discover Object Instance
  Subscriber-->>RTI: Discover callback returns
  Note over RTI,Subscriber: Embedded source replans eligible initial values after the callback returns
  RTI-->>Subscriber: Reflect currently held Flavor value V1

  Publisher->>RTI: Update Flavor with V2
  RTI-->>Subscriber: Reflect the separate V2 update
```

The [focused regional-routing test](../../cpp/tests/regional_object_attribute_routing_catch2.cpp#L263)
asserts the disjoint no-callback case, one discovery, one reflection containing
V1, and a separate later reflection containing V2. The [embedded callback
bridge](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L325)
invokes `Discover Object Instance` before asking the registry to plan initial
reflections after that callback returns. The test stores discovery and
reflection reports separately, so the relative ordering in the diagram is
source-observed, not a callback-order assertion made by that test. This is an
embedded 2025 trace, not a normative rule or process-profile result.

#### Process late-subscription path: same selectors, an unresolved initial-value edge

The 2025 process subscription handler asks the shared registry to plan
discoveries for already-registered instances, then routes discovery, scope,
and provider-advisory work through process sessions. For a newly discovered
ordinary object, the current process enqueue path commits receiver-known state,
plans selected initial values from the registry, and stages separate discovery
and attribute-update events ([subscription handler](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L176),
[discovery/value planning](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90)).
The RTI-owned MOM path remains distinct: it derives values from the joined-
federate snapshot and uses an invalid producer handle. Do not infer that the
ordinary and MOM sources are identical merely because both reach
`reflectAttributeValues`.

The public configured-process case
[`RTIambassador reflects stored subscribed values for an object discovered by a configured-process subscription`](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652)
sets `Name` and `Efficiency` before the receiver subscribes to `Name`. Its test
source asserts one discovery and one reflection for the same object, the
sender's producer handle, valid transportation, exactly the selected `Name`
attribute, and its stored bytes; `Efficiency` is absent ([assertions](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L952)).
The fixture has `HLA_EVOKED` and `HLA_IMMEDIATE` sections. In this test only,
the server pairs Evoked with polling and Immediate with pushed events; the
polling section receives discovery and initial reflection as separate
receive-order responses ([fixture option](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L726),
[poll receives](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L812)).
The test source is not a passing result: this documentation review did not run
it, and the process discovery implementation is currently modified in the
worktree. The licensed §5.8 text also remains unavailable here, so this is a
source/test-shape description, not a normative or conformance conclusion.

Polling appends a discovery event and then each nonempty ordinary initial-
value event to the receiving session's separate queues. Push stages discovery
frames separately from value frames and sends the discovery batch first
([poll queueing](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L190),
[push discovery send](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L213),
[push value send](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L226)).
Those are source-order boundaries for this enqueue operation, not a universal
callback-order guarantee across recipients, sessions, or callback models.

For an already-known object, the distinction remains open: after discovery,
the receiver can subscribe to `PayRate` ([public subscription call](../../cpp/tests/ieee1516_2025_connection_attribute_relevance_advisory_catch2.cpp#L819))
and the provider observes `Turn Updates On` ([assertion](../../cpp/tests/ieee1516_2025_connection_attribute_relevance_advisory_catch2.cpp#L830)),
but that test has no subscriber reflection callback and sends no update. The
private process fixture establishes the same narrower advisory path. The
current source does not re-enter the discovery-scoped initial planner for a
known object when only its subscription changes; whether a value-only callback
is required remains untested here. The licensed §5.8 text is not included in
the API-header import; the [official IEEE listing](https://standards.ieee.org/ieee/1516.1/6688/)
directs readers to purchase/subscription access. Push sends and pull queues
also use different session mechanisms, so handler enqueue order is not a
global callback order.

For a reader, first ask whether the receiver already knows the object. Becoming
aware of an existing object and adding an attribute to an already-known object
are different transitions. The following is only a source/profile comparison.
The embedded implementation cites §5.8 in a source comment, but this diagram
does not independently verify that licensed text or establish what the process
profile is required to do.

```mermaid
flowchart TD
  SUB["Receiver subscribes after object registration"]
  KNOWS{"Does this receiver already know<br/>the object?"}
  SUB --> KNOWS

  subgraph NEW["Object exists but is not yet known to this receiver"]
    ELIGIBLE["Updated subscription makes a matching object discoverable"]
    subgraph EMBEDDED["2025 embedded development-profile source"]
      EPLAN["Registry plans discovery"]
      EDISC["Callback bridge invokes Discover Object Instance"]
      EVALUES["After the callback returns, registry plans current eligible values"]
      ETEST["Focused test observes discovery and the selected stored value"]
      EPLAN --> EDISC --> EVALUES --> ETEST
    end
    subgraph PROCESS["2025 process-profile source"]
      PPLAN["Subscription handler plans discoveries"]
      PDISC["Enqueue path commits known-object state and creates a discovery event"]
      PKIND{"RTI-owned MOM instance?"}
      PMOM["MOM: selected values from the joined-federate snapshot"]
      POBJECT["Ordinary: plan currently eligible subscribed stored values"]
      PORDER["Stage discovery before separate initial-value event"]
      PREFLECT["Process callback bridge maps values to Reflect Attribute Values"]
      PPLAN --> PDISC --> PKIND
      PKIND -->|"Yes"| PMOM --> PORDER
      PKIND -->|"No: ordinary object"| POBJECT --> PORDER
      PORDER --> PREFLECT
    end
    ELIGIBLE --> EPLAN
    ELIGIBLE --> PPLAN
  end

  subgraph KNOWN["Receiver already knows object; subscribes to another attribute"]
    CHANGE["Subscription declaration changes"]
    EFFECTS["Plan receiver scope changes and provider relevance advisories"]
    REPLAN["Replan discovery for eligible instances"]
    SKIP["Known-object state suppresses another discovery"]
    VALUEOPEN["No new discovery; current-value-only effect remains open"]
    ADVTEST["Public process test observes provider advisory,<br/>not a value-only callback for the known object"]
    CHANGE --> EFFECTS
    CHANGE --> REPLAN --> SKIP --> VALUEOPEN
    EFFECTS --> ADVTEST
  end
  KNOWS -->|"No"| ELIGIBLE
  KNOWS -->|"Yes"| CHANGE
```

The process lane separates registry planning from application callback
observation. The ordinary branch obtains current eligible values from the
shared object-instance registry after discovery state is committed; the MOM
branch obtains selected values from its joined-federate snapshot. Both become
separate process value events. The focused ordinary process test source
asserts discovery and selected stored-value reflection in both callback-model
sections, but this documentation pass did not run it. Its HLA_EVOKED fixture
uses polling and expects separate discovery/value receives; its HLA_IMMEDIATE
fixture enables pushed events. This is a case-specific source/test boundary,
not a normative callback-order rule or process/embedded parity claim. The MOM
test independently covers its special value path ([MOM test](../../cpp/tests/ieee1516_2025_connection_process_joined_federate_mom_report_file_catch2.cpp#L328));
see the [MOM event-staging note](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#2025-process-profile-federation-mom-after-an-additional-module-join)
for that branch's pull-queue versus push-batch distinction.
The private ordinary post-registration subscription fixture first polls the
object's discovery, then subscribes to `PayRate`; its assertion is the
provider's relevance advisory, not a receiver `Reflect Attribute Values`
callback ([discovery](../../cpp/tests/process_federation_service_attribute_relevance_advisory_catch2.cpp#L356),
[subscription](../../cpp/tests/process_federation_service_attribute_relevance_advisory_catch2.cpp#L386),
[owner advisory](../../cpp/tests/process_federation_service_attribute_relevance_advisory_catch2.cpp#L413)).
It uses the private process service protocol, so it is not public-adapter
evidence for ordinary value reflection. Licensed §5.8 text remains unreviewed;
the ordinary result is still a source/test/normative question—not a
conformance finding or a profile-parity conclusion.

The embedded-to-process comparison is not a global callback-order claim: the
process push and pull paths use different session mechanisms. Sources and
bounded test evidence: [embedded discovery callback](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L325),
[embedded current-value planning](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L375),
[initial-reflection planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L154),
[regional stored-value observation](../../cpp/tests/regional_object_attribute_routing_catch2.cpp#L668),
[process subscription handler](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L176),
[process discovery enqueue and ordinary/MOM initial-value planning](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L11),
[ordinary process late-subscription case](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652),
[process client discovery dispatch](../../cpp/src/internal/federation/process_federation_client_callback_dispatch.cpp#L84),
[process discovery callback mapping](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L622),
[process client value dispatch](../../cpp/src/internal/federation/process_federation_client_callback_dispatch.cpp#L23),
[process attribute-value callback mapping](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L362),
[joined-federate MOM observation](../../cpp/tests/ieee1516_2025_connection_process_joined_federate_mom_report_file_catch2.cpp#L359).

## 3. Regional declarations are separate per-attribute region entries

`subscribeObjectClassAttributesWithRegions` addresses attribute/region-set
pairs. The embedded registry records active/passive mode and update-rate
designator for each supplied attribute/region entry. Regional declarations are
stored separately from ordinary declarations; keep the two unsubscribe forms
and their region lifetimes distinct.

```mermaid
flowchart TD
  Call["Subscribe with attribute/region pairs"] --> Validate["Validate member, class, and attribute handles"]
  Validate --> HasPairs{"Pair collection nonempty?"}
  HasPairs -->|No| EmptyCall["Return; no declaration generation change"]
  HasPairs -->|Yes| ValidateRegions["Validate designator and supplied region handles"]
  ValidateRegions --> Each["Visit each attribute and its region set"]
  Each --> HasRegions{"This set has region handles?"}
  HasRegions -->|No| Preserve["Keep existing regional pairs unchanged"]
  HasRegions -->|Yes| Upsert["Insert or replace these pair modes and rates"]
  Preserve --> Next{"More attribute entries?"}
  Upsert --> Next
  Next -->|Yes| Each
  Next -->|No| Commit["Commit regional pair maps"]
  Commit --> Generation["Advance per-federate declaration generation"]
  Generation --> Scope["Recompute scope and regional receiver work"]
  Scope --> Key["For reduced streams, generation changes the admission key"]
  Key --> First["First sample for a new key starts fresh gate history"]
  Unsubscribe["Unsubscribe selected attribute/region pairs"] --> Erase["Remove only the named regional entries"]
  Erase --> Changed{"Did an entry change?"}
  Changed -->|Yes| Generation
  Changed -->|No| Scope
```

The passive/regional test verifies that a passive overlapping pair does not
discover or reflect for the tested object. Replacing that pair as active makes
the object discoverable and permits an overlapping update. It also submits
attribute entries with empty region sets and different active/rate arguments;
the retained pair's queried rate and active delivery remain unchanged in that
case. For the overlap calculation and region resource lifecycle, use the
[DDM/regions guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md).

### A source/test boundary around the empty-region case

For a nonempty attribute/region-pair map whose region sets contain no handles,
the registry leaves the regional pairs unchanged but still advances the
receiver's `subscriptionGeneration`; a fully empty pair map returns before
that update. Regional unsubscribe differs: its generation changes only when
at least one stored pair is actually removed. The generation is stored with
the receiver's declaration set, copied into each recipient projection, and
included in the update-rate admission key. The gate admits the first
observation for a new key, and its deterministic unit test verifies fresh
history for the same receiver/object/attribute identity under a changed
generation. The combined
source and unit-test chain therefore implies that this empty-region call can
start fresh admission history for that receiver's active attribute streams,
despite adding no regional pair. The public passive-subscription test checks
the retained mode, rate, and later delivery, but does not seed a prior rate
window and assert its reset across this exact call. So this is a source/test-
derived implementation inference, not a normative conclusion or an end-to-end
observation. The test source itself comments that §9.8.4 makes the empty-region
call a no-op ([test-author interpretation](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L253));
that comment records the test author's reading, not an independent verification
of the normative clause. The local checkout does not contain the full licensed standard;
the [official IEEE listing](https://standards.ieee.org/ieee/1516.1/6688/)
indicates access via subscription. Keep the conformance question open until an
authorized copy of the precise regional-subscription rule is reviewed. See the
[update-rate admission guide](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md)
for the separate rate gate.

## Evidence map

| Boundary | Source or focused test | What it supports |
| --- | --- | --- |
| 2025 public ordinary subscribe/unsubscribe | [RTIambassador.h:535](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L535), [RTIambassador.h:563](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L563) | Selected attribute set, active flag, update-rate designator, and separate unsubscribe service |
| 2025 public regional subscribe/unsubscribe | [RTIambassador.h:1559](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1559), [RTIambassador.h:1578](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1578) | Attribute/region pairs and the distinct regional unsubscribe form |
| Embedded ordinary declaration update | [Ambassador entry point:19](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_subscription.cpp#L19), [registry per-attribute upsert:261](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L261), [selected attribute writes:359](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L359) | Validation, per-attribute mode/rate replacement, and scope recalculation |
| Empty-set subset unsubscribe source seam | [Whole-class API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L550), [subset API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L563), [embedded registry erase](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L302), [process client forwarding](../../cpp/src/internal/federation/process_federation_client_declaration_services.cpp#L461), [process expansion](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L200) | Current-source difference only: embedded removes no selected ordinary entries; process expands to ordinary entries and leaves regional entries separate. No valid-state public test or API comment establishes the intended empty-set meaning |
| Embedded regional mutation | [Ambassador entry point:463](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_subscription.cpp#L463), [registry regional update:422](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L422), [regional pair removal:582](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L582) | Separate regional map, region validation, per-pair changes, and regional removal |
| Passive versus active delivery/discovery | [Discovery active-relevance scan](../../cpp/src/internal/federation/federation_registry.cpp#L873), [receiver selection](../../cpp/src/internal/federation/federation_registry.cpp#L1037), [attribute-update recipient planner](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L76), [passive object subscription test](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L120) | Source permits a matching passive receiver after non-producer active relevance; in discovery, the active establisher and passive receiver need not match on the same attribute in this source path. The focused embedded test covers passive-only suppression and passive-to-active transition, not a separate active establisher plus passive receiver |
| Process-service selection and event boundary | [Process service registry reference](../../cpp/src/internal/federation/process_federation_service.hpp#L681), [subscription handler](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L176), [registration handler](../../cpp/src/internal/federation/process_federation_service_request_handlers.cpp#L502), [receive-order update planner call](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L44), [process endpoint guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md) | Process handlers call the same registry selection methods, then use process-session event delivery; the cited process test exercises active, timestamped traffic, not the passive three-role branch |
| Receive-order process reflection with overlapping classes | [Shared recipient/passel planner call](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L44), [push event emission](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L304), [private result](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L325), [public process test](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457) | One untimestamped update is observed once despite parent-plus-child active declarations, under both callback-model sections. The fixture pairs HLA_IMMEDIATE with push and HLA_EVOKED with polling; it does not establish passive three-role, DSE, regional, or embedded/process parity |
| Late discovery, initial values, and separate observer effects | [Embedded callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L266), [initial reflection planner](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L154), [regional discovery/reflection test](../../cpp/tests/regional_object_attribute_routing_catch2.cpp#L198), [owner advisory test](../../cpp/tests/attribute_relevance_advisory_catch2.cpp#L335), [Auto Provide neighboring-path comparison](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md#keep-these-neighboring-paths-distinct) | Embedded 2025 source/test evidence separates discovery, direct initial current-value reflection, provider-facing relevance advice, and the distinct provider solicitation; it is not normative verification or process parity |
| Process late-subscription evidence boundary | [Process subscription handler](../../cpp/src/internal/federation/process_federation_service_declarations.cpp#L176), [ordinary/MOM discovery enqueue](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L11), [ordinary initial-value planner](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90), [discovery callback mapping](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L622), [attribute-value callback mapping](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L362), [public ordinary late-subscription test source](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652), [selected-value assertions](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L952), [public post-discovery `PayRate` advisory](../../cpp/tests/ieee1516_2025_connection_attribute_relevance_advisory_catch2.cpp#L819), [joined-federate MOM initial-value case](../../cpp/tests/ieee1516_2025_connection_process_joined_federate_mom_report_file_catch2.cpp#L359), [private process advisory test](../../cpp/tests/process_federation_service_attribute_relevance_advisory_catch2.cpp#L78) | Current source plans selected ordinary initial values for a newly accepted discovery, and the public test source asserts selected stored-value reflection under both callback-model sections; the implementation is modified in the worktree and the test was not run in this docs pass. The `PayRate` test for an already-known object still checks only provider advisory, so value-only behavior after adding an attribute remains unresolved. The MOM test covers its separate snapshot path. Licensed §5.8 remains unreviewed; no conformance or broad profile-parity finding is claimed |
| Additive per-attribute mode changes | [Additive subscription test:355](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L355) | Adding B preserves A; per-attribute active/passive replacement changes projection without discarding other entries |
| Empty regional set and rate-generation caveat | [Focused regional assertions:244](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L244), [test-author §9.8.4 comment:253](../../cpp/tests/passive_object_attribute_subscription_catch2.cpp#L253), [per-federate generation:2335](../../cpp/src/internal/federation/federation_registry.hpp#L2335), [generation update:524](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L524), [generation in admission key:96](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L96), [first-use gate:19](../../cpp/src/internal/time/update_rate_gate.cpp#L19), [generation-key unit case:40](../../cpp/tests/ieee1516_2025_update_rate_gate_unit_catch2.cpp#L40) | Test source attributes the empty-region expectation to §9.8.4; this is not an independent normative check. The unit case verifies fresh history for a changed generation key; no public integration test isolates the empty-region call's effect |

The ordinary and regional declaration diagrams and their focused tests are
2025 source/test evidence. The late-subscription model distinguishes the
embedded post-discovery callback planner, the process service-stage ordinary
initial-value planner, and the RTI-owned MOM snapshot branch. The new process
test source asserts the ordinary selected-value case but this documentation
pass did not execute it; the already-known-object/value-only case and normative
§5.8 rule remain unresolved. This is not a broad process parity claim, a
conformance finding, or 2010 behavior. For service-report projection, see [service invocation
reporting](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md); for class-level
declaration callbacks, see [relevance advisories](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md).
