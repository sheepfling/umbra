# Portable C++ RTI conformance TCK

The first portable C++ TCK slice is in
[`packages/hla-rti-cpp-tck`](../../packages/hla-rti-cpp-tck/). It is a
separate consumer package, not a collection of source-tree implementation
tests. Its source depends only on the official IEEE 1516.1-2025 C++ API
headers and the C++ standard library.
The runner uses a standard `RTIambassador` plus a direct
`FederateAmbassador` implementation. Factory and `RtiConfiguration` objects
are used only through the same official API to construct and configure those
objects.

## Green P0–P6 ordinary-service, time, FOM, DDM, synchronization, callback, and save/restore boundary coverage

The initial green slice exercises:

- pre-connect `NotConnected` boundaries for representative listing, lookup,
  name-reservation, and disconnect services; all four official `connect`
  overloads (including standard no-credentials and configuration forms),
  unsupported callback-model rejection, duplicate-connect handling,
  reconnect-after-disconnect, empty-queue callback servicing, and both
  standard callback models;
- official basic and composite data-element encode/decode round trips;
- missing, malformed, namespace-invalid, and duplicate-declaration FOM inputs;
- federation Create, Join, query, Resign, Destroy, automatic-resign directive
  pre-connect/pre-join boundaries and round trips, duplicate-create/join and
  destroy-while-joined failures, missing-execution reporting, focused
  execution/member listing callback boundaries, and stale evoked-report cleanup;
- ordinary self-resignation is checked not to synthesize the distinct standard
  `FederateAmbassador::federateResigned` callback; RTI-initiated forced
  resignation remains an adapter-triggered boundary rather than a portable
  public-service action;
- explicit standard-MIM federation creation through an adapter-supplied MIM
  module, shared MOM declaration handles, and caller-FOM preservation;
- standard MOM service reporting for successful ordinary `SendInteraction`,
  regional `SendInteractionWithRegions`,
  `UpdateAttributeValues`, `RequestAttributeValueUpdate`, `ReleaseMultipleObjectInstanceNames`, `ReleaseObjectInstanceName`, `ReserveObjectInstanceName`, `RegisterObjectInstance`, `DeleteObjectInstance`,
  `LocalDeleteObjectInstance`,
  and timestamped `SendInteraction` services, plus ordinary
  `UpdateAttributeValues` failure reports,
  including `HLAreportServiceInvocation`, all seven standard report parameters,
  RTI-generated producer metadata, reliable transport, and typed report values;
- adapter-selected logical-time factory values with pre-connect and pre-join
  lifecycle boundaries, standard `HLAinteger64Time`/`HLAfloat64Time` value and
  factory values and default/named/unknown factory-selection checks,
  concrete time/interval copy and assignment independence, zero/epsilon
  mutators, interval differences, and direct interval encoding,
  variable-length and direct-buffer encode/decode parity,
  encoded-length checks, truncated-buffer rejection, boundary transitions,
  public `HLAlogicalTime`/`HLAlogicalTimeInterval` data-element round trips,
  nested-buffer boundaries, clone/copy independence, incompatible-type rejection,
  comparison, interval arithmetic/order, illegal underflow/overflow boundaries, all
  time-service pre-connect/pre-join boundaries, time roles, logical-time/
  lookahead queries, pre-membership lookahead boundaries, deferred lookahead
  decrease, alternate advance entry points, asynchronous-delivery controls,
  basic advance/grant, and standard duplicate-role, in-progress, backward-time,
  and duplicate-disable failure boundaries;
- federate, object, attribute, interaction, and parameter handle lookups in both
  directions, pre-connect and pre-join lifecycle boundaries across the full
  public lookup family, standard invalid-name/handle boundaries, all public
   handle decoder pre-connect and pre-join lifecycle boundaries, ordinary handle
   encode/decode, direct-buffer and `VariableLengthData&` encoding, encoded-length
   and truncated-buffer checks, copied-handle equality/hash/ordering stability,
   valid `AttributeHandleSet` copy/assignment/lookup/erase semantics, independent
   `AttributeHandleValueMap` and `ParameterHandleValueMap` value storage,
   normalization stability, available-dimension boundaries, and order and
   transportation handle lookups;
- ordinary object and interaction declarations, including pre-connect and
  pre-join lifecycle boundaries, active/passive subscription, withdrawal, and
  invalid-class/attribute failure boundaries;
- dimension, region, and message-retraction handle encoding/decoding, including
  direct-buffer and `VariableLengthData&` parity, encoded-length and
  truncated-buffer checks, copied-handle value semantics, and empty-input
  rejection;
- pre-connect and pre-join lifecycle boundaries across the standard regional
  object, attribute, interaction, range, registration, association, and
  value-update services, including the timestamped regional-send overload, in
  addition to the existing region lifecycle and routing checks;
- ordinary and named object registration and discovery, including pre-connect
  and pre-join registration, deletion, and object-name reservation boundaries,
  invalid-class, unreserved-name, and unknown-object deletion failure
  boundaries;
- single and multiple object-name reservation, release, reuse, and in-use
  failure callbacks;
- instance/class Attribute Value Update requests and Provide Attribute Value
  Update callbacks, with invalid class/attribute boundaries and response
  reflections;
- local deletion, receive-order deletion/removal, and resign-time object
  cleanup;
- ordinary attribute Update/Reflect with value, tag, producer, and transportation checks;
- ordinary interaction Send/Receive with pre-connect and pre-join send
  boundaries, parameter, tag, producer, and transportation checks;
- standard Delay Subscription Evaluation switch composition, late ordinary
  interaction subscription retention, callback-boundary subscription
  rechecking, and suppression after unsubscribe under both callback models;
- ordinary timestamped interaction Send/Receive, invalid interaction-class and
  retraction-handle boundaries, constrained grant delivery, timestamp/order
  metadata, retraction designators, and Request Retraction callbacks;
- timestamped interaction Send/Receive with an adapter-declared custom FOM
  transportation type, constrained grant delivery, standard callback metadata,
  retraction identity, and no region metadata;
- queued ordinary timestamped interaction delivery surviving producer resignation
  for one constrained TAR recipient, preserving payload, producer, timestamp/order,
  transport, and retraction metadata;
- queued ordinary timestamped interaction delivery to each constrained recipient
  after producer resignation, with independent TAR/NMR frontiers and preserved
  payload, producer, timestamp/order, and retraction metadata;
- ordinary timestamped interaction delivery before Flush Queue, Time Advance
  Request Available, and Next Message Request Available grants, including
  callback ordering, logical-time queries, and retraction metadata;
- ordinary timestamp ordering across multiple interaction producers for each
  constrained recipient, with equal-timestamp order left unspecified and
  producer/tag identity preserved;
- ordinary timestamped attribute Update/Reflect with invalid object/attribute
  boundaries, immediate and constrained recipients, metadata, grant ordering,
  and retraction;
- ordinary timestamped Delete/Remove Object Instance with an unknown-object
  boundary, object identity removal, reconstitution after retraction, metadata,
  and terminal deletion;
- Flush Queue Request, Time Advance Request Available, and Next Message Request
  Available, including reflection-before-grant ordering and logical-time queries;
- timestamped directed interaction delivery through Flush Queue Request, Time
  Advance Request Available, and Next Message Request Available, including
  target identity, order/transport metadata, callback ordering, and retraction;
- Next Message Request delivery of the next queued timestamped interaction before
  its grant, including callback ordering, logical-time query, and retraction
  boundary;
- Query GALT and Query LITS undefined and minimum no-TSO boundaries, pending
  regulator advances, grant stability, and resignation/disable transitions;
- pre-connect and pre-join synchronization-service boundaries, global and
  explicit-set synchronization-point registration and announcement, invalid
  explicit-member failure, duplicate-label failure, achievement, and
  federation-synchronized completion;
- callback disable/enable gating around a delivered interaction under both
  callback models;
- asynchronous-delivery enable/disable boundaries, receive-order callback
  gating, `evokeCallback`, `evokeMultipleCallbacks`, and time-advance release;
- pre-connect and pre-join boundaries across the standard save/restore control
  services, including the timestamped save-request overload, plus untimed and
  timestamped federation save/restore request,
  status, begun/complete/not-complete, abort, failure, queued delivery,
  retraction, and post-restore federate-handle rebinding;
- ordinary directed interaction publication, selective and universal subscription,
  pre-connect and pre-join directed declaration and send boundaries,
  target-object routing, receive callbacks, withdrawal, and invalid
  object/interaction-handle boundaries;
- timestamped directed interaction publication/subscription, invalid
  class/target/retraction-handle boundaries, selective and universal target
  routing, constrained grant delivery, timestamp/order metadata, and retraction;
- ordinary receive/timestamp order controls, default and per-instance order and
  transport controls, request confirmations, invalid class/object/attribute/
  transport boundaries, query reports, and delivered transport identity;
- object-class, attribute, scope, interaction, conveyed-region, and service
  reporting support-switch state, active/passive declaration transitions,
  registration and interaction turn-on/turn-off callbacks, named update-rate
  turn-up callbacks, and active per-attribute update-rate queries;
- pre-connect and pre-join boundaries across the standard advisory and support
  switch accessors;
- ordinary and timestamped interaction and attribute-update retention across
  late subscription, with current-subscription rechecking at the ordinary
  callback or time-constrained grant boundary under the standard Delay
  Subscription Evaluation switch;
- named-rate lookup, active/passive/default subscription effects, unsubscribe
  reset, per-federate isolation, and invalid rate/object/attribute boundaries;
- passive subscription delivery suppression and ordinary invalid-class,
  invalid-object, invalid-attribute, non-owner, unpublication, and
  undefined-parameter boundaries;
- adapter-triggered connection loss, lost-member cleanup, and survivor usability;
- ordinary attribute ownership queries and checks, including invalid
  object/attribute boundaries;
- unowned query reports and unconditional-divestiture assumption offers;
- If Available acquisition success and unavailability while another federate
  retains ownership;
- negotiated acquisition/divestiture with confirmation tags;
- direct Divestiture If Wanted transfer to a pending acquirer;
- If Available acquisition after unconditional divestiture;
- owner denial, requester Unavailable notification, acquisition cancellation,
  and negotiated-divestiture cancellation;
- rich valid FOM hierarchy, inherited declarations, dimensions, update rates,
  transportation metadata, advisory switches, and representative typed
  attribute/parameter delivery;
- FOM module composition at federation creation and additional-module
  composition at join.
- pre-connect and pre-join region-service boundaries, two-dimensional region
  creation, dimension metadata, bound changes, commit, query, and deletion
  lifecycle;
- region-qualified object publication/subscription, named registration,
  regional discovery and Update/Reflect, regional value requests, and region
  association changes;
- three-federate regional ownership transfer, former-owner update rejection,
  default-source delivery, and explicit replacement update-region association;
- attribute In/Out Of Scope callbacks, regional source and subscription
  transitions, per-federate switch suppression, and stale evoked-callback
  handling;
- region-qualified ordinary interaction routing, overlap and disjoint filtering,
  conveyed region designators, and declaration changes;
- timestamped regional interaction delivery, constrained grants, retraction,
  timestamp/order metadata, and conveyed region designators;
- queued timestamped interaction delivery surviving producer Time Regulation
  disable/re-enable with changed lookahead, including Query Lookahead, grant
  ordering, metadata, and retraction;
- queued timestamped object deletion/removal surviving producer Time
  Regulation disable/re-enable with changed lookahead, including Query
  Lookahead, removal-before-grant ordering, object identity cleanup, metadata,
  and terminal retraction;
- queued timestamped directed interaction delivery surviving producer Time
  Regulation disable/re-enable with changed lookahead, including target
  routing, Query Lookahead, receive-before-grant ordering, metadata, and
  terminal retraction;
- queued timestamped regional interaction delivery surviving producer Time
  Regulation disable/re-enable with changed lookahead, preserving the
  source-region snapshot, Query Lookahead, receive-before-grant ordering,
  metadata, and terminal retraction;
- timestamped regional interaction delivery through Flush Queue Request, Time
  Advance Request Available, and Next Message Request Available, including
  conveyed source-region metadata, grant/query bounds, callback ordering, and
  backward-time failures;
- explicit-source timestamped regional interaction no-overlap behavior,
  including valid pre-delivery retraction and terminal retraction-handle
  classification without a fabricated receiver callback;
- queued timestamped regional interaction subscription replacement,
  suppressing the queued old-region passel without retargeting or Request
  Retraction, then delivering a later replacement-region passel;
- queued timestamped regional interaction delivery surviving producer
  resignation, with an independent regulator releasing the recipient frontier
  and preserving producer, source-region, and retraction metadata;
- timestamped regional interaction delivery before ordinary TAR and NMR grants,
  with independent recipient frontiers, source-region metadata, and logical-time
  query results;
- queued timestamped regional Update/Reflect delivery surviving producer
  resignation, with object, producer, source-region, payload, timestamp/order,
  and retraction metadata preserved;
- timestamped regional attribute Update/Reflect, overlap and disjoint filtering,
  source-region metadata, constrained grant ordering, time-constrained
  re-enable, and retraction;
- timestamped regional attribute delivery through Flush Queue Request, Time
  Advance Request Available, and Next Message Request Available, including
  grant/query bounds, callback ordering, and backward-time failures;
- queued timestamped regional updates retaining their original source
  association across replacement, with later updates carrying the replacement
  region;
- queued timestamped regional Update/Reflect surviving producer Time Regulation
  disable/re-enable with changed lookahead, preserving the original source
  region snapshot and retraction metadata;
- timestamped default-region Update/Reflect through Flush Queue Request, Time
  Advance Request Available, and Next Message Request Available, with
  supplied-empty region metadata and grant ordering;
- queued timestamped default-region Update/Reflect surviving Time Constrained
  disable/re-enable, with supplied-empty source metadata, grant ordering, and
  retraction;
- queued timestamped default-region interaction delivery surviving producer
  resignation for each constrained recipient, preserving supplied-empty source
  metadata, producer, payload, timestamp/order, and retraction identity;
- queued timestamped default-region interaction delivery surviving Time
  Constrained disable/re-enable, with callback-before-grant ordering and
  terminal retraction;
- queued timestamped default-region interaction delivery surviving Time
  Regulation disable/re-enable with changed lookahead, Query Lookahead, and
  terminal retraction;
- queued timestamped object deletion delivered before independent Flush Queue
  Request, Time Advance Request Available, and Next Message Request Available
  grants, with removal metadata, callback ordering, logical-time queries, and
  terminal retraction;
- delivered timestamped interaction copies producing Request Retraction
  callbacks while queued fan-out copies are suppressed, including the
  post-resignation immediate-delivery boundary;
- timestamped default-region interaction mixed fanout between an immediate and
  constrained regional recipient, with delivered-copy Request Retraction,
  pending-copy suppression, and recipient-local callback ordering;
- timestamped default-region Update/Reflect mixed fanout between an immediate
  and constrained regional recipient, with delivered-copy Request Retraction,
  pending-copy suppression, and supplied-empty source metadata;
- timestamped default-region interaction delivery through a constrained grant,
  with supplied-empty source-region metadata and the terminal retraction
  boundary;
- timestamped default-region interaction delivery through Flush Queue Request,
  Time Advance Request Available, and Next Message Request Available, with
  supplied-empty region metadata, callback ordering, and grant/query bounds;
- zero-dimensional, partial, wrong-context, foreign-region, and in-use region
  boundary cases.

The verified lane currently runs 352 promoted scenarios (704 callback-model
cases) under `HLA_EVOKED` and `HLA_IMMEDIATE`. Shared ordinary-service runner IDs are the same IDs used by
the Java TCK; C++-specific time, DDM, synchronization, callback, and
save/restore scenarios retain distinct IDs.
The promoted `cpp-tck.federation-teardown-isolation` scenario creates two
similarly named executions, establishes independent active named update-rate
histories, destroys only the first execution, and confirms that the surviving
execution still suppresses an in-interval update. Three repeat focused runs
passed all 6/6 callback-model cases; the source uses only the official C++ API,
standard library, and adapter-supplied rich FOM.
The promoted `cpp-tck.mixed-update-rate-subscriptions` scenario uses the
adapter-supplied rich FOM to verify ordinary per-attribute update-rate gating:
the first update delivers both the default-rate reliable and active named-rate
best-effort attributes, while the next update delivers only the reliable
attribute. It accepts standard callback splitting by transport and checks
object, tag, producer, value, and transport identity. Its focused portable
artifact passed 2/2 callback-model cases, and the matching native oracle passed
37 assertions.
The pure contract twins
`cpp-tck.federation-teardown-isolation-contract`,
`cpp-tck.mixed-update-rate-subscriptions-contract`, and
`cpp-tck.timestamped-attribute-update-rate-reduction-contract` reuse those
verified update-rate behaviors through official C++ interfaces only. Their
focused six-scenario lane passed 12/12 callback-model cases. The pure
`cpp-tck.explicit-mim-creation-contract` and
`cpp-tck.federation-mom-current-fdd-contract` runners add the same
standard-only boundary for adapter-supplied MIM composition and the federation
MOM current-FDD surface; their focused four-scenario lane passed 8/8
callback-model cases. The promoted
`cpp-tck.service-report-regional-interaction-contract` and
`cpp-tck.service-report-regional-interaction-subscription-contract` runners add
the same standard-only boundary for successful regional interaction service
reporting, including typed MOM invocation metadata and regional delivery or
subscription-report state. Their focused four-scenario lane passed 8/8
callback-model cases. The full promoted aggregate passed 662/662 CTest cases
with 662 direct passes plus the
two expected connection-loss skips.
The promoted `cpp-tck.federation-mom-save-conditionals-contract` and
`cpp-tck.joined-federate-mom-federate-state-save-restore-contract` runners
make the standard save/restore MOM routes independently selectable. Their
focused four-scenario lane passed 8/8 callback-model cases. The catalog-wide
aggregate now passes 666/666 runnable CTest cases and records 666 direct passes
plus the two expected connection-loss skips.
The promoted `cpp-tck.fom-empty-module-validation` scenario verifies the
standard `InvalidFOM` boundary for an empty create-module set and then proves
that the rejected request did not reserve the federation name by creating and
joining the same execution with the adapter-supplied FOM. It uses only the
official `RTIambassador`/`FederateAmbassador` API.
The promoted `cpp-tck.custom-transportation-interaction-delivery` scenario
uses the adapter-declared rich FOM to verify custom transportation handle/name
round-trips, ordinary interaction publication/subscription/send delivery,
received transportation identity, and the standard transportation-type query
report. Its focused portable artifact passed 2/2 callback-model cases, and the
matching native oracle passed 42 assertions; the reusable source remains on the
official C++ API and standard library boundary.
The promoted `cpp-tck.custom-transportation-timestamped-delivery` scenario uses
the same adapter-declared rich FOM to verify timestamped interaction
publication/subscription/send delivery, constrained grant timing, payload/tag/
producer/time/order/retraction metadata, and the adapter-declared custom
transportation identity without region metadata. Its focused portable artifact
passed 2/2 callback-model cases, and the matching native oracle passed 48
assertions; the reusable source remains on the official C++ API and standard
library boundary.
The promoted `cpp-tck.custom-transportation-timestamped-directed-delivery`
scenario uses the same adapter-declared rich FOM to verify timestamped directed
interaction publication/subscription/send delivery to a registered target,
constrained grant timing, payload/tag/target/producer/time/order/retraction
metadata, and the adapter-declared custom transportation identity. Its focused
portable artifact passed 2/2 callback-model cases, and the matching native oracle
passed 42 assertions; the reusable source remains on the official C++ API and
standard library boundary.
The promoted `cpp-tck.custom-transportation-regional-attribute-delivery`
scenario uses the adapter-declared rich FOM and DDM dimensions to verify ordinary
regional attribute publication/subscription/update delivery, conveyed source
region metadata, overlap filtering, and custom transportation identity. Its
focused portable artifact passed 2/2 callback-model cases, and the matching
native oracle passed 51 assertions; the reusable source remains on the official
C++ API and standard library boundary.
The promoted `cpp-tck.custom-transportation-regional-interaction-delivery`
scenario uses the same adapter-declared rich FOM and DDM dimensions to verify
ordinary regional interaction publication/subscription/send delivery, parameter,
tag, producer, conveyed source-region, and custom transportation metadata. Its
focused portable artifact passed 2/2 callback-model cases, and the matching
native oracle passed 40 assertions; the reusable source remains on the official
C++ API and standard library boundary.
The promoted `cpp-tck.custom-transportation-timestamped-regional-attribute-delivery`
scenario reuses the standard regional timestamped-attribute oracle with
adapter-declared object, attribute, interaction, parameter, transportation, and
DDM names. It verifies regional publication/subscription/update delivery,
timestamp ordering, region metadata, retraction, and the custom transportation
identity. Its focused portable artifact passed 2/2 callback-model cases, and the
matching native oracle passed 60 assertions; the reusable source remains on the
official C++ API and standard library boundary.
The transport-change duplicate-request negative case is asserted in evoked
mode, where the request remains pending until its confirmation callback;
immediate mode verifies the committed confirmation, query, and delivery state.
The federation-list, federate-lookup, object-name-reservation, object-registration-discovery, Allow Relaxed DDM, multi-attribute, three-dimensional, and default-region regional object update, timestamped interaction, timestamped regional attribute/object-management cases, alternate-advance,
Next Message Request, Query GALT/LITS, three FOM scenarios, thirty-two DDM scenarios,
synchronization-point, asynchronous-delivery, save/restore interlock, and connection-loss
scenarios are C++ adapter extensions.
Time scenarios are explicitly marked
`skipped` unless the adapter selects a logical-time implementation. Connection
loss is explicitly marked `skipped` unless the adapter supplies a fault trigger.
The IEEE API defines the callback and cleanup surface but does not define a
fault-injection service.
The current-package adapter carries that optional setup through
`HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER` and
`HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED`; the portable executable
only observes the standard callback, while the adapter owns the fault fixture.
The current-process adapter’s shell-free Python harness passed both callback
models and recorded `.build\cpp-tck-all\connection-loss-current-process-python.json`;
that is adapter-specific evidence: it makes the connection-loss case green for
that adapter, while the default no-fixture installed-package baseline remains
explicitly skipped because the IEEE API does not define fault injection.

