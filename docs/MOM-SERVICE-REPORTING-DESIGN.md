# MOM Service Reporting Design

## Status

This is an embedded-profile design with a bounded filesystem and an incrementally
public joined-federate-MOM foundation. Umbra now creates a real per-joined-federate
report file and its Table 5 initial record, and appends source-backed
successful-void records for seven Support Services when that file sink is
selected: six Boolean setters (the four relevance/scope advisory setters,
`Set Convey Region Designator Sets Switch`, and `Set Exception Reporting
Switch`) plus `Set Automatic Resign Directive`. The same sink also records five
no-argument Time Management services—`Disable Time Regulation`,
`Enable/Disable Asynchronous Delivery`, and `Enable/Disable Time
Constrained`—plus `Enable Time Regulation` with its one `Lookahead` argument
and `Modify Lookahead` with its one `Requested lookahead` argument. Both use
Table 5's `LogicalTimeInterval` type 32 and quoted `interval.toString()` form.
`Modify Lookahead` is recorded on acceptance even when a lower actual value
remains deferred. `Time Advance Request`, `Time Advance Request Available`,
`Next Message Request`, `Next Message Request Available`, and `Flush Queue
Request` each record one `Logical time` argument with Table 5's `LogicalTime`
type 31 and quoted `time.toString()` form on acceptance. The first four records
precede their later Time Advance Grant callbacks; the Flush Queue record
precedes its distinct Flush Queue Grant, which carries separately selected
actual and optimistic times. The two Next Message Request forms and Flush Queue
Request retain their supplied boundaries even when queued TSO input produces an
earlier grant. `Retract` records its one `MessageRetractionDesignator` with
Table 5's `MessageRetractionHandle` type 33 and
`MessageRetractionHandle<decimal-identity>` text before its separately
callback-gated Request Retraction consequence. `Synchronization Point Achieved`
records its type-53 `Synchronization point label` and type-6 `Optional
synchronization-success indicator` after the accepted §4.17 achievement. The
defaulted C++ Boolean records its effective `true` value; an explicit failure
records `false`. Those direct records precede the separately queued Federation
Synchronized callbacks. The RTI-created §4.18 recipient reports then append
their type-53 label and type-18 failed-federate `FederateHandleSet` before the
corresponding callback is queued. `Register Federation Synchronization Point` records
the accepted §4.14 invocation, then the unified §4.15 confirmation record at
the registering federate before it dispatches either C++ registration-result
callback. Its type-53 `Synchronization point label` and Table 5 type-63
base-64 `User-supplied tag` are followed by the required third slot: the
two-argument C++ overload records type-34 Null for the omitted `Optional set
of joined federate designators`, while the explicit set overload records a
type-18 `FederateHandleSet` array of quoted exact
`FederateHandle::toString()` values (including `[]` for a supplied-empty set).
The RTI-originated confirmation uses the label, type-6 `Registration-success
indicator`, and type-34 Null or type-56 `SynchronizationPointFailureReason`
`Optional failure reason`. §4.16 announcements now use a weak private
per-joined-federate report endpoint: the recipient's selected file receives
the type-53 label and type-63 user tag record before its callback is queued.
The endpoint is attached at Join, so disabling or re-enabling either
report-selection switch only gates appends; it neither replaces the file nor
changes its serial sequence. The Table 5 type-63 literal remains private file text under RL-077. `Request
Federation Save` records its accepted §4.19 invocation before any separately
queued Initiate Federate Save work. Its two public C++ overloads preserve the
same required type-53 `Federation save label` and distinguish the required
optional-slot representation: the untimed overload writes type-34 Null for
`Optional timestamp`, while the timestamped overload writes type-31
`LogicalTime` using the RTI-owned clone's quoted `toString()` value. The
ordinary queued §4.20 recipients now use the same private
per-joined-federate route: each selected file receives its own
`InitiateFederateSave` record with the type-53 label and type-34 Null or
type-31 LogicalTime timestamp before its callback can be evoked. The direct
time-constrained pre-grant path remains a separate reporting lane because it
has re-entrant Time Advancing ordering requirements. The Requirements Lab
currently attaches the relevant page-67 source candidates to §4.21.1; RL-082
records the corrected §4.20 association. The RTI-initiated §4.23
`FederationSaved` record follows the same selected-recipient route: normal
completion writes type-6 `Federation save-success indicator` true plus type-34
Null `Optional failure reason`, while a failed result writes false plus the
type-48 quoted `SaveFailureReason`, before the corresponding result callback
is queued. The focused filesystem regression covers normal completion and the
ordinary `SAVE_ABORTED` result; the Lab's §4.23 candidates retain the
coalesced-page ownership defect recorded under RL-078. `Request Federation Restore` records
its normally returned §4.27 call with one type-53
`Federation save label` before the distinct §4.28 Confirm Federation
Restoration Request record reaches that same requester. The confirmation
retains the type-53 label plus a type-6 `Request-success indicator` Boolean
for both the positive and negative C++ callback forms, and is durable before
either callback can be evoked. The RTI-initiated §4.29 `FederationRestoreBegun`
record then uses each joined recipient's selected file, including the requester,
with the Table 5 successful-void no-argument form before its callback can be
evoked. Its source candidates retain the coalesced-page ownership defect under
RL-078. The following §4.30 `InitiateFederateRestore` record uses that same
recipient-local file and serial sequence, preserving the type-53 `Federation
save label`, type-15 `Joined federate designator`, and type-53 `Federate name`
before `initiateFederateRestore()` can be evoked. `Federate Restore Complete` records §4.31's required type-6 `Federate restore-success
indicator`: `federateRestoreComplete()` selects `true`, and
`federateRestoreNotComplete()` selects `false`, while both use the one
`FederateRestoreComplete` service name before their respective Federation
Restored or Federation Not Restored callback. The per-joined-federate report
serial remains live audit state across a federation application snapshot
restore, so its durable file never reuses a record number. `Abort Federation
Restore` records its accepted §4.33 no-argument request with the Table 5
successful-void `[]` supplied-argument and `[null]` returned-argument forms
before the ordinary `RESTORE_ABORTED` Federation Not Restored callback. A
rejected pre-restore abort appends nothing; the standard's all-members-already-
complete success case remains outside this focused slice. `Query Federation
Restore Status` records its accepted §4.34 no-argument request before the
separate Federation Restore Status Response callback supplies the descriptor
vector. The §4.35 response itself then appends the official-MIM type-20
`FederateRestoreStatusSet` to that querying recipient's selected file before
its callback: each record retains public pre/post `FederateHandle` text and
the quoted `RestoreStatus` spelling. The type name and malformed second Table
5 example are reconciled under RL-083. A `SaveInProgress` rejection appends no
query record. `Resign Federation
Execution` records one accepted §4.12 action as the final record of the
joined-federate lifetime. Its type-44 `HLAresignAction` uses the exact standard
MIM parameter spelling for Umbra's implementation-defined descriptive name;
its value remains the official Table 5 `ResignAction` enum spelling. Because
resignation removes the joined member, the registry reserves the file serial
only after every ordinary rejection path has cleared and immediately before
the member is erased. The adapter appends that reserved record before releasing
the writer; a selected-file append failure still completes the local teardown
and then surfaces the deterministic `RTIinternalError`, with no memory fallback.
The distinct RTI-initiated §4.13 `Federate Resigned` callback records its one
type-53 `Reason for resigning` argument as the final report record before its
callback is queued. It uses the same successful-membership-removal reservation
mechanism but retains the connection; the private test seam proves the durable
record precedes callback delivery and that repeated control requests cannot
append. The separate RTI-initiated §4.4 `Connection Lost` service records its
own type-53 `Fault description` argument as the final report record before the
best-effort callback is queued. The embedded fault seam reserves the serial
while the lost member still owns its report writer, releases that completed
writer after the record, and cannot append again for a duplicate fault. It is
not an alternate spelling or consequence of `Federate Resigned`; its
disconnected lifecycle and remote-fault delivery remain separately scoped.
`Federate Save
Begun` records its accepted §4.21 transition with the explicit Table 5
successful-void form: an empty supplied-argument list and `[null]` returned
argument. A rejected pre-initiation invocation appends nothing; later save
completion and Federation Saved remain separate service/callback boundaries.
`Federate Save Complete` records §4.22's one required type-6 `Federate
save-success indicator` argument. The official C++ binding splits its selector
into `federateSaveComplete()` (`true`) and `federateSaveNotComplete()`
(`false`), but both append the same `FederateSaveComplete` service record
after acceptance and before their respective Federation Saved or Federation Not
Saved callbacks. `Federation Saved` is a separate RTI-initiated §4.23 record:
the recipient file first receives type-6 true plus type-34 Null for normal
completion, or type-6 false plus type-48 `SaveFailureReason` for failure,
then the matching result callback can be evoked. The bounded file lane covers
normal completion and `SAVE_ABORTED`; its §4.23 source candidates retain the
coalesced-page ownership defect documented under RL-078. `Abort Federation Save` records its accepted §4.24 request
with the successful-void `[]` supplied-argument and `[null]`
returned-argument forms before the subsequent save-result callback. The
current focused regression covers the ordinary `SAVE_ABORTED` failure result;
the standard's all-members-already-complete success case remains outside this
slice. `Query Federation Save Status` records an accepted §4.25 request with
the same no-argument successful-void form before its distinct Federation Save
Status Response callback carries the member-status vector. That RTI-initiated
§4.26 response now appends its own selected-file record first: type-17
`FederateHandleSaveStatusPairSet` is a Table 5 array of `handle`/`status`
records with quoted public handle and `SaveStatus` spellings. The focused lane
proves the in-progress `FEDERATE_INSTRUCTED_TO_SAVE` form before HLA_EVOKED
delivery; RL-078 records the Lab's coalesced-page §4.26 ownership defect.
`Change Attribute
Order Type`
records type-37 `Object instance designator`, type-1 `Set of attribute
designators` as a bracketed array of quoted `AttributeHandle::toString()` values,
and type-38 `Order type` as quoted `RECEIVE` or `TIMESTAMP` after the owned
attribute precondition is satisfied. `Change Default Attribute Order Type`
records type-36 `Object class designator` as quoted
`ObjectClassHandle::toString()`, type-1 `Set of attribute designators` as the
same bracketed quoted-handle array, and type-38 `Order type` as quoted
`RECEIVE` or `TIMESTAMP` after a successful class-default invocation. `Change
Default Attribute Transportation Type` uses that same type-36 class-designator
and type-1 attribute-set form with type-59 `Transportation type` as quoted
`TransportationTypeHandle::toString()` text after an accepted prospective
class-default invocation. `Change Interaction Order Type`
records `Interaction class designator` as type 27 with the quoted exact
`InteractionClassHandle::toString()` value and `Order type` as type 38 with
quoted `RECEIVE` or `TIMESTAMP` after the class's publishing precondition is
satisfied. `Request Interaction Transportation Type Change` records its
type-27 `Interaction class designator` and type-59 `Transportation type` as
quoted exact handle `toString()` values when the request is accepted; its
separately queued confirmation remains the preference-change boundary. `Request
Attribute Transportation Type Change` records type-37 `Object instance
designator`, type-1 `Set of attribute designators`, and type-59 `Transportation
type` in the corresponding source-backed Table 5 forms before its confirmation
boundary. `Query Attribute Transportation Type` records type-37 `Object
instance designator` and type-0 `Attribute designator` as quoted
  `handle.toString()` text when its query plan is accepted; its separately queued
  `Report Attribute Transportation Type` callback remains the response boundary.
  `Query Attribute Ownership` records type-37 `Object instance designator` and
  type-1 `Set of attribute designators` as quoted `handle.toString()` text and
  a bracketed array of quoted `AttributeHandle::toString()` values when its
  query plan is accepted; its separately queued grouped federate-owned and
  unowned ownership-result callbacks remain the response boundary.
  The nonregional `Request Attribute Value Update` overloads record their
  accepted §6.21 invocation before any separately queued `Provide Attribute
  Value Update` callback: the instance form uses type-37 `Object instance
  designator`, the class form type-36 `Object class designator`, and both
  use type-1 `Set of attribute designators` plus the Table 5 type-63 base-64
  `User-supplied tag` form. Rejected requests append nothing. As with the
  ownership divestiture record, type 63 is a bounded file-text choice only
  (RL-077).
  The explicit §6.22 `Provide Attribute Value Update` callback retains the
  providing joined federate's selected-file route with its queued work. Once
  its callback-time projection succeeds, the provider's file receives the
  object-management successful-void record immediately before
  `provideAttributeValueUpdate()` enters user code: type-37 `Object instance
  designator`, type-1 `Set of attribute designators`, and type-63 base-64
  `User-supplied tag`. The focused filesystem proof is deliberately limited to
  the explicit object-instance request path; class, regional, and automatic
  provision use the same queue primitive but need their own report-specific
  regressions. A cancelled or stale plan writes nothing, and this remains
  private file text rather than public MOM interaction delivery.
  Auto Provide uses that same provider-side route after discovery, but its
  RTI-invoked §6.22 record carries a zero-length type-63 tag as required by
  §1.5. The focused filesystem proof checks that empty-tag record at the first
  provider callback instruction; it does not imply that the discovery callback
  or the Auto Provide switch setter has its own service-report record.
  The object-class §6.21 request form uses the same provider-side route after
  class expansion: each queued instance callback appends its own type-37
  object-instance, type-1 attribute-set, and propagated type-63 tag record
  immediately before user code. The focused two-instance proof checks serial
  ordering and per-callback file accumulation; it does not serialize the
  requester's class designator into the §6.22 callback record.
  The regional class-request form feeds that same class queue after its
  committed overlap predicate succeeds. Its provider callback report retains
  the concrete object instance, attribute set, and request tag; the focused
  overlap-qualified proof verifies the route and ordering without claiming
  region data belongs in the §6.22 supplied-argument list.
  The invoking §9.13 `Request Attribute Value Update With Regions` service now
  also records its accepted requester-local invocation before that provider
  queue can deliver a callback. Its private successful-void record uses the
  official type-36 `Object class designator`, type-4 `Collection of attribute
  designator set and region designator set pairs`, and copied type-63
  `User-supplied tag` arguments. The exact same report file remains selected
  for the joined federate, and the report/file switches gate this append. This
  requester-side record is deliberately distinct from the provider's later
  type-37/type-1/type-63 callback record; neither is a claim of public MOM
  interaction delivery or conformance.
  The nonregional, non-timestamped `Update Attribute Values` overload records
  its accepted §6.10 invocation before any separately queued `Reflect
  Attribute Values` callback. Its four supplied-argument slots use type-37
  `Object instance designator`, type-2 `Constrained set of attribute
  designator and value pairs` as Table 5's
  `PairList<AttributeHandle:BinaryData>`, type-63 base-64 `User-supplied tag`,
  and type-34 Null for the omitted `Optional timestamp`. Rejected updates
  append nothing. This is deliberately only the receive-order, nonregional
  overload: timestamped update reporting requires a separately source-backed
  return/retraction record before it can be admitted.
  The nonregional `Send Interaction` overloads record their
  accepted §6.12 invocation before any separately queued `Receive Interaction`
  callback. Its four supplied-argument slots use type-27 `Interaction class
  designator`, type-40 `Constrained set of interaction parameter designator
  and value pairs` as Table 5's `PairList<ParameterHandle:BinaryData>`,
  type-63 base-64 `User-supplied tag`, and type-34 Null for the omitted
  `Optional timestamp`; the timestamped overload supplies type-31 logical-time
  text and its type-33 message-retraction return argument after TSO admission.
  Rejected interactions append nothing. The region-context overloads add a
  type-43 `Set of region designators` argument before the user tag and use the
  same timestamp/retraction rules; `HLAsetSwitches`-specific reporting remains
  deferred until its MOM-control record form is source-backed.
  The DDM §9.10 `Subscribe Interaction Class With Regions` and §9.11
  `Unsubscribe Interaction Class With Regions` services now have the same
  accepted-transition file boundary. Subscribe records type-27 `Interaction
  class designator`, type-43 `Set of region designators`, and type-6
  `Optional passive subscription indicator` (the C++ `active` selector is
  inverted); Unsubscribe records type 27 and type 43. The focused lane gates
  these appends with the joined federate's switches and keeps setup/report
  switch mutations out of the serial sequence.
  The non-timestamped `Send Directed Interaction` overload records its
  accepted §6.14 invocation before any separately queued `Receive Directed
  Interaction` callback. Its five supplied-argument slots use type-27
  `Interaction class designator`, type-37 `Object instance designator`,
  type-40 `Constrained set of interaction parameter designator and value
  pairs` as Table 5's `PairList<ParameterHandle:BinaryData>`, type-63 base-64
  `User-supplied tag`, and type-34 Null for the omitted `Optional timestamp`.
  Rejected directed interactions append nothing. This deliberately excludes
  timestamped/retraction and directed DDM forms until their distinct return
  and region records are source-backed.
  The RTI-initiated §6.9 `Discover Object Instance` service retains the
  receiving joined federate's selected-file route with its pending discovery.
  At the callback-time eligibility recheck, and immediately before
  `discoverObjectInstance()` enters user code, it appends type-37 `Object
  instance handle`, type-36 `Object class designator`, type-53 `Object
  instance name`, and type-15 `Producing joined federate designator`. Thus an
  HLA_EVOKED plan does not create a report until it is actually delivered, and
  a cancelled/stale plan writes nothing. The selected-file form remains
  private text rather than public MOM interaction delivery.
  The nonregional, non-timestamped `Delete Object Instance` overload records
  its accepted §6.16 invocation after its local deletion transition and before
  any separately queued `Remove Object Instance` callback. Its three
  supplied-argument slots use type-37 `Object instance designator`, type-63
  base-64 `User-supplied tag`, and type-34 Null for the omitted `Optional
  timestamp`. Rejected deletions append nothing. This deliberately excludes
  the timestamped sender-invocation record and regional forms until their
  distinct return and region records are source-backed.
  The single-name §6.2 `Reserve Object Instance Name` and §6.4 `Release Object
  Instance Name` services now have adjacent successful-void report coverage.
  Each records its type-53 `Name` String after the accepted state transition;
  reservation writes before the asynchronous result callback is queued, while
  release is durable before the public call returns. Switch-gated and rejected
  calls append nothing. The §6.5 `Reserve Multiple Object Instance Names` and
  §6.7 `Release Multiple Object Instance Names` services now use Table 5's
  type-54 `StringSet`/`Array<String>` form, preserving the official C++ set
  order and the exact `Name Set`/`Name set` argument labels. Their accepted
  invocation records are written before reservation callbacks or return;
  switch-gated and atomically rejected calls append nothing. The four result
  callback report forms remain separate work because the multiple-name success
  callback is a composite set of names and success indicators without a
  reviewed Table 5 file-record mapping.
  The receive-order §6.17 `Remove Object Instance` callback carries its
  recipient-local selected-file route through the queued removal. Once its
  callback-time transition succeeds, it appends before
  `removeObjectInstance()` enters user code: type-37 `Object instance
  designator`, type-63 base-64 `User-supplied tag`, type-38 `RECEIVE` sent
  message order, type-15 `Producing joined federate designator`, and type-34
  Null for each absent optional timestamp, receive-order, and retraction slot.
  The bounded timestamped §6.17 callback forms retain the same route. A
  non-time-constrained recipient appends at its immediate callback-time
  transition with type-38 `TIMESTAMP` sent order, type-31 `Optional timestamp`,
  type-38 `RECEIVE` received order, and type-33 `Optional message retraction
  designator` when supplied (otherwise type-34 Null). The constrained TSO
  delivery boundary appends before user code with `TIMESTAMP` received order
  and its supplied retraction designator. These callback records do not imply
  a timestamped `Delete Object Instance` sender-invocation report.
  `Unconditional Attribute Ownership Divestiture` records those same type-37
  and type-1 forms plus its `User-supplied tag` as double-quoted base-64 Binary
  Data when its §7.2 plan is accepted, before any separately queued §7.4
  `Request Attribute Ownership Assumption` callback. Table 5 depicts the tag
  as type 63, while the bundled standard MIM enumerates `UserSuppliedTag` as
  type 60. Umbra follows the Table 5 literal only for the bounded file text and
  does not reuse it in a future emitted MOM-interaction path (RL-077).
  `Attribute Ownership Acquisition` records its accepted §7.8 request with
  type-37 `Object instance designator`, type-1 `Set of attribute designators`,
  and the type-63 base-64 `User-supplied tag` form before any separately
  queued `Request Attribute Ownership Release` or acquisition work. Rejected
  requests append nothing. The type-63 form remains a private file-text choice
  only (RL-077).
  `Attribute Ownership Acquisition If Available` records its accepted §7.9
  request with the same type-37, type-1, and type-63 forms before either its
  supplied-empty no-callback return or a separately queued `Attribute Ownership
  Acquisition Notification` / `Attribute Ownership Unavailable` callback.
  Rejected requests append nothing. This is only the existing nonregional
  requesting-federate path; the type-63 form remains private file text (RL-077).
  `Attribute Ownership Release Denied` records its accepted §7.12 invocation
  with type-37 `Object instance designator`, type-1 `Set of attribute
  designators for which the joined federate is unwilling to divest ownership`,
  and type-63 base-64 `User-supplied tag` before separately queued `Attribute
  Ownership Unavailable` callbacks. Rejected denials append nothing; the long
  source-defined attribute-set name is retained verbatim in the file record.
  `Confirm Divestiture` records its accepted §7.6 transfer with type-37 `Object
  instance designator`, type-1 `Set of attribute designators`, and type-63
  base-64 `User-supplied tag` before separately queued acquisition-notification
  work. Rejected confirmations append nothing; the tag form remains private
  file text (RL-077).
  `Negotiated Attribute Ownership Divestiture` records its accepted §7.3
  request with type-37 `Object instance designator`, type-1 `Set of attribute
  designators`, and type-63 base-64 `User-supplied tag` before separately
  queued `Request Divestiture Confirmation` work. Rejected requests append
  nothing; the type-63 form remains private file text (RL-077).
  `Cancel Negotiated Attribute Ownership Divestiture` records its accepted
  §7.14 cancellation with type-37 `Object instance designator` and type-1 `Set
  of attribute designators` before separately queued ordinary `Request
  Attribute Ownership Release` work is restored. Rejected cancellations append
  nothing. The source-driven runtime behavior is independent of the generated
  state-chart inconsistency recorded in RL-017.
  `Attribute Ownership Divestiture If Wanted` remains intentionally excluded
  from this successful-void inventory. Its §7.13.2 return is the source-named
  set of attributes actually divested, while Table 5 leaves the non-void
  `ReturnArgument` file representation undefined. RL-042 therefore blocks a
  report-specific formatter until an authoritative record shape is available;
  Umbra will not substitute a local generic return convention.
  `Cancel Attribute Ownership Acquisition` records its accepted §7.15
  invocation with type-37 `Object instance designator` and type-1 `Set of
  attribute designators` before either its supplied-empty no-callback return
  or a separately queued `Confirm Attribute Ownership Acquisition
  Cancellation` callback. Rejected cancellations append nothing. This is only
  the existing nonregional, still-pending regular-acquisition path; in-flight
  cancellation races remain outside the report wrapper.
  `Local Delete Object Instance` records its accepted §6.18 local-forget
  transition with type-37 `Object instance designator` as quoted exact
  `ObjectInstanceHandle::toString()` text. A rejected call appends nothing, and
  the local state transition has no separate callback boundary.
  `Publish Object Class Attributes` records its accepted §5.2 publication
  transition with type-36 `Object class designator` as quoted exact
  `ObjectClassHandle::toString()` text and type-1 `Set of attribute
  designators` as a bracketed array of quoted `AttributeHandle::toString()`
  values. A rejected class adds no record; the successful record precedes
  separately queued declaration advisories and ownership-assumption work. The
  Requirements Lab currently locates the coalesced source under §5.2.4 rather
  than §5.2 (RL-078).
  The four RTI-initiated ordinary declaration advisories use the same
  recipient-local route: §5.14 `StartRegistrationForObjectClass` and §5.15
  `StopRegistrationForObjectClass` append type-36 `Object class designator`,
  while §5.16 `TurnInteractionsOn` and §5.17 `TurnInteractionsOff` append
  type-27 `Interaction class designator`. Each selected-file record is durable
  before its HLA_EVOKED callback is queued; it remains private file text, not a
  claim of public MOM interaction delivery. The Lab's page-continuation and
  stale-title limits remain tracked under RL-080/RL-002.
  `Unpublish Object Class Attributes` records its accepted §5.3 teardown with
  type-36 `Object class designator` as quoted exact
  `ObjectClassHandle::toString()` text and type-1 `Optional set of attribute
  designators` as a bracketed array of quoted `AttributeHandle::toString()`
  values. A rejected class appends no record; the successful record follows the
  registry's synchronous ownership cleanup and precedes separately queued
  declaration advisories. The Requirements Lab currently locates the coalesced
  source under §5.3.3 rather than §5.3 (RL-078).
  `Subscribe Object Class Attributes` records its accepted §5.8 subscription
  with type-36 `Object class designator`, type-1 `Set of attribute designators`,
  type-6 `Optional passive subscription indicator`, and type-53 `Optional
  update rate designator`. The public C++ `active` selector is inverted for the
  standards-facing passive indicator; the empty/default update-rate selector
  instead occupies its argument position as type-34 Null. The record precedes
  separately queued declaration, scope, relevance, and discovery callbacks.
  The Lab's cross-page update-rate candidates are incorrectly owned by §5.9;
  RL-080 records that mismatch.
  `Unsubscribe Object Class Attributes` records its accepted §5.9 declaration
  transition with type-36 `Object class designator`. The official C++
  attribute-set overload supplies a type-1 `Optional set of attribute
  designators`, while its whole-class overload leaves that same Table 5
  position as type-34 Null; an empty supplied set remains distinct from Null.
  A rejected class appends no record, and either accepted record precedes
  separately queued declaration, scope, and relevance callbacks. The Lab owns
  the coalesced source candidates as §5.9.4 rather than §5.9 (RL-078).
  `Publish Object Class Directed Interactions` records its accepted §5.6
  declaration with type-36 `Object class designator` and type-28 `Set of
  interaction class designators`. The latter is Table 5's
  `Array<InteractionClassHandle>` form: a bracketed list of quoted exact
  `InteractionClassHandle::toString()` values. An empty supplied set remains a
  successful no-op declaration and is recorded as `[]`, while rejected object
  or interaction class designators append no record. The Lab owns the
  coalesced source candidates as §5.6.3 rather than §5.6 (RL-078).
  `Unpublish Object Class Directed Interactions` records its accepted §5.7
  declaration with type-36 `Object class designator` and an Optional set of
  interaction class designators. The official C++ interaction-set overload
  supplies type-28 `InteractionClassHandleSet` using the Table 5
  `Array<InteractionClassHandle>` form, including `[]` for a supplied-empty
  set. The whole-class overload leaves that same position as type-34 Null. A
  rejected object or interaction class appends no record; the Lab owns the
  coalesced source candidates as §5.7.4 rather than §5.7 (RL-078).
  `Subscribe Object Class Directed Interactions` records its accepted §5.12
  declaration with type-36 `Object class designator`, type-28 `Set of directed
  interaction designators`, and type-6 `Optional universal subscription
  indicator`. The official C++ binding makes that optional selector a defaulted
  Boolean, so the report records its effective value: default by-ownership is
  lowercase `false` and an explicit universal selection is lowercase `true`.
  An empty supplied set remains type-28 `[]`, rather than an absent argument;
  rejected object or interaction class designators append no record. The Lab
  incorrectly assigns the §5.12 continuation on page 102 to §5.13 (RL-080),
  and §11.5.1's Boolean spelling governs over Table 5's uppercase example
  (RL-079).
  `Unsubscribe Object Class Directed Interactions` records its accepted §5.13
  declaration with type-36 `Object class designator` and an Optional set of
  directed interaction designators. The official C++ interaction-set overload
  supplies type-28 `InteractionClassHandleSet`, including `[]` for a
  supplied-empty set; the whole-class overload leaves that same required
  position as type-34 Null. Rejected object or interaction class designators
  append no record. The Lab incorrectly assigns the §5.13 postcondition
  continuation on page 103 to §5.14.3 (RL-080).
  `Publish Interaction Class` records its accepted §5.4 declaration
  transition with type-27 `Interaction class designator` as quoted exact
  `InteractionClassHandle::toString()` text before separately queued declaration
  advisories.
  `Subscribe Interaction Class` records its accepted §5.10 declaration
  transition with type-27 `Interaction class designator` as quoted exact
  `InteractionClassHandle::toString()` text and type-6 `Optional passive
  subscription indicator` as lowercase unquoted `true` or `false`. The public
  C++ `active` selector is inverted to that standards-facing passive indicator;
  §11.5.1's lexical definition governs over Table 5's uppercase example
  (RL-079). The record precedes separately queued declaration advisories.
  `Unpublish Interaction Class` records its accepted §5.5 declaration
  transition with type-27 `Interaction class designator` as quoted exact
  `InteractionClassHandle::toString()` text before separately queued declaration
  advisories.
  `Unsubscribe Interaction Class` records its accepted §5.11 ordinary
  unsubscription invocation with type-27 `Interaction class designator` as
  quoted exact `InteractionClassHandle::toString()` text before separately
  queued declaration advisories.
  `Query Interaction Transportation Type` records type-15 `Federate designator`
  and type-27 `Interaction class designator` as quoted `handle.toString()` text
  when its query plan is accepted; its separately queued `Report Interaction
  Transportation Type` callback remains the response boundary.
Umbra retains an unpublished RTI-owned MIM-object snapshot with
the exact file location. The bounded interaction sink now emits the standard
`HLAreportServiceInvocation` for accepted non-timestamped and timestamped
`Send Interaction` and `Send Interaction With Regions` calls, including all
seven MIM parameters, private-endpoint DDM selection, callback-time
revalidation, reliable receive-order delivery, the region-set supplied
argument, and the timestamped logical-time/message-retraction fields. Generic
return/failure records, periodic/other conditional MOM behavior, public MOM
interactions, Requirements
Lab validation of the callback producer mapping, and broad conformance claims
remain deferred. The first public object-management slice is now implemented:
active ordinary subscribers and matching regional subscribers receive
RTI-owned `HLAfederate` discovery, a reliable initial reflection of all seven
required Table 8 values (including `HLAreportServiceFile`), direct
requested-value reflection for a known object, and removal when the
represented federate resigns. Regional eligibility is evaluated against the
immutable `HLAfederate` point. That slice uses the local default-invalid RTI producer policy recorded
in RL-043 and is explicitly development-profile traceability, not conformance
evidence. A companion event-driven conditional projection now reflects all nine
predefined `HLAfederate` switch attributes after successful individual setters
and accepted `HLAsetSwitches` subsets, plus four bounded temporal-state
attributes after their successful role transitions or time-advance request and
grant transitions, and answers known-object requests from current membership
state. A focused save/restore companion now projects the official
`HLAfederateState` enumeration (`ActiveFederate=1`,
`FederateSaveInProgress=3`, `FederateRestoreInProgress=5`) from the operation
ledgers at the corresponding callback boundaries in both callback models. It
suppresses the save-state event reflection at the federate that is itself
saving while preserving direct known-object AVU access to the current value.
It does not claim the periodic scheduler, other conditional/non-initial
values, or a complete standard producer mapping.

## Authorities

- IEEE 1516.1-2025 §11.5.1 supplies the service-reporting behavior and the
  mutual exclusion with a federate's subscription to the exact report-service
  interaction.
- IEEE 1516.1-2025 §11.5.2--§11.5.2.1 supplies the report-file lifetime,
  initial-record, and brace-delimited log-record rules.
- The vendored 2025 standard MIM defines
  `HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation`,
  its `HLAfederate` and `HLAserviceGroup` dimensions, seven parameters, and
  receive-order / reliable delivery.
- The official 2025 C++ headers remain the public ABI authority.  Umbra must
  not publish a parallel public MOM or encoding API.

## Non-negotiable runtime model

Every report interaction requires one RTI-owned report endpoint update set.
Its private point region has:

- `HLAfederate` set from `normalizeFederateHandle` for the indicated joined
  federate;
- `HLAserviceGroup` set from `normalizeServiceGroup` for the reported
  service; and
- a private identity, so public region services cannot modify, delete, or use
  the endpoint as a caller-supplied update region.

The endpoint is not an ordinary federate-originated interaction.  In
particular, it must not pass through public `sendInteraction`: that path
requires caller publication and caller-owned regions, and would recursively
report the report itself.  A registry-owned reporting planner must instead
perform MIM class/parameter lookup, regional subscription selection, and
callback-time revalidation directly.

## Delivery and state invariants

1. A successful or failed HLA service invocation to or from a joined federate
   is reportable only when that federate's Service Reporting switch is enabled.
2. The subject federate's counter starts at zero and is incremented once for
   every report actually accepted for emission.  It is independent for each
   joined membership and is never advanced by an unsent report.
3. Report interactions use reliable, receive-order delivery and never trigger
   another report.  They bypass the subject's Service Reporting switch during
   their own delivery.
4. Regional subscriptions are selected against the private point region.  The
   normal DDM predicate and callback-time subscription recheck remain the
   common source of truth; the report endpoint is an additional internal
   producer form, not a relaxed special case.
5. A subject federate cannot become a report recipient for itself through the
   excluded exact subscription.  The existing two-way interlock remains the
   admission boundary and must be preserved atomically.
6. `HLAsendServiceReportsToFile` selects the required sink: when enabled for
   the subject federate, the generated interaction is recorded in that
   federate's unique service-report file **instead of** being sent to
   subscribers.  File naming/location remains implementation-defined, but
   bypassing either sink is not permitted.

## Filesystem report-file foundation

The embedded profile's production service-report store is filesystem-backed.
Its public convenience boundary is
`umbra::embedded::ServiceReportConfiguration { std::filesystem::path directory; }`
plus `makeEmbeddedRtiConfiguration`; callers configure a directory and are
never offered a production memory/store-backend choice. The helper serializes
that implementation-defined choice through the official opaque
`RtiConfiguration::additionalSettings` field as
`serviceReportDirectory=<directory>`, preserving the public IEEE ABI. If no
directory is supplied through the official configuration, the embedded profile
uses its own temporary-directory child. The RTI validates/creates the selected
directory at connect time and fails with `RTIinternalError` if it cannot create
or use it. It never silently substitutes an in-memory store.

At each successful join, Umbra allocates an RTI-owned collision-resistant file
path below that directory with exclusive no-replacement filesystem creation,
then writes one
`ServiceReportInitialRecord` before switch state is consulted. The joined
federate retains that writer and immutable path until resignation; a voluntary
§4.12 resignation atomically reserves its final file serial before deleting
that membership, then appends the final record without allocating or changing
the pathname. Switching
file reporting off and on only gates future record appends. A later rejoin gets
a new path. Records are directly concatenated brace-delimited JSON-like
records, with no newline or other separator, matching §11.5.2.1. The writer
opens only the already reserved path for an append: if an external actor
removes it, the append fails deterministically rather than silently recreating
or replacing the joined-federate's logical report file. A writer serializes
local concurrent appends, preserving each complete brace-delimited record;
cross-process append and failure behavior remains a deployment-profile test
item.

If the runtime store cannot create the writer, Join fails with a deterministic
`RTIinternalError` and Umbra rolls the just-created registry membership back
before exposing a successful join. It does not fall back to memory. A
test-injected failing store verifies this rollback; deployment-specific
permission/full-disk failure matrices remain future work.

If an already established writer rejects a selected record append, the
affected public service surfaces `RTIinternalError`. For the terminal voluntary
resignation record, Umbra first completes the irrevocable local departure and
then reports that error, avoiding a stale joined-federate writer. Umbra does
not replace, rotate, recreate, or redirect that writer to memory. A test-only
writer that fails its first append is supplied by a store that captures the
initial record, verifying this boundary;
production permission, full-disk, external-deletion, and cross-process failure
matrices remain future work.

`MemoryServiceReportStore` exists only as an internal unit-test seam. It is
not selected by a public/runtime profile option and does not advertise a
`memory://` location. The non-installed internal `UmbraRtiAmbassador` test
constructor is the only path that injects it; factory-created ambassadors
always construct `FilesystemServiceReportStore`.

