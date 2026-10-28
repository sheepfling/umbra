# IEEE 1516.1-2025 Object and Interaction Information Flow

IEEE 1516.1-2025 distinguishes updates to attributes of registered object
instances from interaction messages sent with class parameters. Understanding
either path requires separate questions about producer authority, recipient
interest and eligibility, and application observation; object delivery also
depends on the receiver's instance knowledge. Publication, ownership, DDM,
time, and callback rules apply at different boundaries, not as one universal
gate. This guide explains that HLA model first and uses selected Umbra 2025
paths and tests as bounded examples—not as the definition of HLA behavior or a
claim of complete support or conformance.

## Scope and authority

- **Edition boundary:** IEEE 1516.1-2025 only. The 2010 API/runtime and its
  tests are a separate compatibility stream; this guide neither describes nor
  compares them.
- **Implementation boundary:** the current embedded federation-management
  development profile and selected 2025 process-endpoint integration cases.
  These are evidence for specific paths, not every transport or deployment.
- **Normative authority:** the [official IEEE 1516.1-2025 Federate Interface
  Specification](https://standards.ieee.org/ieee/1516.1/6688/). Check exact
  service and callback clauses there before turning this guide into a
  requirement or a conformance claim. The guide intentionally avoids inventing
  clause mappings.
- **Evidence rule:** source links describe current implementation decisions;
  test links show only the scenario each test exercises. Internal tests are
  implementation evidence, not public conformance evidence.

For the state machines that constrain delivery, see the [2025 time-management
guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md), [DDM and regions
guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md), and [attribute
ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md). This guide marks
their boundaries instead of folding their rules into one oversized diagram.

## Start with five distinct questions

“Can this value get to that federate?” is not one yes/no state. Work through
these independently, and apply only the questions relevant to the service:

1. **Is the producer's required declaration present?** Publication is the
   sender-side declaration for the relevant object attribute or interaction
   class. A directed send instead uses its object-class/interaction-class
   declaration pair; it is not ordinary interaction publication.
2. **If this is an object-attribute update, does the sender own each instance
   attribute?** Publication and per-instance ownership are distinct. This
   ownership question does not apply to an interaction send.
3. **Which receivers qualify under this service's rules?** Ordinary
   subscriptions and class matching, directed target selectors, and DDM are
   distinct service-specific ways to determine candidate receivers; they are
   not one interchangeable subscription test.
4. **Which participants must know an object instance?** Object services track
   instance knowledge per federate: producer knowledge is not receiver
   discovery. A directed send requires the sender and an eligible receiver to
   know its target. An ordinary interaction has no object-instance knowledge.
5. **When can the application observe the event?** A planned or accepted
   service, queued RTI work, callback dispatch, and callback-visible application
   state are distinct boundaries. Callback model and time management affect
   when the final observation can occur.

Message order, transportation, and timestamp eligibility are additional
service properties, not substitutes for these questions. The cross-cutting
[delivery and observation checklist](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#keep-delivery-and-observation-questions-separate)
and the topic guides below trace those boundaries without imposing one fixed
delivery sequence.

For a cross-service checklist that separates state changes, pending effects,
caller outcomes, reports, and application observation, see the
[cross-cutting state/event/observation guide](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md).
For the callback-model gates that control when application code runs, see the
[callback and service-ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md).
Those companions keep their own service and implementation-profile limits;
this overview does not treat any single delivery path as universal.

## Encoded values enter services as handle-keyed octets

For an application using the standard 2025 `DataElement` API, the FOM member's
declared data type guides the application's choice of a compatible element
format; the type name itself is not carried in the service map.
`DataElement::encode()` produces `VariableLengthData`, as declared by the
[pinned 2025 `DataElement` interface](../../third_party/ieee1516.1-2025/include/RTI/encoding/DataElement.h#L33).
The RTI service API then carries those values in a map keyed by typed attribute
or parameter handles. The outer identity differs too: object update names an
`ObjectInstanceHandle`, while ordinary `sendInteraction` names an
`InteractionClassHandle` and has no object target. This connects the
value-format layer to the service layer without turning either the FOM data
type or the encoded bytes into a routing identity.

```mermaid
flowchart LR
  A["FOM member declares a data type"] --> B["Producer selects a compatible<br/>DataElement"]
  B --> C["Encode value as<br/>VariableLengthData"]
  C --> D{"Which member<br/>is supplied?"}
  D -->|object attribute| E["AttributeHandleValueMap<br/>AttributeHandle → VariableLengthData<br/>for an ObjectInstanceHandle"]
  D -->|interaction parameter| F["ParameterHandleValueMap<br/>ParameterHandle → VariableLengthData<br/>for an InteractionClassHandle"]
  E --> G["updateAttributeValues<br/>updates the named object instance"]
  G --> I["reflectAttributeValues<br/>object instance + attribute/value map"]
  I --> K["Receiver decodes with a compatible DataElement"]
  F --> H["sendInteraction<br/>sends a class-scoped event<br/>no object target"]
  H --> J["receiveInteraction<br/>interaction class + parameter/value map"]
  J --> K
```

The public [value-map typedefs](../../third_party/ieee1516.1-2025/include/RTI/Typedefs.h#L35)
pair each typed handle with `VariableLengthData`. The [object update
signature](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L727)
and [interaction send signature](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L761),
along with the [reflection callback](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L238)
and [interaction callback](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L264),
carry the corresponding maps. A key identifies the typed API member; it is
separate from the value's `DataElement` encoding and from the handle's own
resolution/identity path (see the [typed-handle guide](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md#5-typed-handle-encoding-is-not-numeric-identity)).
The contrasting outer identities are part of these service and callback
signatures; the ordinary interaction path has no object-instance discovery or
attribute-ownership gate. Directed interaction is deliberately separate: it
adds a target object while retaining an interaction callback and its own
selector rules (see the [directed-interaction guide](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md)).
The map is a C++ API container, not a prescribed single serialized payload,
and `userSuppliedTag` is a separate argument, not a value-map entry. This flow
stops at the public API/callback boundary: it does not specify private Umbra
process framing. Element-specific byte order and composite rules, and the
separate process-frame boundary, stay in the [DataElement guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md#pick-the-representation-boundary-before-reasoning-about-bytes);
publication, ownership, recipient eligibility, and observation stay in the
service flows below and their linked guides.

The evidence is layered, not one end-to-end proof: the [basic-element unit
case](../../cpp/tests/ieee1516_2025_basic_data_elements_catch2.cpp#L86) checks
encoded bytes and decode behavior; the [embedded interaction callback
case](../../cpp/tests/receive_order_interaction_catch2.cpp#L97) observes its
parameter handle/value map and separate tag in `receiveInteraction`; and the
[configured-process attribute-reflection case](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457)
checks attribute-map bytes and tag at the callback. The [process Send
Interaction case](../../cpp/tests/ieee1516_2025_connection_send_interaction_catch2.cpp#L457)
instead checks the pulled event's parameter envelope, not a public callback.
These different scenario/profile layers do not prove every data type or
end-to-end embedded/process parity.

### Callback payload references: decode now or copy to retain

The 2025 [`reflectAttributeValues`](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L238)
and [`receiveInteraction`](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L264)
callbacks receive their handle/value maps and user tag through `const&`
parameters. The pinned [`VariableLengthData` copy constructor](../../third_party/ieee1516.1-2025/include/RTI/VariableLengthData.h#L38)
allows an application-owned value, while its [`data()` lifetime note](../../third_party/ieee1516.1-2025/include/RTI/VariableLengthData.h#L50)
warns that a pointer must not be expected to remain valid past the value
object's lifetime or its next data replacement. A safe federate-side rule is
therefore to decode through
the callback arguments while the callback is active, or copy the needed map,
tag, or octets into application-owned storage before returning. Do not retain
a pointer, reference, `span`, or `string_view` into callback-owned data for
later use. This is a C++ lifetime rule derived from the 2025 API surface, not
a claim about Umbra's private wire format or the 2010 callback implementation.

```mermaid
flowchart TD
  CALL["RTI enters reflectAttributeValues or receiveInteraction"] --> BORROW["Callback receives const-reference value map and tag"]
  BORROW --> LATER{"Need these bytes after callback return?"}
  LATER -->|No| NOW["Inspect or decode now"]
  NOW --> RETURN_NOW["Return; discard borrowed views"]
  LATER -->|Yes| COPY["Copy needed map, tag, or octets into application-owned storage"]
  COPY --> RETURN_COPY["Return from callback"]
  RETURN_COPY --> SAVED["Use the owned copy later"]
```

The API's copyable `VariableLengthData` value is one option; copying only the
required bytes is another. A view such as `std::span` is useful for immediate
decoding, but it does not extend the lifetime of the underlying bytes. The
[DataElement guide](HLA-2025-DATA-ELEMENT-ENCODING-FLOW-GUIDE.md) covers how to
decode those octets; this section covers when the receiver must own them.

The focused embedded [receive-order interaction case](../../cpp/tests/receive_order_interaction_catch2.cpp#L53)
copies the parameter map and tag inside `receiveInteraction`, then checks the
saved values after callback draining ([assertions](../../cpp/tests/receive_order_interaction_catch2.cpp#L232)).
That demonstrates a safe test pattern, not a test of every RTI's callback
storage lifetime or a claim about process-profile implementation behavior.

An interaction is not an object update: it has no object-instance knowledge
or attribute ownership transition. Both may share subscription, region, and
time-management machinery, but keep their lifecycle diagrams separate.

For the RTI-initiated notifications those declaration changes can produce, see
the focused [2025 relevance-advisory guide](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md).
Directed interactions use a different recipient rule because a send names a
target object instance and receivers can subscribe universally or by owning a
target attribute; see the focused [2025 directed-interaction
guide](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md).

### Same payload family, different routing question

Use this orientation map to choose the right detailed flow. It summarizes the
current 2025 API and reviewed implementation paths; the two guides below keep
their separate state machines and evidence boundaries.

| Routing question | Ordinary `sendInteraction` | Directed `sendDirectedInteraction` |
| --- | --- | --- |
| What does the send identify? | Interaction class, parameters, and tag; no object instance. | Interaction class plus target object, parameters, and tag; target is context, not recipient list. |
| What does the producer publish? | The interaction class. | An object-class / interaction-class pair for the target class; producer must know the target. |
| How does a receiver qualify? | Active non-sender subscription establishes relevance; receiver needs its own active/passive class match, plus overlap for regional forms. | Receiver must know target and match a directed pair; selector is universal or by-ownership. No ordinary global relevance pass. |
| What context reaches the callback? | `receiveInteraction` with visible parameters, but no target handle. | `receiveDirectedInteraction` includes target handle; still an interaction callback, not an attribute reflection. |
| Which interaction class and parameters reach the callback? | The reviewed ordinary receive-order resolver can promote to the closest eligible subscribed class and project parameters through that class; see the [class-ancestry map](#class-ancestry-has-different-jobs-in-different-paths). | The current directed receive-order candidate retains the sent interaction class. Target-object ancestry selects declaration and receiver policy, not a promoted callback class; parameters are validated against the sent class's namespace. See the directed guide's [parameter test](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#what-the-surveyed-tests-demonstrate) and [source map](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#source-map-and-known-limits). |

In particular, ordinary `active` and directed `universally` are not synonyms:
the first participates in class-level relevance, while the second selects a
directed receiver predicate for a known target. The target's owner need not be
the directed sender, and the sender is not a receiving candidate. See the
ordinary [interaction relevance/projection flow](#interaction-relevance-class-promotion-and-parameter-projection)
and [directed declaration/selector flow](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#the-declaration-matrix-and-receiver-selector)
for the full transitions, focused test evidence, and implementation limits.
Both remain interaction services: target object context does not make a
directed send an attribute update or transfer ownership. Callback-model and
timestamped-delivery gates are separate concerns covered by the callback and
time-management guides.

### Class ancestry has different jobs in different paths

The composed FOM carries separate object-class and interaction-class parent
chains. The [FOM composition guide](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#composition-merge-first-resolve-the-completed-model-second)
covers module merging. This map compares selected current 2025 lookup paths:
most are traced through the embedded runtime, while the directed branch is
also traced through the process endpoint to its shared registry planner.
Delivery fences remain profile-specific. These are source-observed behaviors,
not a complete normative inheritance rule; check the licensed IEEE text for
exact requirements.

```mermaid
flowchart TD
  FOM["Composed 2025 FOM<br/>separate object-class and interaction-class chains"]
  FOM --> ObjectTree["Object-class ancestry"]
  FOM --> InteractionTree["Interaction-class ancestry"]
  ObjectTree --> Discovery["Object discovery<br/>registered class walks toward parents<br/>closest eligible subscription sets receiver's known class"]
  ObjectTree --> AttributeValues["Attribute reflection<br/>receiver's known class limits visible class attributes"]
  InteractionTree --> OrdinaryInteraction["Ordinary interaction<br/>sent class walks toward parents<br/>closest eligible subscription sets callback class and parameter projection"]
  ObjectTree -->|target-class lookup| DirectedInteraction["Directed interaction<br/>applicable pair and receiver selector"]
  InteractionTree -->|sent-class parameter namespace| DirectedInteraction
  DirectedInteraction --> DirectedCallback["Current path retains sent interaction class<br/>callback also carries target context"]
```

This is not one reusable “nearest ancestor” rule. Discovery changes which
object class the receiver knows; attribute reflection uses that per-receiver
class to filter values. Ordinary interaction matching can promote the received
interaction class and then project parameters for it. Directed routing uses
the target object's class ancestry for declaration and selector lookup; the
sent interaction class still supplies the parameter namespace, but the current
candidate path keeps that sent class in its callback rather than promoting it.

Evidence and limits for these branches:

- The composer records parent names separately for object and interaction
  classes ([object classes](../../cpp/src/internal/fom/libxml2_fom_composer.cpp#L1848),
  [interaction classes](../../cpp/src/internal/fom/libxml2_fom_composer.cpp#L1914)).
- Object discovery's closest-class selection is in the [registry resolver](../../cpp/src/internal/federation/federation_registry.cpp#L830);
  the [focused discovery case](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L297)
  registers a child-class object while subscribing at the base class, then
  observes the base class as known and only its base attribute in the initial
  reflection (test assertions at lines 391–395). This is focused embedded
  behavior, not a normative inheritance statement. Attribute filtering through
  each receiver's known class is in the [update recipient planner](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L49)
  and the [embedded projection scenario](../../cpp/tests/ieee1516_2025_receive_order_attribute_update_passel_lifecycle_catch2.cpp#L5).
- Regional object DDM adds a separate source-level ancestry path: available
  dimensions include parent associations, regional subscriptions remain keyed
  by exact class and attribute, and discovery walks from the registered class
  toward parents before later reflection uses the receiver's known class. The
  [DDM guide's object ancestry map](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#object-class-ancestry-regional-discovery-and-value-projection)
  marks the parent/child regional consequence as source-only because reviewed
  regional tests register and subscribe at the same class; this is not a
  normative or 2010 inheritance claim.
- Ordinary interaction promotion and parameter visibility are in the [recipient resolver](../../cpp/src/internal/federation/federation_registry.cpp#L1421)
  and the [registry planning case](../../cpp/tests/federation_registry_composed_fom_catch2.cpp#L1209).
  The internal case checks exact-class delivery plus promotion to two ancestors;
  parameters declared only on the sent child class are absent from both
  promoted projections (assertions at lines 1353–1376). This is registry-unit
  evidence, not public conformance evidence.
- The separate interaction-class × regional-DDM path distinguishes parent-chain
  dimension availability from exact-class declaration storage and the later
  sent-class-to-parent recipient walk; the [DDM guide's interaction ancestry
  map](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#interaction-class-ancestry-available-context-and-declaration-lookup)
  labels this as embedded-source behavior because the reviewed tests do not
  combine parent and child classes across those regional services.
- Directed target validity and pair/selector ancestry are visible in the [shared registry directed resolver](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L15),
  its [pair lookup](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L48),
  and [selector lookup](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L97);
  the same candidate keeps the sent interaction class at [callback projection](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L245).
  The [embedded known-target case](../../cpp/tests/directed_interaction_known_target_catch2.cpp#L121)
  declares and subscribes at the target's exact class and confirms the callback
  retains the sent interaction class; it does not exercise ancestor-class
  declarations or selectors.
  The [2025 process send handler](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L657)
  calls that same registry planner, so these source-level target-class ancestry
  lookups also underlie the surveyed process route. The [process multi-recipient
  case](../../cpp/tests/ieee1516_2025_connection_directed_interaction_multi_recipient_catch2.cpp#L457)
  uses exact-class declarations; it does not test ancestor lookup. Process
  event delivery and its distinct callback boundary are detailed in the
  [directed-interaction guide](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#2025-receive-order-delivery-has-a-profile-boundary).
  The [known-target integration case](../../cpp/tests/directed_interaction_known_target_catch2.cpp#L121)
  exercises target-aware routing, but a bounded search found no focused case
  where a declaration on an ancestor object class is applied to a child target.
  Treat that particular inheritance consequence as source-only; no 2010
  equivalence or normative inheritance interpretation is asserted.

Dimension availability is a separate inheritance question from declaration
lookup. The DDM guide's [child-class regional case](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#inherited-dimensions-in-a-child-class-regional-path)
shows one embedded object path where dimensions associated along the class
parent chain are used by a regional registration and subscription on the
child class; it does not claim the subscription declaration itself is
inherited.

## Object path: declarations, registration, and discovery

The object publisher declares which class attributes it may publish. A
different federate subscribes to attributes of an object class. Registration
creates an instance and initializes the producer's own knowledge; the RTI then
plans discovery for other eligible federates. A later callback-visible
discoverObjectInstance is the receiver's introduction to that instance.

The lifetime of an already-queued discovery callback when its recipient
resigns is a separate callback-dispatch question. See the source-bounded
[2025 process discovery boundary](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#object-instance-discovery-event-specific-staleness-boundary);
it does not change the object-knowledge model here or establish a portable
post-Resign callback rule.

When registration supplies an explicit name, see the focused [2025
object-instance name reservation guide](HLA-2025-OBJECT-INSTANCE-NAME-RESERVATION-FLOW-GUIDE.md)
for the prior reservation callback, ownership, and consumption-at-commit
transitions. The overview below intentionally leaves those details out.

```mermaid
sequenceDiagram
  autonumber
  actor Producer as Producer application
  participant PRTI as Producer RTI
  participant Registry as Federation state
  participant RRTI as Receiver RTI
  actor Receiver as Receiver application

  Producer->>PRTI: publishObjectClassAttributes(class, attributes)
  PRTI->>Registry: record publication
  Receiver->>RRTI: subscribeObjectClassAttributes(class, attributes)
  RRTI->>Registry: record subscription
  Producer->>PRTI: registerObjectInstance(class[, name])
  PRTI->>Registry: validate membership, class, and published attributes
  Registry->>Registry: create instance, record producer ownership and knowledge
  Registry->>Registry: evaluate other members for discovery relevance
  alt receiver is currently eligible
    Registry-->>RRTI: plan discovery callback
    RRTI-->>Receiver: discoverObjectInstance(handle, received class, name, producer)
    Note over Registry,Receiver: Receiver-side known-object state is distinct from producer registration
  else no eligible receiver yet
    Note over Registry,Receiver: No discovery callback is established by this registration alone
    opt a later declaration change makes discovery eligible
      Registry->>RRTI: re-evaluate pending discovery
      RRTI-->>Receiver: discoverObjectInstance(...)
    end
  end
```

### Reading the object diagram correctly

- Publication is not a broadcast and does not itself create an instance.
- Subscription is not discovery. Discovery concerns a registered instance
  and is tracked per receiver.
- A registered object is initially known to its producer. The current
  discovery resolver excludes the producer from an induced discovery callback
  and derives the receiver's exposed class from the registered class and
  matching declarations.
- In the current implementation, discovery planning checks active
  registration relevance separately from the receiving federate's own
  subscription match. A passive subscription does not establish relevance by
  itself, although it can receive discovery made relevant by another active
  declaration. Treat this as a source-backed implementation nuance, not a
  substitute for checking the standard's exact rule.
- Current source records pending discovery separately from known-object state.
  The discovery transition rechecks eligibility and records the receiver's
  known class as callback dispatch begins. Do not collapse “planned” and
  “already known” into the same state.
- The internal commit point is profile-specific in the selected 2025 source:
  embedded discovery begins the known-object transition when the callback task
  runs, before calling `discoverObjectInstance`; the process service begins the
  registry transition before exposing its event to the transport, with the
  client callback bridge later. See the [subscription-state guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md)
  and [process callback guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md).
  The focused public process case asserts eventual discovery under both
  callback models, not use of receiver-known state before callback delivery.
  This source comparison is neither a portable HLA ordering rule nor a parity
  claim.
- For the embedded 2025 callback boundary, the RTI commits receiver-known
  state and releases the federation-state lock before entering
  `discoverObjectInstance`; the source explicitly says matching support
  services are safe to call reentrantly from that callback
  ([discovery transition](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L266)).
  For example, `getKnownObjectClassHandle` reads the committed entry, and the
  pinned API exposes it as a public support service (§10.6)
  ([API declaration](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1729),
  [implementation](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L2676)).
  The focused [ownership-query case](../../cpp/tests/attribute_ownership_query_catch2.cpp#L312)
  calls it only after discovery has been evoked and recorded; no reviewed
  focused test calls it from inside `discoverObjectInstance`. Treat
  reentrant usability as source-supported Umbra behavior, not independently
  test-verified or a normative callback-reentry conclusion: the pinned API
  header contains signatures and clause labels, not the applicable prose.
- The cross-path [class ancestry map](#class-ancestry-has-different-jobs-in-different-paths)
  gives selected hierarchy effects, but does not expand the full object-class
  declaration lattice or every discovery interaction with active/passive
  declarations. See the DDM guide for region-bound behavior.

### Object existence, receiver knowledge, and DDM eligibility are separate states

One object can exist in the federation while a particular receiver does not know
it; a receiver that does know it can still be outside the current value-routing
scope. This embedded 2025 source map keeps those ledgers separate. Regional DDM
can participate in discovery and is checked again for later value routing, but
the inspected receive-order delete planner selects removal recipients from the
known-object ledger rather than retesting current region overlap. This is a
source-level explanation, not a normative claim about every conforming RTI.

```mermaid
flowchart TD
  Register["Register object instance<br/>create federation object record"] --> Candidate{"For this receiver:<br/>does discovery evaluation find an eligible declaration?<br/>If regional, does the current region path qualify?"}
  Candidate -->|No| ExistsUnknown["Object exists;<br/>receiver remains unknown"]
  Candidate -->|Yes| Pending["Discovery is pending"]
  Pending --> Commit{"Receiver-specific discovery<br/>boundary still valid?"}
  Commit -->|No| NoCommit["No known-object transition"]
  Commit -->|Yes| Known["Receiver knows object<br/>at the selected class"]
  Known --> Update{"For a later attribute update:<br/>does this class/subscription path qualify now?<br/>If regional, do current regions overlap?"}
  Update -->|Yes| Value["Value delivery may be planned"]
  Update -->|No| OutOfScope["No value through this path;<br/>receiver-known state remains"]
  Value --> Known
  OutOfScope --> Known
  Register --> Delete["Object deletion is accepted"]
  Delete --> Recipients["Plan removal for other joined,<br/>known receivers with usable routes;<br/>current DDM overlap is not retested here"]
  Recipients --> RemovalGate{"At this receiver's removal boundary:<br/>reservation, membership, and knowledge valid?"}
  RemovalGate -->|No| Suppress["Suppress stale removal callback"]
  RemovalGate -->|Yes| Forget["Clear receiver-known state"]
  Forget --> Remove["Deliver removeObjectInstance"]
```

The standard-facing API exposes these as distinct services—registration
(including [`registerObjectInstanceWithRegions` §9.5](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1487)),
regional subscription (§9.8), delete request (§6.16), and removal callback
(§6.17). The pinned headers establish those API roles, not the detailed
eligibility rules drawn above.

Evidence for the embedded 2025 path: the [discovery candidate resolver](../../cpp/src/internal/federation/federation_registry.cpp#L830)
uses object existence, per-receiver declarations, and current source/receiver
region realizations; [discovery planning](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp#L13)
tracks pending recipients separately from the known-class map. The [value
recipient planner](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L49)
first requires receiver knowledge and then evaluates current declarations and
regional relevance. The [delete planner](../../cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp#L54)
selects other joined, known receivers with callback routes, while the
[removal boundary](../../cpp/src/internal/federation/federation_registry_object_instance_removal.cpp#L49)
clears receiver knowledge before returning the callback snapshot.

The focused [three-dimensional regional case](../../cpp/tests/three_dimensional_regional_object_attribute_overlap_catch2.cpp#L110)
observes initial discovery, suppresses later reflections after committed
disjoint ranges, and observes reflection again after overlap is restored. The
separate [embedded delete-service case](../../cpp/tests/delete_object_instance_service_report_catch2.cpp#L194)
observes ordinary discovery followed by removal. Neither combines a previously
discovered object, a later disjoint-region commit, and deletion while disjoint;
the “known receiver still receives removal” relation is therefore source-only.
This chart does not establish configured-process equivalence or 2010 behavior.

### Object-class attribute availability is not declaration inheritance

Keep three questions separate: which attributes are available on an FOM class;
which class was named by a publication or subscription; and which subscribed
class the receiver is told it knows. The pinned 2025 API gives both publication
and subscription an explicit object-class handle and attribute set (§5.2 and
§5.8), but its headers provide API shape and clause labels rather than the
standard's normative prose. In the reviewed 2025 embedded source, declaration
state is keyed by the class handle supplied to the service, registration checks
publication for the registered class, and attribute validity is resolved
against the supplied class (including attributes available through its FOM
ancestry).

The focused active integration case makes the distinction concrete: the
publisher declares both a base-class attribute and a child-class attribute at
the child class, then registers a child instance. A receiver subscribing at
the base class is discovered as knowing the base class and sees only the base
attribute; trying to subscribe to the child-only attribute at the base class
is rejected. This is an Umbra 2025 embedded observation, not a complete
normative inheritance rule. It does not cover passive relevance, regional
matching, every overlapping declaration arrangement, or process callback
delivery. The separate [subscription-state guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md)
covers the active/passive distinction; no 2010 equivalence is inferred.

Evidence: the official 2025 [publication](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L429)
and [subscription](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L535) signatures;
per-class [publication](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L209)
and [subscription](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L350)
storage and [registration lookup](../../cpp/src/internal/federation/federation_registry_object_instance_registration.cpp#L216);
[class-relative attribute validation](../../cpp/src/internal/federation/federation_registry_catalog_queries.cpp#L687);
the discovery [relevance pass](../../cpp/src/internal/federation/federation_registry.cpp#L864)
and [nearest-class search](../../cpp/src/internal/federation/federation_registry.cpp#L979);
and the [focused base/child integration case](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L297).

### First reflection after discovery: order and evidence boundary

Specifically in the 2025 process pull/`receive_interaction` path, the selector
chooses pending discovery before the attribute-reflection event slot for a
newly registered object, while retaining separate priorities for other event
families. This is not a global callback order or a rule for the pushed stream.
The source comment says this selection matches the official callback ordering
([selector](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L73)).
That is concrete process implementation evidence and an explicit source
interpretation of the standard. The pinned [FederateAmbassador interface](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L229)
labels `discoverObjectInstance` as §6.9 and `reflectAttributeValues` as §6.11,
but supplies declarations, not the normative clause text for independently
checking the ordering statement. The official downloadable API archive also
contains only API sources rather than the standard clauses; see the
[normative source-access audit](HLA-BEHAVIOR-FLOW-GUIDES.md#normative-source-access--2026-10-09).

The focused [public process update case](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457)
registers an object, sends a distinct `Update Attribute Values` call, and
checks that both callbacks occurred ([explicit update](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L801),
[final assertions](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L815)).
That later update is distinct from the now-separate configured-process
late-subscription case: the receiver subscribes after the sender stores values
and the test source asserts discovery plus a reflection containing only the
selected `Name` bytes ([test](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L652),
[assertions](../../cpp/tests/ieee1516_2025_connection_public_object_subscription_catch2.cpp#L952)).
The current process enqueue source commits known-object state, plans current
eligible ordinary values, and stages discovery and value events separately
([process enqueue](../../cpp/src/internal/federation/process_federation_service_object_discovery.cpp#L90)).
The test has HLA_EVOKED and HLA_IMMEDIATE sections; this documentation pass
did not run it, and the process implementation file is modified in the current
worktree. The exact licensed §5.8 rule remains unverified, so these source and
test assertions are not a normative or conformance conclusion.

The selected embedded 2025 path has a distinct ordinary-object initial-state
step: it enters `discoverObjectInstance`, then plans and invokes a reflection
for currently eligible subscribed values ([discovery callback](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L322),
[initial-reflection plan](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L384)).
The focused [late-subscription test](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180)
stores publisher values before the receiver subscribes, then observes one
discovery and one subscribed-value reflection. Its separate callback
collections prove occurrence and value, not relative callback order. Keep this
embedded callback-after-discovery path separate from the process service-stage
initial-value plan; both are distinct from the RTI-owned MOM snapshot path.
The process test source asserts discovery and selected-value reflection under
both callback-model sections, but was not run in this documentation pass and
does not assert a general cross-model callback order. Neither selected
source/test evidence nor the process ordering comment independently verifies
the exact normative §5.8/§6.9/§6.11 rules.

For a visual comparison of the embedded callback-after-discovery path, the
process ordinary discovery/value path, and the separate MOM branch, see the subscription-state guide's
[late-subscription profile chart](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md#process-late-subscription-path-same-selectors-an-unresolved-initial-value-edge).
That companion chart distinguishes the ordinary newly-discovered case from
the still-open value-only effect of adding an attribute to an already-known
object; it does not establish the licensed normative rule.

To distinguish stored-value reflection from discovery-triggered provider
solicitation and a federate-issued request, see the Auto Provide guide's
[neighboring-path comparison](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md#keep-these-neighboring-paths-distinct).
That guide scopes verified discovery-triggered planning to the embedded
profile and explicitly leaves process-profile Auto Provide unverified.

Callback-model timing is a separate question from this discovery/value
sequence. The focused late-subscription case connects both ambassadors with
`HLA_EVOKED` and calls its `drainCallbacks` helper only after
`subscribeObjectClassAttributes` returns ([case](../../cpp/tests/subscribe_object_class_attributes_discovery_catch2.cpp#L180));
it does not include an `HLA_IMMEDIATE` variant or assert relative callback
order. The [callback guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md)
explains the general model distinction. In the selected Umbra implementation,
an immediate submission may drain on the submitting thread, while a submission
from another thread can queue behind an already-running immediate drainer
([dispatcher](../../cpp/src/internal/callbacks/callback_dispatcher.cpp#L122)).
That source detail and the HLA_EVOKED-only test do not establish a
service-specific before-return guarantee for every Subscribe Object Class
Attributes call.

The process update test's recorder stores separate booleans rather than an
ordered callback trace, so it does not independently assert
discovery-before-reflection. Keep the synthetic subscription chart's callback
placement as scenario chronology; do not use it, or either profile's internal
state-commit point, as the normative source for a general callback-order rule.
The exact 2025 §6.9/§6.11 clause wording remains an open source check rather
than a claim this guide has independently verified.

Discovery can also trigger the separate RTI-driven Auto Provide path: current
owners may be solicited for the newly known receiver's in-scope attributes.
That callback has an empty tag in the current automatic path, does not itself
deliver values, and is distinct from the explicit request flow below. See the
[2025 Auto Provide guide](HLA-2025-AUTO-PROVIDE-FLOW-GUIDE.md) for switch,
regional, fan-out, and stale-work details.

## Publication withdrawal is also an ownership boundary

Publication is a member's declaration of which object-class attributes it may
update; it is not ownership of every matching instance attribute. The 2025 API
offers both selected-attribute withdrawal and a whole-class form. In the
current embedded profile, the whole-class form removes the publication set for
that exact class. A publication recorded for an inherited attribute at a
different class remains until that class's declaration is withdrawn; the
focused class-hierarchy case demonstrates this distinction.

For object attributes, successful unpublication has a second consequence in
the embedded registry: the unpublishing member loses ownership of its matching
instance attributes on objects registered at that class or below it. Before
changing declarations, the registry checks whether any removed publication is
still required by a pending ownership acquisition. The implementation comment
maps this guard and the ownership-removal rule to IEEE 1516.1-2025 §5.3.3; the
normative text remains authoritative for exact interpretation.

```mermaid
flowchart TD
  CALL["Request publication withdrawal<br/>class plus selected attributes or whole class"] --> FORM{"Which declaration set?"}
  FORM -->|Selected set| SELECT["Target only the named attributes at this class"]
  FORM -->|Whole-class form| ALL["Use the currently published attributes at this exact class"]
  SELECT --> VALIDATE["Validate member, class, and attribute handles"]
  ALL --> VALIDATE
  VALIDATE --> PENDING{"Would removing any publication conflict with a pending acquisition?"}
  PENDING -->|Yes| REJECT["Reject with OwnershipAcquisitionPending<br/>Keep publication unchanged"]
  PENDING -->|No| COMMIT["Commit this member's publication withdrawal"]
  COMMIT --> INSTANCES["Visit instances registered at this class or below"]
  INSTANCES --> OWNER{"Does this member own a matching instance attribute?"}
  OWNER -->|Yes| REMOVE["Remove this member as attribute owner"]
  OWNER -->|No| NEXT["Continue without an ownership transition"]
  REMOVE --> METADATA["Clear this profile's owner-side update metadata"]
  METADATA --> NEXT
  NEXT --> MORE{"More matching instances or attributes?"}
  MORE -->|Yes| INSTANCES
  MORE -->|No| ADVISORIES["Refresh region-use state and plan declaration advisories"]
```

The preflight rejection happens before the declaration set changes, so a
pending-acquisition conflict does not partially withdraw a multi-attribute
request in this implementation. The focused test exercises a pending
acquisition and observes `OwnershipAcquisitionPending`; the broader declaration
case exercises whole-class withdrawal at a subclass while a separate
superclass publication remains, then withdraws the superclass publication.
The registry source also removes the unpublishing member's owner record across
matching instances and clears its update-region association plus transportation
and order metadata. Those cleanup postconditions are source-derived here: the
cited tests do not assert each one. Although the process adapter forwards these
services, this evidence does not establish process/embedded parity or 2010
behavior. Declaration advisories are planned from the resulting state and keep
their own callback-delivery boundary; see the
[relevance-advisory guide](HLA-2025-RELEVANCE-ADVISORY-FLOW-GUIDE.md).

## Attribute path: request is not the value response

An attribute-value request asks the current provider to supply values; it does
not return the bytes as the request service's result. The provider receives a
provideAttributeValueUpdate callback carrying the requested object,
attributes, and request tag. The provider then chooses whether and when to call
updateAttributeValues. That later update has its own bytes and tag and may
produce a reflectAttributeValues callback for eligible subscribers.

```mermaid
sequenceDiagram
  autonumber
  actor Requester as Requesting federate
  participant RRTI as Requester RTI
  participant Registry as Federation state
  participant PRTI as Provider RTI
  actor Provider as Provider application

  Requester->>RRTI: requestAttributeValueUpdate(object, attributes, requestTag)
  RRTI->>Registry: resolve known object and eligible attribute providers
  Registry-->>PRTI: provideAttributeValueUpdate(object, attributes, requestTag)
  PRTI-->>Provider: provider callback
  Note over Requester,Provider: Request completion is not an attribute-value response
  Provider->>PRTI: updateAttributeValues(object, values, responseTag)
  PRTI->>Registry: validate object, attribute ownership, and delivery relevance
  Registry-->>RRTI: enqueue eligible reflection
  RRTI-->>Requester: reflectAttributeValues(object, values, responseTag)
```

The diagram is the ordinary object-instance request/update path. Class-level
expansion, regional filtering, provider fan-out, and request/response restore
cut points have their own [2025 attribute-value update request guide](HLA-2025-ATTRIBUTE-VALUE-UPDATE-REQUEST-FLOW-GUIDE.md).
The active/passive, additive declaration state behind object-attribute
subscriptions is covered in the
[2025 object-attribute subscription guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md).
Per-attribute update-rate admission is covered separately in the
[2025 update-rate guide](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md).
In particular:

- The request tag is echoed on the provider callback; it is not automatically
  reused as the later update's tag. The provider supplies that update tag.
- The request can lead to a reflection only if a provider actually sends an
  update and the update is eligible for delivery. The callback is not a return
  value from requestAttributeValueUpdate.
- Object knowledge, attribute ownership, subscription matching, and DDM are
  separate gates. Ownership changes are described in the ownership guide;
  regional relevance is described in the DDM guide.
- Object-attribute declarations also compose per selected attribute: active
  and passive entries can coexist without making every attribute eligible.
  See the [subscription-state guide](HLA-2025-OBJECT-ATTRIBUTE-SUBSCRIPTION-STATE-FLOW-GUIDE.md)
  for the declaration mutation boundary.
- Overlapping class subscriptions should be reasoned about as one receiver's
  matching state, not as a promise to invoke application code once per
  declaration. The linked process test observes one reflection for its
  overlapping-subscription scenario; broader combinations need their own
  evidence.
- A queued reflection is not necessarily callback-visible immediately.
  Callback execution policy and service re-entry belong to the
  [2025 callback/service-ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md);
  TSO eligibility and timestamp ordering belong to the
  [2025 time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md).

### Embedded receive-order update: projection, passels, and callback boundary

These diagrams describe the ordinary **2025 embedded, no-time** update path.
They begin when a provider calls `updateAttributeValues`; the request above
only solicits a future provider callback. They do not describe the timestamped
overload or the process endpoint. The three views follow actual responsibility
boundaries: caller-owned input validation, receiver-specific route planning,
and callback-time delivery.

#### 1. Validate and snapshot the sender's update

The submitted set is checked against the sender's registered object class,
ownership, and delivery metadata before caller buffers are copied. The adapter
then makes durable copies and plans again, so the state used for recipient
selection is current after the copy boundary.

```mermaid
flowchart TD
  Call["updateAttributeValues(object, values, tag)"] --> FirstPlan{"Member, known object, attributes, and metadata valid?"}
  FirstPlan -->|No| Reject["Reject before acceptance"]
  FirstPlan -->|Yes| Copy["Copy caller values and tag durably"]
  Copy --> Replan{"Re-plan after copying; still valid?"}
  Replan -->|No| Reject
  Replan -->|Yes| Split["Group attributes by effective transportation, order, and update-region association"]
  Split --> Passels["Produce planning passels for receiver selection"]
```

The plan rejects a repeated or undefined attribute and an attribute not owned
by the sender. It also resolves each attribute's effective order,
transportation, and region association. The passels are the unit carried into
recipient selection; the timestamped overload has a different subsequent
queue/grant path. In the 2025 embedded profile, a timestamped invocation can
also split attributes with different effective orders into receive-order and
TSO paths for the same constrained receiver; see the time guide's
[mixed-order passel flow](HLA-2025-TIME-MANAGEMENT-GUIDE.md#one-timestamped-invocation-can-split-by-attribute-order).
The time guide has a separate process-endpoint view, which preserves the
profile boundary and leaves its constrained receive-order timing evidence
open. Neither view establishes process/embedded parity or 2010 behavior.

#### 2. Project each passel for each receiver

```mermaid
flowchart TD
  Passels["Visit each planned passel"] --> Members["Consider each joined member except the producer"]
  Members --> Candidate{"Receiver knows the object and has a callback route?"}
  Candidate -->|No| Skip["No route for this receiver/passel"]
  Candidate -->|Yes| Project["Filter through receiver's known class, subscription, and applicable region overlap"]
  Project --> Any{"Any submitted attributes remain eligible?"}
  Any -->|No| Skip
  Any -->|Yes| Route["Plan one route for this receiver and this passel"]
  Skip --> More{"More receiver/passel pairs?"}
  Route --> More
  More -->|Yes| Members
  More -->|No| Report["Write selected successful file report, if enabled"]
  Report --> Count["Record one successful update invocation"]
  Count --> Submit["Release sender locks, then submit planned routes"]
```

The registry validates sent attributes against the registered class, then
projects separately for each receiver using that receiver's known class. Thus
a receiver that knows only a superclass does not receive an attribute
introduced below that class, even when the sender submitted it. Subscription
relevance and region overlap remain separate per-attribute gates; their full
rules are in the [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md).
The successful service report, when selected, precedes one successful-update
counter increment for the API invocation, not one per receiver. These source
ordering details are distinct from the focused test assertions below.

#### 3. Dispatch and recheck at the callback boundary

```mermaid
flowchart TD
  Submit["Submit one planned receiver/passel route"] --> Model{"Receiver callback model"}
  Model -->|HLA_EVOKED| Queue["Queue for a later evoke"]
  Model -->|HLA_IMMEDIATE| Inline["May enter receiver code synchronously"]
  Queue --> Recheck["Re-evaluate current receiver projection for this passel"]
  Inline --> Recheck
  Recheck -->|No eligible attributes| Suppress["Suppress this reflection"]
  Recheck -->|Eligible attributes remain| Transport{"Best-effort transportation?"}
  Transport -->|No| Reflect["Invoke reflectAttributeValues"]
  Transport -->|Yes| Rate["Apply per-attribute update-rate admission"]
  Rate -->|All values filtered| Suppress
  Rate -->|Values remain| Reflect
```

At callback entry the ordinary path recomputes eligibility for the submitted
passel only. A later unsubscribe can therefore suppress queued work, and a
best-effort rate gate can further remove values. The [DSE guide](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md)
owns the broader route-only candidate and late-subscription rules; the
[time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md) owns timestamped
updates and grant delivery. Region-descriptor conveyance is controlled
separately and remains in the DDM guide.

The focused embedded test exercises the user-visible distinctions: an exact
receiver gets the child attribute, a receiver that knows only the superclass
gets only the base attributes, reliable and best-effort values arrive in two
separate reflections, an `HLA_IMMEDIATE` receiver sees both before the service
returns, and a queued receiver that unsubscribes before `evokeMultipleCallbacks`
gets neither. It also checks rejection for an unknown object, an undefined
attribute, and an attribute the sender does not own. The test establishes those
scenarios; copy/re-plan ordering, the once-per-invocation counter, and lock
release before fan-out are source-derived details, not assertions in that test.
This is selected embedded 2025 evidence, not process parity or a complete
normative matrix.

### Configured-process untimed update: local validation and remote rejection differ

The pinned 2025 [`updateAttributeValues` API declaration](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L727)
lists `ObjectInstanceNotKnown`, `AttributeNotDefined`, and `AttributeNotOwned`
among its exceptions. In the configured-process source path, the public
adapter first checks connection and membership, then converts the supplied
handle wrappers to numeric values. A failed object-handle conversion is
reported locally as `ObjectInstanceNotKnown`; a failed attribute-handle
conversion is `AttributeNotDefined` ([adapter checks](../../cpp/src/internal/runtime/umbra_rti_ambassador_attribute_update_services.cpp#L82)).
Successful conversion only makes the handles transportable: the process
service still has to establish that the object is known in this execution,
the attributes belong to its registered class, and the producer owns them.

The process service invokes the shared registry planner. The planner retains
distinct statuses for unknown object, undefined attribute, and not-owned
attribute ([planner status](../../cpp/src/internal/federation/federation_registry_service_types.hpp#L1238),
[validation](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L101)).
Unlike the selected embedded adapter path, which maps those statuses to the
corresponding public exceptions ([embedded translation](../../cpp/src/internal/runtime/umbra_rti_ambassador_service_failure_translation.cpp#L409)),
the process service returns one generic `rejected` response for every
non-applied plan ([process handler](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L45),
[generic response](../../cpp/src/internal/federation/process_federation_service.cpp#L317)).
The process client turns that response into `ProcessFederationClientError`,
which the public adapter exposes as `RTIinternalError`
([client](../../cpp/src/internal/federation/process_federation_client_value_services.cpp#L108),
[adapter mapping](../../cpp/src/internal/runtime/umbra_rti_ambassador_attribute_update_services.cpp#L133)).
This is a source-derived process/embedded mapping difference, not a finding
about normative conformance or the complete API exception matrix.

```mermaid
flowchart TD
  CALL["Untimed updateAttributeValues"] --> PRE["Check connection and federation membership"]
  PRE -->|"Not connected / not a member"| PREERR["NotConnected / FederateNotExecutionMember"]
  PRE -->|"Pass"| OBJECT{"Object handle converts locally?"}
  OBJECT -->|"No"| OIN["ObjectInstanceNotKnown"]
  OBJECT -->|"Yes"| ATTR{"Attribute handles convert locally?"}
  ATTR -->|"No"| AND["AttributeNotDefined"]
  ATTR -->|"Yes"| SEND["Copy values and tag; send process request"]
  SEND --> SESSION{"Process session and member match?"}
  SESSION -->|"No"| REJECT["Generic rejected response"]
  SESSION -->|"Yes"| PLAN["Shared registry checks object, class attributes, and ownership"]
  PLAN -->|"Applied"| CONTINUE["Continue accepted-update routing"]
  PLAN -->|"Unknown object / undefined attribute / not owned / other planner failure"| REJECT
  REJECT --> CLIENT["ProcessFederationClientError"]
  CLIENT --> PUBLIC["Public adapter throws RTIinternalError"]
```

The focused [public process update case](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_catch2.cpp#L457)
uses a valid published attribute owned by the sender, asserts that the
untimed call does not throw, and observes a successful response with zero
recipients in its `HLA_EVOKED` and `HLA_IMMEDIATE` sections
([call and assertion](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_catch2.cpp#L597),
[zero-recipient result](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_catch2.cpp#L626)).
It does not test remote unknown-object, undefined-attribute, or not-owned
translation. The process failure mapping above is therefore source evidence,
not an asserted public outcome. The timestamped overload and its TSO failure
paths are separate and are not covered by this flow.

## Interaction path: class subscription, parameter projection, callback

An interaction send carries an interaction class, parameter/value pairs, and a
user tag; it does not name an object instance. The runtime validates the
sender's publication and resolves eligible receiving members from interaction
subscriptions. The callback carries the values that are visible through the
receiver's matched class plus the user tag. The sender does not receive its
own induced receiveInteraction callback in the current joined-federate path.

```mermaid
sequenceDiagram
  autonumber
  actor Sender as Sending federate
  participant SRTI as Sender RTI
  participant Registry as Federation state
  participant RRTI as Receiving RTI
  actor Receiver as Receiving federate

  Sender->>SRTI: publishInteractionClass(class)
  SRTI->>Registry: record publication
  Receiver->>RRTI: subscribeInteractionClass(class[, active])
  RRTI->>Registry: record subscription
  Sender->>SRTI: sendInteraction(class, parameters, tag)
  SRTI->>Registry: validate sender and resolve recipient subscriptions
  Registry->>Registry: match class hierarchy and delivery eligibility
  Registry->>Registry: project class/parameters visible to receiver
  alt at least one eligible receiving federate
    Registry-->>RRTI: queue receiveInteraction(class, parameters, tag)
    RRTI-->>Receiver: receiveInteraction(...)
  else no eligible receiver
    Registry-->>SRTI: service completes without a receiver callback
  end
```

The current resolver walks the sent class and its superclasses, returning the
closest eligible subscribed class for a recipient and projecting parameters
visible through that class. Its source also separates federation-wide active
subscription relevance from a recipient's own matching subscription. These
are important implementation details, but the process-endpoint integration
case linked below exercises a simpler ordinary send; do not mistake that test
for proof of every hierarchy, passive-subscription, parameter, or region
combination.

### Interaction relevance, class promotion, and parameter projection

The registry makes two related but distinct decisions for an ordinary
receive-order interaction. First it determines whether an active subscription
from a non-sending member makes the sent class relevant. Then it evaluates
each other joined member's own subscription and any required region overlap.
A passive subscription does not create relevance by itself, but it can receive
an already-relevant interaction. If a receiver matches a superclass rather
than the sent class, that closest matching class determines both the callback
class and which sent parameters are visible.

```mermaid
flowchart TD
  Send["Send class C with parameter set P"] --> Relevance{"Active subscription on C or an ancestor from a non-sending member?"}
  Relevance -->|No| DseWithoutRelevance{"Delay Subscription Evaluation enabled?"}
  DseWithoutRelevance -->|Yes| Deferred["Retain route-only candidates; defer subscription matching and projection to delivery"]
  DseWithoutRelevance -->|No| PassiveOnly["Passive declarations alone do not create an ordinary receive-order candidate"]
  Relevance -->|Yes| Receivers["Evaluate each joined receiver except the sender"]
  Receivers --> Walk["Walk this receiver's matches from C toward HLAinteractionRoot"]
  Walk --> Match{"Own subscription matches here?<br/>Regional declarations also need overlap"}
  Match -->|No class match| DseWithoutMatch{"Delay Subscription Evaluation enabled?"}
  DseWithoutMatch -->|Yes| Deferred
  DseWithoutMatch -->|No| NoCandidate["No send-time candidate for this receiver"]
  Match -->|Yes: active, or passive under established relevance| Closest["Select the first, closest eligible subscribed class"]
  Closest --> Project["Project P to parameters defined by this class or an ancestor"]
  Project --> One["Plan one callback route:<br/>received class = selected subscription class;<br/>parameters = projected P"]
```

The first relevance check is federation-wide and excludes the sending
federate. The second is recipient-specific: a passive recipient still needs a
matching declaration and, for a regional declaration, an overlapping region.
The parent walk selects the first eligible subscription and returns one plan
for that recipient; projection then drops sent parameters that are not defined
by the selected class or its ancestors. A route-only DSE candidate has no
received class or parameter projection yet; the [DSE guide](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md)
covers that later re-evaluation boundary. Its [object-state versus interaction-history
comparison](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md#object-state-is-not-an-interaction-history)
explains why DSE can make this particular already-sent interaction eligible
later, while a newly discoverable object follows a separate current-state
reflection path. That is a later eligibility check for the retained send, not
a replay of arbitrary interaction history.

The [internal 2025 registry promotion test](../../cpp/tests/federation_registry_composed_fom_catch2.cpp#L1208)
makes the projection concrete for a send of
`MainCourseServed` with three parameters:

| Receiver's subscription | Planned callback class | Parameters in this FOM's plan |
| --- | --- | --- |
| `MainCourseServed` | `MainCourseServed` | All three sent parameters |
| `FoodServed` superclass | `FoodServed` | None; those three parameters are introduced below this class |
| `CustomerTransactions` ancestor | `CustomerTransactions` | None; those three parameters are introduced below this class |

The registry promotion test excludes an unrelated class and a receiver
without a callback route, and verifies parameter projection. Its subscriptions
are on separate receiver members, so it does not test nearest-class choice for
one receiver with declarations at multiple levels. A separate public embedded
receipt test now covers that question directly: with only the base subscribed,
the callback reports the base; with base and middle subscribed, it reports the
middle; with the sent child subscribed too, it reports the child. Each send
produces exactly one callback, whose received-class handle equals the selected
candidate ([test](../../cpp/tests/inherited_interaction_receipt_catch2.cpp#L71)).
That test sends an empty parameter map, so it does not test parameter
projection through an inherited match. The registry projection test and the
public receipt test are complementary evidence, not one end-to-end test of
both behaviors. This remains embedded 2025 development-profile evidence; it
does not establish the normative hierarchy rule, process parity, or passive,
regional, and DSE combinations.

A separate embedded regional-transition test is written to exercise the
passive/relevance split: an active receiver makes the interaction relevant for
an overlapping passive receiver; removing the sole active declaration
suppresses both deliveries while the passive declaration remains; activating
that declaration restores delivery. The last recorded local run failed during
federation creation before those transition assertions, so this remains
source-/test-intent evidence rather than runtime-verified behavior.

The pinned 2025 API exposes `bool active = true` on ordinary Subscribe
Interaction ([§5.10](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L577))
and regional Subscribe Interaction With Regions
([§9.10](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1595)).
Those signatures establish the control surface, not the full
relevance/delivery semantics. The public process declaration test asserts that
`true`, `false`, and unsubscribe are reflected in registry state
([test](../../cpp/tests/ieee1516_2025_connection_interaction_declaration_catch2.cpp#L590)),
but sends no interaction and therefore does not verify process-profile
delivery. Keep the detailed rule above implementation-scoped until the full
normative clauses are checked; do not infer process/embedded or 2010 parity.

### Regional active/passive transition and the empty-set no-op

This sequence isolates the two gates behind regional interaction delivery:
an active declaration anywhere among eligible non-senders establishes class
relevance, while each recipient still needs its own matching subscription and
overlapping region. The API flag is `active`; `false` makes the retained
regional pair passive. The empty-set call below is valid after the ordinary
membership/class checks, but has no effect on that pair.

```mermaid
sequenceDiagram
  participant Pub as Publisher
  participant Registry as Embedded 2025 registry
  participant A as Receiver A
  participant B as Receiver B
  Note over Pub,B: Setup: committed publisher/receiver regions overlap, and both receivers subscribe to the same interaction class
  A->>Registry: Subscribe(regionA, active=true)
  B->>Registry: Subscribe(regionB, active=true)
  Pub->>Registry: Send interaction with regionP
  Registry-->>A: Receive callback
  Registry-->>B: Receive callback
  B->>Registry: Replace regionB pair with active=false
  Pub->>Registry: Send interaction with regionP
  Registry-->>A: Receive callback (A establishes class relevance)
  Registry-->>B: Receive callback (passive pair overlaps)
  A->>Registry: Unsubscribe(regionA)
  Pub->>Registry: Send interaction with regionP
  Note over Registry,B: B's passive pair remains, but no active declaration establishes class relevance, so no recipient callback
  B->>Registry: Subscribe(empty RegionHandleSet, active=true)
  Registry-->>B: Valid no-op, existing pair is unchanged
  Pub->>Registry: Send interaction with regionP
  Note over Registry,B: Still no active declaration, so no recipient callback
  B->>Registry: Subscribe(regionB, active=true)
  Pub->>Registry: Send interaction with regionP
  Registry-->>B: Receive callback, B now establishes relevance and overlaps
```

The focused development-profile test encodes these transitions, but the
current local run threw `Unknown exception` while creating the federation
(test line 121), before reaching any transition assertion. Treat this as a
source/test-shaped embedded 2025 explanation, not a passing runtime observation,
a callback-order guarantee, or evidence of process-endpoint/2010 parity.

### Embedded accepted-send boundary and recipient fan-out

This is the current **2025 embedded implementation** for the ordinary
receive-order overload, not a normative callback-order guarantee and not a
claim that the process endpoint has identical sequencing. It separates one
accepted `sendInteraction` invocation from the zero or more application
callbacks that may follow it.

```mermaid
flowchart TD
  Call["sendInteraction(class, parameters, tag)"] --> Preflight{"Initial plan validates membership, publication, class, and parameters?"}
  Preflight -->|No| Reject["Fail before the successful-send boundary"]
  Preflight -->|Yes| Copy["Copy caller parameter/tag bytes and build report arguments from owned copies"]
  Copy --> Recheck{"Rechecked plan and delivery routes are still valid?"}
  Recheck -->|No| Reject
  Recheck -->|Yes| Snapshot["Resolve transportation and snapshot eligible callback routes"]
  Snapshot --> Report["Apply selected successful-service report route, if enabled"]
  Report --> Count["Record one successful interaction-send count for this API invocation"]
  Count --> Unlock["Submit recipient routes with sender locks released"]
  Unlock --> Recipients{"Any planned recipient routes?"}
  Recipients -->|No| Complete["Accepted send completes without an induced Receive Interaction callback"]
  Recipients -->|Yes| Fanout["Submit each ordinary Receive Interaction route"]
  Fanout --> Policy{"Recipient callback model"}
  Policy -->|HLA_EVOKED| Queue["Callback remains queued for a later evoke"]
  Policy -->|HLA_IMMEDIATE| Immediate["Callback may enter receiver application code synchronously"]
  Queue --> RecheckReceiver["Delivery-time eligibility and parameter projection are rechecked (see DSE guide)"]
  Immediate --> RecheckReceiver
  RecheckReceiver -->|Still eligible| Deliver["Project visible parameters and invoke receiveInteraction"]
  RecheckReceiver -->|Stale work| Suppress["Suppress callback; accepted sender count remains per invocation"]
```

Read the report, counter, and callback stages as separate boundaries. The
selected report may be a `HLAreportServiceInvocation` interaction or a file
record; the [service-reporting guide](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md)
owns its routing and switch rules. In the embedded source, selected report
handling precedes `recordSuccessfulInteractionSend`, which increments once per
accepted invocation rather than once per recipient. A `HLA_IMMEDIATE` service-
report observer may therefore run during that earlier report stage; the
ordinary application Receive Interaction routes are submitted afterward. The
send path releases its sender locks before submitting those routes, since an
immediate recipient callback can synchronously re-enter user code.

With no planned recipients, the source still reaches the report and
per-invocation counter boundary, then has no application route to submit. That
zero-recipient counter detail and the immediate re-entry allowance are
source-derived; the focused tests below do not inspect the counter from inside
a callback or exercise a deliberate re-entrant send. The file-report test
does establish that the successful type-2 service record is present before an
`HLA_EVOKED` receiver enters its separately queued application callback. A
separate immediate-observer test sees the service report while that ordinary
receiver callback is still queued. Receiver-side re-evaluation and stale-work
suppression belong to the [DSE guide](HLA-2025-DELAY-SUBSCRIPTION-EVALUATION-FLOW-GUIDE.md),
not to a second copy of its state machine here.

The `HLA_IMMEDIATE`/`HLA_EVOKED` fork names the callback dispatch model; it
does not bypass the separate receive-order temporal-admission or
callbacks-enabled gates. In this embedded path, either gate can defer
application entry after the sender's accepted-send boundary. See the
[cross-cutting temporal-admission and callback-entry map](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#temporal-admission-does-not-imply-callback-entry)
for that distinct boundary.

**Boundary with other guides:** a timestamped send enters the time-management
queue and has grant/retraction/ordering constraints; a regional send adds
source-region and overlap checks. Follow the [time guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md)
and [DDM guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md) for those
branches rather than adding them to this ordinary receive-order picture.

### Embedded ordinary interaction unpublication and already-accepted work

This is a **2025 embedded source observation**, not a normative rule for every
RTI or a process-endpoint parity claim. It answers a narrow but important
question: does withdrawing an interaction publication itself fence an ordinary
receive-order callback that the RTI has already accepted and queued?

**Portable reading rule:** keep two questions separate: does the changed
declaration allow a *new* service invocation, and what happens to work that was
already accepted? Withdrawal is not automatically cancellation. The answer for
pending work is service-specific, so the directed and ordinary examples below
must not be merged into one rule.

```mermaid
sequenceDiagram
  actor Sender as Sending federate
  participant RTI as Embedded RTI
  actor Receiver as Receiving federate

  Sender->>RTI: sendInteraction(...)
  RTI->>RTI: Validate, re-plan, and accept eligible route
  RTI-->>Sender: Send returns successfully
  Sender->>RTI: unpublishInteractionClass(...)
  RTI->>RTI: Remove current sender publication
  RTI-->>Sender: Unpublish returns
  Note over RTI: The already-accepted route remains pending
  Receiver->>RTI: evokeCallback()
  RTI->>RTI: Recheck receiver eligibility and projection,<br/>not sender publication
  alt Receiver remains eligible
    RTI-->>Receiver: receiveInteraction(...)
  else Receiver is no longer eligible
    Note over RTI,Receiver: No receiveInteraction callback is invoked
  end
```

In the reviewed embedded path, publication is checked while planning the send
and again before its accepted boundary. Unpublishing removes the current
declaration, but does not remove that already-submitted ordinary callback route.
At callback dispatch, the registry re-evaluates the receiver's eligibility and
parameter projection; that predicate does not consult the sender's current
publication set. Therefore, in this path, sender unpublication alone does not
invalidate an already-accepted ordinary route. A receiver that has resigned,
unsubscribed, or otherwise become ineligible can still cause the callback to be
suppressed.

This send → unpublish → evoke ordering was not found as a focused ordinary
interaction test. Treat the behavior above as source-derived until a focused
case exercises it, and do not promote it to an IEEE requirement without checking
the normative service and callback clauses. The [directed-interaction
guide](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#receive-order-send-validate-fan-out-then-dispatch)
shows a distinct embedded path whose queued callback *does* recheck the
sender's directed publication pair; its tested invalidation behavior must not
be copied onto ordinary interactions.

### Process ordinary interactions snapshot the promoted callback view

This companion is a **2025 process-endpoint source observation** for the
ordinary, non-TSO receive-order path. Both profiles call the same registry
planner, which walks from the sent interaction class toward its parents and
selects the closest eligible subscription class. The difference is when that
class and its visible parameter set are fixed for delivery:

- In the embedded path, the queued callback closure asks the registry for a
  fresh recipient projection immediately before the callback. A current
  subscription can therefore select a different superclass projection, or no
  longer produce a callback.
- In the process path, the send handler puts the received interaction-class
  handle and projected parameter handles into a `ProcessFederationInteractionEvent`.
  If delayed subscription evaluation left a route-only candidate without a
  received class, the handler resolves it before creating the event. The later
  delivery route is configurable: pull mode enqueues the event for a later
  receive poll, while push mode sends an unsolicited event frame and does not
  also retain a duplicate in the polling queue. Both routes carry the same
  event-time class and parameter projection to the callback bridge.

```mermaid
sequenceDiagram
  autonumber
  actor Sender as Sending federate
  participant SenderRTI as Sender process RTI
  participant Service as Process send service
  participant Registry as Shared federation registry
  participant Queue as Receiver session event queue
  participant ReceiverRTI as Receiver process RTI
  participant Bridge as Process callback bridge
  participant Dispatcher as Shared callback dispatcher
  actor Receiver as Receiving federate

  Sender->>SenderRTI: sendInteraction(sent class, parameters, tag)
  SenderRTI->>Service: send_interaction request
  Service->>Registry: Plan recipients and choose each received class
  Registry-->>Service: Closest eligible class C′ + visible parameter handles P′
  opt Delayed-evaluation route has no projection yet
    Service->>Registry: Resolve recipient projection before event creation
    Registry-->>Service: Current C′ and P′, or no event for this route
  end
  Service->>Service: Materialize event(C′, P′, sent values, tag)
  Note over Service,Registry: Later declaration changes do not rewrite this event snapshot
  alt Pull mode (multi-recipient fixture: HLA_EVOKED, push disabled)
    Service->>Queue: Enqueue event
    Receiver->>ReceiverRTI: evokeCallback()
    ReceiverRTI->>Service: receive_interaction poll
    Service->>Queue: Dequeue the materialized event
    Service-->>ReceiverRTI: Event(C′, P′, values, tag)
    ReceiverRTI->>Bridge: Submit pulled event
    Bridge->>Dispatcher: Submit callback
    Dispatcher-->>Receiver: receiveInteraction(C′, project values by P′, tag)
  else Push mode (multi-recipient fixture: HLA_IMMEDIATE, push enabled)
    Service-->>ReceiverRTI: Unsolicited receive_interaction event frame
    ReceiverRTI->>ReceiverRTI: Stream reader decodes and buffers event
    ReceiverRTI->>Bridge: Dispatch buffered event
    Bridge->>Dispatcher: Submit callback
    Dispatcher-->>Receiver: receiveInteraction(C′, project values by P′, tag)
    Note over ReceiverRTI,Receiver: Test uses a later receiver getInteractionClassHandle as a fence,<br/>it does not establish callback-before-sender-send-return timing
  end
```

The earlier MainCourseServed promotion example shows the shared planner's
received-class and parameter-map results. This process trace follows that
planner output through event snapshot and delivery; it does not turn the
internal registry test into process callback evidence.

The service option selects the process transport route; it is not itself a
recipient-planning rule. The option defaults to pull mode, while the focused
configured-process multi-recipient case enables pushed events for HLA_IMMEDIATE
and leaves push disabled for HLA_EVOKED. Treat that pairing as test setup, not
as proof that every process host derives transport mode from callback model.
With push enabled, the service sends the event frame instead of keeping a
second copy for polling. With push disabled, the public Evoke path polls the
service and dequeues the queued event.

For either inspected route, the already-materialized process event carries its
class and parameter projection; the later callback path does not query the
registry again. Ordinary publish/unpublish and subscribe/unsubscribe requests
change live declarations but do not rewrite an accepted event. This is a
source-derived process observation, not a normative rule or a focused
declaration-change test. The process public Send Interaction case uses a raw
receiver to pull and inspect one exact-class event's parameter handle/value
and tag; it does not exercise class promotion or a declaration change between
send and receive. The configured-process multi-recipient test asserts actual
`receiveInteraction` callbacks for two exact-class receivers under both
callback models and preserves the two sent tags in FIFO order. Its
HLA_IMMEDIATE branch checks delivery after a later receiver-side
`getInteractionClassHandle` fence returns; it does not establish that remote
callbacks precede the sender's `sendInteraction` return. Ancestor-subscription
promotion is covered by the shared planner's internal test above, but not by a
configured-process callback test. A nonempty inherited-parameter subset and
post-send declaration changes also remain untested in that process profile.
Timestamped interaction delivery and retraction are separate paths.

For the distinct question of whether a prior ancestor subscription can match
a descendant introduced by an additional-module Join, see the [FOM guide's
interaction-catalog growth boundary](HLA-2025-FOM-MODULE-ADMISSION-AND-COMPOSITION-FLOW-GUIDE.md#interaction-catalog-growth-is-not-a-receive-interaction-callback).

Two process-path boundaries sit outside this ordinary recipient flow. First,
the predefined federation `HLAsetSwitches` interaction (including a compatible
descendant) is intercepted and consumed by the process RTI before the ordinary
recipient planner runs; it is a control request, not an ordinary application
interaction delivery. Its process switch-state and MOM-projection behavior is
covered separately in the [Federation MOM guide](HLA-2025-FEDERATION-MOM-FLOW-GUIDE.md#switch-storage-current-mom-projection-and-conditional-reflection-are-separate).
Second, push fan-out is sequential rather than an atomic service boundary in
the inspected source: the service sends recipient frames one at a time and
returns an internal-error response if the session pointer is null or a frame
send fails. No rollback of earlier submitted frames is visible. The process
client turns the failed response into a client error, which the public adapter
maps to `RTIinternalError`; therefore an error returned to the sender does not
by itself establish that no earlier recipient frame was submitted, nor that a
receiver callback ran. The pull route instead queues recipient events and does
not cross this per-recipient frame-send step. These are source observations,
not normative delivery guarantees; the reviewed process interaction tests do
not inject a failure midway through push fan-out.

Evidence: [predefined control interception](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L107),
[sequential push fan-out and failure response](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L595),
[process client response mapping](../../cpp/src/internal/federation/process_federation_client_value_services.cpp#L13),
[public adapter exception mapping](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L137),
and the successful [configured-process multi-recipient case](../../cpp/tests/ieee1516_2025_connection_multi_recipient_interaction_ordering_catch2.cpp#L459).

Evidence: [public 2025 process send adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L46);
process planning and event materialization
([send planner](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389),
[route-only resolution](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L514),
[event snapshot](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L535),
[push/pull branch](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L569),
[push frame send](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L594),
[pull-mode default](../../cpp/src/internal/federation/process_federation_service.hpp#L32));
[push event decode](../../cpp/src/internal/federation/process_federation_client.cpp#L447)
and [bridge submission](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1212);
[public Evoke entry](../../cpp/src/internal/runtime/umbra_rti_ambassador_callback_control_services.cpp#L32),
[process poll request](../../cpp/src/internal/runtime/umbra_rti_ambassador_callback_control_services.cpp#L248),
[service dequeue](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L107),
and [callback projection](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L90).
Focused evidence: the [raw process pull case](../../cpp/tests/ieee1516_2025_connection_send_interaction_catch2.cpp#L457)
and [configured-process multi-recipient callback case](../../cpp/tests/ieee1516_2025_connection_multi_recipient_interaction_ordering_catch2.cpp#L459),
which sets the route at [line 523](../../cpp/tests/ieee1516_2025_connection_multi_recipient_interaction_ordering_catch2.cpp#L523)
and uses the immediate-mode fence at [line 807](../../cpp/tests/ieee1516_2025_connection_multi_recipient_interaction_ordering_catch2.cpp#L807).
The shared class/parameter projection is implemented by the [registry recipient
resolver](../../cpp/src/internal/federation/federation_registry.cpp#L1421) and
[class-scoped parameter filter](../../cpp/src/internal/federation/federation_registry.cpp#L1598); the
embedded delivery-time call is in the [callback closure](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_callbacks.cpp#L77).

### Timestamped interactions resolve the receiver projection later

Do not carry the ordinary process event's send-time projection into the TSO
path. Both inspected 2025 TSO paths retain the sent interaction and resolve a
receiver-specific class/parameter projection later, but at different points
relative to callback entry. This comparison describes current Umbra source;
it is not a normative HLA rule or a claim about the separate 2010 stream. This
section covers ordinary timestamped Send Interaction; timestamped Send
Directed Interaction has a separate target selector and its own
[guide](HLA-2025-DIRECTED-INTERACTION-FLOW-GUIDE.md#timestamped-send-is-the-same-routing-question-at-a-different-time-boundary).

```mermaid
flowchart TD
  Send["Accept timestamped Send Interaction at logical time T"] --> Retain["Retain sent class, parameters, time, and TSO message identity"]
  Retain --> Due{"Recipient reaches its TSO delivery boundary?"}
  Due -->|Not yet| Waiting["Keep this recipient's TSO work pending"]
  Waiting --> Due
  Due -->|Yes| Profile{"2025 endpoint"}
  Profile --> Embedded["Embedded: callback closure resolves the current receiver projection at callback dispatch"]
  Profile --> Process["Process: service resolves the current receiver projection before creating and sending the event"]
  Embedded --> Match{"Projection still available?"}
  Process --> Match
  Match -->|No| Suppress["No timestamped interaction callback for this recipient"]
  Match -->|Yes, embedded| EmbeddedCallback["Invoke timestamped callback with the selected class and visible parameters"]
  Match -->|Yes, process| Event["Event now contains the selected class and parameter handles"]
  Event --> Callback["Callback bridge dispatches the event under the configured callback model"]
```

The embedded callback closure queries the registry while dispatching the
callback. The process service instead computes the projection before it sends
the event frame; that event carries the selected class and parameter handles,
and the later callback bridge does not query the subscription again. Thus a
declaration change before those respective projection points may affect the
selected path, while a change after process event materialization does not
rewrite that event. This timing difference is source-derived; no focused
process case was identified that mutates an ordinary interaction subscription
between TSO send and event creation.

For a tested embedded regional example, the [DDM guide's subscription
replacement flow](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-replacement-regional-interaction-subscription-does-not-retarget-queued-tso-work)
shows the send-time source-region snapshot checked against the receiver's
current subscription at TSO callback dispatch. That case does not establish
process parity or the behavior of every nonregional declaration change. The
[process multiple-message case](../../cpp/tests/ieee1516_2025_connection_process_tso_interaction_multiple_message_ordering_catch2.cpp#L456)
checks two same-time interactions in FIFO callback order before one grant, but
does not mutate subscription state. See the [time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md#timestamped-message-lifecycle-and-callback-order)
for the separate temporal gate and [TSO retraction guide](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md)
for per-recipient Retract/Request Retraction outcomes. The [process endpoint
guide](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#3-a-timestamped-process-delivery-stays-in-transit-through-callback-completion)
traces the later event-frame and acknowledgement boundary.

## Object removal: delete, callback, and per-receiver cleanup

Keep the API roles distinct before tracing state changes:
[`RTIambassador::deleteObjectInstance` (§6.16)](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L823)
is the application's delete request;
[`FederateAmbassador::removeObjectInstance` (§6.17)](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L314)
is the RTI's receiver-side callback; and
[`RTIambassador::localDeleteObjectInstance` (§6.18)](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L853)
is a separate local service. The callback is not an application delete
request, and local delete is not an alias for federation-wide delete. These
clause labels establish the API roles; the detailed state transitions below
are bounded to current Umbra source and focused-test evidence.

### A publication epoch can separate delete privilege across object registrations

`HLAprivilegeToDeleteObject` is a special object-instance attribute, not a
class-wide delete switch. The 2025 standard MIM declares it on
`HLAobjectRoot` as `Static`; Umbra's 2025 publication-snapshot code cites
§5.1.2 for its implicit publication epoch. The unpublish path also applies
the corresponding instance-ownership loss (its source comment cites
§5.3.3(f)). The focused embedded scenario makes the cohort effect visible:
publishing `Efficiency` initially gives object A the implicitly published
delete privilege; explicitly unpublishing that special attribute removes
A's privilege. Republishing establishes a later registration epoch, so
object B receives the privilege but A is not retroactively changed.

When the owner then resigns with `DELETE_OBJECTS_THEN_DIVEST`, the test
observes B removed while A remains and its still-owned `Efficiency` attribute
is offered to the peer. The peer's assumption callback precedes B's removal
callback in this `HLA_EVOKED` embedded case only; that cross-service callback
order is not asserted as a general HLA or process-profile guarantee.

```mermaid
sequenceDiagram
  autonumber
  actor Owner as Publishing federate
  participant RTI as 2025 embedded RTI
  actor Peer as Surviving federate

  Peer->>RTI: Subscribe to the tested class's Efficiency attribute
  Peer->>RTI: Publish the tested class's Efficiency attribute
  Owner->>RTI: Publish the tested class's Efficiency attribute
  Note over Owner,RTI: Delete privilege is implicitly published in this established epoch
  Owner->>RTI: Register object A
  Note over Owner,RTI: A receives HLAprivilegeToDeleteObject
  RTI-->>Peer: discoverObjectInstance(A)
  Peer->>RTI: evoke callbacks for discovery
  Owner->>RTI: Explicitly unpublish HLAprivilegeToDeleteObject
  RTI->>RTI: A loses delete privilege
  Owner->>RTI: Publish HLAprivilegeToDeleteObject
  Owner->>RTI: Register object B
  Note over Owner,RTI: B receives privilege, while A is not retroactively granted it
  RTI-->>Peer: discoverObjectInstance(B)
  Peer->>RTI: evoke callbacks for discovery
  Owner->>RTI: Resign with DELETE_OBJECTS_THEN_DIVEST
  RTI->>RTI: Remove B, retain A, and divest its Efficiency attribute
  Peer->>RTI: evokeCallback()
  RTI-->>Peer: requestAttributeOwnershipAssumption(A, Efficiency)
  Peer->>RTI: evokeCallback()
  RTI-->>Peer: removeObjectInstance(B)
  Note over Peer: Callback order is specific to this embedded test
```

Evidence: the pinned [2025 MIM declaration](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L33) and
[implicit-publication projection](../../cpp/src/internal/federation/federation_registry_catalog_queries.cpp#L1007), plus the
[§5.3.3(f) unpublish-loss path](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L53) and
[publication-epoch flag](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L126).
The pinned 2025 [`publishObjectClassAttributes`](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L429),
[`unpublishObjectClassAttributes`](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L457), and
[`DELETE_OBJECTS_THEN_DIVEST` ResignAction](../../third_party/ieee1516.1-2025/include/RTI/Enums.h#L34)
declarations name the public transitions.
The [31-assertion embedded `DELETE_OBJECTS_THEN_DIVEST` scenario](../../cpp/tests/ieee1516_2025_embedded_resign_action_delete_then_divest_catch2.cpp#L4)
verifies this result. Its test-plan row maps the voluntary ResignAction
behavior to §4.12; it does not establish the full §5.1.2 publication
predicate or process parity.

For the cross-service case where a different federate acquires
`HLAprivilegeToDeleteObject` and then deletes the object, see the ownership
guide's [delete-authority transfer sequence](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#a-new-owner-can-gain-delete-authority-without-becoming-the-registrant).

Object deletion is not the inverse of registration at one instant. In the
current receive-order path, the registry validates that the deleting federate
knows the instance and holds HLAprivilegeToDeleteObject, plans removal for
other federates that know it, and immediately stops treating the deleting
federate as a known recipient. At the accepted-delete boundary it also
invalidates pending discovery, attribute-value-update, and ownership work for
the deleted object.
A receiver's queued removal is only a reservation: at a backend-specific gate,
the registry rechecks that the reservation, membership, and known-object state
are still valid. If they are, it commits that receiver's transition to unknown
before `removeObjectInstance` begins; stale work is suppressed instead of
manufacturing a callback. This is current 2025 implementation behavior for the
receive-order path, not by itself a normative ordering claim for every RTI.

The embedded ownership-ledger cleanup is a separate implementation concern.
For its callback gates, per-operation test coverage, and source-only edges, see
the [accepted-delete ownership flow](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#accepted-object-deletion-invalidates-pending-ownership-work).
That flow does not establish a general callback order or process-profile
parity.

```mermaid
flowchart TD
  Call["deleteObjectInstance(object, tag)"] --> Validate{"Deleting federate knows object<br/>and has delete privilege?"}
  Validate -->|No| Reject["Reject service; accepted-delete transition does not run"]
  Validate -->|Yes| Reserve["Plan and reserve removals for other known receivers"]
  Reserve --> Accept["Accept delete:<br/>clear deleting-federate knowledge<br/>and invalidate pending discovery,<br/>value-update, and ownership work"]
  Accept --> Recipients{"Any receiver removal reservations?"}
  Recipients -->|No| PurgeCheck
  Recipients -->|Yes| Event["Queue or stage one receiver removal event"]
  Event --> Gate{"At this backend's gate: reservation, membership, and known-object state still valid?"}
  Gate -->|No| Suppress["Retire stale reservation; suppress removal callback"]
  Gate -->|Yes| Unknown["Commit receiver transition to unknown<br/>and capture callback snapshot"]
  Unknown --> PurgeCheck{"Can the registry purge<br/>deleted object/name state now?"}
  Suppress --> PurgeCheck
  PurgeCheck -->|Yes| Purge["Purge object/name and refresh region use"]
  PurgeCheck -->|No| Retain["Retain deleted state for remaining receiver cleanup"]
  Purge --> Callback{"Was a valid receiver snapshot captured?"}
  Retain --> Callback
  Callback -->|Yes| Deliver["Dispatch removeObjectInstance(object, tag, producer)"]
  Callback -->|No| Done["No callback for this stale reservation"]
```

Read the receiver portion once per planned recipient; recipients cross this
gate independently, with no global “all removals before any callback” barrier.

The commit gate differs by delivery path. In the embedded path it runs when
the queued callback closure starts. In process pull mode (the default), it runs
when the receiver drains the event, before the response is bridged to the
callback. Optional process push mode runs it before pushing the event. In all
three source paths, the callback is downstream of the receiver-state commit;
the process-push statement is source-observed here, not covered by the focused
process deletion test cited below.

```mermaid
sequenceDiagram
  autonumber
  actor Deleter as Deleting federate
  participant DRTI as Deleter RTI
  participant Registry as Federation state
  participant RRTI as Receiver RTI
  actor Receiver as Receiver application

  Deleter->>DRTI: deleteObjectInstance(object, tag)
  DRTI->>Registry: validate object knowledge and delete privilege
  alt request rejected
    Registry-->>DRTI: error means the deletion transition does not begin
  else deletion accepted
    Registry->>Registry: reserve removal for other known receivers
    Registry->>Registry: mark delete accepted, clear deleting-federate knowledge, and invalidate pending object work
    Registry-->>RRTI: queue or stage removal event(object, tag, producer)
    RRTI->>Registry: reach backend-specific removal commit gate
    alt reservation is stale, receiver is no longer a member, or receiver is no longer known
      Registry-->>RRTI: suppress callback and retire stale reservation
    else receiver is still a member and knows the object
      Registry->>Registry: clear receiver knowledge and capture callback snapshot
      opt deleted object now satisfies purge predicate
        Registry->>Registry: purge object/name state and refresh region use
      end
      RRTI-->>Receiver: removeObjectInstance(object, tag, producer)
    end
  end
```

The deletion planner reserves other known recipients before committing the
accepted deletion. That reservation is not proof that a callback will still be
valid later: a receiver can resign or otherwise lose the relevant state before
its gate. The registry's removal snapshot lets the callback carry the producer
identity even when the final receiver transition makes the deleted registry
entry eligible for immediate purge.

When delete privilege has moved away from the original registrant, keep the
deleting requester distinct from the producer identity delivered to the
receiver. The 2025 ownership guide traces the configured-process source path
and its test limit in the [delete-authority transfer and removal identity
sequence](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#a-new-owner-can-gain-delete-authority-without-becoming-the-registrant).

| 2025 delivery path | Removal commit point | What the cited test establishes |
| --- | --- | --- |
| Embedded callback queue | Callback closure revalidates and commits before service-report routing and the user callback. | The focused ownership-query case observes `ObjectInstanceNotKnown` before callback dispatch and then one recorded `removeObjectInstance` callback. It does not assert every pending-work ledger is cleared. |
| Process pull (default) | Receive polling revalidates and commits before returning the removal event to the callback bridge. | The process deletion case observes the callback's object, producer, and tag payload using default `ProcessFederationServiceOptions`. |
| Process push (optional) | The service revalidates and commits before pushing the event to the session. | The linked process deletion test does not enable push mode; this row is source evidence only. |

Resignation can enter the same object-removal machinery, but its selected
ResignAction also interacts with ownership and other member-owned state. One
focused 2025 scenario shows NO_ACTION rejected while the resigning federate
still owns attributes; DELETE_OBJECTS then causes a peer removal callback.
That is a scenario-specific observation, not a full resign-action matrix. The
separate [federation/federate lifecycle guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#resignaction-effects-in-the-current-implementation)
owns the complete resign, connection-loss, and final-member behavior.

#### Producer `DELETE_OBJECTS` in the 2025 process profile

Keep the two roles separate: the departing producer invokes
[`RTIambassador::resignFederationExecution` (§4.12)](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L250);
a surviving member may later observe
[`FederateAmbassador::removeObjectInstance` (§6.17)](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L314).
The process path below is Umbra 2025 source behavior, not a normative callback
ordering rule or evidence of 2010/embedded parity.

For each eligible object selected by this ResignAction, the registry first
preflights routes for other current members whose known-object map contains the
object. A voluntary resignation with a missing required route is rejected
before those object mutations. On success, the registry reserves one removal
per such receiver, removes the departing producer's own known-object entry,
and returns recipient plans. The two charts separate that producer-side
planning boundary from the survivor's later per-recipient delivery and commit
gate.

##### Producer-side route preflight and planning

```mermaid
flowchart TD
  Producer["Producer requests Resign with DELETE_OBJECTS"] --> Plan["Select eligible objects; preflight routes to known surviving members"]
  Plan --> Routes{"Are all required callback routes available?"}
  Routes -->|No| Reject["Reject voluntary Resign before object mutation"]
  Routes -->|Yes| Reserve["Reserve one removal for each known survivor"]
  Reserve --> Forget["Remove producer's own known-object state"]
  Forget --> Plans["Process service receives the recipient removal plans"]
```

##### Survivor-side delivery and commit gate

For each returned recipient plan, the process service applies the receiver's
membership and known-object gates. The default pull path keeps an unconsumed
reservation in the server queue; optional push crosses the receiver commit
gate before sending the event:

```mermaid
flowchart TD
  Plan["Process service handles one recipient plan"] --> Session{"Is the survivor session still joined?"}
  Session -->|No| Cancel["Cancel the reservation; skip this recipient"]
  Session -->|Yes| Mode{"Is optional push enabled?"}
  Mode -->|No: default pull| Queue["Queue event; keep removal reservation pending"]
  Queue --> OwnResign{"Does this receiver Resign before polling?"}
  OwnResign -->|Yes| Clear["Receiver Resign clears server queue and registry reservation"]
  OwnResign -->|No| Poll["Receiver polls the queued event"]
  Poll --> PullGate["Recheck membership, reservation, and known-object state"]
  Mode -->|Yes: optional push| PushGate["Recheck and commit receiver removal before event push"]
  PullGate --> Valid{"Is the removal snapshot valid?"}
  PushGate --> Valid
  Valid -->|No| Suppress["Suppress stale event; no callback"]
  Valid -->|Yes| Commit["Forget receiver object; capture producer identity"]
  Commit --> Callback["Invoke removeObjectInstance with empty tag and producer"]
```

The Resign handler calls the registry, dispatches the returned plans, then
finishes grant reevaluation and clears the departing producer's session before
returning success. In default pull mode, the server-side event queue and the
registry reservation are not the callback itself: the survivor's receive poll
calls `beginObjectInstanceRemoval`, which rechecks membership, reservation,
and known-object state, commits that receiver's transition to unknown, and
captures the producer identity for callback construction. Optional push mode
crosses that same registry gate before the service sends the event. Because a
survivor can poll while the producer's Resign request is completing, do not
infer a general ordering between the producer's successful return and the
survivor's callback from the service call structure alone.

If the *receiver* successfully Resigns before consuming a producer's event,
that is a second membership transition, not a reversal of the producer's
ResignAction. Pull-queued work can still be discarded at the process service;
already-pushed work has crossed into client-owned buffering and callback
dispatch. The next subsection traces those two boundaries separately. No
focused process test located here settles the already-pushed-after-Resign race.

| Evidence slice | What it establishes | Limit |
| --- | --- | --- |
| Portable C++ TCK [`scenarioResignDeleteObjectLookupBoundaryPortable`](../../packages/hla-rti-cpp-tck/src/portable_tck.cpp#L4556) | Its scenario discovers an object, has its producer Resign with `DELETE_OBJECTS`, waits for the survivor's empty-tag removal callback, then expects name, handle, and known-class lookups to report `ObjectInstanceNotKnown`. | Adapter-agnostic test source; it does not establish that Umbra's process endpoint ran or passed this scenario, and this docs pass did not run it. |
| Umbra 2025 process path: [Resign planning](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L283), [request dispatch](../../cpp/src/internal/federation/process_federation_service_request_handlers.cpp#L153), [recipient enqueue](../../cpp/src/internal/federation/process_federation_service_connection_loss.cpp#L66), [pull commit gate](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L91), and [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L633). | The source implements the per-recipient reservation, profile-specific commit gate, and `removeObjectInstance` projection shown above. The [receiver cleanup](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L56) clears events still queued in that session. | Source-derived; the push-mode resignation path and an already-pushed callback racing receiver Resign are not established by a focused process test. |
| Focused process deletion test [`deleteObjectInstance`](../../cpp/tests/ieee1516_2025_connection_delete_object_instance_catch2.cpp#L771) | Checks the receiver callback's object, producer, and user tag for an explicit delete using default process options. | The later producer `DELETE_OBJECTS` call is cleanup after those assertions, not evidence for a resignation-originated callback. |

No claim is made here that the voluntary process `DELETE_OBJECTS` path has a
focused Umbra Catch2 assertion, or that embedded/2010 callback timing matches
this source trace. The complete action and ownership preconditions remain in
the lifecycle guide. Both producer-removal charts passed Mermaid CLI rendering
and local visual review. Splitting at the returned recipient-plan boundary
keeps producer-side resignation planning separate from survivor-side delivery
and revalidation; GitHub rendering remains unverified.

#### Cross-object `DELETE_OBJECTS_THEN_DIVEST`: removal and ownership callbacks

The 2025 registry handles eligible delete-privileged objects first, then
divests remaining owned attributes. Its divestiture pass skips instances
already marked `deleteAccepted`. A single resignation can therefore produce
two different effects for one surviving member: `removeObjectInstance` for a
known object that is deleted, and `requestAttributeOwnershipAssumption` for a
preflighted recipient of an attribute on a different, retained object. The
attribute owner change is committed in registry state; the callback queues are
separate projections of that transition.

```mermaid
flowchart TD
  Action["Producer Resigns with DELETE_OBJECTS_THEN_DIVEST"] --> Delete["First pass: handle eligible delete-privileged objects"]
  Delete --> Removed["Reserve a removal for each current member that knows a deleted object"]
  Delete --> Retained["Second pass: inspect owned attributes on non-deleted objects"]
  Retained --> Offered{"Was a recipient selected for a released attribute?"}
  Offered -->|Yes| Assumption["Plan requestAttributeOwnershipAssumption on the retained object"]
  Offered -->|No| ReleaseOnly["Release attribute without this recipient callback"]
  Removed --> Work["Return separate removal and ownership-assumption work lists"]
  Assumption --> Work
  ReleaseOnly --> Work
  Work --> Path{"Which 2025 path is being described?"}
  Path -->|Embedded HLA_EVOKED evidence| Embedded["Observed order: assumption, then removal"]
  Path -->|Process default pull| Pull["Receive selector checks removal queue before assumption-event queue"]
  Pull --> PullOrder["Source-derived relative order: removal before assumption when both wait"]
  Path -->|Process push enabled| Push["Service sends assumption frame before removal frame"]
  Push --> PushLimit["Frame submission order only; callback order is untested"]
```

Do not turn those adapter paths into one cross-service HLA ordering rule. The
focused embedded HLA_EVOKED test asserts `assumption` then `remove`. In the
process service, `dispatchConnectionLossResult` queues ownership-assumption
work before object removals. But with default pull, those work items occupy
different queues, and the receive selector reaches the removal branch before
the ownership-acquisition event branch; when both are pending, the removal
result is selected first (subject to earlier event families in that poll).
With optional push enabled, source submits the assumption event frame before
the removal frame, but there is no focused process test asserting the
application callback order. Neither process ordering is claimed as normative.

| Evidence slice | What it establishes | Limit |
| --- | --- | --- |
| Registry action passes: [delete planning](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L566), [divestiture skips deleted instances](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L635), and [assumption work planning](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L708). | Deleted-object removals and retained-object ownership-assumption plans are separate result work. | Source behavior; recipient selection is conditional on the preflighted offered-attribute set. |
| Focused [embedded scenario](../../cpp/tests/ieee1516_2025_embedded_resign_action_delete_then_divest_catch2.cpp#L62). | One peer knows both objects; the test observes an assumption callback for `Efficiency` on the retained object, a removal for the other object, and exact callback order `assumption`, `remove`. | One embedded HLA_EVOKED fixture, not a standard-wide cross-service order or process evidence. |
| Process [result dispatch](../../cpp/src/internal/federation/process_federation_service_connection_loss.cpp#L50), [default-pull selector](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L86), and [assumption-event selector](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L348). | Assumption work is dispatched before removals, but default pull prioritizes the removal queue over the later ownership-assumption event queue. The optional push path sends the assumption frame before sending removal frames. | Source-derived only; no focused process voluntary `DELETE_OBJECTS_THEN_DIVEST` test asserts callback order. |
| Portable C++ TCK [cross-object scenario](../../packages/hla-rti-cpp-tck/src/portable_tck.cpp#L4709). | Distinguishes a deleted object from a retained object, checks survivor removal metadata and known-object lookup, then checks later If-Available acquisition on the retained object. | It does not assert `requestAttributeOwnershipAssumption` or its order relative to removal; it is adapter-agnostic, not proof of Umbra process behavior. |

The focused voluntary callback-order assertion is embedded. The scoped test
scan found forced connection-loss automatic-directive cases, but those are a
different transition and do not establish voluntary process ordering. This
chart is pending Mermaid syntax/render and visual review; GitHub rendering
remains unverified. For the action table and ownership preconditions, see the
[federation lifecycle guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md).

#### Receiver Resign while assumption and removal callbacks are pending

In the cross-object case above, one producer Resign can leave a surviving
receiver with two distinct effects pending: an ownership-assumption callback
for a retained object and a removal callback for a different, deleted object.
If that receiver then successfully Resigns before consuming them, the process
path's result depends on how far each event has travelled. The chart is a
2025 process source trace—not a normative callback-cancellation or ordering
rule. It focuses on the object/ownership-specific server queues and transport
boundary; the general process callback-queue behavior after Resign is mapped
in the [callback and service-ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-profile-resign-and-queued-callback-work).

```mermaid
flowchart TD
  Planned["Producer Resign planned assumption and removal work for this receiver"] --> Receiver["Receiver successfully Resigns before observing both effects"]
  Receiver --> Boundary{"Where has each event reached?"}
  Boundary -->|Still queued for pull| Registry["Registry removes this recipient's assumption marker and removal reservation"]
  Registry --> ServerCleanup["Service filters ownership-acquisition events for the departed ID, then clears this session's removal and ownership-acquisition queues"]
  ServerCleanup --> PullEnd["Server-held pull work is discarded before the Resign response"]
  Boundary -->|Push frame already sent| ClientQueue["Frame may be buffered in the process client's pending-event deque"]
  ClientQueue --> Response["Resign request reads through its matching response"]
  Response --> Bridge["Client submits buffered event to the callback bridge"]
  Bridge --> CallbackGuide["Callback-model and queued-task outcomes: see the callback guide; this race is untested"]
  Boundary -->|Task already submitted to dispatcher| CallbackGuide
  Boundary -->|Callback already entered| Observed["Application observation already happened; Resign cannot undo it"]
```

For pull, registry cleanup removes the departing federate's pending
ownership-assumption callback marker and each object's pending-removal
reservation. The service filters queued ownership-acquisition events that
name the departed federate, then clears the joined session's
`attributeOwnershipAcquisitionEvents` and `objectInstanceRemovalEvents` before
sending the successful Resign response. These are the two server-held queues
in this scenario; a later pull in that ended membership cannot retrieve them.

Push has crossed a different boundary: the process client buffers unsolicited
frames read while waiting for the matching Resign response, then dispatches
those buffered events to the callback bridge. Server cleanup cannot recall a
frame already sent or undo an observation whose callback has entered. Whether
an event runs inline, remains queued for `evokeCallback`, or was already in the
shared dispatcher is covered by the linked callback guide; this combined
ownership/removal race is not established by a focused test. The nearest
process test consumes the assumption callback before Resign.

| Evidence slice | What it establishes | Limit |
| --- | --- | --- |
| Registry and server cleanup: [assumption-ledger invalidation](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L533), [removal-reservation invalidation](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L763), [departed-ID ownership-queue filter](../../cpp/src/internal/federation/process_federation_service_connection_loss.cpp#L25), [session queue clear](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L28), and [Resign request order](../../cpp/src/internal/federation/process_federation_service_request_handlers.cpp#L136). | Shows that registry invalidation and process session cleanup occur before a successful process Resign response. | Server-side source evidence; no focused test asserts both queued effects are absent after receiver Resign. |
| Push path: [assumption frame send](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L206), [removal frame send](../../cpp/src/internal/federation/process_federation_service_object_instance_removal.cpp#L73), [Resign client request](../../cpp/src/internal/federation/process_federation_client_federation_management.cpp#L89), [client buffering and request response boundary](../../cpp/src/internal/federation/process_federation_client.cpp#L430), [post-response dispatch](../../cpp/src/internal/federation/process_federation_client.cpp#L551), and [callback-bridge submission](../../cpp/src/internal/federation/process_federation_client_callback_dispatch.cpp#L265). | Shows where pushed events cross from server delivery into the client callback path. The separate [callback guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#process-profile-resign-and-queued-callback-work) covers dispatcher state after Resign. | Source-derived path only; no test fixes the pushed-frame-versus-receiver-Resign race or proves a post-Resign application callback. |
| Nearest focused process case: [unconsumed pushed ownership-assumption test](../../cpp/tests/ieee1516_2025_process_unconsumed_pushed_ownership_assumption_evoked_catch2.cpp#L305). | Exercises pushed ownership assumption under `HLA_EVOKED`, then Resigns the candidate. | The test consumes the assumption callback before Resign; it is not evidence for pending callback cleanup. No focused process receiver-Resign case was found for this combined assumption/removal scenario. |

This receiver-side cleanup is separate from the producer's action and from the
embedded HLA_EVOKED `assumption`-then-`remove` assertion above. Keep the
process pull result (server queue discarded) distinct from the push result
(client-buffered work may already be submitted to callback dispatch); do not
infer one profile's callback order or post-Resign behavior from the other.
This chart has no fresh Mermaid syntax verdict, render, visual review, or
GitHub presentation check.

Timestamped deletion is not the receive-order path above. Its sender-side
acceptance and each receiver's removal callback cross separate time and
object-knowledge boundaries.

### Timestamped deletion: each receiver reaches its own removal boundary

The pinned API names the timestamped `deleteObjectInstance` overload in §6.16
and gives it a `MessageRetractionHandle` result. The §6.17 removal callback
carries logical time, sent and received order, and an optional retraction
handle. Those signatures establish the operation and observation surfaces;
the detailed path below is current 2025 embedded behavior grounded in the
linked source and selected tests, not a complete normative service rule.
The portable API mental model is simply that the producer's timestamped delete
request and a receiver's timed removal callback are separate observations;
acceptance of the former does not mean the latter has already run.

```mermaid
sequenceDiagram
  autonumber
  actor Producer as Deleting federate
  participant RTI as 2025 embedded RTI
  participant State as Federation state
  actor RO as Known receiver without time constraint
  actor TC as Known time-constrained receiver

  Producer->>RTI: deleteObjectInstance(object, tag, T)
  RTI->>State: Validate object knowledge and HLAprivilegeToDeleteObject
  alt Request rejected
    State-->>RTI: No accepted-delete transition
    RTI-->>Producer: Typed service error
  else Request accepted
    RTI->>State: Retain pending delete,<br/>recipient ledger, invocation snapshot
    RTI-->>Producer: Optional MessageRetractionHandle
    alt At least one planned receiver
      par This receiver has no queued TSO copy
        State-->>RO: removeObjectInstance(object, tag,<br/>producer, T, received=RECEIVE)
        Note over State,RO: Clear receiver knowledge<br/>before its callback
      and This receiver has a queued TSO copy
        State->>State: Hold this recipient entry at T
        TC->>RTI: Reach the selected advance boundary
        State-->>TC: removeObjectInstance(object, tag,<br/>producer, T, timestamp order)
        Note over State,TC: Clear receiver knowledge<br/>before its callback
      end
      Note over State: First valid removal callback commits deletion<br/>and invalidates pending object work
    else No other federate knows the object
      State->>State: Accept deletion without a removal callback
      Note over State: Clear producer knowledge,<br/>retain object and name while designator is retractable
    end
  end
```

The per-recipient split is important. In the inspected embedded adapter, a
public retraction handle is produced only when the producer is time-regulating
and the deletion's effective order is `TIMESTAMP`. For that route, each
eligible known time-constrained recipient gets a queued TSO entry; other
planned recipients take a receive-order callback path. A timestamped callback can therefore have
`sentOrderType=TIMESTAMP` and `receivedOrderType=RECEIVE`. The sender's service
return is not proof that any receiver has run `removeObjectInstance`.
In the embedded no-fanout case, when no other federate knows the object, the
registry accepts deletion without a removal callback and clears the producer's
known-object entry while retaining the object/name and retraction record until
the designator's time bound. This is source-only evidence from the
[enqueue path](../../cpp/src/internal/federation/federation_registry_time_tso_enqueue.cpp#L859);
no focused test isolates this empty-recipient edge.

At a valid removal callback-entry gate, the registry commits that receiver's
transition to unknown before invoking user code. The first such entry commits
the execution-wide deletion, makes the producer unknown, and invalidates
pending discovery, update, and ownership work; other receivers cross their own
known-to-unknown gates independently. The embedded registry retains the
invocation snapshot while a returned designator remains retractable. This
commit/retention model is source-derived and should not be read as a universal
ordering claim. See the separate [TSO retraction flow](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#service-specific-effects-deletion-is-special)
for the service-specific restoration branch.

One focused embedded test uses the same deletion at time 7 with three advance
requests. Its asserted callback order is:

| Receiver request in the focused case | Observed callbacks at the time-7 boundary |
| --- | --- |
| `flushQueueRequest(10)` | `removeObjectInstance`, then `flushQueueGrant` |
| `timeAdvanceRequestAvailable(7)` | `timeAdvanceGrant`, then `removeObjectInstance` |
| `nextMessageRequestAvailable(10)` | `removeObjectInstance`, then `timeAdvanceGrant` |

These are results from the
[selected FQR/TARA/NMRA test](../../cpp/tests/ieee1516_2025_timestamped_delete_object_instance_fqr_tara_nmra_grants_catch2.cpp#L5),
not a claim of one global callback order across advance services or profiles. The test's TARA case explicitly sits at
its equal-time boundary; use the [time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md#boundary-comparison-matrix)
for the broader grant rules.

The embedded retraction tests cover both sides of callback admission. If the
producer retracts before any removal callback starts, the source clears the
pending-delete markers without restoring the invocation snapshot; intervening
instance changes therefore are not overwritten by that path
([source](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L286)).
The focused pre-delivery case asserts cancellation and that the receiver still
knows the object, but does not stage an intervening mutation. If one receiver's
removal has already begun while another copy is pending, Retract restores the
invocation-time object/name/ownership snapshot before queuing
`requestRetraction` for the receiver that saw removal; the still-pending
receiver gets neither stale removal nor retraction callback
([source](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L239)). The
[focused mixed-recipient case](../../cpp/tests/timestamped_object_deletion_retraction_catch2.cpp#L148)
asserts that sequence and restored ownership; the
[pre-delivery case](../../cpp/tests/ieee1516_2025_timestamped_delete_object_instance_retraction_catch2.cpp#L5)
asserts cancellation before callback admission. This deletion-specific undo is
not a generic interpretation of Retract for other timestamped services.

Process-endpoint evidence is narrower: the
[configured-process test](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L457)
checks removal payload, callback models, sender regulation, receiver
constraint, and receive-order preference. It does not exercise Retract after
removal or the invocation-snapshot restoration path. Do not infer process
parity for those embedded retraction details or any 2010 behavior.

For the separate Save/Restore boundary, see the [queued timestamped-deletion
restore case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-queued-timestamped-object-deletion-returns-after-fresh-registry-restore)
and [post-restore retraction after one receiver observes removal](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-retracting-a-restored-deletion-after-one-recipient-receives-it).
These are fresh embedded registries within one test process; their
`process-restart` tags do not establish an OS-process restart, process-endpoint
behavior, or 2010 parity.

### Local delete forgets one federate's view; it does not delete the instance

`localDeleteObjectInstance` is a different transition from
`deleteObjectInstance`. In the current 2025 registry implementation, a
successful local delete erases only the caller's known-object and pending
discovery entries. It leaves the execution-wide instance, its other members'
knowledge, and its attribute-owner map intact. It does not plan a peer
`removeObjectInstance` callback. This is source-observed behavior, not a claim
about every RTI or a substitute for the exact text of service 6.18.

The operation has ownership guards before that local-forget transition: the
caller is rejected if an ownership acquisition/cancellation/divestiture path
is still pending for it, or if it still owns any attributes of the instance.
Those guards prevent forgetting an instance while the registry still has
caller-specific ownership work or ownership to account for.

```mermaid
flowchart TD
  Call["localDeleteObjectInstance(object)"] --> Member{"Caller connected,<br/>joined, and still a member?"}
  Member -->|No| RejectMember["Reject; no knowledge transition"]
  Member -->|Yes| Known{"Object is known to caller?"}
  Known -->|No| RejectKnown["ObjectInstanceNotKnown;<br/>no knowledge transition"]
  Known -->|Yes| Pending{"Caller has pending<br/>ownership work for object?"}
  Pending -->|Yes| RejectPending["OwnershipAcquisitionPending;<br/>no knowledge transition"]
  Pending -->|No| Owns{"Caller still owns<br/>any object attributes?"}
  Owns -->|Yes| RejectOwns["FederateOwnsAttributes;<br/>no knowledge transition"]
  Owns -->|No| Forget["Erase caller's known-object entry<br/>and pending-discovery entry"]
  Forget --> Preserve["Keep federation-wide instance,<br/>other members' knowledge,<br/>and attribute owners unchanged"]
  Preserve --> Later{"A later eligible subscription<br/>causes discovery reevaluation?"}
  Later -->|Yes| Rediscover["Caller can become known again<br/>through a fresh discovery"]
  Later -->|No| StillUnknown["Caller remains unaware;<br/>no peer removal callback was planned"]
```

The embedded scenario exercises both rejection guards, observes
`ObjectInstanceNotKnown` from the caller's next known-object query after
success, then subscribes again and observes a fresh discovery. The original
owner subsequently updates the still-existing instance and the rediscovered
federate receives the reflection. A selected process-endpoint case exercises
successful local deletion, but it does not establish full process/embedded
parity for every rejection or rediscovery branch. Timestamped removal and
queued timestamped work remain separate: in the
[focused embedded recipient-isolation case](../../cpp/tests/local_delete_timestamped_object_removal_catch2.cpp#L141),
local deletion suppresses only that receiver's queued removal while another
known receiver still gets the timestamped callback before its grant. See the [time-management guide's
TSO lifecycle](HLA-2025-TIME-MANAGEMENT-GUIDE.md#timestamped-message-lifecycle-and-callback-order)
and the [TSO retraction flow's delete-specific branch](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#service-specific-effects-deletion-is-special);
neither is the local-forget transition.

### Diagnose disappearance by asking which state changed

The phrase “the object disappeared” can refer to the execution-wide object,
one participant's known-object view, or application observation of a removal.
The following is a comparison of the current 2025 implementation paths, not a
replacement for the separate service flows or the standard's action rules.

| Trigger | Execution-wide object state | One participant's known-object state | Callback meaning |
| --- | --- | --- | --- |
| Accepted `deleteObjectInstance` | Delete-accepted; registry state may outlive acceptance until receiver cleanup finishes. | Deleter becomes unknown at acceptance; each other receiver changes independently at its removal gate. | A valid receiver gets `removeObjectInstance` after its own state is cleared; stale work is suppressed. |
| `localDeleteObjectInstance` | Remains live; other members' knowledge and owners are unchanged. | Only the caller forgets; later discovery can make it known again. | No peer `removeObjectInstance` callback is planned. |
| Voluntary resign with an action that deletes an eligible object | Selected objects are marked delete-accepted; membership removal is a separate effect. | Leaver state/routes end; surviving members clean up independently. | A surviving known receiver can observe `removeObjectInstance`. |
| Connection-loss resignation | Forced path applies the member's automatic directive plus safety and TSO rules. | The lost member is removed; survivor effects depend on action and timing. | Not every loss removes every object or produces the same callback. |

Focused scenarios ground these rows without claiming complete service coverage:
the [embedded delete ordering check](../../cpp/tests/ieee1516_2025_attribute_ownership_check_catch2.cpp#L75)
and [process removal-payload case](../../cpp/tests/ieee1516_2025_connection_delete_object_instance_catch2.cpp#L779),
the [local-delete and rediscovery case](../../cpp/tests/local_delete_object_instance_catch2.cpp#L110),
the [voluntary `DELETE_OBJECTS` case](../../cpp/tests/ieee1516_2025_embedded_resign_action_delete_objects_catch2.cpp#L4),
and the selected [automatic-delete-on-loss case](../../cpp/tests/ieee1516_2025_connection_lost_automatic_delete_resign_directive_catch2.cpp#L5).
See the [lifecycle guide's forced-resignation flow](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#connection-loss-forced-resignation-is-a-different-entry-path)
for the directive and forced-cleanup boundaries.

`removeObjectInstance` is the receiver-side observation of a removal transition,
not the request that caused it: direct deletion and resignation cleanup can both
lead to that callback. For each receiver, the current implementation clears
known-object state before entering the callback and suppresses the callback if
the reservation is stale. Conversely, local deletion is a caller-only
forgetting operation and does not manufacture that peer notification.

One important edge stays in the lifecycle guide: the current embedded
implementation performs a delete-object pass when the final joined member
resigns, even with `NO_ACTION`, but that does not destroy the federation
execution; explicit Destroy is separate. The focused [final-member scenario](../../cpp/tests/ieee1516_2025_embedded_resign_action_final_federate_catch2.cpp#L4)
checks that the execution can be joined again after the object cleanup. This is
implementation evidence, not a complete normative `ResignAction` rule.

## MOM object-information report is a federate-local view

The 2025 MIM pairs the joined-federate request
`HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation` with
`HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstanceInformation`. The
request names one object through `HLAobjectInstance`, typed as
`HLAobjectInstanceHandle`; the report's `HLAobjectInstance` is typed as
`HLAobjectInstanceReference`. The report is reliable, Receive order, and has
the `HLAfederate` dimension ([request](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1184),
[report and fields](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1702)).

```mermaid
flowchart TD
  REQUEST["Joined federate requests one object<br/>HLAobjectInstance: HLAobjectInstanceHandle"]
  VIEW["Evaluate this federate's view of the object"]
  KNOWS{"Does this federate know the object?"}
  KNOWN["One HLAreportObjectInstanceInformation<br/>HLAobjectInstance<br/>HLAownedInstanceAttributeList<br/>HLAregisteredClass + HLAknownClass"]
  NULL["One NULL report of the same class<br/>HLAobjectInstance<br/>empty HLAownedInstanceAttributeList<br/>omit HLAregisteredClass + HLAknownClass"]
  SCOPE["HLAfederate report dimension<br/>reliable, Receive order"]
  OBSERVE["Eligible subscriber observes receiveInteraction"]

  REQUEST --> VIEW --> KNOWS
  KNOWS -- yes --> KNOWN --> SCOPE
  KNOWS -- no --> NULL --> SCOPE
  SCOPE --> OBSERVE
```

An empty `HLAownedInstanceAttributeList` does not by itself mean NULL. A
federate can know an object and own none of its attributes: that report still
includes both class parameters. In the NULL form, the list parameter is
present but empty, while `HLAregisteredClass` and `HLAknownClass` are omitted.
The registered class and the class known by this particular federate answer
different questions; the focused case includes both fields but uses equal
class values, so it does not exercise a case where they differ.

There is a wording conflict within the pinned MIM for the NULL list: the
report-level semantics call `HLAownedInstanceAttributeList` undefined, while
the parameter-level semantics require an empty list. Umbra's embedded encoder
includes the parameter in both shapes, and the focused case observes it present
and empty in the NULL response. Those are implementation/test observations;
this guide does not infer that the two MIM phrases are normatively equivalent
([report semantics](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1709),
[parameter semantics](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1723),
[encoder](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_object_instance_reports.cpp#L141),
[focused assertions](../../cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp#L294)).

The [focused embedded case](../../cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp#L147)
shows three useful states: a self-registered object whose report lists
`Efficiency` and the implicitly published `HLAprivilegeToDeleteObject`; an
owner-registered object known by the requester, with an empty owned-attribute
list but both class fields; and that same requester-local view after
`localDeleteObjectInstance`, where the report has only the object and empty
list parameters. The test checks that the request returns before the report
callback in its `HLA_EVOKED` case, then receives the interaction after callback
draining. This NULL response is about the requester's knowledge, not proof of
federation-wide deletion; see [local deletion's caller-only state change](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#local-delete-forgets-one-federates-view-it-does-not-delete-the-instance).

In the embedded implementation, the [Send Interaction handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L456)
validates the joined requester and object parameter before calling the
[per-federate report planner](../../cpp/src/internal/federation/federation_registry_mom_object_instance_information_reporting.cpp#L29).
The planner reads that federate's known-class and ownership state and plans the
`HLAfederate` dimension; the [report encoder](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_object_instance_reports.cpp#L163)
adds the two class parameters only for a known object. This describes the
reviewed embedded path, not the configured-process endpoint.

The configured-process path has a source-visible gap. The pinned MIM marks the
request Subscribe, while the reviewed process Send Interaction handler sends
non-special classes to the [ordinary receive-order planner](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389),
whose [admission path requires publication](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L50).
Source-derived inference: if this Subscribe-only request reaches that gate
without a publication entry, the generic planner returns
`interaction_class_not_published`; this is not a tested public-process result.
A bounded source/test search found no process-specific request/report handler
or configured-process test for this interaction. The generic route therefore
does not demonstrate the MIM-prescribed report, and the exact public exception
and embedded/process parity remain unverified.

## MOM delete-capability report is an aggregate snapshot

`HLArequestObjectInstancesThatCanBeDeleted` inherits the joined-federate
`HLAfederate` parameter from `HLAmanager.HLAfederate`; that parameter names the
joined federate being queried, which may differ from the requester. The 2025
MIM says the request results in one
`HLAreportObjectInstancesThatCanBeDeleted`. The report is reliable, Receive
order, and dimensioned by `HLAfederate`; its sole
`HLAobjectInstanceCounts` parameter is a list of `(registered object class,
count)` pairs, and the MIM requires an empty list for a NULL response
([inherited target parameter](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L914),
[request class](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1092),
[report shape](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1428)).

```mermaid
flowchart TD
  REQUESTER["Joined requester sends request<br/>HLAfederate = target federate"] --> VALIDATE{"Requester and target are joined?"}
  VALIDATE -->|No| REJECT["Request is rejected by the reviewed embedded handler"]
  VALIDATE -->|Yes| SNAPSHOT["Inspect current live object-instance state"]
  SNAPSHOT --> OWNER{"Does target own this instance's<br/>HLAprivilegeToDeleteObject?"}
  OWNER -->|No| SKIP["Do not count this instance"]
  OWNER -->|Yes| GROUP["Increment count for its registered object class"]
  SKIP --> NEXT{"More live instances?"}
  GROUP --> NEXT
  NEXT -->|Yes| SNAPSHOT
  NEXT -->|No| COUNTS{"Any instances counted?"}
  COUNTS -->|Yes| REPORT["One HLAreportObjectInstancesThatCanBeDeleted<br/>HLAobjectInstanceCounts = class/count list"]
  REPORT --> DIM["HLAfederate dimension identifies the target<br/>reliable, Receive order"]
  DIM --> RECIPIENT["Eligible report subscribers for that dimension<br/>receive the RTI interaction"]
  COUNTS -->|No| NULL["NULL response uses an empty count list"]
  NULL --> DIM
```

The embedded handler decodes the inherited `HLAfederate` target, requires both
the requesting and reported federates to be joined, and dispatches to the
report planner. The planner scans current object instances, excludes instances
whose deletion is already accepted, and counts an instance only when the
target federate owns its `HLAprivilegeToDeleteObject`; it groups by the
registered class, not the class known by a particular receiver. The routing
plan applies the target's `HLAfederate` dimension, while subscription matching
determines which federates receive the report
([embedded request handling](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L282),
[live-instance scan](../../cpp/src/internal/federation/federation_registry_mom_object_instances_that_can_be_deleted_reporting.cpp#L77),
[privilege-owner count predicate](../../cpp/src/internal/federation/federation_registry_mom_object_instances_that_can_be_deleted_reporting.cpp#L96),
[dimension routing](../../cpp/src/internal/federation/federation_registry_mom_object_instances_that_can_be_deleted_reporting.cpp#L107),
[report callback routing](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_object_instance_reports.cpp#L104)).

This is a point-in-time aggregate, not an object-handle list, a reservation,
or permission that remains valid for a later delete call. Delete authority is
the current ownership of the special attribute, and the delete service checks
object knowledge and that authority when the service is invoked; ownership or
object state can change after a report. The separate [delete transition](#object-removal-delete-callback-and-per-receiver-cleanup)
and [authority-transfer sequence](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#a-new-owner-can-gain-delete-authority-without-becoming-the-registrant)
show why the target federate is not necessarily the object's original
registrant. The focused embedded [live-count case](../../cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp#L137)
requests counts for another joined federate, observes one count for each of
two registered classes, deletes one object, then observes only the remaining
class count. It does not exercise an empty/NULL count list, transferred delete
authority, or a configured-process report.

For the untested empty-list branch, the embedded handler passes the planned
count map through the object-class-count encoder and the shared report queue
attaches that encoded value as the report parameter; neither step has a
non-empty-count guard. So the reviewed source path appears to construct the
MIM's empty-list response when the plan is applied and has eligible recipients,
but the focused test does not observe that runtime result
([handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L436),
[encoder](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_activity_reports.cpp#L109),
[queue](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_object_instance_reports.cpp#L21)).
Treat this as source evidence, not a focused NULL-response test.

For the configured-process route, the reviewed Send Interaction handler has no
special case for this MOM request and falls through to the ordinary
receive-order planner, whose admission check requires the sender to publish
the interaction class. Since the pinned MIM marks this request Subscribe, that
route is a source-visible concern—not a tested public exception or a claim
about required standard behavior. The bounded search found no
configured-process request/report test for this interaction
([generic process route](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389),
[publication admission](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L48)).

## MOM updated-instance report is a joined-lifetime ledger

`HLArequestObjectInstancesUpdated` uses the same inherited `HLAfederate`
target, reliable Receive-order request, and target-dimensioned report shape
as the delete-capability request. Its meaning is different: the 2025 MIM says
`HLAreportObjectInstancesUpdated` counts instances by registered class for
which the joined target has successfully invoked Update Attribute Values. It
is not a scan of currently live objects. The report carries one
`HLAobjectInstanceCounts` list, and the MIM describes one report interaction
([request](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1102),
[report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1450)).

```mermaid
flowchart TD
  REQUESTER["Joined requester sends request<br/>HLAfederate = target federate"] --> VALIDATE{"Requester and target are joined?"}
  VALIDATE -->|No| REJECT["Reviewed embedded handler rejects the request"]
  VALIDATE -->|Yes| LEDGER["Read target member's accepted-update ledger"]
  LEDGER --> ANY{"Any recorded objects?"}
  ANY -->|Yes| ENTRY["Each distinct updated object handle retains<br/>its registered class from the accepted update"]
  ANY -->|No| NULL["MIM NULL-response branch<br/>parameter-specific text says empty list"]
  ENTRY --> GROUP["Aggregate one count per recorded object<br/>grouped by registered class"]
  GROUP --> MORE{"More ledger entries?"}
  MORE -->|Yes| ENTRY
  MORE -->|No| REPORT["One HLAreportObjectInstancesUpdated<br/>HLAobjectInstanceCounts = class/count list"]
  REPORT --> DIM["HLAfederate dimension identifies the target<br/>reliable, Receive order"]
  NULL --> DIM
  DIM --> RECIPIENT["Eligible report subscribers for that dimension<br/>receive the RTI interaction"]
  RECIPIENT --> CALLBACK["In the focused HLA_EVOKED case,<br/>callback arrives after explicit draining"]
```

In the embedded registry, only a successfully accepted update reaches the
telemetry mutation: after update validation, the member's total
`HLAupdatesSent` count advances and the object handle enters a distinct-handle
set. The registered class is retained in a separate handle-to-class map for
the report. A second accepted update to the same object changes the service
total but not the distinct-object count or its class bucket. The report planner
iterates that member map, aggregates by saved registered class, and constructs
the report's `HLAfederate` dimension; it does not inspect the live object map
to decide report membership. The source explicitly keeps this as joined-member
history after object deletion, unlike the preceding delete-capability report's
current live ownership scan ([accepted update bookkeeping and retained history](../../cpp/src/internal/federation/federation_registry.cpp#L619),
[joined-member distinct-handle ledger](../../cpp/src/internal/federation/federation_registry_service_types.hpp#L197),
[registered-class ledger](../../cpp/src/internal/federation/federation_registry_service_types.hpp#L201),
[class-grouped report planner](../../cpp/src/internal/federation/federation_registry_mom_object_instances_updated_reporting.cpp#L73),
[target-dimension routing](../../cpp/src/internal/federation/federation_registry_mom_object_instances_updated_reporting.cpp#L85)).

This also separates three similarly named MOM measures. `HLAupdatesSent` is
the number of successful Update Attribute Values service invocations;
`HLAobjectInstancesUpdated` is the total number of distinct instances touched
by that joined federate; and this request/report adds the per-registered-class
breakdown of those distinct instances. The MIM defines the invocation total
and distinct-instance total separately ([`HLAupdatesSent`](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L335),
[`HLAobjectInstancesUpdated`](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L416)).
The existing [joined-federate MOM guide](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md)
explains the periodic scalar measures; this section follows the explicit
request and class-grouped response. It is not interchangeable with the
preceding live delete-authority count.

The focused embedded [class-grouping case](../../cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp#L203)
updates one Server object twice and one Soda object once, then observes one
count for each registered class after draining the requester's evoked callback
queue. It does not test a target with no updated instances, update history
after deletion, transport-specific empty results, or a configured-process
report. The MIM's NULL wording itself needs care: its report-level semantics
mentions no updates "for a given transport type" and an "undefined value",
while the parameter-level semantics explicitly requires an empty list. The
report has no transport-type parameter, and Umbra's reviewed planner groups
only by registered class; this guide records that wording without resolving
normative intent or claiming those NULL details are runtime-tested
([report NULL wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1457),
[parameter NULL wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1464)).

The configured-process Send Interaction route has no reviewed special handler
for this Subscribe-only request; the generic route proceeds to the ordinary
publication-gated planner. This is a source-level warning, not evidence of a
tested public exception or process/embedded parity. The bounded search found
no configured-process request/report test
([generic process route](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389),
[publication admission](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L48)).

## MOM update-activity report groups counts by class and transportation

`HLArequestUpdatesSent` asks the RTI to report how many successful Update
Attribute Values invocations a joined target generated; the MIM scopes the
count from the beginning of the federation execution. Each
`HLAreportUpdatesSent` carries a transportation type and a list of
class/count records, with the target identified by the inherited
`HLAfederate` dimension. This is an activity count, not a distinct-object
count: updating one object twice contributes two updates to its class and
transport bucket ([request](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1122),
[report and parameters](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1498),
[inherited target parameter](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L914)).

```mermaid
flowchart TD
  REQUESTER["Joined requester sends HLArequestUpdatesSent<br/>HLAfederate = target federate"] --> VALIDATE{"Requester and target are joined?"}
  VALIDATE -->|No| REJECT["Embedded handler rejects the request"]
  VALIDATE -->|Yes| LEDGER["Read target's accepted update counts<br/>by registered class and transportation"]
  LEDGER --> BUCKET["Build one report for each transportation bucket<br/>selected by the report planner"]
  BUCKET --> ANY{"Any class counts in this bucket?"}
  ANY -->|Yes| COUNTS["HLAupdateCounts contains class/count records"]
  ANY -->|No| EMPTY["NULL response carries an empty HLAupdateCounts list"]
  COUNTS --> REPORT["HLAreportUpdatesSent includes HLAtransportation"]
  EMPTY --> REPORT
  REPORT --> DIM["HLAfederate dimension identifies the target<br/>RTI report uses reliable Receive order"]
  DIM --> RECIPIENT["Eligible report subscribers receive the interaction"]
  RECIPIENT --> CALLBACK["In the focused HLA_EVOKED test,<br/>report callbacks follow explicit draining"]
```

Keep the three update-related measures separate. `HLAupdatesSent` is the
joined federate's total number of successful Update Attribute Values service
invocations. `HLAobjectInstancesUpdated` counts [distinct object instances](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L416).
`HLAreportUpdatesSent` instead groups update invocations by registered object
class and transportation type. In the embedded registry, a successful update
increments the scalar invocation total once, and increments the per-class
count for each distinct transportation present in that update's planned
passels. Therefore, an update spanning attributes assigned different
transportation types can contribute to more than one transport bucket; that
is source-derived, and the focused test uses one transportation type per
invocation ([MIM scalar totals](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L335),
[accepted update accounting](../../cpp/src/internal/federation/federation_registry.cpp#L562),
[embedded acceptance boundary](../../cpp/src/internal/runtime/umbra_rti_ambassador_attribute_update_services.cpp#L315),
[configured-process counter input](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L181)).

The MIM's report-level text says one interaction is sent per transportation
type used and describes `HLAupdateCounts` as undefined in a NULL response;
the parameter-level text instead requires an empty list when a given type
has no updates. Umbra's inspected planner initializes
both supported transportation buckets even when the target has no updates;
the focused test observes two empty reports for its idle target. The test
confirms that implementation result, but neither it nor this guide resolves
how the MIM's “type used” phrase relates to the per-type NULL requirement
([report-level NULL wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1509),
[parameter-level empty-list rule](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1521),
[planner buckets](../../cpp/src/internal/federation/federation_registry_mom_updates_sent_reporting.cpp#L77),
[focused empty-response assertions](../../cpp/tests/joined_federate_mom_updates_sent_catch2.cpp#L329)).

The focused embedded [class-and-transportation case](../../cpp/tests/joined_federate_mom_updates_sent_catch2.cpp#L145)
updates a Server instance twice using best effort and a Soda instance once
using reliable. It observes a count of two for Server/best effort and one for
Soda/reliable, then queries an idle joined target and verifies empty lists for
both reported transports. The requester sees no report before draining its
HLA_EVOKED queue. It does not exercise a single update call whose planned
passels use multiple transport types, nor a configured-process request/report
case.

The configured-process update service records accepted update counts in the
shared registry, but the bounded process-source review found no dedicated
`HLArequestUpdatesSent` handler or report encoder. The process Send Interaction
route follows its generic publication-planned path, while the pinned MIM marks
the request Subscribe. This is a source-visible profile gap, not a tested
public result or an embedded/process parity claim; no configured-process
report test was found ([generic process route](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389),
[generic publication admission](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L48),
[embedded request handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L1552),
[embedded report encoder/queue](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_activity_reports.cpp#L182)).

## MOM reflection reports separate callback activity from distinct objects

The two receiver-side reflection reports branch from the same `Reflect
Attribute Values` callback path, but retain different facts. `HLArequestReflectionsReceived`
reports callback activity grouped by the reflected object's registered class
and the transportation type used for that receipt. `HLArequestObjectInstancesReflected`
reports distinct object handles grouped by registered class. Repeated
reflections of one object therefore increase the former's count each time but
do not add another distinct object to the latter. Both requests identify the
joined target through `HLAfederate`; the reports use that target's
`HLAfederate` dimension ([activity request/report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1153),
[distinct-instance request/report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1112),
[inherited target parameter](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L914)).

```mermaid
flowchart TD
  SENDER["A federate sends Update Attribute Values"] --> DELIVERY["An eligible receiver reaches a Reflect Attribute Values<br/>callback-entry path"]
  DELIVERY --> ACTIVITY["Increment receiver reflection-callback total<br/>and registered-class × effective-transport count"]
  DELIVERY --> INSTANCE["Record receiver object handle and registered class<br/>in the distinct-instance ledger"]
  INSTANCE --> UNIQUE["One retained handle per object;<br/>later reflections reuse that entry"]

  ACTREQ["Joined requester sends HLArequestReflectionsReceived<br/>HLAfederate = target receiver"] --> AV{"Requester and target are joined?"}
  AV -->|No| AREJ["Reviewed embedded handler rejects the request"]
  AV -->|Yes| AREAD["Read target callback counts by transport and class"]
  ACTIVITY --> AREAD
  AREAD --> ARPT["HLAreportReflectionsReceived per transport<br/>HLAtransportation + HLAreflectCounts class/count list"]
  AREAD -.-> ANOTE["Pinned MIM: one report per type used;<br/>NULL prose says undefined, parameter prose says empty list.<br/>Implementation/test emits reliable and best-effort buckets."]

  OBJREQ["Joined requester sends HLArequestObjectInstancesReflected<br/>HLAfederate = target receiver"] --> OV{"Requester and target are joined?"}
  OV -->|No| OREJ["Reviewed embedded handler rejects the request"]
  OV -->|Yes| OREAD["Read target's retained distinct-handle ledger"]
  UNIQUE --> OREAD
  OREAD --> GROUP["Count distinct handles by saved registered class"]
  GROUP -->|Objects recorded| ORPT["HLAreportObjectInstancesReflected<br/>HLAobjectInstanceCounts class/count list"]
  GROUP -->|No objects| ONULL["MIM NULL-response wording;<br/>parameter-specific text says empty list"]

  ARPT --> ROUTE["Reliable, Receive-order report;<br/>HLAfederate dimension names the target"]
  ORPT --> ROUTE
  ONULL --> ROUTE
  ROUTE --> SUB["Eligible report subscribers for that dimension<br/>receive the RTI interaction"]
  SUB --> CALLBACK["In focused HLA_EVOKED tests,<br/>report callbacks follow explicit draining"]
```

The activity report's count is not an attribute-value count and not a count of
distinct objects: it is grouped receiver callback activity. In the embedded
source, the receipt counter and class/transport bucket advance immediately
before calling the application's
`FederateAmbassador::reflectAttributeValues`; the distinct-object map is
updated at the same callback-entry boundary. The registered class is captured
there as well ([activity ledger update](../../cpp/src/internal/federation/federation_registry.cpp#L779),
[callback dispatch](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L1660),
[distinct-instance update](../../cpp/src/internal/federation/federation_registry.cpp#L646),
[timestamped callback path](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_advance_dispatch.cpp#L466)).
These are implementation checkpoints, not claims that an application callback
returned successfully. The distinct-instance planner reads a retained member
map rather than enumerating only live objects, so deletion does not erase this
implementation's recorded history ([planner](../../cpp/src/internal/federation/federation_registry_mom_object_instances_reflected_reporting.cpp#L72));
the focused test does not cover post-deletion history.

The activity report's pinned MIM text says one report is sent for each
transportation type used and describes an undefined value when a type has no
reflections. Its `HLAreflectCounts` parameter semantics separately require an
empty list in a NULL response ([report semantics and parameter](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1538)).
Umbra's embedded planner initializes reliable and best-effort buckets, and the
focused idle-target test observes two reports with empty lists. That evidence
does not resolve how the MIM's report-level wording relates to its
parameter-level empty-list requirement ([planner](../../cpp/src/internal/federation/federation_registry_mom_reflections_received_reporting.cpp#L77),
[focused activity case](../../cpp/tests/joined_federate_mom_reflections_received_catch2.cpp#L194)).

The test also confirms the query names the receiver whose callback ledger has
the counts—not the application sender—and observes two best-effort Server
reflections and one reliable Soda reflection. It checks that reports are not
observed until the requester's `HLA_EVOKED` queue is drained. It does not cover
a target other than the requester, callback exceptions, post-deletion
distinct-instance history, or a configured-process report path. The existing
distinct-instance case covers repeated callbacks and class grouping, but not
its empty/NULL response ([distinct-instance test](../../cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp#L248)).

The configured-process Send Interaction review found no dedicated
`HLArequestReflectionsReceived` handler or focused request/report test; the
generic route and publication admission were inspected, but this is
source-visible evidence only. It establishes neither a public process result
nor embedded/process parity
([generic process route](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389),
[publication admission](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L48)).

Keep the neighboring MOM measurements separate: `HLAreflectionsReceived` is
the scalar service-invocation total, `HLAobjectInstancesReflected` is the
distinct-instance total, `HLAreportReflectionsReceived` is the on-demand
per-class/per-transport activity breakdown, and
`HLAreportObjectInstancesReflected` is the on-demand distinct-instance
breakdown ([MIM service total](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L320),
[MIM distinct-instance value](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L430),
[joined-federate MOM guide](HLA-2025-JOINED-FEDERATE-MOM-OBJECT-LIFECYCLE-FLOW-GUIDE.md)).

## MOM interaction-receipt report groups by sent class and transportation

The 2025 MIM defines two distinct receiver-side report pairs. The broad
`HLArequestInteractionsReceived` / `HLAreportInteractionsReceived` pair reports
received interactions; the directed
`HLArequestDirectedInteractionsReceived` /
`HLAreportDirectedInteractionsReceived` pair reports only directed receives.
Both identify the target joined federate with `HLAfederate`, group by the
interaction's sent class and transportation, and use reliable, Receive-order
report interactions. These are not the sender's `HLAinteractionsSent` total
([broad request](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1163),
[directed request](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1173),
[broad report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1610),
[directed report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1671),
[inherited target parameter](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L914)).
This is a request-driven Federation MOM report, not the separate
`HLAreportServiceInvocation` event/file mechanism ([reporting distinction](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#three-distinct-traces-call-outcome-invocation-report-requested-mom-report)).

```mermaid
flowchart TD
  SENDER["A federate sends an interaction"] --> ELIGIBLE["An eligible target reaches an embedded<br/>receiveInteraction or receiveDirectedInteraction callback path"]
  ELIGIBLE --> RECORD["At callback entry, update the total sent-class × transport ledger;<br/>directed receives also update the directed-only subset"]
  RECORD --> CALLBACK["Invoke the target's public interaction callback"]

  TOTAL_REQUEST["Requester sends HLArequestInteractionsReceived<br/>HLAfederate = target receiver"] --> TOTAL_VALIDATE{"Embedded route resolves<br/>requester and joined target?"}
  TOTAL_VALIDATE -->|No| TOTAL_REJECT["Embedded handler rejects the request"]
  TOTAL_VALIDATE -->|Yes| TOTAL_READ["Read target's all-interaction<br/>class × transportation ledger"]
  RECORD --> TOTAL_READ
  TOTAL_READ --> TOTAL_PLAN["Plan HLAinteractionCounts<br/>and HLAtransportation buckets"]
  TOTAL_PLAN --> TOTAL_REPORT["HLAreportInteractionsReceived"]

  DIRECTED_REQUEST["Requester sends HLArequestDirectedInteractionsReceived<br/>HLAfederate = target receiver"] --> DIRECTED_VALIDATE{"Embedded route resolves<br/>requester and joined target?"}
  DIRECTED_VALIDATE -->|No| DIRECTED_REJECT["Embedded handler rejects the request"]
  DIRECTED_VALIDATE -->|Yes| DIRECTED_READ["Read target's directed-only<br/>class × transportation subset"]
  RECORD --> DIRECTED_READ
  DIRECTED_READ --> DIRECTED_PLAN["Plan HLAinteractionCounts<br/>and HLAtransportation buckets"]
  DIRECTED_PLAN --> DIRECTED_REPORT["HLAreportDirectedInteractionsReceived"]

  TOTAL_REPORT --> SHAPE["Reliable, Receive order;<br/>HLAfederate dimension identifies the target"]
  DIRECTED_REPORT --> SHAPE
  SHAPE --> SUB["Eligible subscribers for that target dimension<br/>can receive the RTI report interaction"]
  SUB --> OBSERVE["Focused HLA_EVOKED cases observe reports<br/>after draining callbacks"]
```

The embedded registry updates the receiver's total interaction count and its
sent-class/transport bucket at callback entry, immediately before invoking the
public callback. A directed receive updates that same total ledger and a
separate directed-only subset. The broad request plans from the total map; the
directed request plans from the subset. This is *sent* class, even where
ordinary class promotion could make the public callback's received class an
ancestor ([ordinary callback boundary](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_callbacks.cpp#L128),
[timestamped callback boundary](../../cpp/src/internal/runtime/umbra_rti_ambassador_tso_receive_order_interaction.cpp#L161),
[directed callback boundary](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_callbacks.cpp#L196),
[shared total and directed-subset update](../../cpp/src/internal/federation/federation_registry.cpp#L731),
[broad report planner](../../cpp/src/internal/federation/federation_registry_mom_interactions_received_reporting.cpp#L77),
[directed report planner](../../cpp/src/internal/federation/federation_registry_mom_directed_interactions_received_reporting.cpp#L28),
[directed request handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L2108),
[directed report queue](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_activity_reports.cpp#L258),
[scalar receive-count source](../../cpp/src/internal/federation/federation_registry_mom_object_state.cpp#L953)).

The focused embedded [broad-report case](../../cpp/tests/joined_federate_mom_interactions_received_catch2.cpp#L149)
uses ordinary `ServerAction.TakeOrder` receives over reliable and best effort,
and observes empty lists for an idle target. The separate focused embedded
[directed-report case](../../cpp/tests/joined_federate_mom_directed_interactions_received_catch2.cpp#L180)
first receives an ordinary `TakeOrder`, then a directed `TakeOrder` of the same
class. Its directed report contains one reliable count and an empty best-effort
list; the idle target gets empty lists for both standard buckets. This directly
shows that the ordinary receive is excluded from the directed subset. A second
[custom-transport case](../../cpp/tests/ieee1516_2025_custom_transportation_directed_interaction_integration_catch2.cpp#L4)
confirms a declared custom transportation and its directed class count appear
in the directed report. The focused cases use the embedded profile and
`HLA_EVOKED`; the request returns before the test drains callbacks. They do not
establish configured-process behavior or embedded/process parity.

The broad report's own focused case does not send a directed interaction.
Although the shared embedded ledger source shows directed callbacks enter its
total map, that broad-report inclusion remains source-derived rather than
directly exercised; ancestor-class promotion remains untested in these report
cases. The configured-process evidence gap also remains open.

For both report classes, the pinned MIM says one response is sent per
transportation type used and describes a NULL response's
`HLAinteractionCounts` as undefined, while the parameter semantics require an
empty list ([broad report wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1618),
[directed report wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1678),
[directed parameter wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1693)).
The embedded tests observe empty lists, but do not resolve that MIM wording
difference. The directed-report semantics also name
`HLArequest.HLAreportDirectedInteractionsReceived` as the triggering request;
that apparent class-name typo is recorded as written, not silently corrected.

## MOM interaction-send reports separate total sends from the directed subset

The 2025 MIM defines a broad sender-side report pair,
`HLArequestInteractionsSent` / `HLAreportInteractionsSent`, and a separate
directed pair, `HLArequestDirectedInteractionsSent` /
`HLAreportDirectedInteractionsSent`. Each request names the joined federate to
report with `HLAfederate`; each report is scoped by an `HLAfederate` dimension
and carries `HLAtransportation` plus sent-class counts in
`HLAinteractionCounts`. Both request classes are Subscribe, reliable, and
Receive ordered; both report classes are Publish, reliable, and Receive ordered
([broad request](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1132),
[directed request](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1143),
[broad report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1578),
[directed report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1640),
[inherited target parameter](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L914)).

The broad report describes the joined federate's sends since the beginning of
the federation execution, grouped by *sent* class and transportation. Its MIM
text explicitly includes sends with region and says the counts reflect
related DDM usage. The directed pair reports directed sends separately. These
are requested class/transport activity reports, not the scalar
`HLAinteractionsSent` member field or the distinct `HLAreportServiceInvocation`
event/file mechanism ([reporting distinction](HLA-2025-SERVICE-INVOCATION-REPORTING-FLOW-GUIDE.md#three-distinct-traces-call-outcome-invocation-report-requested-mom-report)).

```mermaid
flowchart TD
  ACCEPT["An embedded send passes its final synchronous validation<br/>or queue-admission boundary"] --> TOTAL["Increment scalar HLAinteractionsSent once;<br/>record class × transportation in total report ledger"]
  ACCEPT --> ISDIRECTED{"Was the accepted send directed?"}
  ISDIRECTED -->|Yes| DIRECTED["Also record it in the<br/>directed-only subset"]
  ISDIRECTED -->|No| NODIRECTED["No directed-subset increment"]

  ALL_REQUEST["HLArequestInteractionsSent<br/>HLAfederate = joined sender to report"] --> ALL_VALIDATE{"Embedded requester and target<br/>are joined members?"}
  ALL_VALIDATE -->|No| ALL_REJECT["Embedded handler rejects the request"]
  ALL_VALIDATE -->|Yes| ALL_READ["Read target's total sent-class ×<br/>transportation ledger"]
  TOTAL --> ALL_READ
  ALL_READ --> ALL_PLAN["Build HLAinteractionCounts by<br/>sent class within each transport bucket"]
  ALL_PLAN --> ALL_REPORT["HLAreportInteractionsSent"]

  DIRECTED_REQUEST["HLArequestDirectedInteractionsSent<br/>HLAfederate = joined sender to report"] --> DIRECTED_VALIDATE{"Embedded requester and target<br/>are joined members?"}
  DIRECTED_VALIDATE -->|No| DIRECTED_REJECT["Embedded handler rejects the request"]
  DIRECTED_VALIDATE -->|Yes| DIRECTED_READ["Read target's directed-only<br/>sent-class × transport ledger"]
  DIRECTED --> DIRECTED_READ
  DIRECTED_READ --> DIRECTED_PLAN["Build HLAinteractionCounts by<br/>sent class within each transport bucket"]
  DIRECTED_PLAN --> DIRECTED_REPORT["HLAreportDirectedInteractionsSent"]

  ALL_REPORT --> SHAPE["Reliable, Receive order;<br/>HLAfederate dimension identifies the target"]
  DIRECTED_REPORT --> SHAPE
  SHAPE --> OBSERVER["Eligible report subscribers can receive<br/>the RTI-originated report interaction"]
  OBSERVER --> DRAIN["Focused HLA_EVOKED cases observe reports<br/>after callback draining"]
```

The embedded registry records a send only after the service has crossed its
final synchronous validation/queue-admission boundary. That sender ledger is
independent of receiver fan-out; a directed send increments the total ledger
and the directed-only subset. The focused broad-report case exercises two
transportations and a region-scoped send: it finds the regional
`MainCourseServed` count in the reliable bucket, consistent with the MIM's
explicit region inclusion. The directed-report case exercises two reliable
`TakeOrder` directed sends and finds a count of two in that class bucket. It
does not contrast an ordinary `TakeOrder` send against the directed report,
so exclusion of ordinary sends from that report is established by the separate
source ledgers, not that focused result ([accepted-send ledger](../../cpp/src/internal/federation/federation_registry.cpp#L679),
[broad planner](../../cpp/src/internal/federation/federation_registry_mom_interactions_sent_reporting.cpp#L27),
[directed planner](../../cpp/src/internal/federation/federation_registry_mom_directed_interactions_sent_reporting.cpp#L27),
[broad request handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L1690),
[directed request handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L1830),
[broad focused case](../../cpp/tests/joined_federate_mom_interactions_sent_catch2.cpp#L153),
[directed focused case](../../cpp/tests/ieee1516_2025_federation_mom_directed_interactions_sent_catch2.cpp#L4)).

The MIM says each report pair returns a response per transportation type used
and describes the report-level `HLAinteractionCounts` value in a NULL response
as undefined, while the parameter semantics require an empty list. The
embedded planners seed reliable and best-effort buckets; focused tests observe
both, including empty lists for an idle target ([broad NULL wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1589),
[directed NULL wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1651),
[directed parameter](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1661),
[empty-bucket case](../../cpp/tests/mom_sender_count_reports_null_buckets_catch2.cpp#L116)).
The directed-report semantics also appear to name
`HLAreportDirectedInteractionsSent` as the request class; this apparent MIM
copy/paste error is recorded as written, not silently corrected. The cited
cases are embedded-profile tests; no configured-process report case or custom
transport report case was found in this bounded review.

One additional embedded-source ordering detail is not exercised by those
tests: the request handler snapshots the report plan first, then records the
accepted MOM request itself as a non-directed send before queueing the report.
Thus a self-querying total report is planned before its current request enters
the sender ledger, while a later total query can see previously accepted MOM
request sends. A directed request does not enter the directed-only subset.
Treat this as source-derived implementation behavior, not as a normative or
configured-process claim ([broad request/plan/record order](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L1690),
[directed request/plan/record order](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L1830)).

## Evidence map: implementation versus exercised scenarios

| Topic | Current implementation entry points | Focused test evidence |
| --- | --- | --- |
| Object declarations and discovery | [publication adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_publication.cpp#L19), [subscription adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_subscription.cpp#L19), [registration](../../cpp/src/internal/federation/federation_registry_object_instance_registration.cpp#L196), [discovery planning](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp), [discovery state transition](../../cpp/src/internal/federation/federation_registry_object_instance_discovery_state.cpp#L15), [discovery eligibility](../../cpp/src/internal/federation/federation_registry.cpp#L830) | [process-endpoint object discovery](../../cpp/tests/ieee1516_2025_connection_object_discovery_catch2.cpp#L457) |
| Joined-federate MOM object-information report | [2025 MIM request/report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1184), [embedded handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L456), [per-federate report plan](../../cpp/src/internal/federation/federation_registry_mom_object_instance_information_reporting.cpp#L29), [known/NULL payload shape](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_object_instance_reports.cpp#L163) | [embedded known/NULL case](../../cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp#L147); no configured-process report case was found in the bounded review. |
| Joined-federate MOM delete-capability counts | [2025 MIM request/report](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1092), [embedded handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L282), [live privilege-owner count](../../cpp/src/internal/federation/federation_registry_mom_object_instances_that_can_be_deleted_reporting.cpp#L96), [target dimension](../../cpp/src/internal/federation/federation_registry_mom_object_instances_that_can_be_deleted_reporting.cpp#L107), [report fan-out](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_object_instance_reports.cpp#L104) | [embedded counts change after deletion](../../cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp#L137); empty/NULL, transferred-owner, and configured-process cases were not found in the bounded review. |
| Joined-federate MOM updated-instance counts | [2025 MIM request/report and NULL wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1102), [accepted-update lifetime ledger](../../cpp/src/internal/federation/federation_registry.cpp#L604), [class-grouped report planner](../../cpp/src/internal/federation/federation_registry_mom_object_instances_updated_reporting.cpp#L73), [report routing](../../cpp/src/internal/runtime/umbra_rti_ambassador_mom_object_instance_reports.cpp#L87) | [embedded repeated-update/class-grouping case](../../cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp#L137); empty/NULL and post-deletion-history cases are untested, and no configured-process report test was found in the bounded review. |
| Joined-federate MOM reflection activity counts | [2025 MIM request/report and transport/class parameters](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1153), [callback-entry activity ledger](../../cpp/src/internal/federation/federation_registry.cpp#L779), [per-transport planner](../../cpp/src/internal/federation/federation_registry_mom_reflections_received_reporting.cpp#L77), [embedded request handler](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L1441) | [embedded repeated-reflection, transport-bucket, and empty-list case](../../cpp/tests/joined_federate_mom_reflections_received_catch2.cpp#L194); callback exceptions and configured-process reports are untested. |
| Joined-federate MOM reflected-instance counts | [2025 MIM request/report and NULL wording](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1112), [receiver callback-entry ledger](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L834), [retained registered-class map](../../cpp/src/internal/federation/federation_registry.cpp#L669), [class-grouped report planner](../../cpp/src/internal/federation/federation_registry_mom_object_instances_reflected_reporting.cpp#L72) | [embedded repeated-reflection/class-grouping case](../../cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp#L248); empty/NULL and post-deletion history are untested, and no configured-process report case was found in the bounded review. |
| Joined-federate MOM update-activity counts | [2025 MIM request/report and transport/class parameters](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1122), [accepted per-class/transport update ledger](../../cpp/src/internal/federation/federation_registry.cpp#L562), [report planner](../../cpp/src/internal/federation/federation_registry_mom_updates_sent_reporting.cpp#L29), [embedded handler and report queue](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L1552) | [embedded repeated-update, transport-bucket, and empty-list case](../../cpp/tests/joined_federate_mom_updates_sent_catch2.cpp#L145); mixed-transport single-call and configured-process request/report behavior are untested. |
| Joined-federate MOM interaction-receipt counts | [2025 broad MIM request/report pair](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1163), [receiver callback-entry ledger](../../cpp/src/internal/federation/federation_registry.cpp#L731), [broad planner and route](../../cpp/src/internal/federation/federation_registry_mom_interactions_received_reporting.cpp#L77) | [embedded reliable/best-effort and empty-idle-target case](../../cpp/tests/joined_federate_mom_interactions_received_catch2.cpp#L149) uses ordinary receives only; directed inclusion in the broad report is source-derived, and ancestor-class promotion and configured-process report behavior remain untested. |
| Joined-federate MOM directed-interaction receipt counts | [2025 directed MIM request/report pair](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1173), [directed-only ledger update](../../cpp/src/internal/federation/federation_registry.cpp#L752), [directed planner and request route](../../cpp/src/internal/federation/federation_registry_mom_directed_interactions_received_reporting.cpp#L28), [embedded request handler and queue](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L2108) | [same-class ordinary-versus-directed exclusion, reliable/best-effort buckets, and idle target](../../cpp/tests/joined_federate_mom_directed_interactions_received_catch2.cpp#L180); [declared custom-transport report bucket](../../cpp/tests/ieee1516_2025_custom_transportation_directed_interaction_integration_catch2.cpp#L4). No configured-process request/report case was found in the bounded review. |
| Joined-federate MOM interaction-send reports | [2025 broad and directed MIM request/report pairs](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L1132), [accepted total and directed-send ledgers](../../cpp/src/internal/federation/federation_registry.cpp#L696), [separate report planners](../../cpp/src/internal/federation/federation_registry_mom_interactions_sent_reporting.cpp#L27) and [directed planner](../../cpp/src/internal/federation/federation_registry_mom_directed_interactions_sent_reporting.cpp#L27), [embedded request routes](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L1691) | [broad reliable/best-effort and region-scoped counts](../../cpp/tests/joined_federate_mom_interactions_sent_catch2.cpp#L153), [two directed sends in the directed report](../../cpp/tests/ieee1516_2025_federation_mom_directed_interactions_sent_catch2.cpp#L4), [empty total and directed buckets](../../cpp/tests/mom_sender_count_reports_null_buckets_catch2.cpp#L116). Broad-report inclusion of directed sends and self-query counting order remain source-derived; no configured-process or custom-transport report case was found. |
| Object-class publication withdrawal and ownership effects | [Pinned 2025 API §5.3](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L443), [embedded service adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_publication.cpp#L149), [registry pending-acquisition guard and owner transition](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L45) | [Subclass versus superclass whole-class withdrawal](../../cpp/tests/fom_declaration_management_catch2.cpp#L266), [pending acquisition rejects withdrawal](../../cpp/tests/ieee1516_2025_unpublish_object_class_attributes_ownership_pending_service_report_catch2.cpp#L71). Neither case asserts every owner-metadata cleanup postcondition. |
| Callback value-map and tag retention | [2025 reflection/interaction callback signatures](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L238), [VariableLengthData copy constructor](../../third_party/ieee1516.1-2025/include/RTI/VariableLengthData.h#L38), [data-pointer lifetime note](../../third_party/ieee1516.1-2025/include/RTI/VariableLengthData.h#L50) | The [embedded receive-order interaction case](../../cpp/tests/receive_order_interaction_catch2.cpp#L53) copies the const-reference map and tag during the callback and checks saved bytes after draining; this demonstrates a safe use pattern, not a cross-profile lifetime guarantee. |
| Attribute update request and reflection | [update adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_attribute_update_services.cpp#L61), [provider selection](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L547), [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L396) | [request → provide callback → provider update → reflection](../../cpp/tests/ieee1516_2025_connection_request_attribute_value_update_catch2.cpp#L470), [one reflection for overlapping subscriptions](../../cpp/tests/ieee1516_2025_connection_update_attribute_values_reflect_catch2.cpp#L457) |
| Embedded receive-order update validation, projection, and passels | [preflight, durable copy, re-plan, report/counter boundary, unlocked fan-out](../../cpp/src/internal/runtime/umbra_rti_ambassador_attribute_update_services.cpp#L206), [passel planning](../../cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp#L93), [receiver-known-class projection](../../cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp#L16), [callback-time recheck](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L734) | [embedded passel/projection/callback-lifecycle case](../../cpp/tests/ieee1516_2025_receive_order_attribute_update_passel_lifecycle_catch2.cpp#L5) |
| Receive-order interactions | [ordinary subscription API §5.10](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L577), [regional subscription API §9.10](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1595), [interaction subscriptions](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_subscription_services.cpp), [regional pair mutation and empty-set no-op](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L593), [global relevance and recipient overlap](../../cpp/src/internal/federation/federation_registry.cpp#L1463), [send plan](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L16), [class matching and projection](../../cpp/src/internal/federation/federation_registry.cpp#L1421), [send adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp), [interaction callbacks](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_callbacks.cpp) | [embedded same-receiver inherited receipt and callback-class identity](../../cpp/tests/inherited_interaction_receipt_catch2.cpp#L71), [process active/passive declaration-state test (no send)](../../cpp/tests/ieee1516_2025_connection_interaction_declaration_catch2.cpp#L590), [process-endpoint Send Interaction](../../cpp/tests/ieee1516_2025_connection_send_interaction_catch2.cpp#L457), [regional interaction routing](../../cpp/tests/ieee1516_2025_default_region_interaction_routing_catch2.cpp#L4), [regional active/passive transition and empty-set no-op (last recorded run failed before assertions)](../../cpp/tests/passive_regional_interaction_transition_catch2.cpp#L93), [passive-only ordinary/regional suppression](../../cpp/tests/ieee1516_2025_passive_interaction_subscription_catch2.cpp#L5), [separate-receiver class promotion and parameter projection plan](../../cpp/tests/federation_registry_composed_fom_catch2.cpp#L1209) ; The receipt test covers one embedded receiver's selected class, exactly one callback, and matching received-class identity with empty parameter maps; it does not test inherited parameter projection. The separate registry test covers projection, not same-receiver nearest-class choice. Process delivery remains exact-class evidence only, and the multi-recipient regional transition remains unverified at runtime. |
| Ordinary interaction publication withdrawal after send acceptance | [ordinary declaration removal](../../cpp/src/internal/federation/federation_registry_directed_interactions.cpp#L262), [send-time publication check and recipient planning](../../cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp#L16), [callback-time receiver projection](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_callbacks.cpp#L77), [recipient predicate](../../cpp/src/internal/federation/federation_registry.cpp#L1421) | No focused send → unpublish → evoke case was found in the reviewed suite. The [interaction-relevance advisory case](../../cpp/tests/declaration_relevance_advisory_catch2.cpp#L91) exercises declaration/advisory transitions, not accepted-message delivery after withdrawal. This conclusion is source-only and embedded-profile scoped. |
| Embedded accepted-send/report/counter boundary | [owned input snapshot, re-plan, report, counter, and unlocked fan-out](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_send_services.cpp#L2293), [selected report interaction/file route](../../cpp/src/internal/runtime/umbra_rti_ambassador_service_report_writers.cpp#L131), [delivery-time recipient projection](../../cpp/src/internal/runtime/umbra_rti_ambassador_interaction_callbacks.cpp#L57) | [successful file record before evoked app callback](../../cpp/tests/ieee1516_2025_send_interaction_service_report_file_catch2.cpp#L96), [immediate report observer while receiver callback stays queued](../../cpp/tests/ieee1516_2025_service_report_receive_order_send_interaction_catch2.cpp#L94); the [counter restore test](../../cpp/tests/ieee1516_2025_federation_registry_interaction_send_counters_restore_catch2.cpp#L17) covers persistence/restore, not public-send ordering |
| Delete and resignation effects | [delete planning and pending-work invalidation](../../cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp#L15), [receiver removal commit](../../cpp/src/internal/federation/federation_registry_object_instance_removal.cpp#L48), [embedded callback gate](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L1331), [process push/pull planning](../../cpp/src/internal/federation/process_federation_service_object_instance_removal.cpp#L12), [process pull commit gate](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L86), [process callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L650), [resign lifecycle](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L80) | [embedded unknown-before-callback query](../../cpp/tests/ieee1516_2025_attribute_ownership_check_catch2.cpp#L75), [process-endpoint delete callback payload](../../cpp/tests/ieee1516_2025_connection_delete_object_instance_catch2.cpp#L779), [DELETE_OBJECTS resign scenario](../../cpp/tests/ieee1516_2025_embedded_resign_action_delete_objects_catch2.cpp#L4) |
| Process ordinary interaction projection boundary | [shared send planner](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L389), [event materialization](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L535), [push/pull routing](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L569), [unpublish](../../cpp/src/internal/federation/process_federation_service_request_handlers.cpp#L370), [unsubscribe](../../cpp/src/internal/federation/process_federation_service_request_handlers.cpp#L384), [pull dequeue](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L107), [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L90) | [public process send case](../../cpp/tests/ieee1516_2025_connection_send_interaction_catch2.cpp#L457) inspects an exact-class pulled event's parameter handle/value and tag without asserting a receiver callback; the [configured-process multi-recipient case](../../cpp/tests/ieee1516_2025_connection_multi_recipient_interaction_ordering_catch2.cpp#L459) asserts exact-class callbacks and FIFO tags under both callback models. These process cases do not exercise ancestor promotion or post-send declaration changes. The separate [internal 2025 registry planner test](../../cpp/tests/federation_registry_composed_fom_catch2.cpp#L1208) checks exact-class and ancestor-class plans and filters the child parameters from empty parent views; it does not invoke process delivery, test one receiver subscribed at competing class levels, or exercise a nonempty inherited-parameter subset. |
| Timestamped interaction projection point by 2025 profile | [embedded callback-time projection](../../cpp/src/internal/runtime/umbra_rti_ambassador_tso_receive_order_interaction.cpp#L117), [process event-time projection](../../cpp/src/internal/federation/process_federation_service_tso_delivery.cpp#L209), [process event before matching grant](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L376) | [embedded regional subscription replacement](../../cpp/tests/ieee1516_2025_timestamped_regional_interaction_subscription_replacement_catch2.cpp#L91) covers one delivery-time change; [process same-time FIFO before one grant](../../cpp/tests/ieee1516_2025_connection_process_tso_interaction_multiple_message_ordering_catch2.cpp#L456) does not change declarations. No focused process subscription-mutation case was identified in this bounded review. |
| Federate-local object deletion | [6.18 API entry and declared exceptions](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L863), [runtime adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_deletion_services.cpp#L431), [registry local-forget transition](../../cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp#L136), [process service forwarding](../../cpp/src/internal/federation/process_federation_service_object_instance_deletion.cpp#L8) | [embedded state-preservation, rejection, and rediscovery scenario](../../cpp/tests/local_delete_object_instance_catch2.cpp#L110), [selected process-endpoint success path](../../cpp/tests/ieee1516_2025_connection_local_delete_catch2.cpp#L457) |
| Timestamped object deletion and recipient time gates | [pinned timestamped Delete API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L847), [embedded admission and recipient routing](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_deletion_services.cpp#L147), [per-recipient TSO ledger/snapshot](../../cpp/src/internal/federation/federation_registry_time_tso_enqueue.cpp#L680), [callback-entry removal commit](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L595) | [embedded delivered-delete retraction](../../cpp/tests/timestamped_object_deletion_retraction_catch2.cpp#L148), [FQR/TARA/NMRA boundary split](../../cpp/tests/ieee1516_2025_timestamped_delete_object_instance_fqr_tara_nmra_grants_catch2.cpp#L5), [recipient-local forget](../../cpp/tests/local_delete_timestamped_object_removal_catch2.cpp#L141), [process callback metadata](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L457). Retraction restoration remains embedded-only in the cited evidence. |

## Deliberate limits and next evidence

This guide does not model the full object-class or interaction-class
declaration lattice, every active/passive transition (the interaction diagram
covers only selected ordinary and regional cases), automatic versus
explicit regions, ownership negotiation, complete update-rate cadence/admission,
timestamped reflection/interaction routing, the full timestamped
deletion/retraction and resign-action matrices, callback re-entry, or
save/restore interlocks. Use the linked focused guides for neighboring state
machines; do not infer untested profile or edition parity from the selected
deletion scenarios.

The interaction guide now separates global active relevance, recipient-level
active/passive matching, class promotion, parameter projection, and accepted
send from callback fan-out. The guide now also separates ordinary receive-order
attribute validation, passel grouping, receiver-known-class projection, and
callback-time re-evaluation. The companion [attribute update-rate admission
guide](HLA-2025-ATTRIBUTE-UPDATE-RATE-ADMISSION-FLOW-GUIDE.md) follows active
ordinary/regional rate resolution and per-attribute callback-time filtering,
while keeping TSO grant eligibility separate. Keep timestamped delivery, DDM
overlap, and DSE candidate retention in their existing guides. Keep 2010 in its
separate guide and do not edit the HLA Requirements Lab as part of this Umbra
documentation phase.
