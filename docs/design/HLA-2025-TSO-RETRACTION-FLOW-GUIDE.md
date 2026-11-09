# IEEE 1516.1-2025 TSO Retraction Flow Guide

This guide is a deep dive into the lifecycle of a timestamped message that can
be retracted. It complements the [time-management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md):
that guide introduces queue phases and grant ordering; this one follows the
retraction designator from send, through recipient-specific delivery states,
to successful `Retract` and any `Request Retraction` callbacks.

**Edition:** IEEE 1516.1-2025 only. This guide does not describe the separate
2010 reference-RTI stream or infer behavioral parity between editions.

**Implementation profile:** current 2025 Umbra behavior, with embedded and
process-endpoint details identified separately. Source and tests establish
only the implementation paths and cases linked below; they do not establish
complete support or conformance.

## Keep three questions separate

Do not treat a retraction as one federation-wide yes/no state. Ask these three
related but distinct questions; their order in this table is a reader aid, not
a required RTI processing sequence:

| Question | Evaluated for | What it determines |
| --- | --- | --- |
| Is `Retract` still legal under the producer's time boundary? (§8.22.3) | The originating federate and this message timestamp | Whether the service can proceed; this is separate from whether any recipient has already observed the message. The profile-specific boundary used by Umbra is shown below. |
| What happened to the original delivery? (§§8.22–8.23) | Each planned recipient independently | A still-pending copy can be withdrawn; a recipient that already received the message may instead need `Request Retraction`. One designator can therefore have different recipient outcomes. |
| What happens to each outstanding time request afterward? | Each receiving federate and its own request mode | In the embedded case, removing queued work makes Umbra reconsider pending grant work; whether a grant follows still depends on that request's rules. The example does not make FQR, TARA, and NMRA equivalent. |

This is a reader's decomposition, not an additional normative state machine.
The later sections label the HLA contract separately from Umbra's embedded
and process evidence; verify exact normative wording against the licensed
standard text.

## Two similarly named operations, two different moments

| Name | Who invokes it? | What it means in this flow |
| --- | --- | --- |
| `RTIambassador::retract(handle)` / **Retract** | The federate that originated the timestamped message | Ask the RTI to retract the producer's timestamped message. Its time precondition and recipient-specific consequences are covered below. In the embedded 2025 path, callback entry is the implementation's delivered threshold. |
| `FederateAmbassador::requestRetraction(handle)` / **Request Retraction** | The RTI calls the recipient's ambassador | Notify a recipient that an already-delivered timestamped message was retracted. It is not another send-side service and does not mean a queued TSO callback was delivered. |