The initial record currently captures the actual `RtiConfiguration` fields,
federation/FOM state, and joined federate identity available to the embedded
profile. Its host field is the documented implementation value
`umbra-embedded`; remote host discovery is not implemented.

For a production filesystem writer, the same Join transaction also establishes
one private `HLAobjectRoot.HLAmanager.HLAfederate` snapshot. It reserves an
`ObjectInstanceHandle` from the common federation namespace, holds the complete
effective MIM attribute set as metadata, captures an immutable private
`HLAfederate` point region, and encodes the seven direct initial values using
the official MIM types. The report-file value is the exact absolute pathname
returned by the writer; changing either report switch does not replace the
snapshot or its value, and resignation removes it. Test-only memory stores do
not establish this snapshot and never invent an advertised pathname.

`HLAFOMmoduleDesignatorList` is populated from FOM modules supplied by that
specific Join, not from the federation's accumulated FDD. The registry uses
the validation layer's canonical source identity to retain the first supplied
designator when the same module appears more than once.

This is deliberately not public federate-owned object registration. The
snapshot is held outside the federate-created object map, with a dedicated
RTI-owned object-management ledger. That ledger now feeds the bounded public
discovery, complete seven-value initial reflection, known-object requested-value,
and resignation-removal callbacks; it does not claim ordinary ownership semantics
or a standards-resolved producer handle. A non-installed test inspection seam
still verifies the complete private state without manufacturing a joined
producer identity.

