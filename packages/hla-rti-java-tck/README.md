# HLA Java RTI Contract TCK

This package is a vendor-neutral executable test set for an IEEE 1516.1-2025
Java RTI provider. The Java sources import only JDK classes and the official
`hla.rti1516_2025` API. Provider selection happens through the standard
`RtiFactoryFactory`/Java service-loader boundary.

Use the [Java TCK guide](../../docs/testing/JAVA-RTI-CONFORMANCE-TCK.md) for
the evidence model and provider-matrix workflow.

## Scope

The catalog currently contains 92 scenarios. The default lane covers factory discovery,
standard encoding, federation creation/join/resign/destroy/query, ordinary
publication/subscription, named and unnamed object registration and discovery,
single and multiple name-reservation callbacks, released-name reuse after object
deletion, instance- and class-scoped attribute value update requests, attribute
update/reflection, object deletion and removal callbacks,
ordinary interaction delivery, support lookups, negative cases, relevance
advisories, directed interactions, derived-target subscription eligibility,
and transportation/order controls. The `java-tck.callback-reentrancy` case
checks the official API's callback-service exception boundary; because the
Requirements Lab has no standalone normative requirement for re-entry, it is
classified as API/exception traceability rather than conformance evidence.
The
directed derived-object target scenario uses adapter-selected typed model-FOM
names and verifies typed parameter delivery only at the registered target,
excluding an observer without a directed subscription. It remains unsupported
by default pending execution against a Java provider.

The `java-tck.federation-membership` scenario also covers the unnamed
two-argument join overload: it verifies that the RTI-assigned federate name
round-trips through handle lookup and appears with the requested federate type
in the standard member-report callback. It also verifies both immediate and
evoked federation-list callback behavior across two live executions, including
that disconnect discards an unserviced evoked report. It distinguishes a
never-created federation from a destroyed federation in member-list callbacks.

The opt-in `java-tck.explicit-mim-creation` scenario uses only the standard
`createFederationExecutionWithMIM` API. Enable it in the provider capability
profile and supply the MIM file with `-Dhla.rti.tck.mim=...`; the adapter also
supplies the caller FOM. It checks rejection of the reserved standard-MIM
designator, shared MOM handles/names across federates, and continued lookup of
the caller's object and interaction declarations.

The opt-in `java-tck.federation-mom-current-fdd` scenario consumes the
adapter-supplied FOM, standard MIM, and an additional extension FOM
(`-Dhla.rti.tck.additionalFom=...`). It checks standard federation-MOM
discovery, reliable transportation and `HLAcurrentFDD` reflection, then
verifies that an additional-module join refreshes the reflected FDD and that a
subsequent explicit value request returns the same composition.

The opt-in `java-tck.federation-mom-fom-module-designator-list` scenario uses
the same adapter-supplied base FOM, MIM, and extension FOM. It decodes the
standard `HLAFOMmoduleDesignatorList` as a variable array of Unicode strings,
compares designators without assuming array order, and checks that the list
changes after an additional-module join and matches a later explicit request.

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

The opt-in `java-tck.object-registration-discovery-lifecycle` scenario checks
typed base/derived-class lookup, inherited attribute identity, publication,
unnamed registration, exact and base-class discovery, evoked callback timing,
unsubscribe cancellation, and late subscription. It uses only the standard
IEEE 1516.1-2025 Java API; the adapter supplies its provider, FOM, endpoint,
callback model, and typed class/attribute names. It remains unsupported by
default until run against a Java provider.

The opt-in `java-tck.object-attribute-declarations-valid-inputs-after-resignation`
scenario uses adapter-supplied object-class and attribute names and checks all six
ordinary object-attribute publication/subscription calls while joined and after
resignation. The calls succeed while joined and report `FederateNotExecutionMember`
after resignation. It remains unsupported by default until run against a Java provider.

The opt-in `java-tck.attribute-value-update-request-baseline` scenario uses an
adapter-supplied multi-attribute FOM plus the `hla.rti.tck.multiAttributeObjectClass`,
`hla.rti.tck.multiAttributeFirst`, and `hla.rti.tck.multiAttributeSecond` properties
when its names differ from the TCK defaults. It checks discovery, instance-scoped
requests, the provider callback's object/attribute set/tag, and suppression when
the requester owns or no federate owns the requested instance attributes. Provider,
endpoint, callback model, and FOM remain adapter-configured. It remains unsupported
by default until run against a Java provider.

The opt-in `java-tck.object-class-attribute-value-update-request-baseline` scenario
uses the adapter's typed base/derived FOM and the
`hla.rti.tck.typedIntegerAttribute` property when the integer attribute differs from
the default. It requests inherited identity and base integer attributes at the base
class, then checks one exact callback per derived instance, inherited-attribute
filtering, request-tag propagation, and suppression for the requester's own instance.
It uses no region services and remains unsupported by default until run against a
Java provider.