The normative clause pointers are IEEE 1516.1-2025 §8.22 (Retract), including
§8.22.3's time precondition, and §8.23 (Request Retraction). Start from the
[official IEEE 1516.1-2025 edition page](https://standards.ieee.org/ieee/1516.1/6688/)
and verify normative interpretation against the official licensed clause
text.

## One designator: portable outcomes and 2025 recipient gates

At the HLA level, evaluate the original message separately for each intended
recipient. If its copy is still pending, successful retraction prevents that
future delivery; if the recipient has already received it, Request Retraction
is the notification path. The receiving federate handles consequences of an
already-observed message; the RTI cannot undo arbitrary application state. This
is an orientation model for §§8.22–8.23, not a complete normative decision
table; check detailed conditions and timing against the licensed clause text.

The following deliberately omits RTI data structures and callback scheduling.
It shows only the portable questions: whether Retract succeeds, what each
recipient has observed, and where responsibility for already-observed effects
lands. The exact service-specific outcomes remain governed by §§8.22–8.23.

```mermaid
flowchart TD
  A[Producer invokes Retract for a message designator] --> B{Does the service accept Retract?}
  B -- No --> C[No successful retraction effect]
  B -- Yes --> D[Consider each intended recipient separately]
  D --> E{Has this recipient received the original message?}
  E -- No --> F[Prevent this recipient's future delivery of the original]
  E -- Yes --> G[Notify with Request Retraction when the service rules require it]
  F --> H[Recipient outcomes may differ for one message designator]
  G --> H
  H --> I[The receiving federate handles any effects already observed]
```

This is an HLA-level orientation, not a claim about Umbra's queue, ledger, or
callback phases. A successful Retract cannot roll back arbitrary application
state; the receiver handles consequences of a message it already observed.

### Embedded 2025 recipient-state example

The following graph adds current embedded-profile delivery and callback gates.
Umbra's private recipient ledger records `pending`, `delivered`, `suppressed`,
and `retracted`; labels such as `InTransit` and `RequestCallbackAdmitted` are
reader-facing distinctions, not HLA state names.

```mermaid
stateDiagram-v2
  direction TB
  [*] --> Pending: TSO entry queued
  Pending --> InTransit: Grant selects entry
  InTransit --> Delivered: Callback enters user code
  InTransit --> Suppressed: Runtime gate blocks callback
  Pending --> Withdrawn: Retract removes pending entry
  Delivered --> RetractionRequested: Retract marks delivered recipient
  RetractionRequested --> RequestCallbackAdmitted: Recipient still joined
  RetractionRequested --> RequestCallbackDropped: Recipient left first
  Suppressed --> TerminalWithoutRequest: No original callback was delivered
  Withdrawn --> Terminal: Message's recipient entry is retired
  RequestCallbackAdmitted --> Terminal: requestRetraction(handle) runs
  RequestCallbackDropped --> Terminal: Captured route is invalid
  TerminalWithoutRequest --> Terminal
```

One send can be in different phases for different recipients at the same
instant—for example, already delivered to an unconstrained recipient while a
time-constrained recipient's copy is still pending. The portable distinction
is per-recipient delivery versus non-delivery. `Pending`, `Delivered`, and the
other capitalized states below are Umbra ledger labels that instantiate this
distinction; they are not HLA state names. `Suppressed` and callback-route
admission checks are additional embedded 2025 behavior.

Important transitions:

- **Portable outcome — original copy still pending:** the orientation map
  treats a successful Retract as preventing that recipient's future original
  delivery; there is no already-delivered effect for Request Retraction to
  address. In this embedded path, removing the queued entry implements that
  outcome. Verify the exact normative condition and timing in §§8.22–8.23.
- **Portable outcome — original copy already delivered:** Request Retraction
  is the distinct callback path for the retracted, already-delivered message;
  it is not a second delivery of the original TSO message. In the embedded
  source, `Delivered` is recorded when the original callback enters recipient
  code, and a successful Retract then queues the direct callback with the same
  opaque public handle. That callback-entry threshold is profile evidence, not
  a universal HLA application-state boundary.
- **Embedded 2025 gate — Suppressed → TerminalWithoutRequest:** a grant
  boundary may have been crossed while a runtime declaration/lifetime gate
  suppressed user code.
  This is not a delivery and must not be upgraded to a Request Retraction.
- **Embedded 2025 gate — callback admission is checked again:** in the callback
  route, Umbra rechecks that the recipient is still a member and that the
  ledger still says retracted before entering user code. A captured callback
  route is not permission to call into a federate that has since left.

## Producer-side legality and result

The portable HLA question is the time precondition in IEEE 1516.1-2025
§8.22.3: when may the originating federate retract this designator? Keep that
contract question distinct from the RTI's validation order and per-recipient
ledger updates. The following gate sequence is Umbra's current 2025 path, not a
required RTI architecture; verify normative interpretation in the licensed
clause text.

```mermaid
flowchart TD
  HLA["Portable HLA question — §8.22.3:<br/>what time precondition governs Retract?"]
  HLA --> PROFILE["Case-study lens only:<br/>current Umbra 2025 behavior"]
  PROFILE --> A["Timestamped service returned a MessageRetractionHandle"]
  A --> B["Producer calls Retract(handle)"]
  B --> C{Handle valid and owned by this joined producer?}
  C -->|No| X[InvalidMessageRetractionHandle or membership failure]
  C -->|Yes| D{Producer is currently time regulating?}
  D -->|No| Y[TimeRegulationIsNotEnabled]
  D -->|Yes| E[Choose producer boundary: current time, or pending advance-request time]
  E --> F[Add actual lookahead]
  F --> G{Boundary is strictly earlier than original message timestamp?}
  G -->|No| Z[MessageCanNoLongerBeRetracted; do not apply retraction]
  G -->|Yes| H[Apply retraction once in federation ledger]
  H --> I[Remove all still-pending recipient queue entries]
  H --> J[Notify recipients whose original callbacks were delivered]
  H --> K[Re-evaluate pending time-advance grants]
  I --> L[Successful Retract]
  J --> L
  K --> L
  L --> M[Designator is terminal; a later Retract fails]
```

Only the first two nodes frame the question and case-study boundary; the
subsequent edges describe Umbra's validation and mutation path, not a standard
RTI's mandated internal call sequence.

In Umbra's 2025 check, let `t_base` be the producer's current logical time, or
its most recent advance-request time while an advance is pending; let `L` be
its actual lookahead and `T` the message timestamp. The tested source predicate
is `t_base + L < T`. Equality is not enough. Do not substitute the
timestamped-send admission rule here: the source explicitly keeps the Retract
precondition's actual-lookahead comparison separate from the send-service
epsilon rule.

The outcome is not just “valid handle or invalid handle”:

| Condition | Covered Umbra result |
| --- | --- |
| Malformed/default handle, or the designator belongs to another producer | `InvalidMessageRetractionHandle` |
| Producer is not joined | `FederateNotExecutionMember` |
| Producer is not currently time regulating | `TimeRegulationIsNotEnabled` |
| Timestamp is at or behind the current/requested-time-plus-lookahead boundary, or the designator has reached a terminal state | `MessageCanNoLongerBeRetracted` |
| Valid, owned, still-retractable designator | Retraction is applied once; queued fan-out is withdrawn and delivered recipients are classified separately |

An RTI-internal inconsistency while calculating or applying the operation can
produce `RTIinternalError`; the table is not an exhaustive catalogue of every
failure path.

### Disable Time Regulation temporarily blocks Retract while the designator remains live

The pinned 2025 API lists `TimeRegulationIsNotEnabled` for `RTIambassador::retract`
([§8.22 declaration](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h#L1371)).
Umbra's embedded interaction case and configured-process attribute case show
what happens across that role change: a rejected call during the disabled
interval does not apply a retraction or consume the designator. After the
`Time Regulation Enabled` callback, the same handle can be used again if the
time boundary and message state still allow it.

```mermaid
sequenceDiagram
  actor P as Producer
  participant R as Umbra RTI
  participant C as Time-constrained recipient
  Note over P,C: These two cases keep C before the message timestamp
  P->>R: Send timestamped interaction or attribute update at T=5
  R-->>P: Return live MessageRetractionHandle H
  Note over R,C: Accepted TSO copy remains queued at C
  P->>R: Disable Time Regulation
  P->>R: Retract(H)
  R-->>P: TimeRegulationIsNotEnabled
  Note over P,R: No retraction is applied and H remains usable later
  P->>R: Enable Time Regulation
  R-->>P: Time Regulation Enabled callback
  Note over P,R: The HLA_EVOKED cases drain this callback before retrying
  P->>R: Retract(H)
  alt Boundary and message state still allow retraction
    R-->>P: Retract succeeds
    R-->>C: Withdraw pending TSO copy and suppress reflection
    P->>R: Retract(H) again
    R-->>P: MessageCanNoLongerBeRetracted
  else Time boundary has passed the retraction limit
    R-->>P: MessageCanNoLongerBeRetracted
  end
```

The [embedded interaction case](../../cpp/tests/timestamped_interaction_regulation_reenable_catch2.cpp#L257)
and [configured-process attribute case](../../cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_reenable_catch2.cpp#L459)
exercise the same disable/re-enable lifecycle with different timestamped
service families. The process case also checks that `retraction.toString()`
is unchanged across the failed call and re-enable. Both keep the recipient's
copy pending; they do not establish the delivered-recipient path, which is
covered separately under Request Retraction. They are focused Umbra 2025
observations, not evidence for every timestamped service or every profile.

After re-enable, Retract still has to satisfy the strict §8.22.3 time boundary
described above. Re-enabling restores the service precondition; it does not
guarantee that the message remains retractable. For the recipient-side grant
effect of a queued message surviving producer Disable Time Regulation, see the
[time-management example](HLA-2025-TIME-MANAGEMENT-GUIDE.md#one-queued-tso-still-limits-an-undefined-galt-grant).
For the classification of future sends, see the
[order-control cross-review](HLA-2025-ORDER-AND-TRANSPORTATION-FLOW-GUIDE.md#disable-time-regulation-is-a-separate-input-to-future-send-interaction-classification).

### Why retracting can unblock someone else's grant

The federation time coordinator may be waiting for the timestamp of a queued
message before advancing another federate. Once Retract has changed the queue
and ledger state authoritatively, Umbra re-evaluates pending TAR/TARA/NMR/NMRA
grant work. The time advance is not granted merely because the producer called
Retract; the normal scheduler still decides whether each request is now
eligible, and callback gates still apply.

### Retract a regional update before three available-advance callbacks

The general HLA reading question is two-part: for each receiver, was the
original delivery still pending when Retract took effect, and separately,
what result did that receiver's outstanding time-advance request later
report? Keep recipient delivery state distinct from grant results. The
sequence below is one embedded 2025 case, not a universal schedule for these
three request services.

This embedded 2025 test follows one timestamped regional attribute update
through three receiver-side advance choices. The source region `[0,2)` overlaps
each receiver's `[1,3)` subscription. After the update at time 8, the receivers
submit Flush Queue Request(10), Time Advance Request Available(7), and Next
Message Request Available(7). The producer has lookahead 5, advances to 2, and
then retracts while the receiver callbacks have not yet been drained. The
strict producer boundary is satisfied in this case: `2 + 5 < 8`.

```mermaid
sequenceDiagram
  autonumber
  actor Producer
  participant RTI
  participant FQR as FQR receiver
  participant TARA as TARA receiver
  participant NMRA as NMRA receiver

  Note over Producer,RTI: Source [0,2) overlaps each [1,3) subscription, L=5
  Producer->>RTI: UpdateAttributeValues(t=8)
  RTI-->>Producer: Return designator
  FQR->>RTI: FQR(10)
  TARA->>RTI: TARA(7)
  NMRA->>RTI: NMRA(7)
  Producer->>RTI: TAR(2), producer frontier = 7
  Producer->>RTI: Retract(designator), boundary 7 before T=8
  RTI->>RTI: Withdraw pending regional work
  Producer->>RTI: Retract(same designator)
  RTI-->>Producer: Throws MessageCanNoLongerBeRetracted

  Producer->>Producer: evokeCallback()
  RTI-->>Producer: Time Advance Grant(2)
  FQR->>FQR: evokeCallback()
  RTI-->>FQR: Flush Queue Grant(actual 7, optimistic 10)
  TARA->>TARA: evokeCallback()
  RTI-->>TARA: Time Advance Grant(7)
  NMRA->>NMRA: evokeCallback()
  RTI-->>NMRA: Time Advance Grant(7)
  Note over FQR,NMRA: No receiver gets a Reflect Attribute Values callback
```

The focused test body asserts that the first Retract succeeds, repeating it
throws `MessageCanNoLongerBeRetracted`, and all three advance grants complete
without a reflected attribute update. Notably, FQR reports actual grant 7 but
optimistic time 10 after the sole queued message has been retracted; the time
guide describes the separate FQR actual/optimistic calculation. Treat that
value as an observation of this embedded scenario, not a new normative rule or
a general claim that the three advance services are interchangeable. The test
uses `HLA_EVOKED`; it does not record a Request Retraction callback or establish
process-profile or 2010 behavior. For the corresponding non-retracted regional
update through these three services, see the [DDM guide's mixed-advance case](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#one-regional-update-three-available-advance-choices).

## A mixed-recipient walkthrough

This is one 2025 embedded development-profile interaction example, not another
portable HLA decision flow. The focused test source connects all three
federates with `HLA_EVOKED`; recipient **A** calls `evokeCallback` and enters
the original callback before **P** calls Retract. The sequence therefore shows
callback order, not a callback invoked synchronously by `sendInteraction`.
The first flowchart above remains the portable delivered-versus-pending
orientation.

In the test-shaped example, **P** sends message **M** at timestamp 2. Recipient
**A** is not time constrained and receives the original interaction before
Retract. Recipient **B** is time constrained and has not advanced far enough,
so B's copy remains pending in the embedded TSO queue.

```mermaid
sequenceDiagram
  participant P as Producer P
  participant RTI as Embedded 2025 RTI queue and recipient ledger
  participant A as Recipient A (already delivered)
  participant B as Recipient B (still queued)

  P->>RTI: Timestamped send M at T=2
  RTI-->>A: Original callback M (A enters user code)
  Note over A,RTI: In the HLA_EVOKED test, A evokes this callback before Retract, and it is not synchronous with sendInteraction
  RTI->>RTI: Record A=delivered, B=pending under one designator
  Note over B,RTI: B's TSO entry waits for an eligible grant
  P->>RTI: Retract(M handle)
  RTI->>RTI: Check producer-side Retract preconditions (§8.22.3)
  RTI->>RTI: Remove B's pending TSO entry
  RTI->>RTI: Mark A retracted, but do not mark B delivered
  RTI-->>A: Request Retraction callback for M
  Note over A,B: A can respond to the request, but B never receives M
```

The callback is a request to account for the already-delivered effect, not a
second TSO message sent at timestamp 2. It is a direct standard callback path,
not ordinary receive-order interaction traffic. The federate's callback model
still controls when user callback code runs. The focused [mixed-fanout test
source](../../cpp/tests/ieee1516_2025_request_retraction_interaction_mixed_fanout_catch2.cpp#L3)
asserts that the delivered interaction callback precedes Request Retraction
and that the queued recipient gets neither callback. Its target has no fresh
test result recorded; the checked-in assertions are source evidence, not a
newly verified run. This interaction case does not establish the separate
attribute-update behavior (see the [time-management guide's recipient
comparison](HLA-2025-TIME-MANAGEMENT-GUIDE.md#one-timestamped-update-two-recipient-paths)),
process-profile parity, or 2010 behavior.

### A focused trace: delivery does not decide Retract legality

The embedded 2025 test sends four timestamped interactions with lookahead 5.
It exercises two independent questions: has this recipient's callback already
run, and is the producer's message still beyond its Retract time boundary?
The boundary uses the producer's current time—or its pending advance-request
time—plus actual lookahead, and the timestamp must be **strictly later**. A
delivered message may therefore still be retractable, while another delivered
message may already be too late to retract.

```mermaid
sequenceDiagram
  participant P as Producer P (HLA_EVOKED)
  participant RTI as Embedded RTI
  participant R as Receiver R (HLA_EVOKED)

  Note over P,R: Focused HLA_EVOKED test, callbacks explicitly evoked, L=5
  P->>RTI: Send interaction at T=6
  R->>RTI: TAR(6)
  P->>RTI: Retract the T=6 designator before delivery
  RTI-->>P: Accepted, withdraw pending recipient entry
  P->>RTI: TAR(2)
  Note over RTI,R: Later grant to 6, no T=6 interaction callback

  P->>RTI: Send interaction at T=7
  R->>RTI: TAR(7)
  P->>RTI: TAR(2)
  RTI-->>R: ReceiveInteraction at T=7
  RTI-->>R: TimeAdvanceGrant(7)
  P->>RTI: Retract the delivered T=7 designator
  Note over P,RTI: 2+5=7, equality fails the strict boundary
  RTI-->>P: MessageCanNoLongerBeRetracted
  Note over RTI,R: Rejected Retract sends no Request Retraction

  P->>RTI: Send interaction at T=8
  R->>RTI: TAR(8)
  P->>RTI: TAR(3)
  RTI-->>R: ReceiveInteraction at T=8
  P->>RTI: Retract the delivered T=8 designator
  Note over P,RTI: 3+5=8, equality fails again
  RTI-->>P: MessageCanNoLongerBeRetracted

  P->>RTI: Send interaction at T=9
  R->>RTI: FlushQueueRequest(9)
  RTI-->>R: ReceiveInteraction at T=9
  P->>RTI: Retract the delivered T=9 designator
  Note over P,RTI: Current or pending TAR time + L = 8 < T=9
  RTI-->>P: Accepted
  RTI-->>R: Request Retraction for the delivered interaction
```

The [focused test](../../cpp/tests/timestamped_send_interaction_retraction_catch2.cpp#L153)
checks the pre-delivery withdrawal at T=6, callback order across the T=6/T=7
requests, rejection at the T=7 and T=8 boundaries, and successful post-delivery
retraction at T=9. Its dedicated target was freshly rebuilt and the elevated-
filesystem CTest passes all 61 assertions. The sandboxed failure was traced to
`std::filesystem::canonical` being denied access to repository fixture paths,
not to a retraction assertion. Umbra computes the boundary from
current or pending request time plus actual lookahead ([snapshot and boundary](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L614));
the registry compares it with strict less-than before processing recipient
ledger state ([eligibility and ledger](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L133)).
The source path is embedded `HLA_EVOKED` behavior only, not evidence of
process-endpoint, 2010, or every callback-interleaving behavior. The normative
pointer remains IEEE 1516.1-2025 §8.22.3; consult the licensed clause text for
normative interpretation.

A separate [five-form terminalization test](../../cpp/tests/timestamped_tso_designator_terminalization_catch2.cpp)
is designed to cover TAR, TARA, NMR, NMRA, and FQR. Its dedicated target was
freshly rebuilt in single-node mode, and its focused `.build` CTest passes all
40 assertions when run with filesystem access enabled. This verifies those
five advance forms in the embedded `HLA_EVOKED` test profile only;
`.build-fom` does not register the case and provides no additional evidence.

## Ownership changes and an accepted TSO update are separate timelines

For a timestamped reflection, keep two identities separate: the federate that
produced this update and the federate that currently owns the object attribute.
The 2025 `reflectAttributeValues` callback exposes `producingFederate` alongside
the value, tag, time/order, and optional retraction designator in its
[pinned API signature](../../third_party/ieee1516.1-2025/include/RTI/FederateAmbassador.h#L243).
In the focused embedded cases below, a later ownership transfer does not
rewrite an already-accepted update's producer or cancel its queued recipient
entry.

```mermaid
  flowchart TD
  Send["At invocation, member A owns the attribute and submits Update Attribute Values at T"] --> Admit["RTI accepts the timestamped update for its planned recipient set; message and retraction state retain producer A"]
  Admit --> Change{"Before recipient B reaches T"}
  Change -->|Owner remains joined| Transfer["A divests; B acquires the attribute"]
  Change -->|Source departs| Resign["A resigns; B acquires the attribute"]
  Transfer --> Gate["B reaches its queued TSO and live callback gates"]
  Resign --> Gate
  Gate --> Eligible{"B remains eligible and Reflect begins?"}
  Eligible -->|Yes| Reflect["Reflect the original value/tag at T with producingFederate A and the original designator"]
  Eligible -->|No| Suppress["B's recipient copy may be suppressed at its own callback gate"]
```

The recipient in these cases was already subscribed and time-constrained when
the update was sent. Acquisition does not retroactively add a new recipient;
the diagram follows the original planned TSO entry. Delivery still has its own
recipient-side temporal and callback-projection gates, described in the [time
management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md). An ownership change is not
itself evidence that a receiver's queued copy was withdrawn, nor does this
focused transfer establish delivery after unrelated receiver state changes.

The embedded registry retains the update's producer identity and a separate
delivery state for each planned recipient ([enqueue](../../cpp/src/internal/federation/federation_registry_time_tso_enqueue.cpp#L428),
[producer-bound retraction check](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L115)).
At callback admission it checks the recipient's membership and message-ledger
state ([callback gate](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L373));
the delivery path then supplies the stored producer in `reflectAttributeValues`
([embedded TSO dispatch](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_advance_dispatch.cpp#L472)).
These are explanatory implementation boundaries for the two cases below, not
a claim that every receiver-state change leaves delivery untouched.

Two focused 2025 **embedded `HLA_EVOKED`** tests exercise the positive path:

- In [ownership transfer before the constrained grant](../../cpp/tests/timestamped_attribute_update_ownership_transfer_catch2.cpp#L191),
  A queues an update for B, divests, and B acquires before advancing. The
  current test source asserts B as owner and A as no longer owner after the
  acquisition callback but before B advances ([ownership assertions](../../cpp/tests/timestamped_attribute_update_ownership_transfer_catch2.cpp#L309)).
  Its later reflection assertions retain the original value/tag at time 5,
  timestamp order, A as `producingFederate`, and the original valid retraction
  designator. Read these assertions together: authority over the attribute
  changes to B, while the accepted update still identifies A as its producer.
  The test was inspected but not run during this documentation pass; this is
  not a claim about every RTI or transfer path.
- In [source resignation before delivery](../../cpp/tests/timestamped_attribute_source_resignation_catch2.cpp#L196),
  A resigns after queueing the update; a surviving recipient acquires the
  attribute, then receives the queued update with A still named as producer and
  the original value, tag, time, and valid designator.

This is selected embedded-profile evidence, not process-endpoint parity or a
claim that all recipient-state mutations preserve delivery. For example, a
separate [post-loss subscription-change case](../../cpp/tests/connection_loss_attribute_update_tso_suppressed_cleanup_catch2.cpp#L154)
shows a recipient callback can be suppressed by its own later eligibility
change. The producer's retraction designator is also separate from current
attribute ownership: the registry ties it to the original producer and applies
membership/time/retraction-ledger checks. In the inspected embedded path, the
[ambassador Retract entry point](../../cpp/src/internal/runtime/umbra_rti_ambassador_retract_service.cpp#L22)
first requires the caller to remain joined and time-regulating; the registry's
[producer-bound check](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L115)
then checks current membership, that the caller matches the message's recorded
producer, and the timestamp and retraction-ledger state. That path has no
current-attribute-owner check, so the source suggests that transfer alone is
not a Retract rejection condition while the original producer remains joined
and time-regulating and the message passes its remaining checks. This is not a
portable HLA conclusion or a verified post-transfer service result: the
[ownership-transfer test](../../cpp/tests/timestamped_attribute_update_ownership_transfer_catch2.cpp#L191)
does not call Retract after transfer, and the pinned [API header](../../third_party/ieee1516.1-2025/include/RTI/RTIambassador.h)
does not include the full §8.22 normative text. The resignation test observes
a valid designator in the later reflection but does not establish that a
departed producer can still invoke the service. Process profile behavior is
not established by these embedded observations.

The combined Restore-then-Resign path has separate one- and two-recipient
evidence in the Save/Restore guide's [regional AttributeUpdate case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-a-restored-regional-update-outlives-its-producers-resign).
Those tests also observe delivery with a valid handle after the producer
resigns, but do not call `Retract`; they do not extend the separate
source-resignation service-outcome test to a restored, regional payload.

### Regional interaction delivery can outlive the producer's Retract eligibility

The explicit-source interaction counterpart is the embedded
[queued regional interaction source-resignation case](HLA-2025-DATA-DISTRIBUTION-AND-REGIONS-FLOW-GUIDE.md#a-queued-regional-interaction-survives-source-resignation).
It delivers the already-accepted interaction to its surviving recipient after
the producer resigns, while the producer's retraction record becomes terminal.
The callback still contains a `MessageRetractionHandle` whose `isValid()` is
true; the test then observes `FederateNotExecutionMember` when the departed
producer invokes `Retract`. A handle object's validity and the producer's
current membership are separate checks. This case does not exercise
`Request Retraction`, because it does not successfully invoke `Retract`.

The [Save/Restore interaction case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-a-saved-regional-tso-interaction-becomes-live-again)
has a different transition: restoring the earlier image revives its saved live
retraction record, and the still-joined publisher can later retract the
original designator. Do not read that restoration result as a consequence of
source resignation.

## Service-specific effects: deletion is special

Retract does not perform one universal “undo payload” algorithm for every
timestamped service. Each producer service has side effects that may require
separate state handling. Timestamped Delete Object Instance is the clearest
example:

```mermaid
flowchart TD
  A[Timestamped Delete queued: save invocation snapshot and recipient ledger] --> B{Has a recipient passed the registry Remove gate?}
  B -->|No| C[Legal Retract cancels the pending removal reservation]
  C --> D[Keep intervening object and ownership changes; do not restore the snapshot]
  B -->|Yes| E[Commit deletion and clear pending ownership ledgers]
  E --> F[Legal Retract restores the invocation-time ObjectInstance snapshot]
  F --> G[Filter departed federates and regions that no longer exist]
  G --> H[Request Retraction to eligible recipients]
  F --> S{When does an embedded ownership callback closure reach its registry gate?}
  S -->|Before Retract| T[Delete state or cleared key suppresses the callback]
  T --> U[Retract restores state; no ownership-task resubmission is visible]
  S -->|Still queued until after Retract| V[Restored per-instance key may pass its callback gate]
  V --> W[Exact public outcome is untested]
  F -.-> Q[Federation-level ownership-query identities are outside the object snapshot]
  Q --> R[Retract does not visibly restore the query identity]
  D --> I[Retraction completes]
  H --> I
```

The decision above is the shared registry's accepted-Remove gate, not callback
completion. The embedded callback path crosses that gate immediately before
calling `FederateAmbassador::removeObjectInstance`; the configured-process path
crosses it before sending the removal event, and the client callback bridge has
a later gate of its own. See the [process deletion route](#timestamped-object-removal-follows-a-distinct-process-route).
Do not use “the public callback has started” as a synonym for the registry's
deletion-commit boundary.

On the ordinary recipient-bearing path, enqueue records the object snapshot and
retraction recipients without clearing pending ownership work. The first
recipient accepted by the registry sets `deleteAccepted` and clears both the
per-instance ownership-operation ledgers and federation-level ownership-query
identities. A legal Retract before that gate cancels only the removal
reservation; after it, the registry restores the saved `ObjectInstance` before
queuing Request Retraction and filters out departed holders and deleted
regions.

There is an important source-only limit on what that reconstitution restores.
The saved `ObjectInstance` contains its per-instance ownership fields, while
ownership-query identities are stored separately on `Federation`; the first
accepted Remove erases those query identities, and the reconstitution path does
not visibly recreate them. In the embedded path, ownership callback closures
capture request identities and recheck registry state when dispatched. If a
closure reaches its gate while deletion is committed, it is suppressed; Retract
can later restore its old per-instance ledger entry, but the retraction branch
does not visibly resubmit the consumed callback closure. If that closure remains
queued until after Retract, the restored entry and `deleteAccepted == false` may
allow it through the later gate, subject to current eligibility. Query callbacks
are different: their federation-level identity remains absent, so restoring the
object snapshot does not make the old query closure resolvable. These are source
traces, not observed public outcomes or process-endpoint behavior.

For the corresponding producer and receiver results from
`getObjectInstanceName` and `getObjectInstanceHandle`, see the [Name/Handle
guide's timestamped-deletion lookup comparison](HLA-2025-NAME-AND-HANDLE-RESOLUTION-FLOW-GUIDE.md#timestamped-deletion-splits-producer-and-receiver-lookup-boundaries).

Restore and Resign are separate paths through this deletion state. The
[Save/Restore guide's queued-delete case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-queued-timestamped-object-deletion-returns-after-fresh-registry-restore)
shows a pending time-9 deletion and its retraction data rebound to the restored
object and current recipient routes. Its separate
[restored-delete retraction case](HLA-2025-FEDERATION-SAVE-RESTORE-FLOW-GUIDE.md#worked-case-retracting-a-restored-deletion-after-one-recipient-receives-it)
then observes Request Retraction for the receiver that saw Remove, suppression
of the still-pending receiver copy, and restored object lookup. These are
embedded fresh-registry tests within one process, not OS-process restart or
configured-process evidence. Voluntary Resign instead ends the departing
member's membership and applies a selected cleanup action; see the Lifecycle
guide's [`ResignAction` effects](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#resignaction-effects-in-the-current-implementation).

### Disable Time Constrained separates known-object removal from the Remove callback

When a constrained receiver disables its role with a timestamped deletion queued,
the embedded path removes that receiver's known-object entry during the queue drain.
The receive-order `removeObjectInstance` task reaches the deletion commit gate later.
Those are separate transitions: Disable changes receiver knowledge, while the later
gate decides whether this recipient's deletion is still pending or has been accepted.

```mermaid
flowchart TD
  A["Timestamped Delete accepted<br/>receiver knows object and is time constrained"] --> B["Receiver disables Time Constrained"]
  B --> C["Drain queued TSO as receive-order work"]
  C --> D["Erase receiver's known-object entry now"]
  D --> E["Queue receive-order Remove task"]
  E --> F{"Which registry transition wins?"}
  F -->|Producer's legal Retract first| G["Cancel pending removal;<br/>restore this joined receiver's prior known-object entry"]
  G --> H["Later Remove task fails its pending-delete gate and is suppressed"]
  F -->|Remove task reaches its gate first| I["Mark recipient delivered;<br/>commit deletion and invalidate pending object work"]
  I --> J["Invoke receive-order removeObjectInstance<br/>without timestamp or retraction handle"]
  I --> K{"Can the producer still legally Retract?"}
  K -->|Yes| L["Restore the invocation-time object snapshot;<br/>queue Request Retraction for the delivered recipient"]
  K -->|No| M["The producer's designator has become terminal"]
```

This chart is an embedded-source trace. A successful Disable erases receiver
knowledge before the evoked callback task runs, but that erase alone does not
commit the execution-wide deletion. If a legal producer Retract wins before the
task's registry gate, the registry cancels the pending deletion and restores the
receiver's invocation-time knowledge; the already-queued Remove task is then
suppressed. If the task crosses its gate first, the recipient becomes delivered
and the deletion is committed before user code. A still-legal Retract follows the
delivered-recipient restoration path and queues Request Retraction. The producer's
designator remains governed by its own regulation and timestamp bounds throughout;
the receiver's Disable call does not itself consume it. After the removal gate,
the source does not establish whether a concurrent Remove callback or Request
Retraction becomes observable first.

The embedded source path is the [Disable adapter](../../cpp/src/internal/runtime/umbra_rti_ambassador_time_constrained_control.cpp#L249),
[queue conversion and known-object erase](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L760),
[receive-order callback task](../../cpp/src/internal/runtime/umbra_rti_ambassador_tso_receive_order_delivery.cpp#L385),
[Remove commit gate](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L836),
and [producer Retract/reconstitution branch](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L115).

The configured-process path has a different boundary. Its Disable handler drains
queued TSO through the process service, which crosses the shared registry Remove
gate before sending the receive-order event to the client. Client event buffering
and callback dispatch happen after that server-side gate. Do not move the embedded
pre-gate Retract branch to the process client's callback-entry point. The reviewed
process tests do not combine Disable Time Constrained, a queued timestamped Delete,
and producer Retract, so this guide makes no claim about their combined callback
order. See the process [Disable handler](../../cpp/src/internal/federation/process_federation_service_time_management.cpp#L661)
and [deletion dispatch before event send](../../cpp/src/internal/federation/process_federation_service_tso_payload_dispatch.cpp#L246)
for that earlier server-side gate.

Focused embedded evidence is split across two cases. The [queued-delete Disable
case](../../cpp/tests/timestamped_object_deletion_retraction_catch2.cpp#L641)
observes one receive-order removal after Disable. The [re-enable case](../../cpp/tests/ieee1516_2025_timestamped_delete_object_instance_time_constrained_reenable_catch2.cpp#L5)
checks that the converted callback has no timestamp or retraction handle and that
the producer's designator becomes unusable after the producer advances past its
time bound. Neither case queries receiver knowledge between Disable and callback
entry or combines Disable with Retract; the early knowledge transition and the
pre-gate cancellation branch above are source-derived. The process [Disable
case](../../cpp/tests/ieee1516_2025_connection_process_tso_ingress_proxy_catch2.cpp#L1313)
covers a queued interaction, not object deletion. For its server-side Remove gate
and later client event path, see the [configured-process deletion route](#timestamped-object-removal-follows-a-distinct-process-route).

### Configured-process callback buffers are a separate boundary

Do not carry the embedded closure schedule above over to the configured-process
client. For a queued, time-constrained recipient, ownership work can be waiting
in a server session queue, already sent to the client's `pendingEvents_`, or
submitted to the callback bridge. The timestamped Remove gate is server-side
and precedes sending the deletion event; the process client's Retract handling
is keyed to the deletion's TSO identity, not to unrelated ownership work.

```mermaid
flowchart TD
  subgraph DeleteLifecycle["Timestamped Delete lifecycle"]
    direction TB
    Delete["TSO Delete: snapshot + message ID"] --> Remove["First Remove gate: commit deletion + clear ownership ledgers"]
    Remove --> Retract["Legal Retract: restore saved ObjectInstance"]
    Retract --> Sweep["Filter TSO-ID interaction, removal, and update work"]
  end
  subgraph Pull["Pull ownership path"]
    direction TB
    ServerQueue["Server session queue"] --> Poll["Receive poll + operation-specific gate"]
    Poll --> Gate{"Which state exists at this poll?"}
    Gate -->|Delete still committed| Skip["Per-instance gate may reject work;<br/>query identity is absent"]
    Gate -->|Retract restored ObjectInstance| Reopened["Some per-instance work may pass again;<br/>query identity remains absent"]
    Gate -->|No deletion accepted| Normal["Apply ordinary operation-specific gate"]
    Normal --> PullBuffer["Client event buffer"]
    Reopened --> PullBuffer
    PullBuffer --> PullBridge["Callback bridge"]
  end
  subgraph Push["Push ownership path"]
    direction TB
    Admission["Operation-specific plan or gate"] --> PushBuffer["Sent frame in client event buffer"]
    PushBuffer --> PushBridge["Callback bridge"]
  end
  Sweep -. "Ownership work uses separate identities" .-> NoSweep["No generic ownership-event sweep"]
  Note["Combined Delete/Retract ownership callback outcome: untested"]
  Skip -.-> Note
  PullBridge -.-> Note
  PushBridge -.-> Note
```

In pull mode, ownership callbacks remain in per-session queues until a Receive
poll applies their operation-specific gate or projection. The gate sees the
registry state at poll time: while Delete remains committed, per-instance work
can be rejected and a query callback can find its federation-level identity
erased. If legal Retract restores the saved `ObjectInstance` before that poll,
some per-instance records may again satisfy their callback gate, while the
query identity remains absent because it is stored outside that snapshot. The
server Retract sweep does not remove these separately queued ownership events.
This is a source-derived ordering possibility, not a tested delivery outcome.

In push mode, some ownership events cross a registry gate before send; after
that, the client and callback bridge buffer them independently. The bridge has
a distinct late claim for owner-release callbacks, but no generic
ownership-versus-delete recheck. These are different source paths, not
evidence that every event in either mode is suppressed or delivered after
Retract. The pull poll gates are in the [process receive path](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L392);
retraction restores the object snapshot in the [registry path](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L230)
and removes only TSO-keyed server events in the [process Retract handler](../../cpp/src/internal/federation/process_federation_service_retraction.cpp#L174).

On legal Retract, the process service removes only TSO-identity-matched
interaction, object-removal, and attribute-update entries from its server
queues. The process client removes matching interaction and object-removal
events from `pendingEvents_`; an ownership event has no identity tying it to
that Delete. An already-buffered ownership event is therefore outside those
deletion-specific sweeps, although its own operation may have another gate.
The existing [process object callback-gate test](../../cpp/tests/ieee1516_2025_connection_process_object_retraction_callback_gate_catch2.cpp#L319)
checks suppression of a pushed timestamped removal across callback disable,
Retract, and re-enable. The [process timestamped-delete case](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1506)
and [future-arrival case](../../cpp/tests/ieee1516_2025_connection_process_tso_object_deletion_future_arrival_catch2.cpp#L6)
cover deletion delivery and time-advance behavior. None combines pending
ownership-query or ownership-operation callbacks with Delete/Retract, so the
cross-queue callback outcome remains unverified. Relevant source boundaries:
[server TSO Remove gate](../../cpp/src/internal/federation/process_federation_service_tso_payload_dispatch.cpp#L235),
[pull ownership gates](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L289),
[push ownership work enqueue](../../cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp#L12),
[server Retract queue filtering](../../cpp/src/internal/federation/process_federation_service_retraction.cpp#L174),
[client Retract filtering](../../cpp/src/internal/federation/process_federation_client.cpp#L522), and
[callback-bridge ownership admission](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1825).

The focused [pending-removal/retraction test](../../cpp/tests/ieee1516_2025_timestamped_delete_object_instance_retraction_catch2.cpp#L6)
and [delivered-removal/reconstitution test](../../cpp/tests/timestamped_object_deletion_retraction_catch2.cpp#L229)
do not combine deletion with pending ownership queries or ownership operations,
and neither arranges the callback-before-versus-after-Retract ordering above.
Therefore the callback outcome remains unverified. The cited source paths are
[TSO enqueue and snapshot](../../cpp/src/internal/federation/federation_registry_time_tso_enqueue.cpp#L829),
[first accepted removal and cleanup](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L779),
[query-identity cleanup](../../cpp/src/internal/federation/federation_registry_attribute_ownership_pending_work.cpp#L43),
[per-instance ownership state](../../cpp/src/internal/federation/federation_registry.hpp#L2508),
[federation-level query identities](../../cpp/src/internal/federation/federation_registry.hpp#L2619), and
[deletion reconstitution](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L230).
The embedded callback route and representative gates are
[queued acquisition closures](../../cpp/src/internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.cpp#L133),
[regular acquisition gate](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp#L210),
[assumption callback gate](../../cpp/src/internal/federation/federation_registry_attribute_ownership_assumptions.cpp#L229), and
[ownership-query identity lookup](../../cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition_if_available.cpp#L190).

The [ownership guide's accepted-deletion flow](HLA-2025-ATTRIBUTE-OWNERSHIP-FLOW-GUIDE.md#accepted-object-deletion-invalidates-pending-ownership-work)
describes receive-order deletion. Keep its cleanup point distinct from this
timestamped path's first accepted-Remove gate. Neither guide establishes a
normative callback-order rule from implementation source. For configured-process
queue and client-buffer boundaries, use the separate [process callback-buffer
trace](#configured-process-callback-buffers-are-a-separate-boundary), not the
embedded closure schedule above.

This deletion path is not the generic rule for interactions or attribute
updates. For those services, the RTI's recipient ledger decides which original
callbacks were delivered and which Request Retraction notifications to issue;
the receiving federate remains responsible for handling the callback's
service-specific consequences.

## Embedded versus process route

The public handle remains the standard opaque `MessageRetractionHandle`; the
current private implementation maps it to a federation-owned message identity.
The API route differs by endpoint:

| Endpoint | Where the authoritative retraction decision happens | What is rechecked or withdrawn |
| --- | --- | --- |
| Embedded federation | The ambassador validates its joined/time-regulation snapshot, then the embedded federation registry applies the owner, timestamp, queue, and recipient-ledger checks. | Pending queue fan-out is removed; direct Request Retraction callbacks use captured callback routes with a live membership/ledger recheck. |
| Process endpoint | The client forwards the producer identity and designator; the process service checks its producer-owned map and time state, then calls the shared registry retraction operation. | The process service erases queued event copies and drains a separate cache of recipient sessions to send `request_retraction` transport events. That send path does not recheck receiver membership or callback-started state; the client bridge later checks its per-message retraction state before submitting the public callback. See the process-after-Resign trace below. |

### Process receiver Resign after a cached TSO attribute delivery

This is a 2025 process-endpoint source trace, not a normative HLA sequence or
evidence about the embedded or 2010 streams. The important boundary is that
federation membership and transport-session attachment are separate: a
successful receiver Resign clears its server-owned event queues, but does not
remove that still-connected session from another producer's already-cached
retraction routes. This sequence follows coordinator-dispatched TSO delivery:
the service adds the route only after successfully sending the event frame; a
message still waiting only in the time coordinator has not reached this
route-cache boundary. Other recipients can bypass that coordinator queue at
the original service call, and those send-time branches populate the route
cache differently for attribute updates and ordinary interactions, as
compared below. Session detachment prunes the cache; producer Resign removes
cache entries for that producer's own message designators.

#### Attribute route survives receiver Resign

This sequence ends at the server-to-client control event. It follows the
coordinator-dispatched TSO path; a message still waiting only in the time
coordinator has not reached this route-cache boundary.

```mermaid
sequenceDiagram
    participant P as Producer
    participant S as 2025 process service
    participant R as Attached receiver session

    P->>S: Send timestamped attribute update at T
    S-->>P: Return producer-owned designator
    Note over S,R: Begin after eligible TSO dispatch, not while coordinator-queued
    S-->>R: Send receive_attribute_update frame
    S->>S: Cache session after successful send
    R->>S: Resign
    S->>S: Clear membership and this session's server queues
    Note over S,R: Session stays attached in other messages' retraction routes
    P->>S: Retract while producer remains joined and time regulating
    S->>S: Apply registry retraction, remove queued copies
    S-->>R: Send request_retraction using cached route, no membership check
```

#### Client bridge decides the public callback

The control event reaches the process client later. Whether it becomes a public
callback depends on the original event's client and bridge state. This client
boundary is also used by the timestamped object-removal path below.

```mermaid
sequenceDiagram
    participant C as Process client
    participant B as Callback bridge

    C->>C: Read and buffer request_retraction
    alt Original event remains in pendingEvents_
        C->>C: Erase original event, defer its delivery acknowledgement
    else Original event reached the callback bridge
        C->>B: Submit Request Retraction identity
        alt Original callback has started
            B->>B: Queue Request Retraction on the dispatcher
        else Tracked original callback has not started
            B->>B: Suppress both callbacks, acknowledge tracked TSO work if applicable
        else No per-message state exists
            B->>B: Suppress public Request Retraction
        end
    end
    Note over C,B: Post-Resign public outcome is source-only and untested
```

The process service's recipient cache contains session pointers, not a fresh
membership snapshot. Its detach path removes a session from that cache; the
receiver-Resign cleanup instead clears that session's ordinary event queues and
joined identity. The shared registry filters its own notification vector to
recipients that remain members and have a callback route. In the process TSO
delivery path, however, `beginTsoAttributeUpdateCallback` runs before the
transport event is sent, so the registry's internal `delivered` marker is not
proof that the public `FederateAmbassador` callback has started. The process
handler applies the registry result but sends transport events from the
separate session cache. A later producer `Retract` still has its own
producer-membership and time-regulation gates. When the control event reaches
the client, the callback bridge admits public `requestRetraction` only when it
has a tracked message whose original callback has started; otherwise it
suppresses the control callback (and acknowledges the suppressed delivery
when it has tracked state). The bridge checks its callback session and
retraction state, not the receiver's current federation membership. Immediate
versus evoked execution still follows the selected callback model.

Keep this cached-route observation separate from membership cleanup and the
selected `ResignAction`. The [Lifecycle guide's voluntary-versus-forced exit
comparison](HLA-2025-FEDERATION-AND-FEDERATE-LIFECYCLE-FLOW-GUIDE.md#voluntary-resign-and-forced-connectionlost-use-different-entry-contracts)
explains the distinct Resign and ConnectionLost entry paths; neither path
establishes that a cached process Request Retraction becomes a public callback.

The source path is not proof that a public callback occurs after receiver
Resign: the result depends on the exact client/bridge state and has no focused
process regression test. The [callback-ordering guide's process Resign
section](HLA-2025-CALLBACK-AND-SERVICE-ORDERING-FLOW-GUIDE.md#recipient-departure-after-planning-but-before-observation)
compares the broader buffered-event boundary; do not generalize this route to
directed interactions or other event families.

Source paths: [attribute coordinator TSO delivery records the route after sending](../../cpp/src/internal/federation/process_federation_service_tso_delivery.cpp#L157),
[attribute send-time dispatch records pull and push recipient sessions](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L398),
[receiver-Resign queue cleanup](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L29),
[session detach prunes cached routes](../../cpp/src/internal/federation/process_federation_service.cpp#L115),
[process Retract producer gate](../../cpp/src/internal/federation/process_federation_service_retraction.cpp#L77),
[cached-session frame emission](../../cpp/src/internal/federation/process_federation_service_retraction.cpp#L193),
[the shared recipient ledger](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L115),
[client Resign and joined-identity clearing](../../cpp/src/internal/federation/process_federation_client_federation_management.cpp#L89),
[Request Retraction event buffering](../../cpp/src/internal/federation/process_federation_client.cpp#L537),
[client-to-bridge dispatch](../../cpp/src/internal/federation/process_federation_client_callback_dispatch.cpp#L382),
and [bridge retraction-state admission](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1575).

### Ordinary timestamped interaction comparison

The ordinary 2025 process `TsoInteractionDelivery` path shares the server-side
route-cache and receiver-Resign boundary above, but its client event-buffer
branch is not identical to attribute updates. The interaction dispatcher
reserves the registry recipient state, sends a `receive_interaction` event,
then records the session route. On `request_retraction`, the client removes a
matching interaction event if it is still in `pendingEvents_` and defers the
TSO delivery acknowledgement; if the event already left that queue, the
control event proceeds to the callback bridge. By contrast, the client does
not erase a pending `ProcessFederationAttributeUpdateEvent`; it queues
`PendingRequestRetraction` after it. Both bridge submission paths keep a
per-message `RetractionState`, and `submitRequestRetraction` gates the public
callback on whether the original callback has started. This is a process
implementation comparison, not an HLA rule. It covers ordinary TSO
interaction delivery, not the separate directed-interaction dispatch path.

There is a second callback-boundary difference in the send-time pull branch.
Both coordinator-dispatched TSO functions call their `beginTso...Callback`
gate before sending the event frame; that registry marker is earlier than
public callback entry. When a queued attribute event is pulled, the process
service calls `beginTsoAttributeUpdateCallback` before returning it. The
ordinary interaction pull handler simply removes and returns its queued event;
the inspected handler has no corresponding `beginTsoInteractionCallback`, and
the send-time pull branch has not added the session to the retraction-route
cache. This is source-observed only. The focused process interaction case
exercises coordinator-dispatched delivery, not this send-time pull branch, so
do not infer a public outcome or an HLA conformance result from the difference.

| Process boundary | Timestamped attribute update | Ordinary timestamped interaction |
| --- | --- | --- |
| Coordinator-dispatched TSO recipient | Send `receive_attribute_update`, then add the session to the retraction-route cache. | Send `receive_interaction`, then add the session to the same cache. |
| Send-time recipient that bypasses the coordinator queue | `handleUpdateAttributeValues` records the session for both pushed events and pull-queued attribute events. | `handleSendInteraction` records the session only in its pushed-event branch; its pull branch queues the interaction event without adding the session in this function. This is a source difference, not a tested outcome claim. |
| Callback-state reservation | Coordinator path reserves before send; pull path reserves when the attribute event is polled. | Coordinator path reserves before send; the inspected send-time pull receive handler has no matching reservation call. |
| Receiver Resign | Clear joined-session queues and identity; retain this session in other messages' route entries until detach. | Remove the session from pending pushed-retraction recipients and clear joined identity/event buffers. Pending attribute Region-copy leases are cleared; pending interaction Region-copy leases are not cleared by this Resign path. |
| Client receives Request Retraction | Keep a pending attribute event; bridge state decides whether its callback is suppressed or already started. | Erase a still-pending matching interaction and defer acknowledgement; otherwise pass the control event to the bridge. |
| Focused process evidence | Existing attribute cases exercise retraction before callback observation and fan-out. | The changed-lookahead case observes a coordinator-delivered timestamped interaction with a valid handle, then confirms Retract is too late; it does not test pre-callback retraction, the pull-queued send-time branch, or receiver Resign. |

Sources: [ordinary interaction TSO post-send route insertion](https://github.com/sheepfling/umbra/blob/c9c5a9e410539b039cd8bba35b53d0ef2b24d180/cpp/src/internal/federation/process_federation_service_tso_delivery.cpp#L393),
[attribute TSO post-send route insertion](../../cpp/src/internal/federation/process_federation_service_tso_delivery.cpp#L157),
[attribute send-time route insertion for pull and push](../../cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp#L398),
[interaction send-time push-only route insertion](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L608),
[attribute pull callback reservation](../../cpp/src/internal/federation/process_federation_service.cpp#L160),
[ordinary interaction pull receive handler](../../cpp/src/internal/federation/process_federation_service_receive.cpp#L37),
[interaction client decode](../../cpp/src/internal/federation/process_federation_client.cpp#L465) and [attribute client branch](../../cpp/src/internal/federation/process_federation_client.cpp#L474),
[interaction bridge state](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1346),
[attribute bridge state](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1624),
and the [focused process interaction retraction case](../../cpp/tests/ieee1516_2025_connection_process_tso_interaction_regulation_reenable_changed_lookahead_catch2.cpp#L460).

### Directed timestamped interaction: separate dispatch, shared retraction seam

Keep the directed route separate from ordinary `TsoInteractionDelivery`.
`Send Directed Interaction` creates a `TsoDirectedInteractionMessage` with an
internal message identity and recipient ledger. When time management releases
that recipient's queue entry, the process dispatcher resolves the current
target-aware projection, crosses the registry's interaction-callback gate,
and sends a `receive_interaction` event carrying that message identity. The
coordinator entry remains in transit until the recipient acknowledges it.
The registry gate is earlier than entry into the public callback; the client
callback bridge keeps a separate per-message state for that later boundary.

On the process Retract route, the server checks that the requesting session
owns the message and is currently time regulating, applies the strict
time-plus-lookahead eligibility check, then asks the shared registry to
retract the queued or unacknowledged in-transit message. A successful result
removes matching interaction events still buffered on the server and sends
`request_retraction` to tracked pushed recipients. The client either removes
the matching event from its own pending-event queue and defers the delivery
acknowledgment, or passes the retraction event to the callback bridge. The
bridge exposes public `requestRetraction` only if the original callback has
started; before that boundary it suppresses the original callback and
acknowledges the delivery. Normal callback return also reaches the private
delivery acknowledgment described in the [process TSO delivery
sequence](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#3-a-timestamped-process-delivery-stays-in-transit-through-callback-completion).

The focused configured-process directed TSO cases exercise delivery to a
pending NMR/NMRA recipient and assert a valid public retraction handle, but do
not call Retract. The separate public-endpoint callback-control test uses a
controlled process server; it exercises client-side pre/post-callback
retraction handling, not the production process service's registry and
recipient ledger. No focused test was found that drives a production-service
directed TSO through Retract while its recipient callback is queued or in
transit. The behavior below the callback boundary is therefore source-traced,
not an observed end-to-end result or an HLA-wide callback-order claim.

A separate public process attribute case observes an adjacent TAR sequence:
the receiver requests TAR(5), the producer queues an update at 5, advances
only to 2, successfully Retracts, then advances to 5; the receiver observes no
reflection and receives its grant at 5 after evoking the retracted callback
work. This is direct evidence for that TAR/pre-callback sequence. It does not
combine a pending NMR/NMRA's late-arrival retarget with Retract, so it does not
settle the NMR/NMRA outcome described in the [Time Management guide](HLA-2025-TIME-MANAGEMENT-GUIDE.md#a-later-retract-removes-tso-work-without-restoring-the-old-target).

The focused public process interaction test uses `ProcessFederationServiceOptions{false}`
and an `HLA_EVOKED` receiver. It checks Retract before the receiver's poll,
after the event has been pulled but before callback start, and after the
interaction callback has started. In the post-poll/pre-start case, neither the
interaction nor public `requestRetraction` callback runs, and the service later
receives the private TSO acknowledgment. Its fixture accepts two
`getInteractionClassHandle` fences before or after that acknowledgment, so the
test proves eventual acknowledgment but does not establish its order relative
to those API calls. In the after-start case, the interaction and one
`requestRetraction` callback are observed; the fixture waits for both the
callback-start fence and TSO acknowledgment before it issues Retract. A separate
`pushReceiveOrderEvents=true` receiver-Resign case observes no attribute or
removal callback after rejoin, but it does not assert TSO acknowledgment
behavior. These cases cover distinct source paths and do not establish a
general push/pull ordering rule.

Grant reevaluation is a separate process-service effect. The inspected
`handleRetract` path does not itself call the grant scheduler. If a delivery
acknowledgment for the retired in-transit identity arrives later, the
coordinator reports it as already completed and the acknowledgment handler
still reevaluates pending grants. An accepted time-advance request also enters
the scheduler, which checks the pending-grant set. The Time Management guide's
[retarget-then-Retract flow](HLA-2025-TIME-MANAGEMENT-GUIDE.md#a-later-retract-removes-tso-work-without-restoring-the-old-target)
shows these source-observed triggers while keeping the combined later
grant/callback outcome untested.

Sources: [directed service queue admission and recipient routing](../../cpp/src/internal/federation/process_federation_service_interaction_send.cpp#L883),
[directed message and recipient retraction ledger](../../cpp/src/internal/federation/federation_registry_time_tso_enqueue.cpp#L927),
[coordinator-dispatched directed payload](../../cpp/src/internal/federation/process_federation_service_tso_payload_dispatch.cpp#L45),
[pre-frame registry callback gate](../../cpp/src/internal/federation/process_federation_service_tso_payload_dispatch.cpp#L96),
[producer-owned process Retract and pushed-recipient notification](../../cpp/src/internal/federation/process_federation_service_retraction.cpp#L77),
[shared per-recipient retraction state](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L115),
[recipient callback reservation](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L460),
[client pending-event filter](../../cpp/src/internal/federation/process_federation_client.cpp#L522) and [retraction-event enqueue](../../cpp/src/internal/federation/process_federation_client.cpp#L537),
[callback-start-aware bridge handling](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1583),
[directed pending-NMR process case](../../cpp/tests/ieee1516_2025_connection_tso_directed_interaction_callback_gating_catch2.cpp#L1522),
[directed pending-NMRA process case](../../cpp/tests/ieee1516_2025_connection_tso_directed_interaction_callback_gating_catch2.cpp#L1554),
[process attribute Retract and subsequent TAR grant](../../cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_before_callback_catch2.cpp#L712),
[process interaction retraction across callback entry](../../cpp/tests/ieee1516_2025_connection_process_tso_interaction_retraction_before_callback_catch2.cpp#L145),
[push-enabled process Resign before Retract](../../cpp/tests/ieee1516_2025_connection_process_tso_interaction_retraction_before_callback_catch2.cpp#L654),
and [controlled-server directed callback/retraction test](../../cpp/tests/ieee1516_2025_connection_directed_interaction_callback_catch2.cpp#L457).

Focused process tests exercise ordinary-member retraction before callback
observation: the [single-recipient attribute case](../../cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_before_callback_catch2.cpp#L741)
retracts before the receiver polls the update, and the
[fan-out case](../../cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_fanout_catch2.cpp#L968)
retracts one queued timestamped update across two receivers. Neither resigns a
receiver between send and Retract or asserts post-Resign `requestRetraction`.
These test sources were inspected, not run.

### Receiver Resign around a regional interaction callback gate

For a queued regional interaction, distinguish the registry's private TSO
callback gate from entry into the federate's public callback. The embedded path
uses the gate immediately before local callback dispatch. The configured-process
path marks the recipient delivered before sending the `receive_interaction`
frame, so a callback may still be waiting in the client after the server has
crossed its gate. The following is an implementation trace, not a normative
Resign rule.

```mermaid
flowchart TD
  Start([Receiver resigns while regional TSO work exists])
  Start --> Gate{Has this recipient crossed its registry callback gate?}

  Gate -->|No| Unregister[Unregister drops this recipient's time-coordinator queue and in-transit bookkeeping]
  Unregister --> Reject[The later callback gate rejects the nonmember]
  Reject --> Ledger[The retraction record may retain a pending recipient entry; a departed member does not pin payload retention]

  Gate -->|Yes, embedded| Embedded[The local dispatch already crossed its gate; focused receiver-Resign evidence does not establish cancellation after this point]

  Gate -->|Yes, process| Delivered[Server marks the recipient delivered before sending receive_interaction]
  Delivered --> Copy[When region designators are conveyed, create callback Region copies from saved source snapshots and retain a session lease]
  Copy --> Ack{Does callback acknowledgement complete before Resign cleanup?}
  Ack -->|Yes| ReleaseAck[The acknowledgement erases the lease and releases the copies]
  Ack -->|No| Resign[Resign clears joined identity, server event buffers, and the pending retraction-session entry]
  Resign --> Lease[The Resign path does not erase the pending interaction Region-copy lease]
  Lease --> LateAck[A post-Resign acknowledgement fails the joined-member/session check]
  Lease --> RetractCleanup[A successful producer Retract erases the message-keyed lease across session states]
  Lease --> Detach[Detach clears the lease map; lease destruction releases the copies]
```

The embedded registry unregisters the federate from the time coordinator,
which drops its queued, in-transit, and delivered TSO bookkeeping. Its separate
retraction record is not erased by that operation. After membership is removed,
the payload-retention predicate counts only pending recipients that remain
members, and `beginTsoInteractionCallback` rejects a departed receiver. This
source trace does not establish a public HLA callback outcome for a concurrent
Resign racing with a gate that has already succeeded.

The configured-process interaction dispatcher calls the registry gate before
encoding and sending the event. If regional designators are conveyed, it
retains the callback Region-copy lease by message ID. The Resign handler clears
the session's joined identity, server-side event buffers, and pending pushed
Request Retraction recipient entries. Its cleanup currently clears pending
attribute Region-copy leases but omits
`pendingTsoInteractionCallbackRegionDesignatorCopies`. The acknowledgement
handler requires the same joined identity and live membership; the visible
other cleanup paths are a successful producer `Retract`, which erases that
message-keyed lease across session states, and session `detach`, which clears
all outstanding leases. If neither occurs, the lease remains with the attached
session. This is a source-level Resign-cleanup gap to verify with a focused
process test, not an HLA rule or an asserted public callback result.

The focused [process callback-gating case](../../cpp/tests/ieee1516_2025_connection_process_tso_regional_interaction_arrival_catch2.cpp#L69)
and push/retraction case[^worktree-source]
exercise callback admission and retraction before/after admission, but receiver
Resign occurs only during cleanup. The embedded [regional TSO case](../../cpp/tests/ieee1516_2025_timestamped_regional_interaction_tso_retraction_catch2.cpp#L5)
also resigns the receiver during teardown, after its assertions. None tests
receiver Resign after process dispatch has retained callback Region copies and
before acknowledgement.

Sources: [time-coordinator unregister and recipient discard](../../cpp/src/internal/time/federation_time_coordinator.cpp#L108),
[embedded callback gate](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L460),
[pending-recipient payload rule](../../cpp/src/internal/federation/federation_registry.cpp#L1156),
[process gate and Region-copy lease](../../cpp/src/internal/federation/process_federation_service_tso_delivery.cpp#L299),
[process Resign cleanup](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L29),
[acknowledgement identity check and lease release](../../cpp/src/internal/federation/process_federation_service.cpp#L153),
[successful Retract lease cleanup](../../cpp/src/internal/federation/process_federation_service_retraction.cpp#L157),
and [detach cleanup](../../cpp/src/internal/federation/process_federation_service.cpp#L115).

### Timestamped object removal follows a distinct process route

The ordinary attribute/interaction comparison above is not a complete model
for a timestamped Delete Object Instance. When the producer is time regulating
and the effective order of `HLAprivilegeToDeleteObject` is `TIMESTAMP`, the
2025 process service exposes a public retraction designator and divides planned recipients by time
constraint. Constrained recipients remain in the coordinator's TSO deletion
queue; other recipients take a send-time removal path with received order
`RECEIVE`. Both paths eventually retain a process-session route for a later
Request Retraction, but they add it at different points.

This is a 2025 process-profile source trace. It assumes the producer's own
Retract legality gate accepts the request. Its optional Resign branch begins
after that recipient has entered the route cache; it does not describe a
recipient that resigns while its copy is still waiting in the coordinator.

#### Delivery path creates the cached route

The recipient split is a server-side boundary. Time-constrained recipients
enter the coordinator queue; push and pull receive-order recipients establish
their removal and route state through different service paths.

```mermaid
sequenceDiagram
    participant P as Producer
    participant S as 2025 process service
    participant R as Registry
    participant C as Receiver session

    P->>S: deleteObjectInstance(object, tag, T)
    S->>R: Plan deletion snapshot/recipients, allocate message ID
    Note over P,S: Public handle requires regulation and TIMESTAMP delete privilege

    alt Time-constrained recipient on retractable timestamped delete
        R-->>S: Release TSO deletion at recipient frontier
        S->>R: Begin TSO removal(message, recipient)
        S-->>C: Remove event (TIMESTAMP)
        S->>S: Cache route after successful send
    else Recipient bypasses the coordinator queue
        alt Push receive-order events
            S->>R: Begin removal before transport send
            S->>S: Cache route before send
            S-->>C: Remove event (RECEIVE)
        else Pull receive-order events
            S->>S: Queue removal and cache route at service time
            C->>S: Poll removal
            S->>R: Begin removal before returning result
            S-->>C: Remove result (RECEIVE)
        end
    end

    opt Receiver Resigns while session remains attached
        C->>S: Resign
        S->>S: Clear joined identity and this session's server queues
        Note over S: Cached route remains until Detach
    end
```

#### Server Retract sends on the cached route

Once the producer gate accepts Retract, the server applies the deletion-specific
ledger rule and sends `request_retraction` over the cached session route. The
client-side pending-event and callback-bridge decisions are shared with the
[client bridge flow above](#client-bridge-decides-the-public-callback).

```mermaid
sequenceDiagram
    participant P as Producer
    participant S as 2025 process service
    participant C as Receiver client

    P->>S: Retract(handle), while the producer gate still accepts it
    S->>S: Apply shared registry's Delete-specific retraction rule
    S->>S: Erase queued TSO copies, drain cached recipient sessions
    S-->>C: request_retraction over cached route (no membership recheck)
```

The split matters to readers: process-side object removal is not cached by the
same single post-send path used for coordinator-delivered attributes and
interactions. `enqueueObjectInstanceRemovals` records direct-path sessions
while it queues pull work or prepares pushed work; the coordinator-delivered
TSO branch records the session after sending. The client erases a still-buffered
removal event when its Request Retraction arrives. If the event has already
reached the callback bridge, the bridge's per-message state suppresses an
unstarted removal/retraction pair or admits Request Retraction after the
original callback has started. These are process implementation boundaries,
not general HLA callback rules.

Server-side receiver Resign clears that session's joined identity and event
queues, but does not remove the session pointer from other messages' route
entries; `detach` does. Therefore the source supports a later transport
`request_retraction` reaching a still-attached, resigned session. Whether a
public callback follows depends on the client's buffered event and bridge
state; no focused process test establishes that post-Resign outcome.

The public process Delete [delivery-variation case](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1506)
covers both callback models, producer regulation, queued time-constrained
recipients, and preferred receive order. A separate [pending-NMRA
callback-gating case](../../cpp/tests/ieee1516_2025_connection_timestamped_delete_object_instance_catch2.cpp#L1500)
exercises callback disable and re-enable. These cases cover the removal callback
and whether a handle is exposed; neither calls `retract` or resigns the receiver
with a cached route. The separate
[embedded deletion-retraction case](../../cpp/tests/ieee1516_2025_timestamped_delete_object_instance_retraction_catch2.cpp#L6)
does exercise cancellation before grant and deletion reconstitution, but it is
not process-endpoint evidence. No 2010 route or parity is inferred.

Sources: [process delete recipient split](../../cpp/src/internal/federation/process_federation_service_object_instance_deletion.cpp#L81),
[direct removal queue and route-cache insertion](../../cpp/src/internal/federation/process_federation_service_object_instance_removal.cpp#L12),
[coordinator-delivered TSO removal](../../cpp/src/internal/federation/process_federation_service_tso_payload_dispatch.cpp#L172),
[delete retraction and cached-session notification](../../cpp/src/internal/federation/process_federation_service_retraction.cpp#L160),
[receiver Resign queue cleanup](../../cpp/src/internal/federation/process_federation_service_lifecycle.cpp#L29),
[Detach route pruning](../../cpp/src/internal/federation/process_federation_service.cpp#L115),
[client removal-event queueing](../../cpp/src/internal/federation/process_federation_client.cpp#L427),
[callback-bridge retraction admission](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L1575),
and [removal callback retraction state](../../cpp/src/internal/federation/process_federation_callback_bridge.cpp#L2010).

Recipient state remains per-recipient in an additional embedded case: a peer
receives a directed TSO callback, then Resigns and Disconnects, while another
ownership-qualified recipient keeps its queued copy through save/restore and
later receives the original delivery followed by Request Retraction. This is
evidence for that embedded development-profile scenario only. It does not
establish what a departed recipient should receive, and it does not resolve the
separate process endpoint's cached-route cleanup.

In the process implementation, temporarily disabling time regulation does
not erase the producer-owned designator, but the Retract service itself is
rejected until regulation is enabled again. This is implementation behavior;
do not assume a cached client-side time switch authorizes the operation.
The private process TSO completion acknowledgement is a separate recipient
delivery fence, not a retraction callback or public HLA service. Its
callback-throws branch and remaining `inTransit` consequences are documented
in [the process endpoint flow](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#3-a-timestamped-process-delivery-stays-in-transit-through-callback-completion);
that branch is source-derived and has no focused throwing-callback test. For
timestamped attribute updates, the [grouped callback note](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#grouped-timestamped-attribute-callbacks-share-one-completion-boundary)
explains how the final projected passel callback becomes the message-level
completion boundary, including the tested HLA_EVOKED Retract-before-final-
submission case and the separate source-only after-submission timing branch.
The [client request-depth note](HLA-2025-PROCESS-ENDPOINT-REQUEST-AND-CALLBACK-FLOW-GUIDE.md#a-completion-ack-waits-only-for-an-active-process-request)
explains when the private ACK is queued or sent and what the public process
fan-out test establishes about its order relative to a later TAR.

## Evidence map

### Implementation

- [2025 public Retract service and endpoint split](../../cpp/src/internal/runtime/umbra_rti_ambassador_retract_service.cpp#L22)
- [Retract current/requested-time plus actual-lookahead boundary](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L613)
- [Direct Request Retraction callback route and live admission check](../../cpp/src/internal/runtime/umbra_rti_ambassador.cpp#L1121)
- [Per-recipient retraction ledger, pending withdrawal, delivered-recipient notifications, and delete reconstitution](../../cpp/src/internal/federation/federation_registry_tso_delivery.cpp#L115)
- [Process Retract service and process-event cleanup/notification](../../cpp/src/internal/federation/process_federation_service_retraction.cpp#L77)
- [Private per-recipient ledger states](../../cpp/src/internal/federation/federation_registry.hpp#L2203)

### Focused tests

- [Timestamped Send Interaction: retract before delivery, reject after the time boundary, then Request Retraction after delivery](../../cpp/tests/timestamped_send_interaction_retraction_catch2.cpp#L153)
  — freshly rebuilt dedicated target; elevated-filesystem CTest passes all 61 assertions. The sandboxed failure was
  fixture-path canonicalization access denial.
- [Regional timestamped attribute update retracted across FQR, TARA, and NMRA](../../cpp/tests/timestamped_regional_attribute_update_retract_mixed_advance_catch2.cpp#L160)
  — the focused test body asserts successful then terminal Retract, no attribute reflections, and the three grant
  results; this documentation increment did not rerun C++ tests.
- [Mixed interaction fan-out: notify the delivered recipient and suppress the still-queued recipient](../../cpp/tests/ieee1516_2025_request_retraction_interaction_mixed_fanout_catch2.cpp#L3)
  — no fresh result; its aggregate target was not rebuilt to completion.
- [Attribute-update mixed fan-out and delivered-recipient callback](../../cpp/tests/ieee1516_2025_request_retraction_attribute_update_mixed_fanout_catch2.cpp#L5)
- [Public embedded fresh-registry restore after a peer's post-delivery Resign](../../cpp/tests/ieee1516_2025_public_directed_tso_post_delivery_resignation_restore_catch2.cpp#L6)
  exercises both `HLA_EVOKED` and `HLA_IMMEDIATE`: the peer receives its directed TSO callback, Resigns and Disconnects,
  and a different ownership-qualified recipient keeps its queued copy through restore; a later Retract produces one
  valid Request Retraction for that still-joined recipient. This is development-profile test-body evidence, not
  process-route evidence; it does not assert a later callback to the departed peer, and no fresh CTest result was
  captured in this review.
- [Regional interaction recipient filtering and mixed delivery phases](../../cpp/tests/ieee1516_2025_request_retraction_delivered_regional_interaction_fanout_catch2.cpp#L4)
- [Process endpoint cancels a timestamped attribute update before reflection](../../cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_before_callback_catch2.cpp#L366)
- [Timestamped deletion reconstitution and pending-removal cancellation](../../cpp/tests/ieee1516_2025_timestamped_delete_object_instance_retraction_catch2.cpp#L6)
- [Deletion/retraction state across save and restore](../../cpp/tests/ieee1516_2025_public_object_deletion_retraction_restore_catch2.cpp#L7)

The root cause of the sandboxed setup failures is identified: the test process
could inspect the fixture but `std::filesystem::canonical` was denied access
to its repository path. Fresh elevated-filesystem CTests pass for the FDD
materializer (38 assertions), MIM-first federation preparation (22), focused
TSO retraction (61), and five-form terminalization (40). These are current
results for their named cases, not evidence for process-endpoint or 2010
parity. The mixed-fanout and public explicit-MIM integration checks remain
unverified. The first serial aggregate build hit MSVC C1060 (compiler out of
heap) in `ieee1516_2025_federation_registry_timestamped_tso_payload_filesystem_restart_catch2.cpp`;
a later elevated attempt passed that source but was stopped before the aggregate
target completed, to avoid competing with the main implementation work. Do not
resume aggregate builds as part of this documentation goal. The earlier
aggregate FDD result came from a stale executable and should be disregarded;
the fresh dedicated FOM target provides the current result.

These cases demonstrate their named scenarios only. They do not prove that
every timestamped HLA service family is retraction-capable, that every
callback race is covered, or that arbitrary network failures and responder
restarts preserve the same outcomes.

## Edges and explicit limits

- Only timestamped service families that return a usable message retraction
  designator can enter this lifecycle. The [time-management guide's TSO
  producer list](HLA-2025-TIME-MANAGEMENT-GUIDE.md#timestamped-message-lifecycle-and-callback-order)
  identifies the current tested public producer families; that profile-bounded
  list is not an exhaustive standard enumeration or a claim that every
  timestamped service is implemented.
- Retract is producer-owned and requires current time regulation. The public
  handle is not an integer API, even though Umbra uses a private numeric
  message identity internally.
- Pending, delivered, and suppressed recipients are not interchangeable.
  A callback gate or a recipient lifetime check can make a queue phase
  terminal without any user-code delivery.
- Retract can suppress queued fan-out; Request Retraction addresses a message
  that already entered recipient code. Never draw them as duplicate service
  names for the same transition.
- This guide does not restate all grant ordering, GALT/LITS calculations,
  asynchronous-delivery rules, loss cutoffs, or save/restore behavior. Follow
  the links to the separate time-management and save/restore guides for those
  state machines.
- This is an IEEE 1516.1-2025 guide only. The bounded
  [2010 reference-RTI guide](HLA-2010-REFERENCE-RTI-FLOW-GUIDE.md) has a
  separate implementation boundary and makes no retraction-parity claim.
- No Requirements Lab rows were edited or added. Documentation and test links
  here are not new requirement mappings or conformance evidence.

All 16 current diagrams rendered with Mermaid CLI 12.0.0 on 2026-11-06 and
were visually reviewed as local screenshots. The dense process deletion
sequences were adjusted to use shorter participant and message labels while
preserving the timestamped, push, pull, and cached-route distinctions; the
revised charts remain readable without clipping. This includes the two later
additions beyond the prior 14-diagram visual note. Local review does not verify
GitHub's renderer or final page sizing.

[^worktree-source]: This source/test citation refers to a current-worktree change absent from the hosted base and excluded from the documentation-only publication path. See the [hosted source-dependency audit](HLA-2025-HOSTED-SOURCE-DEPENDENCIES.json) for its path and line evidence.