The composed FOM catalog now retains each object's standard attribute
`updateType`, `updateCondition`, `valueRequired`, and `ownership` alongside
its existing type, sharing, transportation, and order fields. A Catch2 regression reads the
unmodified MIM's `HLAmanager.HLAfederate.HLAreportServiceFile` declaration
through that catalog. The private joined-federate snapshot consumes this
metadata to reject an inconsistent MIM: it accepts only Table 8's `Static`
entry or the vendored MIM's exact documented `Conditional` rule, rather than
treating an arbitrary update policy as equivalent. It neither registers an
object nor chooses a public publication event.

The catalog now also exposes a validated effective-attribute projection that
walks the object-class hierarchy without permitting a derived declaration to
hide an inherited policy. The corresponding MIM regression demonstrates why a
complete joined-federate MOM object cannot be assembled from
`HLAfederate`'s direct attributes alone: it inherits
`HLAprivilegeToDeleteObject` from `HLAobjectRoot`, and that inherited
attribute declares `DivestAcquire`, unlike the direct joined-federate
attributes' `NoTransfer` policy. This is a construction prerequisite, not an
ownership implementation or a claim that a federate may delete a MOM object.

The private snapshot now holds the real `HLAreportServiceFile` value, and the
first public MOM slice exposes that value through the normal reliable attribute
reflection route for active ordinary or matching regional subscriptions. The embedded profile's
runtime decision is fixed: the initial update follows
the IEEE 1516.1-2025 Table 8 `Static` entry and uses the already allocated
immutable pathname. The vendored 1516.2 MIM calls the attribute `Conditional`
at the first time both switches become true; Umbra retains that cross-artifact
discrepancy in RL-041 and in the catalog, but it does not let the conflicting
field postpone pathname selection or create a second file on later enablement.