The ownership scenario is `java-tck.ownership`. It keeps the reusable source
on the standard `RTIambassador`/`FederateAmbassador` boundary and uses only the
ordinary `DivestAcquire` attribute in the portable FOM. It first verifies the
standard `NotConnected` and `FederateNotExecutionMember` boundaries across the
ownership-service matrix, then verifies ownership reports and user tags across
the negotiated, immediate-availability, denial, and cancellation paths; no DDM,
time-management, MOM, or provider-private surface is required.

HLA ownership is defined for object attributes, not as a separate whole-object
owner. The object-management completion therefore covers object-instance
identity and deletion, while the ownership scenario covers transfer of the
ordinary application attribute.

The promoted `cpp-tck.divestiture-if-wanted-mixed-acquirers` scenario uses the
adapter-supplied multi-attribute FOM to exercise `attributeOwnershipDivestitureIfWanted`
with independently pending acquisitions. Evoked mode combines regular and
If Available requests; immediate mode uses regular pending acquisition for the
second requester because an immediate If Available callback closes its pending
window synchronously. Both modes verify exact attribute-set transfer, callback
tags, ownership state, stale release suppression, and the standard
`RTIambassador`/`FederateAmbassador` boundary.

The promoted `cpp-tck.negotiated-divestiture-partial-acquisition-cancellation`
scenario translates the standard multi-attribute negotiated-divestiture path.
The requester acquires both attributes, cancels only the first pending
acquisition, and the owner confirms only the retained second attribute. Both
callback models verify the cancellation callback, exact transfer set,
acquisition and confirmation tags, and final ownership state before canceling
the residual negotiated divestiture.

The promoted `cpp-tck.divestiture-if-wanted-mixed-acquirers-contract` and
`cpp-tck.negotiated-divestiture-partial-acquisition-cancellation-contract`
runners expose those two green ownership paths as independently selectable
pure standard C++ contract twins. Their focused four-case lane passed 4/4
callback-model cases. The catalog-wide promoted gate passed 670/670 ordinary
CTest cases and recorded 670 direct passes plus the two expected
adapter-managed connection-loss skips; the strict catalog validator reported
`valid=true` for
`.build\cpp-tck-all\verified-evidence-ownership-contract-expansion.json`.

The candidate `cpp-tck.ownership-acquisition-cancellation-transfer-race`
translates the native standard-only race oracle using the ordinary object and
ownership API. In evoked mode, cancellation starts from the owner’s release
callback and `attributeOwnershipDivestitureIfWanted` wins the terminal race;
the requester receives only the acquisition notification with the divestiture
tag. Immediate mode is retained as an explicit skip because synchronous
delivery closes the concurrent pending-transfer window before the service can
win that race.

The candidate timed regular-candidate continuation contract twin exercises the
same standard API boundary across regional timestamped update, timed save and
restore, ownership continuation, and Flush Queue delivery. Its focused lane
recorded 2 evoked passes and 2 explicit immediate-model skips; provider, FOM,
endpoint, logical-time, and callback configuration remain adapter inputs.

The candidate timed pre-delivery cancellation contract twin exercises the same
standard API boundary through negotiated cancellation before confirmation
delivery after restore. Its focused lane recorded 2 evoked passes and 2
explicit immediate-model skips; provider, FOM, endpoint, logical-time, and
callback configuration remain adapter inputs.

The candidate timed confirmation-cancellation contract twin exercises the same
standard API boundary through cancellation after confirmation request delivery
and restore. Its focused lane recorded 2 evoked passes and 2 explicit
immediate-model skips; provider, FOM, endpoint, logical-time, and callback
configuration remain adapter inputs.

The promoted `cpp-tck.unnamed-join-overload` scenario isolates the standard
unnamed federation Join overload. It verifies that a federate can join without
supplying a name, that the generated federate name and handle are reversible
through standard lookups, and that the unnamed member appears alongside a
named control member in the standard membership report. The source uses only
the official `RTIambassador`/`FederateAmbassador` API and adapter-supplied
FOM/time configuration.

The promoted `cpp-tck.unnamed-join-overload-contract` runner exposes that
generated-name federation membership boundary as an independently selectable
pure standard C++ contract. The promoted
`cpp-tck.standard-order-and-transportation-lookups-contract` runner exposes
mandatory order and transportation lookup lifecycle, round-trip, and invalid
input boundaries. The promoted `cpp-tck.callback-controls-contract` runner
exposes callback enable/disable gating and interaction release. All three use
only official IEEE C++ headers and the standard library, with provider, FOM,
endpoint, and callback configuration remaining adapter-owned.

The promoted `cpp-tck.federation-list-services` scenario isolates the public
federation-listing surface from the broader lifecycle scenario. It creates two
adapter-FOM federation executions, joins one member, verifies execution names
and selected logical-time implementations, checks member names and federate
types, exercises the missing-execution report, and confirms that a queued
evoked report is discarded when its connection is closed. The same assertions
run at the immediate callback boundary and remain limited to the official
`RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.object-name-reservation-lifecycle` scenario isolates the
standard object-instance name reservation surface from named registration. It
checks pre-connect and pre-join service boundaries, standard name validation,
asynchronous single-name contention, mixed multiple-name success/failure
callbacks, atomic multiple release, reuse after release, and reservation
cleanup on resignation under both callback models. Names and the federation
FOM come from the adapter-owned execution setup; the source uses only the
official `RTIambassador`/`FederateAmbassador` API. The promoted
`cpp-tck.object-name-reservation-lifecycle-contract` runner exposes the same
reservation, release, contention, reuse, callback, and resignation boundaries
as an independently selectable pure standard contract.

The promoted `cpp-tck.object-registration-discovery-lifecycle` scenario
isolates hierarchy-aware registration/discovery from the ordinary
object-management case. Against the adapter’s rich FOM it verifies exact-class
and superclass discovery identities, invalid-class and unpublished
registration boundaries, evoked callback cancellation, late subscription
discovery, stable object/class/name lookups, and duplicate-discovery
suppression. The same source runs under both callback models and uses only the
official `RTIambassador`/`FederateAmbassador` API. The promoted
`cpp-tck.object-registration-discovery-lifecycle-contract` runner exposes the
registration, discovery, lookup, callback-servicing, and lifecycle boundaries
as an independently selectable pure standard contract.

The promoted `cpp-tck.resign-delete-objects` scenario isolates the standard
resign-time deletion boundary. It verifies that a federate owning a registered
object cannot resign with `NO_ACTION`, that `DELETE_OBJECTS` removes the object,
and that the surviving federate receives exactly one ordinary removal callback
with the resigning producer and empty tag before object-name lookup becomes
invalid. The source uses only the official `RTIambassador`/`FederateAmbassador`
API and adapter-supplied FOM names.

The promoted `cpp-tck.resign-unconditional-divestiture` scenario isolates the
standard ownership boundary for an unconditional resign. It verifies that
`NO_ACTION` is rejected while the federate owns the delete privilege, that
`UNCONDITIONALLY_DIVEST_ATTRIBUTES` keeps the object alive and offers both the
value and delete-privilege attributes, and that a surviving publisher can
acquire the value with its tag preserved. The source uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied FOM names.

The promoted `cpp-tck.resign-delete-objects-contract`,
`cpp-tck.resign-unconditional-divestiture-contract`, and
`cpp-tck.final-federate-resignation-cleanup-contract` runners expose standard
resignation-time deletion, unconditional divestiture, final-federate cleanup,
ownership, object-name reuse, and identity boundaries as independently
selectable pure standard C++ contracts. They use only official IEEE C++ headers
and the standard library; provider, FOM, endpoint, and callback configuration
remain adapter-owned.

The promoted `cpp-tck.regional-attribute-value-update-response-recheck` scenario
continues the same standard route through response delivery. It requests a
provider response while the subscriber overlaps the registered object, moves
the committed subscriber region out of overlap before the response is sent,
and verifies that no reflection is delivered. Restoring overlap and issuing a
fresh request then verifies the returned value, tag, reliable transport, and
producer identity. It uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied DDM FOM.

The promoted `cpp-tck.resign-cancel-pending-acquisition` scenario isolates
the standard `CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` resignation action. It
leaves an ordinary acquisition pending, resigns the requester, verifies that
the current owner remains the owner, and pumps both callback models to prove
that no stale release request crosses the cancellation boundary. The source
uses only the official `RTIambassador`/`FederateAmbassador` API and adapter-
supplied FOM names.

The promoted `cpp-tck.resign-cancel-if-available-pending` scenario applies
the same resignation action to an If Available acquisition. It verifies that
the pending requester-side reservation is removed without a stale acquisition
notification or unavailable callback, while the existing owner retains the
attribute. The source uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied FOM names.

The promoted `cpp-tck.resign-cancel-negotiated-pending` scenario applies the
same resignation action after a regular acquisition has entered a negotiated
divestiture path. It verifies that the departing requester leaves no stale
owner-side divestiture or release callback and that the existing owner retains
the attribute. The source uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied FOM names.

The promoted
`cpp-tck.resign-pending-acquisition-rejection-contract`,
`cpp-tck.resign-cancel-pending-acquisition-contract`,
`cpp-tck.resign-cancel-if-available-pending-contract`, and
`cpp-tck.resign-cancel-negotiated-pending-contract` runners expose standard
pending-acquisition rejection and cancellation boundaries as independently
selectable pure standard C++ contracts. They use only official IEEE C++ headers
and the standard library while provider, FOM, endpoint, and callback
configuration remains adapter-owned.

The promoted
`cpp-tck.negotiated-divestiture-cancellation-contract` and
`cpp-tck.negotiated-divestiture-pre-delivery-cancellation-contract` runners
expose standard negotiated ownership-cancellation and pre-delivery callback-
suppression boundaries as independently selectable pure standard C++ contracts.
They use only official IEEE C++ headers and the standard library while provider,
FOM, endpoint, and callback configuration remains adapter-owned.

The promoted `cpp-tck.allow-relaxed-ddm` scenario composes the standard
`Allow Relaxed DDM` support switch from the adapter’s switch FOM with the
adapter’s dimensional FOM. It verifies that touching regions are admitted for
ordinary regional object updates and interactions only when the switch is
enabled, that a strict positive gap remains out of scope, and that delivered
callbacks preserve the source-region designator. The source uses only the
official `RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.region-lifecycle-contract` runner exposes the region and
dimension lifecycle as an independently selectable pure standard C++ contract.
It verifies the standard pre-connect and pre-join boundaries, dimension
metadata and bounds, region creation and commit, range validation, handle-set
queries, region-qualified service boundaries, and deletion. The source uses
only official IEEE C++ headers and the standard library; the provider,
dimensional FOM, endpoint, callback, and logical-time configuration remain
adapter-owned.
Its focused base-and-contract lane passed 4/4 callback-model cases. The
promoted `cpp-tck.regional-unpublish-region-release-contract` runner adds the
standard regional publication and region-release dependency boundary as an
independently selectable pure C++ contract. Together, the focused base-and-
contract lane passed 4/4 callback-model cases, and the catalog-wide promoted
gate passed 674/674 CTest cases with 674 direct passes and only the two
expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-regional-unpublish-region-release-contract.json`.
Both sources use only official IEEE C++ headers and the standard library; the
provider, FOM, endpoint, callback, and logical-time configuration remain
adapter-owned.
The promoted `cpp-tck.regional-object-update-contract` runner adds the
standard regional object publication, subscription, discovery, Update/Reflect,
value-request, and region-reassociation boundary as an independently
selectable pure C++ contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the catalog-wide promoted gate passed 676/676 CTest
cases with 676 direct passes and only the two expected adapter-managed
connection-loss skips in
`.build\cpp-tck-all\verified-evidence-regional-object-update-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
provider, dimensional FOM, endpoint, callback, and logical-time configuration
remain adapter-owned.
The promoted `cpp-tck.regional-attribute-value-request-filtering-contract`
runner adds the standard regional Request Attribute Value Update filtering
boundary as an independently selectable pure C++ contract. Its focused
base-and-contract lane passed 4/4 callback-model cases; the catalog-wide
promoted gate passed 678/678 CTest cases with 678 direct passes and only the
two expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-regional-attribute-value-request-filtering-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
provider, dimensional FOM, endpoint, callback, and logical-time configuration
remain adapter-owned.
The promoted `cpp-tck.regional-attribute-value-update-response-recheck-contract`
runner adds the standard regional attribute-value response eligibility and
reflection-metadata boundary as an independently selectable pure C++ contract.
Its focused base-and-contract lane passed 4/4 callback-model cases; the
catalog-wide promoted gate passed 680/680 CTest cases with 680 direct passes
and only the two expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-regional-attribute-value-update-response-recheck-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
provider, dimensional FOM, endpoint, callback, and logical-time configuration
remain adapter-owned.
The promoted `cpp-tck.default-region-object-routing-contract` runner adds the
standard ordinary/default-region object routing and association-replacement
boundary as an independently selectable pure C++ contract. Its focused
base-and-contract lane passed 4/4 callback-model cases; the catalog-wide
promoted gate passed 682/682 CTest cases with 682 direct passes and only the
two expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-default-region-object-routing-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
provider, dimensional FOM, endpoint, callback, and logical-time configuration
remain adapter-owned.
The promoted `cpp-tck.passive-regional-subscription-contract` runner adds the
standard passive regional subscription suppression and activation boundary as
an independently selectable pure C++ contract. Its focused base-and-contract
lane passed 4/4 callback-model cases; the catalog-wide promoted gate passed
684/684 CTest cases with 684 direct passes and only the two expected
adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-passive-regional-subscription-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
provider, dimensional FOM, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.regional-multi-attribute-update` scenario uses a
separate adapter-supplied two-dimensional FOM containing two attributes on one
object class. Each attribute is registered and subscribed with its own source
region; an X-only or Y-only source mutation suppresses only the affected
projection, while restoring the ranges re-enables both projections and keeps
the source-region designator in the callback. The source uses only the
official `RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.regional-three-dimensional-overlap` scenario uses a
separate adapter-supplied three-dimensional FOM. It verifies complete-overlap
discovery and reflection, suppresses delivery when exactly one source
dimension becomes disjoint, restores the dimension, and checks the conveyed
source-region designator. The test remains limited to the official
`RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.default-region-object-routing` scenario exercises the
ordinary/default-region boundary with three federates. It proves that a
retained ordinary subscription is suppressed while a disjoint explicit
regional declaration is active, that removing the declaration restores
ordinary discovery, and that removing and restoring an explicit source
association switches delivery between the supplied source-region designator
and the supplied-empty default projection. The source uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied FOM names.

The promoted `cpp-tck.passive-regional-subscription` scenario exercises the
regional passive-subscription boundary. It proves that a regional
subscription created with `active=false` suppresses discovery and reflection,
that activating the same declaration delivers the pending object and later
updates, and that the callback carries the source-region designator. The
source uses only the official `RTIambassador`/`FederateAmbassador` API and
adapter-supplied FOM names.

The promoted `cpp-tck.auto-provide` scenario exercises the standard Auto
Provide service with an adapter-supplied FOM whose switch declaration enables
the service. It verifies the switch on both federates, ordinary publication and
subscription, discovery, and one grouped `provideAttributeValueUpdate`
solicitation with the registered object, complete owned attribute set, and
empty user tag. The source uses only the official
`RTIambassador`/`FederateAmbassador` API.
The promoted `cpp-tck.auto-provide-contract` runner exposes that same standard
Auto Provide surface as an independently selectable pure C++ contract. Its
focused base-and-contract lane passed 4/4 callback-model cases; the
catalog-wide promoted gate passed 686/686 CTest cases with 686 direct passes
and only the two expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-auto-provide-contract.json`. The source
uses only official IEEE C++ headers and the standard library; the Auto Provide
FOM, provider, endpoint, callback, and logical-time configuration remain
adapter-owned.

The promoted `cpp-tck.attribute-scope-advisories-contract` runner exposes the
standard attribute-scope switch, regional transition, and stale-callback
boundary as an independently selectable pure C++ contract. Its focused
base-and-contract lane passed 4/4 callback-model cases; the catalog-wide
promoted gate passed 688/688 CTest cases with 688 direct passes and only the
two expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-attribute-scope-advisories-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.regional-declaration-relevance-advisories-contract`
runner exposes the standard object-class and interaction relevance advisory
surface as an independently selectable pure C++ contract. Its focused
base-and-contract lane passed 4/4 callback-model cases; the catalog-wide
promoted gate passed 690/690 CTest cases with 690 direct passes and only the
two expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-regional-declaration-relevance-advisories-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.regional-interaction-routing-contract`
runner exposes the standard ordinary regional interaction routing contract as
an independently selectable pure C++ contract. Its focused base-and-contract
lane passed 4/4 callback-model cases; the catalog-wide promoted gate passed
692/692 CTest cases with 692 direct passes and only the two expected
adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-regional-interaction-routing-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.regional-attribute-value-request-filtering` scenario
exercises the standard regional Request Attribute Value Update filter. It
verifies foreign-region, incompatible-region, and uncommitted-region failures;
empty-request no-op behavior; disjoint filtering that retains the ordinary
default-region object; overlapping solicitation of both source objects; and
callback-time re-evaluation after an evoked request region moves. Immediate
delivery verifies that already-delivered callbacks are not replayed by a later
region commit. The source uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied FOM names.

The promoted `cpp-tck.regional-interaction-subscription-filtering` scenario
exercises the ordinary receive-order regional interaction filter. It verifies
independent ordinary and regional declarations, explicit empty-region no-op
behavior, overlap delivery and conveyed source-region metadata, callback-time
suppression after an evoked subscription-region move, immediate-delivery
non-replay, and the standard foreign, uncommitted, and incompatible-region
exception boundaries. The source uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied FOM names.

The promoted `cpp-tck.regional-interaction-subscription-filtering-contract`
runner exposes that ordinary regional subscription filter as an independently
selectable pure C++ contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the catalog-wide promoted gate passed 696/696 CTest
cases with 696 direct passes and only the two expected adapter-managed
connection-loss skips in
`.build\cpp-tck-all\verified-evidence-regional-interaction-subscription-filtering-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.timestamped-regional-interaction-contract`
runner exposes the standard timestamped regional interaction delivery and
retraction surface as an independently selectable pure C++ contract. Its
focused base-and-contract lane passed 4/4 callback-model cases; the
catalog-wide promoted gate passed 698/698 CTest cases with 698 direct passes
and only the two expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-timestamped-regional-interaction-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.timestamped-regional-interaction-regulation-reenable-contract`
runner exposes the standard timestamped regional interaction Time Regulation
re-enable surface as an independently selectable pure C++ contract. Its
focused base-and-contract lane passed 4/4 callback-model cases; the
catalog-wide promoted gate passed 702/702 CTest cases with 702 direct passes
and only the two expected adapter-managed connection-loss skips in
`.build\cpp-tck-all\verified-evidence-timestamped-regional-interaction-regulation-reenable-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.regional-declaration-relevance-advisories` scenario
exercises active and passive regional object and interaction subscriptions.
It verifies that passive declarations do not change provider relevance, while
active declarations produce the standard `startRegistrationForObjectClass`,
`stopRegistrationForObjectClass`, `turnInteractionsOn`, and
`turnInteractionsOff` callbacks. The source uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied DDM FOM names;
provider-specific service-report file behavior is intentionally outside this
portable slice.

