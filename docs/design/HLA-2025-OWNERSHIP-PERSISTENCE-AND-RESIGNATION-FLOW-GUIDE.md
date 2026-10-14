# IEEE 1516.1-2025 Ownership Persistence and Resignation: Flow Guide

The pinned 2025 API makes the selected `ResignAction` an explicit part of
[`resignFederationExecution`](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L250).
Interpret any one resignation scenario against the standard's full action
rules; a selected implementation trace cannot establish effects for actions
it does not exercise. This companion to the
[core attribute-ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md)
focuses on a narrower question: what selected ownership work crosses a
save/restore boundary, how current 2025 embedded code handles a departing
federate's pending acquisition state, and how selected process paths
distinguish reconstructed ownership work from callbacks already admitted to a
live HLA_EVOKED queue. It is contributor documentation, not an implementation
contract or a claim of conformance.

## Scope and evidence boundaries

- **Standard boundary:** IEEE 1516.1-2025 only. The official [IEEE standard page](https://standards.ieee.org/ieee/1516.1/6688/) identifies the normative document. Consult its exact service text for requirements; this guide does not reconstruct the full resignation-action matrix.
- **Implementation boundary:** selected 2025 embedded ownership ledgers and resignation cleanup, plus selected configured process-endpoint save/restore paths.
- **Test boundary:** the cited public-API tests cover named embedded fresh-registry and process-endpoint scenarios. The process callback-queue cases perform service-level save/restore through a live endpoint; they do not restart the RTI host process. None proves behavior across every deployed process, transport, or profile.
- **Edition boundary:** the 2010 API and runtime are separate. Nothing here implies 2010 behavior or parity.
- **Evidence labels:** source links describe current implementation; focused tests establish only their named observations. Neither substitutes for the standard or proves complete support.

## Keep HLA state distinct from the implementation's recovery model

These rows are a reading aid for selected Umbra paths, not HLA-mandated
storage layers. Membership and attribute ownership are federation facts
exposed through the API. The typed ownership ledger, callback route, and
already-admitted callback queue are different implementation states; classify
which one is being traced before calling work simply “pending.”

| Layer | Examples | Restore/resignation question |
| --- | --- | --- |
| Ownership ledger | Object and attribute identities, owner/requester IDs, pending request IDs, selected attributes, tags, and queued-work markers | Which typed facts can be validated and reconstructed from the saved image? |
| Callback route | Ambassador registration, live dispatcher, and queued executable closure | Which current route can receive reconstructed work? A closure tied to the old registry is not durable data. |
| HLA_EVOKED callback queue | Callback frames already admitted to a live ambassador before save | Does an uninvoked callback remain on the current route? One process case says yes across service-level save/restore; these frames are not saved-image records. |
| Membership | The set of federates currently joined to this execution | Is the saved requester, owner, or notification recipient still a live participant for this restored operation? |

The resignation call has its own caller-visible result. In the pinned 2025
API, `resignFederationExecution(ResignAction)` returns `void` and lists
`OwnershipAcquisitionPending` among its typed exceptions. That exception is
not an ownership callback or a partial ownership outcome. The flow below
documents the current embedded pending-work guard; the action's broader
object/deletion/divestiture effects remain in the separate
[federation lifecycle guide](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#resignaction-effects-in-the-current-implementation)
and the standard's full service rules.

In the cited embedded case, `UNCONDITIONALLY_DIVEST_ATTRIBUTES` is rejected
with `OwnershipAcquisitionPending`, then the member successfully retries with
`CANCEL_THEN_DELETE_THEN_DIVEST`. This is evidence for that retry path, not a
complete normative rule for every `ResignAction`
([test](../../cpp/tests/ieee1516_2025_embedded_resign_action_pending_acquisition_catch2.cpp#L36),
[pinned API](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L250)).

The state-image capture writes typed ownership records. In the route-free fresh-registry path, restore validates their identities against current members and object attributes; for regular acquisition work, it clears old queued-delivery markers so the restored planner can reserve work against live routes. This is deliberately narrower than “all ownership work survives restart.” Only supported ledger shapes should be inferred to restore.

## 1. A pending release callback across save and restore

The focused regular-acquisition case intentionally leaves the owner's release callback undelivered at the save boundary. The test inspects the encoded image, creates a new registry, rejoins, restores, and then checks that the current owner's callback receives the original object, attribute set, and tag. It checks both `HLA_EVOKED` and `HLA_IMMEDIATE` callback models. It does **not** claim that ownership transferred: the requester has no acquisition notification in this scenario.

```mermaid
sequenceDiagram
  participant Requester
  participant Owner
  participant Old as Original registry
  participant Image as Durable image
  participant New as Fresh registry
  Requester->>Old: attributeOwnershipAcquisition(object, attributes, tag)
  Old-->>Owner: Queue requestAttributeOwnershipRelease
  Note over Owner,Old: Leave the release callback pending for this scenario
  Owner->>Old: requestFederationSave(label)
  Owner->>Old: federateSaveBegun() then federateSaveComplete()
  Requester->>Old: federateSaveBegun() then federateSaveComplete()
  Old->>Image: Capture supported request, IDs, attributes, tag, and queued-work facts
  Note over Old,Image: Durable facts are not the old executable callback closure
  Note over Old,New: Tear down the old registry and establish a fresh registry
  Owner->>New: Join restored execution
  Requester->>New: Join restored execution
  Owner->>New: requestFederationRestore(label)
  New->>Image: Load and validate typed ownership records
  Image-->>New: Saved ownership ledger
  New->>New: Rebuild supported pending work against live routes
  Owner->>New: federateRestoreComplete()
  Requester->>New: federateRestoreComplete()
  par Restored ownership work
    New-->>Owner: requestAttributeOwnershipRelease(object, attributes, tag)
  and Restore-status callbacks
    New-->>Owner: federationRestored callback
    New-->>Requester: federationRestored callback
  end
  Note over Owner,Requester: Release request is not an ownership transfer
```

The diagram is a scenario trace, not a normative callback-order guarantee. In the test, callback delivery is drained or enabled according to the selected callback model after restore completion. The later ownership service that resolves the request is outside this chart.

## 2. Resignation with pending acquisition work

The embedded registry first checks whether the departing member is still referenced by regular or If-Available acquisition requests, cancellation work, negotiated-divestiture candidacy, or a pending Confirm Divestiture notification. For the voluntary actions covered by the source guard, unresolved work rejects the resignation attempt before cleanup mutates the federation. The explicit cancellation action removes acquisition-side work belonging to the departing member; it does not erase another joined member's competing request.

```mermaid
flowchart TD
  A[Member calls resignFederationExecution action] --> B{Pending acquisition-side work references this member?}
  B -->|No| E[Continue with the selected action's object policy]
  B -->|Yes| C{Does the selected action explicitly cancel pending acquisitions?}
  C -->|No| D[Reject with OwnershipAcquisitionPending]
  D --> D2[Member and pending ledger remain for a later valid attempt]
  C -->|Yes| F[Erase this member's pending acquisition and candidate records]
  F --> G[Invalidate queued callback work tied to those erased requests]
  G --> H[Keep other members' requests and candidates]
  H --> E
  E --> I[Finish resignation and invalidate the departing live route]
```

In current source, the preflight rejects unresolved acquisition work for `UNCONDITIONALLY_DIVEST_ATTRIBUTES`, `DELETE_OBJECTS`, `DELETE_OBJECTS_THEN_DIVEST`, and `NO_ACTION`; `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` and `CANCEL_THEN_DELETE_THEN_DIVEST` select cancellation. The latter also has its own object action. Do not read the chart as the complete standard action table: last-member deletion and connection-loss cleanup are separate rules and are intentionally not expanded here.

For the distinct deleted-object versus retained-object projections—and the
embedded/process callback-order difference—see the [object and interaction
guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md).

The two focused embedded tests make the contrast tangible: an unconditional-divest resignation is rejected while acquisition work is pending, then a combined cancellation action succeeds; separately, a requester resigning with the cancellation-only action leaves the surviving owner's queued release callback undelivered.

## 3. Remove one candidate without discarding the surviving recipient

One fresh-registry test follows a negotiated divestiture with two selected candidates. The regular candidate resigns with pending-acquisition cancellation. The owner then confirms divestiture for the If-Available candidate. The saved image contains only the surviving recipient's pending Confirm Divestiture notification (`pendingOperationCount == 1`); after restore, that recipient receives the notification with its object, attribute, and tag, while the other attribute remains owned by the original owner.

```mermaid
sequenceDiagram
  participant Owner
  participant Regular as Regular candidate
  participant Available as If-Available candidate
  participant Reg as 2025 embedded registry
  participant Image as Durable image
  Regular->>Reg: attributeOwnershipAcquisition(object, regularAttribute, tagA)
  Available->>Reg: attributeOwnershipAcquisitionIfAvailable(object, availableAttribute, tagB)
  Owner->>Reg: negotiatedAttributeOwnershipDivestiture(object, bothAttributes, divestTag)
  Reg-->>Owner: Queue confirmation work for both selected candidates
  Regular->>Reg: resignFederationExecution(CANCEL_PENDING_OWNERSHIP_ACQUISITIONS)
  Reg->>Reg: Remove Regular's acquisition/candidate state and assumption search
  Note over Reg,Available: Keep the independent If-Available candidate
  Owner->>Reg: confirmDivestiture(object, availableAttribute, confirmationTag)
  Reg->>Reg: Transfer availableAttribute and queue its recipient notification
  Note over Owner,Reg: Owner remains owner of regularAttribute in this scenario
  Owner->>Reg: requestFederationSave(label)
  Reg->>Image: Save surviving notification and its recipient identity
  Note over Reg,Image: Saved object has one pending operation
  Note over Reg,Image: Tear down, create a fresh registry, and rejoin Owner and Available
  Owner->>Reg: requestFederationRestore(label)
  Reg->>Image: Validate recipient and attribute against restored membership/model
  Image-->>Reg: One surviving Confirm Divestiture notification
  Owner->>Reg: federateRestoreComplete()
  Available->>Reg: federateRestoreComplete()
  Reg-->>Available: attributeOwnershipAcquisition notification with confirmationTag
  Note over Owner,Available: Regular attribute stays with Owner, confirmed attribute belongs to Available
```

This chart emphasizes candidate pruning and recipient preservation, not arbitration order or a general fairness rule. In particular, a resigned candidate is not a reason to discard a different candidate's pending operation.

## 4. A pushed callback queue differs from restored ownership work

“Pending ownership callback” can describe two different boundaries in the
selected 2025 process evidence. In the first case, discovery and ownership-
assumption frames have already been pushed into the candidate's live
HLA_EVOKED callback queue before federation save starts. The save/restore
protocol completes without invoking them; later evokeCallback calls deliver
discovery, then the assumption callback, then the queued save/restore status
callbacks in that test's observed order.

In the second case, the restore operation returns value-only ownership-
assumption work. The process service first queues federation-restore
lifecycle notifications and then queues that reconstructed work for a live
recipient. A separate process-endpoint case observes federationRestored
before the ownership-assumption callback. This is not the same event as
retaining the pre-save callback queue.

```mermaid
flowchart TB
  Ask["Which pending-work representation are you tracing?"]

  Ask --> Pushed["Already pushed into a live HLA_EVOKED queue"]
  Pushed --> SaveRestore["Selected process case completes Save and Restore without invoking it"]
  SaveRestore --> Evoke["Later evokeCallback calls"]
  Evoke --> Discovery["discoverObjectInstance"]
  Discovery --> Assumption["requestAttributeOwnershipAssumption"]
  Assumption --> Status["Then save/restore callbacks in this test's queue order"]

  Ask --> Rebuilt["Value-only ownership work returned by restore"]
  Rebuilt --> Validate["Validate saved identities against restored members and object attributes"]
  Validate --> Lifecycle["Queue federation-restore lifecycle notifications"]
  Lifecycle --> Ownership["Then enqueue reconstructed assumption work for a live recipient"]
  Ownership --> Restored["Separate test observes federationRestored before the assumption callback"]
```

These are separate process-profile test scenarios, not two branches of one
execution or an HLA-wide callback-order rule. The pre-pushed case uses a live
configured endpoint and does not restart the RTI process; the reconstructed
case covers a different restore-produced work item. Neither establishes
process/embedded parity, 2010 behavior, or a normative ordering guarantee.

## Implementation and focused test evidence

- **Capture:** [typed ownership image capture](../../cpp/src/internal/federation/federation_registry_state_image_capture.cpp#L1015) records acquisition requests, request/candidate identities, attribute sets, tags, callback markers, and Confirm Divestiture notifications.
- **Restore validation and reconstruction:** [ownership-ledger restore](../../cpp/src/internal/federation/federation_registry_restore_ownership_ledgers.cpp#L569) validates live requester/owner identities and attributes. The route-free regular-request path resets queued callback markers so the restored work can be replanned ([lines 695–704](../../cpp/src/internal/federation/federation_registry_restore_ownership_ledgers.cpp#L695)); Confirm Divestiture notifications likewise require a live recipient ([lines 794–829](../../cpp/src/internal/federation/federation_registry_restore_ownership_ledgers.cpp#L794)).
- **Process callback mapping and restore enqueue boundary:** the [callback bridge](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L501) maps an ownership-assumption event to the public FederateAmbassador callback. The [process restore service](../../cpp/src/internal/federation/process_federation_service_save_restore.cpp#L246) queues federation-restore notifications before enqueueing returned ownership-assumption work; its comment identifies this as the callback-order boundary.
- **Already-pushed HLA_EVOKED callback:** [process endpoint test](../../cpp/tests/ieee1516_2025_process_pushed_ownership_assumption_evoked_save_restore_catch2.cpp#L62) admits discovery and assumption frames before save without invoking them, then checks the later Evoke sequence ([queue admission](../../cpp/tests/ieee1516_2025_process_pushed_ownership_assumption_evoked_save_restore_catch2.cpp#L418), [post-restore delivery](../../cpp/tests/ieee1516_2025_process_pushed_ownership_assumption_evoked_save_restore_catch2.cpp#L464)).
- **Ownership work reconstructed through process restore:** [configured process endpoint test](../../cpp/tests/ieee1516_2025_process_restored_ownership_assumption_catch2.cpp#L62) checks federationRestored before the restored ownership-assumption callback.
- **Resignation preflight and cleanup:** [resignation pending-work check](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L142) and [member-scoped cancellation](../../cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp#L462).
- **Regular release callback restored:** [public fresh-registry restore scenario](../../cpp/tests/ieee1516_2025_public_ownership_acquisition_restore_catch2.cpp#L142) captures pending work; [post-restore callback assertions](../../cpp/tests/ieee1516_2025_public_ownership_acquisition_restore_catch2.cpp#L256) verify object, attribute, and tag with both callback models.
- **Surviving recipient after candidate resignation:** [two-candidate setup and resignation](../../cpp/tests/ieee1516_2025_public_confirm_divestiture_resignation_restore_catch2.cpp#L180), [single-operation saved-image assertions](../../cpp/tests/ieee1516_2025_public_confirm_divestiture_resignation_restore_catch2.cpp#L285), and [restored notification/ownership assertions](../../cpp/tests/ieee1516_2025_public_confirm_divestiture_resignation_restore_catch2.cpp#L361).
- **Resignation rejection:** [unconditional-divest action rejected while acquisition is pending](../../cpp/tests/ieee1516_2025_embedded_resign_action_pending_acquisition_catch2.cpp#L36).
- **Cancellation-only action:** [departing requester cancellation suppresses the queued release callback](../../cpp/tests/ieee1516_2025_embedded_resign_action_cancel_pending_acquisition_catch2.cpp#L49).
- **Adjacent restore coverage:** [If-Available callback](../../cpp/tests/ieee1516_2025_public_pending_ownership_if_available_restore_catch2.cpp#L6) and [mixed negotiated confirmations](../../cpp/tests/ieee1516_2025_public_mixed_negotiated_ownership_restore_catch2.cpp#L6) have separate focused scenarios. They are not extra claims in the diagrams above.

The resignation source comments associate the unresolved-acquisition check with IEEE 1516.1-2025 §4.12.3 and mandatory last-member object deletion with §4.12.4. Verify exact wording and applicability in the official standard before using either as a normative interpretation. The cited tests are development-profile evidence only.

## Deliberate limits and next step

This companion covers selected regular acquisition and Confirm Divestiture/If-Available state, plus the two process callback/restore cases described above. It does not claim every ownership operation is portable across a fresh registry, expand all `ResignAction` combinations, or integrate ownership with time management, DDM, or TSO scheduling. Use the [core ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md) for transfer and callback-time state, and the [save/restore guide](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md) for federation-wide save coordination.

Receive-order deletion and stale ownership-callback invalidation are now
diagrammed in the [core ownership guide](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#accepted-object-deletion-invalidates-pending-ownership-work),
with object-state commit and federate-local deletion in the
[object/interaction guide](HLA-2025-OBJECT-AND-INTERACTION-INFORMATION-FLOW-GUIDE.md#object-removal-delete-callback-and-per-receiver-cleanup).
Keep that object-lifecycle material there instead of duplicating it in this
ownership-persistence companion. A filename-filtered survey of 154 IEEE
1516.1-2025 ownership, resignation, and restore test files found no direct
`deleteObjectInstance` or `removeObjectInstance` call in the focused public
restore/resignation/mixed-ownership cases. The If-Available restore scenario
drains callbacks before resignation ([callback drain](../../cpp/tests/ieee1516_2025_public_pending_ownership_if_available_restore_catch2.cpp#L195));
the negotiated restore fixture likewise drains its confirmation
([callback drain](../../cpp/tests/ieee1516_2025_public_pending_negotiated_owner_confirmation_restore_catch2.cpp#L225));
and the mixed-negotiated case reaches its saved-image assertions before
teardown ([saved-image boundary](../../cpp/tests/ieee1516_2025_public_mixed_negotiated_ownership_restore_catch2.cpp#L272)).

There is limited indirect evidence in two adjacent cases. The If-Wanted restore
test captures a pending notification, then resigns the owner with
`CANCEL_THEN_DELETE_THEN_DIVEST` before any callback drain
([pending ledger](../../cpp/tests/ieee1516_2025_public_divestiture_if_wanted_notification_restore_catch2.cpp#L177),
[resignation action](../../cpp/tests/ieee1516_2025_public_divestiture_if_wanted_notification_restore_catch2.cpp#L190)).
It does not assert whether the requester callback is suppressed after that
transition. The Confirm Divestiture candidate-pruning test similarly records a
pending notification before the candidate resigns, then resigns the owner
without a post-delete callback assertion
([pending notification](../../cpp/tests/ieee1516_2025_public_confirm_divestiture_resignation_restore_catch2.cpp#L285),
[resignation sequence](../../cpp/tests/ieee1516_2025_public_confirm_divestiture_resignation_restore_catch2.cpp#L300)).
A similarly named service-report If-Available case drains and observes its
acquisition notification before deleting the object. These scenarios establish
adjacent state and teardown paths, not direct suppression evidence for
If-Wanted, negotiated divestiture, or Confirm Divestiture.

The ownership-specific preflight diagram complements, rather than replaces,
the [federation lifecycle guide's `ResignAction` effects table](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#resignaction-effects-in-the-current-implementation):
the table covers object/divestiture effects and final-member behavior, while
this chart explains the pending-work reject-versus-cancel guard. The lifecycle
guide now links back here and records eight focused embedded action-test files.
No additional action diagram was justified because it would repeat those two
distinct views rather than clarify a new tested transition.

The byte-order audit is recorded in the [flow-guide backlog](HLA-BEHAVIOR-FLOW-GUIDES.md):
both edition-specific encoding guides explicitly label BE/LE value octets and
separate them from process-frame metadata. The next cross-guide handoff is
tracked there. Do not infer cross-edition parity or edit the HLA Requirements
Lab during this Umbra documentation phase.