## MOM-object publication and remaining scheduler boundary

The standard MIM says that the RTI publishes `HLAmanager.HLAfederate` and
registers one object instance for every joined federate. It also requires the
RTI to satisfy Request Attribute Value Update through the normal attribute
update mechanism regardless of whether an attribute is static, periodic, or
conditional. Consequently, `HLAreportServiceFile` cannot be added as a
special callback, a synthetic `memory://` value, or a call through the public
federate-owned `registerObjectInstance` path.

The public registration path currently requires a joined federate's
publication and models its transfer-capable attribute ownership. A standards
facing MOM object layer needs a distinct, RTI-owned producer model instead.
Umbra now has that dedicated ledger and a bounded object-management adapter
for the complete seven-value initial projection. Regional discovery is now
implemented for the immutable `HLAfederate` point, and successful changes to
all nine predefined switch attributes plus four bounded temporal-state
attributes produce event-driven current-value reflections. Save/restore now
also drives the bounded `HLAfederateState` enumeration from the real operation
ledgers and omits the save-state event reflection at the saving federate while
leaving direct AVU unaffected. Direct known-object AVU now also supplies the
MIM-periodic `HLAlogicalTime` and `HLAlookahead` values from the selected
official time provider (or the MIM empty-array undefined form), even before a
report period is configured. The bounded `HLAsetTiming` slice now accepts the
official target/reference and `HLAseconds` pair, arms one target-local
wall-clock deadline, and routes the catalog-declared periodic subset through
the ordinary active-subscription planner at an Evoke callback boundary. Idle
`HLA_IMMEDIATE` background delivery, the remaining periodic/other conditional
updates, optional/inherited non-initial attributes, public MOM interactions,
and the standard callback producer mapping remain separate work.
For deferred callback models, each event-driven reflection retains the encoded
value captured at the successful transition while revalidating object lifetime
and subscription eligibility at callback delivery; recomputing only at delivery
would lose transient states such as `TimeAdvancing` after a grant is queued.
The bounded non-timestamped, timestamped, and region-context `Send Interaction`
report slice uses the existing
default-invalid `FederateHandle` boundary representation for the RTI producer;
it does not expose the reported federate's numeric identity as a producer
designator. Generic report callbacks must retain that source distinction and
use the ordinary discovery, reflection, subscription, and callback-time
revalidation machinery once the remaining producer-designator rules are
sourced.