The promoted `cpp-tck.regional-interaction-source-region-snapshot` scenario
verifies that an ordinary regional interaction retains its send-time source
region when callback servicing is deferred. It checks delivery with the
original conveyed region, suppression after the source becomes disjoint, and
fresh delivery after overlap is restored. The source uses only the official
`RTIambassador`/`FederateAmbassador` API and adapter-supplied DDM FOM names.

The promoted `cpp-tck.regional-interaction-source-region-snapshot-contract`
runner exposes that send-time regional interaction boundary as an independently
selectable pure C++ contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the catalog-wide promoted gate passed 694/694 CTest
cases with 694 direct passes and only the two expected adapter-managed
connection-loss skips in
`.build\cpp-tck-all\verified-evidence-regional-interaction-source-region-snapshot-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.service-report-regional-interaction-subscription`
scenario applies the standard MOM report route to
`SubscribeInteractionClassWithRegions` and
`UnsubscribeInteractionClassWithRegions`. It verifies the service type,
passive-subscription indicator, exact standard supplied-argument shapes,
null returned argument, exception, serial progression, and both callback
models. The dimensional FOM and standard MIM remain adapter inputs.

The promoted `cpp-tck.ownership-transfer-regional-update` scenario is the
next-level ownership/DDM slice. It uses three federates and the adapter’s
dimensional FOM to prove that an explicit update-region association is cleared
when ownership transfers: the former owner cannot update, the new owner’s
first update uses the default source, and regional delivery resumes only after
the new owner explicitly associates a replacement region. This remains a pure
`RTIambassador`/`FederateAmbassador` test and has no provider-specific FOM or
registry dependency.

The promoted `cpp-tck.timestamped-regional-attribute-update` scenario adds the
timed DDM boundary with four federates. It verifies overlap and disjoint
filtering, timestamp/order and conveyed source-region metadata, retraction
before a constrained grant, time-constrained re-enable, and callback-before-
grant ordering. The provider supplies the dimensional FOM and logical-time
implementation through the adapter; the scenario itself uses only the official
API.

The promoted `cpp-tck.timestamped-regional-attribute-alternate-advances`
scenario runs the explicit-source timestamped update through Flush Queue
Request, Time Advance Request Available, and Next Message Request Available.
It verifies reflection-before-grant ordering, grant and logical-time query
consistency, backward-time rejection, source-region metadata, and the
post-delivery retraction boundary under both callback models.

The promoted `cpp-tck.timestamped-regional-attribute-association-replacement`
scenario checks the enqueue-time source boundary. A queued update associated
with source region A is not retargeted when A is replaced by B; a later update
is delivered with B in its conveyed region designator. This remains a focused
official-API test, with the dimensional FOM and logical time supplied by the
adapter.

The promoted `cpp-tck.timestamped-regional-attribute-regulation-reenable`
scenario covers the producer-side changed-lookahead boundary for an explicit
source region. It moves the live source region out of scope after enqueue,
disables and re-enables Time Regulation with a changed lookahead, verifies
Query Lookahead, and proves that the accepted update still reflects before the
recipient grant with its original source-region metadata.

The promoted `cpp-tck.timestamped-regional-attribute-source-resignation`
scenario queues an overlap-qualified timestamped update, resigns the producer,
and releases the recipient with an independent time regulator. It verifies
preserved object, producer, source-region, payload, timestamp/order, and
retraction metadata, plus the standard post-resignation membership boundary.

The promoted `cpp-tck.timestamped-interaction-source-resignation-fanout`
scenario queues one ordinary timestamped interaction for two constrained
recipients, admits one with TAR and the other with NMR, and resigns the
producer. An independent regulator releases each recipient frontier; both
Receive Interaction callbacks preserve the original parameter, tag, producer,
timestamp/order, transport, and retraction metadata, and the resigned producer
is rejected by the standard membership boundary on Retract.

The promoted `cpp-tck.timestamped-interaction-source-resignation` scenario
isolates the same lifecycle for one constrained TAR recipient. It confirms that
producer resignation leaves the accepted passel deliverable and preserves the
parameter, tag, producer, timestamp/order, transport, and retraction metadata
before the matching grant.

The promoted `cpp-tck.available-time-advances-inclusive-galt` scenario sends
timestamped interactions at the inclusive GALT boundary through Time Advance
Request Available and Next Message Request Available. It verifies that the
interaction callback precedes the matching grant, preserves timestamp/order,
transport, producer, parameter, tag, and retraction metadata, and rejects
post-delivery retraction under both callback models. The source uses only the
official API; the adapter supplies the FOM and logical-time implementation.

The promoted `cpp-tck.timestamped-interaction-mixed-advances` scenario sends one
queued ordinary timestamped interaction to three constrained recipients. It
delivers the passel before Flush Queue Request, Time Advance Request Available,
and Next Message Request Available grants, and checks callback ordering,
logical-time queries, payload, producer, transport, order, and retraction
metadata.

The promoted `cpp-tck.timestamped-interaction-flush-queue-future-input`
scenario queues timestamp-nine and timestamp-twelve interactions, then issues
Flush Queue Request at timestamp ten. It verifies that both timestamped
callbacks precede the Flush Queue Grant, preserves payload, tag, producer,
timestamp/order, transport, and retraction metadata for each message, and
checks that the grant’s optimistic frontier is the earliest queued message.
The adapter supplies the FOM and logical-time implementation; the source uses
only the official API.

The promoted `cpp-tck.timestamped-interaction-tso-designator-terminalization`
scenario applies a five-epsilon lookahead and exercises Time Advance Request,
Time Advance Request Available, Next Message Request, Next Message Request
Available, and Flush Queue Request. Each expired timestamped interaction
retraction becomes terminal without an interaction callback to the idle
constrained receiver; the source uses only the official API and takes the FOM
and logical-time implementation from the adapter.

The promoted `cpp-tck.timestamped-attribute-update-flush-queue-future-input`
scenario applies the same future-input boundary to ordinary attribute updates.
It queues timestamp-nine and timestamp-twelve values, issues Flush Queue
Request at timestamp ten, and verifies reflect-before-grant ordering, value,
tag, producer, timestamp/order, transport, and retraction metadata, the
optimistic frontier, and the logical-time query. The adapter supplies the
object/attribute FOM and logical-time implementation; the source uses only the
official API.

The promoted `cpp-tck.timestamped-interaction-cross-producer-order` scenario
sends timestamp-seven input before timestamp-five input from two independent
producers. Each constrained recipient receives both timestamp-five messages in
an unspecified tie order and the timestamp-seven message afterward, preserving
producer, tag, parameter, order, transport, and retraction metadata.

The promoted `cpp-tck.timestamped-interaction-retraction-fanout` scenario sends
one timestamped interaction to an immediate and a time-constrained recipient.
Retracting it notifies the delivered recipient, suppresses the still-queued
copy, and preserves the callback ordering and retraction identity. After the
constrained recipient resigns, a second immediate delivery confirms the
retraction ledger remains correct across recipient departure.

The promoted `cpp-tck.delay-subscription-evaluation-interaction` scenario
composes an adapter-supplied switch module with the portable FOM. It verifies
that the standard Delay Subscription Evaluation switch retains an ordinary
interaction generated before subscription, while the disabled/default mode
does not; both modes recheck the current subscription at the callback boundary
and suppress a message after unsubscribe.

The promoted `cpp-tck.delay-subscription-evaluation-attribute-update` scenario
applies the same switch boundary to a known object and ordinary attribute
update. It establishes discovery first, removes the attribute declaration,
then verifies enabled late-declaration delivery and disabled generation-time
suppression, followed by callback-boundary suppression after unsubscribe.
The promoted timestamped interaction and timestamped attribute-update variants
repeat that matrix at the time-constrained grant boundary and verify the
standard timestamp, order, transportation, producer, and retraction metadata.

The promoted `cpp-tck.timestamped-default-region-attribute-alternate-advances`
scenario complements the explicit-source alternate-advance cases with ordinary
object registration. It proves regional subscribers receive the provider’s
default-source update through FQR, TARA, and NMRA, and that Convey Region
Designator Sets reports the supplied-empty source set.

The promoted `cpp-tck.timestamped-default-region-attribute-reenable` scenario
queues a timestamped default-region update, disables and re-enables Time
Constrained, and then verifies regional delivery before the subsequent grant.
It also checks the supplied-empty source-region metadata and the standard
post-delivery retraction boundary under both callback models.

The promoted `cpp-tck.timestamped-default-region-attribute-regulation-reenable`
scenario covers the producer-side transition. It queues a timestamped
default-region update under the initial lookahead, disables and re-enables Time
Regulation with a changed lookahead, verifies the new value through Query
Lookahead, and proves delivery before the recipient grant with preserved
metadata and terminal retraction behavior.

The promoted `cpp-tck.timestamped-default-region-attribute-mixed-fanout`
scenario sends one ordinary timestamped update to an immediate and a
time-constrained regional recipient. It verifies supplied-empty source metadata
and full reflection metadata for the delivered copy, Request Retraction only
for that recipient, and suppression of the pending copy before its grant.

The promoted `cpp-tck.timestamped-default-region-interaction` scenario sends a
timestamped interaction without a source region to a subscriber using an
explicit region. It verifies constrained delivery, supplied-empty source-region
metadata, parameter/tag/producer/order/transport metadata, and the terminal
message-retraction boundary.

The promoted `cpp-tck.timestamped-default-region-interaction-alternate-advances`
scenario sends the same default-region interaction to three constrained
recipients using Flush Queue Request, Time Advance Request Available, and Next
Message Request Available. It verifies callback-before-grant ordering,
grant/query bounds, backward-time failures, supplied-empty source-region
metadata, and terminal retraction.

The promoted `cpp-tck.timestamped-default-region-interaction-mixed-fanout`
scenario sends one ordinary timestamped interaction to an immediate and a
time-constrained regional recipient. It verifies supplied-empty source metadata
and full interaction metadata for the delivered copy, Request Retraction only
for that recipient, and suppression of the pending copy before its grant.

The promoted `cpp-tck.timestamped-default-region-interaction-source-resignation`
scenario fans one queued default-source interaction out to two constrained
regional recipients, then resigns the producer. An independent regulator
releases each recipient, and each callback preserves supplied-empty source
metadata, producer, payload, timestamp/order, and retraction identity; the
resigned producer is rejected by the standard membership boundary on Retract.

The promoted `cpp-tck.timestamped-default-region-interaction-reenable`
scenario queues a default-source interaction, disables and re-enables Time
Constrained, and verifies one callback before the matching grant with
supplied-empty source metadata, full interaction metadata, and terminal
retraction.

The promoted `cpp-tck.timestamped-default-region-interaction-regulation-reenable`
scenario queues a default-source interaction, disables and re-enables Time
Regulation with changed lookahead, verifies Query Lookahead, and proves
callback-before-grant delivery and terminal retraction.

The promoted `cpp-tck.timestamped-interaction-regulation-reenable` scenario
applies the same changed-lookahead boundary to an ordinary timestamped
interaction. It verifies Query Lookahead, parameter/tag/producer/order
metadata, receive-before-grant ordering, and terminal retraction using only
adapter-selected interaction and parameter names.

The promoted `cpp-tck.timestamped-object-deletion-regulation-reenable` scenario
applies the changed-lookahead boundary to a queued timestamped object
deletion. It verifies Query Lookahead, removal-before-grant ordering,
timestamp/order metadata, object identity cleanup on both federates, and the
standard terminal retraction boundary using only the official object-management
and time-management API.

The promoted `cpp-tck.timestamped-object-deletion-tombstone` scenario
isolates the terminal deletion boundary: a timestamped deletion at the exact
lookahead edge becomes non-retractable after the owning federate advances, and
the named object-instance reservation can then be reused. It uses only the
standard object-management and time-management API with adapter-supplied FOM
and logical-time configuration.

The promoted `cpp-tck.timestamped-local-delete-object` scenario covers the
recipient-local deletion boundary. A constrained recipient locally deletes an
object while its timestamped removal is queued; that recipient receives only
its time-advance grant, while an independent constrained recipient receives the
original removal before its grant with the standard object, tag, producer, time,
order, and retraction metadata.

The promoted `cpp-tck.timestamped-local-delete-attribute` scenario applies the
same recipient-local boundary to queued timestamped attribute reflection. The
local recipient suppresses its pending reflection, then re-subscribes and
receives a later update alongside the independent recipient, preserving the
standard object, attribute, tag, producer, time, order, transport, and
retraction metadata.

The promoted `cpp-tck.named-registration` scenario isolates the standard named
object-instance path. It verifies single and multiple reservation/release,
reservation contention, named registration and discovery identity, failed
registration reuse, and callback-model parity using only adapter-owned FOM
handles and the official object-management API.

The promoted `cpp-tck.named-registration-contract` runner exposes that same
named object-registration path as an independently selectable pure standard C++
contract. It retains reservation/release and reuse, multiple-name lifecycle,
named registration and discovery identity, contention, and invalid-name
assertions while taking the provider package, FOM, endpoint, and callback
configuration from the adapter.

The promoted `cpp-tck.object-attribute-subscription-lifecycle-contract` runner
exposes the ordinary object-attribute declaration lifecycle as an independently
selectable pure standard C++ contract. It retains passive and active
subscriptions, activation-time discovery, ordinary reflection, downgrade and
reactivation, unsubscription, and stable object identity lookups while taking
the provider package, FOM, endpoint, and callback configuration from the
adapter.

The promoted `cpp-tck.local-delete-object-instance` scenario isolates the
ordinary recipient-local deletion path. It verifies the pre-connect and
pre-join boundaries, ownership and pending-acquisition protections, successful
local deletion from a fresh requester, rediscovery without federation-wide
removal, and continued ordinary attribute reflection using only the
adapter-owned FOM and official object-management API.

The promoted `cpp-tck.local-delete-object-instance-contract` runner exposes the
same local deletion path as an independently selectable pure standard C++
contract. It retains the pre-connect and pre-join boundaries, ownership and
pending-acquisition protections, fresh-requester deletion, rediscovery and
stable identity lookups, and continued ordinary reflection while taking the
provider package, FOM, endpoint, and callback configuration from the adapter.

The promoted `cpp-tck.service-report-local-delete-object-instance` scenario
adds the standard MOM callback contract for that successful local deletion.
Using only the adapter-supplied standard MIM and official APIs, it verifies the
seven `HLAreportServiceInvocation` parameters, service identity and type,
reliable transport, success and empty-exception values, serial number, and
callback metadata.

The promoted `cpp-tck.service-report-interaction-failure` scenario drives
invalid interaction-class, parameter, and unpublished-class inputs through
the standard `SendInteraction` API. It verifies typed failure reports,
exception names, serial progression, and suppression of the ordinary
application callback in both callback models.

The promoted `cpp-tck.service-report-interaction-contract`,
`cpp-tck.service-report-attribute-update-contract`,
`cpp-tck.service-report-register-object-instance-contract`, and
`cpp-tck.service-report-delete-object-instance-contract` runners expose the
four ordinary MOM service-report success routes as independently selectable
pure C++ contracts. Each uses only the official IEEE C++ API, standard MIM
data elements, and standard library; the adapter supplies the provider package,
FOM, MIM, endpoint, and callback configuration.

The promoted `cpp-tck.service-report-regional-interaction` scenario applies
the same public MOM interaction route to `SendInteractionWithRegions`. It uses
only the adapter-supplied dimensional FOM, dimension names, and standard MIM;
both callback models verify the successful report and the independent
overlap-qualified application callback, including payload, tag, producer,
transportation, and conveyed source-region metadata.
The promoted `cpp-tck.service-report-regional-interaction-failure` scenario
drives invalid interaction-class, parameter, and region inputs through the same
standard API. It verifies typed failure reports, exception names, serial
progression, and suppression of the regional application callback in both
callback models.

The promoted `cpp-tck.service-report-interaction-failure-contract`,
`cpp-tck.service-report-attribute-update-failure-contract`, and
`cpp-tck.service-report-regional-interaction-failure-contract` runners expose
those ordinary MOM failure routes as independently selectable pure standard C++
contracts. They retain the typed report, exception, serial, and application-
callback assertions while taking MIM, FOM, DDM, endpoint, and callback
configuration from the adapter.

The promoted `cpp-tck.service-report-timestamped-attribute-update-failure-contract`,
`cpp-tck.service-report-timestamped-interaction-failure-contract`, and
`cpp-tck.service-report-timestamped-delete-object-instance-failure-contract`
runners extend the same pure boundary to timestamped
`UpdateAttributeValues`, `SendInteraction`, and `DeleteObjectInstance`. They
take the logical-time implementation from the adapter and preserve optional
timestamp, exception, serial, and callback-suppression assertions without
adding provider-specific dependencies.

The promoted `cpp-tck.service-report-timestamped-interaction-contract` runner
exposes the successful timestamped interaction MOM route as an independently
selectable pure standard C++ contract. It retains typed report,
timestamped-delivery, logical-time-grant, payload, retraction, and callback
assertions while taking MIM, FOM, endpoint, callback, and logical-time
configuration from the adapter.

The promoted `cpp-tck.timestamped-interactions-contract` runner exposes the
standard timestamped Send/Receive, time-role, retraction, and Request Retraction
surface as an independently selectable pure standard C++ contract. It retains
timestamp/order, constrained-grant, retraction-handle, and callback assertions
while taking FOM, endpoint, callback, and logical-time configuration from the
adapter.

The promoted `cpp-tck.timestamped-directed-interactions-contract` runner exposes
the standard timestamped directed-interaction target-routing, time-role,
retraction, and Request Retraction surface as an independently selectable pure
standard C++ contract. It retains target/class/handle boundaries, constrained
grant, callback, and retraction assertions while taking FOM, endpoint, callback,
and logical-time configuration from the adapter.

The promoted
`cpp-tck.timestamped-directed-interaction-source-resignation-contract` and
`cpp-tck.timestamped-directed-interaction-source-resignation-fanout-contract`
runners expose the standard queued directed-interaction source-resignation,
target routing, post-resignation retraction, and independent per-recipient
fan-out surfaces as independently selectable pure standard C++ contracts. They
retain target, payload, producer, time/order, transport, callback-ordering, and
recipient TAR assertions while taking FOM, endpoint, callback, and logical-time
configuration from the adapter.

