# IEEE 1516.1-2025 Attribute Ownership: Flow Guide

In HLA, ownership identifies who currently has authority to update a particular
object-instance attribute. It is distinct from publication, subscription,
receiver knowledge, and whether an update is presently eligible for delivery.
Acquisition and divestiture services are separate protocols that can change
that authority; an accepted request and a completed transfer are not the same
event. This guide explains those distinctions first, then uses selected 2025
Umbra paths and tests as bounded examples—not as an implementation contract or
a conformance claim.

## Scope and authority

- **Standard boundary:** IEEE 1516.1-2025 only. The official [IEEE standard
  page](https://standards.ieee.org/ieee/1516.1/6688/) identifies the document
  that defines the RTI services and interfaces. Consult the standard itself
  for normative requirements.
- **Implementation boundary:** current 2025 C++ API scenarios exercised by
  Umbra's non-installable development profile, including focused embedded and
  process-boundary tests. Some private runtime components are shared
  implementation machinery; this guide does not infer 2010 behavior from them.
- **Edition separation:** IEEE 1516.1-2010 is deliberately outside this guide.
  It has separate public API and compatibility tests. No equivalence with its
  ownership behavior is asserted here.
- **Evidence boundary:** source describes current implementation behavior;
  focused tests demonstrate only their named scenarios. Neither overrides the
  standard or proves full feature support, interoperability, or conformance.

The service anchors used here are the 2025 ownership-management services,
including Attribute Ownership Acquisition (§7.8), Attribute Ownership
Acquisition If Available (§7.9), Attribute Ownership Release Denied (§7.12),
and Attribute Ownership Divestiture If Wanted (§7.13). The implementation
source cites these service sections; use the official standard for the
normative text and exact requirements.

## Start with the unit of state

Ownership is tracked for an **object-instance attribute**, not as one indivisible
property of the object. An object can therefore have attributes owned by
different federates, unowned attributes, and attributes with requests pending
at the same time. A single API call containing several attributes can produce
different outcomes for different members of its set.

Keep these concepts separate:

| Concept | What it tells you | What it does not tell you |
| --- | --- | --- |
| Publication | Whether a federate has declared it can produce an object-class attribute. | That it currently owns a particular instance attribute. |
| Subscription/discovery | Whether an instance is relevant and known to a receiving federate. | That the receiver owns any of its attributes. |
| Ownership | Which federate owns a particular instance attribute, or whether it is unowned. | The attribute's current value, whether an update was delivered, or—by itself—the privilege to delete the object. |
| Accepted request | The RTI admitted a service call and recorded or scheduled its work. | That the requested ownership transition has already completed. |
| Ownership callback | A public outcome for the request; in the current 2025 profile, some ownership state is committed at callback dispatch. | A guarantee that every requested attribute will resolve the same way. |

### Unpublishing can also end update authority in this profile

Publication and per-instance ownership are distinct state, but do not assume
that unpublishing only changes the declaration. In the current embedded 2025
path, unpublishing an owned attribute set synchronously removes the caller's
owner entries for those attributes and clears associated update metadata
([registry transition](../../cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp#L170),
[public whole-class route](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_class_publication.cpp#L149)).
The focused receive-order update test first accepts an update, then calls
`unpublishObjectClass`, after which another update fails with
`AttributeNotOwned` ([before](../../cpp/tests/receive_order_attribute_update_catch2.cpp#L249),
[after](../../cpp/tests/receive_order_attribute_update_catch2.cpp#L332)). This
is a bounded embedded implementation/test observation, not a complete
interpretation of normative §5.3 or a statement about the 2010 stream; consult
an authorized standard copy before generalizing it.

Ownership also does not settle what happens to an already accepted timestamped
update if its producer later divests or resigns. The [TSO/ownership comparison](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#ownership-changes-and-an-accepted-tso-update-are-separate-timelines)
shows selected embedded cases and their limits.

### Service return, transfer outcome, and callback time are separate

The 2025 C++ acquisition services return `void`; synchronous precondition
failures are typed exceptions. A normal return therefore carries no
"ownership granted" value. The per-attribute outcome is reported separately
through callbacks such as `attributeOwnershipAcquisitionNotification` and
`attributeOwnershipUnavailable`. See the pinned
[`attributeOwnershipAcquisition` and `attributeOwnershipAcquisitionIfAvailable` signatures](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1042)
and the corresponding [`FederateAmbassador` callbacks](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L432)
([Unavailable](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L440)).

### Compare the call, per-attribute outcome, and observed ownership

The public call result, a transfer callback, and an ownership query answer
different questions. This comparison is limited to the selected embedded
`HLA_EVOKED` cases below; the API exception lists are broader than the examples
exercised by those tests, and this is not process-profile parity evidence.

| Boundary | Acquisition If Available | Divestiture If Wanted |
| --- | --- | --- |
| Synchronous call outcome | The service returns `void`: a normal return admits pending work, not ownership. A typed exception such as `ObjectClassNotPublished`, `AttributeNotPublished`, `ObjectInstanceNotKnown`, or `AttributeNotDefined` rejects the call at the service boundary; the focused case also checks `FederateOwnsAttributes`. | The service returns `void` and fills `divestedAttributes` with the subset transferred synchronously. The focused case sees an empty output set and retained ownership when no acquirer is pending; with eligible acquirers, the output set is replaced with the transferred subset. Typed exceptions such as `ObjectInstanceNotKnown`, `AttributeNotDefined`, and `AttributeNotOwned` are service failures, not transfer callbacks. |
| Per-attribute callback | Pending members resolve separately as acquisition notification or unavailable. A mixed set can produce both outcomes. | A committed transfer can queue acquisition notifications to the new owners; those notifications are separate from the owner's output set and carry the divestiture tag in the focused case. A queued release request made stale by the transfer is suppressed. |
| Ownership observation | `isAttributeOwnedByFederate` is false for the unowned member after the call returns, then true after its notification callback begins; the unavailable member remains unowned by the requester. | In the selected `HLA_EVOKED` case, `isAttributeOwnedByFederate` already reflects the new owners after the normal service return and before their notifications are evoked. |

The links identify the exact public signatures and bounded test assertions:
[If Available API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1047),
[If-Wanted API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1082),
[If-Available exception checks](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_if_available_catch2.cpp#L80),
[If-Available callback/query checks](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_if_available_catch2.cpp#L112),
[If-Wanted exception checks](../../cpp/tests/attribute_ownership_divestiture_if_wanted_pending_catch2.cpp#L172),
and
[If-Wanted transfer and callback checks](../../cpp/tests/attribute_ownership_divestiture_if_wanted_pending_catch2.cpp#L252).
The table distinguishes the synchronous typed-exception gate, each
per-attribute callback, and the state query; it does not assert that callbacks
must always wait until a public call has returned.

Do not read the sequence diagrams as a universal promise that every callback
begins after the API call that caused the protocol transition has returned.
Callback delivery follows the selected model; see the
[callback/service-ordering guide](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#hla_immediate-callback-work-can-be-on-the-service-stack).
For example, the focused If-Available case checks that the service has returned
while an attribute is still unowned, then checks ownership after its
acquisition-notification callback begins
([pre-callback](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_if_available_catch2.cpp#L118),
[post-callback](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_if_available_catch2.cpp#L178)).
The process-boundary negotiated-divestiture case uses `HLA_IMMEDIATE`, but its
requester's acquisition notification is dispatched during a subsequent
`getObjectClassHandle` response, before that lookup returns—not inside the
owner's earlier `confirmDivestiture` call
([test](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L2268)).
These are two bounded timing observations, not a global callback-order rule.

### Four ownership paths: who initiates, and what reports the outcome?

Use this as an orientation map, not an exhaustive ownership-service catalog or
a callback-order diagram. Ordinary Acquisition, release/denial, cancellation,
and ownership queries have their own paths below. First identify which side
starts one of these four transfer paths; then follow its established detail:
[Acquisition If Available](#if-available-evaluate-at-the-result-callback),
[Negotiated Divestiture](#negotiated-divestiture-adds-a-confirmation-stage),
[Unconditional Divestiture](#unconditional-divestiture-and-assumption-offers),
or [Divestiture If Wanted](#divestiture-if-wanted-has-an-output-set-result-and-later-notifications).

```mermaid
flowchart TD
  Start["Choose among these four ownership paths"] --> Actor{"Who starts this path?"}
  Actor -->|"Requester"| IfAvailable["Acquisition If Available · §7.9"]
  IfAvailable --> IfAvailableResult["Void service call; acquisition-notification or unavailable callback carries each attribute's outcome"]
  IfAvailableResult --> IfAvailableState["In the cited embedded test, ownership changes when its notification callback begins"]
  Actor -->|"Current owner"| Intent{"What does the owner intend?"}
  Intent -->|"Relinquish without naming a recipient"| Unconditional["Unconditional Divestiture · §7.2"]
  Unconditional --> Followup["Owner relinquishes; pending acquisitions and eligible-candidate offers are per-attribute outcomes; an offer alone transfers nothing"]
  Intent -->|"Transfer only the subset already wanted"| IfWanted["Divestiture If Wanted · §7.13"]
  IfWanted --> Subset["Void service fills divestedAttributes with the transferred subset; selected no-acquirer case retains ownership"]
  Subset -. "separate channel" .-> IfWantedCallbacks["Acquisition notifications are not the output set"]
  Intent -->|"Use an owner-confirmation stage"| Negotiated["Owner proposes negotiated divestiture · §7.3"]
  Negotiated --> ConfirmationRequest["RTI asks owner to confirm · §7.5"]
  ConfirmationRequest --> Confirm["Owner calls Confirm Divestiture · §7.6"]
  Confirm --> NegotiatedResult["Pending requester can receive acquisition notification; this is not the owner's service return value"]
```

The RTIambassador operations in these four paths are `void` (including both
negotiated-divestiture calls); Divestiture If Wanted additionally fills
`divestedAttributes`. The chart's outcome notes are bounded to the linked
detailed evidence, not universal timing: the selected embedded If-Available
case observes ownership at notification-callback entry; the If-Wanted and
unconditional tests observe their own state/result boundaries; the process
Confirm Divestiture case does not assert an ownership-query timestamp. Consult
the detailed sections and their tests before applying one profile's state
timing to another.

The pinned signatures are at [§§7.2–7.3](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L989),
[§7.6](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1024),
[§7.9](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1061),
and [§7.13](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1093);
the selected evidence is the [If-Available test](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_if_available_catch2.cpp#L112),
[If-Wanted test](../../cpp/tests/attribute_ownership_divestiture_if_wanted_pending_catch2.cpp#L252),
[unconditional-divestiture test](../../cpp/tests/ieee1516_2025_unconditional_attribute_ownership_divestiture_catch2.cpp#L170),
and [process Confirm Divestiture case](../../cpp/tests/ieee1516_2025_confirm_divestiture_process_assumption_catch2.cpp#L62).
These tests are profile-bounded examples, not one cross-service conformance
matrix. The checkout lacks full normative clause prose, so this service-
selection map does not claim to replace it.

## Ownership transfer and update-region association are separate boundaries

Ownership answers who currently has authority over an object attribute;
`associateRegionsForUpdates` configures the source-region side of that
attribute's DDM path. Keep these as separate questions. The pinned API exposes
ownership transfer (§§7.9 and 7.13) and update-region association/removal
(§§9.6–9.7) as different services. Its declarations identify the service
boundaries, but do not replace the normative clause text.

The selected embedded 2025 cases expose a useful implementation nuance at the
transfer boundary: the former owner's committed association is cleared, not
inherited by the new owner. In two other focused cases, the prospective owner
can establish a deferred association before acquiring the attribute; that
association is inactive while another federate owns the attribute, is promoted
if still present when ownership transfers, and is not promoted if it was
unassociated first. With no association left, the first update after transfer
uses the default source region. If the former owner later reacquires the
attribute, it must explicitly associate its region again. These are selected
embedded test observations, not a claim of process-profile parity or a complete
normative rule for every acquisition path.

```mermaid
sequenceDiagram
  autonumber
  actor A as Current owner
  participant RTI as 2025 embedded RTI
  actor B as Prospective owner
  actor S as Regional receiver

  A->>RTI: Register object with source region R-A
  B->>RTI: Associate update region R-B before ownership
  Note over RTI: In the focused embedded case, R-B is deferred and does not affect A's updates
  A->>RTI: Update attribute value
  RTI-->>S: Reflect using R-A
  opt B cancels its deferred association before transfer
    B->>RTI: Unassociate update region R-B
    RTI->>RTI: Remove B's deferred association
  end
  B->>RTI: Request acquisition If Available
  A->>RTI: Divestiture If Wanted
  RTI->>RTI: Commit ownership A to B
  RTI->>RTI: Clear A's association and promote B's association only if it remains
  RTI-->>B: Acquisition notification
  alt B's deferred association remains
    B->>RTI: Update attribute value
    RTI-->>S: Reflect using R-B
  else No deferred association remains
    B->>RTI: Update attribute value
    RTI-->>S: Reflect using the default source region
  end
```

This is a comparison of separate focused scenarios, not one end-to-end test:
the [association-clearing/reacquisition case](../../cpp/tests/ownership_transfer_update_region_catch2.cpp#L142)
observes the former owner's region disappear, the new owner's first update use
the default source, and explicit association be needed again after reacquisition;
the [deferred-association promotion case](../../cpp/tests/ownership_transfer_update_region_catch2.cpp#L478)
observes a retained prospective-owner association become active at transfer;
and the [canceled-deferred-association case](../../cpp/tests/ownership_transfer_update_region_catch2.cpp#L692)
observes that unassociation prevents later promotion. The pinned API locates
these services at [acquisition If Available §7.9 and divestiture If Wanted
§7.13](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1048),
and [associate/unassociate regions for updates §§9.6–9.7](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1514).
The [selected transfer path](../../cpp/src/internal/federation/federation_registry_attribute_ownership_divestiture_if_wanted.cpp#L286)
clears and then promotes association state; the [association planner](../../cpp/src/internal/federation/federation_registry_update_region_associations.cpp#L75)
keeps non-owner associations deferred. For the broader HLA source/receiver
region model, see the DDM guide's
[object-attribute source-association flow](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#object-attributes-source-association-discovery-relevance-and-updates).

The checkout does not contain the complete normative clause text needed to
independently verify every ownership-loss rule. Read the IEEE standard before
generalizing the source comment or these embedded test outcomes to all RTIs.

### 2025 process-endpoint boundary

The pinned API declares Attribute Ownership Divestiture If Wanted, but this
checkout does not expose it through the 2025 process service route. The public
adapter method directly checks membership in `embeddedFederationRegistry()`
and calls that registry's plan; it does not delegate through the process
client. The process transport/client/server inventory does contain explicit
Confirm Divestiture and Unconditional Divestiture routes, but no corresponding
`attribute_ownership_divestiture_if_wanted` operation or handler. Therefore,
the transfer and update-region results above describe selected embedded
behavior only; they are not evidence that this service is usable through the
process endpoint. This is an Umbra source-inventory finding, not a claim that
IEEE 1516.1-2025 makes the service optional or unavailable. See the pinned
[API declaration](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1093),
the [public adapter implementation](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_services.cpp#L1507),
and representative [process protocol](../../cpp/src/internal/federation/transport_service_protocol.hpp#L211)
and [service handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L610)
routes.

### Process ownership routes at a glance

This is an Umbra 2025 route/evidence index, not an HLA callback-order table.
The public service outcome, private process response, event-transport mode,
and recipient callback model are separate boundaries.

| HLA service | Umbra process route | Focused process evidence and limits |
| --- | --- | --- |
| Acquisition If Available | Explicit request and registry plan; private status/count is separate from pushed events or the pull result slot. See the [detailed process path](#process-endpoint-private-result-and-callback-event-are-separate). | Configured-process `HLA_EVOKED`/pull and `HLA_IMMEDIATE`/push cases cover competing requests for one unowned attribute and a mixed request with one available and one still-owned attribute. They assert per-attribute acquisition/unavailable outcomes without requiring a universal winner or callback order: [competing requests](../../cpp/tests/attribute_ownership_query_catch2.cpp#L1818), [mixed attributes](../../cpp/tests/attribute_ownership_query_catch2.cpp#L2144). |
| Confirm Divestiture | Explicit request and registry plan; private status/count is separate from queued recipient notifications. See [negotiated divestiture and confirmation](#negotiated-divestiture-adds-a-confirmation-stage). | The focused process-boundary case observes selected callbacks and tags under `HLA_IMMEDIATE`, not update-region behavior or an ownership-query timing guarantee. |
| Unconditional Divestiture | Explicit request and registry plan; private acceptance is separate from acquisition work and assumption offers. See the [process route below](#process-endpoint-unconditional-divestiture-and-recipient-work-are-separate). | Focused process cases cover selected successful outcomes: owner-to-unowned query after divestiture, a later explicit acquisition/notification, and ownership-assumption offers under HLA_EVOKED and HLA_IMMEDIATE. They do not cover a regular request already pending at divestiture or the post-commit enqueue/send failure path. The portable C++ TCK's general scenario is not recorded as a process run. |
| Acquisition cancellation | Explicit request; its private acceptance result is distinct from the later confirmation event. See [process cancellation](#process-endpoint-cancellation-result-and-confirmation-are-separate). | Configured-process `HLA_EVOKED`/pull covers confirmation after ordinary cancellation; `HLA_IMMEDIATE`/push covers an idle requester and a buffered owner-release callback. No focused process test combines cancellation with a divestiture-generated acquisition notification or Release Denied after owner-callback entry; deletion/Resign outcomes also remain gaps. |
| Divestiture If Wanted | No process operation, client method, or server handler was found; the public method directly uses the embedded registry, as described above. | The pinned API declares the service, but the selected transfer/region tests exercise only the embedded route. |

### Object deletion uses a special privilege, not ordinary data ownership

The 2025 standard MIM declares [`HLAprivilegeToDeleteObject` on
`HLAobjectRoot`](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L33),
and the pinned 2025 [`deleteObjectInstance` API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L824)
lists `DeletePrivilegeNotHeld`. Do not infer object-deletion authority from
owning an ordinary data attribute: the delete privilege is a distinct
ownership concern. The [object and interaction guide's delete/removal
flow](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#object-removal-delete-callback-and-per-receiver-cleanup)
covers the current implementation's special gate and the separate
receiver-removal lifecycle. That guide also explains how an explicit
unpublish can separate delete privilege across registrations in one class
([publication-epoch detail](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#a-publication-epoch-can-separate-delete-privilege-across-object-registrations)).
The existing acquisition and divestiture diagrams stay focused on per-attribute
transfer. The following sequence connects that transfer to deletion authority
without merging the two state changes.

#### Catalog inheritance and instance authority are different layers

The catalog answers whether an object-class definition exposes the attribute.
Its [effective-attribute lookup](../../cpp/src/internal/fom/fom_catalog.hpp#L229)
walks the class's parent chain and combines declared attributes. A focused
composer test builds the pinned 2025 standard MIM with the Restaurant FOM and
checks that every object class in that composed catalog resolves
`HLAprivilegeToDeleteObject` ([test](../../cpp/tests/libxml2_fom_composer_catch2.cpp#L788)).
A separate catalog test checks that `HLAmanager.HLAfederate` retains the
inherited row's `HLAtoken`, `valueRequired=false`, `DivestAcquire`, `Static`, and
`PublishSubscribe` metadata, distinct from the class's directly declared MOM
attributes ([test](../../cpp/tests/libxml2_fom_composer_catch2.cpp#L741)).
These establish class-schema availability and metadata—not an object instance,
an ownership transfer, or a federate's right to delete.

```mermaid
flowchart TB
  subgraph schema["Composed FOM class schema"]
    direction LR
    MIM["2025 MIM declares the privilege at HLAobjectRoot"] --> PARENTS["Effective lookup walks the class parent chain"]
    PARENTS --> AVAILABLE["Tested MIM plus Restaurant catalog exposes it on each composed class"]
  end
  subgraph runtime["One object instance: selected embedded path"]
    direction LR
    OBJECT["A particular registered object"] --> OWNER["Its current delete-privilege owner"]
    OWNER --> CHECK["Delete gate checks that instance's owner"]
  end
```

The two lanes are deliberately separate: an inherited attribute in the FOM
catalog neither assigns nor proves the owner of any particular object. The
runtime lane is bounded by the [embedded transfer-and-delete case](../../cpp/tests/object_instance_deletion_catch2.cpp#L235)
and the [per-instance registry gate](../../cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp#L44).
The composer cases and runtime test source were inspected but not rerun for
this documentation change; no configured-process parity is claimed.

### A new owner can gain delete authority without becoming the registrant

The [2025 MIM row for `HLAprivilegeToDeleteObject`](../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml#L33)
defines it as an object-class attribute,
and the 2025 [`deleteObjectInstance` API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L824)
lists `DeletePrivilegeNotHeld`. A selected embedded test scenario makes the
cross-service relationship concrete: A registers the object and initially
owns its delete privilege; B must publish that special attribute at the
object's known class before its acquisition request is eligible; A then
divests it if wanted. After the transfer, A cannot delete the instance, while
B can. The same test also transfers an ordinary attribute, showing that its
update authority is a separate per-attribute ownership result.

```mermaid
sequenceDiagram
    participant A as A Registrant
    participant B as B New owner
    participant R as RTI
    participant C as C Observer
    Note over B,R: B must publish HLAprivilegeToDeleteObject for this object class
    alt Special attribute is not published
        B->>R: attributeOwnershipAcquisition
        R-->>B: AttributeNotPublished
        Note over A,B: No transfer. A keeps the delete privilege
    else Special attribute is published
        B->>R: attributeOwnershipAcquisition
        A->>R: attributeOwnershipDivestitureIfWanted
        R-->>B: attributeOwnershipAcquisitionNotification
        Note over A,B: B now owns the delete privilege. A remains the registrant
        A->>R: deleteObjectInstance
        R-->>A: DeletePrivilegeNotHeld
        B->>R: deleteObjectInstance (tag from B)
        R-->>C: removeObjectInstance (producer A)
    end
    Note over A,C: This is the selected embedded HLA_EVOKED test path only
```

The current worktree contains the
[embedded transfer-and-delete test](../../cpp/tests/object_instance_deletion_catch2.cpp#L234),
with direct assertions for [A's `DeletePrivilegeNotHeld` and B's successful
delete](../../cpp/tests/object_instance_deletion_catch2.cpp#L353) and [the
removal callback naming A rather than B](../../cpp/tests/object_instance_deletion_catch2.cpp#L361),
plus the separate
[publication-eligibility test](../../cpp/tests/object_instance_deletion_catch2.cpp#L377).
The eligibility case keeps the object class published through ordinary
`Efficiency` while explicitly unpublishing only the delete privilege; its
acquisition then throws `AttributeNotPublished` until B publishes that
attribute at the known class ([rejection](../../cpp/tests/object_instance_deletion_catch2.cpp#L435),
[later successful acquisition](../../cpp/tests/object_instance_deletion_catch2.cpp#L454)).
Together the two test sources assert the outcomes shown above, including the
removal callback's original registrant identity and B's deletion tag. The
transfer test's source comment attributes the producer-identity rule to
§6.17, but the pinned callback header supplies only the signature and
`producingFederate` parameter, not the clause prose. The checked-in test proves
the expected identity in this embedded HLA_EVOKED scenario; it is not
independent verification of the normative clause. Consult an authorized copy
of IEEE 1516.1-2025 before presenting that exact attribution as verified
([official IEEE listing](https://standards.ieee.org/ieee/1516.1/6688/)).
These test-source assertions were not executed as part of this documentation
review. The embedded registry
checks the per-instance delete-privilege owner
([deletion gate](../../cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp#L41))
and retains the registrant in the removal snapshot
([callback snapshot](../../cpp/src/internal/federation/federation_registry_object_instance_removal.cpp#L108)).
The 2025 process removal path has a distinct identity boundary: its TSO
deletion message records the deleting requester's identity, while its public
removal event is built from the receiver's removal snapshot. Keep these values
separate:

| Process boundary | `producingFederateId` source and meaning |
| --- | --- |
| Receive-order removal event | The event gets the producer from the removal snapshot either while enqueueing pushed callbacks or when the receiver polls a pull-mode event ([push path](../../cpp/src/internal/federation/process_federation_service_object_instance_removal.cpp#L60), [pull path](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L149)). |
| TSO deletion message | `deletionRequest.federateId`, the federate making the deletion request ([message construction](../../cpp/src/internal/federation/process_federation_service_object_instance_deletion.cpp#L107)). |
| Removal snapshot | The object instance's stored registration producer ([receive-order snapshot](../../cpp/src/internal/federation/federation_registry_object_instance_removal.cpp#L106), [TSO snapshot](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L808)); registration records that producer when the object is created ([registration](../../cpp/src/internal/federation/federation_registry_object_instance_registration.cpp#L333)). |
| Public callback | The process removal event carries the snapshot identity into callback delivery, and the bridge converts it to the public `FederateHandle` ([receive-order event](../../cpp/src/internal/federation/process_federation_service_object_instance_removal.cpp#L92), [TSO event](../../cpp/src/internal/federation/process_federation_service_tso_payload_dispatch.cpp#L250), [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L651)). |

Thus current process source maps the original registrant into both receive-order
and timestamped `removeObjectInstance` callbacks, even though the internal TSO
deletion message records the deleting requester. This is source evidence, not a
process test with distinct registrant and delete-privilege owner. The focused
receive-order process fixture registers and deletes through the same sender and
asserts that removal producer matches discovery producer ([register](../../cpp/tests/ieee1516_2025_connection_delete_object_instance_catch2.cpp#L766),
[delete](../../cpp/tests/ieee1516_2025_connection_delete_object_instance_catch2.cpp#L779),
[callback assertion](../../cpp/tests/ieee1516_2025_connection_delete_object_instance_catch2.cpp#L789)); the timestamped
process fixture does likewise ([register](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1053),
[delete](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1123),
[callback assertion](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1158));
neither test can distinguish the two identities. The transfer-and-delete
assertion above is specifically the embedded `HLA_EVOKED` test path; do not
generalize its runtime coverage to process delivery.

The distinction is easy to miss because an internal TSO message names the
deleting requester, while the eventual removal callback uses the original
registrant from the receiver's removal snapshot. This source-only sequence
shows those identities separately; it does not claim that a focused process
test has exercised a transferred delete privilege.

```mermaid
sequenceDiagram
    participant A as A: original registrant
    participant B as B: current delete-privilege owner
    participant S as Configured-process service
    participant R as Shared registry
    participant C as Receiving federate
    participant Bridge as Callback bridge
    Note over A,B: Assume HLAprivilegeToDeleteObject has moved from A to B. Process transfer-and-delete is not tested
    B->>S: deleteObjectInstance(timestamp, tag)
    S->>S: TSO message records requester ID B
    S->>R: Enqueue timestamped deletion for eligible recipients
    Note over S,C: Later, when C reaches its delivery boundary
    S->>R: Commit C's TSO removal
    R-->>S: Removal snapshot returns original registrant ID A
    S->>Bridge: Process event carries producer ID A
    Bridge-->>C: removeObjectInstance(handle, tag, producer A)
    Note over S,C: Focused process tests use the same federate for registration and deletion. A != B remains source-only
```

The final public identity follows the snapshot path, not the TSO message's
requester field. That is the implementation trace shown here; only the
separate embedded transfer-and-delete case directly observes a distinct
registrant and deleting federate.

#### Configured-process deletion authorization: source path and test limit

The configured-process handler first validates its session/federate membership,
then separates receive-order from timestamped requests
([handler](../../cpp/src/internal/federation/process_federation_service_object_instance_deletion.cpp#L37)).
The receive-order path delegates to the registry deletion operation, whose
source resolves the privilege handle for the object's registered class and
compares its owner entry on that specific instance with the requester
([registry gate](../../cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp#L41)).
The timestamped path first uses the TSO deletion planner; it performs the same
instance-owner comparison and derives the effective order before the process
service continues to queue work
([TSO planner](../../cpp/src/internal/federation/federation_registry_time_tso_enqueue.cpp#L627)).
This is a source trace of the configured-process implementation, not a
normative rule or an observed post-transfer process result.

The focused receive-order process case covers a registrant registering and
deleting through the configured endpoint; the timestamped process cases cover
timestamped removal/delivery paths. Neither case transfers
`HLAprivilegeToDeleteObject` and then checks the former owner's rejection or
the new owner's success ([receive-order case](../../cpp/tests/ieee1516_2025_connection_delete_object_instance_catch2.cpp#L457),
[timestamped case](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1221)).
The inspected process cases contain no `DeletePrivilegeNotHeld` assertion.
Therefore the embedded transfer test remains the only focused runtime evidence
here for deletion after privilege transfer; configured-process transfer parity
is still unverified.

The current runtime entry points and callback dispatch are in
[ownership services](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_services.cpp)
and
[ownership callback dispatch](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.cpp).
The per-attribute registry paths are in
[regular acquisition](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp),
[If-Available acquisition](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition_if_available.cpp),
[negotiated divestiture](../../cpp/src/internal/federation/federation_registry_negotiated_attribute_ownership_divestiture.cpp),
and
[If-Wanted divestiture](../../cpp/src/internal/federation/federation_registry_attribute_ownership_divestiture_if_wanted.cpp).

## Requester-side state: request is not ownership

Read the regular acquisition path per attribute: the service can return while
the request is pending, and a callback later resolves it. Cancellation has
its own confirmation boundary. If Available follows a different callback-time
decision path, shown separately below. This selected embedded model is not a
complete normative state machine for every ownership service or profile.

```mermaid
flowchart LR
  Start["Regular acquisition accepted"] --> Pending["Acquisition pending"]
  Pending -->|Notification begins| Own["Requester owns attribute<br/>in the tested callback path"]
  Pending -->|Unavailable begins| Unavailable["No transfer; current owner retains"]
  Pending -->|Cancel service accepted| Cancelling["Await cancellation confirmation"]
  Cancelling -->|Confirmation begins| Cancelled["Request ends; ownership unchanged"]
  Cancelling -->|Object deleted first| Suppressed["Object invalidated; queued callback suppressed"]
```

### Cancel Acquisition is pending until its confirmation callback

The 2025 API separates `cancelAttributeOwnershipAcquisition` (§7.15) from the
`confirmAttributeOwnershipAcquisitionCancellation` callback (§7.16). The
service returns `void`; in the focused embedded case, its return is not the
terminal cancellation observation. Until the confirmation callback begins,
the request still holds the publication fence and blocks an If-Available
request for the same attribute. At confirmation, the pending request is
removed, the queued owner-release callback is stale, and no acquisition
notification is reported. Ownership itself has not changed. The same test
shows that this service does not cancel an If-Available request: it reports
`AttributeAcquisitionWasNotRequested`; a later regular request supersedes
that If-Available work in the exercised scenario. Receive-order object
removal can invalidate a queued cancellation confirmation before application
code observes it.

See the pinned [Cancel Attribute Ownership Acquisition declaration](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1127),
its [confirmation callback](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L456),
and the focused [2025 embedded cancellation-boundary case](../../cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp#L199).

The important callback boundary is that the accepted request alone does not
make isAttributeOwnedByFederate true. In the focused 2025 tests, the requester's
ownership query changes when the acquisition-notification callback is
dispatched. With If Available, the dispatcher rechecks the request and current
ownership before selecting the notification or unavailable outcome. Thus, if
an owner divests before that callback is dispatched, a request that would
otherwise be unavailable can instead acquire the now-unowned attribute. See
the implementation's
[begin-acquisition callback path](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition_if_available.cpp)
and the tests below.

### Process endpoint: cancellation result and confirmation are separate

The pinned API labels cancellation §7.15 and its confirmation callback §7.16.
The process endpoint keeps the same public `void` operation/callback distinction
but adds a private status/count result. The sequence below is the configured
`HLA_EVOKED` pull route covered by the focused process case, not a
cross-profile timing rule.

```mermaid
sequenceDiagram
  autonumber
  actor B as Requesting federate
  participant RTI as 2025 process endpoint

  B->>RTI: Request acquisition
  B->>RTI: Cancel pending request
  RTI->>RTI: Reserve cancellation, queue confirmation
  RTI-->>B: Return private status/count
  Note over B,RTI: Public call is void — confirmation callback has not run yet
  B->>RTI: Dispatch callback queue
  RTI->>RTI: Poll process event queue
  RTI->>RTI: Consume reservation, clear canceled set
  RTI-->>B: Confirm acquisition cancellation
  Note over B,RTI: No acquisition callback observed in this test
```

Here, the public calls are `attributeOwnershipAcquisition`,
`cancelAttributeOwnershipAcquisition`, and `evokeMultipleCallbacks`; the
confirmation event is `confirmAttributeOwnershipAcquisitionCancellation`. The
private status/count and process-event pull are Umbra boundary details, not
HLA ownership outcomes.

The [configured-process HLA_EVOKED case](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1139)
uses one regular-acquisition attribute. It checks that cancellation returns
normally with no confirmation report yet, then requester callback evocation
yields the expected object and attribute confirmation and no acquisition
callback. It does not poll the owner after the cancel call, so it does not
prove suppression of a queued owner-release callback on the pull route.

Two focused [HLA_IMMEDIATE push cases](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L175)
extend the process evidence: one confirms cancellation while the requester is
idle; the [buffered-release case](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L434)
holds the owner inside a Provide callback while the pushed owner-release event
is buffered, then observes cancellation confirmation and suppression of that
release callback after the owner unblocks. These events are owner-side release
requests, not acquisition notifications created when Unconditional Divestiture
exposes an unowned attribute. No focused process test combines that
divestiture-generated notification with cancellation. The selected cases also
do not cover empty sets, the after-entry Release-Denied/cancellation race, or the
deletion/Resign lifecycle branches below.

Umbra source observations: default pull mode commits the reservation when the
queued confirmation reaches the `receive_interaction` delivery gate; configured
push mode begins the transition inside the service handler before sending the
unsolicited event. The registry leaves the acquisition pending when the cancel
service is accepted, suppresses new work for the reserved attributes, and
removes them at confirmation delivery. If an owner-release callback has
already entered user code, it may still answer. The available source does not
establish that Release Denied always wins: the denial planner and
cancellation-confirmation begin gate can commit in either order, with different
requester events. See the [source-only Release Denied race](#release-denied-vs-cancellation-after-owner-callback-entry);
no focused process test exercises it. An empty set is currently accepted as
applied with no callback reservation in source, but that behavior is untested
here and is not presented as a normative rule.

#### Cancellation vs a divest-triggered acquisition notification

This combined race is source-derived, not tested as a configured-process
scenario. The pending-work planner skips an attribute that already has a
cancellation reservation. If notification work is already queued, the
cancellation plan and notification-begin gate serialize on registry state:
when cancellation planning wins, it reserves the attribute but leaves the
acquisition pending, so a later notification-begin gate suppresses transfer.
When notification begin wins, it assigns ownership and removes the attribute
from the pending request; a subsequent cancellation plan is rejected because
the requester already owns the attribute. The accepted cancellation is then
consumed at a separate begin gate. Configured-process timing differs by mode:
push begins cancellation in the service handler before sending its confirmation
frame, while pull queues the confirmation and begins cancellation when the
recipient polls. In either mode, the notification gate revalidates registry
state; a queued event is not itself a transfer. These are Umbra ordering
points, not an HLA callback-order or race-priority rule. See the
[pending-work planner](../../cpp/src/internal/federation/federation_registry_attribute_ownership_pending_work.cpp#L261),
[cancellation reservation](../../cpp/src/internal/federation/federation_registry_attribute_ownership_cancellation.cpp#L15),
[notification begin gate](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp#L209),
[process cancellation handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L396),
[push notification begin point](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L87),
[pull notification begin point](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L400),
and [pull cancellation begin point](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L432).

```mermaid
flowchart TD
  Divest["Owner divests while a regular request is pending"] --> Remove["Registry removes former owner"]
  Remove --> Planner{"Cancellation already reserved?"}
  Planner -->|Yes| Skip["No acquisition notification is scheduled"]
  Planner -->|No| Scheduled["Schedule acquisition-notification work"]
  Scheduled --> Race{"Which registry transition wins first?"}
  Cancel["Requester attempts cancellation"] --> Race
  Race -->|Notification begin first| Acquire["Assign requester as owner and remove attribute from pending request"]
  Acquire --> Callback["Project attributeOwnershipAcquisitionNotification"]
  Acquire --> TooLate["Later cancellation plan is rejected: requester already owns attribute"]
  Race -->|Cancellation plan first| Reserved["Accept cancellation and reserve attribute; request remains pending"]
  Reserved --> NoticeGate["Queued notification begin rechecks registry state"]
  NoticeGate --> Suppress["Reservation or consumed request suppresses this notification"]
  Reserved --> Mode{"Configured-process delivery mode"}
  Skip --> Mode
  Mode -->|Push| PushBegin["Begin cancellation in service handler before confirmation send"]
  Mode -->|Pull| PullQueued["Queue confirmation; reservation remains until receive poll"]
  PullQueued --> PullBegin["Begin cancellation at recipient receive poll"]
  PushBegin --> Confirm["Consume cancellation and remove attribute from pending request"]
  PullBegin --> Confirm
```

The configured-process [pull cancellation case](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1139)
and [push cancellation case](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L175)
exercise accepted cancellation without Unconditional Divestiture. The configured-process
divestiture cases exercise divestiture without cancellation, while the embedded
2025 divestiture case with pending requests does not add cancellation. A
separate [embedded assumption-search case](../../cpp/tests/ieee1516_2025_embedded_ownership_assumption_search_continuation_catch2.cpp#L71)
does cancel before unconditional divestiture and verifies that the requester
becomes eligible for an ownership-assumption callback only after its
cancellation confirmation. That is adjacent evidence about assumption-search
eligibility, not the configured-process queued-notification race. No focused
process test verifies either outcome in this combined state machine; its gates
and push/pull distinction are source-derived.

Before treating that sequence as guaranteed, account for lifecycle invalidation
before callback dispatch. This is a source-derived 2025 process pull-path view,
not an HLA rule, and neither branch has a focused process-boundary test:

```mermaid
flowchart TD
  Queued["Accepted cancellation confirmation<br/>awaits process pull dispatch"] --> Before{"Before callback delivery,<br/>which path occurs first?"}
  Before -->|Requester resigns| Cleanup["Remove queued events naming the departed federate<br/>and clear that session's ownership event queue"]
  Cleanup --> Suppressed["No callback from this queued event"]
  Before -->|Receive gate runs| Gate["receive_interaction revalidates<br/>cancellation ID and current state"]
  Gate -->|Stale ID or object absent| NoDelivery["No delivery returned"]
  Gate -->|Deletion accepted or requester no longer eligible| Empty["Consume cancellation; return no confirmed attributes"]
  NoDelivery --> Skip["Receive loop skips the callback event"]
  Empty --> Skip
  Gate -->|Still eligible| Callback["Confirmed attributes may proceed<br/>to the callback bridge"]
```

On explicit requester Resign, `handleResign` first commits the registry result,
then connection-loss cleanup removes queued acquisition events in that
federation whose requester or recipient is the departing member; session cleanup
also clears the resigning session's remaining ownership events. If pull receive
reaches the event after object state changes, a missing object or stale
cancellation returns no delivery. If deletion is accepted, or the requester is
no longer a member or no longer knows the object class, the registry consumes
the cancellation but returns no confirmed attributes (or no delivery when no
follow-up work remains). The receive loop drops either stale or empty event
before callback projection.

The configured-process success case does not interleave deletion or Resign
between cancellation and receive. The embedded test first observes one
confirmation, then issues a second cancellation followed by object deletion;
the report count remains one while the removal callback arrives, so the second
confirmation is suppressed in that embedded scenario only. The standalone
Resign test covers pending acquisition work and owner-release callback
suppression, not a pending cancellation confirmation. Neither adjacent case is
process-boundary evidence.

Evidence: pinned [§7.15 API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1127)
and [§7.16 callback](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L456);
2025 process [ambassador](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_services.cpp#L1598),
[service handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L376),
[registry reservation/delivery](../../cpp/src/internal/federation/federation_registry_attribute_ownership_cancellation.cpp#L15),
[pull callback gate](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L432),
and [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L546).
Lifecycle sources: [explicit Resign handler](../../cpp/src/internal/federation/process_federation_service_request_handlers.cpp#L160),
[departed-member event cleanup](../../cpp/src/internal/federation/process_federation_service_connection_loss.cpp#L25),
[joined-session queue cleanup](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L28),
[registry cancellation recheck](../../cpp/src/internal/federation/federation_registry_attribute_ownership_cancellation.cpp#L136),
[registry ineligible-state branch](../../cpp/src/internal/federation/federation_registry_attribute_ownership_cancellation.cpp#L194),
[accepted-deletion state](../../cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp#L117),
and [receive-loop empty-event skip](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L496).
Adjacent tests: [embedded deletion case](../../cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp#L199)
and [standalone pending-acquisition Resign case](../../cpp/tests/resign_action_cancel_pending_acquisition_catch2.cpp#L96);
focused [process case](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1136).

### Push delivery commits before the client can dispatch

Push mode crosses a different boundary from the pull path. The service consumes
the cancellation reservation before sending an unsolicited event frame. The
client buffers that frame while waiting for the cancel response, then submits
it to the callback bridge before the client request returns. `HLA_IMMEDIATE`
may invoke the callback before the public cancel call returns; `HLA_EVOKED`
leaves a task in the local callback dispatcher. Once submitted, the callback
task is not rechecked against the registry. Object deletion therefore cannot
invalidate that already-sent confirmation in the inspected push path, and the
process Resign path returns without resetting the local callback dispatcher.
Current source consequently leaves an evoked confirmation available for a
later `evokeMultipleCallbacks`, even if deletion or requester Resign happens
first. This is source-derived 2025 process behavior, not a portable HLA rule.

There is also a send-failure seam: the registry transition occurs before
`session.send`; if that send fails, the handler returns its internal-error path
after the cancellation has already been consumed, with no confirmation frame
sent. The test suite inspected has only the default-pull process cancellation
case; neither this send-failure branch nor push-mode callback ordering after
deletion/Resign is process-tested.

```mermaid
flowchart TD
  Cancel["Valid cancellation plan"] --> Commit["Push path consumes reservation<br/>before sending"]
  Commit --> Send{"Unsolicited event send succeeds?"}
  Send -->|No| SendError["Handler returns internal error after commit<br/>No confirmation frame is sent"]
  Send -->|Yes| Frame["Client buffers frame while waiting<br/>for the cancel response"]
  Frame --> Bridge["Client submits buffered event to callback bridge<br/>before its request returns"]
  Bridge --> Model{"Callback model"}
  Model -->|HLA_IMMEDIATE| Immediate["Dispatcher may invoke before<br/>public cancel returns"]
  Model -->|HLA_EVOKED| Queued["Callback task waits in local dispatcher"]
  Queued --> Later{"Before a later evoke"}
  Later -->|Object deleted| DeletePath["No client-side registry recheck<br/>for this submitted task"]
  Later -->|Requester Resigns| ResignPath["Process Resign path does not<br/>reset the callback dispatcher"]
  DeletePath --> Retained["Current source leaves callback available"]
  ResignPath --> Retained
  Retained --> Evoke["A later evoke may call the confirmation callback"]
```

Push-path evidence: [server commit-before-send](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L435),
[unsolicited frame send](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L449),
[client frame buffering](../../cpp/src/internal/federation/process_federation_client.cpp#L427),
[request dispatch before return](../../cpp/src/internal/federation/process_federation_client.cpp#L504),
[callback-queue handoff](../../cpp/src/internal/federation/process_federation_client_callback_dispatch.cpp#L265),
[callback bridge submission](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1390),
[immediate/evoked dispatcher behavior](../../cpp/src/internal/callbacks/callback_dispatcher.cpp#L122),
and [process Resign branch](../../cpp/src/internal/runtime/umbra_rti_ambassador_federation_lifecycle.cpp#L785).
The focused public process cancellation evidence includes the
[default-pull HLA_EVOKED test](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1136),
[HLA_IMMEDIATE idle push case](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L175),
and [buffered-release push case](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L434).
The buffered-release case checks suppression before owner callback entry, not
the after-entry Release Denied race. No 2010 behavior is inferred.

### Regular acquisition with an owner response

```mermaid
sequenceDiagram
  autonumber
  actor B as Requesting federate
  participant RTI as 2025 RTI
  actor A as Current owner

  B->>RTI: Request regular acquisition (attributes, tag)
  RTI-->>B: Call accepted, attributes become pending
  RTI-->>A: Request release of owned subset (acquisition tag)
  Note over B,RTI: Unowned attributes in the set can resolve independently
  RTI-->>B: Notify acquisition of an eligible unowned attribute
  Note over B: Ownership becomes true at callback dispatch in the tested profile
  alt A denies release
    A->>RTI: Deny release of selected attributes (denial tag)
    RTI-->>B: Report denied attributes as unavailable (denial tag)
    Note over A: A remains owner of the denied attributes
  else A divests through an applicable transfer path
    Note over A,RTI: Unconditional and negotiated paths produce different callbacks
    RTI-->>B: Notify acquisition
    Note over B: Ownership query is true at callback dispatch in the tested profile
  end
```

The shortened labels correspond to `attributeOwnershipAcquisition`, the
`requestAttributeOwnershipRelease` callback, the owner's
`attributeOwnershipReleaseDenied` response, and the requester's
`attributeOwnershipUnavailable` outcome.

This diagram summarizes only the paths represented by the cited tests. It
does not claim a particular arbitration order among multiple requesters or
cover every legal owner response.

The case
[Embedded Attribute Ownership Acquisition honors 2025 release and denial callbacks](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_release_callbacks_catch2.cpp)
demonstrates several easily missed details:

- The request includes both an owner-held attribute and an unowned attribute;
  the unowned attribute can notify and transfer while the owner-held one stays
  pending.
- A regular request can supersede pending If-Available work in this scenario;
  stale queued work is suppressed rather than becoming a second public result.
- Repeating an already-pending regular request does not queue another owner
  release callback in this tested profile.
- The owner receives the requester's acquisition tag with the release request.
  If the owner denies release, the requester receives the denial call's tag in
  its Unavailable callback, and the owner retains ownership.
- Pending acquisition constrains unpublication in this scenario; publication
  is not merely an unrelated declaration detail.

### If Available: evaluate at the result callback

```mermaid
sequenceDiagram
  autonumber
  actor B as Requesting federate
  participant RTI as 2025 RTI
  actor A as Current owner

  B->>RTI: Request If-Available acquisition (attributes, tag)
  RTI-->>B: Call accepted, work remains pending
  Note over RTI: Known class and requester publication are required
  alt Attribute is unowned at the result-time recheck
    RTI->>RTI: Revalidate request and transfer eligible attributes
    RTI-->>B: Notify acquisition (attributes, request tag)
  else Attribute is still owned by A
    RTI-->>B: Report attributes unavailable (request tag)
  else A divests before callback-time recheck
    A->>RTI: Divestiture makes the attribute available
    RTI->>RTI: Revalidate pending If-Available work
    RTI-->>B: Notify acquisition (request tag)
  end
```

The branches are outcomes at the embedded profile's callback-time recheck, not
a promise that an owned attribute will be held indefinitely for the requester.
The focused [embedded If-Available test](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_if_available_catch2.cpp#L112)
shows a mixed attribute set resolving into both Notification and Unavailable
callbacks. It also exercises repeated pending requests and the publication
guard against withdrawing a declaration needed by pending work.

### Process endpoint: private result and callback event are separate

The 2025 process endpoint adds an Umbra transport boundary around this HLA
service. The source-derived view below is not a second normative state machine:

```mermaid
flowchart TD
  Request[Public attributeOwnershipAcquisitionIfAvailable call] --> Plan[Validate and plan request]
  Plan --> PrivateResult[Private correlated result carries status and recipient count]
  PrivateResult -->|Applied| Return[Public void call returns normally]
  PrivateResult -->|Failure status| Exception[Ambassador throws typed public exception]
  Plan --> Mode{Process event delivery mode}
  Mode -->|Push enabled| PushResolve[Server begins request and forms secured/unavailable sets]
  PushResolve --> PushFrame[Send event frame to requester]
  Mode -->|Pull mode| Queue[Queue the request ID]
  Queue --> Poll[Later receive_interaction begins request]
  Poll --> PollFrame[Receive response carries secured/unavailable sets]
  PushFrame --> Bridge[Client callback bridge projects the result sets]
  PollFrame --> Bridge
  Bridge -->|Secured set is nonempty| Notification[attributeOwnershipAcquisitionNotification]
  Bridge -->|Unavailable set is nonempty| Unavailable[attributeOwnershipUnavailable]
```

The public [§7.9 signature](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1061)
is `void`. Umbra maps a non-applied private status to a typed API exception;
the private recipient count is not an ownership result. At the server boundary,
push mode resolves the request before sending its event frame, while pull mode
queues its request ID and resolves it during a later `receive_interaction`
([handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L211),
[poll path](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L468)).
The [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L452)
converts the disjoint result sets to the corresponding public callbacks. These
are implementation scheduling details; callback model and event transport are
separate, and this diagram makes no portable callback-before-return claim.

The focused [configured-process case](../../cpp/tests/attribute_ownership_acquisition_if_available_catch2.cpp#L376)
uses `HLA_EVOKED`, the default pull-event configuration, and one unowned
attribute. It observes the acquisition notification and its tag after callback
evocation, but does not query ownership or cover an owned/unavailable member,
a mixed result, or pushed events. A separate configured-process
[Release Denied case](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1117)
tests `attributeOwnershipUnavailable` after regular acquisition; its
[HLA_IMMEDIATE companion](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1126)
does likewise under push delivery. Neither is If-Available or cancellation
race evidence. Likewise, the process
[acquisition-cancellation case](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1136)
starts with regular acquisition, not §7.9. Do not use either to claim process
coverage of If-Available unavailable or cancellation outcomes. The embedded
mixed-outcome and ownership-query assertions remain a separate evidence lane.

### Release Denied in the process profile: service result versus callback

The HLA-level branch above pairs the owner's §7.12
`attributeOwnershipReleaseDenied` service with the requester's §7.10
`attributeOwnershipUnavailable` callback. This sequence adds the 2025 process
profile's two-client transport boundary; it is not a portable callback-timing
rule.

```mermaid
sequenceDiagram
  autonumber
  actor B as Requesting federate
  participant RTI as 2025 process endpoint
  actor A as Current owner

  B->>RTI: attributeOwnershipAcquisition(attribute, acquisition tag)
  Note over RTI,A: Request remains pending while the current owner is asked to release
  A->>RTI: evokeMultipleCallbacks()
  RTI->>RTI: receive_interaction pull poll
  RTI-->>A: requestAttributeOwnershipRelease(attribute, acquisition tag)
  A->>RTI: attributeOwnershipReleaseDenied(attribute, denial tag)
  RTI->>RTI: Mark pending attribute unavailable and queue recipient event
  RTI-->>A: Private status and recipient count
  Note over A,RTI: Public operation is void — a non-applied status maps to a typed exception
  B->>RTI: evokeMultipleCallbacks()
  RTI->>RTI: receive_interaction pull poll
  RTI->>RTI: Revalidate recipient, object knowledge, and pending unavailable state
  alt Still deliverable
    RTI-->>B: attributeOwnershipUnavailable(attribute, denial tag)
  else No longer deliverable
    RTI->>RTI: Drop stale event and continue receive search
  end
```

In the focused configured-process [HLA_EVOKED test](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1117),
the owner's call returns normally while the requester's report list is still
empty. The requester later observes one Unavailable report with the expected
object, attribute, and owner-supplied denial tag, then queries and confirms that
the original owner still owns the attribute [in this assertion](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1067).
The [HLA_IMMEDIATE companion](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1126)
checks the pushed route with the same assertions. Both use one requester and
one owned attribute; neither tests cancellation racing the response, multiple
requesters, or stale delivery.

Umbra-specific source note: the process handler returns a private status and
recipient count separately from the queued callback event. The registry groups
denied attributes by pending requester, replans remaining acquisition work,
and rechecks the requester, known object/class, and pending unavailable marker
before exposing an event. Cancellation acceptance records a reservation but
leaves the attribute in the pending acquisition. The Release Denied planner
marks it unavailable and consumes a matching reservation if one already
exists; until the later Unavailable gate removes the pending attribute, a
cancellation may still be accepted. Therefore “whichever plan commits first”
is not a complete race rule. The cancellation-confirmation gate removes the
pending attribute if it commits first; otherwise, the Unavailable-recipient
gate rechecks its marker, consumes any matching reservation, and removes the
pending attribute. These are configured-process source observations, not a
portable HLA ordering claim.

#### Release Denied vs cancellation after owner callback entry

This is distinct from cancellation suppressing a release event before the
owner callback enters user code. In configured push mode, the server performs
an initial eligibility check before sending the event, then the callback bridge
claims it again immediately before invoking the public callback; a pushed frame
can be suppressed between those checks. Pull mode makes the owner-release
claim while servicing the owner's receive poll. Once the callback is in user
code, its Release Denied response can overlap the requester's cancellation
plan and the later delivery gates:

```mermaid
flowchart TD
  Start["Owner release callback is in user code;<br/>regular acquisition remains pending"] --> Order{"Which plan happens first?"}
  Order -->|"Cancellation plan first"| Reservation["Accept cancellation;<br/>pending request remains"]
  Reservation --> FirstGate{"Which registry transition follows?"}
  FirstGate -->|"Cancellation begin gate"| CancelBeforeDenial["Consume reservation;<br/>remove pending attribute"]
  CancelBeforeDenial --> ConfirmOnly["Cancellation confirmation may be exposed"]
  CancelBeforeDenial --> DenialNoRecipient["Later Release Denied plan finds no pending recipient;<br/>no Unavailable event is queued"]
  FirstGate -->|"Release Denied plan"| DenialWithReservation["Mark unavailable;<br/>consume the existing reservation"]
  DenialWithReservation --> UnavailableGate["Unavailable gate rechecks marker,<br/>consumes any reservation, and removes pending attribute"]
  DenialWithReservation -.-> CancelStale["Queued cancellation gate<br/>has no reservation"]
  Order -->|"Release Denied plan first"| DenialWithoutReservation["Mark unavailable;<br/>leave the desired attribute pending"]
  DenialWithoutReservation --> LaterCancel{"Cancellation accepted<br/>before Unavailable gate?"}
  LaterCancel -->|No| UnavailableGate
  LaterCancel -->|Yes| DeliveryOrder{"Which delivery gate commits first?"}
  DeliveryOrder -->|"Cancellation begin gate"| CancelAfterDenial["Consume cancellation and<br/>remove pending attribute"]
  CancelAfterDenial --> Confirm["Cancellation confirmation may be exposed"]
  CancelAfterDenial -.-> UnavailableStale["Unavailable gate finds<br/>no pending marker"]
  DeliveryOrder -->|"Unavailable-recipient gate"| UnavailableGate
  UnavailableGate --> Unavailable["Unavailable may be exposed"]
  UnavailableGate -.-> CancelStaleAfter["Later cancellation gate finds<br/>no reservation"]
```

The chart distinguishes an additional source-derived order: the cancellation
confirmation gate can consume the pending request before the owner's later
Release Denied plan runs. In that case the planner finds no pending recipient;
the process handler returns an applied result with zero recipients and queues
no Unavailable event ([planner](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp#L493),
[handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L314)).
This differs from cancellation being accepted first while its confirmation is
still pending: a subsequent denial plan consumes that reservation and schedules
Unavailable work. No focused configured-process or embedded test exercises the
no-recipient ordering; the arranged embedded race below covers the latter
reservation-consumed branch, not this one.

For push release work, the [service's pre-send projection](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L103)
checks eligibility but retains the reservation. The [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1532)
then invokes the [client's release claim](../../cpp/src/internal/federation/process_federation_client_ownership.cpp#L18),
which reaches the service [claim handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L488)
and the registry [owner callback-entry gate](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp#L396)
immediately before public callback invocation. Pull performs that registry gate
at the owner's [receive poll](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L417).
The [cancellation plan](../../cpp/src/internal/federation/federation_registry_attribute_ownership_cancellation.cpp#L15)
records its reservation without removing the pending request; the [denial planner](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp#L493)
marks the pending attribute unavailable and consumes a reservation it finds.
Cancellation confirmation begins in the [push cancellation handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L439)
or at the [pull receive poll](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L432).
Unavailable delivery rechecks and consumes pending state in [push](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L458)
or [pull](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L371).
When both queues are present in pull mode, the receive handler checks the
Unavailable queue before the acquisition/cancellation queue
([queue order](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L367));
the next completed poll can therefore be explained from this route, but it is
not a normative priority rule.

The normal [pull denial test](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1117)
and [push denial test](../../cpp/tests/attribute_ownership_acquisition_catch2.cpp#L1126)
do not include cancellation. The [buffered-release cancellation test](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L434)
blocks the owner inside a different callback; the requester cancels while that
callback is blocked [while the requester waits](../../cpp/tests/ieee1516_2025_process_idle_immediate_ownership_cancellation_catch2.cpp#L640),
and the release event is buffered before owner-release callback entry,
cancellation is confirmed, and the later
release callback is suppressed. It does not test Release Denied after owner
callback entry. A separate [embedded 2025 race case](../../cpp/tests/ownership_acquisition_cancellation_race_catch2.cpp#L166)
holds the owner inside the release callback until requester cancellation has
completed, then calls Release Denied; it observes no cancellation-confirmation
callback, one Unavailable callback with the denial tag, and the original owner
still owning the attribute ([assertions](../../cpp/tests/ownership_acquisition_cancellation_race_catch2.cpp#L282)).
That is evidence for this arranged embedded schedule, not configured-process
behavior. No focused process test covers Release Denied after owner callback
entry or determines the process race outcome. Do not infer normative priority,
callback timing, or embedded/process parity.

Evidence locators: pinned [§7.12 service](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1066)
and [§7.10 callback](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L438);
process [owner handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L314),
[pull unavailable-event recheck](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L371),
and [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L603);
registry [unavailable recipient revalidation](../../cpp/src/internal/federation/federation_registry_attribute_ownership_cancellation.cpp#L220)
and [denial planning](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp#L492).

## Owner-side transfer paths

### Negotiated divestiture adds a confirmation stage

Negotiated divestiture is not the same sequence as denying a release request.
The owner initiates a negotiated divestiture, receives a
requestDivestitureConfirmation callback, and may confirm the transfer. In a
focused 2025 process-boundary scenario, the confirmation then leads to the
regular requester's acquisition-notification callback. A separate assumption
offer can be delivered to another candidate while that confirmation is
pending; the offer itself does not transfer ownership.

```mermaid
sequenceDiagram
  autonumber
  actor B as Regular requester
  participant RTI as 2025 RTI
  actor A as Current owner
  actor C as Other candidate

  B->>RTI: Request acquisition (attribute, acquisition tag)
  A->>RTI: Propose negotiated divestiture (divestiture tag)
  RTI-->>A: Ask owner to confirm (divestiture tag)
  par Independent assumption search
    RTI-->>C: Offer attributes for assumption (divestiture tag)
  and Owner confirmation
    A->>RTI: Confirm transfer (confirmation tag)
    RTI-->>B: Notify requester of acquisition (confirmation tag)
  end
```

The targeted
[Confirm Divestiture with a pushed assumption candidate](../../cpp/tests/ieee1516_2025_confirm_divestiture_process_assumption_catch2.cpp)
test demonstrates the separate callback paths and tag ownership. It is a
process-boundary test, not evidence that every deployed transport or profile
supports the same end-to-end scope.

Keep the process request result separate from the HLA ownership outcome. The
pinned 2025 `attributeOwnershipAcquisition`,
`negotiatedAttributeOwnershipDivestiture`, and `confirmDivestiture` operations
return `void` and declare synchronous typed exceptions
([§7.8](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1042),
[§7.5](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1006),
[§7.6](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1024)).
Internally, the process service returns a correlated status/count to finish each
request, while ownership callback work is queued separately
([acquisition](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L156),
[negotiated divestiture and confirmation](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L545)).
That private result is not a public ownership value or a substitute for the
per-attribute callback. In the linked `HLA_IMMEDIATE` test, the requester's
acquisition notification is dispatched during its next ordinary lookup, before
that lookup returns—not during the owner's earlier confirmation call. The
focused assertions verify the callback kind, target attribute, and confirmation
tag; they do not add process-path `isAttributeOwnedByFederate` evidence. Keep
ownership-query timing claims scoped to the selected cases in
[Query Attribute Ownership](#query-attribute-ownership-reports-a-snapshot-it-does-not-transfer).

#### A requester can cancel after the owner receives confirmation

The successful process-boundary path above is not the only way a negotiated
divestiture can end. A separate focused embedded 2025 case delivers
`requestDivestitureConfirmation`, then accepts the requester's
`cancelAttributeOwnershipAcquisition` before the owner calls
`confirmDivestiture`. The pinned service declaration lists
`NoAcquisitionPending` for that confirmation call (§7.6). In the selected test,
the owner still owns the attribute, the requester does not, and the owner can
clear the negotiated divestiture; the requester later observes its cancellation
confirmation when its callbacks are drained. This is one embedded service
ordering, not process-profile parity or a general callback-timing rule.

```mermaid
sequenceDiagram
  autonumber
  actor B as Requesting federate
  participant RTI as 2025 embedded RTI
  actor A as Current owner

  B->>RTI: attributeOwnershipAcquisition(attribute, acquisition tag)
  A->>RTI: negotiatedAttributeOwnershipDivestiture(attribute, divestiture tag)
  RTI-->>A: requestDivestitureConfirmation(attribute, divestiture tag)
  B->>RTI: cancelAttributeOwnershipAcquisition(attribute)
  Note over B,RTI: Cancellation confirmation has not yet been observed by the requester
  A->>RTI: confirmDivestiture(attribute, confirmation tag)
  RTI-->>A: Throws NoAcquisitionPending
  Note over A: Ownership remains with the current owner
  A->>RTI: cancelNegotiatedAttributeOwnershipDivestiture(attribute)
  B->>RTI: Evoke pending requester callbacks later
  RTI-->>B: confirmAttributeOwnershipAcquisitionCancellation(attribute)
```

The [pinned Confirm Divestiture signature](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1013)
declares the exception. The [focused embedded case](../../cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp#L527)
checks cancellation after the confirmation callback, the typed exception, both
ownership queries, and cleanup; the [registry transition](../../cpp/src/internal/federation/federation_registry_negotiated_attribute_ownership_divestiture.cpp#L354)
retains ownership when it finds no active acquirer. Keep these claims scoped to
the API surface and selected embedded source/test evidence.

### Unconditional divestiture and assumption offers

In the exercised embedded scenario, unconditional divestiture first removes
the old owner's ownership. Pending regular and If-Available requesters may
then receive their own acquisition notification. The RTI can also search for
eligible assumption candidates and send requestAttributeOwnershipAssumption.
That callback is an **offer**, not an ownership transfer: the candidate remains
non-owner until its later acquisition request resolves through an acquisition
notification.

```mermaid
flowchart TD
  A[Owner calls unconditional divestiture] --> B[Owner no longer owns those attributes in the tested profile]
  B --> C{Pending requester or eligible assumption candidate?}
  C -->|Regular acquisition pending| D[Re-evaluate regular acquisition work]
  C -->|If Available pending| E[Recheck availability when result callback begins]
  C -->|Eligible and published candidate| F[Queue ownership-assumption offer]
  D --> G[Acquisition notification or unavailable outcome]
  E --> G
  F --> H[Candidate receives ownership-assumption callback]
  H --> I{Candidate later requests acquisition?}
  I -->|Yes and still eligible| J[Acquisition notification commits ownership]
  I -->|No| K[Offer alone changes no ownership]
```

The focused
[Unconditional Attribute Ownership Divestiture offers eligible 2025 federates](../../cpp/tests/ieee1516_2025_unconditional_attribute_ownership_divestiture_catch2.cpp)
queues both a regular acquisition and an If-Available request before the owner
divests. It later observes a separate acquisition notification for each,
checks each original request tag, and verifies the corresponding requester
owns its attribute. This directly covers the already-pending regular request
in the embedded profile; it does not establish the configured-process path.
The same case shows that a candidate that stopped publishing before callback
dispatch gets no stale offer, and that an assumption recipient remains a
non-owner until it separately requests acquisition. The search can continue
after stale work is suppressed; a candidate that republishes can become
eligible for a fresh offer.

#### A transferred attribute starts a fresh assumption-search interval

The 2025 embedded implementation keeps the assumption-search ledger per
unowned interval, rather than treating an earlier offer as a permanent
"already offered" mark. When unconditional divestiture starts a new interval,
the registry clears that attribute's old candidate set, saved tag, and queued
assumption-callback entries before recording the new tag and searching again
([reset helper](../../cpp/src/internal/federation/federation_registry_attribute_ownership_pending_work.cpp#L27),
[new interval](../../cpp/src/internal/federation/federation_registry_attribute_ownership_assumptions.cpp#L77)).

The focused [search-epoch case](../../cpp/tests/ieee1516_2025_embedded_ownership_assumption_search_epoch_catch2.cpp#L97)
offers an attribute to two candidates; one candidate acquires it, the other
leaves its earlier offer unanswered and becomes ineligible. The new owner then
divests with a different tag. After the second candidate republishes, it
receives a second offer, and the test checks that this fresh offer carries the
new tag ([assertions](../../cpp/tests/ieee1516_2025_embedded_ownership_assumption_search_epoch_catch2.cpp#L107)).
The test does not compare the tag on the first offer. This is a verified
embedded example of a fresh search interval, not a fairness or callback-timing
rule; no configured-process test establishes the same sequence.

```mermaid
flowchart TD
  First["Owner unconditionally divests with tag T1"] --> Offers["Eligible candidates receive assumption offers"]
  Offers --> Accepted["Candidate A requests and acquires the attribute"]
  Offers --> OldOffer["Candidate B leaves its first offer unanswered"]
  Accepted --> NewOwner["Candidate A becomes the owner"]
  OldOffer --> Eligibility["Candidate B becomes ineligible before the next divestiture"]
  NewOwner --> Second["New owner unconditionally divests with tag T2"]
  Eligibility --> Second
  Second --> Reset["Registry clears old search records and seeds a fresh interval"]
  Reset --> Republish["Candidate B republishes"]
  Republish --> FreshOffer["Candidate B receives a second offer carrying T2"]
```

### Process endpoint: Unconditional Divestiture and recipient work are separate

The 2025 process route sends the public request to the process client and
service, where the registry plan removes the previous owner and returns
follow-up acquisition work plus eligible assumption recipients. The service
then projects those recipient events using its configured push or pull event
transport and returns a private accepted Boolean; the public service itself is
`void`. That Boolean does not identify who acquired an attribute or prove that
any callback has entered application code. The receiving callback model is a
separate dispatch choice, and no callback-before-return order is claimed.

One source-only failure edge matters: registry state is committed before the
service attempts all recipient event enqueue/send work. If that work reports
failure, the handler returns a private internal error that the public adapter
maps to `RTIinternalError`; the inspected path shows no rollback of the already
committed registry transition. No focused process test covers this failure.
If-Available results also use their own request/result slot and are not folded
into the ordinary acquisition-work event queue.

```mermaid
flowchart TD
  Owner["Owner calls unconditionalAttributeOwnershipDivestiture"] --> Adapter["2025 adapter sends operation, handles, tag, and callback-enabled state"]
  Adapter --> Client["Process client sends the private request"]
  Client --> Handler["Service validates the session and calls the registry plan"]
  Handler --> Candidates["Registry preselects eligible assumption recipients"]
  Candidates --> Commit["Commit owner removal and replan pending acquisition work"]
  Commit --> Projection["Process acquisition work first, then assumption recipients"]
  Projection -->|"Both helpers succeed"| Result["Private BooleanResult true"]
  Result --> Return["Public void call returns normally"]
  Projection -->|"Queue/send failure"| Error["Private internal error; adapter reports RTIinternalError"]
  Commit -.-> Caveat["State is already committed; inspected path shows no rollback"]
  Projection --> Transport{"Process receive-order event mode"}
  Transport -->|"Push"| Push["Apply registry delivery gate and send event frame"]
  Transport -->|"Pull"| Pull["Retain event for later receive_interaction poll"]
  Push --> Bridge["Recipient bridge applies its callback model"]
  Pull --> Bridge
  Bridge --> Acquired["Acquisition callback work, when produced"]
  Bridge --> Offered["Ownership-assumption callback is an offer, not transfer"]
```

The shared 2025 registry selects assumption recipients before it changes the
owner. Another member must know the object through its known class and publish
the attribute there; that member is excluded for an attribute with a pending
regular acquisition, If-Available acquisition, or cancellation. Those existing
request states continue through their own acquisition or cancellation work
rather than also becoming assumption offers. After building and recording the
candidate set, the plan makes the attributes unowned and replans pending
acquisition work. The process service then projects the returned acquisition
work before the assumption recipients. These are registry and endpoint
boundaries, not a callback-order, fairness, or cross-profile guarantee.

The source sequence is explicit in the
[pending-request exclusion scan](../../cpp/src/internal/federation/federation_registry_attribute_ownership_assumptions.cpp#L82),
[candidate preselection](../../cpp/src/internal/federation/federation_registry_attribute_ownership_assumptions.cpp#L114),
and
[unowned transition plus pending-work replan](../../cpp/src/internal/federation/federation_registry_attribute_ownership_assumptions.cpp#L198).

The 2025 embedded divestiture test queues regular and If-Available requests
before divestiture and observes each request's own acquisition notification;
it explicitly checks that the If-Available requester receives no assumption
callback ([case](../../cpp/tests/ieee1516_2025_unconditional_attribute_ownership_divestiture_catch2.cpp#L173)).

The test does not assert absence of an assumption callback for the regular
requester. The configured-process cases cited below do not start with either
request already pending, so this exclusion remains source-derived for that
endpoint.

Process evidence: the [public adapter branch](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_services.cpp#L831)
calls the [client operation](../../cpp/src/internal/federation/process_federation_client_ownership.cpp#L99);
the [service handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L662)
calls the [registry plan](../../cpp/src/internal/federation/federation_registry_attribute_ownership_assumptions.cpp#L16)
and then the [recipient queue helpers](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L12).
The [process callback bridge's acquisition-notification branch](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L533)
and [ownership-assumption branch](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L571)
project these private events to public callbacks. Focused configured-process
cases establish selected successful outcomes:

- The [process ownership-transition case](../../cpp/tests/attribute_ownership_query_catch2.cpp#L1032)
  observes `Query Attribute Ownership` report the owner before divestiture and
  the unowned state afterward. Its [companion acquisition case](../../cpp/tests/attribute_ownership_query_catch2.cpp#L1164)
  requests regular acquisition only after observing the unowned state, then
  checks the acquisition notification and new owner. It does not exercise a
  regular request already pending when divestiture runs.
- The [unconsumed pushed-assumption case](../../cpp/tests/ieee1516_2025_process_unconsumed_pushed_ownership_assumption_evoked_catch2.cpp#L62)
  verifies the `HLA_EVOKED` sequence: discovery is delivered on one evoke and
  the assumption offer on the next, with the expected attributes and tag.
- The [process HLA_IMMEDIATE assumption case](../../cpp/tests/ieee1516_2025_process_pushed_ownership_assumption_save_restore_catch2.cpp#L62)
  checks the callback's object, attribute subset, and tag in a save/restore
  scenario.

The already-pending regular-acquisition branch is source-derived, not covered
by those process cases. The registry removes the former owner and immediately
replans pending acquisition work. Its planner can produce an acquisition
notification for the first eligible waiting request on an unowned attribute,
subject to current membership/object-awareness, cancellation and reservation
state, and callback-route checks. This registry selection order is an
implementation detail, not an HLA arbitration or fairness claim. The process
service projects that work through its acquisition-event path: push mode
revalidates and begins the notification before sending the event, while pull
mode queues the event and begins it during the recipient's
`receive_interaction` poll. That begin step can establish ownership before the
callback bridge projects `attributeOwnershipAcquisitionNotification`; this is
a source-path distinction, not a measured public timing guarantee. See the
[pending-work planner](../../cpp/src/internal/federation/federation_registry_attribute_ownership_pending_work.cpp#L261),
[push projection](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L87),
[pull receive boundary](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L400),
[registry notification boundary](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp#L209),
and [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L533).
The [companion process acquisition test](../../cpp/tests/attribute_ownership_query_catch2.cpp#L1164)
requests acquisition only after querying the attribute as unowned, and the
assumption-offer cases do not create a pending regular request first. No
focused configured-process test covers this already-pending branch or its
push/pull begin boundary. The cancellation overlap is traced in the [source-only
process race](#cancellation-vs-a-divest-triggered-acquisition-notification).

These cases cover selected public process routes; they do not prove the full
recipient matrix or the handler's enqueue/send failure branch. The process
handler commits the registry plan before calling the acquisition-work helper,
and calls the assumption-recipient helper only if acquisition-work projection
succeeds. Either helper can fail after some state or recipient work has already
changed. The handler then returns an internal error which the public adapter
maps to `RTIinternalError`; the inspected path shows no rollback or
compensation. No focused process test injects an enqueue or send failure. See
the [service handler](../../cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp#L698),
[registry plan](../../cpp/src/internal/federation/federation_registry_attribute_ownership_assumptions.cpp#L179),
[acquisition-work helper](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L12),
[assumption-recipient helper](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L164),
and [public adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_services.cpp#L864).

#### Failure phase and possible residual effects

The two helpers have different push/pull failure checkpoints. This is a
source-only implementation trace, not a prediction of an untested runtime
failure or an HLA rollback rule:

- In push mode, the acquisition-work helper runs each registry begin gate and
  collects event frames before its separate send loop. A failure during that
  preparation can follow an earlier begin gate (including an ownership
  transfer) before any collected frame is sent. If the send loop fails, the
  begin gates for the prepared batch have already run, while only a prefix of
  its frames may have been sent.
- In pull mode, the helper appends events to recipient queues one at a time;
  registry begin gates wait until a later `receive_interaction` poll. A later
  helper failure can therefore leave an earlier queued prefix, with those
  events still subject to their later state checks.
- Only after acquisition-work projection succeeds does the service project
  assumption recipients. Push sends eligible offers per recipient when
  callbacks are enabled, or defers them to the process queue when callbacks
  are disabled; pull queues them for later polling. A later failure can leave
  earlier offers sent or queued. An assumption offer is not an ownership
  transfer.

In either profile, a helper failure becomes `RTIinternalError` at the public
adapter, but the source shows no rollback of the plan or earlier side effects.
The cited process tests cover selected successful calls; they do not exercise
these failure checkpoints.

```mermaid
flowchart TD
  Plan["Registry plan has committed owner removal,<br/>pending-work replanning, and assumption-search state"] --> Work{"Acquisition work returned?"}
  Work -->|None| Assumptions{"Assumption recipients returned?"}
  Work -->|Some| Mode{"Configured-process event mode"}
  Mode -->|Push| Prepare["Run begin gates and collect event frames"]
  Prepare -->|Prepared| SendBatch["Send collected frames sequentially"]
  Prepare -->|Helper returns false| Error["Private internal error; public adapter reports RTIinternalError"]
  SendBatch -->|All sends succeed| Assumptions
  SendBatch -->|Send fails| Error
  Mode -->|Pull| QueueWork["Queue events one at a time;<br/>begin gates wait for receive polling"]
  QueueWork -->|Queued| Assumptions
  QueueWork -->|Helper returns false| Error
  Assumptions -->|None| Success["Private BooleanResult true"]
  Assumptions -->|Some| AssumptionMode{"Mode and callback-enabled state?"}
  AssumptionMode -->|Push, enabled| PushOffers["Resolve and send eligible offers per recipient"]
  AssumptionMode -->|Push, disabled| DeferredOffers["Defer offer events in the process queue"]
  AssumptionMode -->|Pull| PullOffers["Queue offer events for later polling"]
  PushOffers -->|All processed| Success
  PushOffers -->|Helper returns false| Error
  DeferredOffers -->|Queued| Success
  DeferredOffers -->|Helper returns false| Error
  PullOffers -->|Queued| Success
  PullOffers -->|Helper returns false| Error
  Success --> Return["Public void call returns normally"]
  Error -.-> Residual["No rollback shown:<br/>plan and earlier effects may remain"]
```

The [acquisition-work helper's push preparation/send phases](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L83)
and [pull queue insertion](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L140)
support the first two branches. The [assumption helper](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L164)
contains the per-recipient push send and deferred/pull queue paths. No fault
injection evidence is claimed.
The portable C++ TCK has a [general unconditional-divestiture scenario](../../packages/hla-rti-cpp-tck/src/portable_tck.cpp#L11639)
and [scenario catalog entry](../../packages/hla-rti-cpp-tck/README.md#L1256),
but no process-specific TCK execution is recorded. The focused
[standalone Catch2 behavior case](../../cpp/tests/ieee1516_2025_unconditional_attribute_ownership_divestiture_catch2.cpp#L4)
is embedded. The process probe's occurrence of the operation is inside a
connection-loss cleanup fixture ([fixture](../../cpp/tests/process_federation_service_probe.cpp#L678))
and is not evidence for the ordinary success or failure paths. Keep the
process diagram source-derived with these selected test boundaries; do not
infer process-wide behavior or conformance from them.

### Divestiture If Wanted has an output-set result and later notifications

The 2025 `attributeOwnershipDivestitureIfWanted` signature (§7.13) gives the
owner an output set, `divestedAttributes`. This is a different completion
channel from a callback: in the focused embedded `HLA_EVOKED` case, the call
returns an empty set and retains ownership when no acquirer is pending; with eligible
regular and If-Available acquirers, it returns the transferred attributes and
ownership queries already show the new owners before their acquisition
notification callbacks are evoked. Those later notifications carry the
divestiture tag, not the earlier acquisition tags. A queued release request
to the former owner is suppressed as stale.

```mermaid
sequenceDiagram
  autonumber
  actor A as Current owner
  participant RTI as 2025 embedded RTI
  actor B as Pending acquirer

  A->>RTI: Divest If Wanted (attributes, tag)
  alt No eligible acquirer is pending for this attribute
    RTI-->>A: Attribute omitted from divestedAttributes
    Note over A,RTI: Current owner retains the attribute
  else Eligible acquisition is pending
    RTI->>RTI: Transfer ownership and fill divestedAttributes
    RTI-->>A: Attribute included in divestedAttributes
    Note over RTI,B: Ownership queries reflect transfer before callback dispatch in this test
    B->>RTI: evoke callbacks under the selected model
    RTI-->>B: Notify acquisition (attribute, divestiture tag)
  end
```

This is a per-attribute view of selected embedded evidence, not a complete
normative arbitration rule or process-endpoint claim. The focused [2025
embedded test](../../cpp/tests/attribute_ownership_divestiture_if_wanted_pending_catch2.cpp#L255)
checks the empty-result/retain-owner case, synchronous transfer result,
regular and If-Available recipients, tag selection, and stale release-callback
suppression. The pinned [service declaration](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1093)
defines the output-set interface; the acquisition notification is declared in
the [FederateAmbassador API](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L432).

## Query Attribute Ownership reports a snapshot; it does not transfer

The 2025 API separates the request service (§7.17) from its three result
callbacks (§7.18): `informAttributeOwnership`, `attributeIsNotOwned`, and
`attributeIsOwnedByRTI`. These callback names and sections are visible in the
pinned [RTIambassador API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h)
and [FederateAmbassador API](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h).
The following diagrams describe the **embedded 2025 path only**. The process
endpoint takes a separate client/protocol branch in the ambassador and is not
claimed equivalent here.

The service validates membership, object knowledge, and the supplied
attribute handles before planning a report. Failures at those gates are
synchronous service exceptions, not ownership-result callbacks. For an
ordinary object, the embedded registry groups each nonempty subset by current
federate owner and places unowned attributes in a separate group. A
joined-federate MOM object has a separate RTI-owned ledger and produces the
`attributeIsOwnedByRTI` result rather than fabricating a federate owner. All
groups route to the querying federate. The query changes no ownership state;
it reports the classification found by the plan and later rechecked at
delivery.

```mermaid
flowchart TD
  A[Requester calls queryAttributeOwnership] --> B{Connected, operation available, and joined?}
  B -->|No| X[Synchronous service exception]
  B -->|Yes| C{Known object and defined requested attributes?}
  C -->|No| X
  C -->|Yes| D[Embedded registry plans nonempty result groups]
  D --> E{RTI-owned joined-federate MOM object?}
  E -->|Yes| F[Check requester knows MOM object and attributes are effective]
  F -->|No| X
  F -->|Yes| G[One RTI-owned result group]
  E -->|No| H[Check ordinary object is known and not delete-accepted]
  H -->|No| X
  H -->|Yes| I[Validate attributes against requester's known object class]
  I -->|Invalid| X
  I -->|Valid| J[Group attributes by current federate owner]
  J --> K[Make a separate unowned group when needed]
  G --> L[Route each group to the requester]
  K --> L
  L --> M{Result group classification}
  M -->|Federate owner| N[informAttributeOwnership]
  M -->|Unowned| O[attributeIsNotOwned]
  M -->|RTI-owned MOM| P[attributeIsOwnedByRTI]
  N --> Q[Report only; ownership is unchanged]
  O --> Q
  P --> Q
```

In the focused ordinary-object test, querying one owned and one unowned
attribute produces two distinct requester callbacks after the EVOKED callback
queue is drained; the owner receives neither callback. The MOM case exercises
the separate RTI-owned result and confirms that `isAttributeOwnedByFederate`
remains false for those attributes. These are selected embedded scenarios,
not an exhaustive exception matrix or a process-profile parity claim.

Planning a group is not a promise that its callback will still be meaningful
when user code runs. The embedded path records a pending query identity,
submits its callback route after releasing the registry lock, then consumes
and validates that identity and recomputes the current projection at callback
entry. A removed ordinary object, a no-longer-known projection, or attributes
that no longer match the planned result can suppress that group. The focused
deletion case shows that an already queued ownership report is suppressed
after object removal while the removal callback itself is still delivered.

```mermaid
stateDiagram-v2
  direction TB
  [*] --> PlannedGroup: Query plan classifies attributes
  PlannedGroup --> PendingIdentity: Persist requester, object, kind, owner, and attributes
  PendingIdentity --> CallbackRoute: Submit after releasing registry lock
  CallbackRoute --> Recheck: Callback begins
  Recheck --> Suppressed: Query identity or projection invalid
  Recheck --> DeliverFederateOwner: Still owned by federate
  Recheck --> DeliverUnowned: Still unowned
  Recheck --> DeliverRtiOwned: RTI-owned MOM still known
  DeliverFederateOwner --> [*]: informAttributeOwnership
  DeliverUnowned --> [*]: attributeIsNotOwned
  DeliverRtiOwned --> [*]: attributeIsOwnedByRTI
  Suppressed --> [*]: No stale ownership callback
```

The save/restore tests add a separate persistence boundary: selected pending
unowned and RTI-owned query groups are present in the saved image and can be
rebound after a fresh embedded registry is created. Those two focused cases
exercise both `HLA_EVOKED` and `HLA_IMMEDIATE`; they do not establish restore
coverage for every result kind or every save/restore cut point. The diagrams
also do not specify ordering among independently planned result groups.
These query-result groups are distinct from ownership-transfer requests; see
the [ownership-persistence companion](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md)
for selected release, assumption, and recipient work across restore.

## Accepted object deletion invalidates pending ownership work

For an accepted receive-order `deleteObjectInstance`, the embedded registry
marks deletion accepted and clears pending ownership-query identities and
ownership-operation ledgers. Queued query, regular owner-release,
If-Available, and cancellation callbacks therefore re-enter their callback
gates against an object/request that is no longer eligible, and the selected
embedded tests observe those stale ownership callbacks being suppressed. A
separate receiver-removal reservation is still checked at its own gate; if the
receiver remains a member that knows the object, Umbra commits that receiver
to unknown before `removeObjectInstance`. These are separate callback gates,
not a promise of global ordering among ownership and removal callbacks.

Clearing pending ownership ledgers at receive-order delete acceptance does not
by itself establish when the object's update-region references are released.
The [DDM guide's deletion/update-region timeline](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#object-deletion-and-update-region-lifetime)
tracks those references through Remove and timestamped-retraction lifetime
gates; it is a separate embedded 2025 implementation view, not a normative
ownership-callback ordering rule or process-profile parity claim.

This section's cleanup point is specifically receive-order deletion. Timestamped
Delete has a separate enqueue and first accepted-Remove boundary; the latter
clears pending ownership work, while a later Retract restores an
invocation-time object snapshot. The source does not visibly rebuild the
separate federation-level ownership-query identities, and no focused test
combines pending ownership work with timestamped Delete/Retract. For embedded
callback-route work, source shows a possible ordering split: a closure that
runs during committed deletion is suppressed and is not visibly resubmitted by
Retract, while a closure still queued until after Retract may see restored
per-instance state. Neither ordering has a focused test. The configured-process
route has different server-queue and client-buffer boundaries: pull delivery
rechecks ownership work when polled, while already-sent push events can remain
in the client or callback bridge and are not generically swept by a Delete's
TSO identity. Neither process ordering has a combined test. See the
[TSO deletion/retraction boundary](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#service-specific-effects-deletion-is-special)
for the separate embedded and configured-process source traces; neither is a
normative callback-order rule or cross-profile parity claim.

This accepted federation-wide deletion differs from
`localDeleteObjectInstance`, which refuses the caller's local-forget request
while its relevant ownership work is still pending; see the distinct
[local-delete flow](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#local-delete-forgets-one-federates-view-it-does-not-delete-the-instance).
The diagram's stale-work branches are directly exercised for ownership
queries, regular owner-release requests, If-Available results, and cancellation
confirmation. Other cleared ledgers are source-observed here, not individually
proven by these four deletion scenarios. The focused If-Wanted, negotiated
divestiture, and Confirm Divestiture test files surveyed for this update had no
direct delete/removal references. One If-Wanted restore test does stage a
pending notification and then resigns the owner with
`CANCEL_THEN_DELETE_THEN_DIVEST` before draining callbacks, but it makes no
post-deletion callback assertion; treat that as indirect setup evidence, not
proof of suppression. The Confirm Divestiture candidate-resignation test also
captures a pending notification before the candidate resigns, then tears down
the remaining owner without checking a post-delete callback. Deletion
suppression for these operation families remains source-observed.

```mermaid
flowchart TD
  Delete[Receive-order delete is accepted before queued callbacks start] --> Clear[Mark delete accepted and clear pending query and ownership-operation ledgers]
  Clear --> Query[Queued ownership-query result reaches its callback gate]
  Query --> QueryCheck[Revalidate request identity and current projection]
  QueryCheck --> DropQuery[Deleted object invalidates the queued result, suppress callback]
  Clear --> Operation[Queued regular owner-release request, If-Available result, or cancellation confirmation reaches its gate]
  Operation --> OperationCheck[Check the pending operation at callback entry]
  OperationCheck --> DropOperation[Deletion cleared the operation, suppress callback]
  Delete --> Removal[Process a separate receiver-removal reservation]
  Removal --> RemovalCheck{Receiver remains a member and knows the object?}
  RemovalCheck -->|No| Retire[Retire stale removal reservation]
  RemovalCheck -->|Yes| Commit[Commit receiver to unknown and capture removal snapshot]
  Commit --> Callback[Dispatch removeObjectInstance]
```

The registry's [accepted-delete cleanup](../../cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp#L99),
the [query-result gate](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.cpp#L20),
the [If-Available gate](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.cpp#L79),
the [regular release-request gate](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.cpp#L256),
and the [receiver-removal gate](../../cpp/src/internal/runtime/umbra_rti_ambassador_object_instance_lifecycle_callbacks.cpp#L1331)
establish separate source paths. Focused `HLA_EVOKED` tests demonstrate
[query-result suppression](../../cpp/tests/attribute_ownership_query_catch2.cpp#L359),
[regular owner-release suppression](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_release_callbacks_catch2.cpp#L201),
[If-Available suppression](../../cpp/tests/attribute_ownership_acquisition_if_available_catch2.cpp#L353),
and [cancellation-confirmation suppression](../../cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp#L359),
each with a receive-order removal callback. They are separate scenarios and do
not establish all-ledger callback ordering or process-endpoint parity.

## Edge cases worth remembering

- **Attribute sets are not atomic ownership units.** Track outcomes by each
  object-instance attribute, even when a service accepts one set.
- **Callbacks can be stale.** Object deletion, resignation, publication
  changes, or a superseding request can invalidate queued work before user code
  runs. The dispatcher/registry revalidates at delivery boundaries; do not
  draw every queued callback as guaranteed to execute.
- **More than one requester can be pending.** The
  [multiple-acquirer release-denied case](../../cpp/tests/ieee1516_2025_attribute_ownership_release_denied_multi_acquirer_catch2.cpp)
  shows a denial resolving both requesters as unavailable while the current
  owner remains owner. This guide does not specify fairness or winner
  selection for competing successful transfers.
- **A request may be rejected before it enters a pending state.** Connection,
  federation membership, known object instance, defined attributes, and
  publication are distinct preconditions. The cited tests exercise selected
  failures; this is not the complete exception matrix.
- **Ownership is not update delivery.** Time ordering, region relevance,
  subscription filtering, and update-queue behavior form additional state
  machines. Read the dedicated [timing guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md)
  and [DDM/region guide](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md)
  before inferring whether a new owner has received any particular value. The
  [cross-cutting object-attribute map](HLA-2025-CROSS-CUTTING-STATE-EVENT-OBSERVATION-GUIDE.md#an-object-attribute-value-touches-several-independent-boundaries)
  gives the HLA-first overview of these independent gates; the focused guides
  retain their separate state machines. The
  [TSO ownership-change trace](HLA-2025-TSO-RETRACTION-FLOW-GUIDE.md#ownership-changes-and-an-accepted-tso-update-are-separate-timelines)
  shows one embedded case where an already-accepted update survives a transfer;
  acquisition does not add a new recipient retroactively or bypass the current
  recipient's callback gates.
- **Persistence and resignation are outside this core transfer guide.** See the
  [ownership persistence and resignation companion](HLA-2025-OWNERSHIP-PERSISTENCE-AND-RESIGNATION-FLOW-GUIDE.md)
  for selected pending-work restore and member-scoped cancellation scenarios;
  those diagrams remain bounded to the cited 2025 implementation and tests.
  For the broader current-implementation comparison of all six voluntary
  `ResignAction` values, including the pending-acquisition preflight, see the
  [federation lifecycle guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#resignaction-effects-in-the-current-implementation).
  It is not a full normative decision table or process-profile action matrix.

## Source and focused evidence

- Public 2025 service adaptation and callback scheduling:
  [ownership services](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_services.cpp),
  [ownership callback dispatch](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.cpp).
- Query result planning and callback-time projection:
  [embedded registry query planning and recipient revalidation](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition_if_available.cpp),
  [query callback dispatch](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.cpp#L20),
  and the public 2025 API headers linked above.
- State planning and callback-entry revalidation:
  [regular acquisition registry](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp),
  [If-Available registry](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition_if_available.cpp),
  [negotiated divestiture registry](../../cpp/src/internal/federation/federation_registry_negotiated_attribute_ownership_divestiture.cpp).
- Focused tests:
  [regular acquisition, release, and denial](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_release_callbacks_catch2.cpp),
  [If Available](../../cpp/tests/ieee1516_2025_attribute_ownership_acquisition_if_available_catch2.cpp),
  [unconditional divestiture and assumption search](../../cpp/tests/ieee1516_2025_unconditional_attribute_ownership_divestiture_catch2.cpp),
  [negotiated divestiture cancellation and NoAcquisitionPending](../../cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp#L204),
  [multiple acquirers after release denial](../../cpp/tests/ieee1516_2025_attribute_ownership_release_denied_multi_acquirer_catch2.cpp),
  [process-boundary confirmation with an assumption candidate](../../cpp/tests/ieee1516_2025_confirm_divestiture_process_assumption_catch2.cpp),
  [embedded query grouping and deletion invalidation](../../cpp/tests/ieee1516_2025_embedded_query_attribute_ownership_catch2.cpp),
  [RTI-owned MOM query result](../../cpp/tests/ieee1516_2025_rti_owned_mom_ownership_query_catch2.cpp),
  [pending unowned query restore](../../cpp/tests/ieee1516_2025_public_fresh_registry_pending_unowned_query_attribute_ownership_restore_catch2.cpp),
  and [pending RTI-owned query restore](../../cpp/tests/ieee1516_2025_public_fresh_registry_pending_rti_owned_query_attribute_ownership_restore_catch2.cpp).

The first acquisition case is indexed as
umbra-cpp-attribute-ownership-acquisition-integration: 73 assertions, mapped
to four canonical IEEE 1516.1-2025 sections (7.7.3, 7.8, 7.11, and 7.12) by the
current test-plan query. Those mappings are traceability aids, not conformance
evidence.

## Deliberate limits and next step

This guide does not attempt to encode every ownership exception, arbitration
rule, callback control, automatic resignation action, MOM report, persistence
transition, or interaction with time and DDM. It describes a set of high-value
paths whose behavior is visible in current 2025 source and tests. Expand it
only when a focused source-and-test review can support the added diagram.

The planned HLA-first cross-read of the attribute-value update, update-rate,
ownership, and subscription guides is complete. It found the necessary
distinctions already legible across the existing guides: sender service result
and per-attribute authority, receiver interest and eligibility, per-attribute
projection and rate admission, and callback observation. The cross-cutting
object-attribute map supplies the overview, while focused guides preserve the
separate state machines and embedded/process evidence. No synthesis diagram
was warranted. The direct-requester entitlement question for Request Attribute
Value Update remains an explicit normative gap in its dedicated guide; this
cross-read does not resolve it.

The negotiated-divestiture cancellation sequence is source/test-reviewed but
has no fresh Mermaid parser, local render, visual inspection, or GitHub verdict.
The configured Scoop sources had no `mmdc` manifest, so no package was
installed. Keep this validation gap visible; do not treat manual syntax review
as a renderer result.

Next documentation handoff: resolve the open 2025 Request Attribute Value
Update entitlement question: does making an explicit value request itself
make an otherwise ineligible requester a receiver of the provider's later
update? The current guide and selected tests keep solicitation separate from
ordinary update receiver eligibility, but neither establishes the normative
§6.21 rule. Use an authorized copy of the clause; if unavailable, leave the
portable outcome open and continue another bounded HLA-first review. Preserve
2010/2025 and embedded/process boundaries; do not build, rerun tests, start CI,
or edit the Requirements Lab during documentation work.