The private report-routing foundation now carries an explicit RTI interaction
source rather than passing numeric `0` through the ordinary federate-sender
selector. That source can decide DDM/subscription eligibility without claiming
that it is a callback-visible `FederateHandle`. The bounded
`HLAreportFederateLost` path has a separately documented local adapter that
uses a default-invalid handle in its non-timestamped callback (RL-065); the
same bounded representation is now used for direct, timestamped, and
region-context `Send Interaction` service reports, while generic §11.5 traffic
still has no public producer-designator mapping.
The source audit covers both C++ `receiveInteraction` overloads: §6.13 requires
their producer argument to designate a sending joined federate, whereas §11.5
requires the report interaction to be generated by the RTI. Neither the
official C++ binding nor the MIM assigns a special producer handle for that
case (RL-043).
That remains an intentional source gate for the unimplemented generic report
families, not a claim that the bounded direct-send slice covers all §11.5
traffic.

The same RTI-owned object ledger now registers one execution-scoped
`HLAobjectRoot.HLAmanager.HLAfederation` object. Its static values
(`HLAfederationName`, `HLARTIversion`, `HLAMIMdesignator`,
`HLAtimeImplementationName`, and the four federation-wide support switches)
are encoded and delivered through the normal discovery/reflection and
Request Attribute Value Update paths, including the JNI/Java/JPype route. The
object is not tied to a represented member's resignation lifetime. Its
conditional membership/FDD/save/Auto-Provide attributes remain deferred until
their lifecycle update sources and callback plans are implemented.
The first conditional exception is now closed for `HLAautoProvide`: the
federation `HLAsetSwitches` service compares the C++ switch ledger, queues a
conditional reflection only when the value changes, and supports discovery
when the subscription contains no static federation attribute. The remaining
conditional federation values stay open.

The intended implementation order is:

1. Retain the MIM object-attribute policy in the composed catalog (complete).
2. Add a private RTI-owned object-instance foundation with a common-namespace
   identity, full effective-attribute metadata, seven encoded initial values,
   and an immutable `HLAfederate` dimension point (complete).
   The execution-scoped `HLAfederation` static-object slice is now established
   from the same foundation; its conditional attributes remain open.
3. Expose the bounded initial public object-management route: active ordinary
   and immutable-point-filtered regional discovery, reliable reflection of all
   seven required initial values, known-object requested-value reflection for
   that projection, and resignation removal (complete for the development
   profile; local producer policy, not conformance).
4. Register the remaining required joined-federate MOM attributes through the
   same public route and implement the remaining static, conditional, and
   periodic attribute update scheduler from catalog metadata and the relevant
   2025 source rules. Direct AVU for the first two MIM-periodic time values is
   now covered, and the bounded `HLAsetTiming` deadline pump emits the
   catalog-declared periodic subset at an Evoke boundary; the remaining
   periodic/conditional attributes and idle `HLA_IMMEDIATE` background timer
   remain open. The nine event-driven switch projections and four bounded
   temporal-state projections remain the other bounded subset.
5. Add regional updates, public MOM interactions, and a source-backed
   producer-designator rule before any conformance claim. The
   `HLAreportServiceFile` value already uses the allocated filesystem pathname
   and selected Table 8 static publication event in the bounded slice.

This sequence deliberately avoids a partial one-attribute MOM implementation.
It gives all MOM attributes one RTI-owned object foundation and keeps public
binding APIs aligned with the official headers.

## Payload boundary

All seven MIM parameters must be present and use their declared 1516.2 data
types:

| Parameter | MIM type |
| --- | --- |
| `HLAservice` | `HLAunicodeString` |
| `HLAserviceType` | `HLAserviceTypeEnum` |
| `HLAsuccessIndicator` | `HLAboolean` |
| `HLAsuppliedArguments` | `HLAargumentList` |
| `HLAreturnedArgument` | `HLAargument` |
| `HLAexception` | `HLAunicodeString` |
| `HLAserialNumber` | `HLAcount` |

`HLAserialNumber` starts at zero for a joined-federate lifetime and advances
once for every generated service report. It is audit/file-lifetime state, not
federated application state: restoring a federation snapshot cannot rewind it
or reuse a number already written to that federate's immutable report file.

