# HLA Java RTI Contract TCK

This package is a vendor-neutral executable test set for an IEEE 1516.1-2025
Java RTI provider. The Java sources import only JDK classes and the official
`hla.rti1516_2025` API. Provider selection happens through the standard
`RtiFactoryFactory`/Java service-loader boundary.

Use the [Java TCK guide](../../docs/testing/JAVA-RTI-CONFORMANCE-TCK.md) for
the evidence model and provider-matrix workflow.

## Scope

The catalog covers factory discovery,
basic and composite standard encoding, federation creation/join/resign/destroy/query, ordinary
publication/subscription, named and unnamed object registration and discovery,
single and multiple name-reservation callbacks, released-name reuse after object
deletion, instance- and class-scoped attribute value update requests, attribute
update/reflection, object deletion and removal callbacks,
ordinary and timestamped interaction delivery (including the RO-positive/TSO-gated asynchronous-delivery boundary), support lookups, negative cases, relevance
advisories, directed interactions, derived-target subscription eligibility,
and transportation/order controls. The
`java-tck.federate-application-multiple-executions` case creates two
executions, joins multiple federates to each, and reuses one ambassador in the
second execution after resignation. It runs by default after focused JNI
passes in both callback models, using the adapter's FOM and time selection.
The `java-tck.unsubscribe-last-attribute-stops-discovery` and
`java-tck.attribute-unsubscribe-stops-reflection` cases now run by default.
They first establish positive discovery/reflection, then verify that removing
the selected attribute subscription suppresses later callbacks within a
bounded observation window. Both passed focused JNI runs in evoked and
immediate callback modes using the adapter's ordinary object FOM.
The `java-tck.callback-reentrancy` case
checks the official API's callback-service exception boundary; because the
Requirements Lab has no standalone normative requirement for re-entry, it is
classified as API/exception traceability rather than conformance evidence. It
is parity-linked to `cpp-tck.callback-reentrancy` and runs by default after
focused JNI passes in both callback models. Its immediate-callback observation
uses a bounded wait and thread-safe recorder; no independent-provider claim is
made.
The `java-tck.null-federate-ambassador` case invokes every official
`FederateAmbassador` callback overload through `NullFederateAmbassador`,
including timestamped and directed callbacks, and passes independently of
federation/FOM setup on the local JNI provider.
The `java-tck.authorizer-factory-factory` case discovers the standard named
authorizer factory and authorizer and verifies unknown-name rejection; it also
passes without federation/FOM setup on the local JNI provider.
The `java-tck.configuration-authorization-contract` case covers Java builder
accessors, configuration results, information values, standard credentials,
password encoding/decoding, and authorizer dispatch. It passes without
federation/FOM setup. Unlike C++ `VariableLengthData`, Java `Credentials`
exposes its byte array directly, so the test does not impose C++ copy semantics
on the Java API.
The C++ `VariableLengthData` ownership/pointer-adoption contract has no direct
Java equivalent: the standard Java API uses `byte[]` and does not define pointer
ownership or adoption. The Java catalog records this as not applicable instead
of mapping it to a broad API-inventory test.
The `java-tck.data-element-base-contract` case exercises the shared Java
`DataElement` contract for encoded length, octet boundary, full encoding,
positioned `ByteWrapper` encoding/decoding, and truncated input rejection. It
passes without federation/FOM setup. Java's `DataElement` interface does not
expose the C++ clone, type-comparison, hash, `encodeInto`, or `decodeFrom`
members, so those C++-specific methods are not claimed as Java parity.
The `java-tck.handle-collection-values` case exercises standard attribute
handle-set and attribute/parameter value-map factories, set/map clone
independence, attribute/region pair-list cloning, save/restore status value
records, and `RangeBounds` value equality. It passes without federation or FOM
setup. It covers the shared Java surface only; C++-specific handle default
construction, parameter-handle sets, and vector copy/assignment semantics are
not claimed as Java parity.
The `java-tck.standard-enumeration-contract` case mirrors the promoted C++ enum
contract across all eleven corresponding Java enum families. It checks each
official enum's exact standard constant-name set without requiring a federation,
FOM, or provider-specific behavior; this is API-contract evidence, not a claim
about provider runtime behavior.
The `java-tck.standard-exception-hierarchy` case translates the common subset
of the promoted C++ exception contract: all 100 shared exception types must
construct through the standard Java `RTIexception` base and preserve their
standard name and message. Java copy/assignment and stream behavior, plus C++
exception types without direct Java counterparts, remain explicitly unclaimed.
The `java-tck.basic-data-elements` case independently covers the standard scalar
basic-data-element contract, including exact wire vectors, round trips, and
malformed scalar decoding; it passes on the local JNI provider. This is linked
to the promoted C++ basic-data-elements contract. The `java-tck.encoder-round-trip`
case covers fixed and variable arrays
(including the empty-array count prefix), aligned records, variant records,
and extendable variant records. Its wire
checks are paired with the promoted C++ composite-data-elements contract;
runtime validation currently exposes an unknown-discriminant variant getter
behavior difference in the available provider.
The `java-tck.malformed-inputs` scenario now mirrors the promoted C++ case:
it rejects missing and malformed FOM inputs, adapter-supplied invalid FOM
modules, and truncated `HLAinteger32BE` data, then disconnects through the
standard Java API. The adapter supplies invalid modules through
`hla.rti.tck.invalidFomModules`. Both `java-tck.malformed-inputs` and
`java-tck.support-services` pass the adapter's MIM explicitly through the
standard `createFederationExecutionWithMIM` service.
The `java-tck.support-services` case also checks that public name-based
support lookups and service-group normalization reject calls before connect
and before federation join, using the adapter's callback model, and complements
those lifecycle gates with joined-state handle checks.
The `java-tck.standard-order-transportation-lookups` case also verifies that
each federate's standard transportation-handle factory defaults match the
handles returned by the standard `HLAreliable` and `HLAbestEffort` name lookups.
The `java-tck.logical-time-factory` case now mirrors the C++ lifecycle gates
for `getTimeFactory` before connection and after connection but before join,
and uses the adapter-selected callback model. It round-trips initial/final time
values and zero/epsilon intervals through the provider-selected standard
factory, checks offset encoding boundaries, ordering and arithmetic, and rejects
empty encodings. C++ mutable boundary setters and `HLAlogicalTime` DataElement
wrappers have no direct Java equivalent; the catalog marks those C++ slices
non-applicable rather than claiming Java parity. Federation-backed checks still
require a provider that can create an execution with adapter-supplied FOM/time
inputs.
The `java-tck.overloads-and-exceptions` connection slice now covers all four
standard Java `connect` overloads plus the adapter-configured
`RtiConfiguration` overload, validates the standard additional-settings result
codes, and checks pre-connect service exceptions, duplicate connect, callback
disable/enable and empty-queue servicing, and reconnecting the same ambassador.
It uses the adapter-selected callback model and passes on the local JNI provider
in both callback modes. The C++ pre-connect `disconnect`
exception assertion is intentionally not translated because the Java standard
API does not declare `NotConnected` for `disconnect`.
Optional standard connection settings for this scenario come from adapter JVM
properties `hla.rti.tck.configurationName`, `hla.rti.tck.rtiAddress`, and
`hla.rti.tck.additionalSettings`; they default to empty and stay outside the
portable scenario implementation.
The `java-tck.factory-discovery` slice also mirrors the promoted C++
four-octet `HLAinteger32BE` encode/decode check through the discovered standard
encoder factory, the stable non-empty runtime name/version contract, and
repeated standard ambassador construction; the expanded scenario passes on
the local JNI provider.
The `java-tck.transport-order` scenario also checks standard invalid-class,
invalid-attribute, unknown-object, and invalid-transportation boundaries for
default/per-instance order and transportation services, plus unpublished
interaction-order and invalid published-transport requests. It now uses the
adapter-selected callback model and checks the distinct immediate-versus-evoked
transport-change commit behavior for both attributes and interactions.
The federation restore ownership-assumption scenario also preserves pending
object-discovery and ownership-assumption callbacks across a complete save and
restore under both callback models, and checks that ownership is not transferred
before the restored callback is serviced.
The `java-tck.ownership-acquisition-publication-fence` scenario uses the
adapter-supplied multi-attribute FOM from `hla.rti.tck.multiAttributeFom`. It
checks that publication gates acquisition, an If Available request is
superseded by a regular mixed-attribute request, only the owned subset prompts
release, and a pending attribute cannot be unpublished before release denial.
The `java-tck.partial-attribute-ownership-transfer` case uses the
adapter-supplied multi-attribute FOM and logical-time implementation. It cancels
one attribute from a two-attribute acquisition, then verifies that If Wanted
divests and notifies only the remaining requested attribute.
The `java-tck.negotiated-divestiture-partial-acquisition-cancellation` case
uses the adapter-supplied multi-attribute FOM and logical-time implementation.
It confirms negotiated divestiture transfers only the still-requested attribute
after the other acquisition is canceled, with callback payloads checked for
both callback models.
The `java-tck.negotiated-divestiture-pre-delivery-cancellation` case uses the
adapter FOM and logical-time implementation. In the evoked model, it verifies
that canceling before callback servicing suppresses both queued owner callbacks,
then cancels the pending negotiated divestiture and confirms ownership is
unchanged.
The `java-tck.negotiated-divestiture-cancellation` case follows the standard
two-federate acquisition path, checks the confirmation carries the acquisition
tag, cancels the negotiated divestiture, and verifies that the ordinary release
request is restored without changing ownership. It uses byte-array tags and
adapter-supplied federation inputs; provider runtime remains unverified.
The `java-tck.negotiated-willing-to-acquire-continuation` case also uses the
adapter FOM and logical-time implementation. It checks that a canceled first
candidate does not block the second willing candidate from receiving the
negotiated transfer; immediate delivery uses the equivalent regular pending
request path because If Available completes as unavailable immediately.
The `java-tck.resign-pending-acquisition-rejection` case verifies the standard
`OwnershipAcquisitionPending` failure for unconditional resign, confirms the
federate remains joined, then cancels the request through the specified resign
action and checks that evoked cleanup produces no stale owner-release callback.
The `java-tck.resign-cancel-if-available-pending` case resigns with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS` while an If Available request is pending,
then confirms the current owner is unchanged and no unavailable/acquisition
callback remains queued after cancellation.
The `java-tck.resign-cancel-negotiated-pending` case queues a negotiated
divestiture, resigns the requester with the same cancellation action, and checks
after bounded owner callback servicing that neither divestiture-confirmation nor
release-request callbacks remain and ownership stays with the original owner.
The `java-tck.divestiture-if-wanted-mixed-acquirers` scenario uses that same
adapter-supplied FOM. It verifies that If Wanted is a no-op without pending
acquirers, then transfers two attributes to independent requesters with the
exact divestiture tag. With immediate callbacks, If Available is first observed
as unavailable before a regular pending acquisition is used for the transfer.
The `java-tck.ownership-state-save-restore` case completes an ordinary negotiated
transfer before saving, restores that completed owner state, and confirms it
through both `isAttributeOwnedByFederate` and `queryAttributeOwnership`. Cleanup
unconditionally divests the acquirer and leaves the prior owner with `NO_ACTION`,
matching the portable C++ scenario's resign boundary.
The `java-tck.attribute-ownership-acquisition-cancellation` case ports the
ordinary cancellation contract, including pre-request/owned-attribute failures,
the cancellation confirmation metadata, no acquisition notification, and
preserved ownership after cancellation.
The `java-tck.attribute-ownership-release-denied` case verifies the exact
ownership-release request and unavailable callback payloads, unchanged ownership
before and after denial, and the absence of an acquisition notification.
The `java-tck.attribute-ownership-query-owner-report` case checks the exact
`informAttributeOwnership` object, attribute set, and federate handle, then
rechecks that querying did not change ownership.
The `java-tck.ownership-query-partition-cleanup` scenario uses the adapter's
multi-attribute FOM configured with `hla.rti.tck.multiAttributeFom` and the
multi-attribute class/name properties. It checks the owned/unowned callback
partition, invalid object/attribute failures, and that removal suppresses stale
query results.
The `java-tck.ownership-query-save-restore-results` scenario uses the
adapter-supplied multi-attribute FOM configured with
`hla.rti.tck.multiAttributeFom` and the existing multi-attribute class/attribute
name properties. It saves an owner/unowned split, changes the live owner,
restores the saved state, then checks federate-owned, unowned, and RTI-owned
MOM query results through the standard callbacks.
The `java-tck.ownership-acquisition-cancellation-transfer-race` case uses the
adapter's ordinary object class and attribute. With evoked callbacks, it
cancels while the owner's release callback is active, then checks the exact
transfer notification and absence of a stale cancellation confirmation.
Immediate callbacks verify the transfer result without a concurrent
cancellation. Both modes passed focused JNI runs; this is not an
independent-provider or conformance claim.
The separate `java-tck.ownership-acquisition-cancellation-denial-race` case
checks the exact unavailable reply when the owner denies release after the
requester's cancellation call completes. It runs with evoked callbacks only;
immediate mode reports unsupported because it cannot express this callback
boundary race. It does not claim the three-reply aggregate obligation.
The opt-in `java-tck.ownership-acquisition-cancellation-mixed-replies`
candidate translates the promoted C++ aggregate: one cancellation call spans
three adapter-named attributes and expects disjoint cancellation, acquisition,
and unavailable callbacks. It requires evoked callbacks and the adapter's
three-attribute FOM (`hla.rti.tck.attribute`,
`hla.rti.tck.ownershipMixedTransferredAttribute`, and
`hla.rti.tck.ownershipMixedDeniedAttribute`). The local JNI gate currently
receives the acquisition and unavailable replies but not the cancellation
confirmation, so this Java case remains disabled by default.
The `java-tck.auto-provide` case verifies the configured standard Auto Provide
switch and two object discoveries, first with Attribute Scope Advisory disabled
and then enabled. Each discovery yields one provider callback carrying the same
two subscribed attributes and an empty tag under either callback model.
The `java-tck.auto-provide-disabled-explicit-request` case verifies discovery
continues with Auto Provide disabled, while explicit object- and class-scoped
value requests each produce one callback preserving their supplied tag.
The companion discovery-only case verifies that disabling Auto Provide
suppresses only the provider solicitation; normal object discovery still arrives.
The inherited-attribute projection case checks base and derived subscriptions
against a derived object update, including handle stability, identity round-trips,
callback metadata, and the exact inherited/derived attribute sets.
The `java-tck.delay-subscription-evaluation-interaction` case checks the standard
switch with a composed adapter FOM, callback-boundary delivery after subscription,
and suppression when an interaction is unsubscribed before queued callbacks are
serviced. Supply the switch declaration module with
`-Dhla.rti.tck.switchesFom=<adapter FOM>`; no provider-specific API is used.
The companion `java-tck.delay-subscription-evaluation-attribute-update` case
checks known-object discovery, late ordinary attribute subscription, callback
metadata, and suppression after unsubscribe under the same standard switch.
The timestamped attribute counterpart evaluates the same declaration change at
the constrained time-grant boundary and checks timestamp, order, retraction, and
delivery metadata. It uses no DDM regions.
The timestamped interaction counterpart checks delivery after late subscription
at the constrained grant and confirms unsubscribe suppresses a later queued
interaction, including its timestamp and retraction metadata.
The ordinary directed-interaction counterpart exercises by-ownership target
selection, late subscription, and callback-boundary unsubscribe suppression;
it uses no DDM regions.
Its timestamped counterpart applies the same selector checks at constrained
time grants and verifies target, timestamp, order, retraction, and producer
metadata, also without DDM.
The focused `java-tck.asynchronous-delivery` scenario gates receive-order
attribute reflection while asynchronous delivery is disabled, then verifies
release on re-enablement, callback servicing in evoked mode, and the repeated
disable/re-enable boundary. It uses only the 2025 Java API and remains
unsupported by default until run against an adapter-supplied Java provider.
The opt-in `java-tck.update-rate-queries` scenario checks adapter-defined named
rate lookup, active and passive named subscriptions, default-rate behavior,
per-attribute rate reset, and invalid designator/object/attribute failures.
The FOM, object class, attribute, update-rate designator, provider, and callback
model are adapter-supplied; the scenario uses only the official Java 2025 API
and remains unverified against a Java provider runtime.
The opt-in `java-tck.fom-transportation-handle-stability` scenario verifies
that built-in and custom transportation handles survive an additional-module
join, while the newly introduced transportation type becomes available with
one shared handle and round-trips by name. The rich base FOM, extension module,
transportation names, provider, and callback model are adapter-supplied; Java
provider runtime verification remains pending.
The opt-in `java-tck.fom-module-composition` scenario composes the adapter's
base and extension modules both at federation creation and at a later join,
then verifies extension object/attribute and interaction/parameter declaration
identity and ordinary delivery. All module paths and extension names are
adapter-supplied; Java provider runtime verification remains pending.
The federate MOM publication-query cases check ordinary object and interaction
publication reports, including partial/full attribute withdrawal and whole-class
withdrawal. The opt-in partial interaction-class withdrawal case publishes two
adapter-named ordinary classes, unpublishes the secondary, and checks that the
fresh MIM report contains only the retained primary class. Supply the second
class with `--jvm-argument=-Dhla.rti.tck.secondaryInteractionClass=<class>`;
FOM, MIM, names, provider, and callback model remain adapter supplied.
The companion full-withdrawal case removes both classes and checks the exact
MIM NULL response with one empty interaction-class list.
The multi-directed-publication query publishes adapter-selected directed
interactions for two object classes and verifies both standard MIM report rows,
their object-class identities, interaction lists, and total class count. Supply
the second object class with
`--jvm-argument=-Dhla.rti.tck.secondaryDirectedObjectClass=<class>`; the second
directed interaction uses `hla.rti.tck.directedInteractionClass`.
The partial-unpublication companion removes the inherited interaction only from
the derived class and verifies that its class-specific interaction and the
base-class inherited publication remain in the two MIM report rows.
The full-unpublication case first verifies that partial state, then removes all
directed publications from both classes and checks the exact zero-count NULL
report while ordinary publications remain unchanged.
The matching directed-subscription query subscribes to one adapter-selected
interaction under each object class, verifies both inactive standard MIM rows,
then set-unsubscribes the primary pair and verifies only the secondary row
remains. It also checks the unchanged NULL reports for ordinary subscriptions.
Its full-unsubscribe companion preserves both earlier query states, removes the
remaining class subscription with the class-scoped standard API, and checks the
exact two-parameter directed NULL response (zero class count and empty list)
alongside unchanged ordinary subscription NULL reports.
The full interaction-unsubscription companion removes the remaining active
application class after the passive MIM class, then checks the empty interaction
list NULL report while object and directed subscription reports remain present.
The passive-to-active activation case checks the interaction-list state before
and after activation while preserving the active object-attribute report and
empty directed-subscription response. Supply the adapter's rate FOM with
`-Dhla.rti.tck.updateRateFom=<FOM>` and its designator with
`-Dhla.rti.tck.updateRateDesignator=<name>`.
The complete object-class subscription case requests three distinct
adapter-configured attributes and verifies that exact active set and rate in
the standard MIM report; configure the second and third attributes with
`hla.rti.tck.objectClassSecondAttribute` and
`hla.rti.tck.objectClassThirdAttribute`.
The ordinary subscription-lifecycle scenario also requires a distinct second
attribute through `hla.rti.tck.objectClassSecondAttribute`; it publishes and
updates both attributes while subscribing to only the primary one, checking
that initial and subsequent reflection contain only the subscribed value.
The ownership-acquisition-if-available scenario uses a locally deleted object
as its unknown-instance fixture and a valid attribute from another class for
its undefined-attribute boundary. The adapter may set
`hla.rti.tck.foreignAttributeClass` and `hla.rti.tck.foreignAttribute`; their
defaults are the standard MIM `HLAobjectRoot.HLAmanager.HLAfederate` class
and `HLAfederateHandle` attribute. The ownership-query partition/cleanup case
uses the same foreign-attribute settings for its undefined-attribute boundary
and queries the real object handle after removal for its unknown-object boundary.
The ownership-query save/restore case uses the adapter's multi-attribute FOM,
logical-time implementation, and callback model. It joins with explicit
federate names so restored identity reports can be checked exactly.
The default `java-tck.attribute-subscription-invocations-accumulate` case
subscribes to the configured primary attribute, waits for discovery, then
subscribes to the distinct configured second attribute and verifies one update
reflects both exact values and tag using the subscriber's locally resolved
standard handles. Its FOM and names come from the adapter. Focused installed
JNI runs passed in both callback modes; active/passive mode replacement and
independent-provider conformance remain unverified.
The default `java-tck.attribute-subscription-per-attribute-mode-transitions`
case keeps both attribute subscriptions while switching their active/passive
modes, including empty active and passive calls. It checks each reflected
attribute set, value, and tag against receiver-local standard handles. The
adapter supplies the FOM, logical-time implementation, names, provider,
endpoint, and callback model. Focused installed JNI runs passed in both modes;
only the C++ plan row's page-96 line-129 requirement is selected here, pending
reconciliation of the wider C++ catalog mapping. Independent-provider
conformance remains unverified.
The default `java-tck.passive-subscription-no-start-registration` case enables
the publisher's standard object-class relevance advisory switch if needed,
then uses an active subscription as a positive control for Start Registration.
It clears that control and verifies that a passive subscription produces no new
Start Registration callback. It does not claim a Stop Registration effect.
Focused installed JNI runs passed in both callback models; independent-provider
conformance remains unverified.
The default `java-tck.object-publication-registration-fence` case rejects
registration while a class is unpublished, then republishes it and confirms
registration, discovery, and instance-name/class lookups. Discovery identity
uses the subscriber's own standard object handle, resolved from the publisher's
object name. Focused installed JNI runs passed in both callback models;
independent-provider conformance remains unverified.
The default `java-tck.selective-attribute-unsubscription` case first confirms
that two federates independently receive both attributes, removes only the
configured primary attribute for one, and verifies that its secondary value
continues while the other federate still receives both. It uses receiver-local
handles through only the standard Java API. Focused installed JNI runs passed
in both callback modes; independent-provider conformance remains unverified.
The default `java-tck.whole-class-attribute-unsubscription` case confirms both
configured values and the tag are initially reflected, invokes the standard
whole-class unsubscribe service, and verifies that later updates produce no
reflection. It resolves the receiver's local object and attribute handles and
passed focused installed JNI runs in both callback modes; independent-provider
conformance remains unverified. The default
`java-tck.empty-attribute-unsubscription-is-no-op` case submits an empty set
to the standard attribute-set unsubscribe service and verifies both previously
subscribed values continue to arrive using receiver-local handles. Focused
installed JNI runs passed in both callback modes; the passive-state branch
and independent-provider conformance remain unverified.
The default `java-tck.empty-attribute-subscription-is-no-op` case creates an
active non-empty attribute subscription, submits an empty active set, and
verifies the original subscription still delivers the next tagged update using
the subscriber's local standard handles. The adapter supplies the FOM, standard
MIM, object class/attribute names, provider, logical-time implementation,
endpoint, and callback model. Focused installed JNI runs passed in both modes;
passive-state preservation and independent-provider conformance are unverified.
The passive companion uses the standard passive attribute-set subscription
operation and verifies that the same complete attribute list and rate are
reported with the active flag false.
The complete-set activation case first verifies that passive report, then uses
the standard active subscription call and requires the next MIM report to retain
all three attributes and the rate designator with active=true.
The partial attribute-unsubscribe case starts with two active attributes, removes
the first through the standard attribute-set service, and verifies that only the
second remains with the same active state and update-rate designator.
Its full-unsubscribe companion removes both attributes in one standard set call,
then checks the zero-class object-subscription NULL response and unchanged
interaction and directed-subscription NULL reports.
The multiple-interaction-subscription case checks an active adapter-selected
application interaction and a passive standard MIM service-report interaction
in the same report, plus the unchanged empty object and directed subscription
NULL reports.
The companion staged-unsubscribe case removes the passive service-report class,
queries again, and verifies the active application interaction is the sole
remaining interaction subscription while the other two NULL reports remain.
The opt-in `java-tck.fom-additional-module-join-atomicity` scenario retries a
member join with each adapter-supplied invalid FOM input, verifies the failure
is a standard FOM-loading/rejection exception and that membership/declarations
remain unchanged, then checks that a valid extension join succeeds. Supply
invalid paths through `hla.rti.tck.invalidFomModules`, separated by the
platform path separator.
The opt-in `java-tck.fom-update-rate-value-stability` scenario verifies an
existing named update rate retains its value across a later FOM join and that
the new member resolves the same positive value for an extension rate. The rate
FOM, extension module, and both rate names are adapter-supplied.
The opt-in `java-tck.fom-invalid-create-atomicity` scenario retries federation
creation with each adapter-supplied invalid FOM path, then creates and joins a
valid federation using the same name to prove failures did not reserve it.
The opt-in `java-tck.fom-invalid-composite-join-atomicity` case passes each
invalid module together with the valid extension module, verifies atomic
rejection, then confirms the valid join works after all failures.
The opt-in `java-tck.fom-invalid-mim-create-atomicity` scenario exercises the
standard `createFederationExecutionWithMIM` overload with each adapter-supplied
invalid MIM path, then reuses the federation name with valid FOM/MIM inputs.
The opt-in `java-tck.fom-invalid-composite-mim-create-atomicity` scenario
combines each adapter-supplied invalid FOM module with the valid FOM/MIM inputs,
checks standard FOM rejection, and then proves a valid MIM-backed create under
the same federation name succeeds. The opt-in
`java-tck.fom-empty-mim-create-atomicity` case requires an empty FOM module array
to fail with `InvalidFOM`, then retries with valid FOM/MIM inputs under the same
federation name and checks both members' handles.
The opt-in `java-tck.fom-empty-module-validation` case performs the same
name-reuse check using standard Create Federation Execution without MIM.
The opt-in `java-tck.fom-standard-invalid-input-corpus` case accepts a
platform-path-separator-delimited adapter corpus of semantic-invalid FOM files;
each rejected input is followed by valid create/join/destroy using the same
federation name.
The opt-in `java-tck.fom-dimension-handle-stability` case checks base and
extension dimension metadata and object/interaction-class associations across
FOM composition. It uses no DDM region services; the FOMs, dimensions, and class
names are adapter-provided.
The
directed derived-object target scenario uses adapter-selected typed model-FOM
names and verifies typed parameter delivery only at the registered target,
excluding an observer without a directed subscription. It remains unsupported
by default pending execution against a Java provider.

The opt-in `java-tck.custom-transportation-directed-interaction-delivery` case
uses adapter-selected object/interaction/parameter names and a FOM-defined
transportation type to check targeted and universal directed delivery, including
target, value, tag, producer, and transportation metadata. It remains unsupported
by default until exercised against a Java provider.

The opt-in `java-tck.support-lookups-valid-inputs-after-resignation` case first
proves adapter-supplied class/member handles round-trip while joined, then checks
that the corresponding eight handle/name lookups report
`FederateNotExecutionMember` after resignation.

The opt-in `java-tck.update-rate-lookups-valid-inputs-after-resignation` case
uses adapter-supplied rate FOM inputs to verify both named and
object-attribute update-rate lookups while joined and their membership errors
after resignation.

The opt-in `java-tck.timestamped-attribute-update-rate-reduction` case checks
timestamped best-effort suppression while reliable attribute updates continue
to arrive. Its adapter supplies `hla.rti.tck.updateRateFom`,
`hla.rti.tck.updateRateObjectClass`,
`hla.rti.tck.updateRateReliableAttribute`,
`hla.rti.tck.updateRateBestEffortAttribute`, and
`hla.rti.tck.updateRateDesignator`, along with the provider's standard logical-
time and callback configuration.

The default `java-tck.default-rate-subscription-no-reduction` case omits the
optional update-rate designator, sends three distinct ordinary updates before
servicing an evoked callback, and verifies that every original value and tag is
reflected on the receiver's locally known object without assuming callback
order. Its FOM, object class, attribute, endpoint, callback model, and logical
time are adapter-supplied. Focused installed JNI execution passed in both
callback modes; independent-provider conformance remains unverified.

The opt-in `java-tck.mixed-update-rate-subscriptions` case uses the same
adapter inputs to verify ordinary receive-order updates are independently
gated: the first passel includes both attributes, while the next still
delivers the default-rate reliable attribute without repeating the named-rate
best-effort attribute.

The opt-in `java-tck.attribute-relevance-rate-reissue` case verifies owner-side
Turn Updates On reissues as the maximum active subscription rate changes among
two subscribers. It uses the shared rate FOM/object/attribute properties plus
`hla.rti.tck.updateRateFastDesignator`; the adapter supplies both distinct
designators and their positive rate ordering.

The opt-in `java-tck.standard-order-transport-lookups-after-resignation` case
checks the mandatory `Receive`/`TimeStamp` orders and `HLAreliable`/
`HLAbestEffort` transportation types, including their membership errors after
resignation.

The opt-in `java-tck.support-switch-getter-lifecycle-boundaries` case exercises
all thirteen standard support-switch getters while joined, after resignation,
and after disconnect, checking the corresponding lifecycle exceptions.

The opt-in `java-tck.support-switch-setter-lifecycle-boundaries` case reads and
reapplies each writable switch's current value while joined, then verifies all
eight setters report the expected membership and connection errors.

The opt-in `java-tck.resign-unconditional-divestiture` case verifies that
`NO_ACTION` is rejected while attributes remain owned, then checks the standard
unconditional-divestiture assumption and the surviving peer's tagged acquisition,
including the implicit `HLAprivilegeToDeleteObject` attribute.

The default `java-tck.final-federate-resignation-cleanup` case verifies that a
final member's `NO_ACTION` resignation removes its registered object and frees
the former object name for reservation and named registration after rejoining.
It uses only the standard Java API and adapter-supplied class, attribute, FOM,
logical time, provider, endpoint, and callback model. Focused installed JNI
runs passed in both callback modes; independent-provider conformance is unverified.

The default `java-tck.object-name-reservation-lifecycle` case exercises legal
and illegal single-name reservations, contention and release, mixed multiple-name
success/failure callbacks, atomic release, and name reuse after resignation.
Its FOM, logical time, provider, endpoint, and callback model come from the
adapter. Focused installed JNI runs passed in both callback modes; this is not
independent-provider conformance evidence.

The `java-tck.federation-membership` scenario also covers the unnamed
two-argument join overload: it verifies that the RTI-assigned federate name
round-trips through handle lookup and appears with the requested federate type
in the standard member-report callback. It also verifies both immediate and
evoked federation-list callback behavior across two live executions, including
that disconnect discards an unserviced evoked report. It distinguishes a
never-created federation from a destroyed federation in member-list callbacks.

The focused `java-tck.duplicate-federate-name-join-boundary` scenario checks
that a second ambassador receives the standard `FederateNameAlreadyInUse`
exception for an occupied name, can then join under a distinct name, and can
resolve both federate handles and names through the standard API. Its ID is
paired with the equivalent portable C++ scenario.

The focused `java-tck.repeated-join-by-member-boundary` scenario checks that
an ambassador already joined to an execution receives
`FederateAlreadyExecutionMember` when it tries to join that execution again,
and that the original membership remains queryable afterward.

The focused `java-tck.foreign-federate-name-lookup-boundary` scenario creates
two executions and verifies that each member name resolves only within its
own execution, with `NameNotFound` for the foreign execution's name.

The focused `java-tck.foreign-object-instance-name-lookup-boundary` scenario
reserves and registers one named object in each execution, verifies local name
round-trips, and expects `ObjectInstanceNotKnown` when the local member looks
up the foreign execution's object name.

The focused `java-tck.foreign-object-instance-handle-lookup-boundary` case
checks execution scope in the reverse direction: it selects a foreign object
handle that cannot be confused with the local handle and expects
`ObjectInstanceNotKnown` when the local member queries it.

The focused `java-tck.local-delete-object-instance-name-lookup-boundary`
case checks that a member forgets a discovered object locally and can no longer
resolve its handle, while the publishing member retains its object name until
the normal deletion service is called.

The default `java-tck.local-delete-object-instance` case checks owner-attribute
protection, recipient-local forgetting and rediscovery, and continued ordinary
reflection. It resolves each recipient's object by name, without comparing
opaque handles across federates or fabricating invalid handles. The focused
installed JNI gate passed in both callback modes; provider, FOM, endpoint,
and callback model remain adapter-supplied. Independent-provider conformance
remains unverified.

The focused `java-tck.remote-delete-known-object-class-lookup-boundary` case
checks that the deleting member loses known-class lookup immediately, the
other member receives the standard removal callback, and that member then
also receives `ObjectInstanceNotKnown` for the removed handle.

The `java-tck.explicit-mim-creation` scenario runs by default using only the
standard `createFederationExecutionWithMIM` API. The adapter supplies the MIM
file (`-Dhla.rti.tck.mim=...`), caller FOM, and the FOM's object, attribute,
interaction, and parameter names. It checks rejection of the reserved
standard-MIM designator, shared MOM handles/names across federates, and
continued lookup of the caller's declarations. The bundled P0 FOM uses
`Payload` as the `TckInteraction` parameter name. Both callback models passed
focused JNI runs; this is not independent-provider or conformance evidence.

The `java-tck.federation-management-requires-connection` case checks that
Create, Destroy, Join, and Resign reject calls before Connect. The companion
`java-tck.connect-before-federation-disconnect-after-resign` case checks Create
both before Connect and after a successful Join, Resign, and Disconnect.
The `java-tck.standard-exception-boundaries` case checks the portable Java
subset of standard service exceptions, including disconnected federation
operations, ownership-request boundaries, delete privilege, and duplicate
object names. Both callback models passed focused JNI runs. C++ invalid-enum
and cross-implementation logical-time inputs are not representable through the
standard Java API and are not claimed as Java parity.

The opt-in `java-tck.federation-mom-current-fdd` scenario consumes the
adapter-supplied FOM, standard MIM, and an additional extension FOM
(`-Dhla.rti.tck.additionalFom=...`). It checks standard federation-MOM
discovery, reliable transportation and `HLAcurrentFDD` reflection, then
verifies that an additional-module join refreshes the reflected FDD and that a
subsequent explicit value request returns the same composition.

The opt-in `java-tck.federation-mom-fom-module-designator-list` scenario uses
the adapter-supplied base FOM and MIM plus one or more additional FOM modules
(`-Dhla.rti.tck.additionalFomModules=...`, paths separated by the platform
path separator). It decodes the standard `HLAFOMmoduleDesignatorList` as a
variable array of Unicode strings, compares designators without assuming array
order, and checks that the list changes after an additional-module join and
matches a later explicit request. The callback assertion also checks the
standard transport, tag, producer, and DDM-region metadata.

The opt-in `java-tck.federate-mom-fom-module-content-report` scenario requests
one joined federate's FOM module through the standard federate MOM. The adapter
supplies the base FOM, standard MIM, additional FOM module, logical-time
implementation, callback model, and federate type. The test verifies the
standard request/report handles and parameter names, reliable report delivery,
the selected module indicator, and returned FOM XML content.

The opt-in `java-tck.federation-mom-static-identity` scenario consumes the
adapter-supplied FOM, standard MIM, logical-time implementation, and callback
model. It checks standard federation-MOM handle/name round trips and object
identity, then decodes the federation name, RTI version, MIM designator, and
logical-time implementation from the ordinary identity reflection.

The opt-in `java-tck.federation-mom-static-switches` scenario uses the same
adapter-supplied FOM, standard MIM, logical-time implementation, and callback
model. It checks the standard federation MOM's four switch attributes and
requires each decoded `HLAinteger32BE` value to be the standard Disabled (0) or
Enabled (1) enumerator; it does not assume a provider-specific default value.

The opt-in `java-tck.federation-mom-federates-in-federation` scenario uses the
adapter-supplied FOM, standard MIM, logical-time implementation, and callback
model. It decodes the standard nested `HLAfederateReferenceList` encoding with
the official encoder API and federate-handle factory, treats roster order as
unspecified, and verifies requested membership plus conditional updates after
a federate joins and resigns.

The opt-in `java-tck.federation-mom-content-reports` scenario requests the
standard FOM-module and MIM content reports using the adapter-supplied FOM,
MIM, logical-time implementation, and callback model. It decodes the report
parameters with the official `HLAinteger32BE` and `HLAunicodeString` encoders,
and checks interaction identity, empty tags, reliable transportation, and
ordinary receive-order delivery. The scenario has a matching C++ portable TCK
ID and remains unsupported by default until verified with a Java provider.

The default `java-tck.object-registration-discovery-lifecycle` scenario checks
typed base/derived-class lookup, inherited attribute identity, publication,
unnamed registration, exact and base-class discovery, evoked callback timing,
unsubscribe cancellation, and late subscription. It uses only the standard
IEEE 1516.1-2025 Java API; the adapter supplies its provider, FOM, endpoint,
logical time, callback model, and typed class/attribute names. It compares
object and class handles only within each federate, without fabricated invalid
handles. Focused installed JNI execution passed in both callback modes;
independent-provider conformance remains unverified.

The default `java-tck.overlapping-object-class-subscriptions-single-discovery`
scenario registers a derived-class object while one federate actively subscribes
to both the base and derived classes. It requires exactly one discovery reporting
the registered class, then verifies receiver-local name and handle lookups. The
adapter supplies the typed FOM, logical time, provider, endpoint, and callback
model. Focused installed JNI runs passed in both callback modes; this is not an
independent-provider conformance result.

The default `java-tck.inherited-object-attribute-projection` case checks that
base and derived subscribers discover a derived registration under their
respective subscribed classes, then receive only their subscribed inherited and
derived attribute values. It resolves names and handles within each federate,
uses the adapter's typed FOM and logical time, and passed focused installed JNI
runs in both callback modes. Independent-provider conformance is unverified.

The default `java-tck.attribute-value-request-service-boundaries` case covers
ordinary instance- and class-scoped value requests, owner callbacks with request
tags, and the subsequent update/reflect responses. It uses each federate's own
object and attribute handles and the adapter's object class, attribute, FOM,
logical time, provider, endpoint, and callback model. Fabricated invalid-handle
checks are excluded from this portable Java subset. Focused installed JNI runs
passed in both callback modes; independent-provider conformance is unverified.

The opt-in `java-tck.object-attribute-declarations-valid-inputs-after-resignation`
scenario uses adapter-supplied object-class and attribute names and checks all six
ordinary object-attribute publication/subscription calls while joined and after
resignation. The calls succeed while joined and report `FederateNotExecutionMember`
after resignation. It remains unsupported by default until run against a Java provider.

The default-run `java-tck.attribute-value-update-request-baseline` scenario uses an
adapter-supplied multi-attribute FOM plus the `hla.rti.tck.multiAttributeObjectClass`,
`hla.rti.tck.multiAttributeFirst`, and `hla.rti.tck.multiAttributeSecond` properties
when its names differ from the TCK defaults. It checks discovery, instance-scoped
requests, the provider callback's object/attribute set/tag, and suppression when
the requester owns or no federate owns the requested instance attributes. Provider,
endpoint, callback model, and FOM remain adapter-configured. It passed the focused
JNI gate in both `HLA_EVOKED` and `HLA_IMMEDIATE`; this does not establish
independent-provider conformance.

The default-run `java-tck.object-class-attribute-value-update-request-baseline` scenario
uses the adapter's typed base/derived FOM and the
`hla.rti.tck.typedIntegerAttribute` property when the integer attribute differs from
the default. It requests inherited identity and base integer attributes at the base
class, then checks one exact callback per derived instance, inherited-attribute
filtering, request-tag propagation, and suppression for the requester's own instance.
It uses no region services and passed the focused JNI gate in both `HLA_EVOKED`
and `HLA_IMMEDIATE`. This does not establish independent-provider conformance.

The default-run `java-tck.attribute-value-update-response` scenario uses the adapter's
ordinary object-class and attribute names (`hla.rti.tck.objectClass` and
`hla.rti.tck.attribute`). It verifies discovery and request-tag
delivery, then has the owner answer with standard `updateAttributeValues` and checks
the reflected value/tag, reliable transportation, producer, and absence of regions.
Provider, FOM, endpoint, logical-time implementation, and callback model remain
adapter-configured. It passed the focused JNI gate in both `HLA_EVOKED` and
`HLA_IMMEDIATE`; this does not establish independent-provider conformance.

The default-run `java-tck.attribute-value-update-request-multi-requester` scenario
uses the adapter's ordinary object-class and attribute names (`hla.rti.tck.objectClass`
and `hla.rti.tck.attribute`). Two independent
subscribers issue instance-scoped requests with distinct tags; the provider must
receive exactly one matching callback for each, and one ordinary update must be
reflected to both requesters with the expected value and standard receive-order
metadata. It passed the focused JNI gate in both `HLA_EVOKED` and `HLA_IMMEDIATE`;
this does not establish independent-provider conformance.

The default-run `java-tck.object-registration-discovery-multi-recipient` scenario
registers two ordinary objects and verifies that two active subscribers each
receive exactly one discovery per object, with matching producer/class metadata
and stable name/handle/known-class lookups. Provider, FOM, endpoint, and callback
model remain adapter-configured. The shared helper also supports default-run
`java-tck.named-registration-multi-recipient`, which reserves both names before
registration. Both passed the focused JNI gate in `HLA_EVOKED` and `HLA_IMMEDIATE`
without assuming discovery order or cross-federate equality of object, class, or
attribute handles. This does not establish independent-provider conformance.

The default-run `java-tck.ordinary-multi-attribute-subscription-projection` scenario
uses an adapter-supplied multi-attribute FOM and attribute names. It verifies the
complete update for a two-attribute subscriber, selective value projection for
single-attribute subscribers, preservation of per-update tags and producer
metadata, and suppression of updates for an unsubscribed attribute and the
publisher. It passed the focused JNI gate in both callback models using an
adapter-supplied two-attribute FOM. It remains internal API-contract evidence
without a direct canonical requirement mapping or independent-provider claim.

The default-run `java-tck.subscription-initial-reflection-projection` scenario
subscribes to an existing object and checks the current first-attribute value,
then a later two-attribute update. Only the subscribed attribute may appear in
either reflection. The adapter supplies the provider, FOM, names, and callback
model; both callback modes passed the focused JNI gate. This is direct §5.8
parity evidence, not an independent-provider conformance claim.

The default-run `java-tck.active-subscription-object-discovery` scenario checks
discovery of one object registered before subscription and one registered
afterward, each exactly once. It observes no discovery before subscription and
checks each discovered object's identity within the subscriber. The adapter
supplies the provider, FOM, class/attribute names, and callback model. Both
callback modes passed the focused JNI gate; only the recorded §5.8 assertions
are direct standard mappings.

The opt-in `java-tck.time-constrained-enable-galt-equality` scenario compiles
against the pinned IEEE 1516.1-2025 Java API. It checks that constrained-enable
notification follows the GALT equality boundary and that a later advance is
held at the regulating peer's GALT. Its paired C++ installed-package case
passes both callback models; Java provider-runtime execution remains pending.

The opt-in `java-tck.time-constrained-disable-queued-interaction-to-ro` scenario
uses only the standard API and adapter-supplied FOM, logical-time implementation,
and callback model. It checks the untimed receive-order callback for a queued
future interaction after disabling Time Constrained, then the receive-order
metadata of a subsequent timestamped send. The paired C++ installed-package
case passes both time implementations and callback models. Java compiles against
the official API jar but remains unsupported by default until provider-runtime
verification.

The opt-in `java-tck.time-constrained-disable-queued-attribute-to-ro` scenario
uses only the official API and adapter-supplied FOM, logical-time implementation,
and callback model. It checks untimed receive-order reflection of a queued
future update after disabling Time Constrained, then the receive-order metadata
of a subsequent timestamped update. The paired C++ installed-package case
passes both time implementations and callback models. Java compiles against
the official API jar but remains unsupported by default until provider-runtime
verification.

The opt-in `java-tck.time-constrained-disable-queued-deletion-to-ro` scenario
uses two adapter-FOM objects to check the untimed receive-order removal of a
queued future deletion after disabling Time Constrained, followed by a separate
timestamped deletion whose received order is RECEIVE. Its C++ pair passes both
time implementations and callback models. Java compiles against the official
API jar but remains unsupported by default until provider-runtime verification.

The opt-in `java-tck.time-constrained-disable-queued-directed-to-ro` scenario
checks that a queued future directed interaction arrives through the untimed
receive-order callback after disabling Time Constrained, then checks a later
timestamped directed send retains its time with receivedOrder=RECEIVE. Its C++
pair passes both time implementations and callback models. Java compiles against
the official API jar but remains unsupported until provider-runtime verification.

The opt-in `java-tck.service-report-associate-regions-for-updates` scenario
checks the standard Table 5 `AttributeRegionAssociation` report value for one
successful `AssociateRegionsForUpdates` call. Supply `hla.rti.tck.fom`,
`hla.rti.tck.modelFom`, and `hla.rti.tck.mim` as adapter FOM/MIM paths, plus
the matching typed object/attribute, dimension, time, and callback settings.
Its C++ primary and contract pass the focused installed-package gate with
`HLAinteger64Time`; Java compiles against the official API jar but awaits
provider-runtime verification.

The default-run `java-tck.ordinary-multi-attribute-value-update-request-response`
scenario uses the adapter-supplied multi-attribute FOM and names. It exercises
full, first-only, and second-only instance requests, checks the provider callback
attribute sets and request tags, and verifies each ordinary response's values,
tag, producer, reliable transportation, and receive-order metadata. Both callback
modes passed the focused JNI gate with adapter-supplied FOM/names. The direct
standard claims are limited to §§6.21, 6.21.5, 6.22, and 6.10.

The opt-in `java-tck.regional-multi-attribute-update` scenario uses an
adapter-supplied multi-attribute FOM configured with
`hla.rti.tck.multiAttributeFom`, the existing multi-attribute object/attribute
name properties, and two dimensional names. It independently associates each
published attribute with a different source region, verifies projection to two
subscribers, mutates each source range to test independent filtering, then
restores overlap and checks delivery resumes with the correct source-region
metadata. It remains unsupported by default pending Java-provider runtime
verification.

The opt-in `java-tck.allow-relaxed-ddm` scenario composes the adapter-supplied
dimensional DDM FOM with and without its switch-declaration FOM. Across both
object and interaction paths, it verifies that regions touching at their
boundary are filtered in strict mode and admitted when the standard
`getAllowRelaxedDDMSwitch` reports enabled, including source-region callback
metadata. Configure the object, attribute, interaction, parameter, and two
dimension names through the adapter. Provider runtime verification remains
pending.

The opt-in `java-tck.regional-three-dimensional-overlap` scenario uses an
adapter-supplied three-dimensional FOM configured with
`hla.rti.tck.threeDimensionalFom`, the existing DDM object/attribute names, and
the first, second, and `hla.rti.tck.ddmThirdDimension` dimension names. It
verifies discovery and reflection with complete three-axis overlap, suppresses
reflection when each source axis is made disjoint in turn, then confirms
delivery resumes after restoring overlap. It remains unsupported by default
pending Java-provider runtime verification.

The opt-in `java-tck.regional-declaration-relevance-advisories` scenario uses
the adapter-supplied DDM FOM and the configured object class, attribute,
interaction class, and two dimensions. It verifies passive regional object and
interaction subscriptions produce no relevance advisory, active declarations
produce the corresponding start-registration and turn-interactions-on callbacks,
and unsubscription produces the matching stop/off callbacks. Runtime
verification against an installed Java provider remains pending.

The opt-in `java-tck.ownership-transfer-regional-update` scenario uses the
adapter-supplied dimensional DDM FOM and configured object, attribute, and
dimension names. It transfers an attribute to a new owner, checks that the
former owner's regional update association is not inherited, then explicitly
associates the new owner's region and verifies the new source-region designator.
It covers both callback models and remains pending Java-provider runtime
verification.

The opt-in `java-tck.ownership-transfer-deferred-regional-update` scenario
stages the new owner's region association before transfer. It proves the current
owner's updates continue to carry the current owner's region, then transfers the
attribute and verifies the staged region becomes the new update source without
a second association call. Provider runtime verification remains pending.

The opt-in `java-tck.attribute-scope-advisories` scenario checks regional
scope-in/scope-out callbacks caused by source reassociation and active
subscribe/unsubscribe transitions, confirms the standard attribute-scope switch
suppresses both directions, and tests stale in-scope callback suppression when
the region becomes disjoint before EVOKED callbacks are serviced. It uses the
adapter-supplied DDM FOM and names; provider runtime verification remains
pending.

The default `java-tck.object-removal-multi-recipient-fifo` scenario deletes two
ordinary objects and verifies that each of two subscribers receives exactly one
removal per locally known object with the matching tag and producer metadata.
The historical ID preserves C++ parity; Java does not require removal order.
Provider, FOM, endpoint, callback model, and logical time remain adapter-configured.
It passed the focused JNI gate in both callback modes through the standard
1516.1-2025 API. This does not establish independent-provider conformance.

The default `java-tck.object-registration-service-boundaries` scenario checks
ordinary registration, active-subscriber discovery, name and known-class
lookups, recipient-local deletion and rediscovery, then ordinary reflection.
Reservation and local-delete negative boundaries use standard API arguments;
the case does not fabricate invalid provider handles or compare object handles
across ambassadors. It passed focused JNI execution in both callback modes
with provider, FOM, endpoint, and callback model supplied by the adapter.
Independent-provider conformance remains unverified.

The default `java-tck.object-deletion-service-boundaries` scenario checks
pre-connection and pre-membership lookup failures, object name/handle round
trips, local deletion without remote callback, ordinary removal tag/producer
metadata, post-deletion lookup and former-owner ownership-status failures, and
DeleteObjects resignation cleanup.
Its C++ base/contract pair passed two independent fresh GNU 15.2/Ninja
installed-package builds with all four evoked/immediate CTest cases passing in
each build. The Java case also passed focused installed JNI execution in both
callback modes. It uses receiver-local object handles and avoids fabricated
unknown-object handles; independent-provider conformance remains unverified.

The default `java-tck.attribute-multi-recipient-fifo` scenario sends two ordinary
updates and verifies that both subscribers receive each value and matching tag,
with their locally known object, producing federate, and transportation metadata.
The publisher receives no reflection. The historical ID preserves C++ parity;
Java imposes no sender-order assertion without a direct standard mapping.
Provider, FOM, endpoint, and callback model remain adapter-configured. Focused
installed JNI execution passed in both modes; independent-provider conformance
remains unverified.

The default `java-tck.receive-order-attribute-update` scenario checks adapter-name
handle round trips, the evoked callback-servicing boundary, and two distinct
ordinary updates with exact receiver-local object, attribute, value, tag,
producer, receive-order, and transportation metadata. It does not require
sender-order callback delivery without a direct standard mapping. The publisher
receives no reflection. It passed focused installed JNI execution in both
callback modes; independent-provider conformance remains unverified.

The opt-in `java-tck.interaction-declarations-valid-inputs-after-resignation`
scenario confirms a valid adapter-supplied interaction handle/name round trip,
exercises ordinary publication and subscription while joined, then verifies the
four membership-sensitive services report `FederateNotExecutionMember` after
resignation. It is paired with the promoted C++ scenario and remains unsupported
by default until executed against a Java 1516.1-2025 provider.

The opt-in `java-tck.interaction-transportation-query-valid-inputs-after-resignation`
scenario queries a valid active publisher for the adapter-configured interaction
class, verifies the reported federate/class and resolvable transportation name,
then checks `FederateNotExecutionMember` for the same query after requester
resignation. It is paired with the promoted C++ scenario and remains unsupported
by default until executed against a Java 1516.1-2025 provider.

The default `java-tck.directed-interaction-declarations-valid-inputs-after-resignation`
scenario uses the adapter's valid object-class/directed-interaction FOM
association and checks successful set and whole-class publication/subscription
overloads while joined, then `FederateNotExecutionMember` for all six standard
overloads after resignation. It is paired with the promoted C++ scenario and
passed focused installed JNI runs in both callback models. FOM association and
joined-state setup are supplemental; independent-provider conformance remains
unverified.

The opt-in `java-tck.service-report-interaction` scenario uses the adapter's
ordinary FOM, standard MIM, logical-time implementation, and callback model.
It enables standard service reporting only for one ordinary `sendInteraction`,
then checks the received `HLAreportServiceInvocation` identity, all standard
parameters, the reporting federate, `SendInteraction` service name and type,
success indicator, empty exception, initial serial number, reliable transport,
empty tag, and absence of regions. Its C++ base scenario passed independent
installed-package runs in both callback models; Java provider execution remains
unsupported by default pending verification with a Java provider.

The opt-in `java-tck.service-report-query-galt` scenario uses the adapter's FOM,
standard MIM, logical-time implementation, and callback model. It establishes a
time regulator, checks the defined GALT directly, then validates the successful
`HLAreportServiceInvocation`, including its standard parameter set and returned
GALT argument. It is paired with the promoted C++ scenario. The local JNI
provider currently returns `ErrorReadingMIM` while creating the federation with
the official, XML-well-formed 2025 MIM, before scenario assertions; it remains
unsupported by default while the adapter/provider artifacts are reconciled.

The opt-in `java-tck.service-report-next-message-request-available-boundary`
scenario translates the promoted C++ boundary case. It queues a timestamped
interaction, invokes standard `nextMessageRequestAvailable`, checks the MOM
report's encoded logical-time argument, and then checks delivery/grant at the
queued message time. It uses only the standard Java API and adapter-supplied
FOM, MIM, interaction/parameter names, logical-time implementation, and
callback model. It remains unsupported by default until a compatible provider
passes runtime verification.

The opt-in `java-tck.service-report-resign-federation-execution` scenario uses
the same adapter inputs and checks that the observer receives the standard
`HLAreportServiceInvocation` after the subject successfully resigns. It
validates the reported federate, resignation service name/type, success,
exception, serial number, and ordinary interaction metadata. The C++ base and
contract cases passed independent installed-package runs in both callback
models; Java provider execution remains unsupported by default pending
verification with a Java provider.

Ownership, synchronization, DDM, time-managed delivery (including timestamped
attribute reflection and object removal, mixed-advance attribute and interaction
delivery, interaction retraction, cross-producer timestamp ordering, no-fanout
interaction retraction, retraction fan-out, and Time Regulation/Time Constrained
re-enable), save/restore,
and MOM are separate
capability-profile lanes. Timestamped object removal checks the receive-order
asynchronous-delivery gate, checks release both by re-enabling asynchronous
delivery and by a time advance while delivery is disabled, then validates the
timestamped callback metadata and delivery-before-grant ordering. Its callback model is adapter-selectable through
`hla.rti.tck.callbackModel` (`HLA_EVOKED` or `HLA_IMMEDIATE`). A provider profile
can enable a lane only when the provider and supplied FOM support it.
Timestamped reflection checks that the constrained receiver gets no reflection
and does not advance logical time before requesting an advance, then checks the
callback's exact logical time and order metadata, reflection-before-grant
ordering, and the post-grant logical-time query. It remains unsupported by
default until exercised against an installed 2025 provider.

The Java time-advance scenario also verifies that all three alternate advance
requests and a second TAR fail with `InTimeAdvancingState` while a TAR is
pending, without prematurely completing the original request. It takes the
callback model from the adapter configuration rather than hard-coding one.

The opt-in `java-tck.timestamped-default-region-attribute-alternate-advances`
case delivers one ordinary timestamped update to three constrained regional
subscribers, each using Flush Queue, TARA, or NMRA. It verifies empty source-
region metadata, exact value/time/producer/order/retraction/transport data,
reflection-before-grant ordering, each grant and logical-time query, and
past-time rejection for all three advance services. The adapter supplies the
dimensional FOM, names, provider, and logical-time implementation; runtime
verification remains pending.

The `java-tck.timestamped-default-region-attribute-reenable` case uses ordinary
object registration and a timestamped update with a dimensional regional
subscription. It disables and re-enables Time Constrained while delivery is
queued, then verifies the reflection's supplied-empty source-region metadata,
callback-before-grant ordering, timestamp/value/retraction data, and the
post-delivery retraction boundary. The provider, dimensional FOM, names,
logical-time implementation, and callback model are adapter supplied; runtime
verification remains pending.

The `java-tck.timestamped-default-region-attribute-regulation-reenable` case
queues an ordinary timestamped update for regional subscribers, disables and
re-enables Time Regulation with a changed lookahead, and checks that the queued
reflection remains withheld until advance. It verifies empty source-region
metadata, the changed lookahead, reflection-before-grant order, grant/query
times, and the post-delivery retraction boundary. Adapter and provider runtime
verification remain pending.

The `java-tck.timestamped-default-region-attribute-mixed-fanout` case sends
one ordinary timestamped update to immediate and time-constrained regional
subscribers. It verifies the delivered copy's value, tag, producer, time,
orders, transport, retraction handle, and empty source-region set; retraction
then reaches only the immediate recipient, while the queued copy is suppressed
before the constrained grant. Runtime verification remains pending on an
adapter-supplied provider and dimensional FOM.

The `java-tck.delay-subscription-evaluation-timestamped-regional-attribute-update`
case repeats the timestamped DSE enabled/disabled comparison with regional
source association and regional subscriptions. It unsubscribes before sending,
restores the declaration before the grant, and checks that the enabled switch
reevaluates the queued update, including its source-region metadata; a later
update is withheld after unsubscribe. DDM FOM, switch FOM, logical-time
implementation, names, provider, and callback model are adapter supplied.

The `java-tck.delay-subscription-evaluation-timestamped-regional-interaction`
case applies the same enabled/disabled grant-boundary comparison to regional
timestamped interactions. It checks parameter/tag/time/order/producer/transport
metadata and the exact source-region set when the enabled switch makes the
late declaration eligible, then confirms a later send is suppressed after
unsubscribe. DDM and switch FOMs, names, logical time, provider, and callback
model are adapter supplied; runtime verification remains pending.

The `java-tck.timestamped-attribute-order-cohort` scenario queues updates at
timestamps 5, 5, and 7 for two constrained recipients. It accepts either order
for the equal-time pair while requiring both updates before the time-5 grant,
then verifies the later update before the time-7 grant, including exact values,
tags, producer, timestamp, timestamp order, retraction handles, and resolvable
transportation. Callback mode, FOM, and logical-time implementation come from
the adapter; the scenario remains unsupported by default pending Java-provider
execution.

The `java-tck.order-type-controls` scenario changes the class default between
receive and timestamp order around successive object registrations, applies a
per-instance timestamp-order override, and verifies ordinary versus timestamped
attribute callback delivery. It then sends a timestamped interaction explicitly
set to receive order and checks its receive-order metadata and absent retraction
handle. FOM, logical-time implementation, and callback model are adapter-supplied;
the scenario remains unsupported by default until Java-provider execution.

The `java-tck.receive-order-attribute-update-callback-cancellation` scenario
branches on the adapter-selected callback model. In evoked mode it withdraws
one subscriber before servicing callbacks and verifies that only the still-active
subscriber receives the update. In immediate mode it waits until both subscribers
have received the update before withdrawing one subscription. It verifies the
object, attribute, value, tag, producer, and transport callback data; it remains
unsupported by default pending Java-provider execution.

The opt-in `java-tck.receive-order-attribute-update-receiver-resignation-fence`
scenario queues an update for a receiver, resigns and rejoins that receiver
before callback delivery, and verifies the old tagged reflection does not
cross the membership boundary while one later update reaches the new
membership. It is parity-linked to the C++ public-API contract candidate and
uses only `hla.rti1516_2025` types plus adapter-supplied FOM, endpoint,
logical-time implementation, and callback model. All 388 Java source files
compile against the pinned API JAR; Java provider-runtime execution remains
unverified.

The `java-tck.interaction-subscription-lifecycle` scenario exercises passive
and active interaction subscriptions, activation without replay, delivery
while active, downgrade back to passive, and unsubscription. A passive-only
declaration does not establish delivery relevance; the test confirms it
suppresses delivery before activation and after downgrade. Callback model, FOM,
and names are adapter-supplied; it remains unsupported by default pending
Java-provider execution.

The opt-in `java-tck.interaction-service-boundaries` scenario verifies that both
active and passive subscribers receive the same ordinary interaction with the
adapter's parameter handle, value, tag, producer handle, and resolvable
transportation metadata. The case is paired with the promoted C++ scenario and
remains unsupported by default pending Java-provider execution.

The opt-in `java-tck.interaction-order-classification` scenario sends a
receive-order interaction and then a timestamp-order interaction to the same
receiver. It verifies distinct callback paths, exact payload and metadata,
timestamp/retraction/order fields, and timestamped delivery before the matching
time-advance grant. FOM and logical-time implementation remain adapter-supplied;
provider runtime verification is still required.

The opt-in `java-tck.attribute-update-order-classification` scenario sends an
ordinary receive-order attribute update and then a timestamp-order update to
the same receiver. It verifies distinct reflection callback paths, exact
payload and metadata, timestamp/retraction/order fields, and timestamped
delivery before the matching time-advance grant. FOM and logical-time
implementation remain adapter-supplied; provider runtime verification is
still required.

The opt-in `java-tck.custom-transportation-interaction-delivery` scenario uses
the adapter-supplied rich FOM, typed interaction/parameter names, and
`hla.rti.tck.customTransportationType` to verify custom transportation handle
name round trips, ordinary interaction callback metadata, and the standard
interaction transportation query report. The case is paired with the promoted
C++ scenario and remains unsupported by default pending Java-provider execution.

The `java-tck.interaction-publication-send-fence` scenario verifies that sending
after unpublication fails with `InteractionClassNotPublished` and produces no
subscriber callback, then republishes and verifies delivery with exact parameter,
tag, producer, reliable transport, and region metadata. FOM, names, and callback
mode are adapter-supplied; the scenario remains unsupported by default pending
Java-provider execution.

The `java-tck.interaction-publication-send-transition` scenario verifies
successful delivery while published, exact `InteractionClassNotPublished`
rejection after withdrawal with no callback, and successful delivery recovery
after republication. It checks payload, tag, producer, reliable transportation,
empty region metadata, evoked callback timing, and publisher exclusion. FOM,
names, and callback model are adapter-supplied; it remains unsupported by
default pending Java-provider execution.

The `java-tck.interaction-multi-recipient-fifo` scenario sends two tagged
interactions to two active subscribers and verifies each independently receives
the same FIFO sequence with exact class/parameter/value/tag/producer metadata,
resolvable transportation, no region routing, and no publisher loopback. FOM,
interaction and parameter names, and callback model are adapter-supplied; it
remains unsupported by default pending Java-provider execution.

The opt-in `java-tck.custom-transportation-attribute-delivery` scenario
registers and discovers an object from the adapter's rich FOM, then verifies an
ordinary reflected attribute update retains the FOM-declared custom
transportation type. Configure the names with
`hla.rti.tck.customTransportationObjectClass`,
`hla.rti.tck.customTransportationAttribute`, and
`hla.rti.tck.customTransportationType`; defaults match the repository's TCK
rich FOM. The scenario is paired with its promoted C++ case and remains
unsupported by default pending Java-provider execution.

The opt-in `java-tck.custom-transportation-regional-attribute-delivery` case
reuses the ordinary regional-object routing oracle with adapter-supplied custom
FOM declarations. It checks named regional registration and discovery, overlap
filtering, ordinary reflection and regional value-request response, source
region designators, reassociation, and the declared custom transportation name
on each delivery. Configure the DDM FOM, object class, attribute, two dimensions,
and transportation type through the adapter; provider runtime remains pending.

The opt-in `java-tck.custom-transportation-timestamped-attribute-delivery`
scenario uses the same adapter FOM and names across a publisher, an unconstrained
receiver, and a time-constrained receiver. It checks timestamp-order metadata
and custom transportation identity, retracts one queued update before its
constrained delivery, then verifies a later update reaches its timestamp and
cannot be retracted after delivery. Configure the object class, attribute, and
transportation type as above; configure the logical-time implementation through
the adapter. It is paired with the promoted C++ case and remains unsupported by
default pending Java-provider execution.

The opt-in
`java-tck.custom-transportation-timestamped-attribute-alternate-advances` case
uses the existing three-recipient timestamped-attribute oracle with an
adapter-declared custom FOM transportation type. It checks custom class,
attribute, and transport name round trips, then verifies all three alternate
advance callbacks preserve that transport along with the timestamp, order,
payload, tag, and retraction handle. Supply the DDM FOM, logical-time
implementation, names, provider, and callback model through the adapter; runtime
verification remains pending.

The opt-in
`java-tck.custom-transportation-timestamped-interaction-delivery` case covers
ordinary timestamped interaction delivery with the adapter-declared custom
transportation type. It checks name round trips and callback transport identity,
payload, tag, producer, logical time, order, and retraction metadata. The adapter
supplies the FOM, interaction/parameter names, logical-time implementation,
provider, and callback model; runtime verification remains pending.

The opt-in
`java-tck.custom-transportation-timestamped-interaction-alternate-advances`
case reuses the three-recipient timestamped-interaction oracle and verifies the
adapter-declared custom transportation name round-trips through publisher and
receivers and is preserved by Flush Queue, TARA, and NMRA delivery. It checks
timestamp/order, parameter payload, tag, producer, and retraction identity too.
The adapter supplies the FOM, interaction/parameter names, logical-time
implementation, provider, and callback model; runtime verification remains
pending.

The opt-in
`java-tck.custom-transportation-timestamped-directed-interaction-alternate-advances`
case extends the timestamped directed-delivery oracle across Flush Queue, TARA,
and NMRA. It verifies adapter-supplied object, attribute, interaction,
parameter, and transportation names; target name/handle round trip; and custom
transport identity on all three timestamped callbacks. The adapter supplies the
FOM, logical-time implementation, names, provider, and callback model; runtime
verification remains pending.

The opt-in `java-tck.custom-transportation-timestamped-directed-delivery` case
translates the standard single-TAR directed timestamped route with an
adapter-declared custom transport. It checks object/attribute/interaction/
parameter and transport name round trips, discovery, timestamp/order/tag/
retraction metadata, and the custom transport reported in the delivery callback.
The adapter supplies the rich FOM, logical-time implementation, provider, and
callback model; provider runtime verification remains pending.

The `java-tck.directed-interaction-target-lifecycle` scenario verifies directed
delivery to a discovered object target, cancellation/restoration across
selective subscription and publication changes, callback metadata and delivery
ordering across target deletion, the standard removal callback, and rejection
of a later send to the deleted target. An independent `HLA_IMMEDIATE` observer
checks target discovery and directed deliveries even when the adapter-selected
receiver uses evoked callbacks. The primary callback model is selected by the
adapter; the case imports only the IEEE 1516.1-2025 Java API.

The default `java-tck.directed-interaction-subscription-kind` scenario distinguishes
by-ownership from universal delivery, checks that an empty by-ownership set
does not disturb a universal declaration, and resubscribes both subscribers
to different kinds while the federation is active. It uses adapter-supplied
provider, FOM, logical-time implementation, endpoint, names, and callback
model, and receiver-local handles. Focused installed JNI runs passed in both
callback models. Ownership/universal eligibility remains supplemental under
RL-028; independent-provider conformance is unverified.

The default `java-tck.timestamped-directed-interaction-subscription-kind`
scenario checks owner and universal delivery after a live directed-subscription
kind change, timestamped callback metadata before Time Advance Grants, and
valid versus invalid retraction results for regulating and non-regulating
senders. Its Java source imports only the official IEEE 1516.1-2025 API;
provider, FOM, logical time, endpoint, names, and callback model remain
adapter-supplied. Focused evoked and immediate runs passed using a Java
provider JAR rebuilt from source with the installed native library. The
recipient eligibility matrix and positive-lookahead boundary are supplemental;
independent-provider conformance is unverified.

The default `java-tck.directed-derived-object-target-eligibility` scenario runs the
same ownership/universal distinction with an adapter-selected derived object
class and inherited identity attribute. It checks target ownership and
universal delivery while excluding subscribers that are not eligible for the
target. Provider, compatible typed FOM, logical time, endpoint, names, and
callback model are adapter-supplied. It uses receiver-local object and member
handles and passed focused installed JNI runs in both callback models.
Derived-class applicability and the recipient partition are supplemental
regression checks, not independent-provider conformance claims.

The default `java-tck.directed-interaction-multi-recipient-fifo` scenario
registers one target, then sends two ordinary directed interactions to it. Both
universal subscribers independently receive exact class, receiver-local
target/parameter/value/tag/producer metadata and resolvable transportation,
while the sender receives no loopback. FIFO is an additional regression check,
not a standalone conformance claim. The adapter supplies the FOM, names,
logical-time implementation, provider, endpoint, and callback model. Focused
installed JNI runs passed in both modes; independent-provider conformance
remains unverified.

The default `java-tck.directed-interaction-mixed-subscription-fanout` scenario
contrasts a by-ownership subscriber, two universal subscribers, and an
unsubscribed member across two targets. It verifies exact target, parameter,
tag, producer, and resolvable transport metadata, producer order, and that
only the universal subscribers receive an interaction addressed to another
member's object. Provider, FOM, logical-time implementation, names, endpoint,
and callback model are adapter-supplied. It uses receiver-local object and
member handles and passed focused installed JNI runs in both callback modes.
The by-ownership/universal partition, sender exclusion, and ordering are
supplemental regression checks; independent-provider conformance remains
unverified.

The default `java-tck.directed-interaction-no-applicable-subscription` case
first discovers a target by its standard object name, then sends an untimed
directed interaction to a receiver without a matching directed-class
subscription. It checks that neither directed nor ordinary interaction
delivery occurs during bounded callback servicing. It uses receiver-local
handles and passed focused installed JNI runs in both callback models;
independent-provider conformance remains unverified.

The default `java-tck.directed-interaction-subscription-accumulation` case
uses two sequential universal subscriptions for sibling directed classes on
the same object class. Both untimed sends must arrive through the receive-order
directed callback with their own parameters and tags; a timed overload is a
failure. The adapter can override both class and parameter names and supplies
the compatible FOM. Focused installed JNI runs passed in both callback models;
the separate timestamped witness and independent-provider conformance are not
established by this case alone.

The default `java-tck.directed-known-object-class-fallback` case discovers a
derived target only as its subscribed base class, then verifies that one
untimed directed send is received as the base interaction with only the base
parameter. The inapplicable child-class subscription does not change that
projection. It then uses standard time services to queue an NMR and send a
timestamped directed interaction: the same base-class projection must arrive
at message time before the grant, with timestamp, order, tag, and retraction
metadata intact. It uses receiver-local standard handles and passed focused
installed JNI runs in both callback models. Independent-provider conformance
remains unverified.

The default `java-tck.directed-interaction-received-class-projection` case
rejects a base-class send containing a derived-only parameter, then sends a
valid derived interaction. The publisher gets no loopback; the superclass
subscriber receives only the inherited parameter, while the exact-class
subscriber receives both. Its universal subscriptions and receiver-local
handles match the C++ standard-API case. Focused installed JNI runs passed in
both callback models; independent-provider conformance remains unverified.

The `java-tck.receive-order-interaction` scenario sends two tagged interactions
in succession and verifies callback FIFO order, exact parameter values, standard
handle/name round-trips, producer and reliable-transport metadata, no region
routing, and no publisher loopback. FOM, interaction and parameter names, and
callback mode are adapter-supplied; it remains unsupported by default pending
Java-provider execution.

The `java-tck.receive-order-interaction-callback-cancellation` scenario sends
one ordinary reliable interaction to two subscribers. In evoked mode it
withdraws one subscription before callback servicing and confirms that only the
active subscriber receives the interaction; in immediate mode it waits for both
deliveries before withdrawal. It verifies the interaction and parameter handles,
payload, tag, producer, reliable transportation, and absent region routing. Its
FOM, names, and callback model are adapter-supplied; it remains unsupported by
default pending Java-provider execution.

The `java-tck.receive-order-multi-attribute-update-callback-cancellation`
scenario projects a two-attribute update onto a subscriber interested in only
the second attribute. It checks both evoked cancellation and immediate delivery,
exact callback attribute sets and metadata, and that a later first-attribute-only
update reaches only the still-subscribed receiver. The provider-compatible FOM,
class and attribute names, and callback model are adapter-supplied; it remains
unsupported by default pending Java-provider execution.

The `java-tck.modify-lookahead-contract` scenario checks Query Lookahead,
immediate increase, invalid zero rejection without mutation, deferred decrease
until a Time Advance Grant, and the evoked-mode `InTimeAdvancingState` boundary
while a TAR is pending. It uses the adapter's FOM, logical-time implementation,
and callback model, and only the official IEEE 1516.1-2025 Java API. It is
unsupported by default until executed against a compatible 2025 provider.

The `java-tck.modify-lookahead-service-report` scenario composes the adapter's
FOM and standard MIM, requests a lower lookahead while time regulating, verifies
that the change remains deferred, and checks the resulting standard
`HLAreportServiceInvocation`: service identity/type, success, argument type/name
and requested interval, null return, serial number, federate identity, and
callback metadata. Provider, FOM/MIM, logical-time implementation, and callback
model remain adapter inputs; runtime verification is pending.

The `java-tck.timestamped-interaction-mixed-advances` scenario sends the same
timestamped interaction to three constrained receivers, advanced respectively
by Flush Queue, Time Advance Request Available, and Next Message Request
Available. The publisher advances to one lookahead before the interaction
timestamp, making the message time the inclusive GALT boundary for the two
available-advance receivers. It checks callback-before-grant ordering, payload
and tag, producer, time/order/retraction metadata, grant times, and logical-time
queries. It remains unsupported by default until run through a compatible
provider adapter.

The `java-tck.time-coordinated-interaction` scenario mirrors the focused C++
Clause 8.1.3 case: a timestamped interaction remains queued when its constrained
receiver advances only to time 2, then arrives with its original time and
standard delivery metadata before the receiver's matching time-5 grant. The
provider, FOM, logical-time implementation, interaction/parameter names, and
callback model are adapter inputs; Java provider-runtime verification remains
pending.

The `java-tck.timestamped-regional-interaction-alternate-advances` scenario
additionally checks the distinct Flush Queue actual/optimistic frontier (5/7),
then sends a timestamped regional interaction at actual time plus queried
lookahead (6) without changing the receiver's logical time. The C++ and Java
scenarios retain matching parity links and adapter-supplied DDM FOM/time
configuration; Java runtime verification remains adapter-dependent.

The `java-tck.timestamped-directed-alternate-advances` scenario applies the
same Flush Queue, TARA, and NMRA comparison to directed interactions addressed
to a registered object. It verifies target discovery, no delivery before an
advance, callback-before-grant ordering for all three services, exact grant
distinctions, payload/tag/time/order/retraction metadata, and resolvable
transportation. It remains unsupported by default until run through a
compatible provider adapter.

The `java-tck.timestamped-interaction-retraction` scenario follows the promoted
C++ sequence: an unconstrained subscriber receives a timestamped interaction,
then receives the matching Request Retraction callback; a constrained copy is
suppressed when retracted before its grant. A second interaction reaches the
constrained subscriber before its grant, after which retracting its handle must
report `MessageCanNoLongerBeRetracted`. The scenario uses the adapter's callback
model, FOM, and time implementation and remains unsupported by default pending
Java provider execution.

The `java-tck.timestamped-attribute-ownership-transfer` scenario queues a
timestamped attribute update while the original owner is regulating, then
divests and transfers that attribute before the constrained recipient advances.
It checks both ownership callbacks and tags, the unowned interval, successful
acquisition, and exact update value/tag/producer/time/order/retraction metadata
at the constrained grant. Callback mode is adapter-selected. It remains
unsupported by default pending execution against a Java provider.

The focused service-report scenarios decode standard
`HLAreportServiceInvocation` callbacks through the public encoding API. The
ordinary attribute-update case verifies the invoking federate, service type,
success result, empty exception, and initial report serial. The
timestamped attribute-update and object-deletion cases additionally verify
retraction metadata and delay the corresponding reflection/removal until the
constrained receiver and regulating sender complete the required time advances.
The timestamped deletion failure case verifies reports for an already removed
object and for a timestamp below current time plus lookahead, including typed
exception, supplied-argument, null-return, and serial-number fields. These are
adapter-dependent and remain unsupported by default until exercised against an
installed Java provider. The timestamped attribute-update failure case adds
unknown-object, undefined-attribute, and invalid-time reports and confirms that
none of those rejected calls reflects to a subscribed receiver. The timestamped
interaction failure case checks invalid interaction class, invalid parameter,
and invalid-time reports with no application interaction delivery.

The `java-tck.timestamped-attribute-source-resignation` scenario queues an
attribute update, then resigns its producer with unconditional divestiture.
The survivor assumes and acquires the attribute; retraction by the departed
producer must fail with `FederateNotExecutionMember`, while a separate
regulating clock permits the queued update to reach the survivor before its
grant. It checks the original producer and complete timestamped reflection
metadata. Callback mode remains adapter-selected, and the scenario stays
unsupported by default pending Java-provider execution.

The `java-tck.timestamped-attribute-immediate-recipient-multi-transport-retraction`
scenario sends one timestamped attribute update from a regulating publisher to
an unconstrained subscriber. Its adapter FOM declares two attributes using
`HLAreliable` and `HLAbestEffort`; the subscriber must receive exactly one
reflection per transportation passel with the shared timestamp and retraction
handle. Retract must then produce exactly one recipient-local
`requestRetraction`, and a second Retract must fail with
`MessageCanNoLongerBeRetracted`. The adapter supplies the FOM through
`--fom-path` and may override `hla.rti.tck.timestampedAttributePasselObjectClass`,
`hla.rti.tck.timestampedAttributePasselReliableAttribute`, and
`hla.rti.tck.timestampedAttributePasselBestEffortAttribute` with
`--jvm-argument`. The Java source compiles against the official API-only JAR;
the scenario remains unsupported by default until run against an adapter's
Java provider.

The `java-tck.timestamped-attribute-update-alternate-advances` scenario sends
one timestamped attribute update to constrained receivers using Flush Queue,
Time Advance Request Available, Next Message Request Available, and Next Message
Request. It checks reflection-before-grant ordering and, after the early NMR
grant at time five, sends from an owned object at time seven with lookahead one
while confirming logical time remains five. It also checks payload/tag,
producer metadata, timestamp/order/retraction handles, grant/query consistency,
Flush Queue actual and optimistic bounds, and terminal retraction. It remains
unsupported by default pending Java-provider runtime verification.

The `java-tck.timestamped-object-deletion-mixed-advances` scenario queues one
timestamped object deletion for three constrained recipients using Flush Queue,
Time Advance Request Available, and Next Message Request Available. It checks
removal-before-grant ordering, each service's distinct grant and logical-time
result, exact removal metadata, Flush Queue time bounds, and the
`MessageCanNoLongerBeRetracted` terminal boundary. It remains unsupported by
default until run through a compatible provider adapter.

The `java-tck.timestamped-object-deletion-no-fanout` scenario checks a
timestamped deletion exactly at the producer lookahead boundary, retraction of
a later deletion when no recipient is eligible, temporary loss and restoration
of the object-name mapping and attribute ownership, absence of recipient
callbacks, and the already-retracted terminal exception. It remains unsupported
by default until exercised against a Java provider.

The `java-tck.timestamped-object-deletion-tombstone` scenario reserves and
registers a named instance, deletes it at a timestamp, advances to its terminal
boundary, verifies that retraction is no longer possible, then reserves and
registers the same name again and checks the resulting handle lookup. It remains
unsupported by default pending Java-provider execution.

The `java-tck.timestamped-object-deletion-retraction` scenario sends two
timestamped deletions to an unconstrained and a constrained recipient. The first
is retracted after the unconstrained recipient observes provisional removal,
checking the Request Retraction callback and restored name-to-handle identity;
the constrained recipient must not observe the retracted deletion at its grant.
The second deletion is delivered before the constrained grant, after which
retraction must fail with `MessageCanNoLongerBeRetracted` and both recipients
must observe terminal removal; a subsequent timestamped delete using that
removed handle must fail with `ObjectInstanceNotKnown`. It remains unsupported
by default pending execution against a Java provider.

The `java-tck.timestamped-object-deletion-retraction-joined-owners` scenario
transfers one attribute to a second owner, queues a timestamped deletion, and
then has that owner resign before the producer retracts the deletion. It checks
that joined members regain the object identity, the departed owner's attribute
remains unowned, the constrained member receives no stale deletion or
retraction callback, and both time advances complete. The adapter supplies a
multi-attribute FOM plus `hla.rti.tck.multiAttributeObjectClass`,
`hla.rti.tck.multiAttributeFirst`, and `hla.rti.tck.multiAttributeSecond`
properties when its names differ from the TCK defaults. It remains unsupported
by default pending execution against a Java provider.

The `java-tck.timestamped-object-deletion-source-resignation-fanout` scenario
queues one timestamped deletion for two constrained recipients using different
advance services. It resigns the producer before advancing a regulating clock,
then checks independent callback-before-grant delivery, each recipient's grant
and logical time, and the `FederateNotExecutionMember` retraction boundary.
Provider, FOM, logical-time implementation, endpoint, and callback model remain
adapter supplied, so this scenario is unsupported by default until Java-provider
execution.

The `java-tck.timestamped-object-deletion-regulation-reenable` scenario queues
a timestamped removal, disables and re-enables producer regulation with a
changed lookahead, verifies Query Lookahead, and checks that removal metadata
and callback-before-grant ordering survive until the constrained receiver's
grant. It also verifies both logical-time queries and the terminal retraction
exception. It remains unsupported by default pending Java-provider execution.

The `java-tck.timestamped-interaction-cross-producer-order` scenario sends
timestamp-five and timestamp-seven interactions from two regulating producers
to two constrained receivers. It requires increasing timestamp order and
producer/tag identity, while allowing either ordering between the two equal-time
events. It remains unsupported by default until exercised through a compatible
provider adapter.

The `java-tck.timestamped-interaction-no-fanout` scenario publishes and sends
timestamped interactions without any subscribing federate. It checks that send
still returns a valid retraction handle, successful retraction and repeated-
retraction terminalization, expiry after a time advance, and absence of receive
or retraction callbacks. It remains unsupported by default pending Java provider
execution.

The `java-tck.timestamped-interaction-source-resignation` scenario holds a
timestamped interaction behind a constrained receiver's time frontier, resigns
the producer with `NO_ACTION`, and verifies that the accepted interaction is
delivered exactly once before the receiver's grant. It checks parameter, tag,
producer, timestamp, order, transportation, and retraction metadata, including
that the former producer cannot retract after resignation. It remains
unsupported by default pending Java provider execution.

The `java-tck.timestamped-interaction-retraction-fanout` scenario has both an
immediate and a constrained subscriber. It checks delivery metadata, the
delivered-copy Request Retraction callback, suppression of the still-queued
copy through a time-advance grant, then repeats delivery and retraction after
the constrained member resigns. Time Regulation disable is also checked. It
remains unsupported by default until run against a Java provider adapter.

The `java-tck.timestamped-interaction-regulation-reenable` scenario sends a
timestamped interaction, disables the producer's Time Regulation while the
interaction remains queued, then re-enables with a three-epsilon lookahead.
It checks Query Lookahead, delivery-before-grant ordering, callback metadata,
grant/query times, and terminal retraction after delivery. It remains unsupported
by default pending Java provider execution.

The `java-tck.timestamped-interaction-reenable` scenario keeps a timestamped
interaction queued while the receiver disables and re-enables Time Constrained.
It checks suppression during both boundaries, delivery before the later grant,
the original payload/tag/producer/time/order/transport/retraction metadata, and
the terminal retraction exception. It remains unsupported by default pending
Java provider execution.

The `java-tck.timestamped-directed-interaction-reenable` scenario preserves a
timestamped directed interaction queued for a registered target while its
receiver disables and re-enables Time Constrained. It verifies target discovery,
directed publication/subscription, suppression through both boundaries,
delivery-before-grant ordering, callback metadata (including target, payload,
producer, transport, and retraction handle), grant/query times, and terminal
retraction. Its callback model, FOM, logical-time implementation, and provider
are adapter-supplied; it remains unsupported by default pending Java-provider
execution. Its portable defaults target the parameter-bearing P0 declarations;
adapters can override the object class, attribute, directed interaction, and
parameter with `hla.rti.tck.timestampedDirectedObjectClass`,
`hla.rti.tck.timestampedDirectedAttribute`,
`hla.rti.tck.timestampedDirectedInteractionClass`, and
`hla.rti.tck.timestampedDirectedParameter`.

The `java-tck.timestamped-directed-interaction-regulation-reenable` scenario
keeps a timestamped directed message queued while the producer disables and
re-enables Time Regulation with a three-epsilon lookahead. It checks Query
Lookahead, target and payload metadata, delivery-before-grant ordering, both
federates' grant/query times, and terminal retraction after delivery. It uses
the same adapter-supplied directed FOM names and callback model as the directed
Time Constrained re-enable case; it remains unsupported by default pending
Java-provider execution.

The `java-tck.timestamped-directed-interaction-tar-nmr` scenario sends one
timestamped directed interaction to two constrained receivers. One requests a
TAR at the message time; the other requests NMR beyond it. It verifies that
both receive the directed payload before their grants, that TAR grants at its
requested time while NMR grants at the message time, and that target, producer,
timestamp/order, transport, and retraction metadata are preserved. It uses the
adapter-configured callback model and directed FOM names and remains unsupported
by default pending Java-provider execution.

The `java-tck.timestamped-directed-interaction-source-resignation` scenario
keeps the target object with a separate owner, resigns the time-regulated
producer while its directed message is queued, and uses an independent clock
federate to advance the frontier. It verifies that resignation does not release
the callback prematurely, that retraction through the resigned producer fails
with `FederateNotExecutionMember`, and that the receiver eventually gets the
original target and message metadata before its requested-time grant. The case
uses adapter-selected FOM names and callback model. Its P0 default object class
is `HLAobjectRoot.TckObject.TckSecondaryObject`. A focused Java provider run
passed in evoked mode but delivered the callback before the clock advance in
immediate mode; the case remains unsupported by default pending a fresh green
gate in both modes.

The `java-tck.timestamped-directed-interaction-source-resignation-fanout`
scenario extends that lifecycle to two independently constrained recipients.
Both queue a TAR at the directed message time; advancing the independent clock
first releases exactly one receiver, while the other remains pending until a
second clock advance. It checks per-recipient delivery-before-grant ordering,
no duplicate delivery, target and payload identity, producer, timestamp/order,
transport, tag, retraction metadata, and the post-resignation retraction
exception. It uses adapter-supplied FOM names and callback model and remains
unsupported by default pending Java-provider execution.

The `java-tck.timestamped-directed-interaction-immediate-source-resignation`
scenario sends a timestamped directed interaction to an unconstrained receiver,
then immediately resigns the regulated producer. It checks exactly one later
callback with the owner-held target, payload, tag, producer, timestamp, sent and
received order, transport, and retraction metadata intact, and rejects an
unexpected retraction callback. It uses adapter-supplied FOM names and callback
model and remains unsupported by default pending Java-provider execution.

The `java-tck.timestamped-directed-interaction-retraction-fanout` scenario
queues a timestamped directed message for two constrained receivers, retracts
it before either recipient advances, and verifies both reach that time without
delivery or retraction callbacks. It then sends a later message and checks one
delivery to each recipient before each grant, full target/payload/tag/producer/
time/order/transport/retraction metadata, and the
`MessageCanNoLongerBeRetracted` boundary for the delivered handle. It remains
unsupported by default pending Java-provider execution.

The `java-tck.timestamped-directed-interaction-retraction` scenario is the
single-recipient counterpart: it retracts one queued directed interaction,
advances the receiver without a stale delivery, then sends a later interaction
and verifies its callback metadata, grant ordering, and
`MessageCanNoLongerBeRetracted` terminal boundary. It remains unsupported by
default pending Java-provider execution.

The `java-tck.timestamped-directed-interactions` scenario translates the
combined portable C++ case: it distinguishes target-owner, universal, and
non-owner subscriptions; checks invalid interaction, target, and retraction
handles; observes a receive-order callback and its retraction while the target
is unconstrained; then verifies constrained timestamp-order delivery only to
the target and universal subscriber. It checks callback metadata and the
post-delivery retraction boundary. Provider, FOM, time implementation,
timestamped-directed names, and callback model remain adapter inputs; it is
unsupported by default until exercised against an external Java 1516.1-2025
provider.

The `java-tck.directed-interaction-derived-object-target` scenario registers an
adapter-selected derived object at the publisher, then verifies that a distinct
federate with a universal directed-interaction subscription receives the send
while an attribute-only observer does not. It checks target discovery and
interaction callback metadata through receiver-local standard handles. The
provider, typed FOM, time implementation, endpoint, names, and callback model
remain adapter inputs. This default-run case passed focused installed JNI runs
in both callback models. Derived-target identity and observer exclusion are
supplemental checks, not independent-provider conformance claims.

The `java-tck.timestamped-directed-interaction-derived-object-target` scenario
registers an adapter-selected derived object class with an inherited identity
attribute, directs a timestamped typed interaction to that target, and verifies
that an interested constrained federate receives it before TAR grant while a
second federate without a directed subscription receives none. The FOM/model,
derived class, identity attribute, interaction class, integer parameter, time
implementation, and callback model are adapter inputs. The default names use
the repository's TckTyped model; override them with
`hla.rti.tck.typedDerivedObjectClass`,
`hla.rti.tck.typedIdentityAttribute`,
`hla.rti.tck.typedInteractionClass`, and
`hla.rti.tck.typedIntegerParameter`. The case remains unsupported by default
pending Java-provider execution.

The ordinary tests use the IEEE Restaurant example names by default, including
`HLAobjectRoot.Employee.Server`/`Efficiency`,
`HLAobjectRoot.Food.Drink`/`NumberCups`, and
`HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed`.
Directed/relevance scenarios default to
`HLAinteractionRoot.ServerAction.TakeOrder`. Supply a compatible FOM or
override these names with the `hla.rti.tck.*` system properties.

The provider-independent API-surface inventory checks the public signatures of
the factory, ambassador, encoder, and logical-time interfaces. Every exposed
parameter, result, and exception type must belong to the official
`hla.rti1516_2025` namespace or the JDK. This check can run with the API JAR
alone and does not claim behavioral conformance by an RTI provider.

The opt-in `java-tck.receive-order-object-removal-subscription-withdrawal`
scenario deletes an object after two subscribers discover it. In evoked mode,
one subscriber withdraws before callback servicing; both the still-subscribed
and withdrawn subscriber must receive the already-queued terminal removal. In
immediate mode it checks callback delivery before withdrawal. Both paths verify
object/tag/producer identity and suppress owner loopback. Its C++ base/contract
pair passed two independent fresh GNU 15.2/Ninja installed-package builds,
with 4/4 evoked/immediate CTest cases passing in each build. Java provider
execution remains unsupported by default pending an IEEE 1516.1-2025 provider.

The default `java-tck.resign-delete-objects-multi-recipient-fifo` scenario
registers two objects, then resigns the producer with standard `DELETE_OBJECTS`.
Both active subscribers must receive the two empty-tag removals in producer
order with the correct object and producer handles, and the producer must not
receive loopback. Its C++ base/contract pair passed two fresh GNU 15.2/Ninja
installed-package builds, with 4/4 focused evoked/immediate tests passing in
each. It passed a focused Java run against a fresh GNU/Ninja build of the local
JNI provider using the adapter-selected FOM and logical-time implementation.
Its behavior is observable entirely through the standard 1516.1-2025 API.

The default `java-tck.resign-delete-objects` scenario checks that
`NO_ACTION` resignation fails while the federate owns the standard
delete-privilege attribute, then verifies `DELETE_OBJECTS` delivers one empty-
tag removal with the correct object and producer. It also checks discovery
identity/name metadata and that the removed instance name no longer resolves.
The adapter supplies the FOM, logical-time implementation, provider, and
callback model. Its C++ base/contract pair passed two fresh GNU 15.2/Ninja
installed-package builds, each with all four evoked/immediate focused tests
passing. It passed a focused Java run against a fresh GNU/Ninja build of the
local JNI provider. Its behavior is observable entirely through the standard
1516.1-2025 API.

The default `java-tck.resign-delete-objects-then-divest-cross-object-effects`
scenario gives one federate an attribute on another federate's retained object,
then has it register a separate object and resign with
`DELETE_OBJECTS_THEN_DIVEST`. It checks removal delivery and name lookup for the
departing object's deletion, preservation of the retained object, and
successful acquisition of the released attribute by a third federate. Provider,
FOM, logical-time implementation, and callback model are adapter-supplied. Its
C++ base/contract pair passed two independent GNU 15.2/Ninja installed-package
builds with all four evoked/immediate focused CTest cases passing in each; the
Java scenario passed focused execution against a fresh GNU/Ninja JNI provider.
All assertions use the standard 1516.1-2025 API.

The default `java-tck.resign-delete-then-divest-privilege-epoch` case registers
an object, withdraws and republishes the standard delete privilege, then
registers a second object. On `DELETE_OBJECTS_THEN_DIVEST`, the earlier object
is retained for attribute assumption and only the later one is removed. It
compares discovered and removed objects using the receiver's own handles.
Focused installed JNI runs passed in both callback modes with adapter-supplied
FOM, logical time, provider, endpoint, and callback model; independent-provider
conformance remains unverified.

The default `java-tck.resign-assumption-later-publication` case verifies that
resigning an owner with `UNCONDITIONALLY_DIVEST_ATTRIBUTES` does not offer its
attributes prematurely to a known, nonpublishing candidate. Once that
candidate publishes, it receives one assumption offer with its local object
handle, the ordinary attribute and standard delete privilege, and an empty
tag. The case passed focused installed JNI runs in both callback modes using
adapter-supplied inputs; independent-provider conformance is unverified.

The default `java-tck.resign-assumption-later-discovery` case keeps the
federation alive after the owner resigns, then joins a new candidate. That
candidate discovers the retained object before publication, receives no early
offer, and receives one grouped empty-tag assumption offer after publishing.
It checks the candidate's local object handle and passed focused installed JNI
runs in both callback modes; independent-provider conformance is unverified.

The default `java-tck.ownership-assumption-search-after-decline` case holds a
first candidate's assumption callback while a second candidate receives its
acquisition-cancellation confirmation. It verifies that the unowned search
continues only after the first callback returns, with one tagged offer to the
second candidate. Immediate delivery uses only the standard callback
disable/enable services. Receiver-local handles, adapter-supplied inputs, and
focused installed JNI passes in both callback modes keep this portable;
independent-provider conformance is unverified.

The default `java-tck.ownership-assumption-transfer-epoch` case verifies that
both candidates receive the original tagged ownership offer, one acquires the
attributes, and the previously offered candidate receives a fresh grouped
offer with the new tag after transfer and republication. Object and attribute
handles are resolved per federate through standard API lookups. Focused
installed JNI runs passed in both callback modes; independent-provider
conformance remains unverified.

The default `java-tck.resign-multi-survivor-assumption-fanout` case uses
HLA_EVOKED callbacks to verify that servicing one surviving federate's queued
assumption offer leaves the other's queue untouched. Each empty-tag offer is
checked against receiver-local object and attribute handles. A focused
installed JNI run passed in HLA_EVOKED; HLA_IMMEDIATE is explicitly unsupported
for this queue-ordering assertion. Independent-provider conformance is unverified.

The default `java-tck.negotiated-divestiture-concurrent-assumption` case keeps
a third candidate's tagged assumption visible while owner confirmation is
pending, then checks the tagged acquisition and ownership transfer after
confirmation. It uses receiver-local standard API handles and excludes C++
process transport mechanics. Focused installed JNI runs passed in both callback
modes; independent-provider conformance is unverified.

The default `java-tck.confirm-divestiture-from-callback` case checks the
tagged owner confirmation request, calls Confirm Divestiture from that callback,
and verifies one tagged acquisition and ownership transfer using each
federate's locally resolved standard handles. Callback re-entry is supplemental
public-API evidence; C++ process transport is excluded. Focused installed JNI
runs passed in both callback modes; independent-provider conformance is unverified.

The default `java-tck.confirm-divestiture-mixed-set-atomicity` case submits
one requested and one unrequested attribute together to Confirm Divestiture.
It checks the typed rejection, unchanged ownership and acquisition callbacks,
then successful confirmation of the requested attribute alone. The two-attribute
FOM and names come from the adapter; the requester uses its local standard
handles. Focused installed JNI runs passed in both callback modes; process and
MOM mechanics and independent-provider conformance remain outside this evidence.

The default `java-tck.resign-unconditionally-divest-preserves-objects` scenario
establishes foreign ownership of a surviving federate's object, then resigns
the owner with `UNCONDITIONALLY_DIVEST_ATTRIBUTES`. It verifies that the object
remains registered and discoverable, no removal is delivered by that resignation,
and a third federate can acquire the released attribute with the requested tag.
Only a later `DELETE_OBJECTS` resignation removes the object. The Java scenario
passed both callback models against the adapter-selected 1516.1-2025 API and FOM;
its C++ base/contract pair passed four focused tests in each of two independent
GNU 15.2/Ninja installed-package builds. The Java source uses only the official
API and JDK, with provider, FOM, time implementation, and callback model supplied
by the runner.

The default `java-tck.resign-cancel-pending-ownership-acquisitions-action`
scenario queues a standard ownership acquisition, resigns the requester with
`CANCEL_PENDING_OWNERSHIP_ACQUISITIONS`, and verifies the registered object and
current owner are unchanged. The owner then divests and a successor successfully
acquires the attribute with the expected callback tag before ordinary cleanup;
the successor must not receive an `attributeOwnershipUnavailable` callback.
The Java scenario passes both callback models, and its C++ base/contract pair
passes all four focused evoked/immediate cases in each of two independent
GNU 15.2/Ninja installed-package builds. The test uses only the official
1516.1-2025 Java API; provider, FOM, time implementation, and callback model are
adapter-supplied. Its callback wait helper services callbacks only for the
evoked model, avoiding an illegal evoke call under immediate delivery.

The default `java-tck.automatic-resign-directive-lifecycle-boundaries` scenario
checks that the standard automatic-resign directive getter/setter work while
joined, report `FederateNotExecutionMember` after resignation, and report
`NotConnected` after disconnect. The Java scenario passes with both configured
callback models; its C++ base/contract pair passes all four focused tests in
each of two GNU 15.2/Ninja installed-package builds. Only the standard Java API
is used; provider, FOM, logical-time implementation, and callback model remain
adapter inputs.

The default `java-tck.automatic-resign-directive-standard-enumerator-values`
scenario sets and reads back all six standard Java `ResignAction` constants.
It passes with both callback configurations, and the corresponding C++ base/
contract pair passes all four focused tests in each of two GNU 15.2/Ninja
installed-package builds. The Java enum cannot represent the invalid integer
cast tested by C++; that negative boundary remains explicitly C++-only, while
the shared parity slice covers the standard enumerator values.

The default `java-tck.advisory-switch-value-round-trips` scenario toggles each
of the four standard relevance/scope advisory switches away from its current
value and restores it, checking both getter results. It passes with evoked and
immediate callback configuration, as does the C++ base/contract pair in each of
two independent GNU 15.2/Ninja installed-package builds. The scenario uses only
the official 1516.1-2025 API and adapter-supplied federation inputs.

The default `java-tck.resign-cancel-then-delete-then-divest-cross-object-effects`
scenario exercises the compound `CANCEL_THEN_DELETE_THEN_DIVEST` action across
three distinct objects: cancel the departing federate's pending acquisition,
remove its registered object, and release its ownership on a retained object.
It verifies removal metadata and object-name lookups, then checks that a
successor can acquire both released attributes before the original owner deletes
its two objects. Java passes in both callback models; the C++ base/contract pair
passes the four focused tests in each of two GNU 15.2/Ninja installed-package
builds. The scenario depends only on the official Java API and adapter inputs.
Its bounded callback waits service callbacks only in the evoked model; immediate
delivery never invokes `evokeCallback`.

The `java-tck.synchronization` scenario now uses the adapter-selected callback
model for every participant. Evoked mode explicitly services callbacks;
immediate mode checks the same registration, announcement, and barrier results
without calling the evoked callback service. The scenario remains provider-
unverified until run against an installed Java 1516.1-2025 provider.

## Build

From the repository root, use the shell-free Python runner. These commands are
single lines so they work in any shell:

```text
python tools/run_java_tck.py build --api-jar PATH/TO/hla-1516.1-2025-api.jar --output-directory out/java-tck/classes
```

Only the API JAR is needed at compile time.

## Run

At runtime, place the provider JAR and any provider dependencies on the class
path. The FOM module is required for lifecycle and ordinary data scenarios.

```text
python tools/run_java_tck.py run --api-jar PATH/TO/hla-1516.1-2025-api.jar --provider-jar PATH/TO/provider-rti.jar --fom-path PATH/TO/fom.xml --classes-directory out/java-tck/classes --results-path out/java-tck/provider.results.json --junit-path out/java-tck/provider.junit.xml
```

Use `--dependency-jar` for additional provider dependencies and
`--jvm-argument` for provider-owned JVM flags. The reusable runner does not
interpret those flags. By default, scenarios without Java provider-runtime
evidence remain `unsupported`, even when selected with `--scenario`.
Add `--run-unverified` to attempt those cases against the supplied provider;
this is an execution opt-in, not a conformance or promotion claim. An explicit
`unsupported` or `not_applicable` entry in an optional `--capability-profile`
still wins. Add `--scenario java-tck.<id>` to focus a run on one catalog
scenario; other entries are marked `filtered` in the result report.
For diagnostic exception stacks, pass
`--jvm-argument=-Dhla.rti.tck.stacktrace=true`.

The focused time/FOM parity cases take their fixtures from adapter-supplied
Java properties, passed through `--jvm-argument=-D...`:

- `hla.rti.tck.defaultLogicalTimeFom`: a valid FOM with no declared time
  representation;
- `hla.rti.tck.timeRepresentationValidFom`: a valid FOM documenting
  `HLAinteger64Time`;
- `hla.rti.tck.timeRepresentationBasicDataFom` and
  `hla.rti.tck.timeRepresentationReferenceDataFom`: schema-valid FOMs with
  disallowed time-representation data type categories; and
- `hla.rti.tck.unsupportedTimeImplementation`: a factory name the selected
  provider adapter knows is unavailable.

These inputs enable `java-tck.fom-default-logical-time-selection`,
`java-tck.fom-unavailable-logical-time-factory`,
`java-tck.fom-time-representation-valid`, and
`java-tck.fom-time-representation-rejections`. Leave a case default-disabled
when its provider adapter cannot attest the required input.

The adjacent array-cardinality and dimension-duplicate cases also use
adapter-supplied FOM paths. `arrayCardinalityValidFom` supplies the accepted
scalar/list/range/Dynamic/mixed forms; `arrayCardinalityReversedRangeFom`,
`arrayCardinalityFixedMismatchFom`, and
`arrayCardinalityVariableMismatchFom` supply the three independent rejection
branches. `dimensionDuplicateBaseFom` pairs with
`dimensionDuplicateEqualUniqueFom` for the accepted union or with
`dimensionDuplicateConflictFom` for the upper-bound conflict. Prefix each
name with `hla.rti.tck.` when passing it as a Java property. These fixtures
enable the four corresponding `java-tck.fom-array-cardinality-*` and
`java-tck.fom-dimension-duplicate-*` scenarios only for adapters that supply
and attest the relevant 2025 modules.

The dimension-input cases additionally require the standard MIM via
`--mim-path`. Configure `dimensionInputRawBasicFom`, `dimensionInputNaFom`,
and `dimensionInputTableTypeFom` for the input-type case;
`dimensionDescriptionNaInvalidFom`, `dimensionDescriptionTextFom`,
`dimensionDescriptionNamedNaFom`, and `dimensionDescriptionNamedTextFom`
for the description case; and `dimensionInputMixedNaFom` plus the accepted
NA, described-empty, and table-type FOM paths for the NA-exclusivity case.
All names in this paragraph are `hla.rti.tck.`-prefixed Java properties,
and each provider adapter must supply schema-valid 2025 modules.

Further FOM parity fixtures are also adapter properties: use
`dimensionDuplicateWithinModuleFom` with the two legal duplicate-dimension
modules for the name-uniqueness case; the four `objectClassName*Fom` paths
for local-name conventions; `modelModificationDateTimezoneFom` and
`modelModificationDateExactFom` for date presentation; and
`updateRateZeroPreflightFom` with `updateRatePositivePreflightFom` for
rate positivity. Equal and conflicting cross-module update rates use
`updateRateDuplicateBaseFom` paired respectively with
`updateRateDuplicateEqualFom` or `updateRateDuplicateNoteConflictFom`.
Prefix each property with `hla.rti.tck.` and supply `--mim-path` to the
cases that request the standard MIM. These cases remain default-disabled
until a provider adapter attests its fixtures.

The dimension-default-range case uses `dimensionDefaultRangeOutsideFom`
and `dimensionDefaultRangeBoundedFom`, with the standard MIM. Duplicate
synchronization and transportation rows use a `*DuplicateBaseFom` paired
with either `*DuplicateEqualFom` or `*DuplicateNoteConflictFom`; use the
`synchronization` or `transportation` prefix as appropriate. Pass these as
`hla.rti.tck.`-prefixed Java properties backed by adapter-supplied 2025
modules. The default-disabled status remains until that provider supplies
and attests the fixtures.

The FOM switch-duplicate parity cases use adapter-owned modules for explicit
Disabled, equivalent, Enabled, and omitted values. Supply the properties
`switchDuplicateDisabledFom` paired with `switchDuplicateEqualFom`,
`switchDuplicateEnabledFom`, or `switchDuplicateOmittedFom`; the advisory
variants use the analogous `advisorySwitchDuplicate*Fom` names. Prefix each
name with `hla.rti.tck.` when passing it as a Java property. These cases
remain default-disabled until the selected provider supplies and attests
schema-valid 2025 modules.

`matrix` runs the same compiled classes against two or more provider
configurations. `matrix.example.json` shows the portable configuration shape;
its paths are placeholders relative to that JSON file. Each entry may set
`runUnverified: true` and may omit `capabilityProfile`, or point to a provider
profile whose explicit exclusions take precedence:

```text
python tools/run_java_tck.py matrix --configuration packages/hla-rti-java-tck/matrix.example.json --classes-directory out/java-tck/classes --output-directory out/java-tck/matrix
```

The optional JPype handoff is available directly as
`python packages/hla-rti-java-tck/jpype_smoke.py`.

Scenario/API/requirement traceability is maintained in
`compliance/catalogs/java-tck-scenario-catalog.json` and validated by
`tools/java_tck.py`.

The focused `java-tck.remote-delete-object-instance-name-lookup-boundary` case
checks that object-name/handle lookups round-trip before deletion, then become
unknown to both the deleting federate and a subscriber after remote removal.

The focused `java-tck.resign-delete-object-lookup-boundary` case checks the
standard `DELETE_OBJECTS` resignation callback and verifies that the surviving
member can no longer resolve the removed object's name, handle, or known class.

The focused `java-tck.resign-no-action-owned-attributes-rejection-boundary`
case verifies that `NO_ACTION` is rejected while the federate still owns an
object attribute and that the object's identity remains queryable afterward.

The opt-in `java-tck.timestamped-attribute-update-no-fanout` case publishes and
registers an object without subscribing any federate, then verifies timestamped
updates still return retraction handles, a successful first retraction becomes
non-retractable, and a second queued update becomes non-retractable after the
publisher advances through its timestamp. No reflection or retraction callback
may loop back to the publisher. FOM, logical-time implementation, provider, and
callback model are adapter-supplied; the case is paired with the promoted C++
scenario and remains unsupported pending Java-provider runtime evidence.

The opt-in `java-tck.attribute-update-service-boundaries` case registers an
object with one active and one passive subscriber, then verifies discovery and
ordinary attribute reflection reach both after active relevance is established.
It checks object/attribute/value/tag/producer and resolvable transportation
metadata, and confirms the update does not reflect back to its producer. The
adapter supplies the FOM, names, provider, endpoint, and callback model; this
scenario is paired with the promoted C++ case and remains unsupported pending
Java-provider runtime evidence.

The opt-in `java-tck.dimension-lookup-valid-handles-after-resignation` case
resolves adapter-FOM object, interaction, and dimension handles and verifies
dimension name/upper-bound lookups while joined. After resignation, the same
valid handles must fail dimension and class-available-dimensions queries with
`FederateNotExecutionMember`. It performs no DDM region operations and remains
unsupported pending Java-provider runtime verification.

The opt-in `java-tck.interaction-class-lookup-lifecycle` case checks lookup
connection and membership boundaries, stable base-class handle/name round trips,
unknown-name and invalid-handle failures, and extension interaction visibility
before and after a federate joins with an adapter-supplied additional FOM module.
It binds only to the official Java API; Java's null handle is used for the
invalid-handle input because the API exposes no public invalid-handle
constructor. Provider runtime remains unverified until run against an installed
Java 1516.1-2025 provider.

The opt-in `java-tck.object-class-lookup-lifecycle` case checks object-class
lookup connection and membership boundaries, stable base-class handle/name
round trips, unknown-name and invalid-handle failures, and extension object
class visibility before and after an additional-FOM join. It uses only the
official Java API and adapter-provided FOM/configuration. Java's null handle is
used for the invalid-handle input because the API exposes no public invalid-
handle constructor; provider runtime remains unverified.

The opt-in `java-tck.attribute-lookup-lifecycle` case checks ordinary
attribute-name and handle lookup at connection and membership boundaries,
inherited attribute identity across base/derived classes and federates, and
invalid, unknown, and class-mismatch failures. It requires an adapter-supplied
rich FOM and typed class/attribute names, and uses only the official Java API;
provider runtime remains unverified.

The opt-in `java-tck.parameter-lookup-lifecycle` case checks parameter lookup
boundaries, inherited parameter identity across base/derived interaction classes
and federates, and unknown, invalid, and class-mismatch failures. Its rich FOM,
typed class names, and parameter names are adapter-configurable through the
`hla.rti.tck.typedDerivedInteractionClass` and
`hla.rti.tck.typedDerivedParameter` properties (alongside the existing typed
FOM properties). It uses only the official Java API and remains unverified
against a provider runtime.

The opt-in `java-tck.regional-attribute-value-request-filtering` case exercises
regional `requestAttributeValueUpdateWithRegions` overlap filtering, empty
request sets, default-region object eligibility, invalid foreign/context/
uncommitted request regions, and callback-time re-evaluation for the configured
callback model. In addition to the adapter-supplied DDM FOM, object and
attribute names, and two object dimensions, it requires the unrelated dimension
name through `hla.rti.tck.ddmUnrelatedDimension`; provider runtime remains
unverified.

The opt-in `java-tck.regional-attribute-value-update-response-recheck` case
requests an attribute update, moves the committed requester region out of
overlap before the owner responds, verifies that the response is suppressed,
then restores overlap and checks that a fresh request/response preserves the
value, tag, producer, reliable transportation, and source-region designator.
The DDM FOM, object/attribute names, and two dimensions remain adapter-supplied;
provider runtime remains unverified.

The opt-in `java-tck.regional-interaction-empty-subscription-sets` case checks
that an empty regional subscription creates no delivery route and that an empty
regional unsubscription leaves an existing route intact. It also verifies
ordinary delivery before and after a real regional unsubscription, including
payload, producer, transport, tag, and source-region designator metadata. Its
interaction class, parameter, two dimensions, and DDM FOM are adapter supplied;
provider runtime remains unverified.

The opt-in `java-tck.regional-interaction-source-region-snapshot` case queues
an ordinary regional interaction, mutates its source region to in-range
disjoint bounds before callback servicing in evoked mode, and verifies delivery
retains the source-region designator captured at send time. It also confirms
subsequent disjoint sends are filtered and delivery resumes after restoring overlap. The DDM FOM,
interaction class, parameter, dimensions, and callback model are adapter
supplied; provider runtime remains unverified.

The opt-in `java-tck.regional-interaction-subscription-filtering` case ports the
standard regional interaction filter sequence: ordinary delivery, empty
regional declarations, overlap and disjoint filtering at callback servicing,
source-region designator delivery, and foreign/uncommitted/context-invalid
region rejection. Its DDM FOM, interaction and parameter names, two interaction
dimensions, and unrelated dimension are adapter supplied; the latter uses
`hla.rti.tck.ddmUnrelatedDimension`. Provider runtime remains unverified.

The opt-in `java-tck.regional-interaction-region-validation` case verifies
atomic rejection when a regional interaction send or subscription includes a
region on a dimension unavailable to the interaction class. It proves rejected
operations leave no partial delivery/declaration and that a valid region set
still delivers the expected payload and source-region metadata afterward. The
FOM and interaction, parameter, two valid dimensions, and unrelated dimension
are adapter supplied; the unrelated dimension uses
`hla.rti.tck.ddmUnrelatedDimension`. Provider runtime remains unverified.

The opt-in `java-tck.regional-unpublish-region-release` case associates a
registered object's attribute with a region, verifies deletion is rejected
while the region is in use, then unpublishes that attribute and confirms the
region can be deleted immediately and its former handle is invalid. The DDM
FOM, object class, attribute, and two dimensions are adapter supplied; provider
runtime remains unverified.

The opt-in `java-tck.region-lifecycle` case checks DDM services before connect
and before join, timestamped regional-send boundaries, cross-federate dimension
metadata, region range round-trips and committed changes, absent-dimension
errors, invalid range errors, and invalidation after deletion. Its FOM and
object/interaction classes, attribute, parameter, dimensions, logical-time
implementation, and callback model are adapter supplied. `decodeRegionHandle`
is intentionally not represented because the official Java 2025
`RTIambassador` has no corresponding service; provider runtime remains
unverified.

The opt-in `java-tck.region-range-validation` case accepts a committed range
from zero through the adapter dimension's upper bound, then verifies that
equal and reversed lower/upper bounds fail with `InvalidRangeBound`. The
application FOM, standard MIM, dimension name, logical-time implementation,
and callback model are adapter supplied; provider runtime remains unverified.

The opt-in `java-tck.region-range-dimension-validation` case creates a committed
region containing one adapter dimension and verifies both `getRangeBounds` and
`setRangeBounds` reject a different invalid dimension handle with
`RegionDoesNotContainSpecifiedDimension`. The FOM, standard MIM, dimension
name, logical-time implementation, and callback model are adapter supplied;
provider runtime remains unverified.

The opt-in `java-tck.regional-boundaries` case verifies zero-dimensional region
lifecycle; rejects reads/commits of incomplete regions; rejects object
registration and interaction sends using an unrelated dimension; rejects
foreign-region deletion/commit; and checks that passive subscriptions and
registered-object update associations keep regions in use until unsubscribed or
the object is deleted. The DDM FOM, object/interaction classes, attribute,
parameter, two class dimensions, unrelated dimension, and callback model are
adapter supplied; provider runtime remains unverified.

The opt-in `java-tck.passive-regional-interaction-transition` case starts two
overlapping active regional subscriptions, transitions one to passive and
confirms it remains a delivery target while the other active route exists,
then removes the final active route and confirms both go quiet. An empty
regional subscription does not reactivate the passive route; converting it
back to active restores delivery only to that receiver. Delivered callbacks
are checked for payload, tag, producer, transportation, and source-region
metadata. Its DDM FOM, interaction, parameter, dimensions, and callback model
are adapter supplied; provider runtime remains unverified.

The opt-in `java-tck.default-region-interaction-routing` case contrasts three
delivery paths: an explicit regional send reaches an overlapping regional
subscription but not a disjoint regional declaration; removing that disjoint
declaration restores delivery through the receiver's ordinary subscription;
and an ordinary send reaches both regional subscribers with empty source-region
metadata. The adapter supplies a dimensional interaction FOM, interaction,
parameter, two dimensions, and callback model; provider runtime remains
unverified.

The opt-in `java-tck.zero-dimensional-regional-interaction` case first checks
ordinary default-region delivery, then creates and commits a region with no
dimensions and verifies that an explicit interaction realization using it is
not delivered to the ordinary subscriber. Its dimensional interaction FOM,
interaction, parameter, dimensions, and callback model are adapter supplied;
provider runtime remains unverified.

The opt-in `java-tck.multi-region-interaction-routing` case sends from two
disjoint source regions to separate single-region subscribers, a subscriber
with both regions, and a disjoint subscriber. It verifies union delivery only
once per send, correct filtering, payload and producer metadata, and the exact
source-region set in each callback. The adapter supplies the dimensional FOM,
interaction, parameter, dimensions, and callback model; provider runtime remains
unverified.

The opt-in `java-tck.inherited-interaction-class-delivery` case sends a derived
interaction to both a base-class and a derived-class subscription. It verifies
base-subscription projection to the inherited parameter, full inherited and
derived parameter delivery to the derived subscriber, and that a base-class
interaction does not flow to a derived-only subscription. The adapter supplies
the inherited FOM class/parameter names and callback model; values use standard
HLA integer and Unicode data elements. Provider runtime remains unverified.

The opt-in `java-tck.interaction-closest-ancestor-transition` case uses the
adapter's three-level interaction FOM. As a receiver subscribes to base,
middle, and exact grandchild classes in sequence, it checks one callback per
send at the closest subscribed class. The grandchild name can be supplied with
`hla.rti.tck.typedGrandchildInteractionClass`; the packaged rich FOM defines
`HLAinteractionRoot.TckTypedInteraction.TckTypedDerivedInteraction.TckTypedGrandchildInteraction`.
It is API-compiled but has no Java provider-runtime evidence yet.

The opt-in `java-tck.timestamped-inherited-interaction-class-delivery` case
repeats the derived-to-base projection through timestamp-order delivery. It
checks that constrained subscribers do not receive the interaction before
advancing, then verifies both projected callback payloads, timestamp/order and
retraction metadata, and the aligned time-advance grants. The adapter supplies
the inherited FOM names and logical-time implementation; provider runtime
remains unverified.

The opt-in `java-tck.timed-regional-interaction-save-restore` case queues a
timestamped regional interaction beyond a timed federation-save boundary,
retracts it in the live branch, restores the federation, and uses Flush Queue to
verify that the saved interaction and its original retraction handle are live
again. It checks callback payload, source-region metadata, restore identities,
grant ordering, and the post-delivery retraction callback. The adapter supplies
the DDM FOM, names, provider, and logical-time implementation; provider runtime
remains unverified.

The opt-in `java-tck.timed-default-region-interaction-save-restore` case covers
the same saved-queue/retraction lifecycle for an ordinary timestamped send
routed to an adapter-declared regional subscription. After restore and
Flush Queue, it verifies empty source-region metadata, the restored producer
identity and retraction handle, and delivery-before-grant ordering. The adapter
supplies the dimensional FOM, names, provider, and logical-time implementation;
provider runtime remains unverified.

The opt-in `java-tck.timed-default-region-attribute-save-restore` case registers
an object, queues a timestamped default-region attribute update beyond a timed
save frontier, retracts it in the live branch, then restores and flushes the
saved reflection. It verifies restored object/producer identity, payload,
timestamp, empty source-region metadata, the saved/optimistic Flush Queue
frontier, callback-before-grant order, and reuse of the original retraction
handle. The adapter supplies the DDM FOM, names, provider, and logical-time
implementation; provider runtime remains unverified.

The opt-in `java-tck.timed-regional-attribute-ownership-save-restore` case
restores a queued timestamped update and continues the surviving regular
ownership candidate. It checks regional discovery and source metadata, save
and restore identity, the evoked candidate continuation and immediate
If-Available boundary, Flush Queue delivery-before-grant, and the resulting
ownership transfer. The adapter supplies the dimensional object FOM, object
class/attribute/dimension names, provider, and logical-time implementation;
provider runtime remains unverified.

The opt-in `java-tck.timed-regional-attribute-ownership-confirmation-cancel-save-restore`
case uses the same restored multi-recipient timed update, then verifies that
canceling the owner's negotiated divestiture after its confirmation request
does not transfer ownership, a subsequent confirmation is rejected, and the
remaining acquirer's cancellation callback reports the correct object and
attributes. In immediate-callback mode it exercises the standard
If-Available/If-Wanted path instead. Provider runtime remains unverified.

The opt-in `java-tck.timed-regional-attribute-ownership-pre-delivery-cancel-save-restore`
case cancels the negotiated divestiture before servicing its confirmation
callback. It verifies ownership is unchanged, neither stale confirmation nor
acquisition callback is delivered, and cancellation of the remaining request
reports the expected object and attributes. Immediate-callback mode follows
the standard If-Available/If-Wanted boundary instead. Provider runtime remains
unverified.

The opt-in `java-tck.dimension-lookup-lifecycle` case exercises standard
dimension handle/name/upper-bound lookups and object/interaction class dimension
associations through pre-connect, pre-join, joined, and post-resignation states.
The adapter supplies the FOM, object/interaction class names, and two dimension
names (`hla.rti.tck.dimension` and `hla.rti.tck.ddmSecondDimension`). It creates
no regions and calls no DDM region services; provider runtime remains unverified.

The opt-in `java-tck.handle-normalization-membership-boundaries` case checks
service-group, federate, object-class, interaction-class, and object-instance
normalization before connection, while connected but unjoined, and after
resignation. Its fixture uses the adapter's FOM and standard object/interaction
names and registers one ordinary object; provider runtime remains unverified.

The opt-in `java-tck.timestamped-interaction-source-resignation-fanout` case
queues one timestamped interaction for two constrained recipients, advances one
with TAR and the other with NMR, resigns the producer, and verifies independent
delivery, payload/source/order/retraction metadata, callback-before-grant order,
and each recipient's grant time. Provider, time-managed FOM, logical-time
implementation, and callback model remain adapter-supplied; provider runtime is
unverified.