The opt-in `java-tck.attribute-value-update-response` scenario uses the adapter's
ordinary object-class and attribute names. It verifies discovery and request-tag
delivery, then has the owner answer with standard `updateAttributeValues` and checks
the reflected value/tag, reliable transportation, producer, and absence of regions.
Provider, FOM, endpoint, logical-time implementation, and callback model remain
adapter-configured. It remains unsupported by default until run against a Java provider.

The opt-in `java-tck.attribute-value-update-request-multi-requester` scenario
uses the adapter's ordinary object-class and attribute names. Two independent
subscribers issue instance-scoped requests with distinct tags; the provider must
receive exactly one matching callback for each, and one ordinary update must be
reflected to both requesters with the expected value and standard receive-order
metadata. It remains unsupported by default until run against a Java provider.

The opt-in `java-tck.object-registration-discovery-multi-recipient` scenario
registers two ordinary objects and verifies that two active subscribers each
receive exactly one discovery per object, with matching producer/class metadata
and stable name/handle/known-class lookups. Provider, FOM, endpoint, and callback
model remain adapter-configured; runtime verification against a Java provider is
still required before enabling it by default.

The opt-in `java-tck.ordinary-multi-attribute-subscription-projection` scenario
uses an adapter-supplied multi-attribute FOM and attribute names. It verifies the
complete update for a two-attribute subscriber, selective value projection for
single-attribute subscribers, preservation of per-update tags and producer
metadata, and suppression of updates for an unsubscribed attribute and the
publisher. Keep it unsupported by default until run against a Java provider.

The opt-in `java-tck.ordinary-multi-attribute-value-update-request-response`
scenario uses the adapter-supplied multi-attribute FOM and names. It exercises
full, first-only, and second-only instance requests, checks the provider callback
attribute sets and request tags, and verifies each ordinary response's values,
tag, producer, reliable transportation, and receive-order metadata. It remains
unsupported by default until run against a Java provider.

The default `java-tck.object-removal-multi-recipient-fifo` scenario deletes two
ordinary objects in sequence and verifies that each of two subscribers receives
both removals in producer order with the corresponding object, tag, and producer
metadata, while the owner receives no loopback. Provider, FOM, endpoint, and
callback model and logical-time implementation remain adapter-configured. The
case passed focused execution against a fresh GNU/Ninja JNI provider build and
is observable through the standard 1516.1-2025 API only.

The opt-in `java-tck.object-deletion-service-boundaries` scenario checks
pre-connection and pre-membership lookup failures, object name/handle round
trips, local deletion without remote callback, ordinary removal tag/producer
metadata, post-deletion lookup failures, and DeleteObjects resignation cleanup.
Its C++ base/contract pair passed two independent fresh GNU 15.2/Ninja
installed-package builds with all four evoked/immediate CTest cases passing in
each build. Java provider execution remains unsupported by default pending
execution against an IEEE 1516.1-2025 Java provider.

The opt-in `java-tck.attribute-multi-recipient-fifo` scenario sends two ordinary
receive-order updates and verifies that both subscribers observe values and tags
in producer order, with matching object/producer and a valid transportation type.
The publisher must receive no reflection. Provider, FOM, endpoint, and callback
model remain adapter-configured; runtime verification against a Java provider is
required before enabling it by default.

The opt-in `java-tck.directed-interaction-declarations-valid-inputs-after-resignation`
scenario uses the adapter's valid object-class/directed-interaction FOM
association and checks successful set and whole-class publication/subscription
overloads while joined, then `FederateNotExecutionMember` for all six standard
overloads after resignation. It is paired with the promoted C++ scenario and
remains unsupported by default until run against a Java 1516.1-2025 provider.

The opt-in `java-tck.service-report-interaction` scenario uses the adapter's
ordinary FOM, standard MIM, logical-time implementation, and callback model.
It enables standard service reporting only for one ordinary `sendInteraction`,
then checks the received `HLAreportServiceInvocation` identity, all standard
parameters, the reporting federate, `SendInteraction` service name and type,
success indicator, empty exception, initial serial number, reliable transport,
empty tag, and absence of regions. Its C++ base scenario passed independent
installed-package runs in both callback models; Java provider execution remains
unsupported by default pending verification with a Java provider.

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

The `java-tck.interaction-subscription-lifecycle` scenario exercises passive
subscription without delivery, activation without replay, reliable interaction
delivery with exact parameter/tag/producer metadata, downgrade back to passive,
and unsubscription. Callback mode, FOM, and names are adapter-supplied. It
remains unsupported by default pending Java-provider execution.

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

The `java-tck.directed-interaction-target-lifecycle` scenario verifies directed
delivery to a discovered object target, cancellation/restoration across
selective subscription and publication changes, callback metadata and delivery
ordering across target deletion, the standard removal callback, and rejection
of a later send to the deleted target. The callback model is selected by the
adapter; the case imports only the IEEE 1516.1-2025 Java API.