The standard argument-list is a variable array of the MIM's fixed-record
`HLAargument` type (not a variant record).  Umbra now has the first private
MIM encoder and byte-vector tests for that record plus all seven individual
report parameters. The 1516.2 Requirements Lab's `HLAvariableArray` source
requires a signed `HLAinteger32BE` **element count**, followed by properly
padded elements; it is not a byte-length prefix. Umbra applies that rule to
both `HLAargumentList` and the `HLAunicodeString` data element used by MOM
parameters. The public binding also now implements the required
integer, boolean, and Unicode primitives.  Each argument's Unicode value is
the Table 5 JSON-like textual depiction for its `HLAargumentType`; that table
must be modeled before a service wrapper is admitted. This remains an
encoding foundation: the registry now plans a private normalized endpoint and
uses the ordinary DDM subscription predicate for its recipients.
Source-backed successful-void wrappers reserve and append file-selected records
only when their own supplied arguments, successful boundary, and callback
ordering have been reviewed. The direct non-timestamped and timestamped `Send
Interaction` wrappers reserve the interaction serial and emit all seven
standard parameters through the ordinary Java/Python `receiveInteraction`
callback shape; the timestamped form also carries the admitted
message-retraction designator and logical-time argument, while the
RTI-originated producer is represented by the existing default-invalid
`FederateHandle` boundary value. Other service-family interaction reports
remain deferred until their lock-safe callback transport and source mappings
are reviewed. The first `HLAreportException` slice is now implemented for
failed direct, timestamped, directed, and non-timestamped region-context
`Send Interaction` calls: the typed
`InteractionClassNotPublished` still reaches the caller, while the observer
receives the standard `HLAservice`/`HLAexception` report through the same
Java/Python callback path. The declaration-management `Subscribe Interaction
Class` conflict path now preserves `FederateServiceInvocationsAreBeingReportedViaMOM`
and independently emits its `HLAreportException` callback. Other failing-service
families remain deferred. The complementary `Set Service Reporting Switch`
conflict path now preserves `ReportServiceInvocationsAreSubscribed` and emits
its separate `HLAreportException` callback. The time-management `Enable Time
Regulation` conflict path now preserves `TimeRegulationAlreadyEnabled` on a
duplicate standard Java request and emits its separate `HLAreportException`
callback through the same observer route. The matching `Enable Time
Constrained` conflict path now preserves `TimeConstrainedAlreadyEnabled` on a
duplicate standard Java request and emits its separate exception report
through the same observer route. The per-service
The pending `Enable Time Regulation` conflict path now preserves
`RequestForTimeRegulationPending` while the first request awaits its callback
and emits its separate exception report through the same observer route. The
per-service
The pending `Enable Time Constrained` conflict path now preserves
`RequestForTimeConstrainedPending` while the first request awaits its callback
and emits its separate exception report through the same observer route. The
per-service
The asynchronous-delivery `Enable Asynchronous Delivery` conflict path now
preserves `AsynchronousDeliveryAlreadyEnabled` on a duplicate standard Java
request and emits its separate exception report through the same observer
route. The per-service
The inverse-state `Disable Asynchronous Delivery` conflict path now preserves
`AsynchronousDeliveryAlreadyDisabled` while the switch is off and emits its
separate exception report through the same observer route. The per-service
The inverse `Disable Time Regulation` and `Disable Time Constrained` conflict
paths now preserve `TimeRegulationIsNotEnabled` and
`TimeConstrainedIsNotEnabled` while their modes are off, emitting separate
exception reports through the same observer route. The per-service
The unregulated `Query Lookahead` and `Modify Lookahead` conflict paths now
preserve `TimeRegulationIsNotEnabled` and emit separate exception reports
through the same observer route. The per-service
The pending `Modify Lookahead` conflict path now preserves
`InTimeAdvancingState` while a time advance is pending and emits its separate
exception report through the same observer route. The per-service
The regulation-pending `Time Advance Request` and `Time Advance Request
Available` conflict paths now preserve `RequestForTimeRegulationPending` while
Enable Time Regulation awaits its callback and emit separate exception reports
through the same observer route with their service labels. The per-service
The constrained-pending `Time Advance Request` and `Time Advance Request
Available` conflict paths now preserve `RequestForTimeConstrainedPending` while
Enable Time Constrained awaits its callback and emit separate exception reports
through the same observer route with their service labels. The per-service
The pending `Time Advance Request` conflict path now preserves
`InTimeAdvancingState` when a first request is awaiting a grant and emits its
separate exception report through the same observer route. The per-service
The overload `Time Advance Request Available` conflict path now preserves
`InTimeAdvancingState` while its first request is pending and emits its
separately named exception report through the same observer route. The
next-message `Next Message Request` conflict path now preserves
`InTimeAdvancingState` while its first request is pending and emits its
separate exception report through the same observer route. The
The overload `Next Message Request Available` conflict path now preserves
`InTimeAdvancingState` while its first request is pending and emits its
separately named exception report through the same observer route. The
The pending `Flush Queue Request` conflict path now preserves
`InTimeAdvancingState` while its first request is pending and emits its
distinct exception report through the same observer route. The per-service
contracts and focused CTest lanes are the maintained inventory; a single
aggregate count would conflate distinct overloads and shared internal call
sites. Serial reservation is atomic with the selected sink and starts at zero
for each joined federate.

The same rule is now enforced for the public C++ handle values that a future
joined-federate MOM object must place in `HLAfederateHandle` and related MIM
fields. IEEE 1516.1-2025 §12.12.5 requires each C++ handle helper to emit the
corresponding `HLA<type>Handle` data type defined by `HLAstandardMIM`; those
MIM types are dynamic `HLAvariableArray` values of `HLAbyte`. Umbra's internal
identity payload is deliberately fixed to eight octets, so `Handle::encode()`
emits a twelve-octet HLAvariableArray: the signed big-endian element count
`8`, then the eight opaque identity octets. This is an Umbra binding
foundation, not a claim that every RTI uses an eight-octet identity payload or
that interoperability has been demonstrated.

The original §11.5.1 prose confirms the MIM wire representation: both
`HLAsuppliedArguments` and `HLAreturnedArgument` use the `HLAargument` fixed
record, a void or failed service uses the `Null` argument type, and multiple
returns use an appropriate composite `HLAargumentType`.  The interaction
encoder follows that shared source rule.