The promoted
`cpp-tck.timestamped-interaction-source-resignation-contract` and
`cpp-tck.timestamped-interaction-source-resignation-fanout-contract` runners
expose the standard queued timestamped interaction source-resignation and
per-recipient fan-out surfaces as independently selectable pure standard C++
contracts. They retain the post-resignation retraction boundary, TAR/NMR
servicing, payload/time/order/transport metadata, and callback ordering while
taking FOM, endpoint, callback, and logical-time configuration from the adapter.

The promoted `cpp-tck.timestamped-directed-alternate-advances-contract` runner
exposes the standard directed-interaction Flush Queue, TAR-available, and
NMR-available delivery surface as an independently selectable pure standard C++
contract. It retains callback ordering, target, time/order, retraction, and
logical-time assertions while taking FOM, endpoint, callback, and logical-time
configuration from the adapter.

The promoted `cpp-tck.service-report-request-attribute-value-update-contract`,
`cpp-tck.service-report-release-multiple-object-instance-names-contract`,
`cpp-tck.service-report-release-object-instance-name-contract`, and
`cpp-tck.service-report-reserve-object-instance-name-contract` runners expose
the remaining ordinary MOM success routes as independently selectable pure
standard C++ contracts. They retain the typed report, serial, and provider-
callback assertions while taking MIM, FOM, endpoint, and callback configuration
from the adapter.

The promoted `cpp-tck.service-report-local-delete-object-instance-contract`,
`cpp-tck.service-report-local-delete-object-instance-failure-contract`, and
`cpp-tck.service-report-delete-object-instance-failure-contract` runners expose
ordinary and local object-deletion MOM success/failure routes as independently
selectable pure standard C++ contracts. They retain typed report, exception,
serial, returned-argument, and cleanup assertions while taking MIM, FOM,
endpoint, and callback configuration from the adapter.

The promoted `cpp-tck.service-report-reserve-object-instance-name` scenario
applies the same report contract to a successful `ReserveObjectInstanceName`
call and independently verifies the standard
`objectInstanceNameReservationSucceeded` callback, using only the
adapter-supplied standard MIM and ordinary FOM.

The promoted `cpp-tck.service-report-release-object-instance-name` scenario
applies the same report contract to a successful `ReleaseObjectInstanceName`
call after a standard reservation-success callback, using only the
adapter-supplied standard MIM and ordinary FOM.

The promoted `cpp-tck.service-report-release-multiple-object-instance-names`
scenario applies the same report contract to a successful
`ReleaseMultipleObjectInstanceNames` call after a standard
multiple-reservation-success callback, using only the adapter-supplied standard
MIM and ordinary FOM.

The promoted `cpp-tck.service-report-request-attribute-value-update` scenario
applies the same report contract to a successful `RequestAttributeValueUpdate`
call and independently verifies the standard `provideAttributeValueUpdate`
callback, using only the adapter-supplied standard MIM and ordinary FOM. The
scenario covers both standard overloads: the known-object request and the
object-class request. It isolates their MOM reports, verifies serial
progression, and checks the object/class, attribute set, and request tag on
each provider callback.

The promoted `cpp-tck.service-report-register-object-instance` scenario applies
the same report contract to a successful ordinary `RegisterObjectInstance` call
and verifies the independent standard `discoverObjectInstance` callback for the
registered object, using only the adapter-supplied standard MIM and ordinary FOM.

The promoted `cpp-tck.service-report-delete-object-instance` scenario applies
the same report contract to an ordinary `DeleteObjectInstance` call and also
verifies the independent standard `removeObjectInstance` callback with its
object, tag, and producer metadata.

The promoted `cpp-tck.service-report-delete-object-instance-failure` scenario
extends the same contract to invalid and stale ordinary deletion calls. It
verifies the standard failure indicator, `ObjectInstanceNotKnown` exception,
returned-argument encoding, serial progression around the successful deletion,
and the normal removal callback using only the adapter-supplied MIM and
ordinary FOM.

The promoted `cpp-tck.service-report-local-delete-object-instance-failure`
scenario applies the same standard MOM failure contract to
`localDeleteObjectInstance`. It verifies unknown and stale handle failures
around a successful local deletion, supplied and returned argument encodings,
exception text, serial progression, and ordinary object discovery using only
the adapter-supplied MIM and FOM.

The promoted `cpp-tck.attribute-value-update-request-baseline` scenario isolates
the standard object-instance Request Attribute Value Update overload. It verifies
known-object solicitation of exactly the current owner’s attributes, request-tag
propagation, and suppression of requester-owned or unowned attributes. The
promoted `cpp-tck.object-class-attribute-value-update-request-baseline` scenario
separately expands the class overload across two concrete subclass instances,
checks one callback per owner with inherited-attribute filtering, and verifies
requester-owned suppression. Both remain on the official
`RTIambassador`/`FederateAmbassador` surface with adapter-supplied ordinary/rich
FOM inputs.

The promoted `cpp-tck.attribute-value-update-response` scenario completes the
ordinary attribute-value request/response route. It verifies provider callback
object, attribute-set, and request-tag metadata, confirms that no reflection
arrives before the provider responds, and checks the returned value, response
tag, reliable transport, producer identity, and empty region metadata through
the official API and adapter-supplied ordinary FOM.

The promoted `cpp-tck.timestamped-object-deletion-no-fanout` scenario covers
the complementary no-recipient boundary. A deletion at the exact lookahead
edge is terminal and cannot be retracted; a later deletion with no eligible
recipient can be retracted, restoring object-name lookup and local attribute
ownership without a `requestRetraction` callback.

The promoted `cpp-tck.receive-order-attribute-update-callback-cancellation`
scenario keeps the receive-order callback lifecycle portable. In evoked mode,
an attribute reflection queued for a subscribed recipient is suppressed when
the recipient unsubscribes before callback servicing; in immediate mode, the
reflection is observed before the same unsubscribe. It uses only the
adapter-selected ordinary FOM and the official `RTIambassador`/
`FederateAmbassador` surface.

The promoted `cpp-tck.receive-order-interaction-callback-cancellation`
scenario applies the same portable callback lifecycle to ordinary
`SendInteraction`/`receiveInteraction`. Evoked delivery is suppressed when
the queued subscription is removed before callback servicing; immediate
delivery is observed before unsubscribe. Parameter, tag, producer, and
transport metadata are verified through the standard callback.

The promoted `cpp-tck.interaction-subscription-lifecycle` scenario extends
that ordinary interaction surface with the standard declaration lifecycle.
It verifies that a passive subscription suppresses delivery, activation
enables it without replaying the passive send, downgrading back to passive
suppresses later delivery, and unsubscribe removes the declaration. The
same payload, tag, producer, and transportation checks run under both
callback models using only adapter-supplied FOM names.

The promoted `cpp-tck.interaction-subscription-lifecycle-contract` runner
exposes that declaration boundary as an independently selectable pure standard
C++ contract. It uses only official IEEE C++ headers and the standard library;
provider, FOM, endpoint, and callback configuration remain adapter-owned.

The promoted `cpp-tck.object-attribute-subscription-lifecycle` scenario applies
the same lifecycle proof to ordinary object attributes. A passive declaration
suppresses discovery and reflection, activation discovers the existing object
and enables reflection, downgrade suppresses later updates, reactivation
restores reflection, and unsubscribe removes delivery. It uses only the
adapter-supplied ordinary FOM and standard object-management callbacks; no DDM
surface is involved.

The promoted `cpp-tck.object-publication-registration-fence` scenario verifies
the ordinary publication boundary around object registration. Whole-class
`unpublishObjectClass` makes `registerObjectInstance` fail with the standard
`ObjectClassNotPublished` exception; republishing restores registration and
discovery, followed by stable object/class/name lookups. The source remains on
the adapter-selected ordinary FOM and official `RTIambassador`/
`FederateAmbassador` surface.

The promoted `cpp-tck.object-publication-registration-fence-contract` runner
exposes the same publication and registration boundary as an independently
selectable pure standard C++ contract. It uses only official IEEE C++ headers
and the standard library; provider, FOM, endpoint, and callback configuration
remain adapter-owned.

The promoted `cpp-tck.interaction-publication-send-fence` scenario verifies the
corresponding ordinary interaction boundary. Whole-class
`unpublishInteractionClass` makes `sendInteraction` fail with
`InteractionClassNotPublished`; republication restores parameter delivery and
the standard producer, tag, and transportation metadata. The source remains on
the adapter-selected ordinary FOM and official `RTIambassador`/
`FederateAmbassador` surface.

The promoted `cpp-tck.interaction-publication-send-fence-contract` runner
exposes that publication boundary as an independently selectable pure standard
C++ contract. It uses only official IEEE C++ headers and the standard library;
provider, FOM, endpoint, and callback configuration remain adapter-owned.

The promoted `cpp-tck.directed-interaction-publication-send-fence` scenario
verifies the corresponding directed boundary. Whole-class directed
unpublication makes `sendDirectedInteraction` fail with
`InteractionClassNotPublished`; republication restores targeted parameter
delivery and the standard target, producer, tag, and transportation metadata.
The source remains on the adapter-selected ordinary FOM and official
`RTIambassador`/`FederateAmbassador` surface.

The promoted `cpp-tck.directed-interaction-publication-send-fence-contract`
runner exposes that targeted publication boundary as an independently
selectable pure standard C++ contract. It uses only official IEEE C++ headers
and the standard library; provider, FOM, endpoint, and callback configuration
remain adapter-owned.

The promoted `cpp-tck.directed-interaction-target-lifecycle` scenario verifies
targeted delivery across universal subscription, subscription removal and
restoration, source publication removal and restoration, and target
deletion/removal cleanup. The promoted
`cpp-tck.directed-interaction-target-lifecycle-contract` runner exposes the
same target lifecycle as an independently selectable pure standard C++ contract
with provider, FOM, endpoint, and callback configuration remaining adapter-owned.

The promoted `cpp-tck.directed-interaction-subscription-kind-contract`
runner exposes the by-ownership and universal directed-interaction
subscription boundary as an independently selectable pure standard C++
contract. It uses only official IEEE C++ headers and the standard library;
provider, FOM, endpoint, and callback configuration remain adapter-owned.

The promoted `cpp-tck.timestamped-object-deletion-mixed-advances` scenario
queues one timestamped object deletion for three constrained recipients. It
delivers the Remove Object Instance callback before independent Flush Queue
Request, Time Advance Request Available, and Next Message Request Available
grants, then verifies callback ordering, grant/query consistency, removal
metadata, and the standard post-delivery retraction boundary.

The promoted `cpp-tck.timestamped-directed-interaction-regulation-reenable`
scenario applies the same changed-lookahead boundary to a target-qualified
timestamped interaction. It verifies target routing, Query Lookahead,
receive-before-grant ordering, parameter/tag/producer/order metadata, and the
standard terminal retraction boundary using only the official directed
interaction and time-management API.

The promoted `cpp-tck.timestamped-directed-interaction-source-resignation`
scenario queues one target-qualified timestamped interaction, resigns its
producer, and releases the constrained recipient with an independent time
regulator. It verifies that target, parameter, tag, departed producer,
timestamp/order, transport, and retraction metadata survive to the matching
grant, while the resigned producer observes the standard membership boundary.

The promoted `cpp-tck.timestamped-directed-interaction-source-resignation-fanout`
scenario extends that oracle to two constrained recipients. The producer
resigns while one timestamped directed copy is queued for each recipient; the
first recipient drains at its own TAR frontier while the second remains queued
until it issues its own TAR. Both callbacks verify target, parameter, tag,
departed producer, timestamp/order, transport, retraction, and callback-before-
grant metadata through the official directed-interaction and time-management
API only.

The promoted `cpp-tck.timestamped-directed-interaction-tar-nmr` scenario sends
one target-qualified timestamped directed interaction at time 7 to two
constrained recipients. One requests TAR(7) and the other NMR(10); each
callback is observed before its own grant, with the NMR grant returning at the
message time. The scenario preserves target, parameter, tag, producer,
timestamp/order, transport, and retraction metadata and verifies the matching
logical-time queries through the standard directed-interaction and time-
management API.

The pure contract twins
`cpp-tck.timestamped-directed-interaction-tar-nmr-contract` and
`cpp-tck.timestamped-directed-interaction-immediate-source-resignation-contract`
reuse those verified behaviors through standard-only C++ wrapper entry points.
Their contract references name only the official `RTIambassador` and
`FederateAmbassador` surfaces plus the standard library; the adapter owns the
provider package, FOM, endpoint, callback configuration, and logical-time
implementation. The focused four-scenario lane passed 8/8 callback-model
cases in
`.build\cpp-tck-all\focused-standard-timestamped-directed-interaction-tar-nmr-resignation-contracts.json`.

The pure `cpp-tck.query-lits-source-resignation-contract` and
`cpp-tck.partial-attribute-ownership-transfer-contract` runners likewise
reuse the verified standard Query LITS and partial ownership behaviors through
official C++ interfaces only. Their focused four-scenario lane passed 8/8
callback-model cases in
`.build\cpp-tck-all\focused-standard-query-lits-partial-ownership-contracts.json`.

The promoted `cpp-tck.federation-mom-save-conditionals` scenario observes the
standard `HLAfederation` MOM object's `HLAnextSaveName/Time` and
`HLAlastSaveName/Time` conditionals. It verifies empty initial values, a
timestamped pending save, reliable RTI-originated reflection metadata, clearing
of the next values at save admission, and publication of the last values after
successful completion. It uses only the adapter-supplied standard MIM/FOM and
logical-time implementation with the official `RTIambassador`/
`FederateAmbassador` API.

The promoted `cpp-tck.joined-federate-mom-removed-object-count` scenario uses
the standard `HLAobjectRoot.HLAmanager.HLAfederate` MOM object to observe
`HLAobjectInstancesRemoved` after a receive-order ordinary object deletion. It
first queries `HLAfederateHandle` and verifies the standard
`FederateAmbassador::attributeIsOwnedByRTI` result for the RTI-owned MOM
object, then checks reliable RTI-originated reflection metadata, the
removed-object counter, and the ordinary removal callback. It then exercises
the same RTI-owned MOM object through `RequestAttributeValueUpdate` with a
standard `HLAreportServiceInvocation` report, using only the adapter-supplied
standard MIM/FOM and official IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-time-state-durations` scenario uses
the standard joined-federate MOM object to observe `HLAtimeGrantedTime` and
`HLAtimeAdvancingTime` through direct AVU and one `HLAsetTiming` periodic
reflection. It verifies official four-octet nonnegative `HLAinteger32BE`
durations and reliable RTI-originated metadata in both callback models using
only the adapter-supplied standard MIM/FOM and official IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-galt-lits-periodic` scenario uses the
standard joined-federate MOM object to verify direct and periodic `HLAGALT` and
`HLALITS` values, including the undefined-value boundary after the sole time
regulator is disabled. It uses standard MIM/FOM lookup, `HLAsetTiming`, and
official logical-time/encoder types in both callback models.

The promoted `cpp-tck.joined-federate-mom-tso-length-periodic` scenario uses
the standard `HLATSOlength` MOM attribute to verify direct and periodic queued
timestamped-interaction counts, then verifies the count returns to zero after
the timestamp is granted. The interaction and parameter come from the adapter
FOM; all MOM, time, callback, and encoding behavior is exercised through the
official IEEE C++ API in both callback models.

The promoted `cpp-tck.joined-federate-mom-updates-sent-counts` scenario uses the
adapter-supplied ordinary object class and attribute with standard reliable and
best-effort transportation. It requests `HLArequestUpdatesSent` and verifies
the two `HLAreportUpdatesSent` transport buckets, nested standard
`HLAobjectClassBasedCounts` decoding, RTI-originated metadata, and the empty
response for a requester with no sent updates in both callback models.

The promoted `cpp-tck.joined-federate-mom-interactions-received-counts` scenario
delivers one adapter-supplied ordinary interaction reliably and two after a
standard best-effort transport change. It requests
`HLArequestInteractionsReceived` and verifies the two
`HLAreportInteractionsReceived` transport buckets, nested standard
`HLAinteractionCounts` decoding, receiver metadata, and the empty response for
a requester with no received interactions in both callback models.

The promoted `cpp-tck.joined-federate-mom-interactions-sent-counts` scenario
sends one adapter-supplied ordinary interaction reliably and two after a
standard best-effort transport change. It requests
`HLArequestInteractionsSent` and verifies the two
`HLAreportInteractionsSent` transport buckets, nested standard
`HLAinteractionCounts` decoding, sender metadata, and the empty response for
an idle joined federate in both callback models.

