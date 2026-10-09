# IEEE 1516.1-2010 Reference RTI: Implemented Flow Guide

> **Edition and profile boundary:** this page describes Umbra's separate IEEE
> 1516.1-2010 C++ binding and its small in-process reference RTI, version
> `0.2.0-reference`. It is not a 2025 behavior guide and does not claim that
> this reference slice is a complete or production-ready RTI.

This guide makes the 2010 implementation's current behavior legible without
projecting behavior from the 2025 runtime onto it. The [IEEE 1516.1-2010
Federate Interface Specification](https://standards.ieee.org/ieee/1516.1/3745/)
is the authority for normative service behavior. The diagrams describe only
the source-backed Umbra slice; they are not a substitute for that standard or
a conformance claim.

## Read the two layers separately

The IEEE 1516.1-2010 API groups services around federation execution
management, declaration management, object management, synchronization, and
time management. Umbra's current 2010 shell implements a bounded subset. The
generated ambassador shell delegates that subset to `ReferenceRuntime2010`;
services outside it throw `RTIinternalError` with a not-implemented
diagnostic.

| Service concept | What the 2010 profile currently demonstrates | Important limit |
| --- | --- | --- |
| Connection and federation lifecycle | Connect, create/list/destroy an execution, join, resign, and disconnect. | The execution is process-local; destroy is rejected while members remain. |
| Declaration management | Publish and subscribe to object-class attributes and interaction classes. | Names are registered lazily by the provider directory; they are not checked against a loaded FOM. |
| Object information | Register an object, discover it for matching class subscribers, update values, and reflect subscribed attributes. | The owner is tracked per object, not per attribute; no region filtering or time-managed delivery is modeled. |
| Interaction information | Send a published interaction to members subscribed to that exact interaction-class handle. | No region filtering, timed send, or queued callback pump is modeled. |
| Synchronization | Register a point, announce it to its set, collect successful achievements, then report completion. | This path is source-backed but not exercised by the focused binding-shell smoke test found in this survey. |
| Time management | 2010 integer/float logical-time and interval values plus encoding/marshalling tests exist. | The RTI shell's time-management services are unavailable; value types do not imply a working time-advance state machine. |

## 1. Ambassador connection and execution membership

The 2010 ambassador has its own connected/joined flags. Federation existence
is separate global runtime state: connecting does not create an execution, and
resigning does not disconnect the ambassador.

```mermaid
flowchart LR
  subgraph Session["Ambassador session"]
    A0["Disconnected"] -->|"connect(callback, model)"| A1["Connected, not joined"]
    A1 -->|"join an existing execution"| A2["Connected and joined"]
    A2 -->|"resign(action)"| A1
    A1 -->|"disconnect"| A0
  end
  subgraph Execution["Federation execution"]
    F0["Absent"] -->|"create(name, FOM inputs)"| F1["Exists; zero members"]
    F1 -->|"first join"| F2["Exists; one or more members"]
    F2 -->|"last member resigns"| F1
    F1 -->|"destroy"| F0
  end
  A1 -. "successful join adds a member" .-> F2
  A2 -. "if this is the last member" .-> F1
```

The session and execution tracks are independent. Joining moves both tracks;
resigning moves the session back to connected and only moves the execution to
zero-members state when the last member resigns. Read these transitions with
the implementation guards below:

- `connect` stores the supplied callback object and callback-model value. A
  second connect throws `AlreadyConnected`.
- `join` requires an existing execution and rejects a duplicate non-empty
  federate name or a session that is already a member.
- `disconnect` while joined fails; resign first. The shell remains connected
  after a successful resign.
- `destroyFederationExecution` succeeds only when the execution's member map is
  empty.
- Object-class, attribute, interaction-class, and parameter handles are
  allocated lazily by name lookup. A name accepted by this directory is not
  thereby proven to exist in an input FOM.
- The reference `resign` implementation ignores the requested `ResignAction`
  and erases objects owned by that member. It does not send a removal callback
  before erasing them. Do not interpret this as the standard's general resign
  semantics.
- FOM module arguments are accepted by the public shell but are not loaded by
  this provider. The test deliberately passes `ignored-fom.xml`; the runtime
  header describes a deterministic provider directory pending a 2010 FOM
  loader.

Source: [generated 2010 ambassador shell](../../cpp/generated/rti_ambassador_shell_2010.hpp#L17),
[execution creation and teardown](../../cpp/src/internal/runtime/reference_2010.cpp#L83),
[join](../../cpp/src/internal/runtime/reference_2010.cpp#L213),
[resign and disconnect](../../cpp/src/internal/runtime/reference_2010.cpp#L262),
[lazy class lookup](../../cpp/src/internal/runtime/reference_2010.cpp#L292),
[FOM-input forwarding](../../cpp/generated/rti_ambassador_shell_2010.hpp#L40),
[reference-runtime profile note](../../cpp/src/internal/runtime/reference_2010.hpp#L14).

## 2. Object publication, discovery, update, and reflection

The object path is synchronous in this reference runtime. A matching
subscriber's `FederateAmbassador` method is called from the service
implementation before that service returns.

```mermaid
sequenceDiagram
  participant P as Publisher
  participant R as 2010 Reference RTI
  participant S as Subscriber
  P->>R: publishObjectClassAttributes(class, attributes)
  S->>R: subscribeObjectClassAttributes(class, attributes)
  P->>R: registerObjectInstance(class)
  R->>S: discoverObjectInstance(...), if class subscription exists
  P->>R: updateAttributeValues(object, values, tag)
  R->>R: Check object owner; store values; filter by subscribed attributes
  R->>S: reflectAttributeValues(filtered values, tag, RECEIVE, RELIABLE)
```

Important implementation details:

- Registration requires the member to have published the object class. The
  object is recorded with one owning federate.
- Discovery is sent to other members that have a subscription entry for the
  object's class. Reflection values are then filtered against the receiver's
  subscribed attribute handles; an empty filtered map produces no callback.
- An update is accepted only from the recorded object owner. Values and the
  user tag are forwarded to matching subscribers; the implementation uses
  `RECEIVE` order and `RELIABLE` transportation for this callback.
- `queryAttributeOwnership` reports the object's owning federate through
  `informAttributeOwnership`. `isAttributeOwnedByFederate` also compares the
  object's single owner. These are not per-attribute ownership states or
  ownership-transfer support.
- The callback is a direct call, not a callback-queue event. The smoke test
  covers `HLA_IMMEDIATE`; the shell's evoke methods are unavailable. Although
  `connect` records a callback-model value, this code is not evidence of a
  working `HLA_EVOKED` dispatch path.

Source: [attribute publication](../../cpp/src/internal/runtime/reference_2010.cpp#L448),
[attribute subscription](../../cpp/src/internal/runtime/reference_2010.cpp#L476),
[register and discover](../../cpp/src/internal/runtime/reference_2010.cpp#L502),
[update and reflect](../../cpp/src/internal/runtime/reference_2010.cpp#L549),
[ownership query](../../cpp/src/internal/runtime/reference_2010.cpp#L619),
[object-owner check](../../cpp/src/internal/runtime/reference_2010.cpp#L646),
[callback-model storage](../../cpp/generated/rti_ambassador_shell_2010.hpp#L23),
[unavailable evoke methods](../../cpp/generated/rti_ambassador_shell_2010.hpp#L624).

## 3. Interaction send and receive

```mermaid
flowchart LR
  S["Sending federate"] -->|"sendInteraction(class, values, tag)"| G{"Class exists and sender published it?"}
  G -->|"No"| E["Service throws an API exception"]
  G -->|"Yes"| V{"Every supplied parameter belongs to the class?"}
  V -->|"No"| E
  V -->|"Yes"| F["Visit other federation members"]
  F -->|"Subscribed to this exact class handle"| C["Call receiveInteraction inline"]
  F -->|"Not subscribed or is sender"| N["No callback for this member"]
```

The implementation validates the interaction-class handle, requires the
sender's publication, and validates each supplied parameter handle. It then
delivers to other members subscribed to that exact interaction-class handle.
As with object updates, delivery is synchronous and uses `RECEIVE` / `RELIABLE`
with producing-federate supplemental information. This slice does not model
class-hierarchy subscription matching, regions, transportation selection, or
time-stamped interactions.

Source: [interaction publication](../../cpp/src/internal/runtime/reference_2010.cpp#L404),
[interaction subscription](../../cpp/src/internal/runtime/reference_2010.cpp#L426),
[send and receive](../../cpp/src/internal/runtime/reference_2010.cpp#L584).

## 4. Synchronization-point barrier

```mermaid
flowchart TD
  R["Register label and tag"] --> S{"Select synchronization set"}
  S -->|"No set, or empty set"| ALL["All current execution members"]
  S -->|"Non-empty set"| CHECK["Validate every handle is a current member"]
  CHECK --> ANN["Store point; registration-succeeded callback to registrar; announce to set"]
  ALL --> ANN
  ANN --> WAIT["Collect synchronizationPointAchieved calls"]
  WAIT -->|"successfully = false"| WAIT
  WAIT -->|"Successful set incomplete"| WAIT
  WAIT -->|"Every set member succeeded"| DONE["Call federationSynchronized for set; erase point"]
```

The no-set overload constructs the current member set. In the explicit-set
overload an empty set also expands to all current members, so this profile
cannot express an empty-participant barrier through that overload. A
`successfully = false` call does not mark the member achieved and leaves the
point pending. On full success, the runtime calls `federationSynchronized` for
the set and erases the point after callbacks return. Callback exceptions are
translated to `RTIinternalError`; inspect the source before relying on retry or
reentrancy behavior around a throwing callback.

Less-obvious consequences of the current code:

- Empty or duplicate labels are rejected as `RTIinternalError`; an explicit
  set containing an unknown handle is rejected as `InvalidFederateHandle`.
- A member outside the selected set cannot achieve the point and receives
  `FederateNotExecutionMember` from that service.
- The participant set is captured at registration. A later join does not join
  the pending barrier.
- The registering member receives `synchronizationPointRegistrationSucceeded`
  even when it is not in the explicit synchronization set; only set members
  receive the announcement callback.
- Resignation removes the member but does not edit pending point sets. If a
  participant leaves before achieving, the stored set can therefore remain
  incomplete and prevent the success-size check from completing.
- The point is inserted before registration callbacks. If one throws, the
  registration service reports `RTIinternalError` but the point remains stored,
  potentially with announcements not yet delivered to the whole set.
- The point is erased only after all completion callbacks return. If one throws
  after all achievements, the operation reports `RTIinternalError` and retains
  the point; a repeated successful achievement can retry the completion
  callback loop, including callbacks that returned earlier.

Source: [point registration and announcement](../../cpp/src/internal/runtime/reference_2010.cpp#L114),
[achievement barrier](../../cpp/src/internal/runtime/reference_2010.cpp#L164).

## 5. Timing values are not time-managed execution

The 2010 stream contains `HLAinteger64Time` and `HLAfloat64Time` value and
interval implementations, plus encoding and marshalling smoke tests. Those
artifacts establish value construction/comparison/serialization behavior only.
In the 2010 ambassador shell, `enableTimeRegulation`,
`enableTimeConstrained`, `timeAdvanceRequest`, `queryLogicalTime`, and the
callback-evoke services call `unavailable()`. There is consequently no
source-backed 2010 time-advance state machine to diagram yet ([time-service
stubs](../../cpp/generated/rti_ambassador_shell_2010.hpp#L325),
[advance request](../../cpp/generated/rti_ambassador_shell_2010.hpp#L341),
[logical-time query](../../cpp/generated/rti_ambassador_shell_2010.hpp#L373)).

Likewise, federation save/restore, regions/DDM, and broad ownership services
are outside the implemented reference slice. The generated shell's header
comment states that services outside the bounded slice intentionally throw.
Keep these absent flows visible as gaps rather than borrowing a diagram from
the 2025 runtime.

## Evidence and confidence boundaries

| Claim | Evidence | What it does not prove |
| --- | --- | --- |
| The 2010 binding provides a connected, two-member publisher/subscriber path for discovery, reflection, ownership query, and interaction receive. | [`ieee1516_2010_binding_shell_smoke.cpp`](../../cpp/tests/ieee1516_2010_binding_shell_smoke.cpp#L91), registered as [`umbra.ieee1516e_2010.binding_shell`](../../CMakeLists.txt#L423). | Full API conformance, synchronization callbacks, callback-evoked behavior, or omitted services. |
| 2010 integer/float time values and binary marshalling have isolated smoke coverage. | [`integer-time`](../../cpp/tests/ieee1516_2010_integer_time_smoke.cpp), [`float-time`](../../cpp/tests/ieee1516_2010_float_time_smoke.cpp), and [`time-marshalling`](../../cpp/tests/ieee1516_2010_time_marshal_smoke.cpp) smoke programs. | Federation logical-time advancement or ordering of time-stamped messages. |
| Synchronization-point state and callbacks exist in the reference runtime. | [Registration](../../cpp/src/internal/runtime/reference_2010.cpp#L114) and [achievement](../../cpp/src/internal/runtime/reference_2010.cpp#L164) source. | A focused 2010 test of the barrier; none was found in this bounded survey. |
| The 2010 API is intentionally a bounded shell. | [Generated shell profile comment](../../cpp/generated/rti_ambassador_shell_2010.hpp#L1) and its [`unavailable()` path](../../cpp/generated/rti_ambassador_shell_2010.hpp#L691). | That unsupported services are acceptable for every user or use case. |

The focused binding-shell test uses C `assert` checks and one linear scenario.
Treat it as evidence for that scenario, not an exhaustive state-machine test.
The project test target is an executable smoke test, not a Catch2 lane.

## Deliberate boundary and next evidence

This guide is about the 2010 reference profile only. It does not claim
equivalence with the IEEE 1516.1-2025 implementation and does not import any
2025 timing, callback-queue, region, ownership, or lifecycle behavior. The
Requirements Lab remains outside this documentation goal.

Next useful 2010 evidence, if this profile is expanded, is focused coverage for
synchronization-point registration/announcement/achievement, resign cleanup,
callback-model behavior, and the currently unavailable service families. Until
those services exist, keep them as explicit gaps rather than drafting
speculative state diagrams.