There is deliberately no generic `ServiceReportRecord` file formatter. Table
5's log-record row calls the return field `ReturnArgument` without defining
that type and depicts an array-shaped `[null]` no-return value. Section
11.5.2.1 directs log records to Table 5, but does not state how that log-only
notation maps to the interaction's one `HLAargument` fixed record. The
§11.5.1 implementation-dependent wording applies to the textual depiction in
the `HLAargumentName` field; it does not declare the `ReturnArgument` alias or
authorize treating the interaction record as the file record. RL-042 preserves
that boundary. The private code therefore provides Table 5 primitive and
initial-record formatters, the exact interaction encoder, and one deliberately
narrow file formatter for the table's explicit successful-void `[null]` case.
`Set Object Class Relevance Advisory Switch`, `Set Attribute Relevance
Advisory Switch`, `Set Attribute Scope Advisory Switch`, `Set Interaction
Relevance Advisory Switch`, `Set Convey Region Designator Sets Switch`, `Set
Exception Reporting Switch`, and `Set Automatic Resign Directive` are the
first seven wrappers using it. The last uses Table 5's `ResignAction` argument
type 44 and its C++ enum spelling (for example, `DELETE_OBJECTS`), rather than
the distinct MIM wire-enumerator spelling. `Synchronization Point Achieved`
adds type-53 `Synchronization point label` and type-6 `Optional
synchronization-success indicator` after the registry accepts the §4.17
achievement. The public C++ defaulted Boolean is represented as its effective
lowercase Boolean value, rather than as a missing record slot; that mirrors
the existing source-backed defaulted-Boolean convention. The report is written
before the separately queued Federation Synchronized callbacks. `Register
Federation Synchronization Point` adds its §4.14 type-53 `Synchronization
point label`, Table 5 type-63 base-64 `User-supplied tag`, and its required
third optional-set position. The two-argument C++ overload preserves that
position as type-34 Null; the three-argument overload uses type-18
`FederateHandleSet`, including `[]` when the caller explicitly supplies an
empty set. A registration-invocation record is written before the separately
queued §4.15 registration-result callbacks and each recipient-local §4.16
announcement record/callback pair. The recipient route selects its own joined
federate file before queueing the callback. The type-63 spelling remains
bounded private file text under RL-077. `Request
Federation Save` preserves §4.19's two public C++ overload forms in its
accepted report record: both write type-53 `Federation save label`; the
untimed overload retains a type-34 Null `Optional timestamp`, while the
timestamped overload writes type-31 `LogicalTime` from the private clone's
quoted `toString()` representation. Its report precedes any separately queued
Initiate Federate Save work. `Request Federation Restore` records its one
type-53 `Federation save label` on its normally returned §4.27 path.
The distinct RTI-initiated `ConfirmFederationRestorationRequest` record then
uses the requester's selected file and next serial for that same label plus the
type-6 `Request-success indicator`; both the true and false forms are durable
before their respective C++ callbacks. The following RTI-initiated §4.29
`FederationRestoreBegun` record has no supplied arguments and is appended to
every joined recipient's selected file—including the requester—before its
callback is queued. The following §4.30 `InitiateFederateRestore` record uses
that recipient's existing file and next serial, with the type-53 `Federation
save label`, type-15 `Joined federate designator`, and type-53 `Federate name`,
before its callback is queued. `Federate Restore Complete` maps the two official C++ selectors to one
`FederateRestoreComplete` record with its required type-6 `Federate
restore-success indicator`: `true` is durable before Federation Restored and
`false` before Federation Not Restored. Its HLAserialNumber remains a live
per-joined-federate audit sequence across restored application state rather
than rolling back with the snapshot. `Federate Save Begun` records the accepted
§4.21 save-control transition with
the Table 5 successful-void `[]` supplied-argument and `[null]`
returned-argument forms. A rejected pre-initiation invocation appends no
record; completion and Federation Saved are separate service/callback
boundaries. `Federate Save Complete` records §4.22's required type-6
`Federate save-success indicator` after acceptance and before the resulting
Federation Saved or Federation Not Saved callback. `federateSaveComplete()`
selects `true`; `federateSaveNotComplete()` selects `false`, while both retain
the one `FederateSaveComplete` service name. The distinct RTI-initiated
`FederationSaved` report preserves the result form at every selected recipient:
type-6 true with type-34 Null for completion, or type-6 false with type-48
`SaveFailureReason` for failure, before its result callback. The focused lane
proves normal completion and the ordinary `SAVE_ABORTED` failure. `Abort Federation Save` records
its accepted §4.24 request with no supplied arguments before the later
Federation Not Saved result in the ordinary abort path. The all-members-already-
complete success result remains unclaimed. `Query Federation Save Status`
records the accepted §4.25 no-argument request before the separately queued
Federation Save Status Response callback supplies the member-status vector.
The distinct §4.26 report serializes that vector as type-17
`FederateHandleSaveStatusPairSet` before callback delivery: its array elements
are Table 5 `handle`/`status` records with quoted public values. The focused
lane fixes the one-member `FEDERATE_INSTRUCTED_TO_SAVE` shape and ordering.
`Enable Time Regulation` adds its
one `Lookahead` argument using Table 5's `LogicalTimeInterval` type 32 and
the private reference interval's quoted `toString()`; `Modify Lookahead` adds
its one `Requested lookahead` argument with the same type and form. `Disable
Time Regulation`, `Enable/Disable Asynchronous Delivery`, and `Enable/Disable
Time Constrained` are the five no-argument Time Management wrappers for which
§11.5.1 requires empty supplied-argument lists. The regulation and constrained
enable records describe accepted requests, not their later callback-gated state
completions; the Modify Lookahead record likewise describes an accepted lower
request before its actual value changes at a grant boundary. `Time Advance
Request`, `Time Advance Request Available`, `Next Message Request`, `Next
Message Request Available`, and `Flush Queue Request` each add one `Logical
time` argument using Table 5's `LogicalTime` type 31 and the private reference
time's quoted `toString()`; their records are written on acceptance while the
actual logical time still awaits the later grant callback. The four non-Flush
Queue requests later receive Time Advance Grant, while Flush Queue Request
receives the distinct actual/optimistic Flush Queue Grant. The two Next Message
Request forms and Flush Queue Request preserve their supplied boundaries rather
than possibly earlier queued-message grant targets. No generic return,
composite return, failed-service, or
interaction file-record rendering is inferred from that case. The filesystem
foundation also receives accepted `Retract` invocations using Table 5's type-33
`MessageRetractionDesignator` form before callback-gated Request Retraction.
`Change Attribute Order Type` adds type-37 `Object instance designator` as
quoted `ObjectInstanceHandle::toString()`, type-1 `Set of attribute designators`
as a bracketed array of quoted `AttributeHandle::toString()` values, and type-38
`Order type` with the quoted `RECEIVE`/`TIMESTAMP` form after a successful
owned-attribute invocation. `Change Default Attribute Order Type` adds type-36
`Object class designator` as quoted `ObjectClassHandle::toString()`, type-1
`Set of attribute designators` as the same quoted-handle array, and type-38
`Order type` using the quoted `RECEIVE`/`TIMESTAMP` form after a successful
class-default invocation. `Change Default Attribute Transportation Type` uses
the same type-36/type-1 forms with type-59 `Transportation type` as quoted
`TransportationTypeHandle::toString()` text at accepted prospective
class-default request time. `Change Interaction Order Type` adds type-27
`Interaction class designator`
  and type-38 `Order type` arguments using the Table 5 handle-string and
  RECEIVE/TIMESTAMP forms. `Request Interaction Transportation Type Change` adds
  type-27 `Interaction class designator` and type-59 `Transportation type`
  arguments using the same quoted handle-string form at accepted request time;
  its later confirmation remains the preference-change boundary. `Request
  Attribute Transportation Type Change` adds type-37 `Object instance
  designator` as quoted `ObjectInstanceHandle::toString()`, type-1 `Set of
  attribute designators` as a bracketed array of quoted
`AttributeHandle::toString()` values, and type-59 `Transportation type` as
quoted `TransportationTypeHandle::toString()` text at accepted request time;
  its later confirmation remains the preference-change boundary. `Query