The promoted `cpp-tck.joined-federate-mom-reflections-received-counts` scenario
delivers one adapter-supplied attribute reflection reliably and two after a
standard best-effort transport change. It requests
`HLArequestReflectionsReceived` and verifies the two
`HLAreportReflectionsReceived` transport buckets, nested standard
`HLAobjectClassBasedCounts` decoding, receiver metadata, and the empty response
for a requester with no received reflections in both callback models.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-received-counts`
scenario extends the standard joined-federate MOM interaction ledger to
directed delivery. It sends one ordinary and one directed interaction of the
same adapter-supplied class, proves that ordinary receipt is excluded from
`HLAreportDirectedInteractionsReceived`, and verifies the reliable directed
bucket plus empty best-effort and idle buckets through nested standard
`HLAinteractionCounts`. It uses only the adapter-supplied FOM, standard MIM,
and official IEEE C++ API.

The promoted `cpp-tck.joined-federate-mom-directed-interactions-sent-counts`
scenario sends one ordinary and two directed interactions of the same
adapter-supplied class, changes the class to best effort for an ordinary send,
and proves that only the directed sends enter
`HLAreportDirectedInteractionsSent`. It verifies the reliable directed bucket
plus empty best-effort and idle buckets through nested standard
`HLAinteractionCounts`, using only the adapter-supplied FOM/MIM and official
IEEE C++ API.

The promoted `cpp-tck.mom-transportation-type-change-request` scenario uses the
standard MIM request interactions for attribute and interaction transportation-
type changes. It verifies the corresponding confirmation callbacks, ordinary
object reflection and interaction delivery under best effort, reliable
per-federate transport isolation, and the RTI-originated service report. The
request payloads use official standard encodings while the adapter supplies the
FOM, MIM, endpoint, callback model, and logical-time implementation.

The promoted `cpp-tck.joined-federate-mom-reflection-counts` scenario observes
the standard joined-federate MOM `HLAobjectInstancesReflected` and
`HLAreflectionsReceived` counters through direct attribute-value requests and
periodic `HLAsetTiming` reflections. It distinguishes repeated reflections of
one object from first reflections of another, includes a timestamped reflection,
and verifies both ordinary and timestamped callback paths using only the
adapter-supplied FOM, standard MIM, logical-time implementation, and official
IEEE C++ API.

The promoted `cpp-tck.timestamped-attribute-update-no-fanout` scenario proves
that a time-regulating producer receives a valid retraction handle for a
timestamped update with no eligible recipient, can legally retract it once,
and reaches the standard `MessageCanNoLongerBeRetracted` boundary on reuse. It
also verifies that no local reflection or retraction callback is generated,
using only the adapter-supplied ordinary FOM, logical-time implementation, and
official IEEE C++ API.

The promoted `cpp-tck.timestamped-object-deletion-retraction-joined-owners`
scenario transfers one attribute to a joined member, delivers a timestamped
object removal to that member, and then has it resign before the producer
retracts the deletion. It verifies that the departed member receives no
retraction callback, joined members recover the object identity, and the
departed member's former attribute remains unowned. It uses the adapter-
supplied multi-attribute FOM and logical-time implementation with only the
official ownership, object-management, resignation, retraction, and
time-management APIs.

The promoted `cpp-tck.timestamped-directed-interaction-immediate-source-resignation`
scenario covers the complementary immediate callback route. It accepts a
target-qualified timestamped interaction, resigns its producer, and verifies
exactly one delivery while preserving target, parameter, tag, producer,
timestamp/order, transport, and retraction metadata. The source uses only the
adapter-supplied ordinary FOM and official interaction, object-discovery,
time-regulation, resignation, and callback APIs; the selected callback model
may deliver before or after resignation, as permitted by the callback model.

The promoted `cpp-tck.timestamped-regional-interaction-regulation-reenable`
scenario applies the changed-lookahead boundary to an overlap-qualified
timestamped interaction with an explicit source region. It moves the live
source region out of overlap after admission, then verifies Query Lookahead,
source-region snapshot preservation, receive-before-grant ordering,
parameter/tag/producer/order metadata, and terminal retraction using only the
official DDM, interaction, and time-management API.

The promoted `cpp-tck.timestamped-regional-interaction-alternate-advances`
scenario applies the alternate-advance matrix to the same overlap-qualified
timestamped interaction: one receiver uses Flush Queue Request, one uses Time
Advance Request Available, and one uses Next Message Request Available. It
verifies source-region designator metadata, interaction-before-grant ordering,
grant/query bounds, backward-time failures, and the terminal retraction
boundary through the official DDM, interaction, and time-management API.

The promoted `cpp-tck.timestamped-regional-interaction-alternate-advances-contract`
runner exposes that same alternate-advance surface as an independently
selectable pure C++ contract. Its focused base-and-contract lane passed 4/4
callback-model cases; the promoted aggregate passed 702/702 CTest cases with
702 direct passes and only the two expected adapter-managed connection-loss
skips in
`.build\cpp-tck-all\verified-evidence-timestamped-regional-interaction-alternate-advances-contract.json`.
The source uses only official IEEE C++ headers and the standard library; the
dimensional FOM, provider, endpoint, callback, and logical-time configuration
remain adapter-owned.

The promoted `cpp-tck.timestamped-regional-interaction-no-overlap` scenario
covers the complementary negative-routing boundary. It sends through an
explicit source region that has no overlapping subscription, proves that the
timestamped send still returns a valid retraction handle, verifies legal
pre-delivery retraction and terminal classification after producer progress,
and confirms that the receiver receives no interaction callback.

The promoted `cpp-tck.timestamped-regional-interaction-subscription-replacement`
scenario verifies callback-time subscription replacement. A queued passel
admitted under source/receiver region A is suppressed after the receiver
unsubscribes A and subscribes disjoint region B; a later source-B passel
delivers once with replacement source-region metadata and no fabricated
Request Retraction.

The promoted `cpp-tck.timestamped-regional-interaction-source-resignation`
scenario keeps an overlap-qualified timestamped passel in the recipient queue
after its producer resigns. An independent time regulator releases the
recipient frontier, and the callback retains the original producer,
source-region snapshot, payload, timestamp/order, and retraction metadata;
post-resignation retraction is rejected because the producer is no longer an
execution member.

The promoted `cpp-tck.timestamped-regional-interaction-tar-nmr` scenario
exercises two independent regional recipients at the ordinary TAR and NMR
frontiers. Both recipients receive the timestamped interaction before their
matching grant, preserve the conveyed source-region designator and retraction
handle, and report the resulting logical time through the standard query API.

The promoted `cpp-tck.timestamped-attribute-update-alternate-advances`
scenario is the ordinary object-management counterpart to the regional and
interaction alternate-advance cases. It delivers one timestamped
`UpdateAttributeValues` passel through Flush Queue Request, Time Advance
Request Available, and Next Message Request Available, verifies reflection
before each matching grant, checks grant/query bounds and transport/order
metadata, and rejects retraction after delivery. The object class, attribute,
FOM, and logical-time implementation remain adapter inputs.

The promoted `cpp-tck.timestamped-attribute-update-ownership-transfer` scenario
accepts a timestamped attribute update, transfers ownership before the
constrained grant, and verifies exact value, tag, time, producer, order, and
retraction metadata after the new owner acquires the attribute. It uses only
the adapter-supplied FOM, logical-time implementation, and official
`RTIambassador`/`FederateAmbassador` API. The promoted
`cpp-tck.timestamped-attribute-update-ownership-transfer-contract` runner
exposes the same ownership, queued-delivery, callback, metadata, time-advance,
and retraction boundaries as an independently selectable pure standard contract.

The promoted `cpp-tck.timestamped-attribute-source-resignation` scenario queues
an ordinary timestamped attribute update, resigns the source with unconditional
divestiture, acquires the application attribute on the surviving federate, and
then verifies delivery from the resigned producer after an independent clock
advances. It checks ownership callbacks, post-resignation retraction rejection,
callback-before-grant ordering, and exact value, tag, time, producer, order,
transportation, and retraction metadata using only the adapter-supplied FOM,
logical-time implementation, and official `RTIambassador`/`FederateAmbassador`
API.

The promoted `cpp-tck.timestamped-attribute-source-resignation-fanout` scenario
queues one ordinary timestamped attribute update for two constrained recipients,
resigns the source, and verifies independent recipient grants, one reflection
per recipient, preserved value/tag/time/order/transportation metadata, and the
terminal retraction boundary. The promoted
`cpp-tck.timestamped-attribute-update-regulation-reenable` scenario queues an
ordinary timestamped update, disables and re-enables producer Time Regulation
with a changed lookahead, and verifies Query Lookahead, callback-before-grant
ordering, reflection metadata, and terminal retraction. Both scenarios use only
the adapter-supplied FOM, logical-time implementation, and official
`RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.timestamped-attribute-order-cohort` scenario queues
different-timestamp updates plus an equal-timestamp pair and verifies that two
independent constrained recipients each receive the complete timestamp-5
cohort before their grants, followed by the timestamp-7 update. It accepts the
standard's unspecified order within the equal-timestamp cohort while checking
exact value, tag, producer, time/order, transportation, and retraction metadata
through only the adapter-supplied ordinary FOM, logical-time implementation,
and official `RTIambassador`/`FederateAmbassador` API.

The promoted `cpp-tck.timestamped-attribute-update-queued-passel-retraction`
scenario submits two adapter-defined timestamped attribute values, retracts the
first queued update before its grant, and verifies complete value coverage,
callback-before-grant ordering, and exact tag, time, producer, order,
transportation, and retraction metadata for the later delivery. It uses only the
adapter-supplied multi-attribute FOM, logical-time implementation, and official
`RTIambassador`/`FederateAmbassador` API.

## Interaction survey and translation boundary

The interaction survey maps the Java parity cases to the standard C++ surface:

| Area | Portable C++ translation | Deliberately deferred |
| --- | --- | --- |
| Ordinary interactions | `sendInteraction`/`receiveInteraction`, parameters, tags, producer, transport, active/passive delivery, timestamped grant delivery, and message retraction | Directed regional routing |
| Timestamped object management | Timestamped `updateAttributeValues`/`reflectAttributeValues` and `deleteObjectInstance`/`removeObjectInstance`, producer Time Regulation re-enable with changed lookahead, retraction, object identity, and grant ordering | Regional routing and provider transport internals |
| Timestamped ordinary attributes | Timestamped `UpdateAttributeValues`/`ReflectAttributeValues` through Flush Queue Request, Time Advance Request Available, and Next Message Request Available, with callback-before-grant ordering, grant/query bounds, transport/order metadata, and terminal retraction | Provider-specific transport and durable state |
| Timestamped regional attributes | Region-qualified timestamped `Update/Reflect`, overlap and disjoint filtering, conveyed source-region metadata, producer resignation, retraction, time-constrained re-enable/grant ordering, producer Time Regulation re-enable with changed lookahead, FQR/TARA/NMRA alternate advances, default-region delivery, and bounded immediate/constrained default-region mixed fanout | Default-region durable restore |
| Timestamped regional interactions | Region-qualified timestamped `Send/Receive Interaction`, overlap filtering, conveyed source-region metadata, no-overlap retraction boundaries, producer resignation, ordinary TAR/NMR and FQR/TARA/NMRA advances, producer Time Regulation re-enable with changed lookahead, and subscription replacement | Regional save/restore and broader mixed-fanout cases |
| Timestamped default-region interactions | Default-region timestamped `Send/Receive Interaction` with explicit subscriber regions, supplied-empty source-region metadata, constrained delivery, default-region save/restore with queued retraction recovery, bounded immediate/constrained mixed fanout with Request Retraction, FQR/TARA/NMRA advances, query bounds, and retraction | Broader mixed-fanout cases |
| Directed interactions | Pre-connect/pre-join declaration and send boundaries, `publishObjectClassDirectedInteractions`, selective or universal `subscribeObjectClassDirectedInteractions`, target routing, withdrawal, `receiveDirectedInteraction`, timestamped grant delivery, producer Time Regulation re-enable with changed lookahead, and retraction | Regional routing |
| MOM service reporting | Adapter-supplied standard MIM, `HLAreportServiceInvocation`, seven standard report parameters, typed successful `SendInteraction`, `UpdateAttributeValues`, `RequestAttributeValueUpdate`, `ReleaseMultipleObjectInstanceNames`, `ReleaseObjectInstanceName`, `ReserveObjectInstanceName`, `RegisterObjectInstance`, `DeleteObjectInstance`, `LocalDeleteObjectInstance`, and timestamped `SendInteraction` report values, reliable transport, and RTI-generated producer metadata | Provider-specific MOM storage, file formats, and diagnostics |
| Federation listing | `listFederationExecutions`/`listFederationExecutionMembers`, execution/member report callbacks, missing-execution reporting, callback-model boundaries, and disconnected evoked-report cleanup | Provider registry and transport internals |
| Federate lookup | `getFederateHandle`/`getFederateName`, pre-connect/pre-join boundaries, same-execution and foreign-handle identity, and active-name versus departed-designator behavior | Provider-specific identity internals |
| Object-name reservation | Single and multiple reservation/release, standard name validation, asynchronous contention results, mixed-set release atomicity, reuse after release, and resignation cleanup | Provider-specific name allocation and registry internals |
| Object registration/discovery | Rich-FOM hierarchy-aware registration, exact/superclass discovery identities, evoked callback cancellation, late subscription discovery, stable object/name/class lookups, and duplicate-discovery suppression | Provider-specific object registry internals |
| Allow Relaxed DDM | `getAllowRelaxedDDMSwitch` with touching-region admission for ordinary regional object updates/interactions, strict positive-gap suppression, source-region callback metadata, and adapter switch-FOM composition | Provider-specific routing/region internals |
| Multi-attribute regional object update | Separate adapter FOM for two independently region-associated attributes, complete-overlap filtering, X-only/Y-only source mutations, restoration, and conveyed source-region metadata | Broader multi-attribute timestamped, ownership, value-request, and save/restore matrices |
| Three-dimensional regional object overlap | Separate adapter FOM for three dimensions, complete-overlap discovery/reflection, one-dimension disjoint suppression, restoration, and conveyed source-region metadata | Provider-specific region internals and advanced DDM advisories |
| DDM regional services | `createRegion`/`commitRegion`, dimension metadata and bounds, `registerObjectInstanceWithRegions`, regional object/interaction publication and subscription, `associateRegionsForUpdates`, attribute In/Out Of Scope advisories and switch suppression, `requestAttributeValueUpdateWithRegions`, `sendInteractionWithRegions`, conveyed region designators, and standard negative boundaries | Advanced DDM advisories and provider diagnostics |
| Federation synchronization | `registerFederationSynchronizationPoint`, `announceSynchronizationPoint`, `synchronizationPointAchieved`, explicit synchronization sets, invalid-member failure, duplicate-label failure, and `federationSynchronized` completion | Provider-managed external barriers and process coordination |
| Asynchronous callback servicing | `enableAsynchronousDelivery`/`disableAsynchronousDelivery`, receive-order callback gating, `evokeCallback`, `evokeMultipleCallbacks`, and time-advance release | Provider threading and transport internals |
| Federation save/restore | Promoted untimed and timestamped request, status, begun/complete/not-complete, abort, failure, restore initiation, post-restore handles, queued delivery, retraction, and completion callbacks | Durable images, process restart, and provider persistence internals |
| Time management | Adapter-selected logical-time factory with lifecycle boundaries, time roles, logical-time/lookahead queries, deferred lookahead decreases, basic TAR/Grant, Flush Queue Request, Time Advance Request Available, Next Message Request Available, backward-time failure boundaries, and timestamped delivery boundaries | Provider transport internals |
| Order and transportation | Default/per-instance order controls, request/confirmation callbacks, query reports, and observable transport changes | Provider transport internals |
| Relevance and support switches | Advisory/support-switch state, active/passive declaration transitions, registration/interaction advisories, update-rate callbacks, and ordinary Delay Subscription Evaluation callback-boundary rechecking | MOM storage/file formats, DDM region relevance, and implementation-specific diagnostics |

The native-gap survey records three exact semantic mappings to this portable
surface: the regular-candidate continuation lane and two regional-interaction
fixtures. It follows local quoted includes before classifying a native test as
portable, so the three apparent `If Available` candidates are correctly
rejected: they transitively depend on the private save/restore harness, and
their retained-ownership assumptions also do not match the standard's
immediate-unavailability result. There are currently no unmatched native
integration tests that satisfy the pure portability boundary.

Everything in the translated column compiles against only the official IEEE
1516.1-2025 C++ headers and the standard library. The adapter supplies the
provider library, FOM, endpoint/configuration, callback selection, and any
optional logical-time implementation. The deferred cases remain separate
because they need additional standard services or an adapter-owned fixture;
they are not silently represented as generic tests.

## Portable FOM boundary

The portable FOM slice is complete for the public service boundary exercised by
this package. The pure `java-tck.api-surface-inventory` case checks the official
ambassador, callback, logical-time, authorization, credential, handle,
`RangeBounds`, and `VariableLengthData` types, plus standard configuration,
credential, authorization-result, enum, and federation/restore records,
including builder and copy-assignment independence, distinctness of every
standard enum family, copied/borrowed storage,
handle/value-map copy-assignment independence, record and pair/vector
copy-assignment independence, handle collections, and replacement behavior. Its collection
checks use the official `FederateHandleSet`, `InteractionClassHandleSet`,
`DimensionHandleSet`, `RegionHandleSet`, `AttributeHandleSet`, and
`ParameterHandleSet` typedefs and exercise copy, assignment, lookup, duplicate
insertion, and erase semantics. It also checks the standard HLA exception
name/message, copy, assignment, polymorphic access, and stream contract.
It also instantiates the standard `NullFederateAmbassador` and invokes every
callback overload once, providing a provider-independent compile and no-op
dispatch check for the complete callback surface.
The standard `Authorizer` and `AuthorizerFactory` interfaces are also exercised
through local implementations, keeping the auth-extension check provider- and
FOM-independent.
The official `EncoderException` contract and abstract `DataElement` clone,
same-type, encoding, boundary, hash, and decode operations are exercised with a
local standard-library-only element, keeping the encoding-foundation check
provider- and FOM-independent.
The abstract `LogicalTime`, `LogicalTimeInterval`, and `LogicalTimeFactory`
interfaces are likewise exercised through local implementations, including
boundary values, arithmetic, comparisons, difference, both encoding overloads,
both decoding overloads, diagnostics, and standard error boundaries.
The same standard-value inventory checks `VariableLengthData` empty construction,
copy independence, borrowed storage, custom-deleter adoption, and the default
array-deleter `takeDataPointer` overload without provider or FOM dependencies.
The promoted `cpp-tck.variable-length-data-contract` runner exposes this value
contract as an independently selectable slice, so it can be verified without a
provider connection or FOM load.
The promoted `cpp-tck.logical-time-contract` runner similarly exposes the
concrete standard logical-time value and factory contract as an independently
selectable, provider- and FOM-independent slice; RTI time-role and time-advance
behavior remains in `java-tck.logical-time-factory` and `java-tck.time-advance`.
The promoted `cpp-tck.exception-hierarchy-contract` runner exposes the
complete official C++ exception hierarchy as an independently selectable,
provider- and FOM-independent slice; its cross-language parity anchor is
`java-tck.overloads-and-exceptions`.
The promoted `cpp-tck.enum-contract` runner exposes the official C++
enumeration families as an independently selectable, provider- and
FOM-independent slice; its parity anchor is
`java-tck.api-surface-inventory`.
The promoted `cpp-tck.handle-and-collection-contract` runner exposes the
official C++ handle, range, map, set, pair-vector, and federation/restore-vector
value contract as an independently selectable, provider- and FOM-independent
slice; its parity anchor is `java-tck.api-surface-inventory`.
The promoted `cpp-tck.configuration-and-authorization-contract` runner
exposes the official C++ configuration, federation-record, credential,
authorization, and `Authorizer`/`AuthorizerFactory` contract as an independently
selectable, provider- and FOM-independent slice; its parity anchor is
`java-tck.api-surface-inventory`.
The promoted `cpp-tck.authorizer-factory-factory-contract` runner exposes the
official C++ `HLAauthorizerFactoryFactory` selection, naming, creation, and
unsupported-name contract as an independently selectable, provider- and
FOM-independent slice; its parity anchor is `java-tck.api-surface-inventory`.
The promoted `cpp-tck.runtime-identity-contract` runner exposes the official
C++ `rtiName()`/`rtiVersion()` callability, non-empty-value, and process-stability
contract as an independently selectable, provider-neutral slice; its parity
anchor is `java-tck.factory-discovery`.
The promoted `cpp-tck.rti-ambassador-factory-contract` runner exposes the
official C++ `RTIambassadorFactory` construction and repeatable ambassador
creation contract as an independently selectable, provider- and FOM-independent
slice; its parity anchor is `java-tck.factory-discovery`.
The promoted `cpp-tck.logical-time-factory-factory-contract` runner exposes the
official logical-time factory-factory default and integer selection, reference-
factory forwarding, unknown-name rejection, and initial-value construction as an
independently selectable, provider- and FOM-independent slice; its parity anchor
is `java-tck.logical-time-factory`.
The promoted `cpp-tck.logical-time-data-elements-contract` runner exposes the
standard `HLAlogicalTime` and `HLAlogicalTimeInterval` DataElement wrapper
round-trip, nested-buffer, clone/copy, type-compatibility, boundary, and
truncation contract as an independently selectable adapter-backed slice; its
parity anchor is `java-tck.logical-time-factory`.
The promoted `cpp-tck.connection-callback-contract` runner exposes the standard
connection and callback-control surface as an independently selectable,
adapter-backed slice: all four `connect` overloads, unsupported callback-model
rejection, pre-connect boundaries, callback servicing, disconnect, and
reconnect; its parity anchor is `java-tck.overloads-and-exceptions`.
The promoted `cpp-tck.null-federate-ambassador-contract` runner exposes the
official C++ `NullFederateAmbassador` callback and overload contract as an
independently selectable, provider- and FOM-independent slice; its parity
anchor is `java-tck.api-surface-inventory`.
The promoted `cpp-tck.data-element-contract` runner exposes the official
C++ `DataElement` base clone, type, encoding, boundary, hash, and decode
contract as an independently selectable, provider- and FOM-independent
slice; its parity anchor is `java-tck.api-surface-inventory`.
The promoted `cpp-tck.composite-data-elements-contract` runner similarly exposes
the composite array, record, and variant-record wire and validation contract as
an independently selectable, provider- and FOM-independent slice.
The promoted `cpp-tck.basic-data-elements-contract` runner similarly exposes the
scalar basic-data-element wire and validation contract as an independently
selectable, provider- and FOM-independent slice; composite record, array, and
variant-record coverage is separately exposed by
`cpp-tck.composite-data-elements-contract` and retained in
`java-tck.encoder-round-trip` for cross-language parity.
The exception portion instantiates all 109 official derived exception classes, so
the source compile-checks the complete public exception family without provider
headers or provider-specific behavior.
`cpp-tck.fom-model` loads an
adapter-supplied valid model and
checks root/derived object and interaction classes, inherited handle identity,
dimensions and upper bounds, update-rate values, transportation names,
advisory switches, publication/subscription, registration/discovery, ordinary
typed attribute reflection, and typed interaction parameter delivery. The
encoder case covers the official 16/32/64-bit integer, boolean, floating-point,
  text, opaque, array, fixed-record, variant-record, and extendable-variant-record
  data-element classes, including UTF-16BE element counts, alignment, signed counts,
  caller-owned opaque-storage write-through, copied/borrowed storage, custom
  boundary, type-shape, malformed boolean, text, opaque, and truncated-value
  boundaries. The
malformed-input case covers source and encoded-value rejection through standard
exceptions.

`cpp-tck.fom-module-composition` checks both module composition during
federation creation and an additional module supplied during join. It also
checks that declarations become visible to existing members and that handles
remain consistent across members. The fixture set is installed with the
package, but every path is still supplied by the adapter so another provider
can replace it.

The promoted `cpp-tck.fom-model-contract`,
`cpp-tck.fom-module-composition-contract`, and
`cpp-tck.fom-empty-module-validation-contract` runners expose these FOM
surfaces as independently selectable pure C++ contracts. They use only the
official IEEE C++ API and standard library; the adapter supplies the valid
model, additional module, and invalid or empty-module inputs. They do not
assert private parser behavior or provider-specific diagnostics.

`cpp-tck.federation-mom-current-fdd` checks the standard federation MOM object
and `HLAcurrentFDD` attribute through discovery, handle/name round trips,
reliable transportation reporting, explicit attribute-value requests, and the
join-triggered refresh after an adapter-supplied additional FOM module is
joined. It uses the standard `HLAfederation` object instance name and an
adapter-supplied standard MIM; no provider or private header is involved.

The promoted `cpp-tck.explicit-mim-creation-contract` and
`cpp-tck.federation-mom-current-fdd-contract` runners expose those MIM and
federation-MOM boundaries as independently selectable pure standard C++
contracts. They use only official IEEE C++ API headers and the standard
library; the provider package, FOM/MIM modules, endpoint, callback model, and
logical-time implementation remain adapter inputs.

`cpp-tck.joined-federate-mom-federate-state-save-restore` observes the standard
`HLAfederateState` MOM attribute from a joined federate across save initiation,
save completion, restore initiation, and restore completion. It verifies the
state sequence `1 -> 3 -> 1 -> 5 -> 1`, conditional initial-state reflection,
suppression of the saving federate's own state reflection, standard save and
restore callbacks, and reliable RTI-generated metadata in both callback models
using only the standard MIM and IEEE C++ API.

The timestamped attribute update-rate reduction scenario uses the same
adapter-supplied rich-FOM boundary to exercise an active named best-effort
rate alongside reliable attributes. It verifies timestamped reflect callbacks,
time-advance delivery, suppression of excess best-effort passels, and the
standard second-retract terminal exception without depending on private FOM or
provider configuration APIs.

The DDM scenarios use the separate adapter-supplied `ddm-tck.xml` fixture (or
an equivalent model) and a repeatable dimension-name input. The fixture
contains two dimensions, an ordinary object attribute, and an ordinary
interaction parameter. Region handles, dimension names, upper bounds,
range-bounds, conveyed designators, and delivery filtering are verified
through the official API only. Region creation, region-handle decoding, and
dimension-set lookup also assert their standard `NotConnected` and
`FederateNotExecutionMember` boundaries before the joined-federation checks;
the test source does not inspect provider FOM internals.

The three-dimensional regional-object scenario uses the separate
adapter-supplied `ddm-three-dimensional-tck.xml` fixture (or an equivalent
model) with three dimensions and one ordinary byte attribute. It verifies
complete-overlap routing, one-dimension disjoint suppression, restoration, and
the conveyed source-region designator through the official API. The adapter
selects the fixture and supplies the dimension, object-class, and attribute
names; the portable source has no provider-specific FOM dependency.

The synchronization-point scenario exercises pre-connect and pre-join
service boundaries, global and explicit-set point registration, announcements,
duplicate-label failure, achievement, and completion failure sets. The
callback-control scenario verifies callback
disable/enable gating around a real interaction. The asynchronous-delivery
scenario verifies the receive-order gate and both standard callback-servicing
operations. The promoted save/restore scenario covers pre-connect and pre-join
boundaries across the standard save/restore control services, untimed lifecycle
and status callbacks, abort and failure boundaries, and the post-restore
federate-handle/name relationship. The promoted timestamped save/restore
scenario adds timed initiation and restored queued-delivery/retraction
boundaries. The promoted timed regional interaction save/restore scenario
extends that boundary with source-region metadata, Flush Queue delivery, and
restored retraction state; three repeat focused runs passed all 6/6
callback-model cases before promotion. The promoted default-region interaction
save/restore scenario adds the complementary empty source-region designator
boundary; three repeat focused runs passed all 6/6 callback-model cases before
promotion. The promoted default-region attribute save/restore scenario extends
that boundary to timestamped object-attribute reflection, including empty
conveyed source-region metadata, Flush Queue ordering, and restored retraction
state; three repeat focused runs passed all 6/6 callback-model cases before
promotion. The promoted attribute-scope scenario adds regional scope-transition
callbacks and switch gating. The promoted declaration-relevance scenario adds
active/passive regional declaration advisories. All remain on the public
`RTIambassador`/`FederateAmbassador` boundary.

This boundary deliberately does not claim to test private XML parser/schema
internals. Those implementation-level checks remain provider-specific; the
transplantable TCK verifies what a conforming application can observe through
the official API. Durable save images, process restart, advanced DDM
advisories, and provider-specific diagnostics remain outside this slice.

## Portability boundary

The generic CMake project accepts these adapter inputs:

- `HLA_RTI_TCK_API_INCLUDE_DIR`: the official API include directory;
- `HLA_RTI_TCK_PROVIDER_LIBRARIES`: provider library files or imported targets;
- `HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS`: provider ABI/build definitions;
- `HLA_RTI_TCK_PROVIDER_LINK_OPTIONS`: provider-specific linker inputs.

The reusable source does not name or discover a provider. A provider adapter is
responsible for its runtime search path, endpoint configuration, and any
provider-owned process fixture. FOM class and interaction names are command
line parameters, and the installed-package adapter exposes the valid model,
additional modules, invalid corpus, DDM FOM, DDM dimensions, and logical-time
implementation as configuration, so a provider can reuse the test logic with
an equivalent portable fixture without editing the test source. The current
adapter names these DDM inputs
`HLA_RTI_TCK_ADAPTER_DDM_FOM` and
`HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS`. The three-dimensional slice uses
`HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM`,
`HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS`,
`HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_OBJECT_CLASS`, and
`HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_ATTRIBUTE`. The adapter also accepts
`HLA_RTI_TCK_ADAPTER_OBJECT_CLASS`,
`HLA_RTI_TCK_ADAPTER_ATTRIBUTE`,
`HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_OBJECT_CLASS`,
`HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FIRST`,
`HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_SECOND`,
`HLA_RTI_TCK_ADAPTER_INTERACTION_CLASS`, and
`HLA_RTI_TCK_ADAPTER_PARAMETER` so equivalent FOM names apply to both CTest
and direct evidence. The custom transportation regional-attribute slice also
accepts `HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_OBJECT_CLASS`,
`HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_ATTRIBUTE`,
`HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_INTERACTION_CLASS`, and
`HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_PARAMETER`.

Run the source-boundary validator from the repository root:

```text
python tools/cpp_tck.py
```

Run the standard-library-only API-surface audit alongside it. The audit reads
the official IEEE C++ `RTIambassador` and `FederateAmbassador` headers and
requires every public method and callback to be represented in the portable
catalog and source:

```text
python tools/audit_cpp_tck_api_surface.py
```

Use the standard-library-only Python orchestrator for an installed provider
package. It configures and builds the downstream CMake adapter, then runs the
CTest matrix through explicit subprocess argument lists. No shell or package
registry lookup is involved:

```text
python tools/run_cpp_tck.py --package-prefix .build-fom-services/package-smoke-install --build-directory .build/cpp-tck-installed --time-implementation HLAinteger64Time --configuration Debug --scenario-set verified --callback-model both
```

The same runner can produce direct machine-readable evidence. Repeat
`--scenario` for a focused slice and use `--skip-ctest` after the matrix gate
has already been recorded:

```text
python tools/run_cpp_tck.py --package-prefix .build-fom-services/package-smoke-install --build-directory .build/cpp-tck-installed --scenario cpp-tck.regional-three-dimensional-overlap --skip-ctest --results .build/cpp-tck-all/p159-regional-three-dimensional-overlap-focused-results.json
```

That focused artifact intentionally contains one scenario under both callback
models. For the catalog-wide evidence check, omit `--scenario` and validate the
resulting full promoted artifact with
`python tools/cpp_tck.py --results <file> --promotion promoted`.
The validator rejects failed or unapproved skipped cases; it permits only the
explicit `cpp-tck.connection-loss-cleanup` skip whose message states that an
adapter-managed connection-loss fixture is required.

This registers each selected catalog scenario as a separate CTest for each
selected callback model, using the adapter’s FOM, standard MIM, DDM, switch,
and callback
configuration. The default `--scenario-set verified` selects the 352 catalog
entries with `promotion=promoted`, which produces 704 cases with
`--callback-model both`.
After that gate is green, pass `--scenario-set all` to include the later
adapter-required entries; the current catalog contains 362 IDs and 724 cases,
including ten candidates. The candidate-inclusive evidence figures below were
recorded before the two ownership contract twins were promoted and therefore
cover the prior 344-ID, 688-case catalog. The no-fixture candidate-inclusive
baseline completes 686/686
CTest cases with no failures and records 676 direct passes plus 12 explicit
skips: the ten immediate-model candidate skips and the two connection-loss
skips. With `--connection-loss-fixture <path>`, the shell-free Python adapter
owns the external fault; the ordinary matrix remains 686/686 CTest cases and
the merged direct evidence records 678 passes plus only the ten documented
candidate immediate-model skips across all 688 callback-model cases. The
candidate skips cover Willing-to-Acquire continuation, the ownership-acquisition
cancellation transfer race, and the regular-candidate continuation,
pre-delivery-cancellation, and confirmation-cancellation twins. Supply
`--connection-loss-fixture <path>` to `tools/run_cpp_tck.py`; it excludes that
scenario from the ordinary CTest matrix and merges the Python adapter's two
callback-model results into the direct evidence. The aggregate
`hla_rti_cpp_tck_installed` CTest remains available for a single full-run
check. `--model-fom`, `--ddm-fom`, `--mim-fom`, `--switches-fom`, repeated
`--ddm-dimension`, `--multi-attribute-fom`, `--three-dimensional-fom`, repeated
`--three-dimensional-dimension`, `--additional-fom`, `--invalid-fom`, and
`--time-implementation` select the FOM corpus, DDM dimensions, and logical-time
declaration.
`--rti-address`, `--configuration-name`, and `--additional-settings` are available
as adapter options and are passed only through the standard `RtiConfiguration`
API.

The no-fixture installed-package baseline (2026-09-08) remains recorded in
`.build\cpp-tck-all\all-candidate-evidence-standard-timed-ownership-contracts-final.json`.
The latest fixture-backed installed-package evidence is recorded in
`.build\cpp-tck-all\all-candidate-evidence-with-connection-loss.json`:
CTest passed 686/686 ordinary runnable cases, the direct lane recorded 678
passes plus the ten documented candidate immediate-model skips, and the merged
688-case JUnit artifact reported zero failures and zero skips. Both
connection-loss callback-model cases passed through the shell-free Python
adapter fixture.
The matching native survey is recorded in
`.build\cpp-tck-all\native-survey-standard-mom-save-restore-contracts-final.txt`;
it reports 344 catalog IDs, zero portable candidates, and 244 unmatched native
stems. The promoted-only aggregate remains recorded in
`.build\cpp-tck-all\verified-evidence-standard-mom-save-restore-contracts-final.json`
with 666/666 runnable CTest cases, 666 direct passes, and the two expected
connection-loss skips. Earlier
focused gates remain recorded under their scenario-specific evidence files. The
earlier installed-package standard-API inventory gate is recorded in
`.build\cpp-tck-all\verified-evidence-auth.json`: CTest passed 398/398
promoted cases, and the direct lane recorded 398 passes plus the two expected
connection-loss skips (200 evoked and 200 immediate results). The subsequent
official-enum gate is recorded in
`.build\cpp-tck-all\verified-evidence-enums.json` with the same 398/398 CTest
and 398-pass/two-skip direct result. The
standard-exception-hierarchy gate is recorded in
`.build\cpp-tck-all\verified-evidence-exceptions.json` with the same 398/398
CTest and 398-pass/two-skip direct result. The
standard-callback-contract gate is recorded in
`.build\cpp-tck-all\verified-evidence-null-federate.json` with the same 398/398
CTest and 398-pass/two-skip direct result. The
standard-authorizer-contract gate is recorded in
`.build\cpp-tck-all\verified-evidence-authorizer.json` with the same 398/398
CTest and 398-pass/two-skip direct result. The
standard-encoding-contract gate is recorded in
`.build\cpp-tck-all\verified-evidence-encoding-contract.json` with the same
398/398 CTest and 398-pass/two-skip direct result. The
standard-logical-time-contract gate is recorded in
`.build\cpp-tck-all\verified-evidence-logical-time-contract.json` with the
same 398/398 CTest and 398-pass/two-skip direct result. The
standard-value-storage-contract gate is recorded in
`.build\cpp-tck-all\verified-evidence-vld-default-adoption.json` with the same
398/398 CTest and 398-pass/two-skip direct result. The
independent `cpp-tck.variable-length-data-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-variable-length-data-contract-final.json`:
CTest passed 400/400, while the direct lane passed 400 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 201
scenario IDs in both callback models. The
independent `cpp-tck.basic-data-elements-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-basic-data-elements-contract-final.json`:
CTest passed 402/402, while the direct lane passed 402 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 202
scenario IDs in both callback models. The scalar slice covers standard integer,
Boolean, octet/byte, floating-point, ASCII/Unicode, opaque-data, and octet-pair
wire and validation behavior; composite record, array, and variant-record
coverage remains in `java-tck.encoder-round-trip`. The
independent `cpp-tck.composite-data-elements-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-composite-data-elements-contract-final.json`:
CTest passed 404/404, while the direct lane passed 404 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 203
scenario IDs in both callback models. The composite slice covers fixed and
variable arrays, fixed records, nested record arrays, variant records, and
extendable variant records, including alignment and malformed-boundary checks.
The independent `cpp-tck.logical-time-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-logical-time-contract-final.json`:
CTest passed 406/406, while the direct lane passed 406 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 204
scenario IDs in both callback models. It covers provider-independent logical-
time values, intervals, factories, encodings, boundaries, and arithmetic.
The independent `cpp-tck.exception-hierarchy-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-exception-hierarchy-contract-final.json`:
CTest passed 408/408, while the direct lane passed 408 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 205
scenario IDs in both callback models. It covers every official derived
exception constructor plus standard name/message, copy/assignment,
polymorphic-base, and stream behavior.
The independent `cpp-tck.enum-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-enum-contract-final.json`:
CTest passed 410/410, while the direct lane passed 410 cases, skipped the
two adapter-managed connection-loss cases, and reported no failures across
206 scenario IDs in both callback models. It covers the distinct standard
values for settings, callback, order, resign, save, restore, service-group,
synchronization, and authorization-result families.
The independent `cpp-tck.handle-and-collection-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-handle-and-collection-contract-final.json`:
CTest passed 412/412, while the direct lane passed 412 cases, skipped the
two adapter-managed connection-loss cases, and reported no failures across
207 scenario IDs in both callback models. It covers invalid-handle identity,
copy/assignment, ordering and hashing, handle sets, value maps, `RangeBounds`,
region-pair vectors, and federation/restore record vectors.
The independent `cpp-tck.configuration-and-authorization-contract` slice is
recorded in
`.build\cpp-tck-all\verified-evidence-configuration-and-authorization-contract-final.json`:
CTest passed 414/414, while the direct lane passed 414 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 208
scenario IDs in both callback models. It covers configuration builders and
results, federation/restore records, credentials, authorization results, and
`Authorizer`/`AuthorizerFactory` polymorphism.
The independent `cpp-tck.null-federate-ambassador-contract` slice is recorded
in
`.build\cpp-tck-all\verified-evidence-null-federate-ambassador-contract-final.json`:
CTest passed 416/416, while the direct lane passed 416 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 209
scenario IDs in both callback models. It invokes every standard no-op callback
family and each ordinary/timestamped callback overload used by the portable TCK.
The independent `cpp-tck.data-element-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-data-element-contract-final.json`:
CTest passed 418/418, while the direct lane passed 418 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 210
scenario IDs in both callback models. It covers clone/type identity, encoded
length and boundary, hash stability, append encoding, direct decode, and offset
decode.
The independent `cpp-tck.authorizer-factory-factory-contract` slice is recorded
in
`.build\cpp-tck-all\verified-evidence-authorizer-factory-factory-final.json`:
CTest passed 420/420, while the direct lane passed 420 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 211
scenario IDs in both callback models. It covers standard-authorizer selection,
factory and authorizer naming/creation, and unsupported-name rejection without
asserting provider-specific authorization policy.
The independent `cpp-tck.runtime-identity-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-runtime-identity-final.json`:
CTest passed 422/422, while the direct lane passed 422 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 212
scenario IDs in both callback models. It checks only non-empty, process-stable
`rtiName()` and `rtiVersion()` values; vendor-specific strings remain
unconstrained.
The independent `cpp-tck.rti-ambassador-factory-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-rti-ambassador-factory-final.json`:
CTest passed 424/424, while the direct lane passed 424 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 213
scenario IDs in both callback models. It checks only standard factory
construction and repeatable creation of non-null `RTIambassador` objects; it
does not assert provider-specific configuration, endpoint, FOM, or service
behavior.
The independent `cpp-tck.logical-time-factory-factory-contract` slice is
recorded in `.build\cpp-tck-all\verified-evidence-logical-time-factory-factory-final.json`:
CTest passed 426/426, while the direct lane passed 426 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 214
scenario IDs in both callback models. It checks only standard default and
integer factory selection, reference-factory forwarding, unknown-name rejection,
and initial-value construction without provider, endpoint, or FOM assumptions.
The independent `cpp-tck.logical-time-data-elements-contract` slice is
recorded in `.build\cpp-tck-all\verified-evidence-logical-time-data-elements-final.json`:
CTest passed 428/428, while the direct lane passed 428 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 215
scenario IDs in both callback models. It isolates the standard
`HLAlogicalTime` and `HLAlogicalTimeInterval` DataElement wrapper round trips,
nested-buffer boundaries, clone/copy independence, type compatibility, and
truncation failures while taking the provider package, FOM, endpoint, callback
model, and logical-time implementation from the adapter.
The independent `cpp-tck.connection-callback-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-connection-callback-final.json`:
CTest passed 430/430, while the direct lane passed 430 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 216
scenario IDs in both callback models. It isolates the standard four connection
overloads, callback-model rejection, pre-connect boundaries, callback controls
and servicing, disconnect, and reconnect while taking the provider package,
endpoint, and callback configuration from the adapter.
The independent `cpp-tck.federation-lifecycle-contract` slice is recorded in
`.build\cpp-tck-all\verified-evidence-federation-lifecycle-final.json`:
CTest passed 432/432, while the direct lane passed 432 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 217
scenario IDs in both callback models. It isolates the standard federation
create/join/resign/destroy, automatic-resign directive, federation and member
reports, federate handle lookups, ordinary resignation cleanup, and
missing-federation boundaries while taking the provider package, FOM, endpoint,
callback configuration, and logical-time implementation from the adapter.
The independent pure P0 declaration, ordinary object-management, named
registration, ordinary object-attribute subscription, local object deletion,
ordinary object-publication registration fencing, ordinary interaction
subscription and publication fencing, ordinary attribute/interaction,
ordinary directed-interaction publication and target-lifecycle fencing, ordinary
directed-interaction subscription-kind fencing, ordinary directed-interaction,
unnamed-join, federation listing, federate identity lookup, order-control,
receive-order callback cancellation, order/transportation lookup, callback-control,
resignation-time
deletion, unconditional divestiture, final-federate cleanup, pending-acquisition
rejection/cancellation, negotiated divestiture cancellation/pre-delivery
cancellation, update-rate query, handle wire-format, and timestamped directed
source-resignation, Next Message Request, available-time-advance, Query
GALT/LITS, ordinary attribute-value request/response, timestamped interaction
delivery, timestamped interaction ordering/fan-out, timestamped attribute
ordering/fan-out, timestamped attribute advance/re-enable, source-resignation,
and Time Regulation re-enable contract slices, plus the ordinary and directed
timestamped interaction re-enable contracts, source-resignation fan-out,
joined-owner retraction cleanup, and mixed alternate-advance object-deletion
contracts, the pure object-name reservation and object registration/discovery
lifecycle contracts, and the timestamped attribute ownership-transfer contract,
are
recorded together in
`.build\cpp-tck-all\verified-evidence-standard-timestamped-ownership-transfer-contract-final.json`:
CTest passed 640/640 runnable cases, while the direct lane passed 640 cases, skipped the two
adapter-managed connection-loss cases, and reported no failures across 321
scenario IDs in both callback models. Each slice uses only the official C++ API
and standard library while taking the provider package, FOM, endpoint, and
callback configuration from the adapter.
The focused timestamped re-enable base and contract lanes each passed 8/8
callback-model cases, and the focused object-lifecycle base and contract lanes
each passed 4/4 callback-model cases. They are recorded in
`.build\cpp-tck-all\focused-standard-timestamped-reenable-base.json`,
`.build\cpp-tck-all\focused-standard-timestamped-reenable-contracts.json`,
`.build\cpp-tck-all\focused-standard-object-lifecycle-base.json`, and
`.build\cpp-tck-all\focused-standard-object-lifecycle-contracts.json`. The
focused timestamped ownership-transfer base and contract lane passed 4/4
callback-model cases in
`.build\cpp-tck-all\focused-standard-timestamped-ownership-transfer-contract.json`.
The same artifact records the independent pure standard ownership,
synchronization-point, federation save/restore, callback-servicing,
timestamped ordinary update/deletion, including no-fanout, tombstone, and
Time Regulation re-enable deletion boundaries, and alternate time-advance contract
support-service lookup, ordinary order/transport, advisory-switch, and FOM
model/module-composition/empty-module-validation slices;
ordinary MOM service-report interaction, attribute-update, object-registration,
and object-deletion success slices; ordinary interaction, attribute-update, and
regional-interaction service-report failure contract slices; timestamped
attribute-update, interaction, and object-deletion service-report failure
contract slices;
time-dependent slices additionally take the adapter-supplied
logical-time implementation and use no provider-specific save format or header.
The
the no-fixture candidate-inclusive direct lane passed 676 cases and explicitly
skipped ten candidate immediate cases plus the two connection-loss cases (688
total) in
`.build\cpp-tck-all\all-candidate-evidence-standard-timed-ownership-contracts-final.json`.
The fixture-backed candidate-inclusive direct lane passed 678 cases and
explicitly skipped only the ten candidate immediate cases in
`.build\cpp-tck-all\all-candidate-evidence-with-connection-loss.json`; the focused regular-candidate
continuation lane passed one evoked case and explicitly skipped its immediate
case in `.build\cpp-tck-all\candidate-negotiated-regular-evidence.json`,
with three independent evoked repeats also passing; the focused regular
confirmation-cancellation lane passed one evoked case and explicitly skipped
its immediate case in
`.build\cpp-tck-all\candidate-negotiated-regular-confirmation-cancel-both-evidence.json`,
with three independent evoked repeats also passing; the focused
regular pre-delivery cancellation lane passed one evoked case and explicitly
skipped its immediate case in
`.build\cpp-tck-all\candidate-negotiated-regular-pre-delivery-cancel-both-evidence.json`,
with three independent evoked repeats also passing; the focused
federation-teardown-isolation lane passed 2/2 in
`.build\cpp-tck-all\federation-teardown-isolation-candidate.json`, with three
repeat focused runs also passing 2/2; the focused
federation MOM current-FDD lane passed 2/2 in
`.build\cpp-tck-all\federation-mom-current-fdd-promoted.json`; the focused
joined-federate MOM save/restore state lane passed 2/2 in
`.build\cpp-tck-all\joined-federate-mom-federate-state-save-restore-candidate.json`;
the ordinary promoted CTest gate passed 398/398 cases and the adapter-owned
direct gate passed 400/400; the focused
timed-regional-interaction-save-restore lane passed 2/2 in
`.build\cpp-tck-all\timed-regional-interaction-save-restore-candidate.json`,
with three repeat focused runs also passing 2/2; the focused
timed-default-region-interaction-save-restore lane passed 2/2 in
`.build\cpp-tck-all\timed-default-region-interaction-save-restore-candidate.json`,
with three repeat focused runs also passing 2/2; the focused
timed-default-region-attribute-save-restore lane passed 2/2 in
`.build\cpp-tck-all\timed-default-region-attribute-save-restore-candidate.json`,
with three repeat focused runs also passing 2/2; the focused current-process
connection-loss adapter lane passed 2/2 in
`.build\cpp-tck-all\connection-loss-current-process-python.json`; the focused
API-surface inventory lane passed 2/2 in
`.build\cpp-tck-all\api-surface-inventory-focused.json`; the focused
regional declaration-relevance advisory lane passed 2/2 in
`.build\cpp-tck-all\regional-declaration-relevance-focused.json`; the focused
regional-interaction source-region snapshot lane passed 2/2 in
`.build\cpp-tck-all\regional-interaction-source-region-snapshot-focused.json`; the focused
regional-interaction subscription-report lane passed 2/2 in
`.build\cpp-tck-all\regional-interaction-subscription-service-report-focused.json`; the focused
regional service-report contract-twin lane passed 8/8 in
`.build\cpp-tck-all\focused-standard-regional-service-report-contracts.json`; the focused
MOM save/restore contract-twin lane passed 8/8 in
`.build\cpp-tck-all\focused-standard-mom-save-restore-contracts.json`; the focused
ownership candidate contract-twin lane recorded 4 passes and 4 explicit
immediate-model skips in
`.build\cpp-tck-all\focused-standard-ownership-candidate-contracts.json`; the focused
timed regular-candidate contract-twin lane recorded 2 evoked passes and 2
explicit immediate-model skips in
`.build\cpp-tck-all\focused-standard-timed-ownership-candidate-continuation-contract.json`; both
candidate lanes remain `promotion=candidate` pending broader adapter coverage. The focused
timed pre-delivery cancellation contract-twin lane recorded 2 evoked passes and
2 explicit immediate-model skips in
`.build\cpp-tck-all\focused-standard-timed-ownership-candidate-pre-delivery-contract.json`; it
also remains `promotion=candidate` pending broader adapter coverage. The focused
timed confirmation-cancellation contract-twin lane recorded 2 evoked passes and
2 explicit immediate-model skips in
`.build\cpp-tck-all\focused-standard-timed-ownership-candidate-confirmation-contract.json`; it
also remains `promotion=candidate` pending broader adapter coverage. The focused
factory-discovery lane passed 2/2 in
`.build\cpp-tck-all\factory-discovery-focused.json`; the focused
MOM transportation-type-change request lane passed 2/2 in
`.build\cpp-tck-all\mom-transportation-type-change-request-focused.json`; the focused
directed-interactions-sent lane passed 2/2 in
`.build\cpp-tck-all\joined-federate-mom-directed-interactions-sent-counts-focused.json`; the focused
federation MOM save-conditionals lane passed 2/2 in
`.build\cpp-tck-all\federation-mom-save-conditionals-focused.json`; the focused
joined-federate MOM ownership/removal lane passed 2/2 in
`.build\cpp-tck-all\mom-ownership-query.json`; the focused
timestamped attribute-update no-fanout lane passed 2/2 in
`.build\cpp-tck-all\timestamped-attribute-update-no-fanout-focused.json`; the focused
timestamped attribute-update ownership-transfer lane passed 2/2 in
`.build\cpp-tck-all\timestamped-attribute-ownership-transfer-focused.json`; the focused
timestamped attribute source-resignation lane passed 2/2 in
`.build\cpp-tck-all\timestamped-attribute-source-resignation-focused.json`; the focused
timestamped attribute-order cohort lane passed 2/2 in
`.build\cpp-tck-all\timestamped-attribute-order-cohort-focused.json`; the focused
order-type-control lane passed 2/2 in
`.build\cpp-tck-order-controls\order-type-controls-focused.json`; the focused
timestamped attribute-update queued-passel retraction lane passed 2/2 in
`.build\cpp-tck-all\queued-passel-isolation.json`; the focused
timestamped directed TAR/NMR lane passed 2/2 in
`.build\cpp-tck-all\timestamped-directed-interaction-tar-nmr-focused.json`; the focused
timestamped directed TAR/NMR and immediate-source-resignation contract lane
passed 8/8 in
`.build\cpp-tck-all\focused-standard-timestamped-directed-interaction-tar-nmr-resignation-contracts.json`; the focused
Query LITS source-resignation and partial ownership contract lane passed 8/8 in
`.build\cpp-tck-all\focused-standard-query-lits-partial-ownership-contracts.json`; the focused
federation teardown, mixed update-rate, and timestamped update-rate reduction
contract lane passed 12/12 in
`.build\cpp-tck-all\focused-standard-update-rate-isolation-contracts.json`; the focused
explicit-MIM creation and federation-MOM current-FDD contract lane passed 8/8 in
`.build\cpp-tck-all\focused-standard-mim-mom-contracts.json`; the focused
named-registration lane passed 2/2 in
`.build\cpp-tck-all\named-registration-focused.json`; the focused
named-registration contract lane passed 2/2 in
`.build\cpp-tck-all\focused-named-registration-contract.json`; the focused
ordinary object-attribute subscription lifecycle contract lane passed 2/2 in
`.build\cpp-tck-all\focused-object-attribute-subscription-lifecycle-contract.json`; the focused
ordinary local-delete object-instance contract lane passed 2/2 in
`.build\cpp-tck-all\focused-local-delete-object-instance-contract.json`; the focused
timestamped local-delete attribute lane passed 2/2 in
`.build\cpp-tck-all\timestamped-local-delete-attribute-focused.json`; the focused
timestamped local-delete object lane passed 2/2 in
`.build\cpp-tck-all\timestamped-local-delete-object-focused.json`; the focused
ordinary local-delete object-instance lane passed 2/2 in
`.build\cpp-tck-all\local-delete-object-instance-focused.json`; the focused
timestamped object-deletion no-fanout lane passed 2/2 in
`.build\cpp-tck-all\timestamped-object-deletion-no-fanout-focused.json`; the focused
receive-order attribute-update callback-cancellation lane passed 2/2 in
`.build\cpp-tck-all\receive-order-attribute-update-callback-cancellation-focused.json`; the focused
receive-order interaction callback-cancellation lane passed 2/2 in
`.build\cpp-tck-all\receive-order-interaction-callback-cancellation-focused.json`; the focused
ordinary interaction subscription lifecycle lane passed 2/2 in
`.build\cpp-tck-all\interaction-subscription-lifecycle-focused.json`; the focused
ordinary object-attribute subscription lifecycle lane passed 2/2 in
`.build\cpp-tck-all\object-attribute-subscription-lifecycle-focused.json`; the focused
ordinary object-publication registration-fence lane passed 2/2 in
`.build\cpp-tck-all\object-publication-registration-fence-focused.json`; the focused
ordinary object-publication registration-fence contract lane passed 2/2 in
`.build\cpp-tck-all\focused-object-publication-registration-fence-contract.json`; the focused
ordinary interaction-subscription lifecycle contract lane passed 2/2 in
`.build\cpp-tck-all\focused-subscription-contract.json`; the focused
ordinary interaction-publication send-fence lane passed 2/2 in
`.build\cpp-tck-all\interaction-publication-send-fence-focused.json`; the focused
ordinary interaction-publication send-fence contract lane passed 2/2 in
`.build\cpp-tck-all\focused-publication-contract.json`; the focused
directed interaction-publication send-fence lane passed 2/2 in
`.build\cpp-tck-all\directed-interaction-publication-send-fence-focused.json`; the focused
directed interaction-publication send-fence contract lane passed 2/2 in
`.build\cpp-tck-all\focused-directed-publication-contract.json`; the focused
directed interaction target-lifecycle lane passed 2/2 in
`.build\cpp-tck-all\directed-target-lifecycle-focused.json`; the focused
directed interaction target-lifecycle contract lane passed 2/2 in
`.build\cpp-tck-all\focused-directed-target-contract.json`; the focused
unnamed federation join overload contract lane passed 2/2 in
`.build\cpp-tck-all\focused-unnamed-join-overload-contract.json`; the focused
order and transportation lookup contract lane passed 2/2 in
`.build\cpp-tck-all\focused-order-transport-lookups-contract.json`; the focused
callback controls contract lane passed 2/2 in
`.build\cpp-tck-all\focused-callback-controls-contract.json`; the focused
resign-time object-deletion contract lane passed 2/2 in
`.build\cpp-tck-all\focused-resign-delete-objects-contract.json`; the focused
resign-time unconditional-divestiture contract lane passed 2/2 in
`.build\cpp-tck-all\focused-resign-unconditional-divestiture-contract.json`; the focused
final-federate resignation-cleanup contract lane passed 2/2 in
`.build\cpp-tck-all\focused-final-federate-resignation-cleanup-contract.json`; the focused
negotiated divestiture cancellation contract lane passed 2/2 in
`.build\cpp-tck-all\focused-negotiated-divestiture-cancellation-contract.json`; the focused
pre-delivery negotiated cancellation contract lane passed 2/2 in
`.build\cpp-tck-all\focused-negotiated-divestiture-pre-delivery-cancellation-contract.json`; the focused
update-rate query contract lane passed 2/2 in
`.build\cpp-tck-all\focused-update-rate-queries-contract.json`; the focused handle
wire-format contract lane passed 2/2 in
`.build\cpp-tck-all\focused-handle-wire-formats-contract.json`; the focused
federation-listing contract lane passed 2/2 in
`.build\cpp-tck-all\focused-federation-list-services-contract.json`; the focused
federate-lookup lifecycle contract lane passed 2/2 in
`.build\cpp-tck-all\focused-federate-lookup-lifecycle-contract.json`; the focused
order-control contract lane passed 2/2 in
`.build\cpp-tck-all\focused-order-type-controls-contract.json`; the focused
receive-order attribute-cancellation contract lane passed 2/2 in
`.build\cpp-tck-all\focused-receive-order-attribute-update-callback-cancellation-contract.json`;
the focused receive-order interaction-cancellation contract lane passed 2/2 in
`.build\cpp-tck-all\focused-receive-order-interaction-callback-cancellation-contract.json`;
the focused time-management/query contract lane passed 6/6 in
`.build\cpp-tck-all\focused-standard-time-query-contracts.json`;
it covers Next Message Request, available time advances at inclusive GALT, and
Query GALT/Query LITS boundaries;
the focused object/attribute request-response contract lane passed 6/6 in
`.build\cpp-tck-all\focused-standard-object-attribute-request-contracts.json`;
it covers object-instance request, object-class request, and ordinary response/
reflection boundaries;
the focused timestamped-interaction delivery contract lane passed 6/6 in
`.build\cpp-tck-all\focused-standard-timestamped-interaction-delivery-contracts.json`;
it covers mixed alternate advances, future-input Flush Queue delivery, and
timestamped-interaction retraction terminalization;
the focused timestamped-interaction ordering/fan-out contract lane passed 6/6 in
`.build\cpp-tck-all\focused-standard-timestamped-interaction-order-fanout-contracts.json`;
it covers cross-producer timestamp ordering, no-fan-out terminal retraction, and
delivered-versus-queued retraction fan-out;
the focused
directed interaction subscription-kind contract lane passed 2/2 in
`.build\cpp-tck-all\focused-directed-subscription-kind-contract.json`; the focused
terminal timestamped-deletion tombstone lane passed 2/2 in
`.build\cpp-tck-all\timestamped-object-deletion-tombstone-focused.json`; the focused
future-input Flush Queue lane passed 2/2 in
`.build\cpp-tck-all\interaction-future-focused.json`; the focused
TSO-designator terminalization lane passed 2/2 in
`.build\cpp-tck-all\interaction-tso-focused.json`; the focused
timestamped attribute Flush Queue future-input lane passed 2/2 in
`.build\cpp-tck-all\attribute-flush-focused.json`; the focused
inclusive-GALT lane passed 2/2 in
`.build\cpp-tck-all\inclusive-galt-focused.json`; the focused
timestamped service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-timestamped-focused.json`; the focused
regional service-report lane passed 2/2 in
`.build\cpp-tck-all\regional-service-report-focused.json`; the focused
regional service-report failure lane passed 2/2 in
`.build\cpp-tck-all\regional-service-report-failure-focused.json`; the focused
ordinary interaction service-report failure lane passed 2/2 in
`.build\cpp-tck-all\service-report-interaction-failure-focused.json`; the focused
ordinary attribute-update failure service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-attribute-update-failure-focused.json`; the focused
timestamped attribute-update failure service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-timestamped-attribute-update-failure-focused.json`; the focused
timestamped interaction failure service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-timestamped-interaction-failure-focused.json`; the focused
timestamped DeleteObjectInstance failure service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-timestamped-delete-object-instance-failure-focused.json`; the focused
ordinary register-object service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-register-object-instance-focused.json`; the focused
ordinary reserve-object-name service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-reserve-object-instance-name-focused.json`; the focused
ordinary request-attribute-value service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-request-attribute-value-update-focused-class-overload.json`; the focused
ordinary multiple-release-object-name service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-release-multiple-object-instance-names-focused.json`; the focused
ordinary release-object-name service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-release-object-instance-name-focused.json`; the focused
ordinary local-delete service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-local-delete-object-instance-focused.json`;
ordinary local-delete failure-report service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-local-delete-object-instance-failure-focused.json`;
the focused ordinary attribute-value response lane passed 2/2 in
`.build\cpp-tck-all\attribute-value-update-response-focused.json`;
the focused ordinary object-instance attribute-value request lane passed 2/2 in
`.build\cpp-tck-all\attribute-value-update-request-baseline-focused.json`;
the focused object-class attribute-value request lane passed 2/2 in
`.build\cpp-tck-all\object-class-attribute-value-update-request-baseline-focused.json`;
the focused regional attribute-value response recheck lane passed 2/2 in
`.build\cpp-tck-all\regional-attribute-value-update-response-recheck-focused.json`;
the focused ordinary delete-object service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-delete-object-instance-focused.json`; the focused
ordinary delete-object failure-report service-report lane passed 2/2 in
`.build\cpp-tck-all\service-report-delete-object-instance-failure-focused.json`; the focused
ordinary timestamped attribute alternate-advance lane passed 2/2 in
`.build\cpp-tck-all\attribute-alternate-focused.json`; the focused timestamped
attribute update-rate reduction lane passed 2/2 in
`.build\cpp-tck-all\timestamped-attribute-update-rate-reduction-focused.json`; the current promoted
direct lane passed 400/400 in
`.build\cpp-tck-all\verified-evidence-adapter-owned.json`; the installed-package
aggregate wrapper remains a separate 1/1 CTest check. The focused
standard order-and-transportation lookup slice passed 2/2 in
`.build\cpp-tck-standard-support\standard-support-focused.json`, with the
native order and transportation lookup oracles passing 20 and 37 assertions.
The focused empty-FOM validation slice passed 2/2 in
`.build\cpp-tck-fom-empty\fom-empty-focused.json`, with the native
FOM declaration-management oracle passing 58 assertions.
The focused negotiated
partial-acquisition cancellation lane is 2/2 passed in
`.build\cpp-tck-all\negotiated-partial-focused.json`. The focused ownership-
acquisition cancellation transfer race lane has one evoked pass and one
explicit immediate-model skip in
`.build\cpp-tck-all\ownership-cancellation-race-focused-final.json`. The focused divestiture-if-wanted-mixed-acquirers
lane is 2/2 passed in
`cpp-tck-all\divestiture-if-wanted-focused.json`. The focused resign-cancel-negotiated-pending
lane is 2/2 passed in
`cpp-tck-all\resign-cancel-negotiated-focused.json`. The focused resign-cancel-if-available-pending
lane is 2/2 passed in
`cpp-tck-all\resign-if-available-focused.json`. The focused resign-cancel-pending-acquisition
lane is 2/2 passed in
`cpp-tck-all\resign-cancel-focused.json`. The focused timestamped directed
Delay Subscription Evaluation lane is 2/2 passed in
`cpp-tck-all\delay-subscription-tso-directed-focused.json`. The focused directed
Delay Subscription Evaluation lane is 2/2 passed in
`cpp-tck-all\delay-subscription-directed-focused.json`. The focused final-federate
resignation-cleanup lane is 2/2 passed in
`cpp-tck-all\final-federate-resignation-focused.json`. The focused pending-acquisition
rejection contract lane passed 2/2 in
`.build\cpp-tck-all\focused-resign-pending-acquisition-rejection-contract.json`;
pending-acquisition cancellation contract lane passed 2/2 in
`.build\cpp-tck-all\focused-resign-cancel-pending-acquisition-contract.json`;
If Available cancellation contract lane passed 2/2 in
`.build\cpp-tck-all\focused-resign-cancel-if-available-pending-contract.json`;
negotiated cancellation contract lane passed 2/2 in
`.build\cpp-tck-all\focused-resign-cancel-negotiated-pending-contract.json`. The focused pre-delivery
negotiated-divestiture lane is 2/2 passed in
`cpp-tck-all\negotiated-pre-delivery-focused.json`. The focused late-join
synchronization lane is 2/2 passed in
`cpp-tck-all\synchronization-late-join-focused.json`. The focused regional-publication
lane is 2/2 passed in
`cpp-tck-all\regional-unpublish-focused.json`. The focused unnamed-join overload
lane is 2/2 passed in
`cpp-tck-all\p187-unnamed-join-focused-results.json`. The focused federation-list
lane is 2/2 passed in
`cpp-tck-all\p140-federation-list-services-focused-results.json`, and the
focused federate-lookup lifecycle lane is 2/2 passed in
`cpp-tck-all\p142-federate-lookup-lifecycle-focused-results.json`. The
focused object-name reservation lane is 2/2 passed in
`cpp-tck-all\p145-object-name-reservation-lifecycle-focused-results.json`.
The focused object-registration/discovery lane is 2/2 passed in
`cpp-tck-all\p148-object-registration-discovery-lifecycle-focused-results.json`,
the focused Allow Relaxed DDM lane is 2/2 passed in
`cpp-tck-all\p151-allow-relaxed-ddm-focused-results.json`, and the
focused multi-attribute regional object lane is 2/2 passed in
`cpp-tck-all\p154-regional-multi-attribute-update-focused-results.json`.
The focused three-dimensional regional object lane is 2/2 passed in
`cpp-tck-all\p159-regional-three-dimensional-overlap-focused-results.json`,
the focused default-region regional object lane is 2/2 passed in
`cpp-tck-all\p162-default-region-object-routing.json`, and the
focused passive-regional subscription lane is 2/2 passed in
`cpp-tck-all\p165-passive-regional-subscription.json`. The focused Auto Provide
lane is 2/2 passed in `cpp-tck-all\p168-auto-provide-focused-results.json`, and
the focused regional Request Attribute Value Update filtering lane is 2/2
passed in `cpp-tck-all\p171-regional-request-filtering-focused-results.json`.
The focused regional interaction subscription filtering lane is 2/2 passed in
`cpp-tck-all\p174-regional-interaction-filtering-focused-results.json`. The
focused resign-time deletion lane is 2/2 passed in
`cpp-tck-all\p177-resign-delete-focused-results.json`. The
focused unconditional-divestiture lane is 2/2 passed in
`cpp-tck-all\p182-resign-divest-focused-results.json`. The promoted-only direct
lane is 198/198 passed in
`cpp-tck-all\final-federate-promoted.json`. The promoted-only CTest
lane also passed 198/198 cases in
`.build\cpp-tck-final-federate-promoted`. The focused
regional-publication lane is 2/2 passed in
`cpp-tck-all\regional-unpublish-focused.json`. The
focused delay-subscription
ordinary attribute-update lane is 2/2 passed in
`cpp-tck-all\p135-delay-subscription-evaluation-attribute-update-focused-results.json`,
the focused delay-subscription timestamped interaction lane is 2/2 passed in
`cpp-tck-all\p136-delay-subscription-evaluation-timestamped-interaction-focused-results.json`,
and the focused delay-subscription timestamped attribute-update lane is 2/2
passed in
`cpp-tck-all\p137-delay-subscription-evaluation-timestamped-attribute-update-focused-results.json`.
The focused timestamped object-deletion
mixed-advance lane is 2/2 passed in
`cpp-tck-all\p129-timestamped-object-deletion-mixed-advances-focused-results.json`,
and the focused timestamped interaction retraction fan-out lane is 2/2 passed
in `cpp-tck-all\p131-timestamped-interaction-retraction-fanout-focused-results.json`.
The focused ordinary interaction Delay Subscription Evaluation lane is 2/2
passed in
`cpp-tck-all\p133-delay-subscription-evaluation-interaction-focused-results.json`.
The focused timestamped interaction mixed-advance
lane is 2/2 passed in
`cpp-tck-all\p126-timestamped-interaction-mixed-advances-focused-results.json`,
and the focused cross-producer timestamp-order lane is 2/2 passed in
`cpp-tck-all\p127-timestamped-interaction-cross-producer-order-focused-results.json`.
The focused timestamped attribute order/fan-out contract lane passed 6/6 in
`.build\cpp-tck-all\focused-standard-timestamped-attribute-order-fanout-contracts.json`;
it covers equal-timestamp ordering, queued multi-attribute passel retraction,
and no-recipient terminal retraction.
The focused timestamped attribute advance/re-enable contract lane passed 6/6 in
`.build\cpp-tck-all\focused-standard-timestamped-attribute-advance-reenable-contracts.json`;
it covers alternate advance servicing, future-input Flush Queue delivery, and
Time Constrained re-enable.
The focused timestamped regional-interaction
source-resignation lane is 2/2 passed in
`cpp-tck-all\p100-timestamped-regional-interaction-source-resignation-focused-results.json`,
and the focused TAR/NMR lane is 2/2 passed in
`cpp-tck-all\p101-timestamped-regional-interaction-tar-nmr-focused-results.json`.
The focused timestamped regional-attribute source-resignation lane is 2/2
passed in
`cpp-tck-all\p103-timestamped-regional-attribute-source-resignation-focused-results.json`.
The focused timestamped default-region interaction lane is 2/2 passed in
`cpp-tck-all\p104-timestamped-default-region-interaction-focused-results.json`,
and the focused alternate-advance lane is 2/2 passed in
`cpp-tck-all\p105-timestamped-default-region-interaction-alternate-focused-results.json`.
The focused default-region interaction source-resignation lane is 2/2 passed in
`cpp-tck-all\p108-timestamped-default-region-interaction-source-resignation-focused-results.json`.
The focused default-region interaction Time Constrained re-enable lane is 2/2
passed in
`cpp-tck-all\p111-timestamped-default-region-interaction-reenable-focused-results.json`.
The focused default-region interaction Time Regulation re-enable lane is 2/2
passed in
`cpp-tck-all\p112-timestamped-default-region-interaction-regulation-reenable-focused-results.json`.
The focused default-region interaction mixed-fanout lane is 2/2 passed in
`cpp-tck-all\p114-timestamped-default-region-interaction-mixed-fanout-focused-results.json`.
The focused default-region attribute mixed-fanout lane is 2/2 passed in
`cpp-tck-all\p116-timestamped-default-region-attribute-mixed-fanout-focused-results.json`.
The focused ordinary timestamped interaction source-resignation fan-out lane is
2/2 passed in
`cpp-tck-all\p118-timestamped-interaction-source-resignation-fanout-focused-results.json`.
The focused ordinary timestamped interaction source-resignation lane is 2/2
passed in
`cpp-tck-all\p120-timestamped-interaction-source-resignation-focused-results.json`.
The focused timestamped directed-interaction source-resignation lane is 2/2
passed in
`cpp-tck-all\p122-timestamped-directed-interaction-source-resignation-focused-results.json`.
The focused immediate timestamped directed-interaction source-resignation lane
is 2/2 passed in
`cpp-tck-all\directed-immediate-source-resignation-focused.json`.
The focused Query LITS source-resignation lane is 2/2 passed in
`cpp-tck-all\p124-query-lits-source-resignation-focused-results.json`.
The focused timestamped regional-interaction
subscription-replacement lane is 2/2 passed in
`cpp-tck-all\p98-timestamped-regional-interaction-subscription-replacement-focused-results.json`.
The focused timestamped regional-interaction
no-overlap lane is 2/2 passed in
`cpp-tck-all\p96-timestamped-regional-interaction-no-overlap-focused-results.json`.
The focused timestamped regional-interaction
alternate-advance lane is 2/2 passed in
`cpp-tck-all\p93-timestamped-regional-interaction-alternate-focused-results.json`.
The focused timestamped regional-interaction
regulation re-enable lane is 2/2 passed in
`cpp-tck-all\p91-timestamped-regional-interaction-regulation-reenable-focused-results.json`.
The focused timestamped directed-interaction
regulation re-enable lane is 2/2 passed in
`cpp-tck-all\p89-timestamped-directed-interaction-regulation-reenable-focused-results.json`.
The focused timestamped object-deletion
regulation re-enable lane is 2/2 passed in
`cpp-tck-all\p87-timestamped-object-deletion-regulation-reenable-focused-results.json`.
The focused timestamped interaction
regulation re-enable lane is 2/2 passed in
`cpp-tck-all\p85-timestamped-interaction-regulation-reenable-focused-results.json`. The focused timestamped regional regulation
re-enable lane is 2/2 passed in
`cpp-tck-all\p83-timestamped-regional-regulation-reenable-focused-results.json`. The focused timestamped default-region
regulation re-enable lane is 2/2 passed in
`cpp-tck-all\p81-timestamped-default-region-regulation-reenable-focused-results.json`. The focused timestamped default-region
re-enable lane is 2/2 passed in
`cpp-tck-all\p79-timestamped-default-region-reenable-focused-results.json`. The focused timestamped default-region alternate-advance
lane is 2/2 passed in
`cpp-tck-all\p77-timestamped-default-region-alternate-focused-results.json`. The focused timestamped regional association-replacement
lane is 2/2 passed in
`cpp-tck-all\p75-timestamped-regional-association-focused-results.json`. The focused timestamped regional alternate-advance
lane is 2/2 passed in
`cpp-tck-all\p73-timestamped-regional-alternate-focused-results.json`. The focused timestamped regional attribute
lane is 2/2 passed in
`cpp-tck-all\p71-timestamped-regional-attribute-focused-results.json`. The focused ownership-transfer lane is
2/2 passed in `cpp-tck-all\p68-ownership-transfer-regional-update-focused-results.json`.
The focused lifecycle/lookahead lane is 6/6 passed in
`cpp-tck-all\p38-focused-results.json`; the focused DDM region-lifecycle lane
is 2/2 passed in `cpp-tck-all\p40-region-focused-results.json`. Focused
synchronization, ownership, object-name reservation, support-lookup,
automatic-resign, support-switch, save/restore, regional-service,
timestamped-regional-send, and timestamped-directed-alternate lanes are green in
`cpp-tck-all\p42-synchronization-focused-results.json`,
`cpp-tck-all\p45-ownership-focused-results.json`,
`cpp-tck-all\p47-object-focused-results.json`,
`cpp-tck-all\p49-support-focused-results.json`,
`cpp-tck-all\p50-federation-focused-results.json`, and
`cpp-tck-all\p51-relevance-focused-results.json`,
`cpp-tck-all\p57-save-restore-focused-results.json`,
`cpp-tck-all\p59-region-service-focused-results.json`,
`cpp-tck-all\p61-region-timestamped-focused-results.json`, and
`cpp-tck-all\p62-timestamped-directed-alternate-focused-results.json`, and
`cpp-tck-all\p66-save-restore-interlocks-focused-results.json` respectively.
The promoted-only direct evidence passes the strict validator with
`--promotion promoted`; the latest adapter-owned artifact is recorded in
`cpp-tck-all\verified-evidence-adapter-owned.json` and contains 400/400
promoted cases under both callback models. Its ordinary CTest matrix contains
398/398 passed cases, with the adapter-owned connection-loss pair run by the
Python fixture harness. The earlier pre-contract candidate-inclusive adapter-owned
direct matrix covered 410 cases with 405 passes and five explicit skips. The focused
custom-transportation regional attribute lane passed 2/2
in `cpp-tck-custom-transportation-regional\custom-transportation-regional-focused.json`,
and the focused regional interaction lane passed 2/2 in the same artifact; the
matching native oracles passed 51 and 40 assertions. The focused custom-transportation interaction lane passed 2/2
in `cpp-tck-custom-transportation\custom-transportation-focused.json`, and
the matching native oracle passed 42 assertions. The focused custom-transportation
timestamped-delivery lane passed 2/2 in
`cpp-tck-custom-transportation-timestamped\custom-transportation-timestamped-focused.json`,
and the matching native oracle passed 48 assertions. The focused custom-transportation
timestamped-directed-delivery lane passed 2/2 in
`cpp-tck-custom-transportation-timestamped-directed\custom-transportation-timestamped-directed-focused.json`,
and the matching native oracle passed 42 assertions. The focused custom-transportation
timestamped-regional-attribute lane passed 2/2 in
`cpp-tck-custom-transportation-timestamped-regional\custom-transportation-timestamped-regional-focused.json`,
and the matching native oracle passed 60 assertions. The latest MOM periodic focused artifact is
`cpp-tck-all\joined-federate-mom-periodic-focused.json` with 4/4 passed cases.
The focused negotiated-divestiture partial
cancellation lane is recorded in
`cpp-tck-all\negotiated-partial-focused.json` with 2/2 passed cases. The focused negotiated-divestiture
cancellation lane is recorded in
`cpp-tck-all\negotiated-divestiture-focused.json` with 2/2 passed cases, and
the focused pending-acquisition resignation lane is recorded in
`cpp-tck-all\resign-pending-focused.json` with 2/2 passed cases. The
focused pre-delivery negotiated-divestiture cancellation lane is recorded in
`cpp-tck-all\negotiated-pre-delivery-focused.json` with 2/2 passed cases. The
focused Willing-to-Acquire continuation lane is recorded in
`cpp-tck-all\negotiated-wta-continuation-focused.json` with one evoked case
passed and the immediate case explicitly skipped. The
focused late-join synchronization lane is recorded in
`cpp-tck-all\synchronization-late-join-focused.json` with 2/2 passed cases. The
candidate-inclusive direct artifact is
`cpp-tck-all\all-candidate-evidence-with-connection-loss.json`; it contains 678
passed cases and the ten explicitly skipped candidate immediate cases in the
688-case all-scenario matrix. The no-fixture comparison artifact remains
`cpp-tck-all\all-candidate-evidence-standard-timed-ownership-contracts-final.json`.