The `java-tck.directed-interaction-subscription-kind` scenario distinguishes
by-ownership from universal delivery, checks that an empty by-ownership set
does not disturb a universal declaration, and changes both subscribers' kinds
while the federation is active.

The `java-tck.directed-derived-object-target-eligibility` scenario runs the
same ownership/universal distinction with an adapter-selected derived object
class and inherited identity attribute. It checks target ownership and
universal delivery while excluding subscribers that are not eligible for the
target. Supply a compatible typed FOM and callback model through the adapter;
the scenario remains unsupported by default pending Java-provider execution.

The `java-tck.directed-interaction-multi-recipient-fifo` scenario registers and
publishes one target, then sends two ordinary directed interactions to that
target. It verifies both universal subscribers independently receive the same
FIFO sequence with exact class/target/parameter/value/tag/producer metadata and
resolvable transportation, while the sender receives no loopback. FOM and
declaration names, provider, and callback model are adapter-supplied; the case
remains unsupported by default pending Java-provider execution.

The `java-tck.directed-interaction-mixed-subscription-fanout` scenario
contrasts a by-ownership subscriber, two universal subscribers, and an
unsubscribed member across two targets. It verifies exact target, parameter,
tag, producer, and resolvable transport metadata, producer order, and that
only the universal subscribers receive an interaction addressed to another
member's object. Provider, FOM, names, and callback model are adapter-supplied;
the case remains unsupported by default pending Java-provider execution.

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

The `java-tck.timestamped-interaction-mixed-advances` scenario sends the same
timestamped interaction to three constrained receivers, advanced respectively
by Flush Queue, Time Advance Request Available, and Next Message Request
Available. It checks callback-before-grant ordering, payload and tag, producer,
time/order/retraction metadata, grant times, and logical-time queries. It remains
unsupported by default until run through a compatible provider adapter.

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

The `java-tck.timestamped-attribute-source-resignation` scenario queues an
attribute update, then resigns its producer with unconditional divestiture.
The survivor assumes and acquires the attribute; retraction by the departed
producer must fail with `FederateNotExecutionMember`, while a separate
regulating clock permits the queued update to reach the survivor before its
grant. It checks the original producer and complete timestamped reflection
metadata. Callback mode remains adapter-selected, and the scenario stays
unsupported by default pending Java-provider execution.

The `java-tck.timestamped-attribute-update-alternate-advances` scenario sends
one timestamped attribute update to three constrained receivers using Flush
Queue, Time Advance Request Available, and Next Message Request Available. It
checks reflection-before-grant ordering, payload/tag and producer metadata,
timestamp/order/retraction handles, grant/query consistency, Flush Queue actual
and optimistic bounds, and the terminal retraction exception. It remains
unsupported by default until executed through a compatible provider adapter.

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
uses adapter-selected FOM names and callback model and remains unsupported by
default pending Java-provider execution.

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

## Build

From the repository root, use the shell-free Python runner:

```text
python tools/run_java_tck.py build \
  --api-jar C:/path/to/hla-1516.1-2025-api.jar \
  --output-directory out/java-tck/classes
```

Only the API JAR is needed at compile time.

## Run

At runtime, place the provider JAR and any provider dependencies on the class
path. The FOM module is required for lifecycle and ordinary data scenarios.

```text
python tools/run_java_tck.py run \
  --api-jar C:/path/to/hla-1516.1-2025-api.jar \
  --provider-jar C:/path/to/provider-rti.jar \
  --factory-name "Provider RTI" \
  --fom-path C:/path/to/RestaurantFOMmodule-2025.xml \
  --capability-profile packages/hla-rti-java-tck/profiles/provider.properties \
  --classes-directory out/java-tck/classes \
  --results-path out/java-tck/provider.results.json \
  --junit-path out/java-tck/provider.junit.xml
```

Use `--dependency-jar` for additional provider dependencies and
`--jvm-argument` for provider-owned JVM flags. The reusable runner does not
interpret those flags. Add `--scenario java-tck.<id>` to focus a run on one
catalog scenario; other entries are marked `filtered` in the result report.
For diagnostic exception stacks, pass
`--jvm-argument=-Dhla.rti.tck.stacktrace=true`.

`matrix` runs the same compiled classes against two or more provider
configurations. `matrix.example.json` shows the portable configuration shape:

```text
python tools/run_java_tck.py matrix \
  --configuration packages/hla-rti-java-tck/matrix.example.json \
  --classes-directory out/java-tck/classes \
  --output-directory out/java-tck/matrix
```

The optional JPype handoff is available directly as
`python packages/hla-rti-java-tck/jpype_smoke.py`.

Scenario/API/requirement traceability is maintained in
`compliance/catalogs/java-tck-scenario-catalog.json` and validated by
`tools/java_tck.py`.