Attribute Transportation Type` adds type-37 `Object instance designator` and
type-0 `Attribute designator` using quoted `handle.toString()` text at accepted
  query time; its separately queued report callback remains the response
  boundary. `Query Attribute Ownership` adds type-37 `Object instance
  designator` and type-1 `Set of attribute designators` using quoted
  `handle.toString()` text and a bracketed array of quoted
  `AttributeHandle::toString()` values at accepted query time; its separately
  queued grouped ownership-result callbacks remain the response boundary.
  `Unconditional Attribute Ownership Divestiture` adds those type-37/type-1
  forms plus the caller's type-63 Table 5 `UserSuppliedTag` Binary Data at
  accepted §7.2 invocation. §11.5.1 defines Binary Data as double-quoted
  base-64 data, so the private formatter uses canonical base-64 text. Its
  report is written before separately queued §7.4 ownership-assumption work.
  The type-63 Table 5 literal conflicts with the official MIM's type-60
  `UserSuppliedTag`, so the formatter remains file-text-only pending an
  authoritative correction; it is not evidence for a live MOM interaction
  encoding (RL-076/RL-077).
  `Attribute Ownership Acquisition` adds type-37 `Object instance designator`,
  type-1 `Set of attribute designators`, and type-63 base-64 `User-supplied
  tag` after the accepted §7.8 request plan and before its separately queued
  release or acquisition work.
  `Attribute Ownership Acquisition If Available` adds the same type-37,
  type-1, and type-63 forms after its accepted §7.9 request plan and before
  the supplied-empty return or separately queued notification/unavailable
  callback.
  `Attribute Ownership Release Denied` adds type-37 `Object instance
  designator`, the source's long type-1 unwilling-to-divest attribute-set
  name, and type-63 base-64 `User-supplied tag` after its accepted §7.12 plan
  and before separately queued unavailable callbacks.
  `Confirm Divestiture` adds type-37 `Object instance designator`, type-1 `Set
  of attribute designators`, and type-63 base-64 `User-supplied tag` after its
  accepted §7.6 transfer plan and before separately queued acquisition
  notification work. The type-63 form stays private file text (RL-077).
  `Negotiated Attribute Ownership Divestiture` adds type-37 `Object instance
  designator`, type-1 `Set of attribute designators`, and type-63 base-64
  `User-supplied tag` after the accepted §7.3 request plan and before separately
  queued confirmation work. The type-63 form stays private file text (RL-077).
  `Cancel Negotiated Attribute Ownership Divestiture` adds type-37 `Object
  instance designator` and type-1 `Set of attribute designators` after the
  accepted §7.14 cancellation plan and before separately queued ordinary
  release work is restored. RL-017 records the distinct generated state-chart
  inconsistency; it is not this source-backed record's transition authority.
  `Attribute Ownership Divestiture If Wanted` is not routed through this
  successful-void formatter: §7.13.2 returns a source-named actual-divestiture
  attribute set, and RL-042 preserves the missing Table 5 non-void file-record
  shape rather than allowing an invented generic return encoding.
  `Cancel Attribute Ownership Acquisition` adds type-37 `Object instance
  designator` and type-1 `Set of attribute designators` after the accepted
  §7.15 cancellation plan and before its separately queued confirmation
  callback (or its supplied-empty no-callback completion).
  `Local Delete Object Instance` adds its type-37 `Object instance designator`
  after the accepted §6.18 local-forget transition, with no later callback that
  could serve as an alternate reporting boundary.
  `Commit Region Modifications` now records its accepted void-return §9.3
  invocation in the data-distribution-management group with one type-43
  `Set of region designators` argument. The value is Table 5's
  `RegionHandleSet`/`Array<RegionHandle>` form using the official quoted
  `RegionHandle::toString()` values, and the record is appended before queued
  scope/relevance work. Invalid handles append nothing. Create Region
  reporting and generic non-void `ReturnArgument` forms remain outside this
  successful-void inventory; RL-042 still blocks an invented return shape.
  `Delete Region` now has the adjacent focused §9.4 report path: after the
  caller-owned unused region is removed, its one type-42 `Region designator`
  argument is written to the same joined-federate file before the service
  returns. Invalid or otherwise rejected deletion appends nothing. This does
  not imply a report for Create Region, region realizations, callback delivery,
  or any generic non-void return form.
  `Set Range Bounds` now has the neighboring focused §10.28 report path. Its
  accepted record carries type-42 `Region handle`, type-10 `Dimension handle`,
  and type-35 `Range lower bound`/`Range upper bound` Number arguments after
  the pending range is accepted. Invalid bounds append nothing. Get Range
  Bounds and region-realization/callback report forms remain outside this
  inventory.
  `Associate Regions For Updates` and `Unassociate Regions For Updates` now
  record their accepted §9.6/§9.7 transitions with type-37 `Object instance
  designator` and MIM type-4 `AttributeSetRegionSetPairList` (Table 5's
  `AttributeRegionAssociationList`) values. Each record is written before
  scope/relevance work is queued; invalid region input appends nothing. The
  registration-established association, region-realization, and callback
  report forms remain separate work.
  `Subscribe Object Class Attributes With Regions` and `Unsubscribe Object
  Class Attributes With Regions` now record their accepted §9.8/§9.9
  transitions in the DDM group. Both use type-36 `Object class designator` and
  type-4 `AttributeSetRegionSetPairList` (Table 5's
  `AttributeRegionAssociationList`). Regional subscription additionally uses
  type-6 `Optional passive subscription indicator` (the inverse of the public
  C++ `active` selector) and type-53 `Optional update rate designator`, or
  type-34 Null when the default rate is selected. Records are written before
  separately queued scope/relevance/discovery callbacks; invalid regions
  append nothing. Composite callback/report forms remain deferred under
  RL-042.
  `Delete Object Instance` adds type-37 `Object instance designator`, the
  Table 5 type-63 base-64 `User-supplied tag`, and type-34 Null for its
  omitted optional timestamp after the accepted §6.16 deletion transition and
  before its separately queued `Remove Object Instance` callbacks.
  `Send Directed Interaction` adds type-27 `Interaction class designator`,
  type-37 `Object instance designator`, type-40
  `PairList<ParameterHandle:BinaryData>`, the Table 5 type-63 base-64
  `User-supplied tag`, and type-34 Null for its omitted optional timestamp
  after accepted §6.14 route planning and before separately queued `Receive
  Directed Interaction` callbacks.
  `Publish Object Class Attributes` adds its type-36 `Object class designator`
  and type-1 `Set of attribute designators` after the accepted §5.2 publication
  transition and before declaration advisories and ownership-assumption work
  are queued. The source candidate is coalesced under the Lab's §5.2.4 owner,
  rather than §5.2 itself (RL-078).
  `Unpublish Object Class Attributes` adds its type-36 `Object class
  designator` and type-1 `Optional set of attribute designators` after the
  accepted §5.3 teardown and synchronous ownership cleanup, before declaration
  advisories are queued. The source candidates are coalesced under the Lab's
  §5.3.3 owner, rather than §5.3 itself (RL-078).
  `Subscribe Object Class Attributes` adds type-36 `Object class designator`,
  type-1 `Set of attribute designators`, type-6 `Optional passive subscription
  indicator`, and type-53 `Optional update rate designator` after the accepted
  §5.8 subscription transition. It serializes the inverse of the public C++
  `active` selector, while the empty/default update-rate selector uses type-34
  Null. The report precedes declaration, scope, relevance, and discovery
  callbacks. The Lab assigns the §5.8 update-rate continuation candidates to
  §5.9; RL-080 records that cross-page mismatch.
  `Unsubscribe Object Class Attributes` adds type-36 `Object class
  designator` after an accepted §5.9 removal. The official C++ attribute-set
  overload emits the supplied type-1 `Optional set of attribute designators`,
  whereas the official whole-class overload emits type-34 Null in that required
  optional argument position. Both records precede declaration, scope, and
  relevance callbacks; their coalesced source candidates retain the Lab's
  §5.9.4 owner rather than §5.9 itself (RL-078).
  `Publish Object Class Directed Interactions` adds type-36 `Object class
  designator` and type-28 `Set of interaction class designators` after the
  accepted §5.6 declaration transition. The supplied set uses Table 5's
  `Array<InteractionClassHandle>` form, so it is a bracketed list of quoted
  exact `InteractionClassHandle::toString()` values. A successful supplied
  empty set remains an empty array rather than an absent argument. The
  coalesced §5.6 candidates retain the Lab's §5.6.3 owner (RL-078).
  `Unpublish Object Class Directed Interactions` adds type-36 `Object class
  designator` after an accepted §5.7 removal. The interaction-set overload
  supplies its Optional set of interaction class designators as type-28
  `Array<InteractionClassHandle>` (including a supplied-empty `[]`), while the
  whole-class overload emits type-34 Null in that required optional position.
  The coalesced §5.7 candidates retain the Lab's §5.7.4 owner (RL-078).
  `Subscribe Object Class Directed Interactions` adds type-36 `Object class
  designator`, type-28 `Set of directed interaction designators`, and type-6
  `Optional universal subscription indicator` after its accepted §5.12
  subscription transition. The defaulted C++ `universally` selector records its
  effective by-ownership state as lowercase `false`; an explicit universal
  selection records lowercase `true`. A supplied empty set remains type-28
  `[]`. The page-102 continuation is incorrectly assigned to §5.13 by the Lab
  (RL-080), while §11.5.1's lexical Boolean form governs over Table 5's
  uppercase example (RL-079).
  `Unsubscribe Object Class Directed Interactions` adds type-36 `Object class
  designator` after an accepted §5.13 removal. The interaction-set overload
  supplies its Optional set of directed interaction designators as type-28
  `Array<InteractionClassHandle>` (including a supplied-empty `[]`), while the
  whole-class overload emits type-34 Null in that required optional position.
  The Lab assigns the §5.13 postcondition continuation on page 103 to §5.14.3
  (RL-080).
  `Publish Interaction Class` adds its type-27 `Interaction class designator`
  after the accepted §5.4 publication transition and before declaration
  advisories are queued.
  `Subscribe Interaction Class` adds type-27 `Interaction class designator`
  and type-6 `Optional passive subscription indicator` after the accepted §5.10
  subscription transition and before declaration advisories are queued. It
  serializes the inverse of the public C++ `active` selector as lowercase
  `true`/`false` under §11.5.1 rather than copying Table 5's uppercase Boolean
  example (RL-079).
  `Unpublish Interaction Class` adds its type-27 `Interaction class designator`
  after the accepted §5.5 publication removal and before declaration advisories
  are queued.
  `Unsubscribe Interaction Class` adds its type-27 `Interaction class
  designator` after the accepted §5.11 ordinary-unsubscription invocation and
  before declaration advisories are queued.
  `Query Interaction Transportation Type` adds type-15 `Federate
  designator` and type-27 `Interaction class designator` using quoted
  `handle.toString()` text at accepted query time; its separately queued report
  callback remains the response boundary. The filesystem foundation therefore
  includes these source-backed service invocations while public
  `HLAreportServiceFile`
discovery/reflection remains unimplemented.

`Set Service Reporting Switch` and `Set Send Service Reports To File Switch`
are deliberately not among the seven support-service wrappers. Each changes a
predicate that decides
whether the invocation has a report and where it is recorded. The current
2025 source/export does not define whether that predicate is observed before
or after the setter's own successful state transition; RL-066 preserves the
decision rather than making an unmarked embedded-profile choice.

## Implementation order and evidence

1. Keep the bounded public object-management slice covered while resolving the
   RTI-created callback producer-designator rule (RL-043). Its current local
   default-invalid producer policy is not conformance evidence. Extend the
   established joined-federate snapshot to the remaining public
   discovery/reflection and requested-value handling without changing its
   already allocated file identity. Preserve the Table 8/MIM discrepancy as
   RL-041 source traceability. The explicit successful-void `[null]` case is
   enabled only for source-reviewed void-return wrappers; establish a
   source-backed per-service
   `ReturnArgument` mapping before any other generated file records are
   enabled, and do not reuse the interaction `HLAargument` formatter as a
   file-record substitute.
2. Complete the remaining public 1516.2 helpers required beyond the current
   report-payload foundation, using only the official C++ binding types.
3. Add direct tests for the private endpoint planner: normalized point
   coordinates, regional selection, callback-time withdrawal, and no recursive
   reporting.
4. Extend the completed direct, timestamped, and region-context `Send
   Interaction` report slice to the remaining source-backed
   file path service by service, including success and declared-exception
   outcomes. Keep proving the seven encoded parameters and serial behavior
   through Catch2 and the exact-JAR JPype lane.
5. Add the source/API and Requirements Lab contracts only for that concrete
   behavior.  Do not promote a catalog/JUnit/validation record until the Lab
   workflow has accepted real test evidence.
6. Extend report coverage service family by service family, retain the
   established file writer identity across switch changes, and continue the
   `HLAreportException` work beyond the implemented direct, timestamped,
   directed, non-timestamped region-context, and timestamped region-context
   `Send Interaction` slice, the declaration-management subscription conflict,
   the complementary service-switch conflict, and the duplicate
   `Enable Time Regulation` conflict (other declared service failures).

## Standards-gated future test matrix

The adjacent legacy Python RTI contributed useful test topology, not a runtime
model. Its report sink is a JSONL audit facility and must not be transplanted:
the 2025 behavior chooses exactly one generated-report sink. The extracted
matrix is tracked in the [legacy Python RTI scenario backlog](LEGACY-PYTHON-RTI-TEST-BACKLOG.md).
The first Umbra cases should establish the following independently reviewed
2025 expectations:

1. A three-state sink matrix: reporting disabled yields no generated report;
   reporting enabled with file reporting disabled yields exactly one eligible
   MOM interaction; both enabled yields exactly one report-file record and no
   MOM interaction. The last arm is now narrowly covered for one successful
   void support service, and the first two are covered for direct
   non-timestamped, timestamped, and region-context `Send Interaction`; other
   service-family report cases remain future work.
2. A subject/witness observer topology across evoked and immediate callback
   models, including unsubscribe-after-plan and callback-time recipient
   revalidation.
3. Success and declared-exception coverage for one completed service family;
   an invalid incoming MOM adjustment must not create a positive service
   report.
4. Service-family expansion through declaration, ownership, time, and DDM
   paths only after each wrapper's arguments, return values, and exceptions
   have a reviewed 2025 mapping.
5. Multi-federate/rejoin lifecycle pressure: independent serials and files,
   no self-report recursion, toggle stability, and a new file only for a new
   joined-federate lifetime.