The catalog separates promotion from availability: `promotion=promoted` is the
verified baseline, while `default_status` records whether a scenario is
available immediately, requires adapter capabilities, or is unsupported. A
new scenario should remain `promotion=candidate` until it has passed the
source-boundary check and every callback model applicable to its standard
semantics in an adapter run.

The catalog is
[`compliance/catalogs/cpp-tck-scenario-catalog.json`](../../compliance/catalogs/cpp-tck-scenario-catalog.json).
Each entry maps a runner ID to the standard API methods, a source symbol, and
the pinned Requirements Lab contracts.
It also instantiates the standard `NullFederateAmbassador` and invokes every
callback overload once, providing a provider-independent compile and no-op
dispatch check for the complete callback surface.
The standard `Authorizer` and `AuthorizerFactory` interfaces are also exercised
through local implementations, keeping the auth-extension check provider- and
FOM-independent.
The official `EncoderException` contract and abstract `DataElement` clone,
same-type, encoding, boundary, hash, and decode operations are exercised with a
local standard-library-only element, keeping the encoding-foundation check
provider- and FOM-independent.
The abstract `LogicalTime`, `LogicalTimeInterval`, and `LogicalTimeFactory`
interfaces are likewise exercised through local implementations, including
boundary values, arithmetic, comparisons, difference, both encoding overloads,
both decoding overloads, diagnostics, and standard error boundaries.
The same standard-value inventory checks `VariableLengthData` empty construction,
copy independence, borrowed storage, custom-deleter adoption, and the default
array-deleter `takeDataPointer` overload without provider or FOM dependencies.
The promoted `cpp-tck.variable-length-data-contract` runner exposes this value
contract as an independently selectable slice, so it can be verified without a
provider connection or FOM load.
The promoted `cpp-tck.logical-time-contract` runner similarly exposes the
concrete standard logical-time value and factory contract as an independently
selectable, provider- and FOM-independent slice.
The promoted `cpp-tck.exception-hierarchy-contract` runner exposes the complete
official C++ exception hierarchy as an independently selectable,
provider- and FOM-independent slice.
The promoted `cpp-tck.enum-contract` runner exposes the official C++
enumeration families as an independently selectable, provider- and
FOM-independent slice.
The promoted `cpp-tck.handle-and-collection-contract` runner exposes the
official C++ handle, range, map, set, pair-vector, and federation/restore-vector
value contract as an independently selectable, provider- and FOM-independent
slice.
The promoted `cpp-tck.configuration-and-authorization-contract` runner
exposes the official C++ configuration, federation-record, credential,
authorization, and `Authorizer`/`AuthorizerFactory` contract as an independently
selectable, provider- and FOM-independent slice.
The promoted `cpp-tck.authorizer-factory-factory-contract` runner exposes the
official C++ `HLAauthorizerFactoryFactory` selection, naming, creation, and
unsupported-name contract as an independently selectable, provider- and
FOM-independent slice.
The promoted `cpp-tck.runtime-identity-contract` runner exposes the official
C++ `rtiName()`/`rtiVersion()` callability, non-empty-value, and process-stability
contract as an independently selectable, provider-neutral slice.
The promoted `cpp-tck.rti-ambassador-factory-contract` runner exposes the
official C++ `RTIambassadorFactory` construction and repeatable ambassador
creation contract as an independently selectable, provider- and FOM-independent
slice.
The promoted `cpp-tck.logical-time-factory-factory-contract` runner exposes the
official logical-time factory-factory default and integer selection, reference-
factory forwarding, unknown-name rejection, and initial-value construction as an
independently selectable, provider- and FOM-independent slice.
The promoted `cpp-tck.logical-time-data-elements-contract` runner exposes the
standard `HLAlogicalTime` and `HLAlogicalTimeInterval` DataElement wrapper
round-trip, nested-buffer, clone/copy, type-compatibility, boundary, and
truncation contract as an independently selectable adapter-backed slice.
The promoted `cpp-tck.connection-callback-contract` runner exposes the standard
connection and callback-control surface as an independently selectable,
adapter-backed slice.
The promoted `cpp-tck.null-federate-ambassador-contract` runner exposes the
official C++ `NullFederateAmbassador` callback and overload contract as an
independently selectable, provider- and FOM-independent slice.
The promoted `cpp-tck.data-element-contract` runner exposes the official
C++ `DataElement` base clone, type, encoding, boundary, hash, and decode
contract as an independently selectable, provider- and FOM-independent
slice.
